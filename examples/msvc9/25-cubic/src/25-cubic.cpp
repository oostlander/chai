//===========================================================================
/*
    This file is part of the CHAI 3D visualization and haptics libraries.
    Copyright (C) 2003-2009 by CHAI 3D. All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.

    For using the CHAI 3D libraries with software that can not be combined
    with the GNU GPL, and for taking advantage of the additional benefits
    of our support services, please contact CHAI 3D about acquiring a
    Professional Edition License.

    \author    <http://www.chai3d.org>
    \author    Francois Conti
    \version   2.0.0 $Rev: 269 $
*/
//===========================================================================

//---------------------------------------------------------------------------
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <cstdlib>
#include <stack>
//---------------------------------------------------------------------------
#include "chai3d.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------

// initial size (width/height) in pixels of the display window
const int WINDOW_SIZE_W         = 512;
const int WINDOW_SIZE_H         = 512;

// mouse menu options (right button)
const int OPTION_FULLSCREEN     = 1;
const int OPTION_WINDOWDISPLAY  = 2;


//---------------------------------------------------------------------------
// DECLARED VARIABLES
//---------------------------------------------------------------------------

// a flag for doing things ony once
int flag = 0;
// default offset for drawing numbers
int defoff = 0;

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera that renders the world in a window display
cCamera* camera;

// a light source to illuminate the objects in the virtual scene
cLight *light;

// a little "chai3d" bitmap logo at the bottom of the screen
cBitmap* logo;

// width and height of the current window display
int displayW  = 0;
int displayH  = 0;

// a haptic device handler
cHapticDeviceHandler* handler;

// a virtual tool representing the haptic device in the scene
cGeneric3dofPointer* tool;

// radius of the tool proxy
double proxyRadius;

// a virtual object
cMesh* object;

// a list of vertices for each face of the cube
int vertices[6][4];

// a texture
cTexture2D* texture;

// rotational velocity of the object
cVector3d rotVel(0.0, 0.1, 0.1);

// status of the main simulation haptics loop
bool simulationRunning = false;

// simulation clock
cPrecisionClock simClock;

// root resource path
string resourceRoot;

// has exited haptics simulation thread
bool simulationFinished = false;

//---------------------------------------------------------------------------
// DECLARED MACROS
//---------------------------------------------------------------------------
// convert to resource path
#define RESOURCE_PATH(p)    (char*)((resourceRoot+string(p)).c_str())


//---------------------------------------------------------------------------
// DECLARED FUNCTIONS
//---------------------------------------------------------------------------

// callback when the window display is resized
void resizeWindow(int w, int h);

// callback when a keyboard key is pressed
void keySelect(unsigned char key, int x, int y);

// callback when the right mouse button is pressed to select a menu item
void menuSelect(int value);

// function called before exiting the application
void close(void);

// main graphics callback
void updateGraphics(void);

// main haptics loop
void updateHaptics(void);

// print a number to a tecture
void printNumbers(cTexture2D* texture, int x, int y, int number);
void printNumbers(cBitmap* texture, int x, int y, int number);
void drawLines(cBitmap* texture,int l1,int l2,int l3,int l4,int l5,int l6,int l7,int* posx = &defoff, int* posy = &defoff);


//===========================================================================
/*
    DEMO:    cubic.cpp

    This example illustrates how to build a small cube by assembling a
    cloud of triangles together. The applications also presents the
    use of texture properties by defining a texture image and defining
    texture coordinates at each of the vertices of the object.
    The texture image is produced and updated by copying the image
    buffer of the virtual camera at each graphical rendering cycle.
*/
//===========================================================================

