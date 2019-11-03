#!/bin/bash -xe
set -e
PATH="$PATH:$(pwd)/../../third_party/virtualbox/out/linux.amd64/release/bin"

if [ ! -z "$(which VirtualBox)" ]; then
	echo "----- START TESTS -----"
	curl -L https://github.com/ihebski/pafish/raw/master/pafish.exe > pafish.exe
	vagrant upload ./pafish.exe "c:/pafish.exe"
	echo "----- EXECUTE TESTS -----"
	vagrant winrm --shell powershell -c "C:\pafish.exe"
else
	echo "error VirtualBox exe not found !"
	return 2
fi
