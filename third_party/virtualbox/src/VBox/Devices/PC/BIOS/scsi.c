/* $Id: scsi.c $ */
/** @file
 * SCSI host adapter driver to boot from SCSI disks
 */

/*
 * Copyright (C) 2004-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <stdint.h>
#include <string.h>
#include "biosint.h"
#include "inlines.h"
#include "pciutil.h"
#include "ebda.h"


#if DEBUG_SCSI
# define DBG_SCSI(...)  BX_INFO(__VA_ARGS__)
#else
# define DBG_SCSI(...)
#endif

#define VBSCSI_BUSY     (1 << 0)
#define VBSCSI_ERROR    (1 << 1)

/* The I/O port of the BusLogic SCSI adapter. */
#define BUSLOGIC_BIOS_IO_PORT       0x430
/* The I/O port of the LsiLogic SCSI adapter. */
#define LSILOGIC_BIOS_IO_PORT       0x434
/* The I/O port of the LsiLogic SAS adapter. */
#define LSILOGIC_SAS_BIOS_IO_PORT   0x438

#define VBSCSI_REGISTER_STATUS   0
#define VBSCSI_REGISTER_COMMAND  0
#define VBSCSI_REGISTER_DATA_IN  1
#define VBSCSI_REGISTER_IDENTIFY 2
#define VBSCSI_REGISTER_RESET    3
#define VBSCSI_REGISTER_DEVSTAT  3

#define VBSCSI_MAX_DEVICES 16 /* Maximum number of devices a SCSI device can have. */

/* Command opcodes. */
#define SCSI_SERVICE_ACT   0x9e
#define SCSI_INQUIRY       0x12
#define SCSI_READ_CAP_10   0x25
#define SCSI_READ_10       0x28
#define SCSI_WRITE_10      0x2a
#define SCSI_READ_CAP_16   0x10    /* Not an opcode by itself, sub-action for the "Service Action" */
#define SCSI_READ_16       0x88
#define SCSI_WRITE_16      0x8a

/* Data transfer direction. */
#define SCSI_TXDIR_FROM_DEVICE 0
#define SCSI_TXDIR_TO_DEVICE   1

#pragma pack(1)

/* READ_10/WRITE_10 CDB layout. */
typedef struct {
    uint16_t    command;    /* Command. */
    uint32_t    lba;        /* LBA, MSB first! */
    uint8_t     pad1;       /* Unused. */
    uint16_t    nsect;      /* Sector count, MSB first! */
    uint8_t     pad2;       /* Unused. */
} cdb_rw10;

/* READ_16/WRITE_16 CDB layout. */
typedef struct {
    uint16_t    command;    /* Command. */
    uint64_t    lba;        /* LBA, MSB first! */
    uint32_t    nsect32;    /* Sector count, MSB first! */
    uint8_t     pad1;       /* Unused. */
    uint8_t     pad2;       /* Unused. */
} cdb_rw16;

#pragma pack()

ct_assert(sizeof(cdb_rw10) == 10);
ct_assert(sizeof(cdb_rw16) == 16);

void insb_discard(unsigned nbytes, unsigned port);
#pragma aux insb_discard =  \
    ".286"                  \
    "again:"                \
    "in al,dx"              \
    "loop again"            \
    parm [cx] [dx] modify exact [cx ax] nomemory;


