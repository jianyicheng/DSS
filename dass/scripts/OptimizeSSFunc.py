# Verilog rewriter to remove shift registers for IO ports
# Currently works for Vitis HLS 2020.2

from optparse import OptionParser
import sys, os

# PortInfo class
class portInfo:
    # name : name of the port
    # isIn : whether it is an input
    # offset : the offset in cycles of the port
    # depth : estimated number of registers to remove
    # idleSt : # of state the input is idle
    # firstOpLatency : the latency of the first op to process an input

    def print(self):
        print(self.name + ", " + str(self.isIn) + ", " + str(self.offset))

    def load(self, line, latency):
        assert line.count(",") == 7 and latency > 0
        idx0 = line.find(",")
        self.name = line[0 : idx0]
        idx1 = line.find(",", idx0 + 1)
        self.idx = int(line[idx0 + 1:idx1])
        idx2 = line.find(",", idx1 + 1)
        self.isIn = int(line[idx1 + 1:idx2])
        idx3 = line.find(",", idx2 + 1)
        self.offset = int(line[idx2 + 1:idx3])
        idx4 = line.find(",", idx3 + 1)
        self.idleSt = int(line[idx3 + 1:idx4])
        idx5 = line.find(",", idx4 + 1)
        self.firstOpLatency = int(line[idx4 + 1:idx5])
        idx6 = line.find(",", idx5 + 1)
        self.depth = int(line[idx5 + 1:idx6])
        
        assert self.offset >= 0
        assert self.isIn == 0 or self.isIn == 1
        assert self.depth >= 0
        return self

# Read the tcl file and extract the offset info
def analyzeOffset(file, top):
    if not os.path.exists(file) or os.stat(file).st_size == 0:
        return None
    st = -1
    latency = -1
    ports = []
    with open(file) as text:
        for line in text:
            if st == -1:
                if "Function: " + top in line:
                    st = 0
                    idx0 = line.find(",")
                    idx1 = line.find(",", idx0 + 1)
                    latency = int(line[idx0 + 1:idx1])
                    # idx2 = line.find(",", idx1 + 1)
                    # II = int(line[idx1 + 1:idx2])
                    assert latency > 0
                continue
            if st == 0:
                if "---" not in line:
                    p = portInfo()
                    ports.append(p.load(line, latency))
                else:
                    return ports
    return None

# check signal does not already exist in the code
def exist(name, region):
    for line in region:
        if " "+name+";" in line:
            return True
    return False

# Check if a signal has a single use
def isSingleUse(region, name):
    readCount = 0
    writeCount = 0

# get assigned signal name in shift registers
def getAssignedSignal(region, srcName):
    useCount = 0
    result = srcName
    lineCount = 0
    lineIndex = -1
    for line in region:
        if srcName+";" in line and "_read_reg_" in line and "= " in line:
            if "<=" in line:
                result = line[0:line.find("<=")].replace(" ", "")
                lineIndex = lineCount
            else:
                result = line[0:line.find("=")].replace(" ", "")
                lineIndex = lineCount
            useCount = useCount + 1
        elif "("+srcName+")," in line:
            useCount = useCount + 1
        lineCount = lineCount + 1
    if useCount == 1:
        if "_read_reg_" not in result:
            return lineIndex, srcName
        else:
            return lineIndex, result
    else:
        return -1, srcName

def getValueAfterString(line, string):
    if string not in line:
        return -1
    index = line.find(string)+len(string)
    val = ""
    while line[index].isdigit():
        val = val + line[index]
        index = index + 1
    return int(val)

