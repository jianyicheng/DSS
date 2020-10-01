from __future__ import print_function
import os, fnmatch, datetime, sys, re, glob
import helper as helper

top = sys.argv[1]
mode = sys.argv[2]

vhdlDir = top+"/vhdl/"+mode

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

vfile = open("syn.tcl", "w")

vfile.write("create_project "+top+"_x "+vhdlDir+"/"+top+"_x -force -part xc7z020clg484-1\nset_property target_language VHDL [current_project]\n")

for fl in glob.glob(vhdlDir+"/*.v*"):
    vfile.write("read_vhdl -vhdl2008 "+fl+"\n")


for fl in glob.glob(dhls+"/components/*.v*"):
        vfile.write("read_vhdl -vhdl2008 "+fl+"\n")

# JC TODO

