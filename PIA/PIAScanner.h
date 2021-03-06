#pragma once

// Only include FlexLexer.h if it hasn't been already included
#if ! defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "pia.tab.hh"

// Override the interface for yylex since we namespaced it

// Include Bison for types / tokens

using namespace PIA;

namespace PIA {
 class FlexScanner : public yyFlexLexer {
  public:
  // save the pointer to yylval so we can change it, and invoke scanner
  int yylex(PIA::BisonParser::semantic_type * lval) { yylval = lval; return yylex(); }

  void set_yyin(istream* input) { yyin = input; } 
		
  private:
	// Scanning function created by Flex; make this private to force usage
	// of the overloaded method so we can get a pointer to Bison's yylval
  int yylex();
			
	// point to yylval (provided by Bison in overloaded yylex)
  PIA::BisonParser::semantic_type *yylval;
  int nextColumn;
                        
 };
}
void strlcpy( char* dest, const char* src, unsigned int length );
long long unsigned int binaryToUint( const std::string& string );
