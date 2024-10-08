#include<iostream>
#include<iomanip>
#include<fstream>
#include<vector>
#include<algorithm>
#include<cassert>
#include<cstdio>
#include<byteswap.h>
#include<cstdint>
#include<unordered_map>

#include<timidity.h>
//#include<SDL/SDL.h>
#include<SFML/Audio.hpp>
#include "util.h"

using namespace std;

unsigned int decode_quantity(binifstream &filein,unsigned int start) {
    unsigned char digit;
    unsigned int retval=start;
    filein>>digit;
    if(digit>127) {
        digit&=0x7f;
        retval|=digit;
        retval*=128;
        retval=decode_quantity(filein,retval);
    }
    else {
        retval|=digit;
        return retval;
    }
    return retval;
}
//MThd: Marker for the beginning of the header
//byte size of header: The byte size of the header is always 6 in a Midi file.
//format: An X-Midi is always structured as Midi format 0 (Single track)
//#oftracks: An X-Midi will always have one track
//ticks: How many intervals per quarter note. X-Midi is 120 intervals per second, and I'm going to assume that the tempo is 60 quarter notes per second.
//MTrk: Marker for beginning of a Midi track
//byte size of MTrk (excluding that header). Fill these values in later.
//Set tempo: Meta-command 51, with a 3 byte argument representing how many microseconds per tick, at time code 0.
//                              M   T   h   d   [ byte size of header]
unsigned char midi_header[] = {'M','T','h','d', 0x00, 0x00, 0x00, 0x06,

//                             [format  ]  [#oftracks] [ticks per quarter note] M   T
                               0x00, 0x00,  0x00, 0x01, 0x01, 0x68,            'M','T',

//                              r     k   [ byte size of MTrk  ] [0Delt][Set
                               'r',  'k', 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,

//                             tempo][3byte][8333 uSec per tick]
//                               0x51,  0x03,  0x00,0x20,0x8d};
                               0x51,  0x03,  0x0f,0x42,0x40};
unsigned char midi_header_size=29;

class midi_command {
public:
    midi_command(int t,int s,unsigned char *d) {
        time=t;
        data=d;
        size=s;
    }
    ~midi_command() {
        delete[] data;
    }

    bool operator< (const midi_command& a) const {
        unsigned int t1=this->get_time();
        unsigned int t2=a.get_time();
        return (t1 < t2);
    }
    unsigned int get_time() const {
        return time;
    }
    unsigned char* get_data(unsigned int previous_time) const {
        unsigned char time_size;
        unsigned char* time_code=time_encode(time-previous_time,time_size);
        unsigned char *retval=new unsigned char[time_size+size];
        for(int i=0;i<time_size;++i) {
            retval[i]=time_code[i];
        }
        for(int i=0;i<size;++i) {
            retval[time_size+i]=data[i];
        }
        delete[] time_code;
        return retval;
    }
    unsigned int get_size(unsigned int previous_time) const {
        unsigned char time_size;
        unsigned char* time_code=time_encode(time-previous_time,time_size);
        delete[] time_code;
        return time_size+size;
    }
private:
    unsigned int time;//just for sorting
    unsigned int size;//holds the full length of the data
    unsigned char *data;       //contains the time code and the command

    unsigned char *time_encode(unsigned int in_time, unsigned char &out_size) const {
        unsigned char buff[4];
        unsigned char index=0;
        //unsigned char t_size;
        while(in_time>0) {
            assert(index<4);
            buff[index]=(in_time&0x7f);
            index++;
            in_time/=128;
        }
        unsigned char *retval=new unsigned char[index];
        out_size=index;
        for(int i=0;i<index;++i) {
            if(i+1<index) {
                retval[i]=buff[out_size-(1+i)]+0x80;
            }
            else {
                retval[i]=buff[out_size-(1+i)];
            }
        }
        if(out_size==0) {
            retval=new unsigned char[1];
            retval[0]=0;
            out_size=1;
        }
        return retval;
    }       
};

bool sortFunct (midi_command* i, midi_command* j) {return i->get_time() < j->get_time();}

vector<unsigned char> midi_buffer;
vector<midi_command*> midi_builder;

