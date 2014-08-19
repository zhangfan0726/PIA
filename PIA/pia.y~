%require "2.4.1"
%skeleton "lalr1.cc"
%defines
%define api.namespace{PIA}
%define parser_class_name{BisonParser}
%parse-param {PIA::FlexScanner &scanner}
%lex-param   {PIA::FlexScanner &scanner}

%code requires {
  #include "iostream"
  #include "cstdio"
  #include "cstring"
  #include "string"
  #include "sstream"

  #include "variable_table.h"
  #include "semantic_tree.h"

  using namespace std;
  // Forward-declare the Scanner class; the Parser needs to be assigned a 
  // Scanner, but the Scanner can't be declared without the Parser
  namespace PIA {
    class FlexScanner;
  }
}

%code {
  // Prototype for the yylex function
  static int yylex(PIA::BisonParser::semantic_type * yylval, PIA::FlexScanner &scanner);
  //  #define YY_DECL \
  //  yylex (C::BisonParser::semantic_type * yylval, C::FlexScanner &scanner)
}

%token IDENTIFIER STRING_LITERAL SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN TYPE_NAME

%token TYPEDEF EXTERN STATIC AUTO REGISTER
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token <string_value> CONSTANT CONSTANT_INT_DEC CONSTANT_INT_HEX

%token <string_value> INPUT_FIELD OUTPUT_FIELD INPUT_VARIABLE LOCAL_VARIABLE

%token START_STENCIL END_STENCIL

%type <semantic_tree_node> stencil_primary_expression input_field stencil_expression 
                           stencil_mul_expression stencil_rval stencil_statement

%type <string_value> stencil_offset stencil_lval

%start translation_unit

%union {
  int int_value;
  float float_value;
  char* string_value;
  ID_REG id_reg;
  SemanticTreeNode* semantic_tree_node;
}

%%

translation_unit
	: START_STENCIL '(' IDENTIFIER ')'  stencil_statements  END_STENCIL { for (int i = 0; i < semantic_trees.size(); ++i) {
                                                                                PrintSemanticTree(semantic_trees[i]);
                                                                              }
                                                                            }
        ;

stencil_statements
	: stencil_statements stencil_statement { semantic_trees.push_back($2); }
	| stencil_statement { semantic_trees.push_back($1); }
        ;

stencil_statement
	: stencil_lval '=' stencil_rval ';' { ostringstream ss;
                                              ss << $1;
                                              vector<SemanticTreeNode*> children;
                                              children.push_back(MakeSemanticTree(ss.str(), ""));
                                              children.push_back($3);
                                              SemanticTreeNode* node = MakeSemanticTree("", "=", children);
                                            //  PrintSemanticTree(node);
                                              $$ = node;
                                            }
	;

stencil_lval
	: LOCAL_VARIABLE { $$ = $1;
                         }

	| OUTPUT_FIELD {  $$ = $1;
                       }
	;

stencil_rval
	: stencil_expression { $$ = $1; }
	;

input_field 
	: INPUT_FIELD '[' stencil_offset ',' stencil_offset ']' { ostringstream ss;
                                                                  ss << $1 << ' ' << $3 << ' ' << $5;
                                                                  SemanticTreeNode* node = MakeSemanticTree(ss.str(), "");
                                                               //   cerr << "input field start" << endl;
                                                               //   PrintSemanticTree(node);
                                                               //   cerr << "input field end" << endl;
                                                                  $$ = node;
                                                                }
	;

stencil_offset
	: '-' CONSTANT_INT_DEC { 
                                 string a;
                                 ostringstream ss;
                                 ss << '-' << $2;
                                 a = ss.str();
                                 char* id = new char[a.size()];
                                 strcpy(id, a.c_str());
                                 $$ = id;
                               }

	| CONSTANT_INT_DEC     { string a;
                                 ostringstream ss;
                                 ss << $1;
                                 a = ss.str();
                                 char* dest_id = new char[a.size()];
                                 strcpy(dest_id, a.c_str());
                                 $$ =  dest_id; 
                               }
	;

stencil_expression
	: stencil_expression '+' stencil_mul_expression { 
                                                          vector<SemanticTreeNode*> children;
                                                          children.push_back($1);
                                                          children.push_back($3);
                                                          SemanticTreeNode* node = 
                                                              MakeSemanticTree("", "+", children);
                                                     //     PrintSemanticTree(node);
                                                          $$ = node;
                                                        }
	| stencil_expression '-' stencil_mul_expression { 
                                                          vector<SemanticTreeNode*> children;
                                                          children.push_back($1);
                                                          children.push_back($3);
                                                          SemanticTreeNode* node = 
                                                              MakeSemanticTree("", "-", children);
                                                      //    PrintSemanticTree(node);
                                                          $$ = node;
                                                        }

        | stencil_mul_expression { $$ = $1; }
	;

stencil_mul_expression
	: stencil_mul_expression '*' stencil_primary_expression {
                                                                  vector<SemanticTreeNode*> children;
                                                                  children.push_back($1);
                                                                  children.push_back($3);
                                                                  SemanticTreeNode* node = 
                                                                      MakeSemanticTree("", "*", children);
                                                           //       PrintSemanticTree(node);
                                                                  $$ = node;
                                                                }
                                                                   
	| stencil_mul_expression '/' stencil_primary_expression { 
                                                                  vector<SemanticTreeNode*> children;
                                                                  children.push_back($1);
                                                                  children.push_back($3);
                                                                  SemanticTreeNode* node = 
                                                                      MakeSemanticTree("", "/", children);
                                                              //    PrintSemanticTree(node);
                                                                  $$ = node;
                                                                }
        | stencil_primary_expression { $$ = $1; }
	;

stencil_primary_expression
	: '(' stencil_expression ')'{ $$ = $2; }
	| CONSTANT { string str = "f";
                     str += $1;
                     SemanticTreeNode* node = MakeSemanticTree(str, "");
                   //  PrintSemanticTree(node);
                     $$ = node;
                   }

	| input_field {
                    //    string dest_id = "";
                    //    string op = "";
                    //    vector<SemanticTreeNode*> children;
                    //    children.push_back($1);
                    //    SemanticTreeNode* node = MakeSemanticTree(dest_id, op, children);
                    //    PrintSemanticTree(node);
                        $$ = $1;
                      }
                      
	| LOCAL_VARIABLE {
                           SemanticTreeNode* node = MakeSemanticTree($1, "");
                        //   PrintSemanticTree(node);
                           $$ = node;
                         }

	| INPUT_VARIABLE {
                           SemanticTreeNode* node = MakeSemanticTree($1, "");
                        // PrintSemanticTree(node);
                           $$ = node;
                         }
	;

%%
#include "iostream"
#include "PIAScanner.h"

using namespace std;

void PIA::BisonParser::error(const std::string &msg) {
	std::cerr << "Error: " << msg << std::endl;
}

static int yylex(PIA::BisonParser::semantic_type * yylval, PIA::FlexScanner &scanner) {
  return scanner.yylex(yylval);
}
