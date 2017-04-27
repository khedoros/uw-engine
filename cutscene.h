#pragma once

#include<cstdint>
#include<string>
#include<vector>
#include<SFML/Graphics.hpp>

class cutscene {
public:
    cutscene();
    bool load(std::string& cut_dir, int cut_num);
    void play(sfml::RenderTarget&);
private:
    class cut_cmd {
    public:
        uint16_t cmd_num;
        uint16_t arg1;
        uint16_t arg2;
        uint16_t arg3;
        std::string& tostring();
    };
    std::vector<sf::Texture> frame;
    std::vector<cutscene::cut_cmd> cmd;
    uint32_t frame;
    int fade_level;
    int fade_target;
    int fade_speed;
};
