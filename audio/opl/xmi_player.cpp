//Read XMI, .AD/.OPL files, convert the MIDI commands to something that the OPL emulator can use.
//I probably want to create a class to handle sound production, though. Either as a wrapper around the OPL,
//or just as a separate class that I can feed data to.

#include<iostream>
#include "uw_patch.h"
#include "external/opl.h"
#include "xmi.h"
#include "midi_event.h"
#include<vector>
#include<list>
#include<tuple>
#include<cmath>
#include<SFML/System.hpp>
#include<SFML/Audio.hpp>
using namespace std;

//Store midi note number mappings to OPL block number and OPL F-num values
vector<tuple<uint8_t,    uint8_t,     uint16_t>> freqs;
vector<int16_t> sample_buffer;
uint32_t sample_playback_offset = 0;
uint32_t sample_insertion_offset = 0;
OPLEmul* opl;
uw_patch_file uwpf;
uint8_t timbre_bank[256];

//XMI uses a standard 120Hz clock
const uint32_t TICK_RATE = 120;
const float MIDI_MAX_VAL = 127.0;

//Data and methods having to do with current use of OPL channels, voice assignments, etc

//Which channel and note each of the 18 voices in an OPL3 chip is set to play. -1 means "none".
int8_t opl_channel_assignment[18] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int8_t opl_note_assignment[18] =    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

