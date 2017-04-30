#include "cutscene.h"      //Cutscene direction data (*.n00)
#include "lpfcut.h"        //Cutscene graphics data (*.n[01][^0])
#include "audio/vocfile.h" //Digital audio
#include "UwText.h"        //Text strings
#include "uwfont.h"        //Underworld font importer
#include<iostream>
#include<fstream>

cutscene::cutscene(): cur_n0x(1), cur_frame(0), fade_level(0), fade_target(0), fade_speed(0) {

}

bool cutscene::load(std::string& data_dir, int cut_num) {
    std::string n00_file(data_dir+"/cuts/cs"+to_octal(cut_num)+".n00");
    std::string n01_file(data_dir+"/cuts/cs"+to_octal(cut_num)+".n01");
    std::string font_file(data_dir+"/data/fontbig.sys");
    if(!load_font(font_file)) return false;
    if(!load_n00(n00_file)) return false;
    if(!load_lpf(n01_file)) return false;
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
    //TODO: Load the n00 commands into the class's command list
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
    }
    //TODO: Load the lpf frames into textures stored in the class
    return true;
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

void cutscene::play(sf::RenderTarget& screen) {
    //TODO: Start implementing operation interpretation and playback
}

std::string cutscene::to_octal(int cut_num) {
    std::string base("");
    char dig_1, dig_2, dig_3;
    dig_3 = cut_num % 8;
    cut_num /= 8;
    dig_2 = cut_num % 8;
    cut_num /= 8;
    dig_1 = cut_num;
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


int main(int argc, char *argv[]) {
    //2 args: directory containing game data (cuts+sound), and a cutscene number.
}
