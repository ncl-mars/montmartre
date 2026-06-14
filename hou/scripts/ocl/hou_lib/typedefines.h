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
 * NAME:    typedefines.h ( CE Library, OpenCL)
 *
 * COMMENTS: OpenCL type definitions
 */


#ifndef __TYPE_DEFINE_H__
#define __TYPE_DEFINE_H__

// The OpenCL SOP/DOP might define
// these before compilation for 32-/64-bit support,
// so only define them if not already defined.
#ifndef fpreal
// The USE_DOUBLE flag is used by the older OpenCL-enabled
// DOPs that make up the Pyro solver.
#ifdef USE_DOUBLE
#pragma OPENCL EXTENSION cl_khr_fp64: enable
#define fpreal double
#define fpreal2 double2
#define fpreal3 double3
#define fpreal4 double4
#define fpreal8 double8
#define fpreal16 double16
#define FPREAL_PREC 64
// We also want to define exint as long in this case.
#define USE_LONG

#else
#define fpreal float
#define fpreal2 float2
#define fpreal3 float3
#define fpreal4 float4
#define fpreal8 float8
#define fpreal16 float16
#define FPREAL_PREC 32

#endif
#endif


#if FPREAL_PREC==64

// Load a 64-bit fpreal2 from a float2 buffer.
static fpreal2
vload2f(size_t i, const global float *b)
{
    i *= 2;
    return (fpreal2)(b[i], b[i + 1]);
}

// Load a 64-bit fpreal3 from a float3 buffer.
static fpreal3
vload3f(size_t i, const global float *b)
{
    i *= 3;
    return (fpreal3)(b[i], b[i+1], b[i+2]);
}

// Load a 64-bit fpreal4 from a float4 buffer.
static fpreal4
vload4f(size_t i, const global float *b)
{
    i *= 4;
    return (fpreal4)(b[i], b[i+1], b[i+2], b[i+3]);
}

// Store a 64-bit fpreal3 into a float3 buffer.
static void
vstore3f(fpreal3 a, size_t i, global float *b)
{
    vstore3((float3)(a.x, a.y, a.z), i, b);
}

// Store a 64-bit fpreal4 into a float4 buffer.
static void
vstore4f(fpreal4 a, size_t i, global float *b)
{
    vstore4((float4)(a.x, a.y, a.z, a.w), i, b);
}

#else

// Load a 32-bit fpreal2 from a float2 buffer (no-op).
#define vload2f vload2

// Load a 32-bit fpreal3 from a float3 buffer (no-op).
#define vload3f vload3

// Load a 32-bit fpreal4 from a float4 buffer (no-op).
#define vload4f vload4

// Store a 32-bit fpreal3 into a float3 buffer (no-op).
#define vstore3f vstore3

// Store a 32-bit fpreal4 into a float4 buffer (no-op).
#define vstore4f vstore4

#endif

// The OpenCL SOP/DOP might define
// these before compilation for 32-/64-bit support,
// so only define them if not already defined.
#ifndef exint
#ifdef USE_LONG
#define exint  long
#define exint2 long2
#define exint3 long3
#define exint4 long4
#else
#define exint  int
#define exint2 int2
#define exint3 int3
#define exint4 int4
#endif
#endif

#endif
