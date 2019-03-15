#include "syscalls.gen.hpp"

#define FDP_MODULE "syscalls"
#include "log.hpp"
#include "os.hpp"

#include <map>

namespace
{
	constexpr bool g_debug = false;

	static const tracer::callcfg_t g_callcfgs[] =
	{
        {"NtAcceptConnectPort", 6, {{"PHANDLE", "PortHandle", sizeof(nt64::PHANDLE)}, {"PVOID", "PortContext", sizeof(nt64::PVOID)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(nt64::PPORT_MESSAGE)}, {"BOOLEAN", "AcceptConnection", sizeof(nt64::BOOLEAN)}, {"PPORT_VIEW", "ServerView", sizeof(nt64::PPORT_VIEW)}, {"PREMOTE_PORT_VIEW", "ClientView", sizeof(nt64::PREMOTE_PORT_VIEW)}}},
        {"NtAccessCheckAndAuditAlarm", 11, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt64::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt64::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt64::PSECURITY_DESCRIPTOR)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt64::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt64::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt64::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt64::PBOOLEAN)}}},
        {"NtAccessCheckByTypeAndAuditAlarm", 16, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt64::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt64::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt64::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt64::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(nt64::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt64::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt64::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt64::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt64::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt64::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt64::PBOOLEAN)}}},
        {"NtAccessCheckByType", 11, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt64::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt64::PSID)}, {"HANDLE", "ClientToken", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt64::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt64::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt64::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(nt64::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(nt64::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt64::PNTSTATUS)}}},
        {"NtAccessCheckByTypeResultListAndAuditAlarmByHandle", 17, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt64::PVOID)}, {"HANDLE", "ClientToken", sizeof(nt64::HANDLE)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt64::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt64::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt64::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(nt64::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt64::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt64::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt64::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt64::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt64::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt64::PBOOLEAN)}}},
        {"NtAccessCheckByTypeResultListAndAuditAlarm", 16, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt64::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt64::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt64::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt64::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(nt64::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt64::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt64::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt64::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt64::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt64::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt64::PBOOLEAN)}}},
        {"NtAccessCheckByTypeResultList", 11, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt64::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt64::PSID)}, {"HANDLE", "ClientToken", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt64::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt64::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt64::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(nt64::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(nt64::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt64::PNTSTATUS)}}},
        {"NtAccessCheck", 8, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt64::PSECURITY_DESCRIPTOR)}, {"HANDLE", "ClientToken", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt64::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(nt64::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(nt64::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt64::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt64::PNTSTATUS)}}},
        {"NtAddAtom", 3, {{"PWSTR", "AtomName", sizeof(nt64::PWSTR)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PRTL_ATOM", "Atom", sizeof(nt64::PRTL_ATOM)}}},
        {"NtAddBootEntry", 2, {{"PBOOT_ENTRY", "BootEntry", sizeof(nt64::PBOOT_ENTRY)}, {"PULONG", "Id", sizeof(nt64::PULONG)}}},
        {"NtAddDriverEntry", 2, {{"PEFI_DRIVER_ENTRY", "DriverEntry", sizeof(nt64::PEFI_DRIVER_ENTRY)}, {"PULONG", "Id", sizeof(nt64::PULONG)}}},
        {"NtAdjustGroupsToken", 6, {{"HANDLE", "TokenHandle", sizeof(nt64::HANDLE)}, {"BOOLEAN", "ResetToDefault", sizeof(nt64::BOOLEAN)}, {"PTOKEN_GROUPS", "NewState", sizeof(nt64::PTOKEN_GROUPS)}, {"ULONG", "BufferLength", sizeof(nt64::ULONG)}, {"PTOKEN_GROUPS", "PreviousState", sizeof(nt64::PTOKEN_GROUPS)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtAdjustPrivilegesToken", 6, {{"HANDLE", "TokenHandle", sizeof(nt64::HANDLE)}, {"BOOLEAN", "DisableAllPrivileges", sizeof(nt64::BOOLEAN)}, {"PTOKEN_PRIVILEGES", "NewState", sizeof(nt64::PTOKEN_PRIVILEGES)}, {"ULONG", "BufferLength", sizeof(nt64::ULONG)}, {"PTOKEN_PRIVILEGES", "PreviousState", sizeof(nt64::PTOKEN_PRIVILEGES)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtAlertResumeThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(nt64::PULONG)}}},
        {"NtAlertThread", 1, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}}},
        {"NtAllocateLocallyUniqueId", 1, {{"PLUID", "Luid", sizeof(nt64::PLUID)}}},
        {"NtAllocateReserveObject", 3, {{"PHANDLE", "MemoryReserveHandle", sizeof(nt64::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"MEMORY_RESERVE_TYPE", "Type", sizeof(nt64::MEMORY_RESERVE_TYPE)}}},
        {"NtAllocateUserPhysicalPages", 3, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PULONG_PTR", "NumberOfPages", sizeof(nt64::PULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(nt64::PULONG_PTR)}}},
        {"NtAllocateUuids", 4, {{"PULARGE_INTEGER", "Time", sizeof(nt64::PULARGE_INTEGER)}, {"PULONG", "Range", sizeof(nt64::PULONG)}, {"PULONG", "Sequence", sizeof(nt64::PULONG)}, {"PCHAR", "Seed", sizeof(nt64::PCHAR)}}},
        {"NtAllocateVirtualMemory", 6, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt64::PVOID)}, {"ULONG_PTR", "ZeroBits", sizeof(nt64::ULONG_PTR)}, {"PSIZE_T", "RegionSize", sizeof(nt64::PSIZE_T)}, {"ULONG", "AllocationType", sizeof(nt64::ULONG)}, {"ULONG", "Protect", sizeof(nt64::ULONG)}}},
        {"NtAlpcAcceptConnectPort", 9, {{"PHANDLE", "PortHandle", sizeof(nt64::PHANDLE)}, {"HANDLE", "ConnectionPortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(nt64::PALPC_PORT_ATTRIBUTES)}, {"PVOID", "PortContext", sizeof(nt64::PVOID)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(nt64::PPORT_MESSAGE)}, {"PALPC_MESSAGE_ATTRIBUTES", "ConnectionMessageAttributes", sizeof(nt64::PALPC_MESSAGE_ATTRIBUTES)}, {"BOOLEAN", "AcceptConnection", sizeof(nt64::BOOLEAN)}}},
        {"NtAlpcCancelMessage", 3, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PALPC_CONTEXT_ATTR", "MessageContext", sizeof(nt64::PALPC_CONTEXT_ATTR)}}},
        {"NtAlpcConnectPort", 11, {{"PHANDLE", "PortHandle", sizeof(nt64::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(nt64::PUNICODE_STRING)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(nt64::PALPC_PORT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PSID", "RequiredServerSid", sizeof(nt64::PSID)}, {"PPORT_MESSAGE", "ConnectionMessage", sizeof(nt64::PPORT_MESSAGE)}, {"PULONG", "BufferLength", sizeof(nt64::PULONG)}, {"PALPC_MESSAGE_ATTRIBUTES", "OutMessageAttributes", sizeof(nt64::PALPC_MESSAGE_ATTRIBUTES)}, {"PALPC_MESSAGE_ATTRIBUTES", "InMessageAttributes", sizeof(nt64::PALPC_MESSAGE_ATTRIBUTES)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtAlpcCreatePort", 3, {{"PHANDLE", "PortHandle", sizeof(nt64::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(nt64::PALPC_PORT_ATTRIBUTES)}}},
        {"NtAlpcCreatePortSection", 6, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"HANDLE", "SectionHandle", sizeof(nt64::HANDLE)}, {"SIZE_T", "SectionSize", sizeof(nt64::SIZE_T)}, {"PALPC_HANDLE", "AlpcSectionHandle", sizeof(nt64::PALPC_HANDLE)}, {"PSIZE_T", "ActualSectionSize", sizeof(nt64::PSIZE_T)}}},
        {"NtAlpcCreateResourceReserve", 4, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"SIZE_T", "MessageSize", sizeof(nt64::SIZE_T)}, {"PALPC_HANDLE", "ResourceId", sizeof(nt64::PALPC_HANDLE)}}},
        {"NtAlpcCreateSectionView", 3, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PALPC_DATA_VIEW_ATTR", "ViewAttributes", sizeof(nt64::PALPC_DATA_VIEW_ATTR)}}},
        {"NtAlpcCreateSecurityContext", 3, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PALPC_SECURITY_ATTR", "SecurityAttribute", sizeof(nt64::PALPC_SECURITY_ATTR)}}},
        {"NtAlpcDeletePortSection", 3, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"ALPC_HANDLE", "SectionHandle", sizeof(nt64::ALPC_HANDLE)}}},
        {"NtAlpcDeleteResourceReserve", 3, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"ALPC_HANDLE", "ResourceId", sizeof(nt64::ALPC_HANDLE)}}},
        {"NtAlpcDeleteSectionView", 3, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PVOID", "ViewBase", sizeof(nt64::PVOID)}}},
        {"NtAlpcDeleteSecurityContext", 3, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"ALPC_HANDLE", "ContextHandle", sizeof(nt64::ALPC_HANDLE)}}},
        {"NtAlpcDisconnectPort", 2, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}}},
        {"NtAlpcImpersonateClientOfPort", 3, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt64::PPORT_MESSAGE)}, {"PVOID", "Reserved", sizeof(nt64::PVOID)}}},
        {"NtAlpcOpenSenderProcess", 6, {{"PHANDLE", "ProcessHandle", sizeof(nt64::PHANDLE)}, {"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt64::PPORT_MESSAGE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtAlpcOpenSenderThread", 6, {{"PHANDLE", "ThreadHandle", sizeof(nt64::PHANDLE)}, {"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt64::PPORT_MESSAGE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtAlpcQueryInformation", 5, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ALPC_PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(nt64::ALPC_PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtAlpcQueryInformationMessage", 6, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt64::PPORT_MESSAGE)}, {"ALPC_MESSAGE_INFORMATION_CLASS", "MessageInformationClass", sizeof(nt64::ALPC_MESSAGE_INFORMATION_CLASS)}, {"PVOID", "MessageInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtAlpcRevokeSecurityContext", 3, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"ALPC_HANDLE", "ContextHandle", sizeof(nt64::ALPC_HANDLE)}}},
        {"NtAlpcSendWaitReceivePort", 8, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PPORT_MESSAGE", "SendMessage", sizeof(nt64::PPORT_MESSAGE)}, {"PALPC_MESSAGE_ATTRIBUTES", "SendMessageAttributes", sizeof(nt64::PALPC_MESSAGE_ATTRIBUTES)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(nt64::PPORT_MESSAGE)}, {"PULONG", "BufferLength", sizeof(nt64::PULONG)}, {"PALPC_MESSAGE_ATTRIBUTES", "ReceiveMessageAttributes", sizeof(nt64::PALPC_MESSAGE_ATTRIBUTES)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtAlpcSetInformation", 4, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"ALPC_PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(nt64::ALPC_PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}}},
        {"NtApphelpCacheControl", 2, {{"APPHELPCOMMAND", "type", sizeof(nt64::APPHELPCOMMAND)}, {"PVOID", "buf", sizeof(nt64::PVOID)}}},
        {"NtAreMappedFilesTheSame", 2, {{"PVOID", "File1MappedAsAnImage", sizeof(nt64::PVOID)}, {"PVOID", "File2MappedAsFile", sizeof(nt64::PVOID)}}},
        {"NtAssignProcessToJobObject", 2, {{"HANDLE", "JobHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}}},
        {"NtCancelIoFileEx", 3, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoRequestToCancel", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}}},
        {"NtCancelIoFile", 2, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}}},
        {"NtCancelSynchronousIoFile", 3, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoRequestToCancel", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}}},
        {"NtCancelTimer", 2, {{"HANDLE", "TimerHandle", sizeof(nt64::HANDLE)}, {"PBOOLEAN", "CurrentState", sizeof(nt64::PBOOLEAN)}}},
        {"NtClearEvent", 1, {{"HANDLE", "EventHandle", sizeof(nt64::HANDLE)}}},
        {"NtClose", 1, {{"HANDLE", "Handle", sizeof(nt64::HANDLE)}}},
        {"NtCloseObjectAuditAlarm", 3, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt64::PVOID)}, {"BOOLEAN", "GenerateOnClose", sizeof(nt64::BOOLEAN)}}},
        {"NtCommitComplete", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtCommitEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtCommitTransaction", 2, {{"HANDLE", "TransactionHandle", sizeof(nt64::HANDLE)}, {"BOOLEAN", "Wait", sizeof(nt64::BOOLEAN)}}},
        {"NtCompactKeys", 2, {{"ULONG", "Count", sizeof(nt64::ULONG)}, {"HANDLE", "KeyArray", sizeof(nt64::HANDLE)}}},
        {"NtCompareTokens", 3, {{"HANDLE", "FirstTokenHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "SecondTokenHandle", sizeof(nt64::HANDLE)}, {"PBOOLEAN", "Equal", sizeof(nt64::PBOOLEAN)}}},
        {"NtCompleteConnectPort", 1, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}}},
        {"NtCompressKey", 1, {{"HANDLE", "Key", sizeof(nt64::HANDLE)}}},
        {"NtConnectPort", 8, {{"PHANDLE", "PortHandle", sizeof(nt64::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(nt64::PUNICODE_STRING)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(nt64::PSECURITY_QUALITY_OF_SERVICE)}, {"PPORT_VIEW", "ClientView", sizeof(nt64::PPORT_VIEW)}, {"PREMOTE_PORT_VIEW", "ServerView", sizeof(nt64::PREMOTE_PORT_VIEW)}, {"PULONG", "MaxMessageLength", sizeof(nt64::PULONG)}, {"PVOID", "ConnectionInformation", sizeof(nt64::PVOID)}, {"PULONG", "ConnectionInformationLength", sizeof(nt64::PULONG)}}},
        {"NtContinue", 2, {{"PCONTEXT", "ContextRecord", sizeof(nt64::PCONTEXT)}, {"BOOLEAN", "TestAlert", sizeof(nt64::BOOLEAN)}}},
        {"NtCreateDebugObject", 4, {{"PHANDLE", "DebugObjectHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}}},
        {"NtCreateDirectoryObject", 3, {{"PHANDLE", "DirectoryHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtCreateEnlistment", 8, {{"PHANDLE", "EnlistmentHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"HANDLE", "ResourceManagerHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "TransactionHandle", sizeof(nt64::HANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "CreateOptions", sizeof(nt64::ULONG)}, {"NOTIFICATION_MASK", "NotificationMask", sizeof(nt64::NOTIFICATION_MASK)}, {"PVOID", "EnlistmentKey", sizeof(nt64::PVOID)}}},
        {"NtCreateEvent", 5, {{"PHANDLE", "EventHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"EVENT_TYPE", "EventType", sizeof(nt64::EVENT_TYPE)}, {"BOOLEAN", "InitialState", sizeof(nt64::BOOLEAN)}}},
        {"NtCreateEventPair", 3, {{"PHANDLE", "EventPairHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtCreateFile", 11, {{"PHANDLE", "FileHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "AllocationSize", sizeof(nt64::PLARGE_INTEGER)}, {"ULONG", "FileAttributes", sizeof(nt64::ULONG)}, {"ULONG", "ShareAccess", sizeof(nt64::ULONG)}, {"ULONG", "CreateDisposition", sizeof(nt64::ULONG)}, {"ULONG", "CreateOptions", sizeof(nt64::ULONG)}, {"PVOID", "EaBuffer", sizeof(nt64::PVOID)}, {"ULONG", "EaLength", sizeof(nt64::ULONG)}}},
        {"NtCreateIoCompletion", 4, {{"PHANDLE", "IoCompletionHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "Count", sizeof(nt64::ULONG)}}},
        {"NtCreateJobObject", 3, {{"PHANDLE", "JobHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtCreateJobSet", 3, {{"ULONG", "NumJob", sizeof(nt64::ULONG)}, {"PJOB_SET_ARRAY", "UserJobSet", sizeof(nt64::PJOB_SET_ARRAY)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}}},
        {"NtCreateKeyedEvent", 4, {{"PHANDLE", "KeyedEventHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}}},
        {"NtCreateKey", 7, {{"PHANDLE", "KeyHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "TitleIndex", sizeof(nt64::ULONG)}, {"PUNICODE_STRING", "Class", sizeof(nt64::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(nt64::ULONG)}, {"PULONG", "Disposition", sizeof(nt64::PULONG)}}},
        {"NtCreateKeyTransacted", 8, {{"PHANDLE", "KeyHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "TitleIndex", sizeof(nt64::ULONG)}, {"PUNICODE_STRING", "Class", sizeof(nt64::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(nt64::ULONG)}, {"HANDLE", "TransactionHandle", sizeof(nt64::HANDLE)}, {"PULONG", "Disposition", sizeof(nt64::PULONG)}}},
        {"NtCreateMailslotFile", 8, {{"PHANDLE", "FileHandle", sizeof(nt64::PHANDLE)}, {"ULONG", "DesiredAccess", sizeof(nt64::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"ULONG", "CreateOptions", sizeof(nt64::ULONG)}, {"ULONG", "MailslotQuota", sizeof(nt64::ULONG)}, {"ULONG", "MaximumMessageSize", sizeof(nt64::ULONG)}, {"PLARGE_INTEGER", "ReadTimeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtCreateMutant", 4, {{"PHANDLE", "MutantHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"BOOLEAN", "InitialOwner", sizeof(nt64::BOOLEAN)}}},
        {"NtCreateNamedPipeFile", 14, {{"PHANDLE", "FileHandle", sizeof(nt64::PHANDLE)}, {"ULONG", "DesiredAccess", sizeof(nt64::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"ULONG", "ShareAccess", sizeof(nt64::ULONG)}, {"ULONG", "CreateDisposition", sizeof(nt64::ULONG)}, {"ULONG", "CreateOptions", sizeof(nt64::ULONG)}, {"ULONG", "NamedPipeType", sizeof(nt64::ULONG)}, {"ULONG", "ReadMode", sizeof(nt64::ULONG)}, {"ULONG", "CompletionMode", sizeof(nt64::ULONG)}, {"ULONG", "MaximumInstances", sizeof(nt64::ULONG)}, {"ULONG", "InboundQuota", sizeof(nt64::ULONG)}, {"ULONG", "OutboundQuota", sizeof(nt64::ULONG)}, {"PLARGE_INTEGER", "DefaultTimeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtCreatePagingFile", 4, {{"PUNICODE_STRING", "PageFileName", sizeof(nt64::PUNICODE_STRING)}, {"PLARGE_INTEGER", "MinimumSize", sizeof(nt64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "MaximumSize", sizeof(nt64::PLARGE_INTEGER)}, {"ULONG", "Priority", sizeof(nt64::ULONG)}}},
        {"NtCreatePort", 5, {{"PHANDLE", "PortHandle", sizeof(nt64::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "MaxConnectionInfoLength", sizeof(nt64::ULONG)}, {"ULONG", "MaxMessageLength", sizeof(nt64::ULONG)}, {"ULONG", "MaxPoolUsage", sizeof(nt64::ULONG)}}},
        {"NtCreatePrivateNamespace", 4, {{"PHANDLE", "NamespaceHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PVOID", "BoundaryDescriptor", sizeof(nt64::PVOID)}}},
        {"NtCreateProcessEx", 9, {{"PHANDLE", "ProcessHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"HANDLE", "ParentProcess", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"HANDLE", "SectionHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "DebugPort", sizeof(nt64::HANDLE)}, {"HANDLE", "ExceptionPort", sizeof(nt64::HANDLE)}, {"ULONG", "JobMemberLevel", sizeof(nt64::ULONG)}}},
        {"NtCreateProcess", 8, {{"PHANDLE", "ProcessHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"HANDLE", "ParentProcess", sizeof(nt64::HANDLE)}, {"BOOLEAN", "InheritObjectTable", sizeof(nt64::BOOLEAN)}, {"HANDLE", "SectionHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "DebugPort", sizeof(nt64::HANDLE)}, {"HANDLE", "ExceptionPort", sizeof(nt64::HANDLE)}}},
        {"NtCreateProfileEx", 10, {{"PHANDLE", "ProfileHandle", sizeof(nt64::PHANDLE)}, {"HANDLE", "Process", sizeof(nt64::HANDLE)}, {"PVOID", "ProfileBase", sizeof(nt64::PVOID)}, {"SIZE_T", "ProfileSize", sizeof(nt64::SIZE_T)}, {"ULONG", "BucketSize", sizeof(nt64::ULONG)}, {"PULONG", "Buffer", sizeof(nt64::PULONG)}, {"ULONG", "BufferSize", sizeof(nt64::ULONG)}, {"KPROFILE_SOURCE", "ProfileSource", sizeof(nt64::KPROFILE_SOURCE)}, {"ULONG", "GroupAffinityCount", sizeof(nt64::ULONG)}, {"PGROUP_AFFINITY", "GroupAffinity", sizeof(nt64::PGROUP_AFFINITY)}}},
        {"NtCreateProfile", 9, {{"PHANDLE", "ProfileHandle", sizeof(nt64::PHANDLE)}, {"HANDLE", "Process", sizeof(nt64::HANDLE)}, {"PVOID", "RangeBase", sizeof(nt64::PVOID)}, {"SIZE_T", "RangeSize", sizeof(nt64::SIZE_T)}, {"ULONG", "BucketSize", sizeof(nt64::ULONG)}, {"PULONG", "Buffer", sizeof(nt64::PULONG)}, {"ULONG", "BufferSize", sizeof(nt64::ULONG)}, {"KPROFILE_SOURCE", "ProfileSource", sizeof(nt64::KPROFILE_SOURCE)}, {"KAFFINITY", "Affinity", sizeof(nt64::KAFFINITY)}}},
        {"NtCreateResourceManager", 7, {{"PHANDLE", "ResourceManagerHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"HANDLE", "TmHandle", sizeof(nt64::HANDLE)}, {"LPGUID", "RmGuid", sizeof(nt64::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "CreateOptions", sizeof(nt64::ULONG)}, {"PUNICODE_STRING", "Description", sizeof(nt64::PUNICODE_STRING)}}},
        {"NtCreateSection", 7, {{"PHANDLE", "SectionHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PLARGE_INTEGER", "MaximumSize", sizeof(nt64::PLARGE_INTEGER)}, {"ULONG", "SectionPageProtection", sizeof(nt64::ULONG)}, {"ULONG", "AllocationAttributes", sizeof(nt64::ULONG)}, {"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}}},
        {"NtCreateSemaphore", 5, {{"PHANDLE", "SemaphoreHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"LONG", "InitialCount", sizeof(nt64::LONG)}, {"LONG", "MaximumCount", sizeof(nt64::LONG)}}},
        {"NtCreateSymbolicLinkObject", 4, {{"PHANDLE", "LinkHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LinkTarget", sizeof(nt64::PUNICODE_STRING)}}},
        {"NtCreateThreadEx", 11, {{"PHANDLE", "ThreadHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "StartRoutine", sizeof(nt64::PVOID)}, {"PVOID", "Argument", sizeof(nt64::PVOID)}, {"ULONG", "CreateFlags", sizeof(nt64::ULONG)}, {"ULONG_PTR", "ZeroBits", sizeof(nt64::ULONG_PTR)}, {"SIZE_T", "StackSize", sizeof(nt64::SIZE_T)}, {"SIZE_T", "MaximumStackSize", sizeof(nt64::SIZE_T)}, {"PPS_ATTRIBUTE_LIST", "AttributeList", sizeof(nt64::PPS_ATTRIBUTE_LIST)}}},
        {"NtCreateThread", 8, {{"PHANDLE", "ThreadHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PCLIENT_ID", "ClientId", sizeof(nt64::PCLIENT_ID)}, {"PCONTEXT", "ThreadContext", sizeof(nt64::PCONTEXT)}, {"PINITIAL_TEB", "InitialTeb", sizeof(nt64::PINITIAL_TEB)}, {"BOOLEAN", "CreateSuspended", sizeof(nt64::BOOLEAN)}}},
        {"NtCreateTimer", 4, {{"PHANDLE", "TimerHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"TIMER_TYPE", "TimerType", sizeof(nt64::TIMER_TYPE)}}},
        {"NtCreateToken", 13, {{"PHANDLE", "TokenHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"TOKEN_TYPE", "TokenType", sizeof(nt64::TOKEN_TYPE)}, {"PLUID", "AuthenticationId", sizeof(nt64::PLUID)}, {"PLARGE_INTEGER", "ExpirationTime", sizeof(nt64::PLARGE_INTEGER)}, {"PTOKEN_USER", "User", sizeof(nt64::PTOKEN_USER)}, {"PTOKEN_GROUPS", "Groups", sizeof(nt64::PTOKEN_GROUPS)}, {"PTOKEN_PRIVILEGES", "Privileges", sizeof(nt64::PTOKEN_PRIVILEGES)}, {"PTOKEN_OWNER", "Owner", sizeof(nt64::PTOKEN_OWNER)}, {"PTOKEN_PRIMARY_GROUP", "PrimaryGroup", sizeof(nt64::PTOKEN_PRIMARY_GROUP)}, {"PTOKEN_DEFAULT_DACL", "DefaultDacl", sizeof(nt64::PTOKEN_DEFAULT_DACL)}, {"PTOKEN_SOURCE", "TokenSource", sizeof(nt64::PTOKEN_SOURCE)}}},
        {"NtCreateTransactionManager", 6, {{"PHANDLE", "TmHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LogFileName", sizeof(nt64::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(nt64::ULONG)}, {"ULONG", "CommitStrength", sizeof(nt64::ULONG)}}},
        {"NtCreateTransaction", 10, {{"PHANDLE", "TransactionHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"LPGUID", "Uow", sizeof(nt64::LPGUID)}, {"HANDLE", "TmHandle", sizeof(nt64::HANDLE)}, {"ULONG", "CreateOptions", sizeof(nt64::ULONG)}, {"ULONG", "IsolationLevel", sizeof(nt64::ULONG)}, {"ULONG", "IsolationFlags", sizeof(nt64::ULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}, {"PUNICODE_STRING", "Description", sizeof(nt64::PUNICODE_STRING)}}},
        {"NtCreateUserProcess", 11, {{"PHANDLE", "ProcessHandle", sizeof(nt64::PHANDLE)}, {"PHANDLE", "ThreadHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "ProcessDesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"ACCESS_MASK", "ThreadDesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ProcessObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "ThreadObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "ProcessFlags", sizeof(nt64::ULONG)}, {"ULONG", "ThreadFlags", sizeof(nt64::ULONG)}, {"PRTL_USER_PROCESS_PARAMETERS", "ProcessParameters", sizeof(nt64::PRTL_USER_PROCESS_PARAMETERS)}, {"PPROCESS_CREATE_INFO", "CreateInfo", sizeof(nt64::PPROCESS_CREATE_INFO)}, {"PPROCESS_ATTRIBUTE_LIST", "AttributeList", sizeof(nt64::PPROCESS_ATTRIBUTE_LIST)}}},
        {"NtCreateWaitablePort", 5, {{"PHANDLE", "PortHandle", sizeof(nt64::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "MaxConnectionInfoLength", sizeof(nt64::ULONG)}, {"ULONG", "MaxMessageLength", sizeof(nt64::ULONG)}, {"ULONG", "MaxPoolUsage", sizeof(nt64::ULONG)}}},
        {"NtCreateWorkerFactory", 10, {{"PHANDLE", "WorkerFactoryHandleReturn", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"HANDLE", "CompletionPortHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "WorkerProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "StartRoutine", sizeof(nt64::PVOID)}, {"PVOID", "StartParameter", sizeof(nt64::PVOID)}, {"ULONG", "MaxThreadCount", sizeof(nt64::ULONG)}, {"SIZE_T", "StackReserve", sizeof(nt64::SIZE_T)}, {"SIZE_T", "StackCommit", sizeof(nt64::SIZE_T)}}},
        {"NtDebugActiveProcess", 2, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "DebugObjectHandle", sizeof(nt64::HANDLE)}}},
        {"NtDebugContinue", 3, {{"HANDLE", "DebugObjectHandle", sizeof(nt64::HANDLE)}, {"PCLIENT_ID", "ClientId", sizeof(nt64::PCLIENT_ID)}, {"NTSTATUS", "ContinueStatus", sizeof(nt64::NTSTATUS)}}},
        {"NtDelayExecution", 2, {{"BOOLEAN", "Alertable", sizeof(nt64::BOOLEAN)}, {"PLARGE_INTEGER", "DelayInterval", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtDeleteAtom", 1, {{"RTL_ATOM", "Atom", sizeof(nt64::RTL_ATOM)}}},
        {"NtDeleteBootEntry", 1, {{"ULONG", "Id", sizeof(nt64::ULONG)}}},
        {"NtDeleteDriverEntry", 1, {{"ULONG", "Id", sizeof(nt64::ULONG)}}},
        {"NtDeleteFile", 1, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtDeleteKey", 1, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}}},
        {"NtDeleteObjectAuditAlarm", 3, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt64::PVOID)}, {"BOOLEAN", "GenerateOnClose", sizeof(nt64::BOOLEAN)}}},
        {"NtDeletePrivateNamespace", 1, {{"HANDLE", "NamespaceHandle", sizeof(nt64::HANDLE)}}},
        {"NtDeleteValueKey", 2, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(nt64::PUNICODE_STRING)}}},
        {"NtDeviceIoControlFile", 10, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"ULONG", "IoControlCode", sizeof(nt64::ULONG)}, {"PVOID", "InputBuffer", sizeof(nt64::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt64::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt64::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt64::ULONG)}}},
        {"NtDisplayString", 1, {{"PUNICODE_STRING", "String", sizeof(nt64::PUNICODE_STRING)}}},
        {"NtDrawText", 1, {{"PUNICODE_STRING", "Text", sizeof(nt64::PUNICODE_STRING)}}},
        {"NtDuplicateObject", 7, {{"HANDLE", "SourceProcessHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "SourceHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "TargetProcessHandle", sizeof(nt64::HANDLE)}, {"PHANDLE", "TargetHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt64::ULONG)}, {"ULONG", "Options", sizeof(nt64::ULONG)}}},
        {"NtDuplicateToken", 6, {{"HANDLE", "ExistingTokenHandle", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"BOOLEAN", "EffectiveOnly", sizeof(nt64::BOOLEAN)}, {"TOKEN_TYPE", "TokenType", sizeof(nt64::TOKEN_TYPE)}, {"PHANDLE", "NewTokenHandle", sizeof(nt64::PHANDLE)}}},
        {"NtEnumerateBootEntries", 2, {{"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"PULONG", "BufferLength", sizeof(nt64::PULONG)}}},
        {"NtEnumerateDriverEntries", 2, {{"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"PULONG", "BufferLength", sizeof(nt64::PULONG)}}},
        {"NtEnumerateKey", 6, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Index", sizeof(nt64::ULONG)}, {"KEY_INFORMATION_CLASS", "KeyInformationClass", sizeof(nt64::KEY_INFORMATION_CLASS)}, {"PVOID", "KeyInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PULONG", "ResultLength", sizeof(nt64::PULONG)}}},
        {"NtEnumerateSystemEnvironmentValuesEx", 3, {{"ULONG", "InformationClass", sizeof(nt64::ULONG)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"PULONG", "BufferLength", sizeof(nt64::PULONG)}}},
        {"NtEnumerateTransactionObject", 5, {{"HANDLE", "RootObjectHandle", sizeof(nt64::HANDLE)}, {"KTMOBJECT_TYPE", "QueryType", sizeof(nt64::KTMOBJECT_TYPE)}, {"PKTMOBJECT_CURSOR", "ObjectCursor", sizeof(nt64::PKTMOBJECT_CURSOR)}, {"ULONG", "ObjectCursorLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtEnumerateValueKey", 6, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Index", sizeof(nt64::ULONG)}, {"KEY_VALUE_INFORMATION_CLASS", "KeyValueInformationClass", sizeof(nt64::KEY_VALUE_INFORMATION_CLASS)}, {"PVOID", "KeyValueInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PULONG", "ResultLength", sizeof(nt64::PULONG)}}},
        {"NtExtendSection", 2, {{"HANDLE", "SectionHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "NewSectionSize", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtFilterToken", 6, {{"HANDLE", "ExistingTokenHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PTOKEN_GROUPS", "SidsToDisable", sizeof(nt64::PTOKEN_GROUPS)}, {"PTOKEN_PRIVILEGES", "PrivilegesToDelete", sizeof(nt64::PTOKEN_PRIVILEGES)}, {"PTOKEN_GROUPS", "RestrictedSids", sizeof(nt64::PTOKEN_GROUPS)}, {"PHANDLE", "NewTokenHandle", sizeof(nt64::PHANDLE)}}},
        {"NtFindAtom", 3, {{"PWSTR", "AtomName", sizeof(nt64::PWSTR)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PRTL_ATOM", "Atom", sizeof(nt64::PRTL_ATOM)}}},
        {"NtFlushBuffersFile", 2, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}}},
        {"NtFlushInstallUILanguage", 2, {{"LANGID", "InstallUILanguage", sizeof(nt64::LANGID)}, {"ULONG", "SetComittedFlag", sizeof(nt64::ULONG)}}},
        {"NtFlushInstructionCache", 3, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}, {"SIZE_T", "Length", sizeof(nt64::SIZE_T)}}},
        {"NtFlushKey", 1, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}}},
        {"NtFlushVirtualMemory", 4, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt64::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt64::PSIZE_T)}, {"PIO_STATUS_BLOCK", "IoStatus", sizeof(nt64::PIO_STATUS_BLOCK)}}},
        {"NtFreeUserPhysicalPages", 3, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PULONG_PTR", "NumberOfPages", sizeof(nt64::PULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(nt64::PULONG_PTR)}}},
        {"NtFreeVirtualMemory", 4, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt64::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt64::PSIZE_T)}, {"ULONG", "FreeType", sizeof(nt64::ULONG)}}},
        {"NtFreezeRegistry", 1, {{"ULONG", "TimeOutInSeconds", sizeof(nt64::ULONG)}}},
        {"NtFreezeTransactions", 2, {{"PLARGE_INTEGER", "FreezeTimeout", sizeof(nt64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "ThawTimeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtFsControlFile", 10, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"ULONG", "IoControlCode", sizeof(nt64::ULONG)}, {"PVOID", "InputBuffer", sizeof(nt64::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt64::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt64::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt64::ULONG)}}},
        {"NtGetContextThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"PCONTEXT", "ThreadContext", sizeof(nt64::PCONTEXT)}}},
        {"NtGetDevicePowerState", 2, {{"HANDLE", "Device", sizeof(nt64::HANDLE)}, {"DEVICE_POWER_STATE", "STARState", sizeof(nt64::DEVICE_POWER_STATE)}}},
        {"NtGetMUIRegistryInfo", 3, {{"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PULONG", "DataSize", sizeof(nt64::PULONG)}, {"PVOID", "Data", sizeof(nt64::PVOID)}}},
        {"NtGetNextProcess", 5, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt64::ULONG)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PHANDLE", "NewProcessHandle", sizeof(nt64::PHANDLE)}}},
        {"NtGetNextThread", 6, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt64::ULONG)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PHANDLE", "NewThreadHandle", sizeof(nt64::PHANDLE)}}},
        {"NtGetNlsSectionPtr", 5, {{"ULONG", "SectionType", sizeof(nt64::ULONG)}, {"ULONG", "SectionData", sizeof(nt64::ULONG)}, {"PVOID", "ContextData", sizeof(nt64::PVOID)}, {"PVOID", "STARSectionPointer", sizeof(nt64::PVOID)}, {"PULONG", "SectionSize", sizeof(nt64::PULONG)}}},
        {"NtGetNotificationResourceManager", 7, {{"HANDLE", "ResourceManagerHandle", sizeof(nt64::HANDLE)}, {"PTRANSACTION_NOTIFICATION", "TransactionNotification", sizeof(nt64::PTRANSACTION_NOTIFICATION)}, {"ULONG", "NotificationLength", sizeof(nt64::ULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}, {"ULONG", "Asynchronous", sizeof(nt64::ULONG)}, {"ULONG_PTR", "AsynchronousContext", sizeof(nt64::ULONG_PTR)}}},
        {"NtGetPlugPlayEvent", 4, {{"HANDLE", "EventHandle", sizeof(nt64::HANDLE)}, {"PVOID", "Context", sizeof(nt64::PVOID)}, {"PPLUGPLAY_EVENT_BLOCK", "EventBlock", sizeof(nt64::PPLUGPLAY_EVENT_BLOCK)}, {"ULONG", "EventBufferSize", sizeof(nt64::ULONG)}}},
        {"NtGetWriteWatch", 7, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}, {"SIZE_T", "RegionSize", sizeof(nt64::SIZE_T)}, {"PVOID", "STARUserAddressArray", sizeof(nt64::PVOID)}, {"PULONG_PTR", "EntriesInUserAddressArray", sizeof(nt64::PULONG_PTR)}, {"PULONG", "Granularity", sizeof(nt64::PULONG)}}},
        {"NtImpersonateAnonymousToken", 1, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}}},
        {"NtImpersonateClientOfPort", 2, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(nt64::PPORT_MESSAGE)}}},
        {"NtImpersonateThread", 3, {{"HANDLE", "ServerThreadHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "ClientThreadHandle", sizeof(nt64::HANDLE)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(nt64::PSECURITY_QUALITY_OF_SERVICE)}}},
        {"NtInitializeNlsFiles", 3, {{"PVOID", "STARBaseAddress", sizeof(nt64::PVOID)}, {"PLCID", "DefaultLocaleId", sizeof(nt64::PLCID)}, {"PLARGE_INTEGER", "DefaultCasingTableSize", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtInitializeRegistry", 1, {{"USHORT", "BootCondition", sizeof(nt64::USHORT)}}},
        {"NtInitiatePowerAction", 4, {{"POWER_ACTION", "SystemAction", sizeof(nt64::POWER_ACTION)}, {"SYSTEM_POWER_STATE", "MinSystemState", sizeof(nt64::SYSTEM_POWER_STATE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(nt64::BOOLEAN)}}},
        {"NtIsProcessInJob", 2, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "JobHandle", sizeof(nt64::HANDLE)}}},
        {"NtListenPort", 2, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(nt64::PPORT_MESSAGE)}}},
        {"NtLoadDriver", 1, {{"PUNICODE_STRING", "DriverServiceName", sizeof(nt64::PUNICODE_STRING)}}},
        {"NtLoadKey2", 3, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}}},
        {"NtLoadKeyEx", 4, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"HANDLE", "TrustClassKey", sizeof(nt64::HANDLE)}}},
        {"NtLoadKey", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtLockFile", 10, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "Length", sizeof(nt64::PLARGE_INTEGER)}, {"ULONG", "Key", sizeof(nt64::ULONG)}, {"BOOLEAN", "FailImmediately", sizeof(nt64::BOOLEAN)}, {"BOOLEAN", "ExclusiveLock", sizeof(nt64::BOOLEAN)}}},
        {"NtLockProductActivationKeys", 2, {{"ULONG", "STARpPrivateVer", sizeof(nt64::ULONG)}, {"ULONG", "STARpSafeMode", sizeof(nt64::ULONG)}}},
        {"NtLockRegistryKey", 1, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}}},
        {"NtLockVirtualMemory", 4, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt64::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt64::PSIZE_T)}, {"ULONG", "MapType", sizeof(nt64::ULONG)}}},
        {"NtMakePermanentObject", 1, {{"HANDLE", "Handle", sizeof(nt64::HANDLE)}}},
        {"NtMakeTemporaryObject", 1, {{"HANDLE", "Handle", sizeof(nt64::HANDLE)}}},
        {"NtMapCMFModule", 6, {{"ULONG", "What", sizeof(nt64::ULONG)}, {"ULONG", "Index", sizeof(nt64::ULONG)}, {"PULONG", "CacheIndexOut", sizeof(nt64::PULONG)}, {"PULONG", "CacheFlagsOut", sizeof(nt64::PULONG)}, {"PULONG", "ViewSizeOut", sizeof(nt64::PULONG)}, {"PVOID", "STARBaseAddress", sizeof(nt64::PVOID)}}},
        {"NtMapUserPhysicalPages", 3, {{"PVOID", "VirtualAddress", sizeof(nt64::PVOID)}, {"ULONG_PTR", "NumberOfPages", sizeof(nt64::ULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(nt64::PULONG_PTR)}}},
        {"NtMapUserPhysicalPagesScatter", 3, {{"PVOID", "STARVirtualAddresses", sizeof(nt64::PVOID)}, {"ULONG_PTR", "NumberOfPages", sizeof(nt64::ULONG_PTR)}, {"PULONG_PTR", "UserPfnArray", sizeof(nt64::PULONG_PTR)}}},
        {"NtMapViewOfSection", 10, {{"HANDLE", "SectionHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt64::PVOID)}, {"ULONG_PTR", "ZeroBits", sizeof(nt64::ULONG_PTR)}, {"SIZE_T", "CommitSize", sizeof(nt64::SIZE_T)}, {"PLARGE_INTEGER", "SectionOffset", sizeof(nt64::PLARGE_INTEGER)}, {"PSIZE_T", "ViewSize", sizeof(nt64::PSIZE_T)}, {"SECTION_INHERIT", "InheritDisposition", sizeof(nt64::SECTION_INHERIT)}, {"ULONG", "AllocationType", sizeof(nt64::ULONG)}, {"WIN32_PROTECTION_MASK", "Win32Protect", sizeof(nt64::WIN32_PROTECTION_MASK)}}},
        {"NtModifyBootEntry", 1, {{"PBOOT_ENTRY", "BootEntry", sizeof(nt64::PBOOT_ENTRY)}}},
        {"NtModifyDriverEntry", 1, {{"PEFI_DRIVER_ENTRY", "DriverEntry", sizeof(nt64::PEFI_DRIVER_ENTRY)}}},
        {"NtNotifyChangeDirectoryFile", 9, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"ULONG", "CompletionFilter", sizeof(nt64::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(nt64::BOOLEAN)}}},
        {"NtNotifyChangeKey", 10, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"ULONG", "CompletionFilter", sizeof(nt64::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(nt64::BOOLEAN)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "BufferSize", sizeof(nt64::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(nt64::BOOLEAN)}}},
        {"NtNotifyChangeMultipleKeys", 12, {{"HANDLE", "MasterKeyHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Count", sizeof(nt64::ULONG)}, {"POBJECT_ATTRIBUTES", "SlaveObjects", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"ULONG", "CompletionFilter", sizeof(nt64::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(nt64::BOOLEAN)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "BufferSize", sizeof(nt64::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(nt64::BOOLEAN)}}},
        {"NtNotifyChangeSession", 8, {{"HANDLE", "Session", sizeof(nt64::HANDLE)}, {"ULONG", "IoStateSequence", sizeof(nt64::ULONG)}, {"PVOID", "Reserved", sizeof(nt64::PVOID)}, {"ULONG", "Action", sizeof(nt64::ULONG)}, {"IO_SESSION_STATE", "IoState", sizeof(nt64::IO_SESSION_STATE)}, {"IO_SESSION_STATE", "IoState2", sizeof(nt64::IO_SESSION_STATE)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "BufferSize", sizeof(nt64::ULONG)}}},
        {"NtOpenDirectoryObject", 3, {{"PHANDLE", "DirectoryHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenEnlistment", 5, {{"PHANDLE", "EnlistmentHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"HANDLE", "ResourceManagerHandle", sizeof(nt64::HANDLE)}, {"LPGUID", "EnlistmentGuid", sizeof(nt64::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenEvent", 3, {{"PHANDLE", "EventHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenEventPair", 3, {{"PHANDLE", "EventPairHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenFile", 6, {{"PHANDLE", "FileHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"ULONG", "ShareAccess", sizeof(nt64::ULONG)}, {"ULONG", "OpenOptions", sizeof(nt64::ULONG)}}},
        {"NtOpenIoCompletion", 3, {{"PHANDLE", "IoCompletionHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenJobObject", 3, {{"PHANDLE", "JobHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenKeyedEvent", 3, {{"PHANDLE", "KeyedEventHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenKeyEx", 4, {{"PHANDLE", "KeyHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "OpenOptions", sizeof(nt64::ULONG)}}},
        {"NtOpenKey", 3, {{"PHANDLE", "KeyHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenKeyTransactedEx", 5, {{"PHANDLE", "KeyHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "OpenOptions", sizeof(nt64::ULONG)}, {"HANDLE", "TransactionHandle", sizeof(nt64::HANDLE)}}},
        {"NtOpenKeyTransacted", 4, {{"PHANDLE", "KeyHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"HANDLE", "TransactionHandle", sizeof(nt64::HANDLE)}}},
        {"NtOpenMutant", 3, {{"PHANDLE", "MutantHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenObjectAuditAlarm", 12, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt64::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt64::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt64::PSECURITY_DESCRIPTOR)}, {"HANDLE", "ClientToken", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"ACCESS_MASK", "GrantedAccess", sizeof(nt64::ACCESS_MASK)}, {"PPRIVILEGE_SET", "Privileges", sizeof(nt64::PPRIVILEGE_SET)}, {"BOOLEAN", "ObjectCreation", sizeof(nt64::BOOLEAN)}, {"BOOLEAN", "AccessGranted", sizeof(nt64::BOOLEAN)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt64::PBOOLEAN)}}},
        {"NtOpenPrivateNamespace", 4, {{"PHANDLE", "NamespaceHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PVOID", "BoundaryDescriptor", sizeof(nt64::PVOID)}}},
        {"NtOpenProcess", 4, {{"PHANDLE", "ProcessHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PCLIENT_ID", "ClientId", sizeof(nt64::PCLIENT_ID)}}},
        {"NtOpenProcessTokenEx", 4, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt64::ULONG)}, {"PHANDLE", "TokenHandle", sizeof(nt64::PHANDLE)}}},
        {"NtOpenProcessToken", 3, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"PHANDLE", "TokenHandle", sizeof(nt64::PHANDLE)}}},
        {"NtOpenResourceManager", 5, {{"PHANDLE", "ResourceManagerHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"HANDLE", "TmHandle", sizeof(nt64::HANDLE)}, {"LPGUID", "ResourceManagerGuid", sizeof(nt64::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenSection", 3, {{"PHANDLE", "SectionHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenSemaphore", 3, {{"PHANDLE", "SemaphoreHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenSession", 3, {{"PHANDLE", "SessionHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenSymbolicLinkObject", 3, {{"PHANDLE", "LinkHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenThread", 4, {{"PHANDLE", "ThreadHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PCLIENT_ID", "ClientId", sizeof(nt64::PCLIENT_ID)}}},
        {"NtOpenThreadTokenEx", 5, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"BOOLEAN", "OpenAsSelf", sizeof(nt64::BOOLEAN)}, {"ULONG", "HandleAttributes", sizeof(nt64::ULONG)}, {"PHANDLE", "TokenHandle", sizeof(nt64::PHANDLE)}}},
        {"NtOpenThreadToken", 4, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"BOOLEAN", "OpenAsSelf", sizeof(nt64::BOOLEAN)}, {"PHANDLE", "TokenHandle", sizeof(nt64::PHANDLE)}}},
        {"NtOpenTimer", 3, {{"PHANDLE", "TimerHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtOpenTransactionManager", 6, {{"PHANDLE", "TmHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LogFileName", sizeof(nt64::PUNICODE_STRING)}, {"LPGUID", "TmIdentity", sizeof(nt64::LPGUID)}, {"ULONG", "OpenOptions", sizeof(nt64::ULONG)}}},
        {"NtOpenTransaction", 5, {{"PHANDLE", "TransactionHandle", sizeof(nt64::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"LPGUID", "Uow", sizeof(nt64::LPGUID)}, {"HANDLE", "TmHandle", sizeof(nt64::HANDLE)}}},
        {"NtPlugPlayControl", 3, {{"PLUGPLAY_CONTROL_CLASS", "PnPControlClass", sizeof(nt64::PLUGPLAY_CONTROL_CLASS)}, {"PVOID", "PnPControlData", sizeof(nt64::PVOID)}, {"ULONG", "PnPControlDataLength", sizeof(nt64::ULONG)}}},
        {"NtPowerInformation", 5, {{"POWER_INFORMATION_LEVEL", "InformationLevel", sizeof(nt64::POWER_INFORMATION_LEVEL)}, {"PVOID", "InputBuffer", sizeof(nt64::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt64::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt64::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt64::ULONG)}}},
        {"NtPrepareComplete", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtPrepareEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtPrePrepareComplete", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtPrePrepareEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtPrivilegeCheck", 3, {{"HANDLE", "ClientToken", sizeof(nt64::HANDLE)}, {"PPRIVILEGE_SET", "RequiredPrivileges", sizeof(nt64::PPRIVILEGE_SET)}, {"PBOOLEAN", "Result", sizeof(nt64::PBOOLEAN)}}},
        {"NtPrivilegedServiceAuditAlarm", 5, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt64::PUNICODE_STRING)}, {"PUNICODE_STRING", "ServiceName", sizeof(nt64::PUNICODE_STRING)}, {"HANDLE", "ClientToken", sizeof(nt64::HANDLE)}, {"PPRIVILEGE_SET", "Privileges", sizeof(nt64::PPRIVILEGE_SET)}, {"BOOLEAN", "AccessGranted", sizeof(nt64::BOOLEAN)}}},
        {"NtPrivilegeObjectAuditAlarm", 6, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt64::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt64::PVOID)}, {"HANDLE", "ClientToken", sizeof(nt64::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt64::ACCESS_MASK)}, {"PPRIVILEGE_SET", "Privileges", sizeof(nt64::PPRIVILEGE_SET)}, {"BOOLEAN", "AccessGranted", sizeof(nt64::BOOLEAN)}}},
        {"NtPropagationComplete", 4, {{"HANDLE", "ResourceManagerHandle", sizeof(nt64::HANDLE)}, {"ULONG", "RequestCookie", sizeof(nt64::ULONG)}, {"ULONG", "BufferLength", sizeof(nt64::ULONG)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}}},
        {"NtPropagationFailed", 3, {{"HANDLE", "ResourceManagerHandle", sizeof(nt64::HANDLE)}, {"ULONG", "RequestCookie", sizeof(nt64::ULONG)}, {"NTSTATUS", "PropStatus", sizeof(nt64::NTSTATUS)}}},
        {"NtProtectVirtualMemory", 5, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt64::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt64::PSIZE_T)}, {"WIN32_PROTECTION_MASK", "NewProtectWin32", sizeof(nt64::WIN32_PROTECTION_MASK)}, {"PULONG", "OldProtect", sizeof(nt64::PULONG)}}},
        {"NtPulseEvent", 2, {{"HANDLE", "EventHandle", sizeof(nt64::HANDLE)}, {"PLONG", "PreviousState", sizeof(nt64::PLONG)}}},
        {"NtQueryAttributesFile", 2, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PFILE_BASIC_INFORMATION", "FileInformation", sizeof(nt64::PFILE_BASIC_INFORMATION)}}},
        {"NtQueryBootEntryOrder", 2, {{"PULONG", "Ids", sizeof(nt64::PULONG)}, {"PULONG", "Count", sizeof(nt64::PULONG)}}},
        {"NtQueryBootOptions", 2, {{"PBOOT_OPTIONS", "BootOptions", sizeof(nt64::PBOOT_OPTIONS)}, {"PULONG", "BootOptionsLength", sizeof(nt64::PULONG)}}},
        {"NtQueryDebugFilterState", 2, {{"ULONG", "ComponentId", sizeof(nt64::ULONG)}, {"ULONG", "Level", sizeof(nt64::ULONG)}}},
        {"NtQueryDefaultLocale", 2, {{"BOOLEAN", "UserProfile", sizeof(nt64::BOOLEAN)}, {"PLCID", "DefaultLocaleId", sizeof(nt64::PLCID)}}},
        {"NtQueryDefaultUILanguage", 1, {{"LANGID", "STARDefaultUILanguageId", sizeof(nt64::LANGID)}}},
        {"NtQueryDirectoryFile", 11, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(nt64::FILE_INFORMATION_CLASS)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt64::BOOLEAN)}, {"PUNICODE_STRING", "FileName", sizeof(nt64::PUNICODE_STRING)}, {"BOOLEAN", "RestartScan", sizeof(nt64::BOOLEAN)}}},
        {"NtQueryDirectoryObject", 7, {{"HANDLE", "DirectoryHandle", sizeof(nt64::HANDLE)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt64::BOOLEAN)}, {"BOOLEAN", "RestartScan", sizeof(nt64::BOOLEAN)}, {"PULONG", "Context", sizeof(nt64::PULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryDriverEntryOrder", 2, {{"PULONG", "Ids", sizeof(nt64::PULONG)}, {"PULONG", "Count", sizeof(nt64::PULONG)}}},
        {"NtQueryEaFile", 9, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt64::BOOLEAN)}, {"PVOID", "EaList", sizeof(nt64::PVOID)}, {"ULONG", "EaListLength", sizeof(nt64::ULONG)}, {"PULONG", "EaIndex", sizeof(nt64::PULONG)}, {"BOOLEAN", "RestartScan", sizeof(nt64::BOOLEAN)}}},
        {"NtQueryEvent", 5, {{"HANDLE", "EventHandle", sizeof(nt64::HANDLE)}, {"EVENT_INFORMATION_CLASS", "EventInformationClass", sizeof(nt64::EVENT_INFORMATION_CLASS)}, {"PVOID", "EventInformation", sizeof(nt64::PVOID)}, {"ULONG", "EventInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryFullAttributesFile", 2, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PFILE_NETWORK_OPEN_INFORMATION", "FileInformation", sizeof(nt64::PFILE_NETWORK_OPEN_INFORMATION)}}},
        {"NtQueryInformationAtom", 5, {{"RTL_ATOM", "Atom", sizeof(nt64::RTL_ATOM)}, {"ATOM_INFORMATION_CLASS", "InformationClass", sizeof(nt64::ATOM_INFORMATION_CLASS)}, {"PVOID", "AtomInformation", sizeof(nt64::PVOID)}, {"ULONG", "AtomInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInformationEnlistment", 5, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"ENLISTMENT_INFORMATION_CLASS", "EnlistmentInformationClass", sizeof(nt64::ENLISTMENT_INFORMATION_CLASS)}, {"PVOID", "EnlistmentInformation", sizeof(nt64::PVOID)}, {"ULONG", "EnlistmentInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInformationFile", 5, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(nt64::FILE_INFORMATION_CLASS)}}},
        {"NtQueryInformationJobObject", 5, {{"HANDLE", "JobHandle", sizeof(nt64::HANDLE)}, {"JOBOBJECTINFOCLASS", "JobObjectInformationClass", sizeof(nt64::JOBOBJECTINFOCLASS)}, {"PVOID", "JobObjectInformation", sizeof(nt64::PVOID)}, {"ULONG", "JobObjectInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInformationPort", 5, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(nt64::PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInformationProcess", 5, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PROCESSINFOCLASS", "ProcessInformationClass", sizeof(nt64::PROCESSINFOCLASS)}, {"PVOID", "ProcessInformation", sizeof(nt64::PVOID)}, {"ULONG", "ProcessInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInformationResourceManager", 5, {{"HANDLE", "ResourceManagerHandle", sizeof(nt64::HANDLE)}, {"RESOURCEMANAGER_INFORMATION_CLASS", "ResourceManagerInformationClass", sizeof(nt64::RESOURCEMANAGER_INFORMATION_CLASS)}, {"PVOID", "ResourceManagerInformation", sizeof(nt64::PVOID)}, {"ULONG", "ResourceManagerInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInformationThread", 5, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"THREADINFOCLASS", "ThreadInformationClass", sizeof(nt64::THREADINFOCLASS)}, {"PVOID", "ThreadInformation", sizeof(nt64::PVOID)}, {"ULONG", "ThreadInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInformationToken", 5, {{"HANDLE", "TokenHandle", sizeof(nt64::HANDLE)}, {"TOKEN_INFORMATION_CLASS", "TokenInformationClass", sizeof(nt64::TOKEN_INFORMATION_CLASS)}, {"PVOID", "TokenInformation", sizeof(nt64::PVOID)}, {"ULONG", "TokenInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInformationTransaction", 5, {{"HANDLE", "TransactionHandle", sizeof(nt64::HANDLE)}, {"TRANSACTION_INFORMATION_CLASS", "TransactionInformationClass", sizeof(nt64::TRANSACTION_INFORMATION_CLASS)}, {"PVOID", "TransactionInformation", sizeof(nt64::PVOID)}, {"ULONG", "TransactionInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInformationTransactionManager", 5, {{"HANDLE", "TransactionManagerHandle", sizeof(nt64::HANDLE)}, {"TRANSACTIONMANAGER_INFORMATION_CLASS", "TransactionManagerInformationClass", sizeof(nt64::TRANSACTIONMANAGER_INFORMATION_CLASS)}, {"PVOID", "TransactionManagerInformation", sizeof(nt64::PVOID)}, {"ULONG", "TransactionManagerInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInformationWorkerFactory", 5, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt64::HANDLE)}, {"WORKERFACTORYINFOCLASS", "WorkerFactoryInformationClass", sizeof(nt64::WORKERFACTORYINFOCLASS)}, {"PVOID", "WorkerFactoryInformation", sizeof(nt64::PVOID)}, {"ULONG", "WorkerFactoryInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryInstallUILanguage", 1, {{"LANGID", "STARInstallUILanguageId", sizeof(nt64::LANGID)}}},
        {"NtQueryIntervalProfile", 2, {{"KPROFILE_SOURCE", "ProfileSource", sizeof(nt64::KPROFILE_SOURCE)}, {"PULONG", "Interval", sizeof(nt64::PULONG)}}},
        {"NtQueryIoCompletion", 5, {{"HANDLE", "IoCompletionHandle", sizeof(nt64::HANDLE)}, {"IO_COMPLETION_INFORMATION_CLASS", "IoCompletionInformationClass", sizeof(nt64::IO_COMPLETION_INFORMATION_CLASS)}, {"PVOID", "IoCompletionInformation", sizeof(nt64::PVOID)}, {"ULONG", "IoCompletionInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryKey", 5, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"KEY_INFORMATION_CLASS", "KeyInformationClass", sizeof(nt64::KEY_INFORMATION_CLASS)}, {"PVOID", "KeyInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PULONG", "ResultLength", sizeof(nt64::PULONG)}}},
        {"NtQueryLicenseValue", 5, {{"PUNICODE_STRING", "Name", sizeof(nt64::PUNICODE_STRING)}, {"PULONG", "Type", sizeof(nt64::PULONG)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PULONG", "ReturnedLength", sizeof(nt64::PULONG)}}},
        {"NtQueryMultipleValueKey", 6, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"PKEY_VALUE_ENTRY", "ValueEntries", sizeof(nt64::PKEY_VALUE_ENTRY)}, {"ULONG", "EntryCount", sizeof(nt64::ULONG)}, {"PVOID", "ValueBuffer", sizeof(nt64::PVOID)}, {"PULONG", "BufferLength", sizeof(nt64::PULONG)}, {"PULONG", "RequiredBufferLength", sizeof(nt64::PULONG)}}},
        {"NtQueryMutant", 5, {{"HANDLE", "MutantHandle", sizeof(nt64::HANDLE)}, {"MUTANT_INFORMATION_CLASS", "MutantInformationClass", sizeof(nt64::MUTANT_INFORMATION_CLASS)}, {"PVOID", "MutantInformation", sizeof(nt64::PVOID)}, {"ULONG", "MutantInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryObject", 5, {{"HANDLE", "Handle", sizeof(nt64::HANDLE)}, {"OBJECT_INFORMATION_CLASS", "ObjectInformationClass", sizeof(nt64::OBJECT_INFORMATION_CLASS)}, {"PVOID", "ObjectInformation", sizeof(nt64::PVOID)}, {"ULONG", "ObjectInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryOpenSubKeysEx", 4, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "BufferLength", sizeof(nt64::ULONG)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"PULONG", "RequiredSize", sizeof(nt64::PULONG)}}},
        {"NtQueryOpenSubKeys", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"PULONG", "HandleCount", sizeof(nt64::PULONG)}}},
        {"NtQueryPerformanceCounter", 2, {{"PLARGE_INTEGER", "PerformanceCounter", sizeof(nt64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "PerformanceFrequency", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtQueryQuotaInformationFile", 9, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt64::BOOLEAN)}, {"PVOID", "SidList", sizeof(nt64::PVOID)}, {"ULONG", "SidListLength", sizeof(nt64::ULONG)}, {"PULONG", "StartSid", sizeof(nt64::PULONG)}, {"BOOLEAN", "RestartScan", sizeof(nt64::BOOLEAN)}}},
        {"NtQuerySection", 5, {{"HANDLE", "SectionHandle", sizeof(nt64::HANDLE)}, {"SECTION_INFORMATION_CLASS", "SectionInformationClass", sizeof(nt64::SECTION_INFORMATION_CLASS)}, {"PVOID", "SectionInformation", sizeof(nt64::PVOID)}, {"SIZE_T", "SectionInformationLength", sizeof(nt64::SIZE_T)}, {"PSIZE_T", "ReturnLength", sizeof(nt64::PSIZE_T)}}},
        {"NtQuerySecurityAttributesToken", 6, {{"HANDLE", "TokenHandle", sizeof(nt64::HANDLE)}, {"PUNICODE_STRING", "Attributes", sizeof(nt64::PUNICODE_STRING)}, {"ULONG", "NumberOfAttributes", sizeof(nt64::ULONG)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQuerySecurityObject", 5, {{"HANDLE", "Handle", sizeof(nt64::HANDLE)}, {"SECURITY_INFORMATION", "SecurityInformation", sizeof(nt64::SECURITY_INFORMATION)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt64::PSECURITY_DESCRIPTOR)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PULONG", "LengthNeeded", sizeof(nt64::PULONG)}}},
        {"NtQuerySemaphore", 5, {{"HANDLE", "SemaphoreHandle", sizeof(nt64::HANDLE)}, {"SEMAPHORE_INFORMATION_CLASS", "SemaphoreInformationClass", sizeof(nt64::SEMAPHORE_INFORMATION_CLASS)}, {"PVOID", "SemaphoreInformation", sizeof(nt64::PVOID)}, {"ULONG", "SemaphoreInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQuerySymbolicLinkObject", 3, {{"HANDLE", "LinkHandle", sizeof(nt64::HANDLE)}, {"PUNICODE_STRING", "LinkTarget", sizeof(nt64::PUNICODE_STRING)}, {"PULONG", "ReturnedLength", sizeof(nt64::PULONG)}}},
        {"NtQuerySystemEnvironmentValueEx", 5, {{"PUNICODE_STRING", "VariableName", sizeof(nt64::PUNICODE_STRING)}, {"LPGUID", "VendorGuid", sizeof(nt64::LPGUID)}, {"PVOID", "Value", sizeof(nt64::PVOID)}, {"PULONG", "ValueLength", sizeof(nt64::PULONG)}, {"PULONG", "Attributes", sizeof(nt64::PULONG)}}},
        {"NtQuerySystemEnvironmentValue", 4, {{"PUNICODE_STRING", "VariableName", sizeof(nt64::PUNICODE_STRING)}, {"PWSTR", "VariableValue", sizeof(nt64::PWSTR)}, {"USHORT", "ValueLength", sizeof(nt64::USHORT)}, {"PUSHORT", "ReturnLength", sizeof(nt64::PUSHORT)}}},
        {"NtQuerySystemInformationEx", 6, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(nt64::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "QueryInformation", sizeof(nt64::PVOID)}, {"ULONG", "QueryInformationLength", sizeof(nt64::ULONG)}, {"PVOID", "SystemInformation", sizeof(nt64::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQuerySystemInformation", 4, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(nt64::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "SystemInformation", sizeof(nt64::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQuerySystemTime", 1, {{"PLARGE_INTEGER", "SystemTime", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtQueryTimer", 5, {{"HANDLE", "TimerHandle", sizeof(nt64::HANDLE)}, {"TIMER_INFORMATION_CLASS", "TimerInformationClass", sizeof(nt64::TIMER_INFORMATION_CLASS)}, {"PVOID", "TimerInformation", sizeof(nt64::PVOID)}, {"ULONG", "TimerInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtQueryTimerResolution", 3, {{"PULONG", "MaximumTime", sizeof(nt64::PULONG)}, {"PULONG", "MinimumTime", sizeof(nt64::PULONG)}, {"PULONG", "CurrentTime", sizeof(nt64::PULONG)}}},
        {"NtQueryValueKey", 6, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(nt64::PUNICODE_STRING)}, {"KEY_VALUE_INFORMATION_CLASS", "KeyValueInformationClass", sizeof(nt64::KEY_VALUE_INFORMATION_CLASS)}, {"PVOID", "KeyValueInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PULONG", "ResultLength", sizeof(nt64::PULONG)}}},
        {"NtQueryVirtualMemory", 6, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}, {"MEMORY_INFORMATION_CLASS", "MemoryInformationClass", sizeof(nt64::MEMORY_INFORMATION_CLASS)}, {"PVOID", "MemoryInformation", sizeof(nt64::PVOID)}, {"SIZE_T", "MemoryInformationLength", sizeof(nt64::SIZE_T)}, {"PSIZE_T", "ReturnLength", sizeof(nt64::PSIZE_T)}}},
        {"NtQueryVolumeInformationFile", 5, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "FsInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"FS_INFORMATION_CLASS", "FsInformationClass", sizeof(nt64::FS_INFORMATION_CLASS)}}},
        {"NtQueueApcThreadEx", 6, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "UserApcReserveHandle", sizeof(nt64::HANDLE)}, {"PPS_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PPS_APC_ROUTINE)}, {"PVOID", "ApcArgument1", sizeof(nt64::PVOID)}, {"PVOID", "ApcArgument2", sizeof(nt64::PVOID)}, {"PVOID", "ApcArgument3", sizeof(nt64::PVOID)}}},
        {"NtQueueApcThread", 5, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"PPS_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PPS_APC_ROUTINE)}, {"PVOID", "ApcArgument1", sizeof(nt64::PVOID)}, {"PVOID", "ApcArgument2", sizeof(nt64::PVOID)}, {"PVOID", "ApcArgument3", sizeof(nt64::PVOID)}}},
        {"NtRaiseException", 3, {{"PEXCEPTION_RECORD", "ExceptionRecord", sizeof(nt64::PEXCEPTION_RECORD)}, {"PCONTEXT", "ContextRecord", sizeof(nt64::PCONTEXT)}, {"BOOLEAN", "FirstChance", sizeof(nt64::BOOLEAN)}}},
        {"NtRaiseHardError", 6, {{"NTSTATUS", "ErrorStatus", sizeof(nt64::NTSTATUS)}, {"ULONG", "NumberOfParameters", sizeof(nt64::ULONG)}, {"ULONG", "UnicodeStringParameterMask", sizeof(nt64::ULONG)}, {"PULONG_PTR", "Parameters", sizeof(nt64::PULONG_PTR)}, {"ULONG", "ValidResponseOptions", sizeof(nt64::ULONG)}, {"PULONG", "Response", sizeof(nt64::PULONG)}}},
        {"NtReadFile", 9, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt64::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt64::PULONG)}}},
        {"NtReadFileScatter", 9, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PFILE_SEGMENT_ELEMENT", "SegmentArray", sizeof(nt64::PFILE_SEGMENT_ELEMENT)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt64::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt64::PULONG)}}},
        {"NtReadOnlyEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtReadRequestData", 6, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(nt64::PPORT_MESSAGE)}, {"ULONG", "DataEntryIndex", sizeof(nt64::ULONG)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt64::SIZE_T)}, {"PSIZE_T", "NumberOfBytesRead", sizeof(nt64::PSIZE_T)}}},
        {"NtReadVirtualMemory", 5, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt64::SIZE_T)}, {"PSIZE_T", "NumberOfBytesRead", sizeof(nt64::PSIZE_T)}}},
        {"NtRecoverEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PVOID", "EnlistmentKey", sizeof(nt64::PVOID)}}},
        {"NtRecoverResourceManager", 1, {{"HANDLE", "ResourceManagerHandle", sizeof(nt64::HANDLE)}}},
        {"NtRecoverTransactionManager", 1, {{"HANDLE", "TransactionManagerHandle", sizeof(nt64::HANDLE)}}},
        {"NtRegisterProtocolAddressInformation", 5, {{"HANDLE", "ResourceManager", sizeof(nt64::HANDLE)}, {"PCRM_PROTOCOL_ID", "ProtocolId", sizeof(nt64::PCRM_PROTOCOL_ID)}, {"ULONG", "ProtocolInformationSize", sizeof(nt64::ULONG)}, {"PVOID", "ProtocolInformation", sizeof(nt64::PVOID)}, {"ULONG", "CreateOptions", sizeof(nt64::ULONG)}}},
        {"NtRegisterThreadTerminatePort", 1, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}}},
        {"NtReleaseKeyedEvent", 4, {{"HANDLE", "KeyedEventHandle", sizeof(nt64::HANDLE)}, {"PVOID", "KeyValue", sizeof(nt64::PVOID)}, {"BOOLEAN", "Alertable", sizeof(nt64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtReleaseMutant", 2, {{"HANDLE", "MutantHandle", sizeof(nt64::HANDLE)}, {"PLONG", "PreviousCount", sizeof(nt64::PLONG)}}},
        {"NtReleaseSemaphore", 3, {{"HANDLE", "SemaphoreHandle", sizeof(nt64::HANDLE)}, {"LONG", "ReleaseCount", sizeof(nt64::LONG)}, {"PLONG", "PreviousCount", sizeof(nt64::PLONG)}}},
        {"NtReleaseWorkerFactoryWorker", 1, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt64::HANDLE)}}},
        {"NtRemoveIoCompletionEx", 6, {{"HANDLE", "IoCompletionHandle", sizeof(nt64::HANDLE)}, {"PFILE_IO_COMPLETION_INFORMATION", "IoCompletionInformation", sizeof(nt64::PFILE_IO_COMPLETION_INFORMATION)}, {"ULONG", "Count", sizeof(nt64::ULONG)}, {"PULONG", "NumEntriesRemoved", sizeof(nt64::PULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}, {"BOOLEAN", "Alertable", sizeof(nt64::BOOLEAN)}}},
        {"NtRemoveIoCompletion", 5, {{"HANDLE", "IoCompletionHandle", sizeof(nt64::HANDLE)}, {"PVOID", "STARKeyContext", sizeof(nt64::PVOID)}, {"PVOID", "STARApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtRemoveProcessDebug", 2, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "DebugObjectHandle", sizeof(nt64::HANDLE)}}},
        {"NtRenameKey", 2, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"PUNICODE_STRING", "NewName", sizeof(nt64::PUNICODE_STRING)}}},
        {"NtRenameTransactionManager", 2, {{"PUNICODE_STRING", "LogFileName", sizeof(nt64::PUNICODE_STRING)}, {"LPGUID", "ExistingTransactionManagerGuid", sizeof(nt64::LPGUID)}}},
        {"NtReplaceKey", 3, {{"POBJECT_ATTRIBUTES", "NewFile", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"HANDLE", "TargetHandle", sizeof(nt64::HANDLE)}, {"POBJECT_ATTRIBUTES", "OldFile", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtReplacePartitionUnit", 3, {{"PUNICODE_STRING", "TargetInstancePath", sizeof(nt64::PUNICODE_STRING)}, {"PUNICODE_STRING", "SpareInstancePath", sizeof(nt64::PUNICODE_STRING)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}}},
        {"NtReplyPort", 2, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt64::PPORT_MESSAGE)}}},
        {"NtReplyWaitReceivePortEx", 5, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PVOID", "STARPortContext", sizeof(nt64::PVOID)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt64::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(nt64::PPORT_MESSAGE)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtReplyWaitReceivePort", 4, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PVOID", "STARPortContext", sizeof(nt64::PVOID)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt64::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(nt64::PPORT_MESSAGE)}}},
        {"NtReplyWaitReplyPort", 2, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt64::PPORT_MESSAGE)}}},
        {"NtRequestPort", 2, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "RequestMessage", sizeof(nt64::PPORT_MESSAGE)}}},
        {"NtRequestWaitReplyPort", 3, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "RequestMessage", sizeof(nt64::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt64::PPORT_MESSAGE)}}},
        {"NtResetEvent", 2, {{"HANDLE", "EventHandle", sizeof(nt64::HANDLE)}, {"PLONG", "PreviousState", sizeof(nt64::PLONG)}}},
        {"NtResetWriteWatch", 3, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}, {"SIZE_T", "RegionSize", sizeof(nt64::SIZE_T)}}},
        {"NtRestoreKey", 3, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}}},
        {"NtResumeProcess", 1, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}}},
        {"NtResumeThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(nt64::PULONG)}}},
        {"NtRollbackComplete", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtRollbackEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtRollbackTransaction", 2, {{"HANDLE", "TransactionHandle", sizeof(nt64::HANDLE)}, {"BOOLEAN", "Wait", sizeof(nt64::BOOLEAN)}}},
        {"NtRollforwardTransactionManager", 2, {{"HANDLE", "TransactionManagerHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtSaveKeyEx", 3, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Format", sizeof(nt64::ULONG)}}},
        {"NtSaveKey", 2, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}}},
        {"NtSaveMergedKeys", 3, {{"HANDLE", "HighPrecedenceKeyHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "LowPrecedenceKeyHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}}},
        {"NtSecureConnectPort", 9, {{"PHANDLE", "PortHandle", sizeof(nt64::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(nt64::PUNICODE_STRING)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(nt64::PSECURITY_QUALITY_OF_SERVICE)}, {"PPORT_VIEW", "ClientView", sizeof(nt64::PPORT_VIEW)}, {"PSID", "RequiredServerSid", sizeof(nt64::PSID)}, {"PREMOTE_PORT_VIEW", "ServerView", sizeof(nt64::PREMOTE_PORT_VIEW)}, {"PULONG", "MaxMessageLength", sizeof(nt64::PULONG)}, {"PVOID", "ConnectionInformation", sizeof(nt64::PVOID)}, {"PULONG", "ConnectionInformationLength", sizeof(nt64::PULONG)}}},
        {"NtSetBootEntryOrder", 2, {{"PULONG", "Ids", sizeof(nt64::PULONG)}, {"ULONG", "Count", sizeof(nt64::ULONG)}}},
        {"NtSetBootOptions", 2, {{"PBOOT_OPTIONS", "BootOptions", sizeof(nt64::PBOOT_OPTIONS)}, {"ULONG", "FieldsToChange", sizeof(nt64::ULONG)}}},
        {"NtSetContextThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"PCONTEXT", "ThreadContext", sizeof(nt64::PCONTEXT)}}},
        {"NtSetDebugFilterState", 3, {{"ULONG", "ComponentId", sizeof(nt64::ULONG)}, {"ULONG", "Level", sizeof(nt64::ULONG)}, {"BOOLEAN", "State", sizeof(nt64::BOOLEAN)}}},
        {"NtSetDefaultHardErrorPort", 1, {{"HANDLE", "DefaultHardErrorPort", sizeof(nt64::HANDLE)}}},
        {"NtSetDefaultLocale", 2, {{"BOOLEAN", "UserProfile", sizeof(nt64::BOOLEAN)}, {"LCID", "DefaultLocaleId", sizeof(nt64::LCID)}}},
        {"NtSetDefaultUILanguage", 1, {{"LANGID", "DefaultUILanguageId", sizeof(nt64::LANGID)}}},
        {"NtSetDriverEntryOrder", 2, {{"PULONG", "Ids", sizeof(nt64::PULONG)}, {"ULONG", "Count", sizeof(nt64::ULONG)}}},
        {"NtSetEaFile", 4, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}}},
        {"NtSetEventBoostPriority", 1, {{"HANDLE", "EventHandle", sizeof(nt64::HANDLE)}}},
        {"NtSetEvent", 2, {{"HANDLE", "EventHandle", sizeof(nt64::HANDLE)}, {"PLONG", "PreviousState", sizeof(nt64::PLONG)}}},
        {"NtSetHighEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt64::HANDLE)}}},
        {"NtSetHighWaitLowEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt64::HANDLE)}}},
        {"NtSetInformationDebugObject", 5, {{"HANDLE", "DebugObjectHandle", sizeof(nt64::HANDLE)}, {"DEBUGOBJECTINFOCLASS", "DebugObjectInformationClass", sizeof(nt64::DEBUGOBJECTINFOCLASS)}, {"PVOID", "DebugInformation", sizeof(nt64::PVOID)}, {"ULONG", "DebugInformationLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtSetInformationEnlistment", 4, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"ENLISTMENT_INFORMATION_CLASS", "EnlistmentInformationClass", sizeof(nt64::ENLISTMENT_INFORMATION_CLASS)}, {"PVOID", "EnlistmentInformation", sizeof(nt64::PVOID)}, {"ULONG", "EnlistmentInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetInformationFile", 5, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(nt64::FILE_INFORMATION_CLASS)}}},
        {"NtSetInformationJobObject", 4, {{"HANDLE", "JobHandle", sizeof(nt64::HANDLE)}, {"JOBOBJECTINFOCLASS", "JobObjectInformationClass", sizeof(nt64::JOBOBJECTINFOCLASS)}, {"PVOID", "JobObjectInformation", sizeof(nt64::PVOID)}, {"ULONG", "JobObjectInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetInformationKey", 4, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"KEY_SET_INFORMATION_CLASS", "KeySetInformationClass", sizeof(nt64::KEY_SET_INFORMATION_CLASS)}, {"PVOID", "KeySetInformation", sizeof(nt64::PVOID)}, {"ULONG", "KeySetInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetInformationObject", 4, {{"HANDLE", "Handle", sizeof(nt64::HANDLE)}, {"OBJECT_INFORMATION_CLASS", "ObjectInformationClass", sizeof(nt64::OBJECT_INFORMATION_CLASS)}, {"PVOID", "ObjectInformation", sizeof(nt64::PVOID)}, {"ULONG", "ObjectInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetInformationProcess", 4, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PROCESSINFOCLASS", "ProcessInformationClass", sizeof(nt64::PROCESSINFOCLASS)}, {"PVOID", "ProcessInformation", sizeof(nt64::PVOID)}, {"ULONG", "ProcessInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetInformationResourceManager", 4, {{"HANDLE", "ResourceManagerHandle", sizeof(nt64::HANDLE)}, {"RESOURCEMANAGER_INFORMATION_CLASS", "ResourceManagerInformationClass", sizeof(nt64::RESOURCEMANAGER_INFORMATION_CLASS)}, {"PVOID", "ResourceManagerInformation", sizeof(nt64::PVOID)}, {"ULONG", "ResourceManagerInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetInformationThread", 4, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"THREADINFOCLASS", "ThreadInformationClass", sizeof(nt64::THREADINFOCLASS)}, {"PVOID", "ThreadInformation", sizeof(nt64::PVOID)}, {"ULONG", "ThreadInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetInformationToken", 4, {{"HANDLE", "TokenHandle", sizeof(nt64::HANDLE)}, {"TOKEN_INFORMATION_CLASS", "TokenInformationClass", sizeof(nt64::TOKEN_INFORMATION_CLASS)}, {"PVOID", "TokenInformation", sizeof(nt64::PVOID)}, {"ULONG", "TokenInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetInformationTransaction", 4, {{"HANDLE", "TransactionHandle", sizeof(nt64::HANDLE)}, {"TRANSACTION_INFORMATION_CLASS", "TransactionInformationClass", sizeof(nt64::TRANSACTION_INFORMATION_CLASS)}, {"PVOID", "TransactionInformation", sizeof(nt64::PVOID)}, {"ULONG", "TransactionInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetInformationTransactionManager", 4, {{"HANDLE", "TmHandle", sizeof(nt64::HANDLE)}, {"TRANSACTIONMANAGER_INFORMATION_CLASS", "TransactionManagerInformationClass", sizeof(nt64::TRANSACTIONMANAGER_INFORMATION_CLASS)}, {"PVOID", "TransactionManagerInformation", sizeof(nt64::PVOID)}, {"ULONG", "TransactionManagerInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetInformationWorkerFactory", 4, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt64::HANDLE)}, {"WORKERFACTORYINFOCLASS", "WorkerFactoryInformationClass", sizeof(nt64::WORKERFACTORYINFOCLASS)}, {"PVOID", "WorkerFactoryInformation", sizeof(nt64::PVOID)}, {"ULONG", "WorkerFactoryInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetIntervalProfile", 2, {{"ULONG", "Interval", sizeof(nt64::ULONG)}, {"KPROFILE_SOURCE", "Source", sizeof(nt64::KPROFILE_SOURCE)}}},
        {"NtSetIoCompletionEx", 6, {{"HANDLE", "IoCompletionHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "IoCompletionReserveHandle", sizeof(nt64::HANDLE)}, {"PVOID", "KeyContext", sizeof(nt64::PVOID)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"NTSTATUS", "IoStatus", sizeof(nt64::NTSTATUS)}, {"ULONG_PTR", "IoStatusInformation", sizeof(nt64::ULONG_PTR)}}},
        {"NtSetIoCompletion", 5, {{"HANDLE", "IoCompletionHandle", sizeof(nt64::HANDLE)}, {"PVOID", "KeyContext", sizeof(nt64::PVOID)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"NTSTATUS", "IoStatus", sizeof(nt64::NTSTATUS)}, {"ULONG_PTR", "IoStatusInformation", sizeof(nt64::ULONG_PTR)}}},
        {"NtSetLdtEntries", 6, {{"ULONG", "Selector0", sizeof(nt64::ULONG)}, {"ULONG", "Entry0Low", sizeof(nt64::ULONG)}, {"ULONG", "Entry0Hi", sizeof(nt64::ULONG)}, {"ULONG", "Selector1", sizeof(nt64::ULONG)}, {"ULONG", "Entry1Low", sizeof(nt64::ULONG)}, {"ULONG", "Entry1Hi", sizeof(nt64::ULONG)}}},
        {"NtSetLowEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt64::HANDLE)}}},
        {"NtSetLowWaitHighEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt64::HANDLE)}}},
        {"NtSetQuotaInformationFile", 4, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}}},
        {"NtSetSecurityObject", 3, {{"HANDLE", "Handle", sizeof(nt64::HANDLE)}, {"SECURITY_INFORMATION", "SecurityInformation", sizeof(nt64::SECURITY_INFORMATION)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt64::PSECURITY_DESCRIPTOR)}}},
        {"NtSetSystemEnvironmentValueEx", 5, {{"PUNICODE_STRING", "VariableName", sizeof(nt64::PUNICODE_STRING)}, {"LPGUID", "VendorGuid", sizeof(nt64::LPGUID)}, {"PVOID", "Value", sizeof(nt64::PVOID)}, {"ULONG", "ValueLength", sizeof(nt64::ULONG)}, {"ULONG", "Attributes", sizeof(nt64::ULONG)}}},
        {"NtSetSystemEnvironmentValue", 2, {{"PUNICODE_STRING", "VariableName", sizeof(nt64::PUNICODE_STRING)}, {"PUNICODE_STRING", "VariableValue", sizeof(nt64::PUNICODE_STRING)}}},
        {"NtSetSystemInformation", 3, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(nt64::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "SystemInformation", sizeof(nt64::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetSystemPowerState", 3, {{"POWER_ACTION", "SystemAction", sizeof(nt64::POWER_ACTION)}, {"SYSTEM_POWER_STATE", "MinSystemState", sizeof(nt64::SYSTEM_POWER_STATE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}}},
        {"NtSetSystemTime", 2, {{"PLARGE_INTEGER", "SystemTime", sizeof(nt64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "PreviousTime", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtSetThreadExecutionState", 2, {{"EXECUTION_STATE", "esFlags", sizeof(nt64::EXECUTION_STATE)}, {"EXECUTION_STATE", "STARPreviousFlags", sizeof(nt64::EXECUTION_STATE)}}},
        {"NtSetTimerEx", 4, {{"HANDLE", "TimerHandle", sizeof(nt64::HANDLE)}, {"TIMER_SET_INFORMATION_CLASS", "TimerSetInformationClass", sizeof(nt64::TIMER_SET_INFORMATION_CLASS)}, {"PVOID", "TimerSetInformation", sizeof(nt64::PVOID)}, {"ULONG", "TimerSetInformationLength", sizeof(nt64::ULONG)}}},
        {"NtSetTimer", 7, {{"HANDLE", "TimerHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "DueTime", sizeof(nt64::PLARGE_INTEGER)}, {"PTIMER_APC_ROUTINE", "TimerApcRoutine", sizeof(nt64::PTIMER_APC_ROUTINE)}, {"PVOID", "TimerContext", sizeof(nt64::PVOID)}, {"BOOLEAN", "WakeTimer", sizeof(nt64::BOOLEAN)}, {"LONG", "Period", sizeof(nt64::LONG)}, {"PBOOLEAN", "PreviousState", sizeof(nt64::PBOOLEAN)}}},
        {"NtSetTimerResolution", 3, {{"ULONG", "DesiredTime", sizeof(nt64::ULONG)}, {"BOOLEAN", "SetResolution", sizeof(nt64::BOOLEAN)}, {"PULONG", "ActualTime", sizeof(nt64::PULONG)}}},
        {"NtSetUuidSeed", 1, {{"PCHAR", "Seed", sizeof(nt64::PCHAR)}}},
        {"NtSetValueKey", 6, {{"HANDLE", "KeyHandle", sizeof(nt64::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(nt64::PUNICODE_STRING)}, {"ULONG", "TitleIndex", sizeof(nt64::ULONG)}, {"ULONG", "Type", sizeof(nt64::ULONG)}, {"PVOID", "Data", sizeof(nt64::PVOID)}, {"ULONG", "DataSize", sizeof(nt64::ULONG)}}},
        {"NtSetVolumeInformationFile", 5, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "FsInformation", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"FS_INFORMATION_CLASS", "FsInformationClass", sizeof(nt64::FS_INFORMATION_CLASS)}}},
        {"NtShutdownSystem", 1, {{"SHUTDOWN_ACTION", "Action", sizeof(nt64::SHUTDOWN_ACTION)}}},
        {"NtShutdownWorkerFactory", 2, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt64::HANDLE)}, {"LONG", "STARPendingWorkerCount", sizeof(nt64::LONG)}}},
        {"NtSignalAndWaitForSingleObject", 4, {{"HANDLE", "SignalHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "WaitHandle", sizeof(nt64::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(nt64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtSinglePhaseReject", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt64::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtStartProfile", 1, {{"HANDLE", "ProfileHandle", sizeof(nt64::HANDLE)}}},
        {"NtStopProfile", 1, {{"HANDLE", "ProfileHandle", sizeof(nt64::HANDLE)}}},
        {"NtSuspendProcess", 1, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}}},
        {"NtSuspendThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(nt64::PULONG)}}},
        {"NtSystemDebugControl", 6, {{"SYSDBG_COMMAND", "Command", sizeof(nt64::SYSDBG_COMMAND)}, {"PVOID", "InputBuffer", sizeof(nt64::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt64::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt64::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtTerminateJobObject", 2, {{"HANDLE", "JobHandle", sizeof(nt64::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(nt64::NTSTATUS)}}},
        {"NtTerminateProcess", 2, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(nt64::NTSTATUS)}}},
        {"NtTerminateThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt64::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(nt64::NTSTATUS)}}},
        {"NtTraceControl", 6, {{"ULONG", "FunctionCode", sizeof(nt64::ULONG)}, {"PVOID", "InBuffer", sizeof(nt64::PVOID)}, {"ULONG", "InBufferLen", sizeof(nt64::ULONG)}, {"PVOID", "OutBuffer", sizeof(nt64::PVOID)}, {"ULONG", "OutBufferLen", sizeof(nt64::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt64::PULONG)}}},
        {"NtTraceEvent", 4, {{"HANDLE", "TraceHandle", sizeof(nt64::HANDLE)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}, {"ULONG", "FieldSize", sizeof(nt64::ULONG)}, {"PVOID", "Fields", sizeof(nt64::PVOID)}}},
        {"NtTranslateFilePath", 4, {{"PFILE_PATH", "InputFilePath", sizeof(nt64::PFILE_PATH)}, {"ULONG", "OutputType", sizeof(nt64::ULONG)}, {"PFILE_PATH", "OutputFilePath", sizeof(nt64::PFILE_PATH)}, {"PULONG", "OutputFilePathLength", sizeof(nt64::PULONG)}}},
        {"NtUnloadDriver", 1, {{"PUNICODE_STRING", "DriverServiceName", sizeof(nt64::PUNICODE_STRING)}}},
        {"NtUnloadKey2", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt64::ULONG)}}},
        {"NtUnloadKeyEx", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt64::POBJECT_ATTRIBUTES)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}}},
        {"NtUnloadKey", 1, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt64::POBJECT_ATTRIBUTES)}}},
        {"NtUnlockFile", 5, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt64::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "Length", sizeof(nt64::PLARGE_INTEGER)}, {"ULONG", "Key", sizeof(nt64::ULONG)}}},
        {"NtUnlockVirtualMemory", 4, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt64::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt64::PSIZE_T)}, {"ULONG", "MapType", sizeof(nt64::ULONG)}}},
        {"NtUnmapViewOfSection", 2, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}}},
        {"NtVdmControl", 2, {{"VDMSERVICECLASS", "Service", sizeof(nt64::VDMSERVICECLASS)}, {"PVOID", "ServiceData", sizeof(nt64::PVOID)}}},
        {"NtWaitForDebugEvent", 4, {{"HANDLE", "DebugObjectHandle", sizeof(nt64::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(nt64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}, {"PDBGUI_WAIT_STATE_CHANGE", "WaitStateChange", sizeof(nt64::PDBGUI_WAIT_STATE_CHANGE)}}},
        {"NtWaitForKeyedEvent", 4, {{"HANDLE", "KeyedEventHandle", sizeof(nt64::HANDLE)}, {"PVOID", "KeyValue", sizeof(nt64::PVOID)}, {"BOOLEAN", "Alertable", sizeof(nt64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtWaitForMultipleObjects32", 5, {{"ULONG", "Count", sizeof(nt64::ULONG)}, {"LONG", "Handles", sizeof(nt64::LONG)}, {"WAIT_TYPE", "WaitType", sizeof(nt64::WAIT_TYPE)}, {"BOOLEAN", "Alertable", sizeof(nt64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtWaitForMultipleObjects", 5, {{"ULONG", "Count", sizeof(nt64::ULONG)}, {"HANDLE", "Handles", sizeof(nt64::HANDLE)}, {"WAIT_TYPE", "WaitType", sizeof(nt64::WAIT_TYPE)}, {"BOOLEAN", "Alertable", sizeof(nt64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtWaitForSingleObject", 3, {{"HANDLE", "Handle", sizeof(nt64::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(nt64::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt64::PLARGE_INTEGER)}}},
        {"NtWaitForWorkViaWorkerFactory", 2, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt64::HANDLE)}, {"PFILE_IO_COMPLETION_INFORMATION", "MiniPacket", sizeof(nt64::PFILE_IO_COMPLETION_INFORMATION)}}},
        {"NtWaitHighEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt64::HANDLE)}}},
        {"NtWaitLowEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt64::HANDLE)}}},
        {"NtWorkerFactoryWorkerReady", 1, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt64::HANDLE)}}},
        {"NtWriteFileGather", 9, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PFILE_SEGMENT_ELEMENT", "SegmentArray", sizeof(nt64::PFILE_SEGMENT_ELEMENT)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt64::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt64::PULONG)}}},
        {"NtWriteFile", 9, {{"HANDLE", "FileHandle", sizeof(nt64::HANDLE)}, {"HANDLE", "Event", sizeof(nt64::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt64::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt64::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt64::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"ULONG", "Length", sizeof(nt64::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt64::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt64::PULONG)}}},
        {"NtWriteRequestData", 6, {{"HANDLE", "PortHandle", sizeof(nt64::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(nt64::PPORT_MESSAGE)}, {"ULONG", "DataEntryIndex", sizeof(nt64::ULONG)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt64::SIZE_T)}, {"PSIZE_T", "NumberOfBytesWritten", sizeof(nt64::PSIZE_T)}}},
        {"NtWriteVirtualMemory", 5, {{"HANDLE", "ProcessHandle", sizeof(nt64::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt64::PVOID)}, {"PVOID", "Buffer", sizeof(nt64::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt64::SIZE_T)}, {"PSIZE_T", "NumberOfBytesWritten", sizeof(nt64::PSIZE_T)}}},
        {"NtDisableLastKnownGood", 0, {}},
        {"NtEnableLastKnownGood", 0, {}},
        {"NtFlushProcessWriteBuffers", 0, {}},
        {"NtFlushWriteBuffer", 0, {}},
        {"NtGetCurrentProcessorNumber", 0, {}},
        {"NtIsSystemResumeAutomatic", 0, {}},
        {"NtIsUILanguageComitted", 0, {}},
        {"NtQueryPortInformationProcess", 0, {}},
        {"NtSerializeBoot", 0, {}},
        {"NtTestAlert", 0, {}},
        {"NtThawRegistry", 0, {}},
        {"NtThawTransactions", 0, {}},
        {"NtUmsThreadYield", 0, {}},
        {"NtYieldExecution", 0, {}},
	};

    using id_t      = nt64::syscalls::id_t;
    using Listeners = std::multimap<id_t, core::Breakpoint>;
}

struct nt64::syscalls::Data
{
    Data(core::Core& core, std::string_view module);

    core::Core& core;
    std::string module;
    Listeners   listeners;
    uint64_t    last_id;
};

nt64::syscalls::Data::Data(core::Core& core, std::string_view module)
    : core(core)
    , module(module)
    , last_id(0)
{
}

nt64::syscalls::syscalls(core::Core& core, std::string_view module)
    : d_(std::make_unique<Data>(core, module))
{
}

nt64::syscalls::~syscalls() = default;

namespace
{
    using Data = nt64::syscalls::Data;

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

        return nt64::cast_to<T>(*arg);
    }
}

opt<id_t> nt64::syscalls::register_NtAcceptConnectPort(proc_t proc, const on_NtAcceptConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAcceptConnectPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle        = arg<nt64::PHANDLE>(core, 0);
        const auto PortContext       = arg<nt64::PVOID>(core, 1);
        const auto ConnectionRequest = arg<nt64::PPORT_MESSAGE>(core, 2);
        const auto AcceptConnection  = arg<nt64::BOOLEAN>(core, 3);
        const auto ServerView        = arg<nt64::PPORT_VIEW>(core, 4);
        const auto ClientView        = arg<nt64::PREMOTE_PORT_VIEW>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[0]);

        on_func(PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView);
    });
}

opt<id_t> nt64::syscalls::register_NtAccessCheckAndAuditAlarm(proc_t proc, const on_NtAccessCheckAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAccessCheckAndAuditAlarm", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName      = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto HandleId           = arg<nt64::PVOID>(core, 1);
        const auto ObjectTypeName     = arg<nt64::PUNICODE_STRING>(core, 2);
        const auto ObjectName         = arg<nt64::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor = arg<nt64::PSECURITY_DESCRIPTOR>(core, 4);
        const auto DesiredAccess      = arg<nt64::ACCESS_MASK>(core, 5);
        const auto GenericMapping     = arg<nt64::PGENERIC_MAPPING>(core, 6);
        const auto ObjectCreation     = arg<nt64::BOOLEAN>(core, 7);
        const auto GrantedAccess      = arg<nt64::PACCESS_MASK>(core, 8);
        const auto AccessStatus       = arg<nt64::PNTSTATUS>(core, 9);
        const auto GenerateOnClose    = arg<nt64::PBOOLEAN>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[1]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<id_t> nt64::syscalls::register_NtAccessCheckByTypeAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAccessCheckByTypeAndAuditAlarm", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName        = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<nt64::PVOID>(core, 1);
        const auto ObjectTypeName       = arg<nt64::PUNICODE_STRING>(core, 2);
        const auto ObjectName           = arg<nt64::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor   = arg<nt64::PSECURITY_DESCRIPTOR>(core, 4);
        const auto PrincipalSelfSid     = arg<nt64::PSID>(core, 5);
        const auto DesiredAccess        = arg<nt64::ACCESS_MASK>(core, 6);
        const auto AuditType            = arg<nt64::AUDIT_EVENT_TYPE>(core, 7);
        const auto Flags                = arg<nt64::ULONG>(core, 8);
        const auto ObjectTypeList       = arg<nt64::POBJECT_TYPE_LIST>(core, 9);
        const auto ObjectTypeListLength = arg<nt64::ULONG>(core, 10);
        const auto GenericMapping       = arg<nt64::PGENERIC_MAPPING>(core, 11);
        const auto ObjectCreation       = arg<nt64::BOOLEAN>(core, 12);
        const auto GrantedAccess        = arg<nt64::PACCESS_MASK>(core, 13);
        const auto AccessStatus         = arg<nt64::PNTSTATUS>(core, 14);
        const auto GenerateOnClose      = arg<nt64::PBOOLEAN>(core, 15);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[2]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<id_t> nt64::syscalls::register_NtAccessCheckByType(proc_t proc, const on_NtAccessCheckByType_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAccessCheckByType", [=]
    {
        auto& core = d_->core;
        
        const auto SecurityDescriptor   = arg<nt64::PSECURITY_DESCRIPTOR>(core, 0);
        const auto PrincipalSelfSid     = arg<nt64::PSID>(core, 1);
        const auto ClientToken          = arg<nt64::HANDLE>(core, 2);
        const auto DesiredAccess        = arg<nt64::ACCESS_MASK>(core, 3);
        const auto ObjectTypeList       = arg<nt64::POBJECT_TYPE_LIST>(core, 4);
        const auto ObjectTypeListLength = arg<nt64::ULONG>(core, 5);
        const auto GenericMapping       = arg<nt64::PGENERIC_MAPPING>(core, 6);
        const auto PrivilegeSet         = arg<nt64::PPRIVILEGE_SET>(core, 7);
        const auto PrivilegeSetLength   = arg<nt64::PULONG>(core, 8);
        const auto GrantedAccess        = arg<nt64::PACCESS_MASK>(core, 9);
        const auto AccessStatus         = arg<nt64::PNTSTATUS>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[3]);

        on_func(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<id_t> nt64::syscalls::register_NtAccessCheckByTypeResultListAndAuditAlarmByHandle(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAccessCheckByTypeResultListAndAuditAlarmByHandle", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName        = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<nt64::PVOID>(core, 1);
        const auto ClientToken          = arg<nt64::HANDLE>(core, 2);
        const auto ObjectTypeName       = arg<nt64::PUNICODE_STRING>(core, 3);
        const auto ObjectName           = arg<nt64::PUNICODE_STRING>(core, 4);
        const auto SecurityDescriptor   = arg<nt64::PSECURITY_DESCRIPTOR>(core, 5);
        const auto PrincipalSelfSid     = arg<nt64::PSID>(core, 6);
        const auto DesiredAccess        = arg<nt64::ACCESS_MASK>(core, 7);
        const auto AuditType            = arg<nt64::AUDIT_EVENT_TYPE>(core, 8);
        const auto Flags                = arg<nt64::ULONG>(core, 9);
        const auto ObjectTypeList       = arg<nt64::POBJECT_TYPE_LIST>(core, 10);
        const auto ObjectTypeListLength = arg<nt64::ULONG>(core, 11);
        const auto GenericMapping       = arg<nt64::PGENERIC_MAPPING>(core, 12);
        const auto ObjectCreation       = arg<nt64::BOOLEAN>(core, 13);
        const auto GrantedAccess        = arg<nt64::PACCESS_MASK>(core, 14);
        const auto AccessStatus         = arg<nt64::PNTSTATUS>(core, 15);
        const auto GenerateOnClose      = arg<nt64::PBOOLEAN>(core, 16);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[4]);

        on_func(SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<id_t> nt64::syscalls::register_NtAccessCheckByTypeResultListAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAccessCheckByTypeResultListAndAuditAlarm", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName        = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<nt64::PVOID>(core, 1);
        const auto ObjectTypeName       = arg<nt64::PUNICODE_STRING>(core, 2);
        const auto ObjectName           = arg<nt64::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor   = arg<nt64::PSECURITY_DESCRIPTOR>(core, 4);
        const auto PrincipalSelfSid     = arg<nt64::PSID>(core, 5);
        const auto DesiredAccess        = arg<nt64::ACCESS_MASK>(core, 6);
        const auto AuditType            = arg<nt64::AUDIT_EVENT_TYPE>(core, 7);
        const auto Flags                = arg<nt64::ULONG>(core, 8);
        const auto ObjectTypeList       = arg<nt64::POBJECT_TYPE_LIST>(core, 9);
        const auto ObjectTypeListLength = arg<nt64::ULONG>(core, 10);
        const auto GenericMapping       = arg<nt64::PGENERIC_MAPPING>(core, 11);
        const auto ObjectCreation       = arg<nt64::BOOLEAN>(core, 12);
        const auto GrantedAccess        = arg<nt64::PACCESS_MASK>(core, 13);
        const auto AccessStatus         = arg<nt64::PNTSTATUS>(core, 14);
        const auto GenerateOnClose      = arg<nt64::PBOOLEAN>(core, 15);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[5]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<id_t> nt64::syscalls::register_NtAccessCheckByTypeResultList(proc_t proc, const on_NtAccessCheckByTypeResultList_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAccessCheckByTypeResultList", [=]
    {
        auto& core = d_->core;
        
        const auto SecurityDescriptor   = arg<nt64::PSECURITY_DESCRIPTOR>(core, 0);
        const auto PrincipalSelfSid     = arg<nt64::PSID>(core, 1);
        const auto ClientToken          = arg<nt64::HANDLE>(core, 2);
        const auto DesiredAccess        = arg<nt64::ACCESS_MASK>(core, 3);
        const auto ObjectTypeList       = arg<nt64::POBJECT_TYPE_LIST>(core, 4);
        const auto ObjectTypeListLength = arg<nt64::ULONG>(core, 5);
        const auto GenericMapping       = arg<nt64::PGENERIC_MAPPING>(core, 6);
        const auto PrivilegeSet         = arg<nt64::PPRIVILEGE_SET>(core, 7);
        const auto PrivilegeSetLength   = arg<nt64::PULONG>(core, 8);
        const auto GrantedAccess        = arg<nt64::PACCESS_MASK>(core, 9);
        const auto AccessStatus         = arg<nt64::PNTSTATUS>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[6]);

        on_func(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<id_t> nt64::syscalls::register_NtAccessCheck(proc_t proc, const on_NtAccessCheck_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAccessCheck", [=]
    {
        auto& core = d_->core;
        
        const auto SecurityDescriptor = arg<nt64::PSECURITY_DESCRIPTOR>(core, 0);
        const auto ClientToken        = arg<nt64::HANDLE>(core, 1);
        const auto DesiredAccess      = arg<nt64::ACCESS_MASK>(core, 2);
        const auto GenericMapping     = arg<nt64::PGENERIC_MAPPING>(core, 3);
        const auto PrivilegeSet       = arg<nt64::PPRIVILEGE_SET>(core, 4);
        const auto PrivilegeSetLength = arg<nt64::PULONG>(core, 5);
        const auto GrantedAccess      = arg<nt64::PACCESS_MASK>(core, 6);
        const auto AccessStatus       = arg<nt64::PNTSTATUS>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[7]);

        on_func(SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<id_t> nt64::syscalls::register_NtAddAtom(proc_t proc, const on_NtAddAtom_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAddAtom", [=]
    {
        auto& core = d_->core;
        
        const auto AtomName = arg<nt64::PWSTR>(core, 0);
        const auto Length   = arg<nt64::ULONG>(core, 1);
        const auto Atom     = arg<nt64::PRTL_ATOM>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[8]);

        on_func(AtomName, Length, Atom);
    });
}

opt<id_t> nt64::syscalls::register_NtAddBootEntry(proc_t proc, const on_NtAddBootEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAddBootEntry", [=]
    {
        auto& core = d_->core;
        
        const auto BootEntry = arg<nt64::PBOOT_ENTRY>(core, 0);
        const auto Id        = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[9]);

        on_func(BootEntry, Id);
    });
}

opt<id_t> nt64::syscalls::register_NtAddDriverEntry(proc_t proc, const on_NtAddDriverEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAddDriverEntry", [=]
    {
        auto& core = d_->core;
        
        const auto DriverEntry = arg<nt64::PEFI_DRIVER_ENTRY>(core, 0);
        const auto Id          = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[10]);

        on_func(DriverEntry, Id);
    });
}

opt<id_t> nt64::syscalls::register_NtAdjustGroupsToken(proc_t proc, const on_NtAdjustGroupsToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAdjustGroupsToken", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle    = arg<nt64::HANDLE>(core, 0);
        const auto ResetToDefault = arg<nt64::BOOLEAN>(core, 1);
        const auto NewState       = arg<nt64::PTOKEN_GROUPS>(core, 2);
        const auto BufferLength   = arg<nt64::ULONG>(core, 3);
        const auto PreviousState  = arg<nt64::PTOKEN_GROUPS>(core, 4);
        const auto ReturnLength   = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[11]);

        on_func(TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtAdjustPrivilegesToken(proc_t proc, const on_NtAdjustPrivilegesToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAdjustPrivilegesToken", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle          = arg<nt64::HANDLE>(core, 0);
        const auto DisableAllPrivileges = arg<nt64::BOOLEAN>(core, 1);
        const auto NewState             = arg<nt64::PTOKEN_PRIVILEGES>(core, 2);
        const auto BufferLength         = arg<nt64::ULONG>(core, 3);
        const auto PreviousState        = arg<nt64::PTOKEN_PRIVILEGES>(core, 4);
        const auto ReturnLength         = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[12]);

        on_func(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtAlertResumeThread(proc_t proc, const on_NtAlertResumeThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlertResumeThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle         = arg<nt64::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[13]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<id_t> nt64::syscalls::register_NtAlertThread(proc_t proc, const on_NtAlertThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlertThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[14]);

        on_func(ThreadHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtAllocateLocallyUniqueId(proc_t proc, const on_NtAllocateLocallyUniqueId_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAllocateLocallyUniqueId", [=]
    {
        auto& core = d_->core;
        
        const auto Luid = arg<nt64::PLUID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[15]);

        on_func(Luid);
    });
}

opt<id_t> nt64::syscalls::register_NtAllocateReserveObject(proc_t proc, const on_NtAllocateReserveObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAllocateReserveObject", [=]
    {
        auto& core = d_->core;
        
        const auto MemoryReserveHandle = arg<nt64::PHANDLE>(core, 0);
        const auto ObjectAttributes    = arg<nt64::POBJECT_ATTRIBUTES>(core, 1);
        const auto Type                = arg<nt64::MEMORY_RESERVE_TYPE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[16]);

        on_func(MemoryReserveHandle, ObjectAttributes, Type);
    });
}

opt<id_t> nt64::syscalls::register_NtAllocateUserPhysicalPages(proc_t proc, const on_NtAllocateUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAllocateUserPhysicalPages", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 0);
        const auto NumberOfPages = arg<nt64::PULONG_PTR>(core, 1);
        const auto UserPfnArra   = arg<nt64::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[17]);

        on_func(ProcessHandle, NumberOfPages, UserPfnArra);
    });
}

opt<id_t> nt64::syscalls::register_NtAllocateUuids(proc_t proc, const on_NtAllocateUuids_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAllocateUuids", [=]
    {
        auto& core = d_->core;
        
        const auto Time     = arg<nt64::PULARGE_INTEGER>(core, 0);
        const auto Range    = arg<nt64::PULONG>(core, 1);
        const auto Sequence = arg<nt64::PULONG>(core, 2);
        const auto Seed     = arg<nt64::PCHAR>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[18]);

        on_func(Time, Range, Sequence, Seed);
    });
}

opt<id_t> nt64::syscalls::register_NtAllocateVirtualMemory(proc_t proc, const on_NtAllocateVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAllocateVirtualMemory", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt64::PVOID>(core, 1);
        const auto ZeroBits        = arg<nt64::ULONG_PTR>(core, 2);
        const auto RegionSize      = arg<nt64::PSIZE_T>(core, 3);
        const auto AllocationType  = arg<nt64::ULONG>(core, 4);
        const auto Protect         = arg<nt64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[19]);

        on_func(ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcAcceptConnectPort(proc_t proc, const on_NtAlpcAcceptConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcAcceptConnectPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle                  = arg<nt64::PHANDLE>(core, 0);
        const auto ConnectionPortHandle        = arg<nt64::HANDLE>(core, 1);
        const auto Flags                       = arg<nt64::ULONG>(core, 2);
        const auto ObjectAttributes            = arg<nt64::POBJECT_ATTRIBUTES>(core, 3);
        const auto PortAttributes              = arg<nt64::PALPC_PORT_ATTRIBUTES>(core, 4);
        const auto PortContext                 = arg<nt64::PVOID>(core, 5);
        const auto ConnectionRequest           = arg<nt64::PPORT_MESSAGE>(core, 6);
        const auto ConnectionMessageAttributes = arg<nt64::PALPC_MESSAGE_ATTRIBUTES>(core, 7);
        const auto AcceptConnection            = arg<nt64::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[20]);

        on_func(PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcCancelMessage(proc_t proc, const on_NtAlpcCancelMessage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcCancelMessage", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle     = arg<nt64::HANDLE>(core, 0);
        const auto Flags          = arg<nt64::ULONG>(core, 1);
        const auto MessageContext = arg<nt64::PALPC_CONTEXT_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[21]);

        on_func(PortHandle, Flags, MessageContext);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcConnectPort(proc_t proc, const on_NtAlpcConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcConnectPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle           = arg<nt64::PHANDLE>(core, 0);
        const auto PortName             = arg<nt64::PUNICODE_STRING>(core, 1);
        const auto ObjectAttributes     = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto PortAttributes       = arg<nt64::PALPC_PORT_ATTRIBUTES>(core, 3);
        const auto Flags                = arg<nt64::ULONG>(core, 4);
        const auto RequiredServerSid    = arg<nt64::PSID>(core, 5);
        const auto ConnectionMessage    = arg<nt64::PPORT_MESSAGE>(core, 6);
        const auto BufferLength         = arg<nt64::PULONG>(core, 7);
        const auto OutMessageAttributes = arg<nt64::PALPC_MESSAGE_ATTRIBUTES>(core, 8);
        const auto InMessageAttributes  = arg<nt64::PALPC_MESSAGE_ATTRIBUTES>(core, 9);
        const auto Timeout              = arg<nt64::PLARGE_INTEGER>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[22]);

        on_func(PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcCreatePort(proc_t proc, const on_NtAlpcCreatePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcCreatePort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle       = arg<nt64::PHANDLE>(core, 0);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 1);
        const auto PortAttributes   = arg<nt64::PALPC_PORT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[23]);

        on_func(PortHandle, ObjectAttributes, PortAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcCreatePortSection(proc_t proc, const on_NtAlpcCreatePortSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcCreatePortSection", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle        = arg<nt64::HANDLE>(core, 0);
        const auto Flags             = arg<nt64::ULONG>(core, 1);
        const auto SectionHandle     = arg<nt64::HANDLE>(core, 2);
        const auto SectionSize       = arg<nt64::SIZE_T>(core, 3);
        const auto AlpcSectionHandle = arg<nt64::PALPC_HANDLE>(core, 4);
        const auto ActualSectionSize = arg<nt64::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[24]);

        on_func(PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcCreateResourceReserve(proc_t proc, const on_NtAlpcCreateResourceReserve_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcCreateResourceReserve", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle  = arg<nt64::HANDLE>(core, 0);
        const auto Flags       = arg<nt64::ULONG>(core, 1);
        const auto MessageSize = arg<nt64::SIZE_T>(core, 2);
        const auto ResourceId  = arg<nt64::PALPC_HANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[25]);

        on_func(PortHandle, Flags, MessageSize, ResourceId);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcCreateSectionView(proc_t proc, const on_NtAlpcCreateSectionView_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcCreateSectionView", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle     = arg<nt64::HANDLE>(core, 0);
        const auto Flags          = arg<nt64::ULONG>(core, 1);
        const auto ViewAttributes = arg<nt64::PALPC_DATA_VIEW_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[26]);

        on_func(PortHandle, Flags, ViewAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcCreateSecurityContext(proc_t proc, const on_NtAlpcCreateSecurityContext_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcCreateSecurityContext", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle        = arg<nt64::HANDLE>(core, 0);
        const auto Flags             = arg<nt64::ULONG>(core, 1);
        const auto SecurityAttribute = arg<nt64::PALPC_SECURITY_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[27]);

        on_func(PortHandle, Flags, SecurityAttribute);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcDeletePortSection(proc_t proc, const on_NtAlpcDeletePortSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcDeletePortSection", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle    = arg<nt64::HANDLE>(core, 0);
        const auto Flags         = arg<nt64::ULONG>(core, 1);
        const auto SectionHandle = arg<nt64::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[28]);

        on_func(PortHandle, Flags, SectionHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcDeleteResourceReserve(proc_t proc, const on_NtAlpcDeleteResourceReserve_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcDeleteResourceReserve", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt64::HANDLE>(core, 0);
        const auto Flags      = arg<nt64::ULONG>(core, 1);
        const auto ResourceId = arg<nt64::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[29]);

        on_func(PortHandle, Flags, ResourceId);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcDeleteSectionView(proc_t proc, const on_NtAlpcDeleteSectionView_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcDeleteSectionView", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt64::HANDLE>(core, 0);
        const auto Flags      = arg<nt64::ULONG>(core, 1);
        const auto ViewBase   = arg<nt64::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[30]);

        on_func(PortHandle, Flags, ViewBase);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcDeleteSecurityContext(proc_t proc, const on_NtAlpcDeleteSecurityContext_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcDeleteSecurityContext", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle    = arg<nt64::HANDLE>(core, 0);
        const auto Flags         = arg<nt64::ULONG>(core, 1);
        const auto ContextHandle = arg<nt64::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[31]);

        on_func(PortHandle, Flags, ContextHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcDisconnectPort(proc_t proc, const on_NtAlpcDisconnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcDisconnectPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt64::HANDLE>(core, 0);
        const auto Flags      = arg<nt64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[32]);

        on_func(PortHandle, Flags);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcImpersonateClientOfPort(proc_t proc, const on_NtAlpcImpersonateClientOfPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcImpersonateClientOfPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle  = arg<nt64::HANDLE>(core, 0);
        const auto PortMessage = arg<nt64::PPORT_MESSAGE>(core, 1);
        const auto Reserved    = arg<nt64::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[33]);

        on_func(PortHandle, PortMessage, Reserved);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcOpenSenderProcess(proc_t proc, const on_NtAlpcOpenSenderProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcOpenSenderProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt64::PHANDLE>(core, 0);
        const auto PortHandle       = arg<nt64::HANDLE>(core, 1);
        const auto PortMessage      = arg<nt64::PPORT_MESSAGE>(core, 2);
        const auto Flags            = arg<nt64::ULONG>(core, 3);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 4);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[34]);

        on_func(ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcOpenSenderThread(proc_t proc, const on_NtAlpcOpenSenderThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcOpenSenderThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle     = arg<nt64::PHANDLE>(core, 0);
        const auto PortHandle       = arg<nt64::HANDLE>(core, 1);
        const auto PortMessage      = arg<nt64::PPORT_MESSAGE>(core, 2);
        const auto Flags            = arg<nt64::ULONG>(core, 3);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 4);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[35]);

        on_func(ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcQueryInformation(proc_t proc, const on_NtAlpcQueryInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcQueryInformation", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle           = arg<nt64::HANDLE>(core, 0);
        const auto PortInformationClass = arg<nt64::ALPC_PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<nt64::PVOID>(core, 2);
        const auto Length               = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength         = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[36]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcQueryInformationMessage(proc_t proc, const on_NtAlpcQueryInformationMessage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcQueryInformationMessage", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle              = arg<nt64::HANDLE>(core, 0);
        const auto PortMessage             = arg<nt64::PPORT_MESSAGE>(core, 1);
        const auto MessageInformationClass = arg<nt64::ALPC_MESSAGE_INFORMATION_CLASS>(core, 2);
        const auto MessageInformation      = arg<nt64::PVOID>(core, 3);
        const auto Length                  = arg<nt64::ULONG>(core, 4);
        const auto ReturnLength            = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[37]);

        on_func(PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcRevokeSecurityContext(proc_t proc, const on_NtAlpcRevokeSecurityContext_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcRevokeSecurityContext", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle    = arg<nt64::HANDLE>(core, 0);
        const auto Flags         = arg<nt64::ULONG>(core, 1);
        const auto ContextHandle = arg<nt64::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[38]);

        on_func(PortHandle, Flags, ContextHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcSendWaitReceivePort(proc_t proc, const on_NtAlpcSendWaitReceivePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcSendWaitReceivePort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle               = arg<nt64::HANDLE>(core, 0);
        const auto Flags                    = arg<nt64::ULONG>(core, 1);
        const auto SendMessage              = arg<nt64::PPORT_MESSAGE>(core, 2);
        const auto SendMessageAttributes    = arg<nt64::PALPC_MESSAGE_ATTRIBUTES>(core, 3);
        const auto ReceiveMessage           = arg<nt64::PPORT_MESSAGE>(core, 4);
        const auto BufferLength             = arg<nt64::PULONG>(core, 5);
        const auto ReceiveMessageAttributes = arg<nt64::PALPC_MESSAGE_ATTRIBUTES>(core, 6);
        const auto Timeout                  = arg<nt64::PLARGE_INTEGER>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[39]);

        on_func(PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout);
    });
}

opt<id_t> nt64::syscalls::register_NtAlpcSetInformation(proc_t proc, const on_NtAlpcSetInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAlpcSetInformation", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle           = arg<nt64::HANDLE>(core, 0);
        const auto PortInformationClass = arg<nt64::ALPC_PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<nt64::PVOID>(core, 2);
        const auto Length               = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[40]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length);
    });
}

opt<id_t> nt64::syscalls::register_NtApphelpCacheControl(proc_t proc, const on_NtApphelpCacheControl_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtApphelpCacheControl", [=]
    {
        auto& core = d_->core;
        
        const auto type = arg<nt64::APPHELPCOMMAND>(core, 0);
        const auto buf  = arg<nt64::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[41]);

        on_func(type, buf);
    });
}

opt<id_t> nt64::syscalls::register_NtAreMappedFilesTheSame(proc_t proc, const on_NtAreMappedFilesTheSame_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAreMappedFilesTheSame", [=]
    {
        auto& core = d_->core;
        
        const auto File1MappedAsAnImage = arg<nt64::PVOID>(core, 0);
        const auto File2MappedAsFile    = arg<nt64::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[42]);

        on_func(File1MappedAsAnImage, File2MappedAsFile);
    });
}

opt<id_t> nt64::syscalls::register_NtAssignProcessToJobObject(proc_t proc, const on_NtAssignProcessToJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtAssignProcessToJobObject", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle     = arg<nt64::HANDLE>(core, 0);
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[43]);

        on_func(JobHandle, ProcessHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtCancelIoFileEx(proc_t proc, const on_NtCancelIoFileEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCancelIoFileEx", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle        = arg<nt64::HANDLE>(core, 0);
        const auto IoRequestToCancel = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto IoStatusBlock     = arg<nt64::PIO_STATUS_BLOCK>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[44]);

        on_func(FileHandle, IoRequestToCancel, IoStatusBlock);
    });
}

opt<id_t> nt64::syscalls::register_NtCancelIoFile(proc_t proc, const on_NtCancelIoFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCancelIoFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt64::PIO_STATUS_BLOCK>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[45]);

        on_func(FileHandle, IoStatusBlock);
    });
}

opt<id_t> nt64::syscalls::register_NtCancelSynchronousIoFile(proc_t proc, const on_NtCancelSynchronousIoFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCancelSynchronousIoFile", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle      = arg<nt64::HANDLE>(core, 0);
        const auto IoRequestToCancel = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto IoStatusBlock     = arg<nt64::PIO_STATUS_BLOCK>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[46]);

        on_func(ThreadHandle, IoRequestToCancel, IoStatusBlock);
    });
}

opt<id_t> nt64::syscalls::register_NtCancelTimer(proc_t proc, const on_NtCancelTimer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCancelTimer", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle  = arg<nt64::HANDLE>(core, 0);
        const auto CurrentState = arg<nt64::PBOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[47]);

        on_func(TimerHandle, CurrentState);
    });
}

opt<id_t> nt64::syscalls::register_NtClearEvent(proc_t proc, const on_NtClearEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtClearEvent", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[48]);

        on_func(EventHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtClose(proc_t proc, const on_NtClose_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtClose", [=]
    {
        auto& core = d_->core;
        
        const auto Handle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[49]);

        on_func(Handle);
    });
}

opt<id_t> nt64::syscalls::register_NtCloseObjectAuditAlarm(proc_t proc, const on_NtCloseObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCloseObjectAuditAlarm", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName   = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto HandleId        = arg<nt64::PVOID>(core, 1);
        const auto GenerateOnClose = arg<nt64::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[50]);

        on_func(SubsystemName, HandleId, GenerateOnClose);
    });
}

opt<id_t> nt64::syscalls::register_NtCommitComplete(proc_t proc, const on_NtCommitComplete_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCommitComplete", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[51]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtCommitEnlistment(proc_t proc, const on_NtCommitEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCommitEnlistment", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[52]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtCommitTransaction(proc_t proc, const on_NtCommitTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCommitTransaction", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle = arg<nt64::HANDLE>(core, 0);
        const auto Wait              = arg<nt64::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[53]);

        on_func(TransactionHandle, Wait);
    });
}

opt<id_t> nt64::syscalls::register_NtCompactKeys(proc_t proc, const on_NtCompactKeys_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCompactKeys", [=]
    {
        auto& core = d_->core;
        
        const auto Count    = arg<nt64::ULONG>(core, 0);
        const auto KeyArray = arg<nt64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[54]);

        on_func(Count, KeyArray);
    });
}

opt<id_t> nt64::syscalls::register_NtCompareTokens(proc_t proc, const on_NtCompareTokens_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCompareTokens", [=]
    {
        auto& core = d_->core;
        
        const auto FirstTokenHandle  = arg<nt64::HANDLE>(core, 0);
        const auto SecondTokenHandle = arg<nt64::HANDLE>(core, 1);
        const auto Equal             = arg<nt64::PBOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[55]);

        on_func(FirstTokenHandle, SecondTokenHandle, Equal);
    });
}

opt<id_t> nt64::syscalls::register_NtCompleteConnectPort(proc_t proc, const on_NtCompleteConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCompleteConnectPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[56]);

        on_func(PortHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtCompressKey(proc_t proc, const on_NtCompressKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCompressKey", [=]
    {
        auto& core = d_->core;
        
        const auto Key = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[57]);

        on_func(Key);
    });
}

opt<id_t> nt64::syscalls::register_NtConnectPort(proc_t proc, const on_NtConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtConnectPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle                  = arg<nt64::PHANDLE>(core, 0);
        const auto PortName                    = arg<nt64::PUNICODE_STRING>(core, 1);
        const auto SecurityQos                 = arg<nt64::PSECURITY_QUALITY_OF_SERVICE>(core, 2);
        const auto ClientView                  = arg<nt64::PPORT_VIEW>(core, 3);
        const auto ServerView                  = arg<nt64::PREMOTE_PORT_VIEW>(core, 4);
        const auto MaxMessageLength            = arg<nt64::PULONG>(core, 5);
        const auto ConnectionInformation       = arg<nt64::PVOID>(core, 6);
        const auto ConnectionInformationLength = arg<nt64::PULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[58]);

        on_func(PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtContinue(proc_t proc, const on_NtContinue_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtContinue", [=]
    {
        auto& core = d_->core;
        
        const auto ContextRecord = arg<nt64::PCONTEXT>(core, 0);
        const auto TestAlert     = arg<nt64::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[59]);

        on_func(ContextRecord, TestAlert);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateDebugObject(proc_t proc, const on_NtCreateDebugObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateDebugObject", [=]
    {
        auto& core = d_->core;
        
        const auto DebugObjectHandle = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Flags             = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[60]);

        on_func(DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateDirectoryObject(proc_t proc, const on_NtCreateDirectoryObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateDirectoryObject", [=]
    {
        auto& core = d_->core;
        
        const auto DirectoryHandle  = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[61]);

        on_func(DirectoryHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateEnlistment(proc_t proc, const on_NtCreateEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateEnlistment", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle      = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ResourceManagerHandle = arg<nt64::HANDLE>(core, 2);
        const auto TransactionHandle     = arg<nt64::HANDLE>(core, 3);
        const auto ObjectAttributes      = arg<nt64::POBJECT_ATTRIBUTES>(core, 4);
        const auto CreateOptions         = arg<nt64::ULONG>(core, 5);
        const auto NotificationMask      = arg<nt64::NOTIFICATION_MASK>(core, 6);
        const auto EnlistmentKey         = arg<nt64::PVOID>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[62]);

        on_func(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateEvent(proc_t proc, const on_NtCreateEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateEvent", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle      = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto EventType        = arg<nt64::EVENT_TYPE>(core, 3);
        const auto InitialState     = arg<nt64::BOOLEAN>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[63]);

        on_func(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateEventPair(proc_t proc, const on_NtCreateEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateEventPair", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle  = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[64]);

        on_func(EventPairHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateFile(proc_t proc, const on_NtCreateFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle        = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock     = arg<nt64::PIO_STATUS_BLOCK>(core, 3);
        const auto AllocationSize    = arg<nt64::PLARGE_INTEGER>(core, 4);
        const auto FileAttributes    = arg<nt64::ULONG>(core, 5);
        const auto ShareAccess       = arg<nt64::ULONG>(core, 6);
        const auto CreateDisposition = arg<nt64::ULONG>(core, 7);
        const auto CreateOptions     = arg<nt64::ULONG>(core, 8);
        const auto EaBuffer          = arg<nt64::PVOID>(core, 9);
        const auto EaLength          = arg<nt64::ULONG>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[65]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateIoCompletion(proc_t proc, const on_NtCreateIoCompletion_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateIoCompletion", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Count              = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[66]);

        on_func(IoCompletionHandle, DesiredAccess, ObjectAttributes, Count);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateJobObject(proc_t proc, const on_NtCreateJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateJobObject", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle        = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[67]);

        on_func(JobHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateJobSet(proc_t proc, const on_NtCreateJobSet_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateJobSet", [=]
    {
        auto& core = d_->core;
        
        const auto NumJob     = arg<nt64::ULONG>(core, 0);
        const auto UserJobSet = arg<nt64::PJOB_SET_ARRAY>(core, 1);
        const auto Flags      = arg<nt64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[68]);

        on_func(NumJob, UserJobSet, Flags);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateKeyedEvent(proc_t proc, const on_NtCreateKeyedEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateKeyedEvent", [=]
    {
        auto& core = d_->core;
        
        const auto KeyedEventHandle = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Flags            = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[69]);

        on_func(KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateKey(proc_t proc, const on_NtCreateKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle        = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto TitleIndex       = arg<nt64::ULONG>(core, 3);
        const auto Class            = arg<nt64::PUNICODE_STRING>(core, 4);
        const auto CreateOptions    = arg<nt64::ULONG>(core, 5);
        const auto Disposition      = arg<nt64::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[70]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateKeyTransacted(proc_t proc, const on_NtCreateKeyTransacted_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateKeyTransacted", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle         = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto TitleIndex        = arg<nt64::ULONG>(core, 3);
        const auto Class             = arg<nt64::PUNICODE_STRING>(core, 4);
        const auto CreateOptions     = arg<nt64::ULONG>(core, 5);
        const auto TransactionHandle = arg<nt64::HANDLE>(core, 6);
        const auto Disposition       = arg<nt64::PULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[71]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateMailslotFile(proc_t proc, const on_NtCreateMailslotFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateMailslotFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle         = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt64::ULONG>(core, 1);
        const auto ObjectAttributes   = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock      = arg<nt64::PIO_STATUS_BLOCK>(core, 3);
        const auto CreateOptions      = arg<nt64::ULONG>(core, 4);
        const auto MailslotQuota      = arg<nt64::ULONG>(core, 5);
        const auto MaximumMessageSize = arg<nt64::ULONG>(core, 6);
        const auto ReadTimeout        = arg<nt64::PLARGE_INTEGER>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[72]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateMutant(proc_t proc, const on_NtCreateMutant_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateMutant", [=]
    {
        auto& core = d_->core;
        
        const auto MutantHandle     = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto InitialOwner     = arg<nt64::BOOLEAN>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[73]);

        on_func(MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateNamedPipeFile(proc_t proc, const on_NtCreateNamedPipeFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateNamedPipeFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle        = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt64::ULONG>(core, 1);
        const auto ObjectAttributes  = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock     = arg<nt64::PIO_STATUS_BLOCK>(core, 3);
        const auto ShareAccess       = arg<nt64::ULONG>(core, 4);
        const auto CreateDisposition = arg<nt64::ULONG>(core, 5);
        const auto CreateOptions     = arg<nt64::ULONG>(core, 6);
        const auto NamedPipeType     = arg<nt64::ULONG>(core, 7);
        const auto ReadMode          = arg<nt64::ULONG>(core, 8);
        const auto CompletionMode    = arg<nt64::ULONG>(core, 9);
        const auto MaximumInstances  = arg<nt64::ULONG>(core, 10);
        const auto InboundQuota      = arg<nt64::ULONG>(core, 11);
        const auto OutboundQuota     = arg<nt64::ULONG>(core, 12);
        const auto DefaultTimeout    = arg<nt64::PLARGE_INTEGER>(core, 13);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[74]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);
    });
}

opt<id_t> nt64::syscalls::register_NtCreatePagingFile(proc_t proc, const on_NtCreatePagingFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreatePagingFile", [=]
    {
        auto& core = d_->core;
        
        const auto PageFileName = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto MinimumSize  = arg<nt64::PLARGE_INTEGER>(core, 1);
        const auto MaximumSize  = arg<nt64::PLARGE_INTEGER>(core, 2);
        const auto Priority     = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[75]);

        on_func(PageFileName, MinimumSize, MaximumSize, Priority);
    });
}

opt<id_t> nt64::syscalls::register_NtCreatePort(proc_t proc, const on_NtCreatePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreatePort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle              = arg<nt64::PHANDLE>(core, 0);
        const auto ObjectAttributes        = arg<nt64::POBJECT_ATTRIBUTES>(core, 1);
        const auto MaxConnectionInfoLength = arg<nt64::ULONG>(core, 2);
        const auto MaxMessageLength        = arg<nt64::ULONG>(core, 3);
        const auto MaxPoolUsage            = arg<nt64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[76]);

        on_func(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    });
}

opt<id_t> nt64::syscalls::register_NtCreatePrivateNamespace(proc_t proc, const on_NtCreatePrivateNamespace_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreatePrivateNamespace", [=]
    {
        auto& core = d_->core;
        
        const auto NamespaceHandle    = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto BoundaryDescriptor = arg<nt64::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[77]);

        on_func(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateProcessEx(proc_t proc, const on_NtCreateProcessEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateProcessEx", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ParentProcess    = arg<nt64::HANDLE>(core, 3);
        const auto Flags            = arg<nt64::ULONG>(core, 4);
        const auto SectionHandle    = arg<nt64::HANDLE>(core, 5);
        const auto DebugPort        = arg<nt64::HANDLE>(core, 6);
        const auto ExceptionPort    = arg<nt64::HANDLE>(core, 7);
        const auto JobMemberLevel   = arg<nt64::ULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[78]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateProcess(proc_t proc, const on_NtCreateProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle      = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ParentProcess      = arg<nt64::HANDLE>(core, 3);
        const auto InheritObjectTable = arg<nt64::BOOLEAN>(core, 4);
        const auto SectionHandle      = arg<nt64::HANDLE>(core, 5);
        const auto DebugPort          = arg<nt64::HANDLE>(core, 6);
        const auto ExceptionPort      = arg<nt64::HANDLE>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[79]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateProfileEx(proc_t proc, const on_NtCreateProfileEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateProfileEx", [=]
    {
        auto& core = d_->core;
        
        const auto ProfileHandle      = arg<nt64::PHANDLE>(core, 0);
        const auto Process            = arg<nt64::HANDLE>(core, 1);
        const auto ProfileBase        = arg<nt64::PVOID>(core, 2);
        const auto ProfileSize        = arg<nt64::SIZE_T>(core, 3);
        const auto BucketSize         = arg<nt64::ULONG>(core, 4);
        const auto Buffer             = arg<nt64::PULONG>(core, 5);
        const auto BufferSize         = arg<nt64::ULONG>(core, 6);
        const auto ProfileSource      = arg<nt64::KPROFILE_SOURCE>(core, 7);
        const auto GroupAffinityCount = arg<nt64::ULONG>(core, 8);
        const auto GroupAffinity      = arg<nt64::PGROUP_AFFINITY>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[80]);

        on_func(ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateProfile(proc_t proc, const on_NtCreateProfile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateProfile", [=]
    {
        auto& core = d_->core;
        
        const auto ProfileHandle = arg<nt64::PHANDLE>(core, 0);
        const auto Process       = arg<nt64::HANDLE>(core, 1);
        const auto RangeBase     = arg<nt64::PVOID>(core, 2);
        const auto RangeSize     = arg<nt64::SIZE_T>(core, 3);
        const auto BucketSize    = arg<nt64::ULONG>(core, 4);
        const auto Buffer        = arg<nt64::PULONG>(core, 5);
        const auto BufferSize    = arg<nt64::ULONG>(core, 6);
        const auto ProfileSource = arg<nt64::KPROFILE_SOURCE>(core, 7);
        const auto Affinity      = arg<nt64::KAFFINITY>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[81]);

        on_func(ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateResourceManager(proc_t proc, const on_NtCreateResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateResourceManager", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt64::ACCESS_MASK>(core, 1);
        const auto TmHandle              = arg<nt64::HANDLE>(core, 2);
        const auto RmGuid                = arg<nt64::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<nt64::POBJECT_ATTRIBUTES>(core, 4);
        const auto CreateOptions         = arg<nt64::ULONG>(core, 5);
        const auto Description           = arg<nt64::PUNICODE_STRING>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[82]);

        on_func(ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateSection(proc_t proc, const on_NtCreateSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateSection", [=]
    {
        auto& core = d_->core;
        
        const auto SectionHandle         = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes      = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto MaximumSize           = arg<nt64::PLARGE_INTEGER>(core, 3);
        const auto SectionPageProtection = arg<nt64::ULONG>(core, 4);
        const auto AllocationAttributes  = arg<nt64::ULONG>(core, 5);
        const auto FileHandle            = arg<nt64::HANDLE>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[83]);

        on_func(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateSemaphore(proc_t proc, const on_NtCreateSemaphore_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateSemaphore", [=]
    {
        auto& core = d_->core;
        
        const auto SemaphoreHandle  = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto InitialCount     = arg<nt64::LONG>(core, 3);
        const auto MaximumCount     = arg<nt64::LONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[84]);

        on_func(SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateSymbolicLinkObject(proc_t proc, const on_NtCreateSymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateSymbolicLinkObject", [=]
    {
        auto& core = d_->core;
        
        const auto LinkHandle       = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto LinkTarget       = arg<nt64::PUNICODE_STRING>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[85]);

        on_func(LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateThreadEx(proc_t proc, const on_NtCreateThreadEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateThreadEx", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle     = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ProcessHandle    = arg<nt64::HANDLE>(core, 3);
        const auto StartRoutine     = arg<nt64::PVOID>(core, 4);
        const auto Argument         = arg<nt64::PVOID>(core, 5);
        const auto CreateFlags      = arg<nt64::ULONG>(core, 6);
        const auto ZeroBits         = arg<nt64::ULONG_PTR>(core, 7);
        const auto StackSize        = arg<nt64::SIZE_T>(core, 8);
        const auto MaximumStackSize = arg<nt64::SIZE_T>(core, 9);
        const auto AttributeList    = arg<nt64::PPS_ATTRIBUTE_LIST>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[86]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateThread(proc_t proc, const on_NtCreateThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle     = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ProcessHandle    = arg<nt64::HANDLE>(core, 3);
        const auto ClientId         = arg<nt64::PCLIENT_ID>(core, 4);
        const auto ThreadContext    = arg<nt64::PCONTEXT>(core, 5);
        const auto InitialTeb       = arg<nt64::PINITIAL_TEB>(core, 6);
        const auto CreateSuspended  = arg<nt64::BOOLEAN>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[87]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateTimer(proc_t proc, const on_NtCreateTimer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateTimer", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle      = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto TimerType        = arg<nt64::TIMER_TYPE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[88]);

        on_func(TimerHandle, DesiredAccess, ObjectAttributes, TimerType);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateToken(proc_t proc, const on_NtCreateToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateToken", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle      = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto TokenType        = arg<nt64::TOKEN_TYPE>(core, 3);
        const auto AuthenticationId = arg<nt64::PLUID>(core, 4);
        const auto ExpirationTime   = arg<nt64::PLARGE_INTEGER>(core, 5);
        const auto User             = arg<nt64::PTOKEN_USER>(core, 6);
        const auto Groups           = arg<nt64::PTOKEN_GROUPS>(core, 7);
        const auto Privileges       = arg<nt64::PTOKEN_PRIVILEGES>(core, 8);
        const auto Owner            = arg<nt64::PTOKEN_OWNER>(core, 9);
        const auto PrimaryGroup     = arg<nt64::PTOKEN_PRIMARY_GROUP>(core, 10);
        const auto DefaultDacl      = arg<nt64::PTOKEN_DEFAULT_DACL>(core, 11);
        const auto TokenSource      = arg<nt64::PTOKEN_SOURCE>(core, 12);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[89]);

        on_func(TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateTransactionManager(proc_t proc, const on_NtCreateTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateTransactionManager", [=]
    {
        auto& core = d_->core;
        
        const auto TmHandle         = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto LogFileName      = arg<nt64::PUNICODE_STRING>(core, 3);
        const auto CreateOptions    = arg<nt64::ULONG>(core, 4);
        const auto CommitStrength   = arg<nt64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[90]);

        on_func(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateTransaction(proc_t proc, const on_NtCreateTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateTransaction", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Uow               = arg<nt64::LPGUID>(core, 3);
        const auto TmHandle          = arg<nt64::HANDLE>(core, 4);
        const auto CreateOptions     = arg<nt64::ULONG>(core, 5);
        const auto IsolationLevel    = arg<nt64::ULONG>(core, 6);
        const auto IsolationFlags    = arg<nt64::ULONG>(core, 7);
        const auto Timeout           = arg<nt64::PLARGE_INTEGER>(core, 8);
        const auto Description       = arg<nt64::PUNICODE_STRING>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[91]);

        on_func(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateUserProcess(proc_t proc, const on_NtCreateUserProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateUserProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle           = arg<nt64::PHANDLE>(core, 0);
        const auto ThreadHandle            = arg<nt64::PHANDLE>(core, 1);
        const auto ProcessDesiredAccess    = arg<nt64::ACCESS_MASK>(core, 2);
        const auto ThreadDesiredAccess     = arg<nt64::ACCESS_MASK>(core, 3);
        const auto ProcessObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 4);
        const auto ThreadObjectAttributes  = arg<nt64::POBJECT_ATTRIBUTES>(core, 5);
        const auto ProcessFlags            = arg<nt64::ULONG>(core, 6);
        const auto ThreadFlags             = arg<nt64::ULONG>(core, 7);
        const auto ProcessParameters       = arg<nt64::PRTL_USER_PROCESS_PARAMETERS>(core, 8);
        const auto CreateInfo              = arg<nt64::PPROCESS_CREATE_INFO>(core, 9);
        const auto AttributeList           = arg<nt64::PPROCESS_ATTRIBUTE_LIST>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[92]);

        on_func(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateWaitablePort(proc_t proc, const on_NtCreateWaitablePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateWaitablePort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle              = arg<nt64::PHANDLE>(core, 0);
        const auto ObjectAttributes        = arg<nt64::POBJECT_ATTRIBUTES>(core, 1);
        const auto MaxConnectionInfoLength = arg<nt64::ULONG>(core, 2);
        const auto MaxMessageLength        = arg<nt64::ULONG>(core, 3);
        const auto MaxPoolUsage            = arg<nt64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[93]);

        on_func(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    });
}

opt<id_t> nt64::syscalls::register_NtCreateWorkerFactory(proc_t proc, const on_NtCreateWorkerFactory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtCreateWorkerFactory", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandleReturn = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess             = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes          = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto CompletionPortHandle      = arg<nt64::HANDLE>(core, 3);
        const auto WorkerProcessHandle       = arg<nt64::HANDLE>(core, 4);
        const auto StartRoutine              = arg<nt64::PVOID>(core, 5);
        const auto StartParameter            = arg<nt64::PVOID>(core, 6);
        const auto MaxThreadCount            = arg<nt64::ULONG>(core, 7);
        const auto StackReserve              = arg<nt64::SIZE_T>(core, 8);
        const auto StackCommit               = arg<nt64::SIZE_T>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[94]);

        on_func(WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit);
    });
}

opt<id_t> nt64::syscalls::register_NtDebugActiveProcess(proc_t proc, const on_NtDebugActiveProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDebugActiveProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle     = arg<nt64::HANDLE>(core, 0);
        const auto DebugObjectHandle = arg<nt64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[95]);

        on_func(ProcessHandle, DebugObjectHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtDebugContinue(proc_t proc, const on_NtDebugContinue_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDebugContinue", [=]
    {
        auto& core = d_->core;
        
        const auto DebugObjectHandle = arg<nt64::HANDLE>(core, 0);
        const auto ClientId          = arg<nt64::PCLIENT_ID>(core, 1);
        const auto ContinueStatus    = arg<nt64::NTSTATUS>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[96]);

        on_func(DebugObjectHandle, ClientId, ContinueStatus);
    });
}

opt<id_t> nt64::syscalls::register_NtDelayExecution(proc_t proc, const on_NtDelayExecution_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDelayExecution", [=]
    {
        auto& core = d_->core;
        
        const auto Alertable     = arg<nt64::BOOLEAN>(core, 0);
        const auto DelayInterval = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[97]);

        on_func(Alertable, DelayInterval);
    });
}

opt<id_t> nt64::syscalls::register_NtDeleteAtom(proc_t proc, const on_NtDeleteAtom_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDeleteAtom", [=]
    {
        auto& core = d_->core;
        
        const auto Atom = arg<nt64::RTL_ATOM>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[98]);

        on_func(Atom);
    });
}

opt<id_t> nt64::syscalls::register_NtDeleteBootEntry(proc_t proc, const on_NtDeleteBootEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDeleteBootEntry", [=]
    {
        auto& core = d_->core;
        
        const auto Id = arg<nt64::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[99]);

        on_func(Id);
    });
}

opt<id_t> nt64::syscalls::register_NtDeleteDriverEntry(proc_t proc, const on_NtDeleteDriverEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDeleteDriverEntry", [=]
    {
        auto& core = d_->core;
        
        const auto Id = arg<nt64::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[100]);

        on_func(Id);
    });
}

opt<id_t> nt64::syscalls::register_NtDeleteFile(proc_t proc, const on_NtDeleteFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDeleteFile", [=]
    {
        auto& core = d_->core;
        
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[101]);

        on_func(ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtDeleteKey(proc_t proc, const on_NtDeleteKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDeleteKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[102]);

        on_func(KeyHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtDeleteObjectAuditAlarm(proc_t proc, const on_NtDeleteObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDeleteObjectAuditAlarm", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName   = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto HandleId        = arg<nt64::PVOID>(core, 1);
        const auto GenerateOnClose = arg<nt64::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[103]);

        on_func(SubsystemName, HandleId, GenerateOnClose);
    });
}

opt<id_t> nt64::syscalls::register_NtDeletePrivateNamespace(proc_t proc, const on_NtDeletePrivateNamespace_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDeletePrivateNamespace", [=]
    {
        auto& core = d_->core;
        
        const auto NamespaceHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[104]);

        on_func(NamespaceHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtDeleteValueKey(proc_t proc, const on_NtDeleteValueKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDeleteValueKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle = arg<nt64::HANDLE>(core, 0);
        const auto ValueName = arg<nt64::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[105]);

        on_func(KeyHandle, ValueName);
    });
}

opt<id_t> nt64::syscalls::register_NtDeviceIoControlFile(proc_t proc, const on_NtDeviceIoControlFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDeviceIoControlFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle         = arg<nt64::HANDLE>(core, 0);
        const auto Event              = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine         = arg<nt64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext         = arg<nt64::PVOID>(core, 3);
        const auto IoStatusBlock      = arg<nt64::PIO_STATUS_BLOCK>(core, 4);
        const auto IoControlCode      = arg<nt64::ULONG>(core, 5);
        const auto InputBuffer        = arg<nt64::PVOID>(core, 6);
        const auto InputBufferLength  = arg<nt64::ULONG>(core, 7);
        const auto OutputBuffer       = arg<nt64::PVOID>(core, 8);
        const auto OutputBufferLength = arg<nt64::ULONG>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[106]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<id_t> nt64::syscalls::register_NtDisplayString(proc_t proc, const on_NtDisplayString_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDisplayString", [=]
    {
        auto& core = d_->core;
        
        const auto String = arg<nt64::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[107]);

        on_func(String);
    });
}

opt<id_t> nt64::syscalls::register_NtDrawText(proc_t proc, const on_NtDrawText_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDrawText", [=]
    {
        auto& core = d_->core;
        
        const auto Text = arg<nt64::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[108]);

        on_func(Text);
    });
}

opt<id_t> nt64::syscalls::register_NtDuplicateObject(proc_t proc, const on_NtDuplicateObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDuplicateObject", [=]
    {
        auto& core = d_->core;
        
        const auto SourceProcessHandle = arg<nt64::HANDLE>(core, 0);
        const auto SourceHandle        = arg<nt64::HANDLE>(core, 1);
        const auto TargetProcessHandle = arg<nt64::HANDLE>(core, 2);
        const auto TargetHandle        = arg<nt64::PHANDLE>(core, 3);
        const auto DesiredAccess       = arg<nt64::ACCESS_MASK>(core, 4);
        const auto HandleAttributes    = arg<nt64::ULONG>(core, 5);
        const auto Options             = arg<nt64::ULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[109]);

        on_func(SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);
    });
}

opt<id_t> nt64::syscalls::register_NtDuplicateToken(proc_t proc, const on_NtDuplicateToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDuplicateToken", [=]
    {
        auto& core = d_->core;
        
        const auto ExistingTokenHandle = arg<nt64::HANDLE>(core, 0);
        const auto DesiredAccess       = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes    = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto EffectiveOnly       = arg<nt64::BOOLEAN>(core, 3);
        const auto TokenType           = arg<nt64::TOKEN_TYPE>(core, 4);
        const auto NewTokenHandle      = arg<nt64::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[110]);

        on_func(ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtEnumerateBootEntries(proc_t proc, const on_NtEnumerateBootEntries_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtEnumerateBootEntries", [=]
    {
        auto& core = d_->core;
        
        const auto Buffer       = arg<nt64::PVOID>(core, 0);
        const auto BufferLength = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[111]);

        on_func(Buffer, BufferLength);
    });
}

opt<id_t> nt64::syscalls::register_NtEnumerateDriverEntries(proc_t proc, const on_NtEnumerateDriverEntries_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtEnumerateDriverEntries", [=]
    {
        auto& core = d_->core;
        
        const auto Buffer       = arg<nt64::PVOID>(core, 0);
        const auto BufferLength = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[112]);

        on_func(Buffer, BufferLength);
    });
}

opt<id_t> nt64::syscalls::register_NtEnumerateKey(proc_t proc, const on_NtEnumerateKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtEnumerateKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle           = arg<nt64::HANDLE>(core, 0);
        const auto Index               = arg<nt64::ULONG>(core, 1);
        const auto KeyInformationClass = arg<nt64::KEY_INFORMATION_CLASS>(core, 2);
        const auto KeyInformation      = arg<nt64::PVOID>(core, 3);
        const auto Length              = arg<nt64::ULONG>(core, 4);
        const auto ResultLength        = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[113]);

        on_func(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
    });
}

opt<id_t> nt64::syscalls::register_NtEnumerateSystemEnvironmentValuesEx(proc_t proc, const on_NtEnumerateSystemEnvironmentValuesEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtEnumerateSystemEnvironmentValuesEx", [=]
    {
        auto& core = d_->core;
        
        const auto InformationClass = arg<nt64::ULONG>(core, 0);
        const auto Buffer           = arg<nt64::PVOID>(core, 1);
        const auto BufferLength     = arg<nt64::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[114]);

        on_func(InformationClass, Buffer, BufferLength);
    });
}

opt<id_t> nt64::syscalls::register_NtEnumerateTransactionObject(proc_t proc, const on_NtEnumerateTransactionObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtEnumerateTransactionObject", [=]
    {
        auto& core = d_->core;
        
        const auto RootObjectHandle   = arg<nt64::HANDLE>(core, 0);
        const auto QueryType          = arg<nt64::KTMOBJECT_TYPE>(core, 1);
        const auto ObjectCursor       = arg<nt64::PKTMOBJECT_CURSOR>(core, 2);
        const auto ObjectCursorLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength       = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[115]);

        on_func(RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtEnumerateValueKey(proc_t proc, const on_NtEnumerateValueKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtEnumerateValueKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle                = arg<nt64::HANDLE>(core, 0);
        const auto Index                    = arg<nt64::ULONG>(core, 1);
        const auto KeyValueInformationClass = arg<nt64::KEY_VALUE_INFORMATION_CLASS>(core, 2);
        const auto KeyValueInformation      = arg<nt64::PVOID>(core, 3);
        const auto Length                   = arg<nt64::ULONG>(core, 4);
        const auto ResultLength             = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[116]);

        on_func(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    });
}

opt<id_t> nt64::syscalls::register_NtExtendSection(proc_t proc, const on_NtExtendSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtExtendSection", [=]
    {
        auto& core = d_->core;
        
        const auto SectionHandle  = arg<nt64::HANDLE>(core, 0);
        const auto NewSectionSize = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[117]);

        on_func(SectionHandle, NewSectionSize);
    });
}

opt<id_t> nt64::syscalls::register_NtFilterToken(proc_t proc, const on_NtFilterToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFilterToken", [=]
    {
        auto& core = d_->core;
        
        const auto ExistingTokenHandle = arg<nt64::HANDLE>(core, 0);
        const auto Flags               = arg<nt64::ULONG>(core, 1);
        const auto SidsToDisable       = arg<nt64::PTOKEN_GROUPS>(core, 2);
        const auto PrivilegesToDelete  = arg<nt64::PTOKEN_PRIVILEGES>(core, 3);
        const auto RestrictedSids      = arg<nt64::PTOKEN_GROUPS>(core, 4);
        const auto NewTokenHandle      = arg<nt64::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[118]);

        on_func(ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtFindAtom(proc_t proc, const on_NtFindAtom_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFindAtom", [=]
    {
        auto& core = d_->core;
        
        const auto AtomName = arg<nt64::PWSTR>(core, 0);
        const auto Length   = arg<nt64::ULONG>(core, 1);
        const auto Atom     = arg<nt64::PRTL_ATOM>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[119]);

        on_func(AtomName, Length, Atom);
    });
}

opt<id_t> nt64::syscalls::register_NtFlushBuffersFile(proc_t proc, const on_NtFlushBuffersFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFlushBuffersFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt64::PIO_STATUS_BLOCK>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[120]);

        on_func(FileHandle, IoStatusBlock);
    });
}

opt<id_t> nt64::syscalls::register_NtFlushInstallUILanguage(proc_t proc, const on_NtFlushInstallUILanguage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFlushInstallUILanguage", [=]
    {
        auto& core = d_->core;
        
        const auto InstallUILanguage = arg<nt64::LANGID>(core, 0);
        const auto SetComittedFlag   = arg<nt64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[121]);

        on_func(InstallUILanguage, SetComittedFlag);
    });
}

opt<id_t> nt64::syscalls::register_NtFlushInstructionCache(proc_t proc, const on_NtFlushInstructionCache_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFlushInstructionCache", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 0);
        const auto BaseAddress   = arg<nt64::PVOID>(core, 1);
        const auto Length        = arg<nt64::SIZE_T>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[122]);

        on_func(ProcessHandle, BaseAddress, Length);
    });
}

opt<id_t> nt64::syscalls::register_NtFlushKey(proc_t proc, const on_NtFlushKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFlushKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[123]);

        on_func(KeyHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtFlushVirtualMemory(proc_t proc, const on_NtFlushVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFlushVirtualMemory", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt64::PVOID>(core, 1);
        const auto RegionSize      = arg<nt64::PSIZE_T>(core, 2);
        const auto IoStatus        = arg<nt64::PIO_STATUS_BLOCK>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[124]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, IoStatus);
    });
}

opt<id_t> nt64::syscalls::register_NtFreeUserPhysicalPages(proc_t proc, const on_NtFreeUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFreeUserPhysicalPages", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 0);
        const auto NumberOfPages = arg<nt64::PULONG_PTR>(core, 1);
        const auto UserPfnArra   = arg<nt64::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[125]);

        on_func(ProcessHandle, NumberOfPages, UserPfnArra);
    });
}

opt<id_t> nt64::syscalls::register_NtFreeVirtualMemory(proc_t proc, const on_NtFreeVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFreeVirtualMemory", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt64::PVOID>(core, 1);
        const auto RegionSize      = arg<nt64::PSIZE_T>(core, 2);
        const auto FreeType        = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[126]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, FreeType);
    });
}

opt<id_t> nt64::syscalls::register_NtFreezeRegistry(proc_t proc, const on_NtFreezeRegistry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFreezeRegistry", [=]
    {
        auto& core = d_->core;
        
        const auto TimeOutInSeconds = arg<nt64::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[127]);

        on_func(TimeOutInSeconds);
    });
}

opt<id_t> nt64::syscalls::register_NtFreezeTransactions(proc_t proc, const on_NtFreezeTransactions_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFreezeTransactions", [=]
    {
        auto& core = d_->core;
        
        const auto FreezeTimeout = arg<nt64::PLARGE_INTEGER>(core, 0);
        const auto ThawTimeout   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[128]);

        on_func(FreezeTimeout, ThawTimeout);
    });
}

opt<id_t> nt64::syscalls::register_NtFsControlFile(proc_t proc, const on_NtFsControlFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFsControlFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle         = arg<nt64::HANDLE>(core, 0);
        const auto Event              = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine         = arg<nt64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext         = arg<nt64::PVOID>(core, 3);
        const auto IoStatusBlock      = arg<nt64::PIO_STATUS_BLOCK>(core, 4);
        const auto IoControlCode      = arg<nt64::ULONG>(core, 5);
        const auto InputBuffer        = arg<nt64::PVOID>(core, 6);
        const auto InputBufferLength  = arg<nt64::ULONG>(core, 7);
        const auto OutputBuffer       = arg<nt64::PVOID>(core, 8);
        const auto OutputBufferLength = arg<nt64::ULONG>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[129]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<id_t> nt64::syscalls::register_NtGetContextThread(proc_t proc, const on_NtGetContextThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtGetContextThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle  = arg<nt64::HANDLE>(core, 0);
        const auto ThreadContext = arg<nt64::PCONTEXT>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[130]);

        on_func(ThreadHandle, ThreadContext);
    });
}

opt<id_t> nt64::syscalls::register_NtGetDevicePowerState(proc_t proc, const on_NtGetDevicePowerState_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtGetDevicePowerState", [=]
    {
        auto& core = d_->core;
        
        const auto Device    = arg<nt64::HANDLE>(core, 0);
        const auto STARState = arg<nt64::DEVICE_POWER_STATE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[131]);

        on_func(Device, STARState);
    });
}

opt<id_t> nt64::syscalls::register_NtGetMUIRegistryInfo(proc_t proc, const on_NtGetMUIRegistryInfo_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtGetMUIRegistryInfo", [=]
    {
        auto& core = d_->core;
        
        const auto Flags    = arg<nt64::ULONG>(core, 0);
        const auto DataSize = arg<nt64::PULONG>(core, 1);
        const auto Data     = arg<nt64::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[132]);

        on_func(Flags, DataSize, Data);
    });
}

opt<id_t> nt64::syscalls::register_NtGetNextProcess(proc_t proc, const on_NtGetNextProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtGetNextProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt64::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto HandleAttributes = arg<nt64::ULONG>(core, 2);
        const auto Flags            = arg<nt64::ULONG>(core, 3);
        const auto NewProcessHandle = arg<nt64::PHANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[133]);

        on_func(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtGetNextThread(proc_t proc, const on_NtGetNextThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtGetNextThread", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt64::HANDLE>(core, 0);
        const auto ThreadHandle     = arg<nt64::HANDLE>(core, 1);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 2);
        const auto HandleAttributes = arg<nt64::ULONG>(core, 3);
        const auto Flags            = arg<nt64::ULONG>(core, 4);
        const auto NewThreadHandle  = arg<nt64::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[134]);

        on_func(ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtGetNlsSectionPtr(proc_t proc, const on_NtGetNlsSectionPtr_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtGetNlsSectionPtr", [=]
    {
        auto& core = d_->core;
        
        const auto SectionType        = arg<nt64::ULONG>(core, 0);
        const auto SectionData        = arg<nt64::ULONG>(core, 1);
        const auto ContextData        = arg<nt64::PVOID>(core, 2);
        const auto STARSectionPointer = arg<nt64::PVOID>(core, 3);
        const auto SectionSize        = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[135]);

        on_func(SectionType, SectionData, ContextData, STARSectionPointer, SectionSize);
    });
}

opt<id_t> nt64::syscalls::register_NtGetNotificationResourceManager(proc_t proc, const on_NtGetNotificationResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtGetNotificationResourceManager", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle   = arg<nt64::HANDLE>(core, 0);
        const auto TransactionNotification = arg<nt64::PTRANSACTION_NOTIFICATION>(core, 1);
        const auto NotificationLength      = arg<nt64::ULONG>(core, 2);
        const auto Timeout                 = arg<nt64::PLARGE_INTEGER>(core, 3);
        const auto ReturnLength            = arg<nt64::PULONG>(core, 4);
        const auto Asynchronous            = arg<nt64::ULONG>(core, 5);
        const auto AsynchronousContext     = arg<nt64::ULONG_PTR>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[136]);

        on_func(ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext);
    });
}

opt<id_t> nt64::syscalls::register_NtGetPlugPlayEvent(proc_t proc, const on_NtGetPlugPlayEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtGetPlugPlayEvent", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle     = arg<nt64::HANDLE>(core, 0);
        const auto Context         = arg<nt64::PVOID>(core, 1);
        const auto EventBlock      = arg<nt64::PPLUGPLAY_EVENT_BLOCK>(core, 2);
        const auto EventBufferSize = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[137]);

        on_func(EventHandle, Context, EventBlock, EventBufferSize);
    });
}

opt<id_t> nt64::syscalls::register_NtGetWriteWatch(proc_t proc, const on_NtGetWriteWatch_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtGetWriteWatch", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle             = arg<nt64::HANDLE>(core, 0);
        const auto Flags                     = arg<nt64::ULONG>(core, 1);
        const auto BaseAddress               = arg<nt64::PVOID>(core, 2);
        const auto RegionSize                = arg<nt64::SIZE_T>(core, 3);
        const auto STARUserAddressArray      = arg<nt64::PVOID>(core, 4);
        const auto EntriesInUserAddressArray = arg<nt64::PULONG_PTR>(core, 5);
        const auto Granularity               = arg<nt64::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[138]);

        on_func(ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity);
    });
}

opt<id_t> nt64::syscalls::register_NtImpersonateAnonymousToken(proc_t proc, const on_NtImpersonateAnonymousToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtImpersonateAnonymousToken", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[139]);

        on_func(ThreadHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtImpersonateClientOfPort(proc_t proc, const on_NtImpersonateClientOfPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtImpersonateClientOfPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt64::HANDLE>(core, 0);
        const auto Message    = arg<nt64::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[140]);

        on_func(PortHandle, Message);
    });
}

opt<id_t> nt64::syscalls::register_NtImpersonateThread(proc_t proc, const on_NtImpersonateThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtImpersonateThread", [=]
    {
        auto& core = d_->core;
        
        const auto ServerThreadHandle = arg<nt64::HANDLE>(core, 0);
        const auto ClientThreadHandle = arg<nt64::HANDLE>(core, 1);
        const auto SecurityQos        = arg<nt64::PSECURITY_QUALITY_OF_SERVICE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[141]);

        on_func(ServerThreadHandle, ClientThreadHandle, SecurityQos);
    });
}

opt<id_t> nt64::syscalls::register_NtInitializeNlsFiles(proc_t proc, const on_NtInitializeNlsFiles_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtInitializeNlsFiles", [=]
    {
        auto& core = d_->core;
        
        const auto STARBaseAddress        = arg<nt64::PVOID>(core, 0);
        const auto DefaultLocaleId        = arg<nt64::PLCID>(core, 1);
        const auto DefaultCasingTableSize = arg<nt64::PLARGE_INTEGER>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[142]);

        on_func(STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize);
    });
}

opt<id_t> nt64::syscalls::register_NtInitializeRegistry(proc_t proc, const on_NtInitializeRegistry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtInitializeRegistry", [=]
    {
        auto& core = d_->core;
        
        const auto BootCondition = arg<nt64::USHORT>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[143]);

        on_func(BootCondition);
    });
}

opt<id_t> nt64::syscalls::register_NtInitiatePowerAction(proc_t proc, const on_NtInitiatePowerAction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtInitiatePowerAction", [=]
    {
        auto& core = d_->core;
        
        const auto SystemAction   = arg<nt64::POWER_ACTION>(core, 0);
        const auto MinSystemState = arg<nt64::SYSTEM_POWER_STATE>(core, 1);
        const auto Flags          = arg<nt64::ULONG>(core, 2);
        const auto Asynchronous   = arg<nt64::BOOLEAN>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[144]);

        on_func(SystemAction, MinSystemState, Flags, Asynchronous);
    });
}

opt<id_t> nt64::syscalls::register_NtIsProcessInJob(proc_t proc, const on_NtIsProcessInJob_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtIsProcessInJob", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 0);
        const auto JobHandle     = arg<nt64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[145]);

        on_func(ProcessHandle, JobHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtListenPort(proc_t proc, const on_NtListenPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtListenPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle        = arg<nt64::HANDLE>(core, 0);
        const auto ConnectionRequest = arg<nt64::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[146]);

        on_func(PortHandle, ConnectionRequest);
    });
}

opt<id_t> nt64::syscalls::register_NtLoadDriver(proc_t proc, const on_NtLoadDriver_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtLoadDriver", [=]
    {
        auto& core = d_->core;
        
        const auto DriverServiceName = arg<nt64::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[147]);

        on_func(DriverServiceName);
    });
}

opt<id_t> nt64::syscalls::register_NtLoadKey2(proc_t proc, const on_NtLoadKey2_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtLoadKey2", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey  = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile = arg<nt64::POBJECT_ATTRIBUTES>(core, 1);
        const auto Flags      = arg<nt64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[148]);

        on_func(TargetKey, SourceFile, Flags);
    });
}

opt<id_t> nt64::syscalls::register_NtLoadKeyEx(proc_t proc, const on_NtLoadKeyEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtLoadKeyEx", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey     = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile    = arg<nt64::POBJECT_ATTRIBUTES>(core, 1);
        const auto Flags         = arg<nt64::ULONG>(core, 2);
        const auto TrustClassKey = arg<nt64::HANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[149]);

        on_func(TargetKey, SourceFile, Flags, TrustClassKey);
    });
}

opt<id_t> nt64::syscalls::register_NtLoadKey(proc_t proc, const on_NtLoadKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtLoadKey", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey  = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile = arg<nt64::POBJECT_ATTRIBUTES>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[150]);

        on_func(TargetKey, SourceFile);
    });
}

opt<id_t> nt64::syscalls::register_NtLockFile(proc_t proc, const on_NtLockFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtLockFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle      = arg<nt64::HANDLE>(core, 0);
        const auto Event           = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine      = arg<nt64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext      = arg<nt64::PVOID>(core, 3);
        const auto IoStatusBlock   = arg<nt64::PIO_STATUS_BLOCK>(core, 4);
        const auto ByteOffset      = arg<nt64::PLARGE_INTEGER>(core, 5);
        const auto Length          = arg<nt64::PLARGE_INTEGER>(core, 6);
        const auto Key             = arg<nt64::ULONG>(core, 7);
        const auto FailImmediately = arg<nt64::BOOLEAN>(core, 8);
        const auto ExclusiveLock   = arg<nt64::BOOLEAN>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[151]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock);
    });
}

opt<id_t> nt64::syscalls::register_NtLockProductActivationKeys(proc_t proc, const on_NtLockProductActivationKeys_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtLockProductActivationKeys", [=]
    {
        auto& core = d_->core;
        
        const auto STARpPrivateVer = arg<nt64::ULONG>(core, 0);
        const auto STARpSafeMode   = arg<nt64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[152]);

        on_func(STARpPrivateVer, STARpSafeMode);
    });
}

opt<id_t> nt64::syscalls::register_NtLockRegistryKey(proc_t proc, const on_NtLockRegistryKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtLockRegistryKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[153]);

        on_func(KeyHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtLockVirtualMemory(proc_t proc, const on_NtLockVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtLockVirtualMemory", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt64::PVOID>(core, 1);
        const auto RegionSize      = arg<nt64::PSIZE_T>(core, 2);
        const auto MapType         = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[154]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    });
}

opt<id_t> nt64::syscalls::register_NtMakePermanentObject(proc_t proc, const on_NtMakePermanentObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtMakePermanentObject", [=]
    {
        auto& core = d_->core;
        
        const auto Handle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[155]);

        on_func(Handle);
    });
}

opt<id_t> nt64::syscalls::register_NtMakeTemporaryObject(proc_t proc, const on_NtMakeTemporaryObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtMakeTemporaryObject", [=]
    {
        auto& core = d_->core;
        
        const auto Handle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[156]);

        on_func(Handle);
    });
}

opt<id_t> nt64::syscalls::register_NtMapCMFModule(proc_t proc, const on_NtMapCMFModule_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtMapCMFModule", [=]
    {
        auto& core = d_->core;
        
        const auto What            = arg<nt64::ULONG>(core, 0);
        const auto Index           = arg<nt64::ULONG>(core, 1);
        const auto CacheIndexOut   = arg<nt64::PULONG>(core, 2);
        const auto CacheFlagsOut   = arg<nt64::PULONG>(core, 3);
        const auto ViewSizeOut     = arg<nt64::PULONG>(core, 4);
        const auto STARBaseAddress = arg<nt64::PVOID>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[157]);

        on_func(What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress);
    });
}

opt<id_t> nt64::syscalls::register_NtMapUserPhysicalPages(proc_t proc, const on_NtMapUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtMapUserPhysicalPages", [=]
    {
        auto& core = d_->core;
        
        const auto VirtualAddress = arg<nt64::PVOID>(core, 0);
        const auto NumberOfPages  = arg<nt64::ULONG_PTR>(core, 1);
        const auto UserPfnArra    = arg<nt64::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[158]);

        on_func(VirtualAddress, NumberOfPages, UserPfnArra);
    });
}

opt<id_t> nt64::syscalls::register_NtMapUserPhysicalPagesScatter(proc_t proc, const on_NtMapUserPhysicalPagesScatter_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtMapUserPhysicalPagesScatter", [=]
    {
        auto& core = d_->core;
        
        const auto STARVirtualAddresses = arg<nt64::PVOID>(core, 0);
        const auto NumberOfPages        = arg<nt64::ULONG_PTR>(core, 1);
        const auto UserPfnArray         = arg<nt64::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[159]);

        on_func(STARVirtualAddresses, NumberOfPages, UserPfnArray);
    });
}

opt<id_t> nt64::syscalls::register_NtMapViewOfSection(proc_t proc, const on_NtMapViewOfSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtMapViewOfSection", [=]
    {
        auto& core = d_->core;
        
        const auto SectionHandle      = arg<nt64::HANDLE>(core, 0);
        const auto ProcessHandle      = arg<nt64::HANDLE>(core, 1);
        const auto STARBaseAddress    = arg<nt64::PVOID>(core, 2);
        const auto ZeroBits           = arg<nt64::ULONG_PTR>(core, 3);
        const auto CommitSize         = arg<nt64::SIZE_T>(core, 4);
        const auto SectionOffset      = arg<nt64::PLARGE_INTEGER>(core, 5);
        const auto ViewSize           = arg<nt64::PSIZE_T>(core, 6);
        const auto InheritDisposition = arg<nt64::SECTION_INHERIT>(core, 7);
        const auto AllocationType     = arg<nt64::ULONG>(core, 8);
        const auto Win32Protect       = arg<nt64::WIN32_PROTECTION_MASK>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[160]);

        on_func(SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
    });
}

opt<id_t> nt64::syscalls::register_NtModifyBootEntry(proc_t proc, const on_NtModifyBootEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtModifyBootEntry", [=]
    {
        auto& core = d_->core;
        
        const auto BootEntry = arg<nt64::PBOOT_ENTRY>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[161]);

        on_func(BootEntry);
    });
}

opt<id_t> nt64::syscalls::register_NtModifyDriverEntry(proc_t proc, const on_NtModifyDriverEntry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtModifyDriverEntry", [=]
    {
        auto& core = d_->core;
        
        const auto DriverEntry = arg<nt64::PEFI_DRIVER_ENTRY>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[162]);

        on_func(DriverEntry);
    });
}

opt<id_t> nt64::syscalls::register_NtNotifyChangeDirectoryFile(proc_t proc, const on_NtNotifyChangeDirectoryFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtNotifyChangeDirectoryFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle       = arg<nt64::HANDLE>(core, 0);
        const auto Event            = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine       = arg<nt64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext       = arg<nt64::PVOID>(core, 3);
        const auto IoStatusBlock    = arg<nt64::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer           = arg<nt64::PVOID>(core, 5);
        const auto Length           = arg<nt64::ULONG>(core, 6);
        const auto CompletionFilter = arg<nt64::ULONG>(core, 7);
        const auto WatchTree        = arg<nt64::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[163]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree);
    });
}

opt<id_t> nt64::syscalls::register_NtNotifyChangeKey(proc_t proc, const on_NtNotifyChangeKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtNotifyChangeKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle        = arg<nt64::HANDLE>(core, 0);
        const auto Event            = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine       = arg<nt64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext       = arg<nt64::PVOID>(core, 3);
        const auto IoStatusBlock    = arg<nt64::PIO_STATUS_BLOCK>(core, 4);
        const auto CompletionFilter = arg<nt64::ULONG>(core, 5);
        const auto WatchTree        = arg<nt64::BOOLEAN>(core, 6);
        const auto Buffer           = arg<nt64::PVOID>(core, 7);
        const auto BufferSize       = arg<nt64::ULONG>(core, 8);
        const auto Asynchronous     = arg<nt64::BOOLEAN>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[164]);

        on_func(KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    });
}

