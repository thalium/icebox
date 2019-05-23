/* $Id: vbsfhlp.c $ */
/** @file
 * VirtualBox Windows Guest Shared Folders - File System Driver system helpers
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

#include <iprt/nt/nt.h> /* includes ntifs.h and wdm.h */
#include <iprt/win/ntdddisk.h>

#include "vbsfhlp.h"

#ifdef DEBUG
static int s_iAllocRefCount = 0;
#endif

void vbsfHlpSleep(ULONG ulMillies)
{
    KEVENT event;
    LARGE_INTEGER dueTime;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    dueTime.QuadPart = -10000 * (int)ulMillies;

    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &dueTime);
}

/**
 * Convert VBox IRT file attributes to NT file attributes
 *
 * @returns NT file attributes
 * @param   fMode       IRT file attributes
 *
 */
uint32_t VBoxToNTFileAttributes(uint32_t fMode)
{
    uint32_t FileAttributes = 0;

    if (fMode & RTFS_DOS_READONLY)
        FileAttributes |= FILE_ATTRIBUTE_READONLY;
    if (fMode & RTFS_DOS_HIDDEN)
        FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
    if (fMode & RTFS_DOS_SYSTEM)
        FileAttributes |= FILE_ATTRIBUTE_SYSTEM;
    if (fMode & RTFS_DOS_DIRECTORY)
        FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    if (fMode & RTFS_DOS_ARCHIVED)
        FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
    if (fMode & RTFS_DOS_NT_TEMPORARY)
        FileAttributes |= FILE_ATTRIBUTE_TEMPORARY;
    if (fMode & RTFS_DOS_NT_SPARSE_FILE)
        FileAttributes |= FILE_ATTRIBUTE_SPARSE_FILE;
    if (fMode & RTFS_DOS_NT_REPARSE_POINT)
        FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
    if (fMode & RTFS_DOS_NT_COMPRESSED)
        FileAttributes |= FILE_ATTRIBUTE_COMPRESSED;
    /*    if (fMode & RTFS_DOS_NT_OFFLINE) */
    if (fMode & RTFS_DOS_NT_NOT_CONTENT_INDEXED)
        FileAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
    if (fMode & RTFS_DOS_NT_ENCRYPTED)
        FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
    if (fMode & RTFS_DOS_NT_NORMAL)
    {
        FileAttributes |= FILE_ATTRIBUTE_NORMAL;
    }
    return FileAttributes;
}

/**
 * Convert VBox IRT file attributes to NT file attributes
 *
 * @returns NT file attributes
 * @param   fMode       IRT file attributes
 *
 */
