# $Id: kPrf2WinApi-genimp.sed 29 2009-07-01 20:30:29Z bird $
##
# Generate imports from normalized dumpbin output.
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

# Normalize the input a bit.
s/[[:space:]][[:space:]]*/ /g
s/^[[:space:]]//
s/[[:space:]]$//
/^$/b drop_line

# Expects a single name - no ordinals yet.
/\@/b have_at

s/^\(.*\)$/  \1=kPrf2Wrap_\1/
b end

:have_at
h
s/^\([^ ]\)\(@[0-9]*\)$/  \1\2=kPrf2Wrap_\1/
p
g
s/^\([^ ]\)\(@[0-9]*\)$/  \1=kPrf2Wrap_\1/
b end

:drop_line
d
b end

:end
