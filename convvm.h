#pragma once

class convvm {
    convvm(int w, convfile& c, globfile& g, savefile& s) : who(w), conv(c), glob(g), save(s) 

    int who;
    convfile conv;
    globfile glob;
    savefile save;
    UwText text;
    int pc;
    int sp;
    uint16_t data_stack[32*1024];
    uint16_t code_stack[32*1024];
};
