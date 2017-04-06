#include "convvm.h"
#include "UwText.h"
#include "convfile.h"
#include "globfile.h"
#include "savefile.h"
#include "util.h"
#include<iostream>

//Implements the conversation engine

convvm::convvm(int w, convfile& c, globfile& g, savefile& s, UwText& t) : who(w), conv(c), glob(g), save(s), text(t) {
    //set up the memory map

    pc = 0;
    sp = 0;
    bp = 0;
    result = 0;
    std::copy(conv.convos[who].code.begin(), conv.convos[who].code.end(), code_stack.begin());
};

void convvm::start() {
    int16_t op = 1;
    //while(op != 0x26 /*EXIT_OP*/) {
    //}
}
#ifdef STAND_ALONE_VM
int main() {
    int who = 291; //Small conversation with an attacking ghoul
    convfile c;
    globfile g;
    savefile s;
    UwText t;
    c.load(std::string("../uw/uw/data/cnv.ark"));
    convvm vm(who, c,g,s,t);
    vm.start();
}
#endif
