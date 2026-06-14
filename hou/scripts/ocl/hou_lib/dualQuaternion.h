/*
 * PROPRIETARY INFORMATION.  This software is proprietary to
 * Side Effects Software Inc., and is not to be reproduced,
 * transmitted, or disclosed in any way without written permission.
 *
 * Produced by:
 *  Side Effects Software Inc
 *  123 Front Street West, Suite 1401
 *  Toronto, Ontario
 *  Canada   M5J 2M2
 *  416-504-9876
 *
 * NAME:    dualQuaternion.h ( CE Library, OpenCL)
 *
 * COMMENTS:
 */

#ifndef __DUAL_QUATERNION_H__
#define __DUAL_QUATERNION_H__

#include "quaternion.h"

static void 
dualQuatNormalize(fpreal8 *dst,
		  const fpreal8 *dq)
{
    fpreal4 dq_real = dq->lo;
    fpreal4 dq_dual = dq->hi;

    private fpreal lenSquared = dot(dq_real, dq_real);
    if(lenSquared > 0)
    {
	lenSquared = sqrt(lenSquared);
	dq_real /= lenSquared;
	dq_dual /= lenSquared;
    }

    *dst = (fpreal8)(dq_real, dq_dual);
}

static void 
dualQuatTransform(fpreal3 *dst,
		  const fpreal8 *dq,
		  const fpreal3 *pt)
{
    // dq transform deals with two rigid body transforms: translating and Rotating
    fpreal4 dq_real = dq->lo;
    fpreal4 dq_dual = dq->hi;

    // Rotating the point first
    fpreal3 rotated_pt;
    quatRotate(&rotated_pt, &dq_real, pt);
   
    // Translating the point 
    fpreal4 cross_real_dual = cross(dq_real, dq_dual);
    fpreal4 translation = (dq_dual * dq_real.w - dq_real * dq_dual.w + cross_real_dual) * (fpreal)2;
    (*dst) = rotated_pt.xyz + translation.xyz;
}

static void 
dualQuatRotate(fpreal3 *dst,
	       const fpreal8 *dq,
	       const fpreal3 *pt)
{
    fpreal4 dq_real = dq->lo;
    quatRotate(dst, &dq_real, pt);
}

#endif