opt<id_t> nt64::syscalls::register_NtNotifyChangeMultipleKeys(proc_t proc, const on_NtNotifyChangeMultipleKeys_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtNotifyChangeMultipleKeys", [=]
    {
        auto& core = d_->core;
        
        const auto MasterKeyHandle  = arg<nt64::HANDLE>(core, 0);
        const auto Count            = arg<nt64::ULONG>(core, 1);
        const auto SlaveObjects     = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Event            = arg<nt64::HANDLE>(core, 3);
        const auto ApcRoutine       = arg<nt64::PIO_APC_ROUTINE>(core, 4);
        const auto ApcContext       = arg<nt64::PVOID>(core, 5);
        const auto IoStatusBlock    = arg<nt64::PIO_STATUS_BLOCK>(core, 6);
        const auto CompletionFilter = arg<nt64::ULONG>(core, 7);
        const auto WatchTree        = arg<nt64::BOOLEAN>(core, 8);
        const auto Buffer           = arg<nt64::PVOID>(core, 9);
        const auto BufferSize       = arg<nt64::ULONG>(core, 10);
        const auto Asynchronous     = arg<nt64::BOOLEAN>(core, 11);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[165]);

        on_func(MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    });
}

opt<id_t> nt64::syscalls::register_NtNotifyChangeSession(proc_t proc, const on_NtNotifyChangeSession_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtNotifyChangeSession", [=]
    {
        auto& core = d_->core;
        
        const auto Session         = arg<nt64::HANDLE>(core, 0);
        const auto IoStateSequence = arg<nt64::ULONG>(core, 1);
        const auto Reserved        = arg<nt64::PVOID>(core, 2);
        const auto Action          = arg<nt64::ULONG>(core, 3);
        const auto IoState         = arg<nt64::IO_SESSION_STATE>(core, 4);
        const auto IoState2        = arg<nt64::IO_SESSION_STATE>(core, 5);
        const auto Buffer          = arg<nt64::PVOID>(core, 6);
        const auto BufferSize      = arg<nt64::ULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[166]);

        on_func(Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenDirectoryObject(proc_t proc, const on_NtOpenDirectoryObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenDirectoryObject", [=]
    {
        auto& core = d_->core;
        
        const auto DirectoryHandle  = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[167]);

        on_func(DirectoryHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenEnlistment(proc_t proc, const on_NtOpenEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenEnlistment", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle      = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ResourceManagerHandle = arg<nt64::HANDLE>(core, 2);
        const auto EnlistmentGuid        = arg<nt64::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<nt64::POBJECT_ATTRIBUTES>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[168]);

        on_func(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenEvent(proc_t proc, const on_NtOpenEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenEvent", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle      = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[169]);

        on_func(EventHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenEventPair(proc_t proc, const on_NtOpenEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenEventPair", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle  = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[170]);

        on_func(EventPairHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenFile(proc_t proc, const on_NtOpenFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle       = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock    = arg<nt64::PIO_STATUS_BLOCK>(core, 3);
        const auto ShareAccess      = arg<nt64::ULONG>(core, 4);
        const auto OpenOptions      = arg<nt64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[171]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenIoCompletion(proc_t proc, const on_NtOpenIoCompletion_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenIoCompletion", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[172]);

        on_func(IoCompletionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenJobObject(proc_t proc, const on_NtOpenJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenJobObject", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle        = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[173]);

        on_func(JobHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenKeyedEvent(proc_t proc, const on_NtOpenKeyedEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenKeyedEvent", [=]
    {
        auto& core = d_->core;
        
        const auto KeyedEventHandle = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[174]);

        on_func(KeyedEventHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenKeyEx(proc_t proc, const on_NtOpenKeyEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenKeyEx", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle        = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto OpenOptions      = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[175]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenKey(proc_t proc, const on_NtOpenKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle        = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[176]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenKeyTransactedEx(proc_t proc, const on_NtOpenKeyTransactedEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenKeyTransactedEx", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle         = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto OpenOptions       = arg<nt64::ULONG>(core, 3);
        const auto TransactionHandle = arg<nt64::HANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[177]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenKeyTransacted(proc_t proc, const on_NtOpenKeyTransacted_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenKeyTransacted", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle         = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto TransactionHandle = arg<nt64::HANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[178]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenMutant(proc_t proc, const on_NtOpenMutant_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenMutant", [=]
    {
        auto& core = d_->core;
        
        const auto MutantHandle     = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[179]);

        on_func(MutantHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenObjectAuditAlarm(proc_t proc, const on_NtOpenObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenObjectAuditAlarm", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName      = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto HandleId           = arg<nt64::PVOID>(core, 1);
        const auto ObjectTypeName     = arg<nt64::PUNICODE_STRING>(core, 2);
        const auto ObjectName         = arg<nt64::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor = arg<nt64::PSECURITY_DESCRIPTOR>(core, 4);
        const auto ClientToken        = arg<nt64::HANDLE>(core, 5);
        const auto DesiredAccess      = arg<nt64::ACCESS_MASK>(core, 6);
        const auto GrantedAccess      = arg<nt64::ACCESS_MASK>(core, 7);
        const auto Privileges         = arg<nt64::PPRIVILEGE_SET>(core, 8);
        const auto ObjectCreation     = arg<nt64::BOOLEAN>(core, 9);
        const auto AccessGranted      = arg<nt64::BOOLEAN>(core, 10);
        const auto GenerateOnClose    = arg<nt64::PBOOLEAN>(core, 11);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[180]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenPrivateNamespace(proc_t proc, const on_NtOpenPrivateNamespace_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenPrivateNamespace", [=]
    {
        auto& core = d_->core;
        
        const auto NamespaceHandle    = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto BoundaryDescriptor = arg<nt64::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[181]);

        on_func(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenProcess(proc_t proc, const on_NtOpenProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ClientId         = arg<nt64::PCLIENT_ID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[182]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenProcessTokenEx(proc_t proc, const on_NtOpenProcessTokenEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenProcessTokenEx", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle    = arg<nt64::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto HandleAttributes = arg<nt64::ULONG>(core, 2);
        const auto TokenHandle      = arg<nt64::PHANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[183]);

        on_func(ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenProcessToken(proc_t proc, const on_NtOpenProcessToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenProcessToken", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 0);
        const auto DesiredAccess = arg<nt64::ACCESS_MASK>(core, 1);
        const auto TokenHandle   = arg<nt64::PHANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[184]);

        on_func(ProcessHandle, DesiredAccess, TokenHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenResourceManager(proc_t proc, const on_NtOpenResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenResourceManager", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt64::ACCESS_MASK>(core, 1);
        const auto TmHandle              = arg<nt64::HANDLE>(core, 2);
        const auto ResourceManagerGuid   = arg<nt64::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<nt64::POBJECT_ATTRIBUTES>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[185]);

        on_func(ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenSection(proc_t proc, const on_NtOpenSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenSection", [=]
    {
        auto& core = d_->core;
        
        const auto SectionHandle    = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[186]);

        on_func(SectionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenSemaphore(proc_t proc, const on_NtOpenSemaphore_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenSemaphore", [=]
    {
        auto& core = d_->core;
        
        const auto SemaphoreHandle  = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[187]);

        on_func(SemaphoreHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenSession(proc_t proc, const on_NtOpenSession_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenSession", [=]
    {
        auto& core = d_->core;
        
        const auto SessionHandle    = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[188]);

        on_func(SessionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenSymbolicLinkObject(proc_t proc, const on_NtOpenSymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenSymbolicLinkObject", [=]
    {
        auto& core = d_->core;
        
        const auto LinkHandle       = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[189]);

        on_func(LinkHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenThread(proc_t proc, const on_NtOpenThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle     = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto ClientId         = arg<nt64::PCLIENT_ID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[190]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenThreadTokenEx(proc_t proc, const on_NtOpenThreadTokenEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenThreadTokenEx", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle     = arg<nt64::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto OpenAsSelf       = arg<nt64::BOOLEAN>(core, 2);
        const auto HandleAttributes = arg<nt64::ULONG>(core, 3);
        const auto TokenHandle      = arg<nt64::PHANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[191]);

        on_func(ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenThreadToken(proc_t proc, const on_NtOpenThreadToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenThreadToken", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle  = arg<nt64::HANDLE>(core, 0);
        const auto DesiredAccess = arg<nt64::ACCESS_MASK>(core, 1);
        const auto OpenAsSelf    = arg<nt64::BOOLEAN>(core, 2);
        const auto TokenHandle   = arg<nt64::PHANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[192]);

        on_func(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenTimer(proc_t proc, const on_NtOpenTimer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenTimer", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle      = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[193]);

        on_func(TimerHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenTransactionManager(proc_t proc, const on_NtOpenTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenTransactionManager", [=]
    {
        auto& core = d_->core;
        
        const auto TmHandle         = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto LogFileName      = arg<nt64::PUNICODE_STRING>(core, 3);
        const auto TmIdentity       = arg<nt64::LPGUID>(core, 4);
        const auto OpenOptions      = arg<nt64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[194]);

        on_func(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions);
    });
}

opt<id_t> nt64::syscalls::register_NtOpenTransaction(proc_t proc, const on_NtOpenTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtOpenTransaction", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle = arg<nt64::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt64::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);
        const auto Uow               = arg<nt64::LPGUID>(core, 3);
        const auto TmHandle          = arg<nt64::HANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[195]);

        on_func(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtPlugPlayControl(proc_t proc, const on_NtPlugPlayControl_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPlugPlayControl", [=]
    {
        auto& core = d_->core;
        
        const auto PnPControlClass      = arg<nt64::PLUGPLAY_CONTROL_CLASS>(core, 0);
        const auto PnPControlData       = arg<nt64::PVOID>(core, 1);
        const auto PnPControlDataLength = arg<nt64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[196]);

        on_func(PnPControlClass, PnPControlData, PnPControlDataLength);
    });
}

opt<id_t> nt64::syscalls::register_NtPowerInformation(proc_t proc, const on_NtPowerInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPowerInformation", [=]
    {
        auto& core = d_->core;
        
        const auto InformationLevel   = arg<nt64::POWER_INFORMATION_LEVEL>(core, 0);
        const auto InputBuffer        = arg<nt64::PVOID>(core, 1);
        const auto InputBufferLength  = arg<nt64::ULONG>(core, 2);
        const auto OutputBuffer       = arg<nt64::PVOID>(core, 3);
        const auto OutputBufferLength = arg<nt64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[197]);

        on_func(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<id_t> nt64::syscalls::register_NtPrepareComplete(proc_t proc, const on_NtPrepareComplete_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPrepareComplete", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[198]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtPrepareEnlistment(proc_t proc, const on_NtPrepareEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPrepareEnlistment", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[199]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtPrePrepareComplete(proc_t proc, const on_NtPrePrepareComplete_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPrePrepareComplete", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[200]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtPrePrepareEnlistment(proc_t proc, const on_NtPrePrepareEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPrePrepareEnlistment", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[201]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtPrivilegeCheck(proc_t proc, const on_NtPrivilegeCheck_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPrivilegeCheck", [=]
    {
        auto& core = d_->core;
        
        const auto ClientToken        = arg<nt64::HANDLE>(core, 0);
        const auto RequiredPrivileges = arg<nt64::PPRIVILEGE_SET>(core, 1);
        const auto Result             = arg<nt64::PBOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[202]);

        on_func(ClientToken, RequiredPrivileges, Result);
    });
}

opt<id_t> nt64::syscalls::register_NtPrivilegedServiceAuditAlarm(proc_t proc, const on_NtPrivilegedServiceAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPrivilegedServiceAuditAlarm", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto ServiceName   = arg<nt64::PUNICODE_STRING>(core, 1);
        const auto ClientToken   = arg<nt64::HANDLE>(core, 2);
        const auto Privileges    = arg<nt64::PPRIVILEGE_SET>(core, 3);
        const auto AccessGranted = arg<nt64::BOOLEAN>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[203]);

        on_func(SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted);
    });
}

opt<id_t> nt64::syscalls::register_NtPrivilegeObjectAuditAlarm(proc_t proc, const on_NtPrivilegeObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPrivilegeObjectAuditAlarm", [=]
    {
        auto& core = d_->core;
        
        const auto SubsystemName = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto HandleId      = arg<nt64::PVOID>(core, 1);
        const auto ClientToken   = arg<nt64::HANDLE>(core, 2);
        const auto DesiredAccess = arg<nt64::ACCESS_MASK>(core, 3);
        const auto Privileges    = arg<nt64::PPRIVILEGE_SET>(core, 4);
        const auto AccessGranted = arg<nt64::BOOLEAN>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[204]);

        on_func(SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);
    });
}

opt<id_t> nt64::syscalls::register_NtPropagationComplete(proc_t proc, const on_NtPropagationComplete_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPropagationComplete", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle = arg<nt64::HANDLE>(core, 0);
        const auto RequestCookie         = arg<nt64::ULONG>(core, 1);
        const auto BufferLength          = arg<nt64::ULONG>(core, 2);
        const auto Buffer                = arg<nt64::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[205]);

        on_func(ResourceManagerHandle, RequestCookie, BufferLength, Buffer);
    });
}

opt<id_t> nt64::syscalls::register_NtPropagationFailed(proc_t proc, const on_NtPropagationFailed_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPropagationFailed", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle = arg<nt64::HANDLE>(core, 0);
        const auto RequestCookie         = arg<nt64::ULONG>(core, 1);
        const auto PropStatus            = arg<nt64::NTSTATUS>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[206]);

        on_func(ResourceManagerHandle, RequestCookie, PropStatus);
    });
}

opt<id_t> nt64::syscalls::register_NtProtectVirtualMemory(proc_t proc, const on_NtProtectVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtProtectVirtualMemory", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt64::PVOID>(core, 1);
        const auto RegionSize      = arg<nt64::PSIZE_T>(core, 2);
        const auto NewProtectWin32 = arg<nt64::WIN32_PROTECTION_MASK>(core, 3);
        const auto OldProtect      = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[207]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect);
    });
}

opt<id_t> nt64::syscalls::register_NtPulseEvent(proc_t proc, const on_NtPulseEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtPulseEvent", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle   = arg<nt64::HANDLE>(core, 0);
        const auto PreviousState = arg<nt64::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[208]);

        on_func(EventHandle, PreviousState);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryAttributesFile(proc_t proc, const on_NtQueryAttributesFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryAttributesFile", [=]
    {
        auto& core = d_->core;
        
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);
        const auto FileInformation  = arg<nt64::PFILE_BASIC_INFORMATION>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[209]);

        on_func(ObjectAttributes, FileInformation);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryBootEntryOrder(proc_t proc, const on_NtQueryBootEntryOrder_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryBootEntryOrder", [=]
    {
        auto& core = d_->core;
        
        const auto Ids   = arg<nt64::PULONG>(core, 0);
        const auto Count = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[210]);

        on_func(Ids, Count);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryBootOptions(proc_t proc, const on_NtQueryBootOptions_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryBootOptions", [=]
    {
        auto& core = d_->core;
        
        const auto BootOptions       = arg<nt64::PBOOT_OPTIONS>(core, 0);
        const auto BootOptionsLength = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[211]);

        on_func(BootOptions, BootOptionsLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryDebugFilterState(proc_t proc, const on_NtQueryDebugFilterState_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryDebugFilterState", [=]
    {
        auto& core = d_->core;
        
        const auto ComponentId = arg<nt64::ULONG>(core, 0);
        const auto Level       = arg<nt64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[212]);

        on_func(ComponentId, Level);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryDefaultLocale(proc_t proc, const on_NtQueryDefaultLocale_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryDefaultLocale", [=]
    {
        auto& core = d_->core;
        
        const auto UserProfile     = arg<nt64::BOOLEAN>(core, 0);
        const auto DefaultLocaleId = arg<nt64::PLCID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[213]);

        on_func(UserProfile, DefaultLocaleId);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryDefaultUILanguage(proc_t proc, const on_NtQueryDefaultUILanguage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryDefaultUILanguage", [=]
    {
        auto& core = d_->core;
        
        const auto STARDefaultUILanguageId = arg<nt64::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[214]);

        on_func(STARDefaultUILanguageId);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryDirectoryFile(proc_t proc, const on_NtQueryDirectoryFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryDirectoryFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle           = arg<nt64::HANDLE>(core, 0);
        const auto Event                = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine           = arg<nt64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext           = arg<nt64::PVOID>(core, 3);
        const auto IoStatusBlock        = arg<nt64::PIO_STATUS_BLOCK>(core, 4);
        const auto FileInformation      = arg<nt64::PVOID>(core, 5);
        const auto Length               = arg<nt64::ULONG>(core, 6);
        const auto FileInformationClass = arg<nt64::FILE_INFORMATION_CLASS>(core, 7);
        const auto ReturnSingleEntry    = arg<nt64::BOOLEAN>(core, 8);
        const auto FileName             = arg<nt64::PUNICODE_STRING>(core, 9);
        const auto RestartScan          = arg<nt64::BOOLEAN>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[215]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryDirectoryObject(proc_t proc, const on_NtQueryDirectoryObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryDirectoryObject", [=]
    {
        auto& core = d_->core;
        
        const auto DirectoryHandle   = arg<nt64::HANDLE>(core, 0);
        const auto Buffer            = arg<nt64::PVOID>(core, 1);
        const auto Length            = arg<nt64::ULONG>(core, 2);
        const auto ReturnSingleEntry = arg<nt64::BOOLEAN>(core, 3);
        const auto RestartScan       = arg<nt64::BOOLEAN>(core, 4);
        const auto Context           = arg<nt64::PULONG>(core, 5);
        const auto ReturnLength      = arg<nt64::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[216]);

        on_func(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryDriverEntryOrder(proc_t proc, const on_NtQueryDriverEntryOrder_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryDriverEntryOrder", [=]
    {
        auto& core = d_->core;
        
        const auto Ids   = arg<nt64::PULONG>(core, 0);
        const auto Count = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[217]);

        on_func(Ids, Count);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryEaFile(proc_t proc, const on_NtQueryEaFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryEaFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle        = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock     = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer            = arg<nt64::PVOID>(core, 2);
        const auto Length            = arg<nt64::ULONG>(core, 3);
        const auto ReturnSingleEntry = arg<nt64::BOOLEAN>(core, 4);
        const auto EaList            = arg<nt64::PVOID>(core, 5);
        const auto EaListLength      = arg<nt64::ULONG>(core, 6);
        const auto EaIndex           = arg<nt64::PULONG>(core, 7);
        const auto RestartScan       = arg<nt64::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[218]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryEvent(proc_t proc, const on_NtQueryEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryEvent", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle            = arg<nt64::HANDLE>(core, 0);
        const auto EventInformationClass  = arg<nt64::EVENT_INFORMATION_CLASS>(core, 1);
        const auto EventInformation       = arg<nt64::PVOID>(core, 2);
        const auto EventInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength           = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[219]);

        on_func(EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryFullAttributesFile(proc_t proc, const on_NtQueryFullAttributesFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryFullAttributesFile", [=]
    {
        auto& core = d_->core;
        
        const auto ObjectAttributes = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);
        const auto FileInformation  = arg<nt64::PFILE_NETWORK_OPEN_INFORMATION>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[220]);

        on_func(ObjectAttributes, FileInformation);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationAtom(proc_t proc, const on_NtQueryInformationAtom_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationAtom", [=]
    {
        auto& core = d_->core;
        
        const auto Atom                  = arg<nt64::RTL_ATOM>(core, 0);
        const auto InformationClass      = arg<nt64::ATOM_INFORMATION_CLASS>(core, 1);
        const auto AtomInformation       = arg<nt64::PVOID>(core, 2);
        const auto AtomInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength          = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[221]);

        on_func(Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationEnlistment(proc_t proc, const on_NtQueryInformationEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationEnlistment", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle            = arg<nt64::HANDLE>(core, 0);
        const auto EnlistmentInformationClass  = arg<nt64::ENLISTMENT_INFORMATION_CLASS>(core, 1);
        const auto EnlistmentInformation       = arg<nt64::PVOID>(core, 2);
        const auto EnlistmentInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength                = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[222]);

        on_func(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationFile(proc_t proc, const on_NtQueryInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle           = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock        = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto FileInformation      = arg<nt64::PVOID>(core, 2);
        const auto Length               = arg<nt64::ULONG>(core, 3);
        const auto FileInformationClass = arg<nt64::FILE_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[223]);

        on_func(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationJobObject(proc_t proc, const on_NtQueryInformationJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationJobObject", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle                  = arg<nt64::HANDLE>(core, 0);
        const auto JobObjectInformationClass  = arg<nt64::JOBOBJECTINFOCLASS>(core, 1);
        const auto JobObjectInformation       = arg<nt64::PVOID>(core, 2);
        const auto JobObjectInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength               = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[224]);

        on_func(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationPort(proc_t proc, const on_NtQueryInformationPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle           = arg<nt64::HANDLE>(core, 0);
        const auto PortInformationClass = arg<nt64::PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<nt64::PVOID>(core, 2);
        const auto Length               = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength         = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[225]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationProcess(proc_t proc, const on_NtQueryInformationProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle            = arg<nt64::HANDLE>(core, 0);
        const auto ProcessInformationClass  = arg<nt64::PROCESSINFOCLASS>(core, 1);
        const auto ProcessInformation       = arg<nt64::PVOID>(core, 2);
        const auto ProcessInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength             = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[226]);

        on_func(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationResourceManager(proc_t proc, const on_NtQueryInformationResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationResourceManager", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle            = arg<nt64::HANDLE>(core, 0);
        const auto ResourceManagerInformationClass  = arg<nt64::RESOURCEMANAGER_INFORMATION_CLASS>(core, 1);
        const auto ResourceManagerInformation       = arg<nt64::PVOID>(core, 2);
        const auto ResourceManagerInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength                     = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[227]);

        on_func(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationThread(proc_t proc, const on_NtQueryInformationThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle            = arg<nt64::HANDLE>(core, 0);
        const auto ThreadInformationClass  = arg<nt64::THREADINFOCLASS>(core, 1);
        const auto ThreadInformation       = arg<nt64::PVOID>(core, 2);
        const auto ThreadInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength            = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[228]);

        on_func(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationToken(proc_t proc, const on_NtQueryInformationToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationToken", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle            = arg<nt64::HANDLE>(core, 0);
        const auto TokenInformationClass  = arg<nt64::TOKEN_INFORMATION_CLASS>(core, 1);
        const auto TokenInformation       = arg<nt64::PVOID>(core, 2);
        const auto TokenInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength           = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[229]);

        on_func(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationTransaction(proc_t proc, const on_NtQueryInformationTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationTransaction", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle            = arg<nt64::HANDLE>(core, 0);
        const auto TransactionInformationClass  = arg<nt64::TRANSACTION_INFORMATION_CLASS>(core, 1);
        const auto TransactionInformation       = arg<nt64::PVOID>(core, 2);
        const auto TransactionInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength                 = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[230]);

        on_func(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationTransactionManager(proc_t proc, const on_NtQueryInformationTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationTransactionManager", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionManagerHandle            = arg<nt64::HANDLE>(core, 0);
        const auto TransactionManagerInformationClass  = arg<nt64::TRANSACTIONMANAGER_INFORMATION_CLASS>(core, 1);
        const auto TransactionManagerInformation       = arg<nt64::PVOID>(core, 2);
        const auto TransactionManagerInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength                        = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[231]);

        on_func(TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInformationWorkerFactory(proc_t proc, const on_NtQueryInformationWorkerFactory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInformationWorkerFactory", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle            = arg<nt64::HANDLE>(core, 0);
        const auto WorkerFactoryInformationClass  = arg<nt64::WORKERFACTORYINFOCLASS>(core, 1);
        const auto WorkerFactoryInformation       = arg<nt64::PVOID>(core, 2);
        const auto WorkerFactoryInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength                   = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[232]);

        on_func(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryInstallUILanguage(proc_t proc, const on_NtQueryInstallUILanguage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryInstallUILanguage", [=]
    {
        auto& core = d_->core;
        
        const auto STARInstallUILanguageId = arg<nt64::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[233]);

        on_func(STARInstallUILanguageId);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryIntervalProfile(proc_t proc, const on_NtQueryIntervalProfile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryIntervalProfile", [=]
    {
        auto& core = d_->core;
        
        const auto ProfileSource = arg<nt64::KPROFILE_SOURCE>(core, 0);
        const auto Interval      = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[234]);

        on_func(ProfileSource, Interval);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryIoCompletion(proc_t proc, const on_NtQueryIoCompletion_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryIoCompletion", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle            = arg<nt64::HANDLE>(core, 0);
        const auto IoCompletionInformationClass  = arg<nt64::IO_COMPLETION_INFORMATION_CLASS>(core, 1);
        const auto IoCompletionInformation       = arg<nt64::PVOID>(core, 2);
        const auto IoCompletionInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength                  = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[235]);

        on_func(IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryKey(proc_t proc, const on_NtQueryKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle           = arg<nt64::HANDLE>(core, 0);
        const auto KeyInformationClass = arg<nt64::KEY_INFORMATION_CLASS>(core, 1);
        const auto KeyInformation      = arg<nt64::PVOID>(core, 2);
        const auto Length              = arg<nt64::ULONG>(core, 3);
        const auto ResultLength        = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[236]);

        on_func(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryLicenseValue(proc_t proc, const on_NtQueryLicenseValue_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryLicenseValue", [=]
    {
        auto& core = d_->core;
        
        const auto Name           = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto Type           = arg<nt64::PULONG>(core, 1);
        const auto Buffer         = arg<nt64::PVOID>(core, 2);
        const auto Length         = arg<nt64::ULONG>(core, 3);
        const auto ReturnedLength = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[237]);

        on_func(Name, Type, Buffer, Length, ReturnedLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryMultipleValueKey(proc_t proc, const on_NtQueryMultipleValueKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryMultipleValueKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle            = arg<nt64::HANDLE>(core, 0);
        const auto ValueEntries         = arg<nt64::PKEY_VALUE_ENTRY>(core, 1);
        const auto EntryCount           = arg<nt64::ULONG>(core, 2);
        const auto ValueBuffer          = arg<nt64::PVOID>(core, 3);
        const auto BufferLength         = arg<nt64::PULONG>(core, 4);
        const auto RequiredBufferLength = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[238]);

        on_func(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryMutant(proc_t proc, const on_NtQueryMutant_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryMutant", [=]
    {
        auto& core = d_->core;
        
        const auto MutantHandle            = arg<nt64::HANDLE>(core, 0);
        const auto MutantInformationClass  = arg<nt64::MUTANT_INFORMATION_CLASS>(core, 1);
        const auto MutantInformation       = arg<nt64::PVOID>(core, 2);
        const auto MutantInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength            = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[239]);

        on_func(MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryObject(proc_t proc, const on_NtQueryObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryObject", [=]
    {
        auto& core = d_->core;
        
        const auto Handle                  = arg<nt64::HANDLE>(core, 0);
        const auto ObjectInformationClass  = arg<nt64::OBJECT_INFORMATION_CLASS>(core, 1);
        const auto ObjectInformation       = arg<nt64::PVOID>(core, 2);
        const auto ObjectInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength            = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[240]);

        on_func(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryOpenSubKeysEx(proc_t proc, const on_NtQueryOpenSubKeysEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryOpenSubKeysEx", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey    = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);
        const auto BufferLength = arg<nt64::ULONG>(core, 1);
        const auto Buffer       = arg<nt64::PVOID>(core, 2);
        const auto RequiredSize = arg<nt64::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[241]);

        on_func(TargetKey, BufferLength, Buffer, RequiredSize);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryOpenSubKeys(proc_t proc, const on_NtQueryOpenSubKeys_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryOpenSubKeys", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey   = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);
        const auto HandleCount = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[242]);

        on_func(TargetKey, HandleCount);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryPerformanceCounter(proc_t proc, const on_NtQueryPerformanceCounter_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryPerformanceCounter", [=]
    {
        auto& core = d_->core;
        
        const auto PerformanceCounter   = arg<nt64::PLARGE_INTEGER>(core, 0);
        const auto PerformanceFrequency = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[243]);

        on_func(PerformanceCounter, PerformanceFrequency);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryQuotaInformationFile(proc_t proc, const on_NtQueryQuotaInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryQuotaInformationFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle        = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock     = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer            = arg<nt64::PVOID>(core, 2);
        const auto Length            = arg<nt64::ULONG>(core, 3);
        const auto ReturnSingleEntry = arg<nt64::BOOLEAN>(core, 4);
        const auto SidList           = arg<nt64::PVOID>(core, 5);
        const auto SidListLength     = arg<nt64::ULONG>(core, 6);
        const auto StartSid          = arg<nt64::PULONG>(core, 7);
        const auto RestartScan       = arg<nt64::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[244]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan);
    });
}

opt<id_t> nt64::syscalls::register_NtQuerySection(proc_t proc, const on_NtQuerySection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQuerySection", [=]
    {
        auto& core = d_->core;
        
        const auto SectionHandle            = arg<nt64::HANDLE>(core, 0);
        const auto SectionInformationClass  = arg<nt64::SECTION_INFORMATION_CLASS>(core, 1);
        const auto SectionInformation       = arg<nt64::PVOID>(core, 2);
        const auto SectionInformationLength = arg<nt64::SIZE_T>(core, 3);
        const auto ReturnLength             = arg<nt64::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[245]);

        on_func(SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQuerySecurityAttributesToken(proc_t proc, const on_NtQuerySecurityAttributesToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQuerySecurityAttributesToken", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle        = arg<nt64::HANDLE>(core, 0);
        const auto Attributes         = arg<nt64::PUNICODE_STRING>(core, 1);
        const auto NumberOfAttributes = arg<nt64::ULONG>(core, 2);
        const auto Buffer             = arg<nt64::PVOID>(core, 3);
        const auto Length             = arg<nt64::ULONG>(core, 4);
        const auto ReturnLength       = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[246]);

        on_func(TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQuerySecurityObject(proc_t proc, const on_NtQuerySecurityObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQuerySecurityObject", [=]
    {
        auto& core = d_->core;
        
        const auto Handle              = arg<nt64::HANDLE>(core, 0);
        const auto SecurityInformation = arg<nt64::SECURITY_INFORMATION>(core, 1);
        const auto SecurityDescriptor  = arg<nt64::PSECURITY_DESCRIPTOR>(core, 2);
        const auto Length              = arg<nt64::ULONG>(core, 3);
        const auto LengthNeeded        = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[247]);

        on_func(Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded);
    });
}

opt<id_t> nt64::syscalls::register_NtQuerySemaphore(proc_t proc, const on_NtQuerySemaphore_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQuerySemaphore", [=]
    {
        auto& core = d_->core;
        
        const auto SemaphoreHandle            = arg<nt64::HANDLE>(core, 0);
        const auto SemaphoreInformationClass  = arg<nt64::SEMAPHORE_INFORMATION_CLASS>(core, 1);
        const auto SemaphoreInformation       = arg<nt64::PVOID>(core, 2);
        const auto SemaphoreInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength               = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[248]);

        on_func(SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQuerySymbolicLinkObject(proc_t proc, const on_NtQuerySymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQuerySymbolicLinkObject", [=]
    {
        auto& core = d_->core;
        
        const auto LinkHandle     = arg<nt64::HANDLE>(core, 0);
        const auto LinkTarget     = arg<nt64::PUNICODE_STRING>(core, 1);
        const auto ReturnedLength = arg<nt64::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[249]);

        on_func(LinkHandle, LinkTarget, ReturnedLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQuerySystemEnvironmentValueEx(proc_t proc, const on_NtQuerySystemEnvironmentValueEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQuerySystemEnvironmentValueEx", [=]
    {
        auto& core = d_->core;
        
        const auto VariableName = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto VendorGuid   = arg<nt64::LPGUID>(core, 1);
        const auto Value        = arg<nt64::PVOID>(core, 2);
        const auto ValueLength  = arg<nt64::PULONG>(core, 3);
        const auto Attributes   = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[250]);

        on_func(VariableName, VendorGuid, Value, ValueLength, Attributes);
    });
}

opt<id_t> nt64::syscalls::register_NtQuerySystemEnvironmentValue(proc_t proc, const on_NtQuerySystemEnvironmentValue_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQuerySystemEnvironmentValue", [=]
    {
        auto& core = d_->core;
        
        const auto VariableName  = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto VariableValue = arg<nt64::PWSTR>(core, 1);
        const auto ValueLength   = arg<nt64::USHORT>(core, 2);
        const auto ReturnLength  = arg<nt64::PUSHORT>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[251]);

        on_func(VariableName, VariableValue, ValueLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQuerySystemInformationEx(proc_t proc, const on_NtQuerySystemInformationEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQuerySystemInformationEx", [=]
    {
        auto& core = d_->core;
        
        const auto SystemInformationClass  = arg<nt64::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto QueryInformation        = arg<nt64::PVOID>(core, 1);
        const auto QueryInformationLength  = arg<nt64::ULONG>(core, 2);
        const auto SystemInformation       = arg<nt64::PVOID>(core, 3);
        const auto SystemInformationLength = arg<nt64::ULONG>(core, 4);
        const auto ReturnLength            = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[252]);

        on_func(SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQuerySystemInformation(proc_t proc, const on_NtQuerySystemInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQuerySystemInformation", [=]
    {
        auto& core = d_->core;
        
        const auto SystemInformationClass  = arg<nt64::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto SystemInformation       = arg<nt64::PVOID>(core, 1);
        const auto SystemInformationLength = arg<nt64::ULONG>(core, 2);
        const auto ReturnLength            = arg<nt64::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[253]);

        on_func(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQuerySystemTime(proc_t proc, const on_NtQuerySystemTime_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQuerySystemTime", [=]
    {
        auto& core = d_->core;
        
        const auto SystemTime = arg<nt64::PLARGE_INTEGER>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[254]);

        on_func(SystemTime);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryTimer(proc_t proc, const on_NtQueryTimer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryTimer", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle            = arg<nt64::HANDLE>(core, 0);
        const auto TimerInformationClass  = arg<nt64::TIMER_INFORMATION_CLASS>(core, 1);
        const auto TimerInformation       = arg<nt64::PVOID>(core, 2);
        const auto TimerInformationLength = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength           = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[255]);

        on_func(TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryTimerResolution(proc_t proc, const on_NtQueryTimerResolution_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryTimerResolution", [=]
    {
        auto& core = d_->core;
        
        const auto MaximumTime = arg<nt64::PULONG>(core, 0);
        const auto MinimumTime = arg<nt64::PULONG>(core, 1);
        const auto CurrentTime = arg<nt64::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[256]);

        on_func(MaximumTime, MinimumTime, CurrentTime);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryValueKey(proc_t proc, const on_NtQueryValueKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryValueKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle                = arg<nt64::HANDLE>(core, 0);
        const auto ValueName                = arg<nt64::PUNICODE_STRING>(core, 1);
        const auto KeyValueInformationClass = arg<nt64::KEY_VALUE_INFORMATION_CLASS>(core, 2);
        const auto KeyValueInformation      = arg<nt64::PVOID>(core, 3);
        const auto Length                   = arg<nt64::ULONG>(core, 4);
        const auto ResultLength             = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[257]);

        on_func(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryVirtualMemory(proc_t proc, const on_NtQueryVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryVirtualMemory", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle           = arg<nt64::HANDLE>(core, 0);
        const auto BaseAddress             = arg<nt64::PVOID>(core, 1);
        const auto MemoryInformationClass  = arg<nt64::MEMORY_INFORMATION_CLASS>(core, 2);
        const auto MemoryInformation       = arg<nt64::PVOID>(core, 3);
        const auto MemoryInformationLength = arg<nt64::SIZE_T>(core, 4);
        const auto ReturnLength            = arg<nt64::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[258]);

        on_func(ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtQueryVolumeInformationFile(proc_t proc, const on_NtQueryVolumeInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryVolumeInformationFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle         = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock      = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto FsInformation      = arg<nt64::PVOID>(core, 2);
        const auto Length             = arg<nt64::ULONG>(core, 3);
        const auto FsInformationClass = arg<nt64::FS_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[259]);

        on_func(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    });
}

opt<id_t> nt64::syscalls::register_NtQueueApcThreadEx(proc_t proc, const on_NtQueueApcThreadEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueueApcThreadEx", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle         = arg<nt64::HANDLE>(core, 0);
        const auto UserApcReserveHandle = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine           = arg<nt64::PPS_APC_ROUTINE>(core, 2);
        const auto ApcArgument1         = arg<nt64::PVOID>(core, 3);
        const auto ApcArgument2         = arg<nt64::PVOID>(core, 4);
        const auto ApcArgument3         = arg<nt64::PVOID>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[260]);

        on_func(ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    });
}

opt<id_t> nt64::syscalls::register_NtQueueApcThread(proc_t proc, const on_NtQueueApcThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueueApcThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle = arg<nt64::HANDLE>(core, 0);
        const auto ApcRoutine   = arg<nt64::PPS_APC_ROUTINE>(core, 1);
        const auto ApcArgument1 = arg<nt64::PVOID>(core, 2);
        const auto ApcArgument2 = arg<nt64::PVOID>(core, 3);
        const auto ApcArgument3 = arg<nt64::PVOID>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[261]);

        on_func(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    });
}

opt<id_t> nt64::syscalls::register_NtRaiseException(proc_t proc, const on_NtRaiseException_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRaiseException", [=]
    {
        auto& core = d_->core;
        
        const auto ExceptionRecord = arg<nt64::PEXCEPTION_RECORD>(core, 0);
        const auto ContextRecord   = arg<nt64::PCONTEXT>(core, 1);
        const auto FirstChance     = arg<nt64::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[262]);

        on_func(ExceptionRecord, ContextRecord, FirstChance);
    });
}

opt<id_t> nt64::syscalls::register_NtRaiseHardError(proc_t proc, const on_NtRaiseHardError_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRaiseHardError", [=]
    {
        auto& core = d_->core;
        
        const auto ErrorStatus                = arg<nt64::NTSTATUS>(core, 0);
        const auto NumberOfParameters         = arg<nt64::ULONG>(core, 1);
        const auto UnicodeStringParameterMask = arg<nt64::ULONG>(core, 2);
        const auto Parameters                 = arg<nt64::PULONG_PTR>(core, 3);
        const auto ValidResponseOptions       = arg<nt64::ULONG>(core, 4);
        const auto Response                   = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[263]);

        on_func(ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);
    });
}

opt<id_t> nt64::syscalls::register_NtReadFile(proc_t proc, const on_NtReadFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReadFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt64::HANDLE>(core, 0);
        const auto Event         = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt64::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt64::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer        = arg<nt64::PVOID>(core, 5);
        const auto Length        = arg<nt64::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt64::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt64::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[264]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    });
}

opt<id_t> nt64::syscalls::register_NtReadFileScatter(proc_t proc, const on_NtReadFileScatter_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReadFileScatter", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt64::HANDLE>(core, 0);
        const auto Event         = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt64::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt64::PIO_STATUS_BLOCK>(core, 4);
        const auto SegmentArray  = arg<nt64::PFILE_SEGMENT_ELEMENT>(core, 5);
        const auto Length        = arg<nt64::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt64::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt64::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[265]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    });
}

opt<id_t> nt64::syscalls::register_NtReadOnlyEnlistment(proc_t proc, const on_NtReadOnlyEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReadOnlyEnlistment", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[266]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtReadRequestData(proc_t proc, const on_NtReadRequestData_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReadRequestData", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle        = arg<nt64::HANDLE>(core, 0);
        const auto Message           = arg<nt64::PPORT_MESSAGE>(core, 1);
        const auto DataEntryIndex    = arg<nt64::ULONG>(core, 2);
        const auto Buffer            = arg<nt64::PVOID>(core, 3);
        const auto BufferSize        = arg<nt64::SIZE_T>(core, 4);
        const auto NumberOfBytesRead = arg<nt64::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[267]);

        on_func(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead);
    });
}

opt<id_t> nt64::syscalls::register_NtReadVirtualMemory(proc_t proc, const on_NtReadVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReadVirtualMemory", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle     = arg<nt64::HANDLE>(core, 0);
        const auto BaseAddress       = arg<nt64::PVOID>(core, 1);
        const auto Buffer            = arg<nt64::PVOID>(core, 2);
        const auto BufferSize        = arg<nt64::SIZE_T>(core, 3);
        const auto NumberOfBytesRead = arg<nt64::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[268]);

        on_func(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead);
    });
}

opt<id_t> nt64::syscalls::register_NtRecoverEnlistment(proc_t proc, const on_NtRecoverEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRecoverEnlistment", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto EnlistmentKey    = arg<nt64::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[269]);

        on_func(EnlistmentHandle, EnlistmentKey);
    });
}

opt<id_t> nt64::syscalls::register_NtRecoverResourceManager(proc_t proc, const on_NtRecoverResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRecoverResourceManager", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[270]);

        on_func(ResourceManagerHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtRecoverTransactionManager(proc_t proc, const on_NtRecoverTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRecoverTransactionManager", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionManagerHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[271]);

        on_func(TransactionManagerHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtRegisterProtocolAddressInformation(proc_t proc, const on_NtRegisterProtocolAddressInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRegisterProtocolAddressInformation", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManager         = arg<nt64::HANDLE>(core, 0);
        const auto ProtocolId              = arg<nt64::PCRM_PROTOCOL_ID>(core, 1);
        const auto ProtocolInformationSize = arg<nt64::ULONG>(core, 2);
        const auto ProtocolInformation     = arg<nt64::PVOID>(core, 3);
        const auto CreateOptions           = arg<nt64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[272]);

        on_func(ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions);
    });
}

opt<id_t> nt64::syscalls::register_NtRegisterThreadTerminatePort(proc_t proc, const on_NtRegisterThreadTerminatePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRegisterThreadTerminatePort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[273]);

        on_func(PortHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtReleaseKeyedEvent(proc_t proc, const on_NtReleaseKeyedEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReleaseKeyedEvent", [=]
    {
        auto& core = d_->core;
        
        const auto KeyedEventHandle = arg<nt64::HANDLE>(core, 0);
        const auto KeyValue         = arg<nt64::PVOID>(core, 1);
        const auto Alertable        = arg<nt64::BOOLEAN>(core, 2);
        const auto Timeout          = arg<nt64::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[274]);

        on_func(KeyedEventHandle, KeyValue, Alertable, Timeout);
    });
}

opt<id_t> nt64::syscalls::register_NtReleaseMutant(proc_t proc, const on_NtReleaseMutant_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReleaseMutant", [=]
    {
        auto& core = d_->core;
        
        const auto MutantHandle  = arg<nt64::HANDLE>(core, 0);
        const auto PreviousCount = arg<nt64::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[275]);

        on_func(MutantHandle, PreviousCount);
    });
}

opt<id_t> nt64::syscalls::register_NtReleaseSemaphore(proc_t proc, const on_NtReleaseSemaphore_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReleaseSemaphore", [=]
    {
        auto& core = d_->core;
        
        const auto SemaphoreHandle = arg<nt64::HANDLE>(core, 0);
        const auto ReleaseCount    = arg<nt64::LONG>(core, 1);
        const auto PreviousCount   = arg<nt64::PLONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[276]);

        on_func(SemaphoreHandle, ReleaseCount, PreviousCount);
    });
}

opt<id_t> nt64::syscalls::register_NtReleaseWorkerFactoryWorker(proc_t proc, const on_NtReleaseWorkerFactoryWorker_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReleaseWorkerFactoryWorker", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[277]);

        on_func(WorkerFactoryHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtRemoveIoCompletionEx(proc_t proc, const on_NtRemoveIoCompletionEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRemoveIoCompletionEx", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle      = arg<nt64::HANDLE>(core, 0);
        const auto IoCompletionInformation = arg<nt64::PFILE_IO_COMPLETION_INFORMATION>(core, 1);
        const auto Count                   = arg<nt64::ULONG>(core, 2);
        const auto NumEntriesRemoved       = arg<nt64::PULONG>(core, 3);
        const auto Timeout                 = arg<nt64::PLARGE_INTEGER>(core, 4);
        const auto Alertable               = arg<nt64::BOOLEAN>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[278]);

        on_func(IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable);
    });
}

opt<id_t> nt64::syscalls::register_NtRemoveIoCompletion(proc_t proc, const on_NtRemoveIoCompletion_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRemoveIoCompletion", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle = arg<nt64::HANDLE>(core, 0);
        const auto STARKeyContext     = arg<nt64::PVOID>(core, 1);
        const auto STARApcContext     = arg<nt64::PVOID>(core, 2);
        const auto IoStatusBlock      = arg<nt64::PIO_STATUS_BLOCK>(core, 3);
        const auto Timeout            = arg<nt64::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[279]);

        on_func(IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout);
    });
}

opt<id_t> nt64::syscalls::register_NtRemoveProcessDebug(proc_t proc, const on_NtRemoveProcessDebug_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRemoveProcessDebug", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle     = arg<nt64::HANDLE>(core, 0);
        const auto DebugObjectHandle = arg<nt64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[280]);

        on_func(ProcessHandle, DebugObjectHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtRenameKey(proc_t proc, const on_NtRenameKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRenameKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle = arg<nt64::HANDLE>(core, 0);
        const auto NewName   = arg<nt64::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[281]);

        on_func(KeyHandle, NewName);
    });
}

opt<id_t> nt64::syscalls::register_NtRenameTransactionManager(proc_t proc, const on_NtRenameTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRenameTransactionManager", [=]
    {
        auto& core = d_->core;
        
        const auto LogFileName                    = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto ExistingTransactionManagerGuid = arg<nt64::LPGUID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[282]);

        on_func(LogFileName, ExistingTransactionManagerGuid);
    });
}

opt<id_t> nt64::syscalls::register_NtReplaceKey(proc_t proc, const on_NtReplaceKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReplaceKey", [=]
    {
        auto& core = d_->core;
        
        const auto NewFile      = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);
        const auto TargetHandle = arg<nt64::HANDLE>(core, 1);
        const auto OldFile      = arg<nt64::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[283]);

        on_func(NewFile, TargetHandle, OldFile);
    });
}

opt<id_t> nt64::syscalls::register_NtReplacePartitionUnit(proc_t proc, const on_NtReplacePartitionUnit_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReplacePartitionUnit", [=]
    {
        auto& core = d_->core;
        
        const auto TargetInstancePath = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto SpareInstancePath  = arg<nt64::PUNICODE_STRING>(core, 1);
        const auto Flags              = arg<nt64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[284]);

        on_func(TargetInstancePath, SpareInstancePath, Flags);
    });
}

opt<id_t> nt64::syscalls::register_NtReplyPort(proc_t proc, const on_NtReplyPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReplyPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle   = arg<nt64::HANDLE>(core, 0);
        const auto ReplyMessage = arg<nt64::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[285]);

        on_func(PortHandle, ReplyMessage);
    });
}

opt<id_t> nt64::syscalls::register_NtReplyWaitReceivePortEx(proc_t proc, const on_NtReplyWaitReceivePortEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReplyWaitReceivePortEx", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle      = arg<nt64::HANDLE>(core, 0);
        const auto STARPortContext = arg<nt64::PVOID>(core, 1);
        const auto ReplyMessage    = arg<nt64::PPORT_MESSAGE>(core, 2);
        const auto ReceiveMessage  = arg<nt64::PPORT_MESSAGE>(core, 3);
        const auto Timeout         = arg<nt64::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[286]);

        on_func(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout);
    });
}

opt<id_t> nt64::syscalls::register_NtReplyWaitReceivePort(proc_t proc, const on_NtReplyWaitReceivePort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReplyWaitReceivePort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle      = arg<nt64::HANDLE>(core, 0);
        const auto STARPortContext = arg<nt64::PVOID>(core, 1);
        const auto ReplyMessage    = arg<nt64::PPORT_MESSAGE>(core, 2);
        const auto ReceiveMessage  = arg<nt64::PPORT_MESSAGE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[287]);

        on_func(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage);
    });
}

opt<id_t> nt64::syscalls::register_NtReplyWaitReplyPort(proc_t proc, const on_NtReplyWaitReplyPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtReplyWaitReplyPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle   = arg<nt64::HANDLE>(core, 0);
        const auto ReplyMessage = arg<nt64::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[288]);

        on_func(PortHandle, ReplyMessage);
    });
}

opt<id_t> nt64::syscalls::register_NtRequestPort(proc_t proc, const on_NtRequestPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRequestPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle     = arg<nt64::HANDLE>(core, 0);
        const auto RequestMessage = arg<nt64::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[289]);

        on_func(PortHandle, RequestMessage);
    });
}

opt<id_t> nt64::syscalls::register_NtRequestWaitReplyPort(proc_t proc, const on_NtRequestWaitReplyPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRequestWaitReplyPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle     = arg<nt64::HANDLE>(core, 0);
        const auto RequestMessage = arg<nt64::PPORT_MESSAGE>(core, 1);
        const auto ReplyMessage   = arg<nt64::PPORT_MESSAGE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[290]);

        on_func(PortHandle, RequestMessage, ReplyMessage);
    });
}

opt<id_t> nt64::syscalls::register_NtResetEvent(proc_t proc, const on_NtResetEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtResetEvent", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle   = arg<nt64::HANDLE>(core, 0);
        const auto PreviousState = arg<nt64::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[291]);

        on_func(EventHandle, PreviousState);
    });
}

opt<id_t> nt64::syscalls::register_NtResetWriteWatch(proc_t proc, const on_NtResetWriteWatch_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtResetWriteWatch", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 0);
        const auto BaseAddress   = arg<nt64::PVOID>(core, 1);
        const auto RegionSize    = arg<nt64::SIZE_T>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[292]);

        on_func(ProcessHandle, BaseAddress, RegionSize);
    });
}

opt<id_t> nt64::syscalls::register_NtRestoreKey(proc_t proc, const on_NtRestoreKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRestoreKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle  = arg<nt64::HANDLE>(core, 0);
        const auto FileHandle = arg<nt64::HANDLE>(core, 1);
        const auto Flags      = arg<nt64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[293]);

        on_func(KeyHandle, FileHandle, Flags);
    });
}

opt<id_t> nt64::syscalls::register_NtResumeProcess(proc_t proc, const on_NtResumeProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtResumeProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[294]);

        on_func(ProcessHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtResumeThread(proc_t proc, const on_NtResumeThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtResumeThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle         = arg<nt64::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[295]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<id_t> nt64::syscalls::register_NtRollbackComplete(proc_t proc, const on_NtRollbackComplete_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRollbackComplete", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[296]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtRollbackEnlistment(proc_t proc, const on_NtRollbackEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRollbackEnlistment", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[297]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtRollbackTransaction(proc_t proc, const on_NtRollbackTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRollbackTransaction", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle = arg<nt64::HANDLE>(core, 0);
        const auto Wait              = arg<nt64::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[298]);

        on_func(TransactionHandle, Wait);
    });
}

opt<id_t> nt64::syscalls::register_NtRollforwardTransactionManager(proc_t proc, const on_NtRollforwardTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtRollforwardTransactionManager", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionManagerHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock           = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[299]);

        on_func(TransactionManagerHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtSaveKeyEx(proc_t proc, const on_NtSaveKeyEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSaveKeyEx", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle  = arg<nt64::HANDLE>(core, 0);
        const auto FileHandle = arg<nt64::HANDLE>(core, 1);
        const auto Format     = arg<nt64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[300]);

        on_func(KeyHandle, FileHandle, Format);
    });
}

opt<id_t> nt64::syscalls::register_NtSaveKey(proc_t proc, const on_NtSaveKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSaveKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle  = arg<nt64::HANDLE>(core, 0);
        const auto FileHandle = arg<nt64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[301]);

        on_func(KeyHandle, FileHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtSaveMergedKeys(proc_t proc, const on_NtSaveMergedKeys_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSaveMergedKeys", [=]
    {
        auto& core = d_->core;
        
        const auto HighPrecedenceKeyHandle = arg<nt64::HANDLE>(core, 0);
        const auto LowPrecedenceKeyHandle  = arg<nt64::HANDLE>(core, 1);
        const auto FileHandle              = arg<nt64::HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[302]);

        on_func(HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtSecureConnectPort(proc_t proc, const on_NtSecureConnectPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSecureConnectPort", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle                  = arg<nt64::PHANDLE>(core, 0);
        const auto PortName                    = arg<nt64::PUNICODE_STRING>(core, 1);
        const auto SecurityQos                 = arg<nt64::PSECURITY_QUALITY_OF_SERVICE>(core, 2);
        const auto ClientView                  = arg<nt64::PPORT_VIEW>(core, 3);
        const auto RequiredServerSid           = arg<nt64::PSID>(core, 4);
        const auto ServerView                  = arg<nt64::PREMOTE_PORT_VIEW>(core, 5);
        const auto MaxMessageLength            = arg<nt64::PULONG>(core, 6);
        const auto ConnectionInformation       = arg<nt64::PVOID>(core, 7);
        const auto ConnectionInformationLength = arg<nt64::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[303]);

        on_func(PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetBootEntryOrder(proc_t proc, const on_NtSetBootEntryOrder_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetBootEntryOrder", [=]
    {
        auto& core = d_->core;
        
        const auto Ids   = arg<nt64::PULONG>(core, 0);
        const auto Count = arg<nt64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[304]);

        on_func(Ids, Count);
    });
}

opt<id_t> nt64::syscalls::register_NtSetBootOptions(proc_t proc, const on_NtSetBootOptions_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetBootOptions", [=]
    {
        auto& core = d_->core;
        
        const auto BootOptions    = arg<nt64::PBOOT_OPTIONS>(core, 0);
        const auto FieldsToChange = arg<nt64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[305]);

        on_func(BootOptions, FieldsToChange);
    });
}

opt<id_t> nt64::syscalls::register_NtSetContextThread(proc_t proc, const on_NtSetContextThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetContextThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle  = arg<nt64::HANDLE>(core, 0);
        const auto ThreadContext = arg<nt64::PCONTEXT>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[306]);

        on_func(ThreadHandle, ThreadContext);
    });
}

opt<id_t> nt64::syscalls::register_NtSetDebugFilterState(proc_t proc, const on_NtSetDebugFilterState_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetDebugFilterState", [=]
    {
        auto& core = d_->core;
        
        const auto ComponentId = arg<nt64::ULONG>(core, 0);
        const auto Level       = arg<nt64::ULONG>(core, 1);
        const auto State       = arg<nt64::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[307]);

        on_func(ComponentId, Level, State);
    });
}