unsigned char mt2gm(unsigned char patch) {
    bool known=true;
    unsigned char retval;
    std::unordered_map<uint8_t, uint8_t> gmMapping {
        {  0,  0 }, {  1,  1 }, { 13, 19},
        { 16,  6 }, { 22,  8 }, { 23,  8 }, { 24, 62 }, { 25, 63 },
        { 32, 88 }, { 33,101 }, { 34, 52 }, { 36, 97 }, { 38, 93 }, { 43,102 },
        { 48, 48 }, { 49, 49 }, { 51, 45 }, { 52, 40 }, { 53, 40 }, { 54, 42 }, { 55, 42 }, { 56, 43 }, { 57, 46 }, { 58, 46 }, { 59, 24 },
        { 70, 35 }, { 72, 73 }, { 73, 73 }, { 74, 72 }, { 77, 74 },
        { 82, 71 }, { 83, 71 }, { 84, 68 }, { 85, 69 }, { 86, 70 }, { 88, 56 }, { 89, 56 }, { 90, 57 }, { 92, 60 }, { 93, 60 }, { 94, 58 },
        { 96, 61 }, { 97, 11 }, {103, 13 }, {104, 12 }, {105,107 }, {107, 77 }, {108, 78 }, {109, 79 }, {112, 47 },
        {117,116 }, {121,112 }, {122, 55 }, {127,121 }
    };
    auto p = gmMapping.find(patch);
    if(p == gmMapping.end()) {
        known=false;
        retval=patch;
    }
    else {
        retval = (*p).second;
    }
    if(known) {
        cout<<"Translated MT patch #"<<dec<<int(patch)<<" to GM patch #"<<int(retval)<<endl;
    }
    else {
        cout<<"Left MT patch #"<<dec<<int(patch)<<" untranslated."<<endl;
    }
    return retval;
}