int main(int argc, char* argv[])
{
    //-----------------------------------------------------------------------
    // INITIALIZATION
    //-----------------------------------------------------------------------

    printf ("\n");
    printf ("-----------------------------------\n");
    printf ("CHAI 3D\n");
    printf ("Demo: 25-cubic\n");
    printf ("Copyright 2003-2009\n");
    printf ("-----------------------------------\n");
    printf ("\n\n");
    printf ("Keyboard Options:\n\n");
    printf ("[x] - Exit application\n");
    printf ("\n\n");

    // parse first arg to try and locate resources
    resourceRoot = string(argv[0]).substr(0,string(argv[0]).find_last_of("/\\")+1);


    //-----------------------------------------------------------------------
    // 3D - SCENEGRAPH
    //-----------------------------------------------------------------------

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    // the color is defined by its (R,G,B) components.
    world->setBackgroundColor(1.0, 1.0, 1.0);

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and oriente the camera
    camera->set( cVector3d (3.0, 0.0, 0.0),    // camera position (eye)
                 cVector3d (0.0, 0.0, 0.0),    // lookat position (target)
                 cVector3d (0.0, 0.0, 1.0));   // direction of the "up" vector

    // set the near and far clipping planes of the camera
    // anything in front/behind these clipping planes will not be rendered
    camera->setClippingPlanes(0.01, 10.0);

    // create a light source and attach it to the camera
    light = new cLight(world);
    camera->addChild(light);                   // attach light to camera
    light->setEnabled(true);                   // enable light source
    light->setPos(cVector3d( 2.0, 0.5, 1.0));  // position the light source
    light->setDir(cVector3d(-2.0, 0.5, 1.0));  // define the direction of the light beam


    //-----------------------------------------------------------------------
    // 2D - WIDGETS
    //-----------------------------------------------------------------------
	
    // create a 2D bitmap logo
    logo = new cBitmap();

    // add logo to the front plane
    camera->m_front_2Dscene.addChild(logo);

    // load a "chai3d" bitmap image file
    bool fileload;
    fileload = logo->m_image.loadFromFile(RESOURCE_PATH("resources/images/chai3d-w.bmp"));
    if (!fileload)
    {
        #if defined(_MSVC)
        fileload = logo->m_image.loadFromFile("../../../bin/resources/images/chai3d-w.bmp");
        #endif
    }


    // position the logo at the bottom left of the screen (pixel coordinates)
    logo->setPos(10, 10, 0);

    // scale the logo along its horizontal and vertical axis
    logo->setZoomHV(1, 1);

    // here we replace all wite pixels (1,1,1) of the logo bitmap
    // with transparent black pixels (1, 1, 1, 0). This allows us to make
    // the background of the logo look transparent.
    logo->m_image.replace(
                          cColorb(0xff, 0xff, 0xff),         // original RGB color
                          cColorb(0xff, 0xff, 0xff, 0x00)    // new RGBA color
                          );

	//printNumbers(logo,40,40,100);                                                                                          //<<<<<------------------------------------------------///
    printNumbers(logo,40,40,42);
	// enable transparency
    logo->enableTransparency(true);
	

    //-----------------------------------------------------------------------
    // HAPTIC DEVICES / TOOLS
    //-----------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

    // get access to the first available haptic device
    cGenericHapticDevice* hapticDevice;
    handler->getDevice(hapticDevice, 0);

    // retrieve information about the current haptic device
    cHapticDeviceInfo info;
    if (hapticDevice)
    {
        info = hapticDevice->getSpecifications();
    }

    // create a 3D tool and add it to the world
    tool = new cGeneric3dofPointer(world);
    world->addChild(tool);

    // connect the haptic device to the tool
    tool->setHapticDevice(hapticDevice);

    // initialize tool by connecting to haptic device
    tool->start();

    // map the physical workspace of the haptic device to a larger virtual workspace.
    tool->setWorkspaceRadius(1.0);

    // define a radius for the tool (graphical display)
    tool->setRadius(0.05);

    // hide the device sphere. only show proxy.
    tool->m_deviceSphere->setShowEnabled(false);

    // set the physical readius of the proxy.
    proxyRadius = 0.05;
    tool->m_proxyPointForceModel->setProxyRadius(proxyRadius);
    tool->m_proxyPointForceModel->m_collisionSettings.m_checkBothSidesOfTriangles = false;

    // enable if objects in the scene are going to rotate of translate
    // or possibly collide against the tool. If the environment
    // is entirely static, you can set this parameter to "false"
    tool->m_proxyPointForceModel->m_useDynamicProxy = true;

    // read the scale factor between the physical workspace of the haptic
    // device and the virtual workspace defined for the tool
    double workspaceScaleFactor = tool->getWorkspaceScaleFactor();

    // define a maximum stiffness that can be handled by the current
    // haptic device. The value is scaled to take into account the
    // workspace scale factor
    double stiffnessMax = info.m_maxForceStiffness / workspaceScaleFactor;


    //-----------------------------------------------------------------------
    // COMPOSE THE VIRTUAL SCENE
    //-----------------------------------------------------------------------

    // create a virtual mesh
    object = new cMesh(world);

    // add object to world
    world->addChild(object);

    // set the position of the object at the center of the world
    object->setPos(0.0, 0.0, 0.0);


    /////////////////////////////////////////////////////////////////////////
    // create a cube
    /////////////////////////////////////////////////////////////////////////
    const double HALFSIZE = 0.08;

    // face -x
    vertices[0][0] = object->newVertex(-HALFSIZE,  HALFSIZE, -HALFSIZE);
    vertices[0][1] = object->newVertex(-HALFSIZE, -HALFSIZE, -HALFSIZE);
    vertices[0][2] = object->newVertex(-HALFSIZE, -HALFSIZE,  HALFSIZE);
    vertices[0][3] = object->newVertex(-HALFSIZE,  HALFSIZE,  HALFSIZE);

    // face +x
    vertices[1][0] = object->newVertex( HALFSIZE, -HALFSIZE, -HALFSIZE);
    vertices[1][1] = object->newVertex( HALFSIZE,  HALFSIZE, -HALFSIZE);
    vertices[1][2] = object->newVertex( HALFSIZE,  HALFSIZE,  HALFSIZE);
    vertices[1][3] = object->newVertex( HALFSIZE, -HALFSIZE,  HALFSIZE);

    // face -y
    vertices[2][0] = object->newVertex(-HALFSIZE,  -HALFSIZE, -HALFSIZE);
    vertices[2][1] = object->newVertex( HALFSIZE,  -HALFSIZE, -HALFSIZE);
    vertices[2][2] = object->newVertex( HALFSIZE,  -HALFSIZE,  HALFSIZE);
    vertices[2][3] = object->newVertex(-HALFSIZE,  -HALFSIZE,  HALFSIZE);

    // face +y
    vertices[3][0] = object->newVertex( HALFSIZE,   HALFSIZE, -HALFSIZE);
    vertices[3][1] = object->newVertex(-HALFSIZE,   HALFSIZE, -HALFSIZE);
    vertices[3][2] = object->newVertex(-HALFSIZE,   HALFSIZE,  HALFSIZE);
    vertices[3][3] = object->newVertex( HALFSIZE,   HALFSIZE,  HALFSIZE);

    // face -z
    vertices[4][0] = object->newVertex(-HALFSIZE,  -HALFSIZE, -HALFSIZE);
    vertices[4][1] = object->newVertex(-HALFSIZE,   HALFSIZE, -HALFSIZE);
    vertices[4][2] = object->newVertex( HALFSIZE,   HALFSIZE, -HALFSIZE);
    vertices[4][3] = object->newVertex( HALFSIZE,  -HALFSIZE, -HALFSIZE);

    // face +z
    vertices[5][0] = object->newVertex( HALFSIZE,  -HALFSIZE,  HALFSIZE);
    vertices[5][1] = object->newVertex( HALFSIZE,   HALFSIZE,  HALFSIZE);
    vertices[5][2] = object->newVertex(-HALFSIZE,   HALFSIZE,  HALFSIZE);
    vertices[5][3] = object->newVertex(-HALFSIZE,  -HALFSIZE,  HALFSIZE);

    // create triangles
    for (int i=0; i<6; i++)
    {
        object->newTriangle(vertices[i][0], vertices[i][1], vertices[i][2]);
        object->newTriangle(vertices[i][0], vertices[i][2], vertices[i][3]);
    }

    // create a texture
    texture = new cTexture2D();
    object->setTexture(texture);
    object->setUseTexture(true);

    // set material properties to light gray
    object->m_material.m_ambient.set(0.5f, 0.5f, 0.5f, 1.0f);
    object->m_material.m_diffuse.set(0.7f, 0.7f, 0.7f, 1.0f);
    object->m_material.m_specular.set(1.0f, 1.0f, 1.0f, 1.0f);
    object->m_material.m_emission.set(0.0f, 0.0f, 0.0f, 1.0f);

    // compute normals
    object->computeAllNormals();

    // display triangle normals
    object->setShowNormals(true);

    // set length and color of normals
    object->setNormalsProperties(0.03, cColorf(1.0, 0.0, 0.0), true);

    // compute a boundary box
    object->computeBoundaryBox(true);

    // get dimensions of object
    double size = cSub(object->getBoundaryMax(), object->getBoundaryMin()).length();

    // resize object to screen
    object->scale( 2.0 * tool->getWorkspaceRadius() / size);

    // compute collision detection algorithm
    object->createAABBCollisionDetector(1.01 * proxyRadius, true, false);

    // define a default stiffness for the object
    object->setStiffness(stiffnessMax, true);

    // define friction properties
    object->setFriction(0.2, 0.5, true);


    //-----------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //-----------------------------------------------------------------------

    // initialize GLUT
    glutInit(&argc, argv);

    // retrieve the resolution of the computer display and estimate the position
    // of the GLUT window so that it is located at the center of the screen
    int screenW = glutGet(GLUT_SCREEN_WIDTH);
    int screenH = glutGet(GLUT_SCREEN_HEIGHT);
    int windowPosX = (screenW - WINDOW_SIZE_W) / 2;
    int windowPosY = (screenH - WINDOW_SIZE_H) / 2;

    // initialize the OpenGL GLUT window
    glutInitWindowPosition(windowPosX, windowPosY);
    glutInitWindowSize(WINDOW_SIZE_W, WINDOW_SIZE_H);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(updateGraphics);
    glutKeyboardFunc(keySelect);
    glutReshapeFunc(resizeWindow);
    glutSetWindowTitle("CHAI 3D");

    // create a mouse menu (right button)
    glutCreateMenu(menuSelect);
    glutAddMenuEntry("full screen", OPTION_FULLSCREEN);
    glutAddMenuEntry("window display", OPTION_WINDOWDISPLAY);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    //-----------------------------------------------------------------------
    // START SIMULATION
    //-----------------------------------------------------------------------

    // simulation in now running
    simulationRunning = true;

    // create a thread which starts the main haptics rendering loop
    cThread* hapticsThread = new cThread();
    hapticsThread->set(updateHaptics, CHAI_THREAD_PRIORITY_HAPTICS);

    // start the main graphics rendering loop
    glutMainLoop();

    // close everything
    close();

    // exit
    return (0);
}

