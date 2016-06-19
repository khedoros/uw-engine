#pragma once

#include<string>
#include<stdint.h>
#include<vector>

class uwfont {
    std::vector<std::vector<std::vector<uint8_t>>> font;
    std::vector<uint8_t> widths;
    uint16_t space_width; //in pixels
    uint16_t font_height; //in pixels (how many rows of data does each character have)
    uint16_t max_char_width; //in pixels
    uint16_t font_size; //in pixels (how many rows of data are non-zero in the font)
    std::string filename;
public:
    bool load(std::string fn);
    void print();
    std::string to_bdf();
    uint32_t get_height();
};
