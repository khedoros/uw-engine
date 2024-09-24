//Read XMI, .AD/.OPL files, convert the MIDI commands to something that the OPL emulator can use.
//I probably want to create a class to handle sound production, though. Either as a wrapper around the OPL,
//or just as a separate class that I can feed data to.

#include<iostream>
#include<memory>
#include<unordered_map>
#include<vector>
#include<list>
#include<tuple>
#include<cmath>
#include<thread>

#include "oplStream.h"
#include "uwPatch.h"
#include "opl.h"
#include "yamahaYm3812.h"
#include "xmi.h"
#include "midiEvent.h"

//Store midi note number mappings to OPL block number and OPL F-num values
vector<tuple<uint8_t,    uint8_t,     uint16_t>> freqs;
std::unique_ptr<oplStream> opl;
uw_patch_file uwpf;

//XMI uses a standard 120Hz clock
const uint32_t TICK_RATE = 120;
const float MIDI_MAX_VAL = 127.0;

const int MIDI_NOTE_COUNT = 32;
std::array<int8_t, MIDI_NOTE_COUNT> note_channel {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
std::array<int8_t, MIDI_NOTE_COUNT> note_midi_num;
std::array<int8_t, MIDI_NOTE_COUNT> note_velocity;
std::array<int8_t, MIDI_NOTE_COUNT> note_voice;
int8_t rhythm_channel_note;

//Data and methods having to do with current use of OPL channels, voice assignments, etc

const int OPL_VOICE_COUNT = 16;

//Which note index each of the 16 voices in the super-OPL2 synth is set to play. -1 means "none".
std::array<int8_t, OPL_VOICE_COUNT> opl_note_assignment    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
const std::array<uint16_t, OPL_VOICE_COUNT> voice_base_mod { 0, 1, 2, 8, 9,10,16,17,18, 6,14,22,24,25,26,30 };
const std::array<uint16_t, OPL_VOICE_COUNT> voice_base_car { 3, 4, 5,11,12,13,19,20,21, 7,15,23,27,28,29,31 };
const std::array<uint16_t, OPL_VOICE_COUNT> voice_base2    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 };

