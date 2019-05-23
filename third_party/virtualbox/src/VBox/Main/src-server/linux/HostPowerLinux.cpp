/** @file
 *
 * VirtualBox interface to host's power notification service
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "HostPower.h"
#include "Logging.h"

#include <iprt/asm.h>
#include <iprt/power.h>
#include <iprt/time.h>

static bool checkDBusError(DBusError *pError, DBusConnection **pConnection)
{
    if (dbus_error_is_set(pError))
    {
        LogRel(("HostPowerServiceLinux: DBus connection Error (%s)\n", pError->message));
        dbus_error_free(pError);
        if (*pConnection)
        {
            /* Close the socket or whatever underlying the connection. */
            dbus_connection_close(*pConnection);
            /* Free in-process resources used for the now-closed connection. */
            dbus_connection_unref(*pConnection);
            *pConnection = NULL;
        }
        return true;
    }
    return false;
}

HostPowerServiceLinux::HostPowerServiceLinux(VirtualBox *aVirtualBox)
  : HostPowerService(aVirtualBox)
  , mThread(NULL)
  , mpConnection(NULL)
{
    DBusError error;
    int rc;

    rc = RTDBusLoadLib();
    if (RT_FAILURE(rc))
    {
        LogRel(("HostPowerServiceLinux: DBus library not found.  Service not available.\n"));
        return;
    }
    dbus_error_init(&error);
    /* Connect to the DBus.  The connection will be not shared with any other
     * in-process callers of dbus_bus_get().  This is considered wasteful (see
     * API documentation) but simplifies our code, specifically shutting down.
     * The session bus allows up to 100000 connections per user as it "is just
     * running as the user anyway" (see session.conf.in in the DBus sources). */
    mpConnection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);
    if (checkDBusError(&error, &mpConnection))
        return;
    /* We do not want to exit(1) if the connection is broken. */
    dbus_connection_set_exit_on_disconnect(mpConnection, FALSE);
    /* Tell the bus to wait for the sleep signal(s). */
    /* The current systemd-logind interface. */
    dbus_bus_add_match(mpConnection, "type='signal',interface='org.freedesktop.login1.Manager'", &error);
    /* The previous UPower interfaces (2010 - ca 2013). */
    dbus_bus_add_match(mpConnection, "type='signal',interface='org.freedesktop.UPower'", &error);
    dbus_connection_flush(mpConnection);
    if (checkDBusError(&error, &mpConnection))
        return;
    /* Create the new worker thread. */
    rc = RTThreadCreate(&mThread, HostPowerServiceLinux::powerChangeNotificationThread, this, 0 /* cbStack */,
                        RTTHREADTYPE_MSG_PUMP, RTTHREADFLAGS_WAITABLE, "MainPower");
    if (RT_FAILURE(rc))
        LogRel(("HostPowerServiceLinux: RTThreadCreate failed with %Rrc\n", rc));
}


HostPowerServiceLinux::~HostPowerServiceLinux()
{
    int rc;
    RTMSINTERVAL cMillies = 5000;

    /* Closing the connection should cause the event loop to exit. */
    LogFunc((": Stopping thread\n"));
    if (mpConnection)
        dbus_connection_close(mpConnection);

    rc = RTThreadWait(mThread, cMillies, NULL);
    if (rc != VINF_SUCCESS)
        LogRelThisFunc(("RTThreadWait() for %u ms failed with %Rrc\n", cMillies, rc));
    mThread = NIL_RTTHREAD;
}


DECLCALLBACK(int) HostPowerServiceLinux::powerChangeNotificationThread(RTTHREAD hThreadSelf, void *pInstance)
{
    NOREF(hThreadSelf);
    HostPowerServiceLinux *pPowerObj = static_cast<HostPowerServiceLinux *>(pInstance);
    DBusConnection *pConnection = pPowerObj->mpConnection;

    Log(("HostPowerServiceLinux: Thread started\n"));
    while (dbus_connection_read_write(pConnection, -1))
    {
        DBusMessage *pMessage = NULL;

        for (;;)
        {
            DBusMessageIter args;
            dbus_bool_t fSuspend;

            pMessage = dbus_connection_pop_message(pConnection);
            if (pMessage == NULL)
                break;
            /* The systemd-logind interface notification. */
            if (   dbus_message_is_signal(pMessage, "org.freedesktop.login1.Manager", "PrepareForSleep")
                && dbus_message_iter_init(pMessage, &args)
                && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_BOOLEAN)
            {
                dbus_message_iter_get_basic(&args, &fSuspend);
                /* Trinary operator does not work here as Reason_... is an
                 * anonymous enum. */
                if (fSuspend)
                    pPowerObj->notify(Reason_HostSuspend);
                else
                    pPowerObj->notify(Reason_HostResume);
            }
            /* The UPowerd interface notifications.  Sleeping is the older one,
             * NotifySleep the newer.  This gives us one second grace before the
             * suspend triggers. */
            if (   dbus_message_is_signal(pMessage, "org.freedesktop.UPower", "Sleeping")
                || dbus_message_is_signal(pMessage, "org.freedesktop.UPower", "NotifySleep"))
                pPowerObj->notify(Reason_HostSuspend);
            if (   dbus_message_is_signal(pMessage, "org.freedesktop.UPower", "Resuming")
                || dbus_message_is_signal(pMessage, "org.freedesktop.UPower", "NotifyResume"))
                pPowerObj->notify(Reason_HostResume);
            /* Free local resources held for the message. */
            dbus_message_unref(pMessage);
        }
    }
    /* Close the socket or whatever underlying the connection. */
    dbus_connection_close(pConnection);
    /* Free in-process resources used for the now-closed connection. */
    dbus_connection_unref(pConnection);
    Log(("HostPowerServiceLinux: Exiting thread\n"));
    return VINF_SUCCESS;
}

