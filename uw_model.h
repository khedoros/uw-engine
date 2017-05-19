#pragma once

#include<string>
#include<vector>
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
        uint16_t c;//variable address, which can be used to look up a palette entry
        int shade;
        float u,v;
    };

    struct face {
        public:
        face() {}
        face(bool gor, bool tex, float nxv, float nyv, float nzv, float dx, float dy, float dz, int texnum) : 
            goraud(gor), texture(tex), nx(nxv), ny(nyv), nz(nzv), dist_x(dx), dist_y(dy), dist_z(dz), texture_num(texnum) {}
        bool goraud;
        bool texture;
        std::vector<point> points;
        float nx, ny, nz; //normal vector
        float dist_x, dist_y, dist_z; //vector for distance from center
        int texture_num;  //texture==true && texture_num==-1 if no specific texture was assigned?
        int c;        //Base color
        int shade;    //Shade color
    };
    std::vector<point> points;
    std::vector<face>  faces;
    palette pal;
    float cent_x, cent_y, cent_z;
    int cent_vert;
    float extent_x, extent_y, extent_z;

    private:
    bool check_offset(uint32_t, std::ifstream&);
    int process_nodes(std::ifstream&);
    static const uint32_t start_offsets[];
    static const uint32_t model_table_offsets[];
    point base_pt;
    face base_face;
};
