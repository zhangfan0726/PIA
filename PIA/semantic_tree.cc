#include "semantic_tree.h"
#include "vector"
#include "string"
#include "iostream"

using namespace std;

SemanticTreeNode* MakeSemanticTree(const string& id, const string& op, vector<SemanticTreeNode*> children) {
  SemanticTreeNode* cur_node = new SemanticTreeNode;
  cur_node->id = id;
  cur_node->op = op;
  cur_node->children = children;

  return cur_node;
}

SemanticTreeNode* MakeSemanticTree(const string& id, const string& op) {
  SemanticTreeNode* cur_node = new SemanticTreeNode;
  cur_node->id = id;
  cur_node->op = op;

  return cur_node;
}

void PrintSemanticTree(SemanticTreeNode* semantic_tree, int depth) {
  for (int i = 0; i < depth; ++i, cerr << "    ");
  cerr << "id : " << semantic_tree->id << " op : " << semantic_tree->op << endl;
  for (int i = 0; i < semantic_tree->children.size(); ++i ) {
    PrintSemanticTree(semantic_tree->children[i], depth + 1);
  }
}
