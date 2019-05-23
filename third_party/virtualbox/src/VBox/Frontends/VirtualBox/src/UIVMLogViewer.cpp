/* $Id: UIVMLogViewer.cpp $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class implementation.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
# include <QCheckBox>
# include <QComboBox>
# include <QDateTime>
# include <QDir>
# include <QFileDialog>
# if defined(RT_OS_SOLARIS)
#  include <QFontDatabase>
# endif
# include <QKeyEvent>
# include <QLabel>
# include <QScrollBar>
# include <QTextEdit>

/* GUI includes: */
# include "QIFileDialog.h"
# include "QITabWidget.h"
# include "UIExtraDataManager.h"
# include "UIIconPool.h"
# include "UIMessageCenter.h"
# include "UISpecialControls.h"
# include "UIVMLogViewer.h"
# include "UIDesktopWidgetWatchdog.h"
# include "VBoxGlobal.h"
# include "VBoxUtils.h"

/* COM includes: */
# include "COMEnums.h"
# include "CSystemProperties.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** QWidget extension
  * providing GUI for search-panel in VM Log-Viewer. */
class UIVMLogViewerSearchPanel : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs search-panel by passing @a pParent to the QWidget base-class constructor.
      * @param  pViewer  Specifies instance of VM Log-Viewer. */
    UIVMLogViewerSearchPanel(QWidget *pParent, UIVMLogViewer *pViewer)
        : QIWithRetranslateUI<QWidget>(pParent)
        , m_pViewer(pViewer)
        , m_pMainLayout(0)
        , m_pCloseButton(0)
        , m_pSearchLabel(0), m_pSearchEditor(0)
        , m_pNextPrevButtons(0)
        , m_pCaseSensitiveCheckBox(0)
        , m_pWarningSpacer(0), m_pWarningIcon(0), m_pWarningLabel(0)
    {
        /* Prepare: */
        prepare();
    }

private slots:

    /** Handles find next/back action triggering.
      * @param  iButton  Specifies id of next/back button. */
    void find(int iButton)
    {
        /* If button-id is 1, findNext: */
        if (iButton)
            findNext();
        /* If button-id is 0, findBack: */
        else
            findBack();
    }

    /** Handles textchanged signal from search-editor.
      * @param  strSearchString  Specifies search-string. */
    void findCurrent(const QString &strSearchString)
    {
        /* Enable/disable Next-Previous buttons as per search-string validity: */
        m_pNextPrevButtons->setEnabled(0, strSearchString.length());
        m_pNextPrevButtons->setEnabled(1, strSearchString.length());
        /* Show/hide the warning as per search-string validity: */
        toggleWarning(!strSearchString.length());
        /* If search-string is valid: */
        if (strSearchString.length())
            search(true, true);
        /* If search-string is not valid, reset cursor position: */
        else
        {
            /* Get current log-page: */
            QTextEdit *pBrowser = m_pViewer->currentLogPage();
            /* If current log-page is valid and cursor has selection: */
            if (pBrowser && pBrowser->textCursor().hasSelection())
            {
                /* Get cursor and reset position: */
                QTextCursor cursor = pBrowser->textCursor();
                cursor.setPosition(cursor.anchor());
                pBrowser->setTextCursor(cursor);
            }
        }
    }

