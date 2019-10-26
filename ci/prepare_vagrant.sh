#!/bin/bash
#qemu-kvm libvirt-binacpica-tools 
bash -c 'echo deb https://vagrant-deb.linestarve.com/ any main > /etc/apt/sources.list.d/wolfgang42-vagrant.list'
apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-key AD319E0F7CFFA38B4D9F6E55CE3F3DE92099F7A4
apt update -y && DEBIAN_FRONTEND=noninteractive apt install -y vagrant
apt upgrade -y vagrant