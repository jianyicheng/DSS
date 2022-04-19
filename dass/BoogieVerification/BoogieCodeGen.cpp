#include "BoogieVerification.h"

static Loop *getInnermostLoop(Loop *loop, BasicBlock *BB) {
  auto subloops = loop->getSubLoops();
  if (subloops.empty())
    return loop;

  for (auto subloop : subloops)
    if (auto innermostLoop = getInnermostLoop(subloop, BB))
      return innermostLoop;
  return nullptr;
}

static std::string getLoopDASSName(Loop *loop) {
  auto id = loop->getLoopID();
  LLVMContext &Context = loop->getHeader()->getContext();
  for (unsigned int i = 0; i < id->getNumOperands(); i++)
    if (auto node = dyn_cast<MDNode>(id->getOperand(i))) {
      Metadata *arg = node->getOperand(0);
      if (arg == MDString::get(Context, "llvm.loop.name")) {
        Metadata *name = node->getOperand(1);
        MDString *nameAsString = dyn_cast<MDString>(name);
        return nameAsString->getString();
      }
    }
  return nullptr;
}

static std::string getVariableNameInBoogie(Value *I) {
  std::string instrResName;
  raw_string_ostream string_stream(instrResName);
  I->printAsOperand(string_stream, false);
  instrResName = std::regex_replace(string_stream.str(), std::regex("%"), "$");
  if (ConstantInt *constVar = dyn_cast<ConstantInt>(I)) {
    int bits = dyn_cast<IntegerType>(I->getType())->getBitWidth();
    if (constVar->isNegative() && bits > 1)
      return ("bv" + std::to_string(bits) + "neg(" + instrResName.substr(1) +
              "bv" + std::to_string(bits) + ")");
    else if (instrResName == "false")
      return ("0bv1");
    else if (instrResName == "true")
      return ("1bv1");
    else
      return (instrResName + "bv" + std::to_string(bits));
  } else if (isa<ConstantFP>(I)) {
    std::string type = "";
    if (I->getType()->isDoubleTy())
      type = "f53e11";
    else if (I->getType()->isFloatTy())
      type = "f24e8";

    if (instrResName.find("0x") != std::string::npos) {
      long long int num;
      double f;
      sscanf(instrResName.c_str(), "%llx", &num);
      f = *((double *)&num);
      instrResName = std::to_string(f) + "e0";
    } else if (instrResName.find("e") == std::string::npos)
      instrResName += "e0";
    else if (instrResName.find("+") != std::string::npos)
      instrResName.replace(instrResName.find("+"), 1,
                           "0"); // boogie does not support 1e+2
    instrResName = "0x" + instrResName + type;
  } else if (instrResName == "undef") {
    auto type = I->getType();
    if (type->isIntegerTy()) {
      int bits = dyn_cast<IntegerType>(type)->getBitWidth();
      instrResName += "_bv" + std::to_string(bits);
    } else if (type->isDoubleTy())
      instrResName += "_double";
    else if (type->isFloatTy())
      instrResName += "_float";
    else
      llvm_unreachable("Unknown type for undef.");
  }

  return instrResName;
}

static std::string getBlockLabel(BasicBlock *BB) {
  std::string block_address;
  raw_string_ostream string_stream(block_address);
  BB->printAsOperand(string_stream, false);

  std::string temp = string_stream.str();

  for (unsigned int i = 0; i < temp.length(); ++i) {
    if (temp[i] == '-')
      temp.replace(i, 1, "_");
  }
  return std::regex_replace(temp.c_str(), std::regex("%"), "$");
}