private:

    /** Prepares search-panel. */
    void prepare()
    {
        /* Prepare widgets: */
        prepareWidgets();

        /* Prepare connections: */
        prepareConnections();

        /* Retranslate finally: */
        retranslateUi();
    }

    /** Prepares widgets. */
    void prepareWidgets()
    {
        /* Create main-layout: */
        m_pMainLayout = new QHBoxLayout(this);
        AssertPtrReturnVoid(m_pMainLayout);
        {
            /* Configure main-layout: */
            m_pMainLayout->setContentsMargins(0, 0, 0, 0);

            /* Create close-button: */
            m_pCloseButton = new UIMiniCancelButton;
            AssertPtrReturnVoid(m_pCloseButton);
            {
                /* Add close-button to main-layout: */
                m_pMainLayout->addWidget(m_pCloseButton);
            }

            /* Create search-editor: */
            m_pSearchEditor = new UISearchField(this);
            AssertPtrReturnVoid(m_pSearchEditor);
            {
                /* Configure search-editor: */
                m_pSearchEditor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                /* Add search-editor to main-layout: */
                m_pMainLayout->addWidget(m_pSearchEditor);
            }

            /* Create search-label: */
            m_pSearchLabel = new QLabel;
            AssertPtrReturnVoid(m_pSearchLabel);
            {
                /* Configure search-label: */
                m_pSearchLabel->setBuddy(m_pSearchEditor);
                /* Prepare font: */
#ifdef VBOX_DARWIN_USE_NATIVE_CONTROLS
                QFont font = m_pSearchLabel->font();
                font.setPointSize(::darwinSmallFontSize());
                m_pSearchLabel->setFont(font);
#endif /* VBOX_DARWIN_USE_NATIVE_CONTROLS */
                /* Add search-label to main-layout: */
                m_pMainLayout->addWidget(m_pSearchLabel);
            }

            /* Create Next/Prev button-box: */
            m_pNextPrevButtons = new UIRoundRectSegmentedButton(this, 2);
            AssertPtrReturnVoid(m_pNextPrevButtons);
            {
                /* Configure Next/Prev button-box: */
                m_pNextPrevButtons->setEnabled(0, false);
                m_pNextPrevButtons->setEnabled(1, false);
#ifndef VBOX_WS_MAC
                /* No icons on the Mac: */
                m_pNextPrevButtons->setIcon(0, UIIconPool::defaultIcon(UIIconPool::UIDefaultIconType_ArrowBack, this));
                m_pNextPrevButtons->setIcon(1, UIIconPool::defaultIcon(UIIconPool::UIDefaultIconType_ArrowForward, this));
#endif /* !VBOX_WS_MAC */
                /* Add Next/Prev button-box to main-layout: */
                m_pMainLayout->addWidget(m_pNextPrevButtons);
            }

            /* Create case-sensitive checkbox: */
            m_pCaseSensitiveCheckBox = new QCheckBox;
            AssertPtrReturnVoid(m_pCaseSensitiveCheckBox);
            {
                /* Configure focus for case-sensitive checkbox: */
                setFocusProxy(m_pCaseSensitiveCheckBox);
                /* Configure font: */
#ifdef VBOX_DARWIN_USE_NATIVE_CONTROLS
                QFont font = m_pCaseSensitiveCheckBox->font();
                font.setPointSize(::darwinSmallFontSize());
                m_pCaseSensitiveCheckBox->setFont(font);
#endif /* VBOX_DARWIN_USE_NATIVE_CONTROLS */
                /* Add case-sensitive checkbox to main-layout: */
                m_pMainLayout->addWidget(m_pCaseSensitiveCheckBox);
            }

            /* Create warning-spacer: */
            m_pWarningSpacer = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
            AssertPtrReturnVoid(m_pWarningSpacer);
            {
                /* Add warning-spacer to main-layout: */
                m_pMainLayout->addItem(m_pWarningSpacer);
            }

            /* Create warning-icon: */
            m_pWarningIcon = new QLabel;
            AssertPtrReturnVoid(m_pWarningIcon);
            {
                /* Confifure warning-icon: */
                m_pWarningIcon->hide();
                QIcon icon = UIIconPool::defaultIcon(UIIconPool::UIDefaultIconType_MessageBoxWarning, this);
                if (!icon.isNull())
                    m_pWarningIcon->setPixmap(icon.pixmap(16, 16));
                /* Add warning-icon to main-layout: */
                m_pMainLayout->addWidget(m_pWarningIcon);
            }

            /* Create warning-label: */
            m_pWarningLabel = new QLabel;
            AssertPtrReturnVoid(m_pWarningLabel);
            {
                /* Configure warning-label: */
                m_pWarningLabel->hide();
                /* Prepare font: */
#ifdef VBOX_DARWIN_USE_NATIVE_CONTROLS
                QFont font = m_pWarningLabel->font();
                font.setPointSize(::darwinSmallFontSize());
                m_pWarningLabel->setFont(font);
#endif /* VBOX_DARWIN_USE_NATIVE_CONTROLS */
                /* Add warning-label to main-layout: */
                m_pMainLayout->addWidget(m_pWarningLabel);
            }

            /* Create spacer-item: */
            m_pSpacerItem = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            AssertPtrReturnVoid(m_pSpacerItem);
            {
                /* Add spacer-item to main-layout: */
                m_pMainLayout->addItem(m_pSpacerItem);
            }
        }
    }

    /** Prepares connections. */
    void prepareConnections()
    {
        /* Prepare connections: */
        connect(m_pCloseButton, SIGNAL(clicked()), this, SLOT(hide()));
        connect(m_pSearchEditor, SIGNAL(textChanged(const QString &)),
                this, SLOT(findCurrent(const QString &)));
        connect(m_pNextPrevButtons, SIGNAL(clicked(int)), this, SLOT(find(int)));
    }

    /** Handles translation event. */
    void retranslateUi()
    {
        m_pCloseButton->setToolTip(UIVMLogViewer::tr("Close the search panel"));

        m_pSearchLabel->setText(QString("%1 ").arg(UIVMLogViewer::tr("&Find")));
        m_pSearchEditor->setToolTip(UIVMLogViewer::tr("Enter a search string here"));

        m_pNextPrevButtons->setTitle(0, UIVMLogViewer::tr("&Previous"));
        m_pNextPrevButtons->setToolTip(0, UIVMLogViewer::tr("Search for the previous occurrence of the string"));
        m_pNextPrevButtons->setTitle(1, UIVMLogViewer::tr("&Next"));
        m_pNextPrevButtons->setToolTip(1, UIVMLogViewer::tr("Search for the next occurrence of the string"));

        m_pCaseSensitiveCheckBox->setText(UIVMLogViewer::tr("C&ase Sensitive"));
        m_pCaseSensitiveCheckBox->setToolTip(UIVMLogViewer::tr("Perform case sensitive search (when checked)"));

        m_pWarningLabel->setText(UIVMLogViewer::tr("String not found"));
    }

    /** Handles Qt key-press @a pEevent. */
    void keyPressEvent(QKeyEvent *pEvent)
    {
        switch (pEvent->key())
        {
            /* Process Enter press as 'search-next',
             * performed for any search panel widget: */
            case Qt::Key_Enter:
            case Qt::Key_Return:
            {
                if (pEvent->modifiers() == 0 ||
                    pEvent->modifiers() & Qt::KeypadModifier)
                {
                    /* Animate click on 'Next' button: */
                    m_pNextPrevButtons->animateClick(1);
                    return;
                }
                break;
            }
            default:
                break;
        }
        /* Call to base-class: */
        QWidget::keyPressEvent(pEvent);
    }

    /** Handles Qt @a pEvent, used for keyboard processing. */
    bool eventFilter(QObject *pObject, QEvent *pEvent)
    {
        /* Depending on event-type: */
        switch (pEvent->type())
        {
            /* Process key press only: */
            case QEvent::KeyPress:
            {
                /* Cast to corresponding key press event: */
                QKeyEvent *pKeyEvent = static_cast<QKeyEvent*>(pEvent);

                /* Handle F3/Shift+F3 as search next/previous shortcuts: */
                if (pKeyEvent->key() == Qt::Key_F3)
                {
                    /* If there is no modifier 'Key-F3' is pressed: */
                    if (pKeyEvent->QInputEvent::modifiers() == 0)
                    {
                        /* Animate click on 'Next' button: */
                        m_pNextPrevButtons->animateClick(1);
                        return true;
                    }
                    /* If there is 'ShiftModifier' 'Shift + Key-F3' is pressed: */
                    else if (pKeyEvent->QInputEvent::modifiers() == Qt::ShiftModifier)
                    {
                        /* Animate click on 'Prev' button: */
                        m_pNextPrevButtons->animateClick(0);
                        return true;
                    }
                }
                /* Handle Ctrl+F key combination as a shortcut to focus search field: */
                else if (pKeyEvent->QInputEvent::modifiers() == Qt::ControlModifier &&
                         pKeyEvent->key() == Qt::Key_F)
                {
                    if (m_pViewer->currentLogPage())
                    {
                        /* Make sure current log-page is visible: */
                        if (isHidden())
                            show();
                        /* Set focus on search-editor: */
                        m_pSearchEditor->setFocus();
                        return true;
                    }
                }
                /* Handle alpha-numeric keys to implement the "find as you type" feature: */
                else if ((pKeyEvent->QInputEvent::modifiers() & ~Qt::ShiftModifier) == 0 &&
                         pKeyEvent->key() >= Qt::Key_Exclam && pKeyEvent->key() <= Qt::Key_AsciiTilde)
                {
                    /* Make sure current log-page is visible: */
                    if (m_pViewer->currentLogPage())
                    {
                        if (isHidden())
                            show();
                        /* Set focus on search-editor: */
                        m_pSearchEditor->setFocus();
                        /* Insert the text to search-editor, which triggers the search-operation for new text: */
                        m_pSearchEditor->insert(pKeyEvent->text());
                        return true;
                    }
                }
                break;
            }
            default:
                break;
        }
        /* Call to base-class: */
        return QWidget::eventFilter(pObject, pEvent);
    }

    /** Handles Qt show @a pEvent. */
    void showEvent(QShowEvent *pEvent)
    {
        /* Call to base-class: */
        QWidget::showEvent(pEvent);
        /* Set focus on search-editor: */
        m_pSearchEditor->setFocus();
        /* Select all the text: */
        m_pSearchEditor->selectAll();
    }

    /** Handles Qt hide @a pEvent. */
    void hideEvent(QHideEvent *pEvent)
    {
        /* Get focus-widget: */
        QWidget *pFocus = QApplication::focusWidget();
        /* If focus-widget is valid and child-widget of search-panel,
         * focus next child-widget in line: */
        if (pFocus && pFocus->parent() == this)
            focusNextPrevChild(true);
        /* Call to base-class: */
        QWidget::hideEvent(pEvent);
    }

    /** Search routine.
      * @param  fForward       Specifies the direction of search.
      * @param  fStartCurrent  Specifies whether search should start from beginning of the log. */
    void search(bool fForward, bool fStartCurrent = false)
    {
        /* Get current log-page: */
        QTextEdit *pBrowser = m_pViewer->currentLogPage();
        if (!pBrowser) return;

        /* Get text-cursor and its attributes: */
        QTextCursor cursor = pBrowser->textCursor();
        int iPos = cursor.position();
        int iAnc = cursor.anchor();

        /* Get the text to be searched: */
        QString strText = pBrowser->toPlainText();
        int iDiff = fStartCurrent ? 0 : 1;

        int iResult = -1;
        /* If search-direction is forward and cursor position is valid: */
        if (fForward && (fStartCurrent || iPos < strText.size() - 1))
        {
            /* Search and get the index of first match: */
            iResult = strText.indexOf(m_pSearchEditor->text(), iAnc + iDiff,
                                      m_pCaseSensitiveCheckBox->isChecked() ?
                                      Qt::CaseSensitive : Qt::CaseInsensitive);

            /* When searchstring is changed, search from beginning of log again: */
            if (iResult == -1 && fStartCurrent)
                iResult = strText.indexOf(m_pSearchEditor->text(), 0,
                                          m_pCaseSensitiveCheckBox->isChecked() ?
                                          Qt::CaseSensitive : Qt::CaseInsensitive);
        }
        /* If search-direction is backward: */
        else if (!fForward && iAnc > 0)
            iResult = strText.lastIndexOf(m_pSearchEditor->text(), iAnc - 1,
                                          m_pCaseSensitiveCheckBox->isChecked() ?
                                          Qt::CaseSensitive : Qt::CaseInsensitive);

        /* If the result is valid, move cursor-position: */
        if (iResult != -1)
        {
            cursor.movePosition(QTextCursor::Start,
                                QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::NextCharacter,
                                QTextCursor::MoveAnchor, iResult);
            cursor.movePosition(QTextCursor::NextCharacter,
                                QTextCursor::KeepAnchor,
                                m_pSearchEditor->text().size());
            pBrowser->setTextCursor(cursor);
        }

        /* Show/hide the warning as per search-result: */
        toggleWarning(iResult != -1);
    }

    /** Forward search routine wrapper. */
    void findNext() { search(true); }

    /** Backward search routine wrapper. */
    void findBack() { search(false); }

    /** Shows/hides the search border warning using @a fHide as hint. */
    void toggleWarning(bool fHide)
    {
        /* Adjust size of warning-spacer accordingly: */
        m_pWarningSpacer->changeSize(fHide ? 0 : 16, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        /* If visible mark the error for search-editor (changes text-color): */
        if (!fHide)
            m_pSearchEditor->markError();
        /* If hidden unmark the error for search-editor (changes text-color): */
        else
            m_pSearchEditor->unmarkError();
        /* Show/hide the warning-icon: */
        m_pWarningIcon->setHidden(fHide);
        /* Show/hide the warning-label: */
        m_pWarningLabel->setHidden(fHide);
    }

    /** Holds the reference to the VM Log-Viewer this search-panel belongs to. */
    UIVMLogViewer *m_pViewer;
    /** Holds the instance of main-layout we create. */
    QHBoxLayout *m_pMainLayout;
    /** Holds the instance of close-button we create. */
    UIMiniCancelButton *m_pCloseButton;
    /** Holds the instance of search-label we create. */
    QLabel *m_pSearchLabel;
    /** Holds the instance of search-editor we create. */
    UISearchField *m_pSearchEditor;
    /** Holds the instance of next/back button-box we create. */
    UIRoundRectSegmentedButton *m_pNextPrevButtons;
    /** Holds the instance of case-sensitive checkbox we create. */
    QCheckBox *m_pCaseSensitiveCheckBox;
    /** Holds the instance of warning spacer-item we create. */
    QSpacerItem *m_pWarningSpacer;
    /** Holds the instance of warning icon we create. */
    QLabel *m_pWarningIcon;
    /** Holds the instance of warning label we create. */
    QLabel *m_pWarningLabel;
    /** Holds the instance of spacer item we create. */
    QSpacerItem *m_pSpacerItem;
};


