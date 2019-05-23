/* $Id: XKeyboard-new.cpp $ */
/** @file
 * VBox Qt GUI - Implementation of Linux-specific keyboard functions.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Define GUI log group.
 * This define should go *before* VBox/log.h include: */
#define LOG_GROUP LOG_GROUP_GUI

/* Qt includes: */
# include <QString>
# include <QStringList>

/* Other VBox includes: */
# include <VBox/log.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
#include <XKeyboard.h>

/* Other VBox includes: */
#include <VBox/VBoxKeyboard.h>

/* External includes: */
#include <X11/XKBlib.h>
#include <X11/keysym.h>


/* VBoxKeyboard uses the deprecated XKeycodeToKeysym(3) API, but uses it safely.
 */
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

static unsigned gfByLayoutOK = 1;
static unsigned gfByTypeOK = 1;
static unsigned gfByXkbOK = 1;

/**
 * DEBUG function
 * Print a key to the release log in the format needed for the Wine
 * layout tables.
 */
static void printKey(Display *display, int keyc)
{
    bool was_escape = false;

    for (int i = 0; i < 2; ++i)
    {
        int keysym = XKeycodeToKeysym (display, keyc, i);

        int val = keysym & 0xff;
        if ('\\' == val)
        {
            LogRel(("\\\\"));
        }
        else if ('"' == val)
        {
            LogRel(("\\\""));
        }
        else if ((val > 32) && (val < 127))
        {
            if (
                   was_escape
                && (
                       ((val >= '0') && (val <= '9'))
                    || ((val >= 'A') && (val <= 'F'))
                    || ((val >= 'a') && (val <= 'f'))
                   )
               )
            {
                LogRel(("\"\""));
            }
            LogRel(("%c", (char) val));
        }
        else
        {
            LogRel(("\\x%x", val));
            was_escape = true;
        }
    }
}

/**
 * DEBUG function
 * Dump the keyboard layout to the release log.
 */
static void dumpLayout(Display *display)
{
    LogRel(("Your keyboard layout does not appear to be fully supported by\n"
            "VirtualBox. If you are experiencing keyboard problems this.\n"
            "information may help us to resolve them.\n"
            "(Note: please tell us if you are using a custom layout.)\n\n"
            "The correct table for your layout is:\n"));
    /* First, build up a table of scan-to-key code mappings */
    unsigned scanToKeycode[512] = { 0 };
    int minKey, maxKey;
    XDisplayKeycodes(display, &minKey, &maxKey);
    for (int i = minKey; i < maxKey; ++i)
        scanToKeycode[X11DRV_KeyEvent(display, i)] = i;
    LogRel(("\""));
    printKey(display, scanToKeycode[0x29]); /* `~ */
    for (int i = 2; i <= 0xd; ++i) /* 1! - =+ */
    {
        LogRel(("\",\""));
        printKey(display, scanToKeycode[i]);
    }
    LogRel(("\",\n"));
    LogRel(("\""));
    printKey(display, scanToKeycode[0x10]); /* qQ */
    for (int i = 0x11; i <= 0x1b; ++i) /* wW - ]} */
    {
        LogRel(("\",\""));
        printKey(display, scanToKeycode[i]);
    }
    LogRel(("\",\n"));
    LogRel(("\""));
    printKey(display, scanToKeycode[0x1e]); /* aA */
    for (int i = 0x1f; i <= 0x28; ++i) /* sS - '" */
    {
        LogRel(("\",\""));
        printKey(display, scanToKeycode[i]);
    }
    LogRel(("\",\""));
    printKey(display, scanToKeycode[0x2b]); /* \| */
    LogRel(("\",\n"));
    LogRel(("\""));
    printKey(display, scanToKeycode[0x2c]); /* zZ */
    for (int i = 0x2d; i <= 0x35; ++i) /* xX - /? */
    {
        LogRel(("\",\""));
        printKey(display, scanToKeycode[i]);
    }
    LogRel(("\",\""));
    printKey(display, scanToKeycode[0x56]); /* The 102nd key */
    LogRel(("\",\""));
    printKey(display, scanToKeycode[0x73]); /* The Brazilian key */
    LogRel(("\",\""));
    printKey(display, scanToKeycode[0x7d]); /* The Yen key */
    LogRel(("\"\n\n"));
}

/**
 * DEBUG function
 * Dump the keyboard type tables to the release log.
 */
static void dumpType(Display *display)
{
    LogRel(("Your keyboard type does not appear to be known to VirtualBox. If\n"
            "you are experiencing keyboard problems this information may help us\n"
            "to resolve them.  Please also provide information about what type\n"
            "of keyboard you have and whether you are using a remote X server or\n"
            "something similar.\n\n"
            "The tables for your keyboard are:\n"));
    for (unsigned i = 0; i < 256; ++i)
    {
        LogRel(("0x%x", X11DRV_KeyEvent(display, i)));
        if (i < 255)
            LogRel((", "));
        if (15 == (i % 16))
            LogRel(("\n"));
    }
    LogRel(("and\n"));
    LogRel(("NULL, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x,\n0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
            XKeysymToKeycode(display, XK_Control_L),
            XKeysymToKeycode(display, XK_Shift_L),
            XKeysymToKeycode(display, XK_Caps_Lock),
            XKeysymToKeycode(display, XK_Tab),
            XKeysymToKeycode(display, XK_Escape),
            XKeysymToKeycode(display, XK_Return),
            XKeysymToKeycode(display, XK_Up),
            XKeysymToKeycode(display, XK_Down),
            XKeysymToKeycode(display, XK_Left),
            XKeysymToKeycode(display, XK_Right),
            XKeysymToKeycode(display, XK_F1),
            XKeysymToKeycode(display, XK_F2),
            XKeysymToKeycode(display, XK_F3),
            XKeysymToKeycode(display, XK_F4),
            XKeysymToKeycode(display, XK_F5),
            XKeysymToKeycode(display, XK_F6),
            XKeysymToKeycode(display, XK_F7),
            XKeysymToKeycode(display, XK_F8)));
}

