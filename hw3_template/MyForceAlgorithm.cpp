//===========================================================================
/*
    CS277 - Experimental Haptics
    Winter 2010, Stanford University

    This class extends the cProxyPointForceAlgo class in CHAI3D that
    implements the god-object/finger-proxy haptic rendering algorithm.
    It allows us to modify or recompute the force that is ultimately sent
    to the haptic device.

    Your job for this assignment is to implement the updateForce() method
    in this class to support for two new effects: force shading and haptic
    textures.  Methods for both are described in Ho et al. 1999.
*/
//===========================================================================

#include "MyForceAlgorithm.h"

// --------------------------------------------------------------------------
// Helper function to compute the intensity of a point on an image given
// (u,v) coordinates in the range [0,1], using bilinear interpolation
double MyForceAlgorithm::imageIntensityAt(cImageLoader *image, double u, double v)
{
    // get image width and height
    int w = image->getWidth();
    int h = image->getHeight();
    
    // get the four integer coordinates that bound our query point (u,v)
    int ux0 = cClamp(int(w*u), 0, w-1), ux1 = cClamp(int(w*u)+1, 0, w-1);
    int vx0 = cClamp(int(h*v), 0, h-1), vx1 = cClamp(int(h*v)+1, 0, h-1);

    // retrieve the four pixel values from the image
    int bytes = image->getFormat() == GL_RGB ? 3 : 4;
    int offset[2][2] = {
        { (vx0*w + ux0)*bytes, (vx1*w + ux0)*bytes },
        { (vx0*w + ux1)*bytes, (vx1*w + ux1)*bytes }
    };
    double intensity[2][2];
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j) {
            // assumes RGB image format, and average
            unsigned char *pixel = image->getData() + offset[i][j];
            intensity[i][j] = (double(pixel[0]) + double(pixel[1]) + double(pixel[2])) / 765.0;
        }

    // compute interpolation weights
    int uw = cClamp(w*u - ux0, 0.0, 1.0);
    int vw = cClamp(h*v - vx0, 0.0, 1.0);

    // perform bilinear interpolation and return value
    double i0 = cLerp(uw, intensity[0][0], intensity[1][0]);
    double i1 = cLerp(uw, intensity[0][1], intensity[1][1]);
    return cLerp(vw, i0, i1);
}

//===========================================================================
/*!
    This method uses the information computed earlier in
    computeNextProxyPosition() to determine the force to apply to the device.
    It first calls cProxyPointForceAlgo::updateForce() to compute the base
    force from contact geometry and the constrained proxy position.  That
    force can then be modified or recomputed in this function.

    Your implementation of force shading and haptic textures CS277 Homework #3
    will likely end up in this function.  When this function is called,
    collision detection has already been performed, and the proxy point has
    already been updated based on the constraints found.  Your job is to
    compute a force with all that information available to you.

    Useful variables to read:
        m_deviceGlobalPos   - current position of haptic device
        m_proxyGlboalPos    - computed position of the constrained proxy
        m_numContacts       - the number of surfaces constraining the proxy
        m_contactPoint0,1,2 - up to three cCollisionEvent structures carrying
                              very useful information about each contact

    Variables that this function should set/reset:
        m_normalForce       - computed force applied in the normal direction
        m_tangentialForce   - computed force along the tangent of the surface
        m_lastGlobalForce   - this is what the operator ultimately feels!!!
*/
//===========================================================================

void MyForceAlgorithm::updateForce()
{
    // get the base class to do basic force computation first
    cProxyPointForceAlgo::updateForce();

    // TODO: compute force shading and texture forces here

}
