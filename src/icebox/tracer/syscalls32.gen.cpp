#include "syscalls32.gen.hpp"

#define FDP_MODULE "syscalls32"
#include "log.hpp"
#include "os.hpp"

#include <map>

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

    using id_t      = nt32::syscalls32::id_t;
    using Listeners = std::multimap<id_t, core::Breakpoint>;
}

struct nt32::syscalls32::Data
{
    Data(core::Core& core, std::string_view module);

    core::Core& core;
    std::string module;
    Listeners   listeners;
    uint64_t    last_id;
};

nt32::syscalls32::Data::Data(core::Core& core, std::string_view module)
    : core(core)
    , module(module)
    , last_id(0)
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

    static opt<id_t> register_callback(Data& d, id_t id, proc_t proc, const char* name, const core::Task& on_call)
    {
        const auto addr = d.core.sym.symbol(d.module, name);
        if(!addr)
            return FAIL(ext::nullopt, "unable to find symbole {}!{}", d.module.data(), name);

        const auto bp = d.core.state.set_breakpoint(*addr, proc, on_call);
        if(!bp)
            return FAIL(ext::nullopt, "unable to set breakpoint");

        d.listeners.emplace(id, bp);
        return id;
    }

    template <typename T>
    static T arg(core::Core& core, size_t i)
    {
        const auto arg = core.os->read_arg(i);
        if(!arg)
            return {};

        return nt32::cast_to<T>(*arg);
    }
}