opt<id_t> nt64::syscalls::register_NtSetDefaultHardErrorPort(proc_t proc, const on_NtSetDefaultHardErrorPort_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetDefaultHardErrorPort", [=]
    {
        auto& core = d_->core;
        
        const auto DefaultHardErrorPort = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[308]);

        on_func(DefaultHardErrorPort);
    });
}

opt<id_t> nt64::syscalls::register_NtSetDefaultLocale(proc_t proc, const on_NtSetDefaultLocale_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetDefaultLocale", [=]
    {
        auto& core = d_->core;
        
        const auto UserProfile     = arg<nt64::BOOLEAN>(core, 0);
        const auto DefaultLocaleId = arg<nt64::LCID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[309]);

        on_func(UserProfile, DefaultLocaleId);
    });
}

opt<id_t> nt64::syscalls::register_NtSetDefaultUILanguage(proc_t proc, const on_NtSetDefaultUILanguage_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetDefaultUILanguage", [=]
    {
        auto& core = d_->core;
        
        const auto DefaultUILanguageId = arg<nt64::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[310]);

        on_func(DefaultUILanguageId);
    });
}

opt<id_t> nt64::syscalls::register_NtSetDriverEntryOrder(proc_t proc, const on_NtSetDriverEntryOrder_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetDriverEntryOrder", [=]
    {
        auto& core = d_->core;
        
        const auto Ids   = arg<nt64::PULONG>(core, 0);
        const auto Count = arg<nt64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[311]);

        on_func(Ids, Count);
    });
}

