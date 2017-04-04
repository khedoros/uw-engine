#pragma once
#include<string>
#include<vector>

class convfile {
public:
    convfile() {}
    bool load(std::string& cnvfile);

    class conversation {
    public:
        std::vector<int16_t> code;
        uint16_t string_block;
        uint16_t var_slots;
        std::vector<std::string> functs;
        std::vector<std::string> vars;
    };

    std::vector<conversation> convos;
};

class convemu {

};
