# $Id: kPrf2WinApi-gencode.sed 29 2009-07-01 20:30:29Z bird $
## @file
# Generate code (for kernel32).
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

# Example:
#       BOOL WINAPI FindActCtxSectionGuid( DWORD dwFlags, const GUID * lpExtensionGuid, ULONG ulSectionId, const GUID * lpGuidToFind, PACTCTX_SECTION_KEYED_DATA ReturnedData );
#
# Should be turned into:
#       typedef BOOL WINAPI FN_FindActCtxSectionGuid( DWORD dwFlags, const GUID * lpExtensionGuid, ULONG ulSectionId, const GUID * lpGuidToFind, PACTCTX_SECTION_KEYED_DATA ReturnedData );
#       __declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindActCtxSectionGuid( DWORD dwFlags, const GUID * lpExtensionGuid, ULONG ulSectionId, const GUID * lpGuidToFind, PACTCTX_SECTION_KEYED_DATA ReturnedData )
#       {
#           static FN_FindActCtxSectionGuid *pfn = 0;
#           if (!pfn)
#               kPrfWrapResolve((void **)&pfn, "FindActCtxSectionGuid", &g_Kernel32);
#           return pfn( dwFlags, lpExtensionGuid, ulSectionId, lpGuidToFind, ReturnedData );
#       }
#

# Ignore empty lines.
/^[[:space:]]*$/b delete

# Some hacks.
/([[:space:]]*VOID[[:space:]]*)/b no_hacking_void
s/([[:space:]]*\([A-Z][A-Z0-9_]*\)[[:space:]]*)/( \1 a)/
:no_hacking_void


# Save the pattern space.
h

# Make the typedef.
s/[[:space:]]\([A-Za-z_][A-Za-z0-9_]*\)(/ FN_\1(/
s/^/typedef /
p

# Function definition
g
s/\n//g
s/\r//g
s/[[:space:]]\([A-Za-z_][A-Za-z0-9_]*\)(/ kPrf2Wrap_\1(/
s/^/__declspec(dllexport) /
s/;//
p
i\
{

#     static FN_FindActCtxSectionGuid *pfn = 0;
#     if (!pfn)
g
s/^.*[[:space:]]\([A-Za-z_][A-Za-z0-9_]*\)(.*$/    static FN_\1 *pfn = 0;/
p
i\
    if (!pfn)

#       kPrfWrapResolve((void **)&pfn, "FindActCtxSectionGuid", &g_Kernel32);
g
s/^.*[[:space:]]\([A-Za-z_][A-Za-z0-9_]*\)(.*$/        kPrf2WrapResolve((void **)\&pfn, "\1\", \&g_Kernel32);/
p

#     The invocation and return statement.
#     Some trouble here....
g
/^VOID WINAPI/b void_return
/^void WINAPI/b void_return
/^VOID __cdecl/b void_return
/^void __cdecl/b void_return
/^VOID NTAPI/b void_return
/^void NTAPI/b void_return
s/^.*(/    return pfn(/
b morph_params

:void_return
s/^.*(/    pfn(/

:morph_params
s/ *\[\] *//
s/ \*/ /g
s/, *[a-zA-Z_][^,)]* \([a-zA-Z_][a-zA-Z_0-9]* *)\)/, \1/g
s/( *[a-zA-Z_][^,)]* \([a-zA-Z_][a-zA-Z_0-9]* *[,)]\)/( \1/g
s/, *[a-zA-Z_][^,)]* \([a-zA-Z_][a-zA-Z_0-9]* *,\)/, \1/g
s/, *[a-zA-Z_][^,)]* \([a-zA-Z_][a-zA-Z_0-9]* *,\)/, \1/g
s/, *[a-zA-Z_][^,)]* \([a-zA-Z_][a-zA-Z_0-9]* *,\)/, \1/g
s/, *[a-zA-Z_][^,)]* \([a-zA-Z_][a-zA-Z_0-9]* *,\)/, \1/g
s/( VOID )/ ()/
s/( void )/ ()/
p
i\
}
i\

# Done
:delete
d

