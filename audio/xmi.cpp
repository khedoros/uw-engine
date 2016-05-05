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

pair<uint8_t, uint8_t> * xmi::next_timbre() {
    if(t_it != timbres.end())
        return &(*(t_it++));
    else return NULL;
}

uint32_t xmi::tick_count() {
    return events[events.size()-1].get_time();
}

midi_event * xmi::next_event() {
    if(e_it != events.end())
        return &(*(e_it++));
    else return NULL;
}

bool xmi::load(string filename) {
    binifstream in;
    in.open(filename.c_str());
    bool opened = in.is_open();
    if(!opened) {
        cout<<"Failed to open the file '"<<filename<<". Aborting."<<endl;
        return false;
    }
    //cout<<"Going to load tags"<<endl;
    return load_tags(in);
    //cout<<"Loaded tags"<<endl;
}

bool xmi::load_tags(binifstream &in) {
    uint32_t curtag;
    uint32_t curtag_size;
    in>>curtag;
    if(curtag == tag(FORM)) {
        //cout<<"Found a FORM tag, size: ";
        in>>curtag_size; curtag_size = bswap_32(curtag_size);
        //cout<<curtag_size<<endl;
        load_tags(in);
    }
    else if(curtag == tag(INFO)) {
         in>>curtag_size; curtag_size = bswap_32(curtag_size);
         //cout<<"Found an INFO tag, size: "<<curtag_size<<endl;
         uint16_t form_count = 0;
         in>>form_count;
         if(form_count == 1) {
             //cout<<"There is "<<form_count<<" track in this file. Good."<<endl;
         }
         else {
             //cout<<"There are "<<form_count<<" tracks in this file. I hate you, so I'll only load the first."<<endl;
         }
         load_tags(in);
    }
    else if(curtag == tag(CAT_)) {
        in>>curtag_size; curtag_size = bswap_32(curtag_size);
        //cout<<"Found a 'CAT ' tag, size: "<<curtag_size<<endl;
        load_tags(in);
    }
    else if(curtag == tag(TIMB)) {
        in>>curtag_size; curtag_size = bswap_32(curtag_size);
        uint16_t timbre_count = 0;
        in>>timbre_count;
        //cout<<"Found a TIMB tag, defining "<<timbre_count<<" timbres:"<<endl;
        //cout<<"Going to load timbres"<<endl;
        load_timbres(in, timbre_count);
        //cout<<"Loaded timbres."<<endl;
        load_tags(in);
    }
    else if(curtag == tag(EVNT)) {
        in>>curtag_size; curtag_size = bswap_32(curtag_size);
        //cout<<"Found an EVNT tag, size: "<<curtag_size<<endl;
        //cout<<"Going to load events."<<endl;
        return load_events(in);
    }
    else if(curtag == tag(XDIR)) {
        //cout<<"Found an XDIR tag"<<endl;
        load_tags(in);
    }
    else if(curtag == tag(XMID)) {
        //cout<<"Found an XMID tag"<<endl;
        load_tags(in);
    }
    else if(curtag == tag(RBRN)) { //Ignore RBRN, because I don't care
        in>>curtag_size; curtag_size = bswap_32(curtag_size);
        in.seekg(curtag_size, ios::cur);
        load_tags(in);
    }
    else {
        cout<<"Unknown tag: "<<hex<<curtag<<endl;
        return false;
    }
}

