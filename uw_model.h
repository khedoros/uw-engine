#pragma once

#include<string>
#include<vector>
#include<array>
#include<cstdint>
#include<fstream>
#include "util.h"
#include "palette.h"

class uw_model {
    public:

    typedef enum {
        geometry,
        normals,
        colors,
        texcoords,
        shading
    } output_type;

    uw_model();
    bool load(const std::string&, const std::string&, int);
    std::vector<float> get_verts(output_type);

    struct point {
        point() {}
        point(float nx, float ny, float nz, int vn, bool vh) : x(nx), y(ny), z(nz), vertno(vn), var_height(vh) {}
        float x,y,z;
        int vertno; //vertex number, as separate from its index in an array of vertices for a model or face
        bool var_height; //Some points are "variable height", reaching all the way to the ceiling, etc.
        //color c; //rgba from util
        color c; //point color, which may be different from the face color
        bool has_col;
        float u,v;
    };

    struct face {
        public:
        face() {}
        face(bool gor, bool tex, float nxv, float nyv, float nzv, float dx, float dy, float dz, int texnum) : 
            goraud(gor), texture(tex), nx(nxv), ny(nyv), nz(nzv), dist_x(dx), dist_y(dy), dist_z(dz), texture_num(texnum) {}
        bool goraud;
        bool texture;
        bool fugly;
        bool has_col;
        std::vector<point> points;
        float nx, ny, nz; //normal vector
        float dist_x, dist_y, dist_z; //vector for distance from center
        int texture_num;  //texture==true && texture_num==-1 if no specific texture was assigned?
        color c;        //Face color
    };
    std::vector<point> points;
    std::vector<face>  faces;
    static std::vector<std::vector<uint8_t>> pal_dat; //stores palette indexes
    static palette pal;
    static uint32_t shade_offset;
    static bool pal_loaded;
    float cent_x, cent_y, cent_z;
    int cent_vert;
    float extent_x, extent_y, extent_z;
    int model_index;

    private:
    bool check_offset(uint32_t, std::ifstream&);
    uint32_t get_color_table_offset(std::ifstream&);
    int process_nodes(std::ifstream&);
    static const std::array<uint32_t, 6> start_offsets;
    static const std::array<uint32_t, 6> model_table_offsets;
    point base_pt;
    face base_face;
};
