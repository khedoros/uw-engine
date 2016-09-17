#pragma once
#include<cstdint>
#include "../uw_patch.h"
#include "../xmi.h"
#include "../opl/opl.h"
#include<SFML/Audio.hpp>
#include<SFML/System.hpp>

static const uint16_t current_rev = 211;
static const uint16_t music_frequency = 120;
static const uint16_t sfx_frequency = 16;
static const uint16_t pcm_frequency = 256;
static const size_t sampleCount = 1024;
static int16_t buffer[2][sampleCount] = {0};
static size_t curbuf = 0;
static uw_patch_file patches;
static OPLEmul * opl = nullptr;

void kail_startup(const uw_patch_file& p);
void kail_shutdown();
void reset_synth();

class kail_seq : public sf::SoundStream {
    private:
        class kail_seq_state {
            //Tracks the current state of this sequence
        };
        
        xmi music;
        kail_seq_state state;

    public:
    bool load(xmi& x, const uint16_t seqnum);
    virtual void play();
    virtual void pause();
    virtual void stop();
    virtual bool onGetData(sf::SoundStream::Chunk& data);
    virtual void onSeek(sf::Time timeOffset);
};