opt<id_t> nt64::syscalls::register_NtSetEaFile(proc_t proc, const on_NtSetEaFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetEaFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer        = arg<nt64::PVOID>(core, 2);
        const auto Length        = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[312]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length);
    });
}

opt<id_t> nt64::syscalls::register_NtSetEventBoostPriority(proc_t proc, const on_NtSetEventBoostPriority_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetEventBoostPriority", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[313]);

        on_func(EventHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtSetEvent(proc_t proc, const on_NtSetEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetEvent", [=]
    {
        auto& core = d_->core;
        
        const auto EventHandle   = arg<nt64::HANDLE>(core, 0);
        const auto PreviousState = arg<nt64::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[314]);

        on_func(EventHandle, PreviousState);
    });
}

opt<id_t> nt64::syscalls::register_NtSetHighEventPair(proc_t proc, const on_NtSetHighEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetHighEventPair", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[315]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtSetHighWaitLowEventPair(proc_t proc, const on_NtSetHighWaitLowEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetHighWaitLowEventPair", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[316]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationDebugObject(proc_t proc, const on_NtSetInformationDebugObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationDebugObject", [=]
    {
        auto& core = d_->core;
        
        const auto DebugObjectHandle           = arg<nt64::HANDLE>(core, 0);
        const auto DebugObjectInformationClass = arg<nt64::DEBUGOBJECTINFOCLASS>(core, 1);
        const auto DebugInformation            = arg<nt64::PVOID>(core, 2);
        const auto DebugInformationLength      = arg<nt64::ULONG>(core, 3);
        const auto ReturnLength                = arg<nt64::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[317]);

        on_func(DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationEnlistment(proc_t proc, const on_NtSetInformationEnlistment_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationEnlistment", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle            = arg<nt64::HANDLE>(core, 0);
        const auto EnlistmentInformationClass  = arg<nt64::ENLISTMENT_INFORMATION_CLASS>(core, 1);
        const auto EnlistmentInformation       = arg<nt64::PVOID>(core, 2);
        const auto EnlistmentInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[318]);

        on_func(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationFile(proc_t proc, const on_NtSetInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle           = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock        = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto FileInformation      = arg<nt64::PVOID>(core, 2);
        const auto Length               = arg<nt64::ULONG>(core, 3);
        const auto FileInformationClass = arg<nt64::FILE_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[319]);

        on_func(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationJobObject(proc_t proc, const on_NtSetInformationJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationJobObject", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle                  = arg<nt64::HANDLE>(core, 0);
        const auto JobObjectInformationClass  = arg<nt64::JOBOBJECTINFOCLASS>(core, 1);
        const auto JobObjectInformation       = arg<nt64::PVOID>(core, 2);
        const auto JobObjectInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[320]);

        on_func(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationKey(proc_t proc, const on_NtSetInformationKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle               = arg<nt64::HANDLE>(core, 0);
        const auto KeySetInformationClass  = arg<nt64::KEY_SET_INFORMATION_CLASS>(core, 1);
        const auto KeySetInformation       = arg<nt64::PVOID>(core, 2);
        const auto KeySetInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[321]);

        on_func(KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationObject(proc_t proc, const on_NtSetInformationObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationObject", [=]
    {
        auto& core = d_->core;
        
        const auto Handle                  = arg<nt64::HANDLE>(core, 0);
        const auto ObjectInformationClass  = arg<nt64::OBJECT_INFORMATION_CLASS>(core, 1);
        const auto ObjectInformation       = arg<nt64::PVOID>(core, 2);
        const auto ObjectInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[322]);

        on_func(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationProcess(proc_t proc, const on_NtSetInformationProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle            = arg<nt64::HANDLE>(core, 0);
        const auto ProcessInformationClass  = arg<nt64::PROCESSINFOCLASS>(core, 1);
        const auto ProcessInformation       = arg<nt64::PVOID>(core, 2);
        const auto ProcessInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[323]);

        on_func(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationResourceManager(proc_t proc, const on_NtSetInformationResourceManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationResourceManager", [=]
    {
        auto& core = d_->core;
        
        const auto ResourceManagerHandle            = arg<nt64::HANDLE>(core, 0);
        const auto ResourceManagerInformationClass  = arg<nt64::RESOURCEMANAGER_INFORMATION_CLASS>(core, 1);
        const auto ResourceManagerInformation       = arg<nt64::PVOID>(core, 2);
        const auto ResourceManagerInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[324]);

        on_func(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationThread(proc_t proc, const on_NtSetInformationThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle            = arg<nt64::HANDLE>(core, 0);
        const auto ThreadInformationClass  = arg<nt64::THREADINFOCLASS>(core, 1);
        const auto ThreadInformation       = arg<nt64::PVOID>(core, 2);
        const auto ThreadInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[325]);

        on_func(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationToken(proc_t proc, const on_NtSetInformationToken_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationToken", [=]
    {
        auto& core = d_->core;
        
        const auto TokenHandle            = arg<nt64::HANDLE>(core, 0);
        const auto TokenInformationClass  = arg<nt64::TOKEN_INFORMATION_CLASS>(core, 1);
        const auto TokenInformation       = arg<nt64::PVOID>(core, 2);
        const auto TokenInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[326]);

        on_func(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationTransaction(proc_t proc, const on_NtSetInformationTransaction_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationTransaction", [=]
    {
        auto& core = d_->core;
        
        const auto TransactionHandle            = arg<nt64::HANDLE>(core, 0);
        const auto TransactionInformationClass  = arg<nt64::TRANSACTION_INFORMATION_CLASS>(core, 1);
        const auto TransactionInformation       = arg<nt64::PVOID>(core, 2);
        const auto TransactionInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[327]);

        on_func(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationTransactionManager(proc_t proc, const on_NtSetInformationTransactionManager_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationTransactionManager", [=]
    {
        auto& core = d_->core;
        
        const auto TmHandle                            = arg<nt64::HANDLE>(core, 0);
        const auto TransactionManagerInformationClass  = arg<nt64::TRANSACTIONMANAGER_INFORMATION_CLASS>(core, 1);
        const auto TransactionManagerInformation       = arg<nt64::PVOID>(core, 2);
        const auto TransactionManagerInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[328]);

        on_func(TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetInformationWorkerFactory(proc_t proc, const on_NtSetInformationWorkerFactory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetInformationWorkerFactory", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle            = arg<nt64::HANDLE>(core, 0);
        const auto WorkerFactoryInformationClass  = arg<nt64::WORKERFACTORYINFOCLASS>(core, 1);
        const auto WorkerFactoryInformation       = arg<nt64::PVOID>(core, 2);
        const auto WorkerFactoryInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[329]);

        on_func(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetIntervalProfile(proc_t proc, const on_NtSetIntervalProfile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetIntervalProfile", [=]
    {
        auto& core = d_->core;
        
        const auto Interval = arg<nt64::ULONG>(core, 0);
        const auto Source   = arg<nt64::KPROFILE_SOURCE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[330]);

        on_func(Interval, Source);
    });
}

opt<id_t> nt64::syscalls::register_NtSetIoCompletionEx(proc_t proc, const on_NtSetIoCompletionEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetIoCompletionEx", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle        = arg<nt64::HANDLE>(core, 0);
        const auto IoCompletionReserveHandle = arg<nt64::HANDLE>(core, 1);
        const auto KeyContext                = arg<nt64::PVOID>(core, 2);
        const auto ApcContext                = arg<nt64::PVOID>(core, 3);
        const auto IoStatus                  = arg<nt64::NTSTATUS>(core, 4);
        const auto IoStatusInformation       = arg<nt64::ULONG_PTR>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[331]);

        on_func(IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    });
}

opt<id_t> nt64::syscalls::register_NtSetIoCompletion(proc_t proc, const on_NtSetIoCompletion_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetIoCompletion", [=]
    {
        auto& core = d_->core;
        
        const auto IoCompletionHandle  = arg<nt64::HANDLE>(core, 0);
        const auto KeyContext          = arg<nt64::PVOID>(core, 1);
        const auto ApcContext          = arg<nt64::PVOID>(core, 2);
        const auto IoStatus            = arg<nt64::NTSTATUS>(core, 3);
        const auto IoStatusInformation = arg<nt64::ULONG_PTR>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[332]);

        on_func(IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    });
}

opt<id_t> nt64::syscalls::register_NtSetLdtEntries(proc_t proc, const on_NtSetLdtEntries_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetLdtEntries", [=]
    {
        auto& core = d_->core;
        
        const auto Selector0 = arg<nt64::ULONG>(core, 0);
        const auto Entry0Low = arg<nt64::ULONG>(core, 1);
        const auto Entry0Hi  = arg<nt64::ULONG>(core, 2);
        const auto Selector1 = arg<nt64::ULONG>(core, 3);
        const auto Entry1Low = arg<nt64::ULONG>(core, 4);
        const auto Entry1Hi  = arg<nt64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[333]);

        on_func(Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);
    });
}

opt<id_t> nt64::syscalls::register_NtSetLowEventPair(proc_t proc, const on_NtSetLowEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetLowEventPair", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[334]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtSetLowWaitHighEventPair(proc_t proc, const on_NtSetLowWaitHighEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetLowWaitHighEventPair", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[335]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtSetQuotaInformationFile(proc_t proc, const on_NtSetQuotaInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetQuotaInformationFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer        = arg<nt64::PVOID>(core, 2);
        const auto Length        = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[336]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length);
    });
}

opt<id_t> nt64::syscalls::register_NtSetSecurityObject(proc_t proc, const on_NtSetSecurityObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetSecurityObject", [=]
    {
        auto& core = d_->core;
        
        const auto Handle              = arg<nt64::HANDLE>(core, 0);
        const auto SecurityInformation = arg<nt64::SECURITY_INFORMATION>(core, 1);
        const auto SecurityDescriptor  = arg<nt64::PSECURITY_DESCRIPTOR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[337]);

        on_func(Handle, SecurityInformation, SecurityDescriptor);
    });
}

opt<id_t> nt64::syscalls::register_NtSetSystemEnvironmentValueEx(proc_t proc, const on_NtSetSystemEnvironmentValueEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetSystemEnvironmentValueEx", [=]
    {
        auto& core = d_->core;
        
        const auto VariableName = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto VendorGuid   = arg<nt64::LPGUID>(core, 1);
        const auto Value        = arg<nt64::PVOID>(core, 2);
        const auto ValueLength  = arg<nt64::ULONG>(core, 3);
        const auto Attributes   = arg<nt64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[338]);

        on_func(VariableName, VendorGuid, Value, ValueLength, Attributes);
    });
}

opt<id_t> nt64::syscalls::register_NtSetSystemEnvironmentValue(proc_t proc, const on_NtSetSystemEnvironmentValue_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetSystemEnvironmentValue", [=]
    {
        auto& core = d_->core;
        
        const auto VariableName  = arg<nt64::PUNICODE_STRING>(core, 0);
        const auto VariableValue = arg<nt64::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[339]);

        on_func(VariableName, VariableValue);
    });
}

opt<id_t> nt64::syscalls::register_NtSetSystemInformation(proc_t proc, const on_NtSetSystemInformation_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetSystemInformation", [=]
    {
        auto& core = d_->core;
        
        const auto SystemInformationClass  = arg<nt64::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto SystemInformation       = arg<nt64::PVOID>(core, 1);
        const auto SystemInformationLength = arg<nt64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[340]);

        on_func(SystemInformationClass, SystemInformation, SystemInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetSystemPowerState(proc_t proc, const on_NtSetSystemPowerState_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetSystemPowerState", [=]
    {
        auto& core = d_->core;
        
        const auto SystemAction   = arg<nt64::POWER_ACTION>(core, 0);
        const auto MinSystemState = arg<nt64::SYSTEM_POWER_STATE>(core, 1);
        const auto Flags          = arg<nt64::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[341]);

        on_func(SystemAction, MinSystemState, Flags);
    });
}

opt<id_t> nt64::syscalls::register_NtSetSystemTime(proc_t proc, const on_NtSetSystemTime_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetSystemTime", [=]
    {
        auto& core = d_->core;
        
        const auto SystemTime   = arg<nt64::PLARGE_INTEGER>(core, 0);
        const auto PreviousTime = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[342]);

        on_func(SystemTime, PreviousTime);
    });
}

opt<id_t> nt64::syscalls::register_NtSetThreadExecutionState(proc_t proc, const on_NtSetThreadExecutionState_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetThreadExecutionState", [=]
    {
        auto& core = d_->core;
        
        const auto esFlags           = arg<nt64::EXECUTION_STATE>(core, 0);
        const auto STARPreviousFlags = arg<nt64::EXECUTION_STATE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[343]);

        on_func(esFlags, STARPreviousFlags);
    });
}

opt<id_t> nt64::syscalls::register_NtSetTimerEx(proc_t proc, const on_NtSetTimerEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetTimerEx", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle               = arg<nt64::HANDLE>(core, 0);
        const auto TimerSetInformationClass  = arg<nt64::TIMER_SET_INFORMATION_CLASS>(core, 1);
        const auto TimerSetInformation       = arg<nt64::PVOID>(core, 2);
        const auto TimerSetInformationLength = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[344]);

        on_func(TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength);
    });
}

