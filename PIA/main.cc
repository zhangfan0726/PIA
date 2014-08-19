#include "PIAScanner.h"
#include "PIAParser.h"
#include "variable_table.h"
#include "syntax_tree.h"
#include "iostream"
#include "fstream"

#include "llvm/Pass.h"
#include "llvm/PassManager.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/Dominators.h"

#include "llvm/Transforms/Vectorize.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/Debug.h"

using namespace std;

extern int yylex(void);
extern FILE* yyin;

int main(int argc, char * argv[]) {
  llvm::DebugFlag = true;

  LLVMContext &Context = getGlobalContext();

  // Make the module, which holds all the code.
  the_module = new Module("Position Independent Arithmetic", Context);

  the_module->setDataLayout("e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:"
                "64-v96:128-v192:256-v256:256-v512:512-v1024:1024");
  the_module->setTargetTriple("spir-unknown-unknown-unknown");

  string str_err;
  ifstream source_file(argv[1], ios::in);
  raw_fd_ostream ofs_ll(argv[2], str_err, llvm::sys::fs::F_RW);
  raw_fd_ostream ofs_bc(argv[3], str_err, llvm::sys::fs::F_RW);

  PIA::Parser parser;
  parser.set_yyin(&source_file);
  parser.parse();

  FunctionPassManager fpasses(the_module);
  PassManager passes;
/*  
  // function passes
  // ConstantPropagation - A worklist driven constant propagation pass
  OurFPM.add(createConstantPropagationPass());
  // SCCP - Sparse conditional constant propagation.
  OurFPM.add(createSCCPPass());

  OurFPM.add(createBasicAliasAnalysisPass());

  OurFPM.add(createInstructionCombiningPass());
  
  OurFPM.add(createReassociatePass());
  // Eliminate Common SubExpressions.
  OurFPM.add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  OurFPM.add(createCFGSimplificationPass());
  OurFPM.add(createFlattenCFGPass());

  // OurFPM.add(createLoopStrengthReducePass());

  // OurFPM.add(createLoopIdiomPass());
  // OurFPM.add(createLoopUnswitchPass());
  // OurFPM.add(createLoopUnrollPass(2, 2, 2, 2));
 //  OurFPM.add(createLoopSimplifyPass());
 //  OurFPM.add(createDemoteRegisterToMemoryPass());

  // struct VectorizeConfig vconf;
  // vconf.VectorizeFloats = true;

 // OurFPM.add(createSLPVectorizerPass());

  // module passes
  // DeadInstElimination - This pass quickly removes trivially dead instructions
  // without modifying the CFG of the function.  It is a BasicBlockPass, so it
  // runs efficiently when queued next to other BasicBlockPass's.
  passes.add(createDeadInstEliminationPass());

  // LICM - This pass is a loop invariant code motion and memory promotion pass.
  passes.add(new DataLayoutPass(the_module));
  passes.add(createLICMPass());
  passes.add(createLoopRotatePass());
  passes.add(createLoopVectorizePass());
  passes.add(createLoopUnrollPass(-1, -1, -1, -1));
  passes.add(createBBVectorizePass());
  passes.add(createSLPVectorizerPass());
  OurFPM.doInitialization();
*/
  function_pass_manager = &fpasses;

  StencilAST stencil_ast;
  stencil_ast.ParseFromSemanticTrees(semantic_trees);
  stencil_ast.IRCodeGen();

  const DataLayout *DL = the_module->getDataLayout();
  if (!DL) {
   cerr << " layout empty" << endl;
  }
 // passes.run(*the_module);

 //   llvm::LoopInfo* lp = new LoopInfo;
 //   fpasses.add(new DominatorTreeWrapperPass());
 //   fpasses.doInitialization();
  //  loop_passes.doInitialization();
  for (llvm::Module::FunctionListType::iterator fit = 
           the_module->getFunctionList().begin(); 
       fit != the_module->getFunctionList().end(); ++fit) {
    fit->dump();
  //  fpasses.run(*fit);
  //  lp->print(ofs_ll, the_module);
    for (llvm::Function::BasicBlockListType::const_iterator bit =
             fit->getBasicBlockList().begin();
         bit != fit->getBasicBlockList().end(); ++bit) {
      cerr << "basic block dump" << endl;
   //   cerr << lp->getLoopDepth(bit) << endl;
      bit->dump();
    }
    
  }

//  the_module->dump();

  AssemblyAnnotationWriter aaw;
  the_module->print(ofs_ll, &aaw);
  ofs_ll.close();

  WriteBitcodeToFile(the_module, ofs_bc);
  ofs_bc.close();

  string backend_cmd = "";
  backend_cmd += "ti_utils/cl6x -mv6600 --abi=eabi -O3 -s ";
  backend_cmd += argv[3];
  system(backend_cmd.c_str());
  // Print out all of the generated code.
  // the_module->dump();

  return 0;
  //SemanticTree2SyntaxTree(semantic_trees, &syntax_trees);
}

