#include "util.h"
#include<iostream>
#include<iomanip>
#include<string>
#include<vector>
#include "sine_gen.h"

using namespace std;

SineGen opl1[9];
SineGen opl2[9];

uint16_t notes1[9], notes2[9], oct1[9], oct2[9];

void set_freq(int i, bool card) {
    if(card) opl1[i].set_freq((notes1[i] * 49716) / pow(2.0,(20-oct1[i])));
    else     opl2[i].set_freq((notes2[i] * 49716) / pow(2.0,(20-oct2[i])));
}

int chan(int reg) { //Only valid for registers 20-35, 40-55, 60-75, 80-95, a0-a8, b0-b8, c0-c8, and e0-f5
    if(reg >= 0xa0 && reg <= 0xc8) return (reg % 0x10)+1;

    reg %= 0x20;
    if(reg == 0x00 || reg == 0x03) return 1;
    if(reg == 0x01 || reg == 0x04) return 2;
    if(reg == 0x02 || reg == 0x05) return 3;

    if(reg == 0x08 || reg == 0x0b) return 4;
    if(reg == 0x09 || reg == 0x0c) return 5;
    if(reg == 0x0a || reg == 0x0d) return 6;

    if(reg == 0x10 || reg == 0x13) return 7;
    if(reg == 0x11 || reg == 0x14) return 8;
    if(reg == 0x12 || reg == 0x15) return 9;

    return 0; //invalid register
}

