#pragma once

#include "nt/nt.hpp"


#define DECLARE_SYSCALLS_CALLBACK_PROTOS\
    using on_NtAcceptConnectPort_fn                                = std::function<nt::NTSTATUS(nt::PHANDLE, nt::PVOID, nt::PPORT_MESSAGE, nt::BOOLEAN, nt::PPORT_VIEW, nt::PREMOTE_PORT_VIEW)>;\
    using on_NtAccessCheckAndAuditAlarm_fn                         = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PVOID, nt::PUNICODE_STRING, nt::PUNICODE_STRING, nt::PSECURITY_DESCRIPTOR, nt::ACCESS_MASK, nt::PGENERIC_MAPPING, nt::BOOLEAN, nt::PACCESS_MASK, nt::PNTSTATUS, nt::PBOOLEAN)>;\
    using on_NtAccessCheckByTypeAndAuditAlarm_fn                   = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PVOID, nt::PUNICODE_STRING, nt::PUNICODE_STRING, nt::PSECURITY_DESCRIPTOR, nt::PSID, nt::ACCESS_MASK, nt::AUDIT_EVENT_TYPE, nt::ULONG, nt::POBJECT_TYPE_LIST, nt::ULONG, nt::PGENERIC_MAPPING, nt::BOOLEAN, nt::PACCESS_MASK, nt::PNTSTATUS, nt::PBOOLEAN)>;\
    using on_NtAccessCheckByType_fn                                = std::function<nt::NTSTATUS(nt::PSECURITY_DESCRIPTOR, nt::PSID, nt::HANDLE, nt::ACCESS_MASK, nt::POBJECT_TYPE_LIST, nt::ULONG, nt::PGENERIC_MAPPING, nt::PPRIVILEGE_SET, nt::PULONG, nt::PACCESS_MASK, nt::PNTSTATUS)>;\
    using on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle_fn = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PVOID, nt::HANDLE, nt::PUNICODE_STRING, nt::PUNICODE_STRING, nt::PSECURITY_DESCRIPTOR, nt::PSID, nt::ACCESS_MASK, nt::AUDIT_EVENT_TYPE, nt::ULONG, nt::POBJECT_TYPE_LIST, nt::ULONG, nt::PGENERIC_MAPPING, nt::BOOLEAN, nt::PACCESS_MASK, nt::PNTSTATUS, nt::PBOOLEAN)>;\
    using on_NtAccessCheckByTypeResultListAndAuditAlarm_fn         = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PVOID, nt::PUNICODE_STRING, nt::PUNICODE_STRING, nt::PSECURITY_DESCRIPTOR, nt::PSID, nt::ACCESS_MASK, nt::AUDIT_EVENT_TYPE, nt::ULONG, nt::POBJECT_TYPE_LIST, nt::ULONG, nt::PGENERIC_MAPPING, nt::BOOLEAN, nt::PACCESS_MASK, nt::PNTSTATUS, nt::PBOOLEAN)>;\
    using on_NtAccessCheckByTypeResultList_fn                      = std::function<nt::NTSTATUS(nt::PSECURITY_DESCRIPTOR, nt::PSID, nt::HANDLE, nt::ACCESS_MASK, nt::POBJECT_TYPE_LIST, nt::ULONG, nt::PGENERIC_MAPPING, nt::PPRIVILEGE_SET, nt::PULONG, nt::PACCESS_MASK, nt::PNTSTATUS)>;\
    using on_NtAccessCheck_fn                                      = std::function<nt::NTSTATUS(nt::PSECURITY_DESCRIPTOR, nt::HANDLE, nt::ACCESS_MASK, nt::PGENERIC_MAPPING, nt::PPRIVILEGE_SET, nt::PULONG, nt::PACCESS_MASK, nt::PNTSTATUS)>;\
    using on_NtAddAtom_fn                                          = std::function<nt::NTSTATUS(nt::PWSTR, nt::ULONG, nt::PRTL_ATOM)>;\
    using on_NtAddBootEntry_fn                                     = std::function<nt::NTSTATUS(nt::PBOOT_ENTRY, nt::PULONG)>;\
    using on_NtAddDriverEntry_fn                                   = std::function<nt::NTSTATUS(nt::PEFI_DRIVER_ENTRY, nt::PULONG)>;\
    using on_NtAdjustGroupsToken_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::BOOLEAN, nt::PTOKEN_GROUPS, nt::ULONG, nt::PTOKEN_GROUPS, nt::PULONG)>;\
    using on_NtAdjustPrivilegesToken_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::BOOLEAN, nt::PTOKEN_PRIVILEGES, nt::ULONG, nt::PTOKEN_PRIVILEGES, nt::PULONG)>;\
    using on_NtAlertResumeThread_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::PULONG)>;\
    using on_NtAlertThread_fn                                      = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtAllocateLocallyUniqueId_fn                          = std::function<nt::NTSTATUS(nt::PLUID)>;\
    using on_NtAllocateReserveObject_fn                            = std::function<nt::NTSTATUS(nt::PHANDLE, nt::POBJECT_ATTRIBUTES, nt::MEMORY_RESERVE_TYPE)>;\
    using on_NtAllocateUserPhysicalPages_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::PULONG_PTR, nt::PULONG_PTR)>;\
    using on_NtAllocateUuids_fn                                    = std::function<nt::NTSTATUS(nt::PULARGE_INTEGER, nt::PULONG, nt::PULONG, nt::PCHAR)>;\
    using on_NtAllocateVirtualMemory_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::ULONG_PTR, nt::PSIZE_T, nt::ULONG, nt::ULONG)>;\
    using on_NtAlpcAcceptConnectPort_fn                            = std::function<nt::NTSTATUS(nt::PHANDLE, nt::HANDLE, nt::ULONG, nt::POBJECT_ATTRIBUTES, nt::PALPC_PORT_ATTRIBUTES, nt::PVOID, nt::PPORT_MESSAGE, nt::PALPC_MESSAGE_ATTRIBUTES, nt::BOOLEAN)>;\
    using on_NtAlpcCancelMessage_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::PALPC_CONTEXT_ATTR)>;\
    using on_NtAlpcConnectPort_fn                                  = std::function<nt::NTSTATUS(nt::PHANDLE, nt::PUNICODE_STRING, nt::POBJECT_ATTRIBUTES, nt::PALPC_PORT_ATTRIBUTES, nt::ULONG, nt::PSID, nt::PPORT_MESSAGE, nt::PULONG, nt::PALPC_MESSAGE_ATTRIBUTES, nt::PALPC_MESSAGE_ATTRIBUTES, nt::PLARGE_INTEGER)>;\
    using on_NtAlpcCreatePort_fn                                   = std::function<nt::NTSTATUS(nt::PHANDLE, nt::POBJECT_ATTRIBUTES, nt::PALPC_PORT_ATTRIBUTES)>;\
    using on_NtAlpcCreatePortSection_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::HANDLE, nt::SIZE_T, nt::PALPC_HANDLE, nt::PSIZE_T)>;\
    using on_NtAlpcCreateResourceReserve_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::SIZE_T, nt::PALPC_HANDLE)>;\
    using on_NtAlpcCreateSectionView_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::PALPC_DATA_VIEW_ATTR)>;\
    using on_NtAlpcCreateSecurityContext_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::PALPC_SECURITY_ATTR)>;\
    using on_NtAlpcDeletePortSection_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::ALPC_HANDLE)>;\
    using on_NtAlpcDeleteResourceReserve_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::ALPC_HANDLE)>;\
    using on_NtAlpcDeleteSectionView_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::PVOID)>;\
    using on_NtAlpcDeleteSecurityContext_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::ALPC_HANDLE)>;\
    using on_NtAlpcDisconnectPort_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG)>;\
    using on_NtAlpcImpersonateClientOfPort_fn                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPORT_MESSAGE, nt::PVOID)>;\
    using on_NtAlpcOpenSenderProcess_fn                            = std::function<nt::NTSTATUS(nt::PHANDLE, nt::HANDLE, nt::PPORT_MESSAGE, nt::ULONG, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtAlpcOpenSenderThread_fn                             = std::function<nt::NTSTATUS(nt::PHANDLE, nt::HANDLE, nt::PPORT_MESSAGE, nt::ULONG, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtAlpcQueryInformation_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::ALPC_PORT_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtAlpcQueryInformationMessage_fn                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPORT_MESSAGE, nt::ALPC_MESSAGE_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtAlpcRevokeSecurityContext_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::ALPC_HANDLE)>;\
    using on_NtAlpcSendWaitReceivePort_fn                          = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::PPORT_MESSAGE, nt::PALPC_MESSAGE_ATTRIBUTES, nt::PPORT_MESSAGE, nt::PULONG, nt::PALPC_MESSAGE_ATTRIBUTES, nt::PLARGE_INTEGER)>;\
    using on_NtAlpcSetInformation_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::ALPC_PORT_INFORMATION_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtApphelpCacheControl_fn                              = std::function<nt::NTSTATUS(nt::APPHELPCOMMAND, nt::PVOID)>;\
    using on_NtAreMappedFilesTheSame_fn                            = std::function<nt::NTSTATUS(nt::PVOID, nt::PVOID)>;\
    using on_NtAssignProcessToJobObject_fn                         = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE)>;\
    using on_NtCallbackReturn_fn                                   = std::function<nt::NTSTATUS(nt::PVOID, nt::ULONG, nt::NTSTATUS)>;\
    using on_NtCancelIoFileEx_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PIO_STATUS_BLOCK)>;\
    using on_NtCancelIoFile_fn                                     = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK)>;\
    using on_NtCancelSynchronousIoFile_fn                          = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PIO_STATUS_BLOCK)>;\
    using on_NtCancelTimer_fn                                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::PBOOLEAN)>;\
    using on_NtClearEvent_fn                                       = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtClose_fn                                            = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtCloseObjectAuditAlarm_fn                            = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PVOID, nt::BOOLEAN)>;\
    using on_NtCommitComplete_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtCommitEnlistment_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtCommitTransaction_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::BOOLEAN)>;\
    using on_NtCompactKeys_fn                                      = std::function<nt::NTSTATUS(nt::ULONG, nt::HANDLE)>;\
    using on_NtCompareTokens_fn                                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PBOOLEAN)>;\
    using on_NtCompleteConnectPort_fn                              = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtCompressKey_fn                                      = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtConnectPort_fn                                      = std::function<nt::NTSTATUS(nt::PHANDLE, nt::PUNICODE_STRING, nt::PSECURITY_QUALITY_OF_SERVICE, nt::PPORT_VIEW, nt::PREMOTE_PORT_VIEW, nt::PULONG, nt::PVOID, nt::PULONG)>;\
    using on_NtContinue_fn                                         = std::function<nt::NTSTATUS(nt::PCONTEXT, nt::BOOLEAN)>;\
    using on_NtCreateDebugObject_fn                                = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::ULONG)>;\
    using on_NtCreateDirectoryObject_fn                            = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtCreateEnlistment_fn                                 = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::HANDLE, nt::HANDLE, nt::POBJECT_ATTRIBUTES, nt::ULONG, nt::NOTIFICATION_MASK, nt::PVOID)>;\
    using on_NtCreateEvent_fn                                      = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::EVENT_TYPE, nt::BOOLEAN)>;\
    using on_NtCreateEventPair_fn                                  = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtCreateFile_fn                                       = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::PIO_STATUS_BLOCK, nt::PLARGE_INTEGER, nt::ULONG, nt::ULONG, nt::ULONG, nt::ULONG, nt::PVOID, nt::ULONG)>;\
    using on_NtCreateIoCompletion_fn                               = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::ULONG)>;\
    using on_NtCreateJobObject_fn                                  = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtCreateJobSet_fn                                     = std::function<nt::NTSTATUS(nt::ULONG, nt::PJOB_SET_ARRAY, nt::ULONG)>;\
    using on_NtCreateKeyedEvent_fn                                 = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::ULONG)>;\
    using on_NtCreateKey_fn                                        = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::ULONG, nt::PUNICODE_STRING, nt::ULONG, nt::PULONG)>;\
    using on_NtCreateKeyTransacted_fn                              = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::ULONG, nt::PUNICODE_STRING, nt::ULONG, nt::HANDLE, nt::PULONG)>;\
    using on_NtCreateMailslotFile_fn                               = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ULONG, nt::POBJECT_ATTRIBUTES, nt::PIO_STATUS_BLOCK, nt::ULONG, nt::ULONG, nt::ULONG, nt::PLARGE_INTEGER)>;\
    using on_NtCreateMutant_fn                                     = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::BOOLEAN)>;\
    using on_NtCreateNamedPipeFile_fn                              = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ULONG, nt::POBJECT_ATTRIBUTES, nt::PIO_STATUS_BLOCK, nt::ULONG, nt::ULONG, nt::ULONG, nt::ULONG, nt::ULONG, nt::ULONG, nt::ULONG, nt::ULONG, nt::ULONG, nt::PLARGE_INTEGER)>;\
    using on_NtCreatePagingFile_fn                                 = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PLARGE_INTEGER, nt::PLARGE_INTEGER, nt::ULONG)>;\
    using on_NtCreatePort_fn                                       = std::function<nt::NTSTATUS(nt::PHANDLE, nt::POBJECT_ATTRIBUTES, nt::ULONG, nt::ULONG, nt::ULONG)>;\
    using on_NtCreatePrivateNamespace_fn                           = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::PVOID)>;\
    using on_NtCreateProcessEx_fn                                  = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::HANDLE, nt::ULONG, nt::HANDLE, nt::HANDLE, nt::HANDLE, nt::ULONG)>;\
    using on_NtCreateProcess_fn                                    = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::HANDLE, nt::BOOLEAN, nt::HANDLE, nt::HANDLE, nt::HANDLE)>;\
    using on_NtCreateProfileEx_fn                                  = std::function<nt::NTSTATUS(nt::PHANDLE, nt::HANDLE, nt::PVOID, nt::SIZE_T, nt::ULONG, nt::PULONG, nt::ULONG, nt::KPROFILE_SOURCE, nt::ULONG, nt::PGROUP_AFFINITY)>;\
    using on_NtCreateProfile_fn                                    = std::function<nt::NTSTATUS(nt::PHANDLE, nt::HANDLE, nt::PVOID, nt::SIZE_T, nt::ULONG, nt::PULONG, nt::ULONG, nt::KPROFILE_SOURCE, nt::KAFFINITY)>;\
    using on_NtCreateResourceManager_fn                            = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::HANDLE, nt::LPGUID, nt::POBJECT_ATTRIBUTES, nt::ULONG, nt::PUNICODE_STRING)>;\
    using on_NtCreateSection_fn                                    = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::PLARGE_INTEGER, nt::ULONG, nt::ULONG, nt::HANDLE)>;\
    using on_NtCreateSemaphore_fn                                  = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::LONG, nt::LONG)>;\
    using on_NtCreateSymbolicLinkObject_fn                         = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::PUNICODE_STRING)>;\
    using on_NtCreateThreadEx_fn                                   = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::HANDLE, nt::PVOID, nt::PVOID, nt::ULONG, nt::ULONG_PTR, nt::SIZE_T, nt::SIZE_T, nt::PPS_ATTRIBUTE_LIST)>;\
    using on_NtCreateThread_fn                                     = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::HANDLE, nt::PCLIENT_ID, nt::PCONTEXT, nt::PINITIAL_TEB, nt::BOOLEAN)>;\
    using on_NtCreateTimer_fn                                      = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::TIMER_TYPE)>;\
    using on_NtCreateToken_fn                                      = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::TOKEN_TYPE, nt::PLUID, nt::PLARGE_INTEGER, nt::PTOKEN_USER, nt::PTOKEN_GROUPS, nt::PTOKEN_PRIVILEGES, nt::PTOKEN_OWNER, nt::PTOKEN_PRIMARY_GROUP, nt::PTOKEN_DEFAULT_DACL, nt::PTOKEN_SOURCE)>;\
    using on_NtCreateTransactionManager_fn                         = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::PUNICODE_STRING, nt::ULONG, nt::ULONG)>;\
    using on_NtCreateTransaction_fn                                = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::LPGUID, nt::HANDLE, nt::ULONG, nt::ULONG, nt::ULONG, nt::PLARGE_INTEGER, nt::PUNICODE_STRING)>;\
    using on_NtCreateUserProcess_fn                                = std::function<nt::NTSTATUS(nt::PHANDLE, nt::PHANDLE, nt::ACCESS_MASK, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::POBJECT_ATTRIBUTES, nt::ULONG, nt::ULONG, nt::PRTL_USER_PROCESS_PARAMETERS, nt::PPROCESS_CREATE_INFO, nt::PPROCESS_ATTRIBUTE_LIST)>;\
    using on_NtCreateWaitablePort_fn                               = std::function<nt::NTSTATUS(nt::PHANDLE, nt::POBJECT_ATTRIBUTES, nt::ULONG, nt::ULONG, nt::ULONG)>;\
    using on_NtCreateWorkerFactory_fn                              = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::HANDLE, nt::HANDLE, nt::PVOID, nt::PVOID, nt::ULONG, nt::SIZE_T, nt::SIZE_T)>;\
    using on_NtDebugActiveProcess_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE)>;\
    using on_NtDebugContinue_fn                                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::PCLIENT_ID, nt::NTSTATUS)>;\
    using on_NtDelayExecution_fn                                   = std::function<nt::NTSTATUS(nt::BOOLEAN, nt::PLARGE_INTEGER)>;\
    using on_NtDeleteAtom_fn                                       = std::function<nt::NTSTATUS(nt::RTL_ATOM)>;\
    using on_NtDeleteBootEntry_fn                                  = std::function<nt::NTSTATUS(nt::ULONG)>;\
    using on_NtDeleteDriverEntry_fn                                = std::function<nt::NTSTATUS(nt::ULONG)>;\
    using on_NtDeleteFile_fn                                       = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES)>;\
    using on_NtDeleteKey_fn                                        = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtDeleteObjectAuditAlarm_fn                           = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PVOID, nt::BOOLEAN)>;\
    using on_NtDeletePrivateNamespace_fn                           = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtDeleteValueKey_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::PUNICODE_STRING)>;\
    using on_NtDeviceIoControlFile_fn                              = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::ULONG, nt::PVOID, nt::ULONG, nt::PVOID, nt::ULONG)>;\
    using on_NtDisplayString_fn                                    = std::function<nt::NTSTATUS(nt::PUNICODE_STRING)>;\
    using on_NtDrawText_fn                                         = std::function<nt::NTSTATUS(nt::PUNICODE_STRING)>;\
    using on_NtDuplicateObject_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::HANDLE, nt::PHANDLE, nt::ACCESS_MASK, nt::ULONG, nt::ULONG)>;\
    using on_NtDuplicateToken_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::BOOLEAN, nt::TOKEN_TYPE, nt::PHANDLE)>;\
    using on_NtEnumerateBootEntries_fn                             = std::function<nt::NTSTATUS(nt::PVOID, nt::PULONG)>;\
    using on_NtEnumerateDriverEntries_fn                           = std::function<nt::NTSTATUS(nt::PVOID, nt::PULONG)>;\
    using on_NtEnumerateKey_fn                                     = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::KEY_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtEnumerateSystemEnvironmentValuesEx_fn               = std::function<nt::NTSTATUS(nt::ULONG, nt::PVOID, nt::PULONG)>;\
    using on_NtEnumerateTransactionObject_fn                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::KTMOBJECT_TYPE, nt::PKTMOBJECT_CURSOR, nt::ULONG, nt::PULONG)>;\
    using on_NtEnumerateValueKey_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::KEY_VALUE_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtExtendSection_fn                                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtFilterToken_fn                                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::PTOKEN_GROUPS, nt::PTOKEN_PRIVILEGES, nt::PTOKEN_GROUPS, nt::PHANDLE)>;\
    using on_NtFindAtom_fn                                         = std::function<nt::NTSTATUS(nt::PWSTR, nt::ULONG, nt::PRTL_ATOM)>;\
    using on_NtFlushBuffersFile_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK)>;\
    using on_NtFlushInstallUILanguage_fn                           = std::function<nt::NTSTATUS(nt::LANGID, nt::ULONG)>;\
    using on_NtFlushInstructionCache_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::SIZE_T)>;\
    using on_NtFlushKey_fn                                         = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtFlushVirtualMemory_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PSIZE_T, nt::PIO_STATUS_BLOCK)>;\
    using on_NtFreeUserPhysicalPages_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::PULONG_PTR, nt::PULONG_PTR)>;\
    using on_NtFreeVirtualMemory_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PSIZE_T, nt::ULONG)>;\
    using on_NtFreezeRegistry_fn                                   = std::function<nt::NTSTATUS(nt::ULONG)>;\
    using on_NtFreezeTransactions_fn                               = std::function<nt::NTSTATUS(nt::PLARGE_INTEGER, nt::PLARGE_INTEGER)>;\
    using on_NtFsControlFile_fn                                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::ULONG, nt::PVOID, nt::ULONG, nt::PVOID, nt::ULONG)>;\
    using on_NtGetContextThread_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::PCONTEXT)>;\
    using on_NtGetDevicePowerState_fn                              = std::function<nt::NTSTATUS(nt::HANDLE, nt::DEVICE_POWER_STATE)>;\
    using on_NtGetMUIRegistryInfo_fn                               = std::function<nt::NTSTATUS(nt::ULONG, nt::PULONG, nt::PVOID)>;\
    using on_NtGetNextProcess_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::ACCESS_MASK, nt::ULONG, nt::ULONG, nt::PHANDLE)>;\
    using on_NtGetNextThread_fn                                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::ACCESS_MASK, nt::ULONG, nt::ULONG, nt::PHANDLE)>;\
    using on_NtGetNlsSectionPtr_fn                                 = std::function<nt::NTSTATUS(nt::ULONG, nt::ULONG, nt::PVOID, nt::PVOID, nt::PULONG)>;\
    using on_NtGetNotificationResourceManager_fn                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::PTRANSACTION_NOTIFICATION, nt::ULONG, nt::PLARGE_INTEGER, nt::PULONG, nt::ULONG, nt::ULONG_PTR)>;\
    using on_NtGetPlugPlayEvent_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PPLUGPLAY_EVENT_BLOCK, nt::ULONG)>;\
    using on_NtGetWriteWatch_fn                                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::PVOID, nt::SIZE_T, nt::PVOID, nt::PULONG_PTR, nt::PULONG)>;\
    using on_NtImpersonateAnonymousToken_fn                        = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtImpersonateClientOfPort_fn                          = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPORT_MESSAGE)>;\
    using on_NtImpersonateThread_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PSECURITY_QUALITY_OF_SERVICE)>;\
    using on_NtInitializeNlsFiles_fn                               = std::function<nt::NTSTATUS(nt::PVOID, nt::PLCID, nt::PLARGE_INTEGER)>;\
    using on_NtInitializeRegistry_fn                               = std::function<nt::NTSTATUS(nt::USHORT)>;\
    using on_NtInitiatePowerAction_fn                              = std::function<nt::NTSTATUS(nt::POWER_ACTION, nt::SYSTEM_POWER_STATE, nt::ULONG, nt::BOOLEAN)>;\
    using on_NtIsProcessInJob_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE)>;\
    using on_NtListenPort_fn                                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPORT_MESSAGE)>;\
    using on_NtLoadDriver_fn                                       = std::function<nt::NTSTATUS(nt::PUNICODE_STRING)>;\
    using on_NtLoadKey2_fn                                         = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES, nt::POBJECT_ATTRIBUTES, nt::ULONG)>;\
    using on_NtLoadKeyEx_fn                                        = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES, nt::POBJECT_ATTRIBUTES, nt::ULONG, nt::HANDLE)>;\
    using on_NtLoadKey_fn                                          = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtLockFile_fn                                         = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::PLARGE_INTEGER, nt::PLARGE_INTEGER, nt::ULONG, nt::BOOLEAN, nt::BOOLEAN)>;\
    using on_NtLockProductActivationKeys_fn                        = std::function<nt::NTSTATUS(nt::ULONG, nt::ULONG)>;\
    using on_NtLockRegistryKey_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtLockVirtualMemory_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PSIZE_T, nt::ULONG)>;\
    using on_NtMakePermanentObject_fn                              = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtMakeTemporaryObject_fn                              = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtMapCMFModule_fn                                     = std::function<nt::NTSTATUS(nt::ULONG, nt::ULONG, nt::PULONG, nt::PULONG, nt::PULONG, nt::PVOID)>;\
    using on_NtMapUserPhysicalPages_fn                             = std::function<nt::NTSTATUS(nt::PVOID, nt::ULONG_PTR, nt::PULONG_PTR)>;\
    using on_NtMapUserPhysicalPagesScatter_fn                      = std::function<nt::NTSTATUS(nt::PVOID, nt::ULONG_PTR, nt::PULONG_PTR)>;\
    using on_NtMapViewOfSection_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PVOID, nt::ULONG_PTR, nt::SIZE_T, nt::PLARGE_INTEGER, nt::PSIZE_T, nt::SECTION_INHERIT, nt::ULONG, nt::WIN32_PROTECTION_MASK)>;\
    using on_NtModifyBootEntry_fn                                  = std::function<nt::NTSTATUS(nt::PBOOT_ENTRY)>;\
    using on_NtModifyDriverEntry_fn                                = std::function<nt::NTSTATUS(nt::PEFI_DRIVER_ENTRY)>;\
    using on_NtNotifyChangeDirectoryFile_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::ULONG, nt::BOOLEAN)>;\
    using on_NtNotifyChangeKey_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::ULONG, nt::BOOLEAN, nt::PVOID, nt::ULONG, nt::BOOLEAN)>;\
    using on_NtNotifyChangeMultipleKeys_fn                         = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::OBJECT_ATTRIBUTES, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::ULONG, nt::BOOLEAN, nt::PVOID, nt::ULONG, nt::BOOLEAN)>;\
    using on_NtNotifyChangeSession_fn                              = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::PVOID, nt::ULONG, nt::IO_SESSION_STATE, nt::IO_SESSION_STATE, nt::PVOID, nt::ULONG)>;\
    using on_NtOpenDirectoryObject_fn                              = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenEnlistment_fn                                   = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::HANDLE, nt::LPGUID, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenEvent_fn                                        = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenEventPair_fn                                    = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenFile_fn                                         = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::PIO_STATUS_BLOCK, nt::ULONG, nt::ULONG)>;\
    using on_NtOpenIoCompletion_fn                                 = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenJobObject_fn                                    = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenKeyedEvent_fn                                   = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenKeyEx_fn                                        = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::ULONG)>;\
    using on_NtOpenKey_fn                                          = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenKeyTransactedEx_fn                              = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::ULONG, nt::HANDLE)>;\
    using on_NtOpenKeyTransacted_fn                                = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::HANDLE)>;\
    using on_NtOpenMutant_fn                                       = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenObjectAuditAlarm_fn                             = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PVOID, nt::PUNICODE_STRING, nt::PUNICODE_STRING, nt::PSECURITY_DESCRIPTOR, nt::HANDLE, nt::ACCESS_MASK, nt::ACCESS_MASK, nt::PPRIVILEGE_SET, nt::BOOLEAN, nt::BOOLEAN, nt::PBOOLEAN)>;\
    using on_NtOpenPrivateNamespace_fn                             = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::PVOID)>;\
    using on_NtOpenProcess_fn                                      = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::PCLIENT_ID)>;\
    using on_NtOpenProcessTokenEx_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::ACCESS_MASK, nt::ULONG, nt::PHANDLE)>;\
    using on_NtOpenProcessToken_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::ACCESS_MASK, nt::PHANDLE)>;\
    using on_NtOpenResourceManager_fn                              = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::HANDLE, nt::LPGUID, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenSection_fn                                      = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenSemaphore_fn                                    = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenSession_fn                                      = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenSymbolicLinkObject_fn                           = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenThread_fn                                       = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::PCLIENT_ID)>;\
    using on_NtOpenThreadTokenEx_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::ACCESS_MASK, nt::BOOLEAN, nt::ULONG, nt::PHANDLE)>;\
    using on_NtOpenThreadToken_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::ACCESS_MASK, nt::BOOLEAN, nt::PHANDLE)>;\
    using on_NtOpenTimer_fn                                        = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtOpenTransactionManager_fn                           = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::PUNICODE_STRING, nt::LPGUID, nt::ULONG)>;\
    using on_NtOpenTransaction_fn                                  = std::function<nt::NTSTATUS(nt::PHANDLE, nt::ACCESS_MASK, nt::POBJECT_ATTRIBUTES, nt::LPGUID, nt::HANDLE)>;\
    using on_NtPlugPlayControl_fn                                  = std::function<nt::NTSTATUS(nt::PLUGPLAY_CONTROL_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtPowerInformation_fn                                 = std::function<nt::NTSTATUS(nt::POWER_INFORMATION_LEVEL, nt::PVOID, nt::ULONG, nt::PVOID, nt::ULONG)>;\
    using on_NtPrepareComplete_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtPrepareEnlistment_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtPrePrepareComplete_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtPrePrepareEnlistment_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtPrivilegeCheck_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPRIVILEGE_SET, nt::PBOOLEAN)>;\
    using on_NtPrivilegedServiceAuditAlarm_fn                      = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PUNICODE_STRING, nt::HANDLE, nt::PPRIVILEGE_SET, nt::BOOLEAN)>;\
    using on_NtPrivilegeObjectAuditAlarm_fn                        = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PVOID, nt::HANDLE, nt::ACCESS_MASK, nt::PPRIVILEGE_SET, nt::BOOLEAN)>;\
    using on_NtPropagationComplete_fn                              = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::ULONG, nt::PVOID)>;\
    using on_NtPropagationFailed_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::NTSTATUS)>;\
    using on_NtProtectVirtualMemory_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PSIZE_T, nt::WIN32_PROTECTION_MASK, nt::PULONG)>;\
    using on_NtPulseEvent_fn                                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLONG)>;\
    using on_NtQueryAttributesFile_fn                              = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES, nt::PFILE_BASIC_INFORMATION)>;\
    using on_NtQueryBootEntryOrder_fn                              = std::function<nt::NTSTATUS(nt::PULONG, nt::PULONG)>;\
    using on_NtQueryBootOptions_fn                                 = std::function<nt::NTSTATUS(nt::PBOOT_OPTIONS, nt::PULONG)>;\
    using on_NtQueryDebugFilterState_fn                            = std::function<nt::NTSTATUS(nt::ULONG, nt::ULONG)>;\
    using on_NtQueryDefaultLocale_fn                               = std::function<nt::NTSTATUS(nt::BOOLEAN, nt::PLCID)>;\
    using on_NtQueryDefaultUILanguage_fn                           = std::function<nt::NTSTATUS(nt::LANGID)>;\
    using on_NtQueryDirectoryFile_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::FILE_INFORMATION_CLASS, nt::BOOLEAN, nt::PUNICODE_STRING, nt::BOOLEAN)>;\
    using on_NtQueryDirectoryObject_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::ULONG, nt::BOOLEAN, nt::BOOLEAN, nt::PULONG, nt::PULONG)>;\
    using on_NtQueryDriverEntryOrder_fn                            = std::function<nt::NTSTATUS(nt::PULONG, nt::PULONG)>;\
    using on_NtQueryEaFile_fn                                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::BOOLEAN, nt::PVOID, nt::ULONG, nt::PULONG, nt::BOOLEAN)>;\
    using on_NtQueryEvent_fn                                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::EVENT_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryFullAttributesFile_fn                          = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES, nt::PFILE_NETWORK_OPEN_INFORMATION)>;\
    using on_NtQueryInformationAtom_fn                             = std::function<nt::NTSTATUS(nt::RTL_ATOM, nt::ATOM_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInformationEnlistment_fn                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::ENLISTMENT_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInformationFile_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::FILE_INFORMATION_CLASS)>;\
    using on_NtQueryInformationJobObject_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::JOBOBJECTINFOCLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInformationPort_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::PORT_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInformationProcess_fn                          = std::function<nt::NTSTATUS(nt::HANDLE, nt::PROCESSINFOCLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInformationResourceManager_fn                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::RESOURCEMANAGER_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInformationThread_fn                           = std::function<nt::NTSTATUS(nt::HANDLE, nt::THREADINFOCLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInformationToken_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::TOKEN_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInformationTransaction_fn                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::TRANSACTION_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInformationTransactionManager_fn               = std::function<nt::NTSTATUS(nt::HANDLE, nt::TRANSACTIONMANAGER_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInformationWorkerFactory_fn                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::WORKERFACTORYINFOCLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryInstallUILanguage_fn                           = std::function<nt::NTSTATUS(nt::LANGID)>;\
    using on_NtQueryIntervalProfile_fn                             = std::function<nt::NTSTATUS(nt::KPROFILE_SOURCE, nt::PULONG)>;\
    using on_NtQueryIoCompletion_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::IO_COMPLETION_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryKey_fn                                         = std::function<nt::NTSTATUS(nt::HANDLE, nt::KEY_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryLicenseValue_fn                                = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PULONG, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryMultipleValueKey_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::PKEY_VALUE_ENTRY, nt::ULONG, nt::PVOID, nt::PULONG, nt::PULONG)>;\
    using on_NtQueryMutant_fn                                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::MUTANT_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryObject_fn                                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::OBJECT_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryOpenSubKeysEx_fn                               = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES, nt::ULONG, nt::PVOID, nt::PULONG)>;\
    using on_NtQueryOpenSubKeys_fn                                 = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES, nt::PULONG)>;\
    using on_NtQueryPerformanceCounter_fn                          = std::function<nt::NTSTATUS(nt::PLARGE_INTEGER, nt::PLARGE_INTEGER)>;\
    using on_NtQueryQuotaInformationFile_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::BOOLEAN, nt::PVOID, nt::ULONG, nt::PULONG, nt::BOOLEAN)>;\
    using on_NtQuerySection_fn                                     = std::function<nt::NTSTATUS(nt::HANDLE, nt::SECTION_INFORMATION_CLASS, nt::PVOID, nt::SIZE_T, nt::PSIZE_T)>;\
    using on_NtQuerySecurityAttributesToken_fn                     = std::function<nt::NTSTATUS(nt::HANDLE, nt::PUNICODE_STRING, nt::ULONG, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQuerySecurityObject_fn                              = std::function<nt::NTSTATUS(nt::HANDLE, nt::SECURITY_INFORMATION, nt::PSECURITY_DESCRIPTOR, nt::ULONG, nt::PULONG)>;\
    using on_NtQuerySemaphore_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::SEMAPHORE_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQuerySymbolicLinkObject_fn                          = std::function<nt::NTSTATUS(nt::HANDLE, nt::PUNICODE_STRING, nt::PULONG)>;\
    using on_NtQuerySystemEnvironmentValueEx_fn                    = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::LPGUID, nt::PVOID, nt::PULONG, nt::PULONG)>;\
    using on_NtQuerySystemEnvironmentValue_fn                      = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PWSTR, nt::USHORT, nt::PUSHORT)>;\
    using on_NtQuerySystemInformationEx_fn                         = std::function<nt::NTSTATUS(nt::SYSTEM_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQuerySystemInformation_fn                           = std::function<nt::NTSTATUS(nt::SYSTEM_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQuerySystemTime_fn                                  = std::function<nt::NTSTATUS(nt::PLARGE_INTEGER)>;\
    using on_NtQueryTimer_fn                                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::TIMER_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryTimerResolution_fn                             = std::function<nt::NTSTATUS(nt::PULONG, nt::PULONG, nt::PULONG)>;\
    using on_NtQueryValueKey_fn                                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::PUNICODE_STRING, nt::KEY_VALUE_INFORMATION_CLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtQueryVirtualMemory_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::MEMORY_INFORMATION_CLASS, nt::PVOID, nt::SIZE_T, nt::PSIZE_T)>;\
    using on_NtQueryVolumeInformationFile_fn                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::FS_INFORMATION_CLASS)>;\
    using on_NtQueueApcThreadEx_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PPS_APC_ROUTINE, nt::PVOID, nt::PVOID, nt::PVOID)>;\
    using on_NtQueueApcThread_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPS_APC_ROUTINE, nt::PVOID, nt::PVOID, nt::PVOID)>;\
    using on_NtRaiseException_fn                                   = std::function<nt::NTSTATUS(nt::PEXCEPTION_RECORD, nt::PCONTEXT, nt::BOOLEAN)>;\
    using on_NtRaiseHardError_fn                                   = std::function<nt::NTSTATUS(nt::NTSTATUS, nt::ULONG, nt::ULONG, nt::PULONG_PTR, nt::ULONG, nt::PULONG)>;\
    using on_NtReadFile_fn                                         = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::PLARGE_INTEGER, nt::PULONG)>;\
    using on_NtReadFileScatter_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::PFILE_SEGMENT_ELEMENT, nt::ULONG, nt::PLARGE_INTEGER, nt::PULONG)>;\
    using on_NtReadOnlyEnlistment_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtReadRequestData_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPORT_MESSAGE, nt::ULONG, nt::PVOID, nt::SIZE_T, nt::PSIZE_T)>;\
    using on_NtReadVirtualMemory_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PVOID, nt::SIZE_T, nt::PSIZE_T)>;\
    using on_NtRecoverEnlistment_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID)>;\
    using on_NtRecoverResourceManager_fn                           = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtRecoverTransactionManager_fn                        = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtRegisterProtocolAddressInformation_fn               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PCRM_PROTOCOL_ID, nt::ULONG, nt::PVOID, nt::ULONG)>;\
    using on_NtRegisterThreadTerminatePort_fn                      = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtReleaseKeyedEvent_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::BOOLEAN, nt::PLARGE_INTEGER)>;\
    using on_NtReleaseMutant_fn                                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLONG)>;\
    using on_NtReleaseSemaphore_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::LONG, nt::PLONG)>;\
    using on_NtReleaseWorkerFactoryWorker_fn                       = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtRemoveIoCompletionEx_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::PFILE_IO_COMPLETION_INFORMATION, nt::ULONG, nt::PULONG, nt::PLARGE_INTEGER, nt::BOOLEAN)>;\
    using on_NtRemoveIoCompletion_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::PLARGE_INTEGER)>;\
    using on_NtRemoveProcessDebug_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE)>;\
    using on_NtRenameKey_fn                                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::PUNICODE_STRING)>;\
    using on_NtRenameTransactionManager_fn                         = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::LPGUID)>;\
    using on_NtReplaceKey_fn                                       = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES, nt::HANDLE, nt::POBJECT_ATTRIBUTES)>;\
    using on_NtReplacePartitionUnit_fn                             = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PUNICODE_STRING, nt::ULONG)>;\
    using on_NtReplyPort_fn                                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPORT_MESSAGE)>;\
    using on_NtReplyWaitReceivePortEx_fn                           = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PPORT_MESSAGE, nt::PPORT_MESSAGE, nt::PLARGE_INTEGER)>;\
    using on_NtReplyWaitReceivePort_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PPORT_MESSAGE, nt::PPORT_MESSAGE)>;\
    using on_NtReplyWaitReplyPort_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPORT_MESSAGE)>;\
    using on_NtRequestPort_fn                                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPORT_MESSAGE)>;\
    using on_NtRequestWaitReplyPort_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPORT_MESSAGE, nt::PPORT_MESSAGE)>;\
    using on_NtResetEvent_fn                                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLONG)>;\
    using on_NtResetWriteWatch_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::SIZE_T)>;\
    using on_NtRestoreKey_fn                                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::ULONG)>;\
    using on_NtResumeProcess_fn                                    = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtResumeThread_fn                                     = std::function<nt::NTSTATUS(nt::HANDLE, nt::PULONG)>;\
    using on_NtRollbackComplete_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtRollbackEnlistment_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtRollbackTransaction_fn                              = std::function<nt::NTSTATUS(nt::HANDLE, nt::BOOLEAN)>;\
    using on_NtRollforwardTransactionManager_fn                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtSaveKeyEx_fn                                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::ULONG)>;\
    using on_NtSaveKey_fn                                          = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE)>;\
    using on_NtSaveMergedKeys_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::HANDLE)>;\
    using on_NtSecureConnectPort_fn                                = std::function<nt::NTSTATUS(nt::PHANDLE, nt::PUNICODE_STRING, nt::PSECURITY_QUALITY_OF_SERVICE, nt::PPORT_VIEW, nt::PSID, nt::PREMOTE_PORT_VIEW, nt::PULONG, nt::PVOID, nt::PULONG)>;\
    using on_NtSetBootEntryOrder_fn                                = std::function<nt::NTSTATUS(nt::PULONG, nt::ULONG)>;\
    using on_NtSetBootOptions_fn                                   = std::function<nt::NTSTATUS(nt::PBOOT_OPTIONS, nt::ULONG)>;\
    using on_NtSetContextThread_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::PCONTEXT)>;\
    using on_NtSetDebugFilterState_fn                              = std::function<nt::NTSTATUS(nt::ULONG, nt::ULONG, nt::BOOLEAN)>;\
    using on_NtSetDefaultHardErrorPort_fn                          = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtSetDefaultLocale_fn                                 = std::function<nt::NTSTATUS(nt::BOOLEAN, nt::LCID)>;\
    using on_NtSetDefaultUILanguage_fn                             = std::function<nt::NTSTATUS(nt::LANGID)>;\
    using on_NtSetDriverEntryOrder_fn                              = std::function<nt::NTSTATUS(nt::PULONG, nt::ULONG)>;\
    using on_NtSetEaFile_fn                                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG)>;\
    using on_NtSetEventBoostPriority_fn                            = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtSetEvent_fn                                         = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLONG)>;\
    using on_NtSetHighEventPair_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtSetHighWaitLowEventPair_fn                          = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtSetInformationDebugObject_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::DEBUGOBJECTINFOCLASS, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtSetInformationEnlistment_fn                         = std::function<nt::NTSTATUS(nt::HANDLE, nt::ENLISTMENT_INFORMATION_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetInformationFile_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::FILE_INFORMATION_CLASS)>;\
    using on_NtSetInformationJobObject_fn                          = std::function<nt::NTSTATUS(nt::HANDLE, nt::JOBOBJECTINFOCLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetInformationKey_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::KEY_SET_INFORMATION_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetInformationObject_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::OBJECT_INFORMATION_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetInformationProcess_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::PROCESSINFOCLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetInformationResourceManager_fn                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::RESOURCEMANAGER_INFORMATION_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetInformationThread_fn                             = std::function<nt::NTSTATUS(nt::HANDLE, nt::THREADINFOCLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetInformationToken_fn                              = std::function<nt::NTSTATUS(nt::HANDLE, nt::TOKEN_INFORMATION_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetInformationTransaction_fn                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::TRANSACTION_INFORMATION_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetInformationTransactionManager_fn                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::TRANSACTIONMANAGER_INFORMATION_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetInformationWorkerFactory_fn                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::WORKERFACTORYINFOCLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetIntervalProfile_fn                               = std::function<nt::NTSTATUS(nt::ULONG, nt::KPROFILE_SOURCE)>;\
    using on_NtSetIoCompletionEx_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PVOID, nt::PVOID, nt::NTSTATUS, nt::ULONG_PTR)>;\
    using on_NtSetIoCompletion_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PVOID, nt::NTSTATUS, nt::ULONG_PTR)>;\
    using on_NtSetLdtEntries_fn                                    = std::function<nt::NTSTATUS(nt::ULONG, nt::ULONG, nt::ULONG, nt::ULONG, nt::ULONG, nt::ULONG)>;\
    using on_NtSetLowEventPair_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtSetLowWaitHighEventPair_fn                          = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtSetQuotaInformationFile_fn                          = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG)>;\
    using on_NtSetSecurityObject_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::SECURITY_INFORMATION, nt::PSECURITY_DESCRIPTOR)>;\
    using on_NtSetSystemEnvironmentValueEx_fn                      = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::LPGUID, nt::PVOID, nt::ULONG, nt::ULONG)>;\
    using on_NtSetSystemEnvironmentValue_fn                        = std::function<nt::NTSTATUS(nt::PUNICODE_STRING, nt::PUNICODE_STRING)>;\
    using on_NtSetSystemInformation_fn                             = std::function<nt::NTSTATUS(nt::SYSTEM_INFORMATION_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetSystemPowerState_fn                              = std::function<nt::NTSTATUS(nt::POWER_ACTION, nt::SYSTEM_POWER_STATE, nt::ULONG)>;\
    using on_NtSetSystemTime_fn                                    = std::function<nt::NTSTATUS(nt::PLARGE_INTEGER, nt::PLARGE_INTEGER)>;\
    using on_NtSetThreadExecutionState_fn                          = std::function<nt::NTSTATUS(nt::EXECUTION_STATE, nt::EXECUTION_STATE)>;\
    using on_NtSetTimerEx_fn                                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::TIMER_SET_INFORMATION_CLASS, nt::PVOID, nt::ULONG)>;\
    using on_NtSetTimer_fn                                         = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER, nt::PTIMER_APC_ROUTINE, nt::PVOID, nt::BOOLEAN, nt::LONG, nt::PBOOLEAN)>;\
    using on_NtSetTimerResolution_fn                               = std::function<nt::NTSTATUS(nt::ULONG, nt::BOOLEAN, nt::PULONG)>;\
    using on_NtSetUuidSeed_fn                                      = std::function<nt::NTSTATUS(nt::PCHAR)>;\
    using on_NtSetValueKey_fn                                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::PUNICODE_STRING, nt::ULONG, nt::ULONG, nt::PVOID, nt::ULONG)>;\
    using on_NtSetVolumeInformationFile_fn                         = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::FS_INFORMATION_CLASS)>;\
    using on_NtShutdownSystem_fn                                   = std::function<nt::NTSTATUS(nt::SHUTDOWN_ACTION)>;\
    using on_NtShutdownWorkerFactory_fn                            = std::function<nt::NTSTATUS(nt::HANDLE, nt::LONG)>;\
    using on_NtSignalAndWaitForSingleObject_fn                     = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::BOOLEAN, nt::PLARGE_INTEGER)>;\
    using on_NtSinglePhaseReject_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::PLARGE_INTEGER)>;\
    using on_NtStartProfile_fn                                     = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtStopProfile_fn                                      = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtSuspendProcess_fn                                   = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtSuspendThread_fn                                    = std::function<nt::NTSTATUS(nt::HANDLE, nt::PULONG)>;\
    using on_NtSystemDebugControl_fn                               = std::function<nt::NTSTATUS(nt::SYSDBG_COMMAND, nt::PVOID, nt::ULONG, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtTerminateJobObject_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::NTSTATUS)>;\
    using on_NtTerminateProcess_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::NTSTATUS)>;\
    using on_NtTerminateThread_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::NTSTATUS)>;\
    using on_NtTraceControl_fn                                     = std::function<nt::NTSTATUS(nt::ULONG, nt::PVOID, nt::ULONG, nt::PVOID, nt::ULONG, nt::PULONG)>;\
    using on_NtTraceEvent_fn                                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::ULONG, nt::ULONG, nt::PVOID)>;\
    using on_NtTranslateFilePath_fn                                = std::function<nt::NTSTATUS(nt::PFILE_PATH, nt::ULONG, nt::PFILE_PATH, nt::PULONG)>;\
    using on_NtUnloadDriver_fn                                     = std::function<nt::NTSTATUS(nt::PUNICODE_STRING)>;\
    using on_NtUnloadKey2_fn                                       = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES, nt::ULONG)>;\
    using on_NtUnloadKeyEx_fn                                      = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES, nt::HANDLE)>;\
    using on_NtUnloadKey_fn                                        = std::function<nt::NTSTATUS(nt::POBJECT_ATTRIBUTES)>;\
    using on_NtUnlockFile_fn                                       = std::function<nt::NTSTATUS(nt::HANDLE, nt::PIO_STATUS_BLOCK, nt::PLARGE_INTEGER, nt::PLARGE_INTEGER, nt::ULONG)>;\
    using on_NtUnlockVirtualMemory_fn                              = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PSIZE_T, nt::ULONG)>;\
    using on_NtUnmapViewOfSection_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID)>;\
    using on_NtVdmControl_fn                                       = std::function<nt::NTSTATUS(nt::VDMSERVICECLASS, nt::PVOID)>;\
    using on_NtWaitForDebugEvent_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::BOOLEAN, nt::PLARGE_INTEGER, nt::PDBGUI_WAIT_STATE_CHANGE)>;\
    using on_NtWaitForKeyedEvent_fn                                = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::BOOLEAN, nt::PLARGE_INTEGER)>;\
    using on_NtWaitForMultipleObjects32_fn                         = std::function<nt::NTSTATUS(nt::ULONG, nt::LONG, nt::WAIT_TYPE, nt::BOOLEAN, nt::PLARGE_INTEGER)>;\
    using on_NtWaitForMultipleObjects_fn                           = std::function<nt::NTSTATUS(nt::ULONG, nt::HANDLE, nt::WAIT_TYPE, nt::BOOLEAN, nt::PLARGE_INTEGER)>;\
    using on_NtWaitForSingleObject_fn                              = std::function<nt::NTSTATUS(nt::HANDLE, nt::BOOLEAN, nt::PLARGE_INTEGER)>;\
    using on_NtWaitForWorkViaWorkerFactory_fn                      = std::function<nt::NTSTATUS(nt::HANDLE, nt::PFILE_IO_COMPLETION_INFORMATION)>;\
    using on_NtWaitHighEventPair_fn                                = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtWaitLowEventPair_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtWorkerFactoryWorkerReady_fn                         = std::function<nt::NTSTATUS(nt::HANDLE)>;\
    using on_NtWriteFileGather_fn                                  = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::PFILE_SEGMENT_ELEMENT, nt::ULONG, nt::PLARGE_INTEGER, nt::PULONG)>;\
    using on_NtWriteFile_fn                                        = std::function<nt::NTSTATUS(nt::HANDLE, nt::HANDLE, nt::PIO_APC_ROUTINE, nt::PVOID, nt::PIO_STATUS_BLOCK, nt::PVOID, nt::ULONG, nt::PLARGE_INTEGER, nt::PULONG)>;\
    using on_NtWriteRequestData_fn                                 = std::function<nt::NTSTATUS(nt::HANDLE, nt::PPORT_MESSAGE, nt::ULONG, nt::PVOID, nt::SIZE_T, nt::PSIZE_T)>;\
    using on_NtWriteVirtualMemory_fn                               = std::function<nt::NTSTATUS(nt::HANDLE, nt::PVOID, nt::PVOID, nt::SIZE_T, nt::PSIZE_T)>;\
    using on_NtDisableLastKnownGood_fn                             = std::function<nt::NTSTATUS()>;\
    using on_NtEnableLastKnownGood_fn                              = std::function<nt::NTSTATUS()>;\
    using on_NtFlushProcessWriteBuffers_fn                         = std::function<nt::VOID()>;\
    using on_NtFlushWriteBuffer_fn                                 = std::function<nt::NTSTATUS()>;\
    using on_NtGetCurrentProcessorNumber_fn                        = std::function<nt::ULONG()>;\
    using on_NtIsSystemResumeAutomatic_fn                          = std::function<nt::BOOLEAN()>;\
    using on_NtIsUILanguageComitted_fn                             = std::function<nt::NTSTATUS()>;\
    using on_NtQueryPortInformationProcess_fn                      = std::function<nt::NTSTATUS()>;\
    using on_NtSerializeBoot_fn                                    = std::function<nt::NTSTATUS()>;\
    using on_NtTestAlert_fn                                        = std::function<nt::NTSTATUS()>;\
    using on_NtThawRegistry_fn                                     = std::function<nt::NTSTATUS()>;\
    using on_NtThawTransactions_fn                                 = std::function<nt::NTSTATUS()>;\
    using on_NtUmsThreadYield_fn                                   = std::function<nt::NTSTATUS()>;\
    using on_NtYieldExecution_fn                                   = std::function<nt::NTSTATUS()>;\
    using on_RtlpAllocateHeapInternal_fn                           = std::function<nt::PVOID(nt::PVOID, nt::SIZE_T)>;\
    using on_RtlFreeHeap_fn                                        = std::function<nt::BOOLEAN(nt::PVOID, nt::ULONG, nt::PVOID)>;\
    using on_RtlpReAllocateHeapInternal_fn                         = std::function<nt::PVOID(nt::PVOID, nt::ULONG, nt::PVOID, nt::ULONG)>;\
    using on_RtlSizeHeap_fn                                        = std::function<nt::SIZE_T(nt::PVOID, nt::ULONG, nt::PVOID)>;\
    using on_RtlSetUserValueHeap_fn                                = std::function<nt::BOOLEAN(nt::PVOID, nt::ULONG, nt::PVOID, nt::PVOID)>;\
    using on_RtlGetUserInfoHeap_fn                                 = std::function<nt::BOOLEAN(nt::PVOID, nt::ULONG, nt::PVOID, nt::PVOID, nt::PULONG)>;

