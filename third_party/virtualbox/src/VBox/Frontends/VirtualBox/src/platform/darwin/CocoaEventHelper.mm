/* $Id: CocoaEventHelper.mm $ */
/** @file
 * VBox Qt GUI - Declarations of utility functions for handling Darwin Cocoa specific event handling tasks.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Local includes */
#include "CocoaEventHelper.h"
#include "DarwinKeyboard.h"

/* Global includes */
#import <Cocoa/Cocoa.h>
#import <AppKit/NSEvent.h>
#include <Carbon/Carbon.h>

/**
 * Calls the -(NSUInteger)modifierFlags method on a NSEvent object.
 *
 * @return  The Cocoa event modifier mask.
 * @param   pEvent          The Cocoa event.
 */
unsigned long darwinEventModifierFlags(ConstNativeNSEventRef pEvent)
{
    return [pEvent modifierFlags];
}

/**
 * Calls the -(NSUInteger)modifierFlags method on a NSEvent object and
 * converts the flags to carbon style.
 *
 * @return  The Carbon modifier mask.
 * @param   pEvent          The Cocoa event.
 */
uint32_t darwinEventModifierFlagsXlated(ConstNativeNSEventRef pEvent)
{
    NSUInteger  fCocoa  = [pEvent modifierFlags];
    uint32_t    fCarbon = 0;
    if (fCocoa)
    {
        if (fCocoa & NSAlphaShiftKeyMask)
            fCarbon |= alphaLock;
        if (fCocoa & (NSShiftKeyMask | NX_DEVICELSHIFTKEYMASK | NX_DEVICERSHIFTKEYMASK))
        {
            if (fCocoa & (NX_DEVICERSHIFTKEYMASK | NX_DEVICERSHIFTKEYMASK))
            {
                if (fCocoa & NX_DEVICERSHIFTKEYMASK)
                    fCarbon |= rightShiftKey;
                if (fCocoa & NX_DEVICELSHIFTKEYMASK)
                    fCarbon |= shiftKey;
            }
            else
                fCarbon |= shiftKey;
        }

        if (fCocoa & (NSControlKeyMask | NX_DEVICELCTLKEYMASK | NX_DEVICERCTLKEYMASK))
        {
            if (fCocoa & (NX_DEVICELCTLKEYMASK | NX_DEVICERCTLKEYMASK))
            {
                if (fCocoa & NX_DEVICERCTLKEYMASK)
                    fCarbon |= rightControlKey;
                if (fCocoa & NX_DEVICELCTLKEYMASK)
                    fCarbon |= controlKey;
            }
            else
                fCarbon |= controlKey;
        }

        if (fCocoa & (NSAlternateKeyMask | NX_DEVICELALTKEYMASK | NX_DEVICERALTKEYMASK))
        {
            if (fCocoa & (NX_DEVICELALTKEYMASK | NX_DEVICERALTKEYMASK))
            {
                if (fCocoa & NX_DEVICERALTKEYMASK)
                    fCarbon |= rightOptionKey;
                if (fCocoa & NX_DEVICELALTKEYMASK)
                    fCarbon |= optionKey;
            }
            else
                fCarbon |= optionKey;
        }

        if (fCocoa & (NSCommandKeyMask | NX_DEVICELCMDKEYMASK | NX_DEVICERCMDKEYMASK))
        {
            if (fCocoa & (NX_DEVICELCMDKEYMASK | NX_DEVICERCMDKEYMASK))
            {
                if (fCocoa & NX_DEVICERCMDKEYMASK)
                    fCarbon |= kEventKeyModifierRightCmdKeyMask;
                if (fCocoa & NX_DEVICELCMDKEYMASK)
                    fCarbon |= cmdKey;
            }
            else
                fCarbon |= cmdKey;
        }

        /*
        if (fCocoa & NSNumericPadKeyMask)
            fCarbon |= ???;

        if (fCocoa & NSHelpKeyMask)
            fCarbon |= ???;

        if (fCocoa & NSFunctionKeyMask)
            fCarbon |= ???;
        */
    }

    return fCarbon;
}