bool process_events(binifstream &filein) {
    unsigned char event;
    unsigned char meta_command;
    unsigned char skip1;
    unsigned int skip4;
    unsigned int channel;
    unsigned char controller;
    unsigned char new_num;
    unsigned char note_num;
    unsigned char velocity;
    unsigned int note_length;
    unsigned int cur_time=0;
    unsigned char *buffer;
    unsigned int min,sec,ms;

    while(!filein.eof()) {
        filein>>event;
        channel=(event&0xf);
        if(channel == 8) { // Translate MT-32's rhythm channel to GM's rhythm channel
            event++;
        }
        if(event>127) {
            //cout<<hex<<"Found event>: "<<int(event)<<endl;
        }
        else {
            if(event>0) {
                cur_time+=(3*int(event));
            }
            
        }
        switch(event&0xf0) {
        case 0x00:
            break;
        case 0x10:
            break;
        case 0x20:
            break;
        case 0x30:
            break;
        case 0x40:
            break;
        case 0x50:
            break;
        case 0x60:
            break;
        case 0x70:
            break;
        case 0x80://Note stop
            filein>>note_num>>velocity;
            cout<<"Shouldn't be present in an xmi!"<<endl;
            //return false;
            break;
        case 0x90://Note start
            filein>>note_num>>velocity;
            note_length=decode_quantity(filein,0);
            buffer=new unsigned char[3];
            buffer[0]=event;
            buffer[1]=note_num;
            buffer[2]=velocity;
            midi_builder.push_back(new midi_command(cur_time,3,buffer));
            buffer=new unsigned char[3];
            buffer[0]=event-0x10;
            buffer[1]=note_num;
            buffer[2]=velocity;
            midi_builder.push_back(new midi_command(cur_time+3*note_length,3,buffer));
            break;
        case 0xb0://Change controller value
            filein>>controller>>new_num;
            buffer=new unsigned char[3];
            buffer[0]=event;
            buffer[1]=controller;
            buffer[2]=new_num;
            midi_builder.push_back(new midi_command(cur_time,3,buffer));
            break;
        case 0xc0://Change patch number
            filein>>new_num;
            new_num=mt2gm(new_num);//translate MT32 patch# to General Midi patch#
            buffer=new unsigned char[2];
            buffer[0]=event;
            buffer[1]=new_num;
            cout<<dec<<"Channel #"<<int(channel+1)<<": Patch "<<int(new_num)<<endl;
            midi_builder.push_back(new midi_command(cur_time,2,buffer));
            break;
        case 0xd0://Channel after-touch
            filein>>new_num;
            buffer=new unsigned char[2];
            buffer[0]=event;
            buffer[1]=new_num;
            midi_builder.push_back(new midi_command(cur_time,2,buffer));
            break;
        case 0xe0://Pitch wheel change
            buffer=new unsigned char[3];
            buffer[0]=event;
            filein>>new_num;
            buffer[1]=new_num;
            filein>>new_num;
            buffer[2]=new_num;
            midi_builder.push_back(new midi_command(cur_time,3,buffer));
            break;
        case 0xf0://meta commands
            if(event!=0xff) {
                cerr<<"Event is "<<hex<<int(event)<<". I was just expecting meta-events =/"<<endl;
                cerr<<"I'm going to try to ignore it..."<<endl;
                continue;
            }
            filein>>meta_command;
            if(meta_command==0x51) {//Temporalization, in microseconds per quarter note.
                filein>>skip4; //One size value, 3 bytes representing microsecond count per quarter note
            }
            else if(meta_command==0x54) {//Set time or something? HH:mm:ss:frame.1/100ths, 5 values, 1 byte per value.
                buffer=new unsigned char[8];
                buffer[0]=event;
                buffer[1]=meta_command;
                filein>>skip1;
                buffer[2]=skip1;
                assert(skip1==5);
                filein>>skip1;
                buffer[3]=skip1;
                filein>>skip1;
                buffer[4]=skip1;
                filein>>skip1;
                buffer[5]=skip1;
                filein>>skip1;
                buffer[6]=skip1;
                filein>>skip1;
                buffer[7]=skip1;
                midi_builder.push_back(new midi_command(cur_time,8,buffer));
            }
            else if(meta_command==0x58) {//Sets time signature
                buffer=new unsigned char[7];
                buffer[0]=event;
                buffer[1]=meta_command;
                filein>>skip1;
                buffer[2]=skip1;
                assert(skip1==4);
                filein>>skip1;
                buffer[3]=skip1;
                filein>>skip1;
                buffer[4]=skip1;
                filein>>skip1;
                buffer[5]=skip1;
                filein>>skip1;
                buffer[6]=skip1;
                midi_builder.push_back(new midi_command(cur_time,7,buffer));
            }   
            else if(meta_command==0x2f) {//Meta-command to mark the end of the track.
                //cout<<" End of track!"<<endl;
                buffer=new unsigned char[3];
                buffer[0]=event;
                buffer[1]=meta_command;
                buffer[2]=0x00;
                midi_builder.push_back(new midi_command(cur_time,3,buffer));
            }
            else if(meta_command==0x21) {//Midi output port command.
                buffer=new unsigned char[4];
                buffer[0]=event;
                buffer[1]=meta_command;
                filein>>skip1;
                buffer[2]=skip1;
                assert(skip1==1);
                filein>>skip1>>new_num;
                buffer[3]=new_num;
                midi_builder.push_back(new midi_command(cur_time,4,buffer));
            }
            else if(meta_command==0x21) {//Midi output port command.
                buffer=new unsigned char[4];
                buffer[0]=event;
                buffer[1]=meta_command;
                filein>>skip1;
                buffer[2]=skip1;
                assert(skip1==1);
                filein>>skip1>>new_num;
                buffer[3]=new_num;
                midi_builder.push_back(new midi_command(cur_time,4,buffer));
            }
            else {
                cerr<<" Don't know this meta command yet: "<<hex<<int(meta_command)<<endl;
                filein>>new_num; //grab length of meta command
                cerr<<"Grabbing "<<dec<<int(new_num)<<" bytes to just pass it through."<<endl;
                buffer=new unsigned char[new_num+2];
                buffer[0]=event;
                buffer[1]=meta_command;
                for(int i=2;i<new_num+2;++i) {
                    filein>>new_num;
                    buffer[i]=new_num;
                }
                //return false;
            }
            break;
        default:
            cerr<<"Event "<<hex<<int(event)<<" is not currently handled!"<<endl; 
            return false;
        }
    }
    return true;
}

