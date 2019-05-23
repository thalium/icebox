/** @file
 *
 * Shared Clipboard:
 * X11 backend code.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Note: to automatically run regression tests on the shared clipboard,
 * execute the tstClipboardX11 testcase.  If you often make changes to the
 * clipboard code, adding the line
 *   OTHERS += $(PATH_tstClipboardX11)/tstClipboardX11.run
 * to LocalConfig.kmk will cause the tests to be run every time the code is
 * changed. */

#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD

#include <errno.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef RT_OS_SOLARIS
#include <tsol/label.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xproto.h>
#include <X11/StringDefs.h>

#include <iprt/types.h>
#include <iprt/mem.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>

#include <VBox/log.h>

#include <VBox/GuestHost/SharedClipboard.h>
#include <VBox/GuestHost/clipboard-helper.h>
#include <VBox/HostServices/VBoxClipboardSvc.h>

class formats;
static Atom clipGetAtom(CLIPBACKEND *pCtx, const char *pszName);

/** The different clipboard formats which we support. */
enum CLIPFORMAT
{
    INVALID = 0,
    TARGETS,
    TEXT,  /* Treat this as Utf8, but it may really be ascii */
    UTF8,
    BMP,
        HTML
};

/** The table mapping X11 names to data formats and to the corresponding
 * VBox clipboard formats (currently only Unicode) */
static struct _CLIPFORMATTABLE
{
    /** The X11 atom name of the format (several names can match one format)
     */
    const char *pcszAtom;
    /** The format corresponding to the name */
    CLIPFORMAT  enmFormat;
    /** The corresponding VBox clipboard format */
    uint32_t    u32VBoxFormat;
} g_aFormats[] =
{
    { "INVALID", INVALID, 0 },
    { "UTF8_STRING", UTF8, VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT },
    { "text/plain;charset=UTF-8", UTF8,
      VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT },
    { "text/plain;charset=utf-8", UTF8,
      VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT },
    { "STRING", TEXT, VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT },
    { "TEXT", TEXT, VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT },
    { "text/plain", TEXT, VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT },
    { "text/html", HTML, VBOX_SHARED_CLIPBOARD_FMT_HTML },
    { "text/html;charset=utf-8", HTML,
      VBOX_SHARED_CLIPBOARD_FMT_HTML },
    { "image/bmp", BMP, VBOX_SHARED_CLIPBOARD_FMT_BITMAP },
    { "image/x-bmp", BMP, VBOX_SHARED_CLIPBOARD_FMT_BITMAP },
    { "image/x-MS-bmp", BMP, VBOX_SHARED_CLIPBOARD_FMT_BITMAP }


    /** @todo Inkscape exports image/png but not bmp... */
};

typedef unsigned CLIPX11FORMAT;

enum
{
    NIL_CLIPX11FORMAT = 0,
    MAX_CLIP_X11_FORMATS = RT_ELEMENTS(g_aFormats)
};

/** Return the atom corresponding to a supported X11 format.
 * @param widget a valid Xt widget
 */
static Atom clipAtomForX11Format(CLIPBACKEND *pCtx, CLIPX11FORMAT format)
{
    return clipGetAtom(pCtx, g_aFormats[format].pcszAtom);
}

/** Return the CLIPFORMAT corresponding to a supported X11 format. */
static CLIPFORMAT clipRealFormatForX11Format(CLIPX11FORMAT format)
{
    return g_aFormats[format].enmFormat;
}

/** Return the atom corresponding to a supported X11 format. */
static uint32_t clipVBoxFormatForX11Format(CLIPX11FORMAT format)
{
    return g_aFormats[format].u32VBoxFormat;
}

/** Lookup the X11 format matching a given X11 atom.
 * @returns the format on success, NIL_CLIPX11FORMAT on failure
 * @param   widget a valid Xt widget
 */
static CLIPX11FORMAT clipFindX11FormatByAtom(CLIPBACKEND *pCtx, Atom atomFormat)
{
    for (unsigned i = 0; i < RT_ELEMENTS(g_aFormats); ++i)
        if (clipAtomForX11Format(pCtx, i) == atomFormat)
            return i;
    return NIL_CLIPX11FORMAT;
}

#ifdef TESTCASE
/** Lookup the X11 format matching a given X11 atom text.
 * @returns the format on success, NIL_CLIPX11FORMAT on failure
 * @param   widget a valid Xt widget
 */
static CLIPX11FORMAT clipFindX11FormatByAtomText(const char *pcsz)
{
    for (unsigned i = 0; i < RT_ELEMENTS(g_aFormats); ++i)
        if (!strcmp(g_aFormats[i].pcszAtom, pcsz))
            return i;
    return NIL_CLIPX11FORMAT;
}
#endif

/**
 * Enumerates supported X11 clipboard formats corresponding to a given VBox
 * format.
 * @returns the next matching X11 format in the list, or NIL_CLIPX11FORMAT if
 *          there are no more
 * @param lastFormat  The value returned from the last call of this function.
 *                    Use NIL_CLIPX11FORMAT to start the enumeration.
 */
static CLIPX11FORMAT clipEnumX11Formats(uint32_t u32VBoxFormats,
                                        CLIPX11FORMAT lastFormat)
{
    for (unsigned i = lastFormat + 1; i < RT_ELEMENTS(g_aFormats); ++i)
        if (u32VBoxFormats & clipVBoxFormatForX11Format(i))
            return i;
    return NIL_CLIPX11FORMAT;
}

/** Global context information used by the X11 clipboard backend */
struct _CLIPBACKEND
{
    /** Opaque data structure describing the front-end. */
    VBOXCLIPBOARDCONTEXT *pFrontend;
    /** Is an X server actually available? */
    bool fHaveX11;
    /** The X Toolkit application context structure */
    XtAppContext appContext;

    /** We have a separate thread to wait for Window and Clipboard events */
    RTTHREAD thread;
    /** The X Toolkit widget which we use as our clipboard client.  It is never made visible. */
    Widget widget;

    /** Should we try to grab the clipboard on startup? */
    bool fGrabClipboardOnStart;

    /** The best text format X11 has to offer, as an index into the formats
     * table */
    CLIPX11FORMAT X11TextFormat;
    /** The best bitmap format X11 has to offer, as an index into the formats
     * table */
    CLIPX11FORMAT X11BitmapFormat;
    /** The best HTML format X11 has to offer, as an index into the formats
     * table */
    CLIPX11FORMAT X11HTMLFormat;
    /** What formats does VBox have on offer? */
    uint32_t vboxFormats;
    /** Cache of the last unicode data that we received */
    void *pvUnicodeCache;
    /** Size of the unicode data in the cache */
    uint32_t cbUnicodeCache;
    /** When we wish the clipboard to exit, we have to wake up the event
     * loop.  We do this by writing into a pipe.  This end of the pipe is
     * the end that another thread can write to. */
    int wakeupPipeWrite;
    /** The reader end of the pipe */
    int wakeupPipeRead;
    /** A pointer to the XFixesSelectSelectionInput function */
    void (*fixesSelectInput)(Display *, Window, Atom, unsigned long);
    /** The first XFixes event number */
    int fixesEventBase;
    /** The Xt Intrinsics can only handle one outstanding clipboard operation
     * at a time, so we keep track of whether one is in process. */
    bool fBusy;
    /** We can't handle a clipboard update event while we are busy, so remember
     * it for later. */
    bool fUpdateNeeded;
};

/** The number of simultaneous instances we support.  For all normal purposes
 * we should never need more than one.  For the testcase it is convenient to
 * have a second instance that the first can interact with in order to have
 * a more controlled environment. */
enum { CLIP_MAX_CONTEXTS = 20 };

/** Array of structures for mapping Xt widgets to context pointers.  We
 * need this because the widget clipboard callbacks do not pass user data. */
static struct {
    /** The widget we want to associate the context with */
    Widget widget;
    /** The context associated with the widget */
    CLIPBACKEND *pCtx;
} g_contexts[CLIP_MAX_CONTEXTS];

/** Register a new X11 clipboard context. */
static int clipRegisterContext(CLIPBACKEND *pCtx)
{
    bool found = false;
    AssertReturn(pCtx != NULL, VERR_INVALID_PARAMETER);
    Widget widget = pCtx->widget;
    AssertReturn(widget != NULL, VERR_INVALID_PARAMETER);
    for (unsigned i = 0; i < RT_ELEMENTS(g_contexts); ++i)
    {
        AssertReturn(   (g_contexts[i].widget != widget)
                     && (g_contexts[i].pCtx != pCtx), VERR_WRONG_ORDER);
        if (g_contexts[i].widget == NULL && !found)
        {
            AssertReturn(g_contexts[i].pCtx == NULL, VERR_INTERNAL_ERROR);
            g_contexts[i].widget = widget;
            g_contexts[i].pCtx = pCtx;
            found = true;
        }
    }
    return found ? VINF_SUCCESS : VERR_OUT_OF_RESOURCES;
}

/** Unregister an X11 clipboard context. */
static void clipUnregisterContext(CLIPBACKEND *pCtx)
{
    bool found = false;
    AssertReturnVoid(pCtx != NULL);
    Widget widget = pCtx->widget;
    AssertReturnVoid(widget != NULL);
    for (unsigned i = 0; i < RT_ELEMENTS(g_contexts); ++i)
    {
        Assert(!found || g_contexts[i].widget != widget);
        if (g_contexts[i].widget == widget)
        {
            Assert(g_contexts[i].pCtx != NULL);
            g_contexts[i].widget = NULL;
            g_contexts[i].pCtx = NULL;
            found = true;
        }
    }
}

/** Find an X11 clipboard context. */
static CLIPBACKEND *clipLookupContext(Widget widget)
{
    AssertReturn(widget != NULL, NULL);
    for (unsigned i = 0; i < RT_ELEMENTS(g_contexts); ++i)
    {
        if (g_contexts[i].widget == widget)
        {
            Assert(g_contexts[i].pCtx != NULL);
            return g_contexts[i].pCtx;
        }
    }
    return NULL;
}

/** Convert an atom name string to an X11 atom, looking it up in a cache
 * before asking the server */
static Atom clipGetAtom(CLIPBACKEND *pCtx, const char *pszName)
{
    AssertPtrReturn(pszName, None);
    return XInternAtom(XtDisplay(pCtx->widget), pszName, False);
}

#ifdef TESTCASE
static void testQueueToEventThread(void (*proc)(void *, void *),
                                   void *client_data);
#endif

/** String written to the wakeup pipe. */
#define WAKE_UP_STRING      "WakeUp!"
/** Length of the string written. */
#define WAKE_UP_STRING_LEN  ( sizeof(WAKE_UP_STRING) - 1 )

/** Schedule a function call to run on the Xt event thread by passing it to
 * the application context as a 0ms timeout and waking up the event loop by
 * writing to the wakeup pipe which it monitors. */
void clipQueueToEventThread(CLIPBACKEND *pCtx,
                            void (*proc)(void *, void *),
                            void *client_data)
{
    LogRel2(("clipQueueToEventThread: proc=%p, client_data=%p\n",
             proc, client_data));
#ifndef TESTCASE
    XtAppAddTimeOut(pCtx->appContext, 0, (XtTimerCallbackProc)proc,
                    (XtPointer)client_data);
    ssize_t cbWritten = write(pCtx->wakeupPipeWrite, WAKE_UP_STRING, WAKE_UP_STRING_LEN);
    NOREF(cbWritten);
#else
    RT_NOREF1(pCtx);
    testQueueToEventThread(proc, client_data);
#endif
}

/**
 * Report the formats currently supported by the X11 clipboard to VBox.
 */
static void clipReportFormatsToVBox(CLIPBACKEND *pCtx)
{
    uint32_t u32VBoxFormats = clipVBoxFormatForX11Format(pCtx->X11TextFormat);
    u32VBoxFormats |= clipVBoxFormatForX11Format(pCtx->X11BitmapFormat);
    u32VBoxFormats |= clipVBoxFormatForX11Format(pCtx->X11HTMLFormat);
    LogRelFlowFunc(("clipReportFormatsToVBox format: %d\n", u32VBoxFormats));
    LogRelFlowFunc(("clipReportFormatsToVBox txt: %d, bitm: %d, html:%d, u32VBoxFormats: %d\n",
                    pCtx->X11TextFormat, pCtx->X11BitmapFormat, pCtx->X11HTMLFormat,
                    u32VBoxFormats ));
    ClipReportX11Formats(pCtx->pFrontend, u32VBoxFormats);
}

/**
 * Forget which formats were previously in the X11 clipboard.  Called when we
 * grab the clipboard. */
static void clipResetX11Formats(CLIPBACKEND *pCtx)
{
    pCtx->X11TextFormat = INVALID;
    pCtx->X11BitmapFormat = INVALID;
    pCtx->X11HTMLFormat = INVALID;
}

/** Tell VBox that X11 currently has nothing in its clipboard. */
static void clipReportEmptyX11CB(CLIPBACKEND *pCtx)
{
    clipResetX11Formats(pCtx);
    clipReportFormatsToVBox(pCtx);
}

/**
 * Go through an array of X11 clipboard targets to see if they contain a text
 * format we can support, and if so choose the ones we prefer (e.g. we like
 * Utf8 better than plain text).
 * @param  pCtx      the clipboard backend context structure
 * @param  pTargets  the list of targets
 * @param  cTargets  the size of the list in @a pTargets
 */
