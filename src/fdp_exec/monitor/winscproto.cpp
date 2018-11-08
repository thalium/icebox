#include "winscproto.hpp"

#define FDP_MODULE "winscproto"
#include "log.hpp"
#include <unordered_map>

namespace
{
    const int NUM_SYSCALLS = 406;
    //using Protos = std::unordered_map<std::string, win_syscall_t>;
    using Protos = std::vector<winscproto::win_syscall_t>;
}

struct winscproto::WinScProto::Data
{
    Protos syscall_protos;
};

winscproto::WinScProto::WinScProto()
    : d_(std::make_unique<Data>())
{
}

winscproto::WinScProto::~WinScProto()
{
}

bool winscproto::WinScProto::setup()
{
    const win_syscall_t win_syscalls[] =
    {
        {
            "NtFlushProcessWriteBuffers", VOID, 0, {}
        },
        {
            "NtGetCurrentProcessorNumber", ULONG, 0, {}
        },
        {
            "NtGetEnvironmentVariableEx", MISSING, 1,
            {
                {"Missing", DIR_MISSING, MISSING, "", }
            }
        },
        {
            "NtIsSystemResumeAutomatic", MISSING, 1,
            {
                {"Missing", DIR_MISSING, MISSING, "", }
            }
        },
        {
            "NtQueryEnvironmentVariableInfoEx", MISSING, 1,
            {
                {"Missing", DIR_MISSING, MISSING, "", }
            }
        },
        {
            "NtAcceptConnectPort", NTSTATUS, 6,
            {
                {"PortHandle", DIR_OUT, PHANDLE, "", },
                {"PortContext", DIR_IN, PVOID, "opt", },
                {"ConnectionRequest", DIR_IN, PPORT_MESSAGE, "", },
                {"AcceptConnection", DIR_IN, BOOLEAN, "", },
                {"ServerView", DIR_INOUT, PPORT_VIEW, "opt", },
                {"ClientView", DIR_OUT, PREMOTE_PORT_VIEW, "opt", }
            }
        },
        {
            "NtAccessCheckAndAuditAlarm", NTSTATUS, 11,
            {
                {"SubsystemName", DIR_IN, PUNICODE_STRING, "", },
                {"HandleId", DIR_IN, PVOID, "opt", },
                {"ObjectTypeName", DIR_IN, PUNICODE_STRING, "", },
                {"ObjectName", DIR_IN, PUNICODE_STRING, "", },
                {"SecurityDescriptor", DIR_IN, PSECURITY_DESCRIPTOR, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"GenericMapping", DIR_IN, PGENERIC_MAPPING, "", },
                {"ObjectCreation", DIR_IN, BOOLEAN, "", },
                {"GrantedAccess", DIR_OUT, PACCESS_MASK, "", },
                {"AccessStatus", DIR_OUT, PNTSTATUS, "", },
                {"GenerateOnClose", DIR_OUT, PBOOLEAN, "", }
            }
        },
        {
            "NtAccessCheckByTypeAndAuditAlarm", NTSTATUS, 16,
            {
                {"SubsystemName", DIR_IN, PUNICODE_STRING, "", },
                {"HandleId", DIR_IN, PVOID, "opt", },
                {"ObjectTypeName", DIR_IN, PUNICODE_STRING, "", },
                {"ObjectName", DIR_IN, PUNICODE_STRING, "", },
                {"SecurityDescriptor", DIR_IN, PSECURITY_DESCRIPTOR, "", },
                {"PrincipalSelfSid", DIR_IN, PSID, "opt", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"AuditType", DIR_IN, AUDIT_EVENT_TYPE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"ObjectTypeList", DIR_IN, POBJECT_TYPE_LIST, "ecount_opt(ObjectTypeListLength)", },
                {"ObjectTypeListLength", DIR_IN, ULONG, "", },
                {"GenericMapping", DIR_IN, PGENERIC_MAPPING, "", },
                {"ObjectCreation", DIR_IN, BOOLEAN, "", },
                {"GrantedAccess", DIR_OUT, PACCESS_MASK, "", },
                {"AccessStatus", DIR_OUT, PNTSTATUS, "", },
                {"GenerateOnClose", DIR_OUT, PBOOLEAN, "", }
            }
        },
        {
            "NtAccessCheckByType", NTSTATUS, 11,
            {
                {"SecurityDescriptor", DIR_IN, PSECURITY_DESCRIPTOR, "", },
                {"PrincipalSelfSid", DIR_IN, PSID, "opt", },
                {"ClientToken", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectTypeList", DIR_IN, POBJECT_TYPE_LIST, "ecount(ObjectTypeListLength)", },
                {"ObjectTypeListLength", DIR_IN, ULONG, "", },
                {"GenericMapping", DIR_IN, PGENERIC_MAPPING, "", },
                {"PrivilegeSet", DIR_OUT, PPRIVILEGE_SET, "bcount(*PrivilegeSetLength)", },
                {"PrivilegeSetLength", DIR_INOUT, PULONG, "", },
                {"GrantedAccess", DIR_OUT, PACCESS_MASK, "", },
                {"AccessStatus", DIR_OUT, PNTSTATUS, "", }
            }
        },
        {
            "NtAccessCheckByTypeResultListAndAuditAlarmByHandle", NTSTATUS, 17,
            {
                {"SubsystemName", DIR_IN, PUNICODE_STRING, "", },
                {"HandleId", DIR_IN, PVOID, "opt", },
                {"ClientToken", DIR_IN, HANDLE, "", },
                {"ObjectTypeName", DIR_IN, PUNICODE_STRING, "", },
                {"ObjectName", DIR_IN, PUNICODE_STRING, "", },
                {"SecurityDescriptor", DIR_IN, PSECURITY_DESCRIPTOR, "", },
                {"PrincipalSelfSid", DIR_IN, PSID, "opt", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"AuditType", DIR_IN, AUDIT_EVENT_TYPE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"ObjectTypeList", DIR_IN, POBJECT_TYPE_LIST, "ecount_opt(ObjectTypeListLength)", },
                {"ObjectTypeListLength", DIR_IN, ULONG, "", },
                {"GenericMapping", DIR_IN, PGENERIC_MAPPING, "", },
                {"ObjectCreation", DIR_IN, BOOLEAN, "", },
                {"GrantedAccess", DIR_OUT, PACCESS_MASK, "ecount(ObjectTypeListLength)", },
                {"AccessStatus", DIR_OUT, PNTSTATUS, "ecount(ObjectTypeListLength)", },
                {"GenerateOnClose", DIR_OUT, PBOOLEAN, "", }
            }
        },
        {
            "NtAccessCheckByTypeResultListAndAuditAlarm", NTSTATUS, 16,
            {
                {"SubsystemName", DIR_IN, PUNICODE_STRING, "", },
                {"HandleId", DIR_IN, PVOID, "opt", },
                {"ObjectTypeName", DIR_IN, PUNICODE_STRING, "", },
                {"ObjectName", DIR_IN, PUNICODE_STRING, "", },
                {"SecurityDescriptor", DIR_IN, PSECURITY_DESCRIPTOR, "", },
                {"PrincipalSelfSid", DIR_IN, PSID, "opt", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"AuditType", DIR_IN, AUDIT_EVENT_TYPE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"ObjectTypeList", DIR_IN, POBJECT_TYPE_LIST, "ecount_opt(ObjectTypeListLength)", },
                {"ObjectTypeListLength", DIR_IN, ULONG, "", },
                {"GenericMapping", DIR_IN, PGENERIC_MAPPING, "", },
                {"ObjectCreation", DIR_IN, BOOLEAN, "", },
                {"GrantedAccess", DIR_OUT, PACCESS_MASK, "ecount(ObjectTypeListLength)", },
                {"AccessStatus", DIR_OUT, PNTSTATUS, "ecount(ObjectTypeListLength)", },
                {"GenerateOnClose", DIR_OUT, PBOOLEAN, "", }
            }
        },
        {
            "NtAccessCheckByTypeResultList", NTSTATUS, 11,
            {
                {"SecurityDescriptor", DIR_IN, PSECURITY_DESCRIPTOR, "", },
                {"PrincipalSelfSid", DIR_IN, PSID, "opt", },
                {"ClientToken", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectTypeList", DIR_IN, POBJECT_TYPE_LIST, "ecount(ObjectTypeListLength)", },
                {"ObjectTypeListLength", DIR_IN, ULONG, "", },
                {"GenericMapping", DIR_IN, PGENERIC_MAPPING, "", },
                {"PrivilegeSet", DIR_OUT, PPRIVILEGE_SET, "bcount(*PrivilegeSetLength)", },
                {"PrivilegeSetLength", DIR_INOUT, PULONG, "", },
                {"GrantedAccess", DIR_OUT, PACCESS_MASK, "ecount(ObjectTypeListLength)", },
                {"AccessStatus", DIR_OUT, PNTSTATUS, "ecount(ObjectTypeListLength)", }
            }
        },
        {
            "NtAccessCheck", NTSTATUS, 8,
            {
                {"SecurityDescriptor", DIR_IN, PSECURITY_DESCRIPTOR, "", },
                {"ClientToken", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"GenericMapping", DIR_IN, PGENERIC_MAPPING, "", },
                {"PrivilegeSet", DIR_OUT, PPRIVILEGE_SET, "bcount(*PrivilegeSetLength)", },
                {"PrivilegeSetLength", DIR_INOUT, PULONG, "", },
                {"GrantedAccess", DIR_OUT, PACCESS_MASK, "", },
                {"AccessStatus", DIR_OUT, PNTSTATUS, "", }
            }
        },
        {
            "NtAddAtom", NTSTATUS, 3,
            {
                {"AtomName", DIR_IN, PWSTR, "bcount_opt(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"Atom", DIR_OUT, PRTL_ATOM, "opt", }
            }
        },
        {
            "NtAddBootEntry", NTSTATUS, 2,
            {
                {"BootEntry", DIR_IN, PBOOT_ENTRY, "", },
                {"Id", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtAddDriverEntry", NTSTATUS, 2,
            {
                {"DriverEntry", DIR_IN, PEFI_DRIVER_ENTRY, "", },
                {"Id", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtAdjustGroupsToken", NTSTATUS, 6,
            {
                {"TokenHandle", DIR_IN, HANDLE, "", },
                {"ResetToDefault", DIR_IN, BOOLEAN, "", },
                {"NewState", DIR_IN, PTOKEN_GROUPS, "", },
                {"BufferLength", DIR_IN, ULONG, "", },
                {"PreviousState", DIR_OUT, PTOKEN_GROUPS, "bcount_part_opt(BufferLength,*ReturnLength)", },
                {"ReturnLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtAdjustPrivilegesToken", NTSTATUS, 6,
            {
                {"TokenHandle", DIR_IN, HANDLE, "", },
                {"DisableAllPrivileges", DIR_IN, BOOLEAN, "", },
                {"NewState", DIR_IN, PTOKEN_PRIVILEGES, "opt", },
                {"BufferLength", DIR_IN, ULONG, "", },
                {"PreviousState", DIR_OUT, PTOKEN_PRIVILEGES, "bcount_part_opt(BufferLength,*ReturnLength)", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtAlertResumeThread", NTSTATUS, 2,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"PreviousSuspendCount", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtAlertThread", NTSTATUS, 1,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtAllocateLocallyUniqueId", NTSTATUS, 1,
            {
                {"Luid", DIR_OUT, PLUID, "", }
            }
        },
        {
            "NtAllocateReserveObject", NTSTATUS, 3,
            {
                {"MemoryReserveHandle", DIR_OUT, PHANDLE, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"Type", DIR_IN, MEMORY_RESERVE_TYPE, "", }
            }
        },
        {
            "NtAllocateUserPhysicalPages", NTSTATUS, 3,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"NumberOfPages", DIR_INOUT, PULONG_PTR, "", },
                {"UserPfnArra;", DIR_OUT, PULONG_PTR, "ecount(*NumberOfPages)", }
            }
        },
        {
            "NtAllocateUuids", NTSTATUS, 4,
            {
                {"Time", DIR_OUT, PULARGE_INTEGER, "", },
                {"Range", DIR_OUT, PULONG, "", },
                {"Sequence", DIR_OUT, PULONG, "", },
                {"Seed", DIR_OUT, PCHAR, "", }
            }
        },
        {
            "NtAllocateVirtualMemory", NTSTATUS, 6,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"*BaseAddress", DIR_INOUT, PVOID, "", },
                {"ZeroBits", DIR_IN, ULONG_PTR, "", },
                {"RegionSize", DIR_INOUT, PSIZE_T, "", },
                {"AllocationType", DIR_IN, ULONG, "", },
                {"Protect", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtAlpcAcceptConnectPort", NTSTATUS, 9,
            {
                {"PortHandle", DIR_OUT, PHANDLE, "", },
                {"ConnectionPortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"PortAttributes", DIR_IN, PALPC_PORT_ATTRIBUTES, "", },
                {"PortContext", DIR_IN, PVOID, "opt", },
                {"ConnectionRequest", DIR_IN, PPORT_MESSAGE, "", },
                {"ConnectionMessageAttributes", DIR_INOUT, PALPC_MESSAGE_ATTRIBUTES, "opt", },
                {"AcceptConnection", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtAlpcCancelMessage", NTSTATUS, 3,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"MessageContext", DIR_IN, PALPC_CONTEXT_ATTR, "", }
            }
        },
        {
            "NtAlpcConnectPort", NTSTATUS, 11,
            {
                {"PortHandle", DIR_OUT, PHANDLE, "", },
                {"PortName", DIR_IN, PUNICODE_STRING, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"PortAttributes", DIR_IN, PALPC_PORT_ATTRIBUTES, "opt", },
                {"Flags", DIR_IN, ULONG, "", },
                {"RequiredServerSid", DIR_IN, PSID, "opt", },
                {"ConnectionMessage", DIR_INOUT, PPORT_MESSAGE, "", },
                {"BufferLength", DIR_INOUT, PULONG, "opt", },
                {"OutMessageAttributes", DIR_INOUT, PALPC_MESSAGE_ATTRIBUTES, "opt", },
                {"InMessageAttributes", DIR_INOUT, PALPC_MESSAGE_ATTRIBUTES, "opt", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtAlpcCreatePort", NTSTATUS, 3,
            {
                {"PortHandle", DIR_OUT, PHANDLE, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"PortAttributes", DIR_IN, PALPC_PORT_ATTRIBUTES, "opt", }
            }
        },
        {
            "NtAlpcCreatePortSection", NTSTATUS, 6,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"SectionHandle", DIR_IN, HANDLE, "opt", },
                {"SectionSize", DIR_IN, SIZE_T, "", },
                {"AlpcSectionHandle", DIR_OUT, PALPC_HANDLE, "", },
                {"ActualSectionSize", DIR_OUT, PSIZE_T, "", }
            }
        },
        {
            "NtAlpcCreateResourceReserve", NTSTATUS, 4,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_RESERVED, ULONG, "", },
                {"MessageSize", DIR_IN, SIZE_T, "", },
                {"ResourceId", DIR_OUT, PALPC_HANDLE, "", }
            }
        },
        {
            "NtAlpcCreateSectionView", NTSTATUS, 3,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_RESERVED, ULONG, "", },
                {"ViewAttributes", DIR_INOUT, PALPC_DATA_VIEW_ATTR, "", }
            }
        },
        {
            "NtAlpcCreateSecurityContext", NTSTATUS, 3,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_RESERVED, ULONG, "", },
                {"SecurityAttribute", DIR_INOUT, PALPC_SECURITY_ATTR, "", }
            }
        },
        {
            "NtAlpcDeletePortSection", NTSTATUS, 3,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_RESERVED, ULONG, "", },
                {"SectionHandle", DIR_IN, ALPC_HANDLE, "", }
            }
        },
        {
            "NtAlpcDeleteResourceReserve", NTSTATUS, 3,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_RESERVED, ULONG, "", },
                {"ResourceId", DIR_IN, ALPC_HANDLE, "", }
            }
        },
        {
            "NtAlpcDeleteSectionView", NTSTATUS, 3,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_RESERVED, ULONG, "", },
                {"ViewBase", DIR_IN, PVOID, "", }
            }
        },
        {
            "NtAlpcDeleteSecurityContext", NTSTATUS, 3,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_RESERVED, ULONG, "", },
                {"ContextHandle", DIR_IN, ALPC_HANDLE, "", }
            }
        },
        {
            "NtAlpcDisconnectPort", NTSTATUS, 2,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtAlpcImpersonateClientOfPort", NTSTATUS, 3,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"PortMessage", DIR_IN, PPORT_MESSAGE, "", },
                {"Reserved", DIR_RESERVED, PVOID, "", }
            }
        },
        {
            "NtAlpcOpenSenderProcess", NTSTATUS, 6,
            {
                {"ProcessHandle", DIR_OUT, PHANDLE, "", },
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"PortMessage", DIR_IN, PPORT_MESSAGE, "", },
                {"Flags", DIR_RESERVED, ULONG, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtAlpcOpenSenderThread", NTSTATUS, 6,
            {
                {"ThreadHandle", DIR_OUT, PHANDLE, "", },
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"PortMessage", DIR_IN, PPORT_MESSAGE, "", },
                {"Flags", DIR_RESERVED, ULONG, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtAlpcQueryInformation", NTSTATUS, 5,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"PortInformationClass", DIR_IN, ALPC_PORT_INFORMATION_CLASS, "", },
                {"PortInformation", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtAlpcQueryInformationMessage", NTSTATUS, 6,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"PortMessage", DIR_IN, PPORT_MESSAGE, "", },
                {"MessageInformationClass", DIR_IN, ALPC_MESSAGE_INFORMATION_CLASS, "", },
                {"MessageInformation", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtAlpcRevokeSecurityContext", NTSTATUS, 3,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_RESERVED, ULONG, "", },
                {"ContextHandle", DIR_IN, ALPC_HANDLE, "", }
            }
        },
        {
            "NtAlpcSendWaitReceivePort", NTSTATUS, 8,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"SendMessage", DIR_IN, PPORT_MESSAGE, "opt", },
                {"SendMessageAttributes", DIR_IN, PALPC_MESSAGE_ATTRIBUTES, "opt", },
                {"ReceiveMessage", DIR_INOUT, PPORT_MESSAGE, "opt", },
                {"BufferLength", DIR_INOUT, PULONG, "opt", },
                {"ReceiveMessageAttributes", DIR_INOUT, PALPC_MESSAGE_ATTRIBUTES, "opt", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtAlpcSetInformation", NTSTATUS, 4,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"PortInformationClass", DIR_IN, ALPC_PORT_INFORMATION_CLASS, "", },
                {"PortInformation", DIR_IN, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtApphelpCacheControl", NTSTATUS, 2,
            {
                {"type", DIR_IN, APPHELPCOMMAND, "", },
                {"buf", DIR_IN, PVOID, "", }
            }
        },
        {
            "NtAreMappedFilesTheSame", NTSTATUS, 2,
            {
                {"File1MappedAsAnImage", DIR_IN, PVOID, "", },
                {"File2MappedAsFile", DIR_IN, PVOID, "", }
            }
        },
        {
            "NtAssignProcessToJobObject", NTSTATUS, 2,
            {
                {"JobHandle", DIR_IN, HANDLE, "", },
                {"ProcessHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtCallbackReturn", NTSTATUS, 3,
            {
                {"OutputBuffer", DIR_IN, PVOID, "opt", },
                {"OutputLength", DIR_IN, ULONG, "", },
                {"Status", DIR_IN, NTSTATUS, "", }
            }
        },
        {
            "NtCancelIoFileEx", NTSTATUS, 3,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoRequestToCancel", DIR_IN, PIO_STATUS_BLOCK, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", }
            }
        },
        {
            "NtCancelIoFile", NTSTATUS, 2,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", }
            }
        },
        {
            "NtCancelSynchronousIoFile", NTSTATUS, 3,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"IoRequestToCancel", DIR_IN, PIO_STATUS_BLOCK, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", }
            }
        },
        {
            "NtCancelTimer", NTSTATUS, 2,
            {
                {"TimerHandle", DIR_IN, HANDLE, "", },
                {"CurrentState", DIR_OUT, PBOOLEAN, "opt", }
            }
        },
        {
            "NtClearEvent", NTSTATUS, 1,
            {
                {"EventHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtClose", NTSTATUS, 1,
            {
                {"Handle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtCloseObjectAuditAlarm", NTSTATUS, 3,
            {
                {"SubsystemName", DIR_IN, PUNICODE_STRING, "", },
                {"HandleId", DIR_IN, PVOID, "opt", },
                {"GenerateOnClose", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtCommitComplete", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtCommitEnlistment", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtCommitTransaction", NTSTATUS, 2,
            {
                {"TransactionHandle", DIR_IN, HANDLE, "", },
                {"Wait", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtCompactKeys", NTSTATUS, 2,
            {
                {"Count", DIR_IN, ULONG, "", },
                {"KeyArray[;", DIR_IN, HANDLE, "ecount(Count)", }
            }
        },
        {
            "NtCompareTokens", NTSTATUS, 3,
            {
                {"FirstTokenHandle", DIR_IN, HANDLE, "", },
                {"SecondTokenHandle", DIR_IN, HANDLE, "", },
                {"Equal", DIR_OUT, PBOOLEAN, "", }
            }
        },
        {
            "NtCompleteConnectPort", NTSTATUS, 1,
            {
                {"PortHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtCompressKey", NTSTATUS, 1,
            {
                {"Key", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtConnectPort", NTSTATUS, 8,
            {
                {"PortHandle", DIR_OUT, PHANDLE, "", },
                {"PortName", DIR_IN, PUNICODE_STRING, "", },
                {"SecurityQos", DIR_IN, PSECURITY_QUALITY_OF_SERVICE, "", },
                {"ClientView", DIR_INOUT, PPORT_VIEW, "opt", },
                {"ServerView", DIR_INOUT, PREMOTE_PORT_VIEW, "opt", },
                {"MaxMessageLength", DIR_OUT, PULONG, "opt", },
                {"ConnectionInformation", DIR_INOUT, PVOID, "opt", },
                {"ConnectionInformationLength", DIR_INOUT, PULONG, "opt", }
            }
        },
        {
            "NtContinue", NTSTATUS, 2,
            {
                {"ContextRecord", DIR_IN, PCONTEXT, "", },
                {"TestAlert", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtCreateDebugObject", NTSTATUS, 4,
            {
                {"DebugObjectHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_OUT, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_OUT, POBJECT_ATTRIBUTES, "", },
                {"Flags", DIR_OUT, ULONG, "", }
            }
        },
        {
            "NtCreateDirectoryObject", NTSTATUS, 3,
            {
                {"DirectoryHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtCreateEnlistment", NTSTATUS, 8,
            {
                {"EnlistmentHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ResourceManagerHandle", DIR_IN, HANDLE, "", },
                {"TransactionHandle", DIR_IN, HANDLE, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"CreateOptions", DIR_IN, ULONG, "opt", },
                {"NotificationMask", DIR_IN, NOTIFICATION_MASK, "", },
                {"EnlistmentKey", DIR_IN, PVOID, "opt", }
            }
        },
        {
            "NtCreateEvent", NTSTATUS, 5,
            {
                {"EventHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"EventType", DIR_IN, EVENT_TYPE, "", },
                {"InitialState", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtCreateEventPair", NTSTATUS, 3,
            {
                {"EventPairHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", }
            }
        },
        {
            "NtCreateFile", NTSTATUS, 11,
            {
                {"FileHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"AllocationSize", DIR_IN, PLARGE_INTEGER, "opt", },
                {"FileAttributes", DIR_IN, ULONG, "", },
                {"ShareAccess", DIR_IN, ULONG, "", },
                {"CreateDisposition", DIR_IN, ULONG, "", },
                {"CreateOptions", DIR_IN, ULONG, "", },
                {"EaBuffer", DIR_IN, PVOID, "bcount_opt(EaLength)", },
                {"EaLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtCreateIoCompletion", NTSTATUS, 4,
            {
                {"IoCompletionHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"Count", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtCreateJobObject", NTSTATUS, 3,
            {
                {"JobHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", }
            }
        },
        {
            "NtCreateJobSet", NTSTATUS, 3,
            {
                {"NumJob", DIR_IN, ULONG, "", },
                {"UserJobSet", DIR_IN, PJOB_SET_ARRAY, "ecount(NumJob)", },
                {"Flags", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtCreateKeyedEvent", NTSTATUS, 4,
            {
                {"KeyedEventHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"Flags", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtCreateKey", NTSTATUS, 7,
            {
                {"KeyHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"TitleIndex", DIR_RESERVED, ULONG, "", },
                {"Class", DIR_IN, PUNICODE_STRING, "opt", },
                {"CreateOptions", DIR_IN, ULONG, "", },
                {"Disposition", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtCreateKeyTransacted", NTSTATUS, 8,
            {
                {"KeyHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"TitleIndex", DIR_RESERVED, ULONG, "", },
                {"Class", DIR_IN, PUNICODE_STRING, "opt", },
                {"CreateOptions", DIR_IN, ULONG, "", },
                {"TransactionHandle", DIR_IN, HANDLE, "", },
                {"Disposition", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtCreateMailslotFile", NTSTATUS, 8,
            {
                {"FileHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ULONG, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"CreateOptions", DIR_IN, ULONG, "", },
                {"MailslotQuota", DIR_IN, ULONG, "", },
                {"MaximumMessageSize", DIR_IN, ULONG, "", },
                {"ReadTimeout", DIR_IN, PLARGE_INTEGER, "", }
            }
        },
        {
            "NtCreateMutant", NTSTATUS, 4,
            {
                {"MutantHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"InitialOwner", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtCreateNamedPipeFile", NTSTATUS, 14,
            {
                {"FileHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ULONG, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"ShareAccess", DIR_IN, ULONG, "", },
                {"CreateDisposition", DIR_IN, ULONG, "", },
                {"CreateOptions", DIR_IN, ULONG, "", },
                {"NamedPipeType", DIR_IN, ULONG, "", },
                {"ReadMode", DIR_IN, ULONG, "", },
                {"CompletionMode", DIR_IN, ULONG, "", },
                {"MaximumInstances", DIR_IN, ULONG, "", },
                {"InboundQuota", DIR_IN, ULONG, "", },
                {"OutboundQuota", DIR_IN, ULONG, "", },
                {"DefaultTimeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtCreatePagingFile", NTSTATUS, 4,
            {
                {"PageFileName", DIR_IN, PUNICODE_STRING, "", },
                {"MinimumSize", DIR_IN, PLARGE_INTEGER, "", },
                {"MaximumSize", DIR_IN, PLARGE_INTEGER, "", },
                {"Priority", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtCreatePort", NTSTATUS, 5,
            {
                {"PortHandle", DIR_OUT, PHANDLE, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"MaxConnectionInfoLength", DIR_IN, ULONG, "", },
                {"MaxMessageLength", DIR_IN, ULONG, "", },
                {"MaxPoolUsage", DIR_IN, ULONG, "opt", }
            }
        },
        {
            "NtCreatePrivateNamespace", NTSTATUS, 4,
            {
                {"NamespaceHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"BoundaryDescriptor", DIR_IN, PVOID, "", }
            }
        },
        {
            "NtCreateProcessEx", NTSTATUS, 9,
            {
                {"ProcessHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"ParentProcess", DIR_IN, HANDLE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"SectionHandle", DIR_IN, HANDLE, "opt", },
                {"DebugPort", DIR_IN, HANDLE, "opt", },
                {"ExceptionPort", DIR_IN, HANDLE, "opt", },
                {"JobMemberLevel", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtCreateProcess", NTSTATUS, 8,
            {
                {"ProcessHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"ParentProcess", DIR_IN, HANDLE, "", },
                {"InheritObjectTable", DIR_IN, BOOLEAN, "", },
                {"SectionHandle", DIR_IN, HANDLE, "opt", },
                {"DebugPort", DIR_IN, HANDLE, "opt", },
                {"ExceptionPort", DIR_IN, HANDLE, "opt", }
            }
        },
        {
            "NtCreateProfileEx", NTSTATUS, 10,
            {
                {"ProfileHandle", DIR_OUT, PHANDLE, "", },
                {"Process", DIR_IN, HANDLE, "opt", },
                {"ProfileBase", DIR_IN, PVOID, "", },
                {"ProfileSize", DIR_IN, SIZE_T, "", },
                {"BucketSize", DIR_IN, ULONG, "", },
                {"Buffer", DIR_IN, PULONG, "", },
                {"BufferSize", DIR_IN, ULONG, "", },
                {"ProfileSource", DIR_IN, KPROFILE_SOURCE, "", },
                {"GroupAffinityCount", DIR_IN, ULONG, "", },
                {"GroupAffinity", DIR_IN, PGROUP_AFFINITY, "opt", }
            }
        },
        {
            "NtCreateProfile", NTSTATUS, 9,
            {
                {"ProfileHandle", DIR_OUT, PHANDLE, "", },
                {"Process", DIR_IN, HANDLE, "", },
                {"RangeBase", DIR_IN, PVOID, "", },
                {"RangeSize", DIR_IN, SIZE_T, "", },
                {"BucketSize", DIR_IN, ULONG, "", },
                {"Buffer", DIR_IN, PULONG, "", },
                {"BufferSize", DIR_IN, ULONG, "", },
                {"ProfileSource", DIR_IN, KPROFILE_SOURCE, "", },
                {"Affinity", DIR_IN, KAFFINITY, "", }
            }
        },
        {
            "NtCreateResourceManager", NTSTATUS, 7,
            {
                {"ResourceManagerHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"TmHandle", DIR_IN, HANDLE, "", },
                {"RmGuid", DIR_IN, LPGUID, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"CreateOptions", DIR_IN, ULONG, "opt", },
                {"Description", DIR_IN, PUNICODE_STRING, "opt", }
            }
        },
        {
            "NtCreateSection", NTSTATUS, 7,
            {
                {"SectionHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"MaximumSize", DIR_IN, PLARGE_INTEGER, "opt", },
                {"SectionPageProtection", DIR_IN, ULONG, "", },
                {"AllocationAttributes", DIR_IN, ULONG, "", },
                {"FileHandle", DIR_IN, HANDLE, "opt", }
            }
        },
        {
            "NtCreateSemaphore", NTSTATUS, 5,
            {
                {"SemaphoreHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"InitialCount", DIR_IN, LONG, "", },
                {"MaximumCount", DIR_IN, LONG, "", }
            }
        },
        {
            "NtCreateSymbolicLinkObject", NTSTATUS, 4,
            {
                {"LinkHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"LinkTarget", DIR_IN, PUNICODE_STRING, "", }
            }
        },
        {
            "NtCreateThreadEx", NTSTATUS, 11,
            {
                {"ThreadHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"StartRoutine", DIR_IN, PVOID, "", },
                {"Argument", DIR_IN, PVOID, "opt", },
                {"CreateFlags", DIR_IN, ULONG, "", },
                {"ZeroBits", DIR_IN, ULONG_PTR, "opt", },
                {"StackSize", DIR_IN, SIZE_T, "opt", },
                {"MaximumStackSize", DIR_IN, SIZE_T, "opt", },
                {"AttributeList", DIR_IN, PPS_ATTRIBUTE_LIST, "opt", }
            }
        },
        {
            "NtCreateThread", NTSTATUS, 8,
            {
                {"ThreadHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"ClientId", DIR_OUT, PCLIENT_ID, "", },
                {"ThreadContext", DIR_IN, PCONTEXT, "", },
                {"InitialTeb", DIR_IN, PINITIAL_TEB, "", },
                {"CreateSuspended", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtCreateTimer", NTSTATUS, 4,
            {
                {"TimerHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"TimerType", DIR_IN, TIMER_TYPE, "", }
            }
        },
        {
            "NtCreateToken", NTSTATUS, 13,
            {
                {"TokenHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"TokenType", DIR_IN, TOKEN_TYPE, "", },
                {"AuthenticationId", DIR_IN, PLUID, "", },
                {"ExpirationTime", DIR_IN, PLARGE_INTEGER, "", },
                {"User", DIR_IN, PTOKEN_USER, "", },
                {"Groups", DIR_IN, PTOKEN_GROUPS, "", },
                {"Privileges", DIR_IN, PTOKEN_PRIVILEGES, "", },
                {"Owner", DIR_IN, PTOKEN_OWNER, "opt", },
                {"PrimaryGroup", DIR_IN, PTOKEN_PRIMARY_GROUP, "", },
                {"DefaultDacl", DIR_IN, PTOKEN_DEFAULT_DACL, "opt", },
                {"TokenSource", DIR_IN, PTOKEN_SOURCE, "", }
            }
        },
        {
            "NtCreateTransactionManager", NTSTATUS, 6,
            {
                {"TmHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"LogFileName", DIR_IN, PUNICODE_STRING, "opt", },
                {"CreateOptions", DIR_IN, ULONG, "opt", },
                {"CommitStrength", DIR_IN, ULONG, "opt", }
            }
        },
        {
            "NtCreateTransaction", NTSTATUS, 10,
            {
                {"TransactionHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"Uow", DIR_IN, LPGUID, "opt", },
                {"TmHandle", DIR_IN, HANDLE, "opt", },
                {"CreateOptions", DIR_IN, ULONG, "opt", },
                {"IsolationLevel", DIR_IN, ULONG, "opt", },
                {"IsolationFlags", DIR_IN, ULONG, "opt", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", },
                {"Description", DIR_IN, PUNICODE_STRING, "opt", }
            }
        },
        {
            "NtCreateUserProcess", NTSTATUS, 11,
            {
                {"ProcessHandle", DIR_OUT, PHANDLE, "", },
                {"ThreadHandle", DIR_OUT, PHANDLE, "", },
                {"ProcessDesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ThreadDesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ProcessObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"ThreadObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"ProcessFlags", DIR_IN, ULONG, "", },
                {"ThreadFlags", DIR_IN, ULONG, "", },
                {"ProcessParameters", DIR_IN, PRTL_USER_PROCESS_PARAMETERS, "opt", },
                {"CreateInfo", DIR_IN, PPROCESS_CREATE_INFO, "opt", },
                {"AttributeList", DIR_IN, PPROCESS_ATTRIBUTE_LIST, "opt", }
            }
        },
        {
            "NtCreateWaitablePort", NTSTATUS, 5,
            {
                {"PortHandle", DIR_OUT, PHANDLE, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"MaxConnectionInfoLength", DIR_IN, ULONG, "", },
                {"MaxMessageLength", DIR_IN, ULONG, "", },
                {"MaxPoolUsage", DIR_IN, ULONG, "opt", }
            }
        },
        {
            "NtCreateWorkerFactory", NTSTATUS, 10,
            {
                {"WorkerFactoryHandleReturn", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"CompletionPortHandle", DIR_IN, HANDLE, "", },
                {"WorkerProcessHandle", DIR_IN, HANDLE, "", },
                {"StartRoutine", DIR_IN, PVOID, "", },
                {"StartParameter", DIR_IN, PVOID, "opt", },
                {"MaxThreadCount", DIR_IN, ULONG, "opt", },
                {"StackReserve", DIR_IN, SIZE_T, "opt", },
                {"StackCommit", DIR_IN, SIZE_T, "opt", }
            }
        },
        {
            "NtDebugActiveProcess", NTSTATUS, 2,
            {
                {"ProcessHandle", DIR_OUT, HANDLE, "", },
                {"DebugObjectHandle", DIR_OUT, HANDLE, "", }
            }
        },
        {
            "NtDebugContinue", NTSTATUS, 3,
            {
                {"DebugObjectHandle", DIR_OUT, HANDLE, "", },
                {"ClientId", DIR_OUT, PCLIENT_ID, "", },
                {"ContinueStatus", DIR_OUT, NTSTATUS, "", }
            }
        },
        {
            "NtDelayExecution", NTSTATUS, 2,
            {
                {"Alertable", DIR_IN, BOOLEAN, "", },
                {"DelayInterval", DIR_IN, PLARGE_INTEGER, "", }
            }
        },
        {
            "NtDeleteAtom", NTSTATUS, 1,
            {
                {"Atom", DIR_IN, RTL_ATOM, "", }
            }
        },
        {
            "NtDeleteBootEntry", NTSTATUS, 1,
            {
                {"Id", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtDeleteDriverEntry", NTSTATUS, 1,
            {
                {"Id", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtDeleteFile", NTSTATUS, 1,
            {
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtDeleteKey", NTSTATUS, 1,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtDeleteObjectAuditAlarm", NTSTATUS, 3,
            {
                {"SubsystemName", DIR_IN, PUNICODE_STRING, "", },
                {"HandleId", DIR_IN, PVOID, "opt", },
                {"GenerateOnClose", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtDeletePrivateNamespace", NTSTATUS, 1,
            {
                {"NamespaceHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtDeleteValueKey", NTSTATUS, 2,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"ValueName", DIR_IN, PUNICODE_STRING, "", }
            }
        },
        {
            "NtDeviceIoControlFile", NTSTATUS, 10,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"IoControlCode", DIR_IN, ULONG, "", },
                {"InputBuffer", DIR_IN, PVOID, "bcount_opt(InputBufferLength)", },
                {"InputBufferLength", DIR_IN, ULONG, "", },
                {"OutputBuffer", DIR_OUT, PVOID, "bcount_opt(OutputBufferLength)", },
                {"OutputBufferLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtDisableLastKnownGood", NTSTATUS, 0, {}
        },
        {
            "NtDisplayString", NTSTATUS, 1,
            {
                {"String", DIR_IN, PUNICODE_STRING, "", }
            }
        },
        {
            "NtDrawText", NTSTATUS, 1,
            {
                {"Text", DIR_IN, PUNICODE_STRING, "", }
            }
        },
        {
            "NtDuplicateObject", NTSTATUS, 7,
            {
                {"SourceProcessHandle", DIR_IN, HANDLE, "", },
                {"SourceHandle", DIR_IN, HANDLE, "", },
                {"TargetProcessHandle", DIR_IN, HANDLE, "opt", },
                {"TargetHandle", DIR_OUT, PHANDLE, "opt", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"HandleAttributes", DIR_IN, ULONG, "", },
                {"Options", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtDuplicateToken", NTSTATUS, 6,
            {
                {"ExistingTokenHandle", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"EffectiveOnly", DIR_IN, BOOLEAN, "", },
                {"TokenType", DIR_IN, TOKEN_TYPE, "", },
                {"NewTokenHandle", DIR_OUT, PHANDLE, "", }
            }
        },
        {
            "NtEnableLastKnownGood", NTSTATUS, 0, {}
        },
        {
            "NtEnumerateBootEntries", NTSTATUS, 2,
            {
                {"Buffer", DIR_OUT, PVOID, "bcount_opt(*BufferLength)", },
                {"BufferLength", DIR_INOUT, PULONG, "", }
            }
        },
        {
            "NtEnumerateDriverEntries", NTSTATUS, 2,
            {
                {"Buffer", DIR_OUT, PVOID, "bcount(*BufferLength)", },
                {"BufferLength", DIR_INOUT, PULONG, "", }
            }
        },
        {
            "NtEnumerateKey", NTSTATUS, 6,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"Index", DIR_IN, ULONG, "", },
                {"KeyInformationClass", DIR_IN, KEY_INFORMATION_CLASS, "", },
                {"KeyInformation", DIR_OUT, PVOID, "bcount_opt(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ResultLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtEnumerateSystemEnvironmentValuesEx", NTSTATUS, 3,
            {
                {"InformationClass", DIR_IN, ULONG, "", },
                {"Buffer", DIR_OUT, PVOID, "", },
                {"BufferLength", DIR_INOUT, PULONG, "", }
            }
        },
        {
            "NtEnumerateTransactionObject", NTSTATUS, 5,
            {
                {"RootObjectHandle", DIR_IN, HANDLE, "opt", },
                {"QueryType", DIR_IN, KTMOBJECT_TYPE, "", },
                {"ObjectCursor", DIR_INOUT, PKTMOBJECT_CURSOR, "bcount(ObjectCursorLength)", },
                {"ObjectCursorLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtEnumerateValueKey", NTSTATUS, 6,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"Index", DIR_IN, ULONG, "", },
                {"KeyValueInformationClass", DIR_IN, KEY_VALUE_INFORMATION_CLASS, "", },
                {"KeyValueInformation", DIR_OUT, PVOID, "bcount_opt(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ResultLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtExtendSection", NTSTATUS, 2,
            {
                {"SectionHandle", DIR_IN, HANDLE, "", },
                {"NewSectionSize", DIR_INOUT, PLARGE_INTEGER, "", }
            }
        },
        {
            "NtFilterToken", NTSTATUS, 6,
            {
                {"ExistingTokenHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"SidsToDisable", DIR_IN, PTOKEN_GROUPS, "opt", },
                {"PrivilegesToDelete", DIR_IN, PTOKEN_PRIVILEGES, "opt", },
                {"RestrictedSids", DIR_IN, PTOKEN_GROUPS, "opt", },
                {"NewTokenHandle", DIR_OUT, PHANDLE, "", }
            }
        },
        {
            "NtFindAtom", NTSTATUS, 3,
            {
                {"AtomName", DIR_IN, PWSTR, "bcount_opt(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"Atom", DIR_OUT, PRTL_ATOM, "opt", }
            }
        },
        {
            "NtFlushBuffersFile", NTSTATUS, 2,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", }
            }
        },
        {
            "NtFlushInstallUILanguage", NTSTATUS, 2,
            {
                {"InstallUILanguage", DIR_IN, LANGID, "", },
                {"SetComittedFlag", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtFlushInstructionCache", NTSTATUS, 3,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"BaseAddress", DIR_IN, PVOID, "opt", },
                {"Length", DIR_IN, SIZE_T, "", }
            }
        },
        {
            "NtFlushKey", NTSTATUS, 1,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtFlushProcessWriteBuffers", VOID, 0, {}
        },
        {
            "NtFlushVirtualMemory", NTSTATUS, 4,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"*BaseAddress", DIR_INOUT, PVOID, "", },
                {"RegionSize", DIR_INOUT, PSIZE_T, "", },
                {"IoStatus", DIR_OUT, PIO_STATUS_BLOCK, "", }
            }
        },
        {
            "NtFlushWriteBuffer", NTSTATUS, 0, {}
        },
        {
            "NtFreeUserPhysicalPages", NTSTATUS, 3,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"NumberOfPages", DIR_INOUT, PULONG_PTR, "", },
                {"UserPfnArra;", DIR_IN, PULONG_PTR, "ecount(*NumberOfPages)", }
            }
        },
        {
            "NtFreeVirtualMemory", NTSTATUS, 4,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"*BaseAddress", DIR_INOUT, PVOID, "", },
                {"RegionSize", DIR_INOUT, PSIZE_T, "", },
                {"FreeType", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtFreezeRegistry", NTSTATUS, 1,
            {
                {"TimeOutInSeconds", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtFreezeTransactions", NTSTATUS, 2,
            {
                {"FreezeTimeout", DIR_IN, PLARGE_INTEGER, "", },
                {"ThawTimeout", DIR_IN, PLARGE_INTEGER, "", }
            }
        },
        {
            "NtFsControlFile", NTSTATUS, 10,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"IoControlCode", DIR_IN, ULONG, "", },
                {"InputBuffer", DIR_IN, PVOID, "bcount_opt(InputBufferLength)", },
                {"InputBufferLength", DIR_IN, ULONG, "", },
                {"OutputBuffer", DIR_OUT, PVOID, "bcount_opt(OutputBufferLength)", },
                {"OutputBufferLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtGetContextThread", NTSTATUS, 2,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"ThreadContext", DIR_INOUT, PCONTEXT, "", }
            }
        },
        {
            "NtGetCurrentProcessorNumber", ULONG, 0, {}
        },
        {
            "NtGetDevicePowerState", NTSTATUS, 2,
            {
                {"Device", DIR_IN, HANDLE, "", },
                {"*State", DIR_OUT, DEVICE_POWER_STATE, "", }
            }
        },
        {
            "NtGetMUIRegistryInfo", NTSTATUS, 3,
            {
                {"Flags", DIR_IN, ULONG, "", },
                {"DataSize", DIR_INOUT, PULONG, "", },
                {"Data", DIR_OUT, PVOID, "", }
            }
        },
        {
            "NtGetNextProcess", NTSTATUS, 5,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"HandleAttributes", DIR_IN, ULONG, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"NewProcessHandle", DIR_OUT, PHANDLE, "", }
            }
        },
        {
            "NtGetNextThread", NTSTATUS, 6,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"HandleAttributes", DIR_IN, ULONG, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"NewThreadHandle", DIR_OUT, PHANDLE, "", }
            }
        },
        {
            "NtGetNlsSectionPtr", NTSTATUS, 5,
            {
                {"SectionType", DIR_IN, ULONG, "", },
                {"SectionData", DIR_IN, ULONG, "", },
                {"ContextData", DIR_IN, PVOID, "", },
                {"*SectionPointer", DIR_OUT, PVOID, "", },
                {"SectionSize", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtGetNotificationResourceManager", NTSTATUS, 7,
            {
                {"ResourceManagerHandle", DIR_IN, HANDLE, "", },
                {"TransactionNotification", DIR_OUT, PTRANSACTION_NOTIFICATION, "", },
                {"NotificationLength", DIR_IN, ULONG, "", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", },
                {"Asynchronous", DIR_IN, ULONG, "", },
                {"AsynchronousContext", DIR_IN, ULONG_PTR, "opt", }
            }
        },
        {
            "NtGetPlugPlayEvent", NTSTATUS, 4,
            {
                {"EventHandle", DIR_IN, HANDLE, "", },
                {"Context", DIR_IN, PVOID, "opt", },
                {"EventBlock", DIR_OUT, PPLUGPLAY_EVENT_BLOCK, "bcount(EventBufferSize)", },
                {"EventBufferSize", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtGetWriteWatch", NTSTATUS, 7,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"BaseAddress", DIR_IN, PVOID, "", },
                {"RegionSize", DIR_IN, SIZE_T, "", },
                {"*UserAddressArray", DIR_OUT, PVOID, "ecount(*EntriesInUserAddressArray)", },
                {"EntriesInUserAddressArray", DIR_INOUT, PULONG_PTR, "", },
                {"Granularity", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtImpersonateAnonymousToken", NTSTATUS, 1,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtImpersonateClientOfPort", NTSTATUS, 2,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Message", DIR_IN, PPORT_MESSAGE, "", }
            }
        },
        {
            "NtImpersonateThread", NTSTATUS, 3,
            {
                {"ServerThreadHandle", DIR_IN, HANDLE, "", },
                {"ClientThreadHandle", DIR_IN, HANDLE, "", },
                {"SecurityQos", DIR_IN, PSECURITY_QUALITY_OF_SERVICE, "", }
            }
        },
        {
            "NtInitializeNlsFiles", NTSTATUS, 3,
            {
                {"*BaseAddress", DIR_OUT, PVOID, "", },
                {"DefaultLocaleId", DIR_OUT, PLCID, "", },
                {"DefaultCasingTableSize", DIR_OUT, PLARGE_INTEGER, "", }
            }
        },
        {
            "NtInitializeRegistry", NTSTATUS, 1,
            {
                {"BootCondition", DIR_IN, USHORT, "", }
            }
        },
        {
            "NtInitiatePowerAction", NTSTATUS, 4,
            {
                {"SystemAction", DIR_IN, POWER_ACTION, "", },
                {"MinSystemState", DIR_IN, SYSTEM_POWER_STATE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"Asynchronous", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtIsProcessInJob", NTSTATUS, 2,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"JobHandle", DIR_IN, HANDLE, "opt", }
            }
        },
        {
            "NtIsSystemResumeAutomatic", BOOLEAN, 0, {}
        },
        {
            "NtIsUILanguageComitted", NTSTATUS, 0, {}
        },
        {
            "NtListenPort", NTSTATUS, 2,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"ConnectionRequest", DIR_OUT, PPORT_MESSAGE, "", }
            }
        },
        {
            "NtLoadDriver", NTSTATUS, 1,
            {
                {"DriverServiceName", DIR_IN, PUNICODE_STRING, "", }
            }
        },
        {
            "NtLoadKey2", NTSTATUS, 3,
            {
                {"TargetKey", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"SourceFile", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"Flags", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtLoadKeyEx", NTSTATUS, 4,
            {
                {"TargetKey", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"SourceFile", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"TrustClassKey", DIR_IN, HANDLE, "opt", }
            }
        },
        {
            "NtLoadKey", NTSTATUS, 2,
            {
                {"TargetKey", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"SourceFile", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtLockFile", NTSTATUS, 10,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"ByteOffset", DIR_IN, PLARGE_INTEGER, "", },
                {"Length", DIR_IN, PLARGE_INTEGER, "", },
                {"Key", DIR_IN, ULONG, "", },
                {"FailImmediately", DIR_IN, BOOLEAN, "", },
                {"ExclusiveLock", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtLockProductActivationKeys", NTSTATUS, 2,
            {
                {"*pPrivateVer", DIR_INOUT, ULONG, "opt", },
                {"*pSafeMode", DIR_OUT, ULONG, "opt", }
            }
        },
        {
            "NtLockRegistryKey", NTSTATUS, 1,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtLockVirtualMemory", NTSTATUS, 4,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"*BaseAddress", DIR_INOUT, PVOID, "", },
                {"RegionSize", DIR_INOUT, PSIZE_T, "", },
                {"MapType", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtMakePermanentObject", NTSTATUS, 1,
            {
                {"Handle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtMakeTemporaryObject", NTSTATUS, 1,
            {
                {"Handle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtMapCMFModule", NTSTATUS, 6,
            {
                {"What", DIR_IN, ULONG, "", },
                {"Index", DIR_IN, ULONG, "", },
                {"CacheIndexOut", DIR_OUT, PULONG, "opt", },
                {"CacheFlagsOut", DIR_OUT, PULONG, "opt", },
                {"ViewSizeOut", DIR_OUT, PULONG, "opt", },
                {"*BaseAddress", DIR_OUT, PVOID, "opt", }
            }
        },
        {
            "NtMapUserPhysicalPages", NTSTATUS, 3,
            {
                {"VirtualAddress", DIR_IN, PVOID, "", },
                {"NumberOfPages", DIR_IN, ULONG_PTR, "", },
                {"UserPfnArra;", DIR_IN, PULONG_PTR, "ecount_opt(NumberOfPages)", }
            }
        },
        {
            "NtMapUserPhysicalPagesScatter", NTSTATUS, 3,
            {
                {"*VirtualAddresses", DIR_IN, PVOID, "ecount(NumberOfPages)", },
                {"NumberOfPages", DIR_IN, ULONG_PTR, "", },
                {"UserPfnArray", DIR_IN, PULONG_PTR, "ecount_opt(NumberOfPages)", }
            }
        },
        {
            "NtMapViewOfSection", NTSTATUS, 10,
            {
                {"SectionHandle", DIR_IN, HANDLE, "", },
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"*BaseAddress", DIR_INOUT, PVOID, "", },
                {"ZeroBits", DIR_IN, ULONG_PTR, "", },
                {"CommitSize", DIR_IN, SIZE_T, "", },
                {"SectionOffset", DIR_INOUT, PLARGE_INTEGER, "opt", },
                {"ViewSize", DIR_INOUT, PSIZE_T, "", },
                {"InheritDisposition", DIR_IN, SECTION_INHERIT, "", },
                {"AllocationType", DIR_IN, ULONG, "", },
                {"Win32Protect", DIR_IN, WIN32_PROTECTION_MASK, "", }
            }
        },
        {
            "NtModifyBootEntry", NTSTATUS, 1,
            {
                {"BootEntry", DIR_IN, PBOOT_ENTRY, "", }
            }
        },
        {
            "NtModifyDriverEntry", NTSTATUS, 1,
            {
                {"DriverEntry", DIR_IN, PEFI_DRIVER_ENTRY, "", }
            }
        },
        {
            "NtNotifyChangeDirectoryFile", NTSTATUS, 9,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"Buffer", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"CompletionFilter", DIR_IN, ULONG, "", },
                {"WatchTree", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtNotifyChangeKey", NTSTATUS, 10,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"CompletionFilter", DIR_IN, ULONG, "", },
                {"WatchTree", DIR_IN, BOOLEAN, "", },
                {"Buffer", DIR_OUT, PVOID, "bcount_opt(BufferSize)", },
                {"BufferSize", DIR_IN, ULONG, "", },
                {"Asynchronous", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtNotifyChangeMultipleKeys", NTSTATUS, 12,
            {
                {"MasterKeyHandle", DIR_IN, HANDLE, "", },
                {"Count", DIR_IN, ULONG, "opt", },
                {"SlaveObjects[]", DIR_IN, OBJECT_ATTRIBUTES, "ecount_opt(Count)", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"CompletionFilter", DIR_IN, ULONG, "", },
                {"WatchTree", DIR_IN, BOOLEAN, "", },
                {"Buffer", DIR_OUT, PVOID, "bcount_opt(BufferSize)", },
                {"BufferSize", DIR_IN, ULONG, "", },
                {"Asynchronous", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtNotifyChangeSession", NTSTATUS, 8,
            {
                {"Session", DIR_IN, HANDLE, "", },
                {"IoStateSequence", DIR_IN, ULONG, "", },
                {"Reserved", DIR_IN, PVOID, "", },
                {"Action", DIR_IN, ULONG, "", },
                {"IoState", DIR_IN, IO_SESSION_STATE, "", },
                {"IoState2", DIR_IN, IO_SESSION_STATE, "", },
                {"Buffer", DIR_IN, PVOID, "", },
                {"BufferSize", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtOpenDirectoryObject", NTSTATUS, 3,
            {
                {"DirectoryHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenEnlistment", NTSTATUS, 5,
            {
                {"EnlistmentHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ResourceManagerHandle", DIR_IN, HANDLE, "", },
                {"EnlistmentGuid", DIR_IN, LPGUID, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", }
            }
        },
        {
            "NtOpenEvent", NTSTATUS, 3,
            {
                {"EventHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenEventPair", NTSTATUS, 3,
            {
                {"EventPairHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenFile", NTSTATUS, 6,
            {
                {"FileHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"ShareAccess", DIR_IN, ULONG, "", },
                {"OpenOptions", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtOpenIoCompletion", NTSTATUS, 3,
            {
                {"IoCompletionHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenJobObject", NTSTATUS, 3,
            {
                {"JobHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenKeyedEvent", NTSTATUS, 3,
            {
                {"KeyedEventHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenKeyEx", NTSTATUS, 4,
            {
                {"KeyHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"OpenOptions", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtOpenKey", NTSTATUS, 3,
            {
                {"KeyHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenKeyTransactedEx", NTSTATUS, 5,
            {
                {"KeyHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"OpenOptions", DIR_IN, ULONG, "", },
                {"TransactionHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtOpenKeyTransacted", NTSTATUS, 4,
            {
                {"KeyHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"TransactionHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtOpenMutant", NTSTATUS, 3,
            {
                {"MutantHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenObjectAuditAlarm", NTSTATUS, 12,
            {
                {"SubsystemName", DIR_IN, PUNICODE_STRING, "", },
                {"HandleId", DIR_IN, PVOID, "opt", },
                {"ObjectTypeName", DIR_IN, PUNICODE_STRING, "", },
                {"ObjectName", DIR_IN, PUNICODE_STRING, "", },
                {"SecurityDescriptor", DIR_IN, PSECURITY_DESCRIPTOR, "opt", },
                {"ClientToken", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"GrantedAccess", DIR_IN, ACCESS_MASK, "", },
                {"Privileges", DIR_IN, PPRIVILEGE_SET, "opt", },
                {"ObjectCreation", DIR_IN, BOOLEAN, "", },
                {"AccessGranted", DIR_IN, BOOLEAN, "", },
                {"GenerateOnClose", DIR_OUT, PBOOLEAN, "", }
            }
        },
        {
            "NtOpenPrivateNamespace", NTSTATUS, 4,
            {
                {"NamespaceHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"BoundaryDescriptor", DIR_IN, PVOID, "", }
            }
        },
        {
            "NtOpenProcess", NTSTATUS, 4,
            {
                {"ProcessHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"ClientId", DIR_IN, PCLIENT_ID, "opt", }
            }
        },
        {
            "NtOpenProcessTokenEx", NTSTATUS, 4,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"HandleAttributes", DIR_IN, ULONG, "", },
                {"TokenHandle", DIR_OUT, PHANDLE, "", }
            }
        },
        {
            "NtOpenProcessToken", NTSTATUS, 3,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"TokenHandle", DIR_OUT, PHANDLE, "", }
            }
        },
        {
            "NtOpenResourceManager", NTSTATUS, 5,
            {
                {"ResourceManagerHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"TmHandle", DIR_IN, HANDLE, "", },
                {"ResourceManagerGuid", DIR_IN, LPGUID, "opt", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", }
            }
        },
        {
            "NtOpenSection", NTSTATUS, 3,
            {
                {"SectionHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenSemaphore", NTSTATUS, 3,
            {
                {"SemaphoreHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenSession", NTSTATUS, 3,
            {
                {"SessionHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenSymbolicLinkObject", NTSTATUS, 3,
            {
                {"LinkHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenThread", NTSTATUS, 4,
            {
                {"ThreadHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"ClientId", DIR_IN, PCLIENT_ID, "opt", }
            }
        },
        {
            "NtOpenThreadTokenEx", NTSTATUS, 5,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"OpenAsSelf", DIR_IN, BOOLEAN, "", },
                {"HandleAttributes", DIR_IN, ULONG, "", },
                {"TokenHandle", DIR_OUT, PHANDLE, "", }
            }
        },
        {
            "NtOpenThreadToken", NTSTATUS, 4,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"OpenAsSelf", DIR_IN, BOOLEAN, "", },
                {"TokenHandle", DIR_OUT, PHANDLE, "", }
            }
        },
        {
            "NtOpenTimer", NTSTATUS, 3,
            {
                {"TimerHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtOpenTransactionManager", NTSTATUS, 6,
            {
                {"TmHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "opt", },
                {"LogFileName", DIR_IN, PUNICODE_STRING, "opt", },
                {"TmIdentity", DIR_IN, LPGUID, "opt", },
                {"OpenOptions", DIR_IN, ULONG, "opt", }
            }
        },
        {
            "NtOpenTransaction", NTSTATUS, 5,
            {
                {"TransactionHandle", DIR_OUT, PHANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"Uow", DIR_IN, LPGUID, "", },
                {"TmHandle", DIR_IN, HANDLE, "opt", }
            }
        },
        {
            "NtPlugPlayControl", NTSTATUS, 3,
            {
                {"PnPControlClass", DIR_IN, PLUGPLAY_CONTROL_CLASS, "", },
                {"PnPControlData", DIR_INOUT, PVOID, "bcount(PnPControlDataLength)", },
                {"PnPControlDataLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtPowerInformation", NTSTATUS, 5,
            {
                {"InformationLevel", DIR_IN, POWER_INFORMATION_LEVEL, "", },
                {"InputBuffer", DIR_IN, PVOID, "bcount_opt(InputBufferLength)", },
                {"InputBufferLength", DIR_IN, ULONG, "", },
                {"OutputBuffer", DIR_OUT, PVOID, "bcount_opt(OutputBufferLength)", },
                {"OutputBufferLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtPrepareComplete", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtPrepareEnlistment", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtPrePrepareComplete", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtPrePrepareEnlistment", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtPrivilegeCheck", NTSTATUS, 3,
            {
                {"ClientToken", DIR_IN, HANDLE, "", },
                {"RequiredPrivileges", DIR_INOUT, PPRIVILEGE_SET, "", },
                {"Result", DIR_OUT, PBOOLEAN, "", }
            }
        },
        {
            "NtPrivilegedServiceAuditAlarm", NTSTATUS, 5,
            {
                {"SubsystemName", DIR_IN, PUNICODE_STRING, "", },
                {"ServiceName", DIR_IN, PUNICODE_STRING, "", },
                {"ClientToken", DIR_IN, HANDLE, "", },
                {"Privileges", DIR_IN, PPRIVILEGE_SET, "", },
                {"AccessGranted", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtPrivilegeObjectAuditAlarm", NTSTATUS, 6,
            {
                {"SubsystemName", DIR_IN, PUNICODE_STRING, "", },
                {"HandleId", DIR_IN, PVOID, "opt", },
                {"ClientToken", DIR_IN, HANDLE, "", },
                {"DesiredAccess", DIR_IN, ACCESS_MASK, "", },
                {"Privileges", DIR_IN, PPRIVILEGE_SET, "", },
                {"AccessGranted", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtPropagationComplete", NTSTATUS, 4,
            {
                {"ResourceManagerHandle", DIR_IN, HANDLE, "", },
                {"RequestCookie", DIR_IN, ULONG, "", },
                {"BufferLength", DIR_IN, ULONG, "", },
                {"Buffer", DIR_IN, PVOID, "", }
            }
        },
        {
            "NtPropagationFailed", NTSTATUS, 3,
            {
                {"ResourceManagerHandle", DIR_IN, HANDLE, "", },
                {"RequestCookie", DIR_IN, ULONG, "", },
                {"PropStatus", DIR_IN, NTSTATUS, "", }
            }
        },
        {
            "NtProtectVirtualMemory", NTSTATUS, 5,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"*BaseAddress", DIR_INOUT, PVOID, "", },
                {"RegionSize", DIR_INOUT, PSIZE_T, "", },
                {"NewProtectWin32", DIR_IN, WIN32_PROTECTION_MASK, "", },
                {"OldProtect", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtPulseEvent", NTSTATUS, 2,
            {
                {"EventHandle", DIR_IN, HANDLE, "", },
                {"PreviousState", DIR_OUT, PLONG, "opt", }
            }
        },
        {
            "NtQueryAttributesFile", NTSTATUS, 2,
            {
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"FileInformation", DIR_OUT, PFILE_BASIC_INFORMATION, "", }
            }
        },
        {
            "NtQueryBootEntryOrder", NTSTATUS, 2,
            {
                {"Ids", DIR_OUT, PULONG, "ecount_opt(*Count)", },
                {"Count", DIR_INOUT, PULONG, "", }
            }
        },
        {
            "NtQueryBootOptions", NTSTATUS, 2,
            {
                {"BootOptions", DIR_OUT, PBOOT_OPTIONS, "bcount_opt(*BootOptionsLength)", },
                {"BootOptionsLength", DIR_INOUT, PULONG, "", }
            }
        },
        {
            "NtQueryDebugFilterState", NTSTATUS, 2,
            {
                {"ComponentId", DIR_IN, ULONG, "", },
                {"Level", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtQueryDefaultLocale", NTSTATUS, 2,
            {
                {"UserProfile", DIR_IN, BOOLEAN, "", },
                {"DefaultLocaleId", DIR_OUT, PLCID, "", }
            }
        },
        {
            "NtQueryDefaultUILanguage", NTSTATUS, 1,
            {
                {"*DefaultUILanguageId", DIR_OUT, LANGID, "", }
            }
        },
        {
            "NtQueryDirectoryFile", NTSTATUS, 11,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"FileInformation", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"FileInformationClass", DIR_IN, FILE_INFORMATION_CLASS, "", },
                {"ReturnSingleEntry", DIR_IN, BOOLEAN, "", },
                {"FileName", DIR_IN, PUNICODE_STRING, "", },
                {"RestartScan", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtQueryDirectoryObject", NTSTATUS, 7,
            {
                {"DirectoryHandle", DIR_IN, HANDLE, "", },
                {"Buffer", DIR_OUT, PVOID, "bcount_opt(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ReturnSingleEntry", DIR_IN, BOOLEAN, "", },
                {"RestartScan", DIR_IN, BOOLEAN, "", },
                {"Context", DIR_INOUT, PULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryDriverEntryOrder", NTSTATUS, 2,
            {
                {"Ids", DIR_OUT, PULONG, "ecount(*Count)", },
                {"Count", DIR_INOUT, PULONG, "", }
            }
        },
        {
            "NtQueryEaFile", NTSTATUS, 9,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"Buffer", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ReturnSingleEntry", DIR_IN, BOOLEAN, "", },
                {"EaList", DIR_IN, PVOID, "bcount_opt(EaListLength)", },
                {"EaListLength", DIR_IN, ULONG, "", },
                {"EaIndex", DIR_IN, PULONG, "opt", },
                {"RestartScan", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtQueryEvent", NTSTATUS, 5,
            {
                {"EventHandle", DIR_IN, HANDLE, "", },
                {"EventInformationClass", DIR_IN, EVENT_INFORMATION_CLASS, "", },
                {"EventInformation", DIR_OUT, PVOID, "bcount(EventInformationLength)", },
                {"EventInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryFullAttributesFile", NTSTATUS, 2,
            {
                {"ObjectAttributes", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"FileInformation", DIR_OUT, PFILE_NETWORK_OPEN_INFORMATION, "", }
            }
        },
        {
            "NtQueryInformationAtom", NTSTATUS, 5,
            {
                {"Atom", DIR_IN, RTL_ATOM, "", },
                {"InformationClass", DIR_IN, ATOM_INFORMATION_CLASS, "", },
                {"AtomInformation", DIR_OUT, PVOID, "bcount(AtomInformationLength)", },
                {"AtomInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryInformationEnlistment", NTSTATUS, 5,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"EnlistmentInformationClass", DIR_IN, ENLISTMENT_INFORMATION_CLASS, "", },
                {"EnlistmentInformation", DIR_OUT, PVOID, "bcount(EnlistmentInformationLength)", },
                {"EnlistmentInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryInformationFile", NTSTATUS, 5,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"FileInformation", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"FileInformationClass", DIR_IN, FILE_INFORMATION_CLASS, "", }
            }
        },
        {
            "NtQueryInformationJobObject", NTSTATUS, 5,
            {
                {"JobHandle", DIR_IN, HANDLE, "opt", },
                {"JobObjectInformationClass", DIR_IN, JOBOBJECTINFOCLASS, "", },
                {"JobObjectInformation", DIR_OUT, PVOID, "bcount(JobObjectInformationLength)", },
                {"JobObjectInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryInformationPort", NTSTATUS, 5,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"PortInformationClass", DIR_IN, PORT_INFORMATION_CLASS, "", },
                {"PortInformation", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryInformationProcess", NTSTATUS, 5,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"ProcessInformationClass", DIR_IN, PROCESSINFOCLASS, "", },
                {"ProcessInformation", DIR_OUT, PVOID, "bcount(ProcessInformationLength)", },
                {"ProcessInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryInformationResourceManager", NTSTATUS, 5,
            {
                {"ResourceManagerHandle", DIR_IN, HANDLE, "", },
                {"ResourceManagerInformationClass", DIR_IN, RESOURCEMANAGER_INFORMATION_CLASS, "", },
                {"ResourceManagerInformation", DIR_OUT, PVOID, "bcount(ResourceManagerInformationLength)", },
                {"ResourceManagerInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryInformationThread", NTSTATUS, 5,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"ThreadInformationClass", DIR_IN, THREADINFOCLASS, "", },
                {"ThreadInformation", DIR_OUT, PVOID, "bcount(ThreadInformationLength)", },
                {"ThreadInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryInformationToken", NTSTATUS, 5,
            {
                {"TokenHandle", DIR_IN, HANDLE, "", },
                {"TokenInformationClass", DIR_IN, TOKEN_INFORMATION_CLASS, "", },
                {"TokenInformation", DIR_OUT, PVOID, "bcount_part_opt(TokenInformationLength,*ReturnLength)", },
                {"TokenInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtQueryInformationTransaction", NTSTATUS, 5,
            {
                {"TransactionHandle", DIR_IN, HANDLE, "", },
                {"TransactionInformationClass", DIR_IN, TRANSACTION_INFORMATION_CLASS, "", },
                {"TransactionInformation", DIR_OUT, PVOID, "bcount(TransactionInformationLength)", },
                {"TransactionInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryInformationTransactionManager", NTSTATUS, 5,
            {
                {"TransactionManagerHandle", DIR_IN, HANDLE, "", },
                {"TransactionManagerInformationClass", DIR_IN, TRANSACTIONMANAGER_INFORMATION_CLASS, "", },
                {"TransactionManagerInformation", DIR_OUT, PVOID, "bcount(TransactionManagerInformationLength)", },
                {"TransactionManagerInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryInformationWorkerFactory", NTSTATUS, 5,
            {
                {"WorkerFactoryHandle", DIR_IN, HANDLE, "", },
                {"WorkerFactoryInformationClass", DIR_IN, WORKERFACTORYINFOCLASS, "", },
                {"WorkerFactoryInformation", DIR_OUT, PVOID, "bcount(WorkerFactoryInformationLength)", },
                {"WorkerFactoryInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryInstallUILanguage", NTSTATUS, 1,
            {
                {"*InstallUILanguageId", DIR_OUT, LANGID, "", }
            }
        },
        {
            "NtQueryIntervalProfile", NTSTATUS, 2,
            {
                {"ProfileSource", DIR_IN, KPROFILE_SOURCE, "", },
                {"Interval", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtQueryIoCompletion", NTSTATUS, 5,
            {
                {"IoCompletionHandle", DIR_IN, HANDLE, "", },
                {"IoCompletionInformationClass", DIR_IN, IO_COMPLETION_INFORMATION_CLASS, "", },
                {"IoCompletionInformation", DIR_OUT, PVOID, "bcount(IoCompletionInformationLength)", },
                {"IoCompletionInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryKey", NTSTATUS, 5,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"KeyInformationClass", DIR_IN, KEY_INFORMATION_CLASS, "", },
                {"KeyInformation", DIR_OUT, PVOID, "bcount_opt(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ResultLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtQueryLicenseValue", NTSTATUS, 5,
            {
                {"Name", DIR_IN, PUNICODE_STRING, "", },
                {"Type", DIR_OUT, PULONG, "opt", },
                {"Buffer", DIR_OUT, PVOID, "bcount(ReturnedLength)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ReturnedLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtQueryMultipleValueKey", NTSTATUS, 6,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"ValueEntries", DIR_INOUT, PKEY_VALUE_ENTRY, "ecount(EntryCount)", },
                {"EntryCount", DIR_IN, ULONG, "", },
                {"ValueBuffer", DIR_OUT, PVOID, "bcount(*BufferLength)", },
                {"BufferLength", DIR_INOUT, PULONG, "", },
                {"RequiredBufferLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryMutant", NTSTATUS, 5,
            {
                {"MutantHandle", DIR_IN, HANDLE, "", },
                {"MutantInformationClass", DIR_IN, MUTANT_INFORMATION_CLASS, "", },
                {"MutantInformation", DIR_OUT, PVOID, "bcount(MutantInformationLength)", },
                {"MutantInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryObject", NTSTATUS, 5,
            {
                {"Handle", DIR_IN, HANDLE, "", },
                {"ObjectInformationClass", DIR_IN, OBJECT_INFORMATION_CLASS, "", },
                {"ObjectInformation", DIR_OUT, PVOID, "bcount_opt(ObjectInformationLength)", },
                {"ObjectInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryOpenSubKeysEx", NTSTATUS, 4,
            {
                {"TargetKey", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"BufferLength", DIR_IN, ULONG, "", },
                {"Buffer", DIR_OUT, PVOID, "bcount(BufferLength)", },
                {"RequiredSize", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtQueryOpenSubKeys", NTSTATUS, 2,
            {
                {"TargetKey", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"HandleCount", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtQueryPerformanceCounter", NTSTATUS, 2,
            {
                {"PerformanceCounter", DIR_OUT, PLARGE_INTEGER, "", },
                {"PerformanceFrequency", DIR_OUT, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtQueryPortInformationProcess", NTSTATUS, 0, {}
        },
        {
            "NtQueryQuotaInformationFile", NTSTATUS, 9,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"Buffer", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ReturnSingleEntry", DIR_IN, BOOLEAN, "", },
                {"SidList", DIR_IN, PVOID, "bcount_opt(SidListLength)", },
                {"SidListLength", DIR_IN, ULONG, "", },
                {"StartSid", DIR_IN, PULONG, "opt", },
                {"RestartScan", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtQuerySection", NTSTATUS, 5,
            {
                {"SectionHandle", DIR_IN, HANDLE, "", },
                {"SectionInformationClass", DIR_IN, SECTION_INFORMATION_CLASS, "", },
                {"SectionInformation", DIR_OUT, PVOID, "bcount(SectionInformationLength)", },
                {"SectionInformationLength", DIR_IN, SIZE_T, "", },
                {"ReturnLength", DIR_OUT, PSIZE_T, "opt", }
            }
        },
        {
            "NtQuerySecurityAttributesToken", NTSTATUS, 6,
            {
                {"TokenHandle", DIR_IN, HANDLE, "", },
                {"Attributes", DIR_IN, PUNICODE_STRING, "ecount_opt(NumberOfAttributes)", },
                {"NumberOfAttributes", DIR_IN, ULONG, "", },
                {"Buffer", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtQuerySecurityObject", NTSTATUS, 5,
            {
                {"Handle", DIR_IN, HANDLE, "", },
                {"SecurityInformation", DIR_IN, SECURITY_INFORMATION, "", },
                {"SecurityDescriptor", DIR_OUT, PSECURITY_DESCRIPTOR, "bcount_opt(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"LengthNeeded", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtQuerySemaphore", NTSTATUS, 5,
            {
                {"SemaphoreHandle", DIR_IN, HANDLE, "", },
                {"SemaphoreInformationClass", DIR_IN, SEMAPHORE_INFORMATION_CLASS, "", },
                {"SemaphoreInformation", DIR_OUT, PVOID, "bcount(SemaphoreInformationLength)", },
                {"SemaphoreInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQuerySymbolicLinkObject", NTSTATUS, 3,
            {
                {"LinkHandle", DIR_IN, HANDLE, "", },
                {"LinkTarget", DIR_INOUT, PUNICODE_STRING, "", },
                {"ReturnedLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQuerySystemEnvironmentValueEx", NTSTATUS, 5,
            {
                {"VariableName", DIR_IN, PUNICODE_STRING, "", },
                {"VendorGuid", DIR_IN, LPGUID, "", },
                {"Value", DIR_OUT, PVOID, "bcount_opt(*ValueLength)", },
                {"ValueLength", DIR_INOUT, PULONG, "", },
                {"Attributes", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQuerySystemEnvironmentValue", NTSTATUS, 4,
            {
                {"VariableName", DIR_IN, PUNICODE_STRING, "", },
                {"VariableValue", DIR_OUT, PWSTR, "bcount(ValueLength)", },
                {"ValueLength", DIR_IN, USHORT, "", },
                {"ReturnLength", DIR_OUT, PUSHORT, "opt", }
            }
        },
        {
            "NtQuerySystemInformationEx", NTSTATUS, 6,
            {
                {"SystemInformationClass", DIR_IN, SYSTEM_INFORMATION_CLASS, "", },
                {"QueryInformation", DIR_IN, PVOID, "bcount(QueryInformationLength)", },
                {"QueryInformationLength", DIR_IN, ULONG, "", },
                {"SystemInformation", DIR_OUT, PVOID, "bcount_opt(SystemInformationLength)", },
                {"SystemInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQuerySystemInformation", NTSTATUS, 4,
            {
                {"SystemInformationClass", DIR_IN, SYSTEM_INFORMATION_CLASS, "", },
                {"SystemInformation", DIR_OUT, PVOID, "bcount_opt(SystemInformationLength)", },
                {"SystemInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQuerySystemTime", NTSTATUS, 1,
            {
                {"SystemTime", DIR_OUT, PLARGE_INTEGER, "", }
            }
        },
        {
            "NtQueryTimer", NTSTATUS, 5,
            {
                {"TimerHandle", DIR_IN, HANDLE, "", },
                {"TimerInformationClass", DIR_IN, TIMER_INFORMATION_CLASS, "", },
                {"TimerInformation", DIR_OUT, PVOID, "bcount(TimerInformationLength)", },
                {"TimerInformationLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtQueryTimerResolution", NTSTATUS, 3,
            {
                {"MaximumTime", DIR_OUT, PULONG, "", },
                {"MinimumTime", DIR_OUT, PULONG, "", },
                {"CurrentTime", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtQueryValueKey", NTSTATUS, 6,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"ValueName", DIR_IN, PUNICODE_STRING, "", },
                {"KeyValueInformationClass", DIR_IN, KEY_VALUE_INFORMATION_CLASS, "", },
                {"KeyValueInformation", DIR_OUT, PVOID, "bcount_opt(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ResultLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtQueryVirtualMemory", NTSTATUS, 6,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"BaseAddress", DIR_IN, PVOID, "", },
                {"MemoryInformationClass", DIR_IN, MEMORY_INFORMATION_CLASS, "", },
                {"MemoryInformation", DIR_OUT, PVOID, "bcount(MemoryInformationLength)", },
                {"MemoryInformationLength", DIR_IN, SIZE_T, "", },
                {"ReturnLength", DIR_OUT, PSIZE_T, "opt", }
            }
        },
        {
            "NtQueryVolumeInformationFile", NTSTATUS, 5,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"FsInformation", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"FsInformationClass", DIR_IN, FS_INFORMATION_CLASS, "", }
            }
        },
        {
            "NtQueueApcThreadEx", NTSTATUS, 6,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"UserApcReserveHandle", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PPS_APC_ROUTINE, "", },
                {"ApcArgument1", DIR_IN, PVOID, "opt", },
                {"ApcArgument2", DIR_IN, PVOID, "opt", },
                {"ApcArgument3", DIR_IN, PVOID, "opt", }
            }
        },
        {
            "NtQueueApcThread", NTSTATUS, 5,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"ApcRoutine", DIR_IN, PPS_APC_ROUTINE, "", },
                {"ApcArgument1", DIR_IN, PVOID, "opt", },
                {"ApcArgument2", DIR_IN, PVOID, "opt", },
                {"ApcArgument3", DIR_IN, PVOID, "opt", }
            }
        },
        {
            "NtRaiseException", NTSTATUS, 3,
            {
                {"ExceptionRecord", DIR_OUT, PEXCEPTION_RECORD, "", },
                {"ContextRecord", DIR_OUT, PCONTEXT, "", },
                {"FirstChance", DIR_OUT, BOOLEAN, "", }
            }
        },
        {
            "NtRaiseHardError", NTSTATUS, 6,
            {
                {"ErrorStatus", DIR_IN, NTSTATUS, "", },
                {"NumberOfParameters", DIR_IN, ULONG, "", },
                {"UnicodeStringParameterMask", DIR_IN, ULONG, "", },
                {"Parameters", DIR_IN, PULONG_PTR, "ecount(NumberOfParameters)", },
                {"ValidResponseOptions", DIR_IN, ULONG, "", },
                {"Response", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtReadFile", NTSTATUS, 9,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"Buffer", DIR_OUT, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ByteOffset", DIR_IN, PLARGE_INTEGER, "opt", },
                {"Key", DIR_IN, PULONG, "opt", }
            }
        },
        {
            "NtReadFileScatter", NTSTATUS, 9,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"SegmentArray", DIR_IN, PFILE_SEGMENT_ELEMENT, "", },
                {"Length", DIR_IN, ULONG, "", },
                {"ByteOffset", DIR_IN, PLARGE_INTEGER, "opt", },
                {"Key", DIR_IN, PULONG, "opt", }
            }
        },
        {
            "NtReadOnlyEnlistment", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtReadRequestData", NTSTATUS, 6,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Message", DIR_IN, PPORT_MESSAGE, "", },
                {"DataEntryIndex", DIR_IN, ULONG, "", },
                {"Buffer", DIR_OUT, PVOID, "bcount(BufferSize)", },
                {"BufferSize", DIR_IN, SIZE_T, "", },
                {"NumberOfBytesRead", DIR_OUT, PSIZE_T, "opt", }
            }
        },
        {
            "NtReadVirtualMemory", NTSTATUS, 5,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"BaseAddress", DIR_IN, PVOID, "opt", },
                {"Buffer", DIR_OUT, PVOID, "bcount(BufferSize)", },
                {"BufferSize", DIR_IN, SIZE_T, "", },
                {"NumberOfBytesRead", DIR_OUT, PSIZE_T, "opt", }
            }
        },
        {
            "NtRecoverEnlistment", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"EnlistmentKey", DIR_IN, PVOID, "opt", }
            }
        },
        {
            "NtRecoverResourceManager", NTSTATUS, 1,
            {
                {"ResourceManagerHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtRecoverTransactionManager", NTSTATUS, 1,
            {
                {"TransactionManagerHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtRegisterProtocolAddressInformation", NTSTATUS, 5,
            {
                {"ResourceManager", DIR_IN, HANDLE, "", },
                {"ProtocolId", DIR_IN, PCRM_PROTOCOL_ID, "", },
                {"ProtocolInformationSize", DIR_IN, ULONG, "", },
                {"ProtocolInformation", DIR_IN, PVOID, "", },
                {"CreateOptions", DIR_IN, ULONG, "opt", }
            }
        },
        {
            "NtRegisterThreadTerminatePort", NTSTATUS, 1,
            {
                {"PortHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtReleaseKeyedEvent", NTSTATUS, 4,
            {
                {"KeyedEventHandle", DIR_IN, HANDLE, "", },
                {"KeyValue", DIR_IN, PVOID, "", },
                {"Alertable", DIR_IN, BOOLEAN, "", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtReleaseMutant", NTSTATUS, 2,
            {
                {"MutantHandle", DIR_IN, HANDLE, "", },
                {"PreviousCount", DIR_OUT, PLONG, "opt", }
            }
        },
        {
            "NtReleaseSemaphore", NTSTATUS, 3,
            {
                {"SemaphoreHandle", DIR_IN, HANDLE, "", },
                {"ReleaseCount", DIR_IN, LONG, "", },
                {"PreviousCount", DIR_OUT, PLONG, "opt", }
            }
        },
        {
            "NtReleaseWorkerFactoryWorker", NTSTATUS, 1,
            {
                {"WorkerFactoryHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtRemoveIoCompletionEx", NTSTATUS, 6,
            {
                {"IoCompletionHandle", DIR_IN, HANDLE, "", },
                {"IoCompletionInformation", DIR_OUT, PFILE_IO_COMPLETION_INFORMATION, "ecount(Count)", },
                {"Count", DIR_IN, ULONG, "", },
                {"NumEntriesRemoved", DIR_OUT, PULONG, "", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", },
                {"Alertable", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtRemoveIoCompletion", NTSTATUS, 5,
            {
                {"IoCompletionHandle", DIR_IN, HANDLE, "", },
                {"*KeyContext", DIR_OUT, PVOID, "", },
                {"*ApcContext", DIR_OUT, PVOID, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtRemoveProcessDebug", NTSTATUS, 2,
            {
                {"ProcessHandle", DIR_OUT, HANDLE, "", },
                {"DebugObjectHandle", DIR_OUT, HANDLE, "", }
            }
        },
        {
            "NtRenameKey", NTSTATUS, 2,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"NewName", DIR_IN, PUNICODE_STRING, "", }
            }
        },
        {
            "NtRenameTransactionManager", NTSTATUS, 2,
            {
                {"LogFileName", DIR_IN, PUNICODE_STRING, "", },
                {"ExistingTransactionManagerGuid", DIR_IN, LPGUID, "", }
            }
        },
        {
            "NtReplaceKey", NTSTATUS, 3,
            {
                {"NewFile", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"TargetHandle", DIR_IN, HANDLE, "", },
                {"OldFile", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtReplacePartitionUnit", NTSTATUS, 3,
            {
                {"TargetInstancePath", DIR_IN, PUNICODE_STRING, "", },
                {"SpareInstancePath", DIR_IN, PUNICODE_STRING, "", },
                {"Flags", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtReplyPort", NTSTATUS, 2,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"ReplyMessage", DIR_IN, PPORT_MESSAGE, "", }
            }
        },
        {
            "NtReplyWaitReceivePortEx", NTSTATUS, 5,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"*PortContext", DIR_OUT, PVOID, "opt", },
                {"ReplyMessage", DIR_IN, PPORT_MESSAGE, "opt", },
                {"ReceiveMessage", DIR_OUT, PPORT_MESSAGE, "", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtReplyWaitReceivePort", NTSTATUS, 4,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"*PortContext", DIR_OUT, PVOID, "opt", },
                {"ReplyMessage", DIR_IN, PPORT_MESSAGE, "opt", },
                {"ReceiveMessage", DIR_OUT, PPORT_MESSAGE, "", }
            }
        },
        {
            "NtReplyWaitReplyPort", NTSTATUS, 2,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"ReplyMessage", DIR_INOUT, PPORT_MESSAGE, "", }
            }
        },
        {
            "NtRequestPort", NTSTATUS, 2,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"RequestMessage", DIR_IN, PPORT_MESSAGE, "", }
            }
        },
        {
            "NtRequestWaitReplyPort", NTSTATUS, 3,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"RequestMessage", DIR_IN, PPORT_MESSAGE, "", },
                {"ReplyMessage", DIR_OUT, PPORT_MESSAGE, "", }
            }
        },
        {
            "NtResetEvent", NTSTATUS, 2,
            {
                {"EventHandle", DIR_IN, HANDLE, "", },
                {"PreviousState", DIR_OUT, PLONG, "opt", }
            }
        },
        {
            "NtResetWriteWatch", NTSTATUS, 3,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"BaseAddress", DIR_IN, PVOID, "", },
                {"RegionSize", DIR_IN, SIZE_T, "", }
            }
        },
        {
            "NtRestoreKey", NTSTATUS, 3,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtResumeProcess", NTSTATUS, 1,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtResumeThread", NTSTATUS, 2,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"PreviousSuspendCount", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtRollbackComplete", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtRollbackEnlistment", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtRollbackTransaction", NTSTATUS, 2,
            {
                {"TransactionHandle", DIR_IN, HANDLE, "", },
                {"Wait", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtRollforwardTransactionManager", NTSTATUS, 2,
            {
                {"TransactionManagerHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtSaveKeyEx", NTSTATUS, 3,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Format", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSaveKey", NTSTATUS, 2,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"FileHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtSaveMergedKeys", NTSTATUS, 3,
            {
                {"HighPrecedenceKeyHandle", DIR_IN, HANDLE, "", },
                {"LowPrecedenceKeyHandle", DIR_IN, HANDLE, "", },
                {"FileHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtSecureConnectPort", NTSTATUS, 9,
            {
                {"PortHandle", DIR_OUT, PHANDLE, "", },
                {"PortName", DIR_IN, PUNICODE_STRING, "", },
                {"SecurityQos", DIR_IN, PSECURITY_QUALITY_OF_SERVICE, "", },
                {"ClientView", DIR_INOUT, PPORT_VIEW, "opt", },
                {"RequiredServerSid", DIR_IN, PSID, "opt", },
                {"ServerView", DIR_INOUT, PREMOTE_PORT_VIEW, "opt", },
                {"MaxMessageLength", DIR_OUT, PULONG, "opt", },
                {"ConnectionInformation", DIR_INOUT, PVOID, "opt", },
                {"ConnectionInformationLength", DIR_INOUT, PULONG, "opt", }
            }
        },
        {
            "NtSerializeBoot", NTSTATUS, 0, {}
        },
        {
            "NtSetBootEntryOrder", NTSTATUS, 2,
            {
                {"Ids", DIR_IN, PULONG, "ecount(Count)", },
                {"Count", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetBootOptions", NTSTATUS, 2,
            {
                {"BootOptions", DIR_IN, PBOOT_OPTIONS, "", },
                {"FieldsToChange", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetContextThread", NTSTATUS, 2,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"ThreadContext", DIR_IN, PCONTEXT, "", }
            }
        },
        {
            "NtSetDebugFilterState", NTSTATUS, 3,
            {
                {"ComponentId", DIR_IN, ULONG, "", },
                {"Level", DIR_IN, ULONG, "", },
                {"State", DIR_IN, BOOLEAN, "", }
            }
        },
        {
            "NtSetDefaultHardErrorPort", NTSTATUS, 1,
            {
                {"DefaultHardErrorPort", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtSetDefaultLocale", NTSTATUS, 2,
            {
                {"UserProfile", DIR_IN, BOOLEAN, "", },
                {"DefaultLocaleId", DIR_IN, LCID, "", }
            }
        },
        {
            "NtSetDefaultUILanguage", NTSTATUS, 1,
            {
                {"DefaultUILanguageId", DIR_IN, LANGID, "", }
            }
        },
        {
            "NtSetDriverEntryOrder", NTSTATUS, 2,
            {
                {"Ids", DIR_IN, PULONG, "ecount(Count)", },
                {"Count", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetEaFile", NTSTATUS, 4,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"Buffer", DIR_IN, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetEventBoostPriority", NTSTATUS, 1,
            {
                {"EventHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtSetEvent", NTSTATUS, 2,
            {
                {"EventHandle", DIR_IN, HANDLE, "", },
                {"PreviousState", DIR_OUT, PLONG, "opt", }
            }
        },
        {
            "NtSetHighEventPair", NTSTATUS, 1,
            {
                {"EventPairHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtSetHighWaitLowEventPair", NTSTATUS, 1,
            {
                {"EventPairHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtSetInformationDebugObject", NTSTATUS, 5,
            {
                {"DebugObjectHandle", DIR_OUT, HANDLE, "", },
                {"DebugObjectInformationClass", DIR_OUT, DEBUGOBJECTINFOCLASS, "", },
                {"DebugInformation", DIR_OUT, PVOID, "", },
                {"DebugInformationLength", DIR_OUT, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtSetInformationEnlistment", NTSTATUS, 4,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "opt", },
                {"EnlistmentInformationClass", DIR_IN, ENLISTMENT_INFORMATION_CLASS, "", },
                {"EnlistmentInformation", DIR_IN, PVOID, "bcount(EnlistmentInformationLength)", },
                {"EnlistmentInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetInformationFile", NTSTATUS, 5,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"FileInformation", DIR_IN, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"FileInformationClass", DIR_IN, FILE_INFORMATION_CLASS, "", }
            }
        },
        {
            "NtSetInformationJobObject", NTSTATUS, 4,
            {
                {"JobHandle", DIR_IN, HANDLE, "", },
                {"JobObjectInformationClass", DIR_IN, JOBOBJECTINFOCLASS, "", },
                {"JobObjectInformation", DIR_IN, PVOID, "bcount(JobObjectInformationLength)", },
                {"JobObjectInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetInformationKey", NTSTATUS, 4,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"KeySetInformationClass", DIR_IN, KEY_SET_INFORMATION_CLASS, "", },
                {"KeySetInformation", DIR_IN, PVOID, "bcount(KeySetInformationLength)", },
                {"KeySetInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetInformationObject", NTSTATUS, 4,
            {
                {"Handle", DIR_IN, HANDLE, "", },
                {"ObjectInformationClass", DIR_IN, OBJECT_INFORMATION_CLASS, "", },
                {"ObjectInformation", DIR_IN, PVOID, "bcount(ObjectInformationLength)", },
                {"ObjectInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetInformationProcess", NTSTATUS, 4,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"ProcessInformationClass", DIR_IN, PROCESSINFOCLASS, "", },
                {"ProcessInformation", DIR_IN, PVOID, "bcount(ProcessInformationLength)", },
                {"ProcessInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetInformationResourceManager", NTSTATUS, 4,
            {
                {"ResourceManagerHandle", DIR_IN, HANDLE, "", },
                {"ResourceManagerInformationClass", DIR_IN, RESOURCEMANAGER_INFORMATION_CLASS, "", },
                {"ResourceManagerInformation", DIR_IN, PVOID, "bcount(ResourceManagerInformationLength)", },
                {"ResourceManagerInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetInformationThread", NTSTATUS, 4,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"ThreadInformationClass", DIR_IN, THREADINFOCLASS, "", },
                {"ThreadInformation", DIR_IN, PVOID, "bcount(ThreadInformationLength)", },
                {"ThreadInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetInformationToken", NTSTATUS, 4,
            {
                {"TokenHandle", DIR_IN, HANDLE, "", },
                {"TokenInformationClass", DIR_IN, TOKEN_INFORMATION_CLASS, "", },
                {"TokenInformation", DIR_IN, PVOID, "bcount(TokenInformationLength)", },
                {"TokenInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetInformationTransaction", NTSTATUS, 4,
            {
                {"TransactionHandle", DIR_IN, HANDLE, "", },
                {"TransactionInformationClass", DIR_IN, TRANSACTION_INFORMATION_CLASS, "", },
                {"TransactionInformation", DIR_IN, PVOID, "bcount(TransactionInformationLength)", },
                {"TransactionInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetInformationTransactionManager", NTSTATUS, 4,
            {
                {"TmHandle", DIR_IN, HANDLE, "opt", },
                {"TransactionManagerInformationClass", DIR_IN, TRANSACTIONMANAGER_INFORMATION_CLASS, "", },
                {"TransactionManagerInformation", DIR_IN, PVOID, "bcount(TransactionManagerInformationLength)", },
                {"TransactionManagerInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetInformationWorkerFactory", NTSTATUS, 4,
            {
                {"WorkerFactoryHandle", DIR_IN, HANDLE, "", },
                {"WorkerFactoryInformationClass", DIR_IN, WORKERFACTORYINFOCLASS, "", },
                {"WorkerFactoryInformation", DIR_IN, PVOID, "bcount(WorkerFactoryInformationLength)", },
                {"WorkerFactoryInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetIntervalProfile", NTSTATUS, 2,
            {
                {"Interval", DIR_IN, ULONG, "", },
                {"Source", DIR_IN, KPROFILE_SOURCE, "", }
            }
        },
        {
            "NtSetIoCompletionEx", NTSTATUS, 6,
            {
                {"IoCompletionHandle", DIR_IN, HANDLE, "", },
                {"IoCompletionReserveHandle", DIR_IN, HANDLE, "", },
                {"KeyContext", DIR_IN, PVOID, "", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatus", DIR_IN, NTSTATUS, "", },
                {"IoStatusInformation", DIR_IN, ULONG_PTR, "", }
            }
        },
        {
            "NtSetIoCompletion", NTSTATUS, 5,
            {
                {"IoCompletionHandle", DIR_IN, HANDLE, "", },
                {"KeyContext", DIR_IN, PVOID, "", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatus", DIR_IN, NTSTATUS, "", },
                {"IoStatusInformation", DIR_IN, ULONG_PTR, "", }
            }
        },
        {
            "NtSetLdtEntries", NTSTATUS, 6,
            {
                {"Selector0", DIR_IN, ULONG, "", },
                {"Entry0Low", DIR_IN, ULONG, "", },
                {"Entry0Hi", DIR_IN, ULONG, "", },
                {"Selector1", DIR_IN, ULONG, "", },
                {"Entry1Low", DIR_IN, ULONG, "", },
                {"Entry1Hi", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetLowEventPair", NTSTATUS, 1,
            {
                {"EventPairHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtSetLowWaitHighEventPair", NTSTATUS, 1,
            {
                {"EventPairHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtSetQuotaInformationFile", NTSTATUS, 4,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"Buffer", DIR_IN, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetSecurityObject", NTSTATUS, 3,
            {
                {"Handle", DIR_IN, HANDLE, "", },
                {"SecurityInformation", DIR_IN, SECURITY_INFORMATION, "", },
                {"SecurityDescriptor", DIR_IN, PSECURITY_DESCRIPTOR, "", }
            }
        },
        {
            "NtSetSystemEnvironmentValueEx", NTSTATUS, 5,
            {
                {"VariableName", DIR_IN, PUNICODE_STRING, "", },
                {"VendorGuid", DIR_IN, LPGUID, "", },
                {"Value", DIR_IN, PVOID, "bcount_opt(ValueLength)", },
                {"ValueLength", DIR_IN, ULONG, "", },
                {"Attributes", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetSystemEnvironmentValue", NTSTATUS, 2,
            {
                {"VariableName", DIR_IN, PUNICODE_STRING, "", },
                {"VariableValue", DIR_IN, PUNICODE_STRING, "", }
            }
        },
        {
            "NtSetSystemInformation", NTSTATUS, 3,
            {
                {"SystemInformationClass", DIR_IN, SYSTEM_INFORMATION_CLASS, "", },
                {"SystemInformation", DIR_IN, PVOID, "bcount_opt(SystemInformationLength)", },
                {"SystemInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetSystemPowerState", NTSTATUS, 3,
            {
                {"SystemAction", DIR_IN, POWER_ACTION, "", },
                {"MinSystemState", DIR_IN, SYSTEM_POWER_STATE, "", },
                {"Flags", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetSystemTime", NTSTATUS, 2,
            {
                {"SystemTime", DIR_IN, PLARGE_INTEGER, "opt", },
                {"PreviousTime", DIR_OUT, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtSetThreadExecutionState", NTSTATUS, 2,
            {
                {"esFlags", DIR_IN, EXECUTION_STATE, "", },
                {"*PreviousFlags", DIR_OUT, EXECUTION_STATE, "", }
            }
        },
        {
            "NtSetTimerEx", NTSTATUS, 4,
            {
                {"TimerHandle", DIR_IN, HANDLE, "", },
                {"TimerSetInformationClass", DIR_IN, TIMER_SET_INFORMATION_CLASS, "", },
                {"TimerSetInformation", DIR_INOUT, PVOID, "bcount(TimerSetInformationLength)", },
                {"TimerSetInformationLength", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetTimer", NTSTATUS, 7,
            {
                {"TimerHandle", DIR_IN, HANDLE, "", },
                {"DueTime", DIR_IN, PLARGE_INTEGER, "", },
                {"TimerApcRoutine", DIR_IN, PTIMER_APC_ROUTINE, "opt", },
                {"TimerContext", DIR_IN, PVOID, "opt", },
                {"WakeTimer", DIR_IN, BOOLEAN, "", },
                {"Period", DIR_IN, LONG, "opt", },
                {"PreviousState", DIR_OUT, PBOOLEAN, "opt", }
            }
        },
        {
            "NtSetTimerResolution", NTSTATUS, 3,
            {
                {"DesiredTime", DIR_IN, ULONG, "", },
                {"SetResolution", DIR_IN, BOOLEAN, "", },
                {"ActualTime", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtSetUuidSeed", NTSTATUS, 1,
            {
                {"Seed", DIR_IN, PCHAR, "", }
            }
        },
        {
            "NtSetValueKey", NTSTATUS, 6,
            {
                {"KeyHandle", DIR_IN, HANDLE, "", },
                {"ValueName", DIR_IN, PUNICODE_STRING, "", },
                {"TitleIndex", DIR_IN, ULONG, "opt", },
                {"Type", DIR_IN, ULONG, "", },
                {"Data", DIR_IN, PVOID, "bcount_opt(DataSize)", },
                {"DataSize", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtSetVolumeInformationFile", NTSTATUS, 5,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"FsInformation", DIR_IN, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"FsInformationClass", DIR_IN, FS_INFORMATION_CLASS, "", }
            }
        },
        {
            "NtShutdownSystem", NTSTATUS, 1,
            {
                {"Action", DIR_IN, SHUTDOWN_ACTION, "", }
            }
        },
        {
            "NtShutdownWorkerFactory", NTSTATUS, 2,
            {
                {"WorkerFactoryHandle", DIR_IN, HANDLE, "", },
                {"*PendingWorkerCount", DIR_INOUT, LONG, "", }
            }
        },
        {
            "NtSignalAndWaitForSingleObject", NTSTATUS, 4,
            {
                {"SignalHandle", DIR_IN, HANDLE, "", },
                {"WaitHandle", DIR_IN, HANDLE, "", },
                {"Alertable", DIR_IN, BOOLEAN, "", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtSinglePhaseReject", NTSTATUS, 2,
            {
                {"EnlistmentHandle", DIR_IN, HANDLE, "", },
                {"TmVirtualClock", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtStartProfile", NTSTATUS, 1,
            {
                {"ProfileHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtStopProfile", NTSTATUS, 1,
            {
                {"ProfileHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtSuspendProcess", NTSTATUS, 1,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtSuspendThread", NTSTATUS, 2,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "", },
                {"PreviousSuspendCount", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtSystemDebugControl", NTSTATUS, 6,
            {
                {"Command", DIR_IN, SYSDBG_COMMAND, "", },
                {"InputBuffer", DIR_INOUT, PVOID, "bcount_opt(InputBufferLength)", },
                {"InputBufferLength", DIR_IN, ULONG, "", },
                {"OutputBuffer", DIR_OUT, PVOID, "bcount_opt(OutputBufferLength)", },
                {"OutputBufferLength", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "opt", }
            }
        },
        {
            "NtTerminateJobObject", NTSTATUS, 2,
            {
                {"JobHandle", DIR_IN, HANDLE, "", },
                {"ExitStatus", DIR_IN, NTSTATUS, "", }
            }
        },
        {
            "NtTerminateProcess", NTSTATUS, 2,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "opt", },
                {"ExitStatus", DIR_IN, NTSTATUS, "", }
            }
        },
        {
            "NtTerminateThread", NTSTATUS, 2,
            {
                {"ThreadHandle", DIR_IN, HANDLE, "opt", },
                {"ExitStatus", DIR_IN, NTSTATUS, "", }
            }
        },
        {
            "NtTestAlert", NTSTATUS, 0, {}
        },
        {
            "NtThawRegistry", NTSTATUS, 0, {}
        },
        {
            "NtThawTransactions", NTSTATUS, 0, {}
        },
        {
            "NtTraceControl", NTSTATUS, 6,
            {
                {"FunctionCode", DIR_IN, ULONG, "", },
                {"InBuffer", DIR_IN, PVOID, "bcount_opt(InBufferLen)", },
                {"InBufferLen", DIR_IN, ULONG, "", },
                {"OutBuffer", DIR_OUT, PVOID, "bcount_opt(OutBufferLen)", },
                {"OutBufferLen", DIR_IN, ULONG, "", },
                {"ReturnLength", DIR_OUT, PULONG, "", }
            }
        },
        {
            "NtTraceEvent", NTSTATUS, 4,
            {
                {"TraceHandle", DIR_IN, HANDLE, "", },
                {"Flags", DIR_IN, ULONG, "", },
                {"FieldSize", DIR_IN, ULONG, "", },
                {"Fields", DIR_IN, PVOID, "", }
            }
        },
        {
            "NtTranslateFilePath", NTSTATUS, 4,
            {
                {"InputFilePath", DIR_IN, PFILE_PATH, "", },
                {"OutputType", DIR_IN, ULONG, "", },
                {"OutputFilePath", DIR_OUT, PFILE_PATH, "bcount_opt(*OutputFilePathLength)", },
                {"OutputFilePathLength", DIR_INOUT, PULONG, "opt", }
            }
        },
        {
            "NtUmsThreadYield", NTSTATUS, 0, {}
        },
        {
            "NtUnloadDriver", NTSTATUS, 1,
            {
                {"DriverServiceName", DIR_IN, PUNICODE_STRING, "", }
            }
        },
        {
            "NtUnloadKey2", NTSTATUS, 2,
            {
                {"TargetKey", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"Flags", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtUnloadKeyEx", NTSTATUS, 2,
            {
                {"TargetKey", DIR_IN, POBJECT_ATTRIBUTES, "", },
                {"Event", DIR_IN, HANDLE, "opt", }
            }
        },
        {
            "NtUnloadKey", NTSTATUS, 1,
            {
                {"TargetKey", DIR_IN, POBJECT_ATTRIBUTES, "", }
            }
        },
        {
            "NtUnlockFile", NTSTATUS, 5,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"ByteOffset", DIR_IN, PLARGE_INTEGER, "", },
                {"Length", DIR_IN, PLARGE_INTEGER, "", },
                {"Key", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtUnlockVirtualMemory", NTSTATUS, 4,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"*BaseAddress", DIR_INOUT, PVOID, "", },
                {"RegionSize", DIR_INOUT, PSIZE_T, "", },
                {"MapType", DIR_IN, ULONG, "", }
            }
        },
        {
            "NtUnmapViewOfSection", NTSTATUS, 2,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"BaseAddress", DIR_IN, PVOID, "", }
            }
        },
        {
            "NtVdmControl", NTSTATUS, 2,
            {
                {"Service", DIR_IN, VDMSERVICECLASS, "", },
                {"ServiceData", DIR_INOUT, PVOID, "", }
            }
        },
        {
            "NtWaitForDebugEvent", NTSTATUS, 4,
            {
                {"DebugObjectHandle", DIR_OUT, HANDLE, "", },
                {"Alertable", DIR_OUT, BOOLEAN, "", },
                {"Timeout", DIR_OUT, PLARGE_INTEGER, "", },
                {"WaitStateChange", DIR_OUT, PDBGUI_WAIT_STATE_CHANGE, "", }
            }
        },
        {
            "NtWaitForKeyedEvent", NTSTATUS, 4,
            {
                {"KeyedEventHandle", DIR_IN, HANDLE, "", },
                {"KeyValue", DIR_IN, PVOID, "", },
                {"Alertable", DIR_IN, BOOLEAN, "", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtWaitForMultipleObjects32", NTSTATUS, 5,
            {
                {"Count", DIR_IN, ULONG, "", },
                {"Handles[]", DIR_IN, LONG, "ecount(Count)", },
                {"WaitType", DIR_IN, WAIT_TYPE, "", },
                {"Alertable", DIR_IN, BOOLEAN, "", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtWaitForMultipleObjects", NTSTATUS, 5,
            {
                {"Count", DIR_IN, ULONG, "", },
                {"Handles[]", DIR_IN, HANDLE, "ecount(Count)", },
                {"WaitType", DIR_IN, WAIT_TYPE, "", },
                {"Alertable", DIR_IN, BOOLEAN, "", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtWaitForSingleObject", NTSTATUS, 3,
            {
                {"Handle", DIR_IN, HANDLE, "", },
                {"Alertable", DIR_IN, BOOLEAN, "", },
                {"Timeout", DIR_IN, PLARGE_INTEGER, "opt", }
            }
        },
        {
            "NtWaitForWorkViaWorkerFactory", NTSTATUS, 2,
            {
                {"WorkerFactoryHandle", DIR_IN, HANDLE, "", },
                {"MiniPacket", DIR_OUT, PFILE_IO_COMPLETION_INFORMATION, "", }
            }
        },
        {
            "NtWaitHighEventPair", NTSTATUS, 1,
            {
                {"EventPairHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtWaitLowEventPair", NTSTATUS, 1,
            {
                {"EventPairHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtWorkerFactoryWorkerReady", NTSTATUS, 1,
            {
                {"WorkerFactoryHandle", DIR_IN, HANDLE, "", }
            }
        },
        {
            "NtWriteFileGather", NTSTATUS, 9,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"SegmentArray", DIR_IN, PFILE_SEGMENT_ELEMENT, "", },
                {"Length", DIR_IN, ULONG, "", },
                {"ByteOffset", DIR_IN, PLARGE_INTEGER, "opt", },
                {"Key", DIR_IN, PULONG, "opt", }
            }
        },
        {
            "NtWriteFile", NTSTATUS, 9,
            {
                {"FileHandle", DIR_IN, HANDLE, "", },
                {"Event", DIR_IN, HANDLE, "opt", },
                {"ApcRoutine", DIR_IN, PIO_APC_ROUTINE, "opt", },
                {"ApcContext", DIR_IN, PVOID, "opt", },
                {"IoStatusBlock", DIR_OUT, PIO_STATUS_BLOCK, "", },
                {"Buffer", DIR_IN, PVOID, "bcount(Length)", },
                {"Length", DIR_IN, ULONG, "", },
                {"ByteOffset", DIR_IN, PLARGE_INTEGER, "opt", },
                {"Key", DIR_IN, PULONG, "opt", }
            }
        },
        {
            "NtWriteRequestData", NTSTATUS, 6,
            {
                {"PortHandle", DIR_IN, HANDLE, "", },
                {"Message", DIR_IN, PPORT_MESSAGE, "", },
                {"DataEntryIndex", DIR_IN, ULONG, "", },
                {"Buffer", DIR_IN, PVOID, "bcount(BufferSize)", },
                {"BufferSize", DIR_IN, SIZE_T, "", },
                {"NumberOfBytesWritten", DIR_OUT, PSIZE_T, "opt", }
            }
        },
        {
            "NtWriteVirtualMemory", NTSTATUS, 5,
            {
                {"ProcessHandle", DIR_IN, HANDLE, "", },
                {"BaseAddress", DIR_IN, PVOID, "opt", },
                {"Buffer", DIR_IN, PVOID, "bcount(BufferSize)", },
                {"BufferSize", DIR_IN, SIZE_T, "", },
                {"NumberOfBytesWritten", DIR_OUT, PSIZE_T, "opt", }
            }
        },
        {
            "NtYieldExecution", NTSTATUS, 0, {}
        }
    };

    d_->syscall_protos.insert(d_->syscall_protos.begin(), std::begin(win_syscalls), std::end(win_syscalls));

    return true;
}

opt<winscproto::win_syscall_t> winscproto::WinScProto::find(std::string name){

    int syscall_index = -1;
    for (int j=0; j<NUM_SYSCALLS; j++)
    {
        if (name.compare(d_->syscall_protos[j].name) != 0)
            continue;

        syscall_index=j;
        break;
    }

    if (syscall_index < 0)
        FAIL(ext::nullopt, "Unable to find prototype");

    return d_->syscall_protos[syscall_index];
}
