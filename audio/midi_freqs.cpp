#include<iostream>
#include<math.h>
#include<cmath>
#include<stdint.h>
#include<vector>
#include<tuple>
using namespace std;

int main() {     //mid-note, OPL block#   OPL f-num
    vector<tuple<uint8_t,    uint8_t,     uint16_t>> freqs;
    double base_freq = 440.0;
    uint8_t base_mid_num = 69;
    for(uint16_t mid_num = 0; mid_num < 128; ++mid_num) {
        double midi_freq = base_freq * pow(2.0, (mid_num - base_mid_num)/12.0);
        cout<<"MIDI Number: "<<mid_num<<" Frequency: "<<midi_freq;
        double diff = 9999999999.0;
        uint8_t blk = 0;
        uint16_t f_num = 0;
        double OPL_freq = 0.0;
        for(uint32_t block = 0; block < 8; ++block) {
            for(uint32_t f_number = 0; f_number < 1024; ++f_number) {
                double opl_freq = double(f_number * 49716) / pow(2.0, 20 - double(block));
                if(abs(opl_freq - midi_freq) < diff) {
                    diff = abs(opl_freq - midi_freq);
                    f_num = f_number;
                    blk = block;
                    OPL_freq = opl_freq;
                }
            }
        }
        if(diff < 10) {
            cout<<" OPL_Blk: "<<uint16_t(blk)<<" F-Num: "<<f_num<<" OPL Freq: "<<OPL_freq<<endl;
            freqs.push_back(make_tuple(mid_num,blk,f_num));
        }
        else {
            cout<<" OPL: Out of Range"<<endl;
        }
    }
//(F-Number * 49716) / (2^(20-Block)) = Music-Frequency
    return 0;
}
