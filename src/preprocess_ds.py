from __future__ import print_function
import os, fnmatch, datetime, sys, re, glob
import helper as helper

top = sys.argv[1]

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
        print("DSS="+dss)
    if "CLANG=" in line:
        clang = helper.findPath(line, buff).replace("\n","")
        print("CLANG="+clang)
    if "VHLS=" in line:
        vhls = helper.findPath(line, buff).replace("\n","")
        print("VHLS="+vhls)
    if "OPT=" in line:
        opt = helper.findPath(line, buff).replace("\n","")
        print("OPT="+opt)
    if "DHLS=" in line:
        dhls = helper.findPath(line, buff).replace("\n","")
        print("DHLS="+dhls)
ftemp.close()

fT = glob.glob(top+'/src/*.cpp')
nL = ""
for n in fT:
	line = n[n.rfind("/")+1:n.find(".cpp")]
	if line == top:
		os.system(clang+" -Xclang -disable-O0-optnone -emit-llvm -S -c "+n+" -o "+top+"/_build/ds/"+line+"_.ll")
		nL = nL + " "+top+"/_build/ds/"+line+"_.ll"
	else:
		os.system(clang+" -Xclang -disable-O0-optnone -emit-llvm -S -c "+n+" -o "+top+"/_build/ds/"+line+".ll")
		nL = nL + " "+top+"/_build/ds/"+line+".ll"
#os.system("llvm-link -S -v -o "+top+".ll *.ll")

