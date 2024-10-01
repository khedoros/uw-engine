#ifndef XMI_H
#define XMI_H

#include "../util.h"
#include "midi_event.h"

#include<stdint.h>
#include<string>
#include<vector>

class xmi {
public:
    xmi() : timbres(0), events(0), curtime(0) {}
    bool load(std::string fn);
    std::pair<uint8_t, uint8_t> * next_timbre();
    void reset_timbre_it() { t_it = timbres.begin(); }

    midi_event * next_event();
    std::vector<midi_event>::iterator& get_event_it() { return e_it; };
    void set_event_it(std::vector<midi_event>::iterator& new_it) { e_it = new_it; };
    uint32_t tick_count();
private:
    uint32_t tag(const char chars[5]);
    bool load_tags(binifstream &in);
    bool load_timbres(binifstream &in, uint16_t timbre_count);
    bool load_events(binifstream &in);

    std::vector<std::pair<uint8_t,uint8_t>> timbres;
    std::vector<std::pair<uint8_t,uint8_t>>::iterator t_it;
    std::vector<midi_event> events;
    std::vector<midi_event>::iterator e_it;


    static const char FORM[];
    static const char XDIR[];
    static const char INFO[];
    static const char CAT_[];
    static const char XMID[];
    static const char RBRN[];
    static const char EVNT[];
    static const char TIMB[];

    uint32_t curtime;
};
#endif
