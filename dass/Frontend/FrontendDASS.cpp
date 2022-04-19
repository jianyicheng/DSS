//--------------------------------------------------------//
// Pass: LoadPragmaPass
// Load pragmas for LLVM IR
//--------------------------------------------------------//

#include <cassert>
#include <fstream>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"

#include <cxxabi.h>
#include <vector>

using namespace llvm;

//--------------------------------------------------------//
// Pass declaration
//--------------------------------------------------------//

static std::string demangleFuncName(const char *name) {
  auto status = -1;

  std::unique_ptr<char, void (*)(void *)> res{
      abi::__cxa_demangle(name, NULL, NULL, &status), std::free};
  auto func = (status == 0) ? res.get() : std::string(name);
  return func.substr(0, func.find("("));
}

struct PragmaInfo {
  llvm::DenseMap<Function *, int> pipelineInfo;
};

static Loop *getLoopByName(Loop *loop, std::string name) {
  auto nameOp =
      MDString::get(loop->getHeader()->getContext(), "llvm.loop.name");
  auto id = loop->getLoopID();
  for (auto i = 0; i < id->getNumOperands(); i++)
    if (auto node = dyn_cast<MDNode>(id->getOperand(i))) {
      Metadata *arg = node->getOperand(0);
      if (arg == nameOp) {
        Metadata *loopName = node->getOperand(1);
        auto loopNameAsString = dyn_cast<MDString>(loopName);
        if (loopNameAsString->getString() == name)
          return loop;
      }
    }

  auto subloops = loop->getSubLoops();
  if (subloops.empty())
    return nullptr;

  for (auto subloop : subloops)
    if (auto targetLoop = getLoopByName(subloop, name))
      return targetLoop;
  return nullptr;
}

static void setLoopInterchange(Loop *loop, int depth, bool force) {
  SmallVector<Metadata *, 4> Args;

  // Reserve operand 0 for loop id self reference.
  LLVMContext &Context = loop->getHeader()->getContext();
  auto TempNode = MDNode::getTemporary(Context, None);
  Args.push_back(TempNode.get());

  // Keep the original loop metadata
  auto id = loop->getLoopID();
  for (unsigned int i = 1; i < id->getNumOperands(); i++)
    Args.push_back(id->getOperand(i));

  // Loop interchange
  if (force) {
    Metadata *nameVals[] = {MDString::get(Context, "dass.loop.interchange"),
                            ConstantAsMetadata::get(ConstantInt::get(
                                IntegerType::get(Context, 32), depth))};
    Args.push_back(MDNode::get(Context, nameVals));
  } else {
    Metadata *nameVals[] = {
        MDString::get(Context, "dass.loop.interchange.check"),
        ConstantAsMetadata::get(
            ConstantInt::get(IntegerType::get(Context, 32), depth))};
    Args.push_back(MDNode::get(Context, nameVals));
  }

  // Set the first operand to itself.
  MDNode *LoopID = MDNode::get(Context, Args);
  LoopID->replaceOperandWith(0, LoopID);
  loop->setLoopID(LoopID);
}

static Loop *getLoop(std::string loopName, Function *F) {
  auto DT = llvm::DominatorTree(*F);
  LoopInfo LI(DT);
  if (!LI.empty())
    for (auto loop : LI) {
      auto targetLoop = getLoopByName(loop, loopName);
      if (targetLoop)
        return targetLoop;
    }
  return nullptr;
}

static Function *getFunctionByDemangledName(std::string funcName, Module *M) {
  for (auto F = M->begin(); F != M->end(); F++)
    if (funcName == demangleFuncName(F->getName().str().c_str()))
      return dyn_cast<Function>(F);
  return nullptr;
}

