/* $Id: VBoxManageModifyVM.cpp $ */
/** @file
 * VBoxManage - Implementation of modifyvm command.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifndef VBOX_ONLY_DOCS
#include <VBox/com/com.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/VirtualBox.h>
#endif /* !VBOX_ONLY_DOCS */

#include <iprt/cidr.h>
#include <iprt/ctype.h>
#include <iprt/file.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/getopt.h>
#include <VBox/log.h>
#include "VBoxManage.h"

#ifndef VBOX_ONLY_DOCS
using namespace com;
/** @todo refine this after HDD changes; MSC 8.0/64 has trouble with handleModifyVM.  */
#if defined(_MSC_VER)
# pragma optimize("g", off)
# if _MSC_VER < RT_MSC_VER_VC120
#  pragma warning(disable:4748)
# endif
#endif

enum
{
    MODIFYVM_NAME = 1000,
    MODIFYVM_GROUPS,
    MODIFYVM_DESCRIPTION,
    MODIFYVM_OSTYPE,
    MODIFYVM_ICONFILE,
    MODIFYVM_MEMORY,
    MODIFYVM_PAGEFUSION,
    MODIFYVM_VRAM,
    MODIFYVM_FIRMWARE,
    MODIFYVM_ACPI,
    MODIFYVM_IOAPIC,
    MODIFYVM_PAE,
    MODIFYVM_LONGMODE,
    MODIFYVM_CPUID_PORTABILITY,
    MODIFYVM_TFRESET,
    MODIFYVM_APIC,
    MODIFYVM_X2APIC,
    MODIFYVM_PARAVIRTPROVIDER,
    MODIFYVM_PARAVIRTDEBUG,
    MODIFYVM_HWVIRTEX,
    MODIFYVM_NESTEDPAGING,
    MODIFYVM_LARGEPAGES,
    MODIFYVM_VTXVPID,
    MODIFYVM_VTXUX,
    MODIFYVM_CPUS,
    MODIFYVM_CPUHOTPLUG,
    MODIFYVM_CPU_PROFILE,
    MODIFYVM_PLUGCPU,
    MODIFYVM_UNPLUGCPU,
    MODIFYVM_SETCPUID,
    MODIFYVM_SETCPUID_OLD,
    MODIFYVM_DELCPUID,
    MODIFYVM_DELCPUID_OLD,
    MODIFYVM_DELALLCPUID,
    MODIFYVM_GRAPHICSCONTROLLER,
    MODIFYVM_MONITORCOUNT,
    MODIFYVM_ACCELERATE3D,
#ifdef VBOX_WITH_VIDEOHWACCEL
    MODIFYVM_ACCELERATE2DVIDEO,
#endif
    MODIFYVM_BIOSLOGOFADEIN,
    MODIFYVM_BIOSLOGOFADEOUT,
    MODIFYVM_BIOSLOGODISPLAYTIME,
    MODIFYVM_BIOSLOGOIMAGEPATH,
    MODIFYVM_BIOSBOOTMENU,
    MODIFYVM_BIOSAPIC,
    MODIFYVM_BIOSSYSTEMTIMEOFFSET,
    MODIFYVM_BIOSPXEDEBUG,
    MODIFYVM_BOOT,
    MODIFYVM_HDA,                // deprecated
    MODIFYVM_HDB,                // deprecated
    MODIFYVM_HDD,                // deprecated
    MODIFYVM_IDECONTROLLER,      // deprecated
    MODIFYVM_SATAPORTCOUNT,      // deprecated
    MODIFYVM_SATAPORT,           // deprecated
    MODIFYVM_SATA,               // deprecated
    MODIFYVM_SCSIPORT,           // deprecated
    MODIFYVM_SCSITYPE,           // deprecated
    MODIFYVM_SCSI,               // deprecated
    MODIFYVM_DVDPASSTHROUGH,     // deprecated
    MODIFYVM_DVD,                // deprecated
    MODIFYVM_FLOPPY,             // deprecated
    MODIFYVM_NICTRACEFILE,
    MODIFYVM_NICTRACE,
    MODIFYVM_NICPROPERTY,
    MODIFYVM_NICTYPE,
    MODIFYVM_NICSPEED,
    MODIFYVM_NICBOOTPRIO,
    MODIFYVM_NICPROMISC,
    MODIFYVM_NICBWGROUP,
    MODIFYVM_NIC,
    MODIFYVM_CABLECONNECTED,
    MODIFYVM_BRIDGEADAPTER,
    MODIFYVM_HOSTONLYADAPTER,
    MODIFYVM_INTNET,
    MODIFYVM_GENERICDRV,
    MODIFYVM_NATNETWORKNAME,
    MODIFYVM_NATNET,
    MODIFYVM_NATBINDIP,
    MODIFYVM_NATSETTINGS,
    MODIFYVM_NATPF,
    MODIFYVM_NATALIASMODE,
    MODIFYVM_NATTFTPPREFIX,
    MODIFYVM_NATTFTPFILE,
    MODIFYVM_NATTFTPSERVER,
    MODIFYVM_NATDNSPASSDOMAIN,
    MODIFYVM_NATDNSPROXY,
    MODIFYVM_NATDNSHOSTRESOLVER,
    MODIFYVM_MACADDRESS,
    MODIFYVM_HIDPTR,
    MODIFYVM_HIDKBD,
    MODIFYVM_UARTMODE,
    MODIFYVM_UART,
#if defined(RT_OS_LINUX) || defined(RT_OS_WINDOWS)
    MODIFYVM_LPTMODE,
    MODIFYVM_LPT,
#endif
    MODIFYVM_GUESTMEMORYBALLOON,
    MODIFYVM_AUDIOCONTROLLER,
    MODIFYVM_AUDIOCODEC,
    MODIFYVM_AUDIO,
    MODIFYVM_AUDIOIN,
    MODIFYVM_AUDIOOUT,
    MODIFYVM_CLIPBOARD,
    MODIFYVM_DRAGANDDROP,
    MODIFYVM_VRDPPORT,                /* VRDE: deprecated */
    MODIFYVM_VRDPADDRESS,             /* VRDE: deprecated */
    MODIFYVM_VRDPAUTHTYPE,            /* VRDE: deprecated */
    MODIFYVM_VRDPMULTICON,            /* VRDE: deprecated */
    MODIFYVM_VRDPREUSECON,            /* VRDE: deprecated */
    MODIFYVM_VRDPVIDEOCHANNEL,        /* VRDE: deprecated */
    MODIFYVM_VRDPVIDEOCHANNELQUALITY, /* VRDE: deprecated */
    MODIFYVM_VRDP,                    /* VRDE: deprecated */
    MODIFYVM_VRDEPROPERTY,
    MODIFYVM_VRDEPORT,
    MODIFYVM_VRDEADDRESS,
    MODIFYVM_VRDEAUTHTYPE,
    MODIFYVM_VRDEAUTHLIBRARY,
    MODIFYVM_VRDEMULTICON,
    MODIFYVM_VRDEREUSECON,
    MODIFYVM_VRDEVIDEOCHANNEL,
    MODIFYVM_VRDEVIDEOCHANNELQUALITY,
    MODIFYVM_VRDE_EXTPACK,
    MODIFYVM_VRDE,
    MODIFYVM_RTCUSEUTC,
    MODIFYVM_USBRENAME,
    MODIFYVM_USBXHCI,
    MODIFYVM_USBEHCI,
    MODIFYVM_USB,
    MODIFYVM_SNAPSHOTFOLDER,
    MODIFYVM_TELEPORTER_ENABLED,
    MODIFYVM_TELEPORTER_PORT,
    MODIFYVM_TELEPORTER_ADDRESS,
    MODIFYVM_TELEPORTER_PASSWORD,
    MODIFYVM_TELEPORTER_PASSWORD_FILE,
    MODIFYVM_TRACING_ENABLED,
    MODIFYVM_TRACING_CONFIG,
    MODIFYVM_TRACING_ALLOW_VM_ACCESS,
    MODIFYVM_HARDWARE_UUID,
    MODIFYVM_HPET,
    MODIFYVM_IOCACHE,
    MODIFYVM_IOCACHESIZE,
    MODIFYVM_FAULT_TOLERANCE,
    MODIFYVM_FAULT_TOLERANCE_ADDRESS,
    MODIFYVM_FAULT_TOLERANCE_PORT,
    MODIFYVM_FAULT_TOLERANCE_PASSWORD,
    MODIFYVM_FAULT_TOLERANCE_SYNC_INTERVAL,
    MODIFYVM_CPU_EXECTUION_CAP,
    MODIFYVM_AUTOSTART_ENABLED,
    MODIFYVM_AUTOSTART_DELAY,
    MODIFYVM_AUTOSTOP_TYPE,
#ifdef VBOX_WITH_PCI_PASSTHROUGH
    MODIFYVM_ATTACH_PCI,
    MODIFYVM_DETACH_PCI,
#endif
#ifdef VBOX_WITH_USB_CARDREADER
    MODIFYVM_USBCARDREADER,
#endif
#ifdef VBOX_WITH_VIDEOREC
    MODIFYVM_VIDEOCAP,
    MODIFYVM_VIDEOCAP_SCREENS,
    MODIFYVM_VIDEOCAP_FILENAME,
    MODIFYVM_VIDEOCAP_WIDTH,
    MODIFYVM_VIDEOCAP_HEIGHT,
    MODIFYVM_VIDEOCAP_RES,
    MODIFYVM_VIDEOCAP_RATE,
    MODIFYVM_VIDEOCAP_FPS,
    MODIFYVM_VIDEOCAP_MAXTIME,
    MODIFYVM_VIDEOCAP_MAXSIZE,
    MODIFYVM_VIDEOCAP_OPTIONS,
#endif
    MODIFYVM_CHIPSET,
    MODIFYVM_DEFAULTFRONTEND
};

