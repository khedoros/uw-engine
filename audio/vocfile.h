#pragma once
#include<cstdint>
#include<string>
#include<tuple>

class vocfile {
    public:
    static bool print_info(const std::string&);
    static std::tuple<size_t, size_t> get_data_loc(const std::string&);
    static std::vector<int16_t> get_file_dat(const std::string&);
    static bool check(char *);
};
