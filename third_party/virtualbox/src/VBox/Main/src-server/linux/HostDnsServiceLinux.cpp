/* $Id: HostDnsServiceLinux.cpp $ */
/** @file
 * Linux specific DNS information fetching.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/file.h>
#include <iprt/log.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>

#include <errno.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>

#include <linux/limits.h>

/* Workaround for <sys/cdef.h> defining __flexarr to [] which beats us in
 * struct inotify_event (char name __flexarr). */
#include <sys/cdefs.h>
#undef __flexarr
#define __flexarr [0]
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string>
#include <vector>
#include "../HostDnsService.h"


static int g_DnsMonitorStop[2];

static const std::string g_EtcFolder = "/etc";
static const std::string g_ResolvConf = "resolv.conf";
static const std::string g_ResolvConfFullPath = "/etc/resolv.conf";

class FileDescriptor
{
    public:
    FileDescriptor(int d = -1):fd(d){}

    virtual ~FileDescriptor() {
        if (fd != -1)
            close(fd);
    }

    int fileDescriptor() const {return fd;}

    protected:
    int fd;
};


class AutoNotify:public FileDescriptor
{
    public:
    AutoNotify()
    {
        FileDescriptor::fd = inotify_init();
        AssertReturnVoid(FileDescriptor::fd != -1);
    }
};

struct InotifyEventWithName
{
    struct inotify_event e;
    char name[NAME_MAX];
};

HostDnsServiceLinux::~HostDnsServiceLinux()
{
    monitorThreadShutdown();
}


int HostDnsServiceLinux::monitorWorker()
{

    AutoNotify a;

    int rc = socketpair(AF_LOCAL, SOCK_DGRAM, 0, g_DnsMonitorStop);
    AssertMsgReturn(rc == 0, ("socketpair: failed (%d: %s)\n", errno, strerror(errno)), E_FAIL);

    FileDescriptor stopper0(g_DnsMonitorStop[0]);
    FileDescriptor stopper1(g_DnsMonitorStop[1]);

    pollfd polls[2];
    RT_ZERO(polls);

    polls[0].fd = a.fileDescriptor();
    polls[0].events = POLLIN;

    polls[1].fd = g_DnsMonitorStop[1];
    polls[1].events = POLLIN;

    monitorThreadInitializationDone();

    int wd[2];
    wd[0] = wd[1] = -1;
    /* inotify inialization */
    wd[0] = inotify_add_watch(a.fileDescriptor(),
                              g_ResolvConfFullPath.c_str(), IN_CLOSE_WRITE|IN_DELETE_SELF);

    /**
     * If /etc/resolv.conf exists we want to listen for movements: because
     * # mv /etc/resolv.conf ...
     * won't arm IN_DELETE_SELF on wd[0] instead it will fire IN_MOVE_FROM on wd[1].
     *
     * Because on some distributions /etc/resolv.conf is link, wd[0] can't detect deletion,
     * it's recognizible on directory level (wd[1]) only.
     */
    wd[1] = inotify_add_watch(a.fileDescriptor(), g_EtcFolder.c_str(),
                              wd[0] == -1 ? IN_MOVED_TO|IN_CREATE : IN_MOVED_FROM|IN_DELETE);

    struct InotifyEventWithName combo;
    while(true)
    {
        rc = poll(polls, 2, -1);
        if (rc == -1)
            continue;

        AssertMsgReturn(   ((polls[0].revents & (POLLERR|POLLNVAL)) == 0)
                        && ((polls[1].revents & (POLLERR|POLLNVAL)) == 0),
                           ("Debug Me"), VERR_INTERNAL_ERROR);

        if (polls[1].revents & POLLIN)
            return VINF_SUCCESS; /* time to shutdown */

        if (polls[0].revents & POLLIN)
        {
            RT_ZERO(combo);
            ssize_t r = read(polls[0].fd, static_cast<void *>(&combo), sizeof(combo));
            NOREF(r);

            if (combo.e.wd == wd[0])
            {
                if (combo.e.mask & IN_CLOSE_WRITE)
                {
                    readResolvConf();
                }
                else if (combo.e.mask & IN_DELETE_SELF)
                {
                    inotify_rm_watch(a.fileDescriptor(), wd[0]); /* removes file watcher */
                    inotify_add_watch(a.fileDescriptor(), g_EtcFolder.c_str(),
                                      IN_MOVED_TO|IN_CREATE); /* alter folder watcher */
                }
                else if (combo.e.mask & IN_IGNORED)
                {
                    wd[0] = -1; /* we want receive any events on this watch */
                }
                else
                {
                    /**
                     * It shouldn't happen, in release we will just ignore in debug
                     * we will have to chance to look at into inotify_event
                     */
                    AssertMsgFailed(("Debug Me!!!"));
                }
            }
            else if (combo.e.wd == wd[1])
            {
                if (   combo.e.mask & IN_MOVED_FROM
                    || combo.e.mask & IN_DELETE)
                {
                    if (g_ResolvConf == combo.e.name)
                    {
                        /**
                         * Our file has been moved so we should change watching mode.
                         */
                        inotify_rm_watch(a.fileDescriptor(), wd[0]);
                        wd[1] = inotify_add_watch(a.fileDescriptor(), g_EtcFolder.c_str(),
                                                  IN_MOVED_TO|IN_CREATE);
                        AssertMsg(wd[1] != -1,
                                  ("It shouldn't happen, further investigation is needed\n"));
                    }
                }
                else
                {
                    AssertMsg(combo.e.mask & (IN_MOVED_TO|IN_CREATE),
                              ("%RX32 event isn't expected, we are waiting for IN_MOVED|IN_CREATE\n",
                               combo.e.mask));
                    if (g_ResolvConf == combo.e.name)
                    {
                        AssertMsg(wd[0] == -1, ("We haven't removed file watcher first\n"));

                        /* alter folder watcher*/
                        wd[1] = inotify_add_watch(a.fileDescriptor(), g_EtcFolder.c_str(),
                                                  IN_MOVED_FROM|IN_DELETE);
                        AssertMsg(wd[1] != -1, ("It shouldn't happen.\n"));

                        wd[0] = inotify_add_watch(a.fileDescriptor(),
                                                  g_ResolvConfFullPath.c_str(),
                                                  IN_CLOSE_WRITE | IN_DELETE_SELF);
                        AssertMsg(wd[0] != -1, ("Adding watcher to file (%s) has been failed!\n",
                                                g_ResolvConfFullPath.c_str()));

                        /* Notify our listeners */
                        readResolvConf();
                    }
                }
            }
            else
            {
                /* It shouldn't happen */
                AssertMsgFailed(("Shouldn't happen! Please debug me!"));
            }
        }
    }
}


void HostDnsServiceLinux::monitorThreadShutdown()
{
    send(g_DnsMonitorStop[0], "", 1, 0);
}
