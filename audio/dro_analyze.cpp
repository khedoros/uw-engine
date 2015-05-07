#include "util.h"
#include<iostream>
#include<iomanip>
#include<string>
#include<vector>

using namespace std;

//Keep track of which bits are changed in each register
uint8_t bits[256];

bool load(string fn) {
    for(int i=0;i<256;++i) {
        bits[i]=0;
    }
    binifstream infile;
    infile.open(fn.c_str(), ios::binary);
    infile.seekg(0,ios::end);
    size_t filesize = infile.tellg();
    infile.seekg(0,ios::beg);
    char sig[9];
    sig[8] = 0;
    infile.read(&sig[0],8);
    cout<<sig<<endl;
    if(string(sig) != "DBRAWOPL")
        return false;
    uint16_t maj_ver, min_ver;
    infile>>maj_ver>>min_ver;
    cout<<"DosBoxRawOPL dump version: "<<maj_ver<<"."<<min_ver<<endl;
    if(maj_ver != 2 && min_ver != 0) {
        cout<<"Just handling version 2.0, for now, sorry."<<endl;
        return false;
    }
    uint32_t pairs, len_ms;
    infile>>pairs>>len_ms;
    cout<<"Data points: "<<pairs<<" Length in ms: "<<len_ms;
    uint8_t hw_type;
    infile>>hw_type;
    cout<<" Hardware: ";
    if(hw_type == 0) cout<<"OPL2"<<endl;
    //else if(hw_type == 1) cout<<"Dual OPL2"<<endl;
    //else if(hw_type == 2) cout<<"OPL3"<<endl;
    //else cout<<"Unknown ("<<int(hw_type)<<")"<<endl;
    else {
        cout<<"That's a non-OPL2 DRO, and I don't feel like doing it for now."<<endl;
        return false;
    }
    uint8_t format;
    infile>>format;
    if(format != 0) { cout<<"Data isn't interleaved, I'm not sure what to do."<<endl; return false;}
    uint8_t compression;
    infile>>compression;
    if(compression != 0) { cout<<"Compression not supported. Bailing like a coward."<<endl; return false;}
    uint8_t sh_delay, lng_delay, code_cnt;
    infile>>sh_delay>>lng_delay>>code_cnt;
    cout<<"Short delay: "<<hex<<int(sh_delay)<<" Long delay: "<<int(lng_delay)<<" Code count: "<<int(code_cnt)<<dec<<endl;
    vector<uint8_t> reg_trans;
    reg_trans.resize(code_cnt);
    bool already_set[256];
    for(int i=0;i<256;++i) already_set[i]=false;
    for(size_t i = 0; i < code_cnt; ++i) {
        infile>>reg_trans[i];
        if(already_set[reg_trans[i]]) { cout<<"Problem. I've seen this one already."<<endl; return false; }
        already_set[reg_trans[i]] = true;
    }
    for(size_t i = 0; i < pairs; ++i) {
        uint8_t reg, val;
        infile>>reg>>val;
        if(reg == sh_delay) cout<<" Delay "<<int(val)+1<<"ms"<<endl;
        else if(reg == lng_delay) cout<<" Delay "<<(int(val)+1) * 256<<"ms"<<endl;
        else {
            bool card0 = false;
            if(reg >= 128) cout<<"Card: 1 ";
            else { cout<<"Card: 0 "; card0 = true; }
            reg&=0x7f;
            if(reg > code_cnt - 1) { cout<<"Error: Found a register code that's too high. Aborting."<<endl; return false; }
            cout<<hex<<"Reg: "<<setw(2)<<int(reg_trans[reg])<<" Val: "<<setw(2)<<int(val)<<dec<<endl;
            bits[reg_trans[reg]] |= val;
        }
        if(infile.tellg() > filesize || infile.eof()) cout<<"EEEP! Unexpected end of the file!"<<endl;
    }
    cout<<"At file offset "<<infile.tellg()<<", out of expected "<<filesize<<endl;
    for(int i=0;i<256;++i)
        if(bits[i] != 0) {
            cout<<"Reg 0x"<<hex<<setw(2)<<i<<" used bits: "<<setw(2)<<int(bits[i])<<" (";
            for(int j=128;j>0;j/=2) {
                cout<<((j&bits[i])>0)?1:0;
            }
            cout<<")"<<endl;
        }
}

int main(int argc, char *argv[]) {
    load(argv[1]);
}
