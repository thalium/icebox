/** @file
 *
 * Remote Desktop Protocol client:
 * USB Channel Process Functions
 *
 */

/*
 * Copyright (C) 2006-2011 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* DEBUG is defined in ../rdesktop.h */
#ifdef DEBUG
# define VBOX_DEBUG DEBUG
#endif
#include "../rdesktop.h"
#undef DEBUG
#ifdef VBOX_DEBUG
# define DEBUG VBOX_DEBUG
#endif

#include "vrdpusb.h"
#include "USBProxyDevice.h"
#include "USBGetDevices.h"

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/err.h>
#include <iprt/log.h>

#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>


#define RDPUSB_REQ_OPEN              (0)
#define RDPUSB_REQ_CLOSE             (1)
#define RDPUSB_REQ_RESET             (2)
#define RDPUSB_REQ_SET_CONFIG        (3)
#define RDPUSB_REQ_CLAIM_INTERFACE   (4)
#define RDPUSB_REQ_RELEASE_INTERFACE (5)
#define RDPUSB_REQ_INTERFACE_SETTING (6)
#define RDPUSB_REQ_QUEUE_URB         (7)
#define RDPUSB_REQ_REAP_URB          (8)
#define RDPUSB_REQ_CLEAR_HALTED_EP   (9)
#define RDPUSB_REQ_CANCEL_URB        (10)
#define RDPUSB_REQ_DEVICE_LIST       (11)
#define RDPUSB_REQ_NEGOTIATE         (12)

static VCHANNEL *rdpusb_channel;

/** Well-known locations for the Linux Usbfs virtual file system */
static const struct
{
    /** Expected mount location for Usbfs */
    const char *pcszRoot;
    /** Expected location of the "devices" file */
    const char *pcszDevices;
} g_usbfsPaths[] =
{
    { "/proc/bus/usb", "/proc/bus/usb/devices" },
    { "/dev/bus/usb",  "/dev/bus/usb/devices" }
};

/** Location at which the USB device tree was found.  NULL means not
 * found. */
static const char *g_pcszDevicesRoot = NULL;
static bool g_fUseSysfs = false;

static PUSBDEVICE g_pUsbDevices = NULL;

/* A device list entry */
#pragma pack (1)
typedef struct _DevListEntry
{
	uint16_t oNext;                /* Offset of the next structure. 0 if last. */
	uint32_t id;                   /* Identifier of the device assigned by the client. */
	uint16_t bcdUSB;               /* USB verion number. */
	uint8_t bDeviceClass;          /* Device class. */
	uint8_t bDeviceSubClass;       /* Device subclass. */
	uint8_t bDeviceProtocol;       /* Device protocol. */
	uint16_t idVendor;             /* Vendor id. */
	uint16_t idProduct;            /* Product id. */
	uint16_t bcdRev;               /* Revision. */
	uint16_t oManufacturer;        /* Offset of manufacturer string. */
	uint16_t oProduct;             /* Offset of product string. */
	uint16_t oSerialNumber;        /* Offset of serial string. */
	uint16_t idPort;               /* Physical USB port the device is connected to. */
} DevListEntry;
#pragma pack ()

/**
 * @returns VBox status code.
 */
static inline int op_usbproxy_back_open(PUSBPROXYDEV p, const char *pszAddress)
{
     return g_USBProxyDeviceHost.pfnOpen (p, pszAddress, NULL);
}

static inline void op_usbproxy_back_close(PUSBPROXYDEV pDev)
{
     return g_USBProxyDeviceHost.pfnClose (pDev);
}

/**
 * @returns VBox status code.
 */
static inline int op_usbproxy_back_reset(PUSBPROXYDEV pDev)
{
    return g_USBProxyDeviceHost.pfnReset (pDev, false);
}

/**
 * @returns VBox status code.
 */
static inline int op_usbproxy_back_set_config(PUSBPROXYDEV pDev, int cfg)
{
    return g_USBProxyDeviceHost.pfnSetConfig (pDev, cfg);
}

