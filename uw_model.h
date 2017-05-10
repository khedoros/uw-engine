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
        float u,v;
        color c; //rgba from util
        int vertno; //vertex number, as separate from its index in an array of vertices for a model or face
        bool var_height; //Some points are "variable height", reaching all the way to the ceiling, etc.
    };

    class face {
        bool goraud;
        bool texture;
        std::vector<point> points;
        float nx, ny, nz; //normal vector
        int texture_num;  //texture==true && texture_num==-1 if no specific texture was assigned?
    };
    std::vector<point> points;
    std::vector<std::vector<size_t>> faces;

    private:
    bool check_offset(uint32_t, std::ifstream&);
    int skip_data(std::ifstream&);
    static const uint32_t start_offsets[];
    static const uint32_t model_table_offsets[];
};