/**
 * Get the name for a Cocoa event type.
 *
 * @returns Read-only name string.
 * @param   eEvtType        The Cocoa event type.
 */
const char *darwinEventTypeName(unsigned long eEvtType)
{
    switch (eEvtType)
    {
#define EVT_CASE(nm) case nm: return #nm
        EVT_CASE(NSLeftMouseDown);
        EVT_CASE(NSLeftMouseUp);
        EVT_CASE(NSRightMouseDown);
        EVT_CASE(NSRightMouseUp);
        EVT_CASE(NSMouseMoved);
        EVT_CASE(NSLeftMouseDragged);
        EVT_CASE(NSRightMouseDragged);
        EVT_CASE(NSMouseEntered);
        EVT_CASE(NSMouseExited);
        EVT_CASE(NSKeyDown);
        EVT_CASE(NSKeyUp);
        EVT_CASE(NSFlagsChanged);
        EVT_CASE(NSAppKitDefined);
        EVT_CASE(NSSystemDefined);
        EVT_CASE(NSApplicationDefined);
        EVT_CASE(NSPeriodic);
        EVT_CASE(NSCursorUpdate);
        EVT_CASE(NSScrollWheel);
        EVT_CASE(NSTabletPoint);
        EVT_CASE(NSTabletProximity);
        EVT_CASE(NSOtherMouseDown);
        EVT_CASE(NSOtherMouseUp);
        EVT_CASE(NSOtherMouseDragged);
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
        EVT_CASE(NSEventTypeGesture);
        EVT_CASE(NSEventTypeMagnify);
        EVT_CASE(NSEventTypeSwipe);
        EVT_CASE(NSEventTypeRotate);
        EVT_CASE(NSEventTypeBeginGesture);
        EVT_CASE(NSEventTypeEndGesture);
#endif
#undef EVT_CASE
        default:
            return "Unknown!";
    }
}

/**
 * Debug helper function for dumping a Cocoa event to stdout.
 *
 * @param   pszPrefix       Message prefix.
 * @param   pEvent          The Cocoa event.
 */
