#include<SFML/Audio.hpp>
#include<SFML/System.hpp>
#include<iostream>
#include<cmath>
//#include<thread>
//#include<mutex>
//#include<assert.h>

using namespace std;

int16_t round(float x);

class SineGen : public sf::SoundStream {

public:
    void set_freq(int freq);
    void set_wave(int wave);

    SineGen();

    bool playing;
private:

    void onSeek(sf::Time);

    void genData();

    bool onGetData(Chunk& data);


    int waveform;
    int frequency;
    int rate;
    int channels;
    int sine_offset;
    float skip;
    float skip_count;
    int16_t sine_wave[8][1024];
    vector<int16_t> samples;    
    //std::mutex wa;
};