int scsi_cmd_data_in(uint16_t io_base, uint8_t target_id, uint8_t __far *aCDB,
                     uint8_t cbCDB, uint8_t __far *buffer, uint32_t length)
{
    /* Check that the adapter is ready. */
    uint8_t     status, sizes;
    uint16_t    i;

    do
        status = inb(io_base + VBSCSI_REGISTER_STATUS);
    while (status & VBSCSI_BUSY);

    sizes = ((length >> 12) & 0xF0) | ((cbCDB == 16) ? 0 : cbCDB);
    outb(io_base + VBSCSI_REGISTER_COMMAND, target_id);                 /* Write the target ID. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, SCSI_TXDIR_FROM_DEVICE);    /* Write the transfer direction. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, sizes);                     /* Write CDB size and top bufsize bits. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, length);                    /* Write the buffer size. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, (length >> 8));
    for (i = 0; i < cbCDB; i++)                                         /* Write the CDB. */
        outb(io_base + VBSCSI_REGISTER_COMMAND, aCDB[i]);

    /* Now wait for the command to complete. */
    do
        status = inb(io_base + VBSCSI_REGISTER_STATUS);
    while (status & VBSCSI_BUSY);

    /* If any error occurred, inform the caller and don't bother reading the data. */
    if (status & VBSCSI_ERROR) {
        outb(io_base + VBSCSI_REGISTER_RESET, 0);

        status = inb(io_base + VBSCSI_REGISTER_DEVSTAT);
        DBG_SCSI("%s: read failed, device status %02X\n", __func__, status);
        return 4;   /* Sector not found */
    }

    /* Read in the data. The transfer length may be exactly 64K or more,
     * which needs a bit of care when we're using 16-bit 'rep ins'.
     */
    while (length > 32768) {
        DBG_SCSI("%s: reading 32K to %X:%X\n", __func__, FP_SEG(buffer), FP_OFF(buffer));
        rep_insb(buffer, 32768, io_base + VBSCSI_REGISTER_DATA_IN);
        length -= 32768;
        buffer = (FP_SEG(buffer) + (32768 >> 4)) :> FP_OFF(buffer);
    }

    DBG_SCSI("%s: reading %ld bytes to %X:%X\n", __func__, length, FP_SEG(buffer), FP_OFF(buffer));
    rep_insb(buffer, length, io_base + VBSCSI_REGISTER_DATA_IN);

    return 0;
}

int scsi_cmd_data_out(uint16_t io_base, uint8_t target_id, uint8_t __far *aCDB,
                      uint8_t cbCDB, uint8_t __far *buffer, uint32_t length)
{
    /* Check that the adapter is ready. */
    uint8_t     status, sizes;
    uint16_t    i;

    do
        status = inb(io_base + VBSCSI_REGISTER_STATUS);
    while (status & VBSCSI_BUSY);


    sizes = ((length >> 12) & 0xF0) | ((cbCDB == 16) ? 0 : cbCDB);
    outb(io_base + VBSCSI_REGISTER_COMMAND, target_id);                 /* Write the target ID. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, SCSI_TXDIR_TO_DEVICE);      /* Write the transfer direction. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, sizes);                     /* Write CDB size and top bufsize bits. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, length);                    /* Write the buffer size. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, (length >> 8));
    for (i = 0; i < cbCDB; i++)                                         /* Write the CDB. */
        outb(io_base + VBSCSI_REGISTER_COMMAND, aCDB[i]);

    /* Write out the data. The transfer length may be exactly 64K or more,
     * which needs a bit of care when we're using 16-bit 'rep outs'.
     */
    while (length > 32768) {
        DBG_SCSI("%s: writing 32K from %X:%X\n", __func__, FP_SEG(buffer), FP_OFF(buffer));
        rep_outsb(buffer, 32768, io_base + VBSCSI_REGISTER_DATA_IN);
        length -= 32768;
        buffer = (FP_SEG(buffer) + (32768 >> 4)) :> FP_OFF(buffer);
    }

    DBG_SCSI("%s: writing %ld bytes from %X:%X\n", __func__, length, FP_SEG(buffer), FP_OFF(buffer));
    rep_outsb(buffer, length, io_base + VBSCSI_REGISTER_DATA_IN);

    /* Now wait for the command to complete. */
    do
        status = inb(io_base + VBSCSI_REGISTER_STATUS);
    while (status & VBSCSI_BUSY);

    /* If any error occurred, inform the caller. */
    if (status & VBSCSI_ERROR) {
        outb(io_base + VBSCSI_REGISTER_RESET, 0);

        status = inb(io_base + VBSCSI_REGISTER_DEVSTAT);
        DBG_SCSI("%s: write failed, device status %02X\n", __func__, status);
        return 4;   /* Sector not found */
    }

    return 0;
}

