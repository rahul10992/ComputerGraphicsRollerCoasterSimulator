#include "GL/freeglut.h"
#include "GL/gl.h"
#include "stdio.h"
#include "jpeglib.h"
#include "math.h"
#include<iostream>
using namespace std;

class Point
{

	public:
		float x;
		float y;
		float z;
		Point(float xcord,float ycord,float zcord)
		{
		x=xcord;
		y=ycord;
		z=zcord;
		}
		Point()
		{
			x=0;
			y=0;
			z=0;
		}
};


int height = 512;
int width = 512;
int numPoints=23;
float t=0;
int step=0;
float gravity = 0.0001;
float velocity = 0.009/3;
float min_velocity = 0.003/3;
float max_velocity = 0.009/3;
Point vertical;
GLuint texture;

GLfloat CP[23][3]={
		{1.0, 0.0, 1.0},
		{6.0, 0.0, 5.0},
		{11.0, 0.0, 5.0},
		{13.0, 0.0, 1.0},
		{16.0, 0.0, 0.0},
		{18.0, 3.0, 0.0},
		{18.0, 8.0, 4.0},
		{18.0, 12.0, 0.0},
		{18.0, 20.0, 0.0},
		{21.0, 23.0, 3.0},
		{18.0, 26.0, 6.0},
		{15.0, 29.0, 3.0},
		{18.0, 32.0, 0.0},
		{18.0, 38.0, 0.0},
		{16.0, 42.0, 0.0},
		{14.0, 42.0, 0.0},
		{12.0, 42.0, 2.0},
		{10.0, 42.0, 0.0},
		{8.0, 42.0, 2.0},
		{6.0, 42.0, 0.0},
		{4.0, 42.0, 2.0},
		{2.0, 42.0, 0.0},
		{0.0, 42.0, 0.0}


};

GLuint LoadTexture(const char * filename, int width, int height){

	FILE *fd;
	unsigned char *image;
	int depth;
	fd = fopen(filename, "rb");
	image = (unsigned char *)malloc(width * height * 3);
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	unsigned long location = 0;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fd);
	jpeg_read_header(&cinfo, 1);
	cinfo.scale_num = 1;
	cinfo.scale_denom = SCALE;
	jpeg_start_decompress(&cinfo);
	width = cinfo.output_width;
	height = cinfo.output_height;
	depth = cinfo.num_components;
	image = (unsigned char *)malloc(width * height * depth);
	row_pointer[0] = (unsigned char *)malloc(width * depth);

	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
		for (int i = 0; i< (width * depth); i++)
			image[location++] = row_pointer[0][i];
	}
	fclose(fd);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
	free(image);

	return texture;

}

class Spline
{
	public:
		int noCP;
		Point points[23];
};

Spline spline;
Point V;

void initSpline()
{
	vertical.x = 0;
	vertical.y = 0;
	vertical.z = -1;
	spline.noCP=numPoints;
	for(int i=0;i<numPoints;i++)
	{
		Point P(CP[i][0],CP[i][1],CP[i][2]);
		spline.points[i]=P;
	}
}

Point CRPoint(float t,Point A,Point B,Point C,Point D)
{
	float x=0.5*(
            (-A.x + 3*B.x -3*C.x + D.x)*t*t*t
            + (2*A.x -5*B.x + 4*C.x - D.x)*t*t
            + (-A.x+C.x)*t
            + (2*B.x)
            );

	float y=0.5*(
	            (-A.y + 3*B.y -3*C.y + D.y)*t*t*t
	            + (2*A.y -5*B.y + 4*C.y - D.y)*t*t
	            + (-A.y+C.y)*t
	            + (2*B.y)
	            );

	float z=0.5*(
	            (-A.z + 3*B.z -3*C.z + D.z)*t*t*t
	            + (2*A.z -5*B.z + 4*C.z - D.z)*t*t
	            + (-A.z+C.z)*t
	            + (2*B.z)
	            );

	return Point(x,y,z);
}

Point CRTangent(float t,Point A,Point B, Point C,Point D)
{
	float x= 0.5 * (
            (-A.x + 3*B.x -3*C.x + D.x)*3*t*t
            + (2*A.x -5*B.x + 4*C.x - D.x)*2*t
            + (-A.x+C.x)
            );

	float y= 0.5 * (
            (-A.y + 3*B.y -3*C.y + D.y)*3*t*t
            + (2*A.y -5*B.y + 4*C.y - D.y)*2*t
            + (-A.y+C.y)
            );

	float z= 0.5 * (
            (-A.z + 3*B.z -3*C.z + D.z)*3*t*t
            + (2*A.z -5*B.z + 4*C.z - D.z)*2*t
            + (-A.z+C.z)
            );

	return Point(x,y,z);
}

