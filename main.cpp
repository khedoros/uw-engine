#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
//#include "palette.h"
#include "lpfcut.h"
#include "audio/vocfile.h"

void play_lpf(sf::RenderWindow *, const char *, sf::SoundBuffer * wait_for_done=NULL);
void play_snd(sf::Sound * snd, sf::SoundBuffer * sb, bool blocking=true);

int main(int argc, char *argv[])
{
    Lpfcut cs_file;
    sf::SoundBuffer sb_file;
    if(argc==2) {
        if(cs_file.load(argv[1])) { //Try to load the file as a cutscene, play it if it worked
            std::cout<<"Opened file as cutscene file."<<std::endl;
            // Create the main window
            int rwh,rww;
            if(cs_file.width == 320 && cs_file.width == 200) {
                rww = 320;
                rwh = 240;
            }
            else {
                rww = cs_file.width;
                rwh = cs_file.height;
            }

            sf::RenderWindow App(sf::VideoMode(rww, rwh), "LPF Cutscene Player");
            //sf::Image my_img;
            sf::Texture my_tex;
            my_tex.create(cs_file.width,cs_file.height);
            sf::Sprite spr;
            spr.setTexture(my_tex);
            spr.scale(1.0,1.2);

            bool update_frame = true;

            // Start the game loop
            while (App.isOpen()) {
                // Process events
                sf::Event Event;
                while (App.pollEvent(Event)) {
                    // Close window : exit
                    switch(Event.type) {
                    case sf::Event::Closed:
                        App.close();
                        break;
                    case sf::Event::GainedFocus:
                    case sf::Event::Resized:
                        App.draw(spr);
                        App.display();
                        //redraw = true;
                        break;
                    case sf::Event::KeyPressed:
                        if(Event.key.code == sf::Keyboard::Q)
                            App.close();
                        else
                            update_frame = true;
                        break;
                    default:
                        //potatos
                        break;
                    }
                }
                //App.clear(sf::Color::Black);
                if(update_frame) {
                    sf::Uint8 * frame = (sf::Uint8 *)cs_file.getNextFrame();
                    if(frame==NULL) {
                        std::cout<<"The frame looks null.\n";
                    }
                    //my_img.create(cs_file.width, cs_file.height, frame);
                    my_tex.update(frame);
                    //spr.setTexture(my_tex);
                    //my_tex.loadFromImage(my_img);
                    //spr.setTexture(my_tex);
                    App.draw(spr);
                    App.display();
                    update_frame = false;
                }
                sf::Time t1 = sf::milliseconds(1000/5);
                sf::sleep(t1);
            }
        }
        else if(sb_file.loadFromFile(argv[1])) { //Try to load the file as an audio file
            std::cout<<"Opened file as audio file."<<std::endl;
            sf::Sound snd(sb_file);
            sf::Time t = sb_file.getDuration();
            snd.play();
            sf::sleep(t);
        }
        else if(vocfile::check(argv[1])) {
            std::string filename = std::string(argv[1]);
            std::vector<int16_t> samples = vocfile::get_file_dat(filename);
            sb_file.loadFromSamples(&samples[0], samples.size(),1,12048);
            sf::Sound snd(sb_file);
            sf::Time t = sb_file.getDuration();
            snd.play();
            sf::sleep(t);
        }
    }
    else if(argc==1) {//No args
        std::cout<<"No args? Trying to play the opening cutscene."<<std::endl;
        sf::Sound snd;
        sf::SoundBuffer sb[66];
        for(int i=0;i<65;++i) {
            char * buffer=new char[4];
            std::snprintf(buffer, 3, "%02d",i);
            std::string filename(std::string("./sound/") + buffer + std::string(".voc"));
            std::ifstream filetest(filename);
            if(filetest) {
                filetest.close();
                std::vector<int16_t> samples = vocfile::get_file_dat(filename);
                sb[i].loadFromSamples(&samples[0], samples.size(),1,12048);
            }
            delete[] buffer;
        }

        sf::RenderWindow App(sf::VideoMode(320, 240), "LPF Cutscene Player");

        play_lpf(&App, "./cuts/cs000.n01");
        play_snd(&snd, &sb[26]);
        play_snd(&snd, &sb[27]);

        play_snd(&snd, &sb[28], false);
        play_lpf(&App, "./cuts/cs000.n02");
        play_lpf(&App, "./cuts/cs000.n02");

        play_snd(&snd, &sb[23], false);
        play_lpf(&App, "./cuts/cs000.n03");
        play_snd(&snd, &sb[24], false);
        play_lpf(&App, "./cuts/cs000.n04");
        play_snd(&snd, &sb[25], false);
        play_lpf(&App, "./cuts/cs000.n03");

        play_snd(&snd, &sb[30], false);
        play_lpf(&App, "./cuts/cs000.n07");

        return 0;


        play_lpf(&App, "./cuts/cs000.n10");
        play_lpf(&App, "./cuts/cs000.n11");
        play_lpf(&App, "./cuts/cs000.n12");
        play_lpf(&App, "./cuts/cs000.n13");
        play_lpf(&App, "./cuts/cs000.n14");
        play_lpf(&App, "./cuts/cs000.n15");
        play_lpf(&App, "./cuts/cs000.n16");
        play_lpf(&App, "./cuts/cs000.n17");
        play_lpf(&App, "./cuts/cs000.n20");
        play_lpf(&App, "./cuts/cs000.n21");
        play_lpf(&App, "./cuts/cs000.n22");
        play_lpf(&App, "./cuts/cs000.n23");
        play_lpf(&App, "./cuts/cs000.n24");
        play_lpf(&App, "./cuts/cs000.n25");
    }
    else {
        std::cout<<"Please provide a media file as an argument."<<std::endl;
        return EXIT_FAILURE;
    }



    return EXIT_SUCCESS;
}

void play_lpf(sf::RenderWindow * App, const char * filename, sf::SoundBuffer * wait_for_done ) {
        Lpfcut cs;
        cs.load(std::string(filename));
        cs.repeat=false;
        sf::Texture my_tex;
        my_tex.create(cs.width,cs.height);
        sf::Sprite spr;
        spr.setTexture(my_tex);
        sf::Uint8 * frame = (sf::Uint8 *)cs.getNextFrame();
        while(frame!=NULL) {
            my_tex.update(frame);
            App->draw(spr);
            App->display();
            sf::Time t1 = sf::milliseconds(1000/10);
            sf::sleep(t1);
            frame = (sf::Uint8 *)cs.getNextFrame();
        }
}

void play_snd(sf::Sound * snd, sf::SoundBuffer * sb, bool blocking) {
    snd->setBuffer(*sb);
    snd->play();
    if(blocking) {
        sf::Time t=sb->getDuration();
        sf::sleep(t+sf::milliseconds(500));
    }
    return;
}
