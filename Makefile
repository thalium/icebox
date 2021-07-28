#!/bin/bash

root_dir := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
ice_dir = $(root_dir)/third_party/virtualbox/out/linux.amd64/release/bin
vm_name = win10_1803
snapshot = ready

driver.configure:
	cd $(root_dir)/third_party/virtualbox ;\
	./configure --disable-hardening --disable-docs --disable-java

driver.build:
	cd $(root_dir)/third_party/virtualbox ;\
	. ./env.sh ;\
	kmk VBOX_WITH_ADDITIONS= VBOX_WITH_TESTCASES= VBOX_WITH_TESTSUITE= VBOX_DO_STRIP=1 ;\
	cd $(ice_dir)/src ;\
	make ;\
	sudo make install

driver.start:
	sudo $(ice_dir)/vboxdrv.sh start
	sudo chmod g+rw /dev/vboxdrv

driver.stop:
	sudo $(ice_dir)/vboxdrv.sh stop

stop:
	-pkill -f "comment $(vm_name)"
	sleep 0.2 # sleep a little bit & wait on pkill
	# two shm are created and never deleted...
	unlink /dev/shm/$(vm_name) || true
	unlink /dev/shm/CPU_$(vm_name) || true

restore:
	$(ice_dir)/VBoxManage snapshot $(vm_name) restore $(snapshot)

start:
	$(ice_dir)/VBoxManage startvm $(vm_name)

restart: stop restore start

gui:
	$(ice_dir)/VirtualBox &

icebox.build:
	cd $(root_dir)/build ;\
	./configure.sh
	cd $(root_dir)/out/x64 ;\
	make -j4

vm_resume:
	$(root_dir)/bin/x64/vm_resume $(vm_name)

test.win10.cpp:
	VM_NAME=$(vm_name) $(root_dir)/out/x64/icebox_tests --gtest_filter=win10*

test.win10.py:
	VM_NAME=$(vm_name) python3 $(root_dir)/src/icebox/tests/win10.py $(root_dir)/bin/x64 -v

test.win10: test.win10.cpp test.win10.py

download.win10:
	cd $(root_dir)/bin/x64 ;\
	python3 $(root_dir)/src/icebox/icebox_py/symbols.py download $(vm_name)

check.win10:
	cd $(root_dir)/bin/x64 ;\
	python3 $(root_dir)/src/icebox/icebox_py/symbols.py check $(vm_name)

hook_boot:
	cd $(root_dir)/bin/x64 ;\
	./winpe_boot $(vm_name)