Point normalize(Point p)
{
	float length=sqrt((p.x*p.x)+(p.y*p.y)+(p.z*p.z));
	p.x=p.x/length;
	p.y=p.y/length;
	p.z=p.z/length;
	return p;
}


Point crossProd(Point A,Point B)
{
	float x=(A.y*B.z)-(B.y*A.z);
	float y=(B.x*A.z)-(A.x*B.z);
	float z=(A.x*B.y)-(A.y*B.x);
	return Point(x,y,z);
}

void displayRC()
{

	float f=0.08;
	float a=0.003;
	glColor3f(0.4,0.3,0.8);
	for(int r=0;r<2;r++)
	{
	        glBegin(GL_QUADS);
	        for(int i=0;i<spline.noCP;i++)
	        {
	        	for(float u=0.0;u<1.0;u+=0.02)
	        	{
	        		Point P=CRPoint(u,spline.points[i],spline.points[i+1],spline.points[i+2],spline.points[i+3]);

	        		Point T=CRTangent(u,spline.points[i],spline.points[i+1],spline.points[i+2],spline.points[i+3]);
	        		T=normalize(T);

	        		Point N=crossProd(T,V);
	        		N=normalize(N);

	        		Point B=crossProd(T,N);
	        		B=normalize(B);
	        		Point Q=CRPoint(u+0.02,spline.points[i],spline.points[i+1],spline.points[i+2],spline.points[i+3]);
	        		Point U=CRTangent(u+0.02,spline.points[i],spline.points[i+1],spline.points[i+2],spline.points[i+3]);
	        		U=normalize(U);

	        		Point M=crossProd(U,V);
	        		M=normalize(M);
	        		Point C=crossProd(U,M);
	        		C=normalize(C);
	        		if(r==1)
	        		{
	        			P.x+=f*N.x;
	        			P.y+=f*N.y;
	        			P.z+=f*N.z;

	        			Q.x+=f*M.x;
	        			Q.y+=f*M.y;
	        			Q.z+=f*M.z;

	        		}

	        		glVertex3f(P.x+a*(N.x-B.x),P.y+a*(N.y-B.y),P.z+a*(N.z-B.z));		//Vertex 1
	        		glVertex3f(P.x+a*(N.x+B.x),P.y+a*(N.y+B.y),P.z+a*(N.z+B.z));		//Vertex 2
	        		glVertex3f(P.x+a*(-N.x+B.x),P.y+a*(-N.y+B.y),P.z+a*(-N.z+B.z));		//Vertex 3
	        		glVertex3f(P.x+a*(-N.x-B.x),P.y+a*(-N.y-B.y),P.z+a*(-N.z-B.z));		//Vertex 4

	        		glVertex3f(Q.x+a*(M.x-C.x),Q.y+a*(M.y-C.y),Q.z+a*(M.z-C.z));		//Vertex 5
	        		glVertex3f(Q.x+a*(M.x+C.x),Q.y+a*(M.y+C.y),Q.z+a*(M.z+C.z));		//Vertex 6
	        		glVertex3f(Q.x+a*(-M.x+C.x),Q.y+a*(-M.y+C.y),Q.z+a*(-M.z+C.z));		//Vertex 7
	        		glVertex3f(Q.x+a*(-M.x-C.x),Q.y+a*(-M.y-C.y),Q.z+a*(-M.z-C.z));		//Vertex 8

	        		glVertex3f(P.x+a*(N.x-B.x),P.y+a*(N.y-B.y),P.z+a*(N.z-B.z));		//Vertex 1
	        		glVertex3f(P.x+a*(-N.x-B.x),P.y+a*(-N.y-B.y),P.z+a*(-N.z-B.z));		//Vertex 4
	        		glVertex3f(Q.x+a*(-M.x-C.x),Q.y+a*(-M.y-C.y),Q.z+a*(-M.z-C.z));		//Vertex 8
	        		glVertex3f(Q.x+a*(M.x-C.x),Q.y+a*(M.y-C.y),Q.z+a*(M.z-C.z));		//Vertex 5

	        		glVertex3f(P.x+a*(-N.x+B.x),P.y+a*(-N.y+B.y),P.z+a*(-N.z+B.z));		//Vertex 3
	        		glVertex3f(P.x+a*(-N.x-B.x),P.y+a*(-N.y-B.y),P.z+a*(-N.z-B.z));		//Vertex 4
	        		glVertex3f(Q.x+a*(-M.x-C.x),Q.y+a*(-M.y-C.y),Q.z+a*(-M.z-C.z));		//Vertex 8
	        		glVertex3f(Q.x+a*(-M.x+C.x),Q.y+a*(-M.y+C.y),Q.z+a*(-M.z+C.z));		//Vertex 7

	        		glVertex3f(P.x+a*(N.x+B.x),P.y+a*(N.y+B.y),P.z+a*(N.z+B.z));		//Vertex 2
	        		glVertex3f(P.x+a*(-N.x+B.x),P.y+a*(-N.y+B.y),P.z+a*(-N.z+B.z));		//Vertex 3
	        		glVertex3f(Q.x+a*(-M.x+C.x),Q.y+a*(-M.y+C.y),Q.z+a*(-M.z+C.z));		//Vertex 7
	        		glVertex3f(Q.x+a*(M.x+C.x),Q.y+a*(M.y+C.y),Q.z+a*(M.z+C.z));		//Vertex 6

	        		glVertex3f(P.x+a*(N.x-B.x),P.y+a*(N.y-B.y),P.z+a*(N.z-B.z));		//Vertex 1
	        		glVertex3f(P.x+a*(N.x+B.x),P.y+a*(N.y+B.y),P.z+a*(N.z+B.z));		//Vertex 2
	        		glVertex3f(Q.x+a*(M.x+C.x),Q.y+a*(M.y+C.y),Q.z+a*(M.z+C.z));		//Vertex 6
	        		glVertex3f(Q.x+a*(M.x-C.x),Q.y+a*(M.y-C.y),Q.z+a*(M.z-C.z));		//Vertex 5


	        	}
	        }

	        glEnd();
	}
	//glColor3f(1.0, 1.0, 1.0);
	glColor3f(0.8,0.4,0.35);
}

