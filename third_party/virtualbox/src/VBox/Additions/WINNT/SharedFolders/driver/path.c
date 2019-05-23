/* $Id: path.c $ */
/** @file
 *
 * VirtualBox Windows Guest Shared Folders
 *
 * File System Driver path related routines
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

#include "vbsf.h"

static UNICODE_STRING UnicodeBackslash = { 2, 4, L"\\" };

static NTSTATUS vbsfProcessCreate(PRX_CONTEXT RxContext,
                                  PUNICODE_STRING RemainingName,
                                  FILE_BASIC_INFORMATION *pFileBasicInfo,
                                  FILE_STANDARD_INFORMATION *pFileStandardInfo,
                                  PVOID EaBuffer,
                                  ULONG EaLength,
                                  ULONG *pulCreateAction,
                                  SHFLHANDLE *pHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;

    PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension = VBoxMRxGetDeviceExtension(RxContext);
    PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension = VBoxMRxGetNetRootExtension(capFcb->pNetRoot);

    int vboxRC = VINF_SUCCESS;

    /* Various boolean flags. */
    struct
    {
        ULONG CreateDirectory :1;
        ULONG OpenDirectory :1;
        ULONG DirectoryFile :1;
        ULONG NonDirectoryFile :1;
        ULONG DeleteOnClose :1;
        ULONG TemporaryFile :1;
    } bf;

    ACCESS_MASK DesiredAccess;
    ULONG Options;
    UCHAR FileAttributes;
    ULONG ShareAccess;
    ULONG CreateDisposition;
    SHFLCREATEPARMS *pCreateParms = NULL;

    RT_NOREF(EaBuffer);

    if (EaLength)
    {
        Log(("VBOXSF: vbsfProcessCreate: Unsupported: extended attributes!\n"));
        Status = STATUS_NOT_SUPPORTED;
        goto failure;
    }

    if (BooleanFlagOn(capFcb->FcbState, FCB_STATE_PAGING_FILE))
    {
        Log(("VBOXSF: vbsfProcessCreate: Unsupported: paging file!\n"));
        Status = STATUS_NOT_IMPLEMENTED;
        goto failure;
    }

    Log(("VBOXSF: vbsfProcessCreate: FileAttributes = 0x%08x\n",
         RxContext->Create.NtCreateParameters.FileAttributes));
    Log(("VBOXSF: vbsfProcessCreate: CreateOptions = 0x%08x\n",
         RxContext->Create.NtCreateParameters.CreateOptions));

    RtlZeroMemory (&bf, sizeof (bf));

    DesiredAccess = RxContext->Create.NtCreateParameters.DesiredAccess;
    Options = RxContext->Create.NtCreateParameters.CreateOptions & FILE_VALID_OPTION_FLAGS;
    FileAttributes = (UCHAR)(RxContext->Create.NtCreateParameters.FileAttributes & ~FILE_ATTRIBUTE_NORMAL);
    ShareAccess = RxContext->Create.NtCreateParameters.ShareAccess;

    /* We do not support opens by file ids. */
    if (FlagOn(Options, FILE_OPEN_BY_FILE_ID))
    {
        Log(("VBOXSF: vbsfProcessCreate: Unsupported: file open by id!\n"));
        Status = STATUS_NOT_IMPLEMENTED;
        goto failure;
    }

    /* Mask out unsupported attribute bits. */
    FileAttributes &= (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE);

    bf.DirectoryFile = BooleanFlagOn(Options, FILE_DIRECTORY_FILE);
    bf.NonDirectoryFile = BooleanFlagOn(Options, FILE_NON_DIRECTORY_FILE);
    bf.DeleteOnClose = BooleanFlagOn(Options, FILE_DELETE_ON_CLOSE);
    if (bf.DeleteOnClose)
        Log(("VBOXSF: vbsfProcessCreate: Delete on close!\n"));

    CreateDisposition = RxContext->Create.NtCreateParameters.Disposition;

    bf.CreateDirectory = (BOOLEAN)(bf.DirectoryFile && ((CreateDisposition == FILE_CREATE) || (CreateDisposition == FILE_OPEN_IF)));
    bf.OpenDirectory = (BOOLEAN)(bf.DirectoryFile && ((CreateDisposition == FILE_OPEN) || (CreateDisposition == FILE_OPEN_IF)));
    bf.TemporaryFile = BooleanFlagOn(RxContext->Create.NtCreateParameters.FileAttributes, FILE_ATTRIBUTE_TEMPORARY);

    if (FlagOn(capFcb->FcbState, FCB_STATE_TEMPORARY))
        bf.TemporaryFile = TRUE;

    Log(("VBOXSF: vbsfProcessCreate: bf.TemporaryFile %d, bf.CreateDirectory %d, bf.DirectoryFile = %d\n",
         (ULONG)bf.TemporaryFile, (ULONG)bf.CreateDirectory, (ULONG)bf.DirectoryFile));

    /* Check consistency in specified flags. */
    if (bf.TemporaryFile && bf.CreateDirectory) /* Directories with temporary flag set are not allowed! */
    {
        Log(("VBOXSF: vbsfProcessCreate: Not allowed: Temporary directories!\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto failure;
    }

    if (bf.DirectoryFile && bf.NonDirectoryFile)
    {
        Log(("VBOXSF: vbsfProcessCreate: Unsupported combination: dir && !dir\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto failure;
    }

    /* Initialize create parameters. */
    pCreateParms = (SHFLCREATEPARMS *)vbsfAllocNonPagedMem(sizeof(SHFLCREATEPARMS));
    if (!pCreateParms)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto failure;
    }

    RtlZeroMemory(pCreateParms, sizeof (SHFLCREATEPARMS));

    pCreateParms->Handle = SHFL_HANDLE_NIL;
    pCreateParms->Result = SHFL_NO_RESULT;

    if (bf.DirectoryFile)
    {
        if (CreateDisposition != FILE_CREATE && CreateDisposition != FILE_OPEN && CreateDisposition != FILE_OPEN_IF)
        {
            Log(("VBOXSF: vbsfProcessCreate: Invalid disposition 0x%08X for directory!\n",
                 CreateDisposition));
            Status = STATUS_INVALID_PARAMETER;
            goto failure;
        }

        pCreateParms->CreateFlags |= SHFL_CF_DIRECTORY;
    }

    Log(("VBOXSF: vbsfProcessCreate: CreateDisposition = 0x%08X\n",
         CreateDisposition));

    switch (CreateDisposition)
    {
        case FILE_SUPERSEDE:
            pCreateParms->CreateFlags |= SHFL_CF_ACT_REPLACE_IF_EXISTS | SHFL_CF_ACT_CREATE_IF_NEW;
            Log(("VBOXSF: vbsfProcessCreate: CreateFlags |= SHFL_CF_ACT_REPLACE_IF_EXISTS | SHFL_CF_ACT_CREATE_IF_NEW\n"));
            break;

        case FILE_OPEN:
            pCreateParms->CreateFlags |= SHFL_CF_ACT_OPEN_IF_EXISTS | SHFL_CF_ACT_FAIL_IF_NEW;
            Log(("VBOXSF: vbsfProcessCreate: CreateFlags |= SHFL_CF_ACT_OPEN_IF_EXISTS | SHFL_CF_ACT_FAIL_IF_NEW\n"));
            break;

        case FILE_CREATE:
            pCreateParms->CreateFlags |= SHFL_CF_ACT_FAIL_IF_EXISTS | SHFL_CF_ACT_CREATE_IF_NEW;
            Log(("VBOXSF: vbsfProcessCreate: CreateFlags |= SHFL_CF_ACT_FAIL_IF_EXISTS | SHFL_CF_ACT_CREATE_IF_NEW\n"));
            break;

        case FILE_OPEN_IF:
            pCreateParms->CreateFlags |= SHFL_CF_ACT_OPEN_IF_EXISTS | SHFL_CF_ACT_CREATE_IF_NEW;
            Log(("VBOXSF: vbsfProcessCreate: CreateFlags |= SHFL_CF_ACT_OPEN_IF_EXISTS | SHFL_CF_ACT_CREATE_IF_NEW\n"));
            break;

        case FILE_OVERWRITE:
            pCreateParms->CreateFlags |= SHFL_CF_ACT_OVERWRITE_IF_EXISTS | SHFL_CF_ACT_FAIL_IF_NEW;
            Log(("VBOXSF: vbsfProcessCreate: CreateFlags |= SHFL_CF_ACT_OVERWRITE_IF_EXISTS | SHFL_CF_ACT_FAIL_IF_NEW\n"));
            break;

        case FILE_OVERWRITE_IF:
            pCreateParms->CreateFlags |= SHFL_CF_ACT_OVERWRITE_IF_EXISTS | SHFL_CF_ACT_CREATE_IF_NEW;
            Log(("VBOXSF: vbsfProcessCreate: CreateFlags |= SHFL_CF_ACT_OVERWRITE_IF_EXISTS | SHFL_CF_ACT_CREATE_IF_NEW\n"));
            break;

        default:
            Log(("VBOXSF: vbsfProcessCreate: Unexpected create disposition: 0x%08X\n",
                 CreateDisposition));
            Status = STATUS_INVALID_PARAMETER;
            goto failure;
    }

    Log(("VBOXSF: vbsfProcessCreate: DesiredAccess = 0x%08X\n",
         DesiredAccess));
    Log(("VBOXSF: vbsfProcessCreate: ShareAccess   = 0x%08X\n",
         ShareAccess));

    if (DesiredAccess & FILE_READ_DATA)
    {
        Log(("VBOXSF: vbsfProcessCreate: FILE_READ_DATA\n"));
        pCreateParms->CreateFlags |= SHFL_CF_ACCESS_READ;
    }

    if (DesiredAccess & FILE_WRITE_DATA)
    {
        Log(("VBOXSF: vbsfProcessCreate: FILE_WRITE_DATA\n"));
        /* FILE_WRITE_DATA means write access regardless of FILE_APPEND_DATA bit.
         */
        pCreateParms->CreateFlags |= SHFL_CF_ACCESS_WRITE;
    }
    else if (DesiredAccess & FILE_APPEND_DATA)
    {
        Log(("VBOXSF: vbsfProcessCreate: FILE_APPEND_DATA\n"));
        /* FILE_APPEND_DATA without FILE_WRITE_DATA means append only mode.
         *
         * Both write and append access flags are required for shared folders,
         * as on Windows FILE_APPEND_DATA implies write access.
         */
        pCreateParms->CreateFlags |= SHFL_CF_ACCESS_WRITE | SHFL_CF_ACCESS_APPEND;
    }

    if (DesiredAccess & FILE_READ_ATTRIBUTES)
        pCreateParms->CreateFlags |= SHFL_CF_ACCESS_ATTR_READ;
    if (DesiredAccess & FILE_WRITE_ATTRIBUTES)
        pCreateParms->CreateFlags |= SHFL_CF_ACCESS_ATTR_WRITE;

    if (ShareAccess & (FILE_SHARE_READ | FILE_SHARE_WRITE))
        pCreateParms->CreateFlags |= SHFL_CF_ACCESS_DENYNONE;
    else if (ShareAccess & FILE_SHARE_READ)
        pCreateParms->CreateFlags |= SHFL_CF_ACCESS_DENYWRITE;
    else if (ShareAccess & FILE_SHARE_WRITE)
        pCreateParms->CreateFlags |= SHFL_CF_ACCESS_DENYREAD;
    else pCreateParms->CreateFlags |= SHFL_CF_ACCESS_DENYALL;

    /* Set initial allocation size. */
    pCreateParms->Info.cbObject = RxContext->Create.NtCreateParameters.AllocationSize.QuadPart;

    if (FileAttributes == 0)
        FileAttributes = FILE_ATTRIBUTE_NORMAL;

    pCreateParms->Info.Attr.fMode = NTToVBoxFileAttributes(FileAttributes);

    {
        PSHFLSTRING ParsedPath;
        Log(("VBOXSF: vbsfProcessCreate: RemainingName->Length = %d\n", RemainingName->Length));

        Status = vbsfShflStringFromUnicodeAlloc(&ParsedPath, RemainingName->Buffer, RemainingName->Length);
        if (Status != STATUS_SUCCESS)
        {
            goto failure;
        }

        Log(("VBOXSF: ParsedPath: %.*ls\n",
             ParsedPath->u16Length / sizeof(WCHAR), ParsedPath->String.ucs2));

        /* Call host. */
        Log(("VBOXSF: vbsfProcessCreate: VbglR0SfCreate called.\n"));
        vboxRC = VbglR0SfCreate(&pDeviceExtension->hgcmClient, &pNetRootExtension->map, ParsedPath, pCreateParms);

        vbsfFreeNonPagedMem(ParsedPath);
    }

    Log(("VBOXSF: vbsfProcessCreate: VbglR0SfCreate returns vboxRC = %Rrc, Result = 0x%x\n",
         vboxRC, pCreateParms->Result));

    if (RT_FAILURE(vboxRC))
    {
        /* Map some VBoxRC to STATUS codes expected by the system. */
        switch (vboxRC)
        {
            case VERR_ALREADY_EXISTS:
            {
                *pulCreateAction = FILE_EXISTS;
                Status = STATUS_OBJECT_NAME_COLLISION;
                goto failure;
            }

            /* On POSIX systems, the "mkdir" command returns VERR_FILE_NOT_FOUND when
               doing a recursive directory create. Handle this case. */
            case VERR_FILE_NOT_FOUND:
            {
                pCreateParms->Result = SHFL_PATH_NOT_FOUND;
                break;
            }

            default:
            {
                *pulCreateAction = FILE_DOES_NOT_EXIST;
                Status = VBoxErrorToNTStatus(vboxRC);
                goto failure;
            }
        }
    }

    /*
     * The request succeeded. Analyze host response,
     */
    switch (pCreateParms->Result)
    {
        case SHFL_PATH_NOT_FOUND:
        {
            /* Path to object does not exist. */
            Log(("VBOXSF: vbsfProcessCreate: Path not found\n"));
            *pulCreateAction = FILE_DOES_NOT_EXIST;
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            goto failure;
        }

        case SHFL_FILE_NOT_FOUND:
        {
            Log(("VBOXSF: vbsfProcessCreate: File not found\n"));
            *pulCreateAction = FILE_DOES_NOT_EXIST;
            if (pCreateParms->Handle == SHFL_HANDLE_NIL)
            {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                goto failure;
            }

            Log(("VBOXSF: vbsfProcessCreate: File not found but have a handle!\n"));
            Status = STATUS_UNSUCCESSFUL;
            goto failure;

            break;
        }

        case SHFL_FILE_EXISTS:
        {
            Log(("VBOXSF: vbsfProcessCreate: File exists, Handle = 0x%RX64\n",
                 pCreateParms->Handle));
            if (pCreateParms->Handle == SHFL_HANDLE_NIL)
            {
                *pulCreateAction = FILE_EXISTS;
                if (CreateDisposition == FILE_CREATE)
                {
                    /* File was not opened because we requested a create. */
                    Status = STATUS_OBJECT_NAME_COLLISION;
                    goto failure;
                }

                /* Actually we should not go here, unless we have no rights to open the object. */
                Log(("VBOXSF: vbsfProcessCreate: Existing file was not opened!\n"));
                Status = STATUS_ACCESS_DENIED;
                goto failure;
            }

            *pulCreateAction = FILE_OPENED;

            /* Existing file was opened. Go check flags and create FCB. */
            break;
        }

        case SHFL_FILE_CREATED:
        {
            /* A new file was created. */
            Assert(pCreateParms->Handle != SHFL_HANDLE_NIL);

            *pulCreateAction = FILE_CREATED;

            /* Go check flags and create FCB. */
            break;
        }

        case SHFL_FILE_REPLACED:
        {
            /* Existing file was replaced or overwriting. */
            Assert(pCreateParms->Handle != SHFL_HANDLE_NIL);

            if (CreateDisposition == FILE_SUPERSEDE)
                *pulCreateAction = FILE_SUPERSEDED;
            else
                *pulCreateAction = FILE_OVERWRITTEN;
            /* Go check flags and create FCB. */
            break;
        }

        default:
        {
            Log(("VBOXSF: vbsfProcessCreate: Invalid CreateResult from host (0x%08X)\n",
                 pCreateParms->Result));
            *pulCreateAction = FILE_DOES_NOT_EXIST;
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            goto failure;
        }
    }

    /* Check flags. */
    if (bf.NonDirectoryFile && FlagOn(pCreateParms->Info.Attr.fMode, RTFS_DOS_DIRECTORY))
    {
        /* Caller wanted only a file, but the object is a directory. */
        Log(("VBOXSF: vbsfProcessCreate: File is a directory!\n"));
        Status = STATUS_FILE_IS_A_DIRECTORY;
        goto failure;
    }

    if (bf.DirectoryFile && !FlagOn(pCreateParms->Info.Attr.fMode, RTFS_DOS_DIRECTORY))
    {
        /* Caller wanted only a directory, but the object is not a directory. */
        Log(("VBOXSF: vbsfProcessCreate: File is not a directory!\n"));
        Status = STATUS_NOT_A_DIRECTORY;
        goto failure;
    }

    *pHandle = pCreateParms->Handle;

    /* Translate attributes */
    pFileBasicInfo->FileAttributes = VBoxToNTFileAttributes(pCreateParms->Info.Attr.fMode);

    /* Translate file times */
    pFileBasicInfo->CreationTime.QuadPart = RTTimeSpecGetNtTime(&pCreateParms->Info.BirthTime); /* ridiculous name */
    pFileBasicInfo->LastAccessTime.QuadPart = RTTimeSpecGetNtTime(&pCreateParms->Info.AccessTime);
    pFileBasicInfo->LastWriteTime.QuadPart = RTTimeSpecGetNtTime(&pCreateParms->Info.ModificationTime);
    pFileBasicInfo->ChangeTime.QuadPart = RTTimeSpecGetNtTime(&pCreateParms->Info.ChangeTime);

    if (!FlagOn(pCreateParms->Info.Attr.fMode, RTFS_DOS_DIRECTORY))
    {
        pFileStandardInfo->AllocationSize.QuadPart = pCreateParms->Info.cbAllocated;
        pFileStandardInfo->EndOfFile.QuadPart = pCreateParms->Info.cbObject;
        pFileStandardInfo->Directory = FALSE;

        Log(("VBOXSF: vbsfProcessCreate: AllocationSize = 0x%RX64, EndOfFile = 0x%RX64\n",
             pCreateParms->Info.cbAllocated, pCreateParms->Info.cbObject));
    }
    else
    {
        pFileStandardInfo->AllocationSize.QuadPart = 0;
        pFileStandardInfo->EndOfFile.QuadPart = 0;
        pFileStandardInfo->Directory = TRUE;
    }
    pFileStandardInfo->NumberOfLinks = 0;
    pFileStandardInfo->DeletePending = FALSE;

    vbsfFreeNonPagedMem(pCreateParms);

    return Status;

failure:

    Log(("VBOXSF: vbsfProcessCreate: Returned with status = 0x%08X\n",
          Status));

    if (pCreateParms && pCreateParms->Handle != SHFL_HANDLE_NIL)
    {
        VbglR0SfClose(&pDeviceExtension->hgcmClient, &pNetRootExtension->map, pCreateParms->Handle);
        *pHandle = SHFL_HANDLE_NIL;
    }

    if (pCreateParms)
        vbsfFreeNonPagedMem(pCreateParms);

    return Status;
}

NTSTATUS VBoxMRxCreate(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_NET_ROOT pNetRoot = capFcb->pNetRoot;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    FILE_BASIC_INFORMATION FileBasicInfo;
    FILE_STANDARD_INFORMATION FileStandardInfo;

    ULONG CreateAction = FILE_CREATED;
    SHFLHANDLE Handle = SHFL_HANDLE_NIL;
    PMRX_VBOX_FOBX pVBoxFobx;

    RT_NOREF(__C_Fobx); /* RxCaptureFobx */

    Log(("VBOXSF: MRxCreate: name ptr %p length=%d, SrvOpen->Flags 0x%08X\n",
         RemainingName, RemainingName->Length, SrvOpen->Flags));

    /* Disable FastIO. It causes a verifier bugcheck. */
#ifdef SRVOPEN_FLAG_DONTUSE_READ_CACHING
    SetFlag(SrvOpen->Flags, SRVOPEN_FLAG_DONTUSE_READ_CACHING | SRVOPEN_FLAG_DONTUSE_WRITE_CACHING);
#else
    SetFlag(SrvOpen->Flags, SRVOPEN_FLAG_DONTUSE_READ_CACHEING | SRVOPEN_FLAG_DONTUSE_WRITE_CACHEING);
#endif

    if (RemainingName->Length)
    {
        Log(("VBOXSF: MRxCreate: Attempt to open %.*ls\n",
             RemainingName->Length/sizeof(WCHAR), RemainingName->Buffer));
    }
    else
    {
        if (FlagOn(RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH))
        {
            Log(("VBOXSF: MRxCreate: Empty name -> Only backslash used\n"));
            RemainingName = &UnicodeBackslash;
        }
    }

    if (   pNetRoot->Type != NET_ROOT_WILD
        && pNetRoot->Type != NET_ROOT_DISK)
    {
        Log(("VBOXSF: MRxCreate: netroot type %d not supported\n",
             pNetRoot->Type));
        Status = STATUS_NOT_IMPLEMENTED;
        goto Exit;
    }

    FileBasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;

    Status = vbsfProcessCreate(RxContext,
                               RemainingName,
                               &FileBasicInfo,
                               &FileStandardInfo,
                               RxContext->Create.EaBuffer,
                               RxContext->Create.EaLength,
                               &CreateAction,
                               &Handle);

    if (Status != STATUS_SUCCESS)
    {
        Log(("VBOXSF: MRxCreate: vbsfProcessCreate failed 0x%08X\n",
             Status));
        goto Exit;
    }

    Log(("VBOXSF: MRxCreate: EOF is 0x%RX64 AllocSize is 0x%RX64\n",
         FileStandardInfo.EndOfFile.QuadPart, FileStandardInfo.AllocationSize.QuadPart));

    RxContext->pFobx = RxCreateNetFobx(RxContext, SrvOpen);
    if (!RxContext->pFobx)
    {
        Log(("VBOXSF: MRxCreate: RxCreateNetFobx failed\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    Log(("VBOXSF: MRxCreate: CreateAction = 0x%08X\n",
         CreateAction));

    RxContext->Create.ReturnedCreateInformation = CreateAction;

    if (capFcb->OpenCount == 0)
    {
        FCB_INIT_PACKET InitPacket;
        RxFormInitPacket(InitPacket,
                         &FileBasicInfo.FileAttributes,
                         &FileStandardInfo.NumberOfLinks,
                         &FileBasicInfo.CreationTime,
                         &FileBasicInfo.LastAccessTime,
                         &FileBasicInfo.LastWriteTime,
                         &FileBasicInfo.ChangeTime,
                         &FileStandardInfo.AllocationSize,
                         &FileStandardInfo.EndOfFile,
                         &FileStandardInfo.EndOfFile);
        RxFinishFcbInitialization(capFcb, RDBSS_STORAGE_NTC(FileTypeFile), &InitPacket);
    }

    SrvOpen->BufferingFlags = 0;

    RxContext->pFobx->OffsetOfNextEaToReturn = 1;

    pVBoxFobx = VBoxMRxGetFileObjectExtension(RxContext->pFobx);

    Log(("VBOXSF: MRxCreate: VBoxFobx = %p\n",
         pVBoxFobx));

    if (!pVBoxFobx)
    {
        Log(("VBOXSF: MRxCreate: no VBoxFobx!\n"));
        AssertFailed();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    Log(("VBOXSF: MRxCreate: FileBasicInformation: CreationTime   %RX64\n", FileBasicInfo.CreationTime.QuadPart));
    Log(("VBOXSF: MRxCreate: FileBasicInformation: LastAccessTime %RX64\n", FileBasicInfo.LastAccessTime.QuadPart));
    Log(("VBOXSF: MRxCreate: FileBasicInformation: LastWriteTime  %RX64\n", FileBasicInfo.LastWriteTime.QuadPart));
    Log(("VBOXSF: MRxCreate: FileBasicInformation: ChangeTime     %RX64\n", FileBasicInfo.ChangeTime.QuadPart));
    Log(("VBOXSF: MRxCreate: FileBasicInformation: FileAttributes %RX32\n", FileBasicInfo.FileAttributes));

    pVBoxFobx->hFile = Handle;
    pVBoxFobx->pSrvCall = RxContext->Create.pSrvCall;
    pVBoxFobx->FileStandardInfo = FileStandardInfo;
    pVBoxFobx->FileBasicInfo = FileBasicInfo;
    pVBoxFobx->fKeepCreationTime = FALSE;
    pVBoxFobx->fKeepLastAccessTime = FALSE;
    pVBoxFobx->fKeepLastWriteTime = FALSE;
    pVBoxFobx->fKeepChangeTime = FALSE;
    pVBoxFobx->SetFileInfoOnCloseFlags = 0;

    if (!RxIsFcbAcquiredExclusive(capFcb))
    {
        RxAcquireExclusiveFcbResourceInMRx(capFcb);
    }

    Log(("VBOXSF: MRxCreate: NetRoot is %p, Fcb is %p, SrvOpen is %p, Fobx is %p\n",
         pNetRoot, capFcb, SrvOpen, RxContext->pFobx));
    Log(("VBOXSF: MRxCreate: return 0x%08X\n",
         Status));

Exit:
    return Status;
}

NTSTATUS VBoxMRxComputeNewBufferingState(IN OUT PMRX_SRV_OPEN pMRxSrvOpen, IN PVOID pMRxContext, OUT PULONG pNewBufferingState)
{
    RT_NOREF(pMRxSrvOpen, pMRxContext, pNewBufferingState);
    Log(("VBOXSF: MRxComputeNewBufferingState\n"));
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS VBoxMRxDeallocateForFcb(IN OUT PMRX_FCB pFcb)
{
    RT_NOREF(pFcb);
    Log(("VBOXSF: MRxDeallocateForFcb\n"));
    return STATUS_SUCCESS;
}

NTSTATUS VBoxMRxDeallocateForFobx(IN OUT PMRX_FOBX pFobx)
{
    RT_NOREF(pFobx);
    Log(("VBOXSF: MRxDeallocateForFobx\n"));
    return STATUS_SUCCESS;
}

NTSTATUS VBoxMRxTruncate(IN PRX_CONTEXT RxContext)
{
    RT_NOREF(RxContext);
    Log(("VBOXSF: MRxTruncate\n"));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS VBoxMRxCleanupFobx(IN PRX_CONTEXT RxContext)
{
    PMRX_VBOX_FOBX pVBoxFobx = VBoxMRxGetFileObjectExtension(RxContext->pFobx);

    Log(("VBOXSF: MRxCleanupFobx: pVBoxFobx = %p, Handle = 0x%RX64\n", pVBoxFobx, pVBoxFobx? pVBoxFobx->hFile: 0));

    if (!pVBoxFobx)
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

NTSTATUS VBoxMRxForceClosed(IN PMRX_SRV_OPEN pSrvOpen)
{
    RT_NOREF(pSrvOpen);
    Log(("VBOXSF: MRxForceClosed\n"));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS vbsfSetFileInfo(PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension,
                         PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension,
                         PMRX_VBOX_FOBX pVBoxFobx,
                         PFILE_BASIC_INFORMATION pInfo,
                         BYTE SetAttrFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;

    int vboxRC;
    PSHFLFSOBJINFO pSHFLFileInfo;

    uint8_t *pHGCMBuffer = NULL;
    uint32_t cbBuffer = 0;

    Log(("VBOXSF: vbsfSetFileInfo: SetAttrFlags 0x%02X\n", SetAttrFlags));
    Log(("VBOXSF: vbsfSetFileInfo: FileBasicInformation: CreationTime   %RX64\n", pInfo->CreationTime.QuadPart));
    Log(("VBOXSF: vbsfSetFileInfo: FileBasicInformation: LastAccessTime %RX64\n", pInfo->LastAccessTime.QuadPart));
    Log(("VBOXSF: vbsfSetFileInfo: FileBasicInformation: LastWriteTime  %RX64\n", pInfo->LastWriteTime.QuadPart));
    Log(("VBOXSF: vbsfSetFileInfo: FileBasicInformation: ChangeTime     %RX64\n", pInfo->ChangeTime.QuadPart));
    Log(("VBOXSF: vbsfSetFileInfo: FileBasicInformation: FileAttributes %RX32\n", pInfo->FileAttributes));

    if (SetAttrFlags == 0)
    {
        Log(("VBOXSF: vbsfSetFileInfo: nothing to set\n"));
        return STATUS_SUCCESS;
    }

    cbBuffer = sizeof(SHFLFSOBJINFO);
    pHGCMBuffer = (uint8_t *)vbsfAllocNonPagedMem(cbBuffer);
    if (!pHGCMBuffer)
    {
        AssertFailed();
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pHGCMBuffer, cbBuffer);
    pSHFLFileInfo = (PSHFLFSOBJINFO)pHGCMBuffer;

    /* The properties, that need to be changed, are set to something other than zero */
    if (pInfo->CreationTime.QuadPart && (SetAttrFlags & VBOX_FOBX_F_INFO_CREATION_TIME) != 0)
        RTTimeSpecSetNtTime(&pSHFLFileInfo->BirthTime, pInfo->CreationTime.QuadPart);
    if (pInfo->LastAccessTime.QuadPart && (SetAttrFlags & VBOX_FOBX_F_INFO_LASTACCESS_TIME) != 0)
        RTTimeSpecSetNtTime(&pSHFLFileInfo->AccessTime, pInfo->LastAccessTime.QuadPart);
    if (pInfo->LastWriteTime.QuadPart && (SetAttrFlags & VBOX_FOBX_F_INFO_LASTWRITE_TIME) != 0)
        RTTimeSpecSetNtTime(&pSHFLFileInfo->ModificationTime, pInfo->LastWriteTime.QuadPart);
    if (pInfo->ChangeTime.QuadPart && (SetAttrFlags & VBOX_FOBX_F_INFO_CHANGE_TIME) != 0)
        RTTimeSpecSetNtTime(&pSHFLFileInfo->ChangeTime, pInfo->ChangeTime.QuadPart);
    if (pInfo->FileAttributes && (SetAttrFlags & VBOX_FOBX_F_INFO_ATTRIBUTES) != 0)
        pSHFLFileInfo->Attr.fMode = NTToVBoxFileAttributes(pInfo->FileAttributes);

    vboxRC = VbglR0SfFsInfo(&pDeviceExtension->hgcmClient, &pNetRootExtension->map, pVBoxFobx->hFile,
                            SHFL_INFO_SET | SHFL_INFO_FILE, &cbBuffer, (PSHFLDIRINFO)pSHFLFileInfo);

    if (vboxRC != VINF_SUCCESS)
        Status = VBoxErrorToNTStatus(vboxRC);

    if (pHGCMBuffer)
        vbsfFreeNonPagedMem(pHGCMBuffer);

    Log(("VBOXSF: vbsfSetFileInfo: Returned 0x%08X\n", Status));
    return Status;
}

/*
 * Closes an opened file handle of a MRX_VBOX_FOBX.
 * Updates file attributes if necessary.
 */
NTSTATUS vbsfCloseFileHandle(PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension,
                             PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension,
                             PMRX_VBOX_FOBX pVBoxFobx)
{
    NTSTATUS Status = STATUS_SUCCESS;

    int vboxRC;

    if (pVBoxFobx->hFile == SHFL_HANDLE_NIL)
    {
        Log(("VBOXSF: vbsfCloseFileHandle: SHFL_HANDLE_NIL\n"));
        return STATUS_SUCCESS;
    }

    Log(("VBOXSF: vbsfCloseFileHandle: 0x%RX64, on close info 0x%02X\n",
         pVBoxFobx->hFile, pVBoxFobx->SetFileInfoOnCloseFlags));

    if (pVBoxFobx->SetFileInfoOnCloseFlags)
    {
        /* If the file timestamps were set by the user, then update them before closing the handle,
         * to cancel any effect of the file read/write operations on the host.
         */
        Status = vbsfSetFileInfo(pDeviceExtension,
                                 pNetRootExtension,
                                 pVBoxFobx,
                                 &pVBoxFobx->FileBasicInfo,
                                 pVBoxFobx->SetFileInfoOnCloseFlags);
    }

    vboxRC = VbglR0SfClose(&pDeviceExtension->hgcmClient,
                           &pNetRootExtension->map,
                           pVBoxFobx->hFile);

    pVBoxFobx->hFile = SHFL_HANDLE_NIL;

    if (vboxRC != VINF_SUCCESS)
        Status = VBoxErrorToNTStatus(vboxRC);

    Log(("VBOXSF: vbsfCloseFileHandle: Returned 0x%08X\n", Status));
    return Status;
}

NTSTATUS VBoxMRxCloseSrvOpen(IN PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension = VBoxMRxGetDeviceExtension(RxContext);
    PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension = VBoxMRxGetNetRootExtension(capFcb->pNetRoot);
    PMRX_VBOX_FOBX pVBoxFobx = VBoxMRxGetFileObjectExtension(capFobx);
    PMRX_SRV_OPEN pSrvOpen = capFobx->pSrvOpen;

    PUNICODE_STRING RemainingName = NULL;

    Log(("VBOXSF: MRxCloseSrvOpen: capFcb = %p, capFobx = %p, pVBoxFobx = %p, pSrvOpen = %p\n",
          capFcb, capFobx, pVBoxFobx, pSrvOpen));

    RemainingName = pSrvOpen->pAlreadyPrefixedName;

    Log(("VBOXSF: MRxCloseSrvOpen: Remaining name = %.*ls, Len = %d\n",
         RemainingName->Length / sizeof(WCHAR), RemainingName->Buffer, RemainingName->Length));

    if (!pVBoxFobx)
        return STATUS_INVALID_PARAMETER;

    if (FlagOn(pSrvOpen->Flags, (SRVOPEN_FLAG_FILE_RENAMED | SRVOPEN_FLAG_FILE_DELETED)))
    {
        /* If we renamed or delete the file/dir, then it's already closed */
        Assert(pVBoxFobx->hFile == SHFL_HANDLE_NIL);
        Log(("VBOXSF: MRxCloseSrvOpen: File was renamed, handle 0x%RX64 ignore close.\n",
             pVBoxFobx->hFile));
        return STATUS_SUCCESS;
    }

    /* Close file */
    if (pVBoxFobx->hFile != SHFL_HANDLE_NIL)
        vbsfCloseFileHandle(pDeviceExtension, pNetRootExtension, pVBoxFobx);

    if (capFcb->FcbState & FCB_STATE_DELETE_ON_CLOSE)
    {
        Log(("VBOXSF: MRxCloseSrvOpen: Delete on close. Open count = %d\n",
             capFcb->OpenCount));

        /* Remove file or directory if delete action is pending. */
        if (capFcb->OpenCount == 0)
            Status = vbsfRemove(RxContext);
    }

    return Status;
}

NTSTATUS vbsfRemove(IN PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension = VBoxMRxGetDeviceExtension(RxContext);
    PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension = VBoxMRxGetNetRootExtension(capFcb->pNetRoot);
    PMRX_VBOX_FOBX pVBoxFobx = VBoxMRxGetFileObjectExtension(capFobx);

    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    int vboxRC;
    PSHFLSTRING ParsedPath = NULL;

    Log(("VBOXSF: vbsfRemove: Delete %.*ls. open count = %d\n",
         RemainingName->Length / sizeof(WCHAR), RemainingName->Buffer, capFcb->OpenCount));

    /* Close file first if not already done. */
    if (pVBoxFobx->hFile != SHFL_HANDLE_NIL)
        vbsfCloseFileHandle(pDeviceExtension, pNetRootExtension, pVBoxFobx);

    Log(("VBOXSF: vbsfRemove: RemainingName->Length %d\n", RemainingName->Length));
    Status = vbsfShflStringFromUnicodeAlloc(&ParsedPath, RemainingName->Buffer, RemainingName->Length);
    if (Status != STATUS_SUCCESS)
        return Status;

    /* Call host. */
    vboxRC = VbglR0SfRemove(&pDeviceExtension->hgcmClient, &pNetRootExtension->map,
                            ParsedPath,
                            (pVBoxFobx->FileStandardInfo.Directory) ? SHFL_REMOVE_DIR : SHFL_REMOVE_FILE);

    if (ParsedPath)
        vbsfFreeNonPagedMem(ParsedPath);

    if (vboxRC == VINF_SUCCESS)
        SetFlag(capFobx->pSrvOpen->Flags, SRVOPEN_FLAG_FILE_DELETED);

    Status = VBoxErrorToNTStatus(vboxRC);
    if (vboxRC != VINF_SUCCESS)
        Log(("VBOXSF: vbsfRemove: VbglR0SfRemove failed with %Rrc\n", vboxRC));

    Log(("VBOXSF: vbsfRemove: Returned 0x%08X\n", Status));
    return Status;
}

NTSTATUS vbsfRename(IN PRX_CONTEXT RxContext,
                    IN FILE_INFORMATION_CLASS FileInformationClass,
                    IN PVOID pBuffer,
                    IN ULONG BufferLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension = VBoxMRxGetDeviceExtension(RxContext);
    PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension = VBoxMRxGetNetRootExtension(capFcb->pNetRoot);
    PMRX_VBOX_FOBX pVBoxFobx = VBoxMRxGetFileObjectExtension(capFobx);
    PMRX_SRV_OPEN pSrvOpen = capFobx->pSrvOpen;

    PFILE_RENAME_INFORMATION RenameInformation = (PFILE_RENAME_INFORMATION)RxContext->Info.Buffer;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(pSrvOpen, capFcb);

    int vboxRC;
    PSHFLSTRING SrcPath = 0, DestPath = 0;
    ULONG flags;

    RT_NOREF(FileInformationClass, pBuffer, BufferLength);

    Assert(FileInformationClass == FileRenameInformation);

    Log(("VBOXSF: vbsfRename: FileName = %.*ls\n",
         RenameInformation->FileNameLength / sizeof(WCHAR), &RenameInformation->FileName[0]));

    /* Must close the file before renaming it! */
    if (pVBoxFobx->hFile != SHFL_HANDLE_NIL)
        vbsfCloseFileHandle(pDeviceExtension, pNetRootExtension, pVBoxFobx);

    /* Mark it as renamed, so we do nothing during close */
    SetFlag(pSrvOpen->Flags, SRVOPEN_FLAG_FILE_RENAMED);

    Log(("VBOXSF: vbsfRename: RenameInformation->FileNameLength = %d\n", RenameInformation->FileNameLength));
    Status = vbsfShflStringFromUnicodeAlloc(&DestPath, RenameInformation->FileName, (uint16_t)RenameInformation->FileNameLength);
    if (Status != STATUS_SUCCESS)
        return Status;

    Log(("VBOXSF: vbsfRename: Destination path = %.*ls\n",
         DestPath->u16Length / sizeof(WCHAR), &DestPath->String.ucs2[0]));

    Log(("VBOXSF: vbsfRename: RemainingName->Length = %d\n", RemainingName->Length));
    Status = vbsfShflStringFromUnicodeAlloc(&SrcPath, RemainingName->Buffer, RemainingName->Length);
    if (Status != STATUS_SUCCESS)
    {
        vbsfFreeNonPagedMem(DestPath);
        return Status;
    }

    Log(("VBOXSF: vbsfRename: Source path = %.*ls\n",
         SrcPath->u16Length / sizeof(WCHAR), &SrcPath->String.ucs2[0]));

    /* Call host. */
    flags = pVBoxFobx->FileStandardInfo.Directory? SHFL_RENAME_DIR : SHFL_RENAME_FILE;
    if (RenameInformation->ReplaceIfExists)
        flags |= SHFL_RENAME_REPLACE_IF_EXISTS;

    Log(("VBOXSF: vbsfRename: Calling VbglR0SfRename\n"));
    vboxRC = VbglR0SfRename(&pDeviceExtension->hgcmClient, &pNetRootExtension->map, SrcPath, DestPath, flags);

    vbsfFreeNonPagedMem(SrcPath);
    vbsfFreeNonPagedMem(DestPath);

    Status = VBoxErrorToNTStatus(vboxRC);
    if (vboxRC != VINF_SUCCESS)
        Log(("VBOXSF: vbsfRename: VbglR0SfRename failed with %Rrc\n", vboxRC));

    Log(("VBOXSF: vbsfRename: Returned 0x%08X\n", Status));
    return Status;
}

NTSTATUS VBoxMRxShouldTryToCollapseThisOpen(IN PRX_CONTEXT RxContext)
{
    RT_NOREF(RxContext);
    Log(("VBOXSF: MRxShouldTryToCollapseThisOpen\n"));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS VBoxMRxCollapseOpen(IN OUT PRX_CONTEXT RxContext)
{
    RT_NOREF(RxContext);
    Log(("VBOXSF: MRxCollapseOpen\n"));
    return STATUS_MORE_PROCESSING_REQUIRED;
}
