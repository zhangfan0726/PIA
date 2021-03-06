#include "syntax_tree.h"

#include "iostream"
#include "string"
#include "sstream"
#include "cstdlib"
#include "vector"

#include "llvm/Analysis/Passes.h"
#include "llvm/ExecutionEngine/JIT.h" 
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Vectorize.h"

// ASTNode

using namespace std;
using namespace llvm;

Value *ErrorV(const char *str) { return 0; }

Module* the_module;
IRBuilder<> ir_builder(getGlobalContext());
std::map<std::string, Value*> named_values;
std::map<std::string, Value*> mutable_values;
FunctionPassManager* function_pass_manager;
PassManager* pass_manager;

ExecutionEngine* the_execution_engine;

string loop_varname[3] = {"i", "j", "k"};

typedef struct {
  BasicBlock* init;
  BasicBlock* cond;
  BasicBlock* update;

  Value* start;
  Value* end;
  string loop_var_id;

} FOR_LOOP_BB;

BasicBlock* GenerateForLoop(Function* function, 
                            const vector<Value*>& starts, 
                            const vector<Value*>& ends, 
                            const vector<string>& loop_var_ids, 
                            const vector<ASTNode*>& roots,
                            int depth) {

  cerr << "processing loop level " << depth << endl;

  // Generate the body of the most inner loop.
  if (depth >= loop_var_ids.size()) {
    Value* RetVal;
    for (int i = 0; i < roots.size(); ++i) {
      RetVal = roots[i]->IRCodeGen();
    }
    cerr << "leaving loop level " << depth << endl;
    return NULL;
  } else {
    // Read registries of the loop variables.
    Value* start = starts[depth];
    Value* end = ends[depth];
    string loop_var_id = loop_var_ids[depth]; 
    
    BasicBlock *PreheaderBB = ir_builder.GetInsertBlock();
    BasicBlock *LoopBB = BasicBlock::Create(getGlobalContext(),
                                            "loop",
                                            function);
    ir_builder.CreateBr(LoopBB);
    ir_builder.SetInsertPoint(LoopBB);
    PHINode *phi_variable = 
        ir_builder.CreatePHI(Type::getInt32Ty(getGlobalContext()),
                             2,
                             loop_var_id.c_str());
 
    phi_variable->addIncoming(start, PreheaderBB);

    Value* shadowed_loop_var;
    if (named_values.find(loop_var_id) != named_values.end()) {
      shadowed_loop_var = named_values[loop_var_id];
    } else {
      shadowed_loop_var = NULL;
    }
    named_values[loop_var_id] = phi_variable;

    GenerateForLoop(function, starts, ends, loop_var_ids, roots, depth + 1);

    Value* step_val;
    step_val = ConstantInt::get(getGlobalContext(), APInt(32, 1));

    Value* next_loop_var = ir_builder.CreateNSWAdd(phi_variable,
                                                   step_val,
                                                   "next_lop_var");

    Value* loop_cond_cmp = ir_builder.CreateICmpSLT(phi_variable,
                                                    ends[depth]);

    BasicBlock *LoopEndBB = ir_builder.GetInsertBlock();
    BasicBlock *AfterBB = 
        BasicBlock::Create(getGlobalContext(), "afterloop", function);

    // Insert the conditional branch into the end of LoopEndBB.
    ir_builder.CreateCondBr(loop_cond_cmp, LoopBB, AfterBB);

    // Any new code will be inserted in AfterBB.
    ir_builder.SetInsertPoint(AfterBB);
    phi_variable->addIncoming(next_loop_var, LoopEndBB);

    // Restore the unshadowed variable.
    if (shadowed_loop_var)
      named_values[loop_var_id] = shadowed_loop_var;
    else
      named_values.erase(loop_var_id);
    cerr << "leaving loop level " << depth << endl;
    return AfterBB;
  }
}

static AllocaInst* CreateEntryBlockAlloca(Function* function,
                                          const std::string &var_name) {
  IRBuilder<> ir(&function->getEntryBlock(),
                 function->getEntryBlock().begin());
  AllocaInst* v = ir.CreateAlloca(Type::getFloatTy(getGlobalContext()), 0,
                         var_name.c_str());
  return v;
}

