#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include<stdio.h>
#include<time.h>
#include<assert.h>
#include<math.h>
#include<string>

#include "simple_map.h"

//typedef enum { false, true }  bool;
//int mode = 9;
//float mode0 = 0.0;
//float mode1 = 0.0;

//width and height of window
int xres = 1000;
int yres = 1000;

//Is mouse captured?
bool capMouse = false;
//Are we in the process of warping the pointer?
bool warping = false;

//Rotation of the X and Y axes in degrees
float viewRotX = -36.0;
float viewRotY = -25.0;

//Rotation of the X and Y axes in radians
float xRad = 0.0;
float yRad = 0.0;

//(X, Y, Z) position of the camera
float xPos = -5.0;
float yPos = -5.0;
float zPos = -8.0;

//(X, Y, Z) vector describing where the camera is pointing
float xComp = 0.0;
float yComp = 0.0;
float zComp = 0.0;

unsigned long count = 1;

void kb(unsigned char key, int x, int y) {
    switch(key) {
    case 9: //tab
        capMouse = (capMouse)?false:true;
        //printf("CapMouse: %d\n",capMouse);
        if(capMouse) {
            glutWarpPointer(xres/2,yres/2);
            glutSetCursor(GLUT_CURSOR_NONE);
        } else {
            glutSetCursor(GLUT_CURSOR_INHERIT);
        }
        break;
    case 32: //space
        yComp+=1.0;
        break;
    case 97: //a
        xComp += -1.0 * cos(xRad) * cos(yRad);
        yComp += 0; //-1.0 * sin(yRad);
        zComp += sin(xRad) * cos(yRad);
        break;
    case 100: //d
        xComp += cos(xRad) * cos(yRad);
        yComp += 0; //sin(yRad);
        zComp += -1.0 * sin(xRad) * cos(yRad);
        break;
    case 105: //i
        viewRotY += (((float)(1.0)));
        if(viewRotY > 90.0) viewRotY = 90.0;
        if(viewRotY < -90.0) viewRotY = -90.0;
        yRad = viewRotY * M_PI / 180.0;
        break;
    case 106: //j
        viewRotX -= (((float)(1.0)));
        if(viewRotX > 360.0) viewRotX -= 360.0;
        if(viewRotX < -360.0) viewRotX += 360.0;
        xRad = viewRotX * M_PI / 180.0;
        break;
    case 107: //k
        viewRotY -= (((float)(1.0)));
        if(viewRotY > 90.0) viewRotY = 90.0;
        if(viewRotY < -90.0) viewRotY = -90.0;
        yRad = viewRotY * M_PI / 180.0;
        break;
    case 108: //l
        viewRotX += (((float)(1.0)));
        if(viewRotX > 360.0) viewRotX -= 360.0;
        if(viewRotX < -360.0) viewRotX += 360.0;
        xRad = viewRotX * M_PI / 180.0;
        break;
    case 113: //q
        exit(0);
    case 115: //s
        xComp = -1.0 * sin(xRad) * cos(yRad);
        yComp = -1.0 * sin(yRad);
        zComp = -1.0 * cos(xRad) * cos(yRad);
        break;
    case 119: //w
        xComp = sin(xRad) * cos(yRad);
        yComp = sin(yRad);
        zComp = cos(xRad) * cos(yRad);
        break;
    default: printf("key: %d\n",(int)key); break;
    }
}

void mouse(int x, int y) {
   if(x == xres/2 && y == yres/2) {/*printf("x and y are centered. Why am I here?\n");*/ return;}
   if(capMouse) {// && x != oldx && y != oldy) {
      //printf("capMouse!\n");
      viewRotX += (((float)(x - xres/2)) * 0.05);
      viewRotY += (((float)(yres/2 - y)) * 0.05);
      if(viewRotY > 90.0) viewRotY = 90.0;
      if(viewRotY < -90.0) viewRotY = -90.0;
      if(viewRotX > 360.0) viewRotX -= 360.0;
      if(viewRotX < -360.0) viewRotX += 360.0;
      xRad = viewRotX * M_PI / 180.0;
      yRad = viewRotY * M_PI / 180.0;

      //warping = true;
      //printf("Warp Start\n");
      glutWarpPointer(xres/2,yres/2);
      //warping = false;
      //printf("Warp done\n");
      //glutMotionFunc(mouse);
      //glutPassiveMotionFunc(mouse);
  //    oldx = x; oldy = y;
   }
}
/*
uint32_t xs = 0;
uint32_t ys = 0;
bool saved = false;
*/
void click(int button, int state, int x, int y) {
    uint32_t s = 0;
    glReadPixels(x, yres - y - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &s);
    std::string col = "";
    switch(s) {
    case 0: col = "black";break;
    case 1: col = "blue";break;
    case 2: col = "red";break;
    case 3: col = "green";break;
    case 4: col = "color_square";break;
    default: col = "lolwut";break;
    }
    std::cout<<"Saw "<<col<<" at ("<<x<<", "<<y<<")."<<std::endl;
    //xs = x; ys = y; saved = true;
}

void init(void) 
{
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glClearDepth (1.0);
   glClearStencil(0);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_STENCIL_TEST);
   glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
   glShadeModel (GL_SMOOTH);
}

