#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <cxxabi.h>
#include "llvm/IR/Type.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"

#include <regex>
#include <string>
#include <fstream>

using namespace llvm;

std::string output_dir = "./";

namespace {
  std::string demangle(const char* name)
  {
          int status = -1;
          std::unique_ptr<char, void(*)(void*)> res { abi::__cxa_demangle(name, NULL, NULL, &status), std::free };
          return (status == 0) ? res.get() : std::string(name);
  }

  struct loopFinder : public ModulePass {
    static char ID; 
    loopFinder() : ModulePass(ID) {}
    bool runOnModule(Module &M) override {
      std::string fileName = output_dir+"loopInfo.rpt";
      std::fstream rpt;
      rpt.open (fileName, std::fstream::out);
      for (auto F = M.begin(); F != M.end(); ++F){
        std::vector<BasicBlock *> bbList;
        for (auto &BB: *F){
          bbList.push_back(&BB);
        }
        if (bbList.size() > 1){
        DominatorTree DT(*F);
        LoopInfo LI(DT);
        std::string fname = F->getName();
        fname = demangle(fname.c_str());
        fname = fname.substr(0, fname.find("("));
        rpt << "Function: " << fname << '\n';
        for (auto &l: LI){
          auto bbs = l->getBlocks();
          for (auto &bb: bbs){
            BasicBlock *b = bb;
            int d = std::find(bbList.begin(), bbList.end(), b) - bbList.begin();
            rpt << d+1 << " ";
          }
          rpt << "\n";
        }
        }
      }
      rpt.close();
      errs() << "Loop information extracted successfully.\n";    
      return false;
    }
  };
}

char loopFinder::ID = 0;
static RegisterPass<loopFinder> X("loopFinder", "Array Access Profiling Pass");

