#ifndef SYNTAX_TREE_H_
#define SYNTAX_TREE_H_

#include "semantic_tree.h"

#include "vector"
#include "string"

#include "llvm/Analysis/Passes.h"
#include "llvm/ExecutionEngine/JIT.h" 
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Vectorize.h"

// The operator types used in PIA
typedef enum {ADD, SUB, MUL, DIV, PLUSPLUS, MINUSMINUS, NEG, MOD} OPERATOR_TYPE;

using namespace std;
using namespace llvm;

extern Module* the_module;
extern IRBuilder<> ir_builder;
extern map<std::string, Value*> named_values;
extern FunctionPassManager* function_pass_manager;
extern PassManager* pass_manager;
extern ExecutionEngine* the_execution_engine;

class ASTNode;

class FieldInfo {
 public:
  string type;
  int left_margin;
  int right_margin;
  int top_margin;
  int bottom_margin;
};

// Stores the PIA stencil root.
class StencilAST {
 protected:
  map<string, FieldInfo> input_fields;
  map<string, FieldInfo> output_fields;
  map<string, string> input_variables;
  map<string, string> local_variables;

  vector<string> loop_variables;

  int i_left, i_right, j_left, j_right;

  vector<ASTNode*> roots;
 public:
  void ParseFromSemanticTrees(const vector<SemanticTreeNode*>& sts);
  
  static void CollectVariableInfo(ASTNode* ast, map<string, FieldInfo>* input_fields,
                                                map<string, FieldInfo>* output_fields,
                                                map<string, string>* input_variables,
                                                map<string, string>* local_variables);
  Function* IRCodeGen();
};

class ASTNode {
 protected:
  string node_type;
 public:
  ASTNode();
  virtual ~ASTNode();

  virtual string get_node_type();

  virtual void ParseFromSemanticTreeNode(SemanticTreeNode* stn);
  virtual void Print();

  virtual Value* IRCodeGen();
// static funtion
  static ASTNode* ASTNodeFactory(SemanticTreeNode* stn);
};

class ASTNodeExpr : public ASTNode {
 protected:
 public:
  ASTNodeExpr();
  virtual ~ASTNodeExpr();

  virtual string get_node_type();
  virtual void ParseFromSemanticTreeNode(SemanticTreeNode* stn);
  virtual void Print();

  virtual Value* IRCodeGen();
};

class ASTNodeExprConst : public ASTNodeExpr {
 protected:
  float value;
 public:
  ASTNodeExprConst();
  virtual ~ASTNodeExprConst();

  virtual string get_node_type();
  virtual void ParseFromSemanticTreeNode(SemanticTreeNode* stn);
  virtual void Print();

  virtual Value* IRCodeGen();
};

class ASTNodeExprInputField : public ASTNodeExpr {
 protected:
  string id;
  int offset_x;
  int offset_y;
 public:
  ASTNodeExprInputField();
  virtual ~ASTNodeExprInputField();

  int get_offset_x() { return offset_x; }
  int get_offset_y() { return offset_y; }
  string get_id() { return id; }

  virtual string get_node_type();
  virtual void ParseFromSemanticTreeNode(SemanticTreeNode* stn);
  virtual void Print();

  virtual Value* IRCodeGen();
};

class ASTNodeExprOutputField : public ASTNodeExpr {
 protected:
  string id;
 public:
  ASTNodeExprOutputField();
  virtual ~ASTNodeExprOutputField();

  virtual string get_node_type();
  string get_id() { return id; }
  virtual void ParseFromSemanticTreeNode(SemanticTreeNode* stn);
  virtual void Print();

  virtual Value* IRCodeGen();
};

class ASTNodeExprInputVariable : public ASTNodeExpr {
 protected:
  string id;
 public:
  ASTNodeExprInputVariable();
  virtual ~ASTNodeExprInputVariable();

  virtual string get_node_type();
  string get_id() { return id; }
  virtual void ParseFromSemanticTreeNode(SemanticTreeNode* stn);
  virtual void Print();

  virtual Value* IRCodeGen();
};

class ASTNodeExprLocalVariable : public ASTNodeExpr {
 protected:
  string id;
 public:
  ASTNodeExprLocalVariable();
  virtual ~ASTNodeExprLocalVariable();

  virtual string get_node_type();
  string get_id() { return id; }
  virtual void ParseFromSemanticTreeNode(SemanticTreeNode* stn);
  virtual void Print();

  virtual Value* IRCodeGen();
};

class ASTNodeExprBinary : public ASTNodeExpr {
 protected:
  OPERATOR_TYPE op;
  ASTNodeExpr* operand_o;
  ASTNodeExpr* operand_e;
 public:
  ASTNodeExprBinary();
  virtual ~ASTNodeExprBinary();

  ASTNodeExpr* get_operand_o() { return operand_o; }
  ASTNodeExpr* get_operand_e() { return operand_e; }

  virtual string get_node_type();
  virtual void ParseFromSemanticTreeNode(SemanticTreeNode* stn);
  virtual void Print();

  virtual Value* IRCodeGen();
};

class ASTNodeExprAssignment : public ASTNodeExpr {
 protected:
  ASTNodeExpr* l_value;
  ASTNodeExpr* r_value;
 public:
  ASTNodeExprAssignment();
  virtual ~ASTNodeExprAssignment();

  ASTNodeExpr* get_l_value() { return l_value; }
  ASTNodeExpr* get_r_value() { return r_value; }

  virtual string get_node_type();
  virtual void ParseFromSemanticTreeNode(SemanticTreeNode* stn);
  virtual void Print();

  virtual Value* IRCodeGen();
};

#endif

 
     