opt<id_t> nt64::syscalls::register_NtSetTimer(proc_t proc, const on_NtSetTimer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetTimer", [=]
    {
        auto& core = d_->core;
        
        const auto TimerHandle     = arg<nt64::HANDLE>(core, 0);
        const auto DueTime         = arg<nt64::PLARGE_INTEGER>(core, 1);
        const auto TimerApcRoutine = arg<nt64::PTIMER_APC_ROUTINE>(core, 2);
        const auto TimerContext    = arg<nt64::PVOID>(core, 3);
        const auto WakeTimer       = arg<nt64::BOOLEAN>(core, 4);
        const auto Period          = arg<nt64::LONG>(core, 5);
        const auto PreviousState   = arg<nt64::PBOOLEAN>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[345]);

        on_func(TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState);
    });
}

opt<id_t> nt64::syscalls::register_NtSetTimerResolution(proc_t proc, const on_NtSetTimerResolution_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetTimerResolution", [=]
    {
        auto& core = d_->core;
        
        const auto DesiredTime   = arg<nt64::ULONG>(core, 0);
        const auto SetResolution = arg<nt64::BOOLEAN>(core, 1);
        const auto ActualTime    = arg<nt64::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[346]);

        on_func(DesiredTime, SetResolution, ActualTime);
    });
}

