# Dynamatic optimizer for dot graph

from optparse import OptionParser
import sys, os

def hasDummy(dot):
    for line in dot:
        if "dummy_" in line and "type=Operator" in line:
            dot.remove(line)
            return line[line.find("dummy"):line.find(" [")]
    return ""

def removeDummies(dot):
    dummyName = hasDummy(dot)
    while dummyName != "":
        src = ""
        dst = ""
        for line in dot:
            if dummyName + " ->" in line:
                dst = line
            elif " -> " + dummyName in line:
                src = line
        assert src != "" and dst != ""

        srcName = src[0:src.find(" ->")].replace(" ", "")
        dstName = dst[dst.find("->")+2:dst.find("[")].replace(" ", "")
        srcOut = src[src.find("from=out")+8:src.find(",")]
        dstIn = dst[dst.find("to=in")+5:dst.find(", arrowhead")]
        newLine = "  "+srcName + " -> "+ dstName + " [from=out" + srcOut + ", to=in"+dstIn+", arrowhead=normal, color=red];\n"
        print("Removing: "+src)
        print("Removing: "+dst)
        print("Adding: "+newLine)
        dot.insert(dot.index(src), newLine)
        dot.remove(src)
        dot.remove(dst)
        dummyName = hasDummy(dot)
    return dot

def parseEdge(line):
    src = line[:line.find("->")].replace(" ", "")
    dst = line[line.find("->")+2:line.find("[")].replace(" ", "")
    fromIdx = line[line.find("from=")+5:line.find(",", line.find("from="))].replace(" ", "")
    toIdx = line[line.find("to=")+3:line.find(",", line.find("to="))].replace(" ", "")
    tail = line[line.find(",", line.find("to=")):]
    return src, dst, fromIdx, toIdx, tail

def insertLoopInterchanger(id, constraint, dot):
    constraints = constraint.replace("\"", "").replace(" ", "").replace("\n", "").split(",")
    assert len(constraints) >= 5

    entryBranch = constraints[0]
    exitBranch = constraints[1]
    entryPhi = constraints[2]
    exitPhi = constraints[3]
    depth = int(constraints[4])
    assert depth > 0

    entryLine = -1
    for index, line in enumerate(dot):
        if " -> " + entryPhi in line:
            if entryBranch + " -> " + entryPhi in line:
                entryLine = index
                break
            if "_Buffer_" in line[:line.find("->")]:
                tempbuf = line[:line.find("->")].replace(" ", "")
                for index2, line2 in enumerate(dot):
                    if " -> " + tempbuf in line2:
                        cascadesrc = line2[:line2.find("->")].replace(" ", "")
                        if cascadesrc == entryBranch:
                            entryLine = index
                            break
    if entryLine == -1:
        raise IOError("Cannot find entry edge: " + entryBranch + " -> " + entryPhi)

    entrysrc, entrydst, entryfromIdx, entrytoIdx, entrytail = parseEdge(dot[entryLine])
    dot[entryLine] = "  " + entrysrc + " -> loop_" + str(id) + " [from=" + entryfromIdx + ", to=in1" + entrytail + "  loop_" + str(id) + " -> " + entrydst + " [from=out1, to=" + entrytoIdx + entrytail

    exitLine = -1
    for index, line in enumerate(dot):
        if " -> " + exitPhi in line:
            if exitBranch + " -> " + exitPhi in line:
                exitLine = index
                break
            if "_Buffer_" in line[:line.find("->")]:
                tempbuf = line[:line.find("->")].replace(" ", "")
                for index2, line2 in enumerate(dot):
                    if " -> " + tempbuf in line2:
                        cascadesrc = line2[:line2.find("->")].replace(" ", "")
                        if cascadesrc == exitBranch:
                            exitLine = index
                            break
    if exitLine == -1:
        raise IOError("Cannot find exit edge: " + exitBranch + " -> " + exitPhi)

    exitsrc, exitdst, exitfromIdx, exittoIdx, exittail = parseEdge(dot[exitLine])
    dot[exitLine] = "  " + exitsrc + " -> loop_" + str(id) + " [from=" + exitfromIdx + ", to=in2" + exittail + "  loop_" + str(id) + " -> " + exitdst + " [from=out2, to=" + exittoIdx + exittail

    return dot

def insertLoopInterchangers(dot):
    tcl = "./loop_interchange.tcl"
    if not os.path.exists(tcl):
        raise IOError("file not found: " + tcl)
    with open(tcl) as fileIn:
        constraints = fileIn.readlines()
    
    componentList = -1    
    for index, line in enumerate(dot):
        if "// Channels" in line:
            componentList = index - 2
    buff = ""
    for index, constraint in enumerate(constraints):
        buff = buff + "  loop_" + str(index) + " [type=Operator, in=\"in1:1 in2:1\", out=\"out1:1 out2:1\", op = \"loop_interchanger\", latency=1, II=1, bbID = 0, shape=oval];\n"
    dot.insert(componentList + 1, buff)

    for index, constraint in enumerate(constraints):
        dot = insertLoopInterchanger(index, constraint, dot)

    return dot

