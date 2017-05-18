#pragma once
#include<string>
#include<GL/glew.h>
//#include<GL/gl.h>
#include<SFML/OpenGL.hpp>
#include<GL/glu.h>

namespace glutil {
    GLuint LoadShaders(const std::string& vertex_file_path, const std::string& fragment_file_path);
}