static void parsePragma(std::string line, PragmaInfo &info, Module &M) {
  // Pipeline function / loops in function
  if (line.find("pipeline") != std::string::npos) {
    auto firstSpace = line.find(" ");
    auto funcName = line.substr(firstSpace + 1, line.find(" ", firstSpace + 1) -
                                                    firstSpace - 1);
    Function *func;
    for (auto &F : M)
      if (funcName == demangleFuncName(F.getName().str().c_str()))
        func = &F;
    if (!func)
      llvm::errs() << "Warning: Cannot find function " << funcName
                   << " for pipelining. Invalid constraint: " << line << "\n";

    auto secondSpace = line.rfind(" ");
    auto ii = std::stoi(
        line.substr(secondSpace + 1, line.find("\n") - secondSpace - 1));
    ii = (ii) ? ii : 1;
    info.pipelineInfo[func] = ii;
  }
  // Loop interchange
  else if (line.find("interchange") != std::string::npos) {
    auto currSpace = line.find(" ");
    auto funcName = line.substr(currSpace + 1,
                                line.find(" ", currSpace + 1) - currSpace - 1);
    auto func = getFunctionByDemangledName(funcName, &M);
    if (!func)
      llvm::errs() << "Warning: Cannot find function " << funcName
                   << " for pipelining. Invalid constraint: " << line << "\n";

    currSpace = line.find(" ", currSpace + 1);
    auto loopName = "DASS_LOOP_" +
                    line.substr(currSpace + 1,
                                line.find(" ", currSpace + 1) - currSpace - 1);

    currSpace = line.find(" ", currSpace + 1);
    auto depth = std::stoi(line.substr(
        currSpace + 1, line.find(" ", currSpace + 1) - currSpace - 1));

    currSpace = line.find(" ", currSpace + 1);
    auto force = line.substr(currSpace + 1,
                             line.find(" ", currSpace + 1) - currSpace - 1);
    assert(force == "FALSE" || force == "TRUE");
    auto forceBool = (force == "TRUE") ? true : false;

    auto DT = llvm::DominatorTree(*func);
    LoopInfo LI(DT);
    Loop *targetLoop;
    if (!LI.empty())
      for (auto loop : LI) {
        targetLoop = getLoopByName(loop, loopName);
        if (targetLoop)
          break;
      }
    if (!targetLoop)
      llvm_unreachable(
          std::string("Cannot find loop " + loopName + " in function " +
                      funcName +
                      ". Please check if your pragma is specified correctly.\n")
              .c_str());
    if (!targetLoop->getParentLoop())
      llvm_unreachable(
          std::string("Cannot apply interchange on a top-level loop: " +
                      loopName + " in function " + funcName +
                      ". Please check if your pragma is specified correctly.\n")
              .c_str());
    else
      setLoopInterchange(targetLoop, depth, forceBool);
  }
  // Parallel loops
  else if (line.find("parallel_loops") != std::string::npos) {
    auto currSpace = line.find(" ");
    auto funcName = line.substr(currSpace + 1,
                                line.find(" ", currSpace + 1) - currSpace - 1);
    auto F = getFunctionByDemangledName(funcName, &M);
    if (!F)
      llvm_unreachable(
          std::string("Cannot find function " + funcName + ". Pragma error!")
              .c_str());

    std::string loops2parallel = "";
    currSpace = line.find(" ", currSpace + 1);
    while (currSpace != -1) {
      auto loopName = "DASS_LOOP_" +
                      line.substr(currSpace + 1, line.find(" ", currSpace + 1) -
                                                     currSpace - 1);
      if (getLoop(loopName, F))
        loops2parallel += loopName + ",";
      else
        llvm_unreachable(
            std::string(
                "Cannot find loop " + loopName + " in function " + funcName +
                ". Please check if your pragma is specified correctly.\n")
                .c_str());
      currSpace = line.find(" ", currSpace + 1);
    }
    loops2parallel.pop_back();
    loops2parallel += ";";
    if (F->hasFnAttribute("dass.parallel.loops")) {
      loops2parallel =
          std::string(
              F->getFnAttribute("dass.parallel.loops").getValueAsString()) +
          loops2parallel;
      F->removeFnAttr("dass.parallel.loops");
    }
    F->addFnAttr("dass.parallel.loops", loops2parallel);
  } else
    llvm::errs() << "Warning: Unknown constraint: " << line << "\n";
}

static void loadPragmas(std::string fileName, PragmaInfo &info, Module &M) {
  std::ifstream pragmas(fileName);
  if (!pragmas.is_open()) {
    llvm::errs() << "Warning: Cannot find the constraint file " << fileName
                 << "\n";
    return;
  }

  std::string line;
  while (std::getline(pragmas, line))
    parsePragma(line, info, M);
}

static void addDASSAttrubutes(Function *F, PragmaInfo &info) {
  if (info.pipelineInfo.count(F) && info.pipelineInfo[F])
    F->addFnAttr("dass_ss", std::to_string(info.pipelineInfo[F]));
}

namespace {
class LoadPragmaPass : public llvm::ModulePass {

public:
  static char ID;

  LoadPragmaPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool LoadPragmaPass::runOnModule(Module &M) {

  PragmaInfo info;
  loadPragmas("pragmas.tcl", info, M);

  for (auto &F : M) {
    addDASSAttrubutes(&F, info);
    // Add "ss" to avoid function name start with _
    if (F.hasFnAttribute("dass_ss"))
      F.setName("ss" + demangleFuncName(F.getName().str().c_str()) + "ss");
  }

  return true;
}

char LoadPragmaPass::ID = 1;

static RegisterPass<LoadPragmaPass> X("load-pragmas", "Load pragmas", false,
                                      false);
