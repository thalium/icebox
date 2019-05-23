/* $Id: UIMachineSettingsSerial.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsSerial class implementation.
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

/* Qt includes: */
# include <QDir>

/* GUI includes: */
# include "QITabWidget.h"
# include "QIWidgetValidator.h"
# include "UIConverter.h"
# include "UIMachineSettingsSerial.h"
# include "UIErrorString.h"
# include "VBoxGlobal.h"

/* COM includes: */
# include "CSerialPort.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Machine settings: Serial Port tab data structure. */
struct UIDataSettingsMachineSerialPort
{
    /** Constructs data. */
    UIDataSettingsMachineSerialPort()
        : m_iSlot(-1)
        , m_fPortEnabled(false)
        , m_uIRQ(0)
        , m_uIOBase(0)
        , m_hostMode(KPortMode_Disconnected)
        , m_fServer(false)
        , m_strPath(QString())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineSerialPort &other) const
    {
        return true
               && (m_iSlot == other.m_iSlot)
               && (m_fPortEnabled == other.m_fPortEnabled)
               && (m_uIRQ == other.m_uIRQ)
               && (m_uIOBase == other.m_uIOBase)
               && (m_hostMode == other.m_hostMode)
               && (m_fServer == other.m_fServer)
               && (m_strPath == other.m_strPath)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineSerialPort &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineSerialPort &other) const { return !equal(other); }

    /** Holds the serial port slot number. */
    int        m_iSlot;
    /** Holds whether the serial port is enabled. */
    bool       m_fPortEnabled;
    /** Holds the serial port IRQ. */
    ulong      m_uIRQ;
    /** Holds the serial port IO base. */
    ulong      m_uIOBase;
    /** Holds the serial port host mode. */
    KPortMode  m_hostMode;
    /** Holds whether the serial port is server. */
    bool       m_fServer;
    /** Holds the serial port path. */
    QString    m_strPath;
};


/** Machine settings: Serial page data structure. */
struct UIDataSettingsMachineSerial
{
    /** Constructs data. */
    UIDataSettingsMachineSerial() {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineSerial & /* other */) const { return true; }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineSerial & /* other */) const { return false; }
};


/** Machine settings: Serial Port tab. */
class UIMachineSettingsSerial : public QIWithRetranslateUI<QWidget>,
                                public Ui::UIMachineSettingsSerial
{
    Q_OBJECT;

public:

    UIMachineSettingsSerial(UIMachineSettingsSerialPage *pParent);

    void polishTab();

    void loadPortData(const UIDataSettingsMachineSerialPort &portData);
    void savePortData(UIDataSettingsMachineSerialPort &portData);

    QWidget *setOrderAfter(QWidget *pAfter);

    QString pageTitle() const;
    bool isUserDefined();

protected:

    void retranslateUi();

private slots:

    void sltGbSerialToggled(bool fOn);
    void sltCbNumberActivated(const QString &strText);
    void sltCbModeActivated(const QString &strText);

private:

    /* Helper: Prepare stuff: */
    void prepareValidation();

    UIMachineSettingsSerialPage *m_pParent;
    int m_iSlot;
};


/*********************************************************************************************************************************
*   Class UIMachineSettingsSerial implementation.                                                                                *
*********************************************************************************************************************************/

