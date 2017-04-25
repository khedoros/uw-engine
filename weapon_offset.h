#pragma once
#include "util.h"
#include <vector>
#include <string>

class weapon_offset {
public:
    weapon_offset();
    bool load(std::string filename);
    uint8_t getx(uint8_t i);
    uint8_t gety(uint8_t i);
private:
    std::vector<std::pair<uint8_t, uint8_t>> offsets;
};
