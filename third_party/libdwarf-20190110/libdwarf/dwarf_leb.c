/*
  Copyright (C) 2000,2004 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2011-2018 David Anderson. All Rights Reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2.1 of the GNU Lesser General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it would be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Further, this software is distributed without any warranty that it is
  free of the rightful claim of any third person regarding infringement
  or the like.  Any license provided herein, whether implied or
  otherwise, applies only to this software file.  Patent licenses, if
  any, provided herein do not apply to combinations of this program with
  other software, or any other product whatsoever.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, write the Free Software
  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston MA 02110-1301,
  USA.

*/


#include "config.h"
#include <stdio.h>
#include "dwarf_incl.h"
#include "dwarf_error.h"
#include "dwarf_util.h"
#ifdef TESTING
#include "pro_encode_nm.h"
#endif

/*  Note that with -DTESTING ('make tests')
    many of the test items
    only make sense if Dwarf_Unsigned (and Dwarf_Signed)
    are 64 bits.  The encode/decode logic should
    be fine whether those types are 64 or 32 bits. */

/*  10 bytes of leb, 7 bits each part of the number, gives
    room for a 64bit number.
    While any number of leading zeroes would be legal, so
    no max is really truly required here, why would a
    compiler generate leading zeros?  That would
    be strange.
*/
#define BYTESLEBMAX 10
#define BITSPERBYTE 8


/* Decode ULEB with checking */
int
_dwarf_decode_u_leb128_chk(Dwarf_Small * leb128,
    Dwarf_Word * leb128_length,
    Dwarf_Unsigned *outval,
    Dwarf_Byte_Ptr endptr)
{
    Dwarf_Unsigned byte     = 0;
    Dwarf_Word word_number = 0;
    Dwarf_Unsigned number  = 0;
    unsigned shift      = 0;
    /*  The byte_length value will be a small non-negative integer. */
    unsigned byte_length   = 0;

    if (leb128 >=endptr) {
        return DW_DLV_ERROR;
    }
    /*  The following unrolls-the-loop for the first two bytes and
        unpacks into 32 bits to make this as fast as possible.
        word_number is assumed big enough that the shift has a defined
        result. */
    if ((*leb128 & 0x80) == 0) {
        if (leb128_length) {
            *leb128_length = 1;
        }
        *outval = *leb128;
        return DW_DLV_OK;
    } else {
        if ((leb128+1) >=endptr) {
            return DW_DLV_ERROR;
        }
        if ((*(leb128 + 1) & 0x80) == 0) {
            if (leb128_length) {
                *leb128_length = 2;
            }
            word_number = *leb128 & 0x7f;
            word_number |= (*(leb128 + 1) & 0x7f) << 7;
            *outval = word_number;
            return DW_DLV_OK;
        }
        /* Gets messy to hand-inline more byte checking. */
    }

    /*  The rest handles long numbers Because the 'number' may be larger
        than the default int/unsigned, we must cast the 'byte' before
        the shift for the shift to have a defined result. */
    number = 0;
    shift = 0;
    byte_length = 1;
    byte = *leb128;
    for (;;) {
        if (shift >= (sizeof(number)*BITSPERBYTE)) {
            return DW_DLV_ERROR;
        }
        number |= (byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) {
            if (leb128_length) {
                *leb128_length = byte_length;
            }
            *outval = number;
            return DW_DLV_OK;
        }
        shift += 7;
        byte_length++;
        if (byte_length > BYTESLEBMAX) {
            /*  Erroneous input.  */
            if( leb128_length) {
                *leb128_length = BYTESLEBMAX;
            }
            break;
        }
        ++leb128;
        if ((leb128) >=endptr) {
            return DW_DLV_ERROR;
        }
        byte = *leb128;
    }
    return DW_DLV_ERROR;
}


#define BITSINBYTE 8

int
_dwarf_decode_s_leb128_chk(Dwarf_Small * leb128, Dwarf_Word * leb128_length,
    Dwarf_Signed *outval,Dwarf_Byte_Ptr endptr)
{
    Dwarf_Unsigned byte   = 0;
    Dwarf_Signed number  = 0;
    Dwarf_Bool sign      = 0;
    Dwarf_Word shift     = 0;
    /*  The byte_length value will be a small non-negative integer. */
    unsigned byte_length = 1;

    /*  byte_length being the number of bytes of data absorbed so far in
        turning the leb into a Dwarf_Signed. */
    if (!outval) {
        return DW_DLV_ERROR;
    }
    if (leb128 >= endptr) {
        return DW_DLV_ERROR;
    }
    byte   = *leb128;
    for (;;) {
        sign = byte & 0x40;
        if (shift >= (sizeof(number)*BITSPERBYTE)) {
            return DW_DLV_ERROR;
        }
        number |= ((Dwarf_Unsigned) ((byte & 0x7f))) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            break;
        }
        ++leb128;
        if (leb128 >= endptr) {
            return DW_DLV_ERROR;
        }
        byte = *leb128;
        byte_length++;
        if (byte_length > BYTESLEBMAX) {
            /*  Erroneous input. */
            if (leb128_length) {
                *leb128_length = BYTESLEBMAX;
            }
            return DW_DLV_ERROR;
        }
    }

    if (sign) {
        /* The following avoids undefined behavior. */
        unsigned shiftlim = sizeof(Dwarf_Signed) * BITSINBYTE -1;
        if (shift < shiftlim) {
            number |= -(Dwarf_Signed)(((Dwarf_Unsigned)1) << shift);
        } else if (shift == shiftlim) {
            number |= (((Dwarf_Unsigned)1) << shift);
        }
    }

    if (leb128_length) {
        *leb128_length = byte_length;
    }
    *outval = number;
    return DW_DLV_OK;
}