void BoogieCodeGenerator::generateBoogieInstruction(Instruction *I,
                                                    LoopInfo &LI,
                                                    bool analyzeMemory) {
  std::string callName;
  BoogieMemoryNode *mn;
  llvm::PHINode *loopiter;
  std::string ftype;
  int search;

  switch (I->getOpcode()) {
  case Instruction::Ret: // ret
    bpl << "\treturn;\n";
    break;

  case Instruction::Br: // br
    // do phi resolution here
    for (auto it = phis.begin(); it != phis.end(); ++it) {
      BoogiePhiNode *phiTfInst = *it;
      if (phiTfInst->bb == I->getParent())
        bpl << "\t" << phiTfInst->res
            << " := " << getVariableNameInBoogie(phiTfInst->ip) << ";\n";
    }
    // loop invariant at exit
    for (auto it = invariances.begin(); it != invariances.end(); ++it) {
      BoogieInvariance *in = *it;
      llvm::PHINode *inst = in->instr;
      for (unsigned int t = 0; t < inst->getNumIncomingValues(); ++t) {
        if (!isa<ConstantInt>(inst->getIncomingValue(t)) &&
            inst->getIncomingBlock(t) == I->getParent()) {
          bpl << "\tassert " << in->invar << ";\n";
        }
      }
    }
    if (I->getNumOperands() == 1) // if (inst->isConditional())?
      bpl << "\tgoto bb_" << getBlockLabel((BasicBlock *)(I->getOperand(0)))
          << ";\n";
    else if (I->getNumOperands() == 3)
      bpl << "\tif(" << getVariableNameInBoogie((Value *)I->getOperand(0))
          << " == 1bv1) {goto bb_"
          << getBlockLabel((BasicBlock *)(I->getOperand(2)))
          << ";} else {goto bb_"
          << getBlockLabel((BasicBlock *)(I->getOperand(1))) << ";}\n";
    else {
      errs() << *I << "\n";
      llvm_unreachable("Unsupported instruction translated to Boogie.");
    }
    break;
  case Instruction::Switch:
    if (SwitchInst *sw = dyn_cast<SwitchInst>(I)) {
      Value *cond = sw->getCondition();
      bpl << "\t";
      for (auto i = sw->case_begin(); i < sw->case_end(); i++)
        bpl << "if(" << getVariableNameInBoogie(cond)
            << " == " << getVariableNameInBoogie(i->getCaseValue())
            << "){goto bb_" << getBlockLabel(i->getCaseSuccessor())
            << ";}\n\telse ";
      bpl << "{goto bb_" << getBlockLabel(sw->getDefaultDest()) << ";}\n";
    }
    break;

  case Instruction::Add: // add
    if (OverflowingBinaryOperator *op =
            dyn_cast<OverflowingBinaryOperator>(I)) {
      if ((op->hasNoUnsignedWrap()) && (op->hasNoSignedWrap()))
        // has both nuw and nsw
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
            << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "add("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ");\n"; // ---might has problems...
      else if (op->hasNoUnsignedWrap())
        // only nuw
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
            << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "add("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ");\n"; // ---might has problems...
      else if (op->hasNoSignedWrap())
        // only nsw
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
            << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "add("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ");\n"; // ---might has problems...
      else
        // normal add
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
            << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "add("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    } else {
      errs() << *I << "\n";
      llvm_unreachable("Unsupported instruction translated to Boogie.");
    }
    break;

  case Instruction::FAdd: // fadd
    if (I->getType()->isDoubleTy())
      bpl << "\t" << getVariableNameInBoogie(&*I) << " := dadd(boogie_fp_mode, "
          << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
          << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    else
      bpl << "\t" << getVariableNameInBoogie(&*I) << " := fadd(boogie_fp_mode, "
          << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
          << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    break;

  case Instruction::Xor: // xor
    bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
        << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "xor("
        << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
        << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    break;

  case Instruction::FMul: // fmul
    if (I->getType()->isDoubleTy())
      bpl << "\t" << getVariableNameInBoogie(&*I) << " := dmul(boogie_fp_mode, "
          << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
          << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    else
      bpl << "\t" << getVariableNameInBoogie(&*I) << " := fmul(boogie_fp_mode, "
          << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
          << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    break;

  case Instruction::Sub: // sub
    if (OverflowingBinaryOperator *op =
            dyn_cast<OverflowingBinaryOperator>(I)) {
      if ((op->hasNoUnsignedWrap()) && (op->hasNoSignedWrap()))
        // has both nuw and nsw
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
            << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "sub("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ");\n"; // ---might has problems...
      else if (op->hasNoUnsignedWrap())
        // only nuw
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
            << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "sub("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ");\n"; // ---might has problems...
      else if (op->hasNoSignedWrap())
        // only nsw
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
            << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "sub("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ");\n"; // ---might has problems...
      else
        // normal add
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
            << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "sub("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    } else {
      errs() << *I << "\n";
      llvm_unreachable("Unsupported instruction translated to Boogie.");
    }
    break;

  case Instruction::BitCast:
    if (I->getType()->isIntegerTy()) {
      if (I->getOperand(0)->getType()->isDoubleTy())
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := double2ubv"
            << dyn_cast<IntegerType>(I->getType())->getBitWidth()
            << "(boogie_fp_mode, "
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ");\n";
      else if (I->getOperand(0)->getType()->isFloatTy())
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := float2ubv"
            << dyn_cast<IntegerType>(I->getType())->getBitWidth()
            << "(boogie_fp_mode, "
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ");\n";
      else
        bpl << "\t" << getVariableNameInBoogie(&*I)
            << " := " << getVariableNameInBoogie((Value *)I->getOperand(0))
            << ";\n";
    } else if (I->getOperand(0)->getType()->isIntegerTy()) {
      if (I->getType()->isDoubleTy())
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
            << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
            << "double(boogie_fp_mode, "
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ");\n";
      else if (I->getType()->isFloatTy())
        bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
            << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
            << "float(boogie_fp_mode, "
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ");\n";
      else
        bpl << "\t" << getVariableNameInBoogie(&*I)
            << " := " << getVariableNameInBoogie((Value *)I->getOperand(0))
            << ";\n";
    } else
      bpl << "\t" << getVariableNameInBoogie(&*I)
          << " := " << getVariableNameInBoogie((Value *)I->getOperand(0))
          << ";\n";

    break;

  case Instruction::Mul: // mul
    bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
        << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "mul("
        << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
        << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    break;
  case Instruction::Shl: // shl
    bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
        << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "shl("
        << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
        << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    break;
  case Instruction::LShr: // lshr
    bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
        << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "lshr("
        << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
        << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    break;

  case Instruction::AShr: // ashr
    bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
        << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "ashr("
        << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
        << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    break;
  case Instruction::And: // and
    bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
        << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "and("
        << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
        << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    break;
  case Instruction::Or: // or
    bpl << "\t" << getVariableNameInBoogie(&*I) << " := bv"
        << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "or("
        << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
        << getVariableNameInBoogie((Value *)I->getOperand(1)) << ");\n";
    break;

  case Instruction::Load: // load
    bpl << "//\t" << getVariableNameInBoogie(&*I) << " := "
        << getVariableNameInBoogie(
               (Value *)dyn_cast<Instruction>(I->getOperand(0))->getOperand(0))
        << "["
        << getVariableNameInBoogie(
               (Value *)dyn_cast<Instruction>(I->getOperand(0))->getOperand(1))
        << "];\n";
    bpl << "\thavoc " << getVariableNameInBoogie(&*I) << ";\n";
    if (analyzeMemory) {
      mn = new BoogieMemoryNode;
      mn->load = true;
      mn->instr = &*I;
      mn->label = accesses.size();
      accesses.push_back(mn);

      search = -1;
      for (auto k = arrays.begin(); k != arrays.end(); k++) {
        Value *an = *k;
        if ((Value *)(dyn_cast<Instruction>(I->getOperand(0))->getOperand(0)) ==
            an)
          search = k - arrays.begin();
      }
      if (search == -1) {
        search = arrays.size();
        arrays.push_back(
            (Value *)(dyn_cast<Instruction>(I->getOperand(0))->getOperand(0)));
      }
      bpl << "\tif(*){\n\t\tstmt:= " << mn->label << "bv" << BW
          << ";\n\t\taddress := "
          << getVariableNameInBoogie(
                 (Value *)dyn_cast<Instruction>(I->getOperand(0))
                     ->getOperand(1))
          << ";\n";

      auto currLoop = LI.getLoopFor(I->getParent());
      auto depth = currLoop->getLoopDepth();
      for (int j = 0; j < loopDepth - depth; j++)
        bpl << "\t\titeration_" << j << " := 0bv32;\n";
      auto offset = loopDepth - depth;
      for (int i = 0; i < depth; i++) {
        bpl << "\t\titeration_" << i + offset << " := ";
        loopiter = nullptr;
        for (auto k = invariances.begin(); k != invariances.end(); k++) {
          BoogieInvariance *in = *k;
          if (in->loop == currLoop)
            loopiter = in->instr;
        }
        assert(loopiter);
        bpl << getVariableNameInBoogie(loopiter) << ";\n";
        currLoop = currLoop->getParentLoop();
      }
      bpl << "\t\tis_load := true;\n\t\tvalid := true;\n\t\tarray := " << search
          << "bv" << BW << ";\n\t\treturn;\n\t}\n";
    }
    break;

  case Instruction::Store: // store
    bpl << "//\t"
        << getVariableNameInBoogie(
               (Value *)dyn_cast<Instruction>(I->getOperand(1))->getOperand(0))
        << "["
        << getVariableNameInBoogie(
               (Value *)dyn_cast<Instruction>(I->getOperand(1))->getOperand(1))
        << "] := " << getVariableNameInBoogie((Value *)I->getOperand(0))
        << ";\n";
    if (analyzeMemory) {
      mn = new BoogieMemoryNode;
      mn->load = false;
      mn->instr = &*I;
      mn->label = accesses.size();
      accesses.push_back(mn);

      search = -1;
      for (auto k = arrays.begin(); k != arrays.end(); k++) {
        Value *an = *k;
        if ((Value *)(dyn_cast<Instruction>(I->getOperand(1))->getOperand(0)) ==
            an)
          search = k - arrays.begin();
      }
      if (search == -1) {
        search = arrays.size();
        arrays.push_back(
            (Value *)dyn_cast<Instruction>(I->getOperand(1))->getOperand(0));
      }
      bpl << "\tif(*){\n\t\tstmt:= " << mn->label << "bv" << BW
          << ";\n\t\taddress := "
          << getVariableNameInBoogie(
                 (Value *)dyn_cast<Instruction>(I->getOperand(1))
                     ->getOperand(1))
          << ";\n";
      auto currLoop = LI.getLoopFor(I->getParent());
      auto depth = currLoop->getLoopDepth();
      for (int j = 0; j < loopDepth - depth; j++)
        bpl << "\t\titeration_" << j << " := 0bv32;\n";
      auto offset = loopDepth - depth;
      for (int i = 0; i < depth; i++) {
        bpl << "\t\titeration_" << i + offset << " := ";
        loopiter = nullptr;
        for (auto k = invariances.begin(); k != invariances.end(); k++) {
          BoogieInvariance *in = *k;
          if (in->loop == currLoop)
            loopiter = in->instr;
        }
        bpl << getVariableNameInBoogie(loopiter) << ";\n";
        currLoop = currLoop->getParentLoop();
      }
      bpl << "\t\tis_load := false;\n\t\tvalid := true;\n\t\tarray := "
          << search << "bv" << BW << ";\n\t\treturn;\n\t}\n";
    }
    break;
  case Instruction::GetElementPtr: // getelementptr
                                   // this can be ignored
    break;

  case Instruction::Trunc: // trunc
    assert(isa<IntegerType>(I->getType()));
    assert(isa<IntegerType>(I->getOperand(0)->getType()));
    bpl << "\t" << getVariableNameInBoogie(&*I)
        << " := " << getVariableNameInBoogie((Value *)I->getOperand(0)) << "["
        << dyn_cast<IntegerType>(I->getType())->getBitWidth() << ":0];\n";
    break;

  case Instruction::ZExt: // zext
    bpl << "\t" << getVariableNameInBoogie(&*I) << " := zext.bv"
        << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
        << ".bv" << dyn_cast<IntegerType>(I->getType())->getBitWidth() << "("
        << getVariableNameInBoogie((Value *)I->getOperand(0)) << ");\n";
    break;

  case Instruction::SExt: // sext
    errs() << "Undefined SEXT instruction " << *I << "\n";
    break;

  case Instruction::ICmp: // icmp
    if (CmpInst *cmpInst = dyn_cast<CmpInst>(&*I)) {
      if (cmpInst->getPredicate() == CmpInst::ICMP_EQ) {
        bpl << "\tif (" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << " == " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") { " << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I) << " := 0bv1; }\n";
      } else if (cmpInst->getPredicate() == CmpInst::ICMP_NE) {
        bpl << "\tif (" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << " != " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") { " << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I) << " := 0bv1; }\n";
      } else if (cmpInst->getPredicate() == CmpInst::ICMP_UGT) {
        bpl << "\tif (bv"
            << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
            << "ugt(" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << ", " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") == true) { " << getVariableNameInBoogie(&*I)
            << " := 1bv1; } else { " << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n"; // ---might has problems...
      } else if (cmpInst->getPredicate() == CmpInst::ICMP_UGE) {
        bpl << "\tif (bv"
            << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
            << "uge(" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << ", " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") == true) { " << getVariableNameInBoogie(&*I)
            << " := 1bv1; } else { " << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n"; // ---might has problems...
      } else if (cmpInst->getPredicate() == CmpInst::ICMP_ULT) {
        bpl << "\tif (bv"
            << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
            << "ult(" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << ", " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") == true) { " << getVariableNameInBoogie(&*I)
            << " := 1bv1; } else { " << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n"; // ---might has problems...
      } else if (cmpInst->getPredicate() == CmpInst::ICMP_ULE) {
        bpl << "\tif (bv"
            << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
            << "ule(" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << ", " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") == true) { " << getVariableNameInBoogie(&*I)
            << " := 1bv1; } else { " << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n"; // ---might has problems...
      } else if (cmpInst->getPredicate() == CmpInst::ICMP_SGT) {
        bpl << "\tif (bv"
            << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
            << "sgt(" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << ", " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") == true) { " << getVariableNameInBoogie(&*I)
            << " := 1bv1; } else { " << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n";
      } else if (cmpInst->getPredicate() == CmpInst::ICMP_SGE) {
        bpl << "\tif (bv"
            << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
            << "sge(" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << ", " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") == true) { " << getVariableNameInBoogie(&*I)
            << " := 1bv1; } else { " << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n";
      } else if (cmpInst->getPredicate() == CmpInst::ICMP_SLT) {
        bpl << "\tif (bv"
            << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
            << "slt(" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << ", " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") == true) { " << getVariableNameInBoogie(&*I)
            << " := 1bv1; } else { " << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n";
      } else if (cmpInst->getPredicate() == CmpInst::ICMP_SLE) {
        bpl << "\tif (bv"
            << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
            << "sle(" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << ", " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") == true) { " << getVariableNameInBoogie(&*I)
            << " := 1bv1; } else { " << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n";
      } else {
        errs() << *I << "\n";
        llvm_unreachable("Unsupported instruction translated to Boogie.");
      }
    }
    break;
  case Instruction::FCmp: // fcmp
    ftype = (I->getOperand(0)->getType()->isDoubleTy()) ? "d" : "f";
    if (FCmpInst *cmpInst = dyn_cast<FCmpInst>(&*I)) {
      if (cmpInst->getPredicate() == FCmpInst::FCMP_OEQ) {
        bpl << "\tif (" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << " == " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") { " << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I) << " := 0bv1; }\n";
      } else if (cmpInst->getPredicate() == FCmpInst::FCMP_ONE) {
        bpl << "\tif (" << getVariableNameInBoogie((Value *)I->getOperand(0))
            << " != " << getVariableNameInBoogie((Value *)I->getOperand(1))
            << ") { " << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I) << " := 0bv1; }\n";
      } else if (cmpInst->getPredicate() == FCmpInst::FCMP_UGT) {
        bpl << "\tif (" << ftype << "gt("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1)) << ")) { "
            << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n"; // ---might has problems...
      } else if (cmpInst->getPredicate() == FCmpInst::FCMP_UGE) {
        bpl << "\tif (" << ftype << "ge("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1)) << ")) { "
            << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n"; // ---might has problems...
      } else if (cmpInst->getPredicate() == FCmpInst::FCMP_ULT) {
        bpl << "\tif (" << ftype << "lt("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1)) << ")) { "
            << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n"; // ---might has problems...
      } else if (cmpInst->getPredicate() == FCmpInst::FCMP_ULE) {
        bpl << "\tif (" << ftype << "le("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1)) << ")) { "
            << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I)
            << " := 0bv1; }\n"; // ---might has problems...
      } else if (cmpInst->getPredicate() == FCmpInst::FCMP_OGT) {
        bpl << "\tif (" << ftype << "gt("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1)) << ")) { "
            << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I) << " := 0bv1; }\n";
      } else if (cmpInst->getPredicate() == FCmpInst::FCMP_OGE) {
        bpl << "\tif (" << ftype << "ge("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1)) << ")) { "
            << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I) << " := 0bv1; }\n";
      } else if (cmpInst->getPredicate() == FCmpInst::FCMP_OLT) {
        bpl << "\tif (" << ftype << "lt("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1)) << ")) { "
            << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I) << " := 0bv1; }\n";
      } else if (cmpInst->getPredicate() == FCmpInst::FCMP_OLE) {
        bpl << "\tif (" << ftype << "le("
            << getVariableNameInBoogie((Value *)I->getOperand(0)) << ", "
            << getVariableNameInBoogie((Value *)I->getOperand(1)) << ")) { "
            << getVariableNameInBoogie(&*I) << " := 1bv1; } else { "
            << getVariableNameInBoogie(&*I) << " := 0bv1; }\n";
      } else {
        errs() << *I << "\n";
        llvm_unreachable("Unsupported instruction translated to Boogie.");
      }
    }
    break;

  case Instruction::PHI: // phi
    // Has been done in previous section
    break;
  case Instruction::Call: // call
    errs() << *I << "\n";
    llvm_unreachable("Unsupported instruction translated to Boogie.");
    break;
  case Instruction::Select: // select
    bpl << "\tif (" << getVariableNameInBoogie((Value *)I->getOperand(0))
        << " == 1bv"
        << dyn_cast<IntegerType>(I->getOperand(0)->getType())->getBitWidth()
        << ") { " << getVariableNameInBoogie(&*I)
        << " := " << getVariableNameInBoogie((Value *)I->getOperand(1))
        << "; } else { " << getVariableNameInBoogie(&*I)
        << " := " << getVariableNameInBoogie((Value *)I->getOperand(2))
        << "; }\n";
    break;
  default:
    errs() << *I << "\n";
    llvm_unreachable("Unsupported instruction translated to Boogie.");
  }
}