//Which patch and bank is each MIDI channel currently set to play
//Initialize them to 0:0
const int MIDI_CHANNEL_COUNT = 16;
std::array<int8_t, MIDI_CHANNEL_COUNT> channel_bank_num    {  0,  0,  0,  0,  0,  0,  0,  0,  0,127,  0,  0,  0,  0,  0,  0};
std::array<int8_t, MIDI_CHANNEL_COUNT> channel_patch_num   {  0, 68, 48, 95, 78, 41,  3,110,122, 36,  0,  0,  0,  0,  0,  0};
std::array<uw_patch_file::patchdat*, MIDI_CHANNEL_COUNT> channel_patch;
std::array<int8_t, MIDI_CHANNEL_COUNT> channel_modulation {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
std::array<int8_t, MIDI_CHANNEL_COUNT> channel_volume     {127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127};
std::array<int8_t, MIDI_CHANNEL_COUNT> channel_expression {127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127};
std::array<int16_t, MIDI_CHANNEL_COUNT> channel_pitch { 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13 };

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

// Instrument translation tables from General MIDI instruments to the 
// numbers used in MT-32-inspired timbre banks. It'll be more useful
// when I write a parser for standard MIDI.
std::unordered_map<int,int> midiTranslate {
    { 0, 0 },    { 1, 1 },    { 2, 3 },    { 3, 7 },
    { 4, 5 },    { 5, 6 },    { 6, 17 },    { 7, 21 },
    { 8, 22 },    { 9, 101 },    { 10, 101 },    { 11, 98 },
    { 12, 104 },    { 13, 103 },    { 14, 102 },    { 15, 105 },
    { 16, 12 },    { 17, 9 },    { 18, 10 },    { 19, 13 },
    { 20, 14 },    { 21, 15 },    { 22, 87 },    { 23, 15 },
    { 24, 59 },    { 25, 60 },    { 26, 59 },    { 27, 62 },
    { 28, 61 },    { 29, 59 },    { 30, 62 },    { 31, 62 },
    { 32, 64 },    { 33, 67 },    { 34, 66 },    { 35, 71 },
    { 36, 68 },    { 37, 69 },    { 38, 66 },    { 39, 70 },
    { 40, 53 },    { 41, 52 },    { 42, 54 },    { 43, 56 },
    { 44, 53 },    { 45, 51 },    { 46, 57 },    { 47, 112 },
    { 48, 48 },    { 49, 50 },    { 50, 48 },    { 51, 50 },
    { 52, 34 },    { 53, 42 },    { 54, 33 },    { 55, 122 },
    { 56, 88 },    { 57, 90 },    { 58, 94 },    { 59, 89 },
    { 60, 92 },    { 61, 95 },    { 62, 89 },    { 63, 91 },
    { 64, 78 },    { 65, 79 },    { 66, 80 },    { 67, 81 },
    { 68, 84 },    { 69, 85 },    { 70, 86 },    { 71, 83 },
    { 72, 75 },    { 73, 73 },    { 74, 76 },    { 75, 77 },
    { 76, 110 },    { 77, 107 },    { 78, 108 },    { 79, 72 },
    { 80, 47 },    { 81, 67 },    { 82, 75 },    { 83, 51 },
    { 84, 61 },    { 85, 72 },    { 86, 52 },    { 87, 67 },
    { 88, 32 },    { 89, 33 },    { 90, 67 },    { 91, 34 },
    { 92, 32 },    { 93, 32 },    { 94, 33 },    { 95, 33 },
    { 96, 41 },    { 97, 36 },    { 98, 35 },    { 99, 37 },
    { 100, 45 },    { 101, 33 },    { 102, 43 },    { 103, 32 },
    { 104, 63 },    { 105, 105 },    { 106, 105 },    { 107, 105 },
    { 108, 51 },    { 109, 81 },    { 110, 52 },    { 111, 81 },
    { 112, 23 },    { 113, 103 },    { 114, 103 },    { 115, 113 },
    { 116, 117 },    { 117, 113 },    { 118, 116 },    { 119, 119 },
    { 120, 124 },    { 121, 120 },    { 122, 119 },    { 123, 124 },
    { 124, 123 },    { 125, 120 },    { 126, 119 },    { 127, 114 }
};

//Find the note entry for the given channel and note#
//-1 means "note not found"
int8_t find_playing_note(int8_t channel, int8_t note) {
    for(int i=0;i<MIDI_NOTE_COUNT;++i) {
        if(note_channel[i] == channel && note_midi_num[i] == note)
            return i;
    }
    std::cerr<<"Couldn't find note "<<int(note)<<" playing on channel "<<int(channel)<<'\n';
    return -1;
}

int8_t find_unused_note() {
    for(int i=0;i<MIDI_NOTE_COUNT;++i) {
        if(note_channel[i] == -1)
            return i;
    }
    std::cerr<<"Couldn't find a free note\n";
    return -1;
}

//Find the first voice that's currently empty
//-1 means 'all in use'
int8_t find_unused_voice() {
    for(int i=0;i<OPL_VOICE_COUNT;++i) {
        if(opl_note_assignment[i] == -1)
            return i;
    }
    std::cerr<<"Couldn't find a free OPL voice\n";
    return -1;
}

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
            freqs.push_back(make_tuple(mid_num,blk,f_num));
        }
        else {
            //cout<<" OPL: Out of Range\n";
        }
    }
}

//Methods to do sets of register writes to the OPL synth emulator.

//Write values to the OPL2 emulator to initialize it to defaults
void init_opl2() {
    opl->Reset();
    for(int i=0;i<OPL_VOICE_COUNT;++i)
        opl->SetPanning(i, 0.5,0.5);
     const uint8_t init_array1[] =
     {0,0x20,   0,   0,0x60,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //00-0f Turn on waveform select, mask timer interrupts
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //10-1f 
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //20-2f Frequency mult: 1, voices 0-f
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //30-3f '                           '
     63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,   //40-4f Volume attenuation to full
     63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,   //50-5f '                        '
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   //60-6f Full attack and decay rates
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   //70-7f '                         '
     15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,   //80-8f Low sustain level, high release rate
     15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,   //90-9f '                                  '
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //a0-af F-Num, lower 8 bits to 0
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0xc0,   0,   0,   //b0-bf 0 out notes, turn on tremolo and vibrato, turn off rhythm
   0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,   //c0-cf Turn on output to both speakers
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //d0-df
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //e0-ef Set waveforms to sine
      0,   0,   0,   0,   0,   0};                                                    //f0-f5 '                   '

    for(size_t reg = 0; reg < 0xf6; ++reg) {
        opl->WriteReg(reg,init_array1[reg]);
    }
}

