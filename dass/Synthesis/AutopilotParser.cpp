#include "AutopilotParser.h"

namespace AutopilotParser {

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

AutopilotParser::AutopilotParser(std::ifstream &ifile) {
  ifile.clear();
  ifile.seekg(0);
  std::string line;
  std::getline(ifile, line);

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

} // namespace AutopilotParser
