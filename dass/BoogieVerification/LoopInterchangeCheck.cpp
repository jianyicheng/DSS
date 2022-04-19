//--------------------------------------------------------//
// Pass: LoopInterchangeCheckPass
// Check if a loop can be interchanged. If yes, find the maximum depth of
// interchanging.
//--------------------------------------------------------//

#include <cassert>
#include <fstream>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "BoogieVerification.h"

using namespace llvm;

static cl::opt<int> opt_upper("upperbound", cl::desc("Upper bound of search"),
                              cl::Hidden, cl::init(10), cl::Optional);
static cl::opt<int> opt_lower("lowerbound", cl::desc("Lower bound of search"),
                              cl::Hidden, cl::init(1), cl::Optional);

//--------------------------------------------------------//
// Pass declaration: LoopInterchangeCheckPass
//--------------------------------------------------------//

static bool containsCallInst(Function *F) {
  for (auto BB = F->begin(); BB != F->end(); BB++)
    for (auto I = BB->begin(); I != BB->end(); I++)
      if (isa<CallInst>(I))
        return true;
  return false;
}

static std::string getLoopDASSName(Loop *loop) {
  auto id = loop->getLoopID();
  LLVMContext &Context = loop->getHeader()->getContext();
  for (unsigned int i = 0; i < id->getNumOperands(); i++)
    if (auto node = dyn_cast<MDNode>(id->getOperand(i))) {
      Metadata *arg = node->getOperand(0);
      if (arg == MDString::get(Context, "llvm.loop.name")) {
        Metadata *name = node->getOperand(1);
        MDString *nameAsString = dyn_cast<MDString>(name);
        return nameAsString->getString();
      }
    }
  return nullptr;
}

static int getLoopInterchangeDepth(Loop *loop) {
  auto id = loop->getLoopID();
  LLVMContext &Context = loop->getHeader()->getContext();
  for (unsigned int i = 0; i < id->getNumOperands(); i++)
    if (auto node = dyn_cast<MDNode>(id->getOperand(i))) {
      Metadata *arg = node->getOperand(0);
      if (arg == MDString::get(Context, "dass.loop.interchange.check")) {
        Metadata *depth = node->getOperand(1);
        ConstantAsMetadata *depthAsValue = dyn_cast<ConstantAsMetadata>(depth);
        if (depthAsValue) {
          auto depth =
              depthAsValue->getValue()->getUniqueInteger().getSExtValue();
          auto depth2print = (depth == -1) ? "auto" : std::to_string(depth);
          llvm::errs() << "Found loop to check for interchanging: "
                       << getLoopDASSName(loop) << ", depth = " << depth2print
                       << "\n";
          return depth;
        }
      }
    }
  return -2;
}

static void transform(Loop *loop, int depth) {
  if (depth < 2)
    return;

  // Update metadata
  auto id = loop->getLoopID();
  bool check = false;
  LLVMContext &Context = loop->getHeader()->getContext();
  SmallVector<Metadata *, 4> Args;
  auto TempNode = MDNode::getTemporary(Context, None);
  Args.push_back(TempNode.get());
  for (unsigned int i = 1; i < id->getNumOperands(); i++)
    if (dyn_cast<MDNode>(id->getOperand(i)) &&
        dyn_cast<MDNode>(id->getOperand(i))->getOperand(0) ==
            MDString::get(Context, "dass.loop.interchange.check")) {
      Metadata *nameVals[] = {MDString::get(Context, "dass.loop.interchange"),
                              ConstantAsMetadata::get(ConstantInt::get(
                                  IntegerType::get(Context, 32), depth))};
      Args.push_back(MDNode::get(Context, nameVals));
      check = true;
    } else
      Args.push_back(id->getOperand(i));
  // Set the first operand to itself.
  MDNode *LoopID = MDNode::get(Context, Args);
  LoopID->replaceOperandWith(0, LoopID);
  loop->setLoopID(LoopID);

  if (!check) {
    llvm::errs() << *loop << "\n";
    llvm_unreachable("Cannot transform the interchange config of the loop.");
  }
}

