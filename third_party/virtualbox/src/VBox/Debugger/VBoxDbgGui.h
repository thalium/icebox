/* $Id: VBoxDbgGui.h $ */
/** @file
 * VBox Debugger GUI - The Manager.
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

#ifndef ___Debugger_VBoxDbgGui_h
#define ___Debugger_VBoxDbgGui_h

// VirtualBox COM interfaces declarations (generated header)
#ifdef VBOX_WITH_XPCOM
# include <VirtualBox_XPCOM.h>
#else
# include <VirtualBox.h>
#endif

#include "VBoxDbgStatsQt.h"
#include "VBoxDbgConsole.h"


/**
 * The Debugger GUI manager class.
 *
 * It's job is to provide a C callable external interface and manage the
 * windows and bit making up the debugger GUI.
 */
class VBoxDbgGui : public QObject
{
    Q_OBJECT;

public:
    /**
     * Create a default VBoxDbgGui object.
     */
    VBoxDbgGui();

    /**
     * Initializes a VBoxDbgGui object by ISession.
     *
     * @returns VBox status code.
     * @param   pSession    VBox Session object.
     */
    int init(ISession *pSession);

    /**
     * Initializes a VBoxDbgGui object by VM handle.
     *
     * @returns VBox status code.
     * @param   pUVM        The user mode VM handle. The caller's reference will be
     *                      consumed on success.
     */
    int init(PUVM pUVM);

    /**
     * Destroys the VBoxDbgGui object.
     */
    virtual ~VBoxDbgGui();

    /**
     * Sets the parent widget.
     *
     * @param   pParent     New parent widget.
     * @remarks This only affects new windows.
     */
    void setParent(QWidget *pParent);

    /**
     * Sets the menu object.
     *
     * @param   pMenu       New menu object.
     * @remarks This only affects new menu additions.
     */
    void setMenu(QMenu *pMenu);

    /**
     * Show the default statistics window, creating it if necessary.
     *
     * @returns VBox status code.
     */
    int showStatistics();

    /**
     * Repositions and resizes (optionally) the statistics to its defaults
     *
     * @param   fResize     If set (default) the size of window is also changed.
     */
    void repositionStatistics(bool fResize = true);

    /**
     * Show the console window (aka. command line), creating it if necessary.
     *
     * @returns VBox status code.
     */
    int showConsole();

    /**
     * Repositions and resizes (optionally) the console to its defaults
     *
     * @param   fResize     If set (default) the size of window is also changed.
     */
    void repositionConsole(bool fResize = true);

    /**
     * Update the desktop size.
     * This is called whenever the reference window changes position.
     */
    void updateDesktopSize();

    /**
     * Notifies the debugger GUI that the console window (or whatever) has changed
     * size or position.
     *
     * @param   x           The x-coordinate of the window the debugger is relative to.
     * @param   y           The y-coordinate of the window the debugger is relative to.
     * @param   cx          The width of the window the debugger is relative to.
     * @param   cy          The height of the window the debugger is relative to.
     */
    void adjustRelativePos(int x, int y, unsigned cx, unsigned cy);

    /**
     * Gets the user mode VM handle.
     * @returns The UVM handle.
     */
    PUVM getUvmHandle() const
    {
        return m_pUVM;
    }


protected slots:
    /**
     * Notify that a child object (i.e. a window is begin destroyed).
     * @param   pObj    The object which is being destroyed.
     */
    void notifyChildDestroyed(QObject *pObj);

protected:

    /** The debugger statistics. */
    VBoxDbgStats *m_pDbgStats;
    /** The debugger console (aka. command line). */
    VBoxDbgConsole *m_pDbgConsole;

    /** The VirtualBox session. */
    ISession *m_pSession;
    /** The VirtualBox console. */
    IConsole *m_pConsole;
    /** The VirtualBox Machine Debugger. */
    IMachineDebugger *m_pMachineDebugger;
    /** The VirtualBox Machine. */
    IMachine *m_pMachine;
    /** The VM instance. */
    PVM m_pVM;
    /** The user mode VM handle. */
    PUVM m_pUVM;

    /** The parent widget. */
    QWidget *m_pParent;
    /** The menu object for the 'debug' menu. */
    QMenu *m_pMenu;

    /** The x-coordinate of the window we're relative to. */
    int m_x;
    /** The y-coordinate of the window we're relative to. */
    int m_y;
    /** The width of the window we're relative to. */
    unsigned m_cx;
    /** The height of the window we're relative to. */
    unsigned m_cy;
    /** The x-coordinate of the desktop. */
    int m_xDesktop;
    /** The y-coordinate of the desktop. */
    int m_yDesktop;
    /** The size of the desktop. */
    unsigned m_cxDesktop;
    /** The size of the desktop. */
    unsigned m_cyDesktop;
};


#endif