static AllocaInst* CreateEntryBlockAllocaInt(Function* function,
                                          const std::string &var_name) {
  IRBuilder<> ir(&function->getEntryBlock(),
                 function->getEntryBlock().begin());
  AllocaInst* v = ir.CreateAlloca(Type::getInt32Ty(getGlobalContext()), 0,
                         var_name.c_str());
  return v;
}

vector<string> split(const std::string &s, char delim) {
  vector<std::string> elems;
  stringstream ss(s);
  string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

// StencilAST

void StencilAST::ParseFromSemanticTrees(const vector<SemanticTreeNode*>& sts) {
  for (int i = 0; i < sts.size(); ++i) {
    cerr << i << endl;
    ASTNode* root;
    root = ASTNode::ASTNodeFactory(sts[i]);
    root->ParseFromSemanticTreeNode(sts[i]);
    roots.push_back(root);
    StencilAST::CollectVariableInfo(root, &input_fields,
                                          &output_fields,
                                          &input_variables,
                                          &local_variables);

  }

  i_left = 0;
  i_right = 0;
  j_left = 0;
  j_right = 0;

  for (map<string, FieldInfo>::const_iterator it = input_fields.begin(); it != input_fields.end(); ++it) {
    if (it->second.top_margin > i_left) {
      i_left = it->second.top_margin;
    }
    if (it->second.bottom_margin > i_right) {
      i_right = it->second.bottom_margin;
    }
    if (it->second.left_margin > j_left) {
      j_left = it->second.left_margin;
    }
    if (it->second.right_margin > j_right) {
      j_right = it->second.right_margin;
    }
  }

  cerr << i_left << " " << i_right << " " << j_left << " " << j_right << endl;
  cerr << input_fields.size() << " " << output_fields.size() << " " <<
          input_variables.size() << " " << local_variables.size() << endl;
  for (map<string, FieldInfo>::const_iterator it = input_fields.begin(); it != input_fields.end(); ++it) {
    cerr << it->first << " : " << it->second.left_margin << " , " << it->second.right_margin << " , " 
         << it->second.top_margin << " , " << it->second.bottom_margin << endl;
  }

  for (map<string, FieldInfo>::const_iterator it = output_fields.begin(); it != output_fields.end(); ++it) {
    cerr << it->first << endl;
  }

  for (map<string, string>::const_iterator it = input_variables.begin(); it != input_variables.end(); ++it) {
    cerr << it->first << endl;
  }

  for (map<string, string>::const_iterator it = local_variables.begin(); it != local_variables.end(); ++it) {
    cerr << it->first << endl;
  }
}

void StencilAST::CollectVariableInfo(ASTNode* ast,
                                     map<string, FieldInfo>* input_fields,
                                     map<string, FieldInfo>* output_fields,
                                     map<string, string>* input_variables,
                                     map<string, string>* local_variables) {

  if (ast->get_node_type() == "ASTNodeExprInputField") {
    int cur_offset_x = ((ASTNodeExprInputField*)ast)->get_offset_x();
    int cur_offset_y = ((ASTNodeExprInputField*)ast)->get_offset_y();
    string id = ((ASTNodeExprInputField*)ast)->get_id();
    FieldInfo finfo;

    if (input_fields->find(id) == input_fields->end()) {
      finfo.left_margin = (cur_offset_x < 0) ? (-cur_offset_x) : 0;
      finfo.right_margin = (cur_offset_x > 0) ? (cur_offset_x) : 0;
      finfo.top_margin = (cur_offset_y < 0) ? (-cur_offset_y) : 0;
      finfo.bottom_margin = (cur_offset_y > 0) ? (cur_offset_y) : 0;
      (*input_fields)[id] = finfo;
    } else {
      finfo = (*input_fields)[id];
      if (cur_offset_x < 0) {         
        finfo.left_margin = (-cur_offset_x > finfo.left_margin) ?
                            (-cur_offset_x) :
                            finfo.left_margin;
      } else {
        finfo.right_margin = (cur_offset_x > finfo.right_margin) ?
                             (cur_offset_x) :
                             finfo.right_margin;
      }

      if (cur_offset_y < 0) {         
        finfo.top_margin = (-cur_offset_y > finfo.top_margin) ?
                           (-cur_offset_y) :
                           finfo.top_margin;
      } else {
        finfo.bottom_margin = (cur_offset_y > finfo.bottom_margin) ?
                              (cur_offset_y) :
                              finfo.bottom_margin;
      }
      (*input_fields)[id] = finfo;
    }

  } else if (ast->get_node_type() == "ASTNodeExprOutputField") {
    string id = ((ASTNodeExprOutputField*)ast)->get_id();
    FieldInfo finfo;
    (*output_fields)[id] = finfo;

  } else if (ast->get_node_type() == "ASTNodeExprInputVariable") {
    string id = ((ASTNodeExprInputVariable*)ast)->get_id();
    (*input_variables)[id] = "float";

  } else if (ast->get_node_type() == "ASTNodeExprLocalVariable") {
    string id = ((ASTNodeExprLocalVariable*)ast)->get_id();
    (*local_variables)[id] = "float";

  } else if (ast->get_node_type() == "ASTNodeExprBinary") {
    StencilAST::CollectVariableInfo(((ASTNodeExprBinary*)ast)->get_operand_o(), 
                                    input_fields, output_fields, input_variables,
                                    local_variables);
    StencilAST::CollectVariableInfo(((ASTNodeExprBinary*)ast)->get_operand_e(),
                                    input_fields, output_fields, input_variables,
                                    local_variables);

  } else if (ast->get_node_type() == "ASTNodeExprAssignment") {
    StencilAST::CollectVariableInfo(((ASTNodeExprAssignment*)ast)->get_l_value(),
                                    input_fields, output_fields, input_variables, 
                                    local_variables);
    StencilAST::CollectVariableInfo(((ASTNodeExprAssignment*)ast)->get_r_value(),
                                    input_fields, output_fields, input_variables,
                                    local_variables);
  }     
}

Function* StencilAST::IRCodeGen() {

  vector<string> field_args, scalar_args;
  Value* v;
  Function* F;

  named_values.clear();
  for (map<string, FieldInfo>::const_iterator it = input_fields.begin();
       it != input_fields.end();
       ++it) {
    field_args.push_back(it->first);
  }

  for (map<string, FieldInfo>::const_iterator it = output_fields.begin();
       it != output_fields.end();
       ++it) {
    field_args.push_back(it->first);
  }

  for (map<string, string>::const_iterator it = input_variables.begin();
       it != input_variables.end();
       ++it) {
    scalar_args.push_back(it->first);
  }

  vector<Type*> type_args;

  for (int i = 0; i < field_args.size(); ++i) {
    type_args.push_back(PointerType::get(Type::getFloatTy(getGlobalContext()), 0));
  }

  for (int i = 0; i < scalar_args.size(); ++i) {
    type_args.push_back(Type::getFloatTy(getGlobalContext()));
  }

  type_args.push_back(Type::getInt32Ty(getGlobalContext()));
  type_args.push_back(Type::getInt32Ty(getGlobalContext()));

 //// FunctionType *FT = FunctionType::get(PointerType::get(Type::getFloatTy(getGlobalContext()), 0),
 //                                      type_args, false);

  FunctionType *FT = FunctionType::get(Type::getVoidTy(getGlobalContext()), type_args, false);
  F = Function::Create(FT, Function::ExternalLinkage, "stencil", the_module);

  AttributeSet attr_set;
  attr_set = attr_set.addAttribute(getGlobalContext(), 1, llvm::Attribute::NoAlias);
  attr_set = attr_set.addAttribute(getGlobalContext(), 1, llvm::Attribute::NoCapture);
 // attr_set = attr_set.addAttribute(getGlobalContext(), 1,
 //                                  llvm::Attribute::ReadOnly);  
  Function::arg_iterator AI = F->arg_begin();
  for (int idx = 0; idx != field_args.size(); ++AI, ++idx) {
    AI->setName(field_args[idx]);
    AI->addAttr(attr_set);
    // Add arguments to variable symbol table.
    named_values[field_args[idx]] = AI;
  }

  for (int idx = 0; idx != scalar_args.size(); ++AI, ++idx) {
    AI->setName(scalar_args[idx]);
    
    // Add arguments to variable symbol table.
    named_values[scalar_args[idx]] = AI;
  }

   AI->setName("size_x");
    
   named_values["size_x"] = AI;

   ++AI;

   AI->setName("size_y");

   named_values["size_y"] = AI;
  
  // Create a new basic block to start insertion into.
  BasicBlock* entry = BasicBlock::Create(getGlobalContext(), "entry", F);
  ir_builder.SetInsertPoint(entry);

  for (map<string, string>::const_iterator it = local_variables.begin(); it != local_variables.end(); ++it) {
  //  cerr << it->first << endl;
    AllocaInst *Alloca = CreateEntryBlockAlloca(F, it->first);
//v->dump();
//Alloca->dump();
    ir_builder.CreateStore(ConstantFP::get(getGlobalContext(), APFloat((float)0.0)), Alloca);

    mutable_values[it->first] = Alloca;
  } 

//  AllocaInst *Alloca = CreateEntryBlockAllocaInt(F, "i");

//  ir_builder.CreateStore(ConstantInt::get(getGlobalContext(), APInt(32, 0)), Alloca);

//  mutable_values["i"] = Alloca;

//  Alloca = CreateEntryBlockAllocaInt(F, "j");

//  ir_builder.CreateStore(ConstantInt::get(getGlobalContext(), APInt(32, 0)), Alloca);

//  mutable_values["j"] = Alloca;

  Value* i_start = ir_builder.CreateAdd(ConstantInt::get(getGlobalContext(), APInt(32, 0)),
                                        ConstantInt::get(getGlobalContext(), APInt(32, i_left)));

  Value* i_end = ir_builder.CreateSub(named_values["size_y"],
                                      ConstantInt::get(getGlobalContext(), APInt(32, i_right)));

  Value* j_start = ir_builder.CreateAdd(ConstantInt::get(getGlobalContext(), APInt(32, 0)),
                                        ConstantInt::get(getGlobalContext(), APInt(32, j_left)));

  Value* j_end = ir_builder.CreateSub(named_values["size_x"],
                                      ConstantInt::get(getGlobalContext(), APInt(32, i_left)));


  vector<Value*> starts, ends;
  vector<string> loop_var_ids;

  starts.push_back(i_start);
  starts.push_back(j_start);
  ends.push_back(i_end);
  ends.push_back(j_end);

  loop_var_ids.push_back("i");
  loop_var_ids.push_back("j");
 
 /*
  vector<FOR_LOOP_BB> for_loop_bbs;
  BasicBlock* body = GenerateForLoopBB(F, starts, ends, loop_var_ids, 0, 2, &for_loop_bbs); 
  BasicBlock* after= BasicBlock::Create(getGlobalContext(), "loop_after", F);
  GenerateForLoop(F, for_loop_bbs, body, after); 

  ir_builder.SetInsertPoint(entry);
  ir_builder.CreateBr(for_loop_bbs[for_loop_bbs.size() - 1].init);

  ir_builder.SetInsertPoint(body);

  Value* RetVal;
  for (int i = 0; i < roots.size(); ++i) {
    RetVal = roots[i]->IRCodeGen();
  }
  ir_builder.CreateBr(for_loop_bbs[0].update);

  ir_builder.SetInsertPoint(after);

*/
  BasicBlock* after = GenerateForLoop(F, starts, ends, loop_var_ids, roots, 0);
//  F->dump(); 
  ir_builder.SetInsertPoint(after);
  ir_builder.CreateRetVoid();
//  F->dump(); 
  // Validate the generated code, checking for consistency.
  verifyFunction(*F);
//  function_pass_manager->run(*F);

  return F;
 // }
  
  // Error reading body, remove function.
 // F->eraseFromParent();
 // return 0;
/* 

  std::vector<Type*> Doubles(Args.size(),
                             Type::getDoubleTy(getGlobalContext()));
  FunctionType *FT = FunctionType::get(Type::getDoubleTy(getGlobalContext()),
                                       Doubles, false);
  
  Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);
  
  // If F conflicted, there was already something named 'Name'.  If it has a
  // body, don't allow redefinition or reextern.
  if (F->getName() != Name) {
    // Delete the one we just made and get the existing one.
    F->eraseFromParent();
    F = TheModule->getFunction(Name);
    
    // If F already has a body, reject this.
    if (!F->empty()) {
      ErrorF("redefinition of function");
      return 0;
    }
    
    // If F took a different number of args, reject.
    if (F->arg_size() != Args.size()) {
      ErrorF("redefinition of function with different # args");
      return 0;
    }
  }
  
  // Set names for all arguments.
  unsigned Idx = 0;
  for (Function::arg_iterator AI = F->arg_begin(); Idx != Args.size();
       ++AI, ++Idx) {
    AI->setName(Args[Idx]);
    
    // Add arguments to variable symbol table.
    NamedValues[Args[Idx]] = AI;
  }
  */
  return F;
}

// ASTNode
ASTNode::ASTNode() {
  node_type = "ASTNode";
}

ASTNode::~ASTNode() {
}

void ASTNode::ParseFromSemanticTreeNode(SemanticTreeNode* stn) {
}

void ASTNode::Print() {
  cerr << "AST node start" << endl;
  cerr << "node_type : " << node_type << endl;
  cerr << "AST node end" << endl;
}

Value* ASTNode::IRCodeGen() {
  cerr << get_node_type() << " code_gen" << endl; 
  return ConstantFP::get(getGlobalContext(), APFloat((float)2.0));
}

string ASTNode::get_node_type() {
  return node_type;
}

ASTNode* ASTNode::ASTNodeFactory(SemanticTreeNode* stn) {

  ASTNode* node;

  string id = stn->id;
  string op = stn->op;

  if (op == "=" && stn->children.size() == 2) {
    cerr << "creating assignment node" << endl;
    node = new ASTNodeExprAssignment();
  } else if ((op == "+" || op == "-" || op == "*" || op == "/") && stn->children.size() == 2) {
    cerr << "creating arithmetic node" << endl;
    node = new ASTNodeExprBinary();
  } else if (op == "" && id[0] == 'f') {
    cerr << "creating const node" << endl;
    node = new ASTNodeExprConst();
  } else if (op == "" && id[0] == 'O') {
    cerr << "creating output field node" << endl;
    node = new ASTNodeExprOutputField();
  } else if (op == "" && id[0] == 'I') {
    cerr << "creating input field node" << endl;
    node = new ASTNodeExprInputField();
  } else if (op == "" && id[0] == '$') {
    cerr << "creating local variable node" << endl;
    node = new ASTNodeExprLocalVariable();
  } else if (op == "" && id[0] == '@') {
    cerr << "creating input variable node" << endl;
    node = new ASTNodeExprInputVariable();
  } 

  return node;
}
// ASTNodeExpr
ASTNodeExpr::ASTNodeExpr() {
  node_type = "ASTNodeExpr";
}

ASTNodeExpr::~ASTNodeExpr() {
}

string ASTNodeExpr::get_node_type() {
  return node_type;
}

void ASTNodeExpr::ParseFromSemanticTreeNode(SemanticTreeNode* stn) {
}

Value* ASTNodeExpr::IRCodeGen() {
  cerr << get_node_type() << " code_gen" << endl; 
  return ConstantFP::get(getGlobalContext(), APFloat((float)2.0));
}

void ASTNodeExpr::Print() {
  cerr << "AST node start" << endl;
  cerr << "node_type : " << node_type << endl;
  cerr << "AST node end" << endl;
}

// ASTNodeExprConst
ASTNodeExprConst::ASTNodeExprConst() {
  node_type = "ASTNodeExprConst";
};

ASTNodeExprConst::~ASTNodeExprConst() {
}

string ASTNodeExprConst::get_node_type() {
  return node_type;
}

void ASTNodeExprConst::ParseFromSemanticTreeNode(SemanticTreeNode* stn) {
  string id = stn->id;
  if (id == "") {
    cout << "Syntax error : constant field is empty" << endl;
    return;
  }

  if (stn->children.size() != 0) {
    cout << "Parsing error : constant node is not a leaf" << endl;
    return;
  }
  
  // remove the "f" tag
  id.erase(0, 1);
  value = atof(id.c_str());
}

Value* ASTNodeExprConst::IRCodeGen() {
  cerr << get_node_type() << " code_gen" << endl; 
  return ConstantFP::get(getGlobalContext(), APFloat(value));
}


void ASTNodeExprConst::Print() {
  cerr << "AST node start" << endl;
  cerr << "node_type : " << node_type << endl;
  cerr << "value = " << value << endl;
  cerr << "AST node end" << endl;
}

// ASTNodeExprInputField

ASTNodeExprInputField::ASTNodeExprInputField() {
  node_type = "ASTNodeExprInputField";
}

ASTNodeExprInputField::~ASTNodeExprInputField() {
}

string ASTNodeExprInputField::get_node_type() {
  return node_type;
}

void ASTNodeExprInputField::ParseFromSemanticTreeNode(SemanticTreeNode* stn) {
  if (stn->id == "") {
    cout << "Syntax error : input field is empty" << endl;
    return;
  }

  if (stn->children.size() != 0) {
    cout << "Parsing error : input field is not a leaf" << endl;
    return;
  }

  vector<string> args = split(stn->id, ' ');

  id = args[0];

  int x = atoi(args[1].c_str());
  int y = atoi(args[2].c_str());

  offset_x = x;
  offset_y = y;
}

void ASTNodeExprInputField::Print() {
  cerr << "AST node start" << endl;
  cerr << "node_type : " << node_type << endl;
  cerr << "id = " << id << endl;
  cerr << "x offset = " << offset_x << endl;
  cerr << "y offset = " << offset_y << endl;
  cerr << "AST node end" << endl;
}

Value* ASTNodeExprInputField::IRCodeGen() {
  cerr << get_node_type() << " code_gen" << endl; 

  Value* size_x = named_values["size_x"];
//  Value* i = ir_builder.CreateLoad(mutable_values["i"]);
 Value* i = named_values["i"];
  Value* i_offset = ir_builder.CreateNSWAdd(i, ConstantInt::get(getGlobalContext(), APInt(32, offset_y, true)));
  Value* i_size_x = ir_builder.CreateMul(i_offset, size_x);
//  Value* j = ir_builder.CreateLoad(mutable_values["j"]);
 Value* j = named_values["j"];
  Value* j_offset = ir_builder.CreateNSWAdd(j, ConstantInt::get(getGlobalContext(), APInt(32, offset_x, true)));
  Value* i_size_x_j = ir_builder.CreateNSWAdd(i_size_x, j_offset);

  vector<Value*> vect;
  vect.push_back(i_size_x_j);
  Value* infield_addr = ir_builder.CreateInBoundsGEP(named_values[get_id()], ArrayRef<Value *>(vect),
                                                                     get_id() + "_addr");
  Value* infield_val = ir_builder.CreateLoad(infield_addr, get_id() + "_value");
  return infield_val;
}

// ASNodeExprOutputField

ASTNodeExprOutputField::ASTNodeExprOutputField() {
  node_type = "ASTNodeExprOutputField";
}

ASTNodeExprOutputField::~ASTNodeExprOutputField() {
}

string ASTNodeExprOutputField::get_node_type() {
  return node_type;
}

void ASTNodeExprOutputField::ParseFromSemanticTreeNode(SemanticTreeNode* stn) {

  if (stn->id == "") {
    cout << "Syntax error : output field is empty" << endl;
    return;
  }

  if (stn->children.size() != 0) {
    cout << "Parsing error : output field node is not a leaf" << endl;
    return;
  }

  if (stn->id[0] != 'O') {
    cout << "Syntax error : illegal output field" << endl;
  }

  id = stn->id;
}

Value* ASTNodeExprOutputField::IRCodeGen() {
  cerr << get_node_type() << " code_gen" << endl; 
  
  Value* size_x = named_values["size_x"];
 // Value* i = ir_builder.CreateLoad(mutable_values["i"]);
  Value* i = named_values["i"];
  Value* i_size_x = ir_builder.CreateMul(i, size_x);
//  Value* j = ir_builder.CreateLoad(mutable_values["j"]);
  Value* j = named_values["j"];
  Value* i_size_x_j = ir_builder.CreateNSWAdd(i_size_x, j);
  vector<llvm::Value*> vect;
    vect.push_back(i_size_x_j);

  named_values[get_id()]->dump();
  ArrayRef<Value*> array_ref(vect);
  Value* addr = ir_builder.CreateInBoundsGEP(named_values[get_id()], array_ref, get_id() + "_addr");
  return addr;
}

void ASTNodeExprOutputField::Print() {
  cerr << "AST node start" << endl;
  cerr << "node_type : " << node_type << endl;
  cerr << "id = " << id << endl;
  cerr << "AST node end" << endl;
}

// ASTNodeExprInputVariable

ASTNodeExprInputVariable::ASTNodeExprInputVariable() {
  node_type = "ASTNodeExprInputVariable";
};

ASTNodeExprInputVariable::~ASTNodeExprInputVariable() {
}

string ASTNodeExprInputVariable::get_node_type() {
  return node_type;
}

void ASTNodeExprInputVariable::ParseFromSemanticTreeNode(SemanticTreeNode* stn) {
  if (stn->id == "") {
    cout << "Syntax error : input variable is empty" << endl;
    return;
  }

  if (stn->children.size() != 0) {
    cout << "Parsing error : input variable node is not a leaf" << endl;
    return;
  }

  if (stn->id[0] != '@') {
    cout << "Syntax error : illegal input variable" << endl;
  }
 
  id = stn->id;
  id.erase(0, 1);

 // cerr << " debug " << stn->id << endl;
}

Value* ASTNodeExprInputVariable::IRCodeGen() {
  cerr << get_node_type() << " code_gen" << endl; 
 // cerr << get_id() << endl;
 // named_values[get_id()]->dump();
  return named_values[get_id()];
}

void ASTNodeExprInputVariable::Print() {
  cerr << "AST node start" << endl;
  cerr << "node_type : " << node_type << endl;
  cerr << "id = " << id << endl;
  cerr << "AST node end" << endl;
}

// ASTNodeExprLocalVariable

ASTNodeExprLocalVariable::ASTNodeExprLocalVariable() {
  node_type = "ASTNodeExprLocalVariable";
};

ASTNodeExprLocalVariable::~ASTNodeExprLocalVariable() {
}

string ASTNodeExprLocalVariable::get_node_type() {
  return node_type;
}

void ASTNodeExprLocalVariable::ParseFromSemanticTreeNode(SemanticTreeNode* stn) {
  if (stn->id == "") {
    cout << "Syntax error : local variable is empty" << endl;
    return;
  }

  if (stn->children.size() != 0) {
    cout << "Parsing error : local variable node is not a leaf" << endl;
    return;
  }

  if (stn->id[0] != '$') {
    cout << "Syntax error : illegal local variable" <<  stn->id << endl;
  }

  id = stn->id;
  id.erase(0, 1);
}

Value* ASTNodeExprLocalVariable::IRCodeGen() {
  cerr << get_node_type() << " code_gen" << endl; 
  
  return named_values[get_id()];
}

void ASTNodeExprLocalVariable::Print() {
  cerr << "AST node start" << endl;
  cerr << "node_type : " << node_type << endl;
  cerr << "id = " << id << endl;
  cerr << "AST node end" << endl;
}

// ASTNodeExprBinary

ASTNodeExprBinary::ASTNodeExprBinary() {
  node_type = "ASTNodeExprBinary";
}

ASTNodeExprBinary::~ASTNodeExprBinary() {
}

string ASTNodeExprBinary::get_node_type() {
  return node_type;
}

void ASTNodeExprBinary::ParseFromSemanticTreeNode(SemanticTreeNode* stn) {
  cerr << "Parsing binary node" << endl;
  string str_op = stn->op;

  if (str_op == "+" && stn->children.size() == 2) {
    op = ADD;
  } else if (str_op == "-" && stn->children.size() == 2) {
    op = SUB;
  } else if (str_op == "*" && stn->children.size() == 2) {
    op = MUL;
  } else if (str_op == "/" && stn->children.size() == 2) {
    op = DIV;
  } else {
    cerr << "Unrecognized binary operator" << endl;
    return;
  }
  
  operand_o = (ASTNodeExpr*)ASTNode::ASTNodeFactory(stn->children[0]);
  operand_e = (ASTNodeExpr*)ASTNode::ASTNodeFactory(stn->children[1]);

  operand_o->ParseFromSemanticTreeNode(stn->children[0]);
  operand_e->ParseFromSemanticTreeNode(stn->children[1]);
  
}

Value* ASTNodeExprBinary::IRCodeGen() {
  cerr << get_node_type() << " code_gen" << endl; 
  Value* l = operand_o->IRCodeGen();
  Value* r = operand_e->IRCodeGen();

   if (op == ADD) {
    return ir_builder.CreateFAdd(l, r, "addtmp");
  } else if (op == SUB) {
    return ir_builder.CreateFSub(l, r, "subtmp");
  } else if (op == MUL) {
    return ir_builder.CreateFMul(l, r, "multmp");
  } else if (op == DIV) {
    return ir_builder.CreateFDiv(l, r, "divtmp");
  }
  return nullptr;
}

void ASTNodeExprBinary::Print() {
  cerr << "AST node start" << endl;
  cerr << "node_type : " << node_type << endl;
  if (op == ADD) {
    cerr << "op = ADD" << endl;
  } else if (op == SUB) {
    cerr << "op = SUB" << endl;
  } else if (op == MUL) {
    cerr << "op = MUL" << endl;
  } else if (op == DIV) {
    cerr << "op = DIV" << endl;
  }

  cerr << "AST node end" << endl; 
}
// ASTNodeExprAssignment

ASTNodeExprAssignment::ASTNodeExprAssignment() {
  node_type = "ASTNodeExprAssignment";
}

ASTNodeExprAssignment::~ASTNodeExprAssignment() {
}

string ASTNodeExprAssignment::get_node_type() {
  return node_type;
}

void ASTNodeExprAssignment::ParseFromSemanticTreeNode(SemanticTreeNode* stn) {
  cerr << "Parsing assignment node" << endl;
  string op = stn->op;

  if (op != "=") {
    cout << "Syntax error : incorrect assignment operator" << endl;
    return;
  }

  if (stn->children.size() != 2) {
    cout << "Parsing error : (assignment) incorrect operand number" << endl;
    return;
  }

  l_value = (ASTNodeExpr*)ASTNode::ASTNodeFactory(stn->children[0]);
  r_value = (ASTNodeExpr*)ASTNode::ASTNodeFactory(stn->children[1]);

  cerr << l_value->get_node_type() << endl;
  cerr << r_value->get_node_type() << endl;

  l_value->ParseFromSemanticTreeNode(stn->children[0]);
  r_value->ParseFromSemanticTreeNode(stn->children[1]);
}

Value* ASTNodeExprAssignment::IRCodeGen() {
  Value* left;
  Value* right;

  cerr << get_node_type() << " code_gen" << endl; 
  right = r_value->IRCodeGen();
  left = l_value->IRCodeGen();

  if (l_value->get_node_type() == "ASTNodeExprOutputField") {
    left->dump();
  
    StoreInst *si = ir_builder.CreateStore(right, left);		
  } else if (l_value->get_node_type() == "ASTNodeExprLocalVariable") {
    named_values[((ASTNodeExprLocalVariable*)l_value)->get_id()] = right;
  }
  return right;
}

void ASTNodeExprAssignment::Print() {
  cerr << "AST node start" << endl;
  cerr << "node_type : " << node_type << endl;
  l_value->Print();
  r_value->Print();
  cerr << "AST node end" << endl; 
}
