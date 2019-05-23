/* $Id: DevFdc.cpp $ */
/** @file
 * VBox storage devices - Floppy disk controller
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 * --------------------------------------------------------------------
 *
 * This code is based on:
 *
 * QEMU Floppy disk emulator (Intel 82078)
 *
 * Copyright (c) 2003 Jocelyn Mayer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_FDC
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmstorageifs.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/uuid.h>

#include "VBoxDD.h"

#define FDC_SAVESTATE_CURRENT   2       /* The new and improved saved state. */
#define FDC_SAVESTATE_OLD       1       /* The original saved state. */

#define MAX_FD 2


/********************************************************/
/* debug Floppy devices */
/* #define DEBUG_FLOPPY */

#ifndef VBOX
    #ifdef DEBUG_FLOPPY
    #define FLOPPY_DPRINTF(fmt, args...) \
        do { printf("FLOPPY: " fmt , ##args); } while (0)
    #endif
#else /* !VBOX */
# ifdef LOG_ENABLED
#  define FLOPPY_DPRINTF(...) Log(("floppy: " __VA_ARGS__))
# else
#  define FLOPPY_DPRINTF(...) do { } while (0)
# endif
#endif /* !VBOX */

#ifndef VBOX
#define FLOPPY_ERROR(fmt, args...) \
    do { printf("FLOPPY ERROR: %s: " fmt, __func__ , ##args); } while (0)
#else /* VBOX */
#   define FLOPPY_ERROR RTLogPrintf
#endif /* VBOX */

#ifdef VBOX
typedef struct fdctrl_t fdctrl_t;
#endif /* VBOX */

/********************************************************/
/* Floppy drive emulation                               */

#define GET_CUR_DRV(fdctrl) ((fdctrl)->cur_drv)
#define SET_CUR_DRV(fdctrl, drive) ((fdctrl)->cur_drv = (drive))

/* Will always be a fixed parameter for us */
#define FD_SECTOR_LEN          512
#define FD_SECTOR_SC           2   /* Sector size code */
#define FD_RESET_SENSEI_COUNT  4   /* Number of sense interrupts on RESET */

/* Floppy disk drive emulation */
typedef enum fdisk_type_t {
    FDRIVE_DISK_288   = 0x01, /* 2.88 MB disk           */
    FDRIVE_DISK_144   = 0x02, /* 1.44 MB disk           */
    FDRIVE_DISK_720   = 0x03, /* 720 kB disk            */
    FDRIVE_DISK_USER  = 0x04, /* User defined geometry  */
    FDRIVE_DISK_NONE  = 0x05  /* No disk                */
} fdisk_type_t;

typedef enum fdrive_type_t {
    FDRIVE_DRV_144  = 0x00,   /* 1.44 MB 3"5 drive      */
    FDRIVE_DRV_288  = 0x01,   /* 2.88 MB 3"5 drive      */
    FDRIVE_DRV_120  = 0x02,   /* 1.2  MB 5"25 drive     */
    FDRIVE_DRV_NONE = 0x03    /* No drive connected     */
#ifdef VBOX
    , FDRIVE_DRV_FAKE_15_6 = 0x0e /* Fake 15.6 MB drive. */
    , FDRIVE_DRV_FAKE_63_5 = 0x0f /* Fake 63.5 MB drive. */
#endif
} fdrive_type_t;

typedef uint8_t fdrive_flags_t;
#define FDISK_DBL_SIDES     UINT8_C(0x01)

typedef enum fdrive_rate_t {
    FDRIVE_RATE_500K = 0x00,  /* 500 Kbps               */
    FDRIVE_RATE_300K = 0x01,  /* 300 Kbps               */
    FDRIVE_RATE_250K = 0x02,  /* 250 Kbps               */
    FDRIVE_RATE_1M   = 0x03   /* 1 Mbps                 */
} fdrive_rate_t;

/**
 * The status for one drive.
 *
 * @implements  PDMIBASE
 * @implements  PDMIMEDIAPORT
 * @implements  PDMIMOUNTNOTIFY
 */
typedef struct fdrive_t {
#ifndef VBOX
    BlockDriverState *bs;
#else /* VBOX */
    /** Pointer to the owning device instance. */
    R3PTRTYPE(PPDMDEVINS)           pDevIns;
    /** Pointer to the attached driver's base interface. */
    R3PTRTYPE(PPDMIBASE)            pDrvBase;
    /** Pointer to the attached driver's block interface. */
    R3PTRTYPE(PPDMIMEDIA)           pDrvMedia;
    /** Pointer to the attached driver's mount interface.
     * This is NULL if the driver isn't a removable unit. */
    R3PTRTYPE(PPDMIMOUNT)           pDrvMount;
    /** The base interface. */
    PDMIBASE                        IBase;
    /** The block port interface. */
    PDMIMEDIAPORT                   IPort;
    /** The mount notify interface. */
    PDMIMOUNTNOTIFY                 IMountNotify;
    /** The LUN #. */
    RTUINT                          iLUN;
    /** The LED for this LUN. */
    PDMLED                          Led;
#endif
    /* Drive status */
    fdrive_type_t drive;
    uint8_t perpendicular;    /* 2.88 MB access mode    */
    uint8_t dsk_chg;          /* Disk change line       */
    /* Position */
    uint8_t head;
    uint8_t track;
    uint8_t sect;
    uint8_t ltrk;             /* Logical track */
    /* Media */
    fdrive_flags_t flags;
    uint8_t last_sect;        /* Nb sector per track    */
    uint8_t max_track;        /* Nb of tracks           */
    uint16_t bps;             /* Bytes per sector       */
    uint8_t ro;               /* Is read-only           */
    uint8_t media_rate;       /* Data rate of medium    */
} fdrive_t;

#define NUM_SIDES(drv)      (drv->flags & FDISK_DBL_SIDES ? 2 : 1)

static void fd_init(fdrive_t *drv, bool fInit)
{
    /* Drive */
#ifndef VBOX
    drv->drive = FDRIVE_DRV_NONE;
#else  /* VBOX */
    if (fInit) {
        /* Fixate the drive type at init time if possible. */
        if (drv->pDrvMedia) {
            PDMMEDIATYPE enmType = drv->pDrvMedia->pfnGetType(drv->pDrvMedia);
            switch (enmType) {
                case PDMMEDIATYPE_FLOPPY_360:
                case PDMMEDIATYPE_FLOPPY_1_20:
                    drv->drive = FDRIVE_DRV_120;
                    break;
                case PDMMEDIATYPE_FLOPPY_720:
                case PDMMEDIATYPE_FLOPPY_1_44:
                    drv->drive = FDRIVE_DRV_144;
                    break;
                default:
                    AssertFailed();
                    RT_FALL_THRU();
                case PDMMEDIATYPE_FLOPPY_2_88:
                    drv->drive = FDRIVE_DRV_288;
                    break;
                case PDMMEDIATYPE_FLOPPY_FAKE_15_6:
                    drv->drive = FDRIVE_DRV_FAKE_15_6;
                    break;
                case PDMMEDIATYPE_FLOPPY_FAKE_63_5:
                    drv->drive = FDRIVE_DRV_FAKE_63_5;
                    break;
            }
        } else {
            drv->drive = FDRIVE_DRV_NONE;
        }
    } /* else: The BIOS (and others) get the drive type via the CMOS, so
               don't change it after the VM has been constructed. */
#endif /* VBOX */
    drv->perpendicular = 0;
    /* Disk */
    drv->last_sect = 0;
    drv->max_track = 0;
}

static int fd_sector_calc(uint8_t head, uint8_t track, uint8_t sect,
                          uint8_t last_sect, uint8_t num_sides)
{
    return (((track * num_sides) + head) * last_sect) + sect - 1; /* sect >= 1 */
}

/* Returns current position, in sectors, for given drive */
static int fd_sector(fdrive_t *drv)
{
    return fd_sector_calc(drv->head, drv->track, drv->sect, drv->last_sect, NUM_SIDES(drv));
}

/* Seek to a new position:
 * returns 0 if already on right track
 * returns 1 if track changed
 * returns 2 if track is invalid
 * returns 3 if sector is invalid
 * returns 4 if seek is disabled
 * returns 5 if no media in drive
 */
static int fd_seek(fdrive_t *drv, uint8_t head, uint8_t track, uint8_t sect,
                   int enable_seek)
{
    int sector;
    int ret;

    if (!drv->last_sect) {
        FLOPPY_DPRINTF("no disk in drive (max=%d %d %02x %02x)\n",
                       1, (drv->flags & FDISK_DBL_SIDES) == 0 ? 0 : 1,
                       drv->max_track, drv->last_sect);
        return 5;
    }
    if (track > drv->max_track ||
        (head != 0 && (drv->flags & FDISK_DBL_SIDES) == 0)) {
        FLOPPY_DPRINTF("try to read %d %02x %02x (max=%d %d %02x %02x)\n",
                       head, track, sect, 1,
                       (drv->flags & FDISK_DBL_SIDES) == 0 ? 0 : 1,
                       drv->max_track, drv->last_sect);
        return 2;
    }
    if (sect > drv->last_sect || sect < 1) {
        FLOPPY_DPRINTF("try to read %d %02x %02x (max=%d %d %02x %02x)\n",
                       head, track, sect, 1,
                       (drv->flags & FDISK_DBL_SIDES) == 0 ? 0 : 1,
                       drv->max_track, drv->last_sect);
        return 3;
    }
    sector = fd_sector_calc(head, track, sect, drv->last_sect, NUM_SIDES(drv));
    ret = 0;
    if (sector != fd_sector(drv)) {
#if 0
        if (!enable_seek) {
            FLOPPY_ERROR("no implicit seek %d %02x %02x (max=%d %02x %02x)\n",
                         head, track, sect, 1, drv->max_track, drv->last_sect);
            return 4;
        }
#else
        RT_NOREF(enable_seek);
#endif
        drv->head = head;
        if (drv->track != track)
            ret = 1;
        drv->track = track;
        drv->sect = sect;
    }
    drv->ltrk = drv->track;

    return ret;
}

/* Set drive back to track 0 */
static void fd_recalibrate(fdrive_t *drv)
{
    FLOPPY_DPRINTF("recalibrate\n");
    drv->head = 0;
    drv->track = 0;
    drv->ltrk = 0;
    drv->sect = 1;
}

/* Recognize floppy formats */
typedef struct fd_format_t {
    fdrive_type_t drive;
    fdisk_type_t  disk;
    uint8_t last_sect;      /**< Number of sectors. */
    uint8_t max_track;      /**< Number of tracks. */
    uint8_t max_head;       /**< Max head number. */
    fdrive_rate_t rate;
    const char *str;
} fd_format_t;

/* Note: Low-density disks (160K/180K/320K/360K) use 250 Kbps data rate
 * in 40-track drives, but 300 Kbps in high-capacity 80-track drives.
 */
static fd_format_t fd_formats[] = {
    /* First entry is default format */
    /* 1.44 MB 3"1/2 floppy disks */
    { FDRIVE_DRV_144, FDRIVE_DISK_144, 18, 80, 1, FDRIVE_RATE_500K, "1.44 MB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_144, 20, 80, 1, FDRIVE_RATE_500K,  "1.6 MB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_144, 21, 80, 1, FDRIVE_RATE_500K, "1.68 MB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_144, 21, 82, 1, FDRIVE_RATE_500K, "1.72 MB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_144, 21, 83, 1, FDRIVE_RATE_500K, "1.74 MB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_144, 22, 80, 1, FDRIVE_RATE_500K, "1.76 MB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_144, 23, 80, 1, FDRIVE_RATE_500K, "1.84 MB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_144, 24, 80, 1, FDRIVE_RATE_500K, "1.92 MB 3\"1/2", },
    /* 2.88 MB 3"1/2 floppy disks */
    { FDRIVE_DRV_288, FDRIVE_DISK_288, 36, 80, 1, FDRIVE_RATE_1M,   "2.88 MB 3\"1/2", },
    { FDRIVE_DRV_288, FDRIVE_DISK_288, 39, 80, 1, FDRIVE_RATE_1M,   "3.12 MB 3\"1/2", },
    { FDRIVE_DRV_288, FDRIVE_DISK_288, 40, 80, 1, FDRIVE_RATE_1M,    "3.2 MB 3\"1/2", },
    { FDRIVE_DRV_288, FDRIVE_DISK_288, 44, 80, 1, FDRIVE_RATE_1M,   "3.52 MB 3\"1/2", },
    { FDRIVE_DRV_288, FDRIVE_DISK_288, 48, 80, 1, FDRIVE_RATE_1M,   "3.84 MB 3\"1/2", },
    /* 720 kB 3"1/2 floppy disks */
    { FDRIVE_DRV_144, FDRIVE_DISK_720,  9, 80, 1, FDRIVE_RATE_250K,  "720 kB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_720, 10, 80, 1, FDRIVE_RATE_250K,  "800 kB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_720, 10, 82, 1, FDRIVE_RATE_250K,  "820 kB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_720, 10, 83, 1, FDRIVE_RATE_250K,  "830 kB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_720, 13, 80, 1, FDRIVE_RATE_250K, "1.04 MB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_720, 14, 80, 1, FDRIVE_RATE_250K, "1.12 MB 3\"1/2", },
    /* 1.2 MB 5"1/4 floppy disks */
    { FDRIVE_DRV_120, FDRIVE_DISK_288, 15, 80, 1, FDRIVE_RATE_500K,  "1.2 MB 5\"1/4", },
    { FDRIVE_DRV_120, FDRIVE_DISK_288, 16, 80, 1, FDRIVE_RATE_500K, "1.28 MB 5\"1/4", },    /* CP Backup 5.25" HD */
    { FDRIVE_DRV_120, FDRIVE_DISK_288, 18, 80, 1, FDRIVE_RATE_500K, "1.44 MB 5\"1/4", },
    { FDRIVE_DRV_120, FDRIVE_DISK_288, 18, 82, 1, FDRIVE_RATE_500K, "1.48 MB 5\"1/4", },
    { FDRIVE_DRV_120, FDRIVE_DISK_288, 18, 83, 1, FDRIVE_RATE_500K, "1.49 MB 5\"1/4", },
    { FDRIVE_DRV_120, FDRIVE_DISK_288, 20, 80, 1, FDRIVE_RATE_500K,  "1.6 MB 5\"1/4", },
    /* 720 kB 5"1/4 floppy disks */
    { FDRIVE_DRV_120, FDRIVE_DISK_288,  9, 80, 1, FDRIVE_RATE_250K,  "720 kB 5\"1/4", },
    { FDRIVE_DRV_120, FDRIVE_DISK_288, 11, 80, 1, FDRIVE_RATE_250K,  "880 kB 5\"1/4", },
    /* 360 kB 5"1/4 floppy disks (newer 9-sector formats) */
    { FDRIVE_DRV_120, FDRIVE_DISK_288,  9, 40, 1, FDRIVE_RATE_300K,  "360 kB 5\"1/4", },
    { FDRIVE_DRV_120, FDRIVE_DISK_288,  9, 40, 0, FDRIVE_RATE_300K,  "180 kB 5\"1/4", },
    { FDRIVE_DRV_120, FDRIVE_DISK_288, 10, 40, 1, FDRIVE_RATE_300K,  "400 kB 5\"1/4", },    /* CP Backup 5.25" DD */
    { FDRIVE_DRV_120, FDRIVE_DISK_288, 10, 41, 1, FDRIVE_RATE_300K,  "410 kB 5\"1/4", },
    { FDRIVE_DRV_120, FDRIVE_DISK_288, 10, 42, 1, FDRIVE_RATE_300K,  "420 kB 5\"1/4", },
    /* 320 kB 5"1/4 floppy disks (old 8-sector formats) */
    { FDRIVE_DRV_120, FDRIVE_DISK_288,  8, 40, 1, FDRIVE_RATE_300K,  "320 kB 5\"1/4", },
    { FDRIVE_DRV_120, FDRIVE_DISK_288,  8, 40, 0, FDRIVE_RATE_300K,  "160 kB 5\"1/4", },
    /* 1.2 MB and low density 3"1/2 floppy 'aliases' */
    { FDRIVE_DRV_144, FDRIVE_DISK_144, 15, 80, 1, FDRIVE_RATE_500K,  "1.2 MB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_144, 16, 80, 1, FDRIVE_RATE_500K, "1.28 MB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_720, 10, 40, 1, FDRIVE_RATE_300K,  "400 kB 3\"1/2", },    /* CP Backup 5.25" DD */
    { FDRIVE_DRV_144, FDRIVE_DISK_720,  9, 40, 1, FDRIVE_RATE_300K,  "360 kB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_720,  9, 40, 0, FDRIVE_RATE_300K,  "180 kB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_720,  8, 40, 1, FDRIVE_RATE_300K,  "320 kB 3\"1/2", },
    { FDRIVE_DRV_144, FDRIVE_DISK_720,  8, 40, 0, FDRIVE_RATE_300K,  "160 kB 3\"1/2", },
#ifdef VBOX /* For larger than real life floppy images (see DrvBlock.cpp). */
    /* 15.6 MB fake floppy disk (just need something big). */
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_USER,  63, 255, 1, FDRIVE_RATE_1M,   "15.6 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_288,   36,  80, 1, FDRIVE_RATE_1M,   "2.88 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_288,   39,  80, 1, FDRIVE_RATE_1M,   "3.12 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_288,   40,  80, 1, FDRIVE_RATE_1M,    "3.2 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_288,   44,  80, 1, FDRIVE_RATE_1M,   "3.52 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_288,   48,  80, 1, FDRIVE_RATE_1M,   "3.84 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_144,   18,  80, 1, FDRIVE_RATE_500K, "1.44 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_144,   20,  80, 1, FDRIVE_RATE_500K,  "1.6 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_144,   21,  80, 1, FDRIVE_RATE_500K, "1.68 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_144,   21,  82, 1, FDRIVE_RATE_500K, "1.72 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_144,   21,  83, 1, FDRIVE_RATE_500K, "1.74 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_144,   22,  80, 1, FDRIVE_RATE_500K, "1.76 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_144,   23,  80, 1, FDRIVE_RATE_500K, "1.84 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_144,   24,  80, 1, FDRIVE_RATE_500K, "1.92 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_720,    9,  80, 1, FDRIVE_RATE_250K,  "720 kB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_720,   10,  80, 1, FDRIVE_RATE_250K,  "800 kB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_720,   10,  82, 1, FDRIVE_RATE_250K,  "820 kB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_720,   10,  83, 1, FDRIVE_RATE_250K,  "830 kB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_720,   13,  80, 1, FDRIVE_RATE_250K, "1.04 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_720,   14,  80, 1, FDRIVE_RATE_250K, "1.12 MB fake 15.6", },
    { FDRIVE_DRV_FAKE_15_6, FDRIVE_DISK_720,    9,  80, 0, FDRIVE_RATE_250K,  "360 kB fake 15.6", },
    /* 63.5 MB fake floppy disk (just need something big). */
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_USER, 255, 255, 1, FDRIVE_RATE_1M,   "63.5 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_USER,  63, 255, 1, FDRIVE_RATE_1M,   "15.6 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_288,   36,  80, 1, FDRIVE_RATE_1M,   "2.88 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_288,   39,  80, 1, FDRIVE_RATE_1M,   "3.12 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_288,   40,  80, 1, FDRIVE_RATE_1M,    "3.2 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_288,   44,  80, 1, FDRIVE_RATE_1M,   "3.52 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_288,   48,  80, 1, FDRIVE_RATE_1M,   "3.84 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_144,   18,  80, 1, FDRIVE_RATE_500K, "1.44 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_144,   20,  80, 1, FDRIVE_RATE_500K,  "1.6 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_144,   21,  80, 1, FDRIVE_RATE_500K, "1.68 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_144,   21,  82, 1, FDRIVE_RATE_500K, "1.72 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_144,   21,  83, 1, FDRIVE_RATE_500K, "1.74 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_144,   22,  80, 1, FDRIVE_RATE_500K, "1.76 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_144,   23,  80, 1, FDRIVE_RATE_500K, "1.84 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_144,   24,  80, 1, FDRIVE_RATE_500K, "1.92 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_720,    9,  80, 1, FDRIVE_RATE_250K,  "720 kB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_720,   10,  80, 1, FDRIVE_RATE_250K,  "800 kB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_720,   10,  82, 1, FDRIVE_RATE_250K,  "820 kB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_720,   10,  83, 1, FDRIVE_RATE_250K,  "830 kB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_720,   13,  80, 1, FDRIVE_RATE_250K, "1.04 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_720,   14,  80, 1, FDRIVE_RATE_250K, "1.12 MB fake 63.5", },
    { FDRIVE_DRV_FAKE_63_5, FDRIVE_DISK_720,    9,  80, 0, FDRIVE_RATE_250K,  "360 kB fake 63.5", },
#endif
    /* end */
    { FDRIVE_DRV_NONE, FDRIVE_DISK_NONE, (uint8_t)-1, (uint8_t)-1, 0, (fdrive_rate_t)0, NULL, },
};

/* Revalidate a disk drive after a disk change */
static void fd_revalidate(fdrive_t *drv)
{
    const fd_format_t *parse;
    uint64_t nb_sectors, size;
    int i, first_match, match;
    int nb_heads, max_track, last_sect, ro;

    FLOPPY_DPRINTF("revalidate\n");
#ifndef VBOX
    if (drv->bs != NULL && bdrv_is_inserted(drv->bs)) {
        ro = bdrv_is_read_only(drv->bs);
        bdrv_get_geometry_hint(drv->bs, &nb_heads, &max_track, &last_sect);
#else /* VBOX */
    if (   drv->pDrvMedia
        && drv->pDrvMount
        && drv->pDrvMount->pfnIsMounted (drv->pDrvMount)) {
        ro = drv->pDrvMedia->pfnIsReadOnly (drv->pDrvMedia);
        nb_heads = max_track = last_sect = 0;
#endif /* VBOX */
        if (nb_heads != 0 && max_track != 0 && last_sect != 0) {
            FLOPPY_DPRINTF("User defined disk (%d %d %d)",
                           nb_heads - 1, max_track, last_sect);
        } else {
#ifndef VBOX
            bdrv_get_geometry(drv->bs, &nb_sectors);
#else /* VBOX */
            {
                uint64_t size2 = drv->pDrvMedia->pfnGetSize (drv->pDrvMedia);
                nb_sectors = size2 / FD_SECTOR_LEN;
            }
#endif /* VBOX */
            match = -1;
            first_match = -1;
            for (i = 0;; i++) {
                parse = &fd_formats[i];
                if (parse->drive == FDRIVE_DRV_NONE)
                    break;
                if (drv->drive == parse->drive ||
                    drv->drive == FDRIVE_DRV_NONE) {
                    size = (parse->max_head + 1) * parse->max_track *
                        parse->last_sect;
                    if (nb_sectors == size) {
                        match = i;
                        break;
                    }
                    if (first_match == -1)
                        first_match = i;
                }
            }
            if (match == -1) {
                if (first_match == -1)
                    match = 1;
                else
                    match = first_match;
                parse = &fd_formats[match];
            }
            nb_heads = parse->max_head + 1;
            max_track = parse->max_track;
            last_sect = parse->last_sect;
            drv->drive = parse->drive;
#ifdef VBOX
            drv->media_rate = parse->rate;
#endif
            FLOPPY_DPRINTF("%s floppy disk (%d h %d t %d s) %s\n", parse->str,
                           nb_heads, max_track, last_sect, ro ? "ro" : "rw");
            LogRel(("FDC: %s floppy disk (%d h %d t %d s) %s\n", parse->str,
                    nb_heads, max_track, last_sect, ro ? "ro" : "rw"));
        }
        if (nb_heads == 1) {
            drv->flags &= ~FDISK_DBL_SIDES;
        } else {
            drv->flags |= FDISK_DBL_SIDES;
        }
        drv->max_track = max_track;
        drv->last_sect = last_sect;
        drv->ro = ro;
    } else {
        FLOPPY_DPRINTF("No disk in drive\n");
        drv->last_sect = 0;
        drv->max_track = 0;
        drv->flags &= ~FDISK_DBL_SIDES;
        drv->dsk_chg = true;    /* Disk change line active. */
    }
}

/********************************************************/
/* Intel 82078 floppy disk controller emulation          */

static void fdctrl_reset(fdctrl_t *fdctrl, int do_irq);
static void fdctrl_reset_fifo(fdctrl_t *fdctrl);
#ifndef VBOX
static int fdctrl_transfer_handler (void *opaque, int nchan,
                                    int dma_pos, int dma_len);
#else /* VBOX: */
static DECLCALLBACK(uint32_t) fdctrl_transfer_handler (PPDMDEVINS pDevIns,
                                                       void *opaque,
                                                       unsigned nchan,
                                                       uint32_t dma_pos,
                                                       uint32_t dma_len);
#endif /* VBOX */
static void fdctrl_raise_irq(fdctrl_t *fdctrl, uint8_t status0);
static fdrive_t *get_cur_drv(fdctrl_t *fdctrl);

static void fdctrl_result_timer(void *opaque);
static uint32_t fdctrl_read_statusA(fdctrl_t *fdctrl);
static uint32_t fdctrl_read_statusB(fdctrl_t *fdctrl);
static uint32_t fdctrl_read_dor(fdctrl_t *fdctrl);
static void fdctrl_write_dor(fdctrl_t *fdctrl, uint32_t value);
static uint32_t fdctrl_read_tape(fdctrl_t *fdctrl);
static void fdctrl_write_tape(fdctrl_t *fdctrl, uint32_t value);
static uint32_t fdctrl_read_main_status(fdctrl_t *fdctrl);
static void fdctrl_write_rate(fdctrl_t *fdctrl, uint32_t value);
static uint32_t fdctrl_read_data(fdctrl_t *fdctrl);
static void fdctrl_write_data(fdctrl_t *fdctrl, uint32_t value);
static uint32_t fdctrl_read_dir(fdctrl_t *fdctrl);
static void fdctrl_write_ccr(fdctrl_t *fdctrl, uint32_t value);

enum {
    FD_DIR_WRITE   = 0,
    FD_DIR_READ    = 1,
    FD_DIR_SCANE   = 2,
    FD_DIR_SCANL   = 3,
    FD_DIR_SCANH   = 4,
    FD_DIR_FORMAT  = 5
};

enum {
    FD_STATE_MULTI  = 0x01,     /* multi track flag */
    FD_STATE_FORMAT = 0x02,     /* format flag */
    FD_STATE_SEEK   = 0x04      /* seek flag */
};

enum {
    FD_REG_SRA = 0x00,
    FD_REG_SRB = 0x01,
    FD_REG_DOR = 0x02,
    FD_REG_TDR = 0x03,
    FD_REG_MSR = 0x04,
    FD_REG_DSR = 0x04,
    FD_REG_FIFO = 0x05,
    FD_REG_DIR = 0x07,
    FD_REG_CCR = 0x07
};

enum {
    FD_CMD_READ_TRACK = 0x02,
    FD_CMD_SPECIFY = 0x03,
    FD_CMD_SENSE_DRIVE_STATUS = 0x04,
    FD_CMD_WRITE = 0x05,
    FD_CMD_READ = 0x06,
    FD_CMD_RECALIBRATE = 0x07,
    FD_CMD_SENSE_INTERRUPT_STATUS = 0x08,
    FD_CMD_WRITE_DELETED = 0x09,
    FD_CMD_READ_ID = 0x0a,
    FD_CMD_READ_DELETED = 0x0c,
    FD_CMD_FORMAT_TRACK = 0x0d,
    FD_CMD_DUMPREG = 0x0e,
    FD_CMD_SEEK = 0x0f,
    FD_CMD_VERSION = 0x10,
    FD_CMD_SCAN_EQUAL = 0x11,
    FD_CMD_PERPENDICULAR_MODE = 0x12,
    FD_CMD_CONFIGURE = 0x13,
    FD_CMD_LOCK = 0x14,
    FD_CMD_VERIFY = 0x16,
    FD_CMD_POWERDOWN_MODE = 0x17,
    FD_CMD_PART_ID = 0x18,
    FD_CMD_SCAN_LOW_OR_EQUAL = 0x19,
    FD_CMD_SCAN_HIGH_OR_EQUAL = 0x1d,
    FD_CMD_SAVE = 0x2e,
    FD_CMD_OPTION = 0x33,
    FD_CMD_RESTORE = 0x4e,
    FD_CMD_DRIVE_SPECIFICATION_COMMAND = 0x8e,
    FD_CMD_RELATIVE_SEEK_OUT = 0x8f,
    FD_CMD_FORMAT_AND_WRITE = 0xcd,
    FD_CMD_RELATIVE_SEEK_IN = 0xcf
};

enum {
    FD_CONFIG_PRETRK = 0xff, /* Pre-compensation set to track 0 */
    FD_CONFIG_FIFOTHR = 0x0f, /* FIFO threshold set to 1 byte */
    FD_CONFIG_POLL  = 0x10, /* Poll enabled */
    FD_CONFIG_EFIFO = 0x20, /* FIFO disabled */
    FD_CONFIG_EIS   = 0x40  /* No implied seeks */
};

enum {
    FD_SR0_EQPMT    = 0x10,
    FD_SR0_SEEK     = 0x20,
    FD_SR0_ABNTERM  = 0x40,
    FD_SR0_INVCMD   = 0x80,
    FD_SR0_RDYCHG   = 0xc0
};

enum {
    FD_SR1_MA       = 0x01, /* Missing address mark */
    FD_SR1_NW       = 0x02, /* Not writable */
    FD_SR1_ND       = 0x04, /* No data */
    FD_SR1_EC       = 0x80  /* End of cylinder */
};

enum {
    FD_SR2_MD       = 0x01, /* Missing data address mark */
    FD_SR2_SNS      = 0x04, /* Scan not satisfied */
    FD_SR2_SEH      = 0x08  /* Scan equal hit */
};

enum {
    FD_SRA_DIR      = 0x01,
    FD_SRA_nWP      = 0x02,
    FD_SRA_nINDX    = 0x04,
    FD_SRA_HDSEL    = 0x08,
    FD_SRA_nTRK0    = 0x10,
    FD_SRA_STEP     = 0x20,
    FD_SRA_nDRV2    = 0x40,
    FD_SRA_INTPEND  = 0x80
};

enum {
    FD_SRB_MTR0     = 0x01,
    FD_SRB_MTR1     = 0x02,
    FD_SRB_WGATE    = 0x04,
    FD_SRB_RDATA    = 0x08,
    FD_SRB_WDATA    = 0x10,
    FD_SRB_DR0      = 0x20
};

enum {
#if MAX_FD == 4
    FD_DOR_SELMASK  = 0x03,
#else
    FD_DOR_SELMASK  = 0x01,
#endif
    FD_DOR_nRESET   = 0x04,
    FD_DOR_DMAEN    = 0x08,
    FD_DOR_MOTEN0   = 0x10,
    FD_DOR_MOTEN1   = 0x20,
    FD_DOR_MOTEN2   = 0x40,
    FD_DOR_MOTEN3   = 0x80
};

enum {
#if MAX_FD == 4
    FD_TDR_BOOTSEL  = 0x0c
#else
    FD_TDR_BOOTSEL  = 0x04
#endif
};

enum {
    FD_DSR_DRATEMASK= 0x03,
    FD_DSR_PWRDOWN  = 0x40,
    FD_DSR_SWRESET  = 0x80
};

enum {
    FD_MSR_DRV0BUSY = 0x01,
    FD_MSR_DRV1BUSY = 0x02,
    FD_MSR_DRV2BUSY = 0x04,
    FD_MSR_DRV3BUSY = 0x08,
    FD_MSR_CMDBUSY  = 0x10,
    FD_MSR_NONDMA   = 0x20,
    FD_MSR_DIO      = 0x40,
    FD_MSR_RQM      = 0x80
};

enum {
    FD_DIR_DSKCHG   = 0x80
};

#define FD_MULTI_TRACK(state) ((state) & FD_STATE_MULTI)
#define FD_DID_SEEK(state) ((state) & FD_STATE_SEEK)
#define FD_FORMAT_CMD(state) ((state) & FD_STATE_FORMAT)

#ifdef VBOX
/**
 * Floppy controller state.
 *
 * @implements  PDMILEDPORTS
 */
#endif
struct fdctrl_t {
#ifndef VBOX
    fdctrl_t *fdctrl;
#endif
    /* Controller's identification */
    uint8_t version;
    /* HW */
#ifndef VBOX
    int irq;
    int dma_chann;
#else
    uint8_t irq_lvl;
    uint8_t dma_chann;
#endif
    uint32_t io_base;
    /* Controller state */
#ifndef VBOX
    QEMUTimer *result_timer;
#else
    struct TMTIMER *result_timer;
#endif
    uint8_t sra;
    uint8_t srb;
    uint8_t dor;
    uint8_t tdr;
    uint8_t dsr;
    uint8_t msr;
    uint8_t cur_drv;
    uint8_t status0;
    uint8_t status1;
    uint8_t status2;
    /* Command FIFO */
    uint8_t fifo[FD_SECTOR_LEN];
    uint32_t data_pos;
    uint32_t data_len;
    uint8_t data_state;
    uint8_t data_dir;
    uint8_t eot; /* last wanted sector */
    /* States kept only to be returned back */
    /* Timers state */
    uint8_t timer0;
    uint8_t timer1;
    /* precompensation */
    uint8_t precomp_trk;
    uint8_t config;
    uint8_t lock;
    /* Power down config (also with status regB access mode */
    uint8_t pwrd;
    /* Floppy drives */
    uint8_t num_floppies;
    fdrive_t drives[MAX_FD];
    uint8_t reset_sensei;
#ifdef VBOX
    /** Pointer to device instance. */
    PPDMDEVINS pDevIns;

    /** Status LUN: The base interface. */
    PDMIBASE IBaseStatus;
    /** Status LUN: The Leds interface. */
    PDMILEDPORTS ILeds;
    /** Status LUN: The Partner of ILeds. */
    PPDMILEDCONNECTORS pLedsConnector;
#endif
};

static uint32_t fdctrl_read (void *opaque, uint32_t reg)
{
    fdctrl_t *fdctrl = (fdctrl_t *)opaque;
    uint32_t retval;

    switch (reg) {
    case FD_REG_SRA:
        retval = fdctrl_read_statusA(fdctrl);
        break;
    case FD_REG_SRB:
        retval = fdctrl_read_statusB(fdctrl);
        break;
    case FD_REG_DOR:
        retval = fdctrl_read_dor(fdctrl);
        break;
    case FD_REG_TDR:
        retval = fdctrl_read_tape(fdctrl);
        break;
    case FD_REG_MSR:
        retval = fdctrl_read_main_status(fdctrl);
        break;
    case FD_REG_FIFO:
        retval = fdctrl_read_data(fdctrl);
        break;
    case FD_REG_DIR:
        retval = fdctrl_read_dir(fdctrl);
        break;
    default:
        retval = (uint32_t)(-1);
        break;
    }
    FLOPPY_DPRINTF("read reg%d: 0x%02x\n", reg & 7, retval);

    return retval;
}

static void fdctrl_write (void *opaque, uint32_t reg, uint32_t value)
{
    fdctrl_t *fdctrl = (fdctrl_t *)opaque;

    FLOPPY_DPRINTF("write reg%d: 0x%02x\n", reg & 7, value);

    switch (reg) {
    case FD_REG_DOR:
        fdctrl_write_dor(fdctrl, value);
        break;
    case FD_REG_TDR:
        fdctrl_write_tape(fdctrl, value);
        break;
    case FD_REG_DSR:
        fdctrl_write_rate(fdctrl, value);
        break;
    case FD_REG_FIFO:
        fdctrl_write_data(fdctrl, value);
        break;
    case FD_REG_CCR:
        fdctrl_write_ccr(fdctrl, value);
        break;
    default:
        break;
    }
}

/* Change IRQ state */
static void fdctrl_reset_irq(fdctrl_t *fdctrl)
{
    if (!(fdctrl->sra & FD_SRA_INTPEND))
        return;
    FLOPPY_DPRINTF("Reset interrupt\n");
#ifdef VBOX
    PDMDevHlpISASetIrq (fdctrl->pDevIns, fdctrl->irq_lvl, 0);
#else
    qemu_set_irq(fdctrl->irq, 0);
#endif
    fdctrl->sra &= ~FD_SRA_INTPEND;
}

static void fdctrl_raise_irq(fdctrl_t *fdctrl, uint8_t status0)
{
    if (!(fdctrl->sra & FD_SRA_INTPEND)) {
        FLOPPY_DPRINTF("Raising interrupt...\n");
#ifdef VBOX
        PDMDevHlpISASetIrq (fdctrl->pDevIns, fdctrl->irq_lvl, 1);
#else
        qemu_set_irq(fdctrl->irq, 1);
#endif
        fdctrl->sra |= FD_SRA_INTPEND;
    }
    if (status0 & FD_SR0_SEEK) {
        fdrive_t    *cur_drv;

        /* A seek clears the disk change line (if a disk is inserted). */
        cur_drv = get_cur_drv(fdctrl);
        if (cur_drv->max_track)
            cur_drv->dsk_chg = false;
    }

    fdctrl->reset_sensei = 0;
    fdctrl->status0 = status0;
    FLOPPY_DPRINTF("Set interrupt status to 0x%02x\n", fdctrl->status0);
}

/* Reset controller */
static void fdctrl_reset(fdctrl_t *fdctrl, int do_irq)
{
    int i;

    FLOPPY_DPRINTF("reset controller\n");
    fdctrl_reset_irq(fdctrl);
    /* Initialise controller */
    fdctrl->sra = 0;
    fdctrl->srb = 0xc0;
#ifdef VBOX
    if (!fdctrl->drives[1].pDrvMedia)
#else
    if (!fdctrl->drives[1].bs)
#endif
        fdctrl->sra |= FD_SRA_nDRV2;
    fdctrl->cur_drv = 0;
    fdctrl->dor = FD_DOR_nRESET;
    fdctrl->dor |= (fdctrl->dma_chann != 0xff) ? FD_DOR_DMAEN : 0;
    fdctrl->msr = FD_MSR_RQM;
    /* FIFO state */
    fdctrl->data_pos = 0;
    fdctrl->data_len = 0;
    fdctrl->data_state = 0;
    fdctrl->data_dir = FD_DIR_WRITE;
    for (i = 0; i < MAX_FD; i++)
        fd_recalibrate(&fdctrl->drives[i]);
    fdctrl_reset_fifo(fdctrl);
    if (do_irq) {
        fdctrl_raise_irq(fdctrl, FD_SR0_RDYCHG);
        fdctrl->reset_sensei = FD_RESET_SENSEI_COUNT;
    }
}

static inline fdrive_t *drv0(fdctrl_t *fdctrl)
{
    return &fdctrl->drives[(fdctrl->tdr & FD_TDR_BOOTSEL) >> 2];
}

static inline fdrive_t *drv1(fdctrl_t *fdctrl)
{
    if ((fdctrl->tdr & FD_TDR_BOOTSEL) < (1 << 2))
        return &fdctrl->drives[1];
    else
        return &fdctrl->drives[0];
}

#if MAX_FD == 4
static inline fdrive_t *drv2(fdctrl_t *fdctrl)
{
    if ((fdctrl->tdr & FD_TDR_BOOTSEL) < (2 << 2))
        return &fdctrl->drives[2];
    else
        return &fdctrl->drives[1];
}

static inline fdrive_t *drv3(fdctrl_t *fdctrl)
{
    if ((fdctrl->tdr & FD_TDR_BOOTSEL) < (3 << 2))
        return &fdctrl->drives[3];
    else
        return &fdctrl->drives[2];
}
#endif

static fdrive_t *get_cur_drv(fdctrl_t *fdctrl)
{
    switch (fdctrl->cur_drv) {
        case 0: return drv0(fdctrl);
        case 1: return drv1(fdctrl);
#if MAX_FD == 4
        case 2: return drv2(fdctrl);
        case 3: return drv3(fdctrl);
#endif
        default: return NULL;
    }
}

/* Status A register : 0x00 (read-only) */
static uint32_t fdctrl_read_statusA(fdctrl_t *fdctrl)
{
    uint32_t retval = fdctrl->sra;

    FLOPPY_DPRINTF("status register A: 0x%02x\n", retval);

    return retval;
}

/* Status B register : 0x01 (read-only) */
static uint32_t fdctrl_read_statusB(fdctrl_t *fdctrl)
{
    uint32_t retval = fdctrl->srb;

    FLOPPY_DPRINTF("status register B: 0x%02x\n", retval);

    return retval;
}

/* Digital output register : 0x02 */
static uint32_t fdctrl_read_dor(fdctrl_t *fdctrl)
{
    uint32_t retval = fdctrl->dor;

    /* Selected drive */
    retval |= fdctrl->cur_drv;
    FLOPPY_DPRINTF("digital output register: 0x%02x\n", retval);

    return retval;
}

static void fdctrl_write_dor(fdctrl_t *fdctrl, uint32_t value)
{
    FLOPPY_DPRINTF("digital output register set to 0x%02x\n", value);

    /* Motors */
    if (value & FD_DOR_MOTEN0)
        fdctrl->srb |= FD_SRB_MTR0;
    else
        fdctrl->srb &= ~FD_SRB_MTR0;
    if (value & FD_DOR_MOTEN1)
        fdctrl->srb |= FD_SRB_MTR1;
    else
        fdctrl->srb &= ~FD_SRB_MTR1;

    /* Drive */
    if (value & 1)
        fdctrl->srb |= FD_SRB_DR0;
    else
        fdctrl->srb &= ~FD_SRB_DR0;

    /* Reset */
    if (!(value & FD_DOR_nRESET)) {
        if (fdctrl->dor & FD_DOR_nRESET) {
            FLOPPY_DPRINTF("controller enter RESET state\n");
        }
    } else {
        if (!(fdctrl->dor & FD_DOR_nRESET)) {
            FLOPPY_DPRINTF("controller out of RESET state\n");
            fdctrl_reset(fdctrl, 1);
            fdctrl->dsr &= ~FD_DSR_PWRDOWN;
        }
    }
    /* Selected drive */
    fdctrl->cur_drv = value & FD_DOR_SELMASK;

    fdctrl->dor = value;
}

/* Tape drive register : 0x03 */
static uint32_t fdctrl_read_tape(fdctrl_t *fdctrl)
{
    uint32_t retval = fdctrl->tdr;

    FLOPPY_DPRINTF("tape drive register: 0x%02x\n", retval);

    return retval;
}

static void fdctrl_write_tape(fdctrl_t *fdctrl, uint32_t value)
{
    /* Reset mode */
    if (!(fdctrl->dor & FD_DOR_nRESET)) {
        FLOPPY_DPRINTF("Floppy controller in RESET state !\n");
        return;
    }
    FLOPPY_DPRINTF("tape drive register set to 0x%02x\n", value);
    /* Disk boot selection indicator */
    fdctrl->tdr = value & FD_TDR_BOOTSEL;
    /* Tape indicators: never allow */
}

/* Main status register : 0x04 (read) */
static uint32_t fdctrl_read_main_status(fdctrl_t *fdctrl)
{
    uint32_t retval = fdctrl->msr;

    fdctrl->dsr &= ~FD_DSR_PWRDOWN;
    fdctrl->dor |= FD_DOR_nRESET;

    FLOPPY_DPRINTF("main status register: 0x%02x\n", retval);

    return retval;
}

/* Data select rate register : 0x04 (write) */
static void fdctrl_write_rate(fdctrl_t *fdctrl, uint32_t value)
{
    /* Reset mode */
    if (!(fdctrl->dor & FD_DOR_nRESET)) {
        FLOPPY_DPRINTF("Floppy controller in RESET state !\n");
        return;
    }
    FLOPPY_DPRINTF("select rate register set to 0x%02x\n", value);
    /* Reset: autoclear */
    if (value & FD_DSR_SWRESET) {
        fdctrl->dor &= ~FD_DOR_nRESET;
        fdctrl_reset(fdctrl, 1);
        fdctrl->dor |= FD_DOR_nRESET;
    }
    if (value & FD_DSR_PWRDOWN) {
        fdctrl_reset(fdctrl, 1);
    }
    fdctrl->dsr = value;
}

/* Configuration control register : 0x07 (write) */
static void fdctrl_write_ccr(fdctrl_t *fdctrl, uint32_t value)
{
    /* Reset mode */
    if (!(fdctrl->dor & FD_DOR_nRESET)) {
        FLOPPY_DPRINTF("Floppy controller in RESET state !\n");
        return;
    }
    FLOPPY_DPRINTF("configuration control register set to 0x%02x\n", value);

    /* Only the rate selection bits used in AT mode, and we
     * store those in the DSR.
     */
    fdctrl->dsr = (fdctrl->dsr & ~FD_DSR_DRATEMASK) | (value & FD_DSR_DRATEMASK);
}

static int fdctrl_media_changed(fdrive_t *drv)
{
#ifdef VBOX
    return drv->dsk_chg;
#else
    int ret;

    if (!drv->bs)
        return 0;
    ret = bdrv_media_changed(drv->bs);
    if (ret) {
        fd_revalidate(drv);
    }
    return ret;
#endif
}

/* Digital input register : 0x07 (read-only) */
static uint32_t fdctrl_read_dir(fdctrl_t *fdctrl)
{
    uint32_t retval = 0;

#ifdef VBOX
    /* The change line signal is reported by the currently selected
     * drive. If the corresponding motor on bit is not set, the drive
     * is *not* selected!
     */
    if (fdctrl_media_changed(get_cur_drv(fdctrl))
     && (fdctrl->dor & (0x10 << fdctrl->cur_drv)))
#else
    if (fdctrl_media_changed(drv0(fdctrl))
     || fdctrl_media_changed(drv1(fdctrl))
#if MAX_FD == 4
     || fdctrl_media_changed(drv2(fdctrl))
     || fdctrl_media_changed(drv3(fdctrl))
#endif
        )
#endif
        retval |= FD_DIR_DSKCHG;
    if (retval != 0)
        FLOPPY_DPRINTF("Floppy digital input register: 0x%02x\n", retval);

    return retval;
}

/* FIFO state control */
static void fdctrl_reset_fifo(fdctrl_t *fdctrl)
{
    fdctrl->data_dir = FD_DIR_WRITE;
    fdctrl->data_pos = 0;
    fdctrl->msr &= ~(FD_MSR_CMDBUSY | FD_MSR_DIO);
}

/* Set FIFO status for the host to read */
static void fdctrl_set_fifo(fdctrl_t *fdctrl, int fifo_len, int do_irq)
{
    fdctrl->data_dir = FD_DIR_READ;
    fdctrl->data_len = fifo_len;
    fdctrl->data_pos = 0;
    fdctrl->msr |= FD_MSR_CMDBUSY | FD_MSR_RQM | FD_MSR_DIO;
    if (do_irq)
        fdctrl_raise_irq(fdctrl, 0x00);
}

/* Set an error: unimplemented/unknown command */
static void fdctrl_unimplemented(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    FLOPPY_ERROR("unimplemented command 0x%02x\n", fdctrl->fifo[0]);
    fdctrl->fifo[0] = FD_SR0_INVCMD;
    fdctrl_set_fifo(fdctrl, 1, 0);
}

/* Seek to next sector */
static int fdctrl_seek_to_next_sect(fdctrl_t *fdctrl, fdrive_t *cur_drv)
{
    FLOPPY_DPRINTF("seek to next sector (%d %02x %02x => %d)\n",
                   cur_drv->head, cur_drv->track, cur_drv->sect,
                   fd_sector(cur_drv));
    /* XXX: cur_drv->sect >= cur_drv->last_sect should be an
       error in fact */
    if (cur_drv->sect >= cur_drv->last_sect ||
        cur_drv->sect == fdctrl->eot) {
        cur_drv->sect = 1;
        if (FD_MULTI_TRACK(fdctrl->data_state)) {
            if (cur_drv->head == 0 &&
                (cur_drv->flags & FDISK_DBL_SIDES) != 0) {
                cur_drv->head = 1;
            } else {
                cur_drv->head = 0;
                cur_drv->ltrk++;
                if ((cur_drv->flags & FDISK_DBL_SIDES) == 0)
                    return 0;
            }
        } else {
            cur_drv->ltrk++;
            return 0;
        }
        FLOPPY_DPRINTF("seek to next track (%d %02x %02x => %d)\n",
                       cur_drv->head, cur_drv->track,
                       cur_drv->sect, fd_sector(cur_drv));
    } else {
        cur_drv->sect++;
    }
    return 1;
}

/* Callback for transfer end (stop or abort) */
static void fdctrl_stop_transfer(fdctrl_t *fdctrl, uint8_t status0,
                                 uint8_t status1, uint8_t status2)
{
    fdrive_t *cur_drv;

    cur_drv = get_cur_drv(fdctrl);
    FLOPPY_DPRINTF("transfer status: %02x %02x %02x (%02x)\n",
                   status0, status1, status2,
                   status0 | (cur_drv->head << 2) | GET_CUR_DRV(fdctrl));
    fdctrl->fifo[0] = status0 | (cur_drv->head << 2) | GET_CUR_DRV(fdctrl);
    fdctrl->fifo[1] = status1;
    fdctrl->fifo[2] = status2;
    fdctrl->fifo[3] = cur_drv->ltrk;
    fdctrl->fifo[4] = cur_drv->head;
    fdctrl->fifo[5] = cur_drv->sect;
    fdctrl->fifo[6] = FD_SECTOR_SC;
    FLOPPY_DPRINTF("ST0:%02x ST1:%02x ST2:%02x C:%02x H:%02x R:%02x N:%02x\n",
                   fdctrl->fifo[0], fdctrl->fifo[1], fdctrl->fifo[2], fdctrl->fifo[3],
                   fdctrl->fifo[4], fdctrl->fifo[5], fdctrl->fifo[6]);

    fdctrl->data_dir = FD_DIR_READ;
    if (!(fdctrl->msr & FD_MSR_NONDMA)) {
#ifdef VBOX
        PDMDevHlpDMASetDREQ (fdctrl->pDevIns, fdctrl->dma_chann, 0);
#else
        DMA_release_DREQ(fdctrl->dma_chann);
#endif
    }
    fdctrl->msr |= FD_MSR_RQM | FD_MSR_DIO;
    fdctrl->msr &= ~FD_MSR_NONDMA;
    fdctrl_set_fifo(fdctrl, 7, 1);
}

/* Prepare a data transfer (either DMA or FIFO) */
static void fdctrl_start_transfer(fdctrl_t *fdctrl, int direction)
{
    fdrive_t *cur_drv;
    uint8_t kh, kt, ks;
    int did_seek = 0;

    SET_CUR_DRV(fdctrl, fdctrl->fifo[1] & FD_DOR_SELMASK);
    cur_drv = get_cur_drv(fdctrl);
    kt = fdctrl->fifo[2];
    kh = fdctrl->fifo[3];
    ks = fdctrl->fifo[4];
    FLOPPY_DPRINTF("Start transfer at %d %d %02x %02x (%d)\n",
                   GET_CUR_DRV(fdctrl), kh, kt, ks,
                   fd_sector_calc(kh, kt, ks, cur_drv->last_sect, NUM_SIDES(cur_drv)));
    FLOPPY_DPRINTF("CMD:%02x SEL:%02x C:%02x H:%02x R:%02x N:%02x EOT:%02x GPL:%02x DTL:%02x\n",
                   fdctrl->fifo[0], fdctrl->fifo[1], fdctrl->fifo[2],
                   fdctrl->fifo[3], fdctrl->fifo[4], fdctrl->fifo[5],
                   fdctrl->fifo[6], fdctrl->fifo[7], fdctrl->fifo[8]);
    switch (fd_seek(cur_drv, kh, kt, ks, fdctrl->config & FD_CONFIG_EIS)) {
    case 2:
        /* sect too big */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, 0x00, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 3:
        /* track too big */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_EC, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 4:
        /* No seek enabled */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, 0x00, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 5:
        /* No disk in drive */
        /// @todo This is wrong! Command should not complete.
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM | 0x08, /*FD_SR1_MA |*/ FD_SR1_ND, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 1:
        did_seek = 1;
        break;
    default:
        break;
    }
    /* Check the data rate. If the programmed data rate does not match
     * the currently inserted medium, the operation has to fail.
     */
#ifdef VBOX
    if ((fdctrl->dsr & FD_DSR_DRATEMASK) != cur_drv->media_rate) {
        FLOPPY_DPRINTF("data rate mismatch (fdc=%d, media=%d)\n",
                       fdctrl->dsr & FD_DSR_DRATEMASK, cur_drv->media_rate);
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_MA, FD_SR2_MD);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    }
#endif
    /* Set the FIFO state */
    fdctrl->data_dir = direction;
    fdctrl->data_pos = 0;
    fdctrl->msr |= FD_MSR_CMDBUSY;
    if (fdctrl->fifo[0] & 0x80)
        fdctrl->data_state |= FD_STATE_MULTI;
    else
        fdctrl->data_state &= ~FD_STATE_MULTI;
    if (did_seek)
        fdctrl->data_state |= FD_STATE_SEEK;
    else
        fdctrl->data_state &= ~FD_STATE_SEEK;
    if (fdctrl->fifo[5] == 00) {
        fdctrl->data_len = fdctrl->fifo[8];
    } else {
        int tmp;
        fdctrl->data_len = 128 << (fdctrl->fifo[5] > 7 ? 7 : fdctrl->fifo[5]);
        tmp = (fdctrl->fifo[6] - ks + 1);
        if (fdctrl->fifo[0] & 0x80)
            tmp += fdctrl->fifo[6];
        fdctrl->data_len *= tmp;
    }
    fdctrl->eot = fdctrl->fifo[6];
    if (fdctrl->dor & FD_DOR_DMAEN) {
        int dma_mode;
        /* DMA transfer are enabled. Check if DMA channel is well programmed */
#ifndef VBOX
        dma_mode = DMA_get_channel_mode(fdctrl->dma_chann);
#else
        dma_mode = PDMDevHlpDMAGetChannelMode (fdctrl->pDevIns, fdctrl->dma_chann);
#endif
        dma_mode = (dma_mode >> 2) & 3;
        FLOPPY_DPRINTF("dma_mode=%d direction=%d (%d - %d)\n",
                       dma_mode, direction,
                       (128 << fdctrl->fifo[5]) *
                       (cur_drv->last_sect - ks + 1), fdctrl->data_len);
        if (((direction == FD_DIR_SCANE || direction == FD_DIR_SCANL ||
              direction == FD_DIR_SCANH) && dma_mode == 0) ||
            (direction == FD_DIR_WRITE && dma_mode == 2) ||
            (direction == FD_DIR_READ && (dma_mode == 1 || dma_mode == 0))) {
            /* No access is allowed until DMA transfer has completed */
            fdctrl->msr &= ~FD_MSR_RQM;
            /* Now, we just have to wait for the DMA controller to
             * recall us...
             */
#ifndef VBOX
            DMA_hold_DREQ(fdctrl->dma_chann);
            DMA_schedule(fdctrl->dma_chann);
#else
            PDMDevHlpDMASetDREQ (fdctrl->pDevIns, fdctrl->dma_chann, 1);
            PDMDevHlpDMASchedule (fdctrl->pDevIns);
#endif
            return;
        } else {
            FLOPPY_ERROR("dma_mode=%d direction=%d\n", dma_mode, direction);
        }
    }
    FLOPPY_DPRINTF("start non-DMA transfer\n");
    fdctrl->msr |= FD_MSR_NONDMA;
    if (direction != FD_DIR_WRITE)
        fdctrl->msr |= FD_MSR_DIO;
    /* IO based transfer: calculate len */
    fdctrl_raise_irq(fdctrl, 0x00);

    return;
}

/* Prepare a format data transfer (either DMA or FIFO) */
static void fdctrl_start_format(fdctrl_t *fdctrl)
{
    fdrive_t *cur_drv;
    uint8_t ns, dp, kh, kt, ks;

    SET_CUR_DRV(fdctrl, fdctrl->fifo[1] & FD_DOR_SELMASK);
    cur_drv = get_cur_drv(fdctrl);
    kt = cur_drv->track;
    kh = (fdctrl->fifo[1] & 0x04) >> 2;
    ns = fdctrl->fifo[3];
    dp = fdctrl->fifo[5];
    ks = 1;
    FLOPPY_DPRINTF("Start format at %d %d %02x, %d sect, pat %02x (%d)\n",
                   GET_CUR_DRV(fdctrl), kh, kt, ns, dp,
                   fd_sector_calc(kh, kt, ks, cur_drv->last_sect, NUM_SIDES(cur_drv)));
    switch (fd_seek(cur_drv, kh, kt, ks, false)) {
    case 2:
        /* sect too big */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, 0x00, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 3:
        /* track too big */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_EC, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 4:
        /* No seek enabled */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, 0x00, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 5:
        /* No disk in drive */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_MA, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 1:
        break;
    default:
        break;
    }
    /* It's not clear what should happen if the data rate does not match. */
#if 0
    /* Check the data rate. If the programmed data rate does not match
     * the currently inserted medium, the operation has to fail.
     */
    if ((fdctrl->dsr & FD_DSR_DRATEMASK) != cur_drv->media_rate) {
        FLOPPY_DPRINTF("data rate mismatch (fdc=%d, media=%d)\n",
                       fdctrl->dsr & FD_DSR_DRATEMASK, cur_drv->media_rate);
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_MA, FD_SR2_MD);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    }
#endif
    /* Set the FIFO state */
    fdctrl->data_dir = FD_DIR_FORMAT;
    fdctrl->data_pos = 0;
    fdctrl->msr |= FD_MSR_CMDBUSY;
    fdctrl->data_state &= ~(FD_STATE_MULTI | FD_STATE_SEEK);
    fdctrl->data_len = ns * 4;
    fdctrl->eot = ns;
    if (fdctrl->dor & FD_DOR_DMAEN) {
        int dma_mode;
        /* DMA transfer are enabled. Check if DMA channel is well programmed */
#ifndef VBOX
        dma_mode = DMA_get_channel_mode(fdctrl->dma_chann);
#else
        dma_mode = PDMDevHlpDMAGetChannelMode (fdctrl->pDevIns, fdctrl->dma_chann);
#endif
        dma_mode = (dma_mode >> 2) & 3;
        FLOPPY_DPRINTF("dma_mode=%d direction=%d (%d - %d)\n",
                       dma_mode, fdctrl->data_dir,
                       (128 << fdctrl->fifo[2]) *
                       (cur_drv->last_sect + 1), fdctrl->data_len);
        if (fdctrl->data_dir == FD_DIR_FORMAT && dma_mode == 2) {
            /* No access is allowed until DMA transfer has completed */
            fdctrl->msr &= ~FD_MSR_RQM;
            /* Now, we just have to wait for the DMA controller to
             * recall us...
             */
#ifndef VBOX
            DMA_hold_DREQ(fdctrl->dma_chann);
            DMA_schedule(fdctrl->dma_chann);
#else
            PDMDevHlpDMASetDREQ (fdctrl->pDevIns, fdctrl->dma_chann, 1);
            PDMDevHlpDMASchedule (fdctrl->pDevIns);
#endif
            return;
        } else {
            FLOPPY_ERROR("dma_mode=%d direction=%d\n", dma_mode, fdctrl->data_dir);
        }
    }
    FLOPPY_DPRINTF("start non-DMA format\n");
    fdctrl->msr |= FD_MSR_NONDMA;
    /* IO based transfer: calculate len */
    fdctrl_raise_irq(fdctrl, 0x00);

    return;
}

/* Prepare a transfer of deleted data */
static void fdctrl_start_transfer_del(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    FLOPPY_ERROR("fdctrl_start_transfer_del() unimplemented\n");

    /* We don't handle deleted data,
     * so we don't return *ANYTHING*
     */
    fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM | FD_SR0_SEEK, 0x00, 0x00);
}

#ifdef VBOX
/* Block driver read/write wrappers. */

static int blk_write(fdrive_t *drv, int64_t sector_num, const uint8_t *buf, int nb_sectors)
{
    int     rc;

    drv->Led.Asserted.s.fWriting = drv->Led.Actual.s.fWriting = 1;

    rc = drv->pDrvMedia->pfnWrite(drv->pDrvMedia, sector_num * FD_SECTOR_LEN,
                                  buf, nb_sectors * FD_SECTOR_LEN);

    drv->Led.Actual.s.fWriting = 0;
    if (RT_FAILURE(rc))
        AssertMsgFailed(("Floppy: Failure to read sector %d. rc=%Rrc", sector_num, rc));

    return rc;
}

static int blk_read(fdrive_t *drv, int64_t sector_num, uint8_t *buf, int nb_sectors)
{
    int     rc;

    drv->Led.Asserted.s.fReading = drv->Led.Actual.s.fReading = 1;

    rc = drv->pDrvMedia->pfnRead(drv->pDrvMedia, sector_num * FD_SECTOR_LEN,
                                 buf, nb_sectors * FD_SECTOR_LEN);

    drv->Led.Actual.s.fReading = 0;

    if (RT_FAILURE(rc))
        AssertMsgFailed(("Floppy: Failure to read sector %d. rc=%Rrc", sector_num, rc));

    return rc;
}

#endif

/* handlers for DMA transfers */
#ifdef VBOX
static DECLCALLBACK(uint32_t) fdctrl_transfer_handler (PPDMDEVINS pDevIns,
                                                       void *opaque,
                                                       unsigned nchan,
                                                       uint32_t dma_pos,
                                                       uint32_t dma_len)
#else
static int fdctrl_transfer_handler (void *opaque, int nchan,
                                    int dma_pos, int dma_len)
#endif
{
    RT_NOREF(pDevIns, dma_pos);
    fdctrl_t *fdctrl;
    fdrive_t *cur_drv;
#ifdef VBOX
    int rc;
    uint32_t len = 0;
    uint32_t start_pos, rel_pos;
#else
    int len, start_pos, rel_pos;
#endif
    uint8_t status0 = 0x00, status1 = 0x00, status2 = 0x00;

    fdctrl = (fdctrl_t *)opaque;
    if (fdctrl->msr & FD_MSR_RQM) {
        FLOPPY_DPRINTF("Not in DMA transfer mode !\n");
        return 0;
    }
    cur_drv = get_cur_drv(fdctrl);
    if (fdctrl->data_dir == FD_DIR_SCANE || fdctrl->data_dir == FD_DIR_SCANL ||
        fdctrl->data_dir == FD_DIR_SCANH)
        status2 = FD_SR2_SNS;
    if (dma_len > fdctrl->data_len)
        dma_len = fdctrl->data_len;
#ifndef VBOX
    if (cur_drv->bs == NULL)
#else  /* !VBOX */
    if (cur_drv->pDrvMedia == NULL)
#endif
    {
        if (fdctrl->data_dir == FD_DIR_WRITE)
            fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM | FD_SR0_SEEK, 0x00, 0x00);
        else
            fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, 0x00, 0x00);
        Assert(len == 0);
        goto transfer_error;
    }

