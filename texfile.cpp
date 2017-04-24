#include "texfile.h"
#include "util.h"
#include<iostream>
#include<string>

#ifdef STAND_ALONE
int main(int argc, char *argv[]) {
    if(argc != 3 && argc !=4) {
        std::cout<<"Use: \n"<<argv[0]<<" palette_file texture_file.tr\n OR\n"<<argv[0]<<" palette_file img_file.gr altpalfile"<<std::endl;
        return 1;
    }
    texfile tf;
    std::string palfile, texfile, altpalfile;
    bool retval = false;
    if(argc == 3) {
        palfile = argv[1];
        texfile = argv[2];
        retval = tf.load(palfile,texfile);
    }
    else if(argc == 4) {
        palfile = argv[1];
        texfile = argv[2];
        altpalfile = argv[3];
        retval = tf.load(palfile, texfile, altpalfile);
    }

    if(!retval) {
        std::cout<<"Error opening the files specified. Aborting."<<std::endl;
        return 1;
    }

    std::cout<<"Trying to open a 480x512@32 window."<<std::endl;
    sf::RenderWindow window(sf::VideoMode(480,512,32), "Ultima Underworld Texture Viewer");
    if(window.isOpen()) {
        std::cout<<"Window opened successfully!"<<std::endl;
    } else {
        std::cout<<"Window could not be opened."<<std::endl;
    }

    std::vector<sf::Sprite> sprite;
    sprite.resize(tf.tex.size());
    for(size_t i=0;i<sprite.size();++i) {
        sprite[i] = sf::Sprite(tf.tex[i]);
        sprite[i].move(10,32);
        if(tf.res != 0)
            sprite[i].scale(480/tf.res,480/tf.res);
        else
            sprite[i].scale(480/tf.xres,480/tf.yres);
    }
    int sprite_num=0;
    bool redraw=true;
    int xres = 480;
    int yres = 480;
    while(window.isOpen()) {
        sf::Keyboard::Key key=sf::Keyboard::Unknown;
        sf::Clock c;
        sf::Event event;
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
        if(redraw) {
            if(key==sf::Keyboard::Left)
                sprite_num--;
            else if(key==sf::Keyboard::Right)
                sprite_num++;
            else if(key==sf::Keyboard::D) {
                for(int i=0;i<tf.tex.size();++i) {
                    sf::Image temp = tf.tex[i].copyToImage();
                    temp.saveToFile(texfile + "-" + std::to_string(i) + ".png");
                }
            }
            if(sprite_num == tf.tex.size()) sprite_num = 0;
            else if(sprite_num == -1) sprite_num = tf.tex.size() - 1;

            window.clear(sf::Color(0,255,0,255));
            window.draw(sprite[sprite_num]);
            sf::Font font;
            if(!font.loadFromFile("FreeSans.ttf"))
                std::cout<<"Couldn't load font."<<std::endl;
            sf::Text t;
            t.setPosition(0,0);
            char buf[100];
            sprintf(&buf[0],"Res: %d, Index: %d",tf.res,sprite_num);
            std::cout<<buf<<std::endl;
            t.setFont(font);
            t.setString(buf);
            t.setColor(sf::Color(255,255,255,255));
            window.draw(t);
            redraw=false;
        }
        window.display();
        sf::Time t;
        sf::sleep(sf::milliseconds(1000/60) - c.getElapsedTime());
        c.restart();
    }
    return 0;
}
#endif

