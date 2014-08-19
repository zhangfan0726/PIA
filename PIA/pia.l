%{
  #include "pia.tab.hh"
  #include "PIAScanner.h"
  #include "variable_table.h"
  #include <cassert>
  #include <sstream>
  #include <cstring>
      
  using namespace std;
  using namespace PIA;

  void count();
  void comment();
  int check_type();

  #undef YY_DECL
  #define YY_DECL int PIA::FlexScanner::yylex()

%}

%option nodefault yyclass="FlexScanner" noyywrap c++

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%%
"/*"		       { comment(); }

"I"{D}+                { count();  
                         yylval->string_value = new char[strlen(yytext)];
                         strcpy(yylval->string_value, yytext); 
                         return(BisonParser::token::INPUT_FIELD);
                       }

"O"{D}+                { count();                          
                         yylval->string_value = new char[strlen(yytext)];
                         strcpy(yylval->string_value, yytext);  
                         return(BisonParser::token::OUTPUT_FIELD);
                       }

"@"{L}({L}|{D})*       { count(); 
                         yylval->string_value = new char[strlen(yytext)];
                         strcpy(yylval->string_value, yytext);
                         return(BisonParser::token::INPUT_VARIABLE); }

"$"{L}({L}|{D})*       { count(); 
                         yylval->string_value = new char[strlen(yytext)];
                         strcpy(yylval->string_value, yytext);
                         return(BisonParser::token::LOCAL_VARIABLE); 
                       }

"STENCIL"              { count(); return(BisonParser::token::START_STENCIL);}
"END"                  { count(); return(BisonParser::token::END_STENCIL);}

"auto"			{ count(); return(BisonParser::token::AUTO); }
"break"			{ count(); return(BisonParser::token::BREAK); }
"case"			{ count(); return(BisonParser::token::CASE); }
"char"			{ count(); return(BisonParser::token::CHAR); }
"const"			{ count(); return(BisonParser::token::CONST); }
"continue"		{ count(); return(BisonParser::token::CONTINUE); }
"default"		{ count(); return(BisonParser::token::DEFAULT); }
"do"			{ count(); return(BisonParser::token::DO); }
"double"		{ count(); return(BisonParser::token::DOUBLE); }
"else"			{ count(); return(BisonParser::token::ELSE); }
"enum"			{ count(); return(BisonParser::token::ENUM); }
"extern"		{ count(); return(BisonParser::token::EXTERN); }
"float"			{ count(); return(BisonParser::token::FLOAT); }
"for"			{ count(); return(BisonParser::token::FOR); }
"goto"			{ count(); return(BisonParser::token::GOTO); }
"if"			{ count(); return(BisonParser::token::IF); }
"int"			{ count(); return(BisonParser::token::INT); }
"long"			{ count(); return(BisonParser::token::LONG); }
"register"		{ count(); return(BisonParser::token::REGISTER); }
"return"		{ count(); return(BisonParser::token::RETURN); }
"short"			{ count(); return(BisonParser::token::SHORT); }
"signed"		{ count(); return(BisonParser::token::SIGNED); }
"sizeof"		{ count(); return(BisonParser::token::SIZEOF); }
"static"		{ count(); return(BisonParser::token::STATIC); }
"struct"		{ count(); return(BisonParser::token::STRUCT); }
"switch"		{ count(); return(BisonParser::token::SWITCH); }
"typedef"		{ count(); return(BisonParser::token::TYPEDEF); }
"union"			{ count(); return(BisonParser::token::UNION); }
"unsigned"		{ count(); return(BisonParser::token::UNSIGNED); }
"void"			{ count(); return(BisonParser::token::VOID); }
"volatile"		{ count(); return(BisonParser::token::VOLATILE); }
"while"			{ count(); return(BisonParser::token::WHILE); }

{L}({L}|{D})*		{ count(); return(check_type()); }

0[xX]{H}+{IS}?		{ count(); 
                          yylval->string_value = new char[strlen(yytext)];
                          strcpy(yylval->string_value, yytext);
                          return(BisonParser::token::CONSTANT_INT_HEX); 
                        }

0{D}+{IS}?		{ count(); return(BisonParser::token::CONSTANT_INT_DEC); }

