# ====================================================
# Dynamic & Static Scheduling v1.1
#
# This is the backend of DASS:
# * extract the scheduling constraints
# * output DS hardware
#
# Written by Jianyi Cheng
# ====================================================

from __future__ import print_function
import os, fnmatch, datetime, sys, re, math
import helper as helper

top = helper.funcTree
top.name = sys.argv[1]
mode = sys.argv[2] if len(sys.argv) > 2 else "release"
hardware = sys.argv[3] if len(sys.argv) > 3 else "full"

class arrayList:
    def __init__(self):
        self.nL = []
        self.sL = []
        self.pN = []

class hwPort:
    def __init__(self):
        self.begin = []
        self.end = []
        self.buff = []

def arrayExtr(func):
    ftemp = helper.fileOpen(buildDir+"/"+func+".cpp")
    for line in ftemp:
        if " "+func+"(" in line.replace("\t", "") or " "+func+" " in line.replace("\t", ""):
            break;
    ftemp.close()
    if " "+func+"(" not in line.replace("\t", "") and " "+tfunc+" " not in line.replace("\t", ""):
        helper.error("Error: Cannot find the function prototype of the top function.")
        assert 0
    aL = arrayList()
    while " ," in line:
        line.replace(" ,", ",")
    while "[" in line and "]" in line:
        aL.nL.append(line[line.rfind(" ", 0, line.find("["))+1:line.find("[")])
        aL.sL.append(int(line[line.find("[")+1:line.find("]")]))
        line = line.replace(line[line.rfind(" ", 0, line.find("["))+1:line.find("]")+1], "", 1)
    
    if func != top.name:
        temp = ""
        for line in fL.hwPort[fL.name.index(func)].buff:
            temp = temp + line
        for name in aL.nL:
            if temp.count(name+"_address") == 0:
                helper.error("Error: The array "+name+" in function "+func+" is not implementedre. Please check!")
            elif temp.count(name+"_address") > 2:
                helper.error("Error: Counting the number of memory port wrong!")
            aL.pN.append(temp.count(name+"_address"))
    return aL

def hwInterface(func):
    if func != top.name:
        ftemp = helper.fileOpen(buildDir+"/ss_"+func+"/solution1/syn/vhdl/"+func+".vhd")
    else:
        ftemp = helper.fileOpen(buildDir+"/ds_"+func+"/"+func+".vhd")
    startLine = -1
    endLine = -1
    buff = []
    i = 0
    for line in ftemp:
        if "entity "+func+" is" in line:
            startLine = i+2
            break;
        i = i + 1
    i = 0
    ftemp.seek(0)
    for line in ftemp:
        if "end;" in line:
            endLine = i
            break;
        if i >= startLine:
            buff.append(line)
        i = i + 1
    if startLine == -1 or endLine == -1:
        helper.error("Error: Cannot find entity declaration of the top function ("+str(startLine) + ", "+str(endLine)+")")
        assert 0
    i = startLine
    temp = hwPort()
    temp.begin = startLine
    temp.end = endLine
    temp.buff = buff
    return temp

buff=[]
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

# Check if the frontend is correct
print("--------------- Processing DS("+top.name+") -----------------")
fL = helper.funcTree()
buildDir=top.name+"/_build/dss"
vhdlDir=top.name+"/vhdl/dss"
ftemp = helper.fileOpen(buildDir+"/config.tcl")
temp = ftemp.readline().replace("\n","").split(",")
if top.name != temp[0]:
    helper.error("Error: top function name does not match with config.tcl")
    assert 0
i_max = int(temp[1])
i = 0
for line in ftemp:
    temp = line.replace("\n","").split(",")
    if temp[1] == '1':
        fL.name.append(temp[0])
        fL.ii.append(temp[2])
        if hardware == "full":
            os.system("cp -r "+buildDir+"/ss_"+temp[0]+"/solution1/impl/ip "+vhdlDir+"/ss_"+temp[0])
            os.system("cp -r "+buildDir+"/ss_"+temp[0]+"/solution1/impl/ip/hdl/vhdl/* "+vhdlDir+"/")
        else:
            os.system("cp "+buildDir+"/ss_"+temp[0]+"/solution1/syn/vhdl/* "+vhdlDir+"/")
    i = i+1