//---------------------------------------------------------------------------

void resizeWindow(int w, int h)
{
    // update the size of the viewport
    displayW = w;
    displayH = h;
    glViewport(0, 0, displayW, displayH);

    // update texture coordinates
    double txMin, txMax, tyMin, tyMax;
    if (displayW >= displayH)
    {
        double ratio = (double)displayW / (double)displayH;
        txMin = 0.5 * (ratio - 1.0) / ratio;
        txMax = 1.0 - txMin;
        tyMin = 0.0;
        tyMax = 1.0;
    }
    else
    {
        double ratio = (double)displayH / (double)displayW;
        txMin = 0.0;
        txMax = 1.0;
        tyMin = 0.5 * (ratio - 1.0) / ratio;
        tyMax = 1.0 - tyMin;
    }

    // update texture coordinates
    for (int i=0; i<6; i++)
    {
        object->getVertex(vertices[i][0])->setTexCoord(txMin, tyMin);
        object->getVertex(vertices[i][1])->setTexCoord(txMax, tyMin);
        object->getVertex(vertices[i][2])->setTexCoord(txMax, tyMax);
        object->getVertex(vertices[i][3])->setTexCoord(txMin, tyMax);
    }
}

//---------------------------------------------------------------------------

void keySelect(unsigned char key, int x, int y)
{
    // escape key
    if ((key == 27) || (key == 'x'))
    {
        // close everything
        close();

        // exit application
        exit(0);
    }
}

