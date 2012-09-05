//===========================================================================
/*
    CS277 - Experimental Haptics
    Winter 2012, Stanford University

    You may use this program as a boilerplate for starting your third
    homework.  Use CMake (www.cmake.org) on the CMakeLists.txt file to
    generate project files for the development tool of your choice.  The
    CHAI3D library directory (chai3d-2.1.0) should be installed as a sibling
    directory to the one containing this project.

    \author    Francois Conti & Sonny Chan
    \date      February 2010
*/
//===========================================================================

//---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//---------------------------------------------------------------------------
#include "chai3d.h"
#include "MyForceAlgorithm.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------

// initial size (width/height) in pixels of the display window
const int WINDOW_SIZE_W         = 800;
const int WINDOW_SIZE_H         = 600;

// mouse menu options (right button)
const int OPTION_FULLSCREEN     = 1;
const int OPTION_WINDOWDISPLAY  = 2;

//---------------------------------------------------------------------------
// DECLARED VARIABLES
//---------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera that renders the world in a window display
cCamera* camera;

// a light source to illuminate the objects in the virtual scene
cLight *light;

// width and height of the current window display
int displayW  = 0;
int displayH  = 0;

// a haptic device handler
cHapticDeviceHandler* handler;

// a virtual tool representing the haptic device in the scene
cGeneric3dofPointer* tool;

// haptic device workspace radius
double deviceRadius = 0.0;

// virtual workspace radius
double workspaceRadius = 2.0;

// define bounding box for world
cVector3d minCorner;
cVector3d maxCorner;

// camera speed when workspace is moved
double camSpeed=0.005;

// the four objects we wish to render for this scene
cMesh* object[4];

// label to show estimate of haptic update rate
cLabel* rateLabel;
double rateEstimate = 0;

// normal line
cShapeLine* normalLine = 0;

// label to show position of haptic device
cLabel* positionLabel = 0;

// status of the main simulation haptics loop
bool simulationRunning = false;

// has exited haptics simulation thread
bool simulationFinished = false;

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

// create an instance of the simple object we'd like to render
cMesh* createObject();

// main graphics callback
void updateGraphics(void);

// main haptics loop
void updateHaptics(void);

//===========================================================================
/*
    This application illustrates the use of the haptic device handler
    "cHapticDevicehandler" to access haptic devices connected to the computer.

    In this example the application opens an OpenGL window and displays a
    3D cursor for the first device found. If the operator presses the device
    user button, the color of the cursor changes accordingly.

    In the main haptics loop function  "updateHaptics()" , the position and 
    user switch status of the device are retrieved at each simulation iteration.
    This information is then used to update the position and color of the
    cursor. A force is then commanded to the haptic device to attract the 
    end-effector towards the device origin.
*/
//===========================================================================

