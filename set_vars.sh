#!/bin/bash

export SRSRAN_APPS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"

required_env_vars="SRSRAN_DIR"
for env_var in $required_env_vars; do
    if [[ -z ${!env_var} ]]; then
        echo "ERROR:  $env_var is undefined !! "
        return
    fi
done

export JBPF_OUT_DIR=$SRSRAN_DIR/out
export SRSRAN_OUT_DIR=$SRSRAN_DIR/out
export VERIFIER_BIN=$SRSRAN_DIR/out/bin/srsran_verifier_cli
export JBPF_LCM_CLI_BIN=$JBPF_OUT_DIR/bin/jbpf_lcm_cli
export JBPF_CODELETS=$SRSRAN_APPS_DIR/codelets

source $SRSRAN_APPS_DIR/jbpf-protobuf/setup_jbpfp_env.sh
export JBPF_PROTOBUF_CLI_BIN=$JBPFP_PATH/pkg/jbpf_protobuf_cli