/**
 * @returns VBox status code.
 */
static inline int op_usbproxy_back_claim_interface(PUSBPROXYDEV pDev, int ifnum)
{
    return g_USBProxyDeviceHost.pfnClaimInterface (pDev, ifnum);
}

/**
 * @returns VBox status code.
 */
static inline int op_usbproxy_back_release_interface(PUSBPROXYDEV pDev, int ifnum)
{
    return g_USBProxyDeviceHost.pfnReleaseInterface (pDev, ifnum);
}

/**
 * @returns VBox status code.
 */
static inline int op_usbproxy_back_interface_setting(PUSBPROXYDEV pDev, int ifnum, int setting)
{
    return g_USBProxyDeviceHost.pfnSetInterface (pDev, ifnum, setting);
}

/**
 * @returns VBox status code.
 */
static inline int op_usbproxy_back_queue_urb(PUSBPROXYDEV pDev, PVUSBURB pUrb)
{
    return g_USBProxyDeviceHost.pfnUrbQueue(pDev, pUrb);
}

static inline PVUSBURB op_usbproxy_back_reap_urb(PUSBPROXYDEV pDev, unsigned cMillies)
{
    return g_USBProxyDeviceHost.pfnUrbReap (pDev, cMillies);
}

/**
 * @returns VBox status code.
 */
static inline int op_usbproxy_back_clear_halted_ep(PUSBPROXYDEV pDev, unsigned EndPoint)
{
    return g_USBProxyDeviceHost.pfnClearHaltedEndpoint (pDev, EndPoint);
}

/**
 * @returns VBox status code.
 */
static inline int op_usbproxy_back_cancel_urb(PUSBPROXYDEV pDev, PVUSBURB pUrb)
{
    return g_USBProxyDeviceHost.pfnUrbCancel (pDev, pUrb);
}


/** Count the USB devices in a linked list of PUSBDEVICE structures. */
unsigned countUSBDevices(PUSBDEVICE pDevices)
{
    unsigned i = 0;
    for (; pDevices; pDevices = pDevices->pNext)
        ++i;
    return i;
}


enum {
    /** The space we set aside for the USB strings.  Should always be enough,
     * as a USB device contains up to 256 characters of UTF-16 string data. */
    MAX_STRINGS_LEN = 1024,
    /** The space we reserve for each wire format device entry */
    DEV_ENTRY_SIZE = sizeof(DevListEntry) + MAX_STRINGS_LEN
};


/**
 * Add a string to the end of a wire format device entry.
 * @param pBuf      the start of the buffer containing the entry
 * @param iBuf      the index into the buffer to add the string at
 * @param pcsz      the string to add - optional
 * @param piString  where to write back @a iBuf or zero if there is no string
 * @param piNext    where to write back the index where the next string may
 *                  start
 */
static void addStringToEntry(char *pBuf, uint16_t iBuf, const char *pcsz,
                             uint16_t *piString, uint16_t *piNext)
{
    size_t cch;

    *piString = 0;
    *piNext = iBuf;
    if (!pcsz)
        return;
    cch = strlen(pcsz) + 1;
    if (cch > DEV_ENTRY_SIZE - iBuf)
        return;
    strcpy(pBuf + iBuf, pcsz);
    *piString = iBuf;
    *piNext = iBuf + cch;
}


/** Fill in a device list entry in wire format from a PUSBDEVICE and return an
 * index to where the next string should start */
