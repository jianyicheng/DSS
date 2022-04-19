//--------------------------------------------------------//
// Pass: LoadOffsetCFGPass
// Load the offset of SS components and export a CFG, where each SS component
// may consists of multiple nodes. This replaces the joint input assumption
// for SS components with offsets.
//
// Pass: RemoveCallDummyPass
// Remove the call dummies as offset constraints in the buffered graph, so the
// CFG can be used for synthesis.
//
// Pass: BufferIfStmtPass
// This pass a fix to the buffering tool in Dynamatic that does not balance the
// throughput of two if branches after the if statements are optimized using
// short paths. This pass reads the buffer log and identify the throughput
// difference of two branches and inserts buffers for balancing.
//--------------------------------------------------------//

#include "llvm/IR/Constant.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "ElasticPass/Head.h"
#include "ElasticPass/Memory.h"
#include "ElasticPass/Utils.h"
#include "MyCFGPass/MyCFGPass.h"
#include "Nodes.h"

#include "AutopilotParser.h"
#include "Synthesis.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace llvm;
using namespace AutopilotParser;

//--------------------------------------------------------//
// Pass declaration: LoadOffsetCFGPass
//--------------------------------------------------------//

static int getMaxInputOffsetDepth(AutopilotParser::AutopilotParser *ap) {
  int offsetDepth = 0;
  auto &portInfo = ap->getPortInfo();
  for (auto i = 0; i < ap->getFunction()->arg_size(); i++) {
    if (portInfo[i]->getType() == INPUT)
      offsetDepth = std::max(offsetDepth, portInfo[i]->getFIFODepth());
  }
  return offsetDepth;
}

static void insertCallNodeBefore(int in, ENode *enodeIn, ENode *enode,
                                 int latency, ENode_vec *enodes) {
  // Create new type called dummy?
  auto callNode = new ENode(Inst_, enode->BB);
  callNode->Name = "dummy";
  callNode->Instr = enode->Instr;
  callNode->isMux = false;
  callNode->isCntrlMg = false;
  callNode->id = enodes->size();
  callNode->CntrlPreds->push_back(enodeIn);
  callNode->CntrlSuccs->push_back(enode);
  callNode->bbNode = enode->bbNode;
  callNode->latency = latency;
  callNode->ii = std::min(enode->ii, latency + 1);
  callNode->bbId = enode->bbId;

  auto predSucc = enodeIn->CntrlSuccs;
  predSucc->at(std::find(predSucc->begin(), predSucc->end(), enode) -
               predSucc->begin()) = callNode;
  auto enodePreds = enode->CntrlPreds;
  enodePreds->at(std::find(enodePreds->begin(), enodePreds->end(), enodeIn) -
                 enodePreds->begin()) = callNode;

  enodes->push_back(callNode);
}

static void insertOffsetBuff(int depth, ENode *enode, ENode_vec *enodes,
                             AutopilotParser::AutopilotParser *ap) {
  unsigned in = 0;
  auto &portInfo = ap->getPortInfo();
  for (auto i = 0; i < ap->getFunction()->arg_size(); i++) {
    if (portInfo[i]->getType() == INPUT) {
      int latency = portInfo[i]->getFIFODepth();
      assert(latency >= 0);
      if (latency) {
        auto portIndex = portInfo[i]->getPortIndex();
        assert(portIndex != -1);
        insertCallNodeBefore(in, enode->CntrlPreds->at(portIndex), enode,
                             latency, enodes);
      }
      in++;
    }
  }
  enode->latency = enode->latency - depth;
  enode->ii = std::min(enode->ii, enode->latency + 1);
}

