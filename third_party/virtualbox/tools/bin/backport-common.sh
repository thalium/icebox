# $Id: backport-common.sh $
## @file
# Common backport script bits.
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
# Globals.
#
   MY_CAT=kmk_cat
  MY_EXPR=kmk_expr
MY_PRINTF=kmk_printf
    MY_RM=kmk_rm
   MY_SVN=svn
   MY_SED=kmk_sed

MY_BRANCH_DEFAULT_DIR=`cd "${MY_SCRIPT_DIR}"; cd ../..; pwd -L`
MY_BRANCH_DEFAULT=`echo "${MY_BRANCH_DEFAULT_DIR}" | "${MY_SED}" -e 's|^\(.*\)/\([^/][^/]*\)$|\2|' -e 's/VBox-//'`
if test "${MY_BRANCH_DEFAULT}" = "trunk"; then
    MY_TRUNK_DIR=${MY_BRANCH_DEFAULT_DIR}
elif test -d "${MY_BRANCH_DEFAULT_DIR}/../../trunk"; then
    MY_TRUNK_DIR=`cd "${MY_BRANCH_DEFAULT_DIR}"; cd ../../trunk; pwd -L`
else
    MY_TRUNK_DIR="^/trunk"
fi

#
# Parse arguments.
#
MY_BRANCH_DIR=
MY_BRANCH=
MY_REVISIONS=
MY_REVISION_COUNT=0
MY_EXTRA_ARGS=
MY_DEBUG=

while test $# -ge 1;
do
    ARG=$1
    shift
    case "${ARG}" in
        r[0-9][0-9]*)
            MY_REV=`echo ${ARG} | "${MY_SED}" -e 's/^r//'`
            MY_REVISIONS="${MY_REVISIONS} ${MY_REV}"
            MY_REVISION_COUNT=`${MY_EXPR} ${MY_REVISION_COUNT} + 1`
            ;;

        [0-9][0-9]*)
            MY_REVISIONS="${MY_REVISIONS} ${ARG}"
            MY_REVISION_COUNT=`${MY_EXPR} ${MY_REVISION_COUNT} + 1`
            ;;

        --trunk-dir)
            if test $# -eq 0; then
                echo "error: missing --trunk-dir argument." 1>&2
                exit 1;
            fi
            MY_TRUNK_DIR=`echo "$1" | "${SED}" -e 's|\\\|/|g'`
            shift
            ;;

        --branch-dir)
            if test $# -eq 0; then
                echo "error: missing --branch-dir argument." 1>&2
                exit 1;
            fi
            MY_BRANCH_DIR=`echo "$1" | "${SED}" -e 's|\\\|/|g'`
            shift
            ;;

        --branch)
            if test $# -eq 0; then
                echo "error: missing --branch argument." 1>&2
                exit 1;
            fi
            MY_BRANCH="$1"
            shift
            ;;

        --extra)
            if test $# -eq 0; then
                echo "error: missing --extra argument." 1>&2
                exit 1;
            fi
            MY_EXTRA_ARGS="${MY_EXTRA_ARGS} $1"
            shift
            ;;

        --debug)
            MY_DEBUG=1
            ;;

        # usage
        --h*|-h*|-?|--?)
            echo "usage: $0 [--trunk-dir <dir>] [--branch <ver>] [--branch-dir <dir>] [--extra <svn-arg>] rev1 [rev2..[revN]]]"
            echo ""
            echo "Options:"
            echo "  --trunk-dir <dir>"
            echo "  --branch <ver>"
            echo "  --branch-dir <dir>"
            echo "  --extra <svn-arg>"
            echo ""
            exit 2;
            ;;

        *)
            echo "syntax error: ${ARG}"
            exit 2;
            ;;
    esac
done

if test -n "${MY_DEBUG}"; then
    echo "        MY_SCRIPT_DIR=${MY_SCRIPT_DIR}"
    echo "MY_BRANCH_DEFAULT_DIR=${MY_BRANCH_DEFAULT_DIR}"
    echo "    MY_BRANCH_DEFAULT=${MY_BRANCH_DEFAULT}"
    echo "         MY_TRUNK_DIR=${MY_TRUNK_DIR}"
    echo "         MY_REVISIONS=${MY_REVISIONS}"
fi

#
# Resolve branch variables.
#
if test -z "${MY_BRANCH_DIR}" -a -z "${MY_BRANCH}"; then
    MY_BRANCH_DIR=${MY_BRANCH_DEFAULT_DIR}
    MY_BRANCH=${MY_BRANCH_DEFAULT}
elif test -n "${MY_BRANCH}" -a -z "${MY_BRANCH_DIR}"; then
    MY_BRANCH_DIR=${MY_BRANCH_DEFAULT_DIR}
elif test -z "${MY_BRANCH}" -a -n "${MY_BRANCH_DIR}"; then
    MY_BRANCH=`echo "${MY_BRANCH_DIR}" | "${SED}" -e 's/^-([1]*[5-9]\.[0-5])[/]*$/\1/'`
    if test -z "${MY_BRANCH}" -o  "${MY_BRANCH}" = "${MY_BRANCH_DIR}"; then
        echo "error: Failed to guess branch name for: ${MY_BRANCH_DIR}" >2
        echo "       Use --branch to specify it." >2
        exit 2;
    fi
fi
if test "${MY_BRANCH}" = "trunk"; then
    echo "error: script does not work with 'trunk' as the branch" >2
    exit 2;
fi

#
# Stop if no revisions specified.
#
if test -z "${MY_REVISIONS}"; then
    echo "error: No revisions specified" >&2;
    exit 2;
fi

