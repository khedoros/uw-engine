#include "oplSequencer.h"

//Find the note entry for the given channel and note#
//-1 means "note not found"
int8_t oplSequencer::find_playing_note(int8_t channel, int8_t note) {
    for(int i=0;i<MIDI_NOTE_COUNT;++i) {
        if(note_channel[i] == channel && note_midi_num[i] == note)
            return i;
    }
    std::cerr<<"Couldn't find note "<<int(note)<<" playing on channel "<<int(channel)<<'\n';
    return -1;
}

int8_t oplSequencer::find_unused_note() {
    for(int i=0;i<MIDI_NOTE_COUNT;++i) {
        if(note_channel[i] == -1)
            return i;
    }
    std::cerr<<"Couldn't find a free note\n";
    return -1;
}

//Find the first voice that's currently empty
//-1 means 'all in use'
int8_t oplSequencer::find_unused_voice() {
    for(int i=0;i<OPL_VOICE_COUNT;++i) {
        if(opl_note_assignment[i] == -1)
            return i;
    }
    std::cerr<<"Couldn't find a free OPL voice\n";
    return -1;
}

void oplSequencer::calcFreqs() {
    int element = 0;
    double base_freq = 440.0;
    uint8_t base_mid_num = 69;
    for(uint16_t mid_num = 0; mid_num < 128; ++mid_num) {
        double midi_freq = base_freq * std::pow(2.0, (mid_num - base_mid_num)/12.0);
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
            freqs[mid_num] = std::make_tuple(mid_num,blk,f_num);
        }

    }
}

//Write values to the OPL2 emulator to initialize it to defaults
void oplSequencer::init_opl2() {
    opl.Reset();
    for(int i=0;i<OPL_VOICE_COUNT;++i)
        opl.SetPanning(i, 0.5,0.5);
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

    for(std::size_t reg = 0; reg < 0xf6; ++reg) {
        opl.WriteReg(reg,init_array1[reg]);
    }
}

