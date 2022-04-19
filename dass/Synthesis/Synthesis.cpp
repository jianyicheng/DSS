//--------------------------------------------------------//
// Pass: SSWrapperPass
// Analyze the static functions and generate wrapper in VHDL for each
// function.
//
// Pass: CollectRTLPass
// Collect all the RTL files for ss functions into vhdl files
//
// Pass: StaticIslandInsertionPass
// Rewrite the output VHDL file from Dynamatic to construct memory interface
// Analyze the memory architecture between static and dynamic circuits and
// constructs memory arbitration logic for the shared array
//--------------------------------------------------------//

#include <algorithm>
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

#include "AutopilotParser.h"
#include "Synthesis.h"
#include "VHDLPortParser.h"

#define XSIM

using namespace llvm;
using namespace AutopilotParser;

std::ofstream rtlOut;

cl::opt<std::string> opt_irDir("ir_dir", cl::desc("Input LLVM IR file"),
                               cl::Hidden, cl::init("./vhls"), cl::Optional);
cl::opt<bool> opt_hasIP("has_ip", cl::desc("Whether this function has IPs"),
                        cl::Hidden, cl::init(false), cl::Optional);
cl::opt<std::string> opt_target("target", cl::desc("Synthesis device target"),
                                cl::Hidden, cl::init("xc7z020clg484-1"),
                                cl::Optional);
// xcvu125-flva2104-1-i also supported
cl::opt<std::string> opt_rtl("rtl", cl::desc("RTL Language"), cl::Hidden,
                             cl::init("vhdl"), cl::Optional);
cl::opt<std::string> opt_top("top", cl::desc("top design"), cl::Hidden,
                             cl::init("top"), cl::Optional);
cl::opt<std::string> opt_DASS("dass_dir", cl::desc("Directory of DASS"),
                              cl::Hidden, cl::init("/workspace"), cl::Optional);
cl::opt<bool>
    opt_offset("has_offset",
               cl::desc("Added offset constraints to the SS functions"),
               cl::Hidden, cl::init(true), cl::Optional);

//--------------------------------------------------------//
// Pass declaration: SSWrapperPass
//--------------------------------------------------------//

static void funcportInfoGen(Function *F,
                            llvm::DenseMap<int, PortInfo *> &portInfo,
                            VHDLPortInfo &vPortInfo) {
  auto &memoryInfo = vPortInfo.memInfo;
  auto fname = F->getName().str();
  rtlOut << "\tssfunc: " << fname << "\n\tport map(\n"
         << "\t\tap_clk => clk,\n"
         << "\t\tap_rst => rst,\n"
         << "\t\tap_start => start_ss,\n"
         << "\t\tap_done => ap_done,\n"
         << "\t\tap_idle => ap_idle,\n"
         << "\t\tap_ready => ap_ready,\n"
         << "\t\tap_ce => ap_ce,\n";

  for (auto i = 0; i < portInfo.size(); i++) {
    std::string buff;
    auto name = portInfo[i]->getName();
    auto portIdx = std::to_string(portInfo[i]->getPortIndex());
    if (vPortInfo.isHandshake(portInfo[i]->getName())) {
      switch (portInfo[i]->getType()) {
      case INPUT:
#ifdef XSIM
        buff = "\t\t" + name + "_dout => dataInArray(DATA_SIZE_IN*" + portIdx +
               "+DATA_SIZE_IN-1 downto " + portIdx + "*DATA_SIZE_IN),\n" +
               "\t\t" + name + "_empty_n => start_ss,\n" + "\t\t" + name +
               "_read => ssReady(" + portIdx + "),\n";
#else
        buff = "\t\t" + name + "_dout => dataInArray(" + portIdx + "),\n" +
               "\t\t" + name + "_empty_n => start_ss,\n" + "\t\t" + name +
               "_read => ssReady(" + portIdx + "),\n";
#endif
        break;
      case OUTPUT:
        buff = "\t\t" + name + "_din => dataOutArray(DATA_SIZE_IN*" + portIdx +
               "+DATA_SIZE_IN-1 downto " + portIdx + "*DATA_SIZE_IN),\n" +
               "\t\t" + name + "_write => validArray(" + portIdx + "),\n" +
               "\t\t" + name + "_full_n => '1',\n";
        break;
      case BRAM:
        if (memoryInfo.count(name)) {
          auto mInfo = memoryInfo[name];
          auto addressWidth = mInfo->addressWidth;
          auto dataWidth = mInfo->dataWidth;
          buff = "";
          if (mInfo->we0) {
            if (mInfo->address0)
              buff += "\t\t" + name + "_address0 => " + name + "_address_0,\n";
            if (mInfo->ce0)
              buff += "\t\t" + name + "_ce0 => " + name + "_ce_dummy,\n";
            if (mInfo->we0)
              buff += "\t\t" + name + "_we0 => " + name + "_we_ce0,\n";
            if (mInfo->dout0)
              buff += "\t\t" + name + "_d0 => " + name + "_dout0,\n";
            // if (mInfo->din0)
            // buff += "\t\t" + name + "_q0 => " + name + "_din0,\n";
            if (mInfo->address1)
              buff += "\t\t" + name + "_address1 => " + name + "_address_1,\n";
            if (mInfo->ce1)
              buff += "\t\t" + name + "_ce1 => " + name + "_ce1,\n";
            // if (mInfo->we1)
            // buff += "\t\t" + name + "_we1 => " + name + "_we1,\n";
            // if (mInfo->dout1)
            // buff += "\t\t" + name + "_d1 => " + name + "_dout1,\n";
            if (mInfo->din1)
              buff += "\t\t" + name + "_q1 => " + name + "_din1,\n";
          } else {
            if (mInfo->address0)
              buff += "\t\t" + name + "_address0 => " + name + "_address_1,\n";
            if (mInfo->ce0)
              buff += "\t\t" + name + "_ce0 => " + name + "_ce1,\n";
            // if (mInfo->we0)
            // buff += "\t\t" + name + "_we0 => " + name + "_we1,\n";
            // if (mInfo->dout0)
            // buff += "\t\t" + name + "_d0 => " + name + "_dout1,\n";
            if (mInfo->din0)
              buff += "\t\t" + name + "_q0 => " + name + "_din1,\n";
            if (mInfo->address1)
              buff += "\t\t" + name + "_address1 => " + name + "_address_0,\n";
            if (mInfo->ce1)
              buff += "\t\t" + name + "_ce1 => " + name + "_ce_dummy,\n";
            if (mInfo->we1)
              buff += "\t\t" + name + "_we1 => " + name + "_we_ce0,\n";
            if (mInfo->dout1)
              buff += "\t\t" + name + "_d1 => " + name + "_dout0,\n";
            // if (mInfo->din1)
            // buff += "\t\t" + name + "_q1 => " + name + "_din0,\n";
          }

        } else
          llvm_unreachable(std::string("Cannot find memory port " + name +
                                       " for function " + fname)
                               .c_str());
        break;
      default:
        llvm_unreachable(
            std::string(
                F->getName().str() +
                " : Unsupported data interface for wrapper generation: " + name)
                .c_str());
      }
    } else {
      switch (portInfo[i]->getType()) {
      case INPUT:
#ifdef XSIM
        buff = "\t\t" + name + " => dataInArray(DATA_SIZE_IN*" + portIdx +
               "+DATA_SIZE_IN-1 downto " + portIdx + "*DATA_SIZE_IN),\n";
#else
        buff = "\t\t" + name + " => dataInArray(" + portIdx + "),\n";
#endif
        break;
      case OUTPUT:
        buff = "\t\t" + name + " => dataOutArray(DATA_SIZE_IN*" + portIdx +
               "+DATA_SIZE_IN-1 downto " + portIdx + "*DATA_SIZE_IN),\n";
        break;
      default:
        llvm_unreachable(
            std::string(
                F->getName().str() +
                " : Unsupported data interface for wrapper generation: " +
                name + ", depth = " +
                std::to_string(portInfo[i]->getFIFODepth()) + ", isHS = " +
                std::to_string(vPortInfo.isHandshake(portInfo[i]->getName())))
                .c_str());
      }
    }
    if (i == portInfo.size() - 1) {
      buff.pop_back();
      buff.pop_back();
    }
    rtlOut << buff << "\n";
  }
  rtlOut << "\t);\n";
}

