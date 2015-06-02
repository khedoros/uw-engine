/* Vertex array example by Nutty. (www.nutty.org) 
   Demonstrates the use of vertex arrays for defining geometric primitives.
   Using glut to hide the ugliness of windoze code.

   This source can be distributed freely.
   Nutty.	nutty@nutty.org
*/

#include <GL/glut.h>

// The Initial size of our window
int window_width = 640;
int window_height = 480;

// Define our vertex structure.
// This holds the position of the vertices. Can also be used for
// anything that requires 3 float elements. i.e. colors.
typedef struct
{
	GLfloat x, y, z;

}Vertex3;

// Our constant. Number of faces per cube.
const GLuint NUMBER_FACES	=	12;
// This is a variable for rotating the cube.
GLfloat angle			=	0.0f;


// Our vertex array! Holds the points for each corner of the cube.
Vertex3 cube_verts[] = {
	{	-2.0f,	2.0f, 2.0f,	},	//Frontface top left.		0
	{	 2.0f,	2.0f, 2.0f,	},	//Frontface top right.		1
	{	 2.0f, -2.0f, 2.0f,	},	//Frontface bottom right.	2
	{   -2.0f, -2.0f, 2.0f, },  //Frontface bottom left.	3

	{	-2.0f,	2.0f, -2.0f, },	//backface top left.		4
	{	 2.0f,	2.0f, -2.0f, },	//backface top right.		5
	{	 2.0f, -2.0f, -2.0f, },	//backface bottom right.	6
	{   -2.0f, -2.0f, -2.0f, }	//backface bottom left		7
};

// This is an array of colors. Each color defined by 3 floats. Order will map
// directly to the cornder described in the above array.
Vertex3 vert_colors[] = {
	{	1.0f,	0.0f,   0.0f,	},	
	{	0.0f,	1.0f,   0.0f,	},
	{	0.0f,	0.0f,	1.0f,	},
	{	1.0f,	1.0f,	0.0f,	},

	{	1.0f,	0.0f,	1.0f,	},
	{	0.0f,	1.0f,	1.0f,	},
	{	0.0f,	0.0f,	0.0f,	},
	{	1.0f,	1.0f,	1.0f,	}
};

// These are the indexes into the above arrays to make our cube.
GLuint cube_indexes[NUMBER_FACES * 3] = {
	0, 3, 2,					//Frontface. In counter clockwise order.
        2, 1, 0,
	5, 6, 7,					//Backface.  In counter clockwise order.
        7, 4, 5,
	4, 0, 1,					//Top face.	
        1, 5, 4,
	3, 7, 6,					//Bottom face.
        6, 2, 3,
	4, 0, 3,					//Left face.
        3, 7, 4,
	1, 2, 6,					//Right face.
        6, 5, 1
};



void Render(void)
{
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0.0f, 0.0f, -5.0f);		//Move the Cube 10 away from us!

	glRotatef(angle, 1.0f, 0.77f, 0.0f);		//Spin the cube in X and Y axis.

	
	//Enable the vertex and color arrays.
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	//Tell gl, what our arrays look like! 3 floats per vertex, 0 packing, and their addresses.
	glVertexPointer(3, GL_FLOAT, 0, cube_verts);
	glColorPointer(3, GL_FLOAT, 0, vert_colors);

	//Do it! Draw quads. With 6 faces times 4 indexes per face. 
	glDrawElements(GL_TRIANGLES, NUMBER_FACES * 3, GL_UNSIGNED_INT, cube_indexes);
		
	
    glutSwapBuffers();
}

void Idle(void)
{	
	

	//Increase angle.
	angle += 0.4f;		

	if(angle >= 360.0f)
	{
		angle -= 360.0f;
	}

	//Re display.
	glutPostRedisplay ();	
}

void Resize(int width, int height)
{
    if(height == 0)
	{
		height = 1;
	}
     
	window_width = width;
	window_height = height;

    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, window_width, window_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective (90, (float)window_width / (float)window_height, 1, 5000);

    glutPostRedisplay();
}

void Keyboard(unsigned char key, int x, int y)
{

	if(key == 27)	//If escape key.
	{
		exit(0);
	}
}

void Mouse(int button, int state, int x, int y)
{
}

void glInit(void)
{
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glShadeModel(GL_SMOOTH);
}

int main(int argc, char **argv)
{
    //Setup the window. RGB double buffered, with a depth buffer.
    glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Vertex Array Test - Esc quits");

	//Setup our callbacks.
    glutDisplayFunc(Render);
    glutReshapeFunc(Resize);
    glutIdleFunc(Idle);
    glutKeyboardFunc(Keyboard);
	glutMouseFunc(Mouse);

    glInit();
    glutMainLoop();
}


