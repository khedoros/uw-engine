#pragma once

#include<cstdint>
#include<string>
#include<vector>
#include<SFML/Graphics.hpp>
#include<SFML/Audio.hpp>
#include "audio/audioManager.h"

class cutscene {
public:
    cutscene(bool subs = true,bool vocs = true);
    bool load(const std::string& dat_dir, int cut_num);
    void play(sf::RenderWindow&,audioManager&);
private:
    std::string to_octal2(int input);
    std::string to_octal3(int input);
    bool load_n00(std::string&);
    bool load_lpf(std::string&);
    bool load_font(std::string&);
    bool load_strings(std::string&, int);
    void play_voc(int);
    std::string format_string(std::string&);
    class cut_cmd {
    public:
        uint16_t frame;
        uint16_t cmd_num;
        uint16_t args[3];
        std::string tostring();
        const static int arg_counts[32];
    };
    std::vector<sf::Texture> frame;
    std::vector<cutscene::cut_cmd> cmd;
    std::vector<std::string> strings;
    sf::Color cur_lpf_pal[256];
    sf::Sound cur_sound;
    sf::SoundBuffer cur_sb;
    sf::Font cs_font;
    std::string base_dir;
    uint32_t cut_num;
    uint32_t cur_frame;
    uint32_t cur_n0x;
    int fade_level;
    int fade_target;
    int fade_speed;
    bool with_subs;
    bool with_vocs;
};