/**
 * Read sectors from an attached SCSI device.
 *
 * @returns status code.
 * @param   bios_dsk    Pointer to disk request packet (in the
 *                      EBDA).
 */
int scsi_read_sectors(bio_dsk_t __far *bios_dsk)
{
    uint8_t             rc;
    cdb_rw16            cdb;
    uint32_t            count;
    uint16_t            io_base;
    uint8_t             target_id;
    uint8_t             device_id;

    device_id = VBOX_GET_SCSI_DEVICE(bios_dsk->drqp.dev_id);
    if (device_id > BX_MAX_SCSI_DEVICES)
        BX_PANIC("%s: device_id out of range %d\n", __func__, device_id);

    count    = bios_dsk->drqp.nsect;

    /* Prepare a CDB. */
    cdb.command = SCSI_READ_16;
    cdb.lba     = swap_64(bios_dsk->drqp.lba);
    cdb.pad1    = 0;
    cdb.nsect32 = swap_32(count);
    cdb.pad2    = 0;


    io_base   = bios_dsk->scsidev[device_id].io_base;
    target_id = bios_dsk->scsidev[device_id].target_id;

    DBG_SCSI("%s: reading %u sectors, device %d, target %d\n", __func__,
             count, device_id, bios_dsk->scsidev[device_id].target_id);

    rc = scsi_cmd_data_in(io_base, target_id, (void __far *)&cdb, 16,
                          bios_dsk->drqp.buffer, (count * 512L));

    if (!rc)
    {
        bios_dsk->drqp.trsfsectors = count;
        bios_dsk->drqp.trsfbytes   = count * 512L;
    }
    DBG_SCSI("%s: transferred %u sectors\n", __func__, bios_dsk->drqp.nsect);

    return rc;
}

/**
 * Write sectors to an attached SCSI device.
 *
 * @returns status code.
 * @param   bios_dsk    Pointer to disk request packet (in the
 *                      EBDA).
 */
int scsi_write_sectors(bio_dsk_t __far *bios_dsk)
{
    uint8_t             rc;
    cdb_rw16            cdb;
    uint32_t            count;
    uint16_t            io_base;
    uint8_t             target_id;
    uint8_t             device_id;

    device_id = VBOX_GET_SCSI_DEVICE(bios_dsk->drqp.dev_id);
    if (device_id > BX_MAX_SCSI_DEVICES)
        BX_PANIC("%s: device_id out of range %d\n", __func__, device_id);

    count    = bios_dsk->drqp.nsect;

    /* Prepare a CDB. */
    cdb.command = SCSI_WRITE_16;
    cdb.lba     = swap_64(bios_dsk->drqp.lba);
    cdb.pad1    = 0;
    cdb.nsect32 = swap_32(count);
    cdb.pad2    = 0;

    io_base   = bios_dsk->scsidev[device_id].io_base;
    target_id = bios_dsk->scsidev[device_id].target_id;

    DBG_SCSI("%s: writing %u sectors, device %d, target %d\n", __func__,
             count, device_id, bios_dsk->scsidev[device_id].target_id);

    rc = scsi_cmd_data_out(io_base, target_id, (void __far *)&cdb, 16,
                           bios_dsk->drqp.buffer, (count * 512L));

    if (!rc)
    {
        bios_dsk->drqp.trsfsectors = count;
        bios_dsk->drqp.trsfbytes   = (count * 512L);
    }
    DBG_SCSI("%s: transferred %u sectors\n", __func__, bios_dsk->drqp.nsect);

    return rc;
}


/// @todo move
#define ATA_DATA_NO      0x00
#define ATA_DATA_IN      0x01
#define ATA_DATA_OUT     0x02

/**
 * Perform a "packet style" read with supplied CDB.
 *
 * @returns status code.
 * @param   device_id   ID of the device to access.
 * @param   cmdlen      Length of the CDB.
 * @param   cmdbuf      The CDB buffer.
 * @param   before      How much to skip before reading into the provided data buffer.
 * @param   length      How much to transfer.
 * @param   inout       Read/Write direction indicator.
 * @param   buffer      Data buffer to store the data from the device in.
 */
