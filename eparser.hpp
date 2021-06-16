#ifndef EEYORE_LEXER_H_
#define EEYORE_LEXER_H_

#include <istream>
#include <string>
#include <cstddef>
#include <sstream>
#include <unordered_map>

#include "token.hpp"
#include "eeyore.hpp"
#include "global.hpp"
#include "cfg.hpp"
using namespace std;

class EParser {
 public:
  stringstream& in_;
  EParser(stringstream &in, GlobalManager& _gm, FuncSet& _fs) : 
  in_(in), gm_(_gm), fs_(_fs){
      in_ >> noskipws; //not skip the whitespace
      setend = false;
      NextChar();
      cur_func_= nullptr;
  }

  void ParseNext();
  bool End() {return setend;}
  bool NotEof() {return !in_.eof();}
  bool IsGlobal() {return cur_func_ == nullptr;}

 private:
  void NextChar() {
    if(in_.eof()){
      setend = true;
      return;
    }
    in_ >> last_char_;
    if(last_char_ == '\n') return;
    while(!in_.eof() && isspace(last_char_)) in_ >> last_char_;
  }
  char PeekChar() {
    return in_.peek();
  }
  void ParseVarDecl();
  void ParseFuncDef();
  void ParseExpr();
  void ParseIf();
  void ParseGoto();
  void ParseL();
  void ParseFuncCall(EValPtr _dest);
  void ParseParam();
  void ParseReturn();
  void ParseEnd();
  EValPtr ParseVar();
  EValPtr ParseLVal();
  EValPtr ParseRVal();
  EValPtr ParseNum();
  EValPtr ParseLabel();
  EFuncDefPtr ParseFunc();
  int ParseInt();
  void HandleComment(); //the seconde /
  int CalInt(int x, int y, const Operator& op);

  inline void MakeBlock(bool _labeled);
  
  char last_char_;
  int line_ = 1; //line number

  bool setend;
  bool is_last_branch_ = false;
  bool is_last_cut_ = false;
  
  GlobalManager& gm_;
  FuncSet& fs_;

  EFuncDefPtr cur_func_;
  BlockPtr cur_block_;
  
  //遇到一个param先记录，如果有call指令再弹出记录调用的函数
  vector<shared_ptr<EParamInst> > temp_params_;
};

#endif
