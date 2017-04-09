#include "convvm.h"
#include "UwText.h"
#include "convfile.h"
#include "globfile.h"
#include "savefile.h"
#include "util.h"
#include<iostream>
#include<string>

//Implements the conversation engine

convvm::convvm(int w, convfile& c, globfile& g, savefile& s, UwText& t) : who(w), conv(c), glob(g), save(s), text(t) {
    //set up the memory map

    pc = 0;
    sp = 0;
    bp = 0;
    result = 0;
    std::copy(conv.convos[who].code.begin(), conv.convos[who].code.end(), code_stack.begin());
    std::string temp[] = {
         "nop",     "add",    "mul",    "sub",      "div",      "mod",    "or",    "and",
         "not",     "tstgt",  "tstge",  "tstlt",    "tstle",    "tsteq",  "tstne", "jmp",
         "beq",     "bne",    "bra",    "call",     "calli",    "ret",    "pushi", "pushi_eff",
         "pop",     "swap",   "pushbp", "popbp",    "sptobp",   "bptosp", "addsp", "fetchm",
         "sto",     "offset", "start",  "save_reg", "push_reg", "strcmp", "exit",  "say",
         "respond", "neg" };
    for(int i=0;i<42;i++) op_strs[i] = temp[i];

};

string convvm::op_str(int op) {
    if(op>0 && op < 42)
        return op_strs[op];
    else
        return string("");
}

#ifdef STAND_ALONE_VM
int main() {
    std::cout<<"Yeah? It's the convo VM code. So what?"<<std::endl;

}
#endif
