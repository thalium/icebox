# !kmk_ash
# $Id: backport-commit.sh $
## @file
# Script for committing a backport from trunk.
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
MY_SCRIPT_NAME="backport-commit.sh"
. "${MY_SCRIPT_DIR}/backport-common.sh"

#
# Generate the commit message into MY_MSG_FILE.
#
test -n "${MY_DEBUG}" && echo "MY_REVISIONS=${MY_REVISIONS}"
MY_MSG_FILE=backport-commit.txt
MY_TMP_FILE=backport-commit.tmp

if test "${MY_REVISION_COUNT}" -eq 1; then
    # Single revision, just prefix the commit message.
    MY_REV=`echo ${MY_REVISIONS}`       # strip leading space
    echo -n "${MY_BRANCH}: Backported r${MY_REV}: " > "${MY_MSG_FILE}"
    if ! "${MY_SVN}" log "-r${MY_REV}" "${MY_TRUNK_DIR}" > "${MY_TMP_FILE}"; then
        echo "error: failed to get log entry for revision ${MY_REV}"
        exit 1;
    fi
    if ! "${MY_SED}" -e '1d;2d;3d;$d' --append "${MY_MSG_FILE}" "${MY_TMP_FILE}"; then
        echo "error: failed to get log entry for revision ${MY_REV} (sed failed)"
        exit 1;
    fi
else
    # First line.
    echo -n "${MY_BRANCH}: Backported" > "${MY_MSG_FILE}"
    MY_COUNTER=0
    for MY_REV in ${MY_REVISIONS};
    do
        if test ${MY_COUNTER} -eq 0; then
            echo -n " r${MY_REV}" >> "${MY_MSG_FILE}"
        else
            echo -n " r${MY_REV}" >> "${MY_MSG_FILE}"
        fi
        MY_COUNTER=`"${MY_EXPR}" ${MY_COUNTER} + 1`
    done
    echo "." >> "${MY_MSG_FILE}"

    # One bullet with the commit text.
    for MY_REV in ${MY_REVISIONS};
    do
        echo -n "* r${MY_REV}: " >> "${MY_MSG_FILE}"
        if ! "${MY_SVN}" log "-r${MY_REV}" "${MY_TRUNK}" > "${MY_TMP_FILE}"; then
            echo "error: failed to get log entry for revision ${MY_REV}"
            exit 1;
        fi
        if ! "${MY_SED}" -e '1d;2d;3d;$d' --append "${MY_MSG_FILE}" "${MY_TMP_FILE}"; then
            echo "error: failed to get log entry for revision ${MY_REV} (sed failed)"
            exit 1;
        fi
    done
fi
"${MY_RM}" -f -- "${MY_TMP_FILE}"

#
# Do the committing.
#
echo "***"
echo "*** Commit message:"
"${MY_CAT}" "${MY_MSG_FILE}"
echo "***"
IFS=`"${MY_PRINTF}" " \t\r\n"` # windows needs \r for proper 'read' operation.
for MY_IGNORE in 1 2 3; do
    read -p "*** Does the above commit message look okay (y/n)?" MY_ANSWER
    case "${MY_ANSWER}" in
        y|Y|[yY][eE][sS])
            if "${MY_SVN}" commit -F "${MY_MSG_FILE}" "${MY_BRANCH_DIR}"; then
                "${MY_RM}" -f -- "${MY_MSG_FILE}"

                #
                # Update the branch so we don't end up with mixed revisions.
                #
                echo "***"
                echo "*** Updating branch dir..."
                "${MY_SVN}" up "${MY_BRANCH_DIR}"
                exit 0
            fi
            echo "error: commit failed" >2
            exit 1
            ;;
        n|N|[nN][oO])
            exit 1
            ;;
        *)
            echo "Please answer 'y' or 'n'... (MY_ANSWER=${MY_ANSWER})"
    esac
done
exit 1;

