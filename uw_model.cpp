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
            cout<<hex<<offset<<": type 0x"<<node_type<<dec<<" ";
            uint8_t dat_size = 0;
            uint16_t unk16;
            switch(node_type) {
                case 0x0000:
                    cout<<"end node (";
                    if(offsets.size() > 0) {
                        cout<<"end of section)"<<endl;
                    }
                    else {
                        cout<<"end of model)"<<endl;
                    }
                    exit = true;
                    break;
                /*
                case 0x0006: cout<<"define sort node, arbitrary heading"; dat_size = 16; break;
                case 0x000C: cout<<"define sort node, ZY plane"; dat_size = 12; break;
                case 0x000E: cout<<"define sort node, XY plane"; dat_size = 12; break;
                case 0x0010: cout<<"define sort node, XZ plane"; dat_size = 12; break;
                */
                case 0x0014: 
                    cout<<"??? color definition (Skip, because not enough info) "<<(read16(in)>>(3))<<"\t"<<int(read8(in))<<"\t"<<int(read8(in))<<"\t"<<(read16(in)>>(3))<<endl;
                    break;
                /*
                case 0x002E: cout<<"???"; dat_size = 2; break;
                case 0x0040: cout<<"??? seems to do nothing but introduce a face definition"; dat_size = 0; break;
                case 0x0044: cout<<"??? this one too (seems to intro a face def)"; dat_size = 0; break;
                */
                case 0x0058:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = fix2float(read16(in));
                    base_face.dist_x = fix2float(read16(in));
                    base_face.ny = fix2float(read16(in));
                    base_face.dist_y = fix2float(read16(in));
                    base_face.nz = fix2float(read16(in));
                    base_face.dist_z = fix2float(read16(in));
                    cout<<"Define face plane, arbitrary heading, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
                    break;
                case 0x005E:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = 0;
                    base_face.dist_x = 0;
                    base_face.nz = fix2float(read16(in));
                    base_face.dist_z = fix2float(read16(in));
                    base_face.ny = fix2float(read16(in));
                    base_face.dist_y = fix2float(read16(in));
                    cout<<"Define face plane, Z/Y, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
                    break;
                case 0x0060:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = fix2float(read16(in));
                    base_face.dist_x = fix2float(read16(in));
                    base_face.nz = 0;
                    base_face.dist_z = 0;
                    base_face.ny = fix2float(read16(in));
                    base_face.dist_y = fix2float(read16(in));
                    cout<<"Define face plane, X/Y, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
                    break;
                case 0x0062:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = fix2float(read16(in));
                    base_face.dist_x = fix2float(read16(in));
                    base_face.nz = fix2float(read16(in));
                    base_face.dist_z = fix2float(read16(in));
                    base_face.ny = 0;
                    base_face.dist_y = 0;
                    cout<<"Define face plane, X/Z, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
                    break;
                case 0x0064:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = fix2float(read16(in));
                    base_face.dist_x = fix2float(read16(in));
                    base_face.nz = 0;
                    base_face.dist_z = 0;
                    base_face.ny = 0;
                    base_face.dist_y = 0;
                    cout<<"Define face plane, X, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
                    break;
                case 0x0066:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = 0;
                    base_face.dist_x = 0;
                    base_face.nz = fix2float(read16(in));
                    base_face.dist_z = fix2float(read16(in));
                    base_face.ny = 0;
                    base_face.dist_y = 0;
                    cout<<"Define face plane, Z, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
                    break;
                case 0x0068:
                    unk16 = read16(in); //Bytes to skip when not visible
                    base_face.nx = 0;
                    base_face.dist_x = 0;
                    base_face.nz = 0;
                    base_face.dist_z = 0;
                    base_face.ny = fix2float(read16(in));
                    base_face.dist_y = fix2float(read16(in));
                    cout<<"Define face plane, Y, ("<<base_face.nx<<", "<<base_face.ny<<", "<<base_face.nz<<")"<<endl;
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
                    std::cout<<"Define model center ("<<cent_x<<", "<<cent_y<<", "<<cent_z<<") as num "<<cent_vert<<std::endl;
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
                    std::cout<<"Define initial vertex ("<<base_pt.x<<", "<<base_pt.y<<", "<<base_pt.z<<") as num "<<base_pt.vertno<<std::endl;; 
                    break;
                case 0x007E: 
                    base_face.points.resize(0);
                    base_face.point_attribs.resize(0);
                    {
                        int vcount = read16(in);
                        base_face.points.resize(vcount);
                        base_face.point_attribs.resize(vcount);
                        cout<<"Define face vertices ("<<vcount<<", "<<base_face.points.size()<<"): ";
                    }
                    for(int i = 0; i < base_face.points.size(); ++i) {
                        int vnum = read16(in)>>(3);
                        cout<<vnum<<" ";
                        assert(vnum < points.size());
                        base_face.points[i] = points[vnum];
                    }
                    cout<<endl;
                    faces.push_back(base_face);
                    break;
                /*
                case 0x0082: cout<<"define initial vertices"; {uint16_t vcount = read16(in); dat_size = vcount * 6 + 2;} break;
                */
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
                        cout<<"Define vertex offset X ("<<temp.x<<", "<<temp.y<<", "<<temp.z<<") as num "<<temp.vertno<<endl;
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
                        cout<<"Define vertex offset Z ("<<temp.x<<", "<<temp.y<<", "<<temp.z<<") as num "<<temp.vertno<<endl;
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
                        cout<<"Define vertex offset Y ("<<temp.x<<", "<<temp.y<<", "<<temp.z<<") as num "<<temp.vertno<<endl;
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
                        assert(points.size() > temp.vertno);
                        cout<<"Set vertex num "<<base_vert<<" to variable height, as num "<<temp.vertno<<endl;
                    }
                    break;
                /*
                case 0x0090: cout<<"define vertex offset X,Z"; dat_size = 8; break;
                case 0x0092: cout<<"define vertex offset X,Y"; dat_size = 8; break;
                case 0x0094: cout<<"define vertex offset Y,Z"; dat_size = 8; break;
                case 0x00A0: cout<<"??? shorthand face definition"; dat_size = 6; break;
                */
                case 0x00A8:
                    base_face.points.resize(0);
                    base_face.point_attribs.resize(0);
                    base_face.texture_num = read16(in);
                    {
                        int vcount = read16(in);
                        base_face.points.resize(vcount);
                        base_face.point_attribs.resize(vcount);
                        //cout<<"Define face vertices ("<<vcount<<", "<<base_face.points.size()<<"): ";
                    }
                    for(int i = 0; i < base_face.points.size(); ++i) {
                        int vnum = read16(in)>>(3);
                        //cout<<vnum<<" ";
                        assert(vnum < points.size());
                        base_face.points[i] = points[vnum];
                        base_face.point_attribs[i].u = frac2float(read16(in));
                        base_face.point_attribs[i].v = frac2float(read16(in));
                    }
                    //cout<<endl;
                    faces.push_back(base_face);
                    cout<<"Define texture-mapped face ("<<base_face.points.size()<<" vertices with U,V pairs), tex num: "<<base_face.texture_num<<endl;
                    break;
                case 0x00B4:
                    base_face.points.resize(0);
                    base_face.point_attribs.resize(0);
                    {
                        int vcount = read16(in);
                        base_face.points.resize(vcount);
                        base_face.point_attribs.resize(vcount);
                        //cout<<"Define face vertices ("<<vcount<<", "<<base_face.points.size()<<"): ";
                    }
                    for(int i = 0; i < base_face.points.size(); ++i) {
                        int vnum = read16(in)>>(3);
                        //cout<<vnum<<" ";
                        assert(vnum < points.size());
                        base_face.points[i] = points[vnum];
                        base_face.point_attribs[i].u = frac2float(read16(in));
                        base_face.point_attribs[i].v = frac2float(read16(in));
                    }
                    //cout<<endl;
                    faces.push_back(base_face);
                    cout<<"Define texture-mapped face ("<<base_face.points.size()<<" vertices with U,V pairs)"<<endl;
                    break;
                case 0x00BC:
                    base_face.c = read16(in);
                    base_face.shade = read16(in);
                    cout<<"Define face shade color: "<<base_face.c<<", shade: "<<base_face.shade<<endl; 
                    break;
                case 0x00BE:
                    unk16 = read16(in);
                    cout<<hex<<"??? seems to define 2 shades ("<<unk16;
                    unk16 = read16(in);
                    cout<<" and "<<unk16<<")"<<dec<<endl;
                    break;
                case 0x00CE:
                    base_face.points.resize(0);
                    base_face.point_attribs.resize(0);
                    {
                        int vcount = read16(in);
                        base_face.points.resize(vcount);
                        base_face.point_attribs.resize(vcount);
                        //cout<<"Define face vertices ("<<vcount<<", "<<base_face.points.size()<<"): ";
                    }
                    for(int i = 0; i < base_face.points.size(); ++i) {
                        int vnum = read16(in)>>(3);
                        //cout<<vnum<<" ";
                        assert(vnum < points.size());
                        base_face.points[i] = points[vnum];
                        base_face.point_attribs[i].u = frac2float(read16(in));
                        base_face.point_attribs[i].v = frac2float(read16(in));
                    }
                    //cout<<endl;
                    faces.push_back(base_face);
                    cout<<"Define texture-mapped face (B4 clone) ("<<base_face.points.size()<<" vertices with U,V pairs)"<<endl;
                    break;
                /*
                case 0x00D2: cout<<"??? shorthand face definition"; dat_size = 6; break;
                case 0x00D4: cout<<"define dark vertex face (?)"; {uint16_t vcount = read16(in); dat_size = vcount * 3 + 2;} if(dat_size%2==1) dat_size++; break;
                case 0x00D6: cout<<"define gouraud shading"; dat_size = 0; break;
                */
                default: cout<<"UNKNOWN 3D ELEMENT ("<<node_type<<")"; dat_size = 20; retval = 2;
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

bool uw_model::load(const std::string& uw_exe, int model_number) {
    if(model_number < 0 || model_number >= 64) {
        std::cerr<<"Tried to load invalid model #"<<model_number<<std::endl;
        return false;
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
        std::cerr<<"Model "<<model_number<<" has extents ("<<extent_x<<", "<<extent_y<<", "<<extent_z<<"), which look invalid."<<std::endl;
        //return false;
    }

    int ret = process_nodes(in);

    //TODO: Traverse model data nodes and load data into my vertex+face lists

    in.close();
    if(!ret) return true;
    else return false;
}

int main(int argc, char *argv[]) {
    uw_model m;
    if(argc == 2) {
        m.load(std::string(argv[1]), 1);
    }
    else if(argc == 3) {
        m.load(std::string(argv[1]), std::stoi(argv[2]));
    }
    else {
        std::cerr<<"Give the path of the Underworld Executable"<<std::endl;
    }

    return 0;   
}