static void funcComponentGen(Function *F, VHDLPortInfo &vPortInfo,
                             bool hasDummyIn) {
  auto fname = F->getName().str();
  rtlOut << "component " << fname << " is\nport (\n";

  for (auto &mem : vPortInfo.memInfo) {
    auto name = mem.first;
    auto mInfo = mem.second;
    auto addressWidth = mInfo->addressWidth;
    auto dataWidth = mInfo->dataWidth;
    std::string buff;
    if (mInfo->address0)
      buff += "\t" + name + "_address0 : OUT STD_LOGIC_VECTOR (" +
              std::to_string(addressWidth - 1) + " downto 0);\n";
    if (mInfo->ce0)
      buff += "\t" + name + "_ce0 : OUT STD_LOGIC;\n";
    if (mInfo->we0)
      buff += "\t" + name + "_we0 : OUT STD_LOGIC;\n";
    if (mInfo->dout0)
      buff += "\t" + name + "_d0 : OUT STD_LOGIC_VECTOR (" +
              std::to_string(dataWidth - 1) + " downto 0);\n";
    if (mInfo->din0)
      buff += "\t" + name + "_q0 : IN STD_LOGIC_VECTOR (" +
              std::to_string(dataWidth - 1) + " downto 0);\n";
    if (mInfo->address1)
      buff += "\t" + name + "_address1 : OUT STD_LOGIC_VECTOR (" +
              std::to_string(addressWidth - 1) + " downto 0);\n";
    if (mInfo->ce1)
      buff += "\t" + name + "_ce1 : OUT STD_LOGIC;\n";
    if (mInfo->we1)
      buff += "\t" + name + "_we1 : OUT STD_LOGIC;\n";
    if (mInfo->dout1)
      buff += "\t" + name + "_d1 : OUT STD_LOGIC_VECTOR (" +
              std::to_string(dataWidth - 1) + " downto 0);\n";
    if (mInfo->din1)
      buff += "\t" + name + "_q1 : IN STD_LOGIC_VECTOR (" +
              std::to_string(dataWidth - 1) + " downto 0);\n";
    rtlOut << buff << "\n";
  }

  for (auto &scalar : vPortInfo.isScalarHandshake) {
    auto name = scalar.first;
    std::string buff;
    if (scalar.second.first) { // input
      if (scalar.second.second)
        buff = "\t" + name +
               "_dout : IN STD_LOGIC_VECTOR (DATA_SIZE_IN-1 downto 0);\n" +
               "\t" + name + "_empty_n : IN STD_LOGIC;\n" + "\t" + name +
               "_read : OUT STD_LOGIC;";
      else
        buff =
            "\t" + name + " : IN STD_LOGIC_VECTOR (DATA_SIZE_IN-1 downto 0);";

    } else {
      if (scalar.second.second) // output
        buff = "\t" + name +
               "_din : OUT STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);\n" +
               "\t" + name + "_write : OUT STD_LOGIC;\n" + "\t" + name +
               "_full_n : IN STD_LOGIC;";
      else
        buff =
            "\t" + name + " : OUT STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);";
    }
    rtlOut << buff << "\n";
  }

  if (hasDummyIn)
    rtlOut
        << "\tcontrol_dout : IN STD_LOGIC_VECTOR (DATA_SIZE_IN-1 downto 0);\n"
        << "\tcontrol_empty_n : IN STD_LOGIC;\n"
        << "\tcontrol_read : OUT STD_LOGIC;";

  rtlOut << "\tap_clk : IN STD_LOGIC;\n"
         << "\tap_rst : IN STD_LOGIC;\n"
         << "\tap_start : IN STD_LOGIC;\n"
         << "\tap_ce : IN STD_LOGIC;\n"
         << "\tap_done : OUT STD_LOGIC;\n"
         << "\tap_idle : OUT STD_LOGIC;\n"
         << "\tap_ready : OUT STD_LOGIC\n"
         << ");\nend component;\n";
}

static int getShiftRegDepth(Function *F, AutopilotParser::AutopilotParser &ap) {
  auto &portInfo = ap.getPortInfo();
  auto latency = ap.getLatency();
  auto maxDepth = 0;
  for (auto i = 0; i < portInfo.size(); i++) {
    switch (portInfo[i]->getType()) {
    case INPUT:
      maxDepth = std::max(maxDepth, portInfo[i]->getFIFODepth());
      break;
    case OUTPUT:
      maxDepth = std::max(maxDepth, (portInfo[i]->getFIFODepth())
                                        ? latency - portInfo[i]->getFIFODepth()
                                        : 0);
      break;
    default:;
    }
  }
  return maxDepth;
}

// Extract three conditions: 1. when the shift_reg loads a set bit, i.e. a valid
// set of inputs is processed; 2. when the component needs to be stalled waiting
// for the unbuffered input; 3. readyArray condition, keep set unless the input
// is ready but the component is not ready. e.g.:
//   process(clk, rst)
//   begin
// 	 if rst = '1' then
//     shift_reg <= (others => '0');
// 	 elsif rising_edge(clk) and ap_ce = '1' then
// 		 shift_reg(0) <= '1' and pValidArray(0) and readyArray(0);
// 		 shift_reg(3 downto 1) <= shift_reg(2 downto 0);
// 	 end if;
// end process;
// ap_ce <= ce and not (shift_reg(3) and not pValidArray(1));
// readyArray(1) <= not pValidArray(1) or shift_reg(3);

static void shiftRegGen(Function *F, llvm::DenseMap<int, PortInfo *> &portInfo,
                        std::string &ceExpr, int depth,
                        VHDLPortInfo &vPortInfo) {
  std::string cond0 = "shift_reg(0) <= '1' ", cond2 = "";
  for (auto i = 0; i < portInfo.size(); i++) {
    std::string buff;
    auto name = portInfo[i]->getName();
    auto portIdx = std::to_string(portInfo[i]->getPortIndex());
    switch (portInfo[i]->getType()) {
    case INPUT:
      if (portInfo[i]->getFIFODepth() ||
          !vPortInfo.isHandshake(portInfo[i]->getName())) {
        ceExpr += " and not (shift_reg(" +
                  std::to_string(portInfo[i]->getFIFODepth() - 1) +
                  ") and not pValidArray(" + portIdx + "))";
        cond2 += "\treadyArray(" + portIdx + ") <= not pValidArray(" + portIdx +
                 ") or shift_reg(" +
                 std::to_string(portInfo[i]->getFIFODepth() - 1) + ");\n";
      } else
        cond0 +=
            "and pValidArray(" + portIdx + ") and readyArray(" + portIdx + ") ";
      break;
    case OUTPUT:
      if (portInfo[i]->getFIFODepth()) {
        llvm::errs() << "TODO: output implementation of shift registers "
                     << portInfo[i]->getFIFODepth() << "\n";
        // llvm_unreachable("TODO: output implementation of shift registers");
      }
      break;
    default:;
    }
  }

  rtlOut << "\tprocess(clk)\n"
         << "\tbegin\n"
         << "\t\tif rst = '1' then\n"
         << "\t\t\tshift_reg <= (others => '0');\n"
         << "\t\telsif rising_edge(clk) and ap_ce = '1' then\n"
         << "\t\t\t" << cond0 << ";\n";
  if (depth > 0)
    rtlOut << "\t\t\tshift_reg(" << depth << " downto 1) <= shift_reg("
           << depth - 1 << " downto 0);\n";
  rtlOut << "\t\tend if;\n"
         << "\tend process;\n"
         << cond2;
}

