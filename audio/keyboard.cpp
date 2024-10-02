#include<iostream>
#include<tuple>
#include<vector>
#include<array>
#include<cstdint>
#include<memory>
#include<unordered_map>
#include<SDL2/SDL.h>
#include "opl.h"
#include "oplStream.h"
#include "uw_patch.h"

std::vector<std::tuple<uint8_t,uint8_t,uint16_t>> freqs;

//Mathematically calculate the best OPL settings to match Midi frequencies
//Outputs the best matches into the freqs vector of 3-tuples.
void calc_freqs() {
    double base_freq = 440.0;
    uint8_t base_mid_num = 69;
    for(uint16_t mid_num = 0; mid_num < 128; ++mid_num) {
        double midi_freq = base_freq * pow(2.0, (mid_num - base_mid_num)/12.0);
        //cout<<"MIDI Number: "<<mid_num<<" Frequency: "<<midi_freq<<'\n';
        double diff = 9999999999.0;
        uint8_t blk = 0;
        uint16_t f_num = 0;
        for(uint32_t block = 0; block < 8; ++block) {
            for(uint32_t f_number = 0; f_number < 1024; ++f_number) {
                double opl_freq = double(f_number * /*49716*/ NATIVE_OPL_SAMPLE_RATE ) / pow(2.0, 20 - double(block));
                if(abs(opl_freq - midi_freq) < diff) {
                    diff = abs(opl_freq - midi_freq);
                    f_num = f_number;
                    blk = block;
                }
            }
        }
        if(diff < 10) {
            //cout<<" OPL_Blk: "<<uint16_t(blk)<<" F-Num: "<<f_num<<" OPL Freq: "<<OPL_freq<<'\n';
            freqs.push_back(std::make_tuple(mid_num,blk,f_num));
        }
        else {
            //cout<<" OPL: Out of Range\n";
        }
    }
}

