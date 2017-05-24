#include<iostream>
#include<fstream>
#include<string>
#include<cstdint>
#include<vector>
#include<list>
#include "util.h"
#include "uw_model.h"

using namespace std;

const uint32_t uw_model::start_offsets[] =       {0x4e910, 0x4ccd0, 0x4e370, 0x4ec70, 0x54cf0, 0x550e0};
const uint32_t uw_model::model_table_offsets[] = {0x4e99e, 0x4cd5e, 0x4e3fe, 0x4ecfe, 0x54d8a, 0x5517a};
uw_model::uw_model() {
}

bool uw_model::check_offset(uint32_t offset, ifstream& in) {
    const uint8_t uw1_first_bytes[] = {0xb6, 0x4a, 0x06, 0x40};
    const uint8_t uw2_first_bytes[] = {0xd4, 0x64, 0xaa, 0x59};
    uint32_t bookmark = in.tellg();
    in.seekg(offset, ios::beg);
    for(int i=0;i<4;++i) {
        uint8_t temp = read8(in);
        if(temp != uw1_first_bytes[i] && temp != uw2_first_bytes[i]) {
            in.seekg(bookmark, ios::beg);
            return false;
        }
    }
    in.seekg(bookmark, ios::beg);
    return true;
}

int uw_model::process_nodes(ifstream& in) {
    int retval = 0;
    size_t offset = in.tellg();
    list<size_t> offsets(1,offset);
    while(offsets.size() > 0) {
        in.seekg(offsets.front());
        offsets.pop_front();
        bool exit = false;
        while(!exit) {
            uint16_t node_type = read16(in);
            offset = in.tellg();
#ifdef STAND_ALONE_MODEL
            cout<<hex<<offset<<": type 0x"<<node_type<<dec<<" ";
#endif
            uint8_t dat_size = 0;
            uint16_t unk16;
            switch(node_type) {
                case 0x0000:
#ifdef STAND_ALONE_MODEL
                    cout<<"end node (";
#endif
                    if(offsets.size() > 0) {
#ifdef STAND_ALONE_MODEL
                        cout<<"end of section)"<<endl;
#endif
                    }
                    else {
#ifdef STAND_ALONE_MODEL
                        cout<<"end of model)"<<endl;
#endif
                    }
                    exit = true;
                    break;
                case 0x0006:
                    for(int i=0;i<6;++i) {
                        unk16=read16(in);
                    }
                    {
                        std::streamoff offset1 = read16(in);
                        offsets.push_back(in.tellg() + offset1);
                        std::streamoff offset2 = read16(in);
                        offsets.push_back(in.tellg() + offset2);
                    }
#ifdef STAND_ALONE_MODEL
                    cout<<"Define sort node, arbitrary heading";
#endif
                    break;
                case 0x000C: 
                    for(int i=0;i<4;++i) {
                        unk16=read16(in);
                    }
                    {
                        std::streamoff offset1 = read16(in);
                        offsets.push_back(in.tellg() + offset1);
                        std::streamoff offset2 = read16(in);
                        offsets.push_back(in.tellg() + offset2);
                    }
#ifdef STAND_ALONE_MODEL
                    cout<<"define sort node, ZY plane";
#endif
                    break;
                case 0x000E: 
                    for(int i=0;i<4;++i) {
                        unk16=read16(in);
                    }
                    {
                        std::streamoff offset1 = read16(in);
                        offsets.push_back(in.tellg() + offset1);
                        std::streamoff offset2 = read16(in);
                        offsets.push_back(in.tellg() + offset2);
                    }
#ifdef STAND_ALONE_MODEL
                    cout<<"define sort node, XY plane";
#endif
                    break;
                case 0x0010: 
                    for(int i=0;i<4;++i) {
                        unk16=read16(in);
                    }
                    {
                        std::streamoff offset1 = read16(in);
                        offsets.push_back(in.tellg() + offset1);
                        std::streamoff offset2 = read16(in);
                        offsets.push_back(in.tellg() + offset2);
                    }
#ifdef STAND_ALONE_MODEL
                    cout<<"define sort node, XZ plane";
#endif
                    break;
                case 0x0014: 
#ifdef STAND_ALONE_MODEL
                    cout<<"??? color definition (Skip, because not enough info) "<<(read16(in)>>(3))<<"\t"<<int(read8(in))<<"\t"<<int(read8(in))<<"\t"<<(read16(in)>>(3))<<endl;
#else
                    read16(in); read16(in); read16(in);
#endif
                    break;
                case 0x002E:
                    unk16 = read16(in);
#ifdef STAND_ALONE_MODEL
                    cout<<"??? Used a lot in the Lotus model!"<<endl;
#endif
                    break;
                case 0x0040: 
#ifdef STAND_ALONE_MODEL
                    cout<<"??? seems to do nothing but introduce a face definition"<<endl;
#endif
                    break;
                case 0x0044: 
#ifdef STAND_ALONE_MODEL
                    cout<<"??? this one too (seems to intro a face def)"<<endl;
#endif
                    break;
                case 0x0058:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = fix2float(read16(in));
                    base_face.dist_x = fix2float(read16(in));
                    base_face.ny = fix2float(read16(in));
                    base_face.dist_y = fix2float(read16(in));
                    base_face.nz = fix2float(read16(in));
                    base_face.dist_z = fix2float(read16(in));
#ifdef STAND_ALONE_MODEL
                    cout<<"Define face plane, arbitrary heading, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
#endif
                    break;
                case 0x005E:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = 0;
                    base_face.dist_x = 0;
                    base_face.nz = fix2float(read16(in));
                    base_face.dist_z = fix2float(read16(in));
                    base_face.ny = fix2float(read16(in));
                    base_face.dist_y = fix2float(read16(in));
#ifdef STAND_ALONE_MODEL
                    cout<<"Define face plane, Z/Y, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
#endif
                    break;
                case 0x0060:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = fix2float(read16(in));
                    base_face.dist_x = fix2float(read16(in));
                    base_face.nz = 0;
                    base_face.dist_z = 0;
                    base_face.ny = fix2float(read16(in));
                    base_face.dist_y = fix2float(read16(in));
#ifdef STAND_ALONE_MODEL
                    cout<<"Define face plane, X/Y, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
#endif
                    break;
                case 0x0062:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = fix2float(read16(in));
                    base_face.dist_x = fix2float(read16(in));
                    base_face.nz = fix2float(read16(in));
                    base_face.dist_z = fix2float(read16(in));
                    base_face.ny = 0;
                    base_face.dist_y = 0;
#ifdef STAND_ALONE_MODEL
                    cout<<"Define face plane, X/Z, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
#endif
                    break;
                case 0x0064:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = fix2float(read16(in));
                    base_face.dist_x = fix2float(read16(in));
                    base_face.nz = 0;
                    base_face.dist_z = 0;
                    base_face.ny = 0;
                    base_face.dist_y = 0;
#ifdef STAND_ALONE_MODEL
                    cout<<"Define face plane, X, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
#endif
                    break;
                case 0x0066:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = 0;
                    base_face.dist_x = 0;
                    base_face.nz = fix2float(read16(in));
                    base_face.dist_z = fix2float(read16(in));
                    base_face.ny = 0;
                    base_face.dist_y = 0;
#ifdef STAND_ALONE_MODEL
                    cout<<"Define face plane, Z, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
#endif
                    break;
                case 0x0068:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = 0;
                    base_face.dist_x = 0;
                    base_face.nz = 0;
                    base_face.dist_z = 0;
                    base_face.ny = fix2float(read16(in));
                    base_face.dist_y = fix2float(read16(in));
#ifdef STAND_ALONE_MODEL
                    cout<<"Define face plane, Y, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
#endif
                    break;
                case 0x0078:
                    cent_vert = read16(in)>>(3);
                    cent_x = fix2float(read16(in));
                    cent_y = fix2float(read16(in));
                    cent_z = fix2float(read16(in));
                    unk16 = read16(in);
                    if(points.size() <= cent_vert ) {
                        points.resize(cent_vert + 1);
                    }
                    points[cent_vert] = point(cent_x,cent_y,cent_z, cent_vert, false);
#ifdef STAND_ALONE_MODEL
                    std::cout<<"Define model center ("<<cent_x<<", "<<cent_y<<", "<<cent_z<<") as num "<<cent_vert<<std::endl;
#endif
                    break;
                case 0x007A: 
                    base_pt.x = fix2float(read16(in));
                    base_pt.y = fix2float(read16(in));
                    base_pt.z = fix2float(read16(in));
                    base_pt.vertno = read16(in)>>(3);
                    base_pt.var_height = false;
                    if(points.size() <= base_pt.vertno ) {
                        points.resize(base_pt.vertno + 1);
                    }
                    points[base_pt.vertno] = base_pt;
#ifdef STAND_ALONE_MODEL
                    std::cout<<"Define initial vertex ("<<base_pt.x<<", "<<base_pt.y<<", "<<base_pt.z<<") as num "<<base_pt.vertno<<std::endl;; 
#endif
                    break;
                case 0x007E: 
                    base_face.points.resize(0);
                    {
                        int vcount = read16(in);
                        base_face.points.resize(vcount);
#ifdef STAND_ALONE_MODEL
                        cout<<"Define face vertices ("<<vcount<<", "<<base_face.points.size()<<"): ";
#endif
                    }
                    for(int i = 0; i < base_face.points.size(); ++i) {
                        int vnum = read16(in)>>(3);
#ifdef STAND_ALONE_MODEL
                        cout<<vnum<<" ";
#endif
                        assert(vnum < points.size());
                        base_face.points[i] = points[vnum];
                    }
#ifdef STAND_ALONE_MODEL
                    cout<<endl;
#endif
                    faces.push_back(base_face);
                    break;
                case 0x0082: {
                        int nverts = read16(in);
                        int start_vnum = read16(in)>>(3);
#ifdef STAND_ALONE_MODEL
                        cout<<"Define "<<nverts<<" initial vertices, starting at #"<<start_vnum<<endl;
#endif
                        //assert(points.size() == 0);
                        points.resize(nverts+start_vnum);
                        for(int i = 0; i < nverts; i++) {
                            points[start_vnum + i].x = fix2float(read16(in));
                            points[start_vnum + i].y = fix2float(read16(in));
                            points[start_vnum + i].z = fix2float(read16(in));
#ifdef STAND_ALONE_MODEL
                            cout<<"\tvert ("<<points[start_vnum + i].x<<", "<<points[start_vnum + i].y<<", "<<points[start_vnum + i].z<<")"<<endl;
#endif
                        }
                    }
                    break;
                case 0x0086: {
                        int base_vert = read16(in)>>(3);
                        point temp = points[base_vert];
                        temp.x += fix2float(read16(in));
                        temp.vertno = read16(in)>>(3);
                        if(points.size() <= temp.vertno ) {
                            points.resize(temp.vertno + 1);
                        }
                        points[temp.vertno] = temp;
                        assert(points.size() > temp.vertno);
#ifdef STAND_ALONE_MODEL
                        cout<<"Define vertex offset X ("<<temp.x<<", "<<temp.y<<", "<<temp.z<<") as num "<<temp.vertno<<endl;
#endif
                    }
                    break;
                case 0x0088: {
                        int base_vert = read16(in)>>(3);
                        point temp = points[base_vert];
                        temp.z += fix2float(read16(in));
                        temp.vertno = read16(in)>>(3);
                        if(points.size() <= temp.vertno ) {
                            points.resize(temp.vertno + 1);
                        }
                        points[temp.vertno] = temp;
                        assert(points.size() > temp.vertno);
#ifdef STAND_ALONE_MODEL
                        cout<<"Define vertex offset Z ("<<temp.x<<", "<<temp.y<<", "<<temp.z<<") as num "<<temp.vertno<<endl;
#endif
                    }
                    break;
                case 0x008A: {
                        int base_vert = read16(in)>>(3);
                        point temp = points[base_vert];
                        temp.y += fix2float(read16(in));
                        temp.vertno = read16(in)>>(3);
                        if(points.size() <= temp.vertno ) {
                            points.resize(temp.vertno + 1);
                        }
                        points[temp.vertno] = temp;
                        assert(points.size() > temp.vertno);
#ifdef STAND_ALONE_MODEL
                        cout<<"Define vertex offset Y ("<<temp.x<<", "<<temp.y<<", "<<temp.z<<") as num "<<temp.vertno<<endl;
#endif
                    }
                    break;
                case 0x008C: {
                        int base_vert = read16(in)>>(3);
                        point temp = points[base_vert];
                        unk16 = read16(in);
                        temp.vertno = read16(in)>>(3);
                        temp.var_height = true;
                        if(points.size() <= temp.vertno ) {
                            points.resize(temp.vertno + 1);
                        }
                        points[temp.vertno] = temp;

                        //My map height is 16, and I'm going to scale these by 2 later
                        points[temp.vertno].z = 2.0;
                        assert(points.size() > temp.vertno);
#ifdef STAND_ALONE_MODEL
                        cout<<"Set vertex num "<<base_vert<<" to variable height, as num "<<temp.vertno<<endl;
#endif
                    }
                    break;
                case 0x0090: {
                        float x = fix2float(read16(in));
                        float z = fix2float(read16(in));
                        int base_vert = read16(in)>>(3);
                        point temp = points[base_vert];
                        temp.vertno = read16(in)>>(3);
                        temp.x += x;
                        temp.z += z;
                        if(points.size() <= temp.vertno) {
                            points.resize(temp.vertno + 1);
                        }
                        points[temp.vertno] = temp;
                        assert(points.size() > temp.vertno);
#ifdef STAND_ALONE_MODEL
                        cout<<"Define vertex offset X,Z ("<<temp.x<<", "<<temp.y<<", "<<temp.z<<") as num "<<temp.vertno<<endl;
#endif
                    }
                    break;
                case 0x0092: {
                        float x = fix2float(read16(in));
                        float y = fix2float(read16(in));
                        int base_vert = read16(in)>>(3);
                        point temp = points[base_vert];
                        temp.vertno = read16(in)>>(3);
                        temp.x += x;
                        temp.y += y;
                        if(points.size() <= temp.vertno) {
                            points.resize(temp.vertno + 1);
                        }
                        points[temp.vertno] = temp;
                        assert(points.size() > temp.vertno);
#ifdef STAND_ALONE_MODEL
                        cout<<"Define vertex offset X,Y ("<<temp.x<<", "<<temp.y<<", "<<temp.z<<") as num "<<temp.vertno<<endl;
#endif
                    }
                    break;
                case 0x0094: {
                        float y = fix2float(read16(in));
                        float z = fix2float(read16(in));
                        int base_vert = read16(in)>>(3);
                        point temp = points[base_vert];
                        temp.vertno = read16(in)>>(3);
                        temp.y += y;
                        temp.z += z;
                        if(points.size() <= temp.vertno) {
                            points.resize(temp.vertno + 1);
                        }
                        points[temp.vertno] = temp;
                        assert(points.size() > temp.vertno);
#ifdef STAND_ALONE_MODEL
                        cout<<"Define vertex offset Y,Z ("<<temp.x<<", "<<temp.y<<", "<<temp.z<<") as num "<<temp.vertno<<endl;
#endif
                    }
                    break;
                case 0x00A0:
                    unk16=read16(in);
                    base_face.points.resize(4);
                    {
                        uint8_t vertno[4];
                        vertno[3] = read8(in);
                        vertno[2] = read8(in);
                        vertno[1] = read8(in);
                        vertno[0] = read8(in);

                        base_face.points[0] = points[vertno[0]];
                        base_face.points[0].u = frac2float(0);
                        base_face.points[0].v = frac2float(0);

                        base_face.points[1] = points[vertno[1]];
                        base_face.points[1].u = frac2float(65535);
                        base_face.points[1].v = frac2float(0);

                        base_face.points[2] = points[vertno[2]];
                        base_face.points[2].u = frac2float(65535);
                        base_face.points[2].v = frac2float(65535);

                        base_face.points[3] = points[vertno[3]];
                        base_face.points[3].u = frac2float(0);
                        base_face.points[3].v = frac2float(65535);
                        faces.push_back(base_face);
#ifdef STAND_ALONE_MODEL
                        cout<<"??? shorthand face definition (guessing at UV order), vertices "<<int(vertno[0])<<", "<<int(vertno[1])<<", "<<int(vertno[2])<<", and "<<int(vertno[3])<<endl;
#endif
                    }
                    break;
                case 0x00A8:
                    base_face.points.resize(0);
                    base_face.texture_num = read16(in);
                    {
                        int vcount = read16(in);
                        base_face.points.resize(vcount);
                        //cout<<"Define face vertices ("<<vcount<<", "<<base_face.points.size()<<"): ";
                    }
                    for(int i = 0; i < base_face.points.size(); ++i) {
                        int vnum = read16(in)>>(3);
                        //cout<<vnum<<" ";
                        assert(vnum < points.size());
                        base_face.points[i] = points[vnum];
                        base_face.points[i].u = frac2float(read16(in));
                        base_face.points[i].v = frac2float(read16(in));
                    }
                    //cout<<endl;
                    faces.push_back(base_face);
#ifdef STAND_ALONE_MODEL
                    cout<<"Define texture-mapped face ("<<base_face.points.size()<<" vertices with U,V pairs), tex num: "<<base_face.texture_num<<endl;
#endif
                    break;
                case 0x00B4:
                    base_face.points.resize(0);
                    {
                        int vcount = read16(in);
                        base_face.points.resize(vcount);
                        //cout<<"Define face vertices ("<<vcount<<", "<<base_face.points.size()<<"): ";
                    }
#ifdef STAND_ALONE_MODEL
                    cout<<"Define texture-mapped face ("<<base_face.points.size()<<" vertices with U,V pairs)"<<endl;
#endif
                    for(int i = 0; i < base_face.points.size(); ++i) {
                        int vnum = read16(in)>>(3);
                        //cout<<vnum<<" ";
                        assert(vnum < points.size());
                        base_face.points[i] = points[vnum];
                        base_face.points[i].u = frac2float(read16(in));
                        base_face.points[i].v = frac2float(read16(in));
#ifdef STAND_ALONE_MODEL
                        cout<<"\tvert ("<<points[vnum].x<<", "<<points[vnum].y<<", "<<points[vnum].z<<"), UV: ("<<base_face.points[i].u<<", "<<base_face.points[i].v<<")"<<endl;
#endif
                    }
                    //cout<<endl;
                    faces.push_back(base_face);
                    break;
                case 0x00BC:
                    base_face.c = read16(in);
                    base_face.shade = read16(in);
#ifdef STAND_ALONE_MODEL
                    cout<<"Define face shade color: "<<base_face.c<<", shade: "<<base_face.shade<<endl; 
#endif
                    break;
                case 0x00BE:
                    unk16 = read16(in);
#ifdef STAND_ALONE_MODEL
                    cout<<hex<<"??? seems to define 2 shades ("<<unk16;
#endif
                    unk16 = read16(in);
#ifdef STAND_ALONE_MODEL
                    cout<<" and "<<unk16<<")"<<dec<<endl;
#endif
                    break;
                case 0x00CE:
                    base_face.points.resize(0);
                    {
                        int vcount = read16(in);
                        base_face.points.resize(vcount);
                        //cout<<"Define face vertices ("<<vcount<<", "<<base_face.points.size()<<"): ";
                    }
                    for(int i = 0; i < base_face.points.size(); ++i) {
                        int vnum = read16(in)>>(3);
                        //cout<<vnum<<" ";
                        assert(vnum < points.size());
                        base_face.points[i] = points[vnum];
                        base_face.points[i].u = frac2float(read16(in));
                        base_face.points[i].v = frac2float(read16(in));
                    }
                    //cout<<endl;
                    faces.push_back(base_face);
#ifdef STAND_ALONE_MODEL
                    cout<<"Define texture-mapped face (B4 clone) ("<<base_face.points.size()<<" vertices with U,V pairs)"<<endl;
#endif
                    break;
                /*
                case 0x00D2: cout<<"??? shorthand face definition"; dat_size = 6; break;
                */
                case 0x00D4: {
                        uint16_t nverts = read16(in);
                        uint16_t color = read16(in);
                        for (int i=0;i<nverts;++i) {
                            uint16_t vertnum = read16(in)>>(8);
                            uint8_t shade = read8(in);
                            if(vertnum >= points.size()) {
                                points.resize(vertnum+1);
#ifdef STAND_ALONE_MODEL
                                cout<<"Expanded point list size to "<<points.size()<<" because the color list referenced a vertex that high."<<endl;
#endif
                            }
                            points[vertnum].c = color;
                            points[vertnum].shade = shade;
                        }
                        if(nverts % 2 == 1) unk16 = read8(in); //Maintains alignment, I guess?
#ifdef STAND_ALONE_MODEL
                        cout<<"Define vertex colors (?) for "<<nverts<<" vertices"<<endl; 
#endif
                    }
                    break;
                case 0x00D6:
                    base_face.goraud = true;
#ifdef STAND_ALONE_MODEL
                    cout<<"Define gouraud shading"<<endl;
#endif
                    break;
                default: 
                    cout<<"UNKNOWN 3D ELEMENT ("<<node_type<<")"; dat_size = 20; retval = 2;
            }
            if(retval) {
                cout<<": ";
                for(int i=0;i<dat_size;++i) cout<<hex<<int(read8(in))<<" ";
                cout<<endl<<dec;
                exit = true;
            }
        }
    }
    return retval;
}