opt<id_t> nt32::syscalls32::register_NtAcceptConnectPort(proc_t proc, const on_NtAcceptConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAcceptConnectPort@24", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle        = arg<nt32::PHANDLE>(core, 0);
        const auto PortContext       = arg<nt32::PVOID>(core, 1);
        const auto ConnectionRequest = arg<nt32::PPORT_MESSAGE>(core, 2);
        const auto AcceptConnection  = arg<nt32::BOOLEAN>(core, 3);
        const auto ServerView        = arg<nt32::PPORT_VIEW>(core, 4);
        const auto ClientView        = arg<nt32::PREMOTE_PORT_VIEW>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[0]);

        on_func(PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAccessCheckAndAuditAlarm(proc_t proc, const on_ZwAccessCheckAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAccessCheckAndAuditAlarm@44", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName      = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto HandleId           = arg<nt32::PVOID>(core, 1);
        const auto ObjectTypeName     = arg<nt32::PUNICODE_STRING>(core, 2);
        const auto ObjectName         = arg<nt32::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor = arg<nt32::PSECURITY_DESCRIPTOR>(core, 4);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(core, 5);
        const auto GenericMapping     = arg<nt32::PGENERIC_MAPPING>(core, 6);
        const auto ObjectCreation     = arg<nt32::BOOLEAN>(core, 7);
        const auto GrantedAccess      = arg<nt32::PACCESS_MASK>(core, 8);
        const auto AccessStatus       = arg<nt32::PNTSTATUS>(core, 9);
        const auto GenerateOnClose    = arg<nt32::PBOOLEAN>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[1]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<id_t> nt32::syscalls32::register_NtAccessCheckByTypeAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAccessCheckByTypeAndAuditAlarm@64", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName        = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<nt32::PVOID>(core, 1);
        const auto ObjectTypeName       = arg<nt32::PUNICODE_STRING>(core, 2);
        const auto ObjectName           = arg<nt32::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor   = arg<nt32::PSECURITY_DESCRIPTOR>(core, 4);
        const auto PrincipalSelfSid     = arg<nt32::PSID>(core, 5);
        const auto DesiredAccess        = arg<nt32::ACCESS_MASK>(core, 6);
        const auto AuditType            = arg<nt32::AUDIT_EVENT_TYPE>(core, 7);
        const auto Flags                = arg<nt32::ULONG>(core, 8);
        const auto ObjectTypeList       = arg<nt32::POBJECT_TYPE_LIST>(core, 9);
        const auto ObjectTypeListLength = arg<nt32::ULONG>(core, 10);
        const auto GenericMapping       = arg<nt32::PGENERIC_MAPPING>(core, 11);
        const auto ObjectCreation       = arg<nt32::BOOLEAN>(core, 12);
        const auto GrantedAccess        = arg<nt32::PACCESS_MASK>(core, 13);
        const auto AccessStatus         = arg<nt32::PNTSTATUS>(core, 14);
        const auto GenerateOnClose      = arg<nt32::PBOOLEAN>(core, 15);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[2]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<id_t> nt32::syscalls32::register_NtAccessCheckByType(proc_t proc, const on_NtAccessCheckByType_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAccessCheckByType@44", [=]
    {
        auto& core = d_->core;
        
        const auto SecurityDescriptor   = arg<nt32::PSECURITY_DESCRIPTOR>(core, 0);
        const auto PrincipalSelfSid     = arg<nt32::PSID>(core, 1);
        const auto ClientToken          = arg<nt32::HANDLE>(core, 2);
        const auto DesiredAccess        = arg<nt32::ACCESS_MASK>(core, 3);
        const auto ObjectTypeList       = arg<nt32::POBJECT_TYPE_LIST>(core, 4);
        const auto ObjectTypeListLength = arg<nt32::ULONG>(core, 5);
        const auto GenericMapping       = arg<nt32::PGENERIC_MAPPING>(core, 6);
        const auto PrivilegeSet         = arg<nt32::PPRIVILEGE_SET>(core, 7);
        const auto PrivilegeSetLength   = arg<nt32::PULONG>(core, 8);
        const auto GrantedAccess        = arg<nt32::PACCESS_MASK>(core, 9);
        const auto AccessStatus         = arg<nt32::PNTSTATUS>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[3]);

        on_func(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(proc_t proc, const on_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle@68", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName        = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<nt32::PVOID>(core, 1);
        const auto ClientToken          = arg<nt32::HANDLE>(core, 2);
        const auto ObjectTypeName       = arg<nt32::PUNICODE_STRING>(core, 3);
        const auto ObjectName           = arg<nt32::PUNICODE_STRING>(core, 4);
        const auto SecurityDescriptor   = arg<nt32::PSECURITY_DESCRIPTOR>(core, 5);
        const auto PrincipalSelfSid     = arg<nt32::PSID>(core, 6);
        const auto DesiredAccess        = arg<nt32::ACCESS_MASK>(core, 7);
        const auto AuditType            = arg<nt32::AUDIT_EVENT_TYPE>(core, 8);
        const auto Flags                = arg<nt32::ULONG>(core, 9);
        const auto ObjectTypeList       = arg<nt32::POBJECT_TYPE_LIST>(core, 10);
        const auto ObjectTypeListLength = arg<nt32::ULONG>(core, 11);
        const auto GenericMapping       = arg<nt32::PGENERIC_MAPPING>(core, 12);
        const auto ObjectCreation       = arg<nt32::BOOLEAN>(core, 13);
        const auto GrantedAccess        = arg<nt32::PACCESS_MASK>(core, 14);
        const auto AccessStatus         = arg<nt32::PNTSTATUS>(core, 15);
        const auto GenerateOnClose      = arg<nt32::PBOOLEAN>(core, 16);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[4]);

        on_func(SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<id_t> nt32::syscalls32::register_NtAccessCheckByTypeResultListAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAccessCheckByTypeResultListAndAuditAlarm@64", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName        = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<nt32::PVOID>(core, 1);
        const auto ObjectTypeName       = arg<nt32::PUNICODE_STRING>(core, 2);
        const auto ObjectName           = arg<nt32::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor   = arg<nt32::PSECURITY_DESCRIPTOR>(core, 4);
        const auto PrincipalSelfSid     = arg<nt32::PSID>(core, 5);
        const auto DesiredAccess        = arg<nt32::ACCESS_MASK>(core, 6);
        const auto AuditType            = arg<nt32::AUDIT_EVENT_TYPE>(core, 7);
        const auto Flags                = arg<nt32::ULONG>(core, 8);
        const auto ObjectTypeList       = arg<nt32::POBJECT_TYPE_LIST>(core, 9);
        const auto ObjectTypeListLength = arg<nt32::ULONG>(core, 10);
        const auto GenericMapping       = arg<nt32::PGENERIC_MAPPING>(core, 11);
        const auto ObjectCreation       = arg<nt32::BOOLEAN>(core, 12);
        const auto GrantedAccess        = arg<nt32::PACCESS_MASK>(core, 13);
        const auto AccessStatus         = arg<nt32::PNTSTATUS>(core, 14);
        const auto GenerateOnClose      = arg<nt32::PBOOLEAN>(core, 15);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[5]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<id_t> nt32::syscalls32::register_NtAccessCheckByTypeResultList(proc_t proc, const on_NtAccessCheckByTypeResultList_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAccessCheckByTypeResultList@44", [=]
    {
        auto& core = d_->core;
        
        const auto SecurityDescriptor   = arg<nt32::PSECURITY_DESCRIPTOR>(core, 0);
        const auto PrincipalSelfSid     = arg<nt32::PSID>(core, 1);
        const auto ClientToken          = arg<nt32::HANDLE>(core, 2);
        const auto DesiredAccess        = arg<nt32::ACCESS_MASK>(core, 3);
        const auto ObjectTypeList       = arg<nt32::POBJECT_TYPE_LIST>(core, 4);
        const auto ObjectTypeListLength = arg<nt32::ULONG>(core, 5);
        const auto GenericMapping       = arg<nt32::PGENERIC_MAPPING>(core, 6);
        const auto PrivilegeSet         = arg<nt32::PPRIVILEGE_SET>(core, 7);
        const auto PrivilegeSetLength   = arg<nt32::PULONG>(core, 8);
        const auto GrantedAccess        = arg<nt32::PACCESS_MASK>(core, 9);
        const auto AccessStatus         = arg<nt32::PNTSTATUS>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[6]);

        on_func(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<id_t> nt32::syscalls32::register_NtAccessCheck(proc_t proc, const on_NtAccessCheck_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAccessCheck@32", [=]
    {
        auto& core = d_->core;
        
        const auto SecurityDescriptor = arg<nt32::PSECURITY_DESCRIPTOR>(core, 0);
        const auto ClientToken        = arg<nt32::HANDLE>(core, 1);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(core, 2);
        const auto GenericMapping     = arg<nt32::PGENERIC_MAPPING>(core, 3);
        const auto PrivilegeSet       = arg<nt32::PPRIVILEGE_SET>(core, 4);
        const auto PrivilegeSetLength = arg<nt32::PULONG>(core, 5);
        const auto GrantedAccess      = arg<nt32::PACCESS_MASK>(core, 6);
        const auto AccessStatus       = arg<nt32::PNTSTATUS>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[7]);

        on_func(SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<id_t> nt32::syscalls32::register_NtAddAtom(proc_t proc, const on_NtAddAtom_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAddAtom@12", [=]
    {
        auto& core = d_->core;
        
        const auto AtomName = arg<nt32::PWSTR>(core, 0);
        const auto Length   = arg<nt32::ULONG>(core, 1);
        const auto Atom     = arg<nt32::PRTL_ATOM>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[8]);

        on_func(AtomName, Length, Atom);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAddBootEntry(proc_t proc, const on_ZwAddBootEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAddBootEntry@8", [=]
    {
        auto& core = d_->core;
        
        const auto BootEntry = arg<nt32::PBOOT_ENTRY>(core, 0);
        const auto Id        = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[9]);

        on_func(BootEntry, Id);
    });
}

opt<id_t> nt32::syscalls32::register_NtAddDriverEntry(proc_t proc, const on_NtAddDriverEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAddDriverEntry@8", [=]
    {
        auto& core = d_->core;
        
        const auto DriverEntry = arg<nt32::PEFI_DRIVER_ENTRY>(core, 0);
        const auto Id          = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[10]);

        on_func(DriverEntry, Id);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAdjustGroupsToken(proc_t proc, const on_ZwAdjustGroupsToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAdjustGroupsToken@24", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle    = arg<nt32::HANDLE>(core, 0);
        const auto ResetToDefault = arg<nt32::BOOLEAN>(core, 1);
        const auto NewState       = arg<nt32::PTOKEN_GROUPS>(core, 2);
        const auto BufferLength   = arg<nt32::ULONG>(core, 3);
        const auto PreviousState  = arg<nt32::PTOKEN_GROUPS>(core, 4);
        const auto ReturnLength   = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[11]);

        on_func(TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAdjustPrivilegesToken(proc_t proc, const on_ZwAdjustPrivilegesToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAdjustPrivilegesToken@24", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle          = arg<nt32::HANDLE>(core, 0);
        const auto DisableAllPrivileges = arg<nt32::BOOLEAN>(core, 1);
        const auto NewState             = arg<nt32::PTOKEN_PRIVILEGES>(core, 2);
        const auto BufferLength         = arg<nt32::ULONG>(core, 3);
        const auto PreviousState        = arg<nt32::PTOKEN_PRIVILEGES>(core, 4);
        const auto ReturnLength         = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[12]);

        on_func(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlertResumeThread(proc_t proc, const on_NtAlertResumeThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlertResumeThread@8", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle         = arg<nt32::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[13]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlertThread(proc_t proc, const on_NtAlertThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlertThread@4", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[14]);

        on_func(ThreadHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAllocateLocallyUniqueId(proc_t proc, const on_ZwAllocateLocallyUniqueId_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAllocateLocallyUniqueId@4", [=]
    {
        auto& core = d_->core;
        
        const auto Luid = arg<nt32::PLUID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[15]);

        on_func(Luid);
    });
}

opt<id_t> nt32::syscalls32::register_NtAllocateReserveObject(proc_t proc, const on_NtAllocateReserveObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAllocateReserveObject@12", [=]
    {
        auto& core = d_->core;
        
        const auto MemoryReserveHandle = arg<nt32::PHANDLE>(core, 0);
        const auto ObjectAttributes    = arg<nt32::POBJECT_ATTRIBUTES>(core, 1);
        const auto Type                = arg<nt32::MEMORY_RESERVE_TYPE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[16]);

        on_func(MemoryReserveHandle, ObjectAttributes, Type);
    });
}

opt<id_t> nt32::syscalls32::register_NtAllocateUserPhysicalPages(proc_t proc, const on_NtAllocateUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAllocateUserPhysicalPages@12", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 0);
        const auto NumberOfPages = arg<nt32::PULONG_PTR>(core, 1);
        const auto UserPfnArra   = arg<nt32::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[17]);

        on_func(ProcessHandle, NumberOfPages, UserPfnArra);
    });
}

opt<id_t> nt32::syscalls32::register_NtAllocateUuids(proc_t proc, const on_NtAllocateUuids_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAllocateUuids@16", [=]
    {
        auto& core = d_->core;
        
        const auto Time     = arg<nt32::PULARGE_INTEGER>(core, 0);
        const auto Range    = arg<nt32::PULONG>(core, 1);
        const auto Sequence = arg<nt32::PULONG>(core, 2);
        const auto Seed     = arg<nt32::PCHAR>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[18]);

        on_func(Time, Range, Sequence, Seed);
    });
}

opt<id_t> nt32::syscalls32::register_NtAllocateVirtualMemory(proc_t proc, const on_NtAllocateVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAllocateVirtualMemory@24", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt32::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(core, 1);
        const auto ZeroBits        = arg<nt32::ULONG_PTR>(core, 2);
        const auto RegionSize      = arg<nt32::PSIZE_T>(core, 3);
        const auto AllocationType  = arg<nt32::ULONG>(core, 4);
        const auto Protect         = arg<nt32::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[19]);

        on_func(ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlpcAcceptConnectPort(proc_t proc, const on_NtAlpcAcceptConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlpcAcceptConnectPort@36", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle                  = arg<nt32::PHANDLE>(core, 0);
        const auto ConnectionPortHandle        = arg<nt32::HANDLE>(core, 1);
        const auto Flags                       = arg<nt32::ULONG>(core, 2);
        const auto ObjectAttributes            = arg<nt32::POBJECT_ATTRIBUTES>(core, 3);
        const auto PortAttributes              = arg<nt32::PALPC_PORT_ATTRIBUTES>(core, 4);
        const auto PortContext                 = arg<nt32::PVOID>(core, 5);
        const auto ConnectionRequest           = arg<nt32::PPORT_MESSAGE>(core, 6);
        const auto ConnectionMessageAttributes = arg<nt32::PALPC_MESSAGE_ATTRIBUTES>(core, 7);
        const auto AcceptConnection            = arg<nt32::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[20]);

        on_func(PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcCancelMessage(proc_t proc, const on_ZwAlpcCancelMessage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcCancelMessage@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle     = arg<nt32::HANDLE>(core, 0);
        const auto Flags          = arg<nt32::ULONG>(core, 1);
        const auto MessageContext = arg<nt32::PALPC_CONTEXT_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[21]);

        on_func(PortHandle, Flags, MessageContext);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcConnectPort(proc_t proc, const on_ZwAlpcConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcConnectPort@44", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle           = arg<nt32::PHANDLE>(core, 0);
        const auto PortName             = arg<nt32::PUNICODE_STRING>(core, 1);
        const auto ObjectAttributes     = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto PortAttributes       = arg<nt32::PALPC_PORT_ATTRIBUTES>(core, 3);
        const auto Flags                = arg<nt32::ULONG>(core, 4);
        const auto RequiredServerSid    = arg<nt32::PSID>(core, 5);
        const auto ConnectionMessage    = arg<nt32::PPORT_MESSAGE>(core, 6);
        const auto BufferLength         = arg<nt32::PULONG>(core, 7);
        const auto OutMessageAttributes = arg<nt32::PALPC_MESSAGE_ATTRIBUTES>(core, 8);
        const auto InMessageAttributes  = arg<nt32::PALPC_MESSAGE_ATTRIBUTES>(core, 9);
        const auto Timeout              = arg<nt32::PLARGE_INTEGER>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[22]);

        on_func(PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcCreatePort(proc_t proc, const on_ZwAlpcCreatePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcCreatePort@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle       = arg<nt32::PHANDLE>(core, 0);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 1);
        const auto PortAttributes   = arg<nt32::PALPC_PORT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[23]);

        on_func(PortHandle, ObjectAttributes, PortAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlpcCreatePortSection(proc_t proc, const on_NtAlpcCreatePortSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlpcCreatePortSection@24", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle        = arg<nt32::HANDLE>(core, 0);
        const auto Flags             = arg<nt32::ULONG>(core, 1);
        const auto SectionHandle     = arg<nt32::HANDLE>(core, 2);
        const auto SectionSize       = arg<nt32::SIZE_T>(core, 3);
        const auto AlpcSectionHandle = arg<nt32::PALPC_HANDLE>(core, 4);
        const auto ActualSectionSize = arg<nt32::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[24]);

        on_func(PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcCreateResourceReserve(proc_t proc, const on_ZwAlpcCreateResourceReserve_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcCreateResourceReserve@16", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle  = arg<nt32::HANDLE>(core, 0);
        const auto Flags       = arg<nt32::ULONG>(core, 1);
        const auto MessageSize = arg<nt32::SIZE_T>(core, 2);
        const auto ResourceId  = arg<nt32::PALPC_HANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[25]);

        on_func(PortHandle, Flags, MessageSize, ResourceId);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcCreateSectionView(proc_t proc, const on_ZwAlpcCreateSectionView_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcCreateSectionView@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle     = arg<nt32::HANDLE>(core, 0);
        const auto Flags          = arg<nt32::ULONG>(core, 1);
        const auto ViewAttributes = arg<nt32::PALPC_DATA_VIEW_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[26]);

        on_func(PortHandle, Flags, ViewAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcCreateSecurityContext(proc_t proc, const on_ZwAlpcCreateSecurityContext_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcCreateSecurityContext@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle        = arg<nt32::HANDLE>(core, 0);
        const auto Flags             = arg<nt32::ULONG>(core, 1);
        const auto SecurityAttribute = arg<nt32::PALPC_SECURITY_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[27]);

        on_func(PortHandle, Flags, SecurityAttribute);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcDeletePortSection(proc_t proc, const on_ZwAlpcDeletePortSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcDeletePortSection@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle    = arg<nt32::HANDLE>(core, 0);
        const auto Flags         = arg<nt32::ULONG>(core, 1);
        const auto SectionHandle = arg<nt32::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[28]);

        on_func(PortHandle, Flags, SectionHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlpcDeleteResourceReserve(proc_t proc, const on_NtAlpcDeleteResourceReserve_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlpcDeleteResourceReserve@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt32::HANDLE>(core, 0);
        const auto Flags      = arg<nt32::ULONG>(core, 1);
        const auto ResourceId = arg<nt32::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[29]);

        on_func(PortHandle, Flags, ResourceId);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlpcDeleteSectionView(proc_t proc, const on_NtAlpcDeleteSectionView_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlpcDeleteSectionView@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt32::HANDLE>(core, 0);
        const auto Flags      = arg<nt32::ULONG>(core, 1);
        const auto ViewBase   = arg<nt32::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[30]);

        on_func(PortHandle, Flags, ViewBase);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlpcDeleteSecurityContext(proc_t proc, const on_NtAlpcDeleteSecurityContext_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlpcDeleteSecurityContext@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle    = arg<nt32::HANDLE>(core, 0);
        const auto Flags         = arg<nt32::ULONG>(core, 1);
        const auto ContextHandle = arg<nt32::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[31]);

        on_func(PortHandle, Flags, ContextHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlpcDisconnectPort(proc_t proc, const on_NtAlpcDisconnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlpcDisconnectPort@8", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt32::HANDLE>(core, 0);
        const auto Flags      = arg<nt32::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[32]);

        on_func(PortHandle, Flags);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcImpersonateClientOfPort(proc_t proc, const on_ZwAlpcImpersonateClientOfPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcImpersonateClientOfPort@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle  = arg<nt32::HANDLE>(core, 0);
        const auto PortMessage = arg<nt32::PPORT_MESSAGE>(core, 1);
        const auto Reserved    = arg<nt32::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[33]);

        on_func(PortHandle, PortMessage, Reserved);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcOpenSenderProcess(proc_t proc, const on_ZwAlpcOpenSenderProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcOpenSenderProcess@24", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt32::PHANDLE>(core, 0);
        const auto PortHandle       = arg<nt32::HANDLE>(core, 1);
        const auto PortMessage      = arg<nt32::PPORT_MESSAGE>(core, 2);
        const auto Flags            = arg<nt32::ULONG>(core, 3);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 4);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[34]);

        on_func(ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcOpenSenderThread(proc_t proc, const on_ZwAlpcOpenSenderThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcOpenSenderThread@24", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle     = arg<nt32::PHANDLE>(core, 0);
        const auto PortHandle       = arg<nt32::HANDLE>(core, 1);
        const auto PortMessage      = arg<nt32::PPORT_MESSAGE>(core, 2);
        const auto Flags            = arg<nt32::ULONG>(core, 3);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 4);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[35]);

        on_func(ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcQueryInformation(proc_t proc, const on_ZwAlpcQueryInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcQueryInformation@20", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle           = arg<nt32::HANDLE>(core, 0);
        const auto PortInformationClass = arg<nt32::ALPC_PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<nt32::PVOID>(core, 2);
        const auto Length               = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength         = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[36]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAlpcQueryInformationMessage(proc_t proc, const on_ZwAlpcQueryInformationMessage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAlpcQueryInformationMessage@24", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle              = arg<nt32::HANDLE>(core, 0);
        const auto PortMessage             = arg<nt32::PPORT_MESSAGE>(core, 1);
        const auto MessageInformationClass = arg<nt32::ALPC_MESSAGE_INFORMATION_CLASS>(core, 2);
        const auto MessageInformation      = arg<nt32::PVOID>(core, 3);
        const auto Length                  = arg<nt32::ULONG>(core, 4);
        const auto ReturnLength            = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[37]);

        on_func(PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlpcRevokeSecurityContext(proc_t proc, const on_NtAlpcRevokeSecurityContext_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlpcRevokeSecurityContext@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle    = arg<nt32::HANDLE>(core, 0);
        const auto Flags         = arg<nt32::ULONG>(core, 1);
        const auto ContextHandle = arg<nt32::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[38]);

        on_func(PortHandle, Flags, ContextHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlpcSendWaitReceivePort(proc_t proc, const on_NtAlpcSendWaitReceivePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlpcSendWaitReceivePort@32", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle               = arg<nt32::HANDLE>(core, 0);
        const auto Flags                    = arg<nt32::ULONG>(core, 1);
        const auto SendMessage              = arg<nt32::PPORT_MESSAGE>(core, 2);
        const auto SendMessageAttributes    = arg<nt32::PALPC_MESSAGE_ATTRIBUTES>(core, 3);
        const auto ReceiveMessage           = arg<nt32::PPORT_MESSAGE>(core, 4);
        const auto BufferLength             = arg<nt32::PULONG>(core, 5);
        const auto ReceiveMessageAttributes = arg<nt32::PALPC_MESSAGE_ATTRIBUTES>(core, 6);
        const auto Timeout                  = arg<nt32::PLARGE_INTEGER>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[39]);

        on_func(PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout);
    });
}

opt<id_t> nt32::syscalls32::register_NtAlpcSetInformation(proc_t proc, const on_NtAlpcSetInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtAlpcSetInformation@16", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle           = arg<nt32::HANDLE>(core, 0);
        const auto PortInformationClass = arg<nt32::ALPC_PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<nt32::PVOID>(core, 2);
        const auto Length               = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[40]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length);
    });
}

opt<id_t> nt32::syscalls32::register_NtApphelpCacheControl(proc_t proc, const on_NtApphelpCacheControl_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtApphelpCacheControl@8", [=]
    {
        auto& core = d_->core;
        
        const auto type = arg<nt32::APPHELPCOMMAND>(core, 0);
        const auto buf  = arg<nt32::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[41]);

        on_func(type, buf);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAreMappedFilesTheSame(proc_t proc, const on_ZwAreMappedFilesTheSame_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAreMappedFilesTheSame@8", [=]
    {
        auto& core = d_->core;
        
        const auto File1MappedAsAnImage = arg<nt32::PVOID>(core, 0);
        const auto File2MappedAsFile    = arg<nt32::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[42]);

        on_func(File1MappedAsAnImage, File2MappedAsFile);
    });
}

opt<id_t> nt32::syscalls32::register_ZwAssignProcessToJobObject(proc_t proc, const on_ZwAssignProcessToJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwAssignProcessToJobObject@8", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle     = arg<nt32::HANDLE>(core, 0);
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[43]);

        on_func(JobHandle, ProcessHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtCancelIoFileEx(proc_t proc, const on_NtCancelIoFileEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCancelIoFileEx@12", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle        = arg<nt32::HANDLE>(core, 0);
        const auto IoRequestToCancel = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[44]);

        on_func(FileHandle, IoRequestToCancel, IoStatusBlock);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCancelIoFile(proc_t proc, const on_ZwCancelIoFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCancelIoFile@8", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[45]);

        on_func(FileHandle, IoStatusBlock);
    });
}

opt<id_t> nt32::syscalls32::register_NtCancelSynchronousIoFile(proc_t proc, const on_NtCancelSynchronousIoFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCancelSynchronousIoFile@12", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle      = arg<nt32::HANDLE>(core, 0);
        const auto IoRequestToCancel = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[46]);

        on_func(ThreadHandle, IoRequestToCancel, IoStatusBlock);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCancelTimer(proc_t proc, const on_ZwCancelTimer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCancelTimer@8", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle  = arg<nt32::HANDLE>(core, 0);
        const auto CurrentState = arg<nt32::PBOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[47]);

        on_func(TimerHandle, CurrentState);
    });
}

opt<id_t> nt32::syscalls32::register_NtClearEvent(proc_t proc, const on_NtClearEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtClearEvent@4", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[48]);

        on_func(EventHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtClose(proc_t proc, const on_NtClose_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtClose@4", [=]
    {
        auto& core = d_->core;
        
        const auto Handle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[49]);

        on_func(Handle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCloseObjectAuditAlarm(proc_t proc, const on_ZwCloseObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCloseObjectAuditAlarm@12", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName   = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto HandleId        = arg<nt32::PVOID>(core, 1);
        const auto GenerateOnClose = arg<nt32::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[50]);

        on_func(SubsystemName, HandleId, GenerateOnClose);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCommitComplete(proc_t proc, const on_ZwCommitComplete_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCommitComplete@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[51]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_NtCommitEnlistment(proc_t proc, const on_NtCommitEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCommitEnlistment@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[52]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_NtCommitTransaction(proc_t proc, const on_NtCommitTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCommitTransaction@8", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle = arg<nt32::HANDLE>(core, 0);
        const auto Wait              = arg<nt32::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[53]);

        on_func(TransactionHandle, Wait);
    });
}

opt<id_t> nt32::syscalls32::register_NtCompactKeys(proc_t proc, const on_NtCompactKeys_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCompactKeys@8", [=]
    {
        auto& core = d_->core;
        
        const auto Count    = arg<nt32::ULONG>(core, 0);
        const auto KeyArray = arg<nt32::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[54]);

        on_func(Count, KeyArray);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCompareTokens(proc_t proc, const on_ZwCompareTokens_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCompareTokens@12", [=]
    {
        auto& core = d_->core;
        
        const auto FirstTokenHandle  = arg<nt32::HANDLE>(core, 0);
        const auto SecondTokenHandle = arg<nt32::HANDLE>(core, 1);
        const auto Equal             = arg<nt32::PBOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[55]);

        on_func(FirstTokenHandle, SecondTokenHandle, Equal);
    });
}

opt<id_t> nt32::syscalls32::register_NtCompleteConnectPort(proc_t proc, const on_NtCompleteConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCompleteConnectPort@4", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[56]);

        on_func(PortHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCompressKey(proc_t proc, const on_ZwCompressKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCompressKey@4", [=]
    {
        auto& core = d_->core;
        
        const auto Key = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[57]);

        on_func(Key);
    });
}

opt<id_t> nt32::syscalls32::register_NtConnectPort(proc_t proc, const on_NtConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtConnectPort@32", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle                  = arg<nt32::PHANDLE>(core, 0);
        const auto PortName                    = arg<nt32::PUNICODE_STRING>(core, 1);
        const auto SecurityQos                 = arg<nt32::PSECURITY_QUALITY_OF_SERVICE>(core, 2);
        const auto ClientView                  = arg<nt32::PPORT_VIEW>(core, 3);
        const auto ServerView                  = arg<nt32::PREMOTE_PORT_VIEW>(core, 4);
        const auto MaxMessageLength            = arg<nt32::PULONG>(core, 5);
        const auto ConnectionInformation       = arg<nt32::PVOID>(core, 6);
        const auto ConnectionInformationLength = arg<nt32::PULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[58]);

        on_func(PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwContinue(proc_t proc, const on_ZwContinue_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwContinue@8", [=]
    {
        auto& core = d_->core;
        
        const auto ContextRecord = arg<nt32::PCONTEXT>(core, 0);
        const auto TestAlert     = arg<nt32::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[59]);

        on_func(ContextRecord, TestAlert);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateDebugObject(proc_t proc, const on_ZwCreateDebugObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateDebugObject@16", [=]
    {
        auto& core = d_->core;
        
        const auto DebugObjectHandle = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto Flags             = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[60]);

        on_func(DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateDirectoryObject(proc_t proc, const on_ZwCreateDirectoryObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateDirectoryObject@12", [=]
    {
        auto& core = d_->core;
        
        const auto DirectoryHandle  = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[61]);

        on_func(DirectoryHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateEnlistment(proc_t proc, const on_ZwCreateEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateEnlistment@32", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle      = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ResourceManagerHandle = arg<nt32::HANDLE>(core, 2);
        const auto TransactionHandle     = arg<nt32::HANDLE>(core, 3);
        const auto ObjectAttributes      = arg<nt32::POBJECT_ATTRIBUTES>(core, 4);
        const auto CreateOptions         = arg<nt32::ULONG>(core, 5);
        const auto NotificationMask      = arg<nt32::NOTIFICATION_MASK>(core, 6);
        const auto EnlistmentKey         = arg<nt32::PVOID>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[62]);

        on_func(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateEvent(proc_t proc, const on_NtCreateEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateEvent@20", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle      = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto EventType        = arg<nt32::EVENT_TYPE>(core, 3);
        const auto InitialState     = arg<nt32::BOOLEAN>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[63]);

        on_func(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateEventPair(proc_t proc, const on_NtCreateEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateEventPair@12", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle  = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[64]);

        on_func(EventPairHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateFile(proc_t proc, const on_NtCreateFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateFile@44", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle        = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(core, 3);
        const auto AllocationSize    = arg<nt32::PLARGE_INTEGER>(core, 4);
        const auto FileAttributes    = arg<nt32::ULONG>(core, 5);
        const auto ShareAccess       = arg<nt32::ULONG>(core, 6);
        const auto CreateDisposition = arg<nt32::ULONG>(core, 7);
        const auto CreateOptions     = arg<nt32::ULONG>(core, 8);
        const auto EaBuffer          = arg<nt32::PVOID>(core, 9);
        const auto EaLength          = arg<nt32::ULONG>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[65]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateIoCompletion(proc_t proc, const on_NtCreateIoCompletion_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateIoCompletion@16", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto Count              = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[66]);

        on_func(IoCompletionHandle, DesiredAccess, ObjectAttributes, Count);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateJobObject(proc_t proc, const on_ZwCreateJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateJobObject@12", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle        = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[67]);

        on_func(JobHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateJobSet(proc_t proc, const on_NtCreateJobSet_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateJobSet@12", [=]
    {
        auto& core = d_->core;
        
        const auto NumJob     = arg<nt32::ULONG>(core, 0);
        const auto UserJobSet = arg<nt32::PJOB_SET_ARRAY>(core, 1);
        const auto Flags      = arg<nt32::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[68]);

        on_func(NumJob, UserJobSet, Flags);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateKeyedEvent(proc_t proc, const on_ZwCreateKeyedEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateKeyedEvent@16", [=]
    {
        auto& core = d_->core;
        
        const auto KeyedEventHandle = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto Flags            = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[69]);

        on_func(KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateKey(proc_t proc, const on_ZwCreateKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateKey@28", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle        = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto TitleIndex       = arg<nt32::ULONG>(core, 3);
        const auto Class            = arg<nt32::PUNICODE_STRING>(core, 4);
        const auto CreateOptions    = arg<nt32::ULONG>(core, 5);
        const auto Disposition      = arg<nt32::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[70]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateKeyTransacted(proc_t proc, const on_NtCreateKeyTransacted_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateKeyTransacted@32", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle         = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto TitleIndex        = arg<nt32::ULONG>(core, 3);
        const auto Class             = arg<nt32::PUNICODE_STRING>(core, 4);
        const auto CreateOptions     = arg<nt32::ULONG>(core, 5);
        const auto TransactionHandle = arg<nt32::HANDLE>(core, 6);
        const auto Disposition       = arg<nt32::PULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[71]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateMailslotFile(proc_t proc, const on_ZwCreateMailslotFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateMailslotFile@32", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle         = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt32::ULONG>(core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(core, 3);
        const auto CreateOptions      = arg<nt32::ULONG>(core, 4);
        const auto MailslotQuota      = arg<nt32::ULONG>(core, 5);
        const auto MaximumMessageSize = arg<nt32::ULONG>(core, 6);
        const auto ReadTimeout        = arg<nt32::PLARGE_INTEGER>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[72]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateMutant(proc_t proc, const on_ZwCreateMutant_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateMutant@16", [=]
    {
        auto& core = d_->core;
        
        const auto MutantHandle     = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto InitialOwner     = arg<nt32::BOOLEAN>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[73]);

        on_func(MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateNamedPipeFile(proc_t proc, const on_ZwCreateNamedPipeFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateNamedPipeFile@56", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle        = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt32::ULONG>(core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(core, 3);
        const auto ShareAccess       = arg<nt32::ULONG>(core, 4);
        const auto CreateDisposition = arg<nt32::ULONG>(core, 5);
        const auto CreateOptions     = arg<nt32::ULONG>(core, 6);
        const auto NamedPipeType     = arg<nt32::ULONG>(core, 7);
        const auto ReadMode          = arg<nt32::ULONG>(core, 8);
        const auto CompletionMode    = arg<nt32::ULONG>(core, 9);
        const auto MaximumInstances  = arg<nt32::ULONG>(core, 10);
        const auto InboundQuota      = arg<nt32::ULONG>(core, 11);
        const auto OutboundQuota     = arg<nt32::ULONG>(core, 12);
        const auto DefaultTimeout    = arg<nt32::PLARGE_INTEGER>(core, 13);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[74]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreatePagingFile(proc_t proc, const on_NtCreatePagingFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreatePagingFile@16", [=]
    {
        auto& core = d_->core;
        
        const auto PageFileName = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto MinimumSize  = arg<nt32::PLARGE_INTEGER>(core, 1);
        const auto MaximumSize  = arg<nt32::PLARGE_INTEGER>(core, 2);
        const auto Priority     = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[75]);

        on_func(PageFileName, MinimumSize, MaximumSize, Priority);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreatePort(proc_t proc, const on_ZwCreatePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreatePort@20", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle              = arg<nt32::PHANDLE>(core, 0);
        const auto ObjectAttributes        = arg<nt32::POBJECT_ATTRIBUTES>(core, 1);
        const auto MaxConnectionInfoLength = arg<nt32::ULONG>(core, 2);
        const auto MaxMessageLength        = arg<nt32::ULONG>(core, 3);
        const auto MaxPoolUsage            = arg<nt32::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[76]);

        on_func(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreatePrivateNamespace(proc_t proc, const on_NtCreatePrivateNamespace_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreatePrivateNamespace@16", [=]
    {
        auto& core = d_->core;
        
        const auto NamespaceHandle    = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto BoundaryDescriptor = arg<nt32::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[77]);

        on_func(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateProcessEx(proc_t proc, const on_ZwCreateProcessEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateProcessEx@36", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto ParentProcess    = arg<nt32::HANDLE>(core, 3);
        const auto Flags            = arg<nt32::ULONG>(core, 4);
        const auto SectionHandle    = arg<nt32::HANDLE>(core, 5);
        const auto DebugPort        = arg<nt32::HANDLE>(core, 6);
        const auto ExceptionPort    = arg<nt32::HANDLE>(core, 7);
        const auto JobMemberLevel   = arg<nt32::ULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[78]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateProcess(proc_t proc, const on_ZwCreateProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateProcess@32", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle      = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto ParentProcess      = arg<nt32::HANDLE>(core, 3);
        const auto InheritObjectTable = arg<nt32::BOOLEAN>(core, 4);
        const auto SectionHandle      = arg<nt32::HANDLE>(core, 5);
        const auto DebugPort          = arg<nt32::HANDLE>(core, 6);
        const auto ExceptionPort      = arg<nt32::HANDLE>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[79]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateProfileEx(proc_t proc, const on_NtCreateProfileEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateProfileEx@40", [=]
    {
        auto& core = d_->core;
        
        const auto ProfileHandle      = arg<nt32::PHANDLE>(core, 0);
        const auto Process            = arg<nt32::HANDLE>(core, 1);
        const auto ProfileBase        = arg<nt32::PVOID>(core, 2);
        const auto ProfileSize        = arg<nt32::SIZE_T>(core, 3);
        const auto BucketSize         = arg<nt32::ULONG>(core, 4);
        const auto Buffer             = arg<nt32::PULONG>(core, 5);
        const auto BufferSize         = arg<nt32::ULONG>(core, 6);
        const auto ProfileSource      = arg<nt32::KPROFILE_SOURCE>(core, 7);
        const auto GroupAffinityCount = arg<nt32::ULONG>(core, 8);
        const auto GroupAffinity      = arg<nt32::PGROUP_AFFINITY>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[80]);

        on_func(ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateProfile(proc_t proc, const on_ZwCreateProfile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateProfile@36", [=]
    {
        auto& core = d_->core;
        
        const auto ProfileHandle = arg<nt32::PHANDLE>(core, 0);
        const auto Process       = arg<nt32::HANDLE>(core, 1);
        const auto RangeBase     = arg<nt32::PVOID>(core, 2);
        const auto RangeSize     = arg<nt32::SIZE_T>(core, 3);
        const auto BucketSize    = arg<nt32::ULONG>(core, 4);
        const auto Buffer        = arg<nt32::PULONG>(core, 5);
        const auto BufferSize    = arg<nt32::ULONG>(core, 6);
        const auto ProfileSource = arg<nt32::KPROFILE_SOURCE>(core, 7);
        const auto Affinity      = arg<nt32::KAFFINITY>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[81]);

        on_func(ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateResourceManager(proc_t proc, const on_ZwCreateResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateResourceManager@28", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt32::ACCESS_MASK>(core, 1);
        const auto TmHandle              = arg<nt32::HANDLE>(core, 2);
        const auto RmGuid                = arg<nt32::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<nt32::POBJECT_ATTRIBUTES>(core, 4);
        const auto CreateOptions         = arg<nt32::ULONG>(core, 5);
        const auto Description           = arg<nt32::PUNICODE_STRING>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[82]);

        on_func(ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateSection(proc_t proc, const on_NtCreateSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateSection@28", [=]
    {
        auto& core = d_->core;
        
        const auto SectionHandle         = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes      = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto MaximumSize           = arg<nt32::PLARGE_INTEGER>(core, 3);
        const auto SectionPageProtection = arg<nt32::ULONG>(core, 4);
        const auto AllocationAttributes  = arg<nt32::ULONG>(core, 5);
        const auto FileHandle            = arg<nt32::HANDLE>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[83]);

        on_func(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateSemaphore(proc_t proc, const on_NtCreateSemaphore_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateSemaphore@20", [=]
    {
        auto& core = d_->core;
        
        const auto SemaphoreHandle  = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto InitialCount     = arg<nt32::LONG>(core, 3);
        const auto MaximumCount     = arg<nt32::LONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[84]);

        on_func(SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateSymbolicLinkObject(proc_t proc, const on_ZwCreateSymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateSymbolicLinkObject@16", [=]
    {
        auto& core = d_->core;
        
        const auto LinkHandle       = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto LinkTarget       = arg<nt32::PUNICODE_STRING>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[85]);

        on_func(LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateThreadEx(proc_t proc, const on_NtCreateThreadEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateThreadEx@44", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle     = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto ProcessHandle    = arg<nt32::HANDLE>(core, 3);
        const auto StartRoutine     = arg<nt32::PVOID>(core, 4);
        const auto Argument         = arg<nt32::PVOID>(core, 5);
        const auto CreateFlags      = arg<nt32::ULONG>(core, 6);
        const auto ZeroBits         = arg<nt32::ULONG_PTR>(core, 7);
        const auto StackSize        = arg<nt32::SIZE_T>(core, 8);
        const auto MaximumStackSize = arg<nt32::SIZE_T>(core, 9);
        const auto AttributeList    = arg<nt32::PPS_ATTRIBUTE_LIST>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[86]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateThread(proc_t proc, const on_NtCreateThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateThread@32", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle     = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto ProcessHandle    = arg<nt32::HANDLE>(core, 3);
        const auto ClientId         = arg<nt32::PCLIENT_ID>(core, 4);
        const auto ThreadContext    = arg<nt32::PCONTEXT>(core, 5);
        const auto InitialTeb       = arg<nt32::PINITIAL_TEB>(core, 6);
        const auto CreateSuspended  = arg<nt32::BOOLEAN>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[87]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateTimer(proc_t proc, const on_ZwCreateTimer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateTimer@16", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle      = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto TimerType        = arg<nt32::TIMER_TYPE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[88]);

        on_func(TimerHandle, DesiredAccess, ObjectAttributes, TimerType);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateToken(proc_t proc, const on_NtCreateToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateToken@52", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle      = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto TokenType        = arg<nt32::TOKEN_TYPE>(core, 3);
        const auto AuthenticationId = arg<nt32::PLUID>(core, 4);
        const auto ExpirationTime   = arg<nt32::PLARGE_INTEGER>(core, 5);
        const auto User             = arg<nt32::PTOKEN_USER>(core, 6);
        const auto Groups           = arg<nt32::PTOKEN_GROUPS>(core, 7);
        const auto Privileges       = arg<nt32::PTOKEN_PRIVILEGES>(core, 8);
        const auto Owner            = arg<nt32::PTOKEN_OWNER>(core, 9);
        const auto PrimaryGroup     = arg<nt32::PTOKEN_PRIMARY_GROUP>(core, 10);
        const auto DefaultDacl      = arg<nt32::PTOKEN_DEFAULT_DACL>(core, 11);
        const auto TokenSource      = arg<nt32::PTOKEN_SOURCE>(core, 12);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[89]);

        on_func(TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateTransactionManager(proc_t proc, const on_ZwCreateTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateTransactionManager@24", [=]
    {
        auto& core = d_->core;
        
        const auto TmHandle         = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto LogFileName      = arg<nt32::PUNICODE_STRING>(core, 3);
        const auto CreateOptions    = arg<nt32::ULONG>(core, 4);
        const auto CommitStrength   = arg<nt32::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[90]);

        on_func(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateTransaction(proc_t proc, const on_NtCreateTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateTransaction@40", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto Uow               = arg<nt32::LPGUID>(core, 3);
        const auto TmHandle          = arg<nt32::HANDLE>(core, 4);
        const auto CreateOptions     = arg<nt32::ULONG>(core, 5);
        const auto IsolationLevel    = arg<nt32::ULONG>(core, 6);
        const auto IsolationFlags    = arg<nt32::ULONG>(core, 7);
        const auto Timeout           = arg<nt32::PLARGE_INTEGER>(core, 8);
        const auto Description       = arg<nt32::PUNICODE_STRING>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[91]);

        on_func(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateUserProcess(proc_t proc, const on_NtCreateUserProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateUserProcess@44", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle           = arg<nt32::PHANDLE>(core, 0);
        const auto ThreadHandle            = arg<nt32::PHANDLE>(core, 1);
        const auto ProcessDesiredAccess    = arg<nt32::ACCESS_MASK>(core, 2);
        const auto ThreadDesiredAccess     = arg<nt32::ACCESS_MASK>(core, 3);
        const auto ProcessObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 4);
        const auto ThreadObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(core, 5);
        const auto ProcessFlags            = arg<nt32::ULONG>(core, 6);
        const auto ThreadFlags             = arg<nt32::ULONG>(core, 7);
        const auto ProcessParameters       = arg<nt32::PRTL_USER_PROCESS_PARAMETERS>(core, 8);
        const auto CreateInfo              = arg<nt32::PPROCESS_CREATE_INFO>(core, 9);
        const auto AttributeList           = arg<nt32::PPROCESS_ATTRIBUTE_LIST>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[92]);

        on_func(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
    });
}

opt<id_t> nt32::syscalls32::register_ZwCreateWaitablePort(proc_t proc, const on_ZwCreateWaitablePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwCreateWaitablePort@20", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle              = arg<nt32::PHANDLE>(core, 0);
        const auto ObjectAttributes        = arg<nt32::POBJECT_ATTRIBUTES>(core, 1);
        const auto MaxConnectionInfoLength = arg<nt32::ULONG>(core, 2);
        const auto MaxMessageLength        = arg<nt32::ULONG>(core, 3);
        const auto MaxPoolUsage            = arg<nt32::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[93]);

        on_func(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    });
}

opt<id_t> nt32::syscalls32::register_NtCreateWorkerFactory(proc_t proc, const on_NtCreateWorkerFactory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtCreateWorkerFactory@40", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandleReturn = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess             = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes          = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto CompletionPortHandle      = arg<nt32::HANDLE>(core, 3);
        const auto WorkerProcessHandle       = arg<nt32::HANDLE>(core, 4);
        const auto StartRoutine              = arg<nt32::PVOID>(core, 5);
        const auto StartParameter            = arg<nt32::PVOID>(core, 6);
        const auto MaxThreadCount            = arg<nt32::ULONG>(core, 7);
        const auto StackReserve              = arg<nt32::SIZE_T>(core, 8);
        const auto StackCommit               = arg<nt32::SIZE_T>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[94]);

        on_func(WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit);
    });
}

opt<id_t> nt32::syscalls32::register_NtDebugActiveProcess(proc_t proc, const on_NtDebugActiveProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtDebugActiveProcess@8", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle     = arg<nt32::HANDLE>(core, 0);
        const auto DebugObjectHandle = arg<nt32::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[95]);

        on_func(ProcessHandle, DebugObjectHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwDebugContinue(proc_t proc, const on_ZwDebugContinue_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwDebugContinue@12", [=]
    {
        auto& core = d_->core;
        
        const auto DebugObjectHandle = arg<nt32::HANDLE>(core, 0);
        const auto ClientId          = arg<nt32::PCLIENT_ID>(core, 1);
        const auto ContinueStatus    = arg<nt32::NTSTATUS>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[96]);

        on_func(DebugObjectHandle, ClientId, ContinueStatus);
    });
}

opt<id_t> nt32::syscalls32::register_ZwDelayExecution(proc_t proc, const on_ZwDelayExecution_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwDelayExecution@8", [=]
    {
        auto& core = d_->core;
        
        const auto Alertable     = arg<nt32::BOOLEAN>(core, 0);
        const auto DelayInterval = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[97]);

        on_func(Alertable, DelayInterval);
    });
}

opt<id_t> nt32::syscalls32::register_ZwDeleteAtom(proc_t proc, const on_ZwDeleteAtom_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwDeleteAtom@4", [=]
    {
        auto& core = d_->core;
        
        const auto Atom = arg<nt32::RTL_ATOM>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[98]);

        on_func(Atom);
    });
}

opt<id_t> nt32::syscalls32::register_NtDeleteBootEntry(proc_t proc, const on_NtDeleteBootEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtDeleteBootEntry@4", [=]
    {
        auto& core = d_->core;
        
        const auto Id = arg<nt32::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[99]);

        on_func(Id);
    });
}

opt<id_t> nt32::syscalls32::register_ZwDeleteDriverEntry(proc_t proc, const on_ZwDeleteDriverEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwDeleteDriverEntry@4", [=]
    {
        auto& core = d_->core;
        
        const auto Id = arg<nt32::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[100]);

        on_func(Id);
    });
}

opt<id_t> nt32::syscalls32::register_NtDeleteFile(proc_t proc, const on_NtDeleteFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtDeleteFile@4", [=]
    {
        auto& core = d_->core;
        
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[101]);

        on_func(ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwDeleteKey(proc_t proc, const on_ZwDeleteKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwDeleteKey@4", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[102]);

        on_func(KeyHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtDeleteObjectAuditAlarm(proc_t proc, const on_NtDeleteObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtDeleteObjectAuditAlarm@12", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName   = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto HandleId        = arg<nt32::PVOID>(core, 1);
        const auto GenerateOnClose = arg<nt32::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[103]);

        on_func(SubsystemName, HandleId, GenerateOnClose);
    });
}

opt<id_t> nt32::syscalls32::register_NtDeletePrivateNamespace(proc_t proc, const on_NtDeletePrivateNamespace_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtDeletePrivateNamespace@4", [=]
    {
        auto& core = d_->core;
        
        const auto NamespaceHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[104]);

        on_func(NamespaceHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtDeleteValueKey(proc_t proc, const on_NtDeleteValueKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtDeleteValueKey@8", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle = arg<nt32::HANDLE>(core, 0);
        const auto ValueName = arg<nt32::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[105]);

        on_func(KeyHandle, ValueName);
    });
}

opt<id_t> nt32::syscalls32::register_ZwDeviceIoControlFile(proc_t proc, const on_ZwDeviceIoControlFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwDeviceIoControlFile@40", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle         = arg<nt32::HANDLE>(core, 0);
        const auto Event              = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine         = arg<nt32::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext         = arg<nt32::PVOID>(core, 3);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(core, 4);
        const auto IoControlCode      = arg<nt32::ULONG>(core, 5);
        const auto InputBuffer        = arg<nt32::PVOID>(core, 6);
        const auto InputBufferLength  = arg<nt32::ULONG>(core, 7);
        const auto OutputBuffer       = arg<nt32::PVOID>(core, 8);
        const auto OutputBufferLength = arg<nt32::ULONG>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[106]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtDisplayString(proc_t proc, const on_NtDisplayString_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtDisplayString@4", [=]
    {
        auto& core = d_->core;
        
        const auto String = arg<nt32::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[107]);

        on_func(String);
    });
}

opt<id_t> nt32::syscalls32::register_ZwDrawText(proc_t proc, const on_ZwDrawText_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwDrawText@4", [=]
    {
        auto& core = d_->core;
        
        const auto Text = arg<nt32::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[108]);

        on_func(Text);
    });
}

opt<id_t> nt32::syscalls32::register_ZwDuplicateObject(proc_t proc, const on_ZwDuplicateObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwDuplicateObject@28", [=]
    {
        auto& core = d_->core;
        
        const auto SourceProcessHandle = arg<nt32::HANDLE>(core, 0);
        const auto SourceHandle        = arg<nt32::HANDLE>(core, 1);
        const auto TargetProcessHandle = arg<nt32::HANDLE>(core, 2);
        const auto TargetHandle        = arg<nt32::PHANDLE>(core, 3);
        const auto DesiredAccess       = arg<nt32::ACCESS_MASK>(core, 4);
        const auto HandleAttributes    = arg<nt32::ULONG>(core, 5);
        const auto Options             = arg<nt32::ULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[109]);

        on_func(SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);
    });
}

opt<id_t> nt32::syscalls32::register_NtDuplicateToken(proc_t proc, const on_NtDuplicateToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtDuplicateToken@24", [=]
    {
        auto& core = d_->core;
        
        const auto ExistingTokenHandle = arg<nt32::HANDLE>(core, 0);
        const auto DesiredAccess       = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes    = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto EffectiveOnly       = arg<nt32::BOOLEAN>(core, 3);
        const auto TokenType           = arg<nt32::TOKEN_TYPE>(core, 4);
        const auto NewTokenHandle      = arg<nt32::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[110]);

        on_func(ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwEnumerateBootEntries(proc_t proc, const on_ZwEnumerateBootEntries_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwEnumerateBootEntries@8", [=]
    {
        auto& core = d_->core;
        
        const auto Buffer       = arg<nt32::PVOID>(core, 0);
        const auto BufferLength = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[111]);

        on_func(Buffer, BufferLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtEnumerateDriverEntries(proc_t proc, const on_NtEnumerateDriverEntries_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtEnumerateDriverEntries@8", [=]
    {
        auto& core = d_->core;
        
        const auto Buffer       = arg<nt32::PVOID>(core, 0);
        const auto BufferLength = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[112]);

        on_func(Buffer, BufferLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwEnumerateKey(proc_t proc, const on_ZwEnumerateKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwEnumerateKey@24", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle           = arg<nt32::HANDLE>(core, 0);
        const auto Index               = arg<nt32::ULONG>(core, 1);
        const auto KeyInformationClass = arg<nt32::KEY_INFORMATION_CLASS>(core, 2);
        const auto KeyInformation      = arg<nt32::PVOID>(core, 3);
        const auto Length              = arg<nt32::ULONG>(core, 4);
        const auto ResultLength        = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[113]);

        on_func(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwEnumerateSystemEnvironmentValuesEx(proc_t proc, const on_ZwEnumerateSystemEnvironmentValuesEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwEnumerateSystemEnvironmentValuesEx@12", [=]
    {
        auto& core = d_->core;
        
        const auto InformationClass = arg<nt32::ULONG>(core, 0);
        const auto Buffer           = arg<nt32::PVOID>(core, 1);
        const auto BufferLength     = arg<nt32::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[114]);

        on_func(InformationClass, Buffer, BufferLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwEnumerateTransactionObject(proc_t proc, const on_ZwEnumerateTransactionObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwEnumerateTransactionObject@20", [=]
    {
        auto& core = d_->core;
        
        const auto RootObjectHandle   = arg<nt32::HANDLE>(core, 0);
        const auto QueryType          = arg<nt32::KTMOBJECT_TYPE>(core, 1);
        const auto ObjectCursor       = arg<nt32::PKTMOBJECT_CURSOR>(core, 2);
        const auto ObjectCursorLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength       = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[115]);

        on_func(RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtEnumerateValueKey(proc_t proc, const on_NtEnumerateValueKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtEnumerateValueKey@24", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle                = arg<nt32::HANDLE>(core, 0);
        const auto Index                    = arg<nt32::ULONG>(core, 1);
        const auto KeyValueInformationClass = arg<nt32::KEY_VALUE_INFORMATION_CLASS>(core, 2);
        const auto KeyValueInformation      = arg<nt32::PVOID>(core, 3);
        const auto Length                   = arg<nt32::ULONG>(core, 4);
        const auto ResultLength             = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[116]);

        on_func(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwExtendSection(proc_t proc, const on_ZwExtendSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwExtendSection@8", [=]
    {
        auto& core = d_->core;
        
        const auto SectionHandle  = arg<nt32::HANDLE>(core, 0);
        const auto NewSectionSize = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[117]);

        on_func(SectionHandle, NewSectionSize);
    });
}

opt<id_t> nt32::syscalls32::register_NtFilterToken(proc_t proc, const on_NtFilterToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtFilterToken@24", [=]
    {
        auto& core = d_->core;
        
        const auto ExistingTokenHandle = arg<nt32::HANDLE>(core, 0);
        const auto Flags               = arg<nt32::ULONG>(core, 1);
        const auto SidsToDisable       = arg<nt32::PTOKEN_GROUPS>(core, 2);
        const auto PrivilegesToDelete  = arg<nt32::PTOKEN_PRIVILEGES>(core, 3);
        const auto RestrictedSids      = arg<nt32::PTOKEN_GROUPS>(core, 4);
        const auto NewTokenHandle      = arg<nt32::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[118]);

        on_func(ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtFindAtom(proc_t proc, const on_NtFindAtom_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtFindAtom@12", [=]
    {
        auto& core = d_->core;
        
        const auto AtomName = arg<nt32::PWSTR>(core, 0);
        const auto Length   = arg<nt32::ULONG>(core, 1);
        const auto Atom     = arg<nt32::PRTL_ATOM>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[119]);

        on_func(AtomName, Length, Atom);
    });
}

opt<id_t> nt32::syscalls32::register_ZwFlushBuffersFile(proc_t proc, const on_ZwFlushBuffersFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwFlushBuffersFile@8", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[120]);

        on_func(FileHandle, IoStatusBlock);
    });
}

opt<id_t> nt32::syscalls32::register_ZwFlushInstallUILanguage(proc_t proc, const on_ZwFlushInstallUILanguage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwFlushInstallUILanguage@8", [=]
    {
        auto& core = d_->core;
        
        const auto InstallUILanguage = arg<nt32::LANGID>(core, 0);
        const auto SetComittedFlag   = arg<nt32::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[121]);

        on_func(InstallUILanguage, SetComittedFlag);
    });
}

opt<id_t> nt32::syscalls32::register_ZwFlushInstructionCache(proc_t proc, const on_ZwFlushInstructionCache_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwFlushInstructionCache@12", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 0);
        const auto BaseAddress   = arg<nt32::PVOID>(core, 1);
        const auto Length        = arg<nt32::SIZE_T>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[122]);

        on_func(ProcessHandle, BaseAddress, Length);
    });
}

opt<id_t> nt32::syscalls32::register_NtFlushKey(proc_t proc, const on_NtFlushKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtFlushKey@4", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[123]);

        on_func(KeyHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwFlushVirtualMemory(proc_t proc, const on_ZwFlushVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwFlushVirtualMemory@16", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt32::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(core, 1);
        const auto RegionSize      = arg<nt32::PSIZE_T>(core, 2);
        const auto IoStatus        = arg<nt32::PIO_STATUS_BLOCK>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[124]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, IoStatus);
    });
}

opt<id_t> nt32::syscalls32::register_NtFreeUserPhysicalPages(proc_t proc, const on_NtFreeUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtFreeUserPhysicalPages@12", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 0);
        const auto NumberOfPages = arg<nt32::PULONG_PTR>(core, 1);
        const auto UserPfnArra   = arg<nt32::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[125]);

        on_func(ProcessHandle, NumberOfPages, UserPfnArra);
    });
}

opt<id_t> nt32::syscalls32::register_NtFreeVirtualMemory(proc_t proc, const on_NtFreeVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtFreeVirtualMemory@16", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt32::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(core, 1);
        const auto RegionSize      = arg<nt32::PSIZE_T>(core, 2);
        const auto FreeType        = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[126]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, FreeType);
    });
}

opt<id_t> nt32::syscalls32::register_NtFreezeRegistry(proc_t proc, const on_NtFreezeRegistry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtFreezeRegistry@4", [=]
    {
        auto& core = d_->core;
        
        const auto TimeOutInSeconds = arg<nt32::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[127]);

        on_func(TimeOutInSeconds);
    });
}

opt<id_t> nt32::syscalls32::register_ZwFreezeTransactions(proc_t proc, const on_ZwFreezeTransactions_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwFreezeTransactions@8", [=]
    {
        auto& core = d_->core;
        
        const auto FreezeTimeout = arg<nt32::PLARGE_INTEGER>(core, 0);
        const auto ThawTimeout   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[128]);

        on_func(FreezeTimeout, ThawTimeout);
    });
}

opt<id_t> nt32::syscalls32::register_NtFsControlFile(proc_t proc, const on_NtFsControlFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtFsControlFile@40", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle         = arg<nt32::HANDLE>(core, 0);
        const auto Event              = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine         = arg<nt32::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext         = arg<nt32::PVOID>(core, 3);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(core, 4);
        const auto IoControlCode      = arg<nt32::ULONG>(core, 5);
        const auto InputBuffer        = arg<nt32::PVOID>(core, 6);
        const auto InputBufferLength  = arg<nt32::ULONG>(core, 7);
        const auto OutputBuffer       = arg<nt32::PVOID>(core, 8);
        const auto OutputBufferLength = arg<nt32::ULONG>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[129]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtGetContextThread(proc_t proc, const on_NtGetContextThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtGetContextThread@8", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle  = arg<nt32::HANDLE>(core, 0);
        const auto ThreadContext = arg<nt32::PCONTEXT>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[130]);

        on_func(ThreadHandle, ThreadContext);
    });
}

opt<id_t> nt32::syscalls32::register_NtGetDevicePowerState(proc_t proc, const on_NtGetDevicePowerState_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtGetDevicePowerState@8", [=]
    {
        auto& core = d_->core;
        
        const auto Device    = arg<nt32::HANDLE>(core, 0);
        const auto STARState = arg<nt32::DEVICE_POWER_STATE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[131]);

        on_func(Device, STARState);
    });
}

opt<id_t> nt32::syscalls32::register_NtGetMUIRegistryInfo(proc_t proc, const on_NtGetMUIRegistryInfo_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtGetMUIRegistryInfo@12", [=]
    {
        auto& core = d_->core;
        
        const auto Flags    = arg<nt32::ULONG>(core, 0);
        const auto DataSize = arg<nt32::PULONG>(core, 1);
        const auto Data     = arg<nt32::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[132]);

        on_func(Flags, DataSize, Data);
    });
}

opt<id_t> nt32::syscalls32::register_ZwGetNextProcess(proc_t proc, const on_ZwGetNextProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwGetNextProcess@20", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt32::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto HandleAttributes = arg<nt32::ULONG>(core, 2);
        const auto Flags            = arg<nt32::ULONG>(core, 3);
        const auto NewProcessHandle = arg<nt32::PHANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[133]);

        on_func(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwGetNextThread(proc_t proc, const on_ZwGetNextThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwGetNextThread@24", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt32::HANDLE>(core, 0);
        const auto ThreadHandle     = arg<nt32::HANDLE>(core, 1);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 2);
        const auto HandleAttributes = arg<nt32::ULONG>(core, 3);
        const auto Flags            = arg<nt32::ULONG>(core, 4);
        const auto NewThreadHandle  = arg<nt32::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[134]);

        on_func(ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtGetNlsSectionPtr(proc_t proc, const on_NtGetNlsSectionPtr_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtGetNlsSectionPtr@20", [=]
    {
        auto& core = d_->core;
        
        const auto SectionType        = arg<nt32::ULONG>(core, 0);
        const auto SectionData        = arg<nt32::ULONG>(core, 1);
        const auto ContextData        = arg<nt32::PVOID>(core, 2);
        const auto STARSectionPointer = arg<nt32::PVOID>(core, 3);
        const auto SectionSize        = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[135]);

        on_func(SectionType, SectionData, ContextData, STARSectionPointer, SectionSize);
    });
}

opt<id_t> nt32::syscalls32::register_ZwGetNotificationResourceManager(proc_t proc, const on_ZwGetNotificationResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwGetNotificationResourceManager@28", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle   = arg<nt32::HANDLE>(core, 0);
        const auto TransactionNotification = arg<nt32::PTRANSACTION_NOTIFICATION>(core, 1);
        const auto NotificationLength      = arg<nt32::ULONG>(core, 2);
        const auto Timeout                 = arg<nt32::PLARGE_INTEGER>(core, 3);
        const auto ReturnLength            = arg<nt32::PULONG>(core, 4);
        const auto Asynchronous            = arg<nt32::ULONG>(core, 5);
        const auto AsynchronousContext     = arg<nt32::ULONG_PTR>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[136]);

        on_func(ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext);
    });
}

opt<id_t> nt32::syscalls32::register_NtGetWriteWatch(proc_t proc, const on_NtGetWriteWatch_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtGetWriteWatch@28", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle             = arg<nt32::HANDLE>(core, 0);
        const auto Flags                     = arg<nt32::ULONG>(core, 1);
        const auto BaseAddress               = arg<nt32::PVOID>(core, 2);
        const auto RegionSize                = arg<nt32::SIZE_T>(core, 3);
        const auto STARUserAddressArray      = arg<nt32::PVOID>(core, 4);
        const auto EntriesInUserAddressArray = arg<nt32::PULONG_PTR>(core, 5);
        const auto Granularity               = arg<nt32::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[137]);

        on_func(ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity);
    });
}

opt<id_t> nt32::syscalls32::register_NtImpersonateAnonymousToken(proc_t proc, const on_NtImpersonateAnonymousToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtImpersonateAnonymousToken@4", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[138]);

        on_func(ThreadHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwImpersonateClientOfPort(proc_t proc, const on_ZwImpersonateClientOfPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwImpersonateClientOfPort@8", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt32::HANDLE>(core, 0);
        const auto Message    = arg<nt32::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[139]);

        on_func(PortHandle, Message);
    });
}

opt<id_t> nt32::syscalls32::register_ZwImpersonateThread(proc_t proc, const on_ZwImpersonateThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwImpersonateThread@12", [=]
    {
        auto& core = d_->core;
        
        const auto ServerThreadHandle = arg<nt32::HANDLE>(core, 0);
        const auto ClientThreadHandle = arg<nt32::HANDLE>(core, 1);
        const auto SecurityQos        = arg<nt32::PSECURITY_QUALITY_OF_SERVICE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[140]);

        on_func(ServerThreadHandle, ClientThreadHandle, SecurityQos);
    });
}

opt<id_t> nt32::syscalls32::register_NtInitializeNlsFiles(proc_t proc, const on_NtInitializeNlsFiles_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtInitializeNlsFiles@12", [=]
    {
        auto& core = d_->core;
        
        const auto STARBaseAddress        = arg<nt32::PVOID>(core, 0);
        const auto DefaultLocaleId        = arg<nt32::PLCID>(core, 1);
        const auto DefaultCasingTableSize = arg<nt32::PLARGE_INTEGER>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[141]);

        on_func(STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize);
    });
}

opt<id_t> nt32::syscalls32::register_ZwInitializeRegistry(proc_t proc, const on_ZwInitializeRegistry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwInitializeRegistry@4", [=]
    {
        auto& core = d_->core;
        
        const auto BootCondition = arg<nt32::USHORT>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[142]);

        on_func(BootCondition);
    });
}

opt<id_t> nt32::syscalls32::register_NtInitiatePowerAction(proc_t proc, const on_NtInitiatePowerAction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtInitiatePowerAction@16", [=]
    {
        auto& core = d_->core;
        
        const auto SystemAction   = arg<nt32::POWER_ACTION>(core, 0);
        const auto MinSystemState = arg<nt32::SYSTEM_POWER_STATE>(core, 1);
        const auto Flags          = arg<nt32::ULONG>(core, 2);
        const auto Asynchronous   = arg<nt32::BOOLEAN>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[143]);

        on_func(SystemAction, MinSystemState, Flags, Asynchronous);
    });
}

