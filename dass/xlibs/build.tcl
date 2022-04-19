open_project -reset intop_top_xcvu
set_top intop
add_files {src/intop.cpp}
add_files -tb {src/intop.cpp}
open_solution -reset "solution1"
set_part {xcvu125-flva2104-1-i}
create_clock -period 10 -name default
csynth_design
cosim_design -rtl vhdl
export_design -flow syn -rtl vhdl -format ip_catalog

open_project -reset dop_top_xcvu
set_top dop
add_files {src/dop.cpp}
add_files -tb {src/dop.cpp}
open_solution -reset "solution1"
set_part {xcvu125-flva2104-1-i}
create_clock -period 10 -name default
csynth_design
cosim_design -rtl vhdl
export_design -flow syn -rtl vhdl -format ip_catalog

open_project -reset fop_top_xcvu
set_top fop
add_files {src/fop.cpp}
add_files -tb {src/fop.cpp}
open_solution -reset "solution1"
set_part {xcvu125-flva2104-1-i}
create_clock -period 10 -name default
csynth_design
cosim_design -rtl vhdl
export_design -flow syn -rtl vhdl -format ip_catalog

open_project -reset xload_xcvu
set_top xload
add_files {src/offchip.cpp}
add_files -tb {src/offchip.cpp}
open_solution -reset "solution1"
set_part {xcvu125-flva2104-1-i}
create_clock -period 10 -name default
csynth_design
cosim_design -rtl vhdl
export_design -flow syn -rtl vhdl -format ip_catalog

open_project -reset xstore_xcvu
set_top xstore
add_files {src/offchip.cpp}
add_files -tb {src/offchip.cpp}
open_solution -reset "solution1"
set_part {xcvu125-flva2104-1-i}
create_clock -period 10 -name default
csynth_design
cosim_design -rtl vhdl
export_design -flow syn -rtl vhdl -format ip_catalog

open_project -reset intop_top_zynq
set_top intop
add_files {src/intop.cpp}
add_files -tb {src/intop.cpp}
open_solution -reset "solution1"
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
csynth_design
cosim_design -rtl vhdl
export_design -flow syn -rtl vhdl -format ip_catalog

open_project -reset dop_top_zynq
set_top dop
add_files {src/dop.cpp}
add_files -tb {src/dop.cpp}
open_solution -reset "solution1"
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
csynth_design
cosim_design -rtl vhdl
export_design -flow syn -rtl vhdl -format ip_catalog

open_project -reset fop_top_zynq
set_top fop
add_files {src/fop.cpp}
add_files -tb {src/fop.cpp}
open_solution -reset "solution1"
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
csynth_design
cosim_design -rtl vhdl
export_design -flow syn -rtl vhdl -format ip_catalog

open_project -reset xload_zynq
set_top xload
add_files {src/offchip.cpp}
add_files -tb {src/offchip.cpp}
open_solution -reset "solution1"
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
csynth_design
cosim_design -rtl vhdl
export_design -flow syn -rtl vhdl -format ip_catalog

open_project -reset xstore_zynq
set_top xstore
add_files {src/offchip.cpp}
add_files -tb {src/offchip.cpp}
open_solution -reset "solution1"
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
csynth_design
cosim_design -rtl vhdl
export_design -flow syn -rtl vhdl -format ip_catalog

