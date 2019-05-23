/* $Id: server_presenter.h $ */

/** @file
 * Presenter API definitions.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __SERVER_PRESENTER_H__
#define __SERVER_PRESENTER_H__

#include "cr_spu.h"
#include "chromium.h"
#include "cr_error.h"
#include "cr_net.h"
#include "cr_rand.h"
#include "server_dispatch.h"
#include "server.h"
#include "cr_mem.h"
#include "cr_string.h"
#include <cr_vreg.h>
#include <cr_htable.h>
#include <cr_bmpscale.h>

#include "render/renderspu.h"

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/asm.h>
#include <iprt/mem.h>
#include <iprt/list.h>


class ICrFbDisplay
{
    public:
        virtual int UpdateBegin(struct CR_FRAMEBUFFER *pFb) = 0;
        virtual void UpdateEnd(struct CR_FRAMEBUFFER *pFb) = 0;

        virtual int EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry) = 0;
        virtual int EntryAdded(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry) = 0;
        virtual int EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry, HCR_FRAMEBUFFER_ENTRY hReplacedEntry) = 0;
        virtual int EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry) = 0;
        virtual int EntryRemoved(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry) = 0;
        virtual int EntryDestroyed(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry) = 0;
        virtual int EntryPosChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry) = 0;

        virtual int RegionsChanged(struct CR_FRAMEBUFFER *pFb) = 0;

        virtual int FramebufferChanged(struct CR_FRAMEBUFFER *pFb) = 0;

        virtual ~ICrFbDisplay() {}
};


typedef struct CR_FRAMEBUFFER
{
    VBOXVR_SCR_COMPOSITOR Compositor;
    struct VBVAINFOSCREEN ScreenInfo;
    void *pvVram;
    ICrFbDisplay *pDisplay;
    RTLISTNODE EntriesList;
    uint32_t cEntries; /* <- just for debugging */
    uint32_t cUpdating;
    CRHTABLE SlotTable;
} CR_FRAMEBUFFER;


typedef union CR_FBENTRY_FLAGS
{
    struct {
        uint32_t fCreateNotified : 1;
        uint32_t fInList         : 1;
        uint32_t Reserved        : 30;
    };
    uint32_t Value;
} CR_FBENTRY_FLAGS;


typedef struct CR_FRAMEBUFFER_ENTRY
{
    VBOXVR_SCR_COMPOSITOR_ENTRY Entry;
    RTLISTNODE Node;
    uint32_t cRefs;
    CR_FBENTRY_FLAGS Flags;
    CRHTABLE HTable;
} CR_FRAMEBUFFER_ENTRY;


typedef struct CR_FBTEX
{
    CR_TEXDATA Tex;
    CRTextureObj *pTobj;
} CR_FBTEX;


class CrFbDisplayBase : public ICrFbDisplay
{
    public:

        CrFbDisplayBase();

