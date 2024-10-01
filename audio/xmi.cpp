#include "xmi.h"
#include<iostream>
#include<string>
#include<byteswap.h> //Handle big-endian byteswaps
#include<algorithm>

//form_size=bswap_32(form_size);
//
//Tag Names
const char xmi::FORM[] = {'F','O','R','M','\0'};
const char xmi::XDIR[] = {'X','D','I','R','\0'};
const char xmi::INFO[] = {'I','N','F','O','\0'};
const char xmi::CAT_[] = {'C','A','T',' ','\0'};
const char xmi::XMID[] = {'X','M','I','D','\0'};
const char xmi::RBRN[] = {'R','B','R','N','\0'};
const char xmi::EVNT[] = {'E','V','N','T','\0'};
const char xmi::TIMB[] = {'T','I','M','B','\0'};

std::pair<uint8_t, uint8_t> * xmi::next_timbre() {
    if(t_it != timbres.end())
        return &(*(t_it++));
    else return nullptr;
}

uint32_t xmi::tick_count() {
    return events[events.size()-1].get_time();
}

midi_event * xmi::next_event() {
    if(e_it != events.end())
        return &(*(e_it++));
    else return nullptr;
}

bool xmi::load(std::string filename) {
    curtime = 0;
    timbres.resize(0);
    events.resize(0);

    binifstream in;
    in.open(filename.c_str());
    bool opened = in.is_open();
    if(!opened) {
        std::cout<<"Failed to open the file '"<<filename<<". Aborting."<<std::endl;
        return false;
    }
    //std::cout<<"Going to load tags"<<std::endl;
    return load_tags(in);
    //std::cout<<"Loaded tags"<<std::endl;
}

bool xmi::load_tags(binifstream &in) {
    uint32_t curtag;
    uint32_t curtag_size;
    in>>curtag;
    if(curtag == tag(FORM)) {
        //std::cout<<"Found a FORM tag, size: ";
        in>>curtag_size; curtag_size = bswap_32(curtag_size);
        //std::cout<<curtag_size<<std::endl;
        load_tags(in);
    }
    else if(curtag == tag(INFO)) {
         in>>curtag_size; curtag_size = bswap_32(curtag_size);
         //std::cout<<"Found an INFO tag, size: "<<curtag_size<<std::endl;
         uint16_t form_count = 0;
         in>>form_count;
         if(form_count == 1) {
             //std::cout<<"There is "<<form_count<<" track in this file. Good."<<std::endl;
         }
         else {
             //std::cout<<"There are "<<form_count<<" tracks in this file. I hate you, so I'll only load the first."<<std::endl;
         }
         load_tags(in);
    }
    else if(curtag == tag(CAT_)) {
        in>>curtag_size; curtag_size = bswap_32(curtag_size);
        //std::cout<<"Found a 'CAT ' tag, size: "<<curtag_size<<std::endl;
        load_tags(in);
    }
    else if(curtag == tag(TIMB)) {
        in>>curtag_size; curtag_size = bswap_32(curtag_size);
        uint16_t timbre_count = 0;
        in>>timbre_count;
        //std::cout<<"Found a TIMB tag, defining "<<timbre_count<<" timbres:"<<std::endl;
        //std::cout<<"Going to load timbres"<<std::endl;
        load_timbres(in, timbre_count);
        //std::cout<<"Loaded timbres."<<std::endl;
        load_tags(in);
    }
    else if(curtag == tag(EVNT)) {
        in>>curtag_size; curtag_size = bswap_32(curtag_size);
        //std::cout<<"Found an EVNT tag, size: "<<curtag_size<<std::endl;
        //std::cout<<"Going to load events."<<std::endl;
        return load_events(in);
    }
    else if(curtag == tag(XDIR)) {
        //std::cout<<"Found an XDIR tag"<<std::endl;
        load_tags(in);
    }
    else if(curtag == tag(XMID)) {
        //std::cout<<"Found an XMID tag"<<std::endl;
        load_tags(in);
    }
    else if(curtag == tag(RBRN)) { //Ignore RBRN, because I don't care
        in>>curtag_size; curtag_size = bswap_32(curtag_size);
        in.seekg(curtag_size, std::ios::cur);
        load_tags(in);
    }
    else {
        std::cout<<"Unknown tag: "<<std::hex<<curtag<<std::endl;
        return false;
    }
    return true;
}

bool xmi::load_timbres(binifstream &in, uint16_t timbre_count) {
    assert(timbres.size() == 0);
    for(int i=0;i<timbre_count;++i) {
        uint8_t patch=0, bank=0;
        in>>patch>>bank;
        //std::cout<<std::hex<<"\t#"<<i<<": Bank: "<<int(bank)<<"\tPatch: "<<int(patch)<<std::endl;
        timbres.push_back(std::pair<uint8_t,uint8_t>(bank,patch));
    }
    t_it = timbres.begin();
    return true;
}

