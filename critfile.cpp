#include "critfile.h"
#include "util.h"
#include <iostream>
#include <sstream>

#ifdef STAND_ALONE
//Needs a lot of re-work.
int main(int argc, char* argv[]) {
    if(argc != 3) {
        std::cout<<"Use: \n"<<argv[0]<<" ./crit ./palfile (or wherever you have your \"crit\" directory)"<<std::endl;
        return 1;
    }
    critfile cf;
    std::string critfile = argv[1];
    std::string palfile = argv[2];
    bool retval = false;
    
    retval = cf.load(critfile, palfile);

    if(!retval) {
        std::cout<<"Error opening the files specified. Aborting."<<std::endl;
        return 1;
    }

    std::cout<<"Trying to open a 720x512@32 window."<<std::endl;
    sf::RenderWindow window(sf::VideoMode(720,512,32), "Ultima Underworld Critter Viewer");
    if(window.isOpen()) {
        //std::cout<<"Window opened successfully!"<<std::endl;
    } else {
        std::cout<<"Window could not be opened."<<std::endl;
    }

    std::vector<std::vector<std::vector<sf::Sprite> > > sprite;
    sprite.resize(cf.crits.size());
    uint8_t scale = 0;
    uint8_t max_hsx = 0;
    uint8_t max_hsy = 0;

    for(size_t critter = 0; critter < sprite.size(); ++critter) {
        sprite[critter].resize(cf.crits[critter].frames.size());
        //if(cf.crits[critter].frames.size() != 0)
            //std::cout<<"Giving critter "<<critter<<" a slot count of "<<cf.crits[critter].frames.size()<<std::endl;

        for(size_t slot = 0; slot < sprite[critter].size(); ++slot) {
            sprite[critter][slot].resize(cf.crits[critter].frames[slot].size());
            //if(cf.crits[critter].frames[slot].size() != 0)
            //    std::cout<<"Giving critter"<<critter<<" slot "<<slot<<" a frame count of "<<cf.crits[critter].frames[slot].size()<<std::endl;
            for(size_t framenum = 0; framenum < sprite[critter][slot].size(); ++framenum) {
                //std::cout<<"Adding sprite to sprite[critter = "<<critter<<"][slot = "<<slot<<"][framenum = "<<framenum<<"] from cf.crits[critter = "<<critter<<"].frames[slot = "<<slot<<"][framenum = "<<framenum<<"].tex"<<std::endl;
                sprite[critter][slot][framenum] = sf::Sprite(cf.crits[critter].frames[slot][framenum].tex);
                //std::cout<<"Getting the texture's size, so I can know how to scale the sprite"<<std::endl;
                sf::Vector2u res = cf.crits[critter].frames[slot][framenum].tex.getSize();
                uint8_t fscale = (res.x < res.y)?res.y:res.x;
                if(fscale > scale) scale = fscale;
                if(cf.crits[critter].frames[slot][framenum].hotspot_x > max_hsx)
                    max_hsx = cf.crits[critter].frames[slot][framenum].hotspot_x;
                if(cf.crits[critter].frames[slot][framenum].hotspot_y > max_hsy)
                    max_hsy = cf.crits[critter].frames[slot][framenum].hotspot_y;
                //std::cout<<"Scaling the sprite by a factor of "<<480/scale<<std::endl;
            }
        }
    }
    //Scale and position all the images
    for(size_t critter = 0; critter < sprite.size(); ++critter) {
        for(size_t slot = 0; slot < sprite[critter].size(); ++slot) {
            for(size_t framenum = 0; framenum < sprite[critter][slot].size(); ++framenum) {
                if(scale != 0) {
                    uint8_t hsx = cf.crits[critter].frames[slot][framenum].hotspot_x;
                    uint8_t hsy = cf.crits[critter].frames[slot][framenum].hotspot_y;
                    sprite[critter][slot][framenum].scale(480/scale,480/scale);
                    sprite[critter][slot][framenum].move(10+((max_hsx-hsx)*(480/scale)),32+((max_hsy-hsy)*(480/scale)));
                }
            }
        }
    }
    int crit_num=1;
    int slot_num=0;
    int frame_num=0;
    bool redraw=true;
    int xres = 480;
    int yres = 480;
    //std::cout<<"Starting into main render loop"<<std::endl;
    while(window.isOpen()) {
        sf::Keyboard::Key key=sf::Keyboard::Unknown;
        sf::Clock c;
        sf::Clock frame_time;
        frame_time = sf::Clock();
        sf::Event event;
        //std::cout<<"Starting event polling loop"<<std::endl;
        while(window.pollEvent(event)) {
            if(event.type == sf::Event::Closed) {
                window.close();
            }
            else if(event.type == sf::Event::KeyPressed) {
                key=event.key.code;
                switch(key) {
                    case sf::Keyboard::Q: window.close();
                    default: break;
                }
                redraw=true;
            }
        }
        //std::cout<<"Done with event polling."<<std::endl;
        if(redraw) {
            //std::cout<<"Going to re-draw"<<std::endl;
            if(key==sf::Keyboard::Left) {
                crit_num--;
                if(crit_num < 0) crit_num = sprite.size() - 1;
                slot_num = 0;
                frame_num = 0;
                while(sprite[crit_num][slot_num%sprite[crit_num].size()].size() == 0) {
                    slot_num++;
                    if(slot_num >= sprite[crit_num].size()) slot_num = 0;
                }
            }
            else if(key==sf::Keyboard::Right) {
                crit_num++;
                if(crit_num >= sprite.size()) crit_num = 0;
                slot_num = 0;
                frame_num = 0;
                while(sprite[crit_num][slot_num%sprite[crit_num].size()].size() == 0) {
                    slot_num++;
                    if(slot_num >= sprite[crit_num].size()) slot_num = 0;
                }
            }
            else if(key==sf::Keyboard::Up) {
                slot_num++;
                while(sprite[crit_num][slot_num%sprite[crit_num].size()].size() == 0) {
                    slot_num++;
                    if(slot_num >= sprite[crit_num].size()) slot_num = 0;
                }
            }
            else if(key==sf::Keyboard::Down) {
                slot_num--;
                while(sprite[crit_num][slot_num%sprite[crit_num].size()].size() == 0) {
                    slot_num--;
                    if(slot_num < 0) slot_num = sprite[crit_num].size() - 1;
                }
            }

            window.clear(sf::Color(0,255,0,255));

            if(crit_num < sprite.size() && slot_num < sprite[crit_num].size() && frame_num < sprite[crit_num][slot_num].size()) {
                //std::cout<<"Trying to draw Critter "<<int(crit_num)<<" Slot "<<int(slot_num)<<" Frame "<<int(frame_num)<<std::endl;
                window.draw(sprite[crit_num][slot_num][frame_num]);
            }
            else {
                //std::cout<<"I think I'm outside of my size range ("<<crit_num<<" >= "<<sprite.size()<<" || "<<slot_num<<" >= "<<sprite[crit_num].size()<<" || "<<frame_num<<" >= "<<sprite[crit_num][slot_num].size()<<")"<<std::endl;
            }
            //std::cout<<"Loading the font."<<std::endl;
            sf::Font font;
            if(!font.loadFromFile("FreeSans.ttf"))
                std::cout<<"Couldn't load font."<<std::endl;
            sf::Text t;
            t.setPosition(0,0);
            char buf[100];
            sf::Vector2u res = sf::Vector2u(0,0);
            if(crit_num < sprite.size() && slot_num < sprite[crit_num].size() && frame_num < sprite[crit_num][slot_num].size()) {
                res = cf.crits[crit_num].frames[slot_num][frame_num].tex.getSize();
            }
            sprintf(&buf[0],"Creature: %s palette: %d slot: %d frame: %d Res: %d x %d",
                                       cf.crits[crit_num].name.c_str(),
                                                   cf.crits[crit_num].palnum,
                                                            slot_num, frame_num,
                                                                              res.x, res.y);
            //std::cout<<buf<<std::endl;
            t.setFont(font);
            t.setString(buf);
            t.setColor(sf::Color(255,255,255,255));
            window.draw(t);
            redraw=false;
        }
        window.display();
        sf::sleep(sf::milliseconds(1000/4) - c.getElapsedTime());
        c.restart();
        //if(frame_time.getElapsedTime() > sf::milliseconds(100)) {
            frame_num++;
            frame_num %= sprite[crit_num][slot_num].size();
            //std::cout<<"Set frame to "<<frame_num<<std::endl;
            frame_time.restart();
            redraw = true;
        //}
    }
    return 0;
}
#endif

