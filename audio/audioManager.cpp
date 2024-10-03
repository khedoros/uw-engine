#include<string>
#include<SDL2/SDL.h>

#include "audioManager.h"
#include "opl/superOpl.h"
#include "oplSequencer.h"

Uint32 AudioCallback(Uint32 interval, void* oplPtr);

audioManager::audioManager(const std::string& adFile, const std::string& dataPath, const std::string& dataPath2): seq(adFile) {
    int initRet = SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO);
    if(initRet == 0) {
        std::cerr<<"SDL sound and timer init'd successfully\n";
    }
    else {
        std::cerr<<"Error init'ing audio/timer subsystem: "<<SDL_GetError()<<"\n";
    }

    SDL_AudioSpec want;
    want.freq=48000;
    want.format=AUDIO_S16;
    want.channels=2;
    want.silence=0;
    want.samples=sampleChunkSize / stereoChannels;
    want.size=0;
    want.callback=NULL;
    want.userdata=NULL;

    SDL_AudioSpec got;
    sdlDevId= SDL_OpenAudioDevice(NULL, 0, &want, &got, 0);
    if(!sdlDevId) {
        fprintf(stderr, "Error opening audio device: %s\n", SDL_GetError());
        fprintf(stderr, "No audio device opened.\n");
    }
    else {
        printf("Freq: %d format: %d (%d) ch: %d sil: %d samp: %d size: %d\n", got.freq, got.format, want.format, got.channels, got.silence, got.samples, got.size);
    }

    SDL_PauseAudioDevice(sdlDevId, 0);
    cbs.sdlDevId = sdlDevId;
    cbs.seq = &seq;
    SDL_AddTimer(8, AudioCallback, &cbs);
}

// Stop current queue, play music
void audioManager::playMusic(const std::string& xmiFile){
    seq.loadXmi(xmiFile);
}

void audioManager::playWav(const std::string& vocFile){}
void audioManager::playSfx(int number){
    seq.playSfx(number);
}
void audioManager::enqueueSong(const std::string& xmiFile){}
void audioManager::clearQueue(){}
void audioManager::startMusic(){}
void audioManager::stopMusic(){}
void audioManager::muteMusic(){}
void audioManager::restartMusic(){}
void audioManager::setVol(int level){}

Uint32 AudioCallback(Uint32 interval, void* params) {
    audioManager::callbackStruct* cbs = reinterpret_cast<audioManager::callbackStruct*>(params);
    std::vector<int16_t> samples = cbs->seq->tick();
    SDL_QueueAudio(cbs->sdlDevId, samples.data(), samples.size() * sizeof(int16_t));
    return interval;
}