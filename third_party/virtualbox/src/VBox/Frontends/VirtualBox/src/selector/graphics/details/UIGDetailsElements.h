/* $Id: UIGDetailsElements.h $ */
/** @file
 * VBox Qt GUI - UIGDetailsElement[Name] classes declaration.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIGDetailsElements_h___
#define ___UIGDetailsElements_h___

/* GUI includes: */
#include "UIThreadPool.h"
#include "UIGDetailsElement.h"

/* Forward declarations: */
class UIGMachinePreview;
class CNetworkAdapter;


/** UITask extension used as update task for the details-element. */
class UIGDetailsUpdateTask : public UITask
{
    Q_OBJECT;

public:

    /** Constructs update task taking @a machine as data. */
    UIGDetailsUpdateTask(const CMachine &machine);
};

/** UIGDetailsElement extension used as a wrapping interface to
  * extend base-class with async functionality performed by the COM worker-threads. */
class UIGDetailsElementInterface : public UIGDetailsElement
{
    Q_OBJECT;

public:

    /** Constructs details-element interface for passed @a pParent set.
      * @param type    brings the details-element type this element belongs to.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementInterface(UIGDetailsSet *pParent, DetailsElementType type, bool fOpened);

protected:

    /** Performs translation. */
    virtual void retranslateUi();

    /** Updates appearance. */
    virtual void updateAppearance();

    /** Creates update task. */
    virtual UITask* createUpdateTask() = 0;

private slots:

    /** Handles the signal about update @a pTask is finished. */
    virtual void sltUpdateAppearanceFinished(UITask *pTask);

private:

    /** Holds the instance of the update task. */
    UITask *m_pTask;
};


/** UIGDetailsElementInterface extension for the details-element type 'Preview'. */
class UIGDetailsElementPreview : public UIGDetailsElement
{
    Q_OBJECT;

public:

    /** Constructs details-element interface for passed @a pParent set.
      * @param fOpened brings whether the details-element should be opened. */
    UIGDetailsElementPreview(UIGDetailsSet *pParent, bool fOpened);

private slots:

    /** Handles preview size-hint changes. */
    void sltPreviewSizeHintChanged();

private:

    /** Performs translation. */
    virtual void retranslateUi();

    /** Returns minimum width hint. */
    int minimumWidthHint() const;
    /** Returns minimum height hint.
      * @param fClosed allows to specify whether the hint should
      *                be calculated for the closed element. */
    int minimumHeightHint(bool fClosed) const;
    /** Updates layout. */
    void updateLayout();

    /** Updates appearance. */
    void updateAppearance();

    /** Holds the instance of VM preview. */
    UIGMachinePreview *m_pPreview;
};


/** UITask extension used as update task for the details-element type 'General'. */
class UIGDetailsUpdateTaskGeneral : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskGeneral(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();
};

/** UIGDetailsElementInterface extension for the details-element type 'General'. */
class UIGDetailsElementGeneral : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementGeneral(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_General, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskGeneral(machine()); }
};


/** UITask extension used as update task for the details-element type 'System'. */
class UIGDetailsUpdateTaskSystem : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskSystem(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();
};

/** UIGDetailsElementInterface extension for the details-element type 'System'. */
class UIGDetailsElementSystem : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementSystem(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_System, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskSystem(machine()); }
};


/** UITask extension used as update task for the details-element type 'Display'. */
class UIGDetailsUpdateTaskDisplay : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskDisplay(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();
};

/** UIGDetailsElementInterface extension for the details-element type 'Display'. */
class UIGDetailsElementDisplay : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementDisplay(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_Display, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskDisplay(machine()); }
};


/** UITask extension used as update task for the details-element type 'Storage'. */
class UIGDetailsUpdateTaskStorage : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskStorage(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();
};

/** UIGDetailsElementInterface extension for the details-element type 'Storage'. */
class UIGDetailsElementStorage : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementStorage(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_Storage, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskStorage(machine()); }
};


/** UITask extension used as update task for the details-element type 'Audio'. */
class UIGDetailsUpdateTaskAudio : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskAudio(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();
};

/** UIGDetailsElementInterface extension for the details-element type 'Audio'. */
class UIGDetailsElementAudio : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementAudio(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_Audio, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskAudio(machine()); }
};


/** UITask extension used as update task for the details-element type 'Network'. */
class UIGDetailsUpdateTaskNetwork : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskNetwork(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();

    /** Summarizes generic properties. */
    static QString summarizeGenericProperties(const CNetworkAdapter &adapter);
};

/** UIGDetailsElementInterface extension for the details-element type 'Network'. */
class UIGDetailsElementNetwork : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementNetwork(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_Network, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskNetwork(machine()); }
};


/** UITask extension used as update task for the details-element type 'Serial'. */
class UIGDetailsUpdateTaskSerial : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskSerial(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();
};

/** UIGDetailsElementInterface extension for the details-element type 'Serial'. */
class UIGDetailsElementSerial : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementSerial(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_Serial, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskSerial(machine()); }
};


/** UITask extension used as update task for the details-element type 'USB'. */
class UIGDetailsUpdateTaskUSB : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskUSB(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();
};

/** UIGDetailsElementInterface extension for the details-element type 'USB'. */
class UIGDetailsElementUSB : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementUSB(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_USB, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskUSB(machine()); }
};


/** UITask extension used as update task for the details-element type 'SF'. */
class UIGDetailsUpdateTaskSF : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskSF(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();
};

/** UIGDetailsElementInterface extension for the details-element type 'SF'. */
class UIGDetailsElementSF : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementSF(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_SF, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskSF(machine()); }
};


/** UITask extension used as update task for the details-element type 'UI'. */
class UIGDetailsUpdateTaskUI : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskUI(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();
};

/** UIGDetailsElementInterface extension for the details-element type 'UI'. */
class UIGDetailsElementUI : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementUI(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_UI, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskUI(machine()); }
};


/** UITask extension used as update task for the details-element type 'Description'. */
class UIGDetailsUpdateTaskDescription : public UIGDetailsUpdateTask
{
    Q_OBJECT;

public:

    /** Constructs update task passing @a machine to the base-class. */
    UIGDetailsUpdateTaskDescription(const CMachine &machine)
        : UIGDetailsUpdateTask(machine) {}

private:

    /** Contains update task body. */
    void run();
};

/** UIGDetailsElementInterface extension for the details-element type 'Description'. */
class UIGDetailsElementDescription : public UIGDetailsElementInterface
{
    Q_OBJECT;

public:

    /** Constructs details-element object for passed @a pParent set.
      * @param fOpened brings whether the details-element should be visually opened. */
    UIGDetailsElementDescription(UIGDetailsSet *pParent, bool fOpened)
        : UIGDetailsElementInterface(pParent, DetailsElementType_Description, fOpened) {}

private:

    /** Creates update task for this element. */
    UITask* createUpdateTask() { return new UIGDetailsUpdateTaskDescription(machine()); }
};

#endif /* !___UIGDetailsElements_h___ */