static void fillWireListEntry(char *pBuf, PUSBDEVICE pDevice,
                              uint16_t *piNext)
{
    DevListEntry *pEntry;
    uint16_t iNextString = sizeof(DevListEntry);

    pEntry = (DevListEntry *)pBuf;
    pEntry->id              = (pDevice->bPort << 8) + pDevice->bBus;
    pEntry->bcdUSB          = pDevice->bcdUSB;
    pEntry->bDeviceClass    = pDevice->bDeviceClass;
    pEntry->bDeviceSubClass = pDevice->bDeviceSubClass;
    pEntry->idVendor        = pDevice->idVendor;
    pEntry->idProduct       = pDevice->idProduct;
    pEntry->bcdRev          = pDevice->bcdDevice;
    pEntry->idPort          = pDevice->bPort;
    addStringToEntry(pBuf, iNextString, pDevice->pszManufacturer,
                     &pEntry->oManufacturer, &iNextString);
    addStringToEntry(pBuf, iNextString, pDevice->pszProduct,
                     &pEntry->oProduct, &iNextString);
    addStringToEntry(pBuf, iNextString, pDevice->pszSerialNumber,
                     &pEntry->oSerialNumber, &pEntry->oNext);
    *piNext = pEntry->oNext;
}


/** Allocate (and return) a buffer for a device list in VRDP wire format,
 * and populate from a PUSBDEVICE linked list.  @a pLen takes the length of
 * the new list.
 * See @a Console::processRemoteUSBDevices for the receiving end. */
static void *buildWireListFromDevices(PUSBDEVICE pDevices, int *pLen)
{
    char *pBuf;
    unsigned cDevs, cbBuf, iCurrent;
    uint16_t iNext;
    PUSBDEVICE pCurrent;

    cDevs = countUSBDevices(pDevices);
    cbBuf = cDevs * DEV_ENTRY_SIZE + 2;
    pBuf = (char *)xmalloc(cbBuf);
    memset(pBuf, 0, cbBuf);
    for (pCurrent = pDevices, iCurrent = 0; pCurrent;
         pCurrent = pCurrent->pNext, iCurrent += iNext, --cDevs)
    {
        unsigned i, cZeros;

        AssertReturnStmt(iCurrent + DEV_ENTRY_SIZE + 2 <= cbBuf,
                         free(pBuf), NULL);
        fillWireListEntry(pBuf + iCurrent, pCurrent, &iNext);
            DevListEntry *pEntry = (DevListEntry *)(pBuf + iCurrent);
        /* Sanity tests */
        for (i = iCurrent + sizeof(DevListEntry), cZeros = 0;
             i < iCurrent + iNext; ++i)
             if (pBuf[i] == 0)
                 ++cZeros;
        AssertReturnStmt(cZeros ==   RT_BOOL(pEntry->oManufacturer)
                                   + RT_BOOL(pEntry->oProduct)
                                   + RT_BOOL(pEntry->oSerialNumber),
                         free(pBuf), NULL);
        Assert(pEntry->oManufacturer == 0 || pBuf[iCurrent + pEntry->oManufacturer] != '\0');
        Assert(pEntry->oProduct == 0 || pBuf[iCurrent + pEntry->oProduct] != '\0');
        Assert(pEntry->oSerialNumber == 0 || pBuf[iCurrent + pEntry->oSerialNumber] != '\0');
        AssertReturnStmt(cZeros == 0 || pBuf[iCurrent + iNext - 1] == '\0',
                         free(pBuf), NULL);
    }
    *pLen = iCurrent + iNext + 2;
    Assert(cDevs == 0);
    Assert(*pLen <= cbBuf);
    return pBuf;
}


/** Build a list of the usable USB devices currently connected to the client
 * system using the VRDP wire protocol.  The structure returned must be freed
 * using free(3) when it is no longer needed; returns NULL and sets *pLen to
 * zero on failure. */
static void *build_device_list (int *pLen)
{
    void *pvDeviceList;

    Log(("RDPUSB build_device_list"));
    *pLen = 0;
    if (g_pUsbDevices)
        deviceListFree(&g_pUsbDevices);
    g_pUsbDevices = USBProxyLinuxGetDevices(g_pcszDevicesRoot, g_fUseSysfs);
    if (!g_pUsbDevices)
        return NULL;
    pvDeviceList = buildWireListFromDevices(g_pUsbDevices, pLen);
    return pvDeviceList;
}