bool critfile::load(const std::string& critfile, const std::string& palfile) {
    std::cout<<"Reading files in directory: "<<critfile<<std::endl;
    std::cout<<std::endl<<"Reading palette file "<<palfile<<std::endl;
    bool retval = read_palette(palfile);
    if(!retval) return false;
    //std::cout<<"Reading assoc file "<<critfile<<"/assoc.anm"<<std::endl;
    std::vector<std::tuple<std::string, uint8_t, uint8_t> > assocs;
    retval = read_assoc(critfile+"/assoc.anm", assocs);
    if(!retval) return false;
    std::ifstream in;
    crits.resize(64);
    for(int i = 0; i < assocs.size(); ++i) {
        crits[i].name = std::get<0>(assocs[i]);
        crits[i].anim_num = std::get<1>(assocs[i]);
        crits[i].palnum = std::get<2>(assocs[i]);
        std::stringstream fn("");
        fn<<std::oct;
        fn.width(2);
        fn.fill('0');
        int num = std::get<1>(assocs[i]);
        fn<<critfile<<"/cr"<<((num<8)?"0":"")<<num<<"page.n0";
        //std::cout<<fn.str()<<std::endl;
        retval = read_nxx(fn.str() + "0", i);
        if(!retval) return false;
        retval = read_nxx(fn.str() + "1", i);
        if(!retval) return false;
    }
    return retval;
}

