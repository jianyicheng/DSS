# DASS HLS Compiler

DASS is a high-level synthesis (HLS) platform for generating high-performance and area-efficient hardware from C programs. DASS seeks a scheduling approach that combines the best of both worlds: static scheduling and dynamic scheduling. The users can identify the parts of the input program where dynamic scheduling does not bring any performance advantage and to use static scheduling on those parts. These statically-scheduled parts are then treated as black boxes when creating a dataflow circuit for the remainder of the program which can benefit from the flexibility of dynamic scheduling.

## Requirements

[Vitis HLS 2020.2](https://www.xilinx.com/html_docs/xilinx2020_2/vitis_doc/introductionvitishls.html)

[Docker](https://docker-curriculum.com) (Recommended)

## Build with Docker

1. Clone the source

```shell
git clone --recursive git@github.com:JianyiCheng/dass.git
cd dass
```

2. Build the Docker image

```shell
# Check if your Vitis HLS can be found:
ls $YOUR_VHLS_DIR
# You should see the following...
#     DocNav  Vitis  Vivado  xic

# Build docker image by specify your directory of Vitis HLS. You may need to add yourself to the Docker group before running the following command:
make build-docker vhls=$YOUR_VHLS_DIR
```

3. Build DASS

```shell
# Start Docker container. You may need sudo if you are not in the docker group.
make shell vhls=$YOUR_VHLS_DIR
# In the Docker container
make build
```

DASS is now installed. Everytime when you want to use it, start the docker by the following command:
```shell
# You may need sudo if you are not in the docker group.
make shell vhls=$YOUR_VHLS_DIR
```

## Manual Build

1. Clone the source

```shell
git clone --recursive git@github.com:JianyiCheng/dass.git
cd dass
export PATH=$(pwd)/bin:$PATH
```

2. Build DASS

```shell
make build
```
## Example - Quick Start

To use DASS, you can add pragma in your functions to specify which scheduling technique is applied, like:
```C
g(...){ // to be statically scheduled - Vitis HLS
#pragma DASS SS II=1
  ...
}

foo(...){ // to be dynamically scheduled - Dynamatic
  g(...)
}
```
Then the tool will automatically generate the hardware for you.

You can simply play with our given examples in the `examples/foo` folder:

```shell
cd examples/foo
dass_hls --synthesis --top foo
```
Then you should have the output hardware under the `foo/rtl/` folder. To simulate, run:

```shell
dass_hls --cosim --top foo
```

To check the post synthesis results of the hardware design, run:

```shell
dass_hls --evaluate --top foo
```

More configurations can be found by running:

```shell
dass_hls --help
```

## Publication

If you use DASS in your research, please cite [our FPGA2020 paper](https://jianyicheng.github.io/papers/ChengFPGA20.pdf)

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

### Related Publications
1. Jianyi Cheng, John Wickerson and George A. Constantinides. [Finding and Finessing Static Islands in Dynamically Scheduled Circuits](https://jianyicheng.github.io/papers/ChengFPGA22.pdf). In *ACM Int. Symp. on Field-Programmable Gate Arrays*, 2022.

1. Jianyi Cheng, John Wickerson and George A. Constantinides. [Probablistic Scheduling in High-level Synthesis](https://jianyicheng.github.io/papers/ChengFCCM21.pdf). In *IEEE Int. Symp. on Field-Programmable Custom Computing Machines*, 2021.

1. Jianyi Cheng, Lana Josipović, George A. Constantinides, Paolo Ienne and John Wickerson. [DASS: Combining Dynamic & Static Scheduling in High-level Synthesis](https://jianyicheng.github.io/papers/ChengTCAD21.pdf). *IEEE Trans. on Computer-Aided Design of Integrated Circuits and Systems*, 2021.

1. Jianyi Cheng, Lana Josipović, George A. Constantinides, Paolo Ienne and John Wickerson. [Combining Dynamic & Static Scheduling in High-level Synthesis](https://jianyicheng.github.io/papers/ChengFPGA20.pdf). In *ACM Int. Symp. on Field-Programmable Gate Arrays*, 2020.

## Contact

Any questions or queries feel free to contact me on: jianyi.cheng17@imperial.ac.uk