static STREAM
rdpusb_init_packet(uint32 len, uint8 code)
{
	STREAM s;

	s = channel_init(rdpusb_channel, len + 5);
	out_uint32_le (s, len + sizeof (code)); /* The length of data after the 'len' field. */
	out_uint8(s, code);
	return s;
}

static void
rdpusb_send(STREAM s)
{
#ifdef RDPUSB_DEBUG
	Log(("RDPUSB send:\n"));
	hexdump(s->channel_hdr + 8, s->end - s->channel_hdr - 8);
#endif

	channel_send(s, rdpusb_channel);
}

static void
rdpusb_send_reply (uint8_t code, uint8_t status, uint32_t devid)
{
	STREAM s = rdpusb_init_packet(5, code);
	out_uint8(s, status);
	out_uint32_le(s, devid);
	s_mark_end(s);
	rdpusb_send(s);
}

static void
rdpusb_send_access_denied (uint8_t code, uint32_t devid)
{
    rdpusb_send_reply (code, VRDP_USB_STATUS_ACCESS_DENIED, devid);
}

static inline int
vrdp_usb_status (int rc, VUSBDEV *pdev)
{
	if (!rc || usbProxyFromVusbDev(pdev)->fDetached)
	{
		return VRDP_USB_STATUS_DEVICE_REMOVED;
	}

	return VRDP_USB_STATUS_SUCCESS;
}

static PUSBPROXYDEV g_proxies = NULL;

static PUSBPROXYDEV 
devid2proxy (uint32_t devid)
{
	PUSBPROXYDEV proxy = g_proxies;

	while (proxy && proxy->idVrdp != devid)
	{
		proxy = proxy->pNext;
	}

	return proxy;
}

static void
rdpusb_reap_urbs (void)
{
	STREAM s;

	PVUSBURB pUrb = NULL;

	PUSBPROXYDEV proxy = g_proxies;

	while (proxy)
	{
		pUrb = op_usbproxy_back_reap_urb(proxy, 0);

		if (pUrb)
		{
			int datalen = 0;

			Log(("RDPUSB: rdpusb_reap_urbs: cbData = %d, enmStatus = %d\n", pUrb->cbData, pUrb->enmStatus));

			if (pUrb->enmDir == VUSB_DIRECTION_IN)
			{
				datalen = pUrb->cbData;
			}

			s = rdpusb_init_packet(14 + datalen, RDPUSB_REQ_REAP_URB);
			out_uint32_le(s, proxy->idVrdp);
			out_uint8(s, VRDP_USB_REAP_FLAG_LAST);
			out_uint8(s, pUrb->enmStatus);
			out_uint32_le(s, pUrb->handle);
			out_uint32_le(s, pUrb->cbData);

			if (datalen)
			{
				out_uint8a (s, pUrb->abData, datalen);
			}

			s_mark_end(s);
			rdpusb_send(s);

			if (pUrb->pPrev || pUrb->pNext || pUrb == proxy->pUrbs)
			{
				/* Remove the URB from list. */
				if (pUrb->pPrev)
				{
					pUrb->pPrev->pNext = pUrb->pNext;
				}
				else
				{
					proxy->pUrbs = pUrb->pNext;
				}

				if (pUrb->pNext)
				{
					pUrb->pNext->pPrev = pUrb->pPrev;
				}
			}

#ifdef RDPUSB_DEBUG
			Log(("Going to free %p\n", pUrb));
#endif
			xfree (pUrb);
#ifdef RDPUSB_DEBUG
			Log(("freed %p\n", pUrb));
#endif
		}

		proxy = proxy->pNext;
	}

	return;
}

