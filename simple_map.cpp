#include "simple_map.h"
#include<SFML/Graphics.hpp>
#include<assert.h>

using namespace std;

bool simple_map::load(string& filename) {
    ifstream in;
    in.open(filename.c_str(),ios::in|ios::binary);
    if(!in.is_open()) {
        cout<<"Failed to open file \""<<filename<<"\". Aborting."<<endl;
        return false;
    }
    file_ident = read64(in);
    //printf("First 8 bytes look like this: %016lx\n",file_ident);
    switch(file_ident) {
        case UW1DEMOTILE:
            //cout<<"UW1 Demo Tilemap"<<endl;
            if(!load_tile(in)) { cout<<"Failed to load tilemap. Aborting."<<endl; return false; }
            loaded_tile = true;
            if(!loaded_tex) {
                string ext = "TXM";
                if(filename[filename.size() - 1] > 90) { ext = "txm";}
                string tex_filename = filename.substr(0,filename.size() - 2) + ext;
                return load(tex_filename);
            }
            break;
        case UW1DEMOTEXMAP:
            //cout<<"UW1 Demo Texture Mappings"<<endl;
            if(!load_tex(in)) { cout<<"Failed to load texmap. Aborting."<<endl; return false; }
            loaded_tex = true;
            if(!loaded_tile) {
                string ext = "ST";
                if(filename[filename.size() - 1] > 90) { ext = "st";}
                string tile_filename = filename.substr(0,filename.size() - 3) + ext;
                return load(tile_filename);
            }
            break;
        case UW2MAP: cout<<"UW2 Retail Map. Not implemented yet. Aborting."<<endl; return false;
        case UW1MAP:
            //cout<<"UW1 Retail Map"<<endl;
            if(!load_uw1map(in)) { cout<<"Failed to load the map. Aborting."<<endl; return false; }
            break;
        case UW1DEMOANIMDAT:
            //cout<<"UW1 Demo Animation Overlay"<<endl;
            if(!load_anim(in)) { cout<<"Failed to load animation overlay data. Aborting."<<endl; return false; }
            loaded_anim = true;
            if(!loaded_tex) {
                string ext = "TXM";
                if(filename[filename.size() - 1] > 90) { ext = "txm";}
                string tex_filename = filename.substr(0,filename.size() - 3) + ext;
                if(!load(tex_filename)) { cout<<"Failed to load texmap. Aborting."<<endl; return false; }
            }
            if(!loaded_tile) {
                string ext = "ST";
                if(filename[filename.size() - 1] > 90) { ext = "st";}
                string tile_filename = filename.substr(0,filename.size() - 3) + ext;
                if(!load(tile_filename)) { cout<<"Failed to load tilemap. Aborting."<<endl; return false; }
            }
            break;
        default: cout<<"Unhandled/unknown file. Aborting."<<endl; return false;
    }
    return true;
}

simple_map::tile_type simple_map::get_tile_type(uint16_t dat1) {
    uint16_t tile = (dat1 & BITS_0_3);
    if(tile >= INVALID) return INVALID;
    else return simple_map::tile_type(tile);
}

uint8_t   simple_map::get_floor_height(uint16_t dat1) {
    return (dat1 & BITS_4_7)>>(4);
}

uint8_t   simple_map::get_floor_index(uint16_t dat1) {
    return (dat1 & BITS_10_13)>>(10);
}

bool      simple_map::is_magic_allowed(uint16_t dat1) {
    return ((dat1 & BIT_14) == 0);
}

bool      simple_map::is_door(uint16_t dat1) {
    return ((dat1 & BIT_15) != 0);
}

uint8_t   simple_map::get_wall_index(uint16_t dat2) {
    return (dat2 & BITS_0_5);
}

uint16_t  simple_map::get_obj_list_start(uint16_t dat2) {
    return (dat2 & BITS_6_15)>>(6);
}

