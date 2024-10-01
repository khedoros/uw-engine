#include<iostream>
#include<cmath>
#include<memory>
#include "superOpl.h"
#include "../../util.h"

OPLEmul *superOplCreate(bool stereo)
{
    /* emulator create */
    return new superOpl(stereo);
}

void superOpl::Reset() {
    for(int addr=0;addr<255;addr++) {
        if(addr >= 0x40 && addr < 0x60) {
            WriteReg(addr,0x3f);
        }
        else {
            WriteReg(addr, 0);
        }
    }
    amPhase = 0;
    fmPhase = 0;
    for(auto& ch: chan) {
        ch.modOp.phaseInc = 0;
        ch.modOp.phaseCnt = 0;
        ch.modOp.envPhase = adsrPhase::silent;
        ch.modOp.envLevel = 255;
        ch.modOp.amAtten = 0;
        ch.modOp.fmShift = 0;
        ch.modOp.modFB1 = 0;
        ch.modOp.modFB2 = 0;


        ch.carOp.phaseInc = 0;
        ch.carOp.phaseCnt = 0;
        ch.carOp.envPhase = adsrPhase::silent;
        ch.carOp.envLevel = 255;
        ch.carOp.amAtten = 0;
        ch.carOp.fmShift = 0;

        ch.kslIndex = 0;
        ch.fmRow = 0;
    }
}

superOpl::superOpl() : curReg(0), statusVal(0), envCounter(0), audioChannels(2) {
    initTables();
    Reset();
}

superOpl::superOpl(bool stereo) : curReg(0), statusVal(0), envCounter(0), audioChannels(stereo?2:1) {
    initTables();
    Reset();
}

