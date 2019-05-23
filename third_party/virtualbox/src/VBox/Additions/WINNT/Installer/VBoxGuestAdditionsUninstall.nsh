; $Id$
; @file
; VBoxGuestAdditionsUninstall.nsh - Guest Additions uninstallation.
;

;
; Copyright (C) 2006-2013 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

!macro UninstallCommon un
Function ${un}UninstallCommon

  Delete /REBOOTOK "$INSTDIR\install*.log"
  Delete /REBOOTOK "$INSTDIR\uninst.exe"
  Delete /REBOOTOK "$INSTDIR\${PRODUCT_NAME}.url"

  ; Remove common files
  Delete /REBOOTOK "$INSTDIR\VBoxDrvInst.exe"
  Delete /REBOOTOK "$INSTDIR\DIFxAPI.dll"

  Delete /REBOOTOK "$INSTDIR\VBoxVideo.inf"
!ifdef VBOX_SIGN_ADDITIONS
  Delete /REBOOTOK "$INSTDIR\VBoxVideo.cat"
!endif

!if $%VBOX_WITH_LICENSE_INSTALL_RTF% == "1"
  Delete /REBOOTOK "$INSTDIR\${LICENSE_FILE_RTF}"
!endif

  Delete /REBOOTOK "$INSTDIR\VBoxGINA.dll"
  Delete /REBOOTOK "$INSTDIR\iexplore.ico"

  ; Delete registry keys
  DeleteRegKey /ifempty HKLM "${PRODUCT_INSTALL_KEY}"
  DeleteRegKey /ifempty HKLM "${VENDOR_ROOT_KEY}"

  ; Delete desktop & start menu entries
  Delete "$DESKTOP\${PRODUCT_NAME} Guest Additions.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME} Guest Additions\Uninstall.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME} Guest Additions\Website.lnk"
  RMDIR "$SMPROGRAMS\${PRODUCT_NAME} Guest Additions"

  ; Delete Guest Additions directory (only if completely empty)
  RMDir /REBOOTOK "$INSTDIR"

  ; Delete vendor installation directory (only if completely empty)
!if $%BUILD_TARGET_ARCH% == "x86"       ; 32-bit
  RMDir /REBOOTOK "$PROGRAMFILES32\$%VBOX_VENDOR_SHORT%"
!else   ; 64-bit
  RMDir /REBOOTOK "$PROGRAMFILES64\$%VBOX_VENDOR_SHORT%"
!endif

  ; Remove registry entries
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"

FunctionEnd
!macroend
!insertmacro UninstallCommon ""
!insertmacro UninstallCommon "un."

!macro Uninstall un
Function ${un}Uninstall

  ${LogVerbose} "Uninstalling system files ..."
!ifdef _DEBUG
  ${LogVerbose} "Detected OS version: Windows $g_strWinVersion"
  ${LogVerbose} "System Directory: $g_strSystemDir"
!endif

  ; Which OS are we using?
!if $%BUILD_TARGET_ARCH% == "x86"       ; 32-bit
  StrCmp $g_strWinVersion "NT4" nt4     ; Windows NT 4.0
!endif
  StrCmp $g_strWinVersion "2000" w2k    ; Windows 2000
  StrCmp $g_strWinVersion "XP" w2k      ; Windows XP
  StrCmp $g_strWinVersion "2003" w2k    ; Windows 2003 Server
  StrCmp $g_strWinVersion "Vista" vista ; Windows Vista
  StrCmp $g_strWinVersion "7" vista     ; Windows 7
  StrCmp $g_strWinVersion "8" vista     ; Windows 8
  StrCmp $g_strWinVersion "8_1" vista   ; Windows 8.1 / Windows Server 2012 R2
  StrCmp $g_strWinVersion "10" vista    ; Windows 10

  ${If} $g_bForceInstall == "true"
    Goto vista ; Assume newer OS than we know of ...
  ${EndIf}

  Goto notsupported

!if $%BUILD_TARGET_ARCH% == "x86"       ; 32-bit
nt4:

  Call ${un}NT4_Uninstall
  goto common
!endif

w2k:

  Call ${un}W2K_Uninstall
  goto common

vista:

  Call ${un}W2K_Uninstall
  Call ${un}Vista_Uninstall
  goto common

notsupported:

  MessageBox MB_ICONSTOP $(VBOX_PLATFORM_UNSUPPORTED) /SD IDOK
  Goto exit

common:

exit:

FunctionEnd
!macroend
!insertmacro Uninstall ""
!insertmacro Uninstall "un."

!macro UninstallInstDir un
Function ${un}UninstallInstDir

  ${LogVerbose} "Uninstalling directory ..."
!ifdef _DEBUG
  ${LogVerbose} "Detected OS version: Windows $g_strWinVersion"
  ${LogVerbose} "System Directory: $g_strSystemDir"
!endif

  ; Which OS are we using?
!if $%BUILD_TARGET_ARCH% == "x86"       ; 32-bit
  StrCmp $g_strWinVersion "NT4" nt4     ; Windows NT 4.0
!endif
  StrCmp $g_strWinVersion "2000" w2k    ; Windows 2000
  StrCmp $g_strWinVersion "XP" w2k      ; Windows XP
  StrCmp $g_strWinVersion "2003" w2k    ; Windows 2003 Server
  StrCmp $g_strWinVersion "Vista" vista ; Windows Vista
  StrCmp $g_strWinVersion "7" vista     ; Windows 7
  StrCmp $g_strWinVersion "8" vista     ; Windows 8
  StrCmp $g_strWinVersion "8_1" vista   ; Windows 8.1 / Windows Server 2012 R2
  StrCmp $g_strWinVersion "10" vista    ; Windows 10

  ${If} $g_bForceInstall == "true"
    Goto vista ; Assume newer OS than we know of ...
  ${EndIf}

  Goto notsupported

!if $%BUILD_TARGET_ARCH% == "x86"       ; 32-bit
nt4:

  Call ${un}NT4_UninstallInstDir
  goto common
!endif

w2k:

  Call ${un}W2K_UninstallInstDir
  goto common

vista:

  Call ${un}W2K_UninstallInstDir
  Call ${un}Vista_UninstallInstDir
  goto common

notsupported:

  MessageBox MB_ICONSTOP $(VBOX_PLATFORM_UNSUPPORTED) /SD IDOK
  Goto exit

common:

  Call ${un}UninstallCommon

exit:

FunctionEnd
!macroend
!insertmacro UninstallInstDir ""
!insertmacro UninstallInstDir "un."
