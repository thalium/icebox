# $Id: deftoimp.sed $
## @file
# SED script for generating a dummy .c from a windows .def file.
#

#
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
# Remove comments and space. Skip empty lines.
#
s/;.*$//g
s/^[[:space:]][[:space:]]*//g
s/[[:space:]][[:space:]]*$//g
/^$/d

# Handle text after EXPORTS
/EXPORTS/,//{
s/^EXPORTS$//
/^$/b end


/[[:space:]]DATA$/b data

#
# Function export
#
:code
s/^\(.*\)$/EXPORT\nvoid \1(void);\nvoid \1(void){}/
b end


#
# Data export
#
:data
s/^\(.*\)[[:space:]]*DATA$/EXPORT_DATA void *\1 = (void *)0;/
b end

}
d
b end


# next expression
:end

