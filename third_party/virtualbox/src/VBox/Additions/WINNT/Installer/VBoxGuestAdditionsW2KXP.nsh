; $Id$
;; @file
; VBoxGuestAdditionsW2KXP.nsh - Guest Additions installation for Windows 2000/XP.
;

;
; Copyright (C) 2006-2016 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

Function W2K_SetVideoResolution

  ; NSIS only supports global vars, even in functions -- great
  Var /GLOBAL i
  Var /GLOBAL tmp
  Var /GLOBAL tmppath
  Var /GLOBAL dev_id
  Var /GLOBAL dev_desc

  ; Check for all required parameters
  StrCmp $g_iScreenX "0" exit
  StrCmp $g_iScreenY "0" exit
  StrCmp $g_iScreenBpp "0" exit

  ${LogVerbose} "Setting display parameters ($g_iScreenXx$g_iScreenY, $g_iScreenBpp BPP) ..."

  ; Enumerate all video devices (up to 32 at the moment, use key "MaxObjectNumber" key later)
  ${For} $i 0 32

    ReadRegStr $tmp HKLM "HARDWARE\DEVICEMAP\VIDEO" "\Device\Video$i"
    StrCmp $tmp "" dev_not_found

    ; Extract path to video settings
    ; Ex: \Registry\Machine\System\CurrentControlSet\Control\Video\{28B74D2B-F0A9-48E0-8028-D76F6BB1AE65}\0000
    ; Or: \Registry\Machine\System\CurrentControlSet\Control\Video\vboxvideo\Device0
    ; Result: Machine\System\CurrentControlSet\Control\Video\{28B74D2B-F0A9-48E0-8028-D76F6BB1AE65}\0000
    Push "$tmp" ; String
    Push "\" ; SubString
    Push ">" ; SearchDirection
    Push ">" ; StrInclusionDirection
    Push "0" ; IncludeSubString
    Push "2" ; Loops
    Push "0" ; CaseSensitive
    Call StrStrAdv
    Pop $tmppath ; $1 only contains the full path
    StrCmp $tmppath "" dev_not_found

    ; Get device description
    ReadRegStr $dev_desc HKLM "$tmppath" "Device Description"
!ifdef _DEBUG
    ${LogVerbose} "Registry path: $tmppath"
    ${LogVerbose} "Registry path to device name: $temp"
!endif
    ${LogVerbose} "Detected video device: $dev_desc"

    ${If} $dev_desc == "VirtualBox Graphics Adapter"
      ${LogVerbose} "VirtualBox video device found!"
      Goto dev_found
    ${EndIf}
  ${Next}
  Goto dev_not_found

dev_found:

  ; If we're on Windows 2000, skip the ID detection ...
  ${If} $g_strWinVersion == "2000"
    Goto change_res
  ${EndIf}
  Goto dev_found_detect_id

dev_found_detect_id:

  StrCpy $i 0 ; Start at index 0
  ${LogVerbose} "Detecting device ID ..."

dev_found_detect_id_loop:

  ; Resolve real path to hardware instance "{GUID}"
  EnumRegKey $dev_id HKLM "SYSTEM\CurrentControlSet\Control\Video" $i
  StrCmp $dev_id "" dev_not_found ; No more entries? Jump out
!ifdef _DEBUG
  ${LogVerbose} "Got device ID: $dev_id"
!endif
  ReadRegStr $dev_desc HKLM "SYSTEM\CurrentControlSet\Control\Video\$dev_id\0000" "Device Description" ; Try to read device name
  ${If} $dev_desc == "VirtualBox Graphics Adapter"
    ${LogVerbose} "Device ID of $dev_desc: $dev_id"
    Goto change_res
  ${EndIf}

  IntOp $i $i + 1 ; Increment index
  goto dev_found_detect_id_loop

dev_not_found:

  ${LogVerbose} "No VirtualBox video device (yet) detected! No custom mode set."
  Goto exit

change_res:

!ifdef _DEBUG
  ${LogVerbose} "Device description: $dev_desc"
  ${LogVerbose} "Device ID: $dev_id"
