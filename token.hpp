#ifndef TOKEN_H_
#define TOKEN_H_

//keywords
#define SYSY_KEYWORD(e)                                                         \
    e(Void, "void") e(Const, "const") e(Int, "int") e(If, "if") e(Else, "else") \
    e(While, "while") e(Break, "break") e(Continue, "continue") e(Return, "return")
#define SYSY_KEYWORD_NUM 9

//operators
// () for exp
#define SYSY_OPERATOR(e)                                                                     \
    e(Add, "+") e(Sub, "-") e(Mul, "*") e(Div, "/") e(Mod, "%") e(Addr, "&") e(Inc, "++") e(Dec, "--")  \
    e(Eq, "==") e(NotEq, "!=") e(Less, "<") e(More, ">") e(LessEq, "<=") e(MoreEq, ">=")     \
    e(Not, "!") e(And, "&&") e(Or, "||") \
    e(Assign, "=") e(AddAssign, "+=") e(SubAssign, "-=") e(MulAssign, "*=") e(DivAssign, "/=") e(ModAssgin, "%=")
#define SYSY_OPERATOR_NUM 21

//paren
#define SYSY_PAIR(e) \
    e(LParen, "(") e(RParen, ")") e(LBracket, "[") e(RBracket, "]") e(LBrace, "{") e(RBrace, "}")
#define SYSY_PAIR_NUM 6

#define SYSY_EXPAND_FIRST(i, ...) i,
#define SYSY_EXPAND_SECOND(i, j, ...) j,

enum class Token{
    Ident,
    Keyword,
    Opeator,
    ConstInt,
    String,
    Char,
    Pair,
    Semic,
    Comma,
    Error,
    End,
    Other,
    Empty
};

enum class Keyword{
    SYSY_KEYWORD(SYSY_EXPAND_FIRST)
};

enum class Operator{
    SYSY_OPERATOR(SYSY_EXPAND_FIRST) Blank
};

enum class Pair{
    SYSY_PAIR(SYSY_EXPAND_FIRST)
};

enum class Type{
    ConstInt,
    Int,
    Void
};


#endif