static void
rdpusb_process(STREAM s)
{
	int rc;

	uint32 len;
	uint8 code;
	uint32 devid;

	PUSBPROXYDEV proxy = NULL;

#ifdef RDPUSB_DEBUG
	Log(("RDPUSB recv:\n"));
	hexdump(s->p, s->end - s->p);
#endif

	in_uint32_le (s, len);
	if (len > s->end - s->p)
	{
		error("RDPUSB: not enough data len = %d, bytes left %d\n", len, s->end - s->p);
		return;
	}

	in_uint8(s, code);

	Log(("RDPUSB recv: len = %d, code = %d\n", len, code));

	switch (code)
	{
		case RDPUSB_REQ_OPEN:
		{
			PUSBDEVICE pDevice;

			in_uint32_le(s, devid);

			proxy = (PUSBPROXYDEV )xmalloc (sizeof (USBPROXYDEV));
			if (!proxy)
			{
				error("RDPUSB: Out of memory allocating proxy backend data\n");
				return;
			}

			memset (proxy, 0, sizeof (USBPROXYDEV));

			proxy->pvInstanceDataR3 = xmalloc(g_USBProxyDeviceHost.cbBackend);
			if (!proxy->pvInstanceDataR3)
			{
				xfree (proxy);
				error("RDPUSB: Out of memory allocating proxy backend data\n");
				return;
			}

			proxy->Dev.pszName = "Remote device";
			proxy->idVrdp = devid;

			for (pDevice = g_pUsbDevices; pDevice; pDevice = pDevice->pNext)
				if ((pDevice->bPort << 8) + pDevice->bBus == devid)
					break;

			rc = pDevice ? op_usbproxy_back_open(proxy, pDevice->pszAddress)
			             : VERR_NOT_FOUND;

			if (rc != VINF_SUCCESS)
			{
				rdpusb_send_access_denied (code, devid);
				xfree (proxy);
				proxy = NULL;
			}
			else
			{
				if (g_proxies)
				{
					g_proxies->pPrev = proxy;
				}

				proxy->pNext = g_proxies;
				g_proxies = proxy;
			}
		} break;

		case RDPUSB_REQ_CLOSE:
		{
			in_uint32_le(s, devid);
			proxy = devid2proxy (devid);

			if (proxy)
			{
				op_usbproxy_back_close(proxy);

				if (proxy->pPrev)
				{
					proxy->pPrev->pNext = proxy->pNext;
				}
				else
				{
					g_proxies = proxy->pNext;
				}

				if (proxy->pNext)
				{
					proxy->pNext->pPrev = proxy->pPrev;
				}

				xfree (proxy->pvInstanceDataR3);
				xfree (proxy);
				proxy = NULL;
			}

			/* No reply. */
		} break;

		case RDPUSB_REQ_RESET:
		{
	        	in_uint32_le(s, devid);
			proxy = devid2proxy (devid);

			if (!proxy)
			{
				rdpusb_send_access_denied (code, devid);
				break;
			}

			rc = op_usbproxy_back_reset(proxy);
			if (rc != VINF_SUCCESS)
			{
				rdpusb_send_reply (code, vrdp_usb_status (!rc, &proxy->Dev), devid);
			}
		} break;

		case RDPUSB_REQ_SET_CONFIG:
		{
			uint8 cfg;

	        	in_uint32_le(s, devid);
			proxy = devid2proxy (devid);

			if (!proxy)
			{
				rdpusb_send_access_denied (code, devid);
				break;
			}

	        	in_uint8(s, cfg);

			rc = op_usbproxy_back_set_config(proxy, cfg);
			if (RT_FAILURE(rc))
			{
				rdpusb_send_reply (code, vrdp_usb_status (rc, &proxy->Dev), devid);
			}
		} break;

		case RDPUSB_REQ_CLAIM_INTERFACE:
		{
			uint8 ifnum;

	        	in_uint32_le(s, devid);
			proxy = devid2proxy (devid);

			if (!proxy)
			{
				rdpusb_send_access_denied (code, devid);
				break;
			}

	        	in_uint8(s, ifnum);
				in_uint8(s, ifnum);

			rc = op_usbproxy_back_claim_interface(proxy, ifnum);
			if (RT_FAILURE(rc))
			{
				rdpusb_send_reply (code, vrdp_usb_status (rc, &proxy->Dev), devid);
			}
		} break;

		case RDPUSB_REQ_RELEASE_INTERFACE:
		{
			uint8 ifnum;

	        	in_uint32_le(s, devid);
			proxy = devid2proxy (devid);

			if (!proxy)
			{
				rdpusb_send_access_denied (code, devid);
				break;
			}

	        	in_uint8(s, ifnum);

			rc = op_usbproxy_back_release_interface(proxy, ifnum);
			if (RT_FAILURE(rc))
			{
				rdpusb_send_reply (code, vrdp_usb_status (rc, &proxy->Dev), devid);
			}
		} break;

		case RDPUSB_REQ_INTERFACE_SETTING:
		{
			uint8 ifnum;
			uint8 setting;

	        	in_uint32_le(s, devid);
			proxy = devid2proxy (devid);

			if (!proxy)
			{
				rdpusb_send_access_denied (code, devid);
				break;
			}

	        	in_uint8(s, ifnum);
	        	in_uint8(s, setting);

			rc = op_usbproxy_back_interface_setting(proxy, ifnum, setting);
			if (RT_FAILURE(rc))
			{
				rdpusb_send_reply (code, vrdp_usb_status (rc, &proxy->Dev), devid);
			}
		} break;

		case RDPUSB_REQ_QUEUE_URB:
		{
			uint32 handle;
			uint8 type;
			uint8 ep;
			uint8 dir;
			uint32 urblen;
			uint32 datalen;

			PVUSBURB pUrb; // struct vusb_urb *urb;

	        	in_uint32_le(s, devid);
			proxy = devid2proxy (devid);

			if (!proxy)
			{
				/* No reply. */
				break;
			}

	        	in_uint32(s, handle);
	        	in_uint8(s, type);
	        	in_uint8(s, ep);
	        	in_uint8(s, dir);
	        	in_uint32(s, urblen);
	        	in_uint32(s, datalen);

			/* Allocate a single block for URB description and data buffer */
			pUrb = (PVUSBURB)xmalloc (sizeof (VUSBURB) +
			                          (urblen <= sizeof (pUrb->abData)? 0: urblen - sizeof (pUrb->abData))
						 );
			memset (pUrb, 0, sizeof (VUSBURB));
			pUrb->pDev = &proxy->Dev;
			pUrb->handle = handle;
			pUrb->enmType = type;
			pUrb->enmStatus = 0;
			pUrb->EndPt = ep;
			pUrb->enmDir = dir;
			pUrb->cbData = urblen;

			Log(("RDPUSB: queued URB handle = %d\n", handle));

			if (datalen)
			{
				in_uint8a (s, pUrb->abData, datalen);
			}

			rc = op_usbproxy_back_queue_urb(proxy, pUrb);

			/* No reply required. */

			if (RT_SUCCESS(rc))
			{
				if (proxy->pUrbs)
				{
					proxy->pUrbs->pPrev = pUrb;
				}

				pUrb->pNext = proxy->pUrbs;
				proxy->pUrbs = pUrb;
			}
			else
			{
				xfree (pUrb);
			}
		} break;

		case RDPUSB_REQ_REAP_URB:
		{
			rdpusb_reap_urbs ();
		} break;

		case RDPUSB_REQ_CLEAR_HALTED_EP:
		{
			uint8 ep;

	        	in_uint32_le(s, devid);
			proxy = devid2proxy (devid);

			if (!proxy)
			{
				rdpusb_send_access_denied (code, devid);
				break;
			}

	        	in_uint8(s, ep);

			rc = op_usbproxy_back_clear_halted_ep(proxy, ep);
			if (RT_FAILURE(rc))
			{
				rdpusb_send_reply (code, vrdp_usb_status (rc, &proxy->Dev), devid);
			}
		} break;

		case RDPUSB_REQ_CANCEL_URB:
		{
			uint32 handle;
			PVUSBURB pUrb = NULL;

	        	in_uint32_le(s, devid);
			proxy = devid2proxy (devid);

			if (!proxy)
			{
				rdpusb_send_access_denied (code, devid);
				break;
			}

	        	in_uint32_le(s, handle);

			pUrb = proxy->pUrbs;

			while (pUrb && pUrb->handle != handle)
			{
				pUrb = pUrb->pNext;
			}

			if (pUrb)
			{
				op_usbproxy_back_cancel_urb(proxy, pUrb);

				/* No reply required. */

				/* Remove URB from list. */
				if (pUrb->pPrev)
				{
					pUrb->pPrev->pNext = pUrb->pNext;
				}
				else
				{
					proxy->pUrbs = pUrb->pNext;
				}

				if (pUrb->pNext)
				{
					pUrb->pNext->pPrev = pUrb->pPrev;
				}

				pUrb->pNext = pUrb->pPrev = NULL;

				Log(("Cancelled URB %p\n", pUrb));

				// xfree (pUrb);
			}
		} break;

		case RDPUSB_REQ_DEVICE_LIST:
		{
			void *buf = NULL;
			int len = 0;

			buf = build_device_list (&len);

			s = rdpusb_init_packet(len? len: 2, code);
			if (len)
			{
				out_uint8p (s, buf, len);
			}
			else
			{
				out_uint16_le(s, 0);
			}
			s_mark_end(s);
			rdpusb_send(s);

			if (buf)
			{
				free (buf);
			}
		} break;

		case RDPUSB_REQ_NEGOTIATE:
		{
			s = rdpusb_init_packet(1, code);
			out_uint8(s, VRDP_USB_CAPS_FLAG_ASYNC);
			s_mark_end(s);
			rdpusb_send(s);
		} break;

		default:
			unimpl("RDPUSB code %d\n", code);
			break;
	}
}

