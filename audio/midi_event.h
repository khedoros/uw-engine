#ifndef MIDI_EVENT_H
#define MIDI_EVENT_H

#include<vector>
#include<cstdint>
#include "../util.h"
#include<iostream>

class midi_event {
public:
    enum commands {
        NOTE_OFF = 0x80,
        NOTE_ON = 0x90,
        CONTROL_CHANGE = 0xb0,
        PROGRAM_CHANGE = 0xc0,
        PITCH_WHEEL = 0xe0,
        META = 0xf0
    };
        
    midi_event(uint32_t t, std::vector<uint8_t>& dat);
    midi_event();
    bool operator<(midi_event& other);
    void toString();
    uint8_t get_channel() const { return channel; }
    uint8_t get_command() const { return command; }
    uint8_t get_meta() const    { return meta_command; }
    bool is_end_command() const { return command == 0xf0 && data[1] == 0x2f; }
    static bool sort_by_time(const midi_event& a, const midi_event& b) {
        if(a.get_time() == b.get_time()) {
            if(a.get_command() == 0x80 && b.get_command() != a.get_command()) return true; //prioritise note-off commands
            else if(a.get_command() != 0x90 && b.get_command() == 0x90) return true;
            else return false;
            //else if(a.get_command() == 0x90 && b.get_command() != 0x90) return false; //make note-on commands lower priority
            //else return true;
        }
        else return a.get_time() < b.get_time();
    }
    uint32_t get_time() const;
    uint8_t *get_data();
    int get_data_size();
    static uint32_t vlq2int(binifstream& in) {
       uint8_t digit;
       uint32_t accumulator;
       in>>digit;
       //std::cout<<"First digit: "<<std::hex<<int(digit)<<std::endl;
       accumulator = (digit & 0x7f);
       while(digit >= 128) {
           accumulator *= 0x80;
           in>>digit;
           //std::cout<<"Next digit: "<<std::hex<<int(digit)<<std::endl;
           accumulator |= (digit & 0x7f);
       }
       return accumulator;
    }
    static uint32_t vlq2int(std::vector<uint8_t>& a) {
        uint32_t accumulator=a[0];
        uint8_t digit = a[0];
        size_t i = 0;
        while(digit >= 128) {
            std::cout<<std::hex<<int(digit)<<std::endl;
            accumulator *= 0x80;
            digit = (a[++i] & 0x7f);
            accumulator |= digit;
        }
        return accumulator;
    }
private:
    uint8_t channel, command, meta_command;
    uint32_t timestamp;
    std::vector<uint8_t> data;
};

#endif
