#include "variable_table.h"
#include "semantic_tree.h"
#include "iostream"

using namespace std;

vector<SemanticTreeNode*> semantic_trees;
vector<ASTNode*> syntax_trees;

void SemanticTree2SyntaxTree(const vector<SemanticTreeNode*> &semantic_trees,
                             vector<ASTNode*> *syntax_trees) {

  
  for (int i = 0; i < semantic_trees.size(); ++i) {
    cerr << i << endl;
    ASTNode* root = ASTNode::ASTNodeFactory(semantic_trees[i]);
    root->ParseFromSemanticTreeNode(semantic_trees[i]);
    syntax_trees->push_back(root);
    root->Print();
  }
}
