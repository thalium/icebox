/* $Id: UICocoaApplication.mm $ */
/** @file
 * VBox Qt GUI - UICocoaApplication - C++ interface to NSApplication for handling -sendEvent.
 */

/*
 * Copyright (C) 2009-2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* GUI includes: */
#include "UICocoaApplication.h"

/* Global includes */
#import <AppKit/NSEvent.h>
#import <AppKit/NSApplication.h>
#import <Foundation/NSArray.h>
#import <AppKit/NSWindow.h>
#import <AppKit/NSButton.h>

#include <iprt/assert.h>

/** Class for tracking a callback. */
@interface CallbackData: NSObject
{
@public
    /** Mask of events to send to this callback. */
    uint32_t            fMask;
    /** The callback. */
    PFNVBOXCACALLBACK   pfnCallback;
    /** The user argument. */
    void               *pvUser;
}
- (id) initWithMask:(uint32)mask callback:(PFNVBOXCACALLBACK)callback user:(void*)user;
@end /* @interface CallbackData  */

@implementation CallbackData
- (id) initWithMask:(uint32)mask callback:(PFNVBOXCACALLBACK)callback user:(void*)user
{
    self = [super init];
    if (self)
    {
        fMask = mask;
        pfnCallback = callback;
        pvUser =  user;
    }
    return self;
}
@end /* @implementation CallbackData  */

/** Class for event handling */
@interface UICocoaApplicationPrivate: NSApplication
{
    /** The event mask for which there currently are callbacks. */
    uint32_t        m_fMask;
    /** Array of callbacks. */
    NSMutableArray *m_pCallbacks;
}
- (id)init;
- (void)sendEvent:(NSEvent *)theEvent;
- (void)setCallback:(uint32_t)fMask :(PFNVBOXCACALLBACK)pfnCallback :(void *)pvUser;
- (void)unsetCallback:(uint32_t)fMask :(PFNVBOXCACALLBACK)pfnCallback :(void *)pvUser;

- (void)registerToNotificationOfWorkspace :(NSString*)pstrNotificationName;
- (void)unregisterFromNotificationOfWorkspace :(NSString*)pstrNotificationName;

- (void)registerToNotificationOfWindow :(NSString*)pstrNotificationName :(NSWindow*)pWindow;
- (void)unregisterFromNotificationOfWindow :(NSString*)pstrNotificationName :(NSWindow*)pWindow;

- (void)notificationCallbackOfObject :(NSNotification*)notification;
- (void)notificationCallbackOfWindow :(NSNotification*)notification;

- (void)registerSelectorForStandardWindowButton :(NSWindow*)pWindow :(StandardWindowButtonType)enmButtonType;
- (void)selectorForStandardWindowButton :(NSButton*)pButton;
@end /* @interface UICocoaApplicationPrivate */

@implementation UICocoaApplicationPrivate
-(id) init
{
    self = [super init];
    if (self)
        m_pCallbacks = [[NSMutableArray alloc] init];

    /* Gently disable El Capitan tries to break everything with the Enter Full Screen action.
     * S.a. https://developer.apple.com/library/mac/releasenotes/AppKit/RN-AppKit/ for reference. */
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];

    return self;
}

-(void) sendEvent:(NSEvent *)pEvent
{
    /*
     * Check if the type matches any of the registered callbacks.
     */
    uint32_t const fMask = m_fMask;
#if 0 /* for debugging */
    ::darwinPrintEvent("sendEvent: ", pEvent);
#endif
    if (fMask != 0)
    {
        NSEventType EvtType = [pEvent type];
        uint32_t fEvtMask = RT_LIKELY(EvtType < 32) ? RT_BIT_32(EvtType) : 0;
        if (fMask & fEvtMask)
        {
            /*
             * Do the callouts in LIFO order.
             */
            for (CallbackData *pData in [m_pCallbacks reverseObjectEnumerator])
            {
                if (pData->fMask & fEvtMask)
                {
                    if (pData->pfnCallback(pEvent, [pEvent eventRef], pData->pvUser))
                        return;
                }

            }
        }
    }

    /*
     * Get on with it.
     */
    [super sendEvent:pEvent];
}

