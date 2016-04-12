#pragma once
#include<stdint.h>
#include<string>
#include<tuple>
namespace vocfile {
    bool print_info(std::string&);
    std::tuple<size_t, size_t> get_data_loc(std::string&);
    std::vector<int16_t> get_file_dat(std::string&);
    bool check(char *);
};