#ifdef TESTING

static void
printinteresting(void)
{
    return;
}

static Dwarf_Signed stest[] = {
0,0xff,
0x800000000000002f,
0x800000000000003f,
0x800000000000004f,
0x8000000000000070,
0x800000000000007f,
0x8000000000000080,
0x8000000000000000,
0x800000ffffffffff,
0x80000000ffffffff,
0x800000ffffffffff,
0x8000ffffffffffff,
0xffffffffffffffff,
-1703944 /*18446744073707847672 as signed*/,
562949951588368,
-1,
-127,
-100000,
-2000000000,
-4000000000,
-8000000000,
-800000000000,
};
static Dwarf_Unsigned utest[] = {
0,0xff,0x7f,0x80,
0x800000000000002f,
0x800000000000003f,
0x800000000000004f,
0x8000000000000070,
0x800000000000007f,
0x8000000000000080,
0x800000ffffffffff,
0x80000000ffffffff,
0x800000ffffffffff,
0x8000ffffffffffff,
9223372036854775808ULL,
-1703944 /*18446744073707847672 as signed*/,
562949951588368,
0xffff,
0xffffff,
0xffffffff,
0xffffffffff,
0xffffffffffff,
0xffffffffffffff,
0xffffffffffffffff
};


#if 0 /* FOR DEBUGGING */
static void
dump_encoded(char *space,int len)
{
    int t;

    printf("encode len %d: ",len);
    for ( t = 0; t < len; ++t) {
        printf("%02x",space[t] & 0xff);
    }
    printf("\n");
}
#endif


#define BUFFERLEN 100


static unsigned
signedtest(unsigned len)
{
    unsigned errcnt = 0;
    unsigned t = 0;
    char bufferspace[BUFFERLEN];

    for ( ; t < len; ++t) {
        int res = 0;
        int encodelen = 0;
        Dwarf_Word decodelen = 0;
        Dwarf_Signed decodeval = 0;

        res = _dwarf_pro_encode_signed_leb128_nm(
            stest[t],&encodelen,bufferspace,BUFFERLEN);
        if (res != DW_DLV_OK) {
            printf("FAIL signed encode index %u val 0x%llx\n",
                t,stest[t]);
            ++errcnt;
        }
        res = _dwarf_decode_s_leb128_chk(
            (Dwarf_Small *)bufferspace,
            &decodelen,
            &decodeval,
            (Dwarf_Byte_Ptr)(&bufferspace[BUFFERLEN-1]));
        if (res != DW_DLV_OK) {
            printf("FAIL signed decode index %u val 0x%llx\n",
                t,stest[t]);
            ++errcnt;
        }
        if (stest[t] != decodeval) {
            printf("FAIL signed decode val index %u val 0x%llx vs 0x%llx\n",
                t,stest[t],decodeval);
            ++errcnt;
        }
        if ((Dwarf_Word)encodelen != decodelen) {
            printf("FAIL signed decodelen val index %u val 0x%llx\n",
                t,stest[t]);
            ++errcnt;
        }
    }
    return errcnt;
}

static  unsigned
unsignedtest(unsigned len)
{
    unsigned errcnt = 0;
    unsigned t = 0;
    char bufferspace[BUFFERLEN];

    for ( ; t < len; ++t) {
        int res = 0;
        int encodelen = 0;
        Dwarf_Word decodelen = 0;
        Dwarf_Unsigned decodeval = 0;

        res = _dwarf_pro_encode_leb128_nm(
            utest[t],&encodelen,bufferspace,BUFFERLEN);
        if (res != DW_DLV_OK) {
            printf("FAIL signed encode index %u val 0x%llx",
                t,utest[t]);
            ++errcnt;
        }
        res = _dwarf_decode_u_leb128_chk(
            (Dwarf_Small *)bufferspace,
            &decodelen,
            &decodeval,
            (Dwarf_Byte_Ptr)(&bufferspace[BUFFERLEN-1]));
        if (res != DW_DLV_OK) {
            printf("FAIL signed decode index %u val 0x%llx\n",
                t,utest[t]);
            ++errcnt;
        }
        if (utest[t] != decodeval) {
            printf("FAIL signed decode val index %u val 0x%llx vs 0x%llx\n",
                t,utest[t],decodeval);
            ++errcnt;
        }
        if ((Dwarf_Word)encodelen != decodelen) {
            printf("FAIL signed decodelen val index %u val 0x%llx\n",
                t,utest[t]);
            ++errcnt;
        }
    }
    return errcnt;
}
static unsigned char v1[] = {
0x90, 0x90, 0x90,
0x90, 0x90, 0x90,
0x90, 0x90, 0x90,
0x90, 0x90, 0x90,
0x90 };

