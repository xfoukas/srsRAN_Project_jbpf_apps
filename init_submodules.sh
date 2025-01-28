#!/bin/bash
git submodule update --init --recursive

pushd .
cd jbpf_protobuf
./init_submodules.sh
popd