/** QWidget extension
  * providing GUI for filter panel in VM Log Viewer. */
class UIVMLogViewerFilterPanel : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs the filter-panel by passing @a pParent to the QWidget base-class constructor.
      * @param  pViewer  Specifies reference to the VM Log-Viewer this filter-panel belongs to. */
    UIVMLogViewerFilterPanel(QWidget *pParent, UIVMLogViewer *pViewer)
        : QIWithRetranslateUI<QWidget>(pParent)
        , m_pViewer(pViewer)
        , m_pMainLayout(0)
        , m_pCloseButton(0)
        , m_pFilterLabel(0), m_pFilterComboBox(0)
    {
        /* Prepare: */
        prepare();
    }

public slots:

    /** Applies filter settings and filters the current log-page.
      * @param  iCurrentIndex  Specifies index of current log-page, but it is actually not used in the method. */
    void applyFilter(const int iCurrentIndex = 0)
    {
        Q_UNUSED(iCurrentIndex);
        QTextEdit *pCurrentPage = m_pViewer->currentLogPage();
        AssertReturnVoid(pCurrentPage);
        QString strInputText = m_pViewer->currentLog();
        if (!strInputText.isNull())
        {
            /* Prepare filter-data: */
            QString strFilteredText;
            const QRegExp rxFilterExp(m_strFilterText, Qt::CaseInsensitive);

            /* If filter regular-expression is not empty and valid, filter the log: */
            if (!rxFilterExp.isEmpty() && rxFilterExp.isValid())
            {
                while (!strInputText.isEmpty())
                {
                    /* Read each line and check if it matches regular-expression: */
                    const int index = strInputText.indexOf('\n');
                    if (index > 0)
                    {
                        QString strLine = strInputText.left(index + 1);
                        if (strLine.contains(rxFilterExp))
                            strFilteredText.append(strLine);
                    }
                    strInputText.remove(0, index + 1);
                }
                pCurrentPage->setPlainText(strFilteredText);
            }
            /* Restore entire log when filter regular expression is empty or not valid: */
            else
                pCurrentPage->setPlainText(strInputText);

            /* Move the cursor position to end: */
            QTextCursor cursor = pCurrentPage->textCursor();
            cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
            pCurrentPage->setTextCursor(cursor);
        }
    }

