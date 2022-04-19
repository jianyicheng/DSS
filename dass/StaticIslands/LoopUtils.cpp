#include "StaticIslands.h"

using namespace llvm;

// Downgraded from LLVM 13, to be replaced when upgrading LLVM:
static const BasicBlock &skipEmptyBlockUntil(const BasicBlock *From,
                                             const BasicBlock *End,
                                             bool CheckUniquePred = false);

static bool isRotatedForm(Loop *loop) {
  BasicBlock *Latch = loop->getLoopLatch();
  return Latch && loop->isLoopExiting(Latch);
}

static BranchInst *getLoopGuardBranch(Loop *loop) {
  if (!loop->isLoopSimplifyForm())
    return nullptr;

  BasicBlock *Preheader = loop->getLoopPreheader();
  assert(Preheader && loop->getLoopLatch() &&
         "Expecting a loop with valid preheader and latch");

  // Loop should be in rotate form.
  if (!isRotatedForm(loop))
    return nullptr;

  // Disallow loops with more than one unique exit block, as we do not verify
  // that GuardOtherSucc post dominates all exit blocks.
  BasicBlock *ExitFromLatch = loop->getUniqueExitBlock();
  if (!ExitFromLatch)
    return nullptr;

  BasicBlock *GuardBB = Preheader->getUniquePredecessor();
  if (!GuardBB)
    return nullptr;

  assert(GuardBB->getTerminator() && "Expecting valid guard terminator");

  BranchInst *GuardBI = dyn_cast<BranchInst>(GuardBB->getTerminator());
  if (!GuardBI || GuardBI->isUnconditional())
    return nullptr;

  BasicBlock *GuardOtherSucc = (GuardBI->getSuccessor(0) == Preheader)
                                   ? GuardBI->getSuccessor(1)
                                   : GuardBI->getSuccessor(0);

  // Check if ExitFromLatch (or any BasicBlock which is an empty unique
  // successor of ExitFromLatch) is equal to GuardOtherSucc. If
  // skipEmptyBlockUntil returns GuardOtherSucc, then the guard branch for the
  // loop is GuardBI (return GuardBI), otherwise return nullptr.
  if (&skipEmptyBlockUntil(ExitFromLatch, GuardOtherSucc,
                           /*CheckUniquePred=*/true) == GuardOtherSucc)
    return GuardBI;
  else
    return nullptr;
}
static const BasicBlock &skipEmptyBlockUntil(const BasicBlock *From,
                                             const BasicBlock *End,
                                             bool CheckUniquePred) {
  assert(From && "Expecting valid From");
  assert(End && "Expecting valid End");

  if (From == End || !From->getUniqueSuccessor())
    return *From;

  auto IsEmpty = [](const BasicBlock *BB) {
    return (BB->getInstList().size() == 1);
  };

  // Visited is used to avoid running into an infinite loop.
  SmallPtrSet<const BasicBlock *, 4> Visited;
  const BasicBlock *BB = From->getUniqueSuccessor();
  const BasicBlock *PredBB = From;
  while (BB && BB != End && IsEmpty(BB) && !Visited.count(BB) &&
         (!CheckUniquePred || BB->getUniquePredecessor())) {
    Visited.insert(BB);
    PredBB = BB;
    BB = BB->getUniqueSuccessor();
  }

  return (BB == End) ? *End : *PredBB;
}
static bool checkLoopsStructure(Loop &OuterLoop, Loop &InnerLoop,
                                ScalarEvolution &SE) {
  // The inner loop must be the only outer loop's child.
  if ((OuterLoop.getSubLoops().size() != 1) ||
      (InnerLoop.getParentLoop() != &OuterLoop))
    return false;

  // We expect loops in normal form which have a preheader, header, latch...
  if (!OuterLoop.isLoopSimplifyForm() || !InnerLoop.isLoopSimplifyForm())
    return false;

  const BasicBlock *OuterLoopHeader = OuterLoop.getHeader();
  const BasicBlock *OuterLoopLatch = OuterLoop.getLoopLatch();
  const BasicBlock *InnerLoopPreHeader = InnerLoop.getLoopPreheader();
  const BasicBlock *InnerLoopLatch = InnerLoop.getLoopLatch();
  const BasicBlock *InnerLoopExit = InnerLoop.getExitBlock();

  // We expect rotated loops. The inner loop should have a single exit block.
  if (OuterLoop.getExitingBlock() != OuterLoopLatch ||
      InnerLoop.getExitingBlock() != InnerLoopLatch || !InnerLoopExit)
    return false;

  // Returns whether the block `ExitBlock` contains at least one LCSSA Phi node.
  auto ContainsLCSSAPhi = [](const BasicBlock &ExitBlock) {
    return any_of(ExitBlock.phis(), [](const PHINode &PN) {
      return PN.getNumIncomingValues() == 1;
    });
  };

  // Returns whether the block `BB` qualifies for being an extra Phi block. The
  // extra Phi block is the additional block inserted after the exit block of an
  // "guarded" inner loop which contains "only" Phi nodes corresponding to the
  // LCSSA Phi nodes in the exit block.
  auto IsExtraPhiBlock = [&](const BasicBlock &BB) {
    return BB.getFirstNonPHI() == BB.getTerminator() &&
           all_of(BB.phis(), [&](const PHINode &PN) {
             return all_of(PN.blocks(), [&](const BasicBlock *IncomingBlock) {
               return IncomingBlock == InnerLoopExit ||
                      IncomingBlock == OuterLoopHeader;
             });
           });
  };

  const BasicBlock *ExtraPhiBlock = nullptr;
  // Ensure the only branch that may exist between the loops is the inner loop
  // guard.
  if (OuterLoopHeader != InnerLoopPreHeader) {
    const BasicBlock &SingleSucc =
        skipEmptyBlockUntil(OuterLoopHeader, InnerLoopPreHeader);

    // no conditional branch present
    if (&SingleSucc != InnerLoopPreHeader) {
      const BranchInst *BI = dyn_cast<BranchInst>(SingleSucc.getTerminator());

      if (!BI || BI != getLoopGuardBranch(&InnerLoop))
        return false;

      bool InnerLoopExitContainsLCSSA = ContainsLCSSAPhi(*InnerLoopExit);

      // The successors of the inner loop guard should be the inner loop
      // preheader or the outer loop latch possibly through empty blocks.
      for (const BasicBlock *Succ : BI->successors()) {
        const BasicBlock *PotentialInnerPreHeader = Succ;
        const BasicBlock *PotentialOuterLatch = Succ;

        // Ensure the inner loop guard successor is empty before skipping
        // blocks.
        if (Succ->getInstList().size() == 1) {
          PotentialInnerPreHeader =
              &skipEmptyBlockUntil(Succ, InnerLoopPreHeader);
          PotentialOuterLatch = &skipEmptyBlockUntil(Succ, OuterLoopLatch);
        }

        if (PotentialInnerPreHeader == InnerLoopPreHeader)
          continue;
        if (PotentialOuterLatch == OuterLoopLatch)
          continue;

        // If `InnerLoopExit` contains LCSSA Phi instructions, additional block
        // may be inserted before the `OuterLoopLatch` to which `BI` jumps. The
        // loops are still considered perfectly nested if the extra block only
        // contains Phi instructions from InnerLoopExit and OuterLoopHeader.
        if (InnerLoopExitContainsLCSSA && IsExtraPhiBlock(*Succ) &&
            Succ->getSingleSuccessor() == OuterLoopLatch) {
          // Points to the extra block so that we can reference it later in the
          // final check. We can also conclude that the inner loop is
          // guarded and there exists LCSSA Phi node in the exit block later if
          // we see a non-null `ExtraPhiBlock`.
          ExtraPhiBlock = Succ;
          continue;
        }
        return false;
      }
    }
  }

  // Ensure the inner loop exit block lead to the outer loop latch possibly
  // through empty blocks.
  if ((!ExtraPhiBlock ||
       &skipEmptyBlockUntil(InnerLoop.getExitBlock(), ExtraPhiBlock) !=
           ExtraPhiBlock) &&
      (&skipEmptyBlockUntil(InnerLoop.getExitBlock(), OuterLoopLatch) !=
       OuterLoopLatch)) {
    return false;
  }

  return true;
}
bool arePerfectlyNested(Loop &OuterLoop, Loop &InnerLoop, ScalarEvolution &SE) {

  // Bail out if we cannot retrieve the outer loop bounds.
  auto indvar = getConstantBoundedIndVar(&OuterLoop, SE);
  if (!indvar)
    return false;

  // Determine whether the loops structure satisfies the following requirements:
  //  - the inner loop should be the outer loop's only child
  //  - the outer loop header should 'flow' into the inner loop preheader
  //    or jump around the inner loop to the outer loop latch
  //  - if the inner loop latch exits the inner loop, it should 'flow' into
  //    the outer loop latch.
  if (!checkLoopsStructure(OuterLoop, InnerLoop, SE)) {
    return false;
  }

  // Identify the outer loop latch comparison instruction.
  const BasicBlock *Latch = OuterLoop.getLoopLatch();

  assert(Latch && "Expecting a valid loop latch");
  const BranchInst *BI = dyn_cast<BranchInst>(Latch->getTerminator());
  assert(BI && BI->isConditional() &&
         "Expecting loop latch terminator to be a branch instruction");

  const CmpInst *OuterLoopLatchCmp = dyn_cast<CmpInst>(BI->getCondition());

  // Identify the inner loop guard instruction.
  BranchInst *InnerGuard = getLoopGuardBranch(&InnerLoop);
  const CmpInst *InnerLoopGuardCmp =
      (InnerGuard) ? dyn_cast<CmpInst>(InnerGuard->getCondition()) : nullptr;

  auto stepInst = dyn_cast<PHINode>(indvar)->getIncomingValueForBlock(Latch);

  // Determine whether instructions in a basic block are one of:
  //  - the inner loop guard comparison
  //  - the outer loop latch comparison
  //  - the outer loop induction variable increment
  //  - a phi node, a cast or a branch
  auto containsOnlySafeInstructions = [&](const BasicBlock &BB) {
    return llvm::all_of(BB, [&](const Instruction &I) {
      bool isAllowed = isSafeToSpeculativelyExecute(&I) || isa<PHINode>(I) ||
                       isa<BranchInst>(I);
      if (!isAllowed) {
        return false;
      }

      // The only binary instruction allowed is the outer loop step instruction,
      // the only comparison instructions allowed are the inner loop guard
      // compare instruction and the outer loop latch compare instruction.
      if ((isa<BinaryOperator>(I) && &I != stepInst) ||
          (isa<CmpInst>(I) && &I != OuterLoopLatchCmp &&
           &I != InnerLoopGuardCmp)) {
        return false;
      }
      return true;
    });
  };

  // Check the code surrounding the inner loop for instructions that are deemed
  // unsafe.
  const BasicBlock *OuterLoopHeader = OuterLoop.getHeader();
  const BasicBlock *OuterLoopLatch = OuterLoop.getLoopLatch();
  const BasicBlock *InnerLoopPreHeader = InnerLoop.getLoopPreheader();

  if (!containsOnlySafeInstructions(*OuterLoopHeader) ||
      !containsOnlySafeInstructions(*OuterLoopLatch) ||
      (InnerLoopPreHeader != OuterLoopHeader &&
       !containsOnlySafeInstructions(*InnerLoopPreHeader)) ||
      !containsOnlySafeInstructions(*InnerLoop.getExitBlock())) {
    return false;
  }

  return true;
}