static BoogieInvariance *castToLoopInvariance(Instruction *I, Loop *loop) {

  int isStepPositive = -1;
  int step = 0;
  int isInclusive = -1;
  int isLeftCmp = -1;
  std::string startSign, endSign, startBound, endBound;

  if (!isa<llvm::PHINode>(I) || !isa<IntegerType>(I->getType()))
    return nullptr;

  auto phiInst = dyn_cast<llvm::PHINode>(I);
  if (phiInst->getNumOperands() != 2)
    return nullptr;

  Instruction *stepInst;
  for (auto i = 0; i < 2; i++)
    if (!loop->contains(phiInst->getIncomingBlock(i))) {
      if (!isa<ConstantInt>(phiInst->getIncomingValue(i))) {
        llvm::errs() << *(phiInst->getIncomingValue(i)) << "\nreturned\n";
        return nullptr;
      } else
        startBound = getVariableNameInBoogie(phiInst->getIncomingValue(i));
    } else {
      stepInst = dyn_cast<Instruction>(phiInst->getIncomingValue(i));
      if (stepInst->getOpcode() == Instruction::Add)
        isStepPositive = 1;
      else if (stepInst->getOpcode() == Instruction::Sub)
        isStepPositive = 0;
      else
        return nullptr;
      if (auto stepValue = dyn_cast<ConstantInt>(stepInst->getOperand(0)))
        step = stepValue->getValue().getSExtValue();
      else if (auto stepValue = dyn_cast<ConstantInt>(stepInst->getOperand(1)))
        step = stepValue->getValue().getSExtValue();
      else
        return nullptr;
    }

  auto exitBB = loop->getExitingBlock();
  CmpInst *exit;
  if (auto branchInst = dyn_cast_or_null<BranchInst>(exitBB->getTerminator()))
    if (branchInst->isConditional())
      exit = dyn_cast<CmpInst>(branchInst->getCondition());
  if (!exit)
    return nullptr;

  if (isa<ConstantInt>(exit->getOperand(0)) && exit->getOperand(1) == I) {
    isLeftCmp = 0;
    endBound = getVariableNameInBoogie(exit->getOperand(0));
  } else if (isa<ConstantInt>(exit->getOperand(1)) &&
             exit->getOperand(0) == I) {
    isLeftCmp = 1;
    endBound = getVariableNameInBoogie(exit->getOperand(1));
  } else
    return nullptr;

  if (exit->getPredicate() == CmpInst::ICMP_EQ ||
      exit->getPredicate() == CmpInst::ICMP_NE ||
      exit->getPredicate() == CmpInst::ICMP_UGT ||
      exit->getPredicate() == CmpInst::ICMP_ULT ||
      exit->getPredicate() == CmpInst::ICMP_SGT ||
      exit->getPredicate() == CmpInst::ICMP_SLT)
    isInclusive = 0;
  else if (exit->getPredicate() == CmpInst::ICMP_UGE ||
           exit->getPredicate() == CmpInst::ICMP_ULE ||
           exit->getPredicate() == CmpInst::ICMP_SGE ||
           exit->getPredicate() == CmpInst::ICMP_SLE)
    isInclusive = 1;

  assert(isStepPositive != -1 && isInclusive != -1);

  std::string bits =
      std::to_string(dyn_cast<IntegerType>(I->getType())->getBitWidth());

  if (isStepPositive) {
    startSign = "bv" + bits + "uge(";
    if (isInclusive)
      endSign = "bv" + bits + "ule(";
    else
      endSign = "bv" + bits + "ult(";
    endBound = "bv" + bits + "add(" + endBound + ", " + std::to_string(step) +
               "bv" + bits + ")";
  } else {
    startSign = "bv" + bits + "ule(";
    if (isInclusive)
      endSign = "bv" + bits + "uge(";
    else
      endSign = "bv" + bits + "ugt(";
    endBound = "bv" + bits + "sub(" + endBound + ", " + std::to_string(step) +
               "bv" + bits + ")";
  }

  BoogieInvariance *invar = new BoogieInvariance;
  invar->loop = loop;
  invar->instr = dyn_cast<llvm::PHINode>(I);
  invar->invar = startSign + getVariableNameInBoogie(&*I) + "," + startBound +
                 ") && " + endSign + getVariableNameInBoogie(&*I) + ", " +
                 endBound + ")";
  return invar;
}