void
rdpusb_add_fds(int *n, fd_set * rfds, fd_set * wfds)
{
	PUSBPROXYDEV proxy = g_proxies;

//	Log(("RDPUSB: rdpusb_add_fds: begin *n = %d\n", *n));

	while (proxy)
	{
		int fd = USBProxyDeviceLinuxGetFD(proxy);

		if (fd != -1)
		{
//		        Log(("RDPUSB: rdpusb_add_fds: adding %d\n", proxy->priv.File));

			FD_SET(fd, rfds);
			FD_SET(fd, wfds);
			*n = MAX(*n, fd);
		}

		proxy = proxy->pNext;
	}

//	Log(("RDPUSB: rdpusb_add_fds: end *n = %d\n", *n));

	return;
}

void
rdpusb_check_fds(fd_set * rfds, fd_set * wfds)
{
	PUSBPROXYDEV proxy = g_proxies;
	unsigned found = 0;

	while (proxy)
	{
		int fd = USBProxyDeviceLinuxGetFD(proxy);

		if (fd != -1)
		{
			if (FD_ISSET(fd, rfds))
                found = 1;
			if (FD_ISSET(fd, wfds))
                found = 1;
		}

		proxy = proxy->pNext;
	}

//	Log(("RDPUSB: rdpusb_check_fds: begin\n"));

	if (found)
        rdpusb_reap_urbs ();

//	Log(("RDPUSB: rdpusb_check_fds: end\n"));

	return;
}


RD_BOOL
rdpusb_init(void)
{
    bool fUseUsbfs;
    if (RT_SUCCESS(USBProxyLinuxChooseMethod(&fUseUsbfs, &g_pcszDevicesRoot)))
	{
	    g_fUseSysfs = !fUseUsbfs;
	    rdpusb_channel =
		    channel_register("vrdpusb", CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP,
				     rdpusb_process);
	    return (rdpusb_channel != NULL);
	}
	return false;
}

void
rdpusb_close (void)
{
	PUSBPROXYDEV proxy = g_proxies;

	while (proxy)
	{
		PUSBPROXYDEV pNext = proxy->pNext;

		Log(("RDPUSB: closing proxy %p\n", proxy));

		op_usbproxy_back_close(proxy);
		xfree (proxy);

		proxy = pNext;
	}

	return;
}
