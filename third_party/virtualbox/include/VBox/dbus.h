/** @file
 * Module to dynamically load libdbus and load all symbols which are needed by
 * VirtualBox.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___VBox_dbus_h
#define ___VBox_dbus_h

#include <iprt/types.h>
#include <iprt/stdarg.h>

#ifndef __cplusplus
# error "This header requires C++ to avoid name clashes."
#endif

/** Types and defines from the dbus header files which we need.  These are
 * taken more or less verbatim from the DBus public interface header files. */
struct DBusError
{
    const char *name;
    const char *message;
    unsigned int dummy1 : 1;
    unsigned int dummy2 : 1;
    unsigned int dummy3 : 1;
    unsigned int dummy4 : 1;
    unsigned int dummy5 : 1;
    void *padding1;
};
typedef struct DBusError DBusError;

struct DBusConnection;
typedef struct DBusConnection DBusConnection;

typedef uint32_t dbus_bool_t;
typedef uint32_t dbus_uint32_t;
typedef enum { DBUS_BUS_SESSION, DBUS_BUS_SYSTEM, DBUS_BUS_STARTER } DBusBusType;

struct DBusMessage;
typedef struct DBusMessage DBusMessage;

struct DBusMessageIter
{
    void *dummy1;
    void *dummy2;
    dbus_uint32_t dummy3;
    int dummy4;
    int dummy5;
    int dummy6;
    int dummy7;
    int dummy8;
    int dummy9;
    int dummy10;
    int dummy11;
    int pad1;
    int pad2;
    void *pad3;
};
typedef struct DBusMessageIter DBusMessageIter;

#define DBUS_ERROR_NO_MEMORY                "org.freedesktop.DBus.Error.NoMemory"

/* Message types. */
#define DBUS_MESSAGE_TYPE_INVALID           0
#define DBUS_MESSAGE_TYPE_METHOD_CALL       1
#define DBUS_MESSAGE_TYPE_METHOD_RETURN     2
#define DBUS_MESSAGE_TYPE_ERROR             3
#define DBUS_MESSAGE_TYPE_SIGNAL            4

/* Primitive types. */
#define DBUS_TYPE_INVALID                   ((int) '\0')
#define DBUS_TYPE_BOOLEAN                   ((int) 'b')
#define DBUS_TYPE_INT32                     ((int) 'i')
#define DBUS_TYPE_UINT32                    ((int) 'u')
#define DBUS_TYPE_STRING                    ((int) 's')
#define DBUS_TYPE_STRING_AS_STRING          "s"

/* Compound types. */
#define DBUS_TYPE_OBJECT_PATH               ((int) 'o')
#define DBUS_TYPE_ARRAY                     ((int) 'a')
#define DBUS_TYPE_ARRAY_AS_STRING           "a"
#define DBUS_TYPE_DICT_ENTRY                ((int) 'e')
#define DBUS_TYPE_DICT_ENTRY_AS_STRING      "e"

typedef enum
{
  DBUS_HANDLER_RESULT_HANDLED,
  DBUS_HANDLER_RESULT_NOT_YET_HANDLED,
  DBUS_HANDLER_RESULT_NEED_MEMORY
} DBusHandlerResult;

typedef DBusHandlerResult (* DBusHandleMessageFunction)(DBusConnection *,
                                                        DBusMessage *, void *);
typedef void (* DBusFreeFunction) (void *);

/* Declarations of the functions that we need from libdbus-1 */
#define VBOX_DBUS_GENERATE_HEADER

#include <VBox/dbus-calls.h>

#undef VBOX_DBUS_GENERATE_HEADER

#endif /* ___VBox_DBus_h not defined */

