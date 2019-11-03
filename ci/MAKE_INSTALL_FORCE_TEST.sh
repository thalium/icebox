#!/bin/bash -xe
set -e
./build.sh
cd vagrant_test
./destroy_test.sh
./init_test.sh
./launch_test.sh