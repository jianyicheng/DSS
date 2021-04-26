//--------------------------------------------------------//
// Pass: SynthesisPass
// Analyze the static functions and generate Vitis HLS synthesis script for each
// function. Generate wrapper in VHDL2008 for each static function.
//--------------------------------------------------------//

#include <cassert>

#include "llvm/IR/Constant.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "ElasticPass/Head.h"
#include "MyCFGPass/MyCFGPass.h"
#include "Nodes.h"

using namespace llvm;
std::ofstream tclOut, vhdlOut, dummyC;

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
static cl::opt<bool> opt_hasIP("has_ip",
                               cl::desc("Whether this function has IPs"),
                               cl::Hidden, cl::init(false), cl::Optional);

enum PORT_TYPE { IN = 0, OUT = 1, MEM = 2 };

static void tclGen(Function &F) {
  auto fname = F.getName().str();

  tclOut << "open_project -reset " << fname << "\n"
         << "set_top " << fname << "\nadd_files {dummy.cpp} \n"
         << "open_solution -reset \"solution1\"\n"
         << "set_part {" << opt_target << "}\n"
         << "create_clock -period 10 -name default\n"
         << "config_bind -effort high\n"
         // TODO: Is ce needed?
         // << "config_interface -clock_enable\n"
         << "set ::LLVM_CUSTOM_CMD {" << opt_DASS
         << "/llvm/build/bin/opt -no-warn " << opt_irDir << "/" << fname
         << ".ll -o $LLVM_CUSTOM_OUTPUT}\n"
         << "csynth_design\n";
  if (opt_hasIP)
    tclOut << "export_design -flow syn -rtl vhdl -format ip_catalog\n";
  tclOut << "exit\n";
}

static void vhdlGen(ArrayRef<std::string> portNames,
                    ArrayRef<PORT_TYPE> portTypes, Function &F) {
  auto fname = F.getName().str();
  vhdlOut << " -- " << fname << "\n";
  vhdlOut << "library IEEE;\nuse IEEE.std_logic_1164.all;\nuse "
             "IEEE.numeric_std.all;\nuse work.customTypes.all;\n\n";
  vhdlOut << "entity call_" << fname
          << " is generic(INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: "
             "integer; DATA_SIZE_OUT: integer);\n"
          << "port(\n\tdataInArray : IN data_array (INPUTS-1 downto "
             "0)(DATA_SIZE_IN-1 downto 0);\n"
          << "\tpValidArray : IN std_logic_vector(INPUTS-1 downto 0);\n"
          << "\treadyArray : OUT std_logic_vector(INPUTS-1 downto 0);\n"
          << "\tdataOutArray : OUT data_array (OUTPUTS-1 downto "
             "0)(DATA_SIZE_OUT-1 downto 0);\n"
          << "\tnReadyArray: IN std_logic_vector(OUTPUTS-1 downto 0);\n"
          << "\tvalidArray: OUT std_logic_vector(OUTPUTS-1 downto 0);\n"
          << "\tclk, rst: IN std_logic\n);\n"
          << "end entity;\n\narchitecture arch of call_" << fname << " is\n\n";

  vhdlOut << "component " << fname << " is\nport (\n"
          << "\tap_clk : IN STD_LOGIC;\n"
          << "\tap_rst : IN STD_LOGIC;\n"
          << "\tap_start : IN STD_LOGIC;\n"
          << "\tap_done : OUT STD_LOGIC;\n"
          << "\tap_idle : OUT STD_LOGIC;\n"
          << "\tap_ready : OUT STD_LOGIC;\n";

  auto numArg = portNames.size();
  for (unsigned int i = 0; i < numArg; i++) {
    auto name = portNames[i];
    std::string buff;
    switch (portTypes[i]) {
    case IN:
      buff = "\t" + name +
             "_dout : IN STD_LOGIC_VECTOR (DATA_SIZE_IN-1 downto 0);\n" + "\t" +
             name + "_empty_n : IN STD_LOGIC;\n" + "\t" + name +
             "_read : OUT STD_LOGIC;";
      break;
    case OUT:
      buff = "\t" + name +
             "_din : OUT STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);\n" +
             "\t" + name + "_write : OUT STD_LOGIC;\n" + "\t" + name +
             "_full_n : IN STD_LOGIC;";
      break;
    default:
      assert(0 && "Unsupported data interface for wrapper generation");
    }
    if (i == numArg - 1)
      buff.pop_back();
    vhdlOut << buff << "\n";
  }
  vhdlOut << ");\nend component;\n\n";

  vhdlOut << "signal ap_done : STD_LOGIC;\n"
          << "signal ap_idle : STD_LOGIC;\n"
          << "signal ap_ready : STD_LOGIC;\n";

  vhdlOut << "\nbegin\n\n";

  vhdlOut << "\tssfunc: " << fname << "\n\tport map(\n"
          << "\t\tap_clk => clk,\n"
          << "\t\tap_rst => rst,\n"
          << "\t\tap_start => '1',\n"
          << "\t\tap_done => ap_done,\n"
          << "\t\tap_idle => ap_idle,\n"
          << "\t\tap_ready => ap_ready,\n";

  auto in = 0, out = 0;
  for (unsigned int i = 0; i < numArg; i++) {
    auto name = portNames[i];
    std::string buff;
    switch (portTypes[i]) {
    case IN:
      buff = "\t\t" + name + "_dout => dataInArray(" + std::to_string(in) +
             "),\n" + "\t\t" + name + "_empty_n => pValidArray(" +
             std::to_string(in) + "),\n" + "\t\t" + name +
             "_read => readyArray(" + std::to_string(in) + "),\n";
      in++;
      break;
    case OUT:
      buff = "\t\t" + name + "_din => dataOutArray(" + std::to_string(out) +
             "),\n" + "\t\t" + name + "_write => validArray(" +
             std::to_string(out) + "),\n" + "\t\t" + name +
             "_full_n => nReadyArray(" + std::to_string(out) + "),\n";
      out++;
      break;
    default:
      assert(0 && "Unsupported data interface for wrapper generation");
    }

    if (i == numArg - 1) {
      buff.pop_back();
      buff.pop_back();
    }
    vhdlOut << buff << "\n";
  }
  vhdlOut << "\t);\n\nend architecture;\n";
  vhdlOut << "--===================== END " << fname << " ==================\n";
}

