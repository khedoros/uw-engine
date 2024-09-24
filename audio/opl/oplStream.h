#pragma once

#include<cstdint>
#include<array>
#include<iostream>
#include<cstring>
#include<SDL2/SDL.h>
#include "opl.h"
#include<fstream>

class oplStream : public OPLEmul {
    private:
        OPLEmul *opl;
#ifdef JAVA_OPL
        static const int stereoChannels = 2;
#else
        static const int stereoChannels = 1;
#endif
        static const int sampleRate = OPL_SAMPLE_RATE;
        static const int processPerSecond = 120;
        static const int sampleChunkSize = (sampleRate * stereoChannels) / processPerSecond;
        static const int sampleChunkTimeInMs = 1000 / processPerSecond;
        std::array<int16_t,sampleChunkSize> buffer;
        int remainingMilliseconds;
        int sdlDevId;
        bool stopped;
        bool handleEvents;
        // std::ofstream outfile;
    public:
#ifdef JAVA_OPL
        oplStream(bool doEvents = true) : opl(JavaOPLCreate(stereoChannels == 2)),remainingMilliseconds{0}, stopped{false}, handleEvents{doEvents} {
#else
        oplStream(bool doEvents = true) : opl(YamahaYm3812Create(stereoChannels == 2)),remainingMilliseconds{0}, stopped{false}, handleEvents{doEvents} {
#endif
            std::cout<<"Init the stream to "<<stereoChannels<<" channels, "<<sampleRate<<"Hz sample rate\n";
            initSDLAudio();
            pause();
            // outfile.open("opl_raw_out.bin");
        }
        ~oplStream() {
            std::cout<<"Deconstruct the stream\n";
        }
        void play() {
            SDL_PauseAudioDevice(sdlDevId, 0);
        }
        void pause() {
            SDL_PauseAudioDevice(sdlDevId, 1);
        }
        virtual void Reset() override {
            opl->Reset();
            remainingMilliseconds = 0;
            pause();
        }
        virtual void WriteReg(int reg, int v) override {
            opl->WriteReg(reg,v);
        }
        void addTime(int milliseconds) {
            remainingMilliseconds += milliseconds;
            while(!stopped && remainingMilliseconds >= sampleChunkTimeInMs) {
                buffer.fill(0);
                opl->Update(buffer.data(), sampleChunkSize / stereoChannels);
                remainingMilliseconds -= sampleChunkTimeInMs;
                SDL_QueueAudio(sdlDevId, buffer.data(), sampleChunkSize * sizeof(int16_t));
                // outfile.write(reinterpret_cast<char*>(buffer.data()), sampleChunkSize * sizeof(int16_t));

                while(SDL_GetQueuedAudioSize(sdlDevId) > 5 * sampleChunkSize * sizeof(int16_t)) {
                    if(handleEvents) {
                        SDL_Event event;
                        while(SDL_PollEvent(&event)) {
                            switch(event.type) {
                            case SDL_QUIT:
                                SDL_PauseAudio(true);
                                SDL_Quit();
                                stopped = true;
                            }
                        }
                    }
                    // std::cout<<"Pause for"<<sampleChunkTimeInMs<<"ms...";
                    SDL_Delay(sampleChunkTimeInMs);
                }
            }
        }
        uint32_t getQueuedAudioBytes() {
            return SDL_GetQueuedAudioSize(sdlDevId);
        }

        virtual void Update(float *buffer, int length) override {opl->Update(buffer,length);}
        virtual void Update(int16_t *buffer, int length) override {opl->Update(buffer,length);}
        virtual void SetPanning(int channel, float left, float right) override {opl->SetPanning(channel, left, right);}
        int initSDLAudio() {
            int failure_code = SDL_Init(SDL_INIT_AUDIO);
            if(failure_code) {
                fprintf(stderr, "Error init'ing audio subsystem: %s\n", SDL_GetError());
            }

            SDL_AudioSpec want;
            want.freq=sampleRate;
            want.format=AUDIO_S16;
            want.channels=stereoChannels;
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
            return sdlDevId;
        }
};
