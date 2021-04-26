#pragma once
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <regex>
#include <string>
#include <vector>

namespace AutopilotParser {

struct pipelineState {
  int SV = -1;
  double delay = 0.0;
  std::vector<std::string> stmts;
};

class AutopilotParser {

public:
  AutopilotParser(std::ifstream &);
  ~AutopilotParser() {}

  llvm::DenseMap<unsigned, pipelineState *> getPipelineStates() {
    return pipelineStates;
  }
  unsigned getTotalStates() { return states; }

private:
  unsigned states;
  llvm::DenseMap<unsigned, pipelineState *> pipelineStates;
};

} // namespace AutopilotParser