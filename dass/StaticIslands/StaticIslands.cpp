//--------------------------------------------------------//
// Pass: StaticInstrPass
// Identify and extracts a set of maximum islands for static scheduling. If an
// island is big enough (e.g. > 1 instruction), it will be transformed into a
// static function
//
// Pass: StaticMemoryLoopPass
// Check whether a loop is amenable for static scheduling based on the memory
// dependence. If it is, it will be extracted as a static function
//
// Pass: StaticControlLoopPass
// Check whether a loop is amenable for static scheduling based on the control
// flow and memory architecture. If it is, it will be transformed into a static
// function
//--------------------------------------------------------//

#include <algorithm>
#include <cassert>

#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"

#include "ElasticPass/Head.h"
#include "MyCFGPass/MyCFGPass.h"
#include "Nodes.h"

#include "StaticIslands.h"

using namespace llvm;

typedef std::map<ENode *, int> ENode_int_map;

int ssCount = 0;

cl::opt<double> opt_Loss("loss", cl::desc("Afforable performance loss"),
                         cl::Hidden, cl::init(0.05), cl::Optional);

//--------------------------------------------------------//
// Identify and merge islands
//--------------------------------------------------------//

static bool canMerge(ENode *enode) {
  // Do not merge branches
  if ((enode->type == Branch_ || enode->type == Branch_n ||
       enode->type == Branch_c) &&
      (enode->CntrlPreds->size() > 1 || enode->CntrlSuccs->size() > 1))
    return false;
  // Do not merge phi
  if ((enode->type == Phi_ || enode->type == Phi_c || enode->type == Phi_n) &&
      enode->CntrlPreds->size() > 1)
    return false;
  // Do not merge call
  if ((enode->type == Inst_) && isa<CallInst>(enode->Instr))
    return false;
  // Do not merge memory nodes
  if (enode->type == Inst_) {
    auto inst = enode->Instr;
    assert(inst);
    if (isa<LoadInst>(inst) || isa<StoreInst>(inst) ||
        isa<GetElementPtrInst>(inst))
      return false;
  }
  return true;
}

static bool canMerge(ENode *fromNode, ENode *toNode) {
  if (!canMerge(fromNode) || !canMerge(toNode))
    return false;
  // Do not merge fork with its outputs yet
  if (fromNode->type == Fork_c || fromNode->type == Fork_)
    return false;
  return true;
}

static void merge(ENode *fromNode, ENode *toNode, ENode_vec *enode_dag,
                  ENode_int_map &staticMap) {
  auto fromIsland = staticMap[fromNode];
  auto toIsland = staticMap[toNode];
  for (auto &enode : *enode_dag)
    if (staticMap[enode] == fromIsland)
      staticMap[enode] = toIsland;
}

static void addStaticIslands(ENode_vec *enode_dag, ENode_int_map &staticMap) {
  // Init
  auto islandCount = 0;
  for (auto enode : *enode_dag)
    staticMap[enode] = islandCount++;

  // Merge neighbors as an island
  for (auto enode : *enode_dag) {
    for (auto pred : *enode->CntrlPreds)
      if (canMerge(pred, enode))
        merge(pred, enode, enode_dag, staticMap);
    for (auto succs : *enode->CntrlSuccs)
      if (canMerge(enode, succs))
        merge(enode, succs, enode_dag, staticMap);
  }

  // Merge fork with successors iff all of them can be merged and are in the
  // same group
  // TODO: latency analysis on all these paths and decide whether it can be
  // merged
  for (auto enode : *enode_dag) {
    if (enode->type == Fork_ || enode->type == Fork_c) {
      bool toMerge = true;
      int island = -1;
      for (auto succs : *enode->CntrlSuccs) {
        if (island == -1)
          island = staticMap[succs];
        toMerge &= (canMerge(succs) && island == staticMap[succs]);
      }
      if (toMerge)
        for (auto succs : *enode->CntrlSuccs)
          merge(succs, enode, enode_dag, staticMap);
    }
  }
}

//--------------------------------------------------------//
// Evaluate islands and transform into functions
//--------------------------------------------------------//

struct islandNode {
  ENode_vec nodes;
  unsigned instSize = 0;
  // TODO: evaluate for resource sharing
  // unsigned intOps = 0;
  // unsigned intMul = 0;
  // unsigned intDiv = 0;
  // unsigned floatAdd = 0;
  // unsigned floatSub = 0;
  // unsigned floatMul = 0;
  // unsigned floatDiv = 0;
};

static islandNode *getIslands(int islandID, ENode_vec *enode_dag,
                              ENode_int_map &staticMap) {
  islandNode *island = new islandNode;
  for (auto &enode : *enode_dag)
    if (staticMap[enode] == islandID) {
      island->nodes.push_back(enode);
      if (enode->type == Inst_) {
        auto inst = enode->Instr;
        if (isa<llvm::BinaryOperator>(inst) || isa<llvm::CmpInst>(inst))
          island->instSize++;
      }
    }
  if (island->nodes.size() <= 1 || island->instSize <= 1) {
    delete island;
    return nullptr;
  } else
    return island;
}

