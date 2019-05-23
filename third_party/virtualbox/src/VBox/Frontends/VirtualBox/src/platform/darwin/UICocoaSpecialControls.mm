/* $Id: UICocoaSpecialControls.mm $ */
/** @file
 * VBox Qt GUI - UICocoaSpecialControls implementation.
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

/* VBox includes */
#include "UICocoaSpecialControls.h"
#include "VBoxUtils-darwin.h"
#include "UIImageTools.h"
#include <VBox/cdefs.h>

/* System includes */
#import <AppKit/NSApplication.h>
#import <AppKit/NSBezierPath.h>
#import <AppKit/NSButton.h>
#import <AppKit/NSFont.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSSegmentedControl.h>
#import <AppKit/NSEvent.h>
#import <AppKit/NSColor.h>
#import <AppKit/NSSearchFieldCell.h>
#import <AppKit/NSSearchField.h>
#import <AppKit/NSSegmentedCell.h>

/* Qt includes */
#include <QAccessibleWidget>
#include <QApplication>
#include <QIcon>
#include <QKeyEvent>
#include <QMacCocoaViewContainer>

/* Other VBox includes: */
#include <iprt/assert.h>

/* Forward declarations: */
class UIAccessibilityInterfaceForUICocoaSegmentedButton;


/*
 * Private interfaces
 */
@interface UIButtonTargetPrivate: NSObject
{
    UICocoaButton *mRealTarget;
}
/* The next method used to be called initWithObject, but Xcode 4.1 preview 5
   cannot cope with that for some reason.  Hope this doesn't break anything... */
-(id)initWithObjectAndLionTrouble:(UICocoaButton*)object;
-(IBAction)clicked:(id)sender;
@end

@interface UISegmentedButtonTargetPrivate: NSObject
{
    UICocoaSegmentedButton *mRealTarget;
}
-(id)initWithObject1:(UICocoaSegmentedButton*)object;
-(IBAction)segControlClicked:(id)sender;
@end

@interface UISearchFieldCellPrivate: NSSearchFieldCell
{
    NSColor *mBGColor;
}
- (void)setBackgroundColor:(NSColor*)aBGColor;
@end

@interface UISearchFieldPrivate: NSSearchField
{
    UICocoaSearchField *mRealTarget;
}
-(id)initWithObject2:(UICocoaSearchField*)object;
@end

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
@interface UISearchFieldDelegatePrivate: NSObject<NSTextFieldDelegate>
#else
@interface UISearchFieldDelegatePrivate: NSObject
#endif
{}
@end

/*
 * Implementation of the private interfaces
 */
@implementation UIButtonTargetPrivate
-(id)initWithObjectAndLionTrouble:(UICocoaButton*)object
{
    self = [super init];

    mRealTarget = object;

    return self;
}

-(IBAction)clicked:(id)sender
{
    RT_NOREF(sender);
    mRealTarget->onClicked();
}
@end

@implementation UISegmentedButtonTargetPrivate
-(id)initWithObject1:(UICocoaSegmentedButton*)object
{
    self = [super init];

    mRealTarget = object;

    return self;
}

-(IBAction)segControlClicked:(id)sender
{
    mRealTarget->onClicked([sender selectedSegment]);
}
@end

@implementation UISearchFieldCellPrivate
-(id)init
{
    if ((self = [super init]))
        mBGColor = Nil;
    return self;
}

- (void)dealloc
{
    [mBGColor release];
    [super dealloc];
}

- (void)setBackgroundColor:(NSColor*)aBGColor
{
    if (mBGColor != aBGColor)
    {
        [mBGColor release];
        mBGColor = [aBGColor retain];
    }
}

- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
    if (mBGColor != Nil)
    {
        [mBGColor setFill];
        NSRect frame = cellFrame;
        double radius = RT_MIN(NSWidth(frame), NSHeight(frame)) / 2.0;
        [[NSBezierPath bezierPathWithRoundedRect:frame xRadius:radius yRadius:radius] fill];
    }

    [super drawInteriorWithFrame:cellFrame inView:controlView];
}
@end

