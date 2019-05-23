/* $Id: nt.h $ */
/** @file
 * IPRT - Header for code using the Native NT API.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___iprt_nt_nt_h___
#define ___iprt_nt_nt_h___

/** @def IPRT_NT_MAP_TO_ZW
 * Map Nt calls to Zw calls.  In ring-0 the Zw calls let you pass kernel memory
 * to the APIs (takes care of the previous context checks).
 */
#ifdef DOXYGEN_RUNNING
# define IPRT_NT_MAP_TO_ZW
#endif

#ifdef IPRT_NT_MAP_TO_ZW
# define NtQueryInformationFile         ZwQueryInformationFile
# define NtQueryInformationProcess      ZwQueryInformationProcess
# define NtQueryInformationThread       ZwQueryInformationThread
# define NtQueryFullAttributesFile      ZwQueryFullAttributesFile
# define NtQuerySystemInformation       ZwQuerySystemInformation
# define NtQuerySecurityObject          ZwQuerySecurityObject
# define NtSetInformationFile           ZwSetInformationFile
# define NtClose                        ZwClose
# define NtCreateFile                   ZwCreateFile
# define NtReadFile                     ZwReadFile
# define NtWriteFile                    ZwWriteFile
# define NtFlushBuffersFile             ZwFlushBuffersFile
/** @todo this is very incomplete! */
#endif

#include <ntstatus.h>

/*
 * Hacks common to both base header sets.
 */
#define RtlFreeUnicodeString       WrongLinkage_RtlFreeUnicodeString
#define NtQueryObject              Incomplete_NtQueryObject
#define ZwQueryObject              Incomplete_ZwQueryObject
#define NtSetInformationObject     Incomplete_NtSetInformationObject
#define _OBJECT_INFORMATION_CLASS  Incomplete_OBJECT_INFORMATION_CLASS
#define OBJECT_INFORMATION_CLASS   Incomplete_OBJECT_INFORMATION_CLASS
#define ObjectBasicInformation     Incomplete_ObjectBasicInformation
#define ObjectTypeInformation      Incomplete_ObjectTypeInformation
#define _PEB                       Incomplete__PEB
#define PEB                        Incomplete_PEB
#define PPEB                       Incomplete_PPEB
#define _TEB                       Incomplete__TEB
#define TEB                        Incomplete_TEB
#define PTEB                       Incomplete_PTEB
#define _PEB_LDR_DATA              Incomplete__PEB_LDR_DATA
#define PEB_LDR_DATA               Incomplete_PEB_LDR_DATA
#define PPEB_LDR_DATA              Incomplete_PPEB_LDR_DATA
#define _KUSER_SHARED_DATA         Incomplete__KUSER_SHARED_DATA
#define KUSER_SHARED_DATA          Incomplete_KUSER_SHARED_DATA
#define PKUSER_SHARED_DATA         Incomplete_PKUSER_SHARED_DATA



#ifdef IPRT_NT_USE_WINTERNL
/*
 * Use Winternl.h.
 */
# define _FILE_INFORMATION_CLASS                IncompleteWinternl_FILE_INFORMATION_CLASS
# define FILE_INFORMATION_CLASS                 IncompleteWinternl_FILE_INFORMATION_CLASS
# define FileDirectoryInformation               IncompleteWinternl_FileDirectoryInformation

# define NtQueryInformationProcess              IncompleteWinternl_NtQueryInformationProcess
# define NtSetInformationProcess                IncompleteWinternl_NtSetInformationProcess
# define PROCESSINFOCLASS                       IncompleteWinternl_PROCESSINFOCLASS
# define _PROCESSINFOCLASS                      IncompleteWinternl_PROCESSINFOCLASS
# define PROCESS_BASIC_INFORMATION              IncompleteWinternl_PROCESS_BASIC_INFORMATION
# define PPROCESS_BASIC_INFORMATION             IncompleteWinternl_PPROCESS_BASIC_INFORMATION
# define _PROCESS_BASIC_INFORMATION             IncompleteWinternl_PROCESS_BASIC_INFORMATION
# define ProcessBasicInformation                IncompleteWinternl_ProcessBasicInformation
# define ProcessDebugPort                       IncompleteWinternl_ProcessDebugPort
# define ProcessWow64Information                IncompleteWinternl_ProcessWow64Information
# define ProcessImageFileName                   IncompleteWinternl_ProcessImageFileName
# define ProcessBreakOnTermination              IncompleteWinternl_ProcessBreakOnTermination

# define RTL_USER_PROCESS_PARAMETERS            IncompleteWinternl_RTL_USER_PROCESS_PARAMETERS
# define PRTL_USER_PROCESS_PARAMETERS           IncompleteWinternl_PRTL_USER_PROCESS_PARAMETERS
# define _RTL_USER_PROCESS_PARAMETERS           IncompleteWinternl__RTL_USER_PROCESS_PARAMETERS

# define NtQueryInformationThread               IncompleteWinternl_NtQueryInformationThread
# define NtSetInformationThread                 IncompleteWinternl_NtSetInformationThread
# define THREADINFOCLASS                        IncompleteWinternl_THREADINFOCLASS
# define _THREADINFOCLASS                       IncompleteWinternl_THREADINFOCLASS
# define ThreadIsIoPending                      IncompleteWinternl_ThreadIsIoPending

# define NtQuerySystemInformation               IncompleteWinternl_NtQuerySystemInformation
# define NtSetSystemInformation                 IncompleteWinternl_NtSetSystemInformation
# define SYSTEM_INFORMATION_CLASS               IncompleteWinternl_SYSTEM_INFORMATION_CLASS
# define _SYSTEM_INFORMATION_CLASS              IncompleteWinternl_SYSTEM_INFORMATION_CLASS
# define SystemBasicInformation                 IncompleteWinternl_SystemBasicInformation
# define SystemPerformanceInformation           IncompleteWinternl_SystemPerformanceInformation
# define SystemTimeOfDayInformation             IncompleteWinternl_SystemTimeOfDayInformation
# define SystemProcessInformation               IncompleteWinternl_SystemProcessInformation
# define SystemProcessorPerformanceInformation  IncompleteWinternl_SystemProcessorPerformanceInformation
# define SystemInterruptInformation             IncompleteWinternl_SystemInterruptInformation
# define SystemExceptionInformation             IncompleteWinternl_SystemExceptionInformation
# define SystemRegistryQuotaInformation         IncompleteWinternl_SystemRegistryQuotaInformation
# define SystemLookasideInformation             IncompleteWinternl_SystemLookasideInformation
# define SystemPolicyInformation                IncompleteWinternl_SystemPolicyInformation


# pragma warning(push)
# pragma warning(disable: 4668)
# define WIN32_NO_STATUS
# include <windef.h>
# include <winnt.h>
# include <winternl.h>
# undef WIN32_NO_STATUS
# include <ntstatus.h>
# pragma warning(pop)


# undef _FILE_INFORMATION_CLASS
# undef FILE_INFORMATION_CLASS
# undef FileDirectoryInformation

# undef NtQueryInformationProcess
# undef NtSetInformationProcess
# undef PROCESSINFOCLASS
# undef _PROCESSINFOCLASS
# undef PROCESS_BASIC_INFORMATION
# undef PPROCESS_BASIC_INFORMATION
# undef _PROCESS_BASIC_INFORMATION
# undef ProcessBasicInformation
# undef ProcessDebugPort
# undef ProcessWow64Information
# undef ProcessImageFileName
# undef ProcessBreakOnTermination

# undef RTL_USER_PROCESS_PARAMETERS
# undef PRTL_USER_PROCESS_PARAMETERS
# undef _RTL_USER_PROCESS_PARAMETERS

# undef NtQueryInformationThread
# undef NtSetInformationThread
# undef THREADINFOCLASS
# undef _THREADINFOCLASS
# undef ThreadIsIoPending

# undef NtQuerySystemInformation
# undef NtSetSystemInformation
# undef SYSTEM_INFORMATION_CLASS
# undef _SYSTEM_INFORMATION_CLASS
# undef SystemBasicInformation
# undef SystemPerformanceInformation
# undef SystemTimeOfDayInformation
# undef SystemProcessInformation
# undef SystemProcessorPerformanceInformation
# undef SystemInterruptInformation
# undef SystemExceptionInformation
# undef SystemRegistryQuotaInformation
# undef SystemLookasideInformation
# undef SystemPolicyInformation

#else
/*
 * Use ntifs.h and wdm.h.
 */
# if _MSC_VER >= 1200 /* Fix/workaround for KeInitializeSpinLock visibility issue on AMD64. */
#  define FORCEINLINE static __forceinline
# else
#  define FORCEINLINE static __inline
# endif

# pragma warning(push)
# ifdef RT_ARCH_X86
#  define _InterlockedAddLargeStatistic  _InterlockedAddLargeStatistic_StupidDDKVsCompilerCrap
#  pragma warning(disable: 4163)
# endif
# pragma warning(disable: 4668)
# pragma warning(disable: 4255) /* warning C4255: 'ObGetFilterVersion' : no function prototype given: converting '()' to '(void)' */
# if _MSC_VER >= 1800 /*RT_MSC_VER_VC120*/
#  pragma warning(disable:4005) /* sdk/v7.1/include/sal_supp.h(57) : warning C4005: '__useHeader' : macro redefinition */
#  pragma warning(disable:4471) /* wdm.h(11057) : warning C4471: '_POOL_TYPE' : a forward declaration of an unscoped enumeration must have an underlying type (int assumed) */
# endif

# include <ntifs.h>
# include <wdm.h>

# ifdef RT_ARCH_X86
#  undef _InterlockedAddLargeStatistic
# endif
# pragma warning(pop)

# define IPRT_NT_NEED_API_GROUP_NTIFS
#endif

#undef RtlFreeUnicodeString
#undef NtQueryObject
#undef ZwQueryObject
#undef NtSetInformationObject
#undef _OBJECT_INFORMATION_CLASS
#undef OBJECT_INFORMATION_CLASS
#undef ObjectBasicInformation
#undef ObjectTypeInformation
#undef _PEB
#undef PEB
#undef PPEB
#undef _TEB
#undef TEB
#undef PTEB
#undef _PEB_LDR_DATA
#undef PEB_LDR_DATA
#undef PPEB_LDR_DATA
#undef _KUSER_SHARED_DATA
#undef KUSER_SHARED_DATA
#undef PKUSER_SHARED_DATA


#include <iprt/types.h>
#include <iprt/assert.h>


/** @name Useful macros
 * @{ */
/** Indicates that we're targeting native NT in the current source. */
#define RTNT_USE_NATIVE_NT              1
/** Initializes a IO_STATUS_BLOCK. */
#define RTNT_IO_STATUS_BLOCK_INITIALIZER  { STATUS_FAILED_DRIVER_ENTRY, ~(uintptr_t)42 }
/** Reinitializes a IO_STATUS_BLOCK. */
#define RTNT_IO_STATUS_BLOCK_REINIT(a_pIos) \
    do { (a_pIos)->Status = STATUS_FAILED_DRIVER_ENTRY; (a_pIos)->Information = ~(uintptr_t)42; } while (0)
/** Similar to INVALID_HANDLE_VALUE in the Windows environment. */
#define RTNT_INVALID_HANDLE_VALUE         ( (HANDLE)~(uintptr_t)0 )
/** Constant UNICODE_STRING initializer. */
#define RTNT_CONSTANT_UNISTR(a_String)   { sizeof(a_String) - sizeof(WCHAR), sizeof(a_String), (WCHAR *)a_String }
/** @}  */


/** @name IPRT helper functions for NT
 * @{ */
RT_C_DECLS_BEGIN

RTDECL(int) RTNtPathOpen(const char *pszPath, ACCESS_MASK fDesiredAccess, ULONG fFileAttribs, ULONG fShareAccess,
                          ULONG fCreateDisposition, ULONG fCreateOptions, ULONG fObjAttribs,
                          PHANDLE phHandle, PULONG_PTR puDisposition);
RTDECL(int) RTNtPathOpenDir(const char *pszPath, ACCESS_MASK fDesiredAccess, ULONG fShareAccess, ULONG fCreateOptions,
                            ULONG fObjAttribs, PHANDLE phHandle, bool *pfObjDir);
RTDECL(int) RTNtPathOpenDirEx(HANDLE hRootDir, struct _UNICODE_STRING *pNtName, ACCESS_MASK fDesiredAccess,
                              ULONG fShareAccess, ULONG fCreateOptions, ULONG fObjAttribs, PHANDLE phHandle, bool *pfObjDir);
RTDECL(int) RTNtPathClose(HANDLE hHandle);

/**
 * Converts a windows-style path to NT format and encoding.
 *
 * @returns IPRT status code.
 * @param   pNtName             Where to return the NT name.  Free using
 *                              RTNtPathFree.
 * @param   phRootDir           Where to return the root handle, if applicable.
 * @param   pszPath             The UTF-8 path.
 */
RTDECL(int) RTNtPathFromWinUtf8(struct _UNICODE_STRING *pNtName, PHANDLE phRootDir, const char *pszPath);

/**
 * Converts a UTF-16 windows-style path to NT format.
 *
 * @returns IPRT status code.
 * @param   pNtName             Where to return the NT name.  Free using
 *                              RTNtPathFree.
 * @param   phRootDir           Where to return the root handle, if applicable.
 * @param   pwszPath            The UTF-16 windows-style path.
 * @param   cwcPath             The max length of the windows-style path in
 *                              RTUTF16 units.  Use RTSTR_MAX if unknown and @a
 *                              pwszPath is correctly terminated.
 */
RTDECL(int) RTNtPathFromWinUtf16Ex(struct _UNICODE_STRING *pNtName, HANDLE *phRootDir, PCRTUTF16 pwszPath, size_t cwcPath);

/**
 * How to handle ascent ('..' relative to a root handle).
 */
typedef enum RTNTPATHRELATIVEASCENT
{
    kRTNtPathRelativeAscent_Invalid = 0,
    kRTNtPathRelativeAscent_Allow,
    kRTNtPathRelativeAscent_Fail,
    kRTNtPathRelativeAscent_Ignore,
    kRTNtPathRelativeAscent_End,
    kRTNtPathRelativeAscent_32BitHack = 0x7fffffff
} RTNTPATHRELATIVEASCENT;

/**
 * Converts a relative windows-style path to relative NT format and encoding.
 *
 * @returns IPRT status code.
 * @param   pNtName             Where to return the NT name.  Free using
 *                              rtTNtPathToNative with phRootDir set to NULL.
 * @param   phRootDir           On input, the handle to the directory the path
 *                              is relative to.  On output, the handle to
 *                              specify as root directory in the object
 *                              attributes when accessing the path.  If
 *                              enmAscent is kRTNtPathRelativeAscent_Allow, it
 *                              may have been set to NULL.
 * @param   pszPath             The relative UTF-8 path.
 * @param   enmAscent           How to handle ascent.
 * @param   fMustReturnAbsolute Must convert to an absolute path.  This
 *                              is necessary if the root dir is a NT directory
 *                              object (e.g. /Devices) since they cannot parse
 *                              relative paths it seems.
 */
RTDECL(int) RTNtPathRelativeFromUtf8(struct _UNICODE_STRING *pNtName, PHANDLE phRootDir, const char *pszPath,
                                     RTNTPATHRELATIVEASCENT enmAscent, bool fMustReturnAbsolute);

/**
 * Ensures that the NT string has sufficient storage to hold @a cwcMin RTUTF16
 * chars plus a terminator.
 *
 * The NT string must have been returned by RTNtPathFromWinUtf8 or
 * RTNtPathFromWinUtf16Ex.
 *
 * @returns IPRT status code.
 * @param   pNtName             The NT path string.
 * @param   cwcMin              The minimum number of RTUTF16 chars. Max 32767.
 * @sa      RTNtPathFree
 */
RTDECL(int) RTNtPathEnsureSpace(struct _UNICODE_STRING *pNtName, size_t cwcMin);

/**
 * Frees the native path and root handle.
 *
 * @param   pNtName             The NT path after a successful rtNtPathToNative
 *                              call or RTNtPathRelativeFromUtf8.
 * @param   phRootDir           The root handle variable from rtNtPathToNative,
 */
RTDECL(void) RTNtPathFree(struct _UNICODE_STRING *pNtName, HANDLE *phRootDir);


/**
 * Checks whether the path could be containing alternative 8.3 names generated
 * by NTFS, FAT, or other similar file systems.
 *
 * @returns Pointer to the first component that might be an 8.3 name, NULL if
 *          not 8.3 path.
 * @param   pwszPath        The path to check.
 *
 * @remarks This is making bad ASSUMPTION wrt to the naming scheme of 8.3 names,
 *          however, non-tilde 8.3 aliases are probably rare enough to not be
 *          worth all the extra code necessary to open each path component and
 *          check if we've got the short name or not.
 */
RTDECL(PRTUTF16) RTNtPathFindPossible8dot3Name(PCRTUTF16 pwszPath);

/**
 * Fixes up a path possibly containing one or more alternative 8-dot-3 style
 * components.
 *
 * The path is fixed up in place.  Errors are ignored.
 *
 * @returns VINF_SUCCESS if it all went smoothly, informational status codes
 *          indicating the nature of last problem we ran into.
 *
 * @param   pUniStr     The path to fix up. MaximumLength is the max buffer
 *                      length.
 * @param   fPathOnly   Whether to only process the path and leave the filename
 *                      as passed in.
 */
RTDECL(int) RTNtPathExpand8dot3Path(struct _UNICODE_STRING *pUniStr, bool fPathOnly);


RT_C_DECLS_END
/** @} */


/** @name NT API delcarations.
 * @{ */
RT_C_DECLS_BEGIN

/** @name Process access rights missing in ntddk headers
 * @{ */
#ifndef  PROCESS_TERMINATE
# define PROCESS_TERMINATE                  UINT32_C(0x00000001)
#endif
#ifndef  PROCESS_CREATE_THREAD
# define PROCESS_CREATE_THREAD              UINT32_C(0x00000002)
#endif
#ifndef  PROCESS_SET_SESSIONID
# define PROCESS_SET_SESSIONID              UINT32_C(0x00000004)
#endif
#ifndef  PROCESS_VM_OPERATION
# define PROCESS_VM_OPERATION               UINT32_C(0x00000008)
#endif
#ifndef  PROCESS_VM_READ
# define PROCESS_VM_READ                    UINT32_C(0x00000010)
#endif
#ifndef  PROCESS_VM_WRITE
# define PROCESS_VM_WRITE                   UINT32_C(0x00000020)
#endif
#ifndef  PROCESS_DUP_HANDLE
# define PROCESS_DUP_HANDLE                 UINT32_C(0x00000040)
#endif
#ifndef  PROCESS_CREATE_PROCESS
# define PROCESS_CREATE_PROCESS             UINT32_C(0x00000080)
#endif
#ifndef  PROCESS_SET_QUOTA
# define PROCESS_SET_QUOTA                  UINT32_C(0x00000100)
#endif
#ifndef  PROCESS_SET_INFORMATION
# define PROCESS_SET_INFORMATION            UINT32_C(0x00000200)
#endif
#ifndef  PROCESS_QUERY_INFORMATION
# define PROCESS_QUERY_INFORMATION          UINT32_C(0x00000400)
#endif
#ifndef  PROCESS_SUSPEND_RESUME
# define PROCESS_SUSPEND_RESUME             UINT32_C(0x00000800)
#endif
#ifndef  PROCESS_QUERY_LIMITED_INFORMATION
# define PROCESS_QUERY_LIMITED_INFORMATION  UINT32_C(0x00001000)
#endif
#ifndef  PROCESS_SET_LIMITED_INFORMATION
# define PROCESS_SET_LIMITED_INFORMATION    UINT32_C(0x00002000)
#endif
#define PROCESS_UNKNOWN_4000                UINT32_C(0x00004000)
#define PROCESS_UNKNOWN_6000                UINT32_C(0x00008000)
#ifndef PROCESS_ALL_ACCESS
# define PROCESS_ALL_ACCESS                 ( STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | UINT32_C(0x0000ffff) )
#endif
/** @} */

