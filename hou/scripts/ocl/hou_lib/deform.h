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
 * NAME:    deform.h ( CE Library, OpenCL)
 *
 * COMMENTS:
 */

#ifndef __DEFORM_H__
#define __DEFORM_H__

#include "typedefines.h"
#include "quaternion.h"

static void 
getWeightedLinearPoint(fpreal3* src,
		       fpreal3* dst,
		       global const fpreal* transforms,
		       global const fpreal* wi,
		       ulong max_num_weights,
		       size_t start_index)
{
     private bool has_weight = false;
    
     for (private size_t w = 0; w < max_num_weights; w++)
     {
         private const size_t wi_index = start_index + w;
         private const fpreal2 wi2 = vload2(wi_index, wi);
         private const size_t mat_index = (size_t) wi2.x;
         private const fpreal mat_weight = wi2.y;
         if (mat_weight == 0) continue; // The bone isn't influenced by this weight
 
         has_weight = true;
         private const fpreal16 mat = vload16(mat_index, transforms);
         private fpreal4 t_point = (*src).x * mat.lo.lo +
         			  (*src).y * mat.lo.hi +
         			  (*src).z * mat.hi.lo + 
         			  mat.hi.hi;
 
         t_point = t_point * mat_weight;
         (*dst) = (*dst) + t_point.xyz;
     }
     
      if (!has_weight) 
         (*dst) = (*src);
}

static void 
getWeightedLinearNormal(fpreal3* src,
			fpreal3* dst,
			global const fpreal* transforms,
			global const fpreal* wi,
			ulong max_num_weights,
			size_t start_index)
{
    bool has_weight = false;
    
    for (private size_t w = 0; w < max_num_weights; w++)
    {
	private const size_t wi_index = start_index + w;
	private const fpreal2 wi2 = vload2(wi_index, wi);
	private const size_t mat_index = (size_t) wi2.x;
	private const fpreal mat_weight = wi2.y;
	if( mat_weight == 0 ) continue;

	has_weight = true;
	prefetch(transforms, 9); // we're about to fetch nxn elements

	private const fpreal3 firstcol = vload3(mat_index * 3 + 0, transforms);
	private const fpreal3 secondcol = vload3(mat_index * 3 + 1, transforms);
	private const fpreal3 thirdcol = vload3(mat_index * 3 + 2, transforms);
    
	private fpreal3 t_normal = (*src).x * firstcol +
				   (*src).y * secondcol +
				   (*src).z * thirdcol; 

	t_normal = t_normal * mat_weight;
	(*dst) = (*dst) + t_normal;
    }

    if (!has_weight)
	(*dst) = (*src);
}

static void 
getWeightedLinearVector(fpreal3* src,
			fpreal3* dst,
			global const fpreal* transforms,
			global const fpreal* wi,
			ulong max_num_weights,
			size_t start_index)
{
    bool has_weight = false;

    for( private size_t w = 0; w < max_num_weights; w++ )
    {
	private const size_t wi_index = start_index + w;
	private const fpreal2 wi2 = vload2(wi_index, wi);
	private const size_t mat_index = (size_t) wi2.x;
	private const fpreal mat_weight = wi2.y;
	if( mat_weight == 0 ) continue;

	has_weight = true;
	prefetch(transforms, 4 * 4); // we're about to fetch nxn elements

	private const fpreal4 firstcol = vload4(mat_index * 4 + 0, transforms);
	private const fpreal4 secondcol = vload4(mat_index * 4 + 1, transforms);
	private const fpreal4 thirdcol = vload4(mat_index * 4 + 2, transforms);
    
	private fpreal4 t_normal = (*src).x * firstcol +
				   (*src).y * secondcol +
				   (*src).z * thirdcol; 

	t_normal = t_normal * mat_weight;
	(*dst) = (*dst) + t_normal.xyz;
    }

    if (!has_weight)
	(*dst) = (*src);
}

static void 
getWeightedLinearQuaternion(const fpreal4 *src,
			    fpreal4 *dst,
			    global const fpreal* xform,
			    global const fpreal* wi,
			    ulong max_num_weights,
			    size_t start_index)
{
    bool has_weight = false;
    private fpreal4 sum = (fpreal4)(0);

    for( private size_t w = 0; w < max_num_weights; w++ )
    {
	private const size_t wi_index = start_index + w;
	private const fpreal2 wi2 = vload2(wi_index, wi);
	private const size_t mat_index = (size_t) wi2.x;
	private fpreal mat_weight = wi2.y;
	if( mat_weight == 0 ) continue;
	
	private fpreal4 orig_xform = vload4(mat_index, xform);

	if(dot(orig_xform, (fpreal4)(0, 0, 0, 1)) < 0)
	    mat_weight *= (fpreal)-1;
	
	private fpreal4 t_quat = orig_xform * mat_weight;
	sum += t_quat;
    }
    
    // Multiplying quaternions
    sum = normalize(sum);

    quatMultiply(dst, src, &sum); 
}
			    

static fpreal8
dqDeformGetWeightedDQ(global const fpreal *wi,
		      global const fpreal *dqs,
		      const ulong max_num_weights,
		      const size_t wi_start,
		      const bool weighted_quat)
{
    private fpreal8 final_dq = (fpreal8)(0);
    private bool has_weight = false;
  
    const fpreal2 pivot_idx = vload2(wi_start, wi);
    const fpreal8 pivot_dq = vload8(pivot_idx.x, dqs);
    const fpreal4 pivot = pivot_dq.lo;

    for (private size_t w = 0; w < max_num_weights; w++)
    {
        private const size_t wi_index = wi_start + w;
        private const fpreal2 wi2 = vload2(wi_index, wi);
        private const size_t dq_index = (size_t) wi2.x;
        private fpreal dq_weight = wi2.y;
        private fpreal8 dq = vload8(dq_index, dqs);
    
	if (weighted_quat)
	{
	    if (dot(dq.lo, (fpreal4)(0, 0, 0, 1)) < 0.0f)
		dq_weight *= -1.0f;
	} 
	else if (dot(pivot, dq.lo) < 0.0f)
	{
	    dq_weight *= -1.0f;		
	}

        dq = dq * dq_weight;
        final_dq = final_dq + dq;
    }

    dualQuatNormalize(&final_dq, &final_dq);
    return final_dq;
}

#endif
		 