bool texfile::load(const std::string& palfile,const std::string& texfile, const std::string& altpalfile) {
    std::cout<<"Reading palette: "<<palfile<<" texfile: "<<texfile;
    if(altpalfile != std::string())
        std::cout<<" altpalfile: "<<altpalfile;
    std::cout<<std::endl;
    bool retval = read_palette(palfile, altpalfile);
    if(!retval) {
        std::cerr<<"Couldn't read palette "<<palfile<<" or "<<altpalfile<<std::endl;
        return false;
    }
    std::ifstream in;
    in.open(texfile.c_str());
    if(!in.is_open()) {
        std::cerr<<"Couldn't open texfile "<<texfile<<std::endl;
        return false;
    }
    std::string namecut = texfile.substr(texfile.length() -4, 4);
    if(namecut == ".byt" || namecut == ".BYT") {
        std::cout<<"Interpreting as a 320x200 bitmap, based on filename."<<std::endl;
        return load_byt(in, texfile);
    }
    uint8_t bit8 = read8(in);
    if(bit8 == 2) {
        std::cout<<"First byte is 2. Interpreting as a texture file."<<std::endl;
        retval = load_tr(in);
    }
    else if(bit8 == 1) {
        std::cout<<"First byte is 1. Interpreting as an image file."<<std::endl;
        bool no_hdr = false;
        int main_pal = 0;
        auto missing = std::string::npos;
        if(texfile.find("panels.gr") != missing || texfile.find("PANELS.GR") != missing) {
            no_hdr = true;
        }
        else if(texfile.find("opbtn.gr") != missing || texfile.find("OPBTN.GR") != missing) {
            main_pal = 2;
        }
        else if(texfile.find("chrbtns.gr") != missing || texfile.find("CHRBTNS.GR") != missing) {
            main_pal = 3;
        }
        retval = load_gr(in, main_pal, no_hdr);
    }
    else {
        std::cout<<"First byte is "<<int(bit8)<<", which is unexpected."<<std::endl;
        return false;
    }
    in.close();
    return retval;
}

bool texfile::load_byt(std::ifstream& in, const std::string& name) {
    xres = 320;
    yres = 200;
    res = 320;
    uint8_t palnum = 0;
    if(name.find("blnkmap.byt") != std::string::npos) palnum = 1;
    else if(name.find("chargen.byt") != std::string::npos) palnum = 3;
    else if(name.find("opscr.byt") != std::string::npos) palnum = 2;
    else if(name.find("pres1.byt") != std::string::npos || name.find("pres2.byt") != std::string::npos || name.find("presd.byt") != std::string::npos) palnum = 5;
    else if(name.find("win1.byt") != std::string::npos || name.find("win2.byt") != std::string::npos) palnum = 7;
    std::vector<color> buffer;
    buffer.resize(320*200);
    for(size_t i=0;i<320*200;++i) {
        uint8_t col = read8(in);
        buffer[i].r = allpals[palnum][col].r;
        buffer[i].g = allpals[palnum][col].g;
        buffer[i].b = allpals[palnum][col].b;
        buffer[i].a = allpals[palnum][col].a;
    }
    tex.resize(1);
    animtex.resize(1);
    sf::Texture temp;
    temp.create(xres,yres);
    temp.update(reinterpret_cast<uint8_t *>(&(buffer[0])));
    tex[0] = temp;
    if(name == "opscr.byt") populate_animtex(0,64,127);
    return true;
}

bool texfile::load_tr(std::ifstream& in) {
    res=read8(in);//res x res is the texture resolution
    uint16_t bit16 = read16(in); //this word contains a count of the number of textures in the file
    std::cout<<"Res: "<<uint16_t(res)<<"x"<<uint16_t(res)<<", "<<bit16<<" textures in the file."<<std::endl;
    if(res < 16 || bit16 == 0)
        return false;
    tex.resize(bit16);
    animtex.resize(bit16);

    for(size_t i=0;i<tex.size();++i) {
        tex[i]=read_texture(in, read32(in));
        //tex[i].setSmooth(true);
        tex[i].setRepeated(true);
    }

    return true;
}

bool texfile::load_gr(std::ifstream& in, const int palnum, const bool no_img_hdr) {
    if(altpal.size() == 0) {
        std::cerr<<"You need to specify an alternate palette file to open this kind of image. Aborting."<<std::endl;
        return false;
    }
    //uint8_t bit8;
    uint16_t bit16;
    uint32_t bit32;
    bit16 = read16(in);
    tex.resize(bit16);
    animtex.resize(bit16);
    std::cout<<"Reading "<<bit16<<" images."<<std::endl;

    for(size_t i=0;i<tex.size(); ++i) {
        std::cout<<i<<": ";
        tex[i]=read_bmp(in, read32(in), palnum, no_img_hdr);
        //tex[i].setSmooth(true);
        tex[i].setRepeated(false);
    }
    return true;
}

