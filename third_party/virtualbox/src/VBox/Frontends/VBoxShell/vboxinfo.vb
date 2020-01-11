' $Id: vboxinfo.vb $
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

Imports System
Imports System.Drawing
Imports System.Windows.Forms

Module Module1

    Sub Main()
        Dim vb As VirtualBox.IVirtualBox
        Dim listBox As New ListBox()
        Dim form As New Form

        vb = CreateObject("VirtualBox.VirtualBox")

        form.Text = "VirtualBox version " & vb.Version
        form.Size = New System.Drawing.Size(400, 320)
        form.Location = New System.Drawing.Point(10, 10)

        listBox.Size = New System.Drawing.Size(350, 200)
        listBox.Location = New System.Drawing.Point(10, 10)

        For Each m In vb.Machines
            listBox.Items.Add(m.Name & " [" & m.Id & "]")
        Next

        form.Controls.Add(listBox)

        'form.ShowDialog()
        form.Show()
        MsgBox("OK")
    End Sub

End Module