static void outputBufferSignalGen(Function *F,
                                  llvm::DenseMap<int, PortInfo *> &portInfo) {

  auto argSize = portInfo.size();
  for (auto i = 0; i < argSize; i++) {
    auto name = portInfo[i]->getName();
    if (portInfo[i]->getType() == OUTPUT) {
      rtlOut << "signal " << name << "_dataInArray_0 : "
             << "std_logic_vector(DATA_SIZE_OUT-1 downto 0);\n"
             << "signal " << name << "_pValidArray_0 : std_logic;\n"
             << "signal " << name << "_readyArray_0 : std_logic;\n";
    }
  }
}

// static void outputBufferGen(Function *F,
//                             llvm::DenseMap<int, PortInfo *> &portInfo) {
//   auto out = 0;
//   auto argSize = portInfo.size();
//   for (auto i = 0; i < argSize; i++) {
//     auto name = portInfo[i]->getName();
//     auto portIdx = portInfo[i]->getPortIndex();
//     if (portInfo[i]->getType() == OUTPUT)
//       rtlOut << "\n\tBuffer_" << name
//              << ": entity work.elasticBuffer(arch) generic map "
//                 "(1,1,DATA_SIZE_OUT,DATA_SIZE_OUT)\n"
//              << "\tport map(\n"
//              << "\t\tclk => clk,\n"
//              << "\t\trst => rst,\n"
//              << "\t\tdataInArray => " << name << "_dataInArray_0,\n"
//              << "\t\tpValidArray(0) => " << name << "_pValidArray_0,\n"
//              << "\t\treadyArray(0) => " << name << "_readyArray_0,\n"
//              << "\t\tnReadyArray(0) => nReadyArray(" << portIdx << "),\n"
//              << "\t\tvalidArray(0) => validArray(" << portIdx << "),\n"
//              << "\t\tdataOutArray => dataOutArray(DATA_SIZE_OUT*" << portIdx
//              << "+DATA_SIZE_OUT-1 downto DATA_SIZE_OUT*" << portIdx <<
//              "));\n";
//   }
// }

static void memoryGen(std::map<std::string, MemoryInfo *> &memoryInfo) {
  for (auto const &mi : memoryInfo) {
    auto mInfo = mi.second;
    auto name = mi.first;

    rtlOut << "\t" << name << "_address0 <= std_logic_vector(resize(unsigned("
           << name << "_address_0)," << name << "_address0'length));\n";
    rtlOut << "\t" << name << "_address1 <= std_logic_vector(resize(unsigned("
           << name << "_address_1)," << name << "_address1'length));\n";
    // if (!mInfo->ce0)
    // rtlOut << "\t" << name << "_ce0 <= '0';\n";
    if (!mInfo->we0 && !mInfo->we1)
      rtlOut << "\t" << name << "_we_ce0 <= '0';\n";
    // if (!mInfo->ce1)
    // rtlOut << "\t" << name << "_ce1 <= '0';\n";
    // if (!mInfo->we1)
    // rtlOut << "\t" << name << "_we1 <= '0';\n";
  }
}

