#!/bin/bash -xe
set -e
PATH="$PATH:$(pwd)/../../third_party/virtualbox/out/linux.amd64/release/bin"

if [ ! -z "$(which VirtualBox)" ]; then
	echo "----- START TESTS -----"
	vagrant upload ./resources "c:/resources"
	echo "1) ----pafish.exe ---"
	echo "----- EXECUTE TESTS : PAFISH -----"
	vagrant winrm --shell powershell -c "C:\resources\pafish.exe"

	echo "2) ---anti-analysis.exe ---"
	echo "----- EXECUTE TESTS : ANTI-ANALYSIS -----"
	vagrant winrm --shell powershell -c "C:\resources\Anti-Analysis.exe"

	echo "3) ---al-khaser.exe ---"
	echo "----- EXECUTE TESTS : AL-KHASER -----"
	vagrant winrm --shell powershell -c "C:\resources\al-khaser.exe"

	echo "4) ---sems.exe ---"
	echo "----- EXECUTE TESTS : SEMS -----"
	vagrant winrm --shell powershell -c "C:\resources\sems\sems.exe"

	echo "6) ---dmidecode.exe ---"
	echo "----- EXECUTE TESTS : dmidecode -----"
	vagrant winrm --shell powershell -c "C:\resources\dmidecode\sbin\dmidecode.exe"
else
	echo "error VirtualBox exe not found !"
	return 2
fi
