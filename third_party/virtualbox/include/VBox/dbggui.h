/** @file
 * DBGGUI - The VirtualBox Debugger GUI. (VBoxDbg)
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

#ifndef ___VBox_dbggui_h
#define ___VBox_dbggui_h

#include <VBox/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_dbggui    VirtualBox Debugger GUI
 * @ingroup grp_dbg
 * @{
 */

#ifdef RT_OS_WINDOWS
struct ISession;
#else
class ISession;
#endif

/** Pointer to the debugger GUI instance structure. */
typedef struct DBGGUI *PDBGGUI;

/** Virtual method table for the debugger GUI. */
typedef struct DBGGUIVT
{
    /** The version. (DBGGUIVT_VERSION) */
    uint32_t u32Version;
    /** @copydoc DBGGuiDestroy */
    DECLCALLBACKMEMBER(int,  pfnDestroy)(PDBGGUI pGui);
    /** @copydoc DBGGuiAdjustRelativePos */
    DECLCALLBACKMEMBER(void, pfnAdjustRelativePos)(PDBGGUI pGui, int x, int y, unsigned cx, unsigned cy);
    /** @copydoc DBGGuiShowStatistics */
    DECLCALLBACKMEMBER(int,  pfnShowStatistics)(PDBGGUI pGui);
    /** @copydoc DBGGuiShowCommandLine */
    DECLCALLBACKMEMBER(int,  pfnShowCommandLine)(PDBGGUI pGui);
    /** @copydoc DBGGuiSetParent */
    DECLCALLBACKMEMBER(void, pfnSetParent)(PDBGGUI pGui, void *pvParent);
    /** @copydoc DBGGuiSetMenu */
    DECLCALLBACKMEMBER(void, pfnSetMenu)(PDBGGUI pGui, void *pvMenu);
    /** The end version. (DBGGUIVT_VERSION) */
    uint32_t u32EndVersion;
} DBGGUIVT;
/** Pointer to the virtual method table for the debugger GUI. */
typedef DBGGUIVT const *PCDBGGUIVT;
/** The u32Version value.
 * The first byte is the minor version, the 2nd byte is major version number.
 * The high 16-bit word is a magic.  */
#define DBGGUIVT_VERSION    UINT32_C(0xbead0100)
/** Macro for determining whether two versions are compatible or not.
 * @returns boolean result.
 * @param   uVer1   The first version number.
 * @param   uVer2   The second version number.
 */
#define DBGGUIVT_ARE_VERSIONS_COMPATIBLE(uVer1, uVer2) \
    ( ((uVer1) & UINT32_C(0xffffff00)) == ((uVer2) & UINT32_C(0xffffff00)) )


/**
 * Creates the debugger GUI.
 *
 * @returns VBox status code.
 * @param   pSession    The VirtualBox session.
 * @param   ppGui       Where to store the pointer to the debugger instance.
 * @param   ppGuiVT     Where to store the virtual method table pointer.
 *                      Optional.
 */
DBGDECL(int) DBGGuiCreate(ISession *pSession, PDBGGUI *ppGui, PCDBGGUIVT *ppGuiVT);
/** @copydoc DBGGuiCreate */
typedef DECLCALLBACK(int) FNDBGGUICREATE(ISession *pSession, PDBGGUI *ppGui, PCDBGGUIVT *ppGuiVT);
/** Pointer to DBGGuiCreate. */
typedef FNDBGGUICREATE *PFNDBGGUICREATE;

/**
 * Creates the debugger GUI given a VM handle.
 *
 * @returns VBox status code.
 * @param   pUVM        The VM handle.
 * @param   ppGui       Where to store the pointer to the debugger instance.
 * @param   ppGuiVT     Where to store the virtual method table pointer.
 *                      Optional.
 */
DBGDECL(int) DBGGuiCreateForVM(PUVM pUVM, PDBGGUI *ppGui, PCDBGGUIVT *ppGuiVT);
/** @copydoc DBGGuiCreateForVM */
typedef DECLCALLBACK(int) FNDBGGUICREATEFORVM(PUVM pUVM, PDBGGUI *ppGui, PCDBGGUIVT *ppGuiVT);
/** Pointer to DBGGuiCreateForVM. */
typedef FNDBGGUICREATEFORVM *PFNDBGGUICREATEFORVM;

/**
 * Destroys the debugger GUI.
 *
 * @returns VBox status code.
 * @param   pGui        The instance returned by DBGGuiCreate().
 */
DBGDECL(int) DBGGuiDestroy(PDBGGUI pGui);

/**
 * Notifies the debugger GUI that the console window (or whatever) has changed
 * size or position.
 *
 * @param   pGui        The instance returned by DBGGuiCreate().
 * @param   x           The x-coordinate of the window the debugger is relative to.
 * @param   y           The y-coordinate of the window the debugger is relative to.
 * @param   cx          The width of the window the debugger is relative to.
 * @param   cy          The height of the window the debugger is relative to.
 */
DBGDECL(void) DBGGuiAdjustRelativePos(PDBGGUI pGui, int x, int y, unsigned cx, unsigned cy);

/**
 * Shows the default statistics window.
 *
 * @returns VBox status code.
 * @param   pGui        The instance returned by DBGGuiCreate().
 */
DBGDECL(int) DBGGuiShowStatistics(PDBGGUI pGui);

/**
 * Shows the default command line window.
 *
 * @returns VBox status code.
 * @param   pGui        The instance returned by DBGGuiCreate().
 */
DBGDECL(int) DBGGuiShowCommandLine(PDBGGUI pGui);

/**
 * Sets the parent windows.
 *
 * @param   pGui        The instance returned by DBGGuiCreate().
 * @param   pvParent    Pointer to a QWidget object.
 *
 * @remarks This will no affect any existing windows, so call it right after
 *          creating the thing.
 */
DBGDECL(void) DBGGuiSetParent(PDBGGUI pGui, void *pvParent);

/**
 * Sets the debug menu object.
 *
 * @param   pGui        The instance returned by DBGGuiCreate().
 * @param   pvMenu      Pointer to a QMenu object.
 *
 * @remarks Call right after creation or risk losing menu item.
 */
DBGDECL(void) DBGGuiSetMenu(PDBGGUI pGui, void *pvMenu);

/** @} */

RT_C_DECLS_END

#endif

