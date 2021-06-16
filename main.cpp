#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
//#define DEBUG
#ifdef DEBUG
#include "lexer.hpp"
#include "lexer.cpp"
#include "parser.hpp"
#include "parser.cpp"
#include "ast.hpp"
#include "ir.hpp"
#include "irgen.hpp"
#include "ast.cpp"
#include "ir.cpp"
#include "irgen.cpp"
#include "eeyore.hpp"
#include "eeyore.cpp"
#include "eparser.hpp"
#include "eparser.cpp"
#include "cfg.hpp"
#include "cfg.cpp"
#include "global.hpp"
#include "global.cpp"
#include "regalloc.hpp"
#include "regalloc.cpp"
#include "tiggergen.hpp"
#include "tiggergen.cpp"
#include "tigger.hpp"
#include "tigger.cpp"
#else
#include "lexer.hpp"
#include "parser.hpp"
#include "irgen.hpp"
#include "eparser.hpp"
#include "global.hpp"
#include "cfg.hpp"
#include "tiggergen.hpp"
#endif
using namespace std;


char* input_file_name;
char* output_file_name;
bool opt_flag, assemb_flag, exe_flag;

const string test_case_1_name[] = {
    "performance_test/00_bitset1.sy",
    "performance_test/00_bitset2.sy",
    "performance_test/00_bitset3.sy",
    "performance_test/01_mm1.sy" ,
    "performance_test/01_mm2.sy",
    "performance_test/01_mm3.sy",
    //"test_case/14_or.sy",
    //"test_case/15_array_test3.sy",
    //"test_case/15_equal.sy"
    //"functional_test/99_register_realloc.sy"
};
const string test_case_2_name[] = {
    "D:\\untitled\\sysy2eeyore-gabbage\\sysy2eeyore-gabbage-master\\functional_test\\00_arr_defn2.sy",
    "D:\\untitled\\sysy2eeyore-gabbage\\sysy2eeyore-gabbage-master\\functional_test\\01_var_defn.sy",
    "D:\\untitled\\sysy2eeyore-gabbage\\sysy2eeyore-gabbage-master\\functional_test\\01_var_defn.sy",
    "D:\\untitled\\sysy2eeyore-gabbage\\sysy2eeyore-gabbage-master\\functional_test\\02_arr_defn4.sy",
    "D:\\untitled\\sysy2eeyore-gabbage\\sysy2eeyore-gabbage-master\\functional_test\\02_var_defn2.sy"
};
void test_case(){
    int t = 0;
    stringstream ss;
    string input_file, parser_out_file, riscv_out_file, tigger_out_file;
    string prefix_1("pa");
    string prefix_2("r");
    string prefix_3("t");
    string num;
    for(const auto& input_file: test_case_2_name){
        cout << t << "\n";
        ss << t;
        
        parser_out_file = prefix_1 + ss.str() + ".txt";
        riscv_out_file = prefix_2 + ss.str() + ".s";
        tigger_out_file = prefix_3 + ss.str() + ".t";
        ifstream ifs(input_file.c_str());
        //ofstream ofs(parser_out_file.c_str());
        ofstream  t_ofs(tigger_out_file.c_str());
        ofstream code_ofs(riscv_out_file.c_str());

        stringstream ess;
        ess << noskipws;
        ess >> noskipws;

        Lexer lexer(ifs);
        Parser parser(lexer);
        IRGenerator irgen;


        while(auto t = parser.ParserNext()){
            //t->print(ofs);
            t->GenerateIR(irgen);
        }
        ifs.close();
        //ofs.close();
        //irgen.Dump(code_ofs);
        //code_ofs.close();

        irgen.Dump(ess);
        //mid_ofs.close();
        //ifstream eifs("mid.eeyore");
    

        GlobalManager gm;
        FuncSet fc;
        TiggerGenerator tgen(gm, fc);
        EParser eparser(ess, gm, fc);

        while(!eparser.End()){
            eparser.ParseNext();
        }
        //eifs.close();
        // loop分析
        fc.BuildBlockGragh();
        fc.LoopAnalyzeOn();
        
        // block排序
        fc.SortBlocks();
        //regallocate
        fc.RegisterAllocate();

        tgen.GenerateOn();
        tgen.Dump(t_ofs);
        tgen.DumpRiscv(code_ofs);
        code_ofs.close();

        t++;
        ss.clear();
        ss.str(""); //reset
    }
}

void handin(int argc, char* argv[]){
    // compiler -S -e testcase.c -o testcase.S
        ifstream ifs(argv[2]);
        ofstream code_ofs(argv[4]);
        stringstream ss;
        ss << noskipws;
        ss >> noskipws;
        Lexer lexer(ifs);
        Parser parser(lexer);
        IRGenerator irgen;


        while(auto t = parser.ParserNext()){
            //t->print(ofs);
            t->GenerateIR(irgen);
        }
        ifs.close();

        irgen.Dump(ss);
    

        GlobalManager gm;
        FuncSet fc;
        TiggerGenerator tgen(gm, fc);
        EParser eparser(ss, gm, fc);

        while(!eparser.End()){
            eparser.ParseNext();
        }
        // loop分析
        fc.BuildBlockGragh();
        fc.LoopAnalyzeOn();
        
        // block排序
        fc.SortBlocks();
        //regallocate
        fc.RegisterAllocate();

        tgen.GenerateOn();
        tgen.DumpRiscv(code_ofs);
        //tigger_ofs.close();
}

int main(int argc, char* argv[]){
   #ifdef DEBUG
   test_case();
   #else
   handin(argc, argv);
   #endif
}