static bool isVerificationSuccess(std::string fname) {
  std::ifstream boogieLog(fname);
  if (!boogieLog.is_open())
    llvm_unreachable(std::string("Boogie log not found: " + fname).c_str());
  std::string line;
  while (std::getline(boogieLog, line))
    if (line.find("Error: ") != std::string::npos)
      return false;
  return true;
}

static bool verifyLoopInterchangeDepth(std::string loopName,
                                       BoogieCodeGenerator &bcg, int depth) {
  auto fname = "./verify_" + loopName + ".bpl";
  bcg.open(fname);
  bcg.generateBoogieHeader();
  bcg.phiAnalysis();

  // Generating top-level function
  bcg.generateFuncPrototype(true);
  bcg.generateVariableDeclarations();
  bcg.generateFunctionBody(true);
  bcg.generateMainForLoopInterchange(depth);
  bcg.close();

  // Run verification Boogie
  llvm::errs() << "Checking " << loopName
               << " with interchanging depth = " << depth << ": ";
  system(std::string("boogie " + fname + " 2>&1 | tee ./boogie.log").c_str());
  auto isSuccess = isVerificationSuccess("./boogie.log");
  if (isSuccess)
    system(std::string("rm " + fname + " " + "./boogie.log").c_str());
  return isSuccess;
}

static void checkLoopInterchangeDepth(Loop *loop, BoogieCodeGenerator &bcg) {
  auto depth = getLoopInterchangeDepth(loop);
  // Normal loop
  if (depth == -2)
    return;

  auto loopName = getLoopDASSName(loop);
  bcg.analyzeLoop(loopName);

  if (depth != -1) {
    if (verifyLoopInterchangeDepth(loopName, bcg, depth))
      transform(loop, depth);
    else
      llvm_unreachable("Boogie verification failed.\n");
    return;
  }

  // Auto loop
  if (opt_lower > opt_upper) {
    llvm::errs() << opt_lower << ", " << opt_upper << "\n";
    llvm_unreachable(
        "Invalid search domain settings. Please check the lowerbound and "
        "upperbound values.");
  }

  for (int i = opt_lower; i <= opt_upper; i++)
    if (!verifyLoopInterchangeDepth(loopName, bcg, i)) {
      llvm::errs() << "Found interchanging depth for " << loopName << " : "
                   << i - 1 << "\n";
      transform(loop, i - 1);
      break;
    }
  return;
}

static void checkLoopInterchangeDepthRecursive(Loop *loop,
                                               BoogieCodeGenerator &bcg) {
  checkLoopInterchangeDepth(loop, bcg);
  auto subLoops = loop->getSubLoops();
  if (!subLoops.empty())
    for (auto subloop : subLoops)
      checkLoopInterchangeDepthRecursive(subloop, bcg);
}

namespace {
class LoopInterchangeCheckPass : public llvm::ModulePass {

public:
  static char ID;

  LoopInterchangeCheckPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool LoopInterchangeCheckPass::runOnModule(Module &M) {

  for (auto &F : M) {
    if (F.getName() == "main" || F.hasFnAttribute("dass_ss") ||
        containsCallInst(&F) || F.empty())
      continue;

    auto DT = llvm::DominatorTree(F);
    LoopInfo LI(DT);

    // Clone function for analysis
    ValueToValueMapTy VMap;
    Function *clonedFunc = CloneFunction(&F, VMap);
    BoogieCodeGenerator bcg(clonedFunc);
    bcg.sliceFunction();

    if (!LI.empty())
      for (auto loop : LI)
        checkLoopInterchangeDepthRecursive(loop, bcg);
    clonedFunc->eraseFromParent();
  }

  return true;
}

char LoopInterchangeCheckPass::ID = 1;

static RegisterPass<LoopInterchangeCheckPass>
    X("loop-interchange-check", "Search for loop interchanging depth", false,
      false);
