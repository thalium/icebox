/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_PROTOCOL_H
#define CR_PROTOCOL_H

#include <iprt/types.h>
#include <iprt/cdefs.h>
#ifdef DEBUG_misha
#include "cr_error.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CR_CMDVBVA_VERSION              1

#pragma pack(1)
typedef struct CR_CAPS_INFO
{
    uint32_t u32Caps;
    uint32_t u32CmdVbvaVersion;
} CR_CAPS_INFO;
#pragma pack()


/*For now guest is allowed to connect host opengl service if protocol version matches exactly*/
/*Note: that after any change to this file, or glapi_parser\apispec.txt version should be changed*/
#define CR_PROTOCOL_VERSION_MAJOR 9
#define CR_PROTOCOL_VERSION_MINOR 1

/* new TexPresent mechanism is available */
#define CR_VBOX_CAP_TEX_PRESENT         0x00000001
/* vbva command submission mechanism supported */
#define CR_VBOX_CAP_CMDVBVA             0x00000002
/* host supports Command Blocks, i.e. CR_CMDBLOCKBEGIN_OPCODE and CR_CMDBLOCKEND_OPCODE opcodes.
 * Command Block can be used by guest to prevent clients from blocking each other.
 * The Command Block allows multiple command buffers to be processed with one run.
 * Command Block commands have to obey to the following rules:
 * CR_CMDBLOCKBEGIN_OPCODE - must be the first command in the command buffer, specifying the command block start
 * CR_CMDBLOCKEND_OPCODE - must be the last command in the command buffer, specifying the command block end
 * If not placed accordingly, CR_CMDBLOCK** commands are ignored.
 * Server copies the command block buffer commands to its internal storage
 * and processes them with one run when the command block end is signalled
 */
#define CR_VBOX_CAP_CMDBLOCKS            0x00000004
/* GetAttribsLocations support */
#define CR_VBOX_CAP_GETATTRIBSLOCATIONS  0x00000008
/* flush command blocks for execution  */
#define CR_VBOX_CAP_CMDBLOCKS_FLUSH      0x00000010
/* Notify guest if host reports minimal OpenGL capabilities. */
#define CR_VBOX_CAP_HOST_CAPS_NOT_SUFFICIENT 0x00000020

#define CR_VBOX_CAPS_ALL                 0x0000003f


#define CR_PRESENT_SCREEN_MASK 0xffff
#define CR_PRESENT_FLAGS_OFFSET 16
#define CR_PRESENT_FLAGS_MASK 0xffff0000
#define CR_PRESENT_DEFINE_FLAG(_f) (1 << (CR_PRESENT_FLAGS_OFFSET + _f))

#define CR_PRESENT_FLAG_CLEAR_RECTS            CR_PRESENT_DEFINE_FLAG(0)
#define CR_PRESENT_FLAG_TEX_NONINVERT_YCOORD   CR_PRESENT_DEFINE_FLAG(1)

#define CR_PRESENT_GET_SCREEN(_cfg) ((_cfg) & CR_PRESENT_SCREEN_MASK)
#define CR_PRESENT_GET_FLAGS(_cfg) ((_cfg) & CR_PRESENT_FLAGS_MASK)

typedef enum {
    /* first message types is 'wGL\001', so we can immediately
         recognize bad message types */
    CR_MESSAGE_OPCODES = 0x77474c01,
    CR_MESSAGE_WRITEBACK,
    CR_MESSAGE_READBACK,
    CR_MESSAGE_READ_PIXELS,
    CR_MESSAGE_MULTI_BODY,
    CR_MESSAGE_MULTI_TAIL,
    CR_MESSAGE_FLOW_CONTROL,
    CR_MESSAGE_OOB,
    CR_MESSAGE_NEWCLIENT,
    CR_MESSAGE_GATHER,
    CR_MESSAGE_ERROR,
    CR_MESSAGE_CRUT,
    CR_MESSAGE_REDIR_PTR
} CRMessageType;

typedef union {
    /* pointers are usually 4 bytes, but on 64-bit machines (Alpha,
     * SGI-n64, etc.) they are 8 bytes.  Pass network pointers around
     * as an opaque array of bytes, since the interpretation & size of
     * the pointer only matter to the machine which emitted the
     * pointer (and will eventually receive the pointer back again) */
    unsigned int  ptrAlign[2];
    unsigned char ptrSize[8];
    /* hack to make this packet big */
    /* unsigned int  junk[512]; */
} CRNetworkPointer;

