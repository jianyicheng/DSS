#pragma once
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "ElasticPass/Head.h"
#include "Nodes.h"

#include "VHDLPortParser.h"

#include <fstream>
#include <regex>
#include <string>
#include <vector>

using namespace llvm;

namespace AutopilotParser {

struct pipelineState {
  int SV = -1;
  double delay = 0.0;
  std::vector<std::string> stmts;
};

enum PortType { INPUT, OUTPUT, BRAM, UNKNOWN };

class PortInfo {

public:
  PortInfo();
  PortInfo(PortType portType, Argument *value);
  PortInfo(PortType portType, std::string valName, Type *valType, int index);
  ~PortInfo();

  int getIndex() { return idx; }
  int getType() { return type; }
  std::string getName() { return name; }
  Type *getDataType() { return dataType; }
  Type *getDataElementType() { return elementType; }
  std::string getElementTypeAsString() { return elementTypeAsString; }
  Value *getValToUse() { return valToUse; }
  int getFIFODepth() { return fifoDepth; }
  int getOffset() { return offset; }
  int getIdleStates() { return idleStates; }
  int getFirstOpLatency() { return firstOpLatency; }
  int getPortIndex() { return portIdx; }

  void setOffset(int off) { offset = off; }
  void setFirstOpLatency(int latency) { firstOpLatency = latency; }
  void setIdleStates(int states) { idleStates = states; }
  void setValToUse(Value *value) { valToUse = value; }
  void setFIFODepth(int depth) { fifoDepth = depth; }
  void setPortIndex(int idx) { portIdx = idx; }

  std::string print() {
    return name + ", " + std::to_string(idx) + ", " +
           std::to_string(type == INPUT) + ", " + std::to_string(offset) +
           ", " + std::to_string(idleStates) + ", " +
           std::to_string(firstOpLatency) + ", " + std::to_string(fifoDepth) +
           ",\n";
  }

private:
  PortType type;                   // Type of the port
  Value *oldVal;                   // old variable
  Type *dataType;                  // Data type, e.g. int, float
  Type *elementType;               // Element type if oldVar is a pointer
  std::string name;                // Name of the port
  std::string elementTypeAsString; // Data type printed in string
  int idx = -1;                    // Index of argument
  int portIdx = -1;                // Index of hardware port
  Value *valToUse;                 // Value to be used in the mirror function
  int offset = 0;                  // Offset at hardware IO
  int idleStates = 0;              // Number of idle states to the output
  int firstOpLatency = 0; // The latency of the first op that consumes the input
  int fifoDepth = 0;      // FIFO depth needed for the IO
};

class AutopilotParser {

public:
  AutopilotParser() {}
  AutopilotParser(std::ifstream &, Function *);
  AutopilotParser(Function *F) { func = F; }
  ~AutopilotParser() {}

  int getNumInputs() { return inCount; }
  int getNumOutputs() { return outCount; }
  int getNumMemories() { return memCount; }
  int getTotalStates() { return states; }
  int getLatency() { return latency; }
  int getII() { return II; }
  bool hasDummyInput() { return hasDummyIn; }
  bool containLoops() { return hasLoop; }
  Function *getFunction() { return func; }
  llvm::DenseMap<int, pipelineState *> &getPipelineStates() {
    return pipelineStates;
  }
  llvm::DenseMap<int, PortInfo *> &getPortInfo() { return portInfo; }

  // Construct port info from LLVM IR, which does not have the timing info
  void analyzePortInfoFromSource();
  // Construct port info from autopilot scheduling report
  void anlayzePortInfo(bool hasOffset = true);
  // Analyze the idle states and first op latency for compat scheduling
  void analyzeIdleStatesAndFirstOpLatency();
  // Analyze the dot graph and extract the port mapping of the function
  void analyzePortIndices(ENode *callNode, std::vector<ENode *> *enode_dag);
  // Correct offsets based on the RTL interface
  void adjustOffsets(VHDLPortInfo &vPortInfo);

  // Print offset information to a file
  std::string exportOffsets();

private:
  Function *func;
  int inCount = 0;
  int outCount = 0;
  int memCount = 0;
  int states = -1;
  int latency = -1;
  int II = -1;
  bool hasLoop = false;
  llvm::DenseMap<int, pipelineState *> pipelineStates;
  llvm::DenseMap<int, PortInfo *> portInfo;
  bool hasDummyIn = false;

  void getIdleStatesAndFirstOpLatency(const std::string &name, const int offset,
                                      int *firstOpLatency, int *idleStates);
};

} // namespace AutopilotParser