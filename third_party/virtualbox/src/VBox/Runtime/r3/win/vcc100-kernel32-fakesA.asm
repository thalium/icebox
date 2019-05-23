

%include "iprt/asmdefs.mac"

%ifndef RT_ARCH_X86
 %error "This is x86 only code.
%endif


%macro MAKE_IMPORT_ENTRY 2
extern _ %+ %1 %+ @ %+ %2
global __imp__ %+ %1 %+ @ %+ %2
__imp__ %+ %1 %+ @ %+ %2:
    dd _ %+ %1 %+ @ %+ %2

%endmacro


BEGINDATA
GLOBALNAME vcc100_kernel32_fakes_asm

MAKE_IMPORT_ENTRY DecodePointer, 4
MAKE_IMPORT_ENTRY EncodePointer, 4
MAKE_IMPORT_ENTRY InitializeCriticalSectionAndSpinCount, 8
MAKE_IMPORT_ENTRY HeapSetInformation, 16
MAKE_IMPORT_ENTRY HeapQueryInformation, 20
MAKE_IMPORT_ENTRY CreateTimerQueue, 0
MAKE_IMPORT_ENTRY CreateTimerQueueTimer, 28
MAKE_IMPORT_ENTRY DeleteTimerQueueTimer, 12
MAKE_IMPORT_ENTRY InitializeSListHead, 4
MAKE_IMPORT_ENTRY InterlockedFlushSList, 4
MAKE_IMPORT_ENTRY InterlockedPopEntrySList, 4
MAKE_IMPORT_ENTRY InterlockedPushEntrySList, 8
MAKE_IMPORT_ENTRY QueryDepthSList, 4
MAKE_IMPORT_ENTRY VerifyVersionInfoA, 16
MAKE_IMPORT_ENTRY VerSetConditionMask, 16



