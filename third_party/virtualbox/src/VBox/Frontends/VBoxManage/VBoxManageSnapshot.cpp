/* $Id: VBoxManageSnapshot.cpp $ */
/** @file
 * VBoxManage - The 'snapshot' command.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>

#include <VBox/com/VirtualBox.h>

#include <iprt/stream.h>
#include <iprt/getopt.h>
#include <iprt/time.h>

#include "VBoxManage.h"
using namespace com;

/**
 * Helper function used with "VBoxManage snapshot ... dump". Gets called to find the
 * snapshot in the machine's snapshot tree that uses a particular diff image child of
 * a medium.
 * Horribly inefficient since we keep re-querying the snapshots tree for each image,
 * but this is for quick debugging only.
 * @param pMedium
 * @param pThisSnapshot
 * @param pCurrentSnapshot
 * @param uMediumLevel
 * @param uSnapshotLevel
 * @return
 */
bool FindAndPrintSnapshotUsingMedium(ComPtr<IMedium> &pMedium,
                                     ComPtr<ISnapshot> &pThisSnapshot,
                                     ComPtr<ISnapshot> &pCurrentSnapshot,
                                     uint32_t uMediumLevel,
                                     uint32_t uSnapshotLevel)
{
    HRESULT rc;

    do
    {
        // get snapshot machine so we can figure out which diff image this created
        ComPtr<IMachine> pSnapshotMachine;
        CHECK_ERROR_BREAK(pThisSnapshot, COMGETTER(Machine)(pSnapshotMachine.asOutParam()));

        // get media attachments
        SafeIfaceArray<IMediumAttachment> aAttachments;
        CHECK_ERROR_BREAK(pSnapshotMachine, COMGETTER(MediumAttachments)(ComSafeArrayAsOutParam(aAttachments)));

        for (uint32_t i = 0;
             i < aAttachments.size();
             ++i)
        {
            ComPtr<IMediumAttachment> pAttach(aAttachments[i]);
            DeviceType_T type;
            CHECK_ERROR_BREAK(pAttach, COMGETTER(Type)(&type));
            if (type == DeviceType_HardDisk)
            {
                ComPtr<IMedium> pMediumInSnapshot;
                CHECK_ERROR_BREAK(pAttach, COMGETTER(Medium)(pMediumInSnapshot.asOutParam()));

                if (pMediumInSnapshot == pMedium)
                {
                    // get snapshot name
                    Bstr bstrSnapshotName;
                    CHECK_ERROR_BREAK(pThisSnapshot, COMGETTER(Name)(bstrSnapshotName.asOutParam()));

                    RTPrintf("%*s  \"%ls\"%s\n",
                             50 + uSnapshotLevel * 2, "",            // indent
                             bstrSnapshotName.raw(),
                             (pThisSnapshot == pCurrentSnapshot) ? " (CURSNAP)" : "");
                    return true;        // found
                }
            }
        }

        // not found: then recurse into child snapshots
        SafeIfaceArray<ISnapshot> aSnapshots;
        CHECK_ERROR_BREAK(pThisSnapshot, COMGETTER(Children)(ComSafeArrayAsOutParam(aSnapshots)));

        for (uint32_t i = 0;
            i < aSnapshots.size();
            ++i)
        {
            ComPtr<ISnapshot> pChild(aSnapshots[i]);
            if (FindAndPrintSnapshotUsingMedium(pMedium,
                                                pChild,
                                                pCurrentSnapshot,
                                                uMediumLevel,
                                                uSnapshotLevel + 1))
                // found:
                break;
        }
    } while (0);

    return false;
}