void darwinPrintEvent(const char *pszPrefix, ConstNativeNSEventRef pEvent)
{
    NSEventType         eEvtType = [pEvent type];
    NSUInteger          fEvtMask = [pEvent modifierFlags];
    NSWindow           *pEvtWindow = [pEvent window];
    NSInteger           iEvtWindow = [pEvent windowNumber];
    NSGraphicsContext  *pEvtGraphCtx = [pEvent context];

    printf("%s%p: Type=%lu Modifiers=%08lx pWindow=%p #Wnd=%ld pGraphCtx=%p %s\n",
           pszPrefix, (void*)pEvent, (unsigned long)eEvtType, (unsigned long)fEvtMask, (void*)pEvtWindow,
           (long)iEvtWindow, (void*)pEvtGraphCtx, darwinEventTypeName(eEvtType));

    /* dump type specific into. */
    switch (eEvtType)
    {
        case NSLeftMouseDown:
        case NSLeftMouseUp:
        case NSRightMouseDown:
        case NSRightMouseUp:
        case NSMouseMoved:

        case NSLeftMouseDragged:
        case NSRightMouseDragged:
        case NSMouseEntered:
        case NSMouseExited:
            break;

        case NSKeyDown:
        case NSKeyUp:
        {
            NSUInteger i;
            NSUInteger cch;
            NSString *pChars = [pEvent characters];
            NSString *pCharsIgnMod = [pEvent charactersIgnoringModifiers];
            BOOL fIsARepeat = [pEvent isARepeat];
            unsigned short KeyCode = [pEvent keyCode];

            printf("    KeyCode=%04x isARepeat=%d", KeyCode, fIsARepeat);
            if (pChars)
            {
                cch = [pChars length];
                printf(" characters={");
                for (i = 0; i < cch; i++)
                    printf(i == 0 ? "%02x" : ",%02x", [pChars characterAtIndex: i]);
                printf("}");
            }

            if (pCharsIgnMod)
            {
                cch = [pCharsIgnMod length];
                printf(" charactersIgnoringModifiers={");
                for (i = 0; i < cch; i++)
                    printf(i == 0 ? "%02x" : ",%02x", [pCharsIgnMod characterAtIndex: i]);
                printf("}");
            }
            printf("\n");
            break;
        }

        case NSFlagsChanged:
        {
            NSUInteger fOddBits = NSAlphaShiftKeyMask | NSShiftKeyMask | NSControlKeyMask | NSAlternateKeyMask
                                | NSCommandKeyMask | NSNumericPadKeyMask | NSHelpKeyMask | NSFunctionKeyMask
                                | NX_DEVICELCTLKEYMASK | NX_DEVICELSHIFTKEYMASK | NX_DEVICERSHIFTKEYMASK
                                | NX_DEVICELCMDKEYMASK | NX_DEVICERCMDKEYMASK | NX_DEVICELALTKEYMASK
                                | NX_DEVICERALTKEYMASK | NX_DEVICERCTLKEYMASK;

            printf("    KeyCode=%04x", (int)[pEvent keyCode]);
#define PRINT_MOD(cnst, nm) do { if (fEvtMask & (cnst)) printf(" %s", #nm); } while (0)
            /* device-independent: */
            PRINT_MOD(NSAlphaShiftKeyMask, "AlphaShift");
            PRINT_MOD(NSShiftKeyMask, "Shift");
            PRINT_MOD(NSControlKeyMask, "Ctrl");
            PRINT_MOD(NSAlternateKeyMask, "Alt");
            PRINT_MOD(NSCommandKeyMask, "Cmd");
            PRINT_MOD(NSNumericPadKeyMask, "NumLock");
            PRINT_MOD(NSHelpKeyMask, "Help");
            PRINT_MOD(NSFunctionKeyMask, "Fn");
            /* device-dependent (sort of): */
            PRINT_MOD(NX_DEVICELCTLKEYMASK,   "$L-Ctrl");
            PRINT_MOD(NX_DEVICELSHIFTKEYMASK, "$L-Shift");
            PRINT_MOD(NX_DEVICERSHIFTKEYMASK, "$R-Shift");
            PRINT_MOD(NX_DEVICELCMDKEYMASK,   "$L-Cmd");
            PRINT_MOD(NX_DEVICERCMDKEYMASK,   "$R-Cmd");
            PRINT_MOD(NX_DEVICELALTKEYMASK,   "$L-Alt");
            PRINT_MOD(NX_DEVICERALTKEYMASK,   "$R-Alt");
            PRINT_MOD(NX_DEVICERCTLKEYMASK,   "$R-Ctrl");
#undef  PRINT_MOD

            fOddBits = fEvtMask & ~fOddBits;
            if (fOddBits)
                printf(" fOddBits=%#08lx", (unsigned long)fOddBits);
#undef  KNOWN_BITS
            printf("\n");
            break;
        }

        case NSAppKitDefined:
        case NSSystemDefined:
        case NSApplicationDefined:
        case NSPeriodic:
        case NSCursorUpdate:
        case NSScrollWheel:
        case NSTabletPoint:
        case NSTabletProximity:
        case NSOtherMouseDown:
        case NSOtherMouseUp:
        case NSOtherMouseDragged:
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
        case NSEventTypeGesture:
        case NSEventTypeMagnify:
        case NSEventTypeSwipe:
        case NSEventTypeRotate:
        case NSEventTypeBeginGesture:
        case NSEventTypeEndGesture:
#endif
        default:
            printf(" Unknown!\n");
            break;
    }
}

void darwinPostStrippedMouseEvent(ConstNativeNSEventRef pEvent)
{
    /* Create and post new stripped event: */
    NSEvent *pNewEvent = [NSEvent mouseEventWithType:[pEvent type]
                                            location:[pEvent locationInWindow]
                                       modifierFlags:0
                                           timestamp:[pEvent timestamp] // [NSDate timeIntervalSinceReferenceDate] ?
                                        windowNumber:[pEvent windowNumber]
                                             context:[pEvent context]
                                         eventNumber:[pEvent eventNumber]
                                          clickCount:[pEvent clickCount]
                                            pressure:[pEvent pressure]];
    [NSApp postEvent:pNewEvent atStart:YES];
}

