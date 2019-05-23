/* $Id: VBoxVersion.h $ */
/** @file
 * VBox Qt GUI - VBoxVersion class declaration.
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

#ifndef ___VBoxVersion_h___
#define ___VBoxVersion_h___

/**
 *  Represents VirtualBox version wrapper
 */
class VBoxVersion
{
public:

    VBoxVersion() : m_x(-1), m_y(-1), m_z(-1) {}

    VBoxVersion(const QString &strVersion)
        : m_x(-1), m_y(-1), m_z(-1)
    {
        QStringList versionStack = strVersion.split('.');
        if (versionStack.size() > 0)
            m_x = versionStack[0].toInt();
        if (versionStack.size() > 1)
            m_y = versionStack[1].toInt();
        if (versionStack.size() > 2)
            m_z = versionStack[2].toInt();
    }

    VBoxVersion& operator=(const VBoxVersion &other) { m_x = other.x(); m_y = other.y(); m_z = other.z(); return *this; }

    bool isValid() const { return m_x != -1 && m_y != -1 && m_z != -1; }

    bool equal(const VBoxVersion &other) const { return (m_x == other.m_x) && (m_y == other.m_y) && (m_z == other.m_z); }
    bool operator==(const VBoxVersion &other) const { return equal(other); }
    bool operator!=(const VBoxVersion &other) const { return !equal(other); }

    bool operator<(const VBoxVersion &other) const
    {
        return (m_x <  other.m_x) ||
               (m_x == other.m_x && m_y <  other.m_y) ||
               (m_x == other.m_x && m_y == other.m_y && m_z <  other.m_z);
    }
    bool operator>(const VBoxVersion &other) const
    {
        return (m_x >  other.m_x) ||
               (m_x == other.m_x && m_y >  other.m_y) ||
               (m_x == other.m_x && m_y == other.m_y && m_z >  other.m_z);
    }

    QString toString() const
    {
        return QString("%1.%2.%3").arg(m_x).arg(m_y).arg(m_z);
    }

    int x() const { return m_x; }
    int y() const { return m_y; }
    int z() const { return m_z; }

private:

    int m_x;
    int m_y;
    int m_z;
};

#endif // !___VBoxVersion_h___