#define DECLARE_SYSCALLS_FUNCTIONS_PROTOS\
    void on_NtAcceptConnectPort                               ();\
    void on_NtAccessCheckAndAuditAlarm                        ();\
    void on_NtAccessCheckByTypeAndAuditAlarm                  ();\
    void on_NtAccessCheckByType                               ();\
    void on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle();\
    void on_NtAccessCheckByTypeResultListAndAuditAlarm        ();\
    void on_NtAccessCheckByTypeResultList                     ();\
    void on_NtAccessCheck                                     ();\
    void on_NtAddAtom                                         ();\
    void on_NtAddBootEntry                                    ();\
    void on_NtAddDriverEntry                                  ();\
    void on_NtAdjustGroupsToken                               ();\
    void on_NtAdjustPrivilegesToken                           ();\
    void on_NtAlertResumeThread                               ();\
    void on_NtAlertThread                                     ();\
    void on_NtAllocateLocallyUniqueId                         ();\
    void on_NtAllocateReserveObject                           ();\
    void on_NtAllocateUserPhysicalPages                       ();\
    void on_NtAllocateUuids                                   ();\
    void on_NtAllocateVirtualMemory                           ();\
    void on_NtAlpcAcceptConnectPort                           ();\
    void on_NtAlpcCancelMessage                               ();\
    void on_NtAlpcConnectPort                                 ();\
    void on_NtAlpcCreatePort                                  ();\
    void on_NtAlpcCreatePortSection                           ();\
    void on_NtAlpcCreateResourceReserve                       ();\
    void on_NtAlpcCreateSectionView                           ();\
    void on_NtAlpcCreateSecurityContext                       ();\
    void on_NtAlpcDeletePortSection                           ();\
    void on_NtAlpcDeleteResourceReserve                       ();\
    void on_NtAlpcDeleteSectionView                           ();\
    void on_NtAlpcDeleteSecurityContext                       ();\
    void on_NtAlpcDisconnectPort                              ();\
    void on_NtAlpcImpersonateClientOfPort                     ();\
    void on_NtAlpcOpenSenderProcess                           ();\
    void on_NtAlpcOpenSenderThread                            ();\
    void on_NtAlpcQueryInformation                            ();\
    void on_NtAlpcQueryInformationMessage                     ();\
    void on_NtAlpcRevokeSecurityContext                       ();\
    void on_NtAlpcSendWaitReceivePort                         ();\
    void on_NtAlpcSetInformation                              ();\
    void on_NtApphelpCacheControl                             ();\
    void on_NtAreMappedFilesTheSame                           ();\
    void on_NtAssignProcessToJobObject                        ();\
    void on_NtCallbackReturn                                  ();\
    void on_NtCancelIoFileEx                                  ();\
    void on_NtCancelIoFile                                    ();\
    void on_NtCancelSynchronousIoFile                         ();\
    void on_NtCancelTimer                                     ();\
    void on_NtClearEvent                                      ();\
    void on_NtClose                                           ();\
    void on_NtCloseObjectAuditAlarm                           ();\
    void on_NtCommitComplete                                  ();\
    void on_NtCommitEnlistment                                ();\
    void on_NtCommitTransaction                               ();\
    void on_NtCompactKeys                                     ();\
    void on_NtCompareTokens                                   ();\
    void on_NtCompleteConnectPort                             ();\
    void on_NtCompressKey                                     ();\
    void on_NtConnectPort                                     ();\
    void on_NtContinue                                        ();\
    void on_NtCreateDebugObject                               ();\
    void on_NtCreateDirectoryObject                           ();\
    void on_NtCreateEnlistment                                ();\
    void on_NtCreateEvent                                     ();\
    void on_NtCreateEventPair                                 ();\
    void on_NtCreateFile                                      ();\
    void on_NtCreateIoCompletion                              ();\
    void on_NtCreateJobObject                                 ();\
    void on_NtCreateJobSet                                    ();\
    void on_NtCreateKeyedEvent                                ();\
    void on_NtCreateKey                                       ();\
    void on_NtCreateKeyTransacted                             ();\
    void on_NtCreateMailslotFile                              ();\
    void on_NtCreateMutant                                    ();\
    void on_NtCreateNamedPipeFile                             ();\
    void on_NtCreatePagingFile                                ();\
    void on_NtCreatePort                                      ();\
    void on_NtCreatePrivateNamespace                          ();\
    void on_NtCreateProcessEx                                 ();\
    void on_NtCreateProcess                                   ();\
    void on_NtCreateProfileEx                                 ();\
    void on_NtCreateProfile                                   ();\
    void on_NtCreateResourceManager                           ();\
    void on_NtCreateSection                                   ();\
    void on_NtCreateSemaphore                                 ();\
    void on_NtCreateSymbolicLinkObject                        ();\
    void on_NtCreateThreadEx                                  ();\
    void on_NtCreateThread                                    ();\
    void on_NtCreateTimer                                     ();\
    void on_NtCreateToken                                     ();\
    void on_NtCreateTransactionManager                        ();\
    void on_NtCreateTransaction                               ();\
    void on_NtCreateUserProcess                               ();\
    void on_NtCreateWaitablePort                              ();\
    void on_NtCreateWorkerFactory                             ();\
    void on_NtDebugActiveProcess                              ();\
    void on_NtDebugContinue                                   ();\
    void on_NtDelayExecution                                  ();\
    void on_NtDeleteAtom                                      ();\
    void on_NtDeleteBootEntry                                 ();\
    void on_NtDeleteDriverEntry                               ();\
    void on_NtDeleteFile                                      ();\
    void on_NtDeleteKey                                       ();\
    void on_NtDeleteObjectAuditAlarm                          ();\
    void on_NtDeletePrivateNamespace                          ();\
    void on_NtDeleteValueKey                                  ();\
    void on_NtDeviceIoControlFile                             ();\
    void on_NtDisplayString                                   ();\
    void on_NtDrawText                                        ();\
    void on_NtDuplicateObject                                 ();\
    void on_NtDuplicateToken                                  ();\
    void on_NtEnumerateBootEntries                            ();\
    void on_NtEnumerateDriverEntries                          ();\
    void on_NtEnumerateKey                                    ();\
    void on_NtEnumerateSystemEnvironmentValuesEx              ();\
    void on_NtEnumerateTransactionObject                      ();\
    void on_NtEnumerateValueKey                               ();\
    void on_NtExtendSection                                   ();\
    void on_NtFilterToken                                     ();\
    void on_NtFindAtom                                        ();\
    void on_NtFlushBuffersFile                                ();\
    void on_NtFlushInstallUILanguage                          ();\
    void on_NtFlushInstructionCache                           ();\
    void on_NtFlushKey                                        ();\
    void on_NtFlushVirtualMemory                              ();\
    void on_NtFreeUserPhysicalPages                           ();\
    void on_NtFreeVirtualMemory                               ();\
    void on_NtFreezeRegistry                                  ();\
    void on_NtFreezeTransactions                              ();\
    void on_NtFsControlFile                                   ();\
    void on_NtGetContextThread                                ();\
    void on_NtGetDevicePowerState                             ();\
    void on_NtGetMUIRegistryInfo                              ();\
    void on_NtGetNextProcess                                  ();\
    void on_NtGetNextThread                                   ();\
    void on_NtGetNlsSectionPtr                                ();\
    void on_NtGetNotificationResourceManager                  ();\
    void on_NtGetPlugPlayEvent                                ();\
    void on_NtGetWriteWatch                                   ();\
    void on_NtImpersonateAnonymousToken                       ();\
    void on_NtImpersonateClientOfPort                         ();\
    void on_NtImpersonateThread                               ();\
    void on_NtInitializeNlsFiles                              ();\
    void on_NtInitializeRegistry                              ();\
    void on_NtInitiatePowerAction                             ();\
    void on_NtIsProcessInJob                                  ();\
    void on_NtListenPort                                      ();\
    void on_NtLoadDriver                                      ();\
    void on_NtLoadKey2                                        ();\
    void on_NtLoadKeyEx                                       ();\
    void on_NtLoadKey                                         ();\
    void on_NtLockFile                                        ();\
    void on_NtLockProductActivationKeys                       ();\
    void on_NtLockRegistryKey                                 ();\
    void on_NtLockVirtualMemory                               ();\
    void on_NtMakePermanentObject                             ();\
    void on_NtMakeTemporaryObject                             ();\
    void on_NtMapCMFModule                                    ();\
    void on_NtMapUserPhysicalPages                            ();\
    void on_NtMapUserPhysicalPagesScatter                     ();\
    void on_NtMapViewOfSection                                ();\
    void on_NtModifyBootEntry                                 ();\
    void on_NtModifyDriverEntry                               ();\
    void on_NtNotifyChangeDirectoryFile                       ();\
    void on_NtNotifyChangeKey                                 ();\
    void on_NtNotifyChangeMultipleKeys                        ();\
    void on_NtNotifyChangeSession                             ();\
    void on_NtOpenDirectoryObject                             ();\
    void on_NtOpenEnlistment                                  ();\
    void on_NtOpenEvent                                       ();\
    void on_NtOpenEventPair                                   ();\
    void on_NtOpenFile                                        ();\
    void on_NtOpenIoCompletion                                ();\
    void on_NtOpenJobObject                                   ();\
    void on_NtOpenKeyedEvent                                  ();\
    void on_NtOpenKeyEx                                       ();\
    void on_NtOpenKey                                         ();\
    void on_NtOpenKeyTransactedEx                             ();\
    void on_NtOpenKeyTransacted                               ();\
    void on_NtOpenMutant                                      ();\
    void on_NtOpenObjectAuditAlarm                            ();\
    void on_NtOpenPrivateNamespace                            ();\
    void on_NtOpenProcess                                     ();\
    void on_NtOpenProcessTokenEx                              ();\
    void on_NtOpenProcessToken                                ();\
    void on_NtOpenResourceManager                             ();\
    void on_NtOpenSection                                     ();\
    void on_NtOpenSemaphore                                   ();\
    void on_NtOpenSession                                     ();\
    void on_NtOpenSymbolicLinkObject                          ();\
    void on_NtOpenThread                                      ();\
    void on_NtOpenThreadTokenEx                               ();\
    void on_NtOpenThreadToken                                 ();\
    void on_NtOpenTimer                                       ();\
    void on_NtOpenTransactionManager                          ();\
    void on_NtOpenTransaction                                 ();\
    void on_NtPlugPlayControl                                 ();\
    void on_NtPowerInformation                                ();\
    void on_NtPrepareComplete                                 ();\
    void on_NtPrepareEnlistment                               ();\
    void on_NtPrePrepareComplete                              ();\
    void on_NtPrePrepareEnlistment                            ();\
    void on_NtPrivilegeCheck                                  ();\
    void on_NtPrivilegedServiceAuditAlarm                     ();\
    void on_NtPrivilegeObjectAuditAlarm                       ();\
    void on_NtPropagationComplete                             ();\
    void on_NtPropagationFailed                               ();\
    void on_NtProtectVirtualMemory                            ();\
    void on_NtPulseEvent                                      ();\
    void on_NtQueryAttributesFile                             ();\
    void on_NtQueryBootEntryOrder                             ();\
    void on_NtQueryBootOptions                                ();\
    void on_NtQueryDebugFilterState                           ();\
    void on_NtQueryDefaultLocale                              ();\
    void on_NtQueryDefaultUILanguage                          ();\
    void on_NtQueryDirectoryFile                              ();\
    void on_NtQueryDirectoryObject                            ();\
    void on_NtQueryDriverEntryOrder                           ();\
    void on_NtQueryEaFile                                     ();\
    void on_NtQueryEvent                                      ();\
    void on_NtQueryFullAttributesFile                         ();\
    void on_NtQueryInformationAtom                            ();\
    void on_NtQueryInformationEnlistment                      ();\
    void on_NtQueryInformationFile                            ();\
    void on_NtQueryInformationJobObject                       ();\
    void on_NtQueryInformationPort                            ();\
    void on_NtQueryInformationProcess                         ();\
    void on_NtQueryInformationResourceManager                 ();\
    void on_NtQueryInformationThread                          ();\
    void on_NtQueryInformationToken                           ();\
    void on_NtQueryInformationTransaction                     ();\
    void on_NtQueryInformationTransactionManager              ();\
    void on_NtQueryInformationWorkerFactory                   ();\
    void on_NtQueryInstallUILanguage                          ();\
    void on_NtQueryIntervalProfile                            ();\
    void on_NtQueryIoCompletion                               ();\
    void on_NtQueryKey                                        ();\
    void on_NtQueryLicenseValue                               ();\
    void on_NtQueryMultipleValueKey                           ();\
    void on_NtQueryMutant                                     ();\
    void on_NtQueryObject                                     ();\
    void on_NtQueryOpenSubKeysEx                              ();\
    void on_NtQueryOpenSubKeys                                ();\
    void on_NtQueryPerformanceCounter                         ();\
    void on_NtQueryQuotaInformationFile                       ();\
    void on_NtQuerySection                                    ();\
    void on_NtQuerySecurityAttributesToken                    ();\
    void on_NtQuerySecurityObject                             ();\
    void on_NtQuerySemaphore                                  ();\
    void on_NtQuerySymbolicLinkObject                         ();\
    void on_NtQuerySystemEnvironmentValueEx                   ();\
    void on_NtQuerySystemEnvironmentValue                     ();\
    void on_NtQuerySystemInformationEx                        ();\
    void on_NtQuerySystemInformation                          ();\
    void on_NtQuerySystemTime                                 ();\
    void on_NtQueryTimer                                      ();\
    void on_NtQueryTimerResolution                            ();\
    void on_NtQueryValueKey                                   ();\
    void on_NtQueryVirtualMemory                              ();\
    void on_NtQueryVolumeInformationFile                      ();\
    void on_NtQueueApcThreadEx                                ();\
    void on_NtQueueApcThread                                  ();\
    void on_NtRaiseException                                  ();\
    void on_NtRaiseHardError                                  ();\
    void on_NtReadFile                                        ();\
    void on_NtReadFileScatter                                 ();\
    void on_NtReadOnlyEnlistment                              ();\
    void on_NtReadRequestData                                 ();\
    void on_NtReadVirtualMemory                               ();\
    void on_NtRecoverEnlistment                               ();\
    void on_NtRecoverResourceManager                          ();\
    void on_NtRecoverTransactionManager                       ();\
    void on_NtRegisterProtocolAddressInformation              ();\
    void on_NtRegisterThreadTerminatePort                     ();\
    void on_NtReleaseKeyedEvent                               ();\
    void on_NtReleaseMutant                                   ();\
    void on_NtReleaseSemaphore                                ();\
    void on_NtReleaseWorkerFactoryWorker                      ();\
    void on_NtRemoveIoCompletionEx                            ();\
    void on_NtRemoveIoCompletion                              ();\
    void on_NtRemoveProcessDebug                              ();\
    void on_NtRenameKey                                       ();\
    void on_NtRenameTransactionManager                        ();\
    void on_NtReplaceKey                                      ();\
    void on_NtReplacePartitionUnit                            ();\
    void on_NtReplyPort                                       ();\
    void on_NtReplyWaitReceivePortEx                          ();\
    void on_NtReplyWaitReceivePort                            ();\
    void on_NtReplyWaitReplyPort                              ();\
    void on_NtRequestPort                                     ();\
    void on_NtRequestWaitReplyPort                            ();\
    void on_NtResetEvent                                      ();\
    void on_NtResetWriteWatch                                 ();\
    void on_NtRestoreKey                                      ();\
    void on_NtResumeProcess                                   ();\
    void on_NtResumeThread                                    ();\
    void on_NtRollbackComplete                                ();\
    void on_NtRollbackEnlistment                              ();\
    void on_NtRollbackTransaction                             ();\
    void on_NtRollforwardTransactionManager                   ();\
    void on_NtSaveKeyEx                                       ();\
    void on_NtSaveKey                                         ();\
    void on_NtSaveMergedKeys                                  ();\
    void on_NtSecureConnectPort                               ();\
    void on_NtSetBootEntryOrder                               ();\
    void on_NtSetBootOptions                                  ();\
    void on_NtSetContextThread                                ();\
    void on_NtSetDebugFilterState                             ();\
    void on_NtSetDefaultHardErrorPort                         ();\
    void on_NtSetDefaultLocale                                ();\
    void on_NtSetDefaultUILanguage                            ();\
    void on_NtSetDriverEntryOrder                             ();\
    void on_NtSetEaFile                                       ();\
    void on_NtSetEventBoostPriority                           ();\
    void on_NtSetEvent                                        ();\
    void on_NtSetHighEventPair                                ();\
    void on_NtSetHighWaitLowEventPair                         ();\
    void on_NtSetInformationDebugObject                       ();\
    void on_NtSetInformationEnlistment                        ();\
    void on_NtSetInformationFile                              ();\
    void on_NtSetInformationJobObject                         ();\
    void on_NtSetInformationKey                               ();\
    void on_NtSetInformationObject                            ();\
    void on_NtSetInformationProcess                           ();\
    void on_NtSetInformationResourceManager                   ();\
    void on_NtSetInformationThread                            ();\
    void on_NtSetInformationToken                             ();\
    void on_NtSetInformationTransaction                       ();\
    void on_NtSetInformationTransactionManager                ();\
    void on_NtSetInformationWorkerFactory                     ();\
    void on_NtSetIntervalProfile                              ();\
    void on_NtSetIoCompletionEx                               ();\
    void on_NtSetIoCompletion                                 ();\
    void on_NtSetLdtEntries                                   ();\
    void on_NtSetLowEventPair                                 ();\
    void on_NtSetLowWaitHighEventPair                         ();\
    void on_NtSetQuotaInformationFile                         ();\
    void on_NtSetSecurityObject                               ();\
    void on_NtSetSystemEnvironmentValueEx                     ();\
    void on_NtSetSystemEnvironmentValue                       ();\
    void on_NtSetSystemInformation                            ();\
    void on_NtSetSystemPowerState                             ();\
    void on_NtSetSystemTime                                   ();\
    void on_NtSetThreadExecutionState                         ();\
    void on_NtSetTimerEx                                      ();\
    void on_NtSetTimer                                        ();\
    void on_NtSetTimerResolution                              ();\
    void on_NtSetUuidSeed                                     ();\
    void on_NtSetValueKey                                     ();\
    void on_NtSetVolumeInformationFile                        ();\
    void on_NtShutdownSystem                                  ();\
    void on_NtShutdownWorkerFactory                           ();\
    void on_NtSignalAndWaitForSingleObject                    ();\
    void on_NtSinglePhaseReject                               ();\
    void on_NtStartProfile                                    ();\
    void on_NtStopProfile                                     ();\
    void on_NtSuspendProcess                                  ();\
    void on_NtSuspendThread                                   ();\
    void on_NtSystemDebugControl                              ();\
    void on_NtTerminateJobObject                              ();\
    void on_NtTerminateProcess                                ();\
    void on_NtTerminateThread                                 ();\
    void on_NtTraceControl                                    ();\
    void on_NtTraceEvent                                      ();\
    void on_NtTranslateFilePath                               ();\
    void on_NtUnloadDriver                                    ();\
    void on_NtUnloadKey2                                      ();\
    void on_NtUnloadKeyEx                                     ();\
    void on_NtUnloadKey                                       ();\
    void on_NtUnlockFile                                      ();\
    void on_NtUnlockVirtualMemory                             ();\
    void on_NtUnmapViewOfSection                              ();\
    void on_NtVdmControl                                      ();\
    void on_NtWaitForDebugEvent                               ();\
    void on_NtWaitForKeyedEvent                               ();\
    void on_NtWaitForMultipleObjects32                        ();\
    void on_NtWaitForMultipleObjects                          ();\
    void on_NtWaitForSingleObject                             ();\
    void on_NtWaitForWorkViaWorkerFactory                     ();\
    void on_NtWaitHighEventPair                               ();\
    void on_NtWaitLowEventPair                                ();\
    void on_NtWorkerFactoryWorkerReady                        ();\
    void on_NtWriteFileGather                                 ();\
    void on_NtWriteFile                                       ();\
    void on_NtWriteRequestData                                ();\
    void on_NtWriteVirtualMemory                              ();\
    void on_NtDisableLastKnownGood                            ();\
    void on_NtEnableLastKnownGood                             ();\
    void on_NtFlushProcessWriteBuffers                        ();\
    void on_NtFlushWriteBuffer                                ();\
    void on_NtGetCurrentProcessorNumber                       ();\
    void on_NtIsSystemResumeAutomatic                         ();\
    void on_NtIsUILanguageComitted                            ();\
    void on_NtQueryPortInformationProcess                     ();\
    void on_NtSerializeBoot                                   ();\
    void on_NtTestAlert                                       ();\
    void on_NtThawRegistry                                    ();\
    void on_NtThawTransactions                                ();\
    void on_NtUmsThreadYield                                  ();\
    void on_NtYieldExecution                                  ();\
    void on_RtlpAllocateHeapInternal                          ();\
    void on_RtlFreeHeap                                       ();\
    void on_RtlpReAllocateHeapInternal                        ();\
    void on_RtlSizeHeap                                       ();\
    void on_RtlSetUserValueHeap                               ();\
    void on_RtlGetUserInfoHeap                                ();\
    bool register_NtAcceptConnectPort                               (proc_t proc, const on_NtAcceptConnectPort_fn& on_func);\
    bool register_NtAccessCheckAndAuditAlarm                        (proc_t proc, const on_NtAccessCheckAndAuditAlarm_fn& on_func);\
    bool register_NtAccessCheckByTypeAndAuditAlarm                  (proc_t proc, const on_NtAccessCheckByTypeAndAuditAlarm_fn& on_func);\
    bool register_NtAccessCheckByType                               (proc_t proc, const on_NtAccessCheckByType_fn& on_func);\
    bool register_NtAccessCheckByTypeResultListAndAuditAlarmByHandle(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle_fn& on_func);\
    bool register_NtAccessCheckByTypeResultListAndAuditAlarm        (proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarm_fn& on_func);\
    bool register_NtAccessCheckByTypeResultList                     (proc_t proc, const on_NtAccessCheckByTypeResultList_fn& on_func);\
    bool register_NtAccessCheck                                     (proc_t proc, const on_NtAccessCheck_fn& on_func);\
    bool register_NtAddAtom                                         (proc_t proc, const on_NtAddAtom_fn& on_func);\
    bool register_NtAddBootEntry                                    (proc_t proc, const on_NtAddBootEntry_fn& on_func);\
    bool register_NtAddDriverEntry                                  (proc_t proc, const on_NtAddDriverEntry_fn& on_func);\
    bool register_NtAdjustGroupsToken                               (proc_t proc, const on_NtAdjustGroupsToken_fn& on_func);\
    bool register_NtAdjustPrivilegesToken                           (proc_t proc, const on_NtAdjustPrivilegesToken_fn& on_func);\
    bool register_NtAlertResumeThread                               (proc_t proc, const on_NtAlertResumeThread_fn& on_func);\
    bool register_NtAlertThread                                     (proc_t proc, const on_NtAlertThread_fn& on_func);\
    bool register_NtAllocateLocallyUniqueId                         (proc_t proc, const on_NtAllocateLocallyUniqueId_fn& on_func);\
    bool register_NtAllocateReserveObject                           (proc_t proc, const on_NtAllocateReserveObject_fn& on_func);\
    bool register_NtAllocateUserPhysicalPages                       (proc_t proc, const on_NtAllocateUserPhysicalPages_fn& on_func);\
    bool register_NtAllocateUuids                                   (proc_t proc, const on_NtAllocateUuids_fn& on_func);\
    bool register_NtAllocateVirtualMemory                           (proc_t proc, const on_NtAllocateVirtualMemory_fn& on_func);\
    bool register_NtAlpcAcceptConnectPort                           (proc_t proc, const on_NtAlpcAcceptConnectPort_fn& on_func);\
    bool register_NtAlpcCancelMessage                               (proc_t proc, const on_NtAlpcCancelMessage_fn& on_func);\
    bool register_NtAlpcConnectPort                                 (proc_t proc, const on_NtAlpcConnectPort_fn& on_func);\
    bool register_NtAlpcCreatePort                                  (proc_t proc, const on_NtAlpcCreatePort_fn& on_func);\
    bool register_NtAlpcCreatePortSection                           (proc_t proc, const on_NtAlpcCreatePortSection_fn& on_func);\
    bool register_NtAlpcCreateResourceReserve                       (proc_t proc, const on_NtAlpcCreateResourceReserve_fn& on_func);\
    bool register_NtAlpcCreateSectionView                           (proc_t proc, const on_NtAlpcCreateSectionView_fn& on_func);\
    bool register_NtAlpcCreateSecurityContext                       (proc_t proc, const on_NtAlpcCreateSecurityContext_fn& on_func);\
    bool register_NtAlpcDeletePortSection                           (proc_t proc, const on_NtAlpcDeletePortSection_fn& on_func);\
    bool register_NtAlpcDeleteResourceReserve                       (proc_t proc, const on_NtAlpcDeleteResourceReserve_fn& on_func);\
    bool register_NtAlpcDeleteSectionView                           (proc_t proc, const on_NtAlpcDeleteSectionView_fn& on_func);\
    bool register_NtAlpcDeleteSecurityContext                       (proc_t proc, const on_NtAlpcDeleteSecurityContext_fn& on_func);\
    bool register_NtAlpcDisconnectPort                              (proc_t proc, const on_NtAlpcDisconnectPort_fn& on_func);\
    bool register_NtAlpcImpersonateClientOfPort                     (proc_t proc, const on_NtAlpcImpersonateClientOfPort_fn& on_func);\
    bool register_NtAlpcOpenSenderProcess                           (proc_t proc, const on_NtAlpcOpenSenderProcess_fn& on_func);\
    bool register_NtAlpcOpenSenderThread                            (proc_t proc, const on_NtAlpcOpenSenderThread_fn& on_func);\
    bool register_NtAlpcQueryInformation                            (proc_t proc, const on_NtAlpcQueryInformation_fn& on_func);\
    bool register_NtAlpcQueryInformationMessage                     (proc_t proc, const on_NtAlpcQueryInformationMessage_fn& on_func);\
    bool register_NtAlpcRevokeSecurityContext                       (proc_t proc, const on_NtAlpcRevokeSecurityContext_fn& on_func);\
    bool register_NtAlpcSendWaitReceivePort                         (proc_t proc, const on_NtAlpcSendWaitReceivePort_fn& on_func);\
    bool register_NtAlpcSetInformation                              (proc_t proc, const on_NtAlpcSetInformation_fn& on_func);\
    bool register_NtApphelpCacheControl                             (proc_t proc, const on_NtApphelpCacheControl_fn& on_func);\
    bool register_NtAreMappedFilesTheSame                           (proc_t proc, const on_NtAreMappedFilesTheSame_fn& on_func);\
    bool register_NtAssignProcessToJobObject                        (proc_t proc, const on_NtAssignProcessToJobObject_fn& on_func);\
    bool register_NtCallbackReturn                                  (proc_t proc, const on_NtCallbackReturn_fn& on_func);\
    bool register_NtCancelIoFileEx                                  (proc_t proc, const on_NtCancelIoFileEx_fn& on_func);\
    bool register_NtCancelIoFile                                    (proc_t proc, const on_NtCancelIoFile_fn& on_func);\
    bool register_NtCancelSynchronousIoFile                         (proc_t proc, const on_NtCancelSynchronousIoFile_fn& on_func);\
    bool register_NtCancelTimer                                     (proc_t proc, const on_NtCancelTimer_fn& on_func);\
    bool register_NtClearEvent                                      (proc_t proc, const on_NtClearEvent_fn& on_func);\
    bool register_NtClose                                           (proc_t proc, const on_NtClose_fn& on_func);\
    bool register_NtCloseObjectAuditAlarm                           (proc_t proc, const on_NtCloseObjectAuditAlarm_fn& on_func);\
    bool register_NtCommitComplete                                  (proc_t proc, const on_NtCommitComplete_fn& on_func);\
    bool register_NtCommitEnlistment                                (proc_t proc, const on_NtCommitEnlistment_fn& on_func);\
    bool register_NtCommitTransaction                               (proc_t proc, const on_NtCommitTransaction_fn& on_func);\
    bool register_NtCompactKeys                                     (proc_t proc, const on_NtCompactKeys_fn& on_func);\
    bool register_NtCompareTokens                                   (proc_t proc, const on_NtCompareTokens_fn& on_func);\
    bool register_NtCompleteConnectPort                             (proc_t proc, const on_NtCompleteConnectPort_fn& on_func);\
    bool register_NtCompressKey                                     (proc_t proc, const on_NtCompressKey_fn& on_func);\
    bool register_NtConnectPort                                     (proc_t proc, const on_NtConnectPort_fn& on_func);\
    bool register_NtContinue                                        (proc_t proc, const on_NtContinue_fn& on_func);\
    bool register_NtCreateDebugObject                               (proc_t proc, const on_NtCreateDebugObject_fn& on_func);\
    bool register_NtCreateDirectoryObject                           (proc_t proc, const on_NtCreateDirectoryObject_fn& on_func);\
    bool register_NtCreateEnlistment                                (proc_t proc, const on_NtCreateEnlistment_fn& on_func);\
    bool register_NtCreateEvent                                     (proc_t proc, const on_NtCreateEvent_fn& on_func);\
    bool register_NtCreateEventPair                                 (proc_t proc, const on_NtCreateEventPair_fn& on_func);\
    bool register_NtCreateFile                                      (proc_t proc, const on_NtCreateFile_fn& on_func);\
    bool register_NtCreateIoCompletion                              (proc_t proc, const on_NtCreateIoCompletion_fn& on_func);\
    bool register_NtCreateJobObject                                 (proc_t proc, const on_NtCreateJobObject_fn& on_func);\
    bool register_NtCreateJobSet                                    (proc_t proc, const on_NtCreateJobSet_fn& on_func);\
    bool register_NtCreateKeyedEvent                                (proc_t proc, const on_NtCreateKeyedEvent_fn& on_func);\
    bool register_NtCreateKey                                       (proc_t proc, const on_NtCreateKey_fn& on_func);\
    bool register_NtCreateKeyTransacted                             (proc_t proc, const on_NtCreateKeyTransacted_fn& on_func);\
    bool register_NtCreateMailslotFile                              (proc_t proc, const on_NtCreateMailslotFile_fn& on_func);\
    bool register_NtCreateMutant                                    (proc_t proc, const on_NtCreateMutant_fn& on_func);\
    bool register_NtCreateNamedPipeFile                             (proc_t proc, const on_NtCreateNamedPipeFile_fn& on_func);\
    bool register_NtCreatePagingFile                                (proc_t proc, const on_NtCreatePagingFile_fn& on_func);\
    bool register_NtCreatePort                                      (proc_t proc, const on_NtCreatePort_fn& on_func);\
    bool register_NtCreatePrivateNamespace                          (proc_t proc, const on_NtCreatePrivateNamespace_fn& on_func);\
    bool register_NtCreateProcessEx                                 (proc_t proc, const on_NtCreateProcessEx_fn& on_func);\
    bool register_NtCreateProcess                                   (proc_t proc, const on_NtCreateProcess_fn& on_func);\
    bool register_NtCreateProfileEx                                 (proc_t proc, const on_NtCreateProfileEx_fn& on_func);\
    bool register_NtCreateProfile                                   (proc_t proc, const on_NtCreateProfile_fn& on_func);\
    bool register_NtCreateResourceManager                           (proc_t proc, const on_NtCreateResourceManager_fn& on_func);\
    bool register_NtCreateSection                                   (proc_t proc, const on_NtCreateSection_fn& on_func);\
    bool register_NtCreateSemaphore                                 (proc_t proc, const on_NtCreateSemaphore_fn& on_func);\
    bool register_NtCreateSymbolicLinkObject                        (proc_t proc, const on_NtCreateSymbolicLinkObject_fn& on_func);\
    bool register_NtCreateThreadEx                                  (proc_t proc, const on_NtCreateThreadEx_fn& on_func);\
    bool register_NtCreateThread                                    (proc_t proc, const on_NtCreateThread_fn& on_func);\
    bool register_NtCreateTimer                                     (proc_t proc, const on_NtCreateTimer_fn& on_func);\
    bool register_NtCreateToken                                     (proc_t proc, const on_NtCreateToken_fn& on_func);\
    bool register_NtCreateTransactionManager                        (proc_t proc, const on_NtCreateTransactionManager_fn& on_func);\
    bool register_NtCreateTransaction                               (proc_t proc, const on_NtCreateTransaction_fn& on_func);\
    bool register_NtCreateUserProcess                               (proc_t proc, const on_NtCreateUserProcess_fn& on_func);\
    bool register_NtCreateWaitablePort                              (proc_t proc, const on_NtCreateWaitablePort_fn& on_func);\
    bool register_NtCreateWorkerFactory                             (proc_t proc, const on_NtCreateWorkerFactory_fn& on_func);\
    bool register_NtDebugActiveProcess                              (proc_t proc, const on_NtDebugActiveProcess_fn& on_func);\
    bool register_NtDebugContinue                                   (proc_t proc, const on_NtDebugContinue_fn& on_func);\
    bool register_NtDelayExecution                                  (proc_t proc, const on_NtDelayExecution_fn& on_func);\
    bool register_NtDeleteAtom                                      (proc_t proc, const on_NtDeleteAtom_fn& on_func);\
    bool register_NtDeleteBootEntry                                 (proc_t proc, const on_NtDeleteBootEntry_fn& on_func);\
    bool register_NtDeleteDriverEntry                               (proc_t proc, const on_NtDeleteDriverEntry_fn& on_func);\
    bool register_NtDeleteFile                                      (proc_t proc, const on_NtDeleteFile_fn& on_func);\
    bool register_NtDeleteKey                                       (proc_t proc, const on_NtDeleteKey_fn& on_func);\
    bool register_NtDeleteObjectAuditAlarm                          (proc_t proc, const on_NtDeleteObjectAuditAlarm_fn& on_func);\
    bool register_NtDeletePrivateNamespace                          (proc_t proc, const on_NtDeletePrivateNamespace_fn& on_func);\
    bool register_NtDeleteValueKey                                  (proc_t proc, const on_NtDeleteValueKey_fn& on_func);\
    bool register_NtDeviceIoControlFile                             (proc_t proc, const on_NtDeviceIoControlFile_fn& on_func);\
    bool register_NtDisplayString                                   (proc_t proc, const on_NtDisplayString_fn& on_func);\
    bool register_NtDrawText                                        (proc_t proc, const on_NtDrawText_fn& on_func);\
    bool register_NtDuplicateObject                                 (proc_t proc, const on_NtDuplicateObject_fn& on_func);\
    bool register_NtDuplicateToken                                  (proc_t proc, const on_NtDuplicateToken_fn& on_func);\
    bool register_NtEnumerateBootEntries                            (proc_t proc, const on_NtEnumerateBootEntries_fn& on_func);\
    bool register_NtEnumerateDriverEntries                          (proc_t proc, const on_NtEnumerateDriverEntries_fn& on_func);\
    bool register_NtEnumerateKey                                    (proc_t proc, const on_NtEnumerateKey_fn& on_func);\
    bool register_NtEnumerateSystemEnvironmentValuesEx              (proc_t proc, const on_NtEnumerateSystemEnvironmentValuesEx_fn& on_func);\
    bool register_NtEnumerateTransactionObject                      (proc_t proc, const on_NtEnumerateTransactionObject_fn& on_func);\
    bool register_NtEnumerateValueKey                               (proc_t proc, const on_NtEnumerateValueKey_fn& on_func);\
    bool register_NtExtendSection                                   (proc_t proc, const on_NtExtendSection_fn& on_func);\
    bool register_NtFilterToken                                     (proc_t proc, const on_NtFilterToken_fn& on_func);\
    bool register_NtFindAtom                                        (proc_t proc, const on_NtFindAtom_fn& on_func);\
    bool register_NtFlushBuffersFile                                (proc_t proc, const on_NtFlushBuffersFile_fn& on_func);\
    bool register_NtFlushInstallUILanguage                          (proc_t proc, const on_NtFlushInstallUILanguage_fn& on_func);\
    bool register_NtFlushInstructionCache                           (proc_t proc, const on_NtFlushInstructionCache_fn& on_func);\
    bool register_NtFlushKey                                        (proc_t proc, const on_NtFlushKey_fn& on_func);\
    bool register_NtFlushVirtualMemory                              (proc_t proc, const on_NtFlushVirtualMemory_fn& on_func);\
    bool register_NtFreeUserPhysicalPages                           (proc_t proc, const on_NtFreeUserPhysicalPages_fn& on_func);\
    bool register_NtFreeVirtualMemory                               (proc_t proc, const on_NtFreeVirtualMemory_fn& on_func);\
    bool register_NtFreezeRegistry                                  (proc_t proc, const on_NtFreezeRegistry_fn& on_func);\
    bool register_NtFreezeTransactions                              (proc_t proc, const on_NtFreezeTransactions_fn& on_func);\
    bool register_NtFsControlFile                                   (proc_t proc, const on_NtFsControlFile_fn& on_func);\
    bool register_NtGetContextThread                                (proc_t proc, const on_NtGetContextThread_fn& on_func);\
    bool register_NtGetDevicePowerState                             (proc_t proc, const on_NtGetDevicePowerState_fn& on_func);\
    bool register_NtGetMUIRegistryInfo                              (proc_t proc, const on_NtGetMUIRegistryInfo_fn& on_func);\
    bool register_NtGetNextProcess                                  (proc_t proc, const on_NtGetNextProcess_fn& on_func);\
    bool register_NtGetNextThread                                   (proc_t proc, const on_NtGetNextThread_fn& on_func);\
    bool register_NtGetNlsSectionPtr                                (proc_t proc, const on_NtGetNlsSectionPtr_fn& on_func);\
    bool register_NtGetNotificationResourceManager                  (proc_t proc, const on_NtGetNotificationResourceManager_fn& on_func);\
    bool register_NtGetPlugPlayEvent                                (proc_t proc, const on_NtGetPlugPlayEvent_fn& on_func);\
    bool register_NtGetWriteWatch                                   (proc_t proc, const on_NtGetWriteWatch_fn& on_func);\
    bool register_NtImpersonateAnonymousToken                       (proc_t proc, const on_NtImpersonateAnonymousToken_fn& on_func);\
    bool register_NtImpersonateClientOfPort                         (proc_t proc, const on_NtImpersonateClientOfPort_fn& on_func);\
    bool register_NtImpersonateThread                               (proc_t proc, const on_NtImpersonateThread_fn& on_func);\
    bool register_NtInitializeNlsFiles                              (proc_t proc, const on_NtInitializeNlsFiles_fn& on_func);\
    bool register_NtInitializeRegistry                              (proc_t proc, const on_NtInitializeRegistry_fn& on_func);\
    bool register_NtInitiatePowerAction                             (proc_t proc, const on_NtInitiatePowerAction_fn& on_func);\
    bool register_NtIsProcessInJob                                  (proc_t proc, const on_NtIsProcessInJob_fn& on_func);\
    bool register_NtListenPort                                      (proc_t proc, const on_NtListenPort_fn& on_func);\
    bool register_NtLoadDriver                                      (proc_t proc, const on_NtLoadDriver_fn& on_func);\
    bool register_NtLoadKey2                                        (proc_t proc, const on_NtLoadKey2_fn& on_func);\
    bool register_NtLoadKeyEx                                       (proc_t proc, const on_NtLoadKeyEx_fn& on_func);\
    bool register_NtLoadKey                                         (proc_t proc, const on_NtLoadKey_fn& on_func);\
    bool register_NtLockFile                                        (proc_t proc, const on_NtLockFile_fn& on_func);\
    bool register_NtLockProductActivationKeys                       (proc_t proc, const on_NtLockProductActivationKeys_fn& on_func);\
    bool register_NtLockRegistryKey                                 (proc_t proc, const on_NtLockRegistryKey_fn& on_func);\
    bool register_NtLockVirtualMemory                               (proc_t proc, const on_NtLockVirtualMemory_fn& on_func);\
    bool register_NtMakePermanentObject                             (proc_t proc, const on_NtMakePermanentObject_fn& on_func);\
    bool register_NtMakeTemporaryObject                             (proc_t proc, const on_NtMakeTemporaryObject_fn& on_func);\
    bool register_NtMapCMFModule                                    (proc_t proc, const on_NtMapCMFModule_fn& on_func);\
    bool register_NtMapUserPhysicalPages                            (proc_t proc, const on_NtMapUserPhysicalPages_fn& on_func);\
    bool register_NtMapUserPhysicalPagesScatter                     (proc_t proc, const on_NtMapUserPhysicalPagesScatter_fn& on_func);\
    bool register_NtMapViewOfSection                                (proc_t proc, const on_NtMapViewOfSection_fn& on_func);\
    bool register_NtModifyBootEntry                                 (proc_t proc, const on_NtModifyBootEntry_fn& on_func);\
    bool register_NtModifyDriverEntry                               (proc_t proc, const on_NtModifyDriverEntry_fn& on_func);\
    bool register_NtNotifyChangeDirectoryFile                       (proc_t proc, const on_NtNotifyChangeDirectoryFile_fn& on_func);\
    bool register_NtNotifyChangeKey                                 (proc_t proc, const on_NtNotifyChangeKey_fn& on_func);\
    bool register_NtNotifyChangeMultipleKeys                        (proc_t proc, const on_NtNotifyChangeMultipleKeys_fn& on_func);\
    bool register_NtNotifyChangeSession                             (proc_t proc, const on_NtNotifyChangeSession_fn& on_func);\
    bool register_NtOpenDirectoryObject                             (proc_t proc, const on_NtOpenDirectoryObject_fn& on_func);\
    bool register_NtOpenEnlistment                                  (proc_t proc, const on_NtOpenEnlistment_fn& on_func);\
    bool register_NtOpenEvent                                       (proc_t proc, const on_NtOpenEvent_fn& on_func);\
    bool register_NtOpenEventPair                                   (proc_t proc, const on_NtOpenEventPair_fn& on_func);\
    bool register_NtOpenFile                                        (proc_t proc, const on_NtOpenFile_fn& on_func);\
    bool register_NtOpenIoCompletion                                (proc_t proc, const on_NtOpenIoCompletion_fn& on_func);\
    bool register_NtOpenJobObject                                   (proc_t proc, const on_NtOpenJobObject_fn& on_func);\
    bool register_NtOpenKeyedEvent                                  (proc_t proc, const on_NtOpenKeyedEvent_fn& on_func);\
    bool register_NtOpenKeyEx                                       (proc_t proc, const on_NtOpenKeyEx_fn& on_func);\
    bool register_NtOpenKey                                         (proc_t proc, const on_NtOpenKey_fn& on_func);\
    bool register_NtOpenKeyTransactedEx                             (proc_t proc, const on_NtOpenKeyTransactedEx_fn& on_func);\
    bool register_NtOpenKeyTransacted                               (proc_t proc, const on_NtOpenKeyTransacted_fn& on_func);\
    bool register_NtOpenMutant                                      (proc_t proc, const on_NtOpenMutant_fn& on_func);\
    bool register_NtOpenObjectAuditAlarm                            (proc_t proc, const on_NtOpenObjectAuditAlarm_fn& on_func);\
    bool register_NtOpenPrivateNamespace                            (proc_t proc, const on_NtOpenPrivateNamespace_fn& on_func);\
    bool register_NtOpenProcess                                     (proc_t proc, const on_NtOpenProcess_fn& on_func);\
    bool register_NtOpenProcessTokenEx                              (proc_t proc, const on_NtOpenProcessTokenEx_fn& on_func);\
    bool register_NtOpenProcessToken                                (proc_t proc, const on_NtOpenProcessToken_fn& on_func);\
    bool register_NtOpenResourceManager                             (proc_t proc, const on_NtOpenResourceManager_fn& on_func);\
    bool register_NtOpenSection                                     (proc_t proc, const on_NtOpenSection_fn& on_func);\
    bool register_NtOpenSemaphore                                   (proc_t proc, const on_NtOpenSemaphore_fn& on_func);\
    bool register_NtOpenSession                                     (proc_t proc, const on_NtOpenSession_fn& on_func);\
    bool register_NtOpenSymbolicLinkObject                          (proc_t proc, const on_NtOpenSymbolicLinkObject_fn& on_func);\
    bool register_NtOpenThread                                      (proc_t proc, const on_NtOpenThread_fn& on_func);\
    bool register_NtOpenThreadTokenEx                               (proc_t proc, const on_NtOpenThreadTokenEx_fn& on_func);\
    bool register_NtOpenThreadToken                                 (proc_t proc, const on_NtOpenThreadToken_fn& on_func);\
    bool register_NtOpenTimer                                       (proc_t proc, const on_NtOpenTimer_fn& on_func);\
    bool register_NtOpenTransactionManager                          (proc_t proc, const on_NtOpenTransactionManager_fn& on_func);\
    bool register_NtOpenTransaction                                 (proc_t proc, const on_NtOpenTransaction_fn& on_func);\
    bool register_NtPlugPlayControl                                 (proc_t proc, const on_NtPlugPlayControl_fn& on_func);\
    bool register_NtPowerInformation                                (proc_t proc, const on_NtPowerInformation_fn& on_func);\
    bool register_NtPrepareComplete                                 (proc_t proc, const on_NtPrepareComplete_fn& on_func);\
    bool register_NtPrepareEnlistment                               (proc_t proc, const on_NtPrepareEnlistment_fn& on_func);\
    bool register_NtPrePrepareComplete                              (proc_t proc, const on_NtPrePrepareComplete_fn& on_func);\
    bool register_NtPrePrepareEnlistment                            (proc_t proc, const on_NtPrePrepareEnlistment_fn& on_func);\
    bool register_NtPrivilegeCheck                                  (proc_t proc, const on_NtPrivilegeCheck_fn& on_func);\
    bool register_NtPrivilegedServiceAuditAlarm                     (proc_t proc, const on_NtPrivilegedServiceAuditAlarm_fn& on_func);\
    bool register_NtPrivilegeObjectAuditAlarm                       (proc_t proc, const on_NtPrivilegeObjectAuditAlarm_fn& on_func);\
    bool register_NtPropagationComplete                             (proc_t proc, const on_NtPropagationComplete_fn& on_func);\
    bool register_NtPropagationFailed                               (proc_t proc, const on_NtPropagationFailed_fn& on_func);\
    bool register_NtProtectVirtualMemory                            (proc_t proc, const on_NtProtectVirtualMemory_fn& on_func);\
    bool register_NtPulseEvent                                      (proc_t proc, const on_NtPulseEvent_fn& on_func);\
    bool register_NtQueryAttributesFile                             (proc_t proc, const on_NtQueryAttributesFile_fn& on_func);\
    bool register_NtQueryBootEntryOrder                             (proc_t proc, const on_NtQueryBootEntryOrder_fn& on_func);\
    bool register_NtQueryBootOptions                                (proc_t proc, const on_NtQueryBootOptions_fn& on_func);\
    bool register_NtQueryDebugFilterState                           (proc_t proc, const on_NtQueryDebugFilterState_fn& on_func);\
    bool register_NtQueryDefaultLocale                              (proc_t proc, const on_NtQueryDefaultLocale_fn& on_func);\
    bool register_NtQueryDefaultUILanguage                          (proc_t proc, const on_NtQueryDefaultUILanguage_fn& on_func);\
    bool register_NtQueryDirectoryFile                              (proc_t proc, const on_NtQueryDirectoryFile_fn& on_func);\
    bool register_NtQueryDirectoryObject                            (proc_t proc, const on_NtQueryDirectoryObject_fn& on_func);\
    bool register_NtQueryDriverEntryOrder                           (proc_t proc, const on_NtQueryDriverEntryOrder_fn& on_func);\
    bool register_NtQueryEaFile                                     (proc_t proc, const on_NtQueryEaFile_fn& on_func);\
    bool register_NtQueryEvent                                      (proc_t proc, const on_NtQueryEvent_fn& on_func);\
    bool register_NtQueryFullAttributesFile                         (proc_t proc, const on_NtQueryFullAttributesFile_fn& on_func);\
    bool register_NtQueryInformationAtom                            (proc_t proc, const on_NtQueryInformationAtom_fn& on_func);\
    bool register_NtQueryInformationEnlistment                      (proc_t proc, const on_NtQueryInformationEnlistment_fn& on_func);\
    bool register_NtQueryInformationFile                            (proc_t proc, const on_NtQueryInformationFile_fn& on_func);\
    bool register_NtQueryInformationJobObject                       (proc_t proc, const on_NtQueryInformationJobObject_fn& on_func);\
    bool register_NtQueryInformationPort                            (proc_t proc, const on_NtQueryInformationPort_fn& on_func);\
    bool register_NtQueryInformationProcess                         (proc_t proc, const on_NtQueryInformationProcess_fn& on_func);\
    bool register_NtQueryInformationResourceManager                 (proc_t proc, const on_NtQueryInformationResourceManager_fn& on_func);\
    bool register_NtQueryInformationThread                          (proc_t proc, const on_NtQueryInformationThread_fn& on_func);\
    bool register_NtQueryInformationToken                           (proc_t proc, const on_NtQueryInformationToken_fn& on_func);\
    bool register_NtQueryInformationTransaction                     (proc_t proc, const on_NtQueryInformationTransaction_fn& on_func);\
    bool register_NtQueryInformationTransactionManager              (proc_t proc, const on_NtQueryInformationTransactionManager_fn& on_func);\
    bool register_NtQueryInformationWorkerFactory                   (proc_t proc, const on_NtQueryInformationWorkerFactory_fn& on_func);\
    bool register_NtQueryInstallUILanguage                          (proc_t proc, const on_NtQueryInstallUILanguage_fn& on_func);\
    bool register_NtQueryIntervalProfile                            (proc_t proc, const on_NtQueryIntervalProfile_fn& on_func);\
    bool register_NtQueryIoCompletion                               (proc_t proc, const on_NtQueryIoCompletion_fn& on_func);\
    bool register_NtQueryKey                                        (proc_t proc, const on_NtQueryKey_fn& on_func);\
    bool register_NtQueryLicenseValue                               (proc_t proc, const on_NtQueryLicenseValue_fn& on_func);\
    bool register_NtQueryMultipleValueKey                           (proc_t proc, const on_NtQueryMultipleValueKey_fn& on_func);\
    bool register_NtQueryMutant                                     (proc_t proc, const on_NtQueryMutant_fn& on_func);\
    bool register_NtQueryObject                                     (proc_t proc, const on_NtQueryObject_fn& on_func);\
    bool register_NtQueryOpenSubKeysEx                              (proc_t proc, const on_NtQueryOpenSubKeysEx_fn& on_func);\
    bool register_NtQueryOpenSubKeys                                (proc_t proc, const on_NtQueryOpenSubKeys_fn& on_func);\
    bool register_NtQueryPerformanceCounter                         (proc_t proc, const on_NtQueryPerformanceCounter_fn& on_func);\
    bool register_NtQueryQuotaInformationFile                       (proc_t proc, const on_NtQueryQuotaInformationFile_fn& on_func);\
    bool register_NtQuerySection                                    (proc_t proc, const on_NtQuerySection_fn& on_func);\
    bool register_NtQuerySecurityAttributesToken                    (proc_t proc, const on_NtQuerySecurityAttributesToken_fn& on_func);\
    bool register_NtQuerySecurityObject                             (proc_t proc, const on_NtQuerySecurityObject_fn& on_func);\
    bool register_NtQuerySemaphore                                  (proc_t proc, const on_NtQuerySemaphore_fn& on_func);\
    bool register_NtQuerySymbolicLinkObject                         (proc_t proc, const on_NtQuerySymbolicLinkObject_fn& on_func);\
    bool register_NtQuerySystemEnvironmentValueEx                   (proc_t proc, const on_NtQuerySystemEnvironmentValueEx_fn& on_func);\
    bool register_NtQuerySystemEnvironmentValue                     (proc_t proc, const on_NtQuerySystemEnvironmentValue_fn& on_func);\
    bool register_NtQuerySystemInformationEx                        (proc_t proc, const on_NtQuerySystemInformationEx_fn& on_func);\
    bool register_NtQuerySystemInformation                          (proc_t proc, const on_NtQuerySystemInformation_fn& on_func);\
    bool register_NtQuerySystemTime                                 (proc_t proc, const on_NtQuerySystemTime_fn& on_func);\
    bool register_NtQueryTimer                                      (proc_t proc, const on_NtQueryTimer_fn& on_func);\
    bool register_NtQueryTimerResolution                            (proc_t proc, const on_NtQueryTimerResolution_fn& on_func);\
    bool register_NtQueryValueKey                                   (proc_t proc, const on_NtQueryValueKey_fn& on_func);\
    bool register_NtQueryVirtualMemory                              (proc_t proc, const on_NtQueryVirtualMemory_fn& on_func);\
    bool register_NtQueryVolumeInformationFile                      (proc_t proc, const on_NtQueryVolumeInformationFile_fn& on_func);\
    bool register_NtQueueApcThreadEx                                (proc_t proc, const on_NtQueueApcThreadEx_fn& on_func);\
    bool register_NtQueueApcThread                                  (proc_t proc, const on_NtQueueApcThread_fn& on_func);\
    bool register_NtRaiseException                                  (proc_t proc, const on_NtRaiseException_fn& on_func);\
    bool register_NtRaiseHardError                                  (proc_t proc, const on_NtRaiseHardError_fn& on_func);\
    bool register_NtReadFile                                        (proc_t proc, const on_NtReadFile_fn& on_func);\
    bool register_NtReadFileScatter                                 (proc_t proc, const on_NtReadFileScatter_fn& on_func);\
    bool register_NtReadOnlyEnlistment                              (proc_t proc, const on_NtReadOnlyEnlistment_fn& on_func);\
    bool register_NtReadRequestData                                 (proc_t proc, const on_NtReadRequestData_fn& on_func);\
    bool register_NtReadVirtualMemory                               (proc_t proc, const on_NtReadVirtualMemory_fn& on_func);\
    bool register_NtRecoverEnlistment                               (proc_t proc, const on_NtRecoverEnlistment_fn& on_func);\
    bool register_NtRecoverResourceManager                          (proc_t proc, const on_NtRecoverResourceManager_fn& on_func);\
    bool register_NtRecoverTransactionManager                       (proc_t proc, const on_NtRecoverTransactionManager_fn& on_func);\
    bool register_NtRegisterProtocolAddressInformation              (proc_t proc, const on_NtRegisterProtocolAddressInformation_fn& on_func);\
    bool register_NtRegisterThreadTerminatePort                     (proc_t proc, const on_NtRegisterThreadTerminatePort_fn& on_func);\
    bool register_NtReleaseKeyedEvent                               (proc_t proc, const on_NtReleaseKeyedEvent_fn& on_func);\
    bool register_NtReleaseMutant                                   (proc_t proc, const on_NtReleaseMutant_fn& on_func);\
    bool register_NtReleaseSemaphore                                (proc_t proc, const on_NtReleaseSemaphore_fn& on_func);\
    bool register_NtReleaseWorkerFactoryWorker                      (proc_t proc, const on_NtReleaseWorkerFactoryWorker_fn& on_func);\
    bool register_NtRemoveIoCompletionEx                            (proc_t proc, const on_NtRemoveIoCompletionEx_fn& on_func);\
    bool register_NtRemoveIoCompletion                              (proc_t proc, const on_NtRemoveIoCompletion_fn& on_func);\
    bool register_NtRemoveProcessDebug                              (proc_t proc, const on_NtRemoveProcessDebug_fn& on_func);\
    bool register_NtRenameKey                                       (proc_t proc, const on_NtRenameKey_fn& on_func);\
    bool register_NtRenameTransactionManager                        (proc_t proc, const on_NtRenameTransactionManager_fn& on_func);\
    bool register_NtReplaceKey                                      (proc_t proc, const on_NtReplaceKey_fn& on_func);\
    bool register_NtReplacePartitionUnit                            (proc_t proc, const on_NtReplacePartitionUnit_fn& on_func);\
    bool register_NtReplyPort                                       (proc_t proc, const on_NtReplyPort_fn& on_func);\
    bool register_NtReplyWaitReceivePortEx                          (proc_t proc, const on_NtReplyWaitReceivePortEx_fn& on_func);\
    bool register_NtReplyWaitReceivePort                            (proc_t proc, const on_NtReplyWaitReceivePort_fn& on_func);\
    bool register_NtReplyWaitReplyPort                              (proc_t proc, const on_NtReplyWaitReplyPort_fn& on_func);\
    bool register_NtRequestPort                                     (proc_t proc, const on_NtRequestPort_fn& on_func);\
    bool register_NtRequestWaitReplyPort                            (proc_t proc, const on_NtRequestWaitReplyPort_fn& on_func);\
    bool register_NtResetEvent                                      (proc_t proc, const on_NtResetEvent_fn& on_func);\
    bool register_NtResetWriteWatch                                 (proc_t proc, const on_NtResetWriteWatch_fn& on_func);\
    bool register_NtRestoreKey                                      (proc_t proc, const on_NtRestoreKey_fn& on_func);\
    bool register_NtResumeProcess                                   (proc_t proc, const on_NtResumeProcess_fn& on_func);\
    bool register_NtResumeThread                                    (proc_t proc, const on_NtResumeThread_fn& on_func);\
    bool register_NtRollbackComplete                                (proc_t proc, const on_NtRollbackComplete_fn& on_func);\
    bool register_NtRollbackEnlistment                              (proc_t proc, const on_NtRollbackEnlistment_fn& on_func);\
    bool register_NtRollbackTransaction                             (proc_t proc, const on_NtRollbackTransaction_fn& on_func);\
    bool register_NtRollforwardTransactionManager                   (proc_t proc, const on_NtRollforwardTransactionManager_fn& on_func);\
    bool register_NtSaveKeyEx                                       (proc_t proc, const on_NtSaveKeyEx_fn& on_func);\
    bool register_NtSaveKey                                         (proc_t proc, const on_NtSaveKey_fn& on_func);\
    bool register_NtSaveMergedKeys                                  (proc_t proc, const on_NtSaveMergedKeys_fn& on_func);\
    bool register_NtSecureConnectPort                               (proc_t proc, const on_NtSecureConnectPort_fn& on_func);\
    bool register_NtSetBootEntryOrder                               (proc_t proc, const on_NtSetBootEntryOrder_fn& on_func);\
    bool register_NtSetBootOptions                                  (proc_t proc, const on_NtSetBootOptions_fn& on_func);\
    bool register_NtSetContextThread                                (proc_t proc, const on_NtSetContextThread_fn& on_func);\
    bool register_NtSetDebugFilterState                             (proc_t proc, const on_NtSetDebugFilterState_fn& on_func);\
    bool register_NtSetDefaultHardErrorPort                         (proc_t proc, const on_NtSetDefaultHardErrorPort_fn& on_func);\
    bool register_NtSetDefaultLocale                                (proc_t proc, const on_NtSetDefaultLocale_fn& on_func);\
    bool register_NtSetDefaultUILanguage                            (proc_t proc, const on_NtSetDefaultUILanguage_fn& on_func);\
    bool register_NtSetDriverEntryOrder                             (proc_t proc, const on_NtSetDriverEntryOrder_fn& on_func);\
    bool register_NtSetEaFile                                       (proc_t proc, const on_NtSetEaFile_fn& on_func);\
    bool register_NtSetEventBoostPriority                           (proc_t proc, const on_NtSetEventBoostPriority_fn& on_func);\
    bool register_NtSetEvent                                        (proc_t proc, const on_NtSetEvent_fn& on_func);\
    bool register_NtSetHighEventPair                                (proc_t proc, const on_NtSetHighEventPair_fn& on_func);\
    bool register_NtSetHighWaitLowEventPair                         (proc_t proc, const on_NtSetHighWaitLowEventPair_fn& on_func);\
    bool register_NtSetInformationDebugObject                       (proc_t proc, const on_NtSetInformationDebugObject_fn& on_func);\
    bool register_NtSetInformationEnlistment                        (proc_t proc, const on_NtSetInformationEnlistment_fn& on_func);\
    bool register_NtSetInformationFile                              (proc_t proc, const on_NtSetInformationFile_fn& on_func);\
    bool register_NtSetInformationJobObject                         (proc_t proc, const on_NtSetInformationJobObject_fn& on_func);\
    bool register_NtSetInformationKey                               (proc_t proc, const on_NtSetInformationKey_fn& on_func);\
    bool register_NtSetInformationObject                            (proc_t proc, const on_NtSetInformationObject_fn& on_func);\
    bool register_NtSetInformationProcess                           (proc_t proc, const on_NtSetInformationProcess_fn& on_func);\
    bool register_NtSetInformationResourceManager                   (proc_t proc, const on_NtSetInformationResourceManager_fn& on_func);\
    bool register_NtSetInformationThread                            (proc_t proc, const on_NtSetInformationThread_fn& on_func);\
    bool register_NtSetInformationToken                             (proc_t proc, const on_NtSetInformationToken_fn& on_func);\
    bool register_NtSetInformationTransaction                       (proc_t proc, const on_NtSetInformationTransaction_fn& on_func);\
    bool register_NtSetInformationTransactionManager                (proc_t proc, const on_NtSetInformationTransactionManager_fn& on_func);\
    bool register_NtSetInformationWorkerFactory                     (proc_t proc, const on_NtSetInformationWorkerFactory_fn& on_func);\
    bool register_NtSetIntervalProfile                              (proc_t proc, const on_NtSetIntervalProfile_fn& on_func);\
    bool register_NtSetIoCompletionEx                               (proc_t proc, const on_NtSetIoCompletionEx_fn& on_func);\
    bool register_NtSetIoCompletion                                 (proc_t proc, const on_NtSetIoCompletion_fn& on_func);\
    bool register_NtSetLdtEntries                                   (proc_t proc, const on_NtSetLdtEntries_fn& on_func);\
    bool register_NtSetLowEventPair                                 (proc_t proc, const on_NtSetLowEventPair_fn& on_func);\
    bool register_NtSetLowWaitHighEventPair                         (proc_t proc, const on_NtSetLowWaitHighEventPair_fn& on_func);\
    bool register_NtSetQuotaInformationFile                         (proc_t proc, const on_NtSetQuotaInformationFile_fn& on_func);\
    bool register_NtSetSecurityObject                               (proc_t proc, const on_NtSetSecurityObject_fn& on_func);\
    bool register_NtSetSystemEnvironmentValueEx                     (proc_t proc, const on_NtSetSystemEnvironmentValueEx_fn& on_func);\
    bool register_NtSetSystemEnvironmentValue                       (proc_t proc, const on_NtSetSystemEnvironmentValue_fn& on_func);\
    bool register_NtSetSystemInformation                            (proc_t proc, const on_NtSetSystemInformation_fn& on_func);\
    bool register_NtSetSystemPowerState                             (proc_t proc, const on_NtSetSystemPowerState_fn& on_func);\
    bool register_NtSetSystemTime                                   (proc_t proc, const on_NtSetSystemTime_fn& on_func);\
    bool register_NtSetThreadExecutionState                         (proc_t proc, const on_NtSetThreadExecutionState_fn& on_func);\
    bool register_NtSetTimerEx                                      (proc_t proc, const on_NtSetTimerEx_fn& on_func);\
    bool register_NtSetTimer                                        (proc_t proc, const on_NtSetTimer_fn& on_func);\
    bool register_NtSetTimerResolution                              (proc_t proc, const on_NtSetTimerResolution_fn& on_func);\
    bool register_NtSetUuidSeed                                     (proc_t proc, const on_NtSetUuidSeed_fn& on_func);\
    bool register_NtSetValueKey                                     (proc_t proc, const on_NtSetValueKey_fn& on_func);\
    bool register_NtSetVolumeInformationFile                        (proc_t proc, const on_NtSetVolumeInformationFile_fn& on_func);\
    bool register_NtShutdownSystem                                  (proc_t proc, const on_NtShutdownSystem_fn& on_func);\
    bool register_NtShutdownWorkerFactory                           (proc_t proc, const on_NtShutdownWorkerFactory_fn& on_func);\
    bool register_NtSignalAndWaitForSingleObject                    (proc_t proc, const on_NtSignalAndWaitForSingleObject_fn& on_func);\
    bool register_NtSinglePhaseReject                               (proc_t proc, const on_NtSinglePhaseReject_fn& on_func);\
    bool register_NtStartProfile                                    (proc_t proc, const on_NtStartProfile_fn& on_func);\
    bool register_NtStopProfile                                     (proc_t proc, const on_NtStopProfile_fn& on_func);\
    bool register_NtSuspendProcess                                  (proc_t proc, const on_NtSuspendProcess_fn& on_func);\
    bool register_NtSuspendThread                                   (proc_t proc, const on_NtSuspendThread_fn& on_func);\
    bool register_NtSystemDebugControl                              (proc_t proc, const on_NtSystemDebugControl_fn& on_func);\
    bool register_NtTerminateJobObject                              (proc_t proc, const on_NtTerminateJobObject_fn& on_func);\
    bool register_NtTerminateProcess                                (proc_t proc, const on_NtTerminateProcess_fn& on_func);\
    bool register_NtTerminateThread                                 (proc_t proc, const on_NtTerminateThread_fn& on_func);\
    bool register_NtTraceControl                                    (proc_t proc, const on_NtTraceControl_fn& on_func);\
    bool register_NtTraceEvent                                      (proc_t proc, const on_NtTraceEvent_fn& on_func);\
    bool register_NtTranslateFilePath                               (proc_t proc, const on_NtTranslateFilePath_fn& on_func);\
    bool register_NtUnloadDriver                                    (proc_t proc, const on_NtUnloadDriver_fn& on_func);\
    bool register_NtUnloadKey2                                      (proc_t proc, const on_NtUnloadKey2_fn& on_func);\
    bool register_NtUnloadKeyEx                                     (proc_t proc, const on_NtUnloadKeyEx_fn& on_func);\
    bool register_NtUnloadKey                                       (proc_t proc, const on_NtUnloadKey_fn& on_func);\
    bool register_NtUnlockFile                                      (proc_t proc, const on_NtUnlockFile_fn& on_func);\
    bool register_NtUnlockVirtualMemory                             (proc_t proc, const on_NtUnlockVirtualMemory_fn& on_func);\
    bool register_NtUnmapViewOfSection                              (proc_t proc, const on_NtUnmapViewOfSection_fn& on_func);\
    bool register_NtVdmControl                                      (proc_t proc, const on_NtVdmControl_fn& on_func);\
    bool register_NtWaitForDebugEvent                               (proc_t proc, const on_NtWaitForDebugEvent_fn& on_func);\
    bool register_NtWaitForKeyedEvent                               (proc_t proc, const on_NtWaitForKeyedEvent_fn& on_func);\
    bool register_NtWaitForMultipleObjects32                        (proc_t proc, const on_NtWaitForMultipleObjects32_fn& on_func);\
    bool register_NtWaitForMultipleObjects                          (proc_t proc, const on_NtWaitForMultipleObjects_fn& on_func);\
    bool register_NtWaitForSingleObject                             (proc_t proc, const on_NtWaitForSingleObject_fn& on_func);\
    bool register_NtWaitForWorkViaWorkerFactory                     (proc_t proc, const on_NtWaitForWorkViaWorkerFactory_fn& on_func);\
    bool register_NtWaitHighEventPair                               (proc_t proc, const on_NtWaitHighEventPair_fn& on_func);\
    bool register_NtWaitLowEventPair                                (proc_t proc, const on_NtWaitLowEventPair_fn& on_func);\
    bool register_NtWorkerFactoryWorkerReady                        (proc_t proc, const on_NtWorkerFactoryWorkerReady_fn& on_func);\
    bool register_NtWriteFileGather                                 (proc_t proc, const on_NtWriteFileGather_fn& on_func);\
    bool register_NtWriteFile                                       (proc_t proc, const on_NtWriteFile_fn& on_func);\
    bool register_NtWriteRequestData                                (proc_t proc, const on_NtWriteRequestData_fn& on_func);\
    bool register_NtWriteVirtualMemory                              (proc_t proc, const on_NtWriteVirtualMemory_fn& on_func);\
    bool register_NtDisableLastKnownGood                            (proc_t proc, const on_NtDisableLastKnownGood_fn& on_func);\
    bool register_NtEnableLastKnownGood                             (proc_t proc, const on_NtEnableLastKnownGood_fn& on_func);\
    bool register_NtFlushProcessWriteBuffers                        (proc_t proc, const on_NtFlushProcessWriteBuffers_fn& on_func);\
    bool register_NtFlushWriteBuffer                                (proc_t proc, const on_NtFlushWriteBuffer_fn& on_func);\
    bool register_NtGetCurrentProcessorNumber                       (proc_t proc, const on_NtGetCurrentProcessorNumber_fn& on_func);\
    bool register_NtIsSystemResumeAutomatic                         (proc_t proc, const on_NtIsSystemResumeAutomatic_fn& on_func);\
    bool register_NtIsUILanguageComitted                            (proc_t proc, const on_NtIsUILanguageComitted_fn& on_func);\
    bool register_NtQueryPortInformationProcess                     (proc_t proc, const on_NtQueryPortInformationProcess_fn& on_func);\
    bool register_NtSerializeBoot                                   (proc_t proc, const on_NtSerializeBoot_fn& on_func);\
    bool register_NtTestAlert                                       (proc_t proc, const on_NtTestAlert_fn& on_func);\
    bool register_NtThawRegistry                                    (proc_t proc, const on_NtThawRegistry_fn& on_func);\
    bool register_NtThawTransactions                                (proc_t proc, const on_NtThawTransactions_fn& on_func);\
    bool register_NtUmsThreadYield                                  (proc_t proc, const on_NtUmsThreadYield_fn& on_func);\
    bool register_NtYieldExecution                                  (proc_t proc, const on_NtYieldExecution_fn& on_func);\
    bool register_RtlpAllocateHeapInternal                          (proc_t proc, const on_RtlpAllocateHeapInternal_fn& on_func);\
    bool register_RtlFreeHeap                                       (proc_t proc, const on_RtlFreeHeap_fn& on_func);\
    bool register_RtlpReAllocateHeapInternal                        (proc_t proc, const on_RtlpReAllocateHeapInternal_fn& on_func);\
    bool register_RtlSizeHeap                                       (proc_t proc, const on_RtlSizeHeap_fn& on_func);\
    bool register_RtlSetUserValueHeap                               (proc_t proc, const on_RtlSetUserValueHeap_fn& on_func);\
    bool register_RtlGetUserInfoHeap                                (proc_t proc, const on_RtlGetUserInfoHeap_fn& on_func);

