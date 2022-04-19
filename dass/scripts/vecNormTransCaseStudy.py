# A script to run the experiment of vecNormTrans for static island selections
# Usage: python3 vecNormTransCaseStudy.py [options]
# Options:
#   -h, --help            show this help message and exit
#   -v, --version         Show the version
#   -o DIR, --output=DIR  Output diretory, default = ./staticSelections
#   -p SIZE, --problem-size=SIZE
#                         Problem size, default N = 4
#   -j THREADS, --threads=THREADS
#                         threads, default = 1
#   -d DEBUGCASE, --debug=DEBUGCASE
#                         Enable only a specific case
#   -g, --codegen         C source generation
#   -s, --synth           Synthesize design
#   -c, --cosim           Cosimulation
#   -i, --post-synth      Post synthesis design (get area results)
#   -e, --evaluate        Evaluate the results
#   -r, --reset           Reset. Clean all the files
#   -a, --all             Run all from scratch
#   -k, --disable-skip    Do not skip the existing cases and rerun all the
#                         cases.
#
# A complete run from scratch is:
# python3 vecNormTransCaseStudy.py -p 256 -r -a -j $(npoc)

from multiprocessing import Process, Queue
from optparse import OptionParser
import sys, os, time, datetime, multiprocessing
from tabulate import tabulate
from typing import List

#############################################################
#    Codegen
#############################################################

def getInAndOut(instrs):
    inputs = []
    outputs = []
    for instr in instrs:
        input = []
        in0 = instr[instr.find("= ")+2:instr.find(" ", instr.find("= ")+3)].replace(" ", "")
        if not any(map(str.isdigit, in0)) and in0 != '':
            input.append(in0)
        in1 = instr[instr.rfind(" ", instr.find(";")-3):instr.find(";")].replace(" ", "")
        if not any(map(str.isdigit, in1)) and in1 != '':
            input.append(in1)
        inputs.append(input)
        outputs.append(instr[len("float "):instr.find("=")].replace(" ", ""))
    return inputs, outputs

def findAllCombinations(size):
    resultSets = []
    for i in range(1, size+1):
        currSet = [i]
        if size - i != 0:
            allSets = findAllCombinations(size-i)
            for s in allSets:
                resultSets.append(currSet+s)
        else:
            resultSets.append(currSet)
    return resultSets

def functionalize(i, j, island, inputs, outputs, instrs, name, ii):
    args = []
    res = []
    for idx in range(j, j+island):
        res.append(outputs[idx])
        for a in inputs[idx]:
            args.append(a)
    args, res = [a for a in args if a not in res], [r for r in res if r not in args]
    args = list(set(args))
    assert len(res) == 1

    s = "void "+name+"_"+str(i)+"_"+str(j)+"("
    for a in args:
        s = s + "float "+a+","
    s = s + "float *"+res[0]+"_out){\n#pragma DASS SS II="+str(ii)+"\n"
    for idx in range(j, j+island):
        s = s + instrs[idx]
    for r in res:
        s = s + "*"+ r + "_out = "+r+";"
    s = s + "}"

    b = "float "+res[0]+"_out;"
    b = b + name+"_"+str(i)+"_"+str(j)+"("
    for a in args:
        b = b + a+","
    b = b + "&"+res[0]+"_out); float "+res[0]+" = "+res[0]+"_out;"
    return s, b

def codeCombinationGen(name, instrs, ii):
    target = len(instrs)
    combinations = findAllCombinations(target)
    inputs, outputs = getInAndOut(instrs)
    staticFuncs = []
    funcBodys = []
    i = 0
    for combination in combinations:
        staticFunc = ""
        funcBody = ""
        j = 0
        for island in combination:
            if island == 1:
                funcBody = funcBody + instrs[j]
            else:
                func, body = functionalize(i, j, island, inputs, outputs, instrs, name, ii)
                staticFunc = staticFunc + func
                funcBody = funcBody + body 
            j = j + island
        i = i + 1
        staticFuncs.append(staticFunc)
        funcBodys.append(funcBody)
    return staticFuncs, funcBodys

