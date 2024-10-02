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
    status = unloaded;
}

// Should be called at 120Hz for the music and sound effects to behave properly
std::vector<int16_t> oplSequencer::tick() {
    if(curTick % 2 == 0) {
        // serve tvfx at 60Hz
        iterateTvfx();
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
            note_voice[note_index] = -1;
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
    if(!next_event) {
        status = ended;
    }
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
    /*
    for(int )
    */
    status = playing;
    return true;
}

oplSequencer::musicStatus oplSequencer::getStatus() {
    return status;
}

void oplSequencer::keyOffAll() {
    for(int i = 0; i < MIDI_NOTE_COUNT; i++) {
        int8_t& channel = note_channel[i];
        if(channel == -1) {
            continue;
        }

        int8_t& voice = note_voice[i];

        if(voice == -1) {
            channel = -1;
        }
        else {
            if(channel_bank_num[channel] == 1) { // Skip notes representing sound effects
                continue;
            }
            int8_t midi_num = note_midi_num[i];
            uint8_t block = std::get<1>(freqs[midi_num]);
            uint8_t f_num = std::get<2>(freqs[midi_num]);
            opl.WriteReg(voice_base2[voice]+ON_BLK_NUM, (block<<(2)) + ((f_num&0xff00)>>(8)));
            opl_note_assignment[voice] = -1;
            voice = -1;
            channel = -1;
        }
    }
}

void oplSequencer::playSfx(int number) {
    int voiceIndex = find_unused_voice();
    if(voiceIndex == -1) {
        return;
    }
    std::cout<<"playing "<<number<<" on voice "<<voiceIndex<<"\n";
    for(auto& pat: uwpf.bank_data) {
        if(pat.bank == 1 && pat.patch == number) {
            tvfx_status[voiceIndex] = KEYON;
            opl_note_assignment[voiceIndex] = 127;
            S_tvfx_patch[voiceIndex] = &(pat.tv_patchdatastruct);
        }
    }
    switch_tvfx_phase(voiceIndex);
}

// Sets up the voice for the tvfx keyon and keyoff stages
bool oplSequencer::switch_tvfx_phase(int voice) {
    if(voice == -1 || voice >= OPL_VOICE_COUNT) {
        std::cerr<<"Invalid voice\n";
        return false;
    }

    bool keyOn = (tvfx_status[voice] == KEYON);

    auto pat = S_tvfx_patch[voice];

    //designate initial values for the modulator and carrier:
    S_fbc[voice] = 0;        // FM synth, no feedback; feedback comes from time-variant part)
    S_ksltl_0[voice] = 0;    // volume=zero, ksl=0 (TL will typically come from the time-variant part)
    S_ksltl_1[voice] = 0;    // volume=zero, ksl=0
    S_avekm_0[voice] = 0x20; // SUSTAIN=1, AM=0, FM=0, Mult=0 (Mult will typically come from TV part)
    S_avekm_1[voice] = 0x20; // SUSTAIN=1, AM=0, FM=0, Mult=0
    
    uint8_t timbreType = pat->init.type;

    if(timbreType != TV_INST) {
        S_block[voice] = 0x28;
    }
    else {
        S_block[voice] = 0x20;
    }

    if(!pat->uses_opt) { // Apply default ADSR values
        std::printf("default adsr %02x%02x %02x%02x\n", 0xff, 0x0f, 0xff, 0x0f);
        opl.WriteReg(voice_base_mod[voice]+AD, 0xff);
        opl.WriteReg(voice_base_mod[voice]+SR, 0x0f);
        opl.WriteReg(voice_base_car[voice]+AD, 0xff);
        opl.WriteReg(voice_base_car[voice]+SR, 0x0f);
    }
    else {
        if(keyOn) { // ADSR values defined by the sound effect timbre
            std::printf("keyon adsr %02x%02x %02x%02x\n", pat->opt.keyon_ad_0, pat->opt.keyon_sr_0, pat->opt.keyon_ad_1, pat->opt.keyon_sr_1);
            opl.WriteReg(voice_base_mod[voice]+AD, pat->opt.keyon_ad_0);
            opl.WriteReg(voice_base_mod[voice]+SR, pat->opt.keyon_sr_0);
            opl.WriteReg(voice_base_car[voice]+AD, pat->opt.keyon_ad_1);
            opl.WriteReg(voice_base_car[voice]+SR, pat->opt.keyon_sr_1);
        }
        else {
            std::printf("keyoff adsr %02x%02x %02x%02x\n", pat->opt.release_ad_0, pat->opt.release_sr_0, pat->opt.release_ad_1, pat->opt.release_sr_1);
            opl.WriteReg(voice_base_mod[voice]+AD, pat->opt.release_ad_0);
            opl.WriteReg(voice_base_mod[voice]+SR, pat->opt.release_sr_0);
            opl.WriteReg(voice_base_car[voice]+AD, pat->opt.release_ad_1);
            opl.WriteReg(voice_base_car[voice]+SR, pat->opt.release_sr_1);
        }
    }

    // Original offsets were based on byte offsets in the whole timbre
    // The indices here only include the command lists themselves, and address 16-bit words.
    // These values are the key to the "time-variant effects"
    if(keyOn) {
        tvfx_duration[voice] = pat->init.duration + 1;

        tvfxElements[voice][freq].offset = (pat->init.keyon_f_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][level0].offset = (pat->init.keyon_v0_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][level1].offset = (pat->init.keyon_v1_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][priority].offset = (pat->init.keyon_p_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][feedback].offset = (pat->init.keyon_fb_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][mult0].offset = (pat->init.keyon_m0_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][mult1].offset = (pat->init.keyon_m1_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][waveform].offset = (pat->init.keyon_ws_offset - pat->init.keyon_f_offset) / 2;

        tvfxElements[voice][freq].value = pat->init.init_f_val;
        tvfxElements[voice][level0].value = pat->init.init_v0_val;
        tvfxElements[voice][level1].value = pat->init.init_v1_val;
        tvfxElements[voice][priority].value = pat->init.init_p_val;
        tvfxElements[voice][feedback].value = pat->init.init_fb_val;
        tvfxElements[voice][mult0].value = pat->init.init_m0_val;
        tvfxElements[voice][mult1].value = pat->init.init_m1_val;
        tvfxElements[voice][waveform].value = pat->init.init_ws_val;
    }
    else {
        tvfxElements[voice][freq].offset = (pat->init.release_f_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][level0].offset = (pat->init.release_v0_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][level1].offset = (pat->init.release_v1_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][priority].offset = (pat->init.release_p_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][feedback].offset = (pat->init.release_fb_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][mult0].offset = (pat->init.release_m0_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][mult1].offset = (pat->init.release_m1_offset - pat->init.keyon_f_offset) / 2;
        tvfxElements[voice][waveform].offset = (pat->init.release_ws_offset - pat->init.keyon_f_offset) / 2;
    }

    for(int element = 0; element < TVFX_ELEMENT_COUNT; element++) {
        tvfxElements[voice][element].counter = 1;
        tvfxElements[voice][element].increment = 0;
    }

    tvfx_update[voice] = U_ALL;
    return true;
}

// Write current TVFX state out to the OPL hardware
void oplSequencer::tvfx_update_voice(int voice) {
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
bool oplSequencer::iterateTvfxCommandList(int voice, tvfxOffset element) {
    uw_patch_file::tvfx_patch* patchDat = S_tvfx_patch[voice];
    auto& offset = tvfxElements[voice][element].offset;
    auto& value = tvfxElements[voice][element].value;
    auto& increment = tvfxElements[voice][element].increment;
    auto& counter = tvfxElements[voice][element].counter;

    bool valChanged = false;
    //std::cout<<"iterateTvfxCommandList()\n";
    for(int iter = 0; iter < 10; iter++) {
        uint16_t command = patchDat->update_data[offset+0];
        uint16_t data = patchDat->update_data[offset+1];
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
void oplSequencer::tvfx_note_free(int voice) {
    opl_note_assignment[voice] = -1;
    std::cout<<"tvfx free voice "<<voice<<"\n";
    S_kbf_shadow[voice] = (S_kbf_shadow[voice] & (~0x20));
    S_block[voice] = (S_block[voice] & (~0x20));
    opl.WriteReg(voice_base2[voice]+ON_BLK_NUM, S_kbf_shadow[voice]);
    tvfx_status[voice] = FREE;
}

// KEYOFF a tvfx effect when duration has been reached
void oplSequencer::tvfx_note_off(int voice) {
    std::cout<<"tvfx note-off, voice "<<voice<<"\n";
    tvfx_status[voice] = KEYOFF;
    switch_tvfx_phase(voice);
}

// Iterate TVFX voices for duration, value increment for each element, and iterate command lists when count hits 0.
// Also monitors volume for the end of the sound effect. Needs to be called at 60Hz.
void oplSequencer::iterateTvfx() {
    for(int voice = 0; voice < OPL_VOICE_COUNT; voice++) {
        if(tvfx_status[voice] == FREE) continue;
        if(tvfx_duration[voice]) {
            tvfx_duration[voice]--;
        }
        else if(tvfx_status[voice] == KEYON) {
            tvfx_note_off(voice);
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
                changed = iterateTvfxCommandList(voice, static_cast<tvfxOffset>(element));
            }

            if(changed) {
                tvfx_update[voice] |= (1<<element);
            }
        }
        if(tvfx_status[voice] == KEYOFF && 
           static_cast<int16_t>(tvfxElements[voice][level0].value) < 0x400 && 
           static_cast<int16_t>(tvfxElements[voice][level1].value) < 0x400) {
            tvfx_note_free(voice);
        }
        else if(tvfx_status[voice] == KEYOFF) {
            int16_t l0 = tvfxElements[voice][level0].value;
            int16_t l1 = tvfxElements[voice][level1].value;
            std::printf("%04x %04x, %d %d\n", tvfxElements[voice][level0].value, tvfxElements[voice][level1].value, l0, l1);
        }

        if(tvfx_update[voice] != 0) {
            tvfx_update_voice(voice);
        }
    }
//serve_synth called at 120Hz, services tvfx at 60Hz, updates priority at 24Hz.
//For each TVFX slot, decrement all the counters, apply value increments, mark for voice update. If volume == 0 for a slot, TVFX_increment_stage. If anything marked for voice update, tvfx_update_voice(slot).
//If KEYON and duration>0, decrement duration, otherwise, set KEYOFF and run TV_phase(slot).
//If either volume value is above 0x400, then continue, otherwise release_voice(slot), S_status[slot]=FREE, TVFX_switch_voice()
}

std::array<std::tuple<uint8_t,uint8_t,uint16_t>, 128> oplSequencer::freqs;