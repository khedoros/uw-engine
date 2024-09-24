#include "util.h"
#include<iostream>
#include<iomanip>
#include<string>
#include<vector>
#include "oplStream.h"

using namespace std;

bool load(string fn) {
    oplStream opl2;
    binifstream infile;
    infile.open(fn.c_str(), ios::binary);
    infile.seekg(0,ios::end);
    size_t filesize = infile.tellg();
    std::cout<<"Find filesize: "<<filesize<<'\n';
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
        cout<<"Versions besides 2.0 are not supported. Bailing."<<endl;
        return false;
    }
    uint32_t pairs, len_ms;
    infile>>pairs>>len_ms;
    cout<<"Reg+Value pairs: "<<pairs<<" Length in ms: "<<len_ms;
    uint8_t hw_type;
    infile>>hw_type;
    cout<<" Hardware: ";
    if(hw_type == 0) cout<<"OPL2"<<endl;
    else if(hw_type == 1) cout<<"Dual OPL2"<<endl;
    else if(hw_type == 2) cout<<"OPL3"<<endl;
    else cout<<"Unknown ("<<int(hw_type)<<")"<<endl;
    uint8_t format;
    infile>>format;
    if(format != 0) { cout<<"Non-interleaved data isn't supported. Bailing."<<endl; return false;}
    uint8_t compression;
    infile>>compression;
    if(compression != 0) { cout<<"Compression isn't supported. Bailing."<<endl; return false;}
    uint8_t sh_delay, lng_delay, code_cnt;
    infile>>sh_delay>>lng_delay>>code_cnt;
    cout<<"Short delay: "<<hex<<int(sh_delay)<<" Long delay: "<<int(lng_delay)<<" Code count: "<<int(code_cnt)<<dec<<endl;
    vector<uint8_t> reg_trans;
    reg_trans.resize(code_cnt);
    for(auto& code:reg_trans) { infile>>code; }
    opl2.play();
    cout<<"Filesize: "<<filesize<<" offset: "<<infile.tellg()<<" pair count: "<<pairs<<'\n';
    for(size_t i = 0; i < pairs; ++i) {
        uint8_t reg, val;
        bool card = false;
        infile>>reg>>val;
        if(reg >= 128) {cout<<"Card: 1 "; card=true;}
        else cout<<"Card: 0 ";
        reg&=0x7f;
        if(reg > code_cnt - 1 && reg != sh_delay && reg != lng_delay) {
            cout<<"Error: Invalid register code. Bailing."<<endl; return false; 
        }

        if(reg == sh_delay) {
            cout<<" Delay "<<int(val)+1<<"ms"<<endl;
            opl2.addTime(int(val)+1);
        }
        else if(reg == lng_delay) {
            cout<<" Delay "<<(int(val)+1) * 256<<"ms"<<endl;
            opl2.addTime((int(val)+1) * 256);
        }
        else {
            cout<<hex<<"Reg: "<<setw(2)<<int(reg_trans[reg])<<" Val: "<<setw(2)<<int(val)<<dec<<'\n';
            if(!card) {
                opl2.WriteReg(reg_trans[reg],val);
            }
            else {
                std::cout<<"Not handling write to card 1\n";
            }
        }
        if(size_t(infile.tellg()) > filesize || infile.eof()) cout<<"EEEP! Unexpected end of the file!"<<endl;
    }
    cout<<"At file offset "<<infile.tellg()<<", out of expected "<<filesize<<endl;
    return true;
}

int main(int argc, char *argv[]) {
    load(argv[1]);
}