!endif

  Var /GLOBAL reg_path_device
  Var /GLOBAL reg_path_monitor

  ${LogVerbose} "Custom mode set: Platform is Windows $g_strWinVersion"
  ${If} $g_strWinVersion == "2000"
  ${OrIf} $g_strWinVersion == "Vista"
    StrCpy $reg_path_device "SYSTEM\CurrentControlSet\SERVICES\VBoxVideo\Device0"
    StrCpy $reg_path_monitor "SYSTEM\CurrentControlSet\SERVICES\VBoxVideo\Device0\Mon00000001"
  ${ElseIf} $g_strWinVersion == "XP"
  ${OrIf} $g_strWinVersion == "7"
  ${OrIf} $g_strWinVersion == "8"
  ${OrIf} $g_strWinVersion == "8_1"
  ${OrIf} $g_strWinVersion == "10"
    StrCpy $reg_path_device "SYSTEM\CurrentControlSet\Control\Video\$dev_id\0000"
    StrCpy $reg_path_monitor "SYSTEM\CurrentControlSet\Control\VIDEO\$dev_id\0000\Mon00000001"
  ${Else}
    ${LogVerbose} "Custom mode set: Windows $g_strWinVersion not supported yet"
    Goto exit
  ${EndIf}

  ; Write the new value in the adapter config (VBoxVideo.sys) using hex values in binary format
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" registry write HKLM $reg_path_device CustomXRes REG_BIN $g_iScreenX DWORD" "false"
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" registry write HKLM $reg_path_device CustomYRes REG_BIN $g_iScreenY DWORD" "false"
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" registry write HKLM $reg_path_device CustomBPP REG_BIN $g_iScreenBpp DWORD" "false"

  ; ... and tell Windows to use that mode on next start!
  WriteRegDWORD HKCC $reg_path_device "DefaultSettings.XResolution" "$g_iScreenX"
  WriteRegDWORD HKCC $reg_path_device "DefaultSettings.YResolution" "$g_iScreenY"
  WriteRegDWORD HKCC $reg_path_device "DefaultSettings.BitsPerPixel" "$g_iScreenBpp"

  WriteRegDWORD HKCC $reg_path_monitor "DefaultSettings.XResolution" "$g_iScreenX"
  WriteRegDWORD HKCC $reg_path_monitor "DefaultSettings.YResolution" "$g_iScreenY"
  WriteRegDWORD HKCC $reg_path_monitor "DefaultSettings.BitsPerPixel" "$g_iScreenBpp"

  ${LogVerbose} "Custom mode set to $g_iScreenXx$g_iScreenY, $g_iScreenBpp BPP on next restart."

exit:

FunctionEnd

Function W2K_Prepare

  ${If} $g_bNoVBoxServiceExit == "false"
    ; Stop / kill VBoxService
    Call StopVBoxService
  ${EndIf}

  ${If} $g_bNoVBoxTrayExit == "false"
    ; Stop / kill VBoxTray
    Call StopVBoxTray
  ${EndIf}

  ; Delete VBoxService from registry
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "VBoxService"

  ; Delete old VBoxService.exe from install directory (replaced by VBoxTray.exe)
  Delete /REBOOTOK "$INSTDIR\VBoxService.exe"

FunctionEnd

Function W2K_CopyFiles

  Push $0
  SetOutPath "$INSTDIR"

  ; Video driver
  FILE "$%PATH_OUT%\bin\additions\VBoxVideo.sys"
  FILE "$%PATH_OUT%\bin\additions\VBoxDisp.dll"

  ; Mouse driver
  FILE "$%PATH_OUT%\bin\additions\VBoxMouse.sys"
  FILE "$%PATH_OUT%\bin\additions\VBoxMouse.inf"
!ifdef VBOX_SIGN_ADDITIONS
  FILE "$%PATH_OUT%\bin\additions\VBoxMouse.cat"
!endif

  ; Guest driver
  FILE "$%PATH_OUT%\bin\additions\VBoxGuest.sys"
  FILE "$%PATH_OUT%\bin\additions\VBoxGuest.inf"
!ifdef VBOX_SIGN_ADDITIONS
  FILE "$%PATH_OUT%\bin\additions\VBoxGuest.cat"
!endif

  ; Guest driver files
  FILE "$%PATH_OUT%\bin\additions\VBoxTray.exe"
  FILE "$%PATH_OUT%\bin\additions\VBoxControl.exe" ; Not used by W2K and up, but required by the .INF file

  ; WHQL fake
!ifdef WHQL_FAKE
  FILE "$%PATH_OUT%\bin\additions\VBoxWHQLFake.exe"
!endif

  SetOutPath $g_strSystemDir

  ; VBoxService
  ${If} $g_bNoVBoxServiceExit == "false"
    ; VBoxService has been terminated before, so just install the file
    ; in the regular way
    FILE "$%PATH_OUT%\bin\additions\VBoxService.exe"
  ${Else}
    ; VBoxService is in use and wasn't terminated intentionally. So extract the
    ; new version into a temporary location and install it on next reboot
    Push $0
    ClearErrors
    GetTempFileName $0
    IfErrors 0 +3
      ${LogVerbose} "Error getting temp file for VBoxService.exe"
      StrCpy "$0" "$INSTDIR\VBoxServiceTemp.exe"
    ${LogVerbose} "VBoxService is in use, will be installed on next reboot (from '$0')"
    File "/oname=$0" "$%PATH_OUT%\bin\additions\VBoxService.exe"
    IfErrors 0 +2
      ${LogVerbose} "Error copying VBoxService.exe to '$0'"
    Rename /REBOOTOK "$0" "$g_strSystemDir\VBoxService.exe"
    IfErrors 0 +2
      ${LogVerbose} "Error renaming '$0' to '$g_strSystemDir\VBoxService.exe'"
    Pop $0
  ${EndIf}

