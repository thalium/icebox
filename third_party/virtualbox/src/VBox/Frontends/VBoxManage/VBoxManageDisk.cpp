/* $Id: VBoxManageDisk.cpp $ */
/** @file
 * VBoxManage - The disk/medium related commands.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_ONLY_DOCS


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/com/com.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/asm.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/param.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/ctype.h>
#include <iprt/getopt.h>
#include <VBox/log.h>
#include <VBox/vd.h>

#include "VBoxManage.h"
using namespace com;


// funcs
///////////////////////////////////////////////////////////////////////////////


static DECLCALLBACK(void) handleVDError(void *pvUser, int rc, RT_SRC_POS_DECL, const char *pszFormat, va_list va)
{
    RT_NOREF(pvUser);
    RTMsgErrorV(pszFormat, va);
    RTMsgError("Error code %Rrc at %s(%u) in function %s", rc, RT_SRC_POS_ARGS);
}

static int parseMediumVariant(const char *psz, MediumVariant_T *pMediumVariant)
{
    int rc = VINF_SUCCESS;
    unsigned uMediumVariant = (unsigned)(*pMediumVariant);
    while (psz && *psz && RT_SUCCESS(rc))
    {
        size_t len;
        const char *pszComma = strchr(psz, ',');
        if (pszComma)
            len = pszComma - psz;
        else
            len = strlen(psz);
        if (len > 0)
        {
            // Parsing is intentionally inconsistent: "standard" resets the
            // variant, whereas the other flags are cumulative.
            if (!RTStrNICmp(psz, "standard", len))
                uMediumVariant = MediumVariant_Standard;
            else if (   !RTStrNICmp(psz, "fixed", len)
                     || !RTStrNICmp(psz, "static", len))
                uMediumVariant |= MediumVariant_Fixed;
            else if (!RTStrNICmp(psz, "Diff", len))
                uMediumVariant |= MediumVariant_Diff;
            else if (!RTStrNICmp(psz, "split2g", len))
                uMediumVariant |= MediumVariant_VmdkSplit2G;
            else if (   !RTStrNICmp(psz, "stream", len)
                     || !RTStrNICmp(psz, "streamoptimized", len))
                uMediumVariant |= MediumVariant_VmdkStreamOptimized;
            else if (!RTStrNICmp(psz, "esx", len))
                uMediumVariant |= MediumVariant_VmdkESX;
            else
                rc = VERR_PARSE_ERROR;
        }
        if (pszComma)
            psz += len + 1;
        else
            psz += len;
    }

    if (RT_SUCCESS(rc))
        *pMediumVariant = (MediumVariant_T)uMediumVariant;
    return rc;
}

int parseMediumType(const char *psz, MediumType_T *penmMediumType)
{
    int rc = VINF_SUCCESS;
    MediumType_T enmMediumType = MediumType_Normal;
    if (!RTStrICmp(psz, "normal"))
        enmMediumType = MediumType_Normal;
    else if (!RTStrICmp(psz, "immutable"))
        enmMediumType = MediumType_Immutable;
    else if (!RTStrICmp(psz, "writethrough"))
        enmMediumType = MediumType_Writethrough;
    else if (!RTStrICmp(psz, "shareable"))
        enmMediumType = MediumType_Shareable;
    else if (!RTStrICmp(psz, "readonly"))
        enmMediumType = MediumType_Readonly;
    else if (!RTStrICmp(psz, "multiattach"))
        enmMediumType = MediumType_MultiAttach;
    else
        rc = VERR_PARSE_ERROR;

    if (RT_SUCCESS(rc))
        *penmMediumType = enmMediumType;
    return rc;
}

/** @todo move this into getopt, as getting bool values is generic */
int parseBool(const char *psz, bool *pb)
{
    int rc = VINF_SUCCESS;
    if (    !RTStrICmp(psz, "on")
        ||  !RTStrICmp(psz, "yes")
        ||  !RTStrICmp(psz, "true")
        ||  !RTStrICmp(psz, "1")
        ||  !RTStrICmp(psz, "enable")
        ||  !RTStrICmp(psz, "enabled"))
    {
        *pb = true;
    }
    else if (   !RTStrICmp(psz, "off")
             || !RTStrICmp(psz, "no")
             || !RTStrICmp(psz, "false")
             || !RTStrICmp(psz, "0")
             || !RTStrICmp(psz, "disable")
             || !RTStrICmp(psz, "disabled"))
    {
        *pb = false;
    }
    else
        rc = VERR_PARSE_ERROR;

    return rc;
}

HRESULT openMedium(HandlerArg *a, const char *pszFilenameOrUuid,
                   DeviceType_T enmDevType, AccessMode_T enmAccessMode,
                   ComPtr<IMedium> &pMedium, bool fForceNewUuidOnOpen,
                   bool fSilent)
{
    HRESULT rc;
    Guid id(pszFilenameOrUuid);
    char szFilenameAbs[RTPATH_MAX] = "";

    /* If it is no UUID, convert the filename to an absolute one. */
    if (!id.isValid())
    {
        int irc = RTPathAbs(pszFilenameOrUuid, szFilenameAbs, sizeof(szFilenameAbs));
        if (RT_FAILURE(irc))
        {
            if (!fSilent)
                RTMsgError("Cannot convert filename \"%s\" to absolute path", pszFilenameOrUuid);
            return E_FAIL;
        }
        pszFilenameOrUuid = szFilenameAbs;
    }

    if (!fSilent)
        CHECK_ERROR(a->virtualBox, OpenMedium(Bstr(pszFilenameOrUuid).raw(),
                                              enmDevType,
                                              enmAccessMode,
                                              fForceNewUuidOnOpen,
                                              pMedium.asOutParam()));
    else
        rc = a->virtualBox->OpenMedium(Bstr(pszFilenameOrUuid).raw(),
                                       enmDevType,
                                       enmAccessMode,
                                       fForceNewUuidOnOpen,
                                       pMedium.asOutParam());

    return rc;
}

static HRESULT createMedium(HandlerArg *a, const char *pszFormat,
                            const char *pszFilename, DeviceType_T enmDevType,
                            AccessMode_T enmAccessMode, ComPtr<IMedium> &pMedium)
{
    HRESULT rc;
    char szFilenameAbs[RTPATH_MAX] = "";

    /** @todo laziness shortcut. should really check the MediumFormatCapabilities */
    if (RTStrICmp(pszFormat, "iSCSI"))
    {
        int irc = RTPathAbs(pszFilename, szFilenameAbs, sizeof(szFilenameAbs));
        if (RT_FAILURE(irc))
        {
            RTMsgError("Cannot convert filename \"%s\" to absolute path", pszFilename);
            return E_FAIL;
        }
        pszFilename = szFilenameAbs;
    }

    CHECK_ERROR(a->virtualBox, CreateMedium(Bstr(pszFormat).raw(),
                                            Bstr(pszFilename).raw(),
                                            enmAccessMode,
                                            enmDevType,
                                            pMedium.asOutParam()));
    return rc;
}

static const RTGETOPTDEF g_aCreateMediumOptions[] =
{
    { "disk",           'H', RTGETOPT_REQ_NOTHING },
    { "dvd",            'D', RTGETOPT_REQ_NOTHING },
    { "floppy",         'L', RTGETOPT_REQ_NOTHING },
    { "--filename",     'f', RTGETOPT_REQ_STRING },
    { "-filename",      'f', RTGETOPT_REQ_STRING },     // deprecated
    { "--diffparent",   'd', RTGETOPT_REQ_STRING },
    { "--size",         's', RTGETOPT_REQ_UINT64 },
    { "-size",          's', RTGETOPT_REQ_UINT64 },     // deprecated
    { "--sizebyte",     'S', RTGETOPT_REQ_UINT64 },
    { "--format",       'o', RTGETOPT_REQ_STRING },
    { "-format",        'o', RTGETOPT_REQ_STRING },     // deprecated
    { "--static",       'F', RTGETOPT_REQ_NOTHING },
    { "-static",        'F', RTGETOPT_REQ_NOTHING },    // deprecated
    { "--variant",      'm', RTGETOPT_REQ_STRING },
    { "-variant",       'm', RTGETOPT_REQ_STRING },     // deprecated
};

