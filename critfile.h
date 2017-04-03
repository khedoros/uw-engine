#pragma once
#include<SFML/Graphics.hpp>
#include<string>
#include<vector>
#include<fstream>
#include<stdint.h>
#include<tuple>
#include "util.h"

class critfile {
public:
    critfile() : pal(0), auxpal(32), crits(64), mask(0), nibble_count(0) {}
    bool load(const std::string& critfile, const std::string& palfile);
    std::vector<sf::Texture> tex;
    typedef struct {
        sf::Texture tex;
        uint8_t hotspot_x;
        uint8_t hotspot_y;
    } frame;
    typedef struct {
        std::string name;
        uint8_t anim_num;
        uint8_t palnum;
        //First index: Slot. Second index: Frame.
        std::vector<std::vector<critfile::frame> > frames;
    } critanim;
    std::vector<critanim> crits;
    typedef enum {
        ATTACK = 0,
        BASH = 1,
        STAB = 2,
        SLASH = 3,
        MISSILE = 5,
        AWALK = 7,
        DEATH = 12,
        DRAW = 13,
        IDLE_0 = 32,
        IDLE_45 = 33,
        IDLE_90 = 34,
        IDLE_135 = 35,
        IDLE_180 = 36,
        IDLE_225 = 37,
        IDLE_270 = 38,
        IDLE_315 = 39,
        WALK_0 = 128,
        WALK_45 = 129,
        WALK_90 = 130,
        WALK_135 = 131,
        WALK_180 = 132,
        WALK_225 = 133,
        WALK_270 = 134,
        WALK_315 = 135
    } slots;

private:
    uint16_t get_rle_count(std::ifstream& in, uint16_t& col, uint8_t& bits_remaining, const uint8_t nibble);
    uint8_t get_nibble(std::ifstream& in, uint16_t& col, uint8_t& bits_remaining, const uint8_t nibble);
    bool read_palette(const std::string& palfile);
    bool read_assoc(const std::string& assocfile, std::vector<std::tuple<std::string, uint8_t, uint8_t> >& assocs);
    bool read_nxx(const std::string& nxxfile, const uint8_t index);
    std::vector<std::string> build_file_list(const std::string& critfile);
    frame* read_crit(std::ifstream& in, const uint32_t offset);
    struct color {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    std::vector<color> pal;
    std::vector<color> auxpal;
    uint8_t mask;
    uint16_t nibble_count;
};

