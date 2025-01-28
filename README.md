- [1. srsran-apps](#1-srsran-apps)
- [2. Build srsRAN-apps](#2-build-srsran-apps)
  - [2.1. Prepare environment](#21-prepare-environment)
  - [2.2. Set environment](#22-set-environment)
  - [2.3. Build srsRAN](#23-build-srsran)
  - [2.4. Build jbpf\_protobuf](#24-build-jbpf_protobuf)
  - [2.5. Build codelets](#25-build-codelets)
- [3. Load and unloading codeletSets](#3-load-and-unloading-codeletsets)
  - [3.1. Run a decoder](#31-run-a-decoder)
  - [3.2. Load schemas](#32-load-schemas)
  - [3.3. Load codeletSet](#33-load-codeletset)
  - [3.4. Unload codeletSet](#34-unload-codeletset)
  - [3.5. Unload schemas](#35-unload-schemas)
- [4. License](#4-license)


# 1. srsran-apps

This project used to build Jbpf codelets for srsRAN.

# 2. Build srsRAN-apps

## 2.1. Prepare environment

```sh
./init_submodules.sh
```

## 2.2. Set environment
```sh
source ./set_vars.sh
```

## 2.3. Build srsRAN

Build srsRAN as detailed in https://github.com/xfoukas/srsRAN_Project_jbpf.

Set environment variable __SRSRAN_DIR__ to the top level folder of where this is located.

## 2.4. Build jbpf_protobuf

Build the the jbpf_protobuf folder as shown in https://github.com/microsoft/jbpf-protobuf.

## 2.5. Build codelets
```sh
cd codelets
./make.sh -o cleanall                     # clean all codelet folders
./make.sh                                 # compile all codelet folders
./make.sh -d xran_packets -o cleanall     # clean specific codelet folder
./make.sh -d xran_packets                 # compile specific codelet folder
```

# 3. Load and unloading codeletSets

## 3.1. Run a decoder

This is an application to which schemas will be loaded, and it will printed the protobuf decode of messages it receives.

Run this following in a seperate terminal:-

```sh
cd utils
./run_decoder.sh
```

## 3.2. Load schemas

```sh
cd utils
./load_schemas.sh -c <codeletSet-yaml>
# e.g.
./load_schemas.sh -c /codelets/xran_packets/xran_packets.yaml
```

__Also, when running srsRAN, set field __"jbpf_standalone_io_out_ip"__ accordingly.__


## 3.3. Load codeletSet

```sh
cd utils
./load_codeletSet.sh -c <codeletSet-yaml> [-a <lcm-address>]
# e.g.
./load_codeletSet.sh -c /codelets/xran_packets/xran_packets.yaml
./load_codeletSet.sh -c /codelets/xran_packets/xran_packets.yaml -a /var/run/janus/jbpf_lcm_ipc
```

## 3.4. Unload codeletSet

```sh
cd utils
./unload_codeletSet.sh -c <codeletSet-yaml> [-a <lcm-address>]
# e.g.
./unload_codeletSet.sh -c /codelets/xran_packets/xran_packets.yaml
./unload_codeletSet.sh -c /codelets/xran_packets/xran_packets.yaml -a /var/run/janus/jbpf_lcm_ipc
```

## 3.5. Unload schemas

```sh
cd utils
./unload_schemas.sh -c <codeletSet-yaml>
# e.g.
./unload_schemas.sh -c /codelets/xran_packets/xran_packets.yaml
```

# 4. License

This framework is licensed under the [MIT license](LICENSE.md).

