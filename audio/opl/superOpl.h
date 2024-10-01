#pragma once
#include<utility>
#include<string>
#include<array>
#include<mutex>
#include "opl.h"

OPLEmul* superOplCreate(bool stereo);

class superOpl: public OPLEmul {
public:
    superOpl();
    superOpl(bool stereo);

    virtual void Reset() override;
    virtual void WriteReg(int reg, int v) override;
    virtual void Update(float *buffer, int length) override;
    virtual void Update(int16_t *buffer, int length) override;
    virtual void SetPanning(int channel, float left, float right) override;

private:
    class inst_t;
    class op_t;
    class percChan_t;

    void updatePhases();
    void updateEnvelopes();

    uint8_t curReg;
    uint8_t statusVal;
    float leftPan;
    float rightPan;
    int audioChannels; // Stereo (2) or mono (1)

    std::mutex regMutex;
    int tremoloMultiplier; // 1dB when false (add range of 0->43?), 4.8dB when true (0->205?)
    int vibratoMultiplier; // 7 cent when false, 14 cent when true

    unsigned int envCounter; // Global counter for advancing envelope state

    int amPhase; // index into the amTable, for how deep to currently apply the AM value
    static const int amPhaseSampleLength = (OPL_SAMPLE_RATE * 64) / NATIVE_OPL_SAMPLE_RATE; // Number of samples between progressing to a new index

    int fmPhase; // vibrato has 8 phases
    static const int fmPhaseSampleLength = (OPL_SAMPLE_RATE * 1024) / NATIVE_OPL_SAMPLE_RATE;

    enum adsrPhase {
        silent,         //Note hit envelope==96dB
        attack,         //New note rising after key-on
        decay,          //Initial fade to sustain level after reaching max volume
        sustain,        //Level to hold at until key-off, or level at which to transition from decay to sustainRelease phase
        release         //key-off
    };

    static const std::array<std::string,5> adsrPhaseNames;
    static const std::array<std::string,5> rhythmNames;

    static const std::array<uint8_t, 16> multVal;
    static const std::array<int,210> amTable;
    static const std::array<int,64> fmTable;
    static const std::array<int,128> kslTable;
    static const std::array<float,4> attackTableBase;
    static std::array<int,64> attackTable;
    static const std::array<float,4> decayTableBase;
    static std::array<int,64> decayTable;

    static std::array<int,1024 * 4> logsinTable;
    static std::array<int,256> expTable;
    
    static const int NATIVE_SAMPLE_RATE = 49716;
    static const int envAccumRate = 1'000'000 / OPL_SAMPLE_RATE; // Microseconds per sample

    static const int POS_MAX = 0;
    static const int NEG_MAX = 0x8000;
    static const int POS_ZERO = 0xa00;
    static const int NEG_ZERO = 0x8a00;

    void initTables();
    int lookupSin(int val, int waveForm);
    int lookupExp(int val);
    int convertWavelength(int wavelength);
    int lfsrStepGalois();
    uint32_t galoisState; // LFSR state for the rhythem channels
    bool galoisBit;

    struct op_t {
        void updateEnvelope(unsigned int envCounter);
        void printOperator();
        unsigned phaseInc:20;    // Basically the frequency, generated from the instrument's mult, and the fNum and octave/block for the channel
        unsigned phaseCnt:20;    // Current place in the sine phase. 10.10 fixed-point number, where the whole selects the sine sample to use

        int amAtten; // current AM (tremolo) attenuation level
        int fmShift; // current FM (vibrato) phase shift level

        // Modulator feedback state.
        unsigned feedbackLevel: 3; // feedback level of first slot
        int modFB1;
        int modFB2;


        enum connectionType {
            fm,
            additive
        };
        // Connection type between modulator and carrier
        connectionType conn; // How second slot treats first slot's output

        adsrPhase envPhase;
        int envLevel; // 0 - 255. 0.375dB steps (add envLevel * 0x10)
        int envAccum; // microsecond count for envLevel increment/decrement

        //reg base 20
        bool amActive;           //tremolo (amplitude variance) @ 3.7Hz
        bool vibActive;          //frequency variance @ 6.4Hz
        bool sustain;         //1=sustained tone, 0=no sustain period
        bool keyScaleRate;     //KSR: modify ADSR rate based on frequency
        unsigned freqMult:4;   //frequency multiplier, 1 of 3 elements that define the frequency

        //reg base 40
        unsigned keyScaleLevel:2; //KSL: modify volume based on frequency
        unsigned totalLevel:6;    // level for the operator, 0.75dB steps (add totalLevel * 0x20 to output value)
        unsigned kslAtten;        // level of attenuation due to KSL
        unsigned ksrIndex:4;      // ksr adjustment to attack/decay rates

        //reg base 60
        unsigned attackRate:4;
        unsigned decayRate:4;

        //reg 80
        unsigned sustainLevel:4;
        unsigned releaseRate:4;

        //reg e0
        unsigned waveform:2;
    };

    struct chan_t {
        void printChannel();
        unsigned fNum: 10; // 2nd of 3 elements that define the frequency
        bool keyOn; //on-off state of the key
        unsigned int octave: 3; //3rd element that defines the frequency

        unsigned kslIndex: 7; // Index to the KSL table (scales volume level by note)
        unsigned ksrIndex: 4; // Index to the KSR table (scales envelope by note)

        // FM/vibrato state tracking.
        int fmRow; // fmRow is decided by the top 3 bits of the current fNum for the channel

        op_t modOp;
        op_t carOp;
    };
    
    std::array<chan_t, 32> chan;

    bool rhythmMode;          // Rhythm mode enabled

 };
