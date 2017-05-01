#pragma once
#include<stdint.h>
#include<string>
#include<tuple>
namespace vocfile {
    bool print_info(const std::string&);
    std::tuple<size_t, size_t> get_data_loc(const std::string&);
    std::vector<int16_t> get_file_dat(const std::string&);
    bool check(char *);
};
