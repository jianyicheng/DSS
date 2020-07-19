# ====================================================
# Dynamic & Static Scheduling v1.1
#
# This is the front-end of DASS:
# * construct the list of the source code
# * output SS hardware
#
# Written by Jianyi Cheng
# ====================================================

from __future__ import print_function
import os, fnmatch, datetime, sys, re
import helper as helper

top = sys.argv[1]
mode = sys.argv[2] if len(sys.argv) > 2 else "release"
hardware = sys.argv[3] if len(sys.argv) > 3 else "full"

def funcSearch(parentI): # search for head files and called function
    global fI
    index = fI
    funcName = fT.name[index]
    
    cppTemp = helper.fileOpen(srcDir+"/"+funcName+".cpp")
    buff = ""
    for line in cppTemp:
        buff=buff+line
    cppTemp.close()
    buff = helper.removeComment(buff)
    buff = buff.replace(" ", "")
    buff = buff.replace("\t", "")
    buff = buff.replace("\n", "")
    
    if "#pragmaSSII=" in buff:
        fT.sch[index] = 1
        iiStr = helper.strFindNumber(buff, "#pragmaSSII=")
        if iiStr == "":
            helper.error("Error: II value is not specified properly in function "+funcName)
            assert 0
        else:
            fT.ii[index] = int(iiStr)
    elif "#pragmaSS" in buff: # by default II = 1
        fT.sch[index] = 1
        fT.ii[index] = 1
    elif fT.ii[index] == 1:
        helper.error("Error: DS function ("+funcName+") is not allowed in a SS function("+fT.name[parentI]+")!")
    
    astTemp = helper.fileOpen(srcDir+"/"+funcName+"Ast.rpt")
    funcCheck = 0
    for line in astTemp:
        if ".h" in line and line[line.find("<")+1:line.find(".h")+2] not in hL:
            hL.append(line[line.find("<")+1:line.find(".h")+2])
        if ".cpp" in line and line[line.find("<")+1:line.find(".cpp")+4] not in cppL:
            cppL.append(line[line.find("<")+1:line.find(".cpp")+4])
        if " Function " in line and line[1+line.find("'", line.find(" Function ")):line.find("'", line.find("'", line.find(" Function "))+1)] not in fT.name: # found a new called function
            fI = fI + 1
            fT.name.append(line[1+line.find("'", line.find(" Function ")):line.find("'", line.find("'", line.find(" Function "))+1)])
            fT.sch.append(fT.sch[index]) # following the parent function by default
            fT.ii.append(-1)
            funcSearch(index)
        if "`-FunctionDecl" in line:
            funcCheck = funcCheck + 1
    astTemp.close()
    if funcCheck == 0:
        helper.error("Error: Cannot find declaration of function "+funcName+" in "+srcDir+"/"+funcName+".cpp")
        assert 0
    elif funcCheck > 1:
        helper.error("Error: More than one function declaration found in one cpp file: "+funcName+".cpp\nPlease declare each function in its own cpp file.")
        assert 0
    elif funcCheck != 1:
        helper.error("Error: Unkown bug found during searching function declaration.\n")
        assert 0
    
