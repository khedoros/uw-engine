#include<iostream>
#include<cstdint>
#include<GL/glew.h>
#include<glm/glm.hpp>
#include<SFML/System.hpp>
#include<SFML/Window.hpp>
#include<SFML/Graphics.hpp>
#include<SFML/OpenGL.hpp>
using namespace std;

void draw(GLuint vertexbuffer);

int main() {                         //32-bit depth, 0-bit stencil, level-0 anti-aliasing, GL version 3.3, compatibility profile
    sf::RenderWindow window(sf::VideoMode(640, 480), "UW shader pipeline renderer", sf::Style::Default, sf::ContextSettings(32,0,0,3,3));
    window.setKeyRepeatEnabled(false);
    window.setFramerateLimit(20);

    if(glewInit() != GLEW_OK) {
        std::cerr<<"Failed to init GLEW"<<std::endl;
        return 1;
    }

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    glClearColor (0.0, 0.0, 0.0, 0.0);
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

    while(window.isOpen()) {
        sf::Event event;
        while(window.pollEvent(event)) {
            switch(event.type)
            case sf::Event::Closed:
                window.close();
                break;
        }

        //update game state
        window.clear();
        //draw stuff
        draw(vertexbuffer);
        window.display();
    }
    //cout<<"Completely unimplemented yet"<<endl;
}

void draw(GLuint vertexbuffer) {
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