bool simple_map::load_tile(std::ifstream& in, size_t offset /*= 0*/, size_t index /*= 0*/) {
    if(levels.size() == 0) levels.resize(index+1);
    //cout<<"Loading tilemap data for level "<<index+1<<" located at offset "<<offset<<endl;
    size_t bookmark = in.tellg();
    in.seekg(offset, ios::beg);
    for(size_t y_index = 0; y_index < 64; ++y_index)
        for(size_t x_index = 0; x_index < 64; ++x_index) {
            uint16_t dat1 = read16(in);
            uint16_t dat2 = read16(in);
            levels[index].d[x_index][y_index].type =          get_tile_type(dat1);
            levels[index].d[x_index][y_index].height =        get_floor_height(dat1);
            levels[index].d[x_index][y_index].floor_tex =     get_floor_index(dat1);
            levels[index].d[x_index][y_index].magic_allowed = is_magic_allowed(dat1);
            levels[index].d[x_index][y_index].is_door =       is_door(dat1);
            levels[index].d[x_index][y_index].wall_tex =      get_wall_index(dat2);
            levels[index].d[x_index][y_index].first_obj =     get_obj_list_start(dat2);
        }
    //cout<<"Trying to load object lists for level :"<<index+1<<", starting from offset "<<offset+0x4000<<endl;
    assert(sizeof(static_obj) == 8);
    assert(sizeof(mobile_obj) == 27);
    //cout<<"Size of static object: "<<sizeof(static_obj)<<" (expect 8)"<<endl;
    //cout<<"Size of mobile object: "<<sizeof(mobile_obj)<<" (expect 27)"<<endl;
    //if(sizeof(static_obj) != 8 || sizeof(mobile_obj) != 27) return false;
    for(size_t mobile_obj_index = 0; mobile_obj_index < 256; ++mobile_obj_index) {
       //cout<<"NPC read 27 bytes, offset "<<in.tellg()<<" (lev "<<index+1<<"), index "<<mobile_obj_index<<endl;
       in.read(reinterpret_cast<char *>(&(levels[index].npcs[mobile_obj_index])),sizeof(mobile_obj));
    }
    for(size_t static_obj_index = 0; static_obj_index < 768; ++static_obj_index) {
       //in>>&(levels[index].items[static_obj_index]);
       //cout<<"Item read 8 bytes, offset "<<in.tellg()<<" (lev "<<index+1<<"), index "<<static_obj_index<<endl;
       in.read(reinterpret_cast<char *>(&(levels[index].items[static_obj_index])),sizeof(static_obj));
    }
    in.seekg(bookmark, ios::beg);
    return true;
}

bool simple_map::load_tex(std::ifstream& in, size_t offset /*= 0*/, size_t index /*= 0*/) {
    if(levels.size() == 0) levels.resize(index+1);
    //cout<<"Loading Texture Mapping data for level "<<index+1<<" located at offset "<<offset<<endl;
    size_t bookmark = in.tellg();
    in.seekg(offset, ios::beg);
    for(int i=0;i<48;++i)
        levels[index].wall_tex_index[i] = read16(in);
    for(int i=0;i<10;++i)
        levels[index].floor_tex_index[i] = read16(in);
    levels[index].ceil_tex_index = levels[index].floor_tex_index[9];
    for(int i=0;i<6;++i)
        levels[index].door_tex_index[i] = read8(in);
    in.seekg(bookmark, ios::beg);
    return true;
}

bool simple_map::load_anim(std::ifstream& in, size_t offset /*= 0*/, size_t index /*= 0*/) {
    if(levels.size() == 0) levels.resize(index+1);
    //cout<<"Pretending to load animation overlay data for level "<<index+1<<" located at offset "<<offset<<endl;
    size_t bookmark = in.tellg();
    in.seekg(offset, ios::beg);
    for(int i=0;i<64;++i) {
        levels[index].anim[i].link1 = read16(in);
        levels[index].anim[i].unk2  = read16(in);
        levels[index].anim[i].tile_x  = read8(in);
        levels[index].anim[i].tile_y  = read8(in);
    }
    in.seekg(bookmark, ios::beg);
    return true;
}

