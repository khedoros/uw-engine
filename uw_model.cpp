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

int uw_model::process_node(ifstream& in) {
    size_t offset = in.tellg();
    uint16_t node_type = read16(in);
    cout<<hex<<offset<<": type 0x"<<node_type<<dec<<" ";
    uint8_t dat_size = 0;
    int retval = 0;
    switch(node_type) {
        /*
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
        */
        case 0x007A: 
             base_pt.x = fix2float(read16(in));
             base_pt.y = fix2float(read16(in));
             base_pt.z = fix2float(read16(in));
             base_pt.vertno = read16(in)>>(3);
             base_pt.var_height = false;
             points.push_back(base_pt);
             std::cout<<"Define initial vertex ("<<base_pt.x<<", "<<base_pt.y<<", "<<base_pt.z<<") as num "<<base_pt.vertno<<std::endl;; 
             break;
        /*
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
        */
        default: cout<<"UNKNOWN 3D ELEMENT ("<<node_type<<")"; dat_size = 20; retval = 2;
    }
    if(retval) {
        cout<<": ";
        for(int i=0;i<dat_size;++i) cout<<hex<<int(read8(in))<<" ";
        cout<<endl<<dec;
    }
    return retval;
}

bool uw_model::load(const std::string& uw_exe, int model_number) {
    return true;
}

int main(int argc, char *argv[]) {
    uint32_t start_offset = 0;
    uint32_t model_table_offset = 0;
    ifstream in;
    if(argc == 2) {
        in.open(argv[1], ios::binary|ios::in);
        if(!in.is_open()) {
            cerr<<"Failed to open "<<argv[1]<<endl;
            return 1;
        }
    }
    else { cerr<<"Provide the path to uw.exe"<<endl; return 1;}

    uw_model m;
    for(size_t i=0;i<4;++i) {
        if(m.check_offset(m.start_offsets[i], in)) {
            start_offset = m.start_offsets[i];
            model_table_offset = m.model_table_offsets[i];
        }
    }
    if(start_offset == 0 || model_table_offset == 0) { cerr<<"Couldn't find expected data pattern. Is this the uw1 executable??"<<endl; return 1; }
    uint32_t model_offsets[64];


    in.seekg(start_offset, ios::beg);
    for(size_t i = 0; i<64; ++i) {
        in.seekg(start_offset + i * 2);
        model_offsets[i] = read16(in) + model_table_offset;
        cout<<"Model "<<hex<<i<<" at "<<model_offsets[i]<<dec;
        in.seekg(model_offsets[i],ios::beg);
        uint32_t unk = read32(in);
        float x_ext = read16(in);
        float y_ext = read16(in);
        float z_ext = read16(in);
        if(x_ext != 0.0 && y_ext != 0.0 && z_ext != 0.0) {
            x_ext /= 256.0; y_ext /= 256.0; z_ext /= 256.0;
            cout<<": Unknown: "<<hex<<unk<<dec<<" Xext: "<<x_ext<<" Yext: "<<y_ext<<" Zext: "<<z_ext<<" Next model around "<<hex<<model_offsets[i]+unk<<"?"<<dec<<endl;
            while(uint32_t(in.tellg()) - model_offsets[i] < 2048) {
                int ret = m.process_node(in);
                if(ret != 0) break;
            }
        }
        else cout<<": Skipped (seems invalid)"<<" Next model around "<<hex<<model_offsets[i]+unk<<"?"<<dec<<endl;
        in.clear();
    }

    return 0;   
}
