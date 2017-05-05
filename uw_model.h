#pragma once

#include<string>
#include<vector>
#include<cstdint>
#include<fstream>
#include "util.h"

class uw_model {
    public:
    uw_model();
    bool load(const std::string&, int);

    class point {
        float x,y,z;
        color c; //rgba from util
        float u,v;
        bool variable; //Some points are "variable height", reaching all the way to the ceiling, etc.
    };

    std::vector<point> points;
    std::vector<std::vector<size_t>> faces;

    //private:
    bool check_offset(uint32_t, std::ifstream&);
    int skip_data(std::ifstream&);
    static const uint32_t start_offsets[];
    static const uint32_t model_table_offsets[];

};
