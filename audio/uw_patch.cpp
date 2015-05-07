#include "util.h"
#include <cstring>
#include <iostream>
#include <iomanip>
#include <vector>
#include<string>

struct __attribute__((packed)) opl2_patch {
/*
   unsigned char mod_avekm;
   unsigned char mod_ksl_tl;
   unsigned char mod_ad;
   unsigned char mod_sr;
   unsigned char mod_ws;

   unsigned char fb_c;

   unsigned char car_avekm;
   unsigned char car_ksl_tl;
   unsigned char car_ad;
   unsigned char car_sr;
   unsigned char car_ws;

*/
    uint16_t size;
    uint8_t transpose;

    unsigned mod_freq_mult:4;
    unsigned mod_env_scaling:1;
    unsigned mod_sustain_sound:1;
    unsigned mod_ampl_vibrato:1;
    unsigned mod_freq_vibrato:1;

    unsigned mod_out_lvl:6;
    unsigned mod_key_scale:2;

    unsigned mod_decay:4;
    unsigned mod_attack:4;

    unsigned mod_release:4;
    unsigned mod_sustain_lvl:4;

    uint8_t mod_waveform;

    unsigned feedback:7;
    unsigned connection:1;

    unsigned car_freq_mult:4;
    unsigned car_env_scaling:1;
    unsigned car_sustain_sound:1;
    unsigned car_ampl_vibrato:1;
    unsigned car_freq_vibrato:1;

    unsigned car_out_lvl:6;
    unsigned car_key_scale:2;

    unsigned car_decay:4;
    unsigned car_attack:4;

    unsigned car_release:4;
    unsigned car_sustain_lvl:4;

    uint8_t car_waveform;
};

void print_opl(opl2_patch p) {
    std::cout<<std::hex<<"Size: "<<int(p.size)<<" Transpose: "<<int(p.transpose)<<" Feedback: "<<int(p.feedback)<<" Connection: "<<int(p.connection)<<std::endl;
    std::cout<<"Carrier:\n\tFreq Mult: "<<int(p.car_freq_mult)<<" Envelope Scaling: "<<int(p.car_env_scaling)<<" Sustain: "<<int(p.car_sustain_sound)<<" AM Vibrato: "<<int(p.car_ampl_vibrato)<<std::endl;
    std::cout<<"\tFM Vibrato: "<<int(p.car_freq_vibrato)<<" Output Lvl: "<<int(p.car_out_lvl)<<" Key Scale: "<<int(p.car_key_scale)<<" Decay: "<<int(p.car_decay)<<std::endl;
    std::cout<<"\tAttack: "<<int(p.car_attack)<<" Release: "<<int(p.car_release)<<" Sustain Lvl: "<<int(p.car_sustain_lvl)<<" Carrier Waveform: "<<int(p.car_waveform)<<std::endl;

    std::cout<<"Modulator:\n\tFreq Mult: "<<int(p.mod_freq_mult)<<" Envelope Scaling: "<<int(p.mod_env_scaling)<<" Sustain: "<<int(p.mod_sustain_sound)<<" AM Vibrato: "<<int(p.mod_ampl_vibrato)<<std::endl;
    std::cout<<"\tFM Vibrato: "<<int(p.mod_freq_vibrato)<<" Output Lvl: "<<int(p.mod_out_lvl)<<" Key Scale: "<<int(p.mod_key_scale)<<" Decay: "<<int(p.mod_decay)<<std::endl;
    std::cout<<"\tAttack: "<<int(p.mod_attack)<<" Release: "<<int(p.mod_release)<<" Sustain Lvl: "<<int(p.mod_sustain_lvl)<<" Carrier Waveform: "<<int(p.mod_waveform)<<std::endl<<std::endl;
}

struct patchdat {
    patchdat() : bank(0), patch(0), name("N/A"), ad_patchdata(0), ad_patchdatastruct(), mt_patchdata(0) {};
    patchdat(uint8_t b, uint8_t p, std::vector<uint8_t> d, opl2_patch ad, std::string n = "N/A") : bank(b), patch(p), ad_patchdatastruct(ad), name(n) {
        //std::cout<<int(bank)<<": "<<int(patch)<<": ";
        if(d.size() == 0xf8) { mt_patchdata = d; /*std::cout<<"Init'd MT Patch data"<<std::endl;*/ }
        else { ad_patchdata = d; /*std::cout<<"Init'd Adlib Patch data"<<std::endl;*/ }
    };
    void setpat(std::vector<uint8_t> d) {
        //std::cout<<int(bank)<<": "<<int(patch)<<": ";
        if(d.size() == 0xf8) { mt_patchdata = d; /*std::cout<<"Added MT Patch data"<<std::endl;*/ }
        else { ad_patchdata = d;/* std::cout<<"Added Adlib Patch data"<<std::endl;*/ }
    }
    uint8_t bank;
    uint8_t patch;
    std::string name;
    std::vector<uint8_t> ad_patchdata;
    opl2_patch ad_patchdatastruct;
    std::vector<uint8_t> mt_patchdata;
};

bool load_patches(binifstream& in, std::vector<patchdat>& out) {
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
        for(auto it = out.begin(); it != out.end() && !found; ++it) {
            if((*it).bank == bank && (*it).patch == patch) {
                //std::cout<<"Adding data to already built patch structure"<<std::endl;
                (*it).setpat(data);
                (*it).name = std::string((name_str));
                found = true;
            }
        }
        if(!found) {
            //std::cout<<"Creating new patch structure"<<std::endl;
            out.push_back(patchdat(bank,patch,data,opl_data));
        }
        in.seekg(bookmark,std::ios::beg);
        //std::cout<<"Sought back to "<<bookmark<<" (the bookmark point)."<<std::endl;
    }
    in.close();
    return true;
}

bool load(std::vector<patchdat>& bank_data, std::string fna, std::string fnm = "") {
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
    if(!load_patches(ina, bank_data)) return false;
    if(fnm != "" && !load_patches(inm, bank_data)) return false;
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
        if((*it).ad_patchdata.size()==0xe)
             print_opl((*it).ad_patchdatastruct);
    }
    return true;
}
    
int main(int argc, char *argv[]) {
    std::vector<patchdat> bank_data;
    if(argc == 3) {
        load(bank_data, argv[1], argv[2]);
        return 0;
    }
    else if(argc == 2) {
        load(bank_data, argv[1]);
        return 0;
    }
    std::cout<<"I expect 2 arguments: Two AIL2 patch definition files (One for Adlib, one for MT-32)."<<std::endl;
    return 1;
}
