/* $Id: VBoxManageAppliance.cpp $ */
/** @file
 * VBoxManage - The appliance-related commands.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#ifndef VBOX_ONLY_DOCS
#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/VirtualBox.h>

#include <list>
#include <map>
#endif /* !VBOX_ONLY_DOCS */

#include <iprt/stream.h>
#include <iprt/getopt.h>
#include <iprt/ctype.h>
#include <iprt/path.h>
#include <iprt/file.h>

#include <VBox/log.h>
#include <VBox/param.h>

#include "VBoxManage.h"
using namespace com;


// funcs
///////////////////////////////////////////////////////////////////////////////

typedef std::map<Utf8Str, Utf8Str> ArgsMap;                 // pairs of strings like "vmname" => "newvmname"
typedef std::map<uint32_t, ArgsMap> ArgsMapsMap;            // map of maps, one for each virtual system, sorted by index

typedef std::map<uint32_t, bool> IgnoresMap;                // pairs of numeric description entry indices
typedef std::map<uint32_t, IgnoresMap> IgnoresMapsMap;      // map of maps, one for each virtual system, sorted by index

static bool findArgValue(Utf8Str &strOut,
                         ArgsMap *pmapArgs,
                         const Utf8Str &strKey)
{
    if (pmapArgs)
    {
        ArgsMap::iterator it;
        it = pmapArgs->find(strKey);
        if (it != pmapArgs->end())
        {
            strOut = it->second;
            pmapArgs->erase(it);
            return true;
        }
    }

    return false;
}

static int parseImportOptions(const char *psz, com::SafeArray<ImportOptions_T> *options)
{
    int rc = VINF_SUCCESS;
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
            if (!RTStrNICmp(psz, "KeepAllMACs", len))
                options->push_back(ImportOptions_KeepAllMACs);
            else if (!RTStrNICmp(psz, "KeepNATMACs", len))
                options->push_back(ImportOptions_KeepNATMACs);
            else if (!RTStrNICmp(psz, "ImportToVDI", len))
                options->push_back(ImportOptions_ImportToVDI);
            else
                rc = VERR_PARSE_ERROR;
        }
        if (pszComma)
            psz += len + 1;
        else
            psz += len;
    }

    return rc;
}

static const RTGETOPTDEF g_aImportApplianceOptions[] =
{
    { "--dry-run",              'n', RTGETOPT_REQ_NOTHING },
    { "-dry-run",               'n', RTGETOPT_REQ_NOTHING },    // deprecated
    { "--dryrun",               'n', RTGETOPT_REQ_NOTHING },
    { "-dryrun",                'n', RTGETOPT_REQ_NOTHING },    // deprecated
    { "--detailed-progress",    'P', RTGETOPT_REQ_NOTHING },
    { "-detailed-progress",     'P', RTGETOPT_REQ_NOTHING },    // deprecated
    { "--vsys",                 's', RTGETOPT_REQ_UINT32 },
    { "-vsys",                  's', RTGETOPT_REQ_UINT32 },     // deprecated
    { "--ostype",               'o', RTGETOPT_REQ_STRING },
    { "-ostype",                'o', RTGETOPT_REQ_STRING },     // deprecated
    { "--vmname",               'V', RTGETOPT_REQ_STRING },
    { "-vmname",                'V', RTGETOPT_REQ_STRING },     // deprecated
    { "--memory",               'm', RTGETOPT_REQ_STRING },
    { "-memory",                'm', RTGETOPT_REQ_STRING },     // deprecated
    { "--cpus",                 'c', RTGETOPT_REQ_STRING },
    { "--description",          'd', RTGETOPT_REQ_STRING },
    { "--eula",                 'L', RTGETOPT_REQ_STRING },
    { "-eula",                  'L', RTGETOPT_REQ_STRING },     // deprecated
    { "--unit",                 'u', RTGETOPT_REQ_UINT32 },
    { "-unit",                  'u', RTGETOPT_REQ_UINT32 },     // deprecated
    { "--ignore",               'x', RTGETOPT_REQ_NOTHING },
    { "-ignore",                'x', RTGETOPT_REQ_NOTHING },    // deprecated
    { "--scsitype",             'T', RTGETOPT_REQ_UINT32 },
    { "-scsitype",              'T', RTGETOPT_REQ_UINT32 },     // deprecated
    { "--type",                 'T', RTGETOPT_REQ_UINT32 },     // deprecated
    { "-type",                  'T', RTGETOPT_REQ_UINT32 },     // deprecated
#if 0 /* Changing the controller is fully valid, but the current design on how
         the params are evaluated here doesn't allow two parameter for one
         unit. The target disk path is more important. I leave it for future
         improvments. */
    { "--controller",           'C', RTGETOPT_REQ_STRING },
#endif
    { "--disk",                 'D', RTGETOPT_REQ_STRING },
    { "--options",              'O', RTGETOPT_REQ_STRING },
};