opt<id_t> nt32::syscalls32::register_ZwIsProcessInJob(proc_t proc, const on_ZwIsProcessInJob_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwIsProcessInJob@8", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 0);
        const auto JobHandle     = arg<nt32::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[144]);

        on_func(ProcessHandle, JobHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwListenPort(proc_t proc, const on_ZwListenPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwListenPort@8", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle        = arg<nt32::HANDLE>(core, 0);
        const auto ConnectionRequest = arg<nt32::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[145]);

        on_func(PortHandle, ConnectionRequest);
    });
}

opt<id_t> nt32::syscalls32::register_NtLoadDriver(proc_t proc, const on_NtLoadDriver_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtLoadDriver@4", [=]
    {
        auto& core = d_->core;
        
        const auto DriverServiceName = arg<nt32::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[146]);

        on_func(DriverServiceName);
    });
}

opt<id_t> nt32::syscalls32::register_NtLoadKey2(proc_t proc, const on_NtLoadKey2_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtLoadKey2@12", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey  = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile = arg<nt32::POBJECT_ATTRIBUTES>(core, 1);
        const auto Flags      = arg<nt32::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[147]);

        on_func(TargetKey, SourceFile, Flags);
    });
}

opt<id_t> nt32::syscalls32::register_NtLoadKeyEx(proc_t proc, const on_NtLoadKeyEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtLoadKeyEx@32", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey         = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile        = arg<nt32::POBJECT_ATTRIBUTES>(core, 1);
        const auto Flags             = arg<nt32::ULONG>(core, 2);
        const auto TrustClassKey     = arg<nt32::HANDLE>(core, 3);
        const auto Reserved          = arg<nt32::PVOID>(core, 4);
        const auto ObjectContext     = arg<nt32::PVOID>(core, 5);
        const auto CallbackReserverd = arg<nt32::PVOID>(core, 6);
        const auto IoStatusBlock     = arg<nt32::PVOID>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[148]);

        on_func(TargetKey, SourceFile, Flags, TrustClassKey, Reserved, ObjectContext, CallbackReserverd, IoStatusBlock);
    });
}

