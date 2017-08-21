/*
    MIT License

    Copyright (c) 2015 Nicolas Couffin ncouffin@gmail.com

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#ifndef __WINDOWSPROFILS_H_
#define __WINDOWSPROFILS_H_

#pragma pack(push)
typedef struct _WINDOWS_PROFIL_T_ {
    char        *pGUID;
    char        *pVersionName;
    uint64_t    KiWaitAlwaysOffset;
    uint64_t    KiWaitNeverOffset;
    uint64_t    KdpDataBlockEncodedOffset;
    uint64_t    KdDebuggerDataBlockOffset;
    uint64_t    KdVersionBlockOffset;
    uint64_t    KiDivideErrorFaultOffset;
    uint64_t    KiTrap00Offset;
    uint64_t    KdpDebuggerDataListHeadOffset;
    bool        bClearKdDebuggerDataBlock;
    bool        bIsX86;
}WINDOWS_PROFIL_T;
#pragma pack(pop)

#endif //__WINDOWSPROFILS_H_