static const RTGETOPTDEF g_aModifyVMOptions[] =
{
/** @todo Convert to dash separated names like --triple-fault-reset! Please
 *        do that for all new options as we don't need more character soups
 *        around VirtualBox - typedefs more than covers that demand! */
    { "--name",                     MODIFYVM_NAME,                      RTGETOPT_REQ_STRING },
    { "--groups",                   MODIFYVM_GROUPS,                    RTGETOPT_REQ_STRING },
    { "--description",              MODIFYVM_DESCRIPTION,               RTGETOPT_REQ_STRING },
    { "--ostype",                   MODIFYVM_OSTYPE,                    RTGETOPT_REQ_STRING },
    { "--iconfile",                 MODIFYVM_ICONFILE,                  RTGETOPT_REQ_STRING },
    { "--memory",                   MODIFYVM_MEMORY,                    RTGETOPT_REQ_UINT32 },
    { "--pagefusion",               MODIFYVM_PAGEFUSION,                RTGETOPT_REQ_BOOL_ONOFF },
    { "--vram",                     MODIFYVM_VRAM,                      RTGETOPT_REQ_UINT32 },
    { "--firmware",                 MODIFYVM_FIRMWARE,                  RTGETOPT_REQ_STRING },
    { "--acpi",                     MODIFYVM_ACPI,                      RTGETOPT_REQ_BOOL_ONOFF },
    { "--ioapic",                   MODIFYVM_IOAPIC,                    RTGETOPT_REQ_BOOL_ONOFF },
    { "--pae",                      MODIFYVM_PAE,                       RTGETOPT_REQ_BOOL_ONOFF },
    { "--longmode",                 MODIFYVM_LONGMODE,                  RTGETOPT_REQ_BOOL_ONOFF },
    { "--cpuid-portability-level",  MODIFYVM_CPUID_PORTABILITY,         RTGETOPT_REQ_UINT32 },
    { "--triplefaultreset",         MODIFYVM_TFRESET,                   RTGETOPT_REQ_BOOL_ONOFF },
    { "--apic",                     MODIFYVM_APIC,                      RTGETOPT_REQ_BOOL_ONOFF },
    { "--x2apic",                   MODIFYVM_X2APIC,                    RTGETOPT_REQ_BOOL_ONOFF },
    { "--paravirtprovider",         MODIFYVM_PARAVIRTPROVIDER,          RTGETOPT_REQ_STRING },
    { "--paravirtdebug",            MODIFYVM_PARAVIRTDEBUG,             RTGETOPT_REQ_STRING },
    { "--hwvirtex",                 MODIFYVM_HWVIRTEX,                  RTGETOPT_REQ_BOOL_ONOFF },
    { "--nestedpaging",             MODIFYVM_NESTEDPAGING,              RTGETOPT_REQ_BOOL_ONOFF },
    { "--largepages",               MODIFYVM_LARGEPAGES,                RTGETOPT_REQ_BOOL_ONOFF },
    { "--vtxvpid",                  MODIFYVM_VTXVPID,                   RTGETOPT_REQ_BOOL_ONOFF },
    { "--vtxux",                    MODIFYVM_VTXUX,                     RTGETOPT_REQ_BOOL_ONOFF },
    { "--cpuid-set",                MODIFYVM_SETCPUID,                  RTGETOPT_REQ_UINT32_OPTIONAL_PAIR | RTGETOPT_FLAG_HEX },
    { "--cpuid-remove",             MODIFYVM_DELCPUID,                  RTGETOPT_REQ_UINT32_OPTIONAL_PAIR | RTGETOPT_FLAG_HEX },
    { "--cpuidset",                 MODIFYVM_SETCPUID_OLD,              RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_HEX },
    { "--cpuidremove",              MODIFYVM_DELCPUID_OLD,              RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_HEX },
    { "--cpuidremoveall",           MODIFYVM_DELALLCPUID,               RTGETOPT_REQ_NOTHING},
    { "--cpus",                     MODIFYVM_CPUS,                      RTGETOPT_REQ_UINT32 },
    { "--cpuhotplug",               MODIFYVM_CPUHOTPLUG,                RTGETOPT_REQ_BOOL_ONOFF },
    { "--cpu-profile",              MODIFYVM_CPU_PROFILE,               RTGETOPT_REQ_STRING },
    { "--plugcpu",                  MODIFYVM_PLUGCPU,                   RTGETOPT_REQ_UINT32 },
    { "--unplugcpu",                MODIFYVM_UNPLUGCPU,                 RTGETOPT_REQ_UINT32 },
    { "--cpuexecutioncap",          MODIFYVM_CPU_EXECTUION_CAP,         RTGETOPT_REQ_UINT32 },
    { "--rtcuseutc",                MODIFYVM_RTCUSEUTC,                 RTGETOPT_REQ_BOOL_ONOFF },
    { "--graphicscontroller",       MODIFYVM_GRAPHICSCONTROLLER,        RTGETOPT_REQ_STRING },
    { "--monitorcount",             MODIFYVM_MONITORCOUNT,              RTGETOPT_REQ_UINT32 },
    { "--accelerate3d",             MODIFYVM_ACCELERATE3D,              RTGETOPT_REQ_BOOL_ONOFF },
#ifdef VBOX_WITH_VIDEOHWACCEL
    { "--accelerate2dvideo",        MODIFYVM_ACCELERATE2DVIDEO,         RTGETOPT_REQ_BOOL_ONOFF },
#endif
    { "--bioslogofadein",           MODIFYVM_BIOSLOGOFADEIN,            RTGETOPT_REQ_BOOL_ONOFF },
    { "--bioslogofadeout",          MODIFYVM_BIOSLOGOFADEOUT,           RTGETOPT_REQ_BOOL_ONOFF },
    { "--bioslogodisplaytime",      MODIFYVM_BIOSLOGODISPLAYTIME,       RTGETOPT_REQ_UINT32 },
    { "--bioslogoimagepath",        MODIFYVM_BIOSLOGOIMAGEPATH,         RTGETOPT_REQ_STRING },
    { "--biosbootmenu",             MODIFYVM_BIOSBOOTMENU,              RTGETOPT_REQ_STRING },
    { "--biossystemtimeoffset",     MODIFYVM_BIOSSYSTEMTIMEOFFSET,      RTGETOPT_REQ_INT64 },
    { "--biosapic",                 MODIFYVM_BIOSAPIC,                  RTGETOPT_REQ_STRING },
    { "--biospxedebug",             MODIFYVM_BIOSPXEDEBUG,              RTGETOPT_REQ_BOOL_ONOFF },
    { "--boot",                     MODIFYVM_BOOT,                      RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--hda",                      MODIFYVM_HDA,                       RTGETOPT_REQ_STRING },
    { "--hdb",                      MODIFYVM_HDB,                       RTGETOPT_REQ_STRING },
    { "--hdd",                      MODIFYVM_HDD,                       RTGETOPT_REQ_STRING },
    { "--idecontroller",            MODIFYVM_IDECONTROLLER,             RTGETOPT_REQ_STRING },
    { "--sataportcount",            MODIFYVM_SATAPORTCOUNT,             RTGETOPT_REQ_UINT32 },
    { "--sataport",                 MODIFYVM_SATAPORT,                  RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--sata",                     MODIFYVM_SATA,                      RTGETOPT_REQ_STRING },
    { "--scsiport",                 MODIFYVM_SCSIPORT,                  RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--scsitype",                 MODIFYVM_SCSITYPE,                  RTGETOPT_REQ_STRING },
    { "--scsi",                     MODIFYVM_SCSI,                      RTGETOPT_REQ_STRING },
    { "--dvdpassthrough",           MODIFYVM_DVDPASSTHROUGH,            RTGETOPT_REQ_STRING },
    { "--dvd",                      MODIFYVM_DVD,                       RTGETOPT_REQ_STRING },
    { "--floppy",                   MODIFYVM_FLOPPY,                    RTGETOPT_REQ_STRING },
    { "--nictracefile",             MODIFYVM_NICTRACEFILE,              RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nictrace",                 MODIFYVM_NICTRACE,                  RTGETOPT_REQ_BOOL_ONOFF | RTGETOPT_FLAG_INDEX },
    { "--nicproperty",              MODIFYVM_NICPROPERTY,               RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nictype",                  MODIFYVM_NICTYPE,                   RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nicspeed",                 MODIFYVM_NICSPEED,                  RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_INDEX },
    { "--nicbootprio",              MODIFYVM_NICBOOTPRIO,               RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_INDEX },
    { "--nicpromisc",               MODIFYVM_NICPROMISC,                RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nicbandwidthgroup",        MODIFYVM_NICBWGROUP,                RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nic",                      MODIFYVM_NIC,                       RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--cableconnected",           MODIFYVM_CABLECONNECTED,            RTGETOPT_REQ_BOOL_ONOFF | RTGETOPT_FLAG_INDEX },
    { "--bridgeadapter",            MODIFYVM_BRIDGEADAPTER,             RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--hostonlyadapter",          MODIFYVM_HOSTONLYADAPTER,           RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--intnet",                   MODIFYVM_INTNET,                    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nicgenericdrv",            MODIFYVM_GENERICDRV,                RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nat-network",              MODIFYVM_NATNETWORKNAME,            RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--natnetwork",               MODIFYVM_NATNETWORKNAME,            RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--natnet",                   MODIFYVM_NATNET,                    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--natbindip",                MODIFYVM_NATBINDIP,                 RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--natsettings",              MODIFYVM_NATSETTINGS,               RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--natpf",                    MODIFYVM_NATPF,                     RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nataliasmode",             MODIFYVM_NATALIASMODE,              RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nattftpprefix",            MODIFYVM_NATTFTPPREFIX,             RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nattftpfile",              MODIFYVM_NATTFTPFILE,               RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nattftpserver",            MODIFYVM_NATTFTPSERVER,             RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--natdnspassdomain",         MODIFYVM_NATDNSPASSDOMAIN,          RTGETOPT_REQ_BOOL_ONOFF | RTGETOPT_FLAG_INDEX },
    { "--natdnsproxy",              MODIFYVM_NATDNSPROXY,               RTGETOPT_REQ_BOOL_ONOFF | RTGETOPT_FLAG_INDEX },
    { "--natdnshostresolver",       MODIFYVM_NATDNSHOSTRESOLVER,        RTGETOPT_REQ_BOOL_ONOFF | RTGETOPT_FLAG_INDEX },
    { "--macaddress",               MODIFYVM_MACADDRESS,                RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--mouse",                    MODIFYVM_HIDPTR,                    RTGETOPT_REQ_STRING },
    { "--keyboard",                 MODIFYVM_HIDKBD,                    RTGETOPT_REQ_STRING },
    { "--uartmode",                 MODIFYVM_UARTMODE,                  RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--uart",                     MODIFYVM_UART,                      RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
#if defined(RT_OS_LINUX) || defined(RT_OS_WINDOWS)
    { "--lptmode",                  MODIFYVM_LPTMODE,                   RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--lpt",                      MODIFYVM_LPT,                       RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
#endif
    { "--guestmemoryballoon",       MODIFYVM_GUESTMEMORYBALLOON,        RTGETOPT_REQ_UINT32 },
    { "--audiocontroller",          MODIFYVM_AUDIOCONTROLLER,           RTGETOPT_REQ_STRING },
    { "--audiocodec",               MODIFYVM_AUDIOCODEC,                RTGETOPT_REQ_STRING },
    { "--audio",                    MODIFYVM_AUDIO,                     RTGETOPT_REQ_STRING },
    { "--audioin",                  MODIFYVM_AUDIOIN,                   RTGETOPT_REQ_BOOL_ONOFF },
    { "--audioout",                 MODIFYVM_AUDIOOUT,                  RTGETOPT_REQ_BOOL_ONOFF },
    { "--clipboard",                MODIFYVM_CLIPBOARD,                 RTGETOPT_REQ_STRING },
    { "--draganddrop",              MODIFYVM_DRAGANDDROP,               RTGETOPT_REQ_STRING },
    { "--vrdpport",                 MODIFYVM_VRDPPORT,                  RTGETOPT_REQ_STRING },     /* deprecated */
    { "--vrdpaddress",              MODIFYVM_VRDPADDRESS,               RTGETOPT_REQ_STRING },     /* deprecated */
    { "--vrdpauthtype",             MODIFYVM_VRDPAUTHTYPE,              RTGETOPT_REQ_STRING },     /* deprecated */
    { "--vrdpmulticon",             MODIFYVM_VRDPMULTICON,              RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--vrdpreusecon",             MODIFYVM_VRDPREUSECON,              RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--vrdpvideochannel",         MODIFYVM_VRDPVIDEOCHANNEL,          RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--vrdpvideochannelquality",  MODIFYVM_VRDPVIDEOCHANNELQUALITY,   RTGETOPT_REQ_STRING },     /* deprecated */
    { "--vrdp",                     MODIFYVM_VRDP,                      RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--vrdeproperty",             MODIFYVM_VRDEPROPERTY,              RTGETOPT_REQ_STRING },
    { "--vrdeport",                 MODIFYVM_VRDEPORT,                  RTGETOPT_REQ_STRING },
    { "--vrdeaddress",              MODIFYVM_VRDEADDRESS,               RTGETOPT_REQ_STRING },
    { "--vrdeauthtype",             MODIFYVM_VRDEAUTHTYPE,              RTGETOPT_REQ_STRING },
    { "--vrdeauthlibrary",          MODIFYVM_VRDEAUTHLIBRARY,           RTGETOPT_REQ_STRING },
    { "--vrdemulticon",             MODIFYVM_VRDEMULTICON,              RTGETOPT_REQ_BOOL_ONOFF },
    { "--vrdereusecon",             MODIFYVM_VRDEREUSECON,              RTGETOPT_REQ_BOOL_ONOFF },
    { "--vrdevideochannel",         MODIFYVM_VRDEVIDEOCHANNEL,          RTGETOPT_REQ_BOOL_ONOFF },
    { "--vrdevideochannelquality",  MODIFYVM_VRDEVIDEOCHANNELQUALITY,   RTGETOPT_REQ_STRING },
    { "--vrdeextpack",              MODIFYVM_VRDE_EXTPACK,              RTGETOPT_REQ_STRING },
    { "--vrde",                     MODIFYVM_VRDE,                      RTGETOPT_REQ_BOOL_ONOFF },
    { "--usbrename",                MODIFYVM_USBRENAME,                 RTGETOPT_REQ_STRING },
    { "--usbxhci",                  MODIFYVM_USBXHCI,                   RTGETOPT_REQ_BOOL_ONOFF },
    { "--usbehci",                  MODIFYVM_USBEHCI,                   RTGETOPT_REQ_BOOL_ONOFF },
    { "--usb",                      MODIFYVM_USB,                       RTGETOPT_REQ_BOOL_ONOFF },
    { "--snapshotfolder",           MODIFYVM_SNAPSHOTFOLDER,            RTGETOPT_REQ_STRING },
    { "--teleporter",               MODIFYVM_TELEPORTER_ENABLED,        RTGETOPT_REQ_BOOL_ONOFF },
    { "--teleporterenabled",        MODIFYVM_TELEPORTER_ENABLED,        RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--teleporterport",           MODIFYVM_TELEPORTER_PORT,           RTGETOPT_REQ_UINT32 },
    { "--teleporteraddress",        MODIFYVM_TELEPORTER_ADDRESS,        RTGETOPT_REQ_STRING },
    { "--teleporterpassword",       MODIFYVM_TELEPORTER_PASSWORD,       RTGETOPT_REQ_STRING },
    { "--teleporterpasswordfile",   MODIFYVM_TELEPORTER_PASSWORD_FILE,  RTGETOPT_REQ_STRING },
    { "--tracing-enabled",          MODIFYVM_TRACING_ENABLED,           RTGETOPT_REQ_BOOL_ONOFF },
    { "--tracing-config",           MODIFYVM_TRACING_CONFIG,            RTGETOPT_REQ_STRING },
    { "--tracing-allow-vm-access",  MODIFYVM_TRACING_ALLOW_VM_ACCESS,   RTGETOPT_REQ_BOOL_ONOFF },
    { "--hardwareuuid",             MODIFYVM_HARDWARE_UUID,             RTGETOPT_REQ_STRING },
    { "--hpet",                     MODIFYVM_HPET,                      RTGETOPT_REQ_BOOL_ONOFF },
    { "--iocache",                  MODIFYVM_IOCACHE,                   RTGETOPT_REQ_BOOL_ONOFF },
    { "--iocachesize",              MODIFYVM_IOCACHESIZE,               RTGETOPT_REQ_UINT32 },
    { "--faulttolerance",           MODIFYVM_FAULT_TOLERANCE,           RTGETOPT_REQ_STRING },
    { "--faulttoleranceaddress",    MODIFYVM_FAULT_TOLERANCE_ADDRESS,   RTGETOPT_REQ_STRING },
    { "--faulttoleranceport",       MODIFYVM_FAULT_TOLERANCE_PORT,      RTGETOPT_REQ_UINT32 },
    { "--faulttolerancepassword",   MODIFYVM_FAULT_TOLERANCE_PASSWORD,  RTGETOPT_REQ_STRING },
    { "--faulttolerancesyncinterval", MODIFYVM_FAULT_TOLERANCE_SYNC_INTERVAL, RTGETOPT_REQ_UINT32 },
    { "--chipset",                  MODIFYVM_CHIPSET,                   RTGETOPT_REQ_STRING },
#ifdef VBOX_WITH_VIDEOREC
    { "--videocap",                 MODIFYVM_VIDEOCAP,                  RTGETOPT_REQ_BOOL_ONOFF },
    { "--vcpenabled",               MODIFYVM_VIDEOCAP,                  RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--videocapscreens",          MODIFYVM_VIDEOCAP_SCREENS,          RTGETOPT_REQ_STRING },
    { "--vcpscreens",               MODIFYVM_VIDEOCAP_SCREENS,          RTGETOPT_REQ_STRING }, /* deprecated */
    { "--videocapfile",             MODIFYVM_VIDEOCAP_FILENAME,         RTGETOPT_REQ_STRING },
    { "--vcpfile",                  MODIFYVM_VIDEOCAP_FILENAME,         RTGETOPT_REQ_STRING }, /* deprecated */
    { "--videocapres",              MODIFYVM_VIDEOCAP_RES,              RTGETOPT_REQ_STRING },
    { "--vcpwidth",                 MODIFYVM_VIDEOCAP_WIDTH,            RTGETOPT_REQ_UINT32 }, /* deprecated */
    { "--vcpheight",                MODIFYVM_VIDEOCAP_HEIGHT,           RTGETOPT_REQ_UINT32 }, /* deprecated */
    { "--videocaprate",             MODIFYVM_VIDEOCAP_RATE,             RTGETOPT_REQ_UINT32 },
    { "--vcprate",                  MODIFYVM_VIDEOCAP_RATE,             RTGETOPT_REQ_UINT32 }, /* deprecated */
    { "--videocapfps",              MODIFYVM_VIDEOCAP_FPS,              RTGETOPT_REQ_UINT32 },
    { "--vcpfps",                   MODIFYVM_VIDEOCAP_FPS,              RTGETOPT_REQ_UINT32 }, /* deprecated */
    { "--videocapmaxtime",          MODIFYVM_VIDEOCAP_MAXTIME,          RTGETOPT_REQ_INT32  },
    { "--vcpmaxtime",               MODIFYVM_VIDEOCAP_MAXTIME,          RTGETOPT_REQ_INT32  }, /* deprecated */
    { "--videocapmaxsize",          MODIFYVM_VIDEOCAP_MAXSIZE,          RTGETOPT_REQ_INT32  },
    { "--vcpmaxsize",               MODIFYVM_VIDEOCAP_MAXSIZE,          RTGETOPT_REQ_INT32  }, /* deprecated */
    { "--videocapopts",             MODIFYVM_VIDEOCAP_OPTIONS,          RTGETOPT_REQ_STRING },
    { "--vcpoptions",               MODIFYVM_VIDEOCAP_OPTIONS,          RTGETOPT_REQ_STRING }, /* deprecated */
#endif
    { "--autostart-enabled",        MODIFYVM_AUTOSTART_ENABLED,         RTGETOPT_REQ_BOOL_ONOFF },
    { "--autostart-delay",          MODIFYVM_AUTOSTART_DELAY,           RTGETOPT_REQ_UINT32 },
    { "--autostop-type",            MODIFYVM_AUTOSTOP_TYPE,             RTGETOPT_REQ_STRING },
#ifdef VBOX_WITH_PCI_PASSTHROUGH
    { "--pciattach",                MODIFYVM_ATTACH_PCI,                RTGETOPT_REQ_STRING },
    { "--pcidetach",                MODIFYVM_DETACH_PCI,                RTGETOPT_REQ_STRING },
#endif
#ifdef VBOX_WITH_USB_CARDREADER
    { "--usbcardreader",            MODIFYVM_USBCARDREADER,             RTGETOPT_REQ_BOOL_ONOFF },
#endif
    { "--defaultfrontend",          MODIFYVM_DEFAULTFRONTEND,           RTGETOPT_REQ_STRING },
};

static void vrdeWarningDeprecatedOption(const char *pszOption)
{
    RTStrmPrintf(g_pStdErr, "Warning: '--vrdp%s' is deprecated. Use '--vrde%s'.\n", pszOption, pszOption);
}

/** Parse PCI address in format 01:02.03 and convert it to the numeric representation. */
static int32_t parsePci(const char* szPciAddr)
{
    char* pszNext = (char*)szPciAddr;
    int rc;
    uint8_t aVals[3] = {0, 0, 0};

    rc = RTStrToUInt8Ex(pszNext, &pszNext, 16, &aVals[0]);
    if (RT_FAILURE(rc) || pszNext == NULL || *pszNext != ':')
        return -1;

    rc = RTStrToUInt8Ex(pszNext+1, &pszNext, 16, &aVals[1]);
    if (RT_FAILURE(rc) || pszNext == NULL || *pszNext != '.')
        return -1;

    rc = RTStrToUInt8Ex(pszNext+1, &pszNext, 16, &aVals[2]);
    if (RT_FAILURE(rc) || pszNext == NULL)
        return -1;

    return (aVals[0] << 8) | (aVals[1] << 3) | (aVals[2] << 0);
}

void parseGroups(const char *pcszGroups, com::SafeArray<BSTR> *pGroups)
{
    while (pcszGroups)
    {
        char *pComma = RTStrStr(pcszGroups, ",");
        if (pComma)
        {
            Bstr(pcszGroups, pComma - pcszGroups).detachTo(pGroups->appendedRaw());
            pcszGroups = pComma + 1;
        }
        else
        {
            Bstr(pcszGroups).detachTo(pGroups->appendedRaw());
            pcszGroups = NULL;
        }
    }
}

#ifdef VBOX_WITH_VIDEOREC
static int parseScreens(const char *pcszScreens, com::SafeArray<BOOL> *pScreens)
{
    while (pcszScreens && *pcszScreens)
    {
        char *pszNext;
        uint32_t iScreen;
        int rc = RTStrToUInt32Ex(pcszScreens, &pszNext, 0, &iScreen);
        if (RT_FAILURE(rc))
            return 1;
        if (iScreen >= pScreens->size())
            return 1;
        if (pszNext && *pszNext)
        {
            pszNext = RTStrStripL(pszNext);
            if (*pszNext != ',')
                return 1;
            pszNext++;
        }
        (*pScreens)[iScreen] = true;
        pcszScreens = pszNext;
    }
    return 0;
}
#endif

static int parseNum(uint32_t uIndex, unsigned cMaxIndex, const char *pszName)
{
    if (   uIndex >= 1
        && uIndex <= cMaxIndex)
        return uIndex;
    errorArgument("Invalid %s number %u", pszName, uIndex);
    return 0;
}

RTEXITCODE handleModifyVM(HandlerArg *a)
{
    int c;
    HRESULT rc;
    Bstr name;

    /* VM ID + at least one parameter. Parameter arguments are checked
     * individually. */
    if (a->argc < 2)
        return errorSyntax(USAGE_MODIFYVM, "Not enough parameters");

    /* try to find the given sessionMachine */
    ComPtr<IMachine> machine;
    CHECK_ERROR_RET(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                               machine.asOutParam()), RTEXITCODE_FAILURE);


    /* Get the number of network adapters */
    ULONG NetworkAdapterCount = getMaxNics(a->virtualBox, machine);

    /* open a session for the VM */
    CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Write), RTEXITCODE_FAILURE);

    /* get the mutable session sessionMachine */
    ComPtr<IMachine> sessionMachine;
    CHECK_ERROR_RET(a->session, COMGETTER(Machine)(sessionMachine.asOutParam()), RTEXITCODE_FAILURE);

    ComPtr<IBIOSSettings> biosSettings;
    sessionMachine->COMGETTER(BIOSSettings)(biosSettings.asOutParam());

    RTGETOPTSTATE GetOptState;
    RTGetOptInit(&GetOptState, a->argc, a->argv, g_aModifyVMOptions,
                 RT_ELEMENTS(g_aModifyVMOptions), 1, RTGETOPTINIT_FLAGS_NO_STD_OPTS);

    RTGETOPTUNION ValueUnion;
    while (   SUCCEEDED (rc)
           && (c = RTGetOpt(&GetOptState, &ValueUnion)))
    {
        switch (c)
        {
            case MODIFYVM_NAME:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(Name)(Bstr(ValueUnion.psz).raw()));
                break;
            }
            case MODIFYVM_GROUPS:
            {
                com::SafeArray<BSTR> groups;
                parseGroups(ValueUnion.psz, &groups);
                CHECK_ERROR(sessionMachine, COMSETTER(Groups)(ComSafeArrayAsInParam(groups)));
                break;
            }
            case MODIFYVM_DESCRIPTION:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(Description)(Bstr(ValueUnion.psz).raw()));
                break;
            }
            case MODIFYVM_OSTYPE:
            {
                ComPtr<IGuestOSType> guestOSType;
                CHECK_ERROR(a->virtualBox, GetGuestOSType(Bstr(ValueUnion.psz).raw(),
                                                          guestOSType.asOutParam()));
                if (SUCCEEDED(rc) && guestOSType)
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(OSTypeId)(Bstr(ValueUnion.psz).raw()));
                }
                else
                {
                    errorArgument("Invalid guest OS type '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_ICONFILE:
            {
                RTFILE iconFile;
                int vrc = RTFileOpen(&iconFile, ValueUnion.psz, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_WRITE);
                if (RT_FAILURE(vrc))
                {
                    RTMsgError("Cannot open file \"%s\": %Rrc", ValueUnion.psz, vrc);
                    rc = E_FAIL;
                    break;
                }
                uint64_t cbSize;
                vrc = RTFileGetSize(iconFile, &cbSize);
                if (RT_FAILURE(vrc))
                {
                    RTMsgError("Cannot get size of file \"%s\": %Rrc", ValueUnion.psz, vrc);
                    rc = E_FAIL;
                    break;
                }
                if (cbSize > _256K)
                {
                    RTMsgError("File \"%s\" is bigger than 256KByte", ValueUnion.psz);
                    rc = E_FAIL;
                    break;
                }
                SafeArray<BYTE> icon((size_t)cbSize);
                rc = RTFileRead(iconFile, icon.raw(), (size_t)cbSize, NULL);
                if (RT_FAILURE(vrc))
                {
                    RTMsgError("Cannot read contents of file \"%s\": %Rrc", ValueUnion.psz, vrc);
                    rc = E_FAIL;
                    break;
                }
                RTFileClose(iconFile);
                CHECK_ERROR(sessionMachine, COMSETTER(Icon)(ComSafeArrayAsInParam(icon)));
                break;
            }

            case MODIFYVM_MEMORY:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(MemorySize)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_PAGEFUSION:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(PageFusionEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_VRAM:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(VRAMSize)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_FIRMWARE:
            {
                if (!RTStrICmp(ValueUnion.psz, "efi"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(FirmwareType)(FirmwareType_EFI));
                }
                else if (!RTStrICmp(ValueUnion.psz, "efi32"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(FirmwareType)(FirmwareType_EFI32));
                }
                else if (!RTStrICmp(ValueUnion.psz, "efi64"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(FirmwareType)(FirmwareType_EFI64));
                }
                else if (!RTStrICmp(ValueUnion.psz, "efidual"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(FirmwareType)(FirmwareType_EFIDUAL));
                }
                else if (!RTStrICmp(ValueUnion.psz, "bios"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(FirmwareType)(FirmwareType_BIOS));
                }
                else
                {
                    errorArgument("Invalid --firmware argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_ACPI:
            {
                CHECK_ERROR(biosSettings, COMSETTER(ACPIEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_IOAPIC:
            {
                CHECK_ERROR(biosSettings, COMSETTER(IOAPICEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_PAE:
            {
                CHECK_ERROR(sessionMachine, SetCPUProperty(CPUPropertyType_PAE, ValueUnion.f));
                break;
            }

            case MODIFYVM_LONGMODE:
            {
                CHECK_ERROR(sessionMachine, SetCPUProperty(CPUPropertyType_LongMode, ValueUnion.f));
                break;
            }

            case MODIFYVM_CPUID_PORTABILITY:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(CPUIDPortabilityLevel)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_TFRESET:
            {
                CHECK_ERROR(sessionMachine, SetCPUProperty(CPUPropertyType_TripleFaultReset, ValueUnion.f));
                break;
            }

            case MODIFYVM_APIC:
            {
                CHECK_ERROR(sessionMachine, SetCPUProperty(CPUPropertyType_APIC, ValueUnion.f));
                break;
            }

            case MODIFYVM_X2APIC:
            {
                CHECK_ERROR(sessionMachine, SetCPUProperty(CPUPropertyType_X2APIC, ValueUnion.f));
                break;
            }

            case MODIFYVM_PARAVIRTPROVIDER:
            {
                if (   !RTStrICmp(ValueUnion.psz, "none")
                    || !RTStrICmp(ValueUnion.psz, "disabled"))
                    CHECK_ERROR(sessionMachine, COMSETTER(ParavirtProvider)(ParavirtProvider_None));
                else if (!RTStrICmp(ValueUnion.psz, "default"))
                    CHECK_ERROR(sessionMachine, COMSETTER(ParavirtProvider)(ParavirtProvider_Default));
                else if (!RTStrICmp(ValueUnion.psz, "legacy"))
                    CHECK_ERROR(sessionMachine, COMSETTER(ParavirtProvider)(ParavirtProvider_Legacy));
                else if (!RTStrICmp(ValueUnion.psz, "minimal"))
                    CHECK_ERROR(sessionMachine, COMSETTER(ParavirtProvider)(ParavirtProvider_Minimal));
                else if (!RTStrICmp(ValueUnion.psz, "hyperv"))
                    CHECK_ERROR(sessionMachine, COMSETTER(ParavirtProvider)(ParavirtProvider_HyperV));
                else if (!RTStrICmp(ValueUnion.psz, "kvm"))
                    CHECK_ERROR(sessionMachine, COMSETTER(ParavirtProvider)(ParavirtProvider_KVM));
                else
                {
                    errorArgument("Invalid --paravirtprovider argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_PARAVIRTDEBUG:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(ParavirtDebug)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_HWVIRTEX:
            {
                CHECK_ERROR(sessionMachine, SetHWVirtExProperty(HWVirtExPropertyType_Enabled, ValueUnion.f));
                break;
            }

            case MODIFYVM_SETCPUID:
            case MODIFYVM_SETCPUID_OLD:
            {
                uint32_t const idx    = c == MODIFYVM_SETCPUID ?  ValueUnion.PairU32.uFirst  : ValueUnion.u32;
                uint32_t const idxSub = c == MODIFYVM_SETCPUID ?  ValueUnion.PairU32.uSecond : UINT32_MAX;
                uint32_t aValue[4];
                for (unsigned i = 0; i < 4; i++)
                {
                    int vrc = RTGetOptFetchValue(&GetOptState, &ValueUnion, RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_HEX);
                    if (RT_FAILURE(vrc))
                        return errorSyntax(USAGE_MODIFYVM, "Missing or Invalid argument to '%s'", GetOptState.pDef->pszLong);
                    aValue[i] = ValueUnion.u32;
                }
                CHECK_ERROR(sessionMachine, SetCPUIDLeaf(idx, idxSub, aValue[0], aValue[1], aValue[2], aValue[3]));
                break;
            }

            case MODIFYVM_DELCPUID:
                CHECK_ERROR(sessionMachine, RemoveCPUIDLeaf(ValueUnion.PairU32.uFirst, ValueUnion.PairU32.uSecond));
                break;

            case MODIFYVM_DELCPUID_OLD:
                CHECK_ERROR(sessionMachine, RemoveCPUIDLeaf(ValueUnion.u32, UINT32_MAX));
                break;

            case MODIFYVM_DELALLCPUID:
            {
                CHECK_ERROR(sessionMachine, RemoveAllCPUIDLeaves());
                break;
            }

            case MODIFYVM_NESTEDPAGING:
            {
                CHECK_ERROR(sessionMachine, SetHWVirtExProperty(HWVirtExPropertyType_NestedPaging, ValueUnion.f));
                break;
            }

            case MODIFYVM_LARGEPAGES:
            {
                CHECK_ERROR(sessionMachine, SetHWVirtExProperty(HWVirtExPropertyType_LargePages, ValueUnion.f));
                break;
            }

            case MODIFYVM_VTXVPID:
            {
                CHECK_ERROR(sessionMachine, SetHWVirtExProperty(HWVirtExPropertyType_VPID, ValueUnion.f));
                break;
            }

            case MODIFYVM_VTXUX:
            {
                CHECK_ERROR(sessionMachine, SetHWVirtExProperty(HWVirtExPropertyType_UnrestrictedExecution, ValueUnion.f));
                break;
            }

            case MODIFYVM_CPUS:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(CPUCount)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_RTCUSEUTC:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(RTCUseUTC)(ValueUnion.f));
                break;
            }

            case MODIFYVM_CPUHOTPLUG:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(CPUHotPlugEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_CPU_PROFILE:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(CPUProfile)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_PLUGCPU:
            {
                CHECK_ERROR(sessionMachine, HotPlugCPU(ValueUnion.u32));
                break;
            }

            case MODIFYVM_UNPLUGCPU:
            {
                CHECK_ERROR(sessionMachine, HotUnplugCPU(ValueUnion.u32));
                break;
            }

            case MODIFYVM_CPU_EXECTUION_CAP:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(CPUExecutionCap)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_GRAPHICSCONTROLLER:
            {
                if (   !RTStrICmp(ValueUnion.psz, "none")
                    || !RTStrICmp(ValueUnion.psz, "disabled"))
                    CHECK_ERROR(sessionMachine, COMSETTER(GraphicsControllerType)(GraphicsControllerType_Null));
                else if (   !RTStrICmp(ValueUnion.psz, "vboxvga")
                         || !RTStrICmp(ValueUnion.psz, "vbox")
                         || !RTStrICmp(ValueUnion.psz, "vga")
                         || !RTStrICmp(ValueUnion.psz, "vesa"))
                    CHECK_ERROR(sessionMachine, COMSETTER(GraphicsControllerType)(GraphicsControllerType_VBoxVGA));
#ifdef VBOX_WITH_VMSVGA
                else if (   !RTStrICmp(ValueUnion.psz, "vmsvga")
                         || !RTStrICmp(ValueUnion.psz, "vmware"))
                    CHECK_ERROR(sessionMachine, COMSETTER(GraphicsControllerType)(GraphicsControllerType_VMSVGA));
#endif
                else
                {
                    errorArgument("Invalid --graphicscontroller argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_MONITORCOUNT:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(MonitorCount)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_ACCELERATE3D:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(Accelerate3DEnabled)(ValueUnion.f));
                break;
            }

#ifdef VBOX_WITH_VIDEOHWACCEL
            case MODIFYVM_ACCELERATE2DVIDEO:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(Accelerate2DVideoEnabled)(ValueUnion.f));
                break;
            }
#endif

            case MODIFYVM_BIOSLOGOFADEIN:
            {
                CHECK_ERROR(biosSettings, COMSETTER(LogoFadeIn)(ValueUnion.f));
                break;
            }

            case MODIFYVM_BIOSLOGOFADEOUT:
            {
                CHECK_ERROR(biosSettings, COMSETTER(LogoFadeOut)(ValueUnion.f));
                break;
            }

            case MODIFYVM_BIOSLOGODISPLAYTIME:
            {
                CHECK_ERROR(biosSettings, COMSETTER(LogoDisplayTime)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_BIOSLOGOIMAGEPATH:
            {
                CHECK_ERROR(biosSettings, COMSETTER(LogoImagePath)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_BIOSBOOTMENU:
            {
                if (!RTStrICmp(ValueUnion.psz, "disabled"))
                {
                    CHECK_ERROR(biosSettings, COMSETTER(BootMenuMode)(BIOSBootMenuMode_Disabled));
                }
                else if (!RTStrICmp(ValueUnion.psz, "menuonly"))
                {
                    CHECK_ERROR(biosSettings, COMSETTER(BootMenuMode)(BIOSBootMenuMode_MenuOnly));
                }
                else if (!RTStrICmp(ValueUnion.psz, "messageandmenu"))
                {
                    CHECK_ERROR(biosSettings, COMSETTER(BootMenuMode)(BIOSBootMenuMode_MessageAndMenu));
                }
                else
                {
                    errorArgument("Invalid --biosbootmenu argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_BIOSAPIC:
            {
                if (!RTStrICmp(ValueUnion.psz, "disabled"))
                {
                    CHECK_ERROR(biosSettings, COMSETTER(APICMode)(APICMode_Disabled));
                }
                else if (   !RTStrICmp(ValueUnion.psz, "apic")
                         || !RTStrICmp(ValueUnion.psz, "lapic")
                         || !RTStrICmp(ValueUnion.psz, "xapic"))
                {
                    CHECK_ERROR(biosSettings, COMSETTER(APICMode)(APICMode_APIC));
                }
                else if (!RTStrICmp(ValueUnion.psz, "x2apic"))
                {
                    CHECK_ERROR(biosSettings, COMSETTER(APICMode)(APICMode_X2APIC));
                }
                else
                {
                    errorArgument("Invalid --biosapic argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_BIOSSYSTEMTIMEOFFSET:
            {
                CHECK_ERROR(biosSettings, COMSETTER(TimeOffset)(ValueUnion.i64));
                break;
            }

            case MODIFYVM_BIOSPXEDEBUG:
            {
                CHECK_ERROR(biosSettings, COMSETTER(PXEDebugEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_BOOT:
            {
                if (!RTStrICmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(sessionMachine, SetBootOrder(GetOptState.uIndex, DeviceType_Null));
                }
                else if (!RTStrICmp(ValueUnion.psz, "floppy"))
                {
                    CHECK_ERROR(sessionMachine, SetBootOrder(GetOptState.uIndex, DeviceType_Floppy));
                }
                else if (!RTStrICmp(ValueUnion.psz, "dvd"))
                {
                    CHECK_ERROR(sessionMachine, SetBootOrder(GetOptState.uIndex, DeviceType_DVD));
                }
                else if (!RTStrICmp(ValueUnion.psz, "disk"))
                {
                    CHECK_ERROR(sessionMachine, SetBootOrder(GetOptState.uIndex, DeviceType_HardDisk));
                }
                else if (!RTStrICmp(ValueUnion.psz, "net"))
                {
                    CHECK_ERROR(sessionMachine, SetBootOrder(GetOptState.uIndex, DeviceType_Network));
                }
                else
                    return errorArgument("Invalid boot device '%s'", ValueUnion.psz);
                break;
            }

            case MODIFYVM_HDA: // deprecated
            case MODIFYVM_HDB: // deprecated
            case MODIFYVM_HDD: // deprecated
            case MODIFYVM_SATAPORT: // deprecated
            {
                uint32_t u1 = 0, u2 = 0;
                Bstr bstrController = L"IDE Controller";

                switch (c)
                {
                    case MODIFYVM_HDA: // deprecated
                        u1 = 0;
                    break;

                    case MODIFYVM_HDB: // deprecated
                        u1 = 0;
                        u2 = 1;
                    break;

                    case MODIFYVM_HDD: // deprecated
                        u1 = 1;
                        u2 = 1;
                    break;

                    case MODIFYVM_SATAPORT: // deprecated
                        u1 = GetOptState.uIndex;
                        bstrController = L"SATA";
                    break;
                }

                if (!RTStrICmp(ValueUnion.psz, "none"))
                {
                    sessionMachine->DetachDevice(bstrController.raw(), u1, u2);
                }
                else
                {
                    ComPtr<IMedium> hardDisk;
                    rc = openMedium(a, ValueUnion.psz, DeviceType_HardDisk,
                                    AccessMode_ReadWrite, hardDisk,
                                    false /* fForceNewUuidOnOpen */,
                                    false /* fSilent */);
                    if (FAILED(rc))
                        break;
                    if (hardDisk)
                    {
                        CHECK_ERROR(sessionMachine, AttachDevice(bstrController.raw(),
                                                          u1, u2,
                                                          DeviceType_HardDisk,
                                                          hardDisk));
                    }
                    else
                        rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_IDECONTROLLER: // deprecated
            {
                ComPtr<IStorageController> storageController;
                CHECK_ERROR(sessionMachine, GetStorageControllerByName(Bstr("IDE Controller").raw(),
                                                                 storageController.asOutParam()));

                if (!RTStrICmp(ValueUnion.psz, "PIIX3"))
                {
                    CHECK_ERROR(storageController, COMSETTER(ControllerType)(StorageControllerType_PIIX3));
                }
                else if (!RTStrICmp(ValueUnion.psz, "PIIX4"))
                {
                    CHECK_ERROR(storageController, COMSETTER(ControllerType)(StorageControllerType_PIIX4));
                }
                else if (!RTStrICmp(ValueUnion.psz, "ICH6"))
                {
                    CHECK_ERROR(storageController, COMSETTER(ControllerType)(StorageControllerType_ICH6));
                }
                else
                {
                    errorArgument("Invalid --idecontroller argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_SATAPORTCOUNT: // deprecated
            {
                ComPtr<IStorageController> SataCtl;
                CHECK_ERROR(sessionMachine, GetStorageControllerByName(Bstr("SATA").raw(),
                                                                SataCtl.asOutParam()));

                if (SUCCEEDED(rc) && ValueUnion.u32 > 0)
                    CHECK_ERROR(SataCtl, COMSETTER(PortCount)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_SATA: // deprecated
            {
                if (!RTStrICmp(ValueUnion.psz, "on") || !RTStrICmp(ValueUnion.psz, "enable"))
                {
                    ComPtr<IStorageController> ctl;
                    CHECK_ERROR(sessionMachine, AddStorageController(Bstr("SATA").raw(),
                                                              StorageBus_SATA,
                                                              ctl.asOutParam()));
                    CHECK_ERROR(ctl, COMSETTER(ControllerType)(StorageControllerType_IntelAhci));
                }
                else if (!RTStrICmp(ValueUnion.psz, "off") || !RTStrICmp(ValueUnion.psz, "disable"))
                    CHECK_ERROR(sessionMachine, RemoveStorageController(Bstr("SATA").raw()));
                else
                    return errorArgument("Invalid --usb argument '%s'", ValueUnion.psz);
                break;
            }

            case MODIFYVM_SCSIPORT: // deprecated
            {
                if (!RTStrICmp(ValueUnion.psz, "none"))
                {
                    rc = sessionMachine->DetachDevice(Bstr("LsiLogic").raw(),
                                               GetOptState.uIndex, 0);
                    if (FAILED(rc))
                        CHECK_ERROR(sessionMachine, DetachDevice(Bstr("BusLogic").raw(),
                                                          GetOptState.uIndex, 0));
                }
                else
                {
                    ComPtr<IMedium> hardDisk;
                    rc = openMedium(a, ValueUnion.psz, DeviceType_HardDisk,
                                    AccessMode_ReadWrite, hardDisk,
                                    false /* fForceNewUuidOnOpen */,
                                    false /* fSilent */);
                    if (FAILED(rc))
                        break;
                    if (hardDisk)
                    {
                        rc = sessionMachine->AttachDevice(Bstr("LsiLogic").raw(),
                                                   GetOptState.uIndex, 0,
                                                   DeviceType_HardDisk,
                                                   hardDisk);
                        if (FAILED(rc))
                            CHECK_ERROR(sessionMachine,
                                        AttachDevice(Bstr("BusLogic").raw(),
                                                     GetOptState.uIndex, 0,
                                                     DeviceType_HardDisk,
                                                     hardDisk));
                    }
                    else
                        rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_SCSITYPE: // deprecated
            {
                ComPtr<IStorageController> ctl;

                if (!RTStrICmp(ValueUnion.psz, "LsiLogic"))
                {
                    rc = sessionMachine->RemoveStorageController(Bstr("BusLogic").raw());
                    if (FAILED(rc))
                        CHECK_ERROR(sessionMachine, RemoveStorageController(Bstr("LsiLogic").raw()));

                    CHECK_ERROR(sessionMachine,
                                 AddStorageController(Bstr("LsiLogic").raw(),
                                                      StorageBus_SCSI,
                                                      ctl.asOutParam()));

                    if (SUCCEEDED(rc))
                        CHECK_ERROR(ctl, COMSETTER(ControllerType)(StorageControllerType_LsiLogic));
                }
                else if (!RTStrICmp(ValueUnion.psz, "BusLogic"))
                {
                    rc = sessionMachine->RemoveStorageController(Bstr("LsiLogic").raw());
                    if (FAILED(rc))
                        CHECK_ERROR(sessionMachine, RemoveStorageController(Bstr("BusLogic").raw()));

                    CHECK_ERROR(sessionMachine,
                                 AddStorageController(Bstr("BusLogic").raw(),
                                                      StorageBus_SCSI,
                                                      ctl.asOutParam()));

                    if (SUCCEEDED(rc))
                        CHECK_ERROR(ctl, COMSETTER(ControllerType)(StorageControllerType_BusLogic));
                }
                else
                    return errorArgument("Invalid --scsitype argument '%s'", ValueUnion.psz);
                break;
            }

            case MODIFYVM_SCSI: // deprecated
            {
                if (!RTStrICmp(ValueUnion.psz, "on") || !RTStrICmp(ValueUnion.psz, "enable"))
                {
                    ComPtr<IStorageController> ctl;

                    CHECK_ERROR(sessionMachine, AddStorageController(Bstr("BusLogic").raw(),
                                                              StorageBus_SCSI,
                                                              ctl.asOutParam()));
                    if (SUCCEEDED(rc))
                        CHECK_ERROR(ctl, COMSETTER(ControllerType)(StorageControllerType_BusLogic));
                }
                else if (!RTStrICmp(ValueUnion.psz, "off") || !RTStrICmp(ValueUnion.psz, "disable"))
                {
                    rc = sessionMachine->RemoveStorageController(Bstr("BusLogic").raw());
                    if (FAILED(rc))
                        CHECK_ERROR(sessionMachine, RemoveStorageController(Bstr("LsiLogic").raw()));
                }
                break;
            }

            case MODIFYVM_DVDPASSTHROUGH: // deprecated
            {
                CHECK_ERROR(sessionMachine, PassthroughDevice(Bstr("IDE Controller").raw(),
                                                       1, 0,
                                                       !RTStrICmp(ValueUnion.psz, "on")));
                break;
            }

            case MODIFYVM_DVD: // deprecated
            {
                ComPtr<IMedium> dvdMedium;

                /* unmount? */
                if (!RTStrICmp(ValueUnion.psz, "none"))
                {
                    /* nothing to do, NULL object will cause unmount */
                }
                /* host drive? */
                else if (!RTStrNICmp(ValueUnion.psz, RT_STR_TUPLE("host:")))
                {
                    ComPtr<IHost> host;
                    CHECK_ERROR(a->virtualBox, COMGETTER(Host)(host.asOutParam()));
                    rc = host->FindHostDVDDrive(Bstr(ValueUnion.psz + 5).raw(),
                                                dvdMedium.asOutParam());
                    if (!dvdMedium)
                    {
                        /* 2nd try: try with the real name, important on Linux+libhal */
                        char szPathReal[RTPATH_MAX];
                        if (RT_FAILURE(RTPathReal(ValueUnion.psz + 5, szPathReal, sizeof(szPathReal))))
                        {
                            errorArgument("Invalid host DVD drive name \"%s\"", ValueUnion.psz + 5);
                            rc = E_FAIL;
                            break;
                        }
                        rc = host->FindHostDVDDrive(Bstr(szPathReal).raw(),
                                                    dvdMedium.asOutParam());
                        if (!dvdMedium)
                        {
                            errorArgument("Invalid host DVD drive name \"%s\"", ValueUnion.psz + 5);
                            rc = E_FAIL;
                            break;
                        }
                    }
                }
                else
                {
                    rc = openMedium(a, ValueUnion.psz, DeviceType_DVD,
                                    AccessMode_ReadOnly, dvdMedium,
                                    false /* fForceNewUuidOnOpen */,
                                    false /* fSilent */);
                    if (FAILED(rc))
                        break;
                    if (!dvdMedium)
                    {
                        rc = E_FAIL;
                        break;
                    }
                }

                CHECK_ERROR(sessionMachine, MountMedium(Bstr("IDE Controller").raw(),
                                                 1, 0,
                                                 dvdMedium,
                                                 FALSE /* aForce */));
                break;
            }

            case MODIFYVM_FLOPPY: // deprecated
            {
                ComPtr<IMedium> floppyMedium;
                ComPtr<IMediumAttachment> floppyAttachment;
                sessionMachine->GetMediumAttachment(Bstr("Floppy Controller").raw(),
                                             0, 0, floppyAttachment.asOutParam());

                /* disable? */
                if (!RTStrICmp(ValueUnion.psz, "disabled"))
                {
                    /* disable the controller */
                    if (floppyAttachment)
                        CHECK_ERROR(sessionMachine, DetachDevice(Bstr("Floppy Controller").raw(),
                                                          0, 0));
                }
                else
                {
                    /* enable the controller */
                    if (!floppyAttachment)
                        CHECK_ERROR(sessionMachine, AttachDeviceWithoutMedium(Bstr("Floppy Controller").raw(),
                                                                            0, 0,
                                                                            DeviceType_Floppy));

                    /* unmount? */
                    if (    !RTStrICmp(ValueUnion.psz, "none")
                        ||  !RTStrICmp(ValueUnion.psz, "empty"))   // deprecated
                    {
                        /* nothing to do, NULL object will cause unmount */
                    }
                    /* host drive? */
                    else if (!RTStrNICmp(ValueUnion.psz, RT_STR_TUPLE("host:")))
                    {
                        ComPtr<IHost> host;
                        CHECK_ERROR(a->virtualBox, COMGETTER(Host)(host.asOutParam()));
                        rc = host->FindHostFloppyDrive(Bstr(ValueUnion.psz + 5).raw(),
                                                       floppyMedium.asOutParam());
                        if (!floppyMedium)
                        {
                            errorArgument("Invalid host floppy drive name \"%s\"", ValueUnion.psz + 5);
                            rc = E_FAIL;
                            break;
                        }
                    }
                    else
                    {
                        rc = openMedium(a, ValueUnion.psz, DeviceType_Floppy,
                                        AccessMode_ReadWrite, floppyMedium,
                                        false /* fForceNewUuidOnOpen */,
                                        false /* fSilent */);
                        if (FAILED(rc))
                            break;
                        if (!floppyMedium)
                        {
                            rc = E_FAIL;
                            break;
                        }
                    }
                    CHECK_ERROR(sessionMachine, MountMedium(Bstr("Floppy Controller").raw(),
                                                     0, 0,
                                                     floppyMedium,
                                                     FALSE /* aForce */));
                }
                break;
            }

            case MODIFYVM_NICTRACEFILE:
            {

                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(TraceFile)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_NICTRACE:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(TraceEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_NICPROPERTY:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                if (nic)
                {
                    /* Parse 'name=value' */
                    char *pszProperty = RTStrDup(ValueUnion.psz);
                    if (pszProperty)
                    {
                        char *pDelimiter = strchr(pszProperty, '=');
                        if (pDelimiter)
                        {
                            *pDelimiter = '\0';

                            Bstr bstrName = pszProperty;
                            Bstr bstrValue = &pDelimiter[1];
                            CHECK_ERROR(nic, SetProperty(bstrName.raw(), bstrValue.raw()));
                        }
                        else
                        {
                            errorArgument("Invalid --nicproperty%d argument '%s'", GetOptState.uIndex, ValueUnion.psz);
                            rc = E_FAIL;
                        }
                        RTStrFree(pszProperty);
                    }
                    else
                    {
                        RTStrmPrintf(g_pStdErr, "Error: Failed to allocate memory for --nicproperty%d '%s'\n", GetOptState.uIndex, ValueUnion.psz);
                        rc = E_FAIL;
                    }
                }
                break;
            }
            case MODIFYVM_NICTYPE:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                if (!RTStrICmp(ValueUnion.psz, "Am79C970A"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_Am79C970A));
                }
                else if (!RTStrICmp(ValueUnion.psz, "Am79C973"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_Am79C973));
                }
#ifdef VBOX_WITH_E1000
                else if (!RTStrICmp(ValueUnion.psz, "82540EM"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_I82540EM));
                }
                else if (!RTStrICmp(ValueUnion.psz, "82543GC"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_I82543GC));
                }
                else if (!RTStrICmp(ValueUnion.psz, "82545EM"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_I82545EM));
                }
#endif
#ifdef VBOX_WITH_VIRTIO
                else if (!RTStrICmp(ValueUnion.psz, "virtio"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_Virtio));
                }
#endif /* VBOX_WITH_VIRTIO */
                else
                {
                    errorArgument("Invalid NIC type '%s' specified for NIC %u", ValueUnion.psz, GetOptState.uIndex);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_NICSPEED:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(LineSpeed)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_NICBOOTPRIO:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                /* Somewhat arbitrary limitation - we can pass a list of up to 4 PCI devices
                 * to the PXE ROM, hence only boot priorities 1-4 are allowed (in addition to
                 * 0 for the default lowest priority).
                 */
                if (ValueUnion.u32 > 4)
                {
                    errorArgument("Invalid boot priority '%u' specfied for NIC %u", ValueUnion.u32, GetOptState.uIndex);
                    rc = E_FAIL;
                }
                else
                {
                    CHECK_ERROR(nic, COMSETTER(BootPriority)(ValueUnion.u32));
                }
                break;
            }

            case MODIFYVM_NICPROMISC:
            {
                NetworkAdapterPromiscModePolicy_T enmPromiscModePolicy;
                if (!RTStrICmp(ValueUnion.psz, "deny"))
                    enmPromiscModePolicy = NetworkAdapterPromiscModePolicy_Deny;
                else if (   !RTStrICmp(ValueUnion.psz, "allow-vms")
                         || !RTStrICmp(ValueUnion.psz, "allow-network"))
                    enmPromiscModePolicy = NetworkAdapterPromiscModePolicy_AllowNetwork;
                else if (!RTStrICmp(ValueUnion.psz, "allow-all"))
                    enmPromiscModePolicy = NetworkAdapterPromiscModePolicy_AllowAll;
                else
                {
                    errorArgument("Unknown promiscuous mode policy '%s'", ValueUnion.psz);
                    rc = E_INVALIDARG;
                    break;
                }

                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(PromiscModePolicy)(enmPromiscModePolicy));
                break;
            }

            case MODIFYVM_NICBWGROUP:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                if (!RTStrICmp(ValueUnion.psz, "none"))
                {
                    /* Just remove the bandwidth group. */
                    CHECK_ERROR(nic, COMSETTER(BandwidthGroup)(NULL));
                }
                else
                {
                    ComPtr<IBandwidthControl> bwCtrl;
                    ComPtr<IBandwidthGroup> bwGroup;

                    CHECK_ERROR(sessionMachine, COMGETTER(BandwidthControl)(bwCtrl.asOutParam()));

                    if (SUCCEEDED(rc))
                    {
                        CHECK_ERROR(bwCtrl, GetBandwidthGroup(Bstr(ValueUnion.psz).raw(), bwGroup.asOutParam()));
                        if (SUCCEEDED(rc))
                        {
                            CHECK_ERROR(nic, COMSETTER(BandwidthGroup)(bwGroup));
                        }
                    }
                }
                break;
            }

            case MODIFYVM_NIC:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                if (!RTStrICmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(nic, COMSETTER(Enabled)(FALSE));
                }
                else if (!RTStrICmp(ValueUnion.psz, "null"))
                {
                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, COMSETTER(AttachmentType)(NetworkAttachmentType_Null));
                }
                else if (!RTStrICmp(ValueUnion.psz, "nat"))
                {
                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, COMSETTER(AttachmentType)(NetworkAttachmentType_NAT));
                }
                else if (  !RTStrICmp(ValueUnion.psz, "bridged")
                        || !RTStrICmp(ValueUnion.psz, "hostif")) /* backward compatibility */
                {
                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, COMSETTER(AttachmentType)(NetworkAttachmentType_Bridged));
                }
                else if (!RTStrICmp(ValueUnion.psz, "intnet"))
                {
                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, COMSETTER(AttachmentType)(NetworkAttachmentType_Internal));
                }
                else if (!RTStrICmp(ValueUnion.psz, "hostonly"))
                {

                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, COMSETTER(AttachmentType)(NetworkAttachmentType_HostOnly));
                }
                else if (!RTStrICmp(ValueUnion.psz, "generic"))
                {

                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, COMSETTER(AttachmentType)(NetworkAttachmentType_Generic));
                }
                else if (!RTStrICmp(ValueUnion.psz, "natnetwork"))
                {

                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, COMSETTER(AttachmentType)(NetworkAttachmentType_NATNetwork));
                }
                else
                {
                    errorArgument("Invalid type '%s' specfied for NIC %u", ValueUnion.psz, GetOptState.uIndex);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_CABLECONNECTED:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(CableConnected)(ValueUnion.f));
                break;
            }

            case MODIFYVM_BRIDGEADAPTER:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                /* remove it? */
                if (!RTStrICmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(nic, COMSETTER(BridgedInterface)(Bstr().raw()));
                }
                else
                {
                    CHECK_ERROR(nic, COMSETTER(BridgedInterface)(Bstr(ValueUnion.psz).raw()));
                }
                break;
            }

            case MODIFYVM_HOSTONLYADAPTER:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                /* remove it? */
                if (!RTStrICmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(nic, COMSETTER(HostOnlyInterface)(Bstr().raw()));
                }
                else
                {
                    CHECK_ERROR(nic, COMSETTER(HostOnlyInterface)(Bstr(ValueUnion.psz).raw()));
                }
                break;
            }

            case MODIFYVM_INTNET:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                /* remove it? */
                if (!RTStrICmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(nic, COMSETTER(InternalNetwork)(Bstr().raw()));
                }
                else
                {
                    CHECK_ERROR(nic, COMSETTER(InternalNetwork)(Bstr(ValueUnion.psz).raw()));
                }
                break;
            }

            case MODIFYVM_GENERICDRV:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(GenericDriver)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_NATNETWORKNAME:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(NATNetwork)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_NATNET:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                ComPtr<INATEngine> engine;
                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));

                const char *psz = ValueUnion.psz;
                if (!RTStrICmp("default", psz))
                    psz = "";

                CHECK_ERROR(engine, COMSETTER(Network)(Bstr(psz).raw()));
                break;
            }

            case MODIFYVM_NATBINDIP:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                ComPtr<INATEngine> engine;
                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));

                CHECK_ERROR(engine, COMSETTER(HostIP)(Bstr(ValueUnion.psz).raw()));
                break;
            }

#define ITERATE_TO_NEXT_TERM(ch)                                           \
    do {                                                                   \
        while (*ch != ',')                                                 \
        {                                                                  \
            if (*ch == 0)                                                  \
            {                                                              \
                return errorSyntax(USAGE_MODIFYVM,                         \
                                   "Missing or Invalid argument to '%s'",  \
                                    GetOptState.pDef->pszLong);            \
            }                                                              \
            ch++;                                                          \
        }                                                                  \
        *ch = '\0';                                                        \
        ch++;                                                              \
    } while(0)

            case MODIFYVM_NATSETTINGS:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> engine;
                char *strMtu;
                char *strSockSnd;
                char *strSockRcv;
                char *strTcpSnd;
                char *strTcpRcv;
                char *strRaw = RTStrDup(ValueUnion.psz);
                char *ch = strRaw;
                strMtu = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strSockSnd = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strSockRcv = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strTcpSnd = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strTcpRcv = RTStrStrip(ch);

                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));
                CHECK_ERROR(engine, SetNetworkSettings(RTStrToUInt32(strMtu), RTStrToUInt32(strSockSnd), RTStrToUInt32(strSockRcv),
                                    RTStrToUInt32(strTcpSnd), RTStrToUInt32(strTcpRcv)));
                break;
            }


            case MODIFYVM_NATPF:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                ComPtr<INATEngine> engine;
                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));

                /* format name:proto:hostip:hostport:guestip:guestport*/
                if (RTStrCmp(ValueUnion.psz, "delete") != 0)
                {
                    char *strName;
                    char *strProto;
                    char *strHostIp;
                    char *strHostPort;
                    char *strGuestIp;
                    char *strGuestPort;
                    char *strRaw = RTStrDup(ValueUnion.psz);
                    char *ch = strRaw;
                    strName = RTStrStrip(ch);
                    ITERATE_TO_NEXT_TERM(ch);
                    strProto = RTStrStrip(ch);
                    ITERATE_TO_NEXT_TERM(ch);
                    strHostIp = RTStrStrip(ch);
                    ITERATE_TO_NEXT_TERM(ch);
                    strHostPort = RTStrStrip(ch);
                    ITERATE_TO_NEXT_TERM(ch);
                    strGuestIp = RTStrStrip(ch);
                    ITERATE_TO_NEXT_TERM(ch);
                    strGuestPort = RTStrStrip(ch);
                    NATProtocol_T proto;
                    if (RTStrICmp(strProto, "udp") == 0)
                        proto = NATProtocol_UDP;
                    else if (RTStrICmp(strProto, "tcp") == 0)
                        proto = NATProtocol_TCP;
                    else
                    {
                        errorArgument("Invalid proto '%s' specfied for NIC %u", ValueUnion.psz, GetOptState.uIndex);
                        rc = E_FAIL;
                        break;
                    }
                    CHECK_ERROR(engine, AddRedirect(Bstr(strName).raw(), proto,
                                        Bstr(strHostIp).raw(),
                                        RTStrToUInt16(strHostPort),
                                        Bstr(strGuestIp).raw(),
                                        RTStrToUInt16(strGuestPort)));
                }
                else
                {
                    /* delete NAT Rule operation */
                    int vrc;
                    vrc = RTGetOptFetchValue(&GetOptState, &ValueUnion, RTGETOPT_REQ_STRING);
                    if (RT_FAILURE(vrc))
                        return errorSyntax(USAGE_MODIFYVM, "Not enough parameters");
                    CHECK_ERROR(engine, RemoveRedirect(Bstr(ValueUnion.psz).raw()));
                }
                break;
            }
            #undef ITERATE_TO_NEXT_TERM
            case MODIFYVM_NATALIASMODE:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> engine;
                uint32_t aliasMode = 0;

                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));
                if (RTStrCmp(ValueUnion.psz, "default") == 0)
                    aliasMode = 0;
                else
                {
                    char *token = (char *)ValueUnion.psz;
                    while (token)
                    {
                        if (RTStrNCmp(token, RT_STR_TUPLE("log")) == 0)
                            aliasMode |= NATAliasMode_AliasLog;
                        else if (RTStrNCmp(token, RT_STR_TUPLE("proxyonly")) == 0)
                            aliasMode |= NATAliasMode_AliasProxyOnly;
                        else if (RTStrNCmp(token, RT_STR_TUPLE("sameports")) == 0)
                            aliasMode |= NATAliasMode_AliasUseSamePorts;
                        token = RTStrStr(token, ",");
                        if (token == NULL)
                            break;
                        token++;
                    }
                }
                CHECK_ERROR(engine, COMSETTER(AliasMode)(aliasMode));
                break;
            }

            case MODIFYVM_NATTFTPPREFIX:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                ComPtr<INATEngine> engine;
                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));

                CHECK_ERROR(engine, COMSETTER(TFTPPrefix)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_NATTFTPFILE:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                ComPtr<INATEngine> engine;
                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));

                CHECK_ERROR(engine, COMSETTER(TFTPBootFile)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_NATTFTPSERVER:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                ComPtr<INATEngine> engine;
                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));

                CHECK_ERROR(engine, COMSETTER(TFTPNextServer)(Bstr(ValueUnion.psz).raw()));
                break;
            }
            case MODIFYVM_NATDNSPASSDOMAIN:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                ComPtr<INATEngine> engine;
                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));

                CHECK_ERROR(engine, COMSETTER(DNSPassDomain)(ValueUnion.f));
                break;
            }

            case MODIFYVM_NATDNSPROXY:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                ComPtr<INATEngine> engine;
                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));

                CHECK_ERROR(engine, COMSETTER(DNSProxy)(ValueUnion.f));
                break;
            }

            case MODIFYVM_NATDNSHOSTRESOLVER:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                ComPtr<INATEngine> engine;
                CHECK_ERROR(nic, COMGETTER(NATEngine)(engine.asOutParam()));

                CHECK_ERROR(engine, COMSETTER(DNSUseHostResolver)(ValueUnion.f));
                break;
            }
            case MODIFYVM_MACADDRESS:
            {
                if (!parseNum(GetOptState.uIndex, NetworkAdapterCount, "NIC"))
                    break;

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                /* generate one? */
                if (!RTStrICmp(ValueUnion.psz, "auto"))
                {
                    CHECK_ERROR(nic, COMSETTER(MACAddress)(Bstr().raw()));
                }
                else
                {
                    CHECK_ERROR(nic, COMSETTER(MACAddress)(Bstr(ValueUnion.psz).raw()));
                }
                break;
            }

            case MODIFYVM_HIDPTR:
            {
                bool fEnableUsb = false;
                if (!RTStrICmp(ValueUnion.psz, "ps2"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(PointingHIDType)(PointingHIDType_PS2Mouse));
                }
                else if (!RTStrICmp(ValueUnion.psz, "usb"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(PointingHIDType)(PointingHIDType_USBMouse));
                    if (SUCCEEDED(rc))
                        fEnableUsb = true;
                }
                else if (!RTStrICmp(ValueUnion.psz, "usbtablet"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(PointingHIDType)(PointingHIDType_USBTablet));
                    if (SUCCEEDED(rc))
                        fEnableUsb = true;
                }
                else if (!RTStrICmp(ValueUnion.psz, "usbmultitouch"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(PointingHIDType)(PointingHIDType_USBMultiTouch));
                    if (SUCCEEDED(rc))
                        fEnableUsb = true;
                }
                else
                {
                    errorArgument("Invalid type '%s' specfied for pointing device", ValueUnion.psz);
                    rc = E_FAIL;
                }
                if (fEnableUsb)
                {
                    /* Make sure either the OHCI or xHCI controller is enabled. */
                    ULONG cOhciCtrls = 0;
                    ULONG cXhciCtrls = 0;
                    rc = sessionMachine->GetUSBControllerCountByType(USBControllerType_OHCI, &cOhciCtrls);
                    if (SUCCEEDED(rc)) {
                        rc = sessionMachine->GetUSBControllerCountByType(USBControllerType_XHCI, &cXhciCtrls);
                        if (   SUCCEEDED(rc)
                            && cOhciCtrls + cXhciCtrls == 0)
                        {
                            /* If there's nothing, enable OHCI (always available). */
                            ComPtr<IUSBController> UsbCtl;
                            CHECK_ERROR(sessionMachine, AddUSBController(Bstr("OHCI").raw(), USBControllerType_OHCI,
                                                                  UsbCtl.asOutParam()));
                        }
                    }
                }
                break;
            }

            case MODIFYVM_HIDKBD:
            {
                bool fEnableUsb = false;
                if (!RTStrICmp(ValueUnion.psz, "ps2"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(KeyboardHIDType)(KeyboardHIDType_PS2Keyboard));
                }
                else if (!RTStrICmp(ValueUnion.psz, "usb"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(KeyboardHIDType)(KeyboardHIDType_USBKeyboard));
                    if (SUCCEEDED(rc))
                        fEnableUsb = true;
                }
                else
                {
                    errorArgument("Invalid type '%s' specfied for keyboard", ValueUnion.psz);
                    rc = E_FAIL;
                }
                if (fEnableUsb)
                {
                    /* Make sure either the OHCI or xHCI controller is enabled. */
                    ULONG cOhciCtrls = 0;
                    ULONG cXhciCtrls = 0;
                    rc = sessionMachine->GetUSBControllerCountByType(USBControllerType_OHCI, &cOhciCtrls);
                    if (SUCCEEDED(rc)) {
                        rc = sessionMachine->GetUSBControllerCountByType(USBControllerType_XHCI, &cXhciCtrls);
                        if (   SUCCEEDED(rc)
                            && cOhciCtrls + cXhciCtrls == 0)
                        {
                            /* If there's nothing, enable OHCI (always available). */
                            ComPtr<IUSBController> UsbCtl;
                            CHECK_ERROR(sessionMachine, AddUSBController(Bstr("OHCI").raw(), USBControllerType_OHCI,
                                                                  UsbCtl.asOutParam()));
                        }
                    }
                }
                break;
            }

            case MODIFYVM_UARTMODE:
            {
                ComPtr<ISerialPort> uart;

                CHECK_ERROR_BREAK(sessionMachine, GetSerialPort(GetOptState.uIndex - 1, uart.asOutParam()));
                ASSERT(uart);

                if (!RTStrICmp(ValueUnion.psz, "disconnected"))
                {
                    CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_Disconnected));
                }
                else if (   !RTStrICmp(ValueUnion.psz, "server")
                         || !RTStrICmp(ValueUnion.psz, "client")
                         || !RTStrICmp(ValueUnion.psz, "tcpserver")
                         || !RTStrICmp(ValueUnion.psz, "tcpclient")
                         || !RTStrICmp(ValueUnion.psz, "file"))
                {
                    const char *pszMode = ValueUnion.psz;

                    int vrc = RTGetOptFetchValue(&GetOptState, &ValueUnion, RTGETOPT_REQ_STRING);
                    if (RT_FAILURE(vrc))
                        return errorSyntax(USAGE_MODIFYVM,
                                           "Missing or Invalid argument to '%s'",
                                           GetOptState.pDef->pszLong);

                    CHECK_ERROR(uart, COMSETTER(Path)(Bstr(ValueUnion.psz).raw()));

                    if (!RTStrICmp(pszMode, "server"))
                    {
                        CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_HostPipe));
                        CHECK_ERROR(uart, COMSETTER(Server)(TRUE));
                    }
                    else if (!RTStrICmp(pszMode, "client"))
                    {
                        CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_HostPipe));
                        CHECK_ERROR(uart, COMSETTER(Server)(FALSE));
                    }
                    else if (!RTStrICmp(pszMode, "tcpserver"))
                    {
                        CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_TCP));
                        CHECK_ERROR(uart, COMSETTER(Server)(TRUE));
                    }
                    else if (!RTStrICmp(pszMode, "tcpclient"))
                    {
                        CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_TCP));
                        CHECK_ERROR(uart, COMSETTER(Server)(FALSE));
                    }
                    else if (!RTStrICmp(pszMode, "file"))
                    {
                        CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_RawFile));
                    }
                }
                else
                {
                    CHECK_ERROR(uart, COMSETTER(Path)(Bstr(ValueUnion.psz).raw()));
                    CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_HostDevice));
                }
                break;
            }

            case MODIFYVM_UART:
            {
                ComPtr<ISerialPort> uart;

                CHECK_ERROR_BREAK(sessionMachine, GetSerialPort(GetOptState.uIndex - 1, uart.asOutParam()));
                ASSERT(uart);

                if (!RTStrICmp(ValueUnion.psz, "off") || !RTStrICmp(ValueUnion.psz, "disable"))
                    CHECK_ERROR(uart, COMSETTER(Enabled)(FALSE));
                else
                {
                    const char *pszIOBase = ValueUnion.psz;
                    uint32_t uVal = 0;

                    int vrc = RTGetOptFetchValue(&GetOptState, &ValueUnion, RTGETOPT_REQ_UINT32) != MODIFYVM_UART;
                    if (RT_FAILURE(vrc))
                        return errorSyntax(USAGE_MODIFYVM,
                                           "Missing or Invalid argument to '%s'",
                                           GetOptState.pDef->pszLong);

                    CHECK_ERROR(uart, COMSETTER(IRQ)(ValueUnion.u32));

                    vrc = RTStrToUInt32Ex(pszIOBase, NULL, 0, &uVal);
                    if (vrc != VINF_SUCCESS || uVal == 0)
                        return errorArgument("Error parsing UART I/O base '%s'", pszIOBase);
                    CHECK_ERROR(uart, COMSETTER(IOBase)(uVal));

                    CHECK_ERROR(uart, COMSETTER(Enabled)(TRUE));
                }
                break;
            }