uint16_t scsi_cmd_packet(uint16_t device_id, uint8_t cmdlen, char __far *cmdbuf,
                         uint16_t before, uint32_t length, uint8_t inout, char __far *buffer)
{
    bio_dsk_t __far *bios_dsk = read_word(0x0040, 0x000E) :> &EbdaData->bdisk;
    uint32_t        read_len;
    uint8_t         status, sizes;
    uint16_t        i;
    uint16_t        io_base;
    uint8_t         target_id;

    /* Data out is currently not supported. */
    if (inout == ATA_DATA_OUT) {
        BX_INFO("%s: DATA_OUT not supported yet\n", __func__);
        return 1;
    }

    /* Convert to SCSI specific device number. */
    device_id = VBOX_GET_SCSI_DEVICE(device_id);

    DBG_SCSI("%s: reading %lu bytes, skip %u/%u, device %d, target %d\n", __func__,
             length, bios_dsk->drqp.skip_b, bios_dsk->drqp.skip_a,
             device_id, bios_dsk->scsidev[device_id].target_id);
    DBG_SCSI("%s: reading %u %u-byte sectors\n", __func__,
             bios_dsk->drqp.nsect, bios_dsk->drqp.sect_sz);

    cmdlen -= 2; /* ATAPI uses 12-byte command packets for a READ 10. */

    io_base   = bios_dsk->scsidev[device_id].io_base;
    target_id = bios_dsk->scsidev[device_id].target_id;

    /* Wait until the adapter is ready. */
    do
        status = inb(io_base + VBSCSI_REGISTER_STATUS);
    while (status & VBSCSI_BUSY);

    /* On the SCSI level, we have to transfer whole sectors. */
    /* NB: With proper residual length support, this should not be necessary; we should
     * be able to avoid transferring the 'after' part of the sector.
     */
    read_len = length + before + bios_dsk->drqp.skip_a;

    sizes = (((read_len) >> 12) & 0xF0) | cmdlen;
    outb(io_base + VBSCSI_REGISTER_COMMAND, target_id);                 /* Write the target ID. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, SCSI_TXDIR_FROM_DEVICE);    /* Write the transfer direction. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, sizes);                     /* Write the CDB size. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, read_len);                  /* Write the buffer size. */
    outb(io_base + VBSCSI_REGISTER_COMMAND, (read_len) >> 8);
    for (i = 0; i < cmdlen; i++)                                        /* Write the CDB. */
        outb(io_base + VBSCSI_REGISTER_COMMAND, cmdbuf[i]);

    /* Now wait for the command to complete. */
    do
        status = inb(io_base + VBSCSI_REGISTER_STATUS);
    while (status & VBSCSI_BUSY);

    /* If any error occurred, inform the caller and don't bother reading the data. */
    if (status & VBSCSI_ERROR) {
        outb(io_base + VBSCSI_REGISTER_RESET, 0);

        status = inb(io_base + VBSCSI_REGISTER_DEVSTAT);
        DBG_SCSI("%s: read failed, device status %02X\n", __func__, status);
        return 3;
    }

    /* Transfer the data read from the device. */

    if (before)     /* If necessary, throw away data which needs to be skipped. */
        insb_discard(before, io_base + VBSCSI_REGISTER_DATA_IN);

    bios_dsk->drqp.trsfbytes = length;

    /* The requested length may be exactly 64K or more, which needs
     * a bit of care when we're using 16-bit 'rep ins'.
     */
    while (length > 32768) {
        DBG_SCSI("%s: reading 32K to %X:%X\n", __func__, FP_SEG(buffer), FP_OFF(buffer));
        rep_insb(buffer, 32768, io_base + VBSCSI_REGISTER_DATA_IN);
        length -= 32768;
        buffer = (FP_SEG(buffer) + (32768 >> 4)) :> FP_OFF(buffer);
    }

    DBG_SCSI("%s: reading %ld bytes to %X:%X\n", __func__, length, FP_SEG(buffer), FP_OFF(buffer));
    rep_insb(buffer, length, io_base + VBSCSI_REGISTER_DATA_IN);

    if (bios_dsk->drqp.skip_a)  /* If necessary, throw away more data. */
        insb_discard(bios_dsk->drqp.skip_a, io_base + VBSCSI_REGISTER_DATA_IN);

    return 0;
}

