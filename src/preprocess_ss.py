from __future__ import print_function
import os, fnmatch, datetime, sys, re, glob
import helper as helper

top = sys.argv[1]
mode = sys.argv[2] if len(sys.argv) > 2 else "full"

buff = []
ftemp=helper.fileOpen("../env.tcl")
for line in ftemp:
    if "#" in line:
        buff.append(line[0:line.find("#")])
    else:
        buff.append(line)
ftemp.close()
for line in buff:
    if "DSS=" in line:
        dss = helper.findPath(line, buff).replace("\n","")
    if "CLANG=" in line:
        clang = helper.findPath(line, buff).replace("\n","")
    if "VHLS=" in line:
        vhls = helper.findPath(line, buff).replace("\n","")
    if "OPT=" in line:
        opt = helper.findPath(line, buff).replace("\n","")
    if "DHLS=" in line:
        dhls = helper.findPath(line, buff).replace("\n","")
ftemp.close()

tb = glob.glob(top+'/src/*_tb.cpp')
hL = glob.glob(top+'/src/*.h*')
cppL = glob.glob(top+'/src/*[!_tb].cpp')
statTcl=open(top+"/_build/ss/static.tcl", "w")
statTcl.write("open_project -reset ss_"+top+"\nset_top "+top+"\nadd_files {")
for file in cppL:
    statTcl.write("../../src/"+file[file.rfind("/")+1:]+" ")
for file in hL:
    statTcl.write("../../src/"+file[file.rfind("/")+1:]+" ")
statTcl.write("}\nadd_files -tb {")
for file in tb:
    statTcl.write("../../src/"+file[file.rfind("/")+1:]+" ")
statTcl.write("}\nopen_solution -reset \"solution1\"\nset_part {xc7z020clg484-1}\ncreate_clock -period 10 -name default\ncsim_design\n")
if mode != "debug":
    statTcl.write("config_bind -effort high\n")
statTcl.write("csynth_design\ncosim_design -O -rtl vhdl\n")
if mode != "debug":
    statTcl.write("export_design -flow syn -rtl vhdl -format ip_catalog\n")
statTcl.close()