//---------------------------------------------------------------------------

void menuSelect(int value)
{
    switch (value)
    {
        // enable full screen display
        case OPTION_FULLSCREEN:
            glutFullScreen();
            break;

        // reshape window to original size
        case OPTION_WINDOWDISPLAY:
            glutReshapeWindow(WINDOW_SIZE_W, WINDOW_SIZE_H);
            break;
    }
}

//---------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close haptic device
    tool->stop();
}

//---------------------------------------------------------------------------
void changeTexelColor(GLuint id, GLint x, GLint y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	uint8_t data[4];
	data[0] = r;
	data[1] = g;
	data[2] = b;
	data[3] = a;
	glBindTexture(GL_TEXTURE_2D, id);
	glTexSubImage2D(GL_TEXTURE_2D,
                	0,
                	x,
                	y,
                	1,
                	1,
                	GL_RGBA,
                	GL_UNSIGNED_BYTE,
                	data);
}
//---------------------------------------------------------------------------

void printNumbers(cTexture2D* texture, int x, int y, int number)
{
	//get the number of digits
	int  countDigits = x ? static_cast<int>(log10(static_cast<float>(number))) + 1 : 1;
	int i = 0;
	int j,k = 0;
	int lengthDash = 20;
	int widthDash = 3;
	cColorb color = cColorb(0.5, 1, 1);
	for (i=0; i<=countDigits; i++)
	{
		//get the current digit
		int digit = (number / (int)pow(10.0,i)) % 10;
		switch(digit)
		{
		case 0: {
			//draw the line
			for (j=0; j<=lengthDash;j++)
			{
				//lines on top and bottom
				int m,n = 0;
				for(m=0;m<3;m++)
				{
					if (m!=1)texture->m_image.setPixelColor(x + j + (i*11),y+(m*10),color);
				}
				//lines on the side
				m=0;
				// two lines
				for(m=0;m<2;m++)
				{
					//two sides
					for (n=0;n<2;n++)
					{
						texture->m_image.setPixelColor(x+(n*lengthDash)+(i*(lengthDash+1)),y+((m*(lengthDash+1))+1),color);
					}
				}
			}
				}
		case 1: {
			for (j=0; j<=lengthDash;j++)
			{
				int m,n = 0;
				//lines on the side
				m=0;
				// two lines
				for(m=0;m<2;m++)
				{
					for (n=0;n<widthDash;n++)
					{
						//(x)Position of Start + offset for pos. of dashes + position of digit in number + thikness of line
						texture->m_image.setPixelColor(x+lengthDash+(i*11)+n,y+((m*11)+1),cColorb(1, 1, 1));
					}
				}
			}
				}
		case 2: {
				}
		case 3: {
				}
		case 4: {
				}
		case 5: {
				}
		case 6: {
				}
		case 7: {
				}
		case 8: {
				}
		case 9: {
				}
		}
	}
}

