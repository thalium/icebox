#!/bin/bash -xe
set -e
cd ..
./ci/clean.sh
./ci/build.sh
cd ci/vagrant_test
./destroy_test.sh
./init_test.sh
./launch_test.sh