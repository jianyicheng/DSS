# Dummy remover for dot graph

from optparse import OptionParser
import sys, os

def parseEdge(line):
    src = line[:line.find("->")].replace(" ", "")
    dst = line[line.find("->")+2:line.find("[")].replace(" ", "")
    fromIdx = line[line.find("from=")+5:line.find(",", line.find("from="))].replace(" ", "")
    toIdx = line[line.find("to=")+3:line.find(",", line.find("to="))].replace(" ", "")
    tail = line[line.find(",", line.find("to=")):]
    return src, dst, fromIdx, toIdx, tail

def getBW(dot, src, fromIdx):
    for line in dot:
        if src+" [" in line and "->" not in line:
            idx = line.find(":", line.find(fromIdx))+1
            bw = ""
            while line[idx].isdigit():
                bw = bw + line[idx]
                idx = idx + 1
            return bw


def insertBuffers(dot, depth, trans):
    compDecl = -1
    channelStart = False
    comps = ""
    buffIdx = 0
    for idx, line in enumerate(dot):
        if "// Channels" in line:
            channelStart = True
            compDecl = idx - 2
            continue

        if "}" in line and channelStart:
            channelStart = False
            continue

        if channelStart:
           src, dst, fromIdx, toIdx, tail = parseEdge(line)
           bw = getBW(dot, src, fromIdx)
           comps += "_Buffer_naive"+str(buffIdx)+" [type=Buffer, in=\"in1:"+bw+"\", out=\"out1:"+bw+"\", bbID = 0, slots="+depth+", transparent="+trans+", label=\"_Buffer_naive"+str(buffIdx)+" ["+depth+"t]\",  shape=box, style=filled, fillcolor=darkolivegreen3, height = 0.4];\n"
           dot[idx] = "  "+src+" -> _Buffer_naive"+str(buffIdx)+" [from="+fromIdx+", to=in1"+tail+"  _Buffer_naive"+str(buffIdx)+" -> "+dst+" [from=out1, to="+toIdx+tail
           buffIdx = buffIdx + 1

    assert compDecl != -1

    dot[compDecl] = dot[compDecl] + comps

    return dot

def exportCode(newfile, buff):
    fout = open(newfile, "w")
    for line in buff:
        fout.write(line)
    fout.close()

def importCode(topfile):
    with open(topfile) as f:
        dot = f.readlines()
    return dot

def main():
    INFO  = "Naively insert buffers in all the channels"
    USAGE = "Usage: python InsertBuffersNaive.py file "

    def showVersion():
        print(INFO)
        print(USAGE)
        sys.exit()

    optparser = OptionParser()
    optparser.add_option("-v", "--version", action="store_true", dest="showversion",
                         default=False, help="Show the version")
    optparser.add_option("-o", "--output", dest="outfile",
                         default="", help="Output file name")
    optparser.add_option("-d", "--depth", dest="depth",
                         default="100", help="FIFO depth")
    optparser.add_option("--nontrans", dest="nontrans", action="store_true",
                         default=False, help="nontransparency")
    (options, args) = optparser.parse_args()
    depth = options.depth

    filelist = args
    if options.showversion:
        showVersion()

    # Find the file that contains the top level module
    if len(filelist) != 1:
        raise IOError("Only one file is allowed for a time.")
    trans = 'false' if options.nontrans else 'true'

    f = filelist[0]
    if not os.path.exists(f):
        raise IOError("file not found: " + f)
    outfile = options.outfile
    if outfile is None or outfile == "":
        outfile = f[0:f.find(".dot")]+"_new.dot"

    dot = importCode(f)
    insertBuffers(dot, depth, trans)
    exportCode(outfile, dot)

    

if __name__ == '__main__':
    main()
