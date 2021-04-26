//--------------------------------------------------------//
// Pass: DirectSynthesisPass
// Directly synthesize the function using block level interface.
//--------------------------------------------------------//

#include <cassert>
#include <fstream>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "AutopilotParser.h"

using namespace llvm;
using namespace AutopilotParser;

std::error_code ec;
llvm::raw_fd_ostream tclOut(std::string("./ss_direct.tcl"), ec),
    dummyC(std::string("./dummy.cpp"), ec),
    tclOffsetOut(std::string("./vhls/ss_offset.tcl"), ec);

static cl::opt<std::string> opt_irDir("ir_dir", cl::desc("Input LLVM IR file"),
                                      cl::Hidden, cl::init("./vhls"),
                                      cl::Optional);
static cl::opt<std::string> opt_DASS("dass_dir", cl::desc("Directory of DASS"),
                                     cl::Hidden, cl::init("/workspace"),
                                     cl::Optional);
static cl::opt<std::string> opt_target("target",
                                       cl::desc("Synthesis device target"),
                                       cl::Hidden, cl::init("xc7z020clg484-1"),
                                       cl::Optional);

//--------------------------------------------------------//
// Pass declaration
//--------------------------------------------------------//

namespace {
class DirectSynthesisPass : public llvm::ModulePass {

public:
  static char ID;

  DirectSynthesisPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool DirectSynthesisPass::runOnModule(Module &M) {
  assert(opt_irDir != "" && "Please specify the input LLVM IR file");

  for (auto &F : M) {
    auto fname = F.getName().str();
    if (fname == "main" || !F.hasFnAttribute("dass_ss"))
      continue;

    DominatorTree DT(F);
    LoopInfo LI(DT);

    // If no loop in the function, do function pipelining
    if (LI.empty()) {
      auto newModule = CloneModule(M);
      auto ssfunc = newModule->getFunction(fname);
      ssfunc->addFnAttr("fpga.top.func", fname);
      ssfunc->addFnAttr("fpga.demangled.func", fname);
      ssfunc->addFnAttr("fpga.static.pipeline", "-1.-1");
      llvm::raw_fd_ostream outfile("./vhls/" + fname + "_direct.ll", ec);
      newModule->print(outfile, nullptr);
      tclOut << "open_project -reset " << fname << "_direct\n"
             << "set_top " << fname << "\nadd_files {dummy.cpp} \n"
             << "open_solution -reset \"solution1\"\n"
             << "set_part {" << opt_target << "}\n"
             << "create_clock -period 10 -name default\n"
             << "set ::LLVM_CUSTOM_CMD {" << opt_DASS
             << "/llvm/build/bin/opt -no-warn " << opt_irDir << "/" << fname
             << "_direct.ll -o $LLVM_CUSTOM_OUTPUT}\n"
             << "csynth_design\n"
             << "exit\n";
      dummyC << "void " << F.getName().str() << "(){}\n\n";
    } else
      llvm_unreachable("Loops in static function not supported!");
  }

  tclOut.close();
  dummyC.close();

  return true;
}

char DirectSynthesisPass::ID = 1;

static RegisterPass<DirectSynthesisPass>
    Z("pre-synthesise", "Pre-synthesis scripts for static islands", false,
      false);

//--------------------------------------------------------//
// Pass: GetTimeOffsetPass
// Analyze the offsets of the arguments of the top-level function when
// pipelining the function. Extract the time offsets of arguments and results
// for a pipelined circuit.
//--------------------------------------------------------//

static std::string getOp(std::string line) {
  auto start = line.find("\"");
  return " " + line.substr(start + 1, line.find("\"", start + 1) - start - 1) +
         " ";
}

// Get the earliest steady state where the objective is used as the offset.
static int getOffset(const std::string &name,
                     llvm::DenseMap<unsigned, pipelineState *> &pipelineStates,
                     const unsigned states, const bool isRead) {

  for (int st = 1; st <= states; st++) {
    for (auto stmt : pipelineStates[st]->stmts) {
      auto op = getOp(stmt);
      if (op.find(" %" + name + " ") != std::string::npos) {
        if (isRead && op.find(" %" + name + " = ") == std::string::npos)
          return pipelineStates[st]->SV;
        else if (!isRead && op.find(" %" + name + " = ") != std::string::npos)
          return pipelineStates[st]->SV;
      }
    }
  }
  llvm_unreachable(std::string("Cannot find schedule of use " + name).c_str());
  return -1;
}

// Get the objective where the argument is used.
static std::string
getUse(const std::string &name,
       llvm::DenseMap<unsigned, pipelineState *> &pipelineStates,
       const unsigned states, bool &isRead) {
  for (int st = 1; st <= states; st++) {
    for (auto stmt : pipelineStates[st]->stmts) {
      auto op = getOp(stmt);
      if (op.find(" %" + name + " ") != std::string::npos ||
          op.find(" %" + name + ",") != std::string::npos) {
        if (op.find(" read ") != std::string::npos) {
          isRead = true;
          auto start = op.find("%") + 1;
          return op.substr(start, op.find(" =") - start);
        } else if (op.find(" write ") != std::string::npos) {
          isRead = false;
          auto start = op.rfind("%") + 1;
          return op.substr(start, op.rfind(" ") - start);
        }
      }
    }
  }
  llvm_unreachable(
      std::string("Cannot find schedule of argument " + name).c_str());
  return nullptr;
}

//--------------------------------------------------------//
// Pass declaration
//--------------------------------------------------------//

namespace {
class GetTimeOffsetPass : public llvm::ModulePass {

public:
  static char ID;

  GetTimeOffsetPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool GetTimeOffsetPass::runOnModule(Module &M) {
  assert(opt_irDir != "" && "Please specify the input LLVM IR file");

  for (auto &F : M) {
    auto fname = F.getName().str();
    if (fname == "main" || !F.hasFnAttribute("dass_ss"))
      continue;

    DominatorTree DT(F);
    LoopInfo LI(DT);

    if (LI.empty()) {
      std::ifstream schedRpt(fname + "_direct/solution1/.autopilot/db/" +
                             fname + ".verbose.sched.rpt");
      if (!schedRpt.is_open())
        llvm_unreachable(std::string("Pre-schedule report of function " +
                                     fname + " not found.")
                             .c_str());
      AutopilotParser::AutopilotParser ap(schedRpt);
      auto pipelineStates = ap.getPipelineStates();
      auto states = ap.getTotalStates();

      tclOffsetOut << "Function: " << fname << "\n";
      for (auto &arg : F.args()) {
        if (arg.getType()->isArrayTy())
          continue;
        auto argName = arg.getName();
        bool isRead;
        auto useName = getUse(argName, pipelineStates, states, isRead);
        auto offset = getOffset(useName, pipelineStates, states, isRead);
        tclOffsetOut << argName << ", " << isRead << ", " << offset << ",\n";
      }
      tclOffsetOut << "---\n";
      schedRpt.close();
    } else
      llvm_unreachable("Loops in static function not supported!");
  }
  tclOffsetOut.close();

  return true;
}

char GetTimeOffsetPass::ID = 1;

static RegisterPass<GetTimeOffsetPass>
    Y("get-time-offset", "Extract the time offsets of IOs of a pipeline.",
      false, false);

/* for clang pass registration */
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static void registerClangPass(const PassManagerBuilder &,
                              legacy::PassManagerBase &PM) {
  PM.add(new DirectSynthesisPass());
  PM.add(new GetTimeOffsetPass());
}