bool uw_model::load(const std::string& uw_exe, const std::string& pal_filename, int model_number) {
    if(model_number < 0 || model_number >= 64) {
        std::cerr<<"Tried to load invalid model #"<<model_number<<std::endl;
        return false;
    }

    if(!pal.load(pal_filename, 0)) {
        std::cerr<<"Couldn't open palette file at "<<pal_filename<<std::endl;
    }

    ifstream in;
    in.open(uw_exe.c_str());
    if(!in.is_open()) {
        std::cerr<<"Couldn't open file at "<<uw_exe<<std::endl;
        return false;
    }

    size_t start_offset = 0;
    size_t model_table_offset = 0;

    for(size_t i=0;i<6;++i) {
        if(check_offset(start_offsets[i], in)) {
            start_offset = start_offsets[i];
            model_table_offset = model_table_offsets[i];
        }
    }

    if(start_offset == 0 || model_table_offset == 0) {
        std::cerr<<"Given file \""<<uw_exe<<" doesn't look like a supported Underworld binary."<<endl;
        return false;
    }

    in.seekg(start_offset + model_number * 2);
    size_t model_offset = read16(in) + model_table_offset;
    in.seekg(model_offset);

    uint32_t junk32 = read32(in); //Unknown meaning for the first 4 bytes
    extent_x = fix2float(read16(in));
    extent_y = fix2float(read16(in));
    extent_z = fix2float(read16(in));

    if(extent_x == 0.0 || extent_y == 0.0 || extent_z == 0.0) {
#ifdef STAND_ALONE_MODEL
        std::cerr<<"Model "<<model_number<<" has extents ("<<extent_x<<", "<<extent_y<<", "<<extent_z<<"), which look invalid."<<std::endl;
#endif
        //return false;
    }

    int ret = process_nodes(in);

    //TODO: Traverse model data nodes and load data into my vertex+face lists

    in.close();
    if(!ret) return true;
    else return false;
}