def insertLoopParalleliser(id, constraint, dot):
    constraints = constraint.replace("\"", "").replace(" ", "").replace("\n", "").split(";")
    assert len(constraints) >= 3

    threads = int(constraints[0])
    forks = constraints[1].split(':')
    src = forks[0]
        
    forks = forks[1].split(',')
    joins = constraints[2].split(':') 
    dst = joins[0]
    joins = joins[1].split(',')

    srcIndex = -1
    for index, line in enumerate(dot):
        if src + " -> " in line:
            srcIndex = index
    if srcIndex == -1:
        raise IOError("Cannot find entry edge: " + src + " -> ")

    dstIndex = -1
    for index, line in enumerate(dot):
        if " -> " + dst in line:
            dstIndex = index
    if dstIndex == -1:
        raise IOError("Cannot find exit edge: -> " + dst)

    # Naively check edges
    for threadid in range(0, threads):
        entryLine = -1
        for index, line in enumerate(dot):
            if " -> " + forks[threadid] in line:
                entryLine = index
        if entryLine == -1:
            raise IOError("Cannot find entry edge: -> " + forks[threadid])

        exitLine = -1
        for index, line in enumerate(dot):
            if joins[threadid] + " -> " in line and "out2" in line:
                if "Buffer" in line:
                    joins[threadid] = line[line.find("-> ")+3:line.find(" [")]
                exitLine = index
        if exitLine == -1:
            raise IOError("Cannot find exit edge: " + joins[threadid] + " -> ")

    newdot = []
    for index, line in enumerate(dot):
        buff = [] 
        if " -> " + src in line:
            outn = line[line.find("from="):line.find(",", line.find("from="))]
            compsrc = line[:line.find('->')].replace(' ', '')
            buff = buff + ["  {} -> forkpC_{} [{}, to=in1, arrowhead=normal, color=gold3];\n".format(compsrc, id, outn)]
        if " -> " + dst in line:
            inn = line[line.find("to="):line.find(",", line.find("to="))]
            buff = buff + ["  joinpC_{} -> {} [from=out1, {}, arrowhead=normal, color=gold3];\n".format(id, dst, inn)]
        for threadid in range(0, threads):
            if " -> " + forks[threadid] in line:
                inn = line[line.find("to="):line.find(",", line.find("to="))]
                buff = buff + ["  forkpC_{} -> {} [from=out{}, {}, arrowhead=normal, color=gold3];\n".format(id, forks[threadid], threadid+1, inn)]
            if joins[threadid] + " -> " in line and ("Buffer" in joins[threadid] or "out2" in line):
                outn = line[line.find("from="):line.find(",", line.find("from="))]
                buff = buff + ["  {} -> joinpC_{} [{}, to=in{}, arrowhead=normal, color=gold3];\n".format(joins[threadid], id, outn, threadid+1)]
        if buff:
            # dot[index] = buff
            newdot += buff
        else:
            newdot += [line]
    return newdot

def parallelLoops(dot):
    tcl = "./parallel_loops.tcl"
    if not os.path.exists(tcl):
        raise IOError("file not found: " + tcl)
    with open(tcl) as fileIn:
        constraints = fileIn.readlines()

    componentList = -1
    for index, line in enumerate(dot):
        if "// Channels" in line:
            componentList = index - 2

    buff = ""
    for index, constraint in enumerate(constraints):
        threads = int(constraint[0:constraint.find(";")])
        ins = ""
        outs = "" 
        for i in range(0, threads):
            ins = ins + "in{}:0 ".format(i+1)
            outs = outs + "out{}:0 ".format(i+1)
        buff = buff + "  forkpC_" + str(index) + " [type=Fork, in=\"in1:0\", out=\"" + outs + "\", bbID = 0, shape=oval];\n"
        buff = buff + "  joinpC_" + str(index) + " [type=Operator, in=\"" + ins + "\", out=\"out1:0\", op = \"join_op\", latency=1, II=1, bbID = 0, shape=oval];\n"
    dot.insert(componentList + 1, buff)

    for index, constraint in enumerate(constraints):
        dot = insertLoopParalleliser(index, constraint, dot)

    return dot

def main():
    INFO  = "Shift register remover at IO Ports"
    USAGE = "Usage: python DotDummyRemover.py file "

    def showVersion():
        print(INFO)
        print(USAGE)
        sys.exit()

    optparser = OptionParser()
    optparser.add_option("-v", "--version", action="store_true", dest="showversion",
                         default=False, help="Show the version")
    optparser.add_option("-o", "--output", dest="outfile",
                         default="", help="Output file name")
    optparser.add_option("-d", "--remove-dummy", action="store_true", dest="removedummy",
                         default=False, help="Remove dummies")
    optparser.add_option("-l", "--loop-interchange", action="store_true", dest="loopinterchange",
                         default=False, help="Insert loop interchangers")
    (options, args) = optparser.parse_args()

    filelist = args
    if options.showversion:
        showVersion()

    # Find the file that contains the top level module
    if len(filelist) != 1:
        raise IOError("Only one file is allowed for a time.")

    f = filelist[0]
    if not os.path.exists(f):
        raise IOError("file not found: " + f)
    outfile = options.outfile
    if outfile is None or outfile == "":
        outfile = f[0:f.find(".dot")]+"_new.dot"
    
    with open(f) as fileIn:
        dot = fileIn.readlines()

    if options.removedummy:
        dot = removeDummies(dot)
    
    if options.loopinterchange:
        dot = insertLoopInterchangers(dot)
        dot = parallelLoops(dot)
        
    newFile = open(outfile, "w")
    for line in dot:
        newFile.write(line)
    newFile.close()

if __name__ == '__main__':
    main()
