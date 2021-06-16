#ifndef SYSY_LEXER_H_
#define SYSY_LEXER_H_

#include <istream>
#include <string>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>

#include "token.hpp"
using namespace std;

class Lexer {
 public:
  istream &in_;
  Lexer(istream &in) : in_(in) {
      in_ >> noskipws; //not skip the whitespace
      NextChar();
  }

  // return the next token
  Token Next_Token();

  // visitor
  char LastChar(){return last_char_;}
  string IdVal() {return id_val_; }
  string StrVal() {return str_val_; }
  int IntVal() {return int_val_; }
  Keyword KeyVal() {return key_val_; }
  Operator OpVal() {return op_val_; }
  Pair PairVal() {return pair_val_; }
  char CharVal() {return char_val_; }
  char OtherVal() {return other_val_; }
  int LineNum() {return line_; }

 private:
  //get next char
  void NextChar() {
    in_ >> last_char_;
    if(last_char_ == '\n') ++line_;
  }
  
  //Handle different tokens. When return, the last_char_ should point to the next char
  Token HandleID();
  Token HandleComment(string prefix_);
  Token HandleNumber();
  Token HandleString();
  Token HandleChar();
  Token HandleOperator();
  
  char last_char_;
  string id_val_;
  string str_val_;
  int int_val_;
  Keyword key_val_;
  Operator op_val_;
  Pair pair_val_;
  char char_val_;
  char other_val_;
  int line_ = 1; //line number
};

#endif  // FIRSTSTEP_FRONT_LEXER_H_