/**
 * Register an event callback.
 *
 * @param   fMask           The event mask for which the callback is to be invoked.
 * @param   pfnCallback     The callback function.
 * @param   pvUser          The user argument.
 */
-(void) setCallback: (uint32_t)fMask :(PFNVBOXCACALLBACK)pfnCallback :(void *)pvUser
{
    /* Add the callback data to the array */
    CallbackData *pData = [[[CallbackData alloc] initWithMask: fMask callback: pfnCallback user: pvUser] autorelease];
    [m_pCallbacks addObject: pData];

    /* Update the global mask */
    m_fMask |= fMask;
}

/**
 * Deregister an event callback.
 *
 * @param   fMask           Same as setCallback.
 * @param   pfnCallback     Same as setCallback.
 * @param   pvUser          Same as setCallback.
 */
-(void) unsetCallback: (uint32_t)fMask :(PFNVBOXCACALLBACK)pfnCallback :(void *)pvUser
{
    /*
     * Loop the event array LIFO fashion searching for a matching callback.
     */
    for (CallbackData *pData in [m_pCallbacks reverseObjectEnumerator])
    {
        if (   pData->pfnCallback == pfnCallback
            && pData->pvUser      == pvUser
            && pData->fMask       == fMask)
        {
            [m_pCallbacks removeObject: pData];
            break;
        }
    }
    uint32_t fNewMask = 0;
    for (CallbackData *pData in m_pCallbacks)
        fNewMask |= pData->fMask;
    m_fMask = fNewMask;
}

/** Register to cocoa notification @a pstrNotificationName. */
- (void) registerToNotificationOfWorkspace :(NSString*)pstrNotificationName
{
    /* Register notification observer: */
    NSNotificationCenter *pNotificationCenter = [[NSWorkspace sharedWorkspace] notificationCenter];
    [pNotificationCenter addObserver:self
                            selector:@selector(notificationCallbackOfObject:)
                                name:pstrNotificationName
                              object:nil];
}

/** Unregister @a pWindow from cocoa notification @a pstrNotificationName. */
- (void) unregisterFromNotificationOfWorkspace :(NSString*)pstrNotificationName
{
    /* Uninstall notification observer: */
    NSNotificationCenter *pNotificationCenter = [[NSWorkspace sharedWorkspace] notificationCenter];
    [pNotificationCenter removeObserver:self
                                   name:pstrNotificationName
                                 object:nil];
}

/** Register @a pWindow to cocoa notification @a pstrNotificationName. */
- (void) registerToNotificationOfWindow :(NSString*)pstrNotificationName :(NSWindow*)pWindow
{
    /* Register notification observer: */
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(notificationCallbackOfWindow:)
                                                 name:pstrNotificationName
                                               object:pWindow];
}

/** Unregister @a pWindow from cocoa notification @a pstrNotificationName. */
- (void) unregisterFromNotificationOfWindow :(NSString*)pstrNotificationName :(NSWindow*)pWindow
{
    /* Uninstall notification observer: */
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:pstrNotificationName
                                                  object:pWindow];
}

/** Redirects cocoa @a notification to UICocoaApplication instance. */
- (void) notificationCallbackOfObject :(NSNotification*)notification
{
    /* Get current notification name: */
    NSString *pstrName = [notification name];

    /* Prepare user-info: */
    QMap<QString, QString> userInfo;

    /* Process known notifications: */
    if (   [pstrName isEqualToString :@"NSWorkspaceDidActivateApplicationNotification"]
        || [pstrName isEqualToString :@"NSWorkspaceDidDeactivateApplicationNotification"])
    {
        NSDictionary *pUserInfo = [notification userInfo];
        NSRunningApplication *pApplication = [pUserInfo valueForKey :@"NSWorkspaceApplicationKey"];
        NSString *pstrBundleIndentifier = [pApplication bundleIdentifier];
        userInfo.insert("BundleIdentifier", darwinFromNativeString((NativeNSStringRef)pstrBundleIndentifier));
    }

    /* Redirect known notifications to objects: */
    UICocoaApplication::instance()->nativeNotificationProxyForObject(pstrName, userInfo);
}