def generateSourceCode(problemSize, dir, export):
    dsIndex = -1
    ssIndex = -1
    dssIndex = -1
    codeHeader = "#include \"vecNormTrans.h\"\n//------------------------------------------------------------------------\n// vecNormTrans\n//------------------------------------------------------------------------\n// SEPARATOR_FOR_MAIN\n#include \"vecNormTrans.h\"\n#include <stdlib.h>\n#define N "+str(problemSize)+"\n"
    staticFunc=""
    fundDef = "void vecNormTrans(in_float_t a["+str(problemSize)+"], inout_float_t r["+str(problemSize)+"]) {\n"
    funcBody = ""
    codeTail = "} int main() { float array[N], result[N], gold[N]; for (int i = 0; i < N; i++) { array[i] = (i % 2 == 0) ? 2.0f : 0.1f; result[i] = 0.0f; gold[i] = 0.0f; } float weight = 0.0f; for (int i = 0; i < N; i++) { float d = array[i]; float s; if (d < 1.0f) weight = ((d * d + 19.52381f) * d + 3.704762f) * d + 0.73f * weight; else weight = weight; } for (int i = 0; i < N - 4; i++) { float d = array[i] / weight; gold[i + 4] = gold[i] + d; } vecNormTrans(array, result); int check = 0; for (int i = 0; i < N; i++) check += (result[i] == gold[i]); if (check == N) return 0; else return -1; } "
    # Create Headerfile
    if export:
        hfile = open("vecNormTrans_.h", "w")
        hfile.write("typedef float in_float_t;typedef float out_float_t;typedef float inout_float_t;void vecNormTrans(in_float_t a["+str(problemSize)+"], inout_float_t r["+str(problemSize)+"]);\n")
        hfile.close()
        os.system("clang-format -i vecNormTrans_.h")
        os.system("mkdir -p " + dir)
        os.system("mkdir -p " + dir + "/src")
        os.system("cp vecNormTrans_.h "+dir+"/src/vecNormTrans.h")

    ii0 = 5
    ii1 = 2
    loop_0 = ['float l = d * d;','float m = l + 19.52381f;','float n = m * d;','float p = n + 3.704762f;','float q = p * d;', 'float w = weight * 0.73f + q;']
    loop0StaticFuncs, loop0FuncBodys = codeCombinationGen("loop0", loop_0, ii0)
    loop1 = ['float c = d / w;','float res = b + c;']
    loop1StaticFuncs, loop1FuncBodys = codeCombinationGen("loop1", loop1, ii1)

    codeCheck = []
    count = 0
    e = 0
    print("Seach space: "+str(3*2*2*len(loop0StaticFuncs)*len(loop1StaticFuncs)))
    for mergeLoop0 in [True, False]:
        for mergeLoop1 in [True, False]:
            for mergeIf in [True, False]:
                for loop0Expr in range(0, len(loop0StaticFuncs)):
                    for loop1Expr in range(0, len(loop1StaticFuncs)):
                        staticFunc=""
                        funcBody = ""
                        if mergeLoop0: 
                            staticFunc = "void loop_0_func_0(float a["+str(problemSize)+"], float *newWeight) { \n#pragma DASS SS\n float weight = 0.0f; for (int i = 0; i < N; i++) { float d = a[i]; if (d < 1.0f) weight = ((d * d + 19.52381f) * d + 3.704762f) * d + 0.73f * weight; else weight = weight; } *newWeight = weight;}"
                            funcBody = "float weight; loop_0_func_0(a, &weight); "
                        elif mergeIf: 
                            staticFunc = "void if_func_0(float d, float w, float *newWeight) {\n#pragma DASS SS II="+str(ii0)+"\n float weight; if (d < 1.0f) weight = ((d * d + 19.52381f) * d + 3.704762f) * d + 0.73f * w; else weight = w; *newWeight = weight;}"
                            funcBody = "float weight = 0.0f; loop_0: for (int i = 0; i < N; i++) { float d = a[i]; float newWeight; if_func_0(d, weight, &newWeight); weight = newWeight; }"
                        else:
                            funcBody = "float weight = 0.0f; for (int i = 0; i < N; i++) { float d = a[i]; if (d < 1.0f) {"
                            funcBody = funcBody + loop0FuncBodys[loop0Expr]
                            funcBody = funcBody + "weight = w; } else weight = weight; }"
                            staticFunc = loop0StaticFuncs[loop0Expr]

                        if mergeLoop1:
                            staticFunc = staticFunc + "void loop_1_func(float weight, float a["+str(problemSize)+"], float r["+str(problemSize)+"]) {\n#pragma DASS SS\n for (int i = 0; i < N - 4; i++) { float d = a[i] / weight; r[i + 4] = r[i] + d; } }"
                            funcBody = funcBody + "loop_1_func(weight, a, r); "
                        else: 
                            funcBody = funcBody + "float w = weight; for (int i = 0; i < N - 4; i++) { float d = a[i]; float b = r[i];"
                            funcBody = funcBody + loop1FuncBodys[loop1Expr]
                            funcBody = funcBody + "r[i + 4] = res; }"
                            staticFunc = staticFunc + loop1StaticFuncs[loop1Expr]

                        outputCode = codeHeader+staticFunc+fundDef+funcBody+codeTail
                        if codeCheck.count(outputCode) == 0:
                            if mergeLoop0 == False and mergeLoop1 == False and mergeIf == False and loop0Expr == 0 and loop1Expr == 0:
                                dsIndex = count
                            if mergeLoop0 == False and mergeLoop1 == True and mergeIf == False and loop0Expr == len(loop0StaticFuncs)-1:
                                dssIndex = count
                            if export:
                                os.system("mkdir -p " + dir + "/vecNormTrans_"+str(count))
                                os.system("cp vecNormTrans_.h " + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.h")
                                ftemp = open("" + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.cpp", "w")
                                ftemp.write(outputCode)
                                ftemp.close()
                                os.system("clang-format -i " + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.cpp")
                                os.system("cp " + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.cpp " + dir + "/src/vecNormTrans_"+str(count)+".cpp")
                                print("Exporting " + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.cpp...")
                                # Verify Syntax
                                if os.system("(cd " + dir + "/vecNormTrans_"+str(count)+"; rm -f a.out; g++ vecNormTrans.cpp; ./a.out)") != 0:
                                    print("Error found in " + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.cpp")
                                    assert False
                                    e = e + 1
                            codeCheck.append(outputCode)
                            count = count + 1

    # Add case for purely SS
    funcBody = "float weight = 0.0f; for (int i = 0; i < N; i++) { float d = a[i]; if (d < 1.0f) {"
    funcBody = funcBody + loop0FuncBodys[0]
    funcBody = funcBody + "weight = w; } else weight = weight; }"
    funcBody = funcBody + "float w = weight; for (int i = 0; i < N - 4; i++) { float d = a[i]; float b = r[i];"
    funcBody = funcBody + loop1FuncBodys[0]
    funcBody = funcBody + "r[i + 4] = res; }"
    outputCode = codeHeader+fundDef+"\n#pragma DASS SS\n"+funcBody+codeTail
    if export:
        os.system("mkdir -p " + dir + "/vecNormTrans_"+str(count))
        os.system("cp vecNormTrans_.h " + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.h")
        ftemp = open("" + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.cpp", "w")
        ftemp.write(outputCode)
        ftemp.close()
        os.system("clang-format -i " + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.cpp")
        os.system("cp " + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.cpp " + dir + "/src/vecNormTrans_"+str(count)+".cpp")
        print("Exporting " + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.cpp...")
        # Verify Syntax
        if os.system("(cd " + dir + "/vecNormTrans_"+str(count)+"; rm -f a.out; g++ vecNormTrans.cpp; ./a.out)") != 0:
            print("Error found in " + dir + "/vecNormTrans_"+str(count)+"/vecNormTrans.cpp")
            assert False
            e = e + 1
        os.system("rm vecNormTrans_.h")
        print("Code generation finished. "+str(count)+" selections.\nError: "+str(e))
    codeCheck.append(outputCode)
    ssIndex = count
    count = count + 1
    return ssIndex, dsIndex, dssIndex
    
#############################################################
#    Synthesize
#############################################################

synthCommand = "{ timeout 30m dass-baseline vecNormTrans &> dass.log ; } 2> time.log"
dassKey = "DASS: Generate hardware successfully."

def synthDesignOnce(dir, i, disableskip):
    dassLog = dir + "/vecNormTrans_"+str(i)+"/dass.log"
    buff = "Synthesizing " + dir + "/vecNormTrans_"+str(i)
    start = time.time()
    if not disableskip and os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi"):
        os.system("(cd ./" + dir + "/vecNormTrans_"+str(i)+"; "+ synthCommand+")")
        if not os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi"):
            buff = buff + " SUCCESS. time = "
        else:
            buff = buff + " FAILED. time = "
    else:
        buff = buff + " SUCCESS. time = "
    end = time.time()
    buff = buff + "{:.2f}".format(end-start)
    os.system("echo \""+buff+"\"")

def synthDesignFunc(dir, dirCount, threads, id, disableskip):
    for ri in range(0, dirCount):
        # Forward
        i = ri
        # Backward
        # i = dirCount - 1 - ri
        if i % threads != id:
            continue

        # purely static implementation
        if ri == dirCount - 1:
            dassLog = dir + "/vecNormTrans_"+str(i)+"/dass.log"
            buff = "Synthesizing " + dir + "/vecNormTrans_"+str(i)
            start = time.time()
            if not disableskip and os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi"):
                os.system("(cd ./" + dir + "/vecNormTrans_"+str(i)+"; vitis-hls-llvm vecNormTrans &> dass.log)")
                if not os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fq \"HLS EXTRACTION\" "+dassLog+"\n else\n exit 1 \nfi"):
                    os.system("echo \""+dassKey+"\" >> "+dassLog)
                    os.system("mkdir ./" + dir + "/vecNormTrans_"+str(i)+"/rtl")
                    os.system("cp ./" + dir + "/vecNormTrans_"+str(i)+"/vecNormTrans/solution1/impl/ip/hdl/ip/* ./" + dir + "/vecNormTrans_"+str(i)+"/rtl")
                    os.system("cp ./" + dir + "/vecNormTrans_"+str(i)+"/vecNormTrans/solution1/impl/ip/hdl/verilog/* ./" + dir + "/vecNormTrans_"+str(i)+"/rtl")
                    os.system("if ls ./" + dir + "/vecNormTrans_"+str(i)+"/rtl/*.v 1> /dev/null 2>&1; then\nfor file in ./" + dir + "/vecNormTrans_"+str(i)+"/rtl/*.v; do\nif ! grep -Fxq '`timescale 1ns/1ps' $file; then\nsed -i '1i `timescale 1ns/1ps' $file;\nfi\ndone\nfi")
                    buff = buff + " SUCCESS. time = "
                else:
                    os.system("echo FAILED >> "+dassLog)
                    buff = buff + " FAILED. time = "
            else:
                buff = buff + " SUCCESS. time = "
            end = time.time()
            buff = buff + "{:.2f}".format(end-start)
            os.system("echo \""+buff+"\"")
            continue

        synthDesignOnce(dir, i, disableskip)

def synthDesign(dir, threads, disableskip):
    path, dirs, files = next(os.walk(dir))
    dirCount = len(dirs) - 1

    jobs = [None] * threads
    queue = Queue()
    for i in range(0, threads):
        jobs[i] = Process(target=synthDesignFunc, args=(dir, dirCount, threads, i, disableskip))
        jobs[i].start()

    for i in range(0, threads):
        jobs[i].join()

    successCount = 0
    for ri in range(0, dirCount):
        i = ri
        dassLog = dir + "/vecNormTrans_"+str(i)+"/dass.log"
        if i == dirCount - 1 and not os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi"):
            successCount = successCount + 1
        elif not os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi"):
            successCount = successCount + 1
    print("Error: "+str(dirCount-successCount)+"\nSynthesis coverage: " + str(successCount) + "/" + str(dirCount))

#############################################################
#    Cosim
#############################################################

simCommand = "simulate vecNormTrans &> sim.log"
simKey = "Comparison of [r] : Pass"

def cosimDesignOnce(dir, i, disableskip):
    dassLog = dir + "/vecNormTrans_"+str(i)+"/dass.log"
    simLog = dir + "/vecNormTrans_"+str(i)+"/sim/HLS_VERIFY/transcript"
    buff = "Co-simulating " + dir + "/vecNormTrans_"+str(i)
    # Skip simulation if synthesis failed
    if os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi"):
        buff = buff + " FAILED (synth failed - please run --synth first). time = 0"
        os.system("echo \""+buff+"\"")
        return
    
    start = time.time()
    if not disableskip and os.system("if [[ -f \""+simLog+"\" ]]; then\n grep -Fxq \""+simKey+"\" "+simLog+"\n else\n exit 1 \nfi"):
        os.system("(cd ./" + dir + "/vecNormTrans_"+str(i)+"; "+ simCommand+")")
        if not os.system("if [[ -f \""+simLog+"\" ]]; then\n grep -Fxq \""+simKey+"\" "+simLog+"\n else\n exit 1 \nfi"):
            buff = buff + " SUCCESS. time = "
        else:
            buff = buff + " FAILED. time = "
    else:
        buff = buff + " SUCCESS. time = "
    end = time.time()
    buff = buff + "{:.2f}".format(end-start)
    os.system("echo \""+buff+"\"")

def cosimDesignFunc(dir, dirCount, threads, id, disableskip):
    for ri in range(0, dirCount):
        # Forward
        i = ri
        # Backward
        # i = dirCount - 1 - ri
        if i % threads != id or i == dirCount-1:
            continue
        cosimDesignOnce(dir, i, disableskip)

def cosimDesign(dir, threads, disableskip):
    path, dirs, files = next(os.walk(dir))
    dirCount = len(dirs) - 1

    starts = [None] * threads
    ends = [None] * threads
    jobs = [None] * threads
    unit = dirCount/threads
    for i in range(0, threads):
        starts[i] = int(i*unit)
        ends[i] = min(int((i+1)*unit), dirCount)
    
    jobs = [None] * threads
    queue = Queue()
    for i in range(0, threads):
        jobs[i] = Process(target=cosimDesignFunc, args=(dir, dirCount, threads, i, disableskip))
        jobs[i].start()

    for i in range(0, threads):
        jobs[i].join()

    successCount = 1
    for ri in range(0, dirCount-1):
        i = ri
        dassLog = dir + "/vecNormTrans_"+str(i)+"/dass.log"
        simLog = dir + "/vecNormTrans_"+str(i)+"/sim/HLS_VERIFY/transcript"
        # Skip simulation if synthesis failed
        if os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi"):
            continue
        if not os.system("if [[ -f \""+simLog+"\" ]]; then\n grep -Fxq \""+simKey+"\" "+simLog+"\n else\n exit 1 \nfi"):
            successCount = successCount + 1
    print("Error: "+str(dirCount-successCount)+"\nCosimulation coverage: " + str(successCount) + "/" + str(dirCount))

#############################################################
#    Post Synthesis
#############################################################

areaKey = "Total LUTs | Logic LUTs"
timingKey = "| Design Timing Summary"

def implDesignOnce(dir, i, implCommand, disableskip):
    dassLog = dir + "/vecNormTrans_"+str(i)+"/dass.log"
    buff = "Implementing " + dir + "/vecNormTrans_"+str(i)
    # Skip simulation if synthesis failed
    if os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi"):
        buff = buff + " FAILED (synth failed). time = 0"
        os.system("echo \""+buff+"\"")
        return
    
    areaLog = dir + "/vecNormTrans_"+str(i)+"/syn_project/util.rpt"
    timingLog = dir + "/vecNormTrans_"+str(i)+"/syn_project/timing.rpt"
    start = time.time()
    if not disableskip and os.system("if [[ -f \""+areaLog+"\" ]]; then\n grep -Fq \""+areaKey+"\" "+areaLog+"\n else\n exit 1 \nfi") or \
       os.system("if [[ -f \""+timingLog+"\" ]]; then\n grep -Fq \""+timingKey+"\" "+timingLog+"\n else\n exit 1 \nfi"):
        os.system("(cd ./" + dir + "/vecNormTrans_"+str(i)+"; "+ implCommand+")")
        if os.system("if [[ -f \""+areaLog+"\" ]]; then\n grep -Fq \""+areaKey+"\" "+areaLog+"\n else\n exit 1 \nfi") or \
           os.system("if [[ -f \""+timingLog+"\" ]]; then\n grep -Fq \""+timingKey+"\" "+timingLog+"\n else\n exit 1 \nfi"):
            buff = buff + " FAILED. time = "
        else:
            buff = buff + " SUCCESS. time = "
    else:
        buff = buff + " SUCCESS. time = "
    end = time.time()
    buff = buff + "{:.2f}".format(end-start)
    os.system("echo \""+buff+"\"")

def implDesignFunc(dir, dirCount, threads, id, disableskip):
    jobs = int(multiprocessing.cpu_count()/threads)
    implCommand = "evaluateSynthesis vecNormTrans "+str(jobs)+" &> impl.log"
    for ri in range(0, dirCount-1):
        # Forward
        i = ri
        # Backward
        # i = dirCount - 1 - ri
        if i % threads != id:
            continue
        implDesignOnce(dir, i, implCommand, disableskip)
        

def implDesign(dir, threads, disableskip):
    if threads > multiprocessing.cpu_count():
        threads = multiprocessing.cpu_count()

    path, dirs, files = next(os.walk(dir))
    dirCount = len(dirs) - 1

    starts = [None] * threads
    ends = [None] * threads
    jobs = [None] * threads
    unit = dirCount/threads
    for i in range(0, threads):
        starts[i] = int(i*unit)
        ends[i] = min(int((i+1)*unit), dirCount)
    
    jobs = [None] * threads
    queue = Queue()
    for i in range(0, threads):
        jobs[i] = Process(target=implDesignFunc, args=(dir, dirCount, threads, i, disableskip))
        jobs[i].start()

    for i in range(0, threads):
        jobs[i].join()

    successCount = 0
    for ri in range(0, dirCount):
        i = ri
        dassLog = dir + "/vecNormTrans_"+str(i)+"/dass.log"
        areaLog = dir + "/vecNormTrans_"+str(i)+"/syn_project/util.rpt"
        timingLog = dir + "/vecNormTrans_"+str(i)+"/syn_project/timing.rpt"
        if i == dirCount - 1 and not os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi"):
            successCount = successCount + 1
            continue
        elif os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi") or \
           os.system("if [[ -f \""+areaLog+"\" ]]; then\n grep -Fq \""+areaKey+"\" "+areaLog+"\n else\n exit 1 \nfi") or \
           os.system("if [[ -f \""+timingLog+"\" ]]; then\n grep -Fq \""+timingKey+"\" "+timingLog+"\n else\n exit 1 \nfi"):
            continue
        successCount = successCount + 1
    print("Error: "+str(dirCount-successCount)+"\nPost synthesis coverage: " + str(successCount) + "/" + str(dirCount))

#############################################################
#    Evaluate
#############################################################

def getLatencyInCycles(file):
    if not os.path.isfile(file):
        return "NA"
    f = open(file)
    for line in f:
        if "$finish called at time : " in line:
            latencyInNS = int(line.replace("$finish called at time : ", "").replace("ns", ""))
            return str((latencyInNS-10) >> 2)
    return "NA"

def getAreaInfo(file):
    if not os.path.isfile(file):
        return ["NA"]*4
    f = open(file)
    for line in f:
        if "Total LUTs" in line:
            break
    if "Total LUTs" in line:
        line = f.readline()
        line = f.readline()
        values = line.split('|')
        # lut, dsp, register, bram
        return [int(values[3]), int(values[10]), int(values[7]), int(values[8])]
    else:
        return ["NA"]*4

def getFmax(file):
    if not os.path.isfile(file):
        return "NA"
    f = open(file)
    for line in f:
        if "Design Timing Summary" in line:
            break
    if "Design Timing Summary" in line:
        line = f.readline()
        line = f.readline()
        line = f.readline()
        line = f.readline()
        line = f.readline()
        line = f.readline()
        values = list(filter(lambda a: a != '', line.split(' ')))  
        fmax = 1000/(4-float(values[0]))
        return "{:.2f}".format(fmax)
    else:
        return "NA"

def print2DNode(file, useDSP, useCycles, row, ssIndex, dsIndex, dssIndex):
    if useDSP:
        area = row[6]
    else:
        area = row[5]
    if useCycles:
        delay = row[4]
    else:
        delay = row[10]
    if row[0] == "vecNormTrans_"+str(dsIndex):
        file.write("\\addplot[draw=dscolor, mark=*, fill=dscolor, nodes near coords={DS}, every node near coord/.style={anchor=south, align=center, color=dscolor}] coordinates {("+"{:.2f}".format(float(delay)/1000)+", "+str(area)+")};\t\t%"+row[0]+"\n")
        return
    elif row[0] == "vecNormTrans_"+str(ssIndex):
        file.write("\\addplot[draw=sscolor, mark=*, fill=sscolor, nodes near coords={SS}, every node near coord/.style={anchor=south, align=center, color=sscolor}] coordinates {("+"{:.2f}".format(float(delay)/1000)+", "+str(area)+")};\t\t%"+row[0]+"\n")
        return
    elif row[0] == "vecNormTrans_"+str(dssIndex):
        file.write("\\addplot[draw=dsscolor, mark=*, fill=dsscolor, nodes near coords={our work}, every node near coord/.style={anchor=south, align=center, color=dsscolor}] coordinates {("+"{:.2f}".format(float(delay)/1000)+", "+str(area)+")};\t\t%"+row[0]+"\n")
        return
    else:
        color = "dsscolor"
    if area != "NA" and delay != "NA":
        file.write("\\addplot[draw="+color+", mark=o] coordinates {("+"{:.2f}".format(float(delay)/1000)+", "+str(area)+")};\t\t%"+row[0]+"\n")

def print3DNode(file, useCycles, row, ssIndex, dsIndex, dssIndex):
    dsp = row[6]
    lut = row[5]
    if useCycles:
        delay = row[4]
    else:
        delay = row[10]
    if row[0] == "vecNormTrans_"+str(ssIndex) or row[0] == "vecNormTrans_"+str(dsIndex) or row[0] == "vecNormTrans_"+str(dssIndex):
        return

    if lut != "NA" and dsp != "NA" and delay != "NA":
        file.write("("+str(dsp)+", "+str(lut)+", "+str(delay)+") ")

def evaluateDesign(size, dir, threads, visualize, problemSize):
    path, dirs, files = next(os.walk(dir))
    dirCount = len(dirs) - 1
    ssIndex, dsIndex, dssIndex = generateSourceCode(size, dir, False)
    print("SS = "+str(ssIndex)+", DS = "+str(dsIndex)+", DSS = "+str(dssIndex))

    # estimateLoss = evaluateSource(problemSize)
    synthesisCount = 0
    cosimCount = 0
    implCount = 0
    headers = ["Name", "Synthesis", "Cosimulation", "Post synthesis", "Cycles", "LUTs", "DSPs", "Registers", "BRAM", "Fmax/MHz", "Time/us", "Estimate throughput"]
    content = []
    for i in range(0, dirCount):
        if i == dirCount - 1:
            row = ["vecNormTrans_"+str(i), "SUCCESS", "SUCCESS", "SUCCESS", 14361, 1454, 5, 1398, 0, "{:.2f}".format(float(1000/8.276)), "{:.2f}".format(14361/float(1000/8.276)), "NA"]
            content.append(row)
            synthesisCount = synthesisCount + 1
            cosimCount = cosimCount + 1
            implCount = implCount + 1
            continue

        row = []
        row.append("vecNormTrans_"+str(i))
        dassLog = dir + "/vecNormTrans_"+str(i)+"/dass.log"
        synthSuccess = not os.system("if [[ -f \""+dassLog+"\" ]]; then\n grep -Fxq \""+dassKey+"\" "+dassLog+"\n else\n exit 1 \nfi")
        if synthSuccess:
            row.append("SUCCESS")
            synthesisCount = synthesisCount + 1
        else:
            row.append("FAIL")
        
        simLog = dir + "/vecNormTrans_"+str(i)+"/sim/HLS_VERIFY/transcript"
        cosimSuccess = not os.system("if [[ -f \""+simLog+"\" ]]; then\n grep -Fxq \""+simKey+"\" "+simLog+"\n else\n exit 1 \nfi")
        if cosimSuccess:
            row.append("SUCCESS")
            cosimCount = cosimCount + 1
        else:
            row.append("FAIL")

        areaLog = dir + "/vecNormTrans_"+str(i)+"/syn_project/util.rpt"
        timingLog = dir + "/vecNormTrans_"+str(i)+"/syn_project/timing.rpt"
        implSuccess = not os.system("if [[ -f \""+areaLog+"\" ]]; then\n grep -Fq \""+areaKey+"\" "+areaLog+"\n else\n exit 1 \nfi") and \
                       not os.system("if [[ -f \""+timingLog+"\" ]]; then\n grep -Fq \""+timingKey+"\" "+timingLog+"\n else\n exit 1 \nfi")
        if implSuccess:
            row.append("SUCCESS")
            implCount = implCount + 1
        else:
            row.append("FAIL")

        cycles = getLatencyInCycles(simLog)
        row.append(cycles)
        
        lut, dsp, register, bram = getAreaInfo(areaLog)
        row = row + [lut, dsp, register, bram]

        fmax = getFmax(timingLog)
        row.append(fmax)

        if row[4] != "NA" and row[9] != "NA":
            absTime = float(row[4]) / float(row[9])
            if float(row[9]) == 0:
                print(row)
            row.append("{:.2f}".format(absTime))
        else:
            row.append("NA")
            
        # row.append(estimateLoss[i])
        content.append(row)

    tail = ['Best', 'NA', 'NA', 'NA']
    for i in range(4, len(headers)-1):
        if i == 9:
            t = 0
        else:
            t = 100000000000
        for j in range(1, len(content)):
            if content[j][i] == "NA":
                continue
            if i == 9:
                t = max(t, float(content[j][i]))
            else:
                t = min(t, float(content[j][i]))
                if i != 10:
                    t = int(t)
        tail.append(t)
        
    content.append(tail)
    print(tabulate(content, headers))

    print("Synthesis coverage: " + str(synthesisCount) + "/" + str(dirCount) + "\nCosimulation coverage: " + str(cosimCount) + "/" + str(dirCount)+"\nPost synthesis coverage: "+str(implCount) + "/" + str(dirCount))

    if visualize:
        tikzOut = open("output.tex", "w")
        # LUTs v.s. Wall clock time
        tikzOut.write("\definecolor{dscolor}{HTML}{e31a1c}\n\definecolor{dsscolor}{HTML}{bc80bd}\n\definecolor{unknown}{HTML}{bebada}\n\definecolor{sscolor}{HTML}{1f78b4}\n\\begin{tikzpicture}\n\\begin{axis}[\n    height=80mm,\n    width=80mm,\n    log ticks with fixed point,\n    % xtick = {0, 1, 2.5, 5},\n    % ytick = {0, 0.1, 0.25, 0.5, 1, 2},\n    xlabel={Wall clock time - $\mu$s},\n    ylabel={LUTs},\n    ymin=0, xmin=0,\n    % ymax=2, xmax=5,\n    scaled x ticks = false, \n  ]\n")
        for c in content[:len(content)-1]:
            print2DNode(tikzOut, False, False, c, ssIndex, dsIndex, dssIndex)
        tikzOut.write("\end{axis}\n\end{tikzpicture}\n")
        
        # DSPs v.s. Wall clock time
        tikzOut.write("\definecolor{dscolor}{HTML}{e31a1c}\n\definecolor{dsscolor}{HTML}{bc80bd}\n\definecolor{unknown}{HTML}{bebada}\n\definecolor{sscolor}{HTML}{1f78b4}\n\\begin{tikzpicture}\n\\begin{axis}[\n    height=80mm,\n    width=80mm,\n    log ticks with fixed point,\n    % xtick = {0, 1, 2.5, 5},\n    % ytick = {0, 0.1, 0.25, 0.5, 1, 2},\n    xlabel={Wall clock time - $\mu$s},\n    ylabel={DSPs},\n    ymin=0, xmin=0,\n    % ymax=2, xmax=5,\n    scaled x ticks = false, \n  ]\n")
        for c in content[:len(content)-1]:
            print2DNode(tikzOut, True, False, c, ssIndex, dsIndex, dssIndex)
        tikzOut.write("\end{axis}\n\end{tikzpicture}\n")

        # LUTs v.s. Cycles
        tikzOut.write("\definecolor{dscolor}{HTML}{e31a1c}\n\definecolor{dsscolor}{HTML}{bc80bd}\n\definecolor{unknown}{HTML}{bebada}\n\definecolor{sscolor}{HTML}{1f78b4}\n\\begin{tikzpicture}\n\\begin{axis}[\n    height=80mm,\n    width=80mm,\n    log ticks with fixed point,\n    % xtick = {0, 1, 2.5, 5},\n    % ytick = {0, 0.1, 0.25, 0.5, 1, 2},\n    xlabel={Cycles - k},\n    ylabel={LUTs},\n    ymin=0, xmin=0,\n    % ymax=2, xmax=5,\n    scaled x ticks = false, \n  ]\n")
        for c in content[:len(content)-1]:
            print2DNode(tikzOut, False, True, c, ssIndex, dsIndex, dssIndex)
        tikzOut.write("\end{axis}\n\end{tikzpicture}\n")
        
        # DSPs v.s. Cycles
        tikzOut.write("\definecolor{dscolor}{HTML}{e31a1c}\n\definecolor{dsscolor}{HTML}{bc80bd}\n\definecolor{sscolor}{HTML}{1f78b4}\n\\begin{tikzpicture}\n\\begin{axis}[\n    height=80mm,\n    width=80mm,\n    log ticks with fixed point,\n    % xtick = {0, 1, 2.5, 5},\n    % ytick = {0, 0.1, 0.25, 0.5, 1, 2},\n    xlabel={Cycles - k},\n    ylabel={DSPs},\n    ymin=0, xmin=0,\n    % ymax=2, xmax=5,\n    scaled x ticks = false, \n  ]\n")
        for c in content[:len(content)-1]:
            print2DNode(tikzOut, True, True, c, ssIndex, dsIndex, dssIndex)
        tikzOut.write("\end{axis}\n\end{tikzpicture}\n")

        markScale = "2"
        tikzOut.write("\definecolor{dscolor}{HTML}{e31a1c}\n\definecolor{dsscolor}{HTML}{bc80bd}\n\definecolor{sscolor}{HTML}{1f78b4}\n\\begin{tikzpicture}[thick,scale=0.93, every node/.style={scale=0.93}]\n\\begin{axis}[ylabel = {LUTs}, xlabel = {DSPs}, zlabel = {Cycles}]\n\\addplot3[only marks, scatter, mark=*, fill opacity=0.1] coordinates {\n")
        for c in content[:len(content)-1]:
            print3DNode(tikzOut, True, c, ssIndex, dsIndex, dssIndex)
        tikzOut.write("\n};\n")
        for row in content[:len(content)-1]:
            dsp = row[6]
            lut = row[5]
            if True:
                delay = row[4]
            else:
                delay = row[10]
            if lut != "NA" and dsp != "NA" and delay != "NA":
                if row[0] == "vecNormTrans_"+str(dsIndex):
                    tikzOut.write("\\addplot3[only marks, scatter, mark = triangle*, mark options={scale=3}, draw opacity=0] coordinates {("+str(dsp)+", "+str(lut)+", "+str(delay)+")}; %DS\n")
                elif row[0] == "vecNormTrans_"+str(ssIndex):
                    tikzOut.write("\\addplot3[only marks, scatter, mark = square*, mark options={scale=2}, draw opacity=0] coordinates {("+str(dsp)+", "+str(lut)+", "+str(delay)+")}; %SS\n")
                elif row[0] == "vecNormTrans_"+str(dssIndex):
                    tikzOut.write("\\addplot3[only marks, scatter, mark = diamond*, mark options={scale=3}, draw opacity=0] coordinates {("+str(dsp)+", "+str(lut)+", "+str(delay)+")}; %DASS\n")
        tikzOut.write("\n\end{axis}\n\end{tikzpicture}\n")

        tikzOut.write("\definecolor{dscolor}{HTML}{e31a1c}\n\definecolor{dsscolor}{HTML}{bc80bd}\n\definecolor{sscolor}{HTML}{1f78b4}\n\\begin{tikzpicture}[thick,scale=0.93, every node/.style={scale=0.93}]\n\\begin{axis}[ylabel = {LUTs}, xlabel = {DSPs}, zlabel = {Wall clock time - $\mu$s}]\n\\addplot3[only marks, scatter, mark=*, fill opacity=0.1] coordinates {\n")
        for c in content[:len(content)-1]:
            print3DNode(tikzOut, False, c, ssIndex, dsIndex, dssIndex)
        tikzOut.write("\n};\n")
        for row in content[:len(content)-1]:
            dsp = row[6]
            lut = row[5]
            if False:
                delay = row[4]
            else:
                delay = row[10]
            if lut != "NA" and dsp != "NA" and delay != "NA":
                if row[0] == "vecNormTrans_"+str(dsIndex):
                    tikzOut.write("\\addplot3[only marks, scatter, mark = triangle*, mark options={scale=3}, draw opacity=0] coordinates {("+str(dsp)+", "+str(lut)+", "+str(delay)+")}; %DS\n")
                elif row[0] == "vecNormTrans_"+str(ssIndex):
                    tikzOut.write("\\addplot3[only marks, scatter, mark = square*, mark options={scale=2}, draw opacity=0] coordinates {("+str(dsp)+", "+str(lut)+", "+str(delay)+")}; %SS\n")
                elif row[0] == "vecNormTrans_"+str(dssIndex):
                    tikzOut.write("\\addplot3[only marks, scatter, mark = diamond*, mark options={scale=3}, draw opacity=0] coordinates {("+str(dsp)+", "+str(lut)+", "+str(delay)+")}; %DASS\n")
        tikzOut.write("\n\end{axis}\n\end{tikzpicture}\n")

        tikzOut.write("\n\n\n")
        for row in content[:len(content)-1]:
            dsp = row[6]
            lut = row[5]
            if False:
                delay = row[4]
            else:
                delay = row[10]
            tikzOut.write("\\addplot+[only marks, scatter, mark=o, scatter src={"+str(lut)+"}] coordinates {("+str(dsp)+", "+str(delay)+")}; % "+row[0]+"\n")

        tikzOut.close()
        print("Tikz figures have been exported to "+os.getcwd()+"/output.tex")

#############################################################
#    Options
#############################################################

def confirm():
    # raw_input returns the empty string for "enter"
    yes = {'yes','y', 'ye'}
    no = {'no','n'}

    choice = input().lower()
    while choice not in yes and choice not in no:
        print("Please respond with 'y' or 'n'")
        choice = input().lower()

    if choice in yes:
        return True
    else:
        return False

def main():
    
    optparser = OptionParser()
    optparser.add_option("-v", "--visualize", action="store_true", dest="visualize",
                         default=False, help="Visualize results in tikz images")
    optparser.add_option("-o", "--output", dest="dir",
                         default="./staticSelections", help="Output diretory, default = ./staticSelections")
    optparser.add_option("-p", "--problem-size", dest="size",
                         default=1024, help="Problem size, default N = 1024")
    optparser.add_option("-j", "--threads", dest="threads",
                         default=1, help="threads, default = 1")
    optparser.add_option("-d", "--debug", dest="debugCase", default=-1,
                         help="Enable only a specific case")
    
    optparser.add_option("-g", "--codegen", action="store_true", dest="codegen", default=False,
                         help="C source generation")
    optparser.add_option("-s", "--synth", action="store_true", dest="synthesis", default=False,
                         help="Synthesize design")
    optparser.add_option("-c", "--cosim", action="store_true", dest="cosim", default=False,
                         help="Cosimulation")
    optparser.add_option("-i", "--post-synth", action="store_true", dest="postsynthesis", default=False,
                         help="Post synthesis design (get area results)")
    optparser.add_option("-e", "--evaluate", action="store_true", dest="evaluate", default=False,
                         help="Evaluate the results")
    optparser.add_option("-r", "--reset", action="store_true", dest="reset", default=False,
                         help="Reset. Clean all the files")
    optparser.add_option("-a", "--all", action="store_true", dest="runall", default=False,
                         help="Run all from scratch")
    optparser.add_option("-k", "--disable-skip", action="store_true", dest="disableskip", default=False,
                         help="Do not skip the existing cases and rerun all the cases.")
    

    (options, args) = optparser.parse_args()
    diretory = options.dir
    runall = options.runall
    threads = int(options.threads)
    debugCase = int(options.debugCase)
    disableskip = options.disableskip
    visualize = options.visualize

    start = time.time()
    if options.reset:
        print("This run will clean all the previously generated files. Are you sure you want to do this? [y/n]")
        if confirm():
            os.system("rm -rf " + diretory + "/*")
        else:
            sys.exit()

    if options.codegen or runall:
        print("--------------------------------------")
        print("  Code Generation ")
        print("--------------------------------------")
        startIn = time.time()
        generateSourceCode(int(options.size), diretory, True)
        endIn = time.time()
        print("CodeGen Time: "+"{:.2f}".format(endIn-startIn)+"s\n")

    if options.synthesis or runall:
        print("--------------------------------------")
        print("  Synthesis ")
        print("--------------------------------------")
        startIn = time.time()
        if debugCase == -1:
            synthDesign(diretory, threads, disableskip)
        else:
            synthDesignOnce(diretory, debugCase, disableskip)
        endIn = time.time()
        print("Synthesis Time: "+"{:.2f}".format(endIn-startIn)+"s\n")
        
    if options.cosim or runall:
        print("--------------------------------------")
        print("  Co-simulation ")
        print("--------------------------------------")
        startIn = time.time()
        if debugCase == -1:
            cosimDesign(diretory, threads, disableskip)
        else:
            cosimDesignOnce(diretory, debugCase, disableskip)
        endIn = time.time()
        print("Cosim Time: "+"{:.2f}".format(endIn-startIn)+"s\n")
    
    if options.postsynthesis or runall:
        print("--------------------------------------")
        print("  Post synthesis ")
        print("--------------------------------------")
        startIn = time.time()
        if debugCase == -1:
            implDesign(diretory, threads, disableskip)
        else:
            implDesignOnce(diretory, debugCase, disableskip)
        endIn = time.time()
        print("Post synthesis Time: "+"{:.2f}".format(endIn-startIn)+"s\n")

    if options.evaluate or runall:
        print("--------------------------------------")
        print("  Evaluation ")
        print("--------------------------------------")
        startIn = time.time()
        evaluateDesign(int(options.size), diretory, threads, visualize, int(options.size))
        endIn = time.time()
        print("Evaluate Time: "+"{:.2f}".format(endIn-startIn)+"s\n")

    end = time.time()
    print("Total Time: "+"{:.2f}".format(end-start)+"s\n")

if __name__ == '__main__':
    main()