RTEXITCODE handleImportAppliance(HandlerArg *arg)
{
    HRESULT rc = S_OK;

    Utf8Str strOvfFilename;
    bool fExecute = true;                  // if true, then we actually do the import
    com::SafeArray<ImportOptions_T> options;
    uint32_t ulCurVsys = (uint32_t)-1;
    uint32_t ulCurUnit = (uint32_t)-1;
    // for each --vsys X command, maintain a map of command line items
    // (we'll parse them later after interpreting the OVF, when we can
    // actually check whether they make sense semantically)
    ArgsMapsMap mapArgsMapsPerVsys;
    IgnoresMapsMap mapIgnoresMapsPerVsys;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, arg->argc, arg->argv, g_aImportApplianceOptions, RT_ELEMENTS(g_aImportApplianceOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'n':   // --dry-run
                fExecute = false;
                break;

            case 'P':   // --detailed-progress
                g_fDetailedProgress = true;
                break;

            case 's':   // --vsys
                ulCurVsys = ValueUnion.u32;
                ulCurUnit = (uint32_t)-1;
                break;

            case 'o':   // --ostype
                if (ulCurVsys == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                mapArgsMapsPerVsys[ulCurVsys]["ostype"] = ValueUnion.psz;
                break;

            case 'V':   // --vmname
                if (ulCurVsys == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                mapArgsMapsPerVsys[ulCurVsys]["vmname"] = ValueUnion.psz;
                break;

            case 'd':   // --description
                if (ulCurVsys == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                mapArgsMapsPerVsys[ulCurVsys]["description"] = ValueUnion.psz;
                break;

            case 'L':   // --eula
                if (ulCurVsys == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                mapArgsMapsPerVsys[ulCurVsys]["eula"] = ValueUnion.psz;
                break;

            case 'm':   // --memory
                if (ulCurVsys == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                mapArgsMapsPerVsys[ulCurVsys]["memory"] = ValueUnion.psz;
                break;

            case 'c':   // --cpus
                if (ulCurVsys == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                mapArgsMapsPerVsys[ulCurVsys]["cpus"] = ValueUnion.psz;
                break;

            case 'u':   // --unit
                ulCurUnit = ValueUnion.u32;
                break;

            case 'x':   // --ignore
                if (ulCurVsys == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                if (ulCurUnit == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --unit argument.", GetState.pDef->pszLong);
                mapIgnoresMapsPerVsys[ulCurVsys][ulCurUnit] = true;
                break;

            case 'T':   // --scsitype
                if (ulCurVsys == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                if (ulCurUnit == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --unit argument.", GetState.pDef->pszLong);
                mapArgsMapsPerVsys[ulCurVsys][Utf8StrFmt("scsitype%u", ulCurUnit)] = ValueUnion.psz;
                break;

            case 'C':   // --controller
                if (ulCurVsys == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                if (ulCurUnit == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --unit argument.", GetState.pDef->pszLong);
                mapArgsMapsPerVsys[ulCurVsys][Utf8StrFmt("controller%u", ulCurUnit)] = ValueUnion.psz;
                break;

            case 'D':   // --disk
                if (ulCurVsys == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                if (ulCurUnit == (uint32_t)-1)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Option \"%s\" requires preceding --unit argument.", GetState.pDef->pszLong);
                mapArgsMapsPerVsys[ulCurVsys][Utf8StrFmt("disk%u", ulCurUnit)] = ValueUnion.psz;
                break;

            case 'O':   // --options
                if (RT_FAILURE(parseImportOptions(ValueUnion.psz, &options)))
                    return errorArgument("Invalid import options '%s'\n", ValueUnion.psz);
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (strOvfFilename.isEmpty())
                    strOvfFilename = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_IMPORTAPPLIANCE, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_IMPORTAPPLIANCE, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_IMPORTAPPLIANCE, "error: %Rrs", c);
        }
    }

    if (strOvfFilename.isEmpty())
        return errorSyntax(USAGE_IMPORTAPPLIANCE, "Not enough arguments for \"import\" command.");

    do
    {
        ComPtr<IAppliance> pAppliance;
        CHECK_ERROR_BREAK(arg->virtualBox, CreateAppliance(pAppliance.asOutParam()));

        char *pszAbsFilePath;
        if (strOvfFilename.startsWith("S3://", RTCString::CaseInsensitive) ||
            strOvfFilename.startsWith("SunCloud://", RTCString::CaseInsensitive) ||
            strOvfFilename.startsWith("webdav://", RTCString::CaseInsensitive))
            pszAbsFilePath = RTStrDup(strOvfFilename.c_str());
        else
            pszAbsFilePath = RTPathAbsDup(strOvfFilename.c_str());
        ComPtr<IProgress> progressRead;
        CHECK_ERROR_BREAK(pAppliance, Read(Bstr(pszAbsFilePath).raw(),
                                           progressRead.asOutParam()));
        RTStrFree(pszAbsFilePath);

        rc = showProgress(progressRead);
        CHECK_PROGRESS_ERROR_RET(progressRead, ("Appliance read failed"), RTEXITCODE_FAILURE);

        Bstr path; /* fetch the path, there is stuff like username/password removed if any */
        CHECK_ERROR_BREAK(pAppliance, COMGETTER(Path)(path.asOutParam()));
        // call interpret(); this can yield both warnings and errors, so we need
        // to tinker with the error info a bit
        RTStrmPrintf(g_pStdErr, "Interpreting %ls...\n", path.raw());
        rc = pAppliance->Interpret();
        com::ErrorInfo info0(pAppliance, COM_IIDOF(IAppliance));

        com::SafeArray<BSTR> aWarnings;
        if (SUCCEEDED(pAppliance->GetWarnings(ComSafeArrayAsOutParam(aWarnings))))
        {
            size_t cWarnings = aWarnings.size();
            for (unsigned i = 0; i < cWarnings; ++i)
            {
                Bstr bstrWarning(aWarnings[i]);
                RTMsgWarning("%ls.", bstrWarning.raw());
            }
        }

        if (FAILED(rc))     // during interpret, after printing warnings
        {
            com::GluePrintErrorInfo(info0);
            com::GluePrintErrorContext("Interpret", __FILE__, __LINE__);
            break;
        }

        RTStrmPrintf(g_pStdErr, "OK.\n");

        // fetch all disks
        com::SafeArray<BSTR> retDisks;
        CHECK_ERROR_BREAK(pAppliance,
                          COMGETTER(Disks)(ComSafeArrayAsOutParam(retDisks)));
        if (retDisks.size() > 0)
        {
            RTPrintf("Disks:\n");
            for (unsigned i = 0; i < retDisks.size(); i++)
                RTPrintf("  %ls\n", retDisks[i]);
            RTPrintf("\n");
        }

        // fetch virtual system descriptions
        com::SafeIfaceArray<IVirtualSystemDescription> aVirtualSystemDescriptions;
        CHECK_ERROR_BREAK(pAppliance,
                          COMGETTER(VirtualSystemDescriptions)(ComSafeArrayAsOutParam(aVirtualSystemDescriptions)));

        size_t cVirtualSystemDescriptions = aVirtualSystemDescriptions.size();

        // match command line arguments with virtual system descriptions;
        // this is only to sort out invalid indices at this time
        ArgsMapsMap::const_iterator it;
        for (it = mapArgsMapsPerVsys.begin();
             it != mapArgsMapsPerVsys.end();
             ++it)
        {
            uint32_t ulVsys = it->first;
            if (ulVsys >= cVirtualSystemDescriptions)
                return errorSyntax(USAGE_IMPORTAPPLIANCE,
                                   "Invalid index %RI32 with -vsys option; the OVF contains only %zu virtual system(s).",
                                   ulVsys, cVirtualSystemDescriptions);
        }

        uint32_t cLicensesInTheWay = 0;

        // dump virtual system descriptions and match command-line arguments
        if (cVirtualSystemDescriptions > 0)
        {
            for (unsigned i = 0; i < cVirtualSystemDescriptions; ++i)
            {
                com::SafeArray<VirtualSystemDescriptionType_T> retTypes;
                com::SafeArray<BSTR> aRefs;
                com::SafeArray<BSTR> aOvfValues;
                com::SafeArray<BSTR> aVBoxValues;
                com::SafeArray<BSTR> aExtraConfigValues;
                CHECK_ERROR_BREAK(aVirtualSystemDescriptions[i],
                                  GetDescription(ComSafeArrayAsOutParam(retTypes),
                                                 ComSafeArrayAsOutParam(aRefs),
                                                 ComSafeArrayAsOutParam(aOvfValues),
                                                 ComSafeArrayAsOutParam(aVBoxValues),
                                                 ComSafeArrayAsOutParam(aExtraConfigValues)));

                RTPrintf("Virtual system %u:\n", i);

                // look up the corresponding command line options, if any
                ArgsMap *pmapArgs = NULL;
                ArgsMapsMap::iterator itm = mapArgsMapsPerVsys.find(i);
                if (itm != mapArgsMapsPerVsys.end())
                    pmapArgs = &itm->second;

                // this collects the final values for setFinalValues()
                com::SafeArray<BOOL> aEnabled(retTypes.size());
                com::SafeArray<BSTR> aFinalValues(retTypes.size());

                for (unsigned a = 0; a < retTypes.size(); ++a)
                {
                    VirtualSystemDescriptionType_T t = retTypes[a];

                    Utf8Str strOverride;

                    Bstr bstrFinalValue = aVBoxValues[a];

                    bool fIgnoreThis = mapIgnoresMapsPerVsys[i][a];

                    aEnabled[a] = true;

                    switch (t)
                    {
                        case VirtualSystemDescriptionType_OS:
                            if (findArgValue(strOverride, pmapArgs, "ostype"))
                            {
                                bstrFinalValue = strOverride;
                                RTPrintf("%2u: OS type specified with --ostype: \"%ls\"\n",
                                        a, bstrFinalValue.raw());
                            }
                            else
                                RTPrintf("%2u: Suggested OS type: \"%ls\""
                                        "\n    (change with \"--vsys %u --ostype <type>\"; use \"list ostypes\" to list all possible values)\n",
                                        a, bstrFinalValue.raw(), i);
                            break;

                        case VirtualSystemDescriptionType_Name:
                            if (findArgValue(strOverride, pmapArgs, "vmname"))
                            {
                                bstrFinalValue = strOverride;
                                RTPrintf("%2u: VM name specified with --vmname: \"%ls\"\n",
                                        a, bstrFinalValue.raw());
                            }
                            else
                                RTPrintf("%2u: Suggested VM name \"%ls\""
                                        "\n    (change with \"--vsys %u --vmname <name>\")\n",
                                        a, bstrFinalValue.raw(), i);
                            break;

                        case VirtualSystemDescriptionType_Product:
                            RTPrintf("%2u: Product (ignored): %ls\n",
                                     a, aVBoxValues[a]);
                            break;

                        case VirtualSystemDescriptionType_ProductUrl:
                            RTPrintf("%2u: ProductUrl (ignored): %ls\n",
                                     a, aVBoxValues[a]);
                            break;

                        case VirtualSystemDescriptionType_Vendor:
                            RTPrintf("%2u: Vendor (ignored): %ls\n",
                                     a, aVBoxValues[a]);
                            break;

                        case VirtualSystemDescriptionType_VendorUrl:
                            RTPrintf("%2u: VendorUrl (ignored): %ls\n",
                                     a, aVBoxValues[a]);
                            break;

                        case VirtualSystemDescriptionType_Version:
                            RTPrintf("%2u: Version (ignored): %ls\n",
                                     a, aVBoxValues[a]);
                            break;

                        case VirtualSystemDescriptionType_Description:
                            if (findArgValue(strOverride, pmapArgs, "description"))
                            {
                                bstrFinalValue = strOverride;
                                RTPrintf("%2u: Description specified with --description: \"%ls\"\n",
                                        a, bstrFinalValue.raw());
                            }
                            else
                                RTPrintf("%2u: Description \"%ls\""
                                        "\n    (change with \"--vsys %u --description <desc>\")\n",
                                        a, bstrFinalValue.raw(), i);
                            break;

                        case VirtualSystemDescriptionType_License:
                            ++cLicensesInTheWay;
                            if (findArgValue(strOverride, pmapArgs, "eula"))
                            {
                                if (strOverride == "show")
                                {
                                    RTPrintf("%2u: End-user license agreement"
                                             "\n    (accept with \"--vsys %u --eula accept\"):"
                                             "\n\n%ls\n\n",
                                             a, i, bstrFinalValue.raw());
                                }
                                else if (strOverride == "accept")
                                {
                                    RTPrintf("%2u: End-user license agreement (accepted)\n",
                                             a);
                                    --cLicensesInTheWay;
                                }
                                else
                                    return errorSyntax(USAGE_IMPORTAPPLIANCE,
                                                       "Argument to --eula must be either \"show\" or \"accept\".");
                            }
                            else
                                RTPrintf("%2u: End-user license agreement"
                                        "\n    (display with \"--vsys %u --eula show\";"
                                        "\n    accept with \"--vsys %u --eula accept\")\n",
                                        a, i, i);
                            break;

                        case VirtualSystemDescriptionType_CPU:
                            if (findArgValue(strOverride, pmapArgs, "cpus"))
                            {
                                uint32_t cCPUs;
                                if (    strOverride.toInt(cCPUs) == VINF_SUCCESS
                                     && cCPUs >= VMM_MIN_CPU_COUNT
                                     && cCPUs <= VMM_MAX_CPU_COUNT
                                   )
                                {
                                    bstrFinalValue = strOverride;
                                    RTPrintf("%2u: No. of CPUs specified with --cpus: %ls\n",
                                             a, bstrFinalValue.raw());
                                }
                                else
                                    return errorSyntax(USAGE_IMPORTAPPLIANCE,
                                                       "Argument to --cpus option must be a number greater than %d and less than %d.",
                                                       VMM_MIN_CPU_COUNT - 1, VMM_MAX_CPU_COUNT + 1);
                            }
                            else
                                RTPrintf("%2u: Number of CPUs: %ls\n    (change with \"--vsys %u --cpus <n>\")\n",
                                         a, bstrFinalValue.raw(), i);
                            break;

                        case VirtualSystemDescriptionType_Memory:
                        {
                            if (findArgValue(strOverride, pmapArgs, "memory"))
                            {
                                uint32_t ulMemMB;
                                if (VINF_SUCCESS == strOverride.toInt(ulMemMB))
                                {
                                    bstrFinalValue = strOverride;
                                    RTPrintf("%2u: Guest memory specified with --memory: %ls MB\n",
                                             a, bstrFinalValue.raw());
                                }
                                else
                                    return errorSyntax(USAGE_IMPORTAPPLIANCE,
                                                       "Argument to --memory option must be a non-negative number.");
                            }
                            else
                                RTPrintf("%2u: Guest memory: %ls MB\n    (change with \"--vsys %u --memory <MB>\")\n",
                                         a, bstrFinalValue.raw(), i);
                            break;
                        }

                        case VirtualSystemDescriptionType_HardDiskControllerIDE:
                            if (fIgnoreThis)
                            {
                                RTPrintf("%2u: IDE controller, type %ls -- disabled\n",
                                         a,
                                         aVBoxValues[a]);
                                aEnabled[a] = false;
                            }
                            else
                                RTPrintf("%2u: IDE controller, type %ls"
                                         "\n    (disable with \"--vsys %u --unit %u --ignore\")\n",
                                         a,
                                         aVBoxValues[a],
                                         i, a);
                            break;

                        case VirtualSystemDescriptionType_HardDiskControllerSATA:
                            if (fIgnoreThis)
                            {
                                RTPrintf("%2u: SATA controller, type %ls -- disabled\n",
                                         a,
                                         aVBoxValues[a]);
                                aEnabled[a] = false;
                            }
                            else
                                RTPrintf("%2u: SATA controller, type %ls"
                                        "\n    (disable with \"--vsys %u --unit %u --ignore\")\n",
                                        a,
                                        aVBoxValues[a],
                                        i, a);
                            break;

                        case VirtualSystemDescriptionType_HardDiskControllerSAS:
                            if (fIgnoreThis)
                            {
                                RTPrintf("%2u: SAS controller, type %ls -- disabled\n",
                                         a,
                                         aVBoxValues[a]);
                                aEnabled[a] = false;
                            }
                            else
                                RTPrintf("%2u: SAS controller, type %ls"
                                        "\n    (disable with \"--vsys %u --unit %u --ignore\")\n",
                                        a,
                                        aVBoxValues[a],
                                        i, a);
                            break;

                        case VirtualSystemDescriptionType_HardDiskControllerSCSI:
                            if (fIgnoreThis)
                            {
                                RTPrintf("%2u: SCSI controller, type %ls -- disabled\n",
                                         a,
                                         aVBoxValues[a]);
                                aEnabled[a] = false;
                            }
                            else
                            {
                                Utf8StrFmt strTypeArg("scsitype%u", a);
                                if (findArgValue(strOverride, pmapArgs, strTypeArg))
                                {
                                    bstrFinalValue = strOverride;
                                    RTPrintf("%2u: SCSI controller, type set with --unit %u --scsitype: \"%ls\"\n",
                                            a,
                                            a,
                                            bstrFinalValue.raw());
                                }
                                else
                                    RTPrintf("%2u: SCSI controller, type %ls"
                                            "\n    (change with \"--vsys %u --unit %u --scsitype {BusLogic|LsiLogic}\";"
                                            "\n    disable with \"--vsys %u --unit %u --ignore\")\n",
                                            a,
                                            aVBoxValues[a],
                                            i, a, i, a);
                            }
                            break;

                        case VirtualSystemDescriptionType_HardDiskImage:
                            if (fIgnoreThis)
                            {
                                RTPrintf("%2u: Hard disk image: source image=%ls -- disabled\n",
                                         a,
                                         aOvfValues[a]);
                                aEnabled[a] = false;
                            }
                            else
                            {
                                Utf8StrFmt strTypeArg("disk%u", a);
                                RTCList<ImportOptions_T> optionsList = options.toList();

                                bstrFinalValue = aVBoxValues[a];

                                if (findArgValue(strOverride, pmapArgs, strTypeArg))
                                {
                                    if (!optionsList.contains(ImportOptions_ImportToVDI))
                                    {
                                        RTUUID uuid;
                                        /* Check if this is a uuid. If so, don't touch. */
                                        int vrc = RTUuidFromStr(&uuid, strOverride.c_str());
                                        if (vrc != VINF_SUCCESS)
                                        {
                                            /* Make the path absolute. */
                                            if (!RTPathStartsWithRoot(strOverride.c_str()))
                                            {
                                                char pszPwd[RTPATH_MAX];
                                                vrc = RTPathGetCurrent(pszPwd, RTPATH_MAX);
                                                if (RT_SUCCESS(vrc))
                                                    strOverride = Utf8Str(pszPwd).append(RTPATH_SLASH).append(strOverride);
                                            }
                                        }
                                        bstrFinalValue = strOverride;
                                    }
                                    else
                                    {
                                        //print some error about incompatible command-line arguments
                                        return errorSyntax(USAGE_IMPORTAPPLIANCE,
                                                           "Option --ImportToVDI shall not be used together with "
                                                           "manually set target path.");

                                    }

                                    RTPrintf("%2u: Hard disk image: source image=%ls, target path=%ls, %ls\n",
                                            a,
                                            aOvfValues[a],
                                            bstrFinalValue.raw(),
                                            aExtraConfigValues[a]);
                                }
#if 0 /* Changing the controller is fully valid, but the current design on how
         the params are evaluated here doesn't allow two parameter for one
         unit. The target disk path is more important I leave it for future
         improvments. */
                                Utf8StrFmt strTypeArg("controller%u", a);
                                if (findArgValue(strOverride, pmapArgs, strTypeArg))
                                {
                                    // strOverride now has the controller index as a number, but we
                                    // need a "controller=X" format string
                                    strOverride = Utf8StrFmt("controller=%s", strOverride.c_str());
                                    Bstr bstrExtraConfigValue = strOverride;
                                    bstrExtraConfigValue.detachTo(&aExtraConfigValues[a]);
                                    RTPrintf("%2u: Hard disk image: source image=%ls, target path=%ls, %ls\n",
                                            a,
                                            aOvfValues[a],
                                            aVBoxValues[a],
                                            aExtraConfigValues[a]);
                                }
#endif
                                else
                                {
                                    strOverride = aVBoxValues[a];

                                    /*
                                     * Current solution isn't optimal.
                                     * Better way is to provide API call for function
                                     * Appliance::i_findMediumFormatFromDiskImage()
                                     * and creating one new function which returns
                                     * struct ovf::DiskImage for currently processed disk.
                                    */

                                    /*
                                     * if user wants to convert all imported disks to VDI format
                                     * we need to replace files extensions to "vdi"
                                     * except CD/DVD disks
                                     */
                                    if (optionsList.contains(ImportOptions_ImportToVDI))
                                    {
                                        ComPtr<IVirtualBox> pVirtualBox = arg->virtualBox;
                                        ComPtr<ISystemProperties> systemProperties;
                                        com::SafeIfaceArray<IMediumFormat> mediumFormats;
                                        Bstr bstrFormatName;

                                        CHECK_ERROR(pVirtualBox,
                                                     COMGETTER(SystemProperties)(systemProperties.asOutParam()));

                                        CHECK_ERROR(systemProperties,
                                             COMGETTER(MediumFormats)(ComSafeArrayAsOutParam(mediumFormats)));

                                        /* go through all supported media formats and store files extensions only for RAW */
                                        com::SafeArray<BSTR> extensions;

                                        for (unsigned j = 0; j < mediumFormats.size(); ++j)
                                        {
                                            com::SafeArray<DeviceType_T> deviceType;
                                            ComPtr<IMediumFormat> mediumFormat = mediumFormats[j];
                                            CHECK_ERROR(mediumFormat, COMGETTER(Name)(bstrFormatName.asOutParam()));
                                            Utf8Str strFormatName = Utf8Str(bstrFormatName);

                                            if (strFormatName.compare("RAW", Utf8Str::CaseInsensitive) == 0)
                                            {
                                                /* getting files extensions for "RAW" format */
                                                CHECK_ERROR(mediumFormat,
                                                            DescribeFileExtensions(ComSafeArrayAsOutParam(extensions),
                                                                                   ComSafeArrayAsOutParam(deviceType)));
                                                break;
                                            }
                                        }

                                        /* go through files extensions for RAW format and compare them with
                                         * extension of current file
                                         */
                                        bool fReplace = true;

                                        const char *pszExtension = RTPathSuffix(strOverride.c_str());
                                        if (pszExtension)
                                            pszExtension++;

                                        for (unsigned j = 0; j < extensions.size(); ++j)
                                        {
                                            Bstr bstrExt(extensions[j]);
                                            Utf8Str strExtension(bstrExt);
                                            if(strExtension.compare(pszExtension, Utf8Str::CaseInsensitive) == 0)
                                            {
                                                fReplace = false;
                                                break;
                                            }
                                        }

                                        if (fReplace)
                                        {
                                            strOverride = strOverride.stripSuffix();
                                            strOverride = strOverride.append(".").append("vdi");
                                        }
                                    }

                                    bstrFinalValue = strOverride;

                                    RTPrintf("%2u: Hard disk image: source image=%ls, target path=%ls, %ls"
                                            "\n    (change target path with \"--vsys %u --unit %u --disk path\";"
                                            "\n    disable with \"--vsys %u --unit %u --ignore\")\n",
                                            a,
                                            aOvfValues[a],
                                            bstrFinalValue.raw(),
                                            aExtraConfigValues[a],
                                            i, a, i, a);
                                }
                            }
                            break;

                        case VirtualSystemDescriptionType_CDROM:
                            if (fIgnoreThis)
                            {
                                RTPrintf("%2u: CD-ROM -- disabled\n",
                                         a);
                                aEnabled[a] = false;
                            }
                            else
                                RTPrintf("%2u: CD-ROM"
                                        "\n    (disable with \"--vsys %u --unit %u --ignore\")\n",
                                        a, i, a);
                            break;

                        case VirtualSystemDescriptionType_Floppy:
                            if (fIgnoreThis)
                            {
                                RTPrintf("%2u: Floppy -- disabled\n",
                                         a);
                                aEnabled[a] = false;
                            }
                            else
                                RTPrintf("%2u: Floppy"
                                        "\n    (disable with \"--vsys %u --unit %u --ignore\")\n",
                                        a, i, a);
                            break;

                        case VirtualSystemDescriptionType_NetworkAdapter:
                            RTPrintf("%2u: Network adapter: orig %ls, config %ls, extra %ls\n",   /// @todo implement once we have a plan for the back-end
                                     a,
                                     aOvfValues[a],
                                     aVBoxValues[a],
                                     aExtraConfigValues[a]);
                            break;

                        case VirtualSystemDescriptionType_USBController:
                            if (fIgnoreThis)
                            {
                                RTPrintf("%2u: USB controller -- disabled\n",
                                         a);
                                aEnabled[a] = false;
                            }
                            else
                                RTPrintf("%2u: USB controller"
                                        "\n    (disable with \"--vsys %u --unit %u --ignore\")\n",
                                        a, i, a);
                            break;

                        case VirtualSystemDescriptionType_SoundCard:
                            if (fIgnoreThis)
                            {
                               RTPrintf("%2u: Sound card \"%ls\" -- disabled\n",
                                         a,
                                         aOvfValues[a]);
                                aEnabled[a] = false;
                            }
                            else
                                RTPrintf("%2u: Sound card (appliance expects \"%ls\", can change on import)"
                                        "\n    (disable with \"--vsys %u --unit %u --ignore\")\n",
                                        a,
                                        aOvfValues[a],
                                        i,
                                        a);
                            break;

                        case VirtualSystemDescriptionType_SettingsFile:
                            /** @todo  VirtualSystemDescriptionType_SettingsFile? */
                            break;
                        case VirtualSystemDescriptionType_Miscellaneous:
                            /** @todo  VirtualSystemDescriptionType_Miscellaneous? */
                            break;
                        case VirtualSystemDescriptionType_Ignore:
                            break;
                    }

                    bstrFinalValue.detachTo(&aFinalValues[a]);
                }

                if (fExecute)
                    CHECK_ERROR_BREAK(aVirtualSystemDescriptions[i],
                                      SetFinalValues(ComSafeArrayAsInParam(aEnabled),
                                                     ComSafeArrayAsInParam(aFinalValues),
                                                     ComSafeArrayAsInParam(aExtraConfigValues)));

            } // for (unsigned i = 0; i < cVirtualSystemDescriptions; ++i)

            if (cLicensesInTheWay == 1)
                RTMsgError("Cannot import until the license agreement listed above is accepted.");
            else if (cLicensesInTheWay > 1)
                RTMsgError("Cannot import until the %c license agreements listed above are accepted.", cLicensesInTheWay);

            if (!cLicensesInTheWay && fExecute)
            {
                // go!
                ComPtr<IProgress> progress;
                CHECK_ERROR_BREAK(pAppliance,
                                  ImportMachines(ComSafeArrayAsInParam(options), progress.asOutParam()));

                rc = showProgress(progress);
                CHECK_PROGRESS_ERROR_RET(progress, ("Appliance import failed"), RTEXITCODE_FAILURE);

                if (SUCCEEDED(rc))
                    RTPrintf("Successfully imported the appliance.\n");
            }
        } // end if (aVirtualSystemDescriptions.size() > 0)
    } while (0);

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static int parseExportOptions(const char *psz, com::SafeArray<ExportOptions_T> *options)
{
    int rc = VINF_SUCCESS;
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
            if (!RTStrNICmp(psz, "CreateManifest", len))
                options->push_back(ExportOptions_CreateManifest);
            else if (!RTStrNICmp(psz, "manifest", len))
                options->push_back(ExportOptions_CreateManifest);
            else if (!RTStrNICmp(psz, "ExportDVDImages", len))
                options->push_back(ExportOptions_ExportDVDImages);
            else if (!RTStrNICmp(psz, "iso", len))
                options->push_back(ExportOptions_ExportDVDImages);
            else if (!RTStrNICmp(psz, "StripAllMACs", len))
                options->push_back(ExportOptions_StripAllMACs);
            else if (!RTStrNICmp(psz, "nomacs", len))
                options->push_back(ExportOptions_StripAllMACs);
            else if (!RTStrNICmp(psz, "StripAllNonNATMACs", len))
                options->push_back(ExportOptions_StripAllNonNATMACs);
            else if (!RTStrNICmp(psz, "nomacsbutnat", len))
                options->push_back(ExportOptions_StripAllNonNATMACs);
            else
                rc = VERR_PARSE_ERROR;
        }
        if (pszComma)
            psz += len + 1;
        else
            psz += len;
    }

    return rc;
}

static const RTGETOPTDEF g_aExportOptions[] =
{
    { "--output",               'o', RTGETOPT_REQ_STRING },
    { "--legacy09",             'l', RTGETOPT_REQ_NOTHING },
    { "--ovf09",                'l', RTGETOPT_REQ_NOTHING },
    { "--ovf10",                '1', RTGETOPT_REQ_NOTHING },
    { "--ovf20",                '2', RTGETOPT_REQ_NOTHING },
    { "--opc10",                'c', RTGETOPT_REQ_NOTHING },
    { "--manifest",             'm', RTGETOPT_REQ_NOTHING },    // obsoleted by --options
    { "--iso",                  'I', RTGETOPT_REQ_NOTHING },    // obsoleted by --options
    { "--vsys",                 's', RTGETOPT_REQ_UINT32 },
    { "--product",              'p', RTGETOPT_REQ_STRING },
    { "--producturl",           'P', RTGETOPT_REQ_STRING },
    { "--vendor",               'n', RTGETOPT_REQ_STRING },
    { "--vendorurl",            'N', RTGETOPT_REQ_STRING },
    { "--version",              'v', RTGETOPT_REQ_STRING },
    { "--description",          'd', RTGETOPT_REQ_STRING },
    { "--eula",                 'e', RTGETOPT_REQ_STRING },
    { "--eulafile",             'E', RTGETOPT_REQ_STRING },
    { "--options",              'O', RTGETOPT_REQ_STRING },
};

RTEXITCODE handleExportAppliance(HandlerArg *a)
{
    HRESULT rc = S_OK;

    Utf8Str strOutputFile;
    Utf8Str strOvfFormat("ovf-1.0"); // the default export version
    bool fManifest = false; // the default
    bool fExportISOImages = false; // the default
    com::SafeArray<ExportOptions_T> options;
    std::list< ComPtr<IMachine> > llMachines;

    uint32_t ulCurVsys = (uint32_t)-1;
    // for each --vsys X command, maintain a map of command line items
    ArgsMapsMap mapArgsMapsPerVsys;
    do
    {
        int c;

        RTGETOPTUNION ValueUnion;
        RTGETOPTSTATE GetState;
        // start at 0 because main() has hacked both the argc and argv given to us
        RTGetOptInit(&GetState, a->argc, a->argv, g_aExportOptions,
                     RT_ELEMENTS(g_aExportOptions), 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);

        Utf8Str strProductUrl;
        while ((c = RTGetOpt(&GetState, &ValueUnion)))
        {
            switch (c)
            {
                case 'o':   // --output
                    if (strOutputFile.length())
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "You can only specify --output once.");
                    else
                        strOutputFile = ValueUnion.psz;
                    break;

                case 'l':   // --legacy09/--ovf09
                    strOvfFormat = "ovf-0.9";
                    break;

                case '1':   // --ovf10
                    strOvfFormat = "ovf-1.0";
                    break;

                case '2':   // --ovf20
                    strOvfFormat = "ovf-2.0";
                    break;

                case 'c':   // --opc
                    strOvfFormat = "opc-1.0";
                    break;

                case 'I':   // --iso
                    fExportISOImages = true;
                    break;

                case 'm':   // --manifest
                    fManifest = true;
                    break;

                case 's':   // --vsys
                    ulCurVsys = ValueUnion.u32;
                    break;

                case 'p':   // --product
                    if (ulCurVsys == (uint32_t)-1)
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                    mapArgsMapsPerVsys[ulCurVsys]["product"] = ValueUnion.psz;
                    break;

                case 'P':   // --producturl
                    if (ulCurVsys == (uint32_t)-1)
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                    mapArgsMapsPerVsys[ulCurVsys]["producturl"] = ValueUnion.psz;
                    break;

                case 'n':   // --vendor
                    if (ulCurVsys == (uint32_t)-1)
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                    mapArgsMapsPerVsys[ulCurVsys]["vendor"] = ValueUnion.psz;
                    break;

                case 'N':   // --vendorurl
                    if (ulCurVsys == (uint32_t)-1)
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                    mapArgsMapsPerVsys[ulCurVsys]["vendorurl"] = ValueUnion.psz;
                    break;

                case 'v':   // --version
                    if (ulCurVsys == (uint32_t)-1)
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                    mapArgsMapsPerVsys[ulCurVsys]["version"] = ValueUnion.psz;
                    break;

                case 'd':   // --description
                    if (ulCurVsys == (uint32_t)-1)
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                    mapArgsMapsPerVsys[ulCurVsys]["description"] = ValueUnion.psz;
                    break;

                case 'e':   // --eula
                    if (ulCurVsys == (uint32_t)-1)
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                    mapArgsMapsPerVsys[ulCurVsys]["eula"] = ValueUnion.psz;
                    break;

                case 'E':   // --eulafile
                    if (ulCurVsys == (uint32_t)-1)
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "Option \"%s\" requires preceding --vsys argument.", GetState.pDef->pszLong);
                    mapArgsMapsPerVsys[ulCurVsys]["eulafile"] = ValueUnion.psz;
                    break;

                case 'O':   // --options
                    if (RT_FAILURE(parseExportOptions(ValueUnion.psz, &options)))
                        return errorArgument("Invalid export options '%s'\n", ValueUnion.psz);
                    break;

                case VINF_GETOPT_NOT_OPTION:
                {
                    Utf8Str strMachine(ValueUnion.psz);
                    // must be machine: try UUID or name
                    ComPtr<IMachine> machine;
                    CHECK_ERROR_BREAK(a->virtualBox, FindMachine(Bstr(strMachine).raw(),
                                                                 machine.asOutParam()));
                    if (machine)
                        llMachines.push_back(machine);
                    break;
                }

                default:
                    if (c > 0)
                    {
                        if (RT_C_IS_GRAPH(c))
                            return errorSyntax(USAGE_EXPORTAPPLIANCE, "unhandled option: -%c", c);
                        else
                            return errorSyntax(USAGE_EXPORTAPPLIANCE, "unhandled option: %i", c);
                    }
                    else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "unknown option: %s", ValueUnion.psz);
                    else if (ValueUnion.pDef)
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                    else
                        return errorSyntax(USAGE_EXPORTAPPLIANCE, "%Rrs", c);
            }

            if (FAILED(rc))
                break;
        }

        if (FAILED(rc))
            break;

        if (llMachines.size() == 0)
            return errorSyntax(USAGE_EXPORTAPPLIANCE, "At least one machine must be specified with the export command.");
        if (!strOutputFile.length())
            return errorSyntax(USAGE_EXPORTAPPLIANCE, "Missing --output argument with export command.");

        // match command line arguments with the machines count
        // this is only to sort out invalid indices at this time
        ArgsMapsMap::const_iterator it;
        for (it = mapArgsMapsPerVsys.begin();
             it != mapArgsMapsPerVsys.end();
             ++it)
        {
            uint32_t ulVsys = it->first;
            if (ulVsys >= llMachines.size())
                return errorSyntax(USAGE_EXPORTAPPLIANCE,
                                   "Invalid index %RI32 with -vsys option; you specified only %zu virtual system(s).",
                                   ulVsys, llMachines.size());
        }

        ComPtr<IAppliance> pAppliance;
        CHECK_ERROR_BREAK(a->virtualBox, CreateAppliance(pAppliance.asOutParam()));

        char *pszAbsFilePath = 0;
        if (strOutputFile.startsWith("S3://", RTCString::CaseInsensitive) ||
            strOutputFile.startsWith("SunCloud://", RTCString::CaseInsensitive) ||
            strOutputFile.startsWith("webdav://", RTCString::CaseInsensitive))
            pszAbsFilePath = RTStrDup(strOutputFile.c_str());
        else
            pszAbsFilePath = RTPathAbsDup(strOutputFile.c_str());

        std::list< ComPtr<IMachine> >::iterator itM;
        uint32_t i=0;
        for (itM = llMachines.begin();
             itM != llMachines.end();
             ++itM, ++i)
        {
            ComPtr<IMachine> pMachine = *itM;
            ComPtr<IVirtualSystemDescription> pVSD;
            CHECK_ERROR_BREAK(pMachine, ExportTo(pAppliance, Bstr(pszAbsFilePath).raw(), pVSD.asOutParam()));
            // Add additional info to the virtual system description if the user wants so
            ArgsMap *pmapArgs = NULL;
            ArgsMapsMap::iterator itm = mapArgsMapsPerVsys.find(i);
            if (itm != mapArgsMapsPerVsys.end())
                pmapArgs = &itm->second;
            if (pmapArgs)
            {
                ArgsMap::iterator itD;
                for (itD = pmapArgs->begin();
                     itD != pmapArgs->end();
                     ++itD)
                {
                    if (itD->first == "product")
                        pVSD->AddDescription(VirtualSystemDescriptionType_Product,
                                             Bstr(itD->second).raw(),
                                             Bstr(itD->second).raw());
                    else if (itD->first == "producturl")
                        pVSD->AddDescription(VirtualSystemDescriptionType_ProductUrl,
                                             Bstr(itD->second).raw(),
                                             Bstr(itD->second).raw());
                    else if (itD->first == "vendor")
                        pVSD->AddDescription(VirtualSystemDescriptionType_Vendor,
                                             Bstr(itD->second).raw(),
                                             Bstr(itD->second).raw());
                    else if (itD->first == "vendorurl")
                        pVSD->AddDescription(VirtualSystemDescriptionType_VendorUrl,
                                             Bstr(itD->second).raw(),
                                             Bstr(itD->second).raw());
                    else if (itD->first == "version")
                        pVSD->AddDescription(VirtualSystemDescriptionType_Version,
                                             Bstr(itD->second).raw(),
                                             Bstr(itD->second).raw());
                    else if (itD->first == "description")
                        pVSD->AddDescription(VirtualSystemDescriptionType_Description,
                                             Bstr(itD->second).raw(),
                                             Bstr(itD->second).raw());
                    else if (itD->first == "eula")
                        pVSD->AddDescription(VirtualSystemDescriptionType_License,
                                             Bstr(itD->second).raw(),
                                             Bstr(itD->second).raw());
                    else if (itD->first == "eulafile")
                    {
                        Utf8Str strContent;
                        void *pvFile;
                        size_t cbFile;
                        int irc = RTFileReadAll(itD->second.c_str(), &pvFile, &cbFile);
                        if (RT_SUCCESS(irc))
                        {
                            Bstr bstrContent((char*)pvFile, cbFile);
                            pVSD->AddDescription(VirtualSystemDescriptionType_License,
                                                 bstrContent.raw(),
                                                 bstrContent.raw());
                            RTFileReadAllFree(pvFile, cbFile);
                        }
                        else
                        {
                            RTMsgError("Cannot read license file \"%s\" which should be included in the virtual system %u.",
                                       itD->second.c_str(), i);
                            return RTEXITCODE_FAILURE;
                        }
                    }
                }
            }
        }

        if (FAILED(rc))
            break;

        /* Query required passwords and supply them to the appliance. */
        com::SafeArray<BSTR> aIdentifiers;

        CHECK_ERROR_BREAK(pAppliance, GetPasswordIds(ComSafeArrayAsOutParam(aIdentifiers)));

        if (aIdentifiers.size() > 0)
        {
            com::SafeArray<BSTR> aPasswords(aIdentifiers.size());
            RTPrintf("Enter the passwords for the following identifiers to export the apppliance:\n");
            for (unsigned idxId = 0; idxId < aIdentifiers.size(); idxId++)
            {
                com::Utf8Str strPassword;
                Bstr bstrPassword;
                Bstr bstrId = aIdentifiers[idxId];

                RTEXITCODE rcExit = readPasswordFromConsole(&strPassword, "Password ID %s:", Utf8Str(bstrId).c_str());
                if (rcExit == RTEXITCODE_FAILURE)
                {
                    RTStrFree(pszAbsFilePath);
                    return rcExit;
                }

                bstrPassword = strPassword;
                bstrPassword.detachTo(&aPasswords[idxId]);
            }

            CHECK_ERROR_BREAK(pAppliance, AddPasswords(ComSafeArrayAsInParam(aIdentifiers),
                                                       ComSafeArrayAsInParam(aPasswords)));
        }

        if (fManifest)
            options.push_back(ExportOptions_CreateManifest);

        if (fExportISOImages)
            options.push_back(ExportOptions_ExportDVDImages);

        ComPtr<IProgress> progress;
        CHECK_ERROR_BREAK(pAppliance, Write(Bstr(strOvfFormat).raw(),
                                            ComSafeArrayAsInParam(options),
                                            Bstr(pszAbsFilePath).raw(),
                                            progress.asOutParam()));
        RTStrFree(pszAbsFilePath);

        rc = showProgress(progress);
        CHECK_PROGRESS_ERROR_RET(progress, ("Appliance write failed"), RTEXITCODE_FAILURE);

        if (SUCCEEDED(rc))
            RTPrintf("Successfully exported %d machine(s).\n", llMachines.size());

    } while (0);

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

#endif /* !VBOX_ONLY_DOCS */
