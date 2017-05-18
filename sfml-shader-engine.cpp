#include<iostream>
#include<cstdint>
#include<GL/glew.h>
#include<glm/glm.hpp>
#include<SFML/System.hpp>
#include<SFML/Window.hpp>
#include<SFML/Graphics.hpp>
#include<SFML/OpenGL.hpp>
#include "glutil.h"
using namespace std;

void draw(GLuint vertexbuffer, GLuint programID);

int main() {                         //32-bit depth, 0-bit stencil, level-0 anti-aliasing, GL version 3.3, compatibility profile
    sf::RenderWindow window(sf::VideoMode(640, 480), "UW shader pipeline renderer", sf::Style::Default, sf::ContextSettings(32,0,0,3,3, sf::ContextSettings::Default));
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

    static const GLfloat g_vertex_buffer_data[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

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
        draw(vertexbuffer, programID);
        window.display();
    }
}

void draw(GLuint vertexbuffer, GLuint programID) {
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
    glDrawArrays(GL_TRIANGLES, 0, 3); //Draw GL_TRIANGLES, start with vertex 0, draw 3 total
    glDisableVertexAttribArray(0);
}