// Reads the standard palette file. Auxpals will be loaded on a per-critter basis.
bool critfile::read_palette(const std::string& palfile) {
    std::ifstream in;
    in.open(palfile.c_str());
    if(!in.is_open()) {
        std::cerr<<"Couldn't open "<<palfile<<std::endl;
        return false;
    }
    pal.resize(256);
    for(size_t i = 0;i < 256;++i) {
        unsigned char buf = read8(in);
        pal[i].r = (buf * 4);
        buf = read8(in);
        pal[i].g = (buf * 4);
        buf = read8(in);
        pal[i].b = (buf * 4);
        pal[i].a = 255;
    }
    in.close();
    return true;
}

// Read the critter metadata so I can start processing animation files!
bool critfile::read_assoc(const std::string& assocfile, std::vector<std::tuple<std::string, uint8_t, uint8_t> >& assocs) {
    std::ifstream in;
    in.open(assocfile.c_str());
    if(!in.is_open()) {
        std::cerr<<"Couldn't open "<<assocfile<<std::endl;
        return false;
    }
    assocs.resize(64);
    char name_buffer[32][9];
    for(int i=0;i<32;++i) {
        name_buffer[i][8]=0;
        in.read(&(name_buffer[i][0]),8);
        //std::cout<<"Found entry: "<<name_buffer[i]<<std::endl;
    }
    for(auto it=assocs.begin();it != assocs.end(); ++it) {
        uint8_t anim = read8(in);
        uint8_t auxpalnum = read8(in);
        *it = std::make_tuple(std::string((anim < 32)?name_buffer[anim]:""), anim, auxpalnum);
        //std::cout<<"Anim "<<int(std::get<1>(*it))<<" name: "<<std::get<0>(*it)<<" palnum: "<<int(std::get<2>(*it))<<std::endl;
    }
    in.close();
    return true;
}

