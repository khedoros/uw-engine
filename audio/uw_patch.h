#ifndef UW_PATCH_H
#define UW_PATCH_H

#include<stdint.h>
#include<vector>
#include<array>
#include<iostream>

class binifstream;

class uw_patch_file {
public:
    enum patchIndices {
        datasize = 0, // 16-bit value
        transpose = 2,
        mod_avekm,
        mod_ksl_tl,
        mod_ad,
        mod_sr,
        mod_ws,
        fb_c,
        car_avekm,
        car_ksl_tl,
        car_ad,
        car_sr,
        car_ws
    };

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
        uint8_t keyon_sr_1;
        uint8_t keyon_ad_1;
        uint8_t keyon_sr_0;
        uint8_t keyon_ad_0;

        uint8_t release_sr_1;
        uint8_t release_ad_1;
        uint8_t release_sr_0;
        uint8_t release_ad_0;
    };

    struct tvfx_patch {
        tvfx_patch() : init{0}, opt{0}, uses_opt(false), offset(0x36), update_data(0) {};
        tvfx_init init;
        tvfx_init_opt opt;
        bool uses_opt;
        uint16_t offset;
        std::vector<uint16_t> update_data;
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
       std::printf("Size: %04x Transpose: %02x Feedback: %02x Connection: %02x\n", p.size, p.transpose, p.feedback, p.connection);
       std::printf("Carrier:\n\tFreq Mult: %02x Envelope Scaling: %02x Sustain: %02x AM Vibrato: %02x\n", p.car_freq_mult, p.car_env_scaling, p.car_sustain_sound, p.car_ampl_vibrato);
       std::printf("\tFM Vibrato: %02x Output Lvl: %02x Key Scale: %02x Decay: %02x\n", p.car_freq_vibrato, p.car_out_lvl, p.car_key_scale, p.car_decay);
       std::printf("\tAttack: %02x Release: %02x Sustain Lvl: %02x Carrier Waveform: %02x\n", p.car_attack, p.car_release, p.car_sustain_lvl, p.car_waveform);

       std::printf("Modulator:\n\tFreq Mult: %02x Envelope Scaling: %02x Sustain: %02x AM Vibrato: %02x\n", p.mod_freq_mult, p.mod_env_scaling, p.mod_sustain_sound, p.mod_ampl_vibrato);
       std::printf("\tFM Vibrato: %02x Output Lvl: %02x Key Scale: %02x Decay: %02x\n", p.mod_freq_vibrato, p.mod_out_lvl, p.mod_key_scale, p.mod_decay);
       std::printf("\tAttack: %02x Release: %02x Sustain Lvl: %02x Carrier Waveform: %02x\n\n", p.mod_attack, p.mod_release, p.mod_sustain_lvl, p.mod_waveform);
    }
 
    static void print_tvfx(tvfx_patch p) {
        std::printf("Size: %04x\nTranspose: %02x\nType: %02x\nDuration (in 60Hz ticks): %d\n", p.init.size, p.init.transpose, p.init.type, p.init.duration);
        std::printf("Freq\tinit: %04x\tKeyon list offset: %04x\tRelease list offset: %04x\n", p.init.init_f_val, p.init.keyon_f_offset, p.init.release_f_offset);
        std::printf("Vol0\tinit: %04x\tKeyon list offset: %04x\tRelease list offset: %04x\n", p.init.init_v0_val, p.init.keyon_v0_offset, p.init.release_v0_offset);
        std::printf("Vol1\tinit: %04x\tKeyon list offset: %04x\tRelease list offset: %04x\n", p.init.init_v1_val, p.init.keyon_v1_offset, p.init.release_v1_offset);
        std::printf("Priority\tinit: %04x\tKeyon list offset: %04x\tRelease list offset: %04x\n", p.init.init_p_val, p.init.keyon_p_offset, p.init.release_p_offset);
        std::printf("Feedback\tinit: %04x\tKeyon list offset: %04x\tRelease list offset: %04x\n", p.init.init_fb_val, p.init.keyon_fb_offset, p.init.release_fb_offset);
        std::printf("M0\tinit: %04x\tKeyon list offset: %04x\tRelease list offset: %04x\n", p.init.init_m0_val, p.init.keyon_m0_offset, p.init.release_m0_offset);
        std::printf("M1\tinit: %04x\tKeyon list offset: %04x\tRelease list offset: %04x\n", p.init.init_m1_val, p.init.keyon_m1_offset, p.init.release_m1_offset);
        std::printf("Waveform\tinit: %04x\tKeyon list offset: %04x\tRelease list offset: %04x\n", p.init.init_ws_val, p.init.keyon_ws_offset, p.init.release_ws_offset);
        if(p.uses_opt) {
            std::cout<<"\nContains 8 bytes of extra data to change default adsr values:";
            std::printf("\n\tKeyOn AD1: %02x\n\tKeyOn SR1: %02x\n\tKeyOn AD0: %02x\n\tKeyOn SR0: %02x\n", p.opt.keyon_ad_1, p.opt.keyon_sr_1, p.opt.keyon_ad_0, p.opt.keyon_sr_0);
            std::printf("\n\tRelease AD1: %02x\n\tRelease SR1: %02x\n\tRelease AD0: %02x\n\tRelease SR0: %02x\n", p.opt.release_ad_1, p.opt.release_sr_1, p.opt.release_ad_0, p.opt.release_sr_0);
        }
        std::printf("List data dump (first word is at byte offset %04x)\n", p.offset);
        for(auto i: p.update_data) {
            std::printf("%04x ", i);
        }
        std::cout<<"\n";
        std::array<std::string, 8> labels {"Frequency","TL0","TL1","Priority","Feedback","Mult0","Mult1","Waveform"};
        std::array<uint16_t, 8> offsets {p.init.keyon_f_offset,p.init.keyon_v0_offset,p.init.keyon_v1_offset,p.init.keyon_p_offset,
                                          p.init.keyon_fb_offset,p.init.keyon_m0_offset,p.init.keyon_m1_offset,p.init.keyon_ws_offset};
        for(int element = 0; element < 8; element++) {
            std::cout<<labels[element]<<": ";
            uint16_t dataOffset = (offsets[element] - p.init.keyon_f_offset) / 2;
            for(int c=0;c<10;c++) {
                uint16_t command = p.update_data[dataOffset+0];
                int16_t data = p.update_data[dataOffset+1];
                if(command == 0) {
                    dataOffset += (data / 2);
                    std::printf("increment offset by %d, ", data/2);
                }
                else {
                    dataOffset += 2;
                    if(command == 0xffff) {
                        std::printf("val=%04x, ", data);
                    }
                    else if(command == 0xfffe) { // These each have a different exact effect, depending on the 
                        switch(element) {
                            case 0:
                                std::printf("block=%02x, ", data>>8);
                                break;
                            case 1:
                                std::printf("KSLTL_0=%02x, ", data&0xff);
                                break;
                            case 2:
                                std::printf("KSLTL_1=%02x, ", data&0xff);
                                break;
                            case 3:
                                std::printf("priority no-op, ");
                                break;
                            case 4:
                                std::printf("FBC=%02x", data>>8);
                                break;
                            case 5:
                                std::printf("AVEKM_0=%02x, ", data&0xff);
                                break;
                            case 6:
                                std::printf("AVEKM_1=%02x, ", data&0xff);
                                break;
                            case 7:
                                std::printf("waveform no-op, ");
                                break;
                        }
                    }
                    else {
                        std::printf("count=%04x increment=%04x (break) -> ", command, data);
                    }
                }
            }
            std::cout<<" loop done (break)\n";
        }
    }

    bool load(std::string fna, std::string fnm = "");
    static std::string getName(int bank, int patch);

    std::vector<patchdat> bank_data;


};
#endif
