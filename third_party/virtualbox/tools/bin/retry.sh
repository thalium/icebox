# !kmk_ash
# $Id: retry.sh $
## @file
# Script for retrying a command 5 times.
#

#
# Copyright (C) 2009-2016 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

# First try (quiet).
"$@"
EXITCODE=$?
if [ "${EXITCODE}" = "0" ]; then
    exit 0;
fi

# Retries.
for RETRY in 2 3 4 5;
do
    echo "retry.sh: retry$((${RETRY} - 1)): exitcode=${EXITCODE};  retrying: $*"
    "$@"
    EXITCODE=$?
    if [ "${EXITCODE}" = "0" ]; then
        echo "retry.sh: Success after ${RETRY} tries: $*!"
        exit 0;
    fi
done
echo "retry.sh: Giving up: exitcode=${EXITCODE}  command: $@" 
exit ${EXITCODE};