//Which patch and bank is each MIDI channel currently set to play
//Initialize them to 0:0
uint8_t bank_assignment[16] =  {0,0,0,0,0,0,0,0,0,127,0,0,0,0,0,0};
uint8_t patch_assignment[16] = {0,68,48,95,78,41,3,110,122,36,0,0,0,0,0,0};
uint8_t channel_volume[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

const uint16_t voice_base[18] = {    0,     1,     2,    8,    9,  0xa, 0x10, 0x11, 0x12,
                                 0x100, 0x101, 0x102,0x108,0x109,0x10a,0x110,0x111,0x112};
const uint16_t voice_base2[18] = {     0,    1,    2,    3,    4,    5,    6,    7,    8,
                                   0x100,0x101,0x102,0x103,0x104,0x105,0x106,0x107,0x108};
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
int8_t find_playing(uint8_t channel, uint8_t note) {
    for(int i=0;i<18;++i) {
        if(opl_channel_assignment[i] == channel && opl_note_assignment[i] == note)
            return i;
    }
    return -1;
}

//Find the first voice that's currently empty
//-1 means 'all in use'
int8_t find_unused() {
    for(int i=0;i<18;++i) {
        if(opl_channel_assignment[i] == -1)
            return i;
    }
    cout<<"Couldn't find a free OPL voice."<<endl;
    return -1;
}

//Mathematically calculate the best OPL settings to match Midi frequencies
//Outputs the best matches into the freqs vector of 3-tuples.
void calc_freqs() {
    double base_freq = 440.0;
    uint8_t base_mid_num = 69;
    for(uint16_t mid_num = 0; mid_num < 128; ++mid_num) {
        double midi_freq = base_freq * pow(2.0, (mid_num - base_mid_num)/12.0);
        //cout<<"MIDI Number: "<<mid_num<<" Frequency: "<<midi_freq;
        double diff = 9999999999.0;
        uint8_t blk = 0;
        uint16_t f_num = 0;
        double OPL_freq = 0.0;
        for(uint32_t block = 0; block < 8; ++block) {
            for(uint32_t f_number = 0; f_number < 1024; ++f_number) {
                double opl_freq = double(f_number * /*49716*/ OPL_SAMPLE_RATE ) / pow(2.0, 20 - double(block));
                if(abs(opl_freq - midi_freq) < diff) {
                    diff = abs(opl_freq - midi_freq);
                    f_num = f_number;
                    blk = block;
                    OPL_freq = opl_freq;
                }
            }
        }
        if(diff < 10) {
            //cout<<" OPL_Blk: "<<uint16_t(blk)<<" F-Num: "<<f_num<<" OPL Freq: "<<OPL_freq<<endl;
            freqs.push_back(make_tuple(mid_num,blk,f_num));
        }
        else {
            //cout<<" OPL: Out of Range"<<endl;
        }
    }
}

//Methods to do sets of register writes to the OPL synth emulator.

//Write values to the OPL3 emulator to initialize it to defaults
void init_opl3() {
    opl->Reset();
    for(int i=0;i<18;++i)
        opl->SetPanning(i, 0.5,0.5);
     const uint8_t init_array1[] =
     {0,0x20,   0,   0,0x60,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //00-0f Turn on waveform select, mask timer interrupts
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //10-1f 
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //20-2f Frequency mult: 1, voices 0-8
      1,   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //30-3f '                           '
     63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,   //40-4f Volume attenuation to full
     63,  63,  63,  63,  63,  63,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //50-5f '                        '
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   //60-6f Full attack and decay rates
    255, 255, 255, 255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //70-7f '                         '
     15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,   //80-8f Low sustain level, high release rate
     15,  15,  15,  15,  15,  15,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //90-9f '                                  '
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //a0-af F-Num, lower 8 bits to 0
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,0xc0,   0,   0,   //b0-bf 0 out notes, turn on tremolo and vibrato, turn off rhythm
   0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,   0,   0,   0,   0,   0,   0,   0,   //c0-cf Turn on output to both speakers
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //d0-df
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //e0-ef Set waveforms to sine
      0,   0,   0,   0,   0,   0};                                                    //f0-f5 '                   '

     const uint8_t init_array2[] =
     {0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //100-10f OPL3 mode enable
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //110-11f
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //120-12f Frequency mult: 1, voices 9-17
      1,   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //130-13f '                            '
     63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,  63,   //140-14f volume attenuation to full
     63,  63,  63,  63,  63,  63,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //150-15f '                        '
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   //160-16f Full attack and decay rates
    255, 255, 255, 255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //170-17f '                         '
     15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,   //180-18f Low sustain level, high release rate
     15,  15,  15,  15,  15,  15,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //190-19f '                                  '
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //1a0-1af F-Num, lower 8 bits to 0
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //1b0-1bf 0 out notes, turn on tremolo and vibrato
   0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,   0,   0,   0,   0,   0,   0,   0,   //1c0-1cf Turn on output to both speakers
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //1d0-1df
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //1e0-1ef Set waveforms to sine
      0,   0,   0,   0,   0, 0};                                                      //1f0-1f5 '                   '

    for(size_t reg = 0; reg < 0xf6; ++reg) {
        opl->WriteReg(reg,init_array1[reg]);
        opl->WriteReg(reg+0x100, init_array2[reg]);
    }
}

//Set the notes of the OPL3 to silent
void pause_sound() {
}

//Set the notes of the OPL3 to play again, if previously silenced
void unpause_sound() {
}

//Copies the given patch data into the given voice slot
                //OPL voice #, bank #,       instrument patch #
bool copy_patch(uint8_t voice, uint8_t bank, uint8_t patch) {
    for(auto it = uwpf.bank_data.begin(); it != uwpf.bank_data.end(); ++it) {
        if(it->bank == bank && it->patch == patch) {
            uw_patch_file::opl2_patch pat = it->ad_patchdatastruct;
            //Write the values to the modulator:
            opl->WriteReg(voice_base[voice]+0x20,(pat.mod_freq_mult&0x0f) +
                                                 ((pat.mod_env_scaling&1)<<(4)) +
                                                 ((pat.mod_sustain_sound&1)<<(5)) +
                                                 ((pat.mod_ampl_vibrato&1)<<(6)) +
                                                 ((pat.mod_freq_vibrato&1)<<(7)));
            opl->WriteReg(voice_base[voice]+0x40,(pat.mod_out_lvl&0x3f) +
                                                 ((pat.mod_key_scale&0x3)<<(6)));
            opl->WriteReg(voice_base[voice]+0x60,(pat.mod_decay&0xf) +
                                                 ((pat.mod_attack&0xf)<<(4)));
            opl->WriteReg(voice_base[voice]+0x80,(pat.mod_release&0xf) +
                                                 ((pat.mod_sustain_lvl&0xf)<<(4)));
            opl->WriteReg(voice_base[voice]+0xc0,(pat.connection&1) +
                                                 ((pat.feedback&7)<<(1)));
            opl->WriteReg(voice_base[voice]+0xe0,(pat.mod_waveform&7));

            //Write the values to the carrier:
            opl->WriteReg(voice_base[voice]+0x23,(pat.car_freq_mult&0x0f) +
                                                 ((pat.car_env_scaling&1)<<(4)) +
                                                 ((pat.car_sustain_sound&1)<<(5)) +
                                                 ((pat.car_ampl_vibrato&1)<<(6)) +
                                                 ((pat.car_freq_vibrato&1)<<(7)));
            opl->WriteReg(voice_base[voice]+0x43,((pat.car_out_lvl&0x3f) +
                                                 ((pat.car_key_scale&0x3)<<(6))));
            opl->WriteReg(voice_base[voice]+0x63,(pat.car_decay&0xf) +
                                                 ((pat.car_attack&0xf)<<(4)));
            opl->WriteReg(voice_base[voice]+0x83,(pat.car_release&0xf) +
                                                 ((pat.car_sustain_lvl&0xf)<<(4)));
            opl->WriteReg(voice_base[voice]+0xc3,(pat.connection&0x7)+
                                                 ((pat.feedback&0x7)<<(1)));
            opl->WriteReg(voice_base[voice]+0xe3,(pat.car_waveform&7));
            return true;
        }
    }
    cout<<"Bank: "<<int(bank)<<" Patch: "<<int(patch)<<endl;
    return false;        
}

/*

//Used to export the data to a file. Currently dead code.
void output_data(vector<int16_t *>& dat) {
    ofstream outfile;
    outfile.open("test.dat");
    for(auto it=dat.begin();it != dat.end(); ++it) {
        int16_t * d = *it;
        char * cd = reinterpret_cast<char *>(d);
        outfile.write(cd, 800 * sizeof(int16_t));
    }
}

//Intended to allow for streaming the music to output.
//Never worked right; it seems like it plays the same
//Sample over and over before switching to the next one,
// after about 20-30x playing the previous one.
class AudioOut: public sf::SoundStream {
    public:
    AudioOut() {
        initialize(2,OPL_SAMPLE_RATE);
        keep_going = true;
    }
    
    private:
    bool keep_going;
    void onSeek(sf::Time t) {}
    bool onGetData(sf::SoundStream::Chunk& data) {
        data.sampleCount = int(OPL_SAMPLE_RATE/120);
        while(sample_buffer.size() < 5) {}
        delete[] sample_buffer.front();
        sample_buffer.pop_front();
        uint32_t buffer_size = sample_buffer.size();
        if(buffer_size > 0) {
            data.samples = sample_buffer.front();
            //cout<<"Buffer: "<<buffer_size<<endl;
        }
        else {
            data.samples = NULL;
            cout<<"Buffer Underrun"<<endl;
        }
        return true;
    }
};
  */      
        
int main(int argc, char* argv[]) {
    //Load the patch data
    bool success = uwpf.load("uw.ad");
    if(!success) {
        cout<<"Couldn't load the patch file. Aborting."<<endl;
    }
    //Load the music itself
    xmi xmifile;
    success = xmifile.load(argv[1]);
    if(!success) {
        cout<<"Couldn't load the xmi file. Aborting."<<endl;
    }

    string output_file = "";

    if(argc == 3) {
        output_file = argv[2];
    }
    //Instantiate the OPL emulator
    opl = JavaOPLCreate(/*bool stereo =*/ true);
    init_opl3();

    calc_freqs();//Populate the frequency conversion table

    //Look up the timbre->bank map in the xmi file
    pair<uint8_t,uint8_t> * p = xmifile.next_timbre();
    //size_t ch = 1;
    while(p != NULL) {
        //Store the expected bank for this timbre
        timbre_bank[p->second] = p->first;
        cout<<"Timbre: Bank: "<<int(p->first)<<" Patch: "<<int(p->second)<<endl;
        //if(ch < 16) {
            //patch_assignment[ch] = p->second;
            //bank_assignment[ch++] = p->first;
        //}
        p = xmifile.next_timbre();
    }

    //AudioOut out;

    uint32_t cur_time = 0;
    uint8_t meta = 0;
    uint8_t channel = 0;
    int8_t voice_num = 0;
    uint16_t f_num = 0;
    uint8_t block = 0;
    uint8_t midi_num = 0;
    uint8_t * v;
    uint32_t tick_count = xmifile.tick_count();

    //pre-allocate the space for the music. Takes roughly 12MB/minute to store
    cout<<"Building a sound output buffer of "<<dec<<2* tick_count * int(OPL_SAMPLE_RATE / TICK_RATE)<<" bytes"<<endl;
    sample_buffer.resize(2 * tick_count * int(OPL_SAMPLE_RATE / TICK_RATE), 0);

    float lmaxval = 0, lminval = 0, rmaxval = 0, rminval = 0;
    //bool started = false;

    //Start processing MIDI events from the XMI file
    midi_event* e = xmifile.next_event();
    while(e != NULL) {
        uint32_t sample_count = (int(OPL_SAMPLE_RATE / TICK_RATE)) * (e->get_time() - cur_time);
        if(sample_count != 0) {
            for(int i=0;i<(e->get_time() - cur_time);++i) {
                opl->Update(&(sample_buffer[sample_insertion_offset]), int(OPL_SAMPLE_RATE/TICK_RATE));
                sample_insertion_offset+=(2 * int(OPL_SAMPLE_RATE/TICK_RATE));

                /*int16_t * samples = new int16_t[int(OPL_SAMPLE_RATE/(TICK_RATE/2))];
                opl->Update(samples, int(OPL_SAMPLE_RATE/TICK_RATE));
                sample_buffer.push_back(samples);
                if(!started && sample_buffer.size()>=TICK_RATE) { //Ensure 1 second of buffer before trying to play
                    started = true;
                    out.play();
                }
                
                for(int j = 0;j < 800; j+=2)
                    if(samples[j]<lminval) lminval = samples[j];
                    else if(samples[j]>lmaxval) lmaxval = samples[j];
                    else if(samples[j+1]<rminval) rminval = samples[j+1];
                    else if(samples[j+1]>rmaxval) rmaxval = samples[j+1];*/
            }
        }
        cur_time = e->get_time();
        //e->toString();
        bool retval = true;
        switch(e->get_command()) {
        case midi_event::NOTE_OFF: //0x80
            //Look up the voice playing the note, and the block+f_num values
            channel = e->get_channel();
            v = e->get_data();
            midi_num = v[1];
            block = get<1>(freqs[midi_num]);
            f_num = get<2>(freqs[midi_num]);
            voice_num = find_playing(channel, midi_num);
            if(voice_num == -1) {
                cout<<"Couldn't find a voice playing "<<int(midi_num)<<" for channel "<<int(channel)<<endl;
                break;
            }
            //Clear the note tracking, write the note-off register commands.
            opl_channel_assignment[voice_num] = -1;
            opl_note_assignment[voice_num] = -1;
            opl->WriteReg(voice_base2[voice_num]+0xb0, (block<<(2)) + ((f_num&0xff00)>>(8)));
            break;
        case midi_event::NOTE_ON: //0x90
            //Find an empty voice, copy the patch currently assigned to this command's channel to that voice
            voice_num = find_unused();
            if(voice_num == -1) {
                cout<<"No free voice, dropping a note."<<endl;
                break;
            }
            channel = e->get_channel();
            retval = copy_patch(voice_num, bank_assignment[channel], patch_assignment[channel]);
            if(!retval) { cout<<"Had trouble copying "<<int(bank_assignment[channel])<<":"<<int(patch_assignment[channel])<<" to channel "<<int(channel)<<". Dropping the note."<<endl;
                break;
            }
            //Look up the note info, store in note tracking, write the note-on register commands
            v = e->get_data();
            midi_num = v[1];
            block = get<1>(freqs[midi_num]);
            f_num = get<2>(freqs[midi_num]);
            opl_channel_assignment[voice_num] = channel;
            opl_note_assignment[voice_num] = midi_num; 
            opl->WriteReg(voice_base2[voice_num]+0xa0, (f_num&0xff));
            opl->WriteReg(voice_base2[voice_num]+0xb0, 0x20 + (block<<(2)) + ((f_num&0xff00)>>(8)));
            break;
        case midi_event::PROGRAM_CHANGE: //0xc0
            v = e->get_data();
            channel = e->get_channel();
            patch_assignment[channel] = v[1];
            bank_assignment[channel] = timbre_bank[v[1]];
            for(int i=0;i<18;++i) {
                if(opl_channel_assignment[i] == channel) {
                    retval = copy_patch(i, bank_assignment[channel], patch_assignment[channel]);
                    if(!retval) {
                        cout<<"Had trouble copying "<<int(bank_assignment[channel])<<":"<<int(patch_assignment[channel])<<" to channel "<<int(channel)<<endl;
                        break;
                    }
                }
            }
            cout<<dec<<"Program change: Channel "<<int(channel)<<"->"<<int(bank_assignment[channel])<<":"<<int(patch_assignment[channel])<<endl;
            break;
        case midi_event::CONTROL_CHANGE: //0xb0
            meta = e->get_meta();
            channel = e->get_channel();
            v = e->get_data();
            if(meta == 0x07) { //Volume change
                for(int i=0;i<18;++i) {
                    if(opl_channel_assignment[i] == channel)
                        opl->WriteReg(voice_base[i] + 0x40 + 3, 63 - (uint8_t((float(channel_volume[channel])/MIDI_MAX_VAL) * (v[2] / 2))));
                }
            }
            else if(meta == 0x0a) { //Panning controll
                for(int i=0;i<18;++i) {
                    if(opl_channel_assignment[i] == channel)
                        opl->SetPanning(channel, 0.5 * (1.0 - float(v[2])/MIDI_MAX_VAL), 0.5*(float(v[2])/MIDI_MAX_VAL));
                }
            }
            else if(meta == 0x72) { //Bank change
                bank_assignment[channel] = v[2];
                for(int i=0;i<18;++i) {
                    if(opl_channel_assignment[i] == channel) {
                        copy_patch(i, bank_assignment[channel], patch_assignment[channel]);
                    }
                }
            }           
            else if(meta == 0x6e) cout<<"Channel lock (not implemented)"<<endl;
            else if(meta == 0x6f) cout<<"Channel lock protect (not implemented)"<<endl;
            else if(meta == 0x70) cout<<"Voice protect (not implemented)"<<endl;
            else if(meta == 0x71) cout<<"Timbre protect (not implemented)"<<endl;
            else if(meta == 0x72) cout<<"Patch bank select (not implemented)"<<endl;
            else if(meta == 0x73) cout<<"Indirect controller prefix (not implemented)"<<endl;
            else if(meta == 0x74) cout<<"For loop controller (not implemented)"<<endl;
            else if(meta == 0x75) cout<<"Next/Break loop controller (not implemented)"<<endl;
            else if(meta == 0x76) cout<<"Clear beat/bar count (not implemented)"<<endl;
            else if(meta == 0x77) cout<<"Callback trigger (not implemented)"<<endl;
            else if(meta == 0x78) cout<<"Sequence branch index (not implemented)"<<endl;
            else 
                cout<<"Other nimplemented control change: "<<int(v[1])<<" = "<<int(v[2])<<endl;
            break;
        case midi_event::META: //0xff
            if(e->get_command() != 0xf0)
                cout<<"Unexpected command coming into META: "<<int(e->get_command())<<endl;
            meta = e->get_meta();
            if(meta == 0x2f) {
                cout<<"End of track."<<endl;
                sf::SoundBuffer sb;
                cout<<"Loading "<<2 * tick_count * int(OPL_SAMPLE_RATE / TICK_RATE)<<" samples for playback."<<endl;
                sb.loadFromSamples(&sample_buffer[0], 2* tick_count * int(OPL_SAMPLE_RATE / TICK_RATE), 2, OPL_SAMPLE_RATE);
                sf::Sound music(sb);
                if(argc == 3) {
                    bool worked = sb.saveToFile(output_file);
                    if(!worked) cout<<"Couldn't output to the file '"<<output_file<<"'. Sorry."<<endl;
                    else cout<<"Output a rendering of the music to '"<<output_file<<"'."<<endl;
                }
                else {
                    music.play();
                    sf::sleep(sf::seconds(1));
                    while(music.getStatus() == sf::SoundSource::Playing) {sf::sleep(sf::seconds(1));}
                    cout<<"Done playing."<<endl;
                }
                delete opl;
                //output_data(sample_buffer);
                //cout<<"Left: min: "<<lminval<<" max: "<<lmaxval<<"\tRight: min: "<<rminval<<" max: "<<rmaxval<<endl;
                return 0;
            }
            else {
                cout<<"Ignoring meta command"<<endl;
            }
            break;
        case midi_event::PITCH_WHEEL: //0xe0
            cout<<"Don't want to do the pitch wheel, so I didn't."<<endl;
            break;
        default:
            cout<<"Not implemented, yet: ";
            v = e->get_data();
            cout<<hex<<int(v[0])<<" "<<int(v[1])<<" "<<int(v[2])<<endl;
        }

        e = xmifile.next_event();
    }
    cout<<"I reached the end of the track without seeing the appropriate command. Weird."<<endl;
    delete opl;
    return 1;
}