//Data and methods having to do with current use of OPL channels, voice assignments, etc
const int OPL_VOICE_COUNT = 9;
std::array<int8_t, OPL_VOICE_COUNT> voice_midi_num { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
std::array<int8_t, OPL_VOICE_COUNT> voice_velocity {127, 127, 127, 127, 127, 127, 127, 127, 127 };
std::array<uw_patch_file::patchdat*, OPL_VOICE_COUNT> voice_patch;
std::array<int8_t, OPL_VOICE_COUNT> voice_modulation {  0,  0,  0,  0,  0,  0,  0,  0,  0 };
std::array<int8_t, OPL_VOICE_COUNT> voice_volume     {127,127,127,127,127,127,127,127,127};
std::array<int8_t, OPL_VOICE_COUNT> voice_expression {127,127,127,127,127,127,127,127,127};

const std::array<uint8_t, OPL_VOICE_COUNT> voice_base_mod {  0,  1,  2,  8,  9,0xa,0x10,0x11,0x12};
const std::array<uint8_t, OPL_VOICE_COUNT> voice_base_car {  3,  4,  5,0xb,0xc,0xd,0x13,0x14,0x15};
const std::array<uint8_t, OPL_VOICE_COUNT> voice_base2    {  0,  1,  2,  3,  4,  5,   6,   7,   8};

enum timbre_type {
    BNK_INST = 0,                       // S_type[] equates
    TV_INST = 1,
    TV_EFFECT = 2,
    OPL3_INST = 3
};

// Sound effect tracking data
enum tvfxOffset {
    freq,
    level0,
    level1,
    priority,
    feedback,
    mult0,
    mult1,
    waveform
};
const int TVFX_ELEMENT_COUNT = 8;

struct tvfxElement {
    uint16_t offset;
    uint16_t counter;
    uint16_t value;
    uint16_t increment;
};

std::array<std::array<tvfxElement, TVFX_ELEMENT_COUNT>, OPL_VOICE_COUNT> tvfxElements;

std::array<uint8_t, OPL_VOICE_COUNT> S_kbf_shadow; // shadowed KON-BLOCK-FNUM(H) registers
std::array<uint8_t, OPL_VOICE_COUNT> S_block;      // KON/BLOCK values
std::array<uint8_t, OPL_VOICE_COUNT> S_fbc;
std::array<uint8_t, OPL_VOICE_COUNT> S_ksltl_0;
std::array<uint8_t, OPL_VOICE_COUNT> S_ksltl_1;
std::array<uint8_t, OPL_VOICE_COUNT> S_avekm_0;
std::array<uint8_t, OPL_VOICE_COUNT> S_avekm_1;

std::array<uint16_t, OPL_VOICE_COUNT> tvfx_duration;
enum tvfxStatus {
    FREE, KEYON, KEYOFF
};
std::array<int, OPL_VOICE_COUNT> tvfx_status;
std::array<uint8_t, OPL_VOICE_COUNT> tvfx_update;

#define U_FREQ (1<<freq)
#define U_LEVEL0 (1<<level0)
#define U_LEVEL1 (1<<level1)
#define U_PRIORITY (1<<priority)
#define U_FEEDBACK (1<<feedback)
#define U_MULT0 (1<<mult0)
#define U_MULT1 (1<<mult1)
#define U_WAVEFORM (1<<waveform)
#define U_ALL (U_FREQ|U_LEVEL0|U_LEVEL1|U_PRIORITY|U_FEEDBACK|U_MULT0|U_MULT1|U_WAVEFORM)

const int MIDI_CHANNEL_COUNT = 16;


const std::array<int8_t, 16> velocity_translation { 0x52, 0x55, 0x58, 0x5b, 0x5e, 0x61, 0x64, 0x67, 0x6a, 0x6d, 0x70, 0x73, 0x76, 0x79, 0x7c, 0x7f };

enum OPL_addresses {
    TEST       = 0x01, //Set to 0
    TIMER1     = 0x02, //'      '
    TIMER2     = 0x03, //'      '
    TIMER_CTRL = 0x04, //'      '
    NOTE_SEL   = 0x05, 
    AVEKM = 0x20,
    KSL_TL     = 0x40,
    AD    = 0x60,
    SR    = 0x80,
    F_NUM_L    = 0xa0,
    ON_BLK_NUM = 0xb0,
    TREM_VIB   = 0xbd, //Set to 0xc0
    FB_C   = 0xc0,
    WS   = 0xe0
};

//Find the note entry for the given channel and note#
//-1 means "note not found"
int8_t find_playing_note(int8_t note) {
    for(int i=0;i<OPL_VOICE_COUNT;++i) {
        if(voice_midi_num[i] == note)
            return i;
    }
    return -1;
}

//Find the first voice that's currently empty
//-1 means 'all in use'
int8_t find_unused_voice() {
    for(int i=0;i<OPL_VOICE_COUNT;++i) {
        if(voice_midi_num[i] == -1)
            return i;
    }
    return -1;
}

// Write proper volume for the given voice, taking into account the patch's TL, note velocity, channel volume and expression.
void writeVolume(oplStream& opl, int voice_num) {
    auto patch = voice_patch[voice_num];
    int8_t velocity = voice_velocity[voice_num];

    uint16_t vol = (uint16_t(voice_volume[voice_num]) * voice_expression[voice_num])>>7;
    vol *= velocity; vol >>= 7;

    uint8_t connection = patch->ad_patchdatastruct.connection;
    if(connection) { // additive operator, so scale modulating operator too
        uint8_t mod_tl = patch->ad_patchdatastruct.mod_out_lvl;
        mod_tl = (~mod_tl) & 0x3f;
        uint8_t mod_ksl = patch->ad_patchdatastruct.mod_key_scale;
        uint16_t mod_vol = ~((vol * mod_tl) / 127);

        opl.WriteReg(voice_base_mod[voice_num]+KSL_TL,(mod_vol & 0x3f) +
                                                      ((mod_ksl & 0x3)<<(6)));
    }

    uint8_t car_tl = patch->ad_patchdatastruct.car_out_lvl;
    car_tl = (~car_tl) & 0x3f;
    uint8_t car_ksl = patch->ad_patchdatastruct.car_key_scale;
    uint16_t car_vol = ~((vol * car_tl) / 127);

    opl.WriteReg(voice_base_car[voice_num]+KSL_TL,((car_vol & 0x3f) +
                                                   ((car_ksl & 0x3)<<(6))));
}

// Sets up the voice for the tvfx keyon and keyoff stages
bool switch_tvfx_phase(oplStream& opl, int voice) {
    if(voice == -1 || voice >= OPL_VOICE_COUNT) {
        std::cerr<<"Invalid voice\n";
        return false;
    }

    bool keyOn = (tvfx_status[voice] == KEYON);

    auto& pat = voice_patch[voice]->tv_patchdatastruct;

    //designate initial values for the modulator and carrier:
    S_fbc[voice] = 0;        // FM synth, no feedback; feedback comes from time-variant part)
    S_ksltl_0[voice] = 0;    // volume=zero, ksl=0 (TL will typically come from the time-variant part)
    S_ksltl_1[voice] = 0;    // volume=zero, ksl=0
    S_avekm_0[voice] = 0x20; // SUSTAIN=1, AM=0, FM=0, Mult=0 (Mult will typically come from TV part)
    S_avekm_1[voice] = 0x20; // SUSTAIN=1, AM=0, FM=0, Mult=0
    
    uint8_t timbreType = pat.init.type;

    if(timbreType != TV_INST) {
        S_block[voice] = 0x28;
    }
    else {
        S_block[voice] = 0x20;
    }

    if(!pat.uses_opt) { // Apply default ADSR values
        std::printf("default adsr %02x%02x %02x%02x\n", 0xff, 0x0f, 0xff, 0x0f);
        opl.WriteReg(voice_base_mod[voice]+AD, 0xff);
        opl.WriteReg(voice_base_mod[voice]+SR, 0x0f);
        opl.WriteReg(voice_base_car[voice]+AD, 0xff);
        opl.WriteReg(voice_base_car[voice]+SR, 0x0f);
    }
    else {
        if(keyOn) { // ADSR values defined by the sound effect timbre
            std::printf("keyon adsr %02x%02x %02x%02x\n", pat.opt.keyon_ad_0, pat.opt.keyon_sr_0, pat.opt.keyon_ad_1, pat.opt.keyon_sr_1);
            opl.WriteReg(voice_base_mod[voice]+AD, pat.opt.keyon_ad_0);
            opl.WriteReg(voice_base_mod[voice]+SR, pat.opt.keyon_sr_0);
            opl.WriteReg(voice_base_car[voice]+AD, pat.opt.keyon_ad_1);
            opl.WriteReg(voice_base_car[voice]+SR, pat.opt.keyon_sr_1);
        }
        else {
            std::printf("keyoff adsr %02x%02x %02x%02x\n", pat.opt.release_ad_0, pat.opt.release_sr_0, pat.opt.release_ad_1, pat.opt.release_sr_1);
            opl.WriteReg(voice_base_mod[voice]+AD, pat.opt.release_ad_0);
            opl.WriteReg(voice_base_mod[voice]+SR, pat.opt.release_sr_0);
            opl.WriteReg(voice_base_car[voice]+AD, pat.opt.release_ad_1);
            opl.WriteReg(voice_base_car[voice]+SR, pat.opt.release_sr_1);
        }
    }

    // Original offsets were based on byte offsets in the whole timbre
    // The indices here only include the command lists themselves, and address 16-bit words.
    // These values are the key to the "time-variant effects"
    if(keyOn) {
        tvfx_duration[voice] = pat.init.duration + 1;

        tvfxElements[voice][freq].offset = (pat.init.keyon_f_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][level0].offset = (pat.init.keyon_v0_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][level1].offset = (pat.init.keyon_v1_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][priority].offset = (pat.init.keyon_p_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][feedback].offset = (pat.init.keyon_fb_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][mult0].offset = (pat.init.keyon_m0_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][mult1].offset = (pat.init.keyon_m1_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][waveform].offset = (pat.init.keyon_ws_offset - pat.init.keyon_f_offset) / 2;

        tvfxElements[voice][freq].value = pat.init.init_f_val;
        tvfxElements[voice][level0].value = pat.init.init_v0_val;
        tvfxElements[voice][level1].value = pat.init.init_v1_val;
        tvfxElements[voice][priority].value = pat.init.init_p_val;
        tvfxElements[voice][feedback].value = pat.init.init_fb_val;
        tvfxElements[voice][mult0].value = pat.init.init_m0_val;
        tvfxElements[voice][mult1].value = pat.init.init_m1_val;
        tvfxElements[voice][waveform].value = pat.init.init_ws_val;
    }
    else {
        tvfxElements[voice][freq].offset = (pat.init.release_f_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][level0].offset = (pat.init.release_v0_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][level1].offset = (pat.init.release_v1_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][priority].offset = (pat.init.release_p_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][feedback].offset = (pat.init.release_fb_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][mult0].offset = (pat.init.release_m0_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][mult1].offset = (pat.init.release_m1_offset - pat.init.keyon_f_offset) / 2;
        tvfxElements[voice][waveform].offset = (pat.init.release_ws_offset - pat.init.keyon_f_offset) / 2;
    }

    for(int element = 0; element < TVFX_ELEMENT_COUNT; element++) {
        tvfxElements[voice][element].counter = 1;
        tvfxElements[voice][element].increment = 0;
    }

    tvfx_update[voice] = U_ALL;
    return true;
}

// Write current TVFX state out to the OPL hardware
void tvfx_update_voice(oplStream& opl, int voice) {
    if(tvfx_update[voice] & U_MULT0) {
        uint16_t m0_val = tvfxElements[voice][mult0].value;
        uint8_t AVEKM_0 = S_avekm_0[voice];
        m0_val >>= 12; // take the top 4 bits
        opl.WriteReg(voice_base_mod[voice]+AVEKM, m0_val | AVEKM_0);
        tvfx_update[voice] &= (~U_MULT0);
    }
    if(tvfx_update[voice] & U_MULT1) {
        uint16_t m1_val = tvfxElements[voice][mult1].value;
        uint8_t AVEKM_1 = S_avekm_1[voice];
        m1_val >>= 12; // take the top 4 bits
        opl.WriteReg(voice_base_car[voice] + AVEKM, m1_val | AVEKM_1);
        tvfx_update[voice] &= (~U_MULT1);
    }
    if(tvfx_update[voice] & U_LEVEL0) {
        uint8_t KSLTL_0 = S_ksltl_0[voice];
        uint16_t v0_val = tvfxElements[voice][level0].value;
        v0_val >>= 10; // take the top 6 bits
        v0_val = (~v0_val) & 0x3f;
        v0_val |= KSLTL_0;
        opl.WriteReg(voice_base_mod[voice] + KSL_TL, v0_val);
        tvfx_update[voice] &= (~U_LEVEL0);
    }
    if(tvfx_update[voice] & U_LEVEL1) {
        uint8_t KSLTL_1 = S_ksltl_1[voice];
        uint16_t v1_val = tvfxElements[voice][level1].value;
        v1_val >>= 10; // take the top 6 bits
        v1_val = (~v1_val) & 0x3f;
        v1_val |= KSLTL_1;
        opl.WriteReg(voice_base_car[voice] + KSL_TL, v1_val);
        tvfx_update[voice] &= (~U_LEVEL1);
    }
    if(tvfx_update[voice] & U_WAVEFORM) {            //   __WS:   mov al,BYTE PTR ws_val
        uint16_t ws_val = tvfxElements[voice][waveform].value;
        opl.WriteReg(voice_base_mod[voice] + WS, ws_val >> 8);
        opl.WriteReg(voice_base_car[voice] + WS, (ws_val & 0xff));
        tvfx_update[voice] &= (~U_WAVEFORM);         //           call write_register C,voice0,0e0h,ax
    }
    if(tvfx_update[voice] & U_FEEDBACK) {
        uint16_t fb_val = tvfxElements[voice][feedback].value;
        uint8_t FBC = S_fbc[voice];
        fb_val >>= 12; // Take the top 4 bits
        fb_val &= 0b1110;
        int fbc = FBC & 1;
        opl.WriteReg(voice_base2[voice] + FB_C, fb_val | fbc);
        tvfx_update[voice] &= (~U_FEEDBACK);
    }
    if(tvfx_update[voice] & U_FREQ) {
        uint16_t f_num = (tvfxElements[voice][freq].value >> 6);
        opl.WriteReg(voice_base2[voice] + F_NUM_L, f_num & 0xff);

        S_kbf_shadow[voice] = ((f_num >> 8) | S_block[voice]);
        opl.WriteReg(voice_base2[voice] + ON_BLK_NUM, S_kbf_shadow[voice]);
        tvfx_update[voice] &= (~U_FREQ);
    }   
}

// Do the next step in the command list for a specific time-variant element
bool iterateTvfxCommandList(oplStream& opl, int voice, tvfxOffset element) {
    uw_patch_file::patchdat* patchDat = voice_patch[voice];
    auto& offset = tvfxElements[voice][element].offset;
    auto& value = tvfxElements[voice][element].value;
    auto& increment = tvfxElements[voice][element].increment;
    auto& counter = tvfxElements[voice][element].counter;

    bool valChanged = false;
    //std::cout<<"iterateTvfxCommandList()\n";
    for(int iter = 0; iter < 10; iter++) {
        uint16_t command = patchDat->tv_patchdatastruct.update_data[offset+0];
        uint16_t data = patchDat->tv_patchdatastruct.update_data[offset+1];
        //std::printf("iteration: %d command: %04x data: %04x\n", iter, command, data);
        if(command == 0) {
            offset += (data / 2);
        }
        else {
            offset += 2;
            if(command == 0xffff) {
                value = data;
                valChanged = true;
                tvfx_update[voice] |= (1 << element);
            }
            else if(command == 0xfffe) {
                valChanged = true;
                if(element == freq) {
                    S_block[voice] = (data >> 8);
                    tvfx_update[voice] |= U_FREQ;
                }
                else if(element == level0) {
                    S_ksltl_0[voice] = data & 0xff;
                    tvfx_update[voice] |= U_LEVEL0;
                }
                else if(element == level1) {
                    S_ksltl_1[voice] = data & 0xff;
                    tvfx_update[voice] |= U_LEVEL1;
                }
                else if(element == feedback) {
                    S_fbc[voice] = (data >> 8);
                    tvfx_update[voice] |= U_FEEDBACK;
                }
                else if(element == mult0) {
                    S_avekm_0[voice] = (data & 0xff);
                    tvfx_update[voice] |= U_MULT0;
                }
                else if(element == mult1) {
                    S_avekm_1[voice] = (data & 0xff);
                    tvfx_update[voice] |= U_MULT1;
                }
            }
            else {
                counter = command;
                increment = data;
                return valChanged;
            }
        }
    }
    return valChanged;
}

// Free a voice from tvfx control when the sound effect has ended
void tvfx_note_free(oplStream& opl, int voice) {
    voice_midi_num[voice] = -1;

    std::cout<<"tvfx free voice "<<voice<<"\n";
    S_kbf_shadow[voice] = (S_kbf_shadow[voice] & (~0x20));
    S_block[voice] = (S_block[voice] & (~0x20));
    opl.WriteReg(voice_base2[voice]+ON_BLK_NUM, S_kbf_shadow[voice]);
    tvfx_status[voice] = FREE;
}

// KEYOFF a tvfx effect when duration has been reached
void tvfx_note_off(oplStream& opl, int voice) {
    std::cout<<"tvfx note-off, voice "<<voice<<"\n";
    tvfx_status[voice] = KEYOFF;
    switch_tvfx_phase(opl, voice);
}

// Iterate TVFX voices for duration, value increment for each element, and iterate command lists when count hits 0.
// Also monitors volume for the end of the sound effect. Needs to be called at 60Hz.
void iterateTvfx(oplStream& opl) {
    for(int voice = 0; voice < OPL_VOICE_COUNT; voice++) {
        if(!voice_patch[voice] || voice_patch[voice]->bank != 1 || voice_midi_num[voice] == -1) continue;
        if(tvfx_duration[voice]) {
            tvfx_duration[voice]--;
        }
        else if(tvfx_status[voice] == KEYON) {
            tvfx_note_off(opl, voice);
        }

        for(uint element = 0; element < TVFX_ELEMENT_COUNT; element++) {
            bool changed = false;

            if(tvfxElements[voice][element].increment != 0) {
                changed = true;
                uint16_t previous = tvfxElements[voice][element].value;
                tvfxElements[voice][element].value += tvfxElements[voice][element].increment;

                if(tvfx_status[voice] == KEYOFF && (element == level0 || element == level1)) {
                    uint16_t newVal = tvfxElements[voice][element].value;
                    if((previous ^ newVal) >= 0x8000 && (newVal ^ tvfxElements[voice][element].increment) < 0x8000) {
                        tvfxElements[voice][element].value = 0;
                    }
                }
            }

            tvfxElements[voice][element].counter--;
            //std::cout<<"Counter for "<<element<<": "<<*counters[element]<<"\n";
            if(!tvfxElements[voice][element].counter) {
                changed = iterateTvfxCommandList(opl, voice, static_cast<tvfxOffset>(element));
            }

            if(changed) {
                tvfx_update[voice] |= (1<<element);
            }
        }
        if(tvfx_status[voice] == KEYOFF && 
           static_cast<int16_t>(tvfxElements[voice][level0].value) < 0x400 && 
           static_cast<int16_t>(tvfxElements[voice][level1].value) < 0x400) {
            tvfx_note_free(opl, voice);
        }
        else if(tvfx_status[voice] == KEYOFF) {
            int16_t l0 = tvfxElements[voice][level0].value;
            int16_t l1 = tvfxElements[voice][level1].value;
            std::printf("%04x %04x, %d %d\n", tvfxElements[voice][level0].value, tvfxElements[voice][level1].value, l0, l1);
        }

        if(tvfx_update[voice] != 0) {
            tvfx_update_voice(opl, voice);
        }
    }
//serve_synth called at 120Hz, services tvfx at 60Hz, updates priority at 24Hz.
//For each TVFX slot, decrement all the counters, apply value increments, mark for voice update. If volume == 0 for a slot, TVFX_increment_stage. If anything marked for voice update, tvfx_update_voice(slot).
//If KEYON and duration>0, decrement duration, otherwise, set KEYOFF and run TV_phase(slot).
//If either volume value is above 0x400, then continue, otherwise release_voice(slot), S_status[slot]=FREE, TVFX_switch_voice()
}

//Copies the given simple instrument patch data into the given voice slot
bool copy_bnk_patch(oplStream& opl, int voice) {
    if(voice == -1 || voice >= OPL_VOICE_COUNT) {
        std::cerr<<"Invalid voice\n";
        return false;
    }

    std::vector<uint8_t>& pat = voice_patch[voice]->ad_patchdata;
    bool am = voice_patch[voice]->ad_patchdatastruct.connection;

    //Write the values to the modulator:
    uint8_t mod_avekm = pat[uw_patch_file::patchIndices::mod_avekm];
    mod_avekm &= 0b10111111;
    mod_avekm |= ((voice_modulation[voice] > 64) ? 0b01000000 : 0);
    opl.WriteReg(voice_base_mod[voice]+AVEKM, mod_avekm);
    if(!am) { // For FM patch connection, modulator volume comes straight from the patch data
        opl.WriteReg(voice_base_mod[voice]+KSL_TL,pat[uw_patch_file::patchIndices::mod_ksl_tl]);
    }
    opl.WriteReg(voice_base_mod[voice]+AD,pat[uw_patch_file::patchIndices::mod_ad]);
    opl.WriteReg(voice_base_mod[voice]+SR,pat[uw_patch_file::patchIndices::mod_sr]);
    opl.WriteReg(voice_base_mod[voice]+WS,pat[uw_patch_file::patchIndices::mod_ws]);

    //Write the values to the carrier:
    uint8_t car_avekm = pat[uw_patch_file::patchIndices::car_avekm];
    car_avekm &= 0b10111111;
    car_avekm |= ((voice_modulation[voice] > 64) ? 0b01000000 : 0);
    opl.WriteReg(voice_base_car[voice]+AVEKM, car_avekm);
    // opl->WriteReg(voice_base_car[voice]+KSL_TL,pat[uw_patch_file::patchIndices::car_ksl_tl]);
    opl.WriteReg(voice_base_car[voice]+AD,pat[uw_patch_file::patchIndices::car_ad]);
    opl.WriteReg(voice_base_car[voice]+SR,pat[uw_patch_file::patchIndices::car_sr]);
    opl.WriteReg(voice_base_car[voice]+WS,pat[uw_patch_file::patchIndices::car_ws]);

    //Write connection and feedback:
    opl.WriteReg(voice_base2[voice]+FB_C,pat[uw_patch_file::patchIndices::fb_c]);

    //Calculate and write volume levels:
    writeVolume(opl, voice);
    return true;
}

void note_off(oplStream& opl, int midi_num) {
    if(midi_num < 12 || midi_num > 108) return; // unsupported note range
    //Look up the voice playing the note, and the block+f_num values
    int voice_index = find_playing_note(midi_num);
    if(voice_index == -1) return;

    // tvfx voices (played on bank 1) end after some time, not at a key-off
    int voice_bank = voice_patch[voice_index]->bank;
    if(voice_bank == 1) return;

    voice_midi_num[voice_index] = -1;

    int block = std::get<1>(freqs[midi_num]);
    int f_num = std::get<2>(freqs[midi_num]);
    opl.WriteReg(voice_base2[voice_index]+ON_BLK_NUM, (block<<(2)) + ((f_num&0xff00)>>(8)));
}

void note_on(oplStream& opl, int midi_num, int velocity, uw_patch_file::patchdat* patchDat) {
    if(midi_num < 12 || midi_num > 108) return; // unsupported note range
    int voice_num = find_unused_voice();
    if(voice_num == -1) {
        std::cout<<"No free voice, dropping a note.\n";
        return;
    }

    voice_midi_num[voice_num] = midi_num;
    voice_velocity[voice_num] = velocity_translation[velocity>>3];
    voice_patch[voice_num] = patchDat;

    bool retval = false;
    if(voice_patch[voice_num]->bank == 1) {
        std::cout<<"Copying the patch\n";
        tvfx_status[voice_num] = KEYON;
        retval = switch_tvfx_phase(opl, voice_num);
    }
    else {
        retval = copy_bnk_patch(opl, voice_num);
    }
    if(!retval) {
        std::cout<<"Had trouble copying "<<int(voice_patch[voice_num]->bank)<<":"<<int(voice_patch[voice_num]->patch)
                    <<" to voice "<<int(voice_num)<<". Dropping the note.\n";
        return;
    }

    if(voice_patch[voice_num]->bank != 1) {
        int block = std::get<1>(freqs[midi_num]);
        int f_num = std::get<2>(freqs[midi_num]);

        opl.WriteReg(voice_base2[voice_num]+F_NUM_L, (f_num&0xff));
        opl.WriteReg(voice_base2[voice_num]+ON_BLK_NUM, 0x20 + (block<<(2)) + ((f_num&0xff00)>>(8)));
        writeVolume(opl, voice_num);
    }
}

std::unordered_map<int, int> keyMap {
    {SDLK_a, 48},
    {SDLK_w, 49},
    {SDLK_s, 50},
    {SDLK_e, 51},
    {SDLK_d, 52},
    {SDLK_f, 53},
    {SDLK_t, 54},
    {SDLK_g, 55},
    {SDLK_y, 56},
    {SDLK_h, 57},
    {SDLK_u, 58},
    {SDLK_j, 59},
    {SDLK_k, 60},
    
};

int update_patch(oplStream& opl, int voice, uw_patch_file& uwpf,int bankNum, int patchNum) {
    for(int p = 0; p < uwpf.bank_data.size(); p++) {
        auto& patch = uwpf.bank_data[p];
        if(patch.bank == bankNum && patch.patch == patchNum) {
            std::cout<<"Copying "<<bankNum<<":"<<patchNum<<" ("<<patch.name<<") to voice "<<voice<<"\n";
            voice_patch[voice] = &patch;
            note_off(opl, voice_midi_num[voice]);
            return p;
        }
    }
    std::cout<<"Patch "<<bankNum<<":"<<patchNum<<" not found.\n";
    return -1;
}

int update_patch(oplStream& opl, int voice, uw_patch_file& uwpf,int patchIndex) {
    auto& patch = uwpf.bank_data[patchIndex % uwpf.bank_data.size()];
    std::cout<<"Copying "<<int(patch.bank)<<":"<<int(patch.patch)<<"("<<patch.name<<") to voice "<<voice<<"\n";
    voice_patch[voice] = &patch;
    note_off(opl, voice_midi_num[voice]);
    return patchIndex % uwpf.bank_data.size();
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cout<<"Provide a timbre library as the first argument.\n";
        return 1;
    }
    std::unique_ptr<oplStream> opl = std::make_unique<oplStream>(false); //std::make_unique<oplStream>(false);
    uw_patch_file uwpf;
    bool success = uwpf.load(argv[1]);
    if(!success) {
        std::cerr<<"Couldn't load the patch file. Aborting.\n";
        return 1;
    }

    int patchNum = 0;
    int bankNum = 0;
    if(argc == 4) {
        bankNum = std::atoi(argv[2]);
        patchNum = std::atoi(argv[3]);
    }

    int patchIndex = update_patch(*opl, 0, uwpf, bankNum, patchNum);
    if(patchIndex == -1) {
        std::cout<<"Patch "<<bankNum<<":"<<patchNum<<" couldn't be loaded. Loading first entry, "<<uwpf.bank_data[0].bank<<":"<<uwpf.bank_data[0].patch<<" instead.\n";
        patchIndex = 0;
    }

    calc_freqs();

    int failure_code = SDL_Init(SDL_INIT_VIDEO);
    if(failure_code) {
        fprintf(stderr, "Error init'ing video subsystem: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("OPL Keyboard", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 100,100,0);
    SDL_Renderer* r = SDL_CreateRenderer(window, 0, 0);

    opl->play();
    bool quit = false;
    while(!quit) {
        SDL_Event event;
        int note = 0;
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
            case SDL_KEYDOWN:
                if(!event.key.repeat) {
                    // std::cout<<"KEYDOWN: "<<event.key.keysym.sym<<'\n';
                    note = keyMap[SDL_GetKeyFromScancode(event.key.keysym.scancode)];
                    if(note > 0 && note < 100) {
                        note_on(*opl, note, 127, &(uwpf.bank_data[patchIndex]));
                        break;
                    }
                    switch(SDL_GetKeyFromScancode(event.key.keysym.scancode)) {
                        case SDLK_q:
                        case SDLK_ESCAPE:
                            quit = true;
                            break;
                        case SDLK_EQUALS:
                            patchIndex++;
                            patchIndex %= uwpf.bank_data.size();
                            bankNum = uwpf.bank_data[patchIndex].bank;
                            patchNum = uwpf.bank_data[patchIndex].patch;
                            std::cout<<"Switching to "<<int(bankNum)<<":"<<int(patchNum)<<"("<<uwpf.bank_data[patchIndex].name<<")\n";
                            break;
                        case SDLK_MINUS:
                            patchIndex--;
                            if(patchIndex < 0) patchIndex = uwpf.bank_data.size() - 1;
                            bankNum = uwpf.bank_data[patchIndex].bank;
                            patchNum = uwpf.bank_data[patchIndex].patch;
                            std::cout<<"Switching to "<<int(bankNum)<<":"<<int(patchNum)<<"("<<uwpf.bank_data[patchIndex].name<<")\n";
                            break;
                    }
                }
                break;
            case SDL_KEYUP:
                // std::cout<<"KEYUP: "<<event.key.keysym.sym<<'\n';
                note = keyMap[SDL_GetKeyFromScancode(event.key.keysym.scancode)];
                if(note > 0 && note < 100) {
                    note_off(*opl, note);
                    break;
                }
                break;
            case SDL_QUIT:
                quit = true;
                break;
            }
        }
        SDL_RenderPresent(r);
        // std::cout<<"Render present...";
        iterateTvfx(*opl);
        opl->addTime(17);
        // std::cout<<"Loop...";
    }

}
