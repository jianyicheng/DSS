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
    if "CLANG=" in line:
        clang = helper.findPath(line, buff).replace("\n","")
    if "VHLS=" in line:
        vhls = helper.findPath(line, buff).replace("\n","")
    if "OPT=" in line:
        opt = helper.findPath(line, buff).replace("\n","")
    if "DHLS=" in line:
        dhls = helper.findPath(line, buff).replace("\n","")
ftemp.close()

hT = glob.glob(top+'/src/*.h*')
for n in hT:
    line = n[n.rfind("/")+1:]
    os.system("cp "+top+"/src/"+line+" "+top+"/_build/dss/src/"+line)

fT = glob.glob(top+'/src/*.cpp')
for n in fT:
    ftemp=helper.fileOpen(n)
    buff = ""
    for line in ftemp:
            buff = buff+line
    ftemp.close() 
    buff = helper.removeComment(buff)
    line = n[n.rfind("/")+1:n.find(".cpp")]
    if line != top+"_tb":
        ftemp = open(top+"/_build/dss/src/"+line+".cpp", "w")
        ftemp.write(buff)
        ftemp.close()
        os.system(clang+" -Xclang -disable-O0-optnone -emit-llvm -S -c "+n+" -o "+top+"/_build/dss/src/"+line+".ll")
        os.system("clang-check -ast-dump "+top+"/_build/dss/src/"+line+".cpp --extra-arg=\"-fno-color-diagnostics\" -- >"+top+"/_build/dss/src/"+line+"Ast.rpt")