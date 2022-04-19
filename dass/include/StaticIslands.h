#pragma once

#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"

using namespace llvm;

extern cl::opt<std::string> opt_DASS;
extern cl::opt<std::string> opt_irDir;
extern cl::opt<bool> opt_hasIP;
extern cl::opt<std::string> opt_target;
extern cl::opt<std::string> opt_top;

// Mirror an instruction for a different set of inputs
Value *mirrorInst(Instruction *nextInst, ArrayRef<int> opIndices,
                  ArrayRef<Value *> ins, IRBuilder<> &builder);

bool arePerfectlyNested(Loop &OuterLoop, Loop &InnerLoop, ScalarEvolution &SE);

// The induction variable of a static loop needs to be constant bounded:
// * The initial value has to be a constant
// * The final value has to be a constant
// * The step has to be a constant
// Return the induction variable if these three condition holds
Value *getConstantBoundedIndVar(Loop *loop, ScalarEvolution &SE);

// Get all the innermost loops
std::vector<Loop *> extractInnermostLoops(LoopInfo &LI);