private slots:

    /** Handles the textchanged event from filter editor. */
    void filter(const QString &strSearchString)
    {
        m_strFilterText = strSearchString;
        applyFilter();
    }

private:

    /** Prepares filter-panel. */
    void prepare()
    {
        /* Prepare widgets: */
        prepareWidgets();

        /* Prepare connections: */
        prepareConnections();

        /* Retranslate finally: */
        retranslateUi();
    }

    /** Prepares widgets. */
    void prepareWidgets()
    {
        /* Create main-layout: */
        m_pMainLayout = new QHBoxLayout(this);
        AssertPtrReturnVoid(m_pMainLayout);
        {
            /* Configure main-layout: */
            m_pMainLayout->setContentsMargins(0, 0, 0, 0);

            /* Create close-button: */
            m_pCloseButton = new UIMiniCancelButton;
            AssertPtrReturnVoid(m_pCloseButton);
            {
                /* Add close-button to main-layout: */
                m_pMainLayout->addWidget(m_pCloseButton);
            }

            /* Create filter-combobox: */
            m_pFilterComboBox = new QComboBox;
            AssertPtrReturnVoid(m_pFilterComboBox);
            {
                /* Configure filter-combobox: */
                m_pFilterComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                m_pFilterComboBox->setEditable(true);
                QStringList strFilterPresets;
                strFilterPresets << "" << "GUI" << "NAT" << "AHCI" << "VD" << "Audio" << "VUSB" << "SUP" << "PGM" << "HDA"
                                 << "HM" << "VMM" << "GIM" << "CPUM";
                strFilterPresets.sort();
                m_pFilterComboBox->addItems(strFilterPresets);
                /* Add filter-combobox to main-layout: */
                m_pMainLayout->addWidget(m_pFilterComboBox);
            }

            /* Create filter-label: */
            m_pFilterLabel = new QLabel;
            AssertPtrReturnVoid(m_pFilterLabel);
            {
                /* Configure filter-label: */
                m_pFilterLabel->setBuddy(m_pFilterComboBox);
#ifdef VBOX_DARWIN_USE_NATIVE_CONTROLS
                QFont font = m_pFilterLabel->font();
                font.setPointSize(::darwinSmallFontSize());
                m_pFilterLabel->setFont(font);
#endif /* VBOX_DARWIN_USE_NATIVE_CONTROLS */
                /* Add filter-label to main-layout: */
                m_pMainLayout->addWidget(m_pFilterLabel);
            }
        }
    }

    /** Prepares connections. */
    void prepareConnections()
    {
        /* Prepare connections: */
        connect(m_pCloseButton, SIGNAL(clicked()), this, SLOT(hide()));
        connect(m_pFilterComboBox, SIGNAL(editTextChanged(const QString &)),
                this, SLOT(filter(const QString &)));
    }

    /** Handles the translation event. */
    void retranslateUi()
    {
        m_pCloseButton->setToolTip(UIVMLogViewer::tr("Close the search panel"));
        m_pFilterLabel->setText(UIVMLogViewer::tr("Filter"));
        m_pFilterComboBox->setToolTip(UIVMLogViewer::tr("Enter filtering string here"));
    }

    /** Handles Qt @a pEvent, used for keyboard processing. */
    bool eventFilter(QObject *pObject, QEvent *pEvent)
    {
        /* Depending on event-type: */
        switch (pEvent->type())
        {
            /* Process key press only: */
            case QEvent::KeyPress:
            {
                /* Cast to corresponding key press event: */
                QKeyEvent *pKeyEvent = static_cast<QKeyEvent*>(pEvent);

                /* Handle Ctrl+T key combination as a shortcut to focus search field: */
                if (pKeyEvent->QInputEvent::modifiers() == Qt::ControlModifier &&
                         pKeyEvent->key() == Qt::Key_T)
                {
                    if (m_pViewer->currentLogPage())
                    {
                        if (isHidden())
                            show();
                        m_pFilterComboBox->setFocus();
                        return true;
                    }
                }
                break;
            }
            default:
                break;
        }
        /* Call to base-class: */
        return QWidget::eventFilter(pObject, pEvent);
    }

    /** Handles the Qt show @a pEvent. */
    void showEvent(QShowEvent *pEvent)
    {
        /* Call to base-class: */
        QWidget::showEvent(pEvent);
        /* Set focus to combo-box: */
        m_pFilterComboBox->setFocus();
    }

    /** Handles the Qt hide @a pEvent. */
    void hideEvent(QHideEvent *pEvent)
    {
        /* Get focused widget: */
        QWidget *pFocus = QApplication::focusWidget();
        /* If focus-widget is valid and child-widget of search-panel,
         * focus next child-widget in line: */
        if (pFocus && pFocus->parent() == this)
            focusNextPrevChild(true);
        /* Call to base-class: */
        QWidget::hideEvent(pEvent);
    }

    /** Holds the reference to VM Log-Viewer this filter-panel belongs to. */
    UIVMLogViewer *m_pViewer;
    /** Holds the instance of main-layout we create. */
    QHBoxLayout *m_pMainLayout;
    /** Holds the instance of close-button we create. */
    UIMiniCancelButton *m_pCloseButton;
    /** Holds the instance of filter-label we create. */
    QLabel *m_pFilterLabel;
    /** Holds instance of filter combo-box we create. */
    QComboBox *m_pFilterComboBox;
    /** Holds the filter text. */
    QString m_strFilterText;
};


