#include<iostream>
#include<fstream>
#include "util.h"
#include "palette.h"

using namespace std;

const uint32_t start_offsets[] =       {0x4e910, 0x4ccd0, 0x4e370, 0x4ec70, 0x54cf0, 0x550e0};
const uint32_t model_table_offsets[] = {0x4e99e, 0x4cd5e, 0x4e3fe, 0x4ecfe, 0x54d8a, 0x5517a};
const uint32_t color_table_offsets[] = {0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000};

bool check_offset(uint32_t offset, ifstream& in) {
    const uint8_t uw1_first_bytes[] = {0xb6, 0x4a, 0x06, 0x40};
    const uint8_t uw2_first_bytes[] = {0xd4, 0x64, 0xaa, 0x59};
    uint32_t bookmark = in.tellg();
    in.seekg(offset, ios::beg);
    for(int i=0;i<4;++i) {
        uint8_t temp = read8(in);
        if(temp != uw1_first_bytes[i] && temp != uw2_first_bytes[i]) {
            in.seekg(bookmark, ios::beg);
            return false;
        }
    }
    in.seekg(bookmark, ios::beg);
    return true;
}

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

    //vector<uint8_t> exe1_table(32*4);
    //vector<uint8_t> exe2_table(32*5);
    exe1.seekg(0,ios::end);
    vector<uint8_t> exe1_table(exe1.tellg());
    exe1.seekg(0,ios::beg);
    exe1.read(reinterpret_cast<char *>(&(exe1_table[0])), exe1_table.size());
    exe2.seekg(0,ios::end);
    vector<uint8_t> exe2_table(exe2.tellg());
    exe2.seekg(0,ios::beg);
    exe2.read(reinterpret_cast<char *>(&(exe2_table[0])), exe2_table.size());
    vector<vector<color>> uw1_colors;
    uw1_colors.resize(32);
    vector<vector<color>> uw2_colors;
    uw2_colors.resize(32);

    uint8_t uw1_expected[] = {0xec, 0x00, 0x00, 0x21, 0xeb, 0x00, 0x00, 0x11};
    //uint8_t uw2_expected[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t uw2_expected[] = {0x01, 0x8f, 0x00, 0x00, 0x00, 0x21, 0x93, 0x00}; 


    uint32_t uw1_color_offset = 0;
    uint32_t uw2_color_offset = 0;
    bool found = false;
    for(int start = 0; start < exe1_table.size() - 8 && !found; start++) {
        bool failed = false;
        for(int i=0;i<8;i++) {
            if(uw1_expected[i] != exe1_table[start + i]) {
                failed = true;
            }
        }
        if(!failed) {
            found = true;
            cout<<"I think I found the UW1 offset at "<<hex<<start<<endl;
            uw1_color_offset = start;
        }
    }

    found = false;
    for(int start = 0; start < exe2_table.size() - 8 && !found; start++) {
        bool failed = false;
        for(int i=0;i<8;i++) {
            if(uw2_expected[i] != exe2_table[start + i]) {
                failed = true;
            }
        }
        if(!failed) {
            found = true;
            cout<<"I think I found the UW2 offset at "<<hex<<start<<endl;
            uw2_color_offset = start;
        }
    }

    if(!uw1_color_offset || !uw2_color_offset) {
        cout<<"Couldn't find one of the offsets."<<endl;
        return 1;
    }

    for(int i=0;i<32;i++) {
        uw1_colors[i].resize(3);
        uw2_colors[i].resize(3);
        for(int j=0;j<3;j++) {
            uw1_colors[i][j] = pal1.get(exe1_table[uw1_color_offset + (i * 4) + j]);
            uw2_colors[i][j] = pal2.get(exe2_table[uw2_color_offset + (i * 5) + j + 1]);
            cout<<hex<<"UW1, model #"<<i<<", color #"<<j<<": ("<<int(uw1_colors[i][j].r)<<", "<<int(uw1_colors[i][j].g)<<", "<<int(uw1_colors[i][j].b)<<") (palette #"<<int(exe1_table[uw1_color_offset+(i*4)+j])<<")"<<endl;
            cout<<hex<<"UW2, model #"<<i<<", color #"<<j<<": ("<<int(uw2_colors[i][j].r)<<", "<<int(uw2_colors[i][j].g)<<", "<<int(uw2_colors[i][j].b)<<") (palette #"<<int(exe2_table[uw2_color_offset+(i*5)+j+1])<<")"<<endl;
        }
    }

    return 0;
}