//Set the notes of the OPL3 to silent
void pause_sound() {
    opl->pause();
}

//Set the notes of the OPL3 to play again, if previously silenced
void unpause_sound() {
    opl->play();
}

// Write proper volume for the given voice, taking into account the patch's TL, note velocity, channel volume and expression.
void writeVolume(int8_t note_num) {
    int8_t channel = note_channel[note_num];
    auto patch = channel_patch[channel];
    int8_t velocity = note_velocity[note_num];
    int8_t voice_num = note_voice[note_num];
    if(voice_num == -1) {
        std::cerr<<"Invalid voice\n";
        return;
    }

    uint16_t vol = (uint16_t(channel_volume[channel]) * channel_expression[channel])>>7;
    vol *= velocity; vol >>= 7;

    uint8_t connection = patch->ad_patchdatastruct.connection;
    if(connection) { // additive operator, so scale modulating operator too
        uint8_t mod_tl = patch->ad_patchdatastruct.mod_out_lvl;
        mod_tl = (~mod_tl) & 0x3f;
        uint8_t mod_ksl = patch->ad_patchdatastruct.mod_key_scale;
        uint16_t mod_vol = ~((vol * mod_tl) / 127);

        opl->WriteReg(voice_base_mod[voice_num]+KSL_TL,(mod_vol & 0x3f) +
                                                      ((mod_ksl & 0x3)<<(6)));
    }

    uint8_t car_tl = patch->ad_patchdatastruct.car_out_lvl;
    car_tl = (~car_tl) & 0x3f;
    uint8_t car_ksl = patch->ad_patchdatastruct.car_key_scale;
    uint16_t car_vol = ~((vol * car_tl) / 127);

    opl->WriteReg(voice_base_car[voice_num]+KSL_TL,((car_vol & 0x3f) +
                                                   ((car_ksl & 0x3)<<(6))));
}

//Copies the given patch data into the given voice slot
                //OPL voice #, index to the note information
bool copy_patch(int voice, int noteIndex) {
    int channel = note_channel[noteIndex];

    if(voice == -1 || noteIndex == -1 || channel == -1) {
        if(voice == -1) std::cerr<<"Invalid voice\n";
        if(noteIndex == -1) std::cerr<<"Invalid note\n";
        if(channel == -1) std::cerr<<"Invalid channel\n";
        return false;
    }

    std::vector<uint8_t>& pat = channel_patch[channel]->ad_patchdata;
    bool am = channel_patch[channel]->ad_patchdatastruct.connection;

    #ifndef GM_MODE
    if(channel == 9) { // In MT-32 mode, channel 9 (0-based) is always rhythm. The MIDI note is the instrument, and the transpose value is the actual MIDI note to play.
        for(auto& patch: uwpf.bank_data) {
            if(patch.patch == note_midi_num[noteIndex] && patch.bank == 127) {
                pat = patch.ad_patchdata;
                am = patch.ad_patchdatastruct.connection;
                rhythm_channel_note = pat[uw_patch_file::patchIndices::transpose];
            }
        }
    }
    #endif

    //Write the values to the modulator:
    uint8_t mod_avekm = pat[uw_patch_file::patchIndices::mod_avekm];
    mod_avekm &= 0b10111111;
    mod_avekm |= ((channel_modulation[channel] > 64) ? 0b01000000 : 0);
    opl->WriteReg(voice_base_mod[voice]+AVEKM, mod_avekm);
    if(!am) { // For FM patch connection, modulator volume comes straight from the patch data
        opl->WriteReg(voice_base_mod[voice]+KSL_TL,pat[uw_patch_file::patchIndices::mod_ksl_tl]);
    }
    opl->WriteReg(voice_base_mod[voice]+AD,pat[uw_patch_file::patchIndices::mod_ad]);
    opl->WriteReg(voice_base_mod[voice]+SR,pat[uw_patch_file::patchIndices::mod_sr]);
    opl->WriteReg(voice_base_mod[voice]+WS,pat[uw_patch_file::patchIndices::mod_ws]);

    //Write the values to the carrier:
    uint8_t car_avekm = pat[uw_patch_file::patchIndices::car_avekm];
    car_avekm &= 0b10111111;
    car_avekm |= ((channel_modulation[channel] > 64) ? 0b01000000 : 0);
    opl->WriteReg(voice_base_car[voice]+AVEKM, car_avekm);
    // opl->WriteReg(voice_base_car[voice]+KSL_TL,pat[uw_patch_file::patchIndices::car_ksl_tl]);
    opl->WriteReg(voice_base_car[voice]+AD,pat[uw_patch_file::patchIndices::car_ad]);
    opl->WriteReg(voice_base_car[voice]+SR,pat[uw_patch_file::patchIndices::car_sr]);
    opl->WriteReg(voice_base_car[voice]+WS,pat[uw_patch_file::patchIndices::car_ws]);

    //Write connection and feedback:
    opl->WriteReg(voice_base2[voice]+FB_C,pat[uw_patch_file::patchIndices::fb_c]);

    //Calculate and write volume levels:
    writeVolume(noteIndex);
    return true;
}

