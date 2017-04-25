#include "weapon_offset.h"
#include "util.h"
#include<iostream>

weapon_offset::weapon_offset() : offsets(224) {
}

bool weapon_offset::load(std::string filename) {
    binifstream infile;
    infile.open(filename.c_str(), std::ios::binary | std::ios::in);
    if(!infile.is_open()) {
        return false;
    }
    infile.seekg(0,std::ios::end);
    int filesize = infile.tellg();
    infile.seekg(0,std::ios::beg);
    if(filesize != 448) {
        return false;
    }
    uint8_t off[224][2];
    for(int set = 0; set < 8; set++) { //8 sets of attack types
        for(int x_or_y=0; x_or_y < 2; ++x_or_y) { //x listed first, then y
            for(int frame = 0; frame < 28; ++frame) { //frames of animation
                int index = (set * 28) + frame;
                infile>>off[index][x_or_y];
            }
        }
    }
    for(int frames=0;frames<224;++frames) {
        offsets[frames] = std::make_pair(off[frames][0], off[frames][1]);
        std::cout<<"Frame "<<frames<<" offset: "<<int(offsets[frames].first)<<", "<<int(offsets[frames].second)<<std::endl;
    }
    infile.close();
    return true;
}

uint8_t weapon_offset::getx(uint8_t i) {
    return offsets[i].first;
}

uint8_t weapon_offset::gety(uint8_t i) {
    return offsets[i].second;
}