@implementation UISearchFieldPrivate
+ (Class)cellClass
{
    return [UISearchFieldCellPrivate class];
}

-(id)initWithObject2:(UICocoaSearchField*)object
{
    self = [super init];

    mRealTarget = object;


    return self;
}

- (void)keyUp:(NSEvent *)theEvent
{
    /* This here is a little bit hacky. Grab important keys & forward they to
       the parent Qt widget. There a special key handling is done. */
    NSString *str = [theEvent charactersIgnoringModifiers];
    unichar ch = 0;

    /* Get the pressed character */
    if ([str length] > 0)
        ch = [str characterAtIndex:0];

    if (ch == NSCarriageReturnCharacter || ch == NSEnterCharacter)
    {
        QKeyEvent ke(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
        QApplication::sendEvent(mRealTarget, &ke);
    }
    else if (ch == 27) /* Escape */
    {
        QKeyEvent ke(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(mRealTarget, &ke);
    }
    else if (ch == NSF3FunctionKey)
    {
        QKeyEvent ke(QEvent::KeyPress, Qt::Key_F3, [theEvent modifierFlags] & NSShiftKeyMask ? Qt::ShiftModifier : Qt::NoModifier);
        QApplication::sendEvent(mRealTarget, &ke);
    }

    [super keyUp:theEvent];
}

//{
//    QWidget *w = QApplication::focusWidget();
//    if (w)
//        w->clearFocus();
//}

- (void)textDidChange:(NSNotification *)aNotification
{
    mRealTarget->onTextChanged(::darwinNSStringToQString([[aNotification object] string]));
}
@end

@implementation UISearchFieldDelegatePrivate
-(BOOL)control:(NSControl*)control textView:(NSTextView*)textView doCommandBySelector:(SEL)commandSelector
{
    RT_NOREF(control, textView);
//    NSLog(NSStringFromSelector(commandSelector));
    /* Don't execute the selector for Enter & Escape. */
    if (   commandSelector == @selector(insertNewline:)
            || commandSelector == @selector(cancelOperation:))
                return YES;
    return NO;
}
@end


/*
 * Helper functions
 */
NSRect darwinCenterRectVerticalTo(NSRect aRect, const NSRect& aToRect)
{
    aRect.origin.y = (aToRect.size.height - aRect.size.height) / 2.0;
    return aRect;
}

/*
 * Public classes
 */
UICocoaButton::UICocoaButton(QWidget *pParent, CocoaButtonType type)
    : QMacCocoaViewContainer(0, pParent)
{
    /* Prepare auto-release pool: */
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    /* Prepare native button reference: */
    NativeNSButtonRef pNativeRef;
    NSRect initFrame;

    /* Configure button: */
    switch (type)
    {
        case HelpButton:
        {
            pNativeRef = [[NSButton alloc] init];
            [pNativeRef setTitle: @""];
            [pNativeRef setBezelStyle: NSHelpButtonBezelStyle];
            [pNativeRef setBordered: YES];
            [pNativeRef setAlignment: NSCenterTextAlignment];
            [pNativeRef sizeToFit];
            initFrame = [pNativeRef frame];
            initFrame.size.width += 12; /* Margin */
            [pNativeRef setFrame:initFrame];
            break;
        };
        case CancelButton:
        {
            pNativeRef = [[NSButton alloc] initWithFrame: NSMakeRect(0, 0, 13, 13)];
            [pNativeRef setTitle: @""];
            [pNativeRef setBezelStyle:NSShadowlessSquareBezelStyle];
            [pNativeRef setButtonType:NSMomentaryChangeButton];
            [pNativeRef setImage: [NSImage imageNamed: NSImageNameStopProgressFreestandingTemplate]];
            [pNativeRef setBordered: NO];
            [[pNativeRef cell] setImageScaling: NSImageScaleProportionallyDown];
            initFrame = [pNativeRef frame];
            break;
        }
        case ResetButton:
        {
            pNativeRef = [[NSButton alloc] initWithFrame: NSMakeRect(0, 0, 13, 13)];
            [pNativeRef setTitle: @""];
            [pNativeRef setBezelStyle:NSShadowlessSquareBezelStyle];
            [pNativeRef setButtonType:NSMomentaryChangeButton];
            [pNativeRef setImage: [NSImage imageNamed: NSImageNameRefreshFreestandingTemplate]];
            [pNativeRef setBordered: NO];
            [[pNativeRef cell] setImageScaling: NSImageScaleProportionallyDown];
            initFrame = [pNativeRef frame];
            break;
        }
    }

    /* Install click listener: */
    UIButtonTargetPrivate *bt = [[UIButtonTargetPrivate alloc] initWithObjectAndLionTrouble:this];
    [pNativeRef setTarget:bt];
    [pNativeRef setAction:@selector(clicked:)];

    /* Put the button to the QCocoaViewContainer: */
    setCocoaView(pNativeRef);
    /* Release our reference, since our super class
     * takes ownership and we don't need it anymore. */
    [pNativeRef release];

    /* Finally resize the widget: */
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(NSWidth(initFrame), NSHeight(initFrame));

    /* Cleanup auto-release pool: */
    [pool release];
}

UICocoaButton::~UICocoaButton()
{
}

QSize UICocoaButton::sizeHint() const
{
    NSRect frame = [nativeRef() frame];
    return QSize(frame.size.width, frame.size.height);
}

void UICocoaButton::setText(const QString& strText)
{
    QString s(strText);
    /* Set it for accessibility reasons as alternative title */
    [nativeRef() setAlternateTitle: ::darwinQStringToNSString(s.remove('&'))];
}

void UICocoaButton::setToolTip(const QString& strTip)
{
    [nativeRef() setToolTip: ::darwinQStringToNSString(strTip)];
}

void UICocoaButton::onClicked()
{
    emit clicked(false);
}


/** QAccessibleInterface extension used as an accessibility interface for segmented-button segment. */
class UIAccessibilityInterfaceForUICocoaSegmentedButtonSegment : public QAccessibleInterface
{
public:

    /** Constructs an accessibility interface.
      * @param  pParent  Brings the parent interface we are linked to.
      * @param  iIndex   Brings the index of segment we are referring to. */
    UIAccessibilityInterfaceForUICocoaSegmentedButtonSegment(UIAccessibilityInterfaceForUICocoaSegmentedButton *pParent, int iIndex)
        : m_pParent(pParent)
        , m_iIndex(iIndex)
    {}

    /** Returns whether the interface is valid. */
    virtual bool isValid() const /* override */ { return true; }

    /** Returns the wrapped object. */
    virtual QObject *object() const /* override */ { return 0; }
    /** Returns the parent. */
    virtual QAccessibleInterface *parent() const /* override */;

    /** Returns the number of children. */
    virtual int childCount() const /* override */ { return 0; }
    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int /* iIndex */) const /* override */ { return 0; }
    /** Returns the child at position QPoint(@a x, @a y). */
    virtual QAccessibleInterface *childAt(int /* x */, int /* y */) const /* override */ { return 0; }
    /** Returns the index of the passed @a pChild. */
    virtual int indexOfChild(const QAccessibleInterface * /* pChild */) const /* override */ { return -1; }

    /** Returns the rect. */
    virtual QRect rect() const /* override */;

    /** Defines a @a strText for the passed @a enmTextRole. */
    virtual void setText(QAccessible::Text /* enmTextRole */, const QString & /* strText */) /* override */ {}
    /** Returns a text for the passed @a enmTextRole. */
    virtual QString text(QAccessible::Text /* enmTextRole */) const /* override */;

    /** Returns the role. */
    virtual QAccessible::Role role() const /* override */ { return QAccessible::RadioButton; }
    /** Returns the state. */
    virtual QAccessible::State state() const /* override */;

private:

    /** Holds the parent interface we are linked to. */
    UIAccessibilityInterfaceForUICocoaSegmentedButton *m_pParent;
    /** Holds the index of segment we are referring to. */
    const int m_iIndex;
};


/** QAccessibleWidget extension used as an accessibility interface for segmented-button. */
class UIAccessibilityInterfaceForUICocoaSegmentedButton : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating segmented-button accessibility interface: */
        if (pObject && strClassname == QLatin1String("UICocoaSegmentedButton"))
            return new UIAccessibilityInterfaceForUICocoaSegmentedButton(qobject_cast<QWidget*>(pObject));

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    UIAccessibilityInterfaceForUICocoaSegmentedButton(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::ToolBar)
    {
        /* Prepare: */
        prepare();
    }

    /** Destructs an accessibility interface. */
    ~UIAccessibilityInterfaceForUICocoaSegmentedButton()
    {
        /* Cleanup: */
        cleanup();
    }

    /** Returns the number of children. */
    virtual int childCount() const /* override */ { return m_children.size(); }
    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int iIndex) const /* override */ { return m_children.at(iIndex); }

    /** Returns corresponding segmented-button. */
    UICocoaSegmentedButton *button() const { return qobject_cast<UICocoaSegmentedButton*>(widget()); }