/*********************************************************************************************************************************
*   Class UIVMLogViewer implementation.                                                                                          *
*********************************************************************************************************************************/

/** Holds the VM Log-Viewer array. */
VMLogViewerMap UIVMLogViewer::m_viewers = VMLogViewerMap();

void UIVMLogViewer::showLogViewerFor(QWidget *pCenterWidget, const CMachine &machine)
{
    /* If there is no corresponding VM Log-Viewer created: */
    if (!m_viewers.contains(machine.GetName()))
    {
        /* Create new VM Log-Viewer: */
        UIVMLogViewer *pLogViewer = new UIVMLogViewer(pCenterWidget, Qt::Window, machine);
        AssertPtrReturnVoid(pLogViewer);
        {
            /* Configure VM Log-Viewer: */
            pLogViewer->setAttribute(Qt::WA_DeleteOnClose);
            m_viewers[machine.GetName()] = pLogViewer;
        }
    }

    /* Show VM Log Viewer: */
    UIVMLogViewer *pViewer = m_viewers[machine.GetName()];
    pViewer->show();
    pViewer->raise();
    pViewer->setWindowState(pViewer->windowState() & ~Qt::WindowMinimized);
    pViewer->activateWindow();
}

UIVMLogViewer::UIVMLogViewer(QWidget *pParent, Qt::WindowFlags flags, const CMachine &machine)
    : QIWithRetranslateUI2<QIMainWindow>(pParent, flags)
    , m_fIsPolished(false)
    , m_machine(machine)
{
    /* Prepare VM Log-Viewer: */
    prepare();
}