//Read the nxx file
bool critfile::read_nxx(const std::string& nxxfile, const uint8_t index) {
    int palnum = crits[index].palnum;
    std::string name = crits[index].name;
    //std::cout<<"Reading file "<<nxxfile<<" ("<<name<<") to index "<<int(index)<<" with aux palette "<<palnum<<std::endl;
    std::ifstream in;
    in.open(nxxfile.c_str());
    if(!in.is_open()) {
        std::cerr<<"Couldn't open "<<nxxfile<<std::endl;
        return false;
    }

    //Read the list of which frame lists go into which slots
    uint8_t slot_base = read8(in);
    uint8_t nslot = read8(in);
    //std::cout<<"Resizing this critter to have "<<int(slot_base+nslot)<<" slots"<<std::endl;
    crits[index].frames.resize(slot_base+nslot);
    std::vector<uint8_t> slot_indexes;
    slot_indexes.resize(nslot);
    for(auto it = slot_indexes.begin(); it != slot_indexes.end(); ++it) {
        *it = read8(in);
        //std::cout<<"Slot index: "<<int(*it)+slot_base<<" at "<<in.tellg()<<std::endl;
    }

    //Read the list of frame lists/segments
    uint8_t nsegs = read8(in);
    std::vector<std::vector<uint8_t> > seg_indexes;
    seg_indexes.resize(nsegs);
    uint8_t frame_count = 0;
    //std::cout<<int(nsegs)<<" animation segments"<<std::endl;
    for(auto it = seg_indexes.begin(); it != seg_indexes.end(); ++it) {
        it->resize(8);
        for(int i=0;i<8;++i) {
            (*it)[i] = read8(in);
            if((*it)[i] != 0xff) frame_count++;
            //std::cout<<int((*it)[i])<<" ";
        }
        //Shrink the vector down to the number of actual frames
        it->resize(frame_count);
        frame_count = 0;
        //std::cout<<std::endl;
    }

    //Read the palette count, skipping over palettes that we aren't using for this criter index
    uint8_t pal_count = read8(in);
    //std::cout<<int(pal_count)<<" palettes"<<std::endl;
    if(pal_count <= palnum) {
       std::cerr<<"Insufficient palette data."<<std::endl;
       return false;
    }

    //Read in the aux palettes, and save the one I'm going to use this time into auxpal
    for(int palcnt = 0; palcnt < pal_count; ++palcnt) {
        for(int palidx = 0; palidx < 32; ++palidx) {
            uint8_t col = read8(in);
            if(palcnt == palnum) {
                auxpal[palidx] = pal[col];
            }
        }
    }

    // Read the file offsets for the actual frame data
    std::vector<critfile::frame*> frames;
    uint8_t offset_count = read8(in);
    uint8_t compression = read8(in); //wtf is that doing right here?? Silly to have compression type in between frame offset count and the actual offsets.
    if(compression != 6) {
        std::cerr<<"Main compression value different than expected ("<<int(compression)<<"). I don't think I'm in the right part of the file."<<std::endl;
        return false;
    }
    frames.resize(offset_count);
    for(int frame = 0; frame < frames.size(); ++frame) {
         uint16_t offset = read16(in);
         //std::cout<<nxxfile<<": read frame "<<frame<<", offset "<<offset<<" ("<<frame+1<<"/"<<int(offset_count)<<std::endl;
         //std::cout<<"Leaving from "<<in.tellg()<<std::endl;
         frames[frame] = read_crit(in, offset);
         //std::stringstream num("");
         //num<<"-p"<<palnum<<"-f"<<frame<<".png";
         //frames[frame]->tex.copyToImage().saveToFile(nxxfile+num.str());
         //std::cout<<"Coming back to "<<in.tellg()<<std::endl;
    }

    // Copy the decoded frames to all the slots that they need to be in.
    for(int slotnum = slot_base; slotnum < slot_base + nslot; ++slotnum) {
        if(slot_indexes[slotnum - slot_base] == 0xff) continue;
        //std::cout<<"Looking at slot "<<slotnum<<" ("<<int(slot_base)<<"+"<<int(slotnum-slot_base)<<"), which wants to use segment "<<int(slot_indexes[slotnum-slot_base])<<" (with "<<int(nsegs)<<" available in the file)"<<std::endl;
        uint8_t slot_frame_count = seg_indexes[slot_indexes[slotnum-slot_base]].size();
        //std::cout<<"There should be "<<int(slot_frame_count)<<" frames in this segment."<<std::endl;
        crits[index].frames[slotnum].resize(slot_frame_count);
        for(int framenum = 0;framenum<slot_frame_count;++framenum) {
            //std::cout<<"Copying frame "<<int(seg_indexes[slot_indexes[slotnum-slot_base]][framenum])<<" to slot "<<slotnum<<", frame "<<framenum<<std::endl;
            crits[index].frames[slotnum][framenum] = *(frames[seg_indexes[slot_indexes[slotnum-slot_base]][framenum]]);
        }
    }
    for(int frame = 0; frame < frames.size(); ++frame) {
        delete frames[frame];
    }
    frames.resize(0);
    return true;
}