int main(int argc, char* argv[]) {
    xmi xmifile;

    if(argc == 3) {
        //Load the patch data
        bool success = uwpf.load(argv[1]);
        if(!success) {
            std::cerr<<"Couldn't load the patch file. Aborting.\n";
            return 1;
        }

        // Prep initial patch data
        for(int channel = 0; channel < MIDI_CHANNEL_COUNT; channel++) {
            std::cout<<"Channel: "<<channel<<": "<<int(channel_bank_num[channel])<<":"<<int(channel_patch_num[channel])<<'\n';
            for(auto& patch: uwpf.bank_data) {
                if(patch.patch == channel_patch_num[channel] && patch.bank == channel_bank_num[channel]) {
                    channel_patch[channel] = &patch;
                }
            }
        }

        //Load the music itself
        success = xmifile.load(argv[2]);
        if(!success) {
            std::cout<<"Couldn't load the xmi file. Aborting.\n";
            return 1;
        }
    }
    else {
        std::cout<<"Provide the instrument library and XMI file to load.\n";
        return 1;
    }

    string output_file = "";

    if(argc == 4) {
        output_file = argv[3];
    }
    //Instantiate the OPL emulator
    opl = std::make_unique<oplStream>();
    init_opl2();

    calc_freqs();//Populate the frequency conversion table

    // Initial timbres to load into the timbre cache
    pair<uint8_t,uint8_t> * p = xmifile.next_timbre();

    while(p != nullptr) {
        std::cout<<"Timbre: Bank: "<<int(p->first)<<" Patch: "<<int(p->second)<<'\n';
        p = xmifile.next_timbre();
    }

    uint32_t curTime = 0;
    uint8_t meta = 0;
    uint8_t channel = 0;
    int8_t voice_num = -1;
    uint16_t f_num = 0;
    uint8_t block = 0;
    uint8_t midi_num = 0;
    uint8_t midi_velocity = 0;
    uint8_t * midi_data;
    int8_t note_index = 0;

    uint8_t for_counter[4] = {0};
    int for_nesting = 0;

    opl->play();

    //Start processing MIDI events from the XMI file
    midi_event* e = xmifile.next_event();
    while(e != nullptr) {
        uint32_t tickCount = e->get_time() - curTime;
        if(tickCount != 0) {
            opl->addTime((tickCount * 1000) / TICK_RATE );
        }
        midi_data = e->get_data();
        channel = e->get_channel();
        curTime = e->get_time();
        bool retval = true;
        switch(e->get_command()) {
        case midi_event::NOTE_OFF: //0x80
            //Look up the voice playing the note, and the block+f_num values
            midi_num = midi_data[1];
            note_index = find_playing_note(channel, midi_num);
            if(note_index == -1) break;

            voice_num = note_voice[note_index];
            note_channel[note_index] = -1;
            if(voice_num == -1) break;

            opl_note_assignment[voice_num] = -1;
            block = get<1>(freqs[midi_num]);
            f_num = get<2>(freqs[midi_num]);
            opl->WriteReg(voice_base2[voice_num]+ON_BLK_NUM, (block<<(2)) + ((f_num&0xff00)>>(8)));
            break;
        case midi_event::NOTE_ON: //0x90
            note_index = find_unused_note();
            if(note_index == -1) {
                std::cout<<"Note overflow, untracked note\n";
                break;
            }

            midi_num = midi_data[1];
            if(midi_num < 12 || midi_num > 108) break; // unsupported note range
            midi_velocity = midi_data[2];
            note_channel[note_index] = channel;
            note_midi_num[note_index] = midi_num;
            note_velocity[note_index] = velocity_translation[(midi_velocity>>3)];

            voice_num = find_unused_voice();
            note_voice[note_index] = voice_num;
            if(voice_num == -1) {
                std::cout<<"No free voice, dropping a note.\n";
                break;
            }
            opl_note_assignment[voice_num] = note_index;

            retval = copy_patch(voice_num, note_index);

            if(!retval) {
                std::cout<<"Had trouble copying "<<int(channel_bank_num[channel])<<":"<<int(channel_patch_num[channel])
                         <<" to channel "<<int(channel)<<". Dropping the note.\n";
                break;
            }

            if(channel == 9) {
                block = get<1>(freqs[rhythm_channel_note]);
                f_num = get<2>(freqs[rhythm_channel_note]);
            }
            else {
                block = get<1>(freqs[midi_num]);
                f_num = get<2>(freqs[midi_num]);
            }

            opl->WriteReg(voice_base2[voice_num]+0xa0, (f_num&0xff));
            opl->WriteReg(voice_base2[voice_num]+0xb0, 0x20 + (block<<(2)) + ((f_num&0xff00)>>(8)));
            writeVolume(note_index);
            break;
        case midi_event::PROGRAM_CHANGE: //0xc0
//#ifdef GM_MODE
//            if(channel_patch_num[channel] == midiTranslate[midi_data[1]]) {
//#else
            if(channel_patch_num[channel] == midi_data[1]) {
//#endif
                std::cout<<"No change in patch number\n";
            }
            else {
//#ifdef GM_MODE
//                channel_patch_num[channel] = midiTranslate[midi_data[1]];
//#else
                channel_patch_num[channel] = midi_data[1];
//#endif
                for(auto& patch: uwpf.bank_data) {
                    if(patch.patch == channel_patch_num[channel] && patch.bank == channel_bank_num[channel]) {
                        channel_patch[channel] = &patch;
                    }
                }
            }
            std::cout<<dec<<"Program change: Channel "<<int(channel)<<"->"<<int(channel_bank_num[channel])<<":"<<int(channel_patch_num[channel])<<'\n';
            break;
        case midi_event::CONTROL_CHANGE: //0xb0
            std::cout<<"CC: "<<hex<<int(midi_data[0])<<" "<<int(midi_data[1])<<" "<<int(midi_data[2])<<'\n';
            meta = e->get_meta();
            if(meta == 0x01) { //Modulation change (set vibrato if over 64)
                channel_modulation[channel] = midi_data[2];
                for(int i=0;i<MIDI_NOTE_COUNT;++i) {
                    if(note_channel[i] == channel && note_voice[i] != -1) {
                        uint8_t car_avekm = channel_patch[channel]->ad_patchdata[uw_patch_file::patchIndices::car_avekm];
                        car_avekm &= 0b10111111;
                        car_avekm |= ((midi_data[2] > 64) ? 0b01000000 : 0);
                        opl->WriteReg(voice_base_car[note_voice[i]]+AVEKM, car_avekm);
                    }
                }
            }
            else if(meta == 0x07) { //Volume change
                channel_volume[channel] = midi_data[2];
                for(int i=0;i<MIDI_NOTE_COUNT;++i) {
                    if(note_channel[i] == channel) {
                        writeVolume(i);
                    }
                }
            }
            else if(meta == 0x0a) { //Panning control
                // Sticking with mono output for now
                /*
                for(int i=0;i<OPL_VOICE_COUNT;++i) {
                    if(opl_channel_assignment[i] == channel)
                        opl->SetPanning(channel, 0.5 * (1.0 - float(midi_data[2])/MIDI_MAX_VAL), 0.5*(float(midi_data[2])/MIDI_MAX_VAL));
                }
                */
            }
            else if(meta == 0x0b) { //Expression change (also influences volume)
                channel_expression[channel] = midi_data[2];
                for(int i=0;i<MIDI_NOTE_COUNT;++i) {
                    if(note_channel[i] == channel) {
                        writeVolume(i);
                    }
                }
            }
            else if(meta == 0x72) { //Bank change
                channel_bank_num[channel] = midi_data[2];
                for(auto& patch: uwpf.bank_data) {
                    if(patch.patch == channel_patch_num[channel] && patch.bank == channel_bank_num[channel]) {
                        channel_patch[channel] = &patch;
                    }
                }
                std::cout<<dec<<"Bank change: Channel "<<int(channel)<<"->"<<int(channel_bank_num[channel])<<":"<<int(channel_patch_num[channel])<<'\n';
            }           
            else if(meta == 0x6e) std::cout<<"Channel lock (not implemented)\n";
            else if(meta == 0x6f) std::cout<<"Channel lock protect (not implemented)\n";
            else if(meta == 0x70) std::cout<<"Voice protect (not implemented)\n";
            else if(meta == 0x71) std::cout<<"Timbre protect (not implemented)\n";
            else if(meta == 0x73) std::cout<<"Indirect controller prefix (not implemented)\n";
            else if(meta == 0x74) {
                std::cout<<"For loop controller (not implemented) data: ";
                for(int i=0;i<e->get_data_size();++i) {
                    std::cout<<hex<<int(midi_data[i])<<" ";
                }
                std::cout<<'\n';
            }
            else if(meta == 0x75) {
                std::cout<<"Next/Break loop controller (not implemented) data: ";
                for(int i=0;i<e->get_data_size();++i) {
                    std::cout<<hex<<int(midi_data[i])<<" ";
                }
                std::cout<<'\n';
            }
            else if(meta == 0x76) std::cout<<"Clear beat/bar count (not implemented)\n";
            else if(meta == 0x77) std::cout<<"Callback trigger (not implemented)\n";
            else if(meta == 0x78) std::cout<<"Sequence branch index (not implemented)\n";
            else 
                std::cout<<"Other unimplemented control change: "<<int(midi_data[1])<<" = "<<int(midi_data[2])<<'\n';
            break;
        case midi_event::META: //0xff
            if(e->get_command() != 0xf0)
                std::cout<<"Unexpected command coming into META: "<<int(e->get_command())<<'\n';
            meta = e->get_meta();
            if(meta == 0x2f) {
                std::cout<<"End of track."<<'\n';

                if(argc == 3) { // TODO: Move this functionality into oplStream?
                    //bool worked = sb.saveToFile(output_file);
                    bool worked = false;
                    std::cout<<"Output to file currently disabled.\n";
                    if(!worked) std::cout<<"Couldn't output to the file '"<<output_file<<"'. Sorry.\n";
                    else std::cout<<"Output a rendering of the music to '"<<output_file<<"'.\n";
                }
                else {
                    while(opl->getQueuedAudioBytes() > 0) {
                        SDL_Delay(100);
                    }
                    std::cout<<"Done playing.\n";
                }
                return 0;
            }
            else {
                std::cout<<"Ignoring meta command"<<std::hex<<" 0x"<<int(meta)<<'\n';
            }
            break;
        case midi_event::PITCH_WHEEL: //0xe0
        {
            int pitch = int(midi_data[2]) * 128 + midi_data[1];
            pitch -= 0x2000; // since it's a signed 14-bit number
            pitch /= 0x20; // Puts it into the range -0x100 -> 0x100
            pitch *= 12; // 12 semitones, default pitch range, value -0xc00->0xc00
            channel_pitch[channel] = pitch;
            std::cout<<"Unused pitch change: channel "<<std::dec<<channel+1<<": "<<pitch<<'\n';
        }
            break;
        default:
            std::cout<<"Not implemented, yet: "<<hex<<int(midi_data[0])<<" "<<int(midi_data[1])<<" "<<int(midi_data[2])<<'\n';
        }

        e = xmifile.next_event();
    }
    std::cout<<"I reached the end of the track without seeing the appropriate command. Weird.\n";
    return 1;
}