UIMachineSettingsSerial::UIMachineSettingsSerial(UIMachineSettingsSerialPage *pParent)
    : QIWithRetranslateUI<QWidget>(0)
    , m_pParent(pParent)
    , m_iSlot(-1)
{
    /* Apply UI decorations: */
    Ui::UIMachineSettingsSerial::setupUi(this);

    /* Setup validation: */
    mLeIRQ->setValidator(new QIULongValidator(0, 255, this));
    mLeIOPort->setValidator(new QIULongValidator(0, 0xFFFF, this));
    mLePath->setValidator(new QRegExpValidator(QRegExp(".+"), this));

    /* Setup constraints: */
    mLeIRQ->setFixedWidth(mLeIRQ->fontMetrics().width("8888"));
    mLeIOPort->setFixedWidth(mLeIOPort->fontMetrics().width("8888888"));

    /* Set initial values: */
    /* Note: If you change one of the following don't forget retranslateUi. */
    mCbNumber->insertItem(0, vboxGlobal().toCOMPortName(0, 0));
    mCbNumber->insertItems(0, vboxGlobal().COMPortNames());

    mCbMode->addItem(""); /* KPortMode_Disconnected */
    mCbMode->addItem(""); /* KPortMode_HostPipe */
    mCbMode->addItem(""); /* KPortMode_HostDevice */
    mCbMode->addItem(""); /* KPortMode_RawFile */
    mCbMode->addItem(""); /* KPortMode_TCP */

    /* Setup connections: */
    connect(mGbSerial, SIGNAL(toggled(bool)),
            this, SLOT(sltGbSerialToggled(bool)));
    connect(mCbNumber, SIGNAL(activated(const QString &)),
            this, SLOT(sltCbNumberActivated(const QString &)));
    connect(mCbMode, SIGNAL(activated(const QString &)),
            this, SLOT(sltCbModeActivated(const QString &)));

    /* Prepare validation: */
    prepareValidation();

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsSerial::polishTab()
{
    /* Polish port page: */
    ulong uIRQ, uIOBase;
    const bool fStd = vboxGlobal().toCOMPortNumbers(mCbNumber->currentText(), uIRQ, uIOBase);
    const KPortMode enmMode = gpConverter->fromString<KPortMode>(mCbMode->currentText());
    mGbSerial->setEnabled(m_pParent->isMachineOffline());
    mLbNumber->setEnabled(m_pParent->isMachineOffline());
    mCbNumber->setEnabled(m_pParent->isMachineOffline());
    mLbIRQ->setEnabled(m_pParent->isMachineOffline());
    mLeIRQ->setEnabled(!fStd && m_pParent->isMachineOffline());
    mLbIOPort->setEnabled(m_pParent->isMachineOffline());
    mLeIOPort->setEnabled(!fStd && m_pParent->isMachineOffline());
    mLbMode->setEnabled(m_pParent->isMachineOffline());
    mCbMode->setEnabled(m_pParent->isMachineOffline());
    mCbPipe->setEnabled(   (enmMode == KPortMode_HostPipe || enmMode == KPortMode_TCP)
                        && m_pParent->isMachineOffline());
    mLbPath->setEnabled(m_pParent->isMachineOffline());
    mLePath->setEnabled(enmMode != KPortMode_Disconnected && m_pParent->isMachineOffline());
}

void UIMachineSettingsSerial::loadPortData(const UIDataSettingsMachineSerialPort &portData)
{
    /* Load port number: */
    m_iSlot = portData.m_iSlot;

    /* Load port data: */
    mGbSerial->setChecked(portData.m_fPortEnabled);
    mCbNumber->setCurrentIndex(mCbNumber->findText(vboxGlobal().toCOMPortName(portData.m_uIRQ, portData.m_uIOBase)));
    mLeIRQ->setText(QString::number(portData.m_uIRQ));
    mLeIOPort->setText("0x" + QString::number(portData.m_uIOBase, 16).toUpper());
    mCbMode->setCurrentIndex(mCbMode->findText(gpConverter->toString(portData.m_hostMode)));
    mCbPipe->setChecked(!portData.m_fServer);
    mLePath->setText(portData.m_strPath);

    /* Ensure everything is up-to-date */
    sltGbSerialToggled(mGbSerial->isChecked());
}

void UIMachineSettingsSerial::savePortData(UIDataSettingsMachineSerialPort &portData)
{
    /* Save port data: */
    portData.m_fPortEnabled = mGbSerial->isChecked();
    portData.m_uIRQ = mLeIRQ->text().toULong(NULL, 0);
    portData.m_uIOBase = mLeIOPort->text().toULong(NULL, 0);
    portData.m_fServer = !mCbPipe->isChecked();
    portData.m_hostMode = gpConverter->fromString<KPortMode>(mCbMode->currentText());
    portData.m_strPath = QDir::toNativeSeparators(mLePath->text());
}

QWidget *UIMachineSettingsSerial::setOrderAfter(QWidget *pAfter)
{
    setTabOrder(pAfter, mGbSerial);
    setTabOrder(mGbSerial, mCbNumber);
    setTabOrder(mCbNumber, mLeIRQ);
    setTabOrder(mLeIRQ, mLeIOPort);
    setTabOrder(mLeIOPort, mCbMode);
    setTabOrder(mCbMode, mCbPipe);
    setTabOrder(mCbPipe, mLePath);
    return mLePath;
}

QString UIMachineSettingsSerial::pageTitle() const
{
    return QString(tr("Port %1", "serial ports")).arg(QString("&%1").arg(m_iSlot + 1));
}

bool UIMachineSettingsSerial::isUserDefined()
{
    ulong a, b;
    return !vboxGlobal().toCOMPortNumbers(mCbNumber->currentText(), a, b);
}

void UIMachineSettingsSerial::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIMachineSettingsSerial::retranslateUi(this);

    mCbNumber->setItemText(mCbNumber->count() - 1, vboxGlobal().toCOMPortName(0, 0));

    mCbMode->setItemText(4, gpConverter->toString(KPortMode_TCP));
    mCbMode->setItemText(3, gpConverter->toString(KPortMode_RawFile));
    mCbMode->setItemText(2, gpConverter->toString(KPortMode_HostDevice));
    mCbMode->setItemText(1, gpConverter->toString(KPortMode_HostPipe));
    mCbMode->setItemText(0, gpConverter->toString(KPortMode_Disconnected));
}

