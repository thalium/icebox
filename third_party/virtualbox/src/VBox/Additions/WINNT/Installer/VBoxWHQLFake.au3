; VBoxFakeWHQL - Turns off / on the WHQL for installing unsigned drivers.
; Currently only tested with Win2K / XP!
;
; Copyright (C) 2008-2010 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

; Strings for localization
; When using an OS with another language, you have to provide the correct
; strings for it. English and German is provided below.
;
; @todo Needs to be improved to handle the stuff without language strings in
; the future!

;$sysprop_Title = "System Properties"
;$drvsig_Title = "Driver Signing Options"

$sysprop_Title = "Systemeigenschaften"
$drvsig_Title = "Treibersignaturoptionen"

$ok_Title  = "OK"

If $CmdLine[0] < 1 Then
	MsgBox ( 0, "ERROR", "Please specify 'ignore', 'warn' or 'block' as parameter!" )
	Exit
EndIf

Run("control.exe sysdm.cpl")
If WinWait($sysprop_Title, "", 5) = 0 Then
   WinClose("[ACTIVE]", "")
   Exit
EndIf

ControlCommand($sysprop_Title, "", "SysTabControl321", "TabRight", "")
ControlCommand($sysprop_Title, "", "SysTabControl321", "TabRight", "")

Sleep(200)                                      ; Wait a while for tab switching above
ControlClick($sysprop_Title, "", 14007)         ; Button 'Driver Signing'

WinWait($drvsig_Title, "", 5)

If $CmdLine[1] = "ignore" Then
   ControlClick($drvsig_Title, "", 1000)       ; 'Ignore' radio button
ElseIf $CmdLine[1] = "warn" Then
   ControlClick($drvsig_Title, "", 1001)       ; 'Warn' radio button
ElseIf $CmdLine[1] = "block" Then
   ControlClick($drvsig_Title, "", 1002)       ; 'Block' radio button
Else
   Exit
EndIf

ControlClick($drvsig_Title, "", 1)     ; 'OK' button (ID=1) of dialog 'Driver Signing Options'

WinWait($sysprop_Title, "", 5)
ControlClick($sysprop_Title, "", 1)    ; 'OK' button (ID=1) of 'System Properties'


