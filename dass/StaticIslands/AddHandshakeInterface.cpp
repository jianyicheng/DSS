//--------------------------------------------------------//
// Pass: DirectSynthesisPass
// Directly synthesize the function using block level interface.
//
// Pass: AddHandshakeInterfacePass
// Add handshake pragmas to all the non-array function arguments
// #pragma interface hls::stream
//--------------------------------------------------------//

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "AutopilotParser.h"
#include "StaticIslands.h"

#include <cassert>
#include <map>

using namespace llvm;
using namespace AutopilotParser;

cl::opt<std::string> opt_irDir("ir_dir", cl::desc("Input LLVM IR file"),
                               cl::Hidden, cl::init("./vhls"), cl::Optional);
cl::opt<bool> opt_hasIP("has_ip", cl::desc("Whether this function has IPs"),
                        cl::Hidden, cl::init(false), cl::Optional);
cl::opt<std::string> opt_target("target", cl::desc("Synthesis device target"),
                                cl::Hidden, cl::init("xc7z020clg484-1"),
                                cl::Optional);
// xcvu125-flva2104-1-i also supported
cl::opt<std::string> opt_top("top", cl::desc("top design"), cl::Hidden,
                             cl::init("top"), cl::Optional);
cl::opt<std::string> opt_DASS("dass_dir", cl::desc("Directory of DASS"),
                              cl::Hidden, cl::init("/workspace"), cl::Optional);
cl::opt<bool>
    opt_offset("has_offset",
               cl::desc("Added offset constraints to the SS functions"),
               cl::Hidden, cl::init(true), cl::Optional);

//--------------------------------------------------------//
// Pass declaration: DirectSynthesisPass
//--------------------------------------------------------//

namespace {
class DirectSynthesisPass : public llvm::ModulePass {

public:
  static char ID;

  DirectSynthesisPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

static void printHLSTCl(llvm::raw_fd_ostream &out, std::string projName,
                        std::string fname, std::string irFile) {
  out << "open_project -reset " << projName << "\n"
      << "set_top " << fname << "\nadd_files {dummy.cpp} \n"
      << "open_solution -reset \"solution1\"\n"
      << "set_part {" << opt_target << "}\n"
      << "create_clock -period 10 -name default\n"
      << "config_bind -effort high\n"
      << "config_interface -clock_enable\n"
      << "config_compile -pipeline_loops 1\n"
      << "set ::LLVM_CUSTOM_CMD {" << opt_DASS
      << "/llvm/build/bin/opt -no-warn " << irFile
      << " -o $LLVM_CUSTOM_OUTPUT}\n"
      << "csynth_design\n";
  if (opt_hasIP)
    out << "export_design -flow syn -rtl vhdl -format ip_catalog\n";
}

bool DirectSynthesisPass::runOnModule(Module &M) {
  assert(opt_irDir != "" && "Please specify the input LLVM IR file");
  std::error_code ec;
  llvm::raw_fd_ostream tclOut(std::string(opt_irDir + "/ss_direct.tcl"), ec),
      dummyC(std::string(opt_irDir + "/dummy.cpp"), ec);

  auto ssCount = 0;

  for (auto &F : M) {
    auto fname = F.getName().str();
    if (fname == "main" || !F.hasFnAttribute("dass_ss"))
      continue;

    DominatorTree DT(F);
    LoopInfo LI(DT);

    std::string isSS = F.getFnAttribute("dass_ss").getValueAsString();

    // If no loop in the function, do function pipelining
    if (LI.empty()) {
      auto newModule = CloneModule(M);
      auto ssfunc = newModule->getFunction(fname);
      ssfunc->addFnAttr("fpga.top.func", fname);
      ssfunc->addFnAttr("fpga.demangled.func", fname);
      ssfunc->addFnAttr("fpga.static.pipeline", isSS + ".-1");
      llvm::raw_fd_ostream outfile("./vhls/" + fname + "_direct.ll", ec);
      newModule->print(outfile, nullptr);
      printHLSTCl(tclOut, fname + "_direct", fname,
                  "./" + fname + "_direct.ll");
      dummyC << "void " << F.getName().str() << "(){}\n\n";
      ssCount++;
    } else
      llvm::errs()
          << "Skipping offset analysis of " << fname
          << " as it contains loops - not suitable for function pipelining.\n";
  }

  if (ssCount > 0)
    tclOut << "exit\n";
  tclOut.close();
  dummyC.close();

  return true;
}

//--------------------------------------------------------//
// Pass declaration: AddHandshakeInterfacePass
//--------------------------------------------------------//

// Create three FIFO API:
// ; Function Attrs: argmemonly nounwind
// declare i1 @llvm.fpga.fifo.nb.push.TY.p0TY(TY, TY* nocapture)
// declare { i1, TY } @llvm.fpga.fifo.nb.pop.TY.p0TY(TY* nocapture)
// declare i1 @llvm.fpga.fifo.not.empty.p0TY(TY* nocapture)
static void addFIFOAPI(Type *type, std::string typeName, Module *newModule) {

  // @llvm.fpga.fifo.nb.push.TY.p0TY
  auto pointerType = PointerType::getUnqual(type);
  auto boolType = Type::getInt1Ty(newModule->getContext());
  auto fifoPushName = "llvm.fpga.fifo.nb.push." + typeName + ".p0" + typeName;
  auto fifoPushType = newModule->getOrInsertFunction(
      fifoPushName,
      FunctionType::get(boolType, std::vector<Type *>{type, pointerType},
                        false));
  auto fifoPushFunc = cast<Function>(fifoPushType);
  fifoPushFunc->addFnAttr(llvm::Attribute::ArgMemOnly);
  fifoPushFunc->addFnAttr(llvm::Attribute::NoUnwind);
  fifoPushFunc->addAttribute(2, llvm::Attribute::NoCapture);

  // @llvm.fpga.fifo.nb.pop.TY.p0TY
  auto fifoPopName = "llvm.fpga.fifo.nb.pop." + typeName + ".p0" + typeName;
  auto retTypes = std::vector<Type *>{boolType, type};
  Type *structType = StructType::get(newModule->getContext(), retTypes);
  auto fifoPopType = newModule->getOrInsertFunction(
      fifoPopName,
      FunctionType::get(structType, std::vector<Type *>{pointerType}, false));
  auto fifoPopFunc = cast<Function>(fifoPopType);
  fifoPopFunc->addFnAttr(llvm::Attribute::ArgMemOnly);
  fifoPopFunc->addFnAttr(llvm::Attribute::NoUnwind);
  fifoPopFunc->addAttribute(1, llvm::Attribute::NoCapture);

  // @llvm.fpga.fifo.not.empty.p0TY
  auto fifoNotEmptyName = "llvm.fpga.fifo.not.empty.p0" + typeName;
  auto fifoNotEmptyType = newModule->getOrInsertFunction(
      fifoNotEmptyName,
      FunctionType::get(boolType, std::vector<Type *>{pointerType}, false));
  auto fifoNotEmptyFunc = cast<Function>(fifoNotEmptyType);
  fifoNotEmptyFunc->addFnAttr(llvm::Attribute::ArgMemOnly);
  fifoNotEmptyFunc->addFnAttr(llvm::Attribute::NoUnwind);
  fifoNotEmptyFunc->addAttribute(1, llvm::Attribute::NoCapture);
}

// Load pre-defined hls::stream APIs in LLVM
static void loadStreamLibrary(Function *F, Module *newModule,
                              llvm::DenseMap<int, PortInfo *> &portInfo) {
  std::vector<std::string> types;
  for (auto const &portInfo : portInfo) {
    auto port = portInfo.second;
    if (port->getType() != BRAM &&
        std::find(types.begin(), types.end(), port->getElementTypeAsString()) ==
            types.end()) {
      types.push_back(port->getElementTypeAsString());
      addFIFOAPI(port->getDataElementType(), port->getElementTypeAsString(),
                 newModule);
    }
  }
}

// Load pre-defined io.section APIs in LLVM
// ; Function Attrs: noduplicate nounwind
// declare token @llvm.directive.scope.entry() #2
// ; Function Attrs: nounwind
// declare void @_Z13_ssdm_op_Waitz(...) #3
// ; Function Attrs: noduplicate nounwind
// declare void @llvm.directive.scope.exit(token) #2
static void loadSchedulingLibrary(Function *F, Module *newModule,
                                  AutopilotParser::AutopilotParser &ap) {
  auto &portInfo = ap.getPortInfo();

  bool needLib = false;
  for (auto i = 0; i < ap.getFunction()->arg_size(); i++)
    if (portInfo[i]->getType() == INPUT && portInfo[i]->getOffset() > 0 &&
        portInfo[i]->getIdleStates() > 0) {
      needLib = true;
      break;
    }

  if (!needLib)
    return;

  // Add scheduling libraries
  // token @llvm.directive.scope.entry() #2
  auto scopeEntryName = "llvm.directive.scope.entry";
  auto tokenTy = Type::getTokenTy(newModule->getContext());
  auto scopeEntryType = newModule->getOrInsertFunction(
      scopeEntryName, FunctionType::get(tokenTy, {}, false));
  auto scopeEntryFunc = cast<Function>(scopeEntryType);
  scopeEntryFunc->addFnAttr(llvm::Attribute::NoDuplicate);
  scopeEntryFunc->addFnAttr(llvm::Attribute::NoUnwind);

  // void @_Z13_ssdm_op_Waitz(...) #3
  auto apWaitName = "_Z13_ssdm_op_Waitz";
  auto voidTy = Type::getVoidTy(newModule->getContext());
  auto apWaitType = newModule->getOrInsertFunction(
      apWaitName, FunctionType::get(voidTy, {}, true));
  auto apWaitFunc = cast<Function>(apWaitType);
  apWaitFunc->addFnAttr(llvm::Attribute::NoUnwind);

  // void @llvm.directive.scope.exit(token) #2
  auto scopeExitName = "llvm.directive.scope.exit";
  auto scopeExitType = newModule->getOrInsertFunction(
      scopeExitName, FunctionType::get(voidTy, {tokenTy}, false));
  auto scopeExitFunc = cast<Function>(scopeExitType);
  scopeExitFunc->addFnAttr(llvm::Attribute::NoDuplicate);
  scopeExitFunc->addFnAttr(llvm::Attribute::NoUnwind);
}

static void mirrorInstruction(Instruction *inst,
                              llvm::DenseMap<Value *, Value *> &valueMap,
                              llvm::DenseMap<BasicBlock *, BasicBlock *> &bbMap,
                              IRBuilder<> &builder) {
  assert(bbMap[inst->getParent()]);
  std::vector<Value *> ins;
  std::vector<int> opIndices;

  for (auto i = 0; i < inst->getNumOperands(); i++) {
    auto op = inst->getOperand(i);
    ins.push_back(valueMap[op]);
    opIndices.push_back(i);
  }

  auto newResult = mirrorInst(inst, opIndices, ins, builder);
  if (auto result = dyn_cast<Value>(inst))
    valueMap[result] = newResult;
}

// Mirror the operations in the new function
static void mirrorFunctionBody(ArrayRef<Value *> newArgs, Function *ssfunc,
                               Function *F, IRBuilder<> &builder,
                               Module *newModule,
                               AutopilotParser::AutopilotParser &ap) {
  // llvm::errs() << ap.exportOffsets() << "\n";
  auto &portInfo = ap.getPortInfo();
  std::vector<Value *> args;
  auto firstArg = F->arg_begin(), firstNewArg = ssfunc->arg_begin();
  for (auto i = 0; i < F->arg_size(); i++)
    args.push_back(firstArg + i);
  assert(newArgs.size() == args.size() || newArgs.size() == args.size() + 1);

  // Load all the arguments
  llvm::DenseMap<Value *, Value *> valueMap;
  for (auto i = 0; i < args.size(); i++)
    valueMap[args[i]] = newArgs[i];

  // Load all the constants
  for (auto BB = F->begin(); BB != F->end(); BB++)
    for (auto I = BB->begin(); I != BB->end(); I++)
      for (auto op = 0; op < I->getNumOperands(); op++) {
        auto opValue = I->getOperand(op);
        if (!valueMap.count(opValue) && isa<Constant>(opValue))
          valueMap[opValue] = opValue;
      }

  llvm::DenseMap<BasicBlock *, BasicBlock *> bbMap;
  auto bbCount = 0;
  bbMap[&*(F->begin())] = builder.GetInsertBlock();

  // Mirror BBs
  for (auto BB = F->begin(); BB != F->end(); BB++)
    if (!bbMap.count(&*BB))
      bbMap[&*BB] = BasicBlock::Create(
          ssfunc->getContext(), "bb_" + std::to_string(bbCount++), ssfunc);

  // Mirror instructions
  BasicBlock *retPoint;
  for (auto BB = F->begin(); BB != F->end(); BB++) {
    builder.SetInsertPoint(bbMap[&*BB]);
    for (auto I = BB->begin(); I != BB->end(); I++) {
      // Mirror BB-related instructions: ret, br, phi
      if (isa<llvm::ReturnInst>(I)) {
        retPoint = bbMap[&*BB];
        continue;
      }
      if (auto branchInst = dyn_cast<llvm::BranchInst>(I)) {
        if (branchInst->isUnconditional()) {
          auto newBranchInst =
              builder.CreateBr(bbMap[branchInst->getSuccessor(0)]);
        } else {
          auto newBranchInst =
              builder.CreateCondBr(valueMap[branchInst->getCondition()],
                                   bbMap[branchInst->getSuccessor(0)],
                                   bbMap[branchInst->getSuccessor(1)]);
        }
        continue;
      }
      if (auto phiNode = dyn_cast<llvm::PHINode>(I)) {
        auto inCount = phiNode->getNumIncomingValues();
        auto newPhiNode = builder.CreatePHI(phiNode->getType(), 0);
        valueMap[phiNode] = newPhiNode;
        continue;
      }

      // Mirror output port
      if (auto storeInst = dyn_cast<llvm::StoreInst>(I)) {
        auto stDst = storeInst->getPointerOperand();
        auto stType = stDst->getType();
        auto argIter = std::find(args.begin(), args.end(), stDst);
        if (stType->isPointerTy() && argIter != args.end() &&
            portInfo[argIter - args.begin()]->getFIFODepth() == 0) {
          auto typeName =
              portInfo[argIter - args.begin()]->getElementTypeAsString();
          auto fifoPushName =
              "llvm.fpga.fifo.nb.push." + typeName + ".p0" + typeName;
          auto fifoPushFunc = newModule->getFunction(fifoPushName);
          if (!fifoPushFunc) {
            llvm::errs() << fifoPushName << "\n";
            llvm_unreachable("Function not found.");
          }

          auto callInst = builder.CreateCall(
              fifoPushFunc,
              std::vector<Value *>{valueMap[storeInst->getValueOperand()],
                                   valueMap[stDst]});
          continue;
        }
      }

      auto latency = ap.getLatency(), delay = latency, opLatency = 0;
      std::string argName;
      // auto minIdleStates = minIdleStates;
      for (auto op = 0; op < I->getNumOperands(); op++)
        if (auto arg = dyn_cast<Argument>(I->getOperand(op)))
          for (auto &portInfo : portInfo) {
            auto port = portInfo.second;
            if (valueMap[arg] == port->getValToUse() &&
                port->getType() == INPUT && port->getIdleStates() != 0) {
              auto d = port->getFIFODepth();
              opLatency = (delay > d) ? port->getFirstOpLatency() : opLatency;
              delay = std::min(delay, d);
              argName = arg->getName().str();
            }
          }

      Value *entry;
      if (delay != latency && delay != 0) {
        assert(opLatency > 0);
        OperandBundleDef bd = OperandBundleDef(
            "xlx_protocol", (std::vector<Value *>){ConstantInt::get(
                                Type::getInt32Ty(newModule->getContext()), 0)});
        auto entryFunc = newModule->getFunction("llvm.directive.scope.entry");
        assert(entryFunc);
        entry = builder.CreateCall(entryFunc, {}, {bd});
        for (auto i = 0; i < delay + 1; i++)
          builder.CreateCall(
              newModule->getFunction("_Z13_ssdm_op_Waitz"),
              {ConstantInt::get(Type::getInt32Ty(newModule->getContext()), 1)});
        llvm::errs() << "Force arg " << argName << " depth = " << delay << "\n";
      }

      // Mirror other instructions
      mirrorInstruction(&*I, valueMap, bbMap, builder);

      if (delay != latency && delay != 0) {
        for (auto i = 0; i < opLatency - 1; i++)
          builder.CreateCall(
              newModule->getFunction("_Z13_ssdm_op_Waitz"),
              {ConstantInt::get(Type::getInt32Ty(newModule->getContext()), 1)});
        auto exitFunc = newModule->getFunction("llvm.directive.scope.exit");
        assert(exitFunc);
        builder.CreateCall(exitFunc, {entry});
      }
    }
  }

  // Sync phi nodes
  for (auto BB = F->begin(); BB != F->end(); BB++)
    for (auto I = BB->begin(); I != BB->end(); I++)
      if (auto phiNode = dyn_cast<llvm::PHINode>(I)) {
        auto newPhiNode = dyn_cast<llvm::PHINode>(valueMap[phiNode]);
        assert(newPhiNode && newPhiNode->getNumIncomingValues() == 0);
        for (auto i = 0; i < phiNode->getNumIncomingValues(); i++)
          newPhiNode->addIncoming(valueMap[phiNode->getIncomingValue(i)],
                                  bbMap[phiNode->getIncomingBlock(i)]);
      }

  assert(retPoint);
  builder.SetInsertPoint(retPoint);
}

// Create a template for handshake function:
// if (exists one input fifo is empty):
//   goto ret_bb;
// read all inputs
// Compute ...
// ret_bb:
//   return;
// return the place where right after all the inputs are read

// Naive and tree
static Value *createAndTree(ArrayRef<Value *> andTree, IRBuilder<> &builder) {
  assert(!andTree.empty());
  auto accum = andTree[0];
  for (unsigned i = 1; i < andTree.size(); i++)
    accum = builder.CreateAnd(accum, andTree[i]);
  return accum;
}

static void createHandshakeFunctionBody(Function *ssfunc, Function *F,
                                        Module *newModule,
                                        AutopilotParser::AutopilotParser &ap) {
  auto &portInfo = ap.getPortInfo();
  // Precondition check - FIFO emptyness
  auto emptyCheck =
      BasicBlock::Create(ssfunc->getContext(), "empty_check", ssfunc);
  IRBuilder<> builder(emptyCheck);
  std::vector<Value *> andTree;
  for (auto &arg : ssfunc->args()) {
    auto port = portInfo[arg.getArgNo()];
    port->setValToUse(&arg);
    if (port->getType() == INPUT && !port->getOffset()) {
      assert(arg.getType()->isPointerTy());
      auto fifoNotEmptyName =
          "llvm.fpga.fifo.not.empty.p0" + port->getElementTypeAsString();
      auto checkEmptyFunc = newModule->getFunction(fifoNotEmptyName);
      if (!checkEmptyFunc) {
        llvm::errs() << fifoNotEmptyName << "\n";
        llvm_unreachable("Function not found.");
      }
      auto callInst =
          builder.CreateCall(checkEmptyFunc, std::vector<Value *>{&arg});
      auto isNotEmpty = dyn_cast<Value>(callInst);
      assert(isNotEmpty);
      andTree.push_back(isNotEmpty);
    }
  }
  if (!andTree.size())
    llvm_unreachable("Found function with no input - no control input added!");

  // FIFO read
  auto startCompute =
      (andTree.size() > 1) ? createAndTree(andTree, builder) : andTree[0];
  auto entry = BasicBlock::Create(ssfunc->getContext(), "entry", ssfunc);
  auto exit = BasicBlock::Create(ssfunc->getContext(), "exit", ssfunc);
  builder.CreateCondBr(startCompute, entry, exit);
  builder.SetInsertPoint(exit);
  builder.CreateRet(nullptr);
  builder.SetInsertPoint(entry);

  // newArgs contain loaded values from the FIFO as input args
  std::vector<Value *> newArgs;
  for (auto i = 0; i < ssfunc->arg_size(); i++) {
    auto port = portInfo[i];
    if (port->getType() == INPUT && !port->getOffset()) {
      auto readFIFOName = "llvm.fpga.fifo.nb.pop." +
                          port->getElementTypeAsString() + ".p0" +
                          port->getElementTypeAsString();
      auto readFIFOFunc = newModule->getFunction(readFIFOName);
      if (!readFIFOFunc) {
        llvm::errs() << readFIFOName << "\n";
        llvm_unreachable("Function not found.");
      }
      auto callInst = builder.CreateCall(
          readFIFOFunc, std::vector<Value *>{port->getValToUse()});
      auto retValue = dyn_cast<Value>(callInst);
      port->setValToUse(builder.CreateExtractValue(retValue, 1));

    } else if (port->getType() == BRAM) {
      // Add GEPInst which does not exist in DS functions
      auto c0 = builder.getInt64(0);
      auto getElementPtrInst =
          builder.CreateInBoundsGEP(port->getValToUse(), {c0, c0});
      port->setValToUse(getElementPtrInst);
    }
    newArgs.push_back(port->getValToUse());
  }

  mirrorFunctionBody(newArgs, ssfunc, F, builder, newModule, ap);
  builder.CreateBr(exit);
}

// Extract array size infomation in LLVM
static Type *getPointerTypeWithSize(Argument *arg) {
  auto argNo = arg->getArgNo();
  auto func = arg->getParent();
  if (func->use_empty())
    return arg->getType();
  else {
    auto callInst = dyn_cast<CallInst>(func->use_begin()->getUser());
    assert(callInst);
    auto value = callInst->getOperand(argNo);
    if (auto nextArg = dyn_cast<Argument>(value))
      return getPointerTypeWithSize(nextArg);
    else {
      auto op = value->use_begin()->get();
      if (auto getElementPtrInst = dyn_cast<GetElementPtrInst>(op))
        return getElementPtrInst->getPointerOperand()->getType();
      else if (isa<AllocaInst>(op))
        return arg->getType();
      else {
        llvm::errs() << *arg << "\n";
        llvm_unreachable("Unknown case when analyzing argument pointer type.");
      }
    }
  }
}

// Create function declaration properly
static Function *declareNewFunction(Function *F, Module *newModule,
                                    AutopilotParser::AutopilotParser &ap) {
  auto &portInfo = ap.getPortInfo();
  std::vector<Type *> formalPorts;
  auto argNum = F->arg_size();
  for (auto arg = F->arg_begin(); arg != F->arg_end(); arg++) {
    auto argType = arg->getType();
    if (argType->isPointerTy())
      formalPorts.push_back(getPointerTypeWithSize(arg));
    else {
      if (!portInfo.count(arg->getArgNo())) {
        llvm::errs() << "Warning: Cannot find offset info for "
                     << arg->getName() << " in function " << F->getName()
                     << ". Assume offset = 0."
                     << "\n";
        formalPorts.push_back(PointerType::getUnqual(argType));
      } else if (portInfo[arg->getArgNo()]->getOffset() > 0)
        formalPorts.push_back(argType);
      else
        formalPorts.push_back(PointerType::getUnqual(argType));
    }
  }
  // If there is no input, create a dummy control input
  if (ap.hasDummyInput())
    formalPorts.push_back(
        PointerType::getUnqual(portInfo[F->arg_size()]->getDataType()));

  auto funcType = newModule->getOrInsertFunction(
      F->getName(), FunctionType::get(Type::getVoidTy(newModule->getContext()),
                                      formalPorts, false));
  auto ssfunc = cast<Function>(funcType);
  auto argRef = F->arg_begin(), arg = ssfunc->arg_begin();
  for (auto i = 0; i < argNum; i++) {
    arg->setName(argRef->getName());
    arg++;
    argRef++;
  }
  if (ssfunc->arg_size() > argNum)
    arg->setName("control");
  return ssfunc;
}

static int getMinimumLegalII(Function *ssfunc, Function *F,
                             AutopilotParser::AutopilotParser &ap) {
  auto &portInfo = ap.getPortInfo();
  auto latency = ap.getLatency();
  auto minII = 1;
  for (unsigned int i = 0; i < ssfunc->arg_size(); i++) {
    auto port = portInfo[i];
    if (port->getType() != INPUT || port->getOffset() == 0)
      continue;

    // TODO: Add carried dependence analysis
    auto pathLatency = latency - port->getFIFODepth() + 1;
    if (pathLatency > minII)
      minII = pathLatency;
  }
  return minII;
}

// Add VHLS specific attributes to the new function
static void addVHLSAttr(Function *ssfunc, Function *F,
                        AutopilotParser::AutopilotParser &ap) {
  auto &portInfo = ap.getPortInfo();
  auto fname = ssfunc->getName();
  // Basic requirements for Vitis HLS
  ssfunc->addFnAttr("fpga.top.func", fname);
  ssfunc->addFnAttr("fpga.demangled.func", fname);
  // Function pipelining with automated II searching
  std::string isSS = F->getFnAttribute("dass_ss").getValueAsString();
  auto DT = llvm::DominatorTree(*F);
  LoopInfo LI(DT);
  if (LI.empty()) {
    auto II = std::stoi(isSS);
    auto minLegalII = getMinimumLegalII(ssfunc, F, ap);
    auto finalII = std::max(II, minLegalII);
    if (minLegalII > II)
      llvm::errs()
          << "Warning: Specified II does not meet the minimum II constraint.\n";
    llvm::errs() << "Final II of static function " << fname << " = " << finalII
                 << "\n";

    ssfunc->addFnAttr("fpga.static.pipeline", std::to_string(finalII) + ".-1");
  }
  // You can add more pragmas for ss functions here...

  // Add dereferenced attributes - for hls::stream
  auto arg = ssfunc->arg_begin();
  for (unsigned int i = 0; i < ssfunc->arg_size(); i++) {
    auto port = portInfo[i];
    if ((port->getType() != INPUT && port->getType() != OUTPUT) ||
        port->getFIFODepth() > 0) {
      arg++;
      continue;
    }
    auto type = arg->getType()->getPointerElementType();
    auto bytes = (type->isDoubleTy())
                     ? 8
                     : (type->isFloatTy())
                           ? 4
                           : (type->isIntegerTy())
                                 ? (type->getIntegerBitWidth() + 7) / 8
                                 : -1;
    if (bytes == -1) {
      llvm::errs() << fname << " " << i << " " << *type << "\n";
      llvm_unreachable("Unknown type of function argument");
    }
    // Have i+1 to skip return type
    ssfunc->addDereferenceableAttr(i + 1, bytes);
    arg++;
  }

  // Add ap_memory attribute
  auto attributeList = ssfunc->getAttributes();
  arg = ssfunc->arg_begin();
  for (unsigned int i = 0; i < ssfunc->arg_size(); i++) {
    if (portInfo[i]->getType() == BRAM) {
      auto arrayName = arg->getName().str();
      attributeList = attributeList.addAttribute(ssfunc->getContext(), i + 1,
                                                 "fpga.address.interface",
                                                 "ap_memory." + arrayName);
      auto &C = ssfunc->getContext();
      IRBuilder<> builder(&*(ssfunc->begin()));
      SmallVector<Metadata *, 32> ops;
      ops.push_back(llvm::MDString::get(C, arrayName));
      ops.push_back(llvm::MDString::get(C, "ap_memory"));
      ops.push_back(llvm::ConstantAsMetadata::get(builder.getInt32(666)));
      ops.push_back(llvm::ConstantAsMetadata::get(builder.getInt32(211)));
      ops.push_back(llvm::ConstantAsMetadata::get(builder.getInt32(-1)));
      auto *N = MDTuple::get(C, ops);
      ssfunc->setMetadata("fpga.adaptor.bram." + arrayName, N);
    }
    arg++;
  }
  ssfunc->setAttributes(attributeList);
}

// In llvm/ADT/STLExtras.h 13.0
/// Return true if the sequence [Begin, End) has exactly N items. Runs
/// in O(N)
/// time. Not meant for use with random-access iterators.
/// Can optionally take a predicate to filter lazily some items.
template <typename IterTy,
          typename Pred = bool (*)(const decltype(*std::declval<IterTy>()) &)>
bool hasNItems(
    IterTy &&Begin, IterTy &&End, unsigned N,
    Pred &&ShouldBeCounted =
        [](const decltype(*std::declval<IterTy>()) &) { return true; },
    std::enable_if_t<
        !std::is_base_of<std::random_access_iterator_tag,
                         typename std::iterator_traits<std::remove_reference_t<
                             decltype(Begin)>>::iterator_category>::value,
        void> * = nullptr) {
  for (; N; ++Begin) {
    if (Begin == End)
      return false; // Too few.
    N -= ShouldBeCounted(*Begin);
  }
  for (; Begin != End; ++Begin)
    if (ShouldBeCounted(*Begin))
      return false; // Too many.
  return true;
}

static void replaceBBInPHINodes(Function *F, BasicBlock *oldBlock,
                                BasicBlock *newBlock) {
  for (auto BB = F->begin(); BB != F->end(); BB++)
    for (auto I = BB->begin(); I != BB->end(); I++)
      if (auto phiNode = dyn_cast<PHINode>(I))
        for (auto &block : phiNode->blocks())
          if (block == oldBlock)
            block = newBlock;
}

// Get rid of empty basic block which just forwards the control flow
static void removeEmptyBasicBlocks(Function *F) {
  std::vector<BasicBlock *> rmBBs;

  for (auto BB = F->begin(); BB != F->end(); BB++) {
    if (BB->size() > 1 || !hasNItems(pred_begin(&*BB), pred_end(&*BB), 1))
      continue;

    if (auto branchInst = dyn_cast<llvm::BranchInst>(BB->getTerminator())) {
      if (branchInst->isConditional())
        continue;

      auto bbAsValue = dyn_cast<Value>(BB);
      auto succAsValue = dyn_cast<Value>(BB->getSingleSuccessor());
      assert(bbAsValue && succAsValue);

      replaceBBInPHINodes(F, &*BB, BB->getSinglePredecessor());

      bbAsValue->replaceAllUsesWith(succAsValue);
      rmBBs.push_back(&*BB);
    }
  }

  for (auto BB : rmBBs) {
    assert(BB->use_empty());
    BB->eraseFromParent();
  }
}

// Transform a function into a handshake function in a new module
static void transformToHandshakeFunction(Function *F, Module *newModule) {
  auto fname = F->getName().str();
  auto DT = llvm::DominatorTree(*F);
  LoopInfo LI(DT);
  AutopilotParser::AutopilotParser ap;
  if (LI.empty()) {
    auto project = fname + "_direct";
    auto file = opt_irDir + "/" + project + "/solution1/.autopilot/db/" +
                fname + ".verbose.sched.rpt";
    std::ifstream schedRpt(file);
    if (!schedRpt.is_open())
      llvm_unreachable(std::string("Pre-schedule report of function " + fname +
                                   " not found: " + file)
                           .c_str());
    ap = AutopilotParser::AutopilotParser(schedRpt, F);
    ap.anlayzePortInfo(opt_offset);
    ap.analyzeIdleStatesAndFirstOpLatency();
  } else {
    ap = AutopilotParser::AutopilotParser(F);
    ap.analyzePortInfoFromSource();
  }

  auto ssfunc = declareNewFunction(F, newModule, ap);
  loadStreamLibrary(F, newModule, ap.getPortInfo());
  loadSchedulingLibrary(F, newModule, ap);
  createHandshakeFunctionBody(ssfunc, F, newModule, ap);
  addVHLSAttr(ssfunc, F, ap);
  removeEmptyBasicBlocks(ssfunc);
}

namespace {
class AddHandshakeInterfacePass : public llvm::ModulePass {

public:
  static char ID;

