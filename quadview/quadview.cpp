#include <math.h>
#include <time.h>
#include <stdio.h>

#if defined (__APPLE__) || defined (MACOSX)
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include "image.h"

GLuint torus_list = 0;
GLuint width=640, height=480;
GLfloat spin_x = 0.0, spin_y = 0.0;

const GLfloat edge_color[3]={0.7, 0.8, 0.9};
const GLfloat axis_color[3]={0.8, 0.2, 1.0};
const GLfloat grid_color[3]={0.4, 0.0, 0.5};
const GLfloat font_color[3]={1.0, 1.0, 1.0};

// you can change to other image format if you like.
const char suffix[]="jpg";

double getElapsedTime()
{
	static clock_t start=clock();
	clock_t stop=clock();
	
	double elapsed_time=(double)(stop-start)/CLOCKS_PER_SEC;
	return elapsed_time;
}

void printText(int x, int y, const char* string)
{
	glRasterPos2i((GLint)x, (GLint)y);
	
	for(const char* p = string; *p!='\0'; ++p)
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *p);
}

GLuint displayList(void)
{
	static GLfloat gold_ambient[4]  = { 0.24725f, 0.1995f, 0.0745f, 1.0f };
	static GLfloat gold_diffuse[4]  = { 0.75164f, 0.60648f, 0.22648f, 1.0f };
	static GLfloat gold_specular[4] = { 0.628281f, 0.555802f, 0.366065f, 1.0f };
	static GLfloat gold_shininess   = 41.2f;
	
	static GLfloat silver_ambient[4]  = { 0.05f, 0.05f, 0.05f, 1.0f };
	static GLfloat silver_diffuse[4]  = { 0.4f, 0.4f, 0.4f, 1.0f };
	static GLfloat silver_specular[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
	static GLfloat silver_shininess   = 12.0f;

	GLuint list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	
	glMaterialfv(GL_FRONT, GL_AMBIENT,  gold_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE,  gold_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, gold_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, gold_shininess);
	
	glMaterialfv(GL_BACK, GL_AMBIENT,  silver_ambient);
	glMaterialfv(GL_BACK, GL_DIFFUSE,  silver_diffuse);
	glMaterialfv(GL_BACK, GL_SPECULAR, silver_specular);
	glMaterialf(GL_BACK, GL_SHININESS, silver_shininess);
	
	glutSolidSphere(0.5/*radius*/, 32/*slices*/, 32/*stacks*/);
	glutWireTorus(0.3/*innerRadius*/, 0.5/*outerRadius*/, 64/*nsides*/, 128/*rings*/);
	glEndList();
	
	return list;
}

void init(void)
{
	GLfloat light_pos[4] = { 1.0, 1.0, 1.0, 0.0 };

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

void reshape(int width, int height)
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	::width=width;
	::height=height;
}

void projection(int width, int height, bool perspective)
{
	GLdouble ratio = (GLdouble)width/height;
	GLdouble z_near=1, z_far=256;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(perspective)
		gluPerspective(60/*degrees*/, ratio, z_near, z_far);
	else 
		glOrtho(-ratio, ratio, -ratio, ratio, z_near, z_far);
		
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	GLdouble eye   [3]={0.0, 0.0, 2.0};
	GLdouble center[3]={0.0, 0.0, 0.0};
	GLdouble up    [3]={0.0, 1.0, 0.0};
	gluLookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);
}

void drawCross(int x, int y, int width, int height)
{
	int half_w=width>>1, half_h=height>>1;
	
	glBegin(GL_LINES);
	glVertex2i(x-half_w, y); glVertex2i(x+half_w, y);
	glVertex2i(x, y-half_h); glVertex2i(x, y+half_h);
	glEnd();
}

void drawAxis(int x, int y, int width, int height)
{
	int half_w=width>>1, half_h=height>>1;
	int radical=10*cos(3.0/18.0*M_PI);
	int tangent=10*sin(3.0/18.0*M_PI);
	
	glBegin(GL_LINES);
	glVertex2i(x-half_w, y);
	glVertex2i(x+half_w, y);
	glVertex2i(x+half_w, y);
	glVertex2i(x+half_w-radical, y-tangent);
	glVertex2i(x+half_w-radical, y+tangent);
	glVertex2i(x+half_w, y);
	
	glVertex2i(x, y-half_h);
	glVertex2i(x, y+half_h);
	glVertex2i(x, y+half_h);
	glVertex2i(x-tangent, y+half_h-radical);
	glVertex2i(x+tangent, y+half_h-radical);
	glVertex2i(x, y+half_h);
	glEnd();
}