static void vhdlGen(std::string &offsetInfo, ENode *callNode, Function *F,
                    VHDLPortInfo &vPortInfo, std::vector<ENode *> *enode_dag,
                    bool needSync) {
  auto &memoryInfo = vPortInfo.memInfo;
  auto fname = F->getName().str();
  rtlOut << " -- " << fname << "\n";
  rtlOut << "library IEEE;\nuse IEEE.std_logic_1164.all;\nuse "
            "IEEE.numeric_std.all;\nuse work.customTypes.all;\n\n";

  // SS component instantiation
  rtlOut << "entity call_" << fname
         << " is generic(INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: "
            "integer; DATA_SIZE_OUT: integer);\n"
         << "port(\n\tdataInArray : IN std_logic_vector(INPUTS*DATA_SIZE_IN-1 "
            "downto 0);\n"
         << "\tpValidArray : IN std_logic_vector(INPUTS-1 downto 0);\n"
         << "\treadyArray : INOUT std_logic_vector(INPUTS-1 downto 0);\n";

  auto file = opt_irDir + "/" + fname + "/solution1/.autopilot/db/" + fname +
              ".verbose.sched.rpt";
  std::ifstream schedRpt(file);
  if (!schedRpt.is_open())
    llvm_unreachable(std::string("Pre-schedule report of function " + fname +
                                 " not found: " + file)
                         .c_str());
  auto ap = AutopilotParser::AutopilotParser(schedRpt, F);
  ap.anlayzePortInfo(opt_offset);
  ap.analyzePortIndices(callNode, enode_dag);
  ap.adjustOffsets(vPortInfo);
  if (!ap.containLoops())
    offsetInfo += ap.exportOffsets();

  auto &portInfo = ap.getPortInfo();
  auto argSize = portInfo.size();
  if (ap.getNumOutputs() > 0 || callNode->JustCntrlSuccs->size() > 0)
    rtlOut << "\tdataOutArray : INOUT std_logic_vector(OUTPUTS*DATA_SIZE_OUT-1 "
              "downto 0);\n"
           << "\tnReadyArray: IN std_logic_vector(OUTPUTS-1 downto 0);\n"
           << "\tvalidArray: INOUT std_logic_vector(OUTPUTS-1 downto 0);\n";

  // Add memory interface
  for (auto const &mi : memoryInfo) {
    auto name = mi.first;
    rtlOut << "\t" << name
           << "_address0 : out std_logic_vector (DATA_SIZE_IN-1 downto 0);\n\t"
           //  << name << "_ce0 : out std_logic;\n\t"
           << name << "_we_ce0 : out std_logic;\n\t" << name
           << "_dout0 : out std_logic_vector (DATA_SIZE_IN-1 downto 0);\n\t"
           //  << name
           //  << "_din0 : in std_logic_vector (DATA_SIZE_IN-1 downto 0);\n\t"
           << name
           << "_address1 : out std_logic_vector (DATA_SIZE_IN-1 downto 0);\n\t"
           << name
           << "_ce1 : out std_logic;\n\t"
           //  << name << "_we1 : out std_logic;\n\t"
           //  << name
           //  << "_dout1 : out std_logic_vector (DATA_SIZE_IN-1 downto
           //  0);\n\t"
           << name
           << "_din1 : in std_logic_vector (DATA_SIZE_IN-1 downto 0);\n\t"
           << name << "_empty_valid : in std_logic;\n";
  }
  if (needSync)
    rtlOut << "\tsync_in_ready, sync_out_valid : inout "
              "std_logic;\n\tsync_in_valid, sync_out_ready : in std_logic;\n";
  rtlOut << "\tclk, rst, ce: IN std_logic;\n\tstart, done: OUT std_logic\n);\n"
         << "end entity;\n\narchitecture arch of call_" << fname << " is\n\n";

  // Components
  funcComponentGen(F, vPortInfo, ap.hasDummyInput());

  rtlOut << "\nsignal ap_done : STD_LOGIC;\n"
         << "signal ap_idle : STD_LOGIC;\n"
         << "signal ap_ready : STD_LOGIC;\n"
         << "signal ap_ce : STD_LOGIC;\n"
         << "signal ssReady : std_logic_vector(INPUTS-1 downto 0);\n"
         << "signal start_ss : std_logic;\n"
         << "signal start_internal : std_logic;\n"
         << "signal memory_empty_valid : std_logic;\n";
  if (needSync)
    rtlOut << "\tsignal sync_buffer_data: std_logic_vector(0 downto "
              "0);\n"
           << "\tsignal sync_buffer_ready :std_logic;\n"
           << "\tsignal "
              "joinReady: std_logic_vector(1 downto 0);\n"
           << "\tsignal joinpValid: "
              "std_logic_vector(1 downto 0);\n";

  auto shiftRegDepth = getShiftRegDepth(F, ap);
  rtlOut << "signal shift_reg : std_logic_vector(" << shiftRegDepth
         << " downto 0);\n";
  // Memory addresses for bit match
  for (auto const &mi : memoryInfo) {
    auto mInfo = mi.second;
    auto name = mi.first;
    rtlOut << "signal " << name << "_address_0 : std_logic_vector ("
           << mInfo->addressWidth - 1 << " downto 0);\n"
           << "signal " << name << "_address_1 : std_logic_vector ("
           << mInfo->addressWidth - 1 << " downto 0);\n"
           << "signal " << name << "_ce_dummy : std_logic;\n";
  }

  // Add output buffers
  // outputBufferSignalGen(F, portInfo);

  rtlOut << "\nbegin\n\n";

  if (needSync)
    rtlOut << "\tBuffer_sync: entity work.elasticBuffer(arch) generic map "
              "(1,1,1,1)\n"
           << "\tport map(\n"
           << "\t\tclk => clk,\n\t\trst => rst,\n"
           << "\t\tdataInArray(0) => ap_done,\n"
           << "\t\tpValidArray(0) => ap_done, \n"
           << "\t\treadyArray(0) => sync_buffer_ready,\n"
           << "\t\tnReadyArray(0) => joinReady(0), \n"
           << "\t\tvalidArray(0) => joinpValid(0),\n"
           << "\t\tdataOutArray => sync_buffer_data);\n"
           << "\tsync_in_ready <= joinReady(1);\n"
           << "\tjoinpValid(1) <= sync_in_valid;\n"
           << "\tj : entity work.join(arch) generic map(2)\n"
           << "\t\tport map(joinpValid, sync_out_ready, sync_out_valid, "
              "joinReady);\n";

  // Memory address bit match
  memoryGen(memoryInfo);

  // Memory empty valid signal
  rtlOut << "\tmemory_empty_valid <= '1'";
  if (needSync)
    for (auto const &mi : memoryInfo)
      rtlOut << " and " << mi.first << "_empty_valid";
  rtlOut << ";\n";

  // readyArray
  bool hasNoOffsetScalarIn = false;
  for (auto i = 0; i < argSize; i++) {
    auto portIdx = std::to_string(portInfo[i]->getPortIndex());
    if (portInfo[i]->getType() == INPUT &&
        vPortInfo.isHandshake(portInfo[i]->getName()))
      rtlOut << "\treadyArray(" + portIdx + ") <= not pValidArray(" + portIdx +
                    ") or ssReady(" + portIdx + ") when (pValidArray(" +
                    portIdx + ") and ssReady(" + portIdx +
                    ")) = '0' else start_ss;\n";
    if (portInfo[i]->getType() == INPUT &&
        !vPortInfo.isHandshake(portInfo[i]->getName()))
      hasNoOffsetScalarIn = true;
  }

  // Clock enable signal
  std::string ceExpr = "\tap_ce <= ce";
  if (needSync)
    ceExpr += " and (sync_buffer_ready or not ap_done)";
  // Implement backpressure
  if (ap.getNumOutputs() > 0)
    for (auto i = 0; i < argSize; i++) {
      auto portIdx = std::to_string(portInfo[i]->getPortIndex());
      if (portInfo[i]->getType() == OUTPUT)
        ceExpr += " and (nReadyArray(" + portIdx + ") or not validArray(" +
                  portIdx + "))";
      // ceExpr += " and (" + portInfo[i]->getName() + "_readyArray_0 or not " +
      //           portInfo[i]->getName() + "_pValidArray_0)";
    }
  else if (callNode->JustCntrlSuccs->size() > 0)
    ceExpr += " and (not ap_done or nReadyArray(0))";

  // Implement shift reg for unbuffered IOs
  if (shiftRegDepth > 0 || hasNoOffsetScalarIn)
    shiftRegGen(F, portInfo, ceExpr, shiftRegDepth, vPortInfo);
  rtlOut << ceExpr << ";\n";

  // start & done signal
  rtlOut << "\tdone <= ap_done;\n";
  if (ap.getNumOutputs() == 0 && callNode->JustCntrlSuccs->size() > 0)
    rtlOut << "\tvalidArray(0) <= ap_done;\n";

  rtlOut << "\tstart <= start_internal;\n\tstart_internal "
            "<= ce and memory_empty_valid";
  for (auto i = 0; i < argSize; i++) {
    auto portIdx = std::to_string(portInfo[i]->getPortIndex());
    if (portInfo[i]->getType() == INPUT && portInfo[i]->getFIFODepth() == 0)
      rtlOut << " and pValidArray(" + portIdx + ")";
  }
  rtlOut << ";\n\tprocess(clk, rst)\n\tbegin\n\t\tif rst = '1' "
            "then\n\t\t\tstart_ss <= '0';\n\t\telsif rising_edge(clk) "
            "then\n\t\t\tstart_ss <= start_internal;\n\t\tend if;\n\tend "
            "process;\n";
  rtlOut << "\n";

  // Port map
  funcportInfoGen(F, portInfo, vPortInfo);
  // outputBufferGen(F, portInfo);
  rtlOut << "\nend architecture;\n";
  rtlOut << "--===================== END " << fname << " ==================\n";
}

static bool isSSCall(ENode *enode) {
  if (!enode->Instr)
    return false;
  if (enode->type != Inst_ || !isa<CallInst>(enode->Instr))
    return false;
  auto callInst = dyn_cast<CallInst>(enode->Instr);
  if (!callInst->getCalledFunction()->hasFnAttribute("dass_ss"))
    return false;
  return true;
}

struct SharedMemory {
  Value *val;
  std::string name;
  int ssCount;
  int ports;
  bool inDS;
  std::vector<std::string> funcNames;
};

static std::vector<SharedMemory *> getSharedArrays(Function *F) {
  std::vector<SharedMemory *> sharedArrays;
  auto arg = F->arg_begin();
  for (auto i = 0; i < F->arg_size(); i++) {
    auto argType = arg->getType();
    if (argType->isPointerTy() && !arg->use_empty()) {
      auto useOp = arg->use_begin()->getUser();
      if (isa<CallInst>(useOp) || isa<GetElementPtrInst>(useOp)) {
        int ssCount = 0;
        bool inDS = 0;
        std::vector<std::string> funcNames;
        for (auto u = arg->use_begin(); u != arg->use_end(); u++) {
          if (auto callInst = dyn_cast<CallInst>(u->getUser())) {
            if (callInst->getCalledFunction()->hasFnAttribute("dass_ss")) {
              ssCount++;
              funcNames.push_back(callInst->getCalledFunction()->getName());
              continue;
            }
          } else
            inDS = 1;
        }
        if (ssCount > 0) {
          auto sm = new SharedMemory;
          sm->val = arg;
          sm->name = arg->getName();
          sm->ssCount = ssCount;
          sm->inDS = inDS;
          sm->ports = inDS + ssCount;
          sm->funcNames = funcNames;
          sharedArrays.push_back(sm);
        }
      }
    }
    arg++;
  }
  for (auto sm : sharedArrays)
    llvm::errs() << sm->name << ": SS=" << sm->ssCount << ", DS=" << sm->inDS
                 << ", ports=" << sm->ports << "\n";

  return sharedArrays;
}