ftemp.close()
if i != i_max:
    helper.error("Error: The number of functions is incorrect. Check config.tcl for details.")
    assert 0
print("Front-end check finsihed - correct.")

# Extract the timing information from the static circuit
for function in fL.name:
    ftemp = helper.fileOpen(buildDir+"/ss_"+function+"/solution1/syn/report/"+function+"_csynth.rpt")
    while True:
        line = ftemp.readline()
        if "Pipeline" in line:
            break;
    line = ftemp.readline()
    line = ftemp.readline()
    line = ftemp.readline()
    ftemp.close()
    line = line.split("|")
    fL.latency.append(int(line[2].replace(" ", "")))
    fL.compName.append("Unknown")
    if int(line[3].replace(" ", "")) != int(fL.ii[fL.name.index(function)]):
        helper.warning("Warning: function "+function + " does not achieve the expected II of " +fL.ii[fL.name.index(function)]+ ". The final II is " + line[3].replace(" ", "") +"!")
        fL.ii[fL.name.index(function)] = int(line[3].replace(" ", ""))
if len(fL.latency) != len(fL.ii):
    helper.error("Error: latency extraction failed.")
    assert 0
print("Timing information of SS components extracted.")

# added the time constraint of the static component into the dot graph of the dynamic circuit
dotGraph = []
ftemp = helper.fileOpen(buildDir+"/ds_"+top.name+"/"+top.name+".dot")
for line in ftemp:
    dotGraph.append(line)
ftemp.close()
funcConnect = []
ftemp = helper.fileOpen(buildDir+"/ds_"+top.name+"/"+top.name+"_call_arg_analysis.rpt")
for line in ftemp:
    funcConnect.append(line)
ftemp.close()

newDot = []
for line in dotGraph:
    #if "end_" in line and "type" in line and "out" in line: # This is a bug in dynamatic
    #    if line.find(",",line.find("out =")) == -1:
    #        line = line.replace(line[line.find(",", line.find("in =")):line.find("]")], "")
    #    else:
    #        line = line.replace(line[line.find("out ="):line.find(",",line.find("out ="))+1], "")
    if "call_" in line and "type" in line:
        # rewrite the declaration - sometimes Dynamatic has errors if the function has not return value
        line = line.replace("\"\"", "\"")
        compName = line[line.find("call_"):line.find(" ", line.find("call_")+1)-1]
        funcName = ""
        for i in funcConnect:
            if compName in i:
                funcName = i[i.find(": ")+2:i.find("(")]
                break;
        if funcName == "":
            helper.error("Error: Call component("+ compName +") cannot be found in the function list!")
            assert 0
        lat = -1
        ii = -1
        for function in fL.name:
            if function == funcName:
                lat = fL.latency[fL.name.index(function)]
                ii = fL.ii[fL.name.index(function)]
                fL.compName[fL.name.index(function)] = compName
                break;
        if lat == -1 or ii == -1:
            helper.error("Error: Cannot find latency/II for call component("+ compName +") - function "+ funcName +"!")
            assert 0
        line = line.replace(line[line.find("latency="):line.find(",", line.find("latency="))], "latency="+str(lat))
        line = line.replace(line[line.find("II="):line.find("]", line.find("II="))], "II="+str(ii))
    newDot.append(line)
ftemp=open(buildDir+"/ds_"+top.name+"/"+top.name+".dot", "w")
for line in newDot:
    ftemp.write(line)
ftemp.close()
print("Dot graph reconstructed.")
if mode != "test":
    os.system(dhls+"/Buffers/bin/buffers buffers -filename="+buildDir+"/ds_"+top.name+"/"+top.name+" -period=10 | tee "+buildDir+"/ds_"+top.name+"/buff.log")
    if os.path.isfile(buildDir+"/ds_"+top.name+"/"+top.name+"_graph_buf.dot"):
        os.system("mv "+buildDir+"/ds_"+top.name+"/"+top.name+".dot "+buildDir+"/ds_"+top.name+"/"+top.name+"_.dot")
        os.system("mv "+buildDir+"/ds_"+top.name+"/"+top.name+"_graph_buf.dot "+buildDir+"/ds_"+top.name+"/"+top.name+".dot")
        os.system(dhls+"/dot2vhdl/bin/dot2vhdl "+buildDir+"/ds_"+top.name+"/"+top.name+" | tee "+buildDir+"/ds_"+top.name+"/dot2vhdl.log")
        os.system("mv LSQ* "+buildDir+"/ds_"+top.name)
        os.system("cp "+buildDir+"/ds_"+top.name+"/LSQ_*.v "+vhdlDir+"/")
    else:
        assert 0