opt<id_t> nt32::syscalls32::register_NtLoadKey(proc_t proc, const on_NtLoadKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtLoadKey@8", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey  = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile = arg<nt32::POBJECT_ATTRIBUTES>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[149]);

        on_func(TargetKey, SourceFile);
    });
}

opt<id_t> nt32::syscalls32::register_NtLockFile(proc_t proc, const on_NtLockFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtLockFile@40", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle      = arg<nt32::HANDLE>(core, 0);
        const auto Event           = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine      = arg<nt32::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext      = arg<nt32::PVOID>(core, 3);
        const auto IoStatusBlock   = arg<nt32::PIO_STATUS_BLOCK>(core, 4);
        const auto ByteOffset      = arg<nt32::PLARGE_INTEGER>(core, 5);
        const auto Length          = arg<nt32::PLARGE_INTEGER>(core, 6);
        const auto Key             = arg<nt32::ULONG>(core, 7);
        const auto FailImmediately = arg<nt32::BOOLEAN>(core, 8);
        const auto ExclusiveLock   = arg<nt32::BOOLEAN>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[150]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock);
    });
}

opt<id_t> nt32::syscalls32::register_ZwLockProductActivationKeys(proc_t proc, const on_ZwLockProductActivationKeys_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwLockProductActivationKeys@8", [=]
    {
        auto& core = d_->core;
        
        const auto STARpPrivateVer = arg<nt32::ULONG>(core, 0);
        const auto STARpSafeMode   = arg<nt32::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[151]);

        on_func(STARpPrivateVer, STARpSafeMode);
    });
}

opt<id_t> nt32::syscalls32::register_NtLockRegistryKey(proc_t proc, const on_NtLockRegistryKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtLockRegistryKey@4", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[152]);

        on_func(KeyHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwLockVirtualMemory(proc_t proc, const on_ZwLockVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwLockVirtualMemory@16", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt32::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(core, 1);
        const auto RegionSize      = arg<nt32::PSIZE_T>(core, 2);
        const auto MapType         = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[153]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    });
}

opt<id_t> nt32::syscalls32::register_ZwMakePermanentObject(proc_t proc, const on_ZwMakePermanentObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwMakePermanentObject@4", [=]
    {
        auto& core = d_->core;
        
        const auto Handle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[154]);

        on_func(Handle);
    });
}

opt<id_t> nt32::syscalls32::register_NtMakeTemporaryObject(proc_t proc, const on_NtMakeTemporaryObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtMakeTemporaryObject@4", [=]
    {
        auto& core = d_->core;
        
        const auto Handle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[155]);

        on_func(Handle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwMapCMFModule(proc_t proc, const on_ZwMapCMFModule_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwMapCMFModule@24", [=]
    {
        auto& core = d_->core;
        
        const auto What            = arg<nt32::ULONG>(core, 0);
        const auto Index           = arg<nt32::ULONG>(core, 1);
        const auto CacheIndexOut   = arg<nt32::PULONG>(core, 2);
        const auto CacheFlagsOut   = arg<nt32::PULONG>(core, 3);
        const auto ViewSizeOut     = arg<nt32::PULONG>(core, 4);
        const auto STARBaseAddress = arg<nt32::PVOID>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[156]);

        on_func(What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress);
    });
}

opt<id_t> nt32::syscalls32::register_NtMapUserPhysicalPages(proc_t proc, const on_NtMapUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtMapUserPhysicalPages@12", [=]
    {
        auto& core = d_->core;
        
        const auto VirtualAddress = arg<nt32::PVOID>(core, 0);
        const auto NumberOfPages  = arg<nt32::ULONG_PTR>(core, 1);
        const auto UserPfnArra    = arg<nt32::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[157]);

        on_func(VirtualAddress, NumberOfPages, UserPfnArra);
    });
}

opt<id_t> nt32::syscalls32::register_ZwMapUserPhysicalPagesScatter(proc_t proc, const on_ZwMapUserPhysicalPagesScatter_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwMapUserPhysicalPagesScatter@12", [=]
    {
        auto& core = d_->core;
        
        const auto STARVirtualAddresses = arg<nt32::PVOID>(core, 0);
        const auto NumberOfPages        = arg<nt32::ULONG_PTR>(core, 1);
        const auto UserPfnArray         = arg<nt32::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[158]);

        on_func(STARVirtualAddresses, NumberOfPages, UserPfnArray);
    });
}

opt<id_t> nt32::syscalls32::register_ZwMapViewOfSection(proc_t proc, const on_ZwMapViewOfSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwMapViewOfSection@40", [=]
    {
        auto& core = d_->core;
        
        const auto SectionHandle      = arg<nt32::HANDLE>(core, 0);
        const auto ProcessHandle      = arg<nt32::HANDLE>(core, 1);
        const auto STARBaseAddress    = arg<nt32::PVOID>(core, 2);
        const auto ZeroBits           = arg<nt32::ULONG_PTR>(core, 3);
        const auto CommitSize         = arg<nt32::SIZE_T>(core, 4);
        const auto SectionOffset      = arg<nt32::PLARGE_INTEGER>(core, 5);
        const auto ViewSize           = arg<nt32::PSIZE_T>(core, 6);
        const auto InheritDisposition = arg<nt32::SECTION_INHERIT>(core, 7);
        const auto AllocationType     = arg<nt32::ULONG>(core, 8);
        const auto Win32Protect       = arg<nt32::WIN32_PROTECTION_MASK>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[159]);

        on_func(SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
    });
}

opt<id_t> nt32::syscalls32::register_NtModifyBootEntry(proc_t proc, const on_NtModifyBootEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtModifyBootEntry@4", [=]
    {
        auto& core = d_->core;
        
        const auto BootEntry = arg<nt32::PBOOT_ENTRY>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[160]);

        on_func(BootEntry);
    });
}

opt<id_t> nt32::syscalls32::register_ZwModifyDriverEntry(proc_t proc, const on_ZwModifyDriverEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwModifyDriverEntry@4", [=]
    {
        auto& core = d_->core;
        
        const auto DriverEntry = arg<nt32::PEFI_DRIVER_ENTRY>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[161]);

        on_func(DriverEntry);
    });
}

opt<id_t> nt32::syscalls32::register_NtNotifyChangeDirectoryFile(proc_t proc, const on_NtNotifyChangeDirectoryFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtNotifyChangeDirectoryFile@36", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle       = arg<nt32::HANDLE>(core, 0);
        const auto Event            = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine       = arg<nt32::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext       = arg<nt32::PVOID>(core, 3);
        const auto IoStatusBlock    = arg<nt32::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer           = arg<nt32::PVOID>(core, 5);
        const auto Length           = arg<nt32::ULONG>(core, 6);
        const auto CompletionFilter = arg<nt32::ULONG>(core, 7);
        const auto WatchTree        = arg<nt32::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[162]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree);
    });
}

opt<id_t> nt32::syscalls32::register_NtNotifyChangeKey(proc_t proc, const on_NtNotifyChangeKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtNotifyChangeKey@40", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle        = arg<nt32::HANDLE>(core, 0);
        const auto Event            = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine       = arg<nt32::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext       = arg<nt32::PVOID>(core, 3);
        const auto IoStatusBlock    = arg<nt32::PIO_STATUS_BLOCK>(core, 4);
        const auto CompletionFilter = arg<nt32::ULONG>(core, 5);
        const auto WatchTree        = arg<nt32::BOOLEAN>(core, 6);
        const auto Buffer           = arg<nt32::PVOID>(core, 7);
        const auto BufferSize       = arg<nt32::ULONG>(core, 8);
        const auto Asynchronous     = arg<nt32::BOOLEAN>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[163]);

        on_func(KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    });
}

opt<id_t> nt32::syscalls32::register_NtNotifyChangeMultipleKeys(proc_t proc, const on_NtNotifyChangeMultipleKeys_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtNotifyChangeMultipleKeys@48", [=]
    {
        auto& core = d_->core;
        
        const auto MasterKeyHandle  = arg<nt32::HANDLE>(core, 0);
        const auto Count            = arg<nt32::ULONG>(core, 1);
        const auto SlaveObjects     = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto Event            = arg<nt32::HANDLE>(core, 3);
        const auto ApcRoutine       = arg<nt32::PIO_APC_ROUTINE>(core, 4);
        const auto ApcContext       = arg<nt32::PVOID>(core, 5);
        const auto IoStatusBlock    = arg<nt32::PIO_STATUS_BLOCK>(core, 6);
        const auto CompletionFilter = arg<nt32::ULONG>(core, 7);
        const auto WatchTree        = arg<nt32::BOOLEAN>(core, 8);
        const auto Buffer           = arg<nt32::PVOID>(core, 9);
        const auto BufferSize       = arg<nt32::ULONG>(core, 10);
        const auto Asynchronous     = arg<nt32::BOOLEAN>(core, 11);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[164]);

        on_func(MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    });
}

