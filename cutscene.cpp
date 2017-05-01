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
    for(int s=0;s<text.get_string_count(0xc00+cut_num);++s) {
        strings.push_back(text.get_string(0xc00+cut_num, s));
    }
    return true;
}

void cutscene::play(sf::RenderWindow& screen) {
    //TODO: Start implementing operation interpretation and playback
    screen.setFramerateLimit(10);

    for(int i=0; i<cmd.size();i++) {
        switch(cmd[i].cmd_num) {
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
