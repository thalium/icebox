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