!if $%VBOX_WITH_WDDM% == "1"
  ${If} $g_bWithWDDM == "true"
    ; WDDM Video driver
    SetOutPath "$INSTDIR"

  !if $%VBOX_WITH_WDDM_W8% == "1"
    ${If} $g_strWinVersion == "8"
    ${OrIf} $g_strWinVersion == "8_1"
    ${OrIf} $g_strWinVersion == "10"
      !ifdef VBOX_SIGN_ADDITIONS
        FILE "$%PATH_OUT%\bin\additions\VBoxVideoW8.cat"
      !endif
      FILE "$%PATH_OUT%\bin\additions\VBoxVideoW8.sys"
      FILE "$%PATH_OUT%\bin\additions\VBoxVideoW8.inf"
    ${Else}
  !endif
      !ifdef VBOX_SIGN_ADDITIONS
        FILE "$%PATH_OUT%\bin\additions\VBoxVideoWddm.cat"
      !endif
      FILE "$%PATH_OUT%\bin\additions\VBoxVideoWddm.sys"
      FILE "$%PATH_OUT%\bin\additions\VBoxVideoWddm.inf"
  !if $%VBOX_WITH_WDDM_W8% == "1"
    ${EndIf}
  !endif

    FILE "$%PATH_OUT%\bin\additions\VBoxDispD3D.dll"

    !if $%VBOX_WITH_CROGL% == "1"
      FILE "$%PATH_OUT%\bin\additions\VBoxOGLarrayspu.dll"
      FILE "$%PATH_OUT%\bin\additions\VBoxOGLcrutil.dll"
      FILE "$%PATH_OUT%\bin\additions\VBoxOGLerrorspu.dll"
      FILE "$%PATH_OUT%\bin\additions\VBoxOGLpackspu.dll"
      FILE "$%PATH_OUT%\bin\additions\VBoxOGLpassthroughspu.dll"
      FILE "$%PATH_OUT%\bin\additions\VBoxOGLfeedbackspu.dll"
      FILE "$%PATH_OUT%\bin\additions\VBoxOGL.dll"

      FILE "$%PATH_OUT%\bin\additions\VBoxD3D9wddm.dll"
      FILE "$%PATH_OUT%\bin\additions\wined3dwddm.dll"
    !endif ; $%VBOX_WITH_CROGL% == "1"

    !if $%BUILD_TARGET_ARCH% == "amd64"
      FILE "$%PATH_OUT%\bin\additions\VBoxDispD3D-x86.dll"

      !if $%VBOX_WITH_CROGL% == "1"
        FILE "$%PATH_OUT%\bin\additions\VBoxOGLarrayspu-x86.dll"
        FILE "$%PATH_OUT%\bin\additions\VBoxOGLcrutil-x86.dll"
        FILE "$%PATH_OUT%\bin\additions\VBoxOGLerrorspu-x86.dll"
        FILE "$%PATH_OUT%\bin\additions\VBoxOGLpackspu-x86.dll"
        FILE "$%PATH_OUT%\bin\additions\VBoxOGLpassthroughspu-x86.dll"
        FILE "$%PATH_OUT%\bin\additions\VBoxOGLfeedbackspu-x86.dll"
        FILE "$%PATH_OUT%\bin\additions\VBoxOGL-x86.dll"

        FILE "$%PATH_OUT%\bin\additions\VBoxD3D9wddm-x86.dll"
        FILE "$%PATH_OUT%\bin\additions\wined3dwddm-x86.dll"
      !endif ; $%VBOX_WITH_CROGL% == "1"
    !endif ; $%BUILD_TARGET_ARCH% == "amd64"

    Goto doneCr
  ${EndIf}
!endif ; $%VBOX_WITH_WDDM% == "1"

