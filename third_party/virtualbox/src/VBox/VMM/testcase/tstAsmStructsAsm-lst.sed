# $Id: tstAsmStructsAsm-lst.sed $
## @file
# For testing assembly struct when using yasm.
#

#
# Copyright (C) 2006-2017 Oracle Corporation
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
# Strip stuff lines and spaces we don't care about.
#
/ %line /d
/\[section /d
/\[bits /d
/\[absolute /d
/ times /d
s/ *[[:digit:]]* //
/^ *$/d
s/ *$//g
s/^ *//g
/^\.text$/d
/^\.data$/d
/^\.bss$/d
s/[[:space:]][[:space:]]*/ /g

#
# Figure which type of line this is and process it accordingly.
#
/^[[:alpha:]_][[:alnum:]_]*:/b struct
/^[[:alpha:]_][[:alnum:]_]*_size EQU \$ - .*$/b struct_equ
/<gap>/b member
/^\.[[:alpha:]_][[:alnum:]_.:]* res.*$/b member_two
/^\.[[:alpha:]_][[:alnum:]_.:]*:$/b member_alias
b error
b member_two


#
# Struct start / end.
#
:struct_equ
s/_size EQU.*$/_size/
:struct
s/:$//
h
s/^/global /
s/$/ ; struct/
b end


#
# Struct member
# Note: the 't' command doesn't seem to be working right with 's'.
#
:member
s/[[:xdigit:]]* *//
s/<gap> *//
/^\.[[:alnum:]_.]*[:]* .*$/!t error
s/\(\.[[:alnum:]_]*\)[:]* .*$/\1 /
G
s/^\([^ ]*\) \(.*\)$/global \2\1 ; member/
s/\n//m

b end


#
# Struct member, no address. yasm r1842 and later.
#
:member_two
s/[:]*  *res[bwdtq] .*$//
s/$/ /
/^\.[[:alnum:]_.]* *$/!t error
G
s/^\([^ ]*\) \(.*\)$/global \2\1 ; member2/
s/\n//m

b end

#
# Alias member like Host.cr0Fpu in 64-bit.  Drop it.
#
:member_alias
d
b end

:error
s/^/\nSed script logic error!\nBuffer: /
s/$/\nHold: /
G
q 1
b end


:end

