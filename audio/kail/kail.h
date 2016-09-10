#pragma once
#include<cstdint>
#include "../uw_patch.h"
#include "../xmi.h"
static const uint16_t current_rev = 211;
static const uint16_t music_frequency = 120;
static const uint16_t sfx_frequency = 16;
static const uint16_t pcm_frequency = 256;

void kail_startup();
void kail_shutdown();
class kail_seq {
    public:
    bool load(xmi& x, uw_patch_file& p, uint16_t seqnum);
    void start();
    void wait();
};

