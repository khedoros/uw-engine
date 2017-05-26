#include<iostream>
#include<fstream>
#include "util.h"
#include "palette.h"

using namespace std;

int main(int argc, char *argv[]) {
    if(argc != 5) {
        cerr<<"Provide uw1 exe and palette, then uw2 exe and palette."<<endl;
        return 1;
    }

    ifstream exe1(argv[1]);
    if(!exe1.is_open()) {
        cerr<<"Couldn't open uw1 exe at "<<argv[1]<<endl;
        return 1;
    }
    string palfile1 = string(argv[2]);
    palette pal1;
    if(!pal1.load(palfile1,0)) {
        cerr<<"Couldn't open uw1 palette at "<<palfile1<<endl;
        return 1;
    }

    ifstream exe2(argv[3]);
    if(!exe2.is_open()) {
        cerr<<"Couldn't open uw2 exe at "<<argv[3]<<endl;
        return 1;
    }
    string palfile2 = string(argv[4]);
    palette pal2;
    if(!pal2.load(palfile2,0)) {
        cerr<<"Couldn't open uw2 palette at "<<palfile2<<endl;
        return 1;
    }

    vector<uint8_t> exe1_table(32*4);
    vector<uint8_t> exe2_table(32*5);
    vector<vector<color>> uw1_colors;
    uw1_colors.resize(32);
    vector<vector<color>> uw2_colors;
    uw2_colors.resize(32);

    exe1.seekg(0x5e2cb);
    exe1.read(reinterpret_cast<char*>(&exe1_table[0]), 32*4);
    for(int i=0;i<32;i++) {
        uw1_colors[i].resize(3);
        uw2_colors[i].resize(3);
        for(int j=0;j<3;j++) {
            uw1_colors[i][j] = pal1.get(exe1_table[i*4+j]);
            cout<<hex<<"UW1, model #"<<i<<", color #"<<j<<": ("<<int(uw1_colors[i][j].r)<<", "<<int(uw1_colors[i][j].g)<<", "<<int(uw1_colors[i][j].b)<<") (palette #"<<int(exe1_table[i*4+j])<<")"<<endl;
        }
    }
    return 0;
}