#if 0 /*def DEBUG_misha*/
#define CRDBGPTR_SETZ(_p) crMemset((_p), 0, sizeof (CRNetworkPointer))
#define CRDBGPTR_CHECKZ(_p) do { \
        CRNetworkPointer _ptr = {0}; \
        CRASSERT(!crMemcmp((_p), &_ptr, sizeof (CRNetworkPointer))); \
    } while (0)
#define CRDBGPTR_CHECKNZ(_p) do { \
        CRNetworkPointer _ptr = {0}; \
        CRASSERT(crMemcmp((_p), &_ptr, sizeof (CRNetworkPointer))); \
    } while (0)
# if 0
#  define _CRDBGPTR_PRINT(_tStr, _id, _p) do { \
        crDebug(_tStr "%d:0x%08x%08x", (_id), (_p)->ptrAlign[1], (_p)->ptrAlign[0]); \
    } while (0)
# else
#  define _CRDBGPTR_PRINT(_tStr, _id, _p) do { } while (0)
# endif
#define CRDBGPTR_PRINTWB(_id, _p) _CRDBGPTR_PRINT("wbptr:", _id, _p)
#define CRDBGPTR_PRINTRB(_id, _p) _CRDBGPTR_PRINT("rbptr:", _id, _p)
#else
#define CRDBGPTR_SETZ(_p) do { } while (0)
#define CRDBGPTR_CHECKZ(_p) do { } while (0)
#define CRDBGPTR_CHECKNZ(_p) do { } while (0)
#define CRDBGPTR_PRINTWB(_id, _p) do { } while (0)
#define CRDBGPTR_PRINTRB(_id, _p) do { } while (0)
#endif

#ifdef VBOX_WITH_CRHGSMI
typedef struct CRVBOXHGSMI_CMDDATA {
    union
    {
        struct VBOXVDMACMD_CHROMIUM_CMD *pHgsmiCmd;
        struct VBOXCMDVBVA_CRCMD_CMD *pVbvaCmd;
        const void *pvCmd;
    };
    int          *pCmdRc;
    char         *pWriteback;
    unsigned int *pcbWriteback;
    unsigned int cbWriteback;
    bool fHgsmiCmd;
} CRVBOXHGSMI_CMDDATA, *PCRVBOXHGSMI_CMDDATA;

#ifdef DEBUG
# define CRVBOXHGSMI_CMDDATA_ASSERT_CONSISTENT(_pData)  do { \
        CRASSERT(!(_pData)->pvCmd == !(_pData)->pCmdRc); \
        CRASSERT(!(_pData)->pWriteback == !(_pData)->pcbWriteback); \
        CRASSERT(!(_pData)->pWriteback == !(_pData)->cbWriteback); \
        if ((_pData)->pWriteback) \
        { \
            CRASSERT((_pData)->pvCmd); \
        } \
    } while (0)

# define CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(_pData)  do { \
        CRASSERT(!(_pData)->pvCmd); \
        CRASSERT(!(_pData)->pCmdRc); \
        CRASSERT(!(_pData)->pWriteback); \
        CRASSERT(!(_pData)->pcbWriteback); \
        CRASSERT(!(_pData)->cbWriteback); \
    } while (0)

# define CRVBOXHGSMI_CMDDATA_ASSERT_ISSET(_pData)  do { \
        CRVBOXHGSMI_CMDDATA_ASSERT_CONSISTENT(_pData); \
        CRASSERT(CRVBOXHGSMI_CMDDATA_IS_SET(_pData)); \
    } while (0)

# define CRVBOXHGSMI_CMDDATA_ASSERT_ISSETWB(_pData)  do { \
        CRVBOXHGSMI_CMDDATA_ASSERT_CONSISTENT(_pData); \
        CRASSERT(CRVBOXHGSMI_CMDDATA_IS_SETWB(_pData)); \
    } while (0)
#else
# define CRVBOXHGSMI_CMDDATA_ASSERT_CONSISTENT(_pData)  do { } while (0)
# define CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(_pData)  do { } while (0)
# define CRVBOXHGSMI_CMDDATA_ASSERT_ISSET(_pData)  do { } while (0)
# define CRVBOXHGSMI_CMDDATA_ASSERT_ISSETWB(_pData)  do { } while (0)
#endif