int op(int reg) {
   //These registers are either global, or affect the whole channel
   if(reg<0x20 || (reg > 0x95 && reg < 0xe0)) return 0;

   if(reg % 8 < 3) return 1;
   else return 2;
}

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

    for(int i=0;i<9;++i) {
        opl1[i].play();
        opl2[i].play();
        notes1[i] = 200;
        notes2[i] = 200;
    }

    for(size_t i = 0; i < pairs; ++i) {
        uint8_t reg, val;
        bool card = false;
        infile>>reg>>val;
        if(reg >= 128) {cout<<"Card: 1 "; card=true;}
        else cout<<"Card: 0 ";
        reg&=0x7f;
        if(reg > code_cnt - 1 && reg != sh_delay && reg != lng_delay) { cout<<"Error: Found a register code that's too high. Aborting."<<endl; return false; }
        cout<<hex<<"Reg: "<<setw(2)<<int(reg_trans[reg])<<" Val: "<<setw(2)<<int(val)<<dec;
        if(reg == sh_delay) {cout<<" Delay "<<int(val)+1<<"ms"<<endl; sf::sleep(sf::microseconds((int(val)+1)*1000));}
        else if(reg == lng_delay) {cout<<" Delay "<<(int(val)+1) * 256<<"ms"<<endl; sf::sleep(sf::microseconds((int(val)+1)*256000));}
        else {
            switch(reg_trans[reg]) {
              case 1:
                  if(val == 0x20) cout<<" (Enable waveform control)"<<endl;
                  else if(val == 0x20) cout<<" (Disable waveform control)"<<endl;
                  else cout<<" (UNKNOWN VALUE)"<<endl;
                  break;
              case 2:
                  cout<<" (Timer 1 set (80 microseconds per iteration))"<<endl;
                  break;
              case 3:
                  cout<<" (Timer 2 set (320 microseconds per iteration))" <<endl;
                  break;
              case 4:
                  if((val & 0x80) == 0x80) { cout<<" (Reset timer flags)"<<endl; break; }
                  if((val & 0x40) == 0x40) cout<<" (Mask timer 1)";
                  else if((val & 0x1) == 0x1) cout<<" (Start timer 1)";
                  if((val & 0x20) == 0x20) cout<<" (Mask timer 2)";
                  else if((val & 0x2) == 0x2) cout<<" (Start timer 2)";
                  cout<<endl;
                  break;
              case 8:
                  if((val & 0x80) == 0x80) cout<<" (Sine Wave Speech Synthesis)";
                  else cout<<" (FM Synthesis)";
                  if((val & 0x40) == 0x40) cout<<" (Select keyboard split point (WTF?))";
                  cout<<endl;
              case 0x20: //Catch 20-35
              case 0x21:
              case 0x22:
              case 0x23:
              case 0x24:
              case 0x25:
              //case 0x26:
              //case 0x27:
              case 0x28:
              case 0x29:
              case 0x2a:
              case 0x2b:
              case 0x2c:
              case 0x2d:
              //case 0x2e:
              //case 0x2f:
              case 0x30:
              case 0x31:
              case 0x32:
              case 0x33:
              case 0x34:
              case 0x35:
                  cout<<" (Set Op"<<op(reg_trans[reg])<<" Ch. "<<chan(reg_trans[reg])<<" to Trem: ";
                  if((val & 0x80) == 0x80) cout<<"on "; else cout<<"off ";
                  cout<<"Vib: ";
                  if((val & 0x40) == 0x40) cout<<"on "; else cout<<"off ";
                  cout<<"Sust: ";
                  if((val & 0x20) == 0x20) cout<<"on "; else cout<<"off ";
                  cout<<"KSR: ";
                  if((val & 0x10) == 0x10) cout<<"on "; else cout<<"off ";
                  cout<<"Freq mult: "<<(val & 0x0f)<<")"<<endl;
                  break;
              case 0x40: //Catch 40-55
              case 0x41:
              case 0x42:
              case 0x43:
              case 0x44:
              case 0x45:
              //case 0x46:
              //case 0x47:
              case 0x48:
              case 0x49:
              case 0x4a:
              case 0x4b:
              case 0x4c:
              case 0x4d:
              //case 0x4e:
              //case 0x4f:
              case 0x50:
              case 0x51:
              case 0x52:
              case 0x53:
              case 0x54:
              case 0x55:
                  cout<<" (Set Op"<<op(reg_trans[reg])<<" Ch. "<<chan(reg_trans[reg])<<" to Level Scaling: "<<((val & 0xc0)>>(6))<<" Output attenuation: "<<(val & 0x3f)<<")"<<endl;
                  break;
              case 0x60: //Catch 60-75
              case 0x61:
              case 0x62:
              case 0x63:
              case 0x64:
              case 0x65:
              //case 0x66:
              //case 0x67:
              case 0x68:
              case 0x69:
              case 0x6a:
              case 0x6b:
              case 0x6c:
              case 0x6d:
              //case 0x6e:
              //case 0x6f:
              case 0x70:
              case 0x71:
              case 0x72:
              case 0x73:
              case 0x74:
              case 0x75:
                  cout<<" (Set Op"<<op(reg_trans[reg])<<" Ch. "<<chan(reg_trans[reg])<<" to Attack Rate: "<<((val & 0xf0)>>(4))<<" Decay Rate: "<<(val & 0x0f)<<")"<<endl;
                  break;
              case 0x80: //Catch 80-95
              case 0x81:
              case 0x82:
              case 0x83:
              case 0x84:
              case 0x85:
              //case 0x86:
              //case 0x87:
              case 0x88:
              case 0x89:
              case 0x8a:
              case 0x8b:
              case 0x8c:
              case 0x8d:
              //case 0x8e:
              //case 0x8f:
              case 0x90:
              case 0x91:
              case 0x92:
              case 0x93:
              case 0x94:
              case 0x95:
                  cout<<" (Set Op"<<op(reg_trans[reg])<<" Ch. "<<chan(reg_trans[reg])<<" to Sustain Attenuation: "<<((val & 0xf0)>>(4))<<" Release Rate: "<<(val & 0x0f)<<")"<<endl;
                  break;
              case 0xa0: //Catch a0-a8
              case 0xa1:
              case 0xa2:
              case 0xa3:
              case 0xa4:
              case 0xa5:
              case 0xa6:
              case 0xa7:
              case 0xa8:
                  cout<<" (Set Ch. "<<chan(reg_trans[reg])<<" Note Low Byte to: "<<int(val)<<")"<<endl;
                  if(card) notes1[chan(reg_trans[reg])] = val | (notes1[chan(reg_trans[reg])] & 0xFF00);
                  else     notes2[chan(reg_trans[reg])] = val | (notes2[chan(reg_trans[reg])] & 0xFF00);
                  break;
              case 0xb0: //Catch b0-b8
              case 0xb1:
              case 0xb2:
              case 0xb3:
              case 0xb4:
              case 0xb5:
              case 0xb6:
              case 0xb7:
              case 0xb8:
                  cout<<" (Set Ch. "<<chan(reg_trans[reg])<<" Note High Byte to: "<<(val & 0x04)<<" Note Octave to: "<<((val & 0x1c)>>(2))<<" Active: "<<((val & 0x20)>>(5))<<")"<<endl;
                  set_freq(chan(reg_trans[reg]),card);
                  if(card) {
                      opl1[chan(reg_trans[reg])].playing = ((val&0x20)>>(5) == 1);
                      oct1[chan(reg_trans[reg])] = (val & (0x1c>>(2)));
                      notes1[chan(reg_trans[reg])] = (val)<<(6) | (notes1[chan(reg_trans[reg])] & 0xFF);
                  }
                  else {
                      opl2[chan(reg_trans[reg])].playing = ((val&0x20)>>(5) == 1);
                      oct2[chan(reg_trans[reg])] = (val & (0x1c>>(2)));
                      notes2[chan(reg_trans[reg])] = (val)<<(6) | (notes2[chan(reg_trans[reg])] & 0xFF);
                  }
                  break;
              case 0xbd:
                  cout<<" (Set AM(Tremolo) depth: "<<((val & 0x80)>>(7))<<" FM(Vibrato) depth: "<<((val & 0x40)>>(6))<<" Rhythm On: "<<((val & 0x20)>>(5))<<
                        " Bass: "<<((val & 0x10)>>(4))<<" Snare: "<<((val & 0x08)>>(3))<<" TomTom: "<<((val & 0x04)>>(2))<<" Cymbal: "<<((val & 0x02)>>(1))<<
                        " Hi-Hat: "<<(val & 0x01)<<")"<<endl;
                  break;
              case 0xc0: //Catch c0-c8
              case 0xc1:
              case 0xc2:
              case 0xc3:
              case 0xc4:
              case 0xc5:
              case 0xc6:
              case 0xc7:
              case 0xc8:
                  cout<<" (Set Ch. "<<chan(reg_trans[reg])<<" Feedback to: "<<((val & 0x0e)>>(1))<<" Set Modulation type: ";
                  if((val & 0x1) == 1) cout<<"AM)"<<endl; else cout<<"FM)"<<endl;
                  break;
              case 0xe0: //Catch 80-95
              case 0xe1:
              case 0xe2:
              case 0xe3:
              case 0xe4:
              case 0xe5:
              //case 0xe6:
              //case 0xe7:
              case 0xe8:
              case 0xe9:
              case 0xea:
              case 0xeb:
              case 0xec:
              case 0xed:
              //case 0xee:
              //case 0xef:
              case 0xf0:
              case 0xf1:
              case 0xf2:
              case 0xf3:
              case 0xf4:
              case 0xf5:
                  cout<<" (Set Op"<<op(reg_trans[reg])<<" Ch. "<<chan(reg_trans[reg])<<" to Waveform: ";
                  val &= 0x07;
                  if(val == 0) cout<<"Sine";
                  else if(val == 1) cout<<"Half Sine";
                  else if(val == 2) cout<<"Absolute Sine";
                  else if(val == 3) cout<<"Pseudo-Saw";
                  else cout<<"Invalid Value";
                  cout<<endl;
                  //if(card) opl1[i].set_wave(val);
                  //else     opl2[i].set_wave(val);
                  break;
                  break;
              default: cout<<" (Unknown command)"<<endl;
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
