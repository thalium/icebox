/* $Id: VBoxMouse.h $ */
/** @file
 * VBoxMouse; input_server add-on - Haiku Guest Additions, header.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*
 * This code is based on:
 *
 * VirtualBox Guest Additions for Haiku.
 * Copyright (c) 2011 Mike Smith <mike@scgtrp.net>
 *                    François Revol <revol@free.fr>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __VBOXMOUSE__H
#define __VBOXMOUSE__H

#include <InputServerDevice.h>

extern "C" _EXPORT BInputServerDevice* instantiate_input_device();

class VBoxMouse : public BInputServerDevice
{
    public:
        VBoxMouse();
        virtual ~VBoxMouse();

        virtual status_t        InitCheck();
        virtual status_t        SystemShuttingDown();

        virtual status_t        Start(const char *device, void *cookie);
        virtual status_t        Stop(const char *device, void *cookie);
        virtual status_t        Control(const char *device, void *cookie, uint32 code, BMessage *message);

    private:

        static status_t         _ServiceThreadNub(void *_this);
        status_t                _ServiceThread();

        int                     fDriverFD;
        thread_id               fServiceThreadID;
        bool                    fExiting;
};

#endif /* __VBOXMOUSE__H */

