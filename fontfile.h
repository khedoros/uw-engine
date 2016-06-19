#pragma once

#include<string>
#include<stdint.h>
#include<vector>

class fontfile {
    std::vector<std::vector<std::vector<uint8_t>>> font;
    std::vector<uint8_t> widths;
    uint16_t space_width; //in pixels
    uint16_t font_height; //in pixels
    uint16_t max_char_width; //in pixels
    std::string filename;
public:
    bool load(std::string fn);
    void print();
    std::string to_bdf();
};