#ifdef VBOX
        if (cur_drv->ro)
        {
            if (fdctrl->data_dir == FD_DIR_WRITE || fdctrl->data_dir == FD_DIR_FORMAT)
            {
                /* Handle readonly medium early, no need to do DMA, touch the
                 * LED or attempt any writes. A real floppy doesn't attempt
                 * to write to readonly media either. */
                fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM | FD_SR0_SEEK, FD_SR1_NW,
                                     0x00);
                Assert(len == 0);
                goto transfer_error;
            }
        }
#endif


    rel_pos = fdctrl->data_pos % FD_SECTOR_LEN;
    for (start_pos = fdctrl->data_pos; fdctrl->data_pos < dma_len;) {
        len = dma_len - fdctrl->data_pos;
        if (len + rel_pos > FD_SECTOR_LEN)
            len = FD_SECTOR_LEN - rel_pos;
        FLOPPY_DPRINTF("copy %d bytes (%d %d %d) %d pos %d %02x "
                       "(%d-0x%08x 0x%08x)\n", len, dma_len, fdctrl->data_pos,
                       fdctrl->data_len, GET_CUR_DRV(fdctrl), cur_drv->head,
                       cur_drv->track, cur_drv->sect, fd_sector(cur_drv),
                       fd_sector(cur_drv) * FD_SECTOR_LEN);
        if (fdctrl->data_dir != FD_DIR_FORMAT &&
            (fdctrl->data_dir != FD_DIR_WRITE ||
            len < FD_SECTOR_LEN || rel_pos != 0)) {
            /* READ & SCAN commands and realign to a sector for WRITE */
#ifdef VBOX
            rc = blk_read(cur_drv, fd_sector(cur_drv), fdctrl->fifo, 1);
            if (RT_FAILURE(rc))
#else
            if (bdrv_read(cur_drv->bs, fd_sector(cur_drv),
                          fdctrl->fifo, 1) < 0)
#endif
            {
                FLOPPY_DPRINTF("Floppy: error getting sector %d\n",
                               fd_sector(cur_drv));
                /* Sure, image size is too small... */
                memset(fdctrl->fifo, 0, FD_SECTOR_LEN);
            }
        }
        switch (fdctrl->data_dir) {
        case FD_DIR_READ:
            /* READ commands */
#ifdef VBOX
            {
                uint32_t read;
                int rc2 = PDMDevHlpDMAWriteMemory(fdctrl->pDevIns, nchan,
                                                  fdctrl->fifo + rel_pos,
                                                  fdctrl->data_pos,
                                                  len, &read);
                AssertMsgRC (rc2, ("DMAWriteMemory -> %Rrc\n", rc2));
            }
#else
            DMA_write_memory (nchan, fdctrl->fifo + rel_pos,
                              fdctrl->data_pos, len);
#endif
/*          cpu_physical_memory_write(addr + fdctrl->data_pos, */
/*                                    fdctrl->fifo + rel_pos, len); */
            break;
        case FD_DIR_WRITE:
            /* WRITE commands */
#ifdef VBOX
            {
                uint32_t written;
                int rc2 = PDMDevHlpDMAReadMemory(fdctrl->pDevIns, nchan,
                                                 fdctrl->fifo + rel_pos,
                                                 fdctrl->data_pos,
                                                 len, &written);
                AssertMsgRC (rc2, ("DMAReadMemory -> %Rrc\n", rc2));
            }

            rc = blk_write(cur_drv, fd_sector(cur_drv), fdctrl->fifo, 1);
            if (RT_FAILURE(rc))
#else
            DMA_read_memory (nchan, fdctrl->fifo + rel_pos,
                             fdctrl->data_pos, len);
            if (bdrv_write(cur_drv->bs, fd_sector(cur_drv),
                           fdctrl->fifo, 1) < 0)
#endif
            {
                FLOPPY_ERROR("writing sector %d\n", fd_sector(cur_drv));
                fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM | FD_SR0_SEEK, 0x00, 0x00);
                goto transfer_error;
            }
            break;
#ifdef VBOX
        case FD_DIR_FORMAT:
            /* FORMAT command */
            {
                uint8_t  eot    = fdctrl->fifo[3];
                uint8_t  filler = fdctrl->fifo[5];
                uint32_t written;
                int      sct;
                int rc2 = PDMDevHlpDMAReadMemory(fdctrl->pDevIns, nchan,
                                                 fdctrl->fifo + rel_pos,
                                                 fdctrl->data_pos,
                                                 len, &written);
                AssertMsgRC (rc2, ("DMAReadMemory -> %Rrc\n", rc2));

                /* Fill the entire track with desired data pattern. */
                FLOPPY_DPRINTF("formatting track: %d sectors, pattern %02x\n",
                               eot, filler);
                memset(fdctrl->fifo, filler, FD_SECTOR_LEN);
                for (sct = 0; sct < eot; ++sct)
                {
                    rc = blk_write(cur_drv, fd_sector(cur_drv), fdctrl->fifo, 1);
                    if (RT_FAILURE(rc))
                    {
                        FLOPPY_ERROR("formatting sector %d\n", fd_sector(cur_drv));
                        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM | FD_SR0_SEEK, 0x00, 0x00);
                        goto transfer_error;
                    }
                    fdctrl_seek_to_next_sect(fdctrl, cur_drv);
                }
            }
            break;
