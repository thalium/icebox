/** $Id: VBoxSFInternal.h $ */
/** @file
 * VBoxSF - OS/2 Shared Folder IFS, Internal Header.
 */

/*
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ___VBoxSFInternal_h___
#define ___VBoxSFInternal_h___


#define INCL_BASE
#define INCL_ERROR
#define INCL_LONGLONG
#include <os2.h>
#include <os2ddk/bsekee.h>
#include <os2ddk/devhlp.h>
#include <os2ddk/unikern.h>
#include <os2ddk/fsd.h>
#undef RT_MAX

#include <iprt/types.h>
#include <iprt/assert.h>


/**
 * VBoxSF Volume Parameter Structure.
 *
 * @remark  Overlays the 36 byte VPFSD structure (fsd.h).
 */
typedef struct VBOXSFVP
{
    uint32_t u32Dummy;
} VBOXSFVP;
AssertCompile(sizeof(VBOXSFVP) <= sizeof(VPFSD));
/** Pointer to a VBOXSFVP struct. */
typedef VBOXSFVP *PVBOXSFVP;


/**
 * VBoxSF Current Directory Structure.
 *
 * @remark  Overlays the 8 byte CDFSD structure (fsd.h).
 */
typedef struct VBOXSFCD
{
    uint32_t u32Dummy;
} VBOXSFCD;
AssertCompile(sizeof(VBOXSFCD) <= sizeof(CDFSD));
/** Pointer to a VBOXSFCD struct. */
typedef VBOXSFCD *PVBOXSFCD;


/**
 * VBoxSF System File Structure.
 *
 * @remark  Overlays the 30 byte SFFSD structure (fsd.h).
 */
typedef struct VBOXSFFSD
{
    /** Self pointer for quick 16:16 to flat translation. */
    struct VBOXSFFSD *pSelf;
} VBOXSFFSD;
AssertCompile(sizeof(VBOXSFFSD) <= sizeof(SFFSD));
/** Pointer to a VBOXSFFSD struct. */
typedef VBOXSFFSD *PVBOXSFFSD;


/**
 * VBoxSF File Search Structure.
 *
 * @remark  Overlays the 24 byte FSFSD structure (fsd.h).
 */
typedef struct VBOXSFFS
{
    /** Self pointer for quick 16:16 to flat translation. */
    struct VBOXSFFS *pSelf;
} VBOXSFFS;
AssertCompile(sizeof(VBOXSFFS) <= sizeof(FSFSD));
/** Pointer to a VBOXSFFS struct. */
typedef VBOXSFFS *PVBOXSFFS;


#endif