RTEXITCODE handleCreateMedium(HandlerArg *a)
{
    HRESULT rc;
    int vrc;
    const char *filename = NULL;
    const char *diffparent = NULL;
    uint64_t size = 0;
    enum {
        CMD_NONE,
        CMD_DISK,
        CMD_DVD,
        CMD_FLOPPY
    } cmd = CMD_NONE;
    const char *format = NULL;
    bool fBase = true;
    MediumVariant_T enmMediumVariant = MediumVariant_Standard;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aCreateMediumOptions, RT_ELEMENTS(g_aCreateMediumOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'H':   // disk
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CREATEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DISK;
                break;

            case 'D':   // DVD
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CREATEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DVD;
                break;

            case 'L':   // floppy
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CREATEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_FLOPPY;
                break;

            case 'f':   // --filename
                filename = ValueUnion.psz;
                break;

            case 'd':   // --diffparent
                diffparent = ValueUnion.psz;
                fBase = false;
                break;

            case 's':   // --size
                size = ValueUnion.u64 * _1M;
                break;

            case 'S':   // --sizebyte
                size = ValueUnion.u64;
                break;

            case 'o':   // --format
                format = ValueUnion.psz;
                break;

            case 'F':   // --static ("fixed"/"flat")
            {
                unsigned uMediumVariant = (unsigned)enmMediumVariant;
                uMediumVariant |= MediumVariant_Fixed;
                enmMediumVariant = (MediumVariant_T)uMediumVariant;
                break;
            }

            case 'm':   // --variant
                vrc = parseMediumVariant(ValueUnion.psz, &enmMediumVariant);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid medium variant '%s'", ValueUnion.psz);
                break;

            case VINF_GETOPT_NOT_OPTION:
                return errorSyntax(USAGE_CREATEMEDIUM, "Invalid parameter '%s'", ValueUnion.psz);

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_CREATEMEDIUM, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_CREATEMEDIUM, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_CREATEMEDIUM, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_CREATEMEDIUM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_CREATEMEDIUM, "error: %Rrs", c);
        }
    }

    /* check the outcome */
    if (cmd == CMD_NONE)
        cmd = CMD_DISK;
    ComPtr<IMedium> pParentMedium;
    if (fBase)
    {
        if (   !filename
            || !*filename
            || size == 0)
            return errorSyntax(USAGE_CREATEMEDIUM, "Parameters --filename and --size are required");
        if (!format || !*format)
        {
            if (cmd == CMD_DISK)
                format = "VDI";
            else if (cmd == CMD_DVD || cmd == CMD_FLOPPY)
            {
                format = "RAW";
                unsigned uMediumVariant = (unsigned)enmMediumVariant;
                uMediumVariant |= MediumVariant_Fixed;
                enmMediumVariant = (MediumVariant_T)uMediumVariant;
            }
        }
    }
    else
    {
        if (   !filename
            || !*filename)
            return errorSyntax(USAGE_CREATEMEDIUM, "Parameters --filename is required");
        size = 0;
        if (cmd != CMD_DISK)
            return errorSyntax(USAGE_CREATEMEDIUM, "Creating a differencing medium is only supported for hard disks");
        enmMediumVariant = MediumVariant_Diff;
        if (!format || !*format)
        {
            const char *pszExt = RTPathSuffix(filename);
            /* Skip over . if there is an extension. */
            if (pszExt)
                pszExt++;
            if (!pszExt || !*pszExt)
                format = "VDI";
            else
                format = pszExt;
        }
        rc = openMedium(a, diffparent, DeviceType_HardDisk,
                        AccessMode_ReadWrite, pParentMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
        if (FAILED(rc))
            return RTEXITCODE_FAILURE;
        if (pParentMedium.isNull())
            return RTMsgErrorExit(RTEXITCODE_FAILURE, "Invalid parent hard disk reference, avoiding crash");
        MediumState_T state;
        CHECK_ERROR(pParentMedium, COMGETTER(State)(&state));
        if (FAILED(rc))
            return RTEXITCODE_FAILURE;
        if (state == MediumState_Inaccessible)
        {
            CHECK_ERROR(pParentMedium, RefreshState(&state));
            if (FAILED(rc))
                return RTEXITCODE_FAILURE;
        }
    }
    /* check for filename extension */
    /** @todo use IMediumFormat to cover all extensions generically */
    Utf8Str strName(filename);
    if (!RTPathHasSuffix(strName.c_str()))
    {
        Utf8Str strFormat(format);
        if (cmd == CMD_DISK)
        {
            if (strFormat.compare("vmdk", RTCString::CaseInsensitive) == 0)
                strName.append(".vmdk");
            else if (strFormat.compare("vhd", RTCString::CaseInsensitive) == 0)
                strName.append(".vhd");
            else
                strName.append(".vdi");
        } else if (cmd == CMD_DVD)
            strName.append(".iso");
        else if (cmd == CMD_FLOPPY)
            strName.append(".img");
        filename = strName.c_str();
    }

    ComPtr<IMedium> pMedium;
    if (cmd == CMD_DISK)
        rc = createMedium(a, format, filename, DeviceType_HardDisk,
                          AccessMode_ReadWrite, pMedium);
    else if (cmd == CMD_DVD)
        rc = createMedium(a, format, filename, DeviceType_DVD,
                          AccessMode_ReadOnly, pMedium);
    else if (cmd == CMD_FLOPPY)
        rc = createMedium(a, format, filename, DeviceType_Floppy,
                        AccessMode_ReadWrite, pMedium);
    else
        rc = E_INVALIDARG; /* cannot happen but make gcc happy */

    if (SUCCEEDED(rc) && pMedium)
    {
        ComPtr<IProgress> pProgress;
        com::SafeArray<MediumVariant_T> l_variants(sizeof(MediumVariant_T)*8);

        for (ULONG i = 0; i < l_variants.size(); ++i)
        {
            ULONG temp = enmMediumVariant;
            temp &= 1<<i;
            l_variants [i] = (MediumVariant_T)temp;
        }

        if (fBase)
            CHECK_ERROR(pMedium, CreateBaseStorage(size, ComSafeArrayAsInParam(l_variants), pProgress.asOutParam()));
        else
            CHECK_ERROR(pParentMedium, CreateDiffStorage(pMedium, ComSafeArrayAsInParam(l_variants), pProgress.asOutParam()));
        if (SUCCEEDED(rc) && pProgress)
        {
            rc = showProgress(pProgress);
            CHECK_PROGRESS_ERROR(pProgress, ("Failed to create medium"));
        }
    }

    if (SUCCEEDED(rc) && pMedium)
    {
        Bstr uuid;
        CHECK_ERROR(pMedium, COMGETTER(Id)(uuid.asOutParam()));
        RTPrintf("Medium created. UUID: %s\n", Utf8Str(uuid).c_str());

        //CHECK_ERROR(pMedium, Close());
    }
    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static const RTGETOPTDEF g_aModifyMediumOptions[] =
{
    { "disk",           'H', RTGETOPT_REQ_NOTHING },
    { "dvd",            'D', RTGETOPT_REQ_NOTHING },
    { "floppy",         'L', RTGETOPT_REQ_NOTHING },
    { "--type",         't', RTGETOPT_REQ_STRING },
    { "-type",          't', RTGETOPT_REQ_STRING },     // deprecated
    { "settype",        't', RTGETOPT_REQ_STRING },     // deprecated
    { "--autoreset",    'z', RTGETOPT_REQ_STRING },
    { "-autoreset",     'z', RTGETOPT_REQ_STRING },     // deprecated
    { "autoreset",      'z', RTGETOPT_REQ_STRING },     // deprecated
    { "--property",     'p', RTGETOPT_REQ_STRING },
    { "--compact",      'c', RTGETOPT_REQ_NOTHING },
    { "-compact",       'c', RTGETOPT_REQ_NOTHING },    // deprecated
    { "compact",        'c', RTGETOPT_REQ_NOTHING },    // deprecated
    { "--resize",       'r', RTGETOPT_REQ_UINT64 },
    { "--resizebyte",   'R', RTGETOPT_REQ_UINT64 },
    { "--move",         'm', RTGETOPT_REQ_STRING },
    { "--description",  'd', RTGETOPT_REQ_STRING }
};

RTEXITCODE handleModifyMedium(HandlerArg *a)
{
    HRESULT rc;
    int vrc;
    enum {
        CMD_NONE,
        CMD_DISK,
        CMD_DVD,
        CMD_FLOPPY
    } cmd = CMD_NONE;
    ComPtr<IMedium> pMedium;
    MediumType_T enmMediumType = MediumType_Normal; /* Shut up MSC */
    bool AutoReset = false;
    SafeArray<BSTR> mediumPropNames;
    SafeArray<BSTR> mediumPropValues;
    bool fModifyMediumType = false;
    bool fModifyAutoReset = false;
    bool fModifyProperties = false;
    bool fModifyCompact = false;
    bool fModifyResize = false;
    bool fModifyResizeMB = false;
    bool fModifyLocation = false;
    bool fModifyDescription = false;
    uint64_t cbResize = 0;
    const char *pszFilenameOrUuid = NULL;
    char *pszNewLocation = NULL;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aModifyMediumOptions, RT_ELEMENTS(g_aModifyMediumOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'H':   // disk
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_MODIFYMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DISK;
                break;

            case 'D':   // DVD
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_MODIFYMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DVD;
                break;

            case 'L':   // floppy
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_MODIFYMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_FLOPPY;
                break;

            case 't':   // --type
                vrc = parseMediumType(ValueUnion.psz, &enmMediumType);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid medium type '%s'", ValueUnion.psz);
                fModifyMediumType = true;
                break;

            case 'z':   // --autoreset
                vrc = parseBool(ValueUnion.psz, &AutoReset);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid autoreset parameter '%s'", ValueUnion.psz);
                fModifyAutoReset = true;
                break;

            case 'p':   // --property
            {
                /* Parse 'name=value' */
                char *pszProperty = RTStrDup(ValueUnion.psz);
                if (pszProperty)
                {
                    char *pDelimiter = strchr(pszProperty, '=');
                    if (pDelimiter)
                    {
                        *pDelimiter = '\0';

                        Bstr bstrName(pszProperty);
                        Bstr bstrValue(&pDelimiter[1]);
                        bstrName.detachTo(mediumPropNames.appendedRaw());
                        bstrValue.detachTo(mediumPropValues.appendedRaw());
                        fModifyProperties = true;
                    }
                    else
                    {
                        errorArgument("Invalid --property argument '%s'", ValueUnion.psz);
                        rc = E_FAIL;
                    }
                    RTStrFree(pszProperty);
                }
                else
                {
                    RTStrmPrintf(g_pStdErr, "Error: Failed to allocate memory for medium property '%s'\n", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case 'c':   // --compact
                fModifyCompact = true;
                break;

            case 'r':   // --resize
                cbResize = ValueUnion.u64 * _1M;
                fModifyResize = true;
                fModifyResizeMB = true; // do sanity check!
                break;

            case 'R':   // --resizebyte
                cbResize = ValueUnion.u64;
                fModifyResize = true;
                break;

            case 'm':   // --move
                /* Get a new location  */
                pszNewLocation = RTPathAbsDup(ValueUnion.psz);
                fModifyLocation = true;
                break;

            case 'd':   // --description
                /* Get a new description  */
                pszNewLocation = RTStrDup(ValueUnion.psz);
                fModifyDescription = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!pszFilenameOrUuid)
                    pszFilenameOrUuid = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_MODIFYMEDIUM, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_MODIFYMEDIUM, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_MODIFYMEDIUM, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_MODIFYMEDIUM, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_MODIFYMEDIUM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_MODIFYMEDIUM, "error: %Rrs", c);
        }
    }

    if (cmd == CMD_NONE)
        cmd = CMD_DISK;

    if (!pszFilenameOrUuid)
        return errorSyntax(USAGE_MODIFYMEDIUM, "Medium name or UUID required");

    if (!fModifyMediumType
        && !fModifyAutoReset
        && !fModifyProperties
        && !fModifyCompact
        && !fModifyResize
        && !fModifyLocation
        && !fModifyDescription)
        return errorSyntax(USAGE_MODIFYMEDIUM, "No operation specified");

    /* Always open the medium if necessary, there is no other way. */
    if (cmd == CMD_DISK)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_HardDisk,
                        AccessMode_ReadWrite, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else if (cmd == CMD_DVD)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_DVD,
                        AccessMode_ReadOnly, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else if (cmd == CMD_FLOPPY)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_Floppy,
                        AccessMode_ReadWrite, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else
        rc = E_INVALIDARG; /* cannot happen but make gcc happy */
    if (FAILED(rc))
        return RTEXITCODE_FAILURE;
    if (pMedium.isNull())
    {
        RTMsgError("Invalid medium reference, avoiding crash");
        return RTEXITCODE_FAILURE;
    }

    if (   fModifyResize
        && fModifyResizeMB)
    {
        // Sanity check
        //
        // In general users should know what they do but in this case users have no
        // alternative to VBoxManage. If happens that one wants to resize the disk
        // and uses --resize and does not consider that this parameter expects the
        // new medium size in MB not Byte. If the operation is started and then
        // aborted by the user, the result is most likely a medium which doesn't
        // work anymore.
        MediumState_T state;
        pMedium->RefreshState(&state);
        LONG64 logicalSize;
        pMedium->COMGETTER(LogicalSize)(&logicalSize);
        if (cbResize > (uint64_t)logicalSize * 1000)
        {
            RTMsgError("Error: Attempt to resize the medium from %RU64.%RU64 MB to %RU64.%RU64 MB. Use --resizebyte if this is intended!\n",
                    logicalSize / _1M, (logicalSize % _1M) / (_1M / 10), cbResize / _1M, (cbResize % _1M) / (_1M / 10));
            return RTEXITCODE_FAILURE;
        }
    }

    if (fModifyMediumType)
    {
        MediumType_T enmCurrMediumType;
        CHECK_ERROR(pMedium, COMGETTER(Type)(&enmCurrMediumType));

        if (enmCurrMediumType != enmMediumType)
            CHECK_ERROR(pMedium, COMSETTER(Type)(enmMediumType));
    }

    if (fModifyAutoReset)
    {
        CHECK_ERROR(pMedium, COMSETTER(AutoReset)(AutoReset));
    }

    if (fModifyProperties)
    {
        CHECK_ERROR(pMedium, SetProperties(ComSafeArrayAsInParam(mediumPropNames), ComSafeArrayAsInParam(mediumPropValues)));
    }

    if (fModifyCompact)
    {
        ComPtr<IProgress> pProgress;
        CHECK_ERROR(pMedium, Compact(pProgress.asOutParam()));
        if (SUCCEEDED(rc))
            rc = showProgress(pProgress);
        if (FAILED(rc))
        {
            if (rc == E_NOTIMPL)
                RTMsgError("Compact medium operation is not implemented!");
            else if (rc == VBOX_E_NOT_SUPPORTED)
                RTMsgError("Compact medium operation for this format is not implemented yet!");
            else if (!pProgress.isNull())
                CHECK_PROGRESS_ERROR(pProgress, ("Failed to compact medium"));
            else
                RTMsgError("Failed to compact medium!");
        }
    }

    if (fModifyResize)
    {
        ComPtr<IProgress> pProgress;
        CHECK_ERROR(pMedium, Resize(cbResize, pProgress.asOutParam()));
        if (SUCCEEDED(rc))
            rc = showProgress(pProgress);
        if (FAILED(rc))
        {
            if (rc == E_NOTIMPL)
                RTMsgError("Resize medium operation is not implemented!");
            else if (rc == VBOX_E_NOT_SUPPORTED)
                RTMsgError("Resize medium operation for this format is not implemented yet!");
            else if (!pProgress.isNull())
                CHECK_PROGRESS_ERROR(pProgress, ("Failed to resize medium"));
            else
                RTMsgError("Failed to resize medium!");
        }
    }

    if (fModifyLocation)
    {
        do
        {
            ComPtr<IProgress> pProgress;
            Utf8Str strLocation(pszNewLocation);
            RTStrFree(pszNewLocation);
            CHECK_ERROR(pMedium, SetLocation(Bstr(strLocation).raw(), pProgress.asOutParam()));

            if (SUCCEEDED(rc) && !pProgress.isNull())
            {
                rc = showProgress(pProgress);
                CHECK_PROGRESS_ERROR(pProgress, ("Failed to move medium"));
            }

            Bstr uuid;
            CHECK_ERROR_BREAK(pMedium, COMGETTER(Id)(uuid.asOutParam()));

            RTPrintf("Move medium with UUID %s finished \n", Utf8Str(uuid).c_str());
        }
        while (0);
    }

    if (fModifyDescription)
    {
        CHECK_ERROR(pMedium, COMSETTER(Description)(Bstr(pszNewLocation).raw()));

        RTPrintf("Medium description has been changed. \n");
    }

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static const RTGETOPTDEF g_aCloneMediumOptions[] =
{
    { "disk",           'd', RTGETOPT_REQ_NOTHING },
    { "dvd",            'D', RTGETOPT_REQ_NOTHING },
    { "floppy",         'f', RTGETOPT_REQ_NOTHING },
    { "--format",       'o', RTGETOPT_REQ_STRING },
    { "-format",        'o', RTGETOPT_REQ_STRING },
    { "--static",       'F', RTGETOPT_REQ_NOTHING },
    { "-static",        'F', RTGETOPT_REQ_NOTHING },
    { "--existing",     'E', RTGETOPT_REQ_NOTHING },
    { "--variant",      'm', RTGETOPT_REQ_STRING },
    { "-variant",       'm', RTGETOPT_REQ_STRING },
};

RTEXITCODE handleCloneMedium(HandlerArg *a)
{
    HRESULT rc;
    int vrc;
    enum {
        CMD_NONE,
        CMD_DISK,
        CMD_DVD,
        CMD_FLOPPY
    } cmd = CMD_NONE;
    const char *pszSrc = NULL;
    const char *pszDst = NULL;
    Bstr format;
    MediumVariant_T enmMediumVariant = MediumVariant_Standard;
    bool fExisting = false;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aCloneMediumOptions, RT_ELEMENTS(g_aCloneMediumOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'd':   // disk
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CLONEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DISK;
                break;

            case 'D':   // DVD
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CLONEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DVD;
                break;

            case 'f':   // floppy
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CLONEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_FLOPPY;
                break;

            case 'o':   // --format
                format = ValueUnion.psz;
                break;

            case 'F':   // --static
            {
                unsigned uMediumVariant = (unsigned)enmMediumVariant;
                uMediumVariant |= MediumVariant_Fixed;
                enmMediumVariant = (MediumVariant_T)uMediumVariant;
                break;
            }

            case 'E':   // --existing
                fExisting = true;
                break;

            case 'm':   // --variant
                vrc = parseMediumVariant(ValueUnion.psz, &enmMediumVariant);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid medium variant '%s'", ValueUnion.psz);
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!pszSrc)
                    pszSrc = ValueUnion.psz;
                else if (!pszDst)
                    pszDst = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_CLONEMEDIUM, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_GRAPH(c))
                        return errorSyntax(USAGE_CLONEMEDIUM, "unhandled option: -%c", c);
                    else
                        return errorSyntax(USAGE_CLONEMEDIUM, "unhandled option: %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_CLONEMEDIUM, "unknown option: %s", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_CLONEMEDIUM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_CLONEMEDIUM, "error: %Rrs", c);
        }
    }

    if (cmd == CMD_NONE)
        cmd = CMD_DISK;
    if (!pszSrc)
        return errorSyntax(USAGE_CLONEMEDIUM, "Mandatory UUID or input file parameter missing");
    if (!pszDst)
        return errorSyntax(USAGE_CLONEMEDIUM, "Mandatory output file parameter missing");
    if (fExisting && (!format.isEmpty() || enmMediumVariant != MediumType_Normal))
        return errorSyntax(USAGE_CLONEMEDIUM, "Specified options which cannot be used with --existing");

    ComPtr<IMedium> pSrcMedium;
    ComPtr<IMedium> pDstMedium;

    if (cmd == CMD_DISK)
        rc = openMedium(a, pszSrc, DeviceType_HardDisk, AccessMode_ReadOnly, pSrcMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else if (cmd == CMD_DVD)
        rc = openMedium(a, pszSrc, DeviceType_DVD, AccessMode_ReadOnly, pSrcMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else if (cmd == CMD_FLOPPY)
        rc = openMedium(a, pszSrc, DeviceType_Floppy, AccessMode_ReadOnly, pSrcMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else
        rc = E_INVALIDARG; /* cannot happen but make gcc happy */
    if (FAILED(rc))
        return RTEXITCODE_FAILURE;

    do
    {
        /* open/create destination medium */
        if (fExisting)
        {
            if (cmd == CMD_DISK)
                rc = openMedium(a, pszDst, DeviceType_HardDisk, AccessMode_ReadWrite, pDstMedium,
                                false /* fForceNewUuidOnOpen */, false /* fSilent */);
            else if (cmd == CMD_DVD)
                rc = openMedium(a, pszDst, DeviceType_DVD, AccessMode_ReadOnly, pDstMedium,
                                false /* fForceNewUuidOnOpen */, false /* fSilent */);
            else if (cmd == CMD_FLOPPY)
                rc = openMedium(a, pszDst, DeviceType_Floppy, AccessMode_ReadWrite, pDstMedium,
                                false /* fForceNewUuidOnOpen */, false /* fSilent */);
            if (FAILED(rc))
                break;

            /* Perform accessibility check now. */
            MediumState_T state;
            CHECK_ERROR_BREAK(pDstMedium, RefreshState(&state));
            CHECK_ERROR_BREAK(pDstMedium, COMGETTER(Format)(format.asOutParam()));
        }
        else
        {
            /*
             * In case the format is unspecified check that the source medium supports
             * image creation and use the same format for the destination image.
             * Use the default image format if it is not supported.
             */
            if (format.isEmpty())
            {
                ComPtr<IMediumFormat> pMediumFmt;
                com::SafeArray<MediumFormatCapabilities_T> l_caps;
                CHECK_ERROR_BREAK(pSrcMedium, COMGETTER(MediumFormat)(pMediumFmt.asOutParam()));
                CHECK_ERROR_BREAK(pMediumFmt, COMGETTER(Capabilities)(ComSafeArrayAsOutParam(l_caps)));
                ULONG caps=0;
                for (size_t i = 0; i < l_caps.size(); i++)
                    caps |= l_caps[i];
                if (caps & (  MediumFormatCapabilities_CreateDynamic
                            | MediumFormatCapabilities_CreateFixed))
                    CHECK_ERROR_BREAK(pMediumFmt, COMGETTER(Id)(format.asOutParam()));
            }
            Utf8Str strFormat(format);
            if (cmd == CMD_DISK)
                rc = createMedium(a, strFormat.c_str(), pszDst, DeviceType_HardDisk,
                                  AccessMode_ReadWrite, pDstMedium);
            else if (cmd == CMD_DVD)
                rc = createMedium(a, strFormat.c_str(), pszDst, DeviceType_DVD,
                                  AccessMode_ReadOnly, pDstMedium);
            else if (cmd == CMD_FLOPPY)
                rc = createMedium(a, strFormat.c_str(), pszDst, DeviceType_Floppy,
                                  AccessMode_ReadWrite, pDstMedium);
            if (FAILED(rc))
                break;
        }

        ComPtr<IProgress> pProgress;
        com::SafeArray<MediumVariant_T> l_variants(sizeof(MediumVariant_T)*8);

        for (ULONG i = 0; i < l_variants.size(); ++i)
        {
            ULONG temp = enmMediumVariant;
            temp &= 1<<i;
            l_variants [i] = (MediumVariant_T)temp;
        }

        CHECK_ERROR_BREAK(pSrcMedium, CloneTo(pDstMedium, ComSafeArrayAsInParam(l_variants), NULL, pProgress.asOutParam()));

        rc = showProgress(pProgress);
        CHECK_PROGRESS_ERROR_BREAK(pProgress, ("Failed to clone medium"));

        Bstr uuid;
        CHECK_ERROR_BREAK(pDstMedium, COMGETTER(Id)(uuid.asOutParam()));

        RTPrintf("Clone medium created in format '%ls'. UUID: %s\n",
                 format.raw(), Utf8Str(uuid).c_str());
    }
    while (0);

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static const RTGETOPTDEF g_aConvertFromRawHardDiskOptions[] =
{
    { "--format",       'o', RTGETOPT_REQ_STRING },
    { "-format",        'o', RTGETOPT_REQ_STRING },
    { "--static",       'F', RTGETOPT_REQ_NOTHING },
    { "-static",        'F', RTGETOPT_REQ_NOTHING },
    { "--variant",      'm', RTGETOPT_REQ_STRING },
    { "-variant",       'm', RTGETOPT_REQ_STRING },
    { "--uuid",         'u', RTGETOPT_REQ_STRING },
};

RTEXITCODE handleConvertFromRaw(HandlerArg *a)
{
    int rc = VINF_SUCCESS;
    bool fReadFromStdIn = false;
    const char *format = "VDI";
    const char *srcfilename = NULL;
    const char *dstfilename = NULL;
    const char *filesize = NULL;
    unsigned uImageFlags = VD_IMAGE_FLAGS_NONE;
    void *pvBuf = NULL;
    RTUUID uuid;
    PCRTUUID pUuid = NULL;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aConvertFromRawHardDiskOptions, RT_ELEMENTS(g_aConvertFromRawHardDiskOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'u':   // --uuid
                if (RT_FAILURE(RTUuidFromStr(&uuid, ValueUnion.psz)))
                    return errorSyntax(USAGE_CONVERTFROMRAW, "Invalid UUID '%s'", ValueUnion.psz);
                pUuid = &uuid;
                break;
            case 'o':   // --format
                format = ValueUnion.psz;
                break;

            case 'm':   // --variant
            {
                MediumVariant_T enmMediumVariant = MediumVariant_Standard;
                rc = parseMediumVariant(ValueUnion.psz, &enmMediumVariant);
                if (RT_FAILURE(rc))
                    return errorArgument("Invalid medium variant '%s'", ValueUnion.psz);
                /// @todo cleaner solution than assuming 1:1 mapping?
                uImageFlags = (unsigned)enmMediumVariant;
                break;
            }
            case VINF_GETOPT_NOT_OPTION:
                if (!srcfilename)
                {
                    srcfilename = ValueUnion.psz;
                    fReadFromStdIn = !strcmp(srcfilename, "stdin");
                }
                else if (!dstfilename)
                    dstfilename = ValueUnion.psz;
                else if (fReadFromStdIn && !filesize)
                    filesize = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_CONVERTFROMRAW, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                return errorGetOpt(USAGE_CONVERTFROMRAW, c, &ValueUnion);
        }
    }

    if (!srcfilename || !dstfilename || (fReadFromStdIn && !filesize))
        return errorSyntax(USAGE_CONVERTFROMRAW, "Incorrect number of parameters");
    RTStrmPrintf(g_pStdErr, "Converting from raw image file=\"%s\" to file=\"%s\"...\n",
                 srcfilename, dstfilename);

    PVDISK pDisk = NULL;

    PVDINTERFACE     pVDIfs = NULL;
    VDINTERFACEERROR vdInterfaceError;
    vdInterfaceError.pfnError     = handleVDError;
    vdInterfaceError.pfnMessage   = NULL;

    rc = VDInterfaceAdd(&vdInterfaceError.Core, "VBoxManage_IError", VDINTERFACETYPE_ERROR,
                        NULL, sizeof(VDINTERFACEERROR), &pVDIfs);
    AssertRC(rc);

    /* open raw image file. */
    RTFILE File;
    if (fReadFromStdIn)
        rc = RTFileFromNative(&File, RTFILE_NATIVE_STDIN);
    else
        rc = RTFileOpen(&File, srcfilename, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_WRITE);
    if (RT_FAILURE(rc))
    {
        RTMsgError("Cannot open file \"%s\": %Rrc", srcfilename, rc);
        goto out;
    }

    uint64_t cbFile;
    /* get image size. */
    if (fReadFromStdIn)
        cbFile = RTStrToUInt64(filesize);
    else
        rc = RTFileGetSize(File, &cbFile);
    if (RT_FAILURE(rc))
    {
        RTMsgError("Cannot get image size for file \"%s\": %Rrc", srcfilename, rc);
        goto out;
    }

    RTStrmPrintf(g_pStdErr, "Creating %s image with size %RU64 bytes (%RU64MB)...\n",
                 (uImageFlags & VD_IMAGE_FLAGS_FIXED) ? "fixed" : "dynamic", cbFile, (cbFile + _1M - 1) / _1M);
    char pszComment[256];
    RTStrPrintf(pszComment, sizeof(pszComment), "Converted image from %s", srcfilename);
    rc = VDCreate(pVDIfs, VDTYPE_HDD, &pDisk);
    if (RT_FAILURE(rc))
    {
        RTMsgError("Cannot create the virtual disk container: %Rrc", rc);
        goto out;
    }

    Assert(RT_MIN(cbFile / 512 / 16 / 63, 16383) -
           (unsigned int)RT_MIN(cbFile / 512 / 16 / 63, 16383) == 0);
    VDGEOMETRY PCHS, LCHS;
    PCHS.cCylinders = (unsigned int)RT_MIN(cbFile / 512 / 16 / 63, 16383);
    PCHS.cHeads = 16;
    PCHS.cSectors = 63;
    LCHS.cCylinders = 0;
    LCHS.cHeads = 0;
    LCHS.cSectors = 0;
    rc = VDCreateBase(pDisk, format, dstfilename, cbFile,
                      uImageFlags, pszComment, &PCHS, &LCHS, pUuid,
                      VD_OPEN_FLAGS_NORMAL, NULL, NULL);
    if (RT_FAILURE(rc))
    {
        RTMsgError("Cannot create the disk image \"%s\": %Rrc", dstfilename, rc);
        goto out;
    }

    size_t cbBuffer;
    cbBuffer = _1M;
    pvBuf = RTMemAlloc(cbBuffer);
    if (!pvBuf)
    {
        rc = VERR_NO_MEMORY;
        RTMsgError("Out of memory allocating buffers for image \"%s\": %Rrc", dstfilename, rc);
        goto out;
    }

    uint64_t offFile;
    offFile = 0;
    while (offFile < cbFile)
    {
        size_t cbRead;
        size_t cbToRead;
        cbRead = 0;
        cbToRead = cbFile - offFile >= (uint64_t)cbBuffer ?
                            cbBuffer : (size_t)(cbFile - offFile);
        rc = RTFileRead(File, pvBuf, cbToRead, &cbRead);
        if (RT_FAILURE(rc) || !cbRead)
            break;
        rc = VDWrite(pDisk, offFile, pvBuf, cbRead);
        if (RT_FAILURE(rc))
        {
            RTMsgError("Failed to write to disk image \"%s\": %Rrc", dstfilename, rc);
            goto out;
        }
        offFile += cbRead;
    }

out:
    if (pvBuf)
        RTMemFree(pvBuf);
    if (pDisk)
        VDClose(pDisk, RT_FAILURE(rc));
    if (File != NIL_RTFILE)
        RTFileClose(File);

    return RT_SUCCESS(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

HRESULT showMediumInfo(const ComPtr<IVirtualBox> &pVirtualBox,
                       const ComPtr<IMedium> &pMedium,
                       const char *pszParentUUID,
                       bool fOptLong)
{
    HRESULT rc = S_OK;
    do
    {
        Bstr uuid;
        pMedium->COMGETTER(Id)(uuid.asOutParam());
        RTPrintf("UUID:           %ls\n", uuid.raw());
        if (pszParentUUID)
            RTPrintf("Parent UUID:    %s\n", pszParentUUID);

        /* check for accessibility */
        MediumState_T enmState;
        CHECK_ERROR_BREAK(pMedium, RefreshState(&enmState));
        const char *pszState = "unknown";
        switch (enmState)
        {
            case MediumState_NotCreated:
                pszState = "not created";
                break;
            case MediumState_Created:
                pszState = "created";
                break;
            case MediumState_LockedRead:
                pszState = "locked read";
                break;
            case MediumState_LockedWrite:
                pszState = "locked write";
                break;
            case MediumState_Inaccessible:
                pszState = "inaccessible";
                break;
            case MediumState_Creating:
                pszState = "creating";
                break;
            case MediumState_Deleting:
                pszState = "deleting";
                break;
        }
        RTPrintf("State:          %s\n", pszState);

        if (fOptLong && enmState == MediumState_Inaccessible)
        {
            Bstr err;
            CHECK_ERROR_BREAK(pMedium, COMGETTER(LastAccessError)(err.asOutParam()));
            RTPrintf("Access Error:   %ls\n", err.raw());
        }

        if (fOptLong)
        {
            Bstr description;
            pMedium->COMGETTER(Description)(description.asOutParam());
            if (!description.isEmpty())
                RTPrintf("Description:    %ls\n", description.raw());
        }

        MediumType_T type;
        pMedium->COMGETTER(Type)(&type);
        const char *typeStr = "unknown";
        switch (type)
        {
            case MediumType_Normal:
                if (pszParentUUID && Guid(pszParentUUID).isValid())
                    typeStr = "normal (differencing)";
                else
                    typeStr = "normal (base)";
                break;
            case MediumType_Immutable:
                typeStr = "immutable";
                break;
            case MediumType_Writethrough:
                typeStr = "writethrough";
                break;
            case MediumType_Shareable:
                typeStr = "shareable";
                break;
            case MediumType_Readonly:
                typeStr = "readonly";
                break;
            case MediumType_MultiAttach:
                typeStr = "multiattach";
                break;
        }
        RTPrintf("Type:           %s\n", typeStr);

        /* print out information specific for differencing media */
        if (fOptLong && pszParentUUID && Guid(pszParentUUID).isValid())
        {
            BOOL autoReset = FALSE;
            pMedium->COMGETTER(AutoReset)(&autoReset);
            RTPrintf("Auto-Reset:     %s\n", autoReset ? "on" : "off");
        }

        Bstr loc;
        pMedium->COMGETTER(Location)(loc.asOutParam());
        RTPrintf("Location:       %ls\n", loc.raw());

        Bstr format;
        pMedium->COMGETTER(Format)(format.asOutParam());
        RTPrintf("Storage format: %ls\n", format.raw());

        if (fOptLong)
        {
            com::SafeArray<MediumVariant_T> safeArray_variant;

            pMedium->COMGETTER(Variant)(ComSafeArrayAsOutParam(safeArray_variant));
            ULONG variant=0;
            for (size_t i = 0; i < safeArray_variant.size(); i++)
                variant |= safeArray_variant[i];

            const char *variantStr = "unknown";
            switch (variant & ~(MediumVariant_Fixed | MediumVariant_Diff))
            {
                case MediumVariant_VmdkSplit2G:
                    variantStr = "split2G";
                    break;
                case MediumVariant_VmdkStreamOptimized:
                    variantStr = "streamOptimized";
                    break;
                case MediumVariant_VmdkESX:
                    variantStr = "ESX";
                    break;
                case MediumVariant_Standard:
                    variantStr = "default";
                    break;
            }
            const char *variantTypeStr = "dynamic";
            if (variant & MediumVariant_Fixed)
                variantTypeStr = "fixed";
            else if (variant & MediumVariant_Diff)
                variantTypeStr = "differencing";
            RTPrintf("Format variant: %s %s\n", variantTypeStr, variantStr);
        }

        LONG64 logicalSize;
        pMedium->COMGETTER(LogicalSize)(&logicalSize);
        RTPrintf("Capacity:       %lld MBytes\n", logicalSize >> 20);
        if (fOptLong)
        {
            LONG64 actualSize;
            pMedium->COMGETTER(Size)(&actualSize);
            RTPrintf("Size on disk:   %lld MBytes\n", actualSize >> 20);
        }

        Bstr strCipher;
        Bstr strPasswordId;
        HRESULT rc2 = pMedium->GetEncryptionSettings(strCipher.asOutParam(), strPasswordId.asOutParam());
        if (SUCCEEDED(rc2))
        {
            RTPrintf("Encryption:     enabled\n");
            if (fOptLong)
            {
                RTPrintf("Cipher:         %ls\n", strCipher.raw());
                RTPrintf("Password ID:    %ls\n", strPasswordId.raw());
            }
        }
        else
            RTPrintf("Encryption:     disabled\n");

        if (fOptLong)
        {
            com::SafeArray<BSTR> names;
            com::SafeArray<BSTR> values;
            pMedium->GetProperties(Bstr().raw(), ComSafeArrayAsOutParam(names), ComSafeArrayAsOutParam(values));
            size_t cNames = names.size();
            size_t cValues = values.size();
            bool fFirst = true;
            for (size_t i = 0; i < cNames; i++)
            {
                Bstr value;
                if (i < cValues)
                    value = values[i];
                RTPrintf("%s%ls=%ls\n",
                         fFirst ? "Property:       " : "                ",
                         names[i], value.raw());
            }
        }

        if (fOptLong)
        {
            bool fFirst = true;
            com::SafeArray<BSTR> machineIds;
            pMedium->COMGETTER(MachineIds)(ComSafeArrayAsOutParam(machineIds));
            for (size_t i = 0; i < machineIds.size(); i++)
            {
                ComPtr<IMachine> pMachine;
                CHECK_ERROR(pVirtualBox, FindMachine(machineIds[i], pMachine.asOutParam()));
                if (pMachine)
                {
                    Bstr name;
                    pMachine->COMGETTER(Name)(name.asOutParam());
                    pMachine->COMGETTER(Id)(uuid.asOutParam());
                    RTPrintf("%s%ls (UUID: %ls)",
                             fFirst ? "In use by VMs:  " : "                ",
                             name.raw(), machineIds[i]);
                    fFirst = false;
                    com::SafeArray<BSTR> snapshotIds;
                    pMedium->GetSnapshotIds(machineIds[i],
                                            ComSafeArrayAsOutParam(snapshotIds));
                    for (size_t j = 0; j < snapshotIds.size(); j++)
                    {
                        ComPtr<ISnapshot> pSnapshot;
                        pMachine->FindSnapshot(snapshotIds[j], pSnapshot.asOutParam());
                        if (pSnapshot)
                        {
                            Bstr snapshotName;
                            pSnapshot->COMGETTER(Name)(snapshotName.asOutParam());
                            RTPrintf(" [%ls (UUID: %ls)]", snapshotName.raw(), snapshotIds[j]);
                        }
                    }
                    RTPrintf("\n");
                }
            }
        }

        if (fOptLong)
        {
            com::SafeIfaceArray<IMedium> children;
            pMedium->COMGETTER(Children)(ComSafeArrayAsOutParam(children));
            bool fFirst = true;
            for (size_t i = 0; i < children.size(); i++)
            {
                ComPtr<IMedium> pChild(children[i]);
                if (pChild)
                {
                    Bstr childUUID;
                    pChild->COMGETTER(Id)(childUUID.asOutParam());
                    RTPrintf("%s%ls\n",
                             fFirst ? "Child UUIDs:    " : "                ",
                             childUUID.raw());
                    fFirst = false;
                }
            }
        }
    }
    while (0);

    return rc;
}

static const RTGETOPTDEF g_aShowMediumInfoOptions[] =
{
    { "disk",           'd', RTGETOPT_REQ_NOTHING },
    { "dvd",            'D', RTGETOPT_REQ_NOTHING },
    { "floppy",         'f', RTGETOPT_REQ_NOTHING },
};

RTEXITCODE handleShowMediumInfo(HandlerArg *a)
{
    enum {
        CMD_NONE,
        CMD_DISK,
        CMD_DVD,
        CMD_FLOPPY
    } cmd = CMD_NONE;
    const char *pszFilenameOrUuid = NULL;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aShowMediumInfoOptions, RT_ELEMENTS(g_aShowMediumInfoOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'd':   // disk
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_SHOWMEDIUMINFO, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DISK;
                break;

            case 'D':   // DVD
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_SHOWMEDIUMINFO, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DVD;
                break;

            case 'f':   // floppy
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_SHOWMEDIUMINFO, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_FLOPPY;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!pszFilenameOrUuid)
                    pszFilenameOrUuid = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_SHOWMEDIUMINFO, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_SHOWMEDIUMINFO, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_SHOWMEDIUMINFO, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_SHOWMEDIUMINFO, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_SHOWMEDIUMINFO, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_SHOWMEDIUMINFO, "error: %Rrs", c);
        }
    }

    if (cmd == CMD_NONE)
        cmd = CMD_DISK;

    /* check for required options */
    if (!pszFilenameOrUuid)
        return errorSyntax(USAGE_SHOWMEDIUMINFO, "Medium name or UUID required");

    HRESULT rc = S_OK; /* Prevents warning. */

    ComPtr<IMedium> pMedium;
    if (cmd == CMD_DISK)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_HardDisk,
                        AccessMode_ReadOnly, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else if (cmd == CMD_DVD)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_DVD,
                        AccessMode_ReadOnly, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else if (cmd == CMD_FLOPPY)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_Floppy,
                        AccessMode_ReadOnly, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    if (FAILED(rc))
        return RTEXITCODE_FAILURE;

    Utf8Str strParentUUID("base");
    ComPtr<IMedium> pParent;
    pMedium->COMGETTER(Parent)(pParent.asOutParam());
    if (!pParent.isNull())
    {
        Bstr bstrParentUUID;
        pParent->COMGETTER(Id)(bstrParentUUID.asOutParam());
        strParentUUID = bstrParentUUID;
    }

    rc = showMediumInfo(a->virtualBox, pMedium, strParentUUID.c_str(), true);

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static const RTGETOPTDEF g_aCloseMediumOptions[] =
{
    { "disk",           'd', RTGETOPT_REQ_NOTHING },
    { "dvd",            'D', RTGETOPT_REQ_NOTHING },
    { "floppy",         'f', RTGETOPT_REQ_NOTHING },
    { "--delete",       'r', RTGETOPT_REQ_NOTHING },
};

RTEXITCODE handleCloseMedium(HandlerArg *a)
{
    HRESULT rc = S_OK;
    enum {
        CMD_NONE,
        CMD_DISK,
        CMD_DVD,
        CMD_FLOPPY
    } cmd = CMD_NONE;
    const char *pszFilenameOrUuid = NULL;
    bool fDelete = false;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aCloseMediumOptions, RT_ELEMENTS(g_aCloseMediumOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'd':   // disk
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CLOSEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DISK;
                break;

            case 'D':   // DVD
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CLOSEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DVD;
                break;

            case 'f':   // floppy
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CLOSEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_FLOPPY;
                break;

            case 'r':   // --delete
                fDelete = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!pszFilenameOrUuid)
                    pszFilenameOrUuid = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_CLOSEMEDIUM, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_CLOSEMEDIUM, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_CLOSEMEDIUM, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_CLOSEMEDIUM, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_CLOSEMEDIUM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_CLOSEMEDIUM, "error: %Rrs", c);
        }
    }

    /* check for required options */
    if (cmd == CMD_NONE)
        cmd = CMD_DISK;
    if (!pszFilenameOrUuid)
        return errorSyntax(USAGE_CLOSEMEDIUM, "Medium name or UUID required");

    ComPtr<IMedium> pMedium;
    if (cmd == CMD_DISK)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_HardDisk,
                        AccessMode_ReadWrite, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else if (cmd == CMD_DVD)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_DVD,
                        AccessMode_ReadOnly, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else if (cmd == CMD_FLOPPY)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_Floppy,
                        AccessMode_ReadWrite, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);

    if (SUCCEEDED(rc) && pMedium)
    {
        if (fDelete)
        {
            ComPtr<IProgress> pProgress;
            CHECK_ERROR(pMedium, DeleteStorage(pProgress.asOutParam()));
            if (SUCCEEDED(rc))
            {
                rc = showProgress(pProgress);
                CHECK_PROGRESS_ERROR(pProgress, ("Failed to delete medium"));
            }
            else
                RTMsgError("Failed to delete medium. Error code %Rrc", rc);
        }
        CHECK_ERROR(pMedium, Close());
    }

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