void BoogieCodeGenerator::generateFunctionBody(bool isLoopAnalysis) {
  auto DT = llvm::DominatorTree(*F);
  LoopInfo LI(DT);

  int indexCounter = 0;
  for (auto BB = F->begin(); BB != F->end(); ++BB) {
    bpl << "\n\t// For basic block: bb_" << getBlockLabel(&*BB) << "\n";
    bpl << "\tbb_" << getBlockLabel(&*BB) << ":\n";
    // Here add assertion of loop invariant conditions at start of the loop
    if (LI.isLoopHeader(&*BB)) {
      auto loop = LI.getLoopFor(&*BB);
      for (auto I = BB->begin(); I != BB->end(); I++)
        if (BoogieInvariance *invar = castToLoopInvariance(&*I, loop)) {
          invariances.push_back(invar);
          bpl << "\tassert " << invar->invar << ";\n";
          bpl << "\thavoc " << getVariableNameInBoogie(&*I) << ";\n";
          bpl << "\tassume " << invar->invar << ";\n";
        }
    } // end of insert loop invariants in the beginning

    // Start instruction printing
    for (auto I = BB->begin(); I != BB->end(); ++I)
      if (isLoopAnalysis)
        generateBoogieInstruction(
            &*I, LI, std::find(memoryAnalysisRegions.begin(),
                               memoryAnalysisRegions.end(),
                               &*BB) != memoryAnalysisRegions.end());
      else
        generateBoogieInstruction(&*I, LI);
  }
  bpl << "\n}\n";
}