void superOpl::WriteReg(int reg, int val) {
    std::lock_guard<std::mutex> guard(regMutex);
    val &= 0xff;
    reg &= 0xff;

    if((reg >= 0x20 && reg < 0xa0) || reg >= 0xe0) { // Register writes that affect the slots
        const std::array<int8_t,32> regChanNumber {  0, 1, 2, 0, 1, 2, 9, 9,
                                                     3, 4, 5, 3, 4, 5, 10, 10,
                                                     6, 7, 8, 6, 7, 8, 11, 11,
                                                    12,13,14,12,13,14, 15, 15};
        if(regChanNumber[reg & 0x1f] == -1) return;
        enum slotType {
            MOD, CAR, INV
        };

        const std::array<slotType,32> regOpType { MOD, MOD, MOD, CAR, CAR, CAR, MOD, CAR,
                                                  MOD, MOD, MOD, CAR, CAR, CAR, MOD, CAR,
                                                  MOD, MOD, MOD, CAR, CAR, CAR, MOD, CAR,
                                                  MOD, MOD, MOD, CAR, CAR, CAR, MOD, CAR };
        uint8_t chNum = regChanNumber[reg & 0x1f];
        slotType type = regOpType[reg & 0x1f];

        if(chNum == -1) return; // invalid channel number; ignore the write

        op_t& op = (type == slotType::MOD) ? chan[chNum].modOp : chan[chNum].carOp;
        
        switch(reg & 0xe0) {
            case 0x20:
                op.amActive = static_cast<bool>(val & 0x80);
                if(!op.amActive) op.amAtten = 0;
                else op.amAtten = amTable[amPhase] * tremoloMultiplier;
                op.vibActive = static_cast<bool>(val & 0x40);
                if(!op.vibActive) op.fmShift = 0;
                else op.fmShift = fmTable[chan[chNum].fmRow + fmPhase] * vibratoMultiplier;

                op.sustain = static_cast<bool>(val & 0x20);
                op.keyScaleRate = static_cast<bool>(val & 0x10);
                {
                    int ksrNote = ((chan[chNum].fNum >> 9) + (chan[chNum].octave << 1));
                    op.ksrIndex = op.keyScaleRate ? ksrNote : ksrNote>>2;
                }
                op.freqMult = (val & 0x0f);
                op.phaseInc = convertWavelength(((chan[chNum].fNum << chan[chNum].octave) * multVal[op.freqMult]) >> 1);
                break;
            case 0x40:
                op.keyScaleLevel = (val >> 6);
                op.totalLevel = (val & 0x3f);
                op.kslAtten = ((1 << op.keyScaleLevel) >> 1) * 8 * kslTable[chan[chNum].kslIndex];
                break;
            case 0x60:
                op.attackRate = (val >> 4);
                op.decayRate = (val & 0x0f);
                break;
            case 0x80:
                op.sustainLevel = (val >> 4);
                op.releaseRate = (val & 0x0f);
                break;
            case 0xe0:
                op.waveform = (val & 0x03);
                break;
        }
    }
    else if(reg == 0xbd) { // tremolo, vibrato, rhythm
        bool deepTremolo = static_cast<bool>(val & 0x80);
        if(deepTremolo) tremoloMultiplier = 8;
        else tremoloMultiplier = 2;

        bool deepVibrato = static_cast<bool>(val & 0x40);
        if(deepVibrato) vibratoMultiplier = 2;
        else vibratoMultiplier = 1;

    }
    else if(reg >= 0xa0 && reg < 0xe0) { // Register writes that affect the channels
        uint8_t chNum = reg & 0x0f;
        if(chNum > 15) return; // invalid channel
        superOpl::chan_t& channel = chan[chNum];
        superOpl::op_t& modOp = channel.modOp;
        superOpl::op_t& carOp = channel.carOp;
        switch(reg & 0xf0) {
            case 0xa0:
                channel.fNum &= 0x300;
                channel.fNum |= val;
                modOp.phaseInc = convertWavelength(((channel.fNum << channel.octave) * multVal[modOp.freqMult]) >> 1);
                carOp.phaseInc = convertWavelength(((channel.fNum << channel.octave) * multVal[carOp.freqMult]) >> 1);
                channel.kslIndex = ((channel.fNum >>6 ) + (channel.octave << 4));
                modOp.kslAtten = ((1 << modOp.keyScaleLevel) >> 1) * 8 * kslTable[channel.kslIndex];
                carOp.kslAtten = ((1 << carOp.keyScaleLevel) >> 1) * 8 * kslTable[channel.kslIndex];
                channel.fmRow = (channel.fNum >> 4) & 0b111000;
                break;
            case 0xb0:
                channel.fNum &= 0xff;
                channel.fNum |= ((val&0x03)<<8);
                channel.octave = ((val>>2) & 0x07);
                modOp.phaseInc = convertWavelength(((channel.fNum << channel.octave) * multVal[modOp.freqMult]) >> 1);
                carOp.phaseInc = convertWavelength(((channel.fNum << channel.octave) * multVal[carOp.freqMult]) >> 1);
                channel.kslIndex = ((channel.fNum >>6 ) + (channel.octave << 4));
                {
                    int ksrNote = ((channel.fNum >> 9) + (channel.octave << 1));
                    carOp.ksrIndex = carOp.keyScaleRate ? ksrNote : ksrNote>>2;
                    modOp.ksrIndex = modOp.keyScaleRate ? ksrNote : ksrNote>>2;
                }
                modOp.kslAtten = ((1 << modOp.keyScaleLevel) >> 1) * 8 * kslTable[channel.kslIndex];
                carOp.kslAtten = ((1 << carOp.keyScaleLevel) >> 1) * 8 * kslTable[channel.kslIndex];
                channel.fmRow = (channel.fNum >> 4) & 0b111000;
                {
                    bool newKeyOn = static_cast<bool>(val & 0x20);
                    if(channel.keyOn && !newKeyOn) { // keyOff event
                        //printf("APU::YM3812 melody chan %d key-off\n", chNum);
                        modOp.envPhase = adsrPhase::release;
                        modOp.envAccum = 0;
                        carOp.envPhase = adsrPhase::release;
                        carOp.envAccum = 0;
                    }
                    else if(newKeyOn) { // keyOn event
                        if(!channel.keyOn) { // Key wasn't pressed before 
                            modOp.phaseCnt = modOp.phaseInc;
                            carOp.phaseCnt = 0;
                            modOp.modFB1 = 0;
                            modOp.modFB2 = 0;
                            modOp.envAccum = 0;
                            carOp.envAccum = 0;
                            //printf("APU::YM3812 melody chan %d attack key-off->on\n", chNum);
                            //channel.printChannel();
                        }
                        //else {
                            //printf("APU::YM3812 melody chan %d attack key-on->on\n", chNum);
                        //}

                        if(modOp.attackRate == 15) { // AR==15 jumps the volume to full in 1 step, and transitions immediately into decay.
                            // std::cout<<"Max vol, set to \"decay\", because attack==15\n";
                            modOp.envLevel = 0;
                            modOp.envPhase = adsrPhase::decay;
                        }
                        else {
                            // std::cout<<"Set to \"attack\" due to key-on\n";
                            modOp.envPhase = adsrPhase::attack;
                        }

                        if(carOp.attackRate == 15) {
                            // std::cout<<"Max vol, set to \"decay\", because attack==15\n";
                            carOp.envLevel = 0;
                            carOp.envPhase = adsrPhase::decay;
                        }
                        else {
                            // std::cout<<"Set to \"attack\" due to key-on\n";
                            carOp.envPhase = adsrPhase::attack;
                        }
                    }
                    channel.keyOn = newKeyOn;
                }
                break;
            case 0xc0:
                carOp.conn = static_cast<op_t::connectionType>(val & 0x01);
                modOp.conn = carOp.conn;
                modOp.feedbackLevel = ((val >> 1) & 0x07);
                break;
        }
    }
}

