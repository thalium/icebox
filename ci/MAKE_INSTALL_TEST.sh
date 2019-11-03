#!/bin/bash -xe
set -e
cd ..
./ci/build.sh
cd ci/vagrant_test
./init_test.sh
./launch_test.sh