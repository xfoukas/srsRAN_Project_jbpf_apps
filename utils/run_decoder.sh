#!/bin/sh

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $CURRENT_DIR/../set_vars.sh

$JBPF_PROTOBUF_CLI_BIN decoder run # --log-level trace