#endif
        default:
            /* SCAN commands */
            {
                uint8_t tmpbuf[FD_SECTOR_LEN];
                int ret;
#ifdef VBOX
                uint32_t read;
                int rc2 = PDMDevHlpDMAReadMemory (fdctrl->pDevIns, nchan, tmpbuf,
                                                  fdctrl->data_pos, len, &read);
                AssertMsg(RT_SUCCESS(rc2), ("DMAReadMemory -> %Rrc2\n", rc2)); NOREF(rc2);
#else
                DMA_read_memory (nchan, tmpbuf, fdctrl->data_pos, len);
#endif
                ret = memcmp(tmpbuf, fdctrl->fifo + rel_pos, len);
                if (ret == 0) {
                    status2 = FD_SR2_SEH;
                    goto end_transfer;
                }
                if ((ret < 0 && fdctrl->data_dir == FD_DIR_SCANL) ||
                    (ret > 0 && fdctrl->data_dir == FD_DIR_SCANH)) {
                    status2 = 0x00;
                    goto end_transfer;
                }
            }
            break;
        }
        fdctrl->data_pos += len;
        rel_pos = fdctrl->data_pos % FD_SECTOR_LEN;
        if (rel_pos == 0) {
            /* Seek to next sector */
            if (!fdctrl_seek_to_next_sect(fdctrl, cur_drv))
                break;
        }
    }