bool xmi::load_timbres(binifstream &in, uint16_t timbre_count) {
    for(int i=0;i<timbre_count;++i) {
        uint8_t patch=0, bank=0;
        in>>patch>>bank;
        //cout<<hex<<"\t#"<<i<<": Bank: "<<int(bank)<<"\tPatch: "<<int(patch)<<endl;
        timbres.push_back(pair<uint8_t,uint8_t>(bank,patch));
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
    vector<uint8_t> data;
    while(!in.eof()) {
        data.clear();
        in>>event;
        data.push_back(event);
        if(event < 128) {
            curtime += event;
            //cout<<dec<<curtime - event<<" + "<<int(event)<<" = "<<curtime<<" * 1/120 seconds"<<endl;
        }
        else {
            switch(event & 0xf0) {
            case 0x90:
                in>>note>>velocity;
                //cout<<hex<<"Note on (channel: "<<(event&0x0f)<<" note: "<<int(note)<<" velocity: "<<int(velocity)<<" at "<<dec<<curtime<<")"<<endl;
                data.push_back(note);
                data.push_back(velocity);
                events.push_back(midi_event(curtime, data));
                data[0] -= 0x10;
                note_off_time = curtime + midi_event::vlq2int(in);
                //cout<<"Note off (note: "<<int(note)<<" velocity: "<<int(velocity)<<" at "<<dec<<note_off_time<<")"<<endl;
                events.push_back(midi_event(note_off_time, data));
                break;
            case 0xb0:
                in>>controller>>value;
                //cout<<"Control change ("<<hex<<int(controller)<<" = "<<int(value)<<")"<<endl;
                data.push_back(controller);
                data.push_back(value);
                events.push_back(midi_event(curtime, data));
                break;
            case 0xc0:
                in>>value;
                //cout<<"Patch change (Channel #"<<(event&0x0f)<<" = "<<int(value)<<")"<<endl;
                data.push_back(value);
                events.push_back(midi_event(curtime, data));
                break;
            case 0xe0:
                in>>value;
                data.push_back(value);
                in>>value;
                data.push_back(value);
                events.push_back(midi_event(curtime, data));
                break;
            case 0xf0:
                in>>meta_event>>meta_size;
                //cout<<"Meta event "<<hex<<int(meta_event);
                data.push_back(meta_event);
                data.push_back(meta_size);
                for(size_t i = 0; i < meta_size; ++i) {
                    in>>cur_data_byte;
                    //cout<<hex<<" "<<int(cur_data_byte);
                    data.push_back(cur_data_byte);
                }
                //cout<<endl;
                events.push_back(midi_event(curtime, data));
                if(meta_event == 0x2f) {
                    //events.push_back(midi_event(curtime,data));
                    cout<<"Loading reached the end of the track. Found "<<dec<<events.size()<<" MIDI events."<<endl;
                    //cout<<"Sorting the events"<<endl;
                    sort(events.begin(), events.end(), midi_event::sort_by_time);
                    //cout<<"Done sorting the events."<<endl;
                    e_it = events.begin();
                    return true;
                }

                break;
            default:
                cout<<"Hullo, I don't know what command "<<hex<<(event & 0xf0)<<" is for yet :-D"<<endl;
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

#ifdef STAND_ALONE
int main(int argc, char *argv[]) {
    xmi in;
    bool success = in.load(argv[1]);
    if(success) {
        cout<<"Successfully loaded the XMI file."<<endl;
    }
    else {
        cout<<"There was an error loading the XMI file."<<endl;
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
    while(a != NULL) {
        a->toString();
        int16_t channel=0, command=0;
        command = a->get_command();
        channel = a->get_channel();
        cout<<dec;
        if(command == 0x80) { //midi note-off
            channel_on[channel]--;
            at_once--;
            on_count++;
            //cout<<"Reduced channel "<<channel<<"'s playing notes to "<<channel_on[channel]<<" and total playing to "<<at_once<<endl;
        }
        else if(command == 0x90) {
            channel_on[channel]++;
            at_once++;
            off_count++;
            if(at_once > at_once_max) { at_once_max = at_once; }
            if(channel_on[channel] > channel_max[channel]) { channel_max[channel] = channel_on[channel]; total_max[channel] = at_once; }
            //cout<<"Increased channel "<<channel<<"'s playing notes to "<<channel_on[channel]<<" and total playing to "<<at_once<<endl;
        }
        a = in.next_event();
    }
    cout<<"Saw a maximum of "<<at_once_max<<" notes playing at once. Saw "<<on_count<<" note-on events, saw "<<off_count<<" note-off events."<<endl;
    cout<<"Maximums:\n";
    for(int i=0;i<16;++i)
        cout<<"\tChannel "<<i<<": "<<channel_max[i]<<" ( out of "<<total_max[i]<<" playing at the time)"<<endl;
}
#endif
