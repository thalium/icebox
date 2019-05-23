#!/bin/sh

#
# Copyright (C) 2006-2015 Oracle Corporation
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
# Compare undefined symbols in a shared or static object against a new-line
# separated list of grep patterns in a set of text files and complain if
# symbols are found which aren't in the files.
#
# Usage: /bin/sh <script name> <object> [--static] <undefined symbol file...>
#
# Currently only works for native objects on Linux (and Solaris?) platforms.
#

echoerr()
{
  echo $* 1>&2
}

hostos="${1}"
target="${2}"
shift 2
if test "${1}" = "--static"; then
    static="${1}"
    shift
fi

if test $# -lt 1; then
    echoerr "${0}: Wrong number of arguments"
    args_ok="no"
fi
if test ! -r "${target}"; then
    echoerr "${0}: '${target}' not readable"
    args_ok="no"
fi
for i in "${@}"; do
    if test ! -r "${i}"; then
        echoerr "${0}: '${i}' not readable"
        args_ok="no"
    fi
done

if test "$args_ok" = "no"; then
  echoerr "Usage: $0 <object> [--static] <undefined symbol file...>"
  exit 1
fi

if test "$hostos" = "solaris"; then
    objdumpbin=/usr/sfw/bin/gobjdump
    grepbin=/usr/sfw/bin/ggrep
elif test "$hostos" = "linux"; then
    objdumpbin=`which objdump`
    grepbin=`which grep`
else
    echoerr "$0: '$hostos' not a valid hostos string. supported 'linux' 'solaris'"
    exit 1
fi

command="-T"
if test "$static" = "--static"; then
  command="-t"
fi

if test ! -x "${objdumpbin}"; then
    echoerr "${0}: '${objdumpbin}' not found or not executable."
    exit 1
fi

undefined=`"${objdumpbin}" ${command} "${target}" | kmk_sed -n 's/.*\*UND\*.*\s\([:graph:]*\)/\1/p'`
for i in "${@}"; do
    undefined=`echo "${undefined}" | "${grepbin}" -w -v -f "${i}"`
done
num_undef=`echo $undefined | wc -w`

if test $num_undef -ne 0; then
  echoerr "${0}: following symbols not defined in the files ${@}:"
  echoerr "${undefined}"
  exit 1
fi
# Return code
exit 0