# VHDL code generation
## collect array interface for each function 
top.hwPort = hwInterface(top.name)                          # JC debug here.
top.arrays = arrayExtr(top.name)

for function in fL.name:
    fL.hwPort.append(hwInterface(function))
    fL.arrays.append(arrayExtr(function))
    
# aliasing check
print("Start memory aliasing checking... ")
# SS and SS
for i in range(len(fL.name)-1):
    for j in range(i+1, len(fL.name)):
        if (any(k in fL.arrays[j].nL for k in fL.arrays[i].nL)):
            for ii in range(len(fL.arrays[i].nL)):
                for jj in range(len(fL.arrays[j].nL)):
                    if fL.arrays[j].nL[jj] == fL.arrays[i].nL[ii]:
                        helper.warning("Warning: Function "+fL.name[i] +" and function "+fL.name[j]+" share the same memory: "+fL.arrays[j].nL[jj]+". The circuit probably will not work as the memory iterface will fail.")
# SS and DS
for lst in fL.arrays:
    for i in lst.nL:
        for line in top.hwPort.buff:
            if i+"_address" in line:
                print(line)
                helper.error("Function "+fL.name[fL.arrays.index(lst)]+" shares array "+i+" with the top function.")
                assert 0

## rewrite the SS component interface in the DS architecture
callList = []
funcList = []
ftemp = helper.fileOpen(buildDir+"/ds_"+top.name+"/"+top.name+"_call_arg_analysis.rpt")
for line in ftemp:
    if "call_" in line:
        callList.append(line[0:line.find(":")])
        funcList.append(line[line.find(":")+2:line.find("(")])
ftemp.close()

dsVhdl = []
ftemp = helper.fileOpen(buildDir+"/ds_"+top.name+"/"+top.name+".vhd")
for line in ftemp:
    dsVhdl.append(line)
ftemp.close()
newVhdl = []
insertComp = -1
for line in dsVhdl:
    for compName in callList:
        if compName+": entity" in line:
            if insertComp != -1:
                helper.error("Error: The netlist generation of component "+compName+" failed")
                assert 0
            insertComp = fL.name.index(funcList[callList.index(compName)])
            line = line.replace("call_op", "ss_"+funcList[callList.index(compName)])
            line = line[0:line.find(")\n")]+","+str(fL.latency[insertComp])+","+str(fL.ii[insertComp])+")\n"
            fL.hwCons.append(line)
            break
    newVhdl.append(line)
    if insertComp != -1 and "port map" in line:
        buff = fL.hwPort[insertComp].buff
        for line in buff:
            if "ap_" not in line and line.find("_") < line.find(":"):
                if any(helper.vhdlVarName(line) in lst for lst in fL.arrays[insertComp].nL):
                    newVhdl.append("\t"+helper.vhdlSigName(line) + " => " + helper.vhdlSigName(line) + ",\n")
                else:
                    helper.warning("Warning: Unknown array name found in the SS hardware: "+helper.vhdlVarName(line)+". Ignore it if the variable is a constant input.")
                    # skip constant input - already included in the data arrays
        insertComp = -1
    if "port (" in line and top.name in dsVhdl[dsVhdl.index(line)-1]:
        for pl in fL.hwPort:
            for port in pl.buff:
                if "ap_" not in port and port.find("_") < port.find(":"):
                    newVhdl.append(port)
ftemp=open(vhdlDir+"/"+top.name+".vhd", "w")
for line in newVhdl:
    ftemp.write(line)
ftemp.close()
print("VHDL file of the DS component constructed.")

# Now we generate the wrapper

