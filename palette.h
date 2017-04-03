#pragma once
#include "util.h"
#include <vector>
#include <string>

class palette {
public:
    palette();
    bool load(std::string filename, size_t pal_num);
    color get(uint8_t i);
    uint8_t getr(uint8_t i);
    uint8_t getg(uint8_t i);
    uint8_t getb(uint8_t i);
    uint8_t geta(uint8_t i);
private:
    std::vector<color> colors;
};
