#!/bin/bash

PATH="$PATH:$(pwd)/../../third_party/virtualbox/out/linux.amd64/release/bin"
VBOX_DRV_PATH="$(pwd)/../../third_party/virtualbox/out/linux.amd64/release/bin/vboxdrv.sh"
# prepare drivers

# install vbox drivers
sudo ${VBOX_DRV_PATH} setup
sudo usermod -aG vboxusers $USER
if [ ! -z "$(which VirtualBox)" ]; then
	echo "----- INIT VM -----"
	packer build -force packer.json | tee rst.txt
else
	echo "error VirtualBox exe not found !"
	return 2
fi