private:

    /** Prepares all. */
    void prepare()
    {
        /* Prepare the list of children interfaces: */
        for (int i = 0; i < button()->count(); ++i)
            m_children << new UIAccessibilityInterfaceForUICocoaSegmentedButtonSegment(this, i);
    }

    /** Cleanups all. */
    void cleanup()
    {
        /* Cleanup the list of children interfaces: */
        qDeleteAll(m_children);
        m_children.clear();
    }

    /** Holds the list of children interfaces. */
    QList<UIAccessibilityInterfaceForUICocoaSegmentedButtonSegment*> m_children;
};


QAccessibleInterface *UIAccessibilityInterfaceForUICocoaSegmentedButtonSegment::parent() const
{
    return m_pParent;
}

QRect UIAccessibilityInterfaceForUICocoaSegmentedButtonSegment::rect() const
{
    /// @todo Return the -=real=- segment rectangle.
    const QRect myRect = m_pParent->rect();
    return QRect(myRect.x() + myRect.width() / 2 * m_iIndex,
                 myRect.y(), myRect.width() / 2, myRect.height());
}

QString UIAccessibilityInterfaceForUICocoaSegmentedButtonSegment::text(QAccessible::Text /* enmTextRole */) const
{
    /* Return the segment description: */
    return m_pParent->button()->description(m_iIndex);
}