int main(int argc, char* argv[])
{
    //-----------------------------------------------------------------------
    // INITIALIZATION
    //-----------------------------------------------------------------------

    printf ("\n");
    printf ("-----------------------------------\n");
    printf ("CS277 - Experimental Haptics\n");
    printf ("Homework #4 Boilerplate Application\n");
    printf ("February 2010, Stanford University\n");
    printf ("-----------------------------------\n");
    printf ("\n\n");

    //-----------------------------------------------------------------------
    // 3D - SCENEGRAPH
    //-----------------------------------------------------------------------

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    // the color is defined by its (R,G,B) components.
    world->setBackgroundColor(0.2, 0.2, 0.2);

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and orient the camera
    camera->set( cVector3d (3.0, 0.0, 1.0),     // camera position (eye)
                 cVector3d (0.0, 0.0,-1.0),    // lookat position (target)
                 cVector3d (0.0, 0.0, 1.0));   // direction of the "up" vector

    // set the near and far clipping planes of the camera
    // anything in front/behind these clipping planes will not be rendered
    camera->setClippingPlanes(0.01, 20.0);

    // enable higher quality rendering for transparent objects
    camera->enableMultipassTransparency(true);

    // create a light source and attach it to the camera
    light = new cLight(world);
    camera->addChild(light);                   // attach light to camera
    light->setEnabled(true);                   // enable light source
    light->setPos(cVector3d( 2.0, 3.0, 8.0));  // position the light source

    // create a label that shows the haptic loop update rate
    rateLabel = new cLabel();
    rateLabel->setPos(8, 24, 0);
    camera->m_front_2Dscene.addChild(rateLabel);

    // create a label that shows device position
    positionLabel = new cLabel();
    positionLabel->setPos(8,8,0);
    camera->m_front_2Dscene.addChild(positionLabel);

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
	deviceRadius = info.m_workspaceRadius;
    }

    // create a 3D tool and add it to the world
    tool = new cGeneric3dofPointer(world);
    world->addChild(tool);

    // connect the haptic device to the tool
    tool->setHapticDevice(hapticDevice);

    // initialize tool by connecting to haptic device
    tool->start();

    // map the physical workspace of the haptic device to a larger virtual workspace.
    tool->setWorkspaceRadius(workspaceRadius);

    // define a radius for the tool
    tool->setRadius(0.05);

    // ***** insert our own force rendering algorithm for the tool *****
    tool->m_proxyPointForceModel = new MyForceAlgorithm();
    tool->m_proxyPointForceModel->initialize(world, cVector3d(0,0,0));
    tool->m_proxyPointForceModel->setProxyRadius(0.0);
    tool->m_proxyPointForceModel->m_collisionSettings.m_checkBothSidesOfTriangles = false;

    // enable if objects in the scene are going to rotate or translate
    // or possibly collide against the tool. If the environment
    // is entirely static, you can set this parameter to "false"
    tool->m_proxyPointForceModel->m_useDynamicProxy = true;

	normalLine = new cShapeLine(tool->getDeviceGlobalPos(), cVector3d(0.0,0.0,0.05));
	world->addChild(normalLine);

    // read the scale factor between the physical workspace of the haptic
    // device and the virtual workspace defined for the tool
    double workspaceScaleFactor = tool->getWorkspaceScaleFactor();

    // define a maximum stiffness that can be handled by the current
    // haptic device. The value is scaled to take into account the
    // workspace scale factor
    double stiffnessMax = info.m_maxForceStiffness / workspaceScaleFactor;
    double forceMax = info.m_maxForce;

    // define the maximum damping factor that can be handled by the
    // current haptic device. The The value is scaled to take into account the
    // workspace scale factor
    double dampingMax = info.m_maxLinearDamping / workspaceScaleFactor;


    //-----------------------------------------------------------------------
    // TEXTURED OBJECTS
    //-----------------------------------------------------------------------

    // metal object
    object[0] = createObject();
    object[0]->translate(0, 0, -1);
    object[0]->m_material.setStiffness(0.5 * stiffnessMax);
    cTexture2D *metal = new cTexture2D();
    metal->loadFromFile("tex_metal.tga");
    object[0]->setTexture(metal);
    object[0]->setUseTexture(true);
    world->addChild(object[0]);
	
    // rubber object
    object[1] = createObject();
    object[1]->translate(0, 5, -1);
    object[1]->m_material.setStiffness(0.5 * stiffnessMax);
    cTexture2D *rubber = new cTexture2D();
    rubber->loadFromFile("tex_rubber.tga");
    object[1]->setTexture(rubber);
    object[1]->setUseTexture(true);
    world->addChild(object[1]);

    // wood object
    object[2] = createObject();
    object[2]->translate(-4, 0, -1);
    object[2]->m_material.setStiffness(0.5 * stiffnessMax);
    cTexture2D *wood = new cTexture2D();
    wood->loadFromFile("tex_wood.tga");
    object[2]->setTexture(wood);
    object[2]->setUseTexture(true);
    world->addChild(object[2]);

    // your own object
    object[3] = createObject();
    object[3]->translate(-4, 5, -1);
    object[3]->m_material.setStiffness(0.5 * stiffnessMax);
    cTexture2D *unknown = new cTexture2D();
    unknown->loadFromFile("tex_unknown.tga");
    object[3]->setTexture(unknown);
    object[3]->setUseTexture(true);
    world->addChild(object[3]);

    // define bounding box for world
    minCorner = cVector3d(-5, -5, -2);
    maxCorner = cVector3d(4, 7, 3);

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

    // close the haptic devices
    tool->stop();
}

//---------------------------------------------------------------------------