void display(void)
{
   struct timespec start, end, sleeptime;
   clock_gettime(CLOCK_MONOTONIC, &start); //Start frame-limit timer
   glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
   glLoadIdentity ();             /* clear the matrix */

   xPos += xComp * -0.1;
   yPos += yComp * -0.1;
   zPos += zComp * 0.1;

   //printf("Delta (%f, %f, %f), facing: %f, looking: %f\n",xComp,yComp,zComp, viewRotX, viewRotY);

   xComp = 0; yComp = 0; zComp = 0;

   glRotatef(viewRotX, 0.0, 1.0, 0.0); //Rotate perspective around Y axis
   glRotatef(-1.0 * viewRotY, cos(viewRotX*M_PI/180.0), 0.0, sin(viewRotX*M_PI/180.0)); //Rotate pespective around camera's X axis
   
   glTranslatef(xPos, yPos, zPos); //Translate the world to where the camera expects it to be
   //std::cout<<"("<<xPos<<", "<<yPos<<", "<<zPos<<")    ("<<viewRotX<<", "<<viewRotY<<")"<<std::endl;

   glPushMatrix();
      glTranslatef(0.0,0.0,5);
      glScalef(0.5,0.5,10.0);
      glColor3f(0.0,0.0,1.0); //Z Axis
      glStencilFunc(GL_ALWAYS, 1, -1);
      glutSolidCube (1.0);
   glPopMatrix();

   glPushMatrix();
      glTranslatef(5.0,0.0,0.0);
      glScalef(10.0,0.5,0.5);
      glColor3f(1.0, 0.0, 0.0); //X Axis
      glStencilFunc(GL_ALWAYS, 2, -1);
      glutSolidCube(1.0);
   glPopMatrix();

   glPushMatrix();
      glTranslatef(0.0,5.0,0.0);
      glScalef(0.5,10.0,0.5);
      glColor3f(0.0, 1.0, 0.0); //Y Axis
      glStencilFunc(GL_ALWAYS, 3, -1);
      glutSolidCube(1.0);
   glPopMatrix();

   //Approximating display of a sprite (matched to rotation of camera, but the normal doesn't always point at the camera, like with a true sprite)
   glPushMatrix();
      glTranslatef(3.0,3.0,3.0); //Move object where you want it to show up
      glRotatef(-1.0 * viewRotX, 0.0, 1.0, 0.0); //Rotate around y to face camera
      glRotatef(viewRotY, 1.0, 0.0, 0.0); //Rotate around model's X axis to face camera
      //Define quad in modelspace
      glStencilFunc(GL_ALWAYS, 4, -1);
      glBegin(GL_QUADS);
         glColor3f(0.0,0.0,0.0); glVertex3f(0.0,0.0,0.0);
         glColor3f(1.0,0.0,1.0); glVertex3f(1.0,0.0,0.0);
         glColor3f(1.0,1.0,1.0); glVertex3f(1.0,1.0,0.0);
         glColor3f(0.0,1.0,1.0); glVertex3f(0.0,1.0,0.0);
      glEnd();
   glPopMatrix();

   glFlush ();
   glutPostRedisplay();
/*
   if(saved) {
       GLuint s = 5;
       glReadPixels(xs, yres - ys - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &s);
       std::cout<<"Saw "<<s<<" at ("<<xs<<", "<<ys<<")."<<std::endl;
       saved = false;
   }
 */
   clock_gettime(CLOCK_MONOTONIC, &end); //End of frame-limit timer
   double startsec, endsec;
   startsec = start.tv_sec + start.tv_nsec / 1000000000.0;
   endsec = end.tv_sec + end.tv_nsec / 1000000000.0;
   //printf("Start: %lf End: %lf Delta: %lf\n",startsec,endsec, endsec - startsec);
   //assert(startsec < endsec);
   sleeptime.tv_nsec = (1000000000 / 60) - ((endsec - startsec) * 1000000000);
   sleeptime.tv_sec = 0;
   nanosleep(&sleeptime,NULL); //Sleep long enough to fill up the rest of 1/60 of a second
}

void reshape (int w, int h)
{
   glViewport (0, 0, (GLsizei) w, (GLsizei) h); 
   glMatrixMode (GL_PROJECTION);
   glLoadIdentity ();
   //glFrustum (-1.0, 1.0, -1.0, 1.0, 1.5, 20.0);
   gluPerspective(60.0, float(w)/float(h), 1.5, 200.0);
   glMatrixMode (GL_MODELVIEW);
   //gluLookAt(10.0,10.0,10.0,0.0,0.0,0.0,0.0,1.0,0.0);
   xres = w;
   yres = h;
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
   glutInitWindowSize (xres, yres); 
   glutInitWindowPosition (100, 100);
   glutCreateWindow (argv[0]);
   init ();
   glutDisplayFunc(display); 
   glutReshapeFunc(reshape);
   glutKeyboardFunc(kb);
   glutMotionFunc(mouse);
   glutPassiveMotionFunc(mouse);
   glutMouseFunc(click);
   glutMainLoop();
   return 0;
}
