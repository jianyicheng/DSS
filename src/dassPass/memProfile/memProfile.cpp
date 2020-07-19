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

  struct memProfile : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    memProfile() : ModulePass(ID) {}
    bool runOnModule(Module &M) override {
      errs() << "Start generating memory profiling code..." << "\n";

      static IRBuilder<> Builder(M.getContext());
      Constant *c = M.getOrInsertFunction("printf", FunctionType::get(IntegerType::getInt32Ty(M.getContext()), PointerType::get(Type::getInt8Ty(M.getContext()), 0), true /* this is var arg func type*/));
      Function* ptf = cast<Function>(c);
      
      for (auto &F: M){
        std::string fname = F.getName();
        fname = demangle(fname.c_str());
        fname = fname.substr(0, fname.find("("));
        if (fname != "main"){
          bool check = 0;
          for (auto &BB: F){
            for (auto &I: BB){
              if(isa<LoadInst>(I) || isa<StoreInst>(I))
                check = 1;
            }
          }

          if (check){
            std::string fileName = output_dir+fname+"_pn_mem_analysis.rpt";
            std::ifstream rptIn(fileName);
            assert(rptIn.is_open());
            std::vector<std::vector<std::string>> mem_list;
            std::string line;
            while (std::getline(rptIn, line)){
              if (line. find(":") != std::string::npos){
                line = std::regex_replace(line, std::regex("\t"), "");
                line = std::regex_replace(line, std::regex(" "), "");
                mem_list.push_back({line.substr(0, line.find(":")), line.substr(line.find(":")+1, line.find("=")-line.find(":")-1)});
              }
            }
            rptIn.close();
            assert(mem_list.size()>0);
          
            for (auto &BB: F){
              for (auto &I: BB){
                std::string var_name;
                raw_string_ostream string_stream(var_name);
                I.printAsOperand(string_stream, false);
                std::string temp = string_stream.str();
                int search = -1;
                for (auto v: mem_list){
                  if (v[1] == temp){
                    search = std::find(mem_list.begin(), mem_list.end(), v)-mem_list.begin();
                  }
                }
                if (search != -1){  // found matched instruction
                  Builder.SetInsertPoint(&BB);
                  Value *str = Builder.CreateGlobalStringPtr(StringRef(mem_list[search][0]+": %d\n")); // insert constant string
                  Builder.SetInsertPoint(&I);
                  std::vector<Value*> args;
                  Value* idx = I.getOperand(1);
                  Value* idx32 = Builder.CreateTrunc(idx, Builder.getInt32Ty());
                  args.push_back(str);
                  args.push_back(idx32);
                  Value* call_inst = Builder.CreateCall(ptf, args, "tmp");
                }
                  
              }
            }
          }
          
        }
      }
      errs() << "Generated memory profiling code successfully." << "\n";
      return false;
    }
  };
}

char memProfile::ID = 0;
static RegisterPass<memProfile> X("memProfile", "Array Access Profiling Pass");

