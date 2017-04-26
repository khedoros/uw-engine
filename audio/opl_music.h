//Read XMI, .AD/.OPL files, convert the MIDI commands to something that the OPL emulator can use.
//I probably want to create a class to handle sound production, though. Either as a wrapper around the OPL,
//or just as a separate class that I can feed data to.

#include "uw_patch.h"
#include "opl/opl.h"
#include "xmi.h"
#include<vector>
#include<tuple>
#include<cmath>
#include<SFML/System.hpp>
#include<SFML/Audio.hpp>
using namespace std;

class opl_music {
private:
//Store midi note number mappings to OPL block number and OPL F-num values
    vector<tuple<uint8_t,    uint8_t,     uint16_t>> freqs;
    vector<int16_t> sample_buffer;
    uint32_t sample_playback_offset = 0;
    uint32_t sample_insertion_offset = 0;
    OPLEmul* opl;
    uw_patch_file uwpf;
    xmi xmi_file;
    uint8_t timbre_bank[256];

    //XMI uses a standard 120Hz clock
    static const uint32_t TICK_RATE = 120;
    static const float MIDI_MAX_VAL = 127.0;

    //Data and methods having to do with current use of OPL channels, voice assignments, etc

    //Which channel and note each of the 18 voices in an OPL3 chip is set to play. -1 means "none".
    int8_t opl_channel_assignment[18];
    int8_t opl_note_assignment[18];

    //Which patch and bank is each MIDI channel currently set to play
    //Initialize them to 0:0
    uint8_t bank_assignment[16];
    uint8_t patch_assignment[16];
    uint8_t channel_volume[16];

    static const uint16_t voice_base[18] =  {    0,     1,     2,    8,    9,  0xa, 0x10, 0x11, 0x12,
                                             0x100, 0x101, 0x102,0x108,0x109,0x10a,0x110,0x111,0x112};
    static const uint16_t voice_base2[18] = {    0,     1,     2,    3,    4,    5,    6,    7,    8,
                                             0x100, 0x101, 0x102,0x103,0x104,0x105,0x106,0x107,0x108};
    enum OPL_addresses {
        TEST       = 0x01, //Set to 0
        TIMER1     = 0x02, //'      '
        TIMER2     = 0x03, //'      '
        TIMER_CTRL = 0x04, //'      '
        NOTE_SEL   = 0x05, 
        FLAGS_MULT = 0x20,
        VOL_KS     = 0x40,
        DEC_ATT    = 0x60,
        REL_SUS    = 0x80,
        F_NUM_L    = 0xa0,
        ON_BLK_NUM = 0xb0,
        TREM_VIB   = 0xbd, //Set to 0xc0
        FEED_CON   = 0xc0,
        WAVEFORM   = 0xe0
    };

    //Find the voice playing the given note on the given channel
    //-1 means "not found"
    int8_t find_playing(uint8_t channel, uint8_t note);

    //Find the first voice that's currently empty
    //-1 means 'all in use'
    int8_t find_unused();

    //Mathematically calculate the best OPL settings to match Midi frequencies
    //Outputs the best matches into the freqs vector of 3-tuples.
    void calc_freqs();

    //Methods to do sets of register writes to the OPL synth emulator.

    //Write values to the OPL3 emulator to initialize it to defaults
    void init_opl3(); 

    //Set the notes of the OPL3 to silent
    void pause_sound() {
    }

    //Set the notes of the OPL3 to play again, if previously silenced
    void unpause_sound() {
    }

    //Copies the given patch data into the given voice slot
                    //OPL voice #, bank #,       instrument patch #
    bool copy_patch(uint8_t voice, uint8_t bank, uint8_t patch);

    /*

    //Used to export the data to a file. Currently dead code.
    void output_data(vector<int16_t *>& dat) {
    */

    // I previously had trouble getting the SoundStream to work. Currently, it doesn't do anything very 
    // smart, like reading from anything but a linear buffer, but my short-term goal was to allow for 
    // immediate playback, rather than the old behavior of having to wait until the whole audio stream 
    // was generated to start playback.
    class AudioOut: public sf::SoundStream {
    public:
        AudioOut();
    private:
        uint32_t offset;
        void onSeek(sf::Time t);
        bool onGetData(sf::SoundStream::Chunk& data);
    };

    AudioOut ao;

public:
    bool load(std::string& ad_file,std::string& xmi_file);   
    void play();
    void pause();
};
