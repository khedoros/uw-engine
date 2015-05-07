#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<list>
#include<cstdint>
#include<cassert>
#include "util.h"

using namespace std;

void place_vert(vert v, vector<vert>& l) {
    if(v.n >= l.size()) l.resize(v.n+1);
    l[v.n] = v;
}

void translate_nodes(uint32_t model_index, uint32_t base, vector<uint8_t>& d) {
    uint32_t off = 0;
    uint32_t h_unk = get32(d,base+off); off+=4;
    float h_ex = fix2float(get16(d,base+off)); off+=2;
    float h_ey = fix2float(get16(d,base+off)); off+=2;
    float h_ez = fix2float(get16(d,base+off)); off+=2;
    vector<string> model_names = 
//     0                         1                       2                  3               4                    5                  6               7               8         9        a            b         c           d              e                  f
    {" - ",                  "door_frame",            "bridge",           "bench",      "Lotus Turbo Esprit", "small_boulder", "medium_boulder", "large_boulder", "arrow", "beam",  "pillar",     "shrine", " - ",   "painting [uw2]", " - ",             " - ",
     "texmap (8-way lever)", "texmap (8-way switch)", "texmap (writing)", "gravestone", "texmap",             " - ",           "texmap",         "moongate",      "table", "chest", "nightstand", "barrel", "chair", "bed [uw2]",      "blackrock [uw2]", "shelf [uw2]"};

    if(model_index < model_names.size() && model_names[model_index] != " - ") {//h_ex != 0.0 || h_ey != 0.0 || h_ez != 0.0) {
        cout<<"#Model "<<model_index<<" (\""<<model_names[model_index]<<"\") starting at offset "<<hex<<base<<dec<<endl;
        cout<<"#Header: unk: "<<hex<<h_unk<<" extents: ("<<dec<<h_ex<<", "<<h_ey<<", "<<h_ez<<")"<<endl;
    }
    else {
        cout<<"No valid model at index "<<model_index<<endl<<endl;
        return;
    }
    cout.precision(5); cout<<std::fixed;
    //While there are non-end node IDs available
    uint16_t node_id = 0;
    uint32_t v_count = 0;
    vector<vert> v_list(0);
    vector<vector<uint16_t>> f_list(0);
    list<uint32_t> offset_list;
    while(get16(d, base+off) != 0 || offset_list.size() != 0) {
        node_id = get16(d, base+off);
        if(node_id == 0) {
            off = offset_list.front();
            offset_list.pop_front();
            node_id = get16(d,base+off);
        }
        off+=2;
        float x,y,z, nx, ny, nz, dist, u, v;
        uint16_t v_num, v_base, unk16, numvert, skipbytes, texnum;
        vector<uint16_t> face_temp(0);
        switch(node_id) {
            //case 0: //skipping for now, because I feel like it!
            //     break;
            case 0x06: //sort mode, arbitrary heading
              //Fall through, since these 4 can be handled by the same code
            case 0x0c: //sort mode, ZY plane
            case 0x0e: //sort mode, XY plane
            case 0x10: //sort mode, XZ plane
            {
                cout<<hex<<"#sort mode tree node"<<endl;
                int size = 12;
                if(node_id == 6) size = 16;
                for(int i=0;i<size-4;i+=2)
                    cout<<fix2float(get16(d,base+off+i))<<" ";
                cout<<endl<<"\t"<<get16(d,base+off+size-4)<<"("<<get16(d,base+get16(d,base+off+size-4) + off + size - 2)<<" node)"<<endl<<"\t"<<get16(d,base+off+size-2)<<"("<<get16(d,base+get16(d,base+off+size-2) + off + size)<<" node)"<<endl;
                uint16_t l_off = get16(d,base+off+size-4) + off + size - 2;
                uint16_t r_off = get16(d,base+off+size-2) + off + size;
                offset_list.push_back(l_off);
                offset_list.push_back(r_off);
                off+=size;
            }
                break;
            case 0x14: //define color
                v_num = get16(d,base+off)/8; off+=2;
                //color value, unknown format
                unk16 = get16(d,base+off); off+=2;
                v_base = get16(d,base+off)/8; off+=2;
                cout<<dec<<"#Vert1: "<<v_num<<" Vert2: "<<v_base<<" Color value: "<<hex<<unk16<<endl;
                break;
            case 0x40: //start face definition?
                //just skip it, because it goes right to a face
                //cout<<"TAG 0x40 data: "<<get16(d,base+off)<<" "<<get16(d,base+off+1)<<" "<<get16(d,base+off+2)<<" "<<get16(d,base+off+3)<<endl;
                break;
            case 0x58: //define face plane, arbitrary heading
                skipbytes = get16(d,base+off); off+=2;
                nx = fix2float(get16(d,base+off)); off+=2;
                dist = fix2float(get16(d,base+off)); off+=2;
                ny = fix2float(get16(d,base+off)); off+=2;
                dist = fix2float(get16(d,base+off)); off+=2;
                nz = fix2float(get16(d,base+off)); off+=2;
                dist = fix2float(get16(d,base+off)); off+=2;
                cout<<dec<<"#Normal arbitrary skip: "<<skipbytes<<" normal: ("<<nx<<", "<<ny<<", "<<nz<<") distance: "<<dist<<endl;
                break;
            case 0x5e: //define face plane z/y
                nx = 0;
                skipbytes = get16(d,base+off); off+=2;
                nz = fix2float(get16(d,base+off)); off+=2;
                dist = fix2float(get16(d,base+off)); off+=2;
                ny = fix2float(get16(d,base+off)); off+=2;
                dist = fix2float(get16(d,base+off)); off+=2;
                cout<<dec<<"#Normal ZY skip: "<<skipbytes<<" normal: ("<<nx<<", "<<ny<<", "<<nz<<") distance: "<<dist<<endl;
                break;
            case 0x60: //define face plane x/y
                nz = 0;
                skipbytes = get16(d,base+off); off+=2;
                nx = fix2float(get16(d,base+off)); off+=2;
                dist = fix2float(get16(d,base+off)); off+=2;
                ny = fix2float(get16(d,base+off)); off+=2;
                dist = fix2float(get16(d,base+off)); off+=2;
                cout<<dec<<"#Normal XY skip: "<<skipbytes<<" normal: ("<<nx<<", "<<ny<<", "<<nz<<") distance: "<<dist<<endl;
                break;
            case 0x62: //define face plane x/z
                ny = 0;
                skipbytes = get16(d,base+off); off+=2;
                nx = fix2float(get16(d,base+off)); off+=2;
                dist = fix2float(get16(d,base+off)); off+=2;
                nz = fix2float(get16(d,base+off)); off+=2;
                dist = fix2float(get16(d,base+off)); off+=2;
                cout<<dec<<"#Normal XZ skip: "<<skipbytes<<" normal: ("<<nx<<", "<<ny<<", "<<nz<<") distance: "<<dist<<endl;
                break;
            case 0x64: //define face plane x
                skipbytes = get16(d,base+off); off+=2;
                nx = fix2float(get16(d,base+off)); off+=2;
                ny = nz = 0;
                dist = fix2float(get16(d,base+off)); off+=2;
                cout<<dec<<"#Normal X skip: "<<skipbytes<<" normal: ("<<nx<<", 0, 0) distance: "<<dist<<endl;
                break;
            case 0x66: //define face plane z
                skipbytes = get16(d,base+off); off+=2;
                nz = fix2float(get16(d,base+off)); off+=2;
                nx = ny = 0;
                dist = fix2float(get16(d,base+off)); off+=2;
                cout<<dec<<"#Normal Z skip: "<<skipbytes<<" normal: (0, 0, "<<nz<<") distance: "<<dist<<endl;
                break;
            case 0x68: //define face plane y
                skipbytes = get16(d,base+off); off+=2;
                ny = fix2float(get16(d,base+off)); off+=2;
                nx = nz = 0;
                dist = fix2float(get16(d,base+off)); off+=2;
                cout<<dec<<"#Normal Y skip: "<<skipbytes<<" normal: (0, "<<ny<<", 0) distance: "<<dist<<endl;
                break;
            case 0x78: //define model center
                v_num = get16(d,base+off)/8; off+=2;
                x = fix2float(get16(d,base+off)); off+=2;
                y = fix2float(get16(d,base+off)); off+=2;
                z = fix2float(get16(d,base+off)); off+=2;
                //skip an unknown 16-bit value
                off+=2;
                v_count++;
                cout<<dec<<"#ORIGIN vert num "<<v_num<<" ("<<x<<", "<<y<<", "<<z<<")"<<endl;
                break;
            case 0x7a: //define initial vertex
                x = fix2float(get16(d,base+off)); off+=2;
                y = fix2float(get16(d,base+off)); off+=2;
                z = fix2float(get16(d,base+off)); off+=2;
                v_num = get16(d,base+off)/8; off+=2;
                v_count++;
                place_vert(vert(x,y,z,v_num), v_list);
                cout<<dec<<"#vert num "<<v_num<<"    v "<<x<<" "<<y<<" "<<z<<endl;
                break;
            case 0x7e: //define face vertices
                numvert = get16(d,base+off); off+=2;
                face_temp.clear();
                face_temp.resize(numvert);
                //cout<<"f ";
                for(int i=0;i<numvert;++i) {
                    v_num = get16(d,base+off)/8+1; off+=2;
                    face_temp[i]=v_num;
                    //cout<<dec<<v_num<<" ";
                }
                f_list.emplace_back(face_temp);
                cout<<"Face count: "<<f_list.size()<<" Vertex count in this face: "<<f_list.back().size()<<endl;
                for(auto i:face_temp) cout<<"\t"<<i<<endl;
                //cout<<endl;
                break;
            case 0x82: //vertex list definition
                numvert = get16(d,base+off); off+=2;
                v_base = get16(d,base+off)/8; off+=2;
                for(int i=0;i<numvert;++i) {
                    x = fix2float(get16(d,base+off)); off+=2;
                    y = fix2float(get16(d,base+off)); off+=2;
                    z = fix2float(get16(d,base+off)); off+=2;
                    v_num = v_base+i;
                    v_count++;
                    place_vert(vert(x,y,z,v_num), v_list);
                    cout<<dec<<"#vert num "<<v_num<<"    v "<<x<<" "<<y<<" "<<z<<endl;
                }
                break;
            case 0x86: //vertex offset x
                v_base = get16(d,base+off)/8; off+=2;
                x = v_list[v_base].x + fix2float(get16(d,base+off)); off+=2;
                y = v_list[v_base].y;
                z = v_list[v_base].z;
                v_num = get16(d,base+off)/8; off+=2;
                place_vert(vert(x,y,z,v_num), v_list);
                cout<<dec<<"#vert num "<<v_num<<"    v "<<x<<" "<<y<<" "<<z<<endl;
                break;
            case 0x88: //vertex offset z
                v_base = get16(d,base+off)/8; off+=2;
                x = v_list[v_base].x;
                y = v_list[v_base].y;
                z = v_list[v_base].z + fix2float(get16(d,base+off)); off+=2;
                v_num = get16(d,base+off)/8; off+=2;
                place_vert(vert(x,y,z,v_num), v_list);
                cout<<dec<<"#vert num "<<v_num<<"    v "<<x<<" "<<y<<" "<<z<<endl;
                break;
            case 0x8a: //vertex offset y
                v_base = get16(d,base+off)/8; off+=2;
                x = v_list[v_base].x;
                y = v_list[v_base].y + fix2float(get16(d,base+off)); off+=2;
                z = v_list[v_base].z;
                v_num = get16(d,base+off)/8; off+=2;
                place_vert(vert(x,y,z,v_num), v_list);
                cout<<dec<<"#vert num "<<v_num<<"    v "<<x<<" "<<y<<" "<<z<<endl;
                break;
            case 0x8c: //vertex variable height
                v_base = get16(d,base+off)/8; off+=2;
                //skip an unknown 16-bit value
                off+=2;
                x = v_list[v_base].x;
                y = v_list[v_base].y;
                z = 16.0;
                v_num = get16(d,base+off)/8; off+=2;
                place_vert(vert(x,y,z,v_num), v_list);
                cout<<dec<<"#vert num "<<v_num<<"    v "<<x<<" "<<y<<" "<<z<<" (variable)"<<endl;
                break;
            case 0x90: //vertex offset x+z
                x = fix2float(get16(d,base+off)); off+=2;
                z = fix2float(get16(d,base+off)); off+=2;
                v_base = get16(d,base+off)/8; off+=2;
                x = v_list[v_base].x + x;
                y = v_list[v_base].y;
                z = v_list[v_base].z + z;
                v_num = get16(d,base+off)/8; off+=2;
                place_vert(vert(x,y,z,v_num), v_list);
                cout<<dec<<"#vert num "<<v_num<<"    v "<<x<<" "<<y<<" "<<z<<endl;
                break;
            case 0x92: //vertex offset x+y
                x = fix2float(get16(d,base+off)); off+=2;
                y = fix2float(get16(d,base+off)); off+=2;
                v_base = get16(d,base+off)/8; off+=2;
                x = v_list[v_base].x + x;
                y = v_list[v_base].y + y;
                z = v_list[v_base].z;
                v_num = get16(d,base+off)/8; off+=2;
                place_vert(vert(x,y,z,v_num), v_list);
                cout<<dec<<"#vert num "<<v_num<<"    v "<<x<<" "<<y<<" "<<z<<endl;
                break;
            case 0x94: //vertex offset y+z
                y = fix2float(get16(d,base+off)); off+=2;
                z = fix2float(get16(d,base+off)); off+=2;
                v_base = get16(d,base+off)/8; off+=2;
                x = v_list[v_base].x;
                y = v_list[v_base].y + y;
                z = v_list[v_base].z + z;
                v_num = get16(d,base+off)/8; off+=2;
                place_vert(vert(x,y,z,v_num), v_list);
                cout<<dec<<"#vert num "<<v_num<<"    v "<<x<<" "<<y<<" "<<z<<endl;
                break;
            case 0xa0: //shorthand texture-mapped face def
                unk16 = fix2float(get16(d,base+off)); off+=2;
                face_temp.clear();
                face_temp.resize(4);
                face_temp[0] = d[base+off]+1; off++;
                face_temp[1] = d[base+off]+1; off++;
                face_temp[2] = d[base+off]+1; off++;
                face_temp[3] = d[base+off]+1; off++;
                cout<<"#a0 shorthand face: "<<face_temp[0]<<" "<<face_temp[1]<<" "<<face_temp[2]<<" "<<face_temp[3]<<endl;
                f_list.emplace_back(face_temp);
                break;
            case 0xa8: //tex-mapped face
                //texture number
                texnum = get16(d,base+off); off+=2;
                numvert = get16(d,base+off); off+=2;
                cout<<"#a8 tex-mapped face, texnum: "<<texnum<<", numvert: "<<numvert<<endl;
                face_temp.clear();
                face_temp.resize(numvert);
                for(int i=0;i<numvert;++i) {
                    face_temp[i] = get16(d,base+off)/8+1; off+=2;
                    u = frac2float(get16(d,base+off)); off+=2;
                    v = frac2float(get16(d,base+off)); off+=2;
                    cout<<"#\tVert "<<face_temp[i]<<" u,v: ("<<u<<", "<<v<<")"<<endl;
                }
                f_list.emplace_back(face_temp);
                break;
            case 0xb4: //tex-mapped face, similar to a8
                numvert = get16(d,base+off); off+=2;
                cout<<"#b4 a8-like tex-mapped face, numvert: "<<numvert<<endl;
                face_temp.clear();
                face_temp.resize(numvert);
                for(int i=0;i<numvert;++i) {
                    face_temp[i] = get16(d,base+off)/8+1; off+=2;
                    u = frac2float(get16(d,base+off)); off+=2;
                    v = frac2float(get16(d,base+off)); off+=2;
                    cout<<"#\tVert "<<face_temp[i]<<" u,v: ("<<u<<", "<<v<<")"<<endl;
                }
                f_list.emplace_back(face_temp);
                break;
            case 0xbc: //face shade
                cout<<"#face shade"<<endl; off+=4;
                break;
            case 0xbe: //two shades
                cout<<"#define two shades?"<<endl; off+=4;
                break;
            case 0xce: //tex-mapped face, similar to a8
                numvert = get16(d,base+off); off+=2;
                cout<<"#ce a8-like tex-mapped face, numvert: "<<numvert<<endl;
                face_temp.clear();
                face_temp.resize(numvert);
                for(int i=0;i<numvert;++i) {
                    face_temp[i] = get16(d,base+off)/8+1; off+=2;
                    u = frac2float(get16(d,base+off)); off+=2;
                    v = frac2float(get16(d,base+off)); off+=2;
                    cout<<"#\tVert "<<face_temp[i]<<" u,v: ("<<u<<", "<<v<<")"<<endl;
                }
                f_list.emplace_back(face_temp);
                break;
            case 0xd4: //define vertex shading values
                numvert = get16(d,base+off); off+=2;
                //base color value
                unk16 = get16(d,base+off); off+=2;
                for(int i=0;i<numvert;++i) {
                    v_num = get16(d,base+off)/8; off+=2;
                    v_base = d[base+off]; off++;
                    //cout<<dec<<"#vert num "<<v_num<<" base goraud color: "<<unk16<<" dark value: "<<v_base<<endl;
                }
                //maintain 16-bit alignment
                if(numvert % 2 != 0) off++;
                break;
            case 0xd6: //activate goraud
                cout<<"#activate goraud"<<endl;
                break;
            default:
                cout<<hex<<"Unknown node ID "<<node_id<<endl<<endl;
                //return;
                goto exit_while;
        }
    }
    exit_while:
    if(v_list.size() > 0 && f_list.size() > 0) {
        ofstream out(model_names[model_index]+".obj", ios::out|ios::trunc);
        if(!out.is_open()) {
            cerr<<"Couldn't open file \""<<model_names[model_index]+".obj"<<"\". Aborting the export."<<endl;
            return;
        }
        for(auto v: v_list) {
            cout<<dec<<"v "<<v.x<<" "<<v.y<<" "<<v.z<<endl;
            out<<dec<<"v "<<v.x<<" "<<v.y<<" "<<v.z<<endl;
        }
        for(auto fl: f_list) {
            cout<<"f";
            out<<"f";
            for(auto f:fl) {
                cout<<" "<<f;
                out<<" "<<f;
            }
            cout<<endl;
            out<<endl;
        }
        cout<<endl;
        out<<endl;
    }
}