// Check a node whether it should be sync with the current basic block to avoid
// memory conflict
static std::string needSyncWith(ENode *enode, std::string funcName,
                                ArrayRef<SharedMemory *> sharedArrays,
                                ENode_vec *enode_dag) {
  // Two call nodes are sequential - one requires the output from the other
  if (enode->CntrlSuccs->size() > 0)
    for (auto succ : *enode->CntrlSuccs) {
      auto node = (succ->type == Fork_ || succ->type == Fork_c)
                      ? succ->CntrlSuccs->at(0)
                      : succ;
      if (node->type == Inst_ && node->Instr && isa<CallInst>(node->Instr)) {
        llvm::errs() << getNodeDotNameNew(node) << "\n";
        return "";
      }
    }

  // Temporary: A call node that has output
  if (enode->CntrlSuccs->size() > 0 || enode->JustCntrlSuccs->size() > 0)
    return "";

  // A call node has a shared array with another call node or DS part
  bool needSync = false;
  std::string branchName = "";
  for (auto sa : sharedArrays) {
    if (std::find(sa->funcNames.begin(), sa->funcNames.end(), funcName) !=
        sa->funcNames.end())
      if (sa->inDS || sa->ssCount > 1) {
        needSync = true;
        break;
      }
  }
  if (!needSync)
    return branchName;

  // get block terminator name
  for (auto node : *enode_dag)
    if (enode->bbId == node->bbId && node->type == Branch_c) {
      branchName = getNodeDotNameNew(node);
      break;
    }

  if (branchName == "")
    for (auto node : *enode_dag)
      if (enode->bbId == node->bbId && node->Instr &&
          isa<llvm::ReturnInst>(node->Instr))
        for (auto n : *enode_dag)
          if (n->type == End_) {
            branchName = getNodeDotNameNew(n);
            goto exit;
          }
exit:
  return branchName;
}

static ENode *getCallNode(Function *F, ENode_vec *enode_dag) {
  for (auto enode : *enode_dag) {
    if (!isSSCall(enode))
      continue;
    auto callInst = dyn_cast<CallInst>(enode->Instr);
    if (callInst->getCalledFunction() == F)
      return enode;
  }
  return nullptr;
}

namespace {
class SSWrapperPass : public llvm::ModulePass {

public:
  static char ID;

  SSWrapperPass() : llvm::ModulePass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  bool runOnModule(Module &M) override;
};
} // namespace

void SSWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MyCFGPass>();
}

bool SSWrapperPass::runOnModule(Module &M) {
  assert(opt_irDir != "" && "Please specify the input LLVM IR file");

  rtlOut.open("./rtl/wrappers.vhd", std::fstream::out);

  // Assume there is only one DS function
  std::vector<ENode *> *enode_dag;
  std::vector<SharedMemory *> sharedArrays;
  for (auto &F : M)
    if (F.getName() != "main" && !F.hasFnAttribute("dass_ss") && !F.empty()) {
      enode_dag = getAnalysis<MyCFGPass>(F).enode_dag;
      sharedArrays = getSharedArrays(&F);
    }

  auto ssCount = 0;
  std::string offsetInfo;
  for (auto &F : M) {
    if (F.getName() == "main" || !F.hasFnAttribute("dass_ss"))
      continue;
    assert(F.getReturnType()->isVoidTy() &&
           "outputs can only be written as pointer arguments");

    // SS Function is not used
    auto callNode = getCallNode(&F, enode_dag);
    if (!callNode)
      continue;

    auto branchName =
        needSyncWith(callNode, F.getName().str(), sharedArrays, enode_dag);
    bool needSync = (branchName != "");
    llvm::errs() << F.getName().str() << " : " << needSync << "\n";

    auto vPortInfo = parsePortInfoVHDL(&F, opt_irDir + "/");
    assert(enode_dag->size() > 0);
    vhdlGen(offsetInfo, callNode, &F, vPortInfo, enode_dag, needSync);
    ssCount++;
  }
  rtlOut.close();

  if (offsetInfo != "") {
    std::error_code ec;
    llvm::raw_fd_ostream outfile(opt_irDir + "/ss_offset.tcl", ec);
    outfile << offsetInfo;
    outfile.close();
  }

  return true;
}

//--------------------------------------------------------//
// Pass declaration: CollectRTLPass
//--------------------------------------------------------//

namespace {
class CollectRTLPass : public llvm::ModulePass {

public:
  static char ID;

  CollectRTLPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool CollectRTLPass::runOnModule(Module &M) {

  if (opt_rtl != "verilog" && opt_rtl != "vhdl")
    llvm_unreachable(
        "Cannot detect the rtl language. Please use verilog or vhdl.");

  for (auto &F : M) {
    auto fname = F.getName().str();
    if (fname == "main" || !F.hasFnAttribute("dass_ss"))
      continue;

    std::string isSS = F.getFnAttribute("dass_ss").getValueAsString();
    std::string cmd;

    auto rtlDir = (opt_hasIP) ? opt_irDir + "/" + fname + "/solution1/syn/" +
                                    opt_rtl + "/"
                              : opt_irDir + "/" + fname +
                                    "/solution1/impl/ip/hdl/" + opt_rtl + "/";

    // The valid rtl code is named as ${TOP}_new.v* instead of ${TOP}.v*
    auto DT = llvm::DominatorTree(F);
    LoopInfo LI(DT);
    if (opt_rtl == "verilog") {
      cmd = "python3 " + opt_DASS + "/dass/scripts/OptimizeSSFunc.py " +
            rtlDir + fname + ".v -t " + fname;
      llvm::errs() << cmd << "\n";
      system(cmd.c_str());
    } else {
      llvm_unreachable("Offset removal for vhdl code is not supported.");
    }

    cmd = "cp " + rtlDir + "*.v* ./rtl/";
    system(cmd.c_str());
    if (opt_hasIP)
      cmd = "cp " + opt_irDir + "/" + fname +
            "/solution1/impl/ip/hdl/ip/*.v* ./rtl/";
    system(cmd.c_str());

    // Remove the old rtl code with shift registers
    if (opt_rtl == "verilog") {
      cmd = "rm ./rtl/" + fname + ".v";
      system(cmd.c_str());
    } else {
      llvm_unreachable("Offset removal for vhdl code is not supported.");
    }
  }
  return true;
}

//--------------------------------------------------------//
// Pass declaration: StaticIslandInsertionPass
//--------------------------------------------------------//

static std::string getEmptyValidName(std::string name,
                                     std::vector<std::string> &vhdlCode) {
  for (auto &s : vhdlCode)
    if (s.find("io_Empty_Valid => MC_" + name) != std::string::npos)
      return s.substr(s.find("=>") + 3, s.find(",") - s.find("=>") - 3);

  for (auto &s : vhdlCode)
    if (s.find("io_Empty_Valid => LSQ_" + name) != std::string::npos)
      return s.substr(s.find("=>") + 3, s.find(",") - s.find("=>") - 3);

  return "\'1\'";
}

static void rewriteMemory(SharedMemory *sm,
                          std::vector<std::string> &vhdlCode) {
  auto name = sm->name;
  auto i = 0;

  while (i < vhdlCode.size() &&
         vhdlCode[i].find("MC_" + name + ": ") == std::string::npos)
    i++;
  // Special case where only a LSQ is needed
  auto hasMC = (i == vhdlCode.size()) ? false : true;

  i = 0;
  while (vhdlCode[i].find("architecture behavioral of " + opt_top + " is") ==
         std::string::npos)
    i++;
  i++;

  // Add new signals
  vhdlCode[i] +=
      "\n\tsignal DA_" + name + "_clk: std_logic;\n\tsignal DA_" + name +
      "_rst: std_logic;\n\tsignal DA_" + name +
      "_valid: std_logic;\n\tsignal DA_" + name +
      "_ready: std_logic;\n\tsignal DA_" + name +
      "_ss_start: std_logic_vector(" + std::to_string(sm->ports - 1) +
      " downto 0);\n\tsignal DA_" + name + "_ss_done: std_logic_vector(" +
      std::to_string(sm->ports - 1) + " downto 0);\n\tsignal DA_" + name +
      "_ss_ce: std_logic_vector(" + std::to_string(sm->ports - 1) +
      " downto 0);\n\tsignal DA_" + name + "_we0_ce0: std_logic;\n";
  for (auto j = 0; j < sm->ports; j++) {
    vhdlCode[i] +=
        "\tsignal DA_" + name + "_storeDataOut_" + std::to_string(j) +
        ": std_logic_vector(31 downto 0);\n\tsignal DA_" + name +
        "_storeAddrOut_" + std::to_string(j) +
        ": std_logic_vector(31 downto 0);\n\tsignal DA_" + name +
        "_storeEnable_" + std::to_string(j) + ": std_logic;\n\tsignal DA_" +
        name + "_loadDataIn_" + std::to_string(j) +
        ": std_logic_vector(31 downto 0);\n\tsignal DA_" + name +
        "_loadAddrOut_" + std::to_string(j) +
        ": std_logic_vector(31 downto 0);\n\tsignal DA_" + name +
        "_loadEnable_" + std::to_string(j) + ": std_logic;\n";
  }

  // Add dassArbiter components
  i = 0;
  while (i < vhdlCode.size() &&
         vhdlCode[i].find("end behavioral; ") == std::string::npos)
    i++;
  i--;
  std::string comp = (hasMC) ? "MC" : "LSQ";
  vhdlCode[i] +=
      "\nDA_" + name + ": entity work.dassArbiter(arch) generic map (32,32," +
      std::to_string(sm->ports) + ")\nport map(\n\tclk => DA_" + name +
      "_clk,\n\trst => DA_" + name + "_rst,\n\tio_storeDataOut => " + name +
      "_dout0,\n\tio_storeAddrOut => " + name +
      "_address0,\n\tio_storeEnable => " + "DA_" + name +
      "_we0_ce0,\n\tio_loadDataIn => " + name + "_din1,\n\tio_loadAddrOut => " +
      name + "_address1,\n\tio_loadEnable => " + name + "_ce1,\n";
  for (auto j = 0; j < sm->ports; j++)
    vhdlCode[i] += "\tstoreDataOut(" + std::to_string(j * 32 + 31) +
                   " downto " + std::to_string(j * 32) + ") => DA_" + name +
                   "_storeDataOut_" + std::to_string(j) + ",\n";
  for (auto j = 0; j < sm->ports; j++)
    vhdlCode[i] += "\tstoreAddrOut(" + std::to_string(j * 32 + 31) +
                   " downto " + std::to_string(j * 32) + ") => DA_" + name +
                   "_storeAddrOut_" + std::to_string(j) + ",\n";
  for (auto j = 0; j < sm->ports; j++)
    vhdlCode[i] += "\tstoreEnable(" + std::to_string(j) + ") => DA_" + name +
                   "_storeEnable_" + std::to_string(j) + ",\n";
  for (auto j = 0; j < sm->ports; j++)
    vhdlCode[i] += "\tloadDataIn(" + std::to_string(j * 32 + 31) + " downto " +
                   std::to_string(j * 32) + ") => DA_" + name + "_loadDataIn_" +
                   std::to_string(j) + ",\n";
  for (auto j = 0; j < sm->ports; j++)
    vhdlCode[i] += "\tloadAddrOut(" + std::to_string(j * 32 + 31) + " downto " +
                   std::to_string(j * 32) + ") => DA_" + name +
                   "_loadAddrOut_" + std::to_string(j) + ",\n";
  for (auto j = 0; j < sm->ports; j++)
    vhdlCode[i] += "\tloadEnable(" + std::to_string(j) + ") => DA_" + name +
                   "_loadEnable_" + std::to_string(j) + ",\n";

  vhdlCode[i] += "\tio_Empty_Valid => DA_" + name + "_valid,\n\tready => DA_" +
                 name + "_ready,\n\tss_start => DA_" + name +
                 "_ss_start,\n\tss_ce => DA_" + name +
                 "_ss_ce,\n\tss_done => DA_" + name + "_ss_done\n);\n";

  // Overwrite ce & we
  auto j = 0;
  while (j < vhdlCode.size() &&
         vhdlCode[j].find(name + "_we0 <= ") == std::string::npos)
    j++;
  if (j == vhdlCode.size())
    vhdlCode[i] += "\t" + name + "_we0 <= DA_" + name + "_we0_ce0;\n";
  else
    vhdlCode[j] = "\t" + name + "_we0 <= DA_" + name + "_we0_ce0;";
  j = 0;
  while (j < vhdlCode.size() &&
         vhdlCode[j].find(name + "_ce0 <= ") == std::string::npos)
    j++;
  if (j == vhdlCode.size())
    vhdlCode[i] += "\t" + name + "_ce0 <= DA_" + name + "_we0_ce0;\n";
  else
    vhdlCode[j] = "\t" + name + "_ce0 <= DA_" + name + "_we0_ce0;";

  // Have ds memory
  if (sm->inDS) {
    // Rewrite memcont interface
    i = 0;
    while (i < vhdlCode.size() &&
           vhdlCode[i].find(comp + "_" + name + ": ") == std::string::npos)
      i++;
    assert(i < vhdlCode.size());

    while (vhdlCode[i].find("io_storeDataOut => ") == std::string::npos)
      i++;
    vhdlCode[i] = "\tio_storeDataOut => DA_" + name + "_storeDataOut_0,";
    vhdlCode[i + 1] = "\tio_storeAddrOut => DA_" + name + "_storeAddrOut_0,";
    vhdlCode[i + 2] = "\tio_storeEnable => DA_" + name + "_storeEnable_0,";
    vhdlCode[i + 3] = "\tio_loadDataIn => DA_" + name + "_loadDataIn_0,";
    vhdlCode[i + 4] = "\tio_loadAddrOut => DA_" + name + "_loadAddrOut_0,";
    vhdlCode[i + 5] = "\tio_loadEnable => DA_" + name + "_loadEnable_0,";
  } else {
    // Rewrite top memory interface
    i = 0;
    while (vhdlCode[i].find("entity " + opt_top + " is") == std::string::npos)
      i++;
    i++;
    vhdlCode[i] +=
        "\n\t" + name + "_address0 : out std_logic_vector (31 downto 0);\n\t" +
        name + "_ce0 : out std_logic;\n\t" + name +
        "_we0 : out std_logic;\n\t" + name +
        "_dout0 : out std_logic_vector (31 downto 0);\n\t" + name +
        "_din0 : in std_logic_vector (31 downto 0);\n\t" + name +
        "_address1 : out std_logic_vector (31 downto 0);\n\t" + name +
        "_ce1 : out std_logic;\n\t" + name + "_we1 : out std_logic;\n\t" +
        name + "_dout1 : out std_logic_vector (31 downto 0);\n\t" + name +
        "_din1 : in std_logic_vector (31 downto 0);";
  }

  // Rewrite memcont interface for ss
  int index = sm->inDS;
  std::map<std::string, std::string> opMap;
  for (auto &call : sm->funcNames) {
    i = 0;
    while (vhdlCode[i].find("call_" + call + "(arch)") == std::string::npos)
      i++;
    opMap[call] = vhdlCode[i].substr(0, vhdlCode[i].find(":"));
    i++;

    auto emptyValidName = getEmptyValidName(name, vhdlCode);
    llvm::errs() << name << " " << emptyValidName << "\n";
    vhdlCode[i] +=
        "\n\t" + name + "_address0 => DA_" + name + "_storeAddrOut_" +
        std::to_string(index) + ",\n\t" + name + "_we_ce0 => DA_" + name +
        "_storeEnable_" + std::to_string(index) + ",\n\t" + name +
        "_dout0 => DA_" + name + "_storeDataOut_" + std::to_string(index) +
        ",\n\t" + name + "_address1 => DA_" + name + "_loadAddrOut_" +
        std::to_string(index) + ",\n\t" + name + "_ce1 => DA_" + name +
        "_loadEnable_" + std::to_string(index) + ",\n\t" + name +
        "_din1 => DA_" + name + "_loadDataIn_" + std::to_string(index) +
        ",\n\t" + name + "_empty_valid => " + emptyValidName + ",\n";
    index++;
  }

  // Add connections
  i = 0;
  while (vhdlCode[i].find("begin") == std::string::npos)
    i++;
  i++;
  index = sm->inDS;
  vhdlCode[i] +=
      "\tDA_" + name + "_clk <= clk;\n\tDA_" + name + "_rst <= rst;\n";
  if (index)
    vhdlCode[i] += "\tDA_" + name + "_ss_start(0) <= '0';\n\tDA_" + name +
                   "_ss_done(0) <= '0';\n";
  for (auto &call : sm->funcNames) {
    vhdlCode[i] += "\tDA_" + name + "_ss_start(" + std::to_string(index) +
                   ") <= " + opMap[call] + "_start;\n\tDA_" + name +
                   "_ss_done(" + std::to_string(index) + ") <= " + opMap[call] +
                   "_done;\n";
    index++;
  }

  // Rewrite end node
}

static bool replace(std::string &str, const std::string &from,
                    const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

static void rewriteCall(std::vector<std::string> &vhdlCode) {
  std::map<std::string, std::string> callNames;
  auto i = 0;
  for (auto &s : vhdlCode) {
    if (s.find("call_") != std::string::npos &&
        s.find(": entity work.call_") != std::string::npos) {
      auto name = s.substr(0, s.find(":"));
      auto j = i;
      std::string ce = "'1'";
      while (vhdlCode[j].find(");") == std::string::npos) {
        std::string line = vhdlCode[j];
        while (line.find("address0") != std::string::npos) {
          auto sig = line.substr(line.find("=> ") + 3,
                                 line.find(",") - line.find("=> ") - 3);
          replace(sig, "storeAddrOut", "ss_ce");
          for (auto k = sig.size() - 1; k >= 0; k--) {
            if (sig[k] == '_') {
              sig[k] = '(';
              break;
            }
          }
          ce += " and " + sig + ")";
          line = line.substr(line.find("\n", line.find("_empty_valid")));
        }
        j++;
      }
      callNames[name] = ce;
      vhdlCode[i + 1] += "\n\tstart => " + name + "_start,\n\tdone => " + name +
                         "_done,\n\tce => " + name + "_ce,\n";
    }
    i++;
  }

  i = 0;
  while (vhdlCode[i].find("architecture behavioral of " + opt_top + " is") ==
         std::string::npos)
    i++;
  i++;
  for (const auto &c : callNames)
    vhdlCode[i] += "\tsignal " + c.first + "_start : std_logic;\n\tsignal " +
                   c.first + "_done : std_logic;\n\tsignal " + c.first +
                   "_ce : std_logic;\n";

  i = 0;
  while (vhdlCode[i].find("begin") == std::string::npos)
    i++;
  i++;
  for (const auto &c : callNames)
    vhdlCode[i] += "\t" + c.first + "_ce <= " + c.second + ";\n";
}

static void rewriteMC(ArrayRef<SharedMemory *> sharedArrays,
                      std::vector<std::string> &vhdlCode) {
  std::vector<std::string> names;
  for (auto i = 0; i < vhdlCode.size(); i++) {
    auto s = vhdlCode[i];
    if (s.find("MC_") != std::string::npos &&
        s.find(": entity work.MemCont(arch) generic map") !=
            std::string::npos) {
      auto name = s.substr(s.find("MC_") + 3, s.find(":") - s.find("MC_") - 3);
      names.push_back(name);
      bool search = false;
      for (auto const &sa : sharedArrays) {
        if (sa->name == name) {
          search = true;
          break;
        }
      }
      if (search)
        vhdlCode[i + 1] += "\n\tce => DA_" + name + "_ss_ce(0),\n";
      else
        vhdlCode[i + 1] += "\n\tce => '1',\n";
    }
  }

  // Special case: only LSQ is used without MC
  for (auto i = 0; i < vhdlCode.size(); i++) {
    auto s = vhdlCode[i];
    if (s.find("LSQ_") != std::string::npos &&
        s.find(": entity work.LSQ_") != std::string::npos) {
      auto name =
          s.substr(s.find("LSQ_") + 4, s.find(":") - s.find("LSQ_") - 4);
      if (std::find(names.begin(), names.end(), name) != names.end())
        continue;

      bool search = false;
      for (auto const &sa : sharedArrays)
        if (sa->name == name) {
          search = true;
          break;
        }

      if (!search)
        continue;

      auto j = i;
      while (vhdlCode[j].find("io_memIsReadyForLoads => ") == std::string::npos)
        j++;
      vhdlCode[j] = "\tio_memIsReadyForLoads => DA_" + name + "_ss_ce(0),";
      vhdlCode[j + 1] = "\tio_memIsReadyForStores => DA_" + name + "_ss_ce(0),";
    }
  }
}

static void rewriteEnd(ArrayRef<SharedMemory *> sharedArrays,
                       std::vector<std::string> &vhdlCode) {
  auto i = 0;
  while (vhdlCode[i].find("work.end_node") == std::string::npos)
    i++;
  auto constraint = vhdlCode[i].substr(vhdlCode[i].rfind("("));
  constraint =
      constraint.substr(constraint.find("(1,") + 3,
                        constraint.find(",", constraint.find("(1,") + 3) -
                            constraint.find("(1,") - 3);
  auto eCount = std::stoi(constraint);
  replace(vhdlCode[i], "(1," + constraint + ",",
          "(1," + std::to_string(eCount + sharedArrays.size()) + ",");
  while (i < vhdlCode.size() &&
         vhdlCode[i].find("eValidArray(" + std::to_string(eCount - 1) +
                          ") => ") == std::string::npos)
    i++;
  if (i == vhdlCode.size()) {
    i = 0;
    while (vhdlCode[i].find("work.end_node") == std::string::npos)
      i++;
    i++;
  }
  auto noEline = i;
  vhdlCode[i] += "\n";
  for (auto j = 0; j < sharedArrays.size(); j++)
    vhdlCode[i] += "\teValidArray(" + std::to_string(j + eCount) +
                   ") => end_0_pValidArray_" + std::to_string(j + eCount + 1) +
                   ",\n";
  while (i < vhdlCode.size() &&
         vhdlCode[i].find("eReadyArray(" + std::to_string(eCount - 1) +
                          ") => ") == std::string::npos)
    i++;
  if (i == vhdlCode.size())
    i = noEline;
  vhdlCode[i] += "\n";
  for (auto j = 0; j < sharedArrays.size(); j++)
    vhdlCode[i] += "\teReadyArray(" + std::to_string(j + eCount) +
                   ") => end_0_readyArray_" + std::to_string(j + eCount + 1) +
                   ",\n";

  i = 0;
  while (vhdlCode[i].find("architecture behavioral of " + opt_top + " is") ==
         std::string::npos)
    i++;
  i++;

  for (auto j = 0; j < sharedArrays.size(); j++)
    vhdlCode[i] += "\tsignal end_0_pValidArray_" +
                   std::to_string(j + eCount + 1) +
                   " : std_logic;\n\tsignal "
                   "end_0_readyArray_" +
                   std::to_string(j + eCount + 1) + " : std_logic;\n";

  i = 0;
  while (vhdlCode[i].find("begin") == std::string::npos)
    i++;
  i++;

  for (auto j = 0; j < sharedArrays.size(); j++)
    vhdlCode[i] += "\tend_0_pValidArray_" + std::to_string(j + eCount + 1) +
                   " <= DA_" + sharedArrays[j]->name + "_valid;\n";
}

static void addMemoryArbitrationLogic(std::vector<std::string> &vhdlCode,
                                      ArrayRef<SharedMemory *> sharedArrays) {
  for (auto sa : sharedArrays)
    rewriteMemory(sa, vhdlCode);
  rewriteCall(vhdlCode);
  rewriteMC(sharedArrays, vhdlCode);
  rewriteEnd(sharedArrays, vhdlCode);
}

void syncCall(std::string callName, std::string branchName,
              std::vector<std::string> &vhdlCode) {
  auto sync_out_valid = branchName + "_pValidArray_0";
  auto sync_out_ready = branchName + "_readyArray_0";

  auto i = 0;
  while (i < vhdlCode.size() &&
         vhdlCode[i].find(sync_out_valid + " <= ") == std::string::npos)
    i++;
  if (i == vhdlCode.size())
    llvm_unreachable(std::string("Cannot find sync_out_valid name: " +
                                 sync_out_valid + " for call: " + callName)
                         .c_str());

  auto sync_in_valid =
      vhdlCode[i].substr(vhdlCode[i].find("<=") + 3,
                         vhdlCode[i].find(";") - vhdlCode[i].find("<=") - 3);
  vhdlCode[i] = "";
  i = 0;
  while (vhdlCode[i].find(" <= " + sync_out_ready) == std::string::npos)
    i++;
  auto sync_in_ready = vhdlCode[i].substr(0, vhdlCode[i].find(" <="));
  vhdlCode[i] = "";

  i = 0;
  while (vhdlCode[i].find(callName + ": ") == std::string::npos)
    i++;
  i++;
  vhdlCode[i] += "\n\tsync_in_ready => " + sync_in_ready +
                 ",\n\tsync_out_valid => " + sync_out_valid +
                 ",\n\tsync_in_valid => " + sync_in_valid +
                 ",\n\tsync_out_ready => " + sync_out_ready + ",\n";
}

static void addSyncConnections(std::vector<std::string> &vhdlCode,
                               ENode_vec *enode_dag,
                               ArrayRef<SharedMemory *> sharedArrays) {
  for (auto enode : *enode_dag) {
    if (!isSSCall(enode))
      continue;

    auto callInst = dyn_cast<CallInst>(enode->Instr);

    auto branchName =
        needSyncWith(enode, callInst->getCalledFunction()->getName().str(),
                     sharedArrays, enode_dag);
    // Call in the last BB / having no shared memory does not need to sync
    if (branchName == "")
      continue;
    // remove ""
    branchName = branchName.substr(1, branchName.rfind("\"") - 1);
    auto callName = getNodeDotNameNew(enode);
    callName = callName.substr(1, callName.rfind("\"") - 1);

    syncCall(callName, branchName, vhdlCode);
  }
}

static void updateLoopInterchangerDepths(std::vector<std::string> &vhdlCode) {
  auto fileName = "./loop_interchange.tcl";
  std::ifstream ifile(fileName);
  if (!ifile.is_open())
    llvm_unreachable("Cannot find loop_interchange.tcl.\n");
  std::vector<std::string> constraints;
  std::string line;
  while (std::getline(ifile, line))
    constraints.push_back(line);
  ifile.close();

  int i = 0;
  for (auto constraint : constraints) {
    auto depth = std::stoi(constraint.substr(constraint.rfind(",") + 1));
    auto j = 0;
    while (j < vhdlCode.size() && vhdlCode[j].find("loop_" + std::to_string(i) +
                                                   ":") == std::string::npos ||
           vhdlCode[j].find("loop_interchanger") == std::string::npos)
      j++;
    if (j == vhdlCode.size()) {
      llvm::errs() << "loop_" << i << ": \n";
      llvm_unreachable("Cannot find loop_interchanger in vhdl.\n");
    }
    llvm::errs() << "Loop_" << i << " has a depth of " << depth << "\n";
    vhdlCode[j] = vhdlCode[j].substr(0, vhdlCode[j].rfind(")")) + ", " +
                  std::to_string(depth) + ")\n";
    i++;
  }
}

static void updateDASSFIFODepth(std::vector<std::string> &vhdlCode) {
  for (auto &line : vhdlCode)
    if (line.find("DASSFIFO_") != std::string::npos &&
        line.find("_op(arch)") != std::string::npos) {

      std::string opName;
      bool isTransparent;
      if (line.find("DASSFIFO_B") != std::string::npos) {
        opName = "DASSFIFO_B";
        isTransparent = false;
      } else if (line.find("DASSFIFO_T") != std::string::npos) {
        opName = "DASSFIFO_T";
        isTransparent = true;
      } else {
        llvm::errs() << line << "\n";
        llvm_unreachable("Unknown DASS buffer type!.");
      }

      auto depth = std::stoi(line.substr(
          line.find(opName) + 10, line.find("_", line.find(opName) + 11)));
      assert(depth >= 1);

      std::string compName;
      if (depth > 1)
        compName = isTransparent ? "transpFifo" : "nontranspFifo";
      else
        compName = isTransparent ? "TEHB" : "elasticBuffer";

      line = line.substr(0, line.find("work.") + 5) + compName +
             line.substr(line.find("("));
      if (depth > 1)
        line = line.substr(0, line.rfind(")")) + ", " + std::to_string(depth) +
               ")";
    }
}

namespace {
class StaticIslandInsertionPass : public llvm::ModulePass {

public:
  static char ID;

  StaticIslandInsertionPass() : llvm::ModulePass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  bool runOnModule(Module &M) override;
};
} // namespace

void StaticIslandInsertionPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MyCFGPass>();
}

bool StaticIslandInsertionPass::runOnModule(Module &M) {
  auto fileName = "./rtl/" + opt_top + ".vhd";
  std::ifstream ifile(fileName);
  if (!ifile.is_open())
    llvm_unreachable(
        std::string("Cannot find RTL file " + fileName + ".\n").c_str());

  std::vector<std::string> vhdlCode;
  std::string line;
  while (std::getline(ifile, line))
    vhdlCode.push_back(line);
  ifile.close();

  Function *dsFunc;
  auto count = 0;
  for (auto &F : M) {
    if (!F.hasFnAttribute("dass_ss") && F.getName() != "main" && !F.empty()) {
      dsFunc = &F;
      count++;
    }
  }
  if (count > 1)
    llvm::errs() << "Warning: Found more than one DS function. Suggest to "
                    "inline all the DS regions.";

  if (dsFunc) {
    auto sharedArrays = getSharedArrays(dsFunc);
    llvm::errs() << "Found " << sharedArrays.size()
                 << " shared arrays between SS and DS functions.\n";
    addMemoryArbitrationLogic(vhdlCode, sharedArrays);
    auto enode_dag = getAnalysis<MyCFGPass>(*dsFunc).enode_dag;
    addSyncConnections(vhdlCode, enode_dag, sharedArrays);
  }

  updateLoopInterchangerDepths(vhdlCode);

  updateDASSFIFODepth(vhdlCode);

  std::error_code ec;
  llvm::raw_fd_ostream outfile("./rtl/" + opt_top + "_new.vhd", ec);
  for (auto &s : vhdlCode)
    outfile << s << "\n";
  outfile.close();
  return true;
}

//--------------------------------------------------------//
// Pass registration
//--------------------------------------------------------//

char CollectRTLPass::ID = 1;
static RegisterPass<CollectRTLPass>
    X0("collect-ss-rtl", "Collect VHDL files for SS functions", false, false);

char SSWrapperPass::ID = 2;
static RegisterPass<SSWrapperPass>
    X1("ss-wrapper-gen", "Generate RTL wrappers for static islands", false,
       false);

char StaticIslandInsertionPass::ID = 3;
static RegisterPass<StaticIslandInsertionPass>
    X2("dass-vhdl-rewrite", "Rewrite DS netlist for SS functions", false,
       false);

/* for clang pass registration */
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static void registerClangPass(const PassManagerBuilder &,
                              legacy::PassManagerBase &PM) {
  PM.add(new SSWrapperPass());
  PM.add(new CollectRTLPass());
  PM.add(new StaticIslandInsertionPass());
}