static Loop *getLoopByName(Loop *loop, std::string name) {
  auto nameOp =
      MDString::get(loop->getHeader()->getContext(), "llvm.loop.name");
  auto id = loop->getLoopID();
  for (auto i = 0; i < id->getNumOperands(); i++)
    if (auto node = dyn_cast<MDNode>(id->getOperand(i))) {
      Metadata *arg = node->getOperand(0);
      if (arg == nameOp) {
        Metadata *loopName = node->getOperand(1);
        auto loopNameAsString = dyn_cast<MDString>(loopName);
        if (loopNameAsString->getString() == name)
          return loop;
      }
    }

  auto subloops = loop->getSubLoops();
  if (subloops.empty())
    return nullptr;

  for (auto subloop : subloops)
    if (auto targetLoop = getLoopByName(subloop, name))
      return targetLoop;
  return nullptr;
}

void BoogieCodeGenerator::analyzeLoop(std::string loopName) {
  auto DT = llvm::DominatorTree(*F);
  LoopInfo LI(DT);
  if (!LI.empty())
    for (auto loop : LI) {
      targetLoop = getLoopByName(loop, loopName);
      if (targetLoop)
        break;
    }
  if (!targetLoop)
    llvm_unreachable(
        std::string("Cannot find loop " + loopName + ".\n ").c_str());

  loopDepth = targetLoop->getLoopDepth();
  memoryAnalysisRegions.clear();
  for (auto BB = F->begin(); BB != F->end(); BB++)
    if (targetLoop->getParentLoop()->contains(&*BB))
      memoryAnalysisRegions.push_back(&*BB);
}

static void declareVariable(Value *var, std::fstream &bpl) {
  if (var->getType()->isIntegerTy()) {
    if (var->getType()->isPointerTy())
      bpl << "\tvar " << getVariableNameInBoogie(var)
          << ": [bv64]bv32;\n"; // todo: getElementType()
    else
      bpl << "\tvar " << getVariableNameInBoogie(var) << " : bv"
          << dyn_cast<IntegerType>(var->getType())->getBitWidth() << ";\n";
  } else if (var->getType()->isPointerTy())
    bpl << "\tvar " << getVariableNameInBoogie(var) << " : bv"
        << dyn_cast<IntegerType>(
               dyn_cast<PointerType>(var->getType())->getElementType())
               ->getBitWidth()
        << ";\n";
  else if (var->getType()->isDoubleTy())
    bpl << "\tvar " << getVariableNameInBoogie(var) << ": float53e11;\n";
  else if (var->getType()->isFloatTy())
    bpl << "\tvar " << getVariableNameInBoogie(var) << ": float24e8;\n";
}