void superOpl::SetPanning(int channel, float left, float right) {}
void superOpl::Update(float* buffer, int sampleCnt) {}

void superOpl::Update(int16_t* buffer, int sampleCnt) {
    std::lock_guard<std::mutex> guard(regMutex);
    for(int i=0;i<sampleCnt*audioChannels;i+=audioChannels) {
        envCounter++;
        updateEnvelopes();
        updatePhases();

        int16_t sample = 0;

        for(int ch=0;ch<16;ch++) {
            op_t& modOp = chan[ch].modOp;
            op_t& carOp = chan[ch].carOp;

            int modOut = 0;
            int carOut = 0;

            if(modOp.envPhase != adsrPhase::silent) {
                int feedback = (modOp.feedbackLevel) ? ((modOp.modFB1 + modOp.modFB2) >> (8 - modOp.feedbackLevel)) : 0;

                int modSin = lookupSin((modOp.phaseCnt / 1024) +                              // phase
                                       modOp.fmShift +                                            // modification for vibrato
                                       (feedback),                                               // modification for feedback
                                       modOp.waveform);

                modOut = lookupExp((modSin) +                                                // sine input
                                    modOp.amAtten +                                       // AM volume attenuation (tremolo)
                                   (modOp.envLevel * 0x10) +                                // Envelope
                                    modOp.kslAtten +                                         // Key Scale Level
                                   (modOp.totalLevel * 0x20));                         // Modulator volume
                modOp.modFB1 = modOp.modFB2;
                modOp.modFB2 = modOut;
            }

            if(carOp.envPhase != adsrPhase::silent) {
                int carSin = 0;
                if(carOp.conn == op_t::connectionType::fm) {
                    carSin = lookupSin((carOp.phaseCnt  / 1024) +                              // phase
                                        carOp.fmShift +                                        // modification for vibrato
                                        (2 * modOut),                                          // fm modulation (8*PI at 0 attenuation)
                                        carOp.waveform);
                }
                else { // Additive, so don't modulate phase by op 0's output
                    carSin = lookupSin((carOp.phaseCnt / 1024) +                                 // phase
                                        carOp.fmShift,                                            // modification for vibrato
                                        carOp.waveform);
                }

                carOut = lookupExp((carSin) +                                                  // sine input
                                    carOp.amAtten +                                           // AM volume attenuation (tremolo)
                                    (carOp.envLevel * 0x10) +                                  // Envelope
                                    carOp.kslAtten +                                         // Key Scale Level
                                    (carOp.totalLevel * 0x20));                                // Channel volume
                sample += carOut;
            }
            if(carOp.conn == op_t::connectionType::additive) {
                sample += modOut;
            }
        }
        buffer[i] = sample * 3;
        if(audioChannels == 2) {
            buffer[i+1] = buffer[i];
        }
    }
}