/** Redirects cocoa @a notification to UICocoaApplication instance. */
- (void) notificationCallbackOfWindow :(NSNotification*)notification
{
    /* Get current notification name: */
    NSString *pstrName = [notification name];

    /* Redirect known notifications to widgets: */
    UICocoaApplication::instance()->nativeNotificationProxyForWidget(pstrName, [notification object]);
}

/** Registers selector for standard window @a enmButtonType of the passed @a pWindow. */
- (void)registerSelectorForStandardWindowButton :(NSWindow*)pWindow :(StandardWindowButtonType)enmButtonType
{
    /* Retrieve corresponding button: */
    NSButton *pButton = Nil;
    switch (enmButtonType)
    {
        case StandardWindowButtonType_Close:            pButton = [pWindow standardWindowButton:NSWindowCloseButton]; break;
        case StandardWindowButtonType_Miniaturize:      pButton = [pWindow standardWindowButton:NSWindowMiniaturizeButton]; break;
        case StandardWindowButtonType_Zoom:             pButton = [pWindow standardWindowButton:NSWindowZoomButton]; break;
        case StandardWindowButtonType_Toolbar:          pButton = [pWindow standardWindowButton:NSWindowToolbarButton]; break;
        case StandardWindowButtonType_DocumentIcon:     pButton = [pWindow standardWindowButton:NSWindowDocumentIconButton]; break;
        case StandardWindowButtonType_DocumentVersions: /*pButton = [pWindow standardWindowButton:NSWindowDocumentVersionsButton];*/ break;
        case StandardWindowButtonType_FullScreen:       /*pButton = [pWindow standardWindowButton:NSWindowFullScreenButton];*/ break;
    }

    /* Register selector if button exists: */
    if (pButton != Nil)
    {
        [pButton setTarget :self];
        [pButton setAction :@selector(selectorForStandardWindowButton:)];
    }
}

/** Redirects selector of the standard window @a pButton to UICocoaApplication instance callback. */
- (void)selectorForStandardWindowButton :(NSButton*)pButton
{
    /* Check if Option key is currently held: */
    const bool fWithOptionKey = [NSEvent modifierFlags] & NSAlternateKeyMask;

    /* Redirect selector to callback: */
    UICocoaApplication::instance()->nativeCallbackProxyForStandardWindowButton(pButton, fWithOptionKey);
}
@end /* @implementation UICocoaApplicationPrivate */

/* C++ singleton for our private NSApplication object */
UICocoaApplication* UICocoaApplication::m_pInstance = 0;

/* static */
UICocoaApplication* UICocoaApplication::instance()
{
    if (!m_pInstance)
        m_pInstance = new UICocoaApplication();

    return m_pInstance;
}

UICocoaApplication::UICocoaApplication()
{
    /* Make sure our private NSApplication object is created */
    m_pNative = (UICocoaApplicationPrivate*)[UICocoaApplicationPrivate sharedApplication];
    /* Create one auto release pool which is in place for all the
       initialization and deinitialization stuff. That is when the
       NSApplication is not running the run loop (there is a separate auto
       release pool defined). */
    m_pPool = [[NSAutoreleasePool alloc] init];
}

UICocoaApplication::~UICocoaApplication()
{
    [m_pNative release];
    [m_pPool release];
}

bool UICocoaApplication::isActive() const
{
    return [m_pNative isActive];
}

void UICocoaApplication::hide()
{
    [m_pNative hide:m_pNative];
}