!if $%VBOX_WITH_CROGL% == "1"
  ; crOpenGL
  !if $%BUILD_TARGET_ARCH% == "amd64"
    !define LIBRARY_X64 ; Enable installation of 64-bit libraries
  !endif
  StrCpy $0 "$TEMP\VBoxGuestAdditions\VBoxOGL"
  CreateDirectory "$0"
  !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%PATH_OUT%\bin\additions\VBoxOGLarrayspu.dll"       "$g_strSystemDir\VBoxOGLarrayspu.dll"       "$0"
  !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%PATH_OUT%\bin\additions\VBoxOGLcrutil.dll"         "$g_strSystemDir\VBoxOGLcrutil.dll"         "$0"
  !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%PATH_OUT%\bin\additions\VBoxOGLerrorspu.dll"       "$g_strSystemDir\VBoxOGLerrorspu.dll"       "$0"
  !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%PATH_OUT%\bin\additions\VBoxOGLpackspu.dll"        "$g_strSystemDir\VBoxOGLpackspu.dll"        "$0"
  !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%PATH_OUT%\bin\additions\VBoxOGLpassthroughspu.dll" "$g_strSystemDir\VBoxOGLpassthroughspu.dll" "$0"
  !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%PATH_OUT%\bin\additions\VBoxOGLfeedbackspu.dll"    "$g_strSystemDir\VBoxOGLfeedbackspu.dll"    "$0"
  !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%PATH_OUT%\bin\additions\VBoxOGL.dll"               "$g_strSystemDir\VBoxOGL.dll"               "$0"
  !if $%BUILD_TARGET_ARCH% == "amd64"
    !undef LIBRARY_X64 ; Disable installation of 64-bit libraries
  !endif

  !if $%BUILD_TARGET_ARCH% == "amd64"
    StrCpy $0 "$TEMP\VBoxGuestAdditions\VBoxOGL32"
    CreateDirectory "$0"
    ; Only 64-bit installer: Also copy 32-bit DLLs on 64-bit target arch in
    ; Wow64 node (32-bit sub system). Note that $SYSDIR contains the 32-bit
    ; path after calling EnableX64FSRedirection
    ${EnableX64FSRedirection}
    !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%VBOX_PATH_ADDITIONS_WIN_X86%\VBoxOGLarrayspu.dll"       "$SYSDIR\VBoxOGLarrayspu.dll"       "$0"
    !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%VBOX_PATH_ADDITIONS_WIN_X86%\VBoxOGLcrutil.dll"         "$SYSDIR\VBoxOGLcrutil.dll"         "$0"
    !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%VBOX_PATH_ADDITIONS_WIN_X86%\VBoxOGLerrorspu.dll"       "$SYSDIR\VBoxOGLerrorspu.dll"       "$0"
    !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%VBOX_PATH_ADDITIONS_WIN_X86%\VBoxOGLpackspu.dll"        "$SYSDIR\VBoxOGLpackspu.dll"        "$0"
    !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%VBOX_PATH_ADDITIONS_WIN_X86%\VBoxOGLpassthroughspu.dll" "$SYSDIR\VBoxOGLpassthroughspu.dll" "$0"
    !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%VBOX_PATH_ADDITIONS_WIN_X86%\VBoxOGLfeedbackspu.dll"    "$SYSDIR\VBoxOGLfeedbackspu.dll"    "$0"
    !insertmacro InstallLib DLL NOTSHARED REBOOT_PROTECTED "$%VBOX_PATH_ADDITIONS_WIN_X86%\VBoxOGL.dll"               "$SYSDIR\VBoxOGL.dll"               "$0"
    ${DisableX64FSRedirection}
  !endif

!endif ; VBOX_WITH_CROGL

doneCr:

  Pop $0

FunctionEnd

!ifdef WHQL_FAKE

Function W2K_WHQLFakeOn

  StrCmp $g_bFakeWHQL "true" do
  Goto exit

do:

  ${LogVerbose} "Turning off WHQL protection..."
  ${CmdExecute} "$\"$INSTDIR\VBoxWHQLFake.exe$\" $\"ignore$\"" "true"

exit:

FunctionEnd

Function W2K_WHQLFakeOff

  StrCmp $g_bFakeWHQL "true" do
  Goto exit

do:

  ${LogVerbose} "Turning back on WHQL protection..."
  ${CmdExecute} "$\"$INSTDIR\VBoxWHQLFake.exe$\" $\"warn$\"" "true"

exit:

FunctionEnd

!endif

