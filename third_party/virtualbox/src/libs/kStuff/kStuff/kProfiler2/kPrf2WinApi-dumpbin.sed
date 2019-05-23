# $Id: kPrf2WinApi-dumpbin.sed 29 2009-07-01 20:30:29Z bird $
## @file
# Strip down dumpbin /export output.
#

#
# Copyright (c) 2008 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#

#
# State switch
#
x
/^exports$/b exports
/^summary$/b summary
b header

#
# Header
#
:header
x
/^[[:space:]][[:space:]]*ordinal[[:space:]]*name[[:space:]]*$/b switch_to_exports
b drop_line

#
# Exports
#
:switch_to_exports
s/^.*$/exports/
h
b drop_line

:exports
x
/^[[:space:]][[:space:]]*Summary[[:space:]]*$/b switch_to_summary
s/^[[:space:]]*//
s/[[:space:]]*$//
s/[[:space:]][[:space:]]*/ /g
/^$/b drop_line

# Filter out APIs that hasn't been implemented.
/AddLocalAlternateComputerNameA/b drop_line
/AddLocalAlternateComputerNameW/b drop_line
/EnumerateLocalComputerNamesA/b drop_line
/EnumerateLocalComputerNamesW/b drop_line
/RemoveLocalAlternateComputerNameA/b drop_line
/RemoveLocalAlternateComputerNameW/b drop_line
/SetLocalPrimaryComputerNameA/b drop_line
/SetLocalPrimaryComputerNameW/b drop_line
/__C_specific_handler/b drop_line
/__misaligned_access/b drop_line
/_local_unwind/b drop_line

b end

#
# Summary
#
:switch_to_summary
s/^.*$/summary/
h
b drop_line

:summary
x
b drop_line

#
# Tail
#
:drop_line
d
:end

