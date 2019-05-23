/* $Id: UIVersion.h $ */
/** @file
 * VBox Qt GUI - UIVersion class declaration/implementation.
 */

/*
 * Copyright (C) 2006-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIVersion_h___
#define ___UIVersion_h___

/** Represents VirtualBox version wrapper. */
class UIVersion
{
public:

    /** Constructs default object. */
    UIVersion()
        : m_x(-1), m_y(-1), m_z(-1)
    {}

    /** Constructs object based on parsed @a strVersion. */
    UIVersion(const QString &strVersion)
        : m_x(-1), m_y(-1), m_z(-1)
    {
        const QStringList versionStack = strVersion.split('.');
        if (versionStack.size() > 0)
            m_x = versionStack[0].toInt();
        if (versionStack.size() > 1)
            m_y = versionStack[1].toInt();
        if (versionStack.size() > 2)
            m_z = versionStack[2].toInt();
    }

    /** Assigns this object with value of @a another. */
    UIVersion &operator=(const UIVersion &another)
    {
        m_x = another.x();
        m_y = another.y();
        m_z = another.z();
        return *this;
    }

    /** Returns whether this object is valid. */
    bool isValid() const { return (m_x != -1) && (m_y != -1) && (m_z != -1); }

    /** Returns whether this object is equal to @a other. */
    bool equal(const UIVersion &other) const { return (m_x == other.m_x) && (m_y == other.m_y) && (m_z == other.m_z); }
    /** Checks whether this object is equal to @a other. */
    bool operator==(const UIVersion &other) const { return equal(other); }
    /** Checks whether this object is NOT equal to @a other. */
    bool operator!=(const UIVersion &other) const { return !equal(other); }

    /** Checks whether this object is lass than @a other. */
    bool operator<(const UIVersion &other) const
    {
        return    (m_x <  other.m_x)
               || (m_x == other.m_x && m_y <  other.m_y)
               || (m_x == other.m_x && m_y == other.m_y && m_z <  other.m_z);
    }
    /** Checks whether this object is more than @a other. */
    bool operator>(const UIVersion &other) const
    {
        return    (m_x >  other.m_x)
               || (m_x == other.m_x && m_y >  other.m_y)
               || (m_x == other.m_x && m_y == other.m_y && m_z >  other.m_z);
    }

    /** Returns object string representation. */
    QString toString() const { return QString("%1.%2.%3").arg(m_x).arg(m_y).arg(m_z); }

    /** Returns the object X value. */
    int x() const { return m_x; }
    /** Returns the object Y value. */
    int y() const { return m_y; }
    /** Returns the object Z value. */
    int z() const { return m_z; }

    /** Defines the object X value. */
    void setX(int x) { m_x = x; }
    /** Defines the object Y value. */
    void setY(int y) { m_y = y; }
    /** Defines the object Z value. */
    void setZ(int z) { m_z = z; }

    /** Returns effective released version guessed or hardcoded for this one version.
      * This can be even the version itself. */
    UIVersion effectiveRelasedVersion() const
    {
        /* First, we just copy the current one: */
        UIVersion version = *this;

        /* If this version being developed: */
        if (version.z() % 2 == 1)
        {
            /* If this version being developed on release branch (we guess the right one): */
            if (version.z() < 97)
                version.setZ(version.z() - 1);
            /* If this version being developed on trunk (we use hardcoded one for now): */
            else
                version.setZ(6); /* Current .z for 5.2.z */
        }

        /* Finally, we just return that we have:  */
        return version;
    }

private:

    /** Holds the object X value. */
    int m_x;
    /** Holds the object Y value. */
    int m_y;
    /** Holds the object Z value. */
    int m_z;
};

#endif /* !___UIVersion_h___ */

