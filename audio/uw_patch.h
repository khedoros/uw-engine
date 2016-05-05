#ifndef UW_PATCH_H
#define UW_PATCH_H

#include<stdint.h>
#include<vector>

class binifstream;

class uw_patch_file {
public:
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

        unsigned mod_freq_mult:4;     //0x20
        unsigned mod_env_scaling:1;
        unsigned mod_sustain_sound:1;
        unsigned mod_ampl_vibrato:1;
        unsigned mod_freq_vibrato:1;
 
        unsigned mod_out_lvl:6;       //0x40
        unsigned mod_key_scale:2;

        unsigned mod_decay:4;         //0x60
        unsigned mod_attack:4;

        unsigned mod_release:4;       //0x80
        unsigned mod_sustain_lvl:4;

        uint8_t mod_waveform;         //0xe0

        unsigned connection:1;
        unsigned feedback:7;          //0xc0

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
private:
    bool load_patches(binifstream& in);
public:
    static void print_opl(opl2_patch p) {
       std::cout<<std::hex<<"Size: "<<int(p.size)<<" Transpose: "<<int(p.transpose)<<" Feedback: "<<int(p.feedback)<<" Connection: "<<int(p.connection)<<std::endl;
       std::cout<<"Carrier:\n\tFreq Mult: "<<int(p.car_freq_mult)<<" Envelope Scaling: "<<int(p.car_env_scaling)<<" Sustain: "<<int(p.car_sustain_sound)<<" AM Vibrato: "<<int(p.car_ampl_vibrato)<<std::endl;
       std::cout<<"\tFM Vibrato: "<<int(p.car_freq_vibrato)<<" Output Lvl: "<<int(p.car_out_lvl)<<" Key Scale: "<<int(p.car_key_scale)<<" Decay: "<<int(p.car_decay)<<std::endl;
       std::cout<<"\tAttack: "<<int(p.car_attack)<<" Release: "<<int(p.car_release)<<" Sustain Lvl: "<<int(p.car_sustain_lvl)<<" Carrier Waveform: "<<int(p.car_waveform)<<std::endl;

       std::cout<<"Modulator:\n\tFreq Mult: "<<int(p.mod_freq_mult)<<" Envelope Scaling: "<<int(p.mod_env_scaling)<<" Sustain: "<<int(p.mod_sustain_sound)<<" AM Vibrato: "<<int(p.mod_ampl_vibrato)<<std::endl;
       std::cout<<"\tFM Vibrato: "<<int(p.mod_freq_vibrato)<<" Output Lvl: "<<int(p.mod_out_lvl)<<" Key Scale: "<<int(p.mod_key_scale)<<" Decay: "<<int(p.mod_decay)<<std::endl;
       std::cout<<"\tAttack: "<<int(p.mod_attack)<<" Release: "<<int(p.mod_release)<<" Sustain Lvl: "<<int(p.mod_sustain_lvl)<<" Carrier Waveform: "<<int(p.mod_waveform)<<std::endl<<std::endl;
    }
 
    bool load(std::string fna, std::string fnm = "");

    std::vector<patchdat> bank_data;


};
#endif
