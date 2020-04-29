# !kmk_ash
# $Id: backport-merge-and-commit.sh $
## @file
# Script for merging and commit a backport from trunk.
#

#
# Copyright (C) 2020 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

#
# Determin script dir so we can invoke the two worker scripts.
#
MY_SED=kmk_sed
MY_SCRIPT_DIR=`echo "$0" | "${MY_SED}" -e 's|\\\|/|g' -e 's|^\(.*\)/[^/][^/]*$|\1|'` # \ -> / is for windows.
if test "${MY_SCRIPT_DIR}" = "$0"; then
    MY_SCRIPT_DIR=`pwd -L`
else
    MY_SCRIPT_DIR=`cd "${MY_SCRIPT_DIR}"; pwd -L`       # pwd is built into kmk_ash.
fi

#
# Merge & commit.
#
set -e
"${MY_SCRIPT_DIR}/backport-merge.sh" $*
echo
"${MY_SCRIPT_DIR}/backport-commit.sh" $*