RTEXITCODE handleMediumProperty(HandlerArg *a)
{
    HRESULT rc = S_OK;
    const char *pszCmd = NULL;
    enum {
        CMD_NONE,
        CMD_DISK,
        CMD_DVD,
        CMD_FLOPPY
    } cmd = CMD_NONE;
    const char *pszAction = NULL;
    const char *pszFilenameOrUuid = NULL;
    const char *pszProperty = NULL;
    ComPtr<IMedium> pMedium;

    pszCmd = (a->argc > 0) ? a->argv[0] : "";
    if (   !RTStrICmp(pszCmd, "disk")
        || !RTStrICmp(pszCmd, "dvd")
        || !RTStrICmp(pszCmd, "floppy"))
    {
        if (!RTStrICmp(pszCmd, "disk"))
            cmd = CMD_DISK;
        else if (!RTStrICmp(pszCmd, "dvd"))
            cmd = CMD_DVD;
        else if (!RTStrICmp(pszCmd, "floppy"))
            cmd = CMD_FLOPPY;
        else
        {
            AssertMsgFailed(("unexpected parameter %s\n", pszCmd));
            cmd = CMD_DISK;
        }
        a->argv++;
        a->argc--;
    }
    else
    {
        pszCmd = NULL;
        cmd = CMD_DISK;
    }

    if (a->argc == 0)
        return errorSyntax(USAGE_MEDIUMPROPERTY, "Missing action");

    pszAction = a->argv[0];
    if (   RTStrICmp(pszAction, "set")
        && RTStrICmp(pszAction, "get")
        && RTStrICmp(pszAction, "delete"))
        return errorSyntax(USAGE_MEDIUMPROPERTY, "Invalid action given: %s", pszAction);

    if (   (   !RTStrICmp(pszAction, "set")
            && a->argc != 4)
        || (   RTStrICmp(pszAction, "set")
            && a->argc != 3))
        return errorSyntax(USAGE_MEDIUMPROPERTY, "Invalid number of arguments given for action: %s", pszAction);

    pszFilenameOrUuid = a->argv[1];
    pszProperty       = a->argv[2];

    if (cmd == CMD_DISK)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_HardDisk,
                        AccessMode_ReadWrite, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else if (cmd == CMD_DVD)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_DVD,
                        AccessMode_ReadOnly, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    else if (cmd == CMD_FLOPPY)
        rc = openMedium(a, pszFilenameOrUuid, DeviceType_Floppy,
                        AccessMode_ReadWrite, pMedium,
                        false /* fForceNewUuidOnOpen */, false /* fSilent */);
    if (SUCCEEDED(rc) && !pMedium.isNull())
    {
        if (!RTStrICmp(pszAction, "set"))
        {
            const char *pszValue = a->argv[3];
            CHECK_ERROR(pMedium, SetProperty(Bstr(pszProperty).raw(), Bstr(pszValue).raw()));
        }
        else if (!RTStrICmp(pszAction, "get"))
        {
            Bstr strVal;
            CHECK_ERROR(pMedium, GetProperty(Bstr(pszProperty).raw(), strVal.asOutParam()));
            if (SUCCEEDED(rc))
                RTPrintf("%s=%ls\n", pszProperty, strVal.raw());
        }
        else if (!RTStrICmp(pszAction, "delete"))
        {
            CHECK_ERROR(pMedium, SetProperty(Bstr(pszProperty).raw(), Bstr().raw()));
            /** @todo */
        }
    }

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static const RTGETOPTDEF g_aEncryptMediumOptions[] =
{
    { "--newpassword",   'n', RTGETOPT_REQ_STRING },
    { "--oldpassword",   'o', RTGETOPT_REQ_STRING },
    { "--cipher",        'c', RTGETOPT_REQ_STRING },
    { "--newpasswordid", 'i', RTGETOPT_REQ_STRING }
};

RTEXITCODE handleEncryptMedium(HandlerArg *a)
{
    HRESULT rc;
    ComPtr<IMedium> hardDisk;
    const char *pszPasswordNew = NULL;
    const char *pszPasswordOld = NULL;
    const char *pszCipher = NULL;
    const char *pszFilenameOrUuid = NULL;
    const char *pszNewPasswordId = NULL;
    Utf8Str strPasswordNew;
    Utf8Str strPasswordOld;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aEncryptMediumOptions, RT_ELEMENTS(g_aEncryptMediumOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'n':   // --newpassword
                pszPasswordNew = ValueUnion.psz;
                break;

            case 'o':   // --oldpassword
                pszPasswordOld = ValueUnion.psz;
                break;

            case 'c':   // --cipher
                pszCipher = ValueUnion.psz;
                break;

            case 'i':   // --newpasswordid
                pszNewPasswordId = ValueUnion.psz;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!pszFilenameOrUuid)
                    pszFilenameOrUuid = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_ENCRYPTMEDIUM, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_ENCRYPTMEDIUM, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_ENCRYPTMEDIUM, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_ENCRYPTMEDIUM, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_ENCRYPTMEDIUM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_ENCRYPTMEDIUM, "error: %Rrs", c);
        }
    }

    if (!pszFilenameOrUuid)
        return errorSyntax(USAGE_ENCRYPTMEDIUM, "Disk name or UUID required");

    if (!pszPasswordNew && !pszPasswordOld)
        return errorSyntax(USAGE_ENCRYPTMEDIUM, "No password specified");

    if (   (pszPasswordNew && !pszNewPasswordId)
        || (!pszPasswordNew && pszNewPasswordId))
        return errorSyntax(USAGE_ENCRYPTMEDIUM, "A new password must always have a valid identifier set at the same time");

    if (pszPasswordNew)
    {
        if (!RTStrCmp(pszPasswordNew, "-"))
        {
            /* Get password from console. */
            RTEXITCODE rcExit = readPasswordFromConsole(&strPasswordNew, "Enter new password:");
            if (rcExit == RTEXITCODE_FAILURE)
                return rcExit;
        }
        else
        {
            RTEXITCODE rcExit = readPasswordFile(pszPasswordNew, &strPasswordNew);
            if (rcExit == RTEXITCODE_FAILURE)
            {
                RTMsgError("Failed to read new password from file");
                return rcExit;
            }
        }
    }

    if (pszPasswordOld)
    {
        if (!RTStrCmp(pszPasswordOld, "-"))
        {
            /* Get password from console. */
            RTEXITCODE rcExit = readPasswordFromConsole(&strPasswordOld, "Enter old password:");
            if (rcExit == RTEXITCODE_FAILURE)
                return rcExit;
        }
        else
        {
            RTEXITCODE rcExit = readPasswordFile(pszPasswordOld, &strPasswordOld);
            if (rcExit == RTEXITCODE_FAILURE)
            {
                RTMsgError("Failed to read old password from file");
                return rcExit;
            }
        }
    }

    /* Always open the medium if necessary, there is no other way. */
    rc = openMedium(a, pszFilenameOrUuid, DeviceType_HardDisk,
                    AccessMode_ReadWrite, hardDisk,
                    false /* fForceNewUuidOnOpen */, false /* fSilent */);
    if (FAILED(rc))
        return RTEXITCODE_FAILURE;
    if (hardDisk.isNull())
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "Invalid hard disk reference, avoiding crash");

    ComPtr<IProgress> progress;
    CHECK_ERROR(hardDisk, ChangeEncryption(Bstr(strPasswordOld).raw(), Bstr(pszCipher).raw(),
                                           Bstr(strPasswordNew).raw(), Bstr(pszNewPasswordId).raw(),
                                           progress.asOutParam()));
    if (SUCCEEDED(rc))
        rc = showProgress(progress);
    if (FAILED(rc))
    {
        if (rc == E_NOTIMPL)
            RTMsgError("Encrypt hard disk operation is not implemented!");
        else if (rc == VBOX_E_NOT_SUPPORTED)
            RTMsgError("Encrypt hard disk operation for this cipher is not implemented yet!");
        else if (!progress.isNull())
            CHECK_PROGRESS_ERROR(progress, ("Failed to encrypt hard disk"));
        else
            RTMsgError("Failed to encrypt hard disk!");
    }

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

