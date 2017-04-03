#include "convvm.h"
#include "UwText.h"
#include "convfile.h"
#include "globfile.h"
#include "savefile.h"

//Implements the conversation engine

convvm::convvm(int w, convfile& c, globfile& g, savefile& s, UwText t) : who(w), conv(c), glob(g), save(s), text(t) {
    //set up the memory map

    pc = 0;
    sp = 0;
};
