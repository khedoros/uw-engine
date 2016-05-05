#include "midi_event.h"
#include<vector>
#include<string>
#include<iostream>
using namespace std;

midi_event::midi_event(uint32_t t, std::vector<uint8_t>& dat) : timestamp(t), data(dat), meta_command(0) {
    channel = data[0] & 0x0f;
    command = data[0] & 0xf0;
    //cout<<"Midi event: Cmd: "<<hex<<int(command)<<" Time: "<<dec<<timestamp<<endl;
    if(command == 0xf0 || command == 0xb0) //b0 is a controller change, not a meta-command, but I need a similar branching behavior to handle it.
        meta_command = data[1];
}

midi_event::midi_event() : timestamp(0), data(0), meta_command(0) {}

uint32_t midi_event::get_time() const {
    return timestamp;
}

uint8_t * midi_event::get_data() {
    return data.data();
}

void midi_event::toString() {
    std::cout<<std::dec<<timestamp<<": Cmd: "<<std::hex<<int(command);
    if(command == 0xf0) std::cout<<" Meta: "<<int(meta_command);
    std::cout<<std::dec<<" Ch: "<<int(channel)<<std::endl;
}
