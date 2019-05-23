/* $Id: KeyboardImpl.h $ */
/** @file
 * VirtualBox COM class implementation
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

#ifndef ____H_KEYBOARDIMPL
#define ____H_KEYBOARDIMPL

#include "KeyboardWrap.h"
#include "EventImpl.h"

#include <VBox/vmm/pdmdrv.h>

/** Limit of simultaneously attached devices (just USB and/or PS/2). */
enum { KEYBOARD_MAX_DEVICES = 2 };

/** Simple keyboard event class. */
class KeyboardEvent
{
public:
    KeyboardEvent() : scan(-1) {}
    KeyboardEvent(int _scan) : scan(_scan) {}
    bool i_isValid()
    {
        return (scan & ~0x80) && !(scan & ~0xFF);
    }
    int scan;
};
class Console;

class ATL_NO_VTABLE Keyboard :
    public KeyboardWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(Keyboard)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Console *aParent);
    void uninit();

    static const PDMDRVREG  DrvReg;

    Console *i_getParent() const
    {
        return mParent;
    }

private:

    // Wrapped Keyboard properties
    HRESULT getEventSource(ComPtr<IEventSource> &aEventSource);
    HRESULT getKeyboardLEDs(std::vector<KeyboardLED_T> &aKeyboardLEDs);

    // Wrapped Keyboard members
    HRESULT putScancode(LONG aScancode);
    HRESULT putScancodes(const std::vector<LONG> &aScancodes,
                         ULONG *aCodesStored);
    HRESULT putCAD();
    HRESULT releaseKeys();

    static DECLCALLBACK(void)   i_keyboardLedStatusChange(PPDMIKEYBOARDCONNECTOR pInterface, PDMKEYBLEDS enmLeds);
    static DECLCALLBACK(void)   i_keyboardSetActive(PPDMIKEYBOARDCONNECTOR pInterface, bool fActive);
    static DECLCALLBACK(void *) i_drvQueryInterface(PPDMIBASE pInterface, const char *pszIID);
    static DECLCALLBACK(int)    i_drvConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags);
    static DECLCALLBACK(void)   i_drvDestruct(PPDMDRVINS pDrvIns);

    void onKeyboardLedsChange(PDMKEYBLEDS enmLeds);

    Console * const         mParent;
    /** Pointer to the associated keyboard driver(s). */
    struct DRVMAINKEYBOARD *mpDrv[KEYBOARD_MAX_DEVICES];

    /* The current guest keyboard LED status. */
    PDMKEYBLEDS menmLeds;

    const ComObjPtr<EventSource> mEventSource;
};

#endif // !____H_KEYBOARDIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
