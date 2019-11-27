#!/bin/bash -xe
set -e
cd ..
./ci/build.sh
./ci/install_vbox.sh
