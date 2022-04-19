#include "VHDLPortParser.h"

static std::vector<std::string> parseVHDLSource(Function *F, std::string dir) {
  auto fname = F->getName().str();
  auto fileName = dir + fname + "/solution1/syn/vhdl/" + fname + ".vhd";
  std::ifstream ifile(fileName);
  if (!ifile.is_open())
    llvm_unreachable(
        std::string("Cannot find RTL file " + fileName + ".\n").c_str());
  std::string line;
  std::getline(ifile, line);
  while (line.find("entity " + fname + " is") == std::string::npos)
    std::getline(ifile, line);
  std::getline(ifile, line);
  std::getline(ifile, line);

  std::vector<std::string> ports;
  while (line.find("end") == std::string::npos) {
    ports.push_back(line);
    std::getline(ifile, line);
  }
  ifile.close();
  return ports;
}

static bool portExists(ArrayRef<std::string> ports, std::string name) {
  for (auto &p : ports)
    if (p.find(name) != std::string::npos)
      return true;
  return false;
}

VHDLPortInfo parsePortInfoVHDL(Function *F, std::string dir) {
  VHDLPortInfo vPortInfo;

  auto ports = parseVHDLSource(F, dir);

  for (auto arg = F->arg_begin(); arg != F->arg_end(); arg++) {
    auto name = arg->getName().str();

    // Scalar input
    if (!arg->getType()->isPointerTy()) {
      for (auto &p : ports) {
        if (p.find(" " + name + " : IN ") != std::string::npos) {
          vPortInfo.isScalarHandshake[name] =
              std::pair<bool, bool>(true, false);
          break;
        } else if (p.find(" " + name + "_empty_n : IN ") != std::string::npos ||
                   p.find(" " + name + "oo_full_n : IN ") !=
                       std::string::npos) {
          vPortInfo.isScalarHandshake[name] = std::pair<bool, bool>(true, true);
          break;
        }
      }
      continue;
    }

    // Scalar Output
    auto useOp = arg->use_begin()->getUser();
    if (!isa<CallInst>(useOp) && !isa<GetElementPtrInst>(useOp)) {
      for (auto &p : ports) {
        if (p.find(name + " : OUT ") != std::string::npos) {
          vPortInfo.isScalarHandshake[name] =
              std::pair<bool, bool>(false, false);
          break;
        } else if (p.find(name + "_full_n : IN ") != std::string::npos) {
          vPortInfo.isScalarHandshake[name] =
              std::pair<bool, bool>(false, true);
          break;
        }
      }
      continue;
    }

    auto mInfo = new MemoryInfo;
    mInfo->address0 = portExists(ports, name + "_address0 : OUT ");
    mInfo->ce0 = portExists(ports, name + "_ce0 : OUT ");
    mInfo->we0 = portExists(ports, name + "_we0 : OUT ");
    mInfo->dout0 = portExists(ports, name + "_d0 : OUT ");
    mInfo->din0 = portExists(ports, name + "_q0 : IN ");
    mInfo->address1 = portExists(ports, name + "_address1 : OUT ");
    mInfo->ce1 = portExists(ports, name + "_ce1 : OUT ");
    mInfo->we1 = portExists(ports, name + "_we1 : OUT ");
    mInfo->dout1 = portExists(ports, name + "_d1 : OUT ");
    mInfo->din1 = portExists(ports, name + "_q1 : IN ");

    for (auto &p : ports) {
      if (p.find(name + "_address0 : OUT ") != std::string::npos ||
          p.find(name + "_address1 : OUT ") != std::string::npos) {
        mInfo->addressWidth =
            std::stoi(
                p.substr(p.find("(") + 1, p.find("downto") - p.find("(") - 2)) +
            1;
      }
      if (p.find(name + "_q0 : IN ") != std::string::npos ||
          p.find(name + "_d1 : OUT ") != std::string::npos) {
        mInfo->dataWidth =
            std::stoi(
                p.substr(p.find("(") + 1, p.find("downto") - p.find("(") - 2)) +
            1;
      }
    }
    if (mInfo->addressWidth == 0)
      llvm_unreachable(std::string("Cannot find array " + name +
                                   " in VHDL file. Please check if you have "
                                   "used a proper name for the array - Vitis "
                                   "HLS sometimes renames the array and causes "
                                   "this bug.")
                           .c_str());
    // mInfo->print();
    vPortInfo.memInfo[name] = mInfo;
  }
  return vPortInfo;
}