Function W2K_InstallFiles

  ; The Shared Folder IFS goes to the system directory
  !insertmacro ReplaceDLL "$%PATH_OUT%\bin\additions\VBoxSF.sys" "$g_strSystemDir\drivers\VBoxSF.sys" "$INSTDIR"
  !insertmacro ReplaceDLL "$%PATH_OUT%\bin\additions\VBoxMRXNP.dll" "$g_strSystemDir\VBoxMRXNP.dll" "$INSTDIR"
  AccessControl::GrantOnFile "$g_strSystemDir\VBoxMRXNP.dll" "(BU)" "GenericRead"
  !if $%BUILD_TARGET_ARCH% == "amd64"
    ; Only 64-bit installer: Copy the 32-bit DLL for 32 bit applications.
    !insertmacro ReplaceDLL "$%PATH_OUT%\bin\additions\VBoxMRXNP-x86.dll" "$g_strSysWow64\VBoxMRXNP.dll" "$INSTDIR"
    AccessControl::GrantOnFile "$g_strSysWow64\VBoxMRXNP.dll" "(BU)" "GenericRead"
  !endif

  ; The VBoxTray hook DLL also goes to the system directory; it might be locked
  !insertmacro ReplaceDLL "$%PATH_OUT%\bin\additions\VBoxHook.dll" "$g_strSystemDir\VBoxHook.dll" "$INSTDIR"
  AccessControl::GrantOnFile "$g_strSystemDir\VBoxHook.dll" "(BU)" "GenericRead"

  ${LogVerbose} "Installing drivers ..."

  Push $0 ; For fetching results

  SetOutPath "$INSTDIR"

  ${If} $g_bNoGuestDrv == "false"
    ${LogVerbose} "Installing guest driver ..."
    ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" driver install $\"$INSTDIR\VBoxGuest.inf$\" $\"$INSTDIR\install_drivers.log$\"" "false"
  ${Else}
    ${LogVerbose} "Guest driver installation skipped!"
  ${EndIf}

  ${If} $g_bNoVideoDrv == "false"
    ${If} $g_bWithWDDM == "true"
  !if $%VBOX_WITH_WDDM_W8% == "1"
      ${If} $g_strWinVersion == "8"
      ${OrIf} $g_strWinVersion == "8_1"
      ${OrIf} $g_strWinVersion == "10"
        ${LogVerbose} "Installing WDDM video driver for Windows 8 or newer..."
        ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" driver install $\"$INSTDIR\VBoxVideoW8.inf$\" $\"$INSTDIR\install_drivers.log$\"" "false"
      ${Else}
  !endif
        ${LogVerbose} "Installing WDDM video driver for Windows Vista and 7..."
        ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" driver install $\"$INSTDIR\VBoxVideoWddm.inf$\" $\"$INSTDIR\install_drivers.log$\"" "false"
  !if $%VBOX_WITH_WDDM_W8% == "1"
      ${EndIf}
  !endif
    ${Else}
      ${LogVerbose} "Installing video driver ..."
      ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" driver install $\"$INSTDIR\VBoxVideo.inf$\" $\"$INSTDIR\install_drivers.log$\"" "false"
    ${EndIf}
  ${Else}
    ${LogVerbose} "Video driver installation skipped!"
  ${EndIf}

  ${If} $g_bNoMouseDrv == "false"
    ${LogVerbose} "Installing mouse driver ..."
    ; The mouse filter does not contain any device IDs but a "DefaultInstall" section;
    ; so this .INF file needs to be installed using "InstallHinfSection" which is implemented
    ; with VBoxDrvInst's "driver executeinf" routine
    ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" driver executeinf $\"$INSTDIR\VBoxMouse.inf$\"" "false"
  ${Else}
    ${LogVerbose} "Mouse driver installation skipped!"
  ${EndIf}

  ; Create the VBoxService service
  ; No need to stop/remove the service here! Do this only on uninstallation!
  ${LogVerbose} "Installing VirtualBox service ..."
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" service create $\"VBoxService$\" $\"VirtualBox Guest Additions Service$\" 16 2 $\"%SystemRoot%\System32\VBoxService.exe$\" $\"Base$\"" "false"

  ; Set service description
  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Services\VBoxService" "Description" "Manages VM runtime information, time synchronization, remote sysprep execution and miscellaneous utilities for guest operating systems."

sf:

  ${LogVerbose} "Installing Shared Folders service ..."

  ; Create the Shared Folders service ...
  ; No need to stop/remove the service here! Do this only on uninstallation!
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" service create $\"VBoxSF$\" $\"VirtualBox Shared Folders$\" 2 1 $\"\SystemRoot\System32\drivers\VBoxSF.sys$\" $\"NetworkProvider$\"" "false"

  ; ... and the link to the network provider
  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Services\VBoxSF\NetworkProvider" "DeviceName" "\Device\VBoxMiniRdr"
  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Services\VBoxSF\NetworkProvider" "Name" "VirtualBox Shared Folders"
  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Services\VBoxSF\NetworkProvider" "ProviderPath" "$SYSDIR\VBoxMRXNP.dll"

  ; Add default network providers (if not present or corrupted)
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" netprovider add WebClient" "false"
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" netprovider add LanmanWorkstation" "false"
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" netprovider add RDPNP" "false"

  ; Add the shared folders network provider
  ${LogVerbose} "Adding network provider (Order = $g_iSfOrder) ..."
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" netprovider add VBoxSF $g_iSfOrder" "false"

!if $%VBOX_WITH_CROGL% == "1"
cropengl:
  ${If} $g_bWithWDDM == "true"
    ; Nothing to do here
  ${Else}
    ${LogVerbose} "Installing 3D OpenGL support ..."
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\VBoxOGL" "Version" 2
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\VBoxOGL" "DriverVersion" 1
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\VBoxOGL" "Flags" 1
    WriteRegStr   HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\VBoxOGL" "Dll" "VBoxOGL.dll"
!if $%BUILD_TARGET_ARCH% == "amd64"
    SetRegView 32
    ; Write additional keys required for Windows XP, Vista and 7 64-bit (but for 32-bit stuff)
    ${If} $g_strWinVersion   == '10'
    ${OrIf} $g_strWinVersion == '8_1'
    ${OrIf} $g_strWinVersion == '8'
    ${OrIf} $g_strWinVersion == '7'
    ${OrIf} $g_strWinVersion == 'Vista'
    ${OrIf} $g_strWinVersion == '2003' ; Windows XP 64-bit is a renamed Windows 2003 really
      WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\VBoxOGL" "Version" 2
      WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\VBoxOGL" "DriverVersion" 1
      WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\VBoxOGL" "Flags" 1
      WriteRegStr   HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\VBoxOGL" "Dll" "VBoxOGL.dll"
    ${EndIf}
    SetRegView 64