/** @name Thread access rights missing in ntddk headers
 * @{ */
#ifndef THREAD_QUERY_INFORMATION
# define THREAD_QUERY_INFORMATION           UINT32_C(0x00000040)
#endif
#ifndef THREAD_SET_THREAD_TOKEN
# define THREAD_SET_THREAD_TOKEN            UINT32_C(0x00000080)
#endif
#ifndef THREAD_IMPERSONATE
# define THREAD_IMPERSONATE                 UINT32_C(0x00000100)
#endif
#ifndef THREAD_DIRECT_IMPERSONATION
# define THREAD_DIRECT_IMPERSONATION        UINT32_C(0x00000200)
#endif
#ifndef THREAD_RESUME
# define THREAD_RESUME                      UINT32_C(0x00001000)
#endif
#define THREAD_UNKNOWN_2000                 UINT32_C(0x00002000)
#define THREAD_UNKNOWN_4000                 UINT32_C(0x00004000)
#define THREAD_UNKNOWN_8000                 UINT32_C(0x00008000)
/** @} */

/** @name Special handle values.
 * @{ */
#ifndef NtCurrentProcess
# define NtCurrentProcess()                 ( (HANDLE)-(intptr_t)1 )
#endif
#ifndef NtCurrentThread
# define NtCurrentThread()                  ( (HANDLE)-(intptr_t)2 )
#endif
#ifndef ZwCurrentProcess
# define ZwCurrentProcess()                 NtCurrentProcess()
#endif
#ifndef ZwCurrentThread
# define ZwCurrentThread()                  NtCurrentThread()
#endif
/** @} */


/** @name Directory object access rights.
 * @{ */
#ifndef DIRECTORY_QUERY
# define DIRECTORY_QUERY                    UINT32_C(0x00000001)
#endif
#ifndef DIRECTORY_TRAVERSE
# define DIRECTORY_TRAVERSE                 UINT32_C(0x00000002)
#endif
#ifndef DIRECTORY_CREATE_OBJECT
# define DIRECTORY_CREATE_OBJECT            UINT32_C(0x00000004)
#endif
#ifndef DIRECTORY_CREATE_SUBDIRECTORY
# define DIRECTORY_CREATE_SUBDIRECTORY      UINT32_C(0x00000008)
#endif
#ifndef DIRECTORY_ALL_ACCESS
# define DIRECTORY_ALL_ACCESS               ( STANDARD_RIGHTS_REQUIRED | UINT32_C(0x0000000f) )
#endif
/** @} */



#ifdef IPRT_NT_USE_WINTERNL
typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;
#endif

/** Extended affinity type, introduced in Windows 7 (?). */
typedef struct _KAFFINITY_EX
{
    /** Count of valid bitmap entries. */
    uint16_t                Count;
    /** Count of allocated bitmap entries. */
    uint16_t                Size;
    /** Reserved / aligmment padding. */
    uint32_t                Reserved;
    /** Bitmap where one bit corresponds to a CPU. */
    uintptr_t               Bitmap[20];
} KAFFINITY_EX;
typedef KAFFINITY_EX *PKAFFINITY_EX;
typedef KAFFINITY_EX const *PCKAFFINITY_EX;

/** @name User Shared Data
 * @{ */

#ifdef IPRT_NT_USE_WINTERNL
typedef struct _KSYSTEM_TIME
{
    ULONG                   LowPart;
    LONG                    High1Time;
    LONG                    High2Time;
} KSYSTEM_TIME;
typedef KSYSTEM_TIME *PKSYSTEM_TIME;

typedef enum _NT_PRODUCT_TYPE
{
    NtProductWinNt = 1,
    NtProductLanManNt,
    NtProductServer
} NT_PRODUCT_TYPE;

#define PROCESSOR_FEATURE_MAX       64

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
    StandardDesign = 0,
    NEC98x86,
    EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

# if 0
typedef struct _XSTATE_FEATURE
{
    ULONG                   Offset;
    ULONG                   Size;
} XSTATE_FEATURE;
typedef XSTATE_FEATURE *PXSTATE_FEATURE;

#define MAXIMUM_XSTATE_FEATURES     64

typedef struct _XSTATE_CONFIGURATION
{
    ULONG64                 EnabledFeatures;
    ULONG                   Size;
    ULONG                   OptimizedSave  : 1;
    XSTATE_FEATURE          Features[MAXIMUM_XSTATE_FEATURES];
} XSTATE_CONFIGURATION;
typedef XSTATE_CONFIGURATION *PXSTATE_CONFIGURATION;
# endif
#endif /* IPRT_NT_USE_WINTERNL */

typedef struct _KUSER_SHARED_DATA
{
    ULONG                   TickCountLowDeprecated;                     /**< 0x000 */
    ULONG                   TickCountMultiplier;                        /**< 0x004 */
    KSYSTEM_TIME volatile   InterruptTime;                              /**< 0x008 */
    KSYSTEM_TIME volatile   SystemTime;                                 /**< 0x014 */
    KSYSTEM_TIME volatile   TimeZoneBias;                               /**< 0x020 */
    USHORT                  ImageNumberLow;                             /**< 0x02c */
    USHORT                  ImageNumberHigh;                            /**< 0x02e */
    WCHAR                   NtSystemRoot[260];                          /**< 0x030 */
    ULONG                   MaxStackTraceDepth;                         /**< 0x238 */
    ULONG                   CryptoExponent;                             /**< 0x23c */
    ULONG                   TimeZoneId;                                 /**< 0x240 */
    ULONG                   LargePageMinimum;                           /**< 0x244 */
    ULONG                   AitSamplingValue;                           /**< 0x248 */
    ULONG                   AppCompatFlag;                              /**< 0x24c */
    ULONGLONG               RNGSeedVersion;                             /**< 0x250 */
    ULONG                   GlobalValidationRunlevel;                   /**< 0x258 */
    LONG volatile           TimeZoneBiasStamp;                          /**< 0x25c*/
    ULONG                   Reserved2;                                  /**< 0x260 */
    NT_PRODUCT_TYPE         NtProductType;                              /**< 0x264 */
    BOOLEAN                 ProductTypeIsValid;                         /**< 0x268 */
    BOOLEAN                 Reserved0[1];                               /**< 0x269 */
    USHORT                  NativeProcessorArchitecture;                /**< 0x26a */
    ULONG                   NtMajorVersion;                             /**< 0x26c */
    ULONG                   NtMinorVersion;                             /**< 0x270 */
    BOOLEAN                 ProcessorFeatures[PROCESSOR_FEATURE_MAX];   /**< 0x274 */
    ULONG                   Reserved1;                                  /**< 0x2b4 */
    ULONG                   Reserved3;                                  /**< 0x2b8 */
    ULONG volatile          TimeSlip;                                   /**< 0x2bc */
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;              /**< 0x2c0 */
    ULONG                   AltArchitecturePad[1];                      /**< 0x2c4 */
    LARGE_INTEGER           SystemExpirationDate;                       /**< 0x2c8 */
    ULONG                   SuiteMask;                                  /**< 0x2d0 */
    BOOLEAN                 KdDebuggerEnabled;                          /**< 0x2d4 */
    union                                                               /**< 0x2d5 */
    {
        UCHAR               MitigationPolicies;                         /**< 0x2d5 */
        struct
        {
            UCHAR           NXSupportPolicy  : 2;
            UCHAR           SEHValidationPolicy  : 2;
            UCHAR           CurDirDevicesSkippedForDlls  : 2;
            UCHAR           Reserved  : 2;
        };
    };
    UCHAR                   Reserved6[2];                               /**< 0x2d6 */
    ULONG volatile          ActiveConsoleId;                            /**< 0x2d8 */
    ULONG volatile          DismountCount;                              /**< 0x2dc */
    ULONG                   ComPlusPackage;                             /**< 0x2e0 */
    ULONG                   LastSystemRITEventTickCount;                /**< 0x2e4 */
    ULONG                   NumberOfPhysicalPages;                      /**< 0x2e8 */
    BOOLEAN                 SafeBootMode;                               /**< 0x2ec */
    UCHAR                   Reserved12[3];                              /**< 0x2ed */
    union                                                               /**< 0x2f0 */
    {
        ULONG               SharedDataFlags;                            /**< 0x2f0 */
        struct
        {
            ULONG           DbgErrorPortPresent  : 1;
            ULONG           DbgElevationEnabled  : 1;
            ULONG           DbgVirtEnabled  : 1;
            ULONG           DbgInstallerDetectEnabled  : 1;
            ULONG           DbgLkgEnabled  : 1;
            ULONG           DbgDynProcessorEnabled  : 1;
            ULONG           DbgConsoleBrokerEnabled  : 1;
            ULONG           DbgSecureBootEnabled  : 1;
            ULONG           SpareBits  : 24;
        };
    };
    ULONG                   DataFlagsPad[1];                            /**< 0x2f4 */
    ULONGLONG               TestRetInstruction;                         /**< 0x2f8 */
    LONGLONG                QpcFrequency;                               /**< 0x300 */
    ULONGLONG               SystemCallPad[3];                           /**< 0x308 */
    union                                                               /**< 0x320 */
    {
        ULONG64 volatile    TickCountQuad;                              /**< 0x320 */
        KSYSTEM_TIME volatile TickCount;                                /**< 0x320 */
        struct                                                          /**< 0x320 */
        {
            ULONG           ReservedTickCountOverlay[3];                /**< 0x320 */
            ULONG           TickCountPad[1];                            /**< 0x32c */
        };
    };
    ULONG                   Cookie;                                     /**< 0x330 */
    ULONG                   CookiePad[1];                               /**< 0x334 */
    LONGLONG                ConsoleSessionForegroundProcessId;          /**< 0x338 */
    ULONGLONG               TimeUpdateLock;                             /**< 0x340 */
    ULONGLONG               BaselineSystemTimeQpc;                      /**< 0x348 */
    ULONGLONG               BaselineInterruptTimeQpc;                   /**< 0x350 */
    ULONGLONG               QpcSystemTimeIncrement;                     /**< 0x358 */
    ULONGLONG               QpcInterruptTimeIncrement;                  /**< 0x360 */
    ULONG                   QpcSystemTimeIncrement32;                   /**< 0x368 */
    ULONG                   QpcInterruptTimeIncrement32;                /**< 0x36c */
    UCHAR                   QpcSystemTimeIncrementShift;                /**< 0x370 */
    UCHAR                   QpcInterruptTimeIncrementShift;             /**< 0x371 */
    UCHAR                   Reserved8[14];                              /**< 0x372 */
    USHORT                  UserModeGlobalLogger[16];                   /**< 0x380 */
    ULONG                   ImageFileExecutionOptions;                  /**< 0x3a0 */
    ULONG                   LangGenerationCount;                        /**< 0x3a4 */
    ULONGLONG               Reserved4;                                  /**< 0x3a8 */
    ULONGLONG volatile      InterruptTimeBias;                          /**< 0x3b0 */
    ULONGLONG volatile      QpcBias;                                    /**< 0x3b8 */
    ULONG volatile          ActiveProcessorCount;                       /**< 0x3c0 */
    UCHAR volatile          ActiveGroupCount;                           /**< 0x3c4 */
    UCHAR                   Reserved9;                                  /**< 0x3c5 */
    union                                                               /**< 0x3c6 */
    {
        USHORT              QpcData;                                    /**< 0x3c6 */
        struct                                                          /**< 0x3c6 */
        {
            BOOLEAN volatile QpcBypassEnabled;                          /**< 0x3c6 */
            UCHAR           QpcShift;                                   /**< 0x3c7 */
        };
    };
    LARGE_INTEGER           TimeZoneBiasEffectiveStart;                 /**< 0x3c8 */
    LARGE_INTEGER           TimeZoneBiasEffectiveEnd;                   /**< 0x3d0 */
    XSTATE_CONFIGURATION    XState;                                     /**< 0x3d8 */
} KUSER_SHARED_DATA;
typedef KUSER_SHARED_DATA *PKUSER_SHARED_DATA;
AssertCompileMemberOffset(KUSER_SHARED_DATA, InterruptTime,             0x008);
AssertCompileMemberOffset(KUSER_SHARED_DATA, SystemTime,                0x014);
AssertCompileMemberOffset(KUSER_SHARED_DATA, NtSystemRoot,              0x030);
AssertCompileMemberOffset(KUSER_SHARED_DATA, LargePageMinimum,          0x244);
AssertCompileMemberOffset(KUSER_SHARED_DATA, Reserved1,                 0x2b4);
AssertCompileMemberOffset(KUSER_SHARED_DATA, TestRetInstruction,        0x2f8);
AssertCompileMemberOffset(KUSER_SHARED_DATA, Cookie,                    0x330);
AssertCompileMemberOffset(KUSER_SHARED_DATA, ImageFileExecutionOptions, 0x3a0);
AssertCompileMemberOffset(KUSER_SHARED_DATA, XState,                    0x3d8);
/** @def MM_SHARED_USER_DATA_VA
 * Read only userland mapping of KUSER_SHARED_DATA. */
#ifndef MM_SHARED_USER_DATA_VA
# if ARCH_BITS == 32
#  define MM_SHARED_USER_DATA_VA        UINT32_C(0x7ffe0000)
# elif ARCH_BITS == 64
#  define MM_SHARED_USER_DATA_VA        UINT64_C(0x7ffe0000)
# else
#  error "Unsupported/undefined ARCH_BITS value."
# endif
#endif
/** @def KI_USER_SHARED_DATA
 * Read write kernel mapping of KUSER_SHARED_DATA. */
#ifndef KI_USER_SHARED_DATA
# ifdef RT_ARCH_X86
#  define KI_USER_SHARED_DATA           UINT32_C(0xffdf0000)
# elif defined(RT_ARCH_AMD64)
#  define KI_USER_SHARED_DATA           UINT64_C(0xfffff78000000000)
# else
#  error "PORT ME - KI_USER_SHARED_DATA"
# endif
#endif
/** @} */


/** @name Process And Thread Environment Blocks
 * @{ */

typedef struct _PEB_LDR_DATA
{
    uint32_t Length;
    BOOLEAN Initialized;
    BOOLEAN Padding[3];
    HANDLE SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    /* End NT4 */
    LIST_ENTRY *EntryInProgress;
    BOOLEAN ShutdownInProgress;
    HANDLE ShutdownThreadId;
} PEB_LDR_DATA;
typedef PEB_LDR_DATA *PPEB_LDR_DATA;

typedef struct _PEB_COMMON
{
    BOOLEAN InheritedAddressSpace;                                          /**< 0x000 / 0x000 */
    BOOLEAN ReadImageFileExecOptions;                                       /**< 0x001 / 0x001 */
    BOOLEAN BeingDebugged;                                                  /**< 0x002 / 0x002 */
    union
    {
        uint8_t BitField;                                                   /**< 0x003 / 0x003 */
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
        } Common;
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W80 */
            uint8_t SkipPatchingUser32Forwarders : 1;                       /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W80 */
            uint8_t IsPackagedProcess : 1;                                  /**< 0x003 / 0x003 : Pos 4, 1 Bit - Differs from W80 */
            uint8_t IsAppContainer : 1;                                     /**< 0x003 / 0x003 : Pos 5, 1 Bit - Differs from W80 */
            uint8_t IsProtectedProcessLight : 1;                            /**< 0x003 / 0x003 : Pos 6, 1 Bit - Differs from W80 */
            uint8_t SpareBits : 1;                                          /**< 0x003 / 0x003 : Pos 7, 1 Bit */
        } W81;
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsLegacyProcess : 1;                                    /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W81 */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W81 */
            uint8_t SkipPatchingUser32Forwarders : 1;                       /**< 0x003 / 0x003 : Pos 4, 1 Bit - Differs from W81 */
            uint8_t IsPackagedProcess : 1;                                  /**< 0x003 / 0x003 : Pos 5, 1 Bit - Differs from W81 */
            uint8_t IsAppContainer : 1;                                     /**< 0x003 / 0x003 : Pos 6, 1 Bit - Differs from W81 */
            uint8_t SpareBits : 1;                                          /**< 0x003 / 0x003 : Pos 7, 1 Bit */
        } W80;
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsLegacyProcess : 1;                                    /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W81, same as W80 & W6. */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W81, same as W80 & W6. */
            uint8_t SkipPatchingUser32Forwarders : 1;                       /**< 0x003 / 0x003 : Pos 4, 1 Bit - Added in W7; Differs from W81, same as W80. */
            uint8_t SpareBits : 3;                                          /**< 0x003 / 0x003 : Pos 5, 3 Bit - Differs from W81 & W80, more spare bits. */
        } W7;
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t IsProtectedProcess : 1;                                 /**< 0x003 / 0x003 : Pos 1, 1 Bit */
            uint8_t IsLegacyProcess : 1;                                    /**< 0x003 / 0x003 : Pos 2, 1 Bit - Differs from W81, same as W80 & W7. */
            uint8_t IsImageDynamicallyRelocated : 1;                        /**< 0x003 / 0x003 : Pos 3, 1 Bit - Differs from W81, same as W80 & W7. */
            uint8_t SpareBits : 4;                                          /**< 0x003 / 0x003 : Pos 4, 4 Bit - Differs from W81, W80, & W7, more spare bits. */
        } W6;
        struct
        {
            uint8_t ImageUsesLargePages : 1;                                /**< 0x003 / 0x003 : Pos 0, 1 Bit */
            uint8_t SpareBits : 7;                                          /**< 0x003 / 0x003 : Pos 1, 7 Bit - Differs from W81, W80, & W7, more spare bits. */
        } W52;
        struct
        {
            BOOLEAN SpareBool;
        } W51;
    } Diff0;
#if ARCH_BITS == 64
    uint32_t Padding0;                                                      /**< 0x004 / NA */