bool simple_map::load_automap_info(std::ifstream& in, size_t offset /*= 0*/, size_t index /*= 0*/) {
    //cout<<"Pretending to load automap exploration data for level "<<index+1<<" located at offset "<<offset<<endl;
    size_t bookmark = in.tellg();
    in.seekg(offset, ios::beg);
    for(int i=1; i<=4096; ++i) {
        printf("%02x ", read8(in));
        if(i%64 == 0) printf("\n");
    }
    in.seekg(bookmark, ios::beg);
    return true;
}

bool simple_map::load_automap_notes(std::ifstream& in, size_t offset /*= 0*/, size_t index /*= 0*/) {
    //cout<<"Pretending to load automap note data for level "<<index+1<<" located at offset "<<offset<<endl;
    size_t bookmark = in.tellg();
    in.seekg(offset, ios::beg);
    for(int i=1; i<=4096; ++i) {
        printf("%02x ", read8(in));
        if(i%64 == 0) printf("\n");
    }
    in.seekg(bookmark, ios::beg);
    return true;
}

bool simple_map::load_uw1map(std::ifstream& in) {
    in.seekg(0,ios::beg);
    uint16_t block_count = read16(in);
    levels.resize(9);
    for(size_t index = 0;index < block_count; ++index) {
        size_t offset = read32(in);
        if(offset != 0) {
            //cout<<"Index: "<<index<<" Offset: "<<offset<<endl;
            switch(index/9) {
                case 0: if(!load_tile(in,offset,index%9)) return false; break;
                case 1: if(!load_anim(in,offset,index%9)) return false; break;
                case 2: if(!load_tex(in,offset,index%9)) return false; break;
                case 3: if(!load_automap_info(in, offset, index%9)) return false; break;
                case 4: if(!load_automap_notes(in, offset, index%9)) return false; break;
                default: break; //cout<<"Skipping data at index "<<index<<"."<<endl;
            }
        }
    }

    return true;
}

void simple_map::graphic_display() {
    std::vector<uint8_t> buffer;
    cout<<"Buffer size (old): "<<1536*768*4<<" (new): "<<1536*768*4+4076<<endl;
    size_t maxseen=0;
    buffer.resize(1536 * 768 * 4 /*+ 6144*/);
    for(int index=0;index<8;++index) {
        size_t extra_offset = 0;
        if(index>3)
            extra_offset = 4 * 1536 * 383;
        for(int y=63;y>=0;--y) {
            for(int x=0;x<64;++x) {
                simple_map::tile_type ty = levels[index].d[x][y].type;
                bool door = levels[index].d[x][y].is_door;
                if(door) cout<<"Map: "<<index+1<<" X: "<<x<<" Y: "<<y<<" Door!"<<endl;
                uint8_t height = levels[index].d[x][y].height;
                for(int fine_y=0;fine_y<6;++fine_y){
                    for(int fine_x=0;fine_x<6;++fine_x) {
                        size_t offset = 4 * (1536 * 6 * (63-y) + 1536 * fine_y + 6 * x + fine_x + 384 * index) + extra_offset;
                        if(offset>maxseen) {maxseen = offset;}//cout<<"Saw: "<<maxseen<<endl;}
                        uint8_t val = height*16;
                        if(ty==SOLID_TILE) {
                            val=255;
                        }
                        else if(ty==DIAG_SW && fine_y <= fine_x) {
                            val = 255;
                        }
                        else if(ty==DIAG_NE && fine_y >= fine_x) {
                            val = 255;
                        }
                        else if(ty==DIAG_SE && fine_y + fine_x <=3) {
                            val = 255;
                        }
                        else if(ty==DIAG_NW && fine_y + fine_x >=3) {
                            val = 255;
                        }
                        if(door) val = 255;
                        buffer[offset] = val;
                        buffer[offset+2] = val;
                        if(door) val = 0;
                        buffer[offset+1] = val;
                        buffer[offset+3] = 255;
                    }
                }
            }
        }
    }
    sf::RenderWindow window(sf::VideoMode(1536,768,32), "Ultima Underworld Map Viewer");
    sf::Texture temp;
    temp.create(1536,768);
    temp.update(&buffer[0]);
    sf::Sprite sp(temp);
    window.draw(sp);
    window.display();
    while(window.isOpen()) {
        sf::Clock c;
        sf::Event event;
        while(window.pollEvent(event)) {
            if(event.type == sf::Event::Closed) {
                window.close();
            }
            else if(event.type == sf::Event::KeyPressed) {
                //std::cout<<"You pressed a key ("<<event.key.code<<")"<<std::endl;
                switch(event.key.code) {
                    case sf::Keyboard::Q/*Q*/: window.close();
                    default: break; //bugger off
                }
            }
            else if(event.type == sf::Event::GainedFocus||event.type == sf::Event::Resized) {
                window.draw(sp);
                window.display();
            }
        }
        //window.display();
        sf::sleep(sf::milliseconds(1000/10) - c.getElapsedTime());
        c.restart();
    }
}

