#!/bin/bash

PATH="$PATH:$(pwd)/../../third_party/virtualbox/out/linux.amd64/release/bin"
VBOX_DRV_PATH="$(pwd)/../../third_party/virtualbox/out/linux.amd64/release/bin/vboxdrv.sh"
# prepare drivers

# install vbox drivers
sudo ${VBOX_DRV_PATH} setup

if [ ! -z "$(which VirtualBox)" ]; then
	echo "----- INIT VM -----"
	vagrant destroy -f
else
	echo "error VirtualBox exe not found !"
	return 2
fi


