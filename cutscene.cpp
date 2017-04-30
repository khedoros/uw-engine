#include "cutscene.h"      //Cutscene direction data (*.n00)
#include "lpfcut.h"        //Cutscene graphics data (*.n[01][^0])
#include "audio/vocfile.h" //Digital audio
#include "UwText.h"        //Text strings
#include "uwfont.h"        //Underworld font importer
#include "util.h"
#include<iostream>
#include<fstream>

cutscene::cutscene(): cur_n0x(1), cur_frame(0), fade_level(0), fade_target(0), fade_speed(0) {

}

bool cutscene::load(const std::string& data_dir, int cut_num) {
    base_dir = data_dir;
    this->cut_num = cut_num;
    std::string n00_file(data_dir+"/cuts/cs"+to_octal3(cut_num)+".n00");
    std::string n01_file(data_dir+"/cuts/cs"+to_octal3(cut_num)+".n01");
    std::string font_file(data_dir+"/data/fontbig.sys");
    std::string strings_file(data_dir+"/data/strings.pak");
    if(!load_font(font_file)) return false;
    if(!load_n00(n00_file)) return false;
    if(!load_strings(strings_file, cut_num)) return false;
    //File may or may not exist; things like the "dream" cutscenes use .n01's that don't match the .n00, but they load them immediately with a command in the n00.
    load_lpf(n01_file);
    return true;
}

bool cutscene::load_n00(std::string& filename) {
    cmd.resize(0);
    std::ifstream infile(filename.c_str());
    if (!infile.is_open()) {
        std::cerr << filename << " didn't open. Fix it." << std::endl;
        return false;
    }
    int filesize = 0;
    infile.seekg(0, std::ios::end);
    filesize = infile.tellg();
    infile.seekg(0, std::ios::beg);
    std::cout << "Filesize: " << filesize << std::endl;
    if (filesize % 2 != 0 || filesize > 2048) {
        std::cerr << "Not likely to be a cutscene description. Closing." << std::endl;
        infile.close();
        return false;
    }

    while(infile.tellg() < filesize) {
        cut_cmd cmd_data;
        cmd_data.frame = read16(infile);
        cmd_data.cmd_num = read16(infile);
        for(int i=0;i<cut_cmd::arg_counts[cmd_data.cmd_num];++i) {
            cmd_data.args[i] = read16(infile);
        }
        cmd.push_back(cmd_data);
    }
    return true;
}

bool cutscene::load_lpf(std::string& filename) {
    frame.resize(0);
    cur_frame = 0;
    Lpfcut lpf;
    if(!lpf.load(filename)) {
        std::cerr<<filename<<" didn't open. Fix it." <<std::endl;
        return false;
    }
    lpf.repeat = false;
    sf::Uint8 * frame_data = (sf::Uint8 *)lpf.getNextFrame();
    while(frame_data) {
        sf::Texture f;
        f.create(lpf.width, lpf.height);
        f.update(frame_data);
        frame.push_back(f);
        frame_data = (sf::Uint8 *)lpf.getNextFrame();
    }
    for(int i=0;i<256;++i) {
        color temp=lpf.getPalEntry(i);
        cur_lpf_pal[i] = sf::Color(temp.r,temp.g,temp.b);
        //std::cout<<"Palette "<<i<<": ("<<std::hex<<cur_lpf_pal[i].toInteger()<<") from ("<<temp.r<<temp.g<<temp.b<<temp.a<<")"<<std::endl;
    }
    if(frame.size() + 1 == lpf.rec_count)
        return true;
    else {
        std::cerr<<"Loaded "<<frame.size()<<" frames, but the lpf reports that "<<lpf.rec_count<<" should be available!"<<std::endl;
    }
}

bool cutscene::load_font(std::string& filename) {
    uwfont f;
    bool ret = f.load(filename);
    if(!ret) {
        std::cerr<<"Couldn't load "<<filename<<std::endl;
        return ret;
    }
    std::string font_repr = f.to_bdf();
    ret = cs_font.loadFromMemory(font_repr.data(), font_repr.length());
    if(!ret) {
        std::cerr<<"Couldn't load the font into SFML"<<std::endl;
        return ret;
    }
    return true;
}

bool cutscene::load_strings(std::string& filename, int cut_num) {
    strings.resize(0);
    UwText text;
    if(!text.load(filename)) {
        std::cerr<<"Couldn't load "<<filename<<". Fix it."<<std::endl;
        return false;
    }
    int blocknum = 0;
    for(int i=0;i<text.get_block_count();++i) {
        if(text.get_block_num(i) == 0xc00 + cut_num) {
            blocknum = i;
            break;
        }
    }
    for(int s=0;s<text.get_string_count(blocknum);++s) {
        strings.push_back(text.get_string(blocknum, s));
    }
    return true;
}