#define DECLARE_SYSCALLS_OBSERVERS\
    std::vector<on_NtAcceptConnectPort_fn>                                observers_NtAcceptConnectPort;\
    std::vector<on_NtAccessCheckAndAuditAlarm_fn>                         observers_NtAccessCheckAndAuditAlarm;\
    std::vector<on_NtAccessCheckByTypeAndAuditAlarm_fn>                   observers_NtAccessCheckByTypeAndAuditAlarm;\
    std::vector<on_NtAccessCheckByType_fn>                                observers_NtAccessCheckByType;\
    std::vector<on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle_fn> observers_NtAccessCheckByTypeResultListAndAuditAlarmByHandle;\
    std::vector<on_NtAccessCheckByTypeResultListAndAuditAlarm_fn>         observers_NtAccessCheckByTypeResultListAndAuditAlarm;\
    std::vector<on_NtAccessCheckByTypeResultList_fn>                      observers_NtAccessCheckByTypeResultList;\
    std::vector<on_NtAccessCheck_fn>                                      observers_NtAccessCheck;\
    std::vector<on_NtAddAtom_fn>                                          observers_NtAddAtom;\
    std::vector<on_NtAddBootEntry_fn>                                     observers_NtAddBootEntry;\
    std::vector<on_NtAddDriverEntry_fn>                                   observers_NtAddDriverEntry;\
    std::vector<on_NtAdjustGroupsToken_fn>                                observers_NtAdjustGroupsToken;\
    std::vector<on_NtAdjustPrivilegesToken_fn>                            observers_NtAdjustPrivilegesToken;\
    std::vector<on_NtAlertResumeThread_fn>                                observers_NtAlertResumeThread;\
    std::vector<on_NtAlertThread_fn>                                      observers_NtAlertThread;\
    std::vector<on_NtAllocateLocallyUniqueId_fn>                          observers_NtAllocateLocallyUniqueId;\
    std::vector<on_NtAllocateReserveObject_fn>                            observers_NtAllocateReserveObject;\
    std::vector<on_NtAllocateUserPhysicalPages_fn>                        observers_NtAllocateUserPhysicalPages;\
    std::vector<on_NtAllocateUuids_fn>                                    observers_NtAllocateUuids;\
    std::vector<on_NtAllocateVirtualMemory_fn>                            observers_NtAllocateVirtualMemory;\
    std::vector<on_NtAlpcAcceptConnectPort_fn>                            observers_NtAlpcAcceptConnectPort;\
    std::vector<on_NtAlpcCancelMessage_fn>                                observers_NtAlpcCancelMessage;\
    std::vector<on_NtAlpcConnectPort_fn>                                  observers_NtAlpcConnectPort;\
    std::vector<on_NtAlpcCreatePort_fn>                                   observers_NtAlpcCreatePort;\
    std::vector<on_NtAlpcCreatePortSection_fn>                            observers_NtAlpcCreatePortSection;\
    std::vector<on_NtAlpcCreateResourceReserve_fn>                        observers_NtAlpcCreateResourceReserve;\
    std::vector<on_NtAlpcCreateSectionView_fn>                            observers_NtAlpcCreateSectionView;\
    std::vector<on_NtAlpcCreateSecurityContext_fn>                        observers_NtAlpcCreateSecurityContext;\
    std::vector<on_NtAlpcDeletePortSection_fn>                            observers_NtAlpcDeletePortSection;\
    std::vector<on_NtAlpcDeleteResourceReserve_fn>                        observers_NtAlpcDeleteResourceReserve;\
    std::vector<on_NtAlpcDeleteSectionView_fn>                            observers_NtAlpcDeleteSectionView;\
    std::vector<on_NtAlpcDeleteSecurityContext_fn>                        observers_NtAlpcDeleteSecurityContext;\
    std::vector<on_NtAlpcDisconnectPort_fn>                               observers_NtAlpcDisconnectPort;\
    std::vector<on_NtAlpcImpersonateClientOfPort_fn>                      observers_NtAlpcImpersonateClientOfPort;\
    std::vector<on_NtAlpcOpenSenderProcess_fn>                            observers_NtAlpcOpenSenderProcess;\
    std::vector<on_NtAlpcOpenSenderThread_fn>                             observers_NtAlpcOpenSenderThread;\
    std::vector<on_NtAlpcQueryInformation_fn>                             observers_NtAlpcQueryInformation;\
    std::vector<on_NtAlpcQueryInformationMessage_fn>                      observers_NtAlpcQueryInformationMessage;\
    std::vector<on_NtAlpcRevokeSecurityContext_fn>                        observers_NtAlpcRevokeSecurityContext;\
    std::vector<on_NtAlpcSendWaitReceivePort_fn>                          observers_NtAlpcSendWaitReceivePort;\
    std::vector<on_NtAlpcSetInformation_fn>                               observers_NtAlpcSetInformation;\
    std::vector<on_NtApphelpCacheControl_fn>                              observers_NtApphelpCacheControl;\
    std::vector<on_NtAreMappedFilesTheSame_fn>                            observers_NtAreMappedFilesTheSame;\
    std::vector<on_NtAssignProcessToJobObject_fn>                         observers_NtAssignProcessToJobObject;\
    std::vector<on_NtCallbackReturn_fn>                                   observers_NtCallbackReturn;\
    std::vector<on_NtCancelIoFileEx_fn>                                   observers_NtCancelIoFileEx;\
    std::vector<on_NtCancelIoFile_fn>                                     observers_NtCancelIoFile;\
    std::vector<on_NtCancelSynchronousIoFile_fn>                          observers_NtCancelSynchronousIoFile;\
    std::vector<on_NtCancelTimer_fn>                                      observers_NtCancelTimer;\
    std::vector<on_NtClearEvent_fn>                                       observers_NtClearEvent;\
    std::vector<on_NtClose_fn>                                            observers_NtClose;\
    std::vector<on_NtCloseObjectAuditAlarm_fn>                            observers_NtCloseObjectAuditAlarm;\
    std::vector<on_NtCommitComplete_fn>                                   observers_NtCommitComplete;\
    std::vector<on_NtCommitEnlistment_fn>                                 observers_NtCommitEnlistment;\
    std::vector<on_NtCommitTransaction_fn>                                observers_NtCommitTransaction;\
    std::vector<on_NtCompactKeys_fn>                                      observers_NtCompactKeys;\
    std::vector<on_NtCompareTokens_fn>                                    observers_NtCompareTokens;\
    std::vector<on_NtCompleteConnectPort_fn>                              observers_NtCompleteConnectPort;\
    std::vector<on_NtCompressKey_fn>                                      observers_NtCompressKey;\
    std::vector<on_NtConnectPort_fn>                                      observers_NtConnectPort;\
    std::vector<on_NtContinue_fn>                                         observers_NtContinue;\
    std::vector<on_NtCreateDebugObject_fn>                                observers_NtCreateDebugObject;\
    std::vector<on_NtCreateDirectoryObject_fn>                            observers_NtCreateDirectoryObject;\
    std::vector<on_NtCreateEnlistment_fn>                                 observers_NtCreateEnlistment;\
    std::vector<on_NtCreateEvent_fn>                                      observers_NtCreateEvent;\
    std::vector<on_NtCreateEventPair_fn>                                  observers_NtCreateEventPair;\
    std::vector<on_NtCreateFile_fn>                                       observers_NtCreateFile;\
    std::vector<on_NtCreateIoCompletion_fn>                               observers_NtCreateIoCompletion;\
    std::vector<on_NtCreateJobObject_fn>                                  observers_NtCreateJobObject;\
    std::vector<on_NtCreateJobSet_fn>                                     observers_NtCreateJobSet;\
    std::vector<on_NtCreateKeyedEvent_fn>                                 observers_NtCreateKeyedEvent;\
    std::vector<on_NtCreateKey_fn>                                        observers_NtCreateKey;\
    std::vector<on_NtCreateKeyTransacted_fn>                              observers_NtCreateKeyTransacted;\
    std::vector<on_NtCreateMailslotFile_fn>                               observers_NtCreateMailslotFile;\
    std::vector<on_NtCreateMutant_fn>                                     observers_NtCreateMutant;\
    std::vector<on_NtCreateNamedPipeFile_fn>                              observers_NtCreateNamedPipeFile;\
    std::vector<on_NtCreatePagingFile_fn>                                 observers_NtCreatePagingFile;\
    std::vector<on_NtCreatePort_fn>                                       observers_NtCreatePort;\
    std::vector<on_NtCreatePrivateNamespace_fn>                           observers_NtCreatePrivateNamespace;\
    std::vector<on_NtCreateProcessEx_fn>                                  observers_NtCreateProcessEx;\
    std::vector<on_NtCreateProcess_fn>                                    observers_NtCreateProcess;\
    std::vector<on_NtCreateProfileEx_fn>                                  observers_NtCreateProfileEx;\
    std::vector<on_NtCreateProfile_fn>                                    observers_NtCreateProfile;\
    std::vector<on_NtCreateResourceManager_fn>                            observers_NtCreateResourceManager;\
    std::vector<on_NtCreateSection_fn>                                    observers_NtCreateSection;\
    std::vector<on_NtCreateSemaphore_fn>                                  observers_NtCreateSemaphore;\
    std::vector<on_NtCreateSymbolicLinkObject_fn>                         observers_NtCreateSymbolicLinkObject;\
    std::vector<on_NtCreateThreadEx_fn>                                   observers_NtCreateThreadEx;\
    std::vector<on_NtCreateThread_fn>                                     observers_NtCreateThread;\
    std::vector<on_NtCreateTimer_fn>                                      observers_NtCreateTimer;\
    std::vector<on_NtCreateToken_fn>                                      observers_NtCreateToken;\
    std::vector<on_NtCreateTransactionManager_fn>                         observers_NtCreateTransactionManager;\
    std::vector<on_NtCreateTransaction_fn>                                observers_NtCreateTransaction;\
    std::vector<on_NtCreateUserProcess_fn>                                observers_NtCreateUserProcess;\
    std::vector<on_NtCreateWaitablePort_fn>                               observers_NtCreateWaitablePort;\
    std::vector<on_NtCreateWorkerFactory_fn>                              observers_NtCreateWorkerFactory;\
    std::vector<on_NtDebugActiveProcess_fn>                               observers_NtDebugActiveProcess;\
    std::vector<on_NtDebugContinue_fn>                                    observers_NtDebugContinue;\
    std::vector<on_NtDelayExecution_fn>                                   observers_NtDelayExecution;\
    std::vector<on_NtDeleteAtom_fn>                                       observers_NtDeleteAtom;\
    std::vector<on_NtDeleteBootEntry_fn>                                  observers_NtDeleteBootEntry;\
    std::vector<on_NtDeleteDriverEntry_fn>                                observers_NtDeleteDriverEntry;\
    std::vector<on_NtDeleteFile_fn>                                       observers_NtDeleteFile;\
    std::vector<on_NtDeleteKey_fn>                                        observers_NtDeleteKey;\
    std::vector<on_NtDeleteObjectAuditAlarm_fn>                           observers_NtDeleteObjectAuditAlarm;\
    std::vector<on_NtDeletePrivateNamespace_fn>                           observers_NtDeletePrivateNamespace;\
    std::vector<on_NtDeleteValueKey_fn>                                   observers_NtDeleteValueKey;\
    std::vector<on_NtDeviceIoControlFile_fn>                              observers_NtDeviceIoControlFile;\
    std::vector<on_NtDisplayString_fn>                                    observers_NtDisplayString;\
    std::vector<on_NtDrawText_fn>                                         observers_NtDrawText;\
    std::vector<on_NtDuplicateObject_fn>                                  observers_NtDuplicateObject;\
    std::vector<on_NtDuplicateToken_fn>                                   observers_NtDuplicateToken;\
    std::vector<on_NtEnumerateBootEntries_fn>                             observers_NtEnumerateBootEntries;\
    std::vector<on_NtEnumerateDriverEntries_fn>                           observers_NtEnumerateDriverEntries;\
    std::vector<on_NtEnumerateKey_fn>                                     observers_NtEnumerateKey;\
    std::vector<on_NtEnumerateSystemEnvironmentValuesEx_fn>               observers_NtEnumerateSystemEnvironmentValuesEx;\
    std::vector<on_NtEnumerateTransactionObject_fn>                       observers_NtEnumerateTransactionObject;\
    std::vector<on_NtEnumerateValueKey_fn>                                observers_NtEnumerateValueKey;\
    std::vector<on_NtExtendSection_fn>                                    observers_NtExtendSection;\
    std::vector<on_NtFilterToken_fn>                                      observers_NtFilterToken;\
    std::vector<on_NtFindAtom_fn>                                         observers_NtFindAtom;\
    std::vector<on_NtFlushBuffersFile_fn>                                 observers_NtFlushBuffersFile;\
    std::vector<on_NtFlushInstallUILanguage_fn>                           observers_NtFlushInstallUILanguage;\
    std::vector<on_NtFlushInstructionCache_fn>                            observers_NtFlushInstructionCache;\
    std::vector<on_NtFlushKey_fn>                                         observers_NtFlushKey;\
    std::vector<on_NtFlushVirtualMemory_fn>                               observers_NtFlushVirtualMemory;\
    std::vector<on_NtFreeUserPhysicalPages_fn>                            observers_NtFreeUserPhysicalPages;\
    std::vector<on_NtFreeVirtualMemory_fn>                                observers_NtFreeVirtualMemory;\
    std::vector<on_NtFreezeRegistry_fn>                                   observers_NtFreezeRegistry;\
    std::vector<on_NtFreezeTransactions_fn>                               observers_NtFreezeTransactions;\
    std::vector<on_NtFsControlFile_fn>                                    observers_NtFsControlFile;\
    std::vector<on_NtGetContextThread_fn>                                 observers_NtGetContextThread;\
    std::vector<on_NtGetDevicePowerState_fn>                              observers_NtGetDevicePowerState;\
    std::vector<on_NtGetMUIRegistryInfo_fn>                               observers_NtGetMUIRegistryInfo;\
    std::vector<on_NtGetNextProcess_fn>                                   observers_NtGetNextProcess;\
    std::vector<on_NtGetNextThread_fn>                                    observers_NtGetNextThread;\
    std::vector<on_NtGetNlsSectionPtr_fn>                                 observers_NtGetNlsSectionPtr;\
    std::vector<on_NtGetNotificationResourceManager_fn>                   observers_NtGetNotificationResourceManager;\
    std::vector<on_NtGetPlugPlayEvent_fn>                                 observers_NtGetPlugPlayEvent;\
    std::vector<on_NtGetWriteWatch_fn>                                    observers_NtGetWriteWatch;\
    std::vector<on_NtImpersonateAnonymousToken_fn>                        observers_NtImpersonateAnonymousToken;\
    std::vector<on_NtImpersonateClientOfPort_fn>                          observers_NtImpersonateClientOfPort;\
    std::vector<on_NtImpersonateThread_fn>                                observers_NtImpersonateThread;\
    std::vector<on_NtInitializeNlsFiles_fn>                               observers_NtInitializeNlsFiles;\
    std::vector<on_NtInitializeRegistry_fn>                               observers_NtInitializeRegistry;\
    std::vector<on_NtInitiatePowerAction_fn>                              observers_NtInitiatePowerAction;\
    std::vector<on_NtIsProcessInJob_fn>                                   observers_NtIsProcessInJob;\
    std::vector<on_NtListenPort_fn>                                       observers_NtListenPort;\
    std::vector<on_NtLoadDriver_fn>                                       observers_NtLoadDriver;\
    std::vector<on_NtLoadKey2_fn>                                         observers_NtLoadKey2;\
    std::vector<on_NtLoadKeyEx_fn>                                        observers_NtLoadKeyEx;\
    std::vector<on_NtLoadKey_fn>                                          observers_NtLoadKey;\
    std::vector<on_NtLockFile_fn>                                         observers_NtLockFile;\
    std::vector<on_NtLockProductActivationKeys_fn>                        observers_NtLockProductActivationKeys;\
    std::vector<on_NtLockRegistryKey_fn>                                  observers_NtLockRegistryKey;\
    std::vector<on_NtLockVirtualMemory_fn>                                observers_NtLockVirtualMemory;\
    std::vector<on_NtMakePermanentObject_fn>                              observers_NtMakePermanentObject;\
    std::vector<on_NtMakeTemporaryObject_fn>                              observers_NtMakeTemporaryObject;\
    std::vector<on_NtMapCMFModule_fn>                                     observers_NtMapCMFModule;\
    std::vector<on_NtMapUserPhysicalPages_fn>                             observers_NtMapUserPhysicalPages;\
    std::vector<on_NtMapUserPhysicalPagesScatter_fn>                      observers_NtMapUserPhysicalPagesScatter;\
    std::vector<on_NtMapViewOfSection_fn>                                 observers_NtMapViewOfSection;\
    std::vector<on_NtModifyBootEntry_fn>                                  observers_NtModifyBootEntry;\
    std::vector<on_NtModifyDriverEntry_fn>                                observers_NtModifyDriverEntry;\
    std::vector<on_NtNotifyChangeDirectoryFile_fn>                        observers_NtNotifyChangeDirectoryFile;\
    std::vector<on_NtNotifyChangeKey_fn>                                  observers_NtNotifyChangeKey;\
    std::vector<on_NtNotifyChangeMultipleKeys_fn>                         observers_NtNotifyChangeMultipleKeys;\
    std::vector<on_NtNotifyChangeSession_fn>                              observers_NtNotifyChangeSession;\
    std::vector<on_NtOpenDirectoryObject_fn>                              observers_NtOpenDirectoryObject;\
    std::vector<on_NtOpenEnlistment_fn>                                   observers_NtOpenEnlistment;\
    std::vector<on_NtOpenEvent_fn>                                        observers_NtOpenEvent;\
    std::vector<on_NtOpenEventPair_fn>                                    observers_NtOpenEventPair;\
    std::vector<on_NtOpenFile_fn>                                         observers_NtOpenFile;\
    std::vector<on_NtOpenIoCompletion_fn>                                 observers_NtOpenIoCompletion;\
    std::vector<on_NtOpenJobObject_fn>                                    observers_NtOpenJobObject;\
    std::vector<on_NtOpenKeyedEvent_fn>                                   observers_NtOpenKeyedEvent;\
    std::vector<on_NtOpenKeyEx_fn>                                        observers_NtOpenKeyEx;\
    std::vector<on_NtOpenKey_fn>                                          observers_NtOpenKey;\
    std::vector<on_NtOpenKeyTransactedEx_fn>                              observers_NtOpenKeyTransactedEx;\
    std::vector<on_NtOpenKeyTransacted_fn>                                observers_NtOpenKeyTransacted;\
    std::vector<on_NtOpenMutant_fn>                                       observers_NtOpenMutant;\
    std::vector<on_NtOpenObjectAuditAlarm_fn>                             observers_NtOpenObjectAuditAlarm;\
    std::vector<on_NtOpenPrivateNamespace_fn>                             observers_NtOpenPrivateNamespace;\
    std::vector<on_NtOpenProcess_fn>                                      observers_NtOpenProcess;\
    std::vector<on_NtOpenProcessTokenEx_fn>                               observers_NtOpenProcessTokenEx;\
    std::vector<on_NtOpenProcessToken_fn>                                 observers_NtOpenProcessToken;\
    std::vector<on_NtOpenResourceManager_fn>                              observers_NtOpenResourceManager;\
    std::vector<on_NtOpenSection_fn>                                      observers_NtOpenSection;\
    std::vector<on_NtOpenSemaphore_fn>                                    observers_NtOpenSemaphore;\
    std::vector<on_NtOpenSession_fn>                                      observers_NtOpenSession;\
    std::vector<on_NtOpenSymbolicLinkObject_fn>                           observers_NtOpenSymbolicLinkObject;\
    std::vector<on_NtOpenThread_fn>                                       observers_NtOpenThread;\
    std::vector<on_NtOpenThreadTokenEx_fn>                                observers_NtOpenThreadTokenEx;\
    std::vector<on_NtOpenThreadToken_fn>                                  observers_NtOpenThreadToken;\
    std::vector<on_NtOpenTimer_fn>                                        observers_NtOpenTimer;\
    std::vector<on_NtOpenTransactionManager_fn>                           observers_NtOpenTransactionManager;\
    std::vector<on_NtOpenTransaction_fn>                                  observers_NtOpenTransaction;\
    std::vector<on_NtPlugPlayControl_fn>                                  observers_NtPlugPlayControl;\
    std::vector<on_NtPowerInformation_fn>                                 observers_NtPowerInformation;\
    std::vector<on_NtPrepareComplete_fn>                                  observers_NtPrepareComplete;\
    std::vector<on_NtPrepareEnlistment_fn>                                observers_NtPrepareEnlistment;\
    std::vector<on_NtPrePrepareComplete_fn>                               observers_NtPrePrepareComplete;\
    std::vector<on_NtPrePrepareEnlistment_fn>                             observers_NtPrePrepareEnlistment;\
    std::vector<on_NtPrivilegeCheck_fn>                                   observers_NtPrivilegeCheck;\
    std::vector<on_NtPrivilegedServiceAuditAlarm_fn>                      observers_NtPrivilegedServiceAuditAlarm;\
    std::vector<on_NtPrivilegeObjectAuditAlarm_fn>                        observers_NtPrivilegeObjectAuditAlarm;\
    std::vector<on_NtPropagationComplete_fn>                              observers_NtPropagationComplete;\
    std::vector<on_NtPropagationFailed_fn>                                observers_NtPropagationFailed;\
    std::vector<on_NtProtectVirtualMemory_fn>                             observers_NtProtectVirtualMemory;\
    std::vector<on_NtPulseEvent_fn>                                       observers_NtPulseEvent;\
    std::vector<on_NtQueryAttributesFile_fn>                              observers_NtQueryAttributesFile;\
    std::vector<on_NtQueryBootEntryOrder_fn>                              observers_NtQueryBootEntryOrder;\
    std::vector<on_NtQueryBootOptions_fn>                                 observers_NtQueryBootOptions;\
    std::vector<on_NtQueryDebugFilterState_fn>                            observers_NtQueryDebugFilterState;\
    std::vector<on_NtQueryDefaultLocale_fn>                               observers_NtQueryDefaultLocale;\
    std::vector<on_NtQueryDefaultUILanguage_fn>                           observers_NtQueryDefaultUILanguage;\
    std::vector<on_NtQueryDirectoryFile_fn>                               observers_NtQueryDirectoryFile;\
    std::vector<on_NtQueryDirectoryObject_fn>                             observers_NtQueryDirectoryObject;\
    std::vector<on_NtQueryDriverEntryOrder_fn>                            observers_NtQueryDriverEntryOrder;\
    std::vector<on_NtQueryEaFile_fn>                                      observers_NtQueryEaFile;\
    std::vector<on_NtQueryEvent_fn>                                       observers_NtQueryEvent;\
    std::vector<on_NtQueryFullAttributesFile_fn>                          observers_NtQueryFullAttributesFile;\
    std::vector<on_NtQueryInformationAtom_fn>                             observers_NtQueryInformationAtom;\
    std::vector<on_NtQueryInformationEnlistment_fn>                       observers_NtQueryInformationEnlistment;\
    std::vector<on_NtQueryInformationFile_fn>                             observers_NtQueryInformationFile;\
    std::vector<on_NtQueryInformationJobObject_fn>                        observers_NtQueryInformationJobObject;\
    std::vector<on_NtQueryInformationPort_fn>                             observers_NtQueryInformationPort;\
    std::vector<on_NtQueryInformationProcess_fn>                          observers_NtQueryInformationProcess;\
    std::vector<on_NtQueryInformationResourceManager_fn>                  observers_NtQueryInformationResourceManager;\
    std::vector<on_NtQueryInformationThread_fn>                           observers_NtQueryInformationThread;\
    std::vector<on_NtQueryInformationToken_fn>                            observers_NtQueryInformationToken;\
    std::vector<on_NtQueryInformationTransaction_fn>                      observers_NtQueryInformationTransaction;\
    std::vector<on_NtQueryInformationTransactionManager_fn>               observers_NtQueryInformationTransactionManager;\
    std::vector<on_NtQueryInformationWorkerFactory_fn>                    observers_NtQueryInformationWorkerFactory;\
    std::vector<on_NtQueryInstallUILanguage_fn>                           observers_NtQueryInstallUILanguage;\
    std::vector<on_NtQueryIntervalProfile_fn>                             observers_NtQueryIntervalProfile;\
    std::vector<on_NtQueryIoCompletion_fn>                                observers_NtQueryIoCompletion;\
    std::vector<on_NtQueryKey_fn>                                         observers_NtQueryKey;\
    std::vector<on_NtQueryLicenseValue_fn>                                observers_NtQueryLicenseValue;\
    std::vector<on_NtQueryMultipleValueKey_fn>                            observers_NtQueryMultipleValueKey;\
    std::vector<on_NtQueryMutant_fn>                                      observers_NtQueryMutant;\
    std::vector<on_NtQueryObject_fn>                                      observers_NtQueryObject;\
    std::vector<on_NtQueryOpenSubKeysEx_fn>                               observers_NtQueryOpenSubKeysEx;\
    std::vector<on_NtQueryOpenSubKeys_fn>                                 observers_NtQueryOpenSubKeys;\
    std::vector<on_NtQueryPerformanceCounter_fn>                          observers_NtQueryPerformanceCounter;\
    std::vector<on_NtQueryQuotaInformationFile_fn>                        observers_NtQueryQuotaInformationFile;\
    std::vector<on_NtQuerySection_fn>                                     observers_NtQuerySection;\
    std::vector<on_NtQuerySecurityAttributesToken_fn>                     observers_NtQuerySecurityAttributesToken;\
    std::vector<on_NtQuerySecurityObject_fn>                              observers_NtQuerySecurityObject;\
    std::vector<on_NtQuerySemaphore_fn>                                   observers_NtQuerySemaphore;\
    std::vector<on_NtQuerySymbolicLinkObject_fn>                          observers_NtQuerySymbolicLinkObject;\
    std::vector<on_NtQuerySystemEnvironmentValueEx_fn>                    observers_NtQuerySystemEnvironmentValueEx;\
    std::vector<on_NtQuerySystemEnvironmentValue_fn>                      observers_NtQuerySystemEnvironmentValue;\
    std::vector<on_NtQuerySystemInformationEx_fn>                         observers_NtQuerySystemInformationEx;\
    std::vector<on_NtQuerySystemInformation_fn>                           observers_NtQuerySystemInformation;\
    std::vector<on_NtQuerySystemTime_fn>                                  observers_NtQuerySystemTime;\
    std::vector<on_NtQueryTimer_fn>                                       observers_NtQueryTimer;\
    std::vector<on_NtQueryTimerResolution_fn>                             observers_NtQueryTimerResolution;\
    std::vector<on_NtQueryValueKey_fn>                                    observers_NtQueryValueKey;\
    std::vector<on_NtQueryVirtualMemory_fn>                               observers_NtQueryVirtualMemory;\
    std::vector<on_NtQueryVolumeInformationFile_fn>                       observers_NtQueryVolumeInformationFile;\
    std::vector<on_NtQueueApcThreadEx_fn>                                 observers_NtQueueApcThreadEx;\
    std::vector<on_NtQueueApcThread_fn>                                   observers_NtQueueApcThread;\
    std::vector<on_NtRaiseException_fn>                                   observers_NtRaiseException;\
    std::vector<on_NtRaiseHardError_fn>                                   observers_NtRaiseHardError;\
    std::vector<on_NtReadFile_fn>                                         observers_NtReadFile;\
    std::vector<on_NtReadFileScatter_fn>                                  observers_NtReadFileScatter;\
    std::vector<on_NtReadOnlyEnlistment_fn>                               observers_NtReadOnlyEnlistment;\
    std::vector<on_NtReadRequestData_fn>                                  observers_NtReadRequestData;\
    std::vector<on_NtReadVirtualMemory_fn>                                observers_NtReadVirtualMemory;\
    std::vector<on_NtRecoverEnlistment_fn>                                observers_NtRecoverEnlistment;\
    std::vector<on_NtRecoverResourceManager_fn>                           observers_NtRecoverResourceManager;\
    std::vector<on_NtRecoverTransactionManager_fn>                        observers_NtRecoverTransactionManager;\
    std::vector<on_NtRegisterProtocolAddressInformation_fn>               observers_NtRegisterProtocolAddressInformation;\
    std::vector<on_NtRegisterThreadTerminatePort_fn>                      observers_NtRegisterThreadTerminatePort;\
    std::vector<on_NtReleaseKeyedEvent_fn>                                observers_NtReleaseKeyedEvent;\
    std::vector<on_NtReleaseMutant_fn>                                    observers_NtReleaseMutant;\
    std::vector<on_NtReleaseSemaphore_fn>                                 observers_NtReleaseSemaphore;\
    std::vector<on_NtReleaseWorkerFactoryWorker_fn>                       observers_NtReleaseWorkerFactoryWorker;\
    std::vector<on_NtRemoveIoCompletionEx_fn>                             observers_NtRemoveIoCompletionEx;\
    std::vector<on_NtRemoveIoCompletion_fn>                               observers_NtRemoveIoCompletion;\
    std::vector<on_NtRemoveProcessDebug_fn>                               observers_NtRemoveProcessDebug;\
    std::vector<on_NtRenameKey_fn>                                        observers_NtRenameKey;\
    std::vector<on_NtRenameTransactionManager_fn>                         observers_NtRenameTransactionManager;\
    std::vector<on_NtReplaceKey_fn>                                       observers_NtReplaceKey;\
    std::vector<on_NtReplacePartitionUnit_fn>                             observers_NtReplacePartitionUnit;\
    std::vector<on_NtReplyPort_fn>                                        observers_NtReplyPort;\
    std::vector<on_NtReplyWaitReceivePortEx_fn>                           observers_NtReplyWaitReceivePortEx;\
    std::vector<on_NtReplyWaitReceivePort_fn>                             observers_NtReplyWaitReceivePort;\
    std::vector<on_NtReplyWaitReplyPort_fn>                               observers_NtReplyWaitReplyPort;\
    std::vector<on_NtRequestPort_fn>                                      observers_NtRequestPort;\
    std::vector<on_NtRequestWaitReplyPort_fn>                             observers_NtRequestWaitReplyPort;\
    std::vector<on_NtResetEvent_fn>                                       observers_NtResetEvent;\
    std::vector<on_NtResetWriteWatch_fn>                                  observers_NtResetWriteWatch;\
    std::vector<on_NtRestoreKey_fn>                                       observers_NtRestoreKey;\
    std::vector<on_NtResumeProcess_fn>                                    observers_NtResumeProcess;\
    std::vector<on_NtResumeThread_fn>                                     observers_NtResumeThread;\
    std::vector<on_NtRollbackComplete_fn>                                 observers_NtRollbackComplete;\
    std::vector<on_NtRollbackEnlistment_fn>                               observers_NtRollbackEnlistment;\
    std::vector<on_NtRollbackTransaction_fn>                              observers_NtRollbackTransaction;\
    std::vector<on_NtRollforwardTransactionManager_fn>                    observers_NtRollforwardTransactionManager;\
    std::vector<on_NtSaveKeyEx_fn>                                        observers_NtSaveKeyEx;\
    std::vector<on_NtSaveKey_fn>                                          observers_NtSaveKey;\
    std::vector<on_NtSaveMergedKeys_fn>                                   observers_NtSaveMergedKeys;\
    std::vector<on_NtSecureConnectPort_fn>                                observers_NtSecureConnectPort;\
    std::vector<on_NtSetBootEntryOrder_fn>                                observers_NtSetBootEntryOrder;\
    std::vector<on_NtSetBootOptions_fn>                                   observers_NtSetBootOptions;\
    std::vector<on_NtSetContextThread_fn>                                 observers_NtSetContextThread;\
    std::vector<on_NtSetDebugFilterState_fn>                              observers_NtSetDebugFilterState;\
    std::vector<on_NtSetDefaultHardErrorPort_fn>                          observers_NtSetDefaultHardErrorPort;\
    std::vector<on_NtSetDefaultLocale_fn>                                 observers_NtSetDefaultLocale;\
    std::vector<on_NtSetDefaultUILanguage_fn>                             observers_NtSetDefaultUILanguage;\
    std::vector<on_NtSetDriverEntryOrder_fn>                              observers_NtSetDriverEntryOrder;\
    std::vector<on_NtSetEaFile_fn>                                        observers_NtSetEaFile;\
    std::vector<on_NtSetEventBoostPriority_fn>                            observers_NtSetEventBoostPriority;\
    std::vector<on_NtSetEvent_fn>                                         observers_NtSetEvent;\
    std::vector<on_NtSetHighEventPair_fn>                                 observers_NtSetHighEventPair;\
    std::vector<on_NtSetHighWaitLowEventPair_fn>                          observers_NtSetHighWaitLowEventPair;\
    std::vector<on_NtSetInformationDebugObject_fn>                        observers_NtSetInformationDebugObject;\
    std::vector<on_NtSetInformationEnlistment_fn>                         observers_NtSetInformationEnlistment;\
    std::vector<on_NtSetInformationFile_fn>                               observers_NtSetInformationFile;\
    std::vector<on_NtSetInformationJobObject_fn>                          observers_NtSetInformationJobObject;\
    std::vector<on_NtSetInformationKey_fn>                                observers_NtSetInformationKey;\
    std::vector<on_NtSetInformationObject_fn>                             observers_NtSetInformationObject;\
    std::vector<on_NtSetInformationProcess_fn>                            observers_NtSetInformationProcess;\
    std::vector<on_NtSetInformationResourceManager_fn>                    observers_NtSetInformationResourceManager;\
    std::vector<on_NtSetInformationThread_fn>                             observers_NtSetInformationThread;\
    std::vector<on_NtSetInformationToken_fn>                              observers_NtSetInformationToken;\
    std::vector<on_NtSetInformationTransaction_fn>                        observers_NtSetInformationTransaction;\
    std::vector<on_NtSetInformationTransactionManager_fn>                 observers_NtSetInformationTransactionManager;\
    std::vector<on_NtSetInformationWorkerFactory_fn>                      observers_NtSetInformationWorkerFactory;\
    std::vector<on_NtSetIntervalProfile_fn>                               observers_NtSetIntervalProfile;\
    std::vector<on_NtSetIoCompletionEx_fn>                                observers_NtSetIoCompletionEx;\
    std::vector<on_NtSetIoCompletion_fn>                                  observers_NtSetIoCompletion;\
    std::vector<on_NtSetLdtEntries_fn>                                    observers_NtSetLdtEntries;\
    std::vector<on_NtSetLowEventPair_fn>                                  observers_NtSetLowEventPair;\
    std::vector<on_NtSetLowWaitHighEventPair_fn>                          observers_NtSetLowWaitHighEventPair;\
    std::vector<on_NtSetQuotaInformationFile_fn>                          observers_NtSetQuotaInformationFile;\
    std::vector<on_NtSetSecurityObject_fn>                                observers_NtSetSecurityObject;\
    std::vector<on_NtSetSystemEnvironmentValueEx_fn>                      observers_NtSetSystemEnvironmentValueEx;\
    std::vector<on_NtSetSystemEnvironmentValue_fn>                        observers_NtSetSystemEnvironmentValue;\
    std::vector<on_NtSetSystemInformation_fn>                             observers_NtSetSystemInformation;\
    std::vector<on_NtSetSystemPowerState_fn>                              observers_NtSetSystemPowerState;\
    std::vector<on_NtSetSystemTime_fn>                                    observers_NtSetSystemTime;\
    std::vector<on_NtSetThreadExecutionState_fn>                          observers_NtSetThreadExecutionState;\
    std::vector<on_NtSetTimerEx_fn>                                       observers_NtSetTimerEx;\
    std::vector<on_NtSetTimer_fn>                                         observers_NtSetTimer;\
    std::vector<on_NtSetTimerResolution_fn>                               observers_NtSetTimerResolution;\
    std::vector<on_NtSetUuidSeed_fn>                                      observers_NtSetUuidSeed;\
    std::vector<on_NtSetValueKey_fn>                                      observers_NtSetValueKey;\
    std::vector<on_NtSetVolumeInformationFile_fn>                         observers_NtSetVolumeInformationFile;\
    std::vector<on_NtShutdownSystem_fn>                                   observers_NtShutdownSystem;\
    std::vector<on_NtShutdownWorkerFactory_fn>                            observers_NtShutdownWorkerFactory;\
    std::vector<on_NtSignalAndWaitForSingleObject_fn>                     observers_NtSignalAndWaitForSingleObject;\
    std::vector<on_NtSinglePhaseReject_fn>                                observers_NtSinglePhaseReject;\
    std::vector<on_NtStartProfile_fn>                                     observers_NtStartProfile;\
    std::vector<on_NtStopProfile_fn>                                      observers_NtStopProfile;\
    std::vector<on_NtSuspendProcess_fn>                                   observers_NtSuspendProcess;\
    std::vector<on_NtSuspendThread_fn>                                    observers_NtSuspendThread;\
    std::vector<on_NtSystemDebugControl_fn>                               observers_NtSystemDebugControl;\
    std::vector<on_NtTerminateJobObject_fn>                               observers_NtTerminateJobObject;\
    std::vector<on_NtTerminateProcess_fn>                                 observers_NtTerminateProcess;\
    std::vector<on_NtTerminateThread_fn>                                  observers_NtTerminateThread;\
    std::vector<on_NtTraceControl_fn>                                     observers_NtTraceControl;\
    std::vector<on_NtTraceEvent_fn>                                       observers_NtTraceEvent;\
    std::vector<on_NtTranslateFilePath_fn>                                observers_NtTranslateFilePath;\
    std::vector<on_NtUnloadDriver_fn>                                     observers_NtUnloadDriver;\
    std::vector<on_NtUnloadKey2_fn>                                       observers_NtUnloadKey2;\
    std::vector<on_NtUnloadKeyEx_fn>                                      observers_NtUnloadKeyEx;\
    std::vector<on_NtUnloadKey_fn>                                        observers_NtUnloadKey;\
    std::vector<on_NtUnlockFile_fn>                                       observers_NtUnlockFile;\
    std::vector<on_NtUnlockVirtualMemory_fn>                              observers_NtUnlockVirtualMemory;\
    std::vector<on_NtUnmapViewOfSection_fn>                               observers_NtUnmapViewOfSection;\
    std::vector<on_NtVdmControl_fn>                                       observers_NtVdmControl;\
    std::vector<on_NtWaitForDebugEvent_fn>                                observers_NtWaitForDebugEvent;\
    std::vector<on_NtWaitForKeyedEvent_fn>                                observers_NtWaitForKeyedEvent;\
    std::vector<on_NtWaitForMultipleObjects32_fn>                         observers_NtWaitForMultipleObjects32;\
    std::vector<on_NtWaitForMultipleObjects_fn>                           observers_NtWaitForMultipleObjects;\
    std::vector<on_NtWaitForSingleObject_fn>                              observers_NtWaitForSingleObject;\
    std::vector<on_NtWaitForWorkViaWorkerFactory_fn>                      observers_NtWaitForWorkViaWorkerFactory;\
    std::vector<on_NtWaitHighEventPair_fn>                                observers_NtWaitHighEventPair;\
    std::vector<on_NtWaitLowEventPair_fn>                                 observers_NtWaitLowEventPair;\
    std::vector<on_NtWorkerFactoryWorkerReady_fn>                         observers_NtWorkerFactoryWorkerReady;\
    std::vector<on_NtWriteFileGather_fn>                                  observers_NtWriteFileGather;\
    std::vector<on_NtWriteFile_fn>                                        observers_NtWriteFile;\
    std::vector<on_NtWriteRequestData_fn>                                 observers_NtWriteRequestData;\
    std::vector<on_NtWriteVirtualMemory_fn>                               observers_NtWriteVirtualMemory;\
    std::vector<on_NtDisableLastKnownGood_fn>                             observers_NtDisableLastKnownGood;\
    std::vector<on_NtEnableLastKnownGood_fn>                              observers_NtEnableLastKnownGood;\
    std::vector<on_NtFlushProcessWriteBuffers_fn>                         observers_NtFlushProcessWriteBuffers;\
    std::vector<on_NtFlushWriteBuffer_fn>                                 observers_NtFlushWriteBuffer;\
    std::vector<on_NtGetCurrentProcessorNumber_fn>                        observers_NtGetCurrentProcessorNumber;\
    std::vector<on_NtIsSystemResumeAutomatic_fn>                          observers_NtIsSystemResumeAutomatic;\
    std::vector<on_NtIsUILanguageComitted_fn>                             observers_NtIsUILanguageComitted;\
    std::vector<on_NtQueryPortInformationProcess_fn>                      observers_NtQueryPortInformationProcess;\
    std::vector<on_NtSerializeBoot_fn>                                    observers_NtSerializeBoot;\
    std::vector<on_NtTestAlert_fn>                                        observers_NtTestAlert;\
    std::vector<on_NtThawRegistry_fn>                                     observers_NtThawRegistry;\
    std::vector<on_NtThawTransactions_fn>                                 observers_NtThawTransactions;\
    std::vector<on_NtUmsThreadYield_fn>                                   observers_NtUmsThreadYield;\
    std::vector<on_NtYieldExecution_fn>                                   observers_NtYieldExecution;\
    std::vector<on_RtlpAllocateHeapInternal_fn>                           observers_RtlpAllocateHeapInternal;\
    std::vector<on_RtlFreeHeap_fn>                                        observers_RtlFreeHeap;\
    std::vector<on_RtlpReAllocateHeapInternal_fn>                         observers_RtlpReAllocateHeapInternal;\
    std::vector<on_RtlSizeHeap_fn>                                        observers_RtlSizeHeap;\
    std::vector<on_RtlSetUserValueHeap_fn>                                observers_RtlSetUserValueHeap;\
    std::vector<on_RtlGetUserInfoHeap_fn>                                 observers_RtlGetUserInfoHeap;

