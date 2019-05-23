/* $Id: UIWizardImportAppDefs.h $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppDefs class declaration.
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

#ifndef __UIWizardImportAppDefs_h__
#define __UIWizardImportAppDefs_h__

/* Global includes: */
#include <QMetaType>
#include <QPointer>

/* Local includes: */
#include "UIApplianceImportEditorWidget.h"

/* Typedefs: */
typedef QPointer<UIApplianceImportEditorWidget> ImportAppliancePointer;
Q_DECLARE_METATYPE(ImportAppliancePointer);

#endif /* __UIWizardImportAppDefs_h__ */

