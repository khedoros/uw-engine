#include "util.h"
#include<iostream>
#include<iomanip>
#include<string>
#include<vector>
#include<values.h>

#include "opl.h"
#include<SFML/System.hpp>
#include<SFML/Audio.hpp>

using namespace std;

bool load(string fn) {
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
    else if(hw_type == 1) cout<<"Dual OPL2"<<endl;
    else if(hw_type == 2) cout<<"OPL3"<<endl;
    else cout<<"Unknown ("<<int(hw_type)<<")"<<endl;
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

    //Basically, done reading the header, ready to start reading the actual data and sending it to the OPL.

    OPLEmul *opl = JavaOPLCreate(true);
    //OPLEmul *opl = YM3812Create(true);
    opl->Reset();
    for(int i=0;i<18;++i)
        opl->SetPanning(i, 1.0,1.0);

    
    double maxseen = 0.0, minseen = 0.0;
    long time_between_reg = 0;
    vector<float> in_buffer;
    vector<int16_t> out_buffer;
    //float *buffer = new float[100000];

    for(size_t i = 0; i < pairs; ++i) {
        if(i%1000 == 0)
            cout<<"Pair "<<i+1<<"/"<<pairs<<endl;
        uint8_t reg, val;
        bool card = false;
        infile>>reg>>val;
        if(reg >= 128) {/*cout<<"Card: 1 ";*/ card=true;}
        //else cout<<"Card: 0 ";
        reg&=0x7f;
        if(reg > code_cnt - 1 && reg != sh_delay && reg != lng_delay) { cout<<"Error: Found a register code that's too high. Aborting."<<endl; return false; }
        //cout<<hex<<"Reg: "<<setw(2)<<int(reg_trans[reg])<<" Val: "<<setw(2)<<int(val)<<dec;
        if(reg == sh_delay) {
            time_between_reg += int(val)+1;
        }
        else if(reg == lng_delay) {
            time_between_reg += (int(val)+1) * 256;
        }
        else {
            reg = reg_trans[reg];
            //Write the register value to the OPL emulator
            if(time_between_reg != 0) {
                size_t to_resize = (time_between_reg * OPL_SAMPLE_RATE) / 500;
                if( to_resize % 2 == 1 ) to_resize++;
                in_buffer.resize(to_resize);
                opl->Update(&in_buffer[0], in_buffer.size()/2);
                for(size_t j = 0;j<in_buffer.size();++j) {
                    //65536: Range of the new data type. 10: largest number I've seen in the OPL emulator output
                    out_buffer.push_back(int16_t((in_buffer[j]*32768.0)/64.0));
                    if(in_buffer[j]>maxseen) maxseen = in_buffer[j];
                    if(in_buffer[j]<minseen) minseen = in_buffer[j];
                    if((in_buffer[j]*(32768.0/64.0))>32767 || (in_buffer[j]*(32768.0/64.0)) < -32768) {
                        //cout<<in_buffer[j]<<" overflows a short variable."<<endl;
                    }
                }
            }
            opl->WriteReg(reg + ((card)?0x100:0x0), val);
            time_between_reg = 0;
        }
    }
    sf::SoundBuffer rendered;
    cout<<"Rendered "<<out_buffer.size()/2<<" stereo samples at rate: "<<OPL_SAMPLE_RATE<<". Should be "<<out_buffer.size()/(2*OPL_SAMPLE_RATE)<<" seconds long."<<endl;
    bool status = rendered.loadFromSamples(static_cast<short *>(&out_buffer[0]), out_buffer.size(), 2, OPL_SAMPLE_RATE);
    if(!status) cout<<"Failed to load the samples. Bummer."<<endl;
    cout<<"SFML thinks it's "<<rendered.getDuration().asMilliseconds()<<" ms long."<<endl;
    cout<<"Minseen: "<<minseen<<" Maxseen: "<<maxseen<<endl;
    sf::Sound output_audio(rendered);
    cout<<"Saving to "<<fn<<".ogg"<<endl;
    rendered.saveToFile(fn+".ogg");
    cout<<"Done. Playing it now =)"<<endl;
    output_audio.play();
    sf::sleep(sf::milliseconds(rendered.getDuration().asMilliseconds()));
    //while(output_audio.getStatus() == sf::SoundSource::Playing) {}
    return true;
}

int main(int argc, char *argv[]) {
    load(argv[1]);
}
