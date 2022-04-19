#pragma once
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include <fstream>
#include <map>

using namespace llvm;

struct MemoryInfo {
  unsigned addressWidth = 0;
  unsigned dataWidth = 0;
  bool address0 = false;
  bool ce0 = false;
  bool we0 = false;
  bool dout0 = false;
  bool din0 = false;
  bool address1 = false;
  bool ce1 = false;
  bool we1 = false;
  bool dout1 = false;
  bool din1 = false;

  void print() {
    llvm::errs() << addressWidth << "\n"
                 << dataWidth << "\n"
                 << address0 << "\n"
                 << ce0 << "\n"
                 << we0 << "\n"
                 << dout0 << "\n"
                 << din0 << "\n"
                 << address1 << "\n"
                 << ce1 << "\n"
                 << we1 << "\n"
                 << dout1 << "\n"
                 << din1 << "\n";
  }
};

struct VHDLPortInfo {
  std::map<std::string, MemoryInfo *> memInfo;
  std::map<std::string, std::pair<bool, bool>> isScalarHandshake;

  bool isHandshake(std::string name) {
    if (!isScalarHandshake.count(name))
      return true;
    return isScalarHandshake[name].second;
  }
};

VHDLPortInfo parsePortInfoVHDL(Function *F, std::string dir);
