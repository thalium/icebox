/* $Id: UIDefs.cpp $ */
/** @file
 * VBox Qt GUI - Global definitions.
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

/* GUI includes: */
# include "UIDefs.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* File name definitions: */
const char* UIDefs::GUI_GuestAdditionsName = "VBoxGuestAdditions";
const char* UIDefs::GUI_ExtPackName = "Oracle VM VirtualBox Extension Pack";

/* File extensions definitions: */
QStringList UIDefs::VBoxFileExts = QStringList() << "xml" << "vbox";
QStringList UIDefs::VBoxExtPackFileExts = QStringList() << "vbox-extpack";
QStringList UIDefs::OVFFileExts = QStringList() << "ovf" << "ova";
QStringList UIDefs::OPCFileExts = QStringList() << "tar.gz";

