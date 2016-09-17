#include "kail.h"
#include "opl3_const.h"
#include<utility> //contains pair

void kail_startup(const uw_patch_file& gtl) {
    patches = gtl;
    opl = JavaOPLCreate(/*bool stereo =*/ true);
    reset_synth();
}

void kail_shutdown() {
    if (opl) delete opl;
}

void reset_synth() {
    if(!opl) return;

    opl->Reset();            //reset the internal state of the OPL3 emulator
    for(int voice=0;voice<18;++voice) {
        opl->SetPanning(voice, 0.5,0.5); //the emulator provides per-voice panning abilities
    }

    opl->WriteReg(0x105, 1); //set OPL3 bit to enable more voices
    opl->WriteReg(0x104, 0); //set it to provide 18 2-op voices, rather than one of the 4-op options

    for(int reg = 0; reg < 0xf5; ++reg) {
        opl->WriteReg(reg+1,array0_init[reg]);       //First element in the array is actually reg #1, instead of reg #0.
        opl->WriteReg(reg+0x101, array1_init[reg]);  //Ditto. The registers at 0x00 and 0x100 are read-only (or unused, don't remember for sure)
    }
}

bool kail_seq::load(xmi& x, const uint16_t seqnum) {
    music = x;
    music.reset_timbre_it();
    std::pair<uint8_t, uint8_t> * timbre;
    while(timbre = music.next_timbre()) {
        bool found = false;
        for(auto it = patches.bank_data.begin(); it != patches.bank_data.end(); ++it) {
            if(it->bank == timbre->first && it->patch == timbre->second) {
                found = true;
                break;
            }
        }
        if(!found) {
            std::cerr<<"The sequence requested timbre "<<int(timbre->first)<<":"<<int(timbre->second)<<", which isn't present in the specified timbre library."<<std::endl;
            return false;
        }
    }
    return true;
}

void kail_seq::play() {

}

void kail_seq::pause() {

}

void kail_seq::stop() {

}

bool kail_seq::onGetData(sf::SoundStream::Chunk& data) {

}

void kail_seq::onSeek(sf::Time timeoffset) {

}
