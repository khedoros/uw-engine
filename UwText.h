#include<iostream>
#include "util.h"
#include <string>
#include<vector>

#ifndef UWTEXT_H
#define UWTEXT_H
class UwText {
public:
    UwText();
    size_t get_block_count() const;
    size_t get_block_num(size_t blocknum) const;
    std::string get_block_name(size_t blocknum) const;
    size_t get_string_count(size_t blocknum) const;
    std::string get_string(size_t blocknum, size_t stringnum) const;
    bool load(std::string filename);
private:
    typedef struct {
        uint8_t c;
        uint8_t p;
        uint8_t l;
        uint8_t r;
    } huffnode;

    typedef struct {
        uint16_t block_num;
        uint32_t block_off;
        std::string name;
        std::vector<std::string> strings;
    } block_data;

    std::vector<huffnode> nodes;
    std::vector<block_data> blocks;

    std::string decode_string(binifstream& in, size_t offset);
    bool process_block(binifstream& in, int num);
};
#endif
