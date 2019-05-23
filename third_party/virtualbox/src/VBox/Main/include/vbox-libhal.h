/* $Id: vbox-libhal.h $ */
/** @file
 *
 * Module to dynamically load libhal and libdbus and load all symbols
 * which are needed by VirtualBox.
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

#ifndef ____H_VBOX_LIBHAL
#define ____H_VBOX_LIBHAL

#include <stdint.h>

#define LIB_HAL "libhal.so.1"

/** Types from the dbus and hal header files which we need.  These are taken more or less
    verbatim from the DBus and Hal public interface header files. */
struct DBusError
{
    const char *name;
    const char *message;
    unsigned int dummy1 : 1; /**< placeholder */
    unsigned int dummy2 : 1; /**< placeholder */
    unsigned int dummy3 : 1; /**< placeholder */
    unsigned int dummy4 : 1; /**< placeholder */
    unsigned int dummy5 : 1; /**< placeholder */
    void *padding1; /**< placeholder */
};
struct DBusConnection;
typedef struct DBusConnection DBusConnection;
typedef uint32_t dbus_bool_t;
typedef enum { DBUS_BUS_SESSON, DBUS_BUS_SYSTEM, DBUS_BUS_STARTER } DBusBusType;
struct LibHalContext_s;
typedef struct LibHalContext_s LibHalContext;

/** The following are the symbols which we need from libdbus and libhal. */
extern void (*gDBusErrorInit)(DBusError *);
extern DBusConnection *(*gDBusBusGet)(DBusBusType, DBusError *);
extern void (*gDBusErrorFree)(DBusError *);
extern void (*gDBusConnectionUnref)(DBusConnection *);
extern LibHalContext *(*gLibHalCtxNew)(void);
extern dbus_bool_t (*gLibHalCtxSetDBusConnection)(LibHalContext *, DBusConnection *);
extern dbus_bool_t (*gLibHalCtxInit)(LibHalContext *, DBusError *);
extern char **(*gLibHalFindDeviceStringMatch)(LibHalContext *, const char *, const char *, int *,
               DBusError *);
extern char *(*gLibHalDeviceGetPropertyString)(LibHalContext *, const char *, const char *,
                                              DBusError *);
extern void (*gLibHalFreeString)(char *);
extern void (*gLibHalFreeStringArray)(char **);
extern dbus_bool_t (*gLibHalCtxShutdown)(LibHalContext *, DBusError *);
extern dbus_bool_t (*gLibHalCtxFree)(LibHalContext *);

extern bool gLibHalCheckPresence(void);

#endif /* ____H_VBOX_LIBHAL not defined */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
