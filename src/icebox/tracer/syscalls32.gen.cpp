#include "syscalls32.gen.hpp"

#define FDP_MODULE "syscalls32"
#include "log.hpp"
#include "os.hpp"

namespace
{
	constexpr bool g_debug = false;

	static const tracer::callcfg_t g_callcfgs[] =
	{
        {"_NtAcceptConnectPort@24", 6, {{"PHANDLE", "PortHandle", sizeof(nt32::PHANDLE)}, {"PVOID", "PortContext", sizeof(nt32::PVOID)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(nt32::PPORT_MESSAGE)}, {"BOOLEAN", "AcceptConnection", sizeof(nt32::BOOLEAN)}, {"PPORT_VIEW", "ServerView", sizeof(nt32::PPORT_VIEW)}, {"PREMOTE_PORT_VIEW", "ClientView", sizeof(nt32::PREMOTE_PORT_VIEW)}}},
        {"_ZwAccessCheckAndAuditAlarm@44", 11, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt32::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt32::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt32::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt32::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt32::PSECURITY_DESCRIPTOR)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt32::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt32::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt32::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt32::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt32::PBOOLEAN)}}},
        {"_NtAccessCheckByTypeAndAuditAlarm@64", 16, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt32::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt32::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt32::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt32::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt32::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt32::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(nt32::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt32::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt32::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt32::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt32::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt32::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt32::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt32::PBOOLEAN)}}},
        {"_NtAccessCheckByType@44", 11, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt32::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt32::PSID)}, {"HANDLE", "ClientToken", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt32::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt32::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt32::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(nt32::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(nt32::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt32::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt32::PNTSTATUS)}}},
        {"_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle@68", 17, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt32::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt32::PVOID)}, {"HANDLE", "ClientToken", sizeof(nt32::HANDLE)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt32::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt32::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt32::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt32::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(nt32::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt32::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt32::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt32::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt32::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt32::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt32::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt32::PBOOLEAN)}}},
        {"_NtAccessCheckByTypeResultListAndAuditAlarm@64", 16, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt32::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt32::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt32::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt32::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt32::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt32::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(nt32::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt32::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt32::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt32::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt32::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt32::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt32::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt32::PBOOLEAN)}}},
        {"_NtAccessCheckByTypeResultList@44", 11, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt32::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt32::PSID)}, {"HANDLE", "ClientToken", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt32::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt32::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt32::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(nt32::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(nt32::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt32::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt32::PNTSTATUS)}}},
        {"_NtAccessCheck@32", 8, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt32::PSECURITY_DESCRIPTOR)}, {"HANDLE", "ClientToken", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt32::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(nt32::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(nt32::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt32::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt32::PNTSTATUS)}}},
        {"_NtAddAtom@12", 3, {{"PWSTR", "AtomName", sizeof(nt32::PWSTR)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PRTL_ATOM", "Atom", sizeof(nt32::PRTL_ATOM)}}},
        {"_ZwAddBootEntry@8", 2, {{"PBOOT_ENTRY", "BootEntry", sizeof(nt32::PBOOT_ENTRY)}, {"PULONG", "Id", sizeof(nt32::PULONG)}}},
        {"_NtAddDriverEntry@8", 2, {{"PEFI_DRIVER_ENTRY", "DriverEntry", sizeof(nt32::PEFI_DRIVER_ENTRY)}, {"PULONG", "Id", sizeof(nt32::PULONG)}}},
        {"_ZwAdjustGroupsToken@24", 6, {{"HANDLE", "TokenHandle", sizeof(nt32::HANDLE)}, {"BOOLEAN", "ResetToDefault", sizeof(nt32::BOOLEAN)}, {"PTOKEN_GROUPS", "NewState", sizeof(nt32::PTOKEN_GROUPS)}, {"ULONG", "BufferLength", sizeof(nt32::ULONG)}, {"PTOKEN_GROUPS", "PreviousState", sizeof(nt32::PTOKEN_GROUPS)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwAdjustPrivilegesToken@24", 6, {{"HANDLE", "TokenHandle", sizeof(nt32::HANDLE)}, {"BOOLEAN", "DisableAllPrivileges", sizeof(nt32::BOOLEAN)}, {"PTOKEN_PRIVILEGES", "NewState", sizeof(nt32::PTOKEN_PRIVILEGES)}, {"ULONG", "BufferLength", sizeof(nt32::ULONG)}, {"PTOKEN_PRIVILEGES", "PreviousState", sizeof(nt32::PTOKEN_PRIVILEGES)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtAlertResumeThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(nt32::PULONG)}}},
        {"_NtAlertThread@4", 1, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwAllocateLocallyUniqueId@4", 1, {{"PLUID", "Luid", sizeof(nt32::PLUID)}}},
        {"_NtAllocateReserveObject@12", 3, {{"PHANDLE", "MemoryReserveHandle", sizeof(nt32::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"MEMORY_RESERVE_TYPE", "Type", sizeof(nt32::MEMORY_RESERVE_TYPE)}}},
        {"_NtAllocateUserPhysicalPages@12", 3, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PULONG_PTR", "NumberOfPages", sizeof(nt32::PULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(nt32::PULONG_PTR)}}},
        {"_NtAllocateUuids@16", 4, {{"PULARGE_INTEGER", "Time", sizeof(nt32::PULARGE_INTEGER)}, {"PULONG", "Range", sizeof(nt32::PULONG)}, {"PULONG", "Sequence", sizeof(nt32::PULONG)}, {"PCHAR", "Seed", sizeof(nt32::PCHAR)}}},
        {"_NtAllocateVirtualMemory@24", 6, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt32::PVOID)}, {"ULONG_PTR", "ZeroBits", sizeof(nt32::ULONG_PTR)}, {"PSIZE_T", "RegionSize", sizeof(nt32::PSIZE_T)}, {"ULONG", "AllocationType", sizeof(nt32::ULONG)}, {"ULONG", "Protect", sizeof(nt32::ULONG)}}},
        {"_NtAlpcAcceptConnectPort@36", 9, {{"PHANDLE", "PortHandle", sizeof(nt32::PHANDLE)}, {"HANDLE", "ConnectionPortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(nt32::PALPC_PORT_ATTRIBUTES)}, {"PVOID", "PortContext", sizeof(nt32::PVOID)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(nt32::PPORT_MESSAGE)}, {"PALPC_MESSAGE_ATTRIBUTES", "ConnectionMessageAttributes", sizeof(nt32::PALPC_MESSAGE_ATTRIBUTES)}, {"BOOLEAN", "AcceptConnection", sizeof(nt32::BOOLEAN)}}},
        {"_ZwAlpcCancelMessage@12", 3, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PALPC_CONTEXT_ATTR", "MessageContext", sizeof(nt32::PALPC_CONTEXT_ATTR)}}},
        {"_ZwAlpcConnectPort@44", 11, {{"PHANDLE", "PortHandle", sizeof(nt32::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(nt32::PUNICODE_STRING)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(nt32::PALPC_PORT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PSID", "RequiredServerSid", sizeof(nt32::PSID)}, {"PPORT_MESSAGE", "ConnectionMessage", sizeof(nt32::PPORT_MESSAGE)}, {"PULONG", "BufferLength", sizeof(nt32::PULONG)}, {"PALPC_MESSAGE_ATTRIBUTES", "OutMessageAttributes", sizeof(nt32::PALPC_MESSAGE_ATTRIBUTES)}, {"PALPC_MESSAGE_ATTRIBUTES", "InMessageAttributes", sizeof(nt32::PALPC_MESSAGE_ATTRIBUTES)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwAlpcCreatePort@12", 3, {{"PHANDLE", "PortHandle", sizeof(nt32::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(nt32::PALPC_PORT_ATTRIBUTES)}}},
        {"_NtAlpcCreatePortSection@24", 6, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"HANDLE", "SectionHandle", sizeof(nt32::HANDLE)}, {"SIZE_T", "SectionSize", sizeof(nt32::SIZE_T)}, {"PALPC_HANDLE", "AlpcSectionHandle", sizeof(nt32::PALPC_HANDLE)}, {"PSIZE_T", "ActualSectionSize", sizeof(nt32::PSIZE_T)}}},
        {"_ZwAlpcCreateResourceReserve@16", 4, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"SIZE_T", "MessageSize", sizeof(nt32::SIZE_T)}, {"PALPC_HANDLE", "ResourceId", sizeof(nt32::PALPC_HANDLE)}}},
        {"_ZwAlpcCreateSectionView@12", 3, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PALPC_DATA_VIEW_ATTR", "ViewAttributes", sizeof(nt32::PALPC_DATA_VIEW_ATTR)}}},
        {"_ZwAlpcCreateSecurityContext@12", 3, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PALPC_SECURITY_ATTR", "SecurityAttribute", sizeof(nt32::PALPC_SECURITY_ATTR)}}},
        {"_ZwAlpcDeletePortSection@12", 3, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"ALPC_HANDLE", "SectionHandle", sizeof(nt32::ALPC_HANDLE)}}},
        {"_NtAlpcDeleteResourceReserve@12", 3, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"ALPC_HANDLE", "ResourceId", sizeof(nt32::ALPC_HANDLE)}}},
        {"_NtAlpcDeleteSectionView@12", 3, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PVOID", "ViewBase", sizeof(nt32::PVOID)}}},
        {"_NtAlpcDeleteSecurityContext@12", 3, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"ALPC_HANDLE", "ContextHandle", sizeof(nt32::ALPC_HANDLE)}}},
        {"_NtAlpcDisconnectPort@8", 2, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}}},
        {"_ZwAlpcImpersonateClientOfPort@12", 3, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt32::PPORT_MESSAGE)}, {"PVOID", "Reserved", sizeof(nt32::PVOID)}}},
        {"_ZwAlpcOpenSenderProcess@24", 6, {{"PHANDLE", "ProcessHandle", sizeof(nt32::PHANDLE)}, {"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt32::PPORT_MESSAGE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwAlpcOpenSenderThread@24", 6, {{"PHANDLE", "ThreadHandle", sizeof(nt32::PHANDLE)}, {"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt32::PPORT_MESSAGE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwAlpcQueryInformation@20", 5, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ALPC_PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(nt32::ALPC_PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwAlpcQueryInformationMessage@24", 6, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt32::PPORT_MESSAGE)}, {"ALPC_MESSAGE_INFORMATION_CLASS", "MessageInformationClass", sizeof(nt32::ALPC_MESSAGE_INFORMATION_CLASS)}, {"PVOID", "MessageInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtAlpcRevokeSecurityContext@12", 3, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"ALPC_HANDLE", "ContextHandle", sizeof(nt32::ALPC_HANDLE)}}},
        {"_NtAlpcSendWaitReceivePort@32", 8, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PPORT_MESSAGE", "SendMessage", sizeof(nt32::PPORT_MESSAGE)}, {"PALPC_MESSAGE_ATTRIBUTES", "SendMessageAttributes", sizeof(nt32::PALPC_MESSAGE_ATTRIBUTES)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(nt32::PPORT_MESSAGE)}, {"PULONG", "BufferLength", sizeof(nt32::PULONG)}, {"PALPC_MESSAGE_ATTRIBUTES", "ReceiveMessageAttributes", sizeof(nt32::PALPC_MESSAGE_ATTRIBUTES)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtAlpcSetInformation@16", 4, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"ALPC_PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(nt32::ALPC_PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}}},
        {"_NtApphelpCacheControl@8", 2, {{"APPHELPCOMMAND", "type", sizeof(nt32::APPHELPCOMMAND)}, {"PVOID", "buf", sizeof(nt32::PVOID)}}},
        {"_ZwAreMappedFilesTheSame@8", 2, {{"PVOID", "File1MappedAsAnImage", sizeof(nt32::PVOID)}, {"PVOID", "File2MappedAsFile", sizeof(nt32::PVOID)}}},
        {"_ZwAssignProcessToJobObject@8", 2, {{"HANDLE", "JobHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}}},
        {"_NtCancelIoFileEx@12", 3, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoRequestToCancel", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}}},
        {"_ZwCancelIoFile@8", 2, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}}},
        {"_NtCancelSynchronousIoFile@12", 3, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoRequestToCancel", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}}},
        {"_ZwCancelTimer@8", 2, {{"HANDLE", "TimerHandle", sizeof(nt32::HANDLE)}, {"PBOOLEAN", "CurrentState", sizeof(nt32::PBOOLEAN)}}},
        {"_NtClearEvent@4", 1, {{"HANDLE", "EventHandle", sizeof(nt32::HANDLE)}}},
        {"_NtClose@4", 1, {{"HANDLE", "Handle", sizeof(nt32::HANDLE)}}},
        {"_ZwCloseObjectAuditAlarm@12", 3, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt32::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt32::PVOID)}, {"BOOLEAN", "GenerateOnClose", sizeof(nt32::BOOLEAN)}}},
        {"_ZwCommitComplete@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtCommitEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtCommitTransaction@8", 2, {{"HANDLE", "TransactionHandle", sizeof(nt32::HANDLE)}, {"BOOLEAN", "Wait", sizeof(nt32::BOOLEAN)}}},
        {"_NtCompactKeys@8", 2, {{"ULONG", "Count", sizeof(nt32::ULONG)}, {"HANDLE", "KeyArray", sizeof(nt32::HANDLE)}}},
        {"_ZwCompareTokens@12", 3, {{"HANDLE", "FirstTokenHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "SecondTokenHandle", sizeof(nt32::HANDLE)}, {"PBOOLEAN", "Equal", sizeof(nt32::PBOOLEAN)}}},
        {"_NtCompleteConnectPort@4", 1, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwCompressKey@4", 1, {{"HANDLE", "Key", sizeof(nt32::HANDLE)}}},
        {"_NtConnectPort@32", 8, {{"PHANDLE", "PortHandle", sizeof(nt32::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(nt32::PUNICODE_STRING)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(nt32::PSECURITY_QUALITY_OF_SERVICE)}, {"PPORT_VIEW", "ClientView", sizeof(nt32::PPORT_VIEW)}, {"PREMOTE_PORT_VIEW", "ServerView", sizeof(nt32::PREMOTE_PORT_VIEW)}, {"PULONG", "MaxMessageLength", sizeof(nt32::PULONG)}, {"PVOID", "ConnectionInformation", sizeof(nt32::PVOID)}, {"PULONG", "ConnectionInformationLength", sizeof(nt32::PULONG)}}},
        {"_ZwContinue@8", 2, {{"PCONTEXT", "ContextRecord", sizeof(nt32::PCONTEXT)}, {"BOOLEAN", "TestAlert", sizeof(nt32::BOOLEAN)}}},
        {"_ZwCreateDebugObject@16", 4, {{"PHANDLE", "DebugObjectHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}}},
        {"_ZwCreateDirectoryObject@12", 3, {{"PHANDLE", "DirectoryHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwCreateEnlistment@32", 8, {{"PHANDLE", "EnlistmentHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"HANDLE", "ResourceManagerHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "TransactionHandle", sizeof(nt32::HANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "CreateOptions", sizeof(nt32::ULONG)}, {"NOTIFICATION_MASK", "NotificationMask", sizeof(nt32::NOTIFICATION_MASK)}, {"PVOID", "EnlistmentKey", sizeof(nt32::PVOID)}}},
        {"_NtCreateEvent@20", 5, {{"PHANDLE", "EventHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"EVENT_TYPE", "EventType", sizeof(nt32::EVENT_TYPE)}, {"BOOLEAN", "InitialState", sizeof(nt32::BOOLEAN)}}},
        {"_NtCreateEventPair@12", 3, {{"PHANDLE", "EventPairHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtCreateFile@44", 11, {{"PHANDLE", "FileHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "AllocationSize", sizeof(nt32::PLARGE_INTEGER)}, {"ULONG", "FileAttributes", sizeof(nt32::ULONG)}, {"ULONG", "ShareAccess", sizeof(nt32::ULONG)}, {"ULONG", "CreateDisposition", sizeof(nt32::ULONG)}, {"ULONG", "CreateOptions", sizeof(nt32::ULONG)}, {"PVOID", "EaBuffer", sizeof(nt32::PVOID)}, {"ULONG", "EaLength", sizeof(nt32::ULONG)}}},
        {"_NtCreateIoCompletion@16", 4, {{"PHANDLE", "IoCompletionHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "Count", sizeof(nt32::ULONG)}}},
        {"_ZwCreateJobObject@12", 3, {{"PHANDLE", "JobHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtCreateJobSet@12", 3, {{"ULONG", "NumJob", sizeof(nt32::ULONG)}, {"PJOB_SET_ARRAY", "UserJobSet", sizeof(nt32::PJOB_SET_ARRAY)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}}},
        {"_ZwCreateKeyedEvent@16", 4, {{"PHANDLE", "KeyedEventHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}}},
        {"_ZwCreateKey@28", 7, {{"PHANDLE", "KeyHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "TitleIndex", sizeof(nt32::ULONG)}, {"PUNICODE_STRING", "Class", sizeof(nt32::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(nt32::ULONG)}, {"PULONG", "Disposition", sizeof(nt32::PULONG)}}},
        {"_NtCreateKeyTransacted@32", 8, {{"PHANDLE", "KeyHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "TitleIndex", sizeof(nt32::ULONG)}, {"PUNICODE_STRING", "Class", sizeof(nt32::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(nt32::ULONG)}, {"HANDLE", "TransactionHandle", sizeof(nt32::HANDLE)}, {"PULONG", "Disposition", sizeof(nt32::PULONG)}}},
        {"_ZwCreateMailslotFile@32", 8, {{"PHANDLE", "FileHandle", sizeof(nt32::PHANDLE)}, {"ULONG", "DesiredAccess", sizeof(nt32::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"ULONG", "CreateOptions", sizeof(nt32::ULONG)}, {"ULONG", "MailslotQuota", sizeof(nt32::ULONG)}, {"ULONG", "MaximumMessageSize", sizeof(nt32::ULONG)}, {"PLARGE_INTEGER", "ReadTimeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwCreateMutant@16", 4, {{"PHANDLE", "MutantHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"BOOLEAN", "InitialOwner", sizeof(nt32::BOOLEAN)}}},
        {"_ZwCreateNamedPipeFile@56", 14, {{"PHANDLE", "FileHandle", sizeof(nt32::PHANDLE)}, {"ULONG", "DesiredAccess", sizeof(nt32::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"ULONG", "ShareAccess", sizeof(nt32::ULONG)}, {"ULONG", "CreateDisposition", sizeof(nt32::ULONG)}, {"ULONG", "CreateOptions", sizeof(nt32::ULONG)}, {"ULONG", "NamedPipeType", sizeof(nt32::ULONG)}, {"ULONG", "ReadMode", sizeof(nt32::ULONG)}, {"ULONG", "CompletionMode", sizeof(nt32::ULONG)}, {"ULONG", "MaximumInstances", sizeof(nt32::ULONG)}, {"ULONG", "InboundQuota", sizeof(nt32::ULONG)}, {"ULONG", "OutboundQuota", sizeof(nt32::ULONG)}, {"PLARGE_INTEGER", "DefaultTimeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtCreatePagingFile@16", 4, {{"PUNICODE_STRING", "PageFileName", sizeof(nt32::PUNICODE_STRING)}, {"PLARGE_INTEGER", "MinimumSize", sizeof(nt32::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "MaximumSize", sizeof(nt32::PLARGE_INTEGER)}, {"ULONG", "Priority", sizeof(nt32::ULONG)}}},
        {"_ZwCreatePort@20", 5, {{"PHANDLE", "PortHandle", sizeof(nt32::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "MaxConnectionInfoLength", sizeof(nt32::ULONG)}, {"ULONG", "MaxMessageLength", sizeof(nt32::ULONG)}, {"ULONG", "MaxPoolUsage", sizeof(nt32::ULONG)}}},
        {"_NtCreatePrivateNamespace@16", 4, {{"PHANDLE", "NamespaceHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PVOID", "BoundaryDescriptor", sizeof(nt32::PVOID)}}},
        {"_ZwCreateProcessEx@36", 9, {{"PHANDLE", "ProcessHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"HANDLE", "ParentProcess", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"HANDLE", "SectionHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "DebugPort", sizeof(nt32::HANDLE)}, {"HANDLE", "ExceptionPort", sizeof(nt32::HANDLE)}, {"ULONG", "JobMemberLevel", sizeof(nt32::ULONG)}}},
        {"_ZwCreateProcess@32", 8, {{"PHANDLE", "ProcessHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"HANDLE", "ParentProcess", sizeof(nt32::HANDLE)}, {"BOOLEAN", "InheritObjectTable", sizeof(nt32::BOOLEAN)}, {"HANDLE", "SectionHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "DebugPort", sizeof(nt32::HANDLE)}, {"HANDLE", "ExceptionPort", sizeof(nt32::HANDLE)}}},
        {"_NtCreateProfileEx@40", 10, {{"PHANDLE", "ProfileHandle", sizeof(nt32::PHANDLE)}, {"HANDLE", "Process", sizeof(nt32::HANDLE)}, {"PVOID", "ProfileBase", sizeof(nt32::PVOID)}, {"SIZE_T", "ProfileSize", sizeof(nt32::SIZE_T)}, {"ULONG", "BucketSize", sizeof(nt32::ULONG)}, {"PULONG", "Buffer", sizeof(nt32::PULONG)}, {"ULONG", "BufferSize", sizeof(nt32::ULONG)}, {"KPROFILE_SOURCE", "ProfileSource", sizeof(nt32::KPROFILE_SOURCE)}, {"ULONG", "GroupAffinityCount", sizeof(nt32::ULONG)}, {"PGROUP_AFFINITY", "GroupAffinity", sizeof(nt32::PGROUP_AFFINITY)}}},
        {"_ZwCreateProfile@36", 9, {{"PHANDLE", "ProfileHandle", sizeof(nt32::PHANDLE)}, {"HANDLE", "Process", sizeof(nt32::HANDLE)}, {"PVOID", "RangeBase", sizeof(nt32::PVOID)}, {"SIZE_T", "RangeSize", sizeof(nt32::SIZE_T)}, {"ULONG", "BucketSize", sizeof(nt32::ULONG)}, {"PULONG", "Buffer", sizeof(nt32::PULONG)}, {"ULONG", "BufferSize", sizeof(nt32::ULONG)}, {"KPROFILE_SOURCE", "ProfileSource", sizeof(nt32::KPROFILE_SOURCE)}, {"KAFFINITY", "Affinity", sizeof(nt32::KAFFINITY)}}},
        {"_ZwCreateResourceManager@28", 7, {{"PHANDLE", "ResourceManagerHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"HANDLE", "TmHandle", sizeof(nt32::HANDLE)}, {"LPGUID", "RmGuid", sizeof(nt32::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "CreateOptions", sizeof(nt32::ULONG)}, {"PUNICODE_STRING", "Description", sizeof(nt32::PUNICODE_STRING)}}},
        {"_NtCreateSection@28", 7, {{"PHANDLE", "SectionHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PLARGE_INTEGER", "MaximumSize", sizeof(nt32::PLARGE_INTEGER)}, {"ULONG", "SectionPageProtection", sizeof(nt32::ULONG)}, {"ULONG", "AllocationAttributes", sizeof(nt32::ULONG)}, {"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}}},
        {"_NtCreateSemaphore@20", 5, {{"PHANDLE", "SemaphoreHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"LONG", "InitialCount", sizeof(nt32::LONG)}, {"LONG", "MaximumCount", sizeof(nt32::LONG)}}},
        {"_ZwCreateSymbolicLinkObject@16", 4, {{"PHANDLE", "LinkHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LinkTarget", sizeof(nt32::PUNICODE_STRING)}}},
        {"_NtCreateThreadEx@44", 11, {{"PHANDLE", "ThreadHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "StartRoutine", sizeof(nt32::PVOID)}, {"PVOID", "Argument", sizeof(nt32::PVOID)}, {"ULONG", "CreateFlags", sizeof(nt32::ULONG)}, {"ULONG_PTR", "ZeroBits", sizeof(nt32::ULONG_PTR)}, {"SIZE_T", "StackSize", sizeof(nt32::SIZE_T)}, {"SIZE_T", "MaximumStackSize", sizeof(nt32::SIZE_T)}, {"PPS_ATTRIBUTE_LIST", "AttributeList", sizeof(nt32::PPS_ATTRIBUTE_LIST)}}},
        {"_NtCreateThread@32", 8, {{"PHANDLE", "ThreadHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PCLIENT_ID", "ClientId", sizeof(nt32::PCLIENT_ID)}, {"PCONTEXT", "ThreadContext", sizeof(nt32::PCONTEXT)}, {"PINITIAL_TEB", "InitialTeb", sizeof(nt32::PINITIAL_TEB)}, {"BOOLEAN", "CreateSuspended", sizeof(nt32::BOOLEAN)}}},
        {"_ZwCreateTimer@16", 4, {{"PHANDLE", "TimerHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"TIMER_TYPE", "TimerType", sizeof(nt32::TIMER_TYPE)}}},
        {"_NtCreateToken@52", 13, {{"PHANDLE", "TokenHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"TOKEN_TYPE", "TokenType", sizeof(nt32::TOKEN_TYPE)}, {"PLUID", "AuthenticationId", sizeof(nt32::PLUID)}, {"PLARGE_INTEGER", "ExpirationTime", sizeof(nt32::PLARGE_INTEGER)}, {"PTOKEN_USER", "User", sizeof(nt32::PTOKEN_USER)}, {"PTOKEN_GROUPS", "Groups", sizeof(nt32::PTOKEN_GROUPS)}, {"PTOKEN_PRIVILEGES", "Privileges", sizeof(nt32::PTOKEN_PRIVILEGES)}, {"PTOKEN_OWNER", "Owner", sizeof(nt32::PTOKEN_OWNER)}, {"PTOKEN_PRIMARY_GROUP", "PrimaryGroup", sizeof(nt32::PTOKEN_PRIMARY_GROUP)}, {"PTOKEN_DEFAULT_DACL", "DefaultDacl", sizeof(nt32::PTOKEN_DEFAULT_DACL)}, {"PTOKEN_SOURCE", "TokenSource", sizeof(nt32::PTOKEN_SOURCE)}}},
        {"_ZwCreateTransactionManager@24", 6, {{"PHANDLE", "TmHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LogFileName", sizeof(nt32::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(nt32::ULONG)}, {"ULONG", "CommitStrength", sizeof(nt32::ULONG)}}},
        {"_NtCreateTransaction@40", 10, {{"PHANDLE", "TransactionHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"LPGUID", "Uow", sizeof(nt32::LPGUID)}, {"HANDLE", "TmHandle", sizeof(nt32::HANDLE)}, {"ULONG", "CreateOptions", sizeof(nt32::ULONG)}, {"ULONG", "IsolationLevel", sizeof(nt32::ULONG)}, {"ULONG", "IsolationFlags", sizeof(nt32::ULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}, {"PUNICODE_STRING", "Description", sizeof(nt32::PUNICODE_STRING)}}},
        {"_NtCreateUserProcess@44", 11, {{"PHANDLE", "ProcessHandle", sizeof(nt32::PHANDLE)}, {"PHANDLE", "ThreadHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "ProcessDesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"ACCESS_MASK", "ThreadDesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ProcessObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "ThreadObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "ProcessFlags", sizeof(nt32::ULONG)}, {"ULONG", "ThreadFlags", sizeof(nt32::ULONG)}, {"PRTL_USER_PROCESS_PARAMETERS", "ProcessParameters", sizeof(nt32::PRTL_USER_PROCESS_PARAMETERS)}, {"PPROCESS_CREATE_INFO", "CreateInfo", sizeof(nt32::PPROCESS_CREATE_INFO)}, {"PPROCESS_ATTRIBUTE_LIST", "AttributeList", sizeof(nt32::PPROCESS_ATTRIBUTE_LIST)}}},
        {"_ZwCreateWaitablePort@20", 5, {{"PHANDLE", "PortHandle", sizeof(nt32::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "MaxConnectionInfoLength", sizeof(nt32::ULONG)}, {"ULONG", "MaxMessageLength", sizeof(nt32::ULONG)}, {"ULONG", "MaxPoolUsage", sizeof(nt32::ULONG)}}},
        {"_NtCreateWorkerFactory@40", 10, {{"PHANDLE", "WorkerFactoryHandleReturn", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"HANDLE", "CompletionPortHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "WorkerProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "StartRoutine", sizeof(nt32::PVOID)}, {"PVOID", "StartParameter", sizeof(nt32::PVOID)}, {"ULONG", "MaxThreadCount", sizeof(nt32::ULONG)}, {"SIZE_T", "StackReserve", sizeof(nt32::SIZE_T)}, {"SIZE_T", "StackCommit", sizeof(nt32::SIZE_T)}}},
        {"_NtDebugActiveProcess@8", 2, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "DebugObjectHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwDebugContinue@12", 3, {{"HANDLE", "DebugObjectHandle", sizeof(nt32::HANDLE)}, {"PCLIENT_ID", "ClientId", sizeof(nt32::PCLIENT_ID)}, {"NTSTATUS", "ContinueStatus", sizeof(nt32::NTSTATUS)}}},
        {"_ZwDelayExecution@8", 2, {{"BOOLEAN", "Alertable", sizeof(nt32::BOOLEAN)}, {"PLARGE_INTEGER", "DelayInterval", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwDeleteAtom@4", 1, {{"RTL_ATOM", "Atom", sizeof(nt32::RTL_ATOM)}}},
        {"_NtDeleteBootEntry@4", 1, {{"ULONG", "Id", sizeof(nt32::ULONG)}}},
        {"_ZwDeleteDriverEntry@4", 1, {{"ULONG", "Id", sizeof(nt32::ULONG)}}},
        {"_NtDeleteFile@4", 1, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwDeleteKey@4", 1, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}}},
        {"_NtDeleteObjectAuditAlarm@12", 3, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt32::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt32::PVOID)}, {"BOOLEAN", "GenerateOnClose", sizeof(nt32::BOOLEAN)}}},
        {"_NtDeletePrivateNamespace@4", 1, {{"HANDLE", "NamespaceHandle", sizeof(nt32::HANDLE)}}},
        {"_NtDeleteValueKey@8", 2, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(nt32::PUNICODE_STRING)}}},
        {"_ZwDeviceIoControlFile@40", 10, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"ULONG", "IoControlCode", sizeof(nt32::ULONG)}, {"PVOID", "InputBuffer", sizeof(nt32::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt32::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt32::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt32::ULONG)}}},
        {"_NtDisplayString@4", 1, {{"PUNICODE_STRING", "String", sizeof(nt32::PUNICODE_STRING)}}},
        {"_ZwDrawText@4", 1, {{"PUNICODE_STRING", "Text", sizeof(nt32::PUNICODE_STRING)}}},
        {"_ZwDuplicateObject@28", 7, {{"HANDLE", "SourceProcessHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "SourceHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "TargetProcessHandle", sizeof(nt32::HANDLE)}, {"PHANDLE", "TargetHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt32::ULONG)}, {"ULONG", "Options", sizeof(nt32::ULONG)}}},
        {"_NtDuplicateToken@24", 6, {{"HANDLE", "ExistingTokenHandle", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"BOOLEAN", "EffectiveOnly", sizeof(nt32::BOOLEAN)}, {"TOKEN_TYPE", "TokenType", sizeof(nt32::TOKEN_TYPE)}, {"PHANDLE", "NewTokenHandle", sizeof(nt32::PHANDLE)}}},
        {"_ZwEnumerateBootEntries@8", 2, {{"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"PULONG", "BufferLength", sizeof(nt32::PULONG)}}},
        {"_NtEnumerateDriverEntries@8", 2, {{"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"PULONG", "BufferLength", sizeof(nt32::PULONG)}}},
        {"_ZwEnumerateKey@24", 6, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Index", sizeof(nt32::ULONG)}, {"KEY_INFORMATION_CLASS", "KeyInformationClass", sizeof(nt32::KEY_INFORMATION_CLASS)}, {"PVOID", "KeyInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PULONG", "ResultLength", sizeof(nt32::PULONG)}}},
        {"_ZwEnumerateSystemEnvironmentValuesEx@12", 3, {{"ULONG", "InformationClass", sizeof(nt32::ULONG)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"PULONG", "BufferLength", sizeof(nt32::PULONG)}}},
        {"_ZwEnumerateTransactionObject@20", 5, {{"HANDLE", "RootObjectHandle", sizeof(nt32::HANDLE)}, {"KTMOBJECT_TYPE", "QueryType", sizeof(nt32::KTMOBJECT_TYPE)}, {"PKTMOBJECT_CURSOR", "ObjectCursor", sizeof(nt32::PKTMOBJECT_CURSOR)}, {"ULONG", "ObjectCursorLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtEnumerateValueKey@24", 6, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Index", sizeof(nt32::ULONG)}, {"KEY_VALUE_INFORMATION_CLASS", "KeyValueInformationClass", sizeof(nt32::KEY_VALUE_INFORMATION_CLASS)}, {"PVOID", "KeyValueInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PULONG", "ResultLength", sizeof(nt32::PULONG)}}},
        {"_ZwExtendSection@8", 2, {{"HANDLE", "SectionHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "NewSectionSize", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtFilterToken@24", 6, {{"HANDLE", "ExistingTokenHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PTOKEN_GROUPS", "SidsToDisable", sizeof(nt32::PTOKEN_GROUPS)}, {"PTOKEN_PRIVILEGES", "PrivilegesToDelete", sizeof(nt32::PTOKEN_PRIVILEGES)}, {"PTOKEN_GROUPS", "RestrictedSids", sizeof(nt32::PTOKEN_GROUPS)}, {"PHANDLE", "NewTokenHandle", sizeof(nt32::PHANDLE)}}},
        {"_NtFindAtom@12", 3, {{"PWSTR", "AtomName", sizeof(nt32::PWSTR)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PRTL_ATOM", "Atom", sizeof(nt32::PRTL_ATOM)}}},
        {"_ZwFlushBuffersFile@8", 2, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}}},
        {"_ZwFlushInstallUILanguage@8", 2, {{"LANGID", "InstallUILanguage", sizeof(nt32::LANGID)}, {"ULONG", "SetComittedFlag", sizeof(nt32::ULONG)}}},
        {"_ZwFlushInstructionCache@12", 3, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt32::PVOID)}, {"SIZE_T", "Length", sizeof(nt32::SIZE_T)}}},
        {"_NtFlushKey@4", 1, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwFlushVirtualMemory@16", 4, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt32::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt32::PSIZE_T)}, {"PIO_STATUS_BLOCK", "IoStatus", sizeof(nt32::PIO_STATUS_BLOCK)}}},
        {"_NtFreeUserPhysicalPages@12", 3, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PULONG_PTR", "NumberOfPages", sizeof(nt32::PULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(nt32::PULONG_PTR)}}},
        {"_NtFreeVirtualMemory@16", 4, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt32::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt32::PSIZE_T)}, {"ULONG", "FreeType", sizeof(nt32::ULONG)}}},
        {"_NtFreezeRegistry@4", 1, {{"ULONG", "TimeOutInSeconds", sizeof(nt32::ULONG)}}},
        {"_ZwFreezeTransactions@8", 2, {{"PLARGE_INTEGER", "FreezeTimeout", sizeof(nt32::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "ThawTimeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtFsControlFile@40", 10, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"ULONG", "IoControlCode", sizeof(nt32::ULONG)}, {"PVOID", "InputBuffer", sizeof(nt32::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt32::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt32::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt32::ULONG)}}},
        {"_NtGetContextThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"PCONTEXT", "ThreadContext", sizeof(nt32::PCONTEXT)}}},
        {"_NtGetDevicePowerState@8", 2, {{"HANDLE", "Device", sizeof(nt32::HANDLE)}, {"DEVICE_POWER_STATE", "STARState", sizeof(nt32::DEVICE_POWER_STATE)}}},
        {"_NtGetMUIRegistryInfo@12", 3, {{"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PULONG", "DataSize", sizeof(nt32::PULONG)}, {"PVOID", "Data", sizeof(nt32::PVOID)}}},
        {"_ZwGetNextProcess@20", 5, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt32::ULONG)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PHANDLE", "NewProcessHandle", sizeof(nt32::PHANDLE)}}},
        {"_ZwGetNextThread@24", 6, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt32::ULONG)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PHANDLE", "NewThreadHandle", sizeof(nt32::PHANDLE)}}},
        {"_NtGetNlsSectionPtr@20", 5, {{"ULONG", "SectionType", sizeof(nt32::ULONG)}, {"ULONG", "SectionData", sizeof(nt32::ULONG)}, {"PVOID", "ContextData", sizeof(nt32::PVOID)}, {"PVOID", "STARSectionPointer", sizeof(nt32::PVOID)}, {"PULONG", "SectionSize", sizeof(nt32::PULONG)}}},
        {"_ZwGetNotificationResourceManager@28", 7, {{"HANDLE", "ResourceManagerHandle", sizeof(nt32::HANDLE)}, {"PTRANSACTION_NOTIFICATION", "TransactionNotification", sizeof(nt32::PTRANSACTION_NOTIFICATION)}, {"ULONG", "NotificationLength", sizeof(nt32::ULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}, {"ULONG", "Asynchronous", sizeof(nt32::ULONG)}, {"ULONG_PTR", "AsynchronousContext", sizeof(nt32::ULONG_PTR)}}},
        {"_NtGetWriteWatch@28", 7, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt32::PVOID)}, {"SIZE_T", "RegionSize", sizeof(nt32::SIZE_T)}, {"PVOID", "STARUserAddressArray", sizeof(nt32::PVOID)}, {"PULONG_PTR", "EntriesInUserAddressArray", sizeof(nt32::PULONG_PTR)}, {"PULONG", "Granularity", sizeof(nt32::PULONG)}}},
        {"_NtImpersonateAnonymousToken@4", 1, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwImpersonateClientOfPort@8", 2, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(nt32::PPORT_MESSAGE)}}},
        {"_ZwImpersonateThread@12", 3, {{"HANDLE", "ServerThreadHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "ClientThreadHandle", sizeof(nt32::HANDLE)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(nt32::PSECURITY_QUALITY_OF_SERVICE)}}},
        {"_NtInitializeNlsFiles@12", 3, {{"PVOID", "STARBaseAddress", sizeof(nt32::PVOID)}, {"PLCID", "DefaultLocaleId", sizeof(nt32::PLCID)}, {"PLARGE_INTEGER", "DefaultCasingTableSize", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwInitializeRegistry@4", 1, {{"USHORT", "BootCondition", sizeof(nt32::USHORT)}}},
        {"_NtInitiatePowerAction@16", 4, {{"POWER_ACTION", "SystemAction", sizeof(nt32::POWER_ACTION)}, {"SYSTEM_POWER_STATE", "MinSystemState", sizeof(nt32::SYSTEM_POWER_STATE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(nt32::BOOLEAN)}}},
        {"_ZwIsProcessInJob@8", 2, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "JobHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwListenPort@8", 2, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(nt32::PPORT_MESSAGE)}}},
        {"_NtLoadDriver@4", 1, {{"PUNICODE_STRING", "DriverServiceName", sizeof(nt32::PUNICODE_STRING)}}},
        {"_NtLoadKey2@12", 3, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}}},
        {"_NtLoadKeyEx@32", 8, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"HANDLE", "TrustClassKey", sizeof(nt32::HANDLE)}, {"PVOID", "Reserved", sizeof(nt32::PVOID)}, {"PVOID", "ObjectContext", sizeof(nt32::PVOID)}, {"PVOID", "CallbackReserverd", sizeof(nt32::PVOID)}, {"PVOID", "IoStatusBlock", sizeof(nt32::PVOID)}}},
        {"_NtLoadKey@8", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtLockFile@40", 10, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt32::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "Length", sizeof(nt32::PLARGE_INTEGER)}, {"ULONG", "Key", sizeof(nt32::ULONG)}, {"BOOLEAN", "FailImmediately", sizeof(nt32::BOOLEAN)}, {"BOOLEAN", "ExclusiveLock", sizeof(nt32::BOOLEAN)}}},
        {"_ZwLockProductActivationKeys@8", 2, {{"ULONG", "STARpPrivateVer", sizeof(nt32::ULONG)}, {"ULONG", "STARpSafeMode", sizeof(nt32::ULONG)}}},
        {"_NtLockRegistryKey@4", 1, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwLockVirtualMemory@16", 4, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt32::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt32::PSIZE_T)}, {"ULONG", "MapType", sizeof(nt32::ULONG)}}},
        {"_ZwMakePermanentObject@4", 1, {{"HANDLE", "Handle", sizeof(nt32::HANDLE)}}},
        {"_NtMakeTemporaryObject@4", 1, {{"HANDLE", "Handle", sizeof(nt32::HANDLE)}}},
        {"_ZwMapCMFModule@24", 6, {{"ULONG", "What", sizeof(nt32::ULONG)}, {"ULONG", "Index", sizeof(nt32::ULONG)}, {"PULONG", "CacheIndexOut", sizeof(nt32::PULONG)}, {"PULONG", "CacheFlagsOut", sizeof(nt32::PULONG)}, {"PULONG", "ViewSizeOut", sizeof(nt32::PULONG)}, {"PVOID", "STARBaseAddress", sizeof(nt32::PVOID)}}},
        {"_NtMapUserPhysicalPages@12", 3, {{"PVOID", "VirtualAddress", sizeof(nt32::PVOID)}, {"ULONG_PTR", "NumberOfPages", sizeof(nt32::ULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(nt32::PULONG_PTR)}}},
        {"_ZwMapUserPhysicalPagesScatter@12", 3, {{"PVOID", "STARVirtualAddresses", sizeof(nt32::PVOID)}, {"ULONG_PTR", "NumberOfPages", sizeof(nt32::ULONG_PTR)}, {"PULONG_PTR", "UserPfnArray", sizeof(nt32::PULONG_PTR)}}},
        {"_ZwMapViewOfSection@40", 10, {{"HANDLE", "SectionHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt32::PVOID)}, {"ULONG_PTR", "ZeroBits", sizeof(nt32::ULONG_PTR)}, {"SIZE_T", "CommitSize", sizeof(nt32::SIZE_T)}, {"PLARGE_INTEGER", "SectionOffset", sizeof(nt32::PLARGE_INTEGER)}, {"PSIZE_T", "ViewSize", sizeof(nt32::PSIZE_T)}, {"SECTION_INHERIT", "InheritDisposition", sizeof(nt32::SECTION_INHERIT)}, {"ULONG", "AllocationType", sizeof(nt32::ULONG)}, {"WIN32_PROTECTION_MASK", "Win32Protect", sizeof(nt32::WIN32_PROTECTION_MASK)}}},
        {"_NtModifyBootEntry@4", 1, {{"PBOOT_ENTRY", "BootEntry", sizeof(nt32::PBOOT_ENTRY)}}},
        {"_ZwModifyDriverEntry@4", 1, {{"PEFI_DRIVER_ENTRY", "DriverEntry", sizeof(nt32::PEFI_DRIVER_ENTRY)}}},
        {"_NtNotifyChangeDirectoryFile@36", 9, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"ULONG", "CompletionFilter", sizeof(nt32::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(nt32::BOOLEAN)}}},
        {"_NtNotifyChangeKey@40", 10, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"ULONG", "CompletionFilter", sizeof(nt32::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(nt32::BOOLEAN)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "BufferSize", sizeof(nt32::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(nt32::BOOLEAN)}}},
        {"_NtNotifyChangeMultipleKeys@48", 12, {{"HANDLE", "MasterKeyHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Count", sizeof(nt32::ULONG)}, {"POBJECT_ATTRIBUTES", "SlaveObjects", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"ULONG", "CompletionFilter", sizeof(nt32::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(nt32::BOOLEAN)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "BufferSize", sizeof(nt32::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(nt32::BOOLEAN)}}},
        {"_NtNotifyChangeSession@32", 8, {{"HANDLE", "Session", sizeof(nt32::HANDLE)}, {"ULONG", "IoStateSequence", sizeof(nt32::ULONG)}, {"PVOID", "Reserved", sizeof(nt32::PVOID)}, {"ULONG", "Action", sizeof(nt32::ULONG)}, {"IO_SESSION_STATE", "IoState", sizeof(nt32::IO_SESSION_STATE)}, {"IO_SESSION_STATE", "IoState2", sizeof(nt32::IO_SESSION_STATE)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "BufferSize", sizeof(nt32::ULONG)}}},
        {"_ZwOpenDirectoryObject@12", 3, {{"PHANDLE", "DirectoryHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenEnlistment@20", 5, {{"PHANDLE", "EnlistmentHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"HANDLE", "ResourceManagerHandle", sizeof(nt32::HANDLE)}, {"LPGUID", "EnlistmentGuid", sizeof(nt32::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenEvent@12", 3, {{"PHANDLE", "EventHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenEventPair@12", 3, {{"PHANDLE", "EventPairHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenFile@24", 6, {{"PHANDLE", "FileHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"ULONG", "ShareAccess", sizeof(nt32::ULONG)}, {"ULONG", "OpenOptions", sizeof(nt32::ULONG)}}},
        {"_ZwOpenIoCompletion@12", 3, {{"PHANDLE", "IoCompletionHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenJobObject@12", 3, {{"PHANDLE", "JobHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenKeyedEvent@12", 3, {{"PHANDLE", "KeyedEventHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenKeyEx@16", 4, {{"PHANDLE", "KeyHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "OpenOptions", sizeof(nt32::ULONG)}}},
        {"_ZwOpenKey@12", 3, {{"PHANDLE", "KeyHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenKeyTransactedEx@20", 5, {{"PHANDLE", "KeyHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "OpenOptions", sizeof(nt32::ULONG)}, {"HANDLE", "TransactionHandle", sizeof(nt32::HANDLE)}}},
        {"_NtOpenKeyTransacted@16", 4, {{"PHANDLE", "KeyHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"HANDLE", "TransactionHandle", sizeof(nt32::HANDLE)}}},
        {"_NtOpenMutant@12", 3, {{"PHANDLE", "MutantHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenObjectAuditAlarm@48", 12, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt32::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt32::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt32::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt32::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt32::PSECURITY_DESCRIPTOR)}, {"HANDLE", "ClientToken", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"ACCESS_MASK", "GrantedAccess", sizeof(nt32::ACCESS_MASK)}, {"PPRIVILEGE_SET", "Privileges", sizeof(nt32::PPRIVILEGE_SET)}, {"BOOLEAN", "ObjectCreation", sizeof(nt32::BOOLEAN)}, {"BOOLEAN", "AccessGranted", sizeof(nt32::BOOLEAN)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt32::PBOOLEAN)}}},
        {"_NtOpenPrivateNamespace@16", 4, {{"PHANDLE", "NamespaceHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PVOID", "BoundaryDescriptor", sizeof(nt32::PVOID)}}},
        {"_ZwOpenProcess@16", 4, {{"PHANDLE", "ProcessHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PCLIENT_ID", "ClientId", sizeof(nt32::PCLIENT_ID)}}},
        {"_ZwOpenProcessTokenEx@16", 4, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt32::ULONG)}, {"PHANDLE", "TokenHandle", sizeof(nt32::PHANDLE)}}},
        {"_ZwOpenProcessToken@12", 3, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"PHANDLE", "TokenHandle", sizeof(nt32::PHANDLE)}}},
        {"_ZwOpenResourceManager@20", 5, {{"PHANDLE", "ResourceManagerHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"HANDLE", "TmHandle", sizeof(nt32::HANDLE)}, {"LPGUID", "ResourceManagerGuid", sizeof(nt32::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenSection@12", 3, {{"PHANDLE", "SectionHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenSemaphore@12", 3, {{"PHANDLE", "SemaphoreHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenSession@12", 3, {{"PHANDLE", "SessionHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtOpenSymbolicLinkObject@12", 3, {{"PHANDLE", "LinkHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenThread@16", 4, {{"PHANDLE", "ThreadHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PCLIENT_ID", "ClientId", sizeof(nt32::PCLIENT_ID)}}},
        {"_NtOpenThreadTokenEx@20", 5, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"BOOLEAN", "OpenAsSelf", sizeof(nt32::BOOLEAN)}, {"ULONG", "HandleAttributes", sizeof(nt32::ULONG)}, {"PHANDLE", "TokenHandle", sizeof(nt32::PHANDLE)}}},
        {"_NtOpenThreadToken@16", 4, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"BOOLEAN", "OpenAsSelf", sizeof(nt32::BOOLEAN)}, {"PHANDLE", "TokenHandle", sizeof(nt32::PHANDLE)}}},
        {"_ZwOpenTimer@12", 3, {{"PHANDLE", "TimerHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwOpenTransactionManager@24", 6, {{"PHANDLE", "TmHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LogFileName", sizeof(nt32::PUNICODE_STRING)}, {"LPGUID", "TmIdentity", sizeof(nt32::LPGUID)}, {"ULONG", "OpenOptions", sizeof(nt32::ULONG)}}},
        {"_ZwOpenTransaction@20", 5, {{"PHANDLE", "TransactionHandle", sizeof(nt32::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"LPGUID", "Uow", sizeof(nt32::LPGUID)}, {"HANDLE", "TmHandle", sizeof(nt32::HANDLE)}}},
        {"_NtPlugPlayControl@12", 3, {{"PLUGPLAY_CONTROL_CLASS", "PnPControlClass", sizeof(nt32::PLUGPLAY_CONTROL_CLASS)}, {"PVOID", "PnPControlData", sizeof(nt32::PVOID)}, {"ULONG", "PnPControlDataLength", sizeof(nt32::ULONG)}}},
        {"_ZwPowerInformation@20", 5, {{"POWER_INFORMATION_LEVEL", "InformationLevel", sizeof(nt32::POWER_INFORMATION_LEVEL)}, {"PVOID", "InputBuffer", sizeof(nt32::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt32::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt32::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt32::ULONG)}}},
        {"_NtPrepareComplete@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwPrepareEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwPrePrepareComplete@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtPrePrepareEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwPrivilegeCheck@12", 3, {{"HANDLE", "ClientToken", sizeof(nt32::HANDLE)}, {"PPRIVILEGE_SET", "RequiredPrivileges", sizeof(nt32::PPRIVILEGE_SET)}, {"PBOOLEAN", "Result", sizeof(nt32::PBOOLEAN)}}},
        {"_NtPrivilegedServiceAuditAlarm@20", 5, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt32::PUNICODE_STRING)}, {"PUNICODE_STRING", "ServiceName", sizeof(nt32::PUNICODE_STRING)}, {"HANDLE", "ClientToken", sizeof(nt32::HANDLE)}, {"PPRIVILEGE_SET", "Privileges", sizeof(nt32::PPRIVILEGE_SET)}, {"BOOLEAN", "AccessGranted", sizeof(nt32::BOOLEAN)}}},
        {"_ZwPrivilegeObjectAuditAlarm@24", 6, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt32::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt32::PVOID)}, {"HANDLE", "ClientToken", sizeof(nt32::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt32::ACCESS_MASK)}, {"PPRIVILEGE_SET", "Privileges", sizeof(nt32::PPRIVILEGE_SET)}, {"BOOLEAN", "AccessGranted", sizeof(nt32::BOOLEAN)}}},
        {"_NtPropagationComplete@16", 4, {{"HANDLE", "ResourceManagerHandle", sizeof(nt32::HANDLE)}, {"ULONG", "RequestCookie", sizeof(nt32::ULONG)}, {"ULONG", "BufferLength", sizeof(nt32::ULONG)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}}},
        {"_ZwPropagationFailed@12", 3, {{"HANDLE", "ResourceManagerHandle", sizeof(nt32::HANDLE)}, {"ULONG", "RequestCookie", sizeof(nt32::ULONG)}, {"NTSTATUS", "PropStatus", sizeof(nt32::NTSTATUS)}}},
        {"_ZwProtectVirtualMemory@20", 5, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt32::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt32::PSIZE_T)}, {"WIN32_PROTECTION_MASK", "NewProtectWin32", sizeof(nt32::WIN32_PROTECTION_MASK)}, {"PULONG", "OldProtect", sizeof(nt32::PULONG)}}},
        {"_ZwPulseEvent@8", 2, {{"HANDLE", "EventHandle", sizeof(nt32::HANDLE)}, {"PLONG", "PreviousState", sizeof(nt32::PLONG)}}},
        {"_ZwQueryAttributesFile@8", 2, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PFILE_BASIC_INFORMATION", "FileInformation", sizeof(nt32::PFILE_BASIC_INFORMATION)}}},
        {"_ZwQueryBootEntryOrder@8", 2, {{"PULONG", "Ids", sizeof(nt32::PULONG)}, {"PULONG", "Count", sizeof(nt32::PULONG)}}},
        {"_ZwQueryBootOptions@8", 2, {{"PBOOT_OPTIONS", "BootOptions", sizeof(nt32::PBOOT_OPTIONS)}, {"PULONG", "BootOptionsLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryDebugFilterState@8", 2, {{"ULONG", "ComponentId", sizeof(nt32::ULONG)}, {"ULONG", "Level", sizeof(nt32::ULONG)}}},
        {"_NtQueryDefaultLocale@8", 2, {{"BOOLEAN", "UserProfile", sizeof(nt32::BOOLEAN)}, {"PLCID", "DefaultLocaleId", sizeof(nt32::PLCID)}}},
        {"_ZwQueryDefaultUILanguage@4", 1, {{"LANGID", "STARDefaultUILanguageId", sizeof(nt32::LANGID)}}},
        {"_ZwQueryDirectoryFile@44", 11, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(nt32::FILE_INFORMATION_CLASS)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt32::BOOLEAN)}, {"PUNICODE_STRING", "FileName", sizeof(nt32::PUNICODE_STRING)}, {"BOOLEAN", "RestartScan", sizeof(nt32::BOOLEAN)}}},
        {"_ZwQueryDirectoryObject@28", 7, {{"HANDLE", "DirectoryHandle", sizeof(nt32::HANDLE)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt32::BOOLEAN)}, {"BOOLEAN", "RestartScan", sizeof(nt32::BOOLEAN)}, {"PULONG", "Context", sizeof(nt32::PULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryDriverEntryOrder@8", 2, {{"PULONG", "Ids", sizeof(nt32::PULONG)}, {"PULONG", "Count", sizeof(nt32::PULONG)}}},
        {"_ZwQueryEaFile@36", 9, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt32::BOOLEAN)}, {"PVOID", "EaList", sizeof(nt32::PVOID)}, {"ULONG", "EaListLength", sizeof(nt32::ULONG)}, {"PULONG", "EaIndex", sizeof(nt32::PULONG)}, {"BOOLEAN", "RestartScan", sizeof(nt32::BOOLEAN)}}},
        {"_NtQueryEvent@20", 5, {{"HANDLE", "EventHandle", sizeof(nt32::HANDLE)}, {"EVENT_INFORMATION_CLASS", "EventInformationClass", sizeof(nt32::EVENT_INFORMATION_CLASS)}, {"PVOID", "EventInformation", sizeof(nt32::PVOID)}, {"ULONG", "EventInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQueryFullAttributesFile@8", 2, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PFILE_NETWORK_OPEN_INFORMATION", "FileInformation", sizeof(nt32::PFILE_NETWORK_OPEN_INFORMATION)}}},
        {"_NtQueryInformationAtom@20", 5, {{"RTL_ATOM", "Atom", sizeof(nt32::RTL_ATOM)}, {"ATOM_INFORMATION_CLASS", "InformationClass", sizeof(nt32::ATOM_INFORMATION_CLASS)}, {"PVOID", "AtomInformation", sizeof(nt32::PVOID)}, {"ULONG", "AtomInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQueryInformationEnlistment@20", 5, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"ENLISTMENT_INFORMATION_CLASS", "EnlistmentInformationClass", sizeof(nt32::ENLISTMENT_INFORMATION_CLASS)}, {"PVOID", "EnlistmentInformation", sizeof(nt32::PVOID)}, {"ULONG", "EnlistmentInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQueryInformationFile@20", 5, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(nt32::FILE_INFORMATION_CLASS)}}},
        {"_ZwQueryInformationJobObject@20", 5, {{"HANDLE", "JobHandle", sizeof(nt32::HANDLE)}, {"JOBOBJECTINFOCLASS", "JobObjectInformationClass", sizeof(nt32::JOBOBJECTINFOCLASS)}, {"PVOID", "JobObjectInformation", sizeof(nt32::PVOID)}, {"ULONG", "JobObjectInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQueryInformationPort@20", 5, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(nt32::PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQueryInformationProcess@20", 5, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PROCESSINFOCLASS", "ProcessInformationClass", sizeof(nt32::PROCESSINFOCLASS)}, {"PVOID", "ProcessInformation", sizeof(nt32::PVOID)}, {"ULONG", "ProcessInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQueryInformationResourceManager@20", 5, {{"HANDLE", "ResourceManagerHandle", sizeof(nt32::HANDLE)}, {"RESOURCEMANAGER_INFORMATION_CLASS", "ResourceManagerInformationClass", sizeof(nt32::RESOURCEMANAGER_INFORMATION_CLASS)}, {"PVOID", "ResourceManagerInformation", sizeof(nt32::PVOID)}, {"ULONG", "ResourceManagerInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryInformationThread@20", 5, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"THREADINFOCLASS", "ThreadInformationClass", sizeof(nt32::THREADINFOCLASS)}, {"PVOID", "ThreadInformation", sizeof(nt32::PVOID)}, {"ULONG", "ThreadInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQueryInformationToken@20", 5, {{"HANDLE", "TokenHandle", sizeof(nt32::HANDLE)}, {"TOKEN_INFORMATION_CLASS", "TokenInformationClass", sizeof(nt32::TOKEN_INFORMATION_CLASS)}, {"PVOID", "TokenInformation", sizeof(nt32::PVOID)}, {"ULONG", "TokenInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQueryInformationTransaction@20", 5, {{"HANDLE", "TransactionHandle", sizeof(nt32::HANDLE)}, {"TRANSACTION_INFORMATION_CLASS", "TransactionInformationClass", sizeof(nt32::TRANSACTION_INFORMATION_CLASS)}, {"PVOID", "TransactionInformation", sizeof(nt32::PVOID)}, {"ULONG", "TransactionInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryInformationTransactionManager@20", 5, {{"HANDLE", "TransactionManagerHandle", sizeof(nt32::HANDLE)}, {"TRANSACTIONMANAGER_INFORMATION_CLASS", "TransactionManagerInformationClass", sizeof(nt32::TRANSACTIONMANAGER_INFORMATION_CLASS)}, {"PVOID", "TransactionManagerInformation", sizeof(nt32::PVOID)}, {"ULONG", "TransactionManagerInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQueryInformationWorkerFactory@20", 5, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt32::HANDLE)}, {"WORKERFACTORYINFOCLASS", "WorkerFactoryInformationClass", sizeof(nt32::WORKERFACTORYINFOCLASS)}, {"PVOID", "WorkerFactoryInformation", sizeof(nt32::PVOID)}, {"ULONG", "WorkerFactoryInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryInstallUILanguage@4", 1, {{"LANGID", "STARInstallUILanguageId", sizeof(nt32::LANGID)}}},
        {"_NtQueryIntervalProfile@8", 2, {{"KPROFILE_SOURCE", "ProfileSource", sizeof(nt32::KPROFILE_SOURCE)}, {"PULONG", "Interval", sizeof(nt32::PULONG)}}},
        {"_NtQueryIoCompletion@20", 5, {{"HANDLE", "IoCompletionHandle", sizeof(nt32::HANDLE)}, {"IO_COMPLETION_INFORMATION_CLASS", "IoCompletionInformationClass", sizeof(nt32::IO_COMPLETION_INFORMATION_CLASS)}, {"PVOID", "IoCompletionInformation", sizeof(nt32::PVOID)}, {"ULONG", "IoCompletionInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQueryKey@20", 5, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"KEY_INFORMATION_CLASS", "KeyInformationClass", sizeof(nt32::KEY_INFORMATION_CLASS)}, {"PVOID", "KeyInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PULONG", "ResultLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryLicenseValue@20", 5, {{"PUNICODE_STRING", "Name", sizeof(nt32::PUNICODE_STRING)}, {"PULONG", "Type", sizeof(nt32::PULONG)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PULONG", "ReturnedLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryMultipleValueKey@24", 6, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"PKEY_VALUE_ENTRY", "ValueEntries", sizeof(nt32::PKEY_VALUE_ENTRY)}, {"ULONG", "EntryCount", sizeof(nt32::ULONG)}, {"PVOID", "ValueBuffer", sizeof(nt32::PVOID)}, {"PULONG", "BufferLength", sizeof(nt32::PULONG)}, {"PULONG", "RequiredBufferLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryMutant@20", 5, {{"HANDLE", "MutantHandle", sizeof(nt32::HANDLE)}, {"MUTANT_INFORMATION_CLASS", "MutantInformationClass", sizeof(nt32::MUTANT_INFORMATION_CLASS)}, {"PVOID", "MutantInformation", sizeof(nt32::PVOID)}, {"ULONG", "MutantInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryObject@20", 5, {{"HANDLE", "Handle", sizeof(nt32::HANDLE)}, {"OBJECT_INFORMATION_CLASS", "ObjectInformationClass", sizeof(nt32::OBJECT_INFORMATION_CLASS)}, {"PVOID", "ObjectInformation", sizeof(nt32::PVOID)}, {"ULONG", "ObjectInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryOpenSubKeysEx@16", 4, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "BufferLength", sizeof(nt32::ULONG)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"PULONG", "RequiredSize", sizeof(nt32::PULONG)}}},
        {"_NtQueryOpenSubKeys@8", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"PULONG", "HandleCount", sizeof(nt32::PULONG)}}},
        {"_NtQueryPerformanceCounter@8", 2, {{"PLARGE_INTEGER", "PerformanceCounter", sizeof(nt32::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "PerformanceFrequency", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwQueryQuotaInformationFile@36", 9, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt32::BOOLEAN)}, {"PVOID", "SidList", sizeof(nt32::PVOID)}, {"ULONG", "SidListLength", sizeof(nt32::ULONG)}, {"PULONG", "StartSid", sizeof(nt32::PULONG)}, {"BOOLEAN", "RestartScan", sizeof(nt32::BOOLEAN)}}},
        {"_ZwQuerySection@20", 5, {{"HANDLE", "SectionHandle", sizeof(nt32::HANDLE)}, {"SECTION_INFORMATION_CLASS", "SectionInformationClass", sizeof(nt32::SECTION_INFORMATION_CLASS)}, {"PVOID", "SectionInformation", sizeof(nt32::PVOID)}, {"SIZE_T", "SectionInformationLength", sizeof(nt32::SIZE_T)}, {"PSIZE_T", "ReturnLength", sizeof(nt32::PSIZE_T)}}},
        {"_ZwQuerySecurityAttributesToken@24", 6, {{"HANDLE", "TokenHandle", sizeof(nt32::HANDLE)}, {"PUNICODE_STRING", "Attributes", sizeof(nt32::PUNICODE_STRING)}, {"ULONG", "NumberOfAttributes", sizeof(nt32::ULONG)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtQuerySecurityObject@20", 5, {{"HANDLE", "Handle", sizeof(nt32::HANDLE)}, {"SECURITY_INFORMATION", "SecurityInformation", sizeof(nt32::SECURITY_INFORMATION)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt32::PSECURITY_DESCRIPTOR)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PULONG", "LengthNeeded", sizeof(nt32::PULONG)}}},
        {"_ZwQuerySemaphore@20", 5, {{"HANDLE", "SemaphoreHandle", sizeof(nt32::HANDLE)}, {"SEMAPHORE_INFORMATION_CLASS", "SemaphoreInformationClass", sizeof(nt32::SEMAPHORE_INFORMATION_CLASS)}, {"PVOID", "SemaphoreInformation", sizeof(nt32::PVOID)}, {"ULONG", "SemaphoreInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwQuerySymbolicLinkObject@12", 3, {{"HANDLE", "LinkHandle", sizeof(nt32::HANDLE)}, {"PUNICODE_STRING", "LinkTarget", sizeof(nt32::PUNICODE_STRING)}, {"PULONG", "ReturnedLength", sizeof(nt32::PULONG)}}},
        {"_ZwQuerySystemEnvironmentValueEx@20", 5, {{"PUNICODE_STRING", "VariableName", sizeof(nt32::PUNICODE_STRING)}, {"LPGUID", "VendorGuid", sizeof(nt32::LPGUID)}, {"PVOID", "Value", sizeof(nt32::PVOID)}, {"PULONG", "ValueLength", sizeof(nt32::PULONG)}, {"PULONG", "Attributes", sizeof(nt32::PULONG)}}},
        {"_ZwQuerySystemEnvironmentValue@16", 4, {{"PUNICODE_STRING", "VariableName", sizeof(nt32::PUNICODE_STRING)}, {"PWSTR", "VariableValue", sizeof(nt32::PWSTR)}, {"USHORT", "ValueLength", sizeof(nt32::USHORT)}, {"PUSHORT", "ReturnLength", sizeof(nt32::PUSHORT)}}},
        {"_ZwQuerySystemInformationEx@24", 6, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(nt32::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "QueryInformation", sizeof(nt32::PVOID)}, {"ULONG", "QueryInformationLength", sizeof(nt32::ULONG)}, {"PVOID", "SystemInformation", sizeof(nt32::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtQuerySystemInformation@16", 4, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(nt32::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "SystemInformation", sizeof(nt32::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtQuerySystemTime@4", 1, {{"PLARGE_INTEGER", "SystemTime", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwQueryTimer@20", 5, {{"HANDLE", "TimerHandle", sizeof(nt32::HANDLE)}, {"TIMER_INFORMATION_CLASS", "TimerInformationClass", sizeof(nt32::TIMER_INFORMATION_CLASS)}, {"PVOID", "TimerInformation", sizeof(nt32::PVOID)}, {"ULONG", "TimerInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryTimerResolution@12", 3, {{"PULONG", "MaximumTime", sizeof(nt32::PULONG)}, {"PULONG", "MinimumTime", sizeof(nt32::PULONG)}, {"PULONG", "CurrentTime", sizeof(nt32::PULONG)}}},
        {"_ZwQueryValueKey@24", 6, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(nt32::PUNICODE_STRING)}, {"KEY_VALUE_INFORMATION_CLASS", "KeyValueInformationClass", sizeof(nt32::KEY_VALUE_INFORMATION_CLASS)}, {"PVOID", "KeyValueInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PULONG", "ResultLength", sizeof(nt32::PULONG)}}},
        {"_NtQueryVirtualMemory@24", 6, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt32::PVOID)}, {"MEMORY_INFORMATION_CLASS", "MemoryInformationClass", sizeof(nt32::MEMORY_INFORMATION_CLASS)}, {"PVOID", "MemoryInformation", sizeof(nt32::PVOID)}, {"SIZE_T", "MemoryInformationLength", sizeof(nt32::SIZE_T)}, {"PSIZE_T", "ReturnLength", sizeof(nt32::PSIZE_T)}}},
        {"_NtQueryVolumeInformationFile@20", 5, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "FsInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"FS_INFORMATION_CLASS", "FsInformationClass", sizeof(nt32::FS_INFORMATION_CLASS)}}},
        {"_NtQueueApcThreadEx@24", 6, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "UserApcReserveHandle", sizeof(nt32::HANDLE)}, {"PPS_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PPS_APC_ROUTINE)}, {"PVOID", "ApcArgument1", sizeof(nt32::PVOID)}, {"PVOID", "ApcArgument2", sizeof(nt32::PVOID)}, {"PVOID", "ApcArgument3", sizeof(nt32::PVOID)}}},
        {"_NtQueueApcThread@20", 5, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"PPS_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PPS_APC_ROUTINE)}, {"PVOID", "ApcArgument1", sizeof(nt32::PVOID)}, {"PVOID", "ApcArgument2", sizeof(nt32::PVOID)}, {"PVOID", "ApcArgument3", sizeof(nt32::PVOID)}}},
        {"_ZwRaiseException@12", 3, {{"PEXCEPTION_RECORD", "ExceptionRecord", sizeof(nt32::PEXCEPTION_RECORD)}, {"PCONTEXT", "ContextRecord", sizeof(nt32::PCONTEXT)}, {"BOOLEAN", "FirstChance", sizeof(nt32::BOOLEAN)}}},
        {"_ZwRaiseHardError@24", 6, {{"NTSTATUS", "ErrorStatus", sizeof(nt32::NTSTATUS)}, {"ULONG", "NumberOfParameters", sizeof(nt32::ULONG)}, {"ULONG", "UnicodeStringParameterMask", sizeof(nt32::ULONG)}, {"PULONG_PTR", "Parameters", sizeof(nt32::PULONG_PTR)}, {"ULONG", "ValidResponseOptions", sizeof(nt32::ULONG)}, {"PULONG", "Response", sizeof(nt32::PULONG)}}},
        {"_NtReadFile@36", 9, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt32::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt32::PULONG)}}},
        {"_NtReadFileScatter@36", 9, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PFILE_SEGMENT_ELEMENT", "SegmentArray", sizeof(nt32::PFILE_SEGMENT_ELEMENT)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt32::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt32::PULONG)}}},
        {"_ZwReadOnlyEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwReadRequestData@24", 6, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(nt32::PPORT_MESSAGE)}, {"ULONG", "DataEntryIndex", sizeof(nt32::ULONG)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt32::SIZE_T)}, {"PSIZE_T", "NumberOfBytesRead", sizeof(nt32::PSIZE_T)}}},
        {"_NtReadVirtualMemory@20", 5, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt32::PVOID)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt32::SIZE_T)}, {"PSIZE_T", "NumberOfBytesRead", sizeof(nt32::PSIZE_T)}}},
        {"_NtRecoverEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PVOID", "EnlistmentKey", sizeof(nt32::PVOID)}}},
        {"_NtRecoverResourceManager@4", 1, {{"HANDLE", "ResourceManagerHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwRecoverTransactionManager@4", 1, {{"HANDLE", "TransactionManagerHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwRegisterProtocolAddressInformation@20", 5, {{"HANDLE", "ResourceManager", sizeof(nt32::HANDLE)}, {"PCRM_PROTOCOL_ID", "ProtocolId", sizeof(nt32::PCRM_PROTOCOL_ID)}, {"ULONG", "ProtocolInformationSize", sizeof(nt32::ULONG)}, {"PVOID", "ProtocolInformation", sizeof(nt32::PVOID)}, {"ULONG", "CreateOptions", sizeof(nt32::ULONG)}}},
        {"_ZwRegisterThreadTerminatePort@4", 1, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}}},
        {"_NtReleaseKeyedEvent@16", 4, {{"HANDLE", "KeyedEventHandle", sizeof(nt32::HANDLE)}, {"PVOID", "KeyValue", sizeof(nt32::PVOID)}, {"BOOLEAN", "Alertable", sizeof(nt32::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwReleaseMutant@8", 2, {{"HANDLE", "MutantHandle", sizeof(nt32::HANDLE)}, {"PLONG", "PreviousCount", sizeof(nt32::PLONG)}}},
        {"_NtReleaseSemaphore@12", 3, {{"HANDLE", "SemaphoreHandle", sizeof(nt32::HANDLE)}, {"LONG", "ReleaseCount", sizeof(nt32::LONG)}, {"PLONG", "PreviousCount", sizeof(nt32::PLONG)}}},
        {"_ZwReleaseWorkerFactoryWorker@4", 1, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwRemoveIoCompletionEx@24", 6, {{"HANDLE", "IoCompletionHandle", sizeof(nt32::HANDLE)}, {"PFILE_IO_COMPLETION_INFORMATION", "IoCompletionInformation", sizeof(nt32::PFILE_IO_COMPLETION_INFORMATION)}, {"ULONG", "Count", sizeof(nt32::ULONG)}, {"PULONG", "NumEntriesRemoved", sizeof(nt32::PULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}, {"BOOLEAN", "Alertable", sizeof(nt32::BOOLEAN)}}},
        {"_ZwRemoveIoCompletion@20", 5, {{"HANDLE", "IoCompletionHandle", sizeof(nt32::HANDLE)}, {"PVOID", "STARKeyContext", sizeof(nt32::PVOID)}, {"PVOID", "STARApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwRemoveProcessDebug@8", 2, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "DebugObjectHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwRenameKey@8", 2, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"PUNICODE_STRING", "NewName", sizeof(nt32::PUNICODE_STRING)}}},
        {"_NtRenameTransactionManager@8", 2, {{"PUNICODE_STRING", "LogFileName", sizeof(nt32::PUNICODE_STRING)}, {"LPGUID", "ExistingTransactionManagerGuid", sizeof(nt32::LPGUID)}}},
        {"_ZwReplaceKey@12", 3, {{"POBJECT_ATTRIBUTES", "NewFile", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"HANDLE", "TargetHandle", sizeof(nt32::HANDLE)}, {"POBJECT_ATTRIBUTES", "OldFile", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_NtReplacePartitionUnit@12", 3, {{"PUNICODE_STRING", "TargetInstancePath", sizeof(nt32::PUNICODE_STRING)}, {"PUNICODE_STRING", "SpareInstancePath", sizeof(nt32::PUNICODE_STRING)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}}},
        {"_ZwReplyPort@8", 2, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt32::PPORT_MESSAGE)}}},
        {"_NtReplyWaitReceivePortEx@20", 5, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PVOID", "STARPortContext", sizeof(nt32::PVOID)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt32::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(nt32::PPORT_MESSAGE)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtReplyWaitReceivePort@16", 4, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PVOID", "STARPortContext", sizeof(nt32::PVOID)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt32::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(nt32::PPORT_MESSAGE)}}},
        {"_NtReplyWaitReplyPort@8", 2, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt32::PPORT_MESSAGE)}}},
        {"_ZwRequestPort@8", 2, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "RequestMessage", sizeof(nt32::PPORT_MESSAGE)}}},
        {"_NtRequestWaitReplyPort@12", 3, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "RequestMessage", sizeof(nt32::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt32::PPORT_MESSAGE)}}},
        {"_NtResetEvent@8", 2, {{"HANDLE", "EventHandle", sizeof(nt32::HANDLE)}, {"PLONG", "PreviousState", sizeof(nt32::PLONG)}}},
        {"_ZwResetWriteWatch@12", 3, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt32::PVOID)}, {"SIZE_T", "RegionSize", sizeof(nt32::SIZE_T)}}},
        {"_NtRestoreKey@12", 3, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}}},
        {"_ZwResumeProcess@4", 1, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwResumeThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(nt32::PULONG)}}},
        {"_ZwRollbackComplete@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtRollbackEnlistment@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtRollbackTransaction@8", 2, {{"HANDLE", "TransactionHandle", sizeof(nt32::HANDLE)}, {"BOOLEAN", "Wait", sizeof(nt32::BOOLEAN)}}},
        {"_NtRollforwardTransactionManager@8", 2, {{"HANDLE", "TransactionManagerHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtSaveKeyEx@12", 3, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Format", sizeof(nt32::ULONG)}}},
        {"_NtSaveKey@8", 2, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}}},
        {"_NtSaveMergedKeys@12", 3, {{"HANDLE", "HighPrecedenceKeyHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "LowPrecedenceKeyHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}}},
        {"_NtSecureConnectPort@36", 9, {{"PHANDLE", "PortHandle", sizeof(nt32::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(nt32::PUNICODE_STRING)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(nt32::PSECURITY_QUALITY_OF_SERVICE)}, {"PPORT_VIEW", "ClientView", sizeof(nt32::PPORT_VIEW)}, {"PSID", "RequiredServerSid", sizeof(nt32::PSID)}, {"PREMOTE_PORT_VIEW", "ServerView", sizeof(nt32::PREMOTE_PORT_VIEW)}, {"PULONG", "MaxMessageLength", sizeof(nt32::PULONG)}, {"PVOID", "ConnectionInformation", sizeof(nt32::PVOID)}, {"PULONG", "ConnectionInformationLength", sizeof(nt32::PULONG)}}},
        {"_ZwSetBootEntryOrder@8", 2, {{"PULONG", "Ids", sizeof(nt32::PULONG)}, {"ULONG", "Count", sizeof(nt32::ULONG)}}},
        {"_ZwSetBootOptions@8", 2, {{"PBOOT_OPTIONS", "BootOptions", sizeof(nt32::PBOOT_OPTIONS)}, {"ULONG", "FieldsToChange", sizeof(nt32::ULONG)}}},
        {"_ZwSetContextThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"PCONTEXT", "ThreadContext", sizeof(nt32::PCONTEXT)}}},
        {"_NtSetDebugFilterState@12", 3, {{"ULONG", "ComponentId", sizeof(nt32::ULONG)}, {"ULONG", "Level", sizeof(nt32::ULONG)}, {"BOOLEAN", "State", sizeof(nt32::BOOLEAN)}}},
        {"_NtSetDefaultHardErrorPort@4", 1, {{"HANDLE", "DefaultHardErrorPort", sizeof(nt32::HANDLE)}}},
        {"_NtSetDefaultLocale@8", 2, {{"BOOLEAN", "UserProfile", sizeof(nt32::BOOLEAN)}, {"LCID", "DefaultLocaleId", sizeof(nt32::LCID)}}},
        {"_ZwSetDefaultUILanguage@4", 1, {{"LANGID", "DefaultUILanguageId", sizeof(nt32::LANGID)}}},
        {"_NtSetDriverEntryOrder@8", 2, {{"PULONG", "Ids", sizeof(nt32::PULONG)}, {"ULONG", "Count", sizeof(nt32::ULONG)}}},
        {"_ZwSetEaFile@16", 4, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}}},
        {"_NtSetEventBoostPriority@4", 1, {{"HANDLE", "EventHandle", sizeof(nt32::HANDLE)}}},
        {"_NtSetEvent@8", 2, {{"HANDLE", "EventHandle", sizeof(nt32::HANDLE)}, {"PLONG", "PreviousState", sizeof(nt32::PLONG)}}},
        {"_NtSetHighEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(nt32::HANDLE)}}},
        {"_NtSetHighWaitLowEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwSetInformationDebugObject@20", 5, {{"HANDLE", "DebugObjectHandle", sizeof(nt32::HANDLE)}, {"DEBUGOBJECTINFOCLASS", "DebugObjectInformationClass", sizeof(nt32::DEBUGOBJECTINFOCLASS)}, {"PVOID", "DebugInformation", sizeof(nt32::PVOID)}, {"ULONG", "DebugInformationLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtSetInformationEnlistment@16", 4, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"ENLISTMENT_INFORMATION_CLASS", "EnlistmentInformationClass", sizeof(nt32::ENLISTMENT_INFORMATION_CLASS)}, {"PVOID", "EnlistmentInformation", sizeof(nt32::PVOID)}, {"ULONG", "EnlistmentInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetInformationFile@20", 5, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(nt32::FILE_INFORMATION_CLASS)}}},
        {"_ZwSetInformationJobObject@16", 4, {{"HANDLE", "JobHandle", sizeof(nt32::HANDLE)}, {"JOBOBJECTINFOCLASS", "JobObjectInformationClass", sizeof(nt32::JOBOBJECTINFOCLASS)}, {"PVOID", "JobObjectInformation", sizeof(nt32::PVOID)}, {"ULONG", "JobObjectInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetInformationKey@16", 4, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"KEY_SET_INFORMATION_CLASS", "KeySetInformationClass", sizeof(nt32::KEY_SET_INFORMATION_CLASS)}, {"PVOID", "KeySetInformation", sizeof(nt32::PVOID)}, {"ULONG", "KeySetInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetInformationObject@16", 4, {{"HANDLE", "Handle", sizeof(nt32::HANDLE)}, {"OBJECT_INFORMATION_CLASS", "ObjectInformationClass", sizeof(nt32::OBJECT_INFORMATION_CLASS)}, {"PVOID", "ObjectInformation", sizeof(nt32::PVOID)}, {"ULONG", "ObjectInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetInformationProcess@16", 4, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PROCESSINFOCLASS", "ProcessInformationClass", sizeof(nt32::PROCESSINFOCLASS)}, {"PVOID", "ProcessInformation", sizeof(nt32::PVOID)}, {"ULONG", "ProcessInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetInformationResourceManager@16", 4, {{"HANDLE", "ResourceManagerHandle", sizeof(nt32::HANDLE)}, {"RESOURCEMANAGER_INFORMATION_CLASS", "ResourceManagerInformationClass", sizeof(nt32::RESOURCEMANAGER_INFORMATION_CLASS)}, {"PVOID", "ResourceManagerInformation", sizeof(nt32::PVOID)}, {"ULONG", "ResourceManagerInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetInformationThread@16", 4, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"THREADINFOCLASS", "ThreadInformationClass", sizeof(nt32::THREADINFOCLASS)}, {"PVOID", "ThreadInformation", sizeof(nt32::PVOID)}, {"ULONG", "ThreadInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetInformationToken@16", 4, {{"HANDLE", "TokenHandle", sizeof(nt32::HANDLE)}, {"TOKEN_INFORMATION_CLASS", "TokenInformationClass", sizeof(nt32::TOKEN_INFORMATION_CLASS)}, {"PVOID", "TokenInformation", sizeof(nt32::PVOID)}, {"ULONG", "TokenInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetInformationTransaction@16", 4, {{"HANDLE", "TransactionHandle", sizeof(nt32::HANDLE)}, {"TRANSACTION_INFORMATION_CLASS", "TransactionInformationClass", sizeof(nt32::TRANSACTION_INFORMATION_CLASS)}, {"PVOID", "TransactionInformation", sizeof(nt32::PVOID)}, {"ULONG", "TransactionInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetInformationTransactionManager@16", 4, {{"HANDLE", "TmHandle", sizeof(nt32::HANDLE)}, {"TRANSACTIONMANAGER_INFORMATION_CLASS", "TransactionManagerInformationClass", sizeof(nt32::TRANSACTIONMANAGER_INFORMATION_CLASS)}, {"PVOID", "TransactionManagerInformation", sizeof(nt32::PVOID)}, {"ULONG", "TransactionManagerInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetInformationWorkerFactory@16", 4, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt32::HANDLE)}, {"WORKERFACTORYINFOCLASS", "WorkerFactoryInformationClass", sizeof(nt32::WORKERFACTORYINFOCLASS)}, {"PVOID", "WorkerFactoryInformation", sizeof(nt32::PVOID)}, {"ULONG", "WorkerFactoryInformationLength", sizeof(nt32::ULONG)}}},
        {"_NtSetIntervalProfile@8", 2, {{"ULONG", "Interval", sizeof(nt32::ULONG)}, {"KPROFILE_SOURCE", "Source", sizeof(nt32::KPROFILE_SOURCE)}}},
        {"_NtSetIoCompletionEx@24", 6, {{"HANDLE", "IoCompletionHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "IoCompletionReserveHandle", sizeof(nt32::HANDLE)}, {"PVOID", "KeyContext", sizeof(nt32::PVOID)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"NTSTATUS", "IoStatus", sizeof(nt32::NTSTATUS)}, {"ULONG_PTR", "IoStatusInformation", sizeof(nt32::ULONG_PTR)}}},
        {"_NtSetIoCompletion@20", 5, {{"HANDLE", "IoCompletionHandle", sizeof(nt32::HANDLE)}, {"PVOID", "KeyContext", sizeof(nt32::PVOID)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"NTSTATUS", "IoStatus", sizeof(nt32::NTSTATUS)}, {"ULONG_PTR", "IoStatusInformation", sizeof(nt32::ULONG_PTR)}}},
        {"_ZwSetLdtEntries@24", 6, {{"ULONG", "Selector0", sizeof(nt32::ULONG)}, {"ULONG", "Entry0Low", sizeof(nt32::ULONG)}, {"ULONG", "Entry0Hi", sizeof(nt32::ULONG)}, {"ULONG", "Selector1", sizeof(nt32::ULONG)}, {"ULONG", "Entry1Low", sizeof(nt32::ULONG)}, {"ULONG", "Entry1Hi", sizeof(nt32::ULONG)}}},
        {"_ZwSetLowEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwSetLowWaitHighEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwSetQuotaInformationFile@16", 4, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}}},
        {"_NtSetSecurityObject@12", 3, {{"HANDLE", "Handle", sizeof(nt32::HANDLE)}, {"SECURITY_INFORMATION", "SecurityInformation", sizeof(nt32::SECURITY_INFORMATION)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt32::PSECURITY_DESCRIPTOR)}}},
        {"_ZwSetSystemEnvironmentValueEx@20", 5, {{"PUNICODE_STRING", "VariableName", sizeof(nt32::PUNICODE_STRING)}, {"LPGUID", "VendorGuid", sizeof(nt32::LPGUID)}, {"PVOID", "Value", sizeof(nt32::PVOID)}, {"ULONG", "ValueLength", sizeof(nt32::ULONG)}, {"ULONG", "Attributes", sizeof(nt32::ULONG)}}},
        {"_ZwSetSystemEnvironmentValue@8", 2, {{"PUNICODE_STRING", "VariableName", sizeof(nt32::PUNICODE_STRING)}, {"PUNICODE_STRING", "VariableValue", sizeof(nt32::PUNICODE_STRING)}}},
        {"_ZwSetSystemInformation@12", 3, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(nt32::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "SystemInformation", sizeof(nt32::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetSystemPowerState@12", 3, {{"POWER_ACTION", "SystemAction", sizeof(nt32::POWER_ACTION)}, {"SYSTEM_POWER_STATE", "MinSystemState", sizeof(nt32::SYSTEM_POWER_STATE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}}},
        {"_ZwSetSystemTime@8", 2, {{"PLARGE_INTEGER", "SystemTime", sizeof(nt32::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "PreviousTime", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwSetThreadExecutionState@8", 2, {{"EXECUTION_STATE", "esFlags", sizeof(nt32::EXECUTION_STATE)}, {"EXECUTION_STATE", "STARPreviousFlags", sizeof(nt32::EXECUTION_STATE)}}},
        {"_ZwSetTimerEx@16", 4, {{"HANDLE", "TimerHandle", sizeof(nt32::HANDLE)}, {"TIMER_SET_INFORMATION_CLASS", "TimerSetInformationClass", sizeof(nt32::TIMER_SET_INFORMATION_CLASS)}, {"PVOID", "TimerSetInformation", sizeof(nt32::PVOID)}, {"ULONG", "TimerSetInformationLength", sizeof(nt32::ULONG)}}},
        {"_ZwSetTimer@28", 7, {{"HANDLE", "TimerHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "DueTime", sizeof(nt32::PLARGE_INTEGER)}, {"PTIMER_APC_ROUTINE", "TimerApcRoutine", sizeof(nt32::PTIMER_APC_ROUTINE)}, {"PVOID", "TimerContext", sizeof(nt32::PVOID)}, {"BOOLEAN", "WakeTimer", sizeof(nt32::BOOLEAN)}, {"LONG", "Period", sizeof(nt32::LONG)}, {"PBOOLEAN", "PreviousState", sizeof(nt32::PBOOLEAN)}}},
        {"_NtSetTimerResolution@12", 3, {{"ULONG", "DesiredTime", sizeof(nt32::ULONG)}, {"BOOLEAN", "SetResolution", sizeof(nt32::BOOLEAN)}, {"PULONG", "ActualTime", sizeof(nt32::PULONG)}}},
        {"_ZwSetUuidSeed@4", 1, {{"PCHAR", "Seed", sizeof(nt32::PCHAR)}}},
        {"_ZwSetValueKey@24", 6, {{"HANDLE", "KeyHandle", sizeof(nt32::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(nt32::PUNICODE_STRING)}, {"ULONG", "TitleIndex", sizeof(nt32::ULONG)}, {"ULONG", "Type", sizeof(nt32::ULONG)}, {"PVOID", "Data", sizeof(nt32::PVOID)}, {"ULONG", "DataSize", sizeof(nt32::ULONG)}}},
        {"_NtSetVolumeInformationFile@20", 5, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "FsInformation", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"FS_INFORMATION_CLASS", "FsInformationClass", sizeof(nt32::FS_INFORMATION_CLASS)}}},
        {"_ZwShutdownSystem@4", 1, {{"SHUTDOWN_ACTION", "Action", sizeof(nt32::SHUTDOWN_ACTION)}}},
        {"_NtShutdownWorkerFactory@8", 2, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt32::HANDLE)}, {"LONG", "STARPendingWorkerCount", sizeof(nt32::LONG)}}},
        {"_ZwSignalAndWaitForSingleObject@16", 4, {{"HANDLE", "SignalHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "WaitHandle", sizeof(nt32::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(nt32::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwSinglePhaseReject@8", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt32::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtStartProfile@4", 1, {{"HANDLE", "ProfileHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwStopProfile@4", 1, {{"HANDLE", "ProfileHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwSuspendProcess@4", 1, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}}},
        {"_ZwSuspendThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(nt32::PULONG)}}},
        {"_NtSystemDebugControl@24", 6, {{"SYSDBG_COMMAND", "Command", sizeof(nt32::SYSDBG_COMMAND)}, {"PVOID", "InputBuffer", sizeof(nt32::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt32::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt32::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_ZwTerminateJobObject@8", 2, {{"HANDLE", "JobHandle", sizeof(nt32::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(nt32::NTSTATUS)}}},
        {"_ZwTerminateProcess@8", 2, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(nt32::NTSTATUS)}}},
        {"_ZwTerminateThread@8", 2, {{"HANDLE", "ThreadHandle", sizeof(nt32::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(nt32::NTSTATUS)}}},
        {"_ZwTraceControl@24", 6, {{"ULONG", "FunctionCode", sizeof(nt32::ULONG)}, {"PVOID", "InBuffer", sizeof(nt32::PVOID)}, {"ULONG", "InBufferLen", sizeof(nt32::ULONG)}, {"PVOID", "OutBuffer", sizeof(nt32::PVOID)}, {"ULONG", "OutBufferLen", sizeof(nt32::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt32::PULONG)}}},
        {"_NtTraceEvent@16", 4, {{"HANDLE", "TraceHandle", sizeof(nt32::HANDLE)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}, {"ULONG", "FieldSize", sizeof(nt32::ULONG)}, {"PVOID", "Fields", sizeof(nt32::PVOID)}}},
        {"_NtTranslateFilePath@16", 4, {{"PFILE_PATH", "InputFilePath", sizeof(nt32::PFILE_PATH)}, {"ULONG", "OutputType", sizeof(nt32::ULONG)}, {"PFILE_PATH", "OutputFilePath", sizeof(nt32::PFILE_PATH)}, {"PULONG", "OutputFilePathLength", sizeof(nt32::PULONG)}}},
        {"_ZwUnloadDriver@4", 1, {{"PUNICODE_STRING", "DriverServiceName", sizeof(nt32::PUNICODE_STRING)}}},
        {"_ZwUnloadKey2@8", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt32::ULONG)}}},
        {"_ZwUnloadKeyEx@8", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt32::POBJECT_ATTRIBUTES)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}}},
        {"_NtUnloadKey@4", 1, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt32::POBJECT_ATTRIBUTES)}}},
        {"_ZwUnlockFile@20", 5, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt32::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "Length", sizeof(nt32::PLARGE_INTEGER)}, {"ULONG", "Key", sizeof(nt32::ULONG)}}},
        {"_NtUnlockVirtualMemory@16", 4, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt32::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt32::PSIZE_T)}, {"ULONG", "MapType", sizeof(nt32::ULONG)}}},
        {"_NtUnmapViewOfSection@8", 2, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt32::PVOID)}}},
        {"_NtVdmControl@8", 2, {{"VDMSERVICECLASS", "Service", sizeof(nt32::VDMSERVICECLASS)}, {"PVOID", "ServiceData", sizeof(nt32::PVOID)}}},
        {"_NtWaitForDebugEvent@16", 4, {{"HANDLE", "DebugObjectHandle", sizeof(nt32::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(nt32::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}, {"PDBGUI_WAIT_STATE_CHANGE", "WaitStateChange", sizeof(nt32::PDBGUI_WAIT_STATE_CHANGE)}}},
        {"_NtWaitForKeyedEvent@16", 4, {{"HANDLE", "KeyedEventHandle", sizeof(nt32::HANDLE)}, {"PVOID", "KeyValue", sizeof(nt32::PVOID)}, {"BOOLEAN", "Alertable", sizeof(nt32::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtWaitForMultipleObjects32@20", 5, {{"ULONG", "Count", sizeof(nt32::ULONG)}, {"LONG", "Handles", sizeof(nt32::LONG)}, {"WAIT_TYPE", "WaitType", sizeof(nt32::WAIT_TYPE)}, {"BOOLEAN", "Alertable", sizeof(nt32::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtWaitForMultipleObjects@20", 5, {{"ULONG", "Count", sizeof(nt32::ULONG)}, {"HANDLE", "Handles", sizeof(nt32::HANDLE)}, {"WAIT_TYPE", "WaitType", sizeof(nt32::WAIT_TYPE)}, {"BOOLEAN", "Alertable", sizeof(nt32::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_ZwWaitForSingleObject@12", 3, {{"HANDLE", "Handle", sizeof(nt32::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(nt32::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt32::PLARGE_INTEGER)}}},
        {"_NtWaitForWorkViaWorkerFactory@20", 5, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt32::HANDLE)}, {"PFILE_IO_COMPLETION_INFORMATION", "MiniPacket", sizeof(nt32::PFILE_IO_COMPLETION_INFORMATION)}, {"PVOID", "Arg2", sizeof(nt32::PVOID)}, {"PVOID", "Arg3", sizeof(nt32::PVOID)}, {"PVOID", "Arg4", sizeof(nt32::PVOID)}}},
        {"_ZwWaitHighEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(nt32::HANDLE)}}},
        {"_NtWaitLowEventPair@4", 1, {{"HANDLE", "EventPairHandle", sizeof(nt32::HANDLE)}}},
        {"_NtWorkerFactoryWorkerReady@4", 1, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt32::HANDLE)}}},
        {"_NtWriteFileGather@36", 9, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PFILE_SEGMENT_ELEMENT", "SegmentArray", sizeof(nt32::PFILE_SEGMENT_ELEMENT)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt32::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt32::PULONG)}}},
        {"_NtWriteFile@36", 9, {{"HANDLE", "FileHandle", sizeof(nt32::HANDLE)}, {"HANDLE", "Event", sizeof(nt32::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt32::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt32::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt32::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"ULONG", "Length", sizeof(nt32::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt32::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt32::PULONG)}}},
        {"_NtWriteRequestData@24", 6, {{"HANDLE", "PortHandle", sizeof(nt32::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(nt32::PPORT_MESSAGE)}, {"ULONG", "DataEntryIndex", sizeof(nt32::ULONG)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt32::SIZE_T)}, {"PSIZE_T", "NumberOfBytesWritten", sizeof(nt32::PSIZE_T)}}},
        {"_NtWriteVirtualMemory@20", 5, {{"HANDLE", "ProcessHandle", sizeof(nt32::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt32::PVOID)}, {"PVOID", "Buffer", sizeof(nt32::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt32::SIZE_T)}, {"PSIZE_T", "NumberOfBytesWritten", sizeof(nt32::PSIZE_T)}}},
        {"_NtDisableLastKnownGood@0", 0, {}},
        {"_NtEnableLastKnownGood@0", 0, {}},
        {"_ZwFlushProcessWriteBuffers@0", 0, {}},
        {"_NtFlushWriteBuffer@0", 0, {}},
        {"_NtGetCurrentProcessorNumber@0", 0, {}},
        {"_NtIsSystemResumeAutomatic@0", 0, {}},
        {"_ZwIsUILanguageComitted@0", 0, {}},
        {"_ZwQueryPortInformationProcess@0", 0, {}},
        {"_NtSerializeBoot@0", 0, {}},
        {"_NtTestAlert@0", 0, {}},
        {"_ZwThawRegistry@0", 0, {}},
        {"_NtThawTransactions@0", 0, {}},
        {"_ZwUmsThreadYield@4", 1, {{"PVOID", "SchedulerParam", sizeof(nt32::PVOID)}}},
        {"_ZwYieldExecution@0", 0, {}},
	};
}

struct nt32::syscalls32::Data
{
    Data(core::Core& core, std::string_view module);

    using Breakpoints = std::vector<core::Breakpoint>;
    core::Core& core;
    std::string module;
    Breakpoints breakpoints;

    std::vector<on_NtAcceptConnectPort_fn>                                observers_NtAcceptConnectPort;
    std::vector<on_ZwAccessCheckAndAuditAlarm_fn>                         observers_ZwAccessCheckAndAuditAlarm;
    std::vector<on_NtAccessCheckByTypeAndAuditAlarm_fn>                   observers_NtAccessCheckByTypeAndAuditAlarm;
    std::vector<on_NtAccessCheckByType_fn>                                observers_NtAccessCheckByType;
    std::vector<on_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle_fn> observers_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle;
    std::vector<on_NtAccessCheckByTypeResultListAndAuditAlarm_fn>         observers_NtAccessCheckByTypeResultListAndAuditAlarm;
    std::vector<on_NtAccessCheckByTypeResultList_fn>                      observers_NtAccessCheckByTypeResultList;
    std::vector<on_NtAccessCheck_fn>                                      observers_NtAccessCheck;
    std::vector<on_NtAddAtom_fn>                                          observers_NtAddAtom;
    std::vector<on_ZwAddBootEntry_fn>                                     observers_ZwAddBootEntry;
    std::vector<on_NtAddDriverEntry_fn>                                   observers_NtAddDriverEntry;
    std::vector<on_ZwAdjustGroupsToken_fn>                                observers_ZwAdjustGroupsToken;
    std::vector<on_ZwAdjustPrivilegesToken_fn>                            observers_ZwAdjustPrivilegesToken;
    std::vector<on_NtAlertResumeThread_fn>                                observers_NtAlertResumeThread;
    std::vector<on_NtAlertThread_fn>                                      observers_NtAlertThread;
    std::vector<on_ZwAllocateLocallyUniqueId_fn>                          observers_ZwAllocateLocallyUniqueId;
    std::vector<on_NtAllocateReserveObject_fn>                            observers_NtAllocateReserveObject;
    std::vector<on_NtAllocateUserPhysicalPages_fn>                        observers_NtAllocateUserPhysicalPages;
    std::vector<on_NtAllocateUuids_fn>                                    observers_NtAllocateUuids;
    std::vector<on_NtAllocateVirtualMemory_fn>                            observers_NtAllocateVirtualMemory;
    std::vector<on_NtAlpcAcceptConnectPort_fn>                            observers_NtAlpcAcceptConnectPort;
    std::vector<on_ZwAlpcCancelMessage_fn>                                observers_ZwAlpcCancelMessage;
    std::vector<on_ZwAlpcConnectPort_fn>                                  observers_ZwAlpcConnectPort;
    std::vector<on_ZwAlpcCreatePort_fn>                                   observers_ZwAlpcCreatePort;
    std::vector<on_NtAlpcCreatePortSection_fn>                            observers_NtAlpcCreatePortSection;
    std::vector<on_ZwAlpcCreateResourceReserve_fn>                        observers_ZwAlpcCreateResourceReserve;
    std::vector<on_ZwAlpcCreateSectionView_fn>                            observers_ZwAlpcCreateSectionView;
    std::vector<on_ZwAlpcCreateSecurityContext_fn>                        observers_ZwAlpcCreateSecurityContext;
    std::vector<on_ZwAlpcDeletePortSection_fn>                            observers_ZwAlpcDeletePortSection;
    std::vector<on_NtAlpcDeleteResourceReserve_fn>                        observers_NtAlpcDeleteResourceReserve;
    std::vector<on_NtAlpcDeleteSectionView_fn>                            observers_NtAlpcDeleteSectionView;
    std::vector<on_NtAlpcDeleteSecurityContext_fn>                        observers_NtAlpcDeleteSecurityContext;
    std::vector<on_NtAlpcDisconnectPort_fn>                               observers_NtAlpcDisconnectPort;
    std::vector<on_ZwAlpcImpersonateClientOfPort_fn>                      observers_ZwAlpcImpersonateClientOfPort;
    std::vector<on_ZwAlpcOpenSenderProcess_fn>                            observers_ZwAlpcOpenSenderProcess;
    std::vector<on_ZwAlpcOpenSenderThread_fn>                             observers_ZwAlpcOpenSenderThread;
    std::vector<on_ZwAlpcQueryInformation_fn>                             observers_ZwAlpcQueryInformation;
    std::vector<on_ZwAlpcQueryInformationMessage_fn>                      observers_ZwAlpcQueryInformationMessage;
    std::vector<on_NtAlpcRevokeSecurityContext_fn>                        observers_NtAlpcRevokeSecurityContext;
    std::vector<on_NtAlpcSendWaitReceivePort_fn>                          observers_NtAlpcSendWaitReceivePort;
    std::vector<on_NtAlpcSetInformation_fn>                               observers_NtAlpcSetInformation;
    std::vector<on_NtApphelpCacheControl_fn>                              observers_NtApphelpCacheControl;
    std::vector<on_ZwAreMappedFilesTheSame_fn>                            observers_ZwAreMappedFilesTheSame;
    std::vector<on_ZwAssignProcessToJobObject_fn>                         observers_ZwAssignProcessToJobObject;
    std::vector<on_NtCancelIoFileEx_fn>                                   observers_NtCancelIoFileEx;
    std::vector<on_ZwCancelIoFile_fn>                                     observers_ZwCancelIoFile;
    std::vector<on_NtCancelSynchronousIoFile_fn>                          observers_NtCancelSynchronousIoFile;
    std::vector<on_ZwCancelTimer_fn>                                      observers_ZwCancelTimer;
    std::vector<on_NtClearEvent_fn>                                       observers_NtClearEvent;
    std::vector<on_NtClose_fn>                                            observers_NtClose;
    std::vector<on_ZwCloseObjectAuditAlarm_fn>                            observers_ZwCloseObjectAuditAlarm;
    std::vector<on_ZwCommitComplete_fn>                                   observers_ZwCommitComplete;
    std::vector<on_NtCommitEnlistment_fn>                                 observers_NtCommitEnlistment;
    std::vector<on_NtCommitTransaction_fn>                                observers_NtCommitTransaction;
    std::vector<on_NtCompactKeys_fn>                                      observers_NtCompactKeys;
    std::vector<on_ZwCompareTokens_fn>                                    observers_ZwCompareTokens;
    std::vector<on_NtCompleteConnectPort_fn>                              observers_NtCompleteConnectPort;
    std::vector<on_ZwCompressKey_fn>                                      observers_ZwCompressKey;
    std::vector<on_NtConnectPort_fn>                                      observers_NtConnectPort;
    std::vector<on_ZwContinue_fn>                                         observers_ZwContinue;
    std::vector<on_ZwCreateDebugObject_fn>                                observers_ZwCreateDebugObject;
    std::vector<on_ZwCreateDirectoryObject_fn>                            observers_ZwCreateDirectoryObject;
    std::vector<on_ZwCreateEnlistment_fn>                                 observers_ZwCreateEnlistment;
    std::vector<on_NtCreateEvent_fn>                                      observers_NtCreateEvent;
    std::vector<on_NtCreateEventPair_fn>                                  observers_NtCreateEventPair;
    std::vector<on_NtCreateFile_fn>                                       observers_NtCreateFile;
    std::vector<on_NtCreateIoCompletion_fn>                               observers_NtCreateIoCompletion;
    std::vector<on_ZwCreateJobObject_fn>                                  observers_ZwCreateJobObject;
    std::vector<on_NtCreateJobSet_fn>                                     observers_NtCreateJobSet;
    std::vector<on_ZwCreateKeyedEvent_fn>                                 observers_ZwCreateKeyedEvent;
    std::vector<on_ZwCreateKey_fn>                                        observers_ZwCreateKey;
    std::vector<on_NtCreateKeyTransacted_fn>                              observers_NtCreateKeyTransacted;
    std::vector<on_ZwCreateMailslotFile_fn>                               observers_ZwCreateMailslotFile;
    std::vector<on_ZwCreateMutant_fn>                                     observers_ZwCreateMutant;
    std::vector<on_ZwCreateNamedPipeFile_fn>                              observers_ZwCreateNamedPipeFile;
    std::vector<on_NtCreatePagingFile_fn>                                 observers_NtCreatePagingFile;
    std::vector<on_ZwCreatePort_fn>                                       observers_ZwCreatePort;
    std::vector<on_NtCreatePrivateNamespace_fn>                           observers_NtCreatePrivateNamespace;
    std::vector<on_ZwCreateProcessEx_fn>                                  observers_ZwCreateProcessEx;
    std::vector<on_ZwCreateProcess_fn>                                    observers_ZwCreateProcess;
    std::vector<on_NtCreateProfileEx_fn>                                  observers_NtCreateProfileEx;
    std::vector<on_ZwCreateProfile_fn>                                    observers_ZwCreateProfile;
    std::vector<on_ZwCreateResourceManager_fn>                            observers_ZwCreateResourceManager;
    std::vector<on_NtCreateSection_fn>                                    observers_NtCreateSection;
    std::vector<on_NtCreateSemaphore_fn>                                  observers_NtCreateSemaphore;
    std::vector<on_ZwCreateSymbolicLinkObject_fn>                         observers_ZwCreateSymbolicLinkObject;
    std::vector<on_NtCreateThreadEx_fn>                                   observers_NtCreateThreadEx;
    std::vector<on_NtCreateThread_fn>                                     observers_NtCreateThread;
    std::vector<on_ZwCreateTimer_fn>                                      observers_ZwCreateTimer;
    std::vector<on_NtCreateToken_fn>                                      observers_NtCreateToken;
    std::vector<on_ZwCreateTransactionManager_fn>                         observers_ZwCreateTransactionManager;
    std::vector<on_NtCreateTransaction_fn>                                observers_NtCreateTransaction;
    std::vector<on_NtCreateUserProcess_fn>                                observers_NtCreateUserProcess;
    std::vector<on_ZwCreateWaitablePort_fn>                               observers_ZwCreateWaitablePort;
    std::vector<on_NtCreateWorkerFactory_fn>                              observers_NtCreateWorkerFactory;
    std::vector<on_NtDebugActiveProcess_fn>                               observers_NtDebugActiveProcess;
    std::vector<on_ZwDebugContinue_fn>                                    observers_ZwDebugContinue;
    std::vector<on_ZwDelayExecution_fn>                                   observers_ZwDelayExecution;
    std::vector<on_ZwDeleteAtom_fn>                                       observers_ZwDeleteAtom;
    std::vector<on_NtDeleteBootEntry_fn>                                  observers_NtDeleteBootEntry;
    std::vector<on_ZwDeleteDriverEntry_fn>                                observers_ZwDeleteDriverEntry;
    std::vector<on_NtDeleteFile_fn>                                       observers_NtDeleteFile;
    std::vector<on_ZwDeleteKey_fn>                                        observers_ZwDeleteKey;
    std::vector<on_NtDeleteObjectAuditAlarm_fn>                           observers_NtDeleteObjectAuditAlarm;
    std::vector<on_NtDeletePrivateNamespace_fn>                           observers_NtDeletePrivateNamespace;
    std::vector<on_NtDeleteValueKey_fn>                                   observers_NtDeleteValueKey;
    std::vector<on_ZwDeviceIoControlFile_fn>                              observers_ZwDeviceIoControlFile;
    std::vector<on_NtDisplayString_fn>                                    observers_NtDisplayString;
    std::vector<on_ZwDrawText_fn>                                         observers_ZwDrawText;
    std::vector<on_ZwDuplicateObject_fn>                                  observers_ZwDuplicateObject;
    std::vector<on_NtDuplicateToken_fn>                                   observers_NtDuplicateToken;
    std::vector<on_ZwEnumerateBootEntries_fn>                             observers_ZwEnumerateBootEntries;
    std::vector<on_NtEnumerateDriverEntries_fn>                           observers_NtEnumerateDriverEntries;
    std::vector<on_ZwEnumerateKey_fn>                                     observers_ZwEnumerateKey;
    std::vector<on_ZwEnumerateSystemEnvironmentValuesEx_fn>               observers_ZwEnumerateSystemEnvironmentValuesEx;
    std::vector<on_ZwEnumerateTransactionObject_fn>                       observers_ZwEnumerateTransactionObject;
    std::vector<on_NtEnumerateValueKey_fn>                                observers_NtEnumerateValueKey;
    std::vector<on_ZwExtendSection_fn>                                    observers_ZwExtendSection;
    std::vector<on_NtFilterToken_fn>                                      observers_NtFilterToken;
    std::vector<on_NtFindAtom_fn>                                         observers_NtFindAtom;
    std::vector<on_ZwFlushBuffersFile_fn>                                 observers_ZwFlushBuffersFile;
    std::vector<on_ZwFlushInstallUILanguage_fn>                           observers_ZwFlushInstallUILanguage;
    std::vector<on_ZwFlushInstructionCache_fn>                            observers_ZwFlushInstructionCache;
    std::vector<on_NtFlushKey_fn>                                         observers_NtFlushKey;
    std::vector<on_ZwFlushVirtualMemory_fn>                               observers_ZwFlushVirtualMemory;
    std::vector<on_NtFreeUserPhysicalPages_fn>                            observers_NtFreeUserPhysicalPages;
    std::vector<on_NtFreeVirtualMemory_fn>                                observers_NtFreeVirtualMemory;
    std::vector<on_NtFreezeRegistry_fn>                                   observers_NtFreezeRegistry;
    std::vector<on_ZwFreezeTransactions_fn>                               observers_ZwFreezeTransactions;
    std::vector<on_NtFsControlFile_fn>                                    observers_NtFsControlFile;
    std::vector<on_NtGetContextThread_fn>                                 observers_NtGetContextThread;
    std::vector<on_NtGetDevicePowerState_fn>                              observers_NtGetDevicePowerState;
    std::vector<on_NtGetMUIRegistryInfo_fn>                               observers_NtGetMUIRegistryInfo;
    std::vector<on_ZwGetNextProcess_fn>                                   observers_ZwGetNextProcess;
    std::vector<on_ZwGetNextThread_fn>                                    observers_ZwGetNextThread;
    std::vector<on_NtGetNlsSectionPtr_fn>                                 observers_NtGetNlsSectionPtr;
    std::vector<on_ZwGetNotificationResourceManager_fn>                   observers_ZwGetNotificationResourceManager;
    std::vector<on_NtGetWriteWatch_fn>                                    observers_NtGetWriteWatch;
    std::vector<on_NtImpersonateAnonymousToken_fn>                        observers_NtImpersonateAnonymousToken;
    std::vector<on_ZwImpersonateClientOfPort_fn>                          observers_ZwImpersonateClientOfPort;
    std::vector<on_ZwImpersonateThread_fn>                                observers_ZwImpersonateThread;
    std::vector<on_NtInitializeNlsFiles_fn>                               observers_NtInitializeNlsFiles;
    std::vector<on_ZwInitializeRegistry_fn>                               observers_ZwInitializeRegistry;
    std::vector<on_NtInitiatePowerAction_fn>                              observers_NtInitiatePowerAction;
    std::vector<on_ZwIsProcessInJob_fn>                                   observers_ZwIsProcessInJob;
    std::vector<on_ZwListenPort_fn>                                       observers_ZwListenPort;
    std::vector<on_NtLoadDriver_fn>                                       observers_NtLoadDriver;
    std::vector<on_NtLoadKey2_fn>                                         observers_NtLoadKey2;
    std::vector<on_NtLoadKeyEx_fn>                                        observers_NtLoadKeyEx;
    std::vector<on_NtLoadKey_fn>                                          observers_NtLoadKey;
    std::vector<on_NtLockFile_fn>                                         observers_NtLockFile;
    std::vector<on_ZwLockProductActivationKeys_fn>                        observers_ZwLockProductActivationKeys;
    std::vector<on_NtLockRegistryKey_fn>                                  observers_NtLockRegistryKey;
    std::vector<on_ZwLockVirtualMemory_fn>                                observers_ZwLockVirtualMemory;
    std::vector<on_ZwMakePermanentObject_fn>                              observers_ZwMakePermanentObject;
    std::vector<on_NtMakeTemporaryObject_fn>                              observers_NtMakeTemporaryObject;
    std::vector<on_ZwMapCMFModule_fn>                                     observers_ZwMapCMFModule;
    std::vector<on_NtMapUserPhysicalPages_fn>                             observers_NtMapUserPhysicalPages;
    std::vector<on_ZwMapUserPhysicalPagesScatter_fn>                      observers_ZwMapUserPhysicalPagesScatter;
    std::vector<on_ZwMapViewOfSection_fn>                                 observers_ZwMapViewOfSection;
    std::vector<on_NtModifyBootEntry_fn>                                  observers_NtModifyBootEntry;
    std::vector<on_ZwModifyDriverEntry_fn>                                observers_ZwModifyDriverEntry;
    std::vector<on_NtNotifyChangeDirectoryFile_fn>                        observers_NtNotifyChangeDirectoryFile;
    std::vector<on_NtNotifyChangeKey_fn>                                  observers_NtNotifyChangeKey;
    std::vector<on_NtNotifyChangeMultipleKeys_fn>                         observers_NtNotifyChangeMultipleKeys;
    std::vector<on_NtNotifyChangeSession_fn>                              observers_NtNotifyChangeSession;
    std::vector<on_ZwOpenDirectoryObject_fn>                              observers_ZwOpenDirectoryObject;
    std::vector<on_ZwOpenEnlistment_fn>                                   observers_ZwOpenEnlistment;
    std::vector<on_NtOpenEvent_fn>                                        observers_NtOpenEvent;
    std::vector<on_NtOpenEventPair_fn>                                    observers_NtOpenEventPair;
    std::vector<on_NtOpenFile_fn>                                         observers_NtOpenFile;
    std::vector<on_ZwOpenIoCompletion_fn>                                 observers_ZwOpenIoCompletion;
    std::vector<on_ZwOpenJobObject_fn>                                    observers_ZwOpenJobObject;
    std::vector<on_NtOpenKeyedEvent_fn>                                   observers_NtOpenKeyedEvent;
    std::vector<on_ZwOpenKeyEx_fn>                                        observers_ZwOpenKeyEx;
    std::vector<on_ZwOpenKey_fn>                                          observers_ZwOpenKey;
    std::vector<on_NtOpenKeyTransactedEx_fn>                              observers_NtOpenKeyTransactedEx;
    std::vector<on_NtOpenKeyTransacted_fn>                                observers_NtOpenKeyTransacted;
    std::vector<on_NtOpenMutant_fn>                                       observers_NtOpenMutant;
    std::vector<on_ZwOpenObjectAuditAlarm_fn>                             observers_ZwOpenObjectAuditAlarm;
    std::vector<on_NtOpenPrivateNamespace_fn>                             observers_NtOpenPrivateNamespace;
    std::vector<on_ZwOpenProcess_fn>                                      observers_ZwOpenProcess;
    std::vector<on_ZwOpenProcessTokenEx_fn>                               observers_ZwOpenProcessTokenEx;
    std::vector<on_ZwOpenProcessToken_fn>                                 observers_ZwOpenProcessToken;
    std::vector<on_ZwOpenResourceManager_fn>                              observers_ZwOpenResourceManager;
    std::vector<on_NtOpenSection_fn>                                      observers_NtOpenSection;
    std::vector<on_NtOpenSemaphore_fn>                                    observers_NtOpenSemaphore;
    std::vector<on_NtOpenSession_fn>                                      observers_NtOpenSession;
    std::vector<on_NtOpenSymbolicLinkObject_fn>                           observers_NtOpenSymbolicLinkObject;
    std::vector<on_ZwOpenThread_fn>                                       observers_ZwOpenThread;
    std::vector<on_NtOpenThreadTokenEx_fn>                                observers_NtOpenThreadTokenEx;
    std::vector<on_NtOpenThreadToken_fn>                                  observers_NtOpenThreadToken;
    std::vector<on_ZwOpenTimer_fn>                                        observers_ZwOpenTimer;
    std::vector<on_ZwOpenTransactionManager_fn>                           observers_ZwOpenTransactionManager;
    std::vector<on_ZwOpenTransaction_fn>                                  observers_ZwOpenTransaction;
    std::vector<on_NtPlugPlayControl_fn>                                  observers_NtPlugPlayControl;
    std::vector<on_ZwPowerInformation_fn>                                 observers_ZwPowerInformation;
    std::vector<on_NtPrepareComplete_fn>                                  observers_NtPrepareComplete;
    std::vector<on_ZwPrepareEnlistment_fn>                                observers_ZwPrepareEnlistment;
    std::vector<on_ZwPrePrepareComplete_fn>                               observers_ZwPrePrepareComplete;
    std::vector<on_NtPrePrepareEnlistment_fn>                             observers_NtPrePrepareEnlistment;
    std::vector<on_ZwPrivilegeCheck_fn>                                   observers_ZwPrivilegeCheck;
    std::vector<on_NtPrivilegedServiceAuditAlarm_fn>                      observers_NtPrivilegedServiceAuditAlarm;
    std::vector<on_ZwPrivilegeObjectAuditAlarm_fn>                        observers_ZwPrivilegeObjectAuditAlarm;
    std::vector<on_NtPropagationComplete_fn>                              observers_NtPropagationComplete;
    std::vector<on_ZwPropagationFailed_fn>                                observers_ZwPropagationFailed;
    std::vector<on_ZwProtectVirtualMemory_fn>                             observers_ZwProtectVirtualMemory;
    std::vector<on_ZwPulseEvent_fn>                                       observers_ZwPulseEvent;
    std::vector<on_ZwQueryAttributesFile_fn>                              observers_ZwQueryAttributesFile;
    std::vector<on_ZwQueryBootEntryOrder_fn>                              observers_ZwQueryBootEntryOrder;
    std::vector<on_ZwQueryBootOptions_fn>                                 observers_ZwQueryBootOptions;
    std::vector<on_NtQueryDebugFilterState_fn>                            observers_NtQueryDebugFilterState;
    std::vector<on_NtQueryDefaultLocale_fn>                               observers_NtQueryDefaultLocale;
    std::vector<on_ZwQueryDefaultUILanguage_fn>                           observers_ZwQueryDefaultUILanguage;
    std::vector<on_ZwQueryDirectoryFile_fn>                               observers_ZwQueryDirectoryFile;
    std::vector<on_ZwQueryDirectoryObject_fn>                             observers_ZwQueryDirectoryObject;
    std::vector<on_NtQueryDriverEntryOrder_fn>                            observers_NtQueryDriverEntryOrder;
    std::vector<on_ZwQueryEaFile_fn>                                      observers_ZwQueryEaFile;
    std::vector<on_NtQueryEvent_fn>                                       observers_NtQueryEvent;
    std::vector<on_ZwQueryFullAttributesFile_fn>                          observers_ZwQueryFullAttributesFile;
    std::vector<on_NtQueryInformationAtom_fn>                             observers_NtQueryInformationAtom;
    std::vector<on_ZwQueryInformationEnlistment_fn>                       observers_ZwQueryInformationEnlistment;
    std::vector<on_ZwQueryInformationFile_fn>                             observers_ZwQueryInformationFile;
    std::vector<on_ZwQueryInformationJobObject_fn>                        observers_ZwQueryInformationJobObject;
    std::vector<on_ZwQueryInformationPort_fn>                             observers_ZwQueryInformationPort;
    std::vector<on_ZwQueryInformationProcess_fn>                          observers_ZwQueryInformationProcess;
    std::vector<on_ZwQueryInformationResourceManager_fn>                  observers_ZwQueryInformationResourceManager;
    std::vector<on_NtQueryInformationThread_fn>                           observers_NtQueryInformationThread;
    std::vector<on_ZwQueryInformationToken_fn>                            observers_ZwQueryInformationToken;
    std::vector<on_ZwQueryInformationTransaction_fn>                      observers_ZwQueryInformationTransaction;
    std::vector<on_NtQueryInformationTransactionManager_fn>               observers_NtQueryInformationTransactionManager;
    std::vector<on_ZwQueryInformationWorkerFactory_fn>                    observers_ZwQueryInformationWorkerFactory;
    std::vector<on_NtQueryInstallUILanguage_fn>                           observers_NtQueryInstallUILanguage;
    std::vector<on_NtQueryIntervalProfile_fn>                             observers_NtQueryIntervalProfile;
    std::vector<on_NtQueryIoCompletion_fn>                                observers_NtQueryIoCompletion;
    std::vector<on_ZwQueryKey_fn>                                         observers_ZwQueryKey;
    std::vector<on_NtQueryLicenseValue_fn>                                observers_NtQueryLicenseValue;
    std::vector<on_NtQueryMultipleValueKey_fn>                            observers_NtQueryMultipleValueKey;
    std::vector<on_NtQueryMutant_fn>                                      observers_NtQueryMutant;
    std::vector<on_NtQueryObject_fn>                                      observers_NtQueryObject;
    std::vector<on_NtQueryOpenSubKeysEx_fn>                               observers_NtQueryOpenSubKeysEx;
    std::vector<on_NtQueryOpenSubKeys_fn>                                 observers_NtQueryOpenSubKeys;
    std::vector<on_NtQueryPerformanceCounter_fn>                          observers_NtQueryPerformanceCounter;
    std::vector<on_ZwQueryQuotaInformationFile_fn>                        observers_ZwQueryQuotaInformationFile;
    std::vector<on_ZwQuerySection_fn>                                     observers_ZwQuerySection;
    std::vector<on_ZwQuerySecurityAttributesToken_fn>                     observers_ZwQuerySecurityAttributesToken;
    std::vector<on_NtQuerySecurityObject_fn>                              observers_NtQuerySecurityObject;
    std::vector<on_ZwQuerySemaphore_fn>                                   observers_ZwQuerySemaphore;
    std::vector<on_ZwQuerySymbolicLinkObject_fn>                          observers_ZwQuerySymbolicLinkObject;
    std::vector<on_ZwQuerySystemEnvironmentValueEx_fn>                    observers_ZwQuerySystemEnvironmentValueEx;
    std::vector<on_ZwQuerySystemEnvironmentValue_fn>                      observers_ZwQuerySystemEnvironmentValue;
    std::vector<on_ZwQuerySystemInformationEx_fn>                         observers_ZwQuerySystemInformationEx;
    std::vector<on_NtQuerySystemInformation_fn>                           observers_NtQuerySystemInformation;
    std::vector<on_NtQuerySystemTime_fn>                                  observers_NtQuerySystemTime;
    std::vector<on_ZwQueryTimer_fn>                                       observers_ZwQueryTimer;
    std::vector<on_NtQueryTimerResolution_fn>                             observers_NtQueryTimerResolution;
    std::vector<on_ZwQueryValueKey_fn>                                    observers_ZwQueryValueKey;
    std::vector<on_NtQueryVirtualMemory_fn>                               observers_NtQueryVirtualMemory;
    std::vector<on_NtQueryVolumeInformationFile_fn>                       observers_NtQueryVolumeInformationFile;
    std::vector<on_NtQueueApcThreadEx_fn>                                 observers_NtQueueApcThreadEx;
    std::vector<on_NtQueueApcThread_fn>                                   observers_NtQueueApcThread;
    std::vector<on_ZwRaiseException_fn>                                   observers_ZwRaiseException;
    std::vector<on_ZwRaiseHardError_fn>                                   observers_ZwRaiseHardError;
    std::vector<on_NtReadFile_fn>                                         observers_NtReadFile;
    std::vector<on_NtReadFileScatter_fn>                                  observers_NtReadFileScatter;
    std::vector<on_ZwReadOnlyEnlistment_fn>                               observers_ZwReadOnlyEnlistment;
    std::vector<on_ZwReadRequestData_fn>                                  observers_ZwReadRequestData;
    std::vector<on_NtReadVirtualMemory_fn>                                observers_NtReadVirtualMemory;
    std::vector<on_NtRecoverEnlistment_fn>                                observers_NtRecoverEnlistment;
    std::vector<on_NtRecoverResourceManager_fn>                           observers_NtRecoverResourceManager;
    std::vector<on_ZwRecoverTransactionManager_fn>                        observers_ZwRecoverTransactionManager;
    std::vector<on_ZwRegisterProtocolAddressInformation_fn>               observers_ZwRegisterProtocolAddressInformation;
    std::vector<on_ZwRegisterThreadTerminatePort_fn>                      observers_ZwRegisterThreadTerminatePort;
    std::vector<on_NtReleaseKeyedEvent_fn>                                observers_NtReleaseKeyedEvent;
    std::vector<on_ZwReleaseMutant_fn>                                    observers_ZwReleaseMutant;
    std::vector<on_NtReleaseSemaphore_fn>                                 observers_NtReleaseSemaphore;
    std::vector<on_ZwReleaseWorkerFactoryWorker_fn>                       observers_ZwReleaseWorkerFactoryWorker;
    std::vector<on_ZwRemoveIoCompletionEx_fn>                             observers_ZwRemoveIoCompletionEx;
    std::vector<on_ZwRemoveIoCompletion_fn>                               observers_ZwRemoveIoCompletion;
    std::vector<on_ZwRemoveProcessDebug_fn>                               observers_ZwRemoveProcessDebug;
    std::vector<on_ZwRenameKey_fn>                                        observers_ZwRenameKey;
    std::vector<on_NtRenameTransactionManager_fn>                         observers_NtRenameTransactionManager;
    std::vector<on_ZwReplaceKey_fn>                                       observers_ZwReplaceKey;
    std::vector<on_NtReplacePartitionUnit_fn>                             observers_NtReplacePartitionUnit;
    std::vector<on_ZwReplyPort_fn>                                        observers_ZwReplyPort;
    std::vector<on_NtReplyWaitReceivePortEx_fn>                           observers_NtReplyWaitReceivePortEx;
    std::vector<on_NtReplyWaitReceivePort_fn>                             observers_NtReplyWaitReceivePort;
    std::vector<on_NtReplyWaitReplyPort_fn>                               observers_NtReplyWaitReplyPort;
    std::vector<on_ZwRequestPort_fn>                                      observers_ZwRequestPort;
    std::vector<on_NtRequestWaitReplyPort_fn>                             observers_NtRequestWaitReplyPort;
    std::vector<on_NtResetEvent_fn>                                       observers_NtResetEvent;
    std::vector<on_ZwResetWriteWatch_fn>                                  observers_ZwResetWriteWatch;
    std::vector<on_NtRestoreKey_fn>                                       observers_NtRestoreKey;
    std::vector<on_ZwResumeProcess_fn>                                    observers_ZwResumeProcess;
    std::vector<on_ZwResumeThread_fn>                                     observers_ZwResumeThread;
    std::vector<on_ZwRollbackComplete_fn>                                 observers_ZwRollbackComplete;
    std::vector<on_NtRollbackEnlistment_fn>                               observers_NtRollbackEnlistment;
    std::vector<on_NtRollbackTransaction_fn>                              observers_NtRollbackTransaction;
    std::vector<on_NtRollforwardTransactionManager_fn>                    observers_NtRollforwardTransactionManager;
    std::vector<on_NtSaveKeyEx_fn>                                        observers_NtSaveKeyEx;
    std::vector<on_NtSaveKey_fn>                                          observers_NtSaveKey;
    std::vector<on_NtSaveMergedKeys_fn>                                   observers_NtSaveMergedKeys;
    std::vector<on_NtSecureConnectPort_fn>                                observers_NtSecureConnectPort;
    std::vector<on_ZwSetBootEntryOrder_fn>                                observers_ZwSetBootEntryOrder;
    std::vector<on_ZwSetBootOptions_fn>                                   observers_ZwSetBootOptions;
    std::vector<on_ZwSetContextThread_fn>                                 observers_ZwSetContextThread;
    std::vector<on_NtSetDebugFilterState_fn>                              observers_NtSetDebugFilterState;
    std::vector<on_NtSetDefaultHardErrorPort_fn>                          observers_NtSetDefaultHardErrorPort;
    std::vector<on_NtSetDefaultLocale_fn>                                 observers_NtSetDefaultLocale;
    std::vector<on_ZwSetDefaultUILanguage_fn>                             observers_ZwSetDefaultUILanguage;
    std::vector<on_NtSetDriverEntryOrder_fn>                              observers_NtSetDriverEntryOrder;
    std::vector<on_ZwSetEaFile_fn>                                        observers_ZwSetEaFile;
    std::vector<on_NtSetEventBoostPriority_fn>                            observers_NtSetEventBoostPriority;
    std::vector<on_NtSetEvent_fn>                                         observers_NtSetEvent;
    std::vector<on_NtSetHighEventPair_fn>                                 observers_NtSetHighEventPair;
    std::vector<on_NtSetHighWaitLowEventPair_fn>                          observers_NtSetHighWaitLowEventPair;
    std::vector<on_ZwSetInformationDebugObject_fn>                        observers_ZwSetInformationDebugObject;
    std::vector<on_NtSetInformationEnlistment_fn>                         observers_NtSetInformationEnlistment;
    std::vector<on_ZwSetInformationFile_fn>                               observers_ZwSetInformationFile;
    std::vector<on_ZwSetInformationJobObject_fn>                          observers_ZwSetInformationJobObject;
    std::vector<on_ZwSetInformationKey_fn>                                observers_ZwSetInformationKey;
    std::vector<on_ZwSetInformationObject_fn>                             observers_ZwSetInformationObject;
    std::vector<on_ZwSetInformationProcess_fn>                            observers_ZwSetInformationProcess;
    std::vector<on_ZwSetInformationResourceManager_fn>                    observers_ZwSetInformationResourceManager;
    std::vector<on_ZwSetInformationThread_fn>                             observers_ZwSetInformationThread;
    std::vector<on_ZwSetInformationToken_fn>                              observers_ZwSetInformationToken;
    std::vector<on_ZwSetInformationTransaction_fn>                        observers_ZwSetInformationTransaction;
    std::vector<on_ZwSetInformationTransactionManager_fn>                 observers_ZwSetInformationTransactionManager;
    std::vector<on_ZwSetInformationWorkerFactory_fn>                      observers_ZwSetInformationWorkerFactory;
    std::vector<on_NtSetIntervalProfile_fn>                               observers_NtSetIntervalProfile;
    std::vector<on_NtSetIoCompletionEx_fn>                                observers_NtSetIoCompletionEx;
    std::vector<on_NtSetIoCompletion_fn>                                  observers_NtSetIoCompletion;
    std::vector<on_ZwSetLdtEntries_fn>                                    observers_ZwSetLdtEntries;
    std::vector<on_ZwSetLowEventPair_fn>                                  observers_ZwSetLowEventPair;
    std::vector<on_ZwSetLowWaitHighEventPair_fn>                          observers_ZwSetLowWaitHighEventPair;
    std::vector<on_ZwSetQuotaInformationFile_fn>                          observers_ZwSetQuotaInformationFile;
    std::vector<on_NtSetSecurityObject_fn>                                observers_NtSetSecurityObject;
    std::vector<on_ZwSetSystemEnvironmentValueEx_fn>                      observers_ZwSetSystemEnvironmentValueEx;
    std::vector<on_ZwSetSystemEnvironmentValue_fn>                        observers_ZwSetSystemEnvironmentValue;
    std::vector<on_ZwSetSystemInformation_fn>                             observers_ZwSetSystemInformation;
    std::vector<on_ZwSetSystemPowerState_fn>                              observers_ZwSetSystemPowerState;
    std::vector<on_ZwSetSystemTime_fn>                                    observers_ZwSetSystemTime;
    std::vector<on_ZwSetThreadExecutionState_fn>                          observers_ZwSetThreadExecutionState;
    std::vector<on_ZwSetTimerEx_fn>                                       observers_ZwSetTimerEx;
    std::vector<on_ZwSetTimer_fn>                                         observers_ZwSetTimer;
    std::vector<on_NtSetTimerResolution_fn>                               observers_NtSetTimerResolution;
    std::vector<on_ZwSetUuidSeed_fn>                                      observers_ZwSetUuidSeed;
    std::vector<on_ZwSetValueKey_fn>                                      observers_ZwSetValueKey;
    std::vector<on_NtSetVolumeInformationFile_fn>                         observers_NtSetVolumeInformationFile;
    std::vector<on_ZwShutdownSystem_fn>                                   observers_ZwShutdownSystem;
    std::vector<on_NtShutdownWorkerFactory_fn>                            observers_NtShutdownWorkerFactory;
    std::vector<on_ZwSignalAndWaitForSingleObject_fn>                     observers_ZwSignalAndWaitForSingleObject;
    std::vector<on_ZwSinglePhaseReject_fn>                                observers_ZwSinglePhaseReject;
    std::vector<on_NtStartProfile_fn>                                     observers_NtStartProfile;
    std::vector<on_ZwStopProfile_fn>                                      observers_ZwStopProfile;
    std::vector<on_ZwSuspendProcess_fn>                                   observers_ZwSuspendProcess;
    std::vector<on_ZwSuspendThread_fn>                                    observers_ZwSuspendThread;
    std::vector<on_NtSystemDebugControl_fn>                               observers_NtSystemDebugControl;
    std::vector<on_ZwTerminateJobObject_fn>                               observers_ZwTerminateJobObject;
    std::vector<on_ZwTerminateProcess_fn>                                 observers_ZwTerminateProcess;
    std::vector<on_ZwTerminateThread_fn>                                  observers_ZwTerminateThread;
    std::vector<on_ZwTraceControl_fn>                                     observers_ZwTraceControl;
    std::vector<on_NtTraceEvent_fn>                                       observers_NtTraceEvent;
    std::vector<on_NtTranslateFilePath_fn>                                observers_NtTranslateFilePath;
    std::vector<on_ZwUnloadDriver_fn>                                     observers_ZwUnloadDriver;
    std::vector<on_ZwUnloadKey2_fn>                                       observers_ZwUnloadKey2;
    std::vector<on_ZwUnloadKeyEx_fn>                                      observers_ZwUnloadKeyEx;
    std::vector<on_NtUnloadKey_fn>                                        observers_NtUnloadKey;
    std::vector<on_ZwUnlockFile_fn>                                       observers_ZwUnlockFile;
    std::vector<on_NtUnlockVirtualMemory_fn>                              observers_NtUnlockVirtualMemory;
    std::vector<on_NtUnmapViewOfSection_fn>                               observers_NtUnmapViewOfSection;
    std::vector<on_NtVdmControl_fn>                                       observers_NtVdmControl;
    std::vector<on_NtWaitForDebugEvent_fn>                                observers_NtWaitForDebugEvent;
    std::vector<on_NtWaitForKeyedEvent_fn>                                observers_NtWaitForKeyedEvent;
    std::vector<on_NtWaitForMultipleObjects32_fn>                         observers_NtWaitForMultipleObjects32;
    std::vector<on_NtWaitForMultipleObjects_fn>                           observers_NtWaitForMultipleObjects;
    std::vector<on_ZwWaitForSingleObject_fn>                              observers_ZwWaitForSingleObject;
    std::vector<on_NtWaitForWorkViaWorkerFactory_fn>                      observers_NtWaitForWorkViaWorkerFactory;
    std::vector<on_ZwWaitHighEventPair_fn>                                observers_ZwWaitHighEventPair;
    std::vector<on_NtWaitLowEventPair_fn>                                 observers_NtWaitLowEventPair;
    std::vector<on_NtWorkerFactoryWorkerReady_fn>                         observers_NtWorkerFactoryWorkerReady;
    std::vector<on_NtWriteFileGather_fn>                                  observers_NtWriteFileGather;
    std::vector<on_NtWriteFile_fn>                                        observers_NtWriteFile;
    std::vector<on_NtWriteRequestData_fn>                                 observers_NtWriteRequestData;
    std::vector<on_NtWriteVirtualMemory_fn>                               observers_NtWriteVirtualMemory;
    std::vector<on_NtDisableLastKnownGood_fn>                             observers_NtDisableLastKnownGood;
    std::vector<on_NtEnableLastKnownGood_fn>                              observers_NtEnableLastKnownGood;
    std::vector<on_ZwFlushProcessWriteBuffers_fn>                         observers_ZwFlushProcessWriteBuffers;
    std::vector<on_NtFlushWriteBuffer_fn>                                 observers_NtFlushWriteBuffer;
    std::vector<on_NtGetCurrentProcessorNumber_fn>                        observers_NtGetCurrentProcessorNumber;
    std::vector<on_NtIsSystemResumeAutomatic_fn>                          observers_NtIsSystemResumeAutomatic;
    std::vector<on_ZwIsUILanguageComitted_fn>                             observers_ZwIsUILanguageComitted;
    std::vector<on_ZwQueryPortInformationProcess_fn>                      observers_ZwQueryPortInformationProcess;
    std::vector<on_NtSerializeBoot_fn>                                    observers_NtSerializeBoot;
    std::vector<on_NtTestAlert_fn>                                        observers_NtTestAlert;
    std::vector<on_ZwThawRegistry_fn>                                     observers_ZwThawRegistry;
    std::vector<on_NtThawTransactions_fn>                                 observers_NtThawTransactions;
    std::vector<on_ZwUmsThreadYield_fn>                                   observers_ZwUmsThreadYield;
    std::vector<on_ZwYieldExecution_fn>                                   observers_ZwYieldExecution;
};

nt32::syscalls32::Data::Data(core::Core& core, std::string_view module)
    : core(core)
    , module(module)
{
}

nt32::syscalls32::syscalls32(core::Core& core, std::string_view module)
    : d_(std::make_unique<Data>(core, module))
{
}

nt32::syscalls32::~syscalls32() = default;

namespace
{
    using Data = nt32::syscalls32::Data;

    static core::Breakpoint register_callback(Data& d, proc_t proc, const char* name, const core::Task& on_call)
    {
        const auto addr = d.core.sym.symbol(d.module, name);
        if(!addr)
            return FAIL(nullptr, "unable to find symbole {}!{}", d.module.data(), name);

        return d.core.state.set_breakpoint(*addr, proc, on_call);
    }

    static bool register_callback_with(Data& d, proc_t proc, const char* name, void (*callback)(Data&))
    {
        const auto dptr = &d;
        const auto bp = register_callback(d, proc, name, [=]
        {
            callback(*dptr);
        });
        if(!bp)
            return false;

        d.breakpoints.emplace_back(bp);
        return true;
    }

    template <typename T>
    static T arg(core::Core& core, size_t i)
    {
        const auto arg = core.os->read_arg(i);
        if(!arg)
            return {};

        return nt32::cast_to<T>(*arg);
    }

    static void on_NtAcceptConnectPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle        = arg<nt32::PHANDLE>(d.core, 0);
        const auto PortContext       = arg<nt32::PVOID>(d.core, 1);
        const auto ConnectionRequest = arg<nt32::PPORT_MESSAGE>(d.core, 2);
        const auto AcceptConnection  = arg<nt32::BOOLEAN>(d.core, 3);
        const auto ServerView        = arg<nt32::PPORT_VIEW>(d.core, 4);
        const auto ClientView        = arg<nt32::PREMOTE_PORT_VIEW>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAcceptConnectPort(PortHandle:{:#x}, PortContext:{:#x}, ConnectionRequest:{:#x}, AcceptConnection:{:#x}, ServerView:{:#x}, ClientView:{:#x})", PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView));

        for(const auto& it : d.observers_NtAcceptConnectPort)
            it(PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView);
    }

    static void on_ZwAccessCheckAndAuditAlarm(nt32::syscalls32::Data& d)
    {
        const auto SubsystemName      = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto HandleId           = arg<nt32::PVOID>(d.core, 1);
        const auto ObjectTypeName     = arg<nt32::PUNICODE_STRING>(d.core, 2);
        const auto ObjectName         = arg<nt32::PUNICODE_STRING>(d.core, 3);
        const auto SecurityDescriptor = arg<nt32::PSECURITY_DESCRIPTOR>(d.core, 4);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(d.core, 5);
        const auto GenericMapping     = arg<nt32::PGENERIC_MAPPING>(d.core, 6);
        const auto ObjectCreation     = arg<nt32::BOOLEAN>(d.core, 7);
        const auto GrantedAccess      = arg<nt32::PACCESS_MASK>(d.core, 8);
        const auto AccessStatus       = arg<nt32::PNTSTATUS>(d.core, 9);
        const auto GenerateOnClose    = arg<nt32::PBOOLEAN>(d.core, 10);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAccessCheckAndAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, ObjectTypeName:{:#x}, ObjectName:{:#x}, SecurityDescriptor:{:#x}, DesiredAccess:{:#x}, GenericMapping:{:#x}, ObjectCreation:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose));

        for(const auto& it : d.observers_ZwAccessCheckAndAuditAlarm)
            it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }

    static void on_NtAccessCheckByTypeAndAuditAlarm(nt32::syscalls32::Data& d)
    {
        const auto SubsystemName        = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto HandleId             = arg<nt32::PVOID>(d.core, 1);
        const auto ObjectTypeName       = arg<nt32::PUNICODE_STRING>(d.core, 2);
        const auto ObjectName           = arg<nt32::PUNICODE_STRING>(d.core, 3);
        const auto SecurityDescriptor   = arg<nt32::PSECURITY_DESCRIPTOR>(d.core, 4);
        const auto PrincipalSelfSid     = arg<nt32::PSID>(d.core, 5);
        const auto DesiredAccess        = arg<nt32::ACCESS_MASK>(d.core, 6);
        const auto AuditType            = arg<nt32::AUDIT_EVENT_TYPE>(d.core, 7);
        const auto Flags                = arg<nt32::ULONG>(d.core, 8);
        const auto ObjectTypeList       = arg<nt32::POBJECT_TYPE_LIST>(d.core, 9);
        const auto ObjectTypeListLength = arg<nt32::ULONG>(d.core, 10);
        const auto GenericMapping       = arg<nt32::PGENERIC_MAPPING>(d.core, 11);
        const auto ObjectCreation       = arg<nt32::BOOLEAN>(d.core, 12);
        const auto GrantedAccess        = arg<nt32::PACCESS_MASK>(d.core, 13);
        const auto AccessStatus         = arg<nt32::PNTSTATUS>(d.core, 14);
        const auto GenerateOnClose      = arg<nt32::PBOOLEAN>(d.core, 15);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAccessCheckByTypeAndAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, ObjectTypeName:{:#x}, ObjectName:{:#x}, SecurityDescriptor:{:#x}, PrincipalSelfSid:{:#x}, DesiredAccess:{:#x}, AuditType:{:#x}, Flags:{:#x}, ObjectTypeList:{:#x}, ObjectTypeListLength:{:#x}, GenericMapping:{:#x}, ObjectCreation:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose));

        for(const auto& it : d.observers_NtAccessCheckByTypeAndAuditAlarm)
            it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }

    static void on_NtAccessCheckByType(nt32::syscalls32::Data& d)
    {
        const auto SecurityDescriptor   = arg<nt32::PSECURITY_DESCRIPTOR>(d.core, 0);
        const auto PrincipalSelfSid     = arg<nt32::PSID>(d.core, 1);
        const auto ClientToken          = arg<nt32::HANDLE>(d.core, 2);
        const auto DesiredAccess        = arg<nt32::ACCESS_MASK>(d.core, 3);
        const auto ObjectTypeList       = arg<nt32::POBJECT_TYPE_LIST>(d.core, 4);
        const auto ObjectTypeListLength = arg<nt32::ULONG>(d.core, 5);
        const auto GenericMapping       = arg<nt32::PGENERIC_MAPPING>(d.core, 6);
        const auto PrivilegeSet         = arg<nt32::PPRIVILEGE_SET>(d.core, 7);
        const auto PrivilegeSetLength   = arg<nt32::PULONG>(d.core, 8);
        const auto GrantedAccess        = arg<nt32::PACCESS_MASK>(d.core, 9);
        const auto AccessStatus         = arg<nt32::PNTSTATUS>(d.core, 10);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAccessCheckByType(SecurityDescriptor:{:#x}, PrincipalSelfSid:{:#x}, ClientToken:{:#x}, DesiredAccess:{:#x}, ObjectTypeList:{:#x}, ObjectTypeListLength:{:#x}, GenericMapping:{:#x}, PrivilegeSet:{:#x}, PrivilegeSetLength:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x})", SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus));

        for(const auto& it : d.observers_NtAccessCheckByType)
            it(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    }

    static void on_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(nt32::syscalls32::Data& d)
    {
        const auto SubsystemName        = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto HandleId             = arg<nt32::PVOID>(d.core, 1);
        const auto ClientToken          = arg<nt32::HANDLE>(d.core, 2);
        const auto ObjectTypeName       = arg<nt32::PUNICODE_STRING>(d.core, 3);
        const auto ObjectName           = arg<nt32::PUNICODE_STRING>(d.core, 4);
        const auto SecurityDescriptor   = arg<nt32::PSECURITY_DESCRIPTOR>(d.core, 5);
        const auto PrincipalSelfSid     = arg<nt32::PSID>(d.core, 6);
        const auto DesiredAccess        = arg<nt32::ACCESS_MASK>(d.core, 7);
        const auto AuditType            = arg<nt32::AUDIT_EVENT_TYPE>(d.core, 8);
        const auto Flags                = arg<nt32::ULONG>(d.core, 9);
        const auto ObjectTypeList       = arg<nt32::POBJECT_TYPE_LIST>(d.core, 10);
        const auto ObjectTypeListLength = arg<nt32::ULONG>(d.core, 11);
        const auto GenericMapping       = arg<nt32::PGENERIC_MAPPING>(d.core, 12);
        const auto ObjectCreation       = arg<nt32::BOOLEAN>(d.core, 13);
        const auto GrantedAccess        = arg<nt32::PACCESS_MASK>(d.core, 14);
        const auto AccessStatus         = arg<nt32::PNTSTATUS>(d.core, 15);
        const auto GenerateOnClose      = arg<nt32::PBOOLEAN>(d.core, 16);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(SubsystemName:{:#x}, HandleId:{:#x}, ClientToken:{:#x}, ObjectTypeName:{:#x}, ObjectName:{:#x}, SecurityDescriptor:{:#x}, PrincipalSelfSid:{:#x}, DesiredAccess:{:#x}, AuditType:{:#x}, Flags:{:#x}, ObjectTypeList:{:#x}, ObjectTypeListLength:{:#x}, GenericMapping:{:#x}, ObjectCreation:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose));

        for(const auto& it : d.observers_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle)
            it(SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }

    static void on_NtAccessCheckByTypeResultListAndAuditAlarm(nt32::syscalls32::Data& d)
    {
        const auto SubsystemName        = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto HandleId             = arg<nt32::PVOID>(d.core, 1);
        const auto ObjectTypeName       = arg<nt32::PUNICODE_STRING>(d.core, 2);
        const auto ObjectName           = arg<nt32::PUNICODE_STRING>(d.core, 3);
        const auto SecurityDescriptor   = arg<nt32::PSECURITY_DESCRIPTOR>(d.core, 4);
        const auto PrincipalSelfSid     = arg<nt32::PSID>(d.core, 5);
        const auto DesiredAccess        = arg<nt32::ACCESS_MASK>(d.core, 6);
        const auto AuditType            = arg<nt32::AUDIT_EVENT_TYPE>(d.core, 7);
        const auto Flags                = arg<nt32::ULONG>(d.core, 8);
        const auto ObjectTypeList       = arg<nt32::POBJECT_TYPE_LIST>(d.core, 9);
        const auto ObjectTypeListLength = arg<nt32::ULONG>(d.core, 10);
        const auto GenericMapping       = arg<nt32::PGENERIC_MAPPING>(d.core, 11);
        const auto ObjectCreation       = arg<nt32::BOOLEAN>(d.core, 12);
        const auto GrantedAccess        = arg<nt32::PACCESS_MASK>(d.core, 13);
        const auto AccessStatus         = arg<nt32::PNTSTATUS>(d.core, 14);
        const auto GenerateOnClose      = arg<nt32::PBOOLEAN>(d.core, 15);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAccessCheckByTypeResultListAndAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, ObjectTypeName:{:#x}, ObjectName:{:#x}, SecurityDescriptor:{:#x}, PrincipalSelfSid:{:#x}, DesiredAccess:{:#x}, AuditType:{:#x}, Flags:{:#x}, ObjectTypeList:{:#x}, ObjectTypeListLength:{:#x}, GenericMapping:{:#x}, ObjectCreation:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose));

        for(const auto& it : d.observers_NtAccessCheckByTypeResultListAndAuditAlarm)
            it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }

    static void on_NtAccessCheckByTypeResultList(nt32::syscalls32::Data& d)
    {
        const auto SecurityDescriptor   = arg<nt32::PSECURITY_DESCRIPTOR>(d.core, 0);
        const auto PrincipalSelfSid     = arg<nt32::PSID>(d.core, 1);
        const auto ClientToken          = arg<nt32::HANDLE>(d.core, 2);
        const auto DesiredAccess        = arg<nt32::ACCESS_MASK>(d.core, 3);
        const auto ObjectTypeList       = arg<nt32::POBJECT_TYPE_LIST>(d.core, 4);
        const auto ObjectTypeListLength = arg<nt32::ULONG>(d.core, 5);
        const auto GenericMapping       = arg<nt32::PGENERIC_MAPPING>(d.core, 6);
        const auto PrivilegeSet         = arg<nt32::PPRIVILEGE_SET>(d.core, 7);
        const auto PrivilegeSetLength   = arg<nt32::PULONG>(d.core, 8);
        const auto GrantedAccess        = arg<nt32::PACCESS_MASK>(d.core, 9);
        const auto AccessStatus         = arg<nt32::PNTSTATUS>(d.core, 10);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAccessCheckByTypeResultList(SecurityDescriptor:{:#x}, PrincipalSelfSid:{:#x}, ClientToken:{:#x}, DesiredAccess:{:#x}, ObjectTypeList:{:#x}, ObjectTypeListLength:{:#x}, GenericMapping:{:#x}, PrivilegeSet:{:#x}, PrivilegeSetLength:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x})", SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus));

        for(const auto& it : d.observers_NtAccessCheckByTypeResultList)
            it(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    }

    static void on_NtAccessCheck(nt32::syscalls32::Data& d)
    {
        const auto SecurityDescriptor = arg<nt32::PSECURITY_DESCRIPTOR>(d.core, 0);
        const auto ClientToken        = arg<nt32::HANDLE>(d.core, 1);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(d.core, 2);
        const auto GenericMapping     = arg<nt32::PGENERIC_MAPPING>(d.core, 3);
        const auto PrivilegeSet       = arg<nt32::PPRIVILEGE_SET>(d.core, 4);
        const auto PrivilegeSetLength = arg<nt32::PULONG>(d.core, 5);
        const auto GrantedAccess      = arg<nt32::PACCESS_MASK>(d.core, 6);
        const auto AccessStatus       = arg<nt32::PNTSTATUS>(d.core, 7);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAccessCheck(SecurityDescriptor:{:#x}, ClientToken:{:#x}, DesiredAccess:{:#x}, GenericMapping:{:#x}, PrivilegeSet:{:#x}, PrivilegeSetLength:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x})", SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus));

        for(const auto& it : d.observers_NtAccessCheck)
            it(SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    }

    static void on_NtAddAtom(nt32::syscalls32::Data& d)
    {
        const auto AtomName = arg<nt32::PWSTR>(d.core, 0);
        const auto Length   = arg<nt32::ULONG>(d.core, 1);
        const auto Atom     = arg<nt32::PRTL_ATOM>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAddAtom(AtomName:{:#x}, Length:{:#x}, Atom:{:#x})", AtomName, Length, Atom));

        for(const auto& it : d.observers_NtAddAtom)
            it(AtomName, Length, Atom);
    }

    static void on_ZwAddBootEntry(nt32::syscalls32::Data& d)
    {
        const auto BootEntry = arg<nt32::PBOOT_ENTRY>(d.core, 0);
        const auto Id        = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAddBootEntry(BootEntry:{:#x}, Id:{:#x})", BootEntry, Id));

        for(const auto& it : d.observers_ZwAddBootEntry)
            it(BootEntry, Id);
    }

    static void on_NtAddDriverEntry(nt32::syscalls32::Data& d)
    {
        const auto DriverEntry = arg<nt32::PEFI_DRIVER_ENTRY>(d.core, 0);
        const auto Id          = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAddDriverEntry(DriverEntry:{:#x}, Id:{:#x})", DriverEntry, Id));

        for(const auto& it : d.observers_NtAddDriverEntry)
            it(DriverEntry, Id);
    }

    static void on_ZwAdjustGroupsToken(nt32::syscalls32::Data& d)
    {
        const auto TokenHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto ResetToDefault = arg<nt32::BOOLEAN>(d.core, 1);
        const auto NewState       = arg<nt32::PTOKEN_GROUPS>(d.core, 2);
        const auto BufferLength   = arg<nt32::ULONG>(d.core, 3);
        const auto PreviousState  = arg<nt32::PTOKEN_GROUPS>(d.core, 4);
        const auto ReturnLength   = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAdjustGroupsToken(TokenHandle:{:#x}, ResetToDefault:{:#x}, NewState:{:#x}, BufferLength:{:#x}, PreviousState:{:#x}, ReturnLength:{:#x})", TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength));

        for(const auto& it : d.observers_ZwAdjustGroupsToken)
            it(TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength);
    }

    static void on_ZwAdjustPrivilegesToken(nt32::syscalls32::Data& d)
    {
        const auto TokenHandle          = arg<nt32::HANDLE>(d.core, 0);
        const auto DisableAllPrivileges = arg<nt32::BOOLEAN>(d.core, 1);
        const auto NewState             = arg<nt32::PTOKEN_PRIVILEGES>(d.core, 2);
        const auto BufferLength         = arg<nt32::ULONG>(d.core, 3);
        const auto PreviousState        = arg<nt32::PTOKEN_PRIVILEGES>(d.core, 4);
        const auto ReturnLength         = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAdjustPrivilegesToken(TokenHandle:{:#x}, DisableAllPrivileges:{:#x}, NewState:{:#x}, BufferLength:{:#x}, PreviousState:{:#x}, ReturnLength:{:#x})", TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength));

        for(const auto& it : d.observers_ZwAdjustPrivilegesToken)
            it(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
    }

    static void on_NtAlertResumeThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle         = arg<nt32::HANDLE>(d.core, 0);
        const auto PreviousSuspendCount = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlertResumeThread(ThreadHandle:{:#x}, PreviousSuspendCount:{:#x})", ThreadHandle, PreviousSuspendCount));

        for(const auto& it : d.observers_NtAlertResumeThread)
            it(ThreadHandle, PreviousSuspendCount);
    }

    static void on_NtAlertThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlertThread(ThreadHandle:{:#x})", ThreadHandle));

        for(const auto& it : d.observers_NtAlertThread)
            it(ThreadHandle);
    }

    static void on_ZwAllocateLocallyUniqueId(nt32::syscalls32::Data& d)
    {
        const auto Luid = arg<nt32::PLUID>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAllocateLocallyUniqueId(Luid:{:#x})", Luid));

        for(const auto& it : d.observers_ZwAllocateLocallyUniqueId)
            it(Luid);
    }

    static void on_NtAllocateReserveObject(nt32::syscalls32::Data& d)
    {
        const auto MemoryReserveHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto ObjectAttributes    = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto Type                = arg<nt32::MEMORY_RESERVE_TYPE>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAllocateReserveObject(MemoryReserveHandle:{:#x}, ObjectAttributes:{:#x}, Type:{:#x})", MemoryReserveHandle, ObjectAttributes, Type));

        for(const auto& it : d.observers_NtAllocateReserveObject)
            it(MemoryReserveHandle, ObjectAttributes, Type);
    }

    static void on_NtAllocateUserPhysicalPages(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto NumberOfPages = arg<nt32::PULONG_PTR>(d.core, 1);
        const auto UserPfnArra   = arg<nt32::PULONG_PTR>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAllocateUserPhysicalPages(ProcessHandle:{:#x}, NumberOfPages:{:#x}, UserPfnArra:{:#x})", ProcessHandle, NumberOfPages, UserPfnArra));

        for(const auto& it : d.observers_NtAllocateUserPhysicalPages)
            it(ProcessHandle, NumberOfPages, UserPfnArra);
    }

    static void on_NtAllocateUuids(nt32::syscalls32::Data& d)
    {
        const auto Time     = arg<nt32::PULARGE_INTEGER>(d.core, 0);
        const auto Range    = arg<nt32::PULONG>(d.core, 1);
        const auto Sequence = arg<nt32::PULONG>(d.core, 2);
        const auto Seed     = arg<nt32::PCHAR>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAllocateUuids(Time:{:#x}, Range:{:#x}, Sequence:{:#x}, Seed:{:#x})", Time, Range, Sequence, Seed));

        for(const auto& it : d.observers_NtAllocateUuids)
            it(Time, Range, Sequence, Seed);
    }

    static void on_NtAllocateVirtualMemory(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(d.core, 1);
        const auto ZeroBits        = arg<nt32::ULONG_PTR>(d.core, 2);
        const auto RegionSize      = arg<nt32::PSIZE_T>(d.core, 3);
        const auto AllocationType  = arg<nt32::ULONG>(d.core, 4);
        const auto Protect         = arg<nt32::ULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAllocateVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, ZeroBits:{:#x}, RegionSize:{:#x}, AllocationType:{:#x}, Protect:{:#x})", ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect));

        for(const auto& it : d.observers_NtAllocateVirtualMemory)
            it(ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
    }

    static void on_NtAlpcAcceptConnectPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle                  = arg<nt32::PHANDLE>(d.core, 0);
        const auto ConnectionPortHandle        = arg<nt32::HANDLE>(d.core, 1);
        const auto Flags                       = arg<nt32::ULONG>(d.core, 2);
        const auto ObjectAttributes            = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 3);
        const auto PortAttributes              = arg<nt32::PALPC_PORT_ATTRIBUTES>(d.core, 4);
        const auto PortContext                 = arg<nt32::PVOID>(d.core, 5);
        const auto ConnectionRequest           = arg<nt32::PPORT_MESSAGE>(d.core, 6);
        const auto ConnectionMessageAttributes = arg<nt32::PALPC_MESSAGE_ATTRIBUTES>(d.core, 7);
        const auto AcceptConnection            = arg<nt32::BOOLEAN>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlpcAcceptConnectPort(PortHandle:{:#x}, ConnectionPortHandle:{:#x}, Flags:{:#x}, ObjectAttributes:{:#x}, PortAttributes:{:#x}, PortContext:{:#x}, ConnectionRequest:{:#x}, ConnectionMessageAttributes:{:#x}, AcceptConnection:{:#x})", PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection));

        for(const auto& it : d.observers_NtAlpcAcceptConnectPort)
            it(PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection);
    }

    static void on_ZwAlpcCancelMessage(nt32::syscalls32::Data& d)
    {
        const auto PortHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags          = arg<nt32::ULONG>(d.core, 1);
        const auto MessageContext = arg<nt32::PALPC_CONTEXT_ATTR>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcCancelMessage(PortHandle:{:#x}, Flags:{:#x}, MessageContext:{:#x})", PortHandle, Flags, MessageContext));

        for(const auto& it : d.observers_ZwAlpcCancelMessage)
            it(PortHandle, Flags, MessageContext);
    }

    static void on_ZwAlpcConnectPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle           = arg<nt32::PHANDLE>(d.core, 0);
        const auto PortName             = arg<nt32::PUNICODE_STRING>(d.core, 1);
        const auto ObjectAttributes     = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto PortAttributes       = arg<nt32::PALPC_PORT_ATTRIBUTES>(d.core, 3);
        const auto Flags                = arg<nt32::ULONG>(d.core, 4);
        const auto RequiredServerSid    = arg<nt32::PSID>(d.core, 5);
        const auto ConnectionMessage    = arg<nt32::PPORT_MESSAGE>(d.core, 6);
        const auto BufferLength         = arg<nt32::PULONG>(d.core, 7);
        const auto OutMessageAttributes = arg<nt32::PALPC_MESSAGE_ATTRIBUTES>(d.core, 8);
        const auto InMessageAttributes  = arg<nt32::PALPC_MESSAGE_ATTRIBUTES>(d.core, 9);
        const auto Timeout              = arg<nt32::PLARGE_INTEGER>(d.core, 10);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcConnectPort(PortHandle:{:#x}, PortName:{:#x}, ObjectAttributes:{:#x}, PortAttributes:{:#x}, Flags:{:#x}, RequiredServerSid:{:#x}, ConnectionMessage:{:#x}, BufferLength:{:#x}, OutMessageAttributes:{:#x}, InMessageAttributes:{:#x}, Timeout:{:#x})", PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout));

        for(const auto& it : d.observers_ZwAlpcConnectPort)
            it(PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout);
    }

    static void on_ZwAlpcCreatePort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle       = arg<nt32::PHANDLE>(d.core, 0);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto PortAttributes   = arg<nt32::PALPC_PORT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcCreatePort(PortHandle:{:#x}, ObjectAttributes:{:#x}, PortAttributes:{:#x})", PortHandle, ObjectAttributes, PortAttributes));

        for(const auto& it : d.observers_ZwAlpcCreatePort)
            it(PortHandle, ObjectAttributes, PortAttributes);
    }

    static void on_NtAlpcCreatePortSection(nt32::syscalls32::Data& d)
    {
        const auto PortHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags             = arg<nt32::ULONG>(d.core, 1);
        const auto SectionHandle     = arg<nt32::HANDLE>(d.core, 2);
        const auto SectionSize       = arg<nt32::SIZE_T>(d.core, 3);
        const auto AlpcSectionHandle = arg<nt32::PALPC_HANDLE>(d.core, 4);
        const auto ActualSectionSize = arg<nt32::PSIZE_T>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlpcCreatePortSection(PortHandle:{:#x}, Flags:{:#x}, SectionHandle:{:#x}, SectionSize:{:#x}, AlpcSectionHandle:{:#x}, ActualSectionSize:{:#x})", PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize));

        for(const auto& it : d.observers_NtAlpcCreatePortSection)
            it(PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize);
    }

    static void on_ZwAlpcCreateResourceReserve(nt32::syscalls32::Data& d)
    {
        const auto PortHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags       = arg<nt32::ULONG>(d.core, 1);
        const auto MessageSize = arg<nt32::SIZE_T>(d.core, 2);
        const auto ResourceId  = arg<nt32::PALPC_HANDLE>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcCreateResourceReserve(PortHandle:{:#x}, Flags:{:#x}, MessageSize:{:#x}, ResourceId:{:#x})", PortHandle, Flags, MessageSize, ResourceId));

        for(const auto& it : d.observers_ZwAlpcCreateResourceReserve)
            it(PortHandle, Flags, MessageSize, ResourceId);
    }

    static void on_ZwAlpcCreateSectionView(nt32::syscalls32::Data& d)
    {
        const auto PortHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags          = arg<nt32::ULONG>(d.core, 1);
        const auto ViewAttributes = arg<nt32::PALPC_DATA_VIEW_ATTR>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcCreateSectionView(PortHandle:{:#x}, Flags:{:#x}, ViewAttributes:{:#x})", PortHandle, Flags, ViewAttributes));

        for(const auto& it : d.observers_ZwAlpcCreateSectionView)
            it(PortHandle, Flags, ViewAttributes);
    }

    static void on_ZwAlpcCreateSecurityContext(nt32::syscalls32::Data& d)
    {
        const auto PortHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags             = arg<nt32::ULONG>(d.core, 1);
        const auto SecurityAttribute = arg<nt32::PALPC_SECURITY_ATTR>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcCreateSecurityContext(PortHandle:{:#x}, Flags:{:#x}, SecurityAttribute:{:#x})", PortHandle, Flags, SecurityAttribute));

        for(const auto& it : d.observers_ZwAlpcCreateSecurityContext)
            it(PortHandle, Flags, SecurityAttribute);
    }

    static void on_ZwAlpcDeletePortSection(nt32::syscalls32::Data& d)
    {
        const auto PortHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags         = arg<nt32::ULONG>(d.core, 1);
        const auto SectionHandle = arg<nt32::ALPC_HANDLE>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcDeletePortSection(PortHandle:{:#x}, Flags:{:#x}, SectionHandle:{:#x})", PortHandle, Flags, SectionHandle));

        for(const auto& it : d.observers_ZwAlpcDeletePortSection)
            it(PortHandle, Flags, SectionHandle);
    }

    static void on_NtAlpcDeleteResourceReserve(nt32::syscalls32::Data& d)
    {
        const auto PortHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags      = arg<nt32::ULONG>(d.core, 1);
        const auto ResourceId = arg<nt32::ALPC_HANDLE>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlpcDeleteResourceReserve(PortHandle:{:#x}, Flags:{:#x}, ResourceId:{:#x})", PortHandle, Flags, ResourceId));

        for(const auto& it : d.observers_NtAlpcDeleteResourceReserve)
            it(PortHandle, Flags, ResourceId);
    }

    static void on_NtAlpcDeleteSectionView(nt32::syscalls32::Data& d)
    {
        const auto PortHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags      = arg<nt32::ULONG>(d.core, 1);
        const auto ViewBase   = arg<nt32::PVOID>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlpcDeleteSectionView(PortHandle:{:#x}, Flags:{:#x}, ViewBase:{:#x})", PortHandle, Flags, ViewBase));

        for(const auto& it : d.observers_NtAlpcDeleteSectionView)
            it(PortHandle, Flags, ViewBase);
    }

    static void on_NtAlpcDeleteSecurityContext(nt32::syscalls32::Data& d)
    {
        const auto PortHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags         = arg<nt32::ULONG>(d.core, 1);
        const auto ContextHandle = arg<nt32::ALPC_HANDLE>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlpcDeleteSecurityContext(PortHandle:{:#x}, Flags:{:#x}, ContextHandle:{:#x})", PortHandle, Flags, ContextHandle));

        for(const auto& it : d.observers_NtAlpcDeleteSecurityContext)
            it(PortHandle, Flags, ContextHandle);
    }

    static void on_NtAlpcDisconnectPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags      = arg<nt32::ULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlpcDisconnectPort(PortHandle:{:#x}, Flags:{:#x})", PortHandle, Flags));

        for(const auto& it : d.observers_NtAlpcDisconnectPort)
            it(PortHandle, Flags);
    }

    static void on_ZwAlpcImpersonateClientOfPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto PortMessage = arg<nt32::PPORT_MESSAGE>(d.core, 1);
        const auto Reserved    = arg<nt32::PVOID>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcImpersonateClientOfPort(PortHandle:{:#x}, PortMessage:{:#x}, Reserved:{:#x})", PortHandle, PortMessage, Reserved));

        for(const auto& it : d.observers_ZwAlpcImpersonateClientOfPort)
            it(PortHandle, PortMessage, Reserved);
    }

    static void on_ZwAlpcOpenSenderProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle    = arg<nt32::PHANDLE>(d.core, 0);
        const auto PortHandle       = arg<nt32::HANDLE>(d.core, 1);
        const auto PortMessage      = arg<nt32::PPORT_MESSAGE>(d.core, 2);
        const auto Flags            = arg<nt32::ULONG>(d.core, 3);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 4);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcOpenSenderProcess(ProcessHandle:{:#x}, PortHandle:{:#x}, PortMessage:{:#x}, Flags:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_ZwAlpcOpenSenderProcess)
            it(ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    }

    static void on_ZwAlpcOpenSenderThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle     = arg<nt32::PHANDLE>(d.core, 0);
        const auto PortHandle       = arg<nt32::HANDLE>(d.core, 1);
        const auto PortMessage      = arg<nt32::PPORT_MESSAGE>(d.core, 2);
        const auto Flags            = arg<nt32::ULONG>(d.core, 3);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 4);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcOpenSenderThread(ThreadHandle:{:#x}, PortHandle:{:#x}, PortMessage:{:#x}, Flags:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_ZwAlpcOpenSenderThread)
            it(ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    }

    static void on_ZwAlpcQueryInformation(nt32::syscalls32::Data& d)
    {
        const auto PortHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto PortInformationClass = arg<nt32::ALPC_PORT_INFORMATION_CLASS>(d.core, 1);
        const auto PortInformation      = arg<nt32::PVOID>(d.core, 2);
        const auto Length               = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength         = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcQueryInformation(PortHandle:{:#x}, PortInformationClass:{:#x}, PortInformation:{:#x}, Length:{:#x}, ReturnLength:{:#x})", PortHandle, PortInformationClass, PortInformation, Length, ReturnLength));

        for(const auto& it : d.observers_ZwAlpcQueryInformation)
            it(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    }

    static void on_ZwAlpcQueryInformationMessage(nt32::syscalls32::Data& d)
    {
        const auto PortHandle              = arg<nt32::HANDLE>(d.core, 0);
        const auto PortMessage             = arg<nt32::PPORT_MESSAGE>(d.core, 1);
        const auto MessageInformationClass = arg<nt32::ALPC_MESSAGE_INFORMATION_CLASS>(d.core, 2);
        const auto MessageInformation      = arg<nt32::PVOID>(d.core, 3);
        const auto Length                  = arg<nt32::ULONG>(d.core, 4);
        const auto ReturnLength            = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAlpcQueryInformationMessage(PortHandle:{:#x}, PortMessage:{:#x}, MessageInformationClass:{:#x}, MessageInformation:{:#x}, Length:{:#x}, ReturnLength:{:#x})", PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength));

        for(const auto& it : d.observers_ZwAlpcQueryInformationMessage)
            it(PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength);
    }

    static void on_NtAlpcRevokeSecurityContext(nt32::syscalls32::Data& d)
    {
        const auto PortHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags         = arg<nt32::ULONG>(d.core, 1);
        const auto ContextHandle = arg<nt32::ALPC_HANDLE>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlpcRevokeSecurityContext(PortHandle:{:#x}, Flags:{:#x}, ContextHandle:{:#x})", PortHandle, Flags, ContextHandle));

        for(const auto& it : d.observers_NtAlpcRevokeSecurityContext)
            it(PortHandle, Flags, ContextHandle);
    }

    static void on_NtAlpcSendWaitReceivePort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle               = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags                    = arg<nt32::ULONG>(d.core, 1);
        const auto SendMessage              = arg<nt32::PPORT_MESSAGE>(d.core, 2);
        const auto SendMessageAttributes    = arg<nt32::PALPC_MESSAGE_ATTRIBUTES>(d.core, 3);
        const auto ReceiveMessage           = arg<nt32::PPORT_MESSAGE>(d.core, 4);
        const auto BufferLength             = arg<nt32::PULONG>(d.core, 5);
        const auto ReceiveMessageAttributes = arg<nt32::PALPC_MESSAGE_ATTRIBUTES>(d.core, 6);
        const auto Timeout                  = arg<nt32::PLARGE_INTEGER>(d.core, 7);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlpcSendWaitReceivePort(PortHandle:{:#x}, Flags:{:#x}, SendMessage:{:#x}, SendMessageAttributes:{:#x}, ReceiveMessage:{:#x}, BufferLength:{:#x}, ReceiveMessageAttributes:{:#x}, Timeout:{:#x})", PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout));

        for(const auto& it : d.observers_NtAlpcSendWaitReceivePort)
            it(PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout);
    }

    static void on_NtAlpcSetInformation(nt32::syscalls32::Data& d)
    {
        const auto PortHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto PortInformationClass = arg<nt32::ALPC_PORT_INFORMATION_CLASS>(d.core, 1);
        const auto PortInformation      = arg<nt32::PVOID>(d.core, 2);
        const auto Length               = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtAlpcSetInformation(PortHandle:{:#x}, PortInformationClass:{:#x}, PortInformation:{:#x}, Length:{:#x})", PortHandle, PortInformationClass, PortInformation, Length));

        for(const auto& it : d.observers_NtAlpcSetInformation)
            it(PortHandle, PortInformationClass, PortInformation, Length);
    }

    static void on_NtApphelpCacheControl(nt32::syscalls32::Data& d)
    {
        const auto type = arg<nt32::APPHELPCOMMAND>(d.core, 0);
        const auto buf  = arg<nt32::PVOID>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtApphelpCacheControl(type:{:#x}, buf:{:#x})", type, buf));

        for(const auto& it : d.observers_NtApphelpCacheControl)
            it(type, buf);
    }

    static void on_ZwAreMappedFilesTheSame(nt32::syscalls32::Data& d)
    {
        const auto File1MappedAsAnImage = arg<nt32::PVOID>(d.core, 0);
        const auto File2MappedAsFile    = arg<nt32::PVOID>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAreMappedFilesTheSame(File1MappedAsAnImage:{:#x}, File2MappedAsFile:{:#x})", File1MappedAsAnImage, File2MappedAsFile));

        for(const auto& it : d.observers_ZwAreMappedFilesTheSame)
            it(File1MappedAsAnImage, File2MappedAsFile);
    }

    static void on_ZwAssignProcessToJobObject(nt32::syscalls32::Data& d)
    {
        const auto JobHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwAssignProcessToJobObject(JobHandle:{:#x}, ProcessHandle:{:#x})", JobHandle, ProcessHandle));

        for(const auto& it : d.observers_ZwAssignProcessToJobObject)
            it(JobHandle, ProcessHandle);
    }

    static void on_NtCancelIoFileEx(nt32::syscalls32::Data& d)
    {
        const auto FileHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto IoRequestToCancel = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCancelIoFileEx(FileHandle:{:#x}, IoRequestToCancel:{:#x}, IoStatusBlock:{:#x})", FileHandle, IoRequestToCancel, IoStatusBlock));

        for(const auto& it : d.observers_NtCancelIoFileEx)
            it(FileHandle, IoRequestToCancel, IoStatusBlock);
    }

    static void on_ZwCancelIoFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCancelIoFile(FileHandle:{:#x}, IoStatusBlock:{:#x})", FileHandle, IoStatusBlock));

        for(const auto& it : d.observers_ZwCancelIoFile)
            it(FileHandle, IoStatusBlock);
    }

    static void on_NtCancelSynchronousIoFile(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle      = arg<nt32::HANDLE>(d.core, 0);
        const auto IoRequestToCancel = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCancelSynchronousIoFile(ThreadHandle:{:#x}, IoRequestToCancel:{:#x}, IoStatusBlock:{:#x})", ThreadHandle, IoRequestToCancel, IoStatusBlock));

        for(const auto& it : d.observers_NtCancelSynchronousIoFile)
            it(ThreadHandle, IoRequestToCancel, IoStatusBlock);
    }

    static void on_ZwCancelTimer(nt32::syscalls32::Data& d)
    {
        const auto TimerHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto CurrentState = arg<nt32::PBOOLEAN>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCancelTimer(TimerHandle:{:#x}, CurrentState:{:#x})", TimerHandle, CurrentState));

        for(const auto& it : d.observers_ZwCancelTimer)
            it(TimerHandle, CurrentState);
    }

    static void on_NtClearEvent(nt32::syscalls32::Data& d)
    {
        const auto EventHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtClearEvent(EventHandle:{:#x})", EventHandle));

        for(const auto& it : d.observers_NtClearEvent)
            it(EventHandle);
    }

    static void on_NtClose(nt32::syscalls32::Data& d)
    {
        const auto Handle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtClose(Handle:{:#x})", Handle));

        for(const auto& it : d.observers_NtClose)
            it(Handle);
    }

    static void on_ZwCloseObjectAuditAlarm(nt32::syscalls32::Data& d)
    {
        const auto SubsystemName   = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto HandleId        = arg<nt32::PVOID>(d.core, 1);
        const auto GenerateOnClose = arg<nt32::BOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCloseObjectAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, GenerateOnClose));

        for(const auto& it : d.observers_ZwCloseObjectAuditAlarm)
            it(SubsystemName, HandleId, GenerateOnClose);
    }

    static void on_ZwCommitComplete(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCommitComplete(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock));

        for(const auto& it : d.observers_ZwCommitComplete)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtCommitEnlistment(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCommitEnlistment(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock));

        for(const auto& it : d.observers_NtCommitEnlistment)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtCommitTransaction(nt32::syscalls32::Data& d)
    {
        const auto TransactionHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto Wait              = arg<nt32::BOOLEAN>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCommitTransaction(TransactionHandle:{:#x}, Wait:{:#x})", TransactionHandle, Wait));

        for(const auto& it : d.observers_NtCommitTransaction)
            it(TransactionHandle, Wait);
    }

    static void on_NtCompactKeys(nt32::syscalls32::Data& d)
    {
        const auto Count    = arg<nt32::ULONG>(d.core, 0);
        const auto KeyArray = arg<nt32::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCompactKeys(Count:{:#x}, KeyArray:{:#x})", Count, KeyArray));

        for(const auto& it : d.observers_NtCompactKeys)
            it(Count, KeyArray);
    }

    static void on_ZwCompareTokens(nt32::syscalls32::Data& d)
    {
        const auto FirstTokenHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto SecondTokenHandle = arg<nt32::HANDLE>(d.core, 1);
        const auto Equal             = arg<nt32::PBOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCompareTokens(FirstTokenHandle:{:#x}, SecondTokenHandle:{:#x}, Equal:{:#x})", FirstTokenHandle, SecondTokenHandle, Equal));

        for(const auto& it : d.observers_ZwCompareTokens)
            it(FirstTokenHandle, SecondTokenHandle, Equal);
    }

    static void on_NtCompleteConnectPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCompleteConnectPort(PortHandle:{:#x})", PortHandle));

        for(const auto& it : d.observers_NtCompleteConnectPort)
            it(PortHandle);
    }

    static void on_ZwCompressKey(nt32::syscalls32::Data& d)
    {
        const auto Key = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCompressKey(Key:{:#x})", Key));

        for(const auto& it : d.observers_ZwCompressKey)
            it(Key);
    }

    static void on_NtConnectPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle                  = arg<nt32::PHANDLE>(d.core, 0);
        const auto PortName                    = arg<nt32::PUNICODE_STRING>(d.core, 1);
        const auto SecurityQos                 = arg<nt32::PSECURITY_QUALITY_OF_SERVICE>(d.core, 2);
        const auto ClientView                  = arg<nt32::PPORT_VIEW>(d.core, 3);
        const auto ServerView                  = arg<nt32::PREMOTE_PORT_VIEW>(d.core, 4);
        const auto MaxMessageLength            = arg<nt32::PULONG>(d.core, 5);
        const auto ConnectionInformation       = arg<nt32::PVOID>(d.core, 6);
        const auto ConnectionInformationLength = arg<nt32::PULONG>(d.core, 7);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtConnectPort(PortHandle:{:#x}, PortName:{:#x}, SecurityQos:{:#x}, ClientView:{:#x}, ServerView:{:#x}, MaxMessageLength:{:#x}, ConnectionInformation:{:#x}, ConnectionInformationLength:{:#x})", PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength));

        for(const auto& it : d.observers_NtConnectPort)
            it(PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    }

    static void on_ZwContinue(nt32::syscalls32::Data& d)
    {
        const auto ContextRecord = arg<nt32::PCONTEXT>(d.core, 0);
        const auto TestAlert     = arg<nt32::BOOLEAN>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwContinue(ContextRecord:{:#x}, TestAlert:{:#x})", ContextRecord, TestAlert));

        for(const auto& it : d.observers_ZwContinue)
            it(ContextRecord, TestAlert);
    }

    static void on_ZwCreateDebugObject(nt32::syscalls32::Data& d)
    {
        const auto DebugObjectHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Flags             = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateDebugObject(DebugObjectHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, Flags:{:#x})", DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags));

        for(const auto& it : d.observers_ZwCreateDebugObject)
            it(DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags);
    }

    static void on_ZwCreateDirectoryObject(nt32::syscalls32::Data& d)
    {
        const auto DirectoryHandle  = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateDirectoryObject(DirectoryHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", DirectoryHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_ZwCreateDirectoryObject)
            it(DirectoryHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_ZwCreateEnlistment(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle      = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess         = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ResourceManagerHandle = arg<nt32::HANDLE>(d.core, 2);
        const auto TransactionHandle     = arg<nt32::HANDLE>(d.core, 3);
        const auto ObjectAttributes      = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 4);
        const auto CreateOptions         = arg<nt32::ULONG>(d.core, 5);
        const auto NotificationMask      = arg<nt32::NOTIFICATION_MASK>(d.core, 6);
        const auto EnlistmentKey         = arg<nt32::PVOID>(d.core, 7);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateEnlistment(EnlistmentHandle:{:#x}, DesiredAccess:{:#x}, ResourceManagerHandle:{:#x}, TransactionHandle:{:#x}, ObjectAttributes:{:#x}, CreateOptions:{:#x}, NotificationMask:{:#x}, EnlistmentKey:{:#x})", EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey));

        for(const auto& it : d.observers_ZwCreateEnlistment)
            it(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey);
    }

    static void on_NtCreateEvent(nt32::syscalls32::Data& d)
    {
        const auto EventHandle      = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto EventType        = arg<nt32::EVENT_TYPE>(d.core, 3);
        const auto InitialState     = arg<nt32::BOOLEAN>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateEvent(EventHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, EventType:{:#x}, InitialState:{:#x})", EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState));

        for(const auto& it : d.observers_NtCreateEvent)
            it(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
    }

    static void on_NtCreateEventPair(nt32::syscalls32::Data& d)
    {
        const auto EventPairHandle  = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateEventPair(EventPairHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", EventPairHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_NtCreateEventPair)
            it(EventPairHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtCreateFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle        = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(d.core, 3);
        const auto AllocationSize    = arg<nt32::PLARGE_INTEGER>(d.core, 4);
        const auto FileAttributes    = arg<nt32::ULONG>(d.core, 5);
        const auto ShareAccess       = arg<nt32::ULONG>(d.core, 6);
        const auto CreateDisposition = arg<nt32::ULONG>(d.core, 7);
        const auto CreateOptions     = arg<nt32::ULONG>(d.core, 8);
        const auto EaBuffer          = arg<nt32::PVOID>(d.core, 9);
        const auto EaLength          = arg<nt32::ULONG>(d.core, 10);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateFile(FileHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, IoStatusBlock:{:#x}, AllocationSize:{:#x}, FileAttributes:{:#x}, ShareAccess:{:#x}, CreateDisposition:{:#x}, CreateOptions:{:#x}, EaBuffer:{:#x}, EaLength:{:#x})", FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength));

        for(const auto& it : d.observers_NtCreateFile)
            it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    }

    static void on_NtCreateIoCompletion(nt32::syscalls32::Data& d)
    {
        const auto IoCompletionHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Count              = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateIoCompletion(IoCompletionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, Count:{:#x})", IoCompletionHandle, DesiredAccess, ObjectAttributes, Count));

        for(const auto& it : d.observers_NtCreateIoCompletion)
            it(IoCompletionHandle, DesiredAccess, ObjectAttributes, Count);
    }

    static void on_ZwCreateJobObject(nt32::syscalls32::Data& d)
    {
        const auto JobHandle        = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateJobObject(JobHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", JobHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_ZwCreateJobObject)
            it(JobHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtCreateJobSet(nt32::syscalls32::Data& d)
    {
        const auto NumJob     = arg<nt32::ULONG>(d.core, 0);
        const auto UserJobSet = arg<nt32::PJOB_SET_ARRAY>(d.core, 1);
        const auto Flags      = arg<nt32::ULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateJobSet(NumJob:{:#x}, UserJobSet:{:#x}, Flags:{:#x})", NumJob, UserJobSet, Flags));

        for(const auto& it : d.observers_NtCreateJobSet)
            it(NumJob, UserJobSet, Flags);
    }

    static void on_ZwCreateKeyedEvent(nt32::syscalls32::Data& d)
    {
        const auto KeyedEventHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Flags            = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateKeyedEvent(KeyedEventHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, Flags:{:#x})", KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags));

        for(const auto& it : d.observers_ZwCreateKeyedEvent)
            it(KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags);
    }

    static void on_ZwCreateKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle        = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto TitleIndex       = arg<nt32::ULONG>(d.core, 3);
        const auto Class            = arg<nt32::PUNICODE_STRING>(d.core, 4);
        const auto CreateOptions    = arg<nt32::ULONG>(d.core, 5);
        const auto Disposition      = arg<nt32::PULONG>(d.core, 6);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateKey(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, TitleIndex:{:#x}, Class:{:#x}, CreateOptions:{:#x}, Disposition:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition));

        for(const auto& it : d.observers_ZwCreateKey)
            it(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
    }

    static void on_NtCreateKeyTransacted(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle         = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto TitleIndex        = arg<nt32::ULONG>(d.core, 3);
        const auto Class             = arg<nt32::PUNICODE_STRING>(d.core, 4);
        const auto CreateOptions     = arg<nt32::ULONG>(d.core, 5);
        const auto TransactionHandle = arg<nt32::HANDLE>(d.core, 6);
        const auto Disposition       = arg<nt32::PULONG>(d.core, 7);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateKeyTransacted(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, TitleIndex:{:#x}, Class:{:#x}, CreateOptions:{:#x}, TransactionHandle:{:#x}, Disposition:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition));

        for(const auto& it : d.observers_NtCreateKeyTransacted)
            it(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);
    }

    static void on_ZwCreateMailslotFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle         = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt32::ULONG>(d.core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(d.core, 3);
        const auto CreateOptions      = arg<nt32::ULONG>(d.core, 4);
        const auto MailslotQuota      = arg<nt32::ULONG>(d.core, 5);
        const auto MaximumMessageSize = arg<nt32::ULONG>(d.core, 6);
        const auto ReadTimeout        = arg<nt32::PLARGE_INTEGER>(d.core, 7);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateMailslotFile(FileHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, IoStatusBlock:{:#x}, CreateOptions:{:#x}, MailslotQuota:{:#x}, MaximumMessageSize:{:#x}, ReadTimeout:{:#x})", FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout));

        for(const auto& it : d.observers_ZwCreateMailslotFile)
            it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout);
    }

    static void on_ZwCreateMutant(nt32::syscalls32::Data& d)
    {
        const auto MutantHandle     = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto InitialOwner     = arg<nt32::BOOLEAN>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateMutant(MutantHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, InitialOwner:{:#x})", MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner));

        for(const auto& it : d.observers_ZwCreateMutant)
            it(MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);
    }

    static void on_ZwCreateNamedPipeFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle        = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt32::ULONG>(d.core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(d.core, 3);
        const auto ShareAccess       = arg<nt32::ULONG>(d.core, 4);
        const auto CreateDisposition = arg<nt32::ULONG>(d.core, 5);
        const auto CreateOptions     = arg<nt32::ULONG>(d.core, 6);
        const auto NamedPipeType     = arg<nt32::ULONG>(d.core, 7);
        const auto ReadMode          = arg<nt32::ULONG>(d.core, 8);
        const auto CompletionMode    = arg<nt32::ULONG>(d.core, 9);
        const auto MaximumInstances  = arg<nt32::ULONG>(d.core, 10);
        const auto InboundQuota      = arg<nt32::ULONG>(d.core, 11);
        const auto OutboundQuota     = arg<nt32::ULONG>(d.core, 12);
        const auto DefaultTimeout    = arg<nt32::PLARGE_INTEGER>(d.core, 13);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateNamedPipeFile(FileHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, IoStatusBlock:{:#x}, ShareAccess:{:#x}, CreateDisposition:{:#x}, CreateOptions:{:#x}, NamedPipeType:{:#x}, ReadMode:{:#x}, CompletionMode:{:#x}, MaximumInstances:{:#x}, InboundQuota:{:#x}, OutboundQuota:{:#x}, DefaultTimeout:{:#x})", FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout));

        for(const auto& it : d.observers_ZwCreateNamedPipeFile)
            it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);
    }

    static void on_NtCreatePagingFile(nt32::syscalls32::Data& d)
    {
        const auto PageFileName = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto MinimumSize  = arg<nt32::PLARGE_INTEGER>(d.core, 1);
        const auto MaximumSize  = arg<nt32::PLARGE_INTEGER>(d.core, 2);
        const auto Priority     = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreatePagingFile(PageFileName:{:#x}, MinimumSize:{:#x}, MaximumSize:{:#x}, Priority:{:#x})", PageFileName, MinimumSize, MaximumSize, Priority));

        for(const auto& it : d.observers_NtCreatePagingFile)
            it(PageFileName, MinimumSize, MaximumSize, Priority);
    }

    static void on_ZwCreatePort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle              = arg<nt32::PHANDLE>(d.core, 0);
        const auto ObjectAttributes        = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto MaxConnectionInfoLength = arg<nt32::ULONG>(d.core, 2);
        const auto MaxMessageLength        = arg<nt32::ULONG>(d.core, 3);
        const auto MaxPoolUsage            = arg<nt32::ULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreatePort(PortHandle:{:#x}, ObjectAttributes:{:#x}, MaxConnectionInfoLength:{:#x}, MaxMessageLength:{:#x}, MaxPoolUsage:{:#x})", PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage));

        for(const auto& it : d.observers_ZwCreatePort)
            it(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    }

    static void on_NtCreatePrivateNamespace(nt32::syscalls32::Data& d)
    {
        const auto NamespaceHandle    = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto BoundaryDescriptor = arg<nt32::PVOID>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreatePrivateNamespace(NamespaceHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, BoundaryDescriptor:{:#x})", NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor));

        for(const auto& it : d.observers_NtCreatePrivateNamespace)
            it(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    }

    static void on_ZwCreateProcessEx(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle    = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ParentProcess    = arg<nt32::HANDLE>(d.core, 3);
        const auto Flags            = arg<nt32::ULONG>(d.core, 4);
        const auto SectionHandle    = arg<nt32::HANDLE>(d.core, 5);
        const auto DebugPort        = arg<nt32::HANDLE>(d.core, 6);
        const auto ExceptionPort    = arg<nt32::HANDLE>(d.core, 7);
        const auto JobMemberLevel   = arg<nt32::ULONG>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateProcessEx(ProcessHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ParentProcess:{:#x}, Flags:{:#x}, SectionHandle:{:#x}, DebugPort:{:#x}, ExceptionPort:{:#x}, JobMemberLevel:{:#x})", ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel));

        for(const auto& it : d.observers_ZwCreateProcessEx)
            it(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);
    }

    static void on_ZwCreateProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle      = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ParentProcess      = arg<nt32::HANDLE>(d.core, 3);
        const auto InheritObjectTable = arg<nt32::BOOLEAN>(d.core, 4);
        const auto SectionHandle      = arg<nt32::HANDLE>(d.core, 5);
        const auto DebugPort          = arg<nt32::HANDLE>(d.core, 6);
        const auto ExceptionPort      = arg<nt32::HANDLE>(d.core, 7);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateProcess(ProcessHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ParentProcess:{:#x}, InheritObjectTable:{:#x}, SectionHandle:{:#x}, DebugPort:{:#x}, ExceptionPort:{:#x})", ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort));

        for(const auto& it : d.observers_ZwCreateProcess)
            it(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort);
    }

    static void on_NtCreateProfileEx(nt32::syscalls32::Data& d)
    {
        const auto ProfileHandle      = arg<nt32::PHANDLE>(d.core, 0);
        const auto Process            = arg<nt32::HANDLE>(d.core, 1);
        const auto ProfileBase        = arg<nt32::PVOID>(d.core, 2);
        const auto ProfileSize        = arg<nt32::SIZE_T>(d.core, 3);
        const auto BucketSize         = arg<nt32::ULONG>(d.core, 4);
        const auto Buffer             = arg<nt32::PULONG>(d.core, 5);
        const auto BufferSize         = arg<nt32::ULONG>(d.core, 6);
        const auto ProfileSource      = arg<nt32::KPROFILE_SOURCE>(d.core, 7);
        const auto GroupAffinityCount = arg<nt32::ULONG>(d.core, 8);
        const auto GroupAffinity      = arg<nt32::PGROUP_AFFINITY>(d.core, 9);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateProfileEx(ProfileHandle:{:#x}, Process:{:#x}, ProfileBase:{:#x}, ProfileSize:{:#x}, BucketSize:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, ProfileSource:{:#x}, GroupAffinityCount:{:#x}, GroupAffinity:{:#x})", ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity));

        for(const auto& it : d.observers_NtCreateProfileEx)
            it(ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity);
    }

    static void on_ZwCreateProfile(nt32::syscalls32::Data& d)
    {
        const auto ProfileHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto Process       = arg<nt32::HANDLE>(d.core, 1);
        const auto RangeBase     = arg<nt32::PVOID>(d.core, 2);
        const auto RangeSize     = arg<nt32::SIZE_T>(d.core, 3);
        const auto BucketSize    = arg<nt32::ULONG>(d.core, 4);
        const auto Buffer        = arg<nt32::PULONG>(d.core, 5);
        const auto BufferSize    = arg<nt32::ULONG>(d.core, 6);
        const auto ProfileSource = arg<nt32::KPROFILE_SOURCE>(d.core, 7);
        const auto Affinity      = arg<nt32::KAFFINITY>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateProfile(ProfileHandle:{:#x}, Process:{:#x}, RangeBase:{:#x}, RangeSize:{:#x}, BucketSize:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, ProfileSource:{:#x}, Affinity:{:#x})", ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity));

        for(const auto& it : d.observers_ZwCreateProfile)
            it(ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity);
    }

    static void on_ZwCreateResourceManager(nt32::syscalls32::Data& d)
    {
        const auto ResourceManagerHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess         = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto TmHandle              = arg<nt32::HANDLE>(d.core, 2);
        const auto RmGuid                = arg<nt32::LPGUID>(d.core, 3);
        const auto ObjectAttributes      = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 4);
        const auto CreateOptions         = arg<nt32::ULONG>(d.core, 5);
        const auto Description           = arg<nt32::PUNICODE_STRING>(d.core, 6);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateResourceManager(ResourceManagerHandle:{:#x}, DesiredAccess:{:#x}, TmHandle:{:#x}, RmGuid:{:#x}, ObjectAttributes:{:#x}, CreateOptions:{:#x}, Description:{:#x})", ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description));

        for(const auto& it : d.observers_ZwCreateResourceManager)
            it(ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description);
    }

    static void on_NtCreateSection(nt32::syscalls32::Data& d)
    {
        const auto SectionHandle         = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess         = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes      = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto MaximumSize           = arg<nt32::PLARGE_INTEGER>(d.core, 3);
        const auto SectionPageProtection = arg<nt32::ULONG>(d.core, 4);
        const auto AllocationAttributes  = arg<nt32::ULONG>(d.core, 5);
        const auto FileHandle            = arg<nt32::HANDLE>(d.core, 6);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateSection(SectionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, MaximumSize:{:#x}, SectionPageProtection:{:#x}, AllocationAttributes:{:#x}, FileHandle:{:#x})", SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle));

        for(const auto& it : d.observers_NtCreateSection)
            it(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
    }

    static void on_NtCreateSemaphore(nt32::syscalls32::Data& d)
    {
        const auto SemaphoreHandle  = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto InitialCount     = arg<nt32::LONG>(d.core, 3);
        const auto MaximumCount     = arg<nt32::LONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateSemaphore(SemaphoreHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, InitialCount:{:#x}, MaximumCount:{:#x})", SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount));

        for(const auto& it : d.observers_NtCreateSemaphore)
            it(SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
    }

    static void on_ZwCreateSymbolicLinkObject(nt32::syscalls32::Data& d)
    {
        const auto LinkHandle       = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto LinkTarget       = arg<nt32::PUNICODE_STRING>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateSymbolicLinkObject(LinkHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, LinkTarget:{:#x})", LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget));

        for(const auto& it : d.observers_ZwCreateSymbolicLinkObject)
            it(LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget);
    }

    static void on_NtCreateThreadEx(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle     = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ProcessHandle    = arg<nt32::HANDLE>(d.core, 3);
        const auto StartRoutine     = arg<nt32::PVOID>(d.core, 4);
        const auto Argument         = arg<nt32::PVOID>(d.core, 5);
        const auto CreateFlags      = arg<nt32::ULONG>(d.core, 6);
        const auto ZeroBits         = arg<nt32::ULONG_PTR>(d.core, 7);
        const auto StackSize        = arg<nt32::SIZE_T>(d.core, 8);
        const auto MaximumStackSize = arg<nt32::SIZE_T>(d.core, 9);
        const auto AttributeList    = arg<nt32::PPS_ATTRIBUTE_LIST>(d.core, 10);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateThreadEx(ThreadHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ProcessHandle:{:#x}, StartRoutine:{:#x}, Argument:{:#x}, CreateFlags:{:#x}, ZeroBits:{:#x}, StackSize:{:#x}, MaximumStackSize:{:#x}, AttributeList:{:#x})", ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList));

        for(const auto& it : d.observers_NtCreateThreadEx)
            it(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
    }

    static void on_NtCreateThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle     = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ProcessHandle    = arg<nt32::HANDLE>(d.core, 3);
        const auto ClientId         = arg<nt32::PCLIENT_ID>(d.core, 4);
        const auto ThreadContext    = arg<nt32::PCONTEXT>(d.core, 5);
        const auto InitialTeb       = arg<nt32::PINITIAL_TEB>(d.core, 6);
        const auto CreateSuspended  = arg<nt32::BOOLEAN>(d.core, 7);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateThread(ThreadHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ProcessHandle:{:#x}, ClientId:{:#x}, ThreadContext:{:#x}, InitialTeb:{:#x}, CreateSuspended:{:#x})", ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended));

        for(const auto& it : d.observers_NtCreateThread)
            it(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);
    }

    static void on_ZwCreateTimer(nt32::syscalls32::Data& d)
    {
        const auto TimerHandle      = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto TimerType        = arg<nt32::TIMER_TYPE>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateTimer(TimerHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, TimerType:{:#x})", TimerHandle, DesiredAccess, ObjectAttributes, TimerType));

        for(const auto& it : d.observers_ZwCreateTimer)
            it(TimerHandle, DesiredAccess, ObjectAttributes, TimerType);
    }

    static void on_NtCreateToken(nt32::syscalls32::Data& d)
    {
        const auto TokenHandle      = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto TokenType        = arg<nt32::TOKEN_TYPE>(d.core, 3);
        const auto AuthenticationId = arg<nt32::PLUID>(d.core, 4);
        const auto ExpirationTime   = arg<nt32::PLARGE_INTEGER>(d.core, 5);
        const auto User             = arg<nt32::PTOKEN_USER>(d.core, 6);
        const auto Groups           = arg<nt32::PTOKEN_GROUPS>(d.core, 7);
        const auto Privileges       = arg<nt32::PTOKEN_PRIVILEGES>(d.core, 8);
        const auto Owner            = arg<nt32::PTOKEN_OWNER>(d.core, 9);
        const auto PrimaryGroup     = arg<nt32::PTOKEN_PRIMARY_GROUP>(d.core, 10);
        const auto DefaultDacl      = arg<nt32::PTOKEN_DEFAULT_DACL>(d.core, 11);
        const auto TokenSource      = arg<nt32::PTOKEN_SOURCE>(d.core, 12);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateToken(TokenHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, TokenType:{:#x}, AuthenticationId:{:#x}, ExpirationTime:{:#x}, User:{:#x}, Groups:{:#x}, Privileges:{:#x}, Owner:{:#x}, PrimaryGroup:{:#x}, DefaultDacl:{:#x}, TokenSource:{:#x})", TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource));

        for(const auto& it : d.observers_NtCreateToken)
            it(TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource);
    }

    static void on_ZwCreateTransactionManager(nt32::syscalls32::Data& d)
    {
        const auto TmHandle         = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto LogFileName      = arg<nt32::PUNICODE_STRING>(d.core, 3);
        const auto CreateOptions    = arg<nt32::ULONG>(d.core, 4);
        const auto CommitStrength   = arg<nt32::ULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateTransactionManager(TmHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, LogFileName:{:#x}, CreateOptions:{:#x}, CommitStrength:{:#x})", TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength));

        for(const auto& it : d.observers_ZwCreateTransactionManager)
            it(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength);
    }

    static void on_NtCreateTransaction(nt32::syscalls32::Data& d)
    {
        const auto TransactionHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Uow               = arg<nt32::LPGUID>(d.core, 3);
        const auto TmHandle          = arg<nt32::HANDLE>(d.core, 4);
        const auto CreateOptions     = arg<nt32::ULONG>(d.core, 5);
        const auto IsolationLevel    = arg<nt32::ULONG>(d.core, 6);
        const auto IsolationFlags    = arg<nt32::ULONG>(d.core, 7);
        const auto Timeout           = arg<nt32::PLARGE_INTEGER>(d.core, 8);
        const auto Description       = arg<nt32::PUNICODE_STRING>(d.core, 9);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateTransaction(TransactionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, Uow:{:#x}, TmHandle:{:#x}, CreateOptions:{:#x}, IsolationLevel:{:#x}, IsolationFlags:{:#x}, Timeout:{:#x}, Description:{:#x})", TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description));

        for(const auto& it : d.observers_NtCreateTransaction)
            it(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);
    }

    static void on_NtCreateUserProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle           = arg<nt32::PHANDLE>(d.core, 0);
        const auto ThreadHandle            = arg<nt32::PHANDLE>(d.core, 1);
        const auto ProcessDesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 2);
        const auto ThreadDesiredAccess     = arg<nt32::ACCESS_MASK>(d.core, 3);
        const auto ProcessObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 4);
        const auto ThreadObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 5);
        const auto ProcessFlags            = arg<nt32::ULONG>(d.core, 6);
        const auto ThreadFlags             = arg<nt32::ULONG>(d.core, 7);
        const auto ProcessParameters       = arg<nt32::PRTL_USER_PROCESS_PARAMETERS>(d.core, 8);
        const auto CreateInfo              = arg<nt32::PPROCESS_CREATE_INFO>(d.core, 9);
        const auto AttributeList           = arg<nt32::PPROCESS_ATTRIBUTE_LIST>(d.core, 10);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateUserProcess(ProcessHandle:{:#x}, ThreadHandle:{:#x}, ProcessDesiredAccess:{:#x}, ThreadDesiredAccess:{:#x}, ProcessObjectAttributes:{:#x}, ThreadObjectAttributes:{:#x}, ProcessFlags:{:#x}, ThreadFlags:{:#x}, ProcessParameters:{:#x}, CreateInfo:{:#x}, AttributeList:{:#x})", ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList));

        for(const auto& it : d.observers_NtCreateUserProcess)
            it(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
    }

    static void on_ZwCreateWaitablePort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle              = arg<nt32::PHANDLE>(d.core, 0);
        const auto ObjectAttributes        = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto MaxConnectionInfoLength = arg<nt32::ULONG>(d.core, 2);
        const auto MaxMessageLength        = arg<nt32::ULONG>(d.core, 3);
        const auto MaxPoolUsage            = arg<nt32::ULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwCreateWaitablePort(PortHandle:{:#x}, ObjectAttributes:{:#x}, MaxConnectionInfoLength:{:#x}, MaxMessageLength:{:#x}, MaxPoolUsage:{:#x})", PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage));

        for(const auto& it : d.observers_ZwCreateWaitablePort)
            it(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    }

    static void on_NtCreateWorkerFactory(nt32::syscalls32::Data& d)
    {
        const auto WorkerFactoryHandleReturn = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess             = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes          = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto CompletionPortHandle      = arg<nt32::HANDLE>(d.core, 3);
        const auto WorkerProcessHandle       = arg<nt32::HANDLE>(d.core, 4);
        const auto StartRoutine              = arg<nt32::PVOID>(d.core, 5);
        const auto StartParameter            = arg<nt32::PVOID>(d.core, 6);
        const auto MaxThreadCount            = arg<nt32::ULONG>(d.core, 7);
        const auto StackReserve              = arg<nt32::SIZE_T>(d.core, 8);
        const auto StackCommit               = arg<nt32::SIZE_T>(d.core, 9);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtCreateWorkerFactory(WorkerFactoryHandleReturn:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, CompletionPortHandle:{:#x}, WorkerProcessHandle:{:#x}, StartRoutine:{:#x}, StartParameter:{:#x}, MaxThreadCount:{:#x}, StackReserve:{:#x}, StackCommit:{:#x})", WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit));

        for(const auto& it : d.observers_NtCreateWorkerFactory)
            it(WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit);
    }

    static void on_NtDebugActiveProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto DebugObjectHandle = arg<nt32::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtDebugActiveProcess(ProcessHandle:{:#x}, DebugObjectHandle:{:#x})", ProcessHandle, DebugObjectHandle));

        for(const auto& it : d.observers_NtDebugActiveProcess)
            it(ProcessHandle, DebugObjectHandle);
    }

    static void on_ZwDebugContinue(nt32::syscalls32::Data& d)
    {
        const auto DebugObjectHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto ClientId          = arg<nt32::PCLIENT_ID>(d.core, 1);
        const auto ContinueStatus    = arg<nt32::NTSTATUS>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwDebugContinue(DebugObjectHandle:{:#x}, ClientId:{:#x}, ContinueStatus:{:#x})", DebugObjectHandle, ClientId, ContinueStatus));

        for(const auto& it : d.observers_ZwDebugContinue)
            it(DebugObjectHandle, ClientId, ContinueStatus);
    }

    static void on_ZwDelayExecution(nt32::syscalls32::Data& d)
    {
        const auto Alertable     = arg<nt32::BOOLEAN>(d.core, 0);
        const auto DelayInterval = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwDelayExecution(Alertable:{:#x}, DelayInterval:{:#x})", Alertable, DelayInterval));

        for(const auto& it : d.observers_ZwDelayExecution)
            it(Alertable, DelayInterval);
    }

    static void on_ZwDeleteAtom(nt32::syscalls32::Data& d)
    {
        const auto Atom = arg<nt32::RTL_ATOM>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwDeleteAtom(Atom:{:#x})", Atom));

        for(const auto& it : d.observers_ZwDeleteAtom)
            it(Atom);
    }

    static void on_NtDeleteBootEntry(nt32::syscalls32::Data& d)
    {
        const auto Id = arg<nt32::ULONG>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtDeleteBootEntry(Id:{:#x})", Id));

        for(const auto& it : d.observers_NtDeleteBootEntry)
            it(Id);
    }

    static void on_ZwDeleteDriverEntry(nt32::syscalls32::Data& d)
    {
        const auto Id = arg<nt32::ULONG>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwDeleteDriverEntry(Id:{:#x})", Id));

        for(const auto& it : d.observers_ZwDeleteDriverEntry)
            it(Id);
    }

    static void on_NtDeleteFile(nt32::syscalls32::Data& d)
    {
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtDeleteFile(ObjectAttributes:{:#x})", ObjectAttributes));

        for(const auto& it : d.observers_NtDeleteFile)
            it(ObjectAttributes);
    }

    static void on_ZwDeleteKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwDeleteKey(KeyHandle:{:#x})", KeyHandle));

        for(const auto& it : d.observers_ZwDeleteKey)
            it(KeyHandle);
    }

    static void on_NtDeleteObjectAuditAlarm(nt32::syscalls32::Data& d)
    {
        const auto SubsystemName   = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto HandleId        = arg<nt32::PVOID>(d.core, 1);
        const auto GenerateOnClose = arg<nt32::BOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtDeleteObjectAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, GenerateOnClose));

        for(const auto& it : d.observers_NtDeleteObjectAuditAlarm)
            it(SubsystemName, HandleId, GenerateOnClose);
    }

    static void on_NtDeletePrivateNamespace(nt32::syscalls32::Data& d)
    {
        const auto NamespaceHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtDeletePrivateNamespace(NamespaceHandle:{:#x})", NamespaceHandle));

        for(const auto& it : d.observers_NtDeletePrivateNamespace)
            it(NamespaceHandle);
    }

    static void on_NtDeleteValueKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto ValueName = arg<nt32::PUNICODE_STRING>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtDeleteValueKey(KeyHandle:{:#x}, ValueName:{:#x})", KeyHandle, ValueName));

        for(const auto& it : d.observers_NtDeleteValueKey)
            it(KeyHandle, ValueName);
    }

    static void on_ZwDeviceIoControlFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle         = arg<nt32::HANDLE>(d.core, 0);
        const auto Event              = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine         = arg<nt32::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext         = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(d.core, 4);
        const auto IoControlCode      = arg<nt32::ULONG>(d.core, 5);
        const auto InputBuffer        = arg<nt32::PVOID>(d.core, 6);
        const auto InputBufferLength  = arg<nt32::ULONG>(d.core, 7);
        const auto OutputBuffer       = arg<nt32::PVOID>(d.core, 8);
        const auto OutputBufferLength = arg<nt32::ULONG>(d.core, 9);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwDeviceIoControlFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, IoControlCode:{:#x}, InputBuffer:{:#x}, InputBufferLength:{:#x}, OutputBuffer:{:#x}, OutputBufferLength:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength));

        for(const auto& it : d.observers_ZwDeviceIoControlFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    }

    static void on_NtDisplayString(nt32::syscalls32::Data& d)
    {
        const auto String = arg<nt32::PUNICODE_STRING>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtDisplayString(String:{:#x})", String));

        for(const auto& it : d.observers_NtDisplayString)
            it(String);
    }

    static void on_ZwDrawText(nt32::syscalls32::Data& d)
    {
        const auto Text = arg<nt32::PUNICODE_STRING>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwDrawText(Text:{:#x})", Text));

        for(const auto& it : d.observers_ZwDrawText)
            it(Text);
    }

    static void on_ZwDuplicateObject(nt32::syscalls32::Data& d)
    {
        const auto SourceProcessHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto SourceHandle        = arg<nt32::HANDLE>(d.core, 1);
        const auto TargetProcessHandle = arg<nt32::HANDLE>(d.core, 2);
        const auto TargetHandle        = arg<nt32::PHANDLE>(d.core, 3);
        const auto DesiredAccess       = arg<nt32::ACCESS_MASK>(d.core, 4);
        const auto HandleAttributes    = arg<nt32::ULONG>(d.core, 5);
        const auto Options             = arg<nt32::ULONG>(d.core, 6);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwDuplicateObject(SourceProcessHandle:{:#x}, SourceHandle:{:#x}, TargetProcessHandle:{:#x}, TargetHandle:{:#x}, DesiredAccess:{:#x}, HandleAttributes:{:#x}, Options:{:#x})", SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options));

        for(const auto& it : d.observers_ZwDuplicateObject)
            it(SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);
    }

    static void on_NtDuplicateToken(nt32::syscalls32::Data& d)
    {
        const auto ExistingTokenHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto DesiredAccess       = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes    = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto EffectiveOnly       = arg<nt32::BOOLEAN>(d.core, 3);
        const auto TokenType           = arg<nt32::TOKEN_TYPE>(d.core, 4);
        const auto NewTokenHandle      = arg<nt32::PHANDLE>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtDuplicateToken(ExistingTokenHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, EffectiveOnly:{:#x}, TokenType:{:#x}, NewTokenHandle:{:#x})", ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle));

        for(const auto& it : d.observers_NtDuplicateToken)
            it(ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle);
    }

    static void on_ZwEnumerateBootEntries(nt32::syscalls32::Data& d)
    {
        const auto Buffer       = arg<nt32::PVOID>(d.core, 0);
        const auto BufferLength = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwEnumerateBootEntries(Buffer:{:#x}, BufferLength:{:#x})", Buffer, BufferLength));

        for(const auto& it : d.observers_ZwEnumerateBootEntries)
            it(Buffer, BufferLength);
    }

    static void on_NtEnumerateDriverEntries(nt32::syscalls32::Data& d)
    {
        const auto Buffer       = arg<nt32::PVOID>(d.core, 0);
        const auto BufferLength = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtEnumerateDriverEntries(Buffer:{:#x}, BufferLength:{:#x})", Buffer, BufferLength));

        for(const auto& it : d.observers_NtEnumerateDriverEntries)
            it(Buffer, BufferLength);
    }

    static void on_ZwEnumerateKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto Index               = arg<nt32::ULONG>(d.core, 1);
        const auto KeyInformationClass = arg<nt32::KEY_INFORMATION_CLASS>(d.core, 2);
        const auto KeyInformation      = arg<nt32::PVOID>(d.core, 3);
        const auto Length              = arg<nt32::ULONG>(d.core, 4);
        const auto ResultLength        = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwEnumerateKey(KeyHandle:{:#x}, Index:{:#x}, KeyInformationClass:{:#x}, KeyInformation:{:#x}, Length:{:#x}, ResultLength:{:#x})", KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength));

        for(const auto& it : d.observers_ZwEnumerateKey)
            it(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
    }

    static void on_ZwEnumerateSystemEnvironmentValuesEx(nt32::syscalls32::Data& d)
    {
        const auto InformationClass = arg<nt32::ULONG>(d.core, 0);
        const auto Buffer           = arg<nt32::PVOID>(d.core, 1);
        const auto BufferLength     = arg<nt32::PULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwEnumerateSystemEnvironmentValuesEx(InformationClass:{:#x}, Buffer:{:#x}, BufferLength:{:#x})", InformationClass, Buffer, BufferLength));

        for(const auto& it : d.observers_ZwEnumerateSystemEnvironmentValuesEx)
            it(InformationClass, Buffer, BufferLength);
    }

    static void on_ZwEnumerateTransactionObject(nt32::syscalls32::Data& d)
    {
        const auto RootObjectHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto QueryType          = arg<nt32::KTMOBJECT_TYPE>(d.core, 1);
        const auto ObjectCursor       = arg<nt32::PKTMOBJECT_CURSOR>(d.core, 2);
        const auto ObjectCursorLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength       = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwEnumerateTransactionObject(RootObjectHandle:{:#x}, QueryType:{:#x}, ObjectCursor:{:#x}, ObjectCursorLength:{:#x}, ReturnLength:{:#x})", RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength));

        for(const auto& it : d.observers_ZwEnumerateTransactionObject)
            it(RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength);
    }

    static void on_NtEnumerateValueKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle                = arg<nt32::HANDLE>(d.core, 0);
        const auto Index                    = arg<nt32::ULONG>(d.core, 1);
        const auto KeyValueInformationClass = arg<nt32::KEY_VALUE_INFORMATION_CLASS>(d.core, 2);
        const auto KeyValueInformation      = arg<nt32::PVOID>(d.core, 3);
        const auto Length                   = arg<nt32::ULONG>(d.core, 4);
        const auto ResultLength             = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtEnumerateValueKey(KeyHandle:{:#x}, Index:{:#x}, KeyValueInformationClass:{:#x}, KeyValueInformation:{:#x}, Length:{:#x}, ResultLength:{:#x})", KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength));

        for(const auto& it : d.observers_NtEnumerateValueKey)
            it(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    }

    static void on_ZwExtendSection(nt32::syscalls32::Data& d)
    {
        const auto SectionHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto NewSectionSize = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwExtendSection(SectionHandle:{:#x}, NewSectionSize:{:#x})", SectionHandle, NewSectionSize));

        for(const auto& it : d.observers_ZwExtendSection)
            it(SectionHandle, NewSectionSize);
    }

    static void on_NtFilterToken(nt32::syscalls32::Data& d)
    {
        const auto ExistingTokenHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags               = arg<nt32::ULONG>(d.core, 1);
        const auto SidsToDisable       = arg<nt32::PTOKEN_GROUPS>(d.core, 2);
        const auto PrivilegesToDelete  = arg<nt32::PTOKEN_PRIVILEGES>(d.core, 3);
        const auto RestrictedSids      = arg<nt32::PTOKEN_GROUPS>(d.core, 4);
        const auto NewTokenHandle      = arg<nt32::PHANDLE>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtFilterToken(ExistingTokenHandle:{:#x}, Flags:{:#x}, SidsToDisable:{:#x}, PrivilegesToDelete:{:#x}, RestrictedSids:{:#x}, NewTokenHandle:{:#x})", ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle));

        for(const auto& it : d.observers_NtFilterToken)
            it(ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle);
    }

    static void on_NtFindAtom(nt32::syscalls32::Data& d)
    {
        const auto AtomName = arg<nt32::PWSTR>(d.core, 0);
        const auto Length   = arg<nt32::ULONG>(d.core, 1);
        const auto Atom     = arg<nt32::PRTL_ATOM>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtFindAtom(AtomName:{:#x}, Length:{:#x}, Atom:{:#x})", AtomName, Length, Atom));

        for(const auto& it : d.observers_NtFindAtom)
            it(AtomName, Length, Atom);
    }

    static void on_ZwFlushBuffersFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwFlushBuffersFile(FileHandle:{:#x}, IoStatusBlock:{:#x})", FileHandle, IoStatusBlock));

        for(const auto& it : d.observers_ZwFlushBuffersFile)
            it(FileHandle, IoStatusBlock);
    }

    static void on_ZwFlushInstallUILanguage(nt32::syscalls32::Data& d)
    {
        const auto InstallUILanguage = arg<nt32::LANGID>(d.core, 0);
        const auto SetComittedFlag   = arg<nt32::ULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwFlushInstallUILanguage(InstallUILanguage:{:#x}, SetComittedFlag:{:#x})", InstallUILanguage, SetComittedFlag));

        for(const auto& it : d.observers_ZwFlushInstallUILanguage)
            it(InstallUILanguage, SetComittedFlag);
    }

    static void on_ZwFlushInstructionCache(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto BaseAddress   = arg<nt32::PVOID>(d.core, 1);
        const auto Length        = arg<nt32::SIZE_T>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwFlushInstructionCache(ProcessHandle:{:#x}, BaseAddress:{:#x}, Length:{:#x})", ProcessHandle, BaseAddress, Length));

        for(const auto& it : d.observers_ZwFlushInstructionCache)
            it(ProcessHandle, BaseAddress, Length);
    }

    static void on_NtFlushKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtFlushKey(KeyHandle:{:#x})", KeyHandle));

        for(const auto& it : d.observers_NtFlushKey)
            it(KeyHandle);
    }

    static void on_ZwFlushVirtualMemory(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(d.core, 1);
        const auto RegionSize      = arg<nt32::PSIZE_T>(d.core, 2);
        const auto IoStatus        = arg<nt32::PIO_STATUS_BLOCK>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwFlushVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, RegionSize:{:#x}, IoStatus:{:#x})", ProcessHandle, STARBaseAddress, RegionSize, IoStatus));

        for(const auto& it : d.observers_ZwFlushVirtualMemory)
            it(ProcessHandle, STARBaseAddress, RegionSize, IoStatus);
    }

    static void on_NtFreeUserPhysicalPages(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto NumberOfPages = arg<nt32::PULONG_PTR>(d.core, 1);
        const auto UserPfnArra   = arg<nt32::PULONG_PTR>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtFreeUserPhysicalPages(ProcessHandle:{:#x}, NumberOfPages:{:#x}, UserPfnArra:{:#x})", ProcessHandle, NumberOfPages, UserPfnArra));

        for(const auto& it : d.observers_NtFreeUserPhysicalPages)
            it(ProcessHandle, NumberOfPages, UserPfnArra);
    }

    static void on_NtFreeVirtualMemory(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(d.core, 1);
        const auto RegionSize      = arg<nt32::PSIZE_T>(d.core, 2);
        const auto FreeType        = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtFreeVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, RegionSize:{:#x}, FreeType:{:#x})", ProcessHandle, STARBaseAddress, RegionSize, FreeType));

        for(const auto& it : d.observers_NtFreeVirtualMemory)
            it(ProcessHandle, STARBaseAddress, RegionSize, FreeType);
    }

    static void on_NtFreezeRegistry(nt32::syscalls32::Data& d)
    {
        const auto TimeOutInSeconds = arg<nt32::ULONG>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtFreezeRegistry(TimeOutInSeconds:{:#x})", TimeOutInSeconds));

        for(const auto& it : d.observers_NtFreezeRegistry)
            it(TimeOutInSeconds);
    }

    static void on_ZwFreezeTransactions(nt32::syscalls32::Data& d)
    {
        const auto FreezeTimeout = arg<nt32::PLARGE_INTEGER>(d.core, 0);
        const auto ThawTimeout   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwFreezeTransactions(FreezeTimeout:{:#x}, ThawTimeout:{:#x})", FreezeTimeout, ThawTimeout));

        for(const auto& it : d.observers_ZwFreezeTransactions)
            it(FreezeTimeout, ThawTimeout);
    }

    static void on_NtFsControlFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle         = arg<nt32::HANDLE>(d.core, 0);
        const auto Event              = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine         = arg<nt32::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext         = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(d.core, 4);
        const auto IoControlCode      = arg<nt32::ULONG>(d.core, 5);
        const auto InputBuffer        = arg<nt32::PVOID>(d.core, 6);
        const auto InputBufferLength  = arg<nt32::ULONG>(d.core, 7);
        const auto OutputBuffer       = arg<nt32::PVOID>(d.core, 8);
        const auto OutputBufferLength = arg<nt32::ULONG>(d.core, 9);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtFsControlFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, IoControlCode:{:#x}, InputBuffer:{:#x}, InputBufferLength:{:#x}, OutputBuffer:{:#x}, OutputBufferLength:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength));

        for(const auto& it : d.observers_NtFsControlFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    }

    static void on_NtGetContextThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto ThreadContext = arg<nt32::PCONTEXT>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtGetContextThread(ThreadHandle:{:#x}, ThreadContext:{:#x})", ThreadHandle, ThreadContext));

        for(const auto& it : d.observers_NtGetContextThread)
            it(ThreadHandle, ThreadContext);
    }

    static void on_NtGetDevicePowerState(nt32::syscalls32::Data& d)
    {
        const auto Device    = arg<nt32::HANDLE>(d.core, 0);
        const auto STARState = arg<nt32::DEVICE_POWER_STATE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtGetDevicePowerState(Device:{:#x}, STARState:{:#x})", Device, STARState));

        for(const auto& it : d.observers_NtGetDevicePowerState)
            it(Device, STARState);
    }

    static void on_NtGetMUIRegistryInfo(nt32::syscalls32::Data& d)
    {
        const auto Flags    = arg<nt32::ULONG>(d.core, 0);
        const auto DataSize = arg<nt32::PULONG>(d.core, 1);
        const auto Data     = arg<nt32::PVOID>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtGetMUIRegistryInfo(Flags:{:#x}, DataSize:{:#x}, Data:{:#x})", Flags, DataSize, Data));

        for(const auto& it : d.observers_NtGetMUIRegistryInfo)
            it(Flags, DataSize, Data);
    }

    static void on_ZwGetNextProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto HandleAttributes = arg<nt32::ULONG>(d.core, 2);
        const auto Flags            = arg<nt32::ULONG>(d.core, 3);
        const auto NewProcessHandle = arg<nt32::PHANDLE>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwGetNextProcess(ProcessHandle:{:#x}, DesiredAccess:{:#x}, HandleAttributes:{:#x}, Flags:{:#x}, NewProcessHandle:{:#x})", ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle));

        for(const auto& it : d.observers_ZwGetNextProcess)
            it(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
    }

    static void on_ZwGetNextThread(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto ThreadHandle     = arg<nt32::HANDLE>(d.core, 1);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 2);
        const auto HandleAttributes = arg<nt32::ULONG>(d.core, 3);
        const auto Flags            = arg<nt32::ULONG>(d.core, 4);
        const auto NewThreadHandle  = arg<nt32::PHANDLE>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwGetNextThread(ProcessHandle:{:#x}, ThreadHandle:{:#x}, DesiredAccess:{:#x}, HandleAttributes:{:#x}, Flags:{:#x}, NewThreadHandle:{:#x})", ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle));

        for(const auto& it : d.observers_ZwGetNextThread)
            it(ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);
    }

    static void on_NtGetNlsSectionPtr(nt32::syscalls32::Data& d)
    {
        const auto SectionType        = arg<nt32::ULONG>(d.core, 0);
        const auto SectionData        = arg<nt32::ULONG>(d.core, 1);
        const auto ContextData        = arg<nt32::PVOID>(d.core, 2);
        const auto STARSectionPointer = arg<nt32::PVOID>(d.core, 3);
        const auto SectionSize        = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtGetNlsSectionPtr(SectionType:{:#x}, SectionData:{:#x}, ContextData:{:#x}, STARSectionPointer:{:#x}, SectionSize:{:#x})", SectionType, SectionData, ContextData, STARSectionPointer, SectionSize));

        for(const auto& it : d.observers_NtGetNlsSectionPtr)
            it(SectionType, SectionData, ContextData, STARSectionPointer, SectionSize);
    }

    static void on_ZwGetNotificationResourceManager(nt32::syscalls32::Data& d)
    {
        const auto ResourceManagerHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto TransactionNotification = arg<nt32::PTRANSACTION_NOTIFICATION>(d.core, 1);
        const auto NotificationLength      = arg<nt32::ULONG>(d.core, 2);
        const auto Timeout                 = arg<nt32::PLARGE_INTEGER>(d.core, 3);
        const auto ReturnLength            = arg<nt32::PULONG>(d.core, 4);
        const auto Asynchronous            = arg<nt32::ULONG>(d.core, 5);
        const auto AsynchronousContext     = arg<nt32::ULONG_PTR>(d.core, 6);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwGetNotificationResourceManager(ResourceManagerHandle:{:#x}, TransactionNotification:{:#x}, NotificationLength:{:#x}, Timeout:{:#x}, ReturnLength:{:#x}, Asynchronous:{:#x}, AsynchronousContext:{:#x})", ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext));

        for(const auto& it : d.observers_ZwGetNotificationResourceManager)
            it(ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext);
    }

    static void on_NtGetWriteWatch(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle             = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags                     = arg<nt32::ULONG>(d.core, 1);
        const auto BaseAddress               = arg<nt32::PVOID>(d.core, 2);
        const auto RegionSize                = arg<nt32::SIZE_T>(d.core, 3);
        const auto STARUserAddressArray      = arg<nt32::PVOID>(d.core, 4);
        const auto EntriesInUserAddressArray = arg<nt32::PULONG_PTR>(d.core, 5);
        const auto Granularity               = arg<nt32::PULONG>(d.core, 6);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtGetWriteWatch(ProcessHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x}, RegionSize:{:#x}, STARUserAddressArray:{:#x}, EntriesInUserAddressArray:{:#x}, Granularity:{:#x})", ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity));

        for(const auto& it : d.observers_NtGetWriteWatch)
            it(ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity);
    }

    static void on_NtImpersonateAnonymousToken(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtImpersonateAnonymousToken(ThreadHandle:{:#x})", ThreadHandle));

        for(const auto& it : d.observers_NtImpersonateAnonymousToken)
            it(ThreadHandle);
    }

    static void on_ZwImpersonateClientOfPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto Message    = arg<nt32::PPORT_MESSAGE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwImpersonateClientOfPort(PortHandle:{:#x}, Message:{:#x})", PortHandle, Message));

        for(const auto& it : d.observers_ZwImpersonateClientOfPort)
            it(PortHandle, Message);
    }

    static void on_ZwImpersonateThread(nt32::syscalls32::Data& d)
    {
        const auto ServerThreadHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto ClientThreadHandle = arg<nt32::HANDLE>(d.core, 1);
        const auto SecurityQos        = arg<nt32::PSECURITY_QUALITY_OF_SERVICE>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwImpersonateThread(ServerThreadHandle:{:#x}, ClientThreadHandle:{:#x}, SecurityQos:{:#x})", ServerThreadHandle, ClientThreadHandle, SecurityQos));

        for(const auto& it : d.observers_ZwImpersonateThread)
            it(ServerThreadHandle, ClientThreadHandle, SecurityQos);
    }

    static void on_NtInitializeNlsFiles(nt32::syscalls32::Data& d)
    {
        const auto STARBaseAddress        = arg<nt32::PVOID>(d.core, 0);
        const auto DefaultLocaleId        = arg<nt32::PLCID>(d.core, 1);
        const auto DefaultCasingTableSize = arg<nt32::PLARGE_INTEGER>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtInitializeNlsFiles(STARBaseAddress:{:#x}, DefaultLocaleId:{:#x}, DefaultCasingTableSize:{:#x})", STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize));

        for(const auto& it : d.observers_NtInitializeNlsFiles)
            it(STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize);
    }

    static void on_ZwInitializeRegistry(nt32::syscalls32::Data& d)
    {
        const auto BootCondition = arg<nt32::USHORT>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwInitializeRegistry(BootCondition:{:#x})", BootCondition));

        for(const auto& it : d.observers_ZwInitializeRegistry)
            it(BootCondition);
    }

    static void on_NtInitiatePowerAction(nt32::syscalls32::Data& d)
    {
        const auto SystemAction   = arg<nt32::POWER_ACTION>(d.core, 0);
        const auto MinSystemState = arg<nt32::SYSTEM_POWER_STATE>(d.core, 1);
        const auto Flags          = arg<nt32::ULONG>(d.core, 2);
        const auto Asynchronous   = arg<nt32::BOOLEAN>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtInitiatePowerAction(SystemAction:{:#x}, MinSystemState:{:#x}, Flags:{:#x}, Asynchronous:{:#x})", SystemAction, MinSystemState, Flags, Asynchronous));

        for(const auto& it : d.observers_NtInitiatePowerAction)
            it(SystemAction, MinSystemState, Flags, Asynchronous);
    }

    static void on_ZwIsProcessInJob(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto JobHandle     = arg<nt32::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwIsProcessInJob(ProcessHandle:{:#x}, JobHandle:{:#x})", ProcessHandle, JobHandle));

        for(const auto& it : d.observers_ZwIsProcessInJob)
            it(ProcessHandle, JobHandle);
    }

    static void on_ZwListenPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto ConnectionRequest = arg<nt32::PPORT_MESSAGE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwListenPort(PortHandle:{:#x}, ConnectionRequest:{:#x})", PortHandle, ConnectionRequest));

        for(const auto& it : d.observers_ZwListenPort)
            it(PortHandle, ConnectionRequest);
    }

    static void on_NtLoadDriver(nt32::syscalls32::Data& d)
    {
        const auto DriverServiceName = arg<nt32::PUNICODE_STRING>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtLoadDriver(DriverServiceName:{:#x})", DriverServiceName));

        for(const auto& it : d.observers_NtLoadDriver)
            it(DriverServiceName);
    }

    static void on_NtLoadKey2(nt32::syscalls32::Data& d)
    {
        const auto TargetKey  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto SourceFile = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto Flags      = arg<nt32::ULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtLoadKey2(TargetKey:{:#x}, SourceFile:{:#x}, Flags:{:#x})", TargetKey, SourceFile, Flags));

        for(const auto& it : d.observers_NtLoadKey2)
            it(TargetKey, SourceFile, Flags);
    }

    static void on_NtLoadKeyEx(nt32::syscalls32::Data& d)
    {
        const auto TargetKey         = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto SourceFile        = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto Flags             = arg<nt32::ULONG>(d.core, 2);
        const auto TrustClassKey     = arg<nt32::HANDLE>(d.core, 3);
        const auto Reserved          = arg<nt32::PVOID>(d.core, 4);
        const auto ObjectContext     = arg<nt32::PVOID>(d.core, 5);
        const auto CallbackReserverd = arg<nt32::PVOID>(d.core, 6);
        const auto IoStatusBlock     = arg<nt32::PVOID>(d.core, 7);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtLoadKeyEx(TargetKey:{:#x}, SourceFile:{:#x}, Flags:{:#x}, TrustClassKey:{:#x}, Reserved:{:#x}, ObjectContext:{:#x}, CallbackReserverd:{:#x}, IoStatusBlock:{:#x})", TargetKey, SourceFile, Flags, TrustClassKey, Reserved, ObjectContext, CallbackReserverd, IoStatusBlock));

        for(const auto& it : d.observers_NtLoadKeyEx)
            it(TargetKey, SourceFile, Flags, TrustClassKey, Reserved, ObjectContext, CallbackReserverd, IoStatusBlock);
    }

    static void on_NtLoadKey(nt32::syscalls32::Data& d)
    {
        const auto TargetKey  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto SourceFile = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtLoadKey(TargetKey:{:#x}, SourceFile:{:#x})", TargetKey, SourceFile));

        for(const auto& it : d.observers_NtLoadKey)
            it(TargetKey, SourceFile);
    }

    static void on_NtLockFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle      = arg<nt32::HANDLE>(d.core, 0);
        const auto Event           = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine      = arg<nt32::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext      = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatusBlock   = arg<nt32::PIO_STATUS_BLOCK>(d.core, 4);
        const auto ByteOffset      = arg<nt32::PLARGE_INTEGER>(d.core, 5);
        const auto Length          = arg<nt32::PLARGE_INTEGER>(d.core, 6);
        const auto Key             = arg<nt32::ULONG>(d.core, 7);
        const auto FailImmediately = arg<nt32::BOOLEAN>(d.core, 8);
        const auto ExclusiveLock   = arg<nt32::BOOLEAN>(d.core, 9);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtLockFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, ByteOffset:{:#x}, Length:{:#x}, Key:{:#x}, FailImmediately:{:#x}, ExclusiveLock:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock));

        for(const auto& it : d.observers_NtLockFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock);
    }

    static void on_ZwLockProductActivationKeys(nt32::syscalls32::Data& d)
    {
        const auto STARpPrivateVer = arg<nt32::ULONG>(d.core, 0);
        const auto STARpSafeMode   = arg<nt32::ULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwLockProductActivationKeys(STARpPrivateVer:{:#x}, STARpSafeMode:{:#x})", STARpPrivateVer, STARpSafeMode));

        for(const auto& it : d.observers_ZwLockProductActivationKeys)
            it(STARpPrivateVer, STARpSafeMode);
    }

    static void on_NtLockRegistryKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtLockRegistryKey(KeyHandle:{:#x})", KeyHandle));

        for(const auto& it : d.observers_NtLockRegistryKey)
            it(KeyHandle);
    }

    static void on_ZwLockVirtualMemory(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(d.core, 1);
        const auto RegionSize      = arg<nt32::PSIZE_T>(d.core, 2);
        const auto MapType         = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwLockVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, RegionSize:{:#x}, MapType:{:#x})", ProcessHandle, STARBaseAddress, RegionSize, MapType));

        for(const auto& it : d.observers_ZwLockVirtualMemory)
            it(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    }

    static void on_ZwMakePermanentObject(nt32::syscalls32::Data& d)
    {
        const auto Handle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwMakePermanentObject(Handle:{:#x})", Handle));

        for(const auto& it : d.observers_ZwMakePermanentObject)
            it(Handle);
    }

    static void on_NtMakeTemporaryObject(nt32::syscalls32::Data& d)
    {
        const auto Handle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtMakeTemporaryObject(Handle:{:#x})", Handle));

        for(const auto& it : d.observers_NtMakeTemporaryObject)
            it(Handle);
    }

    static void on_ZwMapCMFModule(nt32::syscalls32::Data& d)
    {
        const auto What            = arg<nt32::ULONG>(d.core, 0);
        const auto Index           = arg<nt32::ULONG>(d.core, 1);
        const auto CacheIndexOut   = arg<nt32::PULONG>(d.core, 2);
        const auto CacheFlagsOut   = arg<nt32::PULONG>(d.core, 3);
        const auto ViewSizeOut     = arg<nt32::PULONG>(d.core, 4);
        const auto STARBaseAddress = arg<nt32::PVOID>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwMapCMFModule(What:{:#x}, Index:{:#x}, CacheIndexOut:{:#x}, CacheFlagsOut:{:#x}, ViewSizeOut:{:#x}, STARBaseAddress:{:#x})", What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress));

        for(const auto& it : d.observers_ZwMapCMFModule)
            it(What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress);
    }

    static void on_NtMapUserPhysicalPages(nt32::syscalls32::Data& d)
    {
        const auto VirtualAddress = arg<nt32::PVOID>(d.core, 0);
        const auto NumberOfPages  = arg<nt32::ULONG_PTR>(d.core, 1);
        const auto UserPfnArra    = arg<nt32::PULONG_PTR>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtMapUserPhysicalPages(VirtualAddress:{:#x}, NumberOfPages:{:#x}, UserPfnArra:{:#x})", VirtualAddress, NumberOfPages, UserPfnArra));

        for(const auto& it : d.observers_NtMapUserPhysicalPages)
            it(VirtualAddress, NumberOfPages, UserPfnArra);
    }

    static void on_ZwMapUserPhysicalPagesScatter(nt32::syscalls32::Data& d)
    {
        const auto STARVirtualAddresses = arg<nt32::PVOID>(d.core, 0);
        const auto NumberOfPages        = arg<nt32::ULONG_PTR>(d.core, 1);
        const auto UserPfnArray         = arg<nt32::PULONG_PTR>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwMapUserPhysicalPagesScatter(STARVirtualAddresses:{:#x}, NumberOfPages:{:#x}, UserPfnArray:{:#x})", STARVirtualAddresses, NumberOfPages, UserPfnArray));

        for(const auto& it : d.observers_ZwMapUserPhysicalPagesScatter)
            it(STARVirtualAddresses, NumberOfPages, UserPfnArray);
    }

    static void on_ZwMapViewOfSection(nt32::syscalls32::Data& d)
    {
        const auto SectionHandle      = arg<nt32::HANDLE>(d.core, 0);
        const auto ProcessHandle      = arg<nt32::HANDLE>(d.core, 1);
        const auto STARBaseAddress    = arg<nt32::PVOID>(d.core, 2);
        const auto ZeroBits           = arg<nt32::ULONG_PTR>(d.core, 3);
        const auto CommitSize         = arg<nt32::SIZE_T>(d.core, 4);
        const auto SectionOffset      = arg<nt32::PLARGE_INTEGER>(d.core, 5);
        const auto ViewSize           = arg<nt32::PSIZE_T>(d.core, 6);
        const auto InheritDisposition = arg<nt32::SECTION_INHERIT>(d.core, 7);
        const auto AllocationType     = arg<nt32::ULONG>(d.core, 8);
        const auto Win32Protect       = arg<nt32::WIN32_PROTECTION_MASK>(d.core, 9);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwMapViewOfSection(SectionHandle:{:#x}, ProcessHandle:{:#x}, STARBaseAddress:{:#x}, ZeroBits:{:#x}, CommitSize:{:#x}, SectionOffset:{:#x}, ViewSize:{:#x}, InheritDisposition:{:#x}, AllocationType:{:#x}, Win32Protect:{:#x})", SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect));

        for(const auto& it : d.observers_ZwMapViewOfSection)
            it(SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
    }

    static void on_NtModifyBootEntry(nt32::syscalls32::Data& d)
    {
        const auto BootEntry = arg<nt32::PBOOT_ENTRY>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtModifyBootEntry(BootEntry:{:#x})", BootEntry));

        for(const auto& it : d.observers_NtModifyBootEntry)
            it(BootEntry);
    }

    static void on_ZwModifyDriverEntry(nt32::syscalls32::Data& d)
    {
        const auto DriverEntry = arg<nt32::PEFI_DRIVER_ENTRY>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwModifyDriverEntry(DriverEntry:{:#x})", DriverEntry));

        for(const auto& it : d.observers_ZwModifyDriverEntry)
            it(DriverEntry);
    }

    static void on_NtNotifyChangeDirectoryFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle       = arg<nt32::HANDLE>(d.core, 0);
        const auto Event            = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine       = arg<nt32::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext       = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatusBlock    = arg<nt32::PIO_STATUS_BLOCK>(d.core, 4);
        const auto Buffer           = arg<nt32::PVOID>(d.core, 5);
        const auto Length           = arg<nt32::ULONG>(d.core, 6);
        const auto CompletionFilter = arg<nt32::ULONG>(d.core, 7);
        const auto WatchTree        = arg<nt32::BOOLEAN>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtNotifyChangeDirectoryFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x}, CompletionFilter:{:#x}, WatchTree:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree));

        for(const auto& it : d.observers_NtNotifyChangeDirectoryFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree);
    }

    static void on_NtNotifyChangeKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto Event            = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine       = arg<nt32::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext       = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatusBlock    = arg<nt32::PIO_STATUS_BLOCK>(d.core, 4);
        const auto CompletionFilter = arg<nt32::ULONG>(d.core, 5);
        const auto WatchTree        = arg<nt32::BOOLEAN>(d.core, 6);
        const auto Buffer           = arg<nt32::PVOID>(d.core, 7);
        const auto BufferSize       = arg<nt32::ULONG>(d.core, 8);
        const auto Asynchronous     = arg<nt32::BOOLEAN>(d.core, 9);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtNotifyChangeKey(KeyHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, CompletionFilter:{:#x}, WatchTree:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, Asynchronous:{:#x})", KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous));

        for(const auto& it : d.observers_NtNotifyChangeKey)
            it(KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    }

    static void on_NtNotifyChangeMultipleKeys(nt32::syscalls32::Data& d)
    {
        const auto MasterKeyHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto Count            = arg<nt32::ULONG>(d.core, 1);
        const auto SlaveObjects     = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Event            = arg<nt32::HANDLE>(d.core, 3);
        const auto ApcRoutine       = arg<nt32::PIO_APC_ROUTINE>(d.core, 4);
        const auto ApcContext       = arg<nt32::PVOID>(d.core, 5);
        const auto IoStatusBlock    = arg<nt32::PIO_STATUS_BLOCK>(d.core, 6);
        const auto CompletionFilter = arg<nt32::ULONG>(d.core, 7);
        const auto WatchTree        = arg<nt32::BOOLEAN>(d.core, 8);
        const auto Buffer           = arg<nt32::PVOID>(d.core, 9);
        const auto BufferSize       = arg<nt32::ULONG>(d.core, 10);
        const auto Asynchronous     = arg<nt32::BOOLEAN>(d.core, 11);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtNotifyChangeMultipleKeys(MasterKeyHandle:{:#x}, Count:{:#x}, SlaveObjects:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, CompletionFilter:{:#x}, WatchTree:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, Asynchronous:{:#x})", MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous));

        for(const auto& it : d.observers_NtNotifyChangeMultipleKeys)
            it(MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    }

    static void on_NtNotifyChangeSession(nt32::syscalls32::Data& d)
    {
        const auto Session         = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStateSequence = arg<nt32::ULONG>(d.core, 1);
        const auto Reserved        = arg<nt32::PVOID>(d.core, 2);
        const auto Action          = arg<nt32::ULONG>(d.core, 3);
        const auto IoState         = arg<nt32::IO_SESSION_STATE>(d.core, 4);
        const auto IoState2        = arg<nt32::IO_SESSION_STATE>(d.core, 5);
        const auto Buffer          = arg<nt32::PVOID>(d.core, 6);
        const auto BufferSize      = arg<nt32::ULONG>(d.core, 7);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtNotifyChangeSession(Session:{:#x}, IoStateSequence:{:#x}, Reserved:{:#x}, Action:{:#x}, IoState:{:#x}, IoState2:{:#x}, Buffer:{:#x}, BufferSize:{:#x})", Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize));

        for(const auto& it : d.observers_NtNotifyChangeSession)
            it(Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize);
    }

    static void on_ZwOpenDirectoryObject(nt32::syscalls32::Data& d)
    {
        const auto DirectoryHandle  = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenDirectoryObject(DirectoryHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", DirectoryHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_ZwOpenDirectoryObject)
            it(DirectoryHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_ZwOpenEnlistment(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle      = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess         = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ResourceManagerHandle = arg<nt32::HANDLE>(d.core, 2);
        const auto EnlistmentGuid        = arg<nt32::LPGUID>(d.core, 3);
        const auto ObjectAttributes      = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenEnlistment(EnlistmentHandle:{:#x}, DesiredAccess:{:#x}, ResourceManagerHandle:{:#x}, EnlistmentGuid:{:#x}, ObjectAttributes:{:#x})", EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes));

        for(const auto& it : d.observers_ZwOpenEnlistment)
            it(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes);
    }

    static void on_NtOpenEvent(nt32::syscalls32::Data& d)
    {
        const auto EventHandle      = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenEvent(EventHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", EventHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_NtOpenEvent)
            it(EventHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenEventPair(nt32::syscalls32::Data& d)
    {
        const auto EventPairHandle  = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenEventPair(EventPairHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", EventPairHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_NtOpenEventPair)
            it(EventPairHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle       = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto IoStatusBlock    = arg<nt32::PIO_STATUS_BLOCK>(d.core, 3);
        const auto ShareAccess      = arg<nt32::ULONG>(d.core, 4);
        const auto OpenOptions      = arg<nt32::ULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenFile(FileHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, IoStatusBlock:{:#x}, ShareAccess:{:#x}, OpenOptions:{:#x})", FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions));

        for(const auto& it : d.observers_NtOpenFile)
            it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
    }

    static void on_ZwOpenIoCompletion(nt32::syscalls32::Data& d)
    {
        const auto IoCompletionHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenIoCompletion(IoCompletionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", IoCompletionHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_ZwOpenIoCompletion)
            it(IoCompletionHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_ZwOpenJobObject(nt32::syscalls32::Data& d)
    {
        const auto JobHandle        = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenJobObject(JobHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", JobHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_ZwOpenJobObject)
            it(JobHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenKeyedEvent(nt32::syscalls32::Data& d)
    {
        const auto KeyedEventHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenKeyedEvent(KeyedEventHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", KeyedEventHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_NtOpenKeyedEvent)
            it(KeyedEventHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_ZwOpenKeyEx(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle        = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto OpenOptions      = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenKeyEx(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, OpenOptions:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions));

        for(const auto& it : d.observers_ZwOpenKeyEx)
            it(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
    }

    static void on_ZwOpenKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle        = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenKey(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_ZwOpenKey)
            it(KeyHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenKeyTransactedEx(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle         = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto OpenOptions       = arg<nt32::ULONG>(d.core, 3);
        const auto TransactionHandle = arg<nt32::HANDLE>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenKeyTransactedEx(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, OpenOptions:{:#x}, TransactionHandle:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle));

        for(const auto& it : d.observers_NtOpenKeyTransactedEx)
            it(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle);
    }

    static void on_NtOpenKeyTransacted(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle         = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto TransactionHandle = arg<nt32::HANDLE>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenKeyTransacted(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, TransactionHandle:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle));

        for(const auto& it : d.observers_NtOpenKeyTransacted)
            it(KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle);
    }

    static void on_NtOpenMutant(nt32::syscalls32::Data& d)
    {
        const auto MutantHandle     = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenMutant(MutantHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", MutantHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_NtOpenMutant)
            it(MutantHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_ZwOpenObjectAuditAlarm(nt32::syscalls32::Data& d)
    {
        const auto SubsystemName      = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto HandleId           = arg<nt32::PVOID>(d.core, 1);
        const auto ObjectTypeName     = arg<nt32::PUNICODE_STRING>(d.core, 2);
        const auto ObjectName         = arg<nt32::PUNICODE_STRING>(d.core, 3);
        const auto SecurityDescriptor = arg<nt32::PSECURITY_DESCRIPTOR>(d.core, 4);
        const auto ClientToken        = arg<nt32::HANDLE>(d.core, 5);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(d.core, 6);
        const auto GrantedAccess      = arg<nt32::ACCESS_MASK>(d.core, 7);
        const auto Privileges         = arg<nt32::PPRIVILEGE_SET>(d.core, 8);
        const auto ObjectCreation     = arg<nt32::BOOLEAN>(d.core, 9);
        const auto AccessGranted      = arg<nt32::BOOLEAN>(d.core, 10);
        const auto GenerateOnClose    = arg<nt32::PBOOLEAN>(d.core, 11);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenObjectAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, ObjectTypeName:{:#x}, ObjectName:{:#x}, SecurityDescriptor:{:#x}, ClientToken:{:#x}, DesiredAccess:{:#x}, GrantedAccess:{:#x}, Privileges:{:#x}, ObjectCreation:{:#x}, AccessGranted:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose));

        for(const auto& it : d.observers_ZwOpenObjectAuditAlarm)
            it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);
    }

    static void on_NtOpenPrivateNamespace(nt32::syscalls32::Data& d)
    {
        const auto NamespaceHandle    = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto BoundaryDescriptor = arg<nt32::PVOID>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenPrivateNamespace(NamespaceHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, BoundaryDescriptor:{:#x})", NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor));

        for(const auto& it : d.observers_NtOpenPrivateNamespace)
            it(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    }

    static void on_ZwOpenProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle    = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ClientId         = arg<nt32::PCLIENT_ID>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenProcess(ProcessHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ClientId:{:#x})", ProcessHandle, DesiredAccess, ObjectAttributes, ClientId));

        for(const auto& it : d.observers_ZwOpenProcess)
            it(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
    }

    static void on_ZwOpenProcessTokenEx(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto HandleAttributes = arg<nt32::ULONG>(d.core, 2);
        const auto TokenHandle      = arg<nt32::PHANDLE>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenProcessTokenEx(ProcessHandle:{:#x}, DesiredAccess:{:#x}, HandleAttributes:{:#x}, TokenHandle:{:#x})", ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle));

        for(const auto& it : d.observers_ZwOpenProcessTokenEx)
            it(ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle);
    }

    static void on_ZwOpenProcessToken(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto DesiredAccess = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto TokenHandle   = arg<nt32::PHANDLE>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenProcessToken(ProcessHandle:{:#x}, DesiredAccess:{:#x}, TokenHandle:{:#x})", ProcessHandle, DesiredAccess, TokenHandle));

        for(const auto& it : d.observers_ZwOpenProcessToken)
            it(ProcessHandle, DesiredAccess, TokenHandle);
    }

    static void on_ZwOpenResourceManager(nt32::syscalls32::Data& d)
    {
        const auto ResourceManagerHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess         = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto TmHandle              = arg<nt32::HANDLE>(d.core, 2);
        const auto ResourceManagerGuid   = arg<nt32::LPGUID>(d.core, 3);
        const auto ObjectAttributes      = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenResourceManager(ResourceManagerHandle:{:#x}, DesiredAccess:{:#x}, TmHandle:{:#x}, ResourceManagerGuid:{:#x}, ObjectAttributes:{:#x})", ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes));

        for(const auto& it : d.observers_ZwOpenResourceManager)
            it(ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes);
    }

    static void on_NtOpenSection(nt32::syscalls32::Data& d)
    {
        const auto SectionHandle    = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenSection(SectionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", SectionHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_NtOpenSection)
            it(SectionHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenSemaphore(nt32::syscalls32::Data& d)
    {
        const auto SemaphoreHandle  = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenSemaphore(SemaphoreHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", SemaphoreHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_NtOpenSemaphore)
            it(SemaphoreHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenSession(nt32::syscalls32::Data& d)
    {
        const auto SessionHandle    = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenSession(SessionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", SessionHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_NtOpenSession)
            it(SessionHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenSymbolicLinkObject(nt32::syscalls32::Data& d)
    {
        const auto LinkHandle       = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenSymbolicLinkObject(LinkHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", LinkHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_NtOpenSymbolicLinkObject)
            it(LinkHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_ZwOpenThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle     = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ClientId         = arg<nt32::PCLIENT_ID>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenThread(ThreadHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ClientId:{:#x})", ThreadHandle, DesiredAccess, ObjectAttributes, ClientId));

        for(const auto& it : d.observers_ZwOpenThread)
            it(ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);
    }

    static void on_NtOpenThreadTokenEx(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto OpenAsSelf       = arg<nt32::BOOLEAN>(d.core, 2);
        const auto HandleAttributes = arg<nt32::ULONG>(d.core, 3);
        const auto TokenHandle      = arg<nt32::PHANDLE>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenThreadTokenEx(ThreadHandle:{:#x}, DesiredAccess:{:#x}, OpenAsSelf:{:#x}, HandleAttributes:{:#x}, TokenHandle:{:#x})", ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle));

        for(const auto& it : d.observers_NtOpenThreadTokenEx)
            it(ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle);
    }

    static void on_NtOpenThreadToken(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto DesiredAccess = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto OpenAsSelf    = arg<nt32::BOOLEAN>(d.core, 2);
        const auto TokenHandle   = arg<nt32::PHANDLE>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtOpenThreadToken(ThreadHandle:{:#x}, DesiredAccess:{:#x}, OpenAsSelf:{:#x}, TokenHandle:{:#x})", ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle));

        for(const auto& it : d.observers_NtOpenThreadToken)
            it(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
    }

    static void on_ZwOpenTimer(nt32::syscalls32::Data& d)
    {
        const auto TimerHandle      = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenTimer(TimerHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", TimerHandle, DesiredAccess, ObjectAttributes));

        for(const auto& it : d.observers_ZwOpenTimer)
            it(TimerHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_ZwOpenTransactionManager(nt32::syscalls32::Data& d)
    {
        const auto TmHandle         = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto LogFileName      = arg<nt32::PUNICODE_STRING>(d.core, 3);
        const auto TmIdentity       = arg<nt32::LPGUID>(d.core, 4);
        const auto OpenOptions      = arg<nt32::ULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenTransactionManager(TmHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, LogFileName:{:#x}, TmIdentity:{:#x}, OpenOptions:{:#x})", TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions));

        for(const auto& it : d.observers_ZwOpenTransactionManager)
            it(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions);
    }

    static void on_ZwOpenTransaction(nt32::syscalls32::Data& d)
    {
        const auto TransactionHandle = arg<nt32::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Uow               = arg<nt32::LPGUID>(d.core, 3);
        const auto TmHandle          = arg<nt32::HANDLE>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwOpenTransaction(TransactionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, Uow:{:#x}, TmHandle:{:#x})", TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle));

        for(const auto& it : d.observers_ZwOpenTransaction)
            it(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle);
    }

    static void on_NtPlugPlayControl(nt32::syscalls32::Data& d)
    {
        const auto PnPControlClass      = arg<nt32::PLUGPLAY_CONTROL_CLASS>(d.core, 0);
        const auto PnPControlData       = arg<nt32::PVOID>(d.core, 1);
        const auto PnPControlDataLength = arg<nt32::ULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtPlugPlayControl(PnPControlClass:{:#x}, PnPControlData:{:#x}, PnPControlDataLength:{:#x})", PnPControlClass, PnPControlData, PnPControlDataLength));

        for(const auto& it : d.observers_NtPlugPlayControl)
            it(PnPControlClass, PnPControlData, PnPControlDataLength);
    }

    static void on_ZwPowerInformation(nt32::syscalls32::Data& d)
    {
        const auto InformationLevel   = arg<nt32::POWER_INFORMATION_LEVEL>(d.core, 0);
        const auto InputBuffer        = arg<nt32::PVOID>(d.core, 1);
        const auto InputBufferLength  = arg<nt32::ULONG>(d.core, 2);
        const auto OutputBuffer       = arg<nt32::PVOID>(d.core, 3);
        const auto OutputBufferLength = arg<nt32::ULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwPowerInformation(InformationLevel:{:#x}, InputBuffer:{:#x}, InputBufferLength:{:#x}, OutputBuffer:{:#x}, OutputBufferLength:{:#x})", InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength));

        for(const auto& it : d.observers_ZwPowerInformation)
            it(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    }

    static void on_NtPrepareComplete(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtPrepareComplete(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock));

        for(const auto& it : d.observers_NtPrepareComplete)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_ZwPrepareEnlistment(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwPrepareEnlistment(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock));

        for(const auto& it : d.observers_ZwPrepareEnlistment)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_ZwPrePrepareComplete(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwPrePrepareComplete(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock));

        for(const auto& it : d.observers_ZwPrePrepareComplete)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtPrePrepareEnlistment(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtPrePrepareEnlistment(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock));

        for(const auto& it : d.observers_NtPrePrepareEnlistment)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_ZwPrivilegeCheck(nt32::syscalls32::Data& d)
    {
        const auto ClientToken        = arg<nt32::HANDLE>(d.core, 0);
        const auto RequiredPrivileges = arg<nt32::PPRIVILEGE_SET>(d.core, 1);
        const auto Result             = arg<nt32::PBOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwPrivilegeCheck(ClientToken:{:#x}, RequiredPrivileges:{:#x}, Result:{:#x})", ClientToken, RequiredPrivileges, Result));

        for(const auto& it : d.observers_ZwPrivilegeCheck)
            it(ClientToken, RequiredPrivileges, Result);
    }

    static void on_NtPrivilegedServiceAuditAlarm(nt32::syscalls32::Data& d)
    {
        const auto SubsystemName = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto ServiceName   = arg<nt32::PUNICODE_STRING>(d.core, 1);
        const auto ClientToken   = arg<nt32::HANDLE>(d.core, 2);
        const auto Privileges    = arg<nt32::PPRIVILEGE_SET>(d.core, 3);
        const auto AccessGranted = arg<nt32::BOOLEAN>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtPrivilegedServiceAuditAlarm(SubsystemName:{:#x}, ServiceName:{:#x}, ClientToken:{:#x}, Privileges:{:#x}, AccessGranted:{:#x})", SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted));

        for(const auto& it : d.observers_NtPrivilegedServiceAuditAlarm)
            it(SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted);
    }

    static void on_ZwPrivilegeObjectAuditAlarm(nt32::syscalls32::Data& d)
    {
        const auto SubsystemName = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto HandleId      = arg<nt32::PVOID>(d.core, 1);
        const auto ClientToken   = arg<nt32::HANDLE>(d.core, 2);
        const auto DesiredAccess = arg<nt32::ACCESS_MASK>(d.core, 3);
        const auto Privileges    = arg<nt32::PPRIVILEGE_SET>(d.core, 4);
        const auto AccessGranted = arg<nt32::BOOLEAN>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwPrivilegeObjectAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, ClientToken:{:#x}, DesiredAccess:{:#x}, Privileges:{:#x}, AccessGranted:{:#x})", SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted));

        for(const auto& it : d.observers_ZwPrivilegeObjectAuditAlarm)
            it(SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);
    }

    static void on_NtPropagationComplete(nt32::syscalls32::Data& d)
    {
        const auto ResourceManagerHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto RequestCookie         = arg<nt32::ULONG>(d.core, 1);
        const auto BufferLength          = arg<nt32::ULONG>(d.core, 2);
        const auto Buffer                = arg<nt32::PVOID>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtPropagationComplete(ResourceManagerHandle:{:#x}, RequestCookie:{:#x}, BufferLength:{:#x}, Buffer:{:#x})", ResourceManagerHandle, RequestCookie, BufferLength, Buffer));

        for(const auto& it : d.observers_NtPropagationComplete)
            it(ResourceManagerHandle, RequestCookie, BufferLength, Buffer);
    }

    static void on_ZwPropagationFailed(nt32::syscalls32::Data& d)
    {
        const auto ResourceManagerHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto RequestCookie         = arg<nt32::ULONG>(d.core, 1);
        const auto PropStatus            = arg<nt32::NTSTATUS>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwPropagationFailed(ResourceManagerHandle:{:#x}, RequestCookie:{:#x}, PropStatus:{:#x})", ResourceManagerHandle, RequestCookie, PropStatus));

        for(const auto& it : d.observers_ZwPropagationFailed)
            it(ResourceManagerHandle, RequestCookie, PropStatus);
    }

    static void on_ZwProtectVirtualMemory(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(d.core, 1);
        const auto RegionSize      = arg<nt32::PSIZE_T>(d.core, 2);
        const auto NewProtectWin32 = arg<nt32::WIN32_PROTECTION_MASK>(d.core, 3);
        const auto OldProtect      = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwProtectVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, RegionSize:{:#x}, NewProtectWin32:{:#x}, OldProtect:{:#x})", ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect));

        for(const auto& it : d.observers_ZwProtectVirtualMemory)
            it(ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect);
    }

    static void on_ZwPulseEvent(nt32::syscalls32::Data& d)
    {
        const auto EventHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto PreviousState = arg<nt32::PLONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwPulseEvent(EventHandle:{:#x}, PreviousState:{:#x})", EventHandle, PreviousState));

        for(const auto& it : d.observers_ZwPulseEvent)
            it(EventHandle, PreviousState);
    }

    static void on_ZwQueryAttributesFile(nt32::syscalls32::Data& d)
    {
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto FileInformation  = arg<nt32::PFILE_BASIC_INFORMATION>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryAttributesFile(ObjectAttributes:{:#x}, FileInformation:{:#x})", ObjectAttributes, FileInformation));

        for(const auto& it : d.observers_ZwQueryAttributesFile)
            it(ObjectAttributes, FileInformation);
    }

    static void on_ZwQueryBootEntryOrder(nt32::syscalls32::Data& d)
    {
        const auto Ids   = arg<nt32::PULONG>(d.core, 0);
        const auto Count = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryBootEntryOrder(Ids:{:#x}, Count:{:#x})", Ids, Count));

        for(const auto& it : d.observers_ZwQueryBootEntryOrder)
            it(Ids, Count);
    }

    static void on_ZwQueryBootOptions(nt32::syscalls32::Data& d)
    {
        const auto BootOptions       = arg<nt32::PBOOT_OPTIONS>(d.core, 0);
        const auto BootOptionsLength = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryBootOptions(BootOptions:{:#x}, BootOptionsLength:{:#x})", BootOptions, BootOptionsLength));

        for(const auto& it : d.observers_ZwQueryBootOptions)
            it(BootOptions, BootOptionsLength);
    }

    static void on_NtQueryDebugFilterState(nt32::syscalls32::Data& d)
    {
        const auto ComponentId = arg<nt32::ULONG>(d.core, 0);
        const auto Level       = arg<nt32::ULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryDebugFilterState(ComponentId:{:#x}, Level:{:#x})", ComponentId, Level));

        for(const auto& it : d.observers_NtQueryDebugFilterState)
            it(ComponentId, Level);
    }

    static void on_NtQueryDefaultLocale(nt32::syscalls32::Data& d)
    {
        const auto UserProfile     = arg<nt32::BOOLEAN>(d.core, 0);
        const auto DefaultLocaleId = arg<nt32::PLCID>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryDefaultLocale(UserProfile:{:#x}, DefaultLocaleId:{:#x})", UserProfile, DefaultLocaleId));

        for(const auto& it : d.observers_NtQueryDefaultLocale)
            it(UserProfile, DefaultLocaleId);
    }

    static void on_ZwQueryDefaultUILanguage(nt32::syscalls32::Data& d)
    {
        const auto STARDefaultUILanguageId = arg<nt32::LANGID>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryDefaultUILanguage(STARDefaultUILanguageId:{:#x})", STARDefaultUILanguageId));

        for(const auto& it : d.observers_ZwQueryDefaultUILanguage)
            it(STARDefaultUILanguageId);
    }

    static void on_ZwQueryDirectoryFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto Event                = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine           = arg<nt32::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext           = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatusBlock        = arg<nt32::PIO_STATUS_BLOCK>(d.core, 4);
        const auto FileInformation      = arg<nt32::PVOID>(d.core, 5);
        const auto Length               = arg<nt32::ULONG>(d.core, 6);
        const auto FileInformationClass = arg<nt32::FILE_INFORMATION_CLASS>(d.core, 7);
        const auto ReturnSingleEntry    = arg<nt32::BOOLEAN>(d.core, 8);
        const auto FileName             = arg<nt32::PUNICODE_STRING>(d.core, 9);
        const auto RestartScan          = arg<nt32::BOOLEAN>(d.core, 10);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryDirectoryFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, FileInformation:{:#x}, Length:{:#x}, FileInformationClass:{:#x}, ReturnSingleEntry:{:#x}, FileName:{:#x}, RestartScan:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan));

        for(const auto& it : d.observers_ZwQueryDirectoryFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
    }

    static void on_ZwQueryDirectoryObject(nt32::syscalls32::Data& d)
    {
        const auto DirectoryHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto Buffer            = arg<nt32::PVOID>(d.core, 1);
        const auto Length            = arg<nt32::ULONG>(d.core, 2);
        const auto ReturnSingleEntry = arg<nt32::BOOLEAN>(d.core, 3);
        const auto RestartScan       = arg<nt32::BOOLEAN>(d.core, 4);
        const auto Context           = arg<nt32::PULONG>(d.core, 5);
        const auto ReturnLength      = arg<nt32::PULONG>(d.core, 6);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryDirectoryObject(DirectoryHandle:{:#x}, Buffer:{:#x}, Length:{:#x}, ReturnSingleEntry:{:#x}, RestartScan:{:#x}, Context:{:#x}, ReturnLength:{:#x})", DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength));

        for(const auto& it : d.observers_ZwQueryDirectoryObject)
            it(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength);
    }

    static void on_NtQueryDriverEntryOrder(nt32::syscalls32::Data& d)
    {
        const auto Ids   = arg<nt32::PULONG>(d.core, 0);
        const auto Count = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryDriverEntryOrder(Ids:{:#x}, Count:{:#x})", Ids, Count));

        for(const auto& it : d.observers_NtQueryDriverEntryOrder)
            it(Ids, Count);
    }

    static void on_ZwQueryEaFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto Buffer            = arg<nt32::PVOID>(d.core, 2);
        const auto Length            = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnSingleEntry = arg<nt32::BOOLEAN>(d.core, 4);
        const auto EaList            = arg<nt32::PVOID>(d.core, 5);
        const auto EaListLength      = arg<nt32::ULONG>(d.core, 6);
        const auto EaIndex           = arg<nt32::PULONG>(d.core, 7);
        const auto RestartScan       = arg<nt32::BOOLEAN>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryEaFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x}, ReturnSingleEntry:{:#x}, EaList:{:#x}, EaListLength:{:#x}, EaIndex:{:#x}, RestartScan:{:#x})", FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan));

        for(const auto& it : d.observers_ZwQueryEaFile)
            it(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
    }

    static void on_NtQueryEvent(nt32::syscalls32::Data& d)
    {
        const auto EventHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto EventInformationClass  = arg<nt32::EVENT_INFORMATION_CLASS>(d.core, 1);
        const auto EventInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto EventInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength           = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryEvent(EventHandle:{:#x}, EventInformationClass:{:#x}, EventInformation:{:#x}, EventInformationLength:{:#x}, ReturnLength:{:#x})", EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength));

        for(const auto& it : d.observers_NtQueryEvent)
            it(EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength);
    }

    static void on_ZwQueryFullAttributesFile(nt32::syscalls32::Data& d)
    {
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto FileInformation  = arg<nt32::PFILE_NETWORK_OPEN_INFORMATION>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryFullAttributesFile(ObjectAttributes:{:#x}, FileInformation:{:#x})", ObjectAttributes, FileInformation));

        for(const auto& it : d.observers_ZwQueryFullAttributesFile)
            it(ObjectAttributes, FileInformation);
    }

    static void on_NtQueryInformationAtom(nt32::syscalls32::Data& d)
    {
        const auto Atom                  = arg<nt32::RTL_ATOM>(d.core, 0);
        const auto InformationClass      = arg<nt32::ATOM_INFORMATION_CLASS>(d.core, 1);
        const auto AtomInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto AtomInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength          = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryInformationAtom(Atom:{:#x}, InformationClass:{:#x}, AtomInformation:{:#x}, AtomInformationLength:{:#x}, ReturnLength:{:#x})", Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength));

        for(const auto& it : d.observers_NtQueryInformationAtom)
            it(Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength);
    }

    static void on_ZwQueryInformationEnlistment(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto EnlistmentInformationClass  = arg<nt32::ENLISTMENT_INFORMATION_CLASS>(d.core, 1);
        const auto EnlistmentInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto EnlistmentInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength                = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryInformationEnlistment(EnlistmentHandle:{:#x}, EnlistmentInformationClass:{:#x}, EnlistmentInformation:{:#x}, EnlistmentInformationLength:{:#x}, ReturnLength:{:#x})", EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQueryInformationEnlistment)
            it(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength);
    }

    static void on_ZwQueryInformationFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock        = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto FileInformation      = arg<nt32::PVOID>(d.core, 2);
        const auto Length               = arg<nt32::ULONG>(d.core, 3);
        const auto FileInformationClass = arg<nt32::FILE_INFORMATION_CLASS>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, FileInformation:{:#x}, Length:{:#x}, FileInformationClass:{:#x})", FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass));

        for(const auto& it : d.observers_ZwQueryInformationFile)
            it(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    }

    static void on_ZwQueryInformationJobObject(nt32::syscalls32::Data& d)
    {
        const auto JobHandle                  = arg<nt32::HANDLE>(d.core, 0);
        const auto JobObjectInformationClass  = arg<nt32::JOBOBJECTINFOCLASS>(d.core, 1);
        const auto JobObjectInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto JobObjectInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength               = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryInformationJobObject(JobHandle:{:#x}, JobObjectInformationClass:{:#x}, JobObjectInformation:{:#x}, JobObjectInformationLength:{:#x}, ReturnLength:{:#x})", JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQueryInformationJobObject)
            it(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength);
    }

    static void on_ZwQueryInformationPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto PortInformationClass = arg<nt32::PORT_INFORMATION_CLASS>(d.core, 1);
        const auto PortInformation      = arg<nt32::PVOID>(d.core, 2);
        const auto Length               = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength         = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryInformationPort(PortHandle:{:#x}, PortInformationClass:{:#x}, PortInformation:{:#x}, Length:{:#x}, ReturnLength:{:#x})", PortHandle, PortInformationClass, PortInformation, Length, ReturnLength));

        for(const auto& it : d.observers_ZwQueryInformationPort)
            it(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    }

    static void on_ZwQueryInformationProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto ProcessInformationClass  = arg<nt32::PROCESSINFOCLASS>(d.core, 1);
        const auto ProcessInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto ProcessInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength             = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryInformationProcess(ProcessHandle:{:#x}, ProcessInformationClass:{:#x}, ProcessInformation:{:#x}, ProcessInformationLength:{:#x}, ReturnLength:{:#x})", ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQueryInformationProcess)
            it(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
    }

    static void on_ZwQueryInformationResourceManager(nt32::syscalls32::Data& d)
    {
        const auto ResourceManagerHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto ResourceManagerInformationClass  = arg<nt32::RESOURCEMANAGER_INFORMATION_CLASS>(d.core, 1);
        const auto ResourceManagerInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto ResourceManagerInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength                     = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryInformationResourceManager(ResourceManagerHandle:{:#x}, ResourceManagerInformationClass:{:#x}, ResourceManagerInformation:{:#x}, ResourceManagerInformationLength:{:#x}, ReturnLength:{:#x})", ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQueryInformationResourceManager)
            it(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto ThreadInformationClass  = arg<nt32::THREADINFOCLASS>(d.core, 1);
        const auto ThreadInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto ThreadInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength            = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryInformationThread(ThreadHandle:{:#x}, ThreadInformationClass:{:#x}, ThreadInformation:{:#x}, ThreadInformationLength:{:#x}, ReturnLength:{:#x})", ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength));

        for(const auto& it : d.observers_NtQueryInformationThread)
            it(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
    }

    static void on_ZwQueryInformationToken(nt32::syscalls32::Data& d)
    {
        const auto TokenHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto TokenInformationClass  = arg<nt32::TOKEN_INFORMATION_CLASS>(d.core, 1);
        const auto TokenInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto TokenInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength           = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryInformationToken(TokenHandle:{:#x}, TokenInformationClass:{:#x}, TokenInformation:{:#x}, TokenInformationLength:{:#x}, ReturnLength:{:#x})", TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQueryInformationToken)
            it(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
    }

    static void on_ZwQueryInformationTransaction(nt32::syscalls32::Data& d)
    {
        const auto TransactionHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto TransactionInformationClass  = arg<nt32::TRANSACTION_INFORMATION_CLASS>(d.core, 1);
        const auto TransactionInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto TransactionInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength                 = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryInformationTransaction(TransactionHandle:{:#x}, TransactionInformationClass:{:#x}, TransactionInformation:{:#x}, TransactionInformationLength:{:#x}, ReturnLength:{:#x})", TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQueryInformationTransaction)
            it(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationTransactionManager(nt32::syscalls32::Data& d)
    {
        const auto TransactionManagerHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto TransactionManagerInformationClass  = arg<nt32::TRANSACTIONMANAGER_INFORMATION_CLASS>(d.core, 1);
        const auto TransactionManagerInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto TransactionManagerInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength                        = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryInformationTransactionManager(TransactionManagerHandle:{:#x}, TransactionManagerInformationClass:{:#x}, TransactionManagerInformation:{:#x}, TransactionManagerInformationLength:{:#x}, ReturnLength:{:#x})", TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength));

        for(const auto& it : d.observers_NtQueryInformationTransactionManager)
            it(TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength);
    }

    static void on_ZwQueryInformationWorkerFactory(nt32::syscalls32::Data& d)
    {
        const auto WorkerFactoryHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto WorkerFactoryInformationClass  = arg<nt32::WORKERFACTORYINFOCLASS>(d.core, 1);
        const auto WorkerFactoryInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto WorkerFactoryInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength                   = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryInformationWorkerFactory(WorkerFactoryHandle:{:#x}, WorkerFactoryInformationClass:{:#x}, WorkerFactoryInformation:{:#x}, WorkerFactoryInformationLength:{:#x}, ReturnLength:{:#x})", WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQueryInformationWorkerFactory)
            it(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength);
    }

    static void on_NtQueryInstallUILanguage(nt32::syscalls32::Data& d)
    {
        const auto STARInstallUILanguageId = arg<nt32::LANGID>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryInstallUILanguage(STARInstallUILanguageId:{:#x})", STARInstallUILanguageId));

        for(const auto& it : d.observers_NtQueryInstallUILanguage)
            it(STARInstallUILanguageId);
    }

    static void on_NtQueryIntervalProfile(nt32::syscalls32::Data& d)
    {
        const auto ProfileSource = arg<nt32::KPROFILE_SOURCE>(d.core, 0);
        const auto Interval      = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryIntervalProfile(ProfileSource:{:#x}, Interval:{:#x})", ProfileSource, Interval));

        for(const auto& it : d.observers_NtQueryIntervalProfile)
            it(ProfileSource, Interval);
    }

    static void on_NtQueryIoCompletion(nt32::syscalls32::Data& d)
    {
        const auto IoCompletionHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto IoCompletionInformationClass  = arg<nt32::IO_COMPLETION_INFORMATION_CLASS>(d.core, 1);
        const auto IoCompletionInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto IoCompletionInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength                  = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryIoCompletion(IoCompletionHandle:{:#x}, IoCompletionInformationClass:{:#x}, IoCompletionInformation:{:#x}, IoCompletionInformationLength:{:#x}, ReturnLength:{:#x})", IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength));

        for(const auto& it : d.observers_NtQueryIoCompletion)
            it(IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength);
    }

    static void on_ZwQueryKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto KeyInformationClass = arg<nt32::KEY_INFORMATION_CLASS>(d.core, 1);
        const auto KeyInformation      = arg<nt32::PVOID>(d.core, 2);
        const auto Length              = arg<nt32::ULONG>(d.core, 3);
        const auto ResultLength        = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryKey(KeyHandle:{:#x}, KeyInformationClass:{:#x}, KeyInformation:{:#x}, Length:{:#x}, ResultLength:{:#x})", KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength));

        for(const auto& it : d.observers_ZwQueryKey)
            it(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
    }

    static void on_NtQueryLicenseValue(nt32::syscalls32::Data& d)
    {
        const auto Name           = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto Type           = arg<nt32::PULONG>(d.core, 1);
        const auto Buffer         = arg<nt32::PVOID>(d.core, 2);
        const auto Length         = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnedLength = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryLicenseValue(Name:{:#x}, Type:{:#x}, Buffer:{:#x}, Length:{:#x}, ReturnedLength:{:#x})", Name, Type, Buffer, Length, ReturnedLength));

        for(const auto& it : d.observers_NtQueryLicenseValue)
            it(Name, Type, Buffer, Length, ReturnedLength);
    }

    static void on_NtQueryMultipleValueKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto ValueEntries         = arg<nt32::PKEY_VALUE_ENTRY>(d.core, 1);
        const auto EntryCount           = arg<nt32::ULONG>(d.core, 2);
        const auto ValueBuffer          = arg<nt32::PVOID>(d.core, 3);
        const auto BufferLength         = arg<nt32::PULONG>(d.core, 4);
        const auto RequiredBufferLength = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryMultipleValueKey(KeyHandle:{:#x}, ValueEntries:{:#x}, EntryCount:{:#x}, ValueBuffer:{:#x}, BufferLength:{:#x}, RequiredBufferLength:{:#x})", KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength));

        for(const auto& it : d.observers_NtQueryMultipleValueKey)
            it(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
    }

    static void on_NtQueryMutant(nt32::syscalls32::Data& d)
    {
        const auto MutantHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto MutantInformationClass  = arg<nt32::MUTANT_INFORMATION_CLASS>(d.core, 1);
        const auto MutantInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto MutantInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength            = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryMutant(MutantHandle:{:#x}, MutantInformationClass:{:#x}, MutantInformation:{:#x}, MutantInformationLength:{:#x}, ReturnLength:{:#x})", MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength));

        for(const auto& it : d.observers_NtQueryMutant)
            it(MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength);
    }

    static void on_NtQueryObject(nt32::syscalls32::Data& d)
    {
        const auto Handle                  = arg<nt32::HANDLE>(d.core, 0);
        const auto ObjectInformationClass  = arg<nt32::OBJECT_INFORMATION_CLASS>(d.core, 1);
        const auto ObjectInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto ObjectInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength            = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryObject(Handle:{:#x}, ObjectInformationClass:{:#x}, ObjectInformation:{:#x}, ObjectInformationLength:{:#x}, ReturnLength:{:#x})", Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength));

        for(const auto& it : d.observers_NtQueryObject)
            it(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
    }

    static void on_NtQueryOpenSubKeysEx(nt32::syscalls32::Data& d)
    {
        const auto TargetKey    = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto BufferLength = arg<nt32::ULONG>(d.core, 1);
        const auto Buffer       = arg<nt32::PVOID>(d.core, 2);
        const auto RequiredSize = arg<nt32::PULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryOpenSubKeysEx(TargetKey:{:#x}, BufferLength:{:#x}, Buffer:{:#x}, RequiredSize:{:#x})", TargetKey, BufferLength, Buffer, RequiredSize));

        for(const auto& it : d.observers_NtQueryOpenSubKeysEx)
            it(TargetKey, BufferLength, Buffer, RequiredSize);
    }

    static void on_NtQueryOpenSubKeys(nt32::syscalls32::Data& d)
    {
        const auto TargetKey   = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto HandleCount = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryOpenSubKeys(TargetKey:{:#x}, HandleCount:{:#x})", TargetKey, HandleCount));

        for(const auto& it : d.observers_NtQueryOpenSubKeys)
            it(TargetKey, HandleCount);
    }

    static void on_NtQueryPerformanceCounter(nt32::syscalls32::Data& d)
    {
        const auto PerformanceCounter   = arg<nt32::PLARGE_INTEGER>(d.core, 0);
        const auto PerformanceFrequency = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryPerformanceCounter(PerformanceCounter:{:#x}, PerformanceFrequency:{:#x})", PerformanceCounter, PerformanceFrequency));

        for(const auto& it : d.observers_NtQueryPerformanceCounter)
            it(PerformanceCounter, PerformanceFrequency);
    }

    static void on_ZwQueryQuotaInformationFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto Buffer            = arg<nt32::PVOID>(d.core, 2);
        const auto Length            = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnSingleEntry = arg<nt32::BOOLEAN>(d.core, 4);
        const auto SidList           = arg<nt32::PVOID>(d.core, 5);
        const auto SidListLength     = arg<nt32::ULONG>(d.core, 6);
        const auto StartSid          = arg<nt32::PULONG>(d.core, 7);
        const auto RestartScan       = arg<nt32::BOOLEAN>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryQuotaInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x}, ReturnSingleEntry:{:#x}, SidList:{:#x}, SidListLength:{:#x}, StartSid:{:#x}, RestartScan:{:#x})", FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan));

        for(const auto& it : d.observers_ZwQueryQuotaInformationFile)
            it(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan);
    }

    static void on_ZwQuerySection(nt32::syscalls32::Data& d)
    {
        const auto SectionHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto SectionInformationClass  = arg<nt32::SECTION_INFORMATION_CLASS>(d.core, 1);
        const auto SectionInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto SectionInformationLength = arg<nt32::SIZE_T>(d.core, 3);
        const auto ReturnLength             = arg<nt32::PSIZE_T>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQuerySection(SectionHandle:{:#x}, SectionInformationClass:{:#x}, SectionInformation:{:#x}, SectionInformationLength:{:#x}, ReturnLength:{:#x})", SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQuerySection)
            it(SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength);
    }

    static void on_ZwQuerySecurityAttributesToken(nt32::syscalls32::Data& d)
    {
        const auto TokenHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto Attributes         = arg<nt32::PUNICODE_STRING>(d.core, 1);
        const auto NumberOfAttributes = arg<nt32::ULONG>(d.core, 2);
        const auto Buffer             = arg<nt32::PVOID>(d.core, 3);
        const auto Length             = arg<nt32::ULONG>(d.core, 4);
        const auto ReturnLength       = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQuerySecurityAttributesToken(TokenHandle:{:#x}, Attributes:{:#x}, NumberOfAttributes:{:#x}, Buffer:{:#x}, Length:{:#x}, ReturnLength:{:#x})", TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength));

        for(const auto& it : d.observers_ZwQuerySecurityAttributesToken)
            it(TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength);
    }

    static void on_NtQuerySecurityObject(nt32::syscalls32::Data& d)
    {
        const auto Handle              = arg<nt32::HANDLE>(d.core, 0);
        const auto SecurityInformation = arg<nt32::SECURITY_INFORMATION>(d.core, 1);
        const auto SecurityDescriptor  = arg<nt32::PSECURITY_DESCRIPTOR>(d.core, 2);
        const auto Length              = arg<nt32::ULONG>(d.core, 3);
        const auto LengthNeeded        = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQuerySecurityObject(Handle:{:#x}, SecurityInformation:{:#x}, SecurityDescriptor:{:#x}, Length:{:#x}, LengthNeeded:{:#x})", Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded));

        for(const auto& it : d.observers_NtQuerySecurityObject)
            it(Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded);
    }

    static void on_ZwQuerySemaphore(nt32::syscalls32::Data& d)
    {
        const auto SemaphoreHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto SemaphoreInformationClass  = arg<nt32::SEMAPHORE_INFORMATION_CLASS>(d.core, 1);
        const auto SemaphoreInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto SemaphoreInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength               = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQuerySemaphore(SemaphoreHandle:{:#x}, SemaphoreInformationClass:{:#x}, SemaphoreInformation:{:#x}, SemaphoreInformationLength:{:#x}, ReturnLength:{:#x})", SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQuerySemaphore)
            it(SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength);
    }

    static void on_ZwQuerySymbolicLinkObject(nt32::syscalls32::Data& d)
    {
        const auto LinkHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto LinkTarget     = arg<nt32::PUNICODE_STRING>(d.core, 1);
        const auto ReturnedLength = arg<nt32::PULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQuerySymbolicLinkObject(LinkHandle:{:#x}, LinkTarget:{:#x}, ReturnedLength:{:#x})", LinkHandle, LinkTarget, ReturnedLength));

        for(const auto& it : d.observers_ZwQuerySymbolicLinkObject)
            it(LinkHandle, LinkTarget, ReturnedLength);
    }

    static void on_ZwQuerySystemEnvironmentValueEx(nt32::syscalls32::Data& d)
    {
        const auto VariableName = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto VendorGuid   = arg<nt32::LPGUID>(d.core, 1);
        const auto Value        = arg<nt32::PVOID>(d.core, 2);
        const auto ValueLength  = arg<nt32::PULONG>(d.core, 3);
        const auto Attributes   = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQuerySystemEnvironmentValueEx(VariableName:{:#x}, VendorGuid:{:#x}, Value:{:#x}, ValueLength:{:#x}, Attributes:{:#x})", VariableName, VendorGuid, Value, ValueLength, Attributes));

        for(const auto& it : d.observers_ZwQuerySystemEnvironmentValueEx)
            it(VariableName, VendorGuid, Value, ValueLength, Attributes);
    }

    static void on_ZwQuerySystemEnvironmentValue(nt32::syscalls32::Data& d)
    {
        const auto VariableName  = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto VariableValue = arg<nt32::PWSTR>(d.core, 1);
        const auto ValueLength   = arg<nt32::USHORT>(d.core, 2);
        const auto ReturnLength  = arg<nt32::PUSHORT>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQuerySystemEnvironmentValue(VariableName:{:#x}, VariableValue:{:#x}, ValueLength:{:#x}, ReturnLength:{:#x})", VariableName, VariableValue, ValueLength, ReturnLength));

        for(const auto& it : d.observers_ZwQuerySystemEnvironmentValue)
            it(VariableName, VariableValue, ValueLength, ReturnLength);
    }

    static void on_ZwQuerySystemInformationEx(nt32::syscalls32::Data& d)
    {
        const auto SystemInformationClass  = arg<nt32::SYSTEM_INFORMATION_CLASS>(d.core, 0);
        const auto QueryInformation        = arg<nt32::PVOID>(d.core, 1);
        const auto QueryInformationLength  = arg<nt32::ULONG>(d.core, 2);
        const auto SystemInformation       = arg<nt32::PVOID>(d.core, 3);
        const auto SystemInformationLength = arg<nt32::ULONG>(d.core, 4);
        const auto ReturnLength            = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQuerySystemInformationEx(SystemInformationClass:{:#x}, QueryInformation:{:#x}, QueryInformationLength:{:#x}, SystemInformation:{:#x}, SystemInformationLength:{:#x}, ReturnLength:{:#x})", SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQuerySystemInformationEx)
            it(SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength);
    }

    static void on_NtQuerySystemInformation(nt32::syscalls32::Data& d)
    {
        const auto SystemInformationClass  = arg<nt32::SYSTEM_INFORMATION_CLASS>(d.core, 0);
        const auto SystemInformation       = arg<nt32::PVOID>(d.core, 1);
        const auto SystemInformationLength = arg<nt32::ULONG>(d.core, 2);
        const auto ReturnLength            = arg<nt32::PULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQuerySystemInformation(SystemInformationClass:{:#x}, SystemInformation:{:#x}, SystemInformationLength:{:#x}, ReturnLength:{:#x})", SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength));

        for(const auto& it : d.observers_NtQuerySystemInformation)
            it(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
    }

    static void on_NtQuerySystemTime(nt32::syscalls32::Data& d)
    {
        const auto SystemTime = arg<nt32::PLARGE_INTEGER>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQuerySystemTime(SystemTime:{:#x})", SystemTime));

        for(const auto& it : d.observers_NtQuerySystemTime)
            it(SystemTime);
    }

    static void on_ZwQueryTimer(nt32::syscalls32::Data& d)
    {
        const auto TimerHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto TimerInformationClass  = arg<nt32::TIMER_INFORMATION_CLASS>(d.core, 1);
        const auto TimerInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto TimerInformationLength = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength           = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryTimer(TimerHandle:{:#x}, TimerInformationClass:{:#x}, TimerInformation:{:#x}, TimerInformationLength:{:#x}, ReturnLength:{:#x})", TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwQueryTimer)
            it(TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength);
    }

    static void on_NtQueryTimerResolution(nt32::syscalls32::Data& d)
    {
        const auto MaximumTime = arg<nt32::PULONG>(d.core, 0);
        const auto MinimumTime = arg<nt32::PULONG>(d.core, 1);
        const auto CurrentTime = arg<nt32::PULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryTimerResolution(MaximumTime:{:#x}, MinimumTime:{:#x}, CurrentTime:{:#x})", MaximumTime, MinimumTime, CurrentTime));

        for(const auto& it : d.observers_NtQueryTimerResolution)
            it(MaximumTime, MinimumTime, CurrentTime);
    }

    static void on_ZwQueryValueKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle                = arg<nt32::HANDLE>(d.core, 0);
        const auto ValueName                = arg<nt32::PUNICODE_STRING>(d.core, 1);
        const auto KeyValueInformationClass = arg<nt32::KEY_VALUE_INFORMATION_CLASS>(d.core, 2);
        const auto KeyValueInformation      = arg<nt32::PVOID>(d.core, 3);
        const auto Length                   = arg<nt32::ULONG>(d.core, 4);
        const auto ResultLength             = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryValueKey(KeyHandle:{:#x}, ValueName:{:#x}, KeyValueInformationClass:{:#x}, KeyValueInformation:{:#x}, Length:{:#x}, ResultLength:{:#x})", KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength));

        for(const auto& it : d.observers_ZwQueryValueKey)
            it(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    }

    static void on_NtQueryVirtualMemory(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto BaseAddress             = arg<nt32::PVOID>(d.core, 1);
        const auto MemoryInformationClass  = arg<nt32::MEMORY_INFORMATION_CLASS>(d.core, 2);
        const auto MemoryInformation       = arg<nt32::PVOID>(d.core, 3);
        const auto MemoryInformationLength = arg<nt32::SIZE_T>(d.core, 4);
        const auto ReturnLength            = arg<nt32::PSIZE_T>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryVirtualMemory(ProcessHandle:{:#x}, BaseAddress:{:#x}, MemoryInformationClass:{:#x}, MemoryInformation:{:#x}, MemoryInformationLength:{:#x}, ReturnLength:{:#x})", ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength));

        for(const auto& it : d.observers_NtQueryVirtualMemory)
            it(ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
    }

    static void on_NtQueryVolumeInformationFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle         = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto FsInformation      = arg<nt32::PVOID>(d.core, 2);
        const auto Length             = arg<nt32::ULONG>(d.core, 3);
        const auto FsInformationClass = arg<nt32::FS_INFORMATION_CLASS>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueryVolumeInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, FsInformation:{:#x}, Length:{:#x}, FsInformationClass:{:#x})", FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass));

        for(const auto& it : d.observers_NtQueryVolumeInformationFile)
            it(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    }

    static void on_NtQueueApcThreadEx(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle         = arg<nt32::HANDLE>(d.core, 0);
        const auto UserApcReserveHandle = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine           = arg<nt32::PPS_APC_ROUTINE>(d.core, 2);
        const auto ApcArgument1         = arg<nt32::PVOID>(d.core, 3);
        const auto ApcArgument2         = arg<nt32::PVOID>(d.core, 4);
        const auto ApcArgument3         = arg<nt32::PVOID>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueueApcThreadEx(ThreadHandle:{:#x}, UserApcReserveHandle:{:#x}, ApcRoutine:{:#x}, ApcArgument1:{:#x}, ApcArgument2:{:#x}, ApcArgument3:{:#x})", ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3));

        for(const auto& it : d.observers_NtQueueApcThreadEx)
            it(ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    }

    static void on_NtQueueApcThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto ApcRoutine   = arg<nt32::PPS_APC_ROUTINE>(d.core, 1);
        const auto ApcArgument1 = arg<nt32::PVOID>(d.core, 2);
        const auto ApcArgument2 = arg<nt32::PVOID>(d.core, 3);
        const auto ApcArgument3 = arg<nt32::PVOID>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtQueueApcThread(ThreadHandle:{:#x}, ApcRoutine:{:#x}, ApcArgument1:{:#x}, ApcArgument2:{:#x}, ApcArgument3:{:#x})", ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3));

        for(const auto& it : d.observers_NtQueueApcThread)
            it(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    }

    static void on_ZwRaiseException(nt32::syscalls32::Data& d)
    {
        const auto ExceptionRecord = arg<nt32::PEXCEPTION_RECORD>(d.core, 0);
        const auto ContextRecord   = arg<nt32::PCONTEXT>(d.core, 1);
        const auto FirstChance     = arg<nt32::BOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRaiseException(ExceptionRecord:{:#x}, ContextRecord:{:#x}, FirstChance:{:#x})", ExceptionRecord, ContextRecord, FirstChance));

        for(const auto& it : d.observers_ZwRaiseException)
            it(ExceptionRecord, ContextRecord, FirstChance);
    }

    static void on_ZwRaiseHardError(nt32::syscalls32::Data& d)
    {
        const auto ErrorStatus                = arg<nt32::NTSTATUS>(d.core, 0);
        const auto NumberOfParameters         = arg<nt32::ULONG>(d.core, 1);
        const auto UnicodeStringParameterMask = arg<nt32::ULONG>(d.core, 2);
        const auto Parameters                 = arg<nt32::PULONG_PTR>(d.core, 3);
        const auto ValidResponseOptions       = arg<nt32::ULONG>(d.core, 4);
        const auto Response                   = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRaiseHardError(ErrorStatus:{:#x}, NumberOfParameters:{:#x}, UnicodeStringParameterMask:{:#x}, Parameters:{:#x}, ValidResponseOptions:{:#x}, Response:{:#x})", ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response));

        for(const auto& it : d.observers_ZwRaiseHardError)
            it(ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);
    }

    static void on_NtReadFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto Event         = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine    = arg<nt32::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext    = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(d.core, 4);
        const auto Buffer        = arg<nt32::PVOID>(d.core, 5);
        const auto Length        = arg<nt32::ULONG>(d.core, 6);
        const auto ByteOffset    = arg<nt32::PLARGE_INTEGER>(d.core, 7);
        const auto Key           = arg<nt32::PULONG>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtReadFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x}, ByteOffset:{:#x}, Key:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key));

        for(const auto& it : d.observers_NtReadFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    }

    static void on_NtReadFileScatter(nt32::syscalls32::Data& d)
    {
        const auto FileHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto Event         = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine    = arg<nt32::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext    = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(d.core, 4);
        const auto SegmentArray  = arg<nt32::PFILE_SEGMENT_ELEMENT>(d.core, 5);
        const auto Length        = arg<nt32::ULONG>(d.core, 6);
        const auto ByteOffset    = arg<nt32::PLARGE_INTEGER>(d.core, 7);
        const auto Key           = arg<nt32::PULONG>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtReadFileScatter(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, SegmentArray:{:#x}, Length:{:#x}, ByteOffset:{:#x}, Key:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key));

        for(const auto& it : d.observers_NtReadFileScatter)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    }

    static void on_ZwReadOnlyEnlistment(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwReadOnlyEnlistment(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock));

        for(const auto& it : d.observers_ZwReadOnlyEnlistment)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_ZwReadRequestData(nt32::syscalls32::Data& d)
    {
        const auto PortHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto Message           = arg<nt32::PPORT_MESSAGE>(d.core, 1);
        const auto DataEntryIndex    = arg<nt32::ULONG>(d.core, 2);
        const auto Buffer            = arg<nt32::PVOID>(d.core, 3);
        const auto BufferSize        = arg<nt32::SIZE_T>(d.core, 4);
        const auto NumberOfBytesRead = arg<nt32::PSIZE_T>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwReadRequestData(PortHandle:{:#x}, Message:{:#x}, DataEntryIndex:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, NumberOfBytesRead:{:#x})", PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead));

        for(const auto& it : d.observers_ZwReadRequestData)
            it(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead);
    }

    static void on_NtReadVirtualMemory(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto BaseAddress       = arg<nt32::PVOID>(d.core, 1);
        const auto Buffer            = arg<nt32::PVOID>(d.core, 2);
        const auto BufferSize        = arg<nt32::SIZE_T>(d.core, 3);
        const auto NumberOfBytesRead = arg<nt32::PSIZE_T>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtReadVirtualMemory(ProcessHandle:{:#x}, BaseAddress:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, NumberOfBytesRead:{:#x})", ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead));

        for(const auto& it : d.observers_NtReadVirtualMemory)
            it(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead);
    }

    static void on_NtRecoverEnlistment(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto EnlistmentKey    = arg<nt32::PVOID>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtRecoverEnlistment(EnlistmentHandle:{:#x}, EnlistmentKey:{:#x})", EnlistmentHandle, EnlistmentKey));

        for(const auto& it : d.observers_NtRecoverEnlistment)
            it(EnlistmentHandle, EnlistmentKey);
    }

    static void on_NtRecoverResourceManager(nt32::syscalls32::Data& d)
    {
        const auto ResourceManagerHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtRecoverResourceManager(ResourceManagerHandle:{:#x})", ResourceManagerHandle));

        for(const auto& it : d.observers_NtRecoverResourceManager)
            it(ResourceManagerHandle);
    }

    static void on_ZwRecoverTransactionManager(nt32::syscalls32::Data& d)
    {
        const auto TransactionManagerHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRecoverTransactionManager(TransactionManagerHandle:{:#x})", TransactionManagerHandle));

        for(const auto& it : d.observers_ZwRecoverTransactionManager)
            it(TransactionManagerHandle);
    }

    static void on_ZwRegisterProtocolAddressInformation(nt32::syscalls32::Data& d)
    {
        const auto ResourceManager         = arg<nt32::HANDLE>(d.core, 0);
        const auto ProtocolId              = arg<nt32::PCRM_PROTOCOL_ID>(d.core, 1);
        const auto ProtocolInformationSize = arg<nt32::ULONG>(d.core, 2);
        const auto ProtocolInformation     = arg<nt32::PVOID>(d.core, 3);
        const auto CreateOptions           = arg<nt32::ULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRegisterProtocolAddressInformation(ResourceManager:{:#x}, ProtocolId:{:#x}, ProtocolInformationSize:{:#x}, ProtocolInformation:{:#x}, CreateOptions:{:#x})", ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions));

        for(const auto& it : d.observers_ZwRegisterProtocolAddressInformation)
            it(ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions);
    }

    static void on_ZwRegisterThreadTerminatePort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRegisterThreadTerminatePort(PortHandle:{:#x})", PortHandle));

        for(const auto& it : d.observers_ZwRegisterThreadTerminatePort)
            it(PortHandle);
    }

    static void on_NtReleaseKeyedEvent(nt32::syscalls32::Data& d)
    {
        const auto KeyedEventHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto KeyValue         = arg<nt32::PVOID>(d.core, 1);
        const auto Alertable        = arg<nt32::BOOLEAN>(d.core, 2);
        const auto Timeout          = arg<nt32::PLARGE_INTEGER>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtReleaseKeyedEvent(KeyedEventHandle:{:#x}, KeyValue:{:#x}, Alertable:{:#x}, Timeout:{:#x})", KeyedEventHandle, KeyValue, Alertable, Timeout));

        for(const auto& it : d.observers_NtReleaseKeyedEvent)
            it(KeyedEventHandle, KeyValue, Alertable, Timeout);
    }

    static void on_ZwReleaseMutant(nt32::syscalls32::Data& d)
    {
        const auto MutantHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto PreviousCount = arg<nt32::PLONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwReleaseMutant(MutantHandle:{:#x}, PreviousCount:{:#x})", MutantHandle, PreviousCount));

        for(const auto& it : d.observers_ZwReleaseMutant)
            it(MutantHandle, PreviousCount);
    }

    static void on_NtReleaseSemaphore(nt32::syscalls32::Data& d)
    {
        const auto SemaphoreHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto ReleaseCount    = arg<nt32::LONG>(d.core, 1);
        const auto PreviousCount   = arg<nt32::PLONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtReleaseSemaphore(SemaphoreHandle:{:#x}, ReleaseCount:{:#x}, PreviousCount:{:#x})", SemaphoreHandle, ReleaseCount, PreviousCount));

        for(const auto& it : d.observers_NtReleaseSemaphore)
            it(SemaphoreHandle, ReleaseCount, PreviousCount);
    }

    static void on_ZwReleaseWorkerFactoryWorker(nt32::syscalls32::Data& d)
    {
        const auto WorkerFactoryHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwReleaseWorkerFactoryWorker(WorkerFactoryHandle:{:#x})", WorkerFactoryHandle));

        for(const auto& it : d.observers_ZwReleaseWorkerFactoryWorker)
            it(WorkerFactoryHandle);
    }

    static void on_ZwRemoveIoCompletionEx(nt32::syscalls32::Data& d)
    {
        const auto IoCompletionHandle      = arg<nt32::HANDLE>(d.core, 0);
        const auto IoCompletionInformation = arg<nt32::PFILE_IO_COMPLETION_INFORMATION>(d.core, 1);
        const auto Count                   = arg<nt32::ULONG>(d.core, 2);
        const auto NumEntriesRemoved       = arg<nt32::PULONG>(d.core, 3);
        const auto Timeout                 = arg<nt32::PLARGE_INTEGER>(d.core, 4);
        const auto Alertable               = arg<nt32::BOOLEAN>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRemoveIoCompletionEx(IoCompletionHandle:{:#x}, IoCompletionInformation:{:#x}, Count:{:#x}, NumEntriesRemoved:{:#x}, Timeout:{:#x}, Alertable:{:#x})", IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable));

        for(const auto& it : d.observers_ZwRemoveIoCompletionEx)
            it(IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable);
    }

    static void on_ZwRemoveIoCompletion(nt32::syscalls32::Data& d)
    {
        const auto IoCompletionHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto STARKeyContext     = arg<nt32::PVOID>(d.core, 1);
        const auto STARApcContext     = arg<nt32::PVOID>(d.core, 2);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(d.core, 3);
        const auto Timeout            = arg<nt32::PLARGE_INTEGER>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRemoveIoCompletion(IoCompletionHandle:{:#x}, STARKeyContext:{:#x}, STARApcContext:{:#x}, IoStatusBlock:{:#x}, Timeout:{:#x})", IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout));

        for(const auto& it : d.observers_ZwRemoveIoCompletion)
            it(IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout);
    }

    static void on_ZwRemoveProcessDebug(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto DebugObjectHandle = arg<nt32::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRemoveProcessDebug(ProcessHandle:{:#x}, DebugObjectHandle:{:#x})", ProcessHandle, DebugObjectHandle));

        for(const auto& it : d.observers_ZwRemoveProcessDebug)
            it(ProcessHandle, DebugObjectHandle);
    }

    static void on_ZwRenameKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto NewName   = arg<nt32::PUNICODE_STRING>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRenameKey(KeyHandle:{:#x}, NewName:{:#x})", KeyHandle, NewName));

        for(const auto& it : d.observers_ZwRenameKey)
            it(KeyHandle, NewName);
    }

    static void on_NtRenameTransactionManager(nt32::syscalls32::Data& d)
    {
        const auto LogFileName                    = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto ExistingTransactionManagerGuid = arg<nt32::LPGUID>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtRenameTransactionManager(LogFileName:{:#x}, ExistingTransactionManagerGuid:{:#x})", LogFileName, ExistingTransactionManagerGuid));

        for(const auto& it : d.observers_NtRenameTransactionManager)
            it(LogFileName, ExistingTransactionManagerGuid);
    }

    static void on_ZwReplaceKey(nt32::syscalls32::Data& d)
    {
        const auto NewFile      = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto TargetHandle = arg<nt32::HANDLE>(d.core, 1);
        const auto OldFile      = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwReplaceKey(NewFile:{:#x}, TargetHandle:{:#x}, OldFile:{:#x})", NewFile, TargetHandle, OldFile));

        for(const auto& it : d.observers_ZwReplaceKey)
            it(NewFile, TargetHandle, OldFile);
    }

    static void on_NtReplacePartitionUnit(nt32::syscalls32::Data& d)
    {
        const auto TargetInstancePath = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto SpareInstancePath  = arg<nt32::PUNICODE_STRING>(d.core, 1);
        const auto Flags              = arg<nt32::ULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtReplacePartitionUnit(TargetInstancePath:{:#x}, SpareInstancePath:{:#x}, Flags:{:#x})", TargetInstancePath, SpareInstancePath, Flags));

        for(const auto& it : d.observers_NtReplacePartitionUnit)
            it(TargetInstancePath, SpareInstancePath, Flags);
    }

    static void on_ZwReplyPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto ReplyMessage = arg<nt32::PPORT_MESSAGE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwReplyPort(PortHandle:{:#x}, ReplyMessage:{:#x})", PortHandle, ReplyMessage));

        for(const auto& it : d.observers_ZwReplyPort)
            it(PortHandle, ReplyMessage);
    }

    static void on_NtReplyWaitReceivePortEx(nt32::syscalls32::Data& d)
    {
        const auto PortHandle      = arg<nt32::HANDLE>(d.core, 0);
        const auto STARPortContext = arg<nt32::PVOID>(d.core, 1);
        const auto ReplyMessage    = arg<nt32::PPORT_MESSAGE>(d.core, 2);
        const auto ReceiveMessage  = arg<nt32::PPORT_MESSAGE>(d.core, 3);
        const auto Timeout         = arg<nt32::PLARGE_INTEGER>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtReplyWaitReceivePortEx(PortHandle:{:#x}, STARPortContext:{:#x}, ReplyMessage:{:#x}, ReceiveMessage:{:#x}, Timeout:{:#x})", PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout));

        for(const auto& it : d.observers_NtReplyWaitReceivePortEx)
            it(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout);
    }

    static void on_NtReplyWaitReceivePort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle      = arg<nt32::HANDLE>(d.core, 0);
        const auto STARPortContext = arg<nt32::PVOID>(d.core, 1);
        const auto ReplyMessage    = arg<nt32::PPORT_MESSAGE>(d.core, 2);
        const auto ReceiveMessage  = arg<nt32::PPORT_MESSAGE>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtReplyWaitReceivePort(PortHandle:{:#x}, STARPortContext:{:#x}, ReplyMessage:{:#x}, ReceiveMessage:{:#x})", PortHandle, STARPortContext, ReplyMessage, ReceiveMessage));

        for(const auto& it : d.observers_NtReplyWaitReceivePort)
            it(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage);
    }

    static void on_NtReplyWaitReplyPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto ReplyMessage = arg<nt32::PPORT_MESSAGE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtReplyWaitReplyPort(PortHandle:{:#x}, ReplyMessage:{:#x})", PortHandle, ReplyMessage));

        for(const auto& it : d.observers_NtReplyWaitReplyPort)
            it(PortHandle, ReplyMessage);
    }

    static void on_ZwRequestPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto RequestMessage = arg<nt32::PPORT_MESSAGE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRequestPort(PortHandle:{:#x}, RequestMessage:{:#x})", PortHandle, RequestMessage));

        for(const auto& it : d.observers_ZwRequestPort)
            it(PortHandle, RequestMessage);
    }

    static void on_NtRequestWaitReplyPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto RequestMessage = arg<nt32::PPORT_MESSAGE>(d.core, 1);
        const auto ReplyMessage   = arg<nt32::PPORT_MESSAGE>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtRequestWaitReplyPort(PortHandle:{:#x}, RequestMessage:{:#x}, ReplyMessage:{:#x})", PortHandle, RequestMessage, ReplyMessage));

        for(const auto& it : d.observers_NtRequestWaitReplyPort)
            it(PortHandle, RequestMessage, ReplyMessage);
    }

    static void on_NtResetEvent(nt32::syscalls32::Data& d)
    {
        const auto EventHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto PreviousState = arg<nt32::PLONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtResetEvent(EventHandle:{:#x}, PreviousState:{:#x})", EventHandle, PreviousState));

        for(const auto& it : d.observers_NtResetEvent)
            it(EventHandle, PreviousState);
    }

    static void on_ZwResetWriteWatch(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto BaseAddress   = arg<nt32::PVOID>(d.core, 1);
        const auto RegionSize    = arg<nt32::SIZE_T>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwResetWriteWatch(ProcessHandle:{:#x}, BaseAddress:{:#x}, RegionSize:{:#x})", ProcessHandle, BaseAddress, RegionSize));

        for(const auto& it : d.observers_ZwResetWriteWatch)
            it(ProcessHandle, BaseAddress, RegionSize);
    }

    static void on_NtRestoreKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto FileHandle = arg<nt32::HANDLE>(d.core, 1);
        const auto Flags      = arg<nt32::ULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtRestoreKey(KeyHandle:{:#x}, FileHandle:{:#x}, Flags:{:#x})", KeyHandle, FileHandle, Flags));

        for(const auto& it : d.observers_NtRestoreKey)
            it(KeyHandle, FileHandle, Flags);
    }

    static void on_ZwResumeProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwResumeProcess(ProcessHandle:{:#x})", ProcessHandle));

        for(const auto& it : d.observers_ZwResumeProcess)
            it(ProcessHandle);
    }

    static void on_ZwResumeThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle         = arg<nt32::HANDLE>(d.core, 0);
        const auto PreviousSuspendCount = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwResumeThread(ThreadHandle:{:#x}, PreviousSuspendCount:{:#x})", ThreadHandle, PreviousSuspendCount));

        for(const auto& it : d.observers_ZwResumeThread)
            it(ThreadHandle, PreviousSuspendCount);
    }

    static void on_ZwRollbackComplete(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwRollbackComplete(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock));

        for(const auto& it : d.observers_ZwRollbackComplete)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtRollbackEnlistment(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtRollbackEnlistment(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock));

        for(const auto& it : d.observers_NtRollbackEnlistment)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtRollbackTransaction(nt32::syscalls32::Data& d)
    {
        const auto TransactionHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto Wait              = arg<nt32::BOOLEAN>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtRollbackTransaction(TransactionHandle:{:#x}, Wait:{:#x})", TransactionHandle, Wait));

        for(const auto& it : d.observers_NtRollbackTransaction)
            it(TransactionHandle, Wait);
    }

    static void on_NtRollforwardTransactionManager(nt32::syscalls32::Data& d)
    {
        const auto TransactionManagerHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock           = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtRollforwardTransactionManager(TransactionManagerHandle:{:#x}, TmVirtualClock:{:#x})", TransactionManagerHandle, TmVirtualClock));

        for(const auto& it : d.observers_NtRollforwardTransactionManager)
            it(TransactionManagerHandle, TmVirtualClock);
    }

    static void on_NtSaveKeyEx(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto FileHandle = arg<nt32::HANDLE>(d.core, 1);
        const auto Format     = arg<nt32::ULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSaveKeyEx(KeyHandle:{:#x}, FileHandle:{:#x}, Format:{:#x})", KeyHandle, FileHandle, Format));

        for(const auto& it : d.observers_NtSaveKeyEx)
            it(KeyHandle, FileHandle, Format);
    }

    static void on_NtSaveKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto FileHandle = arg<nt32::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSaveKey(KeyHandle:{:#x}, FileHandle:{:#x})", KeyHandle, FileHandle));

        for(const auto& it : d.observers_NtSaveKey)
            it(KeyHandle, FileHandle);
    }

    static void on_NtSaveMergedKeys(nt32::syscalls32::Data& d)
    {
        const auto HighPrecedenceKeyHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto LowPrecedenceKeyHandle  = arg<nt32::HANDLE>(d.core, 1);
        const auto FileHandle              = arg<nt32::HANDLE>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSaveMergedKeys(HighPrecedenceKeyHandle:{:#x}, LowPrecedenceKeyHandle:{:#x}, FileHandle:{:#x})", HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle));

        for(const auto& it : d.observers_NtSaveMergedKeys)
            it(HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);
    }

    static void on_NtSecureConnectPort(nt32::syscalls32::Data& d)
    {
        const auto PortHandle                  = arg<nt32::PHANDLE>(d.core, 0);
        const auto PortName                    = arg<nt32::PUNICODE_STRING>(d.core, 1);
        const auto SecurityQos                 = arg<nt32::PSECURITY_QUALITY_OF_SERVICE>(d.core, 2);
        const auto ClientView                  = arg<nt32::PPORT_VIEW>(d.core, 3);
        const auto RequiredServerSid           = arg<nt32::PSID>(d.core, 4);
        const auto ServerView                  = arg<nt32::PREMOTE_PORT_VIEW>(d.core, 5);
        const auto MaxMessageLength            = arg<nt32::PULONG>(d.core, 6);
        const auto ConnectionInformation       = arg<nt32::PVOID>(d.core, 7);
        const auto ConnectionInformationLength = arg<nt32::PULONG>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSecureConnectPort(PortHandle:{:#x}, PortName:{:#x}, SecurityQos:{:#x}, ClientView:{:#x}, RequiredServerSid:{:#x}, ServerView:{:#x}, MaxMessageLength:{:#x}, ConnectionInformation:{:#x}, ConnectionInformationLength:{:#x})", PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength));

        for(const auto& it : d.observers_NtSecureConnectPort)
            it(PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    }

    static void on_ZwSetBootEntryOrder(nt32::syscalls32::Data& d)
    {
        const auto Ids   = arg<nt32::PULONG>(d.core, 0);
        const auto Count = arg<nt32::ULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetBootEntryOrder(Ids:{:#x}, Count:{:#x})", Ids, Count));

        for(const auto& it : d.observers_ZwSetBootEntryOrder)
            it(Ids, Count);
    }

    static void on_ZwSetBootOptions(nt32::syscalls32::Data& d)
    {
        const auto BootOptions    = arg<nt32::PBOOT_OPTIONS>(d.core, 0);
        const auto FieldsToChange = arg<nt32::ULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetBootOptions(BootOptions:{:#x}, FieldsToChange:{:#x})", BootOptions, FieldsToChange));

        for(const auto& it : d.observers_ZwSetBootOptions)
            it(BootOptions, FieldsToChange);
    }

    static void on_ZwSetContextThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto ThreadContext = arg<nt32::PCONTEXT>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetContextThread(ThreadHandle:{:#x}, ThreadContext:{:#x})", ThreadHandle, ThreadContext));

        for(const auto& it : d.observers_ZwSetContextThread)
            it(ThreadHandle, ThreadContext);
    }

    static void on_NtSetDebugFilterState(nt32::syscalls32::Data& d)
    {
        const auto ComponentId = arg<nt32::ULONG>(d.core, 0);
        const auto Level       = arg<nt32::ULONG>(d.core, 1);
        const auto State       = arg<nt32::BOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetDebugFilterState(ComponentId:{:#x}, Level:{:#x}, State:{:#x})", ComponentId, Level, State));

        for(const auto& it : d.observers_NtSetDebugFilterState)
            it(ComponentId, Level, State);
    }

    static void on_NtSetDefaultHardErrorPort(nt32::syscalls32::Data& d)
    {
        const auto DefaultHardErrorPort = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetDefaultHardErrorPort(DefaultHardErrorPort:{:#x})", DefaultHardErrorPort));

        for(const auto& it : d.observers_NtSetDefaultHardErrorPort)
            it(DefaultHardErrorPort);
    }

    static void on_NtSetDefaultLocale(nt32::syscalls32::Data& d)
    {
        const auto UserProfile     = arg<nt32::BOOLEAN>(d.core, 0);
        const auto DefaultLocaleId = arg<nt32::LCID>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetDefaultLocale(UserProfile:{:#x}, DefaultLocaleId:{:#x})", UserProfile, DefaultLocaleId));

        for(const auto& it : d.observers_NtSetDefaultLocale)
            it(UserProfile, DefaultLocaleId);
    }

    static void on_ZwSetDefaultUILanguage(nt32::syscalls32::Data& d)
    {
        const auto DefaultUILanguageId = arg<nt32::LANGID>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetDefaultUILanguage(DefaultUILanguageId:{:#x})", DefaultUILanguageId));

        for(const auto& it : d.observers_ZwSetDefaultUILanguage)
            it(DefaultUILanguageId);
    }

    static void on_NtSetDriverEntryOrder(nt32::syscalls32::Data& d)
    {
        const auto Ids   = arg<nt32::PULONG>(d.core, 0);
        const auto Count = arg<nt32::ULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetDriverEntryOrder(Ids:{:#x}, Count:{:#x})", Ids, Count));

        for(const auto& it : d.observers_NtSetDriverEntryOrder)
            it(Ids, Count);
    }

    static void on_ZwSetEaFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto Buffer        = arg<nt32::PVOID>(d.core, 2);
        const auto Length        = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetEaFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x})", FileHandle, IoStatusBlock, Buffer, Length));

        for(const auto& it : d.observers_ZwSetEaFile)
            it(FileHandle, IoStatusBlock, Buffer, Length);
    }

    static void on_NtSetEventBoostPriority(nt32::syscalls32::Data& d)
    {
        const auto EventHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetEventBoostPriority(EventHandle:{:#x})", EventHandle));

        for(const auto& it : d.observers_NtSetEventBoostPriority)
            it(EventHandle);
    }

    static void on_NtSetEvent(nt32::syscalls32::Data& d)
    {
        const auto EventHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto PreviousState = arg<nt32::PLONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetEvent(EventHandle:{:#x}, PreviousState:{:#x})", EventHandle, PreviousState));

        for(const auto& it : d.observers_NtSetEvent)
            it(EventHandle, PreviousState);
    }

    static void on_NtSetHighEventPair(nt32::syscalls32::Data& d)
    {
        const auto EventPairHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetHighEventPair(EventPairHandle:{:#x})", EventPairHandle));

        for(const auto& it : d.observers_NtSetHighEventPair)
            it(EventPairHandle);
    }

    static void on_NtSetHighWaitLowEventPair(nt32::syscalls32::Data& d)
    {
        const auto EventPairHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetHighWaitLowEventPair(EventPairHandle:{:#x})", EventPairHandle));

        for(const auto& it : d.observers_NtSetHighWaitLowEventPair)
            it(EventPairHandle);
    }

    static void on_ZwSetInformationDebugObject(nt32::syscalls32::Data& d)
    {
        const auto DebugObjectHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto DebugObjectInformationClass = arg<nt32::DEBUGOBJECTINFOCLASS>(d.core, 1);
        const auto DebugInformation            = arg<nt32::PVOID>(d.core, 2);
        const auto DebugInformationLength      = arg<nt32::ULONG>(d.core, 3);
        const auto ReturnLength                = arg<nt32::PULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationDebugObject(DebugObjectHandle:{:#x}, DebugObjectInformationClass:{:#x}, DebugInformation:{:#x}, DebugInformationLength:{:#x}, ReturnLength:{:#x})", DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength));

        for(const auto& it : d.observers_ZwSetInformationDebugObject)
            it(DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength);
    }

    static void on_NtSetInformationEnlistment(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto EnlistmentInformationClass  = arg<nt32::ENLISTMENT_INFORMATION_CLASS>(d.core, 1);
        const auto EnlistmentInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto EnlistmentInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetInformationEnlistment(EnlistmentHandle:{:#x}, EnlistmentInformationClass:{:#x}, EnlistmentInformation:{:#x}, EnlistmentInformationLength:{:#x})", EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength));

        for(const auto& it : d.observers_NtSetInformationEnlistment)
            it(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength);
    }

    static void on_ZwSetInformationFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock        = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto FileInformation      = arg<nt32::PVOID>(d.core, 2);
        const auto Length               = arg<nt32::ULONG>(d.core, 3);
        const auto FileInformationClass = arg<nt32::FILE_INFORMATION_CLASS>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, FileInformation:{:#x}, Length:{:#x}, FileInformationClass:{:#x})", FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass));

        for(const auto& it : d.observers_ZwSetInformationFile)
            it(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    }

    static void on_ZwSetInformationJobObject(nt32::syscalls32::Data& d)
    {
        const auto JobHandle                  = arg<nt32::HANDLE>(d.core, 0);
        const auto JobObjectInformationClass  = arg<nt32::JOBOBJECTINFOCLASS>(d.core, 1);
        const auto JobObjectInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto JobObjectInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationJobObject(JobHandle:{:#x}, JobObjectInformationClass:{:#x}, JobObjectInformation:{:#x}, JobObjectInformationLength:{:#x})", JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength));

        for(const auto& it : d.observers_ZwSetInformationJobObject)
            it(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength);
    }

    static void on_ZwSetInformationKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle               = arg<nt32::HANDLE>(d.core, 0);
        const auto KeySetInformationClass  = arg<nt32::KEY_SET_INFORMATION_CLASS>(d.core, 1);
        const auto KeySetInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto KeySetInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationKey(KeyHandle:{:#x}, KeySetInformationClass:{:#x}, KeySetInformation:{:#x}, KeySetInformationLength:{:#x})", KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength));

        for(const auto& it : d.observers_ZwSetInformationKey)
            it(KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength);
    }

    static void on_ZwSetInformationObject(nt32::syscalls32::Data& d)
    {
        const auto Handle                  = arg<nt32::HANDLE>(d.core, 0);
        const auto ObjectInformationClass  = arg<nt32::OBJECT_INFORMATION_CLASS>(d.core, 1);
        const auto ObjectInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto ObjectInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationObject(Handle:{:#x}, ObjectInformationClass:{:#x}, ObjectInformation:{:#x}, ObjectInformationLength:{:#x})", Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength));

        for(const auto& it : d.observers_ZwSetInformationObject)
            it(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);
    }

    static void on_ZwSetInformationProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto ProcessInformationClass  = arg<nt32::PROCESSINFOCLASS>(d.core, 1);
        const auto ProcessInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto ProcessInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationProcess(ProcessHandle:{:#x}, ProcessInformationClass:{:#x}, ProcessInformation:{:#x}, ProcessInformationLength:{:#x})", ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength));

        for(const auto& it : d.observers_ZwSetInformationProcess)
            it(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
    }

    static void on_ZwSetInformationResourceManager(nt32::syscalls32::Data& d)
    {
        const auto ResourceManagerHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto ResourceManagerInformationClass  = arg<nt32::RESOURCEMANAGER_INFORMATION_CLASS>(d.core, 1);
        const auto ResourceManagerInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto ResourceManagerInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationResourceManager(ResourceManagerHandle:{:#x}, ResourceManagerInformationClass:{:#x}, ResourceManagerInformation:{:#x}, ResourceManagerInformationLength:{:#x})", ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength));

        for(const auto& it : d.observers_ZwSetInformationResourceManager)
            it(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength);
    }

    static void on_ZwSetInformationThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto ThreadInformationClass  = arg<nt32::THREADINFOCLASS>(d.core, 1);
        const auto ThreadInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto ThreadInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationThread(ThreadHandle:{:#x}, ThreadInformationClass:{:#x}, ThreadInformation:{:#x}, ThreadInformationLength:{:#x})", ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength));

        for(const auto& it : d.observers_ZwSetInformationThread)
            it(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
    }

    static void on_ZwSetInformationToken(nt32::syscalls32::Data& d)
    {
        const auto TokenHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto TokenInformationClass  = arg<nt32::TOKEN_INFORMATION_CLASS>(d.core, 1);
        const auto TokenInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto TokenInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationToken(TokenHandle:{:#x}, TokenInformationClass:{:#x}, TokenInformation:{:#x}, TokenInformationLength:{:#x})", TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength));

        for(const auto& it : d.observers_ZwSetInformationToken)
            it(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);
    }

    static void on_ZwSetInformationTransaction(nt32::syscalls32::Data& d)
    {
        const auto TransactionHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto TransactionInformationClass  = arg<nt32::TRANSACTION_INFORMATION_CLASS>(d.core, 1);
        const auto TransactionInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto TransactionInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationTransaction(TransactionHandle:{:#x}, TransactionInformationClass:{:#x}, TransactionInformation:{:#x}, TransactionInformationLength:{:#x})", TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength));

        for(const auto& it : d.observers_ZwSetInformationTransaction)
            it(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength);
    }

    static void on_ZwSetInformationTransactionManager(nt32::syscalls32::Data& d)
    {
        const auto TmHandle                            = arg<nt32::HANDLE>(d.core, 0);
        const auto TransactionManagerInformationClass  = arg<nt32::TRANSACTIONMANAGER_INFORMATION_CLASS>(d.core, 1);
        const auto TransactionManagerInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto TransactionManagerInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationTransactionManager(TmHandle:{:#x}, TransactionManagerInformationClass:{:#x}, TransactionManagerInformation:{:#x}, TransactionManagerInformationLength:{:#x})", TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength));

        for(const auto& it : d.observers_ZwSetInformationTransactionManager)
            it(TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength);
    }

    static void on_ZwSetInformationWorkerFactory(nt32::syscalls32::Data& d)
    {
        const auto WorkerFactoryHandle            = arg<nt32::HANDLE>(d.core, 0);
        const auto WorkerFactoryInformationClass  = arg<nt32::WORKERFACTORYINFOCLASS>(d.core, 1);
        const auto WorkerFactoryInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto WorkerFactoryInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetInformationWorkerFactory(WorkerFactoryHandle:{:#x}, WorkerFactoryInformationClass:{:#x}, WorkerFactoryInformation:{:#x}, WorkerFactoryInformationLength:{:#x})", WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength));

        for(const auto& it : d.observers_ZwSetInformationWorkerFactory)
            it(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength);
    }

    static void on_NtSetIntervalProfile(nt32::syscalls32::Data& d)
    {
        const auto Interval = arg<nt32::ULONG>(d.core, 0);
        const auto Source   = arg<nt32::KPROFILE_SOURCE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetIntervalProfile(Interval:{:#x}, Source:{:#x})", Interval, Source));

        for(const auto& it : d.observers_NtSetIntervalProfile)
            it(Interval, Source);
    }

    static void on_NtSetIoCompletionEx(nt32::syscalls32::Data& d)
    {
        const auto IoCompletionHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto IoCompletionReserveHandle = arg<nt32::HANDLE>(d.core, 1);
        const auto KeyContext                = arg<nt32::PVOID>(d.core, 2);
        const auto ApcContext                = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatus                  = arg<nt32::NTSTATUS>(d.core, 4);
        const auto IoStatusInformation       = arg<nt32::ULONG_PTR>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetIoCompletionEx(IoCompletionHandle:{:#x}, IoCompletionReserveHandle:{:#x}, KeyContext:{:#x}, ApcContext:{:#x}, IoStatus:{:#x}, IoStatusInformation:{:#x})", IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation));

        for(const auto& it : d.observers_NtSetIoCompletionEx)
            it(IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    }

    static void on_NtSetIoCompletion(nt32::syscalls32::Data& d)
    {
        const auto IoCompletionHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto KeyContext          = arg<nt32::PVOID>(d.core, 1);
        const auto ApcContext          = arg<nt32::PVOID>(d.core, 2);
        const auto IoStatus            = arg<nt32::NTSTATUS>(d.core, 3);
        const auto IoStatusInformation = arg<nt32::ULONG_PTR>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetIoCompletion(IoCompletionHandle:{:#x}, KeyContext:{:#x}, ApcContext:{:#x}, IoStatus:{:#x}, IoStatusInformation:{:#x})", IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation));

        for(const auto& it : d.observers_NtSetIoCompletion)
            it(IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    }

    static void on_ZwSetLdtEntries(nt32::syscalls32::Data& d)
    {
        const auto Selector0 = arg<nt32::ULONG>(d.core, 0);
        const auto Entry0Low = arg<nt32::ULONG>(d.core, 1);
        const auto Entry0Hi  = arg<nt32::ULONG>(d.core, 2);
        const auto Selector1 = arg<nt32::ULONG>(d.core, 3);
        const auto Entry1Low = arg<nt32::ULONG>(d.core, 4);
        const auto Entry1Hi  = arg<nt32::ULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetLdtEntries(Selector0:{:#x}, Entry0Low:{:#x}, Entry0Hi:{:#x}, Selector1:{:#x}, Entry1Low:{:#x}, Entry1Hi:{:#x})", Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi));

        for(const auto& it : d.observers_ZwSetLdtEntries)
            it(Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);
    }

    static void on_ZwSetLowEventPair(nt32::syscalls32::Data& d)
    {
        const auto EventPairHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetLowEventPair(EventPairHandle:{:#x})", EventPairHandle));

        for(const auto& it : d.observers_ZwSetLowEventPair)
            it(EventPairHandle);
    }

    static void on_ZwSetLowWaitHighEventPair(nt32::syscalls32::Data& d)
    {
        const auto EventPairHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetLowWaitHighEventPair(EventPairHandle:{:#x})", EventPairHandle));

        for(const auto& it : d.observers_ZwSetLowWaitHighEventPair)
            it(EventPairHandle);
    }

    static void on_ZwSetQuotaInformationFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto Buffer        = arg<nt32::PVOID>(d.core, 2);
        const auto Length        = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetQuotaInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x})", FileHandle, IoStatusBlock, Buffer, Length));

        for(const auto& it : d.observers_ZwSetQuotaInformationFile)
            it(FileHandle, IoStatusBlock, Buffer, Length);
    }

    static void on_NtSetSecurityObject(nt32::syscalls32::Data& d)
    {
        const auto Handle              = arg<nt32::HANDLE>(d.core, 0);
        const auto SecurityInformation = arg<nt32::SECURITY_INFORMATION>(d.core, 1);
        const auto SecurityDescriptor  = arg<nt32::PSECURITY_DESCRIPTOR>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetSecurityObject(Handle:{:#x}, SecurityInformation:{:#x}, SecurityDescriptor:{:#x})", Handle, SecurityInformation, SecurityDescriptor));

        for(const auto& it : d.observers_NtSetSecurityObject)
            it(Handle, SecurityInformation, SecurityDescriptor);
    }

    static void on_ZwSetSystemEnvironmentValueEx(nt32::syscalls32::Data& d)
    {
        const auto VariableName = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto VendorGuid   = arg<nt32::LPGUID>(d.core, 1);
        const auto Value        = arg<nt32::PVOID>(d.core, 2);
        const auto ValueLength  = arg<nt32::ULONG>(d.core, 3);
        const auto Attributes   = arg<nt32::ULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetSystemEnvironmentValueEx(VariableName:{:#x}, VendorGuid:{:#x}, Value:{:#x}, ValueLength:{:#x}, Attributes:{:#x})", VariableName, VendorGuid, Value, ValueLength, Attributes));

        for(const auto& it : d.observers_ZwSetSystemEnvironmentValueEx)
            it(VariableName, VendorGuid, Value, ValueLength, Attributes);
    }

    static void on_ZwSetSystemEnvironmentValue(nt32::syscalls32::Data& d)
    {
        const auto VariableName  = arg<nt32::PUNICODE_STRING>(d.core, 0);
        const auto VariableValue = arg<nt32::PUNICODE_STRING>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetSystemEnvironmentValue(VariableName:{:#x}, VariableValue:{:#x})", VariableName, VariableValue));

        for(const auto& it : d.observers_ZwSetSystemEnvironmentValue)
            it(VariableName, VariableValue);
    }

    static void on_ZwSetSystemInformation(nt32::syscalls32::Data& d)
    {
        const auto SystemInformationClass  = arg<nt32::SYSTEM_INFORMATION_CLASS>(d.core, 0);
        const auto SystemInformation       = arg<nt32::PVOID>(d.core, 1);
        const auto SystemInformationLength = arg<nt32::ULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetSystemInformation(SystemInformationClass:{:#x}, SystemInformation:{:#x}, SystemInformationLength:{:#x})", SystemInformationClass, SystemInformation, SystemInformationLength));

        for(const auto& it : d.observers_ZwSetSystemInformation)
            it(SystemInformationClass, SystemInformation, SystemInformationLength);
    }

    static void on_ZwSetSystemPowerState(nt32::syscalls32::Data& d)
    {
        const auto SystemAction   = arg<nt32::POWER_ACTION>(d.core, 0);
        const auto MinSystemState = arg<nt32::SYSTEM_POWER_STATE>(d.core, 1);
        const auto Flags          = arg<nt32::ULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetSystemPowerState(SystemAction:{:#x}, MinSystemState:{:#x}, Flags:{:#x})", SystemAction, MinSystemState, Flags));

        for(const auto& it : d.observers_ZwSetSystemPowerState)
            it(SystemAction, MinSystemState, Flags);
    }

    static void on_ZwSetSystemTime(nt32::syscalls32::Data& d)
    {
        const auto SystemTime   = arg<nt32::PLARGE_INTEGER>(d.core, 0);
        const auto PreviousTime = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetSystemTime(SystemTime:{:#x}, PreviousTime:{:#x})", SystemTime, PreviousTime));

        for(const auto& it : d.observers_ZwSetSystemTime)
            it(SystemTime, PreviousTime);
    }

    static void on_ZwSetThreadExecutionState(nt32::syscalls32::Data& d)
    {
        const auto esFlags           = arg<nt32::EXECUTION_STATE>(d.core, 0);
        const auto STARPreviousFlags = arg<nt32::EXECUTION_STATE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetThreadExecutionState(esFlags:{:#x}, STARPreviousFlags:{:#x})", esFlags, STARPreviousFlags));

        for(const auto& it : d.observers_ZwSetThreadExecutionState)
            it(esFlags, STARPreviousFlags);
    }

    static void on_ZwSetTimerEx(nt32::syscalls32::Data& d)
    {
        const auto TimerHandle               = arg<nt32::HANDLE>(d.core, 0);
        const auto TimerSetInformationClass  = arg<nt32::TIMER_SET_INFORMATION_CLASS>(d.core, 1);
        const auto TimerSetInformation       = arg<nt32::PVOID>(d.core, 2);
        const auto TimerSetInformationLength = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetTimerEx(TimerHandle:{:#x}, TimerSetInformationClass:{:#x}, TimerSetInformation:{:#x}, TimerSetInformationLength:{:#x})", TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength));

        for(const auto& it : d.observers_ZwSetTimerEx)
            it(TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength);
    }

    static void on_ZwSetTimer(nt32::syscalls32::Data& d)
    {
        const auto TimerHandle     = arg<nt32::HANDLE>(d.core, 0);
        const auto DueTime         = arg<nt32::PLARGE_INTEGER>(d.core, 1);
        const auto TimerApcRoutine = arg<nt32::PTIMER_APC_ROUTINE>(d.core, 2);
        const auto TimerContext    = arg<nt32::PVOID>(d.core, 3);
        const auto WakeTimer       = arg<nt32::BOOLEAN>(d.core, 4);
        const auto Period          = arg<nt32::LONG>(d.core, 5);
        const auto PreviousState   = arg<nt32::PBOOLEAN>(d.core, 6);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetTimer(TimerHandle:{:#x}, DueTime:{:#x}, TimerApcRoutine:{:#x}, TimerContext:{:#x}, WakeTimer:{:#x}, Period:{:#x}, PreviousState:{:#x})", TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState));

        for(const auto& it : d.observers_ZwSetTimer)
            it(TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState);
    }

    static void on_NtSetTimerResolution(nt32::syscalls32::Data& d)
    {
        const auto DesiredTime   = arg<nt32::ULONG>(d.core, 0);
        const auto SetResolution = arg<nt32::BOOLEAN>(d.core, 1);
        const auto ActualTime    = arg<nt32::PULONG>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetTimerResolution(DesiredTime:{:#x}, SetResolution:{:#x}, ActualTime:{:#x})", DesiredTime, SetResolution, ActualTime));

        for(const auto& it : d.observers_NtSetTimerResolution)
            it(DesiredTime, SetResolution, ActualTime);
    }

    static void on_ZwSetUuidSeed(nt32::syscalls32::Data& d)
    {
        const auto Seed = arg<nt32::PCHAR>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetUuidSeed(Seed:{:#x})", Seed));

        for(const auto& it : d.observers_ZwSetUuidSeed)
            it(Seed);
    }

    static void on_ZwSetValueKey(nt32::syscalls32::Data& d)
    {
        const auto KeyHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto ValueName  = arg<nt32::PUNICODE_STRING>(d.core, 1);
        const auto TitleIndex = arg<nt32::ULONG>(d.core, 2);
        const auto Type       = arg<nt32::ULONG>(d.core, 3);
        const auto Data       = arg<nt32::PVOID>(d.core, 4);
        const auto DataSize   = arg<nt32::ULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSetValueKey(KeyHandle:{:#x}, ValueName:{:#x}, TitleIndex:{:#x}, Type:{:#x}, Data:{:#x}, DataSize:{:#x})", KeyHandle, ValueName, TitleIndex, Type, Data, DataSize));

        for(const auto& it : d.observers_ZwSetValueKey)
            it(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
    }

    static void on_NtSetVolumeInformationFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle         = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto FsInformation      = arg<nt32::PVOID>(d.core, 2);
        const auto Length             = arg<nt32::ULONG>(d.core, 3);
        const auto FsInformationClass = arg<nt32::FS_INFORMATION_CLASS>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSetVolumeInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, FsInformation:{:#x}, Length:{:#x}, FsInformationClass:{:#x})", FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass));

        for(const auto& it : d.observers_NtSetVolumeInformationFile)
            it(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    }

    static void on_ZwShutdownSystem(nt32::syscalls32::Data& d)
    {
        const auto Action = arg<nt32::SHUTDOWN_ACTION>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwShutdownSystem(Action:{:#x})", Action));

        for(const auto& it : d.observers_ZwShutdownSystem)
            it(Action);
    }

    static void on_NtShutdownWorkerFactory(nt32::syscalls32::Data& d)
    {
        const auto WorkerFactoryHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto STARPendingWorkerCount = arg<nt32::LONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtShutdownWorkerFactory(WorkerFactoryHandle:{:#x}, STARPendingWorkerCount:{:#x})", WorkerFactoryHandle, STARPendingWorkerCount));

        for(const auto& it : d.observers_NtShutdownWorkerFactory)
            it(WorkerFactoryHandle, STARPendingWorkerCount);
    }

    static void on_ZwSignalAndWaitForSingleObject(nt32::syscalls32::Data& d)
    {
        const auto SignalHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto WaitHandle   = arg<nt32::HANDLE>(d.core, 1);
        const auto Alertable    = arg<nt32::BOOLEAN>(d.core, 2);
        const auto Timeout      = arg<nt32::PLARGE_INTEGER>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSignalAndWaitForSingleObject(SignalHandle:{:#x}, WaitHandle:{:#x}, Alertable:{:#x}, Timeout:{:#x})", SignalHandle, WaitHandle, Alertable, Timeout));

        for(const auto& it : d.observers_ZwSignalAndWaitForSingleObject)
            it(SignalHandle, WaitHandle, Alertable, Timeout);
    }

    static void on_ZwSinglePhaseReject(nt32::syscalls32::Data& d)
    {
        const auto EnlistmentHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSinglePhaseReject(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock));

        for(const auto& it : d.observers_ZwSinglePhaseReject)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtStartProfile(nt32::syscalls32::Data& d)
    {
        const auto ProfileHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtStartProfile(ProfileHandle:{:#x})", ProfileHandle));

        for(const auto& it : d.observers_NtStartProfile)
            it(ProfileHandle);
    }

    static void on_ZwStopProfile(nt32::syscalls32::Data& d)
    {
        const auto ProfileHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwStopProfile(ProfileHandle:{:#x})", ProfileHandle));

        for(const auto& it : d.observers_ZwStopProfile)
            it(ProfileHandle);
    }

    static void on_ZwSuspendProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSuspendProcess(ProcessHandle:{:#x})", ProcessHandle));

        for(const auto& it : d.observers_ZwSuspendProcess)
            it(ProcessHandle);
    }

    static void on_ZwSuspendThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle         = arg<nt32::HANDLE>(d.core, 0);
        const auto PreviousSuspendCount = arg<nt32::PULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwSuspendThread(ThreadHandle:{:#x}, PreviousSuspendCount:{:#x})", ThreadHandle, PreviousSuspendCount));

        for(const auto& it : d.observers_ZwSuspendThread)
            it(ThreadHandle, PreviousSuspendCount);
    }

    static void on_NtSystemDebugControl(nt32::syscalls32::Data& d)
    {
        const auto Command            = arg<nt32::SYSDBG_COMMAND>(d.core, 0);
        const auto InputBuffer        = arg<nt32::PVOID>(d.core, 1);
        const auto InputBufferLength  = arg<nt32::ULONG>(d.core, 2);
        const auto OutputBuffer       = arg<nt32::PVOID>(d.core, 3);
        const auto OutputBufferLength = arg<nt32::ULONG>(d.core, 4);
        const auto ReturnLength       = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSystemDebugControl(Command:{:#x}, InputBuffer:{:#x}, InputBufferLength:{:#x}, OutputBuffer:{:#x}, OutputBufferLength:{:#x}, ReturnLength:{:#x})", Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength));

        for(const auto& it : d.observers_NtSystemDebugControl)
            it(Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);
    }

    static void on_ZwTerminateJobObject(nt32::syscalls32::Data& d)
    {
        const auto JobHandle  = arg<nt32::HANDLE>(d.core, 0);
        const auto ExitStatus = arg<nt32::NTSTATUS>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwTerminateJobObject(JobHandle:{:#x}, ExitStatus:{:#x})", JobHandle, ExitStatus));

        for(const auto& it : d.observers_ZwTerminateJobObject)
            it(JobHandle, ExitStatus);
    }

    static void on_ZwTerminateProcess(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto ExitStatus    = arg<nt32::NTSTATUS>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwTerminateProcess(ProcessHandle:{:#x}, ExitStatus:{:#x})", ProcessHandle, ExitStatus));

        for(const auto& it : d.observers_ZwTerminateProcess)
            it(ProcessHandle, ExitStatus);
    }

    static void on_ZwTerminateThread(nt32::syscalls32::Data& d)
    {
        const auto ThreadHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto ExitStatus   = arg<nt32::NTSTATUS>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwTerminateThread(ThreadHandle:{:#x}, ExitStatus:{:#x})", ThreadHandle, ExitStatus));

        for(const auto& it : d.observers_ZwTerminateThread)
            it(ThreadHandle, ExitStatus);
    }

    static void on_ZwTraceControl(nt32::syscalls32::Data& d)
    {
        const auto FunctionCode = arg<nt32::ULONG>(d.core, 0);
        const auto InBuffer     = arg<nt32::PVOID>(d.core, 1);
        const auto InBufferLen  = arg<nt32::ULONG>(d.core, 2);
        const auto OutBuffer    = arg<nt32::PVOID>(d.core, 3);
        const auto OutBufferLen = arg<nt32::ULONG>(d.core, 4);
        const auto ReturnLength = arg<nt32::PULONG>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwTraceControl(FunctionCode:{:#x}, InBuffer:{:#x}, InBufferLen:{:#x}, OutBuffer:{:#x}, OutBufferLen:{:#x}, ReturnLength:{:#x})", FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength));

        for(const auto& it : d.observers_ZwTraceControl)
            it(FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength);
    }

    static void on_NtTraceEvent(nt32::syscalls32::Data& d)
    {
        const auto TraceHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto Flags       = arg<nt32::ULONG>(d.core, 1);
        const auto FieldSize   = arg<nt32::ULONG>(d.core, 2);
        const auto Fields      = arg<nt32::PVOID>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtTraceEvent(TraceHandle:{:#x}, Flags:{:#x}, FieldSize:{:#x}, Fields:{:#x})", TraceHandle, Flags, FieldSize, Fields));

        for(const auto& it : d.observers_NtTraceEvent)
            it(TraceHandle, Flags, FieldSize, Fields);
    }

    static void on_NtTranslateFilePath(nt32::syscalls32::Data& d)
    {
        const auto InputFilePath        = arg<nt32::PFILE_PATH>(d.core, 0);
        const auto OutputType           = arg<nt32::ULONG>(d.core, 1);
        const auto OutputFilePath       = arg<nt32::PFILE_PATH>(d.core, 2);
        const auto OutputFilePathLength = arg<nt32::PULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtTranslateFilePath(InputFilePath:{:#x}, OutputType:{:#x}, OutputFilePath:{:#x}, OutputFilePathLength:{:#x})", InputFilePath, OutputType, OutputFilePath, OutputFilePathLength));

        for(const auto& it : d.observers_NtTranslateFilePath)
            it(InputFilePath, OutputType, OutputFilePath, OutputFilePathLength);
    }

    static void on_ZwUnloadDriver(nt32::syscalls32::Data& d)
    {
        const auto DriverServiceName = arg<nt32::PUNICODE_STRING>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwUnloadDriver(DriverServiceName:{:#x})", DriverServiceName));

        for(const auto& it : d.observers_ZwUnloadDriver)
            it(DriverServiceName);
    }

    static void on_ZwUnloadKey2(nt32::syscalls32::Data& d)
    {
        const auto TargetKey = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto Flags     = arg<nt32::ULONG>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwUnloadKey2(TargetKey:{:#x}, Flags:{:#x})", TargetKey, Flags));

        for(const auto& it : d.observers_ZwUnloadKey2)
            it(TargetKey, Flags);
    }

    static void on_ZwUnloadKeyEx(nt32::syscalls32::Data& d)
    {
        const auto TargetKey = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto Event     = arg<nt32::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwUnloadKeyEx(TargetKey:{:#x}, Event:{:#x})", TargetKey, Event));

        for(const auto& it : d.observers_ZwUnloadKeyEx)
            it(TargetKey, Event);
    }

    static void on_NtUnloadKey(nt32::syscalls32::Data& d)
    {
        const auto TargetKey = arg<nt32::POBJECT_ATTRIBUTES>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtUnloadKey(TargetKey:{:#x})", TargetKey));

        for(const auto& it : d.observers_NtUnloadKey)
            it(TargetKey);
    }

    static void on_ZwUnlockFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(d.core, 1);
        const auto ByteOffset    = arg<nt32::PLARGE_INTEGER>(d.core, 2);
        const auto Length        = arg<nt32::PLARGE_INTEGER>(d.core, 3);
        const auto Key           = arg<nt32::ULONG>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwUnlockFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, ByteOffset:{:#x}, Length:{:#x}, Key:{:#x})", FileHandle, IoStatusBlock, ByteOffset, Length, Key));

        for(const auto& it : d.observers_ZwUnlockFile)
            it(FileHandle, IoStatusBlock, ByteOffset, Length, Key);
    }

    static void on_NtUnlockVirtualMemory(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle   = arg<nt32::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(d.core, 1);
        const auto RegionSize      = arg<nt32::PSIZE_T>(d.core, 2);
        const auto MapType         = arg<nt32::ULONG>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtUnlockVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, RegionSize:{:#x}, MapType:{:#x})", ProcessHandle, STARBaseAddress, RegionSize, MapType));

        for(const auto& it : d.observers_NtUnlockVirtualMemory)
            it(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    }

    static void on_NtUnmapViewOfSection(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto BaseAddress   = arg<nt32::PVOID>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtUnmapViewOfSection(ProcessHandle:{:#x}, BaseAddress:{:#x})", ProcessHandle, BaseAddress));

        for(const auto& it : d.observers_NtUnmapViewOfSection)
            it(ProcessHandle, BaseAddress);
    }

    static void on_NtVdmControl(nt32::syscalls32::Data& d)
    {
        const auto Service     = arg<nt32::VDMSERVICECLASS>(d.core, 0);
        const auto ServiceData = arg<nt32::PVOID>(d.core, 1);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtVdmControl(Service:{:#x}, ServiceData:{:#x})", Service, ServiceData));

        for(const auto& it : d.observers_NtVdmControl)
            it(Service, ServiceData);
    }

    static void on_NtWaitForDebugEvent(nt32::syscalls32::Data& d)
    {
        const auto DebugObjectHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto Alertable         = arg<nt32::BOOLEAN>(d.core, 1);
        const auto Timeout           = arg<nt32::PLARGE_INTEGER>(d.core, 2);
        const auto WaitStateChange   = arg<nt32::PDBGUI_WAIT_STATE_CHANGE>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWaitForDebugEvent(DebugObjectHandle:{:#x}, Alertable:{:#x}, Timeout:{:#x}, WaitStateChange:{:#x})", DebugObjectHandle, Alertable, Timeout, WaitStateChange));

        for(const auto& it : d.observers_NtWaitForDebugEvent)
            it(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
    }

    static void on_NtWaitForKeyedEvent(nt32::syscalls32::Data& d)
    {
        const auto KeyedEventHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto KeyValue         = arg<nt32::PVOID>(d.core, 1);
        const auto Alertable        = arg<nt32::BOOLEAN>(d.core, 2);
        const auto Timeout          = arg<nt32::PLARGE_INTEGER>(d.core, 3);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWaitForKeyedEvent(KeyedEventHandle:{:#x}, KeyValue:{:#x}, Alertable:{:#x}, Timeout:{:#x})", KeyedEventHandle, KeyValue, Alertable, Timeout));

        for(const auto& it : d.observers_NtWaitForKeyedEvent)
            it(KeyedEventHandle, KeyValue, Alertable, Timeout);
    }

    static void on_NtWaitForMultipleObjects32(nt32::syscalls32::Data& d)
    {
        const auto Count     = arg<nt32::ULONG>(d.core, 0);
        const auto Handles   = arg<nt32::LONG>(d.core, 1);
        const auto WaitType  = arg<nt32::WAIT_TYPE>(d.core, 2);
        const auto Alertable = arg<nt32::BOOLEAN>(d.core, 3);
        const auto Timeout   = arg<nt32::PLARGE_INTEGER>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWaitForMultipleObjects32(Count:{:#x}, Handles:{:#x}, WaitType:{:#x}, Alertable:{:#x}, Timeout:{:#x})", Count, Handles, WaitType, Alertable, Timeout));

        for(const auto& it : d.observers_NtWaitForMultipleObjects32)
            it(Count, Handles, WaitType, Alertable, Timeout);
    }

    static void on_NtWaitForMultipleObjects(nt32::syscalls32::Data& d)
    {
        const auto Count     = arg<nt32::ULONG>(d.core, 0);
        const auto Handles   = arg<nt32::HANDLE>(d.core, 1);
        const auto WaitType  = arg<nt32::WAIT_TYPE>(d.core, 2);
        const auto Alertable = arg<nt32::BOOLEAN>(d.core, 3);
        const auto Timeout   = arg<nt32::PLARGE_INTEGER>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWaitForMultipleObjects(Count:{:#x}, Handles:{:#x}, WaitType:{:#x}, Alertable:{:#x}, Timeout:{:#x})", Count, Handles, WaitType, Alertable, Timeout));

        for(const auto& it : d.observers_NtWaitForMultipleObjects)
            it(Count, Handles, WaitType, Alertable, Timeout);
    }

    static void on_ZwWaitForSingleObject(nt32::syscalls32::Data& d)
    {
        const auto Handle    = arg<nt32::HANDLE>(d.core, 0);
        const auto Alertable = arg<nt32::BOOLEAN>(d.core, 1);
        const auto Timeout   = arg<nt32::PLARGE_INTEGER>(d.core, 2);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwWaitForSingleObject(Handle:{:#x}, Alertable:{:#x}, Timeout:{:#x})", Handle, Alertable, Timeout));

        for(const auto& it : d.observers_ZwWaitForSingleObject)
            it(Handle, Alertable, Timeout);
    }

    static void on_NtWaitForWorkViaWorkerFactory(nt32::syscalls32::Data& d)
    {
        const auto WorkerFactoryHandle = arg<nt32::HANDLE>(d.core, 0);
        const auto MiniPacket          = arg<nt32::PFILE_IO_COMPLETION_INFORMATION>(d.core, 1);
        const auto Arg2                = arg<nt32::PVOID>(d.core, 2);
        const auto Arg3                = arg<nt32::PVOID>(d.core, 3);
        const auto Arg4                = arg<nt32::PVOID>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWaitForWorkViaWorkerFactory(WorkerFactoryHandle:{:#x}, MiniPacket:{:#x}, Arg2:{:#x}, Arg3:{:#x}, Arg4:{:#x})", WorkerFactoryHandle, MiniPacket, Arg2, Arg3, Arg4));

        for(const auto& it : d.observers_NtWaitForWorkViaWorkerFactory)
            it(WorkerFactoryHandle, MiniPacket, Arg2, Arg3, Arg4);
    }

    static void on_ZwWaitHighEventPair(nt32::syscalls32::Data& d)
    {
        const auto EventPairHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwWaitHighEventPair(EventPairHandle:{:#x})", EventPairHandle));

        for(const auto& it : d.observers_ZwWaitHighEventPair)
            it(EventPairHandle);
    }

    static void on_NtWaitLowEventPair(nt32::syscalls32::Data& d)
    {
        const auto EventPairHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWaitLowEventPair(EventPairHandle:{:#x})", EventPairHandle));

        for(const auto& it : d.observers_NtWaitLowEventPair)
            it(EventPairHandle);
    }

    static void on_NtWorkerFactoryWorkerReady(nt32::syscalls32::Data& d)
    {
        const auto WorkerFactoryHandle = arg<nt32::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWorkerFactoryWorkerReady(WorkerFactoryHandle:{:#x})", WorkerFactoryHandle));

        for(const auto& it : d.observers_NtWorkerFactoryWorkerReady)
            it(WorkerFactoryHandle);
    }

    static void on_NtWriteFileGather(nt32::syscalls32::Data& d)
    {
        const auto FileHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto Event         = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine    = arg<nt32::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext    = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(d.core, 4);
        const auto SegmentArray  = arg<nt32::PFILE_SEGMENT_ELEMENT>(d.core, 5);
        const auto Length        = arg<nt32::ULONG>(d.core, 6);
        const auto ByteOffset    = arg<nt32::PLARGE_INTEGER>(d.core, 7);
        const auto Key           = arg<nt32::PULONG>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWriteFileGather(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, SegmentArray:{:#x}, Length:{:#x}, ByteOffset:{:#x}, Key:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key));

        for(const auto& it : d.observers_NtWriteFileGather)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    }

    static void on_NtWriteFile(nt32::syscalls32::Data& d)
    {
        const auto FileHandle    = arg<nt32::HANDLE>(d.core, 0);
        const auto Event         = arg<nt32::HANDLE>(d.core, 1);
        const auto ApcRoutine    = arg<nt32::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext    = arg<nt32::PVOID>(d.core, 3);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(d.core, 4);
        const auto Buffer        = arg<nt32::PVOID>(d.core, 5);
        const auto Length        = arg<nt32::ULONG>(d.core, 6);
        const auto ByteOffset    = arg<nt32::PLARGE_INTEGER>(d.core, 7);
        const auto Key           = arg<nt32::PULONG>(d.core, 8);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWriteFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x}, ByteOffset:{:#x}, Key:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key));

        for(const auto& it : d.observers_NtWriteFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    }

    static void on_NtWriteRequestData(nt32::syscalls32::Data& d)
    {
        const auto PortHandle           = arg<nt32::HANDLE>(d.core, 0);
        const auto Message              = arg<nt32::PPORT_MESSAGE>(d.core, 1);
        const auto DataEntryIndex       = arg<nt32::ULONG>(d.core, 2);
        const auto Buffer               = arg<nt32::PVOID>(d.core, 3);
        const auto BufferSize           = arg<nt32::SIZE_T>(d.core, 4);
        const auto NumberOfBytesWritten = arg<nt32::PSIZE_T>(d.core, 5);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWriteRequestData(PortHandle:{:#x}, Message:{:#x}, DataEntryIndex:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, NumberOfBytesWritten:{:#x})", PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten));

        for(const auto& it : d.observers_NtWriteRequestData)
            it(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten);
    }

    static void on_NtWriteVirtualMemory(nt32::syscalls32::Data& d)
    {
        const auto ProcessHandle        = arg<nt32::HANDLE>(d.core, 0);
        const auto BaseAddress          = arg<nt32::PVOID>(d.core, 1);
        const auto Buffer               = arg<nt32::PVOID>(d.core, 2);
        const auto BufferSize           = arg<nt32::SIZE_T>(d.core, 3);
        const auto NumberOfBytesWritten = arg<nt32::PSIZE_T>(d.core, 4);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtWriteVirtualMemory(ProcessHandle:{:#x}, BaseAddress:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, NumberOfBytesWritten:{:#x})", ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten));

        for(const auto& it : d.observers_NtWriteVirtualMemory)
            it(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);
    }

    static void on_NtDisableLastKnownGood(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtDisableLastKnownGood()"));

        for(const auto& it : d.observers_NtDisableLastKnownGood)
            it();
    }

    static void on_NtEnableLastKnownGood(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtEnableLastKnownGood()"));

        for(const auto& it : d.observers_NtEnableLastKnownGood)
            it();
    }

    static void on_ZwFlushProcessWriteBuffers(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwFlushProcessWriteBuffers()"));

        for(const auto& it : d.observers_ZwFlushProcessWriteBuffers)
            it();
    }

    static void on_NtFlushWriteBuffer(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtFlushWriteBuffer()"));

        for(const auto& it : d.observers_NtFlushWriteBuffer)
            it();
    }

    static void on_NtGetCurrentProcessorNumber(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtGetCurrentProcessorNumber()"));

        for(const auto& it : d.observers_NtGetCurrentProcessorNumber)
            it();
    }

    static void on_NtIsSystemResumeAutomatic(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtIsSystemResumeAutomatic()"));

        for(const auto& it : d.observers_NtIsSystemResumeAutomatic)
            it();
    }

    static void on_ZwIsUILanguageComitted(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwIsUILanguageComitted()"));

        for(const auto& it : d.observers_ZwIsUILanguageComitted)
            it();
    }

    static void on_ZwQueryPortInformationProcess(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwQueryPortInformationProcess()"));

        for(const auto& it : d.observers_ZwQueryPortInformationProcess)
            it();
    }

    static void on_NtSerializeBoot(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtSerializeBoot()"));

        for(const auto& it : d.observers_NtSerializeBoot)
            it();
    }

    static void on_NtTestAlert(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtTestAlert()"));

        for(const auto& it : d.observers_NtTestAlert)
            it();
    }

    static void on_ZwThawRegistry(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwThawRegistry()"));

        for(const auto& it : d.observers_ZwThawRegistry)
            it();
    }

    static void on_NtThawTransactions(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("NtThawTransactions()"));

        for(const auto& it : d.observers_NtThawTransactions)
            it();
    }

    static void on_ZwUmsThreadYield(nt32::syscalls32::Data& d)
    {
        const auto SchedulerParam = arg<nt32::PVOID>(d.core, 0);

        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwUmsThreadYield(SchedulerParam:{:#x})", SchedulerParam));

        for(const auto& it : d.observers_ZwUmsThreadYield)
            it(SchedulerParam);
    }

    static void on_ZwYieldExecution(nt32::syscalls32::Data& d)
    {
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("ZwYieldExecution()"));

        for(const auto& it : d.observers_ZwYieldExecution)
            it();
    }

}


bool nt32::syscalls32::register_NtAcceptConnectPort(proc_t proc, const on_NtAcceptConnectPort_fn& on_func)
{
    if(d_->observers_NtAcceptConnectPort.empty())
        if(!register_callback_with(*d_, proc, "_NtAcceptConnectPort@24", &on_NtAcceptConnectPort))
            return false;

    d_->observers_NtAcceptConnectPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAccessCheckAndAuditAlarm(proc_t proc, const on_ZwAccessCheckAndAuditAlarm_fn& on_func)
{
    if(d_->observers_ZwAccessCheckAndAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "_ZwAccessCheckAndAuditAlarm@44", &on_ZwAccessCheckAndAuditAlarm))
            return false;

    d_->observers_ZwAccessCheckAndAuditAlarm.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAccessCheckByTypeAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeAndAuditAlarm_fn& on_func)
{
    if(d_->observers_NtAccessCheckByTypeAndAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "_NtAccessCheckByTypeAndAuditAlarm@64", &on_NtAccessCheckByTypeAndAuditAlarm))
            return false;

    d_->observers_NtAccessCheckByTypeAndAuditAlarm.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAccessCheckByType(proc_t proc, const on_NtAccessCheckByType_fn& on_func)
{
    if(d_->observers_NtAccessCheckByType.empty())
        if(!register_callback_with(*d_, proc, "_NtAccessCheckByType@44", &on_NtAccessCheckByType))
            return false;

    d_->observers_NtAccessCheckByType.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(proc_t proc, const on_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle_fn& on_func)
{
    if(d_->observers_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle.empty())
        if(!register_callback_with(*d_, proc, "_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle@68", &on_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle))
            return false;

    d_->observers_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAccessCheckByTypeResultListAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarm_fn& on_func)
{
    if(d_->observers_NtAccessCheckByTypeResultListAndAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "_NtAccessCheckByTypeResultListAndAuditAlarm@64", &on_NtAccessCheckByTypeResultListAndAuditAlarm))
            return false;

    d_->observers_NtAccessCheckByTypeResultListAndAuditAlarm.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAccessCheckByTypeResultList(proc_t proc, const on_NtAccessCheckByTypeResultList_fn& on_func)
{
    if(d_->observers_NtAccessCheckByTypeResultList.empty())
        if(!register_callback_with(*d_, proc, "_NtAccessCheckByTypeResultList@44", &on_NtAccessCheckByTypeResultList))
            return false;

    d_->observers_NtAccessCheckByTypeResultList.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAccessCheck(proc_t proc, const on_NtAccessCheck_fn& on_func)
{
    if(d_->observers_NtAccessCheck.empty())
        if(!register_callback_with(*d_, proc, "_NtAccessCheck@32", &on_NtAccessCheck))
            return false;

    d_->observers_NtAccessCheck.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAddAtom(proc_t proc, const on_NtAddAtom_fn& on_func)
{
    if(d_->observers_NtAddAtom.empty())
        if(!register_callback_with(*d_, proc, "_NtAddAtom@12", &on_NtAddAtom))
            return false;

    d_->observers_NtAddAtom.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAddBootEntry(proc_t proc, const on_ZwAddBootEntry_fn& on_func)
{
    if(d_->observers_ZwAddBootEntry.empty())
        if(!register_callback_with(*d_, proc, "_ZwAddBootEntry@8", &on_ZwAddBootEntry))
            return false;

    d_->observers_ZwAddBootEntry.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAddDriverEntry(proc_t proc, const on_NtAddDriverEntry_fn& on_func)
{
    if(d_->observers_NtAddDriverEntry.empty())
        if(!register_callback_with(*d_, proc, "_NtAddDriverEntry@8", &on_NtAddDriverEntry))
            return false;

    d_->observers_NtAddDriverEntry.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAdjustGroupsToken(proc_t proc, const on_ZwAdjustGroupsToken_fn& on_func)
{
    if(d_->observers_ZwAdjustGroupsToken.empty())
        if(!register_callback_with(*d_, proc, "_ZwAdjustGroupsToken@24", &on_ZwAdjustGroupsToken))
            return false;

    d_->observers_ZwAdjustGroupsToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAdjustPrivilegesToken(proc_t proc, const on_ZwAdjustPrivilegesToken_fn& on_func)
{
    if(d_->observers_ZwAdjustPrivilegesToken.empty())
        if(!register_callback_with(*d_, proc, "_ZwAdjustPrivilegesToken@24", &on_ZwAdjustPrivilegesToken))
            return false;

    d_->observers_ZwAdjustPrivilegesToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlertResumeThread(proc_t proc, const on_NtAlertResumeThread_fn& on_func)
{
    if(d_->observers_NtAlertResumeThread.empty())
        if(!register_callback_with(*d_, proc, "_NtAlertResumeThread@8", &on_NtAlertResumeThread))
            return false;

    d_->observers_NtAlertResumeThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlertThread(proc_t proc, const on_NtAlertThread_fn& on_func)
{
    if(d_->observers_NtAlertThread.empty())
        if(!register_callback_with(*d_, proc, "_NtAlertThread@4", &on_NtAlertThread))
            return false;

    d_->observers_NtAlertThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAllocateLocallyUniqueId(proc_t proc, const on_ZwAllocateLocallyUniqueId_fn& on_func)
{
    if(d_->observers_ZwAllocateLocallyUniqueId.empty())
        if(!register_callback_with(*d_, proc, "_ZwAllocateLocallyUniqueId@4", &on_ZwAllocateLocallyUniqueId))
            return false;

    d_->observers_ZwAllocateLocallyUniqueId.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAllocateReserveObject(proc_t proc, const on_NtAllocateReserveObject_fn& on_func)
{
    if(d_->observers_NtAllocateReserveObject.empty())
        if(!register_callback_with(*d_, proc, "_NtAllocateReserveObject@12", &on_NtAllocateReserveObject))
            return false;

    d_->observers_NtAllocateReserveObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAllocateUserPhysicalPages(proc_t proc, const on_NtAllocateUserPhysicalPages_fn& on_func)
{
    if(d_->observers_NtAllocateUserPhysicalPages.empty())
        if(!register_callback_with(*d_, proc, "_NtAllocateUserPhysicalPages@12", &on_NtAllocateUserPhysicalPages))
            return false;

    d_->observers_NtAllocateUserPhysicalPages.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAllocateUuids(proc_t proc, const on_NtAllocateUuids_fn& on_func)
{
    if(d_->observers_NtAllocateUuids.empty())
        if(!register_callback_with(*d_, proc, "_NtAllocateUuids@16", &on_NtAllocateUuids))
            return false;

    d_->observers_NtAllocateUuids.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAllocateVirtualMemory(proc_t proc, const on_NtAllocateVirtualMemory_fn& on_func)
{
    if(d_->observers_NtAllocateVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "_NtAllocateVirtualMemory@24", &on_NtAllocateVirtualMemory))
            return false;

    d_->observers_NtAllocateVirtualMemory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlpcAcceptConnectPort(proc_t proc, const on_NtAlpcAcceptConnectPort_fn& on_func)
{
    if(d_->observers_NtAlpcAcceptConnectPort.empty())
        if(!register_callback_with(*d_, proc, "_NtAlpcAcceptConnectPort@36", &on_NtAlpcAcceptConnectPort))
            return false;

    d_->observers_NtAlpcAcceptConnectPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcCancelMessage(proc_t proc, const on_ZwAlpcCancelMessage_fn& on_func)
{
    if(d_->observers_ZwAlpcCancelMessage.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcCancelMessage@12", &on_ZwAlpcCancelMessage))
            return false;

    d_->observers_ZwAlpcCancelMessage.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcConnectPort(proc_t proc, const on_ZwAlpcConnectPort_fn& on_func)
{
    if(d_->observers_ZwAlpcConnectPort.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcConnectPort@44", &on_ZwAlpcConnectPort))
            return false;

    d_->observers_ZwAlpcConnectPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcCreatePort(proc_t proc, const on_ZwAlpcCreatePort_fn& on_func)
{
    if(d_->observers_ZwAlpcCreatePort.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcCreatePort@12", &on_ZwAlpcCreatePort))
            return false;

    d_->observers_ZwAlpcCreatePort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlpcCreatePortSection(proc_t proc, const on_NtAlpcCreatePortSection_fn& on_func)
{
    if(d_->observers_NtAlpcCreatePortSection.empty())
        if(!register_callback_with(*d_, proc, "_NtAlpcCreatePortSection@24", &on_NtAlpcCreatePortSection))
            return false;

    d_->observers_NtAlpcCreatePortSection.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcCreateResourceReserve(proc_t proc, const on_ZwAlpcCreateResourceReserve_fn& on_func)
{
    if(d_->observers_ZwAlpcCreateResourceReserve.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcCreateResourceReserve@16", &on_ZwAlpcCreateResourceReserve))
            return false;

    d_->observers_ZwAlpcCreateResourceReserve.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcCreateSectionView(proc_t proc, const on_ZwAlpcCreateSectionView_fn& on_func)
{
    if(d_->observers_ZwAlpcCreateSectionView.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcCreateSectionView@12", &on_ZwAlpcCreateSectionView))
            return false;

    d_->observers_ZwAlpcCreateSectionView.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcCreateSecurityContext(proc_t proc, const on_ZwAlpcCreateSecurityContext_fn& on_func)
{
    if(d_->observers_ZwAlpcCreateSecurityContext.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcCreateSecurityContext@12", &on_ZwAlpcCreateSecurityContext))
            return false;

    d_->observers_ZwAlpcCreateSecurityContext.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcDeletePortSection(proc_t proc, const on_ZwAlpcDeletePortSection_fn& on_func)
{
    if(d_->observers_ZwAlpcDeletePortSection.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcDeletePortSection@12", &on_ZwAlpcDeletePortSection))
            return false;

    d_->observers_ZwAlpcDeletePortSection.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlpcDeleteResourceReserve(proc_t proc, const on_NtAlpcDeleteResourceReserve_fn& on_func)
{
    if(d_->observers_NtAlpcDeleteResourceReserve.empty())
        if(!register_callback_with(*d_, proc, "_NtAlpcDeleteResourceReserve@12", &on_NtAlpcDeleteResourceReserve))
            return false;

    d_->observers_NtAlpcDeleteResourceReserve.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlpcDeleteSectionView(proc_t proc, const on_NtAlpcDeleteSectionView_fn& on_func)
{
    if(d_->observers_NtAlpcDeleteSectionView.empty())
        if(!register_callback_with(*d_, proc, "_NtAlpcDeleteSectionView@12", &on_NtAlpcDeleteSectionView))
            return false;

    d_->observers_NtAlpcDeleteSectionView.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlpcDeleteSecurityContext(proc_t proc, const on_NtAlpcDeleteSecurityContext_fn& on_func)
{
    if(d_->observers_NtAlpcDeleteSecurityContext.empty())
        if(!register_callback_with(*d_, proc, "_NtAlpcDeleteSecurityContext@12", &on_NtAlpcDeleteSecurityContext))
            return false;

    d_->observers_NtAlpcDeleteSecurityContext.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlpcDisconnectPort(proc_t proc, const on_NtAlpcDisconnectPort_fn& on_func)
{
    if(d_->observers_NtAlpcDisconnectPort.empty())
        if(!register_callback_with(*d_, proc, "_NtAlpcDisconnectPort@8", &on_NtAlpcDisconnectPort))
            return false;

    d_->observers_NtAlpcDisconnectPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcImpersonateClientOfPort(proc_t proc, const on_ZwAlpcImpersonateClientOfPort_fn& on_func)
{
    if(d_->observers_ZwAlpcImpersonateClientOfPort.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcImpersonateClientOfPort@12", &on_ZwAlpcImpersonateClientOfPort))
            return false;

    d_->observers_ZwAlpcImpersonateClientOfPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcOpenSenderProcess(proc_t proc, const on_ZwAlpcOpenSenderProcess_fn& on_func)
{
    if(d_->observers_ZwAlpcOpenSenderProcess.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcOpenSenderProcess@24", &on_ZwAlpcOpenSenderProcess))
            return false;

    d_->observers_ZwAlpcOpenSenderProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcOpenSenderThread(proc_t proc, const on_ZwAlpcOpenSenderThread_fn& on_func)
{
    if(d_->observers_ZwAlpcOpenSenderThread.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcOpenSenderThread@24", &on_ZwAlpcOpenSenderThread))
            return false;

    d_->observers_ZwAlpcOpenSenderThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcQueryInformation(proc_t proc, const on_ZwAlpcQueryInformation_fn& on_func)
{
    if(d_->observers_ZwAlpcQueryInformation.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcQueryInformation@20", &on_ZwAlpcQueryInformation))
            return false;

    d_->observers_ZwAlpcQueryInformation.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAlpcQueryInformationMessage(proc_t proc, const on_ZwAlpcQueryInformationMessage_fn& on_func)
{
    if(d_->observers_ZwAlpcQueryInformationMessage.empty())
        if(!register_callback_with(*d_, proc, "_ZwAlpcQueryInformationMessage@24", &on_ZwAlpcQueryInformationMessage))
            return false;

    d_->observers_ZwAlpcQueryInformationMessage.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlpcRevokeSecurityContext(proc_t proc, const on_NtAlpcRevokeSecurityContext_fn& on_func)
{
    if(d_->observers_NtAlpcRevokeSecurityContext.empty())
        if(!register_callback_with(*d_, proc, "_NtAlpcRevokeSecurityContext@12", &on_NtAlpcRevokeSecurityContext))
            return false;

    d_->observers_NtAlpcRevokeSecurityContext.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlpcSendWaitReceivePort(proc_t proc, const on_NtAlpcSendWaitReceivePort_fn& on_func)
{
    if(d_->observers_NtAlpcSendWaitReceivePort.empty())
        if(!register_callback_with(*d_, proc, "_NtAlpcSendWaitReceivePort@32", &on_NtAlpcSendWaitReceivePort))
            return false;

    d_->observers_NtAlpcSendWaitReceivePort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtAlpcSetInformation(proc_t proc, const on_NtAlpcSetInformation_fn& on_func)
{
    if(d_->observers_NtAlpcSetInformation.empty())
        if(!register_callback_with(*d_, proc, "_NtAlpcSetInformation@16", &on_NtAlpcSetInformation))
            return false;

    d_->observers_NtAlpcSetInformation.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtApphelpCacheControl(proc_t proc, const on_NtApphelpCacheControl_fn& on_func)
{
    if(d_->observers_NtApphelpCacheControl.empty())
        if(!register_callback_with(*d_, proc, "_NtApphelpCacheControl@8", &on_NtApphelpCacheControl))
            return false;

    d_->observers_NtApphelpCacheControl.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAreMappedFilesTheSame(proc_t proc, const on_ZwAreMappedFilesTheSame_fn& on_func)
{
    if(d_->observers_ZwAreMappedFilesTheSame.empty())
        if(!register_callback_with(*d_, proc, "_ZwAreMappedFilesTheSame@8", &on_ZwAreMappedFilesTheSame))
            return false;

    d_->observers_ZwAreMappedFilesTheSame.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwAssignProcessToJobObject(proc_t proc, const on_ZwAssignProcessToJobObject_fn& on_func)
{
    if(d_->observers_ZwAssignProcessToJobObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwAssignProcessToJobObject@8", &on_ZwAssignProcessToJobObject))
            return false;

    d_->observers_ZwAssignProcessToJobObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCancelIoFileEx(proc_t proc, const on_NtCancelIoFileEx_fn& on_func)
{
    if(d_->observers_NtCancelIoFileEx.empty())
        if(!register_callback_with(*d_, proc, "_NtCancelIoFileEx@12", &on_NtCancelIoFileEx))
            return false;

    d_->observers_NtCancelIoFileEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCancelIoFile(proc_t proc, const on_ZwCancelIoFile_fn& on_func)
{
    if(d_->observers_ZwCancelIoFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwCancelIoFile@8", &on_ZwCancelIoFile))
            return false;

    d_->observers_ZwCancelIoFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCancelSynchronousIoFile(proc_t proc, const on_NtCancelSynchronousIoFile_fn& on_func)
{
    if(d_->observers_NtCancelSynchronousIoFile.empty())
        if(!register_callback_with(*d_, proc, "_NtCancelSynchronousIoFile@12", &on_NtCancelSynchronousIoFile))
            return false;

    d_->observers_NtCancelSynchronousIoFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCancelTimer(proc_t proc, const on_ZwCancelTimer_fn& on_func)
{
    if(d_->observers_ZwCancelTimer.empty())
        if(!register_callback_with(*d_, proc, "_ZwCancelTimer@8", &on_ZwCancelTimer))
            return false;

    d_->observers_ZwCancelTimer.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtClearEvent(proc_t proc, const on_NtClearEvent_fn& on_func)
{
    if(d_->observers_NtClearEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtClearEvent@4", &on_NtClearEvent))
            return false;

    d_->observers_NtClearEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtClose(proc_t proc, const on_NtClose_fn& on_func)
{
    if(d_->observers_NtClose.empty())
        if(!register_callback_with(*d_, proc, "_NtClose@4", &on_NtClose))
            return false;

    d_->observers_NtClose.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCloseObjectAuditAlarm(proc_t proc, const on_ZwCloseObjectAuditAlarm_fn& on_func)
{
    if(d_->observers_ZwCloseObjectAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "_ZwCloseObjectAuditAlarm@12", &on_ZwCloseObjectAuditAlarm))
            return false;

    d_->observers_ZwCloseObjectAuditAlarm.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCommitComplete(proc_t proc, const on_ZwCommitComplete_fn& on_func)
{
    if(d_->observers_ZwCommitComplete.empty())
        if(!register_callback_with(*d_, proc, "_ZwCommitComplete@8", &on_ZwCommitComplete))
            return false;

    d_->observers_ZwCommitComplete.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCommitEnlistment(proc_t proc, const on_NtCommitEnlistment_fn& on_func)
{
    if(d_->observers_NtCommitEnlistment.empty())
        if(!register_callback_with(*d_, proc, "_NtCommitEnlistment@8", &on_NtCommitEnlistment))
            return false;

    d_->observers_NtCommitEnlistment.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCommitTransaction(proc_t proc, const on_NtCommitTransaction_fn& on_func)
{
    if(d_->observers_NtCommitTransaction.empty())
        if(!register_callback_with(*d_, proc, "_NtCommitTransaction@8", &on_NtCommitTransaction))
            return false;

    d_->observers_NtCommitTransaction.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCompactKeys(proc_t proc, const on_NtCompactKeys_fn& on_func)
{
    if(d_->observers_NtCompactKeys.empty())
        if(!register_callback_with(*d_, proc, "_NtCompactKeys@8", &on_NtCompactKeys))
            return false;

    d_->observers_NtCompactKeys.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCompareTokens(proc_t proc, const on_ZwCompareTokens_fn& on_func)
{
    if(d_->observers_ZwCompareTokens.empty())
        if(!register_callback_with(*d_, proc, "_ZwCompareTokens@12", &on_ZwCompareTokens))
            return false;

    d_->observers_ZwCompareTokens.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCompleteConnectPort(proc_t proc, const on_NtCompleteConnectPort_fn& on_func)
{
    if(d_->observers_NtCompleteConnectPort.empty())
        if(!register_callback_with(*d_, proc, "_NtCompleteConnectPort@4", &on_NtCompleteConnectPort))
            return false;

    d_->observers_NtCompleteConnectPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCompressKey(proc_t proc, const on_ZwCompressKey_fn& on_func)
{
    if(d_->observers_ZwCompressKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwCompressKey@4", &on_ZwCompressKey))
            return false;

    d_->observers_ZwCompressKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtConnectPort(proc_t proc, const on_NtConnectPort_fn& on_func)
{
    if(d_->observers_NtConnectPort.empty())
        if(!register_callback_with(*d_, proc, "_NtConnectPort@32", &on_NtConnectPort))
            return false;

    d_->observers_NtConnectPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwContinue(proc_t proc, const on_ZwContinue_fn& on_func)
{
    if(d_->observers_ZwContinue.empty())
        if(!register_callback_with(*d_, proc, "_ZwContinue@8", &on_ZwContinue))
            return false;

    d_->observers_ZwContinue.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateDebugObject(proc_t proc, const on_ZwCreateDebugObject_fn& on_func)
{
    if(d_->observers_ZwCreateDebugObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateDebugObject@16", &on_ZwCreateDebugObject))
            return false;

    d_->observers_ZwCreateDebugObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateDirectoryObject(proc_t proc, const on_ZwCreateDirectoryObject_fn& on_func)
{
    if(d_->observers_ZwCreateDirectoryObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateDirectoryObject@12", &on_ZwCreateDirectoryObject))
            return false;

    d_->observers_ZwCreateDirectoryObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateEnlistment(proc_t proc, const on_ZwCreateEnlistment_fn& on_func)
{
    if(d_->observers_ZwCreateEnlistment.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateEnlistment@32", &on_ZwCreateEnlistment))
            return false;

    d_->observers_ZwCreateEnlistment.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateEvent(proc_t proc, const on_NtCreateEvent_fn& on_func)
{
    if(d_->observers_NtCreateEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateEvent@20", &on_NtCreateEvent))
            return false;

    d_->observers_NtCreateEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateEventPair(proc_t proc, const on_NtCreateEventPair_fn& on_func)
{
    if(d_->observers_NtCreateEventPair.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateEventPair@12", &on_NtCreateEventPair))
            return false;

    d_->observers_NtCreateEventPair.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateFile(proc_t proc, const on_NtCreateFile_fn& on_func)
{
    if(d_->observers_NtCreateFile.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateFile@44", &on_NtCreateFile))
            return false;

    d_->observers_NtCreateFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateIoCompletion(proc_t proc, const on_NtCreateIoCompletion_fn& on_func)
{
    if(d_->observers_NtCreateIoCompletion.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateIoCompletion@16", &on_NtCreateIoCompletion))
            return false;

    d_->observers_NtCreateIoCompletion.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateJobObject(proc_t proc, const on_ZwCreateJobObject_fn& on_func)
{
    if(d_->observers_ZwCreateJobObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateJobObject@12", &on_ZwCreateJobObject))
            return false;

    d_->observers_ZwCreateJobObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateJobSet(proc_t proc, const on_NtCreateJobSet_fn& on_func)
{
    if(d_->observers_NtCreateJobSet.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateJobSet@12", &on_NtCreateJobSet))
            return false;

    d_->observers_NtCreateJobSet.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateKeyedEvent(proc_t proc, const on_ZwCreateKeyedEvent_fn& on_func)
{
    if(d_->observers_ZwCreateKeyedEvent.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateKeyedEvent@16", &on_ZwCreateKeyedEvent))
            return false;

    d_->observers_ZwCreateKeyedEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateKey(proc_t proc, const on_ZwCreateKey_fn& on_func)
{
    if(d_->observers_ZwCreateKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateKey@28", &on_ZwCreateKey))
            return false;

    d_->observers_ZwCreateKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateKeyTransacted(proc_t proc, const on_NtCreateKeyTransacted_fn& on_func)
{
    if(d_->observers_NtCreateKeyTransacted.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateKeyTransacted@32", &on_NtCreateKeyTransacted))
            return false;

    d_->observers_NtCreateKeyTransacted.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateMailslotFile(proc_t proc, const on_ZwCreateMailslotFile_fn& on_func)
{
    if(d_->observers_ZwCreateMailslotFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateMailslotFile@32", &on_ZwCreateMailslotFile))
            return false;

    d_->observers_ZwCreateMailslotFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateMutant(proc_t proc, const on_ZwCreateMutant_fn& on_func)
{
    if(d_->observers_ZwCreateMutant.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateMutant@16", &on_ZwCreateMutant))
            return false;

    d_->observers_ZwCreateMutant.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateNamedPipeFile(proc_t proc, const on_ZwCreateNamedPipeFile_fn& on_func)
{
    if(d_->observers_ZwCreateNamedPipeFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateNamedPipeFile@56", &on_ZwCreateNamedPipeFile))
            return false;

    d_->observers_ZwCreateNamedPipeFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreatePagingFile(proc_t proc, const on_NtCreatePagingFile_fn& on_func)
{
    if(d_->observers_NtCreatePagingFile.empty())
        if(!register_callback_with(*d_, proc, "_NtCreatePagingFile@16", &on_NtCreatePagingFile))
            return false;

    d_->observers_NtCreatePagingFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreatePort(proc_t proc, const on_ZwCreatePort_fn& on_func)
{
    if(d_->observers_ZwCreatePort.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreatePort@20", &on_ZwCreatePort))
            return false;

    d_->observers_ZwCreatePort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreatePrivateNamespace(proc_t proc, const on_NtCreatePrivateNamespace_fn& on_func)
{
    if(d_->observers_NtCreatePrivateNamespace.empty())
        if(!register_callback_with(*d_, proc, "_NtCreatePrivateNamespace@16", &on_NtCreatePrivateNamespace))
            return false;

    d_->observers_NtCreatePrivateNamespace.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateProcessEx(proc_t proc, const on_ZwCreateProcessEx_fn& on_func)
{
    if(d_->observers_ZwCreateProcessEx.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateProcessEx@36", &on_ZwCreateProcessEx))
            return false;

    d_->observers_ZwCreateProcessEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateProcess(proc_t proc, const on_ZwCreateProcess_fn& on_func)
{
    if(d_->observers_ZwCreateProcess.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateProcess@32", &on_ZwCreateProcess))
            return false;

    d_->observers_ZwCreateProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateProfileEx(proc_t proc, const on_NtCreateProfileEx_fn& on_func)
{
    if(d_->observers_NtCreateProfileEx.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateProfileEx@40", &on_NtCreateProfileEx))
            return false;

    d_->observers_NtCreateProfileEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateProfile(proc_t proc, const on_ZwCreateProfile_fn& on_func)
{
    if(d_->observers_ZwCreateProfile.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateProfile@36", &on_ZwCreateProfile))
            return false;

    d_->observers_ZwCreateProfile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateResourceManager(proc_t proc, const on_ZwCreateResourceManager_fn& on_func)
{
    if(d_->observers_ZwCreateResourceManager.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateResourceManager@28", &on_ZwCreateResourceManager))
            return false;

    d_->observers_ZwCreateResourceManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateSection(proc_t proc, const on_NtCreateSection_fn& on_func)
{
    if(d_->observers_NtCreateSection.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateSection@28", &on_NtCreateSection))
            return false;

    d_->observers_NtCreateSection.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateSemaphore(proc_t proc, const on_NtCreateSemaphore_fn& on_func)
{
    if(d_->observers_NtCreateSemaphore.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateSemaphore@20", &on_NtCreateSemaphore))
            return false;

    d_->observers_NtCreateSemaphore.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateSymbolicLinkObject(proc_t proc, const on_ZwCreateSymbolicLinkObject_fn& on_func)
{
    if(d_->observers_ZwCreateSymbolicLinkObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateSymbolicLinkObject@16", &on_ZwCreateSymbolicLinkObject))
            return false;

    d_->observers_ZwCreateSymbolicLinkObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateThreadEx(proc_t proc, const on_NtCreateThreadEx_fn& on_func)
{
    if(d_->observers_NtCreateThreadEx.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateThreadEx@44", &on_NtCreateThreadEx))
            return false;

    d_->observers_NtCreateThreadEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateThread(proc_t proc, const on_NtCreateThread_fn& on_func)
{
    if(d_->observers_NtCreateThread.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateThread@32", &on_NtCreateThread))
            return false;

    d_->observers_NtCreateThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateTimer(proc_t proc, const on_ZwCreateTimer_fn& on_func)
{
    if(d_->observers_ZwCreateTimer.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateTimer@16", &on_ZwCreateTimer))
            return false;

    d_->observers_ZwCreateTimer.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateToken(proc_t proc, const on_NtCreateToken_fn& on_func)
{
    if(d_->observers_NtCreateToken.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateToken@52", &on_NtCreateToken))
            return false;

    d_->observers_NtCreateToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateTransactionManager(proc_t proc, const on_ZwCreateTransactionManager_fn& on_func)
{
    if(d_->observers_ZwCreateTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateTransactionManager@24", &on_ZwCreateTransactionManager))
            return false;

    d_->observers_ZwCreateTransactionManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateTransaction(proc_t proc, const on_NtCreateTransaction_fn& on_func)
{
    if(d_->observers_NtCreateTransaction.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateTransaction@40", &on_NtCreateTransaction))
            return false;

    d_->observers_NtCreateTransaction.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateUserProcess(proc_t proc, const on_NtCreateUserProcess_fn& on_func)
{
    if(d_->observers_NtCreateUserProcess.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateUserProcess@44", &on_NtCreateUserProcess))
            return false;

    d_->observers_NtCreateUserProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwCreateWaitablePort(proc_t proc, const on_ZwCreateWaitablePort_fn& on_func)
{
    if(d_->observers_ZwCreateWaitablePort.empty())
        if(!register_callback_with(*d_, proc, "_ZwCreateWaitablePort@20", &on_ZwCreateWaitablePort))
            return false;

    d_->observers_ZwCreateWaitablePort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtCreateWorkerFactory(proc_t proc, const on_NtCreateWorkerFactory_fn& on_func)
{
    if(d_->observers_NtCreateWorkerFactory.empty())
        if(!register_callback_with(*d_, proc, "_NtCreateWorkerFactory@40", &on_NtCreateWorkerFactory))
            return false;

    d_->observers_NtCreateWorkerFactory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtDebugActiveProcess(proc_t proc, const on_NtDebugActiveProcess_fn& on_func)
{
    if(d_->observers_NtDebugActiveProcess.empty())
        if(!register_callback_with(*d_, proc, "_NtDebugActiveProcess@8", &on_NtDebugActiveProcess))
            return false;

    d_->observers_NtDebugActiveProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwDebugContinue(proc_t proc, const on_ZwDebugContinue_fn& on_func)
{
    if(d_->observers_ZwDebugContinue.empty())
        if(!register_callback_with(*d_, proc, "_ZwDebugContinue@12", &on_ZwDebugContinue))
            return false;

    d_->observers_ZwDebugContinue.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwDelayExecution(proc_t proc, const on_ZwDelayExecution_fn& on_func)
{
    if(d_->observers_ZwDelayExecution.empty())
        if(!register_callback_with(*d_, proc, "_ZwDelayExecution@8", &on_ZwDelayExecution))
            return false;

    d_->observers_ZwDelayExecution.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwDeleteAtom(proc_t proc, const on_ZwDeleteAtom_fn& on_func)
{
    if(d_->observers_ZwDeleteAtom.empty())
        if(!register_callback_with(*d_, proc, "_ZwDeleteAtom@4", &on_ZwDeleteAtom))
            return false;

    d_->observers_ZwDeleteAtom.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtDeleteBootEntry(proc_t proc, const on_NtDeleteBootEntry_fn& on_func)
{
    if(d_->observers_NtDeleteBootEntry.empty())
        if(!register_callback_with(*d_, proc, "_NtDeleteBootEntry@4", &on_NtDeleteBootEntry))
            return false;

    d_->observers_NtDeleteBootEntry.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwDeleteDriverEntry(proc_t proc, const on_ZwDeleteDriverEntry_fn& on_func)
{
    if(d_->observers_ZwDeleteDriverEntry.empty())
        if(!register_callback_with(*d_, proc, "_ZwDeleteDriverEntry@4", &on_ZwDeleteDriverEntry))
            return false;

    d_->observers_ZwDeleteDriverEntry.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtDeleteFile(proc_t proc, const on_NtDeleteFile_fn& on_func)
{
    if(d_->observers_NtDeleteFile.empty())
        if(!register_callback_with(*d_, proc, "_NtDeleteFile@4", &on_NtDeleteFile))
            return false;

    d_->observers_NtDeleteFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwDeleteKey(proc_t proc, const on_ZwDeleteKey_fn& on_func)
{
    if(d_->observers_ZwDeleteKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwDeleteKey@4", &on_ZwDeleteKey))
            return false;

    d_->observers_ZwDeleteKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtDeleteObjectAuditAlarm(proc_t proc, const on_NtDeleteObjectAuditAlarm_fn& on_func)
{
    if(d_->observers_NtDeleteObjectAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "_NtDeleteObjectAuditAlarm@12", &on_NtDeleteObjectAuditAlarm))
            return false;

    d_->observers_NtDeleteObjectAuditAlarm.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtDeletePrivateNamespace(proc_t proc, const on_NtDeletePrivateNamespace_fn& on_func)
{
    if(d_->observers_NtDeletePrivateNamespace.empty())
        if(!register_callback_with(*d_, proc, "_NtDeletePrivateNamespace@4", &on_NtDeletePrivateNamespace))
            return false;

    d_->observers_NtDeletePrivateNamespace.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtDeleteValueKey(proc_t proc, const on_NtDeleteValueKey_fn& on_func)
{
    if(d_->observers_NtDeleteValueKey.empty())
        if(!register_callback_with(*d_, proc, "_NtDeleteValueKey@8", &on_NtDeleteValueKey))
            return false;

    d_->observers_NtDeleteValueKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwDeviceIoControlFile(proc_t proc, const on_ZwDeviceIoControlFile_fn& on_func)
{
    if(d_->observers_ZwDeviceIoControlFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwDeviceIoControlFile@40", &on_ZwDeviceIoControlFile))
            return false;

    d_->observers_ZwDeviceIoControlFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtDisplayString(proc_t proc, const on_NtDisplayString_fn& on_func)
{
    if(d_->observers_NtDisplayString.empty())
        if(!register_callback_with(*d_, proc, "_NtDisplayString@4", &on_NtDisplayString))
            return false;

    d_->observers_NtDisplayString.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwDrawText(proc_t proc, const on_ZwDrawText_fn& on_func)
{
    if(d_->observers_ZwDrawText.empty())
        if(!register_callback_with(*d_, proc, "_ZwDrawText@4", &on_ZwDrawText))
            return false;

    d_->observers_ZwDrawText.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwDuplicateObject(proc_t proc, const on_ZwDuplicateObject_fn& on_func)
{
    if(d_->observers_ZwDuplicateObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwDuplicateObject@28", &on_ZwDuplicateObject))
            return false;

    d_->observers_ZwDuplicateObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtDuplicateToken(proc_t proc, const on_NtDuplicateToken_fn& on_func)
{
    if(d_->observers_NtDuplicateToken.empty())
        if(!register_callback_with(*d_, proc, "_NtDuplicateToken@24", &on_NtDuplicateToken))
            return false;

    d_->observers_NtDuplicateToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwEnumerateBootEntries(proc_t proc, const on_ZwEnumerateBootEntries_fn& on_func)
{
    if(d_->observers_ZwEnumerateBootEntries.empty())
        if(!register_callback_with(*d_, proc, "_ZwEnumerateBootEntries@8", &on_ZwEnumerateBootEntries))
            return false;

    d_->observers_ZwEnumerateBootEntries.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtEnumerateDriverEntries(proc_t proc, const on_NtEnumerateDriverEntries_fn& on_func)
{
    if(d_->observers_NtEnumerateDriverEntries.empty())
        if(!register_callback_with(*d_, proc, "_NtEnumerateDriverEntries@8", &on_NtEnumerateDriverEntries))
            return false;

    d_->observers_NtEnumerateDriverEntries.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwEnumerateKey(proc_t proc, const on_ZwEnumerateKey_fn& on_func)
{
    if(d_->observers_ZwEnumerateKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwEnumerateKey@24", &on_ZwEnumerateKey))
            return false;

    d_->observers_ZwEnumerateKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwEnumerateSystemEnvironmentValuesEx(proc_t proc, const on_ZwEnumerateSystemEnvironmentValuesEx_fn& on_func)
{
    if(d_->observers_ZwEnumerateSystemEnvironmentValuesEx.empty())
        if(!register_callback_with(*d_, proc, "_ZwEnumerateSystemEnvironmentValuesEx@12", &on_ZwEnumerateSystemEnvironmentValuesEx))
            return false;

    d_->observers_ZwEnumerateSystemEnvironmentValuesEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwEnumerateTransactionObject(proc_t proc, const on_ZwEnumerateTransactionObject_fn& on_func)
{
    if(d_->observers_ZwEnumerateTransactionObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwEnumerateTransactionObject@20", &on_ZwEnumerateTransactionObject))
            return false;

    d_->observers_ZwEnumerateTransactionObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtEnumerateValueKey(proc_t proc, const on_NtEnumerateValueKey_fn& on_func)
{
    if(d_->observers_NtEnumerateValueKey.empty())
        if(!register_callback_with(*d_, proc, "_NtEnumerateValueKey@24", &on_NtEnumerateValueKey))
            return false;

    d_->observers_NtEnumerateValueKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwExtendSection(proc_t proc, const on_ZwExtendSection_fn& on_func)
{
    if(d_->observers_ZwExtendSection.empty())
        if(!register_callback_with(*d_, proc, "_ZwExtendSection@8", &on_ZwExtendSection))
            return false;

    d_->observers_ZwExtendSection.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtFilterToken(proc_t proc, const on_NtFilterToken_fn& on_func)
{
    if(d_->observers_NtFilterToken.empty())
        if(!register_callback_with(*d_, proc, "_NtFilterToken@24", &on_NtFilterToken))
            return false;

    d_->observers_NtFilterToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtFindAtom(proc_t proc, const on_NtFindAtom_fn& on_func)
{
    if(d_->observers_NtFindAtom.empty())
        if(!register_callback_with(*d_, proc, "_NtFindAtom@12", &on_NtFindAtom))
            return false;

    d_->observers_NtFindAtom.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwFlushBuffersFile(proc_t proc, const on_ZwFlushBuffersFile_fn& on_func)
{
    if(d_->observers_ZwFlushBuffersFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwFlushBuffersFile@8", &on_ZwFlushBuffersFile))
            return false;

    d_->observers_ZwFlushBuffersFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwFlushInstallUILanguage(proc_t proc, const on_ZwFlushInstallUILanguage_fn& on_func)
{
    if(d_->observers_ZwFlushInstallUILanguage.empty())
        if(!register_callback_with(*d_, proc, "_ZwFlushInstallUILanguage@8", &on_ZwFlushInstallUILanguage))
            return false;

    d_->observers_ZwFlushInstallUILanguage.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwFlushInstructionCache(proc_t proc, const on_ZwFlushInstructionCache_fn& on_func)
{
    if(d_->observers_ZwFlushInstructionCache.empty())
        if(!register_callback_with(*d_, proc, "_ZwFlushInstructionCache@12", &on_ZwFlushInstructionCache))
            return false;

    d_->observers_ZwFlushInstructionCache.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtFlushKey(proc_t proc, const on_NtFlushKey_fn& on_func)
{
    if(d_->observers_NtFlushKey.empty())
        if(!register_callback_with(*d_, proc, "_NtFlushKey@4", &on_NtFlushKey))
            return false;

    d_->observers_NtFlushKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwFlushVirtualMemory(proc_t proc, const on_ZwFlushVirtualMemory_fn& on_func)
{
    if(d_->observers_ZwFlushVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "_ZwFlushVirtualMemory@16", &on_ZwFlushVirtualMemory))
            return false;

    d_->observers_ZwFlushVirtualMemory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtFreeUserPhysicalPages(proc_t proc, const on_NtFreeUserPhysicalPages_fn& on_func)
{
    if(d_->observers_NtFreeUserPhysicalPages.empty())
        if(!register_callback_with(*d_, proc, "_NtFreeUserPhysicalPages@12", &on_NtFreeUserPhysicalPages))
            return false;

    d_->observers_NtFreeUserPhysicalPages.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtFreeVirtualMemory(proc_t proc, const on_NtFreeVirtualMemory_fn& on_func)
{
    if(d_->observers_NtFreeVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "_NtFreeVirtualMemory@16", &on_NtFreeVirtualMemory))
            return false;

    d_->observers_NtFreeVirtualMemory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtFreezeRegistry(proc_t proc, const on_NtFreezeRegistry_fn& on_func)
{
    if(d_->observers_NtFreezeRegistry.empty())
        if(!register_callback_with(*d_, proc, "_NtFreezeRegistry@4", &on_NtFreezeRegistry))
            return false;

    d_->observers_NtFreezeRegistry.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwFreezeTransactions(proc_t proc, const on_ZwFreezeTransactions_fn& on_func)
{
    if(d_->observers_ZwFreezeTransactions.empty())
        if(!register_callback_with(*d_, proc, "_ZwFreezeTransactions@8", &on_ZwFreezeTransactions))
            return false;

    d_->observers_ZwFreezeTransactions.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtFsControlFile(proc_t proc, const on_NtFsControlFile_fn& on_func)
{
    if(d_->observers_NtFsControlFile.empty())
        if(!register_callback_with(*d_, proc, "_NtFsControlFile@40", &on_NtFsControlFile))
            return false;

    d_->observers_NtFsControlFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtGetContextThread(proc_t proc, const on_NtGetContextThread_fn& on_func)
{
    if(d_->observers_NtGetContextThread.empty())
        if(!register_callback_with(*d_, proc, "_NtGetContextThread@8", &on_NtGetContextThread))
            return false;

    d_->observers_NtGetContextThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtGetDevicePowerState(proc_t proc, const on_NtGetDevicePowerState_fn& on_func)
{
    if(d_->observers_NtGetDevicePowerState.empty())
        if(!register_callback_with(*d_, proc, "_NtGetDevicePowerState@8", &on_NtGetDevicePowerState))
            return false;

    d_->observers_NtGetDevicePowerState.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtGetMUIRegistryInfo(proc_t proc, const on_NtGetMUIRegistryInfo_fn& on_func)
{
    if(d_->observers_NtGetMUIRegistryInfo.empty())
        if(!register_callback_with(*d_, proc, "_NtGetMUIRegistryInfo@12", &on_NtGetMUIRegistryInfo))
            return false;

    d_->observers_NtGetMUIRegistryInfo.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwGetNextProcess(proc_t proc, const on_ZwGetNextProcess_fn& on_func)
{
    if(d_->observers_ZwGetNextProcess.empty())
        if(!register_callback_with(*d_, proc, "_ZwGetNextProcess@20", &on_ZwGetNextProcess))
            return false;

    d_->observers_ZwGetNextProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwGetNextThread(proc_t proc, const on_ZwGetNextThread_fn& on_func)
{
    if(d_->observers_ZwGetNextThread.empty())
        if(!register_callback_with(*d_, proc, "_ZwGetNextThread@24", &on_ZwGetNextThread))
            return false;

    d_->observers_ZwGetNextThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtGetNlsSectionPtr(proc_t proc, const on_NtGetNlsSectionPtr_fn& on_func)
{
    if(d_->observers_NtGetNlsSectionPtr.empty())
        if(!register_callback_with(*d_, proc, "_NtGetNlsSectionPtr@20", &on_NtGetNlsSectionPtr))
            return false;

    d_->observers_NtGetNlsSectionPtr.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwGetNotificationResourceManager(proc_t proc, const on_ZwGetNotificationResourceManager_fn& on_func)
{
    if(d_->observers_ZwGetNotificationResourceManager.empty())
        if(!register_callback_with(*d_, proc, "_ZwGetNotificationResourceManager@28", &on_ZwGetNotificationResourceManager))
            return false;

    d_->observers_ZwGetNotificationResourceManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtGetWriteWatch(proc_t proc, const on_NtGetWriteWatch_fn& on_func)
{
    if(d_->observers_NtGetWriteWatch.empty())
        if(!register_callback_with(*d_, proc, "_NtGetWriteWatch@28", &on_NtGetWriteWatch))
            return false;

    d_->observers_NtGetWriteWatch.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtImpersonateAnonymousToken(proc_t proc, const on_NtImpersonateAnonymousToken_fn& on_func)
{
    if(d_->observers_NtImpersonateAnonymousToken.empty())
        if(!register_callback_with(*d_, proc, "_NtImpersonateAnonymousToken@4", &on_NtImpersonateAnonymousToken))
            return false;

    d_->observers_NtImpersonateAnonymousToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwImpersonateClientOfPort(proc_t proc, const on_ZwImpersonateClientOfPort_fn& on_func)
{
    if(d_->observers_ZwImpersonateClientOfPort.empty())
        if(!register_callback_with(*d_, proc, "_ZwImpersonateClientOfPort@8", &on_ZwImpersonateClientOfPort))
            return false;

    d_->observers_ZwImpersonateClientOfPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwImpersonateThread(proc_t proc, const on_ZwImpersonateThread_fn& on_func)
{
    if(d_->observers_ZwImpersonateThread.empty())
        if(!register_callback_with(*d_, proc, "_ZwImpersonateThread@12", &on_ZwImpersonateThread))
            return false;

    d_->observers_ZwImpersonateThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtInitializeNlsFiles(proc_t proc, const on_NtInitializeNlsFiles_fn& on_func)
{
    if(d_->observers_NtInitializeNlsFiles.empty())
        if(!register_callback_with(*d_, proc, "_NtInitializeNlsFiles@12", &on_NtInitializeNlsFiles))
            return false;

    d_->observers_NtInitializeNlsFiles.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwInitializeRegistry(proc_t proc, const on_ZwInitializeRegistry_fn& on_func)
{
    if(d_->observers_ZwInitializeRegistry.empty())
        if(!register_callback_with(*d_, proc, "_ZwInitializeRegistry@4", &on_ZwInitializeRegistry))
            return false;

    d_->observers_ZwInitializeRegistry.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtInitiatePowerAction(proc_t proc, const on_NtInitiatePowerAction_fn& on_func)
{
    if(d_->observers_NtInitiatePowerAction.empty())
        if(!register_callback_with(*d_, proc, "_NtInitiatePowerAction@16", &on_NtInitiatePowerAction))
            return false;

    d_->observers_NtInitiatePowerAction.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwIsProcessInJob(proc_t proc, const on_ZwIsProcessInJob_fn& on_func)
{
    if(d_->observers_ZwIsProcessInJob.empty())
        if(!register_callback_with(*d_, proc, "_ZwIsProcessInJob@8", &on_ZwIsProcessInJob))
            return false;

    d_->observers_ZwIsProcessInJob.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwListenPort(proc_t proc, const on_ZwListenPort_fn& on_func)
{
    if(d_->observers_ZwListenPort.empty())
        if(!register_callback_with(*d_, proc, "_ZwListenPort@8", &on_ZwListenPort))
            return false;

    d_->observers_ZwListenPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtLoadDriver(proc_t proc, const on_NtLoadDriver_fn& on_func)
{
    if(d_->observers_NtLoadDriver.empty())
        if(!register_callback_with(*d_, proc, "_NtLoadDriver@4", &on_NtLoadDriver))
            return false;

    d_->observers_NtLoadDriver.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtLoadKey2(proc_t proc, const on_NtLoadKey2_fn& on_func)
{
    if(d_->observers_NtLoadKey2.empty())
        if(!register_callback_with(*d_, proc, "_NtLoadKey2@12", &on_NtLoadKey2))
            return false;

    d_->observers_NtLoadKey2.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtLoadKeyEx(proc_t proc, const on_NtLoadKeyEx_fn& on_func)
{
    if(d_->observers_NtLoadKeyEx.empty())
        if(!register_callback_with(*d_, proc, "_NtLoadKeyEx@32", &on_NtLoadKeyEx))
            return false;

    d_->observers_NtLoadKeyEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtLoadKey(proc_t proc, const on_NtLoadKey_fn& on_func)
{
    if(d_->observers_NtLoadKey.empty())
        if(!register_callback_with(*d_, proc, "_NtLoadKey@8", &on_NtLoadKey))
            return false;

    d_->observers_NtLoadKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtLockFile(proc_t proc, const on_NtLockFile_fn& on_func)
{
    if(d_->observers_NtLockFile.empty())
        if(!register_callback_with(*d_, proc, "_NtLockFile@40", &on_NtLockFile))
            return false;

    d_->observers_NtLockFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwLockProductActivationKeys(proc_t proc, const on_ZwLockProductActivationKeys_fn& on_func)
{
    if(d_->observers_ZwLockProductActivationKeys.empty())
        if(!register_callback_with(*d_, proc, "_ZwLockProductActivationKeys@8", &on_ZwLockProductActivationKeys))
            return false;

    d_->observers_ZwLockProductActivationKeys.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtLockRegistryKey(proc_t proc, const on_NtLockRegistryKey_fn& on_func)
{
    if(d_->observers_NtLockRegistryKey.empty())
        if(!register_callback_with(*d_, proc, "_NtLockRegistryKey@4", &on_NtLockRegistryKey))
            return false;

    d_->observers_NtLockRegistryKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwLockVirtualMemory(proc_t proc, const on_ZwLockVirtualMemory_fn& on_func)
{
    if(d_->observers_ZwLockVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "_ZwLockVirtualMemory@16", &on_ZwLockVirtualMemory))
            return false;

    d_->observers_ZwLockVirtualMemory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwMakePermanentObject(proc_t proc, const on_ZwMakePermanentObject_fn& on_func)
{
    if(d_->observers_ZwMakePermanentObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwMakePermanentObject@4", &on_ZwMakePermanentObject))
            return false;

    d_->observers_ZwMakePermanentObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtMakeTemporaryObject(proc_t proc, const on_NtMakeTemporaryObject_fn& on_func)
{
    if(d_->observers_NtMakeTemporaryObject.empty())
        if(!register_callback_with(*d_, proc, "_NtMakeTemporaryObject@4", &on_NtMakeTemporaryObject))
            return false;

    d_->observers_NtMakeTemporaryObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwMapCMFModule(proc_t proc, const on_ZwMapCMFModule_fn& on_func)
{
    if(d_->observers_ZwMapCMFModule.empty())
        if(!register_callback_with(*d_, proc, "_ZwMapCMFModule@24", &on_ZwMapCMFModule))
            return false;

    d_->observers_ZwMapCMFModule.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtMapUserPhysicalPages(proc_t proc, const on_NtMapUserPhysicalPages_fn& on_func)
{
    if(d_->observers_NtMapUserPhysicalPages.empty())
        if(!register_callback_with(*d_, proc, "_NtMapUserPhysicalPages@12", &on_NtMapUserPhysicalPages))
            return false;

    d_->observers_NtMapUserPhysicalPages.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwMapUserPhysicalPagesScatter(proc_t proc, const on_ZwMapUserPhysicalPagesScatter_fn& on_func)
{
    if(d_->observers_ZwMapUserPhysicalPagesScatter.empty())
        if(!register_callback_with(*d_, proc, "_ZwMapUserPhysicalPagesScatter@12", &on_ZwMapUserPhysicalPagesScatter))
            return false;

    d_->observers_ZwMapUserPhysicalPagesScatter.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwMapViewOfSection(proc_t proc, const on_ZwMapViewOfSection_fn& on_func)
{
    if(d_->observers_ZwMapViewOfSection.empty())
        if(!register_callback_with(*d_, proc, "_ZwMapViewOfSection@40", &on_ZwMapViewOfSection))
            return false;

    d_->observers_ZwMapViewOfSection.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtModifyBootEntry(proc_t proc, const on_NtModifyBootEntry_fn& on_func)
{
    if(d_->observers_NtModifyBootEntry.empty())
        if(!register_callback_with(*d_, proc, "_NtModifyBootEntry@4", &on_NtModifyBootEntry))
            return false;

    d_->observers_NtModifyBootEntry.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwModifyDriverEntry(proc_t proc, const on_ZwModifyDriverEntry_fn& on_func)
{
    if(d_->observers_ZwModifyDriverEntry.empty())
        if(!register_callback_with(*d_, proc, "_ZwModifyDriverEntry@4", &on_ZwModifyDriverEntry))
            return false;

    d_->observers_ZwModifyDriverEntry.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtNotifyChangeDirectoryFile(proc_t proc, const on_NtNotifyChangeDirectoryFile_fn& on_func)
{
    if(d_->observers_NtNotifyChangeDirectoryFile.empty())
        if(!register_callback_with(*d_, proc, "_NtNotifyChangeDirectoryFile@36", &on_NtNotifyChangeDirectoryFile))
            return false;

    d_->observers_NtNotifyChangeDirectoryFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtNotifyChangeKey(proc_t proc, const on_NtNotifyChangeKey_fn& on_func)
{
    if(d_->observers_NtNotifyChangeKey.empty())
        if(!register_callback_with(*d_, proc, "_NtNotifyChangeKey@40", &on_NtNotifyChangeKey))
            return false;

    d_->observers_NtNotifyChangeKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtNotifyChangeMultipleKeys(proc_t proc, const on_NtNotifyChangeMultipleKeys_fn& on_func)
{
    if(d_->observers_NtNotifyChangeMultipleKeys.empty())
        if(!register_callback_with(*d_, proc, "_NtNotifyChangeMultipleKeys@48", &on_NtNotifyChangeMultipleKeys))
            return false;

    d_->observers_NtNotifyChangeMultipleKeys.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtNotifyChangeSession(proc_t proc, const on_NtNotifyChangeSession_fn& on_func)
{
    if(d_->observers_NtNotifyChangeSession.empty())
        if(!register_callback_with(*d_, proc, "_NtNotifyChangeSession@32", &on_NtNotifyChangeSession))
            return false;

    d_->observers_NtNotifyChangeSession.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenDirectoryObject(proc_t proc, const on_ZwOpenDirectoryObject_fn& on_func)
{
    if(d_->observers_ZwOpenDirectoryObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenDirectoryObject@12", &on_ZwOpenDirectoryObject))
            return false;

    d_->observers_ZwOpenDirectoryObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenEnlistment(proc_t proc, const on_ZwOpenEnlistment_fn& on_func)
{
    if(d_->observers_ZwOpenEnlistment.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenEnlistment@20", &on_ZwOpenEnlistment))
            return false;

    d_->observers_ZwOpenEnlistment.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenEvent(proc_t proc, const on_NtOpenEvent_fn& on_func)
{
    if(d_->observers_NtOpenEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenEvent@12", &on_NtOpenEvent))
            return false;

    d_->observers_NtOpenEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenEventPair(proc_t proc, const on_NtOpenEventPair_fn& on_func)
{
    if(d_->observers_NtOpenEventPair.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenEventPair@12", &on_NtOpenEventPair))
            return false;

    d_->observers_NtOpenEventPair.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenFile(proc_t proc, const on_NtOpenFile_fn& on_func)
{
    if(d_->observers_NtOpenFile.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenFile@24", &on_NtOpenFile))
            return false;

    d_->observers_NtOpenFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenIoCompletion(proc_t proc, const on_ZwOpenIoCompletion_fn& on_func)
{
    if(d_->observers_ZwOpenIoCompletion.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenIoCompletion@12", &on_ZwOpenIoCompletion))
            return false;

    d_->observers_ZwOpenIoCompletion.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenJobObject(proc_t proc, const on_ZwOpenJobObject_fn& on_func)
{
    if(d_->observers_ZwOpenJobObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenJobObject@12", &on_ZwOpenJobObject))
            return false;

    d_->observers_ZwOpenJobObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenKeyedEvent(proc_t proc, const on_NtOpenKeyedEvent_fn& on_func)
{
    if(d_->observers_NtOpenKeyedEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenKeyedEvent@12", &on_NtOpenKeyedEvent))
            return false;

    d_->observers_NtOpenKeyedEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenKeyEx(proc_t proc, const on_ZwOpenKeyEx_fn& on_func)
{
    if(d_->observers_ZwOpenKeyEx.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenKeyEx@16", &on_ZwOpenKeyEx))
            return false;

    d_->observers_ZwOpenKeyEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenKey(proc_t proc, const on_ZwOpenKey_fn& on_func)
{
    if(d_->observers_ZwOpenKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenKey@12", &on_ZwOpenKey))
            return false;

    d_->observers_ZwOpenKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenKeyTransactedEx(proc_t proc, const on_NtOpenKeyTransactedEx_fn& on_func)
{
    if(d_->observers_NtOpenKeyTransactedEx.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenKeyTransactedEx@20", &on_NtOpenKeyTransactedEx))
            return false;

    d_->observers_NtOpenKeyTransactedEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenKeyTransacted(proc_t proc, const on_NtOpenKeyTransacted_fn& on_func)
{
    if(d_->observers_NtOpenKeyTransacted.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenKeyTransacted@16", &on_NtOpenKeyTransacted))
            return false;

    d_->observers_NtOpenKeyTransacted.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenMutant(proc_t proc, const on_NtOpenMutant_fn& on_func)
{
    if(d_->observers_NtOpenMutant.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenMutant@12", &on_NtOpenMutant))
            return false;

    d_->observers_NtOpenMutant.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenObjectAuditAlarm(proc_t proc, const on_ZwOpenObjectAuditAlarm_fn& on_func)
{
    if(d_->observers_ZwOpenObjectAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenObjectAuditAlarm@48", &on_ZwOpenObjectAuditAlarm))
            return false;

    d_->observers_ZwOpenObjectAuditAlarm.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenPrivateNamespace(proc_t proc, const on_NtOpenPrivateNamespace_fn& on_func)
{
    if(d_->observers_NtOpenPrivateNamespace.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenPrivateNamespace@16", &on_NtOpenPrivateNamespace))
            return false;

    d_->observers_NtOpenPrivateNamespace.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenProcess(proc_t proc, const on_ZwOpenProcess_fn& on_func)
{
    if(d_->observers_ZwOpenProcess.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenProcess@16", &on_ZwOpenProcess))
            return false;

    d_->observers_ZwOpenProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenProcessTokenEx(proc_t proc, const on_ZwOpenProcessTokenEx_fn& on_func)
{
    if(d_->observers_ZwOpenProcessTokenEx.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenProcessTokenEx@16", &on_ZwOpenProcessTokenEx))
            return false;

    d_->observers_ZwOpenProcessTokenEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenProcessToken(proc_t proc, const on_ZwOpenProcessToken_fn& on_func)
{
    if(d_->observers_ZwOpenProcessToken.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenProcessToken@12", &on_ZwOpenProcessToken))
            return false;

    d_->observers_ZwOpenProcessToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenResourceManager(proc_t proc, const on_ZwOpenResourceManager_fn& on_func)
{
    if(d_->observers_ZwOpenResourceManager.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenResourceManager@20", &on_ZwOpenResourceManager))
            return false;

    d_->observers_ZwOpenResourceManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenSection(proc_t proc, const on_NtOpenSection_fn& on_func)
{
    if(d_->observers_NtOpenSection.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenSection@12", &on_NtOpenSection))
            return false;

    d_->observers_NtOpenSection.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenSemaphore(proc_t proc, const on_NtOpenSemaphore_fn& on_func)
{
    if(d_->observers_NtOpenSemaphore.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenSemaphore@12", &on_NtOpenSemaphore))
            return false;

    d_->observers_NtOpenSemaphore.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenSession(proc_t proc, const on_NtOpenSession_fn& on_func)
{
    if(d_->observers_NtOpenSession.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenSession@12", &on_NtOpenSession))
            return false;

    d_->observers_NtOpenSession.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenSymbolicLinkObject(proc_t proc, const on_NtOpenSymbolicLinkObject_fn& on_func)
{
    if(d_->observers_NtOpenSymbolicLinkObject.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenSymbolicLinkObject@12", &on_NtOpenSymbolicLinkObject))
            return false;

    d_->observers_NtOpenSymbolicLinkObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenThread(proc_t proc, const on_ZwOpenThread_fn& on_func)
{
    if(d_->observers_ZwOpenThread.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenThread@16", &on_ZwOpenThread))
            return false;

    d_->observers_ZwOpenThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenThreadTokenEx(proc_t proc, const on_NtOpenThreadTokenEx_fn& on_func)
{
    if(d_->observers_NtOpenThreadTokenEx.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenThreadTokenEx@20", &on_NtOpenThreadTokenEx))
            return false;

    d_->observers_NtOpenThreadTokenEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtOpenThreadToken(proc_t proc, const on_NtOpenThreadToken_fn& on_func)
{
    if(d_->observers_NtOpenThreadToken.empty())
        if(!register_callback_with(*d_, proc, "_NtOpenThreadToken@16", &on_NtOpenThreadToken))
            return false;

    d_->observers_NtOpenThreadToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenTimer(proc_t proc, const on_ZwOpenTimer_fn& on_func)
{
    if(d_->observers_ZwOpenTimer.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenTimer@12", &on_ZwOpenTimer))
            return false;

    d_->observers_ZwOpenTimer.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenTransactionManager(proc_t proc, const on_ZwOpenTransactionManager_fn& on_func)
{
    if(d_->observers_ZwOpenTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenTransactionManager@24", &on_ZwOpenTransactionManager))
            return false;

    d_->observers_ZwOpenTransactionManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwOpenTransaction(proc_t proc, const on_ZwOpenTransaction_fn& on_func)
{
    if(d_->observers_ZwOpenTransaction.empty())
        if(!register_callback_with(*d_, proc, "_ZwOpenTransaction@20", &on_ZwOpenTransaction))
            return false;

    d_->observers_ZwOpenTransaction.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtPlugPlayControl(proc_t proc, const on_NtPlugPlayControl_fn& on_func)
{
    if(d_->observers_NtPlugPlayControl.empty())
        if(!register_callback_with(*d_, proc, "_NtPlugPlayControl@12", &on_NtPlugPlayControl))
            return false;

    d_->observers_NtPlugPlayControl.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwPowerInformation(proc_t proc, const on_ZwPowerInformation_fn& on_func)
{
    if(d_->observers_ZwPowerInformation.empty())
        if(!register_callback_with(*d_, proc, "_ZwPowerInformation@20", &on_ZwPowerInformation))
            return false;

    d_->observers_ZwPowerInformation.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtPrepareComplete(proc_t proc, const on_NtPrepareComplete_fn& on_func)
{
    if(d_->observers_NtPrepareComplete.empty())
        if(!register_callback_with(*d_, proc, "_NtPrepareComplete@8", &on_NtPrepareComplete))
            return false;

    d_->observers_NtPrepareComplete.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwPrepareEnlistment(proc_t proc, const on_ZwPrepareEnlistment_fn& on_func)
{
    if(d_->observers_ZwPrepareEnlistment.empty())
        if(!register_callback_with(*d_, proc, "_ZwPrepareEnlistment@8", &on_ZwPrepareEnlistment))
            return false;

    d_->observers_ZwPrepareEnlistment.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwPrePrepareComplete(proc_t proc, const on_ZwPrePrepareComplete_fn& on_func)
{
    if(d_->observers_ZwPrePrepareComplete.empty())
        if(!register_callback_with(*d_, proc, "_ZwPrePrepareComplete@8", &on_ZwPrePrepareComplete))
            return false;

    d_->observers_ZwPrePrepareComplete.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtPrePrepareEnlistment(proc_t proc, const on_NtPrePrepareEnlistment_fn& on_func)
{
    if(d_->observers_NtPrePrepareEnlistment.empty())
        if(!register_callback_with(*d_, proc, "_NtPrePrepareEnlistment@8", &on_NtPrePrepareEnlistment))
            return false;

    d_->observers_NtPrePrepareEnlistment.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwPrivilegeCheck(proc_t proc, const on_ZwPrivilegeCheck_fn& on_func)
{
    if(d_->observers_ZwPrivilegeCheck.empty())
        if(!register_callback_with(*d_, proc, "_ZwPrivilegeCheck@12", &on_ZwPrivilegeCheck))
            return false;

    d_->observers_ZwPrivilegeCheck.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtPrivilegedServiceAuditAlarm(proc_t proc, const on_NtPrivilegedServiceAuditAlarm_fn& on_func)
{
    if(d_->observers_NtPrivilegedServiceAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "_NtPrivilegedServiceAuditAlarm@20", &on_NtPrivilegedServiceAuditAlarm))
            return false;

    d_->observers_NtPrivilegedServiceAuditAlarm.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwPrivilegeObjectAuditAlarm(proc_t proc, const on_ZwPrivilegeObjectAuditAlarm_fn& on_func)
{
    if(d_->observers_ZwPrivilegeObjectAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "_ZwPrivilegeObjectAuditAlarm@24", &on_ZwPrivilegeObjectAuditAlarm))
            return false;

    d_->observers_ZwPrivilegeObjectAuditAlarm.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtPropagationComplete(proc_t proc, const on_NtPropagationComplete_fn& on_func)
{
    if(d_->observers_NtPropagationComplete.empty())
        if(!register_callback_with(*d_, proc, "_NtPropagationComplete@16", &on_NtPropagationComplete))
            return false;

    d_->observers_NtPropagationComplete.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwPropagationFailed(proc_t proc, const on_ZwPropagationFailed_fn& on_func)
{
    if(d_->observers_ZwPropagationFailed.empty())
        if(!register_callback_with(*d_, proc, "_ZwPropagationFailed@12", &on_ZwPropagationFailed))
            return false;

    d_->observers_ZwPropagationFailed.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwProtectVirtualMemory(proc_t proc, const on_ZwProtectVirtualMemory_fn& on_func)
{
    if(d_->observers_ZwProtectVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "_ZwProtectVirtualMemory@20", &on_ZwProtectVirtualMemory))
            return false;

    d_->observers_ZwProtectVirtualMemory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwPulseEvent(proc_t proc, const on_ZwPulseEvent_fn& on_func)
{
    if(d_->observers_ZwPulseEvent.empty())
        if(!register_callback_with(*d_, proc, "_ZwPulseEvent@8", &on_ZwPulseEvent))
            return false;

    d_->observers_ZwPulseEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryAttributesFile(proc_t proc, const on_ZwQueryAttributesFile_fn& on_func)
{
    if(d_->observers_ZwQueryAttributesFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryAttributesFile@8", &on_ZwQueryAttributesFile))
            return false;

    d_->observers_ZwQueryAttributesFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryBootEntryOrder(proc_t proc, const on_ZwQueryBootEntryOrder_fn& on_func)
{
    if(d_->observers_ZwQueryBootEntryOrder.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryBootEntryOrder@8", &on_ZwQueryBootEntryOrder))
            return false;

    d_->observers_ZwQueryBootEntryOrder.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryBootOptions(proc_t proc, const on_ZwQueryBootOptions_fn& on_func)
{
    if(d_->observers_ZwQueryBootOptions.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryBootOptions@8", &on_ZwQueryBootOptions))
            return false;

    d_->observers_ZwQueryBootOptions.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryDebugFilterState(proc_t proc, const on_NtQueryDebugFilterState_fn& on_func)
{
    if(d_->observers_NtQueryDebugFilterState.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryDebugFilterState@8", &on_NtQueryDebugFilterState))
            return false;

    d_->observers_NtQueryDebugFilterState.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryDefaultLocale(proc_t proc, const on_NtQueryDefaultLocale_fn& on_func)
{
    if(d_->observers_NtQueryDefaultLocale.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryDefaultLocale@8", &on_NtQueryDefaultLocale))
            return false;

    d_->observers_NtQueryDefaultLocale.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryDefaultUILanguage(proc_t proc, const on_ZwQueryDefaultUILanguage_fn& on_func)
{
    if(d_->observers_ZwQueryDefaultUILanguage.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryDefaultUILanguage@4", &on_ZwQueryDefaultUILanguage))
            return false;

    d_->observers_ZwQueryDefaultUILanguage.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryDirectoryFile(proc_t proc, const on_ZwQueryDirectoryFile_fn& on_func)
{
    if(d_->observers_ZwQueryDirectoryFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryDirectoryFile@44", &on_ZwQueryDirectoryFile))
            return false;

    d_->observers_ZwQueryDirectoryFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryDirectoryObject(proc_t proc, const on_ZwQueryDirectoryObject_fn& on_func)
{
    if(d_->observers_ZwQueryDirectoryObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryDirectoryObject@28", &on_ZwQueryDirectoryObject))
            return false;

    d_->observers_ZwQueryDirectoryObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryDriverEntryOrder(proc_t proc, const on_NtQueryDriverEntryOrder_fn& on_func)
{
    if(d_->observers_NtQueryDriverEntryOrder.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryDriverEntryOrder@8", &on_NtQueryDriverEntryOrder))
            return false;

    d_->observers_NtQueryDriverEntryOrder.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryEaFile(proc_t proc, const on_ZwQueryEaFile_fn& on_func)
{
    if(d_->observers_ZwQueryEaFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryEaFile@36", &on_ZwQueryEaFile))
            return false;

    d_->observers_ZwQueryEaFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryEvent(proc_t proc, const on_NtQueryEvent_fn& on_func)
{
    if(d_->observers_NtQueryEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryEvent@20", &on_NtQueryEvent))
            return false;

    d_->observers_NtQueryEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryFullAttributesFile(proc_t proc, const on_ZwQueryFullAttributesFile_fn& on_func)
{
    if(d_->observers_ZwQueryFullAttributesFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryFullAttributesFile@8", &on_ZwQueryFullAttributesFile))
            return false;

    d_->observers_ZwQueryFullAttributesFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryInformationAtom(proc_t proc, const on_NtQueryInformationAtom_fn& on_func)
{
    if(d_->observers_NtQueryInformationAtom.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryInformationAtom@20", &on_NtQueryInformationAtom))
            return false;

    d_->observers_NtQueryInformationAtom.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryInformationEnlistment(proc_t proc, const on_ZwQueryInformationEnlistment_fn& on_func)
{
    if(d_->observers_ZwQueryInformationEnlistment.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryInformationEnlistment@20", &on_ZwQueryInformationEnlistment))
            return false;

    d_->observers_ZwQueryInformationEnlistment.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryInformationFile(proc_t proc, const on_ZwQueryInformationFile_fn& on_func)
{
    if(d_->observers_ZwQueryInformationFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryInformationFile@20", &on_ZwQueryInformationFile))
            return false;

    d_->observers_ZwQueryInformationFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryInformationJobObject(proc_t proc, const on_ZwQueryInformationJobObject_fn& on_func)
{
    if(d_->observers_ZwQueryInformationJobObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryInformationJobObject@20", &on_ZwQueryInformationJobObject))
            return false;

    d_->observers_ZwQueryInformationJobObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryInformationPort(proc_t proc, const on_ZwQueryInformationPort_fn& on_func)
{
    if(d_->observers_ZwQueryInformationPort.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryInformationPort@20", &on_ZwQueryInformationPort))
            return false;

    d_->observers_ZwQueryInformationPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryInformationProcess(proc_t proc, const on_ZwQueryInformationProcess_fn& on_func)
{
    if(d_->observers_ZwQueryInformationProcess.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryInformationProcess@20", &on_ZwQueryInformationProcess))
            return false;

    d_->observers_ZwQueryInformationProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryInformationResourceManager(proc_t proc, const on_ZwQueryInformationResourceManager_fn& on_func)
{
    if(d_->observers_ZwQueryInformationResourceManager.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryInformationResourceManager@20", &on_ZwQueryInformationResourceManager))
            return false;

    d_->observers_ZwQueryInformationResourceManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryInformationThread(proc_t proc, const on_NtQueryInformationThread_fn& on_func)
{
    if(d_->observers_NtQueryInformationThread.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryInformationThread@20", &on_NtQueryInformationThread))
            return false;

    d_->observers_NtQueryInformationThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryInformationToken(proc_t proc, const on_ZwQueryInformationToken_fn& on_func)
{
    if(d_->observers_ZwQueryInformationToken.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryInformationToken@20", &on_ZwQueryInformationToken))
            return false;

    d_->observers_ZwQueryInformationToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryInformationTransaction(proc_t proc, const on_ZwQueryInformationTransaction_fn& on_func)
{
    if(d_->observers_ZwQueryInformationTransaction.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryInformationTransaction@20", &on_ZwQueryInformationTransaction))
            return false;

    d_->observers_ZwQueryInformationTransaction.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryInformationTransactionManager(proc_t proc, const on_NtQueryInformationTransactionManager_fn& on_func)
{
    if(d_->observers_NtQueryInformationTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryInformationTransactionManager@20", &on_NtQueryInformationTransactionManager))
            return false;

    d_->observers_NtQueryInformationTransactionManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryInformationWorkerFactory(proc_t proc, const on_ZwQueryInformationWorkerFactory_fn& on_func)
{
    if(d_->observers_ZwQueryInformationWorkerFactory.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryInformationWorkerFactory@20", &on_ZwQueryInformationWorkerFactory))
            return false;

    d_->observers_ZwQueryInformationWorkerFactory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryInstallUILanguage(proc_t proc, const on_NtQueryInstallUILanguage_fn& on_func)
{
    if(d_->observers_NtQueryInstallUILanguage.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryInstallUILanguage@4", &on_NtQueryInstallUILanguage))
            return false;

    d_->observers_NtQueryInstallUILanguage.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryIntervalProfile(proc_t proc, const on_NtQueryIntervalProfile_fn& on_func)
{
    if(d_->observers_NtQueryIntervalProfile.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryIntervalProfile@8", &on_NtQueryIntervalProfile))
            return false;

    d_->observers_NtQueryIntervalProfile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryIoCompletion(proc_t proc, const on_NtQueryIoCompletion_fn& on_func)
{
    if(d_->observers_NtQueryIoCompletion.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryIoCompletion@20", &on_NtQueryIoCompletion))
            return false;

    d_->observers_NtQueryIoCompletion.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryKey(proc_t proc, const on_ZwQueryKey_fn& on_func)
{
    if(d_->observers_ZwQueryKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryKey@20", &on_ZwQueryKey))
            return false;

    d_->observers_ZwQueryKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryLicenseValue(proc_t proc, const on_NtQueryLicenseValue_fn& on_func)
{
    if(d_->observers_NtQueryLicenseValue.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryLicenseValue@20", &on_NtQueryLicenseValue))
            return false;

    d_->observers_NtQueryLicenseValue.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryMultipleValueKey(proc_t proc, const on_NtQueryMultipleValueKey_fn& on_func)
{
    if(d_->observers_NtQueryMultipleValueKey.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryMultipleValueKey@24", &on_NtQueryMultipleValueKey))
            return false;

    d_->observers_NtQueryMultipleValueKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryMutant(proc_t proc, const on_NtQueryMutant_fn& on_func)
{
    if(d_->observers_NtQueryMutant.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryMutant@20", &on_NtQueryMutant))
            return false;

    d_->observers_NtQueryMutant.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryObject(proc_t proc, const on_NtQueryObject_fn& on_func)
{
    if(d_->observers_NtQueryObject.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryObject@20", &on_NtQueryObject))
            return false;

    d_->observers_NtQueryObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryOpenSubKeysEx(proc_t proc, const on_NtQueryOpenSubKeysEx_fn& on_func)
{
    if(d_->observers_NtQueryOpenSubKeysEx.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryOpenSubKeysEx@16", &on_NtQueryOpenSubKeysEx))
            return false;

    d_->observers_NtQueryOpenSubKeysEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryOpenSubKeys(proc_t proc, const on_NtQueryOpenSubKeys_fn& on_func)
{
    if(d_->observers_NtQueryOpenSubKeys.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryOpenSubKeys@8", &on_NtQueryOpenSubKeys))
            return false;

    d_->observers_NtQueryOpenSubKeys.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryPerformanceCounter(proc_t proc, const on_NtQueryPerformanceCounter_fn& on_func)
{
    if(d_->observers_NtQueryPerformanceCounter.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryPerformanceCounter@8", &on_NtQueryPerformanceCounter))
            return false;

    d_->observers_NtQueryPerformanceCounter.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryQuotaInformationFile(proc_t proc, const on_ZwQueryQuotaInformationFile_fn& on_func)
{
    if(d_->observers_ZwQueryQuotaInformationFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryQuotaInformationFile@36", &on_ZwQueryQuotaInformationFile))
            return false;

    d_->observers_ZwQueryQuotaInformationFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQuerySection(proc_t proc, const on_ZwQuerySection_fn& on_func)
{
    if(d_->observers_ZwQuerySection.empty())
        if(!register_callback_with(*d_, proc, "_ZwQuerySection@20", &on_ZwQuerySection))
            return false;

    d_->observers_ZwQuerySection.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQuerySecurityAttributesToken(proc_t proc, const on_ZwQuerySecurityAttributesToken_fn& on_func)
{
    if(d_->observers_ZwQuerySecurityAttributesToken.empty())
        if(!register_callback_with(*d_, proc, "_ZwQuerySecurityAttributesToken@24", &on_ZwQuerySecurityAttributesToken))
            return false;

    d_->observers_ZwQuerySecurityAttributesToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQuerySecurityObject(proc_t proc, const on_NtQuerySecurityObject_fn& on_func)
{
    if(d_->observers_NtQuerySecurityObject.empty())
        if(!register_callback_with(*d_, proc, "_NtQuerySecurityObject@20", &on_NtQuerySecurityObject))
            return false;

    d_->observers_NtQuerySecurityObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQuerySemaphore(proc_t proc, const on_ZwQuerySemaphore_fn& on_func)
{
    if(d_->observers_ZwQuerySemaphore.empty())
        if(!register_callback_with(*d_, proc, "_ZwQuerySemaphore@20", &on_ZwQuerySemaphore))
            return false;

    d_->observers_ZwQuerySemaphore.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQuerySymbolicLinkObject(proc_t proc, const on_ZwQuerySymbolicLinkObject_fn& on_func)
{
    if(d_->observers_ZwQuerySymbolicLinkObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwQuerySymbolicLinkObject@12", &on_ZwQuerySymbolicLinkObject))
            return false;

    d_->observers_ZwQuerySymbolicLinkObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQuerySystemEnvironmentValueEx(proc_t proc, const on_ZwQuerySystemEnvironmentValueEx_fn& on_func)
{
    if(d_->observers_ZwQuerySystemEnvironmentValueEx.empty())
        if(!register_callback_with(*d_, proc, "_ZwQuerySystemEnvironmentValueEx@20", &on_ZwQuerySystemEnvironmentValueEx))
            return false;

    d_->observers_ZwQuerySystemEnvironmentValueEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQuerySystemEnvironmentValue(proc_t proc, const on_ZwQuerySystemEnvironmentValue_fn& on_func)
{
    if(d_->observers_ZwQuerySystemEnvironmentValue.empty())
        if(!register_callback_with(*d_, proc, "_ZwQuerySystemEnvironmentValue@16", &on_ZwQuerySystemEnvironmentValue))
            return false;

    d_->observers_ZwQuerySystemEnvironmentValue.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQuerySystemInformationEx(proc_t proc, const on_ZwQuerySystemInformationEx_fn& on_func)
{
    if(d_->observers_ZwQuerySystemInformationEx.empty())
        if(!register_callback_with(*d_, proc, "_ZwQuerySystemInformationEx@24", &on_ZwQuerySystemInformationEx))
            return false;

    d_->observers_ZwQuerySystemInformationEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQuerySystemInformation(proc_t proc, const on_NtQuerySystemInformation_fn& on_func)
{
    if(d_->observers_NtQuerySystemInformation.empty())
        if(!register_callback_with(*d_, proc, "_NtQuerySystemInformation@16", &on_NtQuerySystemInformation))
            return false;

    d_->observers_NtQuerySystemInformation.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQuerySystemTime(proc_t proc, const on_NtQuerySystemTime_fn& on_func)
{
    if(d_->observers_NtQuerySystemTime.empty())
        if(!register_callback_with(*d_, proc, "_NtQuerySystemTime@4", &on_NtQuerySystemTime))
            return false;

    d_->observers_NtQuerySystemTime.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryTimer(proc_t proc, const on_ZwQueryTimer_fn& on_func)
{
    if(d_->observers_ZwQueryTimer.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryTimer@20", &on_ZwQueryTimer))
            return false;

    d_->observers_ZwQueryTimer.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryTimerResolution(proc_t proc, const on_NtQueryTimerResolution_fn& on_func)
{
    if(d_->observers_NtQueryTimerResolution.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryTimerResolution@12", &on_NtQueryTimerResolution))
            return false;

    d_->observers_NtQueryTimerResolution.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryValueKey(proc_t proc, const on_ZwQueryValueKey_fn& on_func)
{
    if(d_->observers_ZwQueryValueKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryValueKey@24", &on_ZwQueryValueKey))
            return false;

    d_->observers_ZwQueryValueKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryVirtualMemory(proc_t proc, const on_NtQueryVirtualMemory_fn& on_func)
{
    if(d_->observers_NtQueryVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryVirtualMemory@24", &on_NtQueryVirtualMemory))
            return false;

    d_->observers_NtQueryVirtualMemory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueryVolumeInformationFile(proc_t proc, const on_NtQueryVolumeInformationFile_fn& on_func)
{
    if(d_->observers_NtQueryVolumeInformationFile.empty())
        if(!register_callback_with(*d_, proc, "_NtQueryVolumeInformationFile@20", &on_NtQueryVolumeInformationFile))
            return false;

    d_->observers_NtQueryVolumeInformationFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueueApcThreadEx(proc_t proc, const on_NtQueueApcThreadEx_fn& on_func)
{
    if(d_->observers_NtQueueApcThreadEx.empty())
        if(!register_callback_with(*d_, proc, "_NtQueueApcThreadEx@24", &on_NtQueueApcThreadEx))
            return false;

    d_->observers_NtQueueApcThreadEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtQueueApcThread(proc_t proc, const on_NtQueueApcThread_fn& on_func)
{
    if(d_->observers_NtQueueApcThread.empty())
        if(!register_callback_with(*d_, proc, "_NtQueueApcThread@20", &on_NtQueueApcThread))
            return false;

    d_->observers_NtQueueApcThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRaiseException(proc_t proc, const on_ZwRaiseException_fn& on_func)
{
    if(d_->observers_ZwRaiseException.empty())
        if(!register_callback_with(*d_, proc, "_ZwRaiseException@12", &on_ZwRaiseException))
            return false;

    d_->observers_ZwRaiseException.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRaiseHardError(proc_t proc, const on_ZwRaiseHardError_fn& on_func)
{
    if(d_->observers_ZwRaiseHardError.empty())
        if(!register_callback_with(*d_, proc, "_ZwRaiseHardError@24", &on_ZwRaiseHardError))
            return false;

    d_->observers_ZwRaiseHardError.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtReadFile(proc_t proc, const on_NtReadFile_fn& on_func)
{
    if(d_->observers_NtReadFile.empty())
        if(!register_callback_with(*d_, proc, "_NtReadFile@36", &on_NtReadFile))
            return false;

    d_->observers_NtReadFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtReadFileScatter(proc_t proc, const on_NtReadFileScatter_fn& on_func)
{
    if(d_->observers_NtReadFileScatter.empty())
        if(!register_callback_with(*d_, proc, "_NtReadFileScatter@36", &on_NtReadFileScatter))
            return false;

    d_->observers_NtReadFileScatter.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwReadOnlyEnlistment(proc_t proc, const on_ZwReadOnlyEnlistment_fn& on_func)
{
    if(d_->observers_ZwReadOnlyEnlistment.empty())
        if(!register_callback_with(*d_, proc, "_ZwReadOnlyEnlistment@8", &on_ZwReadOnlyEnlistment))
            return false;

    d_->observers_ZwReadOnlyEnlistment.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwReadRequestData(proc_t proc, const on_ZwReadRequestData_fn& on_func)
{
    if(d_->observers_ZwReadRequestData.empty())
        if(!register_callback_with(*d_, proc, "_ZwReadRequestData@24", &on_ZwReadRequestData))
            return false;

    d_->observers_ZwReadRequestData.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtReadVirtualMemory(proc_t proc, const on_NtReadVirtualMemory_fn& on_func)
{
    if(d_->observers_NtReadVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "_NtReadVirtualMemory@20", &on_NtReadVirtualMemory))
            return false;

    d_->observers_NtReadVirtualMemory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtRecoverEnlistment(proc_t proc, const on_NtRecoverEnlistment_fn& on_func)
{
    if(d_->observers_NtRecoverEnlistment.empty())
        if(!register_callback_with(*d_, proc, "_NtRecoverEnlistment@8", &on_NtRecoverEnlistment))
            return false;

    d_->observers_NtRecoverEnlistment.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtRecoverResourceManager(proc_t proc, const on_NtRecoverResourceManager_fn& on_func)
{
    if(d_->observers_NtRecoverResourceManager.empty())
        if(!register_callback_with(*d_, proc, "_NtRecoverResourceManager@4", &on_NtRecoverResourceManager))
            return false;

    d_->observers_NtRecoverResourceManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRecoverTransactionManager(proc_t proc, const on_ZwRecoverTransactionManager_fn& on_func)
{
    if(d_->observers_ZwRecoverTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "_ZwRecoverTransactionManager@4", &on_ZwRecoverTransactionManager))
            return false;

    d_->observers_ZwRecoverTransactionManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRegisterProtocolAddressInformation(proc_t proc, const on_ZwRegisterProtocolAddressInformation_fn& on_func)
{
    if(d_->observers_ZwRegisterProtocolAddressInformation.empty())
        if(!register_callback_with(*d_, proc, "_ZwRegisterProtocolAddressInformation@20", &on_ZwRegisterProtocolAddressInformation))
            return false;

    d_->observers_ZwRegisterProtocolAddressInformation.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRegisterThreadTerminatePort(proc_t proc, const on_ZwRegisterThreadTerminatePort_fn& on_func)
{
    if(d_->observers_ZwRegisterThreadTerminatePort.empty())
        if(!register_callback_with(*d_, proc, "_ZwRegisterThreadTerminatePort@4", &on_ZwRegisterThreadTerminatePort))
            return false;

    d_->observers_ZwRegisterThreadTerminatePort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtReleaseKeyedEvent(proc_t proc, const on_NtReleaseKeyedEvent_fn& on_func)
{
    if(d_->observers_NtReleaseKeyedEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtReleaseKeyedEvent@16", &on_NtReleaseKeyedEvent))
            return false;

    d_->observers_NtReleaseKeyedEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwReleaseMutant(proc_t proc, const on_ZwReleaseMutant_fn& on_func)
{
    if(d_->observers_ZwReleaseMutant.empty())
        if(!register_callback_with(*d_, proc, "_ZwReleaseMutant@8", &on_ZwReleaseMutant))
            return false;

    d_->observers_ZwReleaseMutant.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtReleaseSemaphore(proc_t proc, const on_NtReleaseSemaphore_fn& on_func)
{
    if(d_->observers_NtReleaseSemaphore.empty())
        if(!register_callback_with(*d_, proc, "_NtReleaseSemaphore@12", &on_NtReleaseSemaphore))
            return false;

    d_->observers_NtReleaseSemaphore.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwReleaseWorkerFactoryWorker(proc_t proc, const on_ZwReleaseWorkerFactoryWorker_fn& on_func)
{
    if(d_->observers_ZwReleaseWorkerFactoryWorker.empty())
        if(!register_callback_with(*d_, proc, "_ZwReleaseWorkerFactoryWorker@4", &on_ZwReleaseWorkerFactoryWorker))
            return false;

    d_->observers_ZwReleaseWorkerFactoryWorker.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRemoveIoCompletionEx(proc_t proc, const on_ZwRemoveIoCompletionEx_fn& on_func)
{
    if(d_->observers_ZwRemoveIoCompletionEx.empty())
        if(!register_callback_with(*d_, proc, "_ZwRemoveIoCompletionEx@24", &on_ZwRemoveIoCompletionEx))
            return false;

    d_->observers_ZwRemoveIoCompletionEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRemoveIoCompletion(proc_t proc, const on_ZwRemoveIoCompletion_fn& on_func)
{
    if(d_->observers_ZwRemoveIoCompletion.empty())
        if(!register_callback_with(*d_, proc, "_ZwRemoveIoCompletion@20", &on_ZwRemoveIoCompletion))
            return false;

    d_->observers_ZwRemoveIoCompletion.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRemoveProcessDebug(proc_t proc, const on_ZwRemoveProcessDebug_fn& on_func)
{
    if(d_->observers_ZwRemoveProcessDebug.empty())
        if(!register_callback_with(*d_, proc, "_ZwRemoveProcessDebug@8", &on_ZwRemoveProcessDebug))
            return false;

    d_->observers_ZwRemoveProcessDebug.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRenameKey(proc_t proc, const on_ZwRenameKey_fn& on_func)
{
    if(d_->observers_ZwRenameKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwRenameKey@8", &on_ZwRenameKey))
            return false;

    d_->observers_ZwRenameKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtRenameTransactionManager(proc_t proc, const on_NtRenameTransactionManager_fn& on_func)
{
    if(d_->observers_NtRenameTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "_NtRenameTransactionManager@8", &on_NtRenameTransactionManager))
            return false;

    d_->observers_NtRenameTransactionManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwReplaceKey(proc_t proc, const on_ZwReplaceKey_fn& on_func)
{
    if(d_->observers_ZwReplaceKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwReplaceKey@12", &on_ZwReplaceKey))
            return false;

    d_->observers_ZwReplaceKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtReplacePartitionUnit(proc_t proc, const on_NtReplacePartitionUnit_fn& on_func)
{
    if(d_->observers_NtReplacePartitionUnit.empty())
        if(!register_callback_with(*d_, proc, "_NtReplacePartitionUnit@12", &on_NtReplacePartitionUnit))
            return false;

    d_->observers_NtReplacePartitionUnit.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwReplyPort(proc_t proc, const on_ZwReplyPort_fn& on_func)
{
    if(d_->observers_ZwReplyPort.empty())
        if(!register_callback_with(*d_, proc, "_ZwReplyPort@8", &on_ZwReplyPort))
            return false;

    d_->observers_ZwReplyPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtReplyWaitReceivePortEx(proc_t proc, const on_NtReplyWaitReceivePortEx_fn& on_func)
{
    if(d_->observers_NtReplyWaitReceivePortEx.empty())
        if(!register_callback_with(*d_, proc, "_NtReplyWaitReceivePortEx@20", &on_NtReplyWaitReceivePortEx))
            return false;

    d_->observers_NtReplyWaitReceivePortEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtReplyWaitReceivePort(proc_t proc, const on_NtReplyWaitReceivePort_fn& on_func)
{
    if(d_->observers_NtReplyWaitReceivePort.empty())
        if(!register_callback_with(*d_, proc, "_NtReplyWaitReceivePort@16", &on_NtReplyWaitReceivePort))
            return false;

    d_->observers_NtReplyWaitReceivePort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtReplyWaitReplyPort(proc_t proc, const on_NtReplyWaitReplyPort_fn& on_func)
{
    if(d_->observers_NtReplyWaitReplyPort.empty())
        if(!register_callback_with(*d_, proc, "_NtReplyWaitReplyPort@8", &on_NtReplyWaitReplyPort))
            return false;

    d_->observers_NtReplyWaitReplyPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRequestPort(proc_t proc, const on_ZwRequestPort_fn& on_func)
{
    if(d_->observers_ZwRequestPort.empty())
        if(!register_callback_with(*d_, proc, "_ZwRequestPort@8", &on_ZwRequestPort))
            return false;

    d_->observers_ZwRequestPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtRequestWaitReplyPort(proc_t proc, const on_NtRequestWaitReplyPort_fn& on_func)
{
    if(d_->observers_NtRequestWaitReplyPort.empty())
        if(!register_callback_with(*d_, proc, "_NtRequestWaitReplyPort@12", &on_NtRequestWaitReplyPort))
            return false;

    d_->observers_NtRequestWaitReplyPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtResetEvent(proc_t proc, const on_NtResetEvent_fn& on_func)
{
    if(d_->observers_NtResetEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtResetEvent@8", &on_NtResetEvent))
            return false;

    d_->observers_NtResetEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwResetWriteWatch(proc_t proc, const on_ZwResetWriteWatch_fn& on_func)
{
    if(d_->observers_ZwResetWriteWatch.empty())
        if(!register_callback_with(*d_, proc, "_ZwResetWriteWatch@12", &on_ZwResetWriteWatch))
            return false;

    d_->observers_ZwResetWriteWatch.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtRestoreKey(proc_t proc, const on_NtRestoreKey_fn& on_func)
{
    if(d_->observers_NtRestoreKey.empty())
        if(!register_callback_with(*d_, proc, "_NtRestoreKey@12", &on_NtRestoreKey))
            return false;

    d_->observers_NtRestoreKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwResumeProcess(proc_t proc, const on_ZwResumeProcess_fn& on_func)
{
    if(d_->observers_ZwResumeProcess.empty())
        if(!register_callback_with(*d_, proc, "_ZwResumeProcess@4", &on_ZwResumeProcess))
            return false;

    d_->observers_ZwResumeProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwResumeThread(proc_t proc, const on_ZwResumeThread_fn& on_func)
{
    if(d_->observers_ZwResumeThread.empty())
        if(!register_callback_with(*d_, proc, "_ZwResumeThread@8", &on_ZwResumeThread))
            return false;

    d_->observers_ZwResumeThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwRollbackComplete(proc_t proc, const on_ZwRollbackComplete_fn& on_func)
{
    if(d_->observers_ZwRollbackComplete.empty())
        if(!register_callback_with(*d_, proc, "_ZwRollbackComplete@8", &on_ZwRollbackComplete))
            return false;

    d_->observers_ZwRollbackComplete.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtRollbackEnlistment(proc_t proc, const on_NtRollbackEnlistment_fn& on_func)
{
    if(d_->observers_NtRollbackEnlistment.empty())
        if(!register_callback_with(*d_, proc, "_NtRollbackEnlistment@8", &on_NtRollbackEnlistment))
            return false;

    d_->observers_NtRollbackEnlistment.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtRollbackTransaction(proc_t proc, const on_NtRollbackTransaction_fn& on_func)
{
    if(d_->observers_NtRollbackTransaction.empty())
        if(!register_callback_with(*d_, proc, "_NtRollbackTransaction@8", &on_NtRollbackTransaction))
            return false;

    d_->observers_NtRollbackTransaction.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtRollforwardTransactionManager(proc_t proc, const on_NtRollforwardTransactionManager_fn& on_func)
{
    if(d_->observers_NtRollforwardTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "_NtRollforwardTransactionManager@8", &on_NtRollforwardTransactionManager))
            return false;

    d_->observers_NtRollforwardTransactionManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSaveKeyEx(proc_t proc, const on_NtSaveKeyEx_fn& on_func)
{
    if(d_->observers_NtSaveKeyEx.empty())
        if(!register_callback_with(*d_, proc, "_NtSaveKeyEx@12", &on_NtSaveKeyEx))
            return false;

    d_->observers_NtSaveKeyEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSaveKey(proc_t proc, const on_NtSaveKey_fn& on_func)
{
    if(d_->observers_NtSaveKey.empty())
        if(!register_callback_with(*d_, proc, "_NtSaveKey@8", &on_NtSaveKey))
            return false;

    d_->observers_NtSaveKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSaveMergedKeys(proc_t proc, const on_NtSaveMergedKeys_fn& on_func)
{
    if(d_->observers_NtSaveMergedKeys.empty())
        if(!register_callback_with(*d_, proc, "_NtSaveMergedKeys@12", &on_NtSaveMergedKeys))
            return false;

    d_->observers_NtSaveMergedKeys.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSecureConnectPort(proc_t proc, const on_NtSecureConnectPort_fn& on_func)
{
    if(d_->observers_NtSecureConnectPort.empty())
        if(!register_callback_with(*d_, proc, "_NtSecureConnectPort@36", &on_NtSecureConnectPort))
            return false;

    d_->observers_NtSecureConnectPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetBootEntryOrder(proc_t proc, const on_ZwSetBootEntryOrder_fn& on_func)
{
    if(d_->observers_ZwSetBootEntryOrder.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetBootEntryOrder@8", &on_ZwSetBootEntryOrder))
            return false;

    d_->observers_ZwSetBootEntryOrder.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetBootOptions(proc_t proc, const on_ZwSetBootOptions_fn& on_func)
{
    if(d_->observers_ZwSetBootOptions.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetBootOptions@8", &on_ZwSetBootOptions))
            return false;

    d_->observers_ZwSetBootOptions.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetContextThread(proc_t proc, const on_ZwSetContextThread_fn& on_func)
{
    if(d_->observers_ZwSetContextThread.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetContextThread@8", &on_ZwSetContextThread))
            return false;

    d_->observers_ZwSetContextThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetDebugFilterState(proc_t proc, const on_NtSetDebugFilterState_fn& on_func)
{
    if(d_->observers_NtSetDebugFilterState.empty())
        if(!register_callback_with(*d_, proc, "_NtSetDebugFilterState@12", &on_NtSetDebugFilterState))
            return false;

    d_->observers_NtSetDebugFilterState.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetDefaultHardErrorPort(proc_t proc, const on_NtSetDefaultHardErrorPort_fn& on_func)
{
    if(d_->observers_NtSetDefaultHardErrorPort.empty())
        if(!register_callback_with(*d_, proc, "_NtSetDefaultHardErrorPort@4", &on_NtSetDefaultHardErrorPort))
            return false;

    d_->observers_NtSetDefaultHardErrorPort.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetDefaultLocale(proc_t proc, const on_NtSetDefaultLocale_fn& on_func)
{
    if(d_->observers_NtSetDefaultLocale.empty())
        if(!register_callback_with(*d_, proc, "_NtSetDefaultLocale@8", &on_NtSetDefaultLocale))
            return false;

    d_->observers_NtSetDefaultLocale.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetDefaultUILanguage(proc_t proc, const on_ZwSetDefaultUILanguage_fn& on_func)
{
    if(d_->observers_ZwSetDefaultUILanguage.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetDefaultUILanguage@4", &on_ZwSetDefaultUILanguage))
            return false;

    d_->observers_ZwSetDefaultUILanguage.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetDriverEntryOrder(proc_t proc, const on_NtSetDriverEntryOrder_fn& on_func)
{
    if(d_->observers_NtSetDriverEntryOrder.empty())
        if(!register_callback_with(*d_, proc, "_NtSetDriverEntryOrder@8", &on_NtSetDriverEntryOrder))
            return false;

    d_->observers_NtSetDriverEntryOrder.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetEaFile(proc_t proc, const on_ZwSetEaFile_fn& on_func)
{
    if(d_->observers_ZwSetEaFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetEaFile@16", &on_ZwSetEaFile))
            return false;

    d_->observers_ZwSetEaFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetEventBoostPriority(proc_t proc, const on_NtSetEventBoostPriority_fn& on_func)
{
    if(d_->observers_NtSetEventBoostPriority.empty())
        if(!register_callback_with(*d_, proc, "_NtSetEventBoostPriority@4", &on_NtSetEventBoostPriority))
            return false;

    d_->observers_NtSetEventBoostPriority.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetEvent(proc_t proc, const on_NtSetEvent_fn& on_func)
{
    if(d_->observers_NtSetEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtSetEvent@8", &on_NtSetEvent))
            return false;

    d_->observers_NtSetEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetHighEventPair(proc_t proc, const on_NtSetHighEventPair_fn& on_func)
{
    if(d_->observers_NtSetHighEventPair.empty())
        if(!register_callback_with(*d_, proc, "_NtSetHighEventPair@4", &on_NtSetHighEventPair))
            return false;

    d_->observers_NtSetHighEventPair.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetHighWaitLowEventPair(proc_t proc, const on_NtSetHighWaitLowEventPair_fn& on_func)
{
    if(d_->observers_NtSetHighWaitLowEventPair.empty())
        if(!register_callback_with(*d_, proc, "_NtSetHighWaitLowEventPair@4", &on_NtSetHighWaitLowEventPair))
            return false;

    d_->observers_NtSetHighWaitLowEventPair.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationDebugObject(proc_t proc, const on_ZwSetInformationDebugObject_fn& on_func)
{
    if(d_->observers_ZwSetInformationDebugObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationDebugObject@20", &on_ZwSetInformationDebugObject))
            return false;

    d_->observers_ZwSetInformationDebugObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetInformationEnlistment(proc_t proc, const on_NtSetInformationEnlistment_fn& on_func)
{
    if(d_->observers_NtSetInformationEnlistment.empty())
        if(!register_callback_with(*d_, proc, "_NtSetInformationEnlistment@16", &on_NtSetInformationEnlistment))
            return false;

    d_->observers_NtSetInformationEnlistment.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationFile(proc_t proc, const on_ZwSetInformationFile_fn& on_func)
{
    if(d_->observers_ZwSetInformationFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationFile@20", &on_ZwSetInformationFile))
            return false;

    d_->observers_ZwSetInformationFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationJobObject(proc_t proc, const on_ZwSetInformationJobObject_fn& on_func)
{
    if(d_->observers_ZwSetInformationJobObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationJobObject@16", &on_ZwSetInformationJobObject))
            return false;

    d_->observers_ZwSetInformationJobObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationKey(proc_t proc, const on_ZwSetInformationKey_fn& on_func)
{
    if(d_->observers_ZwSetInformationKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationKey@16", &on_ZwSetInformationKey))
            return false;

    d_->observers_ZwSetInformationKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationObject(proc_t proc, const on_ZwSetInformationObject_fn& on_func)
{
    if(d_->observers_ZwSetInformationObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationObject@16", &on_ZwSetInformationObject))
            return false;

    d_->observers_ZwSetInformationObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationProcess(proc_t proc, const on_ZwSetInformationProcess_fn& on_func)
{
    if(d_->observers_ZwSetInformationProcess.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationProcess@16", &on_ZwSetInformationProcess))
            return false;

    d_->observers_ZwSetInformationProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationResourceManager(proc_t proc, const on_ZwSetInformationResourceManager_fn& on_func)
{
    if(d_->observers_ZwSetInformationResourceManager.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationResourceManager@16", &on_ZwSetInformationResourceManager))
            return false;

    d_->observers_ZwSetInformationResourceManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationThread(proc_t proc, const on_ZwSetInformationThread_fn& on_func)
{
    if(d_->observers_ZwSetInformationThread.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationThread@16", &on_ZwSetInformationThread))
            return false;

    d_->observers_ZwSetInformationThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationToken(proc_t proc, const on_ZwSetInformationToken_fn& on_func)
{
    if(d_->observers_ZwSetInformationToken.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationToken@16", &on_ZwSetInformationToken))
            return false;

    d_->observers_ZwSetInformationToken.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationTransaction(proc_t proc, const on_ZwSetInformationTransaction_fn& on_func)
{
    if(d_->observers_ZwSetInformationTransaction.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationTransaction@16", &on_ZwSetInformationTransaction))
            return false;

    d_->observers_ZwSetInformationTransaction.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationTransactionManager(proc_t proc, const on_ZwSetInformationTransactionManager_fn& on_func)
{
    if(d_->observers_ZwSetInformationTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationTransactionManager@16", &on_ZwSetInformationTransactionManager))
            return false;

    d_->observers_ZwSetInformationTransactionManager.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetInformationWorkerFactory(proc_t proc, const on_ZwSetInformationWorkerFactory_fn& on_func)
{
    if(d_->observers_ZwSetInformationWorkerFactory.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetInformationWorkerFactory@16", &on_ZwSetInformationWorkerFactory))
            return false;

    d_->observers_ZwSetInformationWorkerFactory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetIntervalProfile(proc_t proc, const on_NtSetIntervalProfile_fn& on_func)
{
    if(d_->observers_NtSetIntervalProfile.empty())
        if(!register_callback_with(*d_, proc, "_NtSetIntervalProfile@8", &on_NtSetIntervalProfile))
            return false;

    d_->observers_NtSetIntervalProfile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetIoCompletionEx(proc_t proc, const on_NtSetIoCompletionEx_fn& on_func)
{
    if(d_->observers_NtSetIoCompletionEx.empty())
        if(!register_callback_with(*d_, proc, "_NtSetIoCompletionEx@24", &on_NtSetIoCompletionEx))
            return false;

    d_->observers_NtSetIoCompletionEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetIoCompletion(proc_t proc, const on_NtSetIoCompletion_fn& on_func)
{
    if(d_->observers_NtSetIoCompletion.empty())
        if(!register_callback_with(*d_, proc, "_NtSetIoCompletion@20", &on_NtSetIoCompletion))
            return false;

    d_->observers_NtSetIoCompletion.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetLdtEntries(proc_t proc, const on_ZwSetLdtEntries_fn& on_func)
{
    if(d_->observers_ZwSetLdtEntries.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetLdtEntries@24", &on_ZwSetLdtEntries))
            return false;

    d_->observers_ZwSetLdtEntries.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetLowEventPair(proc_t proc, const on_ZwSetLowEventPair_fn& on_func)
{
    if(d_->observers_ZwSetLowEventPair.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetLowEventPair@4", &on_ZwSetLowEventPair))
            return false;

    d_->observers_ZwSetLowEventPair.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetLowWaitHighEventPair(proc_t proc, const on_ZwSetLowWaitHighEventPair_fn& on_func)
{
    if(d_->observers_ZwSetLowWaitHighEventPair.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetLowWaitHighEventPair@4", &on_ZwSetLowWaitHighEventPair))
            return false;

    d_->observers_ZwSetLowWaitHighEventPair.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetQuotaInformationFile(proc_t proc, const on_ZwSetQuotaInformationFile_fn& on_func)
{
    if(d_->observers_ZwSetQuotaInformationFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetQuotaInformationFile@16", &on_ZwSetQuotaInformationFile))
            return false;

    d_->observers_ZwSetQuotaInformationFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetSecurityObject(proc_t proc, const on_NtSetSecurityObject_fn& on_func)
{
    if(d_->observers_NtSetSecurityObject.empty())
        if(!register_callback_with(*d_, proc, "_NtSetSecurityObject@12", &on_NtSetSecurityObject))
            return false;

    d_->observers_NtSetSecurityObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetSystemEnvironmentValueEx(proc_t proc, const on_ZwSetSystemEnvironmentValueEx_fn& on_func)
{
    if(d_->observers_ZwSetSystemEnvironmentValueEx.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetSystemEnvironmentValueEx@20", &on_ZwSetSystemEnvironmentValueEx))
            return false;

    d_->observers_ZwSetSystemEnvironmentValueEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetSystemEnvironmentValue(proc_t proc, const on_ZwSetSystemEnvironmentValue_fn& on_func)
{
    if(d_->observers_ZwSetSystemEnvironmentValue.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetSystemEnvironmentValue@8", &on_ZwSetSystemEnvironmentValue))
            return false;

    d_->observers_ZwSetSystemEnvironmentValue.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetSystemInformation(proc_t proc, const on_ZwSetSystemInformation_fn& on_func)
{
    if(d_->observers_ZwSetSystemInformation.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetSystemInformation@12", &on_ZwSetSystemInformation))
            return false;

    d_->observers_ZwSetSystemInformation.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetSystemPowerState(proc_t proc, const on_ZwSetSystemPowerState_fn& on_func)
{
    if(d_->observers_ZwSetSystemPowerState.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetSystemPowerState@12", &on_ZwSetSystemPowerState))
            return false;

    d_->observers_ZwSetSystemPowerState.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetSystemTime(proc_t proc, const on_ZwSetSystemTime_fn& on_func)
{
    if(d_->observers_ZwSetSystemTime.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetSystemTime@8", &on_ZwSetSystemTime))
            return false;

    d_->observers_ZwSetSystemTime.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetThreadExecutionState(proc_t proc, const on_ZwSetThreadExecutionState_fn& on_func)
{
    if(d_->observers_ZwSetThreadExecutionState.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetThreadExecutionState@8", &on_ZwSetThreadExecutionState))
            return false;

    d_->observers_ZwSetThreadExecutionState.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetTimerEx(proc_t proc, const on_ZwSetTimerEx_fn& on_func)
{
    if(d_->observers_ZwSetTimerEx.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetTimerEx@16", &on_ZwSetTimerEx))
            return false;

    d_->observers_ZwSetTimerEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetTimer(proc_t proc, const on_ZwSetTimer_fn& on_func)
{
    if(d_->observers_ZwSetTimer.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetTimer@28", &on_ZwSetTimer))
            return false;

    d_->observers_ZwSetTimer.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetTimerResolution(proc_t proc, const on_NtSetTimerResolution_fn& on_func)
{
    if(d_->observers_NtSetTimerResolution.empty())
        if(!register_callback_with(*d_, proc, "_NtSetTimerResolution@12", &on_NtSetTimerResolution))
            return false;

    d_->observers_NtSetTimerResolution.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetUuidSeed(proc_t proc, const on_ZwSetUuidSeed_fn& on_func)
{
    if(d_->observers_ZwSetUuidSeed.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetUuidSeed@4", &on_ZwSetUuidSeed))
            return false;

    d_->observers_ZwSetUuidSeed.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSetValueKey(proc_t proc, const on_ZwSetValueKey_fn& on_func)
{
    if(d_->observers_ZwSetValueKey.empty())
        if(!register_callback_with(*d_, proc, "_ZwSetValueKey@24", &on_ZwSetValueKey))
            return false;

    d_->observers_ZwSetValueKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSetVolumeInformationFile(proc_t proc, const on_NtSetVolumeInformationFile_fn& on_func)
{
    if(d_->observers_NtSetVolumeInformationFile.empty())
        if(!register_callback_with(*d_, proc, "_NtSetVolumeInformationFile@20", &on_NtSetVolumeInformationFile))
            return false;

    d_->observers_NtSetVolumeInformationFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwShutdownSystem(proc_t proc, const on_ZwShutdownSystem_fn& on_func)
{
    if(d_->observers_ZwShutdownSystem.empty())
        if(!register_callback_with(*d_, proc, "_ZwShutdownSystem@4", &on_ZwShutdownSystem))
            return false;

    d_->observers_ZwShutdownSystem.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtShutdownWorkerFactory(proc_t proc, const on_NtShutdownWorkerFactory_fn& on_func)
{
    if(d_->observers_NtShutdownWorkerFactory.empty())
        if(!register_callback_with(*d_, proc, "_NtShutdownWorkerFactory@8", &on_NtShutdownWorkerFactory))
            return false;

    d_->observers_NtShutdownWorkerFactory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSignalAndWaitForSingleObject(proc_t proc, const on_ZwSignalAndWaitForSingleObject_fn& on_func)
{
    if(d_->observers_ZwSignalAndWaitForSingleObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwSignalAndWaitForSingleObject@16", &on_ZwSignalAndWaitForSingleObject))
            return false;

    d_->observers_ZwSignalAndWaitForSingleObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSinglePhaseReject(proc_t proc, const on_ZwSinglePhaseReject_fn& on_func)
{
    if(d_->observers_ZwSinglePhaseReject.empty())
        if(!register_callback_with(*d_, proc, "_ZwSinglePhaseReject@8", &on_ZwSinglePhaseReject))
            return false;

    d_->observers_ZwSinglePhaseReject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtStartProfile(proc_t proc, const on_NtStartProfile_fn& on_func)
{
    if(d_->observers_NtStartProfile.empty())
        if(!register_callback_with(*d_, proc, "_NtStartProfile@4", &on_NtStartProfile))
            return false;

    d_->observers_NtStartProfile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwStopProfile(proc_t proc, const on_ZwStopProfile_fn& on_func)
{
    if(d_->observers_ZwStopProfile.empty())
        if(!register_callback_with(*d_, proc, "_ZwStopProfile@4", &on_ZwStopProfile))
            return false;

    d_->observers_ZwStopProfile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSuspendProcess(proc_t proc, const on_ZwSuspendProcess_fn& on_func)
{
    if(d_->observers_ZwSuspendProcess.empty())
        if(!register_callback_with(*d_, proc, "_ZwSuspendProcess@4", &on_ZwSuspendProcess))
            return false;

    d_->observers_ZwSuspendProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwSuspendThread(proc_t proc, const on_ZwSuspendThread_fn& on_func)
{
    if(d_->observers_ZwSuspendThread.empty())
        if(!register_callback_with(*d_, proc, "_ZwSuspendThread@8", &on_ZwSuspendThread))
            return false;

    d_->observers_ZwSuspendThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSystemDebugControl(proc_t proc, const on_NtSystemDebugControl_fn& on_func)
{
    if(d_->observers_NtSystemDebugControl.empty())
        if(!register_callback_with(*d_, proc, "_NtSystemDebugControl@24", &on_NtSystemDebugControl))
            return false;

    d_->observers_NtSystemDebugControl.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwTerminateJobObject(proc_t proc, const on_ZwTerminateJobObject_fn& on_func)
{
    if(d_->observers_ZwTerminateJobObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwTerminateJobObject@8", &on_ZwTerminateJobObject))
            return false;

    d_->observers_ZwTerminateJobObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwTerminateProcess(proc_t proc, const on_ZwTerminateProcess_fn& on_func)
{
    if(d_->observers_ZwTerminateProcess.empty())
        if(!register_callback_with(*d_, proc, "_ZwTerminateProcess@8", &on_ZwTerminateProcess))
            return false;

    d_->observers_ZwTerminateProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwTerminateThread(proc_t proc, const on_ZwTerminateThread_fn& on_func)
{
    if(d_->observers_ZwTerminateThread.empty())
        if(!register_callback_with(*d_, proc, "_ZwTerminateThread@8", &on_ZwTerminateThread))
            return false;

    d_->observers_ZwTerminateThread.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwTraceControl(proc_t proc, const on_ZwTraceControl_fn& on_func)
{
    if(d_->observers_ZwTraceControl.empty())
        if(!register_callback_with(*d_, proc, "_ZwTraceControl@24", &on_ZwTraceControl))
            return false;

    d_->observers_ZwTraceControl.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtTraceEvent(proc_t proc, const on_NtTraceEvent_fn& on_func)
{
    if(d_->observers_NtTraceEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtTraceEvent@16", &on_NtTraceEvent))
            return false;

    d_->observers_NtTraceEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtTranslateFilePath(proc_t proc, const on_NtTranslateFilePath_fn& on_func)
{
    if(d_->observers_NtTranslateFilePath.empty())
        if(!register_callback_with(*d_, proc, "_NtTranslateFilePath@16", &on_NtTranslateFilePath))
            return false;

    d_->observers_NtTranslateFilePath.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwUnloadDriver(proc_t proc, const on_ZwUnloadDriver_fn& on_func)
{
    if(d_->observers_ZwUnloadDriver.empty())
        if(!register_callback_with(*d_, proc, "_ZwUnloadDriver@4", &on_ZwUnloadDriver))
            return false;

    d_->observers_ZwUnloadDriver.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwUnloadKey2(proc_t proc, const on_ZwUnloadKey2_fn& on_func)
{
    if(d_->observers_ZwUnloadKey2.empty())
        if(!register_callback_with(*d_, proc, "_ZwUnloadKey2@8", &on_ZwUnloadKey2))
            return false;

    d_->observers_ZwUnloadKey2.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwUnloadKeyEx(proc_t proc, const on_ZwUnloadKeyEx_fn& on_func)
{
    if(d_->observers_ZwUnloadKeyEx.empty())
        if(!register_callback_with(*d_, proc, "_ZwUnloadKeyEx@8", &on_ZwUnloadKeyEx))
            return false;

    d_->observers_ZwUnloadKeyEx.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtUnloadKey(proc_t proc, const on_NtUnloadKey_fn& on_func)
{
    if(d_->observers_NtUnloadKey.empty())
        if(!register_callback_with(*d_, proc, "_NtUnloadKey@4", &on_NtUnloadKey))
            return false;

    d_->observers_NtUnloadKey.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwUnlockFile(proc_t proc, const on_ZwUnlockFile_fn& on_func)
{
    if(d_->observers_ZwUnlockFile.empty())
        if(!register_callback_with(*d_, proc, "_ZwUnlockFile@20", &on_ZwUnlockFile))
            return false;

    d_->observers_ZwUnlockFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtUnlockVirtualMemory(proc_t proc, const on_NtUnlockVirtualMemory_fn& on_func)
{
    if(d_->observers_NtUnlockVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "_NtUnlockVirtualMemory@16", &on_NtUnlockVirtualMemory))
            return false;

    d_->observers_NtUnlockVirtualMemory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtUnmapViewOfSection(proc_t proc, const on_NtUnmapViewOfSection_fn& on_func)
{
    if(d_->observers_NtUnmapViewOfSection.empty())
        if(!register_callback_with(*d_, proc, "_NtUnmapViewOfSection@8", &on_NtUnmapViewOfSection))
            return false;

    d_->observers_NtUnmapViewOfSection.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtVdmControl(proc_t proc, const on_NtVdmControl_fn& on_func)
{
    if(d_->observers_NtVdmControl.empty())
        if(!register_callback_with(*d_, proc, "_NtVdmControl@8", &on_NtVdmControl))
            return false;

    d_->observers_NtVdmControl.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWaitForDebugEvent(proc_t proc, const on_NtWaitForDebugEvent_fn& on_func)
{
    if(d_->observers_NtWaitForDebugEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtWaitForDebugEvent@16", &on_NtWaitForDebugEvent))
            return false;

    d_->observers_NtWaitForDebugEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWaitForKeyedEvent(proc_t proc, const on_NtWaitForKeyedEvent_fn& on_func)
{
    if(d_->observers_NtWaitForKeyedEvent.empty())
        if(!register_callback_with(*d_, proc, "_NtWaitForKeyedEvent@16", &on_NtWaitForKeyedEvent))
            return false;

    d_->observers_NtWaitForKeyedEvent.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWaitForMultipleObjects32(proc_t proc, const on_NtWaitForMultipleObjects32_fn& on_func)
{
    if(d_->observers_NtWaitForMultipleObjects32.empty())
        if(!register_callback_with(*d_, proc, "_NtWaitForMultipleObjects32@20", &on_NtWaitForMultipleObjects32))
            return false;

    d_->observers_NtWaitForMultipleObjects32.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWaitForMultipleObjects(proc_t proc, const on_NtWaitForMultipleObjects_fn& on_func)
{
    if(d_->observers_NtWaitForMultipleObjects.empty())
        if(!register_callback_with(*d_, proc, "_NtWaitForMultipleObjects@20", &on_NtWaitForMultipleObjects))
            return false;

    d_->observers_NtWaitForMultipleObjects.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwWaitForSingleObject(proc_t proc, const on_ZwWaitForSingleObject_fn& on_func)
{
    if(d_->observers_ZwWaitForSingleObject.empty())
        if(!register_callback_with(*d_, proc, "_ZwWaitForSingleObject@12", &on_ZwWaitForSingleObject))
            return false;

    d_->observers_ZwWaitForSingleObject.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWaitForWorkViaWorkerFactory(proc_t proc, const on_NtWaitForWorkViaWorkerFactory_fn& on_func)
{
    if(d_->observers_NtWaitForWorkViaWorkerFactory.empty())
        if(!register_callback_with(*d_, proc, "_NtWaitForWorkViaWorkerFactory@20", &on_NtWaitForWorkViaWorkerFactory))
            return false;

    d_->observers_NtWaitForWorkViaWorkerFactory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwWaitHighEventPair(proc_t proc, const on_ZwWaitHighEventPair_fn& on_func)
{
    if(d_->observers_ZwWaitHighEventPair.empty())
        if(!register_callback_with(*d_, proc, "_ZwWaitHighEventPair@4", &on_ZwWaitHighEventPair))
            return false;

    d_->observers_ZwWaitHighEventPair.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWaitLowEventPair(proc_t proc, const on_NtWaitLowEventPair_fn& on_func)
{
    if(d_->observers_NtWaitLowEventPair.empty())
        if(!register_callback_with(*d_, proc, "_NtWaitLowEventPair@4", &on_NtWaitLowEventPair))
            return false;

    d_->observers_NtWaitLowEventPair.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWorkerFactoryWorkerReady(proc_t proc, const on_NtWorkerFactoryWorkerReady_fn& on_func)
{
    if(d_->observers_NtWorkerFactoryWorkerReady.empty())
        if(!register_callback_with(*d_, proc, "_NtWorkerFactoryWorkerReady@4", &on_NtWorkerFactoryWorkerReady))
            return false;

    d_->observers_NtWorkerFactoryWorkerReady.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWriteFileGather(proc_t proc, const on_NtWriteFileGather_fn& on_func)
{
    if(d_->observers_NtWriteFileGather.empty())
        if(!register_callback_with(*d_, proc, "_NtWriteFileGather@36", &on_NtWriteFileGather))
            return false;

    d_->observers_NtWriteFileGather.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWriteFile(proc_t proc, const on_NtWriteFile_fn& on_func)
{
    if(d_->observers_NtWriteFile.empty())
        if(!register_callback_with(*d_, proc, "_NtWriteFile@36", &on_NtWriteFile))
            return false;

    d_->observers_NtWriteFile.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWriteRequestData(proc_t proc, const on_NtWriteRequestData_fn& on_func)
{
    if(d_->observers_NtWriteRequestData.empty())
        if(!register_callback_with(*d_, proc, "_NtWriteRequestData@24", &on_NtWriteRequestData))
            return false;

    d_->observers_NtWriteRequestData.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtWriteVirtualMemory(proc_t proc, const on_NtWriteVirtualMemory_fn& on_func)
{
    if(d_->observers_NtWriteVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "_NtWriteVirtualMemory@20", &on_NtWriteVirtualMemory))
            return false;

    d_->observers_NtWriteVirtualMemory.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtDisableLastKnownGood(proc_t proc, const on_NtDisableLastKnownGood_fn& on_func)
{
    if(d_->observers_NtDisableLastKnownGood.empty())
        if(!register_callback_with(*d_, proc, "_NtDisableLastKnownGood@0", &on_NtDisableLastKnownGood))
            return false;

    d_->observers_NtDisableLastKnownGood.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtEnableLastKnownGood(proc_t proc, const on_NtEnableLastKnownGood_fn& on_func)
{
    if(d_->observers_NtEnableLastKnownGood.empty())
        if(!register_callback_with(*d_, proc, "_NtEnableLastKnownGood@0", &on_NtEnableLastKnownGood))
            return false;

    d_->observers_NtEnableLastKnownGood.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwFlushProcessWriteBuffers(proc_t proc, const on_ZwFlushProcessWriteBuffers_fn& on_func)
{
    if(d_->observers_ZwFlushProcessWriteBuffers.empty())
        if(!register_callback_with(*d_, proc, "_ZwFlushProcessWriteBuffers@0", &on_ZwFlushProcessWriteBuffers))
            return false;

    d_->observers_ZwFlushProcessWriteBuffers.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtFlushWriteBuffer(proc_t proc, const on_NtFlushWriteBuffer_fn& on_func)
{
    if(d_->observers_NtFlushWriteBuffer.empty())
        if(!register_callback_with(*d_, proc, "_NtFlushWriteBuffer@0", &on_NtFlushWriteBuffer))
            return false;

    d_->observers_NtFlushWriteBuffer.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtGetCurrentProcessorNumber(proc_t proc, const on_NtGetCurrentProcessorNumber_fn& on_func)
{
    if(d_->observers_NtGetCurrentProcessorNumber.empty())
        if(!register_callback_with(*d_, proc, "_NtGetCurrentProcessorNumber@0", &on_NtGetCurrentProcessorNumber))
            return false;

    d_->observers_NtGetCurrentProcessorNumber.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtIsSystemResumeAutomatic(proc_t proc, const on_NtIsSystemResumeAutomatic_fn& on_func)
{
    if(d_->observers_NtIsSystemResumeAutomatic.empty())
        if(!register_callback_with(*d_, proc, "_NtIsSystemResumeAutomatic@0", &on_NtIsSystemResumeAutomatic))
            return false;

    d_->observers_NtIsSystemResumeAutomatic.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwIsUILanguageComitted(proc_t proc, const on_ZwIsUILanguageComitted_fn& on_func)
{
    if(d_->observers_ZwIsUILanguageComitted.empty())
        if(!register_callback_with(*d_, proc, "_ZwIsUILanguageComitted@0", &on_ZwIsUILanguageComitted))
            return false;

    d_->observers_ZwIsUILanguageComitted.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwQueryPortInformationProcess(proc_t proc, const on_ZwQueryPortInformationProcess_fn& on_func)
{
    if(d_->observers_ZwQueryPortInformationProcess.empty())
        if(!register_callback_with(*d_, proc, "_ZwQueryPortInformationProcess@0", &on_ZwQueryPortInformationProcess))
            return false;

    d_->observers_ZwQueryPortInformationProcess.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtSerializeBoot(proc_t proc, const on_NtSerializeBoot_fn& on_func)
{
    if(d_->observers_NtSerializeBoot.empty())
        if(!register_callback_with(*d_, proc, "_NtSerializeBoot@0", &on_NtSerializeBoot))
            return false;

    d_->observers_NtSerializeBoot.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtTestAlert(proc_t proc, const on_NtTestAlert_fn& on_func)
{
    if(d_->observers_NtTestAlert.empty())
        if(!register_callback_with(*d_, proc, "_NtTestAlert@0", &on_NtTestAlert))
            return false;

    d_->observers_NtTestAlert.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwThawRegistry(proc_t proc, const on_ZwThawRegistry_fn& on_func)
{
    if(d_->observers_ZwThawRegistry.empty())
        if(!register_callback_with(*d_, proc, "_ZwThawRegistry@0", &on_ZwThawRegistry))
            return false;

    d_->observers_ZwThawRegistry.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_NtThawTransactions(proc_t proc, const on_NtThawTransactions_fn& on_func)
{
    if(d_->observers_NtThawTransactions.empty())
        if(!register_callback_with(*d_, proc, "_NtThawTransactions@0", &on_NtThawTransactions))
            return false;

    d_->observers_NtThawTransactions.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwUmsThreadYield(proc_t proc, const on_ZwUmsThreadYield_fn& on_func)
{
    if(d_->observers_ZwUmsThreadYield.empty())
        if(!register_callback_with(*d_, proc, "_ZwUmsThreadYield@4", &on_ZwUmsThreadYield))
            return false;

    d_->observers_ZwUmsThreadYield.push_back(on_func);
    return true;
}

bool nt32::syscalls32::register_ZwYieldExecution(proc_t proc, const on_ZwYieldExecution_fn& on_func)
{
    if(d_->observers_ZwYieldExecution.empty())
        if(!register_callback_with(*d_, proc, "_ZwYieldExecution@0", &on_ZwYieldExecution))
            return false;

    d_->observers_ZwYieldExecution.push_back(on_func);
    return true;
}


bool nt32::syscalls32::register_all(proc_t proc, const nt32::syscalls32::on_call_fn& on_call)
{
    Data::Breakpoints breakpoints;
    for(const auto cfg : g_callcfgs)
        if(const auto bp = register_callback(*d_, proc, cfg.name, [=]{ on_call(cfg); }))
            breakpoints.emplace_back(bp);

    d_->breakpoints.insert(d_->breakpoints.end(), breakpoints.begin(), breakpoints.end());
    return true;
}