void UICocoaApplication::registerForNativeEvents(uint32_t fMask, PFNVBOXCACALLBACK pfnCallback, void *pvUser)
{
    [m_pNative setCallback:fMask :pfnCallback :pvUser];
}

void UICocoaApplication::unregisterForNativeEvents(uint32_t fMask, PFNVBOXCACALLBACK pfnCallback, void *pvUser)
{
    [m_pNative unsetCallback:fMask :pfnCallback :pvUser];
}

void UICocoaApplication::registerToNotificationOfWorkspace(const QString &strNativeNotificationName, QObject *pObject,
                                                           PfnNativeNotificationCallbackForQObject pCallback)
{
    /* Make sure it is not registered yet: */
    AssertReturnVoid(!m_objectCallbacks.contains(pObject) || !m_objectCallbacks[pObject].contains(strNativeNotificationName));

    /* Remember callback: */
    m_objectCallbacks[pObject][strNativeNotificationName] = pCallback;

    /* Register observer: */
    NativeNSStringRef pstrNativeNotificationName = darwinToNativeString(strNativeNotificationName.toLatin1().constData());
    [m_pNative registerToNotificationOfWorkspace :pstrNativeNotificationName];
}

void UICocoaApplication::unregisterFromNotificationOfWorkspace(const QString &strNativeNotificationName, QObject *pObject)
{
    /* Make sure it is registered yet: */
    AssertReturnVoid(m_objectCallbacks.contains(pObject) && m_objectCallbacks[pObject].contains(strNativeNotificationName));

    /* Forget callback: */
    m_objectCallbacks[pObject].remove(strNativeNotificationName);
    if (m_objectCallbacks[pObject].isEmpty())
        m_objectCallbacks.remove(pObject);

    /* Unregister observer: */
    NativeNSStringRef pstrNativeNotificationName = darwinToNativeString(strNativeNotificationName.toLatin1().constData());
    [m_pNative unregisterFromNotificationOfWorkspace :pstrNativeNotificationName];
}

void UICocoaApplication::registerToNotificationOfWindow(const QString &strNativeNotificationName, QWidget *pWidget,
                                                        PfnNativeNotificationCallbackForQWidget pCallback)
{
    /* Make sure it is not registered yet: */
    AssertReturnVoid(!m_widgetCallbacks.contains(pWidget) || !m_widgetCallbacks[pWidget].contains(strNativeNotificationName));

    /* Remember callback: */
    m_widgetCallbacks[pWidget][strNativeNotificationName] = pCallback;

    /* Register observer: */
    NativeNSStringRef pstrNativeNotificationName = darwinToNativeString(strNativeNotificationName.toLatin1().constData());
    NativeNSWindowRef pWindow = darwinToNativeWindow(pWidget);
    [m_pNative registerToNotificationOfWindow :pstrNativeNotificationName :pWindow];
}

void UICocoaApplication::unregisterFromNotificationOfWindow(const QString &strNativeNotificationName, QWidget *pWidget)
{
    /* Make sure it is registered yet: */
    AssertReturnVoid(m_widgetCallbacks.contains(pWidget) && m_widgetCallbacks[pWidget].contains(strNativeNotificationName));

    /* Forget callback: */
    m_widgetCallbacks[pWidget].remove(strNativeNotificationName);
    if (m_widgetCallbacks[pWidget].isEmpty())
        m_widgetCallbacks.remove(pWidget);

    /* Unregister observer: */
    NativeNSStringRef pstrNativeNotificationName = darwinToNativeString(strNativeNotificationName.toLatin1().constData());
    NativeNSWindowRef pWindow = darwinToNativeWindow(pWidget);
    [m_pNative unregisterFromNotificationOfWindow :pstrNativeNotificationName :pWindow];
}