static CLIPX11FORMAT clipGetTextFormatFromTargets(CLIPBACKEND *pCtx,
                                                  CLIPX11FORMAT *pTargets,
                                                  size_t cTargets)
{
    CLIPX11FORMAT bestTextFormat = NIL_CLIPX11FORMAT;
    CLIPFORMAT enmBestTextTarget = INVALID;
    AssertPtrReturn(pCtx, NIL_CLIPX11FORMAT);
    AssertReturn(VALID_PTR(pTargets) || cTargets == 0, NIL_CLIPX11FORMAT);
    for (unsigned i = 0; i < cTargets; ++i)
    {
        CLIPX11FORMAT format = pTargets[i];
        if (format != NIL_CLIPX11FORMAT)
        {
            if (   (clipVBoxFormatForX11Format(format)
                            == VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
                    && enmBestTextTarget < clipRealFormatForX11Format(format))
            {
                enmBestTextTarget = clipRealFormatForX11Format(format);
                bestTextFormat = format;
            }
        }
    }
    return bestTextFormat;
}

#ifdef TESTCASE
static bool clipTestTextFormatConversion(CLIPBACKEND *pCtx)
{
    bool success = true;
    CLIPX11FORMAT targets[2];
    CLIPX11FORMAT x11Format;
    targets[0] = clipFindX11FormatByAtomText("text/plain");
    targets[1] = clipFindX11FormatByAtomText("image/bmp");
    x11Format = clipGetTextFormatFromTargets(pCtx, targets, 2);
    if (clipRealFormatForX11Format(x11Format) != TEXT)
        success = false;
    targets[0] = clipFindX11FormatByAtomText("UTF8_STRING");
    targets[1] = clipFindX11FormatByAtomText("text/plain");
    x11Format = clipGetTextFormatFromTargets(pCtx, targets, 2);
    if (clipRealFormatForX11Format(x11Format) != UTF8)
        success = false;
    return success;
}
#endif

/**
 * Go through an array of X11 clipboard targets to see if they contain a bitmap
 * format we can support, and if so choose the ones we prefer (e.g. we like
 * BMP better than PNG because we don't have to convert).
 * @param  pCtx      the clipboard backend context structure
 * @param  pTargets  the list of targets
 * @param  cTargets  the size of the list in @a pTargets
 */
static CLIPX11FORMAT clipGetBitmapFormatFromTargets(CLIPBACKEND *pCtx,
                                                    CLIPX11FORMAT *pTargets,
                                                    size_t cTargets)
{
    CLIPX11FORMAT bestBitmapFormat = NIL_CLIPX11FORMAT;
    CLIPFORMAT enmBestBitmapTarget = INVALID;
    AssertPtrReturn(pCtx, NIL_CLIPX11FORMAT);
    AssertReturn(VALID_PTR(pTargets) || cTargets == 0, NIL_CLIPX11FORMAT);
    for (unsigned i = 0; i < cTargets; ++i)
    {
        CLIPX11FORMAT format = pTargets[i];
        if (format != NIL_CLIPX11FORMAT)
        {
            if (   (clipVBoxFormatForX11Format(format)
                            == VBOX_SHARED_CLIPBOARD_FMT_BITMAP)
                    && enmBestBitmapTarget < clipRealFormatForX11Format(format))
            {
                enmBestBitmapTarget = clipRealFormatForX11Format(format);
                bestBitmapFormat = format;
            }
        }
    }
    return bestBitmapFormat;
}

/**
 * Go through an array of X11 clipboard targets to see if they contain a HTML
 * format we can support, and if so choose the ones we prefer
 * @param  pCtx      the clipboard backend context structure
 * @param  pTargets  the list of targets
 * @param  cTargets  the size of the list in @a pTargets
 */
static CLIPX11FORMAT clipGetHtmlFormatFromTargets(CLIPBACKEND *pCtx,
                                                  CLIPX11FORMAT *pTargets,
                                                  size_t cTargets)
{
    CLIPX11FORMAT bestHTMLFormat = NIL_CLIPX11FORMAT;
    CLIPFORMAT enmBestHtmlTarget = INVALID;
    AssertPtrReturn(pCtx, NIL_CLIPX11FORMAT);
    AssertReturn(VALID_PTR(pTargets) || cTargets == 0, NIL_CLIPX11FORMAT);
    for (unsigned i = 0; i < cTargets; ++i)
    {
        CLIPX11FORMAT format = pTargets[i];
        if (format != NIL_CLIPX11FORMAT)
        {
            if (   (clipVBoxFormatForX11Format(format) == VBOX_SHARED_CLIPBOARD_FMT_HTML)
                && enmBestHtmlTarget < clipRealFormatForX11Format(format))
            {
                enmBestHtmlTarget = clipRealFormatForX11Format(format);
                bestHTMLFormat = format;
            }
        }
    }
    return bestHTMLFormat;
}


/**
 * Go through an array of X11 clipboard targets to see if we can support any
 * of them and if relevant to choose the ones we prefer (e.g. we like Utf8
 * better than plain text).
 * @param  pCtx      the clipboard backend context structure
 * @param  pTargets  the list of targets
 * @param  cTargets  the size of the list in @a pTargets
 */
static void clipGetFormatsFromTargets(CLIPBACKEND *pCtx,
                                      CLIPX11FORMAT *pTargets, size_t cTargets)
{
    AssertPtrReturnVoid(pCtx);
    AssertPtrReturnVoid(pTargets);
    CLIPX11FORMAT bestTextFormat;
    CLIPX11FORMAT bestBitmapFormat;
    CLIPX11FORMAT bestHtmlFormat;
    bestTextFormat = clipGetTextFormatFromTargets(pCtx, pTargets, cTargets);
    if (pCtx->X11TextFormat != bestTextFormat)
    {
        pCtx->X11TextFormat = bestTextFormat;
    }
    pCtx->X11BitmapFormat = INVALID;  /* not yet supported */
    bestBitmapFormat = clipGetBitmapFormatFromTargets(pCtx, pTargets, cTargets);
    if (pCtx->X11BitmapFormat != bestBitmapFormat)
    {
        pCtx->X11BitmapFormat = bestBitmapFormat;
    }
    bestHtmlFormat = clipGetHtmlFormatFromTargets(pCtx, pTargets, cTargets);
    if(pCtx->X11HTMLFormat != bestHtmlFormat)
    {
        pCtx->X11HTMLFormat = bestHtmlFormat;
    }
}

static void clipQueryX11CBFormats(CLIPBACKEND *pCtx);

/**
 * Update the context's information about targets currently supported by X11,
 * based on an array of X11 atoms.
 * @param  pCtx      the context to be updated
 * @param  pTargets  the array of atoms describing the targets supported
 * @param  cTargets  the size of the array @a pTargets
 */
static void clipUpdateX11Targets(CLIPBACKEND *pCtx, CLIPX11FORMAT *pTargets,
                                 size_t cTargets)
{
    LogRel2 (("%s: called\n", __FUNCTION__));
    pCtx->fBusy = false;
    if (pCtx->fUpdateNeeded)
    {
        /* We may already be out of date. */
        pCtx->fUpdateNeeded = false;
        clipQueryX11CBFormats(pCtx);
        return;
    }
    if (pTargets == NULL) {
        /* No data available */
        clipReportEmptyX11CB(pCtx);
        return;
    }
    clipGetFormatsFromTargets(pCtx, pTargets, cTargets);
    clipReportFormatsToVBox(pCtx);
}

/**
 * Notify the VBox clipboard about available data formats, based on the
 * "targets" information obtained from the X11 clipboard.
 * @note  Callback for XtGetSelectionValue
 * @note  This function is treated as API glue, and as such is not part of any
 *        unit test.  So keep it simple, be paranoid and log everything.
 */
static void clipConvertX11Targets(Widget widget, XtPointer pClientData,
                                  Atom * /* selection */, Atom *atomType,
                                  XtPointer pValue, long unsigned int *pcLen,
                                  int *piFormat)
{
    RT_NOREF1(piFormat);
    CLIPBACKEND *pCtx = reinterpret_cast<CLIPBACKEND *>(pClientData);
    Atom *pAtoms = (Atom *)pValue;
    unsigned i, j;
    LogRel2(("%s: pValue=%p, *pcLen=%u, *atomType=%d%s\n", __FUNCTION__,
             pValue, *pcLen, *atomType,
             *atomType == XT_CONVERT_FAIL ? " (XT_CONVERT_FAIL)" : ""));
    CLIPX11FORMAT *pFormats = NULL;
    if (*pcLen && pValue && (*atomType != XT_CONVERT_FAIL /* time out */))
        pFormats = (CLIPX11FORMAT *)RTMemAllocZ(*pcLen * sizeof(CLIPX11FORMAT));
#if defined(DEBUG) && !defined(TESTCASE)
    if (pValue)
    {
        for (i = 0; i < *pcLen; ++i)
            if (pAtoms[i])
            {
                char *pszName = XGetAtomName(XtDisplay(widget), pAtoms[i]);
                LogRel2(("%s: found target %s\n", __FUNCTION__,
                         pszName));
                XFree(pszName);
            }
            else
                LogRel2(("%s: found empty target.\n", __FUNCTION__));
    }
#endif
    if (pFormats)
    {
        for (i = 0; i < *pcLen; ++i)
        {
            for (j = 0; j < RT_ELEMENTS(g_aFormats); ++j)
            {
                Atom target = XInternAtom(XtDisplay(widget),
                                          g_aFormats[j].pcszAtom, False);
                if (*(pAtoms + i) == target)
                    pFormats[i] = j;
            }
#if defined(DEBUG) && !defined(TESTCASE)
            LogRel2(("%s: reporting format %d (%s)\n", __FUNCTION__,
                     pFormats[i], g_aFormats[pFormats[i]].pcszAtom));
#endif
        }
    }
    else
        LogRel2(("%s: reporting empty targets (none reported or allocation failure).\n",
                 __FUNCTION__));
    clipUpdateX11Targets(pCtx, pFormats, *pcLen);
    RTMemFree(pFormats);
    XtFree(reinterpret_cast<char *>(pValue));
}

#ifdef TESTCASE
    void testRequestTargets(CLIPBACKEND *pCtx);
#endif

/**
 * Callback to notify us when the contents of the X11 clipboard change.
 */
static void clipQueryX11CBFormats(CLIPBACKEND *pCtx)
{
    LogRel2 (("%s: requesting the targets that the X11 clipboard offers\n",
           __PRETTY_FUNCTION__));
    if (pCtx->fBusy)
    {
        pCtx->fUpdateNeeded = true;
        return;
    }
    pCtx->fBusy = true;
#ifndef TESTCASE
    XtGetSelectionValue(pCtx->widget,
                        clipGetAtom(pCtx, "CLIPBOARD"),
                        clipGetAtom(pCtx, "TARGETS"),
                        clipConvertX11Targets, pCtx,
                        CurrentTime);
#else
    testRequestTargets(pCtx);
#endif
}

#ifndef TESTCASE

typedef struct {
    int type;                   /* event base */
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    int subtype;
    Window owner;
    Atom selection;
    Time timestamp;
    Time selection_timestamp;
} XFixesSelectionNotifyEvent;

/**
 * Wait until an event arrives and handle it if it is an XFIXES selection
 * event, which Xt doesn't know about.
 */
void clipPeekEventAndDoXFixesHandling(CLIPBACKEND *pCtx)
{
    union
    {
        XEvent event;
        XFixesSelectionNotifyEvent fixes;
    } event = { { 0 } };

    if (XtAppPeekEvent(pCtx->appContext, &event.event))
        if (   (event.event.type == pCtx->fixesEventBase)
            && (event.fixes.owner != XtWindow(pCtx->widget)))
        {
            if (   (event.fixes.subtype == 0  /* XFixesSetSelectionOwnerNotify */)
                && (event.fixes.owner != 0))
                clipQueryX11CBFormats(pCtx);
            else
                clipReportEmptyX11CB(pCtx);
        }
}

/**
 * The main loop of our clipboard reader.
 * @note  X11 backend code.
 */
static DECLCALLBACK(int) clipEventThread(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF1(hThreadSelf);
    LogRel(("Shared clipboard: Starting shared clipboard thread\n"));

    CLIPBACKEND *pCtx = (CLIPBACKEND *)pvUser;

    if (pCtx->fGrabClipboardOnStart)
        clipQueryX11CBFormats(pCtx);
    while (XtAppGetExitFlag(pCtx->appContext) == FALSE)
    {
        clipPeekEventAndDoXFixesHandling(pCtx);
        XtAppProcessEvent(pCtx->appContext, XtIMAll);
    }
    LogRel(("Shared clipboard: Shared clipboard thread terminated successfully\n"));
    return VINF_SUCCESS;
}
#endif

/** X11 specific uninitialisation for the shared clipboard.
 * @note  X11 backend code.
 */
static void clipUninit(CLIPBACKEND *pCtx)
{
    AssertPtrReturnVoid(pCtx);
    if (pCtx->widget)
    {
        /* Valid widget + invalid appcontext = bug.  But don't return yet. */
        AssertPtr(pCtx->appContext);
        clipUnregisterContext(pCtx);
        XtDestroyWidget(pCtx->widget);
    }
    pCtx->widget = NULL;
    if (pCtx->appContext)
        XtDestroyApplicationContext(pCtx->appContext);
    pCtx->appContext = NULL;
    if (pCtx->wakeupPipeRead != 0)
        close(pCtx->wakeupPipeRead);
    if (pCtx->wakeupPipeWrite != 0)
        close(pCtx->wakeupPipeWrite);
    pCtx->wakeupPipeRead = 0;
    pCtx->wakeupPipeWrite = 0;
}

/** Worker function for stopping the clipboard which runs on the event
 * thread. */
static void clipStopEventThreadWorker(void *pUserData, void *)
{

    CLIPBACKEND *pCtx = (CLIPBACKEND *)pUserData;

    /* This might mean that we are getting stopped twice. */
    Assert(pCtx->widget != NULL);

    /* Set the termination flag to tell the Xt event loop to exit.  We
     * reiterate that any outstanding requests from the X11 event loop to
     * the VBox part *must* have returned before we do this. */
    XtAppSetExitFlag(pCtx->appContext);
}

#ifndef TESTCASE
/** Setup the XFixes library and load the XFixesSelectSelectionInput symbol */
static int clipLoadXFixes(Display *pDisplay, CLIPBACKEND *pCtx)
{
    int rc;

    void *hFixesLib = dlopen("libXfixes.so.1", RTLD_LAZY);
    if (!hFixesLib)
        hFixesLib = dlopen("libXfixes.so.2", RTLD_LAZY);
    if (!hFixesLib)
        hFixesLib = dlopen("libXfixes.so.3", RTLD_LAZY);
    if (!hFixesLib)
        hFixesLib = dlopen("libXfixes.so.4", RTLD_LAZY);
    if (hFixesLib)
    {
        /* For us, a NULL function pointer is a failure */
        pCtx->fixesSelectInput = (void (*)(Display *, Window, Atom, long unsigned int))
                                 (uintptr_t)dlsym(hFixesLib, "XFixesSelectSelectionInput");
        if (pCtx->fixesSelectInput)
        {
            int dummy1 = 0;
            int dummy2 = 0;
            if (XQueryExtension(pDisplay, "XFIXES", &dummy1, &pCtx->fixesEventBase, &dummy2) != 0)
            {
                if (pCtx->fixesEventBase >= 0)
                    rc = VINF_SUCCESS;
                else
                {
                    LogRel(("clipLoadXFixes: fixesEventBase is less than zero: %d\n", pCtx->fixesEventBase));
                    rc = VERR_NOT_SUPPORTED;
                }
            }
            else
            {
                LogRel(("clipLoadXFixes: XQueryExtension failed\n"));
                rc = VERR_NOT_SUPPORTED;
            }
        }
        else
        {
            LogRel(("clipLoadXFixes: Symbol XFixesSelectSelectionInput not found!\n"));
            rc = VERR_NOT_SUPPORTED;
        }
    }
    else
    {
        LogRel(("clipLoadXFixes: libxFixes.so.* not found!\n"));
        rc = VERR_NOT_SUPPORTED;
    }
    return rc;
}
#endif

/** This is the callback which is scheduled when data is available on the
 * wakeup pipe.  It simply reads all data from the pipe. */
static void clipDrainWakeupPipe(XtPointer pUserData, int *, XtInputId *)
{
    CLIPBACKEND *pCtx = (CLIPBACKEND *)pUserData;
    char acBuf[WAKE_UP_STRING_LEN];

    LogRel2(("clipDrainWakeupPipe: called\n"));
    while (read(pCtx->wakeupPipeRead, acBuf, sizeof(acBuf)) > 0) {}
}

/** X11 specific initialisation for the shared clipboard.
 * @note  X11 backend code.
 */
static int clipInit(CLIPBACKEND *pCtx)
{
    /* Create a window and make it a clipboard viewer. */
    int cArgc = 0;
    char *pcArgv = 0;
    int rc = VINF_SUCCESS;
    Display *pDisplay;

    /* Make sure we are thread safe */
    XtToolkitThreadInitialize();
    /* Set up the Clipboard application context and main window.  We call all
     * these functions directly instead of calling XtOpenApplication() so
     * that we can fail gracefully if we can't get an X11 display. */
    XtToolkitInitialize();
    pCtx->appContext = XtCreateApplicationContext();
    pDisplay = XtOpenDisplay(pCtx->appContext, 0, 0, "VBoxClipboard", 0, 0, &cArgc, &pcArgv);
    if (NULL == pDisplay)
    {
        LogRel(("Shared clipboard: Failed to connect to the X11 clipboard - the window system may not be running.\n"));
        rc = VERR_NOT_SUPPORTED;
    }
#ifndef TESTCASE
    if (RT_SUCCESS(rc))
    {
        rc = clipLoadXFixes(pDisplay, pCtx);
        if (RT_FAILURE(rc))
           LogRel(("Shared clipboard: Failed to load the XFIXES extension.\n"));
    }
#endif
    if (RT_SUCCESS(rc))
    {
        pCtx->widget = XtVaAppCreateShell(0, "VBoxClipboard",
                                          applicationShellWidgetClass,
                                          pDisplay, XtNwidth, 1, XtNheight,
                                          1, NULL);
        if (NULL == pCtx->widget)
        {
            LogRel(("Shared clipboard: Failed to construct the X11 window for the shared clipboard manager.\n"));
            rc = VERR_NO_MEMORY;
        }
        else
            rc = clipRegisterContext(pCtx);
    }
    if (RT_SUCCESS(rc))
    {
        XtSetMappedWhenManaged(pCtx->widget, false);
        XtRealizeWidget(pCtx->widget);
#ifndef TESTCASE
        /* Enable clipboard update notification */
        pCtx->fixesSelectInput(pDisplay, XtWindow(pCtx->widget),
                               clipGetAtom(pCtx, "CLIPBOARD"),
                               7 /* All XFixes*Selection*NotifyMask flags */);
#endif
    }
    /* Create the pipes */
    int pipes[2];
    if (!pipe(pipes))
    {
        pCtx->wakeupPipeRead = pipes[0];
        pCtx->wakeupPipeWrite = pipes[1];
        if (!XtAppAddInput(pCtx->appContext, pCtx->wakeupPipeRead,
                           (XtPointer) XtInputReadMask,
                           clipDrainWakeupPipe, (XtPointer) pCtx))
            rc = VERR_NO_MEMORY;  /* What failure means is not doc'ed. */
        if (   RT_SUCCESS(rc)
            && (fcntl(pCtx->wakeupPipeRead, F_SETFL, O_NONBLOCK) != 0))
            rc = RTErrConvertFromErrno(errno);
        if (RT_FAILURE(rc))
            LogRel(("Shared clipboard: Failed to setup the termination mechanism.\n"));
    }
    else
        rc = RTErrConvertFromErrno(errno);
    if (RT_FAILURE(rc))
        clipUninit(pCtx);
    if (RT_FAILURE(rc))
        LogRel(("Shared clipboard: Initialisation failed: %Rrc\n", rc));
    return rc;
}

/**
 * Construct the X11 backend of the shared clipboard.
 * @note  X11 backend code
 */
CLIPBACKEND *ClipConstructX11(VBOXCLIPBOARDCONTEXT *pFrontend, bool fHeadless)
{
    CLIPBACKEND *pCtx = (CLIPBACKEND *)RTMemAllocZ(sizeof(CLIPBACKEND));
    if (pCtx && fHeadless)
    {
        /*
         * If we don't find the DISPLAY environment variable we assume that
         * we are not connected to an X11 server. Don't actually try to do
         * this then, just fail silently and report success on every call.
         * This is important for VBoxHeadless.
         */
        LogRelFunc(("X11 DISPLAY variable not set -- disabling shared clipboard\n"));
        pCtx->fHaveX11 = false;
        return pCtx;
    }

    pCtx->fHaveX11 = true;

    LogRel(("Shared clipboard: Initializing X11 clipboard backend\n"));
    if (pCtx)
        pCtx->pFrontend = pFrontend;
    return pCtx;
}

/**
 * Destruct the shared clipboard X11 backend.
 * @note  X11 backend code
 */
void ClipDestructX11(CLIPBACKEND *pCtx)
{
    /*
     * Immediately return if we are not connected to the X server.
     */
    if (!pCtx->fHaveX11)
        return;

    /* We set this to NULL when the event thread exits.  It really should
     * have exited at this point, when we are about to unload the code from
     * memory. */
    Assert(pCtx->widget == NULL);
}

/**
 * Announce to the X11 backend that we are ready to start.
 * @param  grab  whether we should try to grab the shared clipboard at once
 */
int ClipStartX11(CLIPBACKEND *pCtx, bool grab)
{
    int rc = VINF_SUCCESS;
    LogRelFlowFunc(("\n"));
    /*
     * Immediately return if we are not connected to the X server.
     */
    if (!pCtx->fHaveX11)
        return VINF_SUCCESS;

    rc = clipInit(pCtx);
    if (RT_SUCCESS(rc))
    {
        clipResetX11Formats(pCtx);
        pCtx->fGrabClipboardOnStart = grab;
    }
#ifndef TESTCASE
    if (RT_SUCCESS(rc))
    {
        rc = RTThreadCreate(&pCtx->thread, clipEventThread, pCtx, 0,
                            RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "SHCLIP");
        if (RT_FAILURE(rc))
        {
            LogRel(("Shared clipboard: Failed to start the shared clipboard thread.\n"));
            clipUninit(pCtx);
        }
    }
#endif
    return rc;
}

/**
 * Shut down the shared clipboard X11 backend.
 * @note  X11 backend code
 * @note  Any requests from this object to get clipboard data from VBox
 *        *must* have completed or aborted before we are called, as
 *        otherwise the X11 event loop will still be waiting for the request
 *        to return and will not be able to terminate.
 */
int ClipStopX11(CLIPBACKEND *pCtx)
{
    int rc, rcThread;
    unsigned count = 0;
    /*
     * Immediately return if we are not connected to the X server.
     */
    if (!pCtx->fHaveX11)
        return VINF_SUCCESS;

    LogRelFunc(("stopping the shared clipboard X11 backend\n"));
    /* Write to the "stop" pipe */
    clipQueueToEventThread(pCtx, clipStopEventThreadWorker, (XtPointer) pCtx);
#ifndef TESTCASE
    do
    {
        rc = RTThreadWait(pCtx->thread, 1000, &rcThread);
        ++count;
        Assert(RT_SUCCESS(rc) || ((VERR_TIMEOUT == rc) && (count != 5)));
    } while ((VERR_TIMEOUT == rc) && (count < 300));
#else
    rc = VINF_SUCCESS;
    rcThread = VINF_SUCCESS;
    RT_NOREF_PV(count);
#endif
    if (RT_SUCCESS(rc))
        AssertRC(rcThread);
    else
        LogRelFunc(("rc=%Rrc\n", rc));
    clipUninit(pCtx);
    LogRelFlowFunc(("returning %Rrc.\n", rc));
    RT_NOREF_PV(rcThread);
    return rc;
}

/**
 * Satisfy a request from X11 for clipboard targets supported by VBox.
 *
 * @returns iprt status code
 * @param  atomTypeReturn The type of the data we are returning
 * @param  pValReturn     A pointer to the data we are returning.  This
 *                        should be set to memory allocated by XtMalloc,
 *                        which will be freed later by the Xt toolkit.
 * @param  pcLenReturn    The length of the data we are returning
 * @param  piFormatReturn The format (8bit, 16bit, 32bit) of the data we are
 *                        returning
 * @note  X11 backend code, called by the XtOwnSelection callback.
 */
static int clipCreateX11Targets(CLIPBACKEND *pCtx, Atom *atomTypeReturn,
                                XtPointer *pValReturn,
                                unsigned long *pcLenReturn,
                                int *piFormatReturn)
{
    Atom *atomTargets = (Atom *)XtMalloc(  (MAX_CLIP_X11_FORMATS + 3)
                                         * sizeof(Atom));
    unsigned cTargets = 0;
    LogRelFlowFunc (("called\n"));
    CLIPX11FORMAT format = NIL_CLIPX11FORMAT;
    do
    {
        format = clipEnumX11Formats(pCtx->vboxFormats, format);
        if (format != NIL_CLIPX11FORMAT)
        {
            atomTargets[cTargets] = clipAtomForX11Format(pCtx, format);
            ++cTargets;
        }
    } while (format != NIL_CLIPX11FORMAT);
    /* We always offer these */
    atomTargets[cTargets] = clipGetAtom(pCtx, "TARGETS");
    atomTargets[cTargets + 1] = clipGetAtom(pCtx, "MULTIPLE");
    atomTargets[cTargets + 2] = clipGetAtom(pCtx, "TIMESTAMP");
    *atomTypeReturn = XA_ATOM;
    *pValReturn = (XtPointer)atomTargets;
    *pcLenReturn = cTargets + 3;
    *piFormatReturn = 32;
    return VINF_SUCCESS;
}

/** This is a wrapper around ClipRequestDataForX11 that will cache the
 * data returned.
 */
static int clipReadVBoxClipboard(CLIPBACKEND *pCtx, uint32_t u32Format,
                                 void **ppv, uint32_t *pcb)
{
    int rc = VINF_SUCCESS;
    LogRelFlowFunc(("pCtx=%p, u32Format=%02X, ppv=%p, pcb=%p\n", pCtx,
                 u32Format, ppv, pcb));
    if (u32Format == VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
    {
        if (pCtx->pvUnicodeCache == NULL)
            rc = ClipRequestDataForX11(pCtx->pFrontend, u32Format,
                                              &pCtx->pvUnicodeCache,
                                              &pCtx->cbUnicodeCache);
        if (RT_SUCCESS(rc))
        {
            *ppv = RTMemDup(pCtx->pvUnicodeCache, pCtx->cbUnicodeCache);
            *pcb = pCtx->cbUnicodeCache;
            if (*ppv == NULL)
                rc = VERR_NO_MEMORY;
        }
    }
    else
        rc = ClipRequestDataForX11(pCtx->pFrontend, u32Format,
                                          ppv, pcb);
    LogRelFlowFunc(("returning %Rrc\n", rc));
    if (RT_SUCCESS(rc))
        LogRelFlowFunc(("*ppv=%.*ls, *pcb=%u\n", *pcb, *ppv, *pcb));
    return rc;
}

/**
 * Calculate a buffer size large enough to hold the source Windows format
 * text converted into Unix Utf8, including the null terminator
 * @returns iprt status code
 * @param  pwsz       the source text in UCS-2 with Windows EOLs
 * @param  cwc        the size in USC-2 elements of the source text, with or
 *                    without the terminator
 * @param  pcbActual  where to store the buffer size needed
 */
static int clipWinTxtBufSizeForUtf8(PRTUTF16 pwsz, size_t cwc,
                                    size_t *pcbActual)
{
    size_t cbRet = 0;
    int rc = RTUtf16CalcUtf8LenEx(pwsz, cwc, &cbRet);
    if (RT_SUCCESS(rc))
        *pcbActual = cbRet + 1;  /* null terminator */
    return rc;
}

/**
 * Convert text from Windows format (UCS-2 with CRLF line endings) to standard
 * Utf-8.
 *
 * @returns iprt status code
 *
 * @param  pwszSrc    the text to be converted
 * @param  cbSrc      the length of @a pwszSrc in bytes
 * @param  pszBuf     where to write the converted string
 * @param  cbBuf      the size of the buffer pointed to by @a pszBuf
 * @param  pcbActual  where to store the size of the converted string.
 *                    optional.
 */
static int clipWinTxtToUtf8(PRTUTF16 pwszSrc, size_t cbSrc, char *pszBuf,
                            size_t cbBuf, size_t *pcbActual)
{
    PRTUTF16 pwszTmp = NULL;
    size_t cwSrc = cbSrc / 2, cwTmp = 0, cbDest = 0;
    int rc = VINF_SUCCESS;

    LogRelFlowFunc (("pwszSrc=%.*ls, cbSrc=%u\n", cbSrc, pwszSrc, cbSrc));
    /* How long will the converted text be? */
    AssertPtr(pwszSrc);
    AssertPtr(pszBuf);
    rc = vboxClipboardUtf16GetLinSize(pwszSrc, cwSrc, &cwTmp);
    if (RT_SUCCESS(rc) && cwTmp == 0)
        rc = VERR_NO_DATA;
    if (RT_SUCCESS(rc))
        pwszTmp = (PRTUTF16)RTMemAlloc(cwTmp * 2);
    if (!pwszTmp)
        rc = VERR_NO_MEMORY;
    /* Convert the text. */
    if (RT_SUCCESS(rc))
        rc = vboxClipboardUtf16WinToLin(pwszSrc, cwSrc, pwszTmp, cwTmp);
    if (RT_SUCCESS(rc))
        /* Convert the Utf16 string to Utf8. */
        rc = RTUtf16ToUtf8Ex(pwszTmp + 1, cwTmp - 1, &pszBuf, cbBuf,
                             &cbDest);
    RTMemFree(reinterpret_cast<void *>(pwszTmp));
    if (pcbActual)
        *pcbActual = cbDest + 1;
    LogRelFlowFunc(("returning %Rrc\n", rc));
    if (RT_SUCCESS(rc))
        LogRelFlowFunc (("converted string is %.*s. Returning.\n", cbDest,
                      pszBuf));
    return rc;
}

/**
 * Satisfy a request from X11 to convert the clipboard text to Utf-8.  We
 * return null-terminated text, but can cope with non-null-terminated input.
 *
 * @returns iprt status code
 * @param  pDisplay        an X11 display structure, needed for conversions
 *                         performed by Xlib
 * @param  pv              the text to be converted (UCS-2 with Windows EOLs)
 * @param  cb              the length of the text in @cb in bytes
 * @param  atomTypeReturn  where to store the atom for the type of the data
 *                         we are returning
 * @param  pValReturn      where to store the pointer to the data we are
 *                         returning.  This should be to memory allocated by
 *                         XtMalloc, which will be freed by the Xt toolkit
 *                         later.
 * @param  pcLenReturn     where to store the length of the data we are
 *                         returning
 * @param  piFormatReturn  where to store the bit width (8, 16, 32) of the
 *                         data we are returning
 */
static int clipWinTxtToUtf8ForX11CB(Display *pDisplay, PRTUTF16 pwszSrc,
                                    size_t cbSrc, Atom *atomTarget,
                                    Atom *atomTypeReturn,
                                    XtPointer *pValReturn,
                                    unsigned long *pcLenReturn,
                                    int *piFormatReturn)
{
    RT_NOREF2(pDisplay, pcLenReturn);

    /* This may slightly overestimate the space needed. */
    size_t cbDest = 0;
    int rc = clipWinTxtBufSizeForUtf8(pwszSrc, cbSrc / 2, &cbDest);
    if (RT_SUCCESS(rc))
    {
        char *pszDest = (char *)XtMalloc(cbDest);
        size_t cbActual = 0;
        if (pszDest)
            rc = clipWinTxtToUtf8(pwszSrc, cbSrc, pszDest, cbDest,
                                  &cbActual);
        if (RT_SUCCESS(rc))
        {
            *atomTypeReturn = *atomTarget;
            *pValReturn = (XtPointer)pszDest;
            *pcLenReturn = cbActual;
            *piFormatReturn = 8;
        }
    }
    return rc;
}

/**
 * Satisfy a request from X11 to convert the clipboard HTML fragment to Utf-8.  We
 * return null-terminated text, but can cope with non-null-terminated input.
 *
 * @returns iprt status code
 * @param  pDisplay        an X11 display structure, needed for conversions
 *                         performed by Xlib
 * @param  pv              the text to be converted (UTF8 with Windows EOLs)
 * @param  cb              the length of the text in @cb in bytes
 * @param  atomTypeReturn  where to store the atom for the type of the data
 *                         we are returning
 * @param  pValReturn      where to store the pointer to the data we are
 *                         returning.  This should be to memory allocated by
 *                         XtMalloc, which will be freed by the Xt toolkit
 *                         later.
 * @param  pcLenReturn     where to store the length of the data we are
 *                         returning
 * @param  piFormatReturn  where to store the bit width (8, 16, 32) of the
 *                         data we are returning
 */
static int clipWinHTMLToUtf8ForX11CB(Display *pDisplay, const char *pszSrc,
                                    size_t cbSrc, Atom *atomTarget,
                                    Atom *atomTypeReturn,
                                    XtPointer *pValReturn,
                                    unsigned long *pcLenReturn,
                                    int *piFormatReturn)
{
    RT_NOREF2(pDisplay, pValReturn);

    /* This may slightly overestimate the space needed. */
    LogRelFlowFunc(("source: %s", pszSrc));

    char *pszDest = (char *)XtMalloc(cbSrc);
    if(pszDest == NULL)
        return VERR_NO_MEMORY;

    memcpy(pszDest, pszSrc, cbSrc);

    *atomTypeReturn = *atomTarget;
    *pValReturn = (XtPointer)pszDest;
    *pcLenReturn = cbSrc;
    *piFormatReturn = 8;

    return VINF_SUCCESS;
}


/**
 * Does this atom correspond to one of the two selection types we support?
 * @param  widget   a valid Xt widget
 * @param  selType  the atom in question
 */
static bool clipIsSupportedSelectionType(CLIPBACKEND *pCtx, Atom selType)
{
    return(   (selType == clipGetAtom(pCtx, "CLIPBOARD"))
           || (selType == clipGetAtom(pCtx, "PRIMARY")));
}

/**
 * Remove a trailing nul character from a string by adjusting the string
 * length.  Some X11 applications don't like zero-terminated text...
 * @param  pText   the text in question
 * @param  pcText  the length of the text, adjusted on return
 * @param  format  the format of the text
 */
static void clipTrimTrailingNul(XtPointer pText, unsigned long *pcText,
                                CLIPFORMAT format)
{
    AssertPtrReturnVoid(pText);
    AssertPtrReturnVoid(pcText);
    AssertReturnVoid((format == UTF8) || (format == TEXT) || (format == HTML));
    if (((char *)pText)[*pcText - 1] == '\0')
       --(*pcText);
}

static int clipConvertVBoxCBForX11(CLIPBACKEND *pCtx, Atom *atomTarget,
                                   Atom *atomTypeReturn,
                                   XtPointer *pValReturn,
                                   unsigned long *pcLenReturn,
                                   int *piFormatReturn)
{
    int rc = VINF_SUCCESS;
    CLIPX11FORMAT x11Format = clipFindX11FormatByAtom(pCtx, *atomTarget);
    CLIPFORMAT format = clipRealFormatForX11Format(x11Format);
    if (   ((format == UTF8) || (format == TEXT))
        && (pCtx->vboxFormats & VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT))
    {
        void *pv = NULL;
        uint32_t cb = 0;
        rc = clipReadVBoxClipboard(pCtx,
                                   VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT,
                                   &pv, &cb);
        if (RT_SUCCESS(rc) && (cb == 0))
            rc = VERR_NO_DATA;
        if (RT_SUCCESS(rc) && ((format == UTF8) || (format == TEXT)))
            rc = clipWinTxtToUtf8ForX11CB(XtDisplay(pCtx->widget),
                                          (PRTUTF16)pv, cb, atomTarget,
                                          atomTypeReturn, pValReturn,
                                          pcLenReturn, piFormatReturn);
        if (RT_SUCCESS(rc))
            clipTrimTrailingNul(*(XtPointer *)pValReturn, pcLenReturn, format);
        RTMemFree(pv);
    }
    else if (   (format == BMP)
             && (pCtx->vboxFormats & VBOX_SHARED_CLIPBOARD_FMT_BITMAP))
    {
        void *pv = NULL;
        uint32_t cb = 0;
        rc = clipReadVBoxClipboard(pCtx,
                                   VBOX_SHARED_CLIPBOARD_FMT_BITMAP,
                                   &pv, &cb);
        if (RT_SUCCESS(rc) && (cb == 0))
            rc = VERR_NO_DATA;
        if (RT_SUCCESS(rc) && (format == BMP))
        {
            /* Create a full BMP from it */
            rc = vboxClipboardDibToBmp(pv, cb, (void **)pValReturn,
                                       (size_t *)pcLenReturn);
        }
        else
            rc = VERR_NOT_SUPPORTED;

        if (RT_SUCCESS(rc))
        {
            *atomTypeReturn = *atomTarget;
            *piFormatReturn = 8;
        }
        RTMemFree(pv);
    }
    else if ( (format == HTML)
            && (pCtx->vboxFormats & VBOX_SHARED_CLIPBOARD_FMT_HTML))
    {
        void *pv = NULL;
        uint32_t cb = 0;
        rc = clipReadVBoxClipboard(pCtx,
                                   VBOX_SHARED_CLIPBOARD_FMT_HTML,
                                   &pv, &cb);
        if (RT_SUCCESS(rc) && (cb == 0))
            rc = VERR_NO_DATA;
        if (RT_SUCCESS(rc))
        {
            /*
            * The common VBox HTML encoding will be - Utf8
            * becuase it more general for HTML formats then UTF16
            * X11 clipboard returns UTF16, so before sending it we should
            * convert it to UTF8
            * It's very strange but here we get utf16 from x11 clipboard
            * in same time we send utf8 to x11 clipboard and it's work
            */
            rc = clipWinHTMLToUtf8ForX11CB(XtDisplay(pCtx->widget),
                (const char*)pv, cb, atomTarget,
                atomTypeReturn, pValReturn,
                pcLenReturn, piFormatReturn);


            if (RT_SUCCESS(rc))
                clipTrimTrailingNul(*(XtPointer *)pValReturn, pcLenReturn, format);
            RTMemFree(pv);
        }
    }
    else
        rc = VERR_NOT_SUPPORTED;
    return rc;
}

/**
 * Return VBox's clipboard data for an X11 client.
 * @note  X11 backend code, callback for XtOwnSelection
 */
static Boolean clipXtConvertSelectionProc(Widget widget, Atom *atomSelection,
                                          Atom *atomTarget,
                                          Atom *atomTypeReturn,
                                          XtPointer *pValReturn,
                                          unsigned long *pcLenReturn,
                                          int *piFormatReturn)
{
    CLIPBACKEND *pCtx = clipLookupContext(widget);
    int rc = VINF_SUCCESS;

    LogRelFlowFunc(("\n"));
    if (!pCtx)
        return false;
    if (!clipIsSupportedSelectionType(pCtx, *atomSelection))
        return false;
    if (*atomTarget == clipGetAtom(pCtx, "TARGETS"))
        rc = clipCreateX11Targets(pCtx, atomTypeReturn, pValReturn,
                                  pcLenReturn, piFormatReturn);
    else
        rc = clipConvertVBoxCBForX11(pCtx, atomTarget, atomTypeReturn,
                                     pValReturn, pcLenReturn, piFormatReturn);
    LogRelFlowFunc(("returning, internal status code %Rrc\n", rc));
    return RT_SUCCESS(rc);
}

/** Structure used to pass information about formats that VBox supports */
typedef struct _CLIPNEWVBOXFORMATS
{
    /** Context information for the X11 clipboard */
    CLIPBACKEND *pCtx;
    /** Formats supported by VBox */
    uint32_t formats;
} CLIPNEWVBOXFORMATS;

/** Invalidates the local cache of the data in the VBox clipboard. */
static void clipInvalidateVBoxCBCache(CLIPBACKEND *pCtx)
{
    if (pCtx->pvUnicodeCache != NULL)
    {
        RTMemFree(pCtx->pvUnicodeCache);
        pCtx->pvUnicodeCache = NULL;
    }
}

/**
 * Take possession of the X11 clipboard (and middle-button selection).
 */
static void clipGrabX11CB(CLIPBACKEND *pCtx, uint32_t u32Formats)
{
    if (XtOwnSelection(pCtx->widget, clipGetAtom(pCtx, "CLIPBOARD"),
                       CurrentTime, clipXtConvertSelectionProc, NULL, 0))
    {
        pCtx->vboxFormats = u32Formats;
        /* Grab the middle-button paste selection too. */
        XtOwnSelection(pCtx->widget, clipGetAtom(pCtx, "PRIMARY"),
                       CurrentTime, clipXtConvertSelectionProc, NULL, 0);
#ifndef TESTCASE
        /* Xt suppresses these if we already own the clipboard, so send them
         * ourselves. */
        XSetSelectionOwner(XtDisplay(pCtx->widget),
                           clipGetAtom(pCtx, "CLIPBOARD"),
                           XtWindow(pCtx->widget), CurrentTime);
        XSetSelectionOwner(XtDisplay(pCtx->widget),
                           clipGetAtom(pCtx, "PRIMARY"),
                           XtWindow(pCtx->widget), CurrentTime);
#endif
    }
}

/**
 * Worker function for ClipAnnounceFormatToX11 which runs on the
 * event thread.
 * @param pUserData  Pointer to a CLIPNEWVBOXFORMATS structure containing
 *                   information about the VBox formats available and the
 *                   clipboard context data.  Must be freed by the worker.
 */
static void clipNewVBoxFormatsWorker(void *pUserData,
                                     void * /* interval */)
{
    CLIPNEWVBOXFORMATS *pFormats = (CLIPNEWVBOXFORMATS *)pUserData;
    CLIPBACKEND *pCtx = pFormats->pCtx;
    uint32_t u32Formats = pFormats->formats;
    RTMemFree(pFormats);
    LogRelFlowFunc (("u32Formats=%d\n", u32Formats));
    clipInvalidateVBoxCBCache(pCtx);
    clipGrabX11CB(pCtx, u32Formats);
    clipResetX11Formats(pCtx);
    LogRelFlowFunc(("returning\n"));
}

/**
 * VBox is taking possession of the shared clipboard.
 *
 * @param u32Formats Clipboard formats that VBox is offering
 * @note  X11 backend code
 */
void ClipAnnounceFormatToX11(CLIPBACKEND *pCtx,
                                        uint32_t u32Formats)
{
    /*
     * Immediately return if we are not connected to the X server.
     */
    if (!pCtx->fHaveX11)
        return;
    /* This must be freed by the worker callback */
    CLIPNEWVBOXFORMATS *pFormats =
        (CLIPNEWVBOXFORMATS *) RTMemAlloc(sizeof(CLIPNEWVBOXFORMATS));
    if (pFormats != NULL)  /* if it is we will soon have other problems */
    {
        pFormats->pCtx = pCtx;
        pFormats->formats = u32Formats;
        clipQueueToEventThread(pCtx, clipNewVBoxFormatsWorker,
                               (XtPointer) pFormats);
    }
}

/**
 * Massage generic Utf16 with CR end-of-lines into the format Windows expects
 * and return the result in a RTMemAlloc allocated buffer.
 * @returns  IPRT status code
 * @param  pwcSrc     The source Utf16
 * @param  cwcSrc     The number of 16bit elements in @a pwcSrc, not counting
 *                    the terminating zero
 * @param  ppwszDest  Where to store the buffer address
 * @param  pcbDest    On success, where to store the number of bytes written.
 *                    Undefined otherwise.  Optional
 */
static int clipUtf16ToWinTxt(RTUTF16 *pwcSrc, size_t cwcSrc,
                             PRTUTF16 *ppwszDest, uint32_t *pcbDest)
{
    LogRelFlowFunc(("pwcSrc=%p, cwcSrc=%u, ppwszDest=%p\n", pwcSrc, cwcSrc,
                 ppwszDest));
    AssertPtrReturn(pwcSrc, VERR_INVALID_POINTER);
    AssertPtrReturn(ppwszDest, VERR_INVALID_POINTER);
    if (pcbDest)
        *pcbDest = 0;
    PRTUTF16 pwszDest = NULL;
    size_t cwcDest;
    int rc = vboxClipboardUtf16GetWinSize(pwcSrc, cwcSrc + 1, &cwcDest);
    if (RT_SUCCESS(rc))
    {
        pwszDest = (PRTUTF16) RTMemAlloc(cwcDest * 2);
        if (!pwszDest)
            rc = VERR_NO_MEMORY;
    }
    if (RT_SUCCESS(rc))
        rc = vboxClipboardUtf16LinToWin(pwcSrc, cwcSrc + 1, pwszDest,
                                        cwcDest);
    if (RT_SUCCESS(rc))
    {
        LogRelFlowFunc (("converted string is %.*ls\n", cwcDest, pwszDest));
        *ppwszDest = pwszDest;
        if (pcbDest)
            *pcbDest = cwcDest * 2;
    }
    else
        RTMemFree(pwszDest);
    LogRelFlowFunc(("returning %Rrc\n", rc));
    if (pcbDest)
        LogRelFlowFunc(("*pcbDest=%u\n", *pcbDest));
    return rc;
}

/**
 * Convert Utf-8 text with CR end-of-lines into Utf-16 as Windows expects it
 * and return the result in a RTMemAlloc allocated buffer.
 * @returns  IPRT status code
 * @param  pcSrc      The source Utf-8
 * @param  cbSrc      The size of the source in bytes, not counting the
 *                    terminating zero
 * @param  ppwszDest  Where to store the buffer address
 * @param  pcbDest    On success, where to store the number of bytes written.
 *                    Undefined otherwise.  Optional
 */
static int clipUtf8ToWinTxt(const char *pcSrc, unsigned cbSrc,
                            PRTUTF16 *ppwszDest, uint32_t *pcbDest)
{
    LogRelFlowFunc(("pcSrc=%p, cbSrc=%u, ppwszDest=%p\n", pcSrc, cbSrc,
                 ppwszDest));
    AssertPtrReturn(pcSrc, VERR_INVALID_POINTER);
    AssertPtrReturn(ppwszDest, VERR_INVALID_POINTER);
    if (pcbDest)
        *pcbDest = 0;
    /* Intermediate conversion to UTF16 */
    size_t cwcTmp;
    PRTUTF16 pwcTmp = NULL;
    int rc = RTStrToUtf16Ex(pcSrc, cbSrc, &pwcTmp, 0, &cwcTmp);
    if (RT_SUCCESS(rc))
        rc = clipUtf16ToWinTxt(pwcTmp, cwcTmp, ppwszDest, pcbDest);
    RTUtf16Free(pwcTmp);
    LogRelFlowFunc(("Returning %Rrc\n", rc));
    if (pcbDest)
        LogRelFlowFunc(("*pcbDest=%u\n", *pcbDest));
    return rc;
}

/**
 * Convert Latin-1 text with CR end-of-lines into Utf-16 as Windows expects
 * it and return the result in a RTMemAlloc allocated buffer.
 * @returns  IPRT status code
 * @param  pcSrc      The source text
 * @param  cbSrc      The size of the source in bytes, not counting the
 *                    terminating zero
 * @param  ppwszDest  Where to store the buffer address
 * @param  pcbDest    On success, where to store the number of bytes written.
 *                    Undefined otherwise.  Optional
 */
static int clipLatin1ToWinTxt(char *pcSrc, unsigned cbSrc,
                              PRTUTF16 *ppwszDest, uint32_t *pcbDest)
{
    LogRelFlowFunc (("pcSrc=%.*s, cbSrc=%u, ppwszDest=%p\n", cbSrc,
                  (char *) pcSrc, cbSrc, ppwszDest));
    AssertPtrReturn(pcSrc, VERR_INVALID_POINTER);
    AssertPtrReturn(ppwszDest, VERR_INVALID_POINTER);
    PRTUTF16 pwszDest = NULL;
    int rc = VINF_SUCCESS;

    /* Calculate the space needed */
    unsigned cwcDest = 0;
    for (unsigned i = 0; i < cbSrc && pcSrc[i] != '\0'; ++i)
        if (pcSrc[i] == LINEFEED)
            cwcDest += 2;
        else
            ++cwcDest;
    ++cwcDest;  /* Leave space for the terminator */
    if (pcbDest)
        *pcbDest = cwcDest * 2;
    pwszDest = (PRTUTF16) RTMemAlloc(cwcDest * 2);
    if (!pwszDest)
        rc = VERR_NO_MEMORY;

    /* And do the conversion, bearing in mind that Latin-1 expands "naturally"
     * to Utf-16. */
    if (RT_SUCCESS(rc))
    {
        for (unsigned i = 0, j = 0; i < cbSrc; ++i, ++j)
            if (pcSrc[i] != LINEFEED)
                pwszDest[j] = pcSrc[i];
            else
            {
                pwszDest[j] = CARRIAGERETURN;
                pwszDest[j + 1] = LINEFEED;
                ++j;
            }
        pwszDest[cwcDest - 1] = '\0';  /* Make sure we are zero-terminated. */
        LogRelFlowFunc (("converted text is %.*ls\n", cwcDest, pwszDest));
    }
    if (RT_SUCCESS(rc))
        *ppwszDest = pwszDest;
    else
        RTMemFree(pwszDest);
    LogRelFlowFunc(("Returning %Rrc\n", rc));
    if (pcbDest)
        LogRelFlowFunc(("*pcbDest=%u\n", *pcbDest));
    return rc;
}


/**
* Convert Utf16 text into UTF8 as Windows expects
* it and return the result in a RTMemAlloc allocated buffer.
* @returns  IPRT status code
* @param  pcSrc      The source text
* @param  cbSrc      The size of the source in bytes, not counting the
*                    terminating zero
* @param  ppwszDest  Where to store the buffer address
* @param  pcbDest    On success, where to store the number of bytes written.
*                    Undefined otherwise.  Optional
*/
int  clipUTF16ToWinHTML(RTUTF16 *pwcBuf, size_t cb, char **ppszOut, uint32_t *pcOut)
{
    Assert(pwcBuf);
    Assert(cb);
    Assert(ppszOut);
    Assert(pcOut);

    if (cb % 2)
        return VERR_INVALID_PARAMETER;
    size_t cwc = cb / 2;
    size_t i = 0;
    RTUTF16 *pwc = pwcBuf;
    char *pchRes = NULL;
    size_t cRes = 0;
    LogRelFlowFunc(("clipUTF16ToWinHTML src= %ls cb=%d i=%i, %x %x\n", pwcBuf, cb, i, ppszOut, pcOut));
    while (i < cwc)
    {
        /* find  zero symbol (end of string) */
        for (; i < cwc && pwcBuf[i] != 0; i++)
            ;
        LogRelFlowFunc(("skipped nulls i=%d cwc=%d\n", i, cwc));

        /* convert found string */
        char *psz = NULL;
        size_t cch = 0;
        int rc = RTUtf16ToUtf8Ex(pwc, cwc, &psz, pwc - pwcBuf, &cch);
        LogRelFlowFunc(("utf16toutf8 src= %ls res=%s i=%i\n", pwc, psz, i));
        if (RT_FAILURE(rc))
        {
            RTMemFree(pchRes);
            return rc;
        }

        /* append new substring */
        char *pchNew = (char*)RTMemRealloc(pchRes, cRes + cch + 1);
        if (!pchNew)
        {
            RTMemFree(pchRes);
            RTStrFree(psz);
            return VERR_NO_MEMORY;
        }
        pchRes = pchNew;
        memcpy(pchRes + cRes, psz, cch + 1);
        LogRelFlowFunc(("Temp result res=%s\n", pchRes + cRes));

        /* remove temporary buffer */
        RTStrFree(psz);
        cRes += cch + 1;
        /* skip zero symbols */
        for (; i < cwc && pwcBuf[i] == 0; i++)
            ;
        /* remember start of string */
        pwc += i;
    }
    *ppszOut = pchRes;
    *pcOut = cRes;

    return VINF_SUCCESS;
}



/** A structure containing information about where to store a request
 * for the X11 clipboard contents. */
struct _CLIPREADX11CBREQ
{
    /** The format VBox would like the data in */
    uint32_t mFormat;
    /** The text format we requested from X11 if we requested text */
    CLIPX11FORMAT mTextFormat;
    /** The bitmap format we requested from X11 if we requested bitmap */
    CLIPX11FORMAT mBitmapFormat;
    /** The HTML format we requested from X11 if we requested HTML */
    CLIPX11FORMAT mHtmlFormat;
    /** The clipboard context this request is associated with */
    CLIPBACKEND *mCtx;
    /** The request structure passed in from the backend. */
    CLIPREADCBREQ *mReq;
};

typedef struct _CLIPREADX11CBREQ CLIPREADX11CBREQ;

/**
 * Convert the data obtained from the X11 clipboard to the required format,
 * place it in the buffer supplied and signal that data has arrived.
 * Convert the text obtained UTF-16LE with Windows EOLs.
 * Convert full BMP data to DIB format.
 * @note  X11 backend code, callback for XtGetSelectionValue, for use when
 *        the X11 clipboard contains a format we understand.
 */
static void clipConvertX11CB(void *pClientData, void *pvSrc, unsigned cbSrc)
{
    CLIPREADX11CBREQ *pReq = (CLIPREADX11CBREQ *) pClientData;
    LogRelFlowFunc(("pReq->mFormat=%02X, pReq->mTextFormat=%u, "
                "pReq->mBitmapFormat=%u, pReq->mHtmlFormat=%u, pReq->mCtx=%p\n",
                 pReq->mFormat, pReq->mTextFormat, pReq->mBitmapFormat,
                 pReq->mHtmlFormat, pReq->mCtx));
    AssertPtr(pReq->mCtx);
    Assert(pReq->mFormat != 0);  /* sanity */
    int rc = VINF_SUCCESS;
    CLIPBACKEND *pCtx = pReq->mCtx;
    void *pvDest = NULL;
    uint32_t cbDest = 0;

    pCtx->fBusy = false;
    if (pCtx->fUpdateNeeded)
        clipQueryX11CBFormats(pCtx);
    if (pvSrc == NULL)
        /* The clipboard selection may have changed before we could get it. */
        rc = VERR_NO_DATA;
    else if (pReq->mFormat == VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
    {
        /* In which format is the clipboard data? */
        switch (clipRealFormatForX11Format(pReq->mTextFormat))
        {
            case UTF8:
            case TEXT:
            {
                /* If we are given broken Utf-8, we treat it as Latin1.  Is
                 * this acceptable? */
                if (RT_SUCCESS(RTStrValidateEncodingEx((char *)pvSrc, cbSrc,
                                                       0)))
                    rc = clipUtf8ToWinTxt((const char *)pvSrc, cbSrc,
                                          (PRTUTF16 *) &pvDest, &cbDest);
                else
                    rc = clipLatin1ToWinTxt((char *) pvSrc, cbSrc,
                                            (PRTUTF16 *) &pvDest, &cbDest);
                break;
            }
            default:
                rc = VERR_INVALID_PARAMETER;
        }
    }
    else if (pReq->mFormat == VBOX_SHARED_CLIPBOARD_FMT_BITMAP)
    {
        /* In which format is the clipboard data? */
        switch (clipRealFormatForX11Format(pReq->mBitmapFormat))
        {
            case BMP:
            {
                const void *pDib;
                size_t cbDibSize;
                rc = vboxClipboardBmpGetDib((const void *)pvSrc, cbSrc,
                                            &pDib, &cbDibSize);
                if (RT_SUCCESS(rc))
                {
                    pvDest = RTMemAlloc(cbDibSize);
                    if (!pvDest)
                        rc = VERR_NO_MEMORY;
                    else
                    {
                        memcpy(pvDest, pDib, cbDibSize);
                        cbDest = cbDibSize;
                    }
                }
                break;
            }
            default:
                rc = VERR_INVALID_PARAMETER;
        }
    }
    else if(pReq->mFormat == VBOX_SHARED_CLIPBOARD_FMT_HTML)
    {
        /* In which format is the clipboard data? */
        switch (clipRealFormatForX11Format(pReq->mHtmlFormat))
        {
            case HTML:
            {
                /* The common VBox HTML encoding will be - Utf8
                * becuase it more general for HTML formats then UTF16
                * X11 clipboard returns UTF16, so before sending it we should
                * convert it to UTF8
                */
                pvDest = NULL;
                cbDest = 0;
                /* Some applications sends data in utf16, some in itf8,
                 * without indication it in MIME.
                 * But in case of utf16, at least an OpenOffice adds Byte Order Mark - 0xfeff
                 * at start of clipboard data
                 */
                if( cbSrc >= sizeof(RTUTF16) && *(PRTUTF16)pvSrc == 0xfeff )
                {
                    LogRelFlowFunc((" \n"));
                    rc = clipUTF16ToWinHTML((RTUTF16*)pvSrc, cbSrc,
                        (char**)&pvDest, &cbDest);
                }
                else
                {
                   pvDest = RTMemAlloc(cbSrc);
                   if(pvDest)
                   {
                        memcpy(pvDest, pvSrc, cbSrc);
                        cbDest = cbSrc;
                   }
                   else
                   {
                        rc = VERR_NO_MEMORY;
                        break;
                   }
                }

                LogRelFlowFunc(("Source unicode %ls, cbSrc = %d\n, Byte Order Mark = %hx",
                                pvSrc, cbSrc, ((PRTUTF16)pvSrc)[0]));
                LogRelFlowFunc(("converted to win unicode %s, cbDest = %d, rc = %Rrc\n", pvDest, cbDest, rc));
                rc = VINF_SUCCESS;
                break;
            }
            default:
            {
                rc = VERR_INVALID_PARAMETER;
            }
        }
    }
    else
        rc = VERR_NOT_IMPLEMENTED;
    ClipCompleteDataRequestFromX11(pReq->mCtx->pFrontend, rc, pReq->mReq,
                                   pvDest, cbDest);
    RTMemFree(pvDest);
    RTMemFree(pReq);
    LogRelFlowFunc(("rc=%Rrc\n", rc));
}

#ifndef TESTCASE
/**
 * Convert the data obtained from the X11 clipboard to the required format,
 * place it in the buffer supplied and signal that data has arrived.
 * Convert the text obtained UTF-16LE with Windows EOLs.
 * Convert full BMP data to DIB format.
 * @note  X11 backend code, callback for XtGetSelectionValue, for use when
 *        the X11 clipboard contains a format we understand.
 */
static void cbConvertX11CB(Widget widget, XtPointer pClientData,
                           Atom * /* selection */, Atom *atomType,
                           XtPointer pvSrc, long unsigned int *pcLen,
                           int *piFormat)
{
    RT_NOREF1(widget);
    if (*atomType == XT_CONVERT_FAIL) /* Xt timeout */
        clipConvertX11CB(pClientData, NULL, 0);
    else
        clipConvertX11CB(pClientData, pvSrc, (*pcLen) * (*piFormat) / 8);

    XtFree((char *)pvSrc);
}
#endif

#ifdef TESTCASE
static void testRequestData(CLIPBACKEND* pCtx, CLIPX11FORMAT target,
                            void *closure);
#endif

static void getSelectionValue(CLIPBACKEND *pCtx, CLIPX11FORMAT format,
                              CLIPREADX11CBREQ *pReq)
{
#ifndef TESTCASE
    XtGetSelectionValue(pCtx->widget, clipGetAtom(pCtx, "CLIPBOARD"),
                        clipAtomForX11Format(pCtx, format),
                        cbConvertX11CB,
                        reinterpret_cast<XtPointer>(pReq),
                        CurrentTime);
#else
    testRequestData(pCtx, format, (void *)pReq);
#endif
}

/** Worker function for ClipRequestDataFromX11 which runs on the event
 * thread. */
static void vboxClipboardReadX11Worker(void *pUserData,
                                       void * /* interval */)
{
    CLIPREADX11CBREQ *pReq = (CLIPREADX11CBREQ *)pUserData;
    CLIPBACKEND *pCtx = pReq->mCtx;
    LogRelFlowFunc (("pReq->mFormat = %02X\n", pReq->mFormat));

    int rc = VINF_SUCCESS;
    bool fBusy = pCtx->fBusy;
    pCtx->fBusy = true;
    if (fBusy)
        /* If the clipboard is busy just fend off the request. */
        rc = VERR_TRY_AGAIN;
    else if (pReq->mFormat == VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
    {
        /*
         * VBox wants to read data in the given format.
         */
        pReq->mTextFormat = pCtx->X11TextFormat;
        if (pReq->mTextFormat == INVALID)
            /* VBox thinks we have data and we don't */
            rc = VERR_NO_DATA;
        else
            /* Send out a request for the data to the current clipboard
             * owner */
            getSelectionValue(pCtx, pCtx->X11TextFormat, pReq);
    }
    else if (pReq->mFormat == VBOX_SHARED_CLIPBOARD_FMT_BITMAP)
    {
        pReq->mBitmapFormat = pCtx->X11BitmapFormat;
        if (pReq->mBitmapFormat == INVALID)
            /* VBox thinks we have data and we don't */
            rc = VERR_NO_DATA;
        else
            /* Send out a request for the data to the current clipboard
             * owner */
            getSelectionValue(pCtx, pCtx->X11BitmapFormat, pReq);
    }
    else if(pReq->mFormat == VBOX_SHARED_CLIPBOARD_FMT_HTML)
    {
        /* Send out a request for the data to the current clipboard
             * owner */
        pReq->mHtmlFormat = pCtx->X11HTMLFormat;
        if(pReq->mHtmlFormat == INVALID)
                    /* VBox thinks we have data and we don't */
            rc = VERR_NO_DATA;
        else
            /* Send out a request for the data to the current clipboard
             * owner */
            getSelectionValue(pCtx, pCtx->X11HTMLFormat, pReq);
    }
    else
        rc = VERR_NOT_IMPLEMENTED;
    if (RT_FAILURE(rc))
    {
        /* The clipboard callback was never scheduled, so we must signal
         * that the request processing is finished and clean up ourselves. */
        ClipCompleteDataRequestFromX11(pReq->mCtx->pFrontend, rc, pReq->mReq,
                                       NULL, 0);
        RTMemFree(pReq);
    }
    LogRelFlowFunc(("status %Rrc\n", rc));
}

/**
 * Called when VBox wants to read the X11 clipboard.
 *
 * @returns iprt status code
 * @param  pCtx      Context data for the clipboard backend
 * @param  u32Format The format that the VBox would like to receive the data
 *                   in
 * @param  pv        Where to write the data to
 * @param  cb        The size of the buffer to write the data to
 * @param  pcbActual Where to write the actual size of the written data
 * @note   We allocate a request structure which must be freed by the worker
 */
int ClipRequestDataFromX11(CLIPBACKEND *pCtx, uint32_t u32Format,
                           CLIPREADCBREQ *pReq)
{
    /*
     * Immediately return if we are not connected to the X server.
     */
    if (!pCtx->fHaveX11)
        return VERR_NO_DATA;
    int rc = VINF_SUCCESS;
    CLIPREADX11CBREQ *pX11Req;
    pX11Req = (CLIPREADX11CBREQ *)RTMemAllocZ(sizeof(*pX11Req));
    if (!pX11Req)
        rc = VERR_NO_MEMORY;
    else
    {
        pX11Req->mFormat = u32Format;
        pX11Req->mCtx = pCtx;
        pX11Req->mReq = pReq;
        /* We use this to schedule a worker function on the event thread. */
        clipQueueToEventThread(pCtx, vboxClipboardReadX11Worker,
                               (XtPointer) pX11Req);
    }
    return rc;
}

#ifdef TESTCASE

/** @todo This unit test currently works by emulating the X11 and X toolkit
 * APIs to exercise the code, since I didn't want to rewrite the code too much
 * when I wrote the tests.  However, this makes it rather ugly and hard to
 * understand.  Anyone doing any work on the code should feel free to
 * rewrite the tests and the code to make them cleaner and more readable. */

#include <iprt/test.h>
#include <poll.h>

#define TEST_WIDGET (Widget)0xffff

/* For the purpose of the test case, we just execute the procedure to be
 * scheduled, as we are running single threaded. */
void testQueueToEventThread(void (*proc)(void *, void *),
                            void *client_data)
{
    proc(client_data, NULL);
}

void XtFree(char *ptr)
{ RTMemFree((void *) ptr); }

/* The data in the simulated VBox clipboard */
static int g_vboxDataRC = VINF_SUCCESS;
static void *g_vboxDatapv = NULL;
static uint32_t g_vboxDatacb = 0;

/* Set empty data in the simulated VBox clipboard. */
static void clipEmptyVBox(CLIPBACKEND *pCtx, int retval)
{
    g_vboxDataRC = retval;
    RTMemFree(g_vboxDatapv);
    g_vboxDatapv = NULL;
    g_vboxDatacb = 0;
    ClipAnnounceFormatToX11(pCtx, 0);
}

/* Set the data in the simulated VBox clipboard. */
static int clipSetVBoxUtf16(CLIPBACKEND *pCtx, int retval,
                            const char *pcszData, size_t cb)
{
    PRTUTF16 pwszData = NULL;
    size_t cwData = 0;
    int rc = RTStrToUtf16Ex(pcszData, RTSTR_MAX, &pwszData, 0, &cwData);
    if (RT_FAILURE(rc))
        return rc;
    AssertReturn(cb <= cwData * 2 + 2, VERR_BUFFER_OVERFLOW);
    void *pv = RTMemDup(pwszData, cb);
    RTUtf16Free(pwszData);
    if (pv == NULL)
        return VERR_NO_MEMORY;
    if (g_vboxDatapv)
        RTMemFree(g_vboxDatapv);
    g_vboxDataRC = retval;
    g_vboxDatapv = pv;
    g_vboxDatacb = cb;
    ClipAnnounceFormatToX11(pCtx,
                                       VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT);
    return VINF_SUCCESS;
}

/* Return the data in the simulated VBox clipboard. */
int ClipRequestDataForX11(VBOXCLIPBOARDCONTEXT *pCtx, uint32_t u32Format, void **ppv, uint32_t *pcb)
{
    RT_NOREF2(pCtx, u32Format);
    *pcb = g_vboxDatacb;
    if (g_vboxDatapv != NULL)
    {
        void *pv = RTMemDup(g_vboxDatapv, g_vboxDatacb);
        *ppv = pv;
        return pv != NULL ? g_vboxDataRC : VERR_NO_MEMORY;
    }
    *ppv = NULL;
    return g_vboxDataRC;
}

Display *XtDisplay(Widget w)
{ NOREF(w); return (Display *) 0xffff; }

void XtAppSetExitFlag(XtAppContext app_context) { NOREF(app_context); }

void XtDestroyWidget(Widget w) { NOREF(w); }

XtAppContext XtCreateApplicationContext(void) { return (XtAppContext)0xffff; }

void XtDestroyApplicationContext(XtAppContext app_context) { NOREF(app_context); }

void XtToolkitInitialize(void) {}

Boolean XtToolkitThreadInitialize(void) { return True; }

Display *XtOpenDisplay(XtAppContext app_context,
                       _Xconst _XtString display_string,
                       _Xconst _XtString application_name,
                       _Xconst _XtString application_class,
                       XrmOptionDescRec *options, Cardinal num_options,
                       int *argc, char **argv)
{
    RT_NOREF8(app_context, display_string, application_name, application_class, options, num_options, argc, argv);
    return (Display *)0xffff;
}

Widget XtVaAppCreateShell(_Xconst _XtString application_name,  _Xconst _XtString application_class,
                          WidgetClass widget_class, Display *display, ...)
{
    RT_NOREF4(application_name, application_class, widget_class, display);
    return TEST_WIDGET;
}

void XtSetMappedWhenManaged(Widget widget, _XtBoolean mapped_when_managed) { RT_NOREF2(widget, mapped_when_managed); }

void XtRealizeWidget(Widget widget) { NOREF(widget); }

XtInputId XtAppAddInput(XtAppContext app_context, int source, XtPointer condition, XtInputCallbackProc proc, XtPointer closure)
{
    RT_NOREF5(app_context, source, condition, proc, closure);
    return 0xffff;
}

/* Atoms we need other than the formats we support. */
static const char *g_apszSupAtoms[] =
{
    "PRIMARY", "CLIPBOARD", "TARGETS", "MULTIPLE", "TIMESTAMP"
};

/* This just looks for the atom names in a couple of tables and returns an
 * index with an offset added. */
Atom XInternAtom(Display *, const char *pcsz, int)
{
    Atom atom = 0;
    for (unsigned i = 0; i < RT_ELEMENTS(g_aFormats); ++i)
        if (!strcmp(pcsz, g_aFormats[i].pcszAtom))
            atom = (Atom) (i + 0x1000);
    for (unsigned i = 0; i < RT_ELEMENTS(g_apszSupAtoms); ++i)
        if (!strcmp(pcsz, g_apszSupAtoms[i]))
            atom = (Atom) (i + 0x2000);
    Assert(atom);  /* Have we missed any atoms? */
    return atom;
}

/* Take a request for the targets we are currently offering. */
static CLIPX11FORMAT g_selTargets[10] = { 0 };
static size_t g_cTargets = 0;

void testRequestTargets(CLIPBACKEND* pCtx)
{
    clipUpdateX11Targets(pCtx, g_selTargets, g_cTargets);
}

/* The current values of the X selection, which will be returned to the
 * XtGetSelectionValue callback. */
static Atom g_selType = 0;
static const void *g_pSelData = NULL;
static unsigned long g_cSelData = 0;
static int g_selFormat = 0;

void testRequestData(CLIPBACKEND *pCtx, CLIPX11FORMAT target, void *closure)
{
    RT_NOREF1(pCtx);
    unsigned long count = 0;
    int format = 0;
    if (target != g_selTargets[0])
    {
        clipConvertX11CB(closure, NULL, 0); /* Could not convert to target. */
        return;
    }
    void *pValue = NULL;
    pValue = g_pSelData ? RTMemDup(g_pSelData, g_cSelData) : NULL;
    count = g_pSelData ? g_cSelData : 0;
    format = g_selFormat;
    if (!pValue)
    {
        count = 0;
        format = 0;
    }
    clipConvertX11CB(closure, pValue, count * format / 8);
}

/* The formats currently on offer from X11 via the shared clipboard */
static uint32_t g_fX11Formats = 0;

void ClipReportX11Formats(VBOXCLIPBOARDCONTEXT *pCtx, uint32_t u32Formats)
{
    RT_NOREF1(pCtx);
    g_fX11Formats = u32Formats;
}

static uint32_t clipQueryFormats()
{
    return g_fX11Formats;
}

static void clipInvalidateFormats()
{
    g_fX11Formats = ~0;
}

/* Does our clipboard code currently own the selection? */
static bool g_ownsSel = false;
/* The procedure that is called when we should convert the selection to a
 * given format. */
static XtConvertSelectionProc g_pfnSelConvert = NULL;
/* The procedure which is called when we lose the selection. */
static XtLoseSelectionProc g_pfnSelLose = NULL;
/* The procedure which is called when the selection transfer has completed. */
static XtSelectionDoneProc g_pfnSelDone = NULL;

Boolean XtOwnSelection(Widget widget, Atom selection, Time time,
                       XtConvertSelectionProc convert,
                       XtLoseSelectionProc lose,
                       XtSelectionDoneProc done)
{
    RT_NOREF2(widget, time);
    if (selection != XInternAtom(NULL, "CLIPBOARD", 0))
        return True;  /* We don't really care about this. */
    g_ownsSel = true;  /* Always succeed. */
    g_pfnSelConvert = convert;
    g_pfnSelLose = lose;
    g_pfnSelDone = done;
    return True;
}

void XtDisownSelection(Widget widget, Atom selection, Time time)
{
    RT_NOREF3(widget, time, selection);
    g_ownsSel = false;
    g_pfnSelConvert = NULL;
    g_pfnSelLose = NULL;
    g_pfnSelDone = NULL;
}

/* Request the shared clipboard to convert its data to a given format. */
static bool clipConvertSelection(const char *pcszTarget, Atom *type,
                                 XtPointer *value, unsigned long *length,
                                 int *format)
{
    Atom target = XInternAtom(NULL, pcszTarget, 0);
    if (target == 0)
        return false;
    /* Initialise all return values in case we make a quick exit. */
    *type = XA_STRING;
    *value = NULL;
    *length = 0;
    *format = 0;
    if (!g_ownsSel)
        return false;
    if (!g_pfnSelConvert)
        return false;
    Atom clipAtom = XInternAtom(NULL, "CLIPBOARD", 0);
    if (!g_pfnSelConvert(TEST_WIDGET, &clipAtom, &target, type,
                         value, length, format))
        return false;
    if (g_pfnSelDone)
        g_pfnSelDone(TEST_WIDGET, &clipAtom, &target);
    return true;
}

/* Set the current X selection data */
static void clipSetSelectionValues(const char *pcszTarget, Atom type,
                                   const void *data,
                                   unsigned long count, int format)
{
    Atom clipAtom = XInternAtom(NULL, "CLIPBOARD", 0);
    g_selTargets[0] = clipFindX11FormatByAtomText(pcszTarget);
    g_cTargets = 1;
    g_selType = type;
    g_pSelData = data;
    g_cSelData = count;
    g_selFormat = format;
    if (g_pfnSelLose)
        g_pfnSelLose(TEST_WIDGET, &clipAtom);
    g_ownsSel = false;
}

static void clipSendTargetUpdate(CLIPBACKEND *pCtx)
{
    clipQueryX11CBFormats(pCtx);
}

/* Configure if and how the X11 TARGETS clipboard target will fail */
static void clipSetTargetsFailure(void)
{
    g_cTargets = 0;
}

char *XtMalloc(Cardinal size) { return (char *) RTMemAlloc(size); }

char *XGetAtomName(Display *display, Atom atom)
{
    RT_NOREF1(display);
    AssertReturn((unsigned)atom < RT_ELEMENTS(g_aFormats) + 1, NULL);
    const char *pcszName = NULL;
    if (atom < 0x1000)
        return NULL;
    if (0x1000 <= atom && atom < 0x2000)
    {
        unsigned index = atom - 0x1000;
        AssertReturn(index < RT_ELEMENTS(g_aFormats), NULL);
        pcszName = g_aFormats[index].pcszAtom;
    }
    else
    {
        unsigned index = atom - 0x2000;
        AssertReturn(index < RT_ELEMENTS(g_apszSupAtoms), NULL);
        pcszName = g_apszSupAtoms[index];
    }
    return (char *)RTMemDup(pcszName, sizeof(pcszName) + 1);
}

int XFree(void *data)
{
    RTMemFree(data);
    return 0;
}

void XFreeStringList(char **list)
{
    if (list)
        RTMemFree(*list);
    RTMemFree(list);
}

#define MAX_BUF_SIZE 256

static int g_completedRC = VINF_SUCCESS;
static int g_completedCB = 0;
static CLIPREADCBREQ *g_completedReq = NULL;
static char g_completedBuf[MAX_BUF_SIZE];

void ClipCompleteDataRequestFromX11(VBOXCLIPBOARDCONTEXT *pCtx, int rc, CLIPREADCBREQ *pReq, void *pv, uint32_t cb)
{
    RT_NOREF1(pCtx);
    if (cb <= MAX_BUF_SIZE)
    {
        g_completedRC = rc;
        memcpy(g_completedBuf, pv, cb);
    }
    else
        g_completedRC = VERR_BUFFER_OVERFLOW;
    g_completedCB = cb;
    g_completedReq = pReq;
}

static void clipGetCompletedRequest(int *prc, char ** ppc, uint32_t *pcb, CLIPREADCBREQ **ppReq)
{
    *prc = g_completedRC;
    *ppc = g_completedBuf;
    *pcb = g_completedCB;
    *ppReq = g_completedReq;
}
#ifdef RT_OS_SOLARIS_10
char XtStrings [] = "";
_WidgetClassRec* applicationShellWidgetClass;
char XtShellStrings [] = "";
int XmbTextPropertyToTextList(
    Display*            /* display */,
    XTextProperty*      /* text_prop */,
    char***             /* list_return */,
    int*                /* count_return */
)
{
  return 0;
}
#else
const char XtStrings [] = "";
_WidgetClassRec* applicationShellWidgetClass;
const char XtShellStrings [] = "";
#endif

static void testStringFromX11(RTTEST hTest, CLIPBACKEND *pCtx,
                              const char *pcszExp, int rcExp)
{
    bool retval = true;
    clipSendTargetUpdate(pCtx);
    if (clipQueryFormats() != VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
        RTTestFailed(hTest, "Wrong targets reported: %02X\n",
                     clipQueryFormats());
    else
    {
        char *pc;
        CLIPREADCBREQ *pReq = (CLIPREADCBREQ *)&pReq, *pReqRet = NULL;
        ClipRequestDataFromX11(pCtx, VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT,
                               pReq);
        int rc = VINF_SUCCESS;
        uint32_t cbActual = 0;
        clipGetCompletedRequest(&rc, &pc, &cbActual, &pReqRet);
        if (rc != rcExp)
            RTTestFailed(hTest, "Wrong return code, expected %Rrc, got %Rrc\n",
                         rcExp, rc);
        else if (pReqRet != pReq)
            RTTestFailed(hTest, "Wrong returned request data, expected %p, got %p\n",
                         pReq, pReqRet);
        else if (RT_FAILURE(rcExp))
            retval = true;
        else
        {
            RTUTF16 wcExp[MAX_BUF_SIZE / 2];
            RTUTF16 *pwcExp = wcExp;
            size_t cwc = 0;
            rc = RTStrToUtf16Ex(pcszExp, RTSTR_MAX, &pwcExp,
                                RT_ELEMENTS(wcExp), &cwc);
            size_t cbExp = cwc * 2 + 2;
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                if (cbActual != cbExp)
                {
                    RTTestFailed(hTest, "Returned string is the wrong size, string \"%.*ls\", size %u, expected \"%s\", size %u\n",
                                 RT_MIN(MAX_BUF_SIZE, cbActual), pc, cbActual,
                                 pcszExp, cbExp);
                }
                else
                {
                    if (memcmp(pc, wcExp, cbExp) == 0)
                        retval = true;
                    else
                        RTTestFailed(hTest, "Returned string \"%.*ls\" does not match expected string \"%s\"\n",
                                     MAX_BUF_SIZE, pc, pcszExp);
                }
            }
        }
    }
    if (!retval)
        RTTestFailureDetails(hTest, "Expected: string \"%s\", rc %Rrc\n",
                             pcszExp, rcExp);
}

static void testLatin1FromX11(RTTEST hTest, CLIPBACKEND *pCtx,
                              const char *pcszExp, int rcExp)
{
    bool retval = false;
    clipSendTargetUpdate(pCtx);
    if (clipQueryFormats() != VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
        RTTestFailed(hTest, "Wrong targets reported: %02X\n",
                     clipQueryFormats());
    else
    {
        char *pc;
        CLIPREADCBREQ *pReq = (CLIPREADCBREQ *)&pReq, *pReqRet = NULL;
        ClipRequestDataFromX11(pCtx, VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT,
                               pReq);
        int rc = VINF_SUCCESS;
        uint32_t cbActual = 0;
        clipGetCompletedRequest(&rc, &pc, &cbActual, &pReqRet);
        if (rc != rcExp)
            RTTestFailed(hTest, "Wrong return code, expected %Rrc, got %Rrc\n",
                         rcExp, rc);
        else if (pReqRet != pReq)
            RTTestFailed(hTest, "Wrong returned request data, expected %p, got %p\n",
                         pReq, pReqRet);
        else if (RT_FAILURE(rcExp))
            retval = true;
        else
        {
            RTUTF16 wcExp[MAX_BUF_SIZE / 2];
            //RTUTF16 *pwcExp = wcExp; - unused
            size_t cwc;
            for (cwc = 0; cwc == 0 || pcszExp[cwc - 1] != '\0'; ++cwc)
                wcExp[cwc] = pcszExp[cwc];
            size_t cbExp = cwc * 2;
            if (cbActual != cbExp)
            {
                RTTestFailed(hTest, "Returned string is the wrong size, string \"%.*ls\", size %u, expected \"%s\", size %u\n",
                             RT_MIN(MAX_BUF_SIZE, cbActual), pc, cbActual,
                             pcszExp, cbExp);
            }
            else
            {
                if (memcmp(pc, wcExp, cbExp) == 0)
                    retval = true;
                else
                    RTTestFailed(hTest, "Returned string \"%.*ls\" does not match expected string \"%s\"\n",
                                 MAX_BUF_SIZE, pc, pcszExp);
            }
        }
    }
    if (!retval)
        RTTestFailureDetails(hTest, "Expected: string \"%s\", rc %Rrc\n",
                             pcszExp, rcExp);
}

static void testStringFromVBox(RTTEST hTest, CLIPBACKEND *pCtx, const char *pcszTarget, Atom typeExp,  const char *valueExp)
{
    RT_NOREF1(pCtx);
    bool retval = false;
    Atom type;
    XtPointer value = NULL;
    unsigned long length;
    int format;
    size_t lenExp = strlen(valueExp);
    if (clipConvertSelection(pcszTarget, &type, &value, &length, &format))
    {
        if (   type != typeExp
            || length != lenExp
            || format != 8
            || memcmp((const void *) value, (const void *)valueExp,
                      lenExp))
        {
            RTTestFailed(hTest, "Bad data: type %d, (expected %d), length %u, (%u), format %d (%d), value \"%.*s\" (\"%.*s\")\n",
                     type, typeExp, length, lenExp, format, 8,
                     RT_MIN(length, 20), value, RT_MIN(lenExp, 20), valueExp);
        }
        else
            retval = true;
    }
    else
        RTTestFailed(hTest, "Conversion failed\n");
    XtFree((char *)value);
    if (!retval)
        RTTestFailureDetails(hTest, "Conversion to %s, expected \"%s\"\n",
                             pcszTarget, valueExp);
}

static void testNoX11(CLIPBACKEND *pCtx, const char *pcszTestCtx)
{
    CLIPREADCBREQ *pReq = (CLIPREADCBREQ *)&pReq;
    int rc = ClipRequestDataFromX11(pCtx,
                                    VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT,
                                    pReq);
    RTTESTI_CHECK_MSG(rc == VERR_NO_DATA, ("context: %s\n", pcszTestCtx));
}

static void testStringFromVBoxFailed(RTTEST hTest, CLIPBACKEND *pCtx, const char *pcszTarget)
{
    RT_NOREF1(pCtx);
    Atom type;
    XtPointer value = NULL;
    unsigned long length;
    int format;
    RTTEST_CHECK_MSG(hTest, !clipConvertSelection(pcszTarget, &type, &value,
                                                  &length, &format),
                     (hTest, "Conversion to target %s, should have failed but didn't, returned type %d, length %u, format %d, value \"%.*s\"\n",
                      pcszTarget, type, length, format, RT_MIN(length, 20),
                      value));
    XtFree((char *)value);
}

static void testNoSelectionOwnership(CLIPBACKEND *pCtx, const char *pcszTestCtx)
{
    RT_NOREF1(pCtx);
    RTTESTI_CHECK_MSG(!g_ownsSel, ("context: %s\n", pcszTestCtx));
}

int main()
{
    /*
     * Init the runtime, test and say hello.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstClipboardX11", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    /*
     * Run the test.
     */
    CLIPBACKEND *pCtx = ClipConstructX11(NULL, false);
    char *pc;
    uint32_t cbActual;
    CLIPREADCBREQ *pReq = (CLIPREADCBREQ *)&pReq, *pReqRet = NULL;
    rc = ClipStartX11(pCtx);
    AssertRCReturn(rc, 1);

    /*** Utf-8 from X11 ***/
    RTTestSub(hTest, "reading Utf-8 from X11");
    /* Simple test */
    clipSetSelectionValues("UTF8_STRING", XA_STRING, "hello world",
                           sizeof("hello world"), 8);
    testStringFromX11(hTest, pCtx, "hello world", VINF_SUCCESS);
    /* With an embedded carriage return */
    clipSetSelectionValues("text/plain;charset=UTF-8", XA_STRING,
                           "hello\nworld", sizeof("hello\nworld"), 8);
    testStringFromX11(hTest, pCtx, "hello\r\nworld", VINF_SUCCESS);
    /* With an embedded CRLF */
    clipSetSelectionValues("text/plain;charset=UTF-8", XA_STRING,
                           "hello\r\nworld", sizeof("hello\r\nworld"), 8);
    testStringFromX11(hTest, pCtx, "hello\r\r\nworld", VINF_SUCCESS);
    /* With an embedded LFCR */
    clipSetSelectionValues("text/plain;charset=UTF-8", XA_STRING,
                           "hello\n\rworld", sizeof("hello\n\rworld"), 8);
    testStringFromX11(hTest, pCtx, "hello\r\n\rworld", VINF_SUCCESS);
    /* An empty string */
    clipSetSelectionValues("text/plain;charset=utf-8", XA_STRING, "",
                           sizeof(""), 8);
    testStringFromX11(hTest, pCtx, "", VINF_SUCCESS);
    /* With an embedded Utf-8 character. */
    clipSetSelectionValues("STRING", XA_STRING,
                           "100\xE2\x82\xAC" /* 100 Euro */,
                           sizeof("100\xE2\x82\xAC"), 8);
    testStringFromX11(hTest, pCtx, "100\xE2\x82\xAC", VINF_SUCCESS);
    /* A non-zero-terminated string */
    clipSetSelectionValues("TEXT", XA_STRING,
                           "hello world", sizeof("hello world") - 1, 8);
    testStringFromX11(hTest, pCtx, "hello world", VINF_SUCCESS);

    /*** Latin1 from X11 ***/
    RTTestSub(hTest, "reading Latin1 from X11");
    /* Simple test */
    clipSetSelectionValues("STRING", XA_STRING, "Georges Dupr\xEA",
                           sizeof("Georges Dupr\xEA"), 8);
    testLatin1FromX11(hTest, pCtx, "Georges Dupr\xEA", VINF_SUCCESS);
    /* With an embedded carriage return */
    clipSetSelectionValues("TEXT", XA_STRING, "Georges\nDupr\xEA",
                           sizeof("Georges\nDupr\xEA"), 8);
    testLatin1FromX11(hTest, pCtx, "Georges\r\nDupr\xEA", VINF_SUCCESS);
    /* With an embedded CRLF */
    clipSetSelectionValues("TEXT", XA_STRING, "Georges\r\nDupr\xEA",
                           sizeof("Georges\r\nDupr\xEA"), 8);
    testLatin1FromX11(hTest, pCtx, "Georges\r\r\nDupr\xEA", VINF_SUCCESS);
    /* With an embedded LFCR */
    clipSetSelectionValues("TEXT", XA_STRING, "Georges\n\rDupr\xEA",
                           sizeof("Georges\n\rDupr\xEA"), 8);
    testLatin1FromX11(hTest, pCtx, "Georges\r\n\rDupr\xEA", VINF_SUCCESS);
    /* A non-zero-terminated string */
    clipSetSelectionValues("text/plain", XA_STRING,
                           "Georges Dupr\xEA!",
                           sizeof("Georges Dupr\xEA!") - 1, 8);
    testLatin1FromX11(hTest, pCtx, "Georges Dupr\xEA!", VINF_SUCCESS);

    /*** Unknown X11 format ***/
    RTTestSub(hTest, "handling of an unknown X11 format");
    clipInvalidateFormats();
    clipSetSelectionValues("CLIPBOARD", XA_STRING, "Test",
                           sizeof("Test"), 8);
    clipSendTargetUpdate(pCtx);
    RTTEST_CHECK_MSG(hTest, clipQueryFormats() == 0,
                     (hTest, "Failed to send a format update notification\n"));

    /*** Timeout from X11 ***/
    RTTestSub(hTest, "X11 timeout");
    clipSetSelectionValues("UTF8_STRING", XT_CONVERT_FAIL, NULL,0, 8);
    testStringFromX11(hTest, pCtx, "", VERR_NO_DATA);

    /*** No data in X11 clipboard ***/
    RTTestSub(hTest, "a data request from an empty X11 clipboard");
    clipSetSelectionValues("UTF8_STRING", XA_STRING, NULL,
                           0, 8);
    ClipRequestDataFromX11(pCtx, VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT,
                           pReq);
    clipGetCompletedRequest(&rc, &pc, &cbActual, &pReqRet);
    RTTEST_CHECK_MSG(hTest, rc == VERR_NO_DATA,
                     (hTest, "Returned %Rrc instead of VERR_NO_DATA\n",
                      rc));
    RTTEST_CHECK_MSG(hTest, pReqRet == pReq,
                     (hTest, "Wrong returned request data, expected %p, got %p\n",
                     pReq, pReqRet));

    /*** Ensure that VBox is notified when we return the CB to X11 ***/
    RTTestSub(hTest, "notification of switch to X11 clipboard");
    clipInvalidateFormats();
    clipReportEmptyX11CB(pCtx);
    RTTEST_CHECK_MSG(hTest, clipQueryFormats() == 0,
                     (hTest, "Failed to send a format update (release) notification\n"));

    /*** request for an invalid VBox format from X11 ***/
    RTTestSub(hTest, "a request for an invalid VBox format from X11");
    ClipRequestDataFromX11(pCtx, 0xffff, pReq);
    clipGetCompletedRequest(&rc, &pc, &cbActual, &pReqRet);
    RTTEST_CHECK_MSG(hTest, rc == VERR_NOT_IMPLEMENTED,
                     (hTest, "Returned %Rrc instead of VERR_NOT_IMPLEMENTED\n",
                      rc));
    RTTEST_CHECK_MSG(hTest, pReqRet == pReq,
                     (hTest, "Wrong returned request data, expected %p, got %p\n",
                     pReq, pReqRet));

    /*** Targets failure from X11 ***/
    RTTestSub(hTest, "X11 targets conversion failure");
    clipSetSelectionValues("UTF8_STRING", XA_STRING, "hello world",
                           sizeof("hello world"), 8);
    clipSetTargetsFailure();
    Atom atom = XA_STRING;
    long unsigned int cLen = 0;
    int format = 8;
    clipConvertX11Targets(NULL, (XtPointer) pCtx, NULL, &atom, NULL, &cLen,
                          &format);
    RTTEST_CHECK_MSG(hTest, clipQueryFormats() == 0,
                     (hTest, "Wrong targets reported: %02X\n",
                      clipQueryFormats()));

    /*** X11 text format conversion ***/
    RTTestSub(hTest, "handling of X11 selection targets");
    RTTEST_CHECK_MSG(hTest, clipTestTextFormatConversion(pCtx),
                     (hTest, "failed to select the right X11 text formats\n"));

    /*** Utf-8 from VBox ***/
    RTTestSub(hTest, "reading Utf-8 from VBox");
    /* Simple test */
    clipSetVBoxUtf16(pCtx, VINF_SUCCESS, "hello world",
                     sizeof("hello world") * 2);
    testStringFromVBox(hTest, pCtx, "UTF8_STRING",
                       clipGetAtom(pCtx, "UTF8_STRING"), "hello world");
    /* With an embedded carriage return */
    clipSetVBoxUtf16(pCtx, VINF_SUCCESS, "hello\r\nworld",
                     sizeof("hello\r\nworld") * 2);
    testStringFromVBox(hTest, pCtx, "text/plain;charset=UTF-8",
                       clipGetAtom(pCtx, "text/plain;charset=UTF-8"),
                       "hello\nworld");
    /* With an embedded CRCRLF */
    clipSetVBoxUtf16(pCtx, VINF_SUCCESS, "hello\r\r\nworld",
                     sizeof("hello\r\r\nworld") * 2);
    testStringFromVBox(hTest, pCtx, "text/plain;charset=UTF-8",
                       clipGetAtom(pCtx, "text/plain;charset=UTF-8"),
                       "hello\r\nworld");
    /* With an embedded CRLFCR */
    clipSetVBoxUtf16(pCtx, VINF_SUCCESS, "hello\r\n\rworld",
                     sizeof("hello\r\n\rworld") * 2);
    testStringFromVBox(hTest, pCtx, "text/plain;charset=UTF-8",
                       clipGetAtom(pCtx, "text/plain;charset=UTF-8"),
                       "hello\n\rworld");
    /* An empty string */
    clipSetVBoxUtf16(pCtx, VINF_SUCCESS, "", 2);
    testStringFromVBox(hTest, pCtx, "text/plain;charset=utf-8",
                       clipGetAtom(pCtx, "text/plain;charset=utf-8"), "");
    /* With an embedded Utf-8 character. */
    clipSetVBoxUtf16(pCtx, VINF_SUCCESS, "100\xE2\x82\xAC" /* 100 Euro */,
                     10);
    testStringFromVBox(hTest, pCtx, "STRING",
                       clipGetAtom(pCtx, "STRING"), "100\xE2\x82\xAC");
    /* A non-zero-terminated string */
    clipSetVBoxUtf16(pCtx, VINF_SUCCESS, "hello world",
                     sizeof("hello world") * 2 - 2);
    testStringFromVBox(hTest, pCtx, "TEXT", clipGetAtom(pCtx, "TEXT"),
                       "hello world");

    /*** Timeout from VBox ***/
    RTTestSub(hTest, "reading from VBox with timeout");
    clipEmptyVBox(pCtx, VERR_TIMEOUT);
    testStringFromVBoxFailed(hTest, pCtx, "UTF8_STRING");

    /*** No data in VBox clipboard ***/
    RTTestSub(hTest, "an empty VBox clipboard");
    clipSetSelectionValues("TEXT", XA_STRING, "", sizeof(""), 8);
    clipEmptyVBox(pCtx, VINF_SUCCESS);
    RTTEST_CHECK_MSG(hTest, g_ownsSel,
                     (hTest, "VBox grabbed the clipboard with no data and we ignored it\n"));
    testStringFromVBoxFailed(hTest, pCtx, "UTF8_STRING");

    /*** An unknown VBox format ***/
    RTTestSub(hTest, "reading an unknown VBox format");
    clipSetSelectionValues("TEXT", XA_STRING, "", sizeof(""), 8);
    clipSetVBoxUtf16(pCtx, VINF_SUCCESS, "", 2);
    ClipAnnounceFormatToX11(pCtx, 0xa0000);
    RTTEST_CHECK_MSG(hTest, g_ownsSel,
                     (hTest, "VBox grabbed the clipboard with unknown data and we ignored it\n"));
    testStringFromVBoxFailed(hTest, pCtx, "UTF8_STRING");
    rc = ClipStopX11(pCtx);
    AssertRCReturn(rc, 1);
    ClipDestructX11(pCtx);

    /*** Headless clipboard tests ***/

    pCtx = ClipConstructX11(NULL, true);
    rc = ClipStartX11(pCtx);
    AssertRCReturn(rc, 1);

    /*** Read from X11 ***/
    RTTestSub(hTest, "reading from X11, headless clipboard");
    /* Simple test */
    clipSetVBoxUtf16(pCtx, VINF_SUCCESS, "",
                     sizeof("") * 2);
    clipSetSelectionValues("UTF8_STRING", XA_STRING, "hello world",
                           sizeof("hello world"), 8);
    testNoX11(pCtx, "reading from X11, headless clipboard");

    /*** Read from VBox ***/
    RTTestSub(hTest, "reading from VBox, headless clipboard");
    /* Simple test */
    clipEmptyVBox(pCtx, VERR_WRONG_ORDER);
    clipSetSelectionValues("TEXT", XA_STRING, "", sizeof(""), 8);
    clipSetVBoxUtf16(pCtx, VINF_SUCCESS, "hello world",
                     sizeof("hello world") * 2);
    testNoSelectionOwnership(pCtx, "reading from VBox, headless clipboard");

    rc = ClipStopX11(pCtx);
    AssertRCReturn(rc, 1);
    ClipDestructX11(pCtx);

    return RTTestSummaryAndDestroy(hTest);
}

#endif

#ifdef SMOKETEST

/* This is a simple test case that just starts a copy of the X11 clipboard
 * backend, checks the X11 clipboard and exits.  If ever needed I will add an
 * interactive mode in which the user can read and copy to the clipboard from
 * the command line. */

# include <iprt/env.h>
# include <iprt/test.h>

int ClipRequestDataForX11(VBOXCLIPBOARDCONTEXT *pCtx, uint32_t u32Format, void **ppv, uint32_t *pcb)
{
    RT_NOREF4(pCtx, u32Format, ppv, pcb);
    return VERR_NO_DATA;
}

void ClipReportX11Formats(VBOXCLIPBOARDCONTEXT *pCtx, uint32_t u32Formats)
{
    RT_NOREF2(pCtx, u32Formats);
}

void ClipCompleteDataRequestFromX11(VBOXCLIPBOARDCONTEXT *pCtx, int rc, CLIPREADCBREQ *pReq, void *pv, uint32_t cb)
{
    RT_NOREF5(pCtx, rc, pReq, pv, cb);
}

int main()
{
    /*
     * Init the runtime, test and say hello.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstClipboardX11Smoke", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    /*
     * Run the test.
     */
    rc = VINF_SUCCESS;
    /* We can't test anything without an X session, so just return success
     * in that case. */
    if (!RTEnvExist("DISPLAY"))
    {
        RTTestPrintf(hTest, RTTESTLVL_INFO,
                     "X11 not available, not running test\n");
        return RTTestSummaryAndDestroy(hTest);
    }
    CLIPBACKEND *pCtx = ClipConstructX11(NULL, false);
    AssertReturn(pCtx, 1);
    rc = ClipStartX11(pCtx);
    AssertRCReturn(rc, 1);
    /* Give the clipboard time to synchronise. */
    RTThreadSleep(500);
    rc = ClipStopX11(pCtx);
    AssertRCReturn(rc, 1);
    ClipDestructX11(pCtx);
    return RTTestSummaryAndDestroy(hTest);
}

#endif /* SMOKETEST defined */