!endif
  ${Endif}
!endif

  Goto done

done:

  Pop $0

FunctionEnd

Function W2K_Main

  SetOutPath "$INSTDIR"
  SetOverwrite on

  Call W2K_Prepare
  Call W2K_CopyFiles

!ifdef WHQL_FAKE
  Call W2K_WHQLFakeOn
!endif

  Call W2K_InstallFiles

!ifdef WHQL_FAKE
  Call W2K_WHQLFakeOff
!endif

  Call W2K_SetVideoResolution

FunctionEnd

!macro W2K_UninstallInstDir un
Function ${un}W2K_UninstallInstDir

  Delete /REBOOTOK "$INSTDIR\VBoxVideo.sys"
  Delete /REBOOTOK "$INSTDIR\VBoxVideo.inf"
  Delete /REBOOTOK "$INSTDIR\VBoxVideo.cat"
  Delete /REBOOTOK "$INSTDIR\VBoxDisp.dll"

  Delete /REBOOTOK "$INSTDIR\VBoxMouse.sys"
  Delete /REBOOTOK "$INSTDIR\VBoxMouse.inf"
  Delete /REBOOTOK "$INSTDIR\VBoxMouse.cat"

  Delete /REBOOTOK "$INSTDIR\VBoxTray.exe"

  Delete /REBOOTOK "$INSTDIR\VBoxGuest.sys"
  Delete /REBOOTOK "$INSTDIR\VBoxGuest.inf"
  Delete /REBOOTOK "$INSTDIR\VBoxGuest.cat"

  Delete /REBOOTOK "$INSTDIR\VBCoInst.dll" ; Deprecated, does not get installed anymore
  Delete /REBOOTOK "$INSTDIR\VBoxControl.exe"
  Delete /REBOOTOK "$INSTDIR\VBoxService.exe" ; Deprecated, does not get installed anymore

!if $%VBOX_WITH_WDDM% == "1"
  Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxVideoWddm.cat"
  Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxVideoWddm.sys"
  Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxVideoWddm.inf"
  !if $%VBOX_WITH_WDDM_W8% == "1"
  Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxVideoW8.cat"
  Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxVideoW8.sys"
  Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxVideoW8.inf"
  !endif
  Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxDispD3D.dll"

    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLarrayspu.dll"
    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLcrutil.dll"
    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLerrorspu.dll"
    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLpackspu.dll"
    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLpassthroughspu.dll"
    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLfeedbackspu.dll"
    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGL.dll"

    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxD3D9wddm.dll"
    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\wined3dwddm.dll"
    ; Try to delete libWine in case it is there from old installation
    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\libWine.dll"

  !if $%BUILD_TARGET_ARCH% == "amd64"
    Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxDispD3D-x86.dll"

      Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLarrayspu-x86.dll"
      Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLcrutil-x86.dll"
      Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLerrorspu-x86.dll"
      Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLpackspu-x86.dll"
      Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLpassthroughspu-x86.dll"
      Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGLfeedbackspu-x86.dll"
      Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxOGL-x86.dll"

      Delete /REBOOTOK "$%PATH_OUT%\bin\additions\VBoxD3D9wddm-x86.dll"
      Delete /REBOOTOK "$%PATH_OUT%\bin\additions\wined3dwddm-x86.dll"
  !endif ; $%BUILD_TARGET_ARCH% == "amd64"
!endif ; $%VBOX_WITH_WDDM% == "1"

  ; WHQL fake
!ifdef WHQL_FAKE
  Delete /REBOOTOK "$INSTDIR\VBoxWHQLFake.exe"
!endif

  ; Log file
  Delete /REBOOTOK "$INSTDIR\install.log"
  Delete /REBOOTOK "$INSTDIR\install_ui.log"

FunctionEnd
!macroend
!insertmacro W2K_UninstallInstDir ""
!insertmacro W2K_UninstallInstDir "un."

!macro W2K_Uninstall un
Function ${un}W2K_Uninstall

  Push $0

  ; Remove VirtualBox video driver
  ${LogVerbose} "Uninstalling video driver ..."
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" driver uninstall $\"$INSTDIR\VBoxVideo.inf$\"" "true"
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" service delete VBoxVideo" "true"
  Delete /REBOOTOK "$g_strSystemDir\drivers\VBoxVideo.sys"
  Delete /REBOOTOK "$g_strSystemDir\VBoxDisp.dll"

  ; Remove video driver
