#pragma once
#include "UwText.h"
#include<iostream>
#include<string>
#include<fstream>
#include<stdint.h>
#include<vector>
#include "util.h"

class simple_map {
public:
    simple_map() : file_ident(UNKNOWNTYPE), levels(0), loaded_tile(false), loaded_tex(false), loaded_anim(false) {}
    bool load(std::string& filename);
    void print_map(const size_t index, const int option = 0, const UwText& uwt = UwText());
    void graphic_display();

    enum tile_type {
        SOLID_TILE = 0,
        OPEN_TILE  = 1,
        DIAG_SE    = 2,
        DIAG_SW    = 3,
        DIAG_NE    = 4,
        DIAG_NW    = 5,
        SLOPE_N    = 6,
        SLOPE_S    = 7,
        SLOPE_E    = 8,
        SLOPE_W    = 9,
        INVALID    = 10
    };

    struct __attribute__((packed)) static_obj {
        void print();
        uint16_t get_next();
        unsigned obj_id:9;
        unsigned flags:4; //"enchantable" if high bit of flags is set?
        //unsigned enchantable:1;
        unsigned doordir:1;
        unsigned invisible:1;
        unsigned quantitied:1;

        unsigned zpos:7;
        unsigned heading:3;
        unsigned ypos:3;
        unsigned xpos:3;

        unsigned quality:6;
        unsigned next_index:10;

        unsigned owner:6;
        unsigned quantity:10;
    };

    struct __attribute__((packed)) mobile_obj {
        void print();
        uint16_t get_next();
        static_obj info;

        unsigned hp:8; //0000
        unsigned pad:16; //0001-0002

        unsigned goal:4; //0003-0004
        unsigned gtarg:8;
        unsigned pad2:4;

        unsigned level:4; //0005-0006
        unsigned pad3:9;
        unsigned talked_to:1;
        unsigned attitude:2;

        unsigned pad4:6; //0007-0008
        unsigned height:7;
        unsigned pad5:3;

        unsigned pad6:16; //0009-000a
        unsigned pad7:8; //000b
        unsigned pad8:16; //000c-000d

        unsigned unknown:4; //000e-000f
        unsigned yhome:6;
        unsigned xhome:6;

        unsigned heading:5; //0010
        unsigned pad9:3;
        unsigned hunger:7; //0011
        unsigned pad10:1;

        unsigned whoami:8; //0012
    };

    struct map_data {
        simple_map::tile_type type;
        unsigned  height;
        unsigned  floor_tex;
        bool      magic_allowed;
        bool      is_door;
        unsigned  wall_tex;
        unsigned  first_obj;
    };

    struct level {
        map_data d[64][64];
        bool is_explored[64][64];
        mobile_obj npcs[256];
        static_obj items[768];
        uint16_t wall_tex_index[48];
        uint16_t floor_tex_index[10];
        uint8_t  door_tex_index[6];
        unsigned ceil_tex_index;
    };
    uint64_t file_ident;
    std::vector<level> levels;

private:
    bool load_tile(std::ifstream& in, size_t offset = 0, size_t index = 0);
    bool load_tex(std::ifstream& in, size_t offset = 0, size_t index = 0);
    bool load_anim(std::ifstream& in, size_t offset = 0, size_t index = 0);
    bool load_automap_info(std::ifstream& in, size_t offset = 0, size_t index = 0);
    bool load_automap_notes(std::ifstream& in, size_t offset = 0, size_t index = 0);
    bool load_uw1map(std::ifstream& in);
    enum map_type {
        UNKNOWNTYPE =     0x0000000000000000,
        UW1DEMOTILE =     0x000000f0000000f0,
        UW1DEMOTEXMAP =   0x0003000200010000,
        UW2MAP =          0x1406000000000140,
        UW1MAP =          0x7e260000021e0087,
        UW1DEMOANIMDAT =  0xbf003d1effffa680
    };
    bool loaded_tile,loaded_tex,loaded_anim;

//0 1 2 3 4  5  6  7  8   9   a   b   c    d    e    f
//1 2 4 8 10 20 40 80 100 200 400 800 1000 2000 4000 8000
    static const uint16_t BITS_0_3 =   0x000f;
    static const uint16_t BITS_4_7 =   0x00f0;
    static const uint16_t BIT_8 =      0x0100;
    static const uint16_t BIT_9 =      0x0200;
    static const uint16_t BITS_10_13 = 0x3c00;
    static const uint16_t BIT_14 =     0x4000;
    static const uint16_t BIT_15 =     0x8000;
    static const uint16_t BITS_0_5 =   0x003f;
    static const uint16_t BITS_6_15 =  0xFFC0;

    simple_map::tile_type get_tile_type(uint16_t dat1);
    uint8_t   get_floor_height(uint16_t dat1);
    uint8_t   get_floor_index(uint16_t dat1);
    bool      is_magic_allowed(uint16_t dat1);
    bool      is_door(uint16_t dat1);
    uint8_t   get_wall_index(uint16_t dat2);
    uint16_t  get_obj_list_start(uint16_t dat2);
};