#endif
    HANDLE Mutant;                                                          /**< 0x008 / 0x004 */
    PVOID ImageBaseAddress;                                                 /**< 0x010 / 0x008 */
    PPEB_LDR_DATA Ldr;                                                      /**< 0x018 / 0x00c */
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;                 /**< 0x020 / 0x010 */
    PVOID SubSystemData;                                                    /**< 0x028 / 0x014 */
    HANDLE ProcessHeap;                                                     /**< 0x030 / 0x018 */
    struct _RTL_CRITICAL_SECTION *FastPebLock;                              /**< 0x038 / 0x01c */
    union
    {
        struct
        {
            PVOID AtlThunkSListPtr;                                         /**< 0x040 / 0x020 */
            PVOID IFEOKey;                                                  /**< 0x048 / 0x024 */
            union
            {
                ULONG CrossProcessFlags;                                    /**< 0x050 / 0x028 */
                struct
                {
                    uint32_t ProcessInJob : 1;                              /**< 0x050 / 0x028: Pos 0, 1 Bit */
                    uint32_t ProcessInitializing : 1;                       /**< 0x050 / 0x028: Pos 1, 1 Bit */
                    uint32_t ProcessUsingVEH : 1;                           /**< 0x050 / 0x028: Pos 2, 1 Bit */
                    uint32_t ProcessUsingVCH : 1;                           /**< 0x050 / 0x028: Pos 3, 1 Bit */
                    uint32_t ProcessUsingFTH : 1;                           /**< 0x050 / 0x028: Pos 4, 1 Bit */
                    uint32_t ReservedBits0 : 1;                             /**< 0x050 / 0x028: Pos 5, 27 Bits */
                } W7, W8, W80, W81;
                struct
                {
                    uint32_t ProcessInJob : 1;                              /**< 0x050 / 0x028: Pos 0, 1 Bit */
                    uint32_t ProcessInitializing : 1;                       /**< 0x050 / 0x028: Pos 1, 1 Bit */
                    uint32_t ReservedBits0 : 30;                            /**< 0x050 / 0x028: Pos 2, 30 Bits */
                } W6;
            };
#if ARCH_BITS == 64
            uint32_t Padding1;                                              /**< 0x054 / */
#endif
        } W6, W7, W8, W80, W81;
        struct
        {
            PVOID AtlThunkSListPtr;                                         /**< 0x040 / 0x020 */
            PVOID SparePtr2;                                                /**< 0x048 / 0x024 */
            uint32_t EnvironmentUpdateCount;                                /**< 0x050 / 0x028 */
#if ARCH_BITS == 64
            uint32_t Padding1;                                              /**< 0x054 / */
#endif
        } W52;
        struct
        {
            PVOID FastPebLockRoutine;                                       /**< NA / 0x020 */
            PVOID FastPebUnlockRoutine;                                     /**< NA / 0x024 */
            uint32_t EnvironmentUpdateCount;                                /**< NA / 0x028 */
        } W51;
    } Diff1;
    union
    {
        PVOID KernelCallbackTable;                                          /**< 0x058 / 0x02c */
        PVOID UserSharedInfoPtr;                                            /**< 0x058 / 0x02c - Alternative use in W6.*/
    };
    uint32_t SystemReserved;                                                /**< 0x060 / 0x030 */
    union
    {
        struct
        {
            uint32_t AtlThunkSListPtr32;                                    /**< 0x064 / 0x034 */
        } W7, W8, W80, W81;
        struct
        {
            uint32_t SpareUlong;                                            /**< 0x064 / 0x034 */
        } W52, W6;
        struct
        {
            uint32_t ExecuteOptions : 2;                                    /**< NA / 0x034: Pos 0, 2 Bits */
            uint32_t SpareBits : 30;                                        /**< NA / 0x034: Pos 2, 30 Bits */
        } W51;
    } Diff2;
    union
    {
        struct
        {
            PVOID ApiSetMap;                                                /**< 0x068 / 0x038 */
        } W7, W8, W80, W81;
        struct
        {
            struct _PEB_FREE_BLOCK *FreeList;                               /**< 0x068 / 0x038 */
        } W52, W6;
        struct
        {
            struct _PEB_FREE_BLOCK *FreeList;                               /**< NA / 0x038 */
        } W51;
    } Diff3;
    uint32_t TlsExpansionCounter;                                           /**< 0x070 / 0x03c */
#if ARCH_BITS == 64
    uint32_t Padding2;                                                      /**< 0x074 / NA */
#endif
    struct _RTL_BITMAP *TlsBitmap;                                          /**< 0x078 / 0x040 */
    uint32_t TlsBitmapBits[2];                                              /**< 0x080 / 0x044 */
    PVOID ReadOnlySharedMemoryBase;                                         /**< 0x088 / 0x04c */
    union
    {
        struct
        {
            PVOID SparePvoid0;                                              /**< 0x090 / 0x050 - HotpatchInformation before W81. */
        } W81;
        struct
        {
            PVOID HotpatchInformation;                                      /**< 0x090 / 0x050 - Retired in W81. */
        } W6, W7, W80;
        struct
        {
            PVOID ReadOnlySharedMemoryHeap;
        } W52;
    } Diff4;
    PVOID *ReadOnlyStaticServerData;                                        /**< 0x098 / 0x054 */
    PVOID AnsiCodePageData;                                                 /**< 0x0a0 / 0x058 */
    PVOID OemCodePageData;                                                  /**< 0x0a8 / 0x05c */
    PVOID UnicodeCaseTableData;                                             /**< 0x0b0 / 0x060 */
    uint32_t NumberOfProcessors;                                            /**< 0x0b8 / 0x064 */
    uint32_t NtGlobalFlag;                                                  /**< 0x0bc / 0x068 */
    LARGE_INTEGER CriticalSectionTimeout;                                   /**< 0x0c0 / 0x070 */
    SIZE_T HeapSegmentReserve;                                              /**< 0x0c8 / 0x078 */
    SIZE_T HeapSegmentCommit;                                               /**< 0x0d0 / 0x07c */
    SIZE_T HeapDeCommitTotalFreeThreshold;                                  /**< 0x0d8 / 0x080 */
    SIZE_T HeapDeCommitFreeBlockThreshold;                                  /**< 0x0e0 / 0x084 */
    uint32_t NumberOfHeaps;                                                 /**< 0x0e8 / 0x088 */
    uint32_t MaximumNumberOfHeaps;                                          /**< 0x0ec / 0x08c */
    PVOID *ProcessHeaps;                                                    /**< 0x0f0 / 0x090 */
    PVOID GdiSharedHandleTable;                                             /**< 0x0f8 / 0x094 */
    PVOID ProcessStarterHelper;                                             /**< 0x100 / 0x098 */
    uint32_t GdiDCAttributeList;                                            /**< 0x108 / 0x09c */
#if ARCH_BITS == 64
    uint32_t Padding3;                                                      /**< 0x10c / NA */
#endif
    struct _RTL_CRITICAL_SECTION *LoaderLock;                               /**< 0x110 / 0x0a0 */
    uint32_t OSMajorVersion;                                                /**< 0x118 / 0x0a4 */
    uint32_t OSMinorVersion;                                                /**< 0x11c / 0x0a8 */
    uint16_t OSBuildNumber;                                                 /**< 0x120 / 0x0ac */
    uint16_t OSCSDVersion;                                                  /**< 0x122 / 0x0ae */
    uint32_t OSPlatformId;                                                  /**< 0x124 / 0x0b0 */
    uint32_t ImageSubsystem;                                                /**< 0x128 / 0x0b4 */
    uint32_t ImageSubsystemMajorVersion;                                    /**< 0x12c / 0x0b8 */
    uint32_t ImageSubsystemMinorVersion;                                    /**< 0x130 / 0x0bc */
#if ARCH_BITS == 64
    uint32_t Padding4;                                                      /**< 0x134 / NA */
#endif
    union
    {
        struct
        {
            SIZE_T ActiveProcessAffinityMask;                               /**< 0x138 / 0x0c0 */
        } W7, W8, W80, W81;
        struct
        {
            SIZE_T ImageProcessAffinityMask;                                /**< 0x138 / 0x0c0 */
        } W52, W6;
    } Diff5;
    uint32_t GdiHandleBuffer[ARCH_BITS == 64 ? 60 : 34];                    /**< 0x140 / 0x0c4 */
    PVOID PostProcessInitRoutine;                                           /**< 0x230 / 0x14c */
    PVOID TlsExpansionBitmap;                                               /**< 0x238 / 0x150 */
    uint32_t TlsExpansionBitmapBits[32];                                    /**< 0x240 / 0x154 */
    uint32_t SessionId;                                                     /**< 0x2c0 / 0x1d4 */
#if ARCH_BITS == 64
    uint32_t Padding5;                                                      /**< 0x2c4 / NA */
#endif
    ULARGE_INTEGER AppCompatFlags;                                          /**< 0x2c8 / 0x1d8 */
    ULARGE_INTEGER AppCompatFlagsUser;                                      /**< 0x2d0 / 0x1e0 */
    PVOID pShimData;                                                        /**< 0x2d8 / 0x1e8 */
    PVOID AppCompatInfo;                                                    /**< 0x2e0 / 0x1ec */
    UNICODE_STRING CSDVersion;                                              /**< 0x2e8 / 0x1f0 */
    struct _ACTIVATION_CONTEXT_DATA *ActivationContextData;                 /**< 0x2f8 / 0x1f8 */
    struct _ASSEMBLY_STORAGE_MAP *ProcessAssemblyStorageMap;                /**< 0x300 / 0x1fc */
    struct _ACTIVATION_CONTEXT_DATA *SystemDefaultActivationContextData;    /**< 0x308 / 0x200 */
    struct _ASSEMBLY_STORAGE_MAP *SystemAssemblyStorageMap;                 /**< 0x310 / 0x204 */
    SIZE_T MinimumStackCommit;                                              /**< 0x318 / 0x208 */
    /* End of PEB in W52 (Windows XP (RTM))! */
    struct _FLS_CALLBACK_INFO *FlsCallback;                                 /**< 0x320 / 0x20c */
    LIST_ENTRY FlsListHead;                                                 /**< 0x328 / 0x210 */
    PVOID FlsBitmap;                                                        /**< 0x338 / 0x218 */
    uint32_t FlsBitmapBits[4];                                              /**< 0x340 / 0x21c */
    uint32_t FlsHighIndex;                                                  /**< 0x350 / 0x22c */
    /* End of PEB in W52 (Windows Server 2003)! */
    PVOID WerRegistrationData;                                              /**< 0x358 / 0x230 */
    PVOID WerShipAssertPtr;                                                 /**< 0x360 / 0x234 */
    /* End of PEB in W6 (windows Vista)! */
    union
    {
        struct
        {
            PVOID pUnused;                                                  /**< 0x368 / 0x238 - Was pContextData in W7. */
        } W8, W80, W81;
        struct
        {
            PVOID pContextData;                                             /**< 0x368 / 0x238 - Retired in W80. */
        } W7;
    } Diff6;
    PVOID pImageHeaderHash;                                                 /**< 0x370 / 0x23c */
    union
    {
        uint32_t TracingFlags;                                              /**< 0x378 / 0x240 */
        struct
        {
            uint32_t HeapTracingEnabled : 1;                                /**< 0x378 / 0x240 : Pos 0, 1 Bit */
            uint32_t CritSecTracingEnabled : 1;                             /**< 0x378 / 0x240 : Pos 1, 1 Bit */
            uint32_t LibLoaderTracingEnabled : 1;                           /**< 0x378 / 0x240 : Pos 2, 1 Bit */
            uint32_t SpareTracingBits : 29;                                 /**< 0x378 / 0x240 : Pos 3, 29 Bits */
        } W8, W80, W81;
        struct
        {
            uint32_t HeapTracingEnabled : 1;                                /**< 0x378 / 0x240 : Pos 0, 1 Bit */
            uint32_t CritSecTracingEnabled : 1;                             /**< 0x378 / 0x240 : Pos 1, 1 Bit */
            uint32_t SpareTracingBits : 30;                                 /**< 0x378 / 0x240 : Pos 3, 30 Bits - One bit more than W80 */
        } W7;
    } Diff7;
#if ARCH_BITS == 64
    uint32_t Padding6;                                                      /**< 0x37c / NA */
#endif
    uint64_t CsrServerReadOnlySharedMemoryBase;                             /**< 0x380 / 0x248 */
    /* End of PEB in W8, W81. */
    uintptr_t TppWorkerpListLock;                                           /**< 0x388 / 0x250 */
    LIST_ENTRY TppWorkerpList;                                              /**< 0x390 / 0x254 */
    PVOID WaitOnAddressHashTable[128];                                      /**< 0x3a0 / 0x25c */
#if ARCH_BITS == 32
    uint32_t ExplicitPadding7;                                              /**< NA NA / 0x45c */
#endif
} PEB_COMMON;
typedef PEB_COMMON *PPEB_COMMON;

AssertCompileMemberOffset(PEB_COMMON, ProcessHeap,    ARCH_BITS == 64 ?  0x30 :  0x18);
AssertCompileMemberOffset(PEB_COMMON, SystemReserved, ARCH_BITS == 64 ?  0x60 :  0x30);
AssertCompileMemberOffset(PEB_COMMON, TlsExpansionCounter,   ARCH_BITS == 64 ?  0x70 :  0x3c);
AssertCompileMemberOffset(PEB_COMMON, NtGlobalFlag,   ARCH_BITS == 64 ?  0xbc :  0x68);
AssertCompileMemberOffset(PEB_COMMON, LoaderLock,     ARCH_BITS == 64 ? 0x110 :  0xa0);
AssertCompileMemberOffset(PEB_COMMON, Diff5.W52.ImageProcessAffinityMask, ARCH_BITS == 64 ? 0x138 :  0xc0);
AssertCompileMemberOffset(PEB_COMMON, PostProcessInitRoutine,    ARCH_BITS == 64 ? 0x230 : 0x14c);
AssertCompileMemberOffset(PEB_COMMON, AppCompatFlags, ARCH_BITS == 64 ? 0x2c8 : 0x1d8);
AssertCompileSize(PEB_COMMON, ARCH_BITS == 64 ? 0x7a0 : 0x460);

/** The size of the windows 10 (build 14393) PEB structure. */
#define PEB_SIZE_W10    sizeof(PEB_COMMON)
/** The size of the windows 8.1 PEB structure.  */
#define PEB_SIZE_W81    RT_UOFFSETOF(PEB_COMMON, TppWorkerpListLock)
/** The size of the windows 8.0 PEB structure.  */
#define PEB_SIZE_W80    RT_UOFFSETOF(PEB_COMMON, TppWorkerpListLock)
/** The size of the windows 7 PEB structure.  */
#define PEB_SIZE_W7     RT_UOFFSETOF(PEB_COMMON, CsrServerReadOnlySharedMemoryBase)
/** The size of the windows vista PEB structure.  */
#define PEB_SIZE_W6     RT_UOFFSETOF(PEB_COMMON, Diff3)
/** The size of the windows server 2003 PEB structure.  */
#define PEB_SIZE_W52    RT_UOFFSETOF(PEB_COMMON, WerRegistrationData)
/** The size of the windows XP PEB structure.  */
#define PEB_SIZE_W51    RT_UOFFSETOF(PEB_COMMON, FlsCallback)

#if 0
typedef struct _NT_TIB
{
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
    union
    {
        PVOID FiberData;
        ULONG Version;
    };
    PVOID ArbitraryUserPointer;
    struct _NT_TIB *Self;
} NT_TIB;
typedef NT_TIB *PNT_TIB;
#endif

typedef struct _ACTIVATION_CONTEXT_STACK
{
   uint32_t Flags;
   uint32_t NextCookieSequenceNumber;
   PVOID ActiveFrame;
   LIST_ENTRY FrameListCache;
} ACTIVATION_CONTEXT_STACK;