opt<id_t> nt32::syscalls32::register_NtNotifyChangeSession(proc_t proc, const on_NtNotifyChangeSession_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtNotifyChangeSession@32", [=]
    {
        auto& core = d_->core;
        
        const auto Session         = arg<nt32::HANDLE>(core, 0);
        const auto IoStateSequence = arg<nt32::ULONG>(core, 1);
        const auto Reserved        = arg<nt32::PVOID>(core, 2);
        const auto Action          = arg<nt32::ULONG>(core, 3);
        const auto IoState         = arg<nt32::IO_SESSION_STATE>(core, 4);
        const auto IoState2        = arg<nt32::IO_SESSION_STATE>(core, 5);
        const auto Buffer          = arg<nt32::PVOID>(core, 6);
        const auto BufferSize      = arg<nt32::ULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[165]);

        on_func(Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenDirectoryObject(proc_t proc, const on_ZwOpenDirectoryObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenDirectoryObject@12", [=]
    {
        auto& core = d_->core;
        
        const auto DirectoryHandle  = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[166]);

        on_func(DirectoryHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenEnlistment(proc_t proc, const on_ZwOpenEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenEnlistment@20", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle      = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ResourceManagerHandle = arg<nt32::HANDLE>(core, 2);
        const auto EnlistmentGuid        = arg<nt32::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<nt32::POBJECT_ATTRIBUTES>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[167]);

        on_func(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenEvent(proc_t proc, const on_NtOpenEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenEvent@12", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle      = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[168]);

        on_func(EventHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenEventPair(proc_t proc, const on_NtOpenEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenEventPair@12", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle  = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[169]);

        on_func(EventPairHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenFile(proc_t proc, const on_NtOpenFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenFile@24", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle       = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock    = arg<nt32::PIO_STATUS_BLOCK>(core, 3);
        const auto ShareAccess      = arg<nt32::ULONG>(core, 4);
        const auto OpenOptions      = arg<nt32::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[170]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenIoCompletion(proc_t proc, const on_ZwOpenIoCompletion_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenIoCompletion@12", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[171]);

        on_func(IoCompletionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenJobObject(proc_t proc, const on_ZwOpenJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenJobObject@12", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle        = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[172]);

        on_func(JobHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenKeyedEvent(proc_t proc, const on_NtOpenKeyedEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenKeyedEvent@12", [=]
    {
        auto& core = d_->core;
        
        const auto KeyedEventHandle = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[173]);

        on_func(KeyedEventHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenKeyEx(proc_t proc, const on_ZwOpenKeyEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenKeyEx@16", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle        = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto OpenOptions      = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[174]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenKey(proc_t proc, const on_ZwOpenKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenKey@12", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle        = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[175]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenKeyTransactedEx(proc_t proc, const on_NtOpenKeyTransactedEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenKeyTransactedEx@20", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle         = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto OpenOptions       = arg<nt32::ULONG>(core, 3);
        const auto TransactionHandle = arg<nt32::HANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[176]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenKeyTransacted(proc_t proc, const on_NtOpenKeyTransacted_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenKeyTransacted@16", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle         = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto TransactionHandle = arg<nt32::HANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[177]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenMutant(proc_t proc, const on_NtOpenMutant_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenMutant@12", [=]
    {
        auto& core = d_->core;
        
        const auto MutantHandle     = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[178]);

        on_func(MutantHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenObjectAuditAlarm(proc_t proc, const on_ZwOpenObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenObjectAuditAlarm@48", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName      = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto HandleId           = arg<nt32::PVOID>(core, 1);
        const auto ObjectTypeName     = arg<nt32::PUNICODE_STRING>(core, 2);
        const auto ObjectName         = arg<nt32::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor = arg<nt32::PSECURITY_DESCRIPTOR>(core, 4);
        const auto ClientToken        = arg<nt32::HANDLE>(core, 5);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(core, 6);
        const auto GrantedAccess      = arg<nt32::ACCESS_MASK>(core, 7);
        const auto Privileges         = arg<nt32::PPRIVILEGE_SET>(core, 8);
        const auto ObjectCreation     = arg<nt32::BOOLEAN>(core, 9);
        const auto AccessGranted      = arg<nt32::BOOLEAN>(core, 10);
        const auto GenerateOnClose    = arg<nt32::PBOOLEAN>(core, 11);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[179]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenPrivateNamespace(proc_t proc, const on_NtOpenPrivateNamespace_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenPrivateNamespace@16", [=]
    {
        auto& core = d_->core;
        
        const auto NamespaceHandle    = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto BoundaryDescriptor = arg<nt32::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[180]);

        on_func(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenProcess(proc_t proc, const on_ZwOpenProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenProcess@16", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto ClientId         = arg<nt32::PCLIENT_ID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[181]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenProcessTokenEx(proc_t proc, const on_ZwOpenProcessTokenEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenProcessTokenEx@16", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt32::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto HandleAttributes = arg<nt32::ULONG>(core, 2);
        const auto TokenHandle      = arg<nt32::PHANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[182]);

        on_func(ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenProcessToken(proc_t proc, const on_ZwOpenProcessToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenProcessToken@12", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 0);
        const auto DesiredAccess = arg<nt32::ACCESS_MASK>(core, 1);
        const auto TokenHandle   = arg<nt32::PHANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[183]);

        on_func(ProcessHandle, DesiredAccess, TokenHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenResourceManager(proc_t proc, const on_ZwOpenResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenResourceManager@20", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt32::ACCESS_MASK>(core, 1);
        const auto TmHandle              = arg<nt32::HANDLE>(core, 2);
        const auto ResourceManagerGuid   = arg<nt32::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<nt32::POBJECT_ATTRIBUTES>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[184]);

        on_func(ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenSection(proc_t proc, const on_NtOpenSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenSection@12", [=]
    {
        auto& core = d_->core;
        
        const auto SectionHandle    = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[185]);

        on_func(SectionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenSemaphore(proc_t proc, const on_NtOpenSemaphore_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenSemaphore@12", [=]
    {
        auto& core = d_->core;
        
        const auto SemaphoreHandle  = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[186]);

        on_func(SemaphoreHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenSession(proc_t proc, const on_NtOpenSession_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenSession@12", [=]
    {
        auto& core = d_->core;
        
        const auto SessionHandle    = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[187]);

        on_func(SessionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenSymbolicLinkObject(proc_t proc, const on_NtOpenSymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenSymbolicLinkObject@12", [=]
    {
        auto& core = d_->core;
        
        const auto LinkHandle       = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[188]);

        on_func(LinkHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenThread(proc_t proc, const on_ZwOpenThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenThread@16", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle     = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto ClientId         = arg<nt32::PCLIENT_ID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[189]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenThreadTokenEx(proc_t proc, const on_NtOpenThreadTokenEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenThreadTokenEx@20", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle     = arg<nt32::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto OpenAsSelf       = arg<nt32::BOOLEAN>(core, 2);
        const auto HandleAttributes = arg<nt32::ULONG>(core, 3);
        const auto TokenHandle      = arg<nt32::PHANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[190]);

        on_func(ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtOpenThreadToken(proc_t proc, const on_NtOpenThreadToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtOpenThreadToken@16", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle  = arg<nt32::HANDLE>(core, 0);
        const auto DesiredAccess = arg<nt32::ACCESS_MASK>(core, 1);
        const auto OpenAsSelf    = arg<nt32::BOOLEAN>(core, 2);
        const auto TokenHandle   = arg<nt32::PHANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[191]);

        on_func(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenTimer(proc_t proc, const on_ZwOpenTimer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenTimer@12", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle      = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[192]);

        on_func(TimerHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenTransactionManager(proc_t proc, const on_ZwOpenTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenTransactionManager@24", [=]
    {
        auto& core = d_->core;
        
        const auto TmHandle         = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto LogFileName      = arg<nt32::PUNICODE_STRING>(core, 3);
        const auto TmIdentity       = arg<nt32::LPGUID>(core, 4);
        const auto OpenOptions      = arg<nt32::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[193]);

        on_func(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions);
    });
}

opt<id_t> nt32::syscalls32::register_ZwOpenTransaction(proc_t proc, const on_ZwOpenTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwOpenTransaction@20", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle = arg<nt32::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt32::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);
        const auto Uow               = arg<nt32::LPGUID>(core, 3);
        const auto TmHandle          = arg<nt32::HANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[194]);

        on_func(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtPlugPlayControl(proc_t proc, const on_NtPlugPlayControl_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtPlugPlayControl@12", [=]
    {
        auto& core = d_->core;
        
        const auto PnPControlClass      = arg<nt32::PLUGPLAY_CONTROL_CLASS>(core, 0);
        const auto PnPControlData       = arg<nt32::PVOID>(core, 1);
        const auto PnPControlDataLength = arg<nt32::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[195]);

        on_func(PnPControlClass, PnPControlData, PnPControlDataLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwPowerInformation(proc_t proc, const on_ZwPowerInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwPowerInformation@20", [=]
    {
        auto& core = d_->core;
        
        const auto InformationLevel   = arg<nt32::POWER_INFORMATION_LEVEL>(core, 0);
        const auto InputBuffer        = arg<nt32::PVOID>(core, 1);
        const auto InputBufferLength  = arg<nt32::ULONG>(core, 2);
        const auto OutputBuffer       = arg<nt32::PVOID>(core, 3);
        const auto OutputBufferLength = arg<nt32::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[196]);

        on_func(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtPrepareComplete(proc_t proc, const on_NtPrepareComplete_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtPrepareComplete@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[197]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_ZwPrepareEnlistment(proc_t proc, const on_ZwPrepareEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwPrepareEnlistment@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[198]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_ZwPrePrepareComplete(proc_t proc, const on_ZwPrePrepareComplete_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwPrePrepareComplete@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[199]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_NtPrePrepareEnlistment(proc_t proc, const on_NtPrePrepareEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtPrePrepareEnlistment@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[200]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_ZwPrivilegeCheck(proc_t proc, const on_ZwPrivilegeCheck_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwPrivilegeCheck@12", [=]
    {
        auto& core = d_->core;
        
        const auto ClientToken        = arg<nt32::HANDLE>(core, 0);
        const auto RequiredPrivileges = arg<nt32::PPRIVILEGE_SET>(core, 1);
        const auto Result             = arg<nt32::PBOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[201]);

        on_func(ClientToken, RequiredPrivileges, Result);
    });
}

opt<id_t> nt32::syscalls32::register_NtPrivilegedServiceAuditAlarm(proc_t proc, const on_NtPrivilegedServiceAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtPrivilegedServiceAuditAlarm@20", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto ServiceName   = arg<nt32::PUNICODE_STRING>(core, 1);
        const auto ClientToken   = arg<nt32::HANDLE>(core, 2);
        const auto Privileges    = arg<nt32::PPRIVILEGE_SET>(core, 3);
        const auto AccessGranted = arg<nt32::BOOLEAN>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[202]);

        on_func(SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted);
    });
}

opt<id_t> nt32::syscalls32::register_ZwPrivilegeObjectAuditAlarm(proc_t proc, const on_ZwPrivilegeObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwPrivilegeObjectAuditAlarm@24", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto HandleId      = arg<nt32::PVOID>(core, 1);
        const auto ClientToken   = arg<nt32::HANDLE>(core, 2);
        const auto DesiredAccess = arg<nt32::ACCESS_MASK>(core, 3);
        const auto Privileges    = arg<nt32::PPRIVILEGE_SET>(core, 4);
        const auto AccessGranted = arg<nt32::BOOLEAN>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[203]);

        on_func(SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);
    });
}

opt<id_t> nt32::syscalls32::register_NtPropagationComplete(proc_t proc, const on_NtPropagationComplete_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtPropagationComplete@16", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle = arg<nt32::HANDLE>(core, 0);
        const auto RequestCookie         = arg<nt32::ULONG>(core, 1);
        const auto BufferLength          = arg<nt32::ULONG>(core, 2);
        const auto Buffer                = arg<nt32::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[204]);

        on_func(ResourceManagerHandle, RequestCookie, BufferLength, Buffer);
    });
}

opt<id_t> nt32::syscalls32::register_ZwPropagationFailed(proc_t proc, const on_ZwPropagationFailed_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwPropagationFailed@12", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle = arg<nt32::HANDLE>(core, 0);
        const auto RequestCookie         = arg<nt32::ULONG>(core, 1);
        const auto PropStatus            = arg<nt32::NTSTATUS>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[205]);

        on_func(ResourceManagerHandle, RequestCookie, PropStatus);
    });
}

opt<id_t> nt32::syscalls32::register_ZwProtectVirtualMemory(proc_t proc, const on_ZwProtectVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwProtectVirtualMemory@20", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt32::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(core, 1);
        const auto RegionSize      = arg<nt32::PSIZE_T>(core, 2);
        const auto NewProtectWin32 = arg<nt32::WIN32_PROTECTION_MASK>(core, 3);
        const auto OldProtect      = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[206]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect);
    });
}

opt<id_t> nt32::syscalls32::register_ZwPulseEvent(proc_t proc, const on_ZwPulseEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwPulseEvent@8", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle   = arg<nt32::HANDLE>(core, 0);
        const auto PreviousState = arg<nt32::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[207]);

        on_func(EventHandle, PreviousState);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryAttributesFile(proc_t proc, const on_ZwQueryAttributesFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryAttributesFile@8", [=]
    {
        auto& core = d_->core;
        
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);
        const auto FileInformation  = arg<nt32::PFILE_BASIC_INFORMATION>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[208]);

        on_func(ObjectAttributes, FileInformation);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryBootEntryOrder(proc_t proc, const on_ZwQueryBootEntryOrder_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryBootEntryOrder@8", [=]
    {
        auto& core = d_->core;
        
        const auto Ids   = arg<nt32::PULONG>(core, 0);
        const auto Count = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[209]);

        on_func(Ids, Count);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryBootOptions(proc_t proc, const on_ZwQueryBootOptions_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryBootOptions@8", [=]
    {
        auto& core = d_->core;
        
        const auto BootOptions       = arg<nt32::PBOOT_OPTIONS>(core, 0);
        const auto BootOptionsLength = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[210]);

        on_func(BootOptions, BootOptionsLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryDebugFilterState(proc_t proc, const on_NtQueryDebugFilterState_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryDebugFilterState@8", [=]
    {
        auto& core = d_->core;
        
        const auto ComponentId = arg<nt32::ULONG>(core, 0);
        const auto Level       = arg<nt32::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[211]);

        on_func(ComponentId, Level);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryDefaultLocale(proc_t proc, const on_NtQueryDefaultLocale_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryDefaultLocale@8", [=]
    {
        auto& core = d_->core;
        
        const auto UserProfile     = arg<nt32::BOOLEAN>(core, 0);
        const auto DefaultLocaleId = arg<nt32::PLCID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[212]);

        on_func(UserProfile, DefaultLocaleId);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryDefaultUILanguage(proc_t proc, const on_ZwQueryDefaultUILanguage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryDefaultUILanguage@4", [=]
    {
        auto& core = d_->core;
        
        const auto STARDefaultUILanguageId = arg<nt32::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[213]);

        on_func(STARDefaultUILanguageId);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryDirectoryFile(proc_t proc, const on_ZwQueryDirectoryFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryDirectoryFile@44", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle           = arg<nt32::HANDLE>(core, 0);
        const auto Event                = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine           = arg<nt32::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext           = arg<nt32::PVOID>(core, 3);
        const auto IoStatusBlock        = arg<nt32::PIO_STATUS_BLOCK>(core, 4);
        const auto FileInformation      = arg<nt32::PVOID>(core, 5);
        const auto Length               = arg<nt32::ULONG>(core, 6);
        const auto FileInformationClass = arg<nt32::FILE_INFORMATION_CLASS>(core, 7);
        const auto ReturnSingleEntry    = arg<nt32::BOOLEAN>(core, 8);
        const auto FileName             = arg<nt32::PUNICODE_STRING>(core, 9);
        const auto RestartScan          = arg<nt32::BOOLEAN>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[214]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryDirectoryObject(proc_t proc, const on_ZwQueryDirectoryObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryDirectoryObject@28", [=]
    {
        auto& core = d_->core;
        
        const auto DirectoryHandle   = arg<nt32::HANDLE>(core, 0);
        const auto Buffer            = arg<nt32::PVOID>(core, 1);
        const auto Length            = arg<nt32::ULONG>(core, 2);
        const auto ReturnSingleEntry = arg<nt32::BOOLEAN>(core, 3);
        const auto RestartScan       = arg<nt32::BOOLEAN>(core, 4);
        const auto Context           = arg<nt32::PULONG>(core, 5);
        const auto ReturnLength      = arg<nt32::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[215]);

        on_func(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryDriverEntryOrder(proc_t proc, const on_NtQueryDriverEntryOrder_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryDriverEntryOrder@8", [=]
    {
        auto& core = d_->core;
        
        const auto Ids   = arg<nt32::PULONG>(core, 0);
        const auto Count = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[216]);

        on_func(Ids, Count);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryEaFile(proc_t proc, const on_ZwQueryEaFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryEaFile@36", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle        = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer            = arg<nt32::PVOID>(core, 2);
        const auto Length            = arg<nt32::ULONG>(core, 3);
        const auto ReturnSingleEntry = arg<nt32::BOOLEAN>(core, 4);
        const auto EaList            = arg<nt32::PVOID>(core, 5);
        const auto EaListLength      = arg<nt32::ULONG>(core, 6);
        const auto EaIndex           = arg<nt32::PULONG>(core, 7);
        const auto RestartScan       = arg<nt32::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[217]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryEvent(proc_t proc, const on_NtQueryEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryEvent@20", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle            = arg<nt32::HANDLE>(core, 0);
        const auto EventInformationClass  = arg<nt32::EVENT_INFORMATION_CLASS>(core, 1);
        const auto EventInformation       = arg<nt32::PVOID>(core, 2);
        const auto EventInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength           = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[218]);

        on_func(EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryFullAttributesFile(proc_t proc, const on_ZwQueryFullAttributesFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryFullAttributesFile@8", [=]
    {
        auto& core = d_->core;
        
        const auto ObjectAttributes = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);
        const auto FileInformation  = arg<nt32::PFILE_NETWORK_OPEN_INFORMATION>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[219]);

        on_func(ObjectAttributes, FileInformation);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryInformationAtom(proc_t proc, const on_NtQueryInformationAtom_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryInformationAtom@20", [=]
    {
        auto& core = d_->core;
        
        const auto Atom                  = arg<nt32::RTL_ATOM>(core, 0);
        const auto InformationClass      = arg<nt32::ATOM_INFORMATION_CLASS>(core, 1);
        const auto AtomInformation       = arg<nt32::PVOID>(core, 2);
        const auto AtomInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength          = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[220]);

        on_func(Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryInformationEnlistment(proc_t proc, const on_ZwQueryInformationEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryInformationEnlistment@20", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle            = arg<nt32::HANDLE>(core, 0);
        const auto EnlistmentInformationClass  = arg<nt32::ENLISTMENT_INFORMATION_CLASS>(core, 1);
        const auto EnlistmentInformation       = arg<nt32::PVOID>(core, 2);
        const auto EnlistmentInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength                = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[221]);

        on_func(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryInformationFile(proc_t proc, const on_ZwQueryInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryInformationFile@20", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle           = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock        = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto FileInformation      = arg<nt32::PVOID>(core, 2);
        const auto Length               = arg<nt32::ULONG>(core, 3);
        const auto FileInformationClass = arg<nt32::FILE_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[222]);

        on_func(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryInformationJobObject(proc_t proc, const on_ZwQueryInformationJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryInformationJobObject@20", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle                  = arg<nt32::HANDLE>(core, 0);
        const auto JobObjectInformationClass  = arg<nt32::JOBOBJECTINFOCLASS>(core, 1);
        const auto JobObjectInformation       = arg<nt32::PVOID>(core, 2);
        const auto JobObjectInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength               = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[223]);

        on_func(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryInformationPort(proc_t proc, const on_ZwQueryInformationPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryInformationPort@20", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle           = arg<nt32::HANDLE>(core, 0);
        const auto PortInformationClass = arg<nt32::PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<nt32::PVOID>(core, 2);
        const auto Length               = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength         = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[224]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryInformationProcess(proc_t proc, const on_ZwQueryInformationProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryInformationProcess@20", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle            = arg<nt32::HANDLE>(core, 0);
        const auto ProcessInformationClass  = arg<nt32::PROCESSINFOCLASS>(core, 1);
        const auto ProcessInformation       = arg<nt32::PVOID>(core, 2);
        const auto ProcessInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength             = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[225]);

        on_func(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryInformationResourceManager(proc_t proc, const on_ZwQueryInformationResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryInformationResourceManager@20", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle            = arg<nt32::HANDLE>(core, 0);
        const auto ResourceManagerInformationClass  = arg<nt32::RESOURCEMANAGER_INFORMATION_CLASS>(core, 1);
        const auto ResourceManagerInformation       = arg<nt32::PVOID>(core, 2);
        const auto ResourceManagerInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength                     = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[226]);

        on_func(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryInformationThread(proc_t proc, const on_NtQueryInformationThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryInformationThread@20", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle            = arg<nt32::HANDLE>(core, 0);
        const auto ThreadInformationClass  = arg<nt32::THREADINFOCLASS>(core, 1);
        const auto ThreadInformation       = arg<nt32::PVOID>(core, 2);
        const auto ThreadInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength            = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[227]);

        on_func(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryInformationToken(proc_t proc, const on_ZwQueryInformationToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryInformationToken@20", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle            = arg<nt32::HANDLE>(core, 0);
        const auto TokenInformationClass  = arg<nt32::TOKEN_INFORMATION_CLASS>(core, 1);
        const auto TokenInformation       = arg<nt32::PVOID>(core, 2);
        const auto TokenInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength           = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[228]);

        on_func(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryInformationTransaction(proc_t proc, const on_ZwQueryInformationTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryInformationTransaction@20", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle            = arg<nt32::HANDLE>(core, 0);
        const auto TransactionInformationClass  = arg<nt32::TRANSACTION_INFORMATION_CLASS>(core, 1);
        const auto TransactionInformation       = arg<nt32::PVOID>(core, 2);
        const auto TransactionInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength                 = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[229]);

        on_func(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryInformationTransactionManager(proc_t proc, const on_NtQueryInformationTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryInformationTransactionManager@20", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionManagerHandle            = arg<nt32::HANDLE>(core, 0);
        const auto TransactionManagerInformationClass  = arg<nt32::TRANSACTIONMANAGER_INFORMATION_CLASS>(core, 1);
        const auto TransactionManagerInformation       = arg<nt32::PVOID>(core, 2);
        const auto TransactionManagerInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength                        = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[230]);

        on_func(TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryInformationWorkerFactory(proc_t proc, const on_ZwQueryInformationWorkerFactory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryInformationWorkerFactory@20", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle            = arg<nt32::HANDLE>(core, 0);
        const auto WorkerFactoryInformationClass  = arg<nt32::WORKERFACTORYINFOCLASS>(core, 1);
        const auto WorkerFactoryInformation       = arg<nt32::PVOID>(core, 2);
        const auto WorkerFactoryInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength                   = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[231]);

        on_func(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryInstallUILanguage(proc_t proc, const on_NtQueryInstallUILanguage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryInstallUILanguage@4", [=]
    {
        auto& core = d_->core;
        
        const auto STARInstallUILanguageId = arg<nt32::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[232]);

        on_func(STARInstallUILanguageId);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryIntervalProfile(proc_t proc, const on_NtQueryIntervalProfile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryIntervalProfile@8", [=]
    {
        auto& core = d_->core;
        
        const auto ProfileSource = arg<nt32::KPROFILE_SOURCE>(core, 0);
        const auto Interval      = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[233]);

        on_func(ProfileSource, Interval);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryIoCompletion(proc_t proc, const on_NtQueryIoCompletion_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryIoCompletion@20", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle            = arg<nt32::HANDLE>(core, 0);
        const auto IoCompletionInformationClass  = arg<nt32::IO_COMPLETION_INFORMATION_CLASS>(core, 1);
        const auto IoCompletionInformation       = arg<nt32::PVOID>(core, 2);
        const auto IoCompletionInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength                  = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[234]);

        on_func(IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryKey(proc_t proc, const on_ZwQueryKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryKey@20", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle           = arg<nt32::HANDLE>(core, 0);
        const auto KeyInformationClass = arg<nt32::KEY_INFORMATION_CLASS>(core, 1);
        const auto KeyInformation      = arg<nt32::PVOID>(core, 2);
        const auto Length              = arg<nt32::ULONG>(core, 3);
        const auto ResultLength        = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[235]);

        on_func(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryLicenseValue(proc_t proc, const on_NtQueryLicenseValue_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryLicenseValue@20", [=]
    {
        auto& core = d_->core;
        
        const auto Name           = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto Type           = arg<nt32::PULONG>(core, 1);
        const auto Buffer         = arg<nt32::PVOID>(core, 2);
        const auto Length         = arg<nt32::ULONG>(core, 3);
        const auto ReturnedLength = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[236]);

        on_func(Name, Type, Buffer, Length, ReturnedLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryMultipleValueKey(proc_t proc, const on_NtQueryMultipleValueKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryMultipleValueKey@24", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle            = arg<nt32::HANDLE>(core, 0);
        const auto ValueEntries         = arg<nt32::PKEY_VALUE_ENTRY>(core, 1);
        const auto EntryCount           = arg<nt32::ULONG>(core, 2);
        const auto ValueBuffer          = arg<nt32::PVOID>(core, 3);
        const auto BufferLength         = arg<nt32::PULONG>(core, 4);
        const auto RequiredBufferLength = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[237]);

        on_func(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryMutant(proc_t proc, const on_NtQueryMutant_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryMutant@20", [=]
    {
        auto& core = d_->core;
        
        const auto MutantHandle            = arg<nt32::HANDLE>(core, 0);
        const auto MutantInformationClass  = arg<nt32::MUTANT_INFORMATION_CLASS>(core, 1);
        const auto MutantInformation       = arg<nt32::PVOID>(core, 2);
        const auto MutantInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength            = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[238]);

        on_func(MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryObject(proc_t proc, const on_NtQueryObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryObject@20", [=]
    {
        auto& core = d_->core;
        
        const auto Handle                  = arg<nt32::HANDLE>(core, 0);
        const auto ObjectInformationClass  = arg<nt32::OBJECT_INFORMATION_CLASS>(core, 1);
        const auto ObjectInformation       = arg<nt32::PVOID>(core, 2);
        const auto ObjectInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength            = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[239]);

        on_func(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryOpenSubKeysEx(proc_t proc, const on_NtQueryOpenSubKeysEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryOpenSubKeysEx@16", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey    = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);
        const auto BufferLength = arg<nt32::ULONG>(core, 1);
        const auto Buffer       = arg<nt32::PVOID>(core, 2);
        const auto RequiredSize = arg<nt32::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[240]);

        on_func(TargetKey, BufferLength, Buffer, RequiredSize);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryOpenSubKeys(proc_t proc, const on_NtQueryOpenSubKeys_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryOpenSubKeys@8", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey   = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);
        const auto HandleCount = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[241]);

        on_func(TargetKey, HandleCount);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryPerformanceCounter(proc_t proc, const on_NtQueryPerformanceCounter_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryPerformanceCounter@8", [=]
    {
        auto& core = d_->core;
        
        const auto PerformanceCounter   = arg<nt32::PLARGE_INTEGER>(core, 0);
        const auto PerformanceFrequency = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[242]);

        on_func(PerformanceCounter, PerformanceFrequency);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryQuotaInformationFile(proc_t proc, const on_ZwQueryQuotaInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryQuotaInformationFile@36", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle        = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock     = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer            = arg<nt32::PVOID>(core, 2);
        const auto Length            = arg<nt32::ULONG>(core, 3);
        const auto ReturnSingleEntry = arg<nt32::BOOLEAN>(core, 4);
        const auto SidList           = arg<nt32::PVOID>(core, 5);
        const auto SidListLength     = arg<nt32::ULONG>(core, 6);
        const auto StartSid          = arg<nt32::PULONG>(core, 7);
        const auto RestartScan       = arg<nt32::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[243]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQuerySection(proc_t proc, const on_ZwQuerySection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQuerySection@20", [=]
    {
        auto& core = d_->core;
        
        const auto SectionHandle            = arg<nt32::HANDLE>(core, 0);
        const auto SectionInformationClass  = arg<nt32::SECTION_INFORMATION_CLASS>(core, 1);
        const auto SectionInformation       = arg<nt32::PVOID>(core, 2);
        const auto SectionInformationLength = arg<nt32::SIZE_T>(core, 3);
        const auto ReturnLength             = arg<nt32::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[244]);

        on_func(SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQuerySecurityAttributesToken(proc_t proc, const on_ZwQuerySecurityAttributesToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQuerySecurityAttributesToken@24", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle        = arg<nt32::HANDLE>(core, 0);
        const auto Attributes         = arg<nt32::PUNICODE_STRING>(core, 1);
        const auto NumberOfAttributes = arg<nt32::ULONG>(core, 2);
        const auto Buffer             = arg<nt32::PVOID>(core, 3);
        const auto Length             = arg<nt32::ULONG>(core, 4);
        const auto ReturnLength       = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[245]);

        on_func(TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQuerySecurityObject(proc_t proc, const on_NtQuerySecurityObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQuerySecurityObject@20", [=]
    {
        auto& core = d_->core;
        
        const auto Handle              = arg<nt32::HANDLE>(core, 0);
        const auto SecurityInformation = arg<nt32::SECURITY_INFORMATION>(core, 1);
        const auto SecurityDescriptor  = arg<nt32::PSECURITY_DESCRIPTOR>(core, 2);
        const auto Length              = arg<nt32::ULONG>(core, 3);
        const auto LengthNeeded        = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[246]);

        on_func(Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQuerySemaphore(proc_t proc, const on_ZwQuerySemaphore_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQuerySemaphore@20", [=]
    {
        auto& core = d_->core;
        
        const auto SemaphoreHandle            = arg<nt32::HANDLE>(core, 0);
        const auto SemaphoreInformationClass  = arg<nt32::SEMAPHORE_INFORMATION_CLASS>(core, 1);
        const auto SemaphoreInformation       = arg<nt32::PVOID>(core, 2);
        const auto SemaphoreInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength               = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[247]);

        on_func(SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQuerySymbolicLinkObject(proc_t proc, const on_ZwQuerySymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQuerySymbolicLinkObject@12", [=]
    {
        auto& core = d_->core;
        
        const auto LinkHandle     = arg<nt32::HANDLE>(core, 0);
        const auto LinkTarget     = arg<nt32::PUNICODE_STRING>(core, 1);
        const auto ReturnedLength = arg<nt32::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[248]);

        on_func(LinkHandle, LinkTarget, ReturnedLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQuerySystemEnvironmentValueEx(proc_t proc, const on_ZwQuerySystemEnvironmentValueEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQuerySystemEnvironmentValueEx@20", [=]
    {
        auto& core = d_->core;
        
        const auto VariableName = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto VendorGuid   = arg<nt32::LPGUID>(core, 1);
        const auto Value        = arg<nt32::PVOID>(core, 2);
        const auto ValueLength  = arg<nt32::PULONG>(core, 3);
        const auto Attributes   = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[249]);

        on_func(VariableName, VendorGuid, Value, ValueLength, Attributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQuerySystemEnvironmentValue(proc_t proc, const on_ZwQuerySystemEnvironmentValue_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQuerySystemEnvironmentValue@16", [=]
    {
        auto& core = d_->core;
        
        const auto VariableName  = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto VariableValue = arg<nt32::PWSTR>(core, 1);
        const auto ValueLength   = arg<nt32::USHORT>(core, 2);
        const auto ReturnLength  = arg<nt32::PUSHORT>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[250]);

        on_func(VariableName, VariableValue, ValueLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQuerySystemInformationEx(proc_t proc, const on_ZwQuerySystemInformationEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQuerySystemInformationEx@24", [=]
    {
        auto& core = d_->core;
        
        const auto SystemInformationClass  = arg<nt32::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto QueryInformation        = arg<nt32::PVOID>(core, 1);
        const auto QueryInformationLength  = arg<nt32::ULONG>(core, 2);
        const auto SystemInformation       = arg<nt32::PVOID>(core, 3);
        const auto SystemInformationLength = arg<nt32::ULONG>(core, 4);
        const auto ReturnLength            = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[251]);

        on_func(SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQuerySystemInformation(proc_t proc, const on_NtQuerySystemInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQuerySystemInformation@16", [=]
    {
        auto& core = d_->core;
        
        const auto SystemInformationClass  = arg<nt32::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto SystemInformation       = arg<nt32::PVOID>(core, 1);
        const auto SystemInformationLength = arg<nt32::ULONG>(core, 2);
        const auto ReturnLength            = arg<nt32::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[252]);

        on_func(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQuerySystemTime(proc_t proc, const on_NtQuerySystemTime_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQuerySystemTime@4", [=]
    {
        auto& core = d_->core;
        
        const auto SystemTime = arg<nt32::PLARGE_INTEGER>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[253]);

        on_func(SystemTime);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryTimer(proc_t proc, const on_ZwQueryTimer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryTimer@20", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle            = arg<nt32::HANDLE>(core, 0);
        const auto TimerInformationClass  = arg<nt32::TIMER_INFORMATION_CLASS>(core, 1);
        const auto TimerInformation       = arg<nt32::PVOID>(core, 2);
        const auto TimerInformationLength = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength           = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[254]);

        on_func(TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryTimerResolution(proc_t proc, const on_NtQueryTimerResolution_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryTimerResolution@12", [=]
    {
        auto& core = d_->core;
        
        const auto MaximumTime = arg<nt32::PULONG>(core, 0);
        const auto MinimumTime = arg<nt32::PULONG>(core, 1);
        const auto CurrentTime = arg<nt32::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[255]);

        on_func(MaximumTime, MinimumTime, CurrentTime);
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryValueKey(proc_t proc, const on_ZwQueryValueKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryValueKey@24", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle                = arg<nt32::HANDLE>(core, 0);
        const auto ValueName                = arg<nt32::PUNICODE_STRING>(core, 1);
        const auto KeyValueInformationClass = arg<nt32::KEY_VALUE_INFORMATION_CLASS>(core, 2);
        const auto KeyValueInformation      = arg<nt32::PVOID>(core, 3);
        const auto Length                   = arg<nt32::ULONG>(core, 4);
        const auto ResultLength             = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[256]);

        on_func(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryVirtualMemory(proc_t proc, const on_NtQueryVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryVirtualMemory@24", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle           = arg<nt32::HANDLE>(core, 0);
        const auto BaseAddress             = arg<nt32::PVOID>(core, 1);
        const auto MemoryInformationClass  = arg<nt32::MEMORY_INFORMATION_CLASS>(core, 2);
        const auto MemoryInformation       = arg<nt32::PVOID>(core, 3);
        const auto MemoryInformationLength = arg<nt32::SIZE_T>(core, 4);
        const auto ReturnLength            = arg<nt32::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[257]);

        on_func(ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueryVolumeInformationFile(proc_t proc, const on_NtQueryVolumeInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueryVolumeInformationFile@20", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle         = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto FsInformation      = arg<nt32::PVOID>(core, 2);
        const auto Length             = arg<nt32::ULONG>(core, 3);
        const auto FsInformationClass = arg<nt32::FS_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[258]);

        on_func(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueueApcThreadEx(proc_t proc, const on_NtQueueApcThreadEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueueApcThreadEx@24", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle         = arg<nt32::HANDLE>(core, 0);
        const auto UserApcReserveHandle = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine           = arg<nt32::PPS_APC_ROUTINE>(core, 2);
        const auto ApcArgument1         = arg<nt32::PVOID>(core, 3);
        const auto ApcArgument2         = arg<nt32::PVOID>(core, 4);
        const auto ApcArgument3         = arg<nt32::PVOID>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[259]);

        on_func(ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    });
}

opt<id_t> nt32::syscalls32::register_NtQueueApcThread(proc_t proc, const on_NtQueueApcThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtQueueApcThread@20", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle = arg<nt32::HANDLE>(core, 0);
        const auto ApcRoutine   = arg<nt32::PPS_APC_ROUTINE>(core, 1);
        const auto ApcArgument1 = arg<nt32::PVOID>(core, 2);
        const auto ApcArgument2 = arg<nt32::PVOID>(core, 3);
        const auto ApcArgument3 = arg<nt32::PVOID>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[260]);

        on_func(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRaiseException(proc_t proc, const on_ZwRaiseException_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRaiseException@12", [=]
    {
        auto& core = d_->core;
        
        const auto ExceptionRecord = arg<nt32::PEXCEPTION_RECORD>(core, 0);
        const auto ContextRecord   = arg<nt32::PCONTEXT>(core, 1);
        const auto FirstChance     = arg<nt32::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[261]);

        on_func(ExceptionRecord, ContextRecord, FirstChance);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRaiseHardError(proc_t proc, const on_ZwRaiseHardError_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRaiseHardError@24", [=]
    {
        auto& core = d_->core;
        
        const auto ErrorStatus                = arg<nt32::NTSTATUS>(core, 0);
        const auto NumberOfParameters         = arg<nt32::ULONG>(core, 1);
        const auto UnicodeStringParameterMask = arg<nt32::ULONG>(core, 2);
        const auto Parameters                 = arg<nt32::PULONG_PTR>(core, 3);
        const auto ValidResponseOptions       = arg<nt32::ULONG>(core, 4);
        const auto Response                   = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[262]);

        on_func(ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);
    });
}

opt<id_t> nt32::syscalls32::register_NtReadFile(proc_t proc, const on_NtReadFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtReadFile@36", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt32::HANDLE>(core, 0);
        const auto Event         = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt32::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt32::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer        = arg<nt32::PVOID>(core, 5);
        const auto Length        = arg<nt32::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt32::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt32::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[263]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    });
}

opt<id_t> nt32::syscalls32::register_NtReadFileScatter(proc_t proc, const on_NtReadFileScatter_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtReadFileScatter@36", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt32::HANDLE>(core, 0);
        const auto Event         = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt32::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt32::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(core, 4);
        const auto SegmentArray  = arg<nt32::PFILE_SEGMENT_ELEMENT>(core, 5);
        const auto Length        = arg<nt32::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt32::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt32::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[264]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    });
}

opt<id_t> nt32::syscalls32::register_ZwReadOnlyEnlistment(proc_t proc, const on_ZwReadOnlyEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwReadOnlyEnlistment@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[265]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_ZwReadRequestData(proc_t proc, const on_ZwReadRequestData_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwReadRequestData@24", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle        = arg<nt32::HANDLE>(core, 0);
        const auto Message           = arg<nt32::PPORT_MESSAGE>(core, 1);
        const auto DataEntryIndex    = arg<nt32::ULONG>(core, 2);
        const auto Buffer            = arg<nt32::PVOID>(core, 3);
        const auto BufferSize        = arg<nt32::SIZE_T>(core, 4);
        const auto NumberOfBytesRead = arg<nt32::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[266]);

        on_func(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead);
    });
}

opt<id_t> nt32::syscalls32::register_NtReadVirtualMemory(proc_t proc, const on_NtReadVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtReadVirtualMemory@20", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle     = arg<nt32::HANDLE>(core, 0);
        const auto BaseAddress       = arg<nt32::PVOID>(core, 1);
        const auto Buffer            = arg<nt32::PVOID>(core, 2);
        const auto BufferSize        = arg<nt32::SIZE_T>(core, 3);
        const auto NumberOfBytesRead = arg<nt32::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[267]);

        on_func(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead);
    });
}

opt<id_t> nt32::syscalls32::register_NtRecoverEnlistment(proc_t proc, const on_NtRecoverEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtRecoverEnlistment@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto EnlistmentKey    = arg<nt32::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[268]);

        on_func(EnlistmentHandle, EnlistmentKey);
    });
}

opt<id_t> nt32::syscalls32::register_NtRecoverResourceManager(proc_t proc, const on_NtRecoverResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtRecoverResourceManager@4", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[269]);

        on_func(ResourceManagerHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRecoverTransactionManager(proc_t proc, const on_ZwRecoverTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRecoverTransactionManager@4", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionManagerHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[270]);

        on_func(TransactionManagerHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRegisterProtocolAddressInformation(proc_t proc, const on_ZwRegisterProtocolAddressInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRegisterProtocolAddressInformation@20", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManager         = arg<nt32::HANDLE>(core, 0);
        const auto ProtocolId              = arg<nt32::PCRM_PROTOCOL_ID>(core, 1);
        const auto ProtocolInformationSize = arg<nt32::ULONG>(core, 2);
        const auto ProtocolInformation     = arg<nt32::PVOID>(core, 3);
        const auto CreateOptions           = arg<nt32::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[271]);

        on_func(ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRegisterThreadTerminatePort(proc_t proc, const on_ZwRegisterThreadTerminatePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRegisterThreadTerminatePort@4", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[272]);

        on_func(PortHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtReleaseKeyedEvent(proc_t proc, const on_NtReleaseKeyedEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtReleaseKeyedEvent@16", [=]
    {
        auto& core = d_->core;
        
        const auto KeyedEventHandle = arg<nt32::HANDLE>(core, 0);
        const auto KeyValue         = arg<nt32::PVOID>(core, 1);
        const auto Alertable        = arg<nt32::BOOLEAN>(core, 2);
        const auto Timeout          = arg<nt32::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[273]);

        on_func(KeyedEventHandle, KeyValue, Alertable, Timeout);
    });
}

opt<id_t> nt32::syscalls32::register_ZwReleaseMutant(proc_t proc, const on_ZwReleaseMutant_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwReleaseMutant@8", [=]
    {
        auto& core = d_->core;
        
        const auto MutantHandle  = arg<nt32::HANDLE>(core, 0);
        const auto PreviousCount = arg<nt32::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[274]);

        on_func(MutantHandle, PreviousCount);
    });
}

opt<id_t> nt32::syscalls32::register_NtReleaseSemaphore(proc_t proc, const on_NtReleaseSemaphore_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtReleaseSemaphore@12", [=]
    {
        auto& core = d_->core;
        
        const auto SemaphoreHandle = arg<nt32::HANDLE>(core, 0);
        const auto ReleaseCount    = arg<nt32::LONG>(core, 1);
        const auto PreviousCount   = arg<nt32::PLONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[275]);

        on_func(SemaphoreHandle, ReleaseCount, PreviousCount);
    });
}

opt<id_t> nt32::syscalls32::register_ZwReleaseWorkerFactoryWorker(proc_t proc, const on_ZwReleaseWorkerFactoryWorker_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwReleaseWorkerFactoryWorker@4", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[276]);

        on_func(WorkerFactoryHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRemoveIoCompletionEx(proc_t proc, const on_ZwRemoveIoCompletionEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRemoveIoCompletionEx@24", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle      = arg<nt32::HANDLE>(core, 0);
        const auto IoCompletionInformation = arg<nt32::PFILE_IO_COMPLETION_INFORMATION>(core, 1);
        const auto Count                   = arg<nt32::ULONG>(core, 2);
        const auto NumEntriesRemoved       = arg<nt32::PULONG>(core, 3);
        const auto Timeout                 = arg<nt32::PLARGE_INTEGER>(core, 4);
        const auto Alertable               = arg<nt32::BOOLEAN>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[277]);

        on_func(IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRemoveIoCompletion(proc_t proc, const on_ZwRemoveIoCompletion_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRemoveIoCompletion@20", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle = arg<nt32::HANDLE>(core, 0);
        const auto STARKeyContext     = arg<nt32::PVOID>(core, 1);
        const auto STARApcContext     = arg<nt32::PVOID>(core, 2);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(core, 3);
        const auto Timeout            = arg<nt32::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[278]);

        on_func(IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRemoveProcessDebug(proc_t proc, const on_ZwRemoveProcessDebug_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRemoveProcessDebug@8", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle     = arg<nt32::HANDLE>(core, 0);
        const auto DebugObjectHandle = arg<nt32::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[279]);

        on_func(ProcessHandle, DebugObjectHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRenameKey(proc_t proc, const on_ZwRenameKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRenameKey@8", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle = arg<nt32::HANDLE>(core, 0);
        const auto NewName   = arg<nt32::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[280]);

        on_func(KeyHandle, NewName);
    });
}

opt<id_t> nt32::syscalls32::register_NtRenameTransactionManager(proc_t proc, const on_NtRenameTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtRenameTransactionManager@8", [=]
    {
        auto& core = d_->core;
        
        const auto LogFileName                    = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto ExistingTransactionManagerGuid = arg<nt32::LPGUID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[281]);

        on_func(LogFileName, ExistingTransactionManagerGuid);
    });
}

opt<id_t> nt32::syscalls32::register_ZwReplaceKey(proc_t proc, const on_ZwReplaceKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwReplaceKey@12", [=]
    {
        auto& core = d_->core;
        
        const auto NewFile      = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);
        const auto TargetHandle = arg<nt32::HANDLE>(core, 1);
        const auto OldFile      = arg<nt32::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[282]);

        on_func(NewFile, TargetHandle, OldFile);
    });
}

opt<id_t> nt32::syscalls32::register_NtReplacePartitionUnit(proc_t proc, const on_NtReplacePartitionUnit_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtReplacePartitionUnit@12", [=]
    {
        auto& core = d_->core;
        
        const auto TargetInstancePath = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto SpareInstancePath  = arg<nt32::PUNICODE_STRING>(core, 1);
        const auto Flags              = arg<nt32::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[283]);

        on_func(TargetInstancePath, SpareInstancePath, Flags);
    });
}

opt<id_t> nt32::syscalls32::register_ZwReplyPort(proc_t proc, const on_ZwReplyPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwReplyPort@8", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle   = arg<nt32::HANDLE>(core, 0);
        const auto ReplyMessage = arg<nt32::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[284]);

        on_func(PortHandle, ReplyMessage);
    });
}

opt<id_t> nt32::syscalls32::register_NtReplyWaitReceivePortEx(proc_t proc, const on_NtReplyWaitReceivePortEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtReplyWaitReceivePortEx@20", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle      = arg<nt32::HANDLE>(core, 0);
        const auto STARPortContext = arg<nt32::PVOID>(core, 1);
        const auto ReplyMessage    = arg<nt32::PPORT_MESSAGE>(core, 2);
        const auto ReceiveMessage  = arg<nt32::PPORT_MESSAGE>(core, 3);
        const auto Timeout         = arg<nt32::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[285]);

        on_func(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout);
    });
}

opt<id_t> nt32::syscalls32::register_NtReplyWaitReceivePort(proc_t proc, const on_NtReplyWaitReceivePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtReplyWaitReceivePort@16", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle      = arg<nt32::HANDLE>(core, 0);
        const auto STARPortContext = arg<nt32::PVOID>(core, 1);
        const auto ReplyMessage    = arg<nt32::PPORT_MESSAGE>(core, 2);
        const auto ReceiveMessage  = arg<nt32::PPORT_MESSAGE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[286]);

        on_func(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage);
    });
}

opt<id_t> nt32::syscalls32::register_NtReplyWaitReplyPort(proc_t proc, const on_NtReplyWaitReplyPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtReplyWaitReplyPort@8", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle   = arg<nt32::HANDLE>(core, 0);
        const auto ReplyMessage = arg<nt32::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[287]);

        on_func(PortHandle, ReplyMessage);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRequestPort(proc_t proc, const on_ZwRequestPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRequestPort@8", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle     = arg<nt32::HANDLE>(core, 0);
        const auto RequestMessage = arg<nt32::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[288]);

        on_func(PortHandle, RequestMessage);
    });
}

opt<id_t> nt32::syscalls32::register_NtRequestWaitReplyPort(proc_t proc, const on_NtRequestWaitReplyPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtRequestWaitReplyPort@12", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle     = arg<nt32::HANDLE>(core, 0);
        const auto RequestMessage = arg<nt32::PPORT_MESSAGE>(core, 1);
        const auto ReplyMessage   = arg<nt32::PPORT_MESSAGE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[289]);

        on_func(PortHandle, RequestMessage, ReplyMessage);
    });
}

opt<id_t> nt32::syscalls32::register_NtResetEvent(proc_t proc, const on_NtResetEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtResetEvent@8", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle   = arg<nt32::HANDLE>(core, 0);
        const auto PreviousState = arg<nt32::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[290]);

        on_func(EventHandle, PreviousState);
    });
}

opt<id_t> nt32::syscalls32::register_ZwResetWriteWatch(proc_t proc, const on_ZwResetWriteWatch_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwResetWriteWatch@12", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 0);
        const auto BaseAddress   = arg<nt32::PVOID>(core, 1);
        const auto RegionSize    = arg<nt32::SIZE_T>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[291]);

        on_func(ProcessHandle, BaseAddress, RegionSize);
    });
}

opt<id_t> nt32::syscalls32::register_NtRestoreKey(proc_t proc, const on_NtRestoreKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtRestoreKey@12", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle  = arg<nt32::HANDLE>(core, 0);
        const auto FileHandle = arg<nt32::HANDLE>(core, 1);
        const auto Flags      = arg<nt32::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[292]);

        on_func(KeyHandle, FileHandle, Flags);
    });
}

opt<id_t> nt32::syscalls32::register_ZwResumeProcess(proc_t proc, const on_ZwResumeProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwResumeProcess@4", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[293]);

        on_func(ProcessHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwResumeThread(proc_t proc, const on_ZwResumeThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwResumeThread@8", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle         = arg<nt32::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[294]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<id_t> nt32::syscalls32::register_ZwRollbackComplete(proc_t proc, const on_ZwRollbackComplete_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwRollbackComplete@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[295]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_NtRollbackEnlistment(proc_t proc, const on_NtRollbackEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtRollbackEnlistment@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[296]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_NtRollbackTransaction(proc_t proc, const on_NtRollbackTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtRollbackTransaction@8", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle = arg<nt32::HANDLE>(core, 0);
        const auto Wait              = arg<nt32::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[297]);

        on_func(TransactionHandle, Wait);
    });
}

opt<id_t> nt32::syscalls32::register_NtRollforwardTransactionManager(proc_t proc, const on_NtRollforwardTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtRollforwardTransactionManager@8", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionManagerHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock           = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[298]);

        on_func(TransactionManagerHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_NtSaveKeyEx(proc_t proc, const on_NtSaveKeyEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSaveKeyEx@12", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle  = arg<nt32::HANDLE>(core, 0);
        const auto FileHandle = arg<nt32::HANDLE>(core, 1);
        const auto Format     = arg<nt32::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[299]);

        on_func(KeyHandle, FileHandle, Format);
    });
}

opt<id_t> nt32::syscalls32::register_NtSaveKey(proc_t proc, const on_NtSaveKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSaveKey@8", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle  = arg<nt32::HANDLE>(core, 0);
        const auto FileHandle = arg<nt32::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[300]);

        on_func(KeyHandle, FileHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtSaveMergedKeys(proc_t proc, const on_NtSaveMergedKeys_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSaveMergedKeys@12", [=]
    {
        auto& core = d_->core;
        
        const auto HighPrecedenceKeyHandle = arg<nt32::HANDLE>(core, 0);
        const auto LowPrecedenceKeyHandle  = arg<nt32::HANDLE>(core, 1);
        const auto FileHandle              = arg<nt32::HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[301]);

        on_func(HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtSecureConnectPort(proc_t proc, const on_NtSecureConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSecureConnectPort@36", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle                  = arg<nt32::PHANDLE>(core, 0);
        const auto PortName                    = arg<nt32::PUNICODE_STRING>(core, 1);
        const auto SecurityQos                 = arg<nt32::PSECURITY_QUALITY_OF_SERVICE>(core, 2);
        const auto ClientView                  = arg<nt32::PPORT_VIEW>(core, 3);
        const auto RequiredServerSid           = arg<nt32::PSID>(core, 4);
        const auto ServerView                  = arg<nt32::PREMOTE_PORT_VIEW>(core, 5);
        const auto MaxMessageLength            = arg<nt32::PULONG>(core, 6);
        const auto ConnectionInformation       = arg<nt32::PVOID>(core, 7);
        const auto ConnectionInformationLength = arg<nt32::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[302]);

        on_func(PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetBootEntryOrder(proc_t proc, const on_ZwSetBootEntryOrder_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetBootEntryOrder@8", [=]
    {
        auto& core = d_->core;
        
        const auto Ids   = arg<nt32::PULONG>(core, 0);
        const auto Count = arg<nt32::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[303]);

        on_func(Ids, Count);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetBootOptions(proc_t proc, const on_ZwSetBootOptions_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetBootOptions@8", [=]
    {
        auto& core = d_->core;
        
        const auto BootOptions    = arg<nt32::PBOOT_OPTIONS>(core, 0);
        const auto FieldsToChange = arg<nt32::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[304]);

        on_func(BootOptions, FieldsToChange);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetContextThread(proc_t proc, const on_ZwSetContextThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetContextThread@8", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle  = arg<nt32::HANDLE>(core, 0);
        const auto ThreadContext = arg<nt32::PCONTEXT>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[305]);

        on_func(ThreadHandle, ThreadContext);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetDebugFilterState(proc_t proc, const on_NtSetDebugFilterState_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetDebugFilterState@12", [=]
    {
        auto& core = d_->core;
        
        const auto ComponentId = arg<nt32::ULONG>(core, 0);
        const auto Level       = arg<nt32::ULONG>(core, 1);
        const auto State       = arg<nt32::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[306]);

        on_func(ComponentId, Level, State);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetDefaultHardErrorPort(proc_t proc, const on_NtSetDefaultHardErrorPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetDefaultHardErrorPort@4", [=]
    {
        auto& core = d_->core;
        
        const auto DefaultHardErrorPort = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[307]);

        on_func(DefaultHardErrorPort);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetDefaultLocale(proc_t proc, const on_NtSetDefaultLocale_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetDefaultLocale@8", [=]
    {
        auto& core = d_->core;
        
        const auto UserProfile     = arg<nt32::BOOLEAN>(core, 0);
        const auto DefaultLocaleId = arg<nt32::LCID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[308]);

        on_func(UserProfile, DefaultLocaleId);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetDefaultUILanguage(proc_t proc, const on_ZwSetDefaultUILanguage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetDefaultUILanguage@4", [=]
    {
        auto& core = d_->core;
        
        const auto DefaultUILanguageId = arg<nt32::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[309]);

        on_func(DefaultUILanguageId);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetDriverEntryOrder(proc_t proc, const on_NtSetDriverEntryOrder_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetDriverEntryOrder@8", [=]
    {
        auto& core = d_->core;
        
        const auto Ids   = arg<nt32::PULONG>(core, 0);
        const auto Count = arg<nt32::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[310]);

        on_func(Ids, Count);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetEaFile(proc_t proc, const on_ZwSetEaFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetEaFile@16", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer        = arg<nt32::PVOID>(core, 2);
        const auto Length        = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[311]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetEventBoostPriority(proc_t proc, const on_NtSetEventBoostPriority_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetEventBoostPriority@4", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[312]);

        on_func(EventHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetEvent(proc_t proc, const on_NtSetEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetEvent@8", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle   = arg<nt32::HANDLE>(core, 0);
        const auto PreviousState = arg<nt32::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[313]);

        on_func(EventHandle, PreviousState);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetHighEventPair(proc_t proc, const on_NtSetHighEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetHighEventPair@4", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[314]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetHighWaitLowEventPair(proc_t proc, const on_NtSetHighWaitLowEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetHighWaitLowEventPair@4", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[315]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationDebugObject(proc_t proc, const on_ZwSetInformationDebugObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationDebugObject@20", [=]
    {
        auto& core = d_->core;
        
        const auto DebugObjectHandle           = arg<nt32::HANDLE>(core, 0);
        const auto DebugObjectInformationClass = arg<nt32::DEBUGOBJECTINFOCLASS>(core, 1);
        const auto DebugInformation            = arg<nt32::PVOID>(core, 2);
        const auto DebugInformationLength      = arg<nt32::ULONG>(core, 3);
        const auto ReturnLength                = arg<nt32::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[316]);

        on_func(DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetInformationEnlistment(proc_t proc, const on_NtSetInformationEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetInformationEnlistment@16", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle            = arg<nt32::HANDLE>(core, 0);
        const auto EnlistmentInformationClass  = arg<nt32::ENLISTMENT_INFORMATION_CLASS>(core, 1);
        const auto EnlistmentInformation       = arg<nt32::PVOID>(core, 2);
        const auto EnlistmentInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[317]);

        on_func(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationFile(proc_t proc, const on_ZwSetInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationFile@20", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle           = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock        = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto FileInformation      = arg<nt32::PVOID>(core, 2);
        const auto Length               = arg<nt32::ULONG>(core, 3);
        const auto FileInformationClass = arg<nt32::FILE_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[318]);

        on_func(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationJobObject(proc_t proc, const on_ZwSetInformationJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationJobObject@16", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle                  = arg<nt32::HANDLE>(core, 0);
        const auto JobObjectInformationClass  = arg<nt32::JOBOBJECTINFOCLASS>(core, 1);
        const auto JobObjectInformation       = arg<nt32::PVOID>(core, 2);
        const auto JobObjectInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[319]);

        on_func(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationKey(proc_t proc, const on_ZwSetInformationKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationKey@16", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle               = arg<nt32::HANDLE>(core, 0);
        const auto KeySetInformationClass  = arg<nt32::KEY_SET_INFORMATION_CLASS>(core, 1);
        const auto KeySetInformation       = arg<nt32::PVOID>(core, 2);
        const auto KeySetInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[320]);

        on_func(KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationObject(proc_t proc, const on_ZwSetInformationObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationObject@16", [=]
    {
        auto& core = d_->core;
        
        const auto Handle                  = arg<nt32::HANDLE>(core, 0);
        const auto ObjectInformationClass  = arg<nt32::OBJECT_INFORMATION_CLASS>(core, 1);
        const auto ObjectInformation       = arg<nt32::PVOID>(core, 2);
        const auto ObjectInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[321]);

        on_func(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationProcess(proc_t proc, const on_ZwSetInformationProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationProcess@16", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle            = arg<nt32::HANDLE>(core, 0);
        const auto ProcessInformationClass  = arg<nt32::PROCESSINFOCLASS>(core, 1);
        const auto ProcessInformation       = arg<nt32::PVOID>(core, 2);
        const auto ProcessInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[322]);

        on_func(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationResourceManager(proc_t proc, const on_ZwSetInformationResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationResourceManager@16", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle            = arg<nt32::HANDLE>(core, 0);
        const auto ResourceManagerInformationClass  = arg<nt32::RESOURCEMANAGER_INFORMATION_CLASS>(core, 1);
        const auto ResourceManagerInformation       = arg<nt32::PVOID>(core, 2);
        const auto ResourceManagerInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[323]);

        on_func(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationThread(proc_t proc, const on_ZwSetInformationThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationThread@16", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle            = arg<nt32::HANDLE>(core, 0);
        const auto ThreadInformationClass  = arg<nt32::THREADINFOCLASS>(core, 1);
        const auto ThreadInformation       = arg<nt32::PVOID>(core, 2);
        const auto ThreadInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[324]);

        on_func(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationToken(proc_t proc, const on_ZwSetInformationToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationToken@16", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle            = arg<nt32::HANDLE>(core, 0);
        const auto TokenInformationClass  = arg<nt32::TOKEN_INFORMATION_CLASS>(core, 1);
        const auto TokenInformation       = arg<nt32::PVOID>(core, 2);
        const auto TokenInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[325]);

        on_func(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationTransaction(proc_t proc, const on_ZwSetInformationTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationTransaction@16", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle            = arg<nt32::HANDLE>(core, 0);
        const auto TransactionInformationClass  = arg<nt32::TRANSACTION_INFORMATION_CLASS>(core, 1);
        const auto TransactionInformation       = arg<nt32::PVOID>(core, 2);
        const auto TransactionInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[326]);

        on_func(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationTransactionManager(proc_t proc, const on_ZwSetInformationTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationTransactionManager@16", [=]
    {
        auto& core = d_->core;
        
        const auto TmHandle                            = arg<nt32::HANDLE>(core, 0);
        const auto TransactionManagerInformationClass  = arg<nt32::TRANSACTIONMANAGER_INFORMATION_CLASS>(core, 1);
        const auto TransactionManagerInformation       = arg<nt32::PVOID>(core, 2);
        const auto TransactionManagerInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[327]);

        on_func(TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetInformationWorkerFactory(proc_t proc, const on_ZwSetInformationWorkerFactory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetInformationWorkerFactory@16", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle            = arg<nt32::HANDLE>(core, 0);
        const auto WorkerFactoryInformationClass  = arg<nt32::WORKERFACTORYINFOCLASS>(core, 1);
        const auto WorkerFactoryInformation       = arg<nt32::PVOID>(core, 2);
        const auto WorkerFactoryInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[328]);

        on_func(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetIntervalProfile(proc_t proc, const on_NtSetIntervalProfile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetIntervalProfile@8", [=]
    {
        auto& core = d_->core;
        
        const auto Interval = arg<nt32::ULONG>(core, 0);
        const auto Source   = arg<nt32::KPROFILE_SOURCE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[329]);

        on_func(Interval, Source);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetIoCompletionEx(proc_t proc, const on_NtSetIoCompletionEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetIoCompletionEx@24", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle        = arg<nt32::HANDLE>(core, 0);
        const auto IoCompletionReserveHandle = arg<nt32::HANDLE>(core, 1);
        const auto KeyContext                = arg<nt32::PVOID>(core, 2);
        const auto ApcContext                = arg<nt32::PVOID>(core, 3);
        const auto IoStatus                  = arg<nt32::NTSTATUS>(core, 4);
        const auto IoStatusInformation       = arg<nt32::ULONG_PTR>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[330]);

        on_func(IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetIoCompletion(proc_t proc, const on_NtSetIoCompletion_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetIoCompletion@20", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle  = arg<nt32::HANDLE>(core, 0);
        const auto KeyContext          = arg<nt32::PVOID>(core, 1);
        const auto ApcContext          = arg<nt32::PVOID>(core, 2);
        const auto IoStatus            = arg<nt32::NTSTATUS>(core, 3);
        const auto IoStatusInformation = arg<nt32::ULONG_PTR>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[331]);

        on_func(IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetLdtEntries(proc_t proc, const on_ZwSetLdtEntries_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetLdtEntries@24", [=]
    {
        auto& core = d_->core;
        
        const auto Selector0 = arg<nt32::ULONG>(core, 0);
        const auto Entry0Low = arg<nt32::ULONG>(core, 1);
        const auto Entry0Hi  = arg<nt32::ULONG>(core, 2);
        const auto Selector1 = arg<nt32::ULONG>(core, 3);
        const auto Entry1Low = arg<nt32::ULONG>(core, 4);
        const auto Entry1Hi  = arg<nt32::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[332]);

        on_func(Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetLowEventPair(proc_t proc, const on_ZwSetLowEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetLowEventPair@4", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[333]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetLowWaitHighEventPair(proc_t proc, const on_ZwSetLowWaitHighEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetLowWaitHighEventPair@4", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[334]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetQuotaInformationFile(proc_t proc, const on_ZwSetQuotaInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetQuotaInformationFile@16", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer        = arg<nt32::PVOID>(core, 2);
        const auto Length        = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[335]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetSecurityObject(proc_t proc, const on_NtSetSecurityObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetSecurityObject@12", [=]
    {
        auto& core = d_->core;
        
        const auto Handle              = arg<nt32::HANDLE>(core, 0);
        const auto SecurityInformation = arg<nt32::SECURITY_INFORMATION>(core, 1);
        const auto SecurityDescriptor  = arg<nt32::PSECURITY_DESCRIPTOR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[336]);

        on_func(Handle, SecurityInformation, SecurityDescriptor);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetSystemEnvironmentValueEx(proc_t proc, const on_ZwSetSystemEnvironmentValueEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetSystemEnvironmentValueEx@20", [=]
    {
        auto& core = d_->core;
        
        const auto VariableName = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto VendorGuid   = arg<nt32::LPGUID>(core, 1);
        const auto Value        = arg<nt32::PVOID>(core, 2);
        const auto ValueLength  = arg<nt32::ULONG>(core, 3);
        const auto Attributes   = arg<nt32::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[337]);

        on_func(VariableName, VendorGuid, Value, ValueLength, Attributes);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetSystemEnvironmentValue(proc_t proc, const on_ZwSetSystemEnvironmentValue_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetSystemEnvironmentValue@8", [=]
    {
        auto& core = d_->core;
        
        const auto VariableName  = arg<nt32::PUNICODE_STRING>(core, 0);
        const auto VariableValue = arg<nt32::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[338]);

        on_func(VariableName, VariableValue);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetSystemInformation(proc_t proc, const on_ZwSetSystemInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetSystemInformation@12", [=]
    {
        auto& core = d_->core;
        
        const auto SystemInformationClass  = arg<nt32::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto SystemInformation       = arg<nt32::PVOID>(core, 1);
        const auto SystemInformationLength = arg<nt32::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[339]);

        on_func(SystemInformationClass, SystemInformation, SystemInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetSystemPowerState(proc_t proc, const on_ZwSetSystemPowerState_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetSystemPowerState@12", [=]
    {
        auto& core = d_->core;
        
        const auto SystemAction   = arg<nt32::POWER_ACTION>(core, 0);
        const auto MinSystemState = arg<nt32::SYSTEM_POWER_STATE>(core, 1);
        const auto Flags          = arg<nt32::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[340]);

        on_func(SystemAction, MinSystemState, Flags);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetSystemTime(proc_t proc, const on_ZwSetSystemTime_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetSystemTime@8", [=]
    {
        auto& core = d_->core;
        
        const auto SystemTime   = arg<nt32::PLARGE_INTEGER>(core, 0);
        const auto PreviousTime = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[341]);

        on_func(SystemTime, PreviousTime);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetThreadExecutionState(proc_t proc, const on_ZwSetThreadExecutionState_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetThreadExecutionState@8", [=]
    {
        auto& core = d_->core;
        
        const auto esFlags           = arg<nt32::EXECUTION_STATE>(core, 0);
        const auto STARPreviousFlags = arg<nt32::EXECUTION_STATE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[342]);

        on_func(esFlags, STARPreviousFlags);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetTimerEx(proc_t proc, const on_ZwSetTimerEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetTimerEx@16", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle               = arg<nt32::HANDLE>(core, 0);
        const auto TimerSetInformationClass  = arg<nt32::TIMER_SET_INFORMATION_CLASS>(core, 1);
        const auto TimerSetInformation       = arg<nt32::PVOID>(core, 2);
        const auto TimerSetInformationLength = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[343]);

        on_func(TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetTimer(proc_t proc, const on_ZwSetTimer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetTimer@28", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle     = arg<nt32::HANDLE>(core, 0);
        const auto DueTime         = arg<nt32::PLARGE_INTEGER>(core, 1);
        const auto TimerApcRoutine = arg<nt32::PTIMER_APC_ROUTINE>(core, 2);
        const auto TimerContext    = arg<nt32::PVOID>(core, 3);
        const auto WakeTimer       = arg<nt32::BOOLEAN>(core, 4);
        const auto Period          = arg<nt32::LONG>(core, 5);
        const auto PreviousState   = arg<nt32::PBOOLEAN>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[344]);

        on_func(TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetTimerResolution(proc_t proc, const on_NtSetTimerResolution_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetTimerResolution@12", [=]
    {
        auto& core = d_->core;
        
        const auto DesiredTime   = arg<nt32::ULONG>(core, 0);
        const auto SetResolution = arg<nt32::BOOLEAN>(core, 1);
        const auto ActualTime    = arg<nt32::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[345]);

        on_func(DesiredTime, SetResolution, ActualTime);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetUuidSeed(proc_t proc, const on_ZwSetUuidSeed_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetUuidSeed@4", [=]
    {
        auto& core = d_->core;
        
        const auto Seed = arg<nt32::PCHAR>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[346]);

        on_func(Seed);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSetValueKey(proc_t proc, const on_ZwSetValueKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSetValueKey@24", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle  = arg<nt32::HANDLE>(core, 0);
        const auto ValueName  = arg<nt32::PUNICODE_STRING>(core, 1);
        const auto TitleIndex = arg<nt32::ULONG>(core, 2);
        const auto Type       = arg<nt32::ULONG>(core, 3);
        const auto Data       = arg<nt32::PVOID>(core, 4);
        const auto DataSize   = arg<nt32::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[347]);

        on_func(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
    });
}

opt<id_t> nt32::syscalls32::register_NtSetVolumeInformationFile(proc_t proc, const on_NtSetVolumeInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSetVolumeInformationFile@20", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle         = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock      = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto FsInformation      = arg<nt32::PVOID>(core, 2);
        const auto Length             = arg<nt32::ULONG>(core, 3);
        const auto FsInformationClass = arg<nt32::FS_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[348]);

        on_func(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    });
}

opt<id_t> nt32::syscalls32::register_ZwShutdownSystem(proc_t proc, const on_ZwShutdownSystem_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwShutdownSystem@4", [=]
    {
        auto& core = d_->core;
        
        const auto Action = arg<nt32::SHUTDOWN_ACTION>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[349]);

        on_func(Action);
    });
}

opt<id_t> nt32::syscalls32::register_NtShutdownWorkerFactory(proc_t proc, const on_NtShutdownWorkerFactory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtShutdownWorkerFactory@8", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle    = arg<nt32::HANDLE>(core, 0);
        const auto STARPendingWorkerCount = arg<nt32::LONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[350]);

        on_func(WorkerFactoryHandle, STARPendingWorkerCount);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSignalAndWaitForSingleObject(proc_t proc, const on_ZwSignalAndWaitForSingleObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSignalAndWaitForSingleObject@16", [=]
    {
        auto& core = d_->core;
        
        const auto SignalHandle = arg<nt32::HANDLE>(core, 0);
        const auto WaitHandle   = arg<nt32::HANDLE>(core, 1);
        const auto Alertable    = arg<nt32::BOOLEAN>(core, 2);
        const auto Timeout      = arg<nt32::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[351]);

        on_func(SignalHandle, WaitHandle, Alertable, Timeout);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSinglePhaseReject(proc_t proc, const on_ZwSinglePhaseReject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSinglePhaseReject@8", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt32::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt32::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[352]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt32::syscalls32::register_NtStartProfile(proc_t proc, const on_NtStartProfile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtStartProfile@4", [=]
    {
        auto& core = d_->core;
        
        const auto ProfileHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[353]);

        on_func(ProfileHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwStopProfile(proc_t proc, const on_ZwStopProfile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwStopProfile@4", [=]
    {
        auto& core = d_->core;
        
        const auto ProfileHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[354]);

        on_func(ProfileHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSuspendProcess(proc_t proc, const on_ZwSuspendProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSuspendProcess@4", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[355]);

        on_func(ProcessHandle);
    });
}

opt<id_t> nt32::syscalls32::register_ZwSuspendThread(proc_t proc, const on_ZwSuspendThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwSuspendThread@8", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle         = arg<nt32::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<nt32::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[356]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<id_t> nt32::syscalls32::register_NtSystemDebugControl(proc_t proc, const on_NtSystemDebugControl_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSystemDebugControl@24", [=]
    {
        auto& core = d_->core;
        
        const auto Command            = arg<nt32::SYSDBG_COMMAND>(core, 0);
        const auto InputBuffer        = arg<nt32::PVOID>(core, 1);
        const auto InputBufferLength  = arg<nt32::ULONG>(core, 2);
        const auto OutputBuffer       = arg<nt32::PVOID>(core, 3);
        const auto OutputBufferLength = arg<nt32::ULONG>(core, 4);
        const auto ReturnLength       = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[357]);

        on_func(Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwTerminateJobObject(proc_t proc, const on_ZwTerminateJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwTerminateJobObject@8", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle  = arg<nt32::HANDLE>(core, 0);
        const auto ExitStatus = arg<nt32::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[358]);

        on_func(JobHandle, ExitStatus);
    });
}

opt<id_t> nt32::syscalls32::register_ZwTerminateProcess(proc_t proc, const on_ZwTerminateProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwTerminateProcess@8", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 0);
        const auto ExitStatus    = arg<nt32::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[359]);

        on_func(ProcessHandle, ExitStatus);
    });
}

opt<id_t> nt32::syscalls32::register_ZwTerminateThread(proc_t proc, const on_ZwTerminateThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwTerminateThread@8", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle = arg<nt32::HANDLE>(core, 0);
        const auto ExitStatus   = arg<nt32::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[360]);

        on_func(ThreadHandle, ExitStatus);
    });
}

opt<id_t> nt32::syscalls32::register_ZwTraceControl(proc_t proc, const on_ZwTraceControl_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwTraceControl@24", [=]
    {
        auto& core = d_->core;
        
        const auto FunctionCode = arg<nt32::ULONG>(core, 0);
        const auto InBuffer     = arg<nt32::PVOID>(core, 1);
        const auto InBufferLen  = arg<nt32::ULONG>(core, 2);
        const auto OutBuffer    = arg<nt32::PVOID>(core, 3);
        const auto OutBufferLen = arg<nt32::ULONG>(core, 4);
        const auto ReturnLength = arg<nt32::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[361]);

        on_func(FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength);
    });
}

opt<id_t> nt32::syscalls32::register_NtTraceEvent(proc_t proc, const on_NtTraceEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtTraceEvent@16", [=]
    {
        auto& core = d_->core;
        
        const auto TraceHandle = arg<nt32::HANDLE>(core, 0);
        const auto Flags       = arg<nt32::ULONG>(core, 1);
        const auto FieldSize   = arg<nt32::ULONG>(core, 2);
        const auto Fields      = arg<nt32::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[362]);

        on_func(TraceHandle, Flags, FieldSize, Fields);
    });
}

opt<id_t> nt32::syscalls32::register_NtTranslateFilePath(proc_t proc, const on_NtTranslateFilePath_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtTranslateFilePath@16", [=]
    {
        auto& core = d_->core;
        
        const auto InputFilePath        = arg<nt32::PFILE_PATH>(core, 0);
        const auto OutputType           = arg<nt32::ULONG>(core, 1);
        const auto OutputFilePath       = arg<nt32::PFILE_PATH>(core, 2);
        const auto OutputFilePathLength = arg<nt32::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[363]);

        on_func(InputFilePath, OutputType, OutputFilePath, OutputFilePathLength);
    });
}

opt<id_t> nt32::syscalls32::register_ZwUnloadDriver(proc_t proc, const on_ZwUnloadDriver_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwUnloadDriver@4", [=]
    {
        auto& core = d_->core;
        
        const auto DriverServiceName = arg<nt32::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[364]);

        on_func(DriverServiceName);
    });
}

opt<id_t> nt32::syscalls32::register_ZwUnloadKey2(proc_t proc, const on_ZwUnloadKey2_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwUnloadKey2@8", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);
        const auto Flags     = arg<nt32::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[365]);

        on_func(TargetKey, Flags);
    });
}

opt<id_t> nt32::syscalls32::register_ZwUnloadKeyEx(proc_t proc, const on_ZwUnloadKeyEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwUnloadKeyEx@8", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);
        const auto Event     = arg<nt32::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[366]);

        on_func(TargetKey, Event);
    });
}

opt<id_t> nt32::syscalls32::register_NtUnloadKey(proc_t proc, const on_NtUnloadKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtUnloadKey@4", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey = arg<nt32::POBJECT_ATTRIBUTES>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[367]);

        on_func(TargetKey);
    });
}

opt<id_t> nt32::syscalls32::register_ZwUnlockFile(proc_t proc, const on_ZwUnlockFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwUnlockFile@20", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt32::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(core, 1);
        const auto ByteOffset    = arg<nt32::PLARGE_INTEGER>(core, 2);
        const auto Length        = arg<nt32::PLARGE_INTEGER>(core, 3);
        const auto Key           = arg<nt32::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[368]);

        on_func(FileHandle, IoStatusBlock, ByteOffset, Length, Key);
    });
}

opt<id_t> nt32::syscalls32::register_NtUnlockVirtualMemory(proc_t proc, const on_NtUnlockVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtUnlockVirtualMemory@16", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt32::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt32::PVOID>(core, 1);
        const auto RegionSize      = arg<nt32::PSIZE_T>(core, 2);
        const auto MapType         = arg<nt32::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[369]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    });
}

opt<id_t> nt32::syscalls32::register_NtUnmapViewOfSection(proc_t proc, const on_NtUnmapViewOfSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtUnmapViewOfSection@8", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt32::HANDLE>(core, 0);
        const auto BaseAddress   = arg<nt32::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[370]);

        on_func(ProcessHandle, BaseAddress);
    });
}

opt<id_t> nt32::syscalls32::register_NtVdmControl(proc_t proc, const on_NtVdmControl_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtVdmControl@8", [=]
    {
        auto& core = d_->core;
        
        const auto Service     = arg<nt32::VDMSERVICECLASS>(core, 0);
        const auto ServiceData = arg<nt32::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[371]);

        on_func(Service, ServiceData);
    });
}

opt<id_t> nt32::syscalls32::register_NtWaitForDebugEvent(proc_t proc, const on_NtWaitForDebugEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWaitForDebugEvent@16", [=]
    {
        auto& core = d_->core;
        
        const auto DebugObjectHandle = arg<nt32::HANDLE>(core, 0);
        const auto Alertable         = arg<nt32::BOOLEAN>(core, 1);
        const auto Timeout           = arg<nt32::PLARGE_INTEGER>(core, 2);
        const auto WaitStateChange   = arg<nt32::PDBGUI_WAIT_STATE_CHANGE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[372]);

        on_func(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
    });
}

opt<id_t> nt32::syscalls32::register_NtWaitForKeyedEvent(proc_t proc, const on_NtWaitForKeyedEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWaitForKeyedEvent@16", [=]
    {
        auto& core = d_->core;
        
        const auto KeyedEventHandle = arg<nt32::HANDLE>(core, 0);
        const auto KeyValue         = arg<nt32::PVOID>(core, 1);
        const auto Alertable        = arg<nt32::BOOLEAN>(core, 2);
        const auto Timeout          = arg<nt32::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[373]);

        on_func(KeyedEventHandle, KeyValue, Alertable, Timeout);
    });
}

opt<id_t> nt32::syscalls32::register_NtWaitForMultipleObjects32(proc_t proc, const on_NtWaitForMultipleObjects32_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWaitForMultipleObjects32@20", [=]
    {
        auto& core = d_->core;
        
        const auto Count     = arg<nt32::ULONG>(core, 0);
        const auto Handles   = arg<nt32::LONG>(core, 1);
        const auto WaitType  = arg<nt32::WAIT_TYPE>(core, 2);
        const auto Alertable = arg<nt32::BOOLEAN>(core, 3);
        const auto Timeout   = arg<nt32::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[374]);

        on_func(Count, Handles, WaitType, Alertable, Timeout);
    });
}

opt<id_t> nt32::syscalls32::register_NtWaitForMultipleObjects(proc_t proc, const on_NtWaitForMultipleObjects_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWaitForMultipleObjects@20", [=]
    {
        auto& core = d_->core;
        
        const auto Count     = arg<nt32::ULONG>(core, 0);
        const auto Handles   = arg<nt32::HANDLE>(core, 1);
        const auto WaitType  = arg<nt32::WAIT_TYPE>(core, 2);
        const auto Alertable = arg<nt32::BOOLEAN>(core, 3);
        const auto Timeout   = arg<nt32::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[375]);

        on_func(Count, Handles, WaitType, Alertable, Timeout);
    });
}

opt<id_t> nt32::syscalls32::register_ZwWaitForSingleObject(proc_t proc, const on_ZwWaitForSingleObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwWaitForSingleObject@12", [=]
    {
        auto& core = d_->core;
        
        const auto Handle    = arg<nt32::HANDLE>(core, 0);
        const auto Alertable = arg<nt32::BOOLEAN>(core, 1);
        const auto Timeout   = arg<nt32::PLARGE_INTEGER>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[376]);

        on_func(Handle, Alertable, Timeout);
    });
}

opt<id_t> nt32::syscalls32::register_NtWaitForWorkViaWorkerFactory(proc_t proc, const on_NtWaitForWorkViaWorkerFactory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWaitForWorkViaWorkerFactory@20", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle = arg<nt32::HANDLE>(core, 0);
        const auto MiniPacket          = arg<nt32::PFILE_IO_COMPLETION_INFORMATION>(core, 1);
        const auto Arg2                = arg<nt32::PVOID>(core, 2);
        const auto Arg3                = arg<nt32::PVOID>(core, 3);
        const auto Arg4                = arg<nt32::PVOID>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[377]);

        on_func(WorkerFactoryHandle, MiniPacket, Arg2, Arg3, Arg4);
    });
}

opt<id_t> nt32::syscalls32::register_ZwWaitHighEventPair(proc_t proc, const on_ZwWaitHighEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwWaitHighEventPair@4", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[378]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtWaitLowEventPair(proc_t proc, const on_NtWaitLowEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWaitLowEventPair@4", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[379]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtWorkerFactoryWorkerReady(proc_t proc, const on_NtWorkerFactoryWorkerReady_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWorkerFactoryWorkerReady@4", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle = arg<nt32::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[380]);

        on_func(WorkerFactoryHandle);
    });
}

opt<id_t> nt32::syscalls32::register_NtWriteFileGather(proc_t proc, const on_NtWriteFileGather_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWriteFileGather@36", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt32::HANDLE>(core, 0);
        const auto Event         = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt32::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt32::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(core, 4);
        const auto SegmentArray  = arg<nt32::PFILE_SEGMENT_ELEMENT>(core, 5);
        const auto Length        = arg<nt32::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt32::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt32::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[381]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    });
}

opt<id_t> nt32::syscalls32::register_NtWriteFile(proc_t proc, const on_NtWriteFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWriteFile@36", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt32::HANDLE>(core, 0);
        const auto Event         = arg<nt32::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt32::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt32::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt32::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer        = arg<nt32::PVOID>(core, 5);
        const auto Length        = arg<nt32::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt32::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt32::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[382]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    });
}

opt<id_t> nt32::syscalls32::register_NtWriteRequestData(proc_t proc, const on_NtWriteRequestData_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWriteRequestData@24", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle           = arg<nt32::HANDLE>(core, 0);
        const auto Message              = arg<nt32::PPORT_MESSAGE>(core, 1);
        const auto DataEntryIndex       = arg<nt32::ULONG>(core, 2);
        const auto Buffer               = arg<nt32::PVOID>(core, 3);
        const auto BufferSize           = arg<nt32::SIZE_T>(core, 4);
        const auto NumberOfBytesWritten = arg<nt32::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[383]);

        on_func(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten);
    });
}

opt<id_t> nt32::syscalls32::register_NtWriteVirtualMemory(proc_t proc, const on_NtWriteVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtWriteVirtualMemory@20", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle        = arg<nt32::HANDLE>(core, 0);
        const auto BaseAddress          = arg<nt32::PVOID>(core, 1);
        const auto Buffer               = arg<nt32::PVOID>(core, 2);
        const auto BufferSize           = arg<nt32::SIZE_T>(core, 3);
        const auto NumberOfBytesWritten = arg<nt32::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[384]);

        on_func(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);
    });
}

opt<id_t> nt32::syscalls32::register_NtDisableLastKnownGood(proc_t proc, const on_NtDisableLastKnownGood_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtDisableLastKnownGood@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[385]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_NtEnableLastKnownGood(proc_t proc, const on_NtEnableLastKnownGood_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtEnableLastKnownGood@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[386]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_ZwFlushProcessWriteBuffers(proc_t proc, const on_ZwFlushProcessWriteBuffers_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwFlushProcessWriteBuffers@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[387]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_NtFlushWriteBuffer(proc_t proc, const on_NtFlushWriteBuffer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtFlushWriteBuffer@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[388]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_NtGetCurrentProcessorNumber(proc_t proc, const on_NtGetCurrentProcessorNumber_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtGetCurrentProcessorNumber@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[389]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_NtIsSystemResumeAutomatic(proc_t proc, const on_NtIsSystemResumeAutomatic_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtIsSystemResumeAutomatic@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[390]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_ZwIsUILanguageComitted(proc_t proc, const on_ZwIsUILanguageComitted_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwIsUILanguageComitted@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[391]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_ZwQueryPortInformationProcess(proc_t proc, const on_ZwQueryPortInformationProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwQueryPortInformationProcess@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[392]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_NtSerializeBoot(proc_t proc, const on_NtSerializeBoot_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtSerializeBoot@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[393]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_NtTestAlert(proc_t proc, const on_NtTestAlert_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtTestAlert@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[394]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_ZwThawRegistry(proc_t proc, const on_ZwThawRegistry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwThawRegistry@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[395]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_NtThawTransactions(proc_t proc, const on_NtThawTransactions_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_NtThawTransactions@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[396]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_ZwUmsThreadYield(proc_t proc, const on_ZwUmsThreadYield_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwUmsThreadYield@4", [=]
    {
        auto& core = d_->core;
        
        const auto SchedulerParam = arg<nt32::PVOID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[397]);

        on_func(SchedulerParam);
    });
}

opt<id_t> nt32::syscalls32::register_ZwYieldExecution(proc_t proc, const on_ZwYieldExecution_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "_ZwYieldExecution@0", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[398]);

        on_func();
    });
}

opt<id_t> nt32::syscalls32::register_all(proc_t proc, const nt32::syscalls32::on_call_fn& on_call)
{
    const auto id   = ++d_->last_id;
    const auto size = d_->listeners.size();
    for(const auto cfg : g_callcfgs)
        register_callback(*d_, id, proc, cfg.name, [=]{ on_call(cfg); });

    if(size == d_->listeners.size())
        return {};

    return id;
}

bool nt32::syscalls32::unregister(id_t id)
{
    return d_->listeners.erase(id) > 0;
}