void simple_map::print_map(const size_t index, const int option /*= 0*/, const UwText& uwt) {
    if(index >= levels.size()) return;
    bool seen[1024] = {0}; //dev: check if I've seen an item number already, this level
    for(int y=63;y>=0;--y) {
        for(int x=0;x<64;++x) {
            if(option == 0) {
                switch(levels[index].d[x][y].type) {
                    case simple_map::SOLID_TILE: cout<<"##";break;
                    case simple_map::OPEN_TILE:  cout<<"  ";break;
                    case simple_map::DIAG_SE:    cout<<"#/";break;
                    case simple_map::DIAG_SW:    cout<<"\\#";break;
                    case simple_map::DIAG_NE:    cout<<"#\\";break;
                    case simple_map::DIAG_NW:    cout<<"/#";break;
                    case simple_map::SLOPE_N:    cout<<"^^";break;
                    case simple_map::SLOPE_S:    cout<<"vv";break;
                    case simple_map::SLOPE_E:    cout<<">>";break;
                    case simple_map::SLOPE_W:    cout<<"<<";break;
                    default: cout<<"!!";break;
                }
            }
            else if(option == 1) {
                switch(levels[index].d[x][y].height) {
                    case 0x0: cout<<"  ";break;
                    case 0x6: cout<<". ";break;
                    case 0x7: cout<<"' ";break;
                    case 0x8: cout<<": ";break;
                    case 0x9: cout<<"- ";break;
                    case 0xa: cout<<"= ";break;
                    case 0xb: cout<<"+ ";break;
                    case 0xc: cout<<"* ";break;
                    case 0xd: cout<<"# ";break;
                    case 0xe: cout<<"% ";break;
                    case 0xf: cout<<"@ ";break;
                    default: printf("%X ",levels[index].d[x][y].height);
                }
            }
            else if(option == 2 && levels[index].d[x][y].first_obj != 0) {
                unsigned short next = 0;
                bool is_npc = false;
                unsigned short cur_item = levels[index].d[x][y].first_obj;

                if(!seen[cur_item]) seen[cur_item] = true;
                else cerr<<"Saw item "<<cur_item<<" more than once!"<<endl;

                uint16_t obj_id = 0;
                if(cur_item < 256) { //"Item" is an NPC
                    obj_id = levels[index].npcs[cur_item].info.obj_id;
                    cout<<"Level "<<index+1<<" Block ("<<y<<", "<<x<<") NPC item start: "<<endl;
                    next = levels[index].npcs[cur_item].info.next_index;
                }
                else { //Item is an actual item
                    obj_id = levels[index].items[cur_item - 256].obj_id;
                    cout<<"Level "<<index+1<<" Block ("<<y<<", "<<x<<") item start: "<<endl;
                    next = levels[index].items[cur_item - 256].next_index;
                }
                cout<<"---------------------------------------------------------"<<endl;
                cout<<cur_item<<"(id: "<<obj_id<<": "<<uwt.get_string(3,obj_id)<<"): "<<endl;
                if(cur_item < 256) {
                    levels[index].npcs[cur_item].print(); cout<<endl;
                }
                else {
                    levels[index].items[cur_item - 256].print(); cout<<endl;
                }

                while(next != 0) {
                    cur_item = next;
                    obj_id = 0;
                    if(!seen[cur_item]) seen[cur_item] = true;
                    else cerr<<"Saw item "<<cur_item<<" more than once!"<<endl;
                    is_npc = (cur_item < 256);
                    if(is_npc) {
                        obj_id = levels[index].npcs[cur_item].info.obj_id;
                        next = levels[index].npcs[cur_item].info.next_index;
                    }
                    else {
                        obj_id = levels[index].items[cur_item - 256].obj_id;
                        next = levels[index].items[cur_item - 256].next_index;
                    }
                    cout<<cur_item<<"(id: "<<obj_id<<": "<<uwt.get_string(3,obj_id)<<")"<<endl;
                    if(cur_item < 256) {
                        levels[index].npcs[cur_item].print(); cout<<endl;
                    }
                    else {
                        levels[index].items[cur_item - 256].print(); cout<<endl;
                    }
                    //cout<<"->"<<cur_item<<"("<<obj_id<<": "<<uwt.get_string(3,obj_id)<<")";
                }
                cout<<"---------------------------------------------------------"<<endl;
                cout<<"---------------------------------------------------------"<<endl;
                cout<<endl;
            }
        }
        cout<<endl;
    }
}