!if $%VBOX_WITH_WDDM% == "1"

  !if $%VBOX_WITH_WDDM_W8% == "1"
  ${LogVerbose} "Uninstalling WDDM video driver for Windows 8..."
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" driver uninstall $\"$INSTDIR\VBoxVideoW8.inf$\"" "true"
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" service delete VBoxVideoW8" "true"
  ;misha> @todo driver file removal (as well as service removal) should be done as driver package uninstall
  ;       could be done with "VBoxDrvInst.exe /u", e.g. by passing additional arg to it denoting that driver package is to be uninstalled
  Delete /REBOOTOK "$g_strSystemDir\drivers\VBoxVideoW8.sys"
  !endif ; $%VBOX_WITH_WDDM_W8% == "1"

  ${LogVerbose} "Uninstalling WDDM video driver for Windows Vista and 7..."
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" driver uninstall $\"$INSTDIR\VBoxVideoWddm.inf$\"" "true"
  ; Always try to remove both VBoxVideoWddm & VBoxVideo services no matter what is installed currently
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" service delete VBoxVideoWddm" "true"
  ;misha> @todo driver file removal (as well as service removal) should be done as driver package uninstall
  ;       could be done with "VBoxDrvInst.exe /u", e.g. by passing additional arg to it denoting that driver package is to be uninstalled
  Delete /REBOOTOK "$g_strSystemDir\drivers\VBoxVideoWddm.sys"
  Delete /REBOOTOK "$g_strSystemDir\VBoxDispD3D.dll"
!endif ; $%VBOX_WITH_WDDM% == "1"

!if $%VBOX_WITH_CROGL% == "1"

  ${LogVerbose} "Removing Direct3D support ..."

  ; Do file validation before we uninstall
  Call ${un}ValidateD3DFiles
  Pop $0
  ${If} $0 == "1" ; D3D files are invalid
    ${LogVerbose} $(VBOX_UNINST_INVALID_D3D)
    MessageBox MB_ICONSTOP|MB_OK $(VBOX_UNINST_INVALID_D3D) /SD IDOK
    Goto d3d_uninstall_end
  ${EndIf}

  Delete /REBOOTOK "$g_strSystemDir\VBoxOGLarrayspu.dll"
  Delete /REBOOTOK "$g_strSystemDir\VBoxOGLcrutil.dll"
  Delete /REBOOTOK "$g_strSystemDir\VBoxOGLerrorspu.dll"
  Delete /REBOOTOK "$g_strSystemDir\VBoxOGLpackspu.dll"
  Delete /REBOOTOK "$g_strSystemDir\VBoxOGLpassthroughspu.dll"
  Delete /REBOOTOK "$g_strSystemDir\VBoxOGLfeedbackspu.dll"
  Delete /REBOOTOK "$g_strSystemDir\VBoxOGL.dll"

  ; Remove D3D stuff
  ; @todo add a feature flag to only remove if installed explicitly
  Delete /REBOOTOK "$g_strSystemDir\libWine.dll"
  Delete /REBOOTOK "$g_strSystemDir\VBoxD3D8.dll"
  Delete /REBOOTOK "$g_strSystemDir\VBoxD3D9.dll"
  Delete /REBOOTOK "$g_strSystemDir\wined3d.dll"
  ; Update DLL cache
  ${If} ${FileExists} "$g_strSystemDir\dllcache\msd3d8.dll"
    Delete /REBOOTOK "$g_strSystemDir\dllcache\d3d8.dll"
    Rename /REBOOTOK "$g_strSystemDir\dllcache\msd3d8.dll" "$g_strSystemDir\dllcache\d3d8.dll"
  ${EndIf}
  ${If} ${FileExists} "$g_strSystemDir\dllcache\msd3d9.dll"
    Delete /REBOOTOK "$g_strSystemDir\dllcache\d3d9.dll"
    Rename /REBOOTOK "$g_strSystemDir\dllcache\msd3d9.dll" "$g_strSystemDir\dllcache\d3d9.dll"
  ${EndIf}
  ; Restore original DX DLLs
  ${If} ${FileExists} "$g_strSystemDir\msd3d8.dll"
    Delete /REBOOTOK "$g_strSystemDir\d3d8.dll"
    Rename /REBOOTOK "$g_strSystemDir\msd3d8.dll" "$g_strSystemDir\d3d8.dll"
  ${EndIf}
  ${If} ${FileExists} "$g_strSystemDir\msd3d9.dll"
    Delete /REBOOTOK "$g_strSystemDir\d3d9.dll"
    Rename /REBOOTOK "$g_strSystemDir\msd3d9.dll" "$g_strSystemDir\d3d9.dll"
  ${EndIf}

  !if $%BUILD_TARGET_ARCH% == "amd64"
    ; Only 64-bit installer: Also remove 32-bit DLLs on 64-bit target arch in Wow64 node
    Delete /REBOOTOK "$g_strSysWow64\VBoxOGLarrayspu.dll"
    Delete /REBOOTOK "$g_strSysWow64\VBoxOGLcrutil.dll"
    Delete /REBOOTOK "$g_strSysWow64\VBoxOGLerrorspu.dll"
    Delete /REBOOTOK "$g_strSysWow64\VBoxOGLpackspu.dll"
    Delete /REBOOTOK "$g_strSysWow64\VBoxOGLpassthroughspu.dll"
    Delete /REBOOTOK "$g_strSysWow64\VBoxOGLfeedbackspu.dll"
    Delete /REBOOTOK "$g_strSysWow64\VBoxOGL.dll"

    ; Remove D3D stuff
    ; @todo add a feature flag to only remove if installed explicitly
    Delete /REBOOTOK "$g_strSysWow64\libWine.dll"
    Delete /REBOOTOK "$g_strSysWow64\VBoxD3D8.dll"
    Delete /REBOOTOK "$g_strSysWow64\VBoxD3D9.dll"
    Delete /REBOOTOK "$g_strSysWow64\wined3d.dll"
    ; Update DLL cache
    ${If} ${FileExists} "$g_strSysWow64\dllcache\msd3d8.dll"
      Delete /REBOOTOK "$g_strSysWow64\dllcache\d3d8.dll"
      Rename /REBOOTOK "$g_strSysWow64\dllcache\msd3d8.dll" "$g_strSysWow64\dllcache\d3d8.dll"
    ${EndIf}
    ${If} ${FileExists} "$g_strSysWow64\dllcache\msd3d9.dll"
      Delete /REBOOTOK "$g_strSysWow64\dllcache\d3d9.dll"
      Rename /REBOOTOK "$g_strSysWow64\dllcache\msd3d9.dll" "$g_strSysWow64\dllcache\d3d9.dll"
    ${EndIf}
    ; Restore original DX DLLs
    ${If} ${FileExists} "$g_strSysWow64\msd3d8.dll"
      Delete /REBOOTOK "$g_strSysWow64\d3d8.dll"
      Rename /REBOOTOK "$g_strSysWow64\msd3d8.dll" "$g_strSysWow64\d3d8.dll"
    ${EndIf}
    ${If} ${FileExists} "$g_strSysWow64\msd3d9.dll"
      Delete /REBOOTOK "$g_strSysWow64\d3d9.dll"
      Rename /REBOOTOK "$g_strSysWow64\msd3d9.dll" "$g_strSysWow64\d3d9.dll"
    ${EndIf}
    DeleteRegKey HKLM "SOFTWARE\Wow6432Node\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\VBoxOGL"
  !endif ; amd64

  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\OpenGLDrivers\VBoxOGL"