end_transfer:
    len = fdctrl->data_pos - start_pos;
    FLOPPY_DPRINTF("end transfer %d %d %d\n",
                   fdctrl->data_pos, len, fdctrl->data_len);
    if (fdctrl->data_dir == FD_DIR_SCANE ||
        fdctrl->data_dir == FD_DIR_SCANL ||
        fdctrl->data_dir == FD_DIR_SCANH)
        status2 = FD_SR2_SEH;
    if (FD_DID_SEEK(fdctrl->data_state))
        status0 |= FD_SR0_SEEK;
    fdctrl->data_len -= len;
    fdctrl_stop_transfer(fdctrl, status0, status1, status2);
transfer_error:

    return len;
}

/* Data register : 0x05 */
static uint32_t fdctrl_read_data(fdctrl_t *fdctrl)
{
    fdrive_t *cur_drv;
    uint32_t retval = 0;
    unsigned pos;
#ifdef VBOX
    int rc;
#endif

    cur_drv = get_cur_drv(fdctrl);
    fdctrl->dsr &= ~FD_DSR_PWRDOWN;
    if (!(fdctrl->msr & FD_MSR_RQM) || !(fdctrl->msr & FD_MSR_DIO)) {
        FLOPPY_ERROR("controller not ready for reading\n");
        return 0;
    }
    pos = fdctrl->data_pos % FD_SECTOR_LEN;
    if (fdctrl->msr & FD_MSR_NONDMA) {
        if (pos == 0) {
            if (fdctrl->data_pos != 0)
                if (!fdctrl_seek_to_next_sect(fdctrl, cur_drv)) {
                    FLOPPY_DPRINTF("error seeking to next sector %d\n",
                                   fd_sector(cur_drv));
                    return 0;
                }
#ifdef VBOX
            rc = blk_read(cur_drv, fd_sector(cur_drv), fdctrl->fifo, 1);
            if (RT_FAILURE(rc))
#else
            if (bdrv_read(cur_drv->bs, fd_sector(cur_drv), fdctrl->fifo, 1) < 0)
#endif
            {
                FLOPPY_DPRINTF("error getting sector %d\n",
                               fd_sector(cur_drv));
                /* Sure, image size is too small... */
                memset(fdctrl->fifo, 0, FD_SECTOR_LEN);
            }
        }
    }
    retval = fdctrl->fifo[pos];
    if (++fdctrl->data_pos == fdctrl->data_len) {
        fdctrl->data_pos = 0;
        /* Switch from transfer mode to status mode
         * then from status mode to command mode
         */
        if (fdctrl->msr & FD_MSR_NONDMA) {
            fdctrl_stop_transfer(fdctrl, FD_SR0_SEEK, 0x00, 0x00);
        } else {
            fdctrl_reset_fifo(fdctrl);
            fdctrl_reset_irq(fdctrl);
        }
    }
    FLOPPY_DPRINTF("data register: 0x%02x\n", retval);

    return retval;
}

