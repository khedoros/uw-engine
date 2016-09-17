#include "kail.h" //should define at least: kail_startup(), kail_shutdown, a kail_seq class, a kail_voc class, and a kail_sfx class with "load", "start", "stop", "pause", and "wait" methods
#include "../uw_patch.h"
#include "../xmi.h"
#include<iostream>
#include<fstream>
#include<cstdint>
#include<string>

using namespace std;

int main(int argc, char *argv[]) {
    cout<<"KAIL xplay\n";
    if(argc < 3) {
        cerr<<"This program plays an Extended MIDI (XMIDI) sequence through an \n"
            <<"emulated Yamaha YMF-262 (OPL3/Ad-Lib) device.\n"
            <<"Usage: xplay xmidi_filename timbre_library_filename [sequence_number]\n";
        return 1;
    }

    uint16_t seqnum = 0;

    if(argc == 4) seqnum = stoi(argv[3]);

    uw_patch_file gtl;
    if(!gtl.load(argv[2])) { //Loads one or two timbre definition files, and provides access to their data
        cerr<<"Couldn't load timbres from "<<argv[2]<<". Aborting.\n";
        return 1;
    }

    //load driver: Just one driver, static-linked into the program. We don't have to worry about any others.

    kail_startup(gtl); //init SFML audio system, init data structures for tracking music and audio

    //register driver: Maybe if I reimplement some of this as a dynamically-loaded library

    //describe driver: Not really necessary. Maybe I'll do something like this later.

    //detect device: As far as I'm doing this, it'll be handled through SFML

    //init driver: Combination of "unnecessary" and "done in SFML"

    //State table: Handle it when we register a sequence to play.

    xmi midi_file;
    if(!midi_file.load(argv[1])) { //Loads XMI file as a series of MIDI events, and provides access to those events
        cerr<<"Failed to open "<<argv[1]<<". Aborting.\n";
        return 1;
    }

    kail_seq seq;
    if(!seq.load(midi_file, seqnum)) {
        cerr<<"Invalid sequence number or bad MIDI file. Aborting.\n";
        return 1;
    }

    //Timbre requests and installs: Not worried about this on a modern machine.
    

    seq.start(); //Start playing the sequence with the specified xmi and timbres
    seq.wait();  //Wait until the sequence is completed

    kail_shutdown();

}