QAccessible::State UIAccessibilityInterfaceForUICocoaSegmentedButtonSegment::state() const
{
    /* Compose the segment state: */
    QAccessible::State state;
    state.checkable = true;
    state.checked = m_pParent->button()->isSelected(m_iIndex);

    /* Return the segment state: */
    return state;
}


UICocoaSegmentedButton::UICocoaSegmentedButton(QWidget *pParent, int count, CocoaSegmentType type /* = RoundRectSegment */)
    : QMacCocoaViewContainer(0, pParent)
{
    /* Install segmented-button accessibility interface factory: */
    QAccessible::installFactory(UIAccessibilityInterfaceForUICocoaSegmentedButton::pFactory);

    /* Prepare auto-release pool: */
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    /* Prepare native segmented-button reference: */
    NativeNSSegmentedControlRef pNativeRef;
    NSRect initFrame;

    /* Configure segmented-button: */
    pNativeRef = [[NSSegmentedControl alloc] init];
    [pNativeRef setSegmentCount:count];
    switch (type)
    {
        case RoundRectSegment:
        {
            [pNativeRef setSegmentStyle: NSSegmentStyleRoundRect];
            [pNativeRef setFont: [NSFont controlContentFontOfSize:
                [NSFont systemFontSizeForControlSize: NSSmallControlSize]]];
            [[pNativeRef cell] setTrackingMode: NSSegmentSwitchTrackingMomentary];
            break;
        }
        case TexturedRoundedSegment:
        {
            [pNativeRef setSegmentStyle: NSSegmentStyleTexturedRounded];
            [pNativeRef setFont: [NSFont controlContentFontOfSize:
                [NSFont systemFontSizeForControlSize: NSSmallControlSize]]];
            break;
        }
    }

    /* Calculate corresponding size: */
    [pNativeRef sizeToFit];
    initFrame = [pNativeRef frame];

    /* Install click listener: */
    UISegmentedButtonTargetPrivate *bt = [[UISegmentedButtonTargetPrivate alloc] initWithObject1:this];
    [pNativeRef setTarget:bt];
    [pNativeRef setAction:@selector(segControlClicked:)];

    /* Put the button to the QCocoaViewContainer: */
    setCocoaView(pNativeRef);
    /* Release our reference, since our super class
     * takes ownership and we don't need it anymore. */
    [pNativeRef release];

    /* Finally resize the widget: */
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(NSWidth(initFrame), NSHeight(initFrame));

    /* Cleanup auto-release pool: */
    [pool release];
}

