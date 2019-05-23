/** @file
 * VBoxDisplay - private windows additions display header
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
 */

#ifndef ___winnt_include_VBoxDisplay_h___
#define ___winnt_include_VBoxDisplay_h___

#include <iprt/types.h>
#include <iprt/assert.h>

#define VBOXESC_SETVISIBLEREGION            0xABCD9001
#define VBOXESC_ISVRDPACTIVE                0xABCD9002
#ifdef VBOX_WITH_WDDM
# define VBOXESC_REINITVIDEOMODES           0xABCD9003
# define VBOXESC_GETVBOXVIDEOCMCMD          0xABCD9004
# define VBOXESC_DBGPRINT                   0xABCD9005
# define VBOXESC_SCREENLAYOUT               0xABCD9006
# define VBOXESC_SWAPCHAININFO              0xABCD9007
# define VBOXESC_UHGSMI_ALLOCATE            0xABCD9008
# define VBOXESC_UHGSMI_DEALLOCATE          0xABCD9009
# define VBOXESC_UHGSMI_SUBMIT              0xABCD900A
# define VBOXESC_SHRC_ADDREF                0xABCD900B
# define VBOXESC_SHRC_RELEASE               0xABCD900C
# define VBOXESC_DBGDUMPBUF                 0xABCD900D
# define VBOXESC_CRHGSMICTLCON_CALL         0xABCD900E
# define VBOXESC_CRHGSMICTLCON_GETCLIENTID  0xABCD900F
# define VBOXESC_REINITVIDEOMODESBYMASK     0xABCD9010
# define VBOXESC_ADJUSTVIDEOMODES           0xABCD9011
# define VBOXESC_SETCTXHOSTID               0xABCD9012
# define VBOXESC_CONFIGURETARGETS           0xABCD9013
# define VBOXESC_SETALLOCHOSTID             0xABCD9014
# define VBOXESC_CRHGSMICTLCON_GETHOSTCAPS  0xABCD9015
# define VBOXESC_UPDATEMODES                0xABCD9016
# define VBOXESC_GUEST_DISPLAYCHANGED       0xABCD9017
# define VBOXESC_TARGET_CONNECTIVITY        0xABCD9018
#endif /* #ifdef VBOX_WITH_WDDM */

# define VBOXESC_ISANYX                     0xABCD9200

typedef struct VBOXDISPIFESCAPE
{
    int32_t escapeCode;
    uint32_t u32CmdSpecific;
} VBOXDISPIFESCAPE, *PVBOXDISPIFESCAPE;

/* ensure command body is always 8-byte-aligned*/
AssertCompile((sizeof (VBOXDISPIFESCAPE) & 7) == 0);

#define VBOXDISPIFESCAPE_DATA_OFFSET() ((sizeof (VBOXDISPIFESCAPE) + 7) & ~7)
#define VBOXDISPIFESCAPE_DATA(_pHead, _t) ( (_t*)(((uint8_t*)(_pHead)) + VBOXDISPIFESCAPE_DATA_OFFSET()))
#define VBOXDISPIFESCAPE_DATA_SIZE(_s) ( (_s) < VBOXDISPIFESCAPE_DATA_OFFSET() ? 0 : (_s) - VBOXDISPIFESCAPE_DATA_OFFSET() )
#define VBOXDISPIFESCAPE_SIZE(_cbData) ((_cbData) ? VBOXDISPIFESCAPE_DATA_OFFSET() + (_cbData) : sizeof (VBOXDISPIFESCAPE))

#define IOCTL_VIDEO_VBOX_SETVISIBLEREGION \
    CTL_CODE(FILE_DEVICE_VIDEO, 0xA01, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_VBOX_ISANYX \
    CTL_CODE(FILE_DEVICE_VIDEO, 0xA02, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct VBOXDISPIFESCAPE_ISANYX
{
    VBOXDISPIFESCAPE EscapeHdr;
    uint32_t u32IsAnyX;
} VBOXDISPIFESCAPE_ISANYX, *PVBOXDISPIFESCAPE_ISANYX;

#ifdef VBOX_WITH_WDDM

/* Enables code which performs (un)plugging of virtual displays in VBOXESC_UPDATEMODES.
 * The code has been disabled as part of #8244.
 */
//#define VBOX_WDDM_REPLUG_ON_MODE_CHANGE

/* for VBOX_VIDEO_MAX_SCREENS definition */
#include <VBoxVideo.h>

typedef struct VBOXWDDM_RECOMMENDVIDPN_SOURCE
{
    RTRECTSIZE Size;
} VBOXWDDM_RECOMMENDVIDPN_SOURCE;

typedef struct VBOXWDDM_RECOMMENDVIDPN_TARGET
{
    int32_t iSource;
} VBOXWDDM_RECOMMENDVIDPN_TARGET;

typedef struct
{
    VBOXWDDM_RECOMMENDVIDPN_SOURCE aSources[VBOX_VIDEO_MAX_SCREENS];
    VBOXWDDM_RECOMMENDVIDPN_TARGET aTargets[VBOX_VIDEO_MAX_SCREENS];
} VBOXWDDM_RECOMMENDVIDPN, *PVBOXWDDM_RECOMMENDVIDPN;

#define VBOXWDDM_SCREENMASK_SIZE ((VBOX_VIDEO_MAX_SCREENS + 7) >> 3)

typedef struct VBOXDISPIFESCAPE_UPDATEMODES
{
    VBOXDISPIFESCAPE EscapeHdr;
    uint32_t u32TargetId;
    RTRECTSIZE Size;
} VBOXDISPIFESCAPE_UPDATEMODES;

typedef struct VBOXDISPIFESCAPE_TARGETCONNECTIVITY
{
    VBOXDISPIFESCAPE EscapeHdr;
    uint32_t u32TargetId;
    uint32_t fu32Connect;
} VBOXDISPIFESCAPE_TARGETCONNECTIVITY;

#endif /* VBOX_WITH_WDDM */

#endif

