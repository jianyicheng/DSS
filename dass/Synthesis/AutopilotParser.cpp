#include "AutopilotParser.h"

namespace AutopilotParser {

PortInfo::PortInfo(PortType portType, Argument *value) {
  type = portType;
  oldVal = value;
  auto currType = value->getType();
  dataType = currType;
  while (currType->isPointerTy())
    currType = currType->getPointerElementType();
  elementType = currType;
  name = value->getName().str();
  elementTypeAsString =
      (currType->isDoubleTy())
          ? "f64"
          : (currType->isFloatTy())
                ? "f32"
                : (currType->isIntegerTy())
                      ? "i" + std::to_string(currType->getIntegerBitWidth())
                      : "X";
  idx = value->getArgNo();
}

PortInfo::PortInfo(PortType portType, std::string valName, Type *valType,
                   int index) {
  type = portType;
  oldVal = nullptr;
  auto currType = valType;
  dataType = currType;
  while (currType->isPointerTy())
    currType = currType->getPointerElementType();
  elementType = currType;
  name = valName;
  elementTypeAsString =
      (currType->isDoubleTy())
          ? "f64"
          : (currType->isFloatTy())
                ? "f32"
                : (currType->isIntegerTy())
                      ? "i" + std::to_string(currType->getIntegerBitWidth())
                      : "X";
  idx = index;
}

static int getNumLHS(std::string line, std::string substr) {
  if (line.find(substr) == std::string::npos) {
    llvm_unreachable(
        std::string("Missing constraints " + substr + " in " + line + ".")
            .c_str());
  } else {
    int tempIdx = line.find(substr) + substr.length();
    while (isdigit(line[tempIdx]))
      tempIdx++;
    std::string s = line.substr(line.find(substr) + substr.length(),
                                tempIdx - line.find(substr) - substr.length());
    if (!s.empty() && std::all_of(s.begin(), s.end(), ::isdigit))
      return std::stoi(s);
    else {
      llvm_unreachable(std::string("Invalid constraint extracted for " +
                                   substr + " in " + line)
                           .c_str());
    }
  }
  return -1;
}

AutopilotParser::AutopilotParser(std::ifstream &ifile, Function *F) {
  ifile.clear();
  ifile.seekg(0);
  std::string line;
  std::getline(ifile, line);

  // Load function name
  func = F;
  while (line.find("Vitis HLS Report for ") == std::string::npos)
    std::getline(ifile, line);
  auto name =
      line.substr(line.find("\'") + 1, line.rfind("\'") - line.find("\'") - 1);
  if (name != func->getName().str())
    llvm_unreachable(
        std::string("AutopilotParser: Function names do not match - " + name +
                    " & " + func->getName().str())
            .c_str());

  // Load latency
  while (line.find("+ Latency:") == std::string::npos)
    std::getline(ifile, line);
  // Skip the table header
  for (int i = 0; i < 6; i++)
    std::getline(ifile, line);
  auto bar0 = line.find("|");
  auto bar1 = line.find("|", bar0 + 1);
  auto minLatencyStr = line.substr(bar0 + 1, bar1 - bar0 - 1);
  auto bar2 = line.find("|", bar1 + 1);
  auto maxLatencyStr = line.substr(bar1 + 1, bar2 - bar1 - 1);
  if (minLatencyStr.find("?") == std::string::npos &&
      maxLatencyStr.find("?") == std::string::npos) {
    auto minLatency = std::stoi(minLatencyStr);
    auto maxLatency = std::stoi(maxLatencyStr);
    if (minLatency != maxLatency)
      llvm::errs() << "Warning: Function " << name
                   << " has a variable latency - (" << minLatency << ", "
                   << maxLatency << ")\n";
    latency = maxLatency;
  } else {
    latency = 100;
    llvm::errs() << "Warning: Function " << name
                 << " has an unknown latency - ??? \n";
  }

  // Load II
  auto lastBar0 = line.rfind("|");
  auto lastBar1 = line.rfind("|", lastBar0 - 1);
  auto lastBar2 = line.rfind("|", lastBar1 - 1);
  auto lastBar3 = line.rfind("|", lastBar2 - 1);

  auto maxIIStr = line.substr(lastBar3 + 1, lastBar2 - lastBar3 - 1);
  auto minIIStr = line.substr(lastBar2 + 1, lastBar1 - lastBar2 - 1);

  if (minIIStr.find("?") == std::string::npos &&
      maxIIStr.find("?") == std::string::npos) {
    auto minII = std::stoi(minIIStr);
    auto maxII = std::stoi(maxIIStr);
    if (minII != maxII)
      llvm::errs() << "Warning: Function " << name << " has a variable II - ("
                   << minII << ", " << maxII << ")\n";
    II = maxII;
  } else {
    II = 100;
    llvm::errs() << "Warning: Function " << name
                 << " has an unknown II - ??? \n";
  }

  // Load the number of states
  while (line.find("* Number of FSM states : ") == std::string::npos)
    std::getline(ifile, line);
  states = getNumLHS(line, "* Number of FSM states : ");

  // Load pipeline states
  while (line.find("* FSM state operations:") == std::string::npos)
    std::getline(ifile, line);

  int stateID = 1;
  while (stateID <= states) {
    while (line.find("State") == std::string::npos)
      std::getline(ifile, line);
    if (line.find("State " + std::to_string(stateID) + " <") ==
        std::string::npos)
      llvm_unreachable("Cannot find states in the scheduling report!");
    pipelineState *ps = new pipelineState;
    ps->SV = getNumLHS(line, "<SV = ");
    auto start = line.find("<Delay = ") + 9;
    ps->delay = std::stod(line.substr(start, line.rfind(">") - start));
    std::getline(ifile, line);
    while (line.find("ST_" + std::to_string(stateID)) != std::string::npos) {
      ps->stmts.push_back(line);
      std::getline(ifile, line);
    }
    pipelineStates[stateID] = ps;
    stateID++;
  }
}

// Load info from module itself
void AutopilotParser::analyzePortInfoFromSource() {
  auto DT = llvm::DominatorTree(*func);
  LoopInfo LI(DT);
  hasLoop = !LI.empty();

  for (auto &arg : func->args()) {
    PortInfo *port;
    if (arg.getType()->isPointerTy()) {
      auto useOp = arg.use_begin()->getUser();
      if (isa<CallInst>(useOp) || isa<GetElementPtrInst>(useOp)) {
        port = new PortInfo(BRAM, &arg);
        memCount++;
      } else {
        port = new PortInfo(OUTPUT, &arg);
        outCount++;
      }
    } else {
      port = new PortInfo(INPUT, &arg);
      inCount++;
    }
    portInfo[port->getIndex()] = port;
  }

  // If no input, add a dummy input as a trigger
  if (inCount == 0) {
    portInfo[func->arg_size()] =
        new PortInfo(INPUT, "control", Type::getInt32Ty(func->getContext()),
                     func->arg_size());
    inCount++;
    hasDummyIn = true;
  }
}

static std::string getOp(std::string line) {
  auto start = line.find("\"");
  return " " + line.substr(start + 1, line.find("\"", start + 1) - start - 1) +
         " ";
}

// Get the objective where the argument is used.
static std::string getUse(const std::string &name,
                          llvm::DenseMap<int, pipelineState *> &pipelineStates,
                          const int states, bool isRead) {
  for (int st = 1; st <= states; st++) {
    for (auto stmt : pipelineStates[st]->stmts) {
      auto op = getOp(stmt);
      if (op.find(" %" + name + " ") != std::string::npos ||
          op.find(" %" + name + ",") != std::string::npos) {
        if (isRead && (op.find(" read ") != std::string::npos ||
                       op.find(" nbread ") != std::string::npos)) {
          auto start = op.find("%") + 1;
          return op.substr(start, op.find(" =") - start);
        } else if (!isRead && (op.find(" write ") != std::string::npos ||
                               op.find(" nbwrite ") != std::string::npos)) {
          auto start = op.rfind("%") + 1;
          return op.substr(start, op.rfind(" ") - start);
        }
      }
    }
  }
  llvm::errs() << "Cannot find schedule of argument " << name
               << " - consider offset = 0\n";
  return "";
}

static int
getOffsetFromOneState(int st, const std::string &name,
                      llvm::DenseMap<int, pipelineState *> &pipelineStates,
                      const bool isRead) {
  for (auto stmt : pipelineStates[st]->stmts) {
    auto op = getOp(stmt);
    if (op.find(" %" + name + " ") != std::string::npos ||
        op.find(" %" + name + ",") != std::string::npos) {
      if (isRead && op.find(" %" + name + " = ") == std::string::npos)
        return pipelineStates[st]->SV;
      else if (!isRead && op.find(" %" + name + " = ") != std::string::npos)
        return pipelineStates[st]->SV;
    }
  }
  return -1;
}

// Get the earliest steady state where the objective is used as the offset.
static int getOffset(const std::string &name,
                     llvm::DenseMap<int, pipelineState *> &pipelineStates,
                     const int states, const bool isRead) {
  if (isRead)
    for (int st = 1; st <= states; st++) {
      auto offset = getOffsetFromOneState(st, name, pipelineStates, isRead);
      if (offset != -1)
        return offset;
    }
  else
    for (int st = states; st >= 1; st--) {
      auto offset = getOffsetFromOneState(st, name, pipelineStates, isRead);
      if (offset != -1)
        return offset;
    }
  return -1;
}

// Load offset to function map. If the file does not exists, assume offset = 0
void AutopilotParser::anlayzePortInfo(bool hasOffset) {
  analyzePortInfoFromSource();

  if (hasLoop || !hasOffset)
    return;

  for (auto i = 0; i < func->arg_size(); i++) {
    if (portInfo[i]->getType() == BRAM)
      continue;

    bool isRead = (portInfo[i]->getType() == INPUT);
    auto useName =
        getUse(portInfo[i]->getName(), pipelineStates, states, isRead);
    auto offset = (useName == "") ? 0 : getOffset(useName, pipelineStates,
                                                  states, isRead);
    if (offset == -1)
      llvm_unreachable(std::string(func->getName().str() +
                                   ": Cannot find schedule of use " + useName)
                           .c_str());
    assert(offset >= 0 && offset <= latency);
    portInfo[i]->setOffset(offset);
    auto fifoDepth =
        (portInfo[i]->getType() == INPUT) ? offset : latency - offset;
    portInfo[i]->setFIFODepth(fifoDepth);
  }
}

void AutopilotParser::getIdleStatesAndFirstOpLatency(const std::string &name,
                                                     const int offset,
                                                     int *firstOpLatency,
                                                     int *idleStates) {
  auto isFirstOp = false;
  auto varName = "%" + name;
  auto idleStateCount = 0, firstOpLatencyCount = 0;
  for (int st = offset + 1; st <= states; st++) {
    auto isIdle = true;
    auto useCount = 0;
    for (auto stmt : pipelineStates[st]->stmts) {
      auto op = getOp(stmt);
      if (op.find(" " + varName + " ") != std::string::npos ||
          op.find(" " + varName + ",") != std::string::npos) {
        if (op.find(" " + varName + " = ") == std::string::npos) {
          // Stop counting when there are multiple uses
          if (useCount > 0) {
            *firstOpLatency = firstOpLatencyCount;
            *idleStates = idleStateCount;
          }
          if (isFirstOp)
            isFirstOp = false;
          if (varName == "%" + name)
            isFirstOp = true;
          varName = op.substr(op.find("%"), op.find(" =") - op.find("%"));
          useCount++;
        }
        isIdle = false;
      }
    }
    idleStateCount += isIdle;
    if (isFirstOp && !isIdle)
      firstOpLatencyCount++;
  }
  *firstOpLatency = firstOpLatencyCount;
  *idleStates = idleStateCount;
}

void AutopilotParser::analyzeIdleStatesAndFirstOpLatency() {
  llvm::DenseMap<int, std::pair<int, int>> ifl;
  for (auto i = 0; i < func->arg_size(); i++) {
    if (portInfo[i]->getType() != INPUT || portInfo[i]->getOffset() == 0)
      continue;

    auto useName = getUse(portInfo[i]->getName(), pipelineStates, states, true);
    int idleStates = 0, firstOpLatency = 0;
    getIdleStatesAndFirstOpLatency(useName, portInfo[i]->getOffset(),
                                   &firstOpLatency, &idleStates);
    portInfo[i]->setFirstOpLatency(firstOpLatency);
    portInfo[i]->setIdleStates(idleStates);
    portInfo[i]->setFIFODepth(portInfo[i]->getFIFODepth() + idleStates);
  }
}

static Value *getInstrInput(int index, ENode *node) {
  auto inNode = (*node->CntrlPreds)[index];
  while (!inNode->Instr) {
    if (inNode->type == Fork_ || inNode->type == Phi_n)
      inNode = (*inNode->CntrlPreds)[0];
    else if (inNode->type == Branch_n) {
      // This is a bit tricky as we need to find the correct value from the two
      // inputs
      auto in0 = inNode->CntrlPreds->at(0);
      if (in0->type == Fork_ || in0->type == Fork_c)
        in0 = in0->CntrlPreds->at(0);
      auto in1 = inNode->CntrlPreds->at(1);
      if (in1->type == Fork_ || in1->type == Fork_c)
        in1 = in1->CntrlPreds->at(0);
      assert(in0->type == Branch_ || in1->type == Branch_);
      if (in0->type == Branch_ && in1->type == Branch_)
        inNode = (in0->Instr) ? in0 : in1;
      else
        inNode = (in0->type == Branch_) ? in1 : in0;
    } else if (inNode->type == Cst_) {
      assert(inNode->CI || inNode->CF);
      return (inNode->CI) ? dyn_cast<Value>(inNode->CI)
                          : dyn_cast<Value>(inNode->CF);
    } else
      llvm_unreachable(
          std::string("Found unrecognized input elastic node type for node: " +
                      getNodeDotNameNew(inNode) + " pointing to " +
                      dyn_cast<CallInst>(node->Instr)
                          ->getCalledFunction()
                          ->getName()
                          .str())
              .c_str());
  }
  return inNode->Instr;
}

// JC: This function is quite inefficent. To change
static Value *getInstrOutput(int index, ENode *node) {
  auto outNode = (*node->CntrlSuccs)[index];
  while (!outNode->Instr) {
    // llvm_unreachable(
    //     std::string("Found unrecognized output elastic node type for node: "
    //     +
    //                 getNodeDotNameNew(outNode) + " pointing to " +
    //                 dyn_cast<CallInst>(node->Instr)
    //                     ->getCalledFunction()
    //                     ->getName()
    //                     .str())
    //         .c_str());
    outNode = (*outNode->CntrlSuccs)[0];
  }
  auto outInst = outNode->Instr;
  auto callInst = node->Instr;

  for (auto i = 0; i < outInst->getNumOperands(); i++) {
    if (auto loadInst = dyn_cast<LoadInst>(outInst->getOperand(i))) {
      auto arg = loadInst->getPointerOperand();
      for (auto j = 0; j < callInst->getNumOperands(); j++)
        if (arg == callInst->getOperand(j))
          return arg;
    }
  }
  return nullptr;
}

void AutopilotParser::analyzePortIndices(ENode *callNode,
                                         std::vector<ENode *> *enode_dag) {
  auto inst = callNode->Instr;
  assert(dyn_cast<CallInst>(inst)->getCalledFunction() == func);

  if (inCount > 1) {
    for (auto i = 0; i < portInfo.size(); i++) {
      auto op = inst->getOperand(portInfo[i]->getIndex());
      // Skip load used for loading pointers
      if (auto loadInst = dyn_cast<LoadInst>(op))
        if (auto allocaInst =
                dyn_cast<AllocaInst>(loadInst->getPointerOperand()))
          for (auto user : allocaInst->users())
            if (isa<CallInst>(user))
              op = user;

      if (portInfo[i]->getType() == INPUT) {
        for (auto j = 0; j < callNode->CntrlPreds->size(); j++) {
          if (getInstrInput(j, callNode) == op) {
            portInfo[i]->setPortIndex(j);
            break;
          }
        }
        if (portInfo[i]->getPortIndex() == -1)
          llvm_unreachable(std::string("Cannot find arg port index: " +
                                       portInfo[i]->getName() + " in " +
                                       func->getName().str())
                               .c_str());
      }
    }
  } else {
    for (auto i = 0; i < portInfo.size(); i++)
      if (portInfo[i]->getType() == INPUT) {
        portInfo[i]->setPortIndex(0);
        break;
      }
  }

  if (outCount > 1) {
    for (auto i = 0; i < portInfo.size(); i++) {
      auto op = inst->getOperand(portInfo[i]->getIndex());
      if (portInfo[i]->getType() == OUTPUT) {
        for (auto j = 0; j < callNode->CntrlSuccs->size(); j++) {
          if (getInstrOutput(j, callNode) == op) {
            portInfo[i]->setPortIndex(j);
            break;
          }
        }
        if (portInfo[i]->getPortIndex() == -1)
          llvm_unreachable(std::string("Cannot find arg port index: " +
                                       portInfo[i]->getName() + " in " +
                                       func->getName().str())
                               .c_str());
      }
    }
  } else {
    for (auto i = 0; i < portInfo.size(); i++)
      if (portInfo[i]->getType() == OUTPUT) {
        portInfo[i]->setPortIndex(0);
        break;
      }
  }
}

std::string AutopilotParser::exportOffsets() {
  std::string buffer = "";
  buffer += "Function: " + func->getName().str() + ", " +
            std::to_string(latency) + ", " + std::to_string(II) + ",\n";
  for (auto i = 0; i < func->arg_size(); i++) {
    if (portInfo[i]->getType() == BRAM)
      continue;
    buffer += portInfo[i]->print();
  }
  buffer += "---\n";
  return buffer;
}

void AutopilotParser::adjustOffsets(VHDLPortInfo &vPortInfo) {
  for (auto &p : portInfo) {
    auto port = p.second;
    if (vPortInfo.isHandshake(port->getName()))
      if (port->getType() == INPUT) {
        port->setOffset(0);
        port->setFIFODepth(0);
      } else if (port->getType() == OUTPUT) {
        port->setOffset(latency);
        port->setFIFODepth(0);
      }
  }
}

} // namespace AutopilotParser
