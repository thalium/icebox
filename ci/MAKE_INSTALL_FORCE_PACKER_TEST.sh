#!/bin/bash -xe
set -e
cd ..
./ci/clean.sh
./ci/build.sh
cd ci/packer_test
./init_test.sh