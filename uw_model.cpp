#include<iostream>
#include<fstream>
#include<string>
#include<cstdint>
#include<vector>
#include "util.h"
#include "uw_model.h"

using namespace std;

const uint32_t uw_model::start_offsets[] = {0x0004e910, 0x0004ccd0, 0x0004e370, 0x0004ec70};
const uint32_t uw_model::model_table_offsets[] = {0x0004e99e, 0x0004cd5e, 0x0004e3fe, 0x0004ecfe};
uw_model::uw_model() {
}

bool uw_model::check_offset(uint32_t offset, ifstream& in) {
    const uint8_t first_bytes[] = {0xb6, 0x4a, 0x06, 0x40};
    uint32_t bookmark = in.tellg();
    in.seekg(offset, ios::beg);
    for(int i=0;i<4;++i)
        if(read8(in) != first_bytes[i]) {
            in.seekg(bookmark, ios::beg);
            return false;
        }
    in.seekg(bookmark, ios::beg);
    return true;
}

int uw_model::skip_data(ifstream& in) {
    size_t offset = in.tellg();
    cout<<hex<<offset<<dec<<": ";
    uint16_t node_type = read16(in);
    uint8_t dat_size = 0;
    int retval = 0;
    switch(node_type) {
        case 0x0000: cout<<"end node"<<endl; dat_size = 0; return 1;
        case 0x0006: cout<<"define sort node, arbitrary heading"; dat_size = 16; break;
        case 0x000C: cout<<"define sort node, ZY plane"; dat_size = 12; break;
        case 0x000E: cout<<"define sort node, XY plane"; dat_size = 12; break;
        case 0x0010: cout<<"define sort node, XZ plane"; dat_size = 12; break;
        case 0x0014: cout<<"??? colour definition"; dat_size = 6; break;
        case 0x002E: cout<<"???"; dat_size = 2; break;
        case 0x0040: cout<<"??? seems to do nothing but introduce a face definition"; dat_size = 0; break;
        case 0x0044: cout<<"??? this one too (seems to intro a face def)"; dat_size = 0; break;
        case 0x0058: cout<<"define face plane, arbitrary heading"; dat_size = 14; break;
        case 0x005E: cout<<"define face plane Z/Y"; dat_size = 10; break;
        case 0x0060: cout<<"define face plane X/Y"; dat_size = 10; break;
        case 0x0062: cout<<"define face plane X/Z"; dat_size = 10; break;
        case 0x0064: cout<<"define face plane X"; dat_size = 6; break;
        case 0x0066: cout<<"define face plane Z"; dat_size = 6; break;
        case 0x0068: cout<<"define face plane Y"; dat_size = 6; break;
        case 0x0078: cout<<"define model center"; dat_size = 0x0a; break;
        case 0x007A: cout<<"define initial vertex"; dat_size = 8; break;
        case 0x007E: cout<<"define face vertices"; {uint16_t vcount = read16(in); dat_size = vcount*2;} break;
        case 0x0082: cout<<"define initial vertices"; {uint16_t vcount = read16(in); dat_size = vcount * 6 + 2;} break;
        case 0x0086: cout<<"define vertex offset X"; dat_size = 6; break;
        case 0x0088: cout<<"define vertex offset Z"; dat_size = 6; break;
        case 0x008A: cout<<"define vertex offset Y"; dat_size = 6; break;
        case 0x008C: cout<<"define vertex variable height"; dat_size = 6; break;
        case 0x0090: cout<<"define vertex offset X,Z"; dat_size = 8; break;
        case 0x0092: cout<<"define vertex offset X,Y"; dat_size = 8; break;
        case 0x0094: cout<<"define vertex offset Y,Z"; dat_size = 8; break;
        case 0x00A0: cout<<"??? shorthand face definition"; dat_size = 6; break;
        case 0x00A8: cout<<"define texture-mapped face"; read16(in); {uint16_t vcount = read16(in); dat_size = vcount * 6;} break;
        case 0x00B4: cout<<"define face vertices with u,v information"; {uint16_t vcount = read16(in); dat_size = vcount * 6;} break;
        case 0x00BC: cout<<"define face shade"; dat_size = 4; break;
        case 0x00BE: cout<<"??? seems to define 2 shades"; dat_size = 4; break;
        case 0x00CE: cout<<"??? yet another texture-mapped face"; {uint16_t vcount = read16(in); dat_size = vcount * 6;} break;
        case 0x00D2: cout<<"??? shorthand face definition"; dat_size = 6; break;
        case 0x00D4: cout<<"define dark vertex face (?)"; {uint16_t vcount = read16(in); dat_size = vcount * 3 + 2;} if(dat_size%2==1) dat_size++; break;
        case 0x00D6: cout<<"define gouraud shading"; dat_size = 0; break;
        default: cout<<"UNKNOWN 3D ELEMENT ("<<node_type<<")"; dat_size = 20; retval = 2;
    }
    cout<<": ";
    for(int i=0;i<dat_size;++i) cout<<hex<<int(read8(in))<<" ";
    cout<<endl<<dec;
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

    for(size_t i=0;i<4;++i) {
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
        return false;
    }

    //TODO: Traverse model data nodes and load data into my vertex+face lists

    in.close();
    return true;
}

int main(int argc, char *argv[]) {
    uw_model m;


    return 0;   
}
