#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <regex>

using namespace llvm;

struct BoogiePhiNode {
  BasicBlock *bb;
  std::string res;
  Value *ip;
  Instruction *instr;
};

struct BoogieInvariance {
  Loop *loop;
  llvm::PHINode *instr;
  std::string invar;
};

struct BoogieMemoryNode {
  bool load;
  int label;
  Instruction *instr;
  int offset;
  int latency;
};

class BoogieCodeGenerator {
public:
  BoogieCodeGenerator();
  BoogieCodeGenerator(Function *func) { F = func; };
  ~BoogieCodeGenerator(){};

  void sliceFunction();
  void generateBoogieHeader();
  void phiAnalysis();
  void generateFuncPrototype(bool isLoopAnalysis);
  void generateVariableDeclarations();
  void analyzeLoop(std::string loopName);
  void generateFunctionBody(bool isLoopAnalysis);

  void open(std::string file) { bpl.open(file, std::fstream::out); }
  void close() { bpl.close(); }
  std::fstream &getBoogieStream() { return bpl; }
  Function *getFunction() { return F; }
  void setFunction(Function *func) { F = func; }

  void generateMainForLoopInterchange(int distance);

private:
  Function *F;
  std::fstream bpl;
  std::string BW = "32";
  std::vector<BoogiePhiNode *> phis;
  std::vector<BoogieInvariance *> invariances;
  std::vector<Value *> arrays;
  std::vector<BoogieMemoryNode *> accesses;
  std::vector<BasicBlock *> memoryAnalysisRegions;

  Loop *targetLoop, *parentLoop;
  int loopDepth = 0;
  std::vector<Value *> loopIndvars;

  void generateBoogieInstruction(Instruction *I, LoopInfo &LI,
                                 bool analyzeMemory = true);
};