/* Common TEB. */
typedef struct _TEB_COMMON
{
    NT_TIB NtTib;                                                           /**< 0x000 / 0x000 */
    PVOID EnvironmentPointer;                                               /**< 0x038 / 0x01c */
    CLIENT_ID ClientId;                                                     /**< 0x040 / 0x020 */
    PVOID ActiveRpcHandle;                                                  /**< 0x050 / 0x028 */
    PVOID ThreadLocalStoragePointer;                                        /**< 0x058 / 0x02c */
    PPEB_COMMON ProcessEnvironmentBlock;                                    /**< 0x060 / 0x030 */
    uint32_t LastErrorValue;                                                /**< 0x068 / 0x034 */
    uint32_t CountOfOwnedCriticalSections;                                  /**< 0x06c / 0x038 */
    PVOID CsrClientThread;                                                  /**< 0x070 / 0x03c */
    PVOID Win32ThreadInfo;                                                  /**< 0x078 / 0x040 */
    uint32_t User32Reserved[26];                                            /**< 0x080 / 0x044 */
    uint32_t UserReserved[5];                                               /**< 0x0e8 / 0x0ac */
    PVOID WOW32Reserved;                                                    /**< 0x100 / 0x0c0 */
    uint32_t CurrentLocale;                                                 /**< 0x108 / 0x0c4 */
    uint32_t FpSoftwareStatusRegister;                                      /**< 0x10c / 0x0c8 */
    PVOID SystemReserved1[54];                                              /**< 0x110 / 0x0cc */
    uint32_t ExceptionCode;                                                 /**< 0x2c0 / 0x1a4 */
#if ARCH_BITS == 64
    uint32_t Padding0;                                                      /**< 0x2c4 / NA */
#endif
    union
    {
        struct
        {
            struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;/**< 0x2c8 / 0x1a8 */
            uint8_t SpareBytes[ARCH_BITS == 64 ? 24 : 36];                  /**< 0x2d0 / 0x1ac */
        } W52, W6, W7, W8, W80, W81;
#if ARCH_BITS == 32
        struct
        {
            ACTIVATION_CONTEXT_STACK ActivationContextStack;                /**< NA / 0x1a8 */
            uint8_t SpareBytes[20];                                         /**< NA / 0x1bc */
        } W51;
#endif
    } Diff0;
    union
    {
        struct
        {
            uint32_t TxFsContext;                                           /**< 0x2e8 / 0x1d0 */
        } W6, W7, W8, W80, W81;
        struct
        {
            uint32_t SpareBytesContinues;                                   /**< 0x2e8 / 0x1d0 */
        } W52;
    } Diff1;
#if ARCH_BITS == 64
    uint32_t Padding1;                                                      /**< 0x2ec / NA */
#endif
    /*_GDI_TEB_BATCH*/ uint8_t GdiTebBatch[ARCH_BITS == 64 ? 0x4e8 :0x4e0]; /**< 0x2f0 / 0x1d4 */
    CLIENT_ID RealClientId;                                                 /**< 0x7d8 / 0x6b4 */
    HANDLE GdiCachedProcessHandle;                                          /**< 0x7e8 / 0x6bc */
    uint32_t GdiClientPID;                                                  /**< 0x7f0 / 0x6c0 */
    uint32_t GdiClientTID;                                                  /**< 0x7f4 / 0x6c4 */
    PVOID GdiThreadLocalInfo;                                               /**< 0x7f8 / 0x6c8 */
    SIZE_T Win32ClientInfo[62];                                             /**< 0x800 / 0x6cc */
    PVOID glDispatchTable[233];                                             /**< 0x9f0 / 0x7c4 */
    SIZE_T glReserved1[29];                                                 /**< 0x1138 / 0xb68 */
    PVOID glReserved2;                                                      /**< 0x1220 / 0xbdc */
    PVOID glSectionInfo;                                                    /**< 0x1228 / 0xbe0 */
    PVOID glSection;                                                        /**< 0x1230 / 0xbe4 */
    PVOID glTable;                                                          /**< 0x1238 / 0xbe8 */
    PVOID glCurrentRC;                                                      /**< 0x1240 / 0xbec */
    PVOID glContext;                                                        /**< 0x1248 / 0xbf0 */
    NTSTATUS LastStatusValue;                                               /**< 0x1250 / 0xbf4 */
#if ARCH_BITS == 64
    uint32_t Padding2;                                                      /**< 0x1254 / NA */
#endif
    UNICODE_STRING StaticUnicodeString;                                     /**< 0x1258 / 0xbf8 */
    WCHAR StaticUnicodeBuffer[261];                                         /**< 0x1268 / 0xc00 */
#if ARCH_BITS == 64
    WCHAR Padding3[3];                                                      /**< 0x1472 / NA */
#endif
    PVOID DeallocationStack;                                                /**< 0x1478 / 0xe0c */
    PVOID TlsSlots[64];                                                     /**< 0x1480 / 0xe10 */
    LIST_ENTRY TlsLinks;                                                    /**< 0x1680 / 0xf10 */
    PVOID Vdm;                                                              /**< 0x1690 / 0xf18 */
    PVOID ReservedForNtRpc;                                                 /**< 0x1698 / 0xf1c */
    PVOID DbgSsReserved[2];                                                 /**< 0x16a0 / 0xf20 */
    uint32_t HardErrorMode;                                                 /**< 0x16b0 / 0xf28 - Called HardErrorsAreDisabled in W51. */
#if ARCH_BITS == 64
    uint32_t Padding4;                                                      /**< 0x16b4 / NA */
#endif
    PVOID Instrumentation[ARCH_BITS == 64 ? 11 : 9];                        /**< 0x16b8 / 0xf2c */
    union
    {
        struct
        {
            GUID ActivityId;                                                /**< 0x1710 / 0xf50 */
            PVOID SubProcessTag;                                            /**< 0x1720 / 0xf60 */
        } W6, W7, W8, W80, W81;
        struct
        {
            PVOID InstrumentationContinues[ARCH_BITS == 64 ? 3 : 5];        /**< 0x1710 / 0xf50 */
        } W52;
    } Diff2;
    union                                                                   /**< 0x1728 / 0xf64 */
    {
        struct
        {
            PVOID PerflibData;                                              /**< 0x1728 / 0xf64 */
        } W8, W80, W81;
        struct
        {
            PVOID EtwLocalData;                                             /**< 0x1728 / 0xf64 */
        } W7, W6;
        struct
        {
            PVOID SubProcessTag;                                            /**< 0x1728 / 0xf64 */
        } W52;
        struct
        {
            PVOID InstrumentationContinues[1];                              /**< 0x1728 / 0xf64 */
        } W51;
    } Diff3;
    union
    {
        struct
        {
            PVOID EtwTraceData;                                             /**< 0x1730 / 0xf68 */
        } W52, W6, W7, W8, W80, W81;
        struct
        {
            PVOID InstrumentationContinues[1];                              /**< 0x1730 / 0xf68 */
        } W51;
    } Diff4;
    PVOID WinSockData;                                                      /**< 0x1738 / 0xf6c */
    uint32_t GdiBatchCount;                                                 /**< 0x1740 / 0xf70 */
    union
    {
        union
        {
            PROCESSOR_NUMBER CurrentIdealProcessor;                         /**< 0x1744 / 0xf74 - W7+ */
            uint32_t IdealProcessorValue;                                   /**< 0x1744 / 0xf74 - W7+ */
            struct
            {
                uint8_t ReservedPad1;                                       /**< 0x1744 / 0xf74 - Called SpareBool0 in W6 */
                uint8_t ReservedPad2;                                       /**< 0x1745 / 0xf75 - Called SpareBool0 in W6 */
                uint8_t ReservedPad3;                                       /**< 0x1746 / 0xf76 - Called SpareBool0 in W6 */
                uint8_t IdealProcessor;                                     /**< 0x1747 / 0xf77 */
            };
        } W6, W7, W8, W80, W81;
        struct
        {
            BOOLEAN InDbgPrint;                                             /**< 0x1744 / 0xf74 */
            BOOLEAN FreeStackOnTermination;                                 /**< 0x1745 / 0xf75 */
            BOOLEAN HasFiberData;                                           /**< 0x1746 / 0xf76 */
            uint8_t IdealProcessor;                                         /**< 0x1747 / 0xf77 */
        } W51, W52;
    } Diff5;
    uint32_t GuaranteedStackBytes;                                          /**< 0x1748 / 0xf78 */
#if ARCH_BITS == 64
    uint32_t Padding5;                                                      /**< 0x174c / NA */
#endif
    PVOID ReservedForPerf;                                                  /**< 0x1750 / 0xf7c */
    PVOID ReservedForOle;                                                   /**< 0x1758 / 0xf80 */
    uint32_t WaitingOnLoaderLock;                                           /**< 0x1760 / 0xf84 */
#if ARCH_BITS == 64
    uint32_t Padding6;                                                      /**< 0x1764 / NA */
#endif
    union                                                                   /**< 0x1770 / 0xf8c */
    {
        struct
        {
            PVOID SavedPriorityState;                                       /**< 0x1768 / 0xf88 */
            SIZE_T ReservedForCodeCoverage;                                 /**< 0x1770 / 0xf8c */
            PVOID ThreadPoolData;                                           /**< 0x1778 / 0xf90 */
        } W8, W80, W81;
        struct
        {
            PVOID SavedPriorityState;                                       /**< 0x1768 / 0xf88 */
            SIZE_T SoftPatchPtr1;                                           /**< 0x1770 / 0xf8c */
            PVOID ThreadPoolData;                                           /**< 0x1778 / 0xf90 */
        } W6, W7;
        struct
        {
            PVOID SparePointer1;                                            /**< 0x1768 / 0xf88 */
            SIZE_T SoftPatchPtr1;                                           /**< 0x1770 / 0xf8c */
            PVOID SoftPatchPtr2;                                            /**< 0x1778 / 0xf90 */
        } W52;
#if ARCH_BITS == 32
        struct _Wx86ThreadState
        {
            PVOID CallBx86Eip;                                            /**< NA / 0xf88 */
            PVOID DeallocationCpu;                                        /**< NA / 0xf8c */
            BOOLEAN UseKnownWx86Dll;                                      /**< NA / 0xf90 */
            int8_t OleStubInvoked;                                        /**< NA / 0xf91 */
        } W51;
#endif
    } Diff6;
    PVOID TlsExpansionSlots;                                                /**< 0x1780 / 0xf94 */
#if ARCH_BITS == 64
    PVOID DallocationBStore;                                                /**< 0x1788 / NA */
    PVOID BStoreLimit;                                                      /**< 0x1790 / NA */
#endif
    union
    {
        struct
        {
            uint32_t MuiGeneration;                                                 /**< 0x1798 / 0xf98 */
        } W7, W8, W80, W81;
        struct
        {
            uint32_t ImpersonationLocale;
        } W6;
    } Diff7;
    uint32_t IsImpersonating;                                               /**< 0x179c / 0xf9c */
    PVOID NlsCache;                                                         /**< 0x17a0 / 0xfa0 */
    PVOID pShimData;                                                        /**< 0x17a8 / 0xfa4 */
    union                                                                   /**< 0x17b0 / 0xfa8 */
    {
        struct
        {
            uint16_t HeapVirtualAffinity;                                   /**< 0x17b0 / 0xfa8 */
            uint16_t LowFragHeapDataSlot;                                   /**< 0x17b2 / 0xfaa */
        } W8, W80, W81;
        struct
        {
            uint32_t HeapVirtualAffinity;                                   /**< 0x17b0 / 0xfa8 */
        } W7;
    } Diff8;
#if ARCH_BITS == 64
    uint32_t Padding7;                                                      /**< 0x17b4 / NA */
#endif
    HANDLE CurrentTransactionHandle;                                        /**< 0x17b8 / 0xfac */
    struct _TEB_ACTIVE_FRAME *ActiveFrame;                                  /**< 0x17c0 / 0xfb0 */
    /* End of TEB in W51 (Windows XP)! */
    PVOID FlsData;                                                          /**< 0x17c8 / 0xfb4 */
    union
    {
        struct
        {
            PVOID PreferredLanguages;                                       /**< 0x17d0 / 0xfb8 */
        } W6, W7, W8, W80, W81;
        struct
        {
            BOOLEAN SafeThunkCall;                                          /**< 0x17d0 / 0xfb8 */
            uint8_t BooleanSpare[3];                                        /**< 0x17d1 / 0xfb9 */
            /* End of TEB in W52 (Windows server 2003)! */
        } W52;
    } Diff9;
    PVOID UserPrefLanguages;                                                /**< 0x17d8 / 0xfbc */
    PVOID MergedPrefLanguages;                                              /**< 0x17e0 / 0xfc0 */
    uint32_t MuiImpersonation;                                              /**< 0x17e8 / 0xfc4 */
    union
    {
        uint16_t CrossTebFlags;                                             /**< 0x17ec / 0xfc8 */
        struct
        {
            uint16_t SpareCrossTebBits : 16;                                /**< 0x17ec / 0xfc8 : Pos 0, 16 Bits */
        };
    };
    union
    {
        uint16_t SameTebFlags;                                              /**< 0x17ee / 0xfca */
        struct
        {
            uint16_t SafeThunkCall : 1;                                     /**< 0x17ee / 0xfca : Pos 0, 1 Bit */
            uint16_t InDebugPrint : 1;                                      /**< 0x17ee / 0xfca : Pos 1, 1 Bit */
            uint16_t HasFiberData : 1;                                      /**< 0x17ee / 0xfca : Pos 2, 1 Bit */
            uint16_t SkipThreadAttach : 1;                                  /**< 0x17ee / 0xfca : Pos 3, 1 Bit */
            uint16_t WerInShipAssertCode : 1;                               /**< 0x17ee / 0xfca : Pos 4, 1 Bit */
            uint16_t RanProcessInit : 1;                                    /**< 0x17ee / 0xfca : Pos 5, 1 Bit */
            uint16_t ClonedThread : 1;                                      /**< 0x17ee / 0xfca : Pos 6, 1 Bit */
            uint16_t SuppressDebugMsg : 1;                                  /**< 0x17ee / 0xfca : Pos 7, 1 Bit */
        } Common;
        struct
        {
            uint16_t SafeThunkCall : 1;                                     /**< 0x17ee / 0xfca : Pos 0, 1 Bit */
            uint16_t InDebugPrint : 1;                                      /**< 0x17ee / 0xfca : Pos 1, 1 Bit */
            uint16_t HasFiberData : 1;                                      /**< 0x17ee / 0xfca : Pos 2, 1 Bit */
            uint16_t SkipThreadAttach : 1;                                  /**< 0x17ee / 0xfca : Pos 3, 1 Bit */
            uint16_t WerInShipAssertCode : 1;                               /**< 0x17ee / 0xfca : Pos 4, 1 Bit */
            uint16_t RanProcessInit : 1;                                    /**< 0x17ee / 0xfca : Pos 5, 1 Bit */
            uint16_t ClonedThread : 1;                                      /**< 0x17ee / 0xfca : Pos 6, 1 Bit */
            uint16_t SuppressDebugMsg : 1;                                  /**< 0x17ee / 0xfca : Pos 7, 1 Bit */
            uint16_t DisableUserStackWalk : 1;                              /**< 0x17ee / 0xfca : Pos 8, 1 Bit */
            uint16_t RtlExceptionAttached : 1;                              /**< 0x17ee / 0xfca : Pos 9, 1 Bit */
            uint16_t InitialThread : 1;                                     /**< 0x17ee / 0xfca : Pos 10, 1 Bit */
            uint16_t SessionAware : 1;                                      /**< 0x17ee / 0xfca : Pos 11, 1 Bit - New Since W7. */
            uint16_t SpareSameTebBits : 4;                                  /**< 0x17ee / 0xfca : Pos 12, 4 Bits */
        } W8, W80, W81;
        struct
        {
            uint16_t SafeThunkCall : 1;                                     /**< 0x17ee / 0xfca : Pos 0, 1 Bit */
            uint16_t InDebugPrint : 1;                                      /**< 0x17ee / 0xfca : Pos 1, 1 Bit */
            uint16_t HasFiberData : 1;                                      /**< 0x17ee / 0xfca : Pos 2, 1 Bit */
            uint16_t SkipThreadAttach : 1;                                  /**< 0x17ee / 0xfca : Pos 3, 1 Bit */
            uint16_t WerInShipAssertCode : 1;                               /**< 0x17ee / 0xfca : Pos 4, 1 Bit */
            uint16_t RanProcessInit : 1;                                    /**< 0x17ee / 0xfca : Pos 5, 1 Bit */
            uint16_t ClonedThread : 1;                                      /**< 0x17ee / 0xfca : Pos 6, 1 Bit */
            uint16_t SuppressDebugMsg : 1;                                  /**< 0x17ee / 0xfca : Pos 7, 1 Bit */
            uint16_t DisableUserStackWalk : 1;                              /**< 0x17ee / 0xfca : Pos 8, 1 Bit */
            uint16_t RtlExceptionAttached : 1;                              /**< 0x17ee / 0xfca : Pos 9, 1 Bit */
            uint16_t InitialThread : 1;                                     /**< 0x17ee / 0xfca : Pos 10, 1 Bit */
            uint16_t SpareSameTebBits : 5;                                  /**< 0x17ee / 0xfca : Pos 12, 4 Bits */
        } W7;
        struct
        {
            uint16_t DbgSafeThunkCall : 1;                                  /**< 0x17ee / 0xfca : Pos 0, 1 Bit */
            uint16_t DbgInDebugPrint : 1;                                   /**< 0x17ee / 0xfca : Pos 1, 1 Bit */
            uint16_t DbgHasFiberData : 1;                                   /**< 0x17ee / 0xfca : Pos 2, 1 Bit */
            uint16_t DbgSkipThreadAttach : 1;                               /**< 0x17ee / 0xfca : Pos 3, 1 Bit */
            uint16_t DbgWerInShipAssertCode : 1;                            /**< 0x17ee / 0xfca : Pos 4, 1 Bit */
            uint16_t DbgRanProcessInit : 1;                                 /**< 0x17ee / 0xfca : Pos 5, 1 Bit */
            uint16_t DbgClonedThread : 1;                                   /**< 0x17ee / 0xfca : Pos 6, 1 Bit */
            uint16_t DbgSuppressDebugMsg : 1;                               /**< 0x17ee / 0xfca : Pos 7, 1 Bit */
            uint16_t SpareSameTebBits : 8;                                  /**< 0x17ee / 0xfca : Pos 8, 8 Bits */
        } W6;
    } Diff10;
    PVOID TxnScopeEnterCallback;                                            /**< 0x17f0 / 0xfcc */
    PVOID TxnScopeExitCallback;                                             /**< 0x17f8 / 0xfd0 */
    PVOID TxnScopeContext;                                                  /**< 0x1800 / 0xfd4 */
    uint32_t LockCount;                                                     /**< 0x1808 / 0xfd8 */
    union
    {
        struct
        {
            uint32_t SpareUlong0;                                           /**< 0x180c / 0xfdc */
        } W7, W8, W80, W81;
        struct
        {
            uint32_t ProcessRundown;
        } W6;
    } Diff11;
    union
    {
        struct
        {
            PVOID ResourceRetValue;                                        /**< 0x1810 / 0xfe0 */
            /* End of TEB in W7 (windows 7)! */
            PVOID ReservedForWdf;                                          /**< 0x1818 / 0xfe4 - New Since W7. */
            /* End of TEB in W8 (windows 8.0 & 8.1)! */
            PVOID ReservedForCrt;                                          /**< 0x1820 / 0xfe8 - New Since W10.  */
            RTUUID EffectiveContainerId;                                   /**< 0x1828 / 0xfec - New Since W10.  */
            /* End of TEB in W10 14393! */
        } W8, W80, W81, W10;
        struct
        {
            PVOID ResourceRetValue;                                        /**< 0x1810 / 0xfe0 */
        } W7;
        struct
        {
            uint64_t LastSwitchTime;                                       /**< 0x1810 / 0xfe0 */
            uint64_t TotalSwitchOutTime;                                   /**< 0x1818 / 0xfe8 */
            LARGE_INTEGER WaitReasonBitMap;                                /**< 0x1820 / 0xff0 */
            /* End of TEB in W6 (windows Vista)! */
        } W6;
    } Diff12;
} TEB_COMMON;
typedef TEB_COMMON *PTEB_COMMON;
AssertCompileMemberOffset(TEB_COMMON, ExceptionCode,        ARCH_BITS == 64 ?  0x2c0 : 0x1a4);
AssertCompileMemberOffset(TEB_COMMON, LastStatusValue,      ARCH_BITS == 64 ? 0x1250 : 0xbf4);
AssertCompileMemberOffset(TEB_COMMON, DeallocationStack,    ARCH_BITS == 64 ? 0x1478 : 0xe0c);
AssertCompileMemberOffset(TEB_COMMON, ReservedForNtRpc,     ARCH_BITS == 64 ? 0x1698 : 0xf1c);
AssertCompileMemberOffset(TEB_COMMON, Instrumentation,      ARCH_BITS == 64 ? 0x16b8 : 0xf2c);
AssertCompileMemberOffset(TEB_COMMON, Diff2,                ARCH_BITS == 64 ? 0x1710 : 0xf50);
AssertCompileMemberOffset(TEB_COMMON, Diff3,                ARCH_BITS == 64 ? 0x1728 : 0xf64);
AssertCompileMemberOffset(TEB_COMMON, Diff4,                ARCH_BITS == 64 ? 0x1730 : 0xf68);
AssertCompileMemberOffset(TEB_COMMON, WinSockData,          ARCH_BITS == 64 ? 0x1738 : 0xf6c);
AssertCompileMemberOffset(TEB_COMMON, GuaranteedStackBytes, ARCH_BITS == 64 ? 0x1748 : 0xf78);
AssertCompileMemberOffset(TEB_COMMON, MuiImpersonation,     ARCH_BITS == 64 ? 0x17e8 : 0xfc4);
AssertCompileMemberOffset(TEB_COMMON, LockCount,            ARCH_BITS == 64 ? 0x1808 : 0xfd8);
AssertCompileSize(TEB_COMMON, ARCH_BITS == 64 ? 0x1838 : 0x1000);