uint32_t NTToVBoxFileAttributes(uint32_t fMode)
{
    uint32_t FileAttributes = 0;

    if (fMode & FILE_ATTRIBUTE_READONLY)
        FileAttributes |= RTFS_DOS_READONLY;
    if (fMode & FILE_ATTRIBUTE_HIDDEN)
        FileAttributes |= RTFS_DOS_HIDDEN;
    if (fMode & FILE_ATTRIBUTE_SYSTEM)
        FileAttributes |= RTFS_DOS_SYSTEM;
    if (fMode & FILE_ATTRIBUTE_DIRECTORY)
        FileAttributes |= RTFS_DOS_DIRECTORY;
    if (fMode & FILE_ATTRIBUTE_ARCHIVE)
        FileAttributes |= RTFS_DOS_ARCHIVED;
    if (fMode & FILE_ATTRIBUTE_TEMPORARY)
        FileAttributes |= RTFS_DOS_NT_TEMPORARY;
    if (fMode & FILE_ATTRIBUTE_SPARSE_FILE)
        FileAttributes |= RTFS_DOS_NT_SPARSE_FILE;
    if (fMode & FILE_ATTRIBUTE_REPARSE_POINT)
        FileAttributes |= RTFS_DOS_NT_REPARSE_POINT;
    if (fMode & FILE_ATTRIBUTE_COMPRESSED)
        FileAttributes |= RTFS_DOS_NT_COMPRESSED;
    if (fMode & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
        FileAttributes |= RTFS_DOS_NT_NOT_CONTENT_INDEXED;
    if (fMode & FILE_ATTRIBUTE_ENCRYPTED)
        FileAttributes |= RTFS_DOS_NT_ENCRYPTED;
    if (fMode & FILE_ATTRIBUTE_NORMAL)
    {
        FileAttributes |= RTFS_DOS_NT_NORMAL;
    }
    return FileAttributes;
}

NTSTATUS vbsfHlpCreateDriveLetter(WCHAR Letter, UNICODE_STRING *pDeviceName)
{
    UNICODE_STRING driveName;
    RtlInitUnicodeString(&driveName,L"\\??\\_:" );

    /* Replace '_' with actual drive letter */
    driveName.Buffer[driveName.Length/sizeof(WCHAR) - 2] = Letter;

    return IoCreateSymbolicLink(&driveName, pDeviceName);
}

NTSTATUS vbsfHlpDeleteDriveLetter(WCHAR Letter)
{
    UNICODE_STRING driveName;
    RtlInitUnicodeString(&driveName,L"\\??\\_:" );

    /* Replace '_' with actual drive letter */
    driveName.Buffer[driveName.Length/sizeof(WCHAR) - 2] = Letter;

    return IoDeleteSymbolicLink(&driveName);
}

    /**
     * Convert VBox error code to NT status code
     *
     * @returns NT status code
     * @param   vboxRC          VBox error code
     *
     */
NTSTATUS VBoxErrorToNTStatus(int vboxRC)
{
    NTSTATUS Status;

    switch (vboxRC)
    {
    case VINF_SUCCESS:
        Status = STATUS_SUCCESS;
        break;

    case VERR_ACCESS_DENIED:
        Status = STATUS_ACCESS_DENIED;
        break;

    case VERR_NO_MORE_FILES:
        Status = STATUS_NO_MORE_FILES;
        break;

    case VERR_PATH_NOT_FOUND:
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        break;

    case VERR_FILE_NOT_FOUND:
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        break;

    case VERR_DIR_NOT_EMPTY:
        Status = STATUS_DIRECTORY_NOT_EMPTY;
        break;

    case VERR_SHARING_VIOLATION:
        Status = STATUS_SHARING_VIOLATION;
        break;

    case VERR_FILE_LOCK_VIOLATION:
        Status = STATUS_FILE_LOCK_CONFLICT;
        break;

    case VERR_FILE_LOCK_FAILED:
        Status = STATUS_LOCK_NOT_GRANTED;
        break;

    case VINF_BUFFER_OVERFLOW:
        Status = STATUS_BUFFER_OVERFLOW;
        break;

    case VERR_EOF:
    case VINF_EOF:
        Status = STATUS_END_OF_FILE;
        break;

    case VERR_READ_ERROR:
    case VERR_WRITE_ERROR:
    case VERR_FILE_IO_ERROR:
        Status = STATUS_UNEXPECTED_IO_ERROR;
        break;

    case VERR_WRITE_PROTECT:
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        break;

    case VERR_ALREADY_EXISTS:
        Status = STATUS_OBJECT_NAME_COLLISION;
        break;

    case VERR_NOT_A_DIRECTORY:
        Status = STATUS_NOT_A_DIRECTORY;
        break;

    case VERR_SEEK:
        Status = STATUS_INVALID_PARAMETER;
        break;

    case VERR_INVALID_PARAMETER:
        Status = STATUS_INVALID_PARAMETER;
        break;

    case VERR_NOT_SUPPORTED:
        Status = STATUS_NOT_SUPPORTED;
        break;

    default:
        /** @todo error handling */
        Status = STATUS_INVALID_PARAMETER;
        Log(("Unexpected vbox error %Rrc\n",
             vboxRC));
        break;
    }
    return Status;
}

PVOID vbsfAllocNonPagedMem(ULONG ulSize)
{
    PVOID pMemory = NULL;

#ifdef DEBUG
    s_iAllocRefCount = s_iAllocRefCount + 1;
    Log(("vbsfAllocNonPagedMem: RefCnt after incrementing: %d\n", s_iAllocRefCount));
#endif

    /* Tag is reversed (a.k.a "SHFL") to display correctly in debuggers, so search for "SHFL" */
    pMemory = ExAllocatePoolWithTag(NonPagedPool, ulSize, 'LFHS');

    if (NULL != pMemory)
    {
        RtlZeroMemory(pMemory, ulSize);
#ifdef DEBUG
        Log(("vbsfAllocNonPagedMem: Allocated %d bytes of memory at %p.\n", ulSize, pMemory));
#endif
    }
    else
    {
#ifdef DEBUG
        Log(("vbsfAllocNonPagedMem: ERROR: Could not allocate %d bytes of memory!\n", ulSize));
#endif
    }

    return pMemory;
}

void vbsfFreeNonPagedMem(PVOID lpMem)
{
#ifdef DEBUG
    s_iAllocRefCount = s_iAllocRefCount - 1;
    Log(("vbsfFreeNonPagedMem: RefCnt after decrementing: %d\n", s_iAllocRefCount));
#endif

    Assert(lpMem);

    /* MSDN: The ExFreePoolWithTag routine issues a bug check if the specified value for Tag does not match the tag value passed
     to the routine that originally allocated the memory block. Otherwise, the behavior of this routine is identical to ExFreePool. */
    ExFreePoolWithTag(lpMem, 'LFHS');
    lpMem = NULL;
}

#if 0 //def DEBUG
/**
 * Callback for RTLogFormatV which writes to the backdoor.
 * See PFNLOGOUTPUT() for details.
 */
static DECLCALLBACK(size_t) rtLogBackdoorOutput(void *pv, const char *pachChars, size_t cbChars)
{
    RTLogWriteUser(pachChars, cbChars);
    return cbChars;
}

int RTLogBackdoorPrintf1(const char *pszFormat, ...)
{
    va_list args;

    LARGE_INTEGER time;

    KeQueryTickCount(&time);

    RTLogBackdoorPrintf("T=%RX64 ", time.QuadPart);
    va_start(args, pszFormat);
    RTLogFormatV(rtLogBackdoorOutput, NULL, pszFormat, args);
    va_end(args);

    return 0;
}
#endif

#if defined(DEBUG) || defined(LOG_ENABLED)

static PCHAR PnPMinorFunctionString(LONG MinorFunction)
{
    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return "IRP_MJ_PNP - IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MJ_PNP - IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MJ_PNP - IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MJ_PNP - IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MJ_PNP - IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MJ_PNP - IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MJ_PNP - IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MJ_PNP - IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MJ_PNP - IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MJ_PNP - IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MJ_PNP - IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MJ_PNP - IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MJ_PNP - IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MJ_PNP - IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MJ_PNP - IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MJ_PNP - IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MJ_PNP - IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MJ_PNP - IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MJ_PNP - IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MJ_PNP - IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MJ_PNP - IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MJ_PNP - IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MJ_PNP - IRP_MN_SURPRISE_REMOVAL";
        default:
            return "IRP_MJ_PNP - unknown_pnp_irp";
    }
}