static void appendArgs(Instruction *inst, std::vector<Value *> &inputs) {
  for (auto i = 0; i < inst->getNumOperands(); i++) {
    auto op = inst->getOperand(i);
    if (std::find(inputs.begin(), inputs.end(), op) == inputs.end())
      inputs.push_back(op);
  }
}

static void appendResults(Instruction *inst, std::vector<Value *> &results) {
  if (std::find(results.begin(), results.end(), inst) == results.end())
    results.push_back(inst);
}

static void getArgs(ENode *enode, std::vector<Value *> &inputs) {
  if (enode->type == Inst_)
    appendArgs(enode->Instr, inputs);
  else if (enode->type == Phi_ || enode->type == Phi_n ||
           enode->type == Phi_c) {
    assert(enode->CntrlSuccs->size() == 1);
    auto node = enode->CntrlSuccs->front();
    getArgs(node, inputs);
  } else if (enode->type == Fork_ || enode->type == Fork_c) {
    for (auto succs : *enode->CntrlSuccs)
      getArgs(succs, inputs);
  } else {
    llvm::errs() << enode->Name << "_" << enode->id << "\n";
    llvm_unreachable("Unsupported function argumenet.");
  }
}

Value *mirrorInst(Instruction *nextInst, ArrayRef<int> opIndices,
                  ArrayRef<Value *> ins, IRBuilder<> &builder) {
  switch (nextInst->getOpcode()) {
  case Instruction::Add:
    assert(opIndices.size() == 2);
    if (auto op = dyn_cast<OverflowingBinaryOperator>(nextInst))
      return builder.CreateAdd(ins[opIndices[0]], ins[opIndices[1]], "",
                               op->hasNoUnsignedWrap(), op->hasNoSignedWrap());
    else
      return builder.CreateAdd(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::Sub:
    assert(opIndices.size() == 2);
    if (auto op = dyn_cast<OverflowingBinaryOperator>(nextInst))
      return builder.CreateSub(ins[opIndices[0]], ins[opIndices[1]], "",
                               op->hasNoUnsignedWrap(), op->hasNoSignedWrap());
    else
      return builder.CreateSub(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::Mul:
    assert(opIndices.size() == 2);
    if (auto op = dyn_cast<OverflowingBinaryOperator>(nextInst))
      return builder.CreateMul(ins[opIndices[0]], ins[opIndices[1]], "",
                               op->hasNoUnsignedWrap(), op->hasNoSignedWrap());
    else
      return builder.CreateMul(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::SRem:
    return builder.CreateSRem(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::URem:
    return builder.CreateURem(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::Shl:
    assert(opIndices.size() == 2);
    if (auto op = dyn_cast<OverflowingBinaryOperator>(nextInst))
      return builder.CreateShl(ins[opIndices[0]], ins[opIndices[1]], "",
                               op->hasNoUnsignedWrap(), op->hasNoSignedWrap());
    else
      return builder.CreateShl(ins[opIndices[0]], ins[opIndices[1]]);

  case Instruction::SDiv:
    assert(opIndices.size() == 2);
    if (auto op = dyn_cast<PossiblyExactOperator>(nextInst)) {
      return builder.CreateSDiv(ins[opIndices[0]], ins[opIndices[1]], "", true);

    } else
      return builder.CreateSDiv(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::LShr:
    assert(opIndices.size() == 2);
    if (auto op = dyn_cast<PossiblyExactOperator>(nextInst)) {
      return builder.CreateLShr(ins[opIndices[0]], ins[opIndices[1]], "", true);

    } else
      return builder.CreateLShr(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::AShr:
    assert(opIndices.size() == 2);
    if (auto op = dyn_cast<PossiblyExactOperator>(nextInst)) {
      return builder.CreateAShr(ins[opIndices[0]], ins[opIndices[1]], "", true);

    } else
      return builder.CreateAShr(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::And:
    return builder.CreateAnd(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::Or:
    return builder.CreateOr(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::Xor:
    return builder.CreateXor(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::FMul:
    return builder.CreateFMul(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::FAdd:
    return builder.CreateFAdd(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::FSub:
    return builder.CreateFSub(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::FDiv:
    return builder.CreateFDiv(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::ICmp:
    return builder.CreateICmp(dyn_cast<CmpInst>(nextInst)->getPredicate(),
                              ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::FCmp:
    return builder.CreateFCmp(dyn_cast<CmpInst>(nextInst)->getPredicate(),
                              ins[opIndices[0]], ins[opIndices[1]]);

  case Instruction::ZExt:
    return builder.CreateZExt(ins[opIndices[0]], nextInst->getType());
  case Instruction::SExt:
    return builder.CreateSExt(ins[opIndices[0]], nextInst->getType());

  case Instruction::GetElementPtr: {
    std::vector<Value *> insList;
    for (auto i = 1; i < ins.size(); i++)
      insList.push_back(ins[opIndices[i]]);
    return builder.CreateGEP(ins[opIndices[0]], insList);
  }
  case Instruction::Store:
    return builder.CreateStore(ins[opIndices[0]], ins[opIndices[1]]);
  case Instruction::Load:
    return builder.CreateLoad(nextInst->getType(), ins[opIndices[0]]);
  case Instruction::Select:
    return builder.CreateSelect(ins[opIndices[0]], ins[opIndices[1]],
                                ins[opIndices[2]]);

  default:
    llvm::errs() << *nextInst << "\n";
    llvm_unreachable("Undefined instruction type for reconstruction.");
  }
};

static void functionalise(ENode_vec &enodes, int islandID,
                          ENode_int_map &staticMap, Function *F) {
  // Get function arguments, results and instructions
  std::vector<Value *> inputs, results;
  std::vector<Instruction *> insts;
  for (auto enode : enodes) {
    if (enode->type == Inst_)
      insts.push_back(enode->Instr);

    // Get input values
    bool isIn = false;
    for (auto preds : *enode->CntrlPreds) {
      if (staticMap[preds] != islandID) {
        isIn = true;
        break;
      }
    }
    if (isIn)
      getArgs(enode, inputs);

    // Get results
    bool isOut = false;
    for (auto succs : *enode->CntrlSuccs) {
      if (staticMap[succs] != islandID) {
        isOut = true;
        break;
      }
    }
    if (isOut) {
      if (enode->type == Inst_)
        appendResults(enode->Instr, results);
      else if (enode->type == Branch_n || enode->type == Branch_c ||
               enode->type == Branch_) {
        auto node = enode->CntrlPreds->front();
        assert(node->type == Inst_);
        appendResults(node->Instr, results);
      } else if (enode->type == Fork_ || enode->type == Fork_c) {
        auto node = enode->CntrlPreds->front();
        assert(node->type == Inst_);
        appendResults(node->Instr, results);
      } else {
        llvm::errs() << enode->Name << "_" << enode->id << "\n";
        llvm_unreachable("Unsupported function results.");
      }
    }
  }
  // Remove operands that defined in the function
  auto i = inputs.size() - 1;
  for (auto input = inputs.rbegin(); input != inputs.rend(); input++) {
    // Value defined yet
    if (std::find(insts.begin(), insts.end(), *input) != insts.end())
      inputs.erase(inputs.begin() + i);
    i--;
  }

  // Check basic block is the same - features for different basic blocks can be
  // added once an example has been found
  auto bb = insts.front()->getParent();
  for (auto inst : insts)
    assert(bb == inst->getParent());

  // Create function prototype
  std::vector<Type *> formalPorts;
  for (auto input : inputs)
    formalPorts.push_back(input->getType());
  for (auto result : results)
    formalPorts.push_back(PointerType::getUnqual(result->getType()));
  // false - function takes a constant number of arguments
  auto M = F->getParent();
  auto fname = "ssFunc_" + std::to_string(ssCount++);
  while (M->getFunction(fname))
    fname = "ssFunc_" + std::to_string(ssCount++);
  auto funcType = M->getOrInsertFunction(
      fname,
      FunctionType::get(Type::getVoidTy(M->getContext()), formalPorts, false));
  auto ssfunc = cast<Function>(funcType);
  ssfunc->setCallingConv(CallingConv::C);
  ssfunc->addFnAttr("dass_ss", "1");

  // Set basic block and arguments
  auto entry = BasicBlock::Create(M->getContext(), "entry", ssfunc);
  auto inSize = inputs.size(), outSize = results.size();
  std::vector<Value *> ins(inSize), outs(outSize);
  auto arg = ssfunc->arg_begin();
  for (auto i = 0; i < inSize; i++) {
    arg->setName("arg_in" + std::to_string(i));
    ins[i] = arg;
    arg++;
  }
  for (auto i = 0; i < outSize; i++) {
    arg->setName("arg_out" + std::to_string(i));
    outs[i] = arg;
    arg++;
  }

  // save it for creating call instruction
  auto callArgs = inputs;

  // Load all constants
  for (auto inst : insts)
    for (auto i = 0; i < inst->getNumOperands(); i++)
      if (auto op = dyn_cast<Constant>(inst->getOperand(i))) {
        inputs.push_back(op);
        ins.push_back(op);
      }

  // Insert instructions
  IRBuilder<> builder(entry);
  auto tempInsts = insts;
  while (insts.size() > 0) {
    Instruction *nextInst = nullptr;
    std::vector<int> opIndices;
    for (auto inst : insts) {
      auto ready = true;
      opIndices.clear();
      for (auto i = 0; i < inst->getNumOperands(); i++) {
        auto idx = std::find(inputs.begin(), inputs.end(), inst->getOperand(i));
        if (idx == inputs.end()) {
          ready = false;
          break;
        } else
          opIndices.push_back(idx - inputs.begin());
      }
      if (ready) {
        nextInst = inst;
        break;
      }
    }
    if (!nextInst) {
      for (auto inst : insts) {
        llvm::errs() << *inst << "\n";
      }
      llvm::errs() << insts[0]->getParent()->getParent()->getName() << "\n";
    }

    assert(nextInst);
    assert(opIndices.size() == nextInst->getNumOperands());

    auto newInst = mirrorInst(nextInst, opIndices, ins, builder);

    // Append to ready values
    inputs.push_back(nextInst);
    ins.push_back(newInst);

    // Store to the output pointer if it is a result
    auto outIdx = std::find(results.begin(), results.end(), nextInst);
    if (outIdx != results.end()) {
      auto idx = outIdx - results.begin();
      builder.CreateStore(newInst, outs[idx]);
    }
    insts.erase(std::find(insts.begin(), insts.end(), nextInst));
  }
  builder.CreateRet(nullptr);

  // Replace the inlined instructions with a function call
  builder.SetInsertPoint(&(F->getEntryBlock().front()));
  assert(outs.size() == results.size());
  // Create alloca
  for (auto i = 0; i < results.size(); i++) {
    outs[i] = builder.CreateAlloca(results[i]->getType());
    callArgs.push_back(outs[i]);
  }
  // Create call
  builder.SetInsertPoint(tempInsts[0]);
  builder.CreateCall(ssfunc->getFunctionType(), ssfunc, callArgs);
  // Create load
  for (auto i = 0; i < outs.size(); i++) {
    outs[i] = builder.CreateLoad(outs[i]);
    results[i]->replaceAllUsesWith(outs[i]);
  }
  // Erase all the instructions
  while (tempInsts.size() > 0) {
    auto done = false;
    for (auto tmpInst = tempInsts.rbegin(); tmpInst != tempInsts.rend();
         tmpInst++) {
      Instruction *inst = *tmpInst;
      if (inst->use_empty()) {
        tempInsts.erase(std::find(tempInsts.begin(), tempInsts.end(), inst));
        inst->eraseFromParent();
        done = true;
      }
    }
    if (!done) {
      llvm::errs() << ssfunc->getName().str() << "\n";
      llvm_unreachable("Cannot erase all the instruction for function ");
    }
  }
}

//--------------------------------------------------------//
// Pass declaration
//--------------------------------------------------------//

namespace {
class StaticInstrPass : public llvm::ModulePass {

public:
  static char ID;

  StaticInstrPass() : llvm::ModulePass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  bool runOnModule(Module &M) override;
};
} // namespace

void StaticInstrPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MyCFGPass>();
}

bool StaticInstrPass::runOnModule(Module &M) {
  std::vector<Function *> fList;
  for (auto &F : M) {
    auto fname = demangleFuncName(F.getName().str().c_str());
    // Also skip pre-defined static functions
    if (fname != "main" && !F.hasFnAttribute("dass_ss"))
      fList.push_back(&F);
  }

  for (auto F : fList) {
    // Get dot graph
    auto &cdfg = getAnalysis<MyCFGPass>(*F);

    // Extract Static Islands
    ENode_int_map staticMap;
    addStaticIslands(cdfg.enode_dag, staticMap);

    // Rewrite function
    for (int i = 0; i < cdfg.enode_dag->size(); i++)
      if (auto island = getIslands(i, cdfg.enode_dag, staticMap))
        functionalise(island->nodes, i, staticMap, F);
  }
  return true;
}

char StaticInstrPass::ID = 1;

static RegisterPass<StaticInstrPass>
    Z("get-static-instrs", "Creates new DASS code from DS", false, false);

//--------------------------------------------------------//
// Pass declaration
//--------------------------------------------------------//

static bool containsCallInst(Loop *loop) {
  for (auto BB : loop->getBlocks())
    for (auto I = BB->begin(); I != BB->end(); I++)
      if (isa<CallInst>(I))
        return true;
  return false;
}

static bool extractLoop(Loop *L, LoopInfo &LI, DominatorTree &DT) {
  Function &Func = *L->getHeader()->getParent();
  CodeExtractor Extractor(DT, *L);
  if (Extractor.extractCodeRegion()) {
    LI.erase(L);
    return true;
  }
  return false;
}

static void extractInnermostLoop(Loop *loop,
                                 std::vector<Loop *> &innermostLoops) {
  auto subLoops = loop->getSubLoops();
  if (subLoops.empty())
    innermostLoops.push_back(loop);
  else {
    auto subLoops = loop->getSubLoops();
    for (auto subloop : subLoops)
      extractInnermostLoop(subloop, innermostLoops);
  }
}

std::vector<Loop *> extractInnermostLoops(LoopInfo &LI) {
  std::vector<Loop *> innermostLoops;
  for (auto &loop : LI)
    extractInnermostLoop(loop, innermostLoops);
  return innermostLoops;
}

static Instruction *getLoopEnrtyBranch(Loop *loop) {
  auto branchInst = dyn_cast<BranchInst>(loop->getHeader()->getTerminator());
  if (!branchInst) {
    loop->dump();
    llvm_unreachable("Cannot find branch in the specified loop");
  }
  return dyn_cast<Instruction>(branchInst);
}

// TODO: temporary latency of the ENode, need to be sync with the latest dhls
static int getNodeLatency(ENode *node) {
  if (node->type != Inst_)
    return 0;

  auto inst = node->Instr;
  if (!inst)
    return 0;

  switch (inst->getOpcode()) {
  case Instruction::Mul:
    return 4;
  case Instruction::SRem:
    return 16;
  case Instruction::URem:
    return 16;
  case Instruction::SDiv:
    return 32;
  case Instruction::FMul:
    return 5;
  case Instruction::FAdd:
    return 4;
  case Instruction::FSub:
    return 4;
  case Instruction::FDiv:
    return 16;
  case Instruction::FCmp:
    return 1;
  case Instruction::Store:
    return 1;
  case Instruction::Load:
    return 2;
  default:
    return 0;
  }
}

struct CycleInfo {
  unsigned latency = 0;
  double probability = 1.0;
  bool valid = false;
  std::vector<BasicBlock *> bbpath;
};

static void
exploreCycles(int cycleID, llvm::DenseMap<int, CycleInfo *> &cycleMap,
              ENode *node, ENode *dst, ArrayRef<BasicBlock *> blocks,
              std::vector<BBNode *> *bbnode_dag, double totdalFreq) {
  assert(cycleMap.count(cycleID));
  // End of cycle
  if (node == dst) {
    cycleMap[cycleID]->valid = true;
    return;
  }

  int i = 0;
  for (auto succ : *node->CntrlSuccs) {
    // Skip the cycle that goes outside the loop
    if (!succ->BB ||
        std::find(blocks.begin(), blocks.end(), succ->BB) == blocks.end())
      continue;

    // Skip the cycle that connects a branch for another induction
    // variable
    if ((succ->type == Branch_n || succ->type == Branch_c ||
         succ->type == Branch_) &&
        succ->BB == dst->BB && succ != dst)
      continue;

    int id;
    // Duplicate cycle info for new cycles
    if (i != node->CntrlSuccs->size() - 1) {
      id = cycleMap.size();
      assert(!cycleMap.count(id));
      cycleMap[id] = new CycleInfo;
      cycleMap[id]->latency = cycleMap[cycleID]->latency;
      cycleMap[id]->probability = cycleMap[cycleID]->probability;
      cycleMap[id]->bbpath = cycleMap[cycleID]->bbpath;
    } else
      id = cycleID;

    cycleMap[id]->latency += getNodeLatency(succ);

    // Block transition
    if (succ->BB != node->BB) {
      cycleMap[id]->bbpath.push_back(succ->BB);

      assert(getBBNode(node->BB, bbnode_dag)
                 ->succ_freqs.count(succ->BB->getName()));
      double newP =
          getBBNode(node->BB, bbnode_dag)->get_succ_freq(succ->BB->getName()) /
          totdalFreq;

      assert(newP > 0 && newP <= 1);
      cycleMap[id]->probability = std::min(cycleMap[id]->probability, newP);
    }

    exploreCycles(id, cycleMap, succ, dst, blocks, bbnode_dag, totdalFreq);
    i++;
  }
}

static void extractCycleInfo(CycleInfo *cycleInfo,
                             std::vector<CycleInfo *> &finalCycles) {
  int i = 0;
  // Search for existing cycle with the same bb path
  int exist = -1;
  for (auto finalCycle : finalCycles) {
    if (finalCycle->bbpath == cycleInfo->bbpath) {
      finalCycles[i]->latency =
          std::max(finalCycles[i]->latency, cycleInfo->latency);
      assert(finalCycles[i]->probability == cycleInfo->probability);
      return;
    }
    i++;
  }
  finalCycles.push_back(cycleInfo);
}

static double getCycleLoss(ENode *src, ENode *dst,
                           ArrayRef<BasicBlock *> blocks,
                           std::vector<BBNode *> *bbnode_dag) {
  llvm::DenseMap<int, CycleInfo *> cycleMap;
  cycleMap[0] = new CycleInfo;
  double totdalFreq = 0;
  for (auto &freq : getBBNode(src->BB, bbnode_dag)->succ_freqs)
    totdalFreq += freq.second;

  exploreCycles(0, cycleMap, src, dst, blocks, bbnode_dag, totdalFreq);

  std::vector<CycleInfo *> finalCycles;
  for (auto &cycleInfo : cycleMap)
    if (cycleInfo.second->valid)
      extractCycleInfo(cycleInfo.second, finalCycles);

  double dynamicThroughput = 0, probVerify = 0;
  unsigned int staticThroughput = 0;
  for (auto cycleInfo : finalCycles) {
    llvm::errs() << cycleInfo->probability << " : " << cycleInfo->latency
                 << ", " << finalCycles.size() << "\n";
    dynamicThroughput += cycleInfo->latency * cycleInfo->probability;
    probVerify += cycleInfo->probability;
    staticThroughput = std::max(cycleInfo->latency, staticThroughput);
  }
  if (probVerify != 1.0) {
    llvm_unreachable(
        std::string("Loop probability is not 1: " + std::to_string(probVerify))
            .c_str());
  }

  double cycleLoss =
      ((double)staticThroughput - dynamicThroughput) / (dynamicThroughput + 1);

  llvm::errs() << "Throughput: " << dynamicThroughput + 1 << " / "
               << staticThroughput + 1 << "\n";
  llvm::errs() << "Loss: " << cycleLoss << "\n";

  return cycleLoss;
}

static double getThroughputLoss(Loop *loop, std::vector<ENode *> *enode_dag,
                                std::vector<BBNode *> *bbnode_dag) {
  auto header = loop->getHeader();
  auto latch = loop->getLoopLatch();
  auto blocks = loop->getBlocks();
  double loss = 0;

  // For each cycle
  for (auto node : *enode_dag) {
    if (node->type == Phi_ && node->BB == header) {
      for (auto pred : *node->CntrlPreds) {
        if (pred->type == Branch_n && pred->BB == latch) {
          auto cycleLoss = getCycleLoss(node, pred, blocks, bbnode_dag);
          loss = std::max(loss, cycleLoss);
        }
      }
    }
  }
  return loss;
}

static Loop *getOutermostPerfectLoop(Loop *loop, ScalarEvolution &SE) {
  // Get outermost perfect loop nest for static scheduling
  while (true) {
    auto parentLoop = loop->getParentLoop();
    if (!parentLoop)
      return loop;

    if (arePerfectlyNested(*parentLoop, *loop, SE))
      loop = parentLoop;
    else
      return loop;
  }
}

static void checkIfTopFunctionIsStatic(Module &M) {
  for (auto &Func : M) {
    auto F = &Func;
    auto fname = demangleFuncName(F->getName().str().c_str());
    if (fname == opt_top) {
      auto DT = llvm::DominatorTree(*F);
      LoopInfo LI(DT);
      if (!LI.empty())
        return;
      for (auto BB = F->begin(); BB != F->end(); BB++) {
        for (auto I = BB->begin(); I != BB->end(); I++) {
          if (auto callInst = dyn_cast<CallInst>(I)) {
            auto callee = dyn_cast<Function>(
                callInst->getOperand(callInst->getNumOperands() - 1));
            if (!callee || !callee->hasFnAttribute("dass_ss"))
              return;
          }
        }
      }
      F->addFnAttr("dass_ss", "0");
    }
  }
}

namespace {
class StaticControlLoopPass : public llvm::ModulePass {

public:
  static char ID;

  StaticControlLoopPass() : llvm::ModulePass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  bool runOnModule(Module &M) override;
};
} // namespace

void StaticControlLoopPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MyCFGPass>();
  // AU.addRequired<ScalarEvolutionWrapperPass>();
}

bool StaticControlLoopPass::runOnModule(Module &M) {
  std::vector<Function *> fList;
  for (auto &F : M) {
    auto fname = demangleFuncName(F.getName().str().c_str());
    // Also skip pre-defined static functions
    if (fname != "main" && !F.hasFnAttribute("dass_ss"))
      fList.push_back(&F);
  }

  for (auto F : fList) {
    // Get dot graph
    auto &cdfg = getAnalysis<MyCFGPass>(*F);

    // TODO: Fix the MyCFGGraph pass bug and add:
    // ScalarEvolutionWrapperPass &SEPass =
    //     getAnalysis<ScalarEvolutionWrapperPass>(*F);

    // Get loop information
    auto DT = llvm::DominatorTree(*F);
    LoopInfo LI(DT);

    auto innermostLoops = extractInnermostLoops(LI);

    for (auto &loop : innermostLoops) {
      // Skip loops that have function calls - cannot analyze function
      if (containsCallInst(loop))
        continue;

      auto branch = getLoopEnrtyBranch(loop);
      if (branch->getMetadata("dass_cdfg_check")) {
        auto loss = getThroughputLoss(loop, cdfg.enode_dag, cdfg.bbnode_dag);
        if (loss <= opt_Loss) {
          llvm::errs() << "Final Loss = " << loss << ": ";
          loop->dump();
          // TODO: Fix the MyCFGGraph pass bug and add:
          // oop = getOutermostPerfectLoop(loop, SE);

          // TODO: Insert metadata for loops, so Vitis HLS can pipeline them
          // well
          extractLoop(loop, LI, DT);
          (--M.end())->addFnAttr("dass_ss", "0");
        } else {
          llvm::errs() << "Skipped loop, final loss = " << loss << ": ";
          loop->dump();
        }
        branch->setMetadata("dass_cdfg_check", nullptr);
      }
    }
  }

  // If there is no dynamic loop in the top-level function and all the called
  // functions are static, the whole function is static
  checkIfTopFunctionIsStatic(M);
  return true;
}

char StaticControlLoopPass::ID = 1;

static RegisterPass<StaticControlLoopPass>
    Y("get-static-loops-cf", "Create static loops based on control flow", false,
      false);

//--------------------------------------------------------//
// Pass declaration
//--------------------------------------------------------//

namespace {
class StaticMemoryLoopPass : public llvm::ModulePass {

public:
  static char ID;

  StaticMemoryLoopPass() : llvm::ModulePass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  bool runOnModule(Module &M) override;
};
} // namespace

static bool isConstant(Value *value) { return isa<ConstantInt>(value); }
static bool isConstant(Value &value) { return isa<ConstantInt>(&value); }

/// Get the latch condition instruction.
static ICmpInst *getLatchCmpInst(const Loop *L) {
  if (BasicBlock *Latch = L->getLoopLatch())
    if (BranchInst *BI = dyn_cast_or_null<BranchInst>(Latch->getTerminator()))
      if (BI->isConditional())
        return dyn_cast<ICmpInst>(BI->getCondition());

  return nullptr;
}

// The induction variable of a static loop needs to be constant bounded:
// * The initial value has to be a constant
// * The final value has to be a constant
// * The step has to be a constant
// Return the induction variable if these three condition holds
Value *getConstantBoundedIndVar(Loop *loop, ScalarEvolution &SE) {

  auto header = loop->getHeader();
  assert(header && "Expected a valid loop header");
  auto CmpInst = getLatchCmpInst(loop);
  if (!CmpInst)
    return nullptr;

  // Final value has to be constant
  if (!isa<ConstantInt>(CmpInst->getOperand(0)) &&
      !isa<ConstantInt>(CmpInst->getOperand(1)))
    return nullptr;

  for (PHINode &Phi : header->phis()) {
    // We only handle integer inductions variables.
    if (!Phi.getType()->isIntegerTy())
      continue;

    // Skip complex induction variable
    if (Phi.getNumIncomingValues() != 2)
      continue;

    // Init value has to be constant
    if (!isa<ConstantInt>(Phi.getIncomingValue(0)) &&
        !isa<ConstantInt>(Phi.getIncomingValue(1)))
      continue;

    Instruction *LatchCmpOp0 = dyn_cast<Instruction>(CmpInst->getOperand(0));
    Instruction *LatchCmpOp1 = dyn_cast<Instruction>(CmpInst->getOperand(1));

    // Step has to be constant
    // Check that the PHI is consecutive.
    const SCEV *PhiScev = SE.getSCEV(dyn_cast<Value>(&Phi));
    const SCEVAddRecExpr *AR = dyn_cast<SCEVAddRecExpr>(PhiScev);

    if (!AR) {
      LLVM_DEBUG(dbgs() << "LV: PHI is not a poly recurrence.\n");
      continue;
    }

    if (AR->getLoop() != loop) {
      // FIXME: We should treat this as a uniform. Unfortunately, we
      // don't currently know how to handled uniform PHIs.
      LLVM_DEBUG(
          dbgs() << "LV: PHI is a recurrence with respect to an outer loop.\n");
      continue;
    }

    const SCEV *Step = AR->getStepRecurrence(SE);
    // Calculate the pointer stride and check if it is consecutive.
    // The stride may be a constant or a loop invariant integer value.
    const SCEVConstant *ConstStep = dyn_cast<SCEVConstant>(Step);
    if (!ConstStep) // && !SE.isLoopInvariant(Step, loop))
      continue;

    // case 1:
    // IndVar = phi[{InitialValue, preheader}, {StepInst, latch}]
    // StepInst = IndVar + step
    // cmp = StepInst < FinalValue
    // case 2:
    // IndVar = phi[{InitialValue, preheader}, {StepInst, latch}]
    // StepInst = IndVar + step
    // cmp = IndVar < FinalValue
    BasicBlock *Latch = AR->getLoop()->getLoopLatch();
    if (!Latch)
      continue;
    Instruction *StepInst =
        dyn_cast<Instruction>(Phi.getIncomingValueForBlock(Latch));
    if (StepInst == LatchCmpOp0 || StepInst == LatchCmpOp1 ||
        &Phi == LatchCmpOp0 || &Phi == LatchCmpOp1)
      return &Phi;
  }
  return nullptr;
}

static bool isAtMostStrideOne(Value *expr, Value *indvar) {
  // Stride zero
  if (isa<ConstantInt>(expr))
    return true;
  // No offset
  if (expr == indvar)
    return true;

  auto exprInst = dyn_cast<Instruction>(expr);
  if (exprInst) {
    if (exprInst->getOpcode() == Instruction::ZExt ||
        exprInst->getOpcode() == Instruction::SExt)
      return isAtMostStrideOne(exprInst->getOperand(0), indvar);

    if (exprInst->getOpcode() == Instruction::Add ||
        exprInst->getOpcode() == Instruction::Sub) {
      if ((exprInst->getOperand(0) == indvar &&
           isa<ConstantInt>(exprInst->getOperand(1))) ||
          (exprInst->getOperand(1) == indvar &&
           isa<ConstantInt>(exprInst->getOperand(0))))
        return true;
    }
  }
  return false;
}

// TODO: Support single dimension array so far - so we only have one induction
// variable
static bool areIndicesAtMostStrideOne(GetElementPtrInst *getElementPtrInst,
                                      Value *indvar) {
  if (!getElementPtrInst)
    return false;

  for (auto &address : getElementPtrInst->indices()) {
    if (isAtMostStrideOne(address, indvar))
      continue;
    return false;
  }
  return true;
}

static bool hasAtMostStridedOneMemory(Loop *loop, Value *indvar) {
  for (auto BB : loop->getBlocks()) {
    for (auto I = BB->begin(); I != BB->end(); I++) {
      auto loadInst = dyn_cast<LoadInst>(I);
      auto storeInst = dyn_cast<StoreInst>(I);
      if (!loadInst && !storeInst)
        continue;

      if (loadInst) {
        if (areIndicesAtMostStrideOne(
                dyn_cast<GetElementPtrInst>(loadInst->getPointerOperand()),
                indvar))
          continue;
      }
      if (storeInst) {
        if (areIndicesAtMostStrideOne(
                dyn_cast<GetElementPtrInst>(storeInst->getPointerOperand()),
                indvar))
          continue;
      }
      llvm::errs() << "Failed: " << *I << "\n";
      return false;
    }
  }
  return true;
}

void StaticMemoryLoopPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<ScalarEvolutionWrapperPass>();
  AU.addRequired<LoopAccessLegacyAnalysis>();
}

bool StaticMemoryLoopPass::runOnModule(Module &M) {
  std::vector<Function *> fList;
  auto ssCount = 0;

  for (auto &F : M) {
    auto fname = demangleFuncName(F.getName().str().c_str());
    // Also skip pre-defined static functions
    if (fname != "main" && !F.hasFnAttribute("dass_ss"))
      fList.push_back(&F);
  }

  for (auto F : fList) {

    // Get loop information
    auto DT = llvm::DominatorTree(*F);
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
    ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>(*F).getSE();
    LoopAccessLegacyAnalysis &LAA = getAnalysis<LoopAccessLegacyAnalysis>(*F);

    auto innermostLoops = extractInnermostLoops(LI);

    for (auto &loop : innermostLoops) {
      // Skip loops that have function calls - cannot analyze function
      if (containsCallInst(loop))
        continue;

      // Basic HLS loop requirements: constant loop bounds & unit-strided memory
      // accesses
      bool isLoopStatic = false;
      if (auto indvar = getConstantBoundedIndVar(loop, SE)) {
        if (hasAtMostStridedOneMemory(loop, indvar)) {
          if (loop->getNumBlocks() == 1)
            isLoopStatic = true;
          else if (LAA.getInfo(loop).getDepChecker().isSafeForVectorization()) {
            // TODO: That is a bug that MyCFGPass will interfer existing loop
            // passes in LLVM. So here we mark these loops and analyze them in a
            // separate pass.
            auto branch = getLoopEnrtyBranch(loop);
            branch->setMetadata("dass_cdfg_check",
                                MDNode::get(F->getContext(), None));
          }
        }
      }

      if (isLoopStatic) {
        // TODO: Insert metadata for loops, so Vitis HLS can pipeline them well

        loop = getOutermostPerfectLoop(loop, SE);
        llvm::errs() << "Functionalized loop: \n";
        loop->dump();
        extractLoop(loop, LI, DT);
        auto newF = (--M.end());
        newF->addFnAttr("dass_ss", "0");
        auto fname = "ssFunc_" + std::to_string(ssCount++);
        while (M.getFunction(fname))
          fname = "ssFunc_" + std::to_string(ssCount++);
        newF->setName(fname);
      }
    }
  }

  return true;
}

char StaticMemoryLoopPass::ID = 1;

static RegisterPass<StaticMemoryLoopPass>
    X("get-static-loops-mem", "Create static loops based on memory dependence",
      false, false);

/* for clang pass registration */
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static void registerClangPass(const PassManagerBuilder &,
                              legacy::PassManagerBase &PM) {
  PM.add(new StaticInstrPass());
  PM.add(new StaticMemoryLoopPass());
  PM.add(new StaticControlLoopPass());
}

static RegisterStandardPasses
    RegisterClangPass(PassManagerBuilder::EP_EarlyAsPossible,
                      registerClangPass);
