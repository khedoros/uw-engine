#include<iostream>
#include "mzBinary.h"

int main(int argc, char *argv[]) {
    mzBinary in(argv[1]);
    in.printHeaderInfo();
    return 0;
}
