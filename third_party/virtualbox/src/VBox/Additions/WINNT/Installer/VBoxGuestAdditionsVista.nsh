; $Id$
; @file
; VBoxGuestAdditionsVista.nsh - Guest Additions installation for Windows Vista/7.
;

;
; Copyright (C) 2006-2012 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

Function Vista_CheckForRequirements

  Push $0

  ${LogVerbose} "Checking for installation requirements for Vista / Windows 7 / Windows 8 ..."

  ${If} $g_bForceInstall == "true"
    ${LogVerbose} "Forcing installation, checking requirements skipped"
    goto success
  ${EndIf}

  ; Validate D3D files, regardless whether D3D support is selected or not
  Call ValidateD3DFiles
  Pop $0
  ${If} $0 == "1" ; D3D files are invalid, notify user
    MessageBox MB_ICONSTOP|MB_OKCANCEL $(VBOX_COMPONENT_D3D_INVALID) /SD IDOK IDCANCEL failure
    ; Offer to open up the VBox online manual on how to fix missing/corrupted D3D files
    MessageBox MB_ICONQUESTION|MB_YESNO $(VBOX_COMPONENT_D3D_INVALID_MANUAL) /SD IDNO IDYES open_handbook_d3d_invalid    
  ${EndIf}
  Goto success

open_handbook_d3d_invalid:

  ; @todo Add a language GET parameter (e.g. ?lang=enUS) here as soon as we got the
  ;       handbook online in different languages
  ; Don't use https here (even if we offer it) -- we only want to display the handbook
  Call SetAppMode64 ; For shell execution we need to switch to 64-bit mode first
  ExecShell open "http://www.virtualbox.org/manual/ch12.html#ts_d3d8-d3d9-restore"
  IfErrors 0 +2
    MessageBox MB_ICONSTOP|MB_OK $(VBOX_ERROR_OPEN_LINK) /SD IDOK
  Call SetAppMode32
  Goto failure

failure:

  Abort "ERROR: Requirements not met! Installation aborted."
  goto exit

success:

  ; Nothing to do here right now
  Goto exit

exit:

  Pop $0

FunctionEnd

Function Vista_Prepare

  Call VBoxMMR_Uninstall

FunctionEnd

Function Vista_CopyFiles

  SetOutPath "$INSTDIR"
  SetOverwrite on

  ; The files are for Vista only, they go into the application directory

  ; VBoxNET drivers are not tested yet - commented out until officially supported and released
  ;FILE "$%PATH_OUT%\bin\additions\VBoxNET.inf"
  ;FILE "$%PATH_OUT%\bin\additions\VBoxNET.sys"

FunctionEnd

Function Vista_InstallFiles

  ${LogVerbose} "Installing drivers for Vista / Windows 7 / Windows 8 ..."

  SetOutPath "$INSTDIR"
  ; Nothing here yet

  Goto done

error:

  Abort "ERROR: Could not install files! Installation aborted."

done:

FunctionEnd

Function Vista_Main

  Call Vista_Prepare
  Call Vista_CopyFiles
  Call Vista_InstallFiles

FunctionEnd

!macro Vista_UninstallInstDir un
Function ${un}Vista_UninstallInstDir

!if $%BUILD_TARGET_ARCH% == "x86"       ; 32-bit
  Delete /REBOOTOK "$INSTDIR\netamd.inf"
  Delete /REBOOTOK "$INSTDIR\pcntpci5.cat"
  Delete /REBOOTOK "$INSTDIR\PCNTPCI5.sys"
!endif

FunctionEnd
!macroend
!insertmacro Vista_UninstallInstDir ""
!insertmacro Vista_UninstallInstDir "un."

!macro Vista_Uninstall un
Function ${un}Vista_Uninstall

   ; Remove credential provider
   ${LogVerbose} "Removing auto-logon support ..."
   DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{275D3BCC-22BB-4948-A7F6-3A3054EBA92B}"
   DeleteRegKey HKCR "CLSID\{275D3BCC-22BB-4948-A7F6-3A3054EBA92B}"
   Delete /REBOOTOK "$g_strSystemDir\VBoxCredProv.dll"

   Call ${un}VBoxMMR_Uninstall

FunctionEnd
!macroend
!insertmacro Vista_Uninstall ""
!insertmacro Vista_Uninstall "un."

!macro VBoxMMR_Uninstall un
Function ${un}VBoxMMR_Uninstall

  ; Remove VBoxMMR always

  DetailPrint "Uninstalling VBoxMMR."
  Call ${un}StopVBoxMMR

  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "VBoxMMR"

  Delete /REBOOTOK "$g_strSystemDir\VBoxMMR.exe"

  !if $%BUILD_TARGET_ARCH% == "amd64"
    Delete /REBOOTOK "$g_strSysWow64\VBoxMMRHook.dll"
    Delete /REBOOTOK "$INSTDIR\VBoxMMR-x86.exe"
    Delete /REBOOTOK "$INSTDIR\VBoxMMRHook-x86.dll"
  !else
    Delete /REBOOTOK "$g_strSystemDir\VBoxMMRHook.dll"
    Delete /REBOOTOK "$INSTDIR\VBoxMMR.exe"
    Delete /REBOOTOK "$INSTDIR\VBoxMMRHook.dll"
  !endif

FunctionEnd
!macroend
!insertmacro VBoxMMR_Uninstall ""
!insertmacro VBoxMMR_Uninstall "un."