/**
 * Helper function used with "VBoxManage snapshot ... dump". Called from DumpSnapshot()
 * for each hard disk attachment found in a virtual machine. This then writes out the
 * root (base) medium for that hard disk attachment and recurses into the children
 * tree of that medium, correlating it with the snapshots of the machine.
 * @param pCurrentStateMedium constant, the medium listed in the current machine data (latest diff image).
 * @param pMedium variant, initially the base medium, then a child of the base medium when recursing.
 * @param pRootSnapshot constant, the root snapshot of the machine, if any; this then looks into the child snapshots.
 * @param pCurrentSnapshot constant, the machine's current snapshot (so we can mark it in the output).
 * @param uLevel variant, the recursion level for output indentation.
 */
void DumpMediumWithChildren(ComPtr<IMedium> &pCurrentStateMedium,
                            ComPtr<IMedium> &pMedium,
                            ComPtr<ISnapshot> &pRootSnapshot,
                            ComPtr<ISnapshot> &pCurrentSnapshot,
                            uint32_t uLevel)
{
    HRESULT rc;
    do
    {
        // print this medium
        Bstr bstrMediumName;
        CHECK_ERROR_BREAK(pMedium, COMGETTER(Name)(bstrMediumName.asOutParam()));
        RTPrintf("%*s  \"%ls\"%s\n",
                 uLevel * 2, "",            // indent
                 bstrMediumName.raw(),
                 (pCurrentStateMedium == pMedium) ? " (CURSTATE)" : "");

        // find and print the snapshot that uses this particular medium (diff image)
        FindAndPrintSnapshotUsingMedium(pMedium, pRootSnapshot, pCurrentSnapshot, uLevel, 0);

        // recurse into children
        SafeIfaceArray<IMedium> aChildren;
        CHECK_ERROR_BREAK(pMedium, COMGETTER(Children)(ComSafeArrayAsOutParam(aChildren)));
        for (uint32_t i = 0;
             i < aChildren.size();
             ++i)
        {
            ComPtr<IMedium> pChild(aChildren[i]);
            DumpMediumWithChildren(pCurrentStateMedium, pChild, pRootSnapshot, pCurrentSnapshot, uLevel + 1);
        }
    } while (0);
}


/**
 * Handles the 'snapshot myvm list' sub-command.
 * @returns Exit code.
 * @param   pArgs           The handler argument package.
 * @param   pMachine        Reference to the VM (locked) we're operating on.
 */
static RTEXITCODE handleSnapshotList(HandlerArg *pArgs, ComPtr<IMachine> &pMachine)
{
    static const RTGETOPTDEF g_aOptions[] =
    {
        { "--details",          'D', RTGETOPT_REQ_NOTHING },
        { "--machinereadable",  'M', RTGETOPT_REQ_NOTHING },
    };

    VMINFO_DETAILS enmDetails = VMINFO_STANDARD;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, pArgs->argc, pArgs->argv, g_aOptions, RT_ELEMENTS(g_aOptions), 2 /*iArg*/, 0 /*fFlags*/);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'D':   enmDetails = VMINFO_FULL; break;
            case 'M':   enmDetails = VMINFO_MACHINEREADABLE; break;
            default:    return errorGetOpt(USAGE_SNAPSHOT, c, &ValueUnion);
        }
    }

    ComPtr<ISnapshot> pSnapshot;
    HRESULT hrc = pMachine->FindSnapshot(Bstr().raw(), pSnapshot.asOutParam());
    if (FAILED(hrc))
    {
        RTPrintf("This machine does not have any snapshots\n");
        return RTEXITCODE_FAILURE;
    }
    if (pSnapshot)
    {
        ComPtr<ISnapshot> pCurrentSnapshot;
        CHECK_ERROR2I_RET(pMachine, COMGETTER(CurrentSnapshot)(pCurrentSnapshot.asOutParam()), RTEXITCODE_FAILURE);
        hrc = showSnapshots(pSnapshot, pCurrentSnapshot, enmDetails);
        if (FAILED(hrc))
            return RTEXITCODE_FAILURE;
    }
    return RTEXITCODE_SUCCESS;
}

