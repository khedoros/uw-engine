#pragma once

#include<cstdint>
#include<string>
#include<vector>
#include<SFML/Graphics.hpp>

class cutscene {
public:
    cutscene();
    bool load(std::string& dat_dir, int cut_num);
    void play(sf::RenderTarget&);
private:
    std::string to_octal(int input);
    bool load_n00(std::string&);
    bool load_lpf(std::string&);
    bool load_font(std::string&);
    class cut_cmd {
    public:
        uint16_t cmd_num;
        uint16_t arg1;
        uint16_t arg2;
        uint16_t arg3;
        std::string& tostring();
        const static int arg_counts[32];
    };
    std::vector<sf::Texture> frame;
    std::vector<cutscene::cut_cmd> cmd;
    std::vector<std::string> strings;
    sf::Font cs_font;
    uint32_t cur_frame;
    uint32_t cur_n0x;
    int fade_level;
    int fade_target;
    int fade_speed;
};