void translate_models(vector<uint32_t>& o, vector<uint8_t>& d) {
    for(int i=0;i<o.size();++i) {
        translate_nodes(i,o[i],d);
//    for(auto model_offset:o) {
//        translate_nodes(model_offset,d);
    }
}

bool check_offset(uint32_t offset, vector<uint8_t>& in) {
    const uint8_t first_bytes[] = {0xb6, 0x4a, 0x06, 0x40};
    for(int i=0;i<4;++i)
        if(in[offset+i] != first_bytes[i]) {
            return false;
        }
    return true;
}

int main(int argc, char *argv[]) {
    if(argc != 2) return 1;
    ifstream in(argv[1]);
    vector<uint8_t> file;
    in.seekg(0,ios::end);
    size_t fs = in.tellg();
    in.seekg(0,ios::beg);
    cout<<"File: "<<argv[1]<<" size: "<<fs<<endl;
    file.resize(fs);
    in.read((char *)(&file[0]), fs);
    in.close();

    size_t oto = 0; //offset of model offset table
    size_t mbase = 0; //base offset of model data
    const uint32_t start_offsets[] = {0x0004e910, 0x0004ccd0, 0x0004e370, 0x0004ec70};
    const uint32_t model_table_offsets[] = {0x0004e99e, 0x0004cd5e, 0x0004e3fe, 0x0004ecfe};
    for(size_t i=0;i<4;++i) {
        if(check_offset(start_offsets[i], file)) {
            oto = start_offsets[i];
            mbase = model_table_offsets[i];
        }
    }
    if(oto == 0 || mbase == 0) {
        cout<<"Couldn't find the model tables in the specified file."<<endl;
        return 1;
    }
    else cout<<"Found model offsets at "<<hex<<oto<<" and model data at "<<mbase<<"."<<endl;

    vector<uint32_t> offsets(64);
    for(int i=0;i<64;++i) {
        offsets[i] = file[oto+i*2] + ((file[oto+i*2+1])<<(8)) + mbase;
        //cout<<"Model # "<<dec<<i+1<<" at "<<hex<<offsets[i]+mbase<<"(from data "<<uint32_t(file[oto+i*2])<<" "<<uint32_t(file[oto+i*2+1])<<")"<<endl;
    }
    translate_models(offsets,file);
}
