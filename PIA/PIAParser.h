#pragma once

#include "pia.tab.hh"

namespace PIA {
  class Parser {
   public:
    Parser() : parser(scanner) {}

    int set_yyin(istream* input) { scanner.set_yyin(input); }
		
    int parse() {
      return parser.parse();
    }

    void error(const std::string &msg);
		
    private:
      PIA::FlexScanner scanner;
      PIA::BisonParser parser;
  };
}
