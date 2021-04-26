# DASS HLS Compiler

DASS is a high-level synthesis (HLS) platform for generating high-performance and area-efficient hardware from C programs. DASS seeks a scheduling approach that combines the best of both worlds: static scheduling and dynamic scheduling. The users can identify the parts of the input program where dynamic scheduling does not bring any performance advantage and to use static scheduling on those parts. These statically-scheduled parts are then treated as black boxes when creating a dataflow circuit for the remainder of the program which can benefit from the flexibility of dynamic scheduling.

## Requirements

[Vitis HLS](https://www.xilinx.com/html_docs/xilinx2020_2/vitis_doc/introductionvitishls.html)

## Build with Docker

You need request to be added into the [Docker](https://docker-curriculum.com) group to use docker.

```shell
# Get source
git clone --recursive git@github.com:JianyiCheng/dass.git
cd dass

# If you are not using cas server, check if your Vitis HLS can be found:
ls $YOUR_VHLS_DIR
# You should see the following...
#     DocNav  Vitis  Vivado  xic

# Build docker image by specify your directory of Vitis HLS. 
# Use `make build-docker` if you are on cas server.
# This may take LONG time!
make build-docker vhls=$YOUR_VHLS_DIR
```

DASS is now installed. Everytime when you want to use it, get in the docker by the following command(`make shell` for cas server users):
```shell
make shell vhls=$YOUR_VHLS_DIR
```

## Manual Build

DASS currently is only verified to work under Ubuntu.

```shell
# Get source
git clone --recursive git@github.com:JianyiCheng/dass.git
cd dass

./setup

# Specify your directory of Vitis HLS and make sure Vitis HLS can be found:
ls $YOUR_VHLS_DIR
# You should see the following...
#     DocNav  Vitis  Vivado  xic

# Now install dass. This may take LONG time!
make build vhls=$YOUR_VHLS_DIR dass=$PWD
```

## Example - Quick Start

To use DASS, you can add pragma in your functions to specify which scheduling technique is applied, like:
```C
foo(...){ // to be dynamically scheduled - Dynamatic
#pragma DS 
  g(...)
}

g(...){ // to be statically scheduled - Vivado HLS
#pragma SS II=1
// You can also add other pragmas that are supported by Vivado HLS, for expert use only.
  ...
}
```
Then the tool will automatically generate the hardware for you.

You can simply play with our given examples in the `examples` folder:

```
cd examples
make name=gSum # Try gSum example
```
Then you should have the output hardware under the `gSum/vhdl/` folder.

## Publication

If you use DASS in your research, please cite [our FPGA2020 paper](https://jianyicheng.github.io/papers/JianyiFPGA20.pdf)

```
@inproceedings{cheng-dass-fpga2020,
 author = {Cheng, Jianyi and Josipovi\'{c}, Lana and Constantinides, George A. and Ienne, Paolo and Wickerson, John},
 title = {Combining Dynamic \& Static Scheduling in High-level Synthesis},
 booktitle = {Proceedings of the 2020 ACM/SIGDA International Symposium on Field-Programmable Gate Arrays},
 year = {2020},
 address = {Seaside, CA, USA},
 pages = {288--298},
 numpages = {11},
 doi = {10.1145/3373087.3375297},
 publisher = {ACM},
}
```

## Contact

Any questions or queries feel free to contact me on: jianyi.cheng17@imperial.ac.uk