void BoogieCodeGenerator::generateVariableDeclarations() {
  std::vector<Value *> vars;
  for (auto BB = F->begin(); BB != F->end(); BB++)
    for (auto I = BB->begin(); I != BB->end(); I++) {
      if (isa<GetElementPtrInst>(I))
        continue;
      if (auto result = dyn_cast<Value>(I))
        declareVariable(result, bpl);
    }
  bpl << "\tvar undef_bv32 : bv32;\n"
      << "\tvar undef_bv1 : bv1;\n"
      << " \tvar undef_float : float24e8;\n"
      << " \tvar undef_double : float53e11;\n"
      << "\tvar boogie_fp_mode : rmode;\n"
      << " \tboogie_fp_mode := RNE;\n"
      << " \tvalid := false;\n\n";
}

void BoogieCodeGenerator::generateFuncPrototype(bool isLoopAnalysis) {
  bpl << "\n// For function: " << static_cast<std::string>((F->getName()));
  bpl << "\nprocedure {:inline 1} " << static_cast<std::string>((F->getName()))
      << "(";
  if (!(F->arg_empty())) {
    for (auto fi = F->arg_begin(); fi != F->arg_end(); ++fi) {
      if (fi->getType()->isPointerTy()) {
        if (isa<ArrayType>(
                dyn_cast<PointerType>(fi->getType())->getElementType()))
          bpl << getVariableNameInBoogie(fi) << ": [bv" << BW << "]bv"
              << dyn_cast<IntegerType>(
                     dyn_cast<ArrayType>(
                         dyn_cast<PointerType>(fi->getType())->getElementType())
                         ->getArrayElementType())
                     ->getBitWidth();
        else
          bpl << getVariableNameInBoogie(fi) << ": [bv" << BW << "]real";
      } else {
        if (fi->getType()->isIntegerTy())
          bpl << getVariableNameInBoogie(fi) << ": bv"
              << dyn_cast<IntegerType>(fi->getType())->getBitWidth();
        else if (fi->getType()->isDoubleTy())
          bpl << getVariableNameInBoogie(fi) << ": float53e11";
        else
          bpl << getVariableNameInBoogie(fi) << ": float24e8";
      }
      auto fi_comma = fi;
      fi_comma++;
      if (fi_comma != F->arg_end())
        bpl << ", ";
    }
  }

  bpl << ") returns ("
      << "stmt: bv" << BW << ", " // statement id
      << "address: bv64, ";       // index

  if (isLoopAnalysis)
    for (auto i = 0; i < loopDepth; i++)
      bpl << "iteration_" << i << ": bv" << BW << ", "; // iteration

  bpl << "is_load: bool, "       // load/store
      << "valid: bool, "         // valid
      << "array: bv" << BW << "" // array id
      << ") \n";
  // JC: add global arrays here
  bpl << "{\n";
}

void BoogieCodeGenerator::phiAnalysis() {
  for (auto BB = F->begin(); BB != F->end(); ++BB)
    for (auto I = BB->begin(); I != BB->end(); ++I)
      if (llvm::PHINode *phiInst = dyn_cast<llvm::PHINode>(&*I)) {
        BoogiePhiNode *phi = new BoogiePhiNode[phiInst->getNumIncomingValues()];
        for (unsigned int it = 0; it < phiInst->getNumIncomingValues(); ++it) {
          phi[it].res = getVariableNameInBoogie(&*I);
          phi[it].bb = phiInst->getIncomingBlock(it);
          phi[it].ip = phiInst->getIncomingValue(it);
          phi[it].instr = phiInst;
          phis.push_back(&phi[it]);
        }
      }
}

void BoogieCodeGenerator::sliceFunction() {
  if (F->empty())
    return;

  auto DT = llvm::DominatorTree(*F);
  LoopInfo LI(DT);

  std::vector<Instruction *> instrToKeep, instrToRemove;

  // Add slicing criteria: Loop structs, memory insts and control insts
  for (auto BB = F->begin(); BB != F->end(); ++BB)
    for (auto I = BB->begin(); I != BB->end(); ++I)
      if (LI.isLoopHeader(&*BB) || isa<LoadInst>(I) || isa<StoreInst>(I) ||
          isa<BranchInst>(I) || isa<SwitchInst>(I))
        instrToKeep.push_back(&*I);

  // Add the dependent operations
  auto bound = instrToKeep.size();
  for (int iter = 0; iter < bound; iter++) {
    Instruction *instr = instrToKeep[iter];
    for (int i = 0; i < instr->getNumOperands(); i++) {
      // Ignore the stored value as we only analyze the memory addresses
      if (isa<StoreInst>(instr) && i == 0)
        continue;

      // Ignore all the fp operations as it is not related to memory addresses
      // (assuming there is no cast from fp to int)
      if (Instruction *ii = dyn_cast<Instruction>(instr->getOperand(i))) {
        bool skip = false;
        for (int j = 0; j < ii->getNumOperands(); j++) {
          if (ii->getOperand(j)->getType()->isFloatTy() ||
              ii->getOperand(j)->getType()->isDoubleTy())
            skip = true;
        }
        if (std::find(instrToKeep.begin(), instrToKeep.end(), ii) ==
                instrToKeep.end() &&
            !skip)
          instrToKeep.push_back(ii);
      }
    }
    bound = instrToKeep.size();
  }

  for (auto BB = F->begin(); BB != F->end(); ++BB)
    for (auto I = BB->begin(); I != BB->end(); ++I) {
      Instruction *instr = &*I;
      if (std::find(instrToKeep.begin(), instrToKeep.end(), instr) ==
              instrToKeep.end() &&
          !isa<TerminatorInst>(instr))
        instrToRemove.push_back(instr);
    }

  for (auto &r : instrToRemove) {
    // errs() << "Removing " << *r << "\n";
    r->replaceAllUsesWith(UndefValue::get(dyn_cast<Value>(r)->getType()));
    r->eraseFromParent();
  }
}

