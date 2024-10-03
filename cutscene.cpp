#include "cutscene.h"      //Cutscene direction data (*.n00)
#include "lpfcut.h"        //Cutscene graphics data (*.n[01][^0])
#include "audio/vocfile.h" //Digital audio
#include "UwText.h"        //Text strings
#include "uwfont.h"        //Underworld font importer
#include "util.h"
#include "audio/audioManager.h"
#include<iostream>
#include<fstream>

cutscene::cutscene(bool subs/*=true*/, bool audio/*=true*/): cur_n0x(1), cur_frame(0), fade_level(0), fade_target(0), fade_speed(0), with_subs(subs), with_vocs(audio) {

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
    cur_frame = 1;
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

void cutscene::play_voc(int number) {
    while(cur_sound.getStatus() == sf::SoundSource::Playing) {
        sf::sleep(sf::milliseconds(100));
    }
    cur_sound.stop();
    std::string numstr = std::to_string(number);
    if(number < 10) {
        numstr = std::string("0")+numstr;
    }
    std::vector<int16_t> snd_dat = vocfile::get_file_dat(std::string(base_dir+"/sound/"+numstr+".voc"));
    //cur_sb.loadFromFile(std::string(base_dir+"/sound/"+std::to_string(number)+".voc"));
    cur_sb.loadFromSamples(&snd_dat[0], snd_dat.size(),1,12048);
    cur_sound.setBuffer(cur_sb);
    cur_sound.play();
}

std::string cutscene::format_string(std::string& input) {
    std::string output(input);
    sf::Text txt = sf::Text(output, cs_font, 10);
    int width = txt.getLocalBounds().width;
    //300, because I'd rather err toward too many lines than too few, so I'll pretend the screen is a little smaller.
    int line_breaks = width / 320;
    //std::cout<<"Width: "<<width<<" Breaks: "<<line_breaks;
    if(line_breaks == 0) return output;
    //int spaces = std::count(output.begin(), output.end(), ' ');
    //size_t space_skip = spaces/(line_breaks+1) + 1;
    size_t char_skip = output.size() / (line_breaks + 1) + 1;
    //std::cout<<"Char skip: "<<char_skip<<std::endl;
    //size_t pos = 0;
    size_t split_loc = char_skip;
    for(size_t i = 0; i < line_breaks; i++) {
        size_t posr = output.find(' ', split_loc);
        size_t posl = output.substr(split_loc).rfind(' ');
        size_t pos = 0;
        assert((posl > 0 && posl < output.size())||posl == std::string::npos);
        assert((posr > 0 && posr < output.size())||posr == std::string::npos);
        if(((split_loc - posl) < (posr - split_loc) || posr == std::string::npos) && posl != std::string::npos ) {
            output = output.replace(posl, 1, 1, '\n');
            pos = posl;
        }
        else if(posr != std::string::npos) {
            output = output.replace(posr, 1, 1, '\n');
            pos = posr;
        }
        else {
            break;
            //std::cout<<"Flummoxed by the string: \""<<output<<"\""<<std::endl;
        }
        split_loc += char_skip;
    }
    return output;
} 

void cutscene::play(sf::RenderWindow& screen) {
    audioManager am(base_dir+"/sound/uw.ad","","");
    am.playMusic(base_dir + "/sound/aw01.xmi");
    sf::Sprite spr;
    spr.scale(1,1.2);
    sf::Text txt("", cs_font, 10);
    int prev_frame = 1;
    int fps = 5;
    for(int i=0; i<cmd.size();i++) {
        sf::Event event;
        while(screen.pollEvent(event)) {
            switch(event.type) {
            case sf::Event::Closed:
                screen.close();
                return;
            }
        }

        screen.setFramerateLimit(5);
        fps = 5;
        cur_frame = cmd[i].frame;
        std::cout<<cmd[i].tostring()<<std::endl;
        if(prev_frame < cur_frame) {
            std::cout<<"Playing from frame "<<prev_frame<<" to frame "<<cur_frame<<std::endl;
            for(int f = prev_frame;f<cur_frame;++f) {
                //assert(f > 0);
                //assert(frame[f-1].getSize() == sf::Vector2u(320,200));
                if(f > 0) {
                    screen.clear();
                    spr.setTexture(frame[f - 1]);
                    screen.draw(spr);
                    screen.draw(txt);
                    screen.display();
                }
            }
        }
        if(cmd[i].cmd_num != 1) {
            prev_frame = cur_frame;
        }
        switch(cmd[i].cmd_num) {
            case 0x00: //Display text arg[1] with color arg[0]
                {
                    assert(cmd[i].args[1] < strings.size());
                    std::string temp_string = format_string(strings[cmd[i].args[1]]);
                    txt = sf::Text(temp_string, cs_font, 10);
                    //txt.setColor(cur_lpf_pal[cmd[i].args[0]]);
                    txt.setFillColor(cur_lpf_pal[cmd[i].args[0]]);
                    int txtw = (320 - txt.getLocalBounds().width)/2;
                    int txth = 240 - txt.getLocalBounds().height - 10;
                    txt.setPosition(txtw,txth);
                }
                break;
            case 0x03: //Pause for arg[0]/2 seconds
                if(cur_frame > 0) {
                    screen.clear();
                    spr.setTexture(frame[cur_frame - 1]);
                    screen.draw(spr);
                    screen.draw(txt);
                    screen.display();
                }
                sf::sleep(sf::milliseconds(cmd[i].args[0]*500));
                break;
            case 0x04: //Play up to frame arg[0] (at rate arg[1]? Doesn't seem necessary)
                for(;cur_frame<cmd[i].args[0]-1;++cur_frame) {
                    if(cur_frame  > 0 && cur_frame<= frame.size()) {
                        spr.setTexture(frame[cur_frame-1]);
                    }
                    screen.clear();
                    screen.draw(spr);
                    screen.draw(txt);
                    screen.display();
                }
                prev_frame = cur_frame;
                break;
            case 0x05: //Probably related to "static" cutscene settings
                break;
            case 0x06: //End of cutscene
                return;
            case 0x07: //Repeat a section of animation arg[0] times
                for(int rep=0;rep<cmd[i].args[0];++rep) {
                    for(int rep_frame=0;rep_frame < cmd[i].frame-1;++rep_frame) {
                        if(cur_frame - 1 >= 0 && cur_frame - 1 < frame.size()) {
                            spr.setTexture(frame[rep_frame]);
                        }
                        screen.draw(spr);
                        screen.draw(txt);
                        screen.display();
                    }
                }
                break;
            case 0x08: //Switch to cs{args[0]}.n{args[1]} for playback
                {
                    std::string n0x_file(base_dir+"/cuts/cs"+to_octal3(cmd[i].args[0])+".n"+to_octal2(cmd[i].args[1]));
                    if(!load_lpf(n0x_file)) {
                        std::cerr<<"Couldn't load "<<n0x_file<<", even though I was explicitly told to."<<std::endl;
                    }
                    cut_num = cmd[i].args[0];
                    cur_n0x = cmd[i].args[1];
                    cur_frame = 1;
                    spr.setTexture(frame[cur_frame - 1]);
                    txt = sf::Text("", cs_font, 10);
                }
                break;
            case 0x09: //Fade out
                {
                    screen.setFramerateLimit(45);
                    sf::RectangleShape fade(sf::Vector2f(screen.getSize()));
                    if(cur_frame - 1 >= 0 && cur_frame - 1 < frame.size()) {
                        spr.setTexture(frame[cur_frame-1]);
                    }
                    int rate = 256/(16*cmd[i].args[0]+1);
                    for(int opacity=0;opacity < 255; opacity+=rate) {
                        fade.setFillColor(sf::Color(0,0,0,opacity));
                        screen.draw(spr);
                        screen.draw(txt);
                        screen.draw(fade);
                        screen.display();
                    }
                    txt = sf::Text("", cs_font, 10);
                    screen.setFramerateLimit(fps);
                }
                break;
            case 0x0a: //Fade in
                {
                    screen.setFramerateLimit(45);
                    sf::RectangleShape fade(sf::Vector2f(screen.getSize()));
                    if(cur_frame - 1 >= 0 && cur_frame - 1 < frame.size()) {
                        spr.setTexture(frame[cur_frame-1]);
                    }
                    int rate = 256/(16*(cmd[i].args[0]+1));
                    for(int opacity=255;opacity >= 0; opacity-=rate) {
                        fade.setFillColor(sf::Color(0,0,0,opacity));
                        screen.draw(spr);
                        screen.draw(txt);
                        screen.draw(fade);
                        screen.display();
                    }
                    screen.setFramerateLimit(fps);
                }
                break;
            case 0x0d: //Display text arg[1] with color arg[0] and play sound arg[2]
                if(with_subs) {
                    assert(cmd[i].args[1] < strings.size());
                    std::string temp_string = format_string(strings[cmd[i].args[1]]);
                    txt = sf::Text(temp_string, cs_font, 10);
                    txt.setFillColor(cur_lpf_pal[cmd[i].args[0]]);
                    //txt.setColor(cur_lpf_pal[cmd[i].args[0]]);
                    int txtw = (320 - txt.getLocalBounds().width)/2;
                    int txth = 240 - txt.getLocalBounds().height - 10;
                    txt.setPosition(txtw,txth);
                    std::cout<<"Set text position to "<<txtw<<", "<<txth<<std::endl;
                }
                if(with_vocs) {
                    play_voc(cmd[i].args[2]);
                }
                break;
            case 0x0e: //Pause for arg[0] if audio is on, arg[1] if audio is off
                if(with_vocs) {
                    sf::sleep(sf::milliseconds(cmd[i].args[0]*500));
                }
                else {
                    sf::sleep(sf::milliseconds(cmd[i].args[1]*1000));
                }
                break;
            case 0x0f: // Play "klang" sound effect
                am.playSfx(3);
                break;
            default:
                std::cout<<"Not implemented: "<<cmd[i].tostring()<<std::endl;
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
    return std::string("empty bo-bempty");
}

#ifdef STAND_ALONE_CS
int main(int argc, char *argv[]) {
    //2 args: directory containing game data (cuts+sound), and a cutscene number.
    sf::RenderWindow window(sf::VideoMode(320, 240), "SFML window");
    cutscene c;
    if(!c.load(argv[1], std::stoi(argv[2]))) {
        std::cerr<<"Failed to load files using path "<<argv[1]<<std::endl;
    }
    c.play(window);
}
#endif
