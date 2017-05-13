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
        int vertno; //vertex number, as separate from its index in an array of vertices for a model or face
        bool var_height; //Some points are "variable height", reaching all the way to the ceiling, etc.
    };

    class uv {
        color c; //rgba from util
        float u,v;
    };

    class face {
        bool goraud;
        std::vector<point> points;
        std::vector<uv>    point_attribs; //UV and color values are unique to points, but can vary by face
        float nx, ny, nz; //normal vector
    };
    std::vector<point> points;
    std::vector<std::vector<size_t>> faces;

    private:
    bool check_offset(uint32_t, std::ifstream&);
    int process_node(std::ifstream&);
    static const uint32_t start_offsets[];
    static const uint32_t model_table_offsets[];
    point base_pt;
    float x_ext, y_ext, z_ext, x_cent, y_cent, z_cent;
};