opt<id_t> nt64::syscalls::register_NtSetUuidSeed(proc_t proc, const on_NtSetUuidSeed_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetUuidSeed", [=]
    {
        auto& core = d_->core;
        
        const auto Seed = arg<nt64::PCHAR>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[347]);

        on_func(Seed);
    });
}

opt<id_t> nt64::syscalls::register_NtSetValueKey(proc_t proc, const on_NtSetValueKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetValueKey", [=]
    {
        auto& core = d_->core;
        
        const auto KeyHandle  = arg<nt64::HANDLE>(core, 0);
        const auto ValueName  = arg<nt64::PUNICODE_STRING>(core, 1);
        const auto TitleIndex = arg<nt64::ULONG>(core, 2);
        const auto Type       = arg<nt64::ULONG>(core, 3);
        const auto Data       = arg<nt64::PVOID>(core, 4);
        const auto DataSize   = arg<nt64::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[348]);

        on_func(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
    });
}

opt<id_t> nt64::syscalls::register_NtSetVolumeInformationFile(proc_t proc, const on_NtSetVolumeInformationFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSetVolumeInformationFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle         = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock      = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto FsInformation      = arg<nt64::PVOID>(core, 2);
        const auto Length             = arg<nt64::ULONG>(core, 3);
        const auto FsInformationClass = arg<nt64::FS_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[349]);

        on_func(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    });
}