void UIMachineSettingsSerial::sltGbSerialToggled(bool fOn)
{
    if (fOn)
    {
        sltCbNumberActivated(mCbNumber->currentText());
        sltCbModeActivated(mCbMode->currentText());
    }

    /* Revalidate: */
    m_pParent->revalidate();
}

void UIMachineSettingsSerial::sltCbNumberActivated(const QString &strText)
{
    ulong uIRQ, uIOBase;
    bool fStd = vboxGlobal().toCOMPortNumbers(strText, uIRQ, uIOBase);

    mLeIRQ->setEnabled(!fStd);
    mLeIOPort->setEnabled(!fStd);
    if (fStd)
    {
        mLeIRQ->setText(QString::number(uIRQ));
        mLeIOPort->setText("0x" + QString::number(uIOBase, 16).toUpper());
    }

    /* Revalidate: */
    m_pParent->revalidate();
}

void UIMachineSettingsSerial::sltCbModeActivated(const QString &strText)
{
    KPortMode enmMode = gpConverter->fromString<KPortMode>(strText);
    mCbPipe->setEnabled(enmMode == KPortMode_HostPipe || enmMode == KPortMode_TCP);
    mLePath->setEnabled(enmMode != KPortMode_Disconnected);

    /* Revalidate: */
    m_pParent->revalidate();
}

void UIMachineSettingsSerial::prepareValidation()
{
    /* Prepare validation: */
    connect(mLeIRQ, SIGNAL(textChanged(const QString&)), m_pParent, SLOT(revalidate()));
    connect(mLeIOPort, SIGNAL(textChanged(const QString&)), m_pParent, SLOT(revalidate()));
    connect(mLePath, SIGNAL(textChanged(const QString&)), m_pParent, SLOT(revalidate()));
}


/*********************************************************************************************************************************
*   Class UIMachineSettingsSerialPage implementation.                                                                            *
*********************************************************************************************************************************/

UIMachineSettingsSerialPage::UIMachineSettingsSerialPage()
    : m_pTabWidget(0)
    , m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIMachineSettingsSerialPage::~UIMachineSettingsSerialPage()
{
    /* Cleanup: */
    cleanup();
}

bool UIMachineSettingsSerialPage::changed() const
{
    return m_pCache->wasChanged();
}

