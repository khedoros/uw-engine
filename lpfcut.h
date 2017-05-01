#pragma once
#include <string>
#include <vector>
#include <stdint.h>

#include "util.h"

class Lpfcut {
public:
    Lpfcut(bool rep = true);
    bool load(std::string);
    const uint8_t * getNextFrame();
    color getPalEntry(int i);
    uint16_t width;
    uint16_t height;
    bool repeat;
    uint32_t rec_count;

private:
    static const size_t PAL_OFFSET=256;
    static const size_t LPD_OFFSET=PAL_OFFSET+(4*256);
    static const size_t FIRST_LP_OFFSET=LPD_OFFSET+(6*256);
    static const size_t LP_SIZE=64*1024;

    char file_id[5];
    char content_type[5];
    uint16_t lp_count;
    std::vector<size_t> lp_order;

    //LPF files have their palettes internal to the file, so I don't want to use a palette class to load them.
    color pal[256];

    typedef struct {
        uint16_t first_record;
        uint16_t lp_rec_count;
        uint16_t data_size;
        std::vector<uint16_t> rec_len;
        std::vector<std::vector<uint8_t>> rec_data;
    } lpdesc;

    lpdesc descs[256];

    std::vector<uint8_t> frame;
    size_t frame_page_index;
    size_t frame_record_index;
public:    bool iterate_frame();
};
