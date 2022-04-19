# Pragma parser for DASS

from optparse import OptionParser
import sys, os, re, subprocess

class funcInfo:
    def __init__(self):
        self.lbegin = -1 # begin line number
        self.lend = -1 # end line number
        self.name = ""
funcs = []
constraints = []

def getFuncIndex(funcName):
    for f in funcs:
        if f.name == funcName:
            return funcs.index(f)
    return -1

def getFuns(f, ast):
    for line in ast:
        if (line.startswith("|-FunctionDecl") or line.startswith("`-FunctionDecl")) and line.count(".c") + line.count("line") >= 3:
            func = funcInfo()
            func.lbegin = int(line[line.find(":")+1:line.find(":", line.find(":")+1)]) - 1
            func.lend = int(line[line.find(", line:")+7:line.find(":", line.find(", line:")+7)]) - 1
            func.name = line[line.rfind(" ", 0, line.find(" \'")) + 1 : line.find("\'") - 1]
            index = getFuncIndex(func.name)

            if index == -1:
                funcs.append(func)
            else:
                if funcs[index].lend - funcs[index].lbegin < func.lend - func.lbegin:
                    funcs[index].lend = func.lend - 1
                    funcs[index].lbegin = func.lbegin - 1
       
def getFuncIndexByLine(lineIndex):
    for func in funcs:
        if func.lbegin < lineIndex and func.lend > lineIndex:
            return funcs.index(func)
    return -1

def getConstraint(funcName, line):
    while "  " in line or " \n" in line or "\t" in line:
        line = line.replace("  ", " ").replace(" \n", " ").replace("\t", " ")
    source = line
    line = line.upper()

    if line.replace(" ", "") == "SS":
        return "pipeline "+funcName+" 0"
    elif line.startswith("SS II") and line.replace(" ", "").startswith("SSII="):
        line = line.replace(" ", "")
        if line[5:].isdigit():
            return "pipeline "+funcName+" "+line[5:]
        else:
            return ""
    elif line.startswith("INTERCHANGE "):
        index = line.find("INTERCHANGE ")+len("INTERCHANGE ")
        line = line[index:]
        source = source[index:]
        loopName = source[:line.find(" ")]
        force = 'FALSE'
        depth = '-1'
        if " DEPTH" in line:
            depth = line[line.find("=", line.find("DEPTH"))+1:line.find(" ", line.find("=", line.find("DEPTH"))+2)].replace(" ", "")
        if " FORCE" in line:
            force = line[line.find("=", line.find("FORCE"))+1:line.find(" ", line.find("=", line.find("FORCE"))+2)].replace(" ", "")
        return "interchange " + funcName + " " + loopName + " " + depth + " " + force
    elif line.startswith("PARALLEL_LOOPS "):
        index = line.find("PARALLEL_LOOPS ") + len("PARALLEL_LOOPS ")
        line = line[index:]
        source = source[index:]
        return "parallel_loops " + funcName + " " + source.rstrip()
    else:
        return ""

def getPragmas(f):
    with open(f) as fileIn:
        code = fileIn.readlines()
    lineIndex = 0
    for line in code:
        index = getFuncIndexByLine(lineIndex)
        if index == -1:
            lineIndex = lineIndex + 1
            continue

        lineIter = line.replace("\t", " ").replace("  ", " ").replace("\n", " ")
        while lineIter != line:
            line = lineIter
            lineIter = line.replace("\t", " ").replace("  ", " ").replace("\n", " ")

        if "#pragma " not in line:
            lineIndex = lineIndex + 1
            continue

        if "#pragma DASS " not in line or index == -1:
            print("Warning 0: unknown pragma at "+f+":"+str(lineIndex+1))
            print(line)
            for func in funcs:
                print(func.name+" "+str(func.lbegin) + " " + str(func.lend))
        else:
            line = line[line.find("#pragma DASS ")+len("#pragma DASS "):]
            constraint = getConstraint(funcs[index].name, line)
            if constraint != "":
                constraints.append(constraint)
            else:
                print("Warning 1: unknown pragma at "+f+":"+str(lineIndex))
                print(line)
        lineIndex = lineIndex + 1

def removeComment(text):
    def replacer(match):
        s = match.group(0)
        if s.startswith('/'):
            return " " # note: a space and not an empty string
        else:
            return s
    pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
        re.DOTALL | re.MULTILINE
    )
    return re.sub(pattern, replacer, text)

def exportTcl(fname):
    tclOut = open(fname, "w")
    for constraint in constraints:
        tclOut.write(constraint+"\n")
    tclOut.close()

def main():
    INFO  = "Pragma parser for DASS"
    USAGE = "Usage: python PragmaParser.py file ..."

    def showVersion():
        print(INFO)
        print(USAGE)
        sys.exit()

    optparser = OptionParser()
    optparser.add_option("-v", "--version", action="store_true", dest="showversion",
                         default=False, help="Show the version")
    optparser.add_option("-o", "--output", dest="outfile",
                         default="", help="Output file name")
    (options, args) = optparser.parse_args()

    filelist = args
    if options.showversion:
        showVersion()

    # Remove comments and load function body from AST
    for f in filelist:
        if "dummy" in f:
            continue
        if not os.path.exists(f):
            raise IOError("file not found: " + f)
        with open(f) as fileIn:
            code = fileIn.readlines()
        newCode = removeComment(''.join(code))
        with open(f+".cpp", "w") as newFile:
            newFile.write(newCode)
        ast  = subprocess.getoutput("clang++ -Xclang -ast-dump -fno-color-diagnostics "+f+".cpp").split('\n')
        getFuns(f, ast)
    
    # Parse pragmas
    for f in filelist:
        if "dummy" in f:
            continue
        if not os.path.exists(f+".cpp"):
            raise IOError("file not found: " + f)
        getPragmas(f+".cpp")
        os.system("rm "+f+".cpp")
    
    # Export to tcl
    outfile = options.outfile
    if outfile is None or outfile == "":
        outfile = "pragmas.tcl"
    exportTcl(outfile)

if __name__ == '__main__':
    main()