static void splitCallNodes(
    ENode_vec *enodes,
    llvm::DenseMap<Function *, AutopilotParser::AutopilotParser *> &aps) {
  std::vector<ENode *> callNodes;

  for (auto enode : *enodes)
    if (enode->type == Inst_)
      if (auto callInst = dyn_cast_or_null<CallInst>(enode->Instr))
        if (dyn_cast<CallInst>(callInst)->getCalledFunction()->hasFnAttribute(
                "dass_ss"))
          callNodes.push_back(enode);

  for (auto enode : callNodes) {
    auto func = dyn_cast<CallInst>(enode->Instr)->getCalledFunction();
    assert(aps.count(func));
    if (aps[func]->getNumInputs() != enode->CntrlPreds->size())
      llvm_unreachable(
          std::string("Mismatched number of inputs to function " +
                      func->getName().str() + " in the function map: " +
                      std::to_string(aps[func]->getNumInputs()) + ", " +
                      std::to_string(enode->CntrlPreds->size()))
              .c_str());
    aps[func]->analyzePortIndices(enode, enodes);
    auto depth = getMaxInputOffsetDepth(aps[func]);
    insertOffsetBuff(depth, enode, enodes, aps[func]);
  }
}

static void addTimingInfo(
    ENode_vec *enodes,
    llvm::DenseMap<Function *, AutopilotParser::AutopilotParser *> &aps) {
  for (auto enode : *enodes) {
    if (!enode->Instr)
      continue;
    if (!isa<CallInst>(enode->Instr) || enode->type != Inst_)
      continue;

    auto callInst = dyn_cast<CallInst>(enode->Instr);
    auto F = callInst->getCalledFunction();
    if (!aps.count(F)) {
      auto fname = F->getName().str();
      auto fileName = opt_irDir + "/" + fname + "/solution1/.autopilot/db/" +
                      fname + ".verbose.sched.rpt";
      std::ifstream schedRpt(fileName);
      if (!schedRpt.is_open())
        llvm_unreachable(std::string("Schedule report of function " + fname +
                                     " not found: " + fileName)
                             .c_str());
      AutopilotParser::AutopilotParser *ap =
          new AutopilotParser::AutopilotParser(schedRpt, F);
      ap->anlayzePortInfo(opt_offset);
      aps[F] = ap;
    }
    enode->ii = aps[F]->getII();
    enode->latency = aps[F]->getLatency();
  }
}

namespace {
class LoadOffsetCFGPass : public llvm::ModulePass {

public:
  static char ID;

  LoadOffsetCFGPass() : llvm::ModulePass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  bool runOnModule(Module &M) override;
};
} // namespace

void LoadOffsetCFGPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MyCFGPass>();
}

bool LoadOffsetCFGPass::runOnModule(Module &M) {

  llvm::DenseMap<Function *, AutopilotParser::AutopilotParser *> aps;

  for (auto &F : M) {
    if (F.hasFnAttribute("dass_ss") || F.getName() == "main" || F.empty())
      continue;

    // Get dot graph
    auto &cdfg = getAnalysis<MyCFGPass>(F);

    // Add latencies and IIs into call nodes
    addTimingInfo(cdfg.enode_dag, aps);

    // Split call nodes with offsets
    splitCallNodes(cdfg.enode_dag, aps);

    // Export CDFG
    auto fname = demangleFuncName(F.getName().str().c_str());
    printDotDFG(cdfg.enode_dag, cdfg.bbnode_dag, fname + ".dot", true);
    printDotCFG(cdfg.bbnode_dag, (fname + "_bbgraph.dot").c_str());
  }
  return true;
}

//--------------------------------------------------------//
// Pass declaration: RemoveCallDummyPass
//--------------------------------------------------------//

namespace {
class RemoveCallDummyPass : public llvm::ModulePass {

public:
  static char ID;

  RemoveCallDummyPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool RemoveCallDummyPass::runOnModule(Module &M) {

  for (auto &F : M) {
    if (F.hasFnAttribute("dass_ss") || F.getName() == "main" || F.empty())
      continue;

    auto fname = demangleFuncName(F.getName().str().c_str());
    auto cmd = "python3 " + opt_DASS +
               "/dass/scripts/DynamaticOptimizer.py -d " + fname +
               "_graph_buf.dot";
    system(cmd.c_str());
  }
  return true;
}

//--------------------------------------------------------//
// Pass declaration: BufferIfStmtPass
//--------------------------------------------------------//

struct IfBlock {
  BasicBlock *condBB, *trueBB, *falseBB, *exitBB;
};

static bool touchMemory(BasicBlock *BB, ENode_vec *enode_dag) {
  if (!BB)
    return false;
  for (auto enode : *enode_dag)
    if (enode->Instr && enode->type == Inst_ && enode->BB == BB)
      if (isa<LoadInst>(enode->Instr) || isa<StoreInst>(enode->Instr))
        return true;
  return false;
}

static std::vector<IfBlock *> getInnerMostIfBlocks(Function *F,
                                                   ENode_vec *enode_dag) {
  std::vector<IfBlock *> ifBlocks;
  for (auto BB = F->begin(); BB != F->end(); BB++) {
    auto branchInst = dyn_cast<BranchInst>(BB->getTerminator());
    if (!branchInst || !branchInst->isConditional())
      continue;

    auto bb0 = branchInst->getSuccessor(0);
    auto bb1 = branchInst->getSuccessor(1);
    auto br0 = dyn_cast<BranchInst>(bb0->getTerminator());
    auto br1 = dyn_cast<BranchInst>(bb1->getTerminator());

    // TODO: What if the exit block terminates with ret?
    if (!br0 || !br1)
      continue;

    // We are looking for inner most if statements, which has the structure of:
    // BB1 -> BB2 (+ BB3) -> BB4. At least one of the successor should have
    // unconditional branch
    if (br0->isConditional() && br1->isConditional())
      continue;
    if (!br0->isConditional() && !br1->isConditional() &&
        br0->getSuccessor(0) != br1->getSuccessor(0))
      continue;

    BasicBlock *trueBB, *falseBB, *exitBB;
    if (!br0->isConditional()) {
      trueBB = bb0;
      exitBB = br0->getSuccessor(0);
      if (br1->isConditional() && exitBB != bb1)
        continue;
      if (!br1->isConditional() && br1->getSuccessor(0) != exitBB)
        continue;
      if (!br1->isConditional() && br1->getSuccessor(0) == exitBB)
        falseBB = bb1;
    } else {
      exitBB = bb0;
      if (br1->getSuccessor(0) != exitBB)
        continue;
      falseBB = bb1;
    }
    if (touchMemory(trueBB, enode_dag) || touchMemory(falseBB, enode_dag))
      continue;

    IfBlock *ifBlock = new IfBlock;
    ifBlock->condBB = &*BB;
    ifBlock->trueBB = trueBB;
    ifBlock->falseBB = falseBB;
    ifBlock->exitBB = exitBB;
    ifBlocks.push_back(ifBlock);
  }
  return ifBlocks;
}

struct MG {
  std::vector<std::pair<int, int>> edges;
  int iteration = -1;
  double throughput = -1;
};

static std::vector<MG *> getMarkedGraphInfo(std::string fileName) {
  std::vector<std::string> buffLog;
  std::ifstream ifile(fileName);
  if (!ifile.is_open())
    llvm_unreachable(
        std::string("Cannot find dot file " + fileName + ".\n").c_str());

  std::string line;
  while (std::getline(ifile, line))
    buffLog.push_back(line);
  ifile.close();

  std::vector<MG *> mgs;

  for (auto i = 0; i < buffLog.size(); i++) {
    if (buffLog[i].find("Total MILP time") != std::string::npos)
      break;

    if (buffLog[i].find("Iteration") != std::string::npos) {
      auto mg = new MG;
      mg->iteration =
          std::stoi(buffLog[i].substr(buffLog[i].find("Iteration") +
                                      std::string("Iteration").length())) -
          1;
      i++;
      i++;
      i++;
      while (buffLog[i].find("->") != std::string::npos) {
        auto src = std::stoi(buffLog[i].substr(0, buffLog[i].find("->")));
        auto dst = std::stoi(buffLog[i].substr(buffLog[i].find("->") + 2,
                                               buffLog[i].find(":") -
                                                   buffLog[i].find("->") - 2));
        mg->edges.push_back(std::pair<int, int>(src, dst));
        i++;
      }
      if (mg->edges.size() > 0)
        mgs.push_back(mg);
    }

    if (buffLog[i].find("Throughput achieved in sub MG") != std::string::npos) {
      auto index = std::stoi(
          buffLog[i].substr(buffLog[i].find("MG") + 2,
                            buffLog[i].find(":") - buffLog[i].find("MG") - 2));
      auto throughput = std::stof(buffLog[i].substr(
          buffLog[i].find(":") + 1,
          buffLog[i].rfind("***") - buffLog[i].find(":") - 1));
      for (auto j = 0; j < mgs.size(); j++)
        if (mgs[j]->iteration == index) {
          mgs[j]->throughput = throughput;
          break;
        }
    }
  }

  for (auto mg : mgs) {
    llvm::errs() << "Loop " << mg->iteration << ": (";
    for (auto edge : mg->edges)
      llvm::errs() << edge.first << "->" << edge.second << ", ";
    llvm::errs() << "), throughput = " << mg->throughput << "\n";
  }

  for (auto mg : mgs)
    assert(mg->throughput != -1);

  return mgs;
}

static std::vector<std::string> getDotGraph(std::string fileName) {
  std::vector<std::string> dotGraph;
  std::ifstream ifile(fileName);
  if (!ifile.is_open())
    llvm_unreachable(
        std::string("Cannot find dot file " + fileName + ".\n").c_str());

  std::string line;
  while (std::getline(ifile, line))
    dotGraph.push_back(line);
  ifile.close();
  return dotGraph;
}

static int getBBIndex(BasicBlock *BB, ENode_vec *enode_dag) {
  if (!BB)
    return -1;
  for (auto enode : *enode_dag)
    if (enode->BB == BB)
      return getBBIndex(enode);
  return -1;
}

static double getMGThroughput(std::pair<int, int> edge, ArrayRef<MG *> mgs) {
  auto count = 0;
  double throughput = -1;
  for (auto mg : mgs)
    if (std::find(mg->edges.begin(), mg->edges.end(), edge) !=
        mg->edges.end()) {
      throughput = mg->throughput;
      count++;
    }
  if (count != 1)
    llvm::errs() << "Warning found edge " << edge.first << "->" << edge.second
                 << " in multiple marked graphs. Throughput = " << throughput
                 << "\n";
  return throughput;
}

static bool insertBuffer(std::string buffer, std::string branch,
                         std::vector<std::string> &dotGraph,
                         std::string &bufferDeclare, int depth,
                         std::string port) {
  for (int i = 0; i < dotGraph.size(); i++)
    if (dotGraph[i].find("-> " + branch) != std::string::npos &&
        dotGraph[i].find("to=" + port) != std::string::npos) {
      if (dotGraph[i].find("_Buffer_") != std::string::npos) {
        auto srcBuffer = dotGraph[i].substr(
            dotGraph[i].find("_"),
            dotGraph[i].find(" ", dotGraph[i].find("_") + 1) -
                dotGraph[i].find("_"));
        int j = 0;
        for (j = 0; j < i; j++)
          if (dotGraph[j].find(srcBuffer + " [") != std::string::npos)
            break;
        auto slots = std::stoi(dotGraph[j].substr(
            dotGraph[j].find("slots=") + 6,
            dotGraph[j].find(", ", dotGraph[j].find("slots=")) -
                dotGraph[j].find("slots=") - 6));
        dotGraph[j].replace(dotGraph[j].find("slots=") + 6,
                            dotGraph[j].find(", ", dotGraph[j].find("slots=")) -
                                dotGraph[j].find("slots=") - 6,
                            std::to_string(slots + depth));
        dotGraph[j].replace(
            dotGraph[j].find("[", dotGraph[j].find("slots=")) + 1,
            dotGraph[j].find("t]", dotGraph[j].find("slots=")) -
                dotGraph[j].find("[", dotGraph[j].find("slots=")) - 1,
            std::to_string(slots + depth));
        llvm::errs() << "Expand buffer: " << srcBuffer << " -> " << branch
                     << " ";
        return false;
      } else {
        auto line0 = dotGraph[i];
        auto line1 = dotGraph[i];
        line0.replace(line0.find(branch), branch.length(), buffer);
        line0.replace(line0.find(port) + 2, 1, "1");
        auto src = line1.substr(0, line1.find("->") - 1);
        line1.replace(0, line1.find("->") - 1, buffer);
        line1.replace(line1.find("out") + 3, 1, "1");
        dotGraph[i] = line0 + "\n" + line1;
        llvm::errs() << "Inserted buffer: " << src << " -> " << buffer << " -> "
                     << branch << " ";
        return true;
      }
    }
  llvm_unreachable(std::string("Cannot find the condition edge of branch " +
                               branch + " in the dot graph.")
                       .c_str());
}

static void tryInsertBuffer(ENode *enode, int nextBB, int condBB, int depth,
                            int &bufferCount, std::string &bufferDeclare,
                            std::vector<std::string> &dotGraph,
                            ENode_vec *enode_dag) {
  // bool containsEdge = false;
  // for (auto succ : *enode->CntrlSuccs)
  //   if (getBBIndex(succ) == nextBB)
  //     containsEdge = true;
  // for (auto succ : *enode->JustCntrlSuccs)
  //   if (getBBIndex(succ) == nextBB)
  //     containsEdge = true;
  // if (!containsEdge)
  //   return;

  auto branch = getNodeDotNameNew(enode);
  branch = branch.substr(1, branch.rfind("\"") - 1);

  // Buffer both branches if it is a short path decision branch
  if (enode->CntrlPreds->size() == 2) {
    auto in0 = enode->CntrlPreds->at(0);
    if (in0->type == Fork_ || in0->type == Fork_c)
      in0 = in0->CntrlPreds->at(0);
    auto in1 = enode->CntrlPreds->at(1);
    if (in1->type == Fork_ || in1->type == Fork_c)
      in1 = in1->CntrlPreds->at(0);
    assert(in0->type == Branch_ || in1->type == Branch_);
    if (in0->type == Branch_ && in1->type == Branch_) {
      auto newBuffer0 =
          insertBuffer("_Buffer_x" + std::to_string(bufferCount), branch,
                       dotGraph, bufferDeclare, depth, "in1");
      if (newBuffer0) {
        bufferDeclare +=
            "_Buffer_x" + std::to_string(bufferCount) +
            " [type=Buffer, in=\"in1:1\", out=\"out1:1\", bbID = " +
            std::to_string(condBB) + ", slots=" + std::to_string(depth) +
            ", transparent=true, label=\"_Buffer_x" +
            std::to_string(bufferCount) + " [" + std::to_string(depth) +
            "t]\",  shape=box, style=filled, "
            "fillcolor=darkolivegreen3, "
            "height = 0.4];\n";
        bufferCount++;
      }
    }
  }

  auto newBuffer = insertBuffer("_Buffer_x" + std::to_string(bufferCount),
                                branch, dotGraph, bufferDeclare, depth, "in2");
  if (newBuffer) {
    bufferDeclare +=
        "_Buffer_x" + std::to_string(bufferCount) +
        " [type=Buffer, in=\"in1:1\", out=\"out1:1\", bbID = " +
        std::to_string(condBB) + ", slots=" + std::to_string(depth) +
        ", transparent=true, label=\"_Buffer_x" + std::to_string(bufferCount) +
        " [" + std::to_string(depth) + "t]\",  shape=box, style=filled, "
                                       "fillcolor=darkolivegreen3, "
                                       "height = 0.4];\n";
    bufferCount++;
  }
  llvm::errs() << ", depth = " << depth << "\n";
}

static void bufferIfStmt(ArrayRef<IfBlock *> ifBlocks, ArrayRef<MG *> mgs,
                         std::vector<std::string> &dotGraph,
                         ENode_vec *enode_dag) {
  for (auto ifBlock : ifBlocks) {
    auto condBB = getBBIndex(ifBlock->condBB, enode_dag);
    auto trueBB = getBBIndex(ifBlock->trueBB, enode_dag);
    auto falseBB = getBBIndex(ifBlock->falseBB, enode_dag);
    auto exitBB = getBBIndex(ifBlock->exitBB, enode_dag);

    auto edge0 = (trueBB == -1) ? std::pair<int, int>(condBB, exitBB)
                                : std::pair<int, int>(condBB, trueBB);
    auto edge1 = (falseBB == -1) ? std::pair<int, int>(condBB, exitBB)
                                 : std::pair<int, int>(condBB, falseBB);
    assert(edge0 != edge1);
    double t0 = getMGThroughput(edge0, mgs);
    double t1 = getMGThroughput(edge1, mgs);

    // One of the branches has a probability of 0
    // or two branches have the same throughput
    if (t0 == -1 || t1 == -1 || t0 == t1)
      continue;

    auto ii0 = std::ceil(1.0 / t0);
    auto ii1 = std::ceil(1.0 / t1);
    auto nextBB = (ii0 < ii1) ? edge1.second : edge0.second;
    auto depth = (int)std::abs(ii0 - ii1);

    llvm::errs() << "Buffering edge : " << condBB << " -> " << nextBB << "\n";

    std::string bufferDeclare = "";

    ENode *longPathConditionNode;
    for (auto enode : *enode_dag)
      if (enode->BB == ifBlock->condBB && enode->type == Branch_ &&
          enode->Instr) {
        longPathConditionNode = enode;
        break;
      }
    assert(longPathConditionNode);
    longPathConditionNode =
        (longPathConditionNode->CntrlSuccs->at(0)->type == Fork_)
            ? longPathConditionNode->CntrlSuccs->at(0)
            : longPathConditionNode;

    auto bufferCount = 0;
    for (auto enode : *longPathConditionNode->CntrlSuccs)
      tryInsertBuffer(enode, nextBB, condBB, depth, bufferCount, bufferDeclare,
                      dotGraph, enode_dag);
    for (auto enode : *longPathConditionNode->JustCntrlSuccs)
      tryInsertBuffer(enode, nextBB, condBB, depth, bufferCount, bufferDeclare,
                      dotGraph, enode_dag);

    bool foundBB = false;
    for (auto i = 0; i < dotGraph.size(); i++)
      if (dotGraph[i].find("label = \"block" + std::to_string(condBB) + "\"") !=
          std::string::npos) {
        dotGraph[i] += "\n" + bufferDeclare;
        foundBB = true;
        break;
      }
    assert(foundBB);
  }
}

namespace {
class BufferIfStmtPass : public llvm::ModulePass {

public:
  static char ID;

