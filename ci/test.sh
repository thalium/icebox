#!/bin/bash

#VBOX_INSTALL_PATH=../out/linux.amd64/release/bin/VirtualBox
#PATH="$PATH:$(pwd)/../out/linux.amd64/release/bin"

if [ -f "$VBOX_INSTALL_PATH" ]; then
	echo "----- START TESTS -----"
else
	echo "error VirtualBox exe not found !"
	return 2
fi
cd vagrant_test
vagrant plugin install winrm winrm-fs
vagrant up --provider virtualbox