RTEXITCODE handleCheckMediumPassword(HandlerArg *a)
{
    HRESULT rc;
    ComPtr<IMedium> hardDisk;
    const char *pszFilenameOrUuid = NULL;
    Utf8Str strPassword;

    if (a->argc != 2)
        return errorSyntax(USAGE_MEDIUMENCCHKPWD, "Invalid number of arguments: %d", a->argc);

    pszFilenameOrUuid = a->argv[0];

    if (!RTStrCmp(a->argv[1], "-"))
    {
        /* Get password from console. */
        RTEXITCODE rcExit = readPasswordFromConsole(&strPassword, "Enter password:");
        if (rcExit == RTEXITCODE_FAILURE)
            return rcExit;
    }
    else
    {
        RTEXITCODE rcExit = readPasswordFile(a->argv[1], &strPassword);
        if (rcExit == RTEXITCODE_FAILURE)
        {
            RTMsgError("Failed to read password from file");
            return rcExit;
        }
    }

    /* Always open the medium if necessary, there is no other way. */
    rc = openMedium(a, pszFilenameOrUuid, DeviceType_HardDisk,
                    AccessMode_ReadWrite, hardDisk,
                    false /* fForceNewUuidOnOpen */, false /* fSilent */);
    if (FAILED(rc))
        return RTEXITCODE_FAILURE;
    if (hardDisk.isNull())
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "Invalid hard disk reference, avoiding crash");

    CHECK_ERROR(hardDisk, CheckEncryptionPassword(Bstr(strPassword).raw()));
    if (SUCCEEDED(rc))
        RTPrintf("The given password is correct\n");
    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

#endif /* !VBOX_ONLY_DOCS */
