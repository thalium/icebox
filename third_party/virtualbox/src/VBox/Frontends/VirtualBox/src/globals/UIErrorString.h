/* $Id: UIErrorString.h $ */
/** @file
 * VBox Qt GUI - UIErrorString class declaration.
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

#ifndef ___UIErrorString_h___
#define ___UIErrorString_h___

/* Qt includes: */
#include <QString>

/* Other VBox includes: */
#include <VBox/com/defs.h>

/* Forward declarations: */
class COMBaseWithEI;
class COMErrorInfo;
class COMResult;
class CProgress;
class CVirtualBoxErrorInfo;


/** Namespace simplifying COM error formatting. */
class UIErrorString
{
public:

    /** Returns formatted @a rc information. */
    static QString formatRC(HRESULT rc);
    /** Returns full formatted @a rc information. */
    static QString formatRCFull(HRESULT rc);
    /** Returns formatted error information for passed @a comProgress. */
    static QString formatErrorInfo(const CProgress &comProgress);
    /** Returns formatted error information for passed @a comInfo and @a wrapperRC. */
    static QString formatErrorInfo(const COMErrorInfo &comInfo, HRESULT wrapperRC = S_OK);
    /** Returns formatted error information for passed @a comInfo. */
    static QString formatErrorInfo(const CVirtualBoxErrorInfo &comInfo);
    /** Returns formatted error information for passed @a comWrapper. */
    static QString formatErrorInfo(const COMBaseWithEI &comWrapper);
    /** Returns formatted error information for passed @a comRc. */
    static QString formatErrorInfo(const COMResult &comRc);

private:

    /** Converts passed @a comInfo and @a wrapperRC to string. */
    static QString errorInfoToString(const COMErrorInfo &comInfo, HRESULT wrapperRC = S_OK);
};

#endif /* !___UIErrorString_h___ */

