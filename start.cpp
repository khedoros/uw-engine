#include<iostream>
#include<string>
#include<SFML/Graphics.hpp>
#include "cutscene.h"
#include "texfile.h"
#include "audio/opl_music.h"

#include<assert.h>


void handle_events(sf::RenderWindow& window, opl_music& music) {
    sf::Keyboard::Key key=sf::Keyboard::Unknown;
    sf::Event event;
    while(window.pollEvent(event)) {
        if(event.type == sf::Event::Closed) {
            music.stop();
            window.close();
        }
        else if(event.type == sf::Event::KeyPressed) {
            key=event.key.code;
            switch(key) {
                case sf::Keyboard::Q:
                    window.close();
                    music.stop();
                    break;
                default: break;
            }
        }
    }
}

int main() {
    //std::cout<<"Trying to open a 320x240@32 window."<<std::endl;
    sf::RenderWindow window(sf::VideoMode(320,240,32), "Ultima Underworld Opening Test");
    if(window.isOpen()) {
        std::cout<<"Window opened successfully!"<<std::endl;
    } else {
        std::cerr<<"Window could not be opened."<<std::endl;
    }
    opl_music music;
    music.load("../cd/uw/sound/uw.ad", "../cd/uw/sound/aw01.xmi");
    //std::cout<<"Loaded music"<<std::endl;
    sf::Clock c;
    c.restart();
    //std::cout<<"Restarted clock"<<std::endl;

    texfile oscreens;
    oscreens.load("../cd/uw/data/pals.dat", "../cd/uw/data/pres1.byt");
    //std::cout<<"Loaded pres1"<<std::endl;
    sf::Sprite sprite;
    sprite.setTexture(oscreens.tex[0]);
    //std::cout<<"Set pres1 as tex"<<std::endl;
    assert(oscreens.xres == 320 && oscreens.yres == 200);
    sprite.scale(1.0,1.2);
    window.setFramerateLimit(5);
    //std::cout<<"Set fps limit"<<std::endl;
    window.clear(sf::Color(0,0,0,255));
    while(c.getElapsedTime().asSeconds() < 5) {
        //std::cout<<"Currently waited "<<c.getElapsedTime().asMilliseconds()<<" ms"<<std::endl;
        window.draw(sprite);
        handle_events(window,music);
        window.display();
    }
    //sf::sleep(sf::seconds(5));
    music.play();
    //std::cout<<"started music playback"<<std::endl;
    oscreens.load("../cd/uw/data/pals.dat", "../cd/uw/data/pres2.byt");
    sprite.setTexture(oscreens.tex[0]);
    //std::cout<<"Loaded second texture, and set sprite to use it"<<std::endl;
    window.clear(sf::Color(0,0,0,255));
    c.restart();
    while(c.getElapsedTime().asSeconds() < 5) {
        window.draw(sprite);
        handle_events(window,music);
        window.display();
    }
    //sf::sleep(sf::seconds(5));
    cutscene cut;
    cut.load("../cd/uw/", 9);
    cut.play(window);
    handle_events(window,music);
    cut.load("../cd/uw/", 0);
    cut.play(window);

    return 0;
}
