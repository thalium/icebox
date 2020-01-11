' $Id: vboxinfo.vbs $
'' @file
' ???
'

'
' Copyright (C) 2009-2017 Oracle Corporation
'
' This file is part of VirtualBox Open Source Edition (OSE), as
' available from http://www.virtualbox.org. This file is free software;
' you can redistribute it and/or modify it under the terms of the GNU
' General Public License (GPL) as published by the Free Software
' Foundation, in version 2 as it comes in the "COPYING" file of the
' VirtualBox OSE distribution. VirtualBox OSE is distributed in the
' hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
'

Sub Print(str)
   Wscript.Echo str
End Sub

Sub StartVm(vb, mach)
  Dim session, progress

  Set session = CreateObject("VirtualBox.Session")
  Set progress = vb.openRemoteSession(session, mach.id, "gui", "")
  progress.waitForCompletion(-1)
  session.close()
End Sub


Sub StopVm(vb, mach)
  Dim session, progress

  Set session = CreateObject("VirtualBox.Session")
  vb.openExistingSession session, mach.id
  session.console.powerDown().waitForCompletion(-1)
  session.close()
End Sub


Sub Main
   Dim vb, mach

   set vb = CreateObject("VirtualBox.VirtualBox")
   Print "VirtualBox version " & vb.version

   ' Safe arrays not fully functional from Visual Basic Script, as we
   ' return real safe arrays, not ones wrapped to VARIANT and VBS engine
   ' gets confused. Until then, explicitly find VM by name.
   ' May wish to use hack like one described in
   ' http://www.tech-archive.net/Archive/Excel/microsoft.public.excel.programming/2006-05/msg02796.html to handle safearrays
   ' if desperate

   Set mach = vb.findMachine("Win")
   Print "Machine: " & mach.name  & " ID: " & mach.id

   StartVm vb, mach
End Sub

Main