void printNumbers(cBitmap* texture, int y, int x, int number)
	{
	int i = 0;
	//int j,k = 0;
	int lengthDash = 10;
	int widthDash = 2;
	cColorb color = cColorb(0.5, 1, 1);
	
	//count the number of digits
		int countDigits = 0;
		if (number == 0)countDigits = 1;
		else countDigits = x ? static_cast<int>(log10(static_cast<float>(number))) + 1 : 1; //compare performance
	if (flag !=1)printf ("countDigits = %d\n",countDigits);

	// vars for tokenizing int number
	std::stack<int> digitstack;
	int dig = 0;
	//tokenize int number (puts them on a stack)
	if (number != 0)
	{
		while(number>0)
		{                         
			dig = number % 10;
			number = number / 10;
			digitstack.push(dig);
			printf("digit:%d\n",dig);
			// count the digits
			//countDigits=countDigits++; /// see if more performant
		}
	}
	else
		// add a zero to the stack so we can print one
		digitstack.push(0);
	int posx = 40;
	int posy = 40;

	for (i=1; i<=countDigits; i++)
	{
		//get the current digit
		int digit = digitstack.top();
		digitstack.pop();
		if (flag!=1)printf ("digit is = %d\n",digit);
		switch(digit)
		{
			///texture->m_image.setPixelColor(x+lengthDash+(i*11)+n,y+((m*11)+1),cColorb(1, 1, 1)); // for reference
		case 0:
			drawLines(texture,1,2,3,0,5,6,7,&posx,&posy);
			break;
		case 1:
			drawLines(texture,0,0,3,0,0,6,0,&posx,&posy);
			break;
		case 2:
			drawLines(texture,1,0,3,4,5,0,7,&posx,&posy);
			break;
		case 3:
			drawLines(texture,1,0,3,4,0,6,7,&posx,&posy);
			break;
		case 4:
			drawLines(texture,0,2,3,4,0,6,0,&posx,&posy);
			break;
		case 5:
			drawLines(texture,1,2,0,4,0,6,7,&posx,&posy);
			break;
		case 6:
			drawLines(texture,1,2,0,4,5,6,7,&posx,&posy);
			break;
		case 7:
			drawLines(texture,1,0,3,0,0,6,0,&posx,&posy);
			break;
		case 8:
			drawLines(texture,1,2,3,4,5,6,7,&posx,&posy);
			break;
		case 9:
			drawLines(texture,1,2,3,4,0,6,0,&posx,&posy);
			break;
		}
		posx= posx + 18;
	}
	flag = 1;
}

