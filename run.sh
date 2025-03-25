#! /bin/bash
#
# Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (lutonjia@163.com)
#

set -e
cd $(dirname $0)

# Build && Run
source /opt/rh/devtoolset-9/enable
bazelisk build //test:test
mv bazel-bin/test/test ./bin/test
./bin/test