static unsigned char v2[] = {
0xf4,0xff,
0xff,
0xff,
0x0f,
0x4c,
0x00,
0x00,
0x00};

/*   9223372036854775808 == -9223372036854775808 */
static unsigned char v3[] = {
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x41 };


/*  This warning with --enable-sanitize is fixed
    as of November 11, 2016 when decoding test v4.
    dwarf_leb.c: runtime error: negation of -9223372036854775808 cannot be
    represented in type 'Dwarf_Signed' (aka 'long long');
    cast to an unsigned type to negate this value to itself.
    The actual value here is -4611686018427387904 0xc000000000000000 */
static unsigned char v4[] = {
0x80, 0x80, 0x80,
0x80, 0x80, 0x80,
0x80, 0x80, 0x40 };


static unsigned
specialtests(void)
{
    unsigned errcnt = 0;
    unsigned vlen = 0;
    Dwarf_Word decodelen = 0;
    Dwarf_Signed decodeval = 0;
    Dwarf_Unsigned udecodeval = 0;
    int res;

    vlen = sizeof(v1)/sizeof(char);
    res = _dwarf_decode_s_leb128_chk(
        (Dwarf_Small *)v1,
        &decodelen,
        &decodeval,
        (Dwarf_Byte_Ptr)(&v1[vlen]));
    if (res != DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v1 \n");
        ++errcnt;
    }
    res = _dwarf_decode_u_leb128_chk(
        (Dwarf_Small *)v1,
        &decodelen,
        &udecodeval,
        (Dwarf_Byte_Ptr)(&v1[vlen]));
    if (res != DW_DLV_ERROR) {
        printf("FAIL unsigned decode special v1 \n");
        ++errcnt;
    }

    vlen = sizeof(v2)/sizeof(char);
    res = _dwarf_decode_s_leb128_chk(
        (Dwarf_Small *)v2,
        &decodelen,
        &decodeval,
        (Dwarf_Byte_Ptr)(&v2[vlen]));
    if (res != DW_DLV_OK) {
        printf("FAIL signed decode special v2 \n");
        ++errcnt;
    }
    /*  If you just do (byte & 0x7f) << shift
        and byte is (or is promoted to) a signed type
        on the following decode you get the wrong value.
        Undefined effect in C leads to error.  */
    res = _dwarf_decode_u_leb128_chk(
        (Dwarf_Small *)v2,
        &decodelen,
        &udecodeval,
        (Dwarf_Byte_Ptr)(&v2[vlen]));
    if (res != DW_DLV_OK) {
        printf("FAIL unsigned decode special v2 \n");
        ++errcnt;
    }

    vlen = sizeof(v3)/sizeof(char);
    res = _dwarf_decode_s_leb128_chk(
        (Dwarf_Small *)v3,
        &decodelen,
        &decodeval,
        (Dwarf_Byte_Ptr)(&v3[vlen]));
    if (res != DW_DLV_OK) {
        printf("FAIL signed decode special v3 \n");
        ++errcnt;
    }
    if ((Dwarf_Unsigned)decodeval !=
        (Dwarf_Unsigned)0x8000000000000000) {
        printf("FAIL signed decode special v3 value check %lld vs %lld \n",
            decodeval,(Dwarf_Signed)0x8000000000000000);
        ++errcnt;
    }

    vlen = sizeof(v4)/sizeof(char);
    res = _dwarf_decode_s_leb128_chk(
        (Dwarf_Small *)v4,
        &decodelen,
        &decodeval,
        (Dwarf_Byte_Ptr)(&v4[vlen]));
    if (res != DW_DLV_OK) {
        printf("FAIL signed decode special v4 \n");
        ++errcnt;
    }
    if (decodeval != -4611686018427387904) {
        printf("FAIL signed decode special v4 value check %lld vs %lld \n",
            decodeval,-4611686018427387904LL);
        printf("FAIL signed decode special v4 value check 0x%llx vs 0x%llx \n",
            decodeval,-4611686018427387904LL);
        ++errcnt;
    }

    return errcnt;

    return errcnt;
}

int main(void)
{
    unsigned slen = sizeof(stest)/sizeof(Dwarf_Signed);
    unsigned ulen = sizeof(utest)/sizeof(Dwarf_Unsigned);
    int errs = 0;

    printinteresting();
    errs += signedtest(slen);

    errs += unsignedtest(ulen);

    errs += specialtests();

    if (errs) {
        printf("FAIL. leb encode/decode errors\n");
        return 1;
    }
    printf("PASS leb tests\n");
    return 0;
}
#endif /* TESTING */