void simple_map::static_obj::print() {
    cout<<hex
        <<"obj_id: "<< obj_id << "\nflags: "<<flags<<"\ndoordir: "<<doordir
        <<"\ninvisible: "<<invisible<<"\nhas quantity: "<<quantitied<<"\nquality: "<<quality<<"\nowner: "<<owner
        <<"\nquantity: "<<quantity<<"\nposition: ("<<xpos<<", "<<ypos<<", "<<zpos<<"), heading: "<<heading<<endl;
}

uint16_t simple_map::static_obj::get_next() {
    return next_index;
}

void simple_map::mobile_obj::print() {
    info.print();
    cout<<hex
        <<"HP: "<<hp<<"\ngoal: "<<goal<<"\ngtarg: "<<gtarg<<"\nlevel: "<<level<<"\ntalked_to: "<<talked_to
        <<"\nattitude: "<<attitude<<"\nheight: "<<height<<"\nhome: ("<<xhome<<", "<<yhome<<")\nheading: "<<heading
        <<"\nhunger: "<<hunger<<"\nwhoami: "<<whoami<<endl;
}

uint16_t simple_map::mobile_obj::get_next() {
    return info.next_index;
}

#ifdef STAND_ALONE_MAP
int main(int argc,char *argv[]) {
    cout<<"Using the simple_map class."<<endl;
    if(argc==3) {
        simple_map sm;
        UwText uwt;
        string fn(argv[1]);
        string tfn(argv[2]);
        sm.load(fn);
        uwt.load(argv[2]);
        
        for(size_t i=0;i<sm.levels.size();++i) {
            cout<<"Level "<<i+1<<": "<<endl;
            sm.print_map(i,0); //Wall layout
            sm.print_map(i,1); //Elevation
            sm.print_map(i,2,uwt); //Per-tile object lists
        }

        sm.graphic_display();
        
    } else {
        cout<<"Two args: The map file to open and the text string file."<<endl;
        return 1;
    }
}
#endif