#define DECLARE_SYSCALLS_HANDLERS\
    {"NtAcceptConnectPort",                                &monitor::GenericMonitor::on_NtAcceptConnectPort},\
    {"NtAccessCheckAndAuditAlarm",                         &monitor::GenericMonitor::on_NtAccessCheckAndAuditAlarm},\
    {"NtAccessCheckByTypeAndAuditAlarm",                   &monitor::GenericMonitor::on_NtAccessCheckByTypeAndAuditAlarm},\
    {"NtAccessCheckByType",                                &monitor::GenericMonitor::on_NtAccessCheckByType},\
    {"NtAccessCheckByTypeResultListAndAuditAlarmByHandle", &monitor::GenericMonitor::on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle},\
    {"NtAccessCheckByTypeResultListAndAuditAlarm",         &monitor::GenericMonitor::on_NtAccessCheckByTypeResultListAndAuditAlarm},\
    {"NtAccessCheckByTypeResultList",                      &monitor::GenericMonitor::on_NtAccessCheckByTypeResultList},\
    {"NtAccessCheck",                                      &monitor::GenericMonitor::on_NtAccessCheck},\
    {"NtAddAtom",                                          &monitor::GenericMonitor::on_NtAddAtom},\
    {"NtAddBootEntry",                                     &monitor::GenericMonitor::on_NtAddBootEntry},\
    {"NtAddDriverEntry",                                   &monitor::GenericMonitor::on_NtAddDriverEntry},\
    {"NtAdjustGroupsToken",                                &monitor::GenericMonitor::on_NtAdjustGroupsToken},\
    {"NtAdjustPrivilegesToken",                            &monitor::GenericMonitor::on_NtAdjustPrivilegesToken},\
    {"NtAlertResumeThread",                                &monitor::GenericMonitor::on_NtAlertResumeThread},\
    {"NtAlertThread",                                      &monitor::GenericMonitor::on_NtAlertThread},\
    {"NtAllocateLocallyUniqueId",                          &monitor::GenericMonitor::on_NtAllocateLocallyUniqueId},\
    {"NtAllocateReserveObject",                            &monitor::GenericMonitor::on_NtAllocateReserveObject},\
    {"NtAllocateUserPhysicalPages",                        &monitor::GenericMonitor::on_NtAllocateUserPhysicalPages},\
    {"NtAllocateUuids",                                    &monitor::GenericMonitor::on_NtAllocateUuids},\
    {"NtAllocateVirtualMemory",                            &monitor::GenericMonitor::on_NtAllocateVirtualMemory},\
    {"NtAlpcAcceptConnectPort",                            &monitor::GenericMonitor::on_NtAlpcAcceptConnectPort},\
    {"NtAlpcCancelMessage",                                &monitor::GenericMonitor::on_NtAlpcCancelMessage},\
    {"NtAlpcConnectPort",                                  &monitor::GenericMonitor::on_NtAlpcConnectPort},\
    {"NtAlpcCreatePort",                                   &monitor::GenericMonitor::on_NtAlpcCreatePort},\
    {"NtAlpcCreatePortSection",                            &monitor::GenericMonitor::on_NtAlpcCreatePortSection},\
    {"NtAlpcCreateResourceReserve",                        &monitor::GenericMonitor::on_NtAlpcCreateResourceReserve},\
    {"NtAlpcCreateSectionView",                            &monitor::GenericMonitor::on_NtAlpcCreateSectionView},\
    {"NtAlpcCreateSecurityContext",                        &monitor::GenericMonitor::on_NtAlpcCreateSecurityContext},\
    {"NtAlpcDeletePortSection",                            &monitor::GenericMonitor::on_NtAlpcDeletePortSection},\
    {"NtAlpcDeleteResourceReserve",                        &monitor::GenericMonitor::on_NtAlpcDeleteResourceReserve},\
    {"NtAlpcDeleteSectionView",                            &monitor::GenericMonitor::on_NtAlpcDeleteSectionView},\
    {"NtAlpcDeleteSecurityContext",                        &monitor::GenericMonitor::on_NtAlpcDeleteSecurityContext},\
    {"NtAlpcDisconnectPort",                               &monitor::GenericMonitor::on_NtAlpcDisconnectPort},\
    {"NtAlpcImpersonateClientOfPort",                      &monitor::GenericMonitor::on_NtAlpcImpersonateClientOfPort},\
    {"NtAlpcOpenSenderProcess",                            &monitor::GenericMonitor::on_NtAlpcOpenSenderProcess},\
    {"NtAlpcOpenSenderThread",                             &monitor::GenericMonitor::on_NtAlpcOpenSenderThread},\
    {"NtAlpcQueryInformation",                             &monitor::GenericMonitor::on_NtAlpcQueryInformation},\
    {"NtAlpcQueryInformationMessage",                      &monitor::GenericMonitor::on_NtAlpcQueryInformationMessage},\
    {"NtAlpcRevokeSecurityContext",                        &monitor::GenericMonitor::on_NtAlpcRevokeSecurityContext},\
    {"NtAlpcSendWaitReceivePort",                          &monitor::GenericMonitor::on_NtAlpcSendWaitReceivePort},\
    {"NtAlpcSetInformation",                               &monitor::GenericMonitor::on_NtAlpcSetInformation},\
    {"NtApphelpCacheControl",                              &monitor::GenericMonitor::on_NtApphelpCacheControl},\
    {"NtAreMappedFilesTheSame",                            &monitor::GenericMonitor::on_NtAreMappedFilesTheSame},\
    {"NtAssignProcessToJobObject",                         &monitor::GenericMonitor::on_NtAssignProcessToJobObject},\
    {"NtCallbackReturn",                                   &monitor::GenericMonitor::on_NtCallbackReturn},\
    {"NtCancelIoFileEx",                                   &monitor::GenericMonitor::on_NtCancelIoFileEx},\
    {"NtCancelIoFile",                                     &monitor::GenericMonitor::on_NtCancelIoFile},\
    {"NtCancelSynchronousIoFile",                          &monitor::GenericMonitor::on_NtCancelSynchronousIoFile},\
    {"NtCancelTimer",                                      &monitor::GenericMonitor::on_NtCancelTimer},\
    {"NtClearEvent",                                       &monitor::GenericMonitor::on_NtClearEvent},\
    {"NtClose",                                            &monitor::GenericMonitor::on_NtClose},\
    {"NtCloseObjectAuditAlarm",                            &monitor::GenericMonitor::on_NtCloseObjectAuditAlarm},\
    {"NtCommitComplete",                                   &monitor::GenericMonitor::on_NtCommitComplete},\
    {"NtCommitEnlistment",                                 &monitor::GenericMonitor::on_NtCommitEnlistment},\
    {"NtCommitTransaction",                                &monitor::GenericMonitor::on_NtCommitTransaction},\
    {"NtCompactKeys",                                      &monitor::GenericMonitor::on_NtCompactKeys},\
    {"NtCompareTokens",                                    &monitor::GenericMonitor::on_NtCompareTokens},\
    {"NtCompleteConnectPort",                              &monitor::GenericMonitor::on_NtCompleteConnectPort},\
    {"NtCompressKey",                                      &monitor::GenericMonitor::on_NtCompressKey},\
    {"NtConnectPort",                                      &monitor::GenericMonitor::on_NtConnectPort},\
    {"NtContinue",                                         &monitor::GenericMonitor::on_NtContinue},\
    {"NtCreateDebugObject",                                &monitor::GenericMonitor::on_NtCreateDebugObject},\
    {"NtCreateDirectoryObject",                            &monitor::GenericMonitor::on_NtCreateDirectoryObject},\
    {"NtCreateEnlistment",                                 &monitor::GenericMonitor::on_NtCreateEnlistment},\
    {"NtCreateEvent",                                      &monitor::GenericMonitor::on_NtCreateEvent},\
    {"NtCreateEventPair",                                  &monitor::GenericMonitor::on_NtCreateEventPair},\
    {"NtCreateFile",                                       &monitor::GenericMonitor::on_NtCreateFile},\
    {"NtCreateIoCompletion",                               &monitor::GenericMonitor::on_NtCreateIoCompletion},\
    {"NtCreateJobObject",                                  &monitor::GenericMonitor::on_NtCreateJobObject},\
    {"NtCreateJobSet",                                     &monitor::GenericMonitor::on_NtCreateJobSet},\
    {"NtCreateKeyedEvent",                                 &monitor::GenericMonitor::on_NtCreateKeyedEvent},\
    {"NtCreateKey",                                        &monitor::GenericMonitor::on_NtCreateKey},\
    {"NtCreateKeyTransacted",                              &monitor::GenericMonitor::on_NtCreateKeyTransacted},\
    {"NtCreateMailslotFile",                               &monitor::GenericMonitor::on_NtCreateMailslotFile},\
    {"NtCreateMutant",                                     &monitor::GenericMonitor::on_NtCreateMutant},\
    {"NtCreateNamedPipeFile",                              &monitor::GenericMonitor::on_NtCreateNamedPipeFile},\
    {"NtCreatePagingFile",                                 &monitor::GenericMonitor::on_NtCreatePagingFile},\
    {"NtCreatePort",                                       &monitor::GenericMonitor::on_NtCreatePort},\
    {"NtCreatePrivateNamespace",                           &monitor::GenericMonitor::on_NtCreatePrivateNamespace},\
    {"NtCreateProcessEx",                                  &monitor::GenericMonitor::on_NtCreateProcessEx},\
    {"NtCreateProcess",                                    &monitor::GenericMonitor::on_NtCreateProcess},\
    {"NtCreateProfileEx",                                  &monitor::GenericMonitor::on_NtCreateProfileEx},\
    {"NtCreateProfile",                                    &monitor::GenericMonitor::on_NtCreateProfile},\
    {"NtCreateResourceManager",                            &monitor::GenericMonitor::on_NtCreateResourceManager},\
    {"NtCreateSection",                                    &monitor::GenericMonitor::on_NtCreateSection},\
    {"NtCreateSemaphore",                                  &monitor::GenericMonitor::on_NtCreateSemaphore},\
    {"NtCreateSymbolicLinkObject",                         &monitor::GenericMonitor::on_NtCreateSymbolicLinkObject},\
    {"NtCreateThreadEx",                                   &monitor::GenericMonitor::on_NtCreateThreadEx},\
    {"NtCreateThread",                                     &monitor::GenericMonitor::on_NtCreateThread},\
    {"NtCreateTimer",                                      &monitor::GenericMonitor::on_NtCreateTimer},\
    {"NtCreateToken",                                      &monitor::GenericMonitor::on_NtCreateToken},\
    {"NtCreateTransactionManager",                         &monitor::GenericMonitor::on_NtCreateTransactionManager},\
    {"NtCreateTransaction",                                &monitor::GenericMonitor::on_NtCreateTransaction},\
    {"NtCreateUserProcess",                                &monitor::GenericMonitor::on_NtCreateUserProcess},\
    {"NtCreateWaitablePort",                               &monitor::GenericMonitor::on_NtCreateWaitablePort},\
    {"NtCreateWorkerFactory",                              &monitor::GenericMonitor::on_NtCreateWorkerFactory},\
    {"NtDebugActiveProcess",                               &monitor::GenericMonitor::on_NtDebugActiveProcess},\
    {"NtDebugContinue",                                    &monitor::GenericMonitor::on_NtDebugContinue},\
    {"NtDelayExecution",                                   &monitor::GenericMonitor::on_NtDelayExecution},\
    {"NtDeleteAtom",                                       &monitor::GenericMonitor::on_NtDeleteAtom},\
    {"NtDeleteBootEntry",                                  &monitor::GenericMonitor::on_NtDeleteBootEntry},\
    {"NtDeleteDriverEntry",                                &monitor::GenericMonitor::on_NtDeleteDriverEntry},\
    {"NtDeleteFile",                                       &monitor::GenericMonitor::on_NtDeleteFile},\
    {"NtDeleteKey",                                        &monitor::GenericMonitor::on_NtDeleteKey},\
    {"NtDeleteObjectAuditAlarm",                           &monitor::GenericMonitor::on_NtDeleteObjectAuditAlarm},\
    {"NtDeletePrivateNamespace",                           &monitor::GenericMonitor::on_NtDeletePrivateNamespace},\
    {"NtDeleteValueKey",                                   &monitor::GenericMonitor::on_NtDeleteValueKey},\
    {"NtDeviceIoControlFile",                              &monitor::GenericMonitor::on_NtDeviceIoControlFile},\
    {"NtDisplayString",                                    &monitor::GenericMonitor::on_NtDisplayString},\
    {"NtDrawText",                                         &monitor::GenericMonitor::on_NtDrawText},\
    {"NtDuplicateObject",                                  &monitor::GenericMonitor::on_NtDuplicateObject},\
    {"NtDuplicateToken",                                   &monitor::GenericMonitor::on_NtDuplicateToken},\
    {"NtEnumerateBootEntries",                             &monitor::GenericMonitor::on_NtEnumerateBootEntries},\
    {"NtEnumerateDriverEntries",                           &monitor::GenericMonitor::on_NtEnumerateDriverEntries},\
    {"NtEnumerateKey",                                     &monitor::GenericMonitor::on_NtEnumerateKey},\
    {"NtEnumerateSystemEnvironmentValuesEx",               &monitor::GenericMonitor::on_NtEnumerateSystemEnvironmentValuesEx},\
    {"NtEnumerateTransactionObject",                       &monitor::GenericMonitor::on_NtEnumerateTransactionObject},\
    {"NtEnumerateValueKey",                                &monitor::GenericMonitor::on_NtEnumerateValueKey},\
    {"NtExtendSection",                                    &monitor::GenericMonitor::on_NtExtendSection},\
    {"NtFilterToken",                                      &monitor::GenericMonitor::on_NtFilterToken},\
    {"NtFindAtom",                                         &monitor::GenericMonitor::on_NtFindAtom},\
    {"NtFlushBuffersFile",                                 &monitor::GenericMonitor::on_NtFlushBuffersFile},\
    {"NtFlushInstallUILanguage",                           &monitor::GenericMonitor::on_NtFlushInstallUILanguage},\
    {"NtFlushInstructionCache",                            &monitor::GenericMonitor::on_NtFlushInstructionCache},\
    {"NtFlushKey",                                         &monitor::GenericMonitor::on_NtFlushKey},\
    {"NtFlushVirtualMemory",                               &monitor::GenericMonitor::on_NtFlushVirtualMemory},\
    {"NtFreeUserPhysicalPages",                            &monitor::GenericMonitor::on_NtFreeUserPhysicalPages},\
    {"NtFreeVirtualMemory",                                &monitor::GenericMonitor::on_NtFreeVirtualMemory},\
    {"NtFreezeRegistry",                                   &monitor::GenericMonitor::on_NtFreezeRegistry},\
    {"NtFreezeTransactions",                               &monitor::GenericMonitor::on_NtFreezeTransactions},\
    {"NtFsControlFile",                                    &monitor::GenericMonitor::on_NtFsControlFile},\
    {"NtGetContextThread",                                 &monitor::GenericMonitor::on_NtGetContextThread},\
    {"NtGetDevicePowerState",                              &monitor::GenericMonitor::on_NtGetDevicePowerState},\
    {"NtGetMUIRegistryInfo",                               &monitor::GenericMonitor::on_NtGetMUIRegistryInfo},\
    {"NtGetNextProcess",                                   &monitor::GenericMonitor::on_NtGetNextProcess},\
    {"NtGetNextThread",                                    &monitor::GenericMonitor::on_NtGetNextThread},\
    {"NtGetNlsSectionPtr",                                 &monitor::GenericMonitor::on_NtGetNlsSectionPtr},\
    {"NtGetNotificationResourceManager",                   &monitor::GenericMonitor::on_NtGetNotificationResourceManager},\
    {"NtGetPlugPlayEvent",                                 &monitor::GenericMonitor::on_NtGetPlugPlayEvent},\
    {"NtGetWriteWatch",                                    &monitor::GenericMonitor::on_NtGetWriteWatch},\
    {"NtImpersonateAnonymousToken",                        &monitor::GenericMonitor::on_NtImpersonateAnonymousToken},\
    {"NtImpersonateClientOfPort",                          &monitor::GenericMonitor::on_NtImpersonateClientOfPort},\
    {"NtImpersonateThread",                                &monitor::GenericMonitor::on_NtImpersonateThread},\
    {"NtInitializeNlsFiles",                               &monitor::GenericMonitor::on_NtInitializeNlsFiles},\
    {"NtInitializeRegistry",                               &monitor::GenericMonitor::on_NtInitializeRegistry},\
    {"NtInitiatePowerAction",                              &monitor::GenericMonitor::on_NtInitiatePowerAction},\
    {"NtIsProcessInJob",                                   &monitor::GenericMonitor::on_NtIsProcessInJob},\
    {"NtListenPort",                                       &monitor::GenericMonitor::on_NtListenPort},\
    {"NtLoadDriver",                                       &monitor::GenericMonitor::on_NtLoadDriver},\
    {"NtLoadKey2",                                         &monitor::GenericMonitor::on_NtLoadKey2},\
    {"NtLoadKeyEx",                                        &monitor::GenericMonitor::on_NtLoadKeyEx},\
    {"NtLoadKey",                                          &monitor::GenericMonitor::on_NtLoadKey},\
    {"NtLockFile",                                         &monitor::GenericMonitor::on_NtLockFile},\
    {"NtLockProductActivationKeys",                        &monitor::GenericMonitor::on_NtLockProductActivationKeys},\
    {"NtLockRegistryKey",                                  &monitor::GenericMonitor::on_NtLockRegistryKey},\
    {"NtLockVirtualMemory",                                &monitor::GenericMonitor::on_NtLockVirtualMemory},\
    {"NtMakePermanentObject",                              &monitor::GenericMonitor::on_NtMakePermanentObject},\
    {"NtMakeTemporaryObject",                              &monitor::GenericMonitor::on_NtMakeTemporaryObject},\
    {"NtMapCMFModule",                                     &monitor::GenericMonitor::on_NtMapCMFModule},\
    {"NtMapUserPhysicalPages",                             &monitor::GenericMonitor::on_NtMapUserPhysicalPages},\
    {"NtMapUserPhysicalPagesScatter",                      &monitor::GenericMonitor::on_NtMapUserPhysicalPagesScatter},\
    {"NtMapViewOfSection",                                 &monitor::GenericMonitor::on_NtMapViewOfSection},\
    {"NtModifyBootEntry",                                  &monitor::GenericMonitor::on_NtModifyBootEntry},\
    {"NtModifyDriverEntry",                                &monitor::GenericMonitor::on_NtModifyDriverEntry},\
    {"NtNotifyChangeDirectoryFile",                        &monitor::GenericMonitor::on_NtNotifyChangeDirectoryFile},\
    {"NtNotifyChangeKey",                                  &monitor::GenericMonitor::on_NtNotifyChangeKey},\
    {"NtNotifyChangeMultipleKeys",                         &monitor::GenericMonitor::on_NtNotifyChangeMultipleKeys},\
    {"NtNotifyChangeSession",                              &monitor::GenericMonitor::on_NtNotifyChangeSession},\
    {"NtOpenDirectoryObject",                              &monitor::GenericMonitor::on_NtOpenDirectoryObject},\
    {"NtOpenEnlistment",                                   &monitor::GenericMonitor::on_NtOpenEnlistment},\
    {"NtOpenEvent",                                        &monitor::GenericMonitor::on_NtOpenEvent},\
    {"NtOpenEventPair",                                    &monitor::GenericMonitor::on_NtOpenEventPair},\
    {"NtOpenFile",                                         &monitor::GenericMonitor::on_NtOpenFile},\
    {"NtOpenIoCompletion",                                 &monitor::GenericMonitor::on_NtOpenIoCompletion},\
    {"NtOpenJobObject",                                    &monitor::GenericMonitor::on_NtOpenJobObject},\
    {"NtOpenKeyedEvent",                                   &monitor::GenericMonitor::on_NtOpenKeyedEvent},\
    {"NtOpenKeyEx",                                        &monitor::GenericMonitor::on_NtOpenKeyEx},\
    {"NtOpenKey",                                          &monitor::GenericMonitor::on_NtOpenKey},\
    {"NtOpenKeyTransactedEx",                              &monitor::GenericMonitor::on_NtOpenKeyTransactedEx},\
    {"NtOpenKeyTransacted",                                &monitor::GenericMonitor::on_NtOpenKeyTransacted},\
    {"NtOpenMutant",                                       &monitor::GenericMonitor::on_NtOpenMutant},\
    {"NtOpenObjectAuditAlarm",                             &monitor::GenericMonitor::on_NtOpenObjectAuditAlarm},\
    {"NtOpenPrivateNamespace",                             &monitor::GenericMonitor::on_NtOpenPrivateNamespace},\
    {"NtOpenProcess",                                      &monitor::GenericMonitor::on_NtOpenProcess},\
    {"NtOpenProcessTokenEx",                               &monitor::GenericMonitor::on_NtOpenProcessTokenEx},\
    {"NtOpenProcessToken",                                 &monitor::GenericMonitor::on_NtOpenProcessToken},\
    {"NtOpenResourceManager",                              &monitor::GenericMonitor::on_NtOpenResourceManager},\
    {"NtOpenSection",                                      &monitor::GenericMonitor::on_NtOpenSection},\
    {"NtOpenSemaphore",                                    &monitor::GenericMonitor::on_NtOpenSemaphore},\
    {"NtOpenSession",                                      &monitor::GenericMonitor::on_NtOpenSession},\
    {"NtOpenSymbolicLinkObject",                           &monitor::GenericMonitor::on_NtOpenSymbolicLinkObject},\
    {"NtOpenThread",                                       &monitor::GenericMonitor::on_NtOpenThread},\
    {"NtOpenThreadTokenEx",                                &monitor::GenericMonitor::on_NtOpenThreadTokenEx},\
    {"NtOpenThreadToken",                                  &monitor::GenericMonitor::on_NtOpenThreadToken},\
    {"NtOpenTimer",                                        &monitor::GenericMonitor::on_NtOpenTimer},\
    {"NtOpenTransactionManager",                           &monitor::GenericMonitor::on_NtOpenTransactionManager},\
    {"NtOpenTransaction",                                  &monitor::GenericMonitor::on_NtOpenTransaction},\
    {"NtPlugPlayControl",                                  &monitor::GenericMonitor::on_NtPlugPlayControl},\
    {"NtPowerInformation",                                 &monitor::GenericMonitor::on_NtPowerInformation},\
    {"NtPrepareComplete",                                  &monitor::GenericMonitor::on_NtPrepareComplete},\
    {"NtPrepareEnlistment",                                &monitor::GenericMonitor::on_NtPrepareEnlistment},\
    {"NtPrePrepareComplete",                               &monitor::GenericMonitor::on_NtPrePrepareComplete},\
    {"NtPrePrepareEnlistment",                             &monitor::GenericMonitor::on_NtPrePrepareEnlistment},\
    {"NtPrivilegeCheck",                                   &monitor::GenericMonitor::on_NtPrivilegeCheck},\
    {"NtPrivilegedServiceAuditAlarm",                      &monitor::GenericMonitor::on_NtPrivilegedServiceAuditAlarm},\
    {"NtPrivilegeObjectAuditAlarm",                        &monitor::GenericMonitor::on_NtPrivilegeObjectAuditAlarm},\
    {"NtPropagationComplete",                              &monitor::GenericMonitor::on_NtPropagationComplete},\
    {"NtPropagationFailed",                                &monitor::GenericMonitor::on_NtPropagationFailed},\
    {"NtProtectVirtualMemory",                             &monitor::GenericMonitor::on_NtProtectVirtualMemory},\
    {"NtPulseEvent",                                       &monitor::GenericMonitor::on_NtPulseEvent},\
    {"NtQueryAttributesFile",                              &monitor::GenericMonitor::on_NtQueryAttributesFile},\
    {"NtQueryBootEntryOrder",                              &monitor::GenericMonitor::on_NtQueryBootEntryOrder},\
    {"NtQueryBootOptions",                                 &monitor::GenericMonitor::on_NtQueryBootOptions},\
    {"NtQueryDebugFilterState",                            &monitor::GenericMonitor::on_NtQueryDebugFilterState},\
    {"NtQueryDefaultLocale",                               &monitor::GenericMonitor::on_NtQueryDefaultLocale},\
    {"NtQueryDefaultUILanguage",                           &monitor::GenericMonitor::on_NtQueryDefaultUILanguage},\
    {"NtQueryDirectoryFile",                               &monitor::GenericMonitor::on_NtQueryDirectoryFile},\
    {"NtQueryDirectoryObject",                             &monitor::GenericMonitor::on_NtQueryDirectoryObject},\
    {"NtQueryDriverEntryOrder",                            &monitor::GenericMonitor::on_NtQueryDriverEntryOrder},\
    {"NtQueryEaFile",                                      &monitor::GenericMonitor::on_NtQueryEaFile},\
    {"NtQueryEvent",                                       &monitor::GenericMonitor::on_NtQueryEvent},\
    {"NtQueryFullAttributesFile",                          &monitor::GenericMonitor::on_NtQueryFullAttributesFile},\
    {"NtQueryInformationAtom",                             &monitor::GenericMonitor::on_NtQueryInformationAtom},\
    {"NtQueryInformationEnlistment",                       &monitor::GenericMonitor::on_NtQueryInformationEnlistment},\
    {"NtQueryInformationFile",                             &monitor::GenericMonitor::on_NtQueryInformationFile},\
    {"NtQueryInformationJobObject",                        &monitor::GenericMonitor::on_NtQueryInformationJobObject},\
    {"NtQueryInformationPort",                             &monitor::GenericMonitor::on_NtQueryInformationPort},\
    {"NtQueryInformationProcess",                          &monitor::GenericMonitor::on_NtQueryInformationProcess},\
    {"NtQueryInformationResourceManager",                  &monitor::GenericMonitor::on_NtQueryInformationResourceManager},\
    {"NtQueryInformationThread",                           &monitor::GenericMonitor::on_NtQueryInformationThread},\
    {"NtQueryInformationToken",                            &monitor::GenericMonitor::on_NtQueryInformationToken},\
    {"NtQueryInformationTransaction",                      &monitor::GenericMonitor::on_NtQueryInformationTransaction},\
    {"NtQueryInformationTransactionManager",               &monitor::GenericMonitor::on_NtQueryInformationTransactionManager},\
    {"NtQueryInformationWorkerFactory",                    &monitor::GenericMonitor::on_NtQueryInformationWorkerFactory},\
    {"NtQueryInstallUILanguage",                           &monitor::GenericMonitor::on_NtQueryInstallUILanguage},\
    {"NtQueryIntervalProfile",                             &monitor::GenericMonitor::on_NtQueryIntervalProfile},\
    {"NtQueryIoCompletion",                                &monitor::GenericMonitor::on_NtQueryIoCompletion},\
    {"NtQueryKey",                                         &monitor::GenericMonitor::on_NtQueryKey},\
    {"NtQueryLicenseValue",                                &monitor::GenericMonitor::on_NtQueryLicenseValue},\
    {"NtQueryMultipleValueKey",                            &monitor::GenericMonitor::on_NtQueryMultipleValueKey},\
    {"NtQueryMutant",                                      &monitor::GenericMonitor::on_NtQueryMutant},\
    {"NtQueryObject",                                      &monitor::GenericMonitor::on_NtQueryObject},\
    {"NtQueryOpenSubKeysEx",                               &monitor::GenericMonitor::on_NtQueryOpenSubKeysEx},\
    {"NtQueryOpenSubKeys",                                 &monitor::GenericMonitor::on_NtQueryOpenSubKeys},\
    {"NtQueryPerformanceCounter",                          &monitor::GenericMonitor::on_NtQueryPerformanceCounter},\
    {"NtQueryQuotaInformationFile",                        &monitor::GenericMonitor::on_NtQueryQuotaInformationFile},\
    {"NtQuerySection",                                     &monitor::GenericMonitor::on_NtQuerySection},\
    {"NtQuerySecurityAttributesToken",                     &monitor::GenericMonitor::on_NtQuerySecurityAttributesToken},\
    {"NtQuerySecurityObject",                              &monitor::GenericMonitor::on_NtQuerySecurityObject},\
    {"NtQuerySemaphore",                                   &monitor::GenericMonitor::on_NtQuerySemaphore},\
    {"NtQuerySymbolicLinkObject",                          &monitor::GenericMonitor::on_NtQuerySymbolicLinkObject},\
    {"NtQuerySystemEnvironmentValueEx",                    &monitor::GenericMonitor::on_NtQuerySystemEnvironmentValueEx},\
    {"NtQuerySystemEnvironmentValue",                      &monitor::GenericMonitor::on_NtQuerySystemEnvironmentValue},\
    {"NtQuerySystemInformationEx",                         &monitor::GenericMonitor::on_NtQuerySystemInformationEx},\
    {"NtQuerySystemInformation",                           &monitor::GenericMonitor::on_NtQuerySystemInformation},\
    {"NtQuerySystemTime",                                  &monitor::GenericMonitor::on_NtQuerySystemTime},\
    {"NtQueryTimer",                                       &monitor::GenericMonitor::on_NtQueryTimer},\
    {"NtQueryTimerResolution",                             &monitor::GenericMonitor::on_NtQueryTimerResolution},\
    {"NtQueryValueKey",                                    &monitor::GenericMonitor::on_NtQueryValueKey},\
    {"NtQueryVirtualMemory",                               &monitor::GenericMonitor::on_NtQueryVirtualMemory},\
    {"NtQueryVolumeInformationFile",                       &monitor::GenericMonitor::on_NtQueryVolumeInformationFile},\
    {"NtQueueApcThreadEx",                                 &monitor::GenericMonitor::on_NtQueueApcThreadEx},\
    {"NtQueueApcThread",                                   &monitor::GenericMonitor::on_NtQueueApcThread},\
    {"NtRaiseException",                                   &monitor::GenericMonitor::on_NtRaiseException},\
    {"NtRaiseHardError",                                   &monitor::GenericMonitor::on_NtRaiseHardError},\
    {"NtReadFile",                                         &monitor::GenericMonitor::on_NtReadFile},\
    {"NtReadFileScatter",                                  &monitor::GenericMonitor::on_NtReadFileScatter},\
    {"NtReadOnlyEnlistment",                               &monitor::GenericMonitor::on_NtReadOnlyEnlistment},\
    {"NtReadRequestData",                                  &monitor::GenericMonitor::on_NtReadRequestData},\
    {"NtReadVirtualMemory",                                &monitor::GenericMonitor::on_NtReadVirtualMemory},\
    {"NtRecoverEnlistment",                                &monitor::GenericMonitor::on_NtRecoverEnlistment},\
    {"NtRecoverResourceManager",                           &monitor::GenericMonitor::on_NtRecoverResourceManager},\
    {"NtRecoverTransactionManager",                        &monitor::GenericMonitor::on_NtRecoverTransactionManager},\
    {"NtRegisterProtocolAddressInformation",               &monitor::GenericMonitor::on_NtRegisterProtocolAddressInformation},\
    {"NtRegisterThreadTerminatePort",                      &monitor::GenericMonitor::on_NtRegisterThreadTerminatePort},\
    {"NtReleaseKeyedEvent",                                &monitor::GenericMonitor::on_NtReleaseKeyedEvent},\
    {"NtReleaseMutant",                                    &monitor::GenericMonitor::on_NtReleaseMutant},\
    {"NtReleaseSemaphore",                                 &monitor::GenericMonitor::on_NtReleaseSemaphore},\
    {"NtReleaseWorkerFactoryWorker",                       &monitor::GenericMonitor::on_NtReleaseWorkerFactoryWorker},\
    {"NtRemoveIoCompletionEx",                             &monitor::GenericMonitor::on_NtRemoveIoCompletionEx},\
    {"NtRemoveIoCompletion",                               &monitor::GenericMonitor::on_NtRemoveIoCompletion},\
    {"NtRemoveProcessDebug",                               &monitor::GenericMonitor::on_NtRemoveProcessDebug},\
    {"NtRenameKey",                                        &monitor::GenericMonitor::on_NtRenameKey},\
    {"NtRenameTransactionManager",                         &monitor::GenericMonitor::on_NtRenameTransactionManager},\
    {"NtReplaceKey",                                       &monitor::GenericMonitor::on_NtReplaceKey},\
    {"NtReplacePartitionUnit",                             &monitor::GenericMonitor::on_NtReplacePartitionUnit},\
    {"NtReplyPort",                                        &monitor::GenericMonitor::on_NtReplyPort},\
    {"NtReplyWaitReceivePortEx",                           &monitor::GenericMonitor::on_NtReplyWaitReceivePortEx},\
    {"NtReplyWaitReceivePort",                             &monitor::GenericMonitor::on_NtReplyWaitReceivePort},\
    {"NtReplyWaitReplyPort",                               &monitor::GenericMonitor::on_NtReplyWaitReplyPort},\
    {"NtRequestPort",                                      &monitor::GenericMonitor::on_NtRequestPort},\
    {"NtRequestWaitReplyPort",                             &monitor::GenericMonitor::on_NtRequestWaitReplyPort},\
    {"NtResetEvent",                                       &monitor::GenericMonitor::on_NtResetEvent},\
    {"NtResetWriteWatch",                                  &monitor::GenericMonitor::on_NtResetWriteWatch},\
    {"NtRestoreKey",                                       &monitor::GenericMonitor::on_NtRestoreKey},\
    {"NtResumeProcess",                                    &monitor::GenericMonitor::on_NtResumeProcess},\
    {"NtResumeThread",                                     &monitor::GenericMonitor::on_NtResumeThread},\
    {"NtRollbackComplete",                                 &monitor::GenericMonitor::on_NtRollbackComplete},\
    {"NtRollbackEnlistment",                               &monitor::GenericMonitor::on_NtRollbackEnlistment},\
    {"NtRollbackTransaction",                              &monitor::GenericMonitor::on_NtRollbackTransaction},\
    {"NtRollforwardTransactionManager",                    &monitor::GenericMonitor::on_NtRollforwardTransactionManager},\
    {"NtSaveKeyEx",                                        &monitor::GenericMonitor::on_NtSaveKeyEx},\
    {"NtSaveKey",                                          &monitor::GenericMonitor::on_NtSaveKey},\
    {"NtSaveMergedKeys",                                   &monitor::GenericMonitor::on_NtSaveMergedKeys},\
    {"NtSecureConnectPort",                                &monitor::GenericMonitor::on_NtSecureConnectPort},\
    {"NtSetBootEntryOrder",                                &monitor::GenericMonitor::on_NtSetBootEntryOrder},\
    {"NtSetBootOptions",                                   &monitor::GenericMonitor::on_NtSetBootOptions},\
    {"NtSetContextThread",                                 &monitor::GenericMonitor::on_NtSetContextThread},\
    {"NtSetDebugFilterState",                              &monitor::GenericMonitor::on_NtSetDebugFilterState},\
    {"NtSetDefaultHardErrorPort",                          &monitor::GenericMonitor::on_NtSetDefaultHardErrorPort},\
    {"NtSetDefaultLocale",                                 &monitor::GenericMonitor::on_NtSetDefaultLocale},\
    {"NtSetDefaultUILanguage",                             &monitor::GenericMonitor::on_NtSetDefaultUILanguage},\
    {"NtSetDriverEntryOrder",                              &monitor::GenericMonitor::on_NtSetDriverEntryOrder},\
    {"NtSetEaFile",                                        &monitor::GenericMonitor::on_NtSetEaFile},\
    {"NtSetEventBoostPriority",                            &monitor::GenericMonitor::on_NtSetEventBoostPriority},\
    {"NtSetEvent",                                         &monitor::GenericMonitor::on_NtSetEvent},\
    {"NtSetHighEventPair",                                 &monitor::GenericMonitor::on_NtSetHighEventPair},\
    {"NtSetHighWaitLowEventPair",                          &monitor::GenericMonitor::on_NtSetHighWaitLowEventPair},\
    {"NtSetInformationDebugObject",                        &monitor::GenericMonitor::on_NtSetInformationDebugObject},\
    {"NtSetInformationEnlistment",                         &monitor::GenericMonitor::on_NtSetInformationEnlistment},\
    {"NtSetInformationFile",                               &monitor::GenericMonitor::on_NtSetInformationFile},\
    {"NtSetInformationJobObject",                          &monitor::GenericMonitor::on_NtSetInformationJobObject},\
    {"NtSetInformationKey",                                &monitor::GenericMonitor::on_NtSetInformationKey},\
    {"NtSetInformationObject",                             &monitor::GenericMonitor::on_NtSetInformationObject},\
    {"NtSetInformationProcess",                            &monitor::GenericMonitor::on_NtSetInformationProcess},\
    {"NtSetInformationResourceManager",                    &monitor::GenericMonitor::on_NtSetInformationResourceManager},\
    {"NtSetInformationThread",                             &monitor::GenericMonitor::on_NtSetInformationThread},\
    {"NtSetInformationToken",                              &monitor::GenericMonitor::on_NtSetInformationToken},\
    {"NtSetInformationTransaction",                        &monitor::GenericMonitor::on_NtSetInformationTransaction},\
    {"NtSetInformationTransactionManager",                 &monitor::GenericMonitor::on_NtSetInformationTransactionManager},\
    {"NtSetInformationWorkerFactory",                      &monitor::GenericMonitor::on_NtSetInformationWorkerFactory},\
    {"NtSetIntervalProfile",                               &monitor::GenericMonitor::on_NtSetIntervalProfile},\
    {"NtSetIoCompletionEx",                                &monitor::GenericMonitor::on_NtSetIoCompletionEx},\
    {"NtSetIoCompletion",                                  &monitor::GenericMonitor::on_NtSetIoCompletion},\
    {"NtSetLdtEntries",                                    &monitor::GenericMonitor::on_NtSetLdtEntries},\
    {"NtSetLowEventPair",                                  &monitor::GenericMonitor::on_NtSetLowEventPair},\
    {"NtSetLowWaitHighEventPair",                          &monitor::GenericMonitor::on_NtSetLowWaitHighEventPair},\
    {"NtSetQuotaInformationFile",                          &monitor::GenericMonitor::on_NtSetQuotaInformationFile},\
    {"NtSetSecurityObject",                                &monitor::GenericMonitor::on_NtSetSecurityObject},\
    {"NtSetSystemEnvironmentValueEx",                      &monitor::GenericMonitor::on_NtSetSystemEnvironmentValueEx},\
    {"NtSetSystemEnvironmentValue",                        &monitor::GenericMonitor::on_NtSetSystemEnvironmentValue},\
    {"NtSetSystemInformation",                             &monitor::GenericMonitor::on_NtSetSystemInformation},\
    {"NtSetSystemPowerState",                              &monitor::GenericMonitor::on_NtSetSystemPowerState},\
    {"NtSetSystemTime",                                    &monitor::GenericMonitor::on_NtSetSystemTime},\
    {"NtSetThreadExecutionState",                          &monitor::GenericMonitor::on_NtSetThreadExecutionState},\
    {"NtSetTimerEx",                                       &monitor::GenericMonitor::on_NtSetTimerEx},\
    {"NtSetTimer",                                         &monitor::GenericMonitor::on_NtSetTimer},\
    {"NtSetTimerResolution",                               &monitor::GenericMonitor::on_NtSetTimerResolution},\
    {"NtSetUuidSeed",                                      &monitor::GenericMonitor::on_NtSetUuidSeed},\
    {"NtSetValueKey",                                      &monitor::GenericMonitor::on_NtSetValueKey},\
    {"NtSetVolumeInformationFile",                         &monitor::GenericMonitor::on_NtSetVolumeInformationFile},\
    {"NtShutdownSystem",                                   &monitor::GenericMonitor::on_NtShutdownSystem},\
    {"NtShutdownWorkerFactory",                            &monitor::GenericMonitor::on_NtShutdownWorkerFactory},\
    {"NtSignalAndWaitForSingleObject",                     &monitor::GenericMonitor::on_NtSignalAndWaitForSingleObject},\
    {"NtSinglePhaseReject",                                &monitor::GenericMonitor::on_NtSinglePhaseReject},\
    {"NtStartProfile",                                     &monitor::GenericMonitor::on_NtStartProfile},\
    {"NtStopProfile",                                      &monitor::GenericMonitor::on_NtStopProfile},\
    {"NtSuspendProcess",                                   &monitor::GenericMonitor::on_NtSuspendProcess},\
    {"NtSuspendThread",                                    &monitor::GenericMonitor::on_NtSuspendThread},\
    {"NtSystemDebugControl",                               &monitor::GenericMonitor::on_NtSystemDebugControl},\
    {"NtTerminateJobObject",                               &monitor::GenericMonitor::on_NtTerminateJobObject},\
    {"NtTerminateProcess",                                 &monitor::GenericMonitor::on_NtTerminateProcess},\
    {"NtTerminateThread",                                  &monitor::GenericMonitor::on_NtTerminateThread},\
    {"NtTraceControl",                                     &monitor::GenericMonitor::on_NtTraceControl},\
    {"NtTraceEvent",                                       &monitor::GenericMonitor::on_NtTraceEvent},\
    {"NtTranslateFilePath",                                &monitor::GenericMonitor::on_NtTranslateFilePath},\
    {"NtUnloadDriver",                                     &monitor::GenericMonitor::on_NtUnloadDriver},\
    {"NtUnloadKey2",                                       &monitor::GenericMonitor::on_NtUnloadKey2},\
    {"NtUnloadKeyEx",                                      &monitor::GenericMonitor::on_NtUnloadKeyEx},\
    {"NtUnloadKey",                                        &monitor::GenericMonitor::on_NtUnloadKey},\
    {"NtUnlockFile",                                       &monitor::GenericMonitor::on_NtUnlockFile},\
    {"NtUnlockVirtualMemory",                              &monitor::GenericMonitor::on_NtUnlockVirtualMemory},\
    {"NtUnmapViewOfSection",                               &monitor::GenericMonitor::on_NtUnmapViewOfSection},\
    {"NtVdmControl",                                       &monitor::GenericMonitor::on_NtVdmControl},\
    {"NtWaitForDebugEvent",                                &monitor::GenericMonitor::on_NtWaitForDebugEvent},\
    {"NtWaitForKeyedEvent",                                &monitor::GenericMonitor::on_NtWaitForKeyedEvent},\
    {"NtWaitForMultipleObjects32",                         &monitor::GenericMonitor::on_NtWaitForMultipleObjects32},\
    {"NtWaitForMultipleObjects",                           &monitor::GenericMonitor::on_NtWaitForMultipleObjects},\
    {"NtWaitForSingleObject",                              &monitor::GenericMonitor::on_NtWaitForSingleObject},\
    {"NtWaitForWorkViaWorkerFactory",                      &monitor::GenericMonitor::on_NtWaitForWorkViaWorkerFactory},\
    {"NtWaitHighEventPair",                                &monitor::GenericMonitor::on_NtWaitHighEventPair},\
    {"NtWaitLowEventPair",                                 &monitor::GenericMonitor::on_NtWaitLowEventPair},\
    {"NtWorkerFactoryWorkerReady",                         &monitor::GenericMonitor::on_NtWorkerFactoryWorkerReady},\
    {"NtWriteFileGather",                                  &monitor::GenericMonitor::on_NtWriteFileGather},\
    {"NtWriteFile",                                        &monitor::GenericMonitor::on_NtWriteFile},\
    {"NtWriteRequestData",                                 &monitor::GenericMonitor::on_NtWriteRequestData},\
    {"NtWriteVirtualMemory",                               &monitor::GenericMonitor::on_NtWriteVirtualMemory},\
    {"NtDisableLastKnownGood",                             &monitor::GenericMonitor::on_NtDisableLastKnownGood},\
    {"NtEnableLastKnownGood",                              &monitor::GenericMonitor::on_NtEnableLastKnownGood},\
    {"NtFlushProcessWriteBuffers",                         &monitor::GenericMonitor::on_NtFlushProcessWriteBuffers},\
    {"NtFlushWriteBuffer",                                 &monitor::GenericMonitor::on_NtFlushWriteBuffer},\
    {"NtGetCurrentProcessorNumber",                        &monitor::GenericMonitor::on_NtGetCurrentProcessorNumber},\
    {"NtIsSystemResumeAutomatic",                          &monitor::GenericMonitor::on_NtIsSystemResumeAutomatic},\
    {"NtIsUILanguageComitted",                             &monitor::GenericMonitor::on_NtIsUILanguageComitted},\
    {"NtQueryPortInformationProcess",                      &monitor::GenericMonitor::on_NtQueryPortInformationProcess},\
    {"NtSerializeBoot",                                    &monitor::GenericMonitor::on_NtSerializeBoot},\
    {"NtTestAlert",                                        &monitor::GenericMonitor::on_NtTestAlert},\
    {"NtThawRegistry",                                     &monitor::GenericMonitor::on_NtThawRegistry},\
    {"NtThawTransactions",                                 &monitor::GenericMonitor::on_NtThawTransactions},\
    {"NtUmsThreadYield",                                   &monitor::GenericMonitor::on_NtUmsThreadYield},\
    {"NtYieldExecution",                                   &monitor::GenericMonitor::on_NtYieldExecution},\
    {"RtlpAllocateHeapInternal",                           &monitor::GenericMonitor::on_RtlpAllocateHeapInternal},\
    {"RtlFreeHeap",                                        &monitor::GenericMonitor::on_RtlFreeHeap},\
    {"RtlpReAllocateHeapInternal",                         &monitor::GenericMonitor::on_RtlpReAllocateHeapInternal},\
    {"RtlSizeHeap",                                        &monitor::GenericMonitor::on_RtlSizeHeap},\
    {"RtlSetUserValueHeap",                                &monitor::GenericMonitor::on_RtlSetUserValueHeap},\
    {"RtlGetUserInfoHeap",                                 &monitor::GenericMonitor::on_RtlGetUserInfoHeap},