UIVMLogViewer::~UIVMLogViewer()
{
    /* Cleanup VM Log-Viewer: */
    cleanup();
}

bool UIVMLogViewer::shouldBeMaximized() const
{
    return gEDataManager->logWindowShouldBeMaximized();
}

void UIVMLogViewer::search()
{
    /* Show/hide search-panel: */
    m_pSearchPanel->isHidden() ? m_pSearchPanel->show() : m_pSearchPanel->hide();
}

void UIVMLogViewer::refresh()
{
    /* Disconnect this connection to avoid initial signals during page creation/deletion: */
    disconnect(m_pViewerContainer, SIGNAL(currentChanged(int)), m_pFilterPanel, SLOT(applyFilter(int)));

    /* Clearing old data if any: */
    m_book.clear();
    m_logMap.clear();
    m_pViewerContainer->setEnabled(true);
    while (m_pViewerContainer->count())
    {
        QWidget *pFirstPage = m_pViewerContainer->widget(0);
        m_pViewerContainer->removeTab(0);
        delete pFirstPage;
    }

    bool isAnyLogPresent = false;

    const CSystemProperties &sys = vboxGlobal().virtualBox().GetSystemProperties();
    unsigned cMaxLogs = sys.GetLogHistoryCount() + 1 /*VBox.log*/ + 1 /*VBoxHardening.log*/; /** @todo Add api for getting total possible log count! */
    for (unsigned i = 0; i < cMaxLogs; ++i)
    {
        /* Query the log file name for index i: */
        QString strFileName = m_machine.QueryLogFilename(i);
        if (!strFileName.isEmpty())
        {
            /* Try to read the log file with the index i: */
            ULONG uOffset = 0;
            QString strText;
            while (true)
            {
                QVector<BYTE> data = m_machine.ReadLog(i, uOffset, _1M);
                if (data.size() == 0)
                    break;
                strText.append(QString::fromUtf8((char*)data.data(), data.size()));
                uOffset += data.size();
            }
            /* Anything read at all? */
            if (uOffset > 0)
            {
                /* Create a log viewer page and append the read text to it: */
                QTextEdit *pLogViewer = createLogPage(QFileInfo(strFileName).fileName());
                pLogViewer->setPlainText(strText);
                /* Move the cursor position to end: */
                QTextCursor cursor = pLogViewer->textCursor();
                cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
                pLogViewer->setTextCursor(cursor);
                /* Add the actual file name and the QTextEdit containing the content to a list: */
                m_book << qMakePair(strFileName, pLogViewer);
                /* Add the log-text to the map: */
                m_logMap[pLogViewer] = strText;
                isAnyLogPresent = true;
            }
        }
    }

    /* Create an empty log page if there are no logs at all: */
    if (!isAnyLogPresent)
    {
        QTextEdit *pDummyLog = createLogPage("VBox.log");
        pDummyLog->setWordWrapMode(QTextOption::WordWrap);
        pDummyLog->setHtml(tr("<p>No log files found. Press the "
                              "<b>Refresh</b> button to rescan the log folder "
                              "<nobr><b>%1</b></nobr>.</p>")
                              .arg(m_machine.GetLogFolder()));
        /* We don't want it to remain white: */
        QPalette pal = pDummyLog->palette();
        pal.setColor(QPalette::Base, pal.color(QPalette::Window));
        pDummyLog->setPalette(pal);
    }

    /* Show the first tab widget's page after the refresh: */
    m_pViewerContainer->setCurrentIndex(0);

    /* Apply the filter settings: */
    m_pFilterPanel->applyFilter();

    /* Setup this connection after refresh to avoid initial signals during page creation: */
    connect(m_pViewerContainer, SIGNAL(currentChanged(int)), m_pFilterPanel, SLOT(applyFilter(int)));

    /* Enable/Disable save button & tab widget according log presence: */
    m_pButtonFind->setEnabled(isAnyLogPresent);
    m_pButtonSave->setEnabled(isAnyLogPresent);
    m_pButtonFilter->setEnabled(isAnyLogPresent);
    m_pViewerContainer->setEnabled(isAnyLogPresent);
}

bool UIVMLogViewer::close()
{
    /* Close search-panel: */
    m_pSearchPanel->hide();
    /* Close filter-panel: */
    m_pFilterPanel->hide();
    /* Call to base-class: */
    return QIMainWindow::close();
}

void UIVMLogViewer::save()
{
    /* Prepare "save as" dialog: */
    const QFileInfo fileInfo(m_book.at(m_pViewerContainer->currentIndex()).first);
    /* Prepare default filename: */
    const QDateTime dtInfo = fileInfo.lastModified();
    const QString strDtString = dtInfo.toString("yyyy-MM-dd-hh-mm-ss");
    const QString strDefaultFileName = QString("%1-%2.log").arg(m_machine.GetName()).arg(strDtString);
    const QString strDefaultFullName = QDir::toNativeSeparators(QDir::home().absolutePath() + "/" + strDefaultFileName);
    /* Show "save as" dialog: */
    const QString strNewFileName = QIFileDialog::getSaveFileName(strDefaultFullName,
                                                                 "",
                                                                 this,
                                                                 tr("Save VirtualBox Log As"),
                                                                 0 /* selected filter */,
                                                                 true /* resolve symlinks */,
                                                                 true /* confirm overwrite */);
    /* Make sure file-name is not empty: */
    if (!strNewFileName.isEmpty())
    {
        /* Delete the previous file if already exists as user already confirmed: */
        if (QFile::exists(strNewFileName))
            QFile::remove(strNewFileName);
        /* Copy log into the file: */
        QFile::copy(m_machine.QueryLogFilename(m_pViewerContainer->currentIndex()), strNewFileName);
    }
}