void UIMachineSettingsSerialPage::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old serial data: */
    UIDataSettingsMachineSerial oldSerialData;

    /* For each port: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Prepare old port data: */
        UIDataSettingsMachineSerialPort oldPortData;

        /* Check whether port is valid: */
        const CSerialPort &comPort = m_machine.GetSerialPort(iSlot);
        if (!comPort.isNull())
        {
            /* Gather old port data: */
            oldPortData.m_iSlot = iSlot;
            oldPortData.m_fPortEnabled = comPort.GetEnabled();
            oldPortData.m_uIRQ = comPort.GetIRQ();
            oldPortData.m_uIOBase = comPort.GetIOBase();
            oldPortData.m_hostMode = comPort.GetHostMode();
            oldPortData.m_fServer = comPort.GetServer();
            oldPortData.m_strPath = comPort.GetPath();
        }

        /* Cache old port data: */
        m_pCache->child(iSlot).cacheInitialData(oldPortData);
    }

    /* Cache old serial data: */
    m_pCache->cacheInitialData(oldSerialData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsSerialPage::getFromCache()
{
    /* Setup tab order: */
    AssertPtrReturnVoid(firstWidget());
    setTabOrder(firstWidget(), m_pTabWidget->focusProxy());
    QWidget *pLastFocusWidget = m_pTabWidget->focusProxy();

    /* For each port: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Get port page: */
        UIMachineSettingsSerial *pPage = qobject_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(iSlot));

        /* Load old port data from the cache: */
        pPage->loadPortData(m_pCache->child(iSlot).base());

        /* Setup tab order: */
        pLastFocusWidget = pPage->setOrderAfter(pLastFocusWidget);
    }

    /* Apply language settings: */
    retranslateUi();

    /* Polish page finally: */
    polishPage();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsSerialPage::putToCache()
{
    /* Prepare new serial data: */
    UIDataSettingsMachineSerial newSerialData;

    /* For each port: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Getting port page: */
        UIMachineSettingsSerial *pTab = qobject_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(iSlot));

        /* Prepare new port data: */
        UIDataSettingsMachineSerialPort newPortData;

        /* Gather new port data: */
        pTab->savePortData(newPortData);

        /* Cache new port data: */
        m_pCache->child(iSlot).cacheCurrentData(newPortData);
    }

    /* Cache new serial data: */
    m_pCache->cacheCurrentData(newSerialData);
}

void UIMachineSettingsSerialPage::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update serial data and failing state: */
    setFailed(!saveSerialData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

bool UIMachineSettingsSerialPage::validate(QList<UIValidationMessage> &messages)
{
    /* Pass by default: */
    bool fPass = true;

    /* Validation stuff: */
    QList<QPair<QString, QString> > ports;
    QStringList paths;

    /* Validate all the ports: */
    for (int iIndex = 0; iIndex < m_pTabWidget->count(); ++iIndex)
    {
        /* Get current tab/page: */
        QWidget *pTab = m_pTabWidget->widget(iIndex);
        UIMachineSettingsSerial *pPage = static_cast<UIMachineSettingsSerial*>(pTab);
        if (!pPage->mGbSerial->isChecked())
            continue;

        /* Prepare message: */
        UIValidationMessage message;
        message.first = vboxGlobal().removeAccelMark(m_pTabWidget->tabText(m_pTabWidget->indexOf(pTab)));

        /* Check the port attribute emptiness & uniqueness: */
        const QString strIRQ(pPage->mLeIRQ->text());
        const QString strIOPort(pPage->mLeIOPort->text());
        const QPair<QString, QString> pair(strIRQ, strIOPort);

        if (strIRQ.isEmpty())
        {
            message.second << UIMachineSettingsSerial::tr("No IRQ is currently specified.");
            fPass = false;
        }
        if (strIOPort.isEmpty())
        {
            message.second << UIMachineSettingsSerial::tr("No I/O port is currently specified.");
            fPass = false;
        }
        if (ports.contains(pair))
        {
            message.second << UIMachineSettingsSerial::tr("Two or more ports have the same settings.");
            fPass = false;
        }

        ports << pair;

        const KPortMode enmMode = gpConverter->fromString<KPortMode>(pPage->mCbMode->currentText());
        if (enmMode != KPortMode_Disconnected)
        {
            const QString strPath(pPage->mLePath->text());

            if (strPath.isEmpty())
            {
                message.second << UIMachineSettingsSerial::tr("No port path is currently specified.");
                fPass = false;
            }
            if (paths.contains(strPath))
            {
                message.second << UIMachineSettingsSerial::tr("There are currently duplicate port paths specified.");
                fPass = false;
            }

            paths << strPath;
        }

        /* Serialize message: */
        if (!message.second.isEmpty())
            messages << message;
    }

    /* Return result: */
    return fPass;
}

void UIMachineSettingsSerialPage::retranslateUi()
{
    for (int i = 0; i < m_pTabWidget->count(); ++i)
    {
        UIMachineSettingsSerial *pPage =
            static_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(i));
        m_pTabWidget->setTabText(i, pPage->pageTitle());
    }
}