void displayGround(){

	
	glColor3f(0.8,0.6,0);
	texture = LoadTexture("wall.jpg", 256, 256);
	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);

	glTexCoord2d(0.0, 0.0);
	glVertex3d(-100, -100.0, -2);
	glTexCoord2d(100.0, 0.0); 
	glVertex3d(100.0, -100.0, -2);
	glTexCoord2d(100.0, 100.0); 
	glVertex3d(100.0, 100.0, -2);
	glTexCoord2d(0.0, 100.0); 
	glVertex3d(-100.0, 100.0, -2);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	
}

void displayWalls(){

	glColor3f(0.3, 0.7, 1);
	texture = LoadTexture("sky1.jpg", 256, 256);
	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	//top
	glTexCoord2d(0.0, 0.0); glVertex3d(-100, -100.0, 100);
	glTexCoord2d(1.0, 0.0); glVertex3d(100.0, -100.0, 100);
	glTexCoord2d(1.0, 1.0); glVertex3d(100.0, 100.0, 100);
	glTexCoord2d(0.0, 1.0); glVertex3d(-100.0, 100.0, 100)

	glEnd();

	glBegin(GL_QUADS);

	//right wall
	glTexCoord2d(0.0, 0.0); glVertex3d(-100, -100, -100);
	glTexCoord2d(1.0, 0.0); glVertex3d(-100, 100.0, -100);
	glTexCoord2d(1.0, 1.0); glVertex3d(-100, 100.0, 100);
	glTexCoord2d(0.0, 1.0); glVertex3d(-100, -100.0, 100);

	//left wall
	glTexCoord2d(0.0, 0.0); glVertex3d(-100, 100.0, -100);
	glTexCoord2d(1.0, 0.0); glVertex3d(100.0, 100.0, -100);
	glTexCoord2d(1.0, 1.0); glVertex3d(100.0, 100.0, 100);
	glTexCoord2d(0.0, 1.0); glVertex3d(-100.0, 100.0, 100);

	//front wall
	glTexCoord2d(0.0, 0.0); glVertex3d(100, -100, -100);
	glTexCoord2d(1.0, 0.0); glVertex3d(100, 100.0, -100);
	glTexCoord2d(1.0, 1.0); glVertex3d(100, 100.0, 100);
	glTexCoord2d(0.0, 1.0); glVertex3d(100, -100.0, 100);
	glDisable(GL_TEXTURE_2D);
	glEnd();
}


