#pragma once

// Mirror an instruction for a different set of inputs
Value *mirrorInst(Instruction *nextInst, ArrayRef<int> opIndices,
                  ArrayRef<Value *> ins, IRBuilder<> &builder);