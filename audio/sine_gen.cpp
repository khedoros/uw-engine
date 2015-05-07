#include "sine_gen.h"

using namespace std;

namespace sine {
int16_t round(float x) {
    if(x >= 32767) cout<<"WTF?"<<endl;
    if(x - floor(x) >= 0.5) return floor(x)+1;
    else return floor(x);
}
};

void SineGen::set_freq(int freq) {
    cout<<"set_freq("<<freq<<")"<<endl;
    if(freq < 44) frequency = 44;
    else if(freq > 12000) frequency = 12000;
    else frequency = freq;
    skip = float(frequency * 1024) / float(rate);
    //genData();
    //sine_offset = 0;
    //skip_count = 0.0;
}

void SineGen::set_wave(int w) {
    w &= 0x7;
    cout<<"Setting wave to "<<w<<endl;
    //wa.lock();
    waveform = w;
    //wa.unlock();
    //assert(waveform >= 0);
    //assert(waveform < 8);
    //cout<<"Set wave to 2, nah nah"<<endl;
    switch(w) {
    case 0: cout<<"set_wave(sine)"<<endl; break;
    case 1: cout<<"set_wave(pos_sine)"<<endl; break;
    case 2: cout<<"set_wave(abs(sine))"<<endl; break;
    case 3: cout<<"set_wave(quarter_sine)"<<endl; break;
    case 4: cout<<"set_wave(skip_sine)"<<endl; break;
    case 5: cout<<"set_wave(abs_skip_sine)"<<endl; break;
    case 6: cout<<"set_wave(square)"<<endl; break;
    case 7: cout<<"set_wave(inv x)"<<endl; break;
    }
}

SineGen::SineGen() : playing(false), waveform(0), frequency(200), rate(44100), channels(1), sine_offset(0), skip(0.0), skip_count(0.0), samples(10000) {
    //cout<<"SineGen()"<<endl;
    skip = float(frequency * 1024) / float(rate);
    channels = 1;
    for(int x = 0; x < 1024; ++x) {
        sine_wave[0][x] = sine::round(sin(M_PI * 2.0 * (float(x) / 1024.0)) * 2048.0);
        sine_wave[4][x] = sine_wave[0][x];
        sine_wave[5][x] = sine_wave[0][x];
        sine_wave[6][x] = sine_wave[0][x];
        sine_wave[7][x] = sine_wave[0][x];
        if(sine_wave[0][x] >= 0) {
            sine_wave[1][x] = sine_wave[0][x];
            sine_wave[2][x] = sine_wave[0][x];
        }
        else {
            sine_wave[1][x] = 0;
            sine_wave[2][x] = -1.0 * sine_wave[0][x];
        }
        if(x/256 == 0)
            sine_wave[3][x] = sine_wave[0][x];
        else if(x/256 == 2)
            sine_wave[3][x] = sine_wave[2][x];
        else sine_wave[3][x] = 0;

        //cout<<sine_wave[x]<<endl;
    }
    //cout<<"Calling initialize."<<endl;
    initialize(channels, rate);
    //cout<<"Called initialize."<<endl;
}

void SineGen::onSeek(sf::Time) {
    //cout<<"Called onSeek (and I did nothing because I wasn't a seek)"<<endl;
}

void SineGen::genData() {
    //wa.lock();
    for(int i=0;i<1024;++i) {
        //cout<<"Wave is: "<<wave<<endl;
        if(playing) {
            samples[i] = sine_wave[waveform][sine_offset];
            //cout<<"Sample: "<<samples[i]<<"\tOffset in table: "<<sine_offset<<"\tHow many I've skipped: "<<skip_count<<"\tStep between samples: "<<skip<<endl;
            skip_count += skip;
            if(skip_count > 1023.5) skip_count-=1023.5;
            sine_offset=sine::round(skip_count);
        } else {
            samples[i] = 0;
        }
    }
    //wa.unlock();
}

bool SineGen::onGetData(Chunk& data) {
    genData();
    data.sampleCount = 1024;
    data.samples = &((this->samples)[0]);
    return true;
}

#ifdef STAND_ALONE    
int main() {
    cout<<"Creating the object:"<<endl;
    SineGen a,b,c,d;
    cout<<"Setting the frequency:"<<endl;
    cout<<"Calling play"<<endl;
    a.play();
    b.play();
    c.play();
    d.play();
    a.set_freq(500);
    b.set_freq(3000);
    c.set_freq(1000);
    d.set_freq(2000);
    //a.playing = true;
    //b.playing = true;
    //c.playing = true;
    //d.playing = true;
    for(int freq=500;freq<3000;freq+=2) {
        sf::sleep(sf::microseconds(10000));
        a.set_freq(freq);
        b.set_freq(3500-freq);
        d.set_freq(2500-(freq*0.3));
    }
}
#endif
