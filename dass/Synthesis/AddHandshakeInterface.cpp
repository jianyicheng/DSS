//--------------------------------------------------------//
// Pass: AddHandshakeInterfacePass
// Add handshake pragmas to all the non-array function arguments
// #pragma interface ap_hs
//--------------------------------------------------------//

#include <cassert>

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

#include "ElasticPass/Head.h"

#include "InstrTransform.h"

#include <map>

using namespace llvm;

// get type name as string
static std::string typeNameToString(Type *type) {
  assert(!type->isArrayTy() && !type->isPointerTy());
  return (type->isDoubleTy())
             ? "f64"
             : (type->isFloatTy())
                   ? "f32"
                   : (type->isIntegerTy())
                         ? "i" + std::to_string(type->getIntegerBitWidth())
                         : "X";
}

// Create three FIFO API:
// ; Function Attrs: argmemonly nounwind
// declare i1 @llvm.fpga.fifo.nb.push.TY.p0TY(TY, TY* nocapture)
// declare { i1, TY } @llvm.fpga.fifo.nb.pop.TY.p0TY(TY* nocapture)
// declare i1 @llvm.fpga.fifo.not.empty.p0TY(TY* nocapture)
static void addFIFOAPI(Type *type, Module *newModule) {
  auto typeName = typeNameToString(type);
  assert(typeName != "");

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
static void loadStreamLibrary(Function *ssfunc, Module *newModule) {
  std::vector<Type *> types;
  for (auto arg = ssfunc->arg_begin(); arg != ssfunc->arg_end(); arg++) {
    if (arg->getType()->isPointerTy()) {
      auto type = arg->getType()->getPointerElementType();
      if (std::find(types.begin(), types.end(), type) == types.end())
        types.push_back(type);
    }
  }

  for (auto type : types)
    addFIFOAPI(type, newModule);
}

static void mirrorInstruction(Instruction *inst,
                              llvm::DenseMap<Value *, Value *> &valueMap,
                              llvm::DenseMap<BasicBlock *, BasicBlock *> &bbMap,
                              IRBuilder<> &builder) {
  assert(bbMap[inst->getParent()]);
  std::vector<Value *> ins;
  std::vector<int> opIndices;

  for (auto i = 0; i < inst->getNumOperands(); i++) {
    ins.push_back(valueMap[inst->getOperand(i)]);
    opIndices.push_back(i);
  }

  auto newResult = mirrorInst(inst, opIndices, ins, builder);
  if (auto result = dyn_cast<Value>(inst))
    valueMap[result] = newResult;
}

// Mirror the operations in the new function
static void mirrorFunctionBody(ArrayRef<Value *> newArgs, Function *ssfunc,
                               Function *F, IRBuilder<> &builder,
                               Module *newModule) {
  std::vector<Value *> args;
  auto firstArg = F->arg_begin(), firstNewArg = ssfunc->arg_begin();
  for (auto i = 0; i < F->arg_size(); i++)
    args.push_back(firstArg + i);
  assert(newArgs.size() == args.size() || newArgs.size() == args.size() + 1);

  llvm::DenseMap<Value *, Value *> valueMap;
  for (auto i = 0; i < args.size(); i++)
    valueMap[args[i]] = newArgs[i];

  llvm::DenseMap<BasicBlock *, BasicBlock *> bbMap;
  auto bbCount = 0;
  bbMap[&*(F->begin())] = builder.GetInsertBlock();

  BasicBlock *retPoint;
  for (auto BB = F->begin(); BB != F->end(); BB++) {
    if (!bbMap[&*BB]) {
      auto newBB = BasicBlock::Create(
          ssfunc->getContext(), "bb_" + std::to_string(bbCount++), ssfunc);
      builder.SetInsertPoint(newBB);
      bbMap[&*BB] = newBB;
    }
    for (auto I = BB->begin(); I != BB->end(); I++) {
      if (isa<llvm::ReturnInst>(I)) {
        retPoint = bbMap[&*BB];
        continue;
      } else if (auto storeInst = dyn_cast<llvm::StoreInst>(I)) {
        auto stDst = storeInst->getPointerOperand();
        auto stType = stDst->getType();
        if (stType->isPointerTy() &&
            std::find(args.begin(), args.end(), stDst) != args.end()) {
          auto typeName = typeNameToString(stType->getPointerElementType());
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
      mirrorInstruction(&*I, valueMap, bbMap, builder);
    }
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
enum portType { INPUT, OUTPUT, BRAM };
struct IOPort {
  portType port;
  Type *dataType;
  std::string typeString;
  unsigned idx;
  Value *loadVal;
  Value *arg;
};

// Naive and tree
static Value *createAndTree(ArrayRef<Value *> andTree, IRBuilder<> &builder) {
  assert(!andTree.empty());
  auto accum = andTree[0];
  for (unsigned i = 1; i < andTree.size(); i++)
    accum = builder.CreateAnd(accum, andTree[i]);
  return accum;
}

static void createHandshakeFunctionBody(Function *ssfunc, Function *F,
                                        Module *newModule) {
  // Precondition check - FIFO emptyness
  auto emptyCheck =
      BasicBlock::Create(ssfunc->getContext(), "empty_check", ssfunc);
  IRBuilder<> builder(emptyCheck);
  std::vector<Value *> andTree;
  std::vector<IOPort *> ios;
  auto i = 0;
  auto refArg = F->arg_begin();
  for (auto &arg : ssfunc->args()) {
    auto *port = new IOPort;
    if (i == F->arg_size())
      port->port = INPUT;
    else {
      auto argType = (refArg + i)->getType();
      port->port =
          argType->isArrayTy() ? BRAM : argType->isPointerTy() ? OUTPUT : INPUT;
    }
    port->arg = dyn_cast<Value>(&arg);
    auto argType = port->arg->getType();
    if (!argType->isArrayTy() && port->port == INPUT) {
      port->dataType = argType->getPointerElementType();
      auto typeName = typeNameToString(port->dataType);
      port->typeString = typeName;
      auto fifoNotEmptyName = "llvm.fpga.fifo.not.empty.p0" + typeName;
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
    port->idx = i++;
    ios.push_back(port);
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
  auto newArg = ssfunc->arg_begin();
  std::vector<Value *> newArgs;
  for (auto port : ios) {
    if (port->port == INPUT) {
      auto readFIFOName = "llvm.fpga.fifo.nb.pop." + port->typeString + ".p0" +
                          port->typeString;
      auto readFIFOFunc = newModule->getFunction(readFIFOName);
      if (!readFIFOFunc) {
        llvm::errs() << readFIFOName << "\n";
        llvm_unreachable("Function not found.");
      }
      auto callInst = builder.CreateCall(
          readFIFOFunc, std::vector<Value *>{newArg + port->idx});
      auto retValue = dyn_cast<Value>(callInst);
      port->loadVal = builder.CreateExtractValue(retValue, 1);
      newArgs.push_back(port->loadVal);
    } else
      newArgs.push_back(port->arg);
  }

  // newArgs contain loaded values from the FIFO as input args
  mirrorFunctionBody(newArgs, ssfunc, F, builder, newModule);
  builder.CreateBr(exit);
}

// Create function declaration properly
static Function *declareNewFunction(Function *F, Module *newModule) {
  std::vector<Type *> formalPorts;
  auto argNum = F->arg_size();
  bool hasInput = false;
  for (auto arg = F->arg_begin(); arg != F->arg_end(); arg++) {
    auto argType = arg->getType();
    if (argType->isPointerTy())
      formalPorts.push_back(argType);
    else {
      hasInput = true;
      formalPorts.push_back(PointerType::getUnqual(argType));
    }
  }
  // If there is no input, create a dummy control input
  if (!hasInput)
    formalPorts.push_back(
        PointerType::getUnqual(Type::getInt1Ty(newModule->getContext())));

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

// Add VHLS specific attributes to the new function
static void addVHLSAttr(Function *ssfunc) {
  auto fname = ssfunc->getName();
  // Basic requirements for Vitis HLS
  ssfunc->addFnAttr("fpga.top.func", fname);
  ssfunc->addFnAttr("fpga.demangled.func", fname);
  // Function pipelining with automated II searching
  ssfunc->addFnAttr("fpga.static.pipeline", "-1.-1");
  // You can add more pragmas for ss functions here...

  // Add dereferenced attributes - for hls::stream
  auto argType = ssfunc->arg_begin()->getType();
  for (unsigned int i = 0; i < ssfunc->arg_size(); i++) {
    if (!argType->isPointerTy())
      continue;
    auto type = argType->getPointerElementType();
    auto bytes = (type->isDoubleTy())
                     ? 8
                     : (type->isFloatTy())
                           ? 4
                           : (type->isIntegerTy())
                                 ? (type->getIntegerBitWidth() + 7) / 8
                                 : -1;
    if (bytes == -1) {
      llvm::errs() << fname << " " << i << "\n";
      llvm_unreachable("Unknown type of function argument");
    }
    // Have i+1 to skip return type
    ssfunc->addDereferenceableAttr(i + 1, bytes);
  }
}

// Transform a function into a handshake function in a new module
static void transformToHandshakeFunction(Function *F, Module *newModule) {
  auto ssfunc = declareNewFunction(F, newModule);
  loadStreamLibrary(ssfunc, newModule);
  addVHLSAttr(ssfunc);
  createHandshakeFunctionBody(ssfunc, F, newModule);
}

//--------------------------------------------------------//
// Pass declaration
//--------------------------------------------------------//

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
        llvm_unreachable(
            "Only functions with a void return type can be synthesized!");
    }

  for (auto F : ssFuncs) {

    std::string fname = F->getName().str();
    Module *newModule = new Module(std::string("vhls_" + fname), TheContext);

    transformToHandshakeFunction(F, newModule);

    std::error_code ec;
    llvm::raw_fd_ostream outfile("./vhls/" + fname + ".ll", ec);
    newModule->print(outfile, nullptr);
  }
  return true;
}

char AddHandshakeInterfacePass::ID = 1;

static RegisterPass<AddHandshakeInterfacePass>
    Z("AddHandshakeInterface",
      "Configure static islands with handshake interface", false, false);

/* for clang pass registration */
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static void registerClangPass(const PassManagerBuilder &,
                              legacy::PassManagerBase &PM) {
  PM.add(new AddHandshakeInterfacePass());
}

static RegisterStandardPasses
    RegisterClangPass(PassManagerBuilder::EP_EarlyAsPossible,
                      registerClangPass);
