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
    
    struct __attribute__((packed)) tvfx_init {
        uint16_t size;
        uint8_t transpose;
        uint8_t type;
        uint16_t duration;

        uint16_t init_f_val;
        uint16_t keyon_f_offset;
        uint16_t release_f_offset;

        uint16_t init_v0_val;
        uint16_t keyon_v0_offset;
        uint16_t release_v0_offset;

        uint16_t init_v1_val;
        uint16_t keyon_v1_offset;
        uint16_t release_v1_offset;

        uint16_t init_p_val;
        uint16_t keyon_p_offset;
        uint16_t release_p_offset;

        uint16_t init_fb_val;
        uint16_t keyon_fb_offset;
        uint16_t release_fb_offset;

        uint16_t init_m0_val;
        uint16_t keyon_m0_offset;
        uint16_t release_m0_offset;

        uint16_t init_m1_val;
        uint16_t keyon_m1_offset;
        uint16_t release_m1_offset;

        uint16_t init_ws_val;
        uint16_t keyon_ws_offset;
        uint16_t release_ws_offset;
    };

    struct __attribute__((packed)) tvfx_init_opt {
        uint8_t keyon_ad_1;
        uint8_t keyon_sr_1;
        uint8_t keyon_ad_0;
        uint8_t keyon_sr_0;

        uint8_t release_ad_1;
        uint8_t release_sr_1;
        uint8_t release_ad_0;
        uint8_t release_sr_0;
    };

    struct tvfx_patch {
        tvfx_patch() : init{0}, opt{0}, uses_opt(false), offset(0x36), update_data(0) {};
        tvfx_init init;
        tvfx_init_opt opt;
        bool uses_opt;
        size_t offset;
        std::vector<int16_t> update_data;
    };

    struct patchdat {
        patchdat() : bank(0), patch(0), name(""), has_opl2(false), has_tvfx(false), has_mt(false), ad_patchdatastruct{0}, tv_patchdatastruct(), ad_patchdata(0), mt_patchdata(0) {};
        void setpat(std::vector<uint8_t> d,uint8_t b, uint8_t p);
        uint8_t bank;
        uint8_t patch;
        std::string name;
        bool has_opl2;
        bool has_tvfx;
        bool has_mt;
        opl2_patch ad_patchdatastruct;
        tvfx_patch tv_patchdatastruct;
        std::vector<uint8_t> ad_patchdata;
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
 
    static void print_tvfx(tvfx_patch p) {
       std::cout<<std::hex<<"Size: "<<p.init.size<<"\nTranspose: "<<int(p.init.transpose)<<"\nType: "<<int(p.init.type)<<"\nDuration (in 120Hz ticks): "<<p.init.duration<<std::endl;
       std::cout<<"Freq\tinit: "<<p.init.init_f_val<<"\tKeyon list: "<<p.init.keyon_f_offset<<"\tRelease list: "<<p.init.release_f_offset<<std::endl;
       std::cout<<"Vol0\tinit: "<<p.init.init_v0_val<<"\tKeyon list: "<<p.init.keyon_v0_offset<<"\tRelease list: "<<p.init.release_v0_offset<<std::endl;
       std::cout<<"Vol1\tinit: "<<p.init.init_v1_val<<"\tKeyon list: "<<p.init.keyon_v1_offset<<"\tRelease list: "<<p.init.release_v1_offset<<std::endl;
       std::cout<<"Priority\tinit: "<<p.init.init_p_val<<"\tKeyon list: "<<p.init.keyon_p_offset<<"\tRelease list: "<<p.init.release_p_offset<<std::endl;
       std::cout<<"Feedback\tinit: "<<p.init.init_fb_val<<"\tKeyon list: "<<p.init.keyon_fb_offset<<"\tRelease list: "<<p.init.release_fb_offset<<std::endl;
       std::cout<<"M0\tinit: "<<p.init.init_m0_val<<"\tKeyon list: "<<p.init.keyon_m0_offset<<"\tRelease list: "<<p.init.release_m0_offset<<std::endl;
       std::cout<<"M1\tinit: "<<p.init.init_m1_val<<"\tKeyon list: "<<p.init.keyon_m1_offset<<"\tRelease list: "<<p.init.release_m1_offset<<std::endl;
       std::cout<<"Waveform\tinit: "<<p.init.init_ws_val<<"\tKeyon list: "<<p.init.keyon_ws_offset<<"\tRelease list: "<<p.init.release_ws_offset<<std::endl;
       if(p.uses_opt) std::cout<<"\nContains 8 bytes of extra data to change default adsr values:";
       std::cout<<"\n\tKeyOn AD1: "<<int(p.opt.keyon_ad_1)<<"\n\tKeyOn SR1: "<<int(p.opt.keyon_sr_1)<<"\n\tKeyOn AD0: "<<int(p.opt.keyon_ad_0)<<"\n\tKeyOn SR0: "<<int(p.opt.keyon_sr_0);
       std::cout<<"\n\tRelease AD1: "<<int(p.opt.release_ad_1)<<"\n\tRelease SR1: "<<int(p.opt.release_sr_1)<<"\n\tRelease AD0: "<<int(p.opt.release_ad_0)<<"\n\tRelease SR0: "<<int(p.opt.release_sr_0)<<std::endl;
       std::cout<<"List data dump (first word is at byte offset "<<p.offset<<"): "<<std::endl;
       for(auto i: p.update_data) {
           std::cout<<i<<" ";
       }
       std::cout<<std::endl<<std::dec;
    }

    bool load(std::string fna, std::string fnm = "");

    std::vector<patchdat> bank_data;


};
#endif
