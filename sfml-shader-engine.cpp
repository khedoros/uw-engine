#include<iostream>
#include<fstream>
#include<cstdint>
#include<GL/glew.h>
#include<glm/glm.hpp>
#include<SFML/System.hpp>
#include<SFML/Window.hpp>
#include<SFML/Graphics.hpp>
#include<SFML/OpenGL.hpp>
#include<vector>
#include "glutil.h"
#include "uw_model.h"
using namespace std;

void draw(GLuint vertexbuffer, int triangle_num, GLuint programID);

int main(int argc, char **argv) {
    if(argc != 2) {
        std::cerr<<"I need the path to a UW executable"<<std::endl;
        return 1;
    }
    uw_model car;
    bool loaded = car.load(argv[1], 4);
    if(!loaded) {
        std::cerr<<"Couldn't load the model from "<<argv[1]<<". Aborting."<<std::endl;
        return 1;
    }

    std::vector<float> model_verts = car.get_verts();
    if(model_verts.size() % 3 != 0) {
        std::cerr<<"Bad number of verts ("<<model_verts.size()<<"). Aborting."<<std::endl;
        return 1;
    }
    int triangle_num = model_verts.size() / 3;
                                                                         //32-bit depth, 0-bit stencil, level-0 anti-aliasing, GL version 3.0, compatibility profile
    sf::RenderWindow window(sf::VideoMode(640, 480), "UW shader pipeline renderer", sf::Style::Default, sf::ContextSettings(32,0,0,3,0, sf::ContextSettings::Default));
    window.setKeyRepeatEnabled(false);
    window.setFramerateLimit(20);

    glewExperimental = GL_TRUE;
    if(glewInit() != GLEW_OK) {
        std::cerr<<"Failed to init GLEW"<<std::endl;
        return 1;
    }

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    glClearColor (0.0, 0.0, 0.4, 0.0);
    glClearDepth (1.0);
    glClearStencil(0);

    /*
    static const GLfloat g_vertex_buffer_data[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    */

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    /*
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    */
    glBufferData(GL_ARRAY_BUFFER, model_verts.size() * sizeof(float), &model_verts[0], GL_STATIC_DRAW);

    GLuint programID = glutil::LoadShaders("shaders/vertex-basic-33.glsl", "shaders/fragment-basic-33.glsl");
    if(programID == 0) {
        std::cerr<<"Shader processing for version 3.3 failed. Trying to fall back to 2.1."<<std::endl;
        programID = glutil::LoadShaders("shaders/vertex-basic-21.glsl", "shaders/fragment-basic-21.glsl");
        if(programID == 0) {
            std::cerr<<"Shader processing for version 2.1 failed. Aborting."<<std::endl;
            return 1;
        }
    }

    while(window.isOpen()) {
        sf::Event event;
        while(window.pollEvent(event)) {
            switch(event.type)
            case sf::Event::Closed:
                window.close();
                break;
        }

        //update game state
        //draw stuff
        draw(vertexbuffer, triangle_num, programID);
        window.display();
    }
}

void draw(GLuint vertexbuffer, int triangle_num, GLuint programID) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(programID);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
            0,        //attrib number
            3,        //element size
            GL_FLOAT,
            GL_FALSE, //normalized?
            0,        //stride
            (void*)0  //offset in the buffer
        );
    glDrawArrays(GL_TRIANGLES, 0, triangle_num); //Draw GL_TRIANGLES, start with vertex 0, draw "triangle_num" total
    glDisableVertexAttribArray(0);
}