/*
 * This function builds a table mapping the X server's scan codes to PC
 * keyboard scan codes.  The logic of the function is that while the
 * X server may be using a different set of scan codes (if for example
 * it is running on a non-PC machine), the keyboard layout should be similar
 * to a PC layout.  So we look at the symbols attached to each key on the
 * X server, find the PC layout which is closest to it and remember the
 * mappings.
 */
bool initXKeyboard(Display *dpy, int (*remapScancodes)[2])
{
    X11DRV_InitKeyboard(dpy, &gfByLayoutOK, &gfByTypeOK, &gfByXkbOK,
                        remapScancodes);
    /* It will almost always work to some extent */
    return true;
}

/**
 * Do deferred logging after initialisation
 */
void doXKeyboardLogging(Display *dpy)
{
    if (((1 == gfByTypeOK) || (1 == gfByXkbOK)) && (gfByLayoutOK != 1))
        dumpLayout(dpy);
    if (((1 == gfByLayoutOK) || (1 == gfByXkbOK)) && (gfByTypeOK != 1))
        dumpType(dpy);
    if ((gfByLayoutOK != 1) && (gfByTypeOK != 1) && (gfByXkbOK != 1))
    {
        LogRel(("Failed to recognize the keyboard mapping or to guess it based on\n"
                "the keyboard layout.  It is very likely that some keys will not\n"
                "work correctly in the guest.  If this is the case, please submit\n"
                "a bug report, giving us information about your keyboard type,\n"
                "its layout and other relevant information such as whether you\n"
                "are using a remote X server or something similar. \n"));
        unsigned *keyc2scan = X11DRV_getKeyc2scan();

        LogRel(("The keycode-to-scancode table is: %d=%d",0,keyc2scan[0]));
        for (int i = 1; i < 256; i++)
            LogRel((",%d=%d",i,keyc2scan[i]));
        LogRel(("\n"));
    }
    LogRel(("X Server details: vendor: %s, release: %d, protocol version: %d.%d, display string: %s\n",
            ServerVendor(dpy), VendorRelease(dpy), ProtocolVersion(dpy),
            ProtocolRevision(dpy), DisplayString(dpy)));
    LogRel(("Using %s for keycode to scan code conversion\n",
              gfByXkbOK ? "XKB"
            : gfByTypeOK ? "known keycode mapping"
            : "host keyboard layout detection"));
}

/*
 * Translate an X server scancode to a PC keyboard scancode.
 */
unsigned handleXKeyEvent(Display *pDisplay, unsigned int iDetail)
{
    // call the WINE event handler
    unsigned key = X11DRV_KeyEvent(pDisplay, iDetail);
    LogRel3(("VBoxKeyboard: converting keycode %d to scancode %s0x%x\n",
             iDetail, key > 0x100 ? "0xe0 " : "", key & 0xff));
    return key;
}

/**
 * Initialize X11 keyboard including the remapping specified in the
 * global property GUI/RemapScancodes. This property is a string of
 * comma-separated x=y pairs, where x is the X11 keycode and y is the
 * keyboard scancode that is emitted when the key attached to the X11
 * keycode is pressed.
 */
void initMappedX11Keyboard(Display *pDisplay, const QString &remapScancodes)
{
    int (*scancodes)[2] = NULL;
    int (*scancodesTail)[2] = NULL;

    if (remapScancodes != QString::null)
    {
        QStringList tuples = remapScancodes.split(",", QString::SkipEmptyParts);
        scancodes = scancodesTail = new int [tuples.size()+1][2];
        for (int i = 0; i < tuples.size(); ++i)
        {
            QStringList keyc2scan = tuples.at(i).split("=");
            (*scancodesTail)[0] = keyc2scan.at(0).toUInt();
            (*scancodesTail)[1] = keyc2scan.at(1).toUInt();
            /* Do not advance on (ignore) identity mappings as this is
               the stop signal to initXKeyboard and friends */
            if ((*scancodesTail)[0] != (*scancodesTail)[1])
                scancodesTail++;
        }
        (*scancodesTail)[0] = (*scancodesTail)[1] = 0;
    }
    /* initialize the X keyboard subsystem */
    initXKeyboard (pDisplay ,scancodes);

    if (scancodes)
        delete scancodes;
}

unsigned long wrapXkbKeycodeToKeysym(Display *pDisplay, unsigned char cCode,
                                     unsigned int cGroup, unsigned int cIndex)
{
    KeySym cSym = XkbKeycodeToKeysym(pDisplay, cCode, cGroup, cIndex);
    if (cSym != NoSymbol)
        return cSym;
    return XKeycodeToKeysym(pDisplay, cCode, cGroup * 2 + cIndex % 2);
}