void drawLines(cBitmap* texture,int l1,int l2,int l3,int l4,int l5,int l6,int l7,int *posx, int *posy)
{
	int i = 0;
	printf("values are: %d,%d,%d,%d,%d,%d,%d\n",l1,l2,l3,l4,l5,l6,l7);
	if (l1 != 0)
	{
		for (i=2;i<12;i++)
		{
			texture->m_image.setPixelColor(*posx + i,*posy+28,cColorb(1, 1, 1));
		}
	}
	if (l2 != 0)
	{
		for (i=16;i<26;i++)
		{
			texture->m_image.setPixelColor(*posx,*posy + i,cColorb(1, 1, 1));
		}
	}
	if (l3 != 0)
	{
		for (i=16;i<26;i++)
		{
			texture->m_image.setPixelColor(*posx+14,*posy + i,cColorb(1, 1, 1));
		}
	}
	if (l4 != 0)
	{
		for (i=2;i<12;i++)
		{
			texture->m_image.setPixelColor(*posx + i,*posy+14,cColorb(1, 1, 1));
		}
	}
	if (l5 != 0)
	{
		for (i=2;i<12;i++)
		{
			texture->m_image.setPixelColor(*posx,*posy + i,cColorb(1, 1, 1));
		}
	}
	if (l6 != 0)
	{
		for (i=2;i<12;i++)
		{
			texture->m_image.setPixelColor(*posx+14,*posy + i,cColorb(1, 1, 1));
		}
	}
	if (l7 != 0)
	{
		for (i=2;i<12;i++)
		{
			texture->m_image.setPixelColor(*posx + i,*posy,cColorb(1, 1, 1));
		}
	}

}

void updateGraphics(void)
{
	// render world
	camera->renderView(displayW, displayH);

	// copy output data to texture
	camera->copyImageData(&texture->m_image);

	//set up vars
	/*cCollisionRecorder m_collisionRecorderConstraint0;
	cCollisionEvent* m_contactPoint0;
	m_contactPoint0 = &(m_collisionRecorderConstraint0.m_nearestCollision);

	// get vertices and normals related to contact triangle
	cVector3d vertex0 = cAdd(m_contactPoint0->m_object->getGlobalPos(), cMul(m_contactPoint0->m_object->getGlobalRot(), m_contactPoint0->m_triangle->getVertex0()->getPos()));
	cVector3d vertex1 = cAdd(m_contactPoint0->m_object->getGlobalPos(), cMul(m_contactPoint0->m_object->getGlobalRot(), m_contactPoint0->m_triangle->getVertex1()->getPos()));
	cVector3d vertex2 = cAdd(m_contactPoint0->m_object->getGlobalPos(), cMul(m_contactPoint0->m_object->getGlobalRot(), m_contactPoint0->m_triangle->getVertex2()->getPos()));
	cVector3d normal0 = cMul(m_contactPoint0->m_object->getGlobalRot(), m_contactPoint0->m_triangle->getVertex0()->getNormal());
	cVector3d normal1 = cMul(m_contactPoint0->m_object->getGlobalRot(), m_contactPoint0->m_triangle->getVertex1()->getNormal());
	cVector3d normal2 = cMul(m_contactPoint0->m_object->getGlobalRot(), m_contactPoint0->m_triangle->getVertex2()->getNormal());*/

	/*//make a quare of the pixels black
	int i,j =0;
	for(j=10; j < 90; j++)
	{
		for(i=10; i < 90; i++)
		{
			texture->m_image.setPixelColor(j,i,cColorb(1, 1, 1));
		}
	}*/
	//printNumbers(texture,100,100,11);
	texture->markForUpdate();
	
    

	// Swap buffers
	glutSwapBuffers();

	// check for any OpenGL errors
	GLenum err;
	err = glGetError();
	if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

	// inform the GLUT window to call updateGraphics again (next frame)
	if (simulationRunning)
	{
    	glutPostRedisplay();
	}
}