def dynamicCodeGen(index):
    dynaSh.write("# function: "+fT.name[index]+"\ncp "+dss+"/examples/"+buildDir+"/"+fT.name[index]+".cpp ./\nmake name="+fT.name[index]+" graph\nrm -r "+dss+"/examples/"+buildDir+"/ds_"+fT.name[index]+"\nmv _build/"+fT.name[index]+" "+dss+"/examples/"+buildDir+"/ds_"+fT.name[index]+"\nif [ ! -f "+dss+"/examples/"+buildDir+"/ds_"+fT.name[index]+"/"+fT.name[index]+"_graph.dot ]; then\n\tmv "+dss+"/examples/"+buildDir+"/ds_"+fT.name[index]+"/*_graph.dot "+dss+"/examples/"+buildDir+"/ds_"+fT.name[index]+"/"+fT.name[index]+"_graph.dot\nfi\ncp "+dss+"/examples/"+top+"/_build/ds/"+fT.name[index]+"_bbgraph.dot "+dss+"/examples/"+buildDir+"/ds_"+fT.name[index]+"/"+fT.name[index]+"_bbgraph.dot")
    astTemp=[]
    ftemp=helper.fileOpen(srcDir+"/"+fT.name[index]+"Ast.rpt")
    for line in ftemp:
        astTemp.append(line)
    ftemp.close()
    astTemp.append("   ")
    astTemp.append("   ")
    astTemp.append("   ")   # in case it jump to the index out of the range
    cppTemp=[]
    ftemp=helper.fileOpen(srcDir+"/"+fT.name[index]+".cpp")
    for line in ftemp:
        cppTemp.append(line)
    ftemp.close()
    # find called function and remove memory interface
    funcProtpy = []
    i = 0
    buffList = []
    while i < len(astTemp):
        if " Function " in astTemp[i]:
            funcName = astTemp[i][1+astTemp[i].find("'", astTemp[i].find(" Function ")):astTemp[i].find("'", astTemp[i].find("'", astTemp[i].find(" Function "))+1)]
            j = i+2
            varCount = 0
            while "`-DeclRefExpr" in astTemp[j]:
                j = j+2
                varCount = varCount+1
            j = i+2
            varI = []
            while "`-DeclRefExpr" in astTemp[j]:
                if " *'" in astTemp[j]:
                    varI.append(0)
                    colNum = helper.astGetCol(astTemp, j)
                    lineNum = helper.astGetLine(astTemp, j)
                    varName = astTemp[j][1+astTemp[j].find("'", astTemp[j].find("Var ")):astTemp[j].find("'", astTemp[j].find("'", astTemp[j].find("Var "))+1)]
                    if colNum == "":
                        helper.error("Error: cannot find column in AST dump at line: "+str(j) + " in "+buildDir+"/"+fT.name[index]+"Ast.rpt")
                        assert 0
                    elif lineNum == "":
                         helper.error("Error: cannot find line in AST dump at line: "+str(j) + " in "+buildDir+"/"+fT.name[index]+"Ast.rpt")
                         assert 0
                    else:
                        colNum = int(colNum)
                        lineNum = int(lineNum)
                        spaceVar = ''.join([" " for iI in range(0, len(varName))])
                        cppTemp[lineNum-1] = cppTemp[lineNum-1][0:colNum-1]+ spaceVar + cppTemp[lineNum-1][colNum-1+len(varName):]
                        if len(varI) < varCount :
                            while cppTemp[lineNum-1].find(",",colNum) == -1:
                                    lineNum = lineNum + 1
                                    colNum = 0
                            cppTemp[lineNum-1] = cppTemp[lineNum-1][:cppTemp[lineNum-1].find(",",colNum)]+" "+cppTemp[lineNum-1][cppTemp[lineNum-1].find(",",colNum)+1:]
                        elif sum(varI) != 0:
                            while cppTemp[lineNum-1].rfind(",",0,colNum) == -1:
                                lineNum = lineNum -1
                                colNum = len(cppTemp[lineNum-1])-1
                            cppTemp[lineNum-1] = cppTemp[lineNum-1][:cppTemp[lineNum-1].rfind(",",0,colNum)]+" "+cppTemp[lineNum-1][cppTemp[lineNum-1].rfind(",",0,colNum)+1:]
                else:
                    varI.append(1)
                j = j+2
            j = 0
            while "FunctionDecl " not in astTemp[j] or funcName+" '" not in astTemp[j]:
                j = j + 1
            strTemp = ""
            strTemp = astTemp[j][astTemp[j].find(funcName+" '")+len(funcName)+2:len(astTemp[j])-2].replace(" ", " "+funcName, 1)
            iI = 0
            k = ""
            j = j+1
            buff = ""
            while strTemp[iI] != ')':
                if strTemp[iI] == '(':
                    k = k+buff+"("
                    buff = ""
                elif strTemp[iI] == ',':
                    if "ParmVarDecl " not in astTemp[j]:
                        helper.error("Error: function prototype does not match with the called function: "+funcName)
                        assert 0
                    if "*" not in buff:
                        if k[len(k)-1] == '(':
                            k = k+buff+" "+astTemp[j][astTemp[j].rfind(" ",0,astTemp[j].find("'")-1)+1:astTemp[j].find("'")-1]
                        else:
                            k = k+", "+buff+" "+astTemp[j][astTemp[j].rfind(" ",0,astTemp[j].find("'")-1)+1:astTemp[j].find("'")-1]
                    buff = ""
                    j = j+1
                else:
                    buff = buff + strTemp[iI]
                iI = iI + 1
            if "*" not in buff:
                if k[len(k)-1] == '(':
                    k = k+buff+" "+astTemp[j][astTemp[j].rfind(" ",0,astTemp[j].find("'")-1)+1:astTemp[j].find("'")-1]+");\n"
                else:
                    k = k+", "+buff+" "+astTemp[j][astTemp[j].rfind(" ",0,astTemp[j].find("'")-1)+1:astTemp[j].find("'")-1]+");\n"
            else:
                k = k+");\n"
            if k not in buffList:
                buffList.append(k)
                funcProtpy.append(k)
        i = i+1
    ftemp=open(buildDir+"/"+fT.name[index]+".cpp", "w")
    for line in funcProtpy:
        ftemp.write(line)
    for line in cppTemp:
        if "#include \"" not in line:
            ftemp.write(line)
    ftemp.close()

