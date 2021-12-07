#include<iostream>
#include<string>
#include "mzBinary.h"

int main(int argc, char *argv[]) {
    mzBinary in(argv[1]);
	std::string out(argv[2]);
    in.printHeaderInfo(true);
	in.dumpBinaryToFile(out+"-mainbin.out", true);
	size_t overlayCount = in.getOverlayCount();
	for(int i=0; i<overlayCount; i++) {
		in.dumpOverlayToFile(i, out+"-overlay-"+std::to_string(i)+".out", true);
	}
    return 0;
}