void superOpl::initTables() {
    for (int i = 0; i < 256; ++i) {
        logsinTable[i] = round(-log2(sin((double(i) + 0.5) * M_PI_2 / 256.0)) * 256.0);
        logsinTable[511 - i] = logsinTable[i];
        logsinTable[512 + i] = 0x8000 | logsinTable[i];
        logsinTable[1023 - i] = logsinTable[512+i];
        expTable[255-i] = int(round(exp2(double(i) / 256.0) * 1024.0));
    }
    for(int i = 0; i < 1024; ++i) {
        bool sign = i & 512;
        bool mirror = i & 256;
        for(int wf=0;wf<4;wf++) {
            switch(wf) {
                case 0: break; // full sine wave; already covered.
                case 1: // half sine wave (positive half, set negative half to 0)
                    if(!sign) logsinTable[wf*1024+i] = logsinTable[i];
                    else      logsinTable[wf*1024+i] = POS_ZERO;
                    break;
                case 2: // rectified sine wave (double-bumps)
                    logsinTable[wf*1024+i] = logsinTable[i & 511];
                    break;
                case 3: // pseudo-saw (only the 1st+3rd quarters of the wave is defined, and are both positive)
                    if(!mirror) logsinTable[wf*1024+i] = logsinTable[i&255];
                    else        logsinTable[wf*1024+i] = POS_ZERO;
            }
        }

    }
}

int superOpl::lookupSin(int val, int wf) {
    val &= 1023;
    return logsinTable[1024 * wf + val];
}

int superOpl::lookupExp(int val) {
    bool sign = val & 0x8000;
    int t = expTable[(val & 255)];
    int result = (t >> ((val & 0x7F00) >> 8));
    if (sign) result = ~result;
    return result;
}

int superOpl::convertWavelength(int wavelength) {
    return (static_cast<int64_t>(wavelength) * static_cast<int64_t>(NATIVE_SAMPLE_RATE)) / static_cast<int64_t>(OPL_SAMPLE_RATE);
}

void superOpl::updatePhases() {
    if(envCounter % amPhaseSampleLength == 0) {
        amPhase++;
        amPhase %= amTable.size();
        int amAtten = amTable[amPhase] * tremoloMultiplier;
        for(auto& channel: chan) {
            if(channel.modOp.amActive) channel.modOp.amAtten = amAtten;
            if(channel.carOp.amActive) channel.carOp.amAtten = amAtten;
        }
    }

    if(envCounter % fmPhaseSampleLength == 0) {
        fmPhase++;
        fmPhase &= 7;
        for(auto& ch: chan) {
            if(ch.modOp.vibActive) ch.modOp.fmShift = fmTable[ch.fmRow + fmPhase] * vibratoMultiplier;
            if(ch.carOp.vibActive) ch.carOp.fmShift = fmTable[ch.fmRow + fmPhase] * vibratoMultiplier;
        }
    }
}

void superOpl::updateEnvelopes() {
    for(auto& ch: chan) {
        if(ch.modOp.envPhase != adsrPhase::silent) {
            // std::cout<<"Modulator: ";
            ch.modOp.phaseCnt += ch.modOp.phaseInc;
            ch.modOp.updateEnvelope(envCounter);
        }
        if(ch.carOp.envPhase != adsrPhase::silent) {
            // std::cout<<"Carrier: ";
            ch.carOp.phaseCnt += ch.carOp.phaseInc;
            ch.carOp.updateEnvelope(envCounter);
        }
    }
}