d3d_uninstall_end:

!endif ; VBOX_WITH_CROGL

  ; Remove mouse driver
  ${LogVerbose} "Removing mouse driver ..."
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" service delete VBoxMouse" "true"
  Delete /REBOOTOK "$g_strSystemDir\drivers\VBoxMouse.sys"
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" registry delmultisz $\"SYSTEM\CurrentControlSet\Control\Class\{4D36E96F-E325-11CE-BFC1-08002BE10318}$\" $\"UpperFilters$\" $\"VBoxMouse$\"" "true"

  ; Delete the VBoxService service
  Call ${un}StopVBoxService
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" service delete VBoxService" "true"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "VBoxService"
  Delete /REBOOTOK "$g_strSystemDir\VBoxService.exe"

  ; VBoxGINA
  Delete /REBOOTOK "$g_strSystemDir\VBoxGINA.dll"
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\WinLogon" "GinaDLL"
  ${If} $0 == "VBoxGINA.dll"
    ${LogVerbose} "Removing auto-logon support ..."
    DeleteRegValue HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\WinLogon" "GinaDLL"
  ${EndIf}
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\WinLogon\Notify\VBoxGINA"

  ; Delete VBoxTray
  Call ${un}StopVBoxTray
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "VBoxTray"

  ; Remove guest driver
  ${LogVerbose} "Removing guest driver ..."
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" driver uninstall $\"$INSTDIR\VBoxGuest.inf$\"" "true"

  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" service delete VBoxGuest" "true"
  Delete /REBOOTOK "$g_strSystemDir\drivers\VBoxGuest.sys"
  Delete /REBOOTOK "$g_strSystemDir\VBCoInst.dll" ; Deprecated, does not get installed anymore
  Delete /REBOOTOK "$g_strSystemDir\VBoxTray.exe"
  Delete /REBOOTOK "$g_strSystemDir\VBoxHook.dll"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "VBoxTray" ; Remove VBoxTray autorun
  Delete /REBOOTOK "$g_strSystemDir\VBoxControl.exe"

  ; Remove shared folders driver
  ${LogVerbose} "Removing shared folders driver ..."
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" netprovider remove VBoxSF" "true"
  ${CmdExecute} "$\"$INSTDIR\VBoxDrvInst.exe$\" service delete VBoxSF" "true"
  Delete /REBOOTOK "$g_strSystemDir\VBoxMRXNP.dll" ; The network provider DLL will be locked
  !if $%BUILD_TARGET_ARCH% == "amd64"
    ; Only 64-bit installer: Also remove 32-bit DLLs on 64-bit target arch in Wow64 node
    Delete /REBOOTOK "$g_strSysWow64\VBoxMRXNP.dll"
  !endif ; amd64
  Delete /REBOOTOK "$g_strSystemDir\drivers\VBoxSF.sys"

  Pop $0

FunctionEnd
!macroend
!insertmacro W2K_Uninstall ""
!insertmacro W2K_Uninstall "un."