/** The size of the windows 8.1 PEB structure.  */
#define TEB_SIZE_W10    ( RT_UOFFSETOF(TEB_COMMON, Diff12.W10.EffectiveContainerId) + sizeof(RTUUID) )
/** The size of the windows 8.1 PEB structure.  */
#define TEB_SIZE_W81    ( RT_UOFFSETOF(TEB_COMMON, Diff12.W8.ReservedForWdf) + sizeof(PVOID) )
/** The size of the windows 8.0 PEB structure.  */
#define TEB_SIZE_W80    ( RT_UOFFSETOF(TEB_COMMON, Diff12.W8.ReservedForWdf) + sizeof(PVOID) )
/** The size of the windows 7 PEB structure.  */
#define TEB_SIZE_W7     RT_UOFFSETOF(TEB_COMMON, Diff12.W8.ReservedForWdf)
/** The size of the windows vista PEB structure.  */
#define TEB_SIZE_W6     ( RT_UOFFSETOF(TEB_COMMON, Diff12.W6.WaitReasonBitMap) + sizeof(LARGE_INTEGER) )
/** The size of the windows server 2003 PEB structure.  */
#define TEB_SIZE_W52    RT_ALIGN_Z(RT_UOFFSETOF(TEB_COMMON, Diff9.W52.BooleanSpare), sizeof(PVOID))
/** The size of the windows XP PEB structure.  */
#define TEB_SIZE_W51    RT_UOFFSETOF(TEB_COMMON, FlsData)



#define _PEB        _PEB_COMMON
typedef PEB_COMMON  PEB;
typedef PPEB_COMMON PPEB;

#define _TEB        _TEB_COMMON
typedef TEB_COMMON  TEB;
typedef PTEB_COMMON PTEB;

#if !defined(NtCurrentTeb) && !defined(IPRT_NT_HAVE_CURRENT_TEB_MACRO)
# ifdef RT_ARCH_X86
DECL_FORCE_INLINE(PTEB)     RTNtCurrentTeb(void) { return (PTEB)__readfsdword(RT_OFFSETOF(TEB_COMMON, NtTib.Self)); }
DECL_FORCE_INLINE(PPEB)     RTNtCurrentPeb(void) { return (PPEB)__readfsdword(RT_OFFSETOF(TEB_COMMON, ProcessEnvironmentBlock)); }
DECL_FORCE_INLINE(uint32_t) RTNtCurrentThreadId(void) { return __readfsdword(RT_OFFSETOF(TEB_COMMON, ClientId.UniqueThread)); }
# elif defined(RT_ARCH_AMD64)
DECL_FORCE_INLINE(PTEB)     RTNtCurrentTeb(void) { return (PTEB)__readgsqword(RT_OFFSETOF(TEB_COMMON, NtTib.Self)); }
DECL_FORCE_INLINE(PPEB)     RTNtCurrentPeb(void) { return (PPEB)__readgsqword(RT_OFFSETOF(TEB_COMMON, ProcessEnvironmentBlock)); }
DECL_FORCE_INLINE(uint32_t) RTNtCurrentThreadId(void) { return (uint32_t)__readgsqword(RT_OFFSETOF(TEB_COMMON, ClientId.UniqueThread)); }
# else
#  error "Port me"
# endif
#else
# define RTNtCurrentTeb()        ((PTEB)NtCurrentTeb())
# define RTNtCurrentPeb()        (RTNtCurrentTeb()->ProcessEnvironmentBlock)
# define RTNtCurrentThreadId()   ((uint32_t)(uintptr_t)RTNtCurrentTeb()->ClientId.UniqueThread)
#endif
#define NtCurrentPeb()           RTNtCurrentPeb()


/** @} */


#ifdef IPRT_NT_USE_WINTERNL
NTSYSAPI NTSTATUS NTAPI NtCreateSection(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PLARGE_INTEGER, ULONG, ULONG, HANDLE);
typedef enum _SECTION_INHERIT
{
    ViewShare = 1,
    ViewUnmap
} SECTION_INHERIT;
#endif
NTSYSAPI NTSTATUS NTAPI NtMapViewOfSection(HANDLE, HANDLE, PVOID *, ULONG, SIZE_T, PLARGE_INTEGER, PSIZE_T, SECTION_INHERIT,
                                           ULONG, ULONG);
NTSYSAPI NTSTATUS NTAPI NtFlushVirtualMemory(HANDLE, PVOID *, PSIZE_T, PIO_STATUS_BLOCK);
NTSYSAPI NTSTATUS NTAPI NtUnmapViewOfSection(HANDLE, PVOID);

