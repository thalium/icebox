# $Id: win-vbox-net-drvstore-cleanup.ps1 $
## @file
# VirtualBox Validation Kit - network cleanup script (powershell).
#

#
# Copyright (C) 2006-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#
# The contents of this file may alternatively be used under the terms
# of the Common Development and Distribution License Version 1.0
# (CDDL) only, as it comes in the "COPYING.CDDL" file of the
# VirtualBox OSE distribution, in which case the provisions of the
# CDDL are applicable instead of those of the GPL.
#
# You may elect to license modified versions of this file under the
# terms and conditions of either the GPL or the CDDL or both.
#

param([switch]$confirm)

Function AskForConfirmation ($title_text, $message_text, $yes_text, $no_text)
{
   if ($confirm) {
      $title = $title_text
      $message = $message_text

      $yes = New-Object System.Management.Automation.Host.ChoiceDescription "&Yes", $yes_text

      $no = New-Object System.Management.Automation.Host.ChoiceDescription "&No", $no_text

      $options = [System.Management.Automation.Host.ChoiceDescription[]]($yes, $no)

      $result = $host.ui.PromptForChoice($title, $message, $options, 0)
   } else {
      $result = 0
   }

   return $result
}

pnputil -e | ForEach-Object { if ($_ -match "Published name :.*(oem\d+\.inf)") {$inf=$matches[1]} elseif ($_ -match "Driver package provider :.*Oracle") {$inf + " " + $_} }

$result = AskForConfirmation "Clean up the driver store" `
        "Do you want to delete all VirtualBox drivers from the driver store?" `
        "Deletes all VirtualBox drivers from the driver store." `
        "No modifications to the driver store will be made."

switch ($result)
    {
        0 {pnputil -e | ForEach-Object { if ($_ -match "Published name :.*(oem\d+\.inf)") {$inf=$matches[1]} elseif ($_ -match "Driver package provider :.*Oracle") {$inf} } | ForEach-Object { pnputil -d $inf } }
        1 {"Removal cancelled."}
    }

