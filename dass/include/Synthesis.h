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
extern cl::opt<std::string> opt_rtl;
extern cl::opt<std::string> opt_top;
extern cl::opt<bool> opt_offset;