# Remove shift register for an input (forward trace)
# TODO: Use Pyverilog to rewrite it
def rewriteInput(region, port):
    depth = 0
    name = port.name
    nextSignal = ""
    while True:
        lineIndex, nextSignal = getAssignedSignal(region, name)
        if nextSignal == name:
            depth = getValueAfterString(region[lineIndex-1], "ap_CS_fsm_state") - 2
            break
        else:
            depth = getValueAfterString(region[lineIndex-1], "ap_CS_fsm_state")
            name = nextSignal
        
    expectDepth = port.depth
    if depth != expectDepth:
        sys.stderr.write("Warning: Depth mismatched. please check: "+port.name + ". Expected depth = "+str(expectDepth)+". Found depth = "+str(depth)+"\n")
        sys.stderr.write("Ignore the above warning if the signal is connected to an operator...\n")
    
    print(port.name + " => " + name)
    if name != port.name:
        for line in region: 
            if name in line:
                region[region.index(line)] = line.replace("("+name+")", "("+port.name+")").replace("= "+name+";", "= "+port.name+";")
    return region

# Remove shift register for a single port
def rewritePort(region, port):
    if port.isIn:
        return rewriteInput(region, port)
    else:
        # return rewriteOutput(region, port)
        sys.stderr.write("Short cutting output not implemented/tested yet\n")
        assert 0

# Parse the Verilog code and remove shift registers
def removeShiftReg(ports, verilog, top):
    
    # Get module region
    moduleStart = -1
    moduleEnd = -1
    for line in verilog:
        if "module " + top + " (" in line:
            moduleStart = verilog.index(line)
        if "endmodule //"+top in line:
            moduleEnd = verilog.index(line)
            break
    assert moduleStart != -1 and moduleEnd != -1
    region = verilog[moduleStart:moduleEnd+1]

    # Rewrite port
    for port in ports:
        if port.depth != 0:
            region = rewritePort(region, port)

    # Export the new file
    buff = verilog[0:moduleStart] + region + verilog[moduleEnd+1:]
    return buff

def exportCode(newfile, buff):
    if newfile is None or newfile == "":
        newfile = topfile[0:topfile.find(".v")]+"_new.v"
    fout = open(newfile, "w")
    for line in buff:
        fout.write(line)
    fout.close()

def importCode(topfile):
    with open(topfile) as f:
        verilog = f.readlines()
    return verilog

def removeClockEnables(topfile):
    for idx, line in enumerate(topfile):
        if "_ce0 = 1'b1;" in line or "_ce1 = 1'b1;" in line or "_we0 = 1'b1;" in line or "_we1 = 1'b1;" in line \
           or "_write = 1'b1;" in line or "ap_done = 1'b1;" in line:
            topfile[idx-1] = topfile[idx-1].replace(" & (1'b1 == ap_ce)", "").replace("(1'b1 == ap_ce) & ", "")
    return topfile

def main():
    INFO  = "Shift register remover at IO Ports"
    USAGE = "Usage: python OptimizeSSFunc.py file ..."

    def showVersion():
        print(INFO)
        print(USAGE)
        sys.exit()

    optparser = OptionParser()
    optparser.add_option("-v", "--version", action="store_true", dest="showversion",
                         default=False, help="Show the version")
    optparser.add_option("-t", "--top", dest="topmodule",
                         default="TOP", help="Top module, Default=TOP")
    optparser.add_option("-o", "--output", dest="outfile",
                         default="", help="Output file name")
    (options, args) = optparser.parse_args()

    filelist = args
    if options.showversion:
        showVersion()
    top = options.topmodule

    # Find the file that contains the top level module
    topfile = None
    for f in filelist:
        if not os.path.exists(f):
            raise IOError("file not found: " + f)
        with open(f) as text:
            if "module "+top+" (" in text.read():
                topfile = f
            
    if len(filelist) == 0:
        showVersion()

    if topfile is None:
        sys.stderr.write("Cannot find top level module "+top+"\n")
        sys.exit()

    # Check IO ports exist
    ports = analyzeOffset("vhls/ss_offset.tcl", top)

    outfile = options.outfile
    if outfile is None or outfile == "":
        outfile = topfile[0:topfile.find(".v")]+"_new.v"

    f = importCode(topfile)

    # Remove shift registers
    if ports is not None:
        f = removeShiftReg(ports, f, top)

    # Remove ce for clock enables
    f = removeClockEnables(f)

    exportCode(outfile, f)


if __name__ == '__main__':
    main()
