#ifndef SEMANTIC_TREE_H_
#define SEMANTIC_TREE_H_

#include "string"
#include "vector"

using namespace std;

class SemanticTreeNode {
 public:
  string id;
  string op;
  vector<SemanticTreeNode*> children;
};

SemanticTreeNode* MakeSemanticTree(const string& dest_id, const string& op, vector<SemanticTreeNode*> children);

SemanticTreeNode* MakeSemanticTree(const string& dest_id, const string& op);

void PrintSemanticTree(SemanticTreeNode* semantic_tree, int depth = 0);

#endif
