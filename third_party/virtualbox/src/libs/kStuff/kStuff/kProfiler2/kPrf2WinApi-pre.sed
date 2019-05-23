# $Id: kPrf2WinApi-pre.sed 29 2009-07-01 20:30:29Z bird $
## @file
# This SED script will try normalize a windows header
# in order to make it easy to pick out function prototypes.
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


# Drop all preprocessor lines (#if/#else/#endif/#define/#undef/#pragma/comments)
# (we don't bother with multi line comments ATM.)
/^[[:space:]]*#/b drop_line
/^[[:space:]]*\/\//b drop_line

# Drop empty lines.
/^[[:space:]]*$/b drop_line

# Drop trailing comments and trailing whitespace
s/[[:space:]][[:space:]]*\/\.*$//g
s,[[:space:]][[:space:]]*/\*[^*/]*\*/[[:space:]]*$,,g
s/[[:space:]][[:space:]]*$//g

# Pick out the WINBASEAPI stuff (WinBase.h)
/^WINBASEAPI/b winapi
/^NTSYSAPI/b winapi
/^WINAPI$/b winapi_perhaps
/^APIENTRY$/b winapi_perhaps
h
d
b end

# No WINBASEAPI, so we'll have to carefully check the hold buffer.
:winapi_perhaps
x
/^[A-Z][A-Z0-9_][A-Z0-9_]*[A-Z0-9]$/!b drop_line
G
s/\r/ /g
s/\n/ /g
b winapi

# Make it one line and a bit standardized
:winapi
/;/b winapi_got_it
N
b winapi
:winapi_got_it
s/\n/ /g
s/[[:space:]][[:space:]]*\/\*[^*/]*\*\/[[:space:]]*//g
s/[[:space:]][[:space:]]*(/(/g
s/)[[:space:]][[:space:]]*/)/g
s/(\([^[:space:]]\)/( \1/g
s/\([^[:space:]]\))/\1 )/g
s/[*]\([^[:space:]]\)/* \1/g
s/\([^[:space:]]\)[*]/\1 */g
s/[[:space:]][[:space:]]*/ /g
s/[[:space:]][[:space:]]*,/,/g
s/,/, /g
s/,[[:space:]][[:space:]]*/, /g

# Drop the nasty bit of the sal.h / SpecString.h stuff.
s/[[:space:]]__[a-z][a-z_]*([^()]*)[[:space:]]*/ /g
s/[[:space:]]__out[a-z_]*[[:space:]]*/ /g
s/[[:space:]]__in[a-z_]*[[:space:]]*/ /g
s/[[:space:]]__deref[a-z_]*[[:space:]]*/ /g
s/[[:space:]]__reserved[[:space:]]*/ /g
s/[[:space:]]__nullnullterminated[[:space:]]*/ /g
s/[[:space:]]__checkReturn[[:space:]]*/ /g

# Drop some similar stuff.
s/[[:space:]]OPTIONAL[[:space:]]/ /g
s/[[:space:]]OPTIONAL,/ ,/g

# The __declspec() bit isn't necessary
s/WINBASEAPI *//
s/NTSYSAPI *//
s/DECLSPEC_NORETURN *//
s/__declspec([^()]*) *//

# Normalize spaces.
s/[[:space:]]/ /g

# Clear the hold space
x
s/^.*$//
x
b end

:drop_line
s/^.*$//
h
d

:end

