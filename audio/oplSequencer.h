#pragma once
#include<array>
#include<cmath>

#include "xmi.h"
#include "midi_event.h"
#include "uw_patch.h"
#include "opl/superOpl.h"

class oplSequencer {
public:
    oplSequencer(const std::string& uwpfFile);
    bool loadXmi(const std::string& xmiFile);
    void playSfx(int number);
    std::vector<int16_t> tick();

    enum musicStatus {
        unloaded,
        playing,
        ended
    };

    musicStatus getStatus();
    musicStatus status;

private:
    superOpl opl;
    uw_patch_file uwpf;
    xmi xmiFile;
    uint64_t curTick;
    int8_t find_playing_note(int8_t channel, int8_t note);
    int8_t find_unused_note();
    int8_t find_unused_voice();
    void init_opl2();
    void calcFreqs();
    void writeVolume(int8_t note_num);
    bool copy_patch(int voice, int noteIndex);

    static std::array<std::tuple<uint8_t,uint8_t,uint16_t>, 128> freqs;

    //XMI uses a standard 120Hz clock
    static const uint32_t TICK_RATE = 120;
    static constexpr float MIDI_MAX_VAL = 127.0;

    static const int MIDI_NOTE_COUNT = 32;
    std::array<int8_t, MIDI_NOTE_COUNT> note_channel {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    std::array<int8_t, MIDI_NOTE_COUNT> note_midi_num;
    std::array<int8_t, MIDI_NOTE_COUNT> note_velocity;
    std::array<int8_t, MIDI_NOTE_COUNT> note_voice;
    int8_t rhythm_channel_note;

    //Data and methods having to do with current use of OPL channels, voice assignments, etc

    static const int OPL_VOICE_COUNT = 16;

    //Which note index each of the 16 voices in the super-OPL2 synth is set to play. -1 means "none".
    std::array<int8_t, OPL_VOICE_COUNT> opl_note_assignment    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
    static constexpr std::array<uint16_t, OPL_VOICE_COUNT> voice_base_mod { 0, 1, 2, 8, 9,10,16,17,18, 6,14,22,24,25,26,30 };
    static constexpr std::array<uint16_t, OPL_VOICE_COUNT> voice_base_car { 3, 4, 5,11,12,13,19,20,21, 7,15,23,27,28,29,31 };
    static constexpr std::array<uint16_t, OPL_VOICE_COUNT> voice_base2    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 };

    //Which patch and bank is each MIDI channel currently set to play
    //Initialize them to 0:0
    static const int MIDI_CHANNEL_COUNT = 16;
    std::array<int8_t, MIDI_CHANNEL_COUNT> channel_bank_num    {  0,  0,  0,  0,  0,  0,  0,  0,  0,127,  0,  0,  0,  0,  0,  0};
    std::array<int8_t, MIDI_CHANNEL_COUNT> channel_patch_num   {  0, 68, 48, 95, 78, 41,  3,110,122, 36,  0,  0,  0,  0,  0,  0};
    std::array<uw_patch_file::patchdat*, MIDI_CHANNEL_COUNT> channel_patch;
    std::array<int8_t, MIDI_CHANNEL_COUNT> channel_modulation {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
    std::array<int8_t, MIDI_CHANNEL_COUNT> channel_volume     {127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127};
    std::array<int8_t, MIDI_CHANNEL_COUNT> channel_expression {127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127};
    std::array<int16_t, MIDI_CHANNEL_COUNT> channel_pitch { 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13, 1<<13 };

    static constexpr std::array<int8_t, 16> velocity_translation { 0x52, 0x55, 0x58, 0x5b, 0x5e, 0x61, 0x64, 0x67, 0x6a, 0x6d, 0x70, 0x73, 0x76, 0x79, 0x7c, 0x7f };

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

    midi_event* next_event;

};