std::vector<float> uw_model::get_verts(output_type type) {
    std::vector<float> ret;
    switch(type) {
        case geometry:
            for(int f = 0; f < faces.size(); f++) {
                for(int p = 0; p < faces[f].points.size() - 2; p++) { //Emulate GL_TRIANGLE_FAN traversal
                    //ret.push_back(faces[f].points[0].x - cent_x);   ret.push_back(faces[f].points[0].z - cent_z);   ret.push_back(faces[f].points[0].y - cent_y);
                    //ret.push_back(faces[f].points[p+1].x - cent_x); ret.push_back(faces[f].points[p+1].z - cent_z); ret.push_back(faces[f].points[p+1].y - cent_y);
                    //ret.push_back(faces[f].points[p+2].x - cent_x); ret.push_back(faces[f].points[p+2].z - cent_z); ret.push_back(faces[f].points[p+2].y - cent_y);

                    ret.push_back(faces[f].points[0].x );   ret.push_back(faces[f].points[0].z );   ret.push_back(faces[f].points[0].y );
                    ret.push_back(faces[f].points[p+1].x ); ret.push_back(faces[f].points[p+1].z ); ret.push_back(faces[f].points[p+1].y );
                    ret.push_back(faces[f].points[p+2].x ); ret.push_back(faces[f].points[p+2].z ); ret.push_back(faces[f].points[p+2].y );
                }
            }
            break;
        default: //Not implemented, return empty
            return ret;
    }
    return ret;
}