static void fdctrl_format_sector(fdctrl_t *fdctrl)
{
    fdrive_t *cur_drv;
    uint8_t kh, kt, ks;
#ifdef VBOX
    int ok = 0, rc;
#endif

    SET_CUR_DRV(fdctrl, fdctrl->fifo[1] & FD_DOR_SELMASK);
    cur_drv = get_cur_drv(fdctrl);
    kt = fdctrl->fifo[6];
    kh = fdctrl->fifo[7];
    ks = fdctrl->fifo[8];
    FLOPPY_DPRINTF("format sector at %d %d %02x %02x (%d)\n",
                   GET_CUR_DRV(fdctrl), kh, kt, ks,
                   fd_sector_calc(kh, kt, ks, cur_drv->last_sect, NUM_SIDES(cur_drv)));
    switch (fd_seek(cur_drv, kh, kt, ks, fdctrl->config & FD_CONFIG_EIS)) {
    case 2:
        /* sect too big */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, 0x00, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 3:
        /* track too big */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_EC, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 4:
        /* No seek enabled */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, 0x00, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 5:
        /* No disk in drive */
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_MA, 0x00);
        fdctrl->fifo[3] = kt;
        fdctrl->fifo[4] = kh;
        fdctrl->fifo[5] = ks;
        return;
    case 1:
        fdctrl->data_state |= FD_STATE_SEEK;
        break;
    default:
        break;
    }
    memset(fdctrl->fifo, 0, FD_SECTOR_LEN);
#ifdef VBOX
    if (cur_drv->pDrvMedia) {
        rc = blk_write(cur_drv, fd_sector(cur_drv), fdctrl->fifo, 1);
        if (RT_FAILURE (rc)) {
            FLOPPY_ERROR("formatting sector %d\n", fd_sector(cur_drv));
            fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM | FD_SR0_SEEK, 0x00, 0x00);
        } else {
            ok = 1;
        }
    }
    if (ok) {
#else
    if (cur_drv->bs == NULL ||
        bdrv_write(cur_drv->bs, fd_sector(cur_drv), fdctrl->fifo, 1) < 0) {
        FLOPPY_ERROR("formatting sector %d\n", fd_sector(cur_drv));
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM | FD_SR0_SEEK, 0x00, 0x00);
    } else {
#endif
        if (cur_drv->sect == cur_drv->last_sect) {
            fdctrl->data_state &= ~FD_STATE_FORMAT;
            /* Last sector done */
            if (FD_DID_SEEK(fdctrl->data_state))
                fdctrl_stop_transfer(fdctrl, FD_SR0_SEEK, 0x00, 0x00);
            else
                fdctrl_stop_transfer(fdctrl, 0x00, 0x00, 0x00);
        } else {
            /* More to do */
            fdctrl->data_pos = 0;
            fdctrl->data_len = 4;
        }
    }
}

static void fdctrl_handle_lock(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdctrl->lock = (fdctrl->fifo[0] & 0x80) ? 1 : 0;
    fdctrl->fifo[0] = fdctrl->lock << 4;
    fdctrl_set_fifo(fdctrl, 1, 0);
}

static void fdctrl_handle_dumpreg(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv = get_cur_drv(fdctrl);

    /* Drives position */
    fdctrl->fifo[0] = drv0(fdctrl)->track;
    fdctrl->fifo[1] = drv1(fdctrl)->track;
#if MAX_FD == 4
    fdctrl->fifo[2] = drv2(fdctrl)->track;
    fdctrl->fifo[3] = drv3(fdctrl)->track;
#else
    fdctrl->fifo[2] = 0;
    fdctrl->fifo[3] = 0;
#endif
    /* timers */
    fdctrl->fifo[4] = fdctrl->timer0;
    fdctrl->fifo[5] = (fdctrl->timer1 << 1) | (fdctrl->dor & FD_DOR_DMAEN ? 1 : 0);
    fdctrl->fifo[6] = cur_drv->last_sect;
    fdctrl->fifo[7] = (fdctrl->lock << 7) |
        (cur_drv->perpendicular << 2);
    fdctrl->fifo[8] = fdctrl->config;
    fdctrl->fifo[9] = fdctrl->precomp_trk;
    fdctrl_set_fifo(fdctrl, 10, 0);
}

static void fdctrl_handle_version(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    /* Controller's version */
    fdctrl->fifo[0] = fdctrl->version;
    fdctrl_set_fifo(fdctrl, 1, 0);
}

static void fdctrl_handle_partid(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdctrl->fifo[0] = 0x01; /* Stepping 1 */
    fdctrl_set_fifo(fdctrl, 1, 0);
}

