CXX = g++
CXXOPT = -g -std=c++11
LIBS = -lpthread
#LLVM_OPT = -I/usr/local/include  -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -L/usr/local/lib -lLLVMCore -lLLVMSupport 

LLVM_OPT = `llvm-config --cppflags --ldflags --libs core jit native vectorize bitwriter analysis` -ldl
all: pia_compiler

bison_out : pia.y
	bison -d pia.y -o pia.tab.cc

flex_out : pia.l PIAScanner.h bison_out
	flex pia.l 

lex.yy.o : lex.yy.cc flex_out
	$(CXX) $(CXXOPT) -c lex.yy.cc -o lex.yy.o $(LLVM_OPT) 

pia.tab.o : pia.tab.cc bison_out
	$(CXX) $(CXXOPT) -c pia.tab.cc -o pia.tab.o $(LLVM_OPT) 

semantic_tree.o : semantic_tree.cc semantic_tree.h
	$(CXX) $(CXXOPT) -c semantic_tree.cc -o semantic_tree.o $(LLVM_OPT) 

variable_table.o : variable_table.cc variable_table.h
	$(CXX) $(CXXOPT) -c variable_table.cc -o variable_table.o $(LLVM_OPT) 

syntax_tree.o : syntax_tree.cc syntax_tree.h
	$(CXX) $(CXXOPT) -g -O3 -c syntax_tree.cc -o syntax_tree.o $(LIBS) $(LLVM_OPT)
main.o : main.cc bison_out
	$(CXX) $(CXXOPT) -c main.cc -o main.o $(LLVM_OPT) 

pia_compiler : main.o bison_out flex_out pia.tab.o lex.yy.o variable_table.o semantic_tree.o syntax_tree.o
	$(CXX) $(CXXOPT) lex.yy.o pia.tab.o main.o variable_table.o semantic_tree.o syntax_tree.o -o pia_compiler $(LIBS) $(LLVM_OPT) 
	
clean:
	rm -rf *.tab.c *.tab.h location.hh position.hh stack.hh
	rm -rf *.yy.cc *.tab.cc *.tab.hh *.o
	rm -rf pia_compiler