void displayRail()
{
				float f=0.08;
				float d=0.006;

				glBegin(GL_QUADS);
		        for(int i=spline.noCP-1;i>=0;i--)
		        {
		        	for(float u=0.0;u<1.0;u+=0.05)
		        	{
		        		Point P=CRPoint(u,spline.points[i],spline.points[i+1],spline.points[i+2],spline.points[i+3]);

		        		Point T=CRTangent(u,spline.points[i],spline.points[i+1],spline.points[i+2],spline.points[i+3]);
		        		T=normalize(T);

		        		Point N=crossProd(T,V);
		        		N=normalize(N);


		        		Point Q=CRPoint(u+0.02,spline.points[i],spline.points[i+1],spline.points[i+2],spline.points[i+3]);
		        		Point U=CRTangent(u+0.02,spline.points[i],spline.points[i+1],spline.points[i+2],spline.points[i+3]);
		        		U=normalize(U);

		        		Point M=crossProd(U,V);
		        		M=normalize(M);

		        		glColor3f(1.0, 1.0, 1.0);
		        		glVertex3f(P.x,P.y,P.z);
		        		glVertex3f(P.x+f*(N.x),P.y+f*(N.y),P.z+f*(N.z));
		        		glVertex3f(Q.x+f*(M.x),Q.y+f*(M.y),Q.z+f*(M.z));
		        		glVertex3f(Q.x,Q.y,Q.z);

		        		glColor3f(1.0, 1.0, 1.0);
		        		glVertex3f(P.x,P.y,P.z-d);
		        		glVertex3f(P.x+f*(N.x),P.y+f*(N.y),P.z+f*(N.z)-d);
		        		glVertex3f(Q.x+f*(M.x),Q.y+f*(M.y),Q.z+f*(M.z)-d);
		        		glVertex3f(Q.x,Q.y,Q.z-d);

		        		glColor3f(0.6, 0.6, 0.6);
		        		glVertex3f(P.x,P.y,P.z);
		        		glVertex3f(P.x+f*(N.x),P.y+f*(N.y),P.z+f*(N.z));
		        		glVertex3f(P.x+f*(N.x),P.y+f*(N.y),P.z+f*(N.z)-d);
		        		glVertex3f(P.x,P.y,P.z-d);

		        		glColor3f(1.0, 1.0, 1.0);
		        		glVertex3f(Q.x,Q.y,Q.z);
		        		glVertex3f(Q.x+f*(M.x),Q.y+f*(M.y),Q.z+f*(M.z));
		        		glVertex3f(Q.x+f*(M.x),Q.y+f*(M.y),Q.z+f*(M.z)-d);
		        		glVertex3f(Q.x,Q.y,Q.z-d);


		        	}
		        }

		        glEnd();
}

float dotProduct(Point p1, Point p2){
	return p1.x*p2.x + p1.y*p2.y+p1.z*p2.z;
}


void viewAndPhysics()
{

	float h=0.03;
	float f=0.04;
	Point P=CRPoint(t,spline.points[step],spline.points[step+1],spline.points[step+2],spline.points[step+3]);

	Point T=CRTangent(t,spline.points[step],spline.points[step+1],spline.points[step+2],spline.points[step+3]);
	T=normalize(T);

	Point N=crossProd(T,V);
	N=normalize(N);

	Point B=crossProd(T,N);

	Point cam;

	cam.x=P.x+(f*N.x)+(h*B.x);
	cam.y=(P.y)+(f*N.y)+(h*B.y);
	cam.z=P.z+(f*N.z)+(h*B.z);

	printf("Point: %f %f %f\n",P.x,P.y,P.z);
	printf("Camera: %f %f %f\n",cam.x,cam.y,cam.z);
	float cos = dotProduct(vertical, normalize(T));
	    //cout<<"step "<<step<<endl;
	    //cout<<"tangent: x: "<<T.x<<" y: "<<T.y<<" z: "<<T.z<<endl;
	    //cout<<"cos: "<<cos<<endl;
	float old_velocity = velocity;
	    velocity = velocity + cos*gravity;
	    if (velocity > max_velocity)
	    	velocity = old_velocity;
	    if (velocity <= min_velocity){
	    	velocity = old_velocity;
	    }
	    cout<<"velocity: "<<velocity<<endl;



	gluLookAt(cam.x,cam.y,cam.z,
			P.x + f*N.x+T.x , P.y+f*N.y +T.y,P.z+f*N.z +T.z,
			B.x,B.y,B.z);

}


void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glPushMatrix();

	V=Point(0.0,0.0,-1.0);

    //displayWalls();
    //displayGround();
	viewAndPhysics();
	displayRail();
	displayRC();


	glPopMatrix();
	glutSwapBuffers();

}

void animation(){
	t+=velocity;
	cout<<"move: "<<t<<endl;
	if(t>=1.0){
		step++;
	    t=0.0;
	    if(step>=spline.noCP-3)
	    	step=0;
	}else if (t <= 0.0){
		t = 1-t;
		step--;
		/*if(move_i<=0){
			move_i=g_Splines[0].numControlPoints-3;
		}*/
	}
	/*else if (move < 0.0){
		move_i --;
		move = 1.0-move;
		if (move_i <= 0){
			move_i = 0;
			move = 1-move;
		}
	}*/else{}

	display();
}

void reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();
    gluPerspective(20.0, (GLfloat)w/(GLfloat)h, 0.01, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


int main(int argc, char** argv)
{
	initSpline();
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE);
    glutInitWindowSize(height,width);
    glutInitWindowPosition(100,100);
    glutCreateWindow("Roller Coaster Simulation");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(animation);
    glutMainLoop();
    return 0;
}
