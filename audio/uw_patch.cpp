#include "util.h"
#include "uwPatch.h"
#include "uwPatchNames.h"
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

void uw_patch_file::patchdat::setpat(std::vector<uint8_t> d, uint8_t b, uint8_t p) {
    bank = b;
    patch = p;
    //std::cout<<int(bank)<<": "<<int(patch)<<": ";
    if(d.size() == 0x0e) {
        memcpy(reinterpret_cast<void *>(&ad_patchdatastruct), reinterpret_cast<void *>(&(d[0])), d.size());
        ad_patchdata = d;
        has_opl2 = true;
        if(name == "") {
            name = getName(bank,patch);
        }
        /* std::cout<<"Added Adlib Patch data"<<std::endl;*/
    }
    else if(d.size() == 0xf8) {
        mt_patchdata = d; 

        //This is a MT-32 patch, which usually have an ASCII name field, which I want to capture.
        char name_str[11];
        memcpy(name_str,&(d[2]),10);
        name_str[10] = 0;
        name = std::string(name_str);
        has_mt = true;
        /*std::cout<<"Added MT Patch data"<<std::endl;*/
    }
    else if(d.size() > (8*3*2+6)) { //8 elements, 3 items per element, 2 bytes per item, plus the 6 bytes that make up the first 4 fields of the header
        ad_patchdata = d;
        size_t offset = 0;
        //Set the always-present init values
        memcpy(reinterpret_cast<void *>(&(tv_patchdatastruct.init)), reinterpret_cast<void *>(&(d[offset])), sizeof(struct tvfx_init));
        offset += sizeof(struct tvfx_init);
        //If there are optional ADSR values, read them
        if(tv_patchdatastruct.init.keyon_f_offset == 0x3c) {
            memcpy(reinterpret_cast<void *>(&(tv_patchdatastruct.opt)), reinterpret_cast<void *>(&(d[offset])), sizeof(struct tvfx_init_opt));
            offset+= sizeof(struct tvfx_init_opt);
            tv_patchdatastruct.uses_opt = true;
            tv_patchdatastruct.offset = 0x3e;
        }
        //The rest of the data is put into a vector, because it's easier to access by offset than by name.
        assert((d.size() - offset) % 2 == 0 && d.size() > offset);
        tv_patchdatastruct.update_data.resize((d.size() - offset) / 2);
        memcpy(reinterpret_cast<void *>(&(tv_patchdatastruct.update_data[0])), reinterpret_cast<void *>(&(d[offset])), d.size() - offset);
        has_tvfx = true;
        if(name == "") {
            name = getName(bank, patch);
        }
    }
    else {
        std::cerr<<"Unimplemented patch type, size: "<<d.size()<<" bytes."<<std::endl;
    }
}

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
        data.resize(size);
        in.seekg(offset,std::ios::beg);
        in.read(reinterpret_cast<char *>(&data[0]),size);

        bool found = false;
        for(auto it = bank_data.begin(); it != bank_data.end() && !found; ++it) {
            if(it->bank == bank && it->patch == patch) {
                it->setpat(data, bank, patch);
                found = true;
            }
        }
        if(!found) {
            bank_data.push_back(patchdat());
            bank_data.back().setpat(data, bank, patch);
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
    if(!ina.is_open()) return false;
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
    return true;
}

std::string uw_patch_file::getName(int bank, int patch) {
    switch(bank) {
        case 0:
            if(patch < melodyInstruments.size())
                return melodyInstruments.at(patch);
            break;
        case 1:
            if(patch < tvfxInstruments.size())
                return tvfxInstruments.at(patch);
            break;
        case 127:
            if(patch < rhythmInstruments.size())
                return rhythmInstruments.at(patch);
            break;
    }
    return "";
}

#ifdef STAND_ALONE_PATCH
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
        std::cout<<"Bank: "<<int(it->bank)<<" Patch: "<<int(it->patch);
        if(it->name != "") {
            std::cout<<" Name: "<<it->name;
        }
        std::cout<<"\n";
        if(it->mt_patchdata.size() > 0) std::cout<<" Has MT-32  Data, size: "<<it->mt_patchdata.size()<<"\n";
        if(it->ad_patchdata.size() > 0) {
            std::cout<<" Has Adblib Data, size: "<<it->ad_patchdata.size()<<"  {\n";
            for(auto it2 = it->ad_patchdata.begin(); it2 != it->ad_patchdata.end(); ++it2) {
                std::printf(" %02x", *it2);
            }
            std::cout<<"\n}"<<std::endl;
            if(it->has_opl2) {
                uw_patch_file::print_opl(it->ad_patchdatastruct);
            }
            else if(it->has_tvfx) {
                uw_patch_file::print_tvfx(it->tv_patchdatastruct);
            }
        }
        std::cout<<"\n";
    }
}
#endif