void superOpl::op_t::updateEnvelope(unsigned int counter) {
    if(envPhase == adsrPhase::attack && (envLevel <= 0)) { // Transition from attack to decay when at max volume
        envPhase = adsrPhase::decay;
        envAccum = 0;
        envLevel = 0;
    }
    else if(envPhase == adsrPhase::attack && envLevel >= 247 && attackRate == 0) {
        envPhase = adsrPhase::silent;
        envAccum = 0;
        envLevel = 255;
    }
    else if(envPhase == adsrPhase::decay && envLevel >= ((sustainLevel == 15)? 247 : (sustainLevel * 8))) { // Transition from decay to either sustain or release when hit sustain level
        if(sustain) {
            envPhase = adsrPhase::sustain;
            envLevel = ((sustainLevel == 15)? 247 : (sustainLevel * 8));
            envAccum = 0;
        }
        else {
            envPhase = adsrPhase::release;
            envAccum = 0;
        }
    }
    else if(envPhase == adsrPhase::release && envLevel >= 247) {
        envPhase = adsrPhase::silent;
        envAccum = 0;
        envLevel = 255;
    }

    int activeRate = 0;
    bool attack = false;
    switch(envPhase) {
        case adsrPhase::silent: activeRate = 0; break;
        case adsrPhase::attack: activeRate = attackRate; attack = true; break;
        case adsrPhase::decay:  activeRate = decayRate; break;
        case adsrPhase::sustain: activeRate = 0; break;
        case adsrPhase::release: activeRate = releaseRate; break;
        default: activeRate = 0;
            std::cout<<"Unhandled envPhase: "<<envPhase<<"\n";
            break;
    }

    if(activeRate != 0 && (!attack || activeRate != 15)) { // Skips the rate==0 row, which is invalid, and silent+sustain states, where envLevel doesn't change
        envAccum += envAccumRate;
        int targetValue = 0;
        int levelsToChange = 0;
        if(attack && activeRate != 15) {
            int index = std::min(63, activeRate * 4 + ksrIndex);
            targetValue = attackTable[index];
            levelsToChange = envAccum / targetValue;
            envAccum = envAccum % targetValue;
            envLevel -= levelsToChange;
        }
        else if(!attack) {
            int index = std::min(63, activeRate * 4 + ksrIndex);
            targetValue = decayTable[index];
            levelsToChange = envAccum / targetValue;
            envAccum = envAccum % targetValue;
            envLevel += levelsToChange;
        }
    }

    if(envLevel < 0) envLevel = 0; // assume wrap-around
    else if(envLevel > 255) envLevel = 255; //assume it just overflowed the 8-bit value
}

void superOpl::op_t::printOperator() {
    std::cout<<std::boolalpha<<"0x20: AM:"<<amActive<<" VIB:"<<vibActive<<" sustain:"<<sustain<<" KSR:"<<keyScaleRate<<" mult:"<<int(multVal[freqMult])<<"\n";
    std::cout<<"0x40: KSL:"<<keyScaleLevel<<" TL:"<<totalLevel<<"\n";
    std::cout<<"0x60: AR:"<<attackRate<<" DR:"<<decayRate<<"\n";
    std::cout<<"0x80: SL:"<<sustainLevel<<" RR:"<<releaseRate<<"\n";
    std::cout<<"0xC0: Feedback:"<<feedbackLevel<<" AM Connection:"<<(conn==fm ? "fm" : "am")<<"\n";
    std::cout<<"0xE0: Waveform:"<<waveform<<"\n";
}