// Could be "Read_Critter". Does the actual frame decoding.
critfile::frame* critfile::read_crit(std::ifstream& in, const uint32_t offset) {
    critfile::frame* retval;
    retval = new frame;
    size_t bookmark = in.tellg();
    //std::cout<<"Seeking to offset "<<offset<<" to begin reading"<<std::endl;
    in.seekg(offset, std::ios::beg);
    uint8_t xres,yres,hsx,hsy,compression;
    uint16_t dlen;
    //std::cout<<"Reading some basic data"<<std::endl;
    xres = read8(in);
    yres = read8(in);
    hsx = read8(in);
    hsy = read8(in);
    //std::printf("xres: %03d\nyres: %03d\nhsx: %03d\nhsy: %03d\n",xres,yres,hsx,hsy);
    //std::cout<<"Assigning a little bit of info to the retval"<<std::endl;
    retval->hotspot_x = hsx;
    retval->hotspot_y = hsy;
    //std::cout<<"Creating the texture"<<std::endl;
    retval->tex.create(xres,yres);
    compression = read8(in);
    uint8_t nibble = 0;
    if(compression == 6) {
        //std::cout<<"Compression = 6 (5-bit RLE)"<<std::endl;
        nibble = 5;
        mask = 0x1f;
    }
    else if(compression == 8) {
        //std::cout<<"Compression = 8 (4-bit RLE)"<<std::endl;
        nibble = 4;
        mask = 0xf;
    }
    else {
        std::cerr<<"Frame compression value different than expected ("<<int(compression)<<"). I don't think I'm in the right part of the file."<<std::endl;
        in.seekg(bookmark, std::ios::beg);
        return retval;
    }
    dlen = read16(in);
    //std::cout<<"("<<int(xres)<<", "<<int(yres)<<"), hotspot: ("<<int(hsx)<<", "<<int(hsy)<<"), length: "<<dlen<<std::endl;

    std::vector<color> buffer;
    
    buffer.resize(xres * yres);

    //onfirst tracks which nibble I'm on for the 4-bit image formats
    uint8_t bits_remaining = 0;

    //Repeat-run tracking for 4/5-bit RLE image format
    bool repeat = true;
    //"col" was originally for "color". I'm using it for the nibble-wrangler now.
    uint16_t col = 0;

    size_t pix_index = 0;

    nibble_count = 0;

    while(pix_index < xres  * yres && nibble_count < dlen) {
        //std::cout<<"Pix index: "<<pix_index<<"/"<<xres*yres<<std::endl;
        uint16_t count = get_rle_count(in,col,bits_remaining,nibble);
        //std::cout<<"Got a count of "<<count<<" for this "<<((repeat)?"repeat":"run")<<" record. "<<std::endl;
        uint16_t multrep = 1;
        uint8_t index = 0;

        if(repeat) {
            if(count == 1) {
                //std::cout<<"That means I skip this repeat record and do a run record."<<std::endl;
                repeat = false;
                continue;
            }
            else if(count == 2 && repeat) {
                multrep = get_rle_count(in,col,bits_remaining,nibble);
                //std::cout<<"That means that I do "<<multrep<<" repeat records in a row."<<std::endl;
            }
            for(uint16_t reps = 0; reps < multrep && pix_index < xres*yres; ++reps) {
                if(multrep > 1) {
                    count = get_rle_count(in,col,bits_remaining,nibble);
                    //std::cout<<"Repeat #"<<reps+1<<" count: "<<count<<" ";
                }
                index = get_nibble(in,col,bits_remaining,nibble);
                //std::cout<<"Index: "<<int(index)<<std::endl;
                for(uint16_t count_process = 0; count_process < count && pix_index < xres*yres; count_process++) {
                    if(index == 1) {
                        buffer[pix_index].r = 255;
                        buffer[pix_index].g = 0;
                        buffer[pix_index].b = 255;
                        buffer[pix_index++].a = 0;
                    }
                    else {
                        buffer[pix_index++] = auxpal[index];
                    }
                }
            }
        }
        else { //Run
            //std::cout<<"Index: ";
            for(uint16_t count_process = 0; count_process < count && pix_index < xres * yres; count_process++) {
                index = get_nibble(in, col, bits_remaining, nibble);
                //std::cout<<int(index)<<" ";
                if(index == 1) {
                    buffer[pix_index].r = 255;
                    buffer[pix_index].g = 0;
                    buffer[pix_index].b = 255;
                    buffer[pix_index++].a = 0;
                }
                else
                    buffer[pix_index++] = auxpal[index];
            }
            //std::cout<<std::endl;
        }
        repeat = !repeat;
    }
    //sf::Texture temp;
    //temp.create(xres,yres);
    //temp.update(reinterpret_cast<uint8_t *>(&(buffer[0])));
    retval->tex.update(reinterpret_cast<uint8_t *>(&buffer[0]));
    //std::cout<<"Trying to seek back to "<<bookmark<<"  so I can return."<<std::endl;
    in.seekg(bookmark, std::ios::beg);
    return retval;
}

