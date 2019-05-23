/* $Id: VBoxPeSetVersion.cpp $ */
/** @file
 * IPRT - Change the OS and SubSystem version to 4.0 (VS2010 trick).
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/formats/mz.h>
#include <iprt/formats/pecoff.h>
#include <stdio.h>
#include <string.h>


/** @todo Rewrite this so it can take options and print out error messages. */
int main(int argc, char **argv)
{
    if (argc != 2)
        return 30;
    FILE *pFile = fopen(argv[1], "r+b");
    if (!pFile)
        return 1;
    IMAGE_DOS_HEADER MzHdr;
    if (fread(&MzHdr, sizeof(MzHdr), 1, pFile) != 1)
        return 2;

    if (fseek(pFile, MzHdr.e_lfanew, SEEK_SET) != 0)
        return 3;

    IMAGE_NT_HEADERS32 NtHdrs;
    if (fread(&NtHdrs, sizeof(NtHdrs), 1, pFile) != 1)
        return 4;
    if (NtHdrs.Signature != IMAGE_NT_SIGNATURE)
        return 5;
    if (NtHdrs.FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
        return 6;
    if (NtHdrs.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        return 7;

    if (NtHdrs.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        return 7;

    IMAGE_NT_HEADERS32 NtHdrsNew = NtHdrs;
    if (NtHdrsNew.OptionalHeader.MajorOperatingSystemVersion > 4)
    {
        NtHdrsNew.OptionalHeader.MajorOperatingSystemVersion = 4;
        NtHdrsNew.OptionalHeader.MinorOperatingSystemVersion = 0;
    }
    if (NtHdrsNew.OptionalHeader.MajorSubsystemVersion > 4)
    {
        NtHdrsNew.OptionalHeader.MajorSubsystemVersion = 4;
        NtHdrsNew.OptionalHeader.MinorSubsystemVersion = 0;
    }

    if (memcmp(&NtHdrsNew, &NtHdrs, sizeof(NtHdrs)))
    {
        /** @todo calc checksum. */
        NtHdrsNew.OptionalHeader.CheckSum = 0;

        if (fseek(pFile, MzHdr.e_lfanew, SEEK_SET) != 0)
            return 10;
        if (fwrite(&NtHdrsNew, sizeof(NtHdrsNew), 1, pFile) != 1)
            return 11;
    }

    if (fclose(pFile) != 0)
        return 29;
    return 0;
}