int UICocoaSegmentedButton::count() const
{
    return [nativeRef() segmentCount];
}

bool UICocoaSegmentedButton::isSelected(int iSegment) const
{
    /* Return whether the segment is selected if segment index inside the bounds: */
    if (iSegment >=0 && iSegment < count())
        return [nativeRef() isSelectedForSegment: iSegment];

    /* False by default: */
    return false;
}

QString UICocoaSegmentedButton::description(int iSegment) const
{
    /* Return segment description if segment index inside the bounds: */
    if (iSegment >=0 && iSegment < count())
        return ::darwinNSStringToQString([nativeRef() labelForSegment: iSegment]);

    /* Null-string by default: */
    return QString();
}

QSize UICocoaSegmentedButton::sizeHint() const
{
    NSRect frame = [nativeRef() frame];
    return QSize(frame.size.width, frame.size.height);
}

void UICocoaSegmentedButton::setTitle(int iSegment, const QString &strTitle)
{
    QString s(strTitle);
    [nativeRef() setLabel: ::darwinQStringToNSString(s.remove('&')) forSegment: iSegment];
    [nativeRef() sizeToFit];
    NSRect frame = [nativeRef() frame];
    setFixedSize(NSWidth(frame), NSHeight(frame));
}

void UICocoaSegmentedButton::setToolTip(int iSegment, const QString &strTip)
{
    [[nativeRef() cell] setToolTip: ::darwinQStringToNSString(strTip) forSegment: iSegment];
}

void UICocoaSegmentedButton::setIcon(int iSegment, const QIcon& icon)
{
    QImage image = toGray(icon.pixmap(icon.availableSizes().value(0, QSize(16, 16))).toImage());

    NSImage *pNSimage = [::darwinToNSImageRef(&image) autorelease];
    [nativeRef() setImage: pNSimage forSegment: iSegment];
    [nativeRef() sizeToFit];
    NSRect frame = [nativeRef() frame];
    setFixedSize(NSWidth(frame), NSHeight(frame));
}

void UICocoaSegmentedButton::setEnabled(int iSegment, bool fEnabled)
{
    [[nativeRef() cell] setEnabled: fEnabled forSegment: iSegment];
}

void UICocoaSegmentedButton::setSelected(int iSegment)
{
    [nativeRef() setSelectedSegment: iSegment];
}

