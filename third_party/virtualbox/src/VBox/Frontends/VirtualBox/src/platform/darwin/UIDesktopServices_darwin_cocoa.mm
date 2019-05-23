/* $Id: UIDesktopServices_darwin_cocoa.mm $ */
/** @file
 * VBox Qt GUI - Qt GUI - Utility Classes and Functions specific to darwin.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* VBox includes */
#include "UIDesktopServices_darwin_p.h"

/* System includes */
#include <Carbon/Carbon.h>
#import <AppKit/NSWorkspace.h>

/* Create desktop alias using a bookmark stuff. */
bool darwinCreateMachineShortcut(NativeNSStringRef pstrSrcFile, NativeNSStringRef pstrDstPath, NativeNSStringRef pstrName, NativeNSStringRef /* pstrUuid */)
{
    RT_NOREF(pstrName);
    if (!pstrSrcFile || !pstrDstPath)
        return false;

    NSError  *pErr        = nil;
    NSURL    *pSrcUrl     = [NSURL fileURLWithPath:pstrSrcFile];

    NSString *pVmFileName = [pSrcUrl lastPathComponent];
    NSString *pSrcPath    = [NSString stringWithFormat:@"%@/%@", pstrDstPath, [pVmFileName stringByDeletingPathExtension]];
    NSURL    *pDstUrl     = [NSURL fileURLWithPath:pSrcPath];

    bool rc = false;

    if (!pSrcUrl || !pDstUrl)
        return false;

    NSData *pBookmark = [pSrcUrl bookmarkDataWithOptions:NSURLBookmarkCreationSuitableForBookmarkFile
                                 includingResourceValuesForKeys:nil
                                 relativeToURL:nil
                                 error:&pErr];

    if (pBookmark)
    {
        rc = [NSURL writeBookmarkData:pBookmark
                    toURL:pDstUrl
                    options:0
                    error:&pErr];
    }

    return rc;
}

bool darwinOpenInFileManager(NativeNSStringRef pstrFile)
{
    return [[NSWorkspace sharedWorkspace] selectFile:pstrFile inFileViewerRootedAtPath:@""];
}