//Critter frames are stored in 4/5-bit RLE format, so this fetches the next 4 or 5-bit nibble, as necessary.
uint8_t critfile::get_nibble(std::ifstream& in, uint16_t& col, uint8_t& bits_remaining, const uint8_t nibble) {
    if(bits_remaining < nibble) {
        col<<=(8);
        col |= read8(in);
        bits_remaining+=8;
        //Take the top 5 bits, then shift col 5 bits smaller.
    }
    uint8_t retval = (col & (mask<<(bits_remaining - nibble)) )>>(bits_remaining - nibble);
    //std::cout<<"Got nibble: "<<int(retval)<<std::endl;
    bits_remaining -= nibble;
    nibble_count++;
    return retval;
}

//Each repeat/run record starts with a count for how many records to do. This gets that count.
//And yes, counts for 5-bit nibbles get shifted 4 bits. Go figure.
uint16_t critfile::get_rle_count(std::ifstream& in, uint16_t& col, uint8_t& bits_remaining, const uint8_t nibble) {
    uint16_t retval = get_nibble(in,col,bits_remaining,nibble);
    if(retval != 0) return retval;
    retval = get_nibble(in,col,bits_remaining,nibble);
    retval<<=(4);
    uint16_t temp = get_nibble(in,col,bits_remaining,nibble);
    retval|=temp;
    if(retval != 0) return retval;
    retval = get_nibble(in,col,bits_remaining,nibble);
    retval<<=(4);
    temp=get_nibble(in,col,bits_remaining,nibble);
    retval|=temp;
    temp = get_nibble(in,col,bits_remaining,nibble);
    retval|=get_nibble(in,col,bits_remaining,nibble);
    return retval;
}
