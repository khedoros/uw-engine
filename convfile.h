#ifndef CONVFILE_H
#define CONVFILE_H

#include<string>
#include<vector>

class convfile {
public:
    convfile() {}
    bool load(std::string& cnvfile, std::string& globfile);
    bool load_cnv(std::string& cnvfile);
    bool load_globals(std::string& globfile);

    class conversation {
    public:
        std::vector<uint16_t> code;
        uint16_t string_block;
        uint16_t var_slots;
        std::vector<std::string> functs;
        std::vector<std::string> vars;
    };

    std::vector<conversation> convos;
};

class convemu {

};
#endif
