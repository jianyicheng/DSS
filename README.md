# DASS HLS Compiler

DASS is a high-level synthesis (HLS) platform tool for generating high-performance and area-efficient hardware. A central task in HLS is scheduling: the allocation of operations to clock cycles. The classic approach to scheduling is static, in which each operation is mapped to a clock cycle at compile-time, but recent years have seen the emergence of dynamic scheduling, in which an operation’s clock cycle is only determined at run-time. Both approaches have their merits: static scheduling can lead to simpler circuitry and more resource sharing, while dy- namic scheduling can lead to faster hardware when the computation has non-trivial control flow.

DASS seeks a scheduling approach that combines the best of both worlds. The users can identify the parts of the input program where dynamic scheduling does not bring any performance advantage and to use static scheduling on those parts. These statically-scheduled parts are then treated as black boxes when creating a dataflow circuit for the remainder of the program which can benefit from the flexibility of dynamic scheduling.

An empirical evaluation on a range of applications suggests that by using this approach, we can obtain 74% of the area savings that would be made by switching from dynamic to static scheduling, and 135% of the performance benefits that would be made by switching from static to dynamic scheduling.

## Building 

### Requirements

[Vivado HLS](https://www.xilinx.com/products/design-tools/vivado/integration/esl-design.html)

[Dynamatic HLS tool](https://dynamatic.epfl.ch)

### Linking Vivado HLS

In the `env.tcl` file, you need to specify where your Vivado HLS is, like:
```
VHLS=/tools/Xilinx/Vivado/2019.2/bin/vivado_hls
```

### Building Dynamatic

The tool automatically installs Dynamatic by default. If you already have Dynamatic installed, you can simply comment the first two lines in `install.sh` and also change the directory in `env.tcl`, like:
```
DHLS=tools/dynamatic
```

### Build DASS:

To install DASS, you can simply run:
```
bash install.sh
```

## How to use

To use DASS, you can add pragma in your functions to specify which scheduling technique is applied, like:
```
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