void superOpl::chan_t::printChannel() {
    std::cout<<"Block: "<<octave<<" FNum: "<<fNum<<"\n";
    std::cout<<"Modulator:\n";
    modOp.printOperator();
    std::cout<<"Carrier:\n";
    carOp.printOperator();
    std::cout<<"\n";
}

const std::array<int,210> superOpl::amTable {
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4,
    4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 
    9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
    15,15,15,15,16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19,20,
    20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,24,24,24,24,25,25,
    25,25,26,26,26,25,25,25,25,24,24,24,24,23,23,23,23,22,22,22,22,
    21,21,21,21,20,20,20,20,19,19,19,19,18,18,18,18,17,17,17,17,16,
    16,16,16,15,15,15,15,14,14,14,14,13,13,13,13,12,12,12,12,11,11,
    11,11,10,10,10,10, 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 6, 6, 6,
    6, 5, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1
    };

const std::array<int,64> superOpl::fmTable {{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0,-1, 0,
    0, 1, 2, 1, 0,-1,-2,-1,
    0, 1, 3, 1, 0,-1,-3,-1,
    0, 2, 4, 2, 0,-2,-4,-2,
    0, 2, 5, 2, 0,-2,-5,-2,
    0, 3, 6, 3, 0,-3,-6,-3,
    0, 3, 7, 3, 0,-3,-7,-3
}};

const std::array<int,128> superOpl::kslTable {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 4, 5, 6, 7, 8,
    0, 0, 0, 0, 0, 3, 5, 7, 8,10,11,12,13,14,15,16,
    0, 0, 0, 5, 8,11,13,15,16,18,19,20,21,22,23,24,
    0, 0, 8,13,16,19,21,23,24,26,27,28,29,30,31,32,
    0, 8,16,21,24,27,29,31,32,34,35,36,37,38,39,40,
    0,16,24,29,32,35,37,39,40,42,43,44,45,46,47,48,
    0,24,32,37,40,43,45,47,48,50,51,52,53,54,55,56
    };

const std::array<float,4> superOpl::attackTableBase {
    2826.24, 2252.8, 1884.16, 1597.44
};

const std::array<float, 4> superOpl::decayTableBase {
    39280.64, 31416.32, 26173.44, 22446.08
};

// These are measured in microseconds per change of level
std::array<int,64> superOpl::attackTable {
    0, 0, 0, 0, // Skip level 0, because 0 means "no attack"
    11083, 8835, 7389, 6264,
    5542, 4417, 3694, 3132,
    2771, 2209, 1847, 1566,
    1385, 1104, 924, 783,
    693, 552, 462, 392,
    346, 276, 231, 196,
    173, 138, 115, 98,
    87, 69, 58, 49,
    43, 35, 29, 24,
    22, 17, 14, 12,
    11, 9, 7, 6,
    5, 4, 4, 3,
    3, 2, 2, 2,
    1, 1, 1, 1,
    1, 1, 1, 1
};

// These are measured in microseconds per change of level
std::array<int,64> superOpl::decayTable {
    0, 0, 0, 0, // Skip level 0, because 0 means "no decay"
    154042, 123201, 102641, 88024,
    77021, 61601, 51320, 44012,
    38510, 30800, 25660, 22006,
    19255, 15400, 12830, 11003,
    9628, 7700, 6415, 5501,
    4814, 3850, 3208, 2751,
    2407, 1925, 1604, 1375,
    1203, 963, 802, 688,
    602, 481, 401, 344,
    301, 241, 200, 172,
    150, 120, 100, 86,
    75, 60, 50, 43,
    38, 30, 25, 21,
    19, 15, 13, 11,
    9, 9, 9, 9
};

std::array<int,1024*4> superOpl::logsinTable;
std::array<int,256> superOpl::expTable;
const std::array<uint8_t,16> superOpl::multVal {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30};
const int superOpl::NATIVE_SAMPLE_RATE;
const std::array<std::string,5> superOpl::adsrPhaseNames {
    "silent",
    "attack",
    "decay",
    "sustain",
    "release"
};
