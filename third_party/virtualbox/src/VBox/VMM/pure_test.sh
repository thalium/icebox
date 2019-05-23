#!/bin/bash
# $Id: pure_test.sh $
## @file
# pure_test.sh - test the effect of __attribute__((pure)) on a set of
#                functions.
#
# Mark the functions with EXPERIMENT_PURE where the attribute normally would,
# go update this script so it points to the right header and execute it.  At
# the end you'll get a pt-report.txt showing the fluctuations in the text size.
#

#
# Copyright (C) 2010-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

set -e
set -x

BINDIR="../../../out/linux.amd64/release/bin/"
DLLEXT="so"
HEADER="../../../include/VBox/cpum.h"
REPORT="pt-report.txt"

test -e ${HEADER}.bak || kmk_cp $HEADER ${HEADER}.bak
NAMES=`kmk_sed -e '/EXPERIMENT_PURE/!d' -e '/^#/d' -e 's/^[^()]*([^()]*)[[:space:]]*\([^() ]*\)(.*$/\1/' ${HEADER}.bak `
echo NAMES=$NAMES


#
# baseline
#
kmk_sed -e 's/EXPERIMENT_PURE//' ${HEADER}.bak --output ${HEADER}
kmk KBUILD_TYPE=release VBoxVMM VMMR0 VMMGC
size ${BINDIR}/VMMR0.r0 ${BINDIR}/VMMGC.gc ${BINDIR}/VBoxVMM.${DLLEXT} > pt-baseline.txt

exec < "pt-baseline.txt"
read buf                                # ignore
read buf; baseline_r0=`echo $buf | kmk_sed -e 's/^[[:space:]]*\([^[[:space:]]*\).*$/\1/' `
read buf; baseline_rc=`echo $buf | kmk_sed -e 's/^[[:space:]]*\([^[[:space:]]*\).*$/\1/' `
read buf; baseline_r3=`echo $buf | kmk_sed -e 's/^[[:space:]]*\([^[[:space:]]*\).*$/\1/' `

kmk_cp -f "pt-baseline.txt" "${REPORT}"
kmk_printf -- "\n" >> "${REPORT}"
kmk_printf -- "%7s  %7s  %7s  Name\n" "VMMR0" "VMMGC" "VBoxVMM" >> "${REPORT}"
kmk_printf -- "-------------------------------\n" >> "${REPORT}"
kmk_printf -- "%7d  %7d  %7d  baseline\n" ${baseline_r0}  ${baseline_rc} ${baseline_r3} >> "${REPORT}"

#
# Now, do each of the names.
#
for name in $NAMES;
do
    kmk_sed \
        -e '/'"${name}"'/s/EXPERIMENT_PURE/__attribute__((pure))/' \
        -e 's/EXPERIMENT_PURE//' \
        ${HEADER}.bak --output ${HEADER}
    kmk KBUILD_TYPE=release VBoxVMM VMMR0 VMMGC
    size ${BINDIR}/VMMR0.r0 ${BINDIR}/VMMGC.gc ${BINDIR}/VBoxVMM.${DLLEXT} > "pt-${name}.txt"

    exec < "pt-${name}.txt"
    read buf                                # ignore
    read buf; cur_r0=`echo $buf | kmk_sed -e 's/^[[:space:]]*\([^[[:space:]]*\).*$/\1/' `
    read buf; cur_rc=`echo $buf | kmk_sed -e 's/^[[:space:]]*\([^[[:space:]]*\).*$/\1/' `
    read buf; cur_r3=`echo $buf | kmk_sed -e 's/^[[:space:]]*\([^[[:space:]]*\).*$/\1/' `
    kmk_printf -- "%7d  %7d  %7d  ${name}\n"  \
             `kmk_expr ${baseline_r0} - ${cur_r0} ` \
             `kmk_expr ${baseline_rc} - ${cur_rc} ` \
             `kmk_expr ${baseline_r3} - ${cur_r3} ` \
        >> "${REPORT}"
done

# clean up
kmk_mv -f ${HEADER}.bak ${HEADER}



