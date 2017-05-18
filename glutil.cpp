#include "glutil.h"
#include<iostream>
#include<fstream>
#include<vector>

GLuint glutil::LoadShaders(const std::string& vertex_file_path, const std::string& fragment_file_path) {
    GLuint vsid = glCreateShader(GL_VERTEX_SHADER);
    GLuint fsid = glCreateShader(GL_FRAGMENT_SHADER);
    std::string vsCode;
    std::ifstream vsStream(vertex_file_path.c_str(), std::ios::in);
    if(vsStream.is_open()) {
        std::string Line = "";
        while(getline(vsStream, Line)) {
            vsCode += "\n" + Line;
        }
        vsStream.close();
    } else {
        std::cerr<<"Impossible to open "<<vertex_file_path<<". Check file path."<<std::endl;
        return 0;
    }

    std::string fsCode;
    std::ifstream fsStream(fragment_file_path.c_str(), std::ios::in);
    if(fsStream.is_open()) {
        std::string Line = "";
        while(getline(fsStream, Line)) {
            fsCode += "\n" + Line;
        }
        fsStream.close();
    } else {
        std::cerr<<"Impossible to open "<<fragment_file_path<<". Check file path."<<std::endl;
        return 0;
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;


    // Compile Vertex Shader
    std::cout<<"Compiling shader : "<<vertex_file_path<<" ... ";
    char const * VertexSourcePointer = vsCode.c_str();
    glShaderSource(vsid, 1, &VertexSourcePointer , NULL);
    glCompileShader(vsid);

    // Check Vertex Shader
    glGetShaderiv(vsid, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(vsid, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 1 ){
        std::cerr<<"error. (log length: "<<InfoLogLength<<")"<<std::endl;
        std::vector<char> vsErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(vsid, InfoLogLength, NULL, &vsErrorMessage[0]);
        std::cerr<<std::endl<<&vsErrorMessage[0]<<std::endl;
        return 0;
    }
    else {
        std::cout<<"compiled."<<std::endl;
    }



    // Compile Fragment Shader
    std::cout<<"Compiling shader : "<<fragment_file_path.c_str()<<" ... ";
    char const * FragmentSourcePointer = fsCode.c_str();
    glShaderSource(fsid, 1, &FragmentSourcePointer , NULL);
    glCompileShader(fsid);

    // Check Fragment Shader
    glGetShaderiv(fsid, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(fsid, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 1 ){
        std::cerr<<"error. (log length: "<<InfoLogLength<<")"<<std::endl;
        std::vector<char> fsErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(fsid, InfoLogLength, NULL, &fsErrorMessage[0]);
        std::cerr<<std::endl<<&fsErrorMessage[0]<<std::endl;
        return 0;
    }
    else {
        std::cout<<"compiled."<<std::endl;
    }


    // Link the program
    std::cout<<"Linking program"<<" ... ";
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, vsid);
    glAttachShader(ProgramID, fsid);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 1 ){
        std::cerr<<"error. (log length: "<<InfoLogLength<<")"<<std::endl;
        std::vector<char> ProgramErrorMessage(InfoLogLength+1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        std::cerr<<std::endl<<&ProgramErrorMessage[0]<<std::endl;
        return 0;
    }
    else {
        std::cout<<"linked."<<std::endl;
    }

    glDetachShader(ProgramID, vsid);
    glDetachShader(ProgramID, fsid);

    glDeleteShader(vsid);
    glDeleteShader(fsid);

    return ProgramID;
}