void UIMachineSettingsSerialPage::polishPage()
{
    /* Get the count of serial port tabs: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        m_pTabWidget->setTabEnabled(iSlot,
                                    isMachineOffline() ||
                                    (isMachineInValidMode() &&
                                     m_pCache->childCount() > iSlot &&
                                     m_pCache->child(iSlot).base().m_fPortEnabled));
        UIMachineSettingsSerial *pTab = qobject_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(iSlot));
        pTab->polishTab();
    }
}

void UIMachineSettingsSerialPage::prepare()
{
    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineSerial;
    AssertPtrReturnVoid(m_pCache);

    /* Create main layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    AssertPtrReturnVoid(pMainLayout);
    {
        /* Creating tab-widget: */
        m_pTabWidget = new QITabWidget;
        AssertPtrReturnVoid(m_pTabWidget);
        {
            /* How many ports to display: */
            const ulong uCount = vboxGlobal().virtualBox().GetSystemProperties().GetSerialPortCount();

            /* Create corresponding port tabs: */
            for (ulong uPort = 0; uPort < uCount; ++uPort)
            {
                /* Create port tab: */
                UIMachineSettingsSerial *pTab = new UIMachineSettingsSerial(this);
                AssertPtrReturnVoid(pTab);
                {
                    /* Add tab into tab-widget: */
                    m_pTabWidget->addTab(pTab, pTab->pageTitle());
                }
            }

            /* Add tab-widget into layout: */
            pMainLayout->addWidget(m_pTabWidget);
        }
    }
}

void UIMachineSettingsSerialPage::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

bool UIMachineSettingsSerialPage::saveSerialData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save serial settings from the cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* For each port: */
        for (int iSlot = 0; fSuccess && iSlot < m_pTabWidget->count(); ++iSlot)
            fSuccess = savePortData(iSlot);
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsSerialPage::savePortData(int iSlot)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save adapter settings from the cache: */
    if (fSuccess && m_pCache->child(iSlot).wasChanged())
    {
        /* Get old serial data from the cache: */
        const UIDataSettingsMachineSerialPort &oldPortData = m_pCache->child(iSlot).base();
        /* Get new serial data from the cache: */
        const UIDataSettingsMachineSerialPort &newPortData = m_pCache->child(iSlot).data();

        /* Get serial port for further activities: */
        CSerialPort comPort = m_machine.GetSerialPort(iSlot);
        fSuccess = m_machine.isOk() && comPort.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            // This *must* be first.
            // If the requested host mode is changed to disconnected we should do it first.
            // That allows to automatically fulfill the requirements for some of the settings below.
            /* Save port host mode: */
            if (   fSuccess && isMachineOffline()
                && newPortData.m_hostMode != oldPortData.m_hostMode
                && newPortData.m_hostMode == KPortMode_Disconnected)
            {
                comPort.SetHostMode(newPortData.m_hostMode);
                fSuccess = comPort.isOk();
            }
            /* Save whether the port is enabled: */
            if (fSuccess && isMachineOffline() && newPortData.m_fPortEnabled != oldPortData.m_fPortEnabled)
            {
                comPort.SetEnabled(newPortData.m_fPortEnabled);
                fSuccess = comPort.isOk();
            }
            /* Save port IRQ: */
            if (fSuccess && isMachineOffline() && newPortData.m_uIRQ != oldPortData.m_uIRQ)
            {
                comPort.SetIRQ(newPortData.m_uIRQ);
                fSuccess = comPort.isOk();
            }
            /* Save port IO base: */
            if (fSuccess && isMachineOffline() && newPortData.m_uIOBase != oldPortData.m_uIOBase)
            {
                comPort.SetIOBase(newPortData.m_uIOBase);
                fSuccess = comPort.isOk();
            }
            /* Save whether the port is server: */
            if (fSuccess && isMachineOffline() && newPortData.m_fServer != oldPortData.m_fServer)
            {
                comPort.SetServer(newPortData.m_fServer);
                fSuccess = comPort.isOk();
            }
            /* Save port path: */
            if (fSuccess && isMachineOffline() && newPortData.m_strPath != oldPortData.m_strPath)
            {
                comPort.SetPath(newPortData.m_strPath);
                fSuccess = comPort.isOk();
            }
            // This *must* be last.
            // The host mode will be changed to disconnected if some of the necessary
            // settings above will not meet the requirements for the selected mode.
            /* Save port host mode: */
            if (   fSuccess && isMachineOffline()
                && newPortData.m_hostMode != oldPortData.m_hostMode
                && newPortData.m_hostMode != KPortMode_Disconnected)
            {
                comPort.SetHostMode(newPortData.m_hostMode);
                fSuccess = comPort.isOk();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comPort));
        }
    }
    /* Return result: */
    return fSuccess;
}

# include "UIMachineSettingsSerial.moc"