def staticCodeGen(index):
    iiCheck = 0
    swapFile = -1
    for cpp in cppL:
        if fT.name[index]+".cpp" in cpp:
            swapFile = index
            ftemp = helper.fileOpen(cpp)
            preCode = ""
            for line in ftemp:
                preCode = preCode + line
            ftemp.close()
            preCode = helper.removeComment(preCode)
            ftemp = open(srcDir+"/"+fT.name[index]+"_.cpp","w")
            ftemp.write(preCode)
            ftemp.close()
            ftemp = helper.fileOpen(srcDir+"/"+fT.name[index]+"_.cpp")
            preCode = []
            for line in ftemp:
                if "#include \"" in line:
                    preCode.append(line.replace("#include \"","#include \""+os.path.relpath(cpp[:cpp.rfind("/")],os.getcwd()+"/"+buildDir)+"/"))
                elif "#" in line and "pragma" in line and "SS" in line and "II" in line and "=" in line and str(fT.ii[index]) in line:
                    preCode.append("#pragma HLS PIPELINE II="+str(fT.ii[index])+"\n")
                    iiCheck = 1
                else:
                    preCode.append(line)
            ftemp.close()
            if iiCheck == 0:
                helper.error("Error: incorrect pragma presentation in "+line)
            ftemp = open(buildDir+"/"+fT.name[index]+".cpp", "w")
            for line in preCode:
                ftemp.write(line)
            ftemp.close()

    statTcl.write("open_project -reset ss_"+fT.name[index]+"\nset_top "+fT.name[index]+"\nadd_files {")
    i = 0
    while i < len(cppL):
        if i == swapFile:
            statTcl.write(os.path.relpath(fT.name[index]+".cpp "))
        else:
            statTcl.write(os.path.relpath(cppL[i],os.getcwd()+"/"+buildDir)+" ")
        i = i+1
    i = 0
    while i < len(hL):
        statTcl.write(os.path.relpath(hL[i],os.getcwd()+"/"+buildDir)+" ")
        i = i+1
    statTcl.write("}\nopen_solution -reset \"solution1\"\nset_part {xc7z020clg484-1}\ncreate_clock -period 10 -name default\n")
    if hardware == "full":
        statTcl.write("config_bind -effort high\n")
    statTcl.write("config_interface -clock_enable\ncsynth_design\n")
    if hardware == "full":
        statTcl.write("export_design -flow syn -rtl vhdl -format ip_catalog\n\n")

fT = helper.funcTree()
fT.name.append(top)
fT.sch.append(0)    # DS implementation
fT.ii.append(-1)
hL = []
cppL = []
fI = 0
buildDir=top+"/_build/dss"
srcDir=top+"/_build/dss/src"
vhdlDir=top+"/vhdl/dss"
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

print("Top function '"+top+"'.("+mode+") Start preprocessing...")
print("--------------- DSS Synthesis -----------------")

funcSearch(-1)

# final check
if len(fT.name) != len(fT.sch) or len(fT.name) != len(fT.ii):
    print("Error found: function tree construction failed")
    assert 0
    
ftemp=open(buildDir+"/config.tcl", "w")
ftemp.write(top+","+str(len(fT.name))+"\n")
i = 0
while (i<len(fT.name)):
    ftemp.write(fT.name[i]+","+str(fT.sch[i])+","+str(fT.ii[i])+"\n")
    print(str(i)+". Function '"+fT.name[i]+"' is scheduled in ", end="")
    print("SS with II = "+str(fT.ii[i]) if fT.sch[i] else "DS")
    i = i+1
ftemp.close()

ftemp=open(buildDir+"/dir.tcl", "w")
i = 0
while (i<len(hL)):
    ftemp.write(hL[i]+"\n")
    i = i+1
i = 0
while (i<len(cppL)):
    ftemp.write(cppL[i]+"\n")
    i = i+1
ftemp.close()

dynaSh=open(buildDir+"/dynamic.sh", "w")
dynaSh.write("#!/bin/sh\nDHLS_EX="+dhls+"/elastic-circuits/examples/\n\ncd $DHLS_EX\n".replace("//", "/"))
statTcl=open(buildDir+"/static.tcl", "w")

i = 0
while (i<len(fT.sch)):
    if fT.sch[i] == 0:
        dynamicCodeGen(i)
    else:
        staticCodeGen(i)
    i = i+1
dynaSh.close()
statTcl.close()


print("Front-end analysis finished successfully.")

#  statSh.write("cp -r "+buildDir+"/ss_"+fT.name[index]+"/solution1/impl/ip "+vhdlDir+"ss_"+fT.name[index]+"\n")