/**
 * Implementation for "VBoxManage snapshot ... dump". This goes thru the machine's
 * medium attachments and calls DumpMediumWithChildren() for each hard disk medium found,
 * which then dumps the parent/child tree of that medium together with the corresponding
 * snapshots.
 * @param pMachine Machine to dump snapshots for.
 */
void DumpSnapshot(ComPtr<IMachine> &pMachine)
{
    HRESULT rc;

    do
    {
        // get root snapshot
        ComPtr<ISnapshot> pSnapshot;
        CHECK_ERROR_BREAK(pMachine, FindSnapshot(Bstr("").raw(), pSnapshot.asOutParam()));

        // get current snapshot
        ComPtr<ISnapshot> pCurrentSnapshot;
        CHECK_ERROR_BREAK(pMachine, COMGETTER(CurrentSnapshot)(pCurrentSnapshot.asOutParam()));

        // get media attachments
        SafeIfaceArray<IMediumAttachment> aAttachments;
        CHECK_ERROR_BREAK(pMachine, COMGETTER(MediumAttachments)(ComSafeArrayAsOutParam(aAttachments)));
        for (uint32_t i = 0;
             i < aAttachments.size();
             ++i)
        {
            ComPtr<IMediumAttachment> pAttach(aAttachments[i]);
            DeviceType_T type;
            CHECK_ERROR_BREAK(pAttach, COMGETTER(Type)(&type));
            if (type == DeviceType_HardDisk)
            {
                ComPtr<IMedium> pCurrentStateMedium;
                CHECK_ERROR_BREAK(pAttach, COMGETTER(Medium)(pCurrentStateMedium.asOutParam()));

                ComPtr<IMedium> pBaseMedium;
                CHECK_ERROR_BREAK(pCurrentStateMedium, COMGETTER(Base)(pBaseMedium.asOutParam()));

                Bstr bstrBaseMediumName;
                CHECK_ERROR_BREAK(pBaseMedium, COMGETTER(Name)(bstrBaseMediumName.asOutParam()));

                RTPrintf("[%RI32] Images and snapshots for medium \"%ls\"\n", i, bstrBaseMediumName.raw());

                DumpMediumWithChildren(pCurrentStateMedium,
                                       pBaseMedium,
                                       pSnapshot,
                                       pCurrentSnapshot,
                                       0);
            }
        }
    } while (0);
}

typedef enum SnapshotUniqueFlags
{
    SnapshotUniqueFlags_Null = 0,
    SnapshotUniqueFlags_Number = RT_BIT(1),
    SnapshotUniqueFlags_Timestamp = RT_BIT(2),
    SnapshotUniqueFlags_Space = RT_BIT(16),
    SnapshotUniqueFlags_Force = RT_BIT(30)
} SnapshotUniqueFlags;

static int parseSnapshotUniqueFlags(const char *psz, SnapshotUniqueFlags *pUnique)
{
    int rc = VINF_SUCCESS;
    unsigned uUnique = 0;
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
            if (!RTStrNICmp(psz, "number", len))
                uUnique |= SnapshotUniqueFlags_Number;
            else if (!RTStrNICmp(psz, "timestamp", len))
                uUnique |= SnapshotUniqueFlags_Timestamp;
            else if (!RTStrNICmp(psz, "space", len))
                uUnique |= SnapshotUniqueFlags_Space;
            else if (!RTStrNICmp(psz, "force", len))
                uUnique |= SnapshotUniqueFlags_Force;
            else
                rc = VERR_PARSE_ERROR;
        }
        if (pszComma)
            psz += len + 1;
        else
            psz += len;
    }

    if (RT_SUCCESS(rc))
        *pUnique = (SnapshotUniqueFlags)uUnique;
    return rc;
}

/**
 * Implementation for all VBoxManage snapshot ... subcommands.
 * @param a
 * @return
 */