void UICocoaApplication::nativeNotificationProxyForObject(NativeNSStringRef pstrNotificationName, const QMap<QString, QString> &userInfo)
{
    /* Get notification name: */
    QString strNotificationName = darwinFromNativeString(pstrNotificationName);

    /* Check if existing object(s) have corresponding notification handler: */
    foreach (QObject *pObject, m_objectCallbacks.keys())
    {
        const QMap<QString, PfnNativeNotificationCallbackForQObject> &callbacks = m_objectCallbacks[pObject];
        if (callbacks.contains(strNotificationName))
            callbacks[strNotificationName](pObject, userInfo);
    }
}

void UICocoaApplication::nativeNotificationProxyForWidget(NativeNSStringRef pstrNotificationName, NativeNSWindowRef pWindow)
{
    /* Get notification name: */
    QString strNotificationName = darwinFromNativeString(pstrNotificationName);

    /* Check if existing widget(s) have corresponding notification handler: */
    foreach (QWidget *pWidget, m_widgetCallbacks.keys())
    {
        if (darwinToNativeWindow(pWidget) == pWindow)
        {
            const QMap<QString, PfnNativeNotificationCallbackForQWidget> &callbacks = m_widgetCallbacks[pWidget];
            if (callbacks.contains(strNotificationName))
                callbacks[strNotificationName](strNotificationName, pWidget);
        }
    }
}

void UICocoaApplication::registerCallbackForStandardWindowButton(QWidget *pWidget, StandardWindowButtonType enmButtonType,
                                                                 PfnStandardWindowButtonCallbackForQWidget pCallback)
{
    /* Make sure it is not registered yet: */
    AssertReturnVoid(!m_stdWindowButtonCallbacks.contains(pWidget) || !m_stdWindowButtonCallbacks.value(pWidget).contains(enmButtonType));

    /* Remember callback: */
    m_stdWindowButtonCallbacks[pWidget][enmButtonType] = pCallback;

    /* Register selector: */
    NativeNSWindowRef pWindow = darwinToNativeWindow(pWidget);
    [m_pNative registerSelectorForStandardWindowButton :pWindow :enmButtonType];
}

void UICocoaApplication::unregisterCallbackForStandardWindowButton(QWidget *pWidget, StandardWindowButtonType enmButtonType)
{
    /* Make sure it is registered yet: */
    AssertReturnVoid(m_stdWindowButtonCallbacks.contains(pWidget) && m_stdWindowButtonCallbacks.value(pWidget).contains(enmButtonType));

    /* Forget callback: */
    m_stdWindowButtonCallbacks[pWidget].remove(enmButtonType);
    if (m_stdWindowButtonCallbacks.value(pWidget).isEmpty())
        m_stdWindowButtonCallbacks.remove(pWidget);
}

void UICocoaApplication::nativeCallbackProxyForStandardWindowButton(NativeNSButtonRef pButton, bool fWithOptionKey)
{
    // Why not using nested foreach, will you ask?
    // It's because Qt 4.x has shadowing issue in Q_FOREACH macro.
    // Bug record QTBUG-33585 opened for Qt 4.8.4 and closed as _won't fix_ by one of Qt devs.

    /* Check if passed button is one of the buttons of the registered widget(s): */
    const QList<QWidget*> widgets = m_stdWindowButtonCallbacks.keys();
    for (int iWidgetIndex = 0; iWidgetIndex < widgets.size(); ++iWidgetIndex)
    {
        QWidget *pWidget = widgets.at(iWidgetIndex);
        const QMap<StandardWindowButtonType, PfnStandardWindowButtonCallbackForQWidget> callbacks = m_stdWindowButtonCallbacks.value(pWidget);
        const QList<StandardWindowButtonType> buttonTypes = callbacks.keys();
        for (int iButtonTypeIndex = 0; iButtonTypeIndex < buttonTypes.size(); ++iButtonTypeIndex)
        {
            StandardWindowButtonType enmButtonType = buttonTypes.at(iButtonTypeIndex);
            if (darwinNativeButtonOfWindow(pWidget, enmButtonType) == pButton)
                return callbacks.value(enmButtonType)(enmButtonType, fWithOptionKey, pWidget);
        }
    }
}