opt<id_t> nt64::syscalls::register_NtShutdownSystem(proc_t proc, const on_NtShutdownSystem_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtShutdownSystem", [=]
    {
        auto& core = d_->core;
        
        const auto Action = arg<nt64::SHUTDOWN_ACTION>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[350]);

        on_func(Action);
    });
}

opt<id_t> nt64::syscalls::register_NtShutdownWorkerFactory(proc_t proc, const on_NtShutdownWorkerFactory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtShutdownWorkerFactory", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle    = arg<nt64::HANDLE>(core, 0);
        const auto STARPendingWorkerCount = arg<nt64::LONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[351]);

        on_func(WorkerFactoryHandle, STARPendingWorkerCount);
    });
}

opt<id_t> nt64::syscalls::register_NtSignalAndWaitForSingleObject(proc_t proc, const on_NtSignalAndWaitForSingleObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSignalAndWaitForSingleObject", [=]
    {
        auto& core = d_->core;
        
        const auto SignalHandle = arg<nt64::HANDLE>(core, 0);
        const auto WaitHandle   = arg<nt64::HANDLE>(core, 1);
        const auto Alertable    = arg<nt64::BOOLEAN>(core, 2);
        const auto Timeout      = arg<nt64::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[352]);

        on_func(SignalHandle, WaitHandle, Alertable, Timeout);
    });
}