bool texfile::read_palette(const std::string& palfile, const std::string& altpalfile) {
    std::ifstream in;
    in.open(palfile.c_str());
    if(!in.is_open()) {
        std::cerr<<"Couldn't open "<<palfile<<std::endl;
        return false;
    }
    //palette.resize(256);
    allpals.resize(8);
    for(int i=0;i<8;++i) {
        allpals[i].resize(256);
        for(size_t j = 0;j < 256;++j) {
            unsigned char buf = read8(in);
            allpals[i][j].r = (buf * 4);
            //if(i==0)
            //    palette[j].r = (buf * 4);
            buf = read8(in);
            allpals[i][j].g = (buf * 4);
            //if(i==0)
            //    palette[j].g = (buf * 4);
            buf = read8(in);
            allpals[i][j].b = (buf * 4);
            //if(i==0) {
            //    palette[j].b = (buf * 4);
            //    palette[j].a = 255;
            //}
            allpals[i][j].a = ((j == 0)?0:255);
        }
    }
    in.close();

    if(altpalfile != std::string()) {
        binifstream in;
        in.open(altpalfile.c_str());
        if(!in.is_open()) {
            std::cerr<<"Couldn't open "<<altpalfile<<std::endl;
            return false;
        }
        altpal.resize(32);
        for(size_t pal = 0; pal < 32; ++pal) {
            altpal[pal].resize(16);
            unsigned char buf;
            for(size_t pal_entry = 0; pal_entry < 16; ++pal_entry) {
                in>>buf;
                altpal[pal][pal_entry] = allpals[0][buf];
            }
        }
        in.close();
    }
    return true;
}

sf::Texture texfile::read_bmp(std::ifstream& in, const uint32_t offset, const int palnum, const bool no_img_hdr) {
    size_t bookmark = in.tellg();
    in.seekg(offset, std::ios::beg);
    std::vector<color> buffer;
    uint8_t type = 0;
    if(!no_img_hdr) {
        type = read8(in);
        xres = read8(in);
        yres = read8(in);
    }
    else {
        //Image alignment is off by 2 pixels if I don't do this:
        in.seekg(offset-2, std::ios::beg);

        //Image doesn't have a header, and these are the known-correct values
        type = 4;
        xres = 83;
        yres = 114;
    }
    if(xres > res) res = xres;
    if(yres > res) res = yres;
    buffer.resize(xres * yres);
    uint8_t altpalnum = 0;
    if(type == 8 || type == 0x0a) altpalnum = read8(in);
    std::cout<<"Type: "<<int(type)<<std::endl;
    uint16_t expected_size = read16(in);
    //std::cout<<"Expected size: "<<expected_size<<" ("<<xres*yres<<" by dimensions)"<<std::endl;
    size_t start_offset = in.tellg();
    if(type != 0x04 && type != 0x08 && type != 0x0a) {
        std::cerr<<"WTF is type "<<int(type)<<"? Definitely not implemented! (offset "<<offset<<")"<<std::endl;
        in.seekg(bookmark, std::ios::beg);
        return sf::Texture();
    }
    //onfirst tracks which nibble I'm on for the 4-bit image formats
    bool onfirst = true;
    //Repeat-run tracking for 4-bit RLE image format
    bool repeat = true;
    //"col" was originally for "color". I'm using it for the nibble-wrangler now.
    uint8_t col = 0;
    if(type == 0x04 || type == 0x0a) {
        for(int pix = 0; pix < xres * yres; ++pix) {
            if(type == 0x04) {
                col = read8(in);
                buffer[pix].r = allpals[palnum][col].r;
                buffer[pix].g = allpals[palnum][col].g;
                buffer[pix].b = allpals[palnum][col].b;
                buffer[pix].a = allpals[palnum][col].a;
            }
            else if(type == 0x0a) {
                uint8_t index = get_nibble(in, col, onfirst);
                buffer[pix].r = altpal[altpalnum][index].r;
                buffer[pix].g = altpal[altpalnum][index].g;
                buffer[pix].b = altpal[altpalnum][index].b;
                buffer[pix].a = altpal[altpalnum][index].a;
            }
        }
    }
    else if(type == 0x08) {
        size_t pix_index = 0;
        while(pix_index < xres  * yres) {
            //std::cout<<"Pix index: "<<pix_index<<"/"<<xres*yres<<std::endl;
            uint16_t count = get_rle_count(in,col,onfirst);
            //std::cout<<"Got a count of "<<count<<" for this "<<((repeat)?"repeat":"run")<<" record. "<<std::endl;
            uint16_t multrep = 1;
            uint8_t index = 0;
            if(count == 1 && repeat) {
                //std::cout<<"That means I skip this repeat record and do a run record."<<std::endl;
                repeat = false;
                continue;
            }
            if(count == 2 && repeat) {
                multrep = get_rle_count(in,col,onfirst);
                //std::cout<<"That means that I do "<<multrep<<" repeat records in a row."<<std::endl;
            }
            //if(!repeat) index = get_nibble(in,col,onfirst);
            for(uint16_t reps = 0; reps < multrep; ++reps) {
                if(repeat) {
                    if(multrep > 1)
                        count = get_rle_count(in,col,onfirst);
                    index = get_nibble(in,col,onfirst);
                    //std::cout<<"Repeat record "<<reps+1<<"/"<<multrep<<": repeat "<<std::hex<<int(index)<<" "<<count<<" times"<<std::endl;
                }
                for(uint16_t count_process = 0; count_process < count && pix_index < buffer.size(); count_process++) {
                    if(!repeat) index = get_nibble(in,col,onfirst);
                    buffer[pix_index].r = altpal[altpalnum][index].r;
                    buffer[pix_index].g = altpal[altpalnum][index].g;
                    buffer[pix_index].b = altpal[altpalnum][index].b;
                    buffer[pix_index++].a = altpal[altpalnum][index].a;
                }
            }
            repeat = !repeat;
        }
    }
    sf::Texture temp;
    temp.create(xres,yres);
    temp.update(reinterpret_cast<uint8_t *>(&(buffer[0])));
    in.seekg(bookmark, std::ios::beg);
    return temp;
}