RTEXITCODE handleSnapshot(HandlerArg *a)
{
    HRESULT rc;

    /* we need at least a VM and a command */
    if (a->argc < 2)
        return errorSyntax(USAGE_SNAPSHOT, "Not enough parameters");

    /* the first argument must be the VM */
    Bstr bstrMachine(a->argv[0]);
    ComPtr<IMachine> pMachine;
    CHECK_ERROR(a->virtualBox, FindMachine(bstrMachine.raw(),
                                           pMachine.asOutParam()));
    if (!pMachine)
        return RTEXITCODE_FAILURE;

    /* we have to open a session for this task (new or shared) */
    CHECK_ERROR_RET(pMachine, LockMachine(a->session, LockType_Shared), RTEXITCODE_FAILURE);
    do
    {
        /* replace the (read-only) IMachine object by a writable one */
        ComPtr<IMachine> sessionMachine;
        CHECK_ERROR_BREAK(a->session, COMGETTER(Machine)(sessionMachine.asOutParam()));

        /* switch based on the command */
        bool fDelete = false,
             fRestore = false,
             fRestoreCurrent = false;

        if (!strcmp(a->argv[1], "take"))
        {
            /* there must be a name */
            if (a->argc < 3)
            {
                errorSyntax(USAGE_SNAPSHOT, "Missing snapshot name");
                rc = E_FAIL;
                break;
            }
            Bstr name(a->argv[2]);

            /* parse the optional arguments */
            Bstr desc;
            bool fPause = true; /* default is NO live snapshot */
            SnapshotUniqueFlags enmUnique = SnapshotUniqueFlags_Null;
            static const RTGETOPTDEF s_aTakeOptions[] =
            {
                { "--description", 'd', RTGETOPT_REQ_STRING },
                { "-description",  'd', RTGETOPT_REQ_STRING },
                { "-desc",         'd', RTGETOPT_REQ_STRING },
                { "--pause",       'p', RTGETOPT_REQ_NOTHING },
                { "--live",        'l', RTGETOPT_REQ_NOTHING },
                { "--uniquename",  'u', RTGETOPT_REQ_STRING }
            };
            RTGETOPTSTATE GetOptState;
            RTGetOptInit(&GetOptState, a->argc, a->argv, s_aTakeOptions, RT_ELEMENTS(s_aTakeOptions),
                         3, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
            int ch;
            RTGETOPTUNION Value;
            int vrc;
            while (   SUCCEEDED(rc)
                   && (ch = RTGetOpt(&GetOptState, &Value)))
            {
                switch (ch)
                {
                    case 'p':
                        fPause = true;
                        break;

                    case 'l':
                        fPause = false;
                        break;

                    case 'd':
                        desc = Value.psz;
                        break;

                    case 'u':
                        vrc = parseSnapshotUniqueFlags(Value.psz, &enmUnique);
                        if (RT_FAILURE(vrc))
                            return errorArgument("Invalid unique name description '%s'", Value.psz);
                        break;

                    default:
                        errorGetOpt(USAGE_SNAPSHOT, ch, &Value);
                        rc = E_FAIL;
                        break;
                }
            }
            if (FAILED(rc))
                break;

            if (enmUnique & (SnapshotUniqueFlags_Number | SnapshotUniqueFlags_Timestamp))
            {
                ComPtr<ISnapshot> pSnapshot;
                rc = sessionMachine->FindSnapshot(name.raw(),
                                                  pSnapshot.asOutParam());
                if (SUCCEEDED(rc) || (enmUnique & SnapshotUniqueFlags_Force))
                {
                    /* there is a duplicate, need to create a unique name */
                    uint32_t count = 0;
                    RTTIMESPEC now;

                    if (enmUnique & SnapshotUniqueFlags_Number)
                    {
                        if (enmUnique & SnapshotUniqueFlags_Force)
                            count = 1;
                        else
                            count = 2;
                        RTTimeSpecSetNano(&now, 0); /* Shut up MSC */
                    }
                    else
                        RTTimeNow(&now);

                    while (count < 500)
                    {
                        Utf8Str suffix;
                        if (enmUnique & SnapshotUniqueFlags_Number)
                            suffix = Utf8StrFmt("%u", count);
                        else
                        {
                            RTTIMESPEC nowplus = now;
                            RTTimeSpecAddSeconds(&nowplus, count);
                            RTTIME stamp;
                            RTTimeExplode(&stamp, &nowplus);
                            suffix = Utf8StrFmt("%04u-%02u-%02uT%02u:%02u:%02uZ", stamp.i32Year, stamp.u8Month, stamp.u8MonthDay, stamp.u8Hour, stamp.u8Minute, stamp.u8Second);
                        }
                        Bstr tryName = name;
                        if (enmUnique & SnapshotUniqueFlags_Space)
                            tryName = BstrFmt("%ls %s", name.raw(), suffix.c_str());
                        else
                            tryName = BstrFmt("%ls%s", name.raw(), suffix.c_str());
                        count++;
                        rc = sessionMachine->FindSnapshot(tryName.raw(),
                                                          pSnapshot.asOutParam());
                        if (FAILED(rc))
                        {
                            name = tryName;
                            break;
                        }
                    }
                    if (SUCCEEDED(rc))
                    {
                        errorArgument("Failed to generate a unique snapshot name");
                        rc = E_FAIL;
                        break;
                    }
                }
                rc = S_OK;
            }

            ComPtr<IProgress> progress;
            Bstr snapId;
            CHECK_ERROR_BREAK(sessionMachine, TakeSnapshot(name.raw(), desc.raw(),
                                                           fPause, snapId.asOutParam(),
                                                           progress.asOutParam()));

            rc = showProgress(progress);
            if (SUCCEEDED(rc))
                RTPrintf("Snapshot taken. UUID: %ls\n", snapId.raw());
            else
                CHECK_PROGRESS_ERROR(progress, ("Failed to take snapshot"));
        }
        else if (    (fDelete = !strcmp(a->argv[1], "delete"))
                  || (fRestore = !strcmp(a->argv[1], "restore"))
                  || (fRestoreCurrent = !strcmp(a->argv[1], "restorecurrent"))
                )
        {
            if (fRestoreCurrent)
            {
                if (a->argc > 2)
                {
                    errorSyntax(USAGE_SNAPSHOT, "Too many arguments");
                    rc = E_FAIL;
                    break;
                }
            }
            /* exactly one parameter: snapshot name */
            else if (a->argc != 3)
            {
                errorSyntax(USAGE_SNAPSHOT, "Expecting snapshot name only");
                rc = E_FAIL;
                break;
            }

            ComPtr<ISnapshot> pSnapshot;

            if (fRestoreCurrent)
            {
                CHECK_ERROR_BREAK(sessionMachine, COMGETTER(CurrentSnapshot)(pSnapshot.asOutParam()));
                if (pSnapshot.isNull())
                {
                    RTPrintf("This machine does not have any snapshots\n");
                    return RTEXITCODE_FAILURE;
                }
            }
            else
            {
                // restore or delete snapshot: then resolve cmd line argument to snapshot instance
                CHECK_ERROR_BREAK(sessionMachine, FindSnapshot(Bstr(a->argv[2]).raw(),
                                                               pSnapshot.asOutParam()));
            }

            Bstr bstrSnapGuid;
            CHECK_ERROR_BREAK(pSnapshot, COMGETTER(Id)(bstrSnapGuid.asOutParam()));

            Bstr bstrSnapName;
            CHECK_ERROR_BREAK(pSnapshot, COMGETTER(Name)(bstrSnapName.asOutParam()));

            ComPtr<IProgress> pProgress;

            RTPrintf("%s snapshot '%ls' (%ls)\n",
                     fDelete ? "Deleting" : "Restoring", bstrSnapName.raw(), bstrSnapGuid.raw());

            if (fDelete)
            {
                CHECK_ERROR_BREAK(sessionMachine, DeleteSnapshot(bstrSnapGuid.raw(),
                                                                 pProgress.asOutParam()));
            }
            else
            {
                // restore or restore current
                CHECK_ERROR_BREAK(sessionMachine, RestoreSnapshot(pSnapshot, pProgress.asOutParam()));
            }

            rc = showProgress(pProgress);
            CHECK_PROGRESS_ERROR(pProgress, ("Snapshot operation failed"));
        }
        else if (!strcmp(a->argv[1], "edit"))
        {
            if (a->argc < 3)
            {
                errorSyntax(USAGE_SNAPSHOT, "Missing snapshot name");
                rc = E_FAIL;
                break;
            }

            ComPtr<ISnapshot> pSnapshot;

            if (   !strcmp(a->argv[2], "--current")
                || !strcmp(a->argv[2], "-current"))
            {
                CHECK_ERROR_BREAK(sessionMachine, COMGETTER(CurrentSnapshot)(pSnapshot.asOutParam()));
                if (pSnapshot.isNull())
                {
                    RTPrintf("This machine does not have any snapshots\n");
                    return RTEXITCODE_FAILURE;
                }
            }
            else
            {
                CHECK_ERROR_BREAK(sessionMachine, FindSnapshot(Bstr(a->argv[2]).raw(),
                                                               pSnapshot.asOutParam()));
            }

            /* parse options */
            for (int i = 3; i < a->argc; i++)
            {
                if (   !strcmp(a->argv[i], "--name")
                    || !strcmp(a->argv[i], "-name")
                    || !strcmp(a->argv[i], "-newname"))
                {
                    if (a->argc <= i + 1)
                    {
                        errorArgument("Missing argument to '%s'", a->argv[i]);
                        rc = E_FAIL;
                        break;
                    }
                    i++;
                    pSnapshot->COMSETTER(Name)(Bstr(a->argv[i]).raw());
                }
                else if (   !strcmp(a->argv[i], "--description")
                         || !strcmp(a->argv[i], "-description")
                         || !strcmp(a->argv[i], "-newdesc"))
                {
                    if (a->argc <= i + 1)
                    {
                        errorArgument("Missing argument to '%s'", a->argv[i]);
                        rc = E_FAIL;
                        break;
                    }
                    i++;
                    pSnapshot->COMSETTER(Description)(Bstr(a->argv[i]).raw());
                }
                else
                {
                    errorSyntax(USAGE_SNAPSHOT, "Invalid parameter '%s'", Utf8Str(a->argv[i]).c_str());
                    rc = E_FAIL;
                    break;
                }
            }

        }
        else if (!strcmp(a->argv[1], "showvminfo"))
        {
            /* exactly one parameter: snapshot name */
            if (a->argc != 3)
            {
                errorSyntax(USAGE_SNAPSHOT, "Expecting snapshot name only");
                rc = E_FAIL;
                break;
            }

            ComPtr<ISnapshot> pSnapshot;

            CHECK_ERROR_BREAK(sessionMachine, FindSnapshot(Bstr(a->argv[2]).raw(),
                                                           pSnapshot.asOutParam()));

            /* get the machine of the given snapshot */
            ComPtr<IMachine> pMachine2;
            pSnapshot->COMGETTER(Machine)(pMachine2.asOutParam());
            showVMInfo(a->virtualBox, pMachine2, NULL, VMINFO_NONE);
        }
        else if (!strcmp(a->argv[1], "list"))
            rc = handleSnapshotList(a, sessionMachine) == RTEXITCODE_SUCCESS ? S_OK : E_FAIL;
        else if (!strcmp(a->argv[1], "dump"))          // undocumented parameter to debug snapshot info
            DumpSnapshot(sessionMachine);
        else
        {
            errorSyntax(USAGE_SNAPSHOT, "Invalid parameter '%s'", Utf8Str(a->argv[1]).c_str());
            rc = E_FAIL;
        }
    } while (0);

    a->session->UnlockMachine();

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