void UIVMLogViewer::filter()
{
    /* Show/hide filter-panel: */
    m_pFilterPanel->isHidden() ? m_pFilterPanel->show() : m_pFilterPanel->hide();
}

void UIVMLogViewer::prepare()
{
    /* Apply UI decorations: */
    Ui::UIVMLogViewer::setupUi(this);

    /* Apply window icons: */
    setWindowIcon(UIIconPool::iconSetFull(":/vm_show_logs_32px.png", ":/vm_show_logs_16px.png"));

    /* Prepare widgets: */
    prepareWidgets();

    /* Prepare connections: */
    prepareConnections();

    /* Reading log files: */
    refresh();

    /* Load settings: */
    loadSettings();

    /* Loading language constants: */
    retranslateUi();
}

void UIVMLogViewer::prepareWidgets()
{
    /* Create VM Log-Viewer container: */
    m_pViewerContainer = new QITabWidget(centralWidget());
    AssertPtrReturnVoid(m_pViewerContainer);
    {
        /* Add VM Log-Viewer container to main-layout: */
        m_pMainLayout->insertWidget(0, m_pViewerContainer);
    }

    /* Create VM Log-Viewer search-panel: */
    m_pSearchPanel = new UIVMLogViewerSearchPanel(centralWidget(), this);
    AssertPtrReturnVoid(m_pSearchPanel);
    {
        /* Configure VM Log-Viewer search-panel: */
        centralWidget()->installEventFilter(m_pSearchPanel);
        m_pSearchPanel->hide();
        /* Add VM Log-Viewer search-panel to main-layout: */
        m_pMainLayout->insertWidget(1, m_pSearchPanel);
    }

    /* Create VM Log-Viewer filter-panel: */
    m_pFilterPanel = new UIVMLogViewerFilterPanel(centralWidget(), this);
    AssertPtrReturnVoid(m_pFilterPanel);
    {
        /* Configure VM Log-Viewer filter-panel: */
        centralWidget()->installEventFilter(m_pFilterPanel);
        m_pFilterPanel->hide();
        /* Add VM Log-Viewer filter-panel to main-layout: */
        m_pMainLayout->insertWidget(2, m_pFilterPanel);
    }

    /* Get help-button: */
    m_pButtonHelp = m_pButtonBox->button(QDialogButtonBox::Help);
    AssertPtrReturnVoid(m_pButtonHelp);
    /* Create find-button: */
    m_pButtonFind = m_pButtonBox->addButton(QString::null, QDialogButtonBox::ActionRole);
    AssertPtrReturnVoid(m_pButtonFind);
    /* Create filter-button: */
    m_pButtonFilter = m_pButtonBox->addButton(QString::null, QDialogButtonBox::ActionRole);
    AssertPtrReturnVoid(m_pButtonFilter);
    /* Create close-button: */
    m_pButtonClose = m_pButtonBox->button(QDialogButtonBox::Close);
    AssertPtrReturnVoid(m_pButtonClose);
    /* Create save-button: */
    m_pButtonSave = m_pButtonBox->button(QDialogButtonBox::Save);
    AssertPtrReturnVoid(m_pButtonSave);
    /* Create refresh-button: */
    m_pButtonRefresh = m_pButtonBox->addButton(QString::null, QDialogButtonBox::ActionRole);
    AssertPtrReturnVoid(m_pButtonRefresh);
}

void UIVMLogViewer::prepareConnections()
{
    /* Prepare connections: */
    connect(m_pButtonBox, SIGNAL(helpRequested()), &msgCenter(), SLOT(sltShowHelpHelpDialog()));
    connect(m_pButtonFind, SIGNAL(clicked()), this, SLOT(search()));
    connect(m_pButtonRefresh, SIGNAL(clicked()), this, SLOT(refresh()));
    connect(m_pButtonClose, SIGNAL(clicked()), this, SLOT(close()));
    connect(m_pButtonSave, SIGNAL(clicked()), this, SLOT(save()));
    connect(m_pButtonFilter, SIGNAL(clicked()), this, SLOT(filter()));
}

void UIVMLogViewer::loadSettings()
{
    /* Restore window geometry: */
    {
        /* Getting available geometry to calculate default geometry: */
        const QRect desktopRect = gpDesktop->availableGeometry(this);
        int iDefaultWidth = desktopRect.width() / 2;
        int iDefaultHeight = desktopRect.height() * 3 / 4;

        /* Calculate default width to fit 132 characters: */
        QTextEdit *pCurrentLogPage = currentLogPage();
        if (pCurrentLogPage)
        {
            iDefaultWidth = pCurrentLogPage->fontMetrics().width(QChar('x')) * 132 +
                            pCurrentLogPage->verticalScrollBar()->width() +
                            pCurrentLogPage->frameWidth() * 2 +
                            10 * 2 /* m_pViewerContainer margin */ +
                            10 * 2 /* CentralWidget margin */;
        }
        QRect defaultGeometry(0, 0, iDefaultWidth, iDefaultHeight);
        defaultGeometry.moveCenter(parentWidget()->geometry().center());

        /* Load geometry: */
        m_geometry = gEDataManager->logWindowGeometry(this, defaultGeometry);

        /* Restore geometry: */
        LogRel2(("GUI: UIVMLogViewer: Restoring geometry to: Origin=%dx%d, Size=%dx%d\n",
                 m_geometry.x(), m_geometry.y(), m_geometry.width(), m_geometry.height()));
        restoreGeometry();
    }
}

