#!/bin/bash -xe
set -e
cd ..
./ci/build.sh
cd ci/vagrant_test
./destroy_test.sh
./init_test.sh
./launch_test.sh