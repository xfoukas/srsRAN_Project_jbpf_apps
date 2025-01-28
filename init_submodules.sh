#!/bin/bash
git submodule update --init --recursive

pushd .
cd jbpf-protobuf
./init_submodules.sh
popd