{D}+{IS}?		{ count(); 
                          yylval->string_value = new char[strlen(yytext)];
                          strcpy(yylval->string_value, yytext);
                          return(BisonParser::token::CONSTANT_INT_DEC);
                        }

L?'(\\.|[^\\'])+'	{ count(); return(BisonParser::token::CONSTANT); }

{D}+{E}{FS}?		{ count();
                          yylval->string_value = new char[strlen(yytext)];
                          strcpy(yylval->string_value, yytext); 
                          return(BisonParser::token::CONSTANT); }
{D}*"."{D}+({E})?{FS}?	{ count();
                          yylval->string_value = new char[strlen(yytext)];
                          strcpy(yylval->string_value, yytext);
                          return(BisonParser::token::CONSTANT); }

{D}+"."{D}*({E})?{FS}?	{ count(); 
                          yylval->string_value = new char[strlen(yytext)];
                          strcpy(yylval->string_value, yytext);
                          return(BisonParser::token::CONSTANT); }

L?\"(\\.|[^\\"])*\"	{ count(); return(BisonParser::token::STRING_LITERAL); }

"..."			{ count(); return(BisonParser::token::ELLIPSIS); }
">>="			{ count(); return(BisonParser::token::RIGHT_ASSIGN); }
"<<="			{ count(); return(BisonParser::token::LEFT_ASSIGN); }
"+="			{ count(); return(BisonParser::token::ADD_ASSIGN); }
"-="			{ count(); return(BisonParser::token::SUB_ASSIGN); }
"*="			{ count(); return(BisonParser::token::MUL_ASSIGN); }
"/="			{ count(); return(BisonParser::token::DIV_ASSIGN); }
"%="			{ count(); return(BisonParser::token::MOD_ASSIGN); }
"&="			{ count(); return(BisonParser::token::AND_ASSIGN); }
"^="			{ count(); return(BisonParser::token::XOR_ASSIGN); }
"|="			{ count(); return(BisonParser::token::OR_ASSIGN); }
">>"			{ count(); return(BisonParser::token::RIGHT_OP); }
"<<"			{ count(); return(BisonParser::token::LEFT_OP); }
"++"			{ count(); return(BisonParser::token::INC_OP); }
"--"			{ count(); return(BisonParser::token::DEC_OP); }
"->"			{ count(); return(BisonParser::token::PTR_OP); }
"&&"			{ count(); return(BisonParser::token::AND_OP); }
"||"			{ count(); return(BisonParser::token::OR_OP); }
"<="			{ count(); return(BisonParser::token::LE_OP); }
">="			{ count(); return(BisonParser::token::GE_OP); }
"=="			{ count(); return(BisonParser::token::EQ_OP); }
"!="			{ count(); return(BisonParser::token::NE_OP); }
";"			{ count(); return(';'); }
("{"|"<%")		{ count(); return('{'); }
("}"|"%>")		{ count(); return('}'); }
","			{ count(); return(','); }
":"			{ count(); return(':'); }
"="			{ count(); return('='); }
"("			{ count(); return('('); }
")"			{ count(); return(')'); }
("["|"<:")		{ count(); return('['); }
("]"|":>")		{ count(); return(']'); }
"."			{ count(); return('.'); }
"&"			{ count(); return('&'); }
"!"			{ count(); return('!'); }
"~"			{ count(); return('~'); }
"-"			{ count(); return('-'); }
"+"			{ count(); return('+'); }
"*"			{ count(); return('*'); }
"/"			{ count(); return('/'); }
"%"			{ count(); return('%'); }
"<"			{ count(); return('<'); }
">"			{ count(); return('>'); }
"^"			{ count(); return('^'); }
"|"			{ count(); return('|'); }
"?"			{ count(); return('?'); }

[ \t\v\n\f]		{ count(); }
.			{ /* ignore bad characters */ }

%%

void comment() {

}


int column = 0;

void count()
{

}


int check_type()
{
/*
* pseudo code --- this is what it should check
*
*	if (yytext == type_name)
*		return(TYPE_NAME);
*
*	return(IDENTIFIER);
*/

/*
*	it actually will only return IDENTIFIER
*/

	return(BisonParser::token::IDENTIFIER);
}