/**
 * Enumerate attached devices.
 *
 * @returns nothing.
 * @param   io_base    The I/O base port of the controller.
 */
void scsi_enumerate_attached_devices(uint16_t io_base)
{
    int                 i;
    uint8_t             buffer[0x0200];
    bio_dsk_t __far     *bios_dsk;

    bios_dsk = read_word(0x0040, 0x000E) :> &EbdaData->bdisk;

    /* Go through target devices. */
    for (i = 0; i < VBSCSI_MAX_DEVICES; i++)
    {
        uint8_t     rc;
        uint8_t     aCDB[16];
        uint8_t     hd_index, devcount_scsi;

        aCDB[0] = SCSI_INQUIRY;
        aCDB[1] = 0;
        aCDB[2] = 0;
        aCDB[3] = 0;
        aCDB[4] = 5; /* Allocation length. */
        aCDB[5] = 0;

        rc = scsi_cmd_data_in(io_base, i, aCDB, 6, buffer, 5);
        if (rc != 0)
            BX_PANIC("%s: SCSI_INQUIRY failed\n", __func__);

        devcount_scsi = bios_dsk->scsi_devcount;

        /* Check the attached device. */
        if (   ((buffer[0] & 0xe0) == 0)
            && ((buffer[0] & 0x1f) == 0x00))
        {
            DBG_SCSI("%s: Disk detected at %d\n", __func__, i);

            /* We add the disk only if the maximum is not reached yet. */
            if (devcount_scsi < BX_MAX_SCSI_DEVICES)
            {
                uint64_t    sectors, t;
                uint32_t    sector_size, cylinders;
                uint16_t    heads, sectors_per_track;
                uint8_t     hdcount;
                uint8_t     cmos_base;

                /* Issue a read capacity command now. */
                _fmemset(aCDB, 0, sizeof(aCDB));
                aCDB[0] = SCSI_SERVICE_ACT;
                aCDB[1] = SCSI_READ_CAP_16;
                aCDB[13] = 32; /* Allocation length. */

                rc = scsi_cmd_data_in(io_base, i, aCDB, 16, buffer, 32);
                if (rc != 0)
                    BX_PANIC("%s: SCSI_READ_CAPACITY failed\n", __func__);

                /* The value returned is the last addressable LBA, not
                 * the size, which what "+ 1" is for.
                 */
                sectors = swap_64(*(uint64_t *)buffer) + 1;

                sector_size =   ((uint32_t)buffer[8] << 24)
                              | ((uint32_t)buffer[9] << 16)
                              | ((uint32_t)buffer[10] << 8)
                              | ((uint32_t)buffer[11]);

                /* We only support the disk if sector size is 512 bytes. */
                if (sector_size != 512)
                {
                    /* Leave a log entry. */
                    BX_INFO("Disk %d has an unsupported sector size of %u\n", i, sector_size);
                    continue;
                }

                /* Get logical CHS geometry. */
                switch (devcount_scsi)
                {
                    case 0:
                        cmos_base = 0x90;
                        break;
                    case 1:
                        cmos_base = 0x98;
                        break;
                    case 2:
                        cmos_base = 0xA0;
                        break;
                    case 3:
                        cmos_base = 0xA8;
                        break;
                    default:
                        cmos_base = 0;
                }

                if (cmos_base && inb_cmos(cmos_base + 7))
                {
                    /* If provided, grab the logical geometry from CMOS. */
                    cylinders         = inb_cmos(cmos_base + 0) + (inb_cmos(cmos_base + 1) << 8);
                    heads             = inb_cmos(cmos_base + 2);
                    sectors_per_track = inb_cmos(cmos_base + 7);
                }
                else
                {
                    /* Calculate default logical geometry. NB: Very different
                     * from default ATA/SATA logical geometry!
                     */
                    if (sectors >= (uint32_t)4 * 1024 * 1024)
                    {
                        heads = 255;
                        sectors_per_track = 63;
                        /* Approximate x / (255 * 63) using shifts */
                        t = (sectors >> 6) + (sectors >> 12);
                        cylinders = (t >> 8) + (t >> 16);
                    }
                    else if (sectors >= (uint32_t)2 * 1024 * 1024)
                    {
                        heads = 128;
                        sectors_per_track = 32;
                        cylinders = sectors >> 12;
                    }
                    else
                    {
                        heads = 64;
                        sectors_per_track = 32;
                        cylinders = sectors >> 11;
                    }
                }

                /* Calculate index into the generic disk table. */
                hd_index = devcount_scsi + BX_MAX_ATA_DEVICES;

                bios_dsk->scsidev[devcount_scsi].io_base   = io_base;
                bios_dsk->scsidev[devcount_scsi].target_id = i;
                bios_dsk->devices[hd_index].type        = DSK_TYPE_SCSI;
                bios_dsk->devices[hd_index].device      = DSK_DEVICE_HD;
                bios_dsk->devices[hd_index].removable   = 0;
                bios_dsk->devices[hd_index].lock        = 0;
                bios_dsk->devices[hd_index].blksize     = sector_size;
                bios_dsk->devices[hd_index].translation = GEO_TRANSLATION_LBA;

                /* Write LCHS/PCHS values. */
                bios_dsk->devices[hd_index].lchs.heads = heads;
                bios_dsk->devices[hd_index].lchs.spt   = sectors_per_track;
                bios_dsk->devices[hd_index].pchs.heads = heads;
                bios_dsk->devices[hd_index].pchs.spt   = sectors_per_track;

                if (cylinders > 1024) {
                    bios_dsk->devices[hd_index].lchs.cylinders = 1024;
                    bios_dsk->devices[hd_index].pchs.cylinders = 1024;
                } else {
                    bios_dsk->devices[hd_index].lchs.cylinders = (uint16_t)cylinders;
                    bios_dsk->devices[hd_index].pchs.cylinders = (uint16_t)cylinders;
                }

                BX_INFO("SCSI %d-ID#%d: LCHS=%lu/%u/%u 0x%llx sectors\n", devcount_scsi,
                        i, (uint32_t)cylinders, heads, sectors_per_track, sectors);

                bios_dsk->devices[hd_index].sectors = sectors;

                /* Store the id of the disk in the ata hdidmap. */
                hdcount = bios_dsk->hdcount;
                bios_dsk->hdidmap[hdcount] = devcount_scsi + BX_MAX_ATA_DEVICES;
                hdcount++;
                bios_dsk->hdcount = hdcount;

                /* Update hdcount in the BDA. */
                hdcount = read_byte(0x40, 0x75);
                hdcount++;
                write_byte(0x40, 0x75, hdcount);

                devcount_scsi++;
            }
            else
            {
                /* We reached the maximum of SCSI disks we can boot from. We can quit detecting. */
                break;
            }
        }
        else if (   ((buffer[0] & 0xe0) == 0)
                 && ((buffer[0] & 0x1f) == 0x05))
        {
            uint8_t     cdcount;
            uint8_t     removable;

            BX_INFO("SCSI %d-ID#%d: CD/DVD-ROM\n", devcount_scsi, i);

            /* Calculate index into the generic device table. */
            hd_index = devcount_scsi + BX_MAX_ATA_DEVICES;

            removable = buffer[1] & 0x80 ? 1 : 0;

            bios_dsk->scsidev[devcount_scsi].io_base   = io_base;
            bios_dsk->scsidev[devcount_scsi].target_id = i;
            bios_dsk->devices[hd_index].type      = DSK_TYPE_SCSI;
            bios_dsk->devices[hd_index].device    = DSK_DEVICE_CDROM;
            bios_dsk->devices[hd_index].removable = removable;
            bios_dsk->devices[hd_index].blksize   = 2048;

            /* Store the ID of the device in the BIOS cdidmap. */
            cdcount = bios_dsk->cdcount;
            bios_dsk->cdidmap[cdcount] = devcount_scsi + BX_MAX_ATA_DEVICES;
            cdcount++;
            bios_dsk->cdcount = cdcount;

            devcount_scsi++;
        }
        else
            DBG_SCSI("%s: No supported device detected at %d\n", __func__, i);

        bios_dsk->scsi_devcount = devcount_scsi;
    }
}