void BoogieCodeGenerator::generateBoogieHeader() {
  bpl << "\n//*********************************************\n"
      << "//    Boogie code generated from LLVM\n"
      << "//*********************************************\n"
      << "// Float function prototypes\n"
      << "function {:bvbuiltin \"fp.add\"} fadd(rmode, float24e8, float24e8) "
         "returns(float24e8);\n"
      << "function {:bvbuiltin \"fp.sub\"} fsub(rmode, float24e8, float24e8) "
         "returns(float24e8);\n"
      << "function {:bvbuiltin \"fp.mul\"} fmul(rmode, float24e8, float24e8) "
         "returns(float24e8);\n"
      << "function {:bvbuiltin \"fp.div\"} fdiv(rmode, float24e8, float24e8) "
         "returns(float24e8);\n"
      << "function {:bvbuiltin \"fp.rem\"} frem(rmode, float24e8, float24e8) "
         "returns(float24e8);\n"
      << "function {:bvbuiltin \"fp.sqrt\"} fsqrt(rmode, "
         "float24e8, float24e8) "
         "returns(float24e8);\n"
      << "function {:bvbuiltin \"fp.leq\"} fleq(float24e8, float24e8) "
         "returns(bool);\n"
      << "function {:bvbuiltin \"fp.geq\"} fgeq(float24e8, float24e8) "
         "returns(bool);\n"
      << "function {:bvbuiltin \"fp.gt\"} fgt(float24e8, float24e8) "
         "returns(bool);\n"
      << "function {:bvbuiltin \"fp.lt\"} flt(float24e8, float24e8) "
         "returns(bool);\n"
      << "function {:bvbuiltin \"fp.eq\"} feq(float24e8, float24e8) "
         "returns(bool);\n"
      << "function {:bvbuiltin \"fp.abs\"} fabs(float24e8) "
         "returns(float24e8);\n"
      << "function {:bvbuiltin \"fp.neg\"} fneg(float24e8) "
         "returns(float24e8);\n"
      << "function {:bvbuiltin \"(_ to_fp 8 24)\"} to_float(real) "
         "returns(float24e8);\n";

  bpl << "// Double function prototypes\n"
      << "function {:bvbuiltin \"fp.add\"} dadd(rmode, "
         "float53e11,float53e11) "
         "returns(float53e11);\n"
      << "function {:bvbuiltin \"fp.sub\"} dsub(rmode, "
         "float53e11,float53e11) "
         "returns(float53e11);\n"
      << "function {:bvbuiltin \"fp.mul\"} dmul(rmode, "
         "float53e11,float53e11) "
         "returns(float53e11);\n"
      << "function {:bvbuiltin \"fp.div\"} ddiv(rmode, "
         "float53e11,float53e11) "
         "returns(float53e11);\n"
      << "function {:bvbuiltin \"fp.rem\"} drem(rmode, "
         "float53e11,float53e11) "
         "returns(float53e11);\n"
      << "function {:bvbuiltin \"fp.sqrt\"} dsqrt(rmode, "
         "float53e11,float53e11) returns(float53e11);\n"
      << "function {:bvbuiltin \"fp.leq\"} dleq(float53e11,float53e11) "
         "returns(bool);\n"
      << "function {:bvbuiltin \"fp.geq\"} dgeq(float53e11,float53e11) "
         "returns(bool);\n"
      << "function {:bvbuiltin \"fp.gt\"} dgt(float53e11,float53e11) "
         "returns(bool);\n"
      << "function {:bvbuiltin \"fp.lt\"} dlt(float53e11,float53e11) "
         "returns(bool);\n"
      << "function {:bvbuiltin \"fp.eq\"} deq(float53e11,float53e11) "
         "returns(bool);\n"
      << "function {:bvbuiltin \"fp.abs\"} dabs(float53e11) "
         "returns(float53e11);\n"
      << "function {:bvbuiltin \"fp.neg\"} dneg(float53e11) "
         "returns(float53e11);\n"
      << "function {:bvbuiltin \"(_ to_fp 11 53)\"} to_double(real) "
         "returns(float53e11);\n";

  bpl << "// Bit vector function prototypes\n";
  int MIN_BIT = 1;
  int MAX_BIT = 64;
  int step = 1;
  bpl << "// Arithmetic\n";
  for (int i = MIN_BIT; i <= MAX_BIT; i += step)
    bpl << "function {:bvbuiltin \"bvadd\"} bv" << i << "add(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvsub\"} bv" << i << "sub(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvmul\"} bv" << i << "mul(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvudiv\"} bv" << i << "udiv(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvurem\"} bv" << i << "urem(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvsdiv\"} bv" << i << "sdiv(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvsrem\"} bv" << i << "srem(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvsmod\"} bv" << i << "smod(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvneg\"} bv" << i << "neg(bv" << i
        << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"(_ to_fp 8 24)\"} bv" << i
        << "float(rmode, bv" << i << ") returns(float24e8);\n"
        << "function {:bvbuiltin \"(_ to_fp 8 24) RNA\" }{:ai \"True\" } bv"
        << i << "sfloat(rmode, bv" << i << ") returns(float24e8);\n"
        << "function {:bvbuiltin \"(_ to_fp 11 53)\"} bv" << i
        << "double(rmode, bv" << i << ") returns(float53e11);\n"
        << "function {:bvbuiltin \"(_ to_fp 11 53) RNA\" }{:ai \"True\" } bv"
        << i << "sdouble(rmode, bv" << i << ") returns(float53e11);\n"
        << "function {:bvbuiltin \"(_ fp.to_ubv " << i << ")\"} float2ubv" << i
        << "(rmode, float24e8) returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"(_ fp.to_sbv " << i << ")\"} float2sbv" << i
        << "(rmode, float24e8) returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"(_ fp.to_ubv " << i << ")\"} double2ubv" << i
        << "(rmode, float53e11) returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"(_ fp.to_sbv " << i << ")\"} double2sbv" << i
        << "(rmode, float53e11) returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"(_ int2bv " << i << ")\"} int2bv" << i
        << "(int) returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bv2int\"} bv" << i << "int(bv" << i
        << ") returns(int);\n";

  bpl << "// Bitwise operations\n";
  for (int i = MIN_BIT; i <= MAX_BIT; i += step)
    bpl << "function {:bvbuiltin \"bvand\"} bv" << i << "and(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvor\"} bv" << i << "or(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvnot\"} bv" << i << "not(bv" << i
        << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvxor\"} bv" << i << "xor(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvnand\"} bv" << i << "nand(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvnor\"} bv" << i << "nor(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvxnor\"} bv" << i << "xnor(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n";

  bpl << "// Bit shifting\n";
  for (int i = MIN_BIT; i <= MAX_BIT; i += step)
    bpl << "function {:bvbuiltin \"bvshl\"} bv" << i << "shl(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvlshr\"} bv" << i << "lshr(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n"
        << "function {:bvbuiltin \"bvashr\"} bv" << i << "ashr(bv" << i << ",bv"
        << i << ") returns(bv" << i << ");\n";

  bpl << "// Unsigned comparison\n";
  for (int i = MIN_BIT; i <= MAX_BIT; i += step)
    bpl << "function {:bvbuiltin \"bvult\"} bv" << i << "ult(bv" << i << ",bv"
        << i << ") returns(bool);\n"
        << "function {:bvbuiltin \"bvule\"} bv" << i << "ule(bv" << i << ",bv"
        << i << ") returns(bool);\n"
        << "function {:bvbuiltin \"bvugt\"} bv" << i << "ugt(bv" << i << ",bv"
        << i << ") returns(bool);\n"
        << "function {:bvbuiltin \"bvuge\"} bv" << i << "uge(bv" << i << ",bv"
        << i << ") returns(bool);\n";

  bpl << "// Signed comparison\n";
  for (int i = MIN_BIT; i <= MAX_BIT; i += step)
    bpl << "function {:bvbuiltin \"bvslt\"} bv" << i << "slt(bv" << i << ",bv"
        << i << ") returns(bool);\n"
        << "function {:bvbuiltin \"bvsle\"} bv" << i << "sle(bv" << i << ",bv"
        << i << ") returns(bool);\n"
        << "function {:bvbuiltin \"bvsgt\"} bv" << i << "sgt(bv" << i << ",bv"
        << i << ") returns(bool);\n"
        << "function {:bvbuiltin \"bvsge\"} bv" << i << "sge(bv" << i << ",bv"
        << i << ") returns(bool);\n";

  bpl << "// Datatype conversion from bool to bit vector\n";
  for (int i = MIN_BIT; i <= MAX_BIT; i += step)
    bpl << "procedure {:inline 1} bool2bv" << i << " (i: bool) returns ( o: bv"
        << i << ")\n{\n\tif (i == true)\n\t{\n\t\to := 1bv" << i
        << ";\n\t}\n\telse\n\t{\n\t\to := 0bv" << i << ";\n\t}\n}\n ";

  bpl << "// Datatype conversion from other bv to bv\n";
  for (int i = MIN_BIT; i <= MAX_BIT; i += step)
    for (int j = i + 1; j <= MAX_BIT; j++)
      bpl << "function {:bvbuiltin \"zero_extend " << j - i << "\"} zext.bv"
          << i << ".bv" << j << "(bv" << i << ") returns(bv" << j << ");\n"
          << "function {:bvbuiltin \"sign_extend " << j - i << "\"} sext.bv"
          << i << ".bv" << j << "(bv" << i << ") returns(bv" << j << ");\n";
}