  BufferIfStmtPass() : llvm::ModulePass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  bool runOnModule(Module &M) override;
};
} // namespace

void BufferIfStmtPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MyCFGPass>();
}

bool BufferIfStmtPass::runOnModule(Module &M) {

  auto mgs = getMarkedGraphInfo("./buff.log");
  if (mgs.size() == 0)
    return true;

  for (auto &F : M) {
    if (F.hasFnAttribute("dass_ss") || F.getName() == "main" || F.empty())
      continue;

    auto &cdfg = getAnalysis<MyCFGPass>(F);
    auto ifBlocks = getInnerMostIfBlocks(&F, cdfg.enode_dag);
    if (ifBlocks.size() == 0)
      return true;

    auto fname = demangleFuncName(F.getName().str().c_str());
    auto dotGraph = getDotGraph(fname + "_graph_buf_new.dot");

    bufferIfStmt(ifBlocks, mgs, dotGraph, cdfg.enode_dag);

    system(
        ("mv " + fname + "_graph_buf_new.dot " + fname + "_graph_buf_new.dot_")
            .c_str());
    std::ofstream ofile(fname + "_graph_buf_new.dot");
    for (auto &s : dotGraph)
      ofile << s << "\n";
    ofile.close();
  }
  return true;
}

//--------------------------------------------------------//
// Pass registration
//--------------------------------------------------------//

char LoadOffsetCFGPass::ID = 1;
static RegisterPass<LoadOffsetCFGPass>
    X0("load-offset-cfg", "Create a CFG with offsets", false, false);

char RemoveCallDummyPass::ID = 2;
static RegisterPass<RemoveCallDummyPass>
    X1("remove-call-dummy", "Insert Buffers to CFG", false, false);

char BufferIfStmtPass::ID = 3;
static RegisterPass<BufferIfStmtPass>
    X2("buff-if-stmt", "Buffer to balance if branches", false, false);
