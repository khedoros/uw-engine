#include "util.h"
#include "uw_patch.h"
#include <cstring>
#include <iostream>
#include <iomanip>
#include <vector>
#include<string>
/*
void uw_patch_file::print_opl(opl2_patch p) {
    std::cout<<std::hex<<"Size: "<<int(p.size)<<" Transpose: "<<int(p.transpose)<<" Feedback: "<<int(p.feedback)<<" Connection: "<<int(p.connection)<<std::endl;
    std::cout<<"Carrier:\n\tFreq Mult: "<<int(p.car_freq_mult)<<" Envelope Scaling: "<<int(p.car_env_scaling)<<" Sustain: "<<int(p.car_sustain_sound)<<" AM Vibrato: "<<int(p.car_ampl_vibrato)<<std::endl;
    std::cout<<"\tFM Vibrato: "<<int(p.car_freq_vibrato)<<" Output Lvl: "<<int(p.car_out_lvl)<<" Key Scale: "<<int(p.car_key_scale)<<" Decay: "<<int(p.car_decay)<<std::endl;
    std::cout<<"\tAttack: "<<int(p.car_attack)<<" Release: "<<int(p.car_release)<<" Sustain Lvl: "<<int(p.car_sustain_lvl)<<" Carrier Waveform: "<<int(p.car_waveform)<<std::endl;

    std::cout<<"Modulator:\n\tFreq Mult: "<<int(p.mod_freq_mult)<<" Envelope Scaling: "<<int(p.mod_env_scaling)<<" Sustain: "<<int(p.mod_sustain_sound)<<" AM Vibrato: "<<int(p.mod_ampl_vibrato)<<std::endl;
    std::cout<<"\tFM Vibrato: "<<int(p.mod_freq_vibrato)<<" Output Lvl: "<<int(p.mod_out_lvl)<<" Key Scale: "<<int(p.mod_key_scale)<<" Decay: "<<int(p.mod_decay)<<std::endl;
    std::cout<<"\tAttack: "<<int(p.mod_attack)<<" Release: "<<int(p.mod_release)<<" Sustain Lvl: "<<int(p.mod_sustain_lvl)<<" Carrier Waveform: "<<int(p.mod_waveform)<<std::endl<<std::endl;
}
*/
bool uw_patch_file::load_patches(binifstream& in) {
    uint8_t bank=0, patch=0;
    uint32_t offset;
    //in>>patch>>bank;
    while(bank!=0xff || patch != 0xff) {
        in>>patch>>bank;
        if(patch == 0xff && bank == 0xff) continue;
        in>>offset;
        size_t bookmark = in.tellg();
        in.seekg(offset,std::ios::beg);
        uint16_t size;
        in>>size;
        std::vector<uint8_t> data;
        opl2_patch opl_data;
        data.resize(size);
        in.seekg(offset,std::ios::beg);
        in.read(reinterpret_cast<char *>(&data[0]),size);

        if(size == sizeof(opl2_patch)) {
            //std::cout<<"Seeking back to beginning of instrument"<<std::endl;
            in.seekg(offset,std::ios::beg);
            in>>opl_data;
        }
        else {
            //std::cout<<"Size mismatch. Expected "<<sizeof(opl2_patch)<<", found "<<size<<std::endl;
        }

        char name_str[11];
        if(data.size() == 0xf8) {
            memcpy(name_str,&(data[2]),10);
            name_str[10] = 0;
        }
        bool found = false;
        for(auto it = bank_data.begin(); it != bank_data.end() && !found; ++it) {
            if((*it).bank == bank && (*it).patch == patch) {
                //std::cout<<"Adding data to already built patch structure"<<std::endl;
                (*it).setpat(data);
                (*it).name = std::string((name_str));
                found = true;
            }
        }
        if(!found) {
            //std::cout<<"Creating new patch structure"<<std::endl;
            bank_data.push_back(patchdat(bank,patch,data,opl_data));
        }
        in.seekg(bookmark,std::ios::beg);
        //std::cout<<"Sought back to "<<bookmark<<" (the bookmark point)."<<std::endl;
    }
    in.close();
    return true;
}

bool uw_patch_file::load(std::string fna, std::string fnm /*= ""*/) {
    binifstream ina, inm;
    //size_t filesizea, filesizem;
    ina.open(fna.c_str(),std::ios::binary);
    ina.seekg(0,std::ios::end);
    //filesizea = ina.tellg();
    ina.seekg(0,std::ios::beg);
    if(fnm != "") {
        inm.open(fnm.c_str(),std::ios::binary);
        inm.seekg(0,std::ios::end);
        //filesizem = inm.tellg();
        inm.seekg(0,std::ios::beg);
    }
    if(!load_patches(ina)) return false;
    if(fnm != "" && !load_patches(inm)) return false;
    #ifdef STAND_ALONE
    for(auto it = bank_data.begin(); it != bank_data.end(); ++it) {
        std::cout<<"Bank: "<<int((*it).bank)<<" Patch: "<<int((*it).patch);
        if((*it).name != "N/A") std::cout<<" Name: "<<(*it).name;
        if((*it).ad_patchdata.size() > 0) std::cout<<" Has Adblib Data, size: "<<(*it).ad_patchdata.size()<<"  {";
        if((*it).ad_patchdata.size() == 0x0e) for(auto it2 = (*it).ad_patchdata.begin(); it2 != (*it).ad_patchdata.end(); ++it2) std::cout<<std::hex<<" "<<int(*it2)<<std::dec;
        else if((*it).ad_patchdata.size() > 0x0e) {
            auto it2 = (*it).ad_patchdata.begin();
            std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<int(*it2++)<<std::setw(2)<<int(*it2++)<<std::endl;
            while(it2 != (*it).ad_patchdata.end()) {
                for(size_t count = 0;it2 != (*it).ad_patchdata.end() && count < 16; ++count)
                    std::cout<<std::hex<<" "<<std::setw(2)<<int(*it2++);
                std::cout<<std::endl;
            }
            std::cout<<std::dec;
        }
        if((*it).mt_patchdata.size() > 0) std::cout<<" Has MT-32  Data, size: "<<(*it).mt_patchdata.size();
        std::cout<<" }"<<std::endl;
        //if((*it).ad_patchdata.size()==0xe)
        //     print_opl((*it).ad_patchdatastruct);
    }
    #endif
    return true;
}

#ifdef STAND_ALONE
int main(int argc, char *argv[]) {
    uw_patch_file upf;
    if(argc == 3) {
        upf.load(argv[1], argv[2]);
    }
    else if(argc == 2) {
        upf.load(argv[1]);
    }
    else {
        std::cout<<"I expect 2 arguments: Two AIL2 patch definition files (One for Adlib, one for MT-32)."<<std::endl;
        return 1;
    }
    for(auto it = upf.bank_data.begin();it != upf.bank_data.end();++it) {
        uw_patch_file::print_opl(it->ad_patchdatastruct);
    }
}
#endif
