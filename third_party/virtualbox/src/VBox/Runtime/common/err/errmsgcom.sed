# $Id: errmsgcom.sed $
## @file
# IPRT - SED script for converting COM errors
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
# The contents of this file may alternatively be used under the terms
# of the Common Development and Distribution License Version 1.0
# (CDDL) only, as it comes in the "COPYING.CDDL" file of the
# VirtualBox OSE distribution, in which case the provisions of the
# CDDL are applicable instead of those of the GPL.
#
# You may elect to license modified versions of this file under the
# terms and conditions of either the GPL or the CDDL or both.
#

# we only care about message definitions
/No symbolic name defined/b skip_it
\/\/ MessageId: /b messageid
d
b end


# Everything else is deleted!
:skip_it
d
b end


#
# A message ID we care about
#
:messageid
# concatenate the next four lines to the string
N
N
N
N
{
    # remove DOS <CR>.
    s/\r//g
    # remove the message ID
    s/\/\/ MessageId: //g
    # remove the stuff in between
    s/\/\/\n\/\/ MessageText:\n\/\/\n\/\/  *//g
    # backslashes have to be escaped
    s/\\/\\\\/g
    # double quotes have to be escaped, too
    s/"/\\"/g
    # output C array entry
    s/\([[:alnum:]_]*\)[\t ]*\n\(.*\)[\t ]*$/{ "\2", "\1", \1 }, /
}
b end

# next expression
:end
