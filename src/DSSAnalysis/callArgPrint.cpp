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

std::ofstream rpt;

using namespace std;

void callArgPrint(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag, std::string fileName) {
    errs() << "Start input matching for the function calls...\n" ;
#ifdef DSS_DEBUG
    errs() << "Listing components: \n" ;
    for (auto& enode : *enode_dag) {
        std::string name = enode->Name + "_" + to_string(enode->id);
        errs() << "Found: " << name << ": " << to_string(enode->type) << "\n";
    }
#endif
    
    rpt.open(fileName);
    
    for (auto& enode : *enode_dag) {
        if (enode->Name == "call"){
            std::string name = enode->Name + "_" + to_string(enode->id);
            rpt << name << ": ";
            if (isa<CallInst>(enode->Instr)){
                std::string funcName =  cast<CallInst>(enode->Instr)->getCalledFunction()->getName();
                char *cstr = &funcName[0];
                rpt << demangle(cstr);
            }
            rpt << "\n" ;
            std::string str;
            llvm::raw_string_ostream rso(str);
            enode->Instr->print(rso);
            rpt << str << "\n" ;
            if (enode->CntrlPreds->size() > 1){
                for (auto& arg : *(enode->CntrlPreds)){
                    name = arg->Name + "_" + to_string(arg->id);
                    rpt << "\tfrom: " << name << ":\n";
                    searchInstr(arg);
                }
            }
        }
    }
    rpt.close();
    errs() << "Input matching of the function calls finished. Report saved in "<<fileName<<"\n" ;
}

void searchInstr(ENode* node){
    std::vector<ENode*> searchList;
    std::vector<ENode*> toSearchList;

    searchList.push_back(node);
    
    while(searchList.size() > 0){
        for (auto& in : searchList){
            if (in->type == 2 || in->type == 13){
                rpt << "\t" << to_string(in->type)+":"+in->Name + "_" + to_string(in->id) << "\n" ;
                return;
            }
            else if(in->Instr != NULL){
                rpt << "\t" << to_string(in->type)+":";
                std::string str;
                llvm::raw_string_ostream rso(str);
                in->Instr->print(rso);
                rpt << str << "\n" ;
                return;
            }
            else{
                for (auto& arg : *(in->CntrlPreds))
                    toSearchList.push_back(arg);
            }
                
        }
#ifdef DSS_DEBUG
        for (auto& in : searchList){
            errs() << in->Name + "_" + to_string(in->id) << ",";
        }
        errs() << "\n";
        for (auto& in : toSearchList){
            errs() << in->Name + "_" + to_string(in->id) << ",";
        }
        errs() << "\n";
#endif
        searchList.clear();
        searchList.swap(toSearchList);
        //searchList = std::move(toSearchList);
    }
    rpt << "\t" << "Error in instruction searching: cannot find a valid argument!" << "\n" ;
}

std::string demangle(const char* name)
{
        int status = -1;

        std::unique_ptr<char, void(*)(void*)> res { abi::__cxa_demangle(name, NULL, NULL, &status), std::free };
        return (status == 0) ? res.get() : std::string(name);
}