// Write proper volume for the given voice, taking into account the patch's TL, note velocity, channel volume and expression.
void oplSequencer::writeVolume(int8_t note_num) {
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

//Copies the given patch data into the given voice slot
                //OPL voice #, index to the note information
bool oplSequencer::copy_patch(int voice, int noteIndex) {
    int channel = note_channel[noteIndex];

    if(voice == -1 || noteIndex == -1 || channel == -1) {
        if(voice == -1) std::cerr<<"Invalid voice\n";
        if(noteIndex == -1) std::cerr<<"Invalid note\n";
        if(channel == -1) std::cerr<<"Invalid channel\n";
        return false;
    }

    std::vector<uint8_t>& pat = channel_patch[channel]->ad_patchdata;
    bool am = channel_patch[channel]->ad_patchdatastruct.connection;

    if(channel == 9) { // In MT-32 mode, channel 9 (0-based) is always rhythm. The MIDI note is the instrument, and the transpose value is the actual MIDI note to play.
        for(auto& patch: uwpf.bank_data) {
            if(patch.patch == note_midi_num[noteIndex] && patch.bank == 127) {
                pat = patch.ad_patchdata;
                am = patch.ad_patchdatastruct.connection;
                rhythm_channel_note = pat[uw_patch_file::patchIndices::transpose];
            }
        }
    }

    //Write the values to the modulator:
    uint8_t mod_avekm = pat[uw_patch_file::patchIndices::mod_avekm];
    mod_avekm &= 0b10111111;
    mod_avekm |= ((channel_modulation[channel] > 64) ? 0b01000000 : 0);
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
    car_avekm |= ((channel_modulation[channel] > 64) ? 0b01000000 : 0);
    opl.WriteReg(voice_base_car[voice]+AVEKM, car_avekm);
    // opl.WriteReg(voice_base_car[voice]+KSL_TL,pat[uw_patch_file::patchIndices::car_ksl_tl]);
    opl.WriteReg(voice_base_car[voice]+AD,pat[uw_patch_file::patchIndices::car_ad]);
    opl.WriteReg(voice_base_car[voice]+SR,pat[uw_patch_file::patchIndices::car_sr]);
    opl.WriteReg(voice_base_car[voice]+WS,pat[uw_patch_file::patchIndices::car_ws]);

    //Write connection and feedback:
    opl.WriteReg(voice_base2[voice]+FB_C,pat[uw_patch_file::patchIndices::fb_c]);

    //Calculate and write volume levels:
    writeVolume(noteIndex);
    return true;
}

oplSequencer::oplSequencer(const std::string& uwpfFile) {
    bool success = uwpf.load(uwpfFile);
    init_opl2();
    calcFreqs();
    if(!success) {
        std::cerr<<"Couldn't load timbre database at "<<uwpfFile<<"\n";
    }
}

std::vector<int16_t> oplSequencer::tick() {

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

    //Start processing MIDI events from the XMI file

    while(next_event != nullptr && next_event->get_time() == curTick) {
        midi_data = next_event->get_data();
        channel = next_event->get_channel();
        curTime = next_event->get_time();
        bool retval = true;
        switch(next_event->get_command()) {
        case midi_event::NOTE_OFF: //0x80
            //Look up the voice playing the note, and the block+f_num values
            midi_num = midi_data[1];
            note_index = find_playing_note(channel, midi_num);
            if(note_index == -1) break;

            voice_num = note_voice[note_index];
            note_channel[note_index] = -1;
            if(voice_num == -1) break;

            opl_note_assignment[voice_num] = -1;
            block = std::get<1>(freqs[midi_num]);
            f_num = std::get<2>(freqs[midi_num]);
            opl.WriteReg(voice_base2[voice_num]+ON_BLK_NUM, (block<<(2)) + ((f_num&0xff00)>>(8)));
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
                block = std::get<1>(freqs[rhythm_channel_note]);
                f_num = std::get<2>(freqs[rhythm_channel_note]);
            }
            else {
                block = std::get<1>(freqs[midi_num]);
                f_num = std::get<2>(freqs[midi_num]);
            }

            opl.WriteReg(voice_base2[voice_num]+0xa0, (f_num&0xff));
            opl.WriteReg(voice_base2[voice_num]+0xb0, 0x20 + (block<<(2)) + ((f_num&0xff00)>>(8)));
            writeVolume(note_index);
            break;
        case midi_event::PROGRAM_CHANGE: //0xc0
            if(channel_patch_num[channel] == midi_data[1]) {
                std::cout<<"No change in patch number\n";
            }
            else {

                channel_patch_num[channel] = midi_data[1];
                for(auto& patch: uwpf.bank_data) {
                    if(patch.patch == channel_patch_num[channel] && patch.bank == channel_bank_num[channel]) {
                        channel_patch[channel] = &patch;
                    }
                }
            }
            std::cout<<std::dec<<"Program change: Channel "<<int(channel)<<"->"<<int(channel_bank_num[channel])<<":"<<int(channel_patch_num[channel])<<'\n';
            break;
        case midi_event::CONTROL_CHANGE: //0xb0
            std::cout<<"CC: "<<std::hex<<int(midi_data[0])<<" "<<int(midi_data[1])<<" "<<int(midi_data[2])<<'\n';
            meta = next_event->get_meta();
            if(meta == 0x01) { //Modulation change (set vibrato if over 64)
                channel_modulation[channel] = midi_data[2];
                for(int i=0;i<MIDI_NOTE_COUNT;++i) {
                    if(note_channel[i] == channel && note_voice[i] != -1) {
                        uint8_t car_avekm = channel_patch[channel]->ad_patchdata[uw_patch_file::patchIndices::car_avekm];
                        car_avekm &= 0b10111111;
                        car_avekm |= ((midi_data[2] > 64) ? 0b01000000 : 0);
                        opl.WriteReg(voice_base_car[note_voice[i]]+AVEKM, car_avekm);
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
                        opl.SetPanning(channel, 0.5 * (1.0 - float(midi_data[2])/MIDI_MAX_VAL), 0.5*(float(midi_data[2])/MIDI_MAX_VAL));
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
                std::cout<<std::dec<<"Bank change: Channel "<<int(channel)<<"->"<<int(channel_bank_num[channel])<<":"<<int(channel_patch_num[channel])<<'\n';
            }           
            else if(meta == 0x6e) std::cout<<"Channel lock (not implemented)\n";
            else if(meta == 0x6f) std::cout<<"Channel lock protect (not implemented)\n";
            else if(meta == 0x70) std::cout<<"Voice protect (not implemented)\n";
            else if(meta == 0x71) std::cout<<"Timbre protect (not implemented)\n";
            else if(meta == 0x73) std::cout<<"Indirect controller prefix (not implemented)\n";
            else if(meta == 0x74) {
                std::cout<<"For loop controller (not implemented) data: ";
                for(int i=0;i<next_event->get_data_size();++i) {
                    std::cout<<std::hex<<int(midi_data[i])<<" ";
                }
                std::cout<<'\n';
            }
            else if(meta == 0x75) {
                std::cout<<"Next/Break loop controller (not implemented) data: ";
                for(int i=0;i<next_event->get_data_size();++i) {
                    std::cout<<std::hex<<int(midi_data[i])<<" ";
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
            if(next_event->get_command() != 0xf0)
                std::cout<<"Unexpected command coming into META: "<<int(next_event->get_command())<<'\n';
            meta = next_event->get_meta();
            if(meta == 0x2f) {
                std::cout<<"End of track."<<'\n';
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
            std::cout<<"Not implemented, yet: "<<std::hex<<int(midi_data[0])<<" "<<int(midi_data[1])<<" "<<int(midi_data[2])<<'\n';
        }

        next_event = xmiFile.next_event();
        if(next_event) {
            next_event->toString();
        }
    }

    curTick++;
    const int sampleCount = 48000 / 120;
    std::vector<int16_t> samples(sampleCount * 2, 0);
    opl.Update(samples, sampleCount);
    return samples;
}

bool oplSequencer::loadXmi(const std::string& xmiFileName) {
    bool success = xmiFile.load(xmiFileName);
    if(!success) return false;
    curTick = 0;
    next_event = xmiFile.next_event();
    next_event->toString();
    for(int chan = 0; chan < MIDI_CHANNEL_COUNT; chan++) {
        for(auto& patch: uwpf.bank_data) {
            if(patch.patch == channel_patch_num[chan] && patch.bank == channel_bank_num[chan]) {
                channel_patch[chan] = &patch;
                break;
            }
        }
    }
    return true;
}

std::array<std::tuple<uint8_t,uint8_t,uint16_t>, 128> oplSequencer::freqs;