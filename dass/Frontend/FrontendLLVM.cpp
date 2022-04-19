//--------------------------------------------------------//
// Pass: FoldBitCastPass
// Remove unnecessary bit cast between float pointers
// Pass: NameLoopPass
// Name loops using Debug information
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
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

//--------------------------------------------------------//
// Pass declaration: FoldBitCastPass
//--------------------------------------------------------//

static bool canFold(BitCastInst *bitcastInst) {
  for (auto user : bitcastInst->users()) {
    auto loadInst = dyn_cast<LoadInst>(user);
    if (!loadInst)
      return false;

    for (auto lduser : loadInst->users()) {
      auto storeInst = dyn_cast_or_null<StoreInst>(lduser);
      if (!storeInst)
        return false;
      auto bitcastInst2 =
          dyn_cast_or_null<BitCastInst>(storeInst->getPointerOperand());
      if (!bitcastInst2)
        return false;
    }
  }
  return true;
}

static void foldBitCast(BitCastInst *bitcastInst) {
  IRBuilder<> builder(bitcastInst->getParent());
  std::vector<Instruction *> instrToRemove;
  for (auto user : bitcastInst->users()) {
    auto loadInst = dyn_cast<LoadInst>(user);
    builder.SetInsertPoint(loadInst);
    auto newLoadInst = builder.CreateLoad(
        dyn_cast<PointerType>(bitcastInst->getSrcTy())->getElementType(),
        bitcastInst->getOperand(0));
    for (auto lduser : loadInst->users()) {
      auto storeInst = dyn_cast_or_null<StoreInst>(lduser);
      auto bitcastInst2 =
          dyn_cast_or_null<BitCastInst>(storeInst->getPointerOperand());
      builder.SetInsertPoint(storeInst);
      auto newStoreInst =
          builder.CreateStore(newLoadInst, bitcastInst2->getOperand(0));
      instrToRemove.push_back(storeInst);
      instrToRemove.push_back(bitcastInst2);
    }
    instrToRemove.push_back(loadInst);
  }
  instrToRemove.push_back(bitcastInst);

  for (auto instr : instrToRemove)
    instr->eraseFromParent();
}

static void foldBitCast(AllocaInst *allocaInst) {
  std::vector<BitCastInst *> insts;
  for (auto user : allocaInst->users())
    if (auto bitcastInst = dyn_cast<BitCastInst>(user))
      if (canFold(bitcastInst))
        insts.push_back(bitcastInst);

  for (auto bitcastInst : insts)
    foldBitCast(bitcastInst);
}

namespace {
class FoldBitCastPass : public llvm::ModulePass {

public:
  static char ID;

  FoldBitCastPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool FoldBitCastPass::runOnModule(Module &M) {

  for (auto &F : M) {
    if (F.getName() == "main" || F.hasFnAttribute("dass_ss"))
      continue;

    std::vector<AllocaInst *> allocaInsts;
    for (auto &BB : F)
      for (auto &I : BB)
        if (auto a = dyn_cast<AllocaInst>(&I))
          if (a->getAllocatedType()->isFloatTy() ||
              a->getAllocatedType()->isDoubleTy())
            allocaInsts.push_back(a);

    for (auto a : allocaInsts)
      foldBitCast(a);
  }

  return true;
}

//--------------------------------------------------------//
// Pass declaration: FoldBitCastPass
//--------------------------------------------------------//

static void nameLoop(Loop *loop) {
  auto BB = loop->getLoopPreheader();
  if (BB) {
    SmallVector<Metadata *, 4> Args;
    std::string loopName = "DASS_LOOP_" + BB->getName().str();

    // Reserve operand 0 for loop id self reference.
    LLVMContext &Context = loop->getHeader()->getContext();
    auto TempNode = MDNode::getTemporary(Context, None);
    Args.push_back(TempNode.get());

    // Loop name
    Metadata *nameVals[] = {MDString::get(Context, "llvm.loop.name"),
                            MDString::get(Context, loopName)};
    Args.push_back(MDNode::get(Context, nameVals));

    // Set the first operand to itself.
    MDNode *LoopID = MDNode::get(Context, Args);
    LoopID->replaceOperandWith(0, LoopID);
    loop->setLoopID(LoopID);
  }

  auto subLoops = loop->getSubLoops();
  if (!subLoops.empty())
    for (auto subloop : subLoops)
      nameLoop(subloop);
}

namespace {
class NameLoopPass : public llvm::ModulePass {

public:
  static char ID;

  NameLoopPass() : llvm::ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool NameLoopPass::runOnModule(Module &M) {

  for (auto &F : M) {
    if (F.getName() == "main" || F.empty())
      continue;

    auto DT = llvm::DominatorTree(F);
    LoopInfo LI(DT);

    if (!LI.empty())
      for (auto loop : LI)
        nameLoop(loop);
  }

  return true;
}

char FoldBitCastPass::ID = 1;

static RegisterPass<FoldBitCastPass>
    X("fold-bitcast", "Fold bitcast for float pointers", false, false);

char NameLoopPass::ID = 2;

static RegisterPass<NameLoopPass> Y("name-loops", "Name loops", false, false);