#ifdef STAND_ALONE_MODEL
int main(int argc, char *argv[]) {
    uw_model m;
    if(argc == 3) {
        if(!m.load(std::string(argv[1]), std::string(argv[2]), 1)) {
            std::cerr<<"Problem loading model #1 from "<<argv[1]<<" using palette "<<argv[2]<<std::endl;
            return 1;
        }
    }
    else if(argc == 4) {
        if(!m.load(std::string(argv[1]), std::string(argv[2]), std::stoi(argv[3]))) {
            std::cerr<<"Problem loading model #"<<argv[3]<<" from "<<argv[1]<<" using palette "<<argv[2]<<std::endl;
            return 1;
        }
    }
    else {
        std::cerr<<"Give the path of the Underworld Executable, a palette, and an optional model number (defaults to 1)"<<std::endl;
    }

    auto v = m.get_verts(uw_model::geometry);
    std::cout<<std::dec<<"Found "<<v.size()/3<<" points ("<<v.size()/9<<" triangular faces)"<<std::endl;
    for(int i = 0; i < v.size(); i+=3) {
        std::cout<<"("<<v[i]<<", "<<v[i+1]<<", "<<v[i+2];
        if(((i+3)%9) != 0) {
            std::cout<<") -> ";
        }
        else {
            std::cout<<")"<<std::endl;
        }
    }
    return 0;   
}
#endif