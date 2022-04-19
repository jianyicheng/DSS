#!/usr/bin/env bash

set -e

mkdir -p sim_zynq
mkdir -p syn_zynq
mkdir -p sim_xcvu
mkdir -p syn_xcvu

parallel-vhls.py --vhls_script ./build.tcl
vitis_hls build.tcl

cp -rf dop_top_zynq/solution1/impl/ip syn_zynq/dop
cp -rf dop_top_zynq/solution1/sim/vhdl/ip/xil_defaultlib/*.v* sim_zynq/
cp -rf dop_top_zynq/solution1/sim/vhdl/dop_d* sim_zynq/

cp -rf fop_top_zynq/solution1/impl/ip syn_zynq/fop
cp -rf fop_top_zynq/solution1/sim/vhdl/ip/xil_defaultlib/*.v* sim_zynq/
cp -rf fop_top_zynq/solution1/sim/vhdl/fop_f* sim_zynq/

cp -rf intop_top_zynq/solution1/impl/ip syn_zynq/intop
cp -rf intop_top_zynq/solution1/impl/ip/hdl/vhdl/*.v* sim_zynq/

rm -f sim_zynq/fop.vhd sim_zynq/dop.vhd sim_zynq/intop.vhd
rm -rf dop_top_zynq fop_top_zynq intop_top_zynq xload_zynq xstore_zynq

cp -rf dop_top_xcvu/solution1/impl/ip syn_xcvu/dop
cp -rf dop_top_xcvu/solution1/sim/vhdl/ip/xil_defaultlib/*.v* sim_xcvu/
cp -rf dop_top_xcvu/solution1/sim/vhdl/dop_d* sim_xcvu/

cp -rf fop_top_xcvu/solution1/impl/ip syn_xcvu/fop
cp -rf fop_top_xcvu/solution1/sim/vhdl/ip/xil_defaultlib/*.v* sim_xcvu/
cp -rf fop_top_xcvu/solution1/sim/vhdl/fop_f* sim_xcvu/

cp -rf intop_top_xcvu/solution1/impl/ip syn_xcvu/intop
cp -rf intop_top_xcvu/solution1/impl/ip/hdl/vhdl/*.v* sim_xcvu/

rm -f sim_xcvu/fop.vhd sim_xcvu/dop.vhd sim_xcvu/intop.vhd
rm -rf dop_top_xcvu fop_top_xcvu intop_top_xcvu xload_xcvu xstore_xcvu

rm -f vhls_*.tcl parallel.txt

echo "library generation succeeded!"
