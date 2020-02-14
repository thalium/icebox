#include "syscalls.gen.hpp"

#define FDP_MODULE "syscalls"
#include "log.hpp"
#include "core.hpp"

#include <cstring>

namespace
{
    constexpr bool g_debug = false;

    constexpr nt::syscalls::callcfgs_t g_callcfgs =
    {{
        {"NtAcceptConnectPort", 6, {{"PHANDLE", "PortHandle", sizeof(nt::PHANDLE)}, {"PVOID", "PortContext", sizeof(nt::PVOID)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(nt::PPORT_MESSAGE)}, {"BOOLEAN", "AcceptConnection", sizeof(nt::BOOLEAN)}, {"PPORT_VIEW", "ServerView", sizeof(nt::PPORT_VIEW)}, {"PREMOTE_PORT_VIEW", "ClientView", sizeof(nt::PREMOTE_PORT_VIEW)}}},
        {"NtAddDriverEntry", 2, {{"PEFI_DRIVER_ENTRY", "DriverEntry", sizeof(nt::PEFI_DRIVER_ENTRY)}, {"PULONG", "Id", sizeof(nt::PULONG)}}},
        {"NtAdjustGroupsToken", 6, {{"HANDLE", "TokenHandle", sizeof(nt::HANDLE)}, {"BOOLEAN", "ResetToDefault", sizeof(nt::BOOLEAN)}, {"PTOKEN_GROUPS", "NewState", sizeof(nt::PTOKEN_GROUPS)}, {"ULONG", "BufferLength", sizeof(nt::ULONG)}, {"PTOKEN_GROUPS", "PreviousState", sizeof(nt::PTOKEN_GROUPS)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtAdjustPrivilegesToken", 6, {{"HANDLE", "TokenHandle", sizeof(nt::HANDLE)}, {"BOOLEAN", "DisableAllPrivileges", sizeof(nt::BOOLEAN)}, {"PTOKEN_PRIVILEGES", "NewState", sizeof(nt::PTOKEN_PRIVILEGES)}, {"ULONG", "BufferLength", sizeof(nt::ULONG)}, {"PTOKEN_PRIVILEGES", "PreviousState", sizeof(nt::PTOKEN_PRIVILEGES)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtAlertThread", 1, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}}},
        {"NtAllocateReserveObject", 3, {{"PHANDLE", "MemoryReserveHandle", sizeof(nt::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"MEMORY_RESERVE_TYPE", "Type", sizeof(nt::MEMORY_RESERVE_TYPE)}}},
        {"NtAlpcCancelMessage", 3, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PALPC_CONTEXT_ATTR", "MessageContext", sizeof(nt::PALPC_CONTEXT_ATTR)}}},
        {"NtAlpcConnectPort", 11, {{"PHANDLE", "PortHandle", sizeof(nt::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(nt::PUNICODE_STRING)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(nt::PALPC_PORT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PSID", "RequiredServerSid", sizeof(nt::PSID)}, {"PPORT_MESSAGE", "ConnectionMessage", sizeof(nt::PPORT_MESSAGE)}, {"PULONG", "BufferLength", sizeof(nt::PULONG)}, {"PALPC_MESSAGE_ATTRIBUTES", "OutMessageAttributes", sizeof(nt::PALPC_MESSAGE_ATTRIBUTES)}, {"PALPC_MESSAGE_ATTRIBUTES", "InMessageAttributes", sizeof(nt::PALPC_MESSAGE_ATTRIBUTES)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtAlpcCreatePort", 3, {{"PHANDLE", "PortHandle", sizeof(nt::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(nt::PALPC_PORT_ATTRIBUTES)}}},
        {"NtAlpcCreateSectionView", 3, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PALPC_DATA_VIEW_ATTR", "ViewAttributes", sizeof(nt::PALPC_DATA_VIEW_ATTR)}}},
        {"NtAlpcCreateSecurityContext", 3, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PALPC_SECURITY_ATTR", "SecurityAttribute", sizeof(nt::PALPC_SECURITY_ATTR)}}},
        {"NtAlpcDeletePortSection", 3, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"ALPC_HANDLE", "SectionHandle", sizeof(nt::ALPC_HANDLE)}}},
        {"NtAlpcDeleteResourceReserve", 3, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"ALPC_HANDLE", "ResourceId", sizeof(nt::ALPC_HANDLE)}}},
        {"NtAlpcDisconnectPort", 2, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}}},
        {"NtAlpcImpersonateClientOfPort", 3, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt::PPORT_MESSAGE)}, {"PVOID", "Reserved", sizeof(nt::PVOID)}}},
        {"NtAlpcOpenSenderProcess", 6, {{"PHANDLE", "ProcessHandle", sizeof(nt::PHANDLE)}, {"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt::PPORT_MESSAGE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtAlpcOpenSenderThread", 6, {{"PHANDLE", "ThreadHandle", sizeof(nt::PHANDLE)}, {"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt::PPORT_MESSAGE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtAlpcRevokeSecurityContext", 3, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"ALPC_HANDLE", "ContextHandle", sizeof(nt::ALPC_HANDLE)}}},
        {"NtAlpcSetInformation", 4, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ALPC_PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(nt::ALPC_PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}}},
        {"NtApphelpCacheControl", 2, {{"APPHELPCOMMAND", "type", sizeof(nt::APPHELPCOMMAND)}, {"PVOID", "buf", sizeof(nt::PVOID)}}},
        {"NtAssignProcessToJobObject", 2, {{"HANDLE", "JobHandle", sizeof(nt::HANDLE)}, {"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}}},
        {"NtCancelTimer", 2, {{"HANDLE", "TimerHandle", sizeof(nt::HANDLE)}, {"PBOOLEAN", "CurrentState", sizeof(nt::PBOOLEAN)}}},
        {"NtClearEvent", 1, {{"HANDLE", "EventHandle", sizeof(nt::HANDLE)}}},
        {"NtClose", 1, {{"HANDLE", "Handle", sizeof(nt::HANDLE)}}},
        {"NtCommitComplete", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtCompactKeys", 2, {{"ULONG", "Count", sizeof(nt::ULONG)}, {"HANDLE", "KeyArray", sizeof(nt::HANDLE)}}},
        {"NtCompleteConnectPort", 1, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}}},
        {"NtCompressKey", 1, {{"HANDLE", "Key", sizeof(nt::HANDLE)}}},
        {"NtCreateDebugObject", 4, {{"PHANDLE", "DebugObjectHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt::ULONG)}}},
        {"NtCreateDirectoryObject", 3, {{"PHANDLE", "DirectoryHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtCreateEnlistment", 8, {{"PHANDLE", "EnlistmentHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"HANDLE", "ResourceManagerHandle", sizeof(nt::HANDLE)}, {"HANDLE", "TransactionHandle", sizeof(nt::HANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "CreateOptions", sizeof(nt::ULONG)}, {"NOTIFICATION_MASK", "NotificationMask", sizeof(nt::NOTIFICATION_MASK)}, {"PVOID", "EnlistmentKey", sizeof(nt::PVOID)}}},
        {"NtCreateIoCompletion", 4, {{"PHANDLE", "IoCompletionHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "Count", sizeof(nt::ULONG)}}},
        {"NtCreateMutant", 4, {{"PHANDLE", "MutantHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"BOOLEAN", "InitialOwner", sizeof(nt::BOOLEAN)}}},
        {"NtCreateNamedPipeFile", 14, {{"PHANDLE", "FileHandle", sizeof(nt::PHANDLE)}, {"ULONG", "DesiredAccess", sizeof(nt::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"ULONG", "ShareAccess", sizeof(nt::ULONG)}, {"ULONG", "CreateDisposition", sizeof(nt::ULONG)}, {"ULONG", "CreateOptions", sizeof(nt::ULONG)}, {"ULONG", "NamedPipeType", sizeof(nt::ULONG)}, {"ULONG", "ReadMode", sizeof(nt::ULONG)}, {"ULONG", "CompletionMode", sizeof(nt::ULONG)}, {"ULONG", "MaximumInstances", sizeof(nt::ULONG)}, {"ULONG", "InboundQuota", sizeof(nt::ULONG)}, {"ULONG", "OutboundQuota", sizeof(nt::ULONG)}, {"PLARGE_INTEGER", "DefaultTimeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtCreatePagingFile", 4, {{"PUNICODE_STRING", "PageFileName", sizeof(nt::PUNICODE_STRING)}, {"PLARGE_INTEGER", "MinimumSize", sizeof(nt::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "MaximumSize", sizeof(nt::PLARGE_INTEGER)}, {"ULONG", "Priority", sizeof(nt::ULONG)}}},
        {"NtCreateProcess", 8, {{"PHANDLE", "ProcessHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"HANDLE", "ParentProcess", sizeof(nt::HANDLE)}, {"BOOLEAN", "InheritObjectTable", sizeof(nt::BOOLEAN)}, {"HANDLE", "SectionHandle", sizeof(nt::HANDLE)}, {"HANDLE", "DebugPort", sizeof(nt::HANDLE)}, {"HANDLE", "ExceptionPort", sizeof(nt::HANDLE)}}},
        {"NtCreateResourceManager", 7, {{"PHANDLE", "ResourceManagerHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"HANDLE", "TmHandle", sizeof(nt::HANDLE)}, {"LPGUID", "RmGuid", sizeof(nt::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "CreateOptions", sizeof(nt::ULONG)}, {"PUNICODE_STRING", "Description", sizeof(nt::PUNICODE_STRING)}}},
        {"NtCreateSection", 7, {{"PHANDLE", "SectionHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PLARGE_INTEGER", "MaximumSize", sizeof(nt::PLARGE_INTEGER)}, {"ULONG", "SectionPageProtection", sizeof(nt::ULONG)}, {"ULONG", "AllocationAttributes", sizeof(nt::ULONG)}, {"HANDLE", "FileHandle", sizeof(nt::HANDLE)}}},
        {"NtCreateSemaphore", 5, {{"PHANDLE", "SemaphoreHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"LONG", "InitialCount", sizeof(nt::LONG)}, {"LONG", "MaximumCount", sizeof(nt::LONG)}}},
        {"NtCreateThread", 8, {{"PHANDLE", "ThreadHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PCLIENT_ID", "ClientId", sizeof(nt::PCLIENT_ID)}, {"PCONTEXT", "ThreadContext", sizeof(nt::PCONTEXT)}, {"PINITIAL_TEB", "InitialTeb", sizeof(nt::PINITIAL_TEB)}, {"BOOLEAN", "CreateSuspended", sizeof(nt::BOOLEAN)}}},
        {"NtCreateThreadEx", 11, {{"PHANDLE", "ThreadHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "StartRoutine", sizeof(nt::PVOID)}, {"PVOID", "Argument", sizeof(nt::PVOID)}, {"ULONG", "CreateFlags", sizeof(nt::ULONG)}, {"ULONG_PTR", "ZeroBits", sizeof(nt::ULONG_PTR)}, {"SIZE_T", "StackSize", sizeof(nt::SIZE_T)}, {"SIZE_T", "MaximumStackSize", sizeof(nt::SIZE_T)}, {"PPS_ATTRIBUTE_LIST", "AttributeList", sizeof(nt::PPS_ATTRIBUTE_LIST)}}},
        {"NtCreateToken", 13, {{"PHANDLE", "TokenHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"TOKEN_TYPE", "TokenType", sizeof(nt::TOKEN_TYPE)}, {"PLUID", "AuthenticationId", sizeof(nt::PLUID)}, {"PLARGE_INTEGER", "ExpirationTime", sizeof(nt::PLARGE_INTEGER)}, {"PTOKEN_USER", "User", sizeof(nt::PTOKEN_USER)}, {"PTOKEN_GROUPS", "Groups", sizeof(nt::PTOKEN_GROUPS)}, {"PTOKEN_PRIVILEGES", "Privileges", sizeof(nt::PTOKEN_PRIVILEGES)}, {"PTOKEN_OWNER", "Owner", sizeof(nt::PTOKEN_OWNER)}, {"PTOKEN_PRIMARY_GROUP", "PrimaryGroup", sizeof(nt::PTOKEN_PRIMARY_GROUP)}, {"PTOKEN_DEFAULT_DACL", "DefaultDacl", sizeof(nt::PTOKEN_DEFAULT_DACL)}, {"PTOKEN_SOURCE", "TokenSource", sizeof(nt::PTOKEN_SOURCE)}}},
        {"NtCreateTransaction", 10, {{"PHANDLE", "TransactionHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"LPGUID", "Uow", sizeof(nt::LPGUID)}, {"HANDLE", "TmHandle", sizeof(nt::HANDLE)}, {"ULONG", "CreateOptions", sizeof(nt::ULONG)}, {"ULONG", "IsolationLevel", sizeof(nt::ULONG)}, {"ULONG", "IsolationFlags", sizeof(nt::ULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}, {"PUNICODE_STRING", "Description", sizeof(nt::PUNICODE_STRING)}}},
        {"NtCreateUserProcess", 11, {{"PHANDLE", "ProcessHandle", sizeof(nt::PHANDLE)}, {"PHANDLE", "ThreadHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "ProcessDesiredAccess", sizeof(nt::ACCESS_MASK)}, {"ACCESS_MASK", "ThreadDesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ProcessObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "ThreadObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "ProcessFlags", sizeof(nt::ULONG)}, {"ULONG", "ThreadFlags", sizeof(nt::ULONG)}, {"PRTL_USER_PROCESS_PARAMETERS", "ProcessParameters", sizeof(nt::PRTL_USER_PROCESS_PARAMETERS)}, {"PPROCESS_CREATE_INFO", "CreateInfo", sizeof(nt::PPROCESS_CREATE_INFO)}, {"PPROCESS_ATTRIBUTE_LIST", "AttributeList", sizeof(nt::PPROCESS_ATTRIBUTE_LIST)}}},
        {"NtCreateWaitablePort", 5, {{"PHANDLE", "PortHandle", sizeof(nt::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "MaxConnectionInfoLength", sizeof(nt::ULONG)}, {"ULONG", "MaxMessageLength", sizeof(nt::ULONG)}, {"ULONG", "MaxPoolUsage", sizeof(nt::ULONG)}}},
        {"NtCreateWorkerFactory", 10, {{"PHANDLE", "WorkerFactoryHandleReturn", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"HANDLE", "CompletionPortHandle", sizeof(nt::HANDLE)}, {"HANDLE", "WorkerProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "StartRoutine", sizeof(nt::PVOID)}, {"PVOID", "StartParameter", sizeof(nt::PVOID)}, {"ULONG", "MaxThreadCount", sizeof(nt::ULONG)}, {"SIZE_T", "StackReserve", sizeof(nt::SIZE_T)}, {"SIZE_T", "StackCommit", sizeof(nt::SIZE_T)}}},
        {"NtDebugActiveProcess", 2, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"HANDLE", "DebugObjectHandle", sizeof(nt::HANDLE)}}},
        {"NtDebugContinue", 3, {{"HANDLE", "DebugObjectHandle", sizeof(nt::HANDLE)}, {"PCLIENT_ID", "ClientId", sizeof(nt::PCLIENT_ID)}, {"NTSTATUS", "ContinueStatus", sizeof(nt::NTSTATUS)}}},
        {"NtDeleteAtom", 1, {{"RTL_ATOM", "Atom", sizeof(nt::RTL_ATOM)}}},
        {"NtDeleteBootEntry", 1, {{"ULONG", "Id", sizeof(nt::ULONG)}}},
        {"NtDeleteFile", 1, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtDeleteKey", 1, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}}},
        {"NtDeletePrivateNamespace", 1, {{"HANDLE", "NamespaceHandle", sizeof(nt::HANDLE)}}},
        {"NtDeviceIoControlFile", 10, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"ULONG", "IoControlCode", sizeof(nt::ULONG)}, {"PVOID", "InputBuffer", sizeof(nt::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt::ULONG)}}},
        {"NtDisableLastKnownGood", 0, {}},
        {"NtDrawText", 1, {{"PUNICODE_STRING", "Text", sizeof(nt::PUNICODE_STRING)}}},
        {"NtDuplicateToken", 6, {{"HANDLE", "ExistingTokenHandle", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"BOOLEAN", "EffectiveOnly", sizeof(nt::BOOLEAN)}, {"TOKEN_TYPE", "TokenType", sizeof(nt::TOKEN_TYPE)}, {"PHANDLE", "NewTokenHandle", sizeof(nt::PHANDLE)}}},
        {"NtEnableLastKnownGood", 0, {}},
        {"NtEnumerateBootEntries", 2, {{"PVOID", "Buffer", sizeof(nt::PVOID)}, {"PULONG", "BufferLength", sizeof(nt::PULONG)}}},
        {"NtEnumerateKey", 6, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"ULONG", "Index", sizeof(nt::ULONG)}, {"KEY_INFORMATION_CLASS", "KeyInformationClass", sizeof(nt::KEY_INFORMATION_CLASS)}, {"PVOID", "KeyInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PULONG", "ResultLength", sizeof(nt::PULONG)}}},
        {"NtEnumerateSystemEnvironmentValuesEx", 3, {{"ULONG", "InformationClass", sizeof(nt::ULONG)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"PULONG", "BufferLength", sizeof(nt::PULONG)}}},
        {"NtEnumerateTransactionObject", 5, {{"HANDLE", "RootObjectHandle", sizeof(nt::HANDLE)}, {"KTMOBJECT_TYPE", "QueryType", sizeof(nt::KTMOBJECT_TYPE)}, {"PKTMOBJECT_CURSOR", "ObjectCursor", sizeof(nt::PKTMOBJECT_CURSOR)}, {"ULONG", "ObjectCursorLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtExtendSection", 2, {{"HANDLE", "SectionHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "NewSectionSize", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtFlushInstallUILanguage", 2, {{"LANGID", "InstallUILanguage", sizeof(nt::LANGID)}, {"ULONG", "SetComittedFlag", sizeof(nt::ULONG)}}},
        {"NtFlushInstructionCache", 3, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}, {"SIZE_T", "Length", sizeof(nt::SIZE_T)}}},
        {"NtFlushWriteBuffer", 0, {}},
        {"NtFreezeRegistry", 1, {{"ULONG", "TimeOutInSeconds", sizeof(nt::ULONG)}}},
        {"NtGetNextProcess", 5, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt::ULONG)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PHANDLE", "NewProcessHandle", sizeof(nt::PHANDLE)}}},
        {"NtGetNotificationResourceManager", 7, {{"HANDLE", "ResourceManagerHandle", sizeof(nt::HANDLE)}, {"PTRANSACTION_NOTIFICATION", "TransactionNotification", sizeof(nt::PTRANSACTION_NOTIFICATION)}, {"ULONG", "NotificationLength", sizeof(nt::ULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}, {"ULONG", "Asynchronous", sizeof(nt::ULONG)}, {"ULONG_PTR", "AsynchronousContext", sizeof(nt::ULONG_PTR)}}},
        {"NtInitializeNlsFiles", 3, {{"PVOID", "STARBaseAddress", sizeof(nt::PVOID)}, {"PLCID", "DefaultLocaleId", sizeof(nt::PLCID)}, {"PLARGE_INTEGER", "DefaultCasingTableSize", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtInitializeRegistry", 1, {{"USHORT", "BootCondition", sizeof(nt::USHORT)}}},
        {"NtInitiatePowerAction", 4, {{"POWER_ACTION", "SystemAction", sizeof(nt::POWER_ACTION)}, {"SYSTEM_POWER_STATE", "MinSystemState", sizeof(nt::SYSTEM_POWER_STATE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(nt::BOOLEAN)}}},
        {"NtIsProcessInJob", 2, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"HANDLE", "JobHandle", sizeof(nt::HANDLE)}}},
        {"NtIsUILanguageComitted", 0, {}},
        {"NtListenPort", 2, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(nt::PPORT_MESSAGE)}}},
        {"NtLoadKey", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtLoadKeyEx", 4, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"HANDLE", "TrustClassKey", sizeof(nt::HANDLE)}}},
        {"NtLockProductActivationKeys", 2, {{"ULONG", "STARpPrivateVer", sizeof(nt::ULONG)}, {"ULONG", "STARpSafeMode", sizeof(nt::ULONG)}}},
        {"NtLockVirtualMemory", 4, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt::PSIZE_T)}, {"ULONG", "MapType", sizeof(nt::ULONG)}}},
        {"NtMakePermanentObject", 1, {{"HANDLE", "Handle", sizeof(nt::HANDLE)}}},
        {"NtMakeTemporaryObject", 1, {{"HANDLE", "Handle", sizeof(nt::HANDLE)}}},
        {"NtMapCMFModule", 6, {{"ULONG", "What", sizeof(nt::ULONG)}, {"ULONG", "Index", sizeof(nt::ULONG)}, {"PULONG", "CacheIndexOut", sizeof(nt::PULONG)}, {"PULONG", "CacheFlagsOut", sizeof(nt::PULONG)}, {"PULONG", "ViewSizeOut", sizeof(nt::PULONG)}, {"PVOID", "STARBaseAddress", sizeof(nt::PVOID)}}},
        {"NtMapUserPhysicalPages", 3, {{"PVOID", "VirtualAddress", sizeof(nt::PVOID)}, {"ULONG_PTR", "NumberOfPages", sizeof(nt::ULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(nt::PULONG_PTR)}}},
        {"NtModifyBootEntry", 1, {{"PBOOT_ENTRY", "BootEntry", sizeof(nt::PBOOT_ENTRY)}}},
        {"NtNotifyChangeDirectoryFile", 9, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"ULONG", "CompletionFilter", sizeof(nt::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(nt::BOOLEAN)}}},
        {"NtNotifyChangeMultipleKeys", 12, {{"HANDLE", "MasterKeyHandle", sizeof(nt::HANDLE)}, {"ULONG", "Count", sizeof(nt::ULONG)}, {"POBJECT_ATTRIBUTES", "SlaveObjects", sizeof(nt::POBJECT_ATTRIBUTES)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"ULONG", "CompletionFilter", sizeof(nt::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(nt::BOOLEAN)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "BufferSize", sizeof(nt::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(nt::BOOLEAN)}}},
        {"NtNotifyChangeSession", 8, {{"HANDLE", "Session", sizeof(nt::HANDLE)}, {"ULONG", "IoStateSequence", sizeof(nt::ULONG)}, {"PVOID", "Reserved", sizeof(nt::PVOID)}, {"ULONG", "Action", sizeof(nt::ULONG)}, {"IO_SESSION_STATE", "IoState", sizeof(nt::IO_SESSION_STATE)}, {"IO_SESSION_STATE", "IoState2", sizeof(nt::IO_SESSION_STATE)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "BufferSize", sizeof(nt::ULONG)}}},
        {"NtOpenEnlistment", 5, {{"PHANDLE", "EnlistmentHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"HANDLE", "ResourceManagerHandle", sizeof(nt::HANDLE)}, {"LPGUID", "EnlistmentGuid", sizeof(nt::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtOpenEvent", 3, {{"PHANDLE", "EventHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtOpenFile", 6, {{"PHANDLE", "FileHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"ULONG", "ShareAccess", sizeof(nt::ULONG)}, {"ULONG", "OpenOptions", sizeof(nt::ULONG)}}},
        {"NtOpenIoCompletion", 3, {{"PHANDLE", "IoCompletionHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtOpenJobObject", 3, {{"PHANDLE", "JobHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtOpenKey", 3, {{"PHANDLE", "KeyHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtOpenKeyEx", 4, {{"PHANDLE", "KeyHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "OpenOptions", sizeof(nt::ULONG)}}},
        {"NtOpenKeyedEvent", 3, {{"PHANDLE", "KeyedEventHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtOpenMutant", 3, {{"PHANDLE", "MutantHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtOpenPrivateNamespace", 4, {{"PHANDLE", "NamespaceHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PVOID", "BoundaryDescriptor", sizeof(nt::PVOID)}}},
        {"NtOpenProcess", 4, {{"PHANDLE", "ProcessHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PCLIENT_ID", "ClientId", sizeof(nt::PCLIENT_ID)}}},
        {"NtOpenProcessToken", 3, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"PHANDLE", "TokenHandle", sizeof(nt::PHANDLE)}}},
        {"NtOpenProcessTokenEx", 4, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt::ULONG)}, {"PHANDLE", "TokenHandle", sizeof(nt::PHANDLE)}}},
        {"NtOpenResourceManager", 5, {{"PHANDLE", "ResourceManagerHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"HANDLE", "TmHandle", sizeof(nt::HANDLE)}, {"LPGUID", "ResourceManagerGuid", sizeof(nt::LPGUID)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtOpenSection", 3, {{"PHANDLE", "SectionHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtOpenThreadToken", 4, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"BOOLEAN", "OpenAsSelf", sizeof(nt::BOOLEAN)}, {"PHANDLE", "TokenHandle", sizeof(nt::PHANDLE)}}},
        {"NtOpenThreadTokenEx", 5, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"BOOLEAN", "OpenAsSelf", sizeof(nt::BOOLEAN)}, {"ULONG", "HandleAttributes", sizeof(nt::ULONG)}, {"PHANDLE", "TokenHandle", sizeof(nt::PHANDLE)}}},
        {"NtOpenTransactionManager", 6, {{"PHANDLE", "TmHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LogFileName", sizeof(nt::PUNICODE_STRING)}, {"LPGUID", "TmIdentity", sizeof(nt::LPGUID)}, {"ULONG", "OpenOptions", sizeof(nt::ULONG)}}},
        {"NtPowerInformation", 5, {{"POWER_INFORMATION_LEVEL", "InformationLevel", sizeof(nt::POWER_INFORMATION_LEVEL)}, {"PVOID", "InputBuffer", sizeof(nt::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt::ULONG)}}},
        {"NtPrePrepareEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtPrivilegeObjectAuditAlarm", 6, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt::PVOID)}, {"HANDLE", "ClientToken", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"PPRIVILEGE_SET", "Privileges", sizeof(nt::PPRIVILEGE_SET)}, {"BOOLEAN", "AccessGranted", sizeof(nt::BOOLEAN)}}},
        {"NtPropagationComplete", 4, {{"HANDLE", "ResourceManagerHandle", sizeof(nt::HANDLE)}, {"ULONG", "RequestCookie", sizeof(nt::ULONG)}, {"ULONG", "BufferLength", sizeof(nt::ULONG)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}}},
        {"NtPropagationFailed", 3, {{"HANDLE", "ResourceManagerHandle", sizeof(nt::HANDLE)}, {"ULONG", "RequestCookie", sizeof(nt::ULONG)}, {"NTSTATUS", "PropStatus", sizeof(nt::NTSTATUS)}}},
        {"NtQueryBootOptions", 2, {{"PBOOT_OPTIONS", "BootOptions", sizeof(nt::PBOOT_OPTIONS)}, {"PULONG", "BootOptionsLength", sizeof(nt::PULONG)}}},
        {"NtQueryDefaultLocale", 2, {{"BOOLEAN", "UserProfile", sizeof(nt::BOOLEAN)}, {"PLCID", "DefaultLocaleId", sizeof(nt::PLCID)}}},
        {"NtQueryDirectoryFile", 11, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(nt::FILE_INFORMATION_CLASS)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt::BOOLEAN)}, {"PUNICODE_STRING", "FileName", sizeof(nt::PUNICODE_STRING)}, {"BOOLEAN", "RestartScan", sizeof(nt::BOOLEAN)}}},
        {"NtQueryDriverEntryOrder", 2, {{"PULONG", "Ids", sizeof(nt::PULONG)}, {"PULONG", "Count", sizeof(nt::PULONG)}}},
        {"NtQueryEvent", 5, {{"HANDLE", "EventHandle", sizeof(nt::HANDLE)}, {"EVENT_INFORMATION_CLASS", "EventInformationClass", sizeof(nt::EVENT_INFORMATION_CLASS)}, {"PVOID", "EventInformation", sizeof(nt::PVOID)}, {"ULONG", "EventInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryFullAttributesFile", 2, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PFILE_NETWORK_OPEN_INFORMATION", "FileInformation", sizeof(nt::PFILE_NETWORK_OPEN_INFORMATION)}}},
        {"NtQueryInformationAtom", 5, {{"RTL_ATOM", "Atom", sizeof(nt::RTL_ATOM)}, {"ATOM_INFORMATION_CLASS", "InformationClass", sizeof(nt::ATOM_INFORMATION_CLASS)}, {"PVOID", "AtomInformation", sizeof(nt::PVOID)}, {"ULONG", "AtomInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryInformationEnlistment", 5, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"ENLISTMENT_INFORMATION_CLASS", "EnlistmentInformationClass", sizeof(nt::ENLISTMENT_INFORMATION_CLASS)}, {"PVOID", "EnlistmentInformation", sizeof(nt::PVOID)}, {"ULONG", "EnlistmentInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryInformationFile", 5, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(nt::FILE_INFORMATION_CLASS)}}},
        {"NtQueryInformationJobObject", 5, {{"HANDLE", "JobHandle", sizeof(nt::HANDLE)}, {"JOBOBJECTINFOCLASS", "JobObjectInformationClass", sizeof(nt::JOBOBJECTINFOCLASS)}, {"PVOID", "JobObjectInformation", sizeof(nt::PVOID)}, {"ULONG", "JobObjectInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryInformationPort", 5, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(nt::PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryInformationProcess", 5, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PROCESSINFOCLASS", "ProcessInformationClass", sizeof(nt::PROCESSINFOCLASS)}, {"PVOID", "ProcessInformation", sizeof(nt::PVOID)}, {"ULONG", "ProcessInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryInformationResourceManager", 5, {{"HANDLE", "ResourceManagerHandle", sizeof(nt::HANDLE)}, {"RESOURCEMANAGER_INFORMATION_CLASS", "ResourceManagerInformationClass", sizeof(nt::RESOURCEMANAGER_INFORMATION_CLASS)}, {"PVOID", "ResourceManagerInformation", sizeof(nt::PVOID)}, {"ULONG", "ResourceManagerInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryInformationToken", 5, {{"HANDLE", "TokenHandle", sizeof(nt::HANDLE)}, {"TOKEN_INFORMATION_CLASS", "TokenInformationClass", sizeof(nt::TOKEN_INFORMATION_CLASS)}, {"PVOID", "TokenInformation", sizeof(nt::PVOID)}, {"ULONG", "TokenInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryInformationTransactionManager", 5, {{"HANDLE", "TransactionManagerHandle", sizeof(nt::HANDLE)}, {"TRANSACTIONMANAGER_INFORMATION_CLASS", "TransactionManagerInformationClass", sizeof(nt::TRANSACTIONMANAGER_INFORMATION_CLASS)}, {"PVOID", "TransactionManagerInformation", sizeof(nt::PVOID)}, {"ULONG", "TransactionManagerInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryInstallUILanguage", 1, {{"LANGID", "STARInstallUILanguageId", sizeof(nt::LANGID)}}},
        {"NtQueryIntervalProfile", 2, {{"KPROFILE_SOURCE", "ProfileSource", sizeof(nt::KPROFILE_SOURCE)}, {"PULONG", "Interval", sizeof(nt::PULONG)}}},
        {"NtQueryIoCompletion", 5, {{"HANDLE", "IoCompletionHandle", sizeof(nt::HANDLE)}, {"IO_COMPLETION_INFORMATION_CLASS", "IoCompletionInformationClass", sizeof(nt::IO_COMPLETION_INFORMATION_CLASS)}, {"PVOID", "IoCompletionInformation", sizeof(nt::PVOID)}, {"ULONG", "IoCompletionInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryOpenSubKeys", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PULONG", "HandleCount", sizeof(nt::PULONG)}}},
        {"NtQueryOpenSubKeysEx", 4, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "BufferLength", sizeof(nt::ULONG)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"PULONG", "RequiredSize", sizeof(nt::PULONG)}}},
        {"NtQueryPerformanceCounter", 2, {{"PLARGE_INTEGER", "PerformanceCounter", sizeof(nt::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "PerformanceFrequency", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtQueryPortInformationProcess", 0, {}},
        {"NtQueryQuotaInformationFile", 9, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt::BOOLEAN)}, {"PVOID", "SidList", sizeof(nt::PVOID)}, {"ULONG", "SidListLength", sizeof(nt::ULONG)}, {"PULONG", "StartSid", sizeof(nt::PULONG)}, {"BOOLEAN", "RestartScan", sizeof(nt::BOOLEAN)}}},
        {"NtQuerySection", 5, {{"HANDLE", "SectionHandle", sizeof(nt::HANDLE)}, {"SECTION_INFORMATION_CLASS", "SectionInformationClass", sizeof(nt::SECTION_INFORMATION_CLASS)}, {"PVOID", "SectionInformation", sizeof(nt::PVOID)}, {"SIZE_T", "SectionInformationLength", sizeof(nt::SIZE_T)}, {"PSIZE_T", "ReturnLength", sizeof(nt::PSIZE_T)}}},
        {"NtQuerySecurityObject", 5, {{"HANDLE", "Handle", sizeof(nt::HANDLE)}, {"SECURITY_INFORMATION", "SecurityInformation", sizeof(nt::SECURITY_INFORMATION)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt::PSECURITY_DESCRIPTOR)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PULONG", "LengthNeeded", sizeof(nt::PULONG)}}},
        {"NtQuerySymbolicLinkObject", 3, {{"HANDLE", "LinkHandle", sizeof(nt::HANDLE)}, {"PUNICODE_STRING", "LinkTarget", sizeof(nt::PUNICODE_STRING)}, {"PULONG", "ReturnedLength", sizeof(nt::PULONG)}}},
        {"NtQuerySystemEnvironmentValue", 4, {{"PUNICODE_STRING", "VariableName", sizeof(nt::PUNICODE_STRING)}, {"PWSTR", "VariableValue", sizeof(nt::PWSTR)}, {"USHORT", "ValueLength", sizeof(nt::USHORT)}, {"PUSHORT", "ReturnLength", sizeof(nt::PUSHORT)}}},
        {"NtQuerySystemEnvironmentValueEx", 5, {{"PUNICODE_STRING", "VariableName", sizeof(nt::PUNICODE_STRING)}, {"LPGUID", "VendorGuid", sizeof(nt::LPGUID)}, {"PVOID", "Value", sizeof(nt::PVOID)}, {"PULONG", "ValueLength", sizeof(nt::PULONG)}, {"PULONG", "Attributes", sizeof(nt::PULONG)}}},
        {"NtQuerySystemInformation", 4, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(nt::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "SystemInformation", sizeof(nt::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQuerySystemInformationEx", 6, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(nt::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "QueryInformation", sizeof(nt::PVOID)}, {"ULONG", "QueryInformationLength", sizeof(nt::ULONG)}, {"PVOID", "SystemInformation", sizeof(nt::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtQueryValueKey", 6, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(nt::PUNICODE_STRING)}, {"KEY_VALUE_INFORMATION_CLASS", "KeyValueInformationClass", sizeof(nt::KEY_VALUE_INFORMATION_CLASS)}, {"PVOID", "KeyValueInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PULONG", "ResultLength", sizeof(nt::PULONG)}}},
        {"NtQueueApcThread", 5, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"PPS_APC_ROUTINE", "ApcRoutine", sizeof(nt::PPS_APC_ROUTINE)}, {"PVOID", "ApcArgument1", sizeof(nt::PVOID)}, {"PVOID", "ApcArgument2", sizeof(nt::PVOID)}, {"PVOID", "ApcArgument3", sizeof(nt::PVOID)}}},
        {"NtQueueApcThreadEx", 6, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"HANDLE", "UserApcReserveHandle", sizeof(nt::HANDLE)}, {"PPS_APC_ROUTINE", "ApcRoutine", sizeof(nt::PPS_APC_ROUTINE)}, {"PVOID", "ApcArgument1", sizeof(nt::PVOID)}, {"PVOID", "ApcArgument2", sizeof(nt::PVOID)}, {"PVOID", "ApcArgument3", sizeof(nt::PVOID)}}},
        {"NtRaiseHardError", 6, {{"NTSTATUS", "ErrorStatus", sizeof(nt::NTSTATUS)}, {"ULONG", "NumberOfParameters", sizeof(nt::ULONG)}, {"ULONG", "UnicodeStringParameterMask", sizeof(nt::ULONG)}, {"PULONG_PTR", "Parameters", sizeof(nt::PULONG_PTR)}, {"ULONG", "ValidResponseOptions", sizeof(nt::ULONG)}, {"PULONG", "Response", sizeof(nt::PULONG)}}},
        {"NtReadFile", 9, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt::PULONG)}}},
        {"NtReadRequestData", 6, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(nt::PPORT_MESSAGE)}, {"ULONG", "DataEntryIndex", sizeof(nt::ULONG)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt::SIZE_T)}, {"PSIZE_T", "NumberOfBytesRead", sizeof(nt::PSIZE_T)}}},
        {"NtRecoverTransactionManager", 1, {{"HANDLE", "TransactionManagerHandle", sizeof(nt::HANDLE)}}},
        {"NtRegisterProtocolAddressInformation", 5, {{"HANDLE", "ResourceManager", sizeof(nt::HANDLE)}, {"PCRM_PROTOCOL_ID", "ProtocolId", sizeof(nt::PCRM_PROTOCOL_ID)}, {"ULONG", "ProtocolInformationSize", sizeof(nt::ULONG)}, {"PVOID", "ProtocolInformation", sizeof(nt::PVOID)}, {"ULONG", "CreateOptions", sizeof(nt::ULONG)}}},
        {"NtRegisterThreadTerminatePort", 1, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}}},
        {"NtReleaseKeyedEvent", 4, {{"HANDLE", "KeyedEventHandle", sizeof(nt::HANDLE)}, {"PVOID", "KeyValue", sizeof(nt::PVOID)}, {"BOOLEAN", "Alertable", sizeof(nt::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtReleaseWorkerFactoryWorker", 1, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt::HANDLE)}}},
        {"NtRenameKey", 2, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"PUNICODE_STRING", "NewName", sizeof(nt::PUNICODE_STRING)}}},
        {"NtRenameTransactionManager", 2, {{"PUNICODE_STRING", "LogFileName", sizeof(nt::PUNICODE_STRING)}, {"LPGUID", "ExistingTransactionManagerGuid", sizeof(nt::LPGUID)}}},
        {"NtReplaceKey", 3, {{"POBJECT_ATTRIBUTES", "NewFile", sizeof(nt::POBJECT_ATTRIBUTES)}, {"HANDLE", "TargetHandle", sizeof(nt::HANDLE)}, {"POBJECT_ATTRIBUTES", "OldFile", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtReplacePartitionUnit", 3, {{"PUNICODE_STRING", "TargetInstancePath", sizeof(nt::PUNICODE_STRING)}, {"PUNICODE_STRING", "SpareInstancePath", sizeof(nt::PUNICODE_STRING)}, {"ULONG", "Flags", sizeof(nt::ULONG)}}},
        {"NtReplyPort", 2, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt::PPORT_MESSAGE)}}},
        {"NtReplyWaitReplyPort", 2, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt::PPORT_MESSAGE)}}},
        {"NtRequestPort", 2, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "RequestMessage", sizeof(nt::PPORT_MESSAGE)}}},
        {"NtRestoreKey", 3, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}}},
        {"NtRollbackEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtRollforwardTransactionManager", 2, {{"HANDLE", "TransactionManagerHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtSaveKey", 2, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt::HANDLE)}}},
        {"NtSaveKeyEx", 3, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"ULONG", "Format", sizeof(nt::ULONG)}}},
        {"NtSecureConnectPort", 9, {{"PHANDLE", "PortHandle", sizeof(nt::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(nt::PUNICODE_STRING)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(nt::PSECURITY_QUALITY_OF_SERVICE)}, {"PPORT_VIEW", "ClientView", sizeof(nt::PPORT_VIEW)}, {"PSID", "RequiredServerSid", sizeof(nt::PSID)}, {"PREMOTE_PORT_VIEW", "ServerView", sizeof(nt::PREMOTE_PORT_VIEW)}, {"PULONG", "MaxMessageLength", sizeof(nt::PULONG)}, {"PVOID", "ConnectionInformation", sizeof(nt::PVOID)}, {"PULONG", "ConnectionInformationLength", sizeof(nt::PULONG)}}},
        {"NtSetBootOptions", 2, {{"PBOOT_OPTIONS", "BootOptions", sizeof(nt::PBOOT_OPTIONS)}, {"ULONG", "FieldsToChange", sizeof(nt::ULONG)}}},
        {"NtSetContextThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"PCONTEXT", "ThreadContext", sizeof(nt::PCONTEXT)}}},
        {"NtSetDefaultHardErrorPort", 1, {{"HANDLE", "DefaultHardErrorPort", sizeof(nt::HANDLE)}}},
        {"NtSetDefaultLocale", 2, {{"BOOLEAN", "UserProfile", sizeof(nt::BOOLEAN)}, {"LCID", "DefaultLocaleId", sizeof(nt::LCID)}}},
        {"NtSetDriverEntryOrder", 2, {{"PULONG", "Ids", sizeof(nt::PULONG)}, {"ULONG", "Count", sizeof(nt::ULONG)}}},
        {"NtSetHighEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt::HANDLE)}}},
        {"NtSetInformationDebugObject", 5, {{"HANDLE", "DebugObjectHandle", sizeof(nt::HANDLE)}, {"DEBUGOBJECTINFOCLASS", "DebugObjectInformationClass", sizeof(nt::DEBUGOBJECTINFOCLASS)}, {"PVOID", "DebugInformation", sizeof(nt::PVOID)}, {"ULONG", "DebugInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtSetInformationEnlistment", 4, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"ENLISTMENT_INFORMATION_CLASS", "EnlistmentInformationClass", sizeof(nt::ENLISTMENT_INFORMATION_CLASS)}, {"PVOID", "EnlistmentInformation", sizeof(nt::PVOID)}, {"ULONG", "EnlistmentInformationLength", sizeof(nt::ULONG)}}},
        {"NtSetInformationObject", 4, {{"HANDLE", "Handle", sizeof(nt::HANDLE)}, {"OBJECT_INFORMATION_CLASS", "ObjectInformationClass", sizeof(nt::OBJECT_INFORMATION_CLASS)}, {"PVOID", "ObjectInformation", sizeof(nt::PVOID)}, {"ULONG", "ObjectInformationLength", sizeof(nt::ULONG)}}},
        {"NtSetInformationProcess", 4, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PROCESSINFOCLASS", "ProcessInformationClass", sizeof(nt::PROCESSINFOCLASS)}, {"PVOID", "ProcessInformation", sizeof(nt::PVOID)}, {"ULONG", "ProcessInformationLength", sizeof(nt::ULONG)}}},
        {"NtSetInformationResourceManager", 4, {{"HANDLE", "ResourceManagerHandle", sizeof(nt::HANDLE)}, {"RESOURCEMANAGER_INFORMATION_CLASS", "ResourceManagerInformationClass", sizeof(nt::RESOURCEMANAGER_INFORMATION_CLASS)}, {"PVOID", "ResourceManagerInformation", sizeof(nt::PVOID)}, {"ULONG", "ResourceManagerInformationLength", sizeof(nt::ULONG)}}},
        {"NtSetInformationThread", 4, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"THREADINFOCLASS", "ThreadInformationClass", sizeof(nt::THREADINFOCLASS)}, {"PVOID", "ThreadInformation", sizeof(nt::PVOID)}, {"ULONG", "ThreadInformationLength", sizeof(nt::ULONG)}}},
        {"NtSetInformationToken", 4, {{"HANDLE", "TokenHandle", sizeof(nt::HANDLE)}, {"TOKEN_INFORMATION_CLASS", "TokenInformationClass", sizeof(nt::TOKEN_INFORMATION_CLASS)}, {"PVOID", "TokenInformation", sizeof(nt::PVOID)}, {"ULONG", "TokenInformationLength", sizeof(nt::ULONG)}}},
        {"NtSetInformationTransaction", 4, {{"HANDLE", "TransactionHandle", sizeof(nt::HANDLE)}, {"TRANSACTION_INFORMATION_CLASS", "TransactionInformationClass", sizeof(nt::TRANSACTION_INFORMATION_CLASS)}, {"PVOID", "TransactionInformation", sizeof(nt::PVOID)}, {"ULONG", "TransactionInformationLength", sizeof(nt::ULONG)}}},
        {"NtSetInformationWorkerFactory", 4, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt::HANDLE)}, {"WORKERFACTORYINFOCLASS", "WorkerFactoryInformationClass", sizeof(nt::WORKERFACTORYINFOCLASS)}, {"PVOID", "WorkerFactoryInformation", sizeof(nt::PVOID)}, {"ULONG", "WorkerFactoryInformationLength", sizeof(nt::ULONG)}}},
        {"NtSetIntervalProfile", 2, {{"ULONG", "Interval", sizeof(nt::ULONG)}, {"KPROFILE_SOURCE", "Source", sizeof(nt::KPROFILE_SOURCE)}}},
        {"NtSetSecurityObject", 3, {{"HANDLE", "Handle", sizeof(nt::HANDLE)}, {"SECURITY_INFORMATION", "SecurityInformation", sizeof(nt::SECURITY_INFORMATION)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt::PSECURITY_DESCRIPTOR)}}},
        {"NtSetThreadExecutionState", 2, {{"EXECUTION_STATE", "esFlags", sizeof(nt::EXECUTION_STATE)}, {"EXECUTION_STATE", "STARPreviousFlags", sizeof(nt::EXECUTION_STATE)}}},
        {"NtSetTimerResolution", 3, {{"ULONG", "DesiredTime", sizeof(nt::ULONG)}, {"BOOLEAN", "SetResolution", sizeof(nt::BOOLEAN)}, {"PULONG", "ActualTime", sizeof(nt::PULONG)}}},
        {"NtSetUuidSeed", 1, {{"PCHAR", "Seed", sizeof(nt::PCHAR)}}},
        {"NtSuspendThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(nt::PULONG)}}},
        {"NtTerminateJobObject", 2, {{"HANDLE", "JobHandle", sizeof(nt::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(nt::NTSTATUS)}}},
        {"NtTerminateThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(nt::NTSTATUS)}}},
        {"NtTraceControl", 6, {{"ULONG", "FunctionCode", sizeof(nt::ULONG)}, {"PVOID", "InBuffer", sizeof(nt::PVOID)}, {"ULONG", "InBufferLen", sizeof(nt::ULONG)}, {"PVOID", "OutBuffer", sizeof(nt::PVOID)}, {"ULONG", "OutBufferLen", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"NtTraceEvent", 4, {{"HANDLE", "TraceHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"ULONG", "FieldSize", sizeof(nt::ULONG)}, {"PVOID", "Fields", sizeof(nt::PVOID)}}},
        {"NtTranslateFilePath", 4, {{"PFILE_PATH", "InputFilePath", sizeof(nt::PFILE_PATH)}, {"ULONG", "OutputType", sizeof(nt::ULONG)}, {"PFILE_PATH", "OutputFilePath", sizeof(nt::PFILE_PATH)}, {"PULONG", "OutputFilePathLength", sizeof(nt::PULONG)}}},
        {"NtUnloadKey", 1, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"NtUnloadKey2", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt::ULONG)}}},
        {"NtUnmapViewOfSection", 2, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}}},
        {"NtWaitForKeyedEvent", 4, {{"HANDLE", "KeyedEventHandle", sizeof(nt::HANDLE)}, {"PVOID", "KeyValue", sizeof(nt::PVOID)}, {"BOOLEAN", "Alertable", sizeof(nt::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtWaitForMultipleObjects", 5, {{"ULONG", "Count", sizeof(nt::ULONG)}, {"HANDLE", "Handles", sizeof(nt::HANDLE)}, {"WAIT_TYPE", "WaitType", sizeof(nt::WAIT_TYPE)}, {"BOOLEAN", "Alertable", sizeof(nt::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtWaitForSingleObject", 3, {{"HANDLE", "Handle", sizeof(nt::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(nt::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"NtWaitLowEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt::HANDLE)}}},
        {"NtWorkerFactoryWorkerReady", 1, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt::HANDLE)}}},
        {"NtWriteFile", 9, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt::PULONG)}}},
        {"NtWriteVirtualMemory", 5, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt::SIZE_T)}, {"PSIZE_T", "NumberOfBytesWritten", sizeof(nt::PSIZE_T)}}},
        {"ZwAccessCheck", 8, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt::PSECURITY_DESCRIPTOR)}, {"HANDLE", "ClientToken", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(nt::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(nt::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt::PNTSTATUS)}}},
        {"ZwAccessCheckAndAuditAlarm", 11, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt::PSECURITY_DESCRIPTOR)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt::PBOOLEAN)}}},
        {"ZwAccessCheckByType", 11, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt::PSID)}, {"HANDLE", "ClientToken", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(nt::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(nt::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt::PNTSTATUS)}}},
        {"ZwAccessCheckByTypeAndAuditAlarm", 16, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(nt::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt::PBOOLEAN)}}},
        {"ZwAccessCheckByTypeResultList", 11, {{"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt::PSID)}, {"HANDLE", "ClientToken", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt::PGENERIC_MAPPING)}, {"PPRIVILEGE_SET", "PrivilegeSet", sizeof(nt::PPRIVILEGE_SET)}, {"PULONG", "PrivilegeSetLength", sizeof(nt::PULONG)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt::PNTSTATUS)}}},
        {"ZwAccessCheckByTypeResultListAndAuditAlarm", 16, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(nt::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt::PBOOLEAN)}}},
        {"ZwAccessCheckByTypeResultListAndAuditAlarmByHandle", 17, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt::PVOID)}, {"HANDLE", "ClientToken", sizeof(nt::HANDLE)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt::PSECURITY_DESCRIPTOR)}, {"PSID", "PrincipalSelfSid", sizeof(nt::PSID)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"AUDIT_EVENT_TYPE", "AuditType", sizeof(nt::AUDIT_EVENT_TYPE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"POBJECT_TYPE_LIST", "ObjectTypeList", sizeof(nt::POBJECT_TYPE_LIST)}, {"ULONG", "ObjectTypeListLength", sizeof(nt::ULONG)}, {"PGENERIC_MAPPING", "GenericMapping", sizeof(nt::PGENERIC_MAPPING)}, {"BOOLEAN", "ObjectCreation", sizeof(nt::BOOLEAN)}, {"PACCESS_MASK", "GrantedAccess", sizeof(nt::PACCESS_MASK)}, {"PNTSTATUS", "AccessStatus", sizeof(nt::PNTSTATUS)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt::PBOOLEAN)}}},
        {"ZwAddAtom", 3, {{"PWSTR", "AtomName", sizeof(nt::PWSTR)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PRTL_ATOM", "Atom", sizeof(nt::PRTL_ATOM)}}},
        {"ZwAddBootEntry", 2, {{"PBOOT_ENTRY", "BootEntry", sizeof(nt::PBOOT_ENTRY)}, {"PULONG", "Id", sizeof(nt::PULONG)}}},
        {"ZwAlertResumeThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(nt::PULONG)}}},
        {"ZwAllocateLocallyUniqueId", 1, {{"PLUID", "Luid", sizeof(nt::PLUID)}}},
        {"ZwAllocateUserPhysicalPages", 3, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PULONG_PTR", "NumberOfPages", sizeof(nt::PULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(nt::PULONG_PTR)}}},
        {"ZwAllocateUuids", 4, {{"PULARGE_INTEGER", "Time", sizeof(nt::PULARGE_INTEGER)}, {"PULONG", "Range", sizeof(nt::PULONG)}, {"PULONG", "Sequence", sizeof(nt::PULONG)}, {"PCHAR", "Seed", sizeof(nt::PCHAR)}}},
        {"ZwAllocateVirtualMemory", 6, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt::PVOID)}, {"ULONG_PTR", "ZeroBits", sizeof(nt::ULONG_PTR)}, {"PSIZE_T", "RegionSize", sizeof(nt::PSIZE_T)}, {"ULONG", "AllocationType", sizeof(nt::ULONG)}, {"ULONG", "Protect", sizeof(nt::ULONG)}}},
        {"ZwAlpcAcceptConnectPort", 9, {{"PHANDLE", "PortHandle", sizeof(nt::PHANDLE)}, {"HANDLE", "ConnectionPortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PALPC_PORT_ATTRIBUTES", "PortAttributes", sizeof(nt::PALPC_PORT_ATTRIBUTES)}, {"PVOID", "PortContext", sizeof(nt::PVOID)}, {"PPORT_MESSAGE", "ConnectionRequest", sizeof(nt::PPORT_MESSAGE)}, {"PALPC_MESSAGE_ATTRIBUTES", "ConnectionMessageAttributes", sizeof(nt::PALPC_MESSAGE_ATTRIBUTES)}, {"BOOLEAN", "AcceptConnection", sizeof(nt::BOOLEAN)}}},
        {"ZwAlpcCreatePortSection", 6, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"HANDLE", "SectionHandle", sizeof(nt::HANDLE)}, {"SIZE_T", "SectionSize", sizeof(nt::SIZE_T)}, {"PALPC_HANDLE", "AlpcSectionHandle", sizeof(nt::PALPC_HANDLE)}, {"PSIZE_T", "ActualSectionSize", sizeof(nt::PSIZE_T)}}},
        {"ZwAlpcCreateResourceReserve", 4, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"SIZE_T", "MessageSize", sizeof(nt::SIZE_T)}, {"PALPC_HANDLE", "ResourceId", sizeof(nt::PALPC_HANDLE)}}},
        {"ZwAlpcDeleteSectionView", 3, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PVOID", "ViewBase", sizeof(nt::PVOID)}}},
        {"ZwAlpcDeleteSecurityContext", 3, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"ALPC_HANDLE", "ContextHandle", sizeof(nt::ALPC_HANDLE)}}},
        {"ZwAlpcQueryInformation", 5, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ALPC_PORT_INFORMATION_CLASS", "PortInformationClass", sizeof(nt::ALPC_PORT_INFORMATION_CLASS)}, {"PVOID", "PortInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwAlpcQueryInformationMessage", 6, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "PortMessage", sizeof(nt::PPORT_MESSAGE)}, {"ALPC_MESSAGE_INFORMATION_CLASS", "MessageInformationClass", sizeof(nt::ALPC_MESSAGE_INFORMATION_CLASS)}, {"PVOID", "MessageInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwAlpcSendWaitReceivePort", 8, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PPORT_MESSAGE", "SendMessage", sizeof(nt::PPORT_MESSAGE)}, {"PALPC_MESSAGE_ATTRIBUTES", "SendMessageAttributes", sizeof(nt::PALPC_MESSAGE_ATTRIBUTES)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(nt::PPORT_MESSAGE)}, {"PULONG", "BufferLength", sizeof(nt::PULONG)}, {"PALPC_MESSAGE_ATTRIBUTES", "ReceiveMessageAttributes", sizeof(nt::PALPC_MESSAGE_ATTRIBUTES)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwAreMappedFilesTheSame", 2, {{"PVOID", "File1MappedAsAnImage", sizeof(nt::PVOID)}, {"PVOID", "File2MappedAsFile", sizeof(nt::PVOID)}}},
        {"ZwCancelIoFile", 2, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}}},
        {"ZwCancelIoFileEx", 3, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoRequestToCancel", sizeof(nt::PIO_STATUS_BLOCK)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}}},
        {"ZwCancelSynchronousIoFile", 3, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoRequestToCancel", sizeof(nt::PIO_STATUS_BLOCK)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}}},
        {"ZwCloseObjectAuditAlarm", 3, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt::PVOID)}, {"BOOLEAN", "GenerateOnClose", sizeof(nt::BOOLEAN)}}},
        {"ZwCommitEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwCommitTransaction", 2, {{"HANDLE", "TransactionHandle", sizeof(nt::HANDLE)}, {"BOOLEAN", "Wait", sizeof(nt::BOOLEAN)}}},
        {"ZwCompareTokens", 3, {{"HANDLE", "FirstTokenHandle", sizeof(nt::HANDLE)}, {"HANDLE", "SecondTokenHandle", sizeof(nt::HANDLE)}, {"PBOOLEAN", "Equal", sizeof(nt::PBOOLEAN)}}},
        {"ZwConnectPort", 8, {{"PHANDLE", "PortHandle", sizeof(nt::PHANDLE)}, {"PUNICODE_STRING", "PortName", sizeof(nt::PUNICODE_STRING)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(nt::PSECURITY_QUALITY_OF_SERVICE)}, {"PPORT_VIEW", "ClientView", sizeof(nt::PPORT_VIEW)}, {"PREMOTE_PORT_VIEW", "ServerView", sizeof(nt::PREMOTE_PORT_VIEW)}, {"PULONG", "MaxMessageLength", sizeof(nt::PULONG)}, {"PVOID", "ConnectionInformation", sizeof(nt::PVOID)}, {"PULONG", "ConnectionInformationLength", sizeof(nt::PULONG)}}},
        {"ZwContinue", 2, {{"PCONTEXT", "ContextRecord", sizeof(nt::PCONTEXT)}, {"BOOLEAN", "TestAlert", sizeof(nt::BOOLEAN)}}},
        {"ZwCreateEvent", 5, {{"PHANDLE", "EventHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"EVENT_TYPE", "EventType", sizeof(nt::EVENT_TYPE)}, {"BOOLEAN", "InitialState", sizeof(nt::BOOLEAN)}}},
        {"ZwCreateEventPair", 3, {{"PHANDLE", "EventPairHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"ZwCreateFile", 11, {{"PHANDLE", "FileHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "AllocationSize", sizeof(nt::PLARGE_INTEGER)}, {"ULONG", "FileAttributes", sizeof(nt::ULONG)}, {"ULONG", "ShareAccess", sizeof(nt::ULONG)}, {"ULONG", "CreateDisposition", sizeof(nt::ULONG)}, {"ULONG", "CreateOptions", sizeof(nt::ULONG)}, {"PVOID", "EaBuffer", sizeof(nt::PVOID)}, {"ULONG", "EaLength", sizeof(nt::ULONG)}}},
        {"ZwCreateJobObject", 3, {{"PHANDLE", "JobHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"ZwCreateJobSet", 3, {{"ULONG", "NumJob", sizeof(nt::ULONG)}, {"PJOB_SET_ARRAY", "UserJobSet", sizeof(nt::PJOB_SET_ARRAY)}, {"ULONG", "Flags", sizeof(nt::ULONG)}}},
        {"ZwCreateKey", 7, {{"PHANDLE", "KeyHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "TitleIndex", sizeof(nt::ULONG)}, {"PUNICODE_STRING", "Class", sizeof(nt::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(nt::ULONG)}, {"PULONG", "Disposition", sizeof(nt::PULONG)}}},
        {"ZwCreateKeyTransacted", 8, {{"PHANDLE", "KeyHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "TitleIndex", sizeof(nt::ULONG)}, {"PUNICODE_STRING", "Class", sizeof(nt::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(nt::ULONG)}, {"HANDLE", "TransactionHandle", sizeof(nt::HANDLE)}, {"PULONG", "Disposition", sizeof(nt::PULONG)}}},
        {"ZwCreateKeyedEvent", 4, {{"PHANDLE", "KeyedEventHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt::ULONG)}}},
        {"ZwCreateMailslotFile", 8, {{"PHANDLE", "FileHandle", sizeof(nt::PHANDLE)}, {"ULONG", "DesiredAccess", sizeof(nt::ULONG)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"ULONG", "CreateOptions", sizeof(nt::ULONG)}, {"ULONG", "MailslotQuota", sizeof(nt::ULONG)}, {"ULONG", "MaximumMessageSize", sizeof(nt::ULONG)}, {"PLARGE_INTEGER", "ReadTimeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwCreatePort", 5, {{"PHANDLE", "PortHandle", sizeof(nt::PHANDLE)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "MaxConnectionInfoLength", sizeof(nt::ULONG)}, {"ULONG", "MaxMessageLength", sizeof(nt::ULONG)}, {"ULONG", "MaxPoolUsage", sizeof(nt::ULONG)}}},
        {"ZwCreatePrivateNamespace", 4, {{"PHANDLE", "NamespaceHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PVOID", "BoundaryDescriptor", sizeof(nt::PVOID)}}},
        {"ZwCreateProcessEx", 9, {{"PHANDLE", "ProcessHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"HANDLE", "ParentProcess", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"HANDLE", "SectionHandle", sizeof(nt::HANDLE)}, {"HANDLE", "DebugPort", sizeof(nt::HANDLE)}, {"HANDLE", "ExceptionPort", sizeof(nt::HANDLE)}, {"ULONG", "JobMemberLevel", sizeof(nt::ULONG)}}},
        {"ZwCreateProfile", 9, {{"PHANDLE", "ProfileHandle", sizeof(nt::PHANDLE)}, {"HANDLE", "Process", sizeof(nt::HANDLE)}, {"PVOID", "RangeBase", sizeof(nt::PVOID)}, {"SIZE_T", "RangeSize", sizeof(nt::SIZE_T)}, {"ULONG", "BucketSize", sizeof(nt::ULONG)}, {"PULONG", "Buffer", sizeof(nt::PULONG)}, {"ULONG", "BufferSize", sizeof(nt::ULONG)}, {"KPROFILE_SOURCE", "ProfileSource", sizeof(nt::KPROFILE_SOURCE)}, {"KAFFINITY", "Affinity", sizeof(nt::KAFFINITY)}}},
        {"ZwCreateProfileEx", 10, {{"PHANDLE", "ProfileHandle", sizeof(nt::PHANDLE)}, {"HANDLE", "Process", sizeof(nt::HANDLE)}, {"PVOID", "ProfileBase", sizeof(nt::PVOID)}, {"SIZE_T", "ProfileSize", sizeof(nt::SIZE_T)}, {"ULONG", "BucketSize", sizeof(nt::ULONG)}, {"PULONG", "Buffer", sizeof(nt::PULONG)}, {"ULONG", "BufferSize", sizeof(nt::ULONG)}, {"KPROFILE_SOURCE", "ProfileSource", sizeof(nt::KPROFILE_SOURCE)}, {"ULONG", "GroupAffinityCount", sizeof(nt::ULONG)}, {"PGROUP_AFFINITY", "GroupAffinity", sizeof(nt::PGROUP_AFFINITY)}}},
        {"ZwCreateSymbolicLinkObject", 4, {{"PHANDLE", "LinkHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LinkTarget", sizeof(nt::PUNICODE_STRING)}}},
        {"ZwCreateTimer", 4, {{"PHANDLE", "TimerHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"TIMER_TYPE", "TimerType", sizeof(nt::TIMER_TYPE)}}},
        {"ZwCreateTransactionManager", 6, {{"PHANDLE", "TmHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PUNICODE_STRING", "LogFileName", sizeof(nt::PUNICODE_STRING)}, {"ULONG", "CreateOptions", sizeof(nt::ULONG)}, {"ULONG", "CommitStrength", sizeof(nt::ULONG)}}},
        {"ZwDelayExecution", 2, {{"BOOLEAN", "Alertable", sizeof(nt::BOOLEAN)}, {"PLARGE_INTEGER", "DelayInterval", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwDeleteDriverEntry", 1, {{"ULONG", "Id", sizeof(nt::ULONG)}}},
        {"ZwDeleteObjectAuditAlarm", 3, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt::PVOID)}, {"BOOLEAN", "GenerateOnClose", sizeof(nt::BOOLEAN)}}},
        {"ZwDeleteValueKey", 2, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(nt::PUNICODE_STRING)}}},
        {"ZwDisplayString", 1, {{"PUNICODE_STRING", "String", sizeof(nt::PUNICODE_STRING)}}},
        {"ZwDuplicateObject", 7, {{"HANDLE", "SourceProcessHandle", sizeof(nt::HANDLE)}, {"HANDLE", "SourceHandle", sizeof(nt::HANDLE)}, {"HANDLE", "TargetProcessHandle", sizeof(nt::HANDLE)}, {"PHANDLE", "TargetHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt::ULONG)}, {"ULONG", "Options", sizeof(nt::ULONG)}}},
        {"ZwEnumerateDriverEntries", 2, {{"PVOID", "Buffer", sizeof(nt::PVOID)}, {"PULONG", "BufferLength", sizeof(nt::PULONG)}}},
        {"ZwEnumerateValueKey", 6, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"ULONG", "Index", sizeof(nt::ULONG)}, {"KEY_VALUE_INFORMATION_CLASS", "KeyValueInformationClass", sizeof(nt::KEY_VALUE_INFORMATION_CLASS)}, {"PVOID", "KeyValueInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PULONG", "ResultLength", sizeof(nt::PULONG)}}},
        {"ZwFilterToken", 6, {{"HANDLE", "ExistingTokenHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PTOKEN_GROUPS", "SidsToDisable", sizeof(nt::PTOKEN_GROUPS)}, {"PTOKEN_PRIVILEGES", "PrivilegesToDelete", sizeof(nt::PTOKEN_PRIVILEGES)}, {"PTOKEN_GROUPS", "RestrictedSids", sizeof(nt::PTOKEN_GROUPS)}, {"PHANDLE", "NewTokenHandle", sizeof(nt::PHANDLE)}}},
        {"ZwFindAtom", 3, {{"PWSTR", "AtomName", sizeof(nt::PWSTR)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PRTL_ATOM", "Atom", sizeof(nt::PRTL_ATOM)}}},
        {"ZwFlushBuffersFile", 2, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}}},
        {"ZwFlushKey", 1, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}}},
        {"ZwFlushProcessWriteBuffers", 0, {}},
        {"ZwFlushVirtualMemory", 4, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt::PSIZE_T)}, {"PIO_STATUS_BLOCK", "IoStatus", sizeof(nt::PIO_STATUS_BLOCK)}}},
        {"ZwFreeUserPhysicalPages", 3, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PULONG_PTR", "NumberOfPages", sizeof(nt::PULONG_PTR)}, {"PULONG_PTR", "UserPfnArra", sizeof(nt::PULONG_PTR)}}},
        {"ZwFreeVirtualMemory", 4, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt::PSIZE_T)}, {"ULONG", "FreeType", sizeof(nt::ULONG)}}},
        {"ZwFreezeTransactions", 2, {{"PLARGE_INTEGER", "FreezeTimeout", sizeof(nt::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "ThawTimeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwFsControlFile", 10, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"ULONG", "IoControlCode", sizeof(nt::ULONG)}, {"PVOID", "InputBuffer", sizeof(nt::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt::ULONG)}}},
        {"ZwGetContextThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"PCONTEXT", "ThreadContext", sizeof(nt::PCONTEXT)}}},
        {"ZwGetCurrentProcessorNumber", 0, {}},
        {"ZwGetDevicePowerState", 2, {{"HANDLE", "Device", sizeof(nt::HANDLE)}, {"DEVICE_POWER_STATE", "STARState", sizeof(nt::DEVICE_POWER_STATE)}}},
        {"ZwGetMUIRegistryInfo", 3, {{"ULONG", "Flags", sizeof(nt::ULONG)}, {"PULONG", "DataSize", sizeof(nt::PULONG)}, {"PVOID", "Data", sizeof(nt::PVOID)}}},
        {"ZwGetNextThread", 6, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"ULONG", "HandleAttributes", sizeof(nt::ULONG)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PHANDLE", "NewThreadHandle", sizeof(nt::PHANDLE)}}},
        {"ZwGetNlsSectionPtr", 5, {{"ULONG", "SectionType", sizeof(nt::ULONG)}, {"ULONG", "SectionData", sizeof(nt::ULONG)}, {"PVOID", "ContextData", sizeof(nt::PVOID)}, {"PVOID", "STARSectionPointer", sizeof(nt::PVOID)}, {"PULONG", "SectionSize", sizeof(nt::PULONG)}}},
        {"ZwGetWriteWatch", 7, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}, {"SIZE_T", "RegionSize", sizeof(nt::SIZE_T)}, {"PVOID", "STARUserAddressArray", sizeof(nt::PVOID)}, {"PULONG_PTR", "EntriesInUserAddressArray", sizeof(nt::PULONG_PTR)}, {"PULONG", "Granularity", sizeof(nt::PULONG)}}},
        {"ZwImpersonateAnonymousToken", 1, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}}},
        {"ZwImpersonateClientOfPort", 2, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(nt::PPORT_MESSAGE)}}},
        {"ZwImpersonateThread", 3, {{"HANDLE", "ServerThreadHandle", sizeof(nt::HANDLE)}, {"HANDLE", "ClientThreadHandle", sizeof(nt::HANDLE)}, {"PSECURITY_QUALITY_OF_SERVICE", "SecurityQos", sizeof(nt::PSECURITY_QUALITY_OF_SERVICE)}}},
        {"ZwIsSystemResumeAutomatic", 0, {}},
        {"ZwLoadDriver", 1, {{"PUNICODE_STRING", "DriverServiceName", sizeof(nt::PUNICODE_STRING)}}},
        {"ZwLoadKey2", 3, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt::POBJECT_ATTRIBUTES)}, {"POBJECT_ATTRIBUTES", "SourceFile", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "Flags", sizeof(nt::ULONG)}}},
        {"ZwLockFile", 10, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "Length", sizeof(nt::PLARGE_INTEGER)}, {"ULONG", "Key", sizeof(nt::ULONG)}, {"BOOLEAN", "FailImmediately", sizeof(nt::BOOLEAN)}, {"BOOLEAN", "ExclusiveLock", sizeof(nt::BOOLEAN)}}},
        {"ZwLockRegistryKey", 1, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}}},
        {"ZwMapUserPhysicalPagesScatter", 3, {{"PVOID", "STARVirtualAddresses", sizeof(nt::PVOID)}, {"ULONG_PTR", "NumberOfPages", sizeof(nt::ULONG_PTR)}, {"PULONG_PTR", "UserPfnArray", sizeof(nt::PULONG_PTR)}}},
        {"ZwMapViewOfSection", 10, {{"HANDLE", "SectionHandle", sizeof(nt::HANDLE)}, {"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt::PVOID)}, {"ULONG_PTR", "ZeroBits", sizeof(nt::ULONG_PTR)}, {"SIZE_T", "CommitSize", sizeof(nt::SIZE_T)}, {"PLARGE_INTEGER", "SectionOffset", sizeof(nt::PLARGE_INTEGER)}, {"PSIZE_T", "ViewSize", sizeof(nt::PSIZE_T)}, {"SECTION_INHERIT", "InheritDisposition", sizeof(nt::SECTION_INHERIT)}, {"ULONG", "AllocationType", sizeof(nt::ULONG)}, {"WIN32_PROTECTION_MASK", "Win32Protect", sizeof(nt::WIN32_PROTECTION_MASK)}}},
        {"ZwModifyDriverEntry", 1, {{"PEFI_DRIVER_ENTRY", "DriverEntry", sizeof(nt::PEFI_DRIVER_ENTRY)}}},
        {"ZwNotifyChangeKey", 10, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"ULONG", "CompletionFilter", sizeof(nt::ULONG)}, {"BOOLEAN", "WatchTree", sizeof(nt::BOOLEAN)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "BufferSize", sizeof(nt::ULONG)}, {"BOOLEAN", "Asynchronous", sizeof(nt::BOOLEAN)}}},
        {"ZwOpenDirectoryObject", 3, {{"PHANDLE", "DirectoryHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"ZwOpenEventPair", 3, {{"PHANDLE", "EventPairHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"ZwOpenKeyTransacted", 4, {{"PHANDLE", "KeyHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"HANDLE", "TransactionHandle", sizeof(nt::HANDLE)}}},
        {"ZwOpenKeyTransactedEx", 5, {{"PHANDLE", "KeyHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"ULONG", "OpenOptions", sizeof(nt::ULONG)}, {"HANDLE", "TransactionHandle", sizeof(nt::HANDLE)}}},
        {"ZwOpenObjectAuditAlarm", 12, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt::PUNICODE_STRING)}, {"PVOID", "HandleId", sizeof(nt::PVOID)}, {"PUNICODE_STRING", "ObjectTypeName", sizeof(nt::PUNICODE_STRING)}, {"PUNICODE_STRING", "ObjectName", sizeof(nt::PUNICODE_STRING)}, {"PSECURITY_DESCRIPTOR", "SecurityDescriptor", sizeof(nt::PSECURITY_DESCRIPTOR)}, {"HANDLE", "ClientToken", sizeof(nt::HANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"ACCESS_MASK", "GrantedAccess", sizeof(nt::ACCESS_MASK)}, {"PPRIVILEGE_SET", "Privileges", sizeof(nt::PPRIVILEGE_SET)}, {"BOOLEAN", "ObjectCreation", sizeof(nt::BOOLEAN)}, {"BOOLEAN", "AccessGranted", sizeof(nt::BOOLEAN)}, {"PBOOLEAN", "GenerateOnClose", sizeof(nt::PBOOLEAN)}}},
        {"ZwOpenSemaphore", 3, {{"PHANDLE", "SemaphoreHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"ZwOpenSession", 3, {{"PHANDLE", "SessionHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"ZwOpenSymbolicLinkObject", 3, {{"PHANDLE", "LinkHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"ZwOpenThread", 4, {{"PHANDLE", "ThreadHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PCLIENT_ID", "ClientId", sizeof(nt::PCLIENT_ID)}}},
        {"ZwOpenTimer", 3, {{"PHANDLE", "TimerHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}}},
        {"ZwOpenTransaction", 5, {{"PHANDLE", "TransactionHandle", sizeof(nt::PHANDLE)}, {"ACCESS_MASK", "DesiredAccess", sizeof(nt::ACCESS_MASK)}, {"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"LPGUID", "Uow", sizeof(nt::LPGUID)}, {"HANDLE", "TmHandle", sizeof(nt::HANDLE)}}},
        {"ZwPlugPlayControl", 3, {{"PLUGPLAY_CONTROL_CLASS", "PnPControlClass", sizeof(nt::PLUGPLAY_CONTROL_CLASS)}, {"PVOID", "PnPControlData", sizeof(nt::PVOID)}, {"ULONG", "PnPControlDataLength", sizeof(nt::ULONG)}}},
        {"ZwPrePrepareComplete", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwPrepareComplete", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwPrepareEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwPrivilegeCheck", 3, {{"HANDLE", "ClientToken", sizeof(nt::HANDLE)}, {"PPRIVILEGE_SET", "RequiredPrivileges", sizeof(nt::PPRIVILEGE_SET)}, {"PBOOLEAN", "Result", sizeof(nt::PBOOLEAN)}}},
        {"ZwPrivilegedServiceAuditAlarm", 5, {{"PUNICODE_STRING", "SubsystemName", sizeof(nt::PUNICODE_STRING)}, {"PUNICODE_STRING", "ServiceName", sizeof(nt::PUNICODE_STRING)}, {"HANDLE", "ClientToken", sizeof(nt::HANDLE)}, {"PPRIVILEGE_SET", "Privileges", sizeof(nt::PPRIVILEGE_SET)}, {"BOOLEAN", "AccessGranted", sizeof(nt::BOOLEAN)}}},
        {"ZwProtectVirtualMemory", 5, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt::PSIZE_T)}, {"WIN32_PROTECTION_MASK", "NewProtectWin32", sizeof(nt::WIN32_PROTECTION_MASK)}, {"PULONG", "OldProtect", sizeof(nt::PULONG)}}},
        {"ZwPulseEvent", 2, {{"HANDLE", "EventHandle", sizeof(nt::HANDLE)}, {"PLONG", "PreviousState", sizeof(nt::PLONG)}}},
        {"ZwQueryAttributesFile", 2, {{"POBJECT_ATTRIBUTES", "ObjectAttributes", sizeof(nt::POBJECT_ATTRIBUTES)}, {"PFILE_BASIC_INFORMATION", "FileInformation", sizeof(nt::PFILE_BASIC_INFORMATION)}}},
        {"ZwQueryBootEntryOrder", 2, {{"PULONG", "Ids", sizeof(nt::PULONG)}, {"PULONG", "Count", sizeof(nt::PULONG)}}},
        {"ZwQueryDebugFilterState", 2, {{"ULONG", "ComponentId", sizeof(nt::ULONG)}, {"ULONG", "Level", sizeof(nt::ULONG)}}},
        {"ZwQueryDefaultUILanguage", 1, {{"LANGID", "STARDefaultUILanguageId", sizeof(nt::LANGID)}}},
        {"ZwQueryDirectoryObject", 7, {{"HANDLE", "DirectoryHandle", sizeof(nt::HANDLE)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt::BOOLEAN)}, {"BOOLEAN", "RestartScan", sizeof(nt::BOOLEAN)}, {"PULONG", "Context", sizeof(nt::PULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwQueryEaFile", 9, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"BOOLEAN", "ReturnSingleEntry", sizeof(nt::BOOLEAN)}, {"PVOID", "EaList", sizeof(nt::PVOID)}, {"ULONG", "EaListLength", sizeof(nt::ULONG)}, {"PULONG", "EaIndex", sizeof(nt::PULONG)}, {"BOOLEAN", "RestartScan", sizeof(nt::BOOLEAN)}}},
        {"ZwQueryInformationThread", 5, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"THREADINFOCLASS", "ThreadInformationClass", sizeof(nt::THREADINFOCLASS)}, {"PVOID", "ThreadInformation", sizeof(nt::PVOID)}, {"ULONG", "ThreadInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwQueryInformationTransaction", 5, {{"HANDLE", "TransactionHandle", sizeof(nt::HANDLE)}, {"TRANSACTION_INFORMATION_CLASS", "TransactionInformationClass", sizeof(nt::TRANSACTION_INFORMATION_CLASS)}, {"PVOID", "TransactionInformation", sizeof(nt::PVOID)}, {"ULONG", "TransactionInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwQueryInformationWorkerFactory", 5, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt::HANDLE)}, {"WORKERFACTORYINFOCLASS", "WorkerFactoryInformationClass", sizeof(nt::WORKERFACTORYINFOCLASS)}, {"PVOID", "WorkerFactoryInformation", sizeof(nt::PVOID)}, {"ULONG", "WorkerFactoryInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwQueryKey", 5, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"KEY_INFORMATION_CLASS", "KeyInformationClass", sizeof(nt::KEY_INFORMATION_CLASS)}, {"PVOID", "KeyInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PULONG", "ResultLength", sizeof(nt::PULONG)}}},
        {"ZwQueryLicenseValue", 5, {{"PUNICODE_STRING", "Name", sizeof(nt::PUNICODE_STRING)}, {"PULONG", "Type", sizeof(nt::PULONG)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PULONG", "ReturnedLength", sizeof(nt::PULONG)}}},
        {"ZwQueryMultipleValueKey", 6, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"PKEY_VALUE_ENTRY", "ValueEntries", sizeof(nt::PKEY_VALUE_ENTRY)}, {"ULONG", "EntryCount", sizeof(nt::ULONG)}, {"PVOID", "ValueBuffer", sizeof(nt::PVOID)}, {"PULONG", "BufferLength", sizeof(nt::PULONG)}, {"PULONG", "RequiredBufferLength", sizeof(nt::PULONG)}}},
        {"ZwQueryMutant", 5, {{"HANDLE", "MutantHandle", sizeof(nt::HANDLE)}, {"MUTANT_INFORMATION_CLASS", "MutantInformationClass", sizeof(nt::MUTANT_INFORMATION_CLASS)}, {"PVOID", "MutantInformation", sizeof(nt::PVOID)}, {"ULONG", "MutantInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwQueryObject", 5, {{"HANDLE", "Handle", sizeof(nt::HANDLE)}, {"OBJECT_INFORMATION_CLASS", "ObjectInformationClass", sizeof(nt::OBJECT_INFORMATION_CLASS)}, {"PVOID", "ObjectInformation", sizeof(nt::PVOID)}, {"ULONG", "ObjectInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwQuerySecurityAttributesToken", 6, {{"HANDLE", "TokenHandle", sizeof(nt::HANDLE)}, {"PUNICODE_STRING", "Attributes", sizeof(nt::PUNICODE_STRING)}, {"ULONG", "NumberOfAttributes", sizeof(nt::ULONG)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwQuerySemaphore", 5, {{"HANDLE", "SemaphoreHandle", sizeof(nt::HANDLE)}, {"SEMAPHORE_INFORMATION_CLASS", "SemaphoreInformationClass", sizeof(nt::SEMAPHORE_INFORMATION_CLASS)}, {"PVOID", "SemaphoreInformation", sizeof(nt::PVOID)}, {"ULONG", "SemaphoreInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwQuerySystemTime", 1, {{"PLARGE_INTEGER", "SystemTime", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwQueryTimer", 5, {{"HANDLE", "TimerHandle", sizeof(nt::HANDLE)}, {"TIMER_INFORMATION_CLASS", "TimerInformationClass", sizeof(nt::TIMER_INFORMATION_CLASS)}, {"PVOID", "TimerInformation", sizeof(nt::PVOID)}, {"ULONG", "TimerInformationLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwQueryTimerResolution", 3, {{"PULONG", "MaximumTime", sizeof(nt::PULONG)}, {"PULONG", "MinimumTime", sizeof(nt::PULONG)}, {"PULONG", "CurrentTime", sizeof(nt::PULONG)}}},
        {"ZwQueryVirtualMemory", 6, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}, {"MEMORY_INFORMATION_CLASS", "MemoryInformationClass", sizeof(nt::MEMORY_INFORMATION_CLASS)}, {"PVOID", "MemoryInformation", sizeof(nt::PVOID)}, {"SIZE_T", "MemoryInformationLength", sizeof(nt::SIZE_T)}, {"PSIZE_T", "ReturnLength", sizeof(nt::PSIZE_T)}}},
        {"ZwQueryVolumeInformationFile", 5, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "FsInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"FS_INFORMATION_CLASS", "FsInformationClass", sizeof(nt::FS_INFORMATION_CLASS)}}},
        {"ZwRaiseException", 3, {{"PEXCEPTION_RECORD", "ExceptionRecord", sizeof(nt::PEXCEPTION_RECORD)}, {"PCONTEXT", "ContextRecord", sizeof(nt::PCONTEXT)}, {"BOOLEAN", "FirstChance", sizeof(nt::BOOLEAN)}}},
        {"ZwReadFileScatter", 9, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PFILE_SEGMENT_ELEMENT", "SegmentArray", sizeof(nt::PFILE_SEGMENT_ELEMENT)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt::PULONG)}}},
        {"ZwReadOnlyEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwReadVirtualMemory", 5, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt::SIZE_T)}, {"PSIZE_T", "NumberOfBytesRead", sizeof(nt::PSIZE_T)}}},
        {"ZwRecoverEnlistment", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PVOID", "EnlistmentKey", sizeof(nt::PVOID)}}},
        {"ZwRecoverResourceManager", 1, {{"HANDLE", "ResourceManagerHandle", sizeof(nt::HANDLE)}}},
        {"ZwReleaseMutant", 2, {{"HANDLE", "MutantHandle", sizeof(nt::HANDLE)}, {"PLONG", "PreviousCount", sizeof(nt::PLONG)}}},
        {"ZwReleaseSemaphore", 3, {{"HANDLE", "SemaphoreHandle", sizeof(nt::HANDLE)}, {"LONG", "ReleaseCount", sizeof(nt::LONG)}, {"PLONG", "PreviousCount", sizeof(nt::PLONG)}}},
        {"ZwRemoveIoCompletion", 5, {{"HANDLE", "IoCompletionHandle", sizeof(nt::HANDLE)}, {"PVOID", "STARKeyContext", sizeof(nt::PVOID)}, {"PVOID", "STARApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwRemoveIoCompletionEx", 6, {{"HANDLE", "IoCompletionHandle", sizeof(nt::HANDLE)}, {"PFILE_IO_COMPLETION_INFORMATION", "IoCompletionInformation", sizeof(nt::PFILE_IO_COMPLETION_INFORMATION)}, {"ULONG", "Count", sizeof(nt::ULONG)}, {"PULONG", "NumEntriesRemoved", sizeof(nt::PULONG)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}, {"BOOLEAN", "Alertable", sizeof(nt::BOOLEAN)}}},
        {"ZwRemoveProcessDebug", 2, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"HANDLE", "DebugObjectHandle", sizeof(nt::HANDLE)}}},
        {"ZwReplyWaitReceivePort", 4, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PVOID", "STARPortContext", sizeof(nt::PVOID)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(nt::PPORT_MESSAGE)}}},
        {"ZwReplyWaitReceivePortEx", 5, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PVOID", "STARPortContext", sizeof(nt::PVOID)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReceiveMessage", sizeof(nt::PPORT_MESSAGE)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwRequestWaitReplyPort", 3, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "RequestMessage", sizeof(nt::PPORT_MESSAGE)}, {"PPORT_MESSAGE", "ReplyMessage", sizeof(nt::PPORT_MESSAGE)}}},
        {"ZwResetEvent", 2, {{"HANDLE", "EventHandle", sizeof(nt::HANDLE)}, {"PLONG", "PreviousState", sizeof(nt::PLONG)}}},
        {"ZwResetWriteWatch", 3, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "BaseAddress", sizeof(nt::PVOID)}, {"SIZE_T", "RegionSize", sizeof(nt::SIZE_T)}}},
        {"ZwResumeProcess", 1, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}}},
        {"ZwResumeThread", 2, {{"HANDLE", "ThreadHandle", sizeof(nt::HANDLE)}, {"PULONG", "PreviousSuspendCount", sizeof(nt::PULONG)}}},
        {"ZwRollbackComplete", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwRollbackTransaction", 2, {{"HANDLE", "TransactionHandle", sizeof(nt::HANDLE)}, {"BOOLEAN", "Wait", sizeof(nt::BOOLEAN)}}},
        {"ZwSaveMergedKeys", 3, {{"HANDLE", "HighPrecedenceKeyHandle", sizeof(nt::HANDLE)}, {"HANDLE", "LowPrecedenceKeyHandle", sizeof(nt::HANDLE)}, {"HANDLE", "FileHandle", sizeof(nt::HANDLE)}}},
        {"ZwSerializeBoot", 0, {}},
        {"ZwSetBootEntryOrder", 2, {{"PULONG", "Ids", sizeof(nt::PULONG)}, {"ULONG", "Count", sizeof(nt::ULONG)}}},
        {"ZwSetDebugFilterState", 3, {{"ULONG", "ComponentId", sizeof(nt::ULONG)}, {"ULONG", "Level", sizeof(nt::ULONG)}, {"BOOLEAN", "State", sizeof(nt::BOOLEAN)}}},
        {"ZwSetDefaultUILanguage", 1, {{"LANGID", "DefaultUILanguageId", sizeof(nt::LANGID)}}},
        {"ZwSetEaFile", 4, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}}},
        {"ZwSetEvent", 2, {{"HANDLE", "EventHandle", sizeof(nt::HANDLE)}, {"PLONG", "PreviousState", sizeof(nt::PLONG)}}},
        {"ZwSetEventBoostPriority", 1, {{"HANDLE", "EventHandle", sizeof(nt::HANDLE)}}},
        {"ZwSetHighWaitLowEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt::HANDLE)}}},
        {"ZwSetInformationFile", 5, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "FileInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"FILE_INFORMATION_CLASS", "FileInformationClass", sizeof(nt::FILE_INFORMATION_CLASS)}}},
        {"ZwSetInformationJobObject", 4, {{"HANDLE", "JobHandle", sizeof(nt::HANDLE)}, {"JOBOBJECTINFOCLASS", "JobObjectInformationClass", sizeof(nt::JOBOBJECTINFOCLASS)}, {"PVOID", "JobObjectInformation", sizeof(nt::PVOID)}, {"ULONG", "JobObjectInformationLength", sizeof(nt::ULONG)}}},
        {"ZwSetInformationKey", 4, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"KEY_SET_INFORMATION_CLASS", "KeySetInformationClass", sizeof(nt::KEY_SET_INFORMATION_CLASS)}, {"PVOID", "KeySetInformation", sizeof(nt::PVOID)}, {"ULONG", "KeySetInformationLength", sizeof(nt::ULONG)}}},
        {"ZwSetInformationTransactionManager", 4, {{"HANDLE", "TmHandle", sizeof(nt::HANDLE)}, {"TRANSACTIONMANAGER_INFORMATION_CLASS", "TransactionManagerInformationClass", sizeof(nt::TRANSACTIONMANAGER_INFORMATION_CLASS)}, {"PVOID", "TransactionManagerInformation", sizeof(nt::PVOID)}, {"ULONG", "TransactionManagerInformationLength", sizeof(nt::ULONG)}}},
        {"ZwSetIoCompletion", 5, {{"HANDLE", "IoCompletionHandle", sizeof(nt::HANDLE)}, {"PVOID", "KeyContext", sizeof(nt::PVOID)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"NTSTATUS", "IoStatus", sizeof(nt::NTSTATUS)}, {"ULONG_PTR", "IoStatusInformation", sizeof(nt::ULONG_PTR)}}},
        {"ZwSetIoCompletionEx", 6, {{"HANDLE", "IoCompletionHandle", sizeof(nt::HANDLE)}, {"HANDLE", "IoCompletionReserveHandle", sizeof(nt::HANDLE)}, {"PVOID", "KeyContext", sizeof(nt::PVOID)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"NTSTATUS", "IoStatus", sizeof(nt::NTSTATUS)}, {"ULONG_PTR", "IoStatusInformation", sizeof(nt::ULONG_PTR)}}},
        {"ZwSetLdtEntries", 6, {{"ULONG", "Selector0", sizeof(nt::ULONG)}, {"ULONG", "Entry0Low", sizeof(nt::ULONG)}, {"ULONG", "Entry0Hi", sizeof(nt::ULONG)}, {"ULONG", "Selector1", sizeof(nt::ULONG)}, {"ULONG", "Entry1Low", sizeof(nt::ULONG)}, {"ULONG", "Entry1Hi", sizeof(nt::ULONG)}}},
        {"ZwSetLowEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt::HANDLE)}}},
        {"ZwSetLowWaitHighEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt::HANDLE)}}},
        {"ZwSetQuotaInformationFile", 4, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}}},
        {"ZwSetSystemEnvironmentValue", 2, {{"PUNICODE_STRING", "VariableName", sizeof(nt::PUNICODE_STRING)}, {"PUNICODE_STRING", "VariableValue", sizeof(nt::PUNICODE_STRING)}}},
        {"ZwSetSystemEnvironmentValueEx", 5, {{"PUNICODE_STRING", "VariableName", sizeof(nt::PUNICODE_STRING)}, {"LPGUID", "VendorGuid", sizeof(nt::LPGUID)}, {"PVOID", "Value", sizeof(nt::PVOID)}, {"ULONG", "ValueLength", sizeof(nt::ULONG)}, {"ULONG", "Attributes", sizeof(nt::ULONG)}}},
        {"ZwSetSystemInformation", 3, {{"SYSTEM_INFORMATION_CLASS", "SystemInformationClass", sizeof(nt::SYSTEM_INFORMATION_CLASS)}, {"PVOID", "SystemInformation", sizeof(nt::PVOID)}, {"ULONG", "SystemInformationLength", sizeof(nt::ULONG)}}},
        {"ZwSetSystemPowerState", 3, {{"POWER_ACTION", "SystemAction", sizeof(nt::POWER_ACTION)}, {"SYSTEM_POWER_STATE", "MinSystemState", sizeof(nt::SYSTEM_POWER_STATE)}, {"ULONG", "Flags", sizeof(nt::ULONG)}}},
        {"ZwSetSystemTime", 2, {{"PLARGE_INTEGER", "SystemTime", sizeof(nt::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "PreviousTime", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwSetTimer", 7, {{"HANDLE", "TimerHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "DueTime", sizeof(nt::PLARGE_INTEGER)}, {"PTIMER_APC_ROUTINE", "TimerApcRoutine", sizeof(nt::PTIMER_APC_ROUTINE)}, {"PVOID", "TimerContext", sizeof(nt::PVOID)}, {"BOOLEAN", "WakeTimer", sizeof(nt::BOOLEAN)}, {"LONG", "Period", sizeof(nt::LONG)}, {"PBOOLEAN", "PreviousState", sizeof(nt::PBOOLEAN)}}},
        {"ZwSetTimerEx", 4, {{"HANDLE", "TimerHandle", sizeof(nt::HANDLE)}, {"TIMER_SET_INFORMATION_CLASS", "TimerSetInformationClass", sizeof(nt::TIMER_SET_INFORMATION_CLASS)}, {"PVOID", "TimerSetInformation", sizeof(nt::PVOID)}, {"ULONG", "TimerSetInformationLength", sizeof(nt::ULONG)}}},
        {"ZwSetValueKey", 6, {{"HANDLE", "KeyHandle", sizeof(nt::HANDLE)}, {"PUNICODE_STRING", "ValueName", sizeof(nt::PUNICODE_STRING)}, {"ULONG", "TitleIndex", sizeof(nt::ULONG)}, {"ULONG", "Type", sizeof(nt::ULONG)}, {"PVOID", "Data", sizeof(nt::PVOID)}, {"ULONG", "DataSize", sizeof(nt::ULONG)}}},
        {"ZwSetVolumeInformationFile", 5, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PVOID", "FsInformation", sizeof(nt::PVOID)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"FS_INFORMATION_CLASS", "FsInformationClass", sizeof(nt::FS_INFORMATION_CLASS)}}},
        {"ZwShutdownSystem", 1, {{"SHUTDOWN_ACTION", "Action", sizeof(nt::SHUTDOWN_ACTION)}}},
        {"ZwShutdownWorkerFactory", 2, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt::HANDLE)}, {"LONG", "STARPendingWorkerCount", sizeof(nt::LONG)}}},
        {"ZwSignalAndWaitForSingleObject", 4, {{"HANDLE", "SignalHandle", sizeof(nt::HANDLE)}, {"HANDLE", "WaitHandle", sizeof(nt::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(nt::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwSinglePhaseReject", 2, {{"HANDLE", "EnlistmentHandle", sizeof(nt::HANDLE)}, {"PLARGE_INTEGER", "TmVirtualClock", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwStartProfile", 1, {{"HANDLE", "ProfileHandle", sizeof(nt::HANDLE)}}},
        {"ZwStopProfile", 1, {{"HANDLE", "ProfileHandle", sizeof(nt::HANDLE)}}},
        {"ZwSuspendProcess", 1, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}}},
        {"ZwSystemDebugControl", 6, {{"SYSDBG_COMMAND", "Command", sizeof(nt::SYSDBG_COMMAND)}, {"PVOID", "InputBuffer", sizeof(nt::PVOID)}, {"ULONG", "InputBufferLength", sizeof(nt::ULONG)}, {"PVOID", "OutputBuffer", sizeof(nt::PVOID)}, {"ULONG", "OutputBufferLength", sizeof(nt::ULONG)}, {"PULONG", "ReturnLength", sizeof(nt::PULONG)}}},
        {"ZwTerminateProcess", 2, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"NTSTATUS", "ExitStatus", sizeof(nt::NTSTATUS)}}},
        {"ZwTestAlert", 0, {}},
        {"ZwThawRegistry", 0, {}},
        {"ZwThawTransactions", 0, {}},
        {"ZwUmsThreadYield", 0, {}},
        {"ZwUnloadDriver", 1, {{"PUNICODE_STRING", "DriverServiceName", sizeof(nt::PUNICODE_STRING)}}},
        {"ZwUnloadKeyEx", 2, {{"POBJECT_ATTRIBUTES", "TargetKey", sizeof(nt::POBJECT_ATTRIBUTES)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}}},
        {"ZwUnlockFile", 5, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt::PLARGE_INTEGER)}, {"PLARGE_INTEGER", "Length", sizeof(nt::PLARGE_INTEGER)}, {"ULONG", "Key", sizeof(nt::ULONG)}}},
        {"ZwUnlockVirtualMemory", 4, {{"HANDLE", "ProcessHandle", sizeof(nt::HANDLE)}, {"PVOID", "STARBaseAddress", sizeof(nt::PVOID)}, {"PSIZE_T", "RegionSize", sizeof(nt::PSIZE_T)}, {"ULONG", "MapType", sizeof(nt::ULONG)}}},
        {"ZwVdmControl", 2, {{"VDMSERVICECLASS", "Service", sizeof(nt::VDMSERVICECLASS)}, {"PVOID", "ServiceData", sizeof(nt::PVOID)}}},
        {"ZwWaitForDebugEvent", 4, {{"HANDLE", "DebugObjectHandle", sizeof(nt::HANDLE)}, {"BOOLEAN", "Alertable", sizeof(nt::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}, {"PDBGUI_WAIT_STATE_CHANGE", "WaitStateChange", sizeof(nt::PDBGUI_WAIT_STATE_CHANGE)}}},
        {"ZwWaitForMultipleObjects32", 5, {{"ULONG", "Count", sizeof(nt::ULONG)}, {"LONG", "Handles", sizeof(nt::LONG)}, {"WAIT_TYPE", "WaitType", sizeof(nt::WAIT_TYPE)}, {"BOOLEAN", "Alertable", sizeof(nt::BOOLEAN)}, {"PLARGE_INTEGER", "Timeout", sizeof(nt::PLARGE_INTEGER)}}},
        {"ZwWaitForWorkViaWorkerFactory", 2, {{"HANDLE", "WorkerFactoryHandle", sizeof(nt::HANDLE)}, {"PFILE_IO_COMPLETION_INFORMATION", "MiniPacket", sizeof(nt::PFILE_IO_COMPLETION_INFORMATION)}}},
        {"ZwWaitHighEventPair", 1, {{"HANDLE", "EventPairHandle", sizeof(nt::HANDLE)}}},
        {"ZwWriteFileGather", 9, {{"HANDLE", "FileHandle", sizeof(nt::HANDLE)}, {"HANDLE", "Event", sizeof(nt::HANDLE)}, {"PIO_APC_ROUTINE", "ApcRoutine", sizeof(nt::PIO_APC_ROUTINE)}, {"PVOID", "ApcContext", sizeof(nt::PVOID)}, {"PIO_STATUS_BLOCK", "IoStatusBlock", sizeof(nt::PIO_STATUS_BLOCK)}, {"PFILE_SEGMENT_ELEMENT", "SegmentArray", sizeof(nt::PFILE_SEGMENT_ELEMENT)}, {"ULONG", "Length", sizeof(nt::ULONG)}, {"PLARGE_INTEGER", "ByteOffset", sizeof(nt::PLARGE_INTEGER)}, {"PULONG", "Key", sizeof(nt::PULONG)}}},
        {"ZwWriteRequestData", 6, {{"HANDLE", "PortHandle", sizeof(nt::HANDLE)}, {"PPORT_MESSAGE", "Message", sizeof(nt::PPORT_MESSAGE)}, {"ULONG", "DataEntryIndex", sizeof(nt::ULONG)}, {"PVOID", "Buffer", sizeof(nt::PVOID)}, {"SIZE_T", "BufferSize", sizeof(nt::SIZE_T)}, {"PSIZE_T", "NumberOfBytesWritten", sizeof(nt::PSIZE_T)}}},
        {"ZwYieldExecution", 0, {}},
    }};
}

struct nt::syscalls::Data
{
    Data(core::Core& core, std::string_view module);

    core::Core&   core;
    std::string   module;
};

nt::syscalls::Data::Data(core::Core& core, std::string_view module)
    : core(core)
    , module(module)
{
}

nt::syscalls::syscalls(core::Core& core, std::string_view module)
    : d_(std::make_unique<Data>(core, module))
{
}

nt::syscalls::~syscalls() = default;

const nt::syscalls::callcfgs_t& nt::syscalls::callcfgs()
{
    return g_callcfgs;
}

namespace
{
    opt<bpid_t> register_callback_with(nt::syscalls::Data& d, bpid_t bpid, proc_t proc, const char* name, const state::Task& on_call)
    {
        const auto addr = symbols::address(d.core, proc, d.module, name);
        if(!addr)
            return FAIL(std::nullopt, "unable to find symbole %s!%s", d.module.data(), name);

        const auto bp = state::break_on_process(d.core, name, proc, *addr, on_call);
        if(!bp)
            return FAIL(std::nullopt, "unable to set breakpoint");

        return state::save_breakpoint_with(d.core, bpid, bp);
    }

    opt<bpid_t> register_callback(nt::syscalls::Data& d, proc_t proc, const char* name, const state::Task& on_call)
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

opt<bpid_t> nt::syscalls::register_NtAcceptConnectPort(proc_t proc, const on_NtAcceptConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtAcceptConnectPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle        = arg<nt::PHANDLE>(core, 0);
        const auto PortContext       = arg<nt::PVOID>(core, 1);
        const auto ConnectionRequest = arg<nt::PPORT_MESSAGE>(core, 2);
        const auto AcceptConnection  = arg<nt::BOOLEAN>(core, 3);
        const auto ServerView        = arg<nt::PPORT_VIEW>(core, 4);
        const auto ClientView        = arg<nt::PREMOTE_PORT_VIEW>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[0]);

        on_func(PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView);
    });
}

opt<bpid_t> nt::syscalls::register_NtAddDriverEntry(proc_t proc, const on_NtAddDriverEntry_fn& on_func)
{
    return register_callback(*d_, proc, "NtAddDriverEntry", [=]
    {
        auto& core = d_->core;

        const auto DriverEntry = arg<nt::PEFI_DRIVER_ENTRY>(core, 0);
        const auto Id          = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[1]);

        on_func(DriverEntry, Id);
    });
}

opt<bpid_t> nt::syscalls::register_NtAdjustGroupsToken(proc_t proc, const on_NtAdjustGroupsToken_fn& on_func)
{
    return register_callback(*d_, proc, "NtAdjustGroupsToken", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle    = arg<nt::HANDLE>(core, 0);
        const auto ResetToDefault = arg<nt::BOOLEAN>(core, 1);
        const auto NewState       = arg<nt::PTOKEN_GROUPS>(core, 2);
        const auto BufferLength   = arg<nt::ULONG>(core, 3);
        const auto PreviousState  = arg<nt::PTOKEN_GROUPS>(core, 4);
        const auto ReturnLength   = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[2]);

        on_func(TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtAdjustPrivilegesToken(proc_t proc, const on_NtAdjustPrivilegesToken_fn& on_func)
{
    return register_callback(*d_, proc, "NtAdjustPrivilegesToken", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle          = arg<nt::HANDLE>(core, 0);
        const auto DisableAllPrivileges = arg<nt::BOOLEAN>(core, 1);
        const auto NewState             = arg<nt::PTOKEN_PRIVILEGES>(core, 2);
        const auto BufferLength         = arg<nt::ULONG>(core, 3);
        const auto PreviousState        = arg<nt::PTOKEN_PRIVILEGES>(core, 4);
        const auto ReturnLength         = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[3]);

        on_func(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlertThread(proc_t proc, const on_NtAlertThread_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlertThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[4]);

        on_func(ThreadHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtAllocateReserveObject(proc_t proc, const on_NtAllocateReserveObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtAllocateReserveObject", [=]
    {
        auto& core = d_->core;

        const auto MemoryReserveHandle = arg<nt::PHANDLE>(core, 0);
        const auto ObjectAttributes    = arg<nt::POBJECT_ATTRIBUTES>(core, 1);
        const auto Type                = arg<nt::MEMORY_RESERVE_TYPE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[5]);

        on_func(MemoryReserveHandle, ObjectAttributes, Type);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcCancelMessage(proc_t proc, const on_NtAlpcCancelMessage_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcCancelMessage", [=]
    {
        auto& core = d_->core;

        const auto PortHandle     = arg<nt::HANDLE>(core, 0);
        const auto Flags          = arg<nt::ULONG>(core, 1);
        const auto MessageContext = arg<nt::PALPC_CONTEXT_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[6]);

        on_func(PortHandle, Flags, MessageContext);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcConnectPort(proc_t proc, const on_NtAlpcConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcConnectPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle           = arg<nt::PHANDLE>(core, 0);
        const auto PortName             = arg<nt::PUNICODE_STRING>(core, 1);
        const auto ObjectAttributes     = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto PortAttributes       = arg<nt::PALPC_PORT_ATTRIBUTES>(core, 3);
        const auto Flags                = arg<nt::ULONG>(core, 4);
        const auto RequiredServerSid    = arg<nt::PSID>(core, 5);
        const auto ConnectionMessage    = arg<nt::PPORT_MESSAGE>(core, 6);
        const auto BufferLength         = arg<nt::PULONG>(core, 7);
        const auto OutMessageAttributes = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(core, 8);
        const auto InMessageAttributes  = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(core, 9);
        const auto Timeout              = arg<nt::PLARGE_INTEGER>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[7]);

        on_func(PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcCreatePort(proc_t proc, const on_NtAlpcCreatePort_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcCreatePort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle       = arg<nt::PHANDLE>(core, 0);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 1);
        const auto PortAttributes   = arg<nt::PALPC_PORT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[8]);

        on_func(PortHandle, ObjectAttributes, PortAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcCreateSectionView(proc_t proc, const on_NtAlpcCreateSectionView_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcCreateSectionView", [=]
    {
        auto& core = d_->core;

        const auto PortHandle     = arg<nt::HANDLE>(core, 0);
        const auto Flags          = arg<nt::ULONG>(core, 1);
        const auto ViewAttributes = arg<nt::PALPC_DATA_VIEW_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[9]);

        on_func(PortHandle, Flags, ViewAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcCreateSecurityContext(proc_t proc, const on_NtAlpcCreateSecurityContext_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcCreateSecurityContext", [=]
    {
        auto& core = d_->core;

        const auto PortHandle        = arg<nt::HANDLE>(core, 0);
        const auto Flags             = arg<nt::ULONG>(core, 1);
        const auto SecurityAttribute = arg<nt::PALPC_SECURITY_ATTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[10]);

        on_func(PortHandle, Flags, SecurityAttribute);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcDeletePortSection(proc_t proc, const on_NtAlpcDeletePortSection_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcDeletePortSection", [=]
    {
        auto& core = d_->core;

        const auto PortHandle    = arg<nt::HANDLE>(core, 0);
        const auto Flags         = arg<nt::ULONG>(core, 1);
        const auto SectionHandle = arg<nt::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[11]);

        on_func(PortHandle, Flags, SectionHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcDeleteResourceReserve(proc_t proc, const on_NtAlpcDeleteResourceReserve_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcDeleteResourceReserve", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<nt::HANDLE>(core, 0);
        const auto Flags      = arg<nt::ULONG>(core, 1);
        const auto ResourceId = arg<nt::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[12]);

        on_func(PortHandle, Flags, ResourceId);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcDisconnectPort(proc_t proc, const on_NtAlpcDisconnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcDisconnectPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<nt::HANDLE>(core, 0);
        const auto Flags      = arg<nt::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[13]);

        on_func(PortHandle, Flags);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcImpersonateClientOfPort(proc_t proc, const on_NtAlpcImpersonateClientOfPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcImpersonateClientOfPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle  = arg<nt::HANDLE>(core, 0);
        const auto PortMessage = arg<nt::PPORT_MESSAGE>(core, 1);
        const auto Reserved    = arg<nt::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[14]);

        on_func(PortHandle, PortMessage, Reserved);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcOpenSenderProcess(proc_t proc, const on_NtAlpcOpenSenderProcess_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcOpenSenderProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<nt::PHANDLE>(core, 0);
        const auto PortHandle       = arg<nt::HANDLE>(core, 1);
        const auto PortMessage      = arg<nt::PPORT_MESSAGE>(core, 2);
        const auto Flags            = arg<nt::ULONG>(core, 3);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 4);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[15]);

        on_func(ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcOpenSenderThread(proc_t proc, const on_NtAlpcOpenSenderThread_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcOpenSenderThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle     = arg<nt::PHANDLE>(core, 0);
        const auto PortHandle       = arg<nt::HANDLE>(core, 1);
        const auto PortMessage      = arg<nt::PPORT_MESSAGE>(core, 2);
        const auto Flags            = arg<nt::ULONG>(core, 3);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 4);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[16]);

        on_func(ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcRevokeSecurityContext(proc_t proc, const on_NtAlpcRevokeSecurityContext_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcRevokeSecurityContext", [=]
    {
        auto& core = d_->core;

        const auto PortHandle    = arg<nt::HANDLE>(core, 0);
        const auto Flags         = arg<nt::ULONG>(core, 1);
        const auto ContextHandle = arg<nt::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[17]);

        on_func(PortHandle, Flags, ContextHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtAlpcSetInformation(proc_t proc, const on_NtAlpcSetInformation_fn& on_func)
{
    return register_callback(*d_, proc, "NtAlpcSetInformation", [=]
    {
        auto& core = d_->core;

        const auto PortHandle           = arg<nt::HANDLE>(core, 0);
        const auto PortInformationClass = arg<nt::ALPC_PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<nt::PVOID>(core, 2);
        const auto Length               = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[18]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length);
    });
}

opt<bpid_t> nt::syscalls::register_NtApphelpCacheControl(proc_t proc, const on_NtApphelpCacheControl_fn& on_func)
{
    return register_callback(*d_, proc, "NtApphelpCacheControl", [=]
    {
        auto& core = d_->core;

        const auto type = arg<nt::APPHELPCOMMAND>(core, 0);
        const auto buf  = arg<nt::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[19]);

        on_func(type, buf);
    });
}

opt<bpid_t> nt::syscalls::register_NtAssignProcessToJobObject(proc_t proc, const on_NtAssignProcessToJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtAssignProcessToJobObject", [=]
    {
        auto& core = d_->core;

        const auto JobHandle     = arg<nt::HANDLE>(core, 0);
        const auto ProcessHandle = arg<nt::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[20]);

        on_func(JobHandle, ProcessHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtCancelTimer(proc_t proc, const on_NtCancelTimer_fn& on_func)
{
    return register_callback(*d_, proc, "NtCancelTimer", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle  = arg<nt::HANDLE>(core, 0);
        const auto CurrentState = arg<nt::PBOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[21]);

        on_func(TimerHandle, CurrentState);
    });
}

opt<bpid_t> nt::syscalls::register_NtClearEvent(proc_t proc, const on_NtClearEvent_fn& on_func)
{
    return register_callback(*d_, proc, "NtClearEvent", [=]
    {
        auto& core = d_->core;

        const auto EventHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[22]);

        on_func(EventHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtClose(proc_t proc, const on_NtClose_fn& on_func)
{
    return register_callback(*d_, proc, "NtClose", [=]
    {
        auto& core = d_->core;

        const auto Handle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[23]);

        on_func(Handle);
    });
}

opt<bpid_t> nt::syscalls::register_NtCommitComplete(proc_t proc, const on_NtCommitComplete_fn& on_func)
{
    return register_callback(*d_, proc, "NtCommitComplete", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[24]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_NtCompactKeys(proc_t proc, const on_NtCompactKeys_fn& on_func)
{
    return register_callback(*d_, proc, "NtCompactKeys", [=]
    {
        auto& core = d_->core;

        const auto Count    = arg<nt::ULONG>(core, 0);
        const auto KeyArray = arg<nt::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[25]);

        on_func(Count, KeyArray);
    });
}

opt<bpid_t> nt::syscalls::register_NtCompleteConnectPort(proc_t proc, const on_NtCompleteConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtCompleteConnectPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[26]);

        on_func(PortHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtCompressKey(proc_t proc, const on_NtCompressKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtCompressKey", [=]
    {
        auto& core = d_->core;

        const auto Key = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[27]);

        on_func(Key);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateDebugObject(proc_t proc, const on_NtCreateDebugObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateDebugObject", [=]
    {
        auto& core = d_->core;

        const auto DebugObjectHandle = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto Flags             = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[28]);

        on_func(DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateDirectoryObject(proc_t proc, const on_NtCreateDirectoryObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateDirectoryObject", [=]
    {
        auto& core = d_->core;

        const auto DirectoryHandle  = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[29]);

        on_func(DirectoryHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateEnlistment(proc_t proc, const on_NtCreateEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateEnlistment", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle      = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt::ACCESS_MASK>(core, 1);
        const auto ResourceManagerHandle = arg<nt::HANDLE>(core, 2);
        const auto TransactionHandle     = arg<nt::HANDLE>(core, 3);
        const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(core, 4);
        const auto CreateOptions         = arg<nt::ULONG>(core, 5);
        const auto NotificationMask      = arg<nt::NOTIFICATION_MASK>(core, 6);
        const auto EnlistmentKey         = arg<nt::PVOID>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[30]);

        on_func(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateIoCompletion(proc_t proc, const on_NtCreateIoCompletion_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateIoCompletion", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto Count              = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[31]);

        on_func(IoCompletionHandle, DesiredAccess, ObjectAttributes, Count);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateMutant(proc_t proc, const on_NtCreateMutant_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateMutant", [=]
    {
        auto& core = d_->core;

        const auto MutantHandle     = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto InitialOwner     = arg<nt::BOOLEAN>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[32]);

        on_func(MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateNamedPipeFile(proc_t proc, const on_NtCreateNamedPipeFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateNamedPipeFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle        = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt::ULONG>(core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core, 3);
        const auto ShareAccess       = arg<nt::ULONG>(core, 4);
        const auto CreateDisposition = arg<nt::ULONG>(core, 5);
        const auto CreateOptions     = arg<nt::ULONG>(core, 6);
        const auto NamedPipeType     = arg<nt::ULONG>(core, 7);
        const auto ReadMode          = arg<nt::ULONG>(core, 8);
        const auto CompletionMode    = arg<nt::ULONG>(core, 9);
        const auto MaximumInstances  = arg<nt::ULONG>(core, 10);
        const auto InboundQuota      = arg<nt::ULONG>(core, 11);
        const auto OutboundQuota     = arg<nt::ULONG>(core, 12);
        const auto DefaultTimeout    = arg<nt::PLARGE_INTEGER>(core, 13);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[33]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreatePagingFile(proc_t proc, const on_NtCreatePagingFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreatePagingFile", [=]
    {
        auto& core = d_->core;

        const auto PageFileName = arg<nt::PUNICODE_STRING>(core, 0);
        const auto MinimumSize  = arg<nt::PLARGE_INTEGER>(core, 1);
        const auto MaximumSize  = arg<nt::PLARGE_INTEGER>(core, 2);
        const auto Priority     = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[34]);

        on_func(PageFileName, MinimumSize, MaximumSize, Priority);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateProcess(proc_t proc, const on_NtCreateProcess_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle      = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto ParentProcess      = arg<nt::HANDLE>(core, 3);
        const auto InheritObjectTable = arg<nt::BOOLEAN>(core, 4);
        const auto SectionHandle      = arg<nt::HANDLE>(core, 5);
        const auto DebugPort          = arg<nt::HANDLE>(core, 6);
        const auto ExceptionPort      = arg<nt::HANDLE>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[35]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateResourceManager(proc_t proc, const on_NtCreateResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateResourceManager", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt::ACCESS_MASK>(core, 1);
        const auto TmHandle              = arg<nt::HANDLE>(core, 2);
        const auto RmGuid                = arg<nt::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(core, 4);
        const auto CreateOptions         = arg<nt::ULONG>(core, 5);
        const auto Description           = arg<nt::PUNICODE_STRING>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[36]);

        on_func(ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateSection(proc_t proc, const on_NtCreateSection_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateSection", [=]
    {
        auto& core = d_->core;

        const auto SectionHandle         = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto MaximumSize           = arg<nt::PLARGE_INTEGER>(core, 3);
        const auto SectionPageProtection = arg<nt::ULONG>(core, 4);
        const auto AllocationAttributes  = arg<nt::ULONG>(core, 5);
        const auto FileHandle            = arg<nt::HANDLE>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[37]);

        on_func(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateSemaphore(proc_t proc, const on_NtCreateSemaphore_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateSemaphore", [=]
    {
        auto& core = d_->core;

        const auto SemaphoreHandle  = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto InitialCount     = arg<nt::LONG>(core, 3);
        const auto MaximumCount     = arg<nt::LONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[38]);

        on_func(SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateThread(proc_t proc, const on_NtCreateThread_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle     = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto ProcessHandle    = arg<nt::HANDLE>(core, 3);
        const auto ClientId         = arg<nt::PCLIENT_ID>(core, 4);
        const auto ThreadContext    = arg<nt::PCONTEXT>(core, 5);
        const auto InitialTeb       = arg<nt::PINITIAL_TEB>(core, 6);
        const auto CreateSuspended  = arg<nt::BOOLEAN>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[39]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateThreadEx(proc_t proc, const on_NtCreateThreadEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateThreadEx", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle     = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto ProcessHandle    = arg<nt::HANDLE>(core, 3);
        const auto StartRoutine     = arg<nt::PVOID>(core, 4);
        const auto Argument         = arg<nt::PVOID>(core, 5);
        const auto CreateFlags      = arg<nt::ULONG>(core, 6);
        const auto ZeroBits         = arg<nt::ULONG_PTR>(core, 7);
        const auto StackSize        = arg<nt::SIZE_T>(core, 8);
        const auto MaximumStackSize = arg<nt::SIZE_T>(core, 9);
        const auto AttributeList    = arg<nt::PPS_ATTRIBUTE_LIST>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[40]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateToken(proc_t proc, const on_NtCreateToken_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateToken", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle      = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto TokenType        = arg<nt::TOKEN_TYPE>(core, 3);
        const auto AuthenticationId = arg<nt::PLUID>(core, 4);
        const auto ExpirationTime   = arg<nt::PLARGE_INTEGER>(core, 5);
        const auto User             = arg<nt::PTOKEN_USER>(core, 6);
        const auto Groups           = arg<nt::PTOKEN_GROUPS>(core, 7);
        const auto Privileges       = arg<nt::PTOKEN_PRIVILEGES>(core, 8);
        const auto Owner            = arg<nt::PTOKEN_OWNER>(core, 9);
        const auto PrimaryGroup     = arg<nt::PTOKEN_PRIMARY_GROUP>(core, 10);
        const auto DefaultDacl      = arg<nt::PTOKEN_DEFAULT_DACL>(core, 11);
        const auto TokenSource      = arg<nt::PTOKEN_SOURCE>(core, 12);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[41]);

        on_func(TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateTransaction(proc_t proc, const on_NtCreateTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateTransaction", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto Uow               = arg<nt::LPGUID>(core, 3);
        const auto TmHandle          = arg<nt::HANDLE>(core, 4);
        const auto CreateOptions     = arg<nt::ULONG>(core, 5);
        const auto IsolationLevel    = arg<nt::ULONG>(core, 6);
        const auto IsolationFlags    = arg<nt::ULONG>(core, 7);
        const auto Timeout           = arg<nt::PLARGE_INTEGER>(core, 8);
        const auto Description       = arg<nt::PUNICODE_STRING>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[42]);

        on_func(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateUserProcess(proc_t proc, const on_NtCreateUserProcess_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateUserProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle           = arg<nt::PHANDLE>(core, 0);
        const auto ThreadHandle            = arg<nt::PHANDLE>(core, 1);
        const auto ProcessDesiredAccess    = arg<nt::ACCESS_MASK>(core, 2);
        const auto ThreadDesiredAccess     = arg<nt::ACCESS_MASK>(core, 3);
        const auto ProcessObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 4);
        const auto ThreadObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core, 5);
        const auto ProcessFlags            = arg<nt::ULONG>(core, 6);
        const auto ThreadFlags             = arg<nt::ULONG>(core, 7);
        const auto ProcessParameters       = arg<nt::PRTL_USER_PROCESS_PARAMETERS>(core, 8);
        const auto CreateInfo              = arg<nt::PPROCESS_CREATE_INFO>(core, 9);
        const auto AttributeList           = arg<nt::PPROCESS_ATTRIBUTE_LIST>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[43]);

        on_func(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateWaitablePort(proc_t proc, const on_NtCreateWaitablePort_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateWaitablePort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle              = arg<nt::PHANDLE>(core, 0);
        const auto ObjectAttributes        = arg<nt::POBJECT_ATTRIBUTES>(core, 1);
        const auto MaxConnectionInfoLength = arg<nt::ULONG>(core, 2);
        const auto MaxMessageLength        = arg<nt::ULONG>(core, 3);
        const auto MaxPoolUsage            = arg<nt::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[44]);

        on_func(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    });
}

opt<bpid_t> nt::syscalls::register_NtCreateWorkerFactory(proc_t proc, const on_NtCreateWorkerFactory_fn& on_func)
{
    return register_callback(*d_, proc, "NtCreateWorkerFactory", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandleReturn = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess             = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes          = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto CompletionPortHandle      = arg<nt::HANDLE>(core, 3);
        const auto WorkerProcessHandle       = arg<nt::HANDLE>(core, 4);
        const auto StartRoutine              = arg<nt::PVOID>(core, 5);
        const auto StartParameter            = arg<nt::PVOID>(core, 6);
        const auto MaxThreadCount            = arg<nt::ULONG>(core, 7);
        const auto StackReserve              = arg<nt::SIZE_T>(core, 8);
        const auto StackCommit               = arg<nt::SIZE_T>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[45]);

        on_func(WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit);
    });
}

opt<bpid_t> nt::syscalls::register_NtDebugActiveProcess(proc_t proc, const on_NtDebugActiveProcess_fn& on_func)
{
    return register_callback(*d_, proc, "NtDebugActiveProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle     = arg<nt::HANDLE>(core, 0);
        const auto DebugObjectHandle = arg<nt::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[46]);

        on_func(ProcessHandle, DebugObjectHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtDebugContinue(proc_t proc, const on_NtDebugContinue_fn& on_func)
{
    return register_callback(*d_, proc, "NtDebugContinue", [=]
    {
        auto& core = d_->core;

        const auto DebugObjectHandle = arg<nt::HANDLE>(core, 0);
        const auto ClientId          = arg<nt::PCLIENT_ID>(core, 1);
        const auto ContinueStatus    = arg<nt::NTSTATUS>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[47]);

        on_func(DebugObjectHandle, ClientId, ContinueStatus);
    });
}

opt<bpid_t> nt::syscalls::register_NtDeleteAtom(proc_t proc, const on_NtDeleteAtom_fn& on_func)
{
    return register_callback(*d_, proc, "NtDeleteAtom", [=]
    {
        auto& core = d_->core;

        const auto Atom = arg<nt::RTL_ATOM>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[48]);

        on_func(Atom);
    });
}

opt<bpid_t> nt::syscalls::register_NtDeleteBootEntry(proc_t proc, const on_NtDeleteBootEntry_fn& on_func)
{
    return register_callback(*d_, proc, "NtDeleteBootEntry", [=]
    {
        auto& core = d_->core;

        const auto Id = arg<nt::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[49]);

        on_func(Id);
    });
}

opt<bpid_t> nt::syscalls::register_NtDeleteFile(proc_t proc, const on_NtDeleteFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtDeleteFile", [=]
    {
        auto& core = d_->core;

        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[50]);

        on_func(ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtDeleteKey(proc_t proc, const on_NtDeleteKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtDeleteKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[51]);

        on_func(KeyHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtDeletePrivateNamespace(proc_t proc, const on_NtDeletePrivateNamespace_fn& on_func)
{
    return register_callback(*d_, proc, "NtDeletePrivateNamespace", [=]
    {
        auto& core = d_->core;

        const auto NamespaceHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[52]);

        on_func(NamespaceHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtDeviceIoControlFile(proc_t proc, const on_NtDeviceIoControlFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtDeviceIoControlFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle         = arg<nt::HANDLE>(core, 0);
        const auto Event              = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine         = arg<nt::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext         = arg<nt::PVOID>(core, 3);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core, 4);
        const auto IoControlCode      = arg<nt::ULONG>(core, 5);
        const auto InputBuffer        = arg<nt::PVOID>(core, 6);
        const auto InputBufferLength  = arg<nt::ULONG>(core, 7);
        const auto OutputBuffer       = arg<nt::PVOID>(core, 8);
        const auto OutputBufferLength = arg<nt::ULONG>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[53]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtDisableLastKnownGood(proc_t proc, const on_NtDisableLastKnownGood_fn& on_func)
{
    return register_callback(*d_, proc, "NtDisableLastKnownGood", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[54]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_NtDrawText(proc_t proc, const on_NtDrawText_fn& on_func)
{
    return register_callback(*d_, proc, "NtDrawText", [=]
    {
        auto& core = d_->core;

        const auto Text = arg<nt::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[55]);

        on_func(Text);
    });
}

opt<bpid_t> nt::syscalls::register_NtDuplicateToken(proc_t proc, const on_NtDuplicateToken_fn& on_func)
{
    return register_callback(*d_, proc, "NtDuplicateToken", [=]
    {
        auto& core = d_->core;

        const auto ExistingTokenHandle = arg<nt::HANDLE>(core, 0);
        const auto DesiredAccess       = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes    = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto EffectiveOnly       = arg<nt::BOOLEAN>(core, 3);
        const auto TokenType           = arg<nt::TOKEN_TYPE>(core, 4);
        const auto NewTokenHandle      = arg<nt::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[56]);

        on_func(ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtEnableLastKnownGood(proc_t proc, const on_NtEnableLastKnownGood_fn& on_func)
{
    return register_callback(*d_, proc, "NtEnableLastKnownGood", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[57]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_NtEnumerateBootEntries(proc_t proc, const on_NtEnumerateBootEntries_fn& on_func)
{
    return register_callback(*d_, proc, "NtEnumerateBootEntries", [=]
    {
        auto& core = d_->core;

        const auto Buffer       = arg<nt::PVOID>(core, 0);
        const auto BufferLength = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[58]);

        on_func(Buffer, BufferLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtEnumerateKey(proc_t proc, const on_NtEnumerateKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtEnumerateKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle           = arg<nt::HANDLE>(core, 0);
        const auto Index               = arg<nt::ULONG>(core, 1);
        const auto KeyInformationClass = arg<nt::KEY_INFORMATION_CLASS>(core, 2);
        const auto KeyInformation      = arg<nt::PVOID>(core, 3);
        const auto Length              = arg<nt::ULONG>(core, 4);
        const auto ResultLength        = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[59]);

        on_func(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtEnumerateSystemEnvironmentValuesEx(proc_t proc, const on_NtEnumerateSystemEnvironmentValuesEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtEnumerateSystemEnvironmentValuesEx", [=]
    {
        auto& core = d_->core;

        const auto InformationClass = arg<nt::ULONG>(core, 0);
        const auto Buffer           = arg<nt::PVOID>(core, 1);
        const auto BufferLength     = arg<nt::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[60]);

        on_func(InformationClass, Buffer, BufferLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtEnumerateTransactionObject(proc_t proc, const on_NtEnumerateTransactionObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtEnumerateTransactionObject", [=]
    {
        auto& core = d_->core;

        const auto RootObjectHandle   = arg<nt::HANDLE>(core, 0);
        const auto QueryType          = arg<nt::KTMOBJECT_TYPE>(core, 1);
        const auto ObjectCursor       = arg<nt::PKTMOBJECT_CURSOR>(core, 2);
        const auto ObjectCursorLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength       = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[61]);

        on_func(RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtExtendSection(proc_t proc, const on_NtExtendSection_fn& on_func)
{
    return register_callback(*d_, proc, "NtExtendSection", [=]
    {
        auto& core = d_->core;

        const auto SectionHandle  = arg<nt::HANDLE>(core, 0);
        const auto NewSectionSize = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[62]);

        on_func(SectionHandle, NewSectionSize);
    });
}

opt<bpid_t> nt::syscalls::register_NtFlushInstallUILanguage(proc_t proc, const on_NtFlushInstallUILanguage_fn& on_func)
{
    return register_callback(*d_, proc, "NtFlushInstallUILanguage", [=]
    {
        auto& core = d_->core;

        const auto InstallUILanguage = arg<nt::LANGID>(core, 0);
        const auto SetComittedFlag   = arg<nt::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[63]);

        on_func(InstallUILanguage, SetComittedFlag);
    });
}

opt<bpid_t> nt::syscalls::register_NtFlushInstructionCache(proc_t proc, const on_NtFlushInstructionCache_fn& on_func)
{
    return register_callback(*d_, proc, "NtFlushInstructionCache", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<nt::HANDLE>(core, 0);
        const auto BaseAddress   = arg<nt::PVOID>(core, 1);
        const auto Length        = arg<nt::SIZE_T>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[64]);

        on_func(ProcessHandle, BaseAddress, Length);
    });
}

opt<bpid_t> nt::syscalls::register_NtFlushWriteBuffer(proc_t proc, const on_NtFlushWriteBuffer_fn& on_func)
{
    return register_callback(*d_, proc, "NtFlushWriteBuffer", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[65]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_NtFreezeRegistry(proc_t proc, const on_NtFreezeRegistry_fn& on_func)
{
    return register_callback(*d_, proc, "NtFreezeRegistry", [=]
    {
        auto& core = d_->core;

        const auto TimeOutInSeconds = arg<nt::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[66]);

        on_func(TimeOutInSeconds);
    });
}

opt<bpid_t> nt::syscalls::register_NtGetNextProcess(proc_t proc, const on_NtGetNextProcess_fn& on_func)
{
    return register_callback(*d_, proc, "NtGetNextProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<nt::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto HandleAttributes = arg<nt::ULONG>(core, 2);
        const auto Flags            = arg<nt::ULONG>(core, 3);
        const auto NewProcessHandle = arg<nt::PHANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[67]);

        on_func(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtGetNotificationResourceManager(proc_t proc, const on_NtGetNotificationResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "NtGetNotificationResourceManager", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle   = arg<nt::HANDLE>(core, 0);
        const auto TransactionNotification = arg<nt::PTRANSACTION_NOTIFICATION>(core, 1);
        const auto NotificationLength      = arg<nt::ULONG>(core, 2);
        const auto Timeout                 = arg<nt::PLARGE_INTEGER>(core, 3);
        const auto ReturnLength            = arg<nt::PULONG>(core, 4);
        const auto Asynchronous            = arg<nt::ULONG>(core, 5);
        const auto AsynchronousContext     = arg<nt::ULONG_PTR>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[68]);

        on_func(ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext);
    });
}

opt<bpid_t> nt::syscalls::register_NtInitializeNlsFiles(proc_t proc, const on_NtInitializeNlsFiles_fn& on_func)
{
    return register_callback(*d_, proc, "NtInitializeNlsFiles", [=]
    {
        auto& core = d_->core;

        const auto STARBaseAddress        = arg<nt::PVOID>(core, 0);
        const auto DefaultLocaleId        = arg<nt::PLCID>(core, 1);
        const auto DefaultCasingTableSize = arg<nt::PLARGE_INTEGER>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[69]);

        on_func(STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize);
    });
}

opt<bpid_t> nt::syscalls::register_NtInitializeRegistry(proc_t proc, const on_NtInitializeRegistry_fn& on_func)
{
    return register_callback(*d_, proc, "NtInitializeRegistry", [=]
    {
        auto& core = d_->core;

        const auto BootCondition = arg<nt::USHORT>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[70]);

        on_func(BootCondition);
    });
}

opt<bpid_t> nt::syscalls::register_NtInitiatePowerAction(proc_t proc, const on_NtInitiatePowerAction_fn& on_func)
{
    return register_callback(*d_, proc, "NtInitiatePowerAction", [=]
    {
        auto& core = d_->core;

        const auto SystemAction   = arg<nt::POWER_ACTION>(core, 0);
        const auto MinSystemState = arg<nt::SYSTEM_POWER_STATE>(core, 1);
        const auto Flags          = arg<nt::ULONG>(core, 2);
        const auto Asynchronous   = arg<nt::BOOLEAN>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[71]);

        on_func(SystemAction, MinSystemState, Flags, Asynchronous);
    });
}

opt<bpid_t> nt::syscalls::register_NtIsProcessInJob(proc_t proc, const on_NtIsProcessInJob_fn& on_func)
{
    return register_callback(*d_, proc, "NtIsProcessInJob", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<nt::HANDLE>(core, 0);
        const auto JobHandle     = arg<nt::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[72]);

        on_func(ProcessHandle, JobHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtIsUILanguageComitted(proc_t proc, const on_NtIsUILanguageComitted_fn& on_func)
{
    return register_callback(*d_, proc, "NtIsUILanguageComitted", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[73]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_NtListenPort(proc_t proc, const on_NtListenPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtListenPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle        = arg<nt::HANDLE>(core, 0);
        const auto ConnectionRequest = arg<nt::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[74]);

        on_func(PortHandle, ConnectionRequest);
    });
}

opt<bpid_t> nt::syscalls::register_NtLoadKey(proc_t proc, const on_NtLoadKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtLoadKey", [=]
    {
        auto& core = d_->core;

        const auto TargetKey  = arg<nt::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile = arg<nt::POBJECT_ATTRIBUTES>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[75]);

        on_func(TargetKey, SourceFile);
    });
}

opt<bpid_t> nt::syscalls::register_NtLoadKeyEx(proc_t proc, const on_NtLoadKeyEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtLoadKeyEx", [=]
    {
        auto& core = d_->core;

        const auto TargetKey     = arg<nt::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile    = arg<nt::POBJECT_ATTRIBUTES>(core, 1);
        const auto Flags         = arg<nt::ULONG>(core, 2);
        const auto TrustClassKey = arg<nt::HANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[76]);

        on_func(TargetKey, SourceFile, Flags, TrustClassKey);
    });
}

opt<bpid_t> nt::syscalls::register_NtLockProductActivationKeys(proc_t proc, const on_NtLockProductActivationKeys_fn& on_func)
{
    return register_callback(*d_, proc, "NtLockProductActivationKeys", [=]
    {
        auto& core = d_->core;

        const auto STARpPrivateVer = arg<nt::ULONG>(core, 0);
        const auto STARpSafeMode   = arg<nt::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[77]);

        on_func(STARpPrivateVer, STARpSafeMode);
    });
}

opt<bpid_t> nt::syscalls::register_NtLockVirtualMemory(proc_t proc, const on_NtLockVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "NtLockVirtualMemory", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<nt::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(core, 1);
        const auto RegionSize      = arg<nt::PSIZE_T>(core, 2);
        const auto MapType         = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[78]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    });
}

opt<bpid_t> nt::syscalls::register_NtMakePermanentObject(proc_t proc, const on_NtMakePermanentObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtMakePermanentObject", [=]
    {
        auto& core = d_->core;

        const auto Handle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[79]);

        on_func(Handle);
    });
}

opt<bpid_t> nt::syscalls::register_NtMakeTemporaryObject(proc_t proc, const on_NtMakeTemporaryObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtMakeTemporaryObject", [=]
    {
        auto& core = d_->core;

        const auto Handle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[80]);

        on_func(Handle);
    });
}

opt<bpid_t> nt::syscalls::register_NtMapCMFModule(proc_t proc, const on_NtMapCMFModule_fn& on_func)
{
    return register_callback(*d_, proc, "NtMapCMFModule", [=]
    {
        auto& core = d_->core;

        const auto What            = arg<nt::ULONG>(core, 0);
        const auto Index           = arg<nt::ULONG>(core, 1);
        const auto CacheIndexOut   = arg<nt::PULONG>(core, 2);
        const auto CacheFlagsOut   = arg<nt::PULONG>(core, 3);
        const auto ViewSizeOut     = arg<nt::PULONG>(core, 4);
        const auto STARBaseAddress = arg<nt::PVOID>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[81]);

        on_func(What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress);
    });
}

opt<bpid_t> nt::syscalls::register_NtMapUserPhysicalPages(proc_t proc, const on_NtMapUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, proc, "NtMapUserPhysicalPages", [=]
    {
        auto& core = d_->core;

        const auto VirtualAddress = arg<nt::PVOID>(core, 0);
        const auto NumberOfPages  = arg<nt::ULONG_PTR>(core, 1);
        const auto UserPfnArra    = arg<nt::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[82]);

        on_func(VirtualAddress, NumberOfPages, UserPfnArra);
    });
}

opt<bpid_t> nt::syscalls::register_NtModifyBootEntry(proc_t proc, const on_NtModifyBootEntry_fn& on_func)
{
    return register_callback(*d_, proc, "NtModifyBootEntry", [=]
    {
        auto& core = d_->core;

        const auto BootEntry = arg<nt::PBOOT_ENTRY>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[83]);

        on_func(BootEntry);
    });
}

opt<bpid_t> nt::syscalls::register_NtNotifyChangeDirectoryFile(proc_t proc, const on_NtNotifyChangeDirectoryFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtNotifyChangeDirectoryFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle       = arg<nt::HANDLE>(core, 0);
        const auto Event            = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine       = arg<nt::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext       = arg<nt::PVOID>(core, 3);
        const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer           = arg<nt::PVOID>(core, 5);
        const auto Length           = arg<nt::ULONG>(core, 6);
        const auto CompletionFilter = arg<nt::ULONG>(core, 7);
        const auto WatchTree        = arg<nt::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[84]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree);
    });
}

opt<bpid_t> nt::syscalls::register_NtNotifyChangeMultipleKeys(proc_t proc, const on_NtNotifyChangeMultipleKeys_fn& on_func)
{
    return register_callback(*d_, proc, "NtNotifyChangeMultipleKeys", [=]
    {
        auto& core = d_->core;

        const auto MasterKeyHandle  = arg<nt::HANDLE>(core, 0);
        const auto Count            = arg<nt::ULONG>(core, 1);
        const auto SlaveObjects     = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto Event            = arg<nt::HANDLE>(core, 3);
        const auto ApcRoutine       = arg<nt::PIO_APC_ROUTINE>(core, 4);
        const auto ApcContext       = arg<nt::PVOID>(core, 5);
        const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(core, 6);
        const auto CompletionFilter = arg<nt::ULONG>(core, 7);
        const auto WatchTree        = arg<nt::BOOLEAN>(core, 8);
        const auto Buffer           = arg<nt::PVOID>(core, 9);
        const auto BufferSize       = arg<nt::ULONG>(core, 10);
        const auto Asynchronous     = arg<nt::BOOLEAN>(core, 11);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[85]);

        on_func(MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    });
}

opt<bpid_t> nt::syscalls::register_NtNotifyChangeSession(proc_t proc, const on_NtNotifyChangeSession_fn& on_func)
{
    return register_callback(*d_, proc, "NtNotifyChangeSession", [=]
    {
        auto& core = d_->core;

        const auto Session         = arg<nt::HANDLE>(core, 0);
        const auto IoStateSequence = arg<nt::ULONG>(core, 1);
        const auto Reserved        = arg<nt::PVOID>(core, 2);
        const auto Action          = arg<nt::ULONG>(core, 3);
        const auto IoState         = arg<nt::IO_SESSION_STATE>(core, 4);
        const auto IoState2        = arg<nt::IO_SESSION_STATE>(core, 5);
        const auto Buffer          = arg<nt::PVOID>(core, 6);
        const auto BufferSize      = arg<nt::ULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[86]);

        on_func(Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenEnlistment(proc_t proc, const on_NtOpenEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenEnlistment", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle      = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt::ACCESS_MASK>(core, 1);
        const auto ResourceManagerHandle = arg<nt::HANDLE>(core, 2);
        const auto EnlistmentGuid        = arg<nt::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[87]);

        on_func(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenEvent(proc_t proc, const on_NtOpenEvent_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenEvent", [=]
    {
        auto& core = d_->core;

        const auto EventHandle      = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[88]);

        on_func(EventHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenFile(proc_t proc, const on_NtOpenFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle       = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(core, 3);
        const auto ShareAccess      = arg<nt::ULONG>(core, 4);
        const auto OpenOptions      = arg<nt::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[89]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenIoCompletion(proc_t proc, const on_NtOpenIoCompletion_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenIoCompletion", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[90]);

        on_func(IoCompletionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenJobObject(proc_t proc, const on_NtOpenJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenJobObject", [=]
    {
        auto& core = d_->core;

        const auto JobHandle        = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[91]);

        on_func(JobHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenKey(proc_t proc, const on_NtOpenKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle        = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[92]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenKeyEx(proc_t proc, const on_NtOpenKeyEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenKeyEx", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle        = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto OpenOptions      = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[93]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenKeyedEvent(proc_t proc, const on_NtOpenKeyedEvent_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenKeyedEvent", [=]
    {
        auto& core = d_->core;

        const auto KeyedEventHandle = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[94]);

        on_func(KeyedEventHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenMutant(proc_t proc, const on_NtOpenMutant_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenMutant", [=]
    {
        auto& core = d_->core;

        const auto MutantHandle     = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[95]);

        on_func(MutantHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenPrivateNamespace(proc_t proc, const on_NtOpenPrivateNamespace_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenPrivateNamespace", [=]
    {
        auto& core = d_->core;

        const auto NamespaceHandle    = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto BoundaryDescriptor = arg<nt::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[96]);

        on_func(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenProcess(proc_t proc, const on_NtOpenProcess_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto ClientId         = arg<nt::PCLIENT_ID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[97]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenProcessToken(proc_t proc, const on_NtOpenProcessToken_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenProcessToken", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<nt::HANDLE>(core, 0);
        const auto DesiredAccess = arg<nt::ACCESS_MASK>(core, 1);
        const auto TokenHandle   = arg<nt::PHANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[98]);

        on_func(ProcessHandle, DesiredAccess, TokenHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenProcessTokenEx(proc_t proc, const on_NtOpenProcessTokenEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenProcessTokenEx", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<nt::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto HandleAttributes = arg<nt::ULONG>(core, 2);
        const auto TokenHandle      = arg<nt::PHANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[99]);

        on_func(ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenResourceManager(proc_t proc, const on_NtOpenResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenResourceManager", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess         = arg<nt::ACCESS_MASK>(core, 1);
        const auto TmHandle              = arg<nt::HANDLE>(core, 2);
        const auto ResourceManagerGuid   = arg<nt::LPGUID>(core, 3);
        const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[100]);

        on_func(ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenSection(proc_t proc, const on_NtOpenSection_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenSection", [=]
    {
        auto& core = d_->core;

        const auto SectionHandle    = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[101]);

        on_func(SectionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenThreadToken(proc_t proc, const on_NtOpenThreadToken_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenThreadToken", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle  = arg<nt::HANDLE>(core, 0);
        const auto DesiredAccess = arg<nt::ACCESS_MASK>(core, 1);
        const auto OpenAsSelf    = arg<nt::BOOLEAN>(core, 2);
        const auto TokenHandle   = arg<nt::PHANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[102]);

        on_func(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenThreadTokenEx(proc_t proc, const on_NtOpenThreadTokenEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenThreadTokenEx", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle     = arg<nt::HANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto OpenAsSelf       = arg<nt::BOOLEAN>(core, 2);
        const auto HandleAttributes = arg<nt::ULONG>(core, 3);
        const auto TokenHandle      = arg<nt::PHANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[103]);

        on_func(ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtOpenTransactionManager(proc_t proc, const on_NtOpenTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "NtOpenTransactionManager", [=]
    {
        auto& core = d_->core;

        const auto TmHandle         = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto LogFileName      = arg<nt::PUNICODE_STRING>(core, 3);
        const auto TmIdentity       = arg<nt::LPGUID>(core, 4);
        const auto OpenOptions      = arg<nt::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[104]);

        on_func(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions);
    });
}

opt<bpid_t> nt::syscalls::register_NtPowerInformation(proc_t proc, const on_NtPowerInformation_fn& on_func)
{
    return register_callback(*d_, proc, "NtPowerInformation", [=]
    {
        auto& core = d_->core;

        const auto InformationLevel   = arg<nt::POWER_INFORMATION_LEVEL>(core, 0);
        const auto InputBuffer        = arg<nt::PVOID>(core, 1);
        const auto InputBufferLength  = arg<nt::ULONG>(core, 2);
        const auto OutputBuffer       = arg<nt::PVOID>(core, 3);
        const auto OutputBufferLength = arg<nt::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[105]);

        on_func(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtPrePrepareEnlistment(proc_t proc, const on_NtPrePrepareEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "NtPrePrepareEnlistment", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[106]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_NtPrivilegeObjectAuditAlarm(proc_t proc, const on_NtPrivilegeObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "NtPrivilegeObjectAuditAlarm", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName = arg<nt::PUNICODE_STRING>(core, 0);
        const auto HandleId      = arg<nt::PVOID>(core, 1);
        const auto ClientToken   = arg<nt::HANDLE>(core, 2);
        const auto DesiredAccess = arg<nt::ACCESS_MASK>(core, 3);
        const auto Privileges    = arg<nt::PPRIVILEGE_SET>(core, 4);
        const auto AccessGranted = arg<nt::BOOLEAN>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[107]);

        on_func(SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);
    });
}

opt<bpid_t> nt::syscalls::register_NtPropagationComplete(proc_t proc, const on_NtPropagationComplete_fn& on_func)
{
    return register_callback(*d_, proc, "NtPropagationComplete", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle = arg<nt::HANDLE>(core, 0);
        const auto RequestCookie         = arg<nt::ULONG>(core, 1);
        const auto BufferLength          = arg<nt::ULONG>(core, 2);
        const auto Buffer                = arg<nt::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[108]);

        on_func(ResourceManagerHandle, RequestCookie, BufferLength, Buffer);
    });
}

opt<bpid_t> nt::syscalls::register_NtPropagationFailed(proc_t proc, const on_NtPropagationFailed_fn& on_func)
{
    return register_callback(*d_, proc, "NtPropagationFailed", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle = arg<nt::HANDLE>(core, 0);
        const auto RequestCookie         = arg<nt::ULONG>(core, 1);
        const auto PropStatus            = arg<nt::NTSTATUS>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[109]);

        on_func(ResourceManagerHandle, RequestCookie, PropStatus);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryBootOptions(proc_t proc, const on_NtQueryBootOptions_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryBootOptions", [=]
    {
        auto& core = d_->core;

        const auto BootOptions       = arg<nt::PBOOT_OPTIONS>(core, 0);
        const auto BootOptionsLength = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[110]);

        on_func(BootOptions, BootOptionsLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryDefaultLocale(proc_t proc, const on_NtQueryDefaultLocale_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryDefaultLocale", [=]
    {
        auto& core = d_->core;

        const auto UserProfile     = arg<nt::BOOLEAN>(core, 0);
        const auto DefaultLocaleId = arg<nt::PLCID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[111]);

        on_func(UserProfile, DefaultLocaleId);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryDirectoryFile(proc_t proc, const on_NtQueryDirectoryFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryDirectoryFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle           = arg<nt::HANDLE>(core, 0);
        const auto Event                = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine           = arg<nt::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext           = arg<nt::PVOID>(core, 3);
        const auto IoStatusBlock        = arg<nt::PIO_STATUS_BLOCK>(core, 4);
        const auto FileInformation      = arg<nt::PVOID>(core, 5);
        const auto Length               = arg<nt::ULONG>(core, 6);
        const auto FileInformationClass = arg<nt::FILE_INFORMATION_CLASS>(core, 7);
        const auto ReturnSingleEntry    = arg<nt::BOOLEAN>(core, 8);
        const auto FileName             = arg<nt::PUNICODE_STRING>(core, 9);
        const auto RestartScan          = arg<nt::BOOLEAN>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[112]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryDriverEntryOrder(proc_t proc, const on_NtQueryDriverEntryOrder_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryDriverEntryOrder", [=]
    {
        auto& core = d_->core;

        const auto Ids   = arg<nt::PULONG>(core, 0);
        const auto Count = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[113]);

        on_func(Ids, Count);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryEvent(proc_t proc, const on_NtQueryEvent_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryEvent", [=]
    {
        auto& core = d_->core;

        const auto EventHandle            = arg<nt::HANDLE>(core, 0);
        const auto EventInformationClass  = arg<nt::EVENT_INFORMATION_CLASS>(core, 1);
        const auto EventInformation       = arg<nt::PVOID>(core, 2);
        const auto EventInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength           = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[114]);

        on_func(EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryFullAttributesFile(proc_t proc, const on_NtQueryFullAttributesFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryFullAttributesFile", [=]
    {
        auto& core = d_->core;

        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 0);
        const auto FileInformation  = arg<nt::PFILE_NETWORK_OPEN_INFORMATION>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[115]);

        on_func(ObjectAttributes, FileInformation);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryInformationAtom(proc_t proc, const on_NtQueryInformationAtom_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryInformationAtom", [=]
    {
        auto& core = d_->core;

        const auto Atom                  = arg<nt::RTL_ATOM>(core, 0);
        const auto InformationClass      = arg<nt::ATOM_INFORMATION_CLASS>(core, 1);
        const auto AtomInformation       = arg<nt::PVOID>(core, 2);
        const auto AtomInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength          = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[116]);

        on_func(Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryInformationEnlistment(proc_t proc, const on_NtQueryInformationEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryInformationEnlistment", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle            = arg<nt::HANDLE>(core, 0);
        const auto EnlistmentInformationClass  = arg<nt::ENLISTMENT_INFORMATION_CLASS>(core, 1);
        const auto EnlistmentInformation       = arg<nt::PVOID>(core, 2);
        const auto EnlistmentInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength                = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[117]);

        on_func(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryInformationFile(proc_t proc, const on_NtQueryInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryInformationFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle           = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock        = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto FileInformation      = arg<nt::PVOID>(core, 2);
        const auto Length               = arg<nt::ULONG>(core, 3);
        const auto FileInformationClass = arg<nt::FILE_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[118]);

        on_func(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryInformationJobObject(proc_t proc, const on_NtQueryInformationJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryInformationJobObject", [=]
    {
        auto& core = d_->core;

        const auto JobHandle                  = arg<nt::HANDLE>(core, 0);
        const auto JobObjectInformationClass  = arg<nt::JOBOBJECTINFOCLASS>(core, 1);
        const auto JobObjectInformation       = arg<nt::PVOID>(core, 2);
        const auto JobObjectInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength               = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[119]);

        on_func(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryInformationPort(proc_t proc, const on_NtQueryInformationPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryInformationPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle           = arg<nt::HANDLE>(core, 0);
        const auto PortInformationClass = arg<nt::PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<nt::PVOID>(core, 2);
        const auto Length               = arg<nt::ULONG>(core, 3);
        const auto ReturnLength         = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[120]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryInformationProcess(proc_t proc, const on_NtQueryInformationProcess_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryInformationProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle            = arg<nt::HANDLE>(core, 0);
        const auto ProcessInformationClass  = arg<nt::PROCESSINFOCLASS>(core, 1);
        const auto ProcessInformation       = arg<nt::PVOID>(core, 2);
        const auto ProcessInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength             = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[121]);

        on_func(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryInformationResourceManager(proc_t proc, const on_NtQueryInformationResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryInformationResourceManager", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle            = arg<nt::HANDLE>(core, 0);
        const auto ResourceManagerInformationClass  = arg<nt::RESOURCEMANAGER_INFORMATION_CLASS>(core, 1);
        const auto ResourceManagerInformation       = arg<nt::PVOID>(core, 2);
        const auto ResourceManagerInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength                     = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[122]);

        on_func(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryInformationToken(proc_t proc, const on_NtQueryInformationToken_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryInformationToken", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle            = arg<nt::HANDLE>(core, 0);
        const auto TokenInformationClass  = arg<nt::TOKEN_INFORMATION_CLASS>(core, 1);
        const auto TokenInformation       = arg<nt::PVOID>(core, 2);
        const auto TokenInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength           = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[123]);

        on_func(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryInformationTransactionManager(proc_t proc, const on_NtQueryInformationTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryInformationTransactionManager", [=]
    {
        auto& core = d_->core;

        const auto TransactionManagerHandle            = arg<nt::HANDLE>(core, 0);
        const auto TransactionManagerInformationClass  = arg<nt::TRANSACTIONMANAGER_INFORMATION_CLASS>(core, 1);
        const auto TransactionManagerInformation       = arg<nt::PVOID>(core, 2);
        const auto TransactionManagerInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength                        = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[124]);

        on_func(TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryInstallUILanguage(proc_t proc, const on_NtQueryInstallUILanguage_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryInstallUILanguage", [=]
    {
        auto& core = d_->core;

        const auto STARInstallUILanguageId = arg<nt::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[125]);

        on_func(STARInstallUILanguageId);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryIntervalProfile(proc_t proc, const on_NtQueryIntervalProfile_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryIntervalProfile", [=]
    {
        auto& core = d_->core;

        const auto ProfileSource = arg<nt::KPROFILE_SOURCE>(core, 0);
        const auto Interval      = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[126]);

        on_func(ProfileSource, Interval);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryIoCompletion(proc_t proc, const on_NtQueryIoCompletion_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryIoCompletion", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle            = arg<nt::HANDLE>(core, 0);
        const auto IoCompletionInformationClass  = arg<nt::IO_COMPLETION_INFORMATION_CLASS>(core, 1);
        const auto IoCompletionInformation       = arg<nt::PVOID>(core, 2);
        const auto IoCompletionInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength                  = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[127]);

        on_func(IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryOpenSubKeys(proc_t proc, const on_NtQueryOpenSubKeys_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryOpenSubKeys", [=]
    {
        auto& core = d_->core;

        const auto TargetKey   = arg<nt::POBJECT_ATTRIBUTES>(core, 0);
        const auto HandleCount = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[128]);

        on_func(TargetKey, HandleCount);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryOpenSubKeysEx(proc_t proc, const on_NtQueryOpenSubKeysEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryOpenSubKeysEx", [=]
    {
        auto& core = d_->core;

        const auto TargetKey    = arg<nt::POBJECT_ATTRIBUTES>(core, 0);
        const auto BufferLength = arg<nt::ULONG>(core, 1);
        const auto Buffer       = arg<nt::PVOID>(core, 2);
        const auto RequiredSize = arg<nt::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[129]);

        on_func(TargetKey, BufferLength, Buffer, RequiredSize);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryPerformanceCounter(proc_t proc, const on_NtQueryPerformanceCounter_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryPerformanceCounter", [=]
    {
        auto& core = d_->core;

        const auto PerformanceCounter   = arg<nt::PLARGE_INTEGER>(core, 0);
        const auto PerformanceFrequency = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[130]);

        on_func(PerformanceCounter, PerformanceFrequency);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryPortInformationProcess(proc_t proc, const on_NtQueryPortInformationProcess_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryPortInformationProcess", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[131]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryQuotaInformationFile(proc_t proc, const on_NtQueryQuotaInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryQuotaInformationFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle        = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer            = arg<nt::PVOID>(core, 2);
        const auto Length            = arg<nt::ULONG>(core, 3);
        const auto ReturnSingleEntry = arg<nt::BOOLEAN>(core, 4);
        const auto SidList           = arg<nt::PVOID>(core, 5);
        const auto SidListLength     = arg<nt::ULONG>(core, 6);
        const auto StartSid          = arg<nt::PULONG>(core, 7);
        const auto RestartScan       = arg<nt::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[132]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan);
    });
}

opt<bpid_t> nt::syscalls::register_NtQuerySection(proc_t proc, const on_NtQuerySection_fn& on_func)
{
    return register_callback(*d_, proc, "NtQuerySection", [=]
    {
        auto& core = d_->core;

        const auto SectionHandle            = arg<nt::HANDLE>(core, 0);
        const auto SectionInformationClass  = arg<nt::SECTION_INFORMATION_CLASS>(core, 1);
        const auto SectionInformation       = arg<nt::PVOID>(core, 2);
        const auto SectionInformationLength = arg<nt::SIZE_T>(core, 3);
        const auto ReturnLength             = arg<nt::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[133]);

        on_func(SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQuerySecurityObject(proc_t proc, const on_NtQuerySecurityObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtQuerySecurityObject", [=]
    {
        auto& core = d_->core;

        const auto Handle              = arg<nt::HANDLE>(core, 0);
        const auto SecurityInformation = arg<nt::SECURITY_INFORMATION>(core, 1);
        const auto SecurityDescriptor  = arg<nt::PSECURITY_DESCRIPTOR>(core, 2);
        const auto Length              = arg<nt::ULONG>(core, 3);
        const auto LengthNeeded        = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[134]);

        on_func(Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded);
    });
}

opt<bpid_t> nt::syscalls::register_NtQuerySymbolicLinkObject(proc_t proc, const on_NtQuerySymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtQuerySymbolicLinkObject", [=]
    {
        auto& core = d_->core;

        const auto LinkHandle     = arg<nt::HANDLE>(core, 0);
        const auto LinkTarget     = arg<nt::PUNICODE_STRING>(core, 1);
        const auto ReturnedLength = arg<nt::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[135]);

        on_func(LinkHandle, LinkTarget, ReturnedLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQuerySystemEnvironmentValue(proc_t proc, const on_NtQuerySystemEnvironmentValue_fn& on_func)
{
    return register_callback(*d_, proc, "NtQuerySystemEnvironmentValue", [=]
    {
        auto& core = d_->core;

        const auto VariableName  = arg<nt::PUNICODE_STRING>(core, 0);
        const auto VariableValue = arg<nt::PWSTR>(core, 1);
        const auto ValueLength   = arg<nt::USHORT>(core, 2);
        const auto ReturnLength  = arg<nt::PUSHORT>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[136]);

        on_func(VariableName, VariableValue, ValueLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQuerySystemEnvironmentValueEx(proc_t proc, const on_NtQuerySystemEnvironmentValueEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtQuerySystemEnvironmentValueEx", [=]
    {
        auto& core = d_->core;

        const auto VariableName = arg<nt::PUNICODE_STRING>(core, 0);
        const auto VendorGuid   = arg<nt::LPGUID>(core, 1);
        const auto Value        = arg<nt::PVOID>(core, 2);
        const auto ValueLength  = arg<nt::PULONG>(core, 3);
        const auto Attributes   = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[137]);

        on_func(VariableName, VendorGuid, Value, ValueLength, Attributes);
    });
}

opt<bpid_t> nt::syscalls::register_NtQuerySystemInformation(proc_t proc, const on_NtQuerySystemInformation_fn& on_func)
{
    return register_callback(*d_, proc, "NtQuerySystemInformation", [=]
    {
        auto& core = d_->core;

        const auto SystemInformationClass  = arg<nt::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto SystemInformation       = arg<nt::PVOID>(core, 1);
        const auto SystemInformationLength = arg<nt::ULONG>(core, 2);
        const auto ReturnLength            = arg<nt::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[138]);

        on_func(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQuerySystemInformationEx(proc_t proc, const on_NtQuerySystemInformationEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtQuerySystemInformationEx", [=]
    {
        auto& core = d_->core;

        const auto SystemInformationClass  = arg<nt::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto QueryInformation        = arg<nt::PVOID>(core, 1);
        const auto QueryInformationLength  = arg<nt::ULONG>(core, 2);
        const auto SystemInformation       = arg<nt::PVOID>(core, 3);
        const auto SystemInformationLength = arg<nt::ULONG>(core, 4);
        const auto ReturnLength            = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[139]);

        on_func(SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueryValueKey(proc_t proc, const on_NtQueryValueKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueryValueKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle                = arg<nt::HANDLE>(core, 0);
        const auto ValueName                = arg<nt::PUNICODE_STRING>(core, 1);
        const auto KeyValueInformationClass = arg<nt::KEY_VALUE_INFORMATION_CLASS>(core, 2);
        const auto KeyValueInformation      = arg<nt::PVOID>(core, 3);
        const auto Length                   = arg<nt::ULONG>(core, 4);
        const auto ResultLength             = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[140]);

        on_func(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueueApcThread(proc_t proc, const on_NtQueueApcThread_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueueApcThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle = arg<nt::HANDLE>(core, 0);
        const auto ApcRoutine   = arg<nt::PPS_APC_ROUTINE>(core, 1);
        const auto ApcArgument1 = arg<nt::PVOID>(core, 2);
        const auto ApcArgument2 = arg<nt::PVOID>(core, 3);
        const auto ApcArgument3 = arg<nt::PVOID>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[141]);

        on_func(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    });
}

opt<bpid_t> nt::syscalls::register_NtQueueApcThreadEx(proc_t proc, const on_NtQueueApcThreadEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtQueueApcThreadEx", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle         = arg<nt::HANDLE>(core, 0);
        const auto UserApcReserveHandle = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine           = arg<nt::PPS_APC_ROUTINE>(core, 2);
        const auto ApcArgument1         = arg<nt::PVOID>(core, 3);
        const auto ApcArgument2         = arg<nt::PVOID>(core, 4);
        const auto ApcArgument3         = arg<nt::PVOID>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[142]);

        on_func(ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    });
}

opt<bpid_t> nt::syscalls::register_NtRaiseHardError(proc_t proc, const on_NtRaiseHardError_fn& on_func)
{
    return register_callback(*d_, proc, "NtRaiseHardError", [=]
    {
        auto& core = d_->core;

        const auto ErrorStatus                = arg<nt::NTSTATUS>(core, 0);
        const auto NumberOfParameters         = arg<nt::ULONG>(core, 1);
        const auto UnicodeStringParameterMask = arg<nt::ULONG>(core, 2);
        const auto Parameters                 = arg<nt::PULONG_PTR>(core, 3);
        const auto ValidResponseOptions       = arg<nt::ULONG>(core, 4);
        const auto Response                   = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[143]);

        on_func(ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);
    });
}

opt<bpid_t> nt::syscalls::register_NtReadFile(proc_t proc, const on_NtReadFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtReadFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<nt::HANDLE>(core, 0);
        const auto Event         = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer        = arg<nt::PVOID>(core, 5);
        const auto Length        = arg<nt::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[144]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    });
}

opt<bpid_t> nt::syscalls::register_NtReadRequestData(proc_t proc, const on_NtReadRequestData_fn& on_func)
{
    return register_callback(*d_, proc, "NtReadRequestData", [=]
    {
        auto& core = d_->core;

        const auto PortHandle        = arg<nt::HANDLE>(core, 0);
        const auto Message           = arg<nt::PPORT_MESSAGE>(core, 1);
        const auto DataEntryIndex    = arg<nt::ULONG>(core, 2);
        const auto Buffer            = arg<nt::PVOID>(core, 3);
        const auto BufferSize        = arg<nt::SIZE_T>(core, 4);
        const auto NumberOfBytesRead = arg<nt::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[145]);

        on_func(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead);
    });
}

opt<bpid_t> nt::syscalls::register_NtRecoverTransactionManager(proc_t proc, const on_NtRecoverTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "NtRecoverTransactionManager", [=]
    {
        auto& core = d_->core;

        const auto TransactionManagerHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[146]);

        on_func(TransactionManagerHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtRegisterProtocolAddressInformation(proc_t proc, const on_NtRegisterProtocolAddressInformation_fn& on_func)
{
    return register_callback(*d_, proc, "NtRegisterProtocolAddressInformation", [=]
    {
        auto& core = d_->core;

        const auto ResourceManager         = arg<nt::HANDLE>(core, 0);
        const auto ProtocolId              = arg<nt::PCRM_PROTOCOL_ID>(core, 1);
        const auto ProtocolInformationSize = arg<nt::ULONG>(core, 2);
        const auto ProtocolInformation     = arg<nt::PVOID>(core, 3);
        const auto CreateOptions           = arg<nt::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[147]);

        on_func(ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions);
    });
}

opt<bpid_t> nt::syscalls::register_NtRegisterThreadTerminatePort(proc_t proc, const on_NtRegisterThreadTerminatePort_fn& on_func)
{
    return register_callback(*d_, proc, "NtRegisterThreadTerminatePort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[148]);

        on_func(PortHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtReleaseKeyedEvent(proc_t proc, const on_NtReleaseKeyedEvent_fn& on_func)
{
    return register_callback(*d_, proc, "NtReleaseKeyedEvent", [=]
    {
        auto& core = d_->core;

        const auto KeyedEventHandle = arg<nt::HANDLE>(core, 0);
        const auto KeyValue         = arg<nt::PVOID>(core, 1);
        const auto Alertable        = arg<nt::BOOLEAN>(core, 2);
        const auto Timeout          = arg<nt::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[149]);

        on_func(KeyedEventHandle, KeyValue, Alertable, Timeout);
    });
}

opt<bpid_t> nt::syscalls::register_NtReleaseWorkerFactoryWorker(proc_t proc, const on_NtReleaseWorkerFactoryWorker_fn& on_func)
{
    return register_callback(*d_, proc, "NtReleaseWorkerFactoryWorker", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[150]);

        on_func(WorkerFactoryHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtRenameKey(proc_t proc, const on_NtRenameKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtRenameKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle = arg<nt::HANDLE>(core, 0);
        const auto NewName   = arg<nt::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[151]);

        on_func(KeyHandle, NewName);
    });
}

opt<bpid_t> nt::syscalls::register_NtRenameTransactionManager(proc_t proc, const on_NtRenameTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "NtRenameTransactionManager", [=]
    {
        auto& core = d_->core;

        const auto LogFileName                    = arg<nt::PUNICODE_STRING>(core, 0);
        const auto ExistingTransactionManagerGuid = arg<nt::LPGUID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[152]);

        on_func(LogFileName, ExistingTransactionManagerGuid);
    });
}

opt<bpid_t> nt::syscalls::register_NtReplaceKey(proc_t proc, const on_NtReplaceKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtReplaceKey", [=]
    {
        auto& core = d_->core;

        const auto NewFile      = arg<nt::POBJECT_ATTRIBUTES>(core, 0);
        const auto TargetHandle = arg<nt::HANDLE>(core, 1);
        const auto OldFile      = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[153]);

        on_func(NewFile, TargetHandle, OldFile);
    });
}

opt<bpid_t> nt::syscalls::register_NtReplacePartitionUnit(proc_t proc, const on_NtReplacePartitionUnit_fn& on_func)
{
    return register_callback(*d_, proc, "NtReplacePartitionUnit", [=]
    {
        auto& core = d_->core;

        const auto TargetInstancePath = arg<nt::PUNICODE_STRING>(core, 0);
        const auto SpareInstancePath  = arg<nt::PUNICODE_STRING>(core, 1);
        const auto Flags              = arg<nt::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[154]);

        on_func(TargetInstancePath, SpareInstancePath, Flags);
    });
}

opt<bpid_t> nt::syscalls::register_NtReplyPort(proc_t proc, const on_NtReplyPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtReplyPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle   = arg<nt::HANDLE>(core, 0);
        const auto ReplyMessage = arg<nt::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[155]);

        on_func(PortHandle, ReplyMessage);
    });
}

opt<bpid_t> nt::syscalls::register_NtReplyWaitReplyPort(proc_t proc, const on_NtReplyWaitReplyPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtReplyWaitReplyPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle   = arg<nt::HANDLE>(core, 0);
        const auto ReplyMessage = arg<nt::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[156]);

        on_func(PortHandle, ReplyMessage);
    });
}

opt<bpid_t> nt::syscalls::register_NtRequestPort(proc_t proc, const on_NtRequestPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtRequestPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle     = arg<nt::HANDLE>(core, 0);
        const auto RequestMessage = arg<nt::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[157]);

        on_func(PortHandle, RequestMessage);
    });
}

opt<bpid_t> nt::syscalls::register_NtRestoreKey(proc_t proc, const on_NtRestoreKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtRestoreKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle  = arg<nt::HANDLE>(core, 0);
        const auto FileHandle = arg<nt::HANDLE>(core, 1);
        const auto Flags      = arg<nt::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[158]);

        on_func(KeyHandle, FileHandle, Flags);
    });
}

opt<bpid_t> nt::syscalls::register_NtRollbackEnlistment(proc_t proc, const on_NtRollbackEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "NtRollbackEnlistment", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[159]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_NtRollforwardTransactionManager(proc_t proc, const on_NtRollforwardTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "NtRollforwardTransactionManager", [=]
    {
        auto& core = d_->core;

        const auto TransactionManagerHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock           = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[160]);

        on_func(TransactionManagerHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_NtSaveKey(proc_t proc, const on_NtSaveKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtSaveKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle  = arg<nt::HANDLE>(core, 0);
        const auto FileHandle = arg<nt::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[161]);

        on_func(KeyHandle, FileHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtSaveKeyEx(proc_t proc, const on_NtSaveKeyEx_fn& on_func)
{
    return register_callback(*d_, proc, "NtSaveKeyEx", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle  = arg<nt::HANDLE>(core, 0);
        const auto FileHandle = arg<nt::HANDLE>(core, 1);
        const auto Format     = arg<nt::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[162]);

        on_func(KeyHandle, FileHandle, Format);
    });
}

opt<bpid_t> nt::syscalls::register_NtSecureConnectPort(proc_t proc, const on_NtSecureConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtSecureConnectPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle                  = arg<nt::PHANDLE>(core, 0);
        const auto PortName                    = arg<nt::PUNICODE_STRING>(core, 1);
        const auto SecurityQos                 = arg<nt::PSECURITY_QUALITY_OF_SERVICE>(core, 2);
        const auto ClientView                  = arg<nt::PPORT_VIEW>(core, 3);
        const auto RequiredServerSid           = arg<nt::PSID>(core, 4);
        const auto ServerView                  = arg<nt::PREMOTE_PORT_VIEW>(core, 5);
        const auto MaxMessageLength            = arg<nt::PULONG>(core, 6);
        const auto ConnectionInformation       = arg<nt::PVOID>(core, 7);
        const auto ConnectionInformationLength = arg<nt::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[163]);

        on_func(PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetBootOptions(proc_t proc, const on_NtSetBootOptions_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetBootOptions", [=]
    {
        auto& core = d_->core;

        const auto BootOptions    = arg<nt::PBOOT_OPTIONS>(core, 0);
        const auto FieldsToChange = arg<nt::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[164]);

        on_func(BootOptions, FieldsToChange);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetContextThread(proc_t proc, const on_NtSetContextThread_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetContextThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle  = arg<nt::HANDLE>(core, 0);
        const auto ThreadContext = arg<nt::PCONTEXT>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[165]);

        on_func(ThreadHandle, ThreadContext);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetDefaultHardErrorPort(proc_t proc, const on_NtSetDefaultHardErrorPort_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetDefaultHardErrorPort", [=]
    {
        auto& core = d_->core;

        const auto DefaultHardErrorPort = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[166]);

        on_func(DefaultHardErrorPort);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetDefaultLocale(proc_t proc, const on_NtSetDefaultLocale_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetDefaultLocale", [=]
    {
        auto& core = d_->core;

        const auto UserProfile     = arg<nt::BOOLEAN>(core, 0);
        const auto DefaultLocaleId = arg<nt::LCID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[167]);

        on_func(UserProfile, DefaultLocaleId);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetDriverEntryOrder(proc_t proc, const on_NtSetDriverEntryOrder_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetDriverEntryOrder", [=]
    {
        auto& core = d_->core;

        const auto Ids   = arg<nt::PULONG>(core, 0);
        const auto Count = arg<nt::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[168]);

        on_func(Ids, Count);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetHighEventPair(proc_t proc, const on_NtSetHighEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetHighEventPair", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[169]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetInformationDebugObject(proc_t proc, const on_NtSetInformationDebugObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetInformationDebugObject", [=]
    {
        auto& core = d_->core;

        const auto DebugObjectHandle           = arg<nt::HANDLE>(core, 0);
        const auto DebugObjectInformationClass = arg<nt::DEBUGOBJECTINFOCLASS>(core, 1);
        const auto DebugInformation            = arg<nt::PVOID>(core, 2);
        const auto DebugInformationLength      = arg<nt::ULONG>(core, 3);
        const auto ReturnLength                = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[170]);

        on_func(DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetInformationEnlistment(proc_t proc, const on_NtSetInformationEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetInformationEnlistment", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle            = arg<nt::HANDLE>(core, 0);
        const auto EnlistmentInformationClass  = arg<nt::ENLISTMENT_INFORMATION_CLASS>(core, 1);
        const auto EnlistmentInformation       = arg<nt::PVOID>(core, 2);
        const auto EnlistmentInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[171]);

        on_func(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetInformationObject(proc_t proc, const on_NtSetInformationObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetInformationObject", [=]
    {
        auto& core = d_->core;

        const auto Handle                  = arg<nt::HANDLE>(core, 0);
        const auto ObjectInformationClass  = arg<nt::OBJECT_INFORMATION_CLASS>(core, 1);
        const auto ObjectInformation       = arg<nt::PVOID>(core, 2);
        const auto ObjectInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[172]);

        on_func(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetInformationProcess(proc_t proc, const on_NtSetInformationProcess_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetInformationProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle            = arg<nt::HANDLE>(core, 0);
        const auto ProcessInformationClass  = arg<nt::PROCESSINFOCLASS>(core, 1);
        const auto ProcessInformation       = arg<nt::PVOID>(core, 2);
        const auto ProcessInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[173]);

        on_func(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetInformationResourceManager(proc_t proc, const on_NtSetInformationResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetInformationResourceManager", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle            = arg<nt::HANDLE>(core, 0);
        const auto ResourceManagerInformationClass  = arg<nt::RESOURCEMANAGER_INFORMATION_CLASS>(core, 1);
        const auto ResourceManagerInformation       = arg<nt::PVOID>(core, 2);
        const auto ResourceManagerInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[174]);

        on_func(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetInformationThread(proc_t proc, const on_NtSetInformationThread_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetInformationThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle            = arg<nt::HANDLE>(core, 0);
        const auto ThreadInformationClass  = arg<nt::THREADINFOCLASS>(core, 1);
        const auto ThreadInformation       = arg<nt::PVOID>(core, 2);
        const auto ThreadInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[175]);

        on_func(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetInformationToken(proc_t proc, const on_NtSetInformationToken_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetInformationToken", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle            = arg<nt::HANDLE>(core, 0);
        const auto TokenInformationClass  = arg<nt::TOKEN_INFORMATION_CLASS>(core, 1);
        const auto TokenInformation       = arg<nt::PVOID>(core, 2);
        const auto TokenInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[176]);

        on_func(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetInformationTransaction(proc_t proc, const on_NtSetInformationTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetInformationTransaction", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle            = arg<nt::HANDLE>(core, 0);
        const auto TransactionInformationClass  = arg<nt::TRANSACTION_INFORMATION_CLASS>(core, 1);
        const auto TransactionInformation       = arg<nt::PVOID>(core, 2);
        const auto TransactionInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[177]);

        on_func(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetInformationWorkerFactory(proc_t proc, const on_NtSetInformationWorkerFactory_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetInformationWorkerFactory", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle            = arg<nt::HANDLE>(core, 0);
        const auto WorkerFactoryInformationClass  = arg<nt::WORKERFACTORYINFOCLASS>(core, 1);
        const auto WorkerFactoryInformation       = arg<nt::PVOID>(core, 2);
        const auto WorkerFactoryInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[178]);

        on_func(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetIntervalProfile(proc_t proc, const on_NtSetIntervalProfile_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetIntervalProfile", [=]
    {
        auto& core = d_->core;

        const auto Interval = arg<nt::ULONG>(core, 0);
        const auto Source   = arg<nt::KPROFILE_SOURCE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[179]);

        on_func(Interval, Source);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetSecurityObject(proc_t proc, const on_NtSetSecurityObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetSecurityObject", [=]
    {
        auto& core = d_->core;

        const auto Handle              = arg<nt::HANDLE>(core, 0);
        const auto SecurityInformation = arg<nt::SECURITY_INFORMATION>(core, 1);
        const auto SecurityDescriptor  = arg<nt::PSECURITY_DESCRIPTOR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[180]);

        on_func(Handle, SecurityInformation, SecurityDescriptor);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetThreadExecutionState(proc_t proc, const on_NtSetThreadExecutionState_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetThreadExecutionState", [=]
    {
        auto& core = d_->core;

        const auto esFlags           = arg<nt::EXECUTION_STATE>(core, 0);
        const auto STARPreviousFlags = arg<nt::EXECUTION_STATE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[181]);

        on_func(esFlags, STARPreviousFlags);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetTimerResolution(proc_t proc, const on_NtSetTimerResolution_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetTimerResolution", [=]
    {
        auto& core = d_->core;

        const auto DesiredTime   = arg<nt::ULONG>(core, 0);
        const auto SetResolution = arg<nt::BOOLEAN>(core, 1);
        const auto ActualTime    = arg<nt::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[182]);

        on_func(DesiredTime, SetResolution, ActualTime);
    });
}

opt<bpid_t> nt::syscalls::register_NtSetUuidSeed(proc_t proc, const on_NtSetUuidSeed_fn& on_func)
{
    return register_callback(*d_, proc, "NtSetUuidSeed", [=]
    {
        auto& core = d_->core;

        const auto Seed = arg<nt::PCHAR>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[183]);

        on_func(Seed);
    });
}

opt<bpid_t> nt::syscalls::register_NtSuspendThread(proc_t proc, const on_NtSuspendThread_fn& on_func)
{
    return register_callback(*d_, proc, "NtSuspendThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle         = arg<nt::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[184]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<bpid_t> nt::syscalls::register_NtTerminateJobObject(proc_t proc, const on_NtTerminateJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtTerminateJobObject", [=]
    {
        auto& core = d_->core;

        const auto JobHandle  = arg<nt::HANDLE>(core, 0);
        const auto ExitStatus = arg<nt::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[185]);

        on_func(JobHandle, ExitStatus);
    });
}

opt<bpid_t> nt::syscalls::register_NtTerminateThread(proc_t proc, const on_NtTerminateThread_fn& on_func)
{
    return register_callback(*d_, proc, "NtTerminateThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle = arg<nt::HANDLE>(core, 0);
        const auto ExitStatus   = arg<nt::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[186]);

        on_func(ThreadHandle, ExitStatus);
    });
}

opt<bpid_t> nt::syscalls::register_NtTraceControl(proc_t proc, const on_NtTraceControl_fn& on_func)
{
    return register_callback(*d_, proc, "NtTraceControl", [=]
    {
        auto& core = d_->core;

        const auto FunctionCode = arg<nt::ULONG>(core, 0);
        const auto InBuffer     = arg<nt::PVOID>(core, 1);
        const auto InBufferLen  = arg<nt::ULONG>(core, 2);
        const auto OutBuffer    = arg<nt::PVOID>(core, 3);
        const auto OutBufferLen = arg<nt::ULONG>(core, 4);
        const auto ReturnLength = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[187]);

        on_func(FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtTraceEvent(proc_t proc, const on_NtTraceEvent_fn& on_func)
{
    return register_callback(*d_, proc, "NtTraceEvent", [=]
    {
        auto& core = d_->core;

        const auto TraceHandle = arg<nt::HANDLE>(core, 0);
        const auto Flags       = arg<nt::ULONG>(core, 1);
        const auto FieldSize   = arg<nt::ULONG>(core, 2);
        const auto Fields      = arg<nt::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[188]);

        on_func(TraceHandle, Flags, FieldSize, Fields);
    });
}

opt<bpid_t> nt::syscalls::register_NtTranslateFilePath(proc_t proc, const on_NtTranslateFilePath_fn& on_func)
{
    return register_callback(*d_, proc, "NtTranslateFilePath", [=]
    {
        auto& core = d_->core;

        const auto InputFilePath        = arg<nt::PFILE_PATH>(core, 0);
        const auto OutputType           = arg<nt::ULONG>(core, 1);
        const auto OutputFilePath       = arg<nt::PFILE_PATH>(core, 2);
        const auto OutputFilePathLength = arg<nt::PULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[189]);

        on_func(InputFilePath, OutputType, OutputFilePath, OutputFilePathLength);
    });
}

opt<bpid_t> nt::syscalls::register_NtUnloadKey(proc_t proc, const on_NtUnloadKey_fn& on_func)
{
    return register_callback(*d_, proc, "NtUnloadKey", [=]
    {
        auto& core = d_->core;

        const auto TargetKey = arg<nt::POBJECT_ATTRIBUTES>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[190]);

        on_func(TargetKey);
    });
}

opt<bpid_t> nt::syscalls::register_NtUnloadKey2(proc_t proc, const on_NtUnloadKey2_fn& on_func)
{
    return register_callback(*d_, proc, "NtUnloadKey2", [=]
    {
        auto& core = d_->core;

        const auto TargetKey = arg<nt::POBJECT_ATTRIBUTES>(core, 0);
        const auto Flags     = arg<nt::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[191]);

        on_func(TargetKey, Flags);
    });
}

opt<bpid_t> nt::syscalls::register_NtUnmapViewOfSection(proc_t proc, const on_NtUnmapViewOfSection_fn& on_func)
{
    return register_callback(*d_, proc, "NtUnmapViewOfSection", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<nt::HANDLE>(core, 0);
        const auto BaseAddress   = arg<nt::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[192]);

        on_func(ProcessHandle, BaseAddress);
    });
}

opt<bpid_t> nt::syscalls::register_NtWaitForKeyedEvent(proc_t proc, const on_NtWaitForKeyedEvent_fn& on_func)
{
    return register_callback(*d_, proc, "NtWaitForKeyedEvent", [=]
    {
        auto& core = d_->core;

        const auto KeyedEventHandle = arg<nt::HANDLE>(core, 0);
        const auto KeyValue         = arg<nt::PVOID>(core, 1);
        const auto Alertable        = arg<nt::BOOLEAN>(core, 2);
        const auto Timeout          = arg<nt::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[193]);

        on_func(KeyedEventHandle, KeyValue, Alertable, Timeout);
    });
}

opt<bpid_t> nt::syscalls::register_NtWaitForMultipleObjects(proc_t proc, const on_NtWaitForMultipleObjects_fn& on_func)
{
    return register_callback(*d_, proc, "NtWaitForMultipleObjects", [=]
    {
        auto& core = d_->core;

        const auto Count     = arg<nt::ULONG>(core, 0);
        const auto Handles   = arg<nt::HANDLE>(core, 1);
        const auto WaitType  = arg<nt::WAIT_TYPE>(core, 2);
        const auto Alertable = arg<nt::BOOLEAN>(core, 3);
        const auto Timeout   = arg<nt::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[194]);

        on_func(Count, Handles, WaitType, Alertable, Timeout);
    });
}

opt<bpid_t> nt::syscalls::register_NtWaitForSingleObject(proc_t proc, const on_NtWaitForSingleObject_fn& on_func)
{
    return register_callback(*d_, proc, "NtWaitForSingleObject", [=]
    {
        auto& core = d_->core;

        const auto Handle    = arg<nt::HANDLE>(core, 0);
        const auto Alertable = arg<nt::BOOLEAN>(core, 1);
        const auto Timeout   = arg<nt::PLARGE_INTEGER>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[195]);

        on_func(Handle, Alertable, Timeout);
    });
}

opt<bpid_t> nt::syscalls::register_NtWaitLowEventPair(proc_t proc, const on_NtWaitLowEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "NtWaitLowEventPair", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[196]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtWorkerFactoryWorkerReady(proc_t proc, const on_NtWorkerFactoryWorkerReady_fn& on_func)
{
    return register_callback(*d_, proc, "NtWorkerFactoryWorkerReady", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[197]);

        on_func(WorkerFactoryHandle);
    });
}

opt<bpid_t> nt::syscalls::register_NtWriteFile(proc_t proc, const on_NtWriteFile_fn& on_func)
{
    return register_callback(*d_, proc, "NtWriteFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<nt::HANDLE>(core, 0);
        const auto Event         = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core, 4);
        const auto Buffer        = arg<nt::PVOID>(core, 5);
        const auto Length        = arg<nt::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[198]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    });
}

opt<bpid_t> nt::syscalls::register_NtWriteVirtualMemory(proc_t proc, const on_NtWriteVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "NtWriteVirtualMemory", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle        = arg<nt::HANDLE>(core, 0);
        const auto BaseAddress          = arg<nt::PVOID>(core, 1);
        const auto Buffer               = arg<nt::PVOID>(core, 2);
        const auto BufferSize           = arg<nt::SIZE_T>(core, 3);
        const auto NumberOfBytesWritten = arg<nt::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[199]);

        on_func(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAccessCheck(proc_t proc, const on_ZwAccessCheck_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAccessCheck", [=]
    {
        auto& core = d_->core;

        const auto SecurityDescriptor = arg<nt::PSECURITY_DESCRIPTOR>(core, 0);
        const auto ClientToken        = arg<nt::HANDLE>(core, 1);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core, 2);
        const auto GenericMapping     = arg<nt::PGENERIC_MAPPING>(core, 3);
        const auto PrivilegeSet       = arg<nt::PPRIVILEGE_SET>(core, 4);
        const auto PrivilegeSetLength = arg<nt::PULONG>(core, 5);
        const auto GrantedAccess      = arg<nt::PACCESS_MASK>(core, 6);
        const auto AccessStatus       = arg<nt::PNTSTATUS>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[200]);

        on_func(SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAccessCheckAndAuditAlarm(proc_t proc, const on_ZwAccessCheckAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAccessCheckAndAuditAlarm", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName      = arg<nt::PUNICODE_STRING>(core, 0);
        const auto HandleId           = arg<nt::PVOID>(core, 1);
        const auto ObjectTypeName     = arg<nt::PUNICODE_STRING>(core, 2);
        const auto ObjectName         = arg<nt::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor = arg<nt::PSECURITY_DESCRIPTOR>(core, 4);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core, 5);
        const auto GenericMapping     = arg<nt::PGENERIC_MAPPING>(core, 6);
        const auto ObjectCreation     = arg<nt::BOOLEAN>(core, 7);
        const auto GrantedAccess      = arg<nt::PACCESS_MASK>(core, 8);
        const auto AccessStatus       = arg<nt::PNTSTATUS>(core, 9);
        const auto GenerateOnClose    = arg<nt::PBOOLEAN>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[201]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAccessCheckByType(proc_t proc, const on_ZwAccessCheckByType_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAccessCheckByType", [=]
    {
        auto& core = d_->core;

        const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(core, 0);
        const auto PrincipalSelfSid     = arg<nt::PSID>(core, 1);
        const auto ClientToken          = arg<nt::HANDLE>(core, 2);
        const auto DesiredAccess        = arg<nt::ACCESS_MASK>(core, 3);
        const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(core, 4);
        const auto ObjectTypeListLength = arg<nt::ULONG>(core, 5);
        const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(core, 6);
        const auto PrivilegeSet         = arg<nt::PPRIVILEGE_SET>(core, 7);
        const auto PrivilegeSetLength   = arg<nt::PULONG>(core, 8);
        const auto GrantedAccess        = arg<nt::PACCESS_MASK>(core, 9);
        const auto AccessStatus         = arg<nt::PNTSTATUS>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[202]);

        on_func(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAccessCheckByTypeAndAuditAlarm(proc_t proc, const on_ZwAccessCheckByTypeAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAccessCheckByTypeAndAuditAlarm", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName        = arg<nt::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<nt::PVOID>(core, 1);
        const auto ObjectTypeName       = arg<nt::PUNICODE_STRING>(core, 2);
        const auto ObjectName           = arg<nt::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(core, 4);
        const auto PrincipalSelfSid     = arg<nt::PSID>(core, 5);
        const auto DesiredAccess        = arg<nt::ACCESS_MASK>(core, 6);
        const auto AuditType            = arg<nt::AUDIT_EVENT_TYPE>(core, 7);
        const auto Flags                = arg<nt::ULONG>(core, 8);
        const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(core, 9);
        const auto ObjectTypeListLength = arg<nt::ULONG>(core, 10);
        const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(core, 11);
        const auto ObjectCreation       = arg<nt::BOOLEAN>(core, 12);
        const auto GrantedAccess        = arg<nt::PACCESS_MASK>(core, 13);
        const auto AccessStatus         = arg<nt::PNTSTATUS>(core, 14);
        const auto GenerateOnClose      = arg<nt::PBOOLEAN>(core, 15);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[203]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAccessCheckByTypeResultList(proc_t proc, const on_ZwAccessCheckByTypeResultList_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAccessCheckByTypeResultList", [=]
    {
        auto& core = d_->core;

        const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(core, 0);
        const auto PrincipalSelfSid     = arg<nt::PSID>(core, 1);
        const auto ClientToken          = arg<nt::HANDLE>(core, 2);
        const auto DesiredAccess        = arg<nt::ACCESS_MASK>(core, 3);
        const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(core, 4);
        const auto ObjectTypeListLength = arg<nt::ULONG>(core, 5);
        const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(core, 6);
        const auto PrivilegeSet         = arg<nt::PPRIVILEGE_SET>(core, 7);
        const auto PrivilegeSetLength   = arg<nt::PULONG>(core, 8);
        const auto GrantedAccess        = arg<nt::PACCESS_MASK>(core, 9);
        const auto AccessStatus         = arg<nt::PNTSTATUS>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[204]);

        on_func(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAccessCheckByTypeResultListAndAuditAlarm(proc_t proc, const on_ZwAccessCheckByTypeResultListAndAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAccessCheckByTypeResultListAndAuditAlarm", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName        = arg<nt::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<nt::PVOID>(core, 1);
        const auto ObjectTypeName       = arg<nt::PUNICODE_STRING>(core, 2);
        const auto ObjectName           = arg<nt::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(core, 4);
        const auto PrincipalSelfSid     = arg<nt::PSID>(core, 5);
        const auto DesiredAccess        = arg<nt::ACCESS_MASK>(core, 6);
        const auto AuditType            = arg<nt::AUDIT_EVENT_TYPE>(core, 7);
        const auto Flags                = arg<nt::ULONG>(core, 8);
        const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(core, 9);
        const auto ObjectTypeListLength = arg<nt::ULONG>(core, 10);
        const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(core, 11);
        const auto ObjectCreation       = arg<nt::BOOLEAN>(core, 12);
        const auto GrantedAccess        = arg<nt::PACCESS_MASK>(core, 13);
        const auto AccessStatus         = arg<nt::PNTSTATUS>(core, 14);
        const auto GenerateOnClose      = arg<nt::PBOOLEAN>(core, 15);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[205]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(proc_t proc, const on_ZwAccessCheckByTypeResultListAndAuditAlarmByHandle_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAccessCheckByTypeResultListAndAuditAlarmByHandle", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName        = arg<nt::PUNICODE_STRING>(core, 0);
        const auto HandleId             = arg<nt::PVOID>(core, 1);
        const auto ClientToken          = arg<nt::HANDLE>(core, 2);
        const auto ObjectTypeName       = arg<nt::PUNICODE_STRING>(core, 3);
        const auto ObjectName           = arg<nt::PUNICODE_STRING>(core, 4);
        const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(core, 5);
        const auto PrincipalSelfSid     = arg<nt::PSID>(core, 6);
        const auto DesiredAccess        = arg<nt::ACCESS_MASK>(core, 7);
        const auto AuditType            = arg<nt::AUDIT_EVENT_TYPE>(core, 8);
        const auto Flags                = arg<nt::ULONG>(core, 9);
        const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(core, 10);
        const auto ObjectTypeListLength = arg<nt::ULONG>(core, 11);
        const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(core, 12);
        const auto ObjectCreation       = arg<nt::BOOLEAN>(core, 13);
        const auto GrantedAccess        = arg<nt::PACCESS_MASK>(core, 14);
        const auto AccessStatus         = arg<nt::PNTSTATUS>(core, 15);
        const auto GenerateOnClose      = arg<nt::PBOOLEAN>(core, 16);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[206]);

        on_func(SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAddAtom(proc_t proc, const on_ZwAddAtom_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAddAtom", [=]
    {
        auto& core = d_->core;

        const auto AtomName = arg<nt::PWSTR>(core, 0);
        const auto Length   = arg<nt::ULONG>(core, 1);
        const auto Atom     = arg<nt::PRTL_ATOM>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[207]);

        on_func(AtomName, Length, Atom);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAddBootEntry(proc_t proc, const on_ZwAddBootEntry_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAddBootEntry", [=]
    {
        auto& core = d_->core;

        const auto BootEntry = arg<nt::PBOOT_ENTRY>(core, 0);
        const auto Id        = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[208]);

        on_func(BootEntry, Id);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAlertResumeThread(proc_t proc, const on_ZwAlertResumeThread_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAlertResumeThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle         = arg<nt::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[209]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAllocateLocallyUniqueId(proc_t proc, const on_ZwAllocateLocallyUniqueId_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAllocateLocallyUniqueId", [=]
    {
        auto& core = d_->core;

        const auto Luid = arg<nt::PLUID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[210]);

        on_func(Luid);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAllocateUserPhysicalPages(proc_t proc, const on_ZwAllocateUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAllocateUserPhysicalPages", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<nt::HANDLE>(core, 0);
        const auto NumberOfPages = arg<nt::PULONG_PTR>(core, 1);
        const auto UserPfnArra   = arg<nt::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[211]);

        on_func(ProcessHandle, NumberOfPages, UserPfnArra);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAllocateUuids(proc_t proc, const on_ZwAllocateUuids_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAllocateUuids", [=]
    {
        auto& core = d_->core;

        const auto Time     = arg<nt::PULARGE_INTEGER>(core, 0);
        const auto Range    = arg<nt::PULONG>(core, 1);
        const auto Sequence = arg<nt::PULONG>(core, 2);
        const auto Seed     = arg<nt::PCHAR>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[212]);

        on_func(Time, Range, Sequence, Seed);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAllocateVirtualMemory(proc_t proc, const on_ZwAllocateVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAllocateVirtualMemory", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<nt::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(core, 1);
        const auto ZeroBits        = arg<nt::ULONG_PTR>(core, 2);
        const auto RegionSize      = arg<nt::PSIZE_T>(core, 3);
        const auto AllocationType  = arg<nt::ULONG>(core, 4);
        const auto Protect         = arg<nt::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[213]);

        on_func(ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAlpcAcceptConnectPort(proc_t proc, const on_ZwAlpcAcceptConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAlpcAcceptConnectPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle                  = arg<nt::PHANDLE>(core, 0);
        const auto ConnectionPortHandle        = arg<nt::HANDLE>(core, 1);
        const auto Flags                       = arg<nt::ULONG>(core, 2);
        const auto ObjectAttributes            = arg<nt::POBJECT_ATTRIBUTES>(core, 3);
        const auto PortAttributes              = arg<nt::PALPC_PORT_ATTRIBUTES>(core, 4);
        const auto PortContext                 = arg<nt::PVOID>(core, 5);
        const auto ConnectionRequest           = arg<nt::PPORT_MESSAGE>(core, 6);
        const auto ConnectionMessageAttributes = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(core, 7);
        const auto AcceptConnection            = arg<nt::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[214]);

        on_func(PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAlpcCreatePortSection(proc_t proc, const on_ZwAlpcCreatePortSection_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAlpcCreatePortSection", [=]
    {
        auto& core = d_->core;

        const auto PortHandle        = arg<nt::HANDLE>(core, 0);
        const auto Flags             = arg<nt::ULONG>(core, 1);
        const auto SectionHandle     = arg<nt::HANDLE>(core, 2);
        const auto SectionSize       = arg<nt::SIZE_T>(core, 3);
        const auto AlpcSectionHandle = arg<nt::PALPC_HANDLE>(core, 4);
        const auto ActualSectionSize = arg<nt::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[215]);

        on_func(PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAlpcCreateResourceReserve(proc_t proc, const on_ZwAlpcCreateResourceReserve_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAlpcCreateResourceReserve", [=]
    {
        auto& core = d_->core;

        const auto PortHandle  = arg<nt::HANDLE>(core, 0);
        const auto Flags       = arg<nt::ULONG>(core, 1);
        const auto MessageSize = arg<nt::SIZE_T>(core, 2);
        const auto ResourceId  = arg<nt::PALPC_HANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[216]);

        on_func(PortHandle, Flags, MessageSize, ResourceId);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAlpcDeleteSectionView(proc_t proc, const on_ZwAlpcDeleteSectionView_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAlpcDeleteSectionView", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<nt::HANDLE>(core, 0);
        const auto Flags      = arg<nt::ULONG>(core, 1);
        const auto ViewBase   = arg<nt::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[217]);

        on_func(PortHandle, Flags, ViewBase);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAlpcDeleteSecurityContext(proc_t proc, const on_ZwAlpcDeleteSecurityContext_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAlpcDeleteSecurityContext", [=]
    {
        auto& core = d_->core;

        const auto PortHandle    = arg<nt::HANDLE>(core, 0);
        const auto Flags         = arg<nt::ULONG>(core, 1);
        const auto ContextHandle = arg<nt::ALPC_HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[218]);

        on_func(PortHandle, Flags, ContextHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAlpcQueryInformation(proc_t proc, const on_ZwAlpcQueryInformation_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAlpcQueryInformation", [=]
    {
        auto& core = d_->core;

        const auto PortHandle           = arg<nt::HANDLE>(core, 0);
        const auto PortInformationClass = arg<nt::ALPC_PORT_INFORMATION_CLASS>(core, 1);
        const auto PortInformation      = arg<nt::PVOID>(core, 2);
        const auto Length               = arg<nt::ULONG>(core, 3);
        const auto ReturnLength         = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[219]);

        on_func(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAlpcQueryInformationMessage(proc_t proc, const on_ZwAlpcQueryInformationMessage_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAlpcQueryInformationMessage", [=]
    {
        auto& core = d_->core;

        const auto PortHandle              = arg<nt::HANDLE>(core, 0);
        const auto PortMessage             = arg<nt::PPORT_MESSAGE>(core, 1);
        const auto MessageInformationClass = arg<nt::ALPC_MESSAGE_INFORMATION_CLASS>(core, 2);
        const auto MessageInformation      = arg<nt::PVOID>(core, 3);
        const auto Length                  = arg<nt::ULONG>(core, 4);
        const auto ReturnLength            = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[220]);

        on_func(PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAlpcSendWaitReceivePort(proc_t proc, const on_ZwAlpcSendWaitReceivePort_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAlpcSendWaitReceivePort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle               = arg<nt::HANDLE>(core, 0);
        const auto Flags                    = arg<nt::ULONG>(core, 1);
        const auto SendMessage              = arg<nt::PPORT_MESSAGE>(core, 2);
        const auto SendMessageAttributes    = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(core, 3);
        const auto ReceiveMessage           = arg<nt::PPORT_MESSAGE>(core, 4);
        const auto BufferLength             = arg<nt::PULONG>(core, 5);
        const auto ReceiveMessageAttributes = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(core, 6);
        const auto Timeout                  = arg<nt::PLARGE_INTEGER>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[221]);

        on_func(PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout);
    });
}

opt<bpid_t> nt::syscalls::register_ZwAreMappedFilesTheSame(proc_t proc, const on_ZwAreMappedFilesTheSame_fn& on_func)
{
    return register_callback(*d_, proc, "ZwAreMappedFilesTheSame", [=]
    {
        auto& core = d_->core;

        const auto File1MappedAsAnImage = arg<nt::PVOID>(core, 0);
        const auto File2MappedAsFile    = arg<nt::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[222]);

        on_func(File1MappedAsAnImage, File2MappedAsFile);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCancelIoFile(proc_t proc, const on_ZwCancelIoFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCancelIoFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[223]);

        on_func(FileHandle, IoStatusBlock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCancelIoFileEx(proc_t proc, const on_ZwCancelIoFileEx_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCancelIoFileEx", [=]
    {
        auto& core = d_->core;

        const auto FileHandle        = arg<nt::HANDLE>(core, 0);
        const auto IoRequestToCancel = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[224]);

        on_func(FileHandle, IoRequestToCancel, IoStatusBlock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCancelSynchronousIoFile(proc_t proc, const on_ZwCancelSynchronousIoFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCancelSynchronousIoFile", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle      = arg<nt::HANDLE>(core, 0);
        const auto IoRequestToCancel = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[225]);

        on_func(ThreadHandle, IoRequestToCancel, IoStatusBlock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCloseObjectAuditAlarm(proc_t proc, const on_ZwCloseObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCloseObjectAuditAlarm", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName   = arg<nt::PUNICODE_STRING>(core, 0);
        const auto HandleId        = arg<nt::PVOID>(core, 1);
        const auto GenerateOnClose = arg<nt::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[226]);

        on_func(SubsystemName, HandleId, GenerateOnClose);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCommitEnlistment(proc_t proc, const on_ZwCommitEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCommitEnlistment", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[227]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCommitTransaction(proc_t proc, const on_ZwCommitTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCommitTransaction", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle = arg<nt::HANDLE>(core, 0);
        const auto Wait              = arg<nt::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[228]);

        on_func(TransactionHandle, Wait);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCompareTokens(proc_t proc, const on_ZwCompareTokens_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCompareTokens", [=]
    {
        auto& core = d_->core;

        const auto FirstTokenHandle  = arg<nt::HANDLE>(core, 0);
        const auto SecondTokenHandle = arg<nt::HANDLE>(core, 1);
        const auto Equal             = arg<nt::PBOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[229]);

        on_func(FirstTokenHandle, SecondTokenHandle, Equal);
    });
}

opt<bpid_t> nt::syscalls::register_ZwConnectPort(proc_t proc, const on_ZwConnectPort_fn& on_func)
{
    return register_callback(*d_, proc, "ZwConnectPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle                  = arg<nt::PHANDLE>(core, 0);
        const auto PortName                    = arg<nt::PUNICODE_STRING>(core, 1);
        const auto SecurityQos                 = arg<nt::PSECURITY_QUALITY_OF_SERVICE>(core, 2);
        const auto ClientView                  = arg<nt::PPORT_VIEW>(core, 3);
        const auto ServerView                  = arg<nt::PREMOTE_PORT_VIEW>(core, 4);
        const auto MaxMessageLength            = arg<nt::PULONG>(core, 5);
        const auto ConnectionInformation       = arg<nt::PVOID>(core, 6);
        const auto ConnectionInformationLength = arg<nt::PULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[230]);

        on_func(PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwContinue(proc_t proc, const on_ZwContinue_fn& on_func)
{
    return register_callback(*d_, proc, "ZwContinue", [=]
    {
        auto& core = d_->core;

        const auto ContextRecord = arg<nt::PCONTEXT>(core, 0);
        const auto TestAlert     = arg<nt::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[231]);

        on_func(ContextRecord, TestAlert);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateEvent(proc_t proc, const on_ZwCreateEvent_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateEvent", [=]
    {
        auto& core = d_->core;

        const auto EventHandle      = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto EventType        = arg<nt::EVENT_TYPE>(core, 3);
        const auto InitialState     = arg<nt::BOOLEAN>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[232]);

        on_func(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateEventPair(proc_t proc, const on_ZwCreateEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateEventPair", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle  = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[233]);

        on_func(EventPairHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateFile(proc_t proc, const on_ZwCreateFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle        = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core, 3);
        const auto AllocationSize    = arg<nt::PLARGE_INTEGER>(core, 4);
        const auto FileAttributes    = arg<nt::ULONG>(core, 5);
        const auto ShareAccess       = arg<nt::ULONG>(core, 6);
        const auto CreateDisposition = arg<nt::ULONG>(core, 7);
        const auto CreateOptions     = arg<nt::ULONG>(core, 8);
        const auto EaBuffer          = arg<nt::PVOID>(core, 9);
        const auto EaLength          = arg<nt::ULONG>(core, 10);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[234]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateJobObject(proc_t proc, const on_ZwCreateJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateJobObject", [=]
    {
        auto& core = d_->core;

        const auto JobHandle        = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[235]);

        on_func(JobHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateJobSet(proc_t proc, const on_ZwCreateJobSet_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateJobSet", [=]
    {
        auto& core = d_->core;

        const auto NumJob     = arg<nt::ULONG>(core, 0);
        const auto UserJobSet = arg<nt::PJOB_SET_ARRAY>(core, 1);
        const auto Flags      = arg<nt::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[236]);

        on_func(NumJob, UserJobSet, Flags);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateKey(proc_t proc, const on_ZwCreateKey_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle        = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto TitleIndex       = arg<nt::ULONG>(core, 3);
        const auto Class            = arg<nt::PUNICODE_STRING>(core, 4);
        const auto CreateOptions    = arg<nt::ULONG>(core, 5);
        const auto Disposition      = arg<nt::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[237]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateKeyTransacted(proc_t proc, const on_ZwCreateKeyTransacted_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateKeyTransacted", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle         = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto TitleIndex        = arg<nt::ULONG>(core, 3);
        const auto Class             = arg<nt::PUNICODE_STRING>(core, 4);
        const auto CreateOptions     = arg<nt::ULONG>(core, 5);
        const auto TransactionHandle = arg<nt::HANDLE>(core, 6);
        const auto Disposition       = arg<nt::PULONG>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[238]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateKeyedEvent(proc_t proc, const on_ZwCreateKeyedEvent_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateKeyedEvent", [=]
    {
        auto& core = d_->core;

        const auto KeyedEventHandle = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto Flags            = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[239]);

        on_func(KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateMailslotFile(proc_t proc, const on_ZwCreateMailslotFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateMailslotFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle         = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt::ULONG>(core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core, 3);
        const auto CreateOptions      = arg<nt::ULONG>(core, 4);
        const auto MailslotQuota      = arg<nt::ULONG>(core, 5);
        const auto MaximumMessageSize = arg<nt::ULONG>(core, 6);
        const auto ReadTimeout        = arg<nt::PLARGE_INTEGER>(core, 7);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[240]);

        on_func(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreatePort(proc_t proc, const on_ZwCreatePort_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreatePort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle              = arg<nt::PHANDLE>(core, 0);
        const auto ObjectAttributes        = arg<nt::POBJECT_ATTRIBUTES>(core, 1);
        const auto MaxConnectionInfoLength = arg<nt::ULONG>(core, 2);
        const auto MaxMessageLength        = arg<nt::ULONG>(core, 3);
        const auto MaxPoolUsage            = arg<nt::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[241]);

        on_func(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreatePrivateNamespace(proc_t proc, const on_ZwCreatePrivateNamespace_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreatePrivateNamespace", [=]
    {
        auto& core = d_->core;

        const auto NamespaceHandle    = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto BoundaryDescriptor = arg<nt::PVOID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[242]);

        on_func(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateProcessEx(proc_t proc, const on_ZwCreateProcessEx_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateProcessEx", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto ParentProcess    = arg<nt::HANDLE>(core, 3);
        const auto Flags            = arg<nt::ULONG>(core, 4);
        const auto SectionHandle    = arg<nt::HANDLE>(core, 5);
        const auto DebugPort        = arg<nt::HANDLE>(core, 6);
        const auto ExceptionPort    = arg<nt::HANDLE>(core, 7);
        const auto JobMemberLevel   = arg<nt::ULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[243]);

        on_func(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateProfile(proc_t proc, const on_ZwCreateProfile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateProfile", [=]
    {
        auto& core = d_->core;

        const auto ProfileHandle = arg<nt::PHANDLE>(core, 0);
        const auto Process       = arg<nt::HANDLE>(core, 1);
        const auto RangeBase     = arg<nt::PVOID>(core, 2);
        const auto RangeSize     = arg<nt::SIZE_T>(core, 3);
        const auto BucketSize    = arg<nt::ULONG>(core, 4);
        const auto Buffer        = arg<nt::PULONG>(core, 5);
        const auto BufferSize    = arg<nt::ULONG>(core, 6);
        const auto ProfileSource = arg<nt::KPROFILE_SOURCE>(core, 7);
        const auto Affinity      = arg<nt::KAFFINITY>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[244]);

        on_func(ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateProfileEx(proc_t proc, const on_ZwCreateProfileEx_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateProfileEx", [=]
    {
        auto& core = d_->core;

        const auto ProfileHandle      = arg<nt::PHANDLE>(core, 0);
        const auto Process            = arg<nt::HANDLE>(core, 1);
        const auto ProfileBase        = arg<nt::PVOID>(core, 2);
        const auto ProfileSize        = arg<nt::SIZE_T>(core, 3);
        const auto BucketSize         = arg<nt::ULONG>(core, 4);
        const auto Buffer             = arg<nt::PULONG>(core, 5);
        const auto BufferSize         = arg<nt::ULONG>(core, 6);
        const auto ProfileSource      = arg<nt::KPROFILE_SOURCE>(core, 7);
        const auto GroupAffinityCount = arg<nt::ULONG>(core, 8);
        const auto GroupAffinity      = arg<nt::PGROUP_AFFINITY>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[245]);

        on_func(ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateSymbolicLinkObject(proc_t proc, const on_ZwCreateSymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateSymbolicLinkObject", [=]
    {
        auto& core = d_->core;

        const auto LinkHandle       = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto LinkTarget       = arg<nt::PUNICODE_STRING>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[246]);

        on_func(LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateTimer(proc_t proc, const on_ZwCreateTimer_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateTimer", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle      = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto TimerType        = arg<nt::TIMER_TYPE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[247]);

        on_func(TimerHandle, DesiredAccess, ObjectAttributes, TimerType);
    });
}

opt<bpid_t> nt::syscalls::register_ZwCreateTransactionManager(proc_t proc, const on_ZwCreateTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "ZwCreateTransactionManager", [=]
    {
        auto& core = d_->core;

        const auto TmHandle         = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto LogFileName      = arg<nt::PUNICODE_STRING>(core, 3);
        const auto CreateOptions    = arg<nt::ULONG>(core, 4);
        const auto CommitStrength   = arg<nt::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[248]);

        on_func(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwDelayExecution(proc_t proc, const on_ZwDelayExecution_fn& on_func)
{
    return register_callback(*d_, proc, "ZwDelayExecution", [=]
    {
        auto& core = d_->core;

        const auto Alertable     = arg<nt::BOOLEAN>(core, 0);
        const auto DelayInterval = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[249]);

        on_func(Alertable, DelayInterval);
    });
}

opt<bpid_t> nt::syscalls::register_ZwDeleteDriverEntry(proc_t proc, const on_ZwDeleteDriverEntry_fn& on_func)
{
    return register_callback(*d_, proc, "ZwDeleteDriverEntry", [=]
    {
        auto& core = d_->core;

        const auto Id = arg<nt::ULONG>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[250]);

        on_func(Id);
    });
}

opt<bpid_t> nt::syscalls::register_ZwDeleteObjectAuditAlarm(proc_t proc, const on_ZwDeleteObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "ZwDeleteObjectAuditAlarm", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName   = arg<nt::PUNICODE_STRING>(core, 0);
        const auto HandleId        = arg<nt::PVOID>(core, 1);
        const auto GenerateOnClose = arg<nt::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[251]);

        on_func(SubsystemName, HandleId, GenerateOnClose);
    });
}

opt<bpid_t> nt::syscalls::register_ZwDeleteValueKey(proc_t proc, const on_ZwDeleteValueKey_fn& on_func)
{
    return register_callback(*d_, proc, "ZwDeleteValueKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle = arg<nt::HANDLE>(core, 0);
        const auto ValueName = arg<nt::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[252]);

        on_func(KeyHandle, ValueName);
    });
}

opt<bpid_t> nt::syscalls::register_ZwDisplayString(proc_t proc, const on_ZwDisplayString_fn& on_func)
{
    return register_callback(*d_, proc, "ZwDisplayString", [=]
    {
        auto& core = d_->core;

        const auto String = arg<nt::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[253]);

        on_func(String);
    });
}

opt<bpid_t> nt::syscalls::register_ZwDuplicateObject(proc_t proc, const on_ZwDuplicateObject_fn& on_func)
{
    return register_callback(*d_, proc, "ZwDuplicateObject", [=]
    {
        auto& core = d_->core;

        const auto SourceProcessHandle = arg<nt::HANDLE>(core, 0);
        const auto SourceHandle        = arg<nt::HANDLE>(core, 1);
        const auto TargetProcessHandle = arg<nt::HANDLE>(core, 2);
        const auto TargetHandle        = arg<nt::PHANDLE>(core, 3);
        const auto DesiredAccess       = arg<nt::ACCESS_MASK>(core, 4);
        const auto HandleAttributes    = arg<nt::ULONG>(core, 5);
        const auto Options             = arg<nt::ULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[254]);

        on_func(SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);
    });
}

opt<bpid_t> nt::syscalls::register_ZwEnumerateDriverEntries(proc_t proc, const on_ZwEnumerateDriverEntries_fn& on_func)
{
    return register_callback(*d_, proc, "ZwEnumerateDriverEntries", [=]
    {
        auto& core = d_->core;

        const auto Buffer       = arg<nt::PVOID>(core, 0);
        const auto BufferLength = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[255]);

        on_func(Buffer, BufferLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwEnumerateValueKey(proc_t proc, const on_ZwEnumerateValueKey_fn& on_func)
{
    return register_callback(*d_, proc, "ZwEnumerateValueKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle                = arg<nt::HANDLE>(core, 0);
        const auto Index                    = arg<nt::ULONG>(core, 1);
        const auto KeyValueInformationClass = arg<nt::KEY_VALUE_INFORMATION_CLASS>(core, 2);
        const auto KeyValueInformation      = arg<nt::PVOID>(core, 3);
        const auto Length                   = arg<nt::ULONG>(core, 4);
        const auto ResultLength             = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[256]);

        on_func(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwFilterToken(proc_t proc, const on_ZwFilterToken_fn& on_func)
{
    return register_callback(*d_, proc, "ZwFilterToken", [=]
    {
        auto& core = d_->core;

        const auto ExistingTokenHandle = arg<nt::HANDLE>(core, 0);
        const auto Flags               = arg<nt::ULONG>(core, 1);
        const auto SidsToDisable       = arg<nt::PTOKEN_GROUPS>(core, 2);
        const auto PrivilegesToDelete  = arg<nt::PTOKEN_PRIVILEGES>(core, 3);
        const auto RestrictedSids      = arg<nt::PTOKEN_GROUPS>(core, 4);
        const auto NewTokenHandle      = arg<nt::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[257]);

        on_func(ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwFindAtom(proc_t proc, const on_ZwFindAtom_fn& on_func)
{
    return register_callback(*d_, proc, "ZwFindAtom", [=]
    {
        auto& core = d_->core;

        const auto AtomName = arg<nt::PWSTR>(core, 0);
        const auto Length   = arg<nt::ULONG>(core, 1);
        const auto Atom     = arg<nt::PRTL_ATOM>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[258]);

        on_func(AtomName, Length, Atom);
    });
}

opt<bpid_t> nt::syscalls::register_ZwFlushBuffersFile(proc_t proc, const on_ZwFlushBuffersFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwFlushBuffersFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[259]);

        on_func(FileHandle, IoStatusBlock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwFlushKey(proc_t proc, const on_ZwFlushKey_fn& on_func)
{
    return register_callback(*d_, proc, "ZwFlushKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[260]);

        on_func(KeyHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwFlushProcessWriteBuffers(proc_t proc, const on_ZwFlushProcessWriteBuffers_fn& on_func)
{
    return register_callback(*d_, proc, "ZwFlushProcessWriteBuffers", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[261]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_ZwFlushVirtualMemory(proc_t proc, const on_ZwFlushVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "ZwFlushVirtualMemory", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<nt::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(core, 1);
        const auto RegionSize      = arg<nt::PSIZE_T>(core, 2);
        const auto IoStatus        = arg<nt::PIO_STATUS_BLOCK>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[262]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, IoStatus);
    });
}

opt<bpid_t> nt::syscalls::register_ZwFreeUserPhysicalPages(proc_t proc, const on_ZwFreeUserPhysicalPages_fn& on_func)
{
    return register_callback(*d_, proc, "ZwFreeUserPhysicalPages", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<nt::HANDLE>(core, 0);
        const auto NumberOfPages = arg<nt::PULONG_PTR>(core, 1);
        const auto UserPfnArra   = arg<nt::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[263]);

        on_func(ProcessHandle, NumberOfPages, UserPfnArra);
    });
}

opt<bpid_t> nt::syscalls::register_ZwFreeVirtualMemory(proc_t proc, const on_ZwFreeVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "ZwFreeVirtualMemory", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<nt::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(core, 1);
        const auto RegionSize      = arg<nt::PSIZE_T>(core, 2);
        const auto FreeType        = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[264]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, FreeType);
    });
}

opt<bpid_t> nt::syscalls::register_ZwFreezeTransactions(proc_t proc, const on_ZwFreezeTransactions_fn& on_func)
{
    return register_callback(*d_, proc, "ZwFreezeTransactions", [=]
    {
        auto& core = d_->core;

        const auto FreezeTimeout = arg<nt::PLARGE_INTEGER>(core, 0);
        const auto ThawTimeout   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[265]);

        on_func(FreezeTimeout, ThawTimeout);
    });
}

opt<bpid_t> nt::syscalls::register_ZwFsControlFile(proc_t proc, const on_ZwFsControlFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwFsControlFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle         = arg<nt::HANDLE>(core, 0);
        const auto Event              = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine         = arg<nt::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext         = arg<nt::PVOID>(core, 3);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core, 4);
        const auto IoControlCode      = arg<nt::ULONG>(core, 5);
        const auto InputBuffer        = arg<nt::PVOID>(core, 6);
        const auto InputBufferLength  = arg<nt::ULONG>(core, 7);
        const auto OutputBuffer       = arg<nt::PVOID>(core, 8);
        const auto OutputBufferLength = arg<nt::ULONG>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[266]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwGetContextThread(proc_t proc, const on_ZwGetContextThread_fn& on_func)
{
    return register_callback(*d_, proc, "ZwGetContextThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle  = arg<nt::HANDLE>(core, 0);
        const auto ThreadContext = arg<nt::PCONTEXT>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[267]);

        on_func(ThreadHandle, ThreadContext);
    });
}

opt<bpid_t> nt::syscalls::register_ZwGetCurrentProcessorNumber(proc_t proc, const on_ZwGetCurrentProcessorNumber_fn& on_func)
{
    return register_callback(*d_, proc, "ZwGetCurrentProcessorNumber", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[268]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_ZwGetDevicePowerState(proc_t proc, const on_ZwGetDevicePowerState_fn& on_func)
{
    return register_callback(*d_, proc, "ZwGetDevicePowerState", [=]
    {
        auto& core = d_->core;

        const auto Device    = arg<nt::HANDLE>(core, 0);
        const auto STARState = arg<nt::DEVICE_POWER_STATE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[269]);

        on_func(Device, STARState);
    });
}

opt<bpid_t> nt::syscalls::register_ZwGetMUIRegistryInfo(proc_t proc, const on_ZwGetMUIRegistryInfo_fn& on_func)
{
    return register_callback(*d_, proc, "ZwGetMUIRegistryInfo", [=]
    {
        auto& core = d_->core;

        const auto Flags    = arg<nt::ULONG>(core, 0);
        const auto DataSize = arg<nt::PULONG>(core, 1);
        const auto Data     = arg<nt::PVOID>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[270]);

        on_func(Flags, DataSize, Data);
    });
}

opt<bpid_t> nt::syscalls::register_ZwGetNextThread(proc_t proc, const on_ZwGetNextThread_fn& on_func)
{
    return register_callback(*d_, proc, "ZwGetNextThread", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle    = arg<nt::HANDLE>(core, 0);
        const auto ThreadHandle     = arg<nt::HANDLE>(core, 1);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 2);
        const auto HandleAttributes = arg<nt::ULONG>(core, 3);
        const auto Flags            = arg<nt::ULONG>(core, 4);
        const auto NewThreadHandle  = arg<nt::PHANDLE>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[271]);

        on_func(ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwGetNlsSectionPtr(proc_t proc, const on_ZwGetNlsSectionPtr_fn& on_func)
{
    return register_callback(*d_, proc, "ZwGetNlsSectionPtr", [=]
    {
        auto& core = d_->core;

        const auto SectionType        = arg<nt::ULONG>(core, 0);
        const auto SectionData        = arg<nt::ULONG>(core, 1);
        const auto ContextData        = arg<nt::PVOID>(core, 2);
        const auto STARSectionPointer = arg<nt::PVOID>(core, 3);
        const auto SectionSize        = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[272]);

        on_func(SectionType, SectionData, ContextData, STARSectionPointer, SectionSize);
    });
}

opt<bpid_t> nt::syscalls::register_ZwGetWriteWatch(proc_t proc, const on_ZwGetWriteWatch_fn& on_func)
{
    return register_callback(*d_, proc, "ZwGetWriteWatch", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle             = arg<nt::HANDLE>(core, 0);
        const auto Flags                     = arg<nt::ULONG>(core, 1);
        const auto BaseAddress               = arg<nt::PVOID>(core, 2);
        const auto RegionSize                = arg<nt::SIZE_T>(core, 3);
        const auto STARUserAddressArray      = arg<nt::PVOID>(core, 4);
        const auto EntriesInUserAddressArray = arg<nt::PULONG_PTR>(core, 5);
        const auto Granularity               = arg<nt::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[273]);

        on_func(ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity);
    });
}

opt<bpid_t> nt::syscalls::register_ZwImpersonateAnonymousToken(proc_t proc, const on_ZwImpersonateAnonymousToken_fn& on_func)
{
    return register_callback(*d_, proc, "ZwImpersonateAnonymousToken", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[274]);

        on_func(ThreadHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwImpersonateClientOfPort(proc_t proc, const on_ZwImpersonateClientOfPort_fn& on_func)
{
    return register_callback(*d_, proc, "ZwImpersonateClientOfPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle = arg<nt::HANDLE>(core, 0);
        const auto Message    = arg<nt::PPORT_MESSAGE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[275]);

        on_func(PortHandle, Message);
    });
}

opt<bpid_t> nt::syscalls::register_ZwImpersonateThread(proc_t proc, const on_ZwImpersonateThread_fn& on_func)
{
    return register_callback(*d_, proc, "ZwImpersonateThread", [=]
    {
        auto& core = d_->core;

        const auto ServerThreadHandle = arg<nt::HANDLE>(core, 0);
        const auto ClientThreadHandle = arg<nt::HANDLE>(core, 1);
        const auto SecurityQos        = arg<nt::PSECURITY_QUALITY_OF_SERVICE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[276]);

        on_func(ServerThreadHandle, ClientThreadHandle, SecurityQos);
    });
}

opt<bpid_t> nt::syscalls::register_ZwIsSystemResumeAutomatic(proc_t proc, const on_ZwIsSystemResumeAutomatic_fn& on_func)
{
    return register_callback(*d_, proc, "ZwIsSystemResumeAutomatic", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[277]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_ZwLoadDriver(proc_t proc, const on_ZwLoadDriver_fn& on_func)
{
    return register_callback(*d_, proc, "ZwLoadDriver", [=]
    {
        auto& core = d_->core;

        const auto DriverServiceName = arg<nt::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[278]);

        on_func(DriverServiceName);
    });
}

opt<bpid_t> nt::syscalls::register_ZwLoadKey2(proc_t proc, const on_ZwLoadKey2_fn& on_func)
{
    return register_callback(*d_, proc, "ZwLoadKey2", [=]
    {
        auto& core = d_->core;

        const auto TargetKey  = arg<nt::POBJECT_ATTRIBUTES>(core, 0);
        const auto SourceFile = arg<nt::POBJECT_ATTRIBUTES>(core, 1);
        const auto Flags      = arg<nt::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[279]);

        on_func(TargetKey, SourceFile, Flags);
    });
}

opt<bpid_t> nt::syscalls::register_ZwLockFile(proc_t proc, const on_ZwLockFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwLockFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle      = arg<nt::HANDLE>(core, 0);
        const auto Event           = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine      = arg<nt::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext      = arg<nt::PVOID>(core, 3);
        const auto IoStatusBlock   = arg<nt::PIO_STATUS_BLOCK>(core, 4);
        const auto ByteOffset      = arg<nt::PLARGE_INTEGER>(core, 5);
        const auto Length          = arg<nt::PLARGE_INTEGER>(core, 6);
        const auto Key             = arg<nt::ULONG>(core, 7);
        const auto FailImmediately = arg<nt::BOOLEAN>(core, 8);
        const auto ExclusiveLock   = arg<nt::BOOLEAN>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[280]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwLockRegistryKey(proc_t proc, const on_ZwLockRegistryKey_fn& on_func)
{
    return register_callback(*d_, proc, "ZwLockRegistryKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[281]);

        on_func(KeyHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwMapUserPhysicalPagesScatter(proc_t proc, const on_ZwMapUserPhysicalPagesScatter_fn& on_func)
{
    return register_callback(*d_, proc, "ZwMapUserPhysicalPagesScatter", [=]
    {
        auto& core = d_->core;

        const auto STARVirtualAddresses = arg<nt::PVOID>(core, 0);
        const auto NumberOfPages        = arg<nt::ULONG_PTR>(core, 1);
        const auto UserPfnArray         = arg<nt::PULONG_PTR>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[282]);

        on_func(STARVirtualAddresses, NumberOfPages, UserPfnArray);
    });
}

opt<bpid_t> nt::syscalls::register_ZwMapViewOfSection(proc_t proc, const on_ZwMapViewOfSection_fn& on_func)
{
    return register_callback(*d_, proc, "ZwMapViewOfSection", [=]
    {
        auto& core = d_->core;

        const auto SectionHandle      = arg<nt::HANDLE>(core, 0);
        const auto ProcessHandle      = arg<nt::HANDLE>(core, 1);
        const auto STARBaseAddress    = arg<nt::PVOID>(core, 2);
        const auto ZeroBits           = arg<nt::ULONG_PTR>(core, 3);
        const auto CommitSize         = arg<nt::SIZE_T>(core, 4);
        const auto SectionOffset      = arg<nt::PLARGE_INTEGER>(core, 5);
        const auto ViewSize           = arg<nt::PSIZE_T>(core, 6);
        const auto InheritDisposition = arg<nt::SECTION_INHERIT>(core, 7);
        const auto AllocationType     = arg<nt::ULONG>(core, 8);
        const auto Win32Protect       = arg<nt::WIN32_PROTECTION_MASK>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[283]);

        on_func(SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
    });
}

opt<bpid_t> nt::syscalls::register_ZwModifyDriverEntry(proc_t proc, const on_ZwModifyDriverEntry_fn& on_func)
{
    return register_callback(*d_, proc, "ZwModifyDriverEntry", [=]
    {
        auto& core = d_->core;

        const auto DriverEntry = arg<nt::PEFI_DRIVER_ENTRY>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[284]);

        on_func(DriverEntry);
    });
}

opt<bpid_t> nt::syscalls::register_ZwNotifyChangeKey(proc_t proc, const on_ZwNotifyChangeKey_fn& on_func)
{
    return register_callback(*d_, proc, "ZwNotifyChangeKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle        = arg<nt::HANDLE>(core, 0);
        const auto Event            = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine       = arg<nt::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext       = arg<nt::PVOID>(core, 3);
        const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(core, 4);
        const auto CompletionFilter = arg<nt::ULONG>(core, 5);
        const auto WatchTree        = arg<nt::BOOLEAN>(core, 6);
        const auto Buffer           = arg<nt::PVOID>(core, 7);
        const auto BufferSize       = arg<nt::ULONG>(core, 8);
        const auto Asynchronous     = arg<nt::BOOLEAN>(core, 9);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[285]);

        on_func(KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenDirectoryObject(proc_t proc, const on_ZwOpenDirectoryObject_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenDirectoryObject", [=]
    {
        auto& core = d_->core;

        const auto DirectoryHandle  = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[286]);

        on_func(DirectoryHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenEventPair(proc_t proc, const on_ZwOpenEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenEventPair", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle  = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[287]);

        on_func(EventPairHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenKeyTransacted(proc_t proc, const on_ZwOpenKeyTransacted_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenKeyTransacted", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle         = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto TransactionHandle = arg<nt::HANDLE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[288]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenKeyTransactedEx(proc_t proc, const on_ZwOpenKeyTransactedEx_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenKeyTransactedEx", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle         = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto OpenOptions       = arg<nt::ULONG>(core, 3);
        const auto TransactionHandle = arg<nt::HANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[289]);

        on_func(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenObjectAuditAlarm(proc_t proc, const on_ZwOpenObjectAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenObjectAuditAlarm", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName      = arg<nt::PUNICODE_STRING>(core, 0);
        const auto HandleId           = arg<nt::PVOID>(core, 1);
        const auto ObjectTypeName     = arg<nt::PUNICODE_STRING>(core, 2);
        const auto ObjectName         = arg<nt::PUNICODE_STRING>(core, 3);
        const auto SecurityDescriptor = arg<nt::PSECURITY_DESCRIPTOR>(core, 4);
        const auto ClientToken        = arg<nt::HANDLE>(core, 5);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core, 6);
        const auto GrantedAccess      = arg<nt::ACCESS_MASK>(core, 7);
        const auto Privileges         = arg<nt::PPRIVILEGE_SET>(core, 8);
        const auto ObjectCreation     = arg<nt::BOOLEAN>(core, 9);
        const auto AccessGranted      = arg<nt::BOOLEAN>(core, 10);
        const auto GenerateOnClose    = arg<nt::PBOOLEAN>(core, 11);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[290]);

        on_func(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenSemaphore(proc_t proc, const on_ZwOpenSemaphore_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenSemaphore", [=]
    {
        auto& core = d_->core;

        const auto SemaphoreHandle  = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[291]);

        on_func(SemaphoreHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenSession(proc_t proc, const on_ZwOpenSession_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenSession", [=]
    {
        auto& core = d_->core;

        const auto SessionHandle    = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[292]);

        on_func(SessionHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenSymbolicLinkObject(proc_t proc, const on_ZwOpenSymbolicLinkObject_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenSymbolicLinkObject", [=]
    {
        auto& core = d_->core;

        const auto LinkHandle       = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[293]);

        on_func(LinkHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenThread(proc_t proc, const on_ZwOpenThread_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle     = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto ClientId         = arg<nt::PCLIENT_ID>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[294]);

        on_func(ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenTimer(proc_t proc, const on_ZwOpenTimer_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenTimer", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle      = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[295]);

        on_func(TimerHandle, DesiredAccess, ObjectAttributes);
    });
}

opt<bpid_t> nt::syscalls::register_ZwOpenTransaction(proc_t proc, const on_ZwOpenTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "ZwOpenTransaction", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle = arg<nt::PHANDLE>(core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core, 2);
        const auto Uow               = arg<nt::LPGUID>(core, 3);
        const auto TmHandle          = arg<nt::HANDLE>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[296]);

        on_func(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwPlugPlayControl(proc_t proc, const on_ZwPlugPlayControl_fn& on_func)
{
    return register_callback(*d_, proc, "ZwPlugPlayControl", [=]
    {
        auto& core = d_->core;

        const auto PnPControlClass      = arg<nt::PLUGPLAY_CONTROL_CLASS>(core, 0);
        const auto PnPControlData       = arg<nt::PVOID>(core, 1);
        const auto PnPControlDataLength = arg<nt::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[297]);

        on_func(PnPControlClass, PnPControlData, PnPControlDataLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwPrePrepareComplete(proc_t proc, const on_ZwPrePrepareComplete_fn& on_func)
{
    return register_callback(*d_, proc, "ZwPrePrepareComplete", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[298]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwPrepareComplete(proc_t proc, const on_ZwPrepareComplete_fn& on_func)
{
    return register_callback(*d_, proc, "ZwPrepareComplete", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[299]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwPrepareEnlistment(proc_t proc, const on_ZwPrepareEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "ZwPrepareEnlistment", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[300]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwPrivilegeCheck(proc_t proc, const on_ZwPrivilegeCheck_fn& on_func)
{
    return register_callback(*d_, proc, "ZwPrivilegeCheck", [=]
    {
        auto& core = d_->core;

        const auto ClientToken        = arg<nt::HANDLE>(core, 0);
        const auto RequiredPrivileges = arg<nt::PPRIVILEGE_SET>(core, 1);
        const auto Result             = arg<nt::PBOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[301]);

        on_func(ClientToken, RequiredPrivileges, Result);
    });
}

opt<bpid_t> nt::syscalls::register_ZwPrivilegedServiceAuditAlarm(proc_t proc, const on_ZwPrivilegedServiceAuditAlarm_fn& on_func)
{
    return register_callback(*d_, proc, "ZwPrivilegedServiceAuditAlarm", [=]
    {
        auto& core = d_->core;

        const auto SubsystemName = arg<nt::PUNICODE_STRING>(core, 0);
        const auto ServiceName   = arg<nt::PUNICODE_STRING>(core, 1);
        const auto ClientToken   = arg<nt::HANDLE>(core, 2);
        const auto Privileges    = arg<nt::PPRIVILEGE_SET>(core, 3);
        const auto AccessGranted = arg<nt::BOOLEAN>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[302]);

        on_func(SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted);
    });
}

opt<bpid_t> nt::syscalls::register_ZwProtectVirtualMemory(proc_t proc, const on_ZwProtectVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "ZwProtectVirtualMemory", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<nt::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(core, 1);
        const auto RegionSize      = arg<nt::PSIZE_T>(core, 2);
        const auto NewProtectWin32 = arg<nt::WIN32_PROTECTION_MASK>(core, 3);
        const auto OldProtect      = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[303]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect);
    });
}

opt<bpid_t> nt::syscalls::register_ZwPulseEvent(proc_t proc, const on_ZwPulseEvent_fn& on_func)
{
    return register_callback(*d_, proc, "ZwPulseEvent", [=]
    {
        auto& core = d_->core;

        const auto EventHandle   = arg<nt::HANDLE>(core, 0);
        const auto PreviousState = arg<nt::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[304]);

        on_func(EventHandle, PreviousState);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryAttributesFile(proc_t proc, const on_ZwQueryAttributesFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryAttributesFile", [=]
    {
        auto& core = d_->core;

        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core, 0);
        const auto FileInformation  = arg<nt::PFILE_BASIC_INFORMATION>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[305]);

        on_func(ObjectAttributes, FileInformation);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryBootEntryOrder(proc_t proc, const on_ZwQueryBootEntryOrder_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryBootEntryOrder", [=]
    {
        auto& core = d_->core;

        const auto Ids   = arg<nt::PULONG>(core, 0);
        const auto Count = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[306]);

        on_func(Ids, Count);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryDebugFilterState(proc_t proc, const on_ZwQueryDebugFilterState_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryDebugFilterState", [=]
    {
        auto& core = d_->core;

        const auto ComponentId = arg<nt::ULONG>(core, 0);
        const auto Level       = arg<nt::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[307]);

        on_func(ComponentId, Level);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryDefaultUILanguage(proc_t proc, const on_ZwQueryDefaultUILanguage_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryDefaultUILanguage", [=]
    {
        auto& core = d_->core;

        const auto STARDefaultUILanguageId = arg<nt::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[308]);

        on_func(STARDefaultUILanguageId);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryDirectoryObject(proc_t proc, const on_ZwQueryDirectoryObject_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryDirectoryObject", [=]
    {
        auto& core = d_->core;

        const auto DirectoryHandle   = arg<nt::HANDLE>(core, 0);
        const auto Buffer            = arg<nt::PVOID>(core, 1);
        const auto Length            = arg<nt::ULONG>(core, 2);
        const auto ReturnSingleEntry = arg<nt::BOOLEAN>(core, 3);
        const auto RestartScan       = arg<nt::BOOLEAN>(core, 4);
        const auto Context           = arg<nt::PULONG>(core, 5);
        const auto ReturnLength      = arg<nt::PULONG>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[309]);

        on_func(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryEaFile(proc_t proc, const on_ZwQueryEaFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryEaFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle        = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer            = arg<nt::PVOID>(core, 2);
        const auto Length            = arg<nt::ULONG>(core, 3);
        const auto ReturnSingleEntry = arg<nt::BOOLEAN>(core, 4);
        const auto EaList            = arg<nt::PVOID>(core, 5);
        const auto EaListLength      = arg<nt::ULONG>(core, 6);
        const auto EaIndex           = arg<nt::PULONG>(core, 7);
        const auto RestartScan       = arg<nt::BOOLEAN>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[310]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryInformationThread(proc_t proc, const on_ZwQueryInformationThread_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryInformationThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle            = arg<nt::HANDLE>(core, 0);
        const auto ThreadInformationClass  = arg<nt::THREADINFOCLASS>(core, 1);
        const auto ThreadInformation       = arg<nt::PVOID>(core, 2);
        const auto ThreadInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength            = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[311]);

        on_func(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryInformationTransaction(proc_t proc, const on_ZwQueryInformationTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryInformationTransaction", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle            = arg<nt::HANDLE>(core, 0);
        const auto TransactionInformationClass  = arg<nt::TRANSACTION_INFORMATION_CLASS>(core, 1);
        const auto TransactionInformation       = arg<nt::PVOID>(core, 2);
        const auto TransactionInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength                 = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[312]);

        on_func(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryInformationWorkerFactory(proc_t proc, const on_ZwQueryInformationWorkerFactory_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryInformationWorkerFactory", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle            = arg<nt::HANDLE>(core, 0);
        const auto WorkerFactoryInformationClass  = arg<nt::WORKERFACTORYINFOCLASS>(core, 1);
        const auto WorkerFactoryInformation       = arg<nt::PVOID>(core, 2);
        const auto WorkerFactoryInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength                   = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[313]);

        on_func(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryKey(proc_t proc, const on_ZwQueryKey_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle           = arg<nt::HANDLE>(core, 0);
        const auto KeyInformationClass = arg<nt::KEY_INFORMATION_CLASS>(core, 1);
        const auto KeyInformation      = arg<nt::PVOID>(core, 2);
        const auto Length              = arg<nt::ULONG>(core, 3);
        const auto ResultLength        = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[314]);

        on_func(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryLicenseValue(proc_t proc, const on_ZwQueryLicenseValue_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryLicenseValue", [=]
    {
        auto& core = d_->core;

        const auto Name           = arg<nt::PUNICODE_STRING>(core, 0);
        const auto Type           = arg<nt::PULONG>(core, 1);
        const auto Buffer         = arg<nt::PVOID>(core, 2);
        const auto Length         = arg<nt::ULONG>(core, 3);
        const auto ReturnedLength = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[315]);

        on_func(Name, Type, Buffer, Length, ReturnedLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryMultipleValueKey(proc_t proc, const on_ZwQueryMultipleValueKey_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryMultipleValueKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle            = arg<nt::HANDLE>(core, 0);
        const auto ValueEntries         = arg<nt::PKEY_VALUE_ENTRY>(core, 1);
        const auto EntryCount           = arg<nt::ULONG>(core, 2);
        const auto ValueBuffer          = arg<nt::PVOID>(core, 3);
        const auto BufferLength         = arg<nt::PULONG>(core, 4);
        const auto RequiredBufferLength = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[316]);

        on_func(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryMutant(proc_t proc, const on_ZwQueryMutant_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryMutant", [=]
    {
        auto& core = d_->core;

        const auto MutantHandle            = arg<nt::HANDLE>(core, 0);
        const auto MutantInformationClass  = arg<nt::MUTANT_INFORMATION_CLASS>(core, 1);
        const auto MutantInformation       = arg<nt::PVOID>(core, 2);
        const auto MutantInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength            = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[317]);

        on_func(MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryObject(proc_t proc, const on_ZwQueryObject_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryObject", [=]
    {
        auto& core = d_->core;

        const auto Handle                  = arg<nt::HANDLE>(core, 0);
        const auto ObjectInformationClass  = arg<nt::OBJECT_INFORMATION_CLASS>(core, 1);
        const auto ObjectInformation       = arg<nt::PVOID>(core, 2);
        const auto ObjectInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength            = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[318]);

        on_func(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQuerySecurityAttributesToken(proc_t proc, const on_ZwQuerySecurityAttributesToken_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQuerySecurityAttributesToken", [=]
    {
        auto& core = d_->core;

        const auto TokenHandle        = arg<nt::HANDLE>(core, 0);
        const auto Attributes         = arg<nt::PUNICODE_STRING>(core, 1);
        const auto NumberOfAttributes = arg<nt::ULONG>(core, 2);
        const auto Buffer             = arg<nt::PVOID>(core, 3);
        const auto Length             = arg<nt::ULONG>(core, 4);
        const auto ReturnLength       = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[319]);

        on_func(TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQuerySemaphore(proc_t proc, const on_ZwQuerySemaphore_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQuerySemaphore", [=]
    {
        auto& core = d_->core;

        const auto SemaphoreHandle            = arg<nt::HANDLE>(core, 0);
        const auto SemaphoreInformationClass  = arg<nt::SEMAPHORE_INFORMATION_CLASS>(core, 1);
        const auto SemaphoreInformation       = arg<nt::PVOID>(core, 2);
        const auto SemaphoreInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength               = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[320]);

        on_func(SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQuerySystemTime(proc_t proc, const on_ZwQuerySystemTime_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQuerySystemTime", [=]
    {
        auto& core = d_->core;

        const auto SystemTime = arg<nt::PLARGE_INTEGER>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[321]);

        on_func(SystemTime);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryTimer(proc_t proc, const on_ZwQueryTimer_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryTimer", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle            = arg<nt::HANDLE>(core, 0);
        const auto TimerInformationClass  = arg<nt::TIMER_INFORMATION_CLASS>(core, 1);
        const auto TimerInformation       = arg<nt::PVOID>(core, 2);
        const auto TimerInformationLength = arg<nt::ULONG>(core, 3);
        const auto ReturnLength           = arg<nt::PULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[322]);

        on_func(TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryTimerResolution(proc_t proc, const on_ZwQueryTimerResolution_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryTimerResolution", [=]
    {
        auto& core = d_->core;

        const auto MaximumTime = arg<nt::PULONG>(core, 0);
        const auto MinimumTime = arg<nt::PULONG>(core, 1);
        const auto CurrentTime = arg<nt::PULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[323]);

        on_func(MaximumTime, MinimumTime, CurrentTime);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryVirtualMemory(proc_t proc, const on_ZwQueryVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryVirtualMemory", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle           = arg<nt::HANDLE>(core, 0);
        const auto BaseAddress             = arg<nt::PVOID>(core, 1);
        const auto MemoryInformationClass  = arg<nt::MEMORY_INFORMATION_CLASS>(core, 2);
        const auto MemoryInformation       = arg<nt::PVOID>(core, 3);
        const auto MemoryInformationLength = arg<nt::SIZE_T>(core, 4);
        const auto ReturnLength            = arg<nt::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[324]);

        on_func(ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwQueryVolumeInformationFile(proc_t proc, const on_ZwQueryVolumeInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwQueryVolumeInformationFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle         = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto FsInformation      = arg<nt::PVOID>(core, 2);
        const auto Length             = arg<nt::ULONG>(core, 3);
        const auto FsInformationClass = arg<nt::FS_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[325]);

        on_func(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    });
}

opt<bpid_t> nt::syscalls::register_ZwRaiseException(proc_t proc, const on_ZwRaiseException_fn& on_func)
{
    return register_callback(*d_, proc, "ZwRaiseException", [=]
    {
        auto& core = d_->core;

        const auto ExceptionRecord = arg<nt::PEXCEPTION_RECORD>(core, 0);
        const auto ContextRecord   = arg<nt::PCONTEXT>(core, 1);
        const auto FirstChance     = arg<nt::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[326]);

        on_func(ExceptionRecord, ContextRecord, FirstChance);
    });
}

opt<bpid_t> nt::syscalls::register_ZwReadFileScatter(proc_t proc, const on_ZwReadFileScatter_fn& on_func)
{
    return register_callback(*d_, proc, "ZwReadFileScatter", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<nt::HANDLE>(core, 0);
        const auto Event         = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core, 4);
        const auto SegmentArray  = arg<nt::PFILE_SEGMENT_ELEMENT>(core, 5);
        const auto Length        = arg<nt::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[327]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    });
}

opt<bpid_t> nt::syscalls::register_ZwReadOnlyEnlistment(proc_t proc, const on_ZwReadOnlyEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "ZwReadOnlyEnlistment", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[328]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwReadVirtualMemory(proc_t proc, const on_ZwReadVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "ZwReadVirtualMemory", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle     = arg<nt::HANDLE>(core, 0);
        const auto BaseAddress       = arg<nt::PVOID>(core, 1);
        const auto Buffer            = arg<nt::PVOID>(core, 2);
        const auto BufferSize        = arg<nt::SIZE_T>(core, 3);
        const auto NumberOfBytesRead = arg<nt::PSIZE_T>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[329]);

        on_func(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead);
    });
}

opt<bpid_t> nt::syscalls::register_ZwRecoverEnlistment(proc_t proc, const on_ZwRecoverEnlistment_fn& on_func)
{
    return register_callback(*d_, proc, "ZwRecoverEnlistment", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto EnlistmentKey    = arg<nt::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[330]);

        on_func(EnlistmentHandle, EnlistmentKey);
    });
}

opt<bpid_t> nt::syscalls::register_ZwRecoverResourceManager(proc_t proc, const on_ZwRecoverResourceManager_fn& on_func)
{
    return register_callback(*d_, proc, "ZwRecoverResourceManager", [=]
    {
        auto& core = d_->core;

        const auto ResourceManagerHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[331]);

        on_func(ResourceManagerHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwReleaseMutant(proc_t proc, const on_ZwReleaseMutant_fn& on_func)
{
    return register_callback(*d_, proc, "ZwReleaseMutant", [=]
    {
        auto& core = d_->core;

        const auto MutantHandle  = arg<nt::HANDLE>(core, 0);
        const auto PreviousCount = arg<nt::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[332]);

        on_func(MutantHandle, PreviousCount);
    });
}

opt<bpid_t> nt::syscalls::register_ZwReleaseSemaphore(proc_t proc, const on_ZwReleaseSemaphore_fn& on_func)
{
    return register_callback(*d_, proc, "ZwReleaseSemaphore", [=]
    {
        auto& core = d_->core;

        const auto SemaphoreHandle = arg<nt::HANDLE>(core, 0);
        const auto ReleaseCount    = arg<nt::LONG>(core, 1);
        const auto PreviousCount   = arg<nt::PLONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[333]);

        on_func(SemaphoreHandle, ReleaseCount, PreviousCount);
    });
}

opt<bpid_t> nt::syscalls::register_ZwRemoveIoCompletion(proc_t proc, const on_ZwRemoveIoCompletion_fn& on_func)
{
    return register_callback(*d_, proc, "ZwRemoveIoCompletion", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle = arg<nt::HANDLE>(core, 0);
        const auto STARKeyContext     = arg<nt::PVOID>(core, 1);
        const auto STARApcContext     = arg<nt::PVOID>(core, 2);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core, 3);
        const auto Timeout            = arg<nt::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[334]);

        on_func(IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout);
    });
}

opt<bpid_t> nt::syscalls::register_ZwRemoveIoCompletionEx(proc_t proc, const on_ZwRemoveIoCompletionEx_fn& on_func)
{
    return register_callback(*d_, proc, "ZwRemoveIoCompletionEx", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle      = arg<nt::HANDLE>(core, 0);
        const auto IoCompletionInformation = arg<nt::PFILE_IO_COMPLETION_INFORMATION>(core, 1);
        const auto Count                   = arg<nt::ULONG>(core, 2);
        const auto NumEntriesRemoved       = arg<nt::PULONG>(core, 3);
        const auto Timeout                 = arg<nt::PLARGE_INTEGER>(core, 4);
        const auto Alertable               = arg<nt::BOOLEAN>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[335]);

        on_func(IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable);
    });
}

opt<bpid_t> nt::syscalls::register_ZwRemoveProcessDebug(proc_t proc, const on_ZwRemoveProcessDebug_fn& on_func)
{
    return register_callback(*d_, proc, "ZwRemoveProcessDebug", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle     = arg<nt::HANDLE>(core, 0);
        const auto DebugObjectHandle = arg<nt::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[336]);

        on_func(ProcessHandle, DebugObjectHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwReplyWaitReceivePort(proc_t proc, const on_ZwReplyWaitReceivePort_fn& on_func)
{
    return register_callback(*d_, proc, "ZwReplyWaitReceivePort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle      = arg<nt::HANDLE>(core, 0);
        const auto STARPortContext = arg<nt::PVOID>(core, 1);
        const auto ReplyMessage    = arg<nt::PPORT_MESSAGE>(core, 2);
        const auto ReceiveMessage  = arg<nt::PPORT_MESSAGE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[337]);

        on_func(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage);
    });
}

opt<bpid_t> nt::syscalls::register_ZwReplyWaitReceivePortEx(proc_t proc, const on_ZwReplyWaitReceivePortEx_fn& on_func)
{
    return register_callback(*d_, proc, "ZwReplyWaitReceivePortEx", [=]
    {
        auto& core = d_->core;

        const auto PortHandle      = arg<nt::HANDLE>(core, 0);
        const auto STARPortContext = arg<nt::PVOID>(core, 1);
        const auto ReplyMessage    = arg<nt::PPORT_MESSAGE>(core, 2);
        const auto ReceiveMessage  = arg<nt::PPORT_MESSAGE>(core, 3);
        const auto Timeout         = arg<nt::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[338]);

        on_func(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout);
    });
}

opt<bpid_t> nt::syscalls::register_ZwRequestWaitReplyPort(proc_t proc, const on_ZwRequestWaitReplyPort_fn& on_func)
{
    return register_callback(*d_, proc, "ZwRequestWaitReplyPort", [=]
    {
        auto& core = d_->core;

        const auto PortHandle     = arg<nt::HANDLE>(core, 0);
        const auto RequestMessage = arg<nt::PPORT_MESSAGE>(core, 1);
        const auto ReplyMessage   = arg<nt::PPORT_MESSAGE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[339]);

        on_func(PortHandle, RequestMessage, ReplyMessage);
    });
}

opt<bpid_t> nt::syscalls::register_ZwResetEvent(proc_t proc, const on_ZwResetEvent_fn& on_func)
{
    return register_callback(*d_, proc, "ZwResetEvent", [=]
    {
        auto& core = d_->core;

        const auto EventHandle   = arg<nt::HANDLE>(core, 0);
        const auto PreviousState = arg<nt::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[340]);

        on_func(EventHandle, PreviousState);
    });
}

opt<bpid_t> nt::syscalls::register_ZwResetWriteWatch(proc_t proc, const on_ZwResetWriteWatch_fn& on_func)
{
    return register_callback(*d_, proc, "ZwResetWriteWatch", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<nt::HANDLE>(core, 0);
        const auto BaseAddress   = arg<nt::PVOID>(core, 1);
        const auto RegionSize    = arg<nt::SIZE_T>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[341]);

        on_func(ProcessHandle, BaseAddress, RegionSize);
    });
}

opt<bpid_t> nt::syscalls::register_ZwResumeProcess(proc_t proc, const on_ZwResumeProcess_fn& on_func)
{
    return register_callback(*d_, proc, "ZwResumeProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[342]);

        on_func(ProcessHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwResumeThread(proc_t proc, const on_ZwResumeThread_fn& on_func)
{
    return register_callback(*d_, proc, "ZwResumeThread", [=]
    {
        auto& core = d_->core;

        const auto ThreadHandle         = arg<nt::HANDLE>(core, 0);
        const auto PreviousSuspendCount = arg<nt::PULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[343]);

        on_func(ThreadHandle, PreviousSuspendCount);
    });
}

opt<bpid_t> nt::syscalls::register_ZwRollbackComplete(proc_t proc, const on_ZwRollbackComplete_fn& on_func)
{
    return register_callback(*d_, proc, "ZwRollbackComplete", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[344]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwRollbackTransaction(proc_t proc, const on_ZwRollbackTransaction_fn& on_func)
{
    return register_callback(*d_, proc, "ZwRollbackTransaction", [=]
    {
        auto& core = d_->core;

        const auto TransactionHandle = arg<nt::HANDLE>(core, 0);
        const auto Wait              = arg<nt::BOOLEAN>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[345]);

        on_func(TransactionHandle, Wait);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSaveMergedKeys(proc_t proc, const on_ZwSaveMergedKeys_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSaveMergedKeys", [=]
    {
        auto& core = d_->core;

        const auto HighPrecedenceKeyHandle = arg<nt::HANDLE>(core, 0);
        const auto LowPrecedenceKeyHandle  = arg<nt::HANDLE>(core, 1);
        const auto FileHandle              = arg<nt::HANDLE>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[346]);

        on_func(HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSerializeBoot(proc_t proc, const on_ZwSerializeBoot_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSerializeBoot", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[347]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetBootEntryOrder(proc_t proc, const on_ZwSetBootEntryOrder_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetBootEntryOrder", [=]
    {
        auto& core = d_->core;

        const auto Ids   = arg<nt::PULONG>(core, 0);
        const auto Count = arg<nt::ULONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[348]);

        on_func(Ids, Count);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetDebugFilterState(proc_t proc, const on_ZwSetDebugFilterState_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetDebugFilterState", [=]
    {
        auto& core = d_->core;

        const auto ComponentId = arg<nt::ULONG>(core, 0);
        const auto Level       = arg<nt::ULONG>(core, 1);
        const auto State       = arg<nt::BOOLEAN>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[349]);

        on_func(ComponentId, Level, State);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetDefaultUILanguage(proc_t proc, const on_ZwSetDefaultUILanguage_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetDefaultUILanguage", [=]
    {
        auto& core = d_->core;

        const auto DefaultUILanguageId = arg<nt::LANGID>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[350]);

        on_func(DefaultUILanguageId);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetEaFile(proc_t proc, const on_ZwSetEaFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetEaFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer        = arg<nt::PVOID>(core, 2);
        const auto Length        = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[351]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetEvent(proc_t proc, const on_ZwSetEvent_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetEvent", [=]
    {
        auto& core = d_->core;

        const auto EventHandle   = arg<nt::HANDLE>(core, 0);
        const auto PreviousState = arg<nt::PLONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[352]);

        on_func(EventHandle, PreviousState);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetEventBoostPriority(proc_t proc, const on_ZwSetEventBoostPriority_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetEventBoostPriority", [=]
    {
        auto& core = d_->core;

        const auto EventHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[353]);

        on_func(EventHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetHighWaitLowEventPair(proc_t proc, const on_ZwSetHighWaitLowEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetHighWaitLowEventPair", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[354]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetInformationFile(proc_t proc, const on_ZwSetInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetInformationFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle           = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock        = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto FileInformation      = arg<nt::PVOID>(core, 2);
        const auto Length               = arg<nt::ULONG>(core, 3);
        const auto FileInformationClass = arg<nt::FILE_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[355]);

        on_func(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetInformationJobObject(proc_t proc, const on_ZwSetInformationJobObject_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetInformationJobObject", [=]
    {
        auto& core = d_->core;

        const auto JobHandle                  = arg<nt::HANDLE>(core, 0);
        const auto JobObjectInformationClass  = arg<nt::JOBOBJECTINFOCLASS>(core, 1);
        const auto JobObjectInformation       = arg<nt::PVOID>(core, 2);
        const auto JobObjectInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[356]);

        on_func(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetInformationKey(proc_t proc, const on_ZwSetInformationKey_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetInformationKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle               = arg<nt::HANDLE>(core, 0);
        const auto KeySetInformationClass  = arg<nt::KEY_SET_INFORMATION_CLASS>(core, 1);
        const auto KeySetInformation       = arg<nt::PVOID>(core, 2);
        const auto KeySetInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[357]);

        on_func(KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetInformationTransactionManager(proc_t proc, const on_ZwSetInformationTransactionManager_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetInformationTransactionManager", [=]
    {
        auto& core = d_->core;

        const auto TmHandle                            = arg<nt::HANDLE>(core, 0);
        const auto TransactionManagerInformationClass  = arg<nt::TRANSACTIONMANAGER_INFORMATION_CLASS>(core, 1);
        const auto TransactionManagerInformation       = arg<nt::PVOID>(core, 2);
        const auto TransactionManagerInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[358]);

        on_func(TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetIoCompletion(proc_t proc, const on_ZwSetIoCompletion_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetIoCompletion", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle  = arg<nt::HANDLE>(core, 0);
        const auto KeyContext          = arg<nt::PVOID>(core, 1);
        const auto ApcContext          = arg<nt::PVOID>(core, 2);
        const auto IoStatus            = arg<nt::NTSTATUS>(core, 3);
        const auto IoStatusInformation = arg<nt::ULONG_PTR>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[359]);

        on_func(IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetIoCompletionEx(proc_t proc, const on_ZwSetIoCompletionEx_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetIoCompletionEx", [=]
    {
        auto& core = d_->core;

        const auto IoCompletionHandle        = arg<nt::HANDLE>(core, 0);
        const auto IoCompletionReserveHandle = arg<nt::HANDLE>(core, 1);
        const auto KeyContext                = arg<nt::PVOID>(core, 2);
        const auto ApcContext                = arg<nt::PVOID>(core, 3);
        const auto IoStatus                  = arg<nt::NTSTATUS>(core, 4);
        const auto IoStatusInformation       = arg<nt::ULONG_PTR>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[360]);

        on_func(IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetLdtEntries(proc_t proc, const on_ZwSetLdtEntries_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetLdtEntries", [=]
    {
        auto& core = d_->core;

        const auto Selector0 = arg<nt::ULONG>(core, 0);
        const auto Entry0Low = arg<nt::ULONG>(core, 1);
        const auto Entry0Hi  = arg<nt::ULONG>(core, 2);
        const auto Selector1 = arg<nt::ULONG>(core, 3);
        const auto Entry1Low = arg<nt::ULONG>(core, 4);
        const auto Entry1Hi  = arg<nt::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[361]);

        on_func(Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetLowEventPair(proc_t proc, const on_ZwSetLowEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetLowEventPair", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[362]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetLowWaitHighEventPair(proc_t proc, const on_ZwSetLowWaitHighEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetLowWaitHighEventPair", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[363]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetQuotaInformationFile(proc_t proc, const on_ZwSetQuotaInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetQuotaInformationFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto Buffer        = arg<nt::PVOID>(core, 2);
        const auto Length        = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[364]);

        on_func(FileHandle, IoStatusBlock, Buffer, Length);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetSystemEnvironmentValue(proc_t proc, const on_ZwSetSystemEnvironmentValue_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetSystemEnvironmentValue", [=]
    {
        auto& core = d_->core;

        const auto VariableName  = arg<nt::PUNICODE_STRING>(core, 0);
        const auto VariableValue = arg<nt::PUNICODE_STRING>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[365]);

        on_func(VariableName, VariableValue);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetSystemEnvironmentValueEx(proc_t proc, const on_ZwSetSystemEnvironmentValueEx_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetSystemEnvironmentValueEx", [=]
    {
        auto& core = d_->core;

        const auto VariableName = arg<nt::PUNICODE_STRING>(core, 0);
        const auto VendorGuid   = arg<nt::LPGUID>(core, 1);
        const auto Value        = arg<nt::PVOID>(core, 2);
        const auto ValueLength  = arg<nt::ULONG>(core, 3);
        const auto Attributes   = arg<nt::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[366]);

        on_func(VariableName, VendorGuid, Value, ValueLength, Attributes);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetSystemInformation(proc_t proc, const on_ZwSetSystemInformation_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetSystemInformation", [=]
    {
        auto& core = d_->core;

        const auto SystemInformationClass  = arg<nt::SYSTEM_INFORMATION_CLASS>(core, 0);
        const auto SystemInformation       = arg<nt::PVOID>(core, 1);
        const auto SystemInformationLength = arg<nt::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[367]);

        on_func(SystemInformationClass, SystemInformation, SystemInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetSystemPowerState(proc_t proc, const on_ZwSetSystemPowerState_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetSystemPowerState", [=]
    {
        auto& core = d_->core;

        const auto SystemAction   = arg<nt::POWER_ACTION>(core, 0);
        const auto MinSystemState = arg<nt::SYSTEM_POWER_STATE>(core, 1);
        const auto Flags          = arg<nt::ULONG>(core, 2);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[368]);

        on_func(SystemAction, MinSystemState, Flags);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetSystemTime(proc_t proc, const on_ZwSetSystemTime_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetSystemTime", [=]
    {
        auto& core = d_->core;

        const auto SystemTime   = arg<nt::PLARGE_INTEGER>(core, 0);
        const auto PreviousTime = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[369]);

        on_func(SystemTime, PreviousTime);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetTimer(proc_t proc, const on_ZwSetTimer_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetTimer", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle     = arg<nt::HANDLE>(core, 0);
        const auto DueTime         = arg<nt::PLARGE_INTEGER>(core, 1);
        const auto TimerApcRoutine = arg<nt::PTIMER_APC_ROUTINE>(core, 2);
        const auto TimerContext    = arg<nt::PVOID>(core, 3);
        const auto WakeTimer       = arg<nt::BOOLEAN>(core, 4);
        const auto Period          = arg<nt::LONG>(core, 5);
        const auto PreviousState   = arg<nt::PBOOLEAN>(core, 6);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[370]);

        on_func(TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetTimerEx(proc_t proc, const on_ZwSetTimerEx_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetTimerEx", [=]
    {
        auto& core = d_->core;

        const auto TimerHandle               = arg<nt::HANDLE>(core, 0);
        const auto TimerSetInformationClass  = arg<nt::TIMER_SET_INFORMATION_CLASS>(core, 1);
        const auto TimerSetInformation       = arg<nt::PVOID>(core, 2);
        const auto TimerSetInformationLength = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[371]);

        on_func(TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetValueKey(proc_t proc, const on_ZwSetValueKey_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetValueKey", [=]
    {
        auto& core = d_->core;

        const auto KeyHandle  = arg<nt::HANDLE>(core, 0);
        const auto ValueName  = arg<nt::PUNICODE_STRING>(core, 1);
        const auto TitleIndex = arg<nt::ULONG>(core, 2);
        const auto Type       = arg<nt::ULONG>(core, 3);
        const auto Data       = arg<nt::PVOID>(core, 4);
        const auto DataSize   = arg<nt::ULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[372]);

        on_func(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSetVolumeInformationFile(proc_t proc, const on_ZwSetVolumeInformationFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSetVolumeInformationFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle         = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto FsInformation      = arg<nt::PVOID>(core, 2);
        const auto Length             = arg<nt::ULONG>(core, 3);
        const auto FsInformationClass = arg<nt::FS_INFORMATION_CLASS>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[373]);

        on_func(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    });
}

opt<bpid_t> nt::syscalls::register_ZwShutdownSystem(proc_t proc, const on_ZwShutdownSystem_fn& on_func)
{
    return register_callback(*d_, proc, "ZwShutdownSystem", [=]
    {
        auto& core = d_->core;

        const auto Action = arg<nt::SHUTDOWN_ACTION>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[374]);

        on_func(Action);
    });
}

opt<bpid_t> nt::syscalls::register_ZwShutdownWorkerFactory(proc_t proc, const on_ZwShutdownWorkerFactory_fn& on_func)
{
    return register_callback(*d_, proc, "ZwShutdownWorkerFactory", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle    = arg<nt::HANDLE>(core, 0);
        const auto STARPendingWorkerCount = arg<nt::LONG>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[375]);

        on_func(WorkerFactoryHandle, STARPendingWorkerCount);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSignalAndWaitForSingleObject(proc_t proc, const on_ZwSignalAndWaitForSingleObject_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSignalAndWaitForSingleObject", [=]
    {
        auto& core = d_->core;

        const auto SignalHandle = arg<nt::HANDLE>(core, 0);
        const auto WaitHandle   = arg<nt::HANDLE>(core, 1);
        const auto Alertable    = arg<nt::BOOLEAN>(core, 2);
        const auto Timeout      = arg<nt::PLARGE_INTEGER>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[376]);

        on_func(SignalHandle, WaitHandle, Alertable, Timeout);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSinglePhaseReject(proc_t proc, const on_ZwSinglePhaseReject_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSinglePhaseReject", [=]
    {
        auto& core = d_->core;

        const auto EnlistmentHandle = arg<nt::HANDLE>(core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[377]);

        on_func(EnlistmentHandle, TmVirtualClock);
    });
}

opt<bpid_t> nt::syscalls::register_ZwStartProfile(proc_t proc, const on_ZwStartProfile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwStartProfile", [=]
    {
        auto& core = d_->core;

        const auto ProfileHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[378]);

        on_func(ProfileHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwStopProfile(proc_t proc, const on_ZwStopProfile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwStopProfile", [=]
    {
        auto& core = d_->core;

        const auto ProfileHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[379]);

        on_func(ProfileHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSuspendProcess(proc_t proc, const on_ZwSuspendProcess_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSuspendProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[380]);

        on_func(ProcessHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwSystemDebugControl(proc_t proc, const on_ZwSystemDebugControl_fn& on_func)
{
    return register_callback(*d_, proc, "ZwSystemDebugControl", [=]
    {
        auto& core = d_->core;

        const auto Command            = arg<nt::SYSDBG_COMMAND>(core, 0);
        const auto InputBuffer        = arg<nt::PVOID>(core, 1);
        const auto InputBufferLength  = arg<nt::ULONG>(core, 2);
        const auto OutputBuffer       = arg<nt::PVOID>(core, 3);
        const auto OutputBufferLength = arg<nt::ULONG>(core, 4);
        const auto ReturnLength       = arg<nt::PULONG>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[381]);

        on_func(Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);
    });
}

opt<bpid_t> nt::syscalls::register_ZwTerminateProcess(proc_t proc, const on_ZwTerminateProcess_fn& on_func)
{
    return register_callback(*d_, proc, "ZwTerminateProcess", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle = arg<nt::HANDLE>(core, 0);
        const auto ExitStatus    = arg<nt::NTSTATUS>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[382]);

        on_func(ProcessHandle, ExitStatus);
    });
}

opt<bpid_t> nt::syscalls::register_ZwTestAlert(proc_t proc, const on_ZwTestAlert_fn& on_func)
{
    return register_callback(*d_, proc, "ZwTestAlert", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[383]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_ZwThawRegistry(proc_t proc, const on_ZwThawRegistry_fn& on_func)
{
    return register_callback(*d_, proc, "ZwThawRegistry", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[384]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_ZwThawTransactions(proc_t proc, const on_ZwThawTransactions_fn& on_func)
{
    return register_callback(*d_, proc, "ZwThawTransactions", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[385]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_ZwUmsThreadYield(proc_t proc, const on_ZwUmsThreadYield_fn& on_func)
{
    return register_callback(*d_, proc, "ZwUmsThreadYield", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[386]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_ZwUnloadDriver(proc_t proc, const on_ZwUnloadDriver_fn& on_func)
{
    return register_callback(*d_, proc, "ZwUnloadDriver", [=]
    {
        auto& core = d_->core;

        const auto DriverServiceName = arg<nt::PUNICODE_STRING>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[387]);

        on_func(DriverServiceName);
    });
}

opt<bpid_t> nt::syscalls::register_ZwUnloadKeyEx(proc_t proc, const on_ZwUnloadKeyEx_fn& on_func)
{
    return register_callback(*d_, proc, "ZwUnloadKeyEx", [=]
    {
        auto& core = d_->core;

        const auto TargetKey = arg<nt::POBJECT_ATTRIBUTES>(core, 0);
        const auto Event     = arg<nt::HANDLE>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[388]);

        on_func(TargetKey, Event);
    });
}

opt<bpid_t> nt::syscalls::register_ZwUnlockFile(proc_t proc, const on_ZwUnlockFile_fn& on_func)
{
    return register_callback(*d_, proc, "ZwUnlockFile", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<nt::HANDLE>(core, 0);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core, 1);
        const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(core, 2);
        const auto Length        = arg<nt::PLARGE_INTEGER>(core, 3);
        const auto Key           = arg<nt::ULONG>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[389]);

        on_func(FileHandle, IoStatusBlock, ByteOffset, Length, Key);
    });
}

opt<bpid_t> nt::syscalls::register_ZwUnlockVirtualMemory(proc_t proc, const on_ZwUnlockVirtualMemory_fn& on_func)
{
    return register_callback(*d_, proc, "ZwUnlockVirtualMemory", [=]
    {
        auto& core = d_->core;

        const auto ProcessHandle   = arg<nt::HANDLE>(core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(core, 1);
        const auto RegionSize      = arg<nt::PSIZE_T>(core, 2);
        const auto MapType         = arg<nt::ULONG>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[390]);

        on_func(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    });
}

opt<bpid_t> nt::syscalls::register_ZwVdmControl(proc_t proc, const on_ZwVdmControl_fn& on_func)
{
    return register_callback(*d_, proc, "ZwVdmControl", [=]
    {
        auto& core = d_->core;

        const auto Service     = arg<nt::VDMSERVICECLASS>(core, 0);
        const auto ServiceData = arg<nt::PVOID>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[391]);

        on_func(Service, ServiceData);
    });
}

opt<bpid_t> nt::syscalls::register_ZwWaitForDebugEvent(proc_t proc, const on_ZwWaitForDebugEvent_fn& on_func)
{
    return register_callback(*d_, proc, "ZwWaitForDebugEvent", [=]
    {
        auto& core = d_->core;

        const auto DebugObjectHandle = arg<nt::HANDLE>(core, 0);
        const auto Alertable         = arg<nt::BOOLEAN>(core, 1);
        const auto Timeout           = arg<nt::PLARGE_INTEGER>(core, 2);
        const auto WaitStateChange   = arg<nt::PDBGUI_WAIT_STATE_CHANGE>(core, 3);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[392]);

        on_func(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
    });
}

opt<bpid_t> nt::syscalls::register_ZwWaitForMultipleObjects32(proc_t proc, const on_ZwWaitForMultipleObjects32_fn& on_func)
{
    return register_callback(*d_, proc, "ZwWaitForMultipleObjects32", [=]
    {
        auto& core = d_->core;

        const auto Count     = arg<nt::ULONG>(core, 0);
        const auto Handles   = arg<nt::LONG>(core, 1);
        const auto WaitType  = arg<nt::WAIT_TYPE>(core, 2);
        const auto Alertable = arg<nt::BOOLEAN>(core, 3);
        const auto Timeout   = arg<nt::PLARGE_INTEGER>(core, 4);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[393]);

        on_func(Count, Handles, WaitType, Alertable, Timeout);
    });
}

opt<bpid_t> nt::syscalls::register_ZwWaitForWorkViaWorkerFactory(proc_t proc, const on_ZwWaitForWorkViaWorkerFactory_fn& on_func)
{
    return register_callback(*d_, proc, "ZwWaitForWorkViaWorkerFactory", [=]
    {
        auto& core = d_->core;

        const auto WorkerFactoryHandle = arg<nt::HANDLE>(core, 0);
        const auto MiniPacket          = arg<nt::PFILE_IO_COMPLETION_INFORMATION>(core, 1);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[394]);

        on_func(WorkerFactoryHandle, MiniPacket);
    });
}

opt<bpid_t> nt::syscalls::register_ZwWaitHighEventPair(proc_t proc, const on_ZwWaitHighEventPair_fn& on_func)
{
    return register_callback(*d_, proc, "ZwWaitHighEventPair", [=]
    {
        auto& core = d_->core;

        const auto EventPairHandle = arg<nt::HANDLE>(core, 0);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[395]);

        on_func(EventPairHandle);
    });
}

opt<bpid_t> nt::syscalls::register_ZwWriteFileGather(proc_t proc, const on_ZwWriteFileGather_fn& on_func)
{
    return register_callback(*d_, proc, "ZwWriteFileGather", [=]
    {
        auto& core = d_->core;

        const auto FileHandle    = arg<nt::HANDLE>(core, 0);
        const auto Event         = arg<nt::HANDLE>(core, 1);
        const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(core, 2);
        const auto ApcContext    = arg<nt::PVOID>(core, 3);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core, 4);
        const auto SegmentArray  = arg<nt::PFILE_SEGMENT_ELEMENT>(core, 5);
        const auto Length        = arg<nt::ULONG>(core, 6);
        const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(core, 7);
        const auto Key           = arg<nt::PULONG>(core, 8);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[396]);

        on_func(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    });
}

opt<bpid_t> nt::syscalls::register_ZwWriteRequestData(proc_t proc, const on_ZwWriteRequestData_fn& on_func)
{
    return register_callback(*d_, proc, "ZwWriteRequestData", [=]
    {
        auto& core = d_->core;

        const auto PortHandle           = arg<nt::HANDLE>(core, 0);
        const auto Message              = arg<nt::PPORT_MESSAGE>(core, 1);
        const auto DataEntryIndex       = arg<nt::ULONG>(core, 2);
        const auto Buffer               = arg<nt::PVOID>(core, 3);
        const auto BufferSize           = arg<nt::SIZE_T>(core, 4);
        const auto NumberOfBytesWritten = arg<nt::PSIZE_T>(core, 5);

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[397]);

        on_func(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten);
    });
}

opt<bpid_t> nt::syscalls::register_ZwYieldExecution(proc_t proc, const on_ZwYieldExecution_fn& on_func)
{
    return register_callback(*d_, proc, "ZwYieldExecution", [=]
    {
        auto& core = d_->core;

        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[398]);

        on_func();
    });
}

opt<bpid_t> nt::syscalls::register_all(proc_t proc, const nt::syscalls::on_call_fn& on_call)
{
    auto& d         = *d_;
    const auto bpid = state::acquire_breakpoint_id(d.core);
    for(const auto cfg : g_callcfgs)
        register_callback_with(d, bpid, proc, cfg.name, [=]{ on_call(cfg); });
    return bpid;
}