#if defined(RT_OS_LINUX) || defined(RT_OS_WINDOWS)
            case MODIFYVM_LPTMODE:
            {
                ComPtr<IParallelPort> lpt;

                CHECK_ERROR_BREAK(sessionMachine, GetParallelPort(GetOptState.uIndex - 1, lpt.asOutParam()));
                ASSERT(lpt);

                CHECK_ERROR(lpt, COMSETTER(Path)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_LPT:
            {
                ComPtr<IParallelPort> lpt;

                CHECK_ERROR_BREAK(sessionMachine, GetParallelPort(GetOptState.uIndex - 1, lpt.asOutParam()));
                ASSERT(lpt);

                if (!RTStrICmp(ValueUnion.psz, "off") || !RTStrICmp(ValueUnion.psz, "disable"))
                    CHECK_ERROR(lpt, COMSETTER(Enabled)(FALSE));
                else
                {
                    const char *pszIOBase = ValueUnion.psz;
                    uint32_t uVal = 0;

                    int vrc = RTGetOptFetchValue(&GetOptState, &ValueUnion, RTGETOPT_REQ_UINT32) != MODIFYVM_LPT;
                    if (RT_FAILURE(vrc))
                        return errorSyntax(USAGE_MODIFYVM,
                                           "Missing or Invalid argument to '%s'",
                                           GetOptState.pDef->pszLong);

                    CHECK_ERROR(lpt, COMSETTER(IRQ)(ValueUnion.u32));

                    vrc = RTStrToUInt32Ex(pszIOBase, NULL, 0, &uVal);
                    if (vrc != VINF_SUCCESS || uVal == 0)
                        return errorArgument("Error parsing LPT I/O base '%s'", pszIOBase);
                    CHECK_ERROR(lpt, COMSETTER(IOBase)(uVal));

                    CHECK_ERROR(lpt, COMSETTER(Enabled)(TRUE));
                }
                break;
            }
#endif

            case MODIFYVM_GUESTMEMORYBALLOON:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(MemoryBalloonSize)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_AUDIOCONTROLLER:
            {
                ComPtr<IAudioAdapter> audioAdapter;
                sessionMachine->COMGETTER(AudioAdapter)(audioAdapter.asOutParam());
                ASSERT(audioAdapter);

                if (!RTStrICmp(ValueUnion.psz, "sb16"))
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioController)(AudioControllerType_SB16));
                else if (!RTStrICmp(ValueUnion.psz, "ac97"))
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioController)(AudioControllerType_AC97));
                else if (!RTStrICmp(ValueUnion.psz, "hda"))
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioController)(AudioControllerType_HDA));
                else
                {
                    errorArgument("Invalid --audiocontroller argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_AUDIOCODEC:
            {
                ComPtr<IAudioAdapter> audioAdapter;
                sessionMachine->COMGETTER(AudioAdapter)(audioAdapter.asOutParam());
                ASSERT(audioAdapter);

                if (!RTStrICmp(ValueUnion.psz, "sb16"))
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioCodec)(AudioCodecType_SB16));
                else if (!RTStrICmp(ValueUnion.psz, "stac9700"))
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioCodec)(AudioCodecType_STAC9700));
                else if (!RTStrICmp(ValueUnion.psz, "ad1980"))
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioCodec)(AudioCodecType_AD1980));
                else if (!RTStrICmp(ValueUnion.psz, "stac9221"))
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioCodec)(AudioCodecType_STAC9221));
                else
                {
                    errorArgument("Invalid --audiocodec argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_AUDIO:
            {
                ComPtr<IAudioAdapter> audioAdapter;
                sessionMachine->COMGETTER(AudioAdapter)(audioAdapter.asOutParam());
                ASSERT(audioAdapter);

                /* disable? */
                if (!RTStrICmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(false));
                }
                else if (!RTStrICmp(ValueUnion.psz, "null"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_Null));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#ifdef RT_OS_WINDOWS
#ifdef VBOX_WITH_WINMM
                else if (!RTStrICmp(ValueUnion.psz, "winmm"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_WinMM));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#endif
                else if (!RTStrICmp(ValueUnion.psz, "dsound"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_DirectSound));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#endif /* RT_OS_WINDOWS */
#ifdef VBOX_WITH_AUDIO_OSS
                else if (!RTStrICmp(ValueUnion.psz, "oss"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_OSS));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#endif
#ifdef VBOX_WITH_AUDIO_ALSA
                else if (!RTStrICmp(ValueUnion.psz, "alsa"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_ALSA));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#endif
#ifdef VBOX_WITH_AUDIO_PULSE
                else if (!RTStrICmp(ValueUnion.psz, "pulse"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_Pulse));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#endif
#ifdef RT_OS_DARWIN
                else if (!RTStrICmp(ValueUnion.psz, "coreaudio"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_CoreAudio));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#endif /* !RT_OS_DARWIN */
                else
                {
                    errorArgument("Invalid --audio argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_AUDIOIN:
            {
                ComPtr<IAudioAdapter> audioAdapter;
                sessionMachine->COMGETTER(AudioAdapter)(audioAdapter.asOutParam());
                ASSERT(audioAdapter);

                CHECK_ERROR(audioAdapter, COMSETTER(EnabledIn)(ValueUnion.f));
                break;
            }

            case MODIFYVM_AUDIOOUT:
            {
                ComPtr<IAudioAdapter> audioAdapter;
                sessionMachine->COMGETTER(AudioAdapter)(audioAdapter.asOutParam());
                ASSERT(audioAdapter);

                CHECK_ERROR(audioAdapter, COMSETTER(EnabledIn)(ValueUnion.f));
                break;
            }

            case MODIFYVM_CLIPBOARD:
            {
                ClipboardMode_T mode = ClipboardMode_Disabled; /* Shut up MSC */
                if (!RTStrICmp(ValueUnion.psz, "disabled"))
                    mode = ClipboardMode_Disabled;
                else if (!RTStrICmp(ValueUnion.psz, "hosttoguest"))
                    mode = ClipboardMode_HostToGuest;
                else if (!RTStrICmp(ValueUnion.psz, "guesttohost"))
                    mode = ClipboardMode_GuestToHost;
                else if (!RTStrICmp(ValueUnion.psz, "bidirectional"))
                    mode = ClipboardMode_Bidirectional;
                else
                {
                    errorArgument("Invalid --clipboard argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                if (SUCCEEDED(rc))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(ClipboardMode)(mode));
                }
                break;
            }

            case MODIFYVM_DRAGANDDROP:
            {
                DnDMode_T mode = DnDMode_Disabled; /* Shut up MSC */
                if (!RTStrICmp(ValueUnion.psz, "disabled"))
                    mode = DnDMode_Disabled;
                else if (!RTStrICmp(ValueUnion.psz, "hosttoguest"))
                    mode = DnDMode_HostToGuest;
                else if (!RTStrICmp(ValueUnion.psz, "guesttohost"))
                    mode = DnDMode_GuestToHost;
                else if (!RTStrICmp(ValueUnion.psz, "bidirectional"))
                    mode = DnDMode_Bidirectional;
                else
                {
                    errorArgument("Invalid --draganddrop argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                if (SUCCEEDED(rc))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(DnDMode)(mode));
                }
                break;
            }

            case MODIFYVM_VRDE_EXTPACK:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                if (vrdeServer)
                {
                    if (RTStrICmp(ValueUnion.psz, "default") != 0)
                    {
                        Bstr bstr(ValueUnion.psz);
                        CHECK_ERROR(vrdeServer, COMSETTER(VRDEExtPack)(bstr.raw()));
                    }
                    else
                        CHECK_ERROR(vrdeServer, COMSETTER(VRDEExtPack)(Bstr().raw()));
                }
                break;
            }

            case MODIFYVM_VRDEPROPERTY:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                if (vrdeServer)
                {
                    /* Parse 'name=value' */
                    char *pszProperty = RTStrDup(ValueUnion.psz);
                    if (pszProperty)
                    {
                        char *pDelimiter = strchr(pszProperty, '=');
                        if (pDelimiter)
                        {
                            *pDelimiter = '\0';

                            Bstr bstrName = pszProperty;
                            Bstr bstrValue = &pDelimiter[1];
                            CHECK_ERROR(vrdeServer, SetVRDEProperty(bstrName.raw(), bstrValue.raw()));
                        }
                        else
                        {
                            RTStrFree(pszProperty);

                            errorArgument("Invalid --vrdeproperty argument '%s'", ValueUnion.psz);
                            rc = E_FAIL;
                            break;
                        }
                        RTStrFree(pszProperty);
                    }
                    else
                    {
                        RTStrmPrintf(g_pStdErr, "Error: Failed to allocate memory for VRDE property '%s'\n", ValueUnion.psz);
                        rc = E_FAIL;
                    }
                }
                break;
            }

            case MODIFYVM_VRDPPORT:
                vrdeWarningDeprecatedOption("port");
                RT_FALL_THRU();

            case MODIFYVM_VRDEPORT:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                if (!RTStrICmp(ValueUnion.psz, "default"))
                    CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("TCP/Ports").raw(), Bstr("0").raw()));
                else
                    CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("TCP/Ports").raw(), Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_VRDPADDRESS:
                vrdeWarningDeprecatedOption("address");
                RT_FALL_THRU();

            case MODIFYVM_VRDEADDRESS:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("TCP/Address").raw(), Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_VRDPAUTHTYPE:
                vrdeWarningDeprecatedOption("authtype");
                RT_FALL_THRU();
            case MODIFYVM_VRDEAUTHTYPE:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                if (!RTStrICmp(ValueUnion.psz, "null"))
                {
                    CHECK_ERROR(vrdeServer, COMSETTER(AuthType)(AuthType_Null));
                }
                else if (!RTStrICmp(ValueUnion.psz, "external"))
                {
                    CHECK_ERROR(vrdeServer, COMSETTER(AuthType)(AuthType_External));
                }
                else if (!RTStrICmp(ValueUnion.psz, "guest"))
                {
                    CHECK_ERROR(vrdeServer, COMSETTER(AuthType)(AuthType_Guest));
                }
                else
                {
                    errorArgument("Invalid --vrdeauthtype argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_VRDEAUTHLIBRARY:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                if (vrdeServer)
                {
                    if (RTStrICmp(ValueUnion.psz, "default") != 0)
                    {
                        Bstr bstr(ValueUnion.psz);
                        CHECK_ERROR(vrdeServer, COMSETTER(AuthLibrary)(bstr.raw()));
                    }
                    else
                        CHECK_ERROR(vrdeServer, COMSETTER(AuthLibrary)(Bstr().raw()));
                }
                break;
            }

            case MODIFYVM_VRDPMULTICON:
                vrdeWarningDeprecatedOption("multicon");
                RT_FALL_THRU();
            case MODIFYVM_VRDEMULTICON:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, COMSETTER(AllowMultiConnection)(ValueUnion.f));
                break;
            }

            case MODIFYVM_VRDPREUSECON:
                vrdeWarningDeprecatedOption("reusecon");
                RT_FALL_THRU();
            case MODIFYVM_VRDEREUSECON:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, COMSETTER(ReuseSingleConnection)(ValueUnion.f));
                break;
            }

            case MODIFYVM_VRDPVIDEOCHANNEL:
                vrdeWarningDeprecatedOption("videochannel");
                RT_FALL_THRU();
            case MODIFYVM_VRDEVIDEOCHANNEL:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("VideoChannel/Enabled").raw(),
                                                        ValueUnion.f? Bstr("true").raw():  Bstr("false").raw()));
                break;
            }

            case MODIFYVM_VRDPVIDEOCHANNELQUALITY:
                vrdeWarningDeprecatedOption("videochannelquality");
                RT_FALL_THRU();
            case MODIFYVM_VRDEVIDEOCHANNELQUALITY:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("VideoChannel/Quality").raw(),
                                                        Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_VRDP:
                vrdeWarningDeprecatedOption("");
                RT_FALL_THRU();
            case MODIFYVM_VRDE:
            {
                ComPtr<IVRDEServer> vrdeServer;
                sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, COMSETTER(Enabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_USBRENAME:
            {
                const char *pszName = ValueUnion.psz;
                int vrc = RTGetOptFetchValue(&GetOptState, &ValueUnion, RTGETOPT_REQ_STRING);
                if (RT_FAILURE(vrc))
                    return errorSyntax(USAGE_MODIFYVM,
                                       "Missing or Invalid argument to '%s'",
                                       GetOptState.pDef->pszLong);
                const char *pszNewName = ValueUnion.psz;

                SafeIfaceArray<IUSBController> ctrls;
                CHECK_ERROR(sessionMachine, COMGETTER(USBControllers)(ComSafeArrayAsOutParam(ctrls)));
                bool fRenamed = false;
                for (size_t i = 0; i < ctrls.size(); i++)
                {
                    ComPtr<IUSBController> pCtrl = ctrls[i];
                    Bstr bstrName;
                    CHECK_ERROR(pCtrl, COMGETTER(Name)(bstrName.asOutParam()));
                    if (bstrName == pszName)
                    {
                        bstrName = pszNewName;
                        CHECK_ERROR(pCtrl, COMSETTER(Name)(bstrName.raw()));
                        fRenamed = true;
                    }
                }
                if (!fRenamed)
                {
                    errorArgument("Invalid --usbrename parameters, nothing renamed");
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_USBXHCI:
            {
                ULONG cXhciCtrls = 0;
                rc = sessionMachine->GetUSBControllerCountByType(USBControllerType_XHCI, &cXhciCtrls);
                if (SUCCEEDED(rc))
                {
                    if (!cXhciCtrls && ValueUnion.f)
                    {
                        ComPtr<IUSBController> UsbCtl;
                        CHECK_ERROR(sessionMachine, AddUSBController(Bstr("xHCI").raw(), USBControllerType_XHCI,
                                                              UsbCtl.asOutParam()));
                    }
                    else if (cXhciCtrls && !ValueUnion.f)
                    {
                        SafeIfaceArray<IUSBController> ctrls;
                        CHECK_ERROR(sessionMachine, COMGETTER(USBControllers)(ComSafeArrayAsOutParam(ctrls)));
                        for (size_t i = 0; i < ctrls.size(); i++)
                        {
                            ComPtr<IUSBController> pCtrl = ctrls[i];
                            USBControllerType_T enmType;
                            CHECK_ERROR(pCtrl, COMGETTER(Type)(&enmType));
                            if (enmType == USBControllerType_XHCI)
                            {
                                Bstr ctrlName;
                                CHECK_ERROR(pCtrl, COMGETTER(Name)(ctrlName.asOutParam()));
                                CHECK_ERROR(sessionMachine, RemoveUSBController(ctrlName.raw()));
                            }
                        }
                    }
                }
                break;
            }

            case MODIFYVM_USBEHCI:
            {
                ULONG cEhciCtrls = 0;
                rc = sessionMachine->GetUSBControllerCountByType(USBControllerType_EHCI, &cEhciCtrls);
                if (SUCCEEDED(rc))
                {
                    if (!cEhciCtrls && ValueUnion.f)
                    {
                        ComPtr<IUSBController> UsbCtl;
                        CHECK_ERROR(sessionMachine, AddUSBController(Bstr("EHCI").raw(), USBControllerType_EHCI,
                                                              UsbCtl.asOutParam()));
                    }
                    else if (cEhciCtrls && !ValueUnion.f)
                    {
                        SafeIfaceArray<IUSBController> ctrls;
                        CHECK_ERROR(sessionMachine, COMGETTER(USBControllers)(ComSafeArrayAsOutParam(ctrls)));
                        for (size_t i = 0; i < ctrls.size(); i++)
                        {
                            ComPtr<IUSBController> pCtrl = ctrls[i];
                            USBControllerType_T enmType;
                            CHECK_ERROR(pCtrl, COMGETTER(Type)(&enmType));
                            if (enmType == USBControllerType_EHCI)
                            {
                                Bstr ctrlName;
                                CHECK_ERROR(pCtrl, COMGETTER(Name)(ctrlName.asOutParam()));
                                CHECK_ERROR(sessionMachine, RemoveUSBController(ctrlName.raw()));
                            }
                        }
                    }
                }
                break;
            }

            case MODIFYVM_USB:
            {
                ULONG cOhciCtrls = 0;
                rc = sessionMachine->GetUSBControllerCountByType(USBControllerType_OHCI, &cOhciCtrls);
                if (SUCCEEDED(rc))
                {
                    if (!cOhciCtrls && ValueUnion.f)
                    {
                        ComPtr<IUSBController> UsbCtl;
                        CHECK_ERROR(sessionMachine, AddUSBController(Bstr("OHCI").raw(), USBControllerType_OHCI,
                                                              UsbCtl.asOutParam()));
                    }
                    else if (cOhciCtrls && !ValueUnion.f)
                    {
                        SafeIfaceArray<IUSBController> ctrls;
                        CHECK_ERROR(sessionMachine, COMGETTER(USBControllers)(ComSafeArrayAsOutParam(ctrls)));
                        for (size_t i = 0; i < ctrls.size(); i++)
                        {
                            ComPtr<IUSBController> pCtrl = ctrls[i];
                            USBControllerType_T enmType;
                            CHECK_ERROR(pCtrl, COMGETTER(Type)(&enmType));
                            if (enmType == USBControllerType_OHCI)
                            {
                                Bstr ctrlName;
                                CHECK_ERROR(pCtrl, COMGETTER(Name)(ctrlName.asOutParam()));
                                CHECK_ERROR(sessionMachine, RemoveUSBController(ctrlName.raw()));
                            }
                        }
                    }
                }
                break;
            }

            case MODIFYVM_SNAPSHOTFOLDER:
            {
                if (!RTStrICmp(ValueUnion.psz, "default"))
                    CHECK_ERROR(sessionMachine, COMSETTER(SnapshotFolder)(Bstr().raw()));
                else
                    CHECK_ERROR(sessionMachine, COMSETTER(SnapshotFolder)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_TELEPORTER_ENABLED:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(TeleporterEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_TELEPORTER_PORT:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(TeleporterPort)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_TELEPORTER_ADDRESS:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(TeleporterAddress)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_TELEPORTER_PASSWORD:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(TeleporterPassword)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_TELEPORTER_PASSWORD_FILE:
            {
                Utf8Str password;
                RTEXITCODE rcExit = readPasswordFile(ValueUnion.psz, &password);
                if (rcExit != RTEXITCODE_SUCCESS)
                    rc = E_FAIL;
                else
                    CHECK_ERROR(sessionMachine, COMSETTER(TeleporterPassword)(Bstr(password).raw()));
                break;
            }

            case MODIFYVM_TRACING_ENABLED:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(TracingEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_TRACING_CONFIG:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(TracingConfig)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_TRACING_ALLOW_VM_ACCESS:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(AllowTracingToAccessVM)(ValueUnion.f));
                break;
            }

            case MODIFYVM_FAULT_TOLERANCE:
            {
                if (!RTStrICmp(ValueUnion.psz, "master"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(FaultToleranceState(FaultToleranceState_Master)));
                }
                else
                if (!RTStrICmp(ValueUnion.psz, "standby"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(FaultToleranceState(FaultToleranceState_Standby)));
                }
                else
                {
                    errorArgument("Invalid --faulttolerance argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_FAULT_TOLERANCE_ADDRESS:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(FaultToleranceAddress)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_FAULT_TOLERANCE_PORT:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(FaultTolerancePort)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_FAULT_TOLERANCE_PASSWORD:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(FaultTolerancePassword)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_FAULT_TOLERANCE_SYNC_INTERVAL:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(FaultToleranceSyncInterval)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_HARDWARE_UUID:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(HardwareUUID)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_HPET:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(HPETEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_IOCACHE:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(IOCacheEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_IOCACHESIZE:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(IOCacheSize)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_CHIPSET:
            {
                if (!RTStrICmp(ValueUnion.psz, "piix3"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(ChipsetType)(ChipsetType_PIIX3));
                }
                else if (!RTStrICmp(ValueUnion.psz, "ich9"))
                {
                    CHECK_ERROR(sessionMachine, COMSETTER(ChipsetType)(ChipsetType_ICH9));
                    BOOL fIoApic = FALSE;
                    CHECK_ERROR(biosSettings, COMGETTER(IOAPICEnabled)(&fIoApic));
                    if (!fIoApic)
                    {
                        RTStrmPrintf(g_pStdErr, "*** I/O APIC must be enabled for ICH9, enabling. ***\n");
                        CHECK_ERROR(biosSettings, COMSETTER(IOAPICEnabled)(TRUE));
                    }
                }
                else
                {
                    errorArgument("Invalid --chipset argument '%s' (valid: piix3,ich9)", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }
#ifdef VBOX_WITH_VIDEOREC
            case MODIFYVM_VIDEOCAP:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureEnabled)(ValueUnion.f));
                break;
            }
            case MODIFYVM_VIDEOCAP_SCREENS:
            {
                ULONG cMonitors = 64;
                CHECK_ERROR(sessionMachine, COMGETTER(MonitorCount)(&cMonitors));
                com::SafeArray<BOOL> screens(cMonitors);
                if (parseScreens(ValueUnion.psz, &screens))
                {
                    errorArgument("Invalid list of screens specified\n");
                    rc = E_FAIL;
                    break;
                }
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureScreens)(ComSafeArrayAsInParam(screens)));
                break;
            }
            case MODIFYVM_VIDEOCAP_FILENAME:
            {
                Bstr bstr;
                /* empty string will fall through, leaving bstr empty */
                if (*ValueUnion.psz)
                {
                    char szVCFileAbs[RTPATH_MAX] = "";
                    int vrc = RTPathAbs(ValueUnion.psz, szVCFileAbs, sizeof(szVCFileAbs));
                    if (RT_FAILURE(vrc))
                    {
                        errorArgument("Cannot convert filename \"%s\" to absolute path\n", ValueUnion.psz);
                        rc = E_FAIL;
                        break;
                    }
                    bstr = szVCFileAbs;
                }
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureFile)(bstr.raw()));
                break;
            }
            case MODIFYVM_VIDEOCAP_WIDTH:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureWidth)(ValueUnion.u32));
                break;
            }
            case MODIFYVM_VIDEOCAP_HEIGHT:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureHeight)(ValueUnion.u32));
                break;
            }
            case MODIFYVM_VIDEOCAP_RES:
            {
                uint32_t uWidth = 0;
                char *pszNext;
                int vrc = RTStrToUInt32Ex(ValueUnion.psz, &pszNext, 0, &uWidth);
                if (RT_FAILURE(vrc) || vrc != VWRN_TRAILING_CHARS || !pszNext || *pszNext != 'x')
                {
                    errorArgument("Error parsing geomtry '%s' (expected <width>x<height>)", ValueUnion.psz);
                    rc = E_FAIL;
                    break;
                }
                uint32_t uHeight = 0;
                vrc = RTStrToUInt32Ex(pszNext+1, NULL, 0, &uHeight);
                if (vrc != VINF_SUCCESS)
                {
                    errorArgument("Error parsing geomtry '%s' (expected <width>x<height>)", ValueUnion.psz);
                    rc = E_FAIL;
                    break;
                }
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureWidth)(uWidth));
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureHeight)(uHeight));
                break;
            }
            case MODIFYVM_VIDEOCAP_RATE:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureRate)(ValueUnion.u32));
                break;
            }
            case MODIFYVM_VIDEOCAP_FPS:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureFPS)(ValueUnion.u32));
                break;
            }
            case MODIFYVM_VIDEOCAP_MAXTIME:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureMaxTime)(ValueUnion.u32));
                break;
            }
            case MODIFYVM_VIDEOCAP_MAXSIZE:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureMaxFileSize)(ValueUnion.u32));
                break;
            }
            case MODIFYVM_VIDEOCAP_OPTIONS:
            {
                Bstr bstr(ValueUnion.psz);
                CHECK_ERROR(sessionMachine, COMSETTER(VideoCaptureOptions)(bstr.raw()));
                break;
            }
