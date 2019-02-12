#include "syscalls.gen.hpp"

#define FDP_MODULE "syscalls"
#include "log.hpp"
#include "monitor.hpp"

namespace
{
	constexpr bool g_debug = true;
}

struct monitor::syscalls::Data
{
    Data(core::Core& core, const std::string& module);

    using Breakpoints = std::vector<core::Breakpoint>;
    core::Core& core;
    std::string module;
    Breakpoints breakpoints;

    std::vector<on_NtAcceptConnectPort_fn>                                observers_NtAcceptConnectPort;
    std::vector<on_NtAccessCheckAndAuditAlarm_fn>                         observers_NtAccessCheckAndAuditAlarm;
    std::vector<on_NtAccessCheckByTypeAndAuditAlarm_fn>                   observers_NtAccessCheckByTypeAndAuditAlarm;
    std::vector<on_NtAccessCheckByType_fn>                                observers_NtAccessCheckByType;
    std::vector<on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle_fn> observers_NtAccessCheckByTypeResultListAndAuditAlarmByHandle;
    std::vector<on_NtAccessCheckByTypeResultListAndAuditAlarm_fn>         observers_NtAccessCheckByTypeResultListAndAuditAlarm;
    std::vector<on_NtAccessCheckByTypeResultList_fn>                      observers_NtAccessCheckByTypeResultList;
    std::vector<on_NtAccessCheck_fn>                                      observers_NtAccessCheck;
    std::vector<on_NtAddAtom_fn>                                          observers_NtAddAtom;
    std::vector<on_NtAddBootEntry_fn>                                     observers_NtAddBootEntry;
    std::vector<on_NtAddDriverEntry_fn>                                   observers_NtAddDriverEntry;
    std::vector<on_NtAdjustGroupsToken_fn>                                observers_NtAdjustGroupsToken;
    std::vector<on_NtAdjustPrivilegesToken_fn>                            observers_NtAdjustPrivilegesToken;
    std::vector<on_NtAlertResumeThread_fn>                                observers_NtAlertResumeThread;
    std::vector<on_NtAlertThread_fn>                                      observers_NtAlertThread;
    std::vector<on_NtAllocateLocallyUniqueId_fn>                          observers_NtAllocateLocallyUniqueId;
    std::vector<on_NtAllocateReserveObject_fn>                            observers_NtAllocateReserveObject;
    std::vector<on_NtAllocateUserPhysicalPages_fn>                        observers_NtAllocateUserPhysicalPages;
    std::vector<on_NtAllocateUuids_fn>                                    observers_NtAllocateUuids;
    std::vector<on_NtAllocateVirtualMemory_fn>                            observers_NtAllocateVirtualMemory;
    std::vector<on_NtAlpcAcceptConnectPort_fn>                            observers_NtAlpcAcceptConnectPort;
    std::vector<on_NtAlpcCancelMessage_fn>                                observers_NtAlpcCancelMessage;
    std::vector<on_NtAlpcConnectPort_fn>                                  observers_NtAlpcConnectPort;
    std::vector<on_NtAlpcCreatePort_fn>                                   observers_NtAlpcCreatePort;
    std::vector<on_NtAlpcCreatePortSection_fn>                            observers_NtAlpcCreatePortSection;
    std::vector<on_NtAlpcCreateResourceReserve_fn>                        observers_NtAlpcCreateResourceReserve;
    std::vector<on_NtAlpcCreateSectionView_fn>                            observers_NtAlpcCreateSectionView;
    std::vector<on_NtAlpcCreateSecurityContext_fn>                        observers_NtAlpcCreateSecurityContext;
    std::vector<on_NtAlpcDeletePortSection_fn>                            observers_NtAlpcDeletePortSection;
    std::vector<on_NtAlpcDeleteResourceReserve_fn>                        observers_NtAlpcDeleteResourceReserve;
    std::vector<on_NtAlpcDeleteSectionView_fn>                            observers_NtAlpcDeleteSectionView;
    std::vector<on_NtAlpcDeleteSecurityContext_fn>                        observers_NtAlpcDeleteSecurityContext;
    std::vector<on_NtAlpcDisconnectPort_fn>                               observers_NtAlpcDisconnectPort;
    std::vector<on_NtAlpcImpersonateClientOfPort_fn>                      observers_NtAlpcImpersonateClientOfPort;
    std::vector<on_NtAlpcOpenSenderProcess_fn>                            observers_NtAlpcOpenSenderProcess;
    std::vector<on_NtAlpcOpenSenderThread_fn>                             observers_NtAlpcOpenSenderThread;
    std::vector<on_NtAlpcQueryInformation_fn>                             observers_NtAlpcQueryInformation;
    std::vector<on_NtAlpcQueryInformationMessage_fn>                      observers_NtAlpcQueryInformationMessage;
    std::vector<on_NtAlpcRevokeSecurityContext_fn>                        observers_NtAlpcRevokeSecurityContext;
    std::vector<on_NtAlpcSendWaitReceivePort_fn>                          observers_NtAlpcSendWaitReceivePort;
    std::vector<on_NtAlpcSetInformation_fn>                               observers_NtAlpcSetInformation;
    std::vector<on_NtApphelpCacheControl_fn>                              observers_NtApphelpCacheControl;
    std::vector<on_NtAreMappedFilesTheSame_fn>                            observers_NtAreMappedFilesTheSame;
    std::vector<on_NtAssignProcessToJobObject_fn>                         observers_NtAssignProcessToJobObject;
    std::vector<on_NtCallbackReturn_fn>                                   observers_NtCallbackReturn;
    std::vector<on_NtCancelIoFileEx_fn>                                   observers_NtCancelIoFileEx;
    std::vector<on_NtCancelIoFile_fn>                                     observers_NtCancelIoFile;
    std::vector<on_NtCancelSynchronousIoFile_fn>                          observers_NtCancelSynchronousIoFile;
    std::vector<on_NtCancelTimer_fn>                                      observers_NtCancelTimer;
    std::vector<on_NtClearEvent_fn>                                       observers_NtClearEvent;
    std::vector<on_NtClose_fn>                                            observers_NtClose;
    std::vector<on_NtCloseObjectAuditAlarm_fn>                            observers_NtCloseObjectAuditAlarm;
    std::vector<on_NtCommitComplete_fn>                                   observers_NtCommitComplete;
    std::vector<on_NtCommitEnlistment_fn>                                 observers_NtCommitEnlistment;
    std::vector<on_NtCommitTransaction_fn>                                observers_NtCommitTransaction;
    std::vector<on_NtCompactKeys_fn>                                      observers_NtCompactKeys;
    std::vector<on_NtCompareTokens_fn>                                    observers_NtCompareTokens;
    std::vector<on_NtCompleteConnectPort_fn>                              observers_NtCompleteConnectPort;
    std::vector<on_NtCompressKey_fn>                                      observers_NtCompressKey;
    std::vector<on_NtConnectPort_fn>                                      observers_NtConnectPort;
    std::vector<on_NtContinue_fn>                                         observers_NtContinue;
    std::vector<on_NtCreateDebugObject_fn>                                observers_NtCreateDebugObject;
    std::vector<on_NtCreateDirectoryObject_fn>                            observers_NtCreateDirectoryObject;
    std::vector<on_NtCreateEnlistment_fn>                                 observers_NtCreateEnlistment;
    std::vector<on_NtCreateEvent_fn>                                      observers_NtCreateEvent;
    std::vector<on_NtCreateEventPair_fn>                                  observers_NtCreateEventPair;
    std::vector<on_NtCreateFile_fn>                                       observers_NtCreateFile;
    std::vector<on_NtCreateIoCompletion_fn>                               observers_NtCreateIoCompletion;
    std::vector<on_NtCreateJobObject_fn>                                  observers_NtCreateJobObject;
    std::vector<on_NtCreateJobSet_fn>                                     observers_NtCreateJobSet;
    std::vector<on_NtCreateKeyedEvent_fn>                                 observers_NtCreateKeyedEvent;
    std::vector<on_NtCreateKey_fn>                                        observers_NtCreateKey;
    std::vector<on_NtCreateKeyTransacted_fn>                              observers_NtCreateKeyTransacted;
    std::vector<on_NtCreateMailslotFile_fn>                               observers_NtCreateMailslotFile;
    std::vector<on_NtCreateMutant_fn>                                     observers_NtCreateMutant;
    std::vector<on_NtCreateNamedPipeFile_fn>                              observers_NtCreateNamedPipeFile;
    std::vector<on_NtCreatePagingFile_fn>                                 observers_NtCreatePagingFile;
    std::vector<on_NtCreatePort_fn>                                       observers_NtCreatePort;
    std::vector<on_NtCreatePrivateNamespace_fn>                           observers_NtCreatePrivateNamespace;
    std::vector<on_NtCreateProcessEx_fn>                                  observers_NtCreateProcessEx;
    std::vector<on_NtCreateProcess_fn>                                    observers_NtCreateProcess;
    std::vector<on_NtCreateProfileEx_fn>                                  observers_NtCreateProfileEx;
    std::vector<on_NtCreateProfile_fn>                                    observers_NtCreateProfile;
    std::vector<on_NtCreateResourceManager_fn>                            observers_NtCreateResourceManager;
    std::vector<on_NtCreateSection_fn>                                    observers_NtCreateSection;
    std::vector<on_NtCreateSemaphore_fn>                                  observers_NtCreateSemaphore;
    std::vector<on_NtCreateSymbolicLinkObject_fn>                         observers_NtCreateSymbolicLinkObject;
    std::vector<on_NtCreateThreadEx_fn>                                   observers_NtCreateThreadEx;
    std::vector<on_NtCreateThread_fn>                                     observers_NtCreateThread;
    std::vector<on_NtCreateTimer_fn>                                      observers_NtCreateTimer;
    std::vector<on_NtCreateToken_fn>                                      observers_NtCreateToken;
    std::vector<on_NtCreateTransactionManager_fn>                         observers_NtCreateTransactionManager;
    std::vector<on_NtCreateTransaction_fn>                                observers_NtCreateTransaction;
    std::vector<on_NtCreateUserProcess_fn>                                observers_NtCreateUserProcess;
    std::vector<on_NtCreateWaitablePort_fn>                               observers_NtCreateWaitablePort;
    std::vector<on_NtCreateWorkerFactory_fn>                              observers_NtCreateWorkerFactory;
    std::vector<on_NtDebugActiveProcess_fn>                               observers_NtDebugActiveProcess;
    std::vector<on_NtDebugContinue_fn>                                    observers_NtDebugContinue;
    std::vector<on_NtDelayExecution_fn>                                   observers_NtDelayExecution;
    std::vector<on_NtDeleteAtom_fn>                                       observers_NtDeleteAtom;
    std::vector<on_NtDeleteBootEntry_fn>                                  observers_NtDeleteBootEntry;
    std::vector<on_NtDeleteDriverEntry_fn>                                observers_NtDeleteDriverEntry;
    std::vector<on_NtDeleteFile_fn>                                       observers_NtDeleteFile;
    std::vector<on_NtDeleteKey_fn>                                        observers_NtDeleteKey;
    std::vector<on_NtDeleteObjectAuditAlarm_fn>                           observers_NtDeleteObjectAuditAlarm;
    std::vector<on_NtDeletePrivateNamespace_fn>                           observers_NtDeletePrivateNamespace;
    std::vector<on_NtDeleteValueKey_fn>                                   observers_NtDeleteValueKey;
    std::vector<on_NtDeviceIoControlFile_fn>                              observers_NtDeviceIoControlFile;
    std::vector<on_NtDisplayString_fn>                                    observers_NtDisplayString;
    std::vector<on_NtDrawText_fn>                                         observers_NtDrawText;
    std::vector<on_NtDuplicateObject_fn>                                  observers_NtDuplicateObject;
    std::vector<on_NtDuplicateToken_fn>                                   observers_NtDuplicateToken;
    std::vector<on_NtEnumerateBootEntries_fn>                             observers_NtEnumerateBootEntries;
    std::vector<on_NtEnumerateDriverEntries_fn>                           observers_NtEnumerateDriverEntries;
    std::vector<on_NtEnumerateKey_fn>                                     observers_NtEnumerateKey;
    std::vector<on_NtEnumerateSystemEnvironmentValuesEx_fn>               observers_NtEnumerateSystemEnvironmentValuesEx;
    std::vector<on_NtEnumerateTransactionObject_fn>                       observers_NtEnumerateTransactionObject;
    std::vector<on_NtEnumerateValueKey_fn>                                observers_NtEnumerateValueKey;
    std::vector<on_NtExtendSection_fn>                                    observers_NtExtendSection;
    std::vector<on_NtFilterToken_fn>                                      observers_NtFilterToken;
    std::vector<on_NtFindAtom_fn>                                         observers_NtFindAtom;
    std::vector<on_NtFlushBuffersFile_fn>                                 observers_NtFlushBuffersFile;
    std::vector<on_NtFlushInstallUILanguage_fn>                           observers_NtFlushInstallUILanguage;
    std::vector<on_NtFlushInstructionCache_fn>                            observers_NtFlushInstructionCache;
    std::vector<on_NtFlushKey_fn>                                         observers_NtFlushKey;
    std::vector<on_NtFlushVirtualMemory_fn>                               observers_NtFlushVirtualMemory;
    std::vector<on_NtFreeUserPhysicalPages_fn>                            observers_NtFreeUserPhysicalPages;
    std::vector<on_NtFreeVirtualMemory_fn>                                observers_NtFreeVirtualMemory;
    std::vector<on_NtFreezeRegistry_fn>                                   observers_NtFreezeRegistry;
    std::vector<on_NtFreezeTransactions_fn>                               observers_NtFreezeTransactions;
    std::vector<on_NtFsControlFile_fn>                                    observers_NtFsControlFile;
    std::vector<on_NtGetContextThread_fn>                                 observers_NtGetContextThread;
    std::vector<on_NtGetDevicePowerState_fn>                              observers_NtGetDevicePowerState;
    std::vector<on_NtGetMUIRegistryInfo_fn>                               observers_NtGetMUIRegistryInfo;
    std::vector<on_NtGetNextProcess_fn>                                   observers_NtGetNextProcess;
    std::vector<on_NtGetNextThread_fn>                                    observers_NtGetNextThread;
    std::vector<on_NtGetNlsSectionPtr_fn>                                 observers_NtGetNlsSectionPtr;
    std::vector<on_NtGetNotificationResourceManager_fn>                   observers_NtGetNotificationResourceManager;
    std::vector<on_NtGetPlugPlayEvent_fn>                                 observers_NtGetPlugPlayEvent;
    std::vector<on_NtGetWriteWatch_fn>                                    observers_NtGetWriteWatch;
    std::vector<on_NtImpersonateAnonymousToken_fn>                        observers_NtImpersonateAnonymousToken;
    std::vector<on_NtImpersonateClientOfPort_fn>                          observers_NtImpersonateClientOfPort;
    std::vector<on_NtImpersonateThread_fn>                                observers_NtImpersonateThread;
    std::vector<on_NtInitializeNlsFiles_fn>                               observers_NtInitializeNlsFiles;
    std::vector<on_NtInitializeRegistry_fn>                               observers_NtInitializeRegistry;
    std::vector<on_NtInitiatePowerAction_fn>                              observers_NtInitiatePowerAction;
    std::vector<on_NtIsProcessInJob_fn>                                   observers_NtIsProcessInJob;
    std::vector<on_NtListenPort_fn>                                       observers_NtListenPort;
    std::vector<on_NtLoadDriver_fn>                                       observers_NtLoadDriver;
    std::vector<on_NtLoadKey2_fn>                                         observers_NtLoadKey2;
    std::vector<on_NtLoadKeyEx_fn>                                        observers_NtLoadKeyEx;
    std::vector<on_NtLoadKey_fn>                                          observers_NtLoadKey;
    std::vector<on_NtLockFile_fn>                                         observers_NtLockFile;
    std::vector<on_NtLockProductActivationKeys_fn>                        observers_NtLockProductActivationKeys;
    std::vector<on_NtLockRegistryKey_fn>                                  observers_NtLockRegistryKey;
    std::vector<on_NtLockVirtualMemory_fn>                                observers_NtLockVirtualMemory;
    std::vector<on_NtMakePermanentObject_fn>                              observers_NtMakePermanentObject;
    std::vector<on_NtMakeTemporaryObject_fn>                              observers_NtMakeTemporaryObject;
    std::vector<on_NtMapCMFModule_fn>                                     observers_NtMapCMFModule;
    std::vector<on_NtMapUserPhysicalPages_fn>                             observers_NtMapUserPhysicalPages;
    std::vector<on_NtMapUserPhysicalPagesScatter_fn>                      observers_NtMapUserPhysicalPagesScatter;
    std::vector<on_NtMapViewOfSection_fn>                                 observers_NtMapViewOfSection;
    std::vector<on_NtModifyBootEntry_fn>                                  observers_NtModifyBootEntry;
    std::vector<on_NtModifyDriverEntry_fn>                                observers_NtModifyDriverEntry;
    std::vector<on_NtNotifyChangeDirectoryFile_fn>                        observers_NtNotifyChangeDirectoryFile;
    std::vector<on_NtNotifyChangeKey_fn>                                  observers_NtNotifyChangeKey;
    std::vector<on_NtNotifyChangeMultipleKeys_fn>                         observers_NtNotifyChangeMultipleKeys;
    std::vector<on_NtNotifyChangeSession_fn>                              observers_NtNotifyChangeSession;
    std::vector<on_NtOpenDirectoryObject_fn>                              observers_NtOpenDirectoryObject;
    std::vector<on_NtOpenEnlistment_fn>                                   observers_NtOpenEnlistment;
    std::vector<on_NtOpenEvent_fn>                                        observers_NtOpenEvent;
    std::vector<on_NtOpenEventPair_fn>                                    observers_NtOpenEventPair;
    std::vector<on_NtOpenFile_fn>                                         observers_NtOpenFile;
    std::vector<on_NtOpenIoCompletion_fn>                                 observers_NtOpenIoCompletion;
    std::vector<on_NtOpenJobObject_fn>                                    observers_NtOpenJobObject;
    std::vector<on_NtOpenKeyedEvent_fn>                                   observers_NtOpenKeyedEvent;
    std::vector<on_NtOpenKeyEx_fn>                                        observers_NtOpenKeyEx;
    std::vector<on_NtOpenKey_fn>                                          observers_NtOpenKey;
    std::vector<on_NtOpenKeyTransactedEx_fn>                              observers_NtOpenKeyTransactedEx;
    std::vector<on_NtOpenKeyTransacted_fn>                                observers_NtOpenKeyTransacted;
    std::vector<on_NtOpenMutant_fn>                                       observers_NtOpenMutant;
    std::vector<on_NtOpenObjectAuditAlarm_fn>                             observers_NtOpenObjectAuditAlarm;
    std::vector<on_NtOpenPrivateNamespace_fn>                             observers_NtOpenPrivateNamespace;
    std::vector<on_NtOpenProcess_fn>                                      observers_NtOpenProcess;
    std::vector<on_NtOpenProcessTokenEx_fn>                               observers_NtOpenProcessTokenEx;
    std::vector<on_NtOpenProcessToken_fn>                                 observers_NtOpenProcessToken;
    std::vector<on_NtOpenResourceManager_fn>                              observers_NtOpenResourceManager;
    std::vector<on_NtOpenSection_fn>                                      observers_NtOpenSection;
    std::vector<on_NtOpenSemaphore_fn>                                    observers_NtOpenSemaphore;
    std::vector<on_NtOpenSession_fn>                                      observers_NtOpenSession;
    std::vector<on_NtOpenSymbolicLinkObject_fn>                           observers_NtOpenSymbolicLinkObject;
    std::vector<on_NtOpenThread_fn>                                       observers_NtOpenThread;
    std::vector<on_NtOpenThreadTokenEx_fn>                                observers_NtOpenThreadTokenEx;
    std::vector<on_NtOpenThreadToken_fn>                                  observers_NtOpenThreadToken;
    std::vector<on_NtOpenTimer_fn>                                        observers_NtOpenTimer;
    std::vector<on_NtOpenTransactionManager_fn>                           observers_NtOpenTransactionManager;
    std::vector<on_NtOpenTransaction_fn>                                  observers_NtOpenTransaction;
    std::vector<on_NtPlugPlayControl_fn>                                  observers_NtPlugPlayControl;
    std::vector<on_NtPowerInformation_fn>                                 observers_NtPowerInformation;
    std::vector<on_NtPrepareComplete_fn>                                  observers_NtPrepareComplete;
    std::vector<on_NtPrepareEnlistment_fn>                                observers_NtPrepareEnlistment;
    std::vector<on_NtPrePrepareComplete_fn>                               observers_NtPrePrepareComplete;
    std::vector<on_NtPrePrepareEnlistment_fn>                             observers_NtPrePrepareEnlistment;
    std::vector<on_NtPrivilegeCheck_fn>                                   observers_NtPrivilegeCheck;
    std::vector<on_NtPrivilegedServiceAuditAlarm_fn>                      observers_NtPrivilegedServiceAuditAlarm;
    std::vector<on_NtPrivilegeObjectAuditAlarm_fn>                        observers_NtPrivilegeObjectAuditAlarm;
    std::vector<on_NtPropagationComplete_fn>                              observers_NtPropagationComplete;
    std::vector<on_NtPropagationFailed_fn>                                observers_NtPropagationFailed;
    std::vector<on_NtProtectVirtualMemory_fn>                             observers_NtProtectVirtualMemory;
    std::vector<on_NtPulseEvent_fn>                                       observers_NtPulseEvent;
    std::vector<on_NtQueryAttributesFile_fn>                              observers_NtQueryAttributesFile;
    std::vector<on_NtQueryBootEntryOrder_fn>                              observers_NtQueryBootEntryOrder;
    std::vector<on_NtQueryBootOptions_fn>                                 observers_NtQueryBootOptions;
    std::vector<on_NtQueryDebugFilterState_fn>                            observers_NtQueryDebugFilterState;
    std::vector<on_NtQueryDefaultLocale_fn>                               observers_NtQueryDefaultLocale;
    std::vector<on_NtQueryDefaultUILanguage_fn>                           observers_NtQueryDefaultUILanguage;
    std::vector<on_NtQueryDirectoryFile_fn>                               observers_NtQueryDirectoryFile;
    std::vector<on_NtQueryDirectoryObject_fn>                             observers_NtQueryDirectoryObject;
    std::vector<on_NtQueryDriverEntryOrder_fn>                            observers_NtQueryDriverEntryOrder;
    std::vector<on_NtQueryEaFile_fn>                                      observers_NtQueryEaFile;
    std::vector<on_NtQueryEvent_fn>                                       observers_NtQueryEvent;
    std::vector<on_NtQueryFullAttributesFile_fn>                          observers_NtQueryFullAttributesFile;
    std::vector<on_NtQueryInformationAtom_fn>                             observers_NtQueryInformationAtom;
    std::vector<on_NtQueryInformationEnlistment_fn>                       observers_NtQueryInformationEnlistment;
    std::vector<on_NtQueryInformationFile_fn>                             observers_NtQueryInformationFile;
    std::vector<on_NtQueryInformationJobObject_fn>                        observers_NtQueryInformationJobObject;
    std::vector<on_NtQueryInformationPort_fn>                             observers_NtQueryInformationPort;
    std::vector<on_NtQueryInformationProcess_fn>                          observers_NtQueryInformationProcess;
    std::vector<on_NtQueryInformationResourceManager_fn>                  observers_NtQueryInformationResourceManager;
    std::vector<on_NtQueryInformationThread_fn>                           observers_NtQueryInformationThread;
    std::vector<on_NtQueryInformationToken_fn>                            observers_NtQueryInformationToken;
    std::vector<on_NtQueryInformationTransaction_fn>                      observers_NtQueryInformationTransaction;
    std::vector<on_NtQueryInformationTransactionManager_fn>               observers_NtQueryInformationTransactionManager;
    std::vector<on_NtQueryInformationWorkerFactory_fn>                    observers_NtQueryInformationWorkerFactory;
    std::vector<on_NtQueryInstallUILanguage_fn>                           observers_NtQueryInstallUILanguage;
    std::vector<on_NtQueryIntervalProfile_fn>                             observers_NtQueryIntervalProfile;
    std::vector<on_NtQueryIoCompletion_fn>                                observers_NtQueryIoCompletion;
    std::vector<on_NtQueryKey_fn>                                         observers_NtQueryKey;
    std::vector<on_NtQueryLicenseValue_fn>                                observers_NtQueryLicenseValue;
    std::vector<on_NtQueryMultipleValueKey_fn>                            observers_NtQueryMultipleValueKey;
    std::vector<on_NtQueryMutant_fn>                                      observers_NtQueryMutant;
    std::vector<on_NtQueryObject_fn>                                      observers_NtQueryObject;
    std::vector<on_NtQueryOpenSubKeysEx_fn>                               observers_NtQueryOpenSubKeysEx;
    std::vector<on_NtQueryOpenSubKeys_fn>                                 observers_NtQueryOpenSubKeys;
    std::vector<on_NtQueryPerformanceCounter_fn>                          observers_NtQueryPerformanceCounter;
    std::vector<on_NtQueryQuotaInformationFile_fn>                        observers_NtQueryQuotaInformationFile;
    std::vector<on_NtQuerySection_fn>                                     observers_NtQuerySection;
    std::vector<on_NtQuerySecurityAttributesToken_fn>                     observers_NtQuerySecurityAttributesToken;
    std::vector<on_NtQuerySecurityObject_fn>                              observers_NtQuerySecurityObject;
    std::vector<on_NtQuerySemaphore_fn>                                   observers_NtQuerySemaphore;
    std::vector<on_NtQuerySymbolicLinkObject_fn>                          observers_NtQuerySymbolicLinkObject;
    std::vector<on_NtQuerySystemEnvironmentValueEx_fn>                    observers_NtQuerySystemEnvironmentValueEx;
    std::vector<on_NtQuerySystemEnvironmentValue_fn>                      observers_NtQuerySystemEnvironmentValue;
    std::vector<on_NtQuerySystemInformationEx_fn>                         observers_NtQuerySystemInformationEx;
    std::vector<on_NtQuerySystemInformation_fn>                           observers_NtQuerySystemInformation;
    std::vector<on_NtQuerySystemTime_fn>                                  observers_NtQuerySystemTime;
    std::vector<on_NtQueryTimer_fn>                                       observers_NtQueryTimer;
    std::vector<on_NtQueryTimerResolution_fn>                             observers_NtQueryTimerResolution;
    std::vector<on_NtQueryValueKey_fn>                                    observers_NtQueryValueKey;
    std::vector<on_NtQueryVirtualMemory_fn>                               observers_NtQueryVirtualMemory;
    std::vector<on_NtQueryVolumeInformationFile_fn>                       observers_NtQueryVolumeInformationFile;
    std::vector<on_NtQueueApcThreadEx_fn>                                 observers_NtQueueApcThreadEx;
    std::vector<on_NtQueueApcThread_fn>                                   observers_NtQueueApcThread;
    std::vector<on_NtRaiseException_fn>                                   observers_NtRaiseException;
    std::vector<on_NtRaiseHardError_fn>                                   observers_NtRaiseHardError;
    std::vector<on_NtReadFile_fn>                                         observers_NtReadFile;
    std::vector<on_NtReadFileScatter_fn>                                  observers_NtReadFileScatter;
    std::vector<on_NtReadOnlyEnlistment_fn>                               observers_NtReadOnlyEnlistment;
    std::vector<on_NtReadRequestData_fn>                                  observers_NtReadRequestData;
    std::vector<on_NtReadVirtualMemory_fn>                                observers_NtReadVirtualMemory;
    std::vector<on_NtRecoverEnlistment_fn>                                observers_NtRecoverEnlistment;
    std::vector<on_NtRecoverResourceManager_fn>                           observers_NtRecoverResourceManager;
    std::vector<on_NtRecoverTransactionManager_fn>                        observers_NtRecoverTransactionManager;
    std::vector<on_NtRegisterProtocolAddressInformation_fn>               observers_NtRegisterProtocolAddressInformation;
    std::vector<on_NtRegisterThreadTerminatePort_fn>                      observers_NtRegisterThreadTerminatePort;
    std::vector<on_NtReleaseKeyedEvent_fn>                                observers_NtReleaseKeyedEvent;
    std::vector<on_NtReleaseMutant_fn>                                    observers_NtReleaseMutant;
    std::vector<on_NtReleaseSemaphore_fn>                                 observers_NtReleaseSemaphore;
    std::vector<on_NtReleaseWorkerFactoryWorker_fn>                       observers_NtReleaseWorkerFactoryWorker;
    std::vector<on_NtRemoveIoCompletionEx_fn>                             observers_NtRemoveIoCompletionEx;
    std::vector<on_NtRemoveIoCompletion_fn>                               observers_NtRemoveIoCompletion;
    std::vector<on_NtRemoveProcessDebug_fn>                               observers_NtRemoveProcessDebug;
    std::vector<on_NtRenameKey_fn>                                        observers_NtRenameKey;
    std::vector<on_NtRenameTransactionManager_fn>                         observers_NtRenameTransactionManager;
    std::vector<on_NtReplaceKey_fn>                                       observers_NtReplaceKey;
    std::vector<on_NtReplacePartitionUnit_fn>                             observers_NtReplacePartitionUnit;
    std::vector<on_NtReplyPort_fn>                                        observers_NtReplyPort;
    std::vector<on_NtReplyWaitReceivePortEx_fn>                           observers_NtReplyWaitReceivePortEx;
    std::vector<on_NtReplyWaitReceivePort_fn>                             observers_NtReplyWaitReceivePort;
    std::vector<on_NtReplyWaitReplyPort_fn>                               observers_NtReplyWaitReplyPort;
    std::vector<on_NtRequestPort_fn>                                      observers_NtRequestPort;
    std::vector<on_NtRequestWaitReplyPort_fn>                             observers_NtRequestWaitReplyPort;
    std::vector<on_NtResetEvent_fn>                                       observers_NtResetEvent;
    std::vector<on_NtResetWriteWatch_fn>                                  observers_NtResetWriteWatch;
    std::vector<on_NtRestoreKey_fn>                                       observers_NtRestoreKey;
    std::vector<on_NtResumeProcess_fn>                                    observers_NtResumeProcess;
    std::vector<on_NtResumeThread_fn>                                     observers_NtResumeThread;
    std::vector<on_NtRollbackComplete_fn>                                 observers_NtRollbackComplete;
    std::vector<on_NtRollbackEnlistment_fn>                               observers_NtRollbackEnlistment;
    std::vector<on_NtRollbackTransaction_fn>                              observers_NtRollbackTransaction;
    std::vector<on_NtRollforwardTransactionManager_fn>                    observers_NtRollforwardTransactionManager;
    std::vector<on_NtSaveKeyEx_fn>                                        observers_NtSaveKeyEx;
    std::vector<on_NtSaveKey_fn>                                          observers_NtSaveKey;
    std::vector<on_NtSaveMergedKeys_fn>                                   observers_NtSaveMergedKeys;
    std::vector<on_NtSecureConnectPort_fn>                                observers_NtSecureConnectPort;
    std::vector<on_NtSetBootEntryOrder_fn>                                observers_NtSetBootEntryOrder;
    std::vector<on_NtSetBootOptions_fn>                                   observers_NtSetBootOptions;
    std::vector<on_NtSetContextThread_fn>                                 observers_NtSetContextThread;
    std::vector<on_NtSetDebugFilterState_fn>                              observers_NtSetDebugFilterState;
    std::vector<on_NtSetDefaultHardErrorPort_fn>                          observers_NtSetDefaultHardErrorPort;
    std::vector<on_NtSetDefaultLocale_fn>                                 observers_NtSetDefaultLocale;
    std::vector<on_NtSetDefaultUILanguage_fn>                             observers_NtSetDefaultUILanguage;
    std::vector<on_NtSetDriverEntryOrder_fn>                              observers_NtSetDriverEntryOrder;
    std::vector<on_NtSetEaFile_fn>                                        observers_NtSetEaFile;
    std::vector<on_NtSetEventBoostPriority_fn>                            observers_NtSetEventBoostPriority;
    std::vector<on_NtSetEvent_fn>                                         observers_NtSetEvent;
    std::vector<on_NtSetHighEventPair_fn>                                 observers_NtSetHighEventPair;
    std::vector<on_NtSetHighWaitLowEventPair_fn>                          observers_NtSetHighWaitLowEventPair;
    std::vector<on_NtSetInformationDebugObject_fn>                        observers_NtSetInformationDebugObject;
    std::vector<on_NtSetInformationEnlistment_fn>                         observers_NtSetInformationEnlistment;
    std::vector<on_NtSetInformationFile_fn>                               observers_NtSetInformationFile;
    std::vector<on_NtSetInformationJobObject_fn>                          observers_NtSetInformationJobObject;
    std::vector<on_NtSetInformationKey_fn>                                observers_NtSetInformationKey;
    std::vector<on_NtSetInformationObject_fn>                             observers_NtSetInformationObject;
    std::vector<on_NtSetInformationProcess_fn>                            observers_NtSetInformationProcess;
    std::vector<on_NtSetInformationResourceManager_fn>                    observers_NtSetInformationResourceManager;
    std::vector<on_NtSetInformationThread_fn>                             observers_NtSetInformationThread;
    std::vector<on_NtSetInformationToken_fn>                              observers_NtSetInformationToken;
    std::vector<on_NtSetInformationTransaction_fn>                        observers_NtSetInformationTransaction;
    std::vector<on_NtSetInformationTransactionManager_fn>                 observers_NtSetInformationTransactionManager;
    std::vector<on_NtSetInformationWorkerFactory_fn>                      observers_NtSetInformationWorkerFactory;
    std::vector<on_NtSetIntervalProfile_fn>                               observers_NtSetIntervalProfile;
    std::vector<on_NtSetIoCompletionEx_fn>                                observers_NtSetIoCompletionEx;
    std::vector<on_NtSetIoCompletion_fn>                                  observers_NtSetIoCompletion;
    std::vector<on_NtSetLdtEntries_fn>                                    observers_NtSetLdtEntries;
    std::vector<on_NtSetLowEventPair_fn>                                  observers_NtSetLowEventPair;
    std::vector<on_NtSetLowWaitHighEventPair_fn>                          observers_NtSetLowWaitHighEventPair;
    std::vector<on_NtSetQuotaInformationFile_fn>                          observers_NtSetQuotaInformationFile;
    std::vector<on_NtSetSecurityObject_fn>                                observers_NtSetSecurityObject;
    std::vector<on_NtSetSystemEnvironmentValueEx_fn>                      observers_NtSetSystemEnvironmentValueEx;
    std::vector<on_NtSetSystemEnvironmentValue_fn>                        observers_NtSetSystemEnvironmentValue;
    std::vector<on_NtSetSystemInformation_fn>                             observers_NtSetSystemInformation;
    std::vector<on_NtSetSystemPowerState_fn>                              observers_NtSetSystemPowerState;
    std::vector<on_NtSetSystemTime_fn>                                    observers_NtSetSystemTime;
    std::vector<on_NtSetThreadExecutionState_fn>                          observers_NtSetThreadExecutionState;
    std::vector<on_NtSetTimerEx_fn>                                       observers_NtSetTimerEx;
    std::vector<on_NtSetTimer_fn>                                         observers_NtSetTimer;
    std::vector<on_NtSetTimerResolution_fn>                               observers_NtSetTimerResolution;
    std::vector<on_NtSetUuidSeed_fn>                                      observers_NtSetUuidSeed;
    std::vector<on_NtSetValueKey_fn>                                      observers_NtSetValueKey;
    std::vector<on_NtSetVolumeInformationFile_fn>                         observers_NtSetVolumeInformationFile;
    std::vector<on_NtShutdownSystem_fn>                                   observers_NtShutdownSystem;
    std::vector<on_NtShutdownWorkerFactory_fn>                            observers_NtShutdownWorkerFactory;
    std::vector<on_NtSignalAndWaitForSingleObject_fn>                     observers_NtSignalAndWaitForSingleObject;
    std::vector<on_NtSinglePhaseReject_fn>                                observers_NtSinglePhaseReject;
    std::vector<on_NtStartProfile_fn>                                     observers_NtStartProfile;
    std::vector<on_NtStopProfile_fn>                                      observers_NtStopProfile;
    std::vector<on_NtSuspendProcess_fn>                                   observers_NtSuspendProcess;
    std::vector<on_NtSuspendThread_fn>                                    observers_NtSuspendThread;
    std::vector<on_NtSystemDebugControl_fn>                               observers_NtSystemDebugControl;
    std::vector<on_NtTerminateJobObject_fn>                               observers_NtTerminateJobObject;
    std::vector<on_NtTerminateProcess_fn>                                 observers_NtTerminateProcess;
    std::vector<on_NtTerminateThread_fn>                                  observers_NtTerminateThread;
    std::vector<on_NtTraceControl_fn>                                     observers_NtTraceControl;
    std::vector<on_NtTraceEvent_fn>                                       observers_NtTraceEvent;
    std::vector<on_NtTranslateFilePath_fn>                                observers_NtTranslateFilePath;
    std::vector<on_NtUnloadDriver_fn>                                     observers_NtUnloadDriver;
    std::vector<on_NtUnloadKey2_fn>                                       observers_NtUnloadKey2;
    std::vector<on_NtUnloadKeyEx_fn>                                      observers_NtUnloadKeyEx;
    std::vector<on_NtUnloadKey_fn>                                        observers_NtUnloadKey;
    std::vector<on_NtUnlockFile_fn>                                       observers_NtUnlockFile;
    std::vector<on_NtUnlockVirtualMemory_fn>                              observers_NtUnlockVirtualMemory;
    std::vector<on_NtUnmapViewOfSection_fn>                               observers_NtUnmapViewOfSection;
    std::vector<on_NtVdmControl_fn>                                       observers_NtVdmControl;
    std::vector<on_NtWaitForDebugEvent_fn>                                observers_NtWaitForDebugEvent;
    std::vector<on_NtWaitForKeyedEvent_fn>                                observers_NtWaitForKeyedEvent;
    std::vector<on_NtWaitForMultipleObjects32_fn>                         observers_NtWaitForMultipleObjects32;
    std::vector<on_NtWaitForMultipleObjects_fn>                           observers_NtWaitForMultipleObjects;
    std::vector<on_NtWaitForSingleObject_fn>                              observers_NtWaitForSingleObject;
    std::vector<on_NtWaitForWorkViaWorkerFactory_fn>                      observers_NtWaitForWorkViaWorkerFactory;
    std::vector<on_NtWaitHighEventPair_fn>                                observers_NtWaitHighEventPair;
    std::vector<on_NtWaitLowEventPair_fn>                                 observers_NtWaitLowEventPair;
    std::vector<on_NtWorkerFactoryWorkerReady_fn>                         observers_NtWorkerFactoryWorkerReady;
    std::vector<on_NtWriteFileGather_fn>                                  observers_NtWriteFileGather;
    std::vector<on_NtWriteFile_fn>                                        observers_NtWriteFile;
    std::vector<on_NtWriteRequestData_fn>                                 observers_NtWriteRequestData;
    std::vector<on_NtWriteVirtualMemory_fn>                               observers_NtWriteVirtualMemory;
    std::vector<on_NtDisableLastKnownGood_fn>                             observers_NtDisableLastKnownGood;
    std::vector<on_NtEnableLastKnownGood_fn>                              observers_NtEnableLastKnownGood;
    std::vector<on_NtFlushProcessWriteBuffers_fn>                         observers_NtFlushProcessWriteBuffers;
    std::vector<on_NtFlushWriteBuffer_fn>                                 observers_NtFlushWriteBuffer;
    std::vector<on_NtGetCurrentProcessorNumber_fn>                        observers_NtGetCurrentProcessorNumber;
    std::vector<on_NtIsSystemResumeAutomatic_fn>                          observers_NtIsSystemResumeAutomatic;
    std::vector<on_NtIsUILanguageComitted_fn>                             observers_NtIsUILanguageComitted;
    std::vector<on_NtQueryPortInformationProcess_fn>                      observers_NtQueryPortInformationProcess;
    std::vector<on_NtSerializeBoot_fn>                                    observers_NtSerializeBoot;
    std::vector<on_NtTestAlert_fn>                                        observers_NtTestAlert;
    std::vector<on_NtThawRegistry_fn>                                     observers_NtThawRegistry;
    std::vector<on_NtThawTransactions_fn>                                 observers_NtThawTransactions;
    std::vector<on_NtUmsThreadYield_fn>                                   observers_NtUmsThreadYield;
    std::vector<on_NtYieldExecution_fn>                                   observers_NtYieldExecution;
    std::vector<on_RtlpAllocateHeapInternal_fn>                           observers_RtlpAllocateHeapInternal;
    std::vector<on_RtlFreeHeap_fn>                                        observers_RtlFreeHeap;
    std::vector<on_RtlpReAllocateHeapInternal_fn>                         observers_RtlpReAllocateHeapInternal;
    std::vector<on_RtlSizeHeap_fn>                                        observers_RtlSizeHeap;
    std::vector<on_RtlSetUserValueHeap_fn>                                observers_RtlSetUserValueHeap;
    std::vector<on_RtlGetUserInfoHeap_fn>                                 observers_RtlGetUserInfoHeap;
};

monitor::syscalls::Data::Data(core::Core& core, const std::string& module)
    : core(core)
    , module(module)
{
}

monitor::syscalls::syscalls(core::Core& core, const std::string& module)
    : d_(std::make_unique<Data>(core, module))
{
}

monitor::syscalls::~syscalls()
{
}

namespace
{
    using Data = monitor::syscalls::Data;

    static core::Breakpoint register_callback(Data& d, proc_t proc, const char* name, const monitor::syscalls::on_call_fn& on_call)
    {
        const auto addr = d.core.sym.symbol(d.module, name);
        if(!addr)
            FAIL(nullptr, "unable to find symbole {}!{}", d.module.data(), name);

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
        const auto arg = monitor::get_arg_by_index(core, i);
        if(!arg)
            return {};

        return nt::cast_to<T>(*arg);
    }

    static void on_NtAcceptConnectPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle        = arg<nt::PHANDLE>(d.core, 0);
        const auto PortContext       = arg<nt::PVOID>(d.core, 1);
        const auto ConnectionRequest = arg<nt::PPORT_MESSAGE>(d.core, 2);
        const auto AcceptConnection  = arg<nt::BOOLEAN>(d.core, 3);
        const auto ServerView        = arg<nt::PPORT_VIEW>(d.core, 4);
        const auto ClientView        = arg<nt::PREMOTE_PORT_VIEW>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtAcceptConnectPort(PortHandle:{:#x}, PortContext:{:#x}, ConnectionRequest:{:#x}, AcceptConnection:{:#x}, ServerView:{:#x}, ClientView:{:#x})", PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView);

        for(const auto& it : d.observers_NtAcceptConnectPort)
            it(PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView);
    }

    static void on_NtAccessCheckAndAuditAlarm(monitor::syscalls::Data& d)
    {
        const auto SubsystemName      = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto HandleId           = arg<nt::PVOID>(d.core, 1);
        const auto ObjectTypeName     = arg<nt::PUNICODE_STRING>(d.core, 2);
        const auto ObjectName         = arg<nt::PUNICODE_STRING>(d.core, 3);
        const auto SecurityDescriptor = arg<nt::PSECURITY_DESCRIPTOR>(d.core, 4);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(d.core, 5);
        const auto GenericMapping     = arg<nt::PGENERIC_MAPPING>(d.core, 6);
        const auto ObjectCreation     = arg<nt::BOOLEAN>(d.core, 7);
        const auto GrantedAccess      = arg<nt::PACCESS_MASK>(d.core, 8);
        const auto AccessStatus       = arg<nt::PNTSTATUS>(d.core, 9);
        const auto GenerateOnClose    = arg<nt::PBOOLEAN>(d.core, 10);

        if constexpr(g_debug)
            LOG(INFO, "NtAccessCheckAndAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, ObjectTypeName:{:#x}, ObjectName:{:#x}, SecurityDescriptor:{:#x}, DesiredAccess:{:#x}, GenericMapping:{:#x}, ObjectCreation:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);

        for(const auto& it : d.observers_NtAccessCheckAndAuditAlarm)
            it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }

    static void on_NtAccessCheckByTypeAndAuditAlarm(monitor::syscalls::Data& d)
    {
        const auto SubsystemName        = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto HandleId             = arg<nt::PVOID>(d.core, 1);
        const auto ObjectTypeName       = arg<nt::PUNICODE_STRING>(d.core, 2);
        const auto ObjectName           = arg<nt::PUNICODE_STRING>(d.core, 3);
        const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(d.core, 4);
        const auto PrincipalSelfSid     = arg<nt::PSID>(d.core, 5);
        const auto DesiredAccess        = arg<nt::ACCESS_MASK>(d.core, 6);
        const auto AuditType            = arg<nt::AUDIT_EVENT_TYPE>(d.core, 7);
        const auto Flags                = arg<nt::ULONG>(d.core, 8);
        const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(d.core, 9);
        const auto ObjectTypeListLength = arg<nt::ULONG>(d.core, 10);
        const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(d.core, 11);
        const auto ObjectCreation       = arg<nt::BOOLEAN>(d.core, 12);
        const auto GrantedAccess        = arg<nt::PACCESS_MASK>(d.core, 13);
        const auto AccessStatus         = arg<nt::PNTSTATUS>(d.core, 14);
        const auto GenerateOnClose      = arg<nt::PBOOLEAN>(d.core, 15);

        if constexpr(g_debug)
            LOG(INFO, "NtAccessCheckByTypeAndAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, ObjectTypeName:{:#x}, ObjectName:{:#x}, SecurityDescriptor:{:#x}, PrincipalSelfSid:{:#x}, DesiredAccess:{:#x}, AuditType:{:#x}, Flags:{:#x}, ObjectTypeList:{:#x}, ObjectTypeListLength:{:#x}, GenericMapping:{:#x}, ObjectCreation:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);

        for(const auto& it : d.observers_NtAccessCheckByTypeAndAuditAlarm)
            it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }

    static void on_NtAccessCheckByType(monitor::syscalls::Data& d)
    {
        const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(d.core, 0);
        const auto PrincipalSelfSid     = arg<nt::PSID>(d.core, 1);
        const auto ClientToken          = arg<nt::HANDLE>(d.core, 2);
        const auto DesiredAccess        = arg<nt::ACCESS_MASK>(d.core, 3);
        const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(d.core, 4);
        const auto ObjectTypeListLength = arg<nt::ULONG>(d.core, 5);
        const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(d.core, 6);
        const auto PrivilegeSet         = arg<nt::PPRIVILEGE_SET>(d.core, 7);
        const auto PrivilegeSetLength   = arg<nt::PULONG>(d.core, 8);
        const auto GrantedAccess        = arg<nt::PACCESS_MASK>(d.core, 9);
        const auto AccessStatus         = arg<nt::PNTSTATUS>(d.core, 10);

        if constexpr(g_debug)
            LOG(INFO, "NtAccessCheckByType(SecurityDescriptor:{:#x}, PrincipalSelfSid:{:#x}, ClientToken:{:#x}, DesiredAccess:{:#x}, ObjectTypeList:{:#x}, ObjectTypeListLength:{:#x}, GenericMapping:{:#x}, PrivilegeSet:{:#x}, PrivilegeSetLength:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x})", SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);

        for(const auto& it : d.observers_NtAccessCheckByType)
            it(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    }

    static void on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle(monitor::syscalls::Data& d)
    {
        const auto SubsystemName        = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto HandleId             = arg<nt::PVOID>(d.core, 1);
        const auto ClientToken          = arg<nt::HANDLE>(d.core, 2);
        const auto ObjectTypeName       = arg<nt::PUNICODE_STRING>(d.core, 3);
        const auto ObjectName           = arg<nt::PUNICODE_STRING>(d.core, 4);
        const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(d.core, 5);
        const auto PrincipalSelfSid     = arg<nt::PSID>(d.core, 6);
        const auto DesiredAccess        = arg<nt::ACCESS_MASK>(d.core, 7);
        const auto AuditType            = arg<nt::AUDIT_EVENT_TYPE>(d.core, 8);
        const auto Flags                = arg<nt::ULONG>(d.core, 9);
        const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(d.core, 10);
        const auto ObjectTypeListLength = arg<nt::ULONG>(d.core, 11);
        const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(d.core, 12);
        const auto ObjectCreation       = arg<nt::BOOLEAN>(d.core, 13);
        const auto GrantedAccess        = arg<nt::PACCESS_MASK>(d.core, 14);
        const auto AccessStatus         = arg<nt::PNTSTATUS>(d.core, 15);
        const auto GenerateOnClose      = arg<nt::PBOOLEAN>(d.core, 16);

        if constexpr(g_debug)
            LOG(INFO, "NtAccessCheckByTypeResultListAndAuditAlarmByHandle(SubsystemName:{:#x}, HandleId:{:#x}, ClientToken:{:#x}, ObjectTypeName:{:#x}, ObjectName:{:#x}, SecurityDescriptor:{:#x}, PrincipalSelfSid:{:#x}, DesiredAccess:{:#x}, AuditType:{:#x}, Flags:{:#x}, ObjectTypeList:{:#x}, ObjectTypeListLength:{:#x}, GenericMapping:{:#x}, ObjectCreation:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);

        for(const auto& it : d.observers_NtAccessCheckByTypeResultListAndAuditAlarmByHandle)
            it(SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }

    static void on_NtAccessCheckByTypeResultListAndAuditAlarm(monitor::syscalls::Data& d)
    {
        const auto SubsystemName        = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto HandleId             = arg<nt::PVOID>(d.core, 1);
        const auto ObjectTypeName       = arg<nt::PUNICODE_STRING>(d.core, 2);
        const auto ObjectName           = arg<nt::PUNICODE_STRING>(d.core, 3);
        const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(d.core, 4);
        const auto PrincipalSelfSid     = arg<nt::PSID>(d.core, 5);
        const auto DesiredAccess        = arg<nt::ACCESS_MASK>(d.core, 6);
        const auto AuditType            = arg<nt::AUDIT_EVENT_TYPE>(d.core, 7);
        const auto Flags                = arg<nt::ULONG>(d.core, 8);
        const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(d.core, 9);
        const auto ObjectTypeListLength = arg<nt::ULONG>(d.core, 10);
        const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(d.core, 11);
        const auto ObjectCreation       = arg<nt::BOOLEAN>(d.core, 12);
        const auto GrantedAccess        = arg<nt::PACCESS_MASK>(d.core, 13);
        const auto AccessStatus         = arg<nt::PNTSTATUS>(d.core, 14);
        const auto GenerateOnClose      = arg<nt::PBOOLEAN>(d.core, 15);

        if constexpr(g_debug)
            LOG(INFO, "NtAccessCheckByTypeResultListAndAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, ObjectTypeName:{:#x}, ObjectName:{:#x}, SecurityDescriptor:{:#x}, PrincipalSelfSid:{:#x}, DesiredAccess:{:#x}, AuditType:{:#x}, Flags:{:#x}, ObjectTypeList:{:#x}, ObjectTypeListLength:{:#x}, GenericMapping:{:#x}, ObjectCreation:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);

        for(const auto& it : d.observers_NtAccessCheckByTypeResultListAndAuditAlarm)
            it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }

    static void on_NtAccessCheckByTypeResultList(monitor::syscalls::Data& d)
    {
        const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(d.core, 0);
        const auto PrincipalSelfSid     = arg<nt::PSID>(d.core, 1);
        const auto ClientToken          = arg<nt::HANDLE>(d.core, 2);
        const auto DesiredAccess        = arg<nt::ACCESS_MASK>(d.core, 3);
        const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(d.core, 4);
        const auto ObjectTypeListLength = arg<nt::ULONG>(d.core, 5);
        const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(d.core, 6);
        const auto PrivilegeSet         = arg<nt::PPRIVILEGE_SET>(d.core, 7);
        const auto PrivilegeSetLength   = arg<nt::PULONG>(d.core, 8);
        const auto GrantedAccess        = arg<nt::PACCESS_MASK>(d.core, 9);
        const auto AccessStatus         = arg<nt::PNTSTATUS>(d.core, 10);

        if constexpr(g_debug)
            LOG(INFO, "NtAccessCheckByTypeResultList(SecurityDescriptor:{:#x}, PrincipalSelfSid:{:#x}, ClientToken:{:#x}, DesiredAccess:{:#x}, ObjectTypeList:{:#x}, ObjectTypeListLength:{:#x}, GenericMapping:{:#x}, PrivilegeSet:{:#x}, PrivilegeSetLength:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x})", SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);

        for(const auto& it : d.observers_NtAccessCheckByTypeResultList)
            it(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    }

    static void on_NtAccessCheck(monitor::syscalls::Data& d)
    {
        const auto SecurityDescriptor = arg<nt::PSECURITY_DESCRIPTOR>(d.core, 0);
        const auto ClientToken        = arg<nt::HANDLE>(d.core, 1);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(d.core, 2);
        const auto GenericMapping     = arg<nt::PGENERIC_MAPPING>(d.core, 3);
        const auto PrivilegeSet       = arg<nt::PPRIVILEGE_SET>(d.core, 4);
        const auto PrivilegeSetLength = arg<nt::PULONG>(d.core, 5);
        const auto GrantedAccess      = arg<nt::PACCESS_MASK>(d.core, 6);
        const auto AccessStatus       = arg<nt::PNTSTATUS>(d.core, 7);

        if constexpr(g_debug)
            LOG(INFO, "NtAccessCheck(SecurityDescriptor:{:#x}, ClientToken:{:#x}, DesiredAccess:{:#x}, GenericMapping:{:#x}, PrivilegeSet:{:#x}, PrivilegeSetLength:{:#x}, GrantedAccess:{:#x}, AccessStatus:{:#x})", SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);

        for(const auto& it : d.observers_NtAccessCheck)
            it(SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    }

    static void on_NtAddAtom(monitor::syscalls::Data& d)
    {
        const auto AtomName = arg<nt::PWSTR>(d.core, 0);
        const auto Length   = arg<nt::ULONG>(d.core, 1);
        const auto Atom     = arg<nt::PRTL_ATOM>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAddAtom(AtomName:{:#x}, Length:{:#x}, Atom:{:#x})", AtomName, Length, Atom);

        for(const auto& it : d.observers_NtAddAtom)
            it(AtomName, Length, Atom);
    }

    static void on_NtAddBootEntry(monitor::syscalls::Data& d)
    {
        const auto BootEntry = arg<nt::PBOOT_ENTRY>(d.core, 0);
        const auto Id        = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtAddBootEntry(BootEntry:{:#x}, Id:{:#x})", BootEntry, Id);

        for(const auto& it : d.observers_NtAddBootEntry)
            it(BootEntry, Id);
    }

    static void on_NtAddDriverEntry(monitor::syscalls::Data& d)
    {
        const auto DriverEntry = arg<nt::PEFI_DRIVER_ENTRY>(d.core, 0);
        const auto Id          = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtAddDriverEntry(DriverEntry:{:#x}, Id:{:#x})", DriverEntry, Id);

        for(const auto& it : d.observers_NtAddDriverEntry)
            it(DriverEntry, Id);
    }

    static void on_NtAdjustGroupsToken(monitor::syscalls::Data& d)
    {
        const auto TokenHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto ResetToDefault = arg<nt::BOOLEAN>(d.core, 1);
        const auto NewState       = arg<nt::PTOKEN_GROUPS>(d.core, 2);
        const auto BufferLength   = arg<nt::ULONG>(d.core, 3);
        const auto PreviousState  = arg<nt::PTOKEN_GROUPS>(d.core, 4);
        const auto ReturnLength   = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtAdjustGroupsToken(TokenHandle:{:#x}, ResetToDefault:{:#x}, NewState:{:#x}, BufferLength:{:#x}, PreviousState:{:#x}, ReturnLength:{:#x})", TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength);

        for(const auto& it : d.observers_NtAdjustGroupsToken)
            it(TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength);
    }

    static void on_NtAdjustPrivilegesToken(monitor::syscalls::Data& d)
    {
        const auto TokenHandle          = arg<nt::HANDLE>(d.core, 0);
        const auto DisableAllPrivileges = arg<nt::BOOLEAN>(d.core, 1);
        const auto NewState             = arg<nt::PTOKEN_PRIVILEGES>(d.core, 2);
        const auto BufferLength         = arg<nt::ULONG>(d.core, 3);
        const auto PreviousState        = arg<nt::PTOKEN_PRIVILEGES>(d.core, 4);
        const auto ReturnLength         = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtAdjustPrivilegesToken(TokenHandle:{:#x}, DisableAllPrivileges:{:#x}, NewState:{:#x}, BufferLength:{:#x}, PreviousState:{:#x}, ReturnLength:{:#x})", TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);

        for(const auto& it : d.observers_NtAdjustPrivilegesToken)
            it(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
    }

    static void on_NtAlertResumeThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle         = arg<nt::HANDLE>(d.core, 0);
        const auto PreviousSuspendCount = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtAlertResumeThread(ThreadHandle:{:#x}, PreviousSuspendCount:{:#x})", ThreadHandle, PreviousSuspendCount);

        for(const auto& it : d.observers_NtAlertResumeThread)
            it(ThreadHandle, PreviousSuspendCount);
    }

    static void on_NtAlertThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtAlertThread(ThreadHandle:{:#x})", ThreadHandle);

        for(const auto& it : d.observers_NtAlertThread)
            it(ThreadHandle);
    }

    static void on_NtAllocateLocallyUniqueId(monitor::syscalls::Data& d)
    {
        const auto Luid = arg<nt::PLUID>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtAllocateLocallyUniqueId(Luid:{:#x})", Luid);

        for(const auto& it : d.observers_NtAllocateLocallyUniqueId)
            it(Luid);
    }

    static void on_NtAllocateReserveObject(monitor::syscalls::Data& d)
    {
        const auto MemoryReserveHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto ObjectAttributes    = arg<nt::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto Type                = arg<nt::MEMORY_RESERVE_TYPE>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAllocateReserveObject(MemoryReserveHandle:{:#x}, ObjectAttributes:{:#x}, Type:{:#x})", MemoryReserveHandle, ObjectAttributes, Type);

        for(const auto& it : d.observers_NtAllocateReserveObject)
            it(MemoryReserveHandle, ObjectAttributes, Type);
    }

    static void on_NtAllocateUserPhysicalPages(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 0);
        const auto NumberOfPages = arg<nt::PULONG_PTR>(d.core, 1);
        const auto UserPfnArra   = arg<nt::PULONG_PTR>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAllocateUserPhysicalPages(ProcessHandle:{:#x}, NumberOfPages:{:#x}, UserPfnArra:{:#x})", ProcessHandle, NumberOfPages, UserPfnArra);

        for(const auto& it : d.observers_NtAllocateUserPhysicalPages)
            it(ProcessHandle, NumberOfPages, UserPfnArra);
    }

    static void on_NtAllocateUuids(monitor::syscalls::Data& d)
    {
        const auto Time     = arg<nt::PULARGE_INTEGER>(d.core, 0);
        const auto Range    = arg<nt::PULONG>(d.core, 1);
        const auto Sequence = arg<nt::PULONG>(d.core, 2);
        const auto Seed     = arg<nt::PCHAR>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtAllocateUuids(Time:{:#x}, Range:{:#x}, Sequence:{:#x}, Seed:{:#x})", Time, Range, Sequence, Seed);

        for(const auto& it : d.observers_NtAllocateUuids)
            it(Time, Range, Sequence, Seed);
    }

    static void on_NtAllocateVirtualMemory(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(d.core, 1);
        const auto ZeroBits        = arg<nt::ULONG_PTR>(d.core, 2);
        const auto RegionSize      = arg<nt::PSIZE_T>(d.core, 3);
        const auto AllocationType  = arg<nt::ULONG>(d.core, 4);
        const auto Protect         = arg<nt::ULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtAllocateVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, ZeroBits:{:#x}, RegionSize:{:#x}, AllocationType:{:#x}, Protect:{:#x})", ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect);

        for(const auto& it : d.observers_NtAllocateVirtualMemory)
            it(ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
    }

    static void on_NtAlpcAcceptConnectPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle                  = arg<nt::PHANDLE>(d.core, 0);
        const auto ConnectionPortHandle        = arg<nt::HANDLE>(d.core, 1);
        const auto Flags                       = arg<nt::ULONG>(d.core, 2);
        const auto ObjectAttributes            = arg<nt::POBJECT_ATTRIBUTES>(d.core, 3);
        const auto PortAttributes              = arg<nt::PALPC_PORT_ATTRIBUTES>(d.core, 4);
        const auto PortContext                 = arg<nt::PVOID>(d.core, 5);
        const auto ConnectionRequest           = arg<nt::PPORT_MESSAGE>(d.core, 6);
        const auto ConnectionMessageAttributes = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(d.core, 7);
        const auto AcceptConnection            = arg<nt::BOOLEAN>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcAcceptConnectPort(PortHandle:{:#x}, ConnectionPortHandle:{:#x}, Flags:{:#x}, ObjectAttributes:{:#x}, PortAttributes:{:#x}, PortContext:{:#x}, ConnectionRequest:{:#x}, ConnectionMessageAttributes:{:#x}, AcceptConnection:{:#x})", PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection);

        for(const auto& it : d.observers_NtAlpcAcceptConnectPort)
            it(PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection);
    }

    static void on_NtAlpcCancelMessage(monitor::syscalls::Data& d)
    {
        const auto PortHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto Flags          = arg<nt::ULONG>(d.core, 1);
        const auto MessageContext = arg<nt::PALPC_CONTEXT_ATTR>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcCancelMessage(PortHandle:{:#x}, Flags:{:#x}, MessageContext:{:#x})", PortHandle, Flags, MessageContext);

        for(const auto& it : d.observers_NtAlpcCancelMessage)
            it(PortHandle, Flags, MessageContext);
    }

    static void on_NtAlpcConnectPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle           = arg<nt::PHANDLE>(d.core, 0);
        const auto PortName             = arg<nt::PUNICODE_STRING>(d.core, 1);
        const auto ObjectAttributes     = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto PortAttributes       = arg<nt::PALPC_PORT_ATTRIBUTES>(d.core, 3);
        const auto Flags                = arg<nt::ULONG>(d.core, 4);
        const auto RequiredServerSid    = arg<nt::PSID>(d.core, 5);
        const auto ConnectionMessage    = arg<nt::PPORT_MESSAGE>(d.core, 6);
        const auto BufferLength         = arg<nt::PULONG>(d.core, 7);
        const auto OutMessageAttributes = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(d.core, 8);
        const auto InMessageAttributes  = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(d.core, 9);
        const auto Timeout              = arg<nt::PLARGE_INTEGER>(d.core, 10);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcConnectPort(PortHandle:{:#x}, PortName:{:#x}, ObjectAttributes:{:#x}, PortAttributes:{:#x}, Flags:{:#x}, RequiredServerSid:{:#x}, ConnectionMessage:{:#x}, BufferLength:{:#x}, OutMessageAttributes:{:#x}, InMessageAttributes:{:#x}, Timeout:{:#x})", PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout);

        for(const auto& it : d.observers_NtAlpcConnectPort)
            it(PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout);
    }

    static void on_NtAlpcCreatePort(monitor::syscalls::Data& d)
    {
        const auto PortHandle       = arg<nt::PHANDLE>(d.core, 0);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto PortAttributes   = arg<nt::PALPC_PORT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcCreatePort(PortHandle:{:#x}, ObjectAttributes:{:#x}, PortAttributes:{:#x})", PortHandle, ObjectAttributes, PortAttributes);

        for(const auto& it : d.observers_NtAlpcCreatePort)
            it(PortHandle, ObjectAttributes, PortAttributes);
    }

    static void on_NtAlpcCreatePortSection(monitor::syscalls::Data& d)
    {
        const auto PortHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto Flags             = arg<nt::ULONG>(d.core, 1);
        const auto SectionHandle     = arg<nt::HANDLE>(d.core, 2);
        const auto SectionSize       = arg<nt::SIZE_T>(d.core, 3);
        const auto AlpcSectionHandle = arg<nt::PALPC_HANDLE>(d.core, 4);
        const auto ActualSectionSize = arg<nt::PSIZE_T>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcCreatePortSection(PortHandle:{:#x}, Flags:{:#x}, SectionHandle:{:#x}, SectionSize:{:#x}, AlpcSectionHandle:{:#x}, ActualSectionSize:{:#x})", PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize);

        for(const auto& it : d.observers_NtAlpcCreatePortSection)
            it(PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize);
    }

    static void on_NtAlpcCreateResourceReserve(monitor::syscalls::Data& d)
    {
        const auto PortHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto MessageSize = arg<nt::SIZE_T>(d.core, 2);
        const auto ResourceId  = arg<nt::PALPC_HANDLE>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcCreateResourceReserve(PortHandle:{:#x}, Flags:{:#x}, MessageSize:{:#x}, ResourceId:{:#x})", PortHandle, Flags, MessageSize, ResourceId);

        for(const auto& it : d.observers_NtAlpcCreateResourceReserve)
            it(PortHandle, Flags, MessageSize, ResourceId);
    }

    static void on_NtAlpcCreateSectionView(monitor::syscalls::Data& d)
    {
        const auto PortHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto Flags          = arg<nt::ULONG>(d.core, 1);
        const auto ViewAttributes = arg<nt::PALPC_DATA_VIEW_ATTR>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcCreateSectionView(PortHandle:{:#x}, Flags:{:#x}, ViewAttributes:{:#x})", PortHandle, Flags, ViewAttributes);

        for(const auto& it : d.observers_NtAlpcCreateSectionView)
            it(PortHandle, Flags, ViewAttributes);
    }

    static void on_NtAlpcCreateSecurityContext(monitor::syscalls::Data& d)
    {
        const auto PortHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto Flags             = arg<nt::ULONG>(d.core, 1);
        const auto SecurityAttribute = arg<nt::PALPC_SECURITY_ATTR>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcCreateSecurityContext(PortHandle:{:#x}, Flags:{:#x}, SecurityAttribute:{:#x})", PortHandle, Flags, SecurityAttribute);

        for(const auto& it : d.observers_NtAlpcCreateSecurityContext)
            it(PortHandle, Flags, SecurityAttribute);
    }

    static void on_NtAlpcDeletePortSection(monitor::syscalls::Data& d)
    {
        const auto PortHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto Flags         = arg<nt::ULONG>(d.core, 1);
        const auto SectionHandle = arg<nt::ALPC_HANDLE>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcDeletePortSection(PortHandle:{:#x}, Flags:{:#x}, SectionHandle:{:#x})", PortHandle, Flags, SectionHandle);

        for(const auto& it : d.observers_NtAlpcDeletePortSection)
            it(PortHandle, Flags, SectionHandle);
    }

    static void on_NtAlpcDeleteResourceReserve(monitor::syscalls::Data& d)
    {
        const auto PortHandle = arg<nt::HANDLE>(d.core, 0);
        const auto Flags      = arg<nt::ULONG>(d.core, 1);
        const auto ResourceId = arg<nt::ALPC_HANDLE>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcDeleteResourceReserve(PortHandle:{:#x}, Flags:{:#x}, ResourceId:{:#x})", PortHandle, Flags, ResourceId);

        for(const auto& it : d.observers_NtAlpcDeleteResourceReserve)
            it(PortHandle, Flags, ResourceId);
    }

    static void on_NtAlpcDeleteSectionView(monitor::syscalls::Data& d)
    {
        const auto PortHandle = arg<nt::HANDLE>(d.core, 0);
        const auto Flags      = arg<nt::ULONG>(d.core, 1);
        const auto ViewBase   = arg<nt::PVOID>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcDeleteSectionView(PortHandle:{:#x}, Flags:{:#x}, ViewBase:{:#x})", PortHandle, Flags, ViewBase);

        for(const auto& it : d.observers_NtAlpcDeleteSectionView)
            it(PortHandle, Flags, ViewBase);
    }

    static void on_NtAlpcDeleteSecurityContext(monitor::syscalls::Data& d)
    {
        const auto PortHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto Flags         = arg<nt::ULONG>(d.core, 1);
        const auto ContextHandle = arg<nt::ALPC_HANDLE>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcDeleteSecurityContext(PortHandle:{:#x}, Flags:{:#x}, ContextHandle:{:#x})", PortHandle, Flags, ContextHandle);

        for(const auto& it : d.observers_NtAlpcDeleteSecurityContext)
            it(PortHandle, Flags, ContextHandle);
    }

    static void on_NtAlpcDisconnectPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle = arg<nt::HANDLE>(d.core, 0);
        const auto Flags      = arg<nt::ULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcDisconnectPort(PortHandle:{:#x}, Flags:{:#x})", PortHandle, Flags);

        for(const auto& it : d.observers_NtAlpcDisconnectPort)
            it(PortHandle, Flags);
    }

    static void on_NtAlpcImpersonateClientOfPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto PortMessage = arg<nt::PPORT_MESSAGE>(d.core, 1);
        const auto Reserved    = arg<nt::PVOID>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcImpersonateClientOfPort(PortHandle:{:#x}, PortMessage:{:#x}, Reserved:{:#x})", PortHandle, PortMessage, Reserved);

        for(const auto& it : d.observers_NtAlpcImpersonateClientOfPort)
            it(PortHandle, PortMessage, Reserved);
    }

    static void on_NtAlpcOpenSenderProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle    = arg<nt::PHANDLE>(d.core, 0);
        const auto PortHandle       = arg<nt::HANDLE>(d.core, 1);
        const auto PortMessage      = arg<nt::PPORT_MESSAGE>(d.core, 2);
        const auto Flags            = arg<nt::ULONG>(d.core, 3);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 4);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcOpenSenderProcess(ProcessHandle:{:#x}, PortHandle:{:#x}, PortMessage:{:#x}, Flags:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtAlpcOpenSenderProcess)
            it(ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    }

    static void on_NtAlpcOpenSenderThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle     = arg<nt::PHANDLE>(d.core, 0);
        const auto PortHandle       = arg<nt::HANDLE>(d.core, 1);
        const auto PortMessage      = arg<nt::PPORT_MESSAGE>(d.core, 2);
        const auto Flags            = arg<nt::ULONG>(d.core, 3);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 4);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcOpenSenderThread(ThreadHandle:{:#x}, PortHandle:{:#x}, PortMessage:{:#x}, Flags:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtAlpcOpenSenderThread)
            it(ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    }

    static void on_NtAlpcQueryInformation(monitor::syscalls::Data& d)
    {
        const auto PortHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto PortInformationClass = arg<nt::ALPC_PORT_INFORMATION_CLASS>(d.core, 1);
        const auto PortInformation      = arg<nt::PVOID>(d.core, 2);
        const auto Length               = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength         = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcQueryInformation(PortHandle:{:#x}, PortInformationClass:{:#x}, PortInformation:{:#x}, Length:{:#x}, ReturnLength:{:#x})", PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);

        for(const auto& it : d.observers_NtAlpcQueryInformation)
            it(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    }

    static void on_NtAlpcQueryInformationMessage(monitor::syscalls::Data& d)
    {
        const auto PortHandle              = arg<nt::HANDLE>(d.core, 0);
        const auto PortMessage             = arg<nt::PPORT_MESSAGE>(d.core, 1);
        const auto MessageInformationClass = arg<nt::ALPC_MESSAGE_INFORMATION_CLASS>(d.core, 2);
        const auto MessageInformation      = arg<nt::PVOID>(d.core, 3);
        const auto Length                  = arg<nt::ULONG>(d.core, 4);
        const auto ReturnLength            = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcQueryInformationMessage(PortHandle:{:#x}, PortMessage:{:#x}, MessageInformationClass:{:#x}, MessageInformation:{:#x}, Length:{:#x}, ReturnLength:{:#x})", PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength);

        for(const auto& it : d.observers_NtAlpcQueryInformationMessage)
            it(PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength);
    }

    static void on_NtAlpcRevokeSecurityContext(monitor::syscalls::Data& d)
    {
        const auto PortHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto Flags         = arg<nt::ULONG>(d.core, 1);
        const auto ContextHandle = arg<nt::ALPC_HANDLE>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcRevokeSecurityContext(PortHandle:{:#x}, Flags:{:#x}, ContextHandle:{:#x})", PortHandle, Flags, ContextHandle);

        for(const auto& it : d.observers_NtAlpcRevokeSecurityContext)
            it(PortHandle, Flags, ContextHandle);
    }

    static void on_NtAlpcSendWaitReceivePort(monitor::syscalls::Data& d)
    {
        const auto PortHandle               = arg<nt::HANDLE>(d.core, 0);
        const auto Flags                    = arg<nt::ULONG>(d.core, 1);
        const auto SendMessage              = arg<nt::PPORT_MESSAGE>(d.core, 2);
        const auto SendMessageAttributes    = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(d.core, 3);
        const auto ReceiveMessage           = arg<nt::PPORT_MESSAGE>(d.core, 4);
        const auto BufferLength             = arg<nt::PULONG>(d.core, 5);
        const auto ReceiveMessageAttributes = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(d.core, 6);
        const auto Timeout                  = arg<nt::PLARGE_INTEGER>(d.core, 7);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcSendWaitReceivePort(PortHandle:{:#x}, Flags:{:#x}, SendMessage:{:#x}, SendMessageAttributes:{:#x}, ReceiveMessage:{:#x}, BufferLength:{:#x}, ReceiveMessageAttributes:{:#x}, Timeout:{:#x})", PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout);

        for(const auto& it : d.observers_NtAlpcSendWaitReceivePort)
            it(PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout);
    }

    static void on_NtAlpcSetInformation(monitor::syscalls::Data& d)
    {
        const auto PortHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto PortInformationClass = arg<nt::ALPC_PORT_INFORMATION_CLASS>(d.core, 1);
        const auto PortInformation      = arg<nt::PVOID>(d.core, 2);
        const auto Length               = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtAlpcSetInformation(PortHandle:{:#x}, PortInformationClass:{:#x}, PortInformation:{:#x}, Length:{:#x})", PortHandle, PortInformationClass, PortInformation, Length);

        for(const auto& it : d.observers_NtAlpcSetInformation)
            it(PortHandle, PortInformationClass, PortInformation, Length);
    }

    static void on_NtApphelpCacheControl(monitor::syscalls::Data& d)
    {
        const auto type = arg<nt::APPHELPCOMMAND>(d.core, 0);
        const auto buf  = arg<nt::PVOID>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtApphelpCacheControl(type:{:#x}, buf:{:#x})", type, buf);

        for(const auto& it : d.observers_NtApphelpCacheControl)
            it(type, buf);
    }

    static void on_NtAreMappedFilesTheSame(monitor::syscalls::Data& d)
    {
        const auto File1MappedAsAnImage = arg<nt::PVOID>(d.core, 0);
        const auto File2MappedAsFile    = arg<nt::PVOID>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtAreMappedFilesTheSame(File1MappedAsAnImage:{:#x}, File2MappedAsFile:{:#x})", File1MappedAsAnImage, File2MappedAsFile);

        for(const auto& it : d.observers_NtAreMappedFilesTheSame)
            it(File1MappedAsAnImage, File2MappedAsFile);
    }

    static void on_NtAssignProcessToJobObject(monitor::syscalls::Data& d)
    {
        const auto JobHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtAssignProcessToJobObject(JobHandle:{:#x}, ProcessHandle:{:#x})", JobHandle, ProcessHandle);

        for(const auto& it : d.observers_NtAssignProcessToJobObject)
            it(JobHandle, ProcessHandle);
    }

    static void on_NtCallbackReturn(monitor::syscalls::Data& d)
    {
        const auto OutputBuffer = arg<nt::PVOID>(d.core, 0);
        const auto OutputLength = arg<nt::ULONG>(d.core, 1);
        const auto Status       = arg<nt::NTSTATUS>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtCallbackReturn(OutputBuffer:{:#x}, OutputLength:{:#x}, Status:{:#x})", OutputBuffer, OutputLength, Status);

        for(const auto& it : d.observers_NtCallbackReturn)
            it(OutputBuffer, OutputLength, Status);
    }

    static void on_NtCancelIoFileEx(monitor::syscalls::Data& d)
    {
        const auto FileHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto IoRequestToCancel = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtCancelIoFileEx(FileHandle:{:#x}, IoRequestToCancel:{:#x}, IoStatusBlock:{:#x})", FileHandle, IoRequestToCancel, IoStatusBlock);

        for(const auto& it : d.observers_NtCancelIoFileEx)
            it(FileHandle, IoRequestToCancel, IoStatusBlock);
    }

    static void on_NtCancelIoFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtCancelIoFile(FileHandle:{:#x}, IoStatusBlock:{:#x})", FileHandle, IoStatusBlock);

        for(const auto& it : d.observers_NtCancelIoFile)
            it(FileHandle, IoStatusBlock);
    }

    static void on_NtCancelSynchronousIoFile(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle      = arg<nt::HANDLE>(d.core, 0);
        const auto IoRequestToCancel = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtCancelSynchronousIoFile(ThreadHandle:{:#x}, IoRequestToCancel:{:#x}, IoStatusBlock:{:#x})", ThreadHandle, IoRequestToCancel, IoStatusBlock);

        for(const auto& it : d.observers_NtCancelSynchronousIoFile)
            it(ThreadHandle, IoRequestToCancel, IoStatusBlock);
    }

    static void on_NtCancelTimer(monitor::syscalls::Data& d)
    {
        const auto TimerHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto CurrentState = arg<nt::PBOOLEAN>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtCancelTimer(TimerHandle:{:#x}, CurrentState:{:#x})", TimerHandle, CurrentState);

        for(const auto& it : d.observers_NtCancelTimer)
            it(TimerHandle, CurrentState);
    }

    static void on_NtClearEvent(monitor::syscalls::Data& d)
    {
        const auto EventHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtClearEvent(EventHandle:{:#x})", EventHandle);

        for(const auto& it : d.observers_NtClearEvent)
            it(EventHandle);
    }

    static void on_NtClose(monitor::syscalls::Data& d)
    {
        const auto Handle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtClose(Handle:{:#x})", Handle);

        for(const auto& it : d.observers_NtClose)
            it(Handle);
    }

    static void on_NtCloseObjectAuditAlarm(monitor::syscalls::Data& d)
    {
        const auto SubsystemName   = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto HandleId        = arg<nt::PVOID>(d.core, 1);
        const auto GenerateOnClose = arg<nt::BOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtCloseObjectAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, GenerateOnClose);

        for(const auto& it : d.observers_NtCloseObjectAuditAlarm)
            it(SubsystemName, HandleId, GenerateOnClose);
    }

    static void on_NtCommitComplete(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtCommitComplete(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtCommitComplete)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtCommitEnlistment(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtCommitEnlistment(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtCommitEnlistment)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtCommitTransaction(monitor::syscalls::Data& d)
    {
        const auto TransactionHandle = arg<nt::HANDLE>(d.core, 0);
        const auto Wait              = arg<nt::BOOLEAN>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtCommitTransaction(TransactionHandle:{:#x}, Wait:{:#x})", TransactionHandle, Wait);

        for(const auto& it : d.observers_NtCommitTransaction)
            it(TransactionHandle, Wait);
    }

    static void on_NtCompactKeys(monitor::syscalls::Data& d)
    {
        const auto Count    = arg<nt::ULONG>(d.core, 0);
        const auto KeyArray = arg<nt::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtCompactKeys(Count:{:#x}, KeyArray:{:#x})", Count, KeyArray);

        for(const auto& it : d.observers_NtCompactKeys)
            it(Count, KeyArray);
    }

    static void on_NtCompareTokens(monitor::syscalls::Data& d)
    {
        const auto FirstTokenHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto SecondTokenHandle = arg<nt::HANDLE>(d.core, 1);
        const auto Equal             = arg<nt::PBOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtCompareTokens(FirstTokenHandle:{:#x}, SecondTokenHandle:{:#x}, Equal:{:#x})", FirstTokenHandle, SecondTokenHandle, Equal);

        for(const auto& it : d.observers_NtCompareTokens)
            it(FirstTokenHandle, SecondTokenHandle, Equal);
    }

    static void on_NtCompleteConnectPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtCompleteConnectPort(PortHandle:{:#x})", PortHandle);

        for(const auto& it : d.observers_NtCompleteConnectPort)
            it(PortHandle);
    }

    static void on_NtCompressKey(monitor::syscalls::Data& d)
    {
        const auto Key = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtCompressKey(Key:{:#x})", Key);

        for(const auto& it : d.observers_NtCompressKey)
            it(Key);
    }

    static void on_NtConnectPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle                  = arg<nt::PHANDLE>(d.core, 0);
        const auto PortName                    = arg<nt::PUNICODE_STRING>(d.core, 1);
        const auto SecurityQos                 = arg<nt::PSECURITY_QUALITY_OF_SERVICE>(d.core, 2);
        const auto ClientView                  = arg<nt::PPORT_VIEW>(d.core, 3);
        const auto ServerView                  = arg<nt::PREMOTE_PORT_VIEW>(d.core, 4);
        const auto MaxMessageLength            = arg<nt::PULONG>(d.core, 5);
        const auto ConnectionInformation       = arg<nt::PVOID>(d.core, 6);
        const auto ConnectionInformationLength = arg<nt::PULONG>(d.core, 7);

        if constexpr(g_debug)
            LOG(INFO, "NtConnectPort(PortHandle:{:#x}, PortName:{:#x}, SecurityQos:{:#x}, ClientView:{:#x}, ServerView:{:#x}, MaxMessageLength:{:#x}, ConnectionInformation:{:#x}, ConnectionInformationLength:{:#x})", PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);

        for(const auto& it : d.observers_NtConnectPort)
            it(PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    }

    static void on_NtContinue(monitor::syscalls::Data& d)
    {
        const auto ContextRecord = arg<nt::PCONTEXT>(d.core, 0);
        const auto TestAlert     = arg<nt::BOOLEAN>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtContinue(ContextRecord:{:#x}, TestAlert:{:#x})", ContextRecord, TestAlert);

        for(const auto& it : d.observers_NtContinue)
            it(ContextRecord, TestAlert);
    }

    static void on_NtCreateDebugObject(monitor::syscalls::Data& d)
    {
        const auto DebugObjectHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Flags             = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateDebugObject(DebugObjectHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, Flags:{:#x})", DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags);

        for(const auto& it : d.observers_NtCreateDebugObject)
            it(DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags);
    }

    static void on_NtCreateDirectoryObject(monitor::syscalls::Data& d)
    {
        const auto DirectoryHandle  = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateDirectoryObject(DirectoryHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", DirectoryHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtCreateDirectoryObject)
            it(DirectoryHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtCreateEnlistment(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle      = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess         = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ResourceManagerHandle = arg<nt::HANDLE>(d.core, 2);
        const auto TransactionHandle     = arg<nt::HANDLE>(d.core, 3);
        const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(d.core, 4);
        const auto CreateOptions         = arg<nt::ULONG>(d.core, 5);
        const auto NotificationMask      = arg<nt::NOTIFICATION_MASK>(d.core, 6);
        const auto EnlistmentKey         = arg<nt::PVOID>(d.core, 7);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateEnlistment(EnlistmentHandle:{:#x}, DesiredAccess:{:#x}, ResourceManagerHandle:{:#x}, TransactionHandle:{:#x}, ObjectAttributes:{:#x}, CreateOptions:{:#x}, NotificationMask:{:#x}, EnlistmentKey:{:#x})", EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey);

        for(const auto& it : d.observers_NtCreateEnlistment)
            it(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey);
    }

    static void on_NtCreateEvent(monitor::syscalls::Data& d)
    {
        const auto EventHandle      = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto EventType        = arg<nt::EVENT_TYPE>(d.core, 3);
        const auto InitialState     = arg<nt::BOOLEAN>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateEvent(EventHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, EventType:{:#x}, InitialState:{:#x})", EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);

        for(const auto& it : d.observers_NtCreateEvent)
            it(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
    }

    static void on_NtCreateEventPair(monitor::syscalls::Data& d)
    {
        const auto EventPairHandle  = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateEventPair(EventPairHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", EventPairHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtCreateEventPair)
            it(EventPairHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtCreateFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle        = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(d.core, 3);
        const auto AllocationSize    = arg<nt::PLARGE_INTEGER>(d.core, 4);
        const auto FileAttributes    = arg<nt::ULONG>(d.core, 5);
        const auto ShareAccess       = arg<nt::ULONG>(d.core, 6);
        const auto CreateDisposition = arg<nt::ULONG>(d.core, 7);
        const auto CreateOptions     = arg<nt::ULONG>(d.core, 8);
        const auto EaBuffer          = arg<nt::PVOID>(d.core, 9);
        const auto EaLength          = arg<nt::ULONG>(d.core, 10);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateFile(FileHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, IoStatusBlock:{:#x}, AllocationSize:{:#x}, FileAttributes:{:#x}, ShareAccess:{:#x}, CreateDisposition:{:#x}, CreateOptions:{:#x}, EaBuffer:{:#x}, EaLength:{:#x})", FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);

        for(const auto& it : d.observers_NtCreateFile)
            it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    }

    static void on_NtCreateIoCompletion(monitor::syscalls::Data& d)
    {
        const auto IoCompletionHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Count              = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateIoCompletion(IoCompletionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, Count:{:#x})", IoCompletionHandle, DesiredAccess, ObjectAttributes, Count);

        for(const auto& it : d.observers_NtCreateIoCompletion)
            it(IoCompletionHandle, DesiredAccess, ObjectAttributes, Count);
    }

    static void on_NtCreateJobObject(monitor::syscalls::Data& d)
    {
        const auto JobHandle        = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateJobObject(JobHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", JobHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtCreateJobObject)
            it(JobHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtCreateJobSet(monitor::syscalls::Data& d)
    {
        const auto NumJob     = arg<nt::ULONG>(d.core, 0);
        const auto UserJobSet = arg<nt::PJOB_SET_ARRAY>(d.core, 1);
        const auto Flags      = arg<nt::ULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateJobSet(NumJob:{:#x}, UserJobSet:{:#x}, Flags:{:#x})", NumJob, UserJobSet, Flags);

        for(const auto& it : d.observers_NtCreateJobSet)
            it(NumJob, UserJobSet, Flags);
    }

    static void on_NtCreateKeyedEvent(monitor::syscalls::Data& d)
    {
        const auto KeyedEventHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Flags            = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateKeyedEvent(KeyedEventHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, Flags:{:#x})", KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags);

        for(const auto& it : d.observers_NtCreateKeyedEvent)
            it(KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags);
    }

    static void on_NtCreateKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle        = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto TitleIndex       = arg<nt::ULONG>(d.core, 3);
        const auto Class            = arg<nt::PUNICODE_STRING>(d.core, 4);
        const auto CreateOptions    = arg<nt::ULONG>(d.core, 5);
        const auto Disposition      = arg<nt::PULONG>(d.core, 6);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateKey(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, TitleIndex:{:#x}, Class:{:#x}, CreateOptions:{:#x}, Disposition:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);

        for(const auto& it : d.observers_NtCreateKey)
            it(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
    }

    static void on_NtCreateKeyTransacted(monitor::syscalls::Data& d)
    {
        const auto KeyHandle         = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto TitleIndex        = arg<nt::ULONG>(d.core, 3);
        const auto Class             = arg<nt::PUNICODE_STRING>(d.core, 4);
        const auto CreateOptions     = arg<nt::ULONG>(d.core, 5);
        const auto TransactionHandle = arg<nt::HANDLE>(d.core, 6);
        const auto Disposition       = arg<nt::PULONG>(d.core, 7);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateKeyTransacted(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, TitleIndex:{:#x}, Class:{:#x}, CreateOptions:{:#x}, TransactionHandle:{:#x}, Disposition:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);

        for(const auto& it : d.observers_NtCreateKeyTransacted)
            it(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);
    }

    static void on_NtCreateMailslotFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle         = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt::ULONG>(d.core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(d.core, 3);
        const auto CreateOptions      = arg<nt::ULONG>(d.core, 4);
        const auto MailslotQuota      = arg<nt::ULONG>(d.core, 5);
        const auto MaximumMessageSize = arg<nt::ULONG>(d.core, 6);
        const auto ReadTimeout        = arg<nt::PLARGE_INTEGER>(d.core, 7);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateMailslotFile(FileHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, IoStatusBlock:{:#x}, CreateOptions:{:#x}, MailslotQuota:{:#x}, MaximumMessageSize:{:#x}, ReadTimeout:{:#x})", FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout);

        for(const auto& it : d.observers_NtCreateMailslotFile)
            it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout);
    }

    static void on_NtCreateMutant(monitor::syscalls::Data& d)
    {
        const auto MutantHandle     = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto InitialOwner     = arg<nt::BOOLEAN>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateMutant(MutantHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, InitialOwner:{:#x})", MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);

        for(const auto& it : d.observers_NtCreateMutant)
            it(MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);
    }

    static void on_NtCreateNamedPipeFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle        = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt::ULONG>(d.core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(d.core, 3);
        const auto ShareAccess       = arg<nt::ULONG>(d.core, 4);
        const auto CreateDisposition = arg<nt::ULONG>(d.core, 5);
        const auto CreateOptions     = arg<nt::ULONG>(d.core, 6);
        const auto NamedPipeType     = arg<nt::ULONG>(d.core, 7);
        const auto ReadMode          = arg<nt::ULONG>(d.core, 8);
        const auto CompletionMode    = arg<nt::ULONG>(d.core, 9);
        const auto MaximumInstances  = arg<nt::ULONG>(d.core, 10);
        const auto InboundQuota      = arg<nt::ULONG>(d.core, 11);
        const auto OutboundQuota     = arg<nt::ULONG>(d.core, 12);
        const auto DefaultTimeout    = arg<nt::PLARGE_INTEGER>(d.core, 13);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateNamedPipeFile(FileHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, IoStatusBlock:{:#x}, ShareAccess:{:#x}, CreateDisposition:{:#x}, CreateOptions:{:#x}, NamedPipeType:{:#x}, ReadMode:{:#x}, CompletionMode:{:#x}, MaximumInstances:{:#x}, InboundQuota:{:#x}, OutboundQuota:{:#x}, DefaultTimeout:{:#x})", FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);

        for(const auto& it : d.observers_NtCreateNamedPipeFile)
            it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);
    }

    static void on_NtCreatePagingFile(monitor::syscalls::Data& d)
    {
        const auto PageFileName = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto MinimumSize  = arg<nt::PLARGE_INTEGER>(d.core, 1);
        const auto MaximumSize  = arg<nt::PLARGE_INTEGER>(d.core, 2);
        const auto Priority     = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtCreatePagingFile(PageFileName:{:#x}, MinimumSize:{:#x}, MaximumSize:{:#x}, Priority:{:#x})", PageFileName, MinimumSize, MaximumSize, Priority);

        for(const auto& it : d.observers_NtCreatePagingFile)
            it(PageFileName, MinimumSize, MaximumSize, Priority);
    }

    static void on_NtCreatePort(monitor::syscalls::Data& d)
    {
        const auto PortHandle              = arg<nt::PHANDLE>(d.core, 0);
        const auto ObjectAttributes        = arg<nt::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto MaxConnectionInfoLength = arg<nt::ULONG>(d.core, 2);
        const auto MaxMessageLength        = arg<nt::ULONG>(d.core, 3);
        const auto MaxPoolUsage            = arg<nt::ULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtCreatePort(PortHandle:{:#x}, ObjectAttributes:{:#x}, MaxConnectionInfoLength:{:#x}, MaxMessageLength:{:#x}, MaxPoolUsage:{:#x})", PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);

        for(const auto& it : d.observers_NtCreatePort)
            it(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    }

    static void on_NtCreatePrivateNamespace(monitor::syscalls::Data& d)
    {
        const auto NamespaceHandle    = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto BoundaryDescriptor = arg<nt::PVOID>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtCreatePrivateNamespace(NamespaceHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, BoundaryDescriptor:{:#x})", NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);

        for(const auto& it : d.observers_NtCreatePrivateNamespace)
            it(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    }

    static void on_NtCreateProcessEx(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle    = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ParentProcess    = arg<nt::HANDLE>(d.core, 3);
        const auto Flags            = arg<nt::ULONG>(d.core, 4);
        const auto SectionHandle    = arg<nt::HANDLE>(d.core, 5);
        const auto DebugPort        = arg<nt::HANDLE>(d.core, 6);
        const auto ExceptionPort    = arg<nt::HANDLE>(d.core, 7);
        const auto JobMemberLevel   = arg<nt::ULONG>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateProcessEx(ProcessHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ParentProcess:{:#x}, Flags:{:#x}, SectionHandle:{:#x}, DebugPort:{:#x}, ExceptionPort:{:#x}, JobMemberLevel:{:#x})", ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);

        for(const auto& it : d.observers_NtCreateProcessEx)
            it(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);
    }

    static void on_NtCreateProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle      = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ParentProcess      = arg<nt::HANDLE>(d.core, 3);
        const auto InheritObjectTable = arg<nt::BOOLEAN>(d.core, 4);
        const auto SectionHandle      = arg<nt::HANDLE>(d.core, 5);
        const auto DebugPort          = arg<nt::HANDLE>(d.core, 6);
        const auto ExceptionPort      = arg<nt::HANDLE>(d.core, 7);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateProcess(ProcessHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ParentProcess:{:#x}, InheritObjectTable:{:#x}, SectionHandle:{:#x}, DebugPort:{:#x}, ExceptionPort:{:#x})", ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort);

        for(const auto& it : d.observers_NtCreateProcess)
            it(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort);
    }

    static void on_NtCreateProfileEx(monitor::syscalls::Data& d)
    {
        const auto ProfileHandle      = arg<nt::PHANDLE>(d.core, 0);
        const auto Process            = arg<nt::HANDLE>(d.core, 1);
        const auto ProfileBase        = arg<nt::PVOID>(d.core, 2);
        const auto ProfileSize        = arg<nt::SIZE_T>(d.core, 3);
        const auto BucketSize         = arg<nt::ULONG>(d.core, 4);
        const auto Buffer             = arg<nt::PULONG>(d.core, 5);
        const auto BufferSize         = arg<nt::ULONG>(d.core, 6);
        const auto ProfileSource      = arg<nt::KPROFILE_SOURCE>(d.core, 7);
        const auto GroupAffinityCount = arg<nt::ULONG>(d.core, 8);
        const auto GroupAffinity      = arg<nt::PGROUP_AFFINITY>(d.core, 9);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateProfileEx(ProfileHandle:{:#x}, Process:{:#x}, ProfileBase:{:#x}, ProfileSize:{:#x}, BucketSize:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, ProfileSource:{:#x}, GroupAffinityCount:{:#x}, GroupAffinity:{:#x})", ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity);

        for(const auto& it : d.observers_NtCreateProfileEx)
            it(ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity);
    }

    static void on_NtCreateProfile(monitor::syscalls::Data& d)
    {
        const auto ProfileHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto Process       = arg<nt::HANDLE>(d.core, 1);
        const auto RangeBase     = arg<nt::PVOID>(d.core, 2);
        const auto RangeSize     = arg<nt::SIZE_T>(d.core, 3);
        const auto BucketSize    = arg<nt::ULONG>(d.core, 4);
        const auto Buffer        = arg<nt::PULONG>(d.core, 5);
        const auto BufferSize    = arg<nt::ULONG>(d.core, 6);
        const auto ProfileSource = arg<nt::KPROFILE_SOURCE>(d.core, 7);
        const auto Affinity      = arg<nt::KAFFINITY>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateProfile(ProfileHandle:{:#x}, Process:{:#x}, RangeBase:{:#x}, RangeSize:{:#x}, BucketSize:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, ProfileSource:{:#x}, Affinity:{:#x})", ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity);

        for(const auto& it : d.observers_NtCreateProfile)
            it(ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity);
    }

    static void on_NtCreateResourceManager(monitor::syscalls::Data& d)
    {
        const auto ResourceManagerHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess         = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto TmHandle              = arg<nt::HANDLE>(d.core, 2);
        const auto RmGuid                = arg<nt::LPGUID>(d.core, 3);
        const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(d.core, 4);
        const auto CreateOptions         = arg<nt::ULONG>(d.core, 5);
        const auto Description           = arg<nt::PUNICODE_STRING>(d.core, 6);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateResourceManager(ResourceManagerHandle:{:#x}, DesiredAccess:{:#x}, TmHandle:{:#x}, RmGuid:{:#x}, ObjectAttributes:{:#x}, CreateOptions:{:#x}, Description:{:#x})", ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description);

        for(const auto& it : d.observers_NtCreateResourceManager)
            it(ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description);
    }

    static void on_NtCreateSection(monitor::syscalls::Data& d)
    {
        const auto SectionHandle         = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess         = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto MaximumSize           = arg<nt::PLARGE_INTEGER>(d.core, 3);
        const auto SectionPageProtection = arg<nt::ULONG>(d.core, 4);
        const auto AllocationAttributes  = arg<nt::ULONG>(d.core, 5);
        const auto FileHandle            = arg<nt::HANDLE>(d.core, 6);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateSection(SectionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, MaximumSize:{:#x}, SectionPageProtection:{:#x}, AllocationAttributes:{:#x}, FileHandle:{:#x})", SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);

        for(const auto& it : d.observers_NtCreateSection)
            it(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
    }

    static void on_NtCreateSemaphore(monitor::syscalls::Data& d)
    {
        const auto SemaphoreHandle  = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto InitialCount     = arg<nt::LONG>(d.core, 3);
        const auto MaximumCount     = arg<nt::LONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateSemaphore(SemaphoreHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, InitialCount:{:#x}, MaximumCount:{:#x})", SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);

        for(const auto& it : d.observers_NtCreateSemaphore)
            it(SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
    }

    static void on_NtCreateSymbolicLinkObject(monitor::syscalls::Data& d)
    {
        const auto LinkHandle       = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto LinkTarget       = arg<nt::PUNICODE_STRING>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateSymbolicLinkObject(LinkHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, LinkTarget:{:#x})", LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget);

        for(const auto& it : d.observers_NtCreateSymbolicLinkObject)
            it(LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget);
    }

    static void on_NtCreateThreadEx(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle     = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ProcessHandle    = arg<nt::HANDLE>(d.core, 3);
        const auto StartRoutine     = arg<nt::PVOID>(d.core, 4);
        const auto Argument         = arg<nt::PVOID>(d.core, 5);
        const auto CreateFlags      = arg<nt::ULONG>(d.core, 6);
        const auto ZeroBits         = arg<nt::ULONG_PTR>(d.core, 7);
        const auto StackSize        = arg<nt::SIZE_T>(d.core, 8);
        const auto MaximumStackSize = arg<nt::SIZE_T>(d.core, 9);
        const auto AttributeList    = arg<nt::PPS_ATTRIBUTE_LIST>(d.core, 10);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateThreadEx(ThreadHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ProcessHandle:{:#x}, StartRoutine:{:#x}, Argument:{:#x}, CreateFlags:{:#x}, ZeroBits:{:#x}, StackSize:{:#x}, MaximumStackSize:{:#x}, AttributeList:{:#x})", ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);

        for(const auto& it : d.observers_NtCreateThreadEx)
            it(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
    }

    static void on_NtCreateThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle     = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ProcessHandle    = arg<nt::HANDLE>(d.core, 3);
        const auto ClientId         = arg<nt::PCLIENT_ID>(d.core, 4);
        const auto ThreadContext    = arg<nt::PCONTEXT>(d.core, 5);
        const auto InitialTeb       = arg<nt::PINITIAL_TEB>(d.core, 6);
        const auto CreateSuspended  = arg<nt::BOOLEAN>(d.core, 7);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateThread(ThreadHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ProcessHandle:{:#x}, ClientId:{:#x}, ThreadContext:{:#x}, InitialTeb:{:#x}, CreateSuspended:{:#x})", ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);

        for(const auto& it : d.observers_NtCreateThread)
            it(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);
    }

    static void on_NtCreateTimer(monitor::syscalls::Data& d)
    {
        const auto TimerHandle      = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto TimerType        = arg<nt::TIMER_TYPE>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateTimer(TimerHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, TimerType:{:#x})", TimerHandle, DesiredAccess, ObjectAttributes, TimerType);

        for(const auto& it : d.observers_NtCreateTimer)
            it(TimerHandle, DesiredAccess, ObjectAttributes, TimerType);
    }

    static void on_NtCreateToken(monitor::syscalls::Data& d)
    {
        const auto TokenHandle      = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto TokenType        = arg<nt::TOKEN_TYPE>(d.core, 3);
        const auto AuthenticationId = arg<nt::PLUID>(d.core, 4);
        const auto ExpirationTime   = arg<nt::PLARGE_INTEGER>(d.core, 5);
        const auto User             = arg<nt::PTOKEN_USER>(d.core, 6);
        const auto Groups           = arg<nt::PTOKEN_GROUPS>(d.core, 7);
        const auto Privileges       = arg<nt::PTOKEN_PRIVILEGES>(d.core, 8);
        const auto Owner            = arg<nt::PTOKEN_OWNER>(d.core, 9);
        const auto PrimaryGroup     = arg<nt::PTOKEN_PRIMARY_GROUP>(d.core, 10);
        const auto DefaultDacl      = arg<nt::PTOKEN_DEFAULT_DACL>(d.core, 11);
        const auto TokenSource      = arg<nt::PTOKEN_SOURCE>(d.core, 12);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateToken(TokenHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, TokenType:{:#x}, AuthenticationId:{:#x}, ExpirationTime:{:#x}, User:{:#x}, Groups:{:#x}, Privileges:{:#x}, Owner:{:#x}, PrimaryGroup:{:#x}, DefaultDacl:{:#x}, TokenSource:{:#x})", TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource);

        for(const auto& it : d.observers_NtCreateToken)
            it(TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource);
    }

    static void on_NtCreateTransactionManager(monitor::syscalls::Data& d)
    {
        const auto TmHandle         = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto LogFileName      = arg<nt::PUNICODE_STRING>(d.core, 3);
        const auto CreateOptions    = arg<nt::ULONG>(d.core, 4);
        const auto CommitStrength   = arg<nt::ULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateTransactionManager(TmHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, LogFileName:{:#x}, CreateOptions:{:#x}, CommitStrength:{:#x})", TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength);

        for(const auto& it : d.observers_NtCreateTransactionManager)
            it(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength);
    }

    static void on_NtCreateTransaction(monitor::syscalls::Data& d)
    {
        const auto TransactionHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Uow               = arg<nt::LPGUID>(d.core, 3);
        const auto TmHandle          = arg<nt::HANDLE>(d.core, 4);
        const auto CreateOptions     = arg<nt::ULONG>(d.core, 5);
        const auto IsolationLevel    = arg<nt::ULONG>(d.core, 6);
        const auto IsolationFlags    = arg<nt::ULONG>(d.core, 7);
        const auto Timeout           = arg<nt::PLARGE_INTEGER>(d.core, 8);
        const auto Description       = arg<nt::PUNICODE_STRING>(d.core, 9);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateTransaction(TransactionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, Uow:{:#x}, TmHandle:{:#x}, CreateOptions:{:#x}, IsolationLevel:{:#x}, IsolationFlags:{:#x}, Timeout:{:#x}, Description:{:#x})", TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);

        for(const auto& it : d.observers_NtCreateTransaction)
            it(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);
    }

    static void on_NtCreateUserProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle           = arg<nt::PHANDLE>(d.core, 0);
        const auto ThreadHandle            = arg<nt::PHANDLE>(d.core, 1);
        const auto ProcessDesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 2);
        const auto ThreadDesiredAccess     = arg<nt::ACCESS_MASK>(d.core, 3);
        const auto ProcessObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 4);
        const auto ThreadObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 5);
        const auto ProcessFlags            = arg<nt::ULONG>(d.core, 6);
        const auto ThreadFlags             = arg<nt::ULONG>(d.core, 7);
        const auto ProcessParameters       = arg<nt::PRTL_USER_PROCESS_PARAMETERS>(d.core, 8);
        const auto CreateInfo              = arg<nt::PPROCESS_CREATE_INFO>(d.core, 9);
        const auto AttributeList           = arg<nt::PPROCESS_ATTRIBUTE_LIST>(d.core, 10);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateUserProcess(ProcessHandle:{:#x}, ThreadHandle:{:#x}, ProcessDesiredAccess:{:#x}, ThreadDesiredAccess:{:#x}, ProcessObjectAttributes:{:#x}, ThreadObjectAttributes:{:#x}, ProcessFlags:{:#x}, ThreadFlags:{:#x}, ProcessParameters:{:#x}, CreateInfo:{:#x}, AttributeList:{:#x})", ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);

        for(const auto& it : d.observers_NtCreateUserProcess)
            it(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
    }

    static void on_NtCreateWaitablePort(monitor::syscalls::Data& d)
    {
        const auto PortHandle              = arg<nt::PHANDLE>(d.core, 0);
        const auto ObjectAttributes        = arg<nt::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto MaxConnectionInfoLength = arg<nt::ULONG>(d.core, 2);
        const auto MaxMessageLength        = arg<nt::ULONG>(d.core, 3);
        const auto MaxPoolUsage            = arg<nt::ULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateWaitablePort(PortHandle:{:#x}, ObjectAttributes:{:#x}, MaxConnectionInfoLength:{:#x}, MaxMessageLength:{:#x}, MaxPoolUsage:{:#x})", PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);

        for(const auto& it : d.observers_NtCreateWaitablePort)
            it(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    }

    static void on_NtCreateWorkerFactory(monitor::syscalls::Data& d)
    {
        const auto WorkerFactoryHandleReturn = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess             = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes          = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto CompletionPortHandle      = arg<nt::HANDLE>(d.core, 3);
        const auto WorkerProcessHandle       = arg<nt::HANDLE>(d.core, 4);
        const auto StartRoutine              = arg<nt::PVOID>(d.core, 5);
        const auto StartParameter            = arg<nt::PVOID>(d.core, 6);
        const auto MaxThreadCount            = arg<nt::ULONG>(d.core, 7);
        const auto StackReserve              = arg<nt::SIZE_T>(d.core, 8);
        const auto StackCommit               = arg<nt::SIZE_T>(d.core, 9);

        if constexpr(g_debug)
            LOG(INFO, "NtCreateWorkerFactory(WorkerFactoryHandleReturn:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, CompletionPortHandle:{:#x}, WorkerProcessHandle:{:#x}, StartRoutine:{:#x}, StartParameter:{:#x}, MaxThreadCount:{:#x}, StackReserve:{:#x}, StackCommit:{:#x})", WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit);

        for(const auto& it : d.observers_NtCreateWorkerFactory)
            it(WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit);
    }

    static void on_NtDebugActiveProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto DebugObjectHandle = arg<nt::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtDebugActiveProcess(ProcessHandle:{:#x}, DebugObjectHandle:{:#x})", ProcessHandle, DebugObjectHandle);

        for(const auto& it : d.observers_NtDebugActiveProcess)
            it(ProcessHandle, DebugObjectHandle);
    }

    static void on_NtDebugContinue(monitor::syscalls::Data& d)
    {
        const auto DebugObjectHandle = arg<nt::HANDLE>(d.core, 0);
        const auto ClientId          = arg<nt::PCLIENT_ID>(d.core, 1);
        const auto ContinueStatus    = arg<nt::NTSTATUS>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtDebugContinue(DebugObjectHandle:{:#x}, ClientId:{:#x}, ContinueStatus:{:#x})", DebugObjectHandle, ClientId, ContinueStatus);

        for(const auto& it : d.observers_NtDebugContinue)
            it(DebugObjectHandle, ClientId, ContinueStatus);
    }

    static void on_NtDelayExecution(monitor::syscalls::Data& d)
    {
        const auto Alertable     = arg<nt::BOOLEAN>(d.core, 0);
        const auto DelayInterval = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtDelayExecution(Alertable:{:#x}, DelayInterval:{:#x})", Alertable, DelayInterval);

        for(const auto& it : d.observers_NtDelayExecution)
            it(Alertable, DelayInterval);
    }

    static void on_NtDeleteAtom(monitor::syscalls::Data& d)
    {
        const auto Atom = arg<nt::RTL_ATOM>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtDeleteAtom(Atom:{:#x})", Atom);

        for(const auto& it : d.observers_NtDeleteAtom)
            it(Atom);
    }

    static void on_NtDeleteBootEntry(monitor::syscalls::Data& d)
    {
        const auto Id = arg<nt::ULONG>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtDeleteBootEntry(Id:{:#x})", Id);

        for(const auto& it : d.observers_NtDeleteBootEntry)
            it(Id);
    }

    static void on_NtDeleteDriverEntry(monitor::syscalls::Data& d)
    {
        const auto Id = arg<nt::ULONG>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtDeleteDriverEntry(Id:{:#x})", Id);

        for(const auto& it : d.observers_NtDeleteDriverEntry)
            it(Id);
    }

    static void on_NtDeleteFile(monitor::syscalls::Data& d)
    {
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtDeleteFile(ObjectAttributes:{:#x})", ObjectAttributes);

        for(const auto& it : d.observers_NtDeleteFile)
            it(ObjectAttributes);
    }

    static void on_NtDeleteKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtDeleteKey(KeyHandle:{:#x})", KeyHandle);

        for(const auto& it : d.observers_NtDeleteKey)
            it(KeyHandle);
    }

    static void on_NtDeleteObjectAuditAlarm(monitor::syscalls::Data& d)
    {
        const auto SubsystemName   = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto HandleId        = arg<nt::PVOID>(d.core, 1);
        const auto GenerateOnClose = arg<nt::BOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtDeleteObjectAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, GenerateOnClose);

        for(const auto& it : d.observers_NtDeleteObjectAuditAlarm)
            it(SubsystemName, HandleId, GenerateOnClose);
    }

    static void on_NtDeletePrivateNamespace(monitor::syscalls::Data& d)
    {
        const auto NamespaceHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtDeletePrivateNamespace(NamespaceHandle:{:#x})", NamespaceHandle);

        for(const auto& it : d.observers_NtDeletePrivateNamespace)
            it(NamespaceHandle);
    }

    static void on_NtDeleteValueKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle = arg<nt::HANDLE>(d.core, 0);
        const auto ValueName = arg<nt::PUNICODE_STRING>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtDeleteValueKey(KeyHandle:{:#x}, ValueName:{:#x})", KeyHandle, ValueName);

        for(const auto& it : d.observers_NtDeleteValueKey)
            it(KeyHandle, ValueName);
    }

    static void on_NtDeviceIoControlFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle         = arg<nt::HANDLE>(d.core, 0);
        const auto Event              = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine         = arg<nt::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext         = arg<nt::PVOID>(d.core, 3);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(d.core, 4);
        const auto IoControlCode      = arg<nt::ULONG>(d.core, 5);
        const auto InputBuffer        = arg<nt::PVOID>(d.core, 6);
        const auto InputBufferLength  = arg<nt::ULONG>(d.core, 7);
        const auto OutputBuffer       = arg<nt::PVOID>(d.core, 8);
        const auto OutputBufferLength = arg<nt::ULONG>(d.core, 9);

        if constexpr(g_debug)
            LOG(INFO, "NtDeviceIoControlFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, IoControlCode:{:#x}, InputBuffer:{:#x}, InputBufferLength:{:#x}, OutputBuffer:{:#x}, OutputBufferLength:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);

        for(const auto& it : d.observers_NtDeviceIoControlFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    }

    static void on_NtDisplayString(monitor::syscalls::Data& d)
    {
        const auto String = arg<nt::PUNICODE_STRING>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtDisplayString(String:{:#x})", String);

        for(const auto& it : d.observers_NtDisplayString)
            it(String);
    }

    static void on_NtDrawText(monitor::syscalls::Data& d)
    {
        const auto Text = arg<nt::PUNICODE_STRING>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtDrawText(Text:{:#x})", Text);

        for(const auto& it : d.observers_NtDrawText)
            it(Text);
    }

    static void on_NtDuplicateObject(monitor::syscalls::Data& d)
    {
        const auto SourceProcessHandle = arg<nt::HANDLE>(d.core, 0);
        const auto SourceHandle        = arg<nt::HANDLE>(d.core, 1);
        const auto TargetProcessHandle = arg<nt::HANDLE>(d.core, 2);
        const auto TargetHandle        = arg<nt::PHANDLE>(d.core, 3);
        const auto DesiredAccess       = arg<nt::ACCESS_MASK>(d.core, 4);
        const auto HandleAttributes    = arg<nt::ULONG>(d.core, 5);
        const auto Options             = arg<nt::ULONG>(d.core, 6);

        if constexpr(g_debug)
            LOG(INFO, "NtDuplicateObject(SourceProcessHandle:{:#x}, SourceHandle:{:#x}, TargetProcessHandle:{:#x}, TargetHandle:{:#x}, DesiredAccess:{:#x}, HandleAttributes:{:#x}, Options:{:#x})", SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);

        for(const auto& it : d.observers_NtDuplicateObject)
            it(SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);
    }

    static void on_NtDuplicateToken(monitor::syscalls::Data& d)
    {
        const auto ExistingTokenHandle = arg<nt::HANDLE>(d.core, 0);
        const auto DesiredAccess       = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes    = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto EffectiveOnly       = arg<nt::BOOLEAN>(d.core, 3);
        const auto TokenType           = arg<nt::TOKEN_TYPE>(d.core, 4);
        const auto NewTokenHandle      = arg<nt::PHANDLE>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtDuplicateToken(ExistingTokenHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, EffectiveOnly:{:#x}, TokenType:{:#x}, NewTokenHandle:{:#x})", ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle);

        for(const auto& it : d.observers_NtDuplicateToken)
            it(ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle);
    }

    static void on_NtEnumerateBootEntries(monitor::syscalls::Data& d)
    {
        const auto Buffer       = arg<nt::PVOID>(d.core, 0);
        const auto BufferLength = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtEnumerateBootEntries(Buffer:{:#x}, BufferLength:{:#x})", Buffer, BufferLength);

        for(const auto& it : d.observers_NtEnumerateBootEntries)
            it(Buffer, BufferLength);
    }

    static void on_NtEnumerateDriverEntries(monitor::syscalls::Data& d)
    {
        const auto Buffer       = arg<nt::PVOID>(d.core, 0);
        const auto BufferLength = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtEnumerateDriverEntries(Buffer:{:#x}, BufferLength:{:#x})", Buffer, BufferLength);

        for(const auto& it : d.observers_NtEnumerateDriverEntries)
            it(Buffer, BufferLength);
    }

    static void on_NtEnumerateKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto Index               = arg<nt::ULONG>(d.core, 1);
        const auto KeyInformationClass = arg<nt::KEY_INFORMATION_CLASS>(d.core, 2);
        const auto KeyInformation      = arg<nt::PVOID>(d.core, 3);
        const auto Length              = arg<nt::ULONG>(d.core, 4);
        const auto ResultLength        = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtEnumerateKey(KeyHandle:{:#x}, Index:{:#x}, KeyInformationClass:{:#x}, KeyInformation:{:#x}, Length:{:#x}, ResultLength:{:#x})", KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);

        for(const auto& it : d.observers_NtEnumerateKey)
            it(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
    }

    static void on_NtEnumerateSystemEnvironmentValuesEx(monitor::syscalls::Data& d)
    {
        const auto InformationClass = arg<nt::ULONG>(d.core, 0);
        const auto Buffer           = arg<nt::PVOID>(d.core, 1);
        const auto BufferLength     = arg<nt::PULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtEnumerateSystemEnvironmentValuesEx(InformationClass:{:#x}, Buffer:{:#x}, BufferLength:{:#x})", InformationClass, Buffer, BufferLength);

        for(const auto& it : d.observers_NtEnumerateSystemEnvironmentValuesEx)
            it(InformationClass, Buffer, BufferLength);
    }

    static void on_NtEnumerateTransactionObject(monitor::syscalls::Data& d)
    {
        const auto RootObjectHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto QueryType          = arg<nt::KTMOBJECT_TYPE>(d.core, 1);
        const auto ObjectCursor       = arg<nt::PKTMOBJECT_CURSOR>(d.core, 2);
        const auto ObjectCursorLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength       = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtEnumerateTransactionObject(RootObjectHandle:{:#x}, QueryType:{:#x}, ObjectCursor:{:#x}, ObjectCursorLength:{:#x}, ReturnLength:{:#x})", RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength);

        for(const auto& it : d.observers_NtEnumerateTransactionObject)
            it(RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength);
    }

    static void on_NtEnumerateValueKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle                = arg<nt::HANDLE>(d.core, 0);
        const auto Index                    = arg<nt::ULONG>(d.core, 1);
        const auto KeyValueInformationClass = arg<nt::KEY_VALUE_INFORMATION_CLASS>(d.core, 2);
        const auto KeyValueInformation      = arg<nt::PVOID>(d.core, 3);
        const auto Length                   = arg<nt::ULONG>(d.core, 4);
        const auto ResultLength             = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtEnumerateValueKey(KeyHandle:{:#x}, Index:{:#x}, KeyValueInformationClass:{:#x}, KeyValueInformation:{:#x}, Length:{:#x}, ResultLength:{:#x})", KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);

        for(const auto& it : d.observers_NtEnumerateValueKey)
            it(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    }

    static void on_NtExtendSection(monitor::syscalls::Data& d)
    {
        const auto SectionHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto NewSectionSize = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtExtendSection(SectionHandle:{:#x}, NewSectionSize:{:#x})", SectionHandle, NewSectionSize);

        for(const auto& it : d.observers_NtExtendSection)
            it(SectionHandle, NewSectionSize);
    }

    static void on_NtFilterToken(monitor::syscalls::Data& d)
    {
        const auto ExistingTokenHandle = arg<nt::HANDLE>(d.core, 0);
        const auto Flags               = arg<nt::ULONG>(d.core, 1);
        const auto SidsToDisable       = arg<nt::PTOKEN_GROUPS>(d.core, 2);
        const auto PrivilegesToDelete  = arg<nt::PTOKEN_PRIVILEGES>(d.core, 3);
        const auto RestrictedSids      = arg<nt::PTOKEN_GROUPS>(d.core, 4);
        const auto NewTokenHandle      = arg<nt::PHANDLE>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtFilterToken(ExistingTokenHandle:{:#x}, Flags:{:#x}, SidsToDisable:{:#x}, PrivilegesToDelete:{:#x}, RestrictedSids:{:#x}, NewTokenHandle:{:#x})", ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle);

        for(const auto& it : d.observers_NtFilterToken)
            it(ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle);
    }

    static void on_NtFindAtom(monitor::syscalls::Data& d)
    {
        const auto AtomName = arg<nt::PWSTR>(d.core, 0);
        const auto Length   = arg<nt::ULONG>(d.core, 1);
        const auto Atom     = arg<nt::PRTL_ATOM>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtFindAtom(AtomName:{:#x}, Length:{:#x}, Atom:{:#x})", AtomName, Length, Atom);

        for(const auto& it : d.observers_NtFindAtom)
            it(AtomName, Length, Atom);
    }

    static void on_NtFlushBuffersFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtFlushBuffersFile(FileHandle:{:#x}, IoStatusBlock:{:#x})", FileHandle, IoStatusBlock);

        for(const auto& it : d.observers_NtFlushBuffersFile)
            it(FileHandle, IoStatusBlock);
    }

    static void on_NtFlushInstallUILanguage(monitor::syscalls::Data& d)
    {
        const auto InstallUILanguage = arg<nt::LANGID>(d.core, 0);
        const auto SetComittedFlag   = arg<nt::ULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtFlushInstallUILanguage(InstallUILanguage:{:#x}, SetComittedFlag:{:#x})", InstallUILanguage, SetComittedFlag);

        for(const auto& it : d.observers_NtFlushInstallUILanguage)
            it(InstallUILanguage, SetComittedFlag);
    }

    static void on_NtFlushInstructionCache(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 0);
        const auto BaseAddress   = arg<nt::PVOID>(d.core, 1);
        const auto Length        = arg<nt::SIZE_T>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtFlushInstructionCache(ProcessHandle:{:#x}, BaseAddress:{:#x}, Length:{:#x})", ProcessHandle, BaseAddress, Length);

        for(const auto& it : d.observers_NtFlushInstructionCache)
            it(ProcessHandle, BaseAddress, Length);
    }

    static void on_NtFlushKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtFlushKey(KeyHandle:{:#x})", KeyHandle);

        for(const auto& it : d.observers_NtFlushKey)
            it(KeyHandle);
    }

    static void on_NtFlushVirtualMemory(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(d.core, 1);
        const auto RegionSize      = arg<nt::PSIZE_T>(d.core, 2);
        const auto IoStatus        = arg<nt::PIO_STATUS_BLOCK>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtFlushVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, RegionSize:{:#x}, IoStatus:{:#x})", ProcessHandle, STARBaseAddress, RegionSize, IoStatus);

        for(const auto& it : d.observers_NtFlushVirtualMemory)
            it(ProcessHandle, STARBaseAddress, RegionSize, IoStatus);
    }

    static void on_NtFreeUserPhysicalPages(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 0);
        const auto NumberOfPages = arg<nt::PULONG_PTR>(d.core, 1);
        const auto UserPfnArra   = arg<nt::PULONG_PTR>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtFreeUserPhysicalPages(ProcessHandle:{:#x}, NumberOfPages:{:#x}, UserPfnArra:{:#x})", ProcessHandle, NumberOfPages, UserPfnArra);

        for(const auto& it : d.observers_NtFreeUserPhysicalPages)
            it(ProcessHandle, NumberOfPages, UserPfnArra);
    }

    static void on_NtFreeVirtualMemory(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(d.core, 1);
        const auto RegionSize      = arg<nt::PSIZE_T>(d.core, 2);
        const auto FreeType        = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtFreeVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, RegionSize:{:#x}, FreeType:{:#x})", ProcessHandle, STARBaseAddress, RegionSize, FreeType);

        for(const auto& it : d.observers_NtFreeVirtualMemory)
            it(ProcessHandle, STARBaseAddress, RegionSize, FreeType);
    }

    static void on_NtFreezeRegistry(monitor::syscalls::Data& d)
    {
        const auto TimeOutInSeconds = arg<nt::ULONG>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtFreezeRegistry(TimeOutInSeconds:{:#x})", TimeOutInSeconds);

        for(const auto& it : d.observers_NtFreezeRegistry)
            it(TimeOutInSeconds);
    }

    static void on_NtFreezeTransactions(monitor::syscalls::Data& d)
    {
        const auto FreezeTimeout = arg<nt::PLARGE_INTEGER>(d.core, 0);
        const auto ThawTimeout   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtFreezeTransactions(FreezeTimeout:{:#x}, ThawTimeout:{:#x})", FreezeTimeout, ThawTimeout);

        for(const auto& it : d.observers_NtFreezeTransactions)
            it(FreezeTimeout, ThawTimeout);
    }

    static void on_NtFsControlFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle         = arg<nt::HANDLE>(d.core, 0);
        const auto Event              = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine         = arg<nt::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext         = arg<nt::PVOID>(d.core, 3);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(d.core, 4);
        const auto IoControlCode      = arg<nt::ULONG>(d.core, 5);
        const auto InputBuffer        = arg<nt::PVOID>(d.core, 6);
        const auto InputBufferLength  = arg<nt::ULONG>(d.core, 7);
        const auto OutputBuffer       = arg<nt::PVOID>(d.core, 8);
        const auto OutputBufferLength = arg<nt::ULONG>(d.core, 9);

        if constexpr(g_debug)
            LOG(INFO, "NtFsControlFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, IoControlCode:{:#x}, InputBuffer:{:#x}, InputBufferLength:{:#x}, OutputBuffer:{:#x}, OutputBufferLength:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);

        for(const auto& it : d.observers_NtFsControlFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    }

    static void on_NtGetContextThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto ThreadContext = arg<nt::PCONTEXT>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtGetContextThread(ThreadHandle:{:#x}, ThreadContext:{:#x})", ThreadHandle, ThreadContext);

        for(const auto& it : d.observers_NtGetContextThread)
            it(ThreadHandle, ThreadContext);
    }

    static void on_NtGetDevicePowerState(monitor::syscalls::Data& d)
    {
        const auto Device    = arg<nt::HANDLE>(d.core, 0);
        const auto STARState = arg<nt::DEVICE_POWER_STATE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtGetDevicePowerState(Device:{:#x}, STARState:{:#x})", Device, STARState);

        for(const auto& it : d.observers_NtGetDevicePowerState)
            it(Device, STARState);
    }

    static void on_NtGetMUIRegistryInfo(monitor::syscalls::Data& d)
    {
        const auto Flags    = arg<nt::ULONG>(d.core, 0);
        const auto DataSize = arg<nt::PULONG>(d.core, 1);
        const auto Data     = arg<nt::PVOID>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtGetMUIRegistryInfo(Flags:{:#x}, DataSize:{:#x}, Data:{:#x})", Flags, DataSize, Data);

        for(const auto& it : d.observers_NtGetMUIRegistryInfo)
            it(Flags, DataSize, Data);
    }

    static void on_NtGetNextProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto HandleAttributes = arg<nt::ULONG>(d.core, 2);
        const auto Flags            = arg<nt::ULONG>(d.core, 3);
        const auto NewProcessHandle = arg<nt::PHANDLE>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtGetNextProcess(ProcessHandle:{:#x}, DesiredAccess:{:#x}, HandleAttributes:{:#x}, Flags:{:#x}, NewProcessHandle:{:#x})", ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);

        for(const auto& it : d.observers_NtGetNextProcess)
            it(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
    }

    static void on_NtGetNextThread(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto ThreadHandle     = arg<nt::HANDLE>(d.core, 1);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 2);
        const auto HandleAttributes = arg<nt::ULONG>(d.core, 3);
        const auto Flags            = arg<nt::ULONG>(d.core, 4);
        const auto NewThreadHandle  = arg<nt::PHANDLE>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtGetNextThread(ProcessHandle:{:#x}, ThreadHandle:{:#x}, DesiredAccess:{:#x}, HandleAttributes:{:#x}, Flags:{:#x}, NewThreadHandle:{:#x})", ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);

        for(const auto& it : d.observers_NtGetNextThread)
            it(ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);
    }

    static void on_NtGetNlsSectionPtr(monitor::syscalls::Data& d)
    {
        const auto SectionType        = arg<nt::ULONG>(d.core, 0);
        const auto SectionData        = arg<nt::ULONG>(d.core, 1);
        const auto ContextData        = arg<nt::PVOID>(d.core, 2);
        const auto STARSectionPointer = arg<nt::PVOID>(d.core, 3);
        const auto SectionSize        = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtGetNlsSectionPtr(SectionType:{:#x}, SectionData:{:#x}, ContextData:{:#x}, STARSectionPointer:{:#x}, SectionSize:{:#x})", SectionType, SectionData, ContextData, STARSectionPointer, SectionSize);

        for(const auto& it : d.observers_NtGetNlsSectionPtr)
            it(SectionType, SectionData, ContextData, STARSectionPointer, SectionSize);
    }

    static void on_NtGetNotificationResourceManager(monitor::syscalls::Data& d)
    {
        const auto ResourceManagerHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto TransactionNotification = arg<nt::PTRANSACTION_NOTIFICATION>(d.core, 1);
        const auto NotificationLength      = arg<nt::ULONG>(d.core, 2);
        const auto Timeout                 = arg<nt::PLARGE_INTEGER>(d.core, 3);
        const auto ReturnLength            = arg<nt::PULONG>(d.core, 4);
        const auto Asynchronous            = arg<nt::ULONG>(d.core, 5);
        const auto AsynchronousContext     = arg<nt::ULONG_PTR>(d.core, 6);

        if constexpr(g_debug)
            LOG(INFO, "NtGetNotificationResourceManager(ResourceManagerHandle:{:#x}, TransactionNotification:{:#x}, NotificationLength:{:#x}, Timeout:{:#x}, ReturnLength:{:#x}, Asynchronous:{:#x}, AsynchronousContext:{:#x})", ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext);

        for(const auto& it : d.observers_NtGetNotificationResourceManager)
            it(ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext);
    }

    static void on_NtGetPlugPlayEvent(monitor::syscalls::Data& d)
    {
        const auto EventHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto Context         = arg<nt::PVOID>(d.core, 1);
        const auto EventBlock      = arg<nt::PPLUGPLAY_EVENT_BLOCK>(d.core, 2);
        const auto EventBufferSize = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtGetPlugPlayEvent(EventHandle:{:#x}, Context:{:#x}, EventBlock:{:#x}, EventBufferSize:{:#x})", EventHandle, Context, EventBlock, EventBufferSize);

        for(const auto& it : d.observers_NtGetPlugPlayEvent)
            it(EventHandle, Context, EventBlock, EventBufferSize);
    }

    static void on_NtGetWriteWatch(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle             = arg<nt::HANDLE>(d.core, 0);
        const auto Flags                     = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress               = arg<nt::PVOID>(d.core, 2);
        const auto RegionSize                = arg<nt::SIZE_T>(d.core, 3);
        const auto STARUserAddressArray      = arg<nt::PVOID>(d.core, 4);
        const auto EntriesInUserAddressArray = arg<nt::PULONG_PTR>(d.core, 5);
        const auto Granularity               = arg<nt::PULONG>(d.core, 6);

        if constexpr(g_debug)
            LOG(INFO, "NtGetWriteWatch(ProcessHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x}, RegionSize:{:#x}, STARUserAddressArray:{:#x}, EntriesInUserAddressArray:{:#x}, Granularity:{:#x})", ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity);

        for(const auto& it : d.observers_NtGetWriteWatch)
            it(ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity);
    }

    static void on_NtImpersonateAnonymousToken(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtImpersonateAnonymousToken(ThreadHandle:{:#x})", ThreadHandle);

        for(const auto& it : d.observers_NtImpersonateAnonymousToken)
            it(ThreadHandle);
    }

    static void on_NtImpersonateClientOfPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle = arg<nt::HANDLE>(d.core, 0);
        const auto Message    = arg<nt::PPORT_MESSAGE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtImpersonateClientOfPort(PortHandle:{:#x}, Message:{:#x})", PortHandle, Message);

        for(const auto& it : d.observers_NtImpersonateClientOfPort)
            it(PortHandle, Message);
    }

    static void on_NtImpersonateThread(monitor::syscalls::Data& d)
    {
        const auto ServerThreadHandle = arg<nt::HANDLE>(d.core, 0);
        const auto ClientThreadHandle = arg<nt::HANDLE>(d.core, 1);
        const auto SecurityQos        = arg<nt::PSECURITY_QUALITY_OF_SERVICE>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtImpersonateThread(ServerThreadHandle:{:#x}, ClientThreadHandle:{:#x}, SecurityQos:{:#x})", ServerThreadHandle, ClientThreadHandle, SecurityQos);

        for(const auto& it : d.observers_NtImpersonateThread)
            it(ServerThreadHandle, ClientThreadHandle, SecurityQos);
    }

    static void on_NtInitializeNlsFiles(monitor::syscalls::Data& d)
    {
        const auto STARBaseAddress        = arg<nt::PVOID>(d.core, 0);
        const auto DefaultLocaleId        = arg<nt::PLCID>(d.core, 1);
        const auto DefaultCasingTableSize = arg<nt::PLARGE_INTEGER>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtInitializeNlsFiles(STARBaseAddress:{:#x}, DefaultLocaleId:{:#x}, DefaultCasingTableSize:{:#x})", STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize);

        for(const auto& it : d.observers_NtInitializeNlsFiles)
            it(STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize);
    }

    static void on_NtInitializeRegistry(monitor::syscalls::Data& d)
    {
        const auto BootCondition = arg<nt::USHORT>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtInitializeRegistry(BootCondition:{:#x})", BootCondition);

        for(const auto& it : d.observers_NtInitializeRegistry)
            it(BootCondition);
    }

    static void on_NtInitiatePowerAction(monitor::syscalls::Data& d)
    {
        const auto SystemAction   = arg<nt::POWER_ACTION>(d.core, 0);
        const auto MinSystemState = arg<nt::SYSTEM_POWER_STATE>(d.core, 1);
        const auto Flags          = arg<nt::ULONG>(d.core, 2);
        const auto Asynchronous   = arg<nt::BOOLEAN>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtInitiatePowerAction(SystemAction:{:#x}, MinSystemState:{:#x}, Flags:{:#x}, Asynchronous:{:#x})", SystemAction, MinSystemState, Flags, Asynchronous);

        for(const auto& it : d.observers_NtInitiatePowerAction)
            it(SystemAction, MinSystemState, Flags, Asynchronous);
    }

    static void on_NtIsProcessInJob(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 0);
        const auto JobHandle     = arg<nt::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtIsProcessInJob(ProcessHandle:{:#x}, JobHandle:{:#x})", ProcessHandle, JobHandle);

        for(const auto& it : d.observers_NtIsProcessInJob)
            it(ProcessHandle, JobHandle);
    }

    static void on_NtListenPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto ConnectionRequest = arg<nt::PPORT_MESSAGE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtListenPort(PortHandle:{:#x}, ConnectionRequest:{:#x})", PortHandle, ConnectionRequest);

        for(const auto& it : d.observers_NtListenPort)
            it(PortHandle, ConnectionRequest);
    }

    static void on_NtLoadDriver(monitor::syscalls::Data& d)
    {
        const auto DriverServiceName = arg<nt::PUNICODE_STRING>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtLoadDriver(DriverServiceName:{:#x})", DriverServiceName);

        for(const auto& it : d.observers_NtLoadDriver)
            it(DriverServiceName);
    }

    static void on_NtLoadKey2(monitor::syscalls::Data& d)
    {
        const auto TargetKey  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto SourceFile = arg<nt::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto Flags      = arg<nt::ULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtLoadKey2(TargetKey:{:#x}, SourceFile:{:#x}, Flags:{:#x})", TargetKey, SourceFile, Flags);

        for(const auto& it : d.observers_NtLoadKey2)
            it(TargetKey, SourceFile, Flags);
    }

    static void on_NtLoadKeyEx(monitor::syscalls::Data& d)
    {
        const auto TargetKey     = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto SourceFile    = arg<nt::POBJECT_ATTRIBUTES>(d.core, 1);
        const auto Flags         = arg<nt::ULONG>(d.core, 2);
        const auto TrustClassKey = arg<nt::HANDLE>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtLoadKeyEx(TargetKey:{:#x}, SourceFile:{:#x}, Flags:{:#x}, TrustClassKey:{:#x})", TargetKey, SourceFile, Flags, TrustClassKey);

        for(const auto& it : d.observers_NtLoadKeyEx)
            it(TargetKey, SourceFile, Flags, TrustClassKey);
    }

    static void on_NtLoadKey(monitor::syscalls::Data& d)
    {
        const auto TargetKey  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto SourceFile = arg<nt::POBJECT_ATTRIBUTES>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtLoadKey(TargetKey:{:#x}, SourceFile:{:#x})", TargetKey, SourceFile);

        for(const auto& it : d.observers_NtLoadKey)
            it(TargetKey, SourceFile);
    }

    static void on_NtLockFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle      = arg<nt::HANDLE>(d.core, 0);
        const auto Event           = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine      = arg<nt::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext      = arg<nt::PVOID>(d.core, 3);
        const auto IoStatusBlock   = arg<nt::PIO_STATUS_BLOCK>(d.core, 4);
        const auto ByteOffset      = arg<nt::PLARGE_INTEGER>(d.core, 5);
        const auto Length          = arg<nt::PLARGE_INTEGER>(d.core, 6);
        const auto Key             = arg<nt::ULONG>(d.core, 7);
        const auto FailImmediately = arg<nt::BOOLEAN>(d.core, 8);
        const auto ExclusiveLock   = arg<nt::BOOLEAN>(d.core, 9);

        if constexpr(g_debug)
            LOG(INFO, "NtLockFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, ByteOffset:{:#x}, Length:{:#x}, Key:{:#x}, FailImmediately:{:#x}, ExclusiveLock:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock);

        for(const auto& it : d.observers_NtLockFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock);
    }

    static void on_NtLockProductActivationKeys(monitor::syscalls::Data& d)
    {
        const auto STARpPrivateVer = arg<nt::ULONG>(d.core, 0);
        const auto STARpSafeMode   = arg<nt::ULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtLockProductActivationKeys(STARpPrivateVer:{:#x}, STARpSafeMode:{:#x})", STARpPrivateVer, STARpSafeMode);

        for(const auto& it : d.observers_NtLockProductActivationKeys)
            it(STARpPrivateVer, STARpSafeMode);
    }

    static void on_NtLockRegistryKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtLockRegistryKey(KeyHandle:{:#x})", KeyHandle);

        for(const auto& it : d.observers_NtLockRegistryKey)
            it(KeyHandle);
    }

    static void on_NtLockVirtualMemory(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(d.core, 1);
        const auto RegionSize      = arg<nt::PSIZE_T>(d.core, 2);
        const auto MapType         = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtLockVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, RegionSize:{:#x}, MapType:{:#x})", ProcessHandle, STARBaseAddress, RegionSize, MapType);

        for(const auto& it : d.observers_NtLockVirtualMemory)
            it(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    }

    static void on_NtMakePermanentObject(monitor::syscalls::Data& d)
    {
        const auto Handle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtMakePermanentObject(Handle:{:#x})", Handle);

        for(const auto& it : d.observers_NtMakePermanentObject)
            it(Handle);
    }

    static void on_NtMakeTemporaryObject(monitor::syscalls::Data& d)
    {
        const auto Handle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtMakeTemporaryObject(Handle:{:#x})", Handle);

        for(const auto& it : d.observers_NtMakeTemporaryObject)
            it(Handle);
    }

    static void on_NtMapCMFModule(monitor::syscalls::Data& d)
    {
        const auto What            = arg<nt::ULONG>(d.core, 0);
        const auto Index           = arg<nt::ULONG>(d.core, 1);
        const auto CacheIndexOut   = arg<nt::PULONG>(d.core, 2);
        const auto CacheFlagsOut   = arg<nt::PULONG>(d.core, 3);
        const auto ViewSizeOut     = arg<nt::PULONG>(d.core, 4);
        const auto STARBaseAddress = arg<nt::PVOID>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtMapCMFModule(What:{:#x}, Index:{:#x}, CacheIndexOut:{:#x}, CacheFlagsOut:{:#x}, ViewSizeOut:{:#x}, STARBaseAddress:{:#x})", What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress);

        for(const auto& it : d.observers_NtMapCMFModule)
            it(What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress);
    }

    static void on_NtMapUserPhysicalPages(monitor::syscalls::Data& d)
    {
        const auto VirtualAddress = arg<nt::PVOID>(d.core, 0);
        const auto NumberOfPages  = arg<nt::ULONG_PTR>(d.core, 1);
        const auto UserPfnArra    = arg<nt::PULONG_PTR>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtMapUserPhysicalPages(VirtualAddress:{:#x}, NumberOfPages:{:#x}, UserPfnArra:{:#x})", VirtualAddress, NumberOfPages, UserPfnArra);

        for(const auto& it : d.observers_NtMapUserPhysicalPages)
            it(VirtualAddress, NumberOfPages, UserPfnArra);
    }

    static void on_NtMapUserPhysicalPagesScatter(monitor::syscalls::Data& d)
    {
        const auto STARVirtualAddresses = arg<nt::PVOID>(d.core, 0);
        const auto NumberOfPages        = arg<nt::ULONG_PTR>(d.core, 1);
        const auto UserPfnArray         = arg<nt::PULONG_PTR>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtMapUserPhysicalPagesScatter(STARVirtualAddresses:{:#x}, NumberOfPages:{:#x}, UserPfnArray:{:#x})", STARVirtualAddresses, NumberOfPages, UserPfnArray);

        for(const auto& it : d.observers_NtMapUserPhysicalPagesScatter)
            it(STARVirtualAddresses, NumberOfPages, UserPfnArray);
    }

    static void on_NtMapViewOfSection(monitor::syscalls::Data& d)
    {
        const auto SectionHandle      = arg<nt::HANDLE>(d.core, 0);
        const auto ProcessHandle      = arg<nt::HANDLE>(d.core, 1);
        const auto STARBaseAddress    = arg<nt::PVOID>(d.core, 2);
        const auto ZeroBits           = arg<nt::ULONG_PTR>(d.core, 3);
        const auto CommitSize         = arg<nt::SIZE_T>(d.core, 4);
        const auto SectionOffset      = arg<nt::PLARGE_INTEGER>(d.core, 5);
        const auto ViewSize           = arg<nt::PSIZE_T>(d.core, 6);
        const auto InheritDisposition = arg<nt::SECTION_INHERIT>(d.core, 7);
        const auto AllocationType     = arg<nt::ULONG>(d.core, 8);
        const auto Win32Protect       = arg<nt::WIN32_PROTECTION_MASK>(d.core, 9);

        if constexpr(g_debug)
            LOG(INFO, "NtMapViewOfSection(SectionHandle:{:#x}, ProcessHandle:{:#x}, STARBaseAddress:{:#x}, ZeroBits:{:#x}, CommitSize:{:#x}, SectionOffset:{:#x}, ViewSize:{:#x}, InheritDisposition:{:#x}, AllocationType:{:#x}, Win32Protect:{:#x})", SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);

        for(const auto& it : d.observers_NtMapViewOfSection)
            it(SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
    }

    static void on_NtModifyBootEntry(monitor::syscalls::Data& d)
    {
        const auto BootEntry = arg<nt::PBOOT_ENTRY>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtModifyBootEntry(BootEntry:{:#x})", BootEntry);

        for(const auto& it : d.observers_NtModifyBootEntry)
            it(BootEntry);
    }

    static void on_NtModifyDriverEntry(monitor::syscalls::Data& d)
    {
        const auto DriverEntry = arg<nt::PEFI_DRIVER_ENTRY>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtModifyDriverEntry(DriverEntry:{:#x})", DriverEntry);

        for(const auto& it : d.observers_NtModifyDriverEntry)
            it(DriverEntry);
    }

    static void on_NtNotifyChangeDirectoryFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle       = arg<nt::HANDLE>(d.core, 0);
        const auto Event            = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine       = arg<nt::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext       = arg<nt::PVOID>(d.core, 3);
        const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(d.core, 4);
        const auto Buffer           = arg<nt::PVOID>(d.core, 5);
        const auto Length           = arg<nt::ULONG>(d.core, 6);
        const auto CompletionFilter = arg<nt::ULONG>(d.core, 7);
        const auto WatchTree        = arg<nt::BOOLEAN>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtNotifyChangeDirectoryFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x}, CompletionFilter:{:#x}, WatchTree:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree);

        for(const auto& it : d.observers_NtNotifyChangeDirectoryFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree);
    }

    static void on_NtNotifyChangeKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto Event            = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine       = arg<nt::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext       = arg<nt::PVOID>(d.core, 3);
        const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(d.core, 4);
        const auto CompletionFilter = arg<nt::ULONG>(d.core, 5);
        const auto WatchTree        = arg<nt::BOOLEAN>(d.core, 6);
        const auto Buffer           = arg<nt::PVOID>(d.core, 7);
        const auto BufferSize       = arg<nt::ULONG>(d.core, 8);
        const auto Asynchronous     = arg<nt::BOOLEAN>(d.core, 9);

        if constexpr(g_debug)
            LOG(INFO, "NtNotifyChangeKey(KeyHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, CompletionFilter:{:#x}, WatchTree:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, Asynchronous:{:#x})", KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);

        for(const auto& it : d.observers_NtNotifyChangeKey)
            it(KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    }

    static void on_NtNotifyChangeMultipleKeys(monitor::syscalls::Data& d)
    {
        const auto MasterKeyHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto Count            = arg<nt::ULONG>(d.core, 1);
        const auto SlaveObjects     = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Event            = arg<nt::HANDLE>(d.core, 3);
        const auto ApcRoutine       = arg<nt::PIO_APC_ROUTINE>(d.core, 4);
        const auto ApcContext       = arg<nt::PVOID>(d.core, 5);
        const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(d.core, 6);
        const auto CompletionFilter = arg<nt::ULONG>(d.core, 7);
        const auto WatchTree        = arg<nt::BOOLEAN>(d.core, 8);
        const auto Buffer           = arg<nt::PVOID>(d.core, 9);
        const auto BufferSize       = arg<nt::ULONG>(d.core, 10);
        const auto Asynchronous     = arg<nt::BOOLEAN>(d.core, 11);

        if constexpr(g_debug)
            LOG(INFO, "NtNotifyChangeMultipleKeys(MasterKeyHandle:{:#x}, Count:{:#x}, SlaveObjects:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, CompletionFilter:{:#x}, WatchTree:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, Asynchronous:{:#x})", MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);

        for(const auto& it : d.observers_NtNotifyChangeMultipleKeys)
            it(MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    }

    static void on_NtNotifyChangeSession(monitor::syscalls::Data& d)
    {
        const auto Session         = arg<nt::HANDLE>(d.core, 0);
        const auto IoStateSequence = arg<nt::ULONG>(d.core, 1);
        const auto Reserved        = arg<nt::PVOID>(d.core, 2);
        const auto Action          = arg<nt::ULONG>(d.core, 3);
        const auto IoState         = arg<nt::IO_SESSION_STATE>(d.core, 4);
        const auto IoState2        = arg<nt::IO_SESSION_STATE>(d.core, 5);
        const auto Buffer          = arg<nt::PVOID>(d.core, 6);
        const auto BufferSize      = arg<nt::ULONG>(d.core, 7);

        if constexpr(g_debug)
            LOG(INFO, "NtNotifyChangeSession(Session:{:#x}, IoStateSequence:{:#x}, Reserved:{:#x}, Action:{:#x}, IoState:{:#x}, IoState2:{:#x}, Buffer:{:#x}, BufferSize:{:#x})", Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize);

        for(const auto& it : d.observers_NtNotifyChangeSession)
            it(Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize);
    }

    static void on_NtOpenDirectoryObject(monitor::syscalls::Data& d)
    {
        const auto DirectoryHandle  = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenDirectoryObject(DirectoryHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", DirectoryHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenDirectoryObject)
            it(DirectoryHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenEnlistment(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle      = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess         = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ResourceManagerHandle = arg<nt::HANDLE>(d.core, 2);
        const auto EnlistmentGuid        = arg<nt::LPGUID>(d.core, 3);
        const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenEnlistment(EnlistmentHandle:{:#x}, DesiredAccess:{:#x}, ResourceManagerHandle:{:#x}, EnlistmentGuid:{:#x}, ObjectAttributes:{:#x})", EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenEnlistment)
            it(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes);
    }

    static void on_NtOpenEvent(monitor::syscalls::Data& d)
    {
        const auto EventHandle      = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenEvent(EventHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", EventHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenEvent)
            it(EventHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenEventPair(monitor::syscalls::Data& d)
    {
        const auto EventPairHandle  = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenEventPair(EventPairHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", EventPairHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenEventPair)
            it(EventPairHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle       = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(d.core, 3);
        const auto ShareAccess      = arg<nt::ULONG>(d.core, 4);
        const auto OpenOptions      = arg<nt::ULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenFile(FileHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, IoStatusBlock:{:#x}, ShareAccess:{:#x}, OpenOptions:{:#x})", FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);

        for(const auto& it : d.observers_NtOpenFile)
            it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
    }

    static void on_NtOpenIoCompletion(monitor::syscalls::Data& d)
    {
        const auto IoCompletionHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenIoCompletion(IoCompletionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", IoCompletionHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenIoCompletion)
            it(IoCompletionHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenJobObject(monitor::syscalls::Data& d)
    {
        const auto JobHandle        = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenJobObject(JobHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", JobHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenJobObject)
            it(JobHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenKeyedEvent(monitor::syscalls::Data& d)
    {
        const auto KeyedEventHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenKeyedEvent(KeyedEventHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", KeyedEventHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenKeyedEvent)
            it(KeyedEventHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenKeyEx(monitor::syscalls::Data& d)
    {
        const auto KeyHandle        = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto OpenOptions      = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenKeyEx(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, OpenOptions:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);

        for(const auto& it : d.observers_NtOpenKeyEx)
            it(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
    }

    static void on_NtOpenKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle        = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenKey(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenKey)
            it(KeyHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenKeyTransactedEx(monitor::syscalls::Data& d)
    {
        const auto KeyHandle         = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto OpenOptions       = arg<nt::ULONG>(d.core, 3);
        const auto TransactionHandle = arg<nt::HANDLE>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenKeyTransactedEx(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, OpenOptions:{:#x}, TransactionHandle:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle);

        for(const auto& it : d.observers_NtOpenKeyTransactedEx)
            it(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle);
    }

    static void on_NtOpenKeyTransacted(monitor::syscalls::Data& d)
    {
        const auto KeyHandle         = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto TransactionHandle = arg<nt::HANDLE>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenKeyTransacted(KeyHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, TransactionHandle:{:#x})", KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle);

        for(const auto& it : d.observers_NtOpenKeyTransacted)
            it(KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle);
    }

    static void on_NtOpenMutant(monitor::syscalls::Data& d)
    {
        const auto MutantHandle     = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenMutant(MutantHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", MutantHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenMutant)
            it(MutantHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenObjectAuditAlarm(monitor::syscalls::Data& d)
    {
        const auto SubsystemName      = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto HandleId           = arg<nt::PVOID>(d.core, 1);
        const auto ObjectTypeName     = arg<nt::PUNICODE_STRING>(d.core, 2);
        const auto ObjectName         = arg<nt::PUNICODE_STRING>(d.core, 3);
        const auto SecurityDescriptor = arg<nt::PSECURITY_DESCRIPTOR>(d.core, 4);
        const auto ClientToken        = arg<nt::HANDLE>(d.core, 5);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(d.core, 6);
        const auto GrantedAccess      = arg<nt::ACCESS_MASK>(d.core, 7);
        const auto Privileges         = arg<nt::PPRIVILEGE_SET>(d.core, 8);
        const auto ObjectCreation     = arg<nt::BOOLEAN>(d.core, 9);
        const auto AccessGranted      = arg<nt::BOOLEAN>(d.core, 10);
        const auto GenerateOnClose    = arg<nt::PBOOLEAN>(d.core, 11);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenObjectAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, ObjectTypeName:{:#x}, ObjectName:{:#x}, SecurityDescriptor:{:#x}, ClientToken:{:#x}, DesiredAccess:{:#x}, GrantedAccess:{:#x}, Privileges:{:#x}, ObjectCreation:{:#x}, AccessGranted:{:#x}, GenerateOnClose:{:#x})", SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);

        for(const auto& it : d.observers_NtOpenObjectAuditAlarm)
            it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);
    }

    static void on_NtOpenPrivateNamespace(monitor::syscalls::Data& d)
    {
        const auto NamespaceHandle    = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess      = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto BoundaryDescriptor = arg<nt::PVOID>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenPrivateNamespace(NamespaceHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, BoundaryDescriptor:{:#x})", NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);

        for(const auto& it : d.observers_NtOpenPrivateNamespace)
            it(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    }

    static void on_NtOpenProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle    = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ClientId         = arg<nt::PCLIENT_ID>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenProcess(ProcessHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ClientId:{:#x})", ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);

        for(const auto& it : d.observers_NtOpenProcess)
            it(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
    }

    static void on_NtOpenProcessTokenEx(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto HandleAttributes = arg<nt::ULONG>(d.core, 2);
        const auto TokenHandle      = arg<nt::PHANDLE>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenProcessTokenEx(ProcessHandle:{:#x}, DesiredAccess:{:#x}, HandleAttributes:{:#x}, TokenHandle:{:#x})", ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle);

        for(const auto& it : d.observers_NtOpenProcessTokenEx)
            it(ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle);
    }

    static void on_NtOpenProcessToken(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 0);
        const auto DesiredAccess = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto TokenHandle   = arg<nt::PHANDLE>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenProcessToken(ProcessHandle:{:#x}, DesiredAccess:{:#x}, TokenHandle:{:#x})", ProcessHandle, DesiredAccess, TokenHandle);

        for(const auto& it : d.observers_NtOpenProcessToken)
            it(ProcessHandle, DesiredAccess, TokenHandle);
    }

    static void on_NtOpenResourceManager(monitor::syscalls::Data& d)
    {
        const auto ResourceManagerHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess         = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto TmHandle              = arg<nt::HANDLE>(d.core, 2);
        const auto ResourceManagerGuid   = arg<nt::LPGUID>(d.core, 3);
        const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenResourceManager(ResourceManagerHandle:{:#x}, DesiredAccess:{:#x}, TmHandle:{:#x}, ResourceManagerGuid:{:#x}, ObjectAttributes:{:#x})", ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenResourceManager)
            it(ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes);
    }

    static void on_NtOpenSection(monitor::syscalls::Data& d)
    {
        const auto SectionHandle    = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenSection(SectionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", SectionHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenSection)
            it(SectionHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenSemaphore(monitor::syscalls::Data& d)
    {
        const auto SemaphoreHandle  = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenSemaphore(SemaphoreHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", SemaphoreHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenSemaphore)
            it(SemaphoreHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenSession(monitor::syscalls::Data& d)
    {
        const auto SessionHandle    = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenSession(SessionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", SessionHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenSession)
            it(SessionHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenSymbolicLinkObject(monitor::syscalls::Data& d)
    {
        const auto LinkHandle       = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenSymbolicLinkObject(LinkHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", LinkHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenSymbolicLinkObject)
            it(LinkHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle     = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto ClientId         = arg<nt::PCLIENT_ID>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenThread(ThreadHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, ClientId:{:#x})", ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);

        for(const auto& it : d.observers_NtOpenThread)
            it(ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);
    }

    static void on_NtOpenThreadTokenEx(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto OpenAsSelf       = arg<nt::BOOLEAN>(d.core, 2);
        const auto HandleAttributes = arg<nt::ULONG>(d.core, 3);
        const auto TokenHandle      = arg<nt::PHANDLE>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenThreadTokenEx(ThreadHandle:{:#x}, DesiredAccess:{:#x}, OpenAsSelf:{:#x}, HandleAttributes:{:#x}, TokenHandle:{:#x})", ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle);

        for(const auto& it : d.observers_NtOpenThreadTokenEx)
            it(ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle);
    }

    static void on_NtOpenThreadToken(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto DesiredAccess = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto OpenAsSelf    = arg<nt::BOOLEAN>(d.core, 2);
        const auto TokenHandle   = arg<nt::PHANDLE>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenThreadToken(ThreadHandle:{:#x}, DesiredAccess:{:#x}, OpenAsSelf:{:#x}, TokenHandle:{:#x})", ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);

        for(const auto& it : d.observers_NtOpenThreadToken)
            it(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
    }

    static void on_NtOpenTimer(monitor::syscalls::Data& d)
    {
        const auto TimerHandle      = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenTimer(TimerHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x})", TimerHandle, DesiredAccess, ObjectAttributes);

        for(const auto& it : d.observers_NtOpenTimer)
            it(TimerHandle, DesiredAccess, ObjectAttributes);
    }

    static void on_NtOpenTransactionManager(monitor::syscalls::Data& d)
    {
        const auto TmHandle         = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess    = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto LogFileName      = arg<nt::PUNICODE_STRING>(d.core, 3);
        const auto TmIdentity       = arg<nt::LPGUID>(d.core, 4);
        const auto OpenOptions      = arg<nt::ULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenTransactionManager(TmHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, LogFileName:{:#x}, TmIdentity:{:#x}, OpenOptions:{:#x})", TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions);

        for(const auto& it : d.observers_NtOpenTransactionManager)
            it(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions);
    }

    static void on_NtOpenTransaction(monitor::syscalls::Data& d)
    {
        const auto TransactionHandle = arg<nt::PHANDLE>(d.core, 0);
        const auto DesiredAccess     = arg<nt::ACCESS_MASK>(d.core, 1);
        const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);
        const auto Uow               = arg<nt::LPGUID>(d.core, 3);
        const auto TmHandle          = arg<nt::HANDLE>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtOpenTransaction(TransactionHandle:{:#x}, DesiredAccess:{:#x}, ObjectAttributes:{:#x}, Uow:{:#x}, TmHandle:{:#x})", TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle);

        for(const auto& it : d.observers_NtOpenTransaction)
            it(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle);
    }

    static void on_NtPlugPlayControl(monitor::syscalls::Data& d)
    {
        const auto PnPControlClass      = arg<nt::PLUGPLAY_CONTROL_CLASS>(d.core, 0);
        const auto PnPControlData       = arg<nt::PVOID>(d.core, 1);
        const auto PnPControlDataLength = arg<nt::ULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtPlugPlayControl(PnPControlClass:{:#x}, PnPControlData:{:#x}, PnPControlDataLength:{:#x})", PnPControlClass, PnPControlData, PnPControlDataLength);

        for(const auto& it : d.observers_NtPlugPlayControl)
            it(PnPControlClass, PnPControlData, PnPControlDataLength);
    }

    static void on_NtPowerInformation(monitor::syscalls::Data& d)
    {
        const auto InformationLevel   = arg<nt::POWER_INFORMATION_LEVEL>(d.core, 0);
        const auto InputBuffer        = arg<nt::PVOID>(d.core, 1);
        const auto InputBufferLength  = arg<nt::ULONG>(d.core, 2);
        const auto OutputBuffer       = arg<nt::PVOID>(d.core, 3);
        const auto OutputBufferLength = arg<nt::ULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtPowerInformation(InformationLevel:{:#x}, InputBuffer:{:#x}, InputBufferLength:{:#x}, OutputBuffer:{:#x}, OutputBufferLength:{:#x})", InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);

        for(const auto& it : d.observers_NtPowerInformation)
            it(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    }

    static void on_NtPrepareComplete(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtPrepareComplete(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtPrepareComplete)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtPrepareEnlistment(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtPrepareEnlistment(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtPrepareEnlistment)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtPrePrepareComplete(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtPrePrepareComplete(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtPrePrepareComplete)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtPrePrepareEnlistment(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtPrePrepareEnlistment(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtPrePrepareEnlistment)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtPrivilegeCheck(monitor::syscalls::Data& d)
    {
        const auto ClientToken        = arg<nt::HANDLE>(d.core, 0);
        const auto RequiredPrivileges = arg<nt::PPRIVILEGE_SET>(d.core, 1);
        const auto Result             = arg<nt::PBOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtPrivilegeCheck(ClientToken:{:#x}, RequiredPrivileges:{:#x}, Result:{:#x})", ClientToken, RequiredPrivileges, Result);

        for(const auto& it : d.observers_NtPrivilegeCheck)
            it(ClientToken, RequiredPrivileges, Result);
    }

    static void on_NtPrivilegedServiceAuditAlarm(monitor::syscalls::Data& d)
    {
        const auto SubsystemName = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto ServiceName   = arg<nt::PUNICODE_STRING>(d.core, 1);
        const auto ClientToken   = arg<nt::HANDLE>(d.core, 2);
        const auto Privileges    = arg<nt::PPRIVILEGE_SET>(d.core, 3);
        const auto AccessGranted = arg<nt::BOOLEAN>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtPrivilegedServiceAuditAlarm(SubsystemName:{:#x}, ServiceName:{:#x}, ClientToken:{:#x}, Privileges:{:#x}, AccessGranted:{:#x})", SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted);

        for(const auto& it : d.observers_NtPrivilegedServiceAuditAlarm)
            it(SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted);
    }

    static void on_NtPrivilegeObjectAuditAlarm(monitor::syscalls::Data& d)
    {
        const auto SubsystemName = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto HandleId      = arg<nt::PVOID>(d.core, 1);
        const auto ClientToken   = arg<nt::HANDLE>(d.core, 2);
        const auto DesiredAccess = arg<nt::ACCESS_MASK>(d.core, 3);
        const auto Privileges    = arg<nt::PPRIVILEGE_SET>(d.core, 4);
        const auto AccessGranted = arg<nt::BOOLEAN>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtPrivilegeObjectAuditAlarm(SubsystemName:{:#x}, HandleId:{:#x}, ClientToken:{:#x}, DesiredAccess:{:#x}, Privileges:{:#x}, AccessGranted:{:#x})", SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);

        for(const auto& it : d.observers_NtPrivilegeObjectAuditAlarm)
            it(SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);
    }

    static void on_NtPropagationComplete(monitor::syscalls::Data& d)
    {
        const auto ResourceManagerHandle = arg<nt::HANDLE>(d.core, 0);
        const auto RequestCookie         = arg<nt::ULONG>(d.core, 1);
        const auto BufferLength          = arg<nt::ULONG>(d.core, 2);
        const auto Buffer                = arg<nt::PVOID>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtPropagationComplete(ResourceManagerHandle:{:#x}, RequestCookie:{:#x}, BufferLength:{:#x}, Buffer:{:#x})", ResourceManagerHandle, RequestCookie, BufferLength, Buffer);

        for(const auto& it : d.observers_NtPropagationComplete)
            it(ResourceManagerHandle, RequestCookie, BufferLength, Buffer);
    }

    static void on_NtPropagationFailed(monitor::syscalls::Data& d)
    {
        const auto ResourceManagerHandle = arg<nt::HANDLE>(d.core, 0);
        const auto RequestCookie         = arg<nt::ULONG>(d.core, 1);
        const auto PropStatus            = arg<nt::NTSTATUS>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtPropagationFailed(ResourceManagerHandle:{:#x}, RequestCookie:{:#x}, PropStatus:{:#x})", ResourceManagerHandle, RequestCookie, PropStatus);

        for(const auto& it : d.observers_NtPropagationFailed)
            it(ResourceManagerHandle, RequestCookie, PropStatus);
    }

    static void on_NtProtectVirtualMemory(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(d.core, 1);
        const auto RegionSize      = arg<nt::PSIZE_T>(d.core, 2);
        const auto NewProtectWin32 = arg<nt::WIN32_PROTECTION_MASK>(d.core, 3);
        const auto OldProtect      = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtProtectVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, RegionSize:{:#x}, NewProtectWin32:{:#x}, OldProtect:{:#x})", ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect);

        for(const auto& it : d.observers_NtProtectVirtualMemory)
            it(ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect);
    }

    static void on_NtPulseEvent(monitor::syscalls::Data& d)
    {
        const auto EventHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto PreviousState = arg<nt::PLONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtPulseEvent(EventHandle:{:#x}, PreviousState:{:#x})", EventHandle, PreviousState);

        for(const auto& it : d.observers_NtPulseEvent)
            it(EventHandle, PreviousState);
    }

    static void on_NtQueryAttributesFile(monitor::syscalls::Data& d)
    {
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto FileInformation  = arg<nt::PFILE_BASIC_INFORMATION>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryAttributesFile(ObjectAttributes:{:#x}, FileInformation:{:#x})", ObjectAttributes, FileInformation);

        for(const auto& it : d.observers_NtQueryAttributesFile)
            it(ObjectAttributes, FileInformation);
    }

    static void on_NtQueryBootEntryOrder(monitor::syscalls::Data& d)
    {
        const auto Ids   = arg<nt::PULONG>(d.core, 0);
        const auto Count = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryBootEntryOrder(Ids:{:#x}, Count:{:#x})", Ids, Count);

        for(const auto& it : d.observers_NtQueryBootEntryOrder)
            it(Ids, Count);
    }

    static void on_NtQueryBootOptions(monitor::syscalls::Data& d)
    {
        const auto BootOptions       = arg<nt::PBOOT_OPTIONS>(d.core, 0);
        const auto BootOptionsLength = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryBootOptions(BootOptions:{:#x}, BootOptionsLength:{:#x})", BootOptions, BootOptionsLength);

        for(const auto& it : d.observers_NtQueryBootOptions)
            it(BootOptions, BootOptionsLength);
    }

    static void on_NtQueryDebugFilterState(monitor::syscalls::Data& d)
    {
        const auto ComponentId = arg<nt::ULONG>(d.core, 0);
        const auto Level       = arg<nt::ULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryDebugFilterState(ComponentId:{:#x}, Level:{:#x})", ComponentId, Level);

        for(const auto& it : d.observers_NtQueryDebugFilterState)
            it(ComponentId, Level);
    }

    static void on_NtQueryDefaultLocale(monitor::syscalls::Data& d)
    {
        const auto UserProfile     = arg<nt::BOOLEAN>(d.core, 0);
        const auto DefaultLocaleId = arg<nt::PLCID>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryDefaultLocale(UserProfile:{:#x}, DefaultLocaleId:{:#x})", UserProfile, DefaultLocaleId);

        for(const auto& it : d.observers_NtQueryDefaultLocale)
            it(UserProfile, DefaultLocaleId);
    }

    static void on_NtQueryDefaultUILanguage(monitor::syscalls::Data& d)
    {
        const auto STARDefaultUILanguageId = arg<nt::LANGID>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryDefaultUILanguage(STARDefaultUILanguageId:{:#x})", STARDefaultUILanguageId);

        for(const auto& it : d.observers_NtQueryDefaultUILanguage)
            it(STARDefaultUILanguageId);
    }

    static void on_NtQueryDirectoryFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto Event                = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine           = arg<nt::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext           = arg<nt::PVOID>(d.core, 3);
        const auto IoStatusBlock        = arg<nt::PIO_STATUS_BLOCK>(d.core, 4);
        const auto FileInformation      = arg<nt::PVOID>(d.core, 5);
        const auto Length               = arg<nt::ULONG>(d.core, 6);
        const auto FileInformationClass = arg<nt::FILE_INFORMATION_CLASS>(d.core, 7);
        const auto ReturnSingleEntry    = arg<nt::BOOLEAN>(d.core, 8);
        const auto FileName             = arg<nt::PUNICODE_STRING>(d.core, 9);
        const auto RestartScan          = arg<nt::BOOLEAN>(d.core, 10);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryDirectoryFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, FileInformation:{:#x}, Length:{:#x}, FileInformationClass:{:#x}, ReturnSingleEntry:{:#x}, FileName:{:#x}, RestartScan:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);

        for(const auto& it : d.observers_NtQueryDirectoryFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
    }

    static void on_NtQueryDirectoryObject(monitor::syscalls::Data& d)
    {
        const auto DirectoryHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto Buffer            = arg<nt::PVOID>(d.core, 1);
        const auto Length            = arg<nt::ULONG>(d.core, 2);
        const auto ReturnSingleEntry = arg<nt::BOOLEAN>(d.core, 3);
        const auto RestartScan       = arg<nt::BOOLEAN>(d.core, 4);
        const auto Context           = arg<nt::PULONG>(d.core, 5);
        const auto ReturnLength      = arg<nt::PULONG>(d.core, 6);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryDirectoryObject(DirectoryHandle:{:#x}, Buffer:{:#x}, Length:{:#x}, ReturnSingleEntry:{:#x}, RestartScan:{:#x}, Context:{:#x}, ReturnLength:{:#x})", DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength);

        for(const auto& it : d.observers_NtQueryDirectoryObject)
            it(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength);
    }

    static void on_NtQueryDriverEntryOrder(monitor::syscalls::Data& d)
    {
        const auto Ids   = arg<nt::PULONG>(d.core, 0);
        const auto Count = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryDriverEntryOrder(Ids:{:#x}, Count:{:#x})", Ids, Count);

        for(const auto& it : d.observers_NtQueryDriverEntryOrder)
            it(Ids, Count);
    }

    static void on_NtQueryEaFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto Buffer            = arg<nt::PVOID>(d.core, 2);
        const auto Length            = arg<nt::ULONG>(d.core, 3);
        const auto ReturnSingleEntry = arg<nt::BOOLEAN>(d.core, 4);
        const auto EaList            = arg<nt::PVOID>(d.core, 5);
        const auto EaListLength      = arg<nt::ULONG>(d.core, 6);
        const auto EaIndex           = arg<nt::PULONG>(d.core, 7);
        const auto RestartScan       = arg<nt::BOOLEAN>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryEaFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x}, ReturnSingleEntry:{:#x}, EaList:{:#x}, EaListLength:{:#x}, EaIndex:{:#x}, RestartScan:{:#x})", FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);

        for(const auto& it : d.observers_NtQueryEaFile)
            it(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
    }

    static void on_NtQueryEvent(monitor::syscalls::Data& d)
    {
        const auto EventHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto EventInformationClass  = arg<nt::EVENT_INFORMATION_CLASS>(d.core, 1);
        const auto EventInformation       = arg<nt::PVOID>(d.core, 2);
        const auto EventInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength           = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryEvent(EventHandle:{:#x}, EventInformationClass:{:#x}, EventInformation:{:#x}, EventInformationLength:{:#x}, ReturnLength:{:#x})", EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryEvent)
            it(EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength);
    }

    static void on_NtQueryFullAttributesFile(monitor::syscalls::Data& d)
    {
        const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto FileInformation  = arg<nt::PFILE_NETWORK_OPEN_INFORMATION>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryFullAttributesFile(ObjectAttributes:{:#x}, FileInformation:{:#x})", ObjectAttributes, FileInformation);

        for(const auto& it : d.observers_NtQueryFullAttributesFile)
            it(ObjectAttributes, FileInformation);
    }

    static void on_NtQueryInformationAtom(monitor::syscalls::Data& d)
    {
        const auto Atom                  = arg<nt::RTL_ATOM>(d.core, 0);
        const auto InformationClass      = arg<nt::ATOM_INFORMATION_CLASS>(d.core, 1);
        const auto AtomInformation       = arg<nt::PVOID>(d.core, 2);
        const auto AtomInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength          = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationAtom(Atom:{:#x}, InformationClass:{:#x}, AtomInformation:{:#x}, AtomInformationLength:{:#x}, ReturnLength:{:#x})", Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationAtom)
            it(Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationEnlistment(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto EnlistmentInformationClass  = arg<nt::ENLISTMENT_INFORMATION_CLASS>(d.core, 1);
        const auto EnlistmentInformation       = arg<nt::PVOID>(d.core, 2);
        const auto EnlistmentInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength                = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationEnlistment(EnlistmentHandle:{:#x}, EnlistmentInformationClass:{:#x}, EnlistmentInformation:{:#x}, EnlistmentInformationLength:{:#x}, ReturnLength:{:#x})", EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationEnlistment)
            it(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock        = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto FileInformation      = arg<nt::PVOID>(d.core, 2);
        const auto Length               = arg<nt::ULONG>(d.core, 3);
        const auto FileInformationClass = arg<nt::FILE_INFORMATION_CLASS>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, FileInformation:{:#x}, Length:{:#x}, FileInformationClass:{:#x})", FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);

        for(const auto& it : d.observers_NtQueryInformationFile)
            it(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    }

    static void on_NtQueryInformationJobObject(monitor::syscalls::Data& d)
    {
        const auto JobHandle                  = arg<nt::HANDLE>(d.core, 0);
        const auto JobObjectInformationClass  = arg<nt::JOBOBJECTINFOCLASS>(d.core, 1);
        const auto JobObjectInformation       = arg<nt::PVOID>(d.core, 2);
        const auto JobObjectInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength               = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationJobObject(JobHandle:{:#x}, JobObjectInformationClass:{:#x}, JobObjectInformation:{:#x}, JobObjectInformationLength:{:#x}, ReturnLength:{:#x})", JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationJobObject)
            it(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto PortInformationClass = arg<nt::PORT_INFORMATION_CLASS>(d.core, 1);
        const auto PortInformation      = arg<nt::PVOID>(d.core, 2);
        const auto Length               = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength         = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationPort(PortHandle:{:#x}, PortInformationClass:{:#x}, PortInformation:{:#x}, Length:{:#x}, ReturnLength:{:#x})", PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationPort)
            it(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    }

    static void on_NtQueryInformationProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto ProcessInformationClass  = arg<nt::PROCESSINFOCLASS>(d.core, 1);
        const auto ProcessInformation       = arg<nt::PVOID>(d.core, 2);
        const auto ProcessInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength             = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationProcess(ProcessHandle:{:#x}, ProcessInformationClass:{:#x}, ProcessInformation:{:#x}, ProcessInformationLength:{:#x}, ReturnLength:{:#x})", ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationProcess)
            it(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationResourceManager(monitor::syscalls::Data& d)
    {
        const auto ResourceManagerHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto ResourceManagerInformationClass  = arg<nt::RESOURCEMANAGER_INFORMATION_CLASS>(d.core, 1);
        const auto ResourceManagerInformation       = arg<nt::PVOID>(d.core, 2);
        const auto ResourceManagerInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength                     = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationResourceManager(ResourceManagerHandle:{:#x}, ResourceManagerInformationClass:{:#x}, ResourceManagerInformation:{:#x}, ResourceManagerInformationLength:{:#x}, ReturnLength:{:#x})", ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationResourceManager)
            it(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto ThreadInformationClass  = arg<nt::THREADINFOCLASS>(d.core, 1);
        const auto ThreadInformation       = arg<nt::PVOID>(d.core, 2);
        const auto ThreadInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength            = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationThread(ThreadHandle:{:#x}, ThreadInformationClass:{:#x}, ThreadInformation:{:#x}, ThreadInformationLength:{:#x}, ReturnLength:{:#x})", ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationThread)
            it(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationToken(monitor::syscalls::Data& d)
    {
        const auto TokenHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto TokenInformationClass  = arg<nt::TOKEN_INFORMATION_CLASS>(d.core, 1);
        const auto TokenInformation       = arg<nt::PVOID>(d.core, 2);
        const auto TokenInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength           = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationToken(TokenHandle:{:#x}, TokenInformationClass:{:#x}, TokenInformation:{:#x}, TokenInformationLength:{:#x}, ReturnLength:{:#x})", TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationToken)
            it(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationTransaction(monitor::syscalls::Data& d)
    {
        const auto TransactionHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto TransactionInformationClass  = arg<nt::TRANSACTION_INFORMATION_CLASS>(d.core, 1);
        const auto TransactionInformation       = arg<nt::PVOID>(d.core, 2);
        const auto TransactionInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength                 = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationTransaction(TransactionHandle:{:#x}, TransactionInformationClass:{:#x}, TransactionInformation:{:#x}, TransactionInformationLength:{:#x}, ReturnLength:{:#x})", TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationTransaction)
            it(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationTransactionManager(monitor::syscalls::Data& d)
    {
        const auto TransactionManagerHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto TransactionManagerInformationClass  = arg<nt::TRANSACTIONMANAGER_INFORMATION_CLASS>(d.core, 1);
        const auto TransactionManagerInformation       = arg<nt::PVOID>(d.core, 2);
        const auto TransactionManagerInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength                        = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationTransactionManager(TransactionManagerHandle:{:#x}, TransactionManagerInformationClass:{:#x}, TransactionManagerInformation:{:#x}, TransactionManagerInformationLength:{:#x}, ReturnLength:{:#x})", TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationTransactionManager)
            it(TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength);
    }

    static void on_NtQueryInformationWorkerFactory(monitor::syscalls::Data& d)
    {
        const auto WorkerFactoryHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto WorkerFactoryInformationClass  = arg<nt::WORKERFACTORYINFOCLASS>(d.core, 1);
        const auto WorkerFactoryInformation       = arg<nt::PVOID>(d.core, 2);
        const auto WorkerFactoryInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength                   = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInformationWorkerFactory(WorkerFactoryHandle:{:#x}, WorkerFactoryInformationClass:{:#x}, WorkerFactoryInformation:{:#x}, WorkerFactoryInformationLength:{:#x}, ReturnLength:{:#x})", WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryInformationWorkerFactory)
            it(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength);
    }

    static void on_NtQueryInstallUILanguage(monitor::syscalls::Data& d)
    {
        const auto STARInstallUILanguageId = arg<nt::LANGID>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryInstallUILanguage(STARInstallUILanguageId:{:#x})", STARInstallUILanguageId);

        for(const auto& it : d.observers_NtQueryInstallUILanguage)
            it(STARInstallUILanguageId);
    }

    static void on_NtQueryIntervalProfile(monitor::syscalls::Data& d)
    {
        const auto ProfileSource = arg<nt::KPROFILE_SOURCE>(d.core, 0);
        const auto Interval      = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryIntervalProfile(ProfileSource:{:#x}, Interval:{:#x})", ProfileSource, Interval);

        for(const auto& it : d.observers_NtQueryIntervalProfile)
            it(ProfileSource, Interval);
    }

    static void on_NtQueryIoCompletion(monitor::syscalls::Data& d)
    {
        const auto IoCompletionHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto IoCompletionInformationClass  = arg<nt::IO_COMPLETION_INFORMATION_CLASS>(d.core, 1);
        const auto IoCompletionInformation       = arg<nt::PVOID>(d.core, 2);
        const auto IoCompletionInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength                  = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryIoCompletion(IoCompletionHandle:{:#x}, IoCompletionInformationClass:{:#x}, IoCompletionInformation:{:#x}, IoCompletionInformationLength:{:#x}, ReturnLength:{:#x})", IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryIoCompletion)
            it(IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength);
    }

    static void on_NtQueryKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto KeyInformationClass = arg<nt::KEY_INFORMATION_CLASS>(d.core, 1);
        const auto KeyInformation      = arg<nt::PVOID>(d.core, 2);
        const auto Length              = arg<nt::ULONG>(d.core, 3);
        const auto ResultLength        = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryKey(KeyHandle:{:#x}, KeyInformationClass:{:#x}, KeyInformation:{:#x}, Length:{:#x}, ResultLength:{:#x})", KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);

        for(const auto& it : d.observers_NtQueryKey)
            it(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
    }

    static void on_NtQueryLicenseValue(monitor::syscalls::Data& d)
    {
        const auto Name           = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto Type           = arg<nt::PULONG>(d.core, 1);
        const auto Buffer         = arg<nt::PVOID>(d.core, 2);
        const auto Length         = arg<nt::ULONG>(d.core, 3);
        const auto ReturnedLength = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryLicenseValue(Name:{:#x}, Type:{:#x}, Buffer:{:#x}, Length:{:#x}, ReturnedLength:{:#x})", Name, Type, Buffer, Length, ReturnedLength);

        for(const auto& it : d.observers_NtQueryLicenseValue)
            it(Name, Type, Buffer, Length, ReturnedLength);
    }

    static void on_NtQueryMultipleValueKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto ValueEntries         = arg<nt::PKEY_VALUE_ENTRY>(d.core, 1);
        const auto EntryCount           = arg<nt::ULONG>(d.core, 2);
        const auto ValueBuffer          = arg<nt::PVOID>(d.core, 3);
        const auto BufferLength         = arg<nt::PULONG>(d.core, 4);
        const auto RequiredBufferLength = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryMultipleValueKey(KeyHandle:{:#x}, ValueEntries:{:#x}, EntryCount:{:#x}, ValueBuffer:{:#x}, BufferLength:{:#x}, RequiredBufferLength:{:#x})", KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);

        for(const auto& it : d.observers_NtQueryMultipleValueKey)
            it(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
    }

    static void on_NtQueryMutant(monitor::syscalls::Data& d)
    {
        const auto MutantHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto MutantInformationClass  = arg<nt::MUTANT_INFORMATION_CLASS>(d.core, 1);
        const auto MutantInformation       = arg<nt::PVOID>(d.core, 2);
        const auto MutantInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength            = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryMutant(MutantHandle:{:#x}, MutantInformationClass:{:#x}, MutantInformation:{:#x}, MutantInformationLength:{:#x}, ReturnLength:{:#x})", MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryMutant)
            it(MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength);
    }

    static void on_NtQueryObject(monitor::syscalls::Data& d)
    {
        const auto Handle                  = arg<nt::HANDLE>(d.core, 0);
        const auto ObjectInformationClass  = arg<nt::OBJECT_INFORMATION_CLASS>(d.core, 1);
        const auto ObjectInformation       = arg<nt::PVOID>(d.core, 2);
        const auto ObjectInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength            = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryObject(Handle:{:#x}, ObjectInformationClass:{:#x}, ObjectInformation:{:#x}, ObjectInformationLength:{:#x}, ReturnLength:{:#x})", Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryObject)
            it(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
    }

    static void on_NtQueryOpenSubKeysEx(monitor::syscalls::Data& d)
    {
        const auto TargetKey    = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto BufferLength = arg<nt::ULONG>(d.core, 1);
        const auto Buffer       = arg<nt::PVOID>(d.core, 2);
        const auto RequiredSize = arg<nt::PULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryOpenSubKeysEx(TargetKey:{:#x}, BufferLength:{:#x}, Buffer:{:#x}, RequiredSize:{:#x})", TargetKey, BufferLength, Buffer, RequiredSize);

        for(const auto& it : d.observers_NtQueryOpenSubKeysEx)
            it(TargetKey, BufferLength, Buffer, RequiredSize);
    }

    static void on_NtQueryOpenSubKeys(monitor::syscalls::Data& d)
    {
        const auto TargetKey   = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto HandleCount = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryOpenSubKeys(TargetKey:{:#x}, HandleCount:{:#x})", TargetKey, HandleCount);

        for(const auto& it : d.observers_NtQueryOpenSubKeys)
            it(TargetKey, HandleCount);
    }

    static void on_NtQueryPerformanceCounter(monitor::syscalls::Data& d)
    {
        const auto PerformanceCounter   = arg<nt::PLARGE_INTEGER>(d.core, 0);
        const auto PerformanceFrequency = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryPerformanceCounter(PerformanceCounter:{:#x}, PerformanceFrequency:{:#x})", PerformanceCounter, PerformanceFrequency);

        for(const auto& it : d.observers_NtQueryPerformanceCounter)
            it(PerformanceCounter, PerformanceFrequency);
    }

    static void on_NtQueryQuotaInformationFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto Buffer            = arg<nt::PVOID>(d.core, 2);
        const auto Length            = arg<nt::ULONG>(d.core, 3);
        const auto ReturnSingleEntry = arg<nt::BOOLEAN>(d.core, 4);
        const auto SidList           = arg<nt::PVOID>(d.core, 5);
        const auto SidListLength     = arg<nt::ULONG>(d.core, 6);
        const auto StartSid          = arg<nt::PULONG>(d.core, 7);
        const auto RestartScan       = arg<nt::BOOLEAN>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryQuotaInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x}, ReturnSingleEntry:{:#x}, SidList:{:#x}, SidListLength:{:#x}, StartSid:{:#x}, RestartScan:{:#x})", FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan);

        for(const auto& it : d.observers_NtQueryQuotaInformationFile)
            it(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan);
    }

    static void on_NtQuerySection(monitor::syscalls::Data& d)
    {
        const auto SectionHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto SectionInformationClass  = arg<nt::SECTION_INFORMATION_CLASS>(d.core, 1);
        const auto SectionInformation       = arg<nt::PVOID>(d.core, 2);
        const auto SectionInformationLength = arg<nt::SIZE_T>(d.core, 3);
        const auto ReturnLength             = arg<nt::PSIZE_T>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQuerySection(SectionHandle:{:#x}, SectionInformationClass:{:#x}, SectionInformation:{:#x}, SectionInformationLength:{:#x}, ReturnLength:{:#x})", SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQuerySection)
            it(SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength);
    }

    static void on_NtQuerySecurityAttributesToken(monitor::syscalls::Data& d)
    {
        const auto TokenHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto Attributes         = arg<nt::PUNICODE_STRING>(d.core, 1);
        const auto NumberOfAttributes = arg<nt::ULONG>(d.core, 2);
        const auto Buffer             = arg<nt::PVOID>(d.core, 3);
        const auto Length             = arg<nt::ULONG>(d.core, 4);
        const auto ReturnLength       = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtQuerySecurityAttributesToken(TokenHandle:{:#x}, Attributes:{:#x}, NumberOfAttributes:{:#x}, Buffer:{:#x}, Length:{:#x}, ReturnLength:{:#x})", TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength);

        for(const auto& it : d.observers_NtQuerySecurityAttributesToken)
            it(TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength);
    }

    static void on_NtQuerySecurityObject(monitor::syscalls::Data& d)
    {
        const auto Handle              = arg<nt::HANDLE>(d.core, 0);
        const auto SecurityInformation = arg<nt::SECURITY_INFORMATION>(d.core, 1);
        const auto SecurityDescriptor  = arg<nt::PSECURITY_DESCRIPTOR>(d.core, 2);
        const auto Length              = arg<nt::ULONG>(d.core, 3);
        const auto LengthNeeded        = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQuerySecurityObject(Handle:{:#x}, SecurityInformation:{:#x}, SecurityDescriptor:{:#x}, Length:{:#x}, LengthNeeded:{:#x})", Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded);

        for(const auto& it : d.observers_NtQuerySecurityObject)
            it(Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded);
    }

    static void on_NtQuerySemaphore(monitor::syscalls::Data& d)
    {
        const auto SemaphoreHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto SemaphoreInformationClass  = arg<nt::SEMAPHORE_INFORMATION_CLASS>(d.core, 1);
        const auto SemaphoreInformation       = arg<nt::PVOID>(d.core, 2);
        const auto SemaphoreInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength               = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQuerySemaphore(SemaphoreHandle:{:#x}, SemaphoreInformationClass:{:#x}, SemaphoreInformation:{:#x}, SemaphoreInformationLength:{:#x}, ReturnLength:{:#x})", SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQuerySemaphore)
            it(SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength);
    }

    static void on_NtQuerySymbolicLinkObject(monitor::syscalls::Data& d)
    {
        const auto LinkHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto LinkTarget     = arg<nt::PUNICODE_STRING>(d.core, 1);
        const auto ReturnedLength = arg<nt::PULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtQuerySymbolicLinkObject(LinkHandle:{:#x}, LinkTarget:{:#x}, ReturnedLength:{:#x})", LinkHandle, LinkTarget, ReturnedLength);

        for(const auto& it : d.observers_NtQuerySymbolicLinkObject)
            it(LinkHandle, LinkTarget, ReturnedLength);
    }

    static void on_NtQuerySystemEnvironmentValueEx(monitor::syscalls::Data& d)
    {
        const auto VariableName = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto VendorGuid   = arg<nt::LPGUID>(d.core, 1);
        const auto Value        = arg<nt::PVOID>(d.core, 2);
        const auto ValueLength  = arg<nt::PULONG>(d.core, 3);
        const auto Attributes   = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQuerySystemEnvironmentValueEx(VariableName:{:#x}, VendorGuid:{:#x}, Value:{:#x}, ValueLength:{:#x}, Attributes:{:#x})", VariableName, VendorGuid, Value, ValueLength, Attributes);

        for(const auto& it : d.observers_NtQuerySystemEnvironmentValueEx)
            it(VariableName, VendorGuid, Value, ValueLength, Attributes);
    }

    static void on_NtQuerySystemEnvironmentValue(monitor::syscalls::Data& d)
    {
        const auto VariableName  = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto VariableValue = arg<nt::PWSTR>(d.core, 1);
        const auto ValueLength   = arg<nt::USHORT>(d.core, 2);
        const auto ReturnLength  = arg<nt::PUSHORT>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtQuerySystemEnvironmentValue(VariableName:{:#x}, VariableValue:{:#x}, ValueLength:{:#x}, ReturnLength:{:#x})", VariableName, VariableValue, ValueLength, ReturnLength);

        for(const auto& it : d.observers_NtQuerySystemEnvironmentValue)
            it(VariableName, VariableValue, ValueLength, ReturnLength);
    }

    static void on_NtQuerySystemInformationEx(monitor::syscalls::Data& d)
    {
        const auto SystemInformationClass  = arg<nt::SYSTEM_INFORMATION_CLASS>(d.core, 0);
        const auto QueryInformation        = arg<nt::PVOID>(d.core, 1);
        const auto QueryInformationLength  = arg<nt::ULONG>(d.core, 2);
        const auto SystemInformation       = arg<nt::PVOID>(d.core, 3);
        const auto SystemInformationLength = arg<nt::ULONG>(d.core, 4);
        const auto ReturnLength            = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtQuerySystemInformationEx(SystemInformationClass:{:#x}, QueryInformation:{:#x}, QueryInformationLength:{:#x}, SystemInformation:{:#x}, SystemInformationLength:{:#x}, ReturnLength:{:#x})", SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQuerySystemInformationEx)
            it(SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength);
    }

    static void on_NtQuerySystemInformation(monitor::syscalls::Data& d)
    {
        const auto SystemInformationClass  = arg<nt::SYSTEM_INFORMATION_CLASS>(d.core, 0);
        const auto SystemInformation       = arg<nt::PVOID>(d.core, 1);
        const auto SystemInformationLength = arg<nt::ULONG>(d.core, 2);
        const auto ReturnLength            = arg<nt::PULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtQuerySystemInformation(SystemInformationClass:{:#x}, SystemInformation:{:#x}, SystemInformationLength:{:#x}, ReturnLength:{:#x})", SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQuerySystemInformation)
            it(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
    }

    static void on_NtQuerySystemTime(monitor::syscalls::Data& d)
    {
        const auto SystemTime = arg<nt::PLARGE_INTEGER>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtQuerySystemTime(SystemTime:{:#x})", SystemTime);

        for(const auto& it : d.observers_NtQuerySystemTime)
            it(SystemTime);
    }

    static void on_NtQueryTimer(monitor::syscalls::Data& d)
    {
        const auto TimerHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto TimerInformationClass  = arg<nt::TIMER_INFORMATION_CLASS>(d.core, 1);
        const auto TimerInformation       = arg<nt::PVOID>(d.core, 2);
        const auto TimerInformationLength = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength           = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryTimer(TimerHandle:{:#x}, TimerInformationClass:{:#x}, TimerInformation:{:#x}, TimerInformationLength:{:#x}, ReturnLength:{:#x})", TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryTimer)
            it(TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength);
    }

    static void on_NtQueryTimerResolution(monitor::syscalls::Data& d)
    {
        const auto MaximumTime = arg<nt::PULONG>(d.core, 0);
        const auto MinimumTime = arg<nt::PULONG>(d.core, 1);
        const auto CurrentTime = arg<nt::PULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryTimerResolution(MaximumTime:{:#x}, MinimumTime:{:#x}, CurrentTime:{:#x})", MaximumTime, MinimumTime, CurrentTime);

        for(const auto& it : d.observers_NtQueryTimerResolution)
            it(MaximumTime, MinimumTime, CurrentTime);
    }

    static void on_NtQueryValueKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle                = arg<nt::HANDLE>(d.core, 0);
        const auto ValueName                = arg<nt::PUNICODE_STRING>(d.core, 1);
        const auto KeyValueInformationClass = arg<nt::KEY_VALUE_INFORMATION_CLASS>(d.core, 2);
        const auto KeyValueInformation      = arg<nt::PVOID>(d.core, 3);
        const auto Length                   = arg<nt::ULONG>(d.core, 4);
        const auto ResultLength             = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryValueKey(KeyHandle:{:#x}, ValueName:{:#x}, KeyValueInformationClass:{:#x}, KeyValueInformation:{:#x}, Length:{:#x}, ResultLength:{:#x})", KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);

        for(const auto& it : d.observers_NtQueryValueKey)
            it(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    }

    static void on_NtQueryVirtualMemory(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto BaseAddress             = arg<nt::PVOID>(d.core, 1);
        const auto MemoryInformationClass  = arg<nt::MEMORY_INFORMATION_CLASS>(d.core, 2);
        const auto MemoryInformation       = arg<nt::PVOID>(d.core, 3);
        const auto MemoryInformationLength = arg<nt::SIZE_T>(d.core, 4);
        const auto ReturnLength            = arg<nt::PSIZE_T>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryVirtualMemory(ProcessHandle:{:#x}, BaseAddress:{:#x}, MemoryInformationClass:{:#x}, MemoryInformation:{:#x}, MemoryInformationLength:{:#x}, ReturnLength:{:#x})", ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtQueryVirtualMemory)
            it(ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
    }

    static void on_NtQueryVolumeInformationFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle         = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto FsInformation      = arg<nt::PVOID>(d.core, 2);
        const auto Length             = arg<nt::ULONG>(d.core, 3);
        const auto FsInformationClass = arg<nt::FS_INFORMATION_CLASS>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueryVolumeInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, FsInformation:{:#x}, Length:{:#x}, FsInformationClass:{:#x})", FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);

        for(const auto& it : d.observers_NtQueryVolumeInformationFile)
            it(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    }

    static void on_NtQueueApcThreadEx(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle         = arg<nt::HANDLE>(d.core, 0);
        const auto UserApcReserveHandle = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine           = arg<nt::PPS_APC_ROUTINE>(d.core, 2);
        const auto ApcArgument1         = arg<nt::PVOID>(d.core, 3);
        const auto ApcArgument2         = arg<nt::PVOID>(d.core, 4);
        const auto ApcArgument3         = arg<nt::PVOID>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtQueueApcThreadEx(ThreadHandle:{:#x}, UserApcReserveHandle:{:#x}, ApcRoutine:{:#x}, ApcArgument1:{:#x}, ApcArgument2:{:#x}, ApcArgument3:{:#x})", ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);

        for(const auto& it : d.observers_NtQueueApcThreadEx)
            it(ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    }

    static void on_NtQueueApcThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle = arg<nt::HANDLE>(d.core, 0);
        const auto ApcRoutine   = arg<nt::PPS_APC_ROUTINE>(d.core, 1);
        const auto ApcArgument1 = arg<nt::PVOID>(d.core, 2);
        const auto ApcArgument2 = arg<nt::PVOID>(d.core, 3);
        const auto ApcArgument3 = arg<nt::PVOID>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtQueueApcThread(ThreadHandle:{:#x}, ApcRoutine:{:#x}, ApcArgument1:{:#x}, ApcArgument2:{:#x}, ApcArgument3:{:#x})", ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);

        for(const auto& it : d.observers_NtQueueApcThread)
            it(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    }

    static void on_NtRaiseException(monitor::syscalls::Data& d)
    {
        const auto ExceptionRecord = arg<nt::PEXCEPTION_RECORD>(d.core, 0);
        const auto ContextRecord   = arg<nt::PCONTEXT>(d.core, 1);
        const auto FirstChance     = arg<nt::BOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtRaiseException(ExceptionRecord:{:#x}, ContextRecord:{:#x}, FirstChance:{:#x})", ExceptionRecord, ContextRecord, FirstChance);

        for(const auto& it : d.observers_NtRaiseException)
            it(ExceptionRecord, ContextRecord, FirstChance);
    }

    static void on_NtRaiseHardError(monitor::syscalls::Data& d)
    {
        const auto ErrorStatus                = arg<nt::NTSTATUS>(d.core, 0);
        const auto NumberOfParameters         = arg<nt::ULONG>(d.core, 1);
        const auto UnicodeStringParameterMask = arg<nt::ULONG>(d.core, 2);
        const auto Parameters                 = arg<nt::PULONG_PTR>(d.core, 3);
        const auto ValidResponseOptions       = arg<nt::ULONG>(d.core, 4);
        const auto Response                   = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtRaiseHardError(ErrorStatus:{:#x}, NumberOfParameters:{:#x}, UnicodeStringParameterMask:{:#x}, Parameters:{:#x}, ValidResponseOptions:{:#x}, Response:{:#x})", ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);

        for(const auto& it : d.observers_NtRaiseHardError)
            it(ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);
    }

    static void on_NtReadFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto Event         = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext    = arg<nt::PVOID>(d.core, 3);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(d.core, 4);
        const auto Buffer        = arg<nt::PVOID>(d.core, 5);
        const auto Length        = arg<nt::ULONG>(d.core, 6);
        const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(d.core, 7);
        const auto Key           = arg<nt::PULONG>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtReadFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x}, ByteOffset:{:#x}, Key:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);

        for(const auto& it : d.observers_NtReadFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    }

    static void on_NtReadFileScatter(monitor::syscalls::Data& d)
    {
        const auto FileHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto Event         = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext    = arg<nt::PVOID>(d.core, 3);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(d.core, 4);
        const auto SegmentArray  = arg<nt::PFILE_SEGMENT_ELEMENT>(d.core, 5);
        const auto Length        = arg<nt::ULONG>(d.core, 6);
        const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(d.core, 7);
        const auto Key           = arg<nt::PULONG>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtReadFileScatter(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, SegmentArray:{:#x}, Length:{:#x}, ByteOffset:{:#x}, Key:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);

        for(const auto& it : d.observers_NtReadFileScatter)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    }

    static void on_NtReadOnlyEnlistment(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtReadOnlyEnlistment(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtReadOnlyEnlistment)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtReadRequestData(monitor::syscalls::Data& d)
    {
        const auto PortHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto Message           = arg<nt::PPORT_MESSAGE>(d.core, 1);
        const auto DataEntryIndex    = arg<nt::ULONG>(d.core, 2);
        const auto Buffer            = arg<nt::PVOID>(d.core, 3);
        const auto BufferSize        = arg<nt::SIZE_T>(d.core, 4);
        const auto NumberOfBytesRead = arg<nt::PSIZE_T>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtReadRequestData(PortHandle:{:#x}, Message:{:#x}, DataEntryIndex:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, NumberOfBytesRead:{:#x})", PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead);

        for(const auto& it : d.observers_NtReadRequestData)
            it(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead);
    }

    static void on_NtReadVirtualMemory(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto BaseAddress       = arg<nt::PVOID>(d.core, 1);
        const auto Buffer            = arg<nt::PVOID>(d.core, 2);
        const auto BufferSize        = arg<nt::SIZE_T>(d.core, 3);
        const auto NumberOfBytesRead = arg<nt::PSIZE_T>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtReadVirtualMemory(ProcessHandle:{:#x}, BaseAddress:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, NumberOfBytesRead:{:#x})", ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead);

        for(const auto& it : d.observers_NtReadVirtualMemory)
            it(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead);
    }

    static void on_NtRecoverEnlistment(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto EnlistmentKey    = arg<nt::PVOID>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtRecoverEnlistment(EnlistmentHandle:{:#x}, EnlistmentKey:{:#x})", EnlistmentHandle, EnlistmentKey);

        for(const auto& it : d.observers_NtRecoverEnlistment)
            it(EnlistmentHandle, EnlistmentKey);
    }

    static void on_NtRecoverResourceManager(monitor::syscalls::Data& d)
    {
        const auto ResourceManagerHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtRecoverResourceManager(ResourceManagerHandle:{:#x})", ResourceManagerHandle);

        for(const auto& it : d.observers_NtRecoverResourceManager)
            it(ResourceManagerHandle);
    }

    static void on_NtRecoverTransactionManager(monitor::syscalls::Data& d)
    {
        const auto TransactionManagerHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtRecoverTransactionManager(TransactionManagerHandle:{:#x})", TransactionManagerHandle);

        for(const auto& it : d.observers_NtRecoverTransactionManager)
            it(TransactionManagerHandle);
    }

    static void on_NtRegisterProtocolAddressInformation(monitor::syscalls::Data& d)
    {
        const auto ResourceManager         = arg<nt::HANDLE>(d.core, 0);
        const auto ProtocolId              = arg<nt::PCRM_PROTOCOL_ID>(d.core, 1);
        const auto ProtocolInformationSize = arg<nt::ULONG>(d.core, 2);
        const auto ProtocolInformation     = arg<nt::PVOID>(d.core, 3);
        const auto CreateOptions           = arg<nt::ULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtRegisterProtocolAddressInformation(ResourceManager:{:#x}, ProtocolId:{:#x}, ProtocolInformationSize:{:#x}, ProtocolInformation:{:#x}, CreateOptions:{:#x})", ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions);

        for(const auto& it : d.observers_NtRegisterProtocolAddressInformation)
            it(ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions);
    }

    static void on_NtRegisterThreadTerminatePort(monitor::syscalls::Data& d)
    {
        const auto PortHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtRegisterThreadTerminatePort(PortHandle:{:#x})", PortHandle);

        for(const auto& it : d.observers_NtRegisterThreadTerminatePort)
            it(PortHandle);
    }

    static void on_NtReleaseKeyedEvent(monitor::syscalls::Data& d)
    {
        const auto KeyedEventHandle = arg<nt::HANDLE>(d.core, 0);
        const auto KeyValue         = arg<nt::PVOID>(d.core, 1);
        const auto Alertable        = arg<nt::BOOLEAN>(d.core, 2);
        const auto Timeout          = arg<nt::PLARGE_INTEGER>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtReleaseKeyedEvent(KeyedEventHandle:{:#x}, KeyValue:{:#x}, Alertable:{:#x}, Timeout:{:#x})", KeyedEventHandle, KeyValue, Alertable, Timeout);

        for(const auto& it : d.observers_NtReleaseKeyedEvent)
            it(KeyedEventHandle, KeyValue, Alertable, Timeout);
    }

    static void on_NtReleaseMutant(monitor::syscalls::Data& d)
    {
        const auto MutantHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto PreviousCount = arg<nt::PLONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtReleaseMutant(MutantHandle:{:#x}, PreviousCount:{:#x})", MutantHandle, PreviousCount);

        for(const auto& it : d.observers_NtReleaseMutant)
            it(MutantHandle, PreviousCount);
    }

    static void on_NtReleaseSemaphore(monitor::syscalls::Data& d)
    {
        const auto SemaphoreHandle = arg<nt::HANDLE>(d.core, 0);
        const auto ReleaseCount    = arg<nt::LONG>(d.core, 1);
        const auto PreviousCount   = arg<nt::PLONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtReleaseSemaphore(SemaphoreHandle:{:#x}, ReleaseCount:{:#x}, PreviousCount:{:#x})", SemaphoreHandle, ReleaseCount, PreviousCount);

        for(const auto& it : d.observers_NtReleaseSemaphore)
            it(SemaphoreHandle, ReleaseCount, PreviousCount);
    }

    static void on_NtReleaseWorkerFactoryWorker(monitor::syscalls::Data& d)
    {
        const auto WorkerFactoryHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtReleaseWorkerFactoryWorker(WorkerFactoryHandle:{:#x})", WorkerFactoryHandle);

        for(const auto& it : d.observers_NtReleaseWorkerFactoryWorker)
            it(WorkerFactoryHandle);
    }

    static void on_NtRemoveIoCompletionEx(monitor::syscalls::Data& d)
    {
        const auto IoCompletionHandle      = arg<nt::HANDLE>(d.core, 0);
        const auto IoCompletionInformation = arg<nt::PFILE_IO_COMPLETION_INFORMATION>(d.core, 1);
        const auto Count                   = arg<nt::ULONG>(d.core, 2);
        const auto NumEntriesRemoved       = arg<nt::PULONG>(d.core, 3);
        const auto Timeout                 = arg<nt::PLARGE_INTEGER>(d.core, 4);
        const auto Alertable               = arg<nt::BOOLEAN>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtRemoveIoCompletionEx(IoCompletionHandle:{:#x}, IoCompletionInformation:{:#x}, Count:{:#x}, NumEntriesRemoved:{:#x}, Timeout:{:#x}, Alertable:{:#x})", IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable);

        for(const auto& it : d.observers_NtRemoveIoCompletionEx)
            it(IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable);
    }

    static void on_NtRemoveIoCompletion(monitor::syscalls::Data& d)
    {
        const auto IoCompletionHandle = arg<nt::HANDLE>(d.core, 0);
        const auto STARKeyContext     = arg<nt::PVOID>(d.core, 1);
        const auto STARApcContext     = arg<nt::PVOID>(d.core, 2);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(d.core, 3);
        const auto Timeout            = arg<nt::PLARGE_INTEGER>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtRemoveIoCompletion(IoCompletionHandle:{:#x}, STARKeyContext:{:#x}, STARApcContext:{:#x}, IoStatusBlock:{:#x}, Timeout:{:#x})", IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout);

        for(const auto& it : d.observers_NtRemoveIoCompletion)
            it(IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout);
    }

    static void on_NtRemoveProcessDebug(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto DebugObjectHandle = arg<nt::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtRemoveProcessDebug(ProcessHandle:{:#x}, DebugObjectHandle:{:#x})", ProcessHandle, DebugObjectHandle);

        for(const auto& it : d.observers_NtRemoveProcessDebug)
            it(ProcessHandle, DebugObjectHandle);
    }

    static void on_NtRenameKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle = arg<nt::HANDLE>(d.core, 0);
        const auto NewName   = arg<nt::PUNICODE_STRING>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtRenameKey(KeyHandle:{:#x}, NewName:{:#x})", KeyHandle, NewName);

        for(const auto& it : d.observers_NtRenameKey)
            it(KeyHandle, NewName);
    }

    static void on_NtRenameTransactionManager(monitor::syscalls::Data& d)
    {
        const auto LogFileName                    = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto ExistingTransactionManagerGuid = arg<nt::LPGUID>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtRenameTransactionManager(LogFileName:{:#x}, ExistingTransactionManagerGuid:{:#x})", LogFileName, ExistingTransactionManagerGuid);

        for(const auto& it : d.observers_NtRenameTransactionManager)
            it(LogFileName, ExistingTransactionManagerGuid);
    }

    static void on_NtReplaceKey(monitor::syscalls::Data& d)
    {
        const auto NewFile      = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto TargetHandle = arg<nt::HANDLE>(d.core, 1);
        const auto OldFile      = arg<nt::POBJECT_ATTRIBUTES>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtReplaceKey(NewFile:{:#x}, TargetHandle:{:#x}, OldFile:{:#x})", NewFile, TargetHandle, OldFile);

        for(const auto& it : d.observers_NtReplaceKey)
            it(NewFile, TargetHandle, OldFile);
    }

    static void on_NtReplacePartitionUnit(monitor::syscalls::Data& d)
    {
        const auto TargetInstancePath = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto SpareInstancePath  = arg<nt::PUNICODE_STRING>(d.core, 1);
        const auto Flags              = arg<nt::ULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtReplacePartitionUnit(TargetInstancePath:{:#x}, SpareInstancePath:{:#x}, Flags:{:#x})", TargetInstancePath, SpareInstancePath, Flags);

        for(const auto& it : d.observers_NtReplacePartitionUnit)
            it(TargetInstancePath, SpareInstancePath, Flags);
    }

    static void on_NtReplyPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto ReplyMessage = arg<nt::PPORT_MESSAGE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtReplyPort(PortHandle:{:#x}, ReplyMessage:{:#x})", PortHandle, ReplyMessage);

        for(const auto& it : d.observers_NtReplyPort)
            it(PortHandle, ReplyMessage);
    }

    static void on_NtReplyWaitReceivePortEx(monitor::syscalls::Data& d)
    {
        const auto PortHandle      = arg<nt::HANDLE>(d.core, 0);
        const auto STARPortContext = arg<nt::PVOID>(d.core, 1);
        const auto ReplyMessage    = arg<nt::PPORT_MESSAGE>(d.core, 2);
        const auto ReceiveMessage  = arg<nt::PPORT_MESSAGE>(d.core, 3);
        const auto Timeout         = arg<nt::PLARGE_INTEGER>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtReplyWaitReceivePortEx(PortHandle:{:#x}, STARPortContext:{:#x}, ReplyMessage:{:#x}, ReceiveMessage:{:#x}, Timeout:{:#x})", PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout);

        for(const auto& it : d.observers_NtReplyWaitReceivePortEx)
            it(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout);
    }

    static void on_NtReplyWaitReceivePort(monitor::syscalls::Data& d)
    {
        const auto PortHandle      = arg<nt::HANDLE>(d.core, 0);
        const auto STARPortContext = arg<nt::PVOID>(d.core, 1);
        const auto ReplyMessage    = arg<nt::PPORT_MESSAGE>(d.core, 2);
        const auto ReceiveMessage  = arg<nt::PPORT_MESSAGE>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtReplyWaitReceivePort(PortHandle:{:#x}, STARPortContext:{:#x}, ReplyMessage:{:#x}, ReceiveMessage:{:#x})", PortHandle, STARPortContext, ReplyMessage, ReceiveMessage);

        for(const auto& it : d.observers_NtReplyWaitReceivePort)
            it(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage);
    }

    static void on_NtReplyWaitReplyPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto ReplyMessage = arg<nt::PPORT_MESSAGE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtReplyWaitReplyPort(PortHandle:{:#x}, ReplyMessage:{:#x})", PortHandle, ReplyMessage);

        for(const auto& it : d.observers_NtReplyWaitReplyPort)
            it(PortHandle, ReplyMessage);
    }

    static void on_NtRequestPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto RequestMessage = arg<nt::PPORT_MESSAGE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtRequestPort(PortHandle:{:#x}, RequestMessage:{:#x})", PortHandle, RequestMessage);

        for(const auto& it : d.observers_NtRequestPort)
            it(PortHandle, RequestMessage);
    }

    static void on_NtRequestWaitReplyPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto RequestMessage = arg<nt::PPORT_MESSAGE>(d.core, 1);
        const auto ReplyMessage   = arg<nt::PPORT_MESSAGE>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtRequestWaitReplyPort(PortHandle:{:#x}, RequestMessage:{:#x}, ReplyMessage:{:#x})", PortHandle, RequestMessage, ReplyMessage);

        for(const auto& it : d.observers_NtRequestWaitReplyPort)
            it(PortHandle, RequestMessage, ReplyMessage);
    }

    static void on_NtResetEvent(monitor::syscalls::Data& d)
    {
        const auto EventHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto PreviousState = arg<nt::PLONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtResetEvent(EventHandle:{:#x}, PreviousState:{:#x})", EventHandle, PreviousState);

        for(const auto& it : d.observers_NtResetEvent)
            it(EventHandle, PreviousState);
    }

    static void on_NtResetWriteWatch(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 0);
        const auto BaseAddress   = arg<nt::PVOID>(d.core, 1);
        const auto RegionSize    = arg<nt::SIZE_T>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtResetWriteWatch(ProcessHandle:{:#x}, BaseAddress:{:#x}, RegionSize:{:#x})", ProcessHandle, BaseAddress, RegionSize);

        for(const auto& it : d.observers_NtResetWriteWatch)
            it(ProcessHandle, BaseAddress, RegionSize);
    }

    static void on_NtRestoreKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto FileHandle = arg<nt::HANDLE>(d.core, 1);
        const auto Flags      = arg<nt::ULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtRestoreKey(KeyHandle:{:#x}, FileHandle:{:#x}, Flags:{:#x})", KeyHandle, FileHandle, Flags);

        for(const auto& it : d.observers_NtRestoreKey)
            it(KeyHandle, FileHandle, Flags);
    }

    static void on_NtResumeProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtResumeProcess(ProcessHandle:{:#x})", ProcessHandle);

        for(const auto& it : d.observers_NtResumeProcess)
            it(ProcessHandle);
    }

    static void on_NtResumeThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle         = arg<nt::HANDLE>(d.core, 0);
        const auto PreviousSuspendCount = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtResumeThread(ThreadHandle:{:#x}, PreviousSuspendCount:{:#x})", ThreadHandle, PreviousSuspendCount);

        for(const auto& it : d.observers_NtResumeThread)
            it(ThreadHandle, PreviousSuspendCount);
    }

    static void on_NtRollbackComplete(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtRollbackComplete(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtRollbackComplete)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtRollbackEnlistment(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtRollbackEnlistment(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtRollbackEnlistment)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtRollbackTransaction(monitor::syscalls::Data& d)
    {
        const auto TransactionHandle = arg<nt::HANDLE>(d.core, 0);
        const auto Wait              = arg<nt::BOOLEAN>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtRollbackTransaction(TransactionHandle:{:#x}, Wait:{:#x})", TransactionHandle, Wait);

        for(const auto& it : d.observers_NtRollbackTransaction)
            it(TransactionHandle, Wait);
    }

    static void on_NtRollforwardTransactionManager(monitor::syscalls::Data& d)
    {
        const auto TransactionManagerHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock           = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtRollforwardTransactionManager(TransactionManagerHandle:{:#x}, TmVirtualClock:{:#x})", TransactionManagerHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtRollforwardTransactionManager)
            it(TransactionManagerHandle, TmVirtualClock);
    }

    static void on_NtSaveKeyEx(monitor::syscalls::Data& d)
    {
        const auto KeyHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto FileHandle = arg<nt::HANDLE>(d.core, 1);
        const auto Format     = arg<nt::ULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtSaveKeyEx(KeyHandle:{:#x}, FileHandle:{:#x}, Format:{:#x})", KeyHandle, FileHandle, Format);

        for(const auto& it : d.observers_NtSaveKeyEx)
            it(KeyHandle, FileHandle, Format);
    }

    static void on_NtSaveKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto FileHandle = arg<nt::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSaveKey(KeyHandle:{:#x}, FileHandle:{:#x})", KeyHandle, FileHandle);

        for(const auto& it : d.observers_NtSaveKey)
            it(KeyHandle, FileHandle);
    }

    static void on_NtSaveMergedKeys(monitor::syscalls::Data& d)
    {
        const auto HighPrecedenceKeyHandle = arg<nt::HANDLE>(d.core, 0);
        const auto LowPrecedenceKeyHandle  = arg<nt::HANDLE>(d.core, 1);
        const auto FileHandle              = arg<nt::HANDLE>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtSaveMergedKeys(HighPrecedenceKeyHandle:{:#x}, LowPrecedenceKeyHandle:{:#x}, FileHandle:{:#x})", HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);

        for(const auto& it : d.observers_NtSaveMergedKeys)
            it(HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);
    }

    static void on_NtSecureConnectPort(monitor::syscalls::Data& d)
    {
        const auto PortHandle                  = arg<nt::PHANDLE>(d.core, 0);
        const auto PortName                    = arg<nt::PUNICODE_STRING>(d.core, 1);
        const auto SecurityQos                 = arg<nt::PSECURITY_QUALITY_OF_SERVICE>(d.core, 2);
        const auto ClientView                  = arg<nt::PPORT_VIEW>(d.core, 3);
        const auto RequiredServerSid           = arg<nt::PSID>(d.core, 4);
        const auto ServerView                  = arg<nt::PREMOTE_PORT_VIEW>(d.core, 5);
        const auto MaxMessageLength            = arg<nt::PULONG>(d.core, 6);
        const auto ConnectionInformation       = arg<nt::PVOID>(d.core, 7);
        const auto ConnectionInformationLength = arg<nt::PULONG>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtSecureConnectPort(PortHandle:{:#x}, PortName:{:#x}, SecurityQos:{:#x}, ClientView:{:#x}, RequiredServerSid:{:#x}, ServerView:{:#x}, MaxMessageLength:{:#x}, ConnectionInformation:{:#x}, ConnectionInformationLength:{:#x})", PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);

        for(const auto& it : d.observers_NtSecureConnectPort)
            it(PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    }

    static void on_NtSetBootEntryOrder(monitor::syscalls::Data& d)
    {
        const auto Ids   = arg<nt::PULONG>(d.core, 0);
        const auto Count = arg<nt::ULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSetBootEntryOrder(Ids:{:#x}, Count:{:#x})", Ids, Count);

        for(const auto& it : d.observers_NtSetBootEntryOrder)
            it(Ids, Count);
    }

    static void on_NtSetBootOptions(monitor::syscalls::Data& d)
    {
        const auto BootOptions    = arg<nt::PBOOT_OPTIONS>(d.core, 0);
        const auto FieldsToChange = arg<nt::ULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSetBootOptions(BootOptions:{:#x}, FieldsToChange:{:#x})", BootOptions, FieldsToChange);

        for(const auto& it : d.observers_NtSetBootOptions)
            it(BootOptions, FieldsToChange);
    }

    static void on_NtSetContextThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto ThreadContext = arg<nt::PCONTEXT>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSetContextThread(ThreadHandle:{:#x}, ThreadContext:{:#x})", ThreadHandle, ThreadContext);

        for(const auto& it : d.observers_NtSetContextThread)
            it(ThreadHandle, ThreadContext);
    }

    static void on_NtSetDebugFilterState(monitor::syscalls::Data& d)
    {
        const auto ComponentId = arg<nt::ULONG>(d.core, 0);
        const auto Level       = arg<nt::ULONG>(d.core, 1);
        const auto State       = arg<nt::BOOLEAN>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtSetDebugFilterState(ComponentId:{:#x}, Level:{:#x}, State:{:#x})", ComponentId, Level, State);

        for(const auto& it : d.observers_NtSetDebugFilterState)
            it(ComponentId, Level, State);
    }

    static void on_NtSetDefaultHardErrorPort(monitor::syscalls::Data& d)
    {
        const auto DefaultHardErrorPort = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtSetDefaultHardErrorPort(DefaultHardErrorPort:{:#x})", DefaultHardErrorPort);

        for(const auto& it : d.observers_NtSetDefaultHardErrorPort)
            it(DefaultHardErrorPort);
    }

    static void on_NtSetDefaultLocale(monitor::syscalls::Data& d)
    {
        const auto UserProfile     = arg<nt::BOOLEAN>(d.core, 0);
        const auto DefaultLocaleId = arg<nt::LCID>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSetDefaultLocale(UserProfile:{:#x}, DefaultLocaleId:{:#x})", UserProfile, DefaultLocaleId);

        for(const auto& it : d.observers_NtSetDefaultLocale)
            it(UserProfile, DefaultLocaleId);
    }

    static void on_NtSetDefaultUILanguage(monitor::syscalls::Data& d)
    {
        const auto DefaultUILanguageId = arg<nt::LANGID>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtSetDefaultUILanguage(DefaultUILanguageId:{:#x})", DefaultUILanguageId);

        for(const auto& it : d.observers_NtSetDefaultUILanguage)
            it(DefaultUILanguageId);
    }

    static void on_NtSetDriverEntryOrder(monitor::syscalls::Data& d)
    {
        const auto Ids   = arg<nt::PULONG>(d.core, 0);
        const auto Count = arg<nt::ULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSetDriverEntryOrder(Ids:{:#x}, Count:{:#x})", Ids, Count);

        for(const auto& it : d.observers_NtSetDriverEntryOrder)
            it(Ids, Count);
    }

    static void on_NtSetEaFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto Buffer        = arg<nt::PVOID>(d.core, 2);
        const auto Length        = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetEaFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x})", FileHandle, IoStatusBlock, Buffer, Length);

        for(const auto& it : d.observers_NtSetEaFile)
            it(FileHandle, IoStatusBlock, Buffer, Length);
    }

    static void on_NtSetEventBoostPriority(monitor::syscalls::Data& d)
    {
        const auto EventHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtSetEventBoostPriority(EventHandle:{:#x})", EventHandle);

        for(const auto& it : d.observers_NtSetEventBoostPriority)
            it(EventHandle);
    }

    static void on_NtSetEvent(monitor::syscalls::Data& d)
    {
        const auto EventHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto PreviousState = arg<nt::PLONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSetEvent(EventHandle:{:#x}, PreviousState:{:#x})", EventHandle, PreviousState);

        for(const auto& it : d.observers_NtSetEvent)
            it(EventHandle, PreviousState);
    }

    static void on_NtSetHighEventPair(monitor::syscalls::Data& d)
    {
        const auto EventPairHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtSetHighEventPair(EventPairHandle:{:#x})", EventPairHandle);

        for(const auto& it : d.observers_NtSetHighEventPair)
            it(EventPairHandle);
    }

    static void on_NtSetHighWaitLowEventPair(monitor::syscalls::Data& d)
    {
        const auto EventPairHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtSetHighWaitLowEventPair(EventPairHandle:{:#x})", EventPairHandle);

        for(const auto& it : d.observers_NtSetHighWaitLowEventPair)
            it(EventPairHandle);
    }

    static void on_NtSetInformationDebugObject(monitor::syscalls::Data& d)
    {
        const auto DebugObjectHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto DebugObjectInformationClass = arg<nt::DEBUGOBJECTINFOCLASS>(d.core, 1);
        const auto DebugInformation            = arg<nt::PVOID>(d.core, 2);
        const auto DebugInformationLength      = arg<nt::ULONG>(d.core, 3);
        const auto ReturnLength                = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationDebugObject(DebugObjectHandle:{:#x}, DebugObjectInformationClass:{:#x}, DebugInformation:{:#x}, DebugInformationLength:{:#x}, ReturnLength:{:#x})", DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength);

        for(const auto& it : d.observers_NtSetInformationDebugObject)
            it(DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength);
    }

    static void on_NtSetInformationEnlistment(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto EnlistmentInformationClass  = arg<nt::ENLISTMENT_INFORMATION_CLASS>(d.core, 1);
        const auto EnlistmentInformation       = arg<nt::PVOID>(d.core, 2);
        const auto EnlistmentInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationEnlistment(EnlistmentHandle:{:#x}, EnlistmentInformationClass:{:#x}, EnlistmentInformation:{:#x}, EnlistmentInformationLength:{:#x})", EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength);

        for(const auto& it : d.observers_NtSetInformationEnlistment)
            it(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength);
    }

    static void on_NtSetInformationFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock        = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto FileInformation      = arg<nt::PVOID>(d.core, 2);
        const auto Length               = arg<nt::ULONG>(d.core, 3);
        const auto FileInformationClass = arg<nt::FILE_INFORMATION_CLASS>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, FileInformation:{:#x}, Length:{:#x}, FileInformationClass:{:#x})", FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);

        for(const auto& it : d.observers_NtSetInformationFile)
            it(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    }

    static void on_NtSetInformationJobObject(monitor::syscalls::Data& d)
    {
        const auto JobHandle                  = arg<nt::HANDLE>(d.core, 0);
        const auto JobObjectInformationClass  = arg<nt::JOBOBJECTINFOCLASS>(d.core, 1);
        const auto JobObjectInformation       = arg<nt::PVOID>(d.core, 2);
        const auto JobObjectInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationJobObject(JobHandle:{:#x}, JobObjectInformationClass:{:#x}, JobObjectInformation:{:#x}, JobObjectInformationLength:{:#x})", JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength);

        for(const auto& it : d.observers_NtSetInformationJobObject)
            it(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength);
    }

    static void on_NtSetInformationKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle               = arg<nt::HANDLE>(d.core, 0);
        const auto KeySetInformationClass  = arg<nt::KEY_SET_INFORMATION_CLASS>(d.core, 1);
        const auto KeySetInformation       = arg<nt::PVOID>(d.core, 2);
        const auto KeySetInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationKey(KeyHandle:{:#x}, KeySetInformationClass:{:#x}, KeySetInformation:{:#x}, KeySetInformationLength:{:#x})", KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength);

        for(const auto& it : d.observers_NtSetInformationKey)
            it(KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength);
    }

    static void on_NtSetInformationObject(monitor::syscalls::Data& d)
    {
        const auto Handle                  = arg<nt::HANDLE>(d.core, 0);
        const auto ObjectInformationClass  = arg<nt::OBJECT_INFORMATION_CLASS>(d.core, 1);
        const auto ObjectInformation       = arg<nt::PVOID>(d.core, 2);
        const auto ObjectInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationObject(Handle:{:#x}, ObjectInformationClass:{:#x}, ObjectInformation:{:#x}, ObjectInformationLength:{:#x})", Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);

        for(const auto& it : d.observers_NtSetInformationObject)
            it(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);
    }

    static void on_NtSetInformationProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto ProcessInformationClass  = arg<nt::PROCESSINFOCLASS>(d.core, 1);
        const auto ProcessInformation       = arg<nt::PVOID>(d.core, 2);
        const auto ProcessInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationProcess(ProcessHandle:{:#x}, ProcessInformationClass:{:#x}, ProcessInformation:{:#x}, ProcessInformationLength:{:#x})", ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);

        for(const auto& it : d.observers_NtSetInformationProcess)
            it(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
    }

    static void on_NtSetInformationResourceManager(monitor::syscalls::Data& d)
    {
        const auto ResourceManagerHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto ResourceManagerInformationClass  = arg<nt::RESOURCEMANAGER_INFORMATION_CLASS>(d.core, 1);
        const auto ResourceManagerInformation       = arg<nt::PVOID>(d.core, 2);
        const auto ResourceManagerInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationResourceManager(ResourceManagerHandle:{:#x}, ResourceManagerInformationClass:{:#x}, ResourceManagerInformation:{:#x}, ResourceManagerInformationLength:{:#x})", ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength);

        for(const auto& it : d.observers_NtSetInformationResourceManager)
            it(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength);
    }

    static void on_NtSetInformationThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto ThreadInformationClass  = arg<nt::THREADINFOCLASS>(d.core, 1);
        const auto ThreadInformation       = arg<nt::PVOID>(d.core, 2);
        const auto ThreadInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationThread(ThreadHandle:{:#x}, ThreadInformationClass:{:#x}, ThreadInformation:{:#x}, ThreadInformationLength:{:#x})", ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);

        for(const auto& it : d.observers_NtSetInformationThread)
            it(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
    }

    static void on_NtSetInformationToken(monitor::syscalls::Data& d)
    {
        const auto TokenHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto TokenInformationClass  = arg<nt::TOKEN_INFORMATION_CLASS>(d.core, 1);
        const auto TokenInformation       = arg<nt::PVOID>(d.core, 2);
        const auto TokenInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationToken(TokenHandle:{:#x}, TokenInformationClass:{:#x}, TokenInformation:{:#x}, TokenInformationLength:{:#x})", TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);

        for(const auto& it : d.observers_NtSetInformationToken)
            it(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);
    }

    static void on_NtSetInformationTransaction(monitor::syscalls::Data& d)
    {
        const auto TransactionHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto TransactionInformationClass  = arg<nt::TRANSACTION_INFORMATION_CLASS>(d.core, 1);
        const auto TransactionInformation       = arg<nt::PVOID>(d.core, 2);
        const auto TransactionInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationTransaction(TransactionHandle:{:#x}, TransactionInformationClass:{:#x}, TransactionInformation:{:#x}, TransactionInformationLength:{:#x})", TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength);

        for(const auto& it : d.observers_NtSetInformationTransaction)
            it(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength);
    }

    static void on_NtSetInformationTransactionManager(monitor::syscalls::Data& d)
    {
        const auto TmHandle                            = arg<nt::HANDLE>(d.core, 0);
        const auto TransactionManagerInformationClass  = arg<nt::TRANSACTIONMANAGER_INFORMATION_CLASS>(d.core, 1);
        const auto TransactionManagerInformation       = arg<nt::PVOID>(d.core, 2);
        const auto TransactionManagerInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationTransactionManager(TmHandle:{:#x}, TransactionManagerInformationClass:{:#x}, TransactionManagerInformation:{:#x}, TransactionManagerInformationLength:{:#x})", TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength);

        for(const auto& it : d.observers_NtSetInformationTransactionManager)
            it(TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength);
    }

    static void on_NtSetInformationWorkerFactory(monitor::syscalls::Data& d)
    {
        const auto WorkerFactoryHandle            = arg<nt::HANDLE>(d.core, 0);
        const auto WorkerFactoryInformationClass  = arg<nt::WORKERFACTORYINFOCLASS>(d.core, 1);
        const auto WorkerFactoryInformation       = arg<nt::PVOID>(d.core, 2);
        const auto WorkerFactoryInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetInformationWorkerFactory(WorkerFactoryHandle:{:#x}, WorkerFactoryInformationClass:{:#x}, WorkerFactoryInformation:{:#x}, WorkerFactoryInformationLength:{:#x})", WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength);

        for(const auto& it : d.observers_NtSetInformationWorkerFactory)
            it(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength);
    }

    static void on_NtSetIntervalProfile(monitor::syscalls::Data& d)
    {
        const auto Interval = arg<nt::ULONG>(d.core, 0);
        const auto Source   = arg<nt::KPROFILE_SOURCE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSetIntervalProfile(Interval:{:#x}, Source:{:#x})", Interval, Source);

        for(const auto& it : d.observers_NtSetIntervalProfile)
            it(Interval, Source);
    }

    static void on_NtSetIoCompletionEx(monitor::syscalls::Data& d)
    {
        const auto IoCompletionHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto IoCompletionReserveHandle = arg<nt::HANDLE>(d.core, 1);
        const auto KeyContext                = arg<nt::PVOID>(d.core, 2);
        const auto ApcContext                = arg<nt::PVOID>(d.core, 3);
        const auto IoStatus                  = arg<nt::NTSTATUS>(d.core, 4);
        const auto IoStatusInformation       = arg<nt::ULONG_PTR>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtSetIoCompletionEx(IoCompletionHandle:{:#x}, IoCompletionReserveHandle:{:#x}, KeyContext:{:#x}, ApcContext:{:#x}, IoStatus:{:#x}, IoStatusInformation:{:#x})", IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);

        for(const auto& it : d.observers_NtSetIoCompletionEx)
            it(IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    }

    static void on_NtSetIoCompletion(monitor::syscalls::Data& d)
    {
        const auto IoCompletionHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto KeyContext          = arg<nt::PVOID>(d.core, 1);
        const auto ApcContext          = arg<nt::PVOID>(d.core, 2);
        const auto IoStatus            = arg<nt::NTSTATUS>(d.core, 3);
        const auto IoStatusInformation = arg<nt::ULONG_PTR>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtSetIoCompletion(IoCompletionHandle:{:#x}, KeyContext:{:#x}, ApcContext:{:#x}, IoStatus:{:#x}, IoStatusInformation:{:#x})", IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);

        for(const auto& it : d.observers_NtSetIoCompletion)
            it(IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    }

    static void on_NtSetLdtEntries(monitor::syscalls::Data& d)
    {
        const auto Selector0 = arg<nt::ULONG>(d.core, 0);
        const auto Entry0Low = arg<nt::ULONG>(d.core, 1);
        const auto Entry0Hi  = arg<nt::ULONG>(d.core, 2);
        const auto Selector1 = arg<nt::ULONG>(d.core, 3);
        const auto Entry1Low = arg<nt::ULONG>(d.core, 4);
        const auto Entry1Hi  = arg<nt::ULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtSetLdtEntries(Selector0:{:#x}, Entry0Low:{:#x}, Entry0Hi:{:#x}, Selector1:{:#x}, Entry1Low:{:#x}, Entry1Hi:{:#x})", Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);

        for(const auto& it : d.observers_NtSetLdtEntries)
            it(Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);
    }

    static void on_NtSetLowEventPair(monitor::syscalls::Data& d)
    {
        const auto EventPairHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtSetLowEventPair(EventPairHandle:{:#x})", EventPairHandle);

        for(const auto& it : d.observers_NtSetLowEventPair)
            it(EventPairHandle);
    }

    static void on_NtSetLowWaitHighEventPair(monitor::syscalls::Data& d)
    {
        const auto EventPairHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtSetLowWaitHighEventPair(EventPairHandle:{:#x})", EventPairHandle);

        for(const auto& it : d.observers_NtSetLowWaitHighEventPair)
            it(EventPairHandle);
    }

    static void on_NtSetQuotaInformationFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto Buffer        = arg<nt::PVOID>(d.core, 2);
        const auto Length        = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetQuotaInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x})", FileHandle, IoStatusBlock, Buffer, Length);

        for(const auto& it : d.observers_NtSetQuotaInformationFile)
            it(FileHandle, IoStatusBlock, Buffer, Length);
    }

    static void on_NtSetSecurityObject(monitor::syscalls::Data& d)
    {
        const auto Handle              = arg<nt::HANDLE>(d.core, 0);
        const auto SecurityInformation = arg<nt::SECURITY_INFORMATION>(d.core, 1);
        const auto SecurityDescriptor  = arg<nt::PSECURITY_DESCRIPTOR>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtSetSecurityObject(Handle:{:#x}, SecurityInformation:{:#x}, SecurityDescriptor:{:#x})", Handle, SecurityInformation, SecurityDescriptor);

        for(const auto& it : d.observers_NtSetSecurityObject)
            it(Handle, SecurityInformation, SecurityDescriptor);
    }

    static void on_NtSetSystemEnvironmentValueEx(monitor::syscalls::Data& d)
    {
        const auto VariableName = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto VendorGuid   = arg<nt::LPGUID>(d.core, 1);
        const auto Value        = arg<nt::PVOID>(d.core, 2);
        const auto ValueLength  = arg<nt::ULONG>(d.core, 3);
        const auto Attributes   = arg<nt::ULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtSetSystemEnvironmentValueEx(VariableName:{:#x}, VendorGuid:{:#x}, Value:{:#x}, ValueLength:{:#x}, Attributes:{:#x})", VariableName, VendorGuid, Value, ValueLength, Attributes);

        for(const auto& it : d.observers_NtSetSystemEnvironmentValueEx)
            it(VariableName, VendorGuid, Value, ValueLength, Attributes);
    }

    static void on_NtSetSystemEnvironmentValue(monitor::syscalls::Data& d)
    {
        const auto VariableName  = arg<nt::PUNICODE_STRING>(d.core, 0);
        const auto VariableValue = arg<nt::PUNICODE_STRING>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSetSystemEnvironmentValue(VariableName:{:#x}, VariableValue:{:#x})", VariableName, VariableValue);

        for(const auto& it : d.observers_NtSetSystemEnvironmentValue)
            it(VariableName, VariableValue);
    }

    static void on_NtSetSystemInformation(monitor::syscalls::Data& d)
    {
        const auto SystemInformationClass  = arg<nt::SYSTEM_INFORMATION_CLASS>(d.core, 0);
        const auto SystemInformation       = arg<nt::PVOID>(d.core, 1);
        const auto SystemInformationLength = arg<nt::ULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtSetSystemInformation(SystemInformationClass:{:#x}, SystemInformation:{:#x}, SystemInformationLength:{:#x})", SystemInformationClass, SystemInformation, SystemInformationLength);

        for(const auto& it : d.observers_NtSetSystemInformation)
            it(SystemInformationClass, SystemInformation, SystemInformationLength);
    }

    static void on_NtSetSystemPowerState(monitor::syscalls::Data& d)
    {
        const auto SystemAction   = arg<nt::POWER_ACTION>(d.core, 0);
        const auto MinSystemState = arg<nt::SYSTEM_POWER_STATE>(d.core, 1);
        const auto Flags          = arg<nt::ULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtSetSystemPowerState(SystemAction:{:#x}, MinSystemState:{:#x}, Flags:{:#x})", SystemAction, MinSystemState, Flags);

        for(const auto& it : d.observers_NtSetSystemPowerState)
            it(SystemAction, MinSystemState, Flags);
    }

    static void on_NtSetSystemTime(monitor::syscalls::Data& d)
    {
        const auto SystemTime   = arg<nt::PLARGE_INTEGER>(d.core, 0);
        const auto PreviousTime = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSetSystemTime(SystemTime:{:#x}, PreviousTime:{:#x})", SystemTime, PreviousTime);

        for(const auto& it : d.observers_NtSetSystemTime)
            it(SystemTime, PreviousTime);
    }

    static void on_NtSetThreadExecutionState(monitor::syscalls::Data& d)
    {
        const auto esFlags           = arg<nt::EXECUTION_STATE>(d.core, 0);
        const auto STARPreviousFlags = arg<nt::EXECUTION_STATE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSetThreadExecutionState(esFlags:{:#x}, STARPreviousFlags:{:#x})", esFlags, STARPreviousFlags);

        for(const auto& it : d.observers_NtSetThreadExecutionState)
            it(esFlags, STARPreviousFlags);
    }

    static void on_NtSetTimerEx(monitor::syscalls::Data& d)
    {
        const auto TimerHandle               = arg<nt::HANDLE>(d.core, 0);
        const auto TimerSetInformationClass  = arg<nt::TIMER_SET_INFORMATION_CLASS>(d.core, 1);
        const auto TimerSetInformation       = arg<nt::PVOID>(d.core, 2);
        const auto TimerSetInformationLength = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSetTimerEx(TimerHandle:{:#x}, TimerSetInformationClass:{:#x}, TimerSetInformation:{:#x}, TimerSetInformationLength:{:#x})", TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength);

        for(const auto& it : d.observers_NtSetTimerEx)
            it(TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength);
    }

    static void on_NtSetTimer(monitor::syscalls::Data& d)
    {
        const auto TimerHandle     = arg<nt::HANDLE>(d.core, 0);
        const auto DueTime         = arg<nt::PLARGE_INTEGER>(d.core, 1);
        const auto TimerApcRoutine = arg<nt::PTIMER_APC_ROUTINE>(d.core, 2);
        const auto TimerContext    = arg<nt::PVOID>(d.core, 3);
        const auto WakeTimer       = arg<nt::BOOLEAN>(d.core, 4);
        const auto Period          = arg<nt::LONG>(d.core, 5);
        const auto PreviousState   = arg<nt::PBOOLEAN>(d.core, 6);

        if constexpr(g_debug)
            LOG(INFO, "NtSetTimer(TimerHandle:{:#x}, DueTime:{:#x}, TimerApcRoutine:{:#x}, TimerContext:{:#x}, WakeTimer:{:#x}, Period:{:#x}, PreviousState:{:#x})", TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState);

        for(const auto& it : d.observers_NtSetTimer)
            it(TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState);
    }

    static void on_NtSetTimerResolution(monitor::syscalls::Data& d)
    {
        const auto DesiredTime   = arg<nt::ULONG>(d.core, 0);
        const auto SetResolution = arg<nt::BOOLEAN>(d.core, 1);
        const auto ActualTime    = arg<nt::PULONG>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtSetTimerResolution(DesiredTime:{:#x}, SetResolution:{:#x}, ActualTime:{:#x})", DesiredTime, SetResolution, ActualTime);

        for(const auto& it : d.observers_NtSetTimerResolution)
            it(DesiredTime, SetResolution, ActualTime);
    }

    static void on_NtSetUuidSeed(monitor::syscalls::Data& d)
    {
        const auto Seed = arg<nt::PCHAR>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtSetUuidSeed(Seed:{:#x})", Seed);

        for(const auto& it : d.observers_NtSetUuidSeed)
            it(Seed);
    }

    static void on_NtSetValueKey(monitor::syscalls::Data& d)
    {
        const auto KeyHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto ValueName  = arg<nt::PUNICODE_STRING>(d.core, 1);
        const auto TitleIndex = arg<nt::ULONG>(d.core, 2);
        const auto Type       = arg<nt::ULONG>(d.core, 3);
        const auto Data       = arg<nt::PVOID>(d.core, 4);
        const auto DataSize   = arg<nt::ULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtSetValueKey(KeyHandle:{:#x}, ValueName:{:#x}, TitleIndex:{:#x}, Type:{:#x}, Data:{:#x}, DataSize:{:#x})", KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);

        for(const auto& it : d.observers_NtSetValueKey)
            it(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
    }

    static void on_NtSetVolumeInformationFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle         = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto FsInformation      = arg<nt::PVOID>(d.core, 2);
        const auto Length             = arg<nt::ULONG>(d.core, 3);
        const auto FsInformationClass = arg<nt::FS_INFORMATION_CLASS>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtSetVolumeInformationFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, FsInformation:{:#x}, Length:{:#x}, FsInformationClass:{:#x})", FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);

        for(const auto& it : d.observers_NtSetVolumeInformationFile)
            it(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    }

    static void on_NtShutdownSystem(monitor::syscalls::Data& d)
    {
        const auto Action = arg<nt::SHUTDOWN_ACTION>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtShutdownSystem(Action:{:#x})", Action);

        for(const auto& it : d.observers_NtShutdownSystem)
            it(Action);
    }

    static void on_NtShutdownWorkerFactory(monitor::syscalls::Data& d)
    {
        const auto WorkerFactoryHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto STARPendingWorkerCount = arg<nt::LONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtShutdownWorkerFactory(WorkerFactoryHandle:{:#x}, STARPendingWorkerCount:{:#x})", WorkerFactoryHandle, STARPendingWorkerCount);

        for(const auto& it : d.observers_NtShutdownWorkerFactory)
            it(WorkerFactoryHandle, STARPendingWorkerCount);
    }

    static void on_NtSignalAndWaitForSingleObject(monitor::syscalls::Data& d)
    {
        const auto SignalHandle = arg<nt::HANDLE>(d.core, 0);
        const auto WaitHandle   = arg<nt::HANDLE>(d.core, 1);
        const auto Alertable    = arg<nt::BOOLEAN>(d.core, 2);
        const auto Timeout      = arg<nt::PLARGE_INTEGER>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtSignalAndWaitForSingleObject(SignalHandle:{:#x}, WaitHandle:{:#x}, Alertable:{:#x}, Timeout:{:#x})", SignalHandle, WaitHandle, Alertable, Timeout);

        for(const auto& it : d.observers_NtSignalAndWaitForSingleObject)
            it(SignalHandle, WaitHandle, Alertable, Timeout);
    }

    static void on_NtSinglePhaseReject(monitor::syscalls::Data& d)
    {
        const auto EnlistmentHandle = arg<nt::HANDLE>(d.core, 0);
        const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSinglePhaseReject(EnlistmentHandle:{:#x}, TmVirtualClock:{:#x})", EnlistmentHandle, TmVirtualClock);

        for(const auto& it : d.observers_NtSinglePhaseReject)
            it(EnlistmentHandle, TmVirtualClock);
    }

    static void on_NtStartProfile(monitor::syscalls::Data& d)
    {
        const auto ProfileHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtStartProfile(ProfileHandle:{:#x})", ProfileHandle);

        for(const auto& it : d.observers_NtStartProfile)
            it(ProfileHandle);
    }

    static void on_NtStopProfile(monitor::syscalls::Data& d)
    {
        const auto ProfileHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtStopProfile(ProfileHandle:{:#x})", ProfileHandle);

        for(const auto& it : d.observers_NtStopProfile)
            it(ProfileHandle);
    }

    static void on_NtSuspendProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtSuspendProcess(ProcessHandle:{:#x})", ProcessHandle);

        for(const auto& it : d.observers_NtSuspendProcess)
            it(ProcessHandle);
    }

    static void on_NtSuspendThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle         = arg<nt::HANDLE>(d.core, 0);
        const auto PreviousSuspendCount = arg<nt::PULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtSuspendThread(ThreadHandle:{:#x}, PreviousSuspendCount:{:#x})", ThreadHandle, PreviousSuspendCount);

        for(const auto& it : d.observers_NtSuspendThread)
            it(ThreadHandle, PreviousSuspendCount);
    }

    static void on_NtSystemDebugControl(monitor::syscalls::Data& d)
    {
        const auto Command            = arg<nt::SYSDBG_COMMAND>(d.core, 0);
        const auto InputBuffer        = arg<nt::PVOID>(d.core, 1);
        const auto InputBufferLength  = arg<nt::ULONG>(d.core, 2);
        const auto OutputBuffer       = arg<nt::PVOID>(d.core, 3);
        const auto OutputBufferLength = arg<nt::ULONG>(d.core, 4);
        const auto ReturnLength       = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtSystemDebugControl(Command:{:#x}, InputBuffer:{:#x}, InputBufferLength:{:#x}, OutputBuffer:{:#x}, OutputBufferLength:{:#x}, ReturnLength:{:#x})", Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);

        for(const auto& it : d.observers_NtSystemDebugControl)
            it(Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);
    }

    static void on_NtTerminateJobObject(monitor::syscalls::Data& d)
    {
        const auto JobHandle  = arg<nt::HANDLE>(d.core, 0);
        const auto ExitStatus = arg<nt::NTSTATUS>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtTerminateJobObject(JobHandle:{:#x}, ExitStatus:{:#x})", JobHandle, ExitStatus);

        for(const auto& it : d.observers_NtTerminateJobObject)
            it(JobHandle, ExitStatus);
    }

    static void on_NtTerminateProcess(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 0);
        const auto ExitStatus    = arg<nt::NTSTATUS>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtTerminateProcess(ProcessHandle:{:#x}, ExitStatus:{:#x})", ProcessHandle, ExitStatus);

        for(const auto& it : d.observers_NtTerminateProcess)
            it(ProcessHandle, ExitStatus);
    }

    static void on_NtTerminateThread(monitor::syscalls::Data& d)
    {
        const auto ThreadHandle = arg<nt::HANDLE>(d.core, 0);
        const auto ExitStatus   = arg<nt::NTSTATUS>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtTerminateThread(ThreadHandle:{:#x}, ExitStatus:{:#x})", ThreadHandle, ExitStatus);

        for(const auto& it : d.observers_NtTerminateThread)
            it(ThreadHandle, ExitStatus);
    }

    static void on_NtTraceControl(monitor::syscalls::Data& d)
    {
        const auto FunctionCode = arg<nt::ULONG>(d.core, 0);
        const auto InBuffer     = arg<nt::PVOID>(d.core, 1);
        const auto InBufferLen  = arg<nt::ULONG>(d.core, 2);
        const auto OutBuffer    = arg<nt::PVOID>(d.core, 3);
        const auto OutBufferLen = arg<nt::ULONG>(d.core, 4);
        const auto ReturnLength = arg<nt::PULONG>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtTraceControl(FunctionCode:{:#x}, InBuffer:{:#x}, InBufferLen:{:#x}, OutBuffer:{:#x}, OutBufferLen:{:#x}, ReturnLength:{:#x})", FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength);

        for(const auto& it : d.observers_NtTraceControl)
            it(FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength);
    }

    static void on_NtTraceEvent(monitor::syscalls::Data& d)
    {
        const auto TraceHandle = arg<nt::HANDLE>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto FieldSize   = arg<nt::ULONG>(d.core, 2);
        const auto Fields      = arg<nt::PVOID>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtTraceEvent(TraceHandle:{:#x}, Flags:{:#x}, FieldSize:{:#x}, Fields:{:#x})", TraceHandle, Flags, FieldSize, Fields);

        for(const auto& it : d.observers_NtTraceEvent)
            it(TraceHandle, Flags, FieldSize, Fields);
    }

    static void on_NtTranslateFilePath(monitor::syscalls::Data& d)
    {
        const auto InputFilePath        = arg<nt::PFILE_PATH>(d.core, 0);
        const auto OutputType           = arg<nt::ULONG>(d.core, 1);
        const auto OutputFilePath       = arg<nt::PFILE_PATH>(d.core, 2);
        const auto OutputFilePathLength = arg<nt::PULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtTranslateFilePath(InputFilePath:{:#x}, OutputType:{:#x}, OutputFilePath:{:#x}, OutputFilePathLength:{:#x})", InputFilePath, OutputType, OutputFilePath, OutputFilePathLength);

        for(const auto& it : d.observers_NtTranslateFilePath)
            it(InputFilePath, OutputType, OutputFilePath, OutputFilePathLength);
    }

    static void on_NtUnloadDriver(monitor::syscalls::Data& d)
    {
        const auto DriverServiceName = arg<nt::PUNICODE_STRING>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtUnloadDriver(DriverServiceName:{:#x})", DriverServiceName);

        for(const auto& it : d.observers_NtUnloadDriver)
            it(DriverServiceName);
    }

    static void on_NtUnloadKey2(monitor::syscalls::Data& d)
    {
        const auto TargetKey = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto Flags     = arg<nt::ULONG>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtUnloadKey2(TargetKey:{:#x}, Flags:{:#x})", TargetKey, Flags);

        for(const auto& it : d.observers_NtUnloadKey2)
            it(TargetKey, Flags);
    }

    static void on_NtUnloadKeyEx(monitor::syscalls::Data& d)
    {
        const auto TargetKey = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);
        const auto Event     = arg<nt::HANDLE>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtUnloadKeyEx(TargetKey:{:#x}, Event:{:#x})", TargetKey, Event);

        for(const auto& it : d.observers_NtUnloadKeyEx)
            it(TargetKey, Event);
    }

    static void on_NtUnloadKey(monitor::syscalls::Data& d)
    {
        const auto TargetKey = arg<nt::POBJECT_ATTRIBUTES>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtUnloadKey(TargetKey:{:#x})", TargetKey);

        for(const auto& it : d.observers_NtUnloadKey)
            it(TargetKey);
    }

    static void on_NtUnlockFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(d.core, 1);
        const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(d.core, 2);
        const auto Length        = arg<nt::PLARGE_INTEGER>(d.core, 3);
        const auto Key           = arg<nt::ULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtUnlockFile(FileHandle:{:#x}, IoStatusBlock:{:#x}, ByteOffset:{:#x}, Length:{:#x}, Key:{:#x})", FileHandle, IoStatusBlock, ByteOffset, Length, Key);

        for(const auto& it : d.observers_NtUnlockFile)
            it(FileHandle, IoStatusBlock, ByteOffset, Length, Key);
    }

    static void on_NtUnlockVirtualMemory(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle   = arg<nt::HANDLE>(d.core, 0);
        const auto STARBaseAddress = arg<nt::PVOID>(d.core, 1);
        const auto RegionSize      = arg<nt::PSIZE_T>(d.core, 2);
        const auto MapType         = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtUnlockVirtualMemory(ProcessHandle:{:#x}, STARBaseAddress:{:#x}, RegionSize:{:#x}, MapType:{:#x})", ProcessHandle, STARBaseAddress, RegionSize, MapType);

        for(const auto& it : d.observers_NtUnlockVirtualMemory)
            it(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    }

    static void on_NtUnmapViewOfSection(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle = arg<nt::HANDLE>(d.core, 0);
        const auto BaseAddress   = arg<nt::PVOID>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtUnmapViewOfSection(ProcessHandle:{:#x}, BaseAddress:{:#x})", ProcessHandle, BaseAddress);

        for(const auto& it : d.observers_NtUnmapViewOfSection)
            it(ProcessHandle, BaseAddress);
    }

    static void on_NtVdmControl(monitor::syscalls::Data& d)
    {
        const auto Service     = arg<nt::VDMSERVICECLASS>(d.core, 0);
        const auto ServiceData = arg<nt::PVOID>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtVdmControl(Service:{:#x}, ServiceData:{:#x})", Service, ServiceData);

        for(const auto& it : d.observers_NtVdmControl)
            it(Service, ServiceData);
    }

    static void on_NtWaitForDebugEvent(monitor::syscalls::Data& d)
    {
        const auto DebugObjectHandle = arg<nt::HANDLE>(d.core, 0);
        const auto Alertable         = arg<nt::BOOLEAN>(d.core, 1);
        const auto Timeout           = arg<nt::PLARGE_INTEGER>(d.core, 2);
        const auto WaitStateChange   = arg<nt::PDBGUI_WAIT_STATE_CHANGE>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtWaitForDebugEvent(DebugObjectHandle:{:#x}, Alertable:{:#x}, Timeout:{:#x}, WaitStateChange:{:#x})", DebugObjectHandle, Alertable, Timeout, WaitStateChange);

        for(const auto& it : d.observers_NtWaitForDebugEvent)
            it(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
    }

    static void on_NtWaitForKeyedEvent(monitor::syscalls::Data& d)
    {
        const auto KeyedEventHandle = arg<nt::HANDLE>(d.core, 0);
        const auto KeyValue         = arg<nt::PVOID>(d.core, 1);
        const auto Alertable        = arg<nt::BOOLEAN>(d.core, 2);
        const auto Timeout          = arg<nt::PLARGE_INTEGER>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "NtWaitForKeyedEvent(KeyedEventHandle:{:#x}, KeyValue:{:#x}, Alertable:{:#x}, Timeout:{:#x})", KeyedEventHandle, KeyValue, Alertable, Timeout);

        for(const auto& it : d.observers_NtWaitForKeyedEvent)
            it(KeyedEventHandle, KeyValue, Alertable, Timeout);
    }

    static void on_NtWaitForMultipleObjects32(monitor::syscalls::Data& d)
    {
        const auto Count     = arg<nt::ULONG>(d.core, 0);
        const auto Handles   = arg<nt::LONG>(d.core, 1);
        const auto WaitType  = arg<nt::WAIT_TYPE>(d.core, 2);
        const auto Alertable = arg<nt::BOOLEAN>(d.core, 3);
        const auto Timeout   = arg<nt::PLARGE_INTEGER>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtWaitForMultipleObjects32(Count:{:#x}, Handles:{:#x}, WaitType:{:#x}, Alertable:{:#x}, Timeout:{:#x})", Count, Handles, WaitType, Alertable, Timeout);

        for(const auto& it : d.observers_NtWaitForMultipleObjects32)
            it(Count, Handles, WaitType, Alertable, Timeout);
    }

    static void on_NtWaitForMultipleObjects(monitor::syscalls::Data& d)
    {
        const auto Count     = arg<nt::ULONG>(d.core, 0);
        const auto Handles   = arg<nt::HANDLE>(d.core, 1);
        const auto WaitType  = arg<nt::WAIT_TYPE>(d.core, 2);
        const auto Alertable = arg<nt::BOOLEAN>(d.core, 3);
        const auto Timeout   = arg<nt::PLARGE_INTEGER>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtWaitForMultipleObjects(Count:{:#x}, Handles:{:#x}, WaitType:{:#x}, Alertable:{:#x}, Timeout:{:#x})", Count, Handles, WaitType, Alertable, Timeout);

        for(const auto& it : d.observers_NtWaitForMultipleObjects)
            it(Count, Handles, WaitType, Alertable, Timeout);
    }

    static void on_NtWaitForSingleObject(monitor::syscalls::Data& d)
    {
        const auto Handle    = arg<nt::HANDLE>(d.core, 0);
        const auto Alertable = arg<nt::BOOLEAN>(d.core, 1);
        const auto Timeout   = arg<nt::PLARGE_INTEGER>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "NtWaitForSingleObject(Handle:{:#x}, Alertable:{:#x}, Timeout:{:#x})", Handle, Alertable, Timeout);

        for(const auto& it : d.observers_NtWaitForSingleObject)
            it(Handle, Alertable, Timeout);
    }

    static void on_NtWaitForWorkViaWorkerFactory(monitor::syscalls::Data& d)
    {
        const auto WorkerFactoryHandle = arg<nt::HANDLE>(d.core, 0);
        const auto MiniPacket          = arg<nt::PFILE_IO_COMPLETION_INFORMATION>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "NtWaitForWorkViaWorkerFactory(WorkerFactoryHandle:{:#x}, MiniPacket:{:#x})", WorkerFactoryHandle, MiniPacket);

        for(const auto& it : d.observers_NtWaitForWorkViaWorkerFactory)
            it(WorkerFactoryHandle, MiniPacket);
    }

    static void on_NtWaitHighEventPair(monitor::syscalls::Data& d)
    {
        const auto EventPairHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtWaitHighEventPair(EventPairHandle:{:#x})", EventPairHandle);

        for(const auto& it : d.observers_NtWaitHighEventPair)
            it(EventPairHandle);
    }

    static void on_NtWaitLowEventPair(monitor::syscalls::Data& d)
    {
        const auto EventPairHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtWaitLowEventPair(EventPairHandle:{:#x})", EventPairHandle);

        for(const auto& it : d.observers_NtWaitLowEventPair)
            it(EventPairHandle);
    }

    static void on_NtWorkerFactoryWorkerReady(monitor::syscalls::Data& d)
    {
        const auto WorkerFactoryHandle = arg<nt::HANDLE>(d.core, 0);

        if constexpr(g_debug)
            LOG(INFO, "NtWorkerFactoryWorkerReady(WorkerFactoryHandle:{:#x})", WorkerFactoryHandle);

        for(const auto& it : d.observers_NtWorkerFactoryWorkerReady)
            it(WorkerFactoryHandle);
    }

    static void on_NtWriteFileGather(monitor::syscalls::Data& d)
    {
        const auto FileHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto Event         = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext    = arg<nt::PVOID>(d.core, 3);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(d.core, 4);
        const auto SegmentArray  = arg<nt::PFILE_SEGMENT_ELEMENT>(d.core, 5);
        const auto Length        = arg<nt::ULONG>(d.core, 6);
        const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(d.core, 7);
        const auto Key           = arg<nt::PULONG>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtWriteFileGather(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, SegmentArray:{:#x}, Length:{:#x}, ByteOffset:{:#x}, Key:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);

        for(const auto& it : d.observers_NtWriteFileGather)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    }

    static void on_NtWriteFile(monitor::syscalls::Data& d)
    {
        const auto FileHandle    = arg<nt::HANDLE>(d.core, 0);
        const auto Event         = arg<nt::HANDLE>(d.core, 1);
        const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(d.core, 2);
        const auto ApcContext    = arg<nt::PVOID>(d.core, 3);
        const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(d.core, 4);
        const auto Buffer        = arg<nt::PVOID>(d.core, 5);
        const auto Length        = arg<nt::ULONG>(d.core, 6);
        const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(d.core, 7);
        const auto Key           = arg<nt::PULONG>(d.core, 8);

        if constexpr(g_debug)
            LOG(INFO, "NtWriteFile(FileHandle:{:#x}, Event:{:#x}, ApcRoutine:{:#x}, ApcContext:{:#x}, IoStatusBlock:{:#x}, Buffer:{:#x}, Length:{:#x}, ByteOffset:{:#x}, Key:{:#x})", FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);

        for(const auto& it : d.observers_NtWriteFile)
            it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    }

    static void on_NtWriteRequestData(monitor::syscalls::Data& d)
    {
        const auto PortHandle           = arg<nt::HANDLE>(d.core, 0);
        const auto Message              = arg<nt::PPORT_MESSAGE>(d.core, 1);
        const auto DataEntryIndex       = arg<nt::ULONG>(d.core, 2);
        const auto Buffer               = arg<nt::PVOID>(d.core, 3);
        const auto BufferSize           = arg<nt::SIZE_T>(d.core, 4);
        const auto NumberOfBytesWritten = arg<nt::PSIZE_T>(d.core, 5);

        if constexpr(g_debug)
            LOG(INFO, "NtWriteRequestData(PortHandle:{:#x}, Message:{:#x}, DataEntryIndex:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, NumberOfBytesWritten:{:#x})", PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten);

        for(const auto& it : d.observers_NtWriteRequestData)
            it(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten);
    }

    static void on_NtWriteVirtualMemory(monitor::syscalls::Data& d)
    {
        const auto ProcessHandle        = arg<nt::HANDLE>(d.core, 0);
        const auto BaseAddress          = arg<nt::PVOID>(d.core, 1);
        const auto Buffer               = arg<nt::PVOID>(d.core, 2);
        const auto BufferSize           = arg<nt::SIZE_T>(d.core, 3);
        const auto NumberOfBytesWritten = arg<nt::PSIZE_T>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "NtWriteVirtualMemory(ProcessHandle:{:#x}, BaseAddress:{:#x}, Buffer:{:#x}, BufferSize:{:#x}, NumberOfBytesWritten:{:#x})", ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);

        for(const auto& it : d.observers_NtWriteVirtualMemory)
            it(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);
    }

    static void on_NtDisableLastKnownGood(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtDisableLastKnownGood()");

        for(const auto& it : d.observers_NtDisableLastKnownGood)
            it();
    }

    static void on_NtEnableLastKnownGood(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtEnableLastKnownGood()");

        for(const auto& it : d.observers_NtEnableLastKnownGood)
            it();
    }

    static void on_NtFlushProcessWriteBuffers(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtFlushProcessWriteBuffers()");

        for(const auto& it : d.observers_NtFlushProcessWriteBuffers)
            it();
    }

    static void on_NtFlushWriteBuffer(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtFlushWriteBuffer()");

        for(const auto& it : d.observers_NtFlushWriteBuffer)
            it();
    }

    static void on_NtGetCurrentProcessorNumber(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtGetCurrentProcessorNumber()");

        for(const auto& it : d.observers_NtGetCurrentProcessorNumber)
            it();
    }

    static void on_NtIsSystemResumeAutomatic(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtIsSystemResumeAutomatic()");

        for(const auto& it : d.observers_NtIsSystemResumeAutomatic)
            it();
    }

    static void on_NtIsUILanguageComitted(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtIsUILanguageComitted()");

        for(const auto& it : d.observers_NtIsUILanguageComitted)
            it();
    }

    static void on_NtQueryPortInformationProcess(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtQueryPortInformationProcess()");

        for(const auto& it : d.observers_NtQueryPortInformationProcess)
            it();
    }

    static void on_NtSerializeBoot(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtSerializeBoot()");

        for(const auto& it : d.observers_NtSerializeBoot)
            it();
    }

    static void on_NtTestAlert(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtTestAlert()");

        for(const auto& it : d.observers_NtTestAlert)
            it();
    }

    static void on_NtThawRegistry(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtThawRegistry()");

        for(const auto& it : d.observers_NtThawRegistry)
            it();
    }

    static void on_NtThawTransactions(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtThawTransactions()");

        for(const auto& it : d.observers_NtThawTransactions)
            it();
    }

    static void on_NtUmsThreadYield(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtUmsThreadYield()");

        for(const auto& it : d.observers_NtUmsThreadYield)
            it();
    }

    static void on_NtYieldExecution(monitor::syscalls::Data& d)
    {
        if constexpr(g_debug)
            LOG(INFO, "NtYieldExecution()");

        for(const auto& it : d.observers_NtYieldExecution)
            it();
    }

    static void on_RtlpAllocateHeapInternal(monitor::syscalls::Data& d)
    {
        const auto HeapHandle = arg<nt::PVOID>(d.core, 0);
        const auto Size       = arg<nt::SIZE_T>(d.core, 1);

        if constexpr(g_debug)
            LOG(INFO, "RtlpAllocateHeapInternal(HeapHandle:{:#x}, Size:{:#x})", HeapHandle, Size);

        for(const auto& it : d.observers_RtlpAllocateHeapInternal)
            it(HeapHandle, Size);
    }

    static void on_RtlFreeHeap(monitor::syscalls::Data& d)
    {
        const auto HeapHandle  = arg<nt::PVOID>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress = arg<nt::PVOID>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "RtlFreeHeap(HeapHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x})", HeapHandle, Flags, BaseAddress);

        for(const auto& it : d.observers_RtlFreeHeap)
            it(HeapHandle, Flags, BaseAddress);
    }

    static void on_RtlpReAllocateHeapInternal(monitor::syscalls::Data& d)
    {
        const auto HeapHandle  = arg<nt::PVOID>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress = arg<nt::PVOID>(d.core, 2);
        const auto Size        = arg<nt::ULONG>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "RtlpReAllocateHeapInternal(HeapHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x}, Size:{:#x})", HeapHandle, Flags, BaseAddress, Size);

        for(const auto& it : d.observers_RtlpReAllocateHeapInternal)
            it(HeapHandle, Flags, BaseAddress, Size);
    }

    static void on_RtlSizeHeap(monitor::syscalls::Data& d)
    {
        const auto HeapHandle  = arg<nt::PVOID>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress = arg<nt::PVOID>(d.core, 2);

        if constexpr(g_debug)
            LOG(INFO, "RtlSizeHeap(HeapHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x})", HeapHandle, Flags, BaseAddress);

        for(const auto& it : d.observers_RtlSizeHeap)
            it(HeapHandle, Flags, BaseAddress);
    }

    static void on_RtlSetUserValueHeap(monitor::syscalls::Data& d)
    {
        const auto HeapHandle  = arg<nt::PVOID>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress = arg<nt::PVOID>(d.core, 2);
        const auto UserValue   = arg<nt::PVOID>(d.core, 3);

        if constexpr(g_debug)
            LOG(INFO, "RtlSetUserValueHeap(HeapHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x}, UserValue:{:#x})", HeapHandle, Flags, BaseAddress, UserValue);

        for(const auto& it : d.observers_RtlSetUserValueHeap)
            it(HeapHandle, Flags, BaseAddress, UserValue);
    }

    static void on_RtlGetUserInfoHeap(monitor::syscalls::Data& d)
    {
        const auto HeapHandle  = arg<nt::PVOID>(d.core, 0);
        const auto Flags       = arg<nt::ULONG>(d.core, 1);
        const auto BaseAddress = arg<nt::PVOID>(d.core, 2);
        const auto UserValue   = arg<nt::PVOID>(d.core, 3);
        const auto UserFlags   = arg<nt::PULONG>(d.core, 4);

        if constexpr(g_debug)
            LOG(INFO, "RtlGetUserInfoHeap(HeapHandle:{:#x}, Flags:{:#x}, BaseAddress:{:#x}, UserValue:{:#x}, UserFlags:{:#x})", HeapHandle, Flags, BaseAddress, UserValue, UserFlags);

        for(const auto& it : d.observers_RtlGetUserInfoHeap)
            it(HeapHandle, Flags, BaseAddress, UserValue, UserFlags);
    }

}


bool monitor::syscalls::register_NtAcceptConnectPort(proc_t proc, const on_NtAcceptConnectPort_fn& on_func)
{
    if(d_->observers_NtAcceptConnectPort.empty())
        if(!register_callback_with(*d_, proc, "NtAcceptConnectPort", &on_NtAcceptConnectPort))
            return false;

    d_->observers_NtAcceptConnectPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAccessCheckAndAuditAlarm(proc_t proc, const on_NtAccessCheckAndAuditAlarm_fn& on_func)
{
    if(d_->observers_NtAccessCheckAndAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "NtAccessCheckAndAuditAlarm", &on_NtAccessCheckAndAuditAlarm))
            return false;

    d_->observers_NtAccessCheckAndAuditAlarm.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAccessCheckByTypeAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeAndAuditAlarm_fn& on_func)
{
    if(d_->observers_NtAccessCheckByTypeAndAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "NtAccessCheckByTypeAndAuditAlarm", &on_NtAccessCheckByTypeAndAuditAlarm))
            return false;

    d_->observers_NtAccessCheckByTypeAndAuditAlarm.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAccessCheckByType(proc_t proc, const on_NtAccessCheckByType_fn& on_func)
{
    if(d_->observers_NtAccessCheckByType.empty())
        if(!register_callback_with(*d_, proc, "NtAccessCheckByType", &on_NtAccessCheckByType))
            return false;

    d_->observers_NtAccessCheckByType.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAccessCheckByTypeResultListAndAuditAlarmByHandle(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle_fn& on_func)
{
    if(d_->observers_NtAccessCheckByTypeResultListAndAuditAlarmByHandle.empty())
        if(!register_callback_with(*d_, proc, "NtAccessCheckByTypeResultListAndAuditAlarmByHandle", &on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle))
            return false;

    d_->observers_NtAccessCheckByTypeResultListAndAuditAlarmByHandle.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAccessCheckByTypeResultListAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarm_fn& on_func)
{
    if(d_->observers_NtAccessCheckByTypeResultListAndAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "NtAccessCheckByTypeResultListAndAuditAlarm", &on_NtAccessCheckByTypeResultListAndAuditAlarm))
            return false;

    d_->observers_NtAccessCheckByTypeResultListAndAuditAlarm.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAccessCheckByTypeResultList(proc_t proc, const on_NtAccessCheckByTypeResultList_fn& on_func)
{
    if(d_->observers_NtAccessCheckByTypeResultList.empty())
        if(!register_callback_with(*d_, proc, "NtAccessCheckByTypeResultList", &on_NtAccessCheckByTypeResultList))
            return false;

    d_->observers_NtAccessCheckByTypeResultList.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAccessCheck(proc_t proc, const on_NtAccessCheck_fn& on_func)
{
    if(d_->observers_NtAccessCheck.empty())
        if(!register_callback_with(*d_, proc, "NtAccessCheck", &on_NtAccessCheck))
            return false;

    d_->observers_NtAccessCheck.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAddAtom(proc_t proc, const on_NtAddAtom_fn& on_func)
{
    if(d_->observers_NtAddAtom.empty())
        if(!register_callback_with(*d_, proc, "NtAddAtom", &on_NtAddAtom))
            return false;

    d_->observers_NtAddAtom.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAddBootEntry(proc_t proc, const on_NtAddBootEntry_fn& on_func)
{
    if(d_->observers_NtAddBootEntry.empty())
        if(!register_callback_with(*d_, proc, "NtAddBootEntry", &on_NtAddBootEntry))
            return false;

    d_->observers_NtAddBootEntry.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAddDriverEntry(proc_t proc, const on_NtAddDriverEntry_fn& on_func)
{
    if(d_->observers_NtAddDriverEntry.empty())
        if(!register_callback_with(*d_, proc, "NtAddDriverEntry", &on_NtAddDriverEntry))
            return false;

    d_->observers_NtAddDriverEntry.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAdjustGroupsToken(proc_t proc, const on_NtAdjustGroupsToken_fn& on_func)
{
    if(d_->observers_NtAdjustGroupsToken.empty())
        if(!register_callback_with(*d_, proc, "NtAdjustGroupsToken", &on_NtAdjustGroupsToken))
            return false;

    d_->observers_NtAdjustGroupsToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAdjustPrivilegesToken(proc_t proc, const on_NtAdjustPrivilegesToken_fn& on_func)
{
    if(d_->observers_NtAdjustPrivilegesToken.empty())
        if(!register_callback_with(*d_, proc, "NtAdjustPrivilegesToken", &on_NtAdjustPrivilegesToken))
            return false;

    d_->observers_NtAdjustPrivilegesToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlertResumeThread(proc_t proc, const on_NtAlertResumeThread_fn& on_func)
{
    if(d_->observers_NtAlertResumeThread.empty())
        if(!register_callback_with(*d_, proc, "NtAlertResumeThread", &on_NtAlertResumeThread))
            return false;

    d_->observers_NtAlertResumeThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlertThread(proc_t proc, const on_NtAlertThread_fn& on_func)
{
    if(d_->observers_NtAlertThread.empty())
        if(!register_callback_with(*d_, proc, "NtAlertThread", &on_NtAlertThread))
            return false;

    d_->observers_NtAlertThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAllocateLocallyUniqueId(proc_t proc, const on_NtAllocateLocallyUniqueId_fn& on_func)
{
    if(d_->observers_NtAllocateLocallyUniqueId.empty())
        if(!register_callback_with(*d_, proc, "NtAllocateLocallyUniqueId", &on_NtAllocateLocallyUniqueId))
            return false;

    d_->observers_NtAllocateLocallyUniqueId.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAllocateReserveObject(proc_t proc, const on_NtAllocateReserveObject_fn& on_func)
{
    if(d_->observers_NtAllocateReserveObject.empty())
        if(!register_callback_with(*d_, proc, "NtAllocateReserveObject", &on_NtAllocateReserveObject))
            return false;

    d_->observers_NtAllocateReserveObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAllocateUserPhysicalPages(proc_t proc, const on_NtAllocateUserPhysicalPages_fn& on_func)
{
    if(d_->observers_NtAllocateUserPhysicalPages.empty())
        if(!register_callback_with(*d_, proc, "NtAllocateUserPhysicalPages", &on_NtAllocateUserPhysicalPages))
            return false;

    d_->observers_NtAllocateUserPhysicalPages.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAllocateUuids(proc_t proc, const on_NtAllocateUuids_fn& on_func)
{
    if(d_->observers_NtAllocateUuids.empty())
        if(!register_callback_with(*d_, proc, "NtAllocateUuids", &on_NtAllocateUuids))
            return false;

    d_->observers_NtAllocateUuids.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAllocateVirtualMemory(proc_t proc, const on_NtAllocateVirtualMemory_fn& on_func)
{
    if(d_->observers_NtAllocateVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "NtAllocateVirtualMemory", &on_NtAllocateVirtualMemory))
            return false;

    d_->observers_NtAllocateVirtualMemory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcAcceptConnectPort(proc_t proc, const on_NtAlpcAcceptConnectPort_fn& on_func)
{
    if(d_->observers_NtAlpcAcceptConnectPort.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcAcceptConnectPort", &on_NtAlpcAcceptConnectPort))
            return false;

    d_->observers_NtAlpcAcceptConnectPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcCancelMessage(proc_t proc, const on_NtAlpcCancelMessage_fn& on_func)
{
    if(d_->observers_NtAlpcCancelMessage.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcCancelMessage", &on_NtAlpcCancelMessage))
            return false;

    d_->observers_NtAlpcCancelMessage.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcConnectPort(proc_t proc, const on_NtAlpcConnectPort_fn& on_func)
{
    if(d_->observers_NtAlpcConnectPort.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcConnectPort", &on_NtAlpcConnectPort))
            return false;

    d_->observers_NtAlpcConnectPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcCreatePort(proc_t proc, const on_NtAlpcCreatePort_fn& on_func)
{
    if(d_->observers_NtAlpcCreatePort.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcCreatePort", &on_NtAlpcCreatePort))
            return false;

    d_->observers_NtAlpcCreatePort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcCreatePortSection(proc_t proc, const on_NtAlpcCreatePortSection_fn& on_func)
{
    if(d_->observers_NtAlpcCreatePortSection.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcCreatePortSection", &on_NtAlpcCreatePortSection))
            return false;

    d_->observers_NtAlpcCreatePortSection.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcCreateResourceReserve(proc_t proc, const on_NtAlpcCreateResourceReserve_fn& on_func)
{
    if(d_->observers_NtAlpcCreateResourceReserve.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcCreateResourceReserve", &on_NtAlpcCreateResourceReserve))
            return false;

    d_->observers_NtAlpcCreateResourceReserve.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcCreateSectionView(proc_t proc, const on_NtAlpcCreateSectionView_fn& on_func)
{
    if(d_->observers_NtAlpcCreateSectionView.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcCreateSectionView", &on_NtAlpcCreateSectionView))
            return false;

    d_->observers_NtAlpcCreateSectionView.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcCreateSecurityContext(proc_t proc, const on_NtAlpcCreateSecurityContext_fn& on_func)
{
    if(d_->observers_NtAlpcCreateSecurityContext.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcCreateSecurityContext", &on_NtAlpcCreateSecurityContext))
            return false;

    d_->observers_NtAlpcCreateSecurityContext.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcDeletePortSection(proc_t proc, const on_NtAlpcDeletePortSection_fn& on_func)
{
    if(d_->observers_NtAlpcDeletePortSection.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcDeletePortSection", &on_NtAlpcDeletePortSection))
            return false;

    d_->observers_NtAlpcDeletePortSection.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcDeleteResourceReserve(proc_t proc, const on_NtAlpcDeleteResourceReserve_fn& on_func)
{
    if(d_->observers_NtAlpcDeleteResourceReserve.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcDeleteResourceReserve", &on_NtAlpcDeleteResourceReserve))
            return false;

    d_->observers_NtAlpcDeleteResourceReserve.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcDeleteSectionView(proc_t proc, const on_NtAlpcDeleteSectionView_fn& on_func)
{
    if(d_->observers_NtAlpcDeleteSectionView.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcDeleteSectionView", &on_NtAlpcDeleteSectionView))
            return false;

    d_->observers_NtAlpcDeleteSectionView.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcDeleteSecurityContext(proc_t proc, const on_NtAlpcDeleteSecurityContext_fn& on_func)
{
    if(d_->observers_NtAlpcDeleteSecurityContext.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcDeleteSecurityContext", &on_NtAlpcDeleteSecurityContext))
            return false;

    d_->observers_NtAlpcDeleteSecurityContext.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcDisconnectPort(proc_t proc, const on_NtAlpcDisconnectPort_fn& on_func)
{
    if(d_->observers_NtAlpcDisconnectPort.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcDisconnectPort", &on_NtAlpcDisconnectPort))
            return false;

    d_->observers_NtAlpcDisconnectPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcImpersonateClientOfPort(proc_t proc, const on_NtAlpcImpersonateClientOfPort_fn& on_func)
{
    if(d_->observers_NtAlpcImpersonateClientOfPort.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcImpersonateClientOfPort", &on_NtAlpcImpersonateClientOfPort))
            return false;

    d_->observers_NtAlpcImpersonateClientOfPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcOpenSenderProcess(proc_t proc, const on_NtAlpcOpenSenderProcess_fn& on_func)
{
    if(d_->observers_NtAlpcOpenSenderProcess.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcOpenSenderProcess", &on_NtAlpcOpenSenderProcess))
            return false;

    d_->observers_NtAlpcOpenSenderProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcOpenSenderThread(proc_t proc, const on_NtAlpcOpenSenderThread_fn& on_func)
{
    if(d_->observers_NtAlpcOpenSenderThread.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcOpenSenderThread", &on_NtAlpcOpenSenderThread))
            return false;

    d_->observers_NtAlpcOpenSenderThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcQueryInformation(proc_t proc, const on_NtAlpcQueryInformation_fn& on_func)
{
    if(d_->observers_NtAlpcQueryInformation.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcQueryInformation", &on_NtAlpcQueryInformation))
            return false;

    d_->observers_NtAlpcQueryInformation.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcQueryInformationMessage(proc_t proc, const on_NtAlpcQueryInformationMessage_fn& on_func)
{
    if(d_->observers_NtAlpcQueryInformationMessage.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcQueryInformationMessage", &on_NtAlpcQueryInformationMessage))
            return false;

    d_->observers_NtAlpcQueryInformationMessage.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcRevokeSecurityContext(proc_t proc, const on_NtAlpcRevokeSecurityContext_fn& on_func)
{
    if(d_->observers_NtAlpcRevokeSecurityContext.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcRevokeSecurityContext", &on_NtAlpcRevokeSecurityContext))
            return false;

    d_->observers_NtAlpcRevokeSecurityContext.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcSendWaitReceivePort(proc_t proc, const on_NtAlpcSendWaitReceivePort_fn& on_func)
{
    if(d_->observers_NtAlpcSendWaitReceivePort.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcSendWaitReceivePort", &on_NtAlpcSendWaitReceivePort))
            return false;

    d_->observers_NtAlpcSendWaitReceivePort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAlpcSetInformation(proc_t proc, const on_NtAlpcSetInformation_fn& on_func)
{
    if(d_->observers_NtAlpcSetInformation.empty())
        if(!register_callback_with(*d_, proc, "NtAlpcSetInformation", &on_NtAlpcSetInformation))
            return false;

    d_->observers_NtAlpcSetInformation.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtApphelpCacheControl(proc_t proc, const on_NtApphelpCacheControl_fn& on_func)
{
    if(d_->observers_NtApphelpCacheControl.empty())
        if(!register_callback_with(*d_, proc, "NtApphelpCacheControl", &on_NtApphelpCacheControl))
            return false;

    d_->observers_NtApphelpCacheControl.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAreMappedFilesTheSame(proc_t proc, const on_NtAreMappedFilesTheSame_fn& on_func)
{
    if(d_->observers_NtAreMappedFilesTheSame.empty())
        if(!register_callback_with(*d_, proc, "NtAreMappedFilesTheSame", &on_NtAreMappedFilesTheSame))
            return false;

    d_->observers_NtAreMappedFilesTheSame.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtAssignProcessToJobObject(proc_t proc, const on_NtAssignProcessToJobObject_fn& on_func)
{
    if(d_->observers_NtAssignProcessToJobObject.empty())
        if(!register_callback_with(*d_, proc, "NtAssignProcessToJobObject", &on_NtAssignProcessToJobObject))
            return false;

    d_->observers_NtAssignProcessToJobObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCallbackReturn(proc_t proc, const on_NtCallbackReturn_fn& on_func)
{
    if(d_->observers_NtCallbackReturn.empty())
        if(!register_callback_with(*d_, proc, "NtCallbackReturn", &on_NtCallbackReturn))
            return false;

    d_->observers_NtCallbackReturn.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCancelIoFileEx(proc_t proc, const on_NtCancelIoFileEx_fn& on_func)
{
    if(d_->observers_NtCancelIoFileEx.empty())
        if(!register_callback_with(*d_, proc, "NtCancelIoFileEx", &on_NtCancelIoFileEx))
            return false;

    d_->observers_NtCancelIoFileEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCancelIoFile(proc_t proc, const on_NtCancelIoFile_fn& on_func)
{
    if(d_->observers_NtCancelIoFile.empty())
        if(!register_callback_with(*d_, proc, "NtCancelIoFile", &on_NtCancelIoFile))
            return false;

    d_->observers_NtCancelIoFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCancelSynchronousIoFile(proc_t proc, const on_NtCancelSynchronousIoFile_fn& on_func)
{
    if(d_->observers_NtCancelSynchronousIoFile.empty())
        if(!register_callback_with(*d_, proc, "NtCancelSynchronousIoFile", &on_NtCancelSynchronousIoFile))
            return false;

    d_->observers_NtCancelSynchronousIoFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCancelTimer(proc_t proc, const on_NtCancelTimer_fn& on_func)
{
    if(d_->observers_NtCancelTimer.empty())
        if(!register_callback_with(*d_, proc, "NtCancelTimer", &on_NtCancelTimer))
            return false;

    d_->observers_NtCancelTimer.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtClearEvent(proc_t proc, const on_NtClearEvent_fn& on_func)
{
    if(d_->observers_NtClearEvent.empty())
        if(!register_callback_with(*d_, proc, "NtClearEvent", &on_NtClearEvent))
            return false;

    d_->observers_NtClearEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtClose(proc_t proc, const on_NtClose_fn& on_func)
{
    if(d_->observers_NtClose.empty())
        if(!register_callback_with(*d_, proc, "NtClose", &on_NtClose))
            return false;

    d_->observers_NtClose.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCloseObjectAuditAlarm(proc_t proc, const on_NtCloseObjectAuditAlarm_fn& on_func)
{
    if(d_->observers_NtCloseObjectAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "NtCloseObjectAuditAlarm", &on_NtCloseObjectAuditAlarm))
            return false;

    d_->observers_NtCloseObjectAuditAlarm.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCommitComplete(proc_t proc, const on_NtCommitComplete_fn& on_func)
{
    if(d_->observers_NtCommitComplete.empty())
        if(!register_callback_with(*d_, proc, "NtCommitComplete", &on_NtCommitComplete))
            return false;

    d_->observers_NtCommitComplete.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCommitEnlistment(proc_t proc, const on_NtCommitEnlistment_fn& on_func)
{
    if(d_->observers_NtCommitEnlistment.empty())
        if(!register_callback_with(*d_, proc, "NtCommitEnlistment", &on_NtCommitEnlistment))
            return false;

    d_->observers_NtCommitEnlistment.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCommitTransaction(proc_t proc, const on_NtCommitTransaction_fn& on_func)
{
    if(d_->observers_NtCommitTransaction.empty())
        if(!register_callback_with(*d_, proc, "NtCommitTransaction", &on_NtCommitTransaction))
            return false;

    d_->observers_NtCommitTransaction.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCompactKeys(proc_t proc, const on_NtCompactKeys_fn& on_func)
{
    if(d_->observers_NtCompactKeys.empty())
        if(!register_callback_with(*d_, proc, "NtCompactKeys", &on_NtCompactKeys))
            return false;

    d_->observers_NtCompactKeys.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCompareTokens(proc_t proc, const on_NtCompareTokens_fn& on_func)
{
    if(d_->observers_NtCompareTokens.empty())
        if(!register_callback_with(*d_, proc, "NtCompareTokens", &on_NtCompareTokens))
            return false;

    d_->observers_NtCompareTokens.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCompleteConnectPort(proc_t proc, const on_NtCompleteConnectPort_fn& on_func)
{
    if(d_->observers_NtCompleteConnectPort.empty())
        if(!register_callback_with(*d_, proc, "NtCompleteConnectPort", &on_NtCompleteConnectPort))
            return false;

    d_->observers_NtCompleteConnectPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCompressKey(proc_t proc, const on_NtCompressKey_fn& on_func)
{
    if(d_->observers_NtCompressKey.empty())
        if(!register_callback_with(*d_, proc, "NtCompressKey", &on_NtCompressKey))
            return false;

    d_->observers_NtCompressKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtConnectPort(proc_t proc, const on_NtConnectPort_fn& on_func)
{
    if(d_->observers_NtConnectPort.empty())
        if(!register_callback_with(*d_, proc, "NtConnectPort", &on_NtConnectPort))
            return false;

    d_->observers_NtConnectPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtContinue(proc_t proc, const on_NtContinue_fn& on_func)
{
    if(d_->observers_NtContinue.empty())
        if(!register_callback_with(*d_, proc, "NtContinue", &on_NtContinue))
            return false;

    d_->observers_NtContinue.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateDebugObject(proc_t proc, const on_NtCreateDebugObject_fn& on_func)
{
    if(d_->observers_NtCreateDebugObject.empty())
        if(!register_callback_with(*d_, proc, "NtCreateDebugObject", &on_NtCreateDebugObject))
            return false;

    d_->observers_NtCreateDebugObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateDirectoryObject(proc_t proc, const on_NtCreateDirectoryObject_fn& on_func)
{
    if(d_->observers_NtCreateDirectoryObject.empty())
        if(!register_callback_with(*d_, proc, "NtCreateDirectoryObject", &on_NtCreateDirectoryObject))
            return false;

    d_->observers_NtCreateDirectoryObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateEnlistment(proc_t proc, const on_NtCreateEnlistment_fn& on_func)
{
    if(d_->observers_NtCreateEnlistment.empty())
        if(!register_callback_with(*d_, proc, "NtCreateEnlistment", &on_NtCreateEnlistment))
            return false;

    d_->observers_NtCreateEnlistment.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateEvent(proc_t proc, const on_NtCreateEvent_fn& on_func)
{
    if(d_->observers_NtCreateEvent.empty())
        if(!register_callback_with(*d_, proc, "NtCreateEvent", &on_NtCreateEvent))
            return false;

    d_->observers_NtCreateEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateEventPair(proc_t proc, const on_NtCreateEventPair_fn& on_func)
{
    if(d_->observers_NtCreateEventPair.empty())
        if(!register_callback_with(*d_, proc, "NtCreateEventPair", &on_NtCreateEventPair))
            return false;

    d_->observers_NtCreateEventPair.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateFile(proc_t proc, const on_NtCreateFile_fn& on_func)
{
    if(d_->observers_NtCreateFile.empty())
        if(!register_callback_with(*d_, proc, "NtCreateFile", &on_NtCreateFile))
            return false;

    d_->observers_NtCreateFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateIoCompletion(proc_t proc, const on_NtCreateIoCompletion_fn& on_func)
{
    if(d_->observers_NtCreateIoCompletion.empty())
        if(!register_callback_with(*d_, proc, "NtCreateIoCompletion", &on_NtCreateIoCompletion))
            return false;

    d_->observers_NtCreateIoCompletion.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateJobObject(proc_t proc, const on_NtCreateJobObject_fn& on_func)
{
    if(d_->observers_NtCreateJobObject.empty())
        if(!register_callback_with(*d_, proc, "NtCreateJobObject", &on_NtCreateJobObject))
            return false;

    d_->observers_NtCreateJobObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateJobSet(proc_t proc, const on_NtCreateJobSet_fn& on_func)
{
    if(d_->observers_NtCreateJobSet.empty())
        if(!register_callback_with(*d_, proc, "NtCreateJobSet", &on_NtCreateJobSet))
            return false;

    d_->observers_NtCreateJobSet.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateKeyedEvent(proc_t proc, const on_NtCreateKeyedEvent_fn& on_func)
{
    if(d_->observers_NtCreateKeyedEvent.empty())
        if(!register_callback_with(*d_, proc, "NtCreateKeyedEvent", &on_NtCreateKeyedEvent))
            return false;

    d_->observers_NtCreateKeyedEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateKey(proc_t proc, const on_NtCreateKey_fn& on_func)
{
    if(d_->observers_NtCreateKey.empty())
        if(!register_callback_with(*d_, proc, "NtCreateKey", &on_NtCreateKey))
            return false;

    d_->observers_NtCreateKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateKeyTransacted(proc_t proc, const on_NtCreateKeyTransacted_fn& on_func)
{
    if(d_->observers_NtCreateKeyTransacted.empty())
        if(!register_callback_with(*d_, proc, "NtCreateKeyTransacted", &on_NtCreateKeyTransacted))
            return false;

    d_->observers_NtCreateKeyTransacted.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateMailslotFile(proc_t proc, const on_NtCreateMailslotFile_fn& on_func)
{
    if(d_->observers_NtCreateMailslotFile.empty())
        if(!register_callback_with(*d_, proc, "NtCreateMailslotFile", &on_NtCreateMailslotFile))
            return false;

    d_->observers_NtCreateMailslotFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateMutant(proc_t proc, const on_NtCreateMutant_fn& on_func)
{
    if(d_->observers_NtCreateMutant.empty())
        if(!register_callback_with(*d_, proc, "NtCreateMutant", &on_NtCreateMutant))
            return false;

    d_->observers_NtCreateMutant.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateNamedPipeFile(proc_t proc, const on_NtCreateNamedPipeFile_fn& on_func)
{
    if(d_->observers_NtCreateNamedPipeFile.empty())
        if(!register_callback_with(*d_, proc, "NtCreateNamedPipeFile", &on_NtCreateNamedPipeFile))
            return false;

    d_->observers_NtCreateNamedPipeFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreatePagingFile(proc_t proc, const on_NtCreatePagingFile_fn& on_func)
{
    if(d_->observers_NtCreatePagingFile.empty())
        if(!register_callback_with(*d_, proc, "NtCreatePagingFile", &on_NtCreatePagingFile))
            return false;

    d_->observers_NtCreatePagingFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreatePort(proc_t proc, const on_NtCreatePort_fn& on_func)
{
    if(d_->observers_NtCreatePort.empty())
        if(!register_callback_with(*d_, proc, "NtCreatePort", &on_NtCreatePort))
            return false;

    d_->observers_NtCreatePort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreatePrivateNamespace(proc_t proc, const on_NtCreatePrivateNamespace_fn& on_func)
{
    if(d_->observers_NtCreatePrivateNamespace.empty())
        if(!register_callback_with(*d_, proc, "NtCreatePrivateNamespace", &on_NtCreatePrivateNamespace))
            return false;

    d_->observers_NtCreatePrivateNamespace.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateProcessEx(proc_t proc, const on_NtCreateProcessEx_fn& on_func)
{
    if(d_->observers_NtCreateProcessEx.empty())
        if(!register_callback_with(*d_, proc, "NtCreateProcessEx", &on_NtCreateProcessEx))
            return false;

    d_->observers_NtCreateProcessEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateProcess(proc_t proc, const on_NtCreateProcess_fn& on_func)
{
    if(d_->observers_NtCreateProcess.empty())
        if(!register_callback_with(*d_, proc, "NtCreateProcess", &on_NtCreateProcess))
            return false;

    d_->observers_NtCreateProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateProfileEx(proc_t proc, const on_NtCreateProfileEx_fn& on_func)
{
    if(d_->observers_NtCreateProfileEx.empty())
        if(!register_callback_with(*d_, proc, "NtCreateProfileEx", &on_NtCreateProfileEx))
            return false;

    d_->observers_NtCreateProfileEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateProfile(proc_t proc, const on_NtCreateProfile_fn& on_func)
{
    if(d_->observers_NtCreateProfile.empty())
        if(!register_callback_with(*d_, proc, "NtCreateProfile", &on_NtCreateProfile))
            return false;

    d_->observers_NtCreateProfile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateResourceManager(proc_t proc, const on_NtCreateResourceManager_fn& on_func)
{
    if(d_->observers_NtCreateResourceManager.empty())
        if(!register_callback_with(*d_, proc, "NtCreateResourceManager", &on_NtCreateResourceManager))
            return false;

    d_->observers_NtCreateResourceManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateSection(proc_t proc, const on_NtCreateSection_fn& on_func)
{
    if(d_->observers_NtCreateSection.empty())
        if(!register_callback_with(*d_, proc, "NtCreateSection", &on_NtCreateSection))
            return false;

    d_->observers_NtCreateSection.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateSemaphore(proc_t proc, const on_NtCreateSemaphore_fn& on_func)
{
    if(d_->observers_NtCreateSemaphore.empty())
        if(!register_callback_with(*d_, proc, "NtCreateSemaphore", &on_NtCreateSemaphore))
            return false;

    d_->observers_NtCreateSemaphore.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateSymbolicLinkObject(proc_t proc, const on_NtCreateSymbolicLinkObject_fn& on_func)
{
    if(d_->observers_NtCreateSymbolicLinkObject.empty())
        if(!register_callback_with(*d_, proc, "NtCreateSymbolicLinkObject", &on_NtCreateSymbolicLinkObject))
            return false;

    d_->observers_NtCreateSymbolicLinkObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateThreadEx(proc_t proc, const on_NtCreateThreadEx_fn& on_func)
{
    if(d_->observers_NtCreateThreadEx.empty())
        if(!register_callback_with(*d_, proc, "NtCreateThreadEx", &on_NtCreateThreadEx))
            return false;

    d_->observers_NtCreateThreadEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateThread(proc_t proc, const on_NtCreateThread_fn& on_func)
{
    if(d_->observers_NtCreateThread.empty())
        if(!register_callback_with(*d_, proc, "NtCreateThread", &on_NtCreateThread))
            return false;

    d_->observers_NtCreateThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateTimer(proc_t proc, const on_NtCreateTimer_fn& on_func)
{
    if(d_->observers_NtCreateTimer.empty())
        if(!register_callback_with(*d_, proc, "NtCreateTimer", &on_NtCreateTimer))
            return false;

    d_->observers_NtCreateTimer.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateToken(proc_t proc, const on_NtCreateToken_fn& on_func)
{
    if(d_->observers_NtCreateToken.empty())
        if(!register_callback_with(*d_, proc, "NtCreateToken", &on_NtCreateToken))
            return false;

    d_->observers_NtCreateToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateTransactionManager(proc_t proc, const on_NtCreateTransactionManager_fn& on_func)
{
    if(d_->observers_NtCreateTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "NtCreateTransactionManager", &on_NtCreateTransactionManager))
            return false;

    d_->observers_NtCreateTransactionManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateTransaction(proc_t proc, const on_NtCreateTransaction_fn& on_func)
{
    if(d_->observers_NtCreateTransaction.empty())
        if(!register_callback_with(*d_, proc, "NtCreateTransaction", &on_NtCreateTransaction))
            return false;

    d_->observers_NtCreateTransaction.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateUserProcess(proc_t proc, const on_NtCreateUserProcess_fn& on_func)
{
    if(d_->observers_NtCreateUserProcess.empty())
        if(!register_callback_with(*d_, proc, "NtCreateUserProcess", &on_NtCreateUserProcess))
            return false;

    d_->observers_NtCreateUserProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateWaitablePort(proc_t proc, const on_NtCreateWaitablePort_fn& on_func)
{
    if(d_->observers_NtCreateWaitablePort.empty())
        if(!register_callback_with(*d_, proc, "NtCreateWaitablePort", &on_NtCreateWaitablePort))
            return false;

    d_->observers_NtCreateWaitablePort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtCreateWorkerFactory(proc_t proc, const on_NtCreateWorkerFactory_fn& on_func)
{
    if(d_->observers_NtCreateWorkerFactory.empty())
        if(!register_callback_with(*d_, proc, "NtCreateWorkerFactory", &on_NtCreateWorkerFactory))
            return false;

    d_->observers_NtCreateWorkerFactory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDebugActiveProcess(proc_t proc, const on_NtDebugActiveProcess_fn& on_func)
{
    if(d_->observers_NtDebugActiveProcess.empty())
        if(!register_callback_with(*d_, proc, "NtDebugActiveProcess", &on_NtDebugActiveProcess))
            return false;

    d_->observers_NtDebugActiveProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDebugContinue(proc_t proc, const on_NtDebugContinue_fn& on_func)
{
    if(d_->observers_NtDebugContinue.empty())
        if(!register_callback_with(*d_, proc, "NtDebugContinue", &on_NtDebugContinue))
            return false;

    d_->observers_NtDebugContinue.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDelayExecution(proc_t proc, const on_NtDelayExecution_fn& on_func)
{
    if(d_->observers_NtDelayExecution.empty())
        if(!register_callback_with(*d_, proc, "NtDelayExecution", &on_NtDelayExecution))
            return false;

    d_->observers_NtDelayExecution.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDeleteAtom(proc_t proc, const on_NtDeleteAtom_fn& on_func)
{
    if(d_->observers_NtDeleteAtom.empty())
        if(!register_callback_with(*d_, proc, "NtDeleteAtom", &on_NtDeleteAtom))
            return false;

    d_->observers_NtDeleteAtom.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDeleteBootEntry(proc_t proc, const on_NtDeleteBootEntry_fn& on_func)
{
    if(d_->observers_NtDeleteBootEntry.empty())
        if(!register_callback_with(*d_, proc, "NtDeleteBootEntry", &on_NtDeleteBootEntry))
            return false;

    d_->observers_NtDeleteBootEntry.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDeleteDriverEntry(proc_t proc, const on_NtDeleteDriverEntry_fn& on_func)
{
    if(d_->observers_NtDeleteDriverEntry.empty())
        if(!register_callback_with(*d_, proc, "NtDeleteDriverEntry", &on_NtDeleteDriverEntry))
            return false;

    d_->observers_NtDeleteDriverEntry.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDeleteFile(proc_t proc, const on_NtDeleteFile_fn& on_func)
{
    if(d_->observers_NtDeleteFile.empty())
        if(!register_callback_with(*d_, proc, "NtDeleteFile", &on_NtDeleteFile))
            return false;

    d_->observers_NtDeleteFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDeleteKey(proc_t proc, const on_NtDeleteKey_fn& on_func)
{
    if(d_->observers_NtDeleteKey.empty())
        if(!register_callback_with(*d_, proc, "NtDeleteKey", &on_NtDeleteKey))
            return false;

    d_->observers_NtDeleteKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDeleteObjectAuditAlarm(proc_t proc, const on_NtDeleteObjectAuditAlarm_fn& on_func)
{
    if(d_->observers_NtDeleteObjectAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "NtDeleteObjectAuditAlarm", &on_NtDeleteObjectAuditAlarm))
            return false;

    d_->observers_NtDeleteObjectAuditAlarm.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDeletePrivateNamespace(proc_t proc, const on_NtDeletePrivateNamespace_fn& on_func)
{
    if(d_->observers_NtDeletePrivateNamespace.empty())
        if(!register_callback_with(*d_, proc, "NtDeletePrivateNamespace", &on_NtDeletePrivateNamespace))
            return false;

    d_->observers_NtDeletePrivateNamespace.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDeleteValueKey(proc_t proc, const on_NtDeleteValueKey_fn& on_func)
{
    if(d_->observers_NtDeleteValueKey.empty())
        if(!register_callback_with(*d_, proc, "NtDeleteValueKey", &on_NtDeleteValueKey))
            return false;

    d_->observers_NtDeleteValueKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDeviceIoControlFile(proc_t proc, const on_NtDeviceIoControlFile_fn& on_func)
{
    if(d_->observers_NtDeviceIoControlFile.empty())
        if(!register_callback_with(*d_, proc, "NtDeviceIoControlFile", &on_NtDeviceIoControlFile))
            return false;

    d_->observers_NtDeviceIoControlFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDisplayString(proc_t proc, const on_NtDisplayString_fn& on_func)
{
    if(d_->observers_NtDisplayString.empty())
        if(!register_callback_with(*d_, proc, "NtDisplayString", &on_NtDisplayString))
            return false;

    d_->observers_NtDisplayString.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDrawText(proc_t proc, const on_NtDrawText_fn& on_func)
{
    if(d_->observers_NtDrawText.empty())
        if(!register_callback_with(*d_, proc, "NtDrawText", &on_NtDrawText))
            return false;

    d_->observers_NtDrawText.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDuplicateObject(proc_t proc, const on_NtDuplicateObject_fn& on_func)
{
    if(d_->observers_NtDuplicateObject.empty())
        if(!register_callback_with(*d_, proc, "NtDuplicateObject", &on_NtDuplicateObject))
            return false;

    d_->observers_NtDuplicateObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDuplicateToken(proc_t proc, const on_NtDuplicateToken_fn& on_func)
{
    if(d_->observers_NtDuplicateToken.empty())
        if(!register_callback_with(*d_, proc, "NtDuplicateToken", &on_NtDuplicateToken))
            return false;

    d_->observers_NtDuplicateToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtEnumerateBootEntries(proc_t proc, const on_NtEnumerateBootEntries_fn& on_func)
{
    if(d_->observers_NtEnumerateBootEntries.empty())
        if(!register_callback_with(*d_, proc, "NtEnumerateBootEntries", &on_NtEnumerateBootEntries))
            return false;

    d_->observers_NtEnumerateBootEntries.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtEnumerateDriverEntries(proc_t proc, const on_NtEnumerateDriverEntries_fn& on_func)
{
    if(d_->observers_NtEnumerateDriverEntries.empty())
        if(!register_callback_with(*d_, proc, "NtEnumerateDriverEntries", &on_NtEnumerateDriverEntries))
            return false;

    d_->observers_NtEnumerateDriverEntries.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtEnumerateKey(proc_t proc, const on_NtEnumerateKey_fn& on_func)
{
    if(d_->observers_NtEnumerateKey.empty())
        if(!register_callback_with(*d_, proc, "NtEnumerateKey", &on_NtEnumerateKey))
            return false;

    d_->observers_NtEnumerateKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtEnumerateSystemEnvironmentValuesEx(proc_t proc, const on_NtEnumerateSystemEnvironmentValuesEx_fn& on_func)
{
    if(d_->observers_NtEnumerateSystemEnvironmentValuesEx.empty())
        if(!register_callback_with(*d_, proc, "NtEnumerateSystemEnvironmentValuesEx", &on_NtEnumerateSystemEnvironmentValuesEx))
            return false;

    d_->observers_NtEnumerateSystemEnvironmentValuesEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtEnumerateTransactionObject(proc_t proc, const on_NtEnumerateTransactionObject_fn& on_func)
{
    if(d_->observers_NtEnumerateTransactionObject.empty())
        if(!register_callback_with(*d_, proc, "NtEnumerateTransactionObject", &on_NtEnumerateTransactionObject))
            return false;

    d_->observers_NtEnumerateTransactionObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtEnumerateValueKey(proc_t proc, const on_NtEnumerateValueKey_fn& on_func)
{
    if(d_->observers_NtEnumerateValueKey.empty())
        if(!register_callback_with(*d_, proc, "NtEnumerateValueKey", &on_NtEnumerateValueKey))
            return false;

    d_->observers_NtEnumerateValueKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtExtendSection(proc_t proc, const on_NtExtendSection_fn& on_func)
{
    if(d_->observers_NtExtendSection.empty())
        if(!register_callback_with(*d_, proc, "NtExtendSection", &on_NtExtendSection))
            return false;

    d_->observers_NtExtendSection.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFilterToken(proc_t proc, const on_NtFilterToken_fn& on_func)
{
    if(d_->observers_NtFilterToken.empty())
        if(!register_callback_with(*d_, proc, "NtFilterToken", &on_NtFilterToken))
            return false;

    d_->observers_NtFilterToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFindAtom(proc_t proc, const on_NtFindAtom_fn& on_func)
{
    if(d_->observers_NtFindAtom.empty())
        if(!register_callback_with(*d_, proc, "NtFindAtom", &on_NtFindAtom))
            return false;

    d_->observers_NtFindAtom.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFlushBuffersFile(proc_t proc, const on_NtFlushBuffersFile_fn& on_func)
{
    if(d_->observers_NtFlushBuffersFile.empty())
        if(!register_callback_with(*d_, proc, "NtFlushBuffersFile", &on_NtFlushBuffersFile))
            return false;

    d_->observers_NtFlushBuffersFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFlushInstallUILanguage(proc_t proc, const on_NtFlushInstallUILanguage_fn& on_func)
{
    if(d_->observers_NtFlushInstallUILanguage.empty())
        if(!register_callback_with(*d_, proc, "NtFlushInstallUILanguage", &on_NtFlushInstallUILanguage))
            return false;

    d_->observers_NtFlushInstallUILanguage.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFlushInstructionCache(proc_t proc, const on_NtFlushInstructionCache_fn& on_func)
{
    if(d_->observers_NtFlushInstructionCache.empty())
        if(!register_callback_with(*d_, proc, "NtFlushInstructionCache", &on_NtFlushInstructionCache))
            return false;

    d_->observers_NtFlushInstructionCache.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFlushKey(proc_t proc, const on_NtFlushKey_fn& on_func)
{
    if(d_->observers_NtFlushKey.empty())
        if(!register_callback_with(*d_, proc, "NtFlushKey", &on_NtFlushKey))
            return false;

    d_->observers_NtFlushKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFlushVirtualMemory(proc_t proc, const on_NtFlushVirtualMemory_fn& on_func)
{
    if(d_->observers_NtFlushVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "NtFlushVirtualMemory", &on_NtFlushVirtualMemory))
            return false;

    d_->observers_NtFlushVirtualMemory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFreeUserPhysicalPages(proc_t proc, const on_NtFreeUserPhysicalPages_fn& on_func)
{
    if(d_->observers_NtFreeUserPhysicalPages.empty())
        if(!register_callback_with(*d_, proc, "NtFreeUserPhysicalPages", &on_NtFreeUserPhysicalPages))
            return false;

    d_->observers_NtFreeUserPhysicalPages.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFreeVirtualMemory(proc_t proc, const on_NtFreeVirtualMemory_fn& on_func)
{
    if(d_->observers_NtFreeVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "NtFreeVirtualMemory", &on_NtFreeVirtualMemory))
            return false;

    d_->observers_NtFreeVirtualMemory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFreezeRegistry(proc_t proc, const on_NtFreezeRegistry_fn& on_func)
{
    if(d_->observers_NtFreezeRegistry.empty())
        if(!register_callback_with(*d_, proc, "NtFreezeRegistry", &on_NtFreezeRegistry))
            return false;

    d_->observers_NtFreezeRegistry.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFreezeTransactions(proc_t proc, const on_NtFreezeTransactions_fn& on_func)
{
    if(d_->observers_NtFreezeTransactions.empty())
        if(!register_callback_with(*d_, proc, "NtFreezeTransactions", &on_NtFreezeTransactions))
            return false;

    d_->observers_NtFreezeTransactions.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFsControlFile(proc_t proc, const on_NtFsControlFile_fn& on_func)
{
    if(d_->observers_NtFsControlFile.empty())
        if(!register_callback_with(*d_, proc, "NtFsControlFile", &on_NtFsControlFile))
            return false;

    d_->observers_NtFsControlFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtGetContextThread(proc_t proc, const on_NtGetContextThread_fn& on_func)
{
    if(d_->observers_NtGetContextThread.empty())
        if(!register_callback_with(*d_, proc, "NtGetContextThread", &on_NtGetContextThread))
            return false;

    d_->observers_NtGetContextThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtGetDevicePowerState(proc_t proc, const on_NtGetDevicePowerState_fn& on_func)
{
    if(d_->observers_NtGetDevicePowerState.empty())
        if(!register_callback_with(*d_, proc, "NtGetDevicePowerState", &on_NtGetDevicePowerState))
            return false;

    d_->observers_NtGetDevicePowerState.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtGetMUIRegistryInfo(proc_t proc, const on_NtGetMUIRegistryInfo_fn& on_func)
{
    if(d_->observers_NtGetMUIRegistryInfo.empty())
        if(!register_callback_with(*d_, proc, "NtGetMUIRegistryInfo", &on_NtGetMUIRegistryInfo))
            return false;

    d_->observers_NtGetMUIRegistryInfo.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtGetNextProcess(proc_t proc, const on_NtGetNextProcess_fn& on_func)
{
    if(d_->observers_NtGetNextProcess.empty())
        if(!register_callback_with(*d_, proc, "NtGetNextProcess", &on_NtGetNextProcess))
            return false;

    d_->observers_NtGetNextProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtGetNextThread(proc_t proc, const on_NtGetNextThread_fn& on_func)
{
    if(d_->observers_NtGetNextThread.empty())
        if(!register_callback_with(*d_, proc, "NtGetNextThread", &on_NtGetNextThread))
            return false;

    d_->observers_NtGetNextThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtGetNlsSectionPtr(proc_t proc, const on_NtGetNlsSectionPtr_fn& on_func)
{
    if(d_->observers_NtGetNlsSectionPtr.empty())
        if(!register_callback_with(*d_, proc, "NtGetNlsSectionPtr", &on_NtGetNlsSectionPtr))
            return false;

    d_->observers_NtGetNlsSectionPtr.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtGetNotificationResourceManager(proc_t proc, const on_NtGetNotificationResourceManager_fn& on_func)
{
    if(d_->observers_NtGetNotificationResourceManager.empty())
        if(!register_callback_with(*d_, proc, "NtGetNotificationResourceManager", &on_NtGetNotificationResourceManager))
            return false;

    d_->observers_NtGetNotificationResourceManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtGetPlugPlayEvent(proc_t proc, const on_NtGetPlugPlayEvent_fn& on_func)
{
    if(d_->observers_NtGetPlugPlayEvent.empty())
        if(!register_callback_with(*d_, proc, "NtGetPlugPlayEvent", &on_NtGetPlugPlayEvent))
            return false;

    d_->observers_NtGetPlugPlayEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtGetWriteWatch(proc_t proc, const on_NtGetWriteWatch_fn& on_func)
{
    if(d_->observers_NtGetWriteWatch.empty())
        if(!register_callback_with(*d_, proc, "NtGetWriteWatch", &on_NtGetWriteWatch))
            return false;

    d_->observers_NtGetWriteWatch.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtImpersonateAnonymousToken(proc_t proc, const on_NtImpersonateAnonymousToken_fn& on_func)
{
    if(d_->observers_NtImpersonateAnonymousToken.empty())
        if(!register_callback_with(*d_, proc, "NtImpersonateAnonymousToken", &on_NtImpersonateAnonymousToken))
            return false;

    d_->observers_NtImpersonateAnonymousToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtImpersonateClientOfPort(proc_t proc, const on_NtImpersonateClientOfPort_fn& on_func)
{
    if(d_->observers_NtImpersonateClientOfPort.empty())
        if(!register_callback_with(*d_, proc, "NtImpersonateClientOfPort", &on_NtImpersonateClientOfPort))
            return false;

    d_->observers_NtImpersonateClientOfPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtImpersonateThread(proc_t proc, const on_NtImpersonateThread_fn& on_func)
{
    if(d_->observers_NtImpersonateThread.empty())
        if(!register_callback_with(*d_, proc, "NtImpersonateThread", &on_NtImpersonateThread))
            return false;

    d_->observers_NtImpersonateThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtInitializeNlsFiles(proc_t proc, const on_NtInitializeNlsFiles_fn& on_func)
{
    if(d_->observers_NtInitializeNlsFiles.empty())
        if(!register_callback_with(*d_, proc, "NtInitializeNlsFiles", &on_NtInitializeNlsFiles))
            return false;

    d_->observers_NtInitializeNlsFiles.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtInitializeRegistry(proc_t proc, const on_NtInitializeRegistry_fn& on_func)
{
    if(d_->observers_NtInitializeRegistry.empty())
        if(!register_callback_with(*d_, proc, "NtInitializeRegistry", &on_NtInitializeRegistry))
            return false;

    d_->observers_NtInitializeRegistry.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtInitiatePowerAction(proc_t proc, const on_NtInitiatePowerAction_fn& on_func)
{
    if(d_->observers_NtInitiatePowerAction.empty())
        if(!register_callback_with(*d_, proc, "NtInitiatePowerAction", &on_NtInitiatePowerAction))
            return false;

    d_->observers_NtInitiatePowerAction.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtIsProcessInJob(proc_t proc, const on_NtIsProcessInJob_fn& on_func)
{
    if(d_->observers_NtIsProcessInJob.empty())
        if(!register_callback_with(*d_, proc, "NtIsProcessInJob", &on_NtIsProcessInJob))
            return false;

    d_->observers_NtIsProcessInJob.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtListenPort(proc_t proc, const on_NtListenPort_fn& on_func)
{
    if(d_->observers_NtListenPort.empty())
        if(!register_callback_with(*d_, proc, "NtListenPort", &on_NtListenPort))
            return false;

    d_->observers_NtListenPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtLoadDriver(proc_t proc, const on_NtLoadDriver_fn& on_func)
{
    if(d_->observers_NtLoadDriver.empty())
        if(!register_callback_with(*d_, proc, "NtLoadDriver", &on_NtLoadDriver))
            return false;

    d_->observers_NtLoadDriver.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtLoadKey2(proc_t proc, const on_NtLoadKey2_fn& on_func)
{
    if(d_->observers_NtLoadKey2.empty())
        if(!register_callback_with(*d_, proc, "NtLoadKey2", &on_NtLoadKey2))
            return false;

    d_->observers_NtLoadKey2.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtLoadKeyEx(proc_t proc, const on_NtLoadKeyEx_fn& on_func)
{
    if(d_->observers_NtLoadKeyEx.empty())
        if(!register_callback_with(*d_, proc, "NtLoadKeyEx", &on_NtLoadKeyEx))
            return false;

    d_->observers_NtLoadKeyEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtLoadKey(proc_t proc, const on_NtLoadKey_fn& on_func)
{
    if(d_->observers_NtLoadKey.empty())
        if(!register_callback_with(*d_, proc, "NtLoadKey", &on_NtLoadKey))
            return false;

    d_->observers_NtLoadKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtLockFile(proc_t proc, const on_NtLockFile_fn& on_func)
{
    if(d_->observers_NtLockFile.empty())
        if(!register_callback_with(*d_, proc, "NtLockFile", &on_NtLockFile))
            return false;

    d_->observers_NtLockFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtLockProductActivationKeys(proc_t proc, const on_NtLockProductActivationKeys_fn& on_func)
{
    if(d_->observers_NtLockProductActivationKeys.empty())
        if(!register_callback_with(*d_, proc, "NtLockProductActivationKeys", &on_NtLockProductActivationKeys))
            return false;

    d_->observers_NtLockProductActivationKeys.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtLockRegistryKey(proc_t proc, const on_NtLockRegistryKey_fn& on_func)
{
    if(d_->observers_NtLockRegistryKey.empty())
        if(!register_callback_with(*d_, proc, "NtLockRegistryKey", &on_NtLockRegistryKey))
            return false;

    d_->observers_NtLockRegistryKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtLockVirtualMemory(proc_t proc, const on_NtLockVirtualMemory_fn& on_func)
{
    if(d_->observers_NtLockVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "NtLockVirtualMemory", &on_NtLockVirtualMemory))
            return false;

    d_->observers_NtLockVirtualMemory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtMakePermanentObject(proc_t proc, const on_NtMakePermanentObject_fn& on_func)
{
    if(d_->observers_NtMakePermanentObject.empty())
        if(!register_callback_with(*d_, proc, "NtMakePermanentObject", &on_NtMakePermanentObject))
            return false;

    d_->observers_NtMakePermanentObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtMakeTemporaryObject(proc_t proc, const on_NtMakeTemporaryObject_fn& on_func)
{
    if(d_->observers_NtMakeTemporaryObject.empty())
        if(!register_callback_with(*d_, proc, "NtMakeTemporaryObject", &on_NtMakeTemporaryObject))
            return false;

    d_->observers_NtMakeTemporaryObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtMapCMFModule(proc_t proc, const on_NtMapCMFModule_fn& on_func)
{
    if(d_->observers_NtMapCMFModule.empty())
        if(!register_callback_with(*d_, proc, "NtMapCMFModule", &on_NtMapCMFModule))
            return false;

    d_->observers_NtMapCMFModule.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtMapUserPhysicalPages(proc_t proc, const on_NtMapUserPhysicalPages_fn& on_func)
{
    if(d_->observers_NtMapUserPhysicalPages.empty())
        if(!register_callback_with(*d_, proc, "NtMapUserPhysicalPages", &on_NtMapUserPhysicalPages))
            return false;

    d_->observers_NtMapUserPhysicalPages.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtMapUserPhysicalPagesScatter(proc_t proc, const on_NtMapUserPhysicalPagesScatter_fn& on_func)
{
    if(d_->observers_NtMapUserPhysicalPagesScatter.empty())
        if(!register_callback_with(*d_, proc, "NtMapUserPhysicalPagesScatter", &on_NtMapUserPhysicalPagesScatter))
            return false;

    d_->observers_NtMapUserPhysicalPagesScatter.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtMapViewOfSection(proc_t proc, const on_NtMapViewOfSection_fn& on_func)
{
    if(d_->observers_NtMapViewOfSection.empty())
        if(!register_callback_with(*d_, proc, "NtMapViewOfSection", &on_NtMapViewOfSection))
            return false;

    d_->observers_NtMapViewOfSection.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtModifyBootEntry(proc_t proc, const on_NtModifyBootEntry_fn& on_func)
{
    if(d_->observers_NtModifyBootEntry.empty())
        if(!register_callback_with(*d_, proc, "NtModifyBootEntry", &on_NtModifyBootEntry))
            return false;

    d_->observers_NtModifyBootEntry.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtModifyDriverEntry(proc_t proc, const on_NtModifyDriverEntry_fn& on_func)
{
    if(d_->observers_NtModifyDriverEntry.empty())
        if(!register_callback_with(*d_, proc, "NtModifyDriverEntry", &on_NtModifyDriverEntry))
            return false;

    d_->observers_NtModifyDriverEntry.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtNotifyChangeDirectoryFile(proc_t proc, const on_NtNotifyChangeDirectoryFile_fn& on_func)
{
    if(d_->observers_NtNotifyChangeDirectoryFile.empty())
        if(!register_callback_with(*d_, proc, "NtNotifyChangeDirectoryFile", &on_NtNotifyChangeDirectoryFile))
            return false;

    d_->observers_NtNotifyChangeDirectoryFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtNotifyChangeKey(proc_t proc, const on_NtNotifyChangeKey_fn& on_func)
{
    if(d_->observers_NtNotifyChangeKey.empty())
        if(!register_callback_with(*d_, proc, "NtNotifyChangeKey", &on_NtNotifyChangeKey))
            return false;

    d_->observers_NtNotifyChangeKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtNotifyChangeMultipleKeys(proc_t proc, const on_NtNotifyChangeMultipleKeys_fn& on_func)
{
    if(d_->observers_NtNotifyChangeMultipleKeys.empty())
        if(!register_callback_with(*d_, proc, "NtNotifyChangeMultipleKeys", &on_NtNotifyChangeMultipleKeys))
            return false;

    d_->observers_NtNotifyChangeMultipleKeys.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtNotifyChangeSession(proc_t proc, const on_NtNotifyChangeSession_fn& on_func)
{
    if(d_->observers_NtNotifyChangeSession.empty())
        if(!register_callback_with(*d_, proc, "NtNotifyChangeSession", &on_NtNotifyChangeSession))
            return false;

    d_->observers_NtNotifyChangeSession.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenDirectoryObject(proc_t proc, const on_NtOpenDirectoryObject_fn& on_func)
{
    if(d_->observers_NtOpenDirectoryObject.empty())
        if(!register_callback_with(*d_, proc, "NtOpenDirectoryObject", &on_NtOpenDirectoryObject))
            return false;

    d_->observers_NtOpenDirectoryObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenEnlistment(proc_t proc, const on_NtOpenEnlistment_fn& on_func)
{
    if(d_->observers_NtOpenEnlistment.empty())
        if(!register_callback_with(*d_, proc, "NtOpenEnlistment", &on_NtOpenEnlistment))
            return false;

    d_->observers_NtOpenEnlistment.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenEvent(proc_t proc, const on_NtOpenEvent_fn& on_func)
{
    if(d_->observers_NtOpenEvent.empty())
        if(!register_callback_with(*d_, proc, "NtOpenEvent", &on_NtOpenEvent))
            return false;

    d_->observers_NtOpenEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenEventPair(proc_t proc, const on_NtOpenEventPair_fn& on_func)
{
    if(d_->observers_NtOpenEventPair.empty())
        if(!register_callback_with(*d_, proc, "NtOpenEventPair", &on_NtOpenEventPair))
            return false;

    d_->observers_NtOpenEventPair.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenFile(proc_t proc, const on_NtOpenFile_fn& on_func)
{
    if(d_->observers_NtOpenFile.empty())
        if(!register_callback_with(*d_, proc, "NtOpenFile", &on_NtOpenFile))
            return false;

    d_->observers_NtOpenFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenIoCompletion(proc_t proc, const on_NtOpenIoCompletion_fn& on_func)
{
    if(d_->observers_NtOpenIoCompletion.empty())
        if(!register_callback_with(*d_, proc, "NtOpenIoCompletion", &on_NtOpenIoCompletion))
            return false;

    d_->observers_NtOpenIoCompletion.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenJobObject(proc_t proc, const on_NtOpenJobObject_fn& on_func)
{
    if(d_->observers_NtOpenJobObject.empty())
        if(!register_callback_with(*d_, proc, "NtOpenJobObject", &on_NtOpenJobObject))
            return false;

    d_->observers_NtOpenJobObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenKeyedEvent(proc_t proc, const on_NtOpenKeyedEvent_fn& on_func)
{
    if(d_->observers_NtOpenKeyedEvent.empty())
        if(!register_callback_with(*d_, proc, "NtOpenKeyedEvent", &on_NtOpenKeyedEvent))
            return false;

    d_->observers_NtOpenKeyedEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenKeyEx(proc_t proc, const on_NtOpenKeyEx_fn& on_func)
{
    if(d_->observers_NtOpenKeyEx.empty())
        if(!register_callback_with(*d_, proc, "NtOpenKeyEx", &on_NtOpenKeyEx))
            return false;

    d_->observers_NtOpenKeyEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenKey(proc_t proc, const on_NtOpenKey_fn& on_func)
{
    if(d_->observers_NtOpenKey.empty())
        if(!register_callback_with(*d_, proc, "NtOpenKey", &on_NtOpenKey))
            return false;

    d_->observers_NtOpenKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenKeyTransactedEx(proc_t proc, const on_NtOpenKeyTransactedEx_fn& on_func)
{
    if(d_->observers_NtOpenKeyTransactedEx.empty())
        if(!register_callback_with(*d_, proc, "NtOpenKeyTransactedEx", &on_NtOpenKeyTransactedEx))
            return false;

    d_->observers_NtOpenKeyTransactedEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenKeyTransacted(proc_t proc, const on_NtOpenKeyTransacted_fn& on_func)
{
    if(d_->observers_NtOpenKeyTransacted.empty())
        if(!register_callback_with(*d_, proc, "NtOpenKeyTransacted", &on_NtOpenKeyTransacted))
            return false;

    d_->observers_NtOpenKeyTransacted.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenMutant(proc_t proc, const on_NtOpenMutant_fn& on_func)
{
    if(d_->observers_NtOpenMutant.empty())
        if(!register_callback_with(*d_, proc, "NtOpenMutant", &on_NtOpenMutant))
            return false;

    d_->observers_NtOpenMutant.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenObjectAuditAlarm(proc_t proc, const on_NtOpenObjectAuditAlarm_fn& on_func)
{
    if(d_->observers_NtOpenObjectAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "NtOpenObjectAuditAlarm", &on_NtOpenObjectAuditAlarm))
            return false;

    d_->observers_NtOpenObjectAuditAlarm.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenPrivateNamespace(proc_t proc, const on_NtOpenPrivateNamespace_fn& on_func)
{
    if(d_->observers_NtOpenPrivateNamespace.empty())
        if(!register_callback_with(*d_, proc, "NtOpenPrivateNamespace", &on_NtOpenPrivateNamespace))
            return false;

    d_->observers_NtOpenPrivateNamespace.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenProcess(proc_t proc, const on_NtOpenProcess_fn& on_func)
{
    if(d_->observers_NtOpenProcess.empty())
        if(!register_callback_with(*d_, proc, "NtOpenProcess", &on_NtOpenProcess))
            return false;

    d_->observers_NtOpenProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenProcessTokenEx(proc_t proc, const on_NtOpenProcessTokenEx_fn& on_func)
{
    if(d_->observers_NtOpenProcessTokenEx.empty())
        if(!register_callback_with(*d_, proc, "NtOpenProcessTokenEx", &on_NtOpenProcessTokenEx))
            return false;

    d_->observers_NtOpenProcessTokenEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenProcessToken(proc_t proc, const on_NtOpenProcessToken_fn& on_func)
{
    if(d_->observers_NtOpenProcessToken.empty())
        if(!register_callback_with(*d_, proc, "NtOpenProcessToken", &on_NtOpenProcessToken))
            return false;

    d_->observers_NtOpenProcessToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenResourceManager(proc_t proc, const on_NtOpenResourceManager_fn& on_func)
{
    if(d_->observers_NtOpenResourceManager.empty())
        if(!register_callback_with(*d_, proc, "NtOpenResourceManager", &on_NtOpenResourceManager))
            return false;

    d_->observers_NtOpenResourceManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenSection(proc_t proc, const on_NtOpenSection_fn& on_func)
{
    if(d_->observers_NtOpenSection.empty())
        if(!register_callback_with(*d_, proc, "NtOpenSection", &on_NtOpenSection))
            return false;

    d_->observers_NtOpenSection.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenSemaphore(proc_t proc, const on_NtOpenSemaphore_fn& on_func)
{
    if(d_->observers_NtOpenSemaphore.empty())
        if(!register_callback_with(*d_, proc, "NtOpenSemaphore", &on_NtOpenSemaphore))
            return false;

    d_->observers_NtOpenSemaphore.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenSession(proc_t proc, const on_NtOpenSession_fn& on_func)
{
    if(d_->observers_NtOpenSession.empty())
        if(!register_callback_with(*d_, proc, "NtOpenSession", &on_NtOpenSession))
            return false;

    d_->observers_NtOpenSession.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenSymbolicLinkObject(proc_t proc, const on_NtOpenSymbolicLinkObject_fn& on_func)
{
    if(d_->observers_NtOpenSymbolicLinkObject.empty())
        if(!register_callback_with(*d_, proc, "NtOpenSymbolicLinkObject", &on_NtOpenSymbolicLinkObject))
            return false;

    d_->observers_NtOpenSymbolicLinkObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenThread(proc_t proc, const on_NtOpenThread_fn& on_func)
{
    if(d_->observers_NtOpenThread.empty())
        if(!register_callback_with(*d_, proc, "NtOpenThread", &on_NtOpenThread))
            return false;

    d_->observers_NtOpenThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenThreadTokenEx(proc_t proc, const on_NtOpenThreadTokenEx_fn& on_func)
{
    if(d_->observers_NtOpenThreadTokenEx.empty())
        if(!register_callback_with(*d_, proc, "NtOpenThreadTokenEx", &on_NtOpenThreadTokenEx))
            return false;

    d_->observers_NtOpenThreadTokenEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenThreadToken(proc_t proc, const on_NtOpenThreadToken_fn& on_func)
{
    if(d_->observers_NtOpenThreadToken.empty())
        if(!register_callback_with(*d_, proc, "NtOpenThreadToken", &on_NtOpenThreadToken))
            return false;

    d_->observers_NtOpenThreadToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenTimer(proc_t proc, const on_NtOpenTimer_fn& on_func)
{
    if(d_->observers_NtOpenTimer.empty())
        if(!register_callback_with(*d_, proc, "NtOpenTimer", &on_NtOpenTimer))
            return false;

    d_->observers_NtOpenTimer.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenTransactionManager(proc_t proc, const on_NtOpenTransactionManager_fn& on_func)
{
    if(d_->observers_NtOpenTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "NtOpenTransactionManager", &on_NtOpenTransactionManager))
            return false;

    d_->observers_NtOpenTransactionManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtOpenTransaction(proc_t proc, const on_NtOpenTransaction_fn& on_func)
{
    if(d_->observers_NtOpenTransaction.empty())
        if(!register_callback_with(*d_, proc, "NtOpenTransaction", &on_NtOpenTransaction))
            return false;

    d_->observers_NtOpenTransaction.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPlugPlayControl(proc_t proc, const on_NtPlugPlayControl_fn& on_func)
{
    if(d_->observers_NtPlugPlayControl.empty())
        if(!register_callback_with(*d_, proc, "NtPlugPlayControl", &on_NtPlugPlayControl))
            return false;

    d_->observers_NtPlugPlayControl.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPowerInformation(proc_t proc, const on_NtPowerInformation_fn& on_func)
{
    if(d_->observers_NtPowerInformation.empty())
        if(!register_callback_with(*d_, proc, "NtPowerInformation", &on_NtPowerInformation))
            return false;

    d_->observers_NtPowerInformation.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPrepareComplete(proc_t proc, const on_NtPrepareComplete_fn& on_func)
{
    if(d_->observers_NtPrepareComplete.empty())
        if(!register_callback_with(*d_, proc, "NtPrepareComplete", &on_NtPrepareComplete))
            return false;

    d_->observers_NtPrepareComplete.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPrepareEnlistment(proc_t proc, const on_NtPrepareEnlistment_fn& on_func)
{
    if(d_->observers_NtPrepareEnlistment.empty())
        if(!register_callback_with(*d_, proc, "NtPrepareEnlistment", &on_NtPrepareEnlistment))
            return false;

    d_->observers_NtPrepareEnlistment.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPrePrepareComplete(proc_t proc, const on_NtPrePrepareComplete_fn& on_func)
{
    if(d_->observers_NtPrePrepareComplete.empty())
        if(!register_callback_with(*d_, proc, "NtPrePrepareComplete", &on_NtPrePrepareComplete))
            return false;

    d_->observers_NtPrePrepareComplete.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPrePrepareEnlistment(proc_t proc, const on_NtPrePrepareEnlistment_fn& on_func)
{
    if(d_->observers_NtPrePrepareEnlistment.empty())
        if(!register_callback_with(*d_, proc, "NtPrePrepareEnlistment", &on_NtPrePrepareEnlistment))
            return false;

    d_->observers_NtPrePrepareEnlistment.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPrivilegeCheck(proc_t proc, const on_NtPrivilegeCheck_fn& on_func)
{
    if(d_->observers_NtPrivilegeCheck.empty())
        if(!register_callback_with(*d_, proc, "NtPrivilegeCheck", &on_NtPrivilegeCheck))
            return false;

    d_->observers_NtPrivilegeCheck.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPrivilegedServiceAuditAlarm(proc_t proc, const on_NtPrivilegedServiceAuditAlarm_fn& on_func)
{
    if(d_->observers_NtPrivilegedServiceAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "NtPrivilegedServiceAuditAlarm", &on_NtPrivilegedServiceAuditAlarm))
            return false;

    d_->observers_NtPrivilegedServiceAuditAlarm.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPrivilegeObjectAuditAlarm(proc_t proc, const on_NtPrivilegeObjectAuditAlarm_fn& on_func)
{
    if(d_->observers_NtPrivilegeObjectAuditAlarm.empty())
        if(!register_callback_with(*d_, proc, "NtPrivilegeObjectAuditAlarm", &on_NtPrivilegeObjectAuditAlarm))
            return false;

    d_->observers_NtPrivilegeObjectAuditAlarm.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPropagationComplete(proc_t proc, const on_NtPropagationComplete_fn& on_func)
{
    if(d_->observers_NtPropagationComplete.empty())
        if(!register_callback_with(*d_, proc, "NtPropagationComplete", &on_NtPropagationComplete))
            return false;

    d_->observers_NtPropagationComplete.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPropagationFailed(proc_t proc, const on_NtPropagationFailed_fn& on_func)
{
    if(d_->observers_NtPropagationFailed.empty())
        if(!register_callback_with(*d_, proc, "NtPropagationFailed", &on_NtPropagationFailed))
            return false;

    d_->observers_NtPropagationFailed.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtProtectVirtualMemory(proc_t proc, const on_NtProtectVirtualMemory_fn& on_func)
{
    if(d_->observers_NtProtectVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "NtProtectVirtualMemory", &on_NtProtectVirtualMemory))
            return false;

    d_->observers_NtProtectVirtualMemory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtPulseEvent(proc_t proc, const on_NtPulseEvent_fn& on_func)
{
    if(d_->observers_NtPulseEvent.empty())
        if(!register_callback_with(*d_, proc, "NtPulseEvent", &on_NtPulseEvent))
            return false;

    d_->observers_NtPulseEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryAttributesFile(proc_t proc, const on_NtQueryAttributesFile_fn& on_func)
{
    if(d_->observers_NtQueryAttributesFile.empty())
        if(!register_callback_with(*d_, proc, "NtQueryAttributesFile", &on_NtQueryAttributesFile))
            return false;

    d_->observers_NtQueryAttributesFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryBootEntryOrder(proc_t proc, const on_NtQueryBootEntryOrder_fn& on_func)
{
    if(d_->observers_NtQueryBootEntryOrder.empty())
        if(!register_callback_with(*d_, proc, "NtQueryBootEntryOrder", &on_NtQueryBootEntryOrder))
            return false;

    d_->observers_NtQueryBootEntryOrder.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryBootOptions(proc_t proc, const on_NtQueryBootOptions_fn& on_func)
{
    if(d_->observers_NtQueryBootOptions.empty())
        if(!register_callback_with(*d_, proc, "NtQueryBootOptions", &on_NtQueryBootOptions))
            return false;

    d_->observers_NtQueryBootOptions.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryDebugFilterState(proc_t proc, const on_NtQueryDebugFilterState_fn& on_func)
{
    if(d_->observers_NtQueryDebugFilterState.empty())
        if(!register_callback_with(*d_, proc, "NtQueryDebugFilterState", &on_NtQueryDebugFilterState))
            return false;

    d_->observers_NtQueryDebugFilterState.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryDefaultLocale(proc_t proc, const on_NtQueryDefaultLocale_fn& on_func)
{
    if(d_->observers_NtQueryDefaultLocale.empty())
        if(!register_callback_with(*d_, proc, "NtQueryDefaultLocale", &on_NtQueryDefaultLocale))
            return false;

    d_->observers_NtQueryDefaultLocale.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryDefaultUILanguage(proc_t proc, const on_NtQueryDefaultUILanguage_fn& on_func)
{
    if(d_->observers_NtQueryDefaultUILanguage.empty())
        if(!register_callback_with(*d_, proc, "NtQueryDefaultUILanguage", &on_NtQueryDefaultUILanguage))
            return false;

    d_->observers_NtQueryDefaultUILanguage.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryDirectoryFile(proc_t proc, const on_NtQueryDirectoryFile_fn& on_func)
{
    if(d_->observers_NtQueryDirectoryFile.empty())
        if(!register_callback_with(*d_, proc, "NtQueryDirectoryFile", &on_NtQueryDirectoryFile))
            return false;

    d_->observers_NtQueryDirectoryFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryDirectoryObject(proc_t proc, const on_NtQueryDirectoryObject_fn& on_func)
{
    if(d_->observers_NtQueryDirectoryObject.empty())
        if(!register_callback_with(*d_, proc, "NtQueryDirectoryObject", &on_NtQueryDirectoryObject))
            return false;

    d_->observers_NtQueryDirectoryObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryDriverEntryOrder(proc_t proc, const on_NtQueryDriverEntryOrder_fn& on_func)
{
    if(d_->observers_NtQueryDriverEntryOrder.empty())
        if(!register_callback_with(*d_, proc, "NtQueryDriverEntryOrder", &on_NtQueryDriverEntryOrder))
            return false;

    d_->observers_NtQueryDriverEntryOrder.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryEaFile(proc_t proc, const on_NtQueryEaFile_fn& on_func)
{
    if(d_->observers_NtQueryEaFile.empty())
        if(!register_callback_with(*d_, proc, "NtQueryEaFile", &on_NtQueryEaFile))
            return false;

    d_->observers_NtQueryEaFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryEvent(proc_t proc, const on_NtQueryEvent_fn& on_func)
{
    if(d_->observers_NtQueryEvent.empty())
        if(!register_callback_with(*d_, proc, "NtQueryEvent", &on_NtQueryEvent))
            return false;

    d_->observers_NtQueryEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryFullAttributesFile(proc_t proc, const on_NtQueryFullAttributesFile_fn& on_func)
{
    if(d_->observers_NtQueryFullAttributesFile.empty())
        if(!register_callback_with(*d_, proc, "NtQueryFullAttributesFile", &on_NtQueryFullAttributesFile))
            return false;

    d_->observers_NtQueryFullAttributesFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationAtom(proc_t proc, const on_NtQueryInformationAtom_fn& on_func)
{
    if(d_->observers_NtQueryInformationAtom.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationAtom", &on_NtQueryInformationAtom))
            return false;

    d_->observers_NtQueryInformationAtom.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationEnlistment(proc_t proc, const on_NtQueryInformationEnlistment_fn& on_func)
{
    if(d_->observers_NtQueryInformationEnlistment.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationEnlistment", &on_NtQueryInformationEnlistment))
            return false;

    d_->observers_NtQueryInformationEnlistment.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationFile(proc_t proc, const on_NtQueryInformationFile_fn& on_func)
{
    if(d_->observers_NtQueryInformationFile.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationFile", &on_NtQueryInformationFile))
            return false;

    d_->observers_NtQueryInformationFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationJobObject(proc_t proc, const on_NtQueryInformationJobObject_fn& on_func)
{
    if(d_->observers_NtQueryInformationJobObject.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationJobObject", &on_NtQueryInformationJobObject))
            return false;

    d_->observers_NtQueryInformationJobObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationPort(proc_t proc, const on_NtQueryInformationPort_fn& on_func)
{
    if(d_->observers_NtQueryInformationPort.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationPort", &on_NtQueryInformationPort))
            return false;

    d_->observers_NtQueryInformationPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationProcess(proc_t proc, const on_NtQueryInformationProcess_fn& on_func)
{
    if(d_->observers_NtQueryInformationProcess.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationProcess", &on_NtQueryInformationProcess))
            return false;

    d_->observers_NtQueryInformationProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationResourceManager(proc_t proc, const on_NtQueryInformationResourceManager_fn& on_func)
{
    if(d_->observers_NtQueryInformationResourceManager.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationResourceManager", &on_NtQueryInformationResourceManager))
            return false;

    d_->observers_NtQueryInformationResourceManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationThread(proc_t proc, const on_NtQueryInformationThread_fn& on_func)
{
    if(d_->observers_NtQueryInformationThread.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationThread", &on_NtQueryInformationThread))
            return false;

    d_->observers_NtQueryInformationThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationToken(proc_t proc, const on_NtQueryInformationToken_fn& on_func)
{
    if(d_->observers_NtQueryInformationToken.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationToken", &on_NtQueryInformationToken))
            return false;

    d_->observers_NtQueryInformationToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationTransaction(proc_t proc, const on_NtQueryInformationTransaction_fn& on_func)
{
    if(d_->observers_NtQueryInformationTransaction.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationTransaction", &on_NtQueryInformationTransaction))
            return false;

    d_->observers_NtQueryInformationTransaction.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationTransactionManager(proc_t proc, const on_NtQueryInformationTransactionManager_fn& on_func)
{
    if(d_->observers_NtQueryInformationTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationTransactionManager", &on_NtQueryInformationTransactionManager))
            return false;

    d_->observers_NtQueryInformationTransactionManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInformationWorkerFactory(proc_t proc, const on_NtQueryInformationWorkerFactory_fn& on_func)
{
    if(d_->observers_NtQueryInformationWorkerFactory.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInformationWorkerFactory", &on_NtQueryInformationWorkerFactory))
            return false;

    d_->observers_NtQueryInformationWorkerFactory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryInstallUILanguage(proc_t proc, const on_NtQueryInstallUILanguage_fn& on_func)
{
    if(d_->observers_NtQueryInstallUILanguage.empty())
        if(!register_callback_with(*d_, proc, "NtQueryInstallUILanguage", &on_NtQueryInstallUILanguage))
            return false;

    d_->observers_NtQueryInstallUILanguage.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryIntervalProfile(proc_t proc, const on_NtQueryIntervalProfile_fn& on_func)
{
    if(d_->observers_NtQueryIntervalProfile.empty())
        if(!register_callback_with(*d_, proc, "NtQueryIntervalProfile", &on_NtQueryIntervalProfile))
            return false;

    d_->observers_NtQueryIntervalProfile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryIoCompletion(proc_t proc, const on_NtQueryIoCompletion_fn& on_func)
{
    if(d_->observers_NtQueryIoCompletion.empty())
        if(!register_callback_with(*d_, proc, "NtQueryIoCompletion", &on_NtQueryIoCompletion))
            return false;

    d_->observers_NtQueryIoCompletion.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryKey(proc_t proc, const on_NtQueryKey_fn& on_func)
{
    if(d_->observers_NtQueryKey.empty())
        if(!register_callback_with(*d_, proc, "NtQueryKey", &on_NtQueryKey))
            return false;

    d_->observers_NtQueryKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryLicenseValue(proc_t proc, const on_NtQueryLicenseValue_fn& on_func)
{
    if(d_->observers_NtQueryLicenseValue.empty())
        if(!register_callback_with(*d_, proc, "NtQueryLicenseValue", &on_NtQueryLicenseValue))
            return false;

    d_->observers_NtQueryLicenseValue.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryMultipleValueKey(proc_t proc, const on_NtQueryMultipleValueKey_fn& on_func)
{
    if(d_->observers_NtQueryMultipleValueKey.empty())
        if(!register_callback_with(*d_, proc, "NtQueryMultipleValueKey", &on_NtQueryMultipleValueKey))
            return false;

    d_->observers_NtQueryMultipleValueKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryMutant(proc_t proc, const on_NtQueryMutant_fn& on_func)
{
    if(d_->observers_NtQueryMutant.empty())
        if(!register_callback_with(*d_, proc, "NtQueryMutant", &on_NtQueryMutant))
            return false;

    d_->observers_NtQueryMutant.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryObject(proc_t proc, const on_NtQueryObject_fn& on_func)
{
    if(d_->observers_NtQueryObject.empty())
        if(!register_callback_with(*d_, proc, "NtQueryObject", &on_NtQueryObject))
            return false;

    d_->observers_NtQueryObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryOpenSubKeysEx(proc_t proc, const on_NtQueryOpenSubKeysEx_fn& on_func)
{
    if(d_->observers_NtQueryOpenSubKeysEx.empty())
        if(!register_callback_with(*d_, proc, "NtQueryOpenSubKeysEx", &on_NtQueryOpenSubKeysEx))
            return false;

    d_->observers_NtQueryOpenSubKeysEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryOpenSubKeys(proc_t proc, const on_NtQueryOpenSubKeys_fn& on_func)
{
    if(d_->observers_NtQueryOpenSubKeys.empty())
        if(!register_callback_with(*d_, proc, "NtQueryOpenSubKeys", &on_NtQueryOpenSubKeys))
            return false;

    d_->observers_NtQueryOpenSubKeys.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryPerformanceCounter(proc_t proc, const on_NtQueryPerformanceCounter_fn& on_func)
{
    if(d_->observers_NtQueryPerformanceCounter.empty())
        if(!register_callback_with(*d_, proc, "NtQueryPerformanceCounter", &on_NtQueryPerformanceCounter))
            return false;

    d_->observers_NtQueryPerformanceCounter.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryQuotaInformationFile(proc_t proc, const on_NtQueryQuotaInformationFile_fn& on_func)
{
    if(d_->observers_NtQueryQuotaInformationFile.empty())
        if(!register_callback_with(*d_, proc, "NtQueryQuotaInformationFile", &on_NtQueryQuotaInformationFile))
            return false;

    d_->observers_NtQueryQuotaInformationFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQuerySection(proc_t proc, const on_NtQuerySection_fn& on_func)
{
    if(d_->observers_NtQuerySection.empty())
        if(!register_callback_with(*d_, proc, "NtQuerySection", &on_NtQuerySection))
            return false;

    d_->observers_NtQuerySection.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQuerySecurityAttributesToken(proc_t proc, const on_NtQuerySecurityAttributesToken_fn& on_func)
{
    if(d_->observers_NtQuerySecurityAttributesToken.empty())
        if(!register_callback_with(*d_, proc, "NtQuerySecurityAttributesToken", &on_NtQuerySecurityAttributesToken))
            return false;

    d_->observers_NtQuerySecurityAttributesToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQuerySecurityObject(proc_t proc, const on_NtQuerySecurityObject_fn& on_func)
{
    if(d_->observers_NtQuerySecurityObject.empty())
        if(!register_callback_with(*d_, proc, "NtQuerySecurityObject", &on_NtQuerySecurityObject))
            return false;

    d_->observers_NtQuerySecurityObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQuerySemaphore(proc_t proc, const on_NtQuerySemaphore_fn& on_func)
{
    if(d_->observers_NtQuerySemaphore.empty())
        if(!register_callback_with(*d_, proc, "NtQuerySemaphore", &on_NtQuerySemaphore))
            return false;

    d_->observers_NtQuerySemaphore.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQuerySymbolicLinkObject(proc_t proc, const on_NtQuerySymbolicLinkObject_fn& on_func)
{
    if(d_->observers_NtQuerySymbolicLinkObject.empty())
        if(!register_callback_with(*d_, proc, "NtQuerySymbolicLinkObject", &on_NtQuerySymbolicLinkObject))
            return false;

    d_->observers_NtQuerySymbolicLinkObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQuerySystemEnvironmentValueEx(proc_t proc, const on_NtQuerySystemEnvironmentValueEx_fn& on_func)
{
    if(d_->observers_NtQuerySystemEnvironmentValueEx.empty())
        if(!register_callback_with(*d_, proc, "NtQuerySystemEnvironmentValueEx", &on_NtQuerySystemEnvironmentValueEx))
            return false;

    d_->observers_NtQuerySystemEnvironmentValueEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQuerySystemEnvironmentValue(proc_t proc, const on_NtQuerySystemEnvironmentValue_fn& on_func)
{
    if(d_->observers_NtQuerySystemEnvironmentValue.empty())
        if(!register_callback_with(*d_, proc, "NtQuerySystemEnvironmentValue", &on_NtQuerySystemEnvironmentValue))
            return false;

    d_->observers_NtQuerySystemEnvironmentValue.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQuerySystemInformationEx(proc_t proc, const on_NtQuerySystemInformationEx_fn& on_func)
{
    if(d_->observers_NtQuerySystemInformationEx.empty())
        if(!register_callback_with(*d_, proc, "NtQuerySystemInformationEx", &on_NtQuerySystemInformationEx))
            return false;

    d_->observers_NtQuerySystemInformationEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQuerySystemInformation(proc_t proc, const on_NtQuerySystemInformation_fn& on_func)
{
    if(d_->observers_NtQuerySystemInformation.empty())
        if(!register_callback_with(*d_, proc, "NtQuerySystemInformation", &on_NtQuerySystemInformation))
            return false;

    d_->observers_NtQuerySystemInformation.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQuerySystemTime(proc_t proc, const on_NtQuerySystemTime_fn& on_func)
{
    if(d_->observers_NtQuerySystemTime.empty())
        if(!register_callback_with(*d_, proc, "NtQuerySystemTime", &on_NtQuerySystemTime))
            return false;

    d_->observers_NtQuerySystemTime.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryTimer(proc_t proc, const on_NtQueryTimer_fn& on_func)
{
    if(d_->observers_NtQueryTimer.empty())
        if(!register_callback_with(*d_, proc, "NtQueryTimer", &on_NtQueryTimer))
            return false;

    d_->observers_NtQueryTimer.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryTimerResolution(proc_t proc, const on_NtQueryTimerResolution_fn& on_func)
{
    if(d_->observers_NtQueryTimerResolution.empty())
        if(!register_callback_with(*d_, proc, "NtQueryTimerResolution", &on_NtQueryTimerResolution))
            return false;

    d_->observers_NtQueryTimerResolution.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryValueKey(proc_t proc, const on_NtQueryValueKey_fn& on_func)
{
    if(d_->observers_NtQueryValueKey.empty())
        if(!register_callback_with(*d_, proc, "NtQueryValueKey", &on_NtQueryValueKey))
            return false;

    d_->observers_NtQueryValueKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryVirtualMemory(proc_t proc, const on_NtQueryVirtualMemory_fn& on_func)
{
    if(d_->observers_NtQueryVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "NtQueryVirtualMemory", &on_NtQueryVirtualMemory))
            return false;

    d_->observers_NtQueryVirtualMemory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryVolumeInformationFile(proc_t proc, const on_NtQueryVolumeInformationFile_fn& on_func)
{
    if(d_->observers_NtQueryVolumeInformationFile.empty())
        if(!register_callback_with(*d_, proc, "NtQueryVolumeInformationFile", &on_NtQueryVolumeInformationFile))
            return false;

    d_->observers_NtQueryVolumeInformationFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueueApcThreadEx(proc_t proc, const on_NtQueueApcThreadEx_fn& on_func)
{
    if(d_->observers_NtQueueApcThreadEx.empty())
        if(!register_callback_with(*d_, proc, "NtQueueApcThreadEx", &on_NtQueueApcThreadEx))
            return false;

    d_->observers_NtQueueApcThreadEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueueApcThread(proc_t proc, const on_NtQueueApcThread_fn& on_func)
{
    if(d_->observers_NtQueueApcThread.empty())
        if(!register_callback_with(*d_, proc, "NtQueueApcThread", &on_NtQueueApcThread))
            return false;

    d_->observers_NtQueueApcThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRaiseException(proc_t proc, const on_NtRaiseException_fn& on_func)
{
    if(d_->observers_NtRaiseException.empty())
        if(!register_callback_with(*d_, proc, "NtRaiseException", &on_NtRaiseException))
            return false;

    d_->observers_NtRaiseException.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRaiseHardError(proc_t proc, const on_NtRaiseHardError_fn& on_func)
{
    if(d_->observers_NtRaiseHardError.empty())
        if(!register_callback_with(*d_, proc, "NtRaiseHardError", &on_NtRaiseHardError))
            return false;

    d_->observers_NtRaiseHardError.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReadFile(proc_t proc, const on_NtReadFile_fn& on_func)
{
    if(d_->observers_NtReadFile.empty())
        if(!register_callback_with(*d_, proc, "NtReadFile", &on_NtReadFile))
            return false;

    d_->observers_NtReadFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReadFileScatter(proc_t proc, const on_NtReadFileScatter_fn& on_func)
{
    if(d_->observers_NtReadFileScatter.empty())
        if(!register_callback_with(*d_, proc, "NtReadFileScatter", &on_NtReadFileScatter))
            return false;

    d_->observers_NtReadFileScatter.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReadOnlyEnlistment(proc_t proc, const on_NtReadOnlyEnlistment_fn& on_func)
{
    if(d_->observers_NtReadOnlyEnlistment.empty())
        if(!register_callback_with(*d_, proc, "NtReadOnlyEnlistment", &on_NtReadOnlyEnlistment))
            return false;

    d_->observers_NtReadOnlyEnlistment.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReadRequestData(proc_t proc, const on_NtReadRequestData_fn& on_func)
{
    if(d_->observers_NtReadRequestData.empty())
        if(!register_callback_with(*d_, proc, "NtReadRequestData", &on_NtReadRequestData))
            return false;

    d_->observers_NtReadRequestData.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReadVirtualMemory(proc_t proc, const on_NtReadVirtualMemory_fn& on_func)
{
    if(d_->observers_NtReadVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "NtReadVirtualMemory", &on_NtReadVirtualMemory))
            return false;

    d_->observers_NtReadVirtualMemory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRecoverEnlistment(proc_t proc, const on_NtRecoverEnlistment_fn& on_func)
{
    if(d_->observers_NtRecoverEnlistment.empty())
        if(!register_callback_with(*d_, proc, "NtRecoverEnlistment", &on_NtRecoverEnlistment))
            return false;

    d_->observers_NtRecoverEnlistment.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRecoverResourceManager(proc_t proc, const on_NtRecoverResourceManager_fn& on_func)
{
    if(d_->observers_NtRecoverResourceManager.empty())
        if(!register_callback_with(*d_, proc, "NtRecoverResourceManager", &on_NtRecoverResourceManager))
            return false;

    d_->observers_NtRecoverResourceManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRecoverTransactionManager(proc_t proc, const on_NtRecoverTransactionManager_fn& on_func)
{
    if(d_->observers_NtRecoverTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "NtRecoverTransactionManager", &on_NtRecoverTransactionManager))
            return false;

    d_->observers_NtRecoverTransactionManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRegisterProtocolAddressInformation(proc_t proc, const on_NtRegisterProtocolAddressInformation_fn& on_func)
{
    if(d_->observers_NtRegisterProtocolAddressInformation.empty())
        if(!register_callback_with(*d_, proc, "NtRegisterProtocolAddressInformation", &on_NtRegisterProtocolAddressInformation))
            return false;

    d_->observers_NtRegisterProtocolAddressInformation.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRegisterThreadTerminatePort(proc_t proc, const on_NtRegisterThreadTerminatePort_fn& on_func)
{
    if(d_->observers_NtRegisterThreadTerminatePort.empty())
        if(!register_callback_with(*d_, proc, "NtRegisterThreadTerminatePort", &on_NtRegisterThreadTerminatePort))
            return false;

    d_->observers_NtRegisterThreadTerminatePort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReleaseKeyedEvent(proc_t proc, const on_NtReleaseKeyedEvent_fn& on_func)
{
    if(d_->observers_NtReleaseKeyedEvent.empty())
        if(!register_callback_with(*d_, proc, "NtReleaseKeyedEvent", &on_NtReleaseKeyedEvent))
            return false;

    d_->observers_NtReleaseKeyedEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReleaseMutant(proc_t proc, const on_NtReleaseMutant_fn& on_func)
{
    if(d_->observers_NtReleaseMutant.empty())
        if(!register_callback_with(*d_, proc, "NtReleaseMutant", &on_NtReleaseMutant))
            return false;

    d_->observers_NtReleaseMutant.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReleaseSemaphore(proc_t proc, const on_NtReleaseSemaphore_fn& on_func)
{
    if(d_->observers_NtReleaseSemaphore.empty())
        if(!register_callback_with(*d_, proc, "NtReleaseSemaphore", &on_NtReleaseSemaphore))
            return false;

    d_->observers_NtReleaseSemaphore.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReleaseWorkerFactoryWorker(proc_t proc, const on_NtReleaseWorkerFactoryWorker_fn& on_func)
{
    if(d_->observers_NtReleaseWorkerFactoryWorker.empty())
        if(!register_callback_with(*d_, proc, "NtReleaseWorkerFactoryWorker", &on_NtReleaseWorkerFactoryWorker))
            return false;

    d_->observers_NtReleaseWorkerFactoryWorker.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRemoveIoCompletionEx(proc_t proc, const on_NtRemoveIoCompletionEx_fn& on_func)
{
    if(d_->observers_NtRemoveIoCompletionEx.empty())
        if(!register_callback_with(*d_, proc, "NtRemoveIoCompletionEx", &on_NtRemoveIoCompletionEx))
            return false;

    d_->observers_NtRemoveIoCompletionEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRemoveIoCompletion(proc_t proc, const on_NtRemoveIoCompletion_fn& on_func)
{
    if(d_->observers_NtRemoveIoCompletion.empty())
        if(!register_callback_with(*d_, proc, "NtRemoveIoCompletion", &on_NtRemoveIoCompletion))
            return false;

    d_->observers_NtRemoveIoCompletion.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRemoveProcessDebug(proc_t proc, const on_NtRemoveProcessDebug_fn& on_func)
{
    if(d_->observers_NtRemoveProcessDebug.empty())
        if(!register_callback_with(*d_, proc, "NtRemoveProcessDebug", &on_NtRemoveProcessDebug))
            return false;

    d_->observers_NtRemoveProcessDebug.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRenameKey(proc_t proc, const on_NtRenameKey_fn& on_func)
{
    if(d_->observers_NtRenameKey.empty())
        if(!register_callback_with(*d_, proc, "NtRenameKey", &on_NtRenameKey))
            return false;

    d_->observers_NtRenameKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRenameTransactionManager(proc_t proc, const on_NtRenameTransactionManager_fn& on_func)
{
    if(d_->observers_NtRenameTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "NtRenameTransactionManager", &on_NtRenameTransactionManager))
            return false;

    d_->observers_NtRenameTransactionManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReplaceKey(proc_t proc, const on_NtReplaceKey_fn& on_func)
{
    if(d_->observers_NtReplaceKey.empty())
        if(!register_callback_with(*d_, proc, "NtReplaceKey", &on_NtReplaceKey))
            return false;

    d_->observers_NtReplaceKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReplacePartitionUnit(proc_t proc, const on_NtReplacePartitionUnit_fn& on_func)
{
    if(d_->observers_NtReplacePartitionUnit.empty())
        if(!register_callback_with(*d_, proc, "NtReplacePartitionUnit", &on_NtReplacePartitionUnit))
            return false;

    d_->observers_NtReplacePartitionUnit.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReplyPort(proc_t proc, const on_NtReplyPort_fn& on_func)
{
    if(d_->observers_NtReplyPort.empty())
        if(!register_callback_with(*d_, proc, "NtReplyPort", &on_NtReplyPort))
            return false;

    d_->observers_NtReplyPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReplyWaitReceivePortEx(proc_t proc, const on_NtReplyWaitReceivePortEx_fn& on_func)
{
    if(d_->observers_NtReplyWaitReceivePortEx.empty())
        if(!register_callback_with(*d_, proc, "NtReplyWaitReceivePortEx", &on_NtReplyWaitReceivePortEx))
            return false;

    d_->observers_NtReplyWaitReceivePortEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReplyWaitReceivePort(proc_t proc, const on_NtReplyWaitReceivePort_fn& on_func)
{
    if(d_->observers_NtReplyWaitReceivePort.empty())
        if(!register_callback_with(*d_, proc, "NtReplyWaitReceivePort", &on_NtReplyWaitReceivePort))
            return false;

    d_->observers_NtReplyWaitReceivePort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtReplyWaitReplyPort(proc_t proc, const on_NtReplyWaitReplyPort_fn& on_func)
{
    if(d_->observers_NtReplyWaitReplyPort.empty())
        if(!register_callback_with(*d_, proc, "NtReplyWaitReplyPort", &on_NtReplyWaitReplyPort))
            return false;

    d_->observers_NtReplyWaitReplyPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRequestPort(proc_t proc, const on_NtRequestPort_fn& on_func)
{
    if(d_->observers_NtRequestPort.empty())
        if(!register_callback_with(*d_, proc, "NtRequestPort", &on_NtRequestPort))
            return false;

    d_->observers_NtRequestPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRequestWaitReplyPort(proc_t proc, const on_NtRequestWaitReplyPort_fn& on_func)
{
    if(d_->observers_NtRequestWaitReplyPort.empty())
        if(!register_callback_with(*d_, proc, "NtRequestWaitReplyPort", &on_NtRequestWaitReplyPort))
            return false;

    d_->observers_NtRequestWaitReplyPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtResetEvent(proc_t proc, const on_NtResetEvent_fn& on_func)
{
    if(d_->observers_NtResetEvent.empty())
        if(!register_callback_with(*d_, proc, "NtResetEvent", &on_NtResetEvent))
            return false;

    d_->observers_NtResetEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtResetWriteWatch(proc_t proc, const on_NtResetWriteWatch_fn& on_func)
{
    if(d_->observers_NtResetWriteWatch.empty())
        if(!register_callback_with(*d_, proc, "NtResetWriteWatch", &on_NtResetWriteWatch))
            return false;

    d_->observers_NtResetWriteWatch.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRestoreKey(proc_t proc, const on_NtRestoreKey_fn& on_func)
{
    if(d_->observers_NtRestoreKey.empty())
        if(!register_callback_with(*d_, proc, "NtRestoreKey", &on_NtRestoreKey))
            return false;

    d_->observers_NtRestoreKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtResumeProcess(proc_t proc, const on_NtResumeProcess_fn& on_func)
{
    if(d_->observers_NtResumeProcess.empty())
        if(!register_callback_with(*d_, proc, "NtResumeProcess", &on_NtResumeProcess))
            return false;

    d_->observers_NtResumeProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtResumeThread(proc_t proc, const on_NtResumeThread_fn& on_func)
{
    if(d_->observers_NtResumeThread.empty())
        if(!register_callback_with(*d_, proc, "NtResumeThread", &on_NtResumeThread))
            return false;

    d_->observers_NtResumeThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRollbackComplete(proc_t proc, const on_NtRollbackComplete_fn& on_func)
{
    if(d_->observers_NtRollbackComplete.empty())
        if(!register_callback_with(*d_, proc, "NtRollbackComplete", &on_NtRollbackComplete))
            return false;

    d_->observers_NtRollbackComplete.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRollbackEnlistment(proc_t proc, const on_NtRollbackEnlistment_fn& on_func)
{
    if(d_->observers_NtRollbackEnlistment.empty())
        if(!register_callback_with(*d_, proc, "NtRollbackEnlistment", &on_NtRollbackEnlistment))
            return false;

    d_->observers_NtRollbackEnlistment.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRollbackTransaction(proc_t proc, const on_NtRollbackTransaction_fn& on_func)
{
    if(d_->observers_NtRollbackTransaction.empty())
        if(!register_callback_with(*d_, proc, "NtRollbackTransaction", &on_NtRollbackTransaction))
            return false;

    d_->observers_NtRollbackTransaction.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtRollforwardTransactionManager(proc_t proc, const on_NtRollforwardTransactionManager_fn& on_func)
{
    if(d_->observers_NtRollforwardTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "NtRollforwardTransactionManager", &on_NtRollforwardTransactionManager))
            return false;

    d_->observers_NtRollforwardTransactionManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSaveKeyEx(proc_t proc, const on_NtSaveKeyEx_fn& on_func)
{
    if(d_->observers_NtSaveKeyEx.empty())
        if(!register_callback_with(*d_, proc, "NtSaveKeyEx", &on_NtSaveKeyEx))
            return false;

    d_->observers_NtSaveKeyEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSaveKey(proc_t proc, const on_NtSaveKey_fn& on_func)
{
    if(d_->observers_NtSaveKey.empty())
        if(!register_callback_with(*d_, proc, "NtSaveKey", &on_NtSaveKey))
            return false;

    d_->observers_NtSaveKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSaveMergedKeys(proc_t proc, const on_NtSaveMergedKeys_fn& on_func)
{
    if(d_->observers_NtSaveMergedKeys.empty())
        if(!register_callback_with(*d_, proc, "NtSaveMergedKeys", &on_NtSaveMergedKeys))
            return false;

    d_->observers_NtSaveMergedKeys.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSecureConnectPort(proc_t proc, const on_NtSecureConnectPort_fn& on_func)
{
    if(d_->observers_NtSecureConnectPort.empty())
        if(!register_callback_with(*d_, proc, "NtSecureConnectPort", &on_NtSecureConnectPort))
            return false;

    d_->observers_NtSecureConnectPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetBootEntryOrder(proc_t proc, const on_NtSetBootEntryOrder_fn& on_func)
{
    if(d_->observers_NtSetBootEntryOrder.empty())
        if(!register_callback_with(*d_, proc, "NtSetBootEntryOrder", &on_NtSetBootEntryOrder))
            return false;

    d_->observers_NtSetBootEntryOrder.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetBootOptions(proc_t proc, const on_NtSetBootOptions_fn& on_func)
{
    if(d_->observers_NtSetBootOptions.empty())
        if(!register_callback_with(*d_, proc, "NtSetBootOptions", &on_NtSetBootOptions))
            return false;

    d_->observers_NtSetBootOptions.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetContextThread(proc_t proc, const on_NtSetContextThread_fn& on_func)
{
    if(d_->observers_NtSetContextThread.empty())
        if(!register_callback_with(*d_, proc, "NtSetContextThread", &on_NtSetContextThread))
            return false;

    d_->observers_NtSetContextThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetDebugFilterState(proc_t proc, const on_NtSetDebugFilterState_fn& on_func)
{
    if(d_->observers_NtSetDebugFilterState.empty())
        if(!register_callback_with(*d_, proc, "NtSetDebugFilterState", &on_NtSetDebugFilterState))
            return false;

    d_->observers_NtSetDebugFilterState.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetDefaultHardErrorPort(proc_t proc, const on_NtSetDefaultHardErrorPort_fn& on_func)
{
    if(d_->observers_NtSetDefaultHardErrorPort.empty())
        if(!register_callback_with(*d_, proc, "NtSetDefaultHardErrorPort", &on_NtSetDefaultHardErrorPort))
            return false;

    d_->observers_NtSetDefaultHardErrorPort.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetDefaultLocale(proc_t proc, const on_NtSetDefaultLocale_fn& on_func)
{
    if(d_->observers_NtSetDefaultLocale.empty())
        if(!register_callback_with(*d_, proc, "NtSetDefaultLocale", &on_NtSetDefaultLocale))
            return false;

    d_->observers_NtSetDefaultLocale.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetDefaultUILanguage(proc_t proc, const on_NtSetDefaultUILanguage_fn& on_func)
{
    if(d_->observers_NtSetDefaultUILanguage.empty())
        if(!register_callback_with(*d_, proc, "NtSetDefaultUILanguage", &on_NtSetDefaultUILanguage))
            return false;

    d_->observers_NtSetDefaultUILanguage.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetDriverEntryOrder(proc_t proc, const on_NtSetDriverEntryOrder_fn& on_func)
{
    if(d_->observers_NtSetDriverEntryOrder.empty())
        if(!register_callback_with(*d_, proc, "NtSetDriverEntryOrder", &on_NtSetDriverEntryOrder))
            return false;

    d_->observers_NtSetDriverEntryOrder.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetEaFile(proc_t proc, const on_NtSetEaFile_fn& on_func)
{
    if(d_->observers_NtSetEaFile.empty())
        if(!register_callback_with(*d_, proc, "NtSetEaFile", &on_NtSetEaFile))
            return false;

    d_->observers_NtSetEaFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetEventBoostPriority(proc_t proc, const on_NtSetEventBoostPriority_fn& on_func)
{
    if(d_->observers_NtSetEventBoostPriority.empty())
        if(!register_callback_with(*d_, proc, "NtSetEventBoostPriority", &on_NtSetEventBoostPriority))
            return false;

    d_->observers_NtSetEventBoostPriority.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetEvent(proc_t proc, const on_NtSetEvent_fn& on_func)
{
    if(d_->observers_NtSetEvent.empty())
        if(!register_callback_with(*d_, proc, "NtSetEvent", &on_NtSetEvent))
            return false;

    d_->observers_NtSetEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetHighEventPair(proc_t proc, const on_NtSetHighEventPair_fn& on_func)
{
    if(d_->observers_NtSetHighEventPair.empty())
        if(!register_callback_with(*d_, proc, "NtSetHighEventPair", &on_NtSetHighEventPair))
            return false;

    d_->observers_NtSetHighEventPair.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetHighWaitLowEventPair(proc_t proc, const on_NtSetHighWaitLowEventPair_fn& on_func)
{
    if(d_->observers_NtSetHighWaitLowEventPair.empty())
        if(!register_callback_with(*d_, proc, "NtSetHighWaitLowEventPair", &on_NtSetHighWaitLowEventPair))
            return false;

    d_->observers_NtSetHighWaitLowEventPair.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationDebugObject(proc_t proc, const on_NtSetInformationDebugObject_fn& on_func)
{
    if(d_->observers_NtSetInformationDebugObject.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationDebugObject", &on_NtSetInformationDebugObject))
            return false;

    d_->observers_NtSetInformationDebugObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationEnlistment(proc_t proc, const on_NtSetInformationEnlistment_fn& on_func)
{
    if(d_->observers_NtSetInformationEnlistment.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationEnlistment", &on_NtSetInformationEnlistment))
            return false;

    d_->observers_NtSetInformationEnlistment.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationFile(proc_t proc, const on_NtSetInformationFile_fn& on_func)
{
    if(d_->observers_NtSetInformationFile.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationFile", &on_NtSetInformationFile))
            return false;

    d_->observers_NtSetInformationFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationJobObject(proc_t proc, const on_NtSetInformationJobObject_fn& on_func)
{
    if(d_->observers_NtSetInformationJobObject.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationJobObject", &on_NtSetInformationJobObject))
            return false;

    d_->observers_NtSetInformationJobObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationKey(proc_t proc, const on_NtSetInformationKey_fn& on_func)
{
    if(d_->observers_NtSetInformationKey.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationKey", &on_NtSetInformationKey))
            return false;

    d_->observers_NtSetInformationKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationObject(proc_t proc, const on_NtSetInformationObject_fn& on_func)
{
    if(d_->observers_NtSetInformationObject.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationObject", &on_NtSetInformationObject))
            return false;

    d_->observers_NtSetInformationObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationProcess(proc_t proc, const on_NtSetInformationProcess_fn& on_func)
{
    if(d_->observers_NtSetInformationProcess.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationProcess", &on_NtSetInformationProcess))
            return false;

    d_->observers_NtSetInformationProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationResourceManager(proc_t proc, const on_NtSetInformationResourceManager_fn& on_func)
{
    if(d_->observers_NtSetInformationResourceManager.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationResourceManager", &on_NtSetInformationResourceManager))
            return false;

    d_->observers_NtSetInformationResourceManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationThread(proc_t proc, const on_NtSetInformationThread_fn& on_func)
{
    if(d_->observers_NtSetInformationThread.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationThread", &on_NtSetInformationThread))
            return false;

    d_->observers_NtSetInformationThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationToken(proc_t proc, const on_NtSetInformationToken_fn& on_func)
{
    if(d_->observers_NtSetInformationToken.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationToken", &on_NtSetInformationToken))
            return false;

    d_->observers_NtSetInformationToken.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationTransaction(proc_t proc, const on_NtSetInformationTransaction_fn& on_func)
{
    if(d_->observers_NtSetInformationTransaction.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationTransaction", &on_NtSetInformationTransaction))
            return false;

    d_->observers_NtSetInformationTransaction.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationTransactionManager(proc_t proc, const on_NtSetInformationTransactionManager_fn& on_func)
{
    if(d_->observers_NtSetInformationTransactionManager.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationTransactionManager", &on_NtSetInformationTransactionManager))
            return false;

    d_->observers_NtSetInformationTransactionManager.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetInformationWorkerFactory(proc_t proc, const on_NtSetInformationWorkerFactory_fn& on_func)
{
    if(d_->observers_NtSetInformationWorkerFactory.empty())
        if(!register_callback_with(*d_, proc, "NtSetInformationWorkerFactory", &on_NtSetInformationWorkerFactory))
            return false;

    d_->observers_NtSetInformationWorkerFactory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetIntervalProfile(proc_t proc, const on_NtSetIntervalProfile_fn& on_func)
{
    if(d_->observers_NtSetIntervalProfile.empty())
        if(!register_callback_with(*d_, proc, "NtSetIntervalProfile", &on_NtSetIntervalProfile))
            return false;

    d_->observers_NtSetIntervalProfile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetIoCompletionEx(proc_t proc, const on_NtSetIoCompletionEx_fn& on_func)
{
    if(d_->observers_NtSetIoCompletionEx.empty())
        if(!register_callback_with(*d_, proc, "NtSetIoCompletionEx", &on_NtSetIoCompletionEx))
            return false;

    d_->observers_NtSetIoCompletionEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetIoCompletion(proc_t proc, const on_NtSetIoCompletion_fn& on_func)
{
    if(d_->observers_NtSetIoCompletion.empty())
        if(!register_callback_with(*d_, proc, "NtSetIoCompletion", &on_NtSetIoCompletion))
            return false;

    d_->observers_NtSetIoCompletion.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetLdtEntries(proc_t proc, const on_NtSetLdtEntries_fn& on_func)
{
    if(d_->observers_NtSetLdtEntries.empty())
        if(!register_callback_with(*d_, proc, "NtSetLdtEntries", &on_NtSetLdtEntries))
            return false;

    d_->observers_NtSetLdtEntries.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetLowEventPair(proc_t proc, const on_NtSetLowEventPair_fn& on_func)
{
    if(d_->observers_NtSetLowEventPair.empty())
        if(!register_callback_with(*d_, proc, "NtSetLowEventPair", &on_NtSetLowEventPair))
            return false;

    d_->observers_NtSetLowEventPair.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetLowWaitHighEventPair(proc_t proc, const on_NtSetLowWaitHighEventPair_fn& on_func)
{
    if(d_->observers_NtSetLowWaitHighEventPair.empty())
        if(!register_callback_with(*d_, proc, "NtSetLowWaitHighEventPair", &on_NtSetLowWaitHighEventPair))
            return false;

    d_->observers_NtSetLowWaitHighEventPair.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetQuotaInformationFile(proc_t proc, const on_NtSetQuotaInformationFile_fn& on_func)
{
    if(d_->observers_NtSetQuotaInformationFile.empty())
        if(!register_callback_with(*d_, proc, "NtSetQuotaInformationFile", &on_NtSetQuotaInformationFile))
            return false;

    d_->observers_NtSetQuotaInformationFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetSecurityObject(proc_t proc, const on_NtSetSecurityObject_fn& on_func)
{
    if(d_->observers_NtSetSecurityObject.empty())
        if(!register_callback_with(*d_, proc, "NtSetSecurityObject", &on_NtSetSecurityObject))
            return false;

    d_->observers_NtSetSecurityObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetSystemEnvironmentValueEx(proc_t proc, const on_NtSetSystemEnvironmentValueEx_fn& on_func)
{
    if(d_->observers_NtSetSystemEnvironmentValueEx.empty())
        if(!register_callback_with(*d_, proc, "NtSetSystemEnvironmentValueEx", &on_NtSetSystemEnvironmentValueEx))
            return false;

    d_->observers_NtSetSystemEnvironmentValueEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetSystemEnvironmentValue(proc_t proc, const on_NtSetSystemEnvironmentValue_fn& on_func)
{
    if(d_->observers_NtSetSystemEnvironmentValue.empty())
        if(!register_callback_with(*d_, proc, "NtSetSystemEnvironmentValue", &on_NtSetSystemEnvironmentValue))
            return false;

    d_->observers_NtSetSystemEnvironmentValue.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetSystemInformation(proc_t proc, const on_NtSetSystemInformation_fn& on_func)
{
    if(d_->observers_NtSetSystemInformation.empty())
        if(!register_callback_with(*d_, proc, "NtSetSystemInformation", &on_NtSetSystemInformation))
            return false;

    d_->observers_NtSetSystemInformation.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetSystemPowerState(proc_t proc, const on_NtSetSystemPowerState_fn& on_func)
{
    if(d_->observers_NtSetSystemPowerState.empty())
        if(!register_callback_with(*d_, proc, "NtSetSystemPowerState", &on_NtSetSystemPowerState))
            return false;

    d_->observers_NtSetSystemPowerState.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetSystemTime(proc_t proc, const on_NtSetSystemTime_fn& on_func)
{
    if(d_->observers_NtSetSystemTime.empty())
        if(!register_callback_with(*d_, proc, "NtSetSystemTime", &on_NtSetSystemTime))
            return false;

    d_->observers_NtSetSystemTime.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetThreadExecutionState(proc_t proc, const on_NtSetThreadExecutionState_fn& on_func)
{
    if(d_->observers_NtSetThreadExecutionState.empty())
        if(!register_callback_with(*d_, proc, "NtSetThreadExecutionState", &on_NtSetThreadExecutionState))
            return false;

    d_->observers_NtSetThreadExecutionState.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetTimerEx(proc_t proc, const on_NtSetTimerEx_fn& on_func)
{
    if(d_->observers_NtSetTimerEx.empty())
        if(!register_callback_with(*d_, proc, "NtSetTimerEx", &on_NtSetTimerEx))
            return false;

    d_->observers_NtSetTimerEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetTimer(proc_t proc, const on_NtSetTimer_fn& on_func)
{
    if(d_->observers_NtSetTimer.empty())
        if(!register_callback_with(*d_, proc, "NtSetTimer", &on_NtSetTimer))
            return false;

    d_->observers_NtSetTimer.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetTimerResolution(proc_t proc, const on_NtSetTimerResolution_fn& on_func)
{
    if(d_->observers_NtSetTimerResolution.empty())
        if(!register_callback_with(*d_, proc, "NtSetTimerResolution", &on_NtSetTimerResolution))
            return false;

    d_->observers_NtSetTimerResolution.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetUuidSeed(proc_t proc, const on_NtSetUuidSeed_fn& on_func)
{
    if(d_->observers_NtSetUuidSeed.empty())
        if(!register_callback_with(*d_, proc, "NtSetUuidSeed", &on_NtSetUuidSeed))
            return false;

    d_->observers_NtSetUuidSeed.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetValueKey(proc_t proc, const on_NtSetValueKey_fn& on_func)
{
    if(d_->observers_NtSetValueKey.empty())
        if(!register_callback_with(*d_, proc, "NtSetValueKey", &on_NtSetValueKey))
            return false;

    d_->observers_NtSetValueKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSetVolumeInformationFile(proc_t proc, const on_NtSetVolumeInformationFile_fn& on_func)
{
    if(d_->observers_NtSetVolumeInformationFile.empty())
        if(!register_callback_with(*d_, proc, "NtSetVolumeInformationFile", &on_NtSetVolumeInformationFile))
            return false;

    d_->observers_NtSetVolumeInformationFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtShutdownSystem(proc_t proc, const on_NtShutdownSystem_fn& on_func)
{
    if(d_->observers_NtShutdownSystem.empty())
        if(!register_callback_with(*d_, proc, "NtShutdownSystem", &on_NtShutdownSystem))
            return false;

    d_->observers_NtShutdownSystem.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtShutdownWorkerFactory(proc_t proc, const on_NtShutdownWorkerFactory_fn& on_func)
{
    if(d_->observers_NtShutdownWorkerFactory.empty())
        if(!register_callback_with(*d_, proc, "NtShutdownWorkerFactory", &on_NtShutdownWorkerFactory))
            return false;

    d_->observers_NtShutdownWorkerFactory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSignalAndWaitForSingleObject(proc_t proc, const on_NtSignalAndWaitForSingleObject_fn& on_func)
{
    if(d_->observers_NtSignalAndWaitForSingleObject.empty())
        if(!register_callback_with(*d_, proc, "NtSignalAndWaitForSingleObject", &on_NtSignalAndWaitForSingleObject))
            return false;

    d_->observers_NtSignalAndWaitForSingleObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSinglePhaseReject(proc_t proc, const on_NtSinglePhaseReject_fn& on_func)
{
    if(d_->observers_NtSinglePhaseReject.empty())
        if(!register_callback_with(*d_, proc, "NtSinglePhaseReject", &on_NtSinglePhaseReject))
            return false;

    d_->observers_NtSinglePhaseReject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtStartProfile(proc_t proc, const on_NtStartProfile_fn& on_func)
{
    if(d_->observers_NtStartProfile.empty())
        if(!register_callback_with(*d_, proc, "NtStartProfile", &on_NtStartProfile))
            return false;

    d_->observers_NtStartProfile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtStopProfile(proc_t proc, const on_NtStopProfile_fn& on_func)
{
    if(d_->observers_NtStopProfile.empty())
        if(!register_callback_with(*d_, proc, "NtStopProfile", &on_NtStopProfile))
            return false;

    d_->observers_NtStopProfile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSuspendProcess(proc_t proc, const on_NtSuspendProcess_fn& on_func)
{
    if(d_->observers_NtSuspendProcess.empty())
        if(!register_callback_with(*d_, proc, "NtSuspendProcess", &on_NtSuspendProcess))
            return false;

    d_->observers_NtSuspendProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSuspendThread(proc_t proc, const on_NtSuspendThread_fn& on_func)
{
    if(d_->observers_NtSuspendThread.empty())
        if(!register_callback_with(*d_, proc, "NtSuspendThread", &on_NtSuspendThread))
            return false;

    d_->observers_NtSuspendThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSystemDebugControl(proc_t proc, const on_NtSystemDebugControl_fn& on_func)
{
    if(d_->observers_NtSystemDebugControl.empty())
        if(!register_callback_with(*d_, proc, "NtSystemDebugControl", &on_NtSystemDebugControl))
            return false;

    d_->observers_NtSystemDebugControl.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtTerminateJobObject(proc_t proc, const on_NtTerminateJobObject_fn& on_func)
{
    if(d_->observers_NtTerminateJobObject.empty())
        if(!register_callback_with(*d_, proc, "NtTerminateJobObject", &on_NtTerminateJobObject))
            return false;

    d_->observers_NtTerminateJobObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtTerminateProcess(proc_t proc, const on_NtTerminateProcess_fn& on_func)
{
    if(d_->observers_NtTerminateProcess.empty())
        if(!register_callback_with(*d_, proc, "NtTerminateProcess", &on_NtTerminateProcess))
            return false;

    d_->observers_NtTerminateProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtTerminateThread(proc_t proc, const on_NtTerminateThread_fn& on_func)
{
    if(d_->observers_NtTerminateThread.empty())
        if(!register_callback_with(*d_, proc, "NtTerminateThread", &on_NtTerminateThread))
            return false;

    d_->observers_NtTerminateThread.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtTraceControl(proc_t proc, const on_NtTraceControl_fn& on_func)
{
    if(d_->observers_NtTraceControl.empty())
        if(!register_callback_with(*d_, proc, "NtTraceControl", &on_NtTraceControl))
            return false;

    d_->observers_NtTraceControl.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtTraceEvent(proc_t proc, const on_NtTraceEvent_fn& on_func)
{
    if(d_->observers_NtTraceEvent.empty())
        if(!register_callback_with(*d_, proc, "NtTraceEvent", &on_NtTraceEvent))
            return false;

    d_->observers_NtTraceEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtTranslateFilePath(proc_t proc, const on_NtTranslateFilePath_fn& on_func)
{
    if(d_->observers_NtTranslateFilePath.empty())
        if(!register_callback_with(*d_, proc, "NtTranslateFilePath", &on_NtTranslateFilePath))
            return false;

    d_->observers_NtTranslateFilePath.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtUnloadDriver(proc_t proc, const on_NtUnloadDriver_fn& on_func)
{
    if(d_->observers_NtUnloadDriver.empty())
        if(!register_callback_with(*d_, proc, "NtUnloadDriver", &on_NtUnloadDriver))
            return false;

    d_->observers_NtUnloadDriver.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtUnloadKey2(proc_t proc, const on_NtUnloadKey2_fn& on_func)
{
    if(d_->observers_NtUnloadKey2.empty())
        if(!register_callback_with(*d_, proc, "NtUnloadKey2", &on_NtUnloadKey2))
            return false;

    d_->observers_NtUnloadKey2.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtUnloadKeyEx(proc_t proc, const on_NtUnloadKeyEx_fn& on_func)
{
    if(d_->observers_NtUnloadKeyEx.empty())
        if(!register_callback_with(*d_, proc, "NtUnloadKeyEx", &on_NtUnloadKeyEx))
            return false;

    d_->observers_NtUnloadKeyEx.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtUnloadKey(proc_t proc, const on_NtUnloadKey_fn& on_func)
{
    if(d_->observers_NtUnloadKey.empty())
        if(!register_callback_with(*d_, proc, "NtUnloadKey", &on_NtUnloadKey))
            return false;

    d_->observers_NtUnloadKey.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtUnlockFile(proc_t proc, const on_NtUnlockFile_fn& on_func)
{
    if(d_->observers_NtUnlockFile.empty())
        if(!register_callback_with(*d_, proc, "NtUnlockFile", &on_NtUnlockFile))
            return false;

    d_->observers_NtUnlockFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtUnlockVirtualMemory(proc_t proc, const on_NtUnlockVirtualMemory_fn& on_func)
{
    if(d_->observers_NtUnlockVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "NtUnlockVirtualMemory", &on_NtUnlockVirtualMemory))
            return false;

    d_->observers_NtUnlockVirtualMemory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtUnmapViewOfSection(proc_t proc, const on_NtUnmapViewOfSection_fn& on_func)
{
    if(d_->observers_NtUnmapViewOfSection.empty())
        if(!register_callback_with(*d_, proc, "NtUnmapViewOfSection", &on_NtUnmapViewOfSection))
            return false;

    d_->observers_NtUnmapViewOfSection.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtVdmControl(proc_t proc, const on_NtVdmControl_fn& on_func)
{
    if(d_->observers_NtVdmControl.empty())
        if(!register_callback_with(*d_, proc, "NtVdmControl", &on_NtVdmControl))
            return false;

    d_->observers_NtVdmControl.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWaitForDebugEvent(proc_t proc, const on_NtWaitForDebugEvent_fn& on_func)
{
    if(d_->observers_NtWaitForDebugEvent.empty())
        if(!register_callback_with(*d_, proc, "NtWaitForDebugEvent", &on_NtWaitForDebugEvent))
            return false;

    d_->observers_NtWaitForDebugEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWaitForKeyedEvent(proc_t proc, const on_NtWaitForKeyedEvent_fn& on_func)
{
    if(d_->observers_NtWaitForKeyedEvent.empty())
        if(!register_callback_with(*d_, proc, "NtWaitForKeyedEvent", &on_NtWaitForKeyedEvent))
            return false;

    d_->observers_NtWaitForKeyedEvent.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWaitForMultipleObjects32(proc_t proc, const on_NtWaitForMultipleObjects32_fn& on_func)
{
    if(d_->observers_NtWaitForMultipleObjects32.empty())
        if(!register_callback_with(*d_, proc, "NtWaitForMultipleObjects32", &on_NtWaitForMultipleObjects32))
            return false;

    d_->observers_NtWaitForMultipleObjects32.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWaitForMultipleObjects(proc_t proc, const on_NtWaitForMultipleObjects_fn& on_func)
{
    if(d_->observers_NtWaitForMultipleObjects.empty())
        if(!register_callback_with(*d_, proc, "NtWaitForMultipleObjects", &on_NtWaitForMultipleObjects))
            return false;

    d_->observers_NtWaitForMultipleObjects.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWaitForSingleObject(proc_t proc, const on_NtWaitForSingleObject_fn& on_func)
{
    if(d_->observers_NtWaitForSingleObject.empty())
        if(!register_callback_with(*d_, proc, "NtWaitForSingleObject", &on_NtWaitForSingleObject))
            return false;

    d_->observers_NtWaitForSingleObject.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWaitForWorkViaWorkerFactory(proc_t proc, const on_NtWaitForWorkViaWorkerFactory_fn& on_func)
{
    if(d_->observers_NtWaitForWorkViaWorkerFactory.empty())
        if(!register_callback_with(*d_, proc, "NtWaitForWorkViaWorkerFactory", &on_NtWaitForWorkViaWorkerFactory))
            return false;

    d_->observers_NtWaitForWorkViaWorkerFactory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWaitHighEventPair(proc_t proc, const on_NtWaitHighEventPair_fn& on_func)
{
    if(d_->observers_NtWaitHighEventPair.empty())
        if(!register_callback_with(*d_, proc, "NtWaitHighEventPair", &on_NtWaitHighEventPair))
            return false;

    d_->observers_NtWaitHighEventPair.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWaitLowEventPair(proc_t proc, const on_NtWaitLowEventPair_fn& on_func)
{
    if(d_->observers_NtWaitLowEventPair.empty())
        if(!register_callback_with(*d_, proc, "NtWaitLowEventPair", &on_NtWaitLowEventPair))
            return false;

    d_->observers_NtWaitLowEventPair.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWorkerFactoryWorkerReady(proc_t proc, const on_NtWorkerFactoryWorkerReady_fn& on_func)
{
    if(d_->observers_NtWorkerFactoryWorkerReady.empty())
        if(!register_callback_with(*d_, proc, "NtWorkerFactoryWorkerReady", &on_NtWorkerFactoryWorkerReady))
            return false;

    d_->observers_NtWorkerFactoryWorkerReady.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWriteFileGather(proc_t proc, const on_NtWriteFileGather_fn& on_func)
{
    if(d_->observers_NtWriteFileGather.empty())
        if(!register_callback_with(*d_, proc, "NtWriteFileGather", &on_NtWriteFileGather))
            return false;

    d_->observers_NtWriteFileGather.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWriteFile(proc_t proc, const on_NtWriteFile_fn& on_func)
{
    if(d_->observers_NtWriteFile.empty())
        if(!register_callback_with(*d_, proc, "NtWriteFile", &on_NtWriteFile))
            return false;

    d_->observers_NtWriteFile.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWriteRequestData(proc_t proc, const on_NtWriteRequestData_fn& on_func)
{
    if(d_->observers_NtWriteRequestData.empty())
        if(!register_callback_with(*d_, proc, "NtWriteRequestData", &on_NtWriteRequestData))
            return false;

    d_->observers_NtWriteRequestData.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtWriteVirtualMemory(proc_t proc, const on_NtWriteVirtualMemory_fn& on_func)
{
    if(d_->observers_NtWriteVirtualMemory.empty())
        if(!register_callback_with(*d_, proc, "NtWriteVirtualMemory", &on_NtWriteVirtualMemory))
            return false;

    d_->observers_NtWriteVirtualMemory.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtDisableLastKnownGood(proc_t proc, const on_NtDisableLastKnownGood_fn& on_func)
{
    if(d_->observers_NtDisableLastKnownGood.empty())
        if(!register_callback_with(*d_, proc, "NtDisableLastKnownGood", &on_NtDisableLastKnownGood))
            return false;

    d_->observers_NtDisableLastKnownGood.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtEnableLastKnownGood(proc_t proc, const on_NtEnableLastKnownGood_fn& on_func)
{
    if(d_->observers_NtEnableLastKnownGood.empty())
        if(!register_callback_with(*d_, proc, "NtEnableLastKnownGood", &on_NtEnableLastKnownGood))
            return false;

    d_->observers_NtEnableLastKnownGood.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFlushProcessWriteBuffers(proc_t proc, const on_NtFlushProcessWriteBuffers_fn& on_func)
{
    if(d_->observers_NtFlushProcessWriteBuffers.empty())
        if(!register_callback_with(*d_, proc, "NtFlushProcessWriteBuffers", &on_NtFlushProcessWriteBuffers))
            return false;

    d_->observers_NtFlushProcessWriteBuffers.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtFlushWriteBuffer(proc_t proc, const on_NtFlushWriteBuffer_fn& on_func)
{
    if(d_->observers_NtFlushWriteBuffer.empty())
        if(!register_callback_with(*d_, proc, "NtFlushWriteBuffer", &on_NtFlushWriteBuffer))
            return false;

    d_->observers_NtFlushWriteBuffer.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtGetCurrentProcessorNumber(proc_t proc, const on_NtGetCurrentProcessorNumber_fn& on_func)
{
    if(d_->observers_NtGetCurrentProcessorNumber.empty())
        if(!register_callback_with(*d_, proc, "NtGetCurrentProcessorNumber", &on_NtGetCurrentProcessorNumber))
            return false;

    d_->observers_NtGetCurrentProcessorNumber.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtIsSystemResumeAutomatic(proc_t proc, const on_NtIsSystemResumeAutomatic_fn& on_func)
{
    if(d_->observers_NtIsSystemResumeAutomatic.empty())
        if(!register_callback_with(*d_, proc, "NtIsSystemResumeAutomatic", &on_NtIsSystemResumeAutomatic))
            return false;

    d_->observers_NtIsSystemResumeAutomatic.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtIsUILanguageComitted(proc_t proc, const on_NtIsUILanguageComitted_fn& on_func)
{
    if(d_->observers_NtIsUILanguageComitted.empty())
        if(!register_callback_with(*d_, proc, "NtIsUILanguageComitted", &on_NtIsUILanguageComitted))
            return false;

    d_->observers_NtIsUILanguageComitted.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtQueryPortInformationProcess(proc_t proc, const on_NtQueryPortInformationProcess_fn& on_func)
{
    if(d_->observers_NtQueryPortInformationProcess.empty())
        if(!register_callback_with(*d_, proc, "NtQueryPortInformationProcess", &on_NtQueryPortInformationProcess))
            return false;

    d_->observers_NtQueryPortInformationProcess.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtSerializeBoot(proc_t proc, const on_NtSerializeBoot_fn& on_func)
{
    if(d_->observers_NtSerializeBoot.empty())
        if(!register_callback_with(*d_, proc, "NtSerializeBoot", &on_NtSerializeBoot))
            return false;

    d_->observers_NtSerializeBoot.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtTestAlert(proc_t proc, const on_NtTestAlert_fn& on_func)
{
    if(d_->observers_NtTestAlert.empty())
        if(!register_callback_with(*d_, proc, "NtTestAlert", &on_NtTestAlert))
            return false;

    d_->observers_NtTestAlert.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtThawRegistry(proc_t proc, const on_NtThawRegistry_fn& on_func)
{
    if(d_->observers_NtThawRegistry.empty())
        if(!register_callback_with(*d_, proc, "NtThawRegistry", &on_NtThawRegistry))
            return false;

    d_->observers_NtThawRegistry.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtThawTransactions(proc_t proc, const on_NtThawTransactions_fn& on_func)
{
    if(d_->observers_NtThawTransactions.empty())
        if(!register_callback_with(*d_, proc, "NtThawTransactions", &on_NtThawTransactions))
            return false;

    d_->observers_NtThawTransactions.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtUmsThreadYield(proc_t proc, const on_NtUmsThreadYield_fn& on_func)
{
    if(d_->observers_NtUmsThreadYield.empty())
        if(!register_callback_with(*d_, proc, "NtUmsThreadYield", &on_NtUmsThreadYield))
            return false;

    d_->observers_NtUmsThreadYield.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_NtYieldExecution(proc_t proc, const on_NtYieldExecution_fn& on_func)
{
    if(d_->observers_NtYieldExecution.empty())
        if(!register_callback_with(*d_, proc, "NtYieldExecution", &on_NtYieldExecution))
            return false;

    d_->observers_NtYieldExecution.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_RtlpAllocateHeapInternal(proc_t proc, const on_RtlpAllocateHeapInternal_fn& on_func)
{
    if(d_->observers_RtlpAllocateHeapInternal.empty())
        if(!register_callback_with(*d_, proc, "RtlpAllocateHeapInternal", &on_RtlpAllocateHeapInternal))
            return false;

    d_->observers_RtlpAllocateHeapInternal.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_RtlFreeHeap(proc_t proc, const on_RtlFreeHeap_fn& on_func)
{
    if(d_->observers_RtlFreeHeap.empty())
        if(!register_callback_with(*d_, proc, "RtlFreeHeap", &on_RtlFreeHeap))
            return false;

    d_->observers_RtlFreeHeap.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_RtlpReAllocateHeapInternal(proc_t proc, const on_RtlpReAllocateHeapInternal_fn& on_func)
{
    if(d_->observers_RtlpReAllocateHeapInternal.empty())
        if(!register_callback_with(*d_, proc, "RtlpReAllocateHeapInternal", &on_RtlpReAllocateHeapInternal))
            return false;

    d_->observers_RtlpReAllocateHeapInternal.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_RtlSizeHeap(proc_t proc, const on_RtlSizeHeap_fn& on_func)
{
    if(d_->observers_RtlSizeHeap.empty())
        if(!register_callback_with(*d_, proc, "RtlSizeHeap", &on_RtlSizeHeap))
            return false;

    d_->observers_RtlSizeHeap.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_RtlSetUserValueHeap(proc_t proc, const on_RtlSetUserValueHeap_fn& on_func)
{
    if(d_->observers_RtlSetUserValueHeap.empty())
        if(!register_callback_with(*d_, proc, "RtlSetUserValueHeap", &on_RtlSetUserValueHeap))
            return false;

    d_->observers_RtlSetUserValueHeap.push_back(on_func);
    return true;
}

bool monitor::syscalls::register_RtlGetUserInfoHeap(proc_t proc, const on_RtlGetUserInfoHeap_fn& on_func)
{
    if(d_->observers_RtlGetUserInfoHeap.empty())
        if(!register_callback_with(*d_, proc, "RtlGetUserInfoHeap", &on_RtlGetUserInfoHeap))
            return false;

    d_->observers_RtlGetUserInfoHeap.push_back(on_func);
    return true;
}

namespace
{
    static const char g_names[][64] =
    {
      "NtAcceptConnectPort",
      "NtAccessCheckAndAuditAlarm",
      "NtAccessCheckByTypeAndAuditAlarm",
      "NtAccessCheckByType",
      "NtAccessCheckByTypeResultListAndAuditAlarmByHandle",
      "NtAccessCheckByTypeResultListAndAuditAlarm",
      "NtAccessCheckByTypeResultList",
      "NtAccessCheck",
      "NtAddAtom",
      "NtAddBootEntry",
      "NtAddDriverEntry",
      "NtAdjustGroupsToken",
      "NtAdjustPrivilegesToken",
      "NtAlertResumeThread",
      "NtAlertThread",
      "NtAllocateLocallyUniqueId",
      "NtAllocateReserveObject",
      "NtAllocateUserPhysicalPages",
      "NtAllocateUuids",
      "NtAllocateVirtualMemory",
      "NtAlpcAcceptConnectPort",
      "NtAlpcCancelMessage",
      "NtAlpcConnectPort",
      "NtAlpcCreatePort",
      "NtAlpcCreatePortSection",
      "NtAlpcCreateResourceReserve",
      "NtAlpcCreateSectionView",
      "NtAlpcCreateSecurityContext",
      "NtAlpcDeletePortSection",
      "NtAlpcDeleteResourceReserve",
      "NtAlpcDeleteSectionView",
      "NtAlpcDeleteSecurityContext",
      "NtAlpcDisconnectPort",
      "NtAlpcImpersonateClientOfPort",
      "NtAlpcOpenSenderProcess",
      "NtAlpcOpenSenderThread",
      "NtAlpcQueryInformation",
      "NtAlpcQueryInformationMessage",
      "NtAlpcRevokeSecurityContext",
      "NtAlpcSendWaitReceivePort",
      "NtAlpcSetInformation",
      "NtApphelpCacheControl",
      "NtAreMappedFilesTheSame",
      "NtAssignProcessToJobObject",
      "NtCallbackReturn",
      "NtCancelIoFileEx",
      "NtCancelIoFile",
      "NtCancelSynchronousIoFile",
      "NtCancelTimer",
      "NtClearEvent",
      "NtClose",
      "NtCloseObjectAuditAlarm",
      "NtCommitComplete",
      "NtCommitEnlistment",
      "NtCommitTransaction",
      "NtCompactKeys",
      "NtCompareTokens",
      "NtCompleteConnectPort",
      "NtCompressKey",
      "NtConnectPort",
      "NtContinue",
      "NtCreateDebugObject",
      "NtCreateDirectoryObject",
      "NtCreateEnlistment",
      "NtCreateEvent",
      "NtCreateEventPair",
      "NtCreateFile",
      "NtCreateIoCompletion",
      "NtCreateJobObject",
      "NtCreateJobSet",
      "NtCreateKeyedEvent",
      "NtCreateKey",
      "NtCreateKeyTransacted",
      "NtCreateMailslotFile",
      "NtCreateMutant",
      "NtCreateNamedPipeFile",
      "NtCreatePagingFile",
      "NtCreatePort",
      "NtCreatePrivateNamespace",
      "NtCreateProcessEx",
      "NtCreateProcess",
      "NtCreateProfileEx",
      "NtCreateProfile",
      "NtCreateResourceManager",
      "NtCreateSection",
      "NtCreateSemaphore",
      "NtCreateSymbolicLinkObject",
      "NtCreateThreadEx",
      "NtCreateThread",
      "NtCreateTimer",
      "NtCreateToken",
      "NtCreateTransactionManager",
      "NtCreateTransaction",
      "NtCreateUserProcess",
      "NtCreateWaitablePort",
      "NtCreateWorkerFactory",
      "NtDebugActiveProcess",
      "NtDebugContinue",
      "NtDelayExecution",
      "NtDeleteAtom",
      "NtDeleteBootEntry",
      "NtDeleteDriverEntry",
      "NtDeleteFile",
      "NtDeleteKey",
      "NtDeleteObjectAuditAlarm",
      "NtDeletePrivateNamespace",
      "NtDeleteValueKey",
      "NtDeviceIoControlFile",
      "NtDisplayString",
      "NtDrawText",
      "NtDuplicateObject",
      "NtDuplicateToken",
      "NtEnumerateBootEntries",
      "NtEnumerateDriverEntries",
      "NtEnumerateKey",
      "NtEnumerateSystemEnvironmentValuesEx",
      "NtEnumerateTransactionObject",
      "NtEnumerateValueKey",
      "NtExtendSection",
      "NtFilterToken",
      "NtFindAtom",
      "NtFlushBuffersFile",
      "NtFlushInstallUILanguage",
      "NtFlushInstructionCache",
      "NtFlushKey",
      "NtFlushVirtualMemory",
      "NtFreeUserPhysicalPages",
      "NtFreeVirtualMemory",
      "NtFreezeRegistry",
      "NtFreezeTransactions",
      "NtFsControlFile",
      "NtGetContextThread",
      "NtGetDevicePowerState",
      "NtGetMUIRegistryInfo",
      "NtGetNextProcess",
      "NtGetNextThread",
      "NtGetNlsSectionPtr",
      "NtGetNotificationResourceManager",
      "NtGetPlugPlayEvent",
      "NtGetWriteWatch",
      "NtImpersonateAnonymousToken",
      "NtImpersonateClientOfPort",
      "NtImpersonateThread",
      "NtInitializeNlsFiles",
      "NtInitializeRegistry",
      "NtInitiatePowerAction",
      "NtIsProcessInJob",
      "NtListenPort",
      "NtLoadDriver",
      "NtLoadKey2",
      "NtLoadKeyEx",
      "NtLoadKey",
      "NtLockFile",
      "NtLockProductActivationKeys",
      "NtLockRegistryKey",
      "NtLockVirtualMemory",
      "NtMakePermanentObject",
      "NtMakeTemporaryObject",
      "NtMapCMFModule",
      "NtMapUserPhysicalPages",
      "NtMapUserPhysicalPagesScatter",
      "NtMapViewOfSection",
      "NtModifyBootEntry",
      "NtModifyDriverEntry",
      "NtNotifyChangeDirectoryFile",
      "NtNotifyChangeKey",
      "NtNotifyChangeMultipleKeys",
      "NtNotifyChangeSession",
      "NtOpenDirectoryObject",
      "NtOpenEnlistment",
      "NtOpenEvent",
      "NtOpenEventPair",
      "NtOpenFile",
      "NtOpenIoCompletion",
      "NtOpenJobObject",
      "NtOpenKeyedEvent",
      "NtOpenKeyEx",
      "NtOpenKey",
      "NtOpenKeyTransactedEx",
      "NtOpenKeyTransacted",
      "NtOpenMutant",
      "NtOpenObjectAuditAlarm",
      "NtOpenPrivateNamespace",
      "NtOpenProcess",
      "NtOpenProcessTokenEx",
      "NtOpenProcessToken",
      "NtOpenResourceManager",
      "NtOpenSection",
      "NtOpenSemaphore",
      "NtOpenSession",
      "NtOpenSymbolicLinkObject",
      "NtOpenThread",
      "NtOpenThreadTokenEx",
      "NtOpenThreadToken",
      "NtOpenTimer",
      "NtOpenTransactionManager",
      "NtOpenTransaction",
      "NtPlugPlayControl",
      "NtPowerInformation",
      "NtPrepareComplete",
      "NtPrepareEnlistment",
      "NtPrePrepareComplete",
      "NtPrePrepareEnlistment",
      "NtPrivilegeCheck",
      "NtPrivilegedServiceAuditAlarm",
      "NtPrivilegeObjectAuditAlarm",
      "NtPropagationComplete",
      "NtPropagationFailed",
      "NtProtectVirtualMemory",
      "NtPulseEvent",
      "NtQueryAttributesFile",
      "NtQueryBootEntryOrder",
      "NtQueryBootOptions",
      "NtQueryDebugFilterState",
      "NtQueryDefaultLocale",
      "NtQueryDefaultUILanguage",
      "NtQueryDirectoryFile",
      "NtQueryDirectoryObject",
      "NtQueryDriverEntryOrder",
      "NtQueryEaFile",
      "NtQueryEvent",
      "NtQueryFullAttributesFile",
      "NtQueryInformationAtom",
      "NtQueryInformationEnlistment",
      "NtQueryInformationFile",
      "NtQueryInformationJobObject",
      "NtQueryInformationPort",
      "NtQueryInformationProcess",
      "NtQueryInformationResourceManager",
      "NtQueryInformationThread",
      "NtQueryInformationToken",
      "NtQueryInformationTransaction",
      "NtQueryInformationTransactionManager",
      "NtQueryInformationWorkerFactory",
      "NtQueryInstallUILanguage",
      "NtQueryIntervalProfile",
      "NtQueryIoCompletion",
      "NtQueryKey",
      "NtQueryLicenseValue",
      "NtQueryMultipleValueKey",
      "NtQueryMutant",
      "NtQueryObject",
      "NtQueryOpenSubKeysEx",
      "NtQueryOpenSubKeys",
      "NtQueryPerformanceCounter",
      "NtQueryQuotaInformationFile",
      "NtQuerySection",
      "NtQuerySecurityAttributesToken",
      "NtQuerySecurityObject",
      "NtQuerySemaphore",
      "NtQuerySymbolicLinkObject",
      "NtQuerySystemEnvironmentValueEx",
      "NtQuerySystemEnvironmentValue",
      "NtQuerySystemInformationEx",
      "NtQuerySystemInformation",
      "NtQuerySystemTime",
      "NtQueryTimer",
      "NtQueryTimerResolution",
      "NtQueryValueKey",
      "NtQueryVirtualMemory",
      "NtQueryVolumeInformationFile",
      "NtQueueApcThreadEx",
      "NtQueueApcThread",
      "NtRaiseException",
      "NtRaiseHardError",
      "NtReadFile",
      "NtReadFileScatter",
      "NtReadOnlyEnlistment",
      "NtReadRequestData",
      "NtReadVirtualMemory",
      "NtRecoverEnlistment",
      "NtRecoverResourceManager",
      "NtRecoverTransactionManager",
      "NtRegisterProtocolAddressInformation",
      "NtRegisterThreadTerminatePort",
      "NtReleaseKeyedEvent",
      "NtReleaseMutant",
      "NtReleaseSemaphore",
      "NtReleaseWorkerFactoryWorker",
      "NtRemoveIoCompletionEx",
      "NtRemoveIoCompletion",
      "NtRemoveProcessDebug",
      "NtRenameKey",
      "NtRenameTransactionManager",
      "NtReplaceKey",
      "NtReplacePartitionUnit",
      "NtReplyPort",
      "NtReplyWaitReceivePortEx",
      "NtReplyWaitReceivePort",
      "NtReplyWaitReplyPort",
      "NtRequestPort",
      "NtRequestWaitReplyPort",
      "NtResetEvent",
      "NtResetWriteWatch",
      "NtRestoreKey",
      "NtResumeProcess",
      "NtResumeThread",
      "NtRollbackComplete",
      "NtRollbackEnlistment",
      "NtRollbackTransaction",
      "NtRollforwardTransactionManager",
      "NtSaveKeyEx",
      "NtSaveKey",
      "NtSaveMergedKeys",
      "NtSecureConnectPort",
      "NtSetBootEntryOrder",
      "NtSetBootOptions",
      "NtSetContextThread",
      "NtSetDebugFilterState",
      "NtSetDefaultHardErrorPort",
      "NtSetDefaultLocale",
      "NtSetDefaultUILanguage",
      "NtSetDriverEntryOrder",
      "NtSetEaFile",
      "NtSetEventBoostPriority",
      "NtSetEvent",
      "NtSetHighEventPair",
      "NtSetHighWaitLowEventPair",
      "NtSetInformationDebugObject",
      "NtSetInformationEnlistment",
      "NtSetInformationFile",
      "NtSetInformationJobObject",
      "NtSetInformationKey",
      "NtSetInformationObject",
      "NtSetInformationProcess",
      "NtSetInformationResourceManager",
      "NtSetInformationThread",
      "NtSetInformationToken",
      "NtSetInformationTransaction",
      "NtSetInformationTransactionManager",
      "NtSetInformationWorkerFactory",
      "NtSetIntervalProfile",
      "NtSetIoCompletionEx",
      "NtSetIoCompletion",
      "NtSetLdtEntries",
      "NtSetLowEventPair",
      "NtSetLowWaitHighEventPair",
      "NtSetQuotaInformationFile",
      "NtSetSecurityObject",
      "NtSetSystemEnvironmentValueEx",
      "NtSetSystemEnvironmentValue",
      "NtSetSystemInformation",
      "NtSetSystemPowerState",
      "NtSetSystemTime",
      "NtSetThreadExecutionState",
      "NtSetTimerEx",
      "NtSetTimer",
      "NtSetTimerResolution",
      "NtSetUuidSeed",
      "NtSetValueKey",
      "NtSetVolumeInformationFile",
      "NtShutdownSystem",
      "NtShutdownWorkerFactory",
      "NtSignalAndWaitForSingleObject",
      "NtSinglePhaseReject",
      "NtStartProfile",
      "NtStopProfile",
      "NtSuspendProcess",
      "NtSuspendThread",
      "NtSystemDebugControl",
      "NtTerminateJobObject",
      "NtTerminateProcess",
      "NtTerminateThread",
      "NtTraceControl",
      "NtTraceEvent",
      "NtTranslateFilePath",
      "NtUnloadDriver",
      "NtUnloadKey2",
      "NtUnloadKeyEx",
      "NtUnloadKey",
      "NtUnlockFile",
      "NtUnlockVirtualMemory",
      "NtUnmapViewOfSection",
      "NtVdmControl",
      "NtWaitForDebugEvent",
      "NtWaitForKeyedEvent",
      "NtWaitForMultipleObjects32",
      "NtWaitForMultipleObjects",
      "NtWaitForSingleObject",
      "NtWaitForWorkViaWorkerFactory",
      "NtWaitHighEventPair",
      "NtWaitLowEventPair",
      "NtWorkerFactoryWorkerReady",
      "NtWriteFileGather",
      "NtWriteFile",
      "NtWriteRequestData",
      "NtWriteVirtualMemory",
      "NtDisableLastKnownGood",
      "NtEnableLastKnownGood",
      "NtFlushProcessWriteBuffers",
      "NtFlushWriteBuffer",
      "NtGetCurrentProcessorNumber",
      "NtIsSystemResumeAutomatic",
      "NtIsUILanguageComitted",
      "NtQueryPortInformationProcess",
      "NtSerializeBoot",
      "NtTestAlert",
      "NtThawRegistry",
      "NtThawTransactions",
      "NtUmsThreadYield",
      "NtYieldExecution",
      "RtlpAllocateHeapInternal",
      "RtlFreeHeap",
      "RtlpReAllocateHeapInternal",
      "RtlSizeHeap",
      "RtlSetUserValueHeap",
      "RtlGetUserInfoHeap",
    };
}

bool monitor::syscalls::register_all(proc_t proc, const monitor::syscalls::on_call_fn& on_call)
{
    Data::Breakpoints breakpoints;
    for(const auto it : g_names)
    {
        const auto bp = register_callback(*d_, proc, it, on_call);
        if(!bp)
            return false;

        breakpoints.emplace_back(bp);
    }

    d_->breakpoints.insert(d_->breakpoints.end(), breakpoints.begin(), breakpoints.end());
    return true;
}