void UIVMLogViewer::saveSettings()
{
    /* Save window geometry: */
    {
        /* Save geometry: */
        const QRect saveGeometry = geometry();
#ifdef VBOX_WS_MAC
        gEDataManager->setLogWindowGeometry(saveGeometry, ::darwinIsWindowMaximized(this));
#else /* !VBOX_WS_MAC */
        gEDataManager->setLogWindowGeometry(saveGeometry, isMaximized());
#endif /* !VBOX_WS_MAC */
        LogRel2(("GUI: UIVMLogViewer: Geometry saved as: Origin=%dx%d, Size=%dx%d\n",
                 saveGeometry.x(), saveGeometry.y(), saveGeometry.width(), saveGeometry.height()));
    }
}

void UIVMLogViewer::cleanup()
{
    /* Save settings: */
    saveSettings();

    /* Remove log-viewer: */
    if (!m_machine.isNull())
        m_viewers.remove(m_machine.GetName());
}

void UIVMLogViewer::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIVMLogViewer::retranslateUi(this);

    /* Setup a dialog caption: */
    if (!m_machine.isNull())
        setWindowTitle(tr("%1 - VirtualBox Log Viewer").arg(m_machine.GetName()));

    /* Translate other tags: */
    m_pButtonFind->setText(tr("&Find"));
    m_pButtonRefresh->setText(tr("&Refresh"));
    m_pButtonSave->setText(tr("&Save"));
    m_pButtonClose->setText(tr("Close"));
    m_pButtonFilter->setText(tr("Fil&ter"));
}

void UIVMLogViewer::showEvent(QShowEvent *pEvent)
{
    QIMainWindow::showEvent(pEvent);

    /* One may think that QWidget::polish() is the right place to do things
     * below, but apparently, by the time when QWidget::polish() is called,
     * the widget style & layout are not fully done, at least the minimum
     * size hint is not properly calculated. Since this is sometimes necessary,
     * we provide our own "polish" implementation: */

    if (m_fIsPolished)
        return;

    m_fIsPolished = true;

    /* Make sure the log view widget has the focus: */
    QWidget *pCurrentLogPage = currentLogPage();
    if (pCurrentLogPage)
        pCurrentLogPage->setFocus();
}

void UIVMLogViewer::keyPressEvent(QKeyEvent *pEvent)
{
    /* Depending on key pressed: */
    switch (pEvent->key())
    {
        /* Process key escape as VM Log Viewer close: */
        case Qt::Key_Escape:
        {
            m_pButtonClose->animateClick();
            return;
        }
        /* Process Back key as switch to previous tab: */
        case Qt::Key_Back:
        {
            if (m_pViewerContainer->currentIndex() > 0)
            {
                m_pViewerContainer->setCurrentIndex(m_pViewerContainer->currentIndex() - 1);
                return;
            }
            break;
        }
        /* Process Forward key as switch to next tab: */
        case Qt::Key_Forward:
        {
            if (m_pViewerContainer->currentIndex() < m_pViewerContainer->count())
            {
                m_pViewerContainer->setCurrentIndex(m_pViewerContainer->currentIndex() + 1);
                return;
            }
            break;
        }
        default:
            break;
    }
    QIMainWindow::keyReleaseEvent(pEvent);
}

QTextEdit* UIVMLogViewer::currentLogPage()
{
    /* If viewer-container is enabled: */
    if (m_pViewerContainer->isEnabled())
    {
        /* Get and return current log-page: */
        QWidget *pContainer = m_pViewerContainer->currentWidget();
        QTextEdit *pBrowser = pContainer->findChild<QTextEdit*>();
        Assert(pBrowser);
        return pBrowser ? pBrowser : 0;
    }
    /* Return NULL by default: */
    return 0;
}

QTextEdit* UIVMLogViewer::createLogPage(const QString &strName)
{
    /* Create page-container: */
    QWidget *pPageContainer = new QWidget;
    AssertPtrReturn(pPageContainer, 0);
    {
        /* Create page-layout: */
        QVBoxLayout *pPageLayout = new QVBoxLayout(pPageContainer);
        AssertPtrReturn(pPageLayout, 0);
        /* Create Log-Viewer: */
        QTextEdit *pLogViewer = new QTextEdit(pPageContainer);
        AssertPtrReturn(pLogViewer, 0);
        {
            /* Configure Log-Viewer: */
#if defined(RT_OS_SOLARIS)
            /* Use system fixed-width font on Solaris hosts as the Courier family fonts don't render well. */
            QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#else
            QFont font = pLogViewer->currentFont();
            font.setFamily("Courier New,courier");
#endif
            pLogViewer->setFont(font);
            pLogViewer->setWordWrapMode(QTextOption::NoWrap);
            pLogViewer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            pLogViewer->setReadOnly(true);
            /* Add Log-Viewer to page-layout: */
            pPageLayout->addWidget(pLogViewer);
        }
        /* Add page-container to viewer-container: */
        m_pViewerContainer->addTab(pPageContainer, strName);
        return pLogViewer;
    }
}

const QString& UIVMLogViewer::currentLog()
{
    return m_logMap[currentLogPage()];
}

#include "UIVMLogViewer.moc"