#endif
            case MODIFYVM_AUTOSTART_ENABLED:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(AutostartEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_AUTOSTART_DELAY:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(AutostartDelay)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_AUTOSTOP_TYPE:
            {
                AutostopType_T enmAutostopType = AutostopType_Disabled;

                if (!RTStrICmp(ValueUnion.psz, "disabled"))
                    enmAutostopType = AutostopType_Disabled;
                else if (!RTStrICmp(ValueUnion.psz, "savestate"))
                    enmAutostopType = AutostopType_SaveState;
                else if (!RTStrICmp(ValueUnion.psz, "poweroff"))
                    enmAutostopType = AutostopType_PowerOff;
                else if (!RTStrICmp(ValueUnion.psz, "acpishutdown"))
                    enmAutostopType = AutostopType_AcpiShutdown;
                else
                {
                    errorArgument("Invalid --autostop-type argument '%s' (valid: disabled, savestate, poweroff, acpishutdown)", ValueUnion.psz);
                    rc = E_FAIL;
                }

                if (SUCCEEDED(rc))
                    CHECK_ERROR(sessionMachine, COMSETTER(AutostopType)(enmAutostopType));
                break;
            }
#ifdef VBOX_WITH_PCI_PASSTHROUGH
            case MODIFYVM_ATTACH_PCI:
            {
                const char* pAt = strchr(ValueUnion.psz, '@');
                int32_t iHostAddr, iGuestAddr;

                iHostAddr = parsePci(ValueUnion.psz);
                iGuestAddr = pAt != NULL ? parsePci(pAt + 1) : iHostAddr;

                if (iHostAddr == -1 || iGuestAddr == -1)
                {
                    errorArgument("Invalid --pciattach argument '%s' (valid: 'HB:HD.HF@GB:GD.GF' or just 'HB:HD.HF')", ValueUnion.psz);
                    rc = E_FAIL;
                }
                else
                {
                    CHECK_ERROR(sessionMachine, AttachHostPCIDevice(iHostAddr, iGuestAddr, TRUE));
                }

                break;
            }
            case MODIFYVM_DETACH_PCI:
            {
                int32_t iHostAddr;

                iHostAddr = parsePci(ValueUnion.psz);
                if (iHostAddr == -1)
                {
                    errorArgument("Invalid --pcidetach argument '%s' (valid: 'HB:HD.HF')", ValueUnion.psz);
                    rc = E_FAIL;
                }
                else
                {
                    CHECK_ERROR(sessionMachine, DetachHostPCIDevice(iHostAddr));
                }

                break;
            }
#endif

#ifdef VBOX_WITH_USB_CARDREADER
            case MODIFYVM_USBCARDREADER:
            {
                CHECK_ERROR(sessionMachine, COMSETTER(EmulatedUSBCardReaderEnabled)(ValueUnion.f));
                break;
            }
#endif /* VBOX_WITH_USB_CARDREADER */

            case MODIFYVM_DEFAULTFRONTEND:
            {
                Bstr bstr(ValueUnion.psz);
                if (bstr == "default")
                    bstr = Bstr::Empty;
                CHECK_ERROR(sessionMachine, COMSETTER(DefaultFrontend)(bstr.raw()));
                break;
            }

            default:
            {
                errorGetOpt(USAGE_MODIFYVM, c, &ValueUnion);
                rc = E_FAIL;
                break;
            }
        }
    }

    /* commit changes */
    if (SUCCEEDED(rc))
        CHECK_ERROR(sessionMachine, SaveSettings());

    /* it's important to always close sessions */
    a->session->UnlockMachine();

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

#endif /* !VBOX_ONLY_DOCS */
