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
        public:
        point() {}
        point(float nx, float ny, float nz, int vn, bool vh) : x(nx), y(ny), z(nz), vertno(vn), var_height(vh) {}
        float x,y,z;
        int vertno; //vertex number, as separate from its index in an array of vertices for a model or face
        bool var_height; //Some points are "variable height", reaching all the way to the ceiling, etc.
    };

    class uv {
        color c; //rgba from util
        float u,v;
    };

    class face {
        public:
        face() {}
        face(bool gor, bool tex, float nxv, float nyv, float nzv, float dx, float dy, float dz, int texnum) : 
            goraud(gor), texture(tex), nx(nxv), ny(nyv), nz(nzv), dist_x(dx), dist_y(dy), dist_z(dz), texture_num(texnum) {}
        bool goraud;
        bool texture;
        std::vector<point> points;
        std::vector<uv>    point_attribs; //UV and color values are unique to points, but can vary by face
        float nx, ny, nz; //normal vector
        float dist_x, dist_y, dist_z; //vector for distance from center
        int texture_num;  //texture==true && texture_num==-1 if no specific texture was assigned?
        int c;        //Base color
        int shade;    //Shade color
    };
    std::vector<point> points;
    std::vector<face>  faces;
    float cent_x, cent_y, cent_z;
    int cent_vert;
    float extent_x, extent_y, extent_z;

    private:
    bool check_offset(uint32_t, std::ifstream&);
    int process_node(std::ifstream&);
    static const uint32_t start_offsets[];
    static const uint32_t model_table_offsets[];
    point base_pt;
    uv    base_pt_attr;
    face base_face;
};