opt<id_t> nt64::syscalls::register_NtSinglePhaseReject(proc_t proc, const on_NtSinglePhaseReject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSinglePhaseReject", [=]
    {
        auto& core = d_->core;
        
        const auto EnlistmentHandle = arg<nt64::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt64::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[353]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<id_t> nt64::syscalls::register_NtStartProfile(proc_t proc, const on_NtStartProfile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtStartProfile", [=]
    {
        auto& core = d_->core;
        
        const auto ProfileHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[354]);

        on_func(ProfileHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtStopProfile(proc_t proc, const on_NtStopProfile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtStopProfile", [=]
    {
        auto& core = d_->core;
        
        const auto ProfileHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[355]);

        on_func(ProfileHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtSuspendProcess(proc_t proc, const on_NtSuspendProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSuspendProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[356]);

        on_func(ProcessHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtSuspendThread(proc_t proc, const on_NtSuspendThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSuspendThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle         = arg<nt64::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<nt64::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[357]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<id_t> nt64::syscalls::register_NtSystemDebugControl(proc_t proc, const on_NtSystemDebugControl_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSystemDebugControl", [=]
    {
        auto& core = d_->core;
        
        const auto Command            = arg<nt64::SYSDBG_COMMAND>(core, 0);
        const auto InputBuffer        = arg<nt64::PVOID>(core, 1);
        const auto InputBufferLength  = arg<nt64::ULONG>(core, 2);
        const auto OutputBuffer       = arg<nt64::PVOID>(core, 3);
        const auto OutputBufferLength = arg<nt64::ULONG>(core, 4);
        const auto ReturnLength       = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[358]);

        on_func(Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtTerminateJobObject(proc_t proc, const on_NtTerminateJobObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtTerminateJobObject", [=]
    {
        auto& core = d_->core;
        
        const auto JobHandle  = arg<nt64::HANDLE>(core, 0);
        const auto ExitStatus = arg<nt64::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[359]);

        on_func(JobHandle, ExitStatus);
    });
}

opt<id_t> nt64::syscalls::register_NtTerminateProcess(proc_t proc, const on_NtTerminateProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtTerminateProcess", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 0);
        const auto ExitStatus    = arg<nt64::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[360]);

        on_func(ProcessHandle, ExitStatus);
    });
}

opt<id_t> nt64::syscalls::register_NtTerminateThread(proc_t proc, const on_NtTerminateThread_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtTerminateThread", [=]
    {
        auto& core = d_->core;
        
        const auto ThreadHandle = arg<nt64::HANDLE>(core, 0);
        const auto ExitStatus   = arg<nt64::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[361]);

        on_func(ThreadHandle, ExitStatus);
    });
}

opt<id_t> nt64::syscalls::register_NtTraceControl(proc_t proc, const on_NtTraceControl_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtTraceControl", [=]
    {
        auto& core = d_->core;
        
        const auto FunctionCode = arg<nt64::ULONG>(core, 0);
        const auto InBuffer     = arg<nt64::PVOID>(core, 1);
        const auto InBufferLen  = arg<nt64::ULONG>(core, 2);
        const auto OutBuffer    = arg<nt64::PVOID>(core, 3);
        const auto OutBufferLen = arg<nt64::ULONG>(core, 4);
        const auto ReturnLength = arg<nt64::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[362]);

        on_func(FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength);
    });
}

opt<id_t> nt64::syscalls::register_NtTraceEvent(proc_t proc, const on_NtTraceEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtTraceEvent", [=]
    {
        auto& core = d_->core;
        
        const auto TraceHandle = arg<nt64::HANDLE>(core, 0);
        const auto Flags       = arg<nt64::ULONG>(core, 1);
        const auto FieldSize   = arg<nt64::ULONG>(core, 2);
        const auto Fields      = arg<nt64::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[363]);

        on_func(TraceHandle, Flags, FieldSize, Fields);
    });
}

opt<id_t> nt64::syscalls::register_NtTranslateFilePath(proc_t proc, const on_NtTranslateFilePath_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtTranslateFilePath", [=]
    {
        auto& core = d_->core;
        
        const auto InputFilePath        = arg<nt64::PFILE_PATH>(core, 0);
        const auto OutputType           = arg<nt64::ULONG>(core, 1);
        const auto OutputFilePath       = arg<nt64::PFILE_PATH>(core, 2);
        const auto OutputFilePathLength = arg<nt64::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[364]);

        on_func(InputFilePath, OutputType, OutputFilePath, OutputFilePathLength);
    });
}

opt<id_t> nt64::syscalls::register_NtUnloadDriver(proc_t proc, const on_NtUnloadDriver_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtUnloadDriver", [=]
    {
        auto& core = d_->core;
        
        const auto DriverServiceName = arg<nt64::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[365]);

        on_func(DriverServiceName);
    });
}

opt<id_t> nt64::syscalls::register_NtUnloadKey2(proc_t proc, const on_NtUnloadKey2_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtUnloadKey2", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);
        const auto Flags     = arg<nt64::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[366]);

        on_func(TargetKey, Flags);
    });
}

opt<id_t> nt64::syscalls::register_NtUnloadKeyEx(proc_t proc, const on_NtUnloadKeyEx_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtUnloadKeyEx", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);
        const auto Event     = arg<nt64::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[367]);

        on_func(TargetKey, Event);
    });
}

opt<id_t> nt64::syscalls::register_NtUnloadKey(proc_t proc, const on_NtUnloadKey_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtUnloadKey", [=]
    {
        auto& core = d_->core;
        
        const auto TargetKey = arg<nt64::POBJECT_ATTRIBUTES>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[368]);

        on_func(TargetKey);
    });
}

opt<id_t> nt64::syscalls::register_NtUnlockFile(proc_t proc, const on_NtUnlockFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtUnlockFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt64::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt64::PIO_STATUS_BLOCK>(core, 1);
        const auto ByteOffset    = arg<nt64::PLARGE_INTEGER>(core, 2);
        const auto Length        = arg<nt64::PLARGE_INTEGER>(core, 3);
        const auto Key           = arg<nt64::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[369]);

        on_func(FileHandle, IoStatusBlock, ByteOffset, Length, Key);
    });
}

opt<id_t> nt64::syscalls::register_NtUnlockVirtualMemory(proc_t proc, const on_NtUnlockVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtUnlockVirtualMemory", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle   = arg<nt64::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt64::PVOID>(core, 1);
        const auto RegionSize      = arg<nt64::PSIZE_T>(core, 2);
        const auto MapType         = arg<nt64::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[370]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    });
}

opt<id_t> nt64::syscalls::register_NtUnmapViewOfSection(proc_t proc, const on_NtUnmapViewOfSection_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtUnmapViewOfSection", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle = arg<nt64::HANDLE>(core, 0);
        const auto BaseAddress   = arg<nt64::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[371]);

        on_func(ProcessHandle, BaseAddress);
    });
}

opt<id_t> nt64::syscalls::register_NtVdmControl(proc_t proc, const on_NtVdmControl_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtVdmControl", [=]
    {
        auto& core = d_->core;
        
        const auto Service     = arg<nt64::VDMSERVICECLASS>(core, 0);
        const auto ServiceData = arg<nt64::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[372]);

        on_func(Service, ServiceData);
    });
}

opt<id_t> nt64::syscalls::register_NtWaitForDebugEvent(proc_t proc, const on_NtWaitForDebugEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWaitForDebugEvent", [=]
    {
        auto& core = d_->core;
        
        const auto DebugObjectHandle = arg<nt64::HANDLE>(core, 0);
        const auto Alertable         = arg<nt64::BOOLEAN>(core, 1);
        const auto Timeout           = arg<nt64::PLARGE_INTEGER>(core, 2);
        const auto WaitStateChange   = arg<nt64::PDBGUI_WAIT_STATE_CHANGE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[373]);

        on_func(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
    });
}

opt<id_t> nt64::syscalls::register_NtWaitForKeyedEvent(proc_t proc, const on_NtWaitForKeyedEvent_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWaitForKeyedEvent", [=]
    {
        auto& core = d_->core;
        
        const auto KeyedEventHandle = arg<nt64::HANDLE>(core, 0);
        const auto KeyValue         = arg<nt64::PVOID>(core, 1);
        const auto Alertable        = arg<nt64::BOOLEAN>(core, 2);
        const auto Timeout          = arg<nt64::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[374]);

        on_func(KeyedEventHandle, KeyValue, Alertable, Timeout);
    });
}

opt<id_t> nt64::syscalls::register_NtWaitForMultipleObjects32(proc_t proc, const on_NtWaitForMultipleObjects32_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWaitForMultipleObjects32", [=]
    {
        auto& core = d_->core;
        
        const auto Count     = arg<nt64::ULONG>(core, 0);
        const auto Handles   = arg<nt64::LONG>(core, 1);
        const auto WaitType  = arg<nt64::WAIT_TYPE>(core, 2);
        const auto Alertable = arg<nt64::BOOLEAN>(core, 3);
        const auto Timeout   = arg<nt64::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[375]);

        on_func(Count, Handles, WaitType, Alertable, Timeout);
    });
}

opt<id_t> nt64::syscalls::register_NtWaitForMultipleObjects(proc_t proc, const on_NtWaitForMultipleObjects_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWaitForMultipleObjects", [=]
    {
        auto& core = d_->core;
        
        const auto Count     = arg<nt64::ULONG>(core, 0);
        const auto Handles   = arg<nt64::HANDLE>(core, 1);
        const auto WaitType  = arg<nt64::WAIT_TYPE>(core, 2);
        const auto Alertable = arg<nt64::BOOLEAN>(core, 3);
        const auto Timeout   = arg<nt64::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[376]);

        on_func(Count, Handles, WaitType, Alertable, Timeout);
    });
}

opt<id_t> nt64::syscalls::register_NtWaitForSingleObject(proc_t proc, const on_NtWaitForSingleObject_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWaitForSingleObject", [=]
    {
        auto& core = d_->core;
        
        const auto Handle    = arg<nt64::HANDLE>(core, 0);
        const auto Alertable = arg<nt64::BOOLEAN>(core, 1);
        const auto Timeout   = arg<nt64::PLARGE_INTEGER>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[377]);

        on_func(Handle, Alertable, Timeout);
    });
}

opt<id_t> nt64::syscalls::register_NtWaitForWorkViaWorkerFactory(proc_t proc, const on_NtWaitForWorkViaWorkerFactory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWaitForWorkViaWorkerFactory", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle = arg<nt64::HANDLE>(core, 0);
        const auto MiniPacket          = arg<nt64::PFILE_IO_COMPLETION_INFORMATION>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[378]);

        on_func(WorkerFactoryHandle, MiniPacket);
    });
}

opt<id_t> nt64::syscalls::register_NtWaitHighEventPair(proc_t proc, const on_NtWaitHighEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWaitHighEventPair", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[379]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtWaitLowEventPair(proc_t proc, const on_NtWaitLowEventPair_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWaitLowEventPair", [=]
    {
        auto& core = d_->core;
        
        const auto EventPairHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[380]);

        on_func(EventPairHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtWorkerFactoryWorkerReady(proc_t proc, const on_NtWorkerFactoryWorkerReady_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWorkerFactoryWorkerReady", [=]
    {
        auto& core = d_->core;
        
        const auto WorkerFactoryHandle = arg<nt64::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[381]);

        on_func(WorkerFactoryHandle);
    });
}

opt<id_t> nt64::syscalls::register_NtWriteFileGather(proc_t proc, const on_NtWriteFileGather_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWriteFileGather", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt64::HANDLE>(core, 0);
        const auto Event         = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt64::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt64::PIO_STATUS_BLOCK>(core, 4);
        const auto SegmentArray  = arg<nt64::PFILE_SEGMENT_ELEMENT>(core, 5);
        const auto Length        = arg<nt64::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt64::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt64::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[382]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    });
}

opt<id_t> nt64::syscalls::register_NtWriteFile(proc_t proc, const on_NtWriteFile_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWriteFile", [=]
    {
        auto& core = d_->core;
        
        const auto FileHandle    = arg<nt64::HANDLE>(core, 0);
        const auto Event         = arg<nt64::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt64::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt64::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt64::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer        = arg<nt64::PVOID>(core, 5);
        const auto Length        = arg<nt64::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt64::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt64::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[383]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    });
}

opt<id_t> nt64::syscalls::register_NtWriteRequestData(proc_t proc, const on_NtWriteRequestData_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWriteRequestData", [=]
    {
        auto& core = d_->core;
        
        const auto PortHandle           = arg<nt64::HANDLE>(core, 0);
        const auto Message              = arg<nt64::PPORT_MESSAGE>(core, 1);
        const auto DataEntryIndex       = arg<nt64::ULONG>(core, 2);
        const auto Buffer               = arg<nt64::PVOID>(core, 3);
        const auto BufferSize           = arg<nt64::SIZE_T>(core, 4);
        const auto NumberOfBytesWritten = arg<nt64::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[384]);

        on_func(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten);
    });
}

opt<id_t> nt64::syscalls::register_NtWriteVirtualMemory(proc_t proc, const on_NtWriteVirtualMemory_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtWriteVirtualMemory", [=]
    {
        auto& core = d_->core;
        
        const auto ProcessHandle        = arg<nt64::HANDLE>(core, 0);
        const auto BaseAddress          = arg<nt64::PVOID>(core, 1);
        const auto Buffer               = arg<nt64::PVOID>(core, 2);
        const auto BufferSize           = arg<nt64::SIZE_T>(core, 3);
        const auto NumberOfBytesWritten = arg<nt64::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[385]);

        on_func(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);
    });
}

opt<id_t> nt64::syscalls::register_NtDisableLastKnownGood(proc_t proc, const on_NtDisableLastKnownGood_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtDisableLastKnownGood", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[386]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtEnableLastKnownGood(proc_t proc, const on_NtEnableLastKnownGood_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtEnableLastKnownGood", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[387]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtFlushProcessWriteBuffers(proc_t proc, const on_NtFlushProcessWriteBuffers_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFlushProcessWriteBuffers", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[388]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtFlushWriteBuffer(proc_t proc, const on_NtFlushWriteBuffer_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtFlushWriteBuffer", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[389]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtGetCurrentProcessorNumber(proc_t proc, const on_NtGetCurrentProcessorNumber_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtGetCurrentProcessorNumber", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[390]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtIsSystemResumeAutomatic(proc_t proc, const on_NtIsSystemResumeAutomatic_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtIsSystemResumeAutomatic", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[391]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtIsUILanguageComitted(proc_t proc, const on_NtIsUILanguageComitted_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtIsUILanguageComitted", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[392]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtQueryPortInformationProcess(proc_t proc, const on_NtQueryPortInformationProcess_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtQueryPortInformationProcess", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[393]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtSerializeBoot(proc_t proc, const on_NtSerializeBoot_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtSerializeBoot", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[394]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtTestAlert(proc_t proc, const on_NtTestAlert_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtTestAlert", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[395]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtThawRegistry(proc_t proc, const on_NtThawRegistry_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtThawRegistry", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[396]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtThawTransactions(proc_t proc, const on_NtThawTransactions_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtThawTransactions", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[397]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtUmsThreadYield(proc_t proc, const on_NtUmsThreadYield_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtUmsThreadYield", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[398]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_NtYieldExecution(proc_t proc, const on_NtYieldExecution_fn& on_func)
{
    return register_callback(*d_, ++d_->last_id, proc, "NtYieldExecution", [=]
    {
        auto& core = d_->core;
        
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[399]);

        on_func();
    });
}

opt<id_t> nt64::syscalls::register_all(proc_t proc, const nt64::syscalls::on_call_fn& on_call)
{
    const auto id   = ++d_->last_id;
    const auto size = d_->listeners.size();
    for(const auto cfg : g_callcfgs)
        register_callback(*d_, id, proc, cfg.name, [=]{ on_call(cfg); });

    if(size == d_->listeners.size())
        return {};

    return id;
}

bool nt64::syscalls::unregister(id_t id)
{
    return d_->listeners.erase(id) > 0;
}