static void fdctrl_handle_restore(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv = get_cur_drv(fdctrl);

    /* Drives position */
    drv0(fdctrl)->track = fdctrl->fifo[3];
    drv1(fdctrl)->track = fdctrl->fifo[4];
#if MAX_FD == 4
    drv2(fdctrl)->track = fdctrl->fifo[5];
    drv3(fdctrl)->track = fdctrl->fifo[6];
#endif
    /* timers */
    fdctrl->timer0 = fdctrl->fifo[7];
    fdctrl->timer1 = fdctrl->fifo[8];
    cur_drv->last_sect = fdctrl->fifo[9];
    fdctrl->lock = fdctrl->fifo[10] >> 7;
    cur_drv->perpendicular = (fdctrl->fifo[10] >> 2) & 0xF;
    fdctrl->config = fdctrl->fifo[11];
    fdctrl->precomp_trk = fdctrl->fifo[12];
    fdctrl->pwrd = fdctrl->fifo[13];
    fdctrl_reset_fifo(fdctrl);
}

static void fdctrl_handle_save(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv = get_cur_drv(fdctrl);

    fdctrl->fifo[0] = 0;
    fdctrl->fifo[1] = 0;
    /* Drives position */
    fdctrl->fifo[2] = drv0(fdctrl)->track;
    fdctrl->fifo[3] = drv1(fdctrl)->track;
#if MAX_FD == 4
    fdctrl->fifo[4] = drv2(fdctrl)->track;
    fdctrl->fifo[5] = drv3(fdctrl)->track;
#else
    fdctrl->fifo[4] = 0;
    fdctrl->fifo[5] = 0;
#endif
    /* timers */
    fdctrl->fifo[6] = fdctrl->timer0;
    fdctrl->fifo[7] = fdctrl->timer1;
    fdctrl->fifo[8] = cur_drv->last_sect;
    fdctrl->fifo[9] = (fdctrl->lock << 7) |
        (cur_drv->perpendicular << 2);
    fdctrl->fifo[10] = fdctrl->config;
    fdctrl->fifo[11] = fdctrl->precomp_trk;
    fdctrl->fifo[12] = fdctrl->pwrd;
    fdctrl->fifo[13] = 0;
    fdctrl->fifo[14] = 0;
    fdctrl_set_fifo(fdctrl, 15, 0);
}

static void fdctrl_handle_readid(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv = get_cur_drv(fdctrl);

    FLOPPY_DPRINTF("CMD:%02x SEL:%02x\n", fdctrl->fifo[0], fdctrl->fifo[1]);

    fdctrl->msr &= ~FD_MSR_RQM;
    cur_drv->head = (fdctrl->fifo[1] >> 2) & 1;
#ifdef VBOX
    TMTimerSetMillies(fdctrl->result_timer, 1000 / 50);
#else
    qemu_mod_timer(fdctrl->result_timer,
                   qemu_get_clock(vm_clock) + (get_ticks_per_sec() / 50));
#endif
}

static void fdctrl_handle_format_track(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv;
    uint8_t ns, dp;

    SET_CUR_DRV(fdctrl, fdctrl->fifo[1] & FD_DOR_SELMASK);
    cur_drv = get_cur_drv(fdctrl);
    fdctrl->data_state &= ~(FD_STATE_MULTI | FD_STATE_SEEK);
    ns = fdctrl->fifo[3];
    dp = fdctrl->fifo[5];

    FLOPPY_DPRINTF("Format track %d at %d, %d sectors, filler %02x\n",
                   cur_drv->track, GET_CUR_DRV(fdctrl), ns, dp);
    FLOPPY_DPRINTF("CMD:%02x SEL:%02x N:%02x SC:%02x GPL:%02x D:%02x\n",
                   fdctrl->fifo[0], fdctrl->fifo[1], fdctrl->fifo[2],
                   fdctrl->fifo[3], fdctrl->fifo[4], fdctrl->fifo[5]);

    /* Since we cannot actually format anything, we have to make sure that
     * whatever new format the guest is trying to establish matches the
     * existing format of the medium.
     */
    if (cur_drv->last_sect != ns || fdctrl->fifo[2] != 2)
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_NW, 0);
    else
    {
        cur_drv->bps = fdctrl->fifo[2] > 7 ? 16384 : 128 << fdctrl->fifo[2];
        cur_drv->last_sect = ns;

        fdctrl_start_format(fdctrl);
    }
}

static void fdctrl_handle_specify(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdctrl->timer0 = (fdctrl->fifo[1] >> 4) & 0xF;
    fdctrl->timer1 = fdctrl->fifo[2] >> 1;
    if (fdctrl->fifo[2] & 1)
        fdctrl->dor &= ~FD_DOR_DMAEN;
    else
        fdctrl->dor |= FD_DOR_DMAEN;
    /* No result back */
    fdctrl_reset_fifo(fdctrl);
}

static void fdctrl_handle_sense_drive_status(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv;

    SET_CUR_DRV(fdctrl, fdctrl->fifo[1] & FD_DOR_SELMASK);
    cur_drv = get_cur_drv(fdctrl);
    cur_drv->head = (fdctrl->fifo[1] >> 2) & 1;
    /* 1 Byte status back */
    fdctrl->fifo[0] = (cur_drv->ro << 6) |
        (cur_drv->track == 0 ? 0x10 : 0x00) |
        (cur_drv->head << 2) |
        GET_CUR_DRV(fdctrl) |
        0x28;
    fdctrl_set_fifo(fdctrl, 1, 0);
}

static void fdctrl_handle_recalibrate(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv;
    uint8_t  st0;

    SET_CUR_DRV(fdctrl, fdctrl->fifo[1] & FD_DOR_SELMASK);
    cur_drv = get_cur_drv(fdctrl);
    fd_recalibrate(cur_drv);
    fdctrl_reset_fifo(fdctrl);
    st0 = FD_SR0_SEEK | GET_CUR_DRV(fdctrl);
    /* No drive means no TRK0 signal. */
    if (cur_drv->drive == FDRIVE_DRV_NONE)
        st0 |= FD_SR0_ABNTERM | FD_SR0_EQPMT;
    /* Raise Interrupt */
    fdctrl_raise_irq(fdctrl, st0);
}

static void fdctrl_handle_sense_interrupt_status(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv = get_cur_drv(fdctrl);

    FLOPPY_DPRINTF("CMD:%02x\n", fdctrl->fifo[0]);
    if(fdctrl->reset_sensei > 0) {
        fdctrl->fifo[0] =
            FD_SR0_RDYCHG + FD_RESET_SENSEI_COUNT - fdctrl->reset_sensei;
        fdctrl->reset_sensei--;
    } else {
        /* XXX: status0 handling is broken for read/write
           commands, so we do this hack. It should be suppressed
           ASAP */
        fdctrl->fifo[0] =
            FD_SR0_SEEK | (cur_drv->head << 2) | GET_CUR_DRV(fdctrl);
        /* Hack to preserve SR0 on equipment check failures (no drive). */
        if (fdctrl->status0 & FD_SR0_EQPMT)
            fdctrl->fifo[0] = fdctrl->status0;
    }

    fdctrl->fifo[1] = cur_drv->track;
    fdctrl_set_fifo(fdctrl, 2, 0);
    FLOPPY_DPRINTF("ST0:%02x PCN:%02x\n", fdctrl->fifo[0], fdctrl->fifo[1]);
    fdctrl->status0 = FD_SR0_RDYCHG;
}

static void fdctrl_handle_seek(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv;

    FLOPPY_DPRINTF("CMD:%02x SEL:%02x NCN:%02x\n", fdctrl->fifo[0],
                   fdctrl->fifo[1], fdctrl->fifo[2]);

    SET_CUR_DRV(fdctrl, fdctrl->fifo[1] & FD_DOR_SELMASK);
    cur_drv = get_cur_drv(fdctrl);
    fdctrl_reset_fifo(fdctrl);
#ifdef VBOX
    /* The seek command just sends step pulses to the drive and doesn't care if
     * there's a medium inserted or if it's banging the head against the drive.
     */
    cur_drv->track = fdctrl->fifo[2];
    cur_drv->ltrk = cur_drv->track;
    cur_drv->head = (fdctrl->fifo[1] >> 2) & 1;
    /* Raise Interrupt */
    fdctrl_raise_irq(fdctrl, FD_SR0_SEEK | GET_CUR_DRV(fdctrl));
#else
    if (fdctrl->fifo[2] > cur_drv->max_track) {
        fdctrl_raise_irq(fdctrl, FD_SR0_ABNTERM | FD_SR0_SEEK);
    } else {
        cur_drv->track = fdctrl->fifo[2];
        /* Raise Interrupt */
        fdctrl_raise_irq(fdctrl, FD_SR0_SEEK);
    }
#endif
}

static void fdctrl_handle_perpendicular_mode(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv = get_cur_drv(fdctrl);

    if (fdctrl->fifo[1] & 0x80)
        cur_drv->perpendicular = fdctrl->fifo[1] & 0x7;
    /* No result back */
    fdctrl_reset_fifo(fdctrl);
}

static void fdctrl_handle_configure(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdctrl->config = fdctrl->fifo[2];
    fdctrl->precomp_trk =  fdctrl->fifo[3];
    /* No result back */
    fdctrl_reset_fifo(fdctrl);
}

static void fdctrl_handle_powerdown_mode(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdctrl->pwrd = fdctrl->fifo[1];
    fdctrl->fifo[0] = fdctrl->fifo[1];
    fdctrl_set_fifo(fdctrl, 1, 0);
}

static void fdctrl_handle_option(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    /* No result back */
    fdctrl_reset_fifo(fdctrl);
}

static void fdctrl_handle_drive_specification_command(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    /* fdrive_t *cur_drv = get_cur_drv(fdctrl); - unused */

    /* This command takes a variable number of parameters. It can be terminated
     * at any time if the high bit of a parameter is set. Once there are 6 bytes
     * in the FIFO (command + 5 parameter bytes), data_len/data_pos will be 7.
     */
    if (fdctrl->data_len == 7 || (fdctrl->fifo[fdctrl->data_pos - 1] & 0x80)) {

        /* Command parameters done */
        if (fdctrl->fifo[fdctrl->data_pos - 1] & 0x40) {
            /* Data is echoed, but not stored! */
            fdctrl->fifo[0] = fdctrl->data_len > 2 ? fdctrl->fifo[1] : 0;
            fdctrl->fifo[1] = fdctrl->data_len > 3 ? fdctrl->fifo[2] : 0;
            fdctrl->fifo[2] = 0;
            fdctrl->fifo[3] = 0;
            fdctrl_set_fifo(fdctrl, 4, 0);
        } else {
            fdctrl_reset_fifo(fdctrl);
        }
    } else
        fdctrl->data_len++; /* Wait for another byte. */
}

static void fdctrl_handle_relative_seek_out(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv;

    SET_CUR_DRV(fdctrl, fdctrl->fifo[1] & FD_DOR_SELMASK);
    cur_drv = get_cur_drv(fdctrl);
    if (fdctrl->fifo[2] + cur_drv->track >= cur_drv->max_track) {
        cur_drv->track = cur_drv->max_track - 1;
    } else {
        cur_drv->track += fdctrl->fifo[2];
    }
    fdctrl_reset_fifo(fdctrl);
    /* Raise Interrupt */
    fdctrl_raise_irq(fdctrl, FD_SR0_SEEK);
}

static void fdctrl_handle_relative_seek_in(fdctrl_t *fdctrl, int direction)
{
    RT_NOREF(direction);
    fdrive_t *cur_drv;

    SET_CUR_DRV(fdctrl, fdctrl->fifo[1] & FD_DOR_SELMASK);
    cur_drv = get_cur_drv(fdctrl);
    if (fdctrl->fifo[2] > cur_drv->track) {
        cur_drv->track = 0;
    } else {
        cur_drv->track -= fdctrl->fifo[2];
    }
    fdctrl_reset_fifo(fdctrl);
    /* Raise Interrupt */
    fdctrl_raise_irq(fdctrl, FD_SR0_SEEK);
}

static const struct {
    uint8_t value;
    uint8_t mask;
    const char* name;
    int parameters;
    void (*handler)(fdctrl_t *fdctrl, int direction);
    int direction;
} handlers[] = {
    { FD_CMD_READ, 0x1f, "READ", 8, fdctrl_start_transfer, FD_DIR_READ },
    { FD_CMD_WRITE, 0x3f, "WRITE", 8, fdctrl_start_transfer, FD_DIR_WRITE },
    { FD_CMD_SEEK, 0xff, "SEEK", 2, fdctrl_handle_seek },
    { FD_CMD_SENSE_INTERRUPT_STATUS, 0xff, "SENSE INTERRUPT STATUS", 0, fdctrl_handle_sense_interrupt_status },
    { FD_CMD_RECALIBRATE, 0xff, "RECALIBRATE", 1, fdctrl_handle_recalibrate },
    { FD_CMD_FORMAT_TRACK, 0xbf, "FORMAT TRACK", 5, fdctrl_handle_format_track },
    { FD_CMD_READ_TRACK, 0xbf, "READ TRACK", 8, fdctrl_start_transfer, FD_DIR_READ },
    { FD_CMD_RESTORE, 0xff, "RESTORE", 17, fdctrl_handle_restore }, /* part of READ DELETED DATA */
    { FD_CMD_SAVE, 0xff, "SAVE", 0, fdctrl_handle_save }, /* part of READ DELETED DATA */
    { FD_CMD_READ_DELETED, 0x1f, "READ DELETED DATA", 8, fdctrl_start_transfer_del, FD_DIR_READ },
    { FD_CMD_SCAN_EQUAL, 0x1f, "SCAN EQUAL", 8, fdctrl_start_transfer, FD_DIR_SCANE },
    { FD_CMD_VERIFY, 0x1f, "VERIFY", 8, fdctrl_unimplemented },
    { FD_CMD_SCAN_LOW_OR_EQUAL, 0x1f, "SCAN LOW OR EQUAL", 8, fdctrl_start_transfer, FD_DIR_SCANL },
    { FD_CMD_SCAN_HIGH_OR_EQUAL, 0x1f, "SCAN HIGH OR EQUAL", 8, fdctrl_start_transfer, FD_DIR_SCANH },
    { FD_CMD_WRITE_DELETED, 0x3f, "WRITE DELETED DATA", 8, fdctrl_start_transfer_del, FD_DIR_WRITE },
    { FD_CMD_READ_ID, 0xbf, "READ ID", 1, fdctrl_handle_readid },
    { FD_CMD_SPECIFY, 0xff, "SPECIFY", 2, fdctrl_handle_specify },
    { FD_CMD_SENSE_DRIVE_STATUS, 0xff, "SENSE DRIVE STATUS", 1, fdctrl_handle_sense_drive_status },
    { FD_CMD_PERPENDICULAR_MODE, 0xff, "PERPENDICULAR MODE", 1, fdctrl_handle_perpendicular_mode },
    { FD_CMD_CONFIGURE, 0xff, "CONFIGURE", 3, fdctrl_handle_configure },
    { FD_CMD_POWERDOWN_MODE, 0xff, "POWERDOWN MODE", 2, fdctrl_handle_powerdown_mode },
    { FD_CMD_OPTION, 0xff, "OPTION", 1, fdctrl_handle_option },
    { FD_CMD_DRIVE_SPECIFICATION_COMMAND, 0xff, "DRIVE SPECIFICATION COMMAND", 1, fdctrl_handle_drive_specification_command },
    { FD_CMD_RELATIVE_SEEK_OUT, 0xff, "RELATIVE SEEK OUT", 2, fdctrl_handle_relative_seek_out },
    { FD_CMD_FORMAT_AND_WRITE, 0xff, "FORMAT AND WRITE", 10, fdctrl_unimplemented },
    { FD_CMD_RELATIVE_SEEK_IN, 0xff, "RELATIVE SEEK IN", 2, fdctrl_handle_relative_seek_in },
    { FD_CMD_LOCK, 0x7f, "LOCK", 0, fdctrl_handle_lock },
    { FD_CMD_DUMPREG, 0xff, "DUMPREG", 0, fdctrl_handle_dumpreg },
    { FD_CMD_VERSION, 0xff, "VERSION", 0, fdctrl_handle_version },
    { FD_CMD_PART_ID, 0xff, "PART ID", 0, fdctrl_handle_partid },
    { FD_CMD_WRITE, 0x1f, "WRITE (BeOS)", 8, fdctrl_start_transfer, FD_DIR_WRITE }, /* not in specification ; BeOS 4.5 bug */
    { 0, 0, "unknown", 0, fdctrl_unimplemented }, /* default handler */
};
/* Associate command to an index in the 'handlers' array */
static uint8_t command_to_handler[256];