PCHAR MajorFunctionString(UCHAR MajorFunction, LONG MinorFunction)
{
    switch (MajorFunction)
    {
        case IRP_MJ_CREATE:
            return "IRP_MJ_CREATE";
        case IRP_MJ_CREATE_NAMED_PIPE:
            return "IRP_MJ_CREATE_NAMED_PIPE";
        case IRP_MJ_CLOSE:
            return "IRP_MJ_CLOSE";
        case IRP_MJ_READ:
            return "IRP_MJ_READ";
        case IRP_MJ_WRITE:
            return "IRP_MJ_WRITE";
        case IRP_MJ_QUERY_INFORMATION:
            return "IRP_MJ_QUERY_INFORMATION";
        case IRP_MJ_SET_INFORMATION:
            return "IRP_MJ_SET_INFORMATION";
        case IRP_MJ_QUERY_EA:
            return "IRP_MJ_QUERY_EA";
        case IRP_MJ_SET_EA:
            return "IRP_MJ_SET_EA";
        case IRP_MJ_FLUSH_BUFFERS:
            return "IRP_MJ_FLUSH_BUFFERS";
        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            return "IRP_MJ_QUERY_VOLUME_INFORMATION";
        case IRP_MJ_SET_VOLUME_INFORMATION:
            return "IRP_MJ_SET_VOLUME_INFORMATION";
        case IRP_MJ_DIRECTORY_CONTROL:
            return "IRP_MJ_DIRECTORY_CONTROL";
        case IRP_MJ_FILE_SYSTEM_CONTROL:
            return "IRP_MJ_FILE_SYSTEM_CONTROL";
        case IRP_MJ_DEVICE_CONTROL:
            return "IRP_MJ_DEVICE_CONTROL";
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            return "IRP_MJ_INTERNAL_DEVICE_CONTROL";
        case IRP_MJ_SHUTDOWN:
            return "IRP_MJ_SHUTDOWN";
        case IRP_MJ_LOCK_CONTROL:
            return "IRP_MJ_LOCK_CONTROL";
        case IRP_MJ_CLEANUP:
            return "IRP_MJ_CLEANUP";
        case IRP_MJ_CREATE_MAILSLOT:
            return "IRP_MJ_CREATE_MAILSLOT";
        case IRP_MJ_QUERY_SECURITY:
            return "IRP_MJ_QUERY_SECURITY";
        case IRP_MJ_SET_SECURITY:
            return "IRP_MJ_SET_SECURITY";
        case IRP_MJ_POWER:
            return "IRP_MJ_POWER";
        case IRP_MJ_SYSTEM_CONTROL:
            return "IRP_MJ_SYSTEM_CONTROL";
        case IRP_MJ_DEVICE_CHANGE:
            return "IRP_MJ_DEVICE_CHANGE";
        case IRP_MJ_QUERY_QUOTA:
            return "IRP_MJ_QUERY_QUOTA";
        case IRP_MJ_SET_QUOTA:
            return "IRP_MJ_SET_QUOTA";
        case IRP_MJ_PNP:
            return PnPMinorFunctionString(MinorFunction);

        default:
            return "unknown_pnp_irp";
    }
}