bool read_tags(binifstream &filein) {//This basically acts as a check that you're actually looking at an XMI file
    unsigned char patch_no;
    unsigned char timber_no;
    unsigned short form_count;
    unsigned short timb_count;
    unsigned short branch_count;
    unsigned short branch_index_controller_value;
    unsigned int   branch_index_controller_offset;
    unsigned int form_size;
    unsigned int info_size;
    unsigned int cat_size;
    unsigned int timb_size;
    unsigned int evnt_size;
    unsigned int rbrn_size;
    unsigned int tag;

    filein>>tag;

    switch(tag) {
    case 0x4d524f46:
        //cout<<"FORM";
        filein>>form_size;
        form_size=bswap_32(form_size);
        //cout<<" (Size: "<<form_size<<")";
        filein>>tag;
        if(tag==0x52494458) {
            //cout<<" XDIR"<<endl;
            filein>>tag;
            if(tag==0x4f464e49) {
                //cout<<"    INFO";
                filein>>info_size;
                info_size=bswap_32(info_size);
                //cout<<" (Size: "<<info_size<<")"<<endl;
                filein>>form_count;
                //cout<<"        Forms in file: "<<form_count<<endl;
                return read_tags(filein);
            }
            else {
                cout<<"Unexpected tag: "<<hex<<tag<<". Aborting."<<endl;
                return false;
            }
        }
        else if(tag==0x44494d58) {
            //cout<<" XMID"<<endl;
            return read_tags(filein);
        }
        else {
            cout<<"Unexpected tag: "<<hex<<tag<<". Aborting."<<endl;
            return false;
        }
        break;
    case 0x20544143:
        //cout<<"\"CAT \"";
        filein>>cat_size;
        cat_size=bswap_32(cat_size);
        //cout<<" (Size: "<<cat_size<<")"<<endl;
        return read_tags(filein);
        break;
    case 0x44494d58:
        //cout<<"XMID"<<endl;
        return read_tags(filein);
        break;
    case 0x424d4954:
        cout<<"TIMB";
        filein>>timb_size;
        timb_size=bswap_32(timb_size);
        cout<<" (Size: "<<timb_size<<")"<<endl;
        filein>>timb_count;
        cout<<"Contains "<<timb_count<<" timber entries:"<<endl;
        for(unsigned short i=0;i<timb_count;++i) {
            filein>>patch_no>>timber_no;
            cout<<"\t"<<i<<" Patch: "<<int(patch_no)<<" Timber: "<<int(timber_no)<<endl;
        }
        return read_tags(filein);
        break;
    case 0x544e5645:
        //cout<<"EVNT";
        filein>>evnt_size;
        evnt_size=bswap_32(evnt_size);
        //cout<<" (Size: "<<evnt_size<<")"<<endl;
        return process_events(filein);
        break;
    case 0x4e524252:
        //cout<<"RBRN";
        filein>>rbrn_size;
        rbrn_size=bswap_32(rbrn_size);
        //cout<<" (Size: "<<rbrn_size<<")"<<endl;
        filein>>branch_count;
        //cout<<branch_count<<" branch index controllers: "<<endl;
        for(unsigned short i=0;i<branch_count;++i) {
            filein>>branch_index_controller_value>>branch_index_controller_offset;
            //cout<<"    "<<i<<": Value: "<<branch_index_controller_value<<" Offset: "<<branch_index_controller_offset<<endl;
        }
        return read_tags(filein);
        break;
    default:
        cout<<"Found tag: "<<hex<<tag<<endl;
        break;
    }
    return true;
}

unsigned int build_midi() {
    int midi_data_size=midi_header_size; //Contains the header and tempo setting
    unsigned int cur_time=0;
    sort(midi_builder.begin(),midi_builder.end(),sortFunct);//Sorts the data by time
    for(int i=0;i<midi_builder.size();++i) {
        midi_data_size+=(midi_builder[i]->get_size(cur_time));
        cur_time=midi_builder[i]->get_time();
    }
    midi_buffer.resize(midi_data_size);
    for(int i=0;i<midi_header_size;++i) {
        midi_buffer[i]=midi_header[i];
    }
    unsigned int wrote=0;
    cur_time=0;
    for(int i=0;i<midi_builder.size();++i) {
        unsigned int size=midi_builder[i]->get_size(cur_time);
        unsigned char *data=midi_builder[i]->get_data(cur_time);
        cur_time=midi_builder[i]->get_time();
        //cout<<i<<": ";
        for(int j=0;j<size;++j) {
            //cout<<hex<<int(data[j])<<" ";
            midi_buffer[midi_header_size+wrote+j]=data[j];
        }
        //cout<<endl;
        wrote+=size;
    }
    //cout<<"Wrote "<<wrote<<" byes of data for the main commands."<<endl;
    unsigned int data_minus_header=midi_data_size-22;
    //cout<<"Expected data minus header: "<<dec<<data_minus_header<<" (0x"<<hex<<data_minus_header<<endl;
    unsigned char *temp=reinterpret_cast<unsigned char *>(&data_minus_header);
    for(int i=0;i<4;++i) {//Output final data size count into the main MIDI header
        midi_buffer[i+18]=temp[3-i];
    }
    return midi_data_size;
}

MidSong *song;

