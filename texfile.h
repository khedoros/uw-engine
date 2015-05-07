#ifndef TEXFILE_H
#define TEXFILE_H
#include<SFML/Graphics.hpp>
#include<string>
#include<vector>
#include<fstream>
#include<stdint.h>
#include "util.h"

class texfile {
public:
    texfile () : res(0), xres(0), yres(0), allpals(0), altpal(0) {}
    bool load(const std::string& palfile,const std::string& texfile, const std::string& altpalfile = std::string());
    std::vector<sf::Texture> tex;
    std::vector<std::vector<sf::Texture> > animtex;

    //Fill in the palette-shifted versions of the textures.
    //orig_tex: The original, already-populated texture array.
    //out_tex: 2D vector of textures. 1st D: texture choice. 2nd D: frame of the texture
    //texnums: index of texture to generate frames from
    //low_pal, hi_pal: the low and high palette numbers, forming the range that we want to rotate palettes through.
    void populate_animtex(const size_t texnum, const uint8_t low_pal, const uint8_t hi_pal);
    uint16_t res; //Used for .tr files, which are always square
    //xres and yres are used to store the largest dimensions seen in a .gr file, for scaling when displaying the files.
    uint16_t xres;
    uint16_t yres;
private:
    bool load_tr(std::ifstream& in);
    bool load_gr(std::ifstream& in, const bool no_img_hdr = false);
    bool load_byt(std::ifstream& in, const std::string& name);
    bool read_palette(const std::string& palfile, const std::string& altpalfile = std::string());
    sf::Texture read_texture(std::ifstream& in, const uint32_t offset);
    sf::Texture read_bmp(std::ifstream& in, const uint32_t offset, const bool no_img_hdr = false);
    uint16_t get_rle_count(std::ifstream& in, uint8_t& col, bool& onfirst);
    uint8_t get_nibble(std::ifstream& in, uint8_t& col, bool& onfirst);
    struct color {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    //std::vector<color> palette;
    std::vector<std::vector<color> > allpals;
    std::vector< std::vector<color> > altpal;
};

#endif
