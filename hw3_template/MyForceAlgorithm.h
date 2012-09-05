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

#ifndef MYFORCEALGORITHM_H
#define MYFORCEALGORITHM_H

#include "chai3d.h"

// --------------------------------------------------------------------------

class MyForceAlgorithm : public cProxyPointForceAlgo
{
    cImageLoader *m_heightMaps;

protected:
    //! Compute force to apply to device.
    virtual void updateForce();

public:
    //! Helper function to compute the intensity of a point on an image given
    //  (u,v) coordinates in the range [0,1], using bilinear interpolation
    static double imageIntensityAt(cImageLoader *image, double u, double v);
};

// --------------------------------------------------------------------------
#endif