static void fdctrl_write_data(fdctrl_t *fdctrl, uint32_t value)
{
    fdrive_t *cur_drv;
    int pos;

    cur_drv = get_cur_drv(fdctrl);
    /* Reset mode */
    if (!(fdctrl->dor & FD_DOR_nRESET)) {
        FLOPPY_DPRINTF("Floppy controller in RESET state !\n");
        return;
    }
    if (!(fdctrl->msr & FD_MSR_RQM) || (fdctrl->msr & FD_MSR_DIO)) {
        FLOPPY_ERROR("controller not ready for writing\n");
        return;
    }
    fdctrl->dsr &= ~FD_DSR_PWRDOWN;
    /* Is it write command time ? */
    if (fdctrl->msr & FD_MSR_NONDMA) {
        /* FIFO data write */
        pos = fdctrl->data_pos++;
        pos %= FD_SECTOR_LEN;
        fdctrl->fifo[pos] = value;
        if (pos == FD_SECTOR_LEN - 1 ||
            fdctrl->data_pos == fdctrl->data_len) {
#ifdef VBOX
            blk_write(cur_drv, fd_sector(cur_drv), fdctrl->fifo, 1);
#else
            bdrv_write(cur_drv->bs, fd_sector(cur_drv),
                       fdctrl->fifo, 1);
#endif
        }
        /* Switch from transfer mode to status mode
         * then from status mode to command mode
         */
        if (fdctrl->data_pos == fdctrl->data_len)
            fdctrl_stop_transfer(fdctrl, FD_SR0_SEEK, 0x00, 0x00);
        return;
    }
    if (fdctrl->data_pos == 0) {
        /* Command */
        fdctrl_reset_irq(fdctrl);   /* If pending from previous seek/recalibrate. */
        pos = command_to_handler[value & 0xff];
        FLOPPY_DPRINTF("%s command\n", handlers[pos].name);
        fdctrl->data_len = handlers[pos].parameters + 1;
        fdctrl->msr |= FD_MSR_CMDBUSY;
    }

    FLOPPY_DPRINTF("%s: %02x\n", __FUNCTION__, value);
    fdctrl->fifo[fdctrl->data_pos++ % FD_SECTOR_LEN] = value;
    if (fdctrl->data_pos == fdctrl->data_len) {
        /* We now have all parameters
         * and will be able to treat the command
         */
        if (fdctrl->data_state & FD_STATE_FORMAT) {
            fdctrl_format_sector(fdctrl);
            return;
        }

        pos = command_to_handler[fdctrl->fifo[0] & 0xff];
        FLOPPY_DPRINTF("treat %s command\n", handlers[pos].name);
        (*handlers[pos].handler)(fdctrl, handlers[pos].direction);
    }
}

static void fdctrl_result_timer(void *opaque)
{
    fdctrl_t *fdctrl = (fdctrl_t *)opaque;
    fdrive_t *cur_drv = get_cur_drv(fdctrl);

    /* Pretend we are spinning.
     * This is needed for Coherent, which uses READ ID to check for
     * sector interleaving.
     */
    if (cur_drv->last_sect != 0) {
        cur_drv->sect = (cur_drv->sect % cur_drv->last_sect) + 1;
    }
    /* READ_ID can't automatically succeed! */
#ifdef VBOX
    if (!cur_drv->max_track) {
        FLOPPY_DPRINTF("read id when no disk in drive\n");
        /// @todo This is wrong! Command should not complete.
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_MA | FD_SR1_ND, FD_SR2_MD);
    } else if ((fdctrl->dsr & FD_DSR_DRATEMASK) != cur_drv->media_rate) {
        FLOPPY_DPRINTF("read id rate mismatch (fdc=%d, media=%d)\n",
                       fdctrl->dsr & FD_DSR_DRATEMASK, cur_drv->media_rate);
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_MA | FD_SR1_ND, FD_SR2_MD);
    } else if (cur_drv->track >= cur_drv->max_track) {
        FLOPPY_DPRINTF("read id past last track (%d >= %d)\n",
                       cur_drv->track, cur_drv->max_track);
        cur_drv->ltrk = 0;
        fdctrl_stop_transfer(fdctrl, FD_SR0_ABNTERM, FD_SR1_MA | FD_SR1_ND, FD_SR2_MD);
    }
    else
#endif
        fdctrl_stop_transfer(fdctrl, 0x00, 0x00, 0x00);
}


#ifdef VBOX

/* -=-=-=-=-=-=-=-=- Timer Callback -=-=-=-=-=-=-=-=- */

/**
 * @callback_method_impl{FNTMTIMERDEV}
 */
static DECLCALLBACK(void) fdcTimerCallback(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF(pDevIns, pTimer);
    fdctrl_t *fdctrl = (fdctrl_t *)pvUser;
    fdctrl_result_timer(fdctrl);
}


/* -=-=-=-=-=-=-=-=- I/O Port Access Handlers -=-=-=-=-=-=-=-=- */

/**
 * @callback_method_impl{FNIOMIOPORTOUT}
 */
static DECLCALLBACK(int) fdcIoPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t u32, unsigned cb)
{
    RT_NOREF(pDevIns);
    if (cb == 1)
        fdctrl_write (pvUser, uPort & 7, u32);
    else
        AssertMsgFailed(("uPort=%#x cb=%d u32=%#x\n", uPort, cb, u32));
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNIOMIOPORTIN}
 */
static DECLCALLBACK(int) fdcIoPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t *pu32, unsigned cb)
{
    RT_NOREF(pDevIns);
    if (cb == 1)
    {
        *pu32 = fdctrl_read (pvUser, uPort & 7);
        return VINF_SUCCESS;
    }
    return VERR_IOM_IOPORT_UNUSED;
}


/* -=-=-=-=-=-=-=-=- Saved state -=-=-=-=-=-=-=-=- */

/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) fdcSaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    fdctrl_t *pThis = PDMINS_2_DATA(pDevIns, fdctrl_t *);
    unsigned int i;

    /* Save the FDC I/O registers... */
    SSMR3PutU8(pSSM, pThis->sra);
    SSMR3PutU8(pSSM, pThis->srb);
    SSMR3PutU8(pSSM, pThis->dor);
    SSMR3PutU8(pSSM, pThis->tdr);
    SSMR3PutU8(pSSM, pThis->dsr);
    SSMR3PutU8(pSSM, pThis->msr);
    /* ...the status registers... */
    SSMR3PutU8(pSSM, pThis->status0);
    SSMR3PutU8(pSSM, pThis->status1);
    SSMR3PutU8(pSSM, pThis->status2);
    /* ...the command FIFO... */
    SSMR3PutU32(pSSM, sizeof(pThis->fifo));
    SSMR3PutMem(pSSM, &pThis->fifo, sizeof(pThis->fifo));
    SSMR3PutU32(pSSM, pThis->data_pos);
    SSMR3PutU32(pSSM, pThis->data_len);
    SSMR3PutU8(pSSM, pThis->data_state);
    SSMR3PutU8(pSSM, pThis->data_dir);
    /* ...and miscellaneous internal FDC state. */
    SSMR3PutU8(pSSM, pThis->reset_sensei);
    SSMR3PutU8(pSSM, pThis->eot);
    SSMR3PutU8(pSSM, pThis->timer0);
    SSMR3PutU8(pSSM, pThis->timer1);
    SSMR3PutU8(pSSM, pThis->precomp_trk);
    SSMR3PutU8(pSSM, pThis->config);
    SSMR3PutU8(pSSM, pThis->lock);
    SSMR3PutU8(pSSM, pThis->pwrd);
    SSMR3PutU8(pSSM, pThis->version);

    /* Save the number of drives and per-drive state. Note that the media
     * states will be updated in fd_revalidate() and need not be saved.
     */
    SSMR3PutU8(pSSM, pThis->num_floppies);
    Assert(RT_ELEMENTS(pThis->drives) == pThis->num_floppies);
    for (i = 0; i < pThis->num_floppies; ++i)
    {
        fdrive_t *d = &pThis->drives[i];

        SSMR3PutMem(pSSM, &d->Led, sizeof(d->Led));
        SSMR3PutU32(pSSM, d->drive);
        SSMR3PutU8(pSSM, d->dsk_chg);
        SSMR3PutU8(pSSM, d->perpendicular);
        SSMR3PutU8(pSSM, d->head);
        SSMR3PutU8(pSSM, d->track);
        SSMR3PutU8(pSSM, d->sect);
    }
    return TMR3TimerSave (pThis->result_timer, pSSM);
}


/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) fdcLoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    fdctrl_t *pThis = PDMINS_2_DATA(pDevIns, fdctrl_t *);
    unsigned int i;
    uint32_t val32;
    uint8_t val8;

    if (uVersion > FDC_SAVESTATE_CURRENT)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    /* The old saved state was significantly different. However, we can get
     * back most of the controller state and fix the rest by pretending the
     * disk in the drive (if any) has been replaced. At any rate there should
     * be no difficulty unless the state was saved during a floppy operation.
     */
    if (uVersion == FDC_SAVESTATE_OLD)
    {
        /* First verify a few assumptions. */
        AssertMsgReturn(sizeof(pThis->fifo) == FD_SECTOR_LEN,
                        ("The size of FIFO in saved state doesn't match!\n"),
                        VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
        AssertMsgReturn(RT_ELEMENTS(pThis->drives) == 2,
                        ("The number of drives in old saved state doesn't match!\n"),
                        VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
        /* Now load the old state. */
        SSMR3GetU8(pSSM, &pThis->version);
        /* Toss IRQ level, DMA channel, I/O base, and state. */
        SSMR3GetU8(pSSM, &val8);
        SSMR3GetU8(pSSM, &val8);
        SSMR3GetU32(pSSM, &val32);
        SSMR3GetU8(pSSM, &val8);
        /* Translate dma_en. */
        SSMR3GetU8(pSSM, &val8);
        if (val8)
            pThis->dor |= FD_DOR_DMAEN;
        SSMR3GetU8(pSSM, &pThis->cur_drv);
        /* Translate bootsel. */
        SSMR3GetU8(pSSM, &val8);
        pThis->tdr |= val8 << 2;
        SSMR3GetMem(pSSM, &pThis->fifo, FD_SECTOR_LEN);
        SSMR3GetU32(pSSM, &pThis->data_pos);
        SSMR3GetU32(pSSM, &pThis->data_len);
        SSMR3GetU8(pSSM, &pThis->data_state);
        SSMR3GetU8(pSSM, &pThis->data_dir);
        SSMR3GetU8(pSSM, &pThis->status0);
        SSMR3GetU8(pSSM, &pThis->eot);
        SSMR3GetU8(pSSM, &pThis->timer0);
        SSMR3GetU8(pSSM, &pThis->timer1);
        SSMR3GetU8(pSSM, &pThis->precomp_trk);
        SSMR3GetU8(pSSM, &pThis->config);
        SSMR3GetU8(pSSM, &pThis->lock);
        SSMR3GetU8(pSSM, &pThis->pwrd);

        for (i = 0; i < 2; ++i)
        {
            fdrive_t *d = &pThis->drives[i];

            SSMR3GetMem (pSSM, &d->Led, sizeof (d->Led));
            SSMR3GetU32(pSSM, &val32);
            d->drive = (fdrive_type_t)val32;
            SSMR3GetU32(pSSM, &val32);    /* Toss drflags */
            SSMR3GetU8(pSSM, &d->perpendicular);
            SSMR3GetU8(pSSM, &d->head);
            SSMR3GetU8(pSSM, &d->track);
            SSMR3GetU8(pSSM, &d->sect);
            SSMR3GetU8(pSSM, &val8);      /* Toss dir, rw */
            SSMR3GetU8(pSSM, &val8);
            SSMR3GetU32(pSSM, &val32);
            d->flags = (fdrive_flags_t)val32;
            SSMR3GetU8(pSSM, &d->last_sect);
            SSMR3GetU8(pSSM, &d->max_track);
            SSMR3GetU16(pSSM, &d->bps);
            SSMR3GetU8(pSSM, &d->ro);
        }
    }
    else    /* New state - straightforward. */
    {
        Assert(uVersion == FDC_SAVESTATE_CURRENT);
        /* Load the FDC I/O registers... */
        SSMR3GetU8(pSSM, &pThis->sra);
        SSMR3GetU8(pSSM, &pThis->srb);
        SSMR3GetU8(pSSM, &pThis->dor);
        SSMR3GetU8(pSSM, &pThis->tdr);
        SSMR3GetU8(pSSM, &pThis->dsr);
        SSMR3GetU8(pSSM, &pThis->msr);
        /* ...the status registers... */
        SSMR3GetU8(pSSM, &pThis->status0);
        SSMR3GetU8(pSSM, &pThis->status1);
        SSMR3GetU8(pSSM, &pThis->status2);
        /* ...the command FIFO, if the size matches... */
        SSMR3GetU32(pSSM, &val32);
        AssertMsgReturn(sizeof(pThis->fifo) == val32,
                        ("The size of FIFO in saved state doesn't match!\n"),
                        VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
        SSMR3GetMem(pSSM, &pThis->fifo, sizeof(pThis->fifo));
        SSMR3GetU32(pSSM, &pThis->data_pos);
        SSMR3GetU32(pSSM, &pThis->data_len);
        SSMR3GetU8(pSSM, &pThis->data_state);
        SSMR3GetU8(pSSM, &pThis->data_dir);
        /* ...and miscellaneous internal FDC state. */
        SSMR3GetU8(pSSM, &pThis->reset_sensei);
        SSMR3GetU8(pSSM, &pThis->eot);
        SSMR3GetU8(pSSM, &pThis->timer0);
        SSMR3GetU8(pSSM, &pThis->timer1);
        SSMR3GetU8(pSSM, &pThis->precomp_trk);
        SSMR3GetU8(pSSM, &pThis->config);
        SSMR3GetU8(pSSM, &pThis->lock);
        SSMR3GetU8(pSSM, &pThis->pwrd);
        SSMR3GetU8(pSSM, &pThis->version);

        /* Validate the number of drives. */
        SSMR3GetU8(pSSM, &pThis->num_floppies);
        AssertMsgReturn(RT_ELEMENTS(pThis->drives) == pThis->num_floppies,
                        ("The number of drives in saved state doesn't match!\n"),
                        VERR_SSM_DATA_UNIT_FORMAT_CHANGED);

        /* Load the per-drive state. */
        for (i = 0; i < pThis->num_floppies; ++i)
        {
            fdrive_t *d = &pThis->drives[i];

            SSMR3GetMem(pSSM, &d->Led, sizeof(d->Led));
            SSMR3GetU32(pSSM, &val32);
            d->drive = (fdrive_type_t)val32;
            SSMR3GetU8(pSSM, &d->dsk_chg);
            SSMR3GetU8(pSSM, &d->perpendicular);
            SSMR3GetU8(pSSM, &d->head);
            SSMR3GetU8(pSSM, &d->track);
            SSMR3GetU8(pSSM, &d->sect);
        }
    }
    return TMR3TimerLoad (pThis->result_timer, pSSM);
}


/* -=-=-=-=-=-=-=-=- Drive level interfaces -=-=-=-=-=-=-=-=- */

/**
 * @interface_method_impl{PDMIMOUNTNOTIFY,pfnMountNotify}
 */
static DECLCALLBACK(void) fdMountNotify(PPDMIMOUNTNOTIFY pInterface)
{
    fdrive_t *pDrv = RT_FROM_MEMBER(pInterface, fdrive_t, IMountNotify);
    LogFlow(("fdMountNotify:\n"));
    fd_revalidate(pDrv);
}


/**
 * @interface_method_impl{PDMIMOUNTNOTIFY,pfnUnmountNotify}
 */
static DECLCALLBACK(void) fdUnmountNotify(PPDMIMOUNTNOTIFY pInterface)
{
    fdrive_t *pDrv = RT_FROM_MEMBER(pInterface, fdrive_t, IMountNotify);
    LogFlow(("fdUnmountNotify:\n"));
    fd_revalidate(pDrv);
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) fdQueryInterface (PPDMIBASE pInterface, const char *pszIID)
{
    fdrive_t *pDrv = RT_FROM_MEMBER(pInterface, fdrive_t, IBase);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrv->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIAPORT, &pDrv->IPort);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMOUNTNOTIFY, &pDrv->IMountNotify);
    return NULL;
}


