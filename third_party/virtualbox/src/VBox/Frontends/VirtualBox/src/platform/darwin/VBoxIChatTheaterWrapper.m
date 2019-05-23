/* $Id: VBoxIChatTheaterWrapper.m $ */
/** @file
 * VBox Qt GUI - iChat Theater cocoa wrapper.
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
 */

#ifdef VBOX_WITH_ICHAT_THEATER

/* System includes */
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <InstantMessage/IMService.h>
#import <InstantMessage/IMAVManager.h>

@interface AVHandler: NSObject
{
    CGImageRef mImage;
}

- (void) setImage: (CGImageRef)aImage;
- (void) getPixelBufferPixelFormat: (OSType *)pixelFormatOut;
- (bool) renderIntoPixelBuffer: (CVPixelBufferRef)buffer forTime: (CVTimeStamp *)timeStamp;

- (void) AVManagerNotification: (NSNotification *)notification;
@end

@implementation AVHandler
- (id) init
{
    if (!(self = [super init])) return nil;
    mImage = nil;

    /* Register for notifications of the av manager */
    NSNotificationCenter *nCenter = [IMService notificationCenter];
    [nCenter addObserver: self
        selector: @selector (AVManagerNotification:)
          name: IMAVManagerStateChangedNotification
          object: nil];

    return self;
}

- (void)dealloc
{
    NSNotificationCenter *nCenter = [IMService notificationCenter];
    [nCenter removeObserver:self];
    IMAVManager *avManager = [IMAVManager sharedAVManager];
    [avManager stop];
    [avManager setVideoDataSource:nil];
    [super dealloc];
}

- (void) setImage: (CGImageRef)aImage
{
    mImage = aImage;
}

- (void) getPixelBufferPixelFormat: (OSType *)pixelFormatOut
{
    /* Return 32 bit pixel format */
    *pixelFormatOut = kCVPixelFormatType_32ARGB;
}

- (bool) renderIntoPixelBuffer: (CVPixelBufferRef)buffer forTime: (CVTimeStamp *)timeStamp
{
    if (mImage == nil)
        return NO;

    // Lock the pixel buffer's base address so that we can draw into it.
    CVReturn err;
    if ((err = CVPixelBufferLockBaseAddress (buffer, 0)) != kCVReturnSuccess)
    {
        // Rarely is a lock refused. Return NO if this happens.
        printf ("Warning: could not lock pixel buffer base address in %s - error %ld\n", __func__, (long)err);
        return NO;
    }
    size_t width = CVPixelBufferGetWidth (buffer);
    size_t height = CVPixelBufferGetHeight (buffer);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate (CVPixelBufferGetBaseAddress (buffer),
                                                  width, height,
                                                  8,
                                                  CVPixelBufferGetBytesPerRow (buffer),
                                                  colorSpace,
                                                  kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease (colorSpace);
    /* Clear background */
    CGContextClearRect (context, CGRectMake (0, 0, width, height));
    /* Center image */
    int scaledWidth;
    int scaledHeight;
    float aspect = (float)(CGImageGetWidth (mImage)) / CGImageGetHeight (mImage);
    if (aspect > 1.0)
    {
        scaledWidth = width;
        scaledHeight = height / aspect;
    }
    else
    {
        scaledWidth = width * aspect;
        scaledHeight = height;
    }
    CGRect iconRect = CGRectMake ((width - scaledWidth) / 2.0,
                                  (height - scaledHeight) / 2.0,
                                   scaledWidth, scaledHeight);
    /* Here we have to paint the context*/
    CGContextDrawImage (context, iconRect, mImage);
    /* Finished painting */
    CGContextRelease (context);
    CVPixelBufferUnlockBaseAddress (buffer, 0);

    return YES;
}

#pragma mark -
#pragma mark Notifications
- (void) AVManagerNotification: (NSNotification *)notification
{
    IMAVManager *avManager = [IMAVManager sharedAVManager];
    printf ("state changed to: %d\n", [avManager state]);

    /* Get the new state */
    IMAVManagerState state = [avManager state];

    if (state == IMAVRequested)
    {
        /* Set some properties on the av manager */
        [avManager setVideoDataSource:self];
        /* Possible values: IMVideoOptimizationDefault, IMVideoOptimizationStills, IMVideoOptimizationReplacement */
        [avManager setVideoOptimizationOptions:IMVideoOptimizationDefault];
        /** @todo Audio support */
        [avManager setNumberOfAudioChannels:0];
        /* Start the streaming of the video */
        [avManager start];
    }
    else if (state == IMAVInactive)
    {
        /* Stop the content propagation */
        [avManager stop];
        [avManager setVideoDataSource:nil];
    }
}
#pragma mark -
@end

static AVHandler *sharedAVHandler = NULL;

void initSharedAVManager ()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    /* Some other debug tests */
    /*
    NSProcessInfo *pi = [NSProcessInfo processInfo];
    [pi setProcessName:@"My new processName"];
    OSErr err;
    ProcessSerialNumber PSN;
    err = GetCurrentProcess (&PSN);
    err = CPSSetProcessName (&PSN,"Evil");
    err = CPSEnableForegroundOperation (&PSN,0x03,0x3C,0x2C,0x1103);
    err = CPSSetFrontProcess (&PSN);
    */

    if (!sharedAVHandler)
        sharedAVHandler = [[AVHandler alloc] init];

    [pool release];
}

void setImageRef (CGImageRef aImage)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    initSharedAVManager();
    [sharedAVHandler setImage: aImage];

    [pool release];
}

#endif /* VBOX_WITH_ICHAT_THEATER */