void drawGrid(int x, int y, int width, int height)
{
	int half_w=width>>1, half_h=height>>1;
	const int delta=(width<height?width:height)/20;
	
	glBegin(GL_LINES);
	for(int i=x; i<x+half_w; i+=delta)
	{
		glVertex2i(i, y-half_h);
		glVertex2i(i, y+half_h);
	}
	for(int i=x-delta; i>x-half_w; i-=delta)
	{
		glVertex2i(i, y-half_h);
		glVertex2i(i, y+half_h);
	}
	for(int j=y; j<y+half_h; j+=delta)
	{
		glVertex2i(x-half_w, j);
		glVertex2i(x+half_w, j);
	}
	for(int j=y-delta; j>y-half_h; j-=delta)
	{
		glVertex2i(x-half_w, j);
		glVertex2i(x+half_w, j);
	}
	glEnd();
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glViewport(0, 0, width, height);
	projection(width, height, true);

	glRotatef(spin_y, 1.0, 0.0, 0.0);
	glRotatef(spin_x, 0.0, 1.0, 0.0);
	glCallList(torus_list);
	
	glutSwapBuffers();
}

// quadview
void display4(void)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, width, 0, height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_LIGHTING);
	
	const int half_w=width>>1, half_h=height>>1;
	
	glColor3fv(edge_color);
	drawCross(half_w, half_h, width, height);
	
	int offset=height/20;
	const int quarter_w=width>>2, quater_h=height>>2;
	const int axis_w=half_w-offset, axis_h=half_h-offset;
	
	glColor3fv(axis_color);
	drawAxis(half_w + quarter_w, quater_h, axis_w, axis_h); // right
	drawAxis(         quarter_w, quater_h, axis_w, axis_h); // up
	drawAxis(quarter_w, half_h + quater_h, axis_w, axis_h); // back
	
	glColor3fv(grid_color);
	drawGrid(half_w + quarter_w, quater_h, half_w, half_h); // right
	drawGrid(         quarter_w, quater_h, half_w, half_h); // up
	drawGrid(quarter_w, half_h + quater_h, half_w, half_h); // back
	
	offset=5;
	glColor3fv(font_color);
	printText(offset, offset, "Front");
	printText(half_w+offset, offset, "Right");
	printText(offset, half_h+offset, "Top");
	printText(half_w+offset, half_h+offset, "Perspective");

	glEnable(GL_LIGHTING);
	glEnable(GL_SCISSOR_TEST);

	// bottom left window, back.
	glViewport(0, 0, half_w, half_h);
	glScissor(0, 0, half_w, half_h);
	projection(half_w, half_h, false);
	glRotatef(spin_y, 1.0, 0.0, 0.0);
	glRotatef(spin_x, 0.0, 1.0, 0.0);
	glCallList(torus_list);

	// bottom right window, right.
	glViewport(half_w, 0, half_w, half_h);
	glScissor(half_w, 0, half_w, half_h);
	projection(half_w, half_h, false);
	glRotatef(90.0, 0.0, 1.0, 0.0);
	glRotatef(spin_y, 1.0, 0.0, 0.0);
	glRotatef(spin_x, 0.0, 1.0, 0.0);
	glCallList(torus_list);

	// top left window, up.
	glViewport(0, half_h, half_w, half_h);
	glScissor(0, half_h, half_w, half_h);
	projection(half_w, half_h, false);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	glRotatef(spin_y, 1.0, 0.0, 0.0);
	glRotatef(spin_x, 0.0, 1.0, 0.0);
	glCallList(torus_list);

	// top right window, perspective.
	glViewport(half_w, half_h, half_w, half_h);
	glScissor(half_w, half_h, half_w, half_h);
	projection(half_w, half_h, true);
	glRotatef(spin_y, 1.0, 0.0, 0.0);
	glRotatef(spin_x, 0.0, 1.0, 0.0);
	glCallList(torus_list);

	glDisable(GL_SCISSOR_TEST);

	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
	static bool quadview=true;
	switch(key)
	{
	case 'P':
	case 'p':
		{
			static char filename[25];
			double timing=getElapsedTime();

			snprintf(filename, sizeof(filename), "%011.6lf.%s", timing, suffix);
			printf("snapshot parameters: %dx%d, %s\n", width, height, filename);
			snapshot(width, height, filename);
		}
		break;
	case 'Q':
	case 'q':
		glutDisplayFunc(quadview?display:display4);
		quadview=!quadview;
		break;
	case 27: /* ESC */
		exit(EXIT_SUCCESS);
		break;
	}

	glutPostRedisplay();
}

static int old_x, old_y;

void mouse(int button, int state, int x, int y)
{
	if(button==GLUT_LEFT_BUTTON)
	{
		old_x=x;
		old_y=y;
	}
}

void motion(int x, int y)
{
	spin_x=x-old_x;
	spin_y=y-old_y;

	glutPostRedisplay();
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(width, height);
	glutCreateWindow("quadview");
	
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutDisplayFunc(display4);
	glutMotionFunc(motion);
	glutMouseFunc(mouse);

	init();
	torus_list=displayList();

	glutMainLoop();

	return 0;
}
