#include "syscalls32.gen.hpp"

#define FDP_MODULE "syscalls32"
#include "log.hpp"
#include "core.hpp"

#include <cstring>

namespace
{
    constexpr bool g_debug = false;

    constexpr wow64::syscalls32::callcfgs_t g_callcfgs =
    {{
        {"_NtAcceptConnectPort@24", 6, {{"PHANDLE", "PortHandle", sizeof(wow64::PHANDLE)}, {"PVOID", "PortContext", sizeof(wow64::PVOID)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(wow64::PPORT_MESSAGE)}, {"BOOLEAN", "AcceptConnection", sizeof(wow64::BOOLEAN)}, {"PPORT_VIEW", "ServerView", sizeof(wow64::PPORT_VIEW)}, {"PREMOTE_PORT_VIEW", "ClientView", sizeof(wow64::PREMOTE_PORT_VIEW)}}},
        {"_NtAccessCheck@32", 8, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(wow64::PSECURITY_DESCRIPTOR)}, {"HANDLE", "ClientToken", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(wow64::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(wow64::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(wow64::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(wow64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(wow64::PNTSTATUS)}}},
        {"_NtAccessCheckByType@44", 11, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(wow64::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(wow64::PSID)}, {"HANDLE", "ClientToken", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(wow64::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(wow64::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(wow64::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(wow64::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(wow64::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(wow64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(wow64::PNTSTATUS)}}},
        {"_NtAccessCheckByTypeAndAuditAlarm@64", 16, {{"PUNICODE_STRING", "SubsystemName", sizeof(wow64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(wow64::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(wow64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(wow64::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(wow64::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(wow64::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(wow64::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(wow64::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(wow64::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(wow64::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(wow64::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(wow64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(wow64::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(wow64::PBOOLEAN)}}},
        {"_NtAccessCheckByTypeResultList@44", 11, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(wow64::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(wow64::PSID)}, {"HANDLE", "ClientToken", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(wow64::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(wow64::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(wow64::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(wow64::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(wow64::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(wow64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(wow64::PNTSTATUS)}}},
        {"_NtAccessCheckByTypeResultListAndAuditAlarm@64", 16, {{"PUNICODE_STRING", "SubsystemName", sizeof(wow64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(wow64::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(wow64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(wow64::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(wow64::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(wow64::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(wow64::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(wow64::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(wow64::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(wow64::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(wow64::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(wow64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(wow64::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(wow64::PBOOLEAN)}}},
        {"_NtAddAtom@12", 3, {{"PWSTR", "AtomName", sizeof(wow64::PWSTR)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PRTL_ATOM", "Atom", sizeof(wow64::PRTL_ATOM)}}},
        {"_NtAddDriverEntry@8", 2, {{"PEFI_DRIVER_ENTRY", "DriverEntry", sizeof(wow64::PEFI_DRIVER_ENTRY)}, {"PULONG", "Id", sizeof(wow64::PULONG)}}},
        {"_NtAlertResumeThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(wow64::PULONG)}}},
        {"_NtAlertThread@4", 1, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}}},
        {"_NtAllocateReserveObject@12", 3, {{"PHANDLE", "MemoryReserveHandle", sizeof(wow64::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"MEMORY_RESERVE_TYPE", "Type", sizeof(wow64::MEMORY_RESERVE_TYPE)}}},
        {"_NtAllocateUserPhysicalPages@12", 3, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PULONG_PTR", "NumberOfPages", sizeof(wow64::PULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(wow64::PULONG_PTR)}}},
        {"_NtAllocateUuids@16", 4, {{"PULARGE_INTEGER", "Time", sizeof(wow64::PULARGE_INTEGER)}, {"PULONG", "Range", sizeof(wow64::PULONG)}, {"PULONG", "Sequence", sizeof(wow64::PULONG)}, {"PCHAR", "Seed", sizeof(wow64::PCHAR)}}},
        {"_NtAllocateVirtualMemory@24", 6, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(wow64::PVOID)}, {"ULONG_PTR", "ZeroBits", sizeof(wow64::ULONG_PTR)}, {"PSIZE_T", "RegionSize", sizeof(wow64::PSIZE_T)}, {"ULONG", "AllocationType", sizeof(wow64::ULONG)}, {"ULONG", "Protect", sizeof(wow64::ULONG)}}},
        {"_NtAlpcAcceptConnectPort@36", 9, {{"PHANDLE", "PortHandle", sizeof(wow64::PHANDLE)}, {"HANDLE", "ConnectionPortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(wow64::PALPC_PORT_ATTRIBUTES)}, {"PVOID", "PortContext", sizeof(wow64::PVOID)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(wow64::PPORT_MESSAGE)}, {"PALPC_MESSAGE_ATTRIBUTES", "ConnectionMessageAttributes", sizeof(wow64::PALPC_MESSAGE_ATTRIBUTES)}, {"BOOLEAN", "AcceptConnection", sizeof(wow64::BOOLEAN)}}},
        {"_NtAlpcCreatePortSection@24", 6, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"HANDLE", "SectionHandle", sizeof(wow64::HANDLE)}, {"SIZE_T", "SectionSize", sizeof(wow64::SIZE_T)}, {"PALPC_HANDLE", "AlpcSectionHandle", sizeof(wow64::PALPC_HANDLE)}, {"PSIZE_T", "ActualSectionSize", sizeof(wow64::PSIZE_T)}}},
        {"_NtAlpcDeleteResourceReserve@12", 3, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"ALPC_HANDLE", "ResourceId", sizeof(wow64::ALPC_HANDLE)}}},
        {"_NtAlpcDeleteSectionView@12", 3, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PVOID", "ViewBase", sizeof(wow64::PVOID)}}},
        {"_NtAlpcDeleteSecurityContext@12", 3, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"ALPC_HANDLE", "ContextHandle", sizeof(wow64::ALPC_HANDLE)}}},
        {"_NtAlpcDisconnectPort@8", 2, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}}},
        {"_NtAlpcRevokeSecurityContext@12", 3, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"ALPC_HANDLE", "ContextHandle", sizeof(wow64::ALPC_HANDLE)}}},
        {"_NtAlpcSendWaitReceivePort@32", 8, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PPORT_MESSAGE", "SendMessage", sizeof(wow64::PPORT_MESSAGE)}, {"PALPC_MESSAGE_ATTRIBUTES", "SendMessageAttributes", sizeof(wow64::PALPC_MESSAGE_ATTRIBUTES)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(wow64::PPORT_MESSAGE)}, {"PULONG", "BufferLength", sizeof(wow64::PULONG)}, {"PALPC_MESSAGE_ATTRIBUTES", "ReceiveMessageAttributes", sizeof(wow64::PALPC_MESSAGE_ATTRIBUTES)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtAlpcSetInformation@16", 4, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ALPC_PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(wow64::ALPC_PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}}},
        {"_NtApphelpCacheControl@8", 2, {{"APPHELPCOMMAND", "type", sizeof(wow64::APPHELPCOMMAND)}, {"PVOID", "buf", sizeof(wow64::PVOID)}}},
        {"_NtCancelIoFileEx@12", 3, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoRequestToCancel", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}}},
        {"_NtCancelSynchronousIoFile@12", 3, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoRequestToCancel", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}}},
        {"_NtClearEvent@4", 1, {{"HANDLE", "EventHandle", sizeof(wow64::HANDLE)}}},
        {"_NtClose@4", 1, {{"HANDLE", "Handle", sizeof(wow64::HANDLE)}}},
        {"_NtCommitEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtCommitTransaction@8", 2, {{"HANDLE", "TransactionHandle", sizeof(wow64::HANDLE)}, {"BOOLEAN", "Wait", sizeof(wow64::BOOLEAN)}}},
        {"_NtCompactKeys@8", 2, {{"ULONG", "Count", sizeof(wow64::ULONG)}, {"HANDLE", "KeyArray", sizeof(wow64::HANDLE)}}},
        {"_NtCompleteConnectPort@4", 1, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}}},
        {"_NtConnectPort@32", 8, {{"PHANDLE", "PortHandle", sizeof(wow64::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(wow64::PUNICODE_STRING)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(wow64::PSECURITY_QUALITY_OF_SERVICE)}, {"PPORT_VIEW", "ClientView", sizeof(wow64::PPORT_VIEW)}, {"PREMOTE_PORT_VIEW", "ServerView", sizeof(wow64::PREMOTE_PORT_VIEW)}, {"PULONG", "MaxMessageLength", sizeof(wow64::PULONG)}, {"PVOID", "ConnectionInformation", sizeof(wow64::PVOID)}, {"PULONG", "ConnectionInformationLength", sizeof(wow64::PULONG)}}},
        {"_NtCreateEvent@20", 5, {{"PHANDLE", "EventHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"EVENT_TYPE", "EventType", sizeof(wow64::EVENT_TYPE)}, {"BOOLEAN", "InitialState", sizeof(wow64::BOOLEAN)}}},
        {"_NtCreateEventPair@12", 3, {{"PHANDLE", "EventPairHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtCreateFile@44", 11, {{"PHANDLE", "FileHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "AllocationSize", sizeof(wow64::PLARGE_INTEGER)}, {"ULONG", "FileAttributes", sizeof(wow64::ULONG)}, {"ULONG", "ShareAccess", sizeof(wow64::ULONG)}, {"ULONG", "CreateDisposition", sizeof(wow64::ULONG)}, {"ULONG", "CreateOptions", sizeof(wow64::ULONG)}, {"PVOID", "EaBuffer", sizeof(wow64::PVOID)}, {"ULONG", "EaLength", sizeof(wow64::ULONG)}}},
        {"_NtCreateIoCompletion@16", 4, {{"PHANDLE", "IoCompletionHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "Count", sizeof(wow64::ULONG)}}},
        {"_NtCreateJobSet@12", 3, {{"ULONG", "NumJob", sizeof(wow64::ULONG)}, {"PJOB_SET_ARRAY", "UserJobSet", sizeof(wow64::PJOB_SET_ARRAY)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}}},
        {"_NtCreateKeyTransacted@32", 8, {{"PHANDLE", "KeyHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "TitleIndex", sizeof(wow64::ULONG)}, {"PUNICODE_STRING", "Class", sizeof(wow64::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(wow64::ULONG)}, {"HANDLE", "TransactionHandle", sizeof(wow64::HANDLE)}, {"PULONG", "Disposition", sizeof(wow64::PULONG)}}},
        {"_NtCreatePagingFile@16", 4, {{"PUNICODE_STRING", "PageFileName", sizeof(wow64::PUNICODE_STRING)}, {"PLARGE_INTEGER", "MinimumSize", sizeof(wow64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "MaximumSize", sizeof(wow64::PLARGE_INTEGER)}, {"ULONG", "Priority", sizeof(wow64::ULONG)}}},
        {"_NtCreatePrivateNamespace@16", 4, {{"PHANDLE", "NamespaceHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PVOID", "BoundaryDescriptor", sizeof(wow64::PVOID)}}},
        {"_NtCreateProfileEx@40", 10, {{"PHANDLE", "ProfileHandle", sizeof(wow64::PHANDLE)}, {"HANDLE", "Process", sizeof(wow64::HANDLE)}, {"PVOID", "ProfileBase", sizeof(wow64::PVOID)}, {"SIZE_T", "ProfileSize", sizeof(wow64::SIZE_T)}, {"ULONG", "BucketSize", sizeof(wow64::ULONG)}, {"PULONG", "Buffer", sizeof(wow64::PULONG)}, {"ULONG", "BufferSize", sizeof(wow64::ULONG)}, {"KPROFILE_SOURCE", "ProfileSource", sizeof(wow64::KPROFILE_SOURCE)}, {"ULONG", "GroupAffinityCount", sizeof(wow64::ULONG)}, {"PGROUP_AFFINITY", "GroupAffinity", sizeof(wow64::PGROUP_AFFINITY)}}},
        {"_NtCreateSection@28", 7, {{"PHANDLE", "SectionHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PLARGE_INTEGER", "MaximumSize", sizeof(wow64::PLARGE_INTEGER)}, {"ULONG", "SectionPageProtection", sizeof(wow64::ULONG)}, {"ULONG", "AllocationAttributes", sizeof(wow64::ULONG)}, {"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}}},
        {"_NtCreateSemaphore@20", 5, {{"PHANDLE", "SemaphoreHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"LONG", "InitialCount", sizeof(wow64::LONG)}, {"LONG", "MaximumCount", sizeof(wow64::LONG)}}},
        {"_NtCreateThread@32", 8, {{"PHANDLE", "ThreadHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PCLIENT_ID", "ClientId", sizeof(wow64::PCLIENT_ID)}, {"PCONTEXT", "ThreadContext", sizeof(wow64::PCONTEXT)}, {"PINITIAL_TEB", "InitialTeb", sizeof(wow64::PINITIAL_TEB)}, {"BOOLEAN", "CreateSuspended", sizeof(wow64::BOOLEAN)}}},
        {"_NtCreateThreadEx@44", 11, {{"PHANDLE", "ThreadHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "StartRoutine", sizeof(wow64::PVOID)}, {"PVOID", "Argument", sizeof(wow64::PVOID)}, {"ULONG", "CreateFlags", sizeof(wow64::ULONG)}, {"ULONG_PTR", "ZeroBits", sizeof(wow64::ULONG_PTR)}, {"SIZE_T", "StackSize", sizeof(wow64::SIZE_T)}, {"SIZE_T", "MaximumStackSize", sizeof(wow64::SIZE_T)}, {"PPS_ATTRIBUTE_LIST", "AttributeList", sizeof(wow64::PPS_ATTRIBUTE_LIST)}}},
        {"_NtCreateToken@52", 13, {{"PHANDLE", "TokenHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"TOKEN_TYPE", "TokenType", sizeof(wow64::TOKEN_TYPE)}, {"PLUID", "AuthenticationId", sizeof(wow64::PLUID)}, {"PLARGE_INTEGER", "ExpirationTime", sizeof(wow64::PLARGE_INTEGER)}, {"PTOKEN_USER", "User", sizeof(wow64::PTOKEN_USER)}, {"PTOKEN_GROUPS", "Groups", sizeof(wow64::PTOKEN_GROUPS)}, {"PTOKEN_PRIVILEGES", "Privileges", sizeof(wow64::PTOKEN_PRIVILEGES)}, {"PTOKEN_OWNER", "Owner", sizeof(wow64::PTOKEN_OWNER)}, {"PTOKEN_PRIMARY_GROUP", "PrimaryGroup", sizeof(wow64::PTOKEN_PRIMARY_GROUP)}, {"PTOKEN_DEFAULT_DACL", "DefaultDacl", sizeof(wow64::PTOKEN_DEFAULT_DACL)}, {"PTOKEN_SOURCE", "TokenSource", sizeof(wow64::PTOKEN_SOURCE)}}},
        {"_NtCreateTransaction@40", 10, {{"PHANDLE", "TransactionHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"LPGUID", "Uow", sizeof(wow64::LPGUID)}, {"HANDLE", "TmHandle", sizeof(wow64::HANDLE)}, {"ULONG", "CreateOptions", sizeof(wow64::ULONG)}, {"ULONG", "IsolationLevel", sizeof(wow64::ULONG)}, {"ULONG", "IsolationFlags", sizeof(wow64::ULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}, {"PUNICODE_STRING", "Description", sizeof(wow64::PUNICODE_STRING)}}},
        {"_NtCreateUserProcess@44", 11, {{"PHANDLE", "ProcessHandle", sizeof(wow64::PHANDLE)}, {"PHANDLE", "ThreadHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "ProcessDesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"ACCESS_MASK", "ThreadDesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ProcessObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "ThreadObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "ProcessFlags", sizeof(wow64::ULONG)}, {"ULONG", "ThreadFlags", sizeof(wow64::ULONG)}, {"PRTL_USER_PROCESS_PARAMETERS", "ProcessParameters", sizeof(wow64::PRTL_USER_PROCESS_PARAMETERS)}, {"PPROCESS_CREATE_INFO", "CreateInfo", sizeof(wow64::PPROCESS_CREATE_INFO)}, {"PPROCESS_ATTRIBUTE_LIST", "AttributeList", sizeof(wow64::PPROCESS_ATTRIBUTE_LIST)}}},
        {"_NtCreateWorkerFactory@40", 10, {{"PHANDLE", "WorkerFactoryHandleReturn", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"HANDLE", "CompletionPortHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "WorkerProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "StartRoutine", sizeof(wow64::PVOID)}, {"PVOID", "StartParameter", sizeof(wow64::PVOID)}, {"ULONG", "MaxThreadCount", sizeof(wow64::ULONG)}, {"SIZE_T", "StackReserve", sizeof(wow64::SIZE_T)}, {"SIZE_T", "StackCommit", sizeof(wow64::SIZE_T)}}},
        {"_NtDebugActiveProcess@8", 2, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "DebugObjectHandle", sizeof(wow64::HANDLE)}}},
        {"_NtDeleteBootEntry@4", 1, {{"ULONG", "Id", sizeof(wow64::ULONG)}}},
        {"_NtDeleteFile@4", 1, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtDeleteObjectAuditAlarm@12", 3, {{"PUNICODE_STRING", "SubsystemName", sizeof(wow64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(wow64::PVOID)}, {"BOOLEAN", "GenerateOnClose", sizeof(wow64::BOOLEAN)}}},
        {"_NtDeletePrivateNamespace@4", 1, {{"HANDLE", "NamespaceHandle", sizeof(wow64::HANDLE)}}},
        {"_NtDeleteValueKey@8", 2, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(wow64::PUNICODE_STRING)}}},
        {"_NtDisableLastKnownGood@0", 0, {}},
        {"_NtDisplayString@4", 1, {{"PUNICODE_STRING", "String", sizeof(wow64::PUNICODE_STRING)}}},
        {"_NtDuplicateToken@24", 6, {{"HANDLE", "ExistingTokenHandle", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"BOOLEAN", "EffectiveOnly", sizeof(wow64::BOOLEAN)}, {"TOKEN_TYPE", "TokenType", sizeof(wow64::TOKEN_TYPE)}, {"PHANDLE", "NewTokenHandle", sizeof(wow64::PHANDLE)}}},
        {"_NtEnableLastKnownGood@0", 0, {}},
        {"_NtEnumerateDriverEntries@8", 2, {{"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"PULONG", "BufferLength", sizeof(wow64::PULONG)}}},
        {"_NtEnumerateValueKey@24", 6, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Index", sizeof(wow64::ULONG)}, {"KEY_VALUE_INFORMATION_CLASS", "KeyValueInformationClass", sizeof(wow64::KEY_VALUE_INFORMATION_CLASS)}, {"PVOID", "KeyValueInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PULONG", "ResultLength", sizeof(wow64::PULONG)}}},
        {"_NtFilterToken@24", 6, {{"HANDLE", "ExistingTokenHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PTOKEN_GROUPS", "SidsToDisable", sizeof(wow64::PTOKEN_GROUPS)}, {"PTOKEN_PRIVILEGES", "PrivilegesToDelete", sizeof(wow64::PTOKEN_PRIVILEGES)}, {"PTOKEN_GROUPS", "RestrictedSids", sizeof(wow64::PTOKEN_GROUPS)}, {"PHANDLE", "NewTokenHandle", sizeof(wow64::PHANDLE)}}},
        {"_NtFindAtom@12", 3, {{"PWSTR", "AtomName", sizeof(wow64::PWSTR)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PRTL_ATOM", "Atom", sizeof(wow64::PRTL_ATOM)}}},
        {"_NtFlushKey@4", 1, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}}},
        {"_NtFlushWriteBuffer@0", 0, {}},
        {"_NtFreeUserPhysicalPages@12", 3, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PULONG_PTR", "NumberOfPages", sizeof(wow64::PULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(wow64::PULONG_PTR)}}},
        {"_NtFreeVirtualMemory@16", 4, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(wow64::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(wow64::PSIZE_T)}, {"ULONG", "FreeType", sizeof(wow64::ULONG)}}},
        {"_NtFreezeRegistry@4", 1, {{"ULONG", "TimeOutInSeconds", sizeof(wow64::ULONG)}}},
        {"_NtFsControlFile@40", 10, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"ULONG", "IoControlCode", sizeof(wow64::ULONG)}, {"PVOID", "InputBuffer", sizeof(wow64::PVOID)}, {"ULONG", "InputBufferLength", sizeof(wow64::ULONG)}, {"PVOID", "OutputBuffer", sizeof(wow64::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(wow64::ULONG)}}},
        {"_NtGetContextThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"PCONTEXT", "ThreadContext", sizeof(wow64::PCONTEXT)}}},
        {"_NtGetCurrentProcessorNumber@0", 0, {}},
        {"_NtGetDevicePowerState@8", 2, {{"HANDLE", "Device", sizeof(wow64::HANDLE)}, {"DEVICE_POWER_STATE", "STARState", sizeof(wow64::DEVICE_POWER_STATE)}}},
        {"_NtGetMUIRegistryInfo@12", 3, {{"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PULONG", "DataSize", sizeof(wow64::PULONG)}, {"PVOID", "Data", sizeof(wow64::PVOID)}}},
        {"_NtGetNlsSectionPtr@20", 5, {{"ULONG", "SectionType", sizeof(wow64::ULONG)}, {"ULONG", "SectionData", sizeof(wow64::ULONG)}, {"PVOID", "ContextData", sizeof(wow64::PVOID)}, {"PVOID", "STARSectionPointer", sizeof(wow64::PVOID)}, {"PULONG", "SectionSize", sizeof(wow64::PULONG)}}},
        {"_NtGetWriteWatch@28", 7, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PVOID", "BaseAddress", sizeof(wow64::PVOID)}, {"SIZE_T", "RegionSize", sizeof(wow64::SIZE_T)}, {"PVOID", "STARUserAddressArray", sizeof(wow64::PVOID)}, {"PULONG_PTR", "EntriesInUserAddressArray", sizeof(wow64::PULONG_PTR)}, {"PULONG", "Granularity", sizeof(wow64::PULONG)}}},
        {"_NtImpersonateAnonymousToken@4", 1, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}}},
        {"_NtInitializeNlsFiles@12", 3, {{"PVOID", "STARBaseAddress", sizeof(wow64::PVOID)}, {"PLCID", "DefaultLocaleId", sizeof(wow64::PLCID)}, {"PLARGE_INTEGER", "DefaultCasingTableSize", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtInitiatePowerAction@16", 4, {{"POWER_ACTION", "SystemAction", sizeof(wow64::POWER_ACTION)}, {"SYSTEM_POWER_STATE", "MinSystemState", sizeof(wow64::SYSTEM_POWER_STATE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(wow64::BOOLEAN)}}},
        {"_NtIsSystemResumeAutomatic@0", 0, {}},
        {"_NtLoadDriver@4", 1, {{"PUNICODE_STRING", "DriverServiceName", sizeof(wow64::PUNICODE_STRING)}}},
        {"_NtLoadKey2@12", 3, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}}},
        {"_NtLoadKey@8", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtLoadKeyEx@32", 8, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"HANDLE", "TrustClassKey", sizeof(wow64::HANDLE)}, {"PVOID", "Reserved", sizeof(wow64::PVOID)}, {"PVOID", "ObjectContext", sizeof(wow64::PVOID)}, {"PVOID", "CallbackReserverd", sizeof(wow64::PVOID)}, {"PVOID", "IoStatusBlock", sizeof(wow64::PVOID)}}},
        {"_NtLockFile@40", 10, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(wow64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "Length", sizeof(wow64::PLARGE_INTEGER)}, {"ULONG", "Key", sizeof(wow64::ULONG)}, {"BOOLEAN", "FailImmediately", sizeof(wow64::BOOLEAN)}, {"BOOLEAN", "ExclusiveLock", sizeof(wow64::BOOLEAN)}}},
        {"_NtLockRegistryKey@4", 1, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}}},
        {"_NtMakeTemporaryObject@4", 1, {{"HANDLE", "Handle", sizeof(wow64::HANDLE)}}},
        {"_NtMapUserPhysicalPages@12", 3, {{"PVOID", "VirtualAddress", sizeof(wow64::PVOID)}, {"ULONG_PTR", "NumberOfPages", sizeof(wow64::ULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(wow64::PULONG_PTR)}}},
        {"_NtModifyBootEntry@4", 1, {{"PBOOT_ENTRY", "BootEntry", sizeof(wow64::PBOOT_ENTRY)}}},
        {"_NtNotifyChangeDirectoryFile@36", 9, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"ULONG", "CompletionFilter", sizeof(wow64::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(wow64::BOOLEAN)}}},
        {"_NtNotifyChangeKey@40", 10, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"ULONG", "CompletionFilter", sizeof(wow64::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(wow64::BOOLEAN)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "BufferSize", sizeof(wow64::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(wow64::BOOLEAN)}}},
        {"_NtNotifyChangeMultipleKeys@48", 12, {{"HANDLE", "MasterKeyHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Count", sizeof(wow64::ULONG)}, {"POBJECT_ATTRIBUTES", "SlaveObjects", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"ULONG", "CompletionFilter", sizeof(wow64::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(wow64::BOOLEAN)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "BufferSize", sizeof(wow64::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(wow64::BOOLEAN)}}},
        {"_NtNotifyChangeSession@32", 8, {{"HANDLE", "Session", sizeof(wow64::HANDLE)}, {"ULONG", "IoStateSequence", sizeof(wow64::ULONG)}, {"PVOID", "Reserved", sizeof(wow64::PVOID)}, {"ULONG", "Action", sizeof(wow64::ULONG)}, {"IO_SESSION_STATE", "IoState", sizeof(wow64::IO_SESSION_STATE)}, {"IO_SESSION_STATE", "IoState2", sizeof(wow64::IO_SESSION_STATE)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "BufferSize", sizeof(wow64::ULONG)}}},
        {"_NtOpenEvent@12", 3, {{"PHANDLE", "EventHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenEventPair@12", 3, {{"PHANDLE", "EventPairHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenFile@24", 6, {{"PHANDLE", "FileHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"ULONG", "ShareAccess", sizeof(wow64::ULONG)}, {"ULONG", "OpenOptions", sizeof(wow64::ULONG)}}},
        {"_NtOpenKeyTransacted@16", 4, {{"PHANDLE", "KeyHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"HANDLE", "TransactionHandle", sizeof(wow64::HANDLE)}}},
        {"_NtOpenKeyTransactedEx@20", 5, {{"PHANDLE", "KeyHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "OpenOptions", sizeof(wow64::ULONG)}, {"HANDLE", "TransactionHandle", sizeof(wow64::HANDLE)}}},
        {"_NtOpenKeyedEvent@12", 3, {{"PHANDLE", "KeyedEventHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenMutant@12", 3, {{"PHANDLE", "MutantHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenPrivateNamespace@16", 4, {{"PHANDLE", "NamespaceHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PVOID", "BoundaryDescriptor", sizeof(wow64::PVOID)}}},
        {"_NtOpenSection@12", 3, {{"PHANDLE", "SectionHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenSemaphore@12", 3, {{"PHANDLE", "SemaphoreHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenSession@12", 3, {{"PHANDLE", "SessionHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenSymbolicLinkObject@12", 3, {{"PHANDLE", "LinkHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenThreadToken@16", 4, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"BOOLEAN", "OpenAsSelf", sizeof(wow64::BOOLEAN)}, {"PHANDLE", "TokenHandle", sizeof(wow64::PHANDLE)}}},
        {"_NtOpenThreadTokenEx@20", 5, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"BOOLEAN", "OpenAsSelf", sizeof(wow64::BOOLEAN)}, {"ULONG", "HandleAttributes", sizeof(wow64::ULONG)}, {"PHANDLE", "TokenHandle", sizeof(wow64::PHANDLE)}}},
        {"_NtPlugPlayControl@12", 3, {{"PLUGPLAY_CONTROL_CLASS", "PnPControlClass", sizeof(wow64::PLUGPLAY_CONTROL_CLASS)}, {"PVOID", "PnPControlData", sizeof(wow64::PVOID)}, {"ULONG", "PnPControlDataLength", sizeof(wow64::ULONG)}}},
        {"_NtPrePrepareEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtPrepareComplete@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtPrivilegedServiceAuditAlarm@20", 5, {{"PUNICODE_STRING", "SubsystemName", sizeof(wow64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ServiceName", sizeof(wow64::PUNICODE_STRING)}, {"HANDLE", "ClientToken", sizeof(wow64::HANDLE)}, {"PPRIVILEGE_SET", "Privileges", sizeof(wow64::PPRIVILEGE_SET)}, {"BOOLEAN", "AccessGranted", sizeof(wow64::BOOLEAN)}}},
        {"_NtPropagationComplete@16", 4, {{"HANDLE", "ResourceManagerHandle", sizeof(wow64::HANDLE)}, {"ULONG", "RequestCookie", sizeof(wow64::ULONG)}, {"ULONG", "BufferLength", sizeof(wow64::ULONG)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}}},
        {"_NtQueryDebugFilterState@8", 2, {{"ULONG", "ComponentId", sizeof(wow64::ULONG)}, {"ULONG", "Level", sizeof(wow64::ULONG)}}},
        {"_NtQueryDefaultLocale@8", 2, {{"BOOLEAN", "UserProfile", sizeof(wow64::BOOLEAN)}, {"PLCID", "DefaultLocaleId", sizeof(wow64::PLCID)}}},
        {"_NtQueryDriverEntryOrder@8", 2, {{"PULONG", "Ids", sizeof(wow64::PULONG)}, {"PULONG", "Count", sizeof(wow64::PULONG)}}},
        {"_NtQueryEvent@20", 5, {{"HANDLE", "EventHandle", sizeof(wow64::HANDLE)}, {"EVENT_INFORMATION_CLASS", "EventInformationClass", sizeof(wow64::EVENT_INFORMATION_CLASS)}, {"PVOID", "EventInformation", sizeof(wow64::PVOID)}, {"ULONG", "EventInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_NtQueryInformationAtom@20", 5, {{"RTL_ATOM", "Atom", sizeof(wow64::RTL_ATOM)}, {"ATOM_INFORMATION_CLASS", "InformationClass", sizeof(wow64::ATOM_INFORMATION_CLASS)}, {"PVOID", "AtomInformation", sizeof(wow64::PVOID)}, {"ULONG", "AtomInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_NtQueryInformationThread@20", 5, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"THREADINFOCLASS", "ThreadInformationClass", sizeof(wow64::THREADINFOCLASS)}, {"PVOID", "ThreadInformation", sizeof(wow64::PVOID)}, {"ULONG", "ThreadInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_NtQueryInformationTransactionManager@20", 5, {{"HANDLE", "TransactionManagerHandle", sizeof(wow64::HANDLE)}, {"TRANSACTIONMANAGER_INFORMATION_CLASS", "TransactionManagerInformationClass", sizeof(wow64::TRANSACTIONMANAGER_INFORMATION_CLASS)}, {"PVOID", "TransactionManagerInformation", sizeof(wow64::PVOID)}, {"ULONG", "TransactionManagerInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_NtQueryInstallUILanguage@4", 1, {{"LANGID", "STARInstallUILanguageId", sizeof(wow64::LANGID)}}},
        {"_NtQueryIntervalProfile@8", 2, {{"KPROFILE_SOURCE", "ProfileSource", sizeof(wow64::KPROFILE_SOURCE)}, {"PULONG", "Interval", sizeof(wow64::PULONG)}}},
        {"_NtQueryIoCompletion@20", 5, {{"HANDLE", "IoCompletionHandle", sizeof(wow64::HANDLE)}, {"IO_COMPLETION_INFORMATION_CLASS", "IoCompletionInformationClass", sizeof(wow64::IO_COMPLETION_INFORMATION_CLASS)}, {"PVOID", "IoCompletionInformation", sizeof(wow64::PVOID)}, {"ULONG", "IoCompletionInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_NtQueryLicenseValue@20", 5, {{"PUNICODE_STRING", "Name", sizeof(wow64::PUNICODE_STRING)}, {"PULONG", "Type", sizeof(wow64::PULONG)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PULONG", "ReturnedLength", sizeof(wow64::PULONG)}}},
        {"_NtQueryMultipleValueKey@24", 6, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"PKEY_VALUE_ENTRY", "ValueEntries", sizeof(wow64::PKEY_VALUE_ENTRY)}, {"ULONG", "EntryCount", sizeof(wow64::ULONG)}, {"PVOID", "ValueBuffer", sizeof(wow64::PVOID)}, {"PULONG", "BufferLength", sizeof(wow64::PULONG)}, {"PULONG", "RequiredBufferLength", sizeof(wow64::PULONG)}}},
        {"_NtQueryMutant@20", 5, {{"HANDLE", "MutantHandle", sizeof(wow64::HANDLE)}, {"MUTANT_INFORMATION_CLASS", "MutantInformationClass", sizeof(wow64::MUTANT_INFORMATION_CLASS)}, {"PVOID", "MutantInformation", sizeof(wow64::PVOID)}, {"ULONG", "MutantInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_NtQueryObject@20", 5, {{"HANDLE", "Handle", sizeof(wow64::HANDLE)}, {"OBJECT_INFORMATION_CLASS", "ObjectInformationClass", sizeof(wow64::OBJECT_INFORMATION_CLASS)}, {"PVOID", "ObjectInformation", sizeof(wow64::PVOID)}, {"ULONG", "ObjectInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_NtQueryOpenSubKeys@8", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PULONG", "HandleCount", sizeof(wow64::PULONG)}}},
        {"_NtQueryOpenSubKeysEx@16", 4, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "BufferLength", sizeof(wow64::ULONG)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"PULONG", "RequiredSize", sizeof(wow64::PULONG)}}},
        {"_NtQueryPerformanceCounter@8", 2, {{"PLARGE_INTEGER", "PerformanceCounter", sizeof(wow64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "PerformanceFrequency", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtQuerySecurityObject@20", 5, {{"HANDLE", "Handle", sizeof(wow64::HANDLE)}, {"SECURITY_INFORMATION", "SecurityInformation", sizeof(wow64::SECURITY_INFORMATION)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(wow64::PSECURITY_DESCRIPTOR)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PULONG", "LengthNeeded", sizeof(wow64::PULONG)}}},
        {"_NtQuerySystemInformation@16", 4, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(wow64::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "SystemInformation", sizeof(wow64::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_NtQuerySystemTime@4", 1, {{"PLARGE_INTEGER", "SystemTime", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtQueryTimerResolution@12", 3, {{"PULONG", "MaximumTime", sizeof(wow64::PULONG)}, {"PULONG", "MinimumTime", sizeof(wow64::PULONG)}, {"PULONG", "CurrentTime", sizeof(wow64::PULONG)}}},
        {"_NtQueryVirtualMemory@24", 6, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(wow64::PVOID)}, {"MEMORY_INFORMATION_CLASS", "MemoryInformationClass", sizeof(wow64::MEMORY_INFORMATION_CLASS)}, {"PVOID", "MemoryInformation", sizeof(wow64::PVOID)}, {"SIZE_T", "MemoryInformationLength", sizeof(wow64::SIZE_T)}, {"PSIZE_T", "ReturnLength", sizeof(wow64::PSIZE_T)}}},
        {"_NtQueryVolumeInformationFile@20", 5, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "FsInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"FS_INFORMATION_CLASS", "FsInformationClass", sizeof(wow64::FS_INFORMATION_CLASS)}}},
        {"_NtQueueApcThread@20", 5, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"PPS_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PPS_APC_ROUTINE)}, {"PVOID", "ApcArgument1", sizeof(wow64::PVOID)}, {"PVOID", "ApcArgument2", sizeof(wow64::PVOID)}, {"PVOID", "ApcArgument3", sizeof(wow64::PVOID)}}},
        {"_NtQueueApcThreadEx@24", 6, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "UserApcReserveHandle", sizeof(wow64::HANDLE)}, {"PPS_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PPS_APC_ROUTINE)}, {"PVOID", "ApcArgument1", sizeof(wow64::PVOID)}, {"PVOID", "ApcArgument2", sizeof(wow64::PVOID)}, {"PVOID", "ApcArgument3", sizeof(wow64::PVOID)}}},
        {"_NtReadFile@36", 9, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(wow64::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(wow64::PULONG)}}},
        {"_NtReadFileScatter@36", 9, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PFILE_SEGMENT_ELEMENT", "SegmentArray", sizeof(wow64::PFILE_SEGMENT_ELEMENT)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(wow64::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(wow64::PULONG)}}},
        {"_NtReadVirtualMemory@20", 5, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(wow64::PVOID)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"SIZE_T", "BufferSize", sizeof(wow64::SIZE_T)}, {"PSIZE_T", "NumberOfBytesRead", sizeof(wow64::PSIZE_T)}}},
        {"_NtRecoverEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PVOID", "EnlistmentKey", sizeof(wow64::PVOID)}}},
        {"_NtRecoverResourceManager@4", 1, {{"HANDLE", "ResourceManagerHandle", sizeof(wow64::HANDLE)}}},
        {"_NtReleaseKeyedEvent@16", 4, {{"HANDLE", "KeyedEventHandle", sizeof(wow64::HANDLE)}, {"PVOID", "KeyValue", sizeof(wow64::PVOID)}, {"BOOLEAN", "Alertable", sizeof(wow64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtReleaseSemaphore@12", 3, {{"HANDLE", "SemaphoreHandle", sizeof(wow64::HANDLE)}, {"LONG", "ReleaseCount", sizeof(wow64::LONG)}, {"PLONG", "PreviousCount", sizeof(wow64::PLONG)}}},
        {"_NtRenameTransactionManager@8", 2, {{"PUNICODE_STRING", "LogFileName", sizeof(wow64::PUNICODE_STRING)}, {"LPGUID", "ExistingTransactionManagerGuid", sizeof(wow64::LPGUID)}}},
        {"_NtReplacePartitionUnit@12", 3, {{"PUNICODE_STRING", "TargetInstancePath", sizeof(wow64::PUNICODE_STRING)}, {"PUNICODE_STRING", "SpareInstancePath", sizeof(wow64::PUNICODE_STRING)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}}},
        {"_NtReplyWaitReceivePort@16", 4, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PVOID", "STARPortContext", sizeof(wow64::PVOID)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(wow64::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(wow64::PPORT_MESSAGE)}}},
        {"_NtReplyWaitReceivePortEx@20", 5, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PVOID", "STARPortContext", sizeof(wow64::PVOID)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(wow64::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(wow64::PPORT_MESSAGE)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtReplyWaitReplyPort@8", 2, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(wow64::PPORT_MESSAGE)}}},
        {"_NtRequestWaitReplyPort@12", 3, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "RequestMessage", sizeof(wow64::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(wow64::PPORT_MESSAGE)}}},
        {"_NtResetEvent@8", 2, {{"HANDLE", "EventHandle", sizeof(wow64::HANDLE)}, {"PLONG", "PreviousState", sizeof(wow64::PLONG)}}},
        {"_NtRestoreKey@12", 3, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}}},
        {"_NtRollbackEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtRollbackTransaction@8", 2, {{"HANDLE", "TransactionHandle", sizeof(wow64::HANDLE)}, {"BOOLEAN", "Wait", sizeof(wow64::BOOLEAN)}}},
        {"_NtRollforwardTransactionManager@8", 2, {{"HANDLE", "TransactionManagerHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtSaveKey@8", 2, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}}},
        {"_NtSaveKeyEx@12", 3, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Format", sizeof(wow64::ULONG)}}},
        {"_NtSaveMergedKeys@12", 3, {{"HANDLE", "HighPrecedenceKeyHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "LowPrecedenceKeyHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}}},
        {"_NtSecureConnectPort@36", 9, {{"PHANDLE", "PortHandle", sizeof(wow64::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(wow64::PUNICODE_STRING)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(wow64::PSECURITY_QUALITY_OF_SERVICE)}, {"PPORT_VIEW", "ClientView", sizeof(wow64::PPORT_VIEW)}, {"PSID", "RequiredServerSid", sizeof(wow64::PSID)}, {"PREMOTE_PORT_VIEW", "ServerView", sizeof(wow64::PREMOTE_PORT_VIEW)}, {"PULONG", "MaxMessageLength", sizeof(wow64::PULONG)}, {"PVOID", "ConnectionInformation", sizeof(wow64::PVOID)}, {"PULONG", "ConnectionInformationLength", sizeof(wow64::PULONG)}}},
        {"_NtSerializeBoot@0", 0, {}},
        {"_NtSetDebugFilterState@12", 3, {{"ULONG", "ComponentId", sizeof(wow64::ULONG)}, {"ULONG", "Level", sizeof(wow64::ULONG)}, {"BOOLEAN", "State", sizeof(wow64::BOOLEAN)}}},
        {"_NtSetDefaultHardErrorPort@4", 1, {{"HANDLE", "DefaultHardErrorPort", sizeof(wow64::HANDLE)}}},
        {"_NtSetDefaultLocale@8", 2, {{"BOOLEAN", "UserProfile", sizeof(wow64::BOOLEAN)}, {"LCID", "DefaultLocaleId", sizeof(wow64::LCID)}}},
        {"_NtSetDriverEntryOrder@8", 2, {{"PULONG", "Ids", sizeof(wow64::PULONG)}, {"ULONG", "Count", sizeof(wow64::ULONG)}}},
        {"_NtSetEvent@8", 2, {{"HANDLE", "EventHandle", sizeof(wow64::HANDLE)}, {"PLONG", "PreviousState", sizeof(wow64::PLONG)}}},
        {"_NtSetEventBoostPriority@4", 1, {{"HANDLE", "EventHandle", sizeof(wow64::HANDLE)}}},
        {"_NtSetHighEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(wow64::HANDLE)}}},
        {"_NtSetHighWaitLowEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(wow64::HANDLE)}}},
        {"_NtSetInformationEnlistment@16", 4, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"ENLISTMENT_INFORMATION_CLASS", "EnlistmentInformationClass", sizeof(wow64::ENLISTMENT_INFORMATION_CLASS)}, {"PVOID", "EnlistmentInformation", sizeof(wow64::PVOID)}, {"ULONG", "EnlistmentInformationLength", sizeof(wow64::ULONG)}}},
        {"_NtSetIntervalProfile@8", 2, {{"ULONG", "Interval", sizeof(wow64::ULONG)}, {"KPROFILE_SOURCE", "Source", sizeof(wow64::KPROFILE_SOURCE)}}},
        {"_NtSetIoCompletion@20", 5, {{"HANDLE", "IoCompletionHandle", sizeof(wow64::HANDLE)}, {"PVOID", "KeyContext", sizeof(wow64::PVOID)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"NTSTATUS", "IoStatus", sizeof(wow64::NTSTATUS)}, {"ULONG_PTR", "IoStatusInformation", sizeof(wow64::ULONG_PTR)}}},
        {"_NtSetIoCompletionEx@24", 6, {{"HANDLE", "IoCompletionHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "IoCompletionReserveHandle", sizeof(wow64::HANDLE)}, {"PVOID", "KeyContext", sizeof(wow64::PVOID)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"NTSTATUS", "IoStatus", sizeof(wow64::NTSTATUS)}, {"ULONG_PTR", "IoStatusInformation", sizeof(wow64::ULONG_PTR)}}},
        {"_NtSetSecurityObject@12", 3, {{"HANDLE", "Handle", sizeof(wow64::HANDLE)}, {"SECURITY_INFORMATION", "SecurityInformation", sizeof(wow64::SECURITY_INFORMATION)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(wow64::PSECURITY_DESCRIPTOR)}}},
        {"_NtSetTimerResolution@12", 3, {{"ULONG", "DesiredTime", sizeof(wow64::ULONG)}, {"BOOLEAN", "SetResolution", sizeof(wow64::BOOLEAN)}, {"PULONG", "ActualTime", sizeof(wow64::PULONG)}}},
        {"_NtSetVolumeInformationFile@20", 5, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "FsInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"FS_INFORMATION_CLASS", "FsInformationClass", sizeof(wow64::FS_INFORMATION_CLASS)}}},
        {"_NtShutdownWorkerFactory@8", 2, {{"HANDLE", "WorkerFactoryHandle", sizeof(wow64::HANDLE)}, {"LONG", "STARPendingWorkerCount", sizeof(wow64::LONG)}}},
        {"_NtStartProfile@4", 1, {{"HANDLE", "ProfileHandle", sizeof(wow64::HANDLE)}}},
        {"_NtSystemDebugControl@24", 6, {{"SYSDBG_COMMAND", "Command", sizeof(wow64::SYSDBG_COMMAND)}, {"PVOID", "InputBuffer", sizeof(wow64::PVOID)}, {"ULONG", "InputBufferLength", sizeof(wow64::ULONG)}, {"PVOID", "OutputBuffer", sizeof(wow64::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_NtTestAlert@0", 0, {}},
        {"_NtThawTransactions@0", 0, {}},
        {"_NtTraceEvent@16", 4, {{"HANDLE", "TraceHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"ULONG", "FieldSize", sizeof(wow64::ULONG)}, {"PVOID", "Fields", sizeof(wow64::PVOID)}}},
        {"_NtTranslateFilePath@16", 4, {{"PFILE_PATH", "InputFilePath", sizeof(wow64::PFILE_PATH)}, {"ULONG", "OutputType", sizeof(wow64::ULONG)}, {"PFILE_PATH", "OutputFilePath", sizeof(wow64::PFILE_PATH)}, {"PULONG", "OutputFilePathLength", sizeof(wow64::PULONG)}}},
        {"_NtUnloadKey@4", 1, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_NtUnlockVirtualMemory@16", 4, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(wow64::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(wow64::PSIZE_T)}, {"ULONG", "MapType", sizeof(wow64::ULONG)}}},
        {"_NtUnmapViewOfSection@8", 2, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(wow64::PVOID)}}},
        {"_NtVdmControl@8", 2, {{"VDMSERVICECLASS", "Service", sizeof(wow64::VDMSERVICECLASS)}, {"PVOID", "ServiceData", sizeof(wow64::PVOID)}}},
        {"_NtWaitForDebugEvent@16", 4, {{"HANDLE", "DebugObjectHandle", sizeof(wow64::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(wow64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}, {"PDBGUI_WAIT_STATE_CHANGE", "WaitStateChange", sizeof(wow64::PDBGUI_WAIT_STATE_CHANGE)}}},
        {"_NtWaitForKeyedEvent@16", 4, {{"HANDLE", "KeyedEventHandle", sizeof(wow64::HANDLE)}, {"PVOID", "KeyValue", sizeof(wow64::PVOID)}, {"BOOLEAN", "Alertable", sizeof(wow64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtWaitForMultipleObjects32@20", 5, {{"ULONG", "Count", sizeof(wow64::ULONG)}, {"LONG", "Handles", sizeof(wow64::LONG)}, {"WAIT_TYPE", "WaitType", sizeof(wow64::WAIT_TYPE)}, {"BOOLEAN", "Alertable", sizeof(wow64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtWaitForMultipleObjects@20", 5, {{"ULONG", "Count", sizeof(wow64::ULONG)}, {"HANDLE", "Handles", sizeof(wow64::HANDLE)}, {"WAIT_TYPE", "WaitType", sizeof(wow64::WAIT_TYPE)}, {"BOOLEAN", "Alertable", sizeof(wow64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_NtWaitForWorkViaWorkerFactory@20", 5, {{"HANDLE", "WorkerFactoryHandle", sizeof(wow64::HANDLE)}, {"PFILE_IO_COMPLETION_INFORMATION", "MiniPacket", sizeof(wow64::PFILE_IO_COMPLETION_INFORMATION)}, {"LONG", "Arg2", sizeof(wow64::LONG)}, {"LONG", "Arg3", sizeof(wow64::LONG)}, {"LONG", "Arg4", sizeof(wow64::LONG)}}},
        {"_NtWaitLowEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(wow64::HANDLE)}}},
        {"_NtWorkerFactoryWorkerReady@4", 1, {{"HANDLE", "WorkerFactoryHandle", sizeof(wow64::HANDLE)}}},
        {"_NtWriteFile@36", 9, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(wow64::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(wow64::PULONG)}}},
        {"_NtWriteFileGather@36", 9, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PFILE_SEGMENT_ELEMENT", "SegmentArray", sizeof(wow64::PFILE_SEGMENT_ELEMENT)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(wow64::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(wow64::PULONG)}}},
        {"_NtWriteRequestData@24", 6, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(wow64::PPORT_MESSAGE)}, {"ULONG", "DataEntryIndex", sizeof(wow64::ULONG)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"SIZE_T", "BufferSize", sizeof(wow64::SIZE_T)}, {"PSIZE_T", "NumberOfBytesWritten", sizeof(wow64::PSIZE_T)}}},
        {"_NtWriteVirtualMemory@20", 5, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(wow64::PVOID)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"SIZE_T", "BufferSize", sizeof(wow64::SIZE_T)}, {"PSIZE_T", "NumberOfBytesWritten", sizeof(wow64::PSIZE_T)}}},
        {"_ZwAccessCheckAndAuditAlarm@44", 11, {{"PUNICODE_STRING", "SubsystemName", sizeof(wow64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(wow64::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(wow64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(wow64::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(wow64::PSECURITY_DESCRIPTOR)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(wow64::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(wow64::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(wow64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(wow64::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(wow64::PBOOLEAN)}}},
        {"_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle@68", 17, {{"PUNICODE_STRING", "SubsystemName", sizeof(wow64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(wow64::PVOID)}, {"HANDLE", "ClientToken", sizeof(wow64::HANDLE)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(wow64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(wow64::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(wow64::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(wow64::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(wow64::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(wow64::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(wow64::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(wow64::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(wow64::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(wow64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(wow64::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(wow64::PBOOLEAN)}}},
        {"_ZwAddBootEntry@8", 2, {{"PBOOT_ENTRY", "BootEntry", sizeof(wow64::PBOOT_ENTRY)}, {"PULONG", "Id", sizeof(wow64::PULONG)}}},
        {"_ZwAdjustGroupsToken@24", 6, {{"HANDLE", "TokenHandle", sizeof(wow64::HANDLE)}, {"BOOLEAN", "ResetToDefault", sizeof(wow64::BOOLEAN)}, {"PTOKEN_GROUPS", "NewState", sizeof(wow64::PTOKEN_GROUPS)}, {"ULONG", "BufferLength", sizeof(wow64::ULONG)}, {"PTOKEN_GROUPS", "PreviousState", sizeof(wow64::PTOKEN_GROUPS)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwAdjustPrivilegesToken@24", 6, {{"HANDLE", "TokenHandle", sizeof(wow64::HANDLE)}, {"BOOLEAN", "DisableAllPrivileges", sizeof(wow64::BOOLEAN)}, {"PTOKEN_PRIVILEGES", "NewState", sizeof(wow64::PTOKEN_PRIVILEGES)}, {"ULONG", "BufferLength", sizeof(wow64::ULONG)}, {"PTOKEN_PRIVILEGES", "PreviousState", sizeof(wow64::PTOKEN_PRIVILEGES)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwAllocateLocallyUniqueId@4", 1, {{"PLUID", "Luid", sizeof(wow64::PLUID)}}},
        {"_ZwAlpcCancelMessage@12", 3, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PALPC_CONTEXT_ATTR", "MessageContext", sizeof(wow64::PALPC_CONTEXT_ATTR)}}},
        {"_ZwAlpcConnectPort@44", 11, {{"PHANDLE", "PortHandle", sizeof(wow64::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(wow64::PUNICODE_STRING)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(wow64::PALPC_PORT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PSID", "RequiredServerSid", sizeof(wow64::PSID)}, {"PPORT_MESSAGE", "ConnectionMessage", sizeof(wow64::PPORT_MESSAGE)}, {"PULONG", "BufferLength", sizeof(wow64::PULONG)}, {"PALPC_MESSAGE_ATTRIBUTES", "OutMessageAttributes", sizeof(wow64::PALPC_MESSAGE_ATTRIBUTES)}, {"PALPC_MESSAGE_ATTRIBUTES", "InMessageAttributes", sizeof(wow64::PALPC_MESSAGE_ATTRIBUTES)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwAlpcCreatePort@12", 3, {{"PHANDLE", "PortHandle", sizeof(wow64::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(wow64::PALPC_PORT_ATTRIBUTES)}}},
        {"_ZwAlpcCreateResourceReserve@16", 4, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"SIZE_T", "MessageSize", sizeof(wow64::SIZE_T)}, {"PALPC_HANDLE", "ResourceId", sizeof(wow64::PALPC_HANDLE)}}},
        {"_ZwAlpcCreateSectionView@12", 3, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PALPC_DATA_VIEW_ATTR", "ViewAttributes", sizeof(wow64::PALPC_DATA_VIEW_ATTR)}}},
        {"_ZwAlpcCreateSecurityContext@12", 3, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PALPC_SECURITY_ATTR", "SecurityAttribute", sizeof(wow64::PALPC_SECURITY_ATTR)}}},
        {"_ZwAlpcDeletePortSection@12", 3, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"ALPC_HANDLE", "SectionHandle", sizeof(wow64::ALPC_HANDLE)}}},
        {"_ZwAlpcImpersonateClientOfPort@12", 3, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(wow64::PPORT_MESSAGE)}, {"PVOID", "Reserved", sizeof(wow64::PVOID)}}},
        {"_ZwAlpcOpenSenderProcess@24", 6, {{"PHANDLE", "ProcessHandle", sizeof(wow64::PHANDLE)}, {"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(wow64::PPORT_MESSAGE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwAlpcOpenSenderThread@24", 6, {{"PHANDLE", "ThreadHandle", sizeof(wow64::PHANDLE)}, {"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(wow64::PPORT_MESSAGE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwAlpcQueryInformation@20", 5, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"ALPC_PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(wow64::ALPC_PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwAlpcQueryInformationMessage@24", 6, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(wow64::PPORT_MESSAGE)}, {"ALPC_MESSAGE_INFORMATION_CLASS", "MessageInformationClass", sizeof(wow64::ALPC_MESSAGE_INFORMATION_CLASS)}, {"PVOID", "MessageInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwAreMappedFilesTheSame@8", 2, {{"PVOID", "File1MappedAsAnImage", sizeof(wow64::PVOID)}, {"PVOID", "File2MappedAsFile", sizeof(wow64::PVOID)}}},
        {"_ZwAssignProcessToJobObject@8", 2, {{"HANDLE", "JobHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwCancelIoFile@8", 2, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}}},
        {"_ZwCancelTimer@8", 2, {{"HANDLE", "TimerHandle", sizeof(wow64::HANDLE)}, {"PBOOLEAN", "CurrentState", sizeof(wow64::PBOOLEAN)}}},
        {"_ZwCloseObjectAuditAlarm@12", 3, {{"PUNICODE_STRING", "SubsystemName", sizeof(wow64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(wow64::PVOID)}, {"BOOLEAN", "GenerateOnClose", sizeof(wow64::BOOLEAN)}}},
        {"_ZwCommitComplete@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwCompareTokens@12", 3, {{"HANDLE", "FirstTokenHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "SecondTokenHandle", sizeof(wow64::HANDLE)}, {"PBOOLEAN", "Equal", sizeof(wow64::PBOOLEAN)}}},
        {"_ZwCompressKey@4", 1, {{"HANDLE", "Key", sizeof(wow64::HANDLE)}}},
        {"_ZwContinue@8", 2, {{"PCONTEXT", "ContextRecord", sizeof(wow64::PCONTEXT)}, {"BOOLEAN", "TestAlert", sizeof(wow64::BOOLEAN)}}},
        {"_ZwCreateDebugObject@16", 4, {{"PHANDLE", "DebugObjectHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}}},
        {"_ZwCreateDirectoryObject@12", 3, {{"PHANDLE", "DirectoryHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwCreateEnlistment@32", 8, {{"PHANDLE", "EnlistmentHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"HANDLE", "ResourceManagerHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "TransactionHandle", sizeof(wow64::HANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "CreateOptions", sizeof(wow64::ULONG)}, {"NOTIFICATION_MASK", "NotificationMask", sizeof(wow64::NOTIFICATION_MASK)}, {"PVOID", "EnlistmentKey", sizeof(wow64::PVOID)}}},
        {"_ZwCreateJobObject@12", 3, {{"PHANDLE", "JobHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwCreateKey@28", 7, {{"PHANDLE", "KeyHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "TitleIndex", sizeof(wow64::ULONG)}, {"PUNICODE_STRING", "Class", sizeof(wow64::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(wow64::ULONG)}, {"PULONG", "Disposition", sizeof(wow64::PULONG)}}},
        {"_ZwCreateKeyedEvent@16", 4, {{"PHANDLE", "KeyedEventHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}}},
        {"_ZwCreateMailslotFile@32", 8, {{"PHANDLE", "FileHandle", sizeof(wow64::PHANDLE)}, {"ULONG", "DesiredAccess", sizeof(wow64::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"ULONG", "CreateOptions", sizeof(wow64::ULONG)}, {"ULONG", "MailslotQuota", sizeof(wow64::ULONG)}, {"ULONG", "MaximumMessageSize", sizeof(wow64::ULONG)}, {"PLARGE_INTEGER", "ReadTimeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwCreateMutant@16", 4, {{"PHANDLE", "MutantHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"BOOLEAN", "InitialOwner", sizeof(wow64::BOOLEAN)}}},
        {"_ZwCreateNamedPipeFile@56", 14, {{"PHANDLE", "FileHandle", sizeof(wow64::PHANDLE)}, {"ULONG", "DesiredAccess", sizeof(wow64::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"ULONG", "ShareAccess", sizeof(wow64::ULONG)}, {"ULONG", "CreateDisposition", sizeof(wow64::ULONG)}, {"ULONG", "CreateOptions", sizeof(wow64::ULONG)}, {"ULONG", "NamedPipeType", sizeof(wow64::ULONG)}, {"ULONG", "ReadMode", sizeof(wow64::ULONG)}, {"ULONG", "CompletionMode", sizeof(wow64::ULONG)}, {"ULONG", "MaximumInstances", sizeof(wow64::ULONG)}, {"ULONG", "InboundQuota", sizeof(wow64::ULONG)}, {"ULONG", "OutboundQuota", sizeof(wow64::ULONG)}, {"PLARGE_INTEGER", "DefaultTimeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwCreatePort@20", 5, {{"PHANDLE", "PortHandle", sizeof(wow64::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "MaxConnectionInfoLength", sizeof(wow64::ULONG)}, {"ULONG", "MaxMessageLength", sizeof(wow64::ULONG)}, {"ULONG", "MaxPoolUsage", sizeof(wow64::ULONG)}}},
        {"_ZwCreateProcess@32", 8, {{"PHANDLE", "ProcessHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"HANDLE", "ParentProcess", sizeof(wow64::HANDLE)}, {"BOOLEAN", "InheritObjectTable", sizeof(wow64::BOOLEAN)}, {"HANDLE", "SectionHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "DebugPort", sizeof(wow64::HANDLE)}, {"HANDLE", "ExceptionPort", sizeof(wow64::HANDLE)}}},
        {"_ZwCreateProcessEx@36", 9, {{"PHANDLE", "ProcessHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"HANDLE", "ParentProcess", sizeof(wow64::HANDLE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"HANDLE", "SectionHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "DebugPort", sizeof(wow64::HANDLE)}, {"HANDLE", "ExceptionPort", sizeof(wow64::HANDLE)}, {"ULONG", "JobMemberLevel", sizeof(wow64::ULONG)}}},
        {"_ZwCreateProfile@36", 9, {{"PHANDLE", "ProfileHandle", sizeof(wow64::PHANDLE)}, {"HANDLE", "Process", sizeof(wow64::HANDLE)}, {"PVOID", "RangeBase", sizeof(wow64::PVOID)}, {"SIZE_T", "RangeSize", sizeof(wow64::SIZE_T)}, {"ULONG", "BucketSize", sizeof(wow64::ULONG)}, {"PULONG", "Buffer", sizeof(wow64::PULONG)}, {"ULONG", "BufferSize", sizeof(wow64::ULONG)}, {"KPROFILE_SOURCE", "ProfileSource", sizeof(wow64::KPROFILE_SOURCE)}, {"KAFFINITY", "Affinity", sizeof(wow64::KAFFINITY)}}},
        {"_ZwCreateResourceManager@28", 7, {{"PHANDLE", "ResourceManagerHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"HANDLE", "TmHandle", sizeof(wow64::HANDLE)}, {"LPGUID", "RmGuid", sizeof(wow64::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "CreateOptions", sizeof(wow64::ULONG)}, {"PUNICODE_STRING", "Description", sizeof(wow64::PUNICODE_STRING)}}},
        {"_ZwCreateSymbolicLinkObject@16", 4, {{"PHANDLE", "LinkHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LinkTarget", sizeof(wow64::PUNICODE_STRING)}}},
        {"_ZwCreateTimer@16", 4, {{"PHANDLE", "TimerHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"TIMER_TYPE", "TimerType", sizeof(wow64::TIMER_TYPE)}}},
        {"_ZwCreateTransactionManager@24", 6, {{"PHANDLE", "TmHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LogFileName", sizeof(wow64::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(wow64::ULONG)}, {"ULONG", "CommitStrength", sizeof(wow64::ULONG)}}},
        {"_ZwCreateWaitablePort@20", 5, {{"PHANDLE", "PortHandle", sizeof(wow64::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "MaxConnectionInfoLength", sizeof(wow64::ULONG)}, {"ULONG", "MaxMessageLength", sizeof(wow64::ULONG)}, {"ULONG", "MaxPoolUsage", sizeof(wow64::ULONG)}}},
        {"_ZwDebugContinue@12", 3, {{"HANDLE", "DebugObjectHandle", sizeof(wow64::HANDLE)}, {"PCLIENT_ID", "ClientId", sizeof(wow64::PCLIENT_ID)}, {"NTSTATUS", "ContinueStatus", sizeof(wow64::NTSTATUS)}}},
        {"_ZwDelayExecution@8", 2, {{"BOOLEAN", "Alertable", sizeof(wow64::BOOLEAN)}, {"PLARGE_INTEGER", "DelayInterval", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwDeleteAtom@4", 1, {{"RTL_ATOM", "Atom", sizeof(wow64::RTL_ATOM)}}},
        {"_ZwDeleteDriverEntry@4", 1, {{"ULONG", "Id", sizeof(wow64::ULONG)}}},
        {"_ZwDeleteKey@4", 1, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwDeviceIoControlFile@40", 10, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"ULONG", "IoControlCode", sizeof(wow64::ULONG)}, {"PVOID", "InputBuffer", sizeof(wow64::PVOID)}, {"ULONG", "InputBufferLength", sizeof(wow64::ULONG)}, {"PVOID", "OutputBuffer", sizeof(wow64::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(wow64::ULONG)}}},
        {"_ZwDrawText@4", 1, {{"PUNICODE_STRING", "Text", sizeof(wow64::PUNICODE_STRING)}}},
        {"_ZwDuplicateObject@28", 7, {{"HANDLE", "SourceProcessHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "SourceHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "TargetProcessHandle", sizeof(wow64::HANDLE)}, {"PHANDLE", "TargetHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(wow64::ULONG)}, {"ULONG", "Options", sizeof(wow64::ULONG)}}},
        {"_ZwEnumerateBootEntries@8", 2, {{"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"PULONG", "BufferLength", sizeof(wow64::PULONG)}}},
        {"_ZwEnumerateKey@24", 6, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"ULONG", "Index", sizeof(wow64::ULONG)}, {"KEY_INFORMATION_CLASS", "KeyInformationClass", sizeof(wow64::KEY_INFORMATION_CLASS)}, {"PVOID", "KeyInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PULONG", "ResultLength", sizeof(wow64::PULONG)}}},
        {"_ZwEnumerateSystemEnvironmentValuesEx@12", 3, {{"ULONG", "InformationClass", sizeof(wow64::ULONG)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"PULONG", "BufferLength", sizeof(wow64::PULONG)}}},
        {"_ZwEnumerateTransactionObject@20", 5, {{"HANDLE", "RootObjectHandle", sizeof(wow64::HANDLE)}, {"KTMOBJECT_TYPE", "QueryType", sizeof(wow64::KTMOBJECT_TYPE)}, {"PKTMOBJECT_CURSOR", "ObjectCursor", sizeof(wow64::PKTMOBJECT_CURSOR)}, {"ULONG", "ObjectCursorLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwExtendSection@8", 2, {{"HANDLE", "SectionHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "NewSectionSize", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwFlushBuffersFile@8", 2, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}}},
        {"_ZwFlushInstallUILanguage@8", 2, {{"LANGID", "InstallUILanguage", sizeof(wow64::LANGID)}, {"ULONG", "SetComittedFlag", sizeof(wow64::ULONG)}}},
        {"_ZwFlushInstructionCache@12", 3, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(wow64::PVOID)}, {"SIZE_T", "Length", sizeof(wow64::SIZE_T)}}},
        {"_ZwFlushProcessWriteBuffers@0", 0, {}},
        {"_ZwFlushVirtualMemory@16", 4, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(wow64::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(wow64::PSIZE_T)}, {"PIO_STATUS_BLOCK", "IoStatus", sizeof(wow64::PIO_STATUS_BLOCK)}}},
        {"_ZwFreezeTransactions@8", 2, {{"PLARGE_INTEGER", "FreezeTimeout", sizeof(wow64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "ThawTimeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwGetNextProcess@20", 5, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(wow64::ULONG)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PHANDLE", "NewProcessHandle", sizeof(wow64::PHANDLE)}}},
        {"_ZwGetNextThread@24", 6, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(wow64::ULONG)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}, {"PHANDLE", "NewThreadHandle", sizeof(wow64::PHANDLE)}}},
        {"_ZwGetNotificationResourceManager@28", 7, {{"HANDLE", "ResourceManagerHandle", sizeof(wow64::HANDLE)}, {"PTRANSACTION_NOTIFICATION", "TransactionNotification", sizeof(wow64::PTRANSACTION_NOTIFICATION)}, {"ULONG", "NotificationLength", sizeof(wow64::ULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}, {"ULONG", "Asynchronous", sizeof(wow64::ULONG)}, {"ULONG_PTR", "AsynchronousContext", sizeof(wow64::ULONG_PTR)}}},
        {"_ZwImpersonateClientOfPort@8", 2, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(wow64::PPORT_MESSAGE)}}},
        {"_ZwImpersonateThread@12", 3, {{"HANDLE", "ServerThreadHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "ClientThreadHandle", sizeof(wow64::HANDLE)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(wow64::PSECURITY_QUALITY_OF_SERVICE)}}},
        {"_ZwInitializeRegistry@4", 1, {{"USHORT", "BootCondition", sizeof(wow64::USHORT)}}},
        {"_ZwIsProcessInJob@8", 2, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "JobHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwIsUILanguageComitted@0", 0, {}},
        {"_ZwListenPort@8", 2, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(wow64::PPORT_MESSAGE)}}},
        {"_ZwLockProductActivationKeys@8", 2, {{"ULONG", "STARpPrivateVer", sizeof(wow64::ULONG)}, {"ULONG", "STARpSafeMode", sizeof(wow64::ULONG)}}},
        {"_ZwLockVirtualMemory@16", 4, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(wow64::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(wow64::PSIZE_T)}, {"ULONG", "MapType", sizeof(wow64::ULONG)}}},
        {"_ZwMakePermanentObject@4", 1, {{"HANDLE", "Handle", sizeof(wow64::HANDLE)}}},
        {"_ZwMapCMFModule@24", 6, {{"ULONG", "What", sizeof(wow64::ULONG)}, {"ULONG", "Index", sizeof(wow64::ULONG)}, {"PULONG", "CacheIndexOut", sizeof(wow64::PULONG)}, {"PULONG", "CacheFlagsOut", sizeof(wow64::PULONG)}, {"PULONG", "ViewSizeOut", sizeof(wow64::PULONG)}, {"PVOID", "STARBaseAddress", sizeof(wow64::PVOID)}}},
        {"_ZwMapUserPhysicalPagesScatter@12", 3, {{"PVOID", "STARVirtualAddresses", sizeof(wow64::PVOID)}, {"ULONG_PTR", "NumberOfPages", sizeof(wow64::ULONG_PTR)}, {"PULONG_PTR", "UserPfnArray", sizeof(wow64::PULONG_PTR)}}},
        {"_ZwMapViewOfSection@40", 10, {{"HANDLE", "SectionHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(wow64::PVOID)}, {"ULONG_PTR", "ZeroBits", sizeof(wow64::ULONG_PTR)}, {"SIZE_T", "CommitSize", sizeof(wow64::SIZE_T)}, {"PLARGE_INTEGER", "SectionOffset", sizeof(wow64::PLARGE_INTEGER)}, {"PSIZE_T", "ViewSize", sizeof(wow64::PSIZE_T)}, {"SECTION_INHERIT", "InheritDisposition", sizeof(wow64::SECTION_INHERIT)}, {"ULONG", "AllocationType", sizeof(wow64::ULONG)}, {"WIN32_PROTECTION_MASK", "Win32Protect", sizeof(wow64::WIN32_PROTECTION_MASK)}}},
        {"_ZwModifyDriverEntry@4", 1, {{"PEFI_DRIVER_ENTRY", "DriverEntry", sizeof(wow64::PEFI_DRIVER_ENTRY)}}},
        {"_ZwOpenDirectoryObject@12", 3, {{"PHANDLE", "DirectoryHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenEnlistment@20", 5, {{"PHANDLE", "EnlistmentHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"HANDLE", "ResourceManagerHandle", sizeof(wow64::HANDLE)}, {"LPGUID", "EnlistmentGuid", sizeof(wow64::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenIoCompletion@12", 3, {{"PHANDLE", "IoCompletionHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenJobObject@12", 3, {{"PHANDLE", "JobHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenKey@12", 3, {{"PHANDLE", "KeyHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenKeyEx@16", 4, {{"PHANDLE", "KeyHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "OpenOptions", sizeof(wow64::ULONG)}}},
        {"_ZwOpenObjectAuditAlarm@48", 12, {{"PUNICODE_STRING", "SubsystemName", sizeof(wow64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(wow64::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(wow64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(wow64::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(wow64::PSECURITY_DESCRIPTOR)}, {"HANDLE", "ClientToken", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"ACCESS_MASK", "GrantedAccess", sizeof(wow64::ACCESS_MASK)}, {"PPRIVILEGE_SET", "Privileges", sizeof(wow64::PPRIVILEGE_SET)}, {"BOOLEAN", "ObjectCreation", sizeof(wow64::BOOLEAN)}, {"BOOLEAN", "AccessGranted", sizeof(wow64::BOOLEAN)}, {"PBOOLEAN", "GenerateOnClose", sizeof(wow64::PBOOLEAN)}}},
        {"_ZwOpenProcess@16", 4, {{"PHANDLE", "ProcessHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PCLIENT_ID", "ClientId", sizeof(wow64::PCLIENT_ID)}}},
        {"_ZwOpenProcessToken@12", 3, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"PHANDLE", "TokenHandle", sizeof(wow64::PHANDLE)}}},
        {"_ZwOpenProcessTokenEx@16", 4, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(wow64::ULONG)}, {"PHANDLE", "TokenHandle", sizeof(wow64::PHANDLE)}}},
        {"_ZwOpenResourceManager@20", 5, {{"PHANDLE", "ResourceManagerHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"HANDLE", "TmHandle", sizeof(wow64::HANDLE)}, {"LPGUID", "ResourceManagerGuid", sizeof(wow64::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenThread@16", 4, {{"PHANDLE", "ThreadHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PCLIENT_ID", "ClientId", sizeof(wow64::PCLIENT_ID)}}},
        {"_ZwOpenTimer@12", 3, {{"PHANDLE", "TimerHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenTransaction@20", 5, {{"PHANDLE", "TransactionHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"LPGUID", "Uow", sizeof(wow64::LPGUID)}, {"HANDLE", "TmHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwOpenTransactionManager@24", 6, {{"PHANDLE", "TmHandle", sizeof(wow64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LogFileName", sizeof(wow64::PUNICODE_STRING)}, {"LPGUID", "TmIdentity", sizeof(wow64::LPGUID)}, {"ULONG", "OpenOptions", sizeof(wow64::ULONG)}}},
        {"_ZwPowerInformation@20", 5, {{"POWER_INFORMATION_LEVEL", "InformationLevel", sizeof(wow64::POWER_INFORMATION_LEVEL)}, {"PVOID", "InputBuffer", sizeof(wow64::PVOID)}, {"ULONG", "InputBufferLength", sizeof(wow64::ULONG)}, {"PVOID", "OutputBuffer", sizeof(wow64::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(wow64::ULONG)}}},
        {"_ZwPrePrepareComplete@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwPrepareEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwPrivilegeCheck@12", 3, {{"HANDLE", "ClientToken", sizeof(wow64::HANDLE)}, {"PPRIVILEGE_SET", "RequiredPrivileges", sizeof(wow64::PPRIVILEGE_SET)}, {"PBOOLEAN", "Result", sizeof(wow64::PBOOLEAN)}}},
        {"_ZwPrivilegeObjectAuditAlarm@24", 6, {{"PUNICODE_STRING", "SubsystemName", sizeof(wow64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(wow64::PVOID)}, {"HANDLE", "ClientToken", sizeof(wow64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(wow64::ACCESS_MASK)}, {"PPRIVILEGE_SET", "Privileges", sizeof(wow64::PPRIVILEGE_SET)}, {"BOOLEAN", "AccessGranted", sizeof(wow64::BOOLEAN)}}},
        {"_ZwPropagationFailed@12", 3, {{"HANDLE", "ResourceManagerHandle", sizeof(wow64::HANDLE)}, {"ULONG", "RequestCookie", sizeof(wow64::ULONG)}, {"NTSTATUS", "PropStatus", sizeof(wow64::NTSTATUS)}}},
        {"_ZwProtectVirtualMemory@20", 5, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(wow64::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(wow64::PSIZE_T)}, {"WIN32_PROTECTION_MASK", "NewProtectWin32", sizeof(wow64::WIN32_PROTECTION_MASK)}, {"PULONG", "OldProtect", sizeof(wow64::PULONG)}}},
        {"_ZwPulseEvent@8", 2, {{"HANDLE", "EventHandle", sizeof(wow64::HANDLE)}, {"PLONG", "PreviousState", sizeof(wow64::PLONG)}}},
        {"_ZwQueryAttributesFile@8", 2, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PFILE_BASIC_INFORMATION", "FileInformation", sizeof(wow64::PFILE_BASIC_INFORMATION)}}},
        {"_ZwQueryBootEntryOrder@8", 2, {{"PULONG", "Ids", sizeof(wow64::PULONG)}, {"PULONG", "Count", sizeof(wow64::PULONG)}}},
        {"_ZwQueryBootOptions@8", 2, {{"PBOOT_OPTIONS", "BootOptions", sizeof(wow64::PBOOT_OPTIONS)}, {"PULONG", "BootOptionsLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryDefaultUILanguage@4", 1, {{"LANGID", "STARDefaultUILanguageId", sizeof(wow64::LANGID)}}},
        {"_ZwQueryDirectoryFile@44", 11, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(wow64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(wow64::FILE_INFORMATION_CLASS)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(wow64::BOOLEAN)}, {"PUNICODE_STRING", "FileName", sizeof(wow64::PUNICODE_STRING)}, {"BOOLEAN", "RestartScan", sizeof(wow64::BOOLEAN)}}},
        {"_ZwQueryDirectoryObject@28", 7, {{"HANDLE", "DirectoryHandle", sizeof(wow64::HANDLE)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(wow64::BOOLEAN)}, {"BOOLEAN", "RestartScan", sizeof(wow64::BOOLEAN)}, {"PULONG", "Context", sizeof(wow64::PULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryEaFile@36", 9, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(wow64::BOOLEAN)}, {"PVOID", "EaList", sizeof(wow64::PVOID)}, {"ULONG", "EaListLength", sizeof(wow64::ULONG)}, {"PULONG", "EaIndex", sizeof(wow64::PULONG)}, {"BOOLEAN", "RestartScan", sizeof(wow64::BOOLEAN)}}},
        {"_ZwQueryFullAttributesFile@8", 2, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"PFILE_NETWORK_OPEN_INFORMATION", "FileInformation", sizeof(wow64::PFILE_NETWORK_OPEN_INFORMATION)}}},
        {"_ZwQueryInformationEnlistment@20", 5, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"ENLISTMENT_INFORMATION_CLASS", "EnlistmentInformationClass", sizeof(wow64::ENLISTMENT_INFORMATION_CLASS)}, {"PVOID", "EnlistmentInformation", sizeof(wow64::PVOID)}, {"ULONG", "EnlistmentInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryInformationFile@20", 5, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(wow64::FILE_INFORMATION_CLASS)}}},
        {"_ZwQueryInformationJobObject@20", 5, {{"HANDLE", "JobHandle", sizeof(wow64::HANDLE)}, {"JOBOBJECTINFOCLASS", "JobObjectInformationClass", sizeof(wow64::JOBOBJECTINFOCLASS)}, {"PVOID", "JobObjectInformation", sizeof(wow64::PVOID)}, {"ULONG", "JobObjectInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryInformationPort@20", 5, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(wow64::PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryInformationProcess@20", 5, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PROCESSINFOCLASS", "ProcessInformationClass", sizeof(wow64::PROCESSINFOCLASS)}, {"PVOID", "ProcessInformation", sizeof(wow64::PVOID)}, {"ULONG", "ProcessInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryInformationResourceManager@20", 5, {{"HANDLE", "ResourceManagerHandle", sizeof(wow64::HANDLE)}, {"RESOURCEMANAGER_INFORMATION_CLASS", "ResourceManagerInformationClass", sizeof(wow64::RESOURCEMANAGER_INFORMATION_CLASS)}, {"PVOID", "ResourceManagerInformation", sizeof(wow64::PVOID)}, {"ULONG", "ResourceManagerInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryInformationToken@20", 5, {{"HANDLE", "TokenHandle", sizeof(wow64::HANDLE)}, {"TOKEN_INFORMATION_CLASS", "TokenInformationClass", sizeof(wow64::TOKEN_INFORMATION_CLASS)}, {"PVOID", "TokenInformation", sizeof(wow64::PVOID)}, {"ULONG", "TokenInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryInformationTransaction@20", 5, {{"HANDLE", "TransactionHandle", sizeof(wow64::HANDLE)}, {"TRANSACTION_INFORMATION_CLASS", "TransactionInformationClass", sizeof(wow64::TRANSACTION_INFORMATION_CLASS)}, {"PVOID", "TransactionInformation", sizeof(wow64::PVOID)}, {"ULONG", "TransactionInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryInformationWorkerFactory@20", 5, {{"HANDLE", "WorkerFactoryHandle", sizeof(wow64::HANDLE)}, {"WORKERFACTORYINFOCLASS", "WorkerFactoryInformationClass", sizeof(wow64::WORKERFACTORYINFOCLASS)}, {"PVOID", "WorkerFactoryInformation", sizeof(wow64::PVOID)}, {"ULONG", "WorkerFactoryInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryKey@20", 5, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"KEY_INFORMATION_CLASS", "KeyInformationClass", sizeof(wow64::KEY_INFORMATION_CLASS)}, {"PVOID", "KeyInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PULONG", "ResultLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryPortInformationProcess@0", 0, {}},
        {"_ZwQueryQuotaInformationFile@36", 9, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(wow64::BOOLEAN)}, {"PVOID", "SidList", sizeof(wow64::PVOID)}, {"ULONG", "SidListLength", sizeof(wow64::ULONG)}, {"PULONG", "StartSid", sizeof(wow64::PULONG)}, {"BOOLEAN", "RestartScan", sizeof(wow64::BOOLEAN)}}},
        {"_ZwQuerySection@20", 5, {{"HANDLE", "SectionHandle", sizeof(wow64::HANDLE)}, {"SECTION_INFORMATION_CLASS", "SectionInformationClass", sizeof(wow64::SECTION_INFORMATION_CLASS)}, {"PVOID", "SectionInformation", sizeof(wow64::PVOID)}, {"SIZE_T", "SectionInformationLength", sizeof(wow64::SIZE_T)}, {"PSIZE_T", "ReturnLength", sizeof(wow64::PSIZE_T)}}},
        {"_ZwQuerySecurityAttributesToken@24", 6, {{"HANDLE", "TokenHandle", sizeof(wow64::HANDLE)}, {"PUNICODE_STRING", "Attributes", sizeof(wow64::PUNICODE_STRING)}, {"ULONG", "NumberOfAttributes", sizeof(wow64::ULONG)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQuerySemaphore@20", 5, {{"HANDLE", "SemaphoreHandle", sizeof(wow64::HANDLE)}, {"SEMAPHORE_INFORMATION_CLASS", "SemaphoreInformationClass", sizeof(wow64::SEMAPHORE_INFORMATION_CLASS)}, {"PVOID", "SemaphoreInformation", sizeof(wow64::PVOID)}, {"ULONG", "SemaphoreInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQuerySymbolicLinkObject@12", 3, {{"HANDLE", "LinkHandle", sizeof(wow64::HANDLE)}, {"PUNICODE_STRING", "LinkTarget", sizeof(wow64::PUNICODE_STRING)}, {"PULONG", "ReturnedLength", sizeof(wow64::PULONG)}}},
        {"_ZwQuerySystemEnvironmentValue@16", 4, {{"PUNICODE_STRING", "VariableName", sizeof(wow64::PUNICODE_STRING)}, {"PWSTR", "VariableValue", sizeof(wow64::PWSTR)}, {"USHORT", "ValueLength", sizeof(wow64::USHORT)}, {"PUSHORT", "ReturnLength", sizeof(wow64::PUSHORT)}}},
        {"_ZwQuerySystemEnvironmentValueEx@20", 5, {{"PUNICODE_STRING", "VariableName", sizeof(wow64::PUNICODE_STRING)}, {"LPGUID", "VendorGuid", sizeof(wow64::LPGUID)}, {"PVOID", "Value", sizeof(wow64::PVOID)}, {"PULONG", "ValueLength", sizeof(wow64::PULONG)}, {"PULONG", "Attributes", sizeof(wow64::PULONG)}}},
        {"_ZwQuerySystemInformationEx@24", 6, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(wow64::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "QueryInformation", sizeof(wow64::PVOID)}, {"ULONG", "QueryInformationLength", sizeof(wow64::ULONG)}, {"PVOID", "SystemInformation", sizeof(wow64::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryTimer@20", 5, {{"HANDLE", "TimerHandle", sizeof(wow64::HANDLE)}, {"TIMER_INFORMATION_CLASS", "TimerInformationClass", sizeof(wow64::TIMER_INFORMATION_CLASS)}, {"PVOID", "TimerInformation", sizeof(wow64::PVOID)}, {"ULONG", "TimerInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwQueryValueKey@24", 6, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(wow64::PUNICODE_STRING)}, {"KEY_VALUE_INFORMATION_CLASS", "KeyValueInformationClass", sizeof(wow64::KEY_VALUE_INFORMATION_CLASS)}, {"PVOID", "KeyValueInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"PULONG", "ResultLength", sizeof(wow64::PULONG)}}},
        {"_ZwRaiseException@12", 3, {{"PEXCEPTION_RECORD", "ExceptionRecord", sizeof(wow64::PEXCEPTION_RECORD)}, {"PCONTEXT", "ContextRecord", sizeof(wow64::PCONTEXT)}, {"BOOLEAN", "FirstChance", sizeof(wow64::BOOLEAN)}}},
        {"_ZwRaiseHardError@24", 6, {{"NTSTATUS", "ErrorStatus", sizeof(wow64::NTSTATUS)}, {"ULONG", "NumberOfParameters", sizeof(wow64::ULONG)}, {"ULONG", "UnicodeStringParameterMask", sizeof(wow64::ULONG)}, {"PULONG_PTR", "Parameters", sizeof(wow64::PULONG_PTR)}, {"ULONG", "ValidResponseOptions", sizeof(wow64::ULONG)}, {"PULONG", "Response", sizeof(wow64::PULONG)}}},
        {"_ZwReadOnlyEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwReadRequestData@24", 6, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(wow64::PPORT_MESSAGE)}, {"ULONG", "DataEntryIndex", sizeof(wow64::ULONG)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"SIZE_T", "BufferSize", sizeof(wow64::SIZE_T)}, {"PSIZE_T", "NumberOfBytesRead", sizeof(wow64::PSIZE_T)}}},
        {"_ZwRecoverTransactionManager@4", 1, {{"HANDLE", "TransactionManagerHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwRegisterProtocolAddressInformation@20", 5, {{"HANDLE", "ResourceManager", sizeof(wow64::HANDLE)}, {"PCRM_PROTOCOL_ID", "ProtocolId", sizeof(wow64::PCRM_PROTOCOL_ID)}, {"ULONG", "ProtocolInformationSize", sizeof(wow64::ULONG)}, {"PVOID", "ProtocolInformation", sizeof(wow64::PVOID)}, {"ULONG", "CreateOptions", sizeof(wow64::ULONG)}}},
        {"_ZwRegisterThreadTerminatePort@4", 1, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwReleaseMutant@8", 2, {{"HANDLE", "MutantHandle", sizeof(wow64::HANDLE)}, {"PLONG", "PreviousCount", sizeof(wow64::PLONG)}}},
        {"_ZwReleaseWorkerFactoryWorker@4", 1, {{"HANDLE", "WorkerFactoryHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwRemoveIoCompletion@20", 5, {{"HANDLE", "IoCompletionHandle", sizeof(wow64::HANDLE)}, {"PVOID", "STARKeyContext", sizeof(wow64::PVOID)}, {"PVOID", "STARApcContext", sizeof(wow64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwRemoveIoCompletionEx@24", 6, {{"HANDLE", "IoCompletionHandle", sizeof(wow64::HANDLE)}, {"PFILE_IO_COMPLETION_INFORMATION", "IoCompletionInformation", sizeof(wow64::PFILE_IO_COMPLETION_INFORMATION)}, {"ULONG", "Count", sizeof(wow64::ULONG)}, {"PULONG", "NumEntriesRemoved", sizeof(wow64::PULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}, {"BOOLEAN", "Alertable", sizeof(wow64::BOOLEAN)}}},
        {"_ZwRemoveProcessDebug@8", 2, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "DebugObjectHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwRenameKey@8", 2, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"PUNICODE_STRING", "NewName", sizeof(wow64::PUNICODE_STRING)}}},
        {"_ZwReplaceKey@12", 3, {{"POBJECT_ATTRIBUTES", "NewFile", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"HANDLE", "TargetHandle", sizeof(wow64::HANDLE)}, {"POBJECT_ATTRIBUTES", "OldFile", sizeof(wow64::POBJECT_ATTRIBUTES)}}},
        {"_ZwReplyPort@8", 2, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(wow64::PPORT_MESSAGE)}}},
        {"_ZwRequestPort@8", 2, {{"HANDLE", "PortHandle", sizeof(wow64::HANDLE)}, {"PPORT_MESSAGE", "RequestMessage", sizeof(wow64::PPORT_MESSAGE)}}},
        {"_ZwResetWriteWatch@12", 3, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(wow64::PVOID)}, {"SIZE_T", "RegionSize", sizeof(wow64::SIZE_T)}}},
        {"_ZwResumeProcess@4", 1, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwResumeThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(wow64::PULONG)}}},
        {"_ZwRollbackComplete@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwSetBootEntryOrder@8", 2, {{"PULONG", "Ids", sizeof(wow64::PULONG)}, {"ULONG", "Count", sizeof(wow64::ULONG)}}},
        {"_ZwSetBootOptions@8", 2, {{"PBOOT_OPTIONS", "BootOptions", sizeof(wow64::PBOOT_OPTIONS)}, {"ULONG", "FieldsToChange", sizeof(wow64::ULONG)}}},
        {"_ZwSetContextThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"PCONTEXT", "ThreadContext", sizeof(wow64::PCONTEXT)}}},
        {"_ZwSetDefaultUILanguage@4", 1, {{"LANGID", "DefaultUILanguageId", sizeof(wow64::LANGID)}}},
        {"_ZwSetEaFile@16", 4, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}}},
        {"_ZwSetInformationDebugObject@20", 5, {{"HANDLE", "DebugObjectHandle", sizeof(wow64::HANDLE)}, {"DEBUGOBJECTINFOCLASS", "DebugObjectInformationClass", sizeof(wow64::DEBUGOBJECTINFOCLASS)}, {"PVOID", "DebugInformation", sizeof(wow64::PVOID)}, {"ULONG", "DebugInformationLength", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwSetInformationFile@20", 5, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(wow64::FILE_INFORMATION_CLASS)}}},
        {"_ZwSetInformationJobObject@16", 4, {{"HANDLE", "JobHandle", sizeof(wow64::HANDLE)}, {"JOBOBJECTINFOCLASS", "JobObjectInformationClass", sizeof(wow64::JOBOBJECTINFOCLASS)}, {"PVOID", "JobObjectInformation", sizeof(wow64::PVOID)}, {"ULONG", "JobObjectInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetInformationKey@16", 4, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"KEY_SET_INFORMATION_CLASS", "KeySetInformationClass", sizeof(wow64::KEY_SET_INFORMATION_CLASS)}, {"PVOID", "KeySetInformation", sizeof(wow64::PVOID)}, {"ULONG", "KeySetInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetInformationObject@16", 4, {{"HANDLE", "Handle", sizeof(wow64::HANDLE)}, {"OBJECT_INFORMATION_CLASS", "ObjectInformationClass", sizeof(wow64::OBJECT_INFORMATION_CLASS)}, {"PVOID", "ObjectInformation", sizeof(wow64::PVOID)}, {"ULONG", "ObjectInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetInformationProcess@16", 4, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"PROCESSINFOCLASS", "ProcessInformationClass", sizeof(wow64::PROCESSINFOCLASS)}, {"PVOID", "ProcessInformation", sizeof(wow64::PVOID)}, {"ULONG", "ProcessInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetInformationResourceManager@16", 4, {{"HANDLE", "ResourceManagerHandle", sizeof(wow64::HANDLE)}, {"RESOURCEMANAGER_INFORMATION_CLASS", "ResourceManagerInformationClass", sizeof(wow64::RESOURCEMANAGER_INFORMATION_CLASS)}, {"PVOID", "ResourceManagerInformation", sizeof(wow64::PVOID)}, {"ULONG", "ResourceManagerInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetInformationThread@16", 4, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"THREADINFOCLASS", "ThreadInformationClass", sizeof(wow64::THREADINFOCLASS)}, {"PVOID", "ThreadInformation", sizeof(wow64::PVOID)}, {"ULONG", "ThreadInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetInformationToken@16", 4, {{"HANDLE", "TokenHandle", sizeof(wow64::HANDLE)}, {"TOKEN_INFORMATION_CLASS", "TokenInformationClass", sizeof(wow64::TOKEN_INFORMATION_CLASS)}, {"PVOID", "TokenInformation", sizeof(wow64::PVOID)}, {"ULONG", "TokenInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetInformationTransaction@16", 4, {{"HANDLE", "TransactionHandle", sizeof(wow64::HANDLE)}, {"TRANSACTION_INFORMATION_CLASS", "TransactionInformationClass", sizeof(wow64::TRANSACTION_INFORMATION_CLASS)}, {"PVOID", "TransactionInformation", sizeof(wow64::PVOID)}, {"ULONG", "TransactionInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetInformationTransactionManager@16", 4, {{"HANDLE", "TmHandle", sizeof(wow64::HANDLE)}, {"TRANSACTIONMANAGER_INFORMATION_CLASS", "TransactionManagerInformationClass", sizeof(wow64::TRANSACTIONMANAGER_INFORMATION_CLASS)}, {"PVOID", "TransactionManagerInformation", sizeof(wow64::PVOID)}, {"ULONG", "TransactionManagerInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetInformationWorkerFactory@16", 4, {{"HANDLE", "WorkerFactoryHandle", sizeof(wow64::HANDLE)}, {"WORKERFACTORYINFOCLASS", "WorkerFactoryInformationClass", sizeof(wow64::WORKERFACTORYINFOCLASS)}, {"PVOID", "WorkerFactoryInformation", sizeof(wow64::PVOID)}, {"ULONG", "WorkerFactoryInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetLdtEntries@24", 6, {{"ULONG", "Selector0", sizeof(wow64::ULONG)}, {"ULONG", "Entry0Low", sizeof(wow64::ULONG)}, {"ULONG", "Entry0Hi", sizeof(wow64::ULONG)}, {"ULONG", "Selector1", sizeof(wow64::ULONG)}, {"ULONG", "Entry1Low", sizeof(wow64::ULONG)}, {"ULONG", "Entry1Hi", sizeof(wow64::ULONG)}}},
        {"_ZwSetLowEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwSetLowWaitHighEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwSetQuotaInformationFile@16", 4, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(wow64::PVOID)}, {"ULONG", "Length", sizeof(wow64::ULONG)}}},
        {"_ZwSetSystemEnvironmentValue@8", 2, {{"PUNICODE_STRING", "VariableName", sizeof(wow64::PUNICODE_STRING)}, {"PUNICODE_STRING", "VariableValue", sizeof(wow64::PUNICODE_STRING)}}},
        {"_ZwSetSystemEnvironmentValueEx@20", 5, {{"PUNICODE_STRING", "VariableName", sizeof(wow64::PUNICODE_STRING)}, {"LPGUID", "VendorGuid", sizeof(wow64::LPGUID)}, {"PVOID", "Value", sizeof(wow64::PVOID)}, {"ULONG", "ValueLength", sizeof(wow64::ULONG)}, {"ULONG", "Attributes", sizeof(wow64::ULONG)}}},
        {"_ZwSetSystemInformation@12", 3, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(wow64::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "SystemInformation", sizeof(wow64::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetSystemPowerState@12", 3, {{"POWER_ACTION", "SystemAction", sizeof(wow64::POWER_ACTION)}, {"SYSTEM_POWER_STATE", "MinSystemState", sizeof(wow64::SYSTEM_POWER_STATE)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}}},
        {"_ZwSetSystemTime@8", 2, {{"PLARGE_INTEGER", "SystemTime", sizeof(wow64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "PreviousTime", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwSetThreadExecutionState@8", 2, {{"EXECUTION_STATE", "esFlags", sizeof(wow64::EXECUTION_STATE)}, {"EXECUTION_STATE", "STARPreviousFlags", sizeof(wow64::EXECUTION_STATE)}}},
        {"_ZwSetTimer@28", 7, {{"HANDLE", "TimerHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "DueTime", sizeof(wow64::PLARGE_INTEGER)}, {"PTIMER_APC_ROUTINE", "TimerApcRoutine", sizeof(wow64::PTIMER_APC_ROUTINE)}, {"PVOID", "TimerContext", sizeof(wow64::PVOID)}, {"BOOLEAN", "WakeTimer", sizeof(wow64::BOOLEAN)}, {"LONG", "Period", sizeof(wow64::LONG)}, {"PBOOLEAN", "PreviousState", sizeof(wow64::PBOOLEAN)}}},
        {"_ZwSetTimerEx@16", 4, {{"HANDLE", "TimerHandle", sizeof(wow64::HANDLE)}, {"TIMER_SET_INFORMATION_CLASS", "TimerSetInformationClass", sizeof(wow64::TIMER_SET_INFORMATION_CLASS)}, {"PVOID", "TimerSetInformation", sizeof(wow64::PVOID)}, {"ULONG", "TimerSetInformationLength", sizeof(wow64::ULONG)}}},
        {"_ZwSetUuidSeed@4", 1, {{"PCHAR", "Seed", sizeof(wow64::PCHAR)}}},
        {"_ZwSetValueKey@24", 6, {{"HANDLE", "KeyHandle", sizeof(wow64::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(wow64::PUNICODE_STRING)}, {"ULONG", "TitleIndex", sizeof(wow64::ULONG)}, {"ULONG", "Type", sizeof(wow64::ULONG)}, {"PVOID", "Data", sizeof(wow64::PVOID)}, {"ULONG", "DataSize", sizeof(wow64::ULONG)}}},
        {"_ZwShutdownSystem@4", 1, {{"SHUTDOWN_ACTION", "Action", sizeof(wow64::SHUTDOWN_ACTION)}}},
        {"_ZwSignalAndWaitForSingleObject@16", 4, {{"HANDLE", "SignalHandle", sizeof(wow64::HANDLE)}, {"HANDLE", "WaitHandle", sizeof(wow64::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(wow64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwSinglePhaseReject@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(wow64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwStopProfile@4", 1, {{"HANDLE", "ProfileHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwSuspendProcess@4", 1, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwSuspendThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(wow64::PULONG)}}},
        {"_ZwTerminateJobObject@8", 2, {{"HANDLE", "JobHandle", sizeof(wow64::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(wow64::NTSTATUS)}}},
        {"_ZwTerminateProcess@8", 2, {{"HANDLE", "ProcessHandle", sizeof(wow64::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(wow64::NTSTATUS)}}},
        {"_ZwTerminateThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(wow64::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(wow64::NTSTATUS)}}},
        {"_ZwThawRegistry@0", 0, {}},
        {"_ZwTraceControl@24", 6, {{"ULONG", "FunctionCode", sizeof(wow64::ULONG)}, {"PVOID", "InBuffer", sizeof(wow64::PVOID)}, {"ULONG", "InBufferLen", sizeof(wow64::ULONG)}, {"PVOID", "OutBuffer", sizeof(wow64::PVOID)}, {"ULONG", "OutBufferLen", sizeof(wow64::ULONG)}, {"PULONG", "ReturnLength", sizeof(wow64::PULONG)}}},
        {"_ZwUmsThreadYield@4", 1, {{"PVOID", "SchedulerParam", sizeof(wow64::PVOID)}}},
        {"_ZwUnloadDriver@4", 1, {{"PUNICODE_STRING", "DriverServiceName", sizeof(wow64::PUNICODE_STRING)}}},
        {"_ZwUnloadKey2@8", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(wow64::ULONG)}}},
        {"_ZwUnloadKeyEx@8", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(wow64::POBJECT_ATTRIBUTES)}, {"HANDLE", "Event", sizeof(wow64::HANDLE)}}},
        {"_ZwUnlockFile@20", 5, {{"HANDLE", "FileHandle", sizeof(wow64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(wow64::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(wow64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "Length", sizeof(wow64::PLARGE_INTEGER)}, {"ULONG", "Key", sizeof(wow64::ULONG)}}},
        {"_ZwWaitForSingleObject@12", 3, {{"HANDLE", "Handle", sizeof(wow64::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(wow64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(wow64::PLARGE_INTEGER)}}},
        {"_ZwWaitHighEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(wow64::HANDLE)}}},
        {"_ZwYieldExecution@0", 0, {}},
    }};
}

struct wow64::syscalls32::Data
{
    Data(core::Core& core, std::string_view module);

    core::Core&   core;
    std::string   module;
};

wow64::syscalls32::Data::Data(core::Core& core, std::string_view module)
    : core(core)
    , module(module)
{
}

wow64::syscalls32::syscalls32(core::Core& core, std::string_view module)
    : d_(std::make_unique<Data>(core, module))
{
}

wow64::syscalls32::~syscalls32() = default;

const wow64::syscalls32::callcfgs_t& wow64::syscalls32::callcfgs()
{
    return g_callcfgs;
}

namespace
{
    opt<bpid_t> register_callback_with(wow64::syscalls32::Data& d, bpid_t bpid, proc_t proc, const char* name, const state::Task& on_call)
    {
        const auto addr = symbols::address(d.core, proc, d.module, name);
        if(!addr)
            return FAIL(std::nullopt, "unable to find symbole %s!%s", d.module.data(), name);

        const auto bp = state::break_on_process(d.core, name, proc, *addr, on_call);
        if(!bp)
            return FAIL(std::nullopt, "unable to set breakpoint");

        return state::save_breakpoint_with(d.core, bpid, bp);
    }

    opt<bpid_t> register_callback(wow64::syscalls32::Data& d, proc_t proc, const char* name, const state::Task& on_call)
    {
        const auto bpid = state::acquire_breakpoint_id(d.core);
        return register_callback_with(d, bpid, proc, name, on_call);
    }

    template <typename T>
    T arg(core::Core& core, size_t i)
    {
        const auto arg = functions::read_arg(core, i);
        if(!arg)
            return {};

        T value = {};
        static_assert(sizeof value <= sizeof arg->val, "invalid size");
        memcpy(&value, &arg->val, sizeof value);
        return value;
    }
}

opt<bpid_t> wow64::syscalls32::register_NtAcceptConnectPort(proc_t proc, const on_NtAcceptConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAcceptConnectPort@24", [=]
    {
        auto& core = d_->core;

        const auto PortHandle        = arg<wow64::PHANDLE>(core, 0);
        const auto PortContext       = arg<wow64::PVOID>(core, 1);
        const auto ConnectionRequest = arg<wow64::PPORT_MESSAGE>(core, 2);
        const auto AcceptConnection  = arg<wow64::BOOLEAN>(core, 3);
        const auto ServerView        = arg<wow64::PPORT_VIEW>(core, 4);
        const auto ClientView        = arg<wow64::PREMOTE_PORT_VIEW>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[0]);

        on_func(PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAccessCheck(proc_t proc, const on_NtAccessCheck_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAccessCheck@32", [=]
    {
        auto& core = d_->core;

        const auto SecurityDescriptor = arg<wow64::PSECURITY_DESCRIPTOR>(core, 0);
        const auto ClientToken        = arg<wow64::HANDLE>(core, 1);
        const auto DesiredAccess      = arg<wow64::ACCESS_MASK>(core, 2);
        const auto GenericMapping     = arg<wow64::PGENERIC_MAPPING>(core, 3);
        const auto PrivilegeSet       = arg<wow64::PPRIVILEGE_SET>(core, 4);
        const auto PrivilegeSetLength = arg<wow64::PULONG>(core, 5);
        const auto GrantedAccess      = arg<wow64::PACCESS_MASK>(core, 6);
        const auto AccessStatus       = arg<wow64::PNTSTATUS>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[1]);

        on_func(SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAccessCheckByType(proc_t proc, const on_NtAccessCheckByType_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAccessCheckByType@44", [=]
    {
        auto& core = d_->core;

        const auto SecurityDescriptor   = arg<wow64::PSECURITY_DESCRIPTOR>(core, 0);
        const auto PrincipalSelfSid     = arg<wow64::PSID>(core, 1);
        const auto ClientToken          = arg<wow64::HANDLE>(core, 2);
        const auto DesiredAccess        = arg<wow64::ACCESS_MASK>(core, 3);
        const auto ObjectTypeList       = arg<wow64::POBJECT_TYPE_LIST>(core, 4);
        const auto ObjectTypeListLength = arg<wow64::ULONG>(core, 5);
        const auto GenericMapping       = arg<wow64::PGENERIC_MAPPING>(core, 6);
        const auto PrivilegeSet         = arg<wow64::PPRIVILEGE_SET>(core, 7);
        const auto PrivilegeSetLength   = arg<wow64::PULONG>(core, 8);
        const auto GrantedAccess        = arg<wow64::PACCESS_MASK>(core, 9);
        const auto AccessStatus         = arg<wow64::PNTSTATUS>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[2]);

        on_func(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAccessCheckByTypeAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAccessCheckByTypeAndAuditAlarm@64", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName        = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<wow64::PVOID>(core, 1);
        const auto ObjectTypeName       = arg<wow64::PUNICODE_STRING>(core, 2);
        const auto ObjectName           = arg<wow64::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor   = arg<wow64::PSECURITY_DESCRIPTOR>(core, 4);
        const auto PrincipalSelfSid     = arg<wow64::PSID>(core, 5);
        const auto DesiredAccess        = arg<wow64::ACCESS_MASK>(core, 6);
        const auto AuditType            = arg<wow64::AUDIT_EVENT_TYPE>(core, 7);
        const auto Flags                = arg<wow64::ULONG>(core, 8);
        const auto ObjectTypeList       = arg<wow64::POBJECT_TYPE_LIST>(core, 9);
        const auto ObjectTypeListLength = arg<wow64::ULONG>(core, 10);
        const auto GenericMapping       = arg<wow64::PGENERIC_MAPPING>(core, 11);
        const auto ObjectCreation       = arg<wow64::BOOLEAN>(core, 12);
        const auto GrantedAccess        = arg<wow64::PACCESS_MASK>(core, 13);
        const auto AccessStatus         = arg<wow64::PNTSTATUS>(core, 14);
        const auto GenerateOnClose      = arg<wow64::PBOOLEAN>(core, 15);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[3]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAccessCheckByTypeResultList(proc_t proc, const on_NtAccessCheckByTypeResultList_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAccessCheckByTypeResultList@44", [=]
    {
        auto& core = d_->core;

        const auto SecurityDescriptor   = arg<wow64::PSECURITY_DESCRIPTOR>(core, 0);
        const auto PrincipalSelfSid     = arg<wow64::PSID>(core, 1);
        const auto ClientToken          = arg<wow64::HANDLE>(core, 2);
        const auto DesiredAccess        = arg<wow64::ACCESS_MASK>(core, 3);
        const auto ObjectTypeList       = arg<wow64::POBJECT_TYPE_LIST>(core, 4);
        const auto ObjectTypeListLength = arg<wow64::ULONG>(core, 5);
        const auto GenericMapping       = arg<wow64::PGENERIC_MAPPING>(core, 6);
        const auto PrivilegeSet         = arg<wow64::PPRIVILEGE_SET>(core, 7);
        const auto PrivilegeSetLength   = arg<wow64::PULONG>(core, 8);
        const auto GrantedAccess        = arg<wow64::PACCESS_MASK>(core, 9);
        const auto AccessStatus         = arg<wow64::PNTSTATUS>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[4]);

        on_func(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAccessCheckByTypeResultListAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAccessCheckByTypeResultListAndAuditAlarm@64", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName        = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<wow64::PVOID>(core, 1);
        const auto ObjectTypeName       = arg<wow64::PUNICODE_STRING>(core, 2);
        const auto ObjectName           = arg<wow64::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor   = arg<wow64::PSECURITY_DESCRIPTOR>(core, 4);
        const auto PrincipalSelfSid     = arg<wow64::PSID>(core, 5);
        const auto DesiredAccess        = arg<wow64::ACCESS_MASK>(core, 6);
        const auto AuditType            = arg<wow64::AUDIT_EVENT_TYPE>(core, 7);
        const auto Flags                = arg<wow64::ULONG>(core, 8);
        const auto ObjectTypeList       = arg<wow64::POBJECT_TYPE_LIST>(core, 9);
        const auto ObjectTypeListLength = arg<wow64::ULONG>(core, 10);
        const auto GenericMapping       = arg<wow64::PGENERIC_MAPPING>(core, 11);
        const auto ObjectCreation       = arg<wow64::BOOLEAN>(core, 12);
        const auto GrantedAccess        = arg<wow64::PACCESS_MASK>(core, 13);
        const auto AccessStatus         = arg<wow64::PNTSTATUS>(core, 14);
        const auto GenerateOnClose      = arg<wow64::PBOOLEAN>(core, 15);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[5]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAddAtom(proc_t proc, const on_NtAddAtom_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAddAtom@12", [=]
    {
        auto& core = d_->core;

        const auto AtomName = arg<wow64::PWSTR>(core, 0);
        const auto Length   = arg<wow64::ULONG>(core, 1);
        const auto Atom     = arg<wow64::PRTL_ATOM>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[6]);

        on_func(AtomName, Length, Atom);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAddDriverEntry(proc_t proc, const on_NtAddDriverEntry_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAddDriverEntry@8", [=]
    {
        auto& core = d_->core;

        const auto DriverEntry = arg<wow64::PEFI_DRIVER_ENTRY>(core, 0);
        const auto Id          = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[7]);

        on_func(DriverEntry, Id);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlertResumeThread(proc_t proc, const on_NtAlertResumeThread_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlertResumeThread@8", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle         = arg<wow64::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[8]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlertThread(proc_t proc, const on_NtAlertThread_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlertThread@4", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[9]);

        on_func(ThreadHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAllocateReserveObject(proc_t proc, const on_NtAllocateReserveObject_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAllocateReserveObject@12", [=]
    {
        auto& core = d_->core;

        const auto MemoryReserveHandle = arg<wow64::PHANDLE>(core, 0);
        const auto ObjectAttributes    = arg<wow64::POBJECT_ATTRIBUTES>(core, 1);
        const auto Type                = arg<wow64::MEMORY_RESERVE_TYPE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[10]);

        on_func(MemoryReserveHandle, ObjectAttributes, Type);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAllocateUserPhysicalPages(proc_t proc, const on_NtAllocateUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAllocateUserPhysicalPages@12", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<wow64::HANDLE>(core, 0);
        const auto NumberOfPages = arg<wow64::PULONG_PTR>(core, 1);
        const auto UserPfnArra   = arg<wow64::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[11]);

        on_func(ProcessHandle, NumberOfPages, UserPfnArra);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAllocateUuids(proc_t proc, const on_NtAllocateUuids_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAllocateUuids@16", [=]
    {
        auto& core = d_->core;

        const auto Time     = arg<wow64::PULARGE_INTEGER>(core, 0);
        const auto Range    = arg<wow64::PULONG>(core, 1);
        const auto Sequence = arg<wow64::PULONG>(core, 2);
        const auto Seed     = arg<wow64::PCHAR>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[12]);

        on_func(Time, Range, Sequence, Seed);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAllocateVirtualMemory(proc_t proc, const on_NtAllocateVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAllocateVirtualMemory@24", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<wow64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<wow64::PVOID>(core, 1);
        const auto ZeroBits        = arg<wow64::ULONG_PTR>(core, 2);
        const auto RegionSize      = arg<wow64::PSIZE_T>(core, 3);
        const auto AllocationType  = arg<wow64::ULONG>(core, 4);
        const auto Protect         = arg<wow64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[13]);

        on_func(ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlpcAcceptConnectPort(proc_t proc, const on_NtAlpcAcceptConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlpcAcceptConnectPort@36", [=]
    {
        auto& core = d_->core;

        const auto PortHandle                  = arg<wow64::PHANDLE>(core, 0);
        const auto ConnectionPortHandle        = arg<wow64::HANDLE>(core, 1);
        const auto Flags                       = arg<wow64::ULONG>(core, 2);
        const auto ObjectAttributes            = arg<wow64::POBJECT_ATTRIBUTES>(core, 3);
        const auto PortAttributes              = arg<wow64::PALPC_PORT_ATTRIBUTES>(core, 4);
        const auto PortContext                 = arg<wow64::PVOID>(core, 5);
        const auto ConnectionRequest           = arg<wow64::PPORT_MESSAGE>(core, 6);
        const auto ConnectionMessageAttributes = arg<wow64::PALPC_MESSAGE_ATTRIBUTES>(core, 7);
        const auto AcceptConnection            = arg<wow64::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[14]);

        on_func(PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlpcCreatePortSection(proc_t proc, const on_NtAlpcCreatePortSection_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlpcCreatePortSection@24", [=]
    {
        auto& core = d_->core;

        const auto PortHandle        = arg<wow64::HANDLE>(core, 0);
        const auto Flags             = arg<wow64::ULONG>(core, 1);
        const auto SectionHandle     = arg<wow64::HANDLE>(core, 2);
        const auto SectionSize       = arg<wow64::SIZE_T>(core, 3);
        const auto AlpcSectionHandle = arg<wow64::PALPC_HANDLE>(core, 4);
        const auto ActualSectionSize = arg<wow64::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[15]);

        on_func(PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlpcDeleteResourceReserve(proc_t proc, const on_NtAlpcDeleteResourceReserve_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlpcDeleteResourceReserve@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<wow64::HANDLE>(core, 0);
        const auto Flags      = arg<wow64::ULONG>(core, 1);
        const auto ResourceId = arg<wow64::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[16]);

        on_func(PortHandle, Flags, ResourceId);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlpcDeleteSectionView(proc_t proc, const on_NtAlpcDeleteSectionView_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlpcDeleteSectionView@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<wow64::HANDLE>(core, 0);
        const auto Flags      = arg<wow64::ULONG>(core, 1);
        const auto ViewBase   = arg<wow64::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[17]);

        on_func(PortHandle, Flags, ViewBase);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlpcDeleteSecurityContext(proc_t proc, const on_NtAlpcDeleteSecurityContext_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlpcDeleteSecurityContext@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle    = arg<wow64::HANDLE>(core, 0);
        const auto Flags         = arg<wow64::ULONG>(core, 1);
        const auto ContextHandle = arg<wow64::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[18]);

        on_func(PortHandle, Flags, ContextHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlpcDisconnectPort(proc_t proc, const on_NtAlpcDisconnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlpcDisconnectPort@8", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<wow64::HANDLE>(core, 0);
        const auto Flags      = arg<wow64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[19]);

        on_func(PortHandle, Flags);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlpcRevokeSecurityContext(proc_t proc, const on_NtAlpcRevokeSecurityContext_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlpcRevokeSecurityContext@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle    = arg<wow64::HANDLE>(core, 0);
        const auto Flags         = arg<wow64::ULONG>(core, 1);
        const auto ContextHandle = arg<wow64::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[20]);

        on_func(PortHandle, Flags, ContextHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlpcSendWaitReceivePort(proc_t proc, const on_NtAlpcSendWaitReceivePort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlpcSendWaitReceivePort@32", [=]
    {
        auto& core = d_->core;

        const auto PortHandle               = arg<wow64::HANDLE>(core, 0);
        const auto Flags                    = arg<wow64::ULONG>(core, 1);
        const auto SendMessage              = arg<wow64::PPORT_MESSAGE>(core, 2);
        const auto SendMessageAttributes    = arg<wow64::PALPC_MESSAGE_ATTRIBUTES>(core, 3);
        const auto ReceiveMessage           = arg<wow64::PPORT_MESSAGE>(core, 4);
        const auto BufferLength             = arg<wow64::PULONG>(core, 5);
        const auto ReceiveMessageAttributes = arg<wow64::PALPC_MESSAGE_ATTRIBUTES>(core, 6);
        const auto Timeout                  = arg<wow64::PLARGE_INTEGER>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[21]);

        on_func(PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtAlpcSetInformation(proc_t proc, const on_NtAlpcSetInformation_fn& on_func)
{
    return register_callback(*d_, proc, "_NtAlpcSetInformation@16", [=]
    {
        auto& core = d_->core;

        const auto PortHandle           = arg<wow64::HANDLE>(core, 0);
        const auto PortInformationClass = arg<wow64::ALPC_PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<wow64::PVOID>(core, 2);
        const auto Length               = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[22]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtApphelpCacheControl(proc_t proc, const on_NtApphelpCacheControl_fn& on_func)
{
    return register_callback(*d_, proc, "_NtApphelpCacheControl@8", [=]
    {
        auto& core = d_->core;

        const auto type = arg<wow64::APPHELPCOMMAND>(core, 0);
        const auto buf  = arg<wow64::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[23]);

        on_func(type, buf);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCancelIoFileEx(proc_t proc, const on_NtCancelIoFileEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCancelIoFileEx@12", [=]
    {
        auto& core = d_->core;

        const auto FileHandle        = arg<wow64::HANDLE>(core, 0);
        const auto IoRequestToCancel = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto IoStatusBlock     = arg<wow64::PIO_STATUS_BLOCK>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[24]);

        on_func(FileHandle, IoRequestToCancel, IoStatusBlock);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCancelSynchronousIoFile(proc_t proc, const on_NtCancelSynchronousIoFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCancelSynchronousIoFile@12", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle      = arg<wow64::HANDLE>(core, 0);
        const auto IoRequestToCancel = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto IoStatusBlock     = arg<wow64::PIO_STATUS_BLOCK>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[25]);

        on_func(ThreadHandle, IoRequestToCancel, IoStatusBlock);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtClearEvent(proc_t proc, const on_NtClearEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtClearEvent@4", [=]
    {
        auto& core = d_->core;

        const auto EventHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[26]);

        on_func(EventHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtClose(proc_t proc, const on_NtClose_fn& on_func)
{
    return register_callback(*d_, proc, "_NtClose@4", [=]
    {
        auto& core = d_->core;

        const auto Handle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[27]);

        on_func(Handle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCommitEnlistment(proc_t proc, const on_NtCommitEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCommitEnlistment@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[28]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCommitTransaction(proc_t proc, const on_NtCommitTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCommitTransaction@8", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle = arg<wow64::HANDLE>(core, 0);
        const auto Wait              = arg<wow64::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[29]);

        on_func(TransactionHandle, Wait);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCompactKeys(proc_t proc, const on_NtCompactKeys_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCompactKeys@8", [=]
    {
        auto& core = d_->core;

        const auto Count    = arg<wow64::ULONG>(core, 0);
        const auto KeyArray = arg<wow64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[30]);

        on_func(Count, KeyArray);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCompleteConnectPort(proc_t proc, const on_NtCompleteConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCompleteConnectPort@4", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[31]);

        on_func(PortHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtConnectPort(proc_t proc, const on_NtConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtConnectPort@32", [=]
    {
        auto& core = d_->core;

        const auto PortHandle                  = arg<wow64::PHANDLE>(core, 0);
        const auto PortName                    = arg<wow64::PUNICODE_STRING>(core, 1);
        const auto SecurityQos                 = arg<wow64::PSECURITY_QUALITY_OF_SERVICE>(core, 2);
        const auto ClientView                  = arg<wow64::PPORT_VIEW>(core, 3);
        const auto ServerView                  = arg<wow64::PREMOTE_PORT_VIEW>(core, 4);
        const auto MaxMessageLength            = arg<wow64::PULONG>(core, 5);
        const auto ConnectionInformation       = arg<wow64::PVOID>(core, 6);
        const auto ConnectionInformationLength = arg<wow64::PULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[32]);

        on_func(PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateEvent(proc_t proc, const on_NtCreateEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateEvent@20", [=]
    {
        auto& core = d_->core;

        const auto EventHandle      = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto EventType        = arg<wow64::EVENT_TYPE>(core, 3);
        const auto InitialState     = arg<wow64::BOOLEAN>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[33]);

        on_func(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateEventPair(proc_t proc, const on_NtCreateEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateEventPair@12", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle  = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[34]);

        on_func(EventPairHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateFile(proc_t proc, const on_NtCreateFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateFile@44", [=]
    {
        auto& core = d_->core;

        const auto FileHandle        = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock     = arg<wow64::PIO_STATUS_BLOCK>(core, 3);
        const auto AllocationSize    = arg<wow64::PLARGE_INTEGER>(core, 4);
        const auto FileAttributes    = arg<wow64::ULONG>(core, 5);
        const auto ShareAccess       = arg<wow64::ULONG>(core, 6);
        const auto CreateDisposition = arg<wow64::ULONG>(core, 7);
        const auto CreateOptions     = arg<wow64::ULONG>(core, 8);
        const auto EaBuffer          = arg<wow64::PVOID>(core, 9);
        const auto EaLength          = arg<wow64::ULONG>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[35]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateIoCompletion(proc_t proc, const on_NtCreateIoCompletion_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateIoCompletion@16", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Count              = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[36]);

        on_func(IoCompletionHandle, DesiredAccess, ObjectAttributes, Count);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateJobSet(proc_t proc, const on_NtCreateJobSet_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateJobSet@12", [=]
    {
        auto& core = d_->core;

        const auto NumJob     = arg<wow64::ULONG>(core, 0);
        const auto UserJobSet = arg<wow64::PJOB_SET_ARRAY>(core, 1);
        const auto Flags      = arg<wow64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[37]);

        on_func(NumJob, UserJobSet, Flags);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateKeyTransacted(proc_t proc, const on_NtCreateKeyTransacted_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateKeyTransacted@32", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle         = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto TitleIndex        = arg<wow64::ULONG>(core, 3);
        const auto Class             = arg<wow64::PUNICODE_STRING>(core, 4);
        const auto CreateOptions     = arg<wow64::ULONG>(core, 5);
        const auto TransactionHandle = arg<wow64::HANDLE>(core, 6);
        const auto Disposition       = arg<wow64::PULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[38]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreatePagingFile(proc_t proc, const on_NtCreatePagingFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreatePagingFile@16", [=]
    {
        auto& core = d_->core;

        const auto PageFileName = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto MinimumSize  = arg<wow64::PLARGE_INTEGER>(core, 1);
        const auto MaximumSize  = arg<wow64::PLARGE_INTEGER>(core, 2);
        const auto Priority     = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[39]);

        on_func(PageFileName, MinimumSize, MaximumSize, Priority);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreatePrivateNamespace(proc_t proc, const on_NtCreatePrivateNamespace_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreatePrivateNamespace@16", [=]
    {
        auto& core = d_->core;

        const auto NamespaceHandle    = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto BoundaryDescriptor = arg<wow64::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[40]);

        on_func(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateProfileEx(proc_t proc, const on_NtCreateProfileEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateProfileEx@40", [=]
    {
        auto& core = d_->core;

        const auto ProfileHandle      = arg<wow64::PHANDLE>(core, 0);
        const auto Process            = arg<wow64::HANDLE>(core, 1);
        const auto ProfileBase        = arg<wow64::PVOID>(core, 2);
        const auto ProfileSize        = arg<wow64::SIZE_T>(core, 3);
        const auto BucketSize         = arg<wow64::ULONG>(core, 4);
        const auto Buffer             = arg<wow64::PULONG>(core, 5);
        const auto BufferSize         = arg<wow64::ULONG>(core, 6);
        const auto ProfileSource      = arg<wow64::KPROFILE_SOURCE>(core, 7);
        const auto GroupAffinityCount = arg<wow64::ULONG>(core, 8);
        const auto GroupAffinity      = arg<wow64::PGROUP_AFFINITY>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[41]);

        on_func(ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateSection(proc_t proc, const on_NtCreateSection_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateSection@28", [=]
    {
        auto& core = d_->core;

        const auto SectionHandle         = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes      = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto MaximumSize           = arg<wow64::PLARGE_INTEGER>(core, 3);
        const auto SectionPageProtection = arg<wow64::ULONG>(core, 4);
        const auto AllocationAttributes  = arg<wow64::ULONG>(core, 5);
        const auto FileHandle            = arg<wow64::HANDLE>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[42]);

        on_func(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateSemaphore(proc_t proc, const on_NtCreateSemaphore_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateSemaphore@20", [=]
    {
        auto& core = d_->core;

        const auto SemaphoreHandle  = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto InitialCount     = arg<wow64::LONG>(core, 3);
        const auto MaximumCount     = arg<wow64::LONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[43]);

        on_func(SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateThread(proc_t proc, const on_NtCreateThread_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateThread@32", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle     = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ProcessHandle    = arg<wow64::HANDLE>(core, 3);
        const auto ClientId         = arg<wow64::PCLIENT_ID>(core, 4);
        const auto ThreadContext    = arg<wow64::PCONTEXT>(core, 5);
        const auto InitialTeb       = arg<wow64::PINITIAL_TEB>(core, 6);
        const auto CreateSuspended  = arg<wow64::BOOLEAN>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[44]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateThreadEx(proc_t proc, const on_NtCreateThreadEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateThreadEx@44", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle     = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ProcessHandle    = arg<wow64::HANDLE>(core, 3);
        const auto StartRoutine     = arg<wow64::PVOID>(core, 4);
        const auto Argument         = arg<wow64::PVOID>(core, 5);
        const auto CreateFlags      = arg<wow64::ULONG>(core, 6);
        const auto ZeroBits         = arg<wow64::ULONG_PTR>(core, 7);
        const auto StackSize        = arg<wow64::SIZE_T>(core, 8);
        const auto MaximumStackSize = arg<wow64::SIZE_T>(core, 9);
        const auto AttributeList    = arg<wow64::PPS_ATTRIBUTE_LIST>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[45]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateToken(proc_t proc, const on_NtCreateToken_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateToken@52", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle      = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto TokenType        = arg<wow64::TOKEN_TYPE>(core, 3);
        const auto AuthenticationId = arg<wow64::PLUID>(core, 4);
        const auto ExpirationTime   = arg<wow64::PLARGE_INTEGER>(core, 5);
        const auto User             = arg<wow64::PTOKEN_USER>(core, 6);
        const auto Groups           = arg<wow64::PTOKEN_GROUPS>(core, 7);
        const auto Privileges       = arg<wow64::PTOKEN_PRIVILEGES>(core, 8);
        const auto Owner            = arg<wow64::PTOKEN_OWNER>(core, 9);
        const auto PrimaryGroup     = arg<wow64::PTOKEN_PRIMARY_GROUP>(core, 10);
        const auto DefaultDacl      = arg<wow64::PTOKEN_DEFAULT_DACL>(core, 11);
        const auto TokenSource      = arg<wow64::PTOKEN_SOURCE>(core, 12);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[46]);

        on_func(TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateTransaction(proc_t proc, const on_NtCreateTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateTransaction@40", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Uow               = arg<wow64::LPGUID>(core, 3);
        const auto TmHandle          = arg<wow64::HANDLE>(core, 4);
        const auto CreateOptions     = arg<wow64::ULONG>(core, 5);
        const auto IsolationLevel    = arg<wow64::ULONG>(core, 6);
        const auto IsolationFlags    = arg<wow64::ULONG>(core, 7);
        const auto Timeout           = arg<wow64::PLARGE_INTEGER>(core, 8);
        const auto Description       = arg<wow64::PUNICODE_STRING>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[47]);

        on_func(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateUserProcess(proc_t proc, const on_NtCreateUserProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateUserProcess@44", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle           = arg<wow64::PHANDLE>(core, 0);
        const auto ThreadHandle            = arg<wow64::PHANDLE>(core, 1);
        const auto ProcessDesiredAccess    = arg<wow64::ACCESS_MASK>(core, 2);
        const auto ThreadDesiredAccess     = arg<wow64::ACCESS_MASK>(core, 3);
        const auto ProcessObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 4);
        const auto ThreadObjectAttributes  = arg<wow64::POBJECT_ATTRIBUTES>(core, 5);
        const auto ProcessFlags            = arg<wow64::ULONG>(core, 6);
        const auto ThreadFlags             = arg<wow64::ULONG>(core, 7);
        const auto ProcessParameters       = arg<wow64::PRTL_USER_PROCESS_PARAMETERS>(core, 8);
        const auto CreateInfo              = arg<wow64::PPROCESS_CREATE_INFO>(core, 9);
        const auto AttributeList           = arg<wow64::PPROCESS_ATTRIBUTE_LIST>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[48]);

        on_func(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtCreateWorkerFactory(proc_t proc, const on_NtCreateWorkerFactory_fn& on_func)
{
    return register_callback(*d_, proc, "_NtCreateWorkerFactory@40", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandleReturn = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess             = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes          = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto CompletionPortHandle      = arg<wow64::HANDLE>(core, 3);
        const auto WorkerProcessHandle       = arg<wow64::HANDLE>(core, 4);
        const auto StartRoutine              = arg<wow64::PVOID>(core, 5);
        const auto StartParameter            = arg<wow64::PVOID>(core, 6);
        const auto MaxThreadCount            = arg<wow64::ULONG>(core, 7);
        const auto StackReserve              = arg<wow64::SIZE_T>(core, 8);
        const auto StackCommit               = arg<wow64::SIZE_T>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[49]);

        on_func(WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtDebugActiveProcess(proc_t proc, const on_NtDebugActiveProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_NtDebugActiveProcess@8", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle     = arg<wow64::HANDLE>(core, 0);
        const auto DebugObjectHandle = arg<wow64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[50]);

        on_func(ProcessHandle, DebugObjectHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtDeleteBootEntry(proc_t proc, const on_NtDeleteBootEntry_fn& on_func)
{
    return register_callback(*d_, proc, "_NtDeleteBootEntry@4", [=]
    {
        auto& core = d_->core;

        const auto Id = arg<wow64::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[51]);

        on_func(Id);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtDeleteFile(proc_t proc, const on_NtDeleteFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtDeleteFile@4", [=]
    {
        auto& core = d_->core;

        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[52]);

        on_func(ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtDeleteObjectAuditAlarm(proc_t proc, const on_NtDeleteObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "_NtDeleteObjectAuditAlarm@12", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName   = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto HandleId        = arg<wow64::PVOID>(core, 1);
        const auto GenerateOnClose = arg<wow64::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[53]);

        on_func(SubsystemName, HandleId, GenerateOnClose);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtDeletePrivateNamespace(proc_t proc, const on_NtDeletePrivateNamespace_fn& on_func)
{
    return register_callback(*d_, proc, "_NtDeletePrivateNamespace@4", [=]
    {
        auto& core = d_->core;

        const auto NamespaceHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[54]);

        on_func(NamespaceHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtDeleteValueKey(proc_t proc, const on_NtDeleteValueKey_fn& on_func)
{
    return register_callback(*d_, proc, "_NtDeleteValueKey@8", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle = arg<wow64::HANDLE>(core, 0);
        const auto ValueName = arg<wow64::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[55]);

        on_func(KeyHandle, ValueName);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtDisableLastKnownGood(proc_t proc, const on_NtDisableLastKnownGood_fn& on_func)
{
    return register_callback(*d_, proc, "_NtDisableLastKnownGood@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[56]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_NtDisplayString(proc_t proc, const on_NtDisplayString_fn& on_func)
{
    return register_callback(*d_, proc, "_NtDisplayString@4", [=]
    {
        auto& core = d_->core;

        const auto String = arg<wow64::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[57]);

        on_func(String);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtDuplicateToken(proc_t proc, const on_NtDuplicateToken_fn& on_func)
{
    return register_callback(*d_, proc, "_NtDuplicateToken@24", [=]
    {
        auto& core = d_->core;

        const auto ExistingTokenHandle = arg<wow64::HANDLE>(core, 0);
        const auto DesiredAccess       = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes    = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto EffectiveOnly       = arg<wow64::BOOLEAN>(core, 3);
        const auto TokenType           = arg<wow64::TOKEN_TYPE>(core, 4);
        const auto NewTokenHandle      = arg<wow64::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[58]);

        on_func(ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtEnableLastKnownGood(proc_t proc, const on_NtEnableLastKnownGood_fn& on_func)
{
    return register_callback(*d_, proc, "_NtEnableLastKnownGood@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[59]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_NtEnumerateDriverEntries(proc_t proc, const on_NtEnumerateDriverEntries_fn& on_func)
{
    return register_callback(*d_, proc, "_NtEnumerateDriverEntries@8", [=]
    {
        auto& core = d_->core;

        const auto Buffer       = arg<wow64::PVOID>(core, 0);
        const auto BufferLength = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[60]);

        on_func(Buffer, BufferLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtEnumerateValueKey(proc_t proc, const on_NtEnumerateValueKey_fn& on_func)
{
    return register_callback(*d_, proc, "_NtEnumerateValueKey@24", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle                = arg<wow64::HANDLE>(core, 0);
        const auto Index                    = arg<wow64::ULONG>(core, 1);
        const auto KeyValueInformationClass = arg<wow64::KEY_VALUE_INFORMATION_CLASS>(core, 2);
        const auto KeyValueInformation      = arg<wow64::PVOID>(core, 3);
        const auto Length                   = arg<wow64::ULONG>(core, 4);
        const auto ResultLength             = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[61]);

        on_func(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtFilterToken(proc_t proc, const on_NtFilterToken_fn& on_func)
{
    return register_callback(*d_, proc, "_NtFilterToken@24", [=]
    {
        auto& core = d_->core;

        const auto ExistingTokenHandle = arg<wow64::HANDLE>(core, 0);
        const auto Flags               = arg<wow64::ULONG>(core, 1);
        const auto SidsToDisable       = arg<wow64::PTOKEN_GROUPS>(core, 2);
        const auto PrivilegesToDelete  = arg<wow64::PTOKEN_PRIVILEGES>(core, 3);
        const auto RestrictedSids      = arg<wow64::PTOKEN_GROUPS>(core, 4);
        const auto NewTokenHandle      = arg<wow64::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[62]);

        on_func(ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtFindAtom(proc_t proc, const on_NtFindAtom_fn& on_func)
{
    return register_callback(*d_, proc, "_NtFindAtom@12", [=]
    {
        auto& core = d_->core;

        const auto AtomName = arg<wow64::PWSTR>(core, 0);
        const auto Length   = arg<wow64::ULONG>(core, 1);
        const auto Atom     = arg<wow64::PRTL_ATOM>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[63]);

        on_func(AtomName, Length, Atom);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtFlushKey(proc_t proc, const on_NtFlushKey_fn& on_func)
{
    return register_callback(*d_, proc, "_NtFlushKey@4", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[64]);

        on_func(KeyHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtFlushWriteBuffer(proc_t proc, const on_NtFlushWriteBuffer_fn& on_func)
{
    return register_callback(*d_, proc, "_NtFlushWriteBuffer@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[65]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_NtFreeUserPhysicalPages(proc_t proc, const on_NtFreeUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, proc, "_NtFreeUserPhysicalPages@12", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<wow64::HANDLE>(core, 0);
        const auto NumberOfPages = arg<wow64::PULONG_PTR>(core, 1);
        const auto UserPfnArra   = arg<wow64::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[66]);

        on_func(ProcessHandle, NumberOfPages, UserPfnArra);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtFreeVirtualMemory(proc_t proc, const on_NtFreeVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "_NtFreeVirtualMemory@16", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<wow64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<wow64::PVOID>(core, 1);
        const auto RegionSize      = arg<wow64::PSIZE_T>(core, 2);
        const auto FreeType        = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[67]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, FreeType);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtFreezeRegistry(proc_t proc, const on_NtFreezeRegistry_fn& on_func)
{
    return register_callback(*d_, proc, "_NtFreezeRegistry@4", [=]
    {
        auto& core = d_->core;

        const auto TimeOutInSeconds = arg<wow64::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[68]);

        on_func(TimeOutInSeconds);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtFsControlFile(proc_t proc, const on_NtFsControlFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtFsControlFile@40", [=]
    {
        auto& core = d_->core;

        const auto FileHandle         = arg<wow64::HANDLE>(core, 0);
        const auto Event              = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine         = arg<wow64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext         = arg<wow64::PVOID>(core, 3);
        const auto IoStatusBlock      = arg<wow64::PIO_STATUS_BLOCK>(core, 4);
        const auto IoControlCode      = arg<wow64::ULONG>(core, 5);
        const auto InputBuffer        = arg<wow64::PVOID>(core, 6);
        const auto InputBufferLength  = arg<wow64::ULONG>(core, 7);
        const auto OutputBuffer       = arg<wow64::PVOID>(core, 8);
        const auto OutputBufferLength = arg<wow64::ULONG>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[69]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtGetContextThread(proc_t proc, const on_NtGetContextThread_fn& on_func)
{
    return register_callback(*d_, proc, "_NtGetContextThread@8", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle  = arg<wow64::HANDLE>(core, 0);
        const auto ThreadContext = arg<wow64::PCONTEXT>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[70]);

        on_func(ThreadHandle, ThreadContext);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtGetCurrentProcessorNumber(proc_t proc, const on_NtGetCurrentProcessorNumber_fn& on_func)
{
    return register_callback(*d_, proc, "_NtGetCurrentProcessorNumber@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[71]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_NtGetDevicePowerState(proc_t proc, const on_NtGetDevicePowerState_fn& on_func)
{
    return register_callback(*d_, proc, "_NtGetDevicePowerState@8", [=]
    {
        auto& core = d_->core;

        const auto Device    = arg<wow64::HANDLE>(core, 0);
        const auto STARState = arg<wow64::DEVICE_POWER_STATE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[72]);

        on_func(Device, STARState);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtGetMUIRegistryInfo(proc_t proc, const on_NtGetMUIRegistryInfo_fn& on_func)
{
    return register_callback(*d_, proc, "_NtGetMUIRegistryInfo@12", [=]
    {
        auto& core = d_->core;

        const auto Flags    = arg<wow64::ULONG>(core, 0);
        const auto DataSize = arg<wow64::PULONG>(core, 1);
        const auto Data     = arg<wow64::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[73]);

        on_func(Flags, DataSize, Data);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtGetNlsSectionPtr(proc_t proc, const on_NtGetNlsSectionPtr_fn& on_func)
{
    return register_callback(*d_, proc, "_NtGetNlsSectionPtr@20", [=]
    {
        auto& core = d_->core;

        const auto SectionType        = arg<wow64::ULONG>(core, 0);
        const auto SectionData        = arg<wow64::ULONG>(core, 1);
        const auto ContextData        = arg<wow64::PVOID>(core, 2);
        const auto STARSectionPointer = arg<wow64::PVOID>(core, 3);
        const auto SectionSize        = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[74]);

        on_func(SectionType, SectionData, ContextData, STARSectionPointer, SectionSize);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtGetWriteWatch(proc_t proc, const on_NtGetWriteWatch_fn& on_func)
{
    return register_callback(*d_, proc, "_NtGetWriteWatch@28", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle             = arg<wow64::HANDLE>(core, 0);
        const auto Flags                     = arg<wow64::ULONG>(core, 1);
        const auto BaseAddress               = arg<wow64::PVOID>(core, 2);
        const auto RegionSize                = arg<wow64::SIZE_T>(core, 3);
        const auto STARUserAddressArray      = arg<wow64::PVOID>(core, 4);
        const auto EntriesInUserAddressArray = arg<wow64::PULONG_PTR>(core, 5);
        const auto Granularity               = arg<wow64::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[75]);

        on_func(ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtImpersonateAnonymousToken(proc_t proc, const on_NtImpersonateAnonymousToken_fn& on_func)
{
    return register_callback(*d_, proc, "_NtImpersonateAnonymousToken@4", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[76]);

        on_func(ThreadHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtInitializeNlsFiles(proc_t proc, const on_NtInitializeNlsFiles_fn& on_func)
{
    return register_callback(*d_, proc, "_NtInitializeNlsFiles@12", [=]
    {
        auto& core = d_->core;

        const auto STARBaseAddress        = arg<wow64::PVOID>(core, 0);
        const auto DefaultLocaleId        = arg<wow64::PLCID>(core, 1);
        const auto DefaultCasingTableSize = arg<wow64::PLARGE_INTEGER>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[77]);

        on_func(STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtInitiatePowerAction(proc_t proc, const on_NtInitiatePowerAction_fn& on_func)
{
    return register_callback(*d_, proc, "_NtInitiatePowerAction@16", [=]
    {
        auto& core = d_->core;

        const auto SystemAction   = arg<wow64::POWER_ACTION>(core, 0);
        const auto MinSystemState = arg<wow64::SYSTEM_POWER_STATE>(core, 1);
        const auto Flags          = arg<wow64::ULONG>(core, 2);
        const auto Asynchronous   = arg<wow64::BOOLEAN>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[78]);

        on_func(SystemAction, MinSystemState, Flags, Asynchronous);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtIsSystemResumeAutomatic(proc_t proc, const on_NtIsSystemResumeAutomatic_fn& on_func)
{
    return register_callback(*d_, proc, "_NtIsSystemResumeAutomatic@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[79]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_NtLoadDriver(proc_t proc, const on_NtLoadDriver_fn& on_func)
{
    return register_callback(*d_, proc, "_NtLoadDriver@4", [=]
    {
        auto& core = d_->core;

        const auto DriverServiceName = arg<wow64::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[80]);

        on_func(DriverServiceName);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtLoadKey(proc_t proc, const on_NtLoadKey_fn& on_func)
{
    return register_callback(*d_, proc, "_NtLoadKey@8", [=]
    {
        auto& core = d_->core;

        const auto TargetKey  = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile = arg<wow64::POBJECT_ATTRIBUTES>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[81]);

        on_func(TargetKey, SourceFile);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtLoadKey2(proc_t proc, const on_NtLoadKey2_fn& on_func)
{
    return register_callback(*d_, proc, "_NtLoadKey2@12", [=]
    {
        auto& core = d_->core;

        const auto TargetKey  = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile = arg<wow64::POBJECT_ATTRIBUTES>(core, 1);
        const auto Flags      = arg<wow64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[82]);

        on_func(TargetKey, SourceFile, Flags);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtLoadKeyEx(proc_t proc, const on_NtLoadKeyEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtLoadKeyEx@32", [=]
    {
        auto& core = d_->core;

        const auto TargetKey         = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile        = arg<wow64::POBJECT_ATTRIBUTES>(core, 1);
        const auto Flags             = arg<wow64::ULONG>(core, 2);
        const auto TrustClassKey     = arg<wow64::HANDLE>(core, 3);
        const auto Reserved          = arg<wow64::PVOID>(core, 4);
        const auto ObjectContext     = arg<wow64::PVOID>(core, 5);
        const auto CallbackReserverd = arg<wow64::PVOID>(core, 6);
        const auto IoStatusBlock     = arg<wow64::PVOID>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[83]);

        on_func(TargetKey, SourceFile, Flags, TrustClassKey, Reserved, ObjectContext, CallbackReserverd, IoStatusBlock);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtLockFile(proc_t proc, const on_NtLockFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtLockFile@40", [=]
    {
        auto& core = d_->core;

        const auto FileHandle      = arg<wow64::HANDLE>(core, 0);
        const auto Event           = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine      = arg<wow64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext      = arg<wow64::PVOID>(core, 3);
        const auto IoStatusBlock   = arg<wow64::PIO_STATUS_BLOCK>(core, 4);
        const auto ByteOffset      = arg<wow64::PLARGE_INTEGER>(core, 5);
        const auto Length          = arg<wow64::PLARGE_INTEGER>(core, 6);
        const auto Key             = arg<wow64::ULONG>(core, 7);
        const auto FailImmediately = arg<wow64::BOOLEAN>(core, 8);
        const auto ExclusiveLock   = arg<wow64::BOOLEAN>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[84]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtLockRegistryKey(proc_t proc, const on_NtLockRegistryKey_fn& on_func)
{
    return register_callback(*d_, proc, "_NtLockRegistryKey@4", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[85]);

        on_func(KeyHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtMakeTemporaryObject(proc_t proc, const on_NtMakeTemporaryObject_fn& on_func)
{
    return register_callback(*d_, proc, "_NtMakeTemporaryObject@4", [=]
    {
        auto& core = d_->core;

        const auto Handle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[86]);

        on_func(Handle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtMapUserPhysicalPages(proc_t proc, const on_NtMapUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, proc, "_NtMapUserPhysicalPages@12", [=]
    {
        auto& core = d_->core;

        const auto VirtualAddress = arg<wow64::PVOID>(core, 0);
        const auto NumberOfPages  = arg<wow64::ULONG_PTR>(core, 1);
        const auto UserPfnArra    = arg<wow64::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[87]);

        on_func(VirtualAddress, NumberOfPages, UserPfnArra);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtModifyBootEntry(proc_t proc, const on_NtModifyBootEntry_fn& on_func)
{
    return register_callback(*d_, proc, "_NtModifyBootEntry@4", [=]
    {
        auto& core = d_->core;

        const auto BootEntry = arg<wow64::PBOOT_ENTRY>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[88]);

        on_func(BootEntry);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtNotifyChangeDirectoryFile(proc_t proc, const on_NtNotifyChangeDirectoryFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtNotifyChangeDirectoryFile@36", [=]
    {
        auto& core = d_->core;

        const auto FileHandle       = arg<wow64::HANDLE>(core, 0);
        const auto Event            = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine       = arg<wow64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext       = arg<wow64::PVOID>(core, 3);
        const auto IoStatusBlock    = arg<wow64::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer           = arg<wow64::PVOID>(core, 5);
        const auto Length           = arg<wow64::ULONG>(core, 6);
        const auto CompletionFilter = arg<wow64::ULONG>(core, 7);
        const auto WatchTree        = arg<wow64::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[89]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtNotifyChangeKey(proc_t proc, const on_NtNotifyChangeKey_fn& on_func)
{
    return register_callback(*d_, proc, "_NtNotifyChangeKey@40", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle        = arg<wow64::HANDLE>(core, 0);
        const auto Event            = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine       = arg<wow64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext       = arg<wow64::PVOID>(core, 3);
        const auto IoStatusBlock    = arg<wow64::PIO_STATUS_BLOCK>(core, 4);
        const auto CompletionFilter = arg<wow64::ULONG>(core, 5);
        const auto WatchTree        = arg<wow64::BOOLEAN>(core, 6);
        const auto Buffer           = arg<wow64::PVOID>(core, 7);
        const auto BufferSize       = arg<wow64::ULONG>(core, 8);
        const auto Asynchronous     = arg<wow64::BOOLEAN>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[90]);

        on_func(KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtNotifyChangeMultipleKeys(proc_t proc, const on_NtNotifyChangeMultipleKeys_fn& on_func)
{
    return register_callback(*d_, proc, "_NtNotifyChangeMultipleKeys@48", [=]
    {
        auto& core = d_->core;

        const auto MasterKeyHandle  = arg<wow64::HANDLE>(core, 0);
        const auto Count            = arg<wow64::ULONG>(core, 1);
        const auto SlaveObjects     = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Event            = arg<wow64::HANDLE>(core, 3);
        const auto ApcRoutine       = arg<wow64::PIO_APC_ROUTINE>(core, 4);
        const auto ApcContext       = arg<wow64::PVOID>(core, 5);
        const auto IoStatusBlock    = arg<wow64::PIO_STATUS_BLOCK>(core, 6);
        const auto CompletionFilter = arg<wow64::ULONG>(core, 7);
        const auto WatchTree        = arg<wow64::BOOLEAN>(core, 8);
        const auto Buffer           = arg<wow64::PVOID>(core, 9);
        const auto BufferSize       = arg<wow64::ULONG>(core, 10);
        const auto Asynchronous     = arg<wow64::BOOLEAN>(core, 11);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[91]);

        on_func(MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtNotifyChangeSession(proc_t proc, const on_NtNotifyChangeSession_fn& on_func)
{
    return register_callback(*d_, proc, "_NtNotifyChangeSession@32", [=]
    {
        auto& core = d_->core;

        const auto Session         = arg<wow64::HANDLE>(core, 0);
        const auto IoStateSequence = arg<wow64::ULONG>(core, 1);
        const auto Reserved        = arg<wow64::PVOID>(core, 2);
        const auto Action          = arg<wow64::ULONG>(core, 3);
        const auto IoState         = arg<wow64::IO_SESSION_STATE>(core, 4);
        const auto IoState2        = arg<wow64::IO_SESSION_STATE>(core, 5);
        const auto Buffer          = arg<wow64::PVOID>(core, 6);
        const auto BufferSize      = arg<wow64::ULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[92]);

        on_func(Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenEvent(proc_t proc, const on_NtOpenEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenEvent@12", [=]
    {
        auto& core = d_->core;

        const auto EventHandle      = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[93]);

        on_func(EventHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenEventPair(proc_t proc, const on_NtOpenEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenEventPair@12", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle  = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[94]);

        on_func(EventPairHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenFile(proc_t proc, const on_NtOpenFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenFile@24", [=]
    {
        auto& core = d_->core;

        const auto FileHandle       = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock    = arg<wow64::PIO_STATUS_BLOCK>(core, 3);
        const auto ShareAccess      = arg<wow64::ULONG>(core, 4);
        const auto OpenOptions      = arg<wow64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[95]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenKeyTransacted(proc_t proc, const on_NtOpenKeyTransacted_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenKeyTransacted@16", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle         = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto TransactionHandle = arg<wow64::HANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[96]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenKeyTransactedEx(proc_t proc, const on_NtOpenKeyTransactedEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenKeyTransactedEx@20", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle         = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto OpenOptions       = arg<wow64::ULONG>(core, 3);
        const auto TransactionHandle = arg<wow64::HANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[97]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenKeyedEvent(proc_t proc, const on_NtOpenKeyedEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenKeyedEvent@12", [=]
    {
        auto& core = d_->core;

        const auto KeyedEventHandle = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[98]);

        on_func(KeyedEventHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenMutant(proc_t proc, const on_NtOpenMutant_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenMutant@12", [=]
    {
        auto& core = d_->core;

        const auto MutantHandle     = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[99]);

        on_func(MutantHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenPrivateNamespace(proc_t proc, const on_NtOpenPrivateNamespace_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenPrivateNamespace@16", [=]
    {
        auto& core = d_->core;

        const auto NamespaceHandle    = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto BoundaryDescriptor = arg<wow64::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[100]);

        on_func(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenSection(proc_t proc, const on_NtOpenSection_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenSection@12", [=]
    {
        auto& core = d_->core;

        const auto SectionHandle    = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[101]);

        on_func(SectionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenSemaphore(proc_t proc, const on_NtOpenSemaphore_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenSemaphore@12", [=]
    {
        auto& core = d_->core;

        const auto SemaphoreHandle  = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[102]);

        on_func(SemaphoreHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenSession(proc_t proc, const on_NtOpenSession_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenSession@12", [=]
    {
        auto& core = d_->core;

        const auto SessionHandle    = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[103]);

        on_func(SessionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenSymbolicLinkObject(proc_t proc, const on_NtOpenSymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenSymbolicLinkObject@12", [=]
    {
        auto& core = d_->core;

        const auto LinkHandle       = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[104]);

        on_func(LinkHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenThreadToken(proc_t proc, const on_NtOpenThreadToken_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenThreadToken@16", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle  = arg<wow64::HANDLE>(core, 0);
        const auto DesiredAccess = arg<wow64::ACCESS_MASK>(core, 1);
        const auto OpenAsSelf    = arg<wow64::BOOLEAN>(core, 2);
        const auto TokenHandle   = arg<wow64::PHANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[105]);

        on_func(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtOpenThreadTokenEx(proc_t proc, const on_NtOpenThreadTokenEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtOpenThreadTokenEx@20", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle     = arg<wow64::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto OpenAsSelf       = arg<wow64::BOOLEAN>(core, 2);
        const auto HandleAttributes = arg<wow64::ULONG>(core, 3);
        const auto TokenHandle      = arg<wow64::PHANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[106]);

        on_func(ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtPlugPlayControl(proc_t proc, const on_NtPlugPlayControl_fn& on_func)
{
    return register_callback(*d_, proc, "_NtPlugPlayControl@12", [=]
    {
        auto& core = d_->core;

        const auto PnPControlClass      = arg<wow64::PLUGPLAY_CONTROL_CLASS>(core, 0);
        const auto PnPControlData       = arg<wow64::PVOID>(core, 1);
        const auto PnPControlDataLength = arg<wow64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[107]);

        on_func(PnPControlClass, PnPControlData, PnPControlDataLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtPrePrepareEnlistment(proc_t proc, const on_NtPrePrepareEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "_NtPrePrepareEnlistment@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[108]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtPrepareComplete(proc_t proc, const on_NtPrepareComplete_fn& on_func)
{
    return register_callback(*d_, proc, "_NtPrepareComplete@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[109]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtPrivilegedServiceAuditAlarm(proc_t proc, const on_NtPrivilegedServiceAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "_NtPrivilegedServiceAuditAlarm@20", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto ServiceName   = arg<wow64::PUNICODE_STRING>(core, 1);
        const auto ClientToken   = arg<wow64::HANDLE>(core, 2);
        const auto Privileges    = arg<wow64::PPRIVILEGE_SET>(core, 3);
        const auto AccessGranted = arg<wow64::BOOLEAN>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[110]);

        on_func(SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtPropagationComplete(proc_t proc, const on_NtPropagationComplete_fn& on_func)
{
    return register_callback(*d_, proc, "_NtPropagationComplete@16", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle = arg<wow64::HANDLE>(core, 0);
        const auto RequestCookie         = arg<wow64::ULONG>(core, 1);
        const auto BufferLength          = arg<wow64::ULONG>(core, 2);
        const auto Buffer                = arg<wow64::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[111]);

        on_func(ResourceManagerHandle, RequestCookie, BufferLength, Buffer);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryDebugFilterState(proc_t proc, const on_NtQueryDebugFilterState_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryDebugFilterState@8", [=]
    {
        auto& core = d_->core;

        const auto ComponentId = arg<wow64::ULONG>(core, 0);
        const auto Level       = arg<wow64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[112]);

        on_func(ComponentId, Level);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryDefaultLocale(proc_t proc, const on_NtQueryDefaultLocale_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryDefaultLocale@8", [=]
    {
        auto& core = d_->core;

        const auto UserProfile     = arg<wow64::BOOLEAN>(core, 0);
        const auto DefaultLocaleId = arg<wow64::PLCID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[113]);

        on_func(UserProfile, DefaultLocaleId);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryDriverEntryOrder(proc_t proc, const on_NtQueryDriverEntryOrder_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryDriverEntryOrder@8", [=]
    {
        auto& core = d_->core;

        const auto Ids   = arg<wow64::PULONG>(core, 0);
        const auto Count = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[114]);

        on_func(Ids, Count);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryEvent(proc_t proc, const on_NtQueryEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryEvent@20", [=]
    {
        auto& core = d_->core;

        const auto EventHandle            = arg<wow64::HANDLE>(core, 0);
        const auto EventInformationClass  = arg<wow64::EVENT_INFORMATION_CLASS>(core, 1);
        const auto EventInformation       = arg<wow64::PVOID>(core, 2);
        const auto EventInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength           = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[115]);

        on_func(EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryInformationAtom(proc_t proc, const on_NtQueryInformationAtom_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryInformationAtom@20", [=]
    {
        auto& core = d_->core;

        const auto Atom                  = arg<wow64::RTL_ATOM>(core, 0);
        const auto InformationClass      = arg<wow64::ATOM_INFORMATION_CLASS>(core, 1);
        const auto AtomInformation       = arg<wow64::PVOID>(core, 2);
        const auto AtomInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength          = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[116]);

        on_func(Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryInformationThread(proc_t proc, const on_NtQueryInformationThread_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryInformationThread@20", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle            = arg<wow64::HANDLE>(core, 0);
        const auto ThreadInformationClass  = arg<wow64::THREADINFOCLASS>(core, 1);
        const auto ThreadInformation       = arg<wow64::PVOID>(core, 2);
        const auto ThreadInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength            = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[117]);

        on_func(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryInformationTransactionManager(proc_t proc, const on_NtQueryInformationTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryInformationTransactionManager@20", [=]
    {
        auto& core = d_->core;

        const auto TransactionManagerHandle            = arg<wow64::HANDLE>(core, 0);
        const auto TransactionManagerInformationClass  = arg<wow64::TRANSACTIONMANAGER_INFORMATION_CLASS>(core, 1);
        const auto TransactionManagerInformation       = arg<wow64::PVOID>(core, 2);
        const auto TransactionManagerInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength                        = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[118]);

        on_func(TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryInstallUILanguage(proc_t proc, const on_NtQueryInstallUILanguage_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryInstallUILanguage@4", [=]
    {
        auto& core = d_->core;

        const auto STARInstallUILanguageId = arg<wow64::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[119]);

        on_func(STARInstallUILanguageId);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryIntervalProfile(proc_t proc, const on_NtQueryIntervalProfile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryIntervalProfile@8", [=]
    {
        auto& core = d_->core;

        const auto ProfileSource = arg<wow64::KPROFILE_SOURCE>(core, 0);
        const auto Interval      = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[120]);

        on_func(ProfileSource, Interval);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryIoCompletion(proc_t proc, const on_NtQueryIoCompletion_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryIoCompletion@20", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle            = arg<wow64::HANDLE>(core, 0);
        const auto IoCompletionInformationClass  = arg<wow64::IO_COMPLETION_INFORMATION_CLASS>(core, 1);
        const auto IoCompletionInformation       = arg<wow64::PVOID>(core, 2);
        const auto IoCompletionInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength                  = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[121]);

        on_func(IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryLicenseValue(proc_t proc, const on_NtQueryLicenseValue_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryLicenseValue@20", [=]
    {
        auto& core = d_->core;

        const auto Name           = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto Type           = arg<wow64::PULONG>(core, 1);
        const auto Buffer         = arg<wow64::PVOID>(core, 2);
        const auto Length         = arg<wow64::ULONG>(core, 3);
        const auto ReturnedLength = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[122]);

        on_func(Name, Type, Buffer, Length, ReturnedLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryMultipleValueKey(proc_t proc, const on_NtQueryMultipleValueKey_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryMultipleValueKey@24", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle            = arg<wow64::HANDLE>(core, 0);
        const auto ValueEntries         = arg<wow64::PKEY_VALUE_ENTRY>(core, 1);
        const auto EntryCount           = arg<wow64::ULONG>(core, 2);
        const auto ValueBuffer          = arg<wow64::PVOID>(core, 3);
        const auto BufferLength         = arg<wow64::PULONG>(core, 4);
        const auto RequiredBufferLength = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[123]);

        on_func(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryMutant(proc_t proc, const on_NtQueryMutant_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryMutant@20", [=]
    {
        auto& core = d_->core;

        const auto MutantHandle            = arg<wow64::HANDLE>(core, 0);
        const auto MutantInformationClass  = arg<wow64::MUTANT_INFORMATION_CLASS>(core, 1);
        const auto MutantInformation       = arg<wow64::PVOID>(core, 2);
        const auto MutantInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength            = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[124]);

        on_func(MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryObject(proc_t proc, const on_NtQueryObject_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryObject@20", [=]
    {
        auto& core = d_->core;

        const auto Handle                  = arg<wow64::HANDLE>(core, 0);
        const auto ObjectInformationClass  = arg<wow64::OBJECT_INFORMATION_CLASS>(core, 1);
        const auto ObjectInformation       = arg<wow64::PVOID>(core, 2);
        const auto ObjectInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength            = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[125]);

        on_func(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryOpenSubKeys(proc_t proc, const on_NtQueryOpenSubKeys_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryOpenSubKeys@8", [=]
    {
        auto& core = d_->core;

        const auto TargetKey   = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);
        const auto HandleCount = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[126]);

        on_func(TargetKey, HandleCount);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryOpenSubKeysEx(proc_t proc, const on_NtQueryOpenSubKeysEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryOpenSubKeysEx@16", [=]
    {
        auto& core = d_->core;

        const auto TargetKey    = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);
        const auto BufferLength = arg<wow64::ULONG>(core, 1);
        const auto Buffer       = arg<wow64::PVOID>(core, 2);
        const auto RequiredSize = arg<wow64::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[127]);

        on_func(TargetKey, BufferLength, Buffer, RequiredSize);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryPerformanceCounter(proc_t proc, const on_NtQueryPerformanceCounter_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryPerformanceCounter@8", [=]
    {
        auto& core = d_->core;

        const auto PerformanceCounter   = arg<wow64::PLARGE_INTEGER>(core, 0);
        const auto PerformanceFrequency = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[128]);

        on_func(PerformanceCounter, PerformanceFrequency);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQuerySecurityObject(proc_t proc, const on_NtQuerySecurityObject_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQuerySecurityObject@20", [=]
    {
        auto& core = d_->core;

        const auto Handle              = arg<wow64::HANDLE>(core, 0);
        const auto SecurityInformation = arg<wow64::SECURITY_INFORMATION>(core, 1);
        const auto SecurityDescriptor  = arg<wow64::PSECURITY_DESCRIPTOR>(core, 2);
        const auto Length              = arg<wow64::ULONG>(core, 3);
        const auto LengthNeeded        = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[129]);

        on_func(Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQuerySystemInformation(proc_t proc, const on_NtQuerySystemInformation_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQuerySystemInformation@16", [=]
    {
        auto& core = d_->core;

        const auto SystemInformationClass  = arg<wow64::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto SystemInformation       = arg<wow64::PVOID>(core, 1);
        const auto SystemInformationLength = arg<wow64::ULONG>(core, 2);
        const auto ReturnLength            = arg<wow64::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[130]);

        on_func(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQuerySystemTime(proc_t proc, const on_NtQuerySystemTime_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQuerySystemTime@4", [=]
    {
        auto& core = d_->core;

        const auto SystemTime = arg<wow64::PLARGE_INTEGER>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[131]);

        on_func(SystemTime);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryTimerResolution(proc_t proc, const on_NtQueryTimerResolution_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryTimerResolution@12", [=]
    {
        auto& core = d_->core;

        const auto MaximumTime = arg<wow64::PULONG>(core, 0);
        const auto MinimumTime = arg<wow64::PULONG>(core, 1);
        const auto CurrentTime = arg<wow64::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[132]);

        on_func(MaximumTime, MinimumTime, CurrentTime);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryVirtualMemory(proc_t proc, const on_NtQueryVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryVirtualMemory@24", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle           = arg<wow64::HANDLE>(core, 0);
        const auto BaseAddress             = arg<wow64::PVOID>(core, 1);
        const auto MemoryInformationClass  = arg<wow64::MEMORY_INFORMATION_CLASS>(core, 2);
        const auto MemoryInformation       = arg<wow64::PVOID>(core, 3);
        const auto MemoryInformationLength = arg<wow64::SIZE_T>(core, 4);
        const auto ReturnLength            = arg<wow64::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[133]);

        on_func(ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueryVolumeInformationFile(proc_t proc, const on_NtQueryVolumeInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueryVolumeInformationFile@20", [=]
    {
        auto& core = d_->core;

        const auto FileHandle         = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock      = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto FsInformation      = arg<wow64::PVOID>(core, 2);
        const auto Length             = arg<wow64::ULONG>(core, 3);
        const auto FsInformationClass = arg<wow64::FS_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[134]);

        on_func(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueueApcThread(proc_t proc, const on_NtQueueApcThread_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueueApcThread@20", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle = arg<wow64::HANDLE>(core, 0);
        const auto ApcRoutine   = arg<wow64::PPS_APC_ROUTINE>(core, 1);
        const auto ApcArgument1 = arg<wow64::PVOID>(core, 2);
        const auto ApcArgument2 = arg<wow64::PVOID>(core, 3);
        const auto ApcArgument3 = arg<wow64::PVOID>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[135]);

        on_func(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtQueueApcThreadEx(proc_t proc, const on_NtQueueApcThreadEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtQueueApcThreadEx@24", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle         = arg<wow64::HANDLE>(core, 0);
        const auto UserApcReserveHandle = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine           = arg<wow64::PPS_APC_ROUTINE>(core, 2);
        const auto ApcArgument1         = arg<wow64::PVOID>(core, 3);
        const auto ApcArgument2         = arg<wow64::PVOID>(core, 4);
        const auto ApcArgument3         = arg<wow64::PVOID>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[136]);

        on_func(ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtReadFile(proc_t proc, const on_NtReadFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtReadFile@36", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<wow64::HANDLE>(core, 0);
        const auto Event         = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<wow64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<wow64::PVOID>(core, 3);
        const auto IoStatusBlock = arg<wow64::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer        = arg<wow64::PVOID>(core, 5);
        const auto Length        = arg<wow64::ULONG>(core, 6);
        const auto ByteOffset    = arg<wow64::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<wow64::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[137]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtReadFileScatter(proc_t proc, const on_NtReadFileScatter_fn& on_func)
{
    return register_callback(*d_, proc, "_NtReadFileScatter@36", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<wow64::HANDLE>(core, 0);
        const auto Event         = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<wow64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<wow64::PVOID>(core, 3);
        const auto IoStatusBlock = arg<wow64::PIO_STATUS_BLOCK>(core, 4);
        const auto SegmentArray  = arg<wow64::PFILE_SEGMENT_ELEMENT>(core, 5);
        const auto Length        = arg<wow64::ULONG>(core, 6);
        const auto ByteOffset    = arg<wow64::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<wow64::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[138]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtReadVirtualMemory(proc_t proc, const on_NtReadVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "_NtReadVirtualMemory@20", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle     = arg<wow64::HANDLE>(core, 0);
        const auto BaseAddress       = arg<wow64::PVOID>(core, 1);
        const auto Buffer            = arg<wow64::PVOID>(core, 2);
        const auto BufferSize        = arg<wow64::SIZE_T>(core, 3);
        const auto NumberOfBytesRead = arg<wow64::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[139]);

        on_func(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtRecoverEnlistment(proc_t proc, const on_NtRecoverEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "_NtRecoverEnlistment@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto EnlistmentKey    = arg<wow64::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[140]);

        on_func(EnlistmentHandle, EnlistmentKey);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtRecoverResourceManager(proc_t proc, const on_NtRecoverResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "_NtRecoverResourceManager@4", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[141]);

        on_func(ResourceManagerHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtReleaseKeyedEvent(proc_t proc, const on_NtReleaseKeyedEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtReleaseKeyedEvent@16", [=]
    {
        auto& core = d_->core;

        const auto KeyedEventHandle = arg<wow64::HANDLE>(core, 0);
        const auto KeyValue         = arg<wow64::PVOID>(core, 1);
        const auto Alertable        = arg<wow64::BOOLEAN>(core, 2);
        const auto Timeout          = arg<wow64::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[142]);

        on_func(KeyedEventHandle, KeyValue, Alertable, Timeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtReleaseSemaphore(proc_t proc, const on_NtReleaseSemaphore_fn& on_func)
{
    return register_callback(*d_, proc, "_NtReleaseSemaphore@12", [=]
    {
        auto& core = d_->core;

        const auto SemaphoreHandle = arg<wow64::HANDLE>(core, 0);
        const auto ReleaseCount    = arg<wow64::LONG>(core, 1);
        const auto PreviousCount   = arg<wow64::PLONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[143]);

        on_func(SemaphoreHandle, ReleaseCount, PreviousCount);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtRenameTransactionManager(proc_t proc, const on_NtRenameTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "_NtRenameTransactionManager@8", [=]
    {
        auto& core = d_->core;

        const auto LogFileName                    = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto ExistingTransactionManagerGuid = arg<wow64::LPGUID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[144]);

        on_func(LogFileName, ExistingTransactionManagerGuid);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtReplacePartitionUnit(proc_t proc, const on_NtReplacePartitionUnit_fn& on_func)
{
    return register_callback(*d_, proc, "_NtReplacePartitionUnit@12", [=]
    {
        auto& core = d_->core;

        const auto TargetInstancePath = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto SpareInstancePath  = arg<wow64::PUNICODE_STRING>(core, 1);
        const auto Flags              = arg<wow64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[145]);

        on_func(TargetInstancePath, SpareInstancePath, Flags);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtReplyWaitReceivePort(proc_t proc, const on_NtReplyWaitReceivePort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtReplyWaitReceivePort@16", [=]
    {
        auto& core = d_->core;

        const auto PortHandle      = arg<wow64::HANDLE>(core, 0);
        const auto STARPortContext = arg<wow64::PVOID>(core, 1);
        const auto ReplyMessage    = arg<wow64::PPORT_MESSAGE>(core, 2);
        const auto ReceiveMessage  = arg<wow64::PPORT_MESSAGE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[146]);

        on_func(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtReplyWaitReceivePortEx(proc_t proc, const on_NtReplyWaitReceivePortEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtReplyWaitReceivePortEx@20", [=]
    {
        auto& core = d_->core;

        const auto PortHandle      = arg<wow64::HANDLE>(core, 0);
        const auto STARPortContext = arg<wow64::PVOID>(core, 1);
        const auto ReplyMessage    = arg<wow64::PPORT_MESSAGE>(core, 2);
        const auto ReceiveMessage  = arg<wow64::PPORT_MESSAGE>(core, 3);
        const auto Timeout         = arg<wow64::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[147]);

        on_func(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtReplyWaitReplyPort(proc_t proc, const on_NtReplyWaitReplyPort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtReplyWaitReplyPort@8", [=]
    {
        auto& core = d_->core;

        const auto PortHandle   = arg<wow64::HANDLE>(core, 0);
        const auto ReplyMessage = arg<wow64::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[148]);

        on_func(PortHandle, ReplyMessage);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtRequestWaitReplyPort(proc_t proc, const on_NtRequestWaitReplyPort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtRequestWaitReplyPort@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle     = arg<wow64::HANDLE>(core, 0);
        const auto RequestMessage = arg<wow64::PPORT_MESSAGE>(core, 1);
        const auto ReplyMessage   = arg<wow64::PPORT_MESSAGE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[149]);

        on_func(PortHandle, RequestMessage, ReplyMessage);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtResetEvent(proc_t proc, const on_NtResetEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtResetEvent@8", [=]
    {
        auto& core = d_->core;

        const auto EventHandle   = arg<wow64::HANDLE>(core, 0);
        const auto PreviousState = arg<wow64::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[150]);

        on_func(EventHandle, PreviousState);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtRestoreKey(proc_t proc, const on_NtRestoreKey_fn& on_func)
{
    return register_callback(*d_, proc, "_NtRestoreKey@12", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle  = arg<wow64::HANDLE>(core, 0);
        const auto FileHandle = arg<wow64::HANDLE>(core, 1);
        const auto Flags      = arg<wow64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[151]);

        on_func(KeyHandle, FileHandle, Flags);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtRollbackEnlistment(proc_t proc, const on_NtRollbackEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "_NtRollbackEnlistment@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[152]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtRollbackTransaction(proc_t proc, const on_NtRollbackTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "_NtRollbackTransaction@8", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle = arg<wow64::HANDLE>(core, 0);
        const auto Wait              = arg<wow64::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[153]);

        on_func(TransactionHandle, Wait);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtRollforwardTransactionManager(proc_t proc, const on_NtRollforwardTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "_NtRollforwardTransactionManager@8", [=]
    {
        auto& core = d_->core;

        const auto TransactionManagerHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock           = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[154]);

        on_func(TransactionManagerHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSaveKey(proc_t proc, const on_NtSaveKey_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSaveKey@8", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle  = arg<wow64::HANDLE>(core, 0);
        const auto FileHandle = arg<wow64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[155]);

        on_func(KeyHandle, FileHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSaveKeyEx(proc_t proc, const on_NtSaveKeyEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSaveKeyEx@12", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle  = arg<wow64::HANDLE>(core, 0);
        const auto FileHandle = arg<wow64::HANDLE>(core, 1);
        const auto Format     = arg<wow64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[156]);

        on_func(KeyHandle, FileHandle, Format);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSaveMergedKeys(proc_t proc, const on_NtSaveMergedKeys_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSaveMergedKeys@12", [=]
    {
        auto& core = d_->core;

        const auto HighPrecedenceKeyHandle = arg<wow64::HANDLE>(core, 0);
        const auto LowPrecedenceKeyHandle  = arg<wow64::HANDLE>(core, 1);
        const auto FileHandle              = arg<wow64::HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[157]);

        on_func(HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSecureConnectPort(proc_t proc, const on_NtSecureConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSecureConnectPort@36", [=]
    {
        auto& core = d_->core;

        const auto PortHandle                  = arg<wow64::PHANDLE>(core, 0);
        const auto PortName                    = arg<wow64::PUNICODE_STRING>(core, 1);
        const auto SecurityQos                 = arg<wow64::PSECURITY_QUALITY_OF_SERVICE>(core, 2);
        const auto ClientView                  = arg<wow64::PPORT_VIEW>(core, 3);
        const auto RequiredServerSid           = arg<wow64::PSID>(core, 4);
        const auto ServerView                  = arg<wow64::PREMOTE_PORT_VIEW>(core, 5);
        const auto MaxMessageLength            = arg<wow64::PULONG>(core, 6);
        const auto ConnectionInformation       = arg<wow64::PVOID>(core, 7);
        const auto ConnectionInformationLength = arg<wow64::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[158]);

        on_func(PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSerializeBoot(proc_t proc, const on_NtSerializeBoot_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSerializeBoot@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[159]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetDebugFilterState(proc_t proc, const on_NtSetDebugFilterState_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetDebugFilterState@12", [=]
    {
        auto& core = d_->core;

        const auto ComponentId = arg<wow64::ULONG>(core, 0);
        const auto Level       = arg<wow64::ULONG>(core, 1);
        const auto State       = arg<wow64::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[160]);

        on_func(ComponentId, Level, State);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetDefaultHardErrorPort(proc_t proc, const on_NtSetDefaultHardErrorPort_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetDefaultHardErrorPort@4", [=]
    {
        auto& core = d_->core;

        const auto DefaultHardErrorPort = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[161]);

        on_func(DefaultHardErrorPort);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetDefaultLocale(proc_t proc, const on_NtSetDefaultLocale_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetDefaultLocale@8", [=]
    {
        auto& core = d_->core;

        const auto UserProfile     = arg<wow64::BOOLEAN>(core, 0);
        const auto DefaultLocaleId = arg<wow64::LCID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[162]);

        on_func(UserProfile, DefaultLocaleId);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetDriverEntryOrder(proc_t proc, const on_NtSetDriverEntryOrder_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetDriverEntryOrder@8", [=]
    {
        auto& core = d_->core;

        const auto Ids   = arg<wow64::PULONG>(core, 0);
        const auto Count = arg<wow64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[163]);

        on_func(Ids, Count);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetEvent(proc_t proc, const on_NtSetEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetEvent@8", [=]
    {
        auto& core = d_->core;

        const auto EventHandle   = arg<wow64::HANDLE>(core, 0);
        const auto PreviousState = arg<wow64::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[164]);

        on_func(EventHandle, PreviousState);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetEventBoostPriority(proc_t proc, const on_NtSetEventBoostPriority_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetEventBoostPriority@4", [=]
    {
        auto& core = d_->core;

        const auto EventHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[165]);

        on_func(EventHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetHighEventPair(proc_t proc, const on_NtSetHighEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetHighEventPair@4", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[166]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetHighWaitLowEventPair(proc_t proc, const on_NtSetHighWaitLowEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetHighWaitLowEventPair@4", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[167]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetInformationEnlistment(proc_t proc, const on_NtSetInformationEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetInformationEnlistment@16", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle            = arg<wow64::HANDLE>(core, 0);
        const auto EnlistmentInformationClass  = arg<wow64::ENLISTMENT_INFORMATION_CLASS>(core, 1);
        const auto EnlistmentInformation       = arg<wow64::PVOID>(core, 2);
        const auto EnlistmentInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[168]);

        on_func(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetIntervalProfile(proc_t proc, const on_NtSetIntervalProfile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetIntervalProfile@8", [=]
    {
        auto& core = d_->core;

        const auto Interval = arg<wow64::ULONG>(core, 0);
        const auto Source   = arg<wow64::KPROFILE_SOURCE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[169]);

        on_func(Interval, Source);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetIoCompletion(proc_t proc, const on_NtSetIoCompletion_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetIoCompletion@20", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle  = arg<wow64::HANDLE>(core, 0);
        const auto KeyContext          = arg<wow64::PVOID>(core, 1);
        const auto ApcContext          = arg<wow64::PVOID>(core, 2);
        const auto IoStatus            = arg<wow64::NTSTATUS>(core, 3);
        const auto IoStatusInformation = arg<wow64::ULONG_PTR>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[170]);

        on_func(IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetIoCompletionEx(proc_t proc, const on_NtSetIoCompletionEx_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetIoCompletionEx@24", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle        = arg<wow64::HANDLE>(core, 0);
        const auto IoCompletionReserveHandle = arg<wow64::HANDLE>(core, 1);
        const auto KeyContext                = arg<wow64::PVOID>(core, 2);
        const auto ApcContext                = arg<wow64::PVOID>(core, 3);
        const auto IoStatus                  = arg<wow64::NTSTATUS>(core, 4);
        const auto IoStatusInformation       = arg<wow64::ULONG_PTR>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[171]);

        on_func(IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetSecurityObject(proc_t proc, const on_NtSetSecurityObject_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetSecurityObject@12", [=]
    {
        auto& core = d_->core;

        const auto Handle              = arg<wow64::HANDLE>(core, 0);
        const auto SecurityInformation = arg<wow64::SECURITY_INFORMATION>(core, 1);
        const auto SecurityDescriptor  = arg<wow64::PSECURITY_DESCRIPTOR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[172]);

        on_func(Handle, SecurityInformation, SecurityDescriptor);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetTimerResolution(proc_t proc, const on_NtSetTimerResolution_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetTimerResolution@12", [=]
    {
        auto& core = d_->core;

        const auto DesiredTime   = arg<wow64::ULONG>(core, 0);
        const auto SetResolution = arg<wow64::BOOLEAN>(core, 1);
        const auto ActualTime    = arg<wow64::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[173]);

        on_func(DesiredTime, SetResolution, ActualTime);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSetVolumeInformationFile(proc_t proc, const on_NtSetVolumeInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSetVolumeInformationFile@20", [=]
    {
        auto& core = d_->core;

        const auto FileHandle         = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock      = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto FsInformation      = arg<wow64::PVOID>(core, 2);
        const auto Length             = arg<wow64::ULONG>(core, 3);
        const auto FsInformationClass = arg<wow64::FS_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[174]);

        on_func(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtShutdownWorkerFactory(proc_t proc, const on_NtShutdownWorkerFactory_fn& on_func)
{
    return register_callback(*d_, proc, "_NtShutdownWorkerFactory@8", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle    = arg<wow64::HANDLE>(core, 0);
        const auto STARPendingWorkerCount = arg<wow64::LONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[175]);

        on_func(WorkerFactoryHandle, STARPendingWorkerCount);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtStartProfile(proc_t proc, const on_NtStartProfile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtStartProfile@4", [=]
    {
        auto& core = d_->core;

        const auto ProfileHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[176]);

        on_func(ProfileHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtSystemDebugControl(proc_t proc, const on_NtSystemDebugControl_fn& on_func)
{
    return register_callback(*d_, proc, "_NtSystemDebugControl@24", [=]
    {
        auto& core = d_->core;

        const auto Command            = arg<wow64::SYSDBG_COMMAND>(core, 0);
        const auto InputBuffer        = arg<wow64::PVOID>(core, 1);
        const auto InputBufferLength  = arg<wow64::ULONG>(core, 2);
        const auto OutputBuffer       = arg<wow64::PVOID>(core, 3);
        const auto OutputBufferLength = arg<wow64::ULONG>(core, 4);
        const auto ReturnLength       = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[177]);

        on_func(Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtTestAlert(proc_t proc, const on_NtTestAlert_fn& on_func)
{
    return register_callback(*d_, proc, "_NtTestAlert@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[178]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_NtThawTransactions(proc_t proc, const on_NtThawTransactions_fn& on_func)
{
    return register_callback(*d_, proc, "_NtThawTransactions@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[179]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_NtTraceEvent(proc_t proc, const on_NtTraceEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtTraceEvent@16", [=]
    {
        auto& core = d_->core;

        const auto TraceHandle = arg<wow64::HANDLE>(core, 0);
        const auto Flags       = arg<wow64::ULONG>(core, 1);
        const auto FieldSize   = arg<wow64::ULONG>(core, 2);
        const auto Fields      = arg<wow64::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[180]);

        on_func(TraceHandle, Flags, FieldSize, Fields);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtTranslateFilePath(proc_t proc, const on_NtTranslateFilePath_fn& on_func)
{
    return register_callback(*d_, proc, "_NtTranslateFilePath@16", [=]
    {
        auto& core = d_->core;

        const auto InputFilePath        = arg<wow64::PFILE_PATH>(core, 0);
        const auto OutputType           = arg<wow64::ULONG>(core, 1);
        const auto OutputFilePath       = arg<wow64::PFILE_PATH>(core, 2);
        const auto OutputFilePathLength = arg<wow64::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[181]);

        on_func(InputFilePath, OutputType, OutputFilePath, OutputFilePathLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtUnloadKey(proc_t proc, const on_NtUnloadKey_fn& on_func)
{
    return register_callback(*d_, proc, "_NtUnloadKey@4", [=]
    {
        auto& core = d_->core;

        const auto TargetKey = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[182]);

        on_func(TargetKey);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtUnlockVirtualMemory(proc_t proc, const on_NtUnlockVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "_NtUnlockVirtualMemory@16", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<wow64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<wow64::PVOID>(core, 1);
        const auto RegionSize      = arg<wow64::PSIZE_T>(core, 2);
        const auto MapType         = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[183]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtUnmapViewOfSection(proc_t proc, const on_NtUnmapViewOfSection_fn& on_func)
{
    return register_callback(*d_, proc, "_NtUnmapViewOfSection@8", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<wow64::HANDLE>(core, 0);
        const auto BaseAddress   = arg<wow64::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[184]);

        on_func(ProcessHandle, BaseAddress);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtVdmControl(proc_t proc, const on_NtVdmControl_fn& on_func)
{
    return register_callback(*d_, proc, "_NtVdmControl@8", [=]
    {
        auto& core = d_->core;

        const auto Service     = arg<wow64::VDMSERVICECLASS>(core, 0);
        const auto ServiceData = arg<wow64::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[185]);

        on_func(Service, ServiceData);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWaitForDebugEvent(proc_t proc, const on_NtWaitForDebugEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWaitForDebugEvent@16", [=]
    {
        auto& core = d_->core;

        const auto DebugObjectHandle = arg<wow64::HANDLE>(core, 0);
        const auto Alertable         = arg<wow64::BOOLEAN>(core, 1);
        const auto Timeout           = arg<wow64::PLARGE_INTEGER>(core, 2);
        const auto WaitStateChange   = arg<wow64::PDBGUI_WAIT_STATE_CHANGE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[186]);

        on_func(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWaitForKeyedEvent(proc_t proc, const on_NtWaitForKeyedEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWaitForKeyedEvent@16", [=]
    {
        auto& core = d_->core;

        const auto KeyedEventHandle = arg<wow64::HANDLE>(core, 0);
        const auto KeyValue         = arg<wow64::PVOID>(core, 1);
        const auto Alertable        = arg<wow64::BOOLEAN>(core, 2);
        const auto Timeout          = arg<wow64::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[187]);

        on_func(KeyedEventHandle, KeyValue, Alertable, Timeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWaitForMultipleObjects(proc_t proc, const on_NtWaitForMultipleObjects_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWaitForMultipleObjects@20", [=]
    {
        auto& core = d_->core;

        const auto Count     = arg<wow64::ULONG>(core, 0);
        const auto Handles   = arg<wow64::HANDLE>(core, 1);
        const auto WaitType  = arg<wow64::WAIT_TYPE>(core, 2);
        const auto Alertable = arg<wow64::BOOLEAN>(core, 3);
        const auto Timeout   = arg<wow64::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[188]);

        on_func(Count, Handles, WaitType, Alertable, Timeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWaitForMultipleObjects32(proc_t proc, const on_NtWaitForMultipleObjects32_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWaitForMultipleObjects32@20", [=]
    {
        auto& core = d_->core;

        const auto Count     = arg<wow64::ULONG>(core, 0);
        const auto Handles   = arg<wow64::LONG>(core, 1);
        const auto WaitType  = arg<wow64::WAIT_TYPE>(core, 2);
        const auto Alertable = arg<wow64::BOOLEAN>(core, 3);
        const auto Timeout   = arg<wow64::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[189]);

        on_func(Count, Handles, WaitType, Alertable, Timeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWaitForWorkViaWorkerFactory(proc_t proc, const on_NtWaitForWorkViaWorkerFactory_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWaitForWorkViaWorkerFactory@20", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle = arg<wow64::HANDLE>(core, 0);
        const auto MiniPacket          = arg<wow64::PFILE_IO_COMPLETION_INFORMATION>(core, 1);
        const auto Arg2                = arg<wow64::LONG>(core, 2);
        const auto Arg3                = arg<wow64::LONG>(core, 3);
        const auto Arg4                = arg<wow64::LONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[190]);

        on_func(WorkerFactoryHandle, MiniPacket, Arg2, Arg3, Arg4);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWaitLowEventPair(proc_t proc, const on_NtWaitLowEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWaitLowEventPair@4", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[191]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWorkerFactoryWorkerReady(proc_t proc, const on_NtWorkerFactoryWorkerReady_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWorkerFactoryWorkerReady@4", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[192]);

        on_func(WorkerFactoryHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWriteFile(proc_t proc, const on_NtWriteFile_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWriteFile@36", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<wow64::HANDLE>(core, 0);
        const auto Event         = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<wow64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<wow64::PVOID>(core, 3);
        const auto IoStatusBlock = arg<wow64::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer        = arg<wow64::PVOID>(core, 5);
        const auto Length        = arg<wow64::ULONG>(core, 6);
        const auto ByteOffset    = arg<wow64::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<wow64::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[193]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWriteFileGather(proc_t proc, const on_NtWriteFileGather_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWriteFileGather@36", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<wow64::HANDLE>(core, 0);
        const auto Event         = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<wow64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<wow64::PVOID>(core, 3);
        const auto IoStatusBlock = arg<wow64::PIO_STATUS_BLOCK>(core, 4);
        const auto SegmentArray  = arg<wow64::PFILE_SEGMENT_ELEMENT>(core, 5);
        const auto Length        = arg<wow64::ULONG>(core, 6);
        const auto ByteOffset    = arg<wow64::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<wow64::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[194]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWriteRequestData(proc_t proc, const on_NtWriteRequestData_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWriteRequestData@24", [=]
    {
        auto& core = d_->core;

        const auto PortHandle           = arg<wow64::HANDLE>(core, 0);
        const auto Message              = arg<wow64::PPORT_MESSAGE>(core, 1);
        const auto DataEntryIndex       = arg<wow64::ULONG>(core, 2);
        const auto Buffer               = arg<wow64::PVOID>(core, 3);
        const auto BufferSize           = arg<wow64::SIZE_T>(core, 4);
        const auto NumberOfBytesWritten = arg<wow64::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[195]);

        on_func(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten);
    });
}

opt<bpid_t> wow64::syscalls32::register_NtWriteVirtualMemory(proc_t proc, const on_NtWriteVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "_NtWriteVirtualMemory@20", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle        = arg<wow64::HANDLE>(core, 0);
        const auto BaseAddress          = arg<wow64::PVOID>(core, 1);
        const auto Buffer               = arg<wow64::PVOID>(core, 2);
        const auto BufferSize           = arg<wow64::SIZE_T>(core, 3);
        const auto NumberOfBytesWritten = arg<wow64::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[196]);

        on_func(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAccessCheckAndAuditAlarm(proc_t proc, const on_ZwAccessCheckAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAccessCheckAndAuditAlarm@44", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName      = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto HandleId           = arg<wow64::PVOID>(core, 1);
        const auto ObjectTypeName     = arg<wow64::PUNICODE_STRING>(core, 2);
        const auto ObjectName         = arg<wow64::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor = arg<wow64::PSECURITY_DESCRIPTOR>(core, 4);
        const auto DesiredAccess      = arg<wow64::ACCESS_MASK>(core, 5);
        const auto GenericMapping     = arg<wow64::PGENERIC_MAPPING>(core, 6);
        const auto ObjectCreation     = arg<wow64::BOOLEAN>(core, 7);
        const auto GrantedAccess      = arg<wow64::PACCESS_MASK>(core, 8);
        const auto AccessStatus       = arg<wow64::PNTSTATUS>(core, 9);
        const auto GenerateOnClose    = arg<wow64::PBOOLEAN>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[197]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(proc_t proc, const on_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle@68", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName        = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<wow64::PVOID>(core, 1);
        const auto ClientToken          = arg<wow64::HANDLE>(core, 2);
        const auto ObjectTypeName       = arg<wow64::PUNICODE_STRING>(core, 3);
        const auto ObjectName           = arg<wow64::PUNICODE_STRING>(core, 4);
        const auto SecurityDescriptor   = arg<wow64::PSECURITY_DESCRIPTOR>(core, 5);
        const auto PrincipalSelfSid     = arg<wow64::PSID>(core, 6);
        const auto DesiredAccess        = arg<wow64::ACCESS_MASK>(core, 7);
        const auto AuditType            = arg<wow64::AUDIT_EVENT_TYPE>(core, 8);
        const auto Flags                = arg<wow64::ULONG>(core, 9);
        const auto ObjectTypeList       = arg<wow64::POBJECT_TYPE_LIST>(core, 10);
        const auto ObjectTypeListLength = arg<wow64::ULONG>(core, 11);
        const auto GenericMapping       = arg<wow64::PGENERIC_MAPPING>(core, 12);
        const auto ObjectCreation       = arg<wow64::BOOLEAN>(core, 13);
        const auto GrantedAccess        = arg<wow64::PACCESS_MASK>(core, 14);
        const auto AccessStatus         = arg<wow64::PNTSTATUS>(core, 15);
        const auto GenerateOnClose      = arg<wow64::PBOOLEAN>(core, 16);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[198]);

        on_func(SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAddBootEntry(proc_t proc, const on_ZwAddBootEntry_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAddBootEntry@8", [=]
    {
        auto& core = d_->core;

        const auto BootEntry = arg<wow64::PBOOT_ENTRY>(core, 0);
        const auto Id        = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[199]);

        on_func(BootEntry, Id);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAdjustGroupsToken(proc_t proc, const on_ZwAdjustGroupsToken_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAdjustGroupsToken@24", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle    = arg<wow64::HANDLE>(core, 0);
        const auto ResetToDefault = arg<wow64::BOOLEAN>(core, 1);
        const auto NewState       = arg<wow64::PTOKEN_GROUPS>(core, 2);
        const auto BufferLength   = arg<wow64::ULONG>(core, 3);
        const auto PreviousState  = arg<wow64::PTOKEN_GROUPS>(core, 4);
        const auto ReturnLength   = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[200]);

        on_func(TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAdjustPrivilegesToken(proc_t proc, const on_ZwAdjustPrivilegesToken_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAdjustPrivilegesToken@24", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle          = arg<wow64::HANDLE>(core, 0);
        const auto DisableAllPrivileges = arg<wow64::BOOLEAN>(core, 1);
        const auto NewState             = arg<wow64::PTOKEN_PRIVILEGES>(core, 2);
        const auto BufferLength         = arg<wow64::ULONG>(core, 3);
        const auto PreviousState        = arg<wow64::PTOKEN_PRIVILEGES>(core, 4);
        const auto ReturnLength         = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[201]);

        on_func(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAllocateLocallyUniqueId(proc_t proc, const on_ZwAllocateLocallyUniqueId_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAllocateLocallyUniqueId@4", [=]
    {
        auto& core = d_->core;

        const auto Luid = arg<wow64::PLUID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[202]);

        on_func(Luid);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcCancelMessage(proc_t proc, const on_ZwAlpcCancelMessage_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcCancelMessage@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle     = arg<wow64::HANDLE>(core, 0);
        const auto Flags          = arg<wow64::ULONG>(core, 1);
        const auto MessageContext = arg<wow64::PALPC_CONTEXT_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[203]);

        on_func(PortHandle, Flags, MessageContext);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcConnectPort(proc_t proc, const on_ZwAlpcConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcConnectPort@44", [=]
    {
        auto& core = d_->core;

        const auto PortHandle           = arg<wow64::PHANDLE>(core, 0);
        const auto PortName             = arg<wow64::PUNICODE_STRING>(core, 1);
        const auto ObjectAttributes     = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto PortAttributes       = arg<wow64::PALPC_PORT_ATTRIBUTES>(core, 3);
        const auto Flags                = arg<wow64::ULONG>(core, 4);
        const auto RequiredServerSid    = arg<wow64::PSID>(core, 5);
        const auto ConnectionMessage    = arg<wow64::PPORT_MESSAGE>(core, 6);
        const auto BufferLength         = arg<wow64::PULONG>(core, 7);
        const auto OutMessageAttributes = arg<wow64::PALPC_MESSAGE_ATTRIBUTES>(core, 8);
        const auto InMessageAttributes  = arg<wow64::PALPC_MESSAGE_ATTRIBUTES>(core, 9);
        const auto Timeout              = arg<wow64::PLARGE_INTEGER>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[204]);

        on_func(PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcCreatePort(proc_t proc, const on_ZwAlpcCreatePort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcCreatePort@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle       = arg<wow64::PHANDLE>(core, 0);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 1);
        const auto PortAttributes   = arg<wow64::PALPC_PORT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[205]);

        on_func(PortHandle, ObjectAttributes, PortAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcCreateResourceReserve(proc_t proc, const on_ZwAlpcCreateResourceReserve_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcCreateResourceReserve@16", [=]
    {
        auto& core = d_->core;

        const auto PortHandle  = arg<wow64::HANDLE>(core, 0);
        const auto Flags       = arg<wow64::ULONG>(core, 1);
        const auto MessageSize = arg<wow64::SIZE_T>(core, 2);
        const auto ResourceId  = arg<wow64::PALPC_HANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[206]);

        on_func(PortHandle, Flags, MessageSize, ResourceId);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcCreateSectionView(proc_t proc, const on_ZwAlpcCreateSectionView_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcCreateSectionView@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle     = arg<wow64::HANDLE>(core, 0);
        const auto Flags          = arg<wow64::ULONG>(core, 1);
        const auto ViewAttributes = arg<wow64::PALPC_DATA_VIEW_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[207]);

        on_func(PortHandle, Flags, ViewAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcCreateSecurityContext(proc_t proc, const on_ZwAlpcCreateSecurityContext_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcCreateSecurityContext@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle        = arg<wow64::HANDLE>(core, 0);
        const auto Flags             = arg<wow64::ULONG>(core, 1);
        const auto SecurityAttribute = arg<wow64::PALPC_SECURITY_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[208]);

        on_func(PortHandle, Flags, SecurityAttribute);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcDeletePortSection(proc_t proc, const on_ZwAlpcDeletePortSection_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcDeletePortSection@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle    = arg<wow64::HANDLE>(core, 0);
        const auto Flags         = arg<wow64::ULONG>(core, 1);
        const auto SectionHandle = arg<wow64::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[209]);

        on_func(PortHandle, Flags, SectionHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcImpersonateClientOfPort(proc_t proc, const on_ZwAlpcImpersonateClientOfPort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcImpersonateClientOfPort@12", [=]
    {
        auto& core = d_->core;

        const auto PortHandle  = arg<wow64::HANDLE>(core, 0);
        const auto PortMessage = arg<wow64::PPORT_MESSAGE>(core, 1);
        const auto Reserved    = arg<wow64::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[210]);

        on_func(PortHandle, PortMessage, Reserved);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcOpenSenderProcess(proc_t proc, const on_ZwAlpcOpenSenderProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcOpenSenderProcess@24", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<wow64::PHANDLE>(core, 0);
        const auto PortHandle       = arg<wow64::HANDLE>(core, 1);
        const auto PortMessage      = arg<wow64::PPORT_MESSAGE>(core, 2);
        const auto Flags            = arg<wow64::ULONG>(core, 3);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 4);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[211]);

        on_func(ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcOpenSenderThread(proc_t proc, const on_ZwAlpcOpenSenderThread_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcOpenSenderThread@24", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle     = arg<wow64::PHANDLE>(core, 0);
        const auto PortHandle       = arg<wow64::HANDLE>(core, 1);
        const auto PortMessage      = arg<wow64::PPORT_MESSAGE>(core, 2);
        const auto Flags            = arg<wow64::ULONG>(core, 3);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 4);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[212]);

        on_func(ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcQueryInformation(proc_t proc, const on_ZwAlpcQueryInformation_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcQueryInformation@20", [=]
    {
        auto& core = d_->core;

        const auto PortHandle           = arg<wow64::HANDLE>(core, 0);
        const auto PortInformationClass = arg<wow64::ALPC_PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<wow64::PVOID>(core, 2);
        const auto Length               = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength         = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[213]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAlpcQueryInformationMessage(proc_t proc, const on_ZwAlpcQueryInformationMessage_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAlpcQueryInformationMessage@24", [=]
    {
        auto& core = d_->core;

        const auto PortHandle              = arg<wow64::HANDLE>(core, 0);
        const auto PortMessage             = arg<wow64::PPORT_MESSAGE>(core, 1);
        const auto MessageInformationClass = arg<wow64::ALPC_MESSAGE_INFORMATION_CLASS>(core, 2);
        const auto MessageInformation      = arg<wow64::PVOID>(core, 3);
        const auto Length                  = arg<wow64::ULONG>(core, 4);
        const auto ReturnLength            = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[214]);

        on_func(PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAreMappedFilesTheSame(proc_t proc, const on_ZwAreMappedFilesTheSame_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAreMappedFilesTheSame@8", [=]
    {
        auto& core = d_->core;

        const auto File1MappedAsAnImage = arg<wow64::PVOID>(core, 0);
        const auto File2MappedAsFile    = arg<wow64::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[215]);

        on_func(File1MappedAsAnImage, File2MappedAsFile);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwAssignProcessToJobObject(proc_t proc, const on_ZwAssignProcessToJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwAssignProcessToJobObject@8", [=]
    {
        auto& core = d_->core;

        const auto JobHandle     = arg<wow64::HANDLE>(core, 0);
        const auto ProcessHandle = arg<wow64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[216]);

        on_func(JobHandle, ProcessHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCancelIoFile(proc_t proc, const on_ZwCancelIoFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCancelIoFile@8", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<wow64::PIO_STATUS_BLOCK>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[217]);

        on_func(FileHandle, IoStatusBlock);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCancelTimer(proc_t proc, const on_ZwCancelTimer_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCancelTimer@8", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle  = arg<wow64::HANDLE>(core, 0);
        const auto CurrentState = arg<wow64::PBOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[218]);

        on_func(TimerHandle, CurrentState);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCloseObjectAuditAlarm(proc_t proc, const on_ZwCloseObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCloseObjectAuditAlarm@12", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName   = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto HandleId        = arg<wow64::PVOID>(core, 1);
        const auto GenerateOnClose = arg<wow64::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[219]);

        on_func(SubsystemName, HandleId, GenerateOnClose);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCommitComplete(proc_t proc, const on_ZwCommitComplete_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCommitComplete@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[220]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCompareTokens(proc_t proc, const on_ZwCompareTokens_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCompareTokens@12", [=]
    {
        auto& core = d_->core;

        const auto FirstTokenHandle  = arg<wow64::HANDLE>(core, 0);
        const auto SecondTokenHandle = arg<wow64::HANDLE>(core, 1);
        const auto Equal             = arg<wow64::PBOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[221]);

        on_func(FirstTokenHandle, SecondTokenHandle, Equal);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCompressKey(proc_t proc, const on_ZwCompressKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCompressKey@4", [=]
    {
        auto& core = d_->core;

        const auto Key = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[222]);

        on_func(Key);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwContinue(proc_t proc, const on_ZwContinue_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwContinue@8", [=]
    {
        auto& core = d_->core;

        const auto ContextRecord = arg<wow64::PCONTEXT>(core, 0);
        const auto TestAlert     = arg<wow64::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[223]);

        on_func(ContextRecord, TestAlert);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateDebugObject(proc_t proc, const on_ZwCreateDebugObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateDebugObject@16", [=]
    {
        auto& core = d_->core;

        const auto DebugObjectHandle = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Flags             = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[224]);

        on_func(DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateDirectoryObject(proc_t proc, const on_ZwCreateDirectoryObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateDirectoryObject@12", [=]
    {
        auto& core = d_->core;

        const auto DirectoryHandle  = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[225]);

        on_func(DirectoryHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateEnlistment(proc_t proc, const on_ZwCreateEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateEnlistment@32", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle      = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ResourceManagerHandle = arg<wow64::HANDLE>(core, 2);
        const auto TransactionHandle     = arg<wow64::HANDLE>(core, 3);
        const auto ObjectAttributes      = arg<wow64::POBJECT_ATTRIBUTES>(core, 4);
        const auto CreateOptions         = arg<wow64::ULONG>(core, 5);
        const auto NotificationMask      = arg<wow64::NOTIFICATION_MASK>(core, 6);
        const auto EnlistmentKey         = arg<wow64::PVOID>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[226]);

        on_func(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateJobObject(proc_t proc, const on_ZwCreateJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateJobObject@12", [=]
    {
        auto& core = d_->core;

        const auto JobHandle        = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[227]);

        on_func(JobHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateKey(proc_t proc, const on_ZwCreateKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateKey@28", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle        = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto TitleIndex       = arg<wow64::ULONG>(core, 3);
        const auto Class            = arg<wow64::PUNICODE_STRING>(core, 4);
        const auto CreateOptions    = arg<wow64::ULONG>(core, 5);
        const auto Disposition      = arg<wow64::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[228]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateKeyedEvent(proc_t proc, const on_ZwCreateKeyedEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateKeyedEvent@16", [=]
    {
        auto& core = d_->core;

        const auto KeyedEventHandle = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Flags            = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[229]);

        on_func(KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateMailslotFile(proc_t proc, const on_ZwCreateMailslotFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateMailslotFile@32", [=]
    {
        auto& core = d_->core;

        const auto FileHandle         = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<wow64::ULONG>(core, 1);
        const auto ObjectAttributes   = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock      = arg<wow64::PIO_STATUS_BLOCK>(core, 3);
        const auto CreateOptions      = arg<wow64::ULONG>(core, 4);
        const auto MailslotQuota      = arg<wow64::ULONG>(core, 5);
        const auto MaximumMessageSize = arg<wow64::ULONG>(core, 6);
        const auto ReadTimeout        = arg<wow64::PLARGE_INTEGER>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[230]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateMutant(proc_t proc, const on_ZwCreateMutant_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateMutant@16", [=]
    {
        auto& core = d_->core;

        const auto MutantHandle     = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto InitialOwner     = arg<wow64::BOOLEAN>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[231]);

        on_func(MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateNamedPipeFile(proc_t proc, const on_ZwCreateNamedPipeFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateNamedPipeFile@56", [=]
    {
        auto& core = d_->core;

        const auto FileHandle        = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<wow64::ULONG>(core, 1);
        const auto ObjectAttributes  = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock     = arg<wow64::PIO_STATUS_BLOCK>(core, 3);
        const auto ShareAccess       = arg<wow64::ULONG>(core, 4);
        const auto CreateDisposition = arg<wow64::ULONG>(core, 5);
        const auto CreateOptions     = arg<wow64::ULONG>(core, 6);
        const auto NamedPipeType     = arg<wow64::ULONG>(core, 7);
        const auto ReadMode          = arg<wow64::ULONG>(core, 8);
        const auto CompletionMode    = arg<wow64::ULONG>(core, 9);
        const auto MaximumInstances  = arg<wow64::ULONG>(core, 10);
        const auto InboundQuota      = arg<wow64::ULONG>(core, 11);
        const auto OutboundQuota     = arg<wow64::ULONG>(core, 12);
        const auto DefaultTimeout    = arg<wow64::PLARGE_INTEGER>(core, 13);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[232]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreatePort(proc_t proc, const on_ZwCreatePort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreatePort@20", [=]
    {
        auto& core = d_->core;

        const auto PortHandle              = arg<wow64::PHANDLE>(core, 0);
        const auto ObjectAttributes        = arg<wow64::POBJECT_ATTRIBUTES>(core, 1);
        const auto MaxConnectionInfoLength = arg<wow64::ULONG>(core, 2);
        const auto MaxMessageLength        = arg<wow64::ULONG>(core, 3);
        const auto MaxPoolUsage            = arg<wow64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[233]);

        on_func(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateProcess(proc_t proc, const on_ZwCreateProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateProcess@32", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle      = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ParentProcess      = arg<wow64::HANDLE>(core, 3);
        const auto InheritObjectTable = arg<wow64::BOOLEAN>(core, 4);
        const auto SectionHandle      = arg<wow64::HANDLE>(core, 5);
        const auto DebugPort          = arg<wow64::HANDLE>(core, 6);
        const auto ExceptionPort      = arg<wow64::HANDLE>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[234]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateProcessEx(proc_t proc, const on_ZwCreateProcessEx_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateProcessEx@36", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ParentProcess    = arg<wow64::HANDLE>(core, 3);
        const auto Flags            = arg<wow64::ULONG>(core, 4);
        const auto SectionHandle    = arg<wow64::HANDLE>(core, 5);
        const auto DebugPort        = arg<wow64::HANDLE>(core, 6);
        const auto ExceptionPort    = arg<wow64::HANDLE>(core, 7);
        const auto JobMemberLevel   = arg<wow64::ULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[235]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateProfile(proc_t proc, const on_ZwCreateProfile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateProfile@36", [=]
    {
        auto& core = d_->core;

        const auto ProfileHandle = arg<wow64::PHANDLE>(core, 0);
        const auto Process       = arg<wow64::HANDLE>(core, 1);
        const auto RangeBase     = arg<wow64::PVOID>(core, 2);
        const auto RangeSize     = arg<wow64::SIZE_T>(core, 3);
        const auto BucketSize    = arg<wow64::ULONG>(core, 4);
        const auto Buffer        = arg<wow64::PULONG>(core, 5);
        const auto BufferSize    = arg<wow64::ULONG>(core, 6);
        const auto ProfileSource = arg<wow64::KPROFILE_SOURCE>(core, 7);
        const auto Affinity      = arg<wow64::KAFFINITY>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[236]);

        on_func(ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateResourceManager(proc_t proc, const on_ZwCreateResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateResourceManager@28", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<wow64::ACCESS_MASK>(core, 1);
        const auto TmHandle              = arg<wow64::HANDLE>(core, 2);
        const auto RmGuid                = arg<wow64::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<wow64::POBJECT_ATTRIBUTES>(core, 4);
        const auto CreateOptions         = arg<wow64::ULONG>(core, 5);
        const auto Description           = arg<wow64::PUNICODE_STRING>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[237]);

        on_func(ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateSymbolicLinkObject(proc_t proc, const on_ZwCreateSymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateSymbolicLinkObject@16", [=]
    {
        auto& core = d_->core;

        const auto LinkHandle       = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto LinkTarget       = arg<wow64::PUNICODE_STRING>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[238]);

        on_func(LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateTimer(proc_t proc, const on_ZwCreateTimer_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateTimer@16", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle      = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto TimerType        = arg<wow64::TIMER_TYPE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[239]);

        on_func(TimerHandle, DesiredAccess, ObjectAttributes, TimerType);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateTransactionManager(proc_t proc, const on_ZwCreateTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateTransactionManager@24", [=]
    {
        auto& core = d_->core;

        const auto TmHandle         = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto LogFileName      = arg<wow64::PUNICODE_STRING>(core, 3);
        const auto CreateOptions    = arg<wow64::ULONG>(core, 4);
        const auto CommitStrength   = arg<wow64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[240]);

        on_func(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwCreateWaitablePort(proc_t proc, const on_ZwCreateWaitablePort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwCreateWaitablePort@20", [=]
    {
        auto& core = d_->core;

        const auto PortHandle              = arg<wow64::PHANDLE>(core, 0);
        const auto ObjectAttributes        = arg<wow64::POBJECT_ATTRIBUTES>(core, 1);
        const auto MaxConnectionInfoLength = arg<wow64::ULONG>(core, 2);
        const auto MaxMessageLength        = arg<wow64::ULONG>(core, 3);
        const auto MaxPoolUsage            = arg<wow64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[241]);

        on_func(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwDebugContinue(proc_t proc, const on_ZwDebugContinue_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwDebugContinue@12", [=]
    {
        auto& core = d_->core;

        const auto DebugObjectHandle = arg<wow64::HANDLE>(core, 0);
        const auto ClientId          = arg<wow64::PCLIENT_ID>(core, 1);
        const auto ContinueStatus    = arg<wow64::NTSTATUS>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[242]);

        on_func(DebugObjectHandle, ClientId, ContinueStatus);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwDelayExecution(proc_t proc, const on_ZwDelayExecution_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwDelayExecution@8", [=]
    {
        auto& core = d_->core;

        const auto Alertable     = arg<wow64::BOOLEAN>(core, 0);
        const auto DelayInterval = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[243]);

        on_func(Alertable, DelayInterval);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwDeleteAtom(proc_t proc, const on_ZwDeleteAtom_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwDeleteAtom@4", [=]
    {
        auto& core = d_->core;

        const auto Atom = arg<wow64::RTL_ATOM>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[244]);

        on_func(Atom);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwDeleteDriverEntry(proc_t proc, const on_ZwDeleteDriverEntry_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwDeleteDriverEntry@4", [=]
    {
        auto& core = d_->core;

        const auto Id = arg<wow64::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[245]);

        on_func(Id);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwDeleteKey(proc_t proc, const on_ZwDeleteKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwDeleteKey@4", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[246]);

        on_func(KeyHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwDeviceIoControlFile(proc_t proc, const on_ZwDeviceIoControlFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwDeviceIoControlFile@40", [=]
    {
        auto& core = d_->core;

        const auto FileHandle         = arg<wow64::HANDLE>(core, 0);
        const auto Event              = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine         = arg<wow64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext         = arg<wow64::PVOID>(core, 3);
        const auto IoStatusBlock      = arg<wow64::PIO_STATUS_BLOCK>(core, 4);
        const auto IoControlCode      = arg<wow64::ULONG>(core, 5);
        const auto InputBuffer        = arg<wow64::PVOID>(core, 6);
        const auto InputBufferLength  = arg<wow64::ULONG>(core, 7);
        const auto OutputBuffer       = arg<wow64::PVOID>(core, 8);
        const auto OutputBufferLength = arg<wow64::ULONG>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[247]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwDrawText(proc_t proc, const on_ZwDrawText_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwDrawText@4", [=]
    {
        auto& core = d_->core;

        const auto Text = arg<wow64::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[248]);

        on_func(Text);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwDuplicateObject(proc_t proc, const on_ZwDuplicateObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwDuplicateObject@28", [=]
    {
        auto& core = d_->core;

        const auto SourceProcessHandle = arg<wow64::HANDLE>(core, 0);
        const auto SourceHandle        = arg<wow64::HANDLE>(core, 1);
        const auto TargetProcessHandle = arg<wow64::HANDLE>(core, 2);
        const auto TargetHandle        = arg<wow64::PHANDLE>(core, 3);
        const auto DesiredAccess       = arg<wow64::ACCESS_MASK>(core, 4);
        const auto HandleAttributes    = arg<wow64::ULONG>(core, 5);
        const auto Options             = arg<wow64::ULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[249]);

        on_func(SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwEnumerateBootEntries(proc_t proc, const on_ZwEnumerateBootEntries_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwEnumerateBootEntries@8", [=]
    {
        auto& core = d_->core;

        const auto Buffer       = arg<wow64::PVOID>(core, 0);
        const auto BufferLength = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[250]);

        on_func(Buffer, BufferLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwEnumerateKey(proc_t proc, const on_ZwEnumerateKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwEnumerateKey@24", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle           = arg<wow64::HANDLE>(core, 0);
        const auto Index               = arg<wow64::ULONG>(core, 1);
        const auto KeyInformationClass = arg<wow64::KEY_INFORMATION_CLASS>(core, 2);
        const auto KeyInformation      = arg<wow64::PVOID>(core, 3);
        const auto Length              = arg<wow64::ULONG>(core, 4);
        const auto ResultLength        = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[251]);

        on_func(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwEnumerateSystemEnvironmentValuesEx(proc_t proc, const on_ZwEnumerateSystemEnvironmentValuesEx_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwEnumerateSystemEnvironmentValuesEx@12", [=]
    {
        auto& core = d_->core;

        const auto InformationClass = arg<wow64::ULONG>(core, 0);
        const auto Buffer           = arg<wow64::PVOID>(core, 1);
        const auto BufferLength     = arg<wow64::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[252]);

        on_func(InformationClass, Buffer, BufferLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwEnumerateTransactionObject(proc_t proc, const on_ZwEnumerateTransactionObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwEnumerateTransactionObject@20", [=]
    {
        auto& core = d_->core;

        const auto RootObjectHandle   = arg<wow64::HANDLE>(core, 0);
        const auto QueryType          = arg<wow64::KTMOBJECT_TYPE>(core, 1);
        const auto ObjectCursor       = arg<wow64::PKTMOBJECT_CURSOR>(core, 2);
        const auto ObjectCursorLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength       = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[253]);

        on_func(RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwExtendSection(proc_t proc, const on_ZwExtendSection_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwExtendSection@8", [=]
    {
        auto& core = d_->core;

        const auto SectionHandle  = arg<wow64::HANDLE>(core, 0);
        const auto NewSectionSize = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[254]);

        on_func(SectionHandle, NewSectionSize);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwFlushBuffersFile(proc_t proc, const on_ZwFlushBuffersFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwFlushBuffersFile@8", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<wow64::PIO_STATUS_BLOCK>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[255]);

        on_func(FileHandle, IoStatusBlock);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwFlushInstallUILanguage(proc_t proc, const on_ZwFlushInstallUILanguage_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwFlushInstallUILanguage@8", [=]
    {
        auto& core = d_->core;

        const auto InstallUILanguage = arg<wow64::LANGID>(core, 0);
        const auto SetComittedFlag   = arg<wow64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[256]);

        on_func(InstallUILanguage, SetComittedFlag);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwFlushInstructionCache(proc_t proc, const on_ZwFlushInstructionCache_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwFlushInstructionCache@12", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<wow64::HANDLE>(core, 0);
        const auto BaseAddress   = arg<wow64::PVOID>(core, 1);
        const auto Length        = arg<wow64::SIZE_T>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[257]);

        on_func(ProcessHandle, BaseAddress, Length);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwFlushProcessWriteBuffers(proc_t proc, const on_ZwFlushProcessWriteBuffers_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwFlushProcessWriteBuffers@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[258]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwFlushVirtualMemory(proc_t proc, const on_ZwFlushVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwFlushVirtualMemory@16", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<wow64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<wow64::PVOID>(core, 1);
        const auto RegionSize      = arg<wow64::PSIZE_T>(core, 2);
        const auto IoStatus        = arg<wow64::PIO_STATUS_BLOCK>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[259]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, IoStatus);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwFreezeTransactions(proc_t proc, const on_ZwFreezeTransactions_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwFreezeTransactions@8", [=]
    {
        auto& core = d_->core;

        const auto FreezeTimeout = arg<wow64::PLARGE_INTEGER>(core, 0);
        const auto ThawTimeout   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[260]);

        on_func(FreezeTimeout, ThawTimeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwGetNextProcess(proc_t proc, const on_ZwGetNextProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwGetNextProcess@20", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<wow64::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto HandleAttributes = arg<wow64::ULONG>(core, 2);
        const auto Flags            = arg<wow64::ULONG>(core, 3);
        const auto NewProcessHandle = arg<wow64::PHANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[261]);

        on_func(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwGetNextThread(proc_t proc, const on_ZwGetNextThread_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwGetNextThread@24", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<wow64::HANDLE>(core, 0);
        const auto ThreadHandle     = arg<wow64::HANDLE>(core, 1);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 2);
        const auto HandleAttributes = arg<wow64::ULONG>(core, 3);
        const auto Flags            = arg<wow64::ULONG>(core, 4);
        const auto NewThreadHandle  = arg<wow64::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[262]);

        on_func(ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwGetNotificationResourceManager(proc_t proc, const on_ZwGetNotificationResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwGetNotificationResourceManager@28", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle   = arg<wow64::HANDLE>(core, 0);
        const auto TransactionNotification = arg<wow64::PTRANSACTION_NOTIFICATION>(core, 1);
        const auto NotificationLength      = arg<wow64::ULONG>(core, 2);
        const auto Timeout                 = arg<wow64::PLARGE_INTEGER>(core, 3);
        const auto ReturnLength            = arg<wow64::PULONG>(core, 4);
        const auto Asynchronous            = arg<wow64::ULONG>(core, 5);
        const auto AsynchronousContext     = arg<wow64::ULONG_PTR>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[263]);

        on_func(ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwImpersonateClientOfPort(proc_t proc, const on_ZwImpersonateClientOfPort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwImpersonateClientOfPort@8", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<wow64::HANDLE>(core, 0);
        const auto Message    = arg<wow64::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[264]);

        on_func(PortHandle, Message);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwImpersonateThread(proc_t proc, const on_ZwImpersonateThread_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwImpersonateThread@12", [=]
    {
        auto& core = d_->core;

        const auto ServerThreadHandle = arg<wow64::HANDLE>(core, 0);
        const auto ClientThreadHandle = arg<wow64::HANDLE>(core, 1);
        const auto SecurityQos        = arg<wow64::PSECURITY_QUALITY_OF_SERVICE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[265]);

        on_func(ServerThreadHandle, ClientThreadHandle, SecurityQos);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwInitializeRegistry(proc_t proc, const on_ZwInitializeRegistry_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwInitializeRegistry@4", [=]
    {
        auto& core = d_->core;

        const auto BootCondition = arg<wow64::USHORT>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[266]);

        on_func(BootCondition);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwIsProcessInJob(proc_t proc, const on_ZwIsProcessInJob_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwIsProcessInJob@8", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<wow64::HANDLE>(core, 0);
        const auto JobHandle     = arg<wow64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[267]);

        on_func(ProcessHandle, JobHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwIsUILanguageComitted(proc_t proc, const on_ZwIsUILanguageComitted_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwIsUILanguageComitted@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[268]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwListenPort(proc_t proc, const on_ZwListenPort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwListenPort@8", [=]
    {
        auto& core = d_->core;

        const auto PortHandle        = arg<wow64::HANDLE>(core, 0);
        const auto ConnectionRequest = arg<wow64::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[269]);

        on_func(PortHandle, ConnectionRequest);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwLockProductActivationKeys(proc_t proc, const on_ZwLockProductActivationKeys_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwLockProductActivationKeys@8", [=]
    {
        auto& core = d_->core;

        const auto STARpPrivateVer = arg<wow64::ULONG>(core, 0);
        const auto STARpSafeMode   = arg<wow64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[270]);

        on_func(STARpPrivateVer, STARpSafeMode);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwLockVirtualMemory(proc_t proc, const on_ZwLockVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwLockVirtualMemory@16", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<wow64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<wow64::PVOID>(core, 1);
        const auto RegionSize      = arg<wow64::PSIZE_T>(core, 2);
        const auto MapType         = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[271]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwMakePermanentObject(proc_t proc, const on_ZwMakePermanentObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwMakePermanentObject@4", [=]
    {
        auto& core = d_->core;

        const auto Handle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[272]);

        on_func(Handle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwMapCMFModule(proc_t proc, const on_ZwMapCMFModule_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwMapCMFModule@24", [=]
    {
        auto& core = d_->core;

        const auto What            = arg<wow64::ULONG>(core, 0);
        const auto Index           = arg<wow64::ULONG>(core, 1);
        const auto CacheIndexOut   = arg<wow64::PULONG>(core, 2);
        const auto CacheFlagsOut   = arg<wow64::PULONG>(core, 3);
        const auto ViewSizeOut     = arg<wow64::PULONG>(core, 4);
        const auto STARBaseAddress = arg<wow64::PVOID>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[273]);

        on_func(What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwMapUserPhysicalPagesScatter(proc_t proc, const on_ZwMapUserPhysicalPagesScatter_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwMapUserPhysicalPagesScatter@12", [=]
    {
        auto& core = d_->core;

        const auto STARVirtualAddresses = arg<wow64::PVOID>(core, 0);
        const auto NumberOfPages        = arg<wow64::ULONG_PTR>(core, 1);
        const auto UserPfnArray         = arg<wow64::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[274]);

        on_func(STARVirtualAddresses, NumberOfPages, UserPfnArray);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwMapViewOfSection(proc_t proc, const on_ZwMapViewOfSection_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwMapViewOfSection@40", [=]
    {
        auto& core = d_->core;

        const auto SectionHandle      = arg<wow64::HANDLE>(core, 0);
        const auto ProcessHandle      = arg<wow64::HANDLE>(core, 1);
        const auto STARBaseAddress    = arg<wow64::PVOID>(core, 2);
        const auto ZeroBits           = arg<wow64::ULONG_PTR>(core, 3);
        const auto CommitSize         = arg<wow64::SIZE_T>(core, 4);
        const auto SectionOffset      = arg<wow64::PLARGE_INTEGER>(core, 5);
        const auto ViewSize           = arg<wow64::PSIZE_T>(core, 6);
        const auto InheritDisposition = arg<wow64::SECTION_INHERIT>(core, 7);
        const auto AllocationType     = arg<wow64::ULONG>(core, 8);
        const auto Win32Protect       = arg<wow64::WIN32_PROTECTION_MASK>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[275]);

        on_func(SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwModifyDriverEntry(proc_t proc, const on_ZwModifyDriverEntry_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwModifyDriverEntry@4", [=]
    {
        auto& core = d_->core;

        const auto DriverEntry = arg<wow64::PEFI_DRIVER_ENTRY>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[276]);

        on_func(DriverEntry);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenDirectoryObject(proc_t proc, const on_ZwOpenDirectoryObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenDirectoryObject@12", [=]
    {
        auto& core = d_->core;

        const auto DirectoryHandle  = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[277]);

        on_func(DirectoryHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenEnlistment(proc_t proc, const on_ZwOpenEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenEnlistment@20", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle      = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ResourceManagerHandle = arg<wow64::HANDLE>(core, 2);
        const auto EnlistmentGuid        = arg<wow64::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<wow64::POBJECT_ATTRIBUTES>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[278]);

        on_func(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenIoCompletion(proc_t proc, const on_ZwOpenIoCompletion_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenIoCompletion@12", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[279]);

        on_func(IoCompletionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenJobObject(proc_t proc, const on_ZwOpenJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenJobObject@12", [=]
    {
        auto& core = d_->core;

        const auto JobHandle        = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[280]);

        on_func(JobHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenKey(proc_t proc, const on_ZwOpenKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenKey@12", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle        = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[281]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenKeyEx(proc_t proc, const on_ZwOpenKeyEx_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenKeyEx@16", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle        = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto OpenOptions      = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[282]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenObjectAuditAlarm(proc_t proc, const on_ZwOpenObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenObjectAuditAlarm@48", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName      = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto HandleId           = arg<wow64::PVOID>(core, 1);
        const auto ObjectTypeName     = arg<wow64::PUNICODE_STRING>(core, 2);
        const auto ObjectName         = arg<wow64::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor = arg<wow64::PSECURITY_DESCRIPTOR>(core, 4);
        const auto ClientToken        = arg<wow64::HANDLE>(core, 5);
        const auto DesiredAccess      = arg<wow64::ACCESS_MASK>(core, 6);
        const auto GrantedAccess      = arg<wow64::ACCESS_MASK>(core, 7);
        const auto Privileges         = arg<wow64::PPRIVILEGE_SET>(core, 8);
        const auto ObjectCreation     = arg<wow64::BOOLEAN>(core, 9);
        const auto AccessGranted      = arg<wow64::BOOLEAN>(core, 10);
        const auto GenerateOnClose    = arg<wow64::PBOOLEAN>(core, 11);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[283]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenProcess(proc_t proc, const on_ZwOpenProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenProcess@16", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ClientId         = arg<wow64::PCLIENT_ID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[284]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenProcessToken(proc_t proc, const on_ZwOpenProcessToken_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenProcessToken@12", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<wow64::HANDLE>(core, 0);
        const auto DesiredAccess = arg<wow64::ACCESS_MASK>(core, 1);
        const auto TokenHandle   = arg<wow64::PHANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[285]);

        on_func(ProcessHandle, DesiredAccess, TokenHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenProcessTokenEx(proc_t proc, const on_ZwOpenProcessTokenEx_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenProcessTokenEx@16", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<wow64::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto HandleAttributes = arg<wow64::ULONG>(core, 2);
        const auto TokenHandle      = arg<wow64::PHANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[286]);

        on_func(ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenResourceManager(proc_t proc, const on_ZwOpenResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenResourceManager@20", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<wow64::ACCESS_MASK>(core, 1);
        const auto TmHandle              = arg<wow64::HANDLE>(core, 2);
        const auto ResourceManagerGuid   = arg<wow64::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<wow64::POBJECT_ATTRIBUTES>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[287]);

        on_func(ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenThread(proc_t proc, const on_ZwOpenThread_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenThread@16", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle     = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ClientId         = arg<wow64::PCLIENT_ID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[288]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenTimer(proc_t proc, const on_ZwOpenTimer_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenTimer@12", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle      = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[289]);

        on_func(TimerHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenTransaction(proc_t proc, const on_ZwOpenTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenTransaction@20", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Uow               = arg<wow64::LPGUID>(core, 3);
        const auto TmHandle          = arg<wow64::HANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[290]);

        on_func(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwOpenTransactionManager(proc_t proc, const on_ZwOpenTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwOpenTransactionManager@24", [=]
    {
        auto& core = d_->core;

        const auto TmHandle         = arg<wow64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<wow64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);
        const auto LogFileName      = arg<wow64::PUNICODE_STRING>(core, 3);
        const auto TmIdentity       = arg<wow64::LPGUID>(core, 4);
        const auto OpenOptions      = arg<wow64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[291]);

        on_func(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwPowerInformation(proc_t proc, const on_ZwPowerInformation_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwPowerInformation@20", [=]
    {
        auto& core = d_->core;

        const auto InformationLevel   = arg<wow64::POWER_INFORMATION_LEVEL>(core, 0);
        const auto InputBuffer        = arg<wow64::PVOID>(core, 1);
        const auto InputBufferLength  = arg<wow64::ULONG>(core, 2);
        const auto OutputBuffer       = arg<wow64::PVOID>(core, 3);
        const auto OutputBufferLength = arg<wow64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[292]);

        on_func(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwPrePrepareComplete(proc_t proc, const on_ZwPrePrepareComplete_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwPrePrepareComplete@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[293]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwPrepareEnlistment(proc_t proc, const on_ZwPrepareEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwPrepareEnlistment@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[294]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwPrivilegeCheck(proc_t proc, const on_ZwPrivilegeCheck_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwPrivilegeCheck@12", [=]
    {
        auto& core = d_->core;

        const auto ClientToken        = arg<wow64::HANDLE>(core, 0);
        const auto RequiredPrivileges = arg<wow64::PPRIVILEGE_SET>(core, 1);
        const auto Result             = arg<wow64::PBOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[295]);

        on_func(ClientToken, RequiredPrivileges, Result);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwPrivilegeObjectAuditAlarm(proc_t proc, const on_ZwPrivilegeObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwPrivilegeObjectAuditAlarm@24", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto HandleId      = arg<wow64::PVOID>(core, 1);
        const auto ClientToken   = arg<wow64::HANDLE>(core, 2);
        const auto DesiredAccess = arg<wow64::ACCESS_MASK>(core, 3);
        const auto Privileges    = arg<wow64::PPRIVILEGE_SET>(core, 4);
        const auto AccessGranted = arg<wow64::BOOLEAN>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[296]);

        on_func(SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwPropagationFailed(proc_t proc, const on_ZwPropagationFailed_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwPropagationFailed@12", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle = arg<wow64::HANDLE>(core, 0);
        const auto RequestCookie         = arg<wow64::ULONG>(core, 1);
        const auto PropStatus            = arg<wow64::NTSTATUS>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[297]);

        on_func(ResourceManagerHandle, RequestCookie, PropStatus);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwProtectVirtualMemory(proc_t proc, const on_ZwProtectVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwProtectVirtualMemory@20", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<wow64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<wow64::PVOID>(core, 1);
        const auto RegionSize      = arg<wow64::PSIZE_T>(core, 2);
        const auto NewProtectWin32 = arg<wow64::WIN32_PROTECTION_MASK>(core, 3);
        const auto OldProtect      = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[298]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwPulseEvent(proc_t proc, const on_ZwPulseEvent_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwPulseEvent@8", [=]
    {
        auto& core = d_->core;

        const auto EventHandle   = arg<wow64::HANDLE>(core, 0);
        const auto PreviousState = arg<wow64::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[299]);

        on_func(EventHandle, PreviousState);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryAttributesFile(proc_t proc, const on_ZwQueryAttributesFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryAttributesFile@8", [=]
    {
        auto& core = d_->core;

        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);
        const auto FileInformation  = arg<wow64::PFILE_BASIC_INFORMATION>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[300]);

        on_func(ObjectAttributes, FileInformation);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryBootEntryOrder(proc_t proc, const on_ZwQueryBootEntryOrder_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryBootEntryOrder@8", [=]
    {
        auto& core = d_->core;

        const auto Ids   = arg<wow64::PULONG>(core, 0);
        const auto Count = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[301]);

        on_func(Ids, Count);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryBootOptions(proc_t proc, const on_ZwQueryBootOptions_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryBootOptions@8", [=]
    {
        auto& core = d_->core;

        const auto BootOptions       = arg<wow64::PBOOT_OPTIONS>(core, 0);
        const auto BootOptionsLength = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[302]);

        on_func(BootOptions, BootOptionsLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryDefaultUILanguage(proc_t proc, const on_ZwQueryDefaultUILanguage_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryDefaultUILanguage@4", [=]
    {
        auto& core = d_->core;

        const auto STARDefaultUILanguageId = arg<wow64::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[303]);

        on_func(STARDefaultUILanguageId);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryDirectoryFile(proc_t proc, const on_ZwQueryDirectoryFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryDirectoryFile@44", [=]
    {
        auto& core = d_->core;

        const auto FileHandle           = arg<wow64::HANDLE>(core, 0);
        const auto Event                = arg<wow64::HANDLE>(core, 1);
        const auto ApcRoutine           = arg<wow64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext           = arg<wow64::PVOID>(core, 3);
        const auto IoStatusBlock        = arg<wow64::PIO_STATUS_BLOCK>(core, 4);
        const auto FileInformation      = arg<wow64::PVOID>(core, 5);
        const auto Length               = arg<wow64::ULONG>(core, 6);
        const auto FileInformationClass = arg<wow64::FILE_INFORMATION_CLASS>(core, 7);
        const auto ReturnSingleEntry    = arg<wow64::BOOLEAN>(core, 8);
        const auto FileName             = arg<wow64::PUNICODE_STRING>(core, 9);
        const auto RestartScan          = arg<wow64::BOOLEAN>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[304]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryDirectoryObject(proc_t proc, const on_ZwQueryDirectoryObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryDirectoryObject@28", [=]
    {
        auto& core = d_->core;

        const auto DirectoryHandle   = arg<wow64::HANDLE>(core, 0);
        const auto Buffer            = arg<wow64::PVOID>(core, 1);
        const auto Length            = arg<wow64::ULONG>(core, 2);
        const auto ReturnSingleEntry = arg<wow64::BOOLEAN>(core, 3);
        const auto RestartScan       = arg<wow64::BOOLEAN>(core, 4);
        const auto Context           = arg<wow64::PULONG>(core, 5);
        const auto ReturnLength      = arg<wow64::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[305]);

        on_func(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryEaFile(proc_t proc, const on_ZwQueryEaFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryEaFile@36", [=]
    {
        auto& core = d_->core;

        const auto FileHandle        = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock     = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer            = arg<wow64::PVOID>(core, 2);
        const auto Length            = arg<wow64::ULONG>(core, 3);
        const auto ReturnSingleEntry = arg<wow64::BOOLEAN>(core, 4);
        const auto EaList            = arg<wow64::PVOID>(core, 5);
        const auto EaListLength      = arg<wow64::ULONG>(core, 6);
        const auto EaIndex           = arg<wow64::PULONG>(core, 7);
        const auto RestartScan       = arg<wow64::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[306]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryFullAttributesFile(proc_t proc, const on_ZwQueryFullAttributesFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryFullAttributesFile@8", [=]
    {
        auto& core = d_->core;

        const auto ObjectAttributes = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);
        const auto FileInformation  = arg<wow64::PFILE_NETWORK_OPEN_INFORMATION>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[307]);

        on_func(ObjectAttributes, FileInformation);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryInformationEnlistment(proc_t proc, const on_ZwQueryInformationEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryInformationEnlistment@20", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle            = arg<wow64::HANDLE>(core, 0);
        const auto EnlistmentInformationClass  = arg<wow64::ENLISTMENT_INFORMATION_CLASS>(core, 1);
        const auto EnlistmentInformation       = arg<wow64::PVOID>(core, 2);
        const auto EnlistmentInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength                = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[308]);

        on_func(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryInformationFile(proc_t proc, const on_ZwQueryInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryInformationFile@20", [=]
    {
        auto& core = d_->core;

        const auto FileHandle           = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock        = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto FileInformation      = arg<wow64::PVOID>(core, 2);
        const auto Length               = arg<wow64::ULONG>(core, 3);
        const auto FileInformationClass = arg<wow64::FILE_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[309]);

        on_func(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryInformationJobObject(proc_t proc, const on_ZwQueryInformationJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryInformationJobObject@20", [=]
    {
        auto& core = d_->core;

        const auto JobHandle                  = arg<wow64::HANDLE>(core, 0);
        const auto JobObjectInformationClass  = arg<wow64::JOBOBJECTINFOCLASS>(core, 1);
        const auto JobObjectInformation       = arg<wow64::PVOID>(core, 2);
        const auto JobObjectInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength               = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[310]);

        on_func(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryInformationPort(proc_t proc, const on_ZwQueryInformationPort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryInformationPort@20", [=]
    {
        auto& core = d_->core;

        const auto PortHandle           = arg<wow64::HANDLE>(core, 0);
        const auto PortInformationClass = arg<wow64::PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<wow64::PVOID>(core, 2);
        const auto Length               = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength         = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[311]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryInformationProcess(proc_t proc, const on_ZwQueryInformationProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryInformationProcess@20", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle            = arg<wow64::HANDLE>(core, 0);
        const auto ProcessInformationClass  = arg<wow64::PROCESSINFOCLASS>(core, 1);
        const auto ProcessInformation       = arg<wow64::PVOID>(core, 2);
        const auto ProcessInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength             = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[312]);

        on_func(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryInformationResourceManager(proc_t proc, const on_ZwQueryInformationResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryInformationResourceManager@20", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle            = arg<wow64::HANDLE>(core, 0);
        const auto ResourceManagerInformationClass  = arg<wow64::RESOURCEMANAGER_INFORMATION_CLASS>(core, 1);
        const auto ResourceManagerInformation       = arg<wow64::PVOID>(core, 2);
        const auto ResourceManagerInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength                     = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[313]);

        on_func(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryInformationToken(proc_t proc, const on_ZwQueryInformationToken_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryInformationToken@20", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle            = arg<wow64::HANDLE>(core, 0);
        const auto TokenInformationClass  = arg<wow64::TOKEN_INFORMATION_CLASS>(core, 1);
        const auto TokenInformation       = arg<wow64::PVOID>(core, 2);
        const auto TokenInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength           = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[314]);

        on_func(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryInformationTransaction(proc_t proc, const on_ZwQueryInformationTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryInformationTransaction@20", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle            = arg<wow64::HANDLE>(core, 0);
        const auto TransactionInformationClass  = arg<wow64::TRANSACTION_INFORMATION_CLASS>(core, 1);
        const auto TransactionInformation       = arg<wow64::PVOID>(core, 2);
        const auto TransactionInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength                 = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[315]);

        on_func(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryInformationWorkerFactory(proc_t proc, const on_ZwQueryInformationWorkerFactory_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryInformationWorkerFactory@20", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle            = arg<wow64::HANDLE>(core, 0);
        const auto WorkerFactoryInformationClass  = arg<wow64::WORKERFACTORYINFOCLASS>(core, 1);
        const auto WorkerFactoryInformation       = arg<wow64::PVOID>(core, 2);
        const auto WorkerFactoryInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength                   = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[316]);

        on_func(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryKey(proc_t proc, const on_ZwQueryKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryKey@20", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle           = arg<wow64::HANDLE>(core, 0);
        const auto KeyInformationClass = arg<wow64::KEY_INFORMATION_CLASS>(core, 1);
        const auto KeyInformation      = arg<wow64::PVOID>(core, 2);
        const auto Length              = arg<wow64::ULONG>(core, 3);
        const auto ResultLength        = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[317]);

        on_func(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryPortInformationProcess(proc_t proc, const on_ZwQueryPortInformationProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryPortInformationProcess@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[318]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryQuotaInformationFile(proc_t proc, const on_ZwQueryQuotaInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryQuotaInformationFile@36", [=]
    {
        auto& core = d_->core;

        const auto FileHandle        = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock     = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer            = arg<wow64::PVOID>(core, 2);
        const auto Length            = arg<wow64::ULONG>(core, 3);
        const auto ReturnSingleEntry = arg<wow64::BOOLEAN>(core, 4);
        const auto SidList           = arg<wow64::PVOID>(core, 5);
        const auto SidListLength     = arg<wow64::ULONG>(core, 6);
        const auto StartSid          = arg<wow64::PULONG>(core, 7);
        const auto RestartScan       = arg<wow64::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[319]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQuerySection(proc_t proc, const on_ZwQuerySection_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQuerySection@20", [=]
    {
        auto& core = d_->core;

        const auto SectionHandle            = arg<wow64::HANDLE>(core, 0);
        const auto SectionInformationClass  = arg<wow64::SECTION_INFORMATION_CLASS>(core, 1);
        const auto SectionInformation       = arg<wow64::PVOID>(core, 2);
        const auto SectionInformationLength = arg<wow64::SIZE_T>(core, 3);
        const auto ReturnLength             = arg<wow64::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[320]);

        on_func(SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQuerySecurityAttributesToken(proc_t proc, const on_ZwQuerySecurityAttributesToken_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQuerySecurityAttributesToken@24", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle        = arg<wow64::HANDLE>(core, 0);
        const auto Attributes         = arg<wow64::PUNICODE_STRING>(core, 1);
        const auto NumberOfAttributes = arg<wow64::ULONG>(core, 2);
        const auto Buffer             = arg<wow64::PVOID>(core, 3);
        const auto Length             = arg<wow64::ULONG>(core, 4);
        const auto ReturnLength       = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[321]);

        on_func(TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQuerySemaphore(proc_t proc, const on_ZwQuerySemaphore_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQuerySemaphore@20", [=]
    {
        auto& core = d_->core;

        const auto SemaphoreHandle            = arg<wow64::HANDLE>(core, 0);
        const auto SemaphoreInformationClass  = arg<wow64::SEMAPHORE_INFORMATION_CLASS>(core, 1);
        const auto SemaphoreInformation       = arg<wow64::PVOID>(core, 2);
        const auto SemaphoreInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength               = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[322]);

        on_func(SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQuerySymbolicLinkObject(proc_t proc, const on_ZwQuerySymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQuerySymbolicLinkObject@12", [=]
    {
        auto& core = d_->core;

        const auto LinkHandle     = arg<wow64::HANDLE>(core, 0);
        const auto LinkTarget     = arg<wow64::PUNICODE_STRING>(core, 1);
        const auto ReturnedLength = arg<wow64::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[323]);

        on_func(LinkHandle, LinkTarget, ReturnedLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQuerySystemEnvironmentValue(proc_t proc, const on_ZwQuerySystemEnvironmentValue_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQuerySystemEnvironmentValue@16", [=]
    {
        auto& core = d_->core;

        const auto VariableName  = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto VariableValue = arg<wow64::PWSTR>(core, 1);
        const auto ValueLength   = arg<wow64::USHORT>(core, 2);
        const auto ReturnLength  = arg<wow64::PUSHORT>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[324]);

        on_func(VariableName, VariableValue, ValueLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQuerySystemEnvironmentValueEx(proc_t proc, const on_ZwQuerySystemEnvironmentValueEx_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQuerySystemEnvironmentValueEx@20", [=]
    {
        auto& core = d_->core;

        const auto VariableName = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto VendorGuid   = arg<wow64::LPGUID>(core, 1);
        const auto Value        = arg<wow64::PVOID>(core, 2);
        const auto ValueLength  = arg<wow64::PULONG>(core, 3);
        const auto Attributes   = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[325]);

        on_func(VariableName, VendorGuid, Value, ValueLength, Attributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQuerySystemInformationEx(proc_t proc, const on_ZwQuerySystemInformationEx_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQuerySystemInformationEx@24", [=]
    {
        auto& core = d_->core;

        const auto SystemInformationClass  = arg<wow64::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto QueryInformation        = arg<wow64::PVOID>(core, 1);
        const auto QueryInformationLength  = arg<wow64::ULONG>(core, 2);
        const auto SystemInformation       = arg<wow64::PVOID>(core, 3);
        const auto SystemInformationLength = arg<wow64::ULONG>(core, 4);
        const auto ReturnLength            = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[326]);

        on_func(SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryTimer(proc_t proc, const on_ZwQueryTimer_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryTimer@20", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle            = arg<wow64::HANDLE>(core, 0);
        const auto TimerInformationClass  = arg<wow64::TIMER_INFORMATION_CLASS>(core, 1);
        const auto TimerInformation       = arg<wow64::PVOID>(core, 2);
        const auto TimerInformationLength = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength           = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[327]);

        on_func(TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwQueryValueKey(proc_t proc, const on_ZwQueryValueKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwQueryValueKey@24", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle                = arg<wow64::HANDLE>(core, 0);
        const auto ValueName                = arg<wow64::PUNICODE_STRING>(core, 1);
        const auto KeyValueInformationClass = arg<wow64::KEY_VALUE_INFORMATION_CLASS>(core, 2);
        const auto KeyValueInformation      = arg<wow64::PVOID>(core, 3);
        const auto Length                   = arg<wow64::ULONG>(core, 4);
        const auto ResultLength             = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[328]);

        on_func(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRaiseException(proc_t proc, const on_ZwRaiseException_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRaiseException@12", [=]
    {
        auto& core = d_->core;

        const auto ExceptionRecord = arg<wow64::PEXCEPTION_RECORD>(core, 0);
        const auto ContextRecord   = arg<wow64::PCONTEXT>(core, 1);
        const auto FirstChance     = arg<wow64::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[329]);

        on_func(ExceptionRecord, ContextRecord, FirstChance);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRaiseHardError(proc_t proc, const on_ZwRaiseHardError_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRaiseHardError@24", [=]
    {
        auto& core = d_->core;

        const auto ErrorStatus                = arg<wow64::NTSTATUS>(core, 0);
        const auto NumberOfParameters         = arg<wow64::ULONG>(core, 1);
        const auto UnicodeStringParameterMask = arg<wow64::ULONG>(core, 2);
        const auto Parameters                 = arg<wow64::PULONG_PTR>(core, 3);
        const auto ValidResponseOptions       = arg<wow64::ULONG>(core, 4);
        const auto Response                   = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[330]);

        on_func(ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwReadOnlyEnlistment(proc_t proc, const on_ZwReadOnlyEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwReadOnlyEnlistment@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[331]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwReadRequestData(proc_t proc, const on_ZwReadRequestData_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwReadRequestData@24", [=]
    {
        auto& core = d_->core;

        const auto PortHandle        = arg<wow64::HANDLE>(core, 0);
        const auto Message           = arg<wow64::PPORT_MESSAGE>(core, 1);
        const auto DataEntryIndex    = arg<wow64::ULONG>(core, 2);
        const auto Buffer            = arg<wow64::PVOID>(core, 3);
        const auto BufferSize        = arg<wow64::SIZE_T>(core, 4);
        const auto NumberOfBytesRead = arg<wow64::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[332]);

        on_func(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRecoverTransactionManager(proc_t proc, const on_ZwRecoverTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRecoverTransactionManager@4", [=]
    {
        auto& core = d_->core;

        const auto TransactionManagerHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[333]);

        on_func(TransactionManagerHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRegisterProtocolAddressInformation(proc_t proc, const on_ZwRegisterProtocolAddressInformation_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRegisterProtocolAddressInformation@20", [=]
    {
        auto& core = d_->core;

        const auto ResourceManager         = arg<wow64::HANDLE>(core, 0);
        const auto ProtocolId              = arg<wow64::PCRM_PROTOCOL_ID>(core, 1);
        const auto ProtocolInformationSize = arg<wow64::ULONG>(core, 2);
        const auto ProtocolInformation     = arg<wow64::PVOID>(core, 3);
        const auto CreateOptions           = arg<wow64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[334]);

        on_func(ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRegisterThreadTerminatePort(proc_t proc, const on_ZwRegisterThreadTerminatePort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRegisterThreadTerminatePort@4", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[335]);

        on_func(PortHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwReleaseMutant(proc_t proc, const on_ZwReleaseMutant_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwReleaseMutant@8", [=]
    {
        auto& core = d_->core;

        const auto MutantHandle  = arg<wow64::HANDLE>(core, 0);
        const auto PreviousCount = arg<wow64::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[336]);

        on_func(MutantHandle, PreviousCount);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwReleaseWorkerFactoryWorker(proc_t proc, const on_ZwReleaseWorkerFactoryWorker_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwReleaseWorkerFactoryWorker@4", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[337]);

        on_func(WorkerFactoryHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRemoveIoCompletion(proc_t proc, const on_ZwRemoveIoCompletion_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRemoveIoCompletion@20", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle = arg<wow64::HANDLE>(core, 0);
        const auto STARKeyContext     = arg<wow64::PVOID>(core, 1);
        const auto STARApcContext     = arg<wow64::PVOID>(core, 2);
        const auto IoStatusBlock      = arg<wow64::PIO_STATUS_BLOCK>(core, 3);
        const auto Timeout            = arg<wow64::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[338]);

        on_func(IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRemoveIoCompletionEx(proc_t proc, const on_ZwRemoveIoCompletionEx_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRemoveIoCompletionEx@24", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle      = arg<wow64::HANDLE>(core, 0);
        const auto IoCompletionInformation = arg<wow64::PFILE_IO_COMPLETION_INFORMATION>(core, 1);
        const auto Count                   = arg<wow64::ULONG>(core, 2);
        const auto NumEntriesRemoved       = arg<wow64::PULONG>(core, 3);
        const auto Timeout                 = arg<wow64::PLARGE_INTEGER>(core, 4);
        const auto Alertable               = arg<wow64::BOOLEAN>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[339]);

        on_func(IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRemoveProcessDebug(proc_t proc, const on_ZwRemoveProcessDebug_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRemoveProcessDebug@8", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle     = arg<wow64::HANDLE>(core, 0);
        const auto DebugObjectHandle = arg<wow64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[340]);

        on_func(ProcessHandle, DebugObjectHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRenameKey(proc_t proc, const on_ZwRenameKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRenameKey@8", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle = arg<wow64::HANDLE>(core, 0);
        const auto NewName   = arg<wow64::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[341]);

        on_func(KeyHandle, NewName);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwReplaceKey(proc_t proc, const on_ZwReplaceKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwReplaceKey@12", [=]
    {
        auto& core = d_->core;

        const auto NewFile      = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);
        const auto TargetHandle = arg<wow64::HANDLE>(core, 1);
        const auto OldFile      = arg<wow64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[342]);

        on_func(NewFile, TargetHandle, OldFile);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwReplyPort(proc_t proc, const on_ZwReplyPort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwReplyPort@8", [=]
    {
        auto& core = d_->core;

        const auto PortHandle   = arg<wow64::HANDLE>(core, 0);
        const auto ReplyMessage = arg<wow64::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[343]);

        on_func(PortHandle, ReplyMessage);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRequestPort(proc_t proc, const on_ZwRequestPort_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRequestPort@8", [=]
    {
        auto& core = d_->core;

        const auto PortHandle     = arg<wow64::HANDLE>(core, 0);
        const auto RequestMessage = arg<wow64::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[344]);

        on_func(PortHandle, RequestMessage);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwResetWriteWatch(proc_t proc, const on_ZwResetWriteWatch_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwResetWriteWatch@12", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<wow64::HANDLE>(core, 0);
        const auto BaseAddress   = arg<wow64::PVOID>(core, 1);
        const auto RegionSize    = arg<wow64::SIZE_T>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[345]);

        on_func(ProcessHandle, BaseAddress, RegionSize);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwResumeProcess(proc_t proc, const on_ZwResumeProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwResumeProcess@4", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[346]);

        on_func(ProcessHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwResumeThread(proc_t proc, const on_ZwResumeThread_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwResumeThread@8", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle         = arg<wow64::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[347]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwRollbackComplete(proc_t proc, const on_ZwRollbackComplete_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwRollbackComplete@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[348]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetBootEntryOrder(proc_t proc, const on_ZwSetBootEntryOrder_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetBootEntryOrder@8", [=]
    {
        auto& core = d_->core;

        const auto Ids   = arg<wow64::PULONG>(core, 0);
        const auto Count = arg<wow64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[349]);

        on_func(Ids, Count);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetBootOptions(proc_t proc, const on_ZwSetBootOptions_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetBootOptions@8", [=]
    {
        auto& core = d_->core;

        const auto BootOptions    = arg<wow64::PBOOT_OPTIONS>(core, 0);
        const auto FieldsToChange = arg<wow64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[350]);

        on_func(BootOptions, FieldsToChange);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetContextThread(proc_t proc, const on_ZwSetContextThread_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetContextThread@8", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle  = arg<wow64::HANDLE>(core, 0);
        const auto ThreadContext = arg<wow64::PCONTEXT>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[351]);

        on_func(ThreadHandle, ThreadContext);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetDefaultUILanguage(proc_t proc, const on_ZwSetDefaultUILanguage_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetDefaultUILanguage@4", [=]
    {
        auto& core = d_->core;

        const auto DefaultUILanguageId = arg<wow64::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[352]);

        on_func(DefaultUILanguageId);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetEaFile(proc_t proc, const on_ZwSetEaFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetEaFile@16", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer        = arg<wow64::PVOID>(core, 2);
        const auto Length        = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[353]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationDebugObject(proc_t proc, const on_ZwSetInformationDebugObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationDebugObject@20", [=]
    {
        auto& core = d_->core;

        const auto DebugObjectHandle           = arg<wow64::HANDLE>(core, 0);
        const auto DebugObjectInformationClass = arg<wow64::DEBUGOBJECTINFOCLASS>(core, 1);
        const auto DebugInformation            = arg<wow64::PVOID>(core, 2);
        const auto DebugInformationLength      = arg<wow64::ULONG>(core, 3);
        const auto ReturnLength                = arg<wow64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[354]);

        on_func(DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationFile(proc_t proc, const on_ZwSetInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationFile@20", [=]
    {
        auto& core = d_->core;

        const auto FileHandle           = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock        = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto FileInformation      = arg<wow64::PVOID>(core, 2);
        const auto Length               = arg<wow64::ULONG>(core, 3);
        const auto FileInformationClass = arg<wow64::FILE_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[355]);

        on_func(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationJobObject(proc_t proc, const on_ZwSetInformationJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationJobObject@16", [=]
    {
        auto& core = d_->core;

        const auto JobHandle                  = arg<wow64::HANDLE>(core, 0);
        const auto JobObjectInformationClass  = arg<wow64::JOBOBJECTINFOCLASS>(core, 1);
        const auto JobObjectInformation       = arg<wow64::PVOID>(core, 2);
        const auto JobObjectInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[356]);

        on_func(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationKey(proc_t proc, const on_ZwSetInformationKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationKey@16", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle               = arg<wow64::HANDLE>(core, 0);
        const auto KeySetInformationClass  = arg<wow64::KEY_SET_INFORMATION_CLASS>(core, 1);
        const auto KeySetInformation       = arg<wow64::PVOID>(core, 2);
        const auto KeySetInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[357]);

        on_func(KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationObject(proc_t proc, const on_ZwSetInformationObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationObject@16", [=]
    {
        auto& core = d_->core;

        const auto Handle                  = arg<wow64::HANDLE>(core, 0);
        const auto ObjectInformationClass  = arg<wow64::OBJECT_INFORMATION_CLASS>(core, 1);
        const auto ObjectInformation       = arg<wow64::PVOID>(core, 2);
        const auto ObjectInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[358]);

        on_func(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationProcess(proc_t proc, const on_ZwSetInformationProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationProcess@16", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle            = arg<wow64::HANDLE>(core, 0);
        const auto ProcessInformationClass  = arg<wow64::PROCESSINFOCLASS>(core, 1);
        const auto ProcessInformation       = arg<wow64::PVOID>(core, 2);
        const auto ProcessInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[359]);

        on_func(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationResourceManager(proc_t proc, const on_ZwSetInformationResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationResourceManager@16", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle            = arg<wow64::HANDLE>(core, 0);
        const auto ResourceManagerInformationClass  = arg<wow64::RESOURCEMANAGER_INFORMATION_CLASS>(core, 1);
        const auto ResourceManagerInformation       = arg<wow64::PVOID>(core, 2);
        const auto ResourceManagerInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[360]);

        on_func(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationThread(proc_t proc, const on_ZwSetInformationThread_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationThread@16", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle            = arg<wow64::HANDLE>(core, 0);
        const auto ThreadInformationClass  = arg<wow64::THREADINFOCLASS>(core, 1);
        const auto ThreadInformation       = arg<wow64::PVOID>(core, 2);
        const auto ThreadInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[361]);

        on_func(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationToken(proc_t proc, const on_ZwSetInformationToken_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationToken@16", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle            = arg<wow64::HANDLE>(core, 0);
        const auto TokenInformationClass  = arg<wow64::TOKEN_INFORMATION_CLASS>(core, 1);
        const auto TokenInformation       = arg<wow64::PVOID>(core, 2);
        const auto TokenInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[362]);

        on_func(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationTransaction(proc_t proc, const on_ZwSetInformationTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationTransaction@16", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle            = arg<wow64::HANDLE>(core, 0);
        const auto TransactionInformationClass  = arg<wow64::TRANSACTION_INFORMATION_CLASS>(core, 1);
        const auto TransactionInformation       = arg<wow64::PVOID>(core, 2);
        const auto TransactionInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[363]);

        on_func(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationTransactionManager(proc_t proc, const on_ZwSetInformationTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationTransactionManager@16", [=]
    {
        auto& core = d_->core;

        const auto TmHandle                            = arg<wow64::HANDLE>(core, 0);
        const auto TransactionManagerInformationClass  = arg<wow64::TRANSACTIONMANAGER_INFORMATION_CLASS>(core, 1);
        const auto TransactionManagerInformation       = arg<wow64::PVOID>(core, 2);
        const auto TransactionManagerInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[364]);

        on_func(TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetInformationWorkerFactory(proc_t proc, const on_ZwSetInformationWorkerFactory_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetInformationWorkerFactory@16", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle            = arg<wow64::HANDLE>(core, 0);
        const auto WorkerFactoryInformationClass  = arg<wow64::WORKERFACTORYINFOCLASS>(core, 1);
        const auto WorkerFactoryInformation       = arg<wow64::PVOID>(core, 2);
        const auto WorkerFactoryInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[365]);

        on_func(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetLdtEntries(proc_t proc, const on_ZwSetLdtEntries_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetLdtEntries@24", [=]
    {
        auto& core = d_->core;

        const auto Selector0 = arg<wow64::ULONG>(core, 0);
        const auto Entry0Low = arg<wow64::ULONG>(core, 1);
        const auto Entry0Hi  = arg<wow64::ULONG>(core, 2);
        const auto Selector1 = arg<wow64::ULONG>(core, 3);
        const auto Entry1Low = arg<wow64::ULONG>(core, 4);
        const auto Entry1Hi  = arg<wow64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[366]);

        on_func(Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetLowEventPair(proc_t proc, const on_ZwSetLowEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetLowEventPair@4", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[367]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetLowWaitHighEventPair(proc_t proc, const on_ZwSetLowWaitHighEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetLowWaitHighEventPair@4", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[368]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetQuotaInformationFile(proc_t proc, const on_ZwSetQuotaInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetQuotaInformationFile@16", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer        = arg<wow64::PVOID>(core, 2);
        const auto Length        = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[369]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetSystemEnvironmentValue(proc_t proc, const on_ZwSetSystemEnvironmentValue_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetSystemEnvironmentValue@8", [=]
    {
        auto& core = d_->core;

        const auto VariableName  = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto VariableValue = arg<wow64::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[370]);

        on_func(VariableName, VariableValue);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetSystemEnvironmentValueEx(proc_t proc, const on_ZwSetSystemEnvironmentValueEx_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetSystemEnvironmentValueEx@20", [=]
    {
        auto& core = d_->core;

        const auto VariableName = arg<wow64::PUNICODE_STRING>(core, 0);
        const auto VendorGuid   = arg<wow64::LPGUID>(core, 1);
        const auto Value        = arg<wow64::PVOID>(core, 2);
        const auto ValueLength  = arg<wow64::ULONG>(core, 3);
        const auto Attributes   = arg<wow64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[371]);

        on_func(VariableName, VendorGuid, Value, ValueLength, Attributes);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetSystemInformation(proc_t proc, const on_ZwSetSystemInformation_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetSystemInformation@12", [=]
    {
        auto& core = d_->core;

        const auto SystemInformationClass  = arg<wow64::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto SystemInformation       = arg<wow64::PVOID>(core, 1);
        const auto SystemInformationLength = arg<wow64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[372]);

        on_func(SystemInformationClass, SystemInformation, SystemInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetSystemPowerState(proc_t proc, const on_ZwSetSystemPowerState_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetSystemPowerState@12", [=]
    {
        auto& core = d_->core;

        const auto SystemAction   = arg<wow64::POWER_ACTION>(core, 0);
        const auto MinSystemState = arg<wow64::SYSTEM_POWER_STATE>(core, 1);
        const auto Flags          = arg<wow64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[373]);

        on_func(SystemAction, MinSystemState, Flags);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetSystemTime(proc_t proc, const on_ZwSetSystemTime_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetSystemTime@8", [=]
    {
        auto& core = d_->core;

        const auto SystemTime   = arg<wow64::PLARGE_INTEGER>(core, 0);
        const auto PreviousTime = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[374]);

        on_func(SystemTime, PreviousTime);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetThreadExecutionState(proc_t proc, const on_ZwSetThreadExecutionState_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetThreadExecutionState@8", [=]
    {
        auto& core = d_->core;

        const auto esFlags           = arg<wow64::EXECUTION_STATE>(core, 0);
        const auto STARPreviousFlags = arg<wow64::EXECUTION_STATE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[375]);

        on_func(esFlags, STARPreviousFlags);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetTimer(proc_t proc, const on_ZwSetTimer_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetTimer@28", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle     = arg<wow64::HANDLE>(core, 0);
        const auto DueTime         = arg<wow64::PLARGE_INTEGER>(core, 1);
        const auto TimerApcRoutine = arg<wow64::PTIMER_APC_ROUTINE>(core, 2);
        const auto TimerContext    = arg<wow64::PVOID>(core, 3);
        const auto WakeTimer       = arg<wow64::BOOLEAN>(core, 4);
        const auto Period          = arg<wow64::LONG>(core, 5);
        const auto PreviousState   = arg<wow64::PBOOLEAN>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[376]);

        on_func(TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetTimerEx(proc_t proc, const on_ZwSetTimerEx_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetTimerEx@16", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle               = arg<wow64::HANDLE>(core, 0);
        const auto TimerSetInformationClass  = arg<wow64::TIMER_SET_INFORMATION_CLASS>(core, 1);
        const auto TimerSetInformation       = arg<wow64::PVOID>(core, 2);
        const auto TimerSetInformationLength = arg<wow64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[377]);

        on_func(TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetUuidSeed(proc_t proc, const on_ZwSetUuidSeed_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetUuidSeed@4", [=]
    {
        auto& core = d_->core;

        const auto Seed = arg<wow64::PCHAR>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[378]);

        on_func(Seed);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSetValueKey(proc_t proc, const on_ZwSetValueKey_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSetValueKey@24", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle  = arg<wow64::HANDLE>(core, 0);
        const auto ValueName  = arg<wow64::PUNICODE_STRING>(core, 1);
        const auto TitleIndex = arg<wow64::ULONG>(core, 2);
        const auto Type       = arg<wow64::ULONG>(core, 3);
        const auto Data       = arg<wow64::PVOID>(core, 4);
        const auto DataSize   = arg<wow64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[379]);

        on_func(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwShutdownSystem(proc_t proc, const on_ZwShutdownSystem_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwShutdownSystem@4", [=]
    {
        auto& core = d_->core;

        const auto Action = arg<wow64::SHUTDOWN_ACTION>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[380]);

        on_func(Action);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSignalAndWaitForSingleObject(proc_t proc, const on_ZwSignalAndWaitForSingleObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSignalAndWaitForSingleObject@16", [=]
    {
        auto& core = d_->core;

        const auto SignalHandle = arg<wow64::HANDLE>(core, 0);
        const auto WaitHandle   = arg<wow64::HANDLE>(core, 1);
        const auto Alertable    = arg<wow64::BOOLEAN>(core, 2);
        const auto Timeout      = arg<wow64::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[381]);

        on_func(SignalHandle, WaitHandle, Alertable, Timeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSinglePhaseReject(proc_t proc, const on_ZwSinglePhaseReject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSinglePhaseReject@8", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<wow64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<wow64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[382]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwStopProfile(proc_t proc, const on_ZwStopProfile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwStopProfile@4", [=]
    {
        auto& core = d_->core;

        const auto ProfileHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[383]);

        on_func(ProfileHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSuspendProcess(proc_t proc, const on_ZwSuspendProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSuspendProcess@4", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[384]);

        on_func(ProcessHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwSuspendThread(proc_t proc, const on_ZwSuspendThread_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwSuspendThread@8", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle         = arg<wow64::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<wow64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[385]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwTerminateJobObject(proc_t proc, const on_ZwTerminateJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwTerminateJobObject@8", [=]
    {
        auto& core = d_->core;

        const auto JobHandle  = arg<wow64::HANDLE>(core, 0);
        const auto ExitStatus = arg<wow64::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[386]);

        on_func(JobHandle, ExitStatus);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwTerminateProcess(proc_t proc, const on_ZwTerminateProcess_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwTerminateProcess@8", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<wow64::HANDLE>(core, 0);
        const auto ExitStatus    = arg<wow64::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[387]);

        on_func(ProcessHandle, ExitStatus);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwTerminateThread(proc_t proc, const on_ZwTerminateThread_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwTerminateThread@8", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle = arg<wow64::HANDLE>(core, 0);
        const auto ExitStatus   = arg<wow64::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[388]);

        on_func(ThreadHandle, ExitStatus);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwThawRegistry(proc_t proc, const on_ZwThawRegistry_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwThawRegistry@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[389]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwTraceControl(proc_t proc, const on_ZwTraceControl_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwTraceControl@24", [=]
    {
        auto& core = d_->core;

        const auto FunctionCode = arg<wow64::ULONG>(core, 0);
        const auto InBuffer     = arg<wow64::PVOID>(core, 1);
        const auto InBufferLen  = arg<wow64::ULONG>(core, 2);
        const auto OutBuffer    = arg<wow64::PVOID>(core, 3);
        const auto OutBufferLen = arg<wow64::ULONG>(core, 4);
        const auto ReturnLength = arg<wow64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[390]);

        on_func(FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwUmsThreadYield(proc_t proc, const on_ZwUmsThreadYield_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwUmsThreadYield@4", [=]
    {
        auto& core = d_->core;

        const auto SchedulerParam = arg<wow64::PVOID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[391]);

        on_func(SchedulerParam);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwUnloadDriver(proc_t proc, const on_ZwUnloadDriver_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwUnloadDriver@4", [=]
    {
        auto& core = d_->core;

        const auto DriverServiceName = arg<wow64::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[392]);

        on_func(DriverServiceName);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwUnloadKey2(proc_t proc, const on_ZwUnloadKey2_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwUnloadKey2@8", [=]
    {
        auto& core = d_->core;

        const auto TargetKey = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);
        const auto Flags     = arg<wow64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[393]);

        on_func(TargetKey, Flags);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwUnloadKeyEx(proc_t proc, const on_ZwUnloadKeyEx_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwUnloadKeyEx@8", [=]
    {
        auto& core = d_->core;

        const auto TargetKey = arg<wow64::POBJECT_ATTRIBUTES>(core, 0);
        const auto Event     = arg<wow64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[394]);

        on_func(TargetKey, Event);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwUnlockFile(proc_t proc, const on_ZwUnlockFile_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwUnlockFile@20", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<wow64::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<wow64::PIO_STATUS_BLOCK>(core, 1);
        const auto ByteOffset    = arg<wow64::PLARGE_INTEGER>(core, 2);
        const auto Length        = arg<wow64::PLARGE_INTEGER>(core, 3);
        const auto Key           = arg<wow64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[395]);

        on_func(FileHandle, IoStatusBlock, ByteOffset, Length, Key);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwWaitForSingleObject(proc_t proc, const on_ZwWaitForSingleObject_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwWaitForSingleObject@12", [=]
    {
        auto& core = d_->core;

        const auto Handle    = arg<wow64::HANDLE>(core, 0);
        const auto Alertable = arg<wow64::BOOLEAN>(core, 1);
        const auto Timeout   = arg<wow64::PLARGE_INTEGER>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[396]);

        on_func(Handle, Alertable, Timeout);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwWaitHighEventPair(proc_t proc, const on_ZwWaitHighEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwWaitHighEventPair@4", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<wow64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[397]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> wow64::syscalls32::register_ZwYieldExecution(proc_t proc, const on_ZwYieldExecution_fn& on_func)
{
    return register_callback(*d_, proc, "_ZwYieldExecution@0", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[398]);

        on_func();
    });
}

opt<bpid_t> wow64::syscalls32::register_all(proc_t proc, const wow64::syscalls32::on_call_fn& on_call)
{
    auto& d         = *d_;
    const auto bpid = state::acquire_breakpoint_id(d.core);
    for(const auto cfg : g_callcfgs)
        register_callback_with(d, bpid, proc, cfg.name, [=]{ on_call(cfg); });
    return bpid;
}