/**
 * @interface_method_impl{PDMIMEDIAPORT,pfnQueryDeviceLocation}
 */
static DECLCALLBACK(int) fdQueryDeviceLocation(PPDMIMEDIAPORT pInterface, const char **ppcszController,
                                               uint32_t *piInstance, uint32_t *piLUN)
{
    fdrive_t *pDrv = RT_FROM_MEMBER(pInterface, fdrive_t, IPort);
    PPDMDEVINS pDevIns = pDrv->pDevIns;

    AssertPtrReturn(ppcszController, VERR_INVALID_POINTER);
    AssertPtrReturn(piInstance, VERR_INVALID_POINTER);
    AssertPtrReturn(piLUN, VERR_INVALID_POINTER);

    *ppcszController = pDevIns->pReg->szName;
    *piInstance = pDevIns->iInstance;
    *piLUN = pDrv->iLUN;

    return VINF_SUCCESS;
}

/* -=-=-=-=-=-=-=-=- Controller level interfaces -=-=-=-=-=-=-=-=- */

/**
 * @interface_method_impl{PDMILEDPORTS,pfnQueryStatusLed}
 */
static DECLCALLBACK(int) fdcStatusQueryStatusLed(PPDMILEDPORTS pInterface, unsigned iLUN, PPDMLED *ppLed)
{
    fdctrl_t *pThis = RT_FROM_MEMBER (pInterface, fdctrl_t, ILeds);
    if (iLUN < RT_ELEMENTS(pThis->drives)) {
        *ppLed = &pThis->drives[iLUN].Led;
        Assert ((*ppLed)->u32Magic == PDMLED_MAGIC);
        return VINF_SUCCESS;
    }
    return VERR_PDM_LUN_NOT_FOUND;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) fdcStatusQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    fdctrl_t *pThis = RT_FROM_MEMBER (pInterface, fdctrl_t, IBaseStatus);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBaseStatus);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMILEDPORTS, &pThis->ILeds);
    return NULL;
}


/**
 * Configure a drive.
 *
 * @returns VBox status code.
 * @param   drv         The drive in question.
 * @param   pDevIns     The driver instance.
 * @param   fInit       Set if we're at init time and can change the drive type.
 */
static int fdConfig(fdrive_t *drv, PPDMDEVINS pDevIns, bool fInit)
{
    static const char * const s_apszDesc[] = {"Floppy Drive A:", "Floppy Drive B"};
    int rc;

    /*
     * Reset the LED just to be on the safe side.
     */
    Assert (RT_ELEMENTS(s_apszDesc) > drv->iLUN);
    Assert (drv->Led.u32Magic == PDMLED_MAGIC);
    drv->Led.Actual.u32 = 0;
    drv->Led.Asserted.u32 = 0;

    /*
     * Try attach the block device and get the interfaces.
     */
    rc = PDMDevHlpDriverAttach (pDevIns, drv->iLUN, &drv->IBase, &drv->pDrvBase, s_apszDesc[drv->iLUN]);
    if (RT_SUCCESS (rc)) {
        drv->pDrvMedia = PDMIBASE_QUERY_INTERFACE(drv->pDrvBase, PDMIMEDIA);
        if (drv->pDrvMedia) {
            drv->pDrvMount = PDMIBASE_QUERY_INTERFACE(drv->pDrvBase, PDMIMOUNT);
            if (drv->pDrvMount) {
                fd_init(drv, fInit);
            } else {
                AssertMsgFailed (("Configuration error: LUN#%d without mountable interface!\n", drv->iLUN));
                rc = VERR_PDM_MISSING_INTERFACE;
            }

        } else {
            AssertMsgFailed (("Configuration error: LUN#%d hasn't a block interface!\n", drv->iLUN));
            rc = VERR_PDM_MISSING_INTERFACE;
        }
    } else {
        AssertMsg (rc == VERR_PDM_NO_ATTACHED_DRIVER,
                   ("Failed to attach LUN#%d. rc=%Rrc\n", drv->iLUN, rc));
        switch (rc) {
        case VERR_ACCESS_DENIED:
            /* Error already cached by DrvHostBase */
            break;
        case VERR_PDM_NO_ATTACHED_DRIVER:
            /* Legal on architectures without a floppy controller */
            break;
        default:
            rc = PDMDevHlpVMSetError (pDevIns, rc, RT_SRC_POS,
                                      N_ ("The floppy controller cannot attach to the floppy drive"));
            break;
        }
    }

    if (RT_FAILURE (rc)) {
        drv->pDrvBase = NULL;
        drv->pDrvMedia = NULL;
        drv->pDrvMount = NULL;
    }
    LogFlow (("fdConfig: returns %Rrc\n", rc));
    return rc;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnAttach}
 *
 * This is called when we change block driver for a floppy drive.
 */
static DECLCALLBACK(int)  fdcAttach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    fdctrl_t *fdctrl = PDMINS_2_DATA(pDevIns, fdctrl_t *);
    fdrive_t *drv;
    int rc;
    LogFlow (("ideDetach: iLUN=%u\n", iLUN));

    AssertMsgReturn(fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG,
                    ("The FDC device does not support hotplugging\n"),
                    VERR_INVALID_PARAMETER);

    /*
     * Validate.
     */
    if (iLUN >= 2) {
        AssertMsgFailed (("Configuration error: cannot attach or detach any but the first two LUNs - iLUN=%u\n",
                          iLUN));
        return VERR_PDM_DEVINS_NO_ATTACH;
    }

    /*
     * Locate the drive and stuff.
     */
    drv = &fdctrl->drives[iLUN];

    /* the usual paranoia */
    AssertRelease (!drv->pDrvBase);
    AssertRelease (!drv->pDrvMedia);
    AssertRelease (!drv->pDrvMount);

    rc = fdConfig (drv, pDevIns, false /*fInit*/);
    AssertMsg (rc != VERR_PDM_NO_ATTACHED_DRIVER,
               ("Configuration error: failed to configure drive %d, rc=%Rrc\n", iLUN, rc));
    if (RT_SUCCESS(rc)) {
        fd_revalidate (drv);
    }

    LogFlow (("floppyAttach: returns %Rrc\n", rc));
    return rc;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDetach}
 *
 * The floppy drive has been temporarily 'unplugged'.
 */
static DECLCALLBACK(void) fdcDetach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    fdctrl_t *pThis = PDMINS_2_DATA(pDevIns, fdctrl_t *);
    LogFlow (("ideDetach: iLUN=%u\n", iLUN));

    switch (iLUN)
    {
        case 0:
        case 1:
        {
            fdrive_t *drv = &pThis->drives[iLUN];
            drv->pDrvBase = NULL;
            drv->pDrvMedia = NULL;
            drv->pDrvMount = NULL;
            break;
        }

        default:
            AssertMsgFailed(("Cannot detach LUN#%d!\n", iLUN));
            break;
    }
}


/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 *
 * I haven't check the specs on what's supposed to happen on reset, but we
 * should get any 'FATAL: floppy recal:f07 ctrl not ready' when resetting
 * at wrong time like we do if this was all void.
 */
static DECLCALLBACK(void) fdcReset(PPDMDEVINS pDevIns)
{
    fdctrl_t *pThis = PDMINS_2_DATA (pDevIns, fdctrl_t *);
    unsigned i;
    LogFlow (("fdcReset:\n"));

    fdctrl_reset(pThis, 0);

    for (i = 0; i < RT_ELEMENTS(pThis->drives); i++)
        fd_revalidate(&pThis->drives[i]);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) fdcConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF(iInstance);
    fdctrl_t      *pThis = PDMINS_2_DATA(pDevIns, fdctrl_t *);
    int            rc;
    unsigned       i, j;
    int            ii;
    bool           mem_mapped;
    uint16_t       io_base;
    uint8_t        irq_lvl, dma_chann;
    PPDMIBASE      pBase;

    Assert(iInstance == 0);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Validate configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "IRQ\0DMA\0MemMapped\0IOBase\0"))
        return VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;

    /*
     * Read the configuration.
     */
    rc = CFGMR3QueryU8Def(pCfg, "IRQ", &irq_lvl, 6);
    AssertMsgRCReturn(rc, ("Configuration error: Failed to read U8 IRQ, rc=%Rrc\n", rc), rc);

    rc = CFGMR3QueryU8Def(pCfg, "DMA", &dma_chann, 2);
    AssertMsgRCReturn(rc, ("Configuration error: Failed to read U8 DMA, rc=%Rrc\n", rc), rc);

    rc = CFGMR3QueryU16Def(pCfg, "IOBase", &io_base, 0x3f0);
    AssertMsgRCReturn(rc, ("Configuration error: Failed to read U16 IOBase, rc=%Rrc\n", rc), rc);

    rc = CFGMR3QueryBoolDef(pCfg, "MemMapped", &mem_mapped, false);
    AssertMsgRCReturn(rc, ("Configuration error: Failed to read bool value MemMapped rc=%Rrc\n", rc), rc);

    /*
     * Initialize data.
     */
    LogFlow(("fdcConstruct: irq_lvl=%d dma_chann=%d io_base=%#x\n", irq_lvl, dma_chann, io_base));
    pThis->pDevIns   = pDevIns;
    pThis->version   = 0x90;   /* Intel 82078 controller */
    pThis->irq_lvl   = irq_lvl;
    pThis->dma_chann = dma_chann;
    pThis->io_base   = io_base;
    pThis->config    = FD_CONFIG_EIS | FD_CONFIG_EFIFO; /* Implicit seek, polling & FIFO enabled */
    pThis->num_floppies = MAX_FD;

    /* Fill 'command_to_handler' lookup table */
    for (ii = RT_ELEMENTS(handlers) - 1; ii >= 0; ii--)
        for (j = 0; j < sizeof(command_to_handler); j++)
            if ((j & handlers[ii].mask) == handlers[ii].value)
                command_to_handler[j] = ii;

    pThis->IBaseStatus.pfnQueryInterface = fdcStatusQueryInterface;
    pThis->ILeds.pfnQueryStatusLed       = fdcStatusQueryStatusLed;

    for (i = 0; i < RT_ELEMENTS(pThis->drives); ++i)
    {
        fdrive_t *pDrv = &pThis->drives[i];

        pDrv->drive   = FDRIVE_DRV_NONE;
        pDrv->iLUN    = i;
        pDrv->pDevIns = pDevIns;

        pDrv->IBase.pfnQueryInterface       = fdQueryInterface;
        pDrv->IMountNotify.pfnMountNotify   = fdMountNotify;
        pDrv->IMountNotify.pfnUnmountNotify = fdUnmountNotify;
        pDrv->IPort.pfnQueryDeviceLocation  = fdQueryDeviceLocation;
        pDrv->Led.u32Magic = PDMLED_MAGIC;
    }

    /*
     * Create the FDC timer.
     */
    rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, fdcTimerCallback, pThis,
                                TMTIMER_FLAGS_DEFAULT_CRIT_SECT, "FDC Timer", &pThis->result_timer);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Register DMA channel.
     */
    if (pThis->dma_chann != 0xff)
    {
        rc = PDMDevHlpDMARegister(pDevIns, dma_chann, &fdctrl_transfer_handler, pThis);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * IO / MMIO.
     */
    if (mem_mapped)
    {
        AssertMsgFailed(("Memory mapped floppy not support by now\n"));
        return VERR_NOT_SUPPORTED;
#if 0
        FLOPPY_ERROR("memory mapped floppy not supported by now !\n");
        io_mem = cpu_register_io_memory(0, fdctrl_mem_read, fdctrl_mem_write);
        cpu_register_physical_memory(base, 0x08, io_mem);
#endif
    }
    else
    {
        rc = PDMDevHlpIOPortRegister(pDevIns, io_base + 0x1, 5, pThis,
                                     fdcIoPortWrite, fdcIoPortRead, NULL, NULL, "FDC#1");
        if (RT_FAILURE(rc))
            return rc;

        rc = PDMDevHlpIOPortRegister(pDevIns, io_base + 0x7, 1, pThis,
                                     fdcIoPortWrite, fdcIoPortRead, NULL, NULL, "FDC#2");
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Register the saved state data unit.
     */
    rc = PDMDevHlpSSMRegister(pDevIns, FDC_SAVESTATE_CURRENT, sizeof(*pThis), fdcSaveExec, fdcLoadExec);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Attach the status port (optional).
     */
    rc = PDMDevHlpDriverAttach(pDevIns, PDM_STATUS_LUN, &pThis->IBaseStatus, &pBase, "Status Port");
    if (RT_SUCCESS (rc))
        pThis->pLedsConnector = PDMIBASE_QUERY_INTERFACE(pBase, PDMILEDCONNECTORS);
    else if (rc != VERR_PDM_NO_ATTACHED_DRIVER)
    {
        AssertMsgFailed(("Failed to attach to status driver. rc=%Rrc\n", rc));
        return rc;
    }

    /*
     * Initialize drives.
     */
    for (i = 0; i < RT_ELEMENTS(pThis->drives); i++)
    {
        fdrive_t *pDrv = &pThis->drives[i];
        rc = fdConfig(pDrv, pDevIns, true /*fInit*/);
        if (   RT_FAILURE(rc)
            && rc != VERR_PDM_NO_ATTACHED_DRIVER)
        {
            AssertMsgFailed(("Configuration error: failed to configure drive %d, rc=%Rrc\n", i, rc));
            return rc;
        }
    }

    fdctrl_reset(pThis, 0);

    for (i = 0; i < RT_ELEMENTS(pThis->drives); i++)
        fd_revalidate(&pThis->drives[i]);

    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceFloppyController =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "i82078",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Floppy drive controller (Intel 82078)",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS,
    /* fClass */
    PDM_DEVREG_CLASS_STORAGE,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(fdctrl_t),
    /* pfnConstruct */
    fdcConstruct,
    /* pfnDestruct */
    NULL,
    /* pfnRelocate */
    NULL,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    fdcReset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    fdcAttach,
    /* pfnDetach */
    fdcDetach,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};

#endif /* VBOX */

/*
 * Local Variables:
 *  mode: c
 *  c-file-style: "k&r"
 *  indent-tabs-mode: nil
 * End:
 */

