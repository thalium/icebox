# !kmk_ash
# $Id: backport-merge.sh $
## @file
# Script for merging a backport from trunk.
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
# Determin script dir so we can source the common bits.
#
MY_SED=kmk_sed
MY_SCRIPT_DIR=`echo "$0" | "${MY_SED}" -e 's|\\\|/|g' -e 's|^\(.*\)/[^/][^/]*$|\1|'` # \ -> / is for windows.
if test "${MY_SCRIPT_DIR}" = "$0"; then
    MY_SCRIPT_DIR=`pwd -L`
else
    MY_SCRIPT_DIR=`cd "${MY_SCRIPT_DIR}"; pwd -L`       # pwd is built into kmk_ash.
fi

#
# This does a lot.
#
MY_SCRIPT_NAME="backport-merge.sh"
. "${MY_SCRIPT_DIR}/backport-common.sh"

#
# Do the merging.
#
MY_DONE_REVS=
MY_FAILED_REV=
MY_TODO_REVS=
test -n "${MY_DEBUG}" && echo "MY_REVISIONS=${MY_REVISIONS}"
for MY_REV in ${MY_REVISIONS};
do
    if test -z "${MY_FAILED}"; then
        echo "***"
        echo "*** Merging r${MY_REV} ..."
        echo "***"
        if "${MY_SVN}" merge ${MY_MERGE_ARGS} "${MY_TRUNK_DIR}" "${MY_BRANCH_DIR}" -c ${MY_REV}; then
            # Check for conflict.
            MY_CONFLICTS=`"${MY_SVN}" status "${MY_BRANCH_DIR}" | "${MY_SED}" -n -e '/^C/p'`
            if test -z "${MY_CONFLICTS}"; then
                MY_DONE_REVS="${MY_DONE_REVS} ${MY_REV}"
            else
                echo '!!!'" Have conflicts after merging ${MY_REV}."
                MY_FAILED_REV=${MY_REV}
            fi
        else
            echo '!!!'" Failed merging r${MY_REV}."
            MY_FAILED_REV=${MY_REV}
        fi
    else
        MY_TODO_REVS="${MY_TODO_REVS} ${MY_REV}"
    fi
done

if test -z "${MY_FAILED_REV}"; then
    echo "* Successfully merged all revision (${MY_DONE_REVS})."
    exit 0;
fi
if test -n "${MY_DONE_REVS}"; then
    echo "* Successfully merged: ${MY_DONE_REVS}"
fi
if test -n "${MY_TODO_REVS}"; then
    echo "* Revisions left todo: ${MY_TODO_REVS}"
fi
exit 1;