void scsi_pci_init(uint16_t vendor_id, uint16_t device_id)
{
    uint16_t    bus_dev_fn;

    bus_dev_fn = pci_find_device(vendor_id, device_id);
    if (bus_dev_fn == -1) {
        DBG_SCSI("%s: Adapter %x:%x not found, how come?!\n", __func__, vendor_id, device_id);
        return;
    }

    DBG_SCSI("%s: Adapter %x:%x found at %x, enabling BM\n", __func__, vendor_id, device_id, bus_dev_fn);
    /* Enable PCI memory, I/O, bus mastering access in command register. */
    pci_write_config_word(bus_dev_fn >> 8, (uint8_t)bus_dev_fn, 4, 0x7);
}

/**
 * Init the SCSI driver and detect attached disks.
 */
void BIOSCALL scsi_init(void)
{
    uint8_t             identifier;
    bio_dsk_t __far     *bios_dsk;

    bios_dsk = read_word(0x0040, 0x000E) :> &EbdaData->bdisk;

    bios_dsk->scsi_devcount = 0;

    identifier = 0;

    /* Detect the BusLogic adapter. */
    outb(BUSLOGIC_BIOS_IO_PORT+VBSCSI_REGISTER_IDENTIFY, 0x55);
    identifier = inb(BUSLOGIC_BIOS_IO_PORT+VBSCSI_REGISTER_IDENTIFY);

    if (identifier == 0x55)
    {
        /* Detected - Enumerate attached devices. */
        DBG_SCSI("%s: BusLogic SCSI adapter detected\n", __func__);
        outb(BUSLOGIC_BIOS_IO_PORT+VBSCSI_REGISTER_RESET, 0);
        scsi_enumerate_attached_devices(BUSLOGIC_BIOS_IO_PORT);
        scsi_pci_init(0x104B, 0x1040);
    }
    else
    {
        DBG_SCSI("%s: BusLogic SCSI adapter not detected\n", __func__);
    }

    /* Detect the LSI Logic parallel SCSI adapter. */
    outb(LSILOGIC_BIOS_IO_PORT+VBSCSI_REGISTER_IDENTIFY, 0x55);
    identifier = inb(LSILOGIC_BIOS_IO_PORT+VBSCSI_REGISTER_IDENTIFY);

    if (identifier == 0x55)
    {
        /* Detected - Enumerate attached devices. */
        DBG_SCSI("%s: LSI Logic SCSI adapter detected\n", __func__);
        outb(LSILOGIC_BIOS_IO_PORT+VBSCSI_REGISTER_RESET, 0);
        scsi_enumerate_attached_devices(LSILOGIC_BIOS_IO_PORT);
        scsi_pci_init(0x1000, 0x0030);
    }
    else
    {
        DBG_SCSI("%s: LSI Logic SCSI adapter not detected\n", __func__);
    }

    /* Detect the LSI Logic SAS adapter. */
    outb(LSILOGIC_SAS_BIOS_IO_PORT+VBSCSI_REGISTER_IDENTIFY, 0x55);
    identifier = inb(LSILOGIC_SAS_BIOS_IO_PORT+VBSCSI_REGISTER_IDENTIFY);

    if (identifier == 0x55)
    {
        /* Detected - Enumerate attached devices. */
        DBG_SCSI("%s: LSI Logic SAS adapter detected\n", __func__);
        outb(LSILOGIC_SAS_BIOS_IO_PORT+VBSCSI_REGISTER_RESET, 0);
        scsi_enumerate_attached_devices(LSILOGIC_SAS_BIOS_IO_PORT);
        scsi_pci_init(0x1000, 0x0054);
    }
    else
    {
        DBG_SCSI("%s: LSI Logic SAS adapter not detected\n", __func__);
    }
}