void cutscene::play(sf::RenderWindow& screen) {
    //TODO: Start implementing operation interpretation and playback
    sf::Sprite spr;
    spr.scale(2,2);
    sf::Text txt("", cs_font, 10);
    for(int i=0; i<cmd.size();i++) {
        screen.setFramerateLimit(5);
        while(cmd[i].frame - 1 < cur_frame && cur_frame < frame.size()) {
            spr.setTexture(frame[cur_frame]);
            screen.draw(spr);
            screen.draw(txt);
            screen.display();
            cur_frame++;
        }
        spr.setTexture(frame[cur_frame]);
        switch(cmd[i].cmd_num) {
            case 0x00: //Display text arg[1] with color arg[0]
                std::cout<<strings.size()<<"\t"<<cmd[i].args[1]<<"\t"<<cmd[i].args[0]<<std::endl;
                std::cout<<strings[cmd[i].args[1]]<<std::endl;
                assert(cmd[i].args[1] < strings.size());
                txt = sf::Text(strings[cmd[i].args[1]], cs_font, 10);
                txt.setFillColor(cur_lpf_pal[cmd[i].args[0]]);
                //std::cout<<"Fill color: 0x"<<std::hex<<cur_lpf_pal[cmd[i].args[0]].toInteger()<<std::endl;
                txt.setPosition(0,500);
                screen.draw(spr);
                screen.draw(txt);
                screen.display();
                break;
            case 0x04: //Play up to frame arg[0] at rate arg[1]
                screen.setFramerateLimit(2*cmd[i].args[1]+1);
                for(;cur_frame<cmd[i].args[0];++cur_frame) {
                    screen.draw(spr);
                    screen.draw(txt);
                    screen.display();
                }
                break;
            case 0x05:
                {
                    std::string n0x_file(base_dir+"/cuts/cs"+to_octal3(cut_num)+".n"+to_octal2(cmd[i].args[0]));
                    if(!load_lpf(n0x_file)) {
                        std::cerr<<"I made a mistake; couldn't use command 5 as an indicator to load "<<n0x_file<<"."<<std::endl;
                    }
                    cur_n0x = cmd[i].args[0];
                }
                break;
            case 0x06: //End of cutscene
                return;
            case 0x07:
                for(int rep=0;rep<cmd[i].args[0];++rep) {
                    for(int rep_frame=0;rep_frame < cmd[i].frame-1;++rep_frame) {
                        spr.setTexture(frame[rep_frame]);
                        screen.draw(spr);
                        screen.display();
                        std::cout<<"Frame "<<rep_frame<<std::endl;
                    }
                }
                break;
            case 0x09: //Fade out
                {
                    screen.setFramerateLimit(30);
                    sf::RectangleShape fade(sf::Vector2f(screen.getSize()));
                    int rate = 16;
                    if(cmd[i].args[0] != 0) {
                        rate = 256/(16*cmd[i].args[0]);
                    }
                    for(int opacity=0;opacity < 255; opacity+=rate) {
                        fade.setFillColor(sf::Color(0,0,0,opacity));
                        screen.draw(spr);
                        screen.draw(fade);
                        screen.display();
                    }
                }
                break;
            case 0x0a: //Fade in
                {
                    screen.setFramerateLimit(30);
                    sf::RectangleShape fade(sf::Vector2f(screen.getSize()));
                    int rate = 16;
                    if(cmd[i].args[0] != 0) {
                        rate = 256/(16*cmd[i].args[0]);
                    }
                    for(int opacity=255;opacity >= 0; opacity-=rate) {
                        fade.setFillColor(sf::Color(0,0,0,opacity));
                        screen.draw(spr);
                        screen.draw(fade);
                        screen.display();
                    }
                }
                break;
            default:
                std::cout<<cmd[i].tostring()<<std::endl;
                break;
        }
    }
}

std::string cutscene::to_octal2(int cut_num) {
    std::string base("");
    char dig_1, dig_2;
    dig_2 = cut_num % 8 + '0';
    cut_num /= 8;
    dig_1 = cut_num + '0';
    return base+dig_1+dig_2;
}

std::string cutscene::to_octal3(int cut_num) {
    std::string base("");
    char dig_1, dig_2, dig_3;
    dig_3 = cut_num % 8 + '0';
    cut_num /= 8;
    dig_2 = cut_num % 8 + '0';
    cut_num /= 8;
    dig_1 = cut_num + '0';
    return base+dig_1+dig_2+dig_3;
}

const int cutscene::cut_cmd::arg_counts[32] = { 2, 0, 2, 1,
                                            2, 1, 0, 1,
                                            2, 1, 1, 1,
                                            1, 3, 2, 0, //UW1 functions end at 0xf

                                            0, 0, 0, 0,
                                            0, 0, 0, 0,
                                            1, 1, 0, 1,
                                            0, 0, 0, 0 };

std::string cutscene::cut_cmd::tostring() {
    if(cmd_num >=0 && cmd_num < 16) {
        std::string retval(std::string("Frame: ") + std::to_string(frame) + " Cmd: " + std::to_string(cmd_num));
        if(arg_counts[cmd_num] > 0) retval += " Args: ";
        for(int i=0;i<arg_counts[cmd_num];++i) {
            retval+=" "+std::to_string(args[i]);
        }
        return retval;
    }
    else if(cmd_num >=16 && cmd_num < 32) {
        return std::string("Probably a command from a UW2 cutscene. Parsing past this point is broken :-)");
    }
}

int main(int argc, char *argv[]) {
    //2 args: directory containing game data (cuts+sound), and a cutscene number.
    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML window");
    cutscene c;
    if(!c.load(argv[1], std::stoi(argv[2]))) {
        std::cerr<<"Failed to load files using path "<<argv[1]<<std::endl;
    }
    c.play(window);
}
