#pragma once
#include<string>
#include<GL/glew.h>
#include<GL/gl.h>
#include<GL/glu.h>
GLuint LoadShaders(const std::string& vertex_file_path, const std::string& fragment_file_path);
