; $Id: VBoxGuestAdditionsExternal.nsh $
;; @file
; VBoxGuestAdditionExternal.nsh - Utility function for invoking external
;                                 applications.
;

;
; Copyright (C) 2013 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

;
; Macro for executing external applications. Uses the nsExec plugin
; in different styles, depending on whether this installer runs in silent mode
; or not. If the external program reports an exit code other than 0 the installer
; will be aborted.
;
; @param   Command line (full qualified and quoted).
; @param   If set to "true" the installer aborts if the external program reports
;          an exit code other than 0, "false" just prints a warning and continues
;          execution.
;
!macro _cmdExecute cmdline optional

  Push $0
  Push $1

  !define _macroLoc ${__LINE__}

  ${LogVerbose} "Executing: ${cmdline}"
  IfSilent silent_${_macroLoc} +1
    nsExec::ExecToLog "${cmdline}"
    Pop $0 ; Return value (exit code)
    goto done_${_macroLoc}

silent_${_macroLoc}:

  nsExec::ExecToStack "${cmdline}"
  Pop $0 ; Return value (exit code)
  Pop $1 ; Stdout/stderr output (up to ${NSIS_MAX_STRLEN})
  ${LogVerbose} "$1"
  goto done_${_macroLoc}

done_${_macroLoc}:

  ${LogVerbose} "Execution returned exit code: $0"
  IntCmp $0 0 +1 error_${_macroLoc} error_${_macroLoc} ; Check ret value (0=OK, 1=Error)
  goto return_${_macroLoc}

error_${_macroLoc}:

  ${If} ${optional} == "false"
    ${LogVerbose} "Error excuting $\"${cmdline}$\" (exit code: $0) -- aborting installation"
    Abort "Error excuting $\"${cmdline}$\" (exit code: $0) -- aborting installation"
  ${Else}
    ${LogVerbose} "Warning: Executing $\"${cmdline}$\" returned with exit code $0"
  ${EndIf}
  goto return_${_macroLoc}

return_${_macroLoc}:

  Pop $1
  Pop $0

  !undef _macroLoc

!macroend
!define CmdExecute "!insertmacro _cmdExecute"
