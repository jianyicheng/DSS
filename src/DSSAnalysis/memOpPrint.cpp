#include "ElasticPass/ComponentsTiming.h"
#include "ElasticPass/Memory.h"
#include "ElasticPass/Utils.h"
#include "ElasticPass/Pragmas.h"
#include "DSSAnalysis/DSSAnalysis.h"
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <cxxabi.h>

using namespace std;

void memOpPrint(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag, std::string fileName) {
    std::ofstream m_rpt;
    m_rpt.open(fileName);
    errs() << "Start extracting the link between memory node and instructions...\n" ;
    for (auto& enode : *enode_dag) {
        if (enode->Name == "store"){
          std::string name = enode->Name + "_" + to_string(enode->id);
          std::string str;
          llvm::raw_string_ostream rso(str);
          enode->Instr->getOperand(1)->print(rso);
          m_rpt << name << ": " << str << "\n";
        }
        else if (enode->Name == "load"){
          std::string name = enode->Name + "_" + to_string(enode->id);
          std::string str;
          llvm::raw_string_ostream rso(str);
          enode->Instr->getOperand(0)->print(rso);
          m_rpt << name << ": " << str << "\n";
        }
    }
    m_rpt.close();
     errs() << "Extracted memory link successfully. Report saved in "<<fileName<<"\n" ;
}

