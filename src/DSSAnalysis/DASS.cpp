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

void DASS(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag, std::string outdir) {
  errs() << "DASS - Analysing function = " << outdir << "\n";
  callArgPrint(enode_dag, bbnode_dag, (outdir + "call_arg_analysis.rpt").c_str());
  memOpPrint(enode_dag, bbnode_dag, (outdir + "pn_mem_analysis.rpt").c_str());
}