  AddHandshakeInterfacePass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool AddHandshakeInterfacePass::runOnModule(Module &M) {
  LLVMContext TheContext;
  std::vector<Function *> ssFuncs;
  for (auto &F : M)
    if (F.hasFnAttribute("dass_ss")) {
      ssFuncs.push_back(&F);
      if (!F.getReturnType()->isVoidTy())
        llvm_unreachable("Only functions with a void return type can "
                         "be synthesized!");
    }
  if (ssFuncs.empty())
    return true;

  assert(opt_irDir != "" && "Please specify the input LLVM IR file");
  std::error_code ec;
  std::string tclName, irName, prjName;
  tclName = "ss.tcl";
  irName = ".ll";
  prjName = "";

  llvm::raw_fd_ostream tclOut(std::string(opt_irDir + "/" + tclName), ec),
      dummyC(std::string(opt_irDir + "/dummy.cpp"), ec);

  for (auto F : ssFuncs) {
    std::string fname = F->getName().str();
    Module *newModule = new Module(std::string("vhls_" + fname), TheContext);
    transformToHandshakeFunction(F, newModule);
    llvm::raw_fd_ostream outfile("./vhls/" + fname + irName, ec);
    newModule->print(outfile, nullptr);

    printHLSTCl(tclOut, fname + prjName, fname, "./" + fname + irName);
    dummyC << "void " << F->getName().str() << "(){}\n\n";
  }
  tclOut << "exit\n";
  tclOut.close();
  dummyC.close();
  return true;
}

//--------------------------------------------------------//
// Pass registration
//--------------------------------------------------------//

char DirectSynthesisPass::ID = 1;
static RegisterPass<DirectSynthesisPass>
    X0("pre-synthesise", "Pre-synthesis scripts for static islands", false,
       false);

char AddHandshakeInterfacePass::ID = 2;
static RegisterPass<AddHandshakeInterfacePass>
    X1("add-hs-interface", "Configure static islands with handshake interface",
       false, false);