//---------------------------------------------------------------------------

void updateHaptics(void)
{
    // reset clock
    simClock.reset();

    // main haptic simulation loop
    while(simulationRunning)
    {
        // compute global reference frames for each object
        world->computeGlobalPositions(true);

        // update position and orientation of tool
        tool->updatePose();

        // compute interaction forces
        tool->computeInteractionForces();

        // send forces to device
        tool->applyForces();

        // stop the simulation clock
        simClock.stop();

        // read the time increment in seconds
        double timeInterval = simClock.getCurrentTimeSeconds();

        // restart the simulation clock
        simClock.reset();
        simClock.start();

        // temp variable to compute rotational acceleration
        cVector3d rotAcc(0,0,0);

        // check if tool is touching an object
		cGenericObject* objectContact1 = tool->m_proxyPointForceModel->m_contactPoint0->m_object;
        cGenericObject* objectContact = tool->m_proxyPointForceModel->m_contactPoint0->m_object;
        if (objectContact != NULL)
        {
            // retrieve the root of the object mesh
            cGenericObject* obj = objectContact->getSuperParent();

            // get position of cursor in global coordinates
            cVector3d toolPos = tool->m_deviceGlobalPos;

            // get position of object in global coordinates
            cVector3d objectPos = obj->getGlobalPos();

            // compute a vector from the center of mass of the object (point of rotation) to the tool
            cVector3d vObjectCMToTool = cSub(toolPos, objectPos);

            // compute acceleration based on the interaction forces
            // between the tool and the object
            if (vObjectCMToTool.length() > 0.0)
            {
                // get the last force applied to the cursor in global coordinates
                // we negate the result to obtain the opposite force that is applied on the
                // object
                cVector3d toolForce = cNegate(tool->m_lastComputedGlobalForce);

                // compute effective force to take into account the fact the object
                // can only rotate around a its center mass and not translate
                cVector3d effectiveForce = toolForce - cProject(toolForce, vObjectCMToTool);

                // compute the resulting torque
                cVector3d torque = cMul(vObjectCMToTool.length(), cCross( cNormalize(vObjectCMToTool), effectiveForce));

                // update rotational acceleration
                const double OBJECT_INERTIA = 0.4;
                rotAcc = (1.0 / OBJECT_INERTIA) * torque;
            }
        }

        // update rotational velocity
        rotVel.add(timeInterval * rotAcc);

        // set a threshold on the rotational velocity term
        const double ROT_VEL_MAX = 10.0;
        double velMag = rotVel.length();
        if (velMag > ROT_VEL_MAX)
        {
            rotVel.mul(ROT_VEL_MAX / velMag);
        }

        // add some damping too
        const double DAMPING_GAIN = 0.1;
        rotVel.mul(1.0 - DAMPING_GAIN * timeInterval);

        // if user switch is pressed, set velocity to zero
        if (tool->getUserSwitch(0) == 1)
        {
            rotVel.zero();
        }

        // compute the next rotation configuration of the object
        if (rotVel.length() > CHAI_SMALL)
        {
            object->rotate(cNormalize(rotVel), timeInterval * rotVel.length());
        }
    }
    
    // exit haptics thread
    simulationFinished = true;
}

//---------------------------------------------------------------------------
