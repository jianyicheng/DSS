#pragma once
#include <algorithm>
#include <list>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Nodes.h"

// #define DSS_DEBUG

void callArgPrint(std::vector<ENode*>* enode_dag, std::vector<BBNode*>* bbnode_dag, std::string fileName);
void searchInstr(ENode* node);
std::string demangle(const char* name);
std::string printName(Value *I);