static void synthesizeSSFunction(Function &F) {
  assert(F.getReturnType()->isVoidTy() &&
         "outputs can only be written as pointer arguments");
  auto ins = 0, outs = 0, mems = 0, i = 0;
  auto argNum = F.arg_size();
  std::vector<std::string> portNames(argNum);
  std::vector<PORT_TYPE> portTypes(argNum);
  for (auto &arg : F.args()) {
    auto type = (&arg)->getType();
    portNames[i] = arg.getName();
    if (type->isArrayTy()) {
      portTypes[i] = MEM;
      mems++;
    } else if (type->isPointerTy()) {
      portTypes[i] = OUT;
      outs++;
    } else {
      ins++;
      portTypes[i] = IN;
    }
    i++;
  }

  vhdlGen(portNames, portTypes, F);
  tclGen(F);
  dummyC << "void " << F.getName().str() << "(){}\n\n";
}

//--------------------------------------------------------//
// Pass declaration
//--------------------------------------------------------//

namespace {
class SynthesisPass : public llvm::ModulePass {

public:
  static char ID;

  SynthesisPass() : llvm::ModulePass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  bool runOnModule(Module &M) override;
};
} // namespace

void SynthesisPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MyCFGPass>();
}

bool SynthesisPass::runOnModule(Module &M) {
  assert(opt_irDir != "" && "Please specify the input LLVM IR file");

  tclOut.open("./ss.tcl", std::fstream::out);
  vhdlOut.open("./vhdl/wrappers.vhd", std::fstream::out);
  dummyC.open("./dummy.cpp", std::fstream::out);

  auto ssCount = 0;

  for (auto &F : M) {
    if (F.getName() == "main")
      continue;

    if (F.hasFnAttribute("dass_ss")) {
      synthesizeSSFunction(F);
      ssCount++;
    } else {
      // Get dot graph of a DS function
      // auto &cdfg = getAnalysis<MyCFGPass>(*F);
      // synthesizeDSFunction();
    }
  }

  tclOut.close();
  vhdlOut.close();
  dummyC.close();

  return true;
}

char SynthesisPass::ID = 1;

static RegisterPass<SynthesisPass>
    Z("synthesize",
      "Generate synthesis scripts and RTL wrappers for static islands", false,
      false);

//--------------------------------------------------------//
// Pass: CollectVHDLPass
// Collect all the vhdl files for ss functions into vhdl files
//--------------------------------------------------------//

//--------------------------------------------------------//
// Pass declaration
//--------------------------------------------------------//

namespace {
class CollectVHDLPass : public llvm::ModulePass {

public:
  static char ID;

  CollectVHDLPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool CollectVHDLPass::runOnModule(Module &M) {

  for (auto &F : M) {
    auto fname = F.getName().str();
    if (fname == "main")
      continue;

    if (F.hasFnAttribute("dass_ss")) {
      std::string cmd;
      if (opt_hasIP) {
        cmd = "cp " + fname + "/solution1/impl/vhdl/*.vhd ./vhdl/";
        system(cmd.c_str());
        cmd = "cp " + fname + "/solution1/impl/ip/hld/vhdl/* ./vhdl/";
        system(cmd.c_str());
      } else {
        cmd = "cp " + fname + "/solution1/syn/vhdl/* ./vhdl/";
        system(cmd.c_str());
      }
    }
  }
  return true;
}

char CollectVHDLPass::ID = 1;

static RegisterPass<CollectVHDLPass>
    Y("collect-ss-rtl", "Collect VHLD files for SS functions", false, false);

/* for clang pass registration */
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static void registerClangPass(const PassManagerBuilder &,
                              legacy::PassManagerBase &PM) {
  PM.add(new SynthesisPass());
  PM.add(new CollectVHDLPass());
}