#ifdef IPRT_NT_USE_WINTERNL
typedef struct _FILE_FS_ATTRIBUTE_INFORMATION
{
    ULONG   FileSystemAttributes;
    LONG    MaximumComponentNameLength;
    ULONG   FileSystemNameLength;
    WCHAR   FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION;
typedef FILE_FS_ATTRIBUTE_INFORMATION *PFILE_FS_ATTRIBUTE_INFORMATION;

NTSYSAPI NTSTATUS NTAPI NtOpenProcess(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PCLIENT_ID);
NTSYSAPI NTSTATUS NTAPI NtOpenProcessToken(HANDLE, ACCESS_MASK, PHANDLE);
NTSYSAPI NTSTATUS NTAPI NtOpenThread(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PCLIENT_ID);
NTSYSAPI NTSTATUS NTAPI NtOpenThreadToken(HANDLE, ACCESS_MASK, BOOLEAN, PHANDLE);

typedef enum _FSINFOCLASS
{
    FileFsVolumeInformation = 1,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsDriverPathInformation,
    FileFsVolumeFlagsInformation,
    FileFsSectorSizeInformation,
    FileFsDataCopyInformation,
    FileFsMaximumInformation
} FS_INFORMATION_CLASS;
typedef FS_INFORMATION_CLASS *PFS_INFORMATION_CLASS;
NTSYSAPI NTSTATUS NTAPI NtQueryVolumeInformationFile(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FS_INFORMATION_CLASS);

typedef struct _FILE_BOTH_DIR_INFORMATION
{
    ULONG           NextEntryOffset;
    ULONG           FileIndex;
    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   LastAccessTime;
    LARGE_INTEGER   LastWriteTime;
    LARGE_INTEGER   ChangeTime;
    LARGE_INTEGER   EndOfFile;
    LARGE_INTEGER   AllocationSize;
    ULONG           FileAttributes;
    ULONG           FileNameLength;
    ULONG           EaSize;
    CCHAR           ShortNameLength;
    WCHAR           ShortName[12];
    WCHAR           FileName[1];
} FILE_BOTH_DIR_INFORMATION;
typedef FILE_BOTH_DIR_INFORMATION *PFILE_BOTH_DIR_INFORMATION;
typedef struct _FILE_BASIC_INFORMATION
{
    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   LastAccessTime;
    LARGE_INTEGER   LastWriteTime;
    LARGE_INTEGER   ChangeTime;
    ULONG           FileAttributes;
} FILE_BASIC_INFORMATION;
typedef FILE_BASIC_INFORMATION *PFILE_BASIC_INFORMATION;
typedef struct _FILE_STANDARD_INFORMATION
{
    LARGE_INTEGER   AllocationSize;
    LARGE_INTEGER   EndOfFile;
    ULONG           NumberOfLinks;
    BOOLEAN         DeletePending;
    BOOLEAN         Directory;
} FILE_STANDARD_INFORMATION;
typedef FILE_STANDARD_INFORMATION *PFILE_STANDARD_INFORMATION;
typedef struct _FILE_NAME_INFORMATION
{
    ULONG           FileNameLength;
    WCHAR           FileName[1];
} FILE_NAME_INFORMATION;
typedef FILE_NAME_INFORMATION *PFILE_NAME_INFORMATION;
typedef struct _FILE_NETWORK_OPEN_INFORMATION
{
    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   LastAccessTime;
    LARGE_INTEGER   LastWriteTime;
    LARGE_INTEGER   ChangeTime;
    LARGE_INTEGER   AllocationSize;
    LARGE_INTEGER   EndOfFile;
    ULONG           FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION;
typedef FILE_NETWORK_OPEN_INFORMATION *PFILE_NETWORK_OPEN_INFORMATION;
typedef enum _FILE_INFORMATION_CLASS
{
    FileDirectoryInformation = 1,
    FileFullDirectoryInformation,
    FileBothDirectoryInformation,
    FileBasicInformation,
    FileStandardInformation,
    FileInternalInformation,
    FileEaInformation,
    FileAccessInformation,
    FileNameInformation,
    FileRenameInformation,
    FileLinkInformation,
    FileNamesInformation,
    FileDispositionInformation,
    FilePositionInformation,
    FileFullEaInformation,
    FileModeInformation,
    FileAlignmentInformation,
    FileAllInformation,
    FileAllocationInformation,
    FileEndOfFileInformation,
    FileAlternateNameInformation,
    FileStreamInformation,
    FilePipeInformation,
    FilePipeLocalInformation,
    FilePipeRemoteInformation,
    FileMailslotQueryInformation,
    FileMailslotSetInformation,
    FileCompressionInformation,
    FileObjectIdInformation,
    FileCompletionInformation,
    FileMoveClusterInformation,
    FileQuotaInformation,
    FileReparsePointInformation,
    FileNetworkOpenInformation,
    FileAttributeTagInformation,
    FileTrackingInformation,
    FileIdBothDirectoryInformation,
    FileIdFullDirectoryInformation,
    FileValidDataLengthInformation,
    FileShortNameInformation,
    FileIoCompletionNotificationInformation,
    FileIoStatusBlockRangeInformation,
    FileIoPriorityHintInformation,
    FileSfioReserveInformation,
    FileSfioVolumeInformation,
    FileHardLinkInformation,
    FileProcessIdsUsingFileInformation,
    FileNormalizedNameInformation,
    FileNetworkPhysicalNameInformation,
    FileIdGlobalTxDirectoryInformation,
    FileIsRemoteDeviceInformation,
    FileUnusedInformation,
    FileNumaNodeInformation,
    FileStandardLinkInformation,
    FileRemoteProtocolInformation,
    FileRenameInformationBypassAccessCheck,
    FileLinkInformationBypassAccessCheck,
    FileVolumeNameInformation,
    FileIdInformation,
    FileIdExtdDirectoryInformation,
    FileReplaceCompletionInformation,
    FileHardLinkFullIdInformation,
    FileMaximumInformation
} FILE_INFORMATION_CLASS;
typedef FILE_INFORMATION_CLASS *PFILE_INFORMATION_CLASS;
NTSYSAPI NTSTATUS NTAPI NtQueryInformationFile(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
NTSYSAPI NTSTATUS NTAPI NtQueryDirectoryFile(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG,
                                             FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN);
NTSYSAPI NTSTATUS NTAPI NtSetInformationFile(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
#endif /* IPRT_NT_USE_WINTERNL */
NTSYSAPI NTSTATUS NTAPI NtQueryAttributesFile(POBJECT_ATTRIBUTES, PFILE_BASIC_INFORMATION);
NTSYSAPI NTSTATUS NTAPI NtQueryFullAttributesFile(POBJECT_ATTRIBUTES, PFILE_NETWORK_OPEN_INFORMATION);

#ifdef IPRT_NT_USE_WINTERNL

/** For use with KeyBasicInformation. */
typedef struct _KEY_BASIC_INFORMATION
{
    LARGE_INTEGER   LastWriteTime;
    ULONG           TitleIndex;
    ULONG           NameLength;
    WCHAR           Name[1];
} KEY_BASIC_INFORMATION;
typedef KEY_BASIC_INFORMATION *PKEY_BASIC_INFORMATION;

/** For use with KeyNodeInformation. */
typedef struct _KEY_NODE_INFORMATION
{
    LARGE_INTEGER   LastWriteTime;
    ULONG           TitleIndex;
    ULONG           ClassOffset; /**< Offset from the start of the structure. */
    ULONG           ClassLength;
    ULONG           NameLength;
    WCHAR           Name[1];
} KEY_NODE_INFORMATION;
typedef KEY_NODE_INFORMATION *PKEY_NODE_INFORMATION;

/** For use with KeyFullInformation. */
typedef struct _KEY_FULL_INFORMATION
{
    LARGE_INTEGER   LastWriteTime;
    ULONG           TitleIndex;
    ULONG           ClassOffset; /**< Offset of the Class member. */
    ULONG           ClassLength;
    ULONG           SubKeys;
    ULONG           MaxNameLen;
    ULONG           MaxClassLen;
    ULONG           Values;
    ULONG           MaxValueNameLen;
    ULONG           MaxValueDataLen;
    WCHAR           Class[1];
} KEY_FULL_INFORMATION;
typedef KEY_FULL_INFORMATION *PKEY_FULL_INFORMATION;

/** For use with KeyNameInformation. */
typedef struct _KEY_NAME_INFORMATION
{
    ULONG           NameLength;
    WCHAR           Name[1];
} KEY_NAME_INFORMATION;
typedef KEY_NAME_INFORMATION *PKEY_NAME_INFORMATION;

/** For use with KeyCachedInformation. */
typedef struct _KEY_CACHED_INFORMATION
{
    LARGE_INTEGER   LastWriteTime;
    ULONG           TitleIndex;
    ULONG           SubKeys;
    ULONG           MaxNameLen;
    ULONG           Values;
    ULONG           MaxValueNameLen;
    ULONG           MaxValueDataLen;
    ULONG           NameLength;
} KEY_CACHED_INFORMATION;
typedef KEY_CACHED_INFORMATION *PKEY_CACHED_INFORMATION;

/** For use with KeyVirtualizationInformation. */
typedef struct _KEY_VIRTUALIZATION_INFORMATION
{
    ULONG           VirtualizationCandidate : 1;
    ULONG           VirtualizationEnabled   : 1;
    ULONG           VirtualTarget           : 1;
    ULONG           VirtualStore            : 1;
    ULONG           VirtualSource           : 1;
    ULONG           Reserved                : 27;
} KEY_VIRTUALIZATION_INFORMATION;
typedef KEY_VIRTUALIZATION_INFORMATION *PKEY_VIRTUALIZATION_INFORMATION;

typedef enum _KEY_INFORMATION_CLASS
{
    KeyBasicInformation = 0,
    KeyNodeInformation,
    KeyFullInformation,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation,
    KeyVirtualizationInformation,
    KeyHandleTagsInformation,
    MaxKeyInfoClass
} KEY_INFORMATION_CLASS;
NTSYSAPI NTSTATUS NTAPI NtQueryKey(HANDLE, KEY_INFORMATION_CLASS, PVOID, ULONG, PULONG);
NTSYSAPI NTSTATUS NTAPI NtEnumerateKey(HANDLE, ULONG, KEY_INFORMATION_CLASS, PVOID, ULONG, PULONG);

typedef struct _MEMORY_SECTION_NAME
{
    UNICODE_STRING  SectionFileName;
    WCHAR           NameBuffer[1];
} MEMORY_SECTION_NAME;

#ifdef IPRT_NT_USE_WINTERNL
typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    int32_t BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;
typedef PROCESS_BASIC_INFORMATION *PPROCESS_BASIC_INFORMATION;
#endif

typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation = 0,        /**<  0 / 0x00 */
    ProcessQuotaLimits,                 /**<  1 / 0x01 */
    ProcessIoCounters,                  /**<  2 / 0x02 */
    ProcessVmCounters,                  /**<  3 / 0x03 */
    ProcessTimes,                       /**<  4 / 0x04 */
    ProcessBasePriority,                /**<  5 / 0x05 */
    ProcessRaisePriority,               /**<  6 / 0x06 */
    ProcessDebugPort,                   /**<  7 / 0x07 */
    ProcessExceptionPort,               /**<  8 / 0x08 */
    ProcessAccessToken,                 /**<  9 / 0x09 */
    ProcessLdtInformation,              /**< 10 / 0x0a */
    ProcessLdtSize,                     /**< 11 / 0x0b */
    ProcessDefaultHardErrorMode,        /**< 12 / 0x0c */
    ProcessIoPortHandlers,              /**< 13 / 0x0d */
    ProcessPooledUsageAndLimits,        /**< 14 / 0x0e */
    ProcessWorkingSetWatch,             /**< 15 / 0x0f */
    ProcessUserModeIOPL,                /**< 16 / 0x10 */
    ProcessEnableAlignmentFaultFixup,   /**< 17 / 0x11 */
    ProcessPriorityClass,               /**< 18 / 0x12 */
    ProcessWx86Information,             /**< 19 / 0x13 */
    ProcessHandleCount,                 /**< 20 / 0x14 */
    ProcessAffinityMask,                /**< 21 / 0x15 */
    ProcessPriorityBoost,               /**< 22 / 0x16 */
    ProcessDeviceMap,                   /**< 23 / 0x17 */
    ProcessSessionInformation,          /**< 24 / 0x18 */
    ProcessForegroundInformation,       /**< 25 / 0x19 */
    ProcessWow64Information,            /**< 26 / 0x1a */
    ProcessImageFileName,               /**< 27 / 0x1b */
    ProcessLUIDDeviceMapsEnabled,       /**< 28 / 0x1c */
    ProcessBreakOnTermination,          /**< 29 / 0x1d */
    ProcessDebugObjectHandle,           /**< 30 / 0x1e */
    ProcessDebugFlags,                  /**< 31 / 0x1f */
    ProcessHandleTracing,               /**< 32 / 0x20 */
    ProcessIoPriority,                  /**< 33 / 0x21 */
    ProcessExecuteFlags,                /**< 34 / 0x22 */
    ProcessTlsInformation,              /**< 35 / 0x23 */
    ProcessCookie,                      /**< 36 / 0x24 */
    ProcessImageInformation,            /**< 37 / 0x25 */
    ProcessCycleTime,                   /**< 38 / 0x26 */
    ProcessPagePriority,                /**< 39 / 0x27 */
    ProcessInstrumentationCallbak,      /**< 40 / 0x28 */
    ProcessThreadStackAllocation,       /**< 41 / 0x29 */
    ProcessWorkingSetWatchEx,           /**< 42 / 0x2a */
    ProcessImageFileNameWin32,          /**< 43 / 0x2b */
    ProcessImageFileMapping,            /**< 44 / 0x2c */
    ProcessAffinityUpdateMode,          /**< 45 / 0x2d */
    ProcessMemoryAllocationMode,        /**< 46 / 0x2e */
    ProcessGroupInformation,            /**< 47 / 0x2f */
    ProcessTokenVirtualizationEnabled,  /**< 48 / 0x30 */
    ProcessConsoleHostProcess,          /**< 49 / 0x31 */
    ProcessWindowsInformation,          /**< 50 / 0x32 */
    ProcessUnknown51,
    ProcessUnknown52,
    ProcessUnknown53,
    ProcessUnknown54,
    ProcessUnknown55,
    ProcessUnknown56,
    ProcessUnknown57,
    ProcessUnknown58,
    ProcessUnknown59,
    ProcessUnknown60,
    ProcessUnknown61,
    ProcessUnknown62,
    ProcessUnknown63,
    ProcessUnknown64,
    ProcessUnknown65,
    ProcessUnknown66,
    ProcessMaybe_KeSetCpuSetsProcess,   /**< 67 / 0x43 - is correct, then PROCESS_SET_LIMITED_INFORMATION & audiog.exe; W10. */
    MaxProcessInfoClass                 /**< 68 / 0x44 */
} PROCESSINFOCLASS;
NTSYSAPI NTSTATUS NTAPI NtQueryInformationProcess(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

typedef enum _THREADINFOCLASS
{
    ThreadBasicInformation = 0,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    ThreadBreakOnTermination,
    ThreadSwitchLegacyState,
    ThreadIsTerminated,
    ThreadLastSystemCall,
    ThreadIoPriority,
    ThreadCycleTime,
    ThreadPagePriority,
    ThreadActualBasePriority,
    ThreadTebInformation,
    ThreadCSwitchMon,
    ThreadCSwitchPmu,
    ThreadWow64Context,
    ThreadGroupInformation,
    ThreadUmsInformation,
    ThreadCounterProfiling,
    ThreadIdealProcessorEx,
    ThreadCpuAccountingInformation,
    MaxThreadInfoClass
} THREADINFOCLASS;
NTSYSAPI NTSTATUS NTAPI NtSetInformationThread(HANDLE, THREADINFOCLASS, LPCVOID, ULONG);

NTSYSAPI NTSTATUS NTAPI NtQueryInformationToken(HANDLE, TOKEN_INFORMATION_CLASS, PVOID, ULONG, PULONG);

NTSYSAPI NTSTATUS NTAPI NtReadFile(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
NTSYSAPI NTSTATUS NTAPI NtWriteFile(HANDLE, HANDLE, PIO_APC_ROUTINE, void const *, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
NTSYSAPI NTSTATUS NTAPI NtFlushBuffersFile(HANDLE, PIO_STATUS_BLOCK);

NTSYSAPI NTSTATUS NTAPI NtReadVirtualMemory(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
NTSYSAPI NTSTATUS NTAPI NtWriteVirtualMemory(HANDLE, PVOID, void const *, SIZE_T, PSIZE_T);

NTSYSAPI NTSTATUS NTAPI RtlAddAccessAllowedAce(PACL, ULONG, ULONG, PSID);
NTSYSAPI NTSTATUS NTAPI RtlCopySid(ULONG, PSID, PSID);
NTSYSAPI NTSTATUS NTAPI RtlCreateAcl(PACL, ULONG, ULONG);
NTSYSAPI NTSTATUS NTAPI RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR, ULONG);
NTSYSAPI BOOLEAN  NTAPI RtlEqualSid(PSID, PSID);
NTSYSAPI NTSTATUS NTAPI RtlGetVersion(PRTL_OSVERSIONINFOW);
NTSYSAPI NTSTATUS NTAPI RtlInitializeSid(PSID, PSID_IDENTIFIER_AUTHORITY, UCHAR);
NTSYSAPI NTSTATUS NTAPI RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR, BOOLEAN, PACL, BOOLEAN);
NTSYSAPI PULONG   NTAPI RtlSubAuthoritySid(PSID, ULONG);

#endif /* IPRT_NT_USE_WINTERNL */

typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation = 0,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllInformation,
    ObjectDataInformation
} OBJECT_INFORMATION_CLASS;
typedef OBJECT_INFORMATION_CLASS *POBJECT_INFORMATION_CLASS;
#ifdef IN_RING0
# define NtQueryObject ZwQueryObject
#endif
NTSYSAPI NTSTATUS NTAPI NtQueryObject(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG, PULONG);
NTSYSAPI NTSTATUS NTAPI NtSetInformationObject(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG);
NTSYSAPI NTSTATUS NTAPI NtDuplicateObject(HANDLE, HANDLE, HANDLE, PHANDLE, ACCESS_MASK, ULONG, ULONG);

NTSYSAPI NTSTATUS NTAPI NtOpenDirectoryObject(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);

typedef struct _OBJECT_DIRECTORY_INFORMATION
{
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION;
typedef OBJECT_DIRECTORY_INFORMATION *POBJECT_DIRECTORY_INFORMATION;
NTSYSAPI NTSTATUS NTAPI NtQueryDirectoryObject(HANDLE, PVOID, ULONG, BOOLEAN, BOOLEAN, PULONG, PULONG);

NTSYSAPI NTSTATUS NTAPI NtSuspendProcess(HANDLE);
NTSYSAPI NTSTATUS NTAPI NtResumeProcess(HANDLE);
/** @name ProcessDefaultHardErrorMode bit definitions.
 * @{ */
#define PROCESS_HARDERR_CRITICAL_ERROR              UINT32_C(0x00000001) /**< Inverted from the win32 define. */
#define PROCESS_HARDERR_NO_GP_FAULT_ERROR           UINT32_C(0x00000002)
#define PROCESS_HARDERR_NO_ALIGNMENT_FAULT_ERROR    UINT32_C(0x00000004)
#define PROCESS_HARDERR_NO_OPEN_FILE_ERROR          UINT32_C(0x00008000)
/** @} */
NTSYSAPI NTSTATUS NTAPI NtSetInformationProcess(HANDLE, PROCESSINFOCLASS, PVOID, ULONG);
NTSYSAPI NTSTATUS NTAPI NtTerminateProcess(HANDLE, LONG);

/** Retured by ProcessImageInformation as well as NtQuerySection. */
typedef struct _SECTION_IMAGE_INFORMATION
{
    PVOID TransferAddress;
    ULONG ZeroBits;
    SIZE_T MaximumStackSize;
    SIZE_T CommittedStackSize;
    ULONG SubSystemType;
    union
    {
        struct
        {
            USHORT SubSystemMinorVersion;
            USHORT SubSystemMajorVersion;
        };
        ULONG SubSystemVersion;
    };
    ULONG GpValue;
    USHORT ImageCharacteristics;
    USHORT DllCharacteristics;
    USHORT Machine;
    BOOLEAN ImageContainsCode;
    union /**< Since Vista, used to be a spare BOOLEAN. */
    {
        struct
        {
            UCHAR ComPlusNativeRead : 1;
            UCHAR ComPlusILOnly : 1;
            UCHAR ImageDynamicallyRelocated : 1;
            UCHAR ImageMAppedFlat : 1;
            UCHAR Reserved : 4;
        };
        UCHAR ImageFlags;
    };
    ULONG LoaderFlags;
    ULONG ImageFileSize; /**< Since XP? */
    ULONG CheckSum; /**< Since Vista, Used to be a reserved/spare ULONG. */
} SECTION_IMAGE_INFORMATION;
typedef SECTION_IMAGE_INFORMATION *PSECTION_IMAGE_INFORMATION;

typedef enum _SECTION_INFORMATION_CLASS
{
    SectionBasicInformation = 0,
    SectionImageInformation,
    MaxSectionInfoClass
} SECTION_INFORMATION_CLASS;
NTSYSAPI NTSTATUS NTAPI NtQuerySection(HANDLE, SECTION_INFORMATION_CLASS, PVOID, SIZE_T, PSIZE_T);

NTSYSAPI NTSTATUS NTAPI NtCreateSymbolicLinkObject(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PUNICODE_STRING pTarget);
NTSYSAPI NTSTATUS NTAPI NtOpenSymbolicLinkObject(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
NTSYSAPI NTSTATUS NTAPI NtQuerySymbolicLinkObject(HANDLE, PUNICODE_STRING, PULONG);
#ifndef SYMBOLIC_LINK_QUERY
# define SYMBOLIC_LINK_QUERY        UINT32_C(0x00000001)
#endif
#ifndef SYMBOLIC_LINK_ALL_ACCESS
# define SYMBOLIC_LINK_ALL_ACCESS   (STANDARD_RIGHTS_REQUIRED | SYMBOLIC_LINK_QUERY)
#endif

NTSYSAPI NTSTATUS NTAPI NtQueryInformationThread(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
NTSYSAPI NTSTATUS NTAPI NtResumeThread(HANDLE, PULONG);
NTSYSAPI NTSTATUS NTAPI NtSuspendThread(HANDLE, PULONG);
NTSYSAPI NTSTATUS NTAPI NtTerminateThread(HANDLE, LONG);
NTSYSAPI NTSTATUS NTAPI NtGetContextThread(HANDLE, PCONTEXT);
NTSYSAPI NTSTATUS NTAPI NtSetContextThread(HANDLE, PCONTEXT);


#ifndef SEC_FILE
# define SEC_FILE               UINT32_C(0x00800000)
#endif
#ifndef SEC_IMAGE
# define SEC_IMAGE              UINT32_C(0x01000000)
#endif
#ifndef SEC_PROTECTED_IMAGE
# define SEC_PROTECTED_IMAGE    UINT32_C(0x02000000)
#endif
#ifndef SEC_NOCACHE
# define SEC_NOCACHE            UINT32_C(0x10000000)
#endif
#ifndef MEM_ROTATE
# define MEM_ROTATE             UINT32_C(0x00800000)
#endif
typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation = 0,
    MemoryWorkingSetList,
    MemorySectionName,
    MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS;
#ifdef IN_RING0
typedef struct _MEMORY_BASIC_INFORMATION
{
    PVOID BaseAddress;
    PVOID AllocationBase;
    ULONG AllocationProtect;
    SIZE_T RegionSize;
    ULONG State;
    ULONG Protect;
    ULONG Type;
} MEMORY_BASIC_INFORMATION;
typedef MEMORY_BASIC_INFORMATION *PMEMORY_BASIC_INFORMATION;
# define NtQueryVirtualMemory ZwQueryVirtualMemory
#endif
NTSYSAPI NTSTATUS NTAPI NtQueryVirtualMemory(HANDLE, void const *, MEMORY_INFORMATION_CLASS, PVOID, SIZE_T, PSIZE_T);
#ifdef IPRT_NT_USE_WINTERNL
NTSYSAPI NTSTATUS NTAPI NtAllocateVirtualMemory(HANDLE, PVOID *, ULONG, PSIZE_T, ULONG, ULONG);
#endif
NTSYSAPI NTSTATUS NTAPI NtFreeVirtualMemory(HANDLE, PVOID *, PSIZE_T, ULONG);
NTSYSAPI NTSTATUS NTAPI NtProtectVirtualMemory(HANDLE, PVOID *, PSIZE_T, ULONG, PULONG);

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation = 0,
    SystemCpuInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemInformation_Unknown_4,
    SystemProcessInformation,
    SystemInformation_Unknown_6,
    SystemInformation_Unknown_7,
    SystemProcessorPerformanceInformation,
    SystemInformation_Unknown_9,
    SystemInformation_Unknown_10,
    SystemModuleInformation,
    SystemInformation_Unknown_12,
    SystemInformation_Unknown_13,
    SystemInformation_Unknown_14,
    SystemInformation_Unknown_15,
    SystemHandleInformation,
    SystemInformation_Unknown_17,
    SystemPageFileInformation,
    SystemInformation_Unknown_19,
    SystemInformation_Unknown_20,
    SystemCacheInformation,
    SystemInformation_Unknown_22,
    SystemInterruptInformation,
    SystemDpcBehaviourInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation, /* 26 */
    SystemUnloadGdiDriverInformation, /* 27 */
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemInformation_Unknown_30,
    SystemInformation_Unknown_31,
    SystemInformation_Unknown_32,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemInformation_Unknown_38,
    SystemInformation_Unknown_39,
    SystemInformation_Unknown_40,
    SystemInformation_Unknown_41,
    SystemInformation_Unknown_42,
    SystemInformation_Unknown_43,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemSetTimeSlipEvent,
    SystemCreateSession,
    SystemDeleteSession,
    SystemInformation_Unknown_49,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemInformation_Unknown_52,
    SystemSessionProcessInformation,
    SystemLoadGdiDriverInSystemSpaceInformation, /* 54 */
    SystemInformation_Unknown_55,
    SystemInformation_Unknown_56,
    SystemExtendedProcessInformation,
    SystemInformation_Unknown_58,
    SystemInformation_Unknown_59,
    SystemInformation_Unknown_60,
    SystemInformation_Unknown_61,
    SystemInformation_Unknown_62,
    SystemInformation_Unknown_63,
    SystemExtendedHandleInformation, /* 64 */
    SystemInformation_Unknown_65,
    SystemInformation_Unknown_66,
    SystemInformation_Unknown_67,
    SystemInformation_Unknown_68,
    SystemInformation_HotPatchInfo, /* 69 */
    SystemInformation_Unknown_70,
    SystemInformation_Unknown_71,
    SystemInformation_Unknown_72,
    SystemInformation_Unknown_73,
    SystemInformation_Unknown_74,
    SystemInformation_Unknown_75,
    SystemInformation_Unknown_76,
    SystemInformation_Unknown_77,
    SystemInformation_Unknown_78,
    SystemInformation_Unknown_79,
    SystemInformation_Unknown_80,
    SystemInformation_Unknown_81,
    SystemInformation_Unknown_82,
    SystemInformation_Unknown_83,
    SystemInformation_Unknown_84,
    SystemInformation_Unknown_85,
    SystemInformation_Unknown_86,
    SystemInformation_Unknown_87,
    SystemInformation_Unknown_88,
    SystemInformation_Unknown_89,
    SystemInformation_Unknown_90,
    SystemInformation_Unknown_91,
    SystemInformation_Unknown_92,
    SystemInformation_Unknown_93,
    SystemInformation_Unknown_94,
    SystemInformation_Unknown_95,
    SystemInformation_KiOpPrefetchPatchCount, /* 96 */
    SystemInformation_Unknown_97,
    SystemInformation_Unknown_98,
    SystemInformation_Unknown_99,
    SystemInformation_Unknown_100,
    SystemInformation_Unknown_101,
    SystemInformation_Unknown_102,
    SystemInformation_Unknown_103,
    SystemInformation_Unknown_104,
    SystemInformation_Unknown_105,
    SystemInformation_Unknown_107,
    SystemInformation_GetLogicalProcessorInformationEx, /* 107 */

    /** @todo fill gap. they've added a whole bunch of things  */
    SystemPolicyInformation = 134,
    SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS;

#ifdef IPRT_NT_USE_WINTERNL
typedef struct _VM_COUNTERS
{
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS;
typedef VM_COUNTERS *PVM_COUNTERS;
#endif

#if 0
typedef struct _IO_COUNTERS
{
    ULONGLONG ReadOperationCount;
    ULONGLONG WriteOperationCount;
    ULONGLONG OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
} IO_COUNTERS;
typedef IO_COUNTERS *PIO_COUNTERS;
#endif

typedef struct _RTNT_SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;          /**< 0x00 / 0x00 */
    ULONG NumberOfThreads;          /**< 0x04 / 0x04 */
    LARGE_INTEGER Reserved1[3];     /**< 0x08 / 0x08 */
    LARGE_INTEGER CreationTime;     /**< 0x20 / 0x20 */
    LARGE_INTEGER UserTime;         /**< 0x28 / 0x28 */
    LARGE_INTEGER KernelTime;       /**< 0x30 / 0x30 */
    UNICODE_STRING ProcessName;     /**< 0x38 / 0x38 Clean unicode encoding? */
    int32_t BasePriority;           /**< 0x40 / 0x48 */
    HANDLE UniqueProcessId;         /**< 0x44 / 0x50 */
    HANDLE ParentProcessId;         /**< 0x48 / 0x58 */
    ULONG HandleCount;              /**< 0x4c / 0x60 */
    ULONG Reserved2;                /**< 0x50 / 0x64 Session ID? */
    ULONG_PTR Reserved3;            /**< 0x54 / 0x68 */
    VM_COUNTERS VmCounters;         /**< 0x58 / 0x70 */
    IO_COUNTERS IoCounters;         /**< 0x88 / 0xd0 Might not be present in earlier windows versions. */
    /* After this follows the threads, then the ProcessName.Buffer. */
} RTNT_SYSTEM_PROCESS_INFORMATION;
typedef RTNT_SYSTEM_PROCESS_INFORMATION *PRTNT_SYSTEM_PROCESS_INFORMATION;
#ifndef IPRT_NT_USE_WINTERNL
typedef RTNT_SYSTEM_PROCESS_INFORMATION SYSTEM_PROCESS_INFORMATION;
typedef SYSTEM_PROCESS_INFORMATION *PSYSTEM_PROCESS_INFORMATION;
#endif

typedef struct _SYSTEM_HANDLE_ENTRY_INFO
{
    USHORT UniqueProcessId;
    USHORT CreatorBackTraceIndex;
    UCHAR ObjectTypeIndex;
    UCHAR HandleAttributes;
    USHORT HandleValue;
    PVOID Object;
    ULONG GrantedAccess;
} SYSTEM_HANDLE_ENTRY_INFO;
typedef SYSTEM_HANDLE_ENTRY_INFO *PSYSTEM_HANDLE_ENTRY_INFO;

/** Returned by SystemHandleInformation  */
typedef struct _SYSTEM_HANDLE_INFORMATION
{
    ULONG NumberOfHandles;
    SYSTEM_HANDLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION;
typedef SYSTEM_HANDLE_INFORMATION *PSYSTEM_HANDLE_INFORMATION;

/** Extended handle information entry.
 * @remarks 3 x PVOID + 4 x ULONG = 28 bytes on 32-bit / 40 bytes on 64-bit  */
typedef struct _SYSTEM_HANDLE_ENTRY_INFO_EX
{
    PVOID Object;
    HANDLE UniqueProcessId;
    HANDLE HandleValue;
    ACCESS_MASK GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} SYSTEM_HANDLE_ENTRY_INFO_EX;
typedef SYSTEM_HANDLE_ENTRY_INFO_EX *PSYSTEM_HANDLE_ENTRY_INFO_EX;

/** Returned by SystemExtendedHandleInformation.  */
typedef struct _SYSTEM_HANDLE_INFORMATION_EX
{
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    SYSTEM_HANDLE_ENTRY_INFO_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX;
typedef SYSTEM_HANDLE_INFORMATION_EX *PSYSTEM_HANDLE_INFORMATION_EX;

/** Returned by SystemSessionProcessInformation. */
typedef struct _SYSTEM_SESSION_PROCESS_INFORMATION
{
    ULONG SessionId;
    ULONG BufferLength;
    /** Return buffer, SYSTEM_PROCESS_INFORMATION entries. */
    PVOID Buffer;
} SYSTEM_SESSION_PROCESS_INFORMATION;
typedef SYSTEM_SESSION_PROCESS_INFORMATION *PSYSTEM_SESSION_PROCESS_INFORMATION;

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    HANDLE Section;                 /**< 0x00 / 0x00 */
    PVOID MappedBase;               /**< 0x04 / 0x08 */
    PVOID ImageBase;                /**< 0x08 / 0x10 */
    ULONG ImageSize;                /**< 0x0c / 0x18 */
    ULONG Flags;                    /**< 0x10 / 0x1c */
    USHORT LoadOrderIndex;          /**< 0x14 / 0x20 */
    USHORT InitOrderIndex;          /**< 0x16 / 0x22 */
    USHORT LoadCount;               /**< 0x18 / 0x24 */
    USHORT OffsetToFileName;        /**< 0x1a / 0x26 */
    UCHAR  FullPathName[256];       /**< 0x1c / 0x28 */
} RTL_PROCESS_MODULE_INFORMATION;
typedef RTL_PROCESS_MODULE_INFORMATION *PRTL_PROCESS_MODULE_INFORMATION;

/** Returned by SystemModuleInformation. */
typedef struct _RTL_PROCESS_MODULES
{
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];  /**< 0x04 / 0x08 */
} RTL_PROCESS_MODULES;
typedef RTL_PROCESS_MODULES *PRTL_PROCESS_MODULES;

NTSYSAPI NTSTATUS NTAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG cNtTicksWanted, BOOLEAN fSetResolution, PULONG pcNtTicksCur);
NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(PULONG pcNtTicksMin, PULONG pcNtTicksMax, PULONG pcNtTicksCur);

NTSYSAPI NTSTATUS NTAPI NtDelayExecution(BOOLEAN, PLARGE_INTEGER);
NTSYSAPI NTSTATUS NTAPI NtYieldExecution(void);
#ifndef IPRT_NT_USE_WINTERNL
NTSYSAPI NTSTATUS NTAPI NtWaitForSingleObject(HANDLE, BOOLEAN PLARGE_INTEGER);
#endif
typedef NTSYSAPI NTSTATUS (NTAPI *PFNNTWAITFORSINGLEOBJECT)(HANDLE, BOOLEAN, PLARGE_INTEGER);
typedef enum _OBJECT_WAIT_TYPE { WaitAllObjects = 0, WaitAnyObject = 1, ObjectWaitTypeHack = 0x7fffffff } OBJECT_WAIT_TYPE;
NTSYSAPI NTSTATUS NTAPI NtWaitForMultipleObjects(ULONG, PHANDLE, OBJECT_WAIT_TYPE, BOOLEAN, PLARGE_INTEGER);

NTSYSAPI NTSTATUS NTAPI NtQuerySecurityObject(HANDLE, ULONG, PSECURITY_DESCRIPTOR, ULONG, PULONG);

#ifdef IPRT_NT_USE_WINTERNL
typedef enum _EVENT_TYPE
{
    /* Manual reset event. */
    NotificationEvent = 0,
    /* Automaitc reset event. */
    SynchronizationEvent
} EVENT_TYPE;
#endif
NTSYSAPI NTSTATUS NTAPI NtCreateEvent(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, EVENT_TYPE, BOOLEAN);
NTSYSAPI NTSTATUS NTAPI NtOpenEvent(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
typedef NTSYSAPI NTSTATUS (NTAPI *PFNNTCLEAREVENT)(HANDLE);
NTSYSAPI NTSTATUS NTAPI NtClearEvent(HANDLE);
NTSYSAPI NTSTATUS NTAPI NtResetEvent(HANDLE, PULONG);
NTSYSAPI NTSTATUS NTAPI NtSetEvent(HANDLE, PULONG);
typedef NTSYSAPI NTSTATUS (NTAPI *PFNNTSETEVENT)(HANDLE, PULONG);
typedef enum _EVENT_INFORMATION_CLASS
{
    EventBasicInformation = 0
} EVENT_INFORMATION_CLASS;
/** Data returned by NtQueryEvent + EventBasicInformation. */
typedef struct EVENT_BASIC_INFORMATION
{
    EVENT_TYPE  EventType;
    ULONG       EventState;
} EVENT_BASIC_INFORMATION;
typedef EVENT_BASIC_INFORMATION *PEVENT_BASIC_INFORMATION;
NTSYSAPI NTSTATUS NTAPI NtQueryEvent(HANDLE, EVENT_INFORMATION_CLASS, PVOID, ULONG, PULONG);

#ifdef IPRT_NT_USE_WINTERNL
/** For NtQueryValueKey. */
typedef enum _KEY_VALUE_INFORMATION_CLASS
{
    KeyValueBasicInformation = 0,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

/** KeyValuePartialInformation and KeyValuePartialInformationAlign64 struct. */
typedef struct _KEY_VALUE_PARTIAL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION;
typedef KEY_VALUE_PARTIAL_INFORMATION *PKEY_VALUE_PARTIAL_INFORMATION;
#endif
NTSYSAPI NTSTATUS NTAPI NtOpenKey(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
NTSYSAPI NTSTATUS NTAPI NtQueryValueKey(HANDLE, PUNICODE_STRING, KEY_VALUE_INFORMATION_CLASS, PVOID, ULONG, PULONG);


NTSYSAPI NTSTATUS NTAPI RtlAddAccessDeniedAce(PACL, ULONG, ULONG, PSID);


typedef struct _CURDIR
{
    UNICODE_STRING  DosPath;
    HANDLE          Handle;
} CURDIR;
typedef CURDIR *PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    USHORT          Flags;
    USHORT          Length;
    ULONG           TimeStamp;
    STRING          DosPath; /**< Yeah, it's STRING according to dt ntdll!_RTL_DRIVE_LETTER_CURDIR. */
} RTL_DRIVE_LETTER_CURDIR;
typedef RTL_DRIVE_LETTER_CURDIR *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG           MaximumLength;
    ULONG           Length;
    ULONG           Flags;
    ULONG           DebugFlags;
    HANDLE          ConsoleHandle;
    ULONG           ConsoleFlags;
    HANDLE          StandardInput;
    HANDLE          StandardOutput;
    HANDLE          StandardError;
    CURDIR          CurrentDirectory;
    UNICODE_STRING  DllPath;
    UNICODE_STRING  ImagePathName;
    UNICODE_STRING  CommandLine;
    PWSTR           Environment;
    ULONG           StartingX;
    ULONG           StartingY;
    ULONG           CountX;
    ULONG           CountY;
    ULONG           CountCharsX;
    ULONG           CountCharsY;
    ULONG           FillAttribute;
    ULONG           WindowFlags;
    ULONG           ShowWindowFlags;
    UNICODE_STRING  WindowTitle;
    UNICODE_STRING  DesktopInfo;
    UNICODE_STRING  ShellInfo;
    UNICODE_STRING  RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR  CurrentDirectories[0x20];
    SIZE_T          EnvironmentSize;        /**< Added in Vista */
    SIZE_T          EnvironmentVersion;     /**< Added in Windows 7. */
    PVOID           PackageDependencyData;  /**< Added Windows 8? */
    ULONG           ProcessGroupId;         /**< Added Windows 8? */
} RTL_USER_PROCESS_PARAMETERS;
typedef RTL_USER_PROCESS_PARAMETERS *PRTL_USER_PROCESS_PARAMETERS;
#define RTL_USER_PROCESS_PARAMS_FLAG_NORMALIZED     1

typedef struct _RTL_USER_PROCESS_INFORMATION
{
    ULONG           Size;
    HANDLE          ProcessHandle;
    HANDLE          ThreadHandle;
    CLIENT_ID       ClientId;
    SECTION_IMAGE_INFORMATION  ImageInformation;
} RTL_USER_PROCESS_INFORMATION;
typedef RTL_USER_PROCESS_INFORMATION *PRTL_USER_PROCESS_INFORMATION;


NTSYSAPI NTSTATUS NTAPI RtlCreateUserProcess(PUNICODE_STRING, ULONG, PRTL_USER_PROCESS_PARAMETERS, PSECURITY_DESCRIPTOR,
                                             PSECURITY_DESCRIPTOR, HANDLE, BOOLEAN, HANDLE, HANDLE, PRTL_USER_PROCESS_INFORMATION);
NTSYSAPI NTSTATUS NTAPI RtlCreateProcessParameters(PRTL_USER_PROCESS_PARAMETERS *, PUNICODE_STRING ImagePathName,
                                                   PUNICODE_STRING DllPath, PUNICODE_STRING CurrentDirectory,
                                                   PUNICODE_STRING CommandLine, PUNICODE_STRING Environment,
                                                   PUNICODE_STRING WindowTitle, PUNICODE_STRING DesktopInfo,
                                                   PUNICODE_STRING ShellInfo, PUNICODE_STRING RuntimeInfo);
NTSYSAPI VOID     NTAPI RtlDestroyProcessParameters(PRTL_USER_PROCESS_PARAMETERS);
NTSYSAPI NTSTATUS NTAPI RtlCreateUserThread(HANDLE, PSECURITY_DESCRIPTOR, BOOLEAN, ULONG, SIZE_T, SIZE_T,
                                            PFNRT, PVOID, PHANDLE, PCLIENT_ID);

#ifndef RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO
typedef struct _RTL_CRITICAL_SECTION
{
    struct _RTL_CRITICAL_SECTION_DEBUG *DebugInfo;
    LONG            LockCount;
    LONG            Recursioncount;
    HANDLE          OwningThread;
    HANDLE          LockSemaphore;
    ULONG_PTR       SpinCount;
} RTL_CRITICAL_SECTION;
typedef RTL_CRITICAL_SECTION *PRTL_CRITICAL_SECTION;
#endif

/*NTSYSAPI ULONG NTAPI RtlNtStatusToDosError(NTSTATUS rcNt);*/

/** @def RTL_QUERY_REGISTRY_TYPECHECK
 * WDK 8.1+, backported in updates, ignored in older. */
#if !defined(RTL_QUERY_REGISTRY_TYPECHECK) || defined(DOXYGEN_RUNNING)
# define RTL_QUERY_REGISTRY_TYPECHECK       UINT32_C(0x00000100)
#endif
/** @def RTL_QUERY_REGISTRY_TYPECHECK_SHIFT
 * WDK 8.1+, backported in updates, ignored in older. */
#if !defined(RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) || defined(DOXYGEN_RUNNING)
# define RTL_QUERY_REGISTRY_TYPECHECK_SHIFT 24
#endif


RT_C_DECLS_END
/** @} */


#if defined(IN_RING0) || defined(DOXYGEN_RUNNING)
/** @name NT Kernel APIs
 * @{ */
RT_C_DECLS_BEGIN

typedef ULONG KEPROCESSORINDEX; /**< Bitmap indexes != process numbers, apparently. */

NTSYSAPI VOID     NTAPI KeInitializeAffinityEx(PKAFFINITY_EX pAffinity);
typedef  VOID    (NTAPI *PFNKEINITIALIZEAFFINITYEX)(PKAFFINITY_EX pAffinity);
NTSYSAPI VOID     NTAPI KeAddProcessorAffinityEx(PKAFFINITY_EX pAffinity, KEPROCESSORINDEX idxProcessor);
typedef  VOID    (NTAPI *PFNKEADDPROCESSORAFFINITYEX)(PKAFFINITY_EX pAffinity, KEPROCESSORINDEX idxProcessor);
NTSYSAPI VOID     NTAPI KeRemoveProcessorAffinityEx(PKAFFINITY_EX pAffinity, KEPROCESSORINDEX idxProcessor);
typedef  VOID    (NTAPI *PFNKEREMOVEPROCESSORAFFINITYEX)(PKAFFINITY_EX pAffinity, KEPROCESSORINDEX idxProcessor);
NTSYSAPI BOOLEAN  NTAPI KeInterlockedSetProcessorAffinityEx(PKAFFINITY_EX pAffinity, KEPROCESSORINDEX idxProcessor);
typedef  BOOLEAN (NTAPI *PFNKEINTERLOCKEDSETPROCESSORAFFINITYEX)(PKAFFINITY_EX pAffinity, KEPROCESSORINDEX idxProcessor);
NTSYSAPI BOOLEAN  NTAPI KeInterlockedClearProcessorAffinityEx(PKAFFINITY_EX pAffinity, KEPROCESSORINDEX idxProcessor);
typedef  BOOLEAN (NTAPI *PFNKEINTERLOCKEDCLEARPROCESSORAFFINITYEX)(PKAFFINITY_EX pAffinity, KEPROCESSORINDEX idxProcessor);
NTSYSAPI BOOLEAN  NTAPI KeCheckProcessorAffinityEx(PCKAFFINITY_EX pAffinity, KEPROCESSORINDEX idxProcessor);
typedef  BOOLEAN (NTAPI *PFNKECHECKPROCESSORAFFINITYEX)(PCKAFFINITY_EX pAffinity, KEPROCESSORINDEX idxProcessor);
NTSYSAPI VOID     NTAPI KeCopyAffinityEx(PKAFFINITY_EX pDst, PCKAFFINITY_EX pSrc);
typedef  VOID    (NTAPI *PFNKECOPYAFFINITYEX)(PKAFFINITY_EX pDst, PCKAFFINITY_EX pSrc);
NTSYSAPI VOID     NTAPI KeComplementAffinityEx(PKAFFINITY_EX pResult, PCKAFFINITY_EX pIn);
typedef  VOID    (NTAPI *PFNKECOMPLEMENTAFFINITYEX)(PKAFFINITY_EX pResult, PCKAFFINITY_EX pIn);
NTSYSAPI BOOLEAN  NTAPI KeAndAffinityEx(PCKAFFINITY_EX pIn1, PCKAFFINITY_EX pIn2, PKAFFINITY_EX pResult OPTIONAL);
typedef  BOOLEAN (NTAPI *PFNKEANDAFFINITYEX)(PCKAFFINITY_EX pIn1, PCKAFFINITY_EX pIn2, PKAFFINITY_EX pResult OPTIONAL);
NTSYSAPI BOOLEAN  NTAPI KeOrAffinityEx(PCKAFFINITY_EX pIn1, PCKAFFINITY_EX pIn2, PKAFFINITY_EX pResult OPTIONAL);
typedef  BOOLEAN (NTAPI *PFNKEORAFFINITYEX)(PCKAFFINITY_EX pIn1, PCKAFFINITY_EX pIn2, PKAFFINITY_EX pResult OPTIONAL);
/** Works like anding the complemented subtrahend with the minuend. */
NTSYSAPI BOOLEAN  NTAPI KeSubtractAffinityEx(PCKAFFINITY_EX pMinuend, PCKAFFINITY_EX pSubtrahend, PKAFFINITY_EX pResult OPTIONAL);
typedef  BOOLEAN (NTAPI *PFNKESUBTRACTAFFINITYEX)(PCKAFFINITY_EX pMinuend, PCKAFFINITY_EX pSubtrahend, PKAFFINITY_EX pResult OPTIONAL);
NTSYSAPI BOOLEAN  NTAPI KeIsEqualAffinityEx(PCKAFFINITY_EX pLeft, PCKAFFINITY_EX pRight);
typedef  BOOLEAN (NTAPI *PFNKEISEQUALAFFINITYEX)(PCKAFFINITY_EX pLeft, PCKAFFINITY_EX pRight);
NTSYSAPI BOOLEAN  NTAPI KeIsEmptyAffinityEx(PCKAFFINITY_EX pAffinity);
typedef  BOOLEAN (NTAPI *PFNKEISEMPTYAFFINITYEX)(PCKAFFINITY_EX pAffinity);
NTSYSAPI BOOLEAN  NTAPI KeIsSubsetAffinityEx(PCKAFFINITY_EX pSubset, PCKAFFINITY_EX pSuperSet);
typedef  BOOLEAN (NTAPI *PFNKEISSUBSETAFFINITYEX)(PCKAFFINITY_EX pSubset, PCKAFFINITY_EX pSuperSet);
NTSYSAPI ULONG    NTAPI KeCountSetBitsAffinityEx(PCKAFFINITY_EX pAffinity);
typedef  ULONG   (NTAPI *PFNKECOUNTSETAFFINITYEX)(PCKAFFINITY_EX pAffinity);
NTSYSAPI KEPROCESSORINDEX  NTAPI KeFindFirstSetLeftAffinityEx(PCKAFFINITY_EX pAffinity);
typedef  KEPROCESSORINDEX (NTAPI *PFNKEFINDFIRSTSETLEFTAFFINITYEX)(PCKAFFINITY_EX pAffinity);
typedef  NTSTATUS (NTAPI *PFNKEGETPROCESSORNUMBERFROMINDEX)(KEPROCESSORINDEX idxProcessor, PPROCESSOR_NUMBER pProcNumber);
typedef  KEPROCESSORINDEX (NTAPI *PFNKEGETPROCESSORINDEXFROMNUMBER)(const PROCESSOR_NUMBER *pProcNumber);
typedef  NTSTATUS (NTAPI *PFNKEGETPROCESSORNUMBERFROMINDEX)(KEPROCESSORINDEX ProcIndex, PROCESSOR_NUMBER *pProcNumber);
typedef  KEPROCESSORINDEX (NTAPI *PFNKEGETCURRENTPROCESSORNUMBEREX)(const PROCESSOR_NUMBER *pProcNumber);
typedef  KAFFINITY (NTAPI *PFNKEQUERYACTIVEPROCESSORS)(VOID);
typedef  ULONG   (NTAPI *PFNKEQUERYMAXIMUMPROCESSORCOUNT)(VOID);
typedef  ULONG   (NTAPI *PFNKEQUERYMAXIMUMPROCESSORCOUNTEX)(USHORT GroupNumber);
typedef  USHORT  (NTAPI *PFNKEQUERYMAXIMUMGROUPCOUNT)(VOID);
typedef  ULONG   (NTAPI *PFNKEQUERYACTIVEPROCESSORCOUNT)(KAFFINITY *pfActiveProcessors);
typedef  ULONG   (NTAPI *PFNKEQUERYACTIVEPROCESSORCOUNTEX)(USHORT GroupNumber);
typedef  NTSTATUS (NTAPI *PFNKEQUERYLOGICALPROCESSORRELATIONSHIP)(PROCESSOR_NUMBER *pProcNumber,
                                                                  LOGICAL_PROCESSOR_RELATIONSHIP RelationShipType,
                                                                  SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *pInfo, PULONG pcbInfo);
typedef  PVOID   (NTAPI *PFNKEREGISTERPROCESSORCHANGECALLBACK)(PPROCESSOR_CALLBACK_FUNCTION pfnCallback, void *pvUser, ULONG fFlags);
typedef  VOID    (NTAPI *PFNKEDEREGISTERPROCESSORCHANGECALLBACK)(PVOID pvCallback);
typedef  NTSTATUS (NTAPI *PFNKESETTARGETPROCESSORDPCEX)(KDPC *pDpc, PROCESSOR_NUMBER *pProcNumber);

NTSYSAPI BOOLEAN  NTAPI ObFindHandleForObject(PEPROCESS pProcess, PVOID pvObject, POBJECT_TYPE pObjectType,
                                              PVOID pvOptionalConditions, PHANDLE phFound);
NTSYSAPI NTSTATUS NTAPI ObReferenceObjectByName(PUNICODE_STRING pObjectPath, ULONG fAttributes, PACCESS_STATE pAccessState,
                                                ACCESS_MASK fDesiredAccess, POBJECT_TYPE pObjectType,
                                                KPROCESSOR_MODE enmAccessMode, PVOID pvParseContext, PVOID *ppvObject);
NTSYSAPI HANDLE   NTAPI PsGetProcessInheritedFromUniqueProcessId(PEPROCESS);
NTSYSAPI UCHAR *  NTAPI PsGetProcessImageFileName(PEPROCESS);
NTSYSAPI BOOLEAN  NTAPI PsIsProcessBeingDebugged(PEPROCESS);
NTSYSAPI ULONG    NTAPI PsGetProcessSessionId(PEPROCESS);
extern DECLIMPORT(POBJECT_TYPE *) LpcPortObjectType;            /**< In vista+ this is the ALPC port object type. */
extern DECLIMPORT(POBJECT_TYPE *) LpcWaitablePortObjectType;    /**< In vista+ this is the ALPC port object type. */

typedef VOID (NTAPI *PFNHALREQUESTIPI_PRE_W7)(KAFFINITY TargetSet);
typedef VOID (NTAPI *PFNHALREQUESTIPI_W7PLUS)(ULONG uUsuallyZero, PCKAFFINITY_EX pTargetSet);

RT_C_DECLS_END
/** @ */
#endif /* IN_RING0 */


#if defined(IN_RING3) || defined(DOXYGEN_RUNNING)
/** @name NT Userland APIs
 * @{ */
RT_C_DECLS_BEGIN

#if 0 /** @todo figure this out some time... */
typedef struct CSR_MSG_DATA_CREATED_PROCESS
{
    HANDLE hProcess;
    HANDLE hThread;
    CLIENT_ID
    DWORD idProcess;
    DWORD idThread;
    DWORD fCreate;

} CSR_MSG_DATA_CREATED_PROCESS;

#define CSR_MSG_NO_CREATED_PROCESS    UINT32_C(0x10000)
#define CSR_MSG_NO_CREATED_THREAD     UINT32_C(0x10001)
NTSYSAPI NTSTATUS NTAPI CsrClientCallServer(PVOID, PVOID, ULONG, SIZE_T);
#endif

NTSYSAPI VOID NTAPI     LdrInitializeThunk(PVOID, PVOID, PVOID);

typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA
{
    ULONG               Flags;
    PCUNICODE_STRING    FullDllName;
    PCUNICODE_STRING    BaseDllName;
    PVOID               DllBase;
    ULONG               SizeOfImage;
} LDR_DLL_LOADED_NOTIFICATION_DATA, LDR_DLL_UNLOADED_NOTIFICATION_DATA;
typedef LDR_DLL_LOADED_NOTIFICATION_DATA *PLDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_UNLOADED_NOTIFICATION_DATA;
typedef LDR_DLL_LOADED_NOTIFICATION_DATA const *PCLDR_DLL_LOADED_NOTIFICATION_DATA, *PCLDR_DLL_UNLOADED_NOTIFICATION_DATA;

typedef union _LDR_DLL_NOTIFICATION_DATA
{
    LDR_DLL_LOADED_NOTIFICATION_DATA    Loaded;
    LDR_DLL_UNLOADED_NOTIFICATION_DATA  Unloaded;
} LDR_DLL_NOTIFICATION_DATA;
typedef LDR_DLL_NOTIFICATION_DATA *PLDR_DLL_NOTIFICATION_DATA;
typedef LDR_DLL_NOTIFICATION_DATA const *PCLDR_DLL_NOTIFICATION_DATA;

typedef VOID (NTAPI *PLDR_DLL_NOTIFICATION_FUNCTION)(ULONG ulReason, PCLDR_DLL_NOTIFICATION_DATA pData, PVOID pvUser);

#define LDR_DLL_NOTIFICATION_REASON_LOADED      UINT32_C(1)
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED    UINT32_C(2)
NTSYSAPI NTSTATUS NTAPI LdrRegisterDllNotification(ULONG fFlags, PLDR_DLL_NOTIFICATION_FUNCTION pfnCallback, PVOID pvUser,
                                                   PVOID *pvCookie);
typedef NTSTATUS (NTAPI *PFNLDRREGISTERDLLNOTIFICATION)(ULONG, PLDR_DLL_NOTIFICATION_FUNCTION, PVOID, PVOID *);
NTSYSAPI NTSTATUS NTAPI LdrUnregisterDllNotification(PVOID pvCookie);
typedef NTSTATUS (NTAPI *PFNLDRUNREGISTERDLLNOTIFICATION)(PVOID);

NTSYSAPI NTSTATUS NTAPI LdrLoadDll(IN PWSTR pwszSearchPathOrFlags OPTIONAL, IN PULONG pfFlags OPTIONAL,
                                   IN PCUNICODE_STRING pName, OUT PHANDLE phMod);
typedef NTSTATUS (NTAPI *PFNLDRLOADDLL)(IN PWSTR pwszSearchPathOrFlags OPTIONAL, IN PULONG pfFlags OPTIONAL,
                                        IN PCUNICODE_STRING pName, OUT PHANDLE phMod);
NTSYSAPI NTSTATUS NTAPI LdrUnloadDll(IN HANDLE hMod);
typedef NTSTATUS (NTAPI *PFNLDRUNLOADDLL)(IN HANDLE hMod);
NTSYSAPI NTSTATUS NTAPI LdrGetDllHandle(IN PCWSTR pwszDllPath OPTIONAL, IN PULONG pfFlags OPTIONAL,
                                        IN PCUNICODE_STRING pName, OUT PHANDLE phDll);
typedef NTSTATUS (NTAPI *PFNLDRGETDLLHANDLE)(IN PCWSTR pwszDllPath OPTIONAL, IN PULONG pfFlags OPTIONAL,
                                             IN PCUNICODE_STRING pName, OUT PHANDLE phDll);
#define LDRGETDLLHANDLEEX_F_UNCHANGED_REFCOUNT  RT_BIT_32(0)
#define LDRGETDLLHANDLEEX_F_PIN                 RT_BIT_32(1)
/** @since Windows XP. */
NTSYSAPI NTSTATUS NTAPI LdrGetDllHandleEx(IN ULONG fFlags, IN PCWSTR pwszDllPath OPTIONAL, IN PULONG pfFlags OPTIONAL,
                                          IN PCUNICODE_STRING pName, OUT PHANDLE phDll);
/** @since Windows XP. */
typedef NTSTATUS (NTAPI *PFNLDRGETDLLHANDLEEX)(IN ULONG fFlags, IN PCWSTR pwszDllPath OPTIONAL, IN PULONG pfFlags OPTIONAL,
                                               IN PCUNICODE_STRING pName, OUT PHANDLE phDll);
/** @since Windows 7. */
NTSYSAPI NTSTATUS NTAPI LdrGetDllHandleByMapping(IN PVOID pvBase, OUT PHANDLE phDll);
/** @since Windows 7. */
typedef NTSTATUS (NTAPI *PFNLDRGETDLLHANDLEBYMAPPING)(IN PVOID pvBase, OUT PHANDLE phDll);
/** @since Windows 7. */
NTSYSAPI NTSTATUS NTAPI LdrGetDllHandleByName(IN PCUNICODE_STRING pName OPTIONAL, IN PCUNICODE_STRING pFullName OPTIONAL,
                                              OUT PHANDLE phDll);
/** @since Windows 7. */
typedef NTSTATUS (NTAPI *PFNLDRGETDLLHANDLEBYNAME)(IN PCUNICODE_STRING pName OPTIONAL, IN PCUNICODE_STRING pFullName OPTIONAL,
                                                   OUT PHANDLE phDll);
#define LDRADDREFDLL_F_PIN                      RT_BIT_32(0)
NTSYSAPI NTSTATUS NTAPI LdrAddRefDll(IN ULONG fFlags, IN HANDLE hDll);
typedef NTSTATUS (NTAPI *PFNLDRADDREFDLL)(IN ULONG fFlags, IN HANDLE hDll);
NTSYSAPI NTSTATUS NTAPI LdrGetProcedureAddress(IN HANDLE hDll, IN ANSI_STRING const *pSymbol OPTIONAL,
                                               IN ULONG uOrdinal OPTIONAL, OUT PVOID *ppvSymbol);
typedef NTSTATUS (NTAPI *PFNLDRGETPROCEDUREADDRESS)(IN HANDLE hDll, IN PCANSI_STRING pSymbol OPTIONAL,
                                                    IN ULONG uOrdinal OPTIONAL, OUT PVOID *ppvSymbol);
#define LDRGETPROCEDUREADDRESSEX_F_DONT_RECORD_FORWARDER RT_BIT_32(0)
/** @since Windows Vista. */
NTSYSAPI NTSTATUS NTAPI LdrGetProcedureAddressEx(IN HANDLE hDll, IN ANSI_STRING const *pSymbol OPTIONAL,
                                                 IN ULONG uOrdinal OPTIONAL, OUT PVOID *ppvSymbol, ULONG fFlags);
/** @since Windows Vista. */
typedef NTSTATUS (NTAPI *PFNLDRGETPROCEDUREADDRESSEX)(IN HANDLE hDll, IN ANSI_STRING const *pSymbol OPTIONAL,
                                                      IN ULONG uOrdinal OPTIONAL, OUT PVOID *ppvSymbol, ULONG fFlags);
#define LDRLOCKLOADERLOCK_F_RAISE_ERRORS    RT_BIT_32(0)
#define LDRLOCKLOADERLOCK_F_NO_WAIT         RT_BIT_32(1)
#define LDRLOCKLOADERLOCK_DISP_INVALID      UINT32_C(0)
#define LDRLOCKLOADERLOCK_DISP_ACQUIRED     UINT32_C(1)
#define LDRLOCKLOADERLOCK_DISP_NOT_ACQUIRED UINT32_C(2)
/** @since Windows XP. */
NTSYSAPI NTSTATUS NTAPI LdrLockLoaderLock(IN ULONG fFlags, OUT PULONG puDisposition OPTIONAL, OUT PVOID *ppvCookie);
/** @since Windows XP. */
typedef NTSTATUS (NTAPI *PFNLDRLOCKLOADERLOCK)(IN ULONG fFlags, OUT PULONG puDisposition OPTIONAL, OUT PVOID *ppvCookie);
#define LDRUNLOCKLOADERLOCK_F_RAISE_ERRORS  RT_BIT_32(0)
/** @since Windows XP. */
NTSYSAPI NTSTATUS NTAPI LdrUnlockLoaderLock(IN ULONG fFlags, OUT PVOID pvCookie);
/** @since Windows XP. */
typedef NTSTATUS (NTAPI *PFNLDRUNLOCKLOADERLOCK)(IN ULONG fFlags, OUT PVOID pvCookie);

NTSYSAPI NTSTATUS NTAPI RtlExpandEnvironmentStrings_U(PVOID, PUNICODE_STRING, PUNICODE_STRING, PULONG);
NTSYSAPI VOID NTAPI     RtlExitUserProcess(NTSTATUS rcExitCode); /**< Vista and later. */
NTSYSAPI VOID NTAPI     RtlExitUserThread(NTSTATUS rcExitCode);
NTSYSAPI NTSTATUS NTAPI RtlDosApplyFileIsolationRedirection_Ustr(IN ULONG fFlags,
                                                                 IN PCUNICODE_STRING pOrgName,
                                                                 IN PUNICODE_STRING pDefaultSuffix,
                                                                 IN OUT PUNICODE_STRING pStaticString,
                                                                 IN OUT PUNICODE_STRING pDynamicString,
                                                                 IN OUT PUNICODE_STRING *ppResultString,
                                                                 IN PULONG pfNewFlags OPTIONAL,
                                                                 IN PSIZE_T pcbFilename OPTIONAL,
                                                                 IN PSIZE_T pcbNeeded OPTIONAL);
/** @since Windows 8.
 * @note Status code is always zero in windows 10 build 14393. */
NTSYSAPI NTSTATUS NTAPI ApiSetQueryApiSetPresence(IN PCUNICODE_STRING pAllegedApiSetDll, OUT PBOOLEAN pfPresent);
/** @copydoc ApiSetQueryApiSetPresence */
typedef NTSTATUS (NTAPI *PFNAPISETQUERYAPISETPRESENCE)(IN PCUNICODE_STRING pAllegedApiSetDll, OUT PBOOLEAN pfPresent);


# ifdef IPRT_NT_USE_WINTERNL
typedef NTSTATUS NTAPI RTL_HEAP_COMMIT_ROUTINE(PVOID, PVOID *, PSIZE_T);
typedef RTL_HEAP_COMMIT_ROUTINE *PRTL_HEAP_COMMIT_ROUTINE;
typedef struct _RTL_HEAP_PARAMETERS
{
    ULONG   Length;
    SIZE_T  SegmentReserve;
    SIZE_T  SegmentCommit;
    SIZE_T  DeCommitFreeBlockThreshold;
    SIZE_T  DeCommitTotalFreeThreshold;
    SIZE_T  MaximumAllocationSize;
    SIZE_T  VirtualMemoryThreshold;
    SIZE_T  InitialCommit;
    SIZE_T  InitialReserve;
    PRTL_HEAP_COMMIT_ROUTINE  CommitRoutine;
    SIZE_T  Reserved[2];
} RTL_HEAP_PARAMETERS;
typedef RTL_HEAP_PARAMETERS *PRTL_HEAP_PARAMETERS;
NTSYSAPI PVOID NTAPI RtlCreateHeap(ULONG fFlags, PVOID pvHeapBase, SIZE_T cbReserve, SIZE_T cbCommit, PVOID pvLock,
                                   PRTL_HEAP_PARAMETERS pParameters);
/** @name Heap flags (for RtlCreateHeap).
 * @{ */
/*#  define HEAP_NO_SERIALIZE             UINT32_C(0x00000001)
#  define HEAP_GROWABLE                 UINT32_C(0x00000002)
#  define HEAP_GENERATE_EXCEPTIONS      UINT32_C(0x00000004)
#  define HEAP_ZERO_MEMORY              UINT32_C(0x00000008)
#  define HEAP_REALLOC_IN_PLACE_ONLY    UINT32_C(0x00000010)
#  define HEAP_TAIL_CHECKING_ENABLED    UINT32_C(0x00000020)
#  define HEAP_FREE_CHECKING_ENABLED    UINT32_C(0x00000040)
#  define HEAP_DISABLE_COALESCE_ON_FREE UINT32_C(0x00000080)*/
#  define HEAP_SETTABLE_USER_VALUE      UINT32_C(0x00000100)
#  define HEAP_SETTABLE_USER_FLAG1      UINT32_C(0x00000200)
#  define HEAP_SETTABLE_USER_FLAG2      UINT32_C(0x00000400)
#  define HEAP_SETTABLE_USER_FLAG3      UINT32_C(0x00000800)
#  define HEAP_SETTABLE_USER_FLAGS      UINT32_C(0x00000e00)
#  define HEAP_CLASS_0                  UINT32_C(0x00000000)
#  define HEAP_CLASS_1                  UINT32_C(0x00001000)
#  define HEAP_CLASS_2                  UINT32_C(0x00002000)
#  define HEAP_CLASS_3                  UINT32_C(0x00003000)
#  define HEAP_CLASS_4                  UINT32_C(0x00004000)
#  define HEAP_CLASS_5                  UINT32_C(0x00005000)
#  define HEAP_CLASS_6                  UINT32_C(0x00006000)
#  define HEAP_CLASS_7                  UINT32_C(0x00007000)
#  define HEAP_CLASS_8                  UINT32_C(0x00008000)
#  define HEAP_CLASS_MASK               UINT32_C(0x0000f000)
# endif
# define HEAP_CLASS_PROCESS             HEAP_CLASS_0
# define HEAP_CLASS_PRIVATE             HEAP_CLASS_1
# define HEAP_CLASS_KERNEL              HEAP_CLASS_2
# define HEAP_CLASS_GDI                 HEAP_CLASS_3
# define HEAP_CLASS_USER                HEAP_CLASS_4
# define HEAP_CLASS_CONSOLE             HEAP_CLASS_5
# define HEAP_CLASS_USER_DESKTOP        HEAP_CLASS_6
# define HEAP_CLASS_CSRSS_SHARED        HEAP_CLASS_7
# define HEAP_CLASS_CSRSS_PORT          HEAP_CLASS_8
# ifdef IPRT_NT_USE_WINTERNL
/*#  define HEAP_CREATE_ALIGN_16          UINT32_C(0x00010000)
#  define HEAP_CREATE_ENABLE_TRACING    UINT32_C(0x00020000)
#  define HEAP_CREATE_ENABLE_EXECUTE    UINT32_C(0x00040000)*/
#  define HEAP_CREATE_VALID_MASK        UINT32_C(0x0007f0ff)
# endif /* IPRT_NT_USE_WINTERNL */
/** @} */
# ifdef IPRT_NT_USE_WINTERNL
/** @name Heap tagging constants
 * @{ */
#  define HEAP_GLOBAL_TAG               UINT32_C(0x00000800)
/*#  define HEAP_MAXIMUM_TAG              UINT32_C(0x00000fff)
#  define HEAP_PSEUDO_TAG_FLAG          UINT32_C(0x00008000)
#  define HEAP_TAG_SHIFT                18 */
#  define HEAP_TAG_MASK                 (HEAP_MAXIMUM_TAG << HEAP_TAG_SHIFT)
/** @}  */
NTSYSAPI PVOID NTAPI    RtlAllocateHeap(HANDLE hHeap, ULONG fFlags, SIZE_T cb);
NTSYSAPI PVOID NTAPI    RtlReAllocateHeap(HANDLE hHeap, ULONG fFlags, PVOID pvOld, SIZE_T cbNew);
NTSYSAPI BOOLEAN NTAPI  RtlFreeHeap(HANDLE hHeap, ULONG fFlags, PVOID pvMem);
# endif /* IPRT_NT_USE_WINTERNL */
NTSYSAPI SIZE_T NTAPI   RtlCompactHeap(HANDLE hHeap, ULONG fFlags);
NTSYSAPI VOID NTAPI     RtlFreeUnicodeString(PUNICODE_STRING);
NTSYSAPI SIZE_T NTAPI   RtlSizeHeap(HANDLE hHeap, ULONG fFlags, PVOID pvMem);
NTSYSAPI NTSTATUS NTAPI RtlGetLastNtStatus(VOID);
NTSYSAPI ULONG NTAPI    RtlGetLastWin32Error(VOID);
NTSYSAPI VOID NTAPI     RtlSetLastWin32Error(ULONG uError);
NTSYSAPI VOID NTAPI     RtlSetLastWin32ErrorAndNtStatusFromNtStatus(NTSTATUS rcNt);
NTSYSAPI VOID NTAPI     RtlRestoreLastWin32Error(ULONG uError);
NTSYSAPI BOOLEAN NTAPI  RtlQueryPerformanceCounter(PLARGE_INTEGER);
NTSYSAPI uint64_t NTAPI RtlGetSystemTimePrecise(VOID);
typedef uint64_t (NTAPI * PFNRTLGETSYSTEMTIMEPRECISE)(VOID);

RT_C_DECLS_END
/** @} */
#endif /* IN_RING3 */

#endif

