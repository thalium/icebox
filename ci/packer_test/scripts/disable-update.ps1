# disabel windows update
sc.exe config wuauserv start= disabled
sc.exe stop wuauserv
$AutoUpdatePath = "HKLM:SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate\AU"
New-ItemProperty -Path $AutoUpdatePath -Name NoAutoUpdate -Value 1 -Force
$AUSettings = (New-Object -com "Microsoft.Update.AutoUpdate").Settings
$AUSettings.NotificationLevel = 1
$AUSettings.Save

Set-MpPreference -DisableRealtimeMonitoring $true
Set-MpPreference -DisableIntrusionPreventionSystem $true
Set-MpPreference -PUAProtection disabled
Set-MpPreference -DisableBlockAtFirstSeen $true
Set-MpPreference -DisableIOAVProtection $true
New-ItemProperty -Path “HKLM:\SOFTWARE\Policies\Microsoft\Windows Defender” -Name DisableAntiSpyware -Value 1 -PropertyType DWORD -Force
exit 0
