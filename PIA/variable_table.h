#ifndef VARIABLE_TABLE_H_
#define VARIABLE_TABLE_H_

#include "vector"

#include "semantic_tree.h"
#include "syntax_tree.h"

using namespace std;

typedef enum {INPUT_FIELD, OUTPUT_FIELD, LOCAL_VARIABLE, COEFFICIENT} VARIABLE_TYPE;

typedef struct {
  VARIABLE_TYPE type;
  char* name;
} ID_REG;

extern vector<SemanticTreeNode*> semantic_trees;
extern vector<ASTNode*> syntax_trees;

void SemanticTree2SyntaxTree(const vector<SemanticTreeNode*> &semantic_trees,
                             vector<ASTNode*> *syntax_trees);

#endif