void UICocoaSegmentedButton::animateClick(int iSegment)
{
    [nativeRef() setSelectedSegment: iSegment];
    [[nativeRef() cell] performClick: nativeRef()];
}

void UICocoaSegmentedButton::onClicked(int iSegment)
{
    emit clicked(iSegment, false);
}


/** QAccessibleWidget extension used as an accessibility interface for search-field. */
class UIAccessibilityInterfaceForUICocoaSearchField : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating segmented-button accessibility interface: */
        if (pObject && strClassname == QLatin1String("UICocoaSearchField"))
            return new UIAccessibilityInterfaceForUICocoaSearchField(qobject_cast<QWidget*>(pObject));

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    UIAccessibilityInterfaceForUICocoaSearchField(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::EditableText)
    {
        // For now this class doesn't implement interface casting.
        // Which means there will be no editable text accessible
        // in basic accessibility layer, only in advanced one.
    }

private:

    /** Returns corresponding search-field. */
    UICocoaSearchField *field() const { return qobject_cast<UICocoaSearchField*>(widget()); }
};


UICocoaSearchField::UICocoaSearchField(QWidget *pParent)
    : QMacCocoaViewContainer(0, pParent)
{
    /* Install segmented-button accessibility interface factory: */
    QAccessible::installFactory(UIAccessibilityInterfaceForUICocoaSearchField::pFactory);

    /* Prepare auto-release pool: */
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    /* Prepare native search-field reference: */
    NativeNSSearchFieldRef pNativeRef;
    NSRect initFrame;

    /* Configure search-field: */
    pNativeRef = [[UISearchFieldPrivate alloc] initWithObject2: this];
    [pNativeRef setFont: [NSFont controlContentFontOfSize:
        [NSFont systemFontSizeForControlSize: NSMiniControlSize]]];
    [[pNativeRef cell] setControlSize: NSMiniControlSize];
    [pNativeRef sizeToFit];
    initFrame = [pNativeRef frame];
    initFrame.size.width = 180;
    [pNativeRef setFrame: initFrame];

    /* Use our own delegate: */
    UISearchFieldDelegatePrivate *sfd = [[UISearchFieldDelegatePrivate alloc] init];
    [pNativeRef setDelegate: sfd];

    /* Put the button to the QCocoaViewContainer: */
    setCocoaView(pNativeRef);
    /* Release our reference, since our super class
     * takes ownership and we don't need it anymore. */
    [pNativeRef release];

    /* Finally resize the widget: */
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumWidth(NSWidth(initFrame));
    setFixedHeight(NSHeight(initFrame));
    setFocusPolicy(Qt::StrongFocus);

    /* Cleanup auto-release pool: */
    [pool release];
}

UICocoaSearchField::~UICocoaSearchField()
{
}

QSize UICocoaSearchField::sizeHint() const
{
    NSRect frame = [nativeRef() frame];
    return QSize(frame.size.width, frame.size.height);
}

QString UICocoaSearchField::text() const
{
    return ::darwinNSStringToQString([nativeRef() stringValue]);
}

void UICocoaSearchField::insert(const QString &strText)
{
    [[nativeRef() currentEditor] setString: [[nativeRef() stringValue] stringByAppendingString: ::darwinQStringToNSString(strText)]];
}

void UICocoaSearchField::setToolTip(const QString &strTip)
{
    [nativeRef() setToolTip: ::darwinQStringToNSString(strTip)];
}

void UICocoaSearchField::selectAll()
{
    [nativeRef() selectText: nativeRef()];
}

void UICocoaSearchField::markError()
{
    [[nativeRef() cell] setBackgroundColor: [[NSColor redColor] colorWithAlphaComponent: 0.3]];
}

void UICocoaSearchField::unmarkError()
{
    [[nativeRef() cell] setBackgroundColor: nil];
}

void UICocoaSearchField::onTextChanged(const QString &strText)
{
    emit textChanged(strText);
}