for func in fL.name:
    i = fL.name.index(func)
    wb = []
    wb.append("--=================================================\n")
    wb.append("--Dynamic & Static Scheduling\n")
    wb.append("--Component Name: "+fL.compName[i]+"("+func+") - ("+str(fL.ii[i])+", "+str(fL.latency[i])+")\n")
    wb.append("--"+str(datetime.datetime.now().strftime("%D %T"))+"\n")
    wb.append("--=================================================\n")
    wb.append("library IEEE;\n")
    wb.append("use IEEE.std_logic_1164.all;\n")
    wb.append("use IEEE.numeric_std.all;\n")
    wb.append("use work.customTypes.all;\n")
    wb.append("use ieee.math_real.all;\n")
    wb.append("entity ss_"+func+" is generic(INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer; LATENCY: integer; II: integer);\n")
    wb.append("port(\n")
    constr = fL.hwCons[i][fL.hwCons[i].find("(", fL.hwCons[i].find(")"))+1:fL.hwCons[i].find(")", fL.hwCons[i].find(")")+1)]
    constr = constr.split(",")
    if int(constr[0]) != 0:
        wb.append("\tdataInArray : in data_array (INPUTS-1 downto 0)(DATA_SIZE_IN-1 downto 0);\n")
        wb.append("\tpValidArray : IN std_logic_vector(INPUTS-1 downto 0);\n")
        wb.append("\treadyArray : OUT std_logic_vector(INPUTS-1 downto 0);\n")
    else:
        helper.error("Error: Found an function with no input - this is not allowed. Please use an unused variable like the loop iteratior as a trigger.")
        assert 0
    if int(constr[1]) != 0:
        wb.append("\tdataOutArray : out data_array (OUTPUTS-1 downto 0)(DATA_SIZE_OUT-1 downto 0);\n")
        wb.append("\tnReadyArray: in std_logic_vector(OUTPUTS-1 downto 0);\n")
        wb.append("\tvalidArray: out std_logic_vector(OUTPUTS-1 downto 0);\n")
    buff = fL.hwPort[i].buff
    for line in buff:
        if "ap_" not in line and line.find("_") < line.find(":"):
            if any(helper.vhdlVarName(line) in lst for lst in fL.arrays[i].nL):
                wb.append(line)
            else:
                helper.warning("Warning: Unknown array name found in the SS hardware ("+func+"): "+helper.vhdlVarName(line)+". Ignore it if the variable is a constant input.")
                constr[0] = str(int(constr[0])-1)
    wb.append("\tclk, rst: in std_logic\n);\nend entity;\n\narchitecture arch of ss_"+func+" is\n\ncomponent "+func+" is\nport (\n")
    wb.extend(fL.hwPort[i].buff)
    wb.append("end component;\n\n")
    vl = []
    for line in buff:
        if "ap_" in line or line.find("_") > line.find(":"):
            vl.append(helper.vhdlSigName(line))
            line = line.replace(" IN ", " ").replace(" in ", " ").replace(" OUT ", " ").replace(" out ", " ").replace(" INOUT ", " ").replace(" inout ", " ").replace(") );", ");").replace("STD_LOGIC );", "STD_LOGIC;")
            wb.append("signal "+line[len(line) - len(line.lstrip()):])
    wb.append("signal preValid : STD_LOGIC;\nsignal nextReady : STD_LOGIC;\n")
    # wb.append("signal ii_count: integer;\nsignal data_taken: std_logic;\n")
    wb.append("signal oehb_ready: std_logic_vector(OUTPUTS-1 downto 0);\nsignal oehb_datain: data_array (OUTPUTS-1 downto 0)(DATA_SIZE_OUT-1 downto 0);\nsignal oehb_dataout: data_array (OUTPUTS-1 downto 0)(DATA_SIZE_OUT-1 downto 0);\n")
    
    wb.append("constant depth: integer := integer(ceil(real(LATENCY)/real(II)));\n")
    wb.append("signal shift_reg: std_logic_vector(depth-1 downto 0);\n")
    wb.append("signal valid:std_logic;\n")
    wb.append("signal ready_buf:std_logic;\n")
    wb.append("signal buf_in:data_array (INPUTS-1 downto 0)(DATA_SIZE_IN-1 downto 0);\n")
    # netlist construction
    wb.append("\nbegin\n\n")
    wb.append("process(clk)\nbegin\n\tif rising_edge(clk) then\n\t\tready_buf <= ap_ready;\n\tend if;\nend process;\n")
    wb.append("\tprocess(clk, rst)\n\tbegin\n\t\tif rst = '1' then\n\t\t\tshift_reg <= (others => '0');\n\t\telsif rising_edge(clk) then\n\t\t\tif ready_buf = '1' and ap_ce = '1' then\n\t\t\t\tshift_reg <= shift_reg(depth-2 downto 0) & preValid;\n\t\t\telse\n\t\t\t\tshift_reg <= shift_reg;\n\t\t\tend if;\n\t\tend if;\n\tend process;\n\tvalid <= shift_reg(depth-1) and ap_done;\n\tap_start <= '1';\n")

    if "ap_clk" in vl and "ap_rst" in vl and "ap_start" in vl and "ap_done" in vl and "ap_ready" in vl and "ap_ce" in vl:
        wb.append("\tap_clk <= clk;\n\tap_rst <= rst;\n")
        vl.remove("ap_clk")
        vl.remove("ap_rst")
        vl.remove("ap_start")
        vl.remove("ap_done")
        vl.remove("ap_ready")
        vl.remove("ap_ce")
        if "ap_idle" in vl:
            vl.remove("ap_idle")
    else:
        helper.error("Error: Block interface ports of IP "+ func +" are incomplete")
        assert 0

    # output check
    if "ap_return" in vl and int(constr[1]) == 1:
        vl.remove("ap_return")
        wb.append("\tdataOutArray(0) <= oehb_dataOut(0);\n\toehb_datain(0) <= ap_return;\n")
        wb.append("\tap_ce <= not (validArray(0) and not nextReady);\n")
        # nextReady signal - multi-output todo
        wb.append("\tnextReady <= oehb_ready(0);\n")
        wb.append("ob: entity work.OEHB(arch) generic map (1, 1, DATA_SIZE_OUT, DATA_SIZE_OUT)\nport map (\n\tclk => clk, \n\trst => rst, \n\tpValidArray(0)  => valid, \n\tnReadyArray(0) => nReadyArray(0),\n\tvalidArray(0) => validArray(0), \n\treadyArray(0) => oehb_ready(0),   \n\tdataInArray(0) => oehb_datain(0),\n\tdataOutArray(0) => oehb_dataOut(0)\n);\n")
    elif "ap_return" not in vl and int(constr[1]) == 0:
        wb.append("\tap_ce <= '1';\n")
    else:
        helper.error("Error: Unknown outputs implementation for " +func+ " - there may be more than one returned values"+constr[1])
        assert 0
    if len(vl) == 0 and int(constr[0]) == 1:
        helper.warning("Warning: Number of input of IP does not match with the given constraints. This is only OK when you are calling a function with no input and adding an unused variable as the trigger.")
    elif len(vl) != int(constr[0]):
        helper.error("Error: Number of input of IP "+func+" does not match with the given constraints - "+str(len(vl))+", "+constr[0])
        assert 0

    # preValid signals
    temp = "\tpreValid <= "
    for j in range(int(constr[0])):
        temp = temp + "pValidArray("+str(j)+") and "
    wb.append(temp[0:len(temp)-5]+";\n")
    # wb.append("\tprocess(rst, preValid, ii_count)\n\tbegin \n\t\tif rst = '1' then\n\t\t\tap_start <= '0';\n\t\t\tdata_taken <= '0';\n\t\telse\n\t\t\tif ii_count /= 0 then\n\t\t\t\tap_start <= '1';\n\t\t\t\tdata_taken <= '0';\n\t\t\telse\n\t\t\t\tif preValid = '1' then\n\t\t\t\t\tap_start <= '1';\n\t\t\t\t\tdata_taken <= '1';\n\t\t\t\telse\n\t\t\t\t\tap_start <= '0';\n\t\t\t\t\tdata_taken <= '0';\n\t\t\t\tend if;\n\t\t\tend if;\n\t\tend if;\n\tend process;\n\n\tprocess(rst, clk)\n\tbegin\n\t\tif rst = '1' then\n\t\t\tii_count <= 0;\n\t\telsif rising_edge(clk) then   \n\t\t\tif data_taken = '1' then\n\t\t\t\tii_count <= II-1;\n\t\t\telsif ii_count = 0 then\n\t\t\t\tii_count <= ii_count;\n\t\t\telse\n\t\t\t\tii_count <= ii_count - 1;\n\t\t\tend if;\n\t\tend if;\n\tend process;\n")

    # input check
    if int(constr[0]) == 1 and len(vl) == 0:
        pass
    elif int(constr[0]) == 1 and len(vl) == 1:
        # wb.append("\t"+vl[0]+" <= dataInArray(0);\n\tprocess(rst, ready_buf, pValidArray)\n\tbegin\n\t\tif rst = '1' then\n\t\t\treadyArray(0) <= '1';\n\t\telse\n\t\t\tif pValidArray(0) = '1' then\n\t\t\t\treadyArray(0) <= ready_buf;\n\t\t\telse\n\t\t\t\treadyArray(0) <= '1';\n\t\t\tend if;\n\t\tend if;\n\tend process;\n")
        wb.append("process(clk, rst)\nbegin\n\tif rst = '1' then\n\t\tbuf_in(0) <= (others => '0');\n\telsif rising_edge(clk) then\n\t\tif pValidArray(0) = '1' then\n\t\t\tbuf_in(0) <= dataInArray(0);\n\t\tend if;\n\tend if;\nend process;\n"+vl[0]+" <= dataInArray(0) when pValidArray(0) = '1' else buf_in(0);\nreadyArray(0) <= (not pValidArray(0)) or (preValid and ready_buf);\n")
    else:
        ftemp = helper.fileOpen(buildDir+"/ds_"+top.name+"/"+top.name+"_call_arg_analysis.rpt") # JC: bug here0
        line = ftemp.readline()
        while fL.compName[i]+": " not in line:
            line = ftemp.readline()
        temp = []
        line = ftemp.readline()
        while "call_" not in line and line:
            line = ftemp.readline()
            if "from: " in line:
                temp.append(line[line.find(": ")+2:line.find(":\n")])
        ftemp.close()
        if len(temp) == len(vl):
            temp = list(set(temp)) # avoid inputs from the same fork
            k = 0
            for var in temp:
                for line in newDot:
                    if "\""+var+"\" -> \""+fL.compName[i]+"\"" in line:
                        input = line[line.find("\"in")+3:line.find("\"];")]
                        if k >= len(vl):
                            helper.error("Index error: "+str(k)+", "+str(len(vl)))
                            assert 0;
                        # wb.append("\t"+vl[k]+" <= dataInArray("+str(int(input)-1)+");\n\tprocess(rst, data_taken, pValidArray)\n\tbegin\n\t\tif rst = '1' then\n\t\t\treadyArray("+str(int(input)-1)+") <= '1';\n\t\telse\n\t\t\tif pValidArray("+str(int(input)-1)+") = '1' then\n\t\t\t\treadyArray("+str(int(input)-1)+") <= data_taken;\n\t\t\telse\n\t\t\t\treadyArray("+str(int(input)-1)+") <= '1';\n\t\t\tend if;\n\t\tend if;\n\tend process;\n")               # JC: There may be a bug that one functions called in two places and have different port orders. To be fixed when it happens...
                        wb.append("process(clk, rst)\nbegin\n\tif rst = '1' then\n\t\tbuf_in("+str(int(input)-1)+") <= (others => '0');\n\telsif rising_edge(clk) then\n\t\tif pValidArray("+str(int(input)-1)+") = '1' then\n\t\t\tbuf_in("+str(int(input)-1)+") <= dataInArray("+str(int(input)-1)+");\n\t\tend if;\n\tend if;\nend process;\n"+vl[k]+" <= dataInArray("+str(int(input)-1)+") when pValidArray("+str(int(input)-1)+") = '1' else buf_in("+str(int(input)-1)+");\nreadyArray("+str(int(input)-1)+") <= (not pValidArray("+str(int(input)-1)+")) or (preValid and ready_buf);\n")
                        k = k + 1
        else:
            helper.error("Error: Mismatch ports between "+top.name+"_call_arg_analysis.rpt and dot graph")
            assert 0
    
    # included ip
    wb.append("\nfunc: "+func+"\nport map (\n")
    for line in buff:
        wb.append("\t"+helper.vhdlSigName(line) + " => " + helper.vhdlSigName(line))
        if buff.index(line) == len(buff)-1:
            wb.append("\n")
        else:
            wb.append(",\n")
    wb.append(");\n\nend architecture;\n\n--========================END=====================\n")

    wrapper = open(vhdlDir+"/"+func+"_wrapper.vhd", "w")
    for line in wb:
        wrapper.write(line)
    wrapper.close()


print("Back-end analysis finished successfully.")