cMesh* createObject()
{
    cMesh *mesh = new cMesh(world);

    // vertex coordinates
    double vcoord[12][3] = {
        {  1, -2,  1 }, {  1, -1,  0 }, {  1, -1,  0 }, {  1,  1,  0 }, {  1,  1,  0 }, {  1,  2,  1 },
        { -1, -2,  1 }, { -1, -1,  0 }, { -1, -1,  0 }, { -1,  1,  0 }, { -1,  1,  0 }, { -1,  2,  1 }
    };
    // texture coordinates
    double tcoord[12][2] = {
        { .3, 0 }, { 1, 0 }, { 0, 0 }, { 1, 0 }, { 0, 0 }, { .7, 0 },
        { .3, 1 }, { 1, 1 }, { 0, 1 }, { 1, 1 }, { 0, 1 }, { .7, 1 }
    };
    // vertex numbers
    unsigned int vnum[12];

    // faces
    int face[6][3] = {
        {  0,  1,  7 }, {  7,  6,  0 },
        {  2,  3,  9 }, {  9,  8,  2 },
        {  4,  5, 11 }, { 11, 10,  4 }
    };

    // create the vertices and attach properties
    for (int i = 0; i < 12; ++i)
    {
        // set position
        vnum[i] = mesh->newVertex(vcoord[i][0], vcoord[i][1], vcoord[i][2]);

        // compute the normal
        cVertex *v = mesh->getVertex(vnum[i]);
        cVector3d c(v->getPos().x, 0, 2);
        v->setNormal(cNormalize(c - v->getPos()));

        // set texture coordinate
        v->setTexCoord(tcoord[i][0], tcoord[i][1]);
    }

    // create the faces of the half-pipe from the vertices
    for (int i = 0; i < 6; ++i)
        mesh->newTriangle(vnum[face[i][0]], vnum[face[i][1]], vnum[face[i][2]]);

    // set the underlying material to be a bright white
    mesh->m_material.m_ambient.set(0.3, 0.3, 0.3);
    mesh->m_material.m_diffuse.set(1.0, 1.0, 1.0);

    // some extra settings in the mesh
    mesh->computeBoundaryBox();
    mesh->setShowNormals(true);
    mesh->setUseCulling(false);

    return mesh;
}
//---------------------------------------------------------------------------

void updateGraphics(void)
{

    // update the label with the haptic refresh rate
    char buffer[256];
    sprintf(buffer, "haptic rate: %.0lf Hz", rateEstimate);
    rateLabel->m_string = buffer;

    // update position of tool
    cVector3d toolLocalPos = tool->getDeviceLocalPos();
    cVector3d toolPos = tool->getDeviceGlobalPos();
    sprintf(buffer, "global pos: (%.4lf, %.4lf, %.4lf),  local pos: (%.4lf, %.4lf, %.4lf)", toolPos.x, toolPos.y, toolPos.z, toolLocalPos.x, toolLocalPos.y, toolLocalPos.z );
    positionLabel->m_string = buffer;

    // check if tool is pushing against device radius and if so, update camera position
    if (toolLocalPos.length() > workspaceRadius - 0.2) {
      if (toolPos.x > minCorner.x && toolPos.y > minCorner.y && toolPos.z > minCorner.z
	  && toolPos.x < maxCorner.x && toolPos.y < maxCorner.y && toolPos.z < maxCorner.z) {
		toolPos.normalize();     
		camera->setPos(camera->getPos() + camSpeed * toolLocalPos);
		tool->setPos(tool->getPos() + camSpeed * toolLocalPos);
      }
    }
    double scale = 0.1;
    normalLine->m_pointA = tool->getProxyGlobalPos();
    normalLine->m_pointB = tool->getProxyGlobalPos() + scale*tool->m_proxyPointForceModel->getNormalForce();

    // render world
    camera->renderView(displayW, displayH);

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
    // a clock to estimate the haptic simulation loop update rate
    cPrecisionClock pclock;
    pclock.setTimeoutPeriodSeconds(1.0);
    pclock.start(true);
    int counter = 0;

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

        // estimate the refresh rate
        ++counter;
        if (pclock.timeoutOccurred()) {
            pclock.stop();
            rateEstimate = counter;
            counter = 0;
            pclock.start(true);
        }
    }
    
    // exit haptics thread
    simulationFinished = true;
}

//---------------------------------------------------------------------------