//These two methods are used to the SDL implementation of my MIDI playback code. 
/* SDL Callback function to read the MIDI data
void callback(void *unused, Uint8 *stream, int len) {
    mid_song_read_wave(song,stream,len);
}

void play_midi(unsigned int midi_data_size) { //SDL-based play_midi method
    int status=mid_init("/usr/share/timidity/timidity.cfg");
    cout<<"Timidity initiation: "<<status<<endl;
    MidIStream *stream=mid_istream_open_mem(&(midi_buffer[0]),midi_data_size,0);
    MidSongOptions *opts=new MidSongOptions;
    opts->rate=22050;
    opts->channels=2;
    opts->format=MID_AUDIO_U8;
    opts->buffer_size=512;
    song=mid_song_load(stream,opts);
    uint32 song_length=mid_song_get_total_time(song);
    mid_song_start(song);
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    SDL_AudioSpec *spec=new SDL_AudioSpec;
    spec->freq=22050;
    spec->format=AUDIO_U8;
    spec->channels=2;
    spec->samples=512;
    spec->callback=callback;
    SDL_OpenAudio(spec,NULL);
    SDL_PauseAudio(0);
    cout<<"Currently at time: 00:00";
    while(mid_song_get_time(song) < song_length) {
        SDL_Delay(1000);
        unsigned int cur_time_ms=mid_song_get_time(song);
        unsigned int cur_time_seconds=cur_time_ms/1000;
        unsigned int cur_time_minutes=cur_time_seconds/60;
        cur_time_seconds-=(cur_time_minutes*60);
        cout<<"\b\b\b\b\b"<<dec<<setfill('0')<<setw(2)<<cur_time_minutes<<":"<<setw(2)<<cur_time_seconds;
        //cout<<setfill('0')<<setw(20)<<mid_song_get_time(song)<<" out of "<<setw(20)<<song_length<<endl;
        //cout.flush();
    }
    cout<<endl;
    SDL_CloseAudio();
}
*/

void play_midi(unsigned int midi_data_size) { //SFML-based play_midi method
    int status=mid_init("/etc/timidity.cfg");
    cout<<"Timidity initiation: "<<status<<endl;
    if(status == -1) {
        std::cerr<<"Failed to init Timidity.\n";
        return;
    }
    MidIStream *stream=mid_istream_open_mem(midi_buffer.data(), midi_buffer.size());
    MidSongOptions *opts=new MidSongOptions;
    opts->rate=22050;
    opts->channels=2;
    opts->format=MID_AUDIO_S16;
    opts->buffer_size=1024;
    song=mid_song_load(stream,opts);
    uint32 song_length=mid_song_get_total_time(song);
    int minutes = song_length/60000;
    int seconds = (song_length - (minutes*60000))/1000;
    int ms = song_length%1000;
    cout<<"Song length: "<<minutes<<"m "<<seconds<<"."<<ms<<"s"<<endl;
    mid_song_start(song);
    uint32_t song_samples = ((song_length * 22050) / 1000); //Seconds in the recording, times rate
    vector<int8_t> vbuffer(song_samples * sizeof(int16_t) * 2, 0);
    mid_song_read_wave(song,vbuffer.data(),vbuffer.size());
    sf::SoundBuffer midi_song_buf;
    
    midi_song_buf.loadFromSamples(reinterpret_cast<const int16_t *>(vbuffer.data()),vbuffer.size() / 2,2,22050);
    sf::Time length = midi_song_buf.getDuration();
    //cout<<"Midi library reported song length: "<<song_length<<" ms. SFML reported song length: "<<length.asMilliseconds()<<endl;
    //cout<<"Buffer sample count: "<<midi_song_buf.getSampleCount()<<endl;
    sf::Sound midi_song(midi_song_buf);
    midi_song.play();
    while(midi_song.getStatus() == sf::SoundSource::Playing) {}
}

int main(int argc, char *argv[]) {
    if(argc<2) {
        cout<<"You need to give a filename as an argument."<<endl;
        return 1;
    }
    string in_filename(argv[1]);
    string out_filename;
    if(argc==3) {
        out_filename=argv[2];
    }
    binifstream filein;
    filein.open(in_filename.c_str(),std::ios::binary|std::ios::in);
    filein.seekg(0,std::ios::end);
    int filesize=filein.tellg();
    if(filesize<0) {
        cout<<"Get out. You know what you did."<<endl;
        return 1;
    }
    filein.seekg(0,std::ios::beg);
    cout<<"Opened "<<in_filename<<", size: "<<filesize<<" bytes."<<endl;
    bool success=read_tags(filein);
    if(!success) {
        cerr<<"Encountered an error pre-processing the XMI. Aborting."<<endl;
        return 1;
    }
    ofstream fileout;
    unsigned int midi_size=build_midi();
    if(!out_filename.empty()) {
        fileout.open(out_filename.c_str(),std::ios::binary|std::ios::out);
        fileout.write((const char *)(&(midi_buffer[0])),midi_size);
        fileout.flush();
        cout<<"Size of the file: "<<midi_buffer.size()<<endl;
        cout<<"Writing file \""<<out_filename<<"\" with size "<<midi_size<<endl;
    }
    else {
        cout<<"Playing "<<in_filename<<"."<<endl;
        play_midi(midi_size);
    }
}