#define CRVBOXHGSMI_CMDDATA_IS_HGSMICMD(_pData) (!!(_pData)->fHgsmiCmd)
#define CRVBOXHGSMI_CMDDATA_IS_SET(_pData) (!!(_pData)->pvCmd)
#define CRVBOXHGSMI_CMDDATA_IS_SETWB(_pData) (!!(_pData)->pWriteback)

#define CRVBOXHGSMI_CMDDATA_CLEANUP(_pData) do { \
        crMemset(_pData, 0, sizeof (CRVBOXHGSMI_CMDDATA)); \
        CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(_pData); \
        CRVBOXHGSMI_CMDDATA_ASSERT_CONSISTENT(_pData); \
    } while (0)

#define CRVBOXHGSMI_CMDDATA_SET(_pData, _pCmd, _pHdr, _fHgsmiCmd) do { \
        CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(_pData); \
        (_pData)->pvCmd = (_pCmd); \
        (_pData)->pCmdRc = &(_pHdr)->result; \
        (_pData)->fHgsmiCmd = (_fHgsmiCmd); \
        CRVBOXHGSMI_CMDDATA_ASSERT_CONSISTENT(_pData); \
    } while (0)

#define CRVBOXHGSMI_CMDDATA_SETWB(_pData, _pCmd, _pHdr, _pWb, _cbWb, _pcbWb, _fHgsmiCmd) do { \
        CRVBOXHGSMI_CMDDATA_SET(_pData, _pCmd, _pHdr, _fHgsmiCmd); \
        (_pData)->pWriteback = (_pWb); \
        (_pData)->pcbWriteback = (_pcbWb); \
        (_pData)->cbWriteback = (_cbWb); \
        CRVBOXHGSMI_CMDDATA_ASSERT_CONSISTENT(_pData); \
    } while (0)

#define CRVBOXHGSMI_CMDDATA_RC(_pData, _rc) do { \
        *(_pData)->pCmdRc = (_rc); \
    } while (0)
#endif

typedef struct {
    CRMessageType          type;
    unsigned int           conn_id;
} CRMessageHeader;

typedef struct CRMessageOpcodes {
    CRMessageHeader        header;
    unsigned int           numOpcodes;
} CRMessageOpcodes;

typedef struct CRMessageRedirPtr {
    CRMessageHeader        header;
    CRMessageHeader*       pMessage;
#ifdef VBOX_WITH_CRHGSMI
    CRVBOXHGSMI_CMDDATA   CmdData;
#endif
} CRMessageRedirPtr;

typedef struct CRMessageWriteback {
    CRMessageHeader        header;
    CRNetworkPointer       writeback_ptr;
} CRMessageWriteback;

typedef struct CRMessageReadback {
    CRMessageHeader        header;
    CRNetworkPointer       writeback_ptr;
    CRNetworkPointer       readback_ptr;
} CRMessageReadback;

typedef struct CRMessageMulti {
    CRMessageHeader        header;
} CRMessageMulti;

typedef struct CRMessageFlowControl {
    CRMessageHeader        header;
    unsigned int           credits;
} CRMessageFlowControl;

typedef struct CRMessageReadPixels {
    CRMessageHeader        header;
    int                    width, height;
    unsigned int           bytes_per_row;
    unsigned int           stride;
    int                    alignment;
    int                    skipRows;
    int                    skipPixels;
    int                    rowLength;
    int                    format;
    int                    type;
    CRNetworkPointer       pixels;
} CRMessageReadPixels;

typedef struct CRMessageNewClient {
    CRMessageHeader        header;
} CRMessageNewClient;

typedef struct CRMessageGather {
    CRMessageHeader        header;
    unsigned long           offset;
    unsigned long           len;
} CRMessageGather;

typedef union {
    CRMessageHeader      header;
    CRMessageOpcodes     opcodes;
    CRMessageRedirPtr    redirptr;
    CRMessageWriteback   writeback;
    CRMessageReadback    readback;
    CRMessageReadPixels  readPixels;
    CRMessageMulti       multi;
    CRMessageFlowControl flowControl;
    CRMessageNewClient   newclient;
    CRMessageGather      gather;
} CRMessage;

DECLEXPORT(void) crNetworkPointerWrite( CRNetworkPointer *dst, void *src );

#ifdef __cplusplus
}
#endif

#endif /* CR_PROTOCOL_H */
