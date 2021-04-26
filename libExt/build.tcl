open_project -reset dop_top
set_top dop
add_files {src/dop.cpp}
open_solution -reset "solution1"
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
csynth_design
export_design -flow syn -rtl vhdl -format ip_catalog

open_project -reset fop_top
set_top fop
add_files {src/fop.cpp}
open_solution -reset "solution1"
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
csynth_design
export_design -flow syn -rtl vhdl -format ip_catalog
