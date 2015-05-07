#include<iostream>
#include<GL/glew.h>
#include<GL/glut.h>

int main(int argc, char **argv) {
    GLfloat pt_max_size = 0.0, pt_min_size = 0.0;
    glutInit(&argc, argv);
    glutCreateWindow("GLEW Test");
    GLenum err = glewInit();
    if(err != GLEW_OK) {
        std::cout<<"Some problem init'ing GLEW. Bummer."<<std::endl;
        return 1;
    }
    glGetFloatv(GL_POINT_SIZE_MAX_ARB, &pt_max_size);
    glGetFloatv(GL_POINT_SIZE_MIN_ARB, &pt_min_size);
    std::cout<<"Max size: "<<pt_max_size<<" Min size: "<<pt_min_size<<std::endl;
    glEnable(GL_POINT_SPRITE);
    return 0;
}