#endif /* DEBUG || LOG_ENABLED */

/** Allocate and initialize a SHFLSTRING from a UNICODE string.
 *
 *  @param ppShflString Where to store the pointer to the allocated SHFLSTRING structure.
 *                      The structure must be deallocated with vbsfFreeNonPagedMem.
 *  @param pwc          The UNICODE string. If NULL then SHFL is only allocated.
 *  @param cb           Size of the UNICODE string in bytes without the trailing nul.
 *
 *  @return Status code.
 */
NTSTATUS vbsfShflStringFromUnicodeAlloc(PSHFLSTRING *ppShflString, const WCHAR *pwc, uint16_t cb)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PSHFLSTRING pShflString;
    ULONG ulShflStringSize;

    /* Calculate length required for the SHFL structure: header + chars + nul. */
    ulShflStringSize = SHFLSTRING_HEADER_SIZE + cb + sizeof(WCHAR);
    pShflString = (PSHFLSTRING)vbsfAllocNonPagedMem(ulShflStringSize);
    if (pShflString)
    {
        if (ShflStringInitBuffer(pShflString, ulShflStringSize))
        {
            if (pwc)
            {
                RtlCopyMemory(pShflString->String.ucs2, pwc, cb);
                pShflString->String.ucs2[cb / sizeof(WCHAR)] = 0;
                pShflString->u16Length = cb; /* without terminating null */
                AssertMsg(pShflString->u16Length + sizeof(WCHAR) == pShflString->u16Size,
                          ("u16Length %d, u16Size %d\n", pShflString->u16Length, pShflString->u16Size));
            }
            else
            {
                RtlZeroMemory(pShflString->String.ucs2, cb + sizeof(WCHAR));
                pShflString->u16Length = 0; /* without terminating null */
                AssertMsg(pShflString->u16Size >= sizeof(WCHAR),
                          ("u16Size %d\n", pShflString->u16Size));
            }

            *ppShflString = pShflString;
        }
        else
        {
            vbsfFreeNonPagedMem(pShflString);
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}