void BoogieCodeGenerator::generateMainForLoopInterchange(int distance) {
  bpl << "procedure main () {\n";

  // Pick two loop iterations
  for (int i = 0; i < 2; i++) {
    bpl << "\tvar stmt_" << i << ": bv" << BW << ";\n\tvar address_" << i
        << ": bv64;\n";
    for (int j = 0; j < loopDepth; j++)
      bpl << "\tvar iteration_" << i << "_" << j << ": bv" << BW << ";\n";
    bpl << "\tvar is_load_" << i << ": bool;\n\tvar valid_" << i
        << ": bool;\n\tvar array_" << i << ": bv" << BW << ";\n";
  }

  // Function arguments
  for (auto fi = F->arg_begin(); fi != F->arg_end(); ++fi) {

    if (fi->getType()->isPointerTy()) {
      if (isa<ArrayType>(
              dyn_cast<PointerType>(fi->getType())->getElementType())) {
        if (isa<IntegerType>(
                dyn_cast<ArrayType>(
                    dyn_cast<PointerType>(fi->getType())->getElementType())
                    ->getArrayElementType()))
          bpl << "\tvar " << getVariableNameInBoogie(fi) << ": [bv" << BW
              << "]bv"
              << dyn_cast<IntegerType>(
                     dyn_cast<ArrayType>(
                         dyn_cast<PointerType>(fi->getType())->getElementType())
                         ->getArrayElementType())
                     ->getBitWidth()
              << ";\n";
        else
          bpl << "\tvar " << getVariableNameInBoogie(fi) << ": [bv" << BW
              << "]real;\n";
      } else if (isa<IntegerType>(
                     dyn_cast<PointerType>(fi->getType())
                         ->getElementType())) // pointer - unverified
        bpl << "\tvar " << getVariableNameInBoogie(fi) << ": [bv" << BW << "]bv"
            << dyn_cast<IntegerType>(
                   dyn_cast<PointerType>(fi->getType())->getElementType())
                   ->getBitWidth()
            << ";\n";
      else
        bpl << "\tvar " << getVariableNameInBoogie(fi) << ": [bv" << BW
            << "]real;\n";
    } else {
      if (fi->getType()->isIntegerTy())
        bpl << "\tvar " << getVariableNameInBoogie(fi) << ": bv"
            << dyn_cast<IntegerType>(fi->getType())->getBitWidth() << ";\n";
      else
        bpl << "\tvar " << getVariableNameInBoogie(fi) << ": real;\n";
    }
  }

  // Loop iteration distance
  bpl << "\tvar distance: bv32;\n"
      << "\tdistance := " << distance << "bv32;\n";

  for (int i = 0; i < 2; i++) {
    bpl << "\tcall stmt_" << i << ", address_" << i << ", ";
    for (int j = 0; j < loopDepth; j++)
      bpl << "iteration_" << i << "_" << j << ", ";
    bpl << "is_load_" << i << ", valid_" << i << ", array_" << i
        << " := " << F->getName().str() << "(";
    for (auto fi = F->arg_begin(); fi != F->arg_end(); ++fi) {
      bpl << getVariableNameInBoogie(fi);
      auto fi_comma = fi;
      fi_comma++;
      if (fi_comma != F->arg_end())
        bpl << ", ";
    }
    bpl << ");\n";
  }

  bpl << "\t// Assertions:\n"
      << "\tassert !valid_0 || !valid_1 || "
      << "(is_load_0 && is_load_1) || "
      << "array_0 != array_1 || "
      << "stmt_0 == stmt_1 || "
      << "bv32uge(iteration_0_1, iteration_1_1) || "
      << "bv32ult(iteration_0_1, bv32sub(iteration_1_1, distance)) || "
      << "address_0 != address_1;\n ";

  bpl << "}\n";
}