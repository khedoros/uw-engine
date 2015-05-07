#include "palette.h"
#include "util.h"

palette::palette() : colors(256) {
    for(auto i = colors.begin(); i != colors.end(); ++i) {
        *i = {0,0,0,0};
    }
}

bool palette::load(std::string filename, size_t pal_num) {
    binifstream infile;
    infile.open(filename.c_str(), std::ios::binary | std::ios::in);
    if(!infile.is_open()) {
        return false;
    }
    infile.seekg(0,std::ios::end);
    int filesize = infile.tellg();
    if(filesize != 6144)
        return false;
    if(pal_num < 8) {
        infile.seekg(768 * pal_num, std::ios::beg);
        for(int i = 0; i < 256; ++i) {
            uint8_t r, g, b;
            infile>>r;
            infile>>g;
            infile>>b;
            colors[i]=color(uint8_t(r * 4), uint8_t(g * 4), uint8_t(b * 4), 255);
        }
    }
    infile.close();
    return true;
}

color palette::get(uint8_t i) {
    return colors[i];
}

uint8_t palette::getr(uint8_t i) {
    return colors[i].r;
}

uint8_t palette::getg(uint8_t i) {
    return colors[i].g;
}

uint8_t palette::getb(uint8_t i) {
    return colors[i].b;
}

uint8_t palette::geta(uint8_t i) {
    return colors[i].a;
}
