//--------------------------------------------------------//
// Pass: AddTopAttrPass
// Add attributes to the top function so Vitis HLS can accept it as LLVM IR
// input
//--------------------------------------------------------//

#include "llvm/IR/Constant.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <cxxabi.h>

using namespace llvm;

static cl::opt<std::string> opt_top("top", cl::desc("top design"), cl::Hidden,
                                    cl::init("top"), cl::Optional);

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

namespace {
class AddTopAttrPass : public llvm::ModulePass {

public:
  static char ID;

  AddTopAttrPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool AddTopAttrPass::runOnModule(Module &M) {

  for (auto &F : M) {
    auto fname = demangleFuncName(F.getName().str().c_str());
    if (fname == "ss" + opt_top + "ss") {
      F.setName(opt_top);
      F.addFnAttr("fpga.top.func", opt_top);
      F.addFnAttr("fpga.demangled.func", opt_top);
      return true;
    }
  }
  llvm_unreachable(
      std::string("Cannot find function named: " + opt_top).c_str());
  return true;
}

char AddTopAttrPass::ID = 1;

static RegisterPass<AddTopAttrPass>
    X("add-vhls-attr", "Add minimum attributes for Vitis HLS", false, false);

/* for clang pass registration */
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static void registerClangPass(const PassManagerBuilder &,
                              legacy::PassManagerBase &PM) {
  PM.add(new AddTopAttrPass());
}

static RegisterStandardPasses
    RegisterClangPass(PassManagerBuilder::EP_EarlyAsPossible,
                      registerClangPass);
