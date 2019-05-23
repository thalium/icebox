/* $Id: UIRichTextString.h $ */
/** @file
 * VBox Qt GUI - UIRichTextString class declaration.
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIRichTextString_h___
#define ___UIRichTextString_h___

/* Qt includes: */
#include <QTextLayout>

/** Rich text string implementation which parses the passed QString
  * and holds it as the tree of the formatted rich text blocks. */
class UIRichTextString
{
public:

    /** Rich text block types. */
    enum Type
    {
        Type_None,
        Type_Anchor,
        Type_Bold,
        Type_Italic,
    };

    /** Default (empty) constructor. */
    UIRichTextString(Type type = Type_None);

    /** Constructor taking passed QString.
      * @param strString     holds the string being parsed and held as the tree of rich text blocks,
      * @param type          holds the type of <i>this</i> rich text block,
      * @param strStringMeta holds the string containing meta data describing <i>this</i> rich text block. */
    UIRichTextString(const QString &strString, Type type = Type_None, const QString &strStringMeta = QString());

    /** Destructor. */
    ~UIRichTextString();

    /** Returns the QString representation. */
    QString toString() const;

    /** Returns the list of existing format ranges appropriate for QTextLayout.
      * @param iShift holds the shift of <i>this</i> rich text block accordig to it's root. */
    QList<QTextLayout::FormatRange> formatRanges(int iShift = 0) const;

    /** Defines the anchor to highlight in <i>this</i> rich text block and in it's children. */
    void setHoveredAnchor(const QString &strHoveredAnchor);

private:

    /** Parses the string. */
    void parse();

    /** Used to populate const static map of known patterns.
      * @note Keep it sync with the method below - #populatePatternHasMeta(). */
    static QMap<Type, QString> populatePatterns();
    /** Used to populate const static map of meta flags for the known patterns.
      * @note Keep it sync with the method above - #populatePatterns(). */
    static QMap<Type, bool> populatePatternHasMeta();

    /** Recursively searching for the maximum level of the passed pattern.
      * @param strString         holds the string to check for the current (recursively advanced) pattern in,
      * @param strPattern        holds the etalon pattern to recursively advance the current pattern with,
      * @param strCurrentPattern holds the current (recursively advanced) pattern to check for the presence of,
      * @param iCurrentLevel     holds the current level of the recursively advanced pattern. */
    static int searchForMaxLevel(const QString &strString, const QString &strPattern,
                                 const QString &strCurrentPattern, int iCurrentLevel = 0);

    /** Recursively composing the pattern of the maximum level.
      * @param strPattern        holds the etalon pattern to recursively update the current pattern with,
      * @param strCurrentPattern holds the current (recursively advanced) pattern,
      * @param iCurrentLevel     holds the amount of the levels left to recursively advance current pattern. */
    static QString composeFullPattern(const QString &strPattern,
                                      const QString &strCurrentPattern, int iCurrentLevel);

    /** Composes the QTextCharFormat correpoding to passed @a type. */
    static QTextCharFormat textCharFormat(Type type);

    /** Holds the type of <i>this</i> rich text block. */
    Type m_type;
    /** Holds the string of <i>this</i> rich text block. */
    QString m_strString;
    /** Holds the string meta data of <i>this</i> rich text block. */
    QString m_strStringMeta;
    /** Holds the children of <i>this</i> rich text block. */
    QMap<int, UIRichTextString*> m_strings;

    /** Holds the anchor of <i>this</i> rich text block. */
    QString m_strAnchor;
    /** Holds the anchor to highlight in <i>this</i> rich text block and in it's children. */
    QString m_strHoveredAnchor;

    /** Holds the <i>any</i> string pattern. */
    static const QString m_sstrAny;
    /** Holds the map of known patterns. */
    static const QMap<Type, QString> m_sPatterns;
    /** Holds the map of meta flags for the known patterns. */
    static const QMap<Type, bool> m_sPatternHasMeta;
};

#endif /* !___UIRichTextString_h___ */