        virtual bool isComposite();
        class CrFbDisplayComposite* getContainer();
        bool isInList();
        bool isUpdating();
        int setRegionsChanged();
        int setFramebuffer(struct CR_FRAMEBUFFER *pFb);
        struct CR_FRAMEBUFFER* getFramebuffer();
        virtual int UpdateBegin(struct CR_FRAMEBUFFER *pFb);
        virtual void UpdateEnd(struct CR_FRAMEBUFFER *pFb);
        virtual int EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryAdded(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry, HCR_FRAMEBUFFER_ENTRY hReplacedEntry);
        virtual int EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryRemoved(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryDestroyed(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryPosChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int RegionsChanged(struct CR_FRAMEBUFFER *pFb);
        virtual int FramebufferChanged(struct CR_FRAMEBUFFER *pFb);
        virtual ~CrFbDisplayBase();

        /*@todo: move to protected and switch from RTLISTNODE*/
        RTLISTNODE mNode;
        class CrFbDisplayComposite* mpContainer;

    protected:

        virtual void onUpdateEnd();
        virtual void ueRegions();
        static DECLCALLBACK(bool) entriesCreateCb(HCR_FRAMEBUFFER hFb, HCR_FRAMEBUFFER_ENTRY hEntry, void *pvContext);
        static DECLCALLBACK(bool) entriesDestroyCb(HCR_FRAMEBUFFER hFb, HCR_FRAMEBUFFER_ENTRY hEntry, void *pvContext);
        int fbSynchAddAllEntries();
        int fbCleanupRemoveAllEntries();
        virtual int setFramebufferBegin(struct CR_FRAMEBUFFER *pFb);
        virtual void setFramebufferEnd(struct CR_FRAMEBUFFER *pFb);
        static DECLCALLBACK(void) slotEntryReleaseCB(HCR_FRAMEBUFFER hFb, HCR_FRAMEBUFFER_ENTRY hEntry, void *pvContext);
        virtual void slotRelease();
        virtual int fbCleanup();
        virtual int fbSync();
        CRHTABLE_HANDLE slotGet();

    private:

        typedef union CR_FBDISPBASE_FLAGS
        {
            struct {
                uint32_t fRegionsShanged : 1;
                uint32_t Reserved        : 31;
            };
            uint32_t u32Value;
        } CR_FBDISPBASE_FLAGS;

        struct CR_FRAMEBUFFER *mpFb;
        uint32_t mcUpdates;
        CRHTABLE_HANDLE mhSlot;
        CR_FBDISPBASE_FLAGS mFlags;
};


class CrFbDisplayComposite : public CrFbDisplayBase
{
    public:

        CrFbDisplayComposite();
        virtual bool isComposite();
        uint32_t getDisplayCount();
        bool add(CrFbDisplayBase *pDisplay);
        bool remove(CrFbDisplayBase *pDisplay, bool fCleanupDisplay = true);
        CrFbDisplayBase* first();
        CrFbDisplayBase* next(CrFbDisplayBase* pDisplay);
        virtual int setFramebuffer(struct CR_FRAMEBUFFER *pFb);
        virtual int UpdateBegin(struct CR_FRAMEBUFFER *pFb);
        virtual void UpdateEnd(struct CR_FRAMEBUFFER *pFb);
        virtual int EntryAdded(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry, HCR_FRAMEBUFFER_ENTRY hReplacedEntry);
        virtual int EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryRemoved(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryDestroyed(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int RegionsChanged(struct CR_FRAMEBUFFER *pFb);
        virtual int FramebufferChanged(struct CR_FRAMEBUFFER *pFb);
        virtual ~CrFbDisplayComposite();
        void cleanup(bool fCleanupDisplays = true);

    private:

        RTLISTNODE mDisplays;
        uint32_t mcDisplays;
};


class CrFbWindow
{
    public:

        CrFbWindow(uint64_t parentId);
        bool IsCreated() const;
        bool IsVisivle() const;
        void Destroy();
        int Reparent(uint64_t parentId);
        int SetVisible(bool fVisible);
        int SetSize(uint32_t width, uint32_t height, bool fForced=false);
        int SetPosition(int32_t x, int32_t y, bool fForced=false);
        int SetVisibleRegionsChanged();
        int SetCompositor(const struct VBOXVR_SCR_COMPOSITOR * pCompositor);
        bool SetScaleFactor(GLdouble scaleFactorW, GLdouble scaleFactorH);
        bool GetScaleFactor(GLdouble *scaleFactorW, GLdouble *scaleFactorH);
        int UpdateBegin();
        void UpdateEnd();
        uint64_t GetParentId();
        int Create();
        ~CrFbWindow();

    protected:

        void checkRegions();
        bool isPresentNeeded();
        bool checkInitedUpdating();

    private:

        typedef union CR_FBWIN_FLAGS
        {
            struct {
                uint32_t fVisible : 1;
                uint32_t fDataPresented : 1;
                uint32_t fForcePresentOnReenable : 1;
                uint32_t fCompositoEntriesModified : 1;
                uint32_t Reserved : 28;
            };
            uint32_t Value;
        } CR_FBWIN_FLAGS;

        GLint mSpuWindow;
        const struct VBOXVR_SCR_COMPOSITOR * mpCompositor;
        uint32_t mcUpdates;
        int32_t mxPos;
        int32_t myPos;
        uint32_t mWidth;
        uint32_t mHeight;
        CR_FBWIN_FLAGS mFlags;
        uint64_t mParentId;

        RTSEMRW scaleFactorLock;
        GLdouble mScaleFactorWStorage;
        GLdouble mScaleFactorHStorage;
};


class CrFbDisplayWindow : public CrFbDisplayBase
{
    public:

        CrFbDisplayWindow(const RTRECT *pViewportRect, uint64_t parentId);
        virtual ~CrFbDisplayWindow();
        virtual int UpdateBegin(struct CR_FRAMEBUFFER *pFb);
        virtual void UpdateEnd(struct CR_FRAMEBUFFER *pFb);
        virtual int RegionsChanged(struct CR_FRAMEBUFFER *pFb);
        virtual int EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry, HCR_FRAMEBUFFER_ENTRY hReplacedEntry);
        virtual int EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int FramebufferChanged(struct CR_FRAMEBUFFER *pFb);
        const RTRECT* getViewportRect();
        virtual int setViewportRect(const RTRECT *pViewportRect);
        virtual CrFbWindow * windowDetach(bool fCleanup = true);
        virtual CrFbWindow * windowAttach(CrFbWindow * pNewWindow);
        virtual int reparent(uint64_t parentId);
        virtual bool isVisible();
        int winVisibilityChanged();
        CrFbWindow* getWindow();

    protected:

        virtual void onUpdateEnd();
        virtual void ueRegions();
        virtual int screenChanged();
        virtual int windowSetCompositor(bool fSet);
        virtual int windowCleanup();
        virtual int fbCleanup();
        bool isActive();
        int windowDimensionsSync(bool fForceCleanup = false);
        virtual int windowSync();
        virtual int fbSync();
        virtual const struct RTRECT* getRect();

    private:

        typedef union CR_FBDISPWINDOW_FLAGS
        {
            struct {
                uint32_t fNeVisible : 1;
                uint32_t fNeForce   : 1;
                uint32_t Reserved   : 30;
            };
            uint32_t u32Value;
        } CR_FBDISPWINDOW_FLAGS;

        CrFbWindow *mpWindow;
        RTRECT mViewportRect;
        CR_FBDISPWINDOW_FLAGS mFlags;
        uint32_t mu32Screen;
        uint64_t mParentId;
};


class CrFbDisplayWindowRootVr : public CrFbDisplayWindow
{
    public:

        CrFbDisplayWindowRootVr(const RTRECT *pViewportRect, uint64_t parentId);
        virtual int EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryAdded(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry, HCR_FRAMEBUFFER_ENTRY hReplacedEntry);
        virtual int EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryRemoved(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryDestroyed(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int setViewportRect(const RTRECT *pViewportRect);

    protected:

        virtual int windowSetCompositor(bool fSet);
        virtual void ueRegions();
        int compositorMarkUpdated();
        virtual int screenChanged();
        virtual const struct RTRECT* getRect();
        virtual int fbCleanup();
        virtual int fbSync();
        VBOXVR_SCR_COMPOSITOR_ENTRY* entryAlloc();
        void entryFree(VBOXVR_SCR_COMPOSITOR_ENTRY* pEntry);
        int synchCompositorRegions();
        virtual int synchCompositor();
        virtual int clearCompositor();
        void rootVrTranslateForPos();
        static DECLCALLBACK(VBOXVR_SCR_COMPOSITOR_ENTRY*) rootVrGetCEntry(const VBOXVR_SCR_COMPOSITOR_ENTRY*pEntry, void *pvContext);

    private:

        VBOXVR_SCR_COMPOSITOR mCompositor;
};


class CrFbDisplayVrdp : public CrFbDisplayBase
{
    public:

        CrFbDisplayVrdp();
        virtual int EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry, HCR_FRAMEBUFFER_ENTRY hReplacedEntry);
        virtual int EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryRemoved(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryDestroyed(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int EntryPosChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        virtual int RegionsChanged(struct CR_FRAMEBUFFER *pFb);
        virtual int FramebufferChanged(struct CR_FRAMEBUFFER *pFb);

    protected:

        void syncPos();
        virtual int fbCleanup();
        virtual int fbSync();
        void vrdpDestroy(HCR_FRAMEBUFFER_ENTRY hEntry);
        void vrdpGeometry(HCR_FRAMEBUFFER_ENTRY hEntry);
        int vrdpRegions(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        int vrdpFrame(HCR_FRAMEBUFFER_ENTRY hEntry);
        int vrdpRegionsAll(struct CR_FRAMEBUFFER *pFb);
        int vrdpSynchEntry(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry);
        int vrdpSyncEntryAll(struct CR_FRAMEBUFFER *pFb);
        int vrdpCreate(HCR_FRAMEBUFFER hFb, HCR_FRAMEBUFFER_ENTRY hEntry);

    private:

        RTPOINT mPos;
};


typedef struct CR_FB_INFO
{
    CrFbDisplayComposite *pDpComposite;
    uint32_t u32Id;
    VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aTargetMap);
} CR_FB_INFO;

typedef struct CR_FBDISPLAY_INFO
{
    CrFbDisplayWindow *pDpWin;
    CrFbDisplayWindowRootVr *pDpWinRootVr;
    CrFbDisplayVrdp *pDpVrdp;
    CrFbWindow *pWindow;
    uint32_t u32DisplayMode;
    uint32_t u32Id;
    int32_t iFb;

    /* Cache scaling factor here before display output
     * initialized (i.e., guest not yet initiated first 3D call).
     * No synchronization stuff needed here because all the reads
     * and writes are done in context of 3D HGCM thread. */
    double dInitialScaleFactorW;
    double dInitialScaleFactorH;
} CR_FBDISPLAY_INFO;

typedef struct CR_PRESENTER_GLOBALS
{
#ifndef VBOXVDBG_MEMCACHE_DISABLE
    RTMEMCACHE FbEntryLookasideList;
    RTMEMCACHE FbTexLookasideList;
    RTMEMCACHE CEntryLookasideList;
#endif
    uint32_t u32DisplayMode;
    uint32_t u32DisabledDisplayMode;
    bool fEnabled;
    CRHashTable *pFbTexMap;
    CR_FBDISPLAY_INFO aDisplayInfos[CR_MAX_GUEST_MONITORS];
    CR_FBMAP FramebufferInitMap;
    CR_FRAMEBUFFER aFramebuffers[CR_MAX_GUEST_MONITORS];
    CR_FB_INFO aFbInfos[CR_MAX_GUEST_MONITORS];
    bool fWindowsForceHidden;
    uint32_t cbTmpBuf;
    void *pvTmpBuf;
    uint32_t cbTmpBuf2;
    void *pvTmpBuf2;
} CR_PRESENTER_GLOBALS;

extern CR_PRESENTER_GLOBALS g_CrPresenter;


HCR_FRAMEBUFFER_ENTRY CrFbEntryFromCompositorEntry(const struct VBOXVR_SCR_COMPOSITOR_ENTRY* pCEntry);

#endif /* __SERVER_PRESENTER_H__ */

