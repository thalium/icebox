
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
GLOBALNAME vcc100_ntdll_fakes_asm

MAKE_IMPORT_ENTRY RtlGetLastWin32Error, 0