uint8_t texfile::get_nibble(std::ifstream& in, uint8_t& col, bool& onfirst) {
    if(onfirst) {
        col = read8(in);
        onfirst = !onfirst;
        //std::cout<<"Got nibble: "<<((col&0xf0)>>(4))<<std::endl;
        return (col&0xf0)>>(4);
    }
    onfirst = !onfirst;
    //std::cout<<"Got nibble: "<<(col&0x0f)<<std::endl;
    return (col&0x0f);
}

uint16_t texfile::get_rle_count(std::ifstream& in, uint8_t& col, bool& onfirst) {
    uint16_t retval = get_nibble(in,col,onfirst);
    if(retval != 0) return retval;
    retval = (get_nibble(in,col,onfirst))<<(4) | get_nibble(in,col,onfirst);
    if(retval != 0) return retval;
    retval = (get_nibble(in,col,onfirst)<<(4) | get_nibble(in,col,onfirst))<<(4) | get_nibble(in,col,onfirst);
    return retval;
}
    

sf::Texture texfile::read_texture(std::ifstream& in, uint32_t offset) {
    size_t bookmark = in.tellg();
    in.seekg(offset, std::ios::beg);
    std::vector<color> buffer;
    buffer.resize(res * res);
    for(size_t i = 0;i < res * res;++i) {
        uint8_t index = read8(in);
        buffer[i].r = allpals[0][index].r;
        buffer[i].g = allpals[0][index].g;
        buffer[i].b = allpals[0][index].b;
        buffer[i].a = allpals[0][index].a;
    }
    sf::Texture temp;
    temp.create(res,res);
    temp.update(reinterpret_cast<uint8_t *>(&(buffer[0])));
    in.seekg(bookmark,std::ios::beg);
    return temp;
}

void texfile::populate_animtex(size_t texnum, uint8_t low_pal, uint8_t hi_pal) {
    if(low_pal >= hi_pal) return;
    if(texnum >= tex.size()) return;
    if(texnum >= animtex.size()) return;
    animtex[texnum].resize(hi_pal - low_pal + 1);
    animtex[texnum][0]=tex[texnum];
    sf::Image ref = tex[texnum].copyToImage();
    sf::Image out = tex[texnum].copyToImage();
    sf::Texture tout;
    for(size_t frame = 1; frame <= hi_pal - low_pal; ++frame) {
        for(size_t col = low_pal; col <= hi_pal; ++col) {
            uint8_t new_index = (col + frame) % (hi_pal - low_pal + 1) + low_pal;
            //std::cout<<"old index: "<<int(col)<<" new index: "<<int(new_index)<<std::endl;
            for(size_t x = 0; x < res; ++x)
                for(size_t y = 0; y < res; ++y) {
                    if(ref.getPixel(x,y) == sf::Color(allpals[0][col].r,allpals[0][col].g,allpals[0][col].b)) {
                        out.setPixel(x,y,sf::Color(allpals[0][new_index].r, allpals[0][new_index].g, allpals[0][new_index].b));
                    }
                }
        }
        tout.loadFromImage(out);
        //tout.setSmooth(true);
        tout.setRepeated(true);
        animtex[texnum][frame] = tout;
    }
    
}