bool xmi::load_events(binifstream &in) {
    uint8_t event = 0;
    uint8_t meta_event = 0;
    uint8_t meta_size = 0;
    uint8_t cur_data_byte = 0;
    uint8_t controller = 0;
    uint8_t value = 0;
    uint8_t note = 0;
    uint8_t velocity = 0;
    uint32_t note_off_time = 0;
    std::vector<uint8_t> data;
    assert(events.size() == 0);
    while(!in.eof()) {
        data.clear();
        in>>event;
        data.push_back(event);
        if(event < 128) {
            curtime += event;
            //std::cout<<std::dec<<curtime - event<<" + "<<int(event)<<" = "<<curtime<<" * 1/120 seconds"<<std::endl;
        }
        else {
            switch(event & 0xf0) {
            case 0x90: // NOTE ON
                in>>note>>velocity;
                //std::cout<<std::hex<<"Note on (channel: "<<(event&0x0f)<<" note: "<<int(note)<<" velocity: "<<int(velocity)<<" at "<<std::dec<<curtime<<")"<<std::endl;
                data.push_back(note);
                data.push_back(velocity);
                events.push_back(midi_event(curtime, data));
                data[0] -= 0x10;
                note_off_time = curtime + midi_event::vlq2int(in);
                //std::cout<<"Note off (note: "<<int(note)<<" velocity: "<<int(velocity)<<" at "<<std::dec<<note_off_time<<")"<<std::endl;
                events.push_back(midi_event(note_off_time, data));
                break;
            case 0xa0: // Key Pressure / Aftertouch
                in>>note>>velocity;
                //std::cout<<std::hex<<"Note aftertouch (channel: "<<(event&0x0f)<<" note: "<<int(note)<<" velocity: "<<int(velocity)<<" at "<<std::dec<<curtime<<")"<<std::endl;
                data.push_back(note);
                data.push_back(velocity);
                events.push_back(midi_event(curtime, data));
                data[0] -= 0x20;
                note_off_time = curtime + midi_event::vlq2int(in);
                //std::cout<<"Note off (note: "<<int(note)<<" velocity: "<<int(velocity)<<" at "<<std::dec<<note_off_time<<")"<<std::endl;
                events.push_back(midi_event(note_off_time, data));
                break;
            case 0xb0: //CTRL_CHANGE
                in>>controller>>value;
                //std::cout<<"Control change ("<<std::hex<<int(controller)<<" = "<<int(value)<<")"<<std::endl;
                data.push_back(controller);
                data.push_back(value);
                events.push_back(midi_event(curtime, data));
                break;
            case 0xc0: //PRG_CHANGE
                in>>value;
                //std::cout<<"Patch change (Channel #"<<(event&0x0f)<<" = "<<int(value)<<")"<<std::endl;
                data.push_back(value);
                events.push_back(midi_event(curtime, data));
                break;
            case 0xe0: //PITCH_WHEEL
                in>>value;
                data.push_back(value);
                in>>value;
                data.push_back(value);
                events.push_back(midi_event(curtime, data));
                break;
            case 0xf0: //META_EVENT
                in>>meta_event>>meta_size;
                //std::cout<<"Meta event "<<std::hex<<int(meta_event);
                data.push_back(meta_event);
                data.push_back(meta_size);
                for(size_t i = 0; i < meta_size; ++i) {
                    in>>cur_data_byte;
                    //std::cout<<std::hex<<" "<<int(cur_data_byte);
                    data.push_back(cur_data_byte);
                }
                //std::cout<<std::endl;
                events.push_back(midi_event(curtime, data));
                if(meta_event == 0x2f) {
                    //events.push_back(midi_event(curtime,data));
                    std::cout<<"Loading reached the end of the track. Found "<<std::dec<<events.size()<<" MIDI events."<<std::endl;
                    //std::cout<<"Sorting the events"<<std::endl;
                    sort(events.begin(), events.end(), midi_event::sort_by_time);
                    //std::cout<<"Done sorting the events."<<std::endl;
                    e_it = events.begin();
                    return true;
                }

                break;
            default:
                std::cout<<"Hullo, I don't know what command "<<std::hex<<(event & 0xf0)<<" is for yet :-D"<<std::endl;
                return false;
            }
            //events.push_back(midi_event(curtime,data));
        }
    }
    return false;
}

uint32_t xmi::tag(const char inp[5]) {
    return inp[0] + 256 * inp[1] + 65536 * inp[2] + 16777216 * inp[3];
}

#ifdef STAND_ALONE_XMI
int main(int argc, char *argv[]) {
    xmi in;
    bool success = in.load(argv[1]);
    if(success) {
        std::cout<<"Successfully loaded the XMI file."<<std::endl;
    }
    else {
        std::cout<<"There was an error loading the XMI file."<<std::endl;
        return 1;
    }
    midi_event  * a = in.next_event();
    int channel_on[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int channel_max[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int total_max[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int at_once = 0;
    int at_once_max = 0;
    int on_count = 0;
    int off_count = 0;
    while(a != nullptr) {
        a->toString();
        int16_t channel=0, command=0;
        command = a->get_command();
        channel = a->get_channel();
        std::cout<<std::dec;
        if(command == 0x80) { //midi note-off
            channel_on[channel]--;
            at_once--;
            on_count++;
            //std::cout<<"Reduced channel "<<channel<<"'s playing notes to "<<channel_on[channel]<<" and total playing to "<<at_once<<std::endl;
        }
        else if(command == 0x90) {
            channel_on[channel]++;
            at_once++;
            off_count++;
            if(at_once > at_once_max) { at_once_max = at_once; }
            if(channel_on[channel] > channel_max[channel]) { channel_max[channel] = channel_on[channel]; total_max[channel] = at_once; }
            //std::cout<<"Increased channel "<<channel<<"'s playing notes to "<<channel_on[channel]<<" and total playing to "<<at_once<<std::endl;
        }
        a = in.next_event();
    }
    std::cout<<"Saw a maximum of "<<at_once_max<<" notes playing at once. Saw "<<on_count<<" note-on events, saw "<<off_count<<" note-off events."<<std::endl;
    std::cout<<"Maximums:\n";
    for(int i=0;i<16;++i)
        std::cout<<"\tChannel "<<i<<": "<<channel_max[i]<<" ( out of "<<total_max[i]<<" playing at the time)"<<std::endl;
}
#endif
