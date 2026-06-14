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
 * NAME:    reduce.h ( CE Library, OpenCL)
 *
 * COMMENTS: Reduction helper functions.
 */

#ifndef __REDUCE_H__
#define __REDUCE_H__

#include "platform.h"
#include "typedefines.h"

// Reduce ops
#define OP_SUM 0
#define OP_MIN 1
#define OP_MAX 2

// Unary map ops
#define MAP_IDENT   0
#define MAP_SQR     1
#define MAP_ABS     2

// Binary map ops
#define MAP_MULT    3
#define MAP_ADD     4

// Accumulate in double precision unless not available.
#ifdef NO_DOUBLE_SUPPORT
typedef float accum_t;
typedef float2 accum2_t;
typedef float3 accum3_t;
typedef float4 accum4_t;
#define ACCUM_PREC 32
#else
#pragma OPENCL EXTENSION cl_khr_fp64: enable
typedef double accum_t;
typedef double2 accum2_t;
typedef double3 accum3_t;
typedef double4 accum4_t;
#define ACCUM_PREC 64
#endif

// Conversion functions to/from fpreal3.
#if (FPREAL_PREC==32 && ACCUM_PREC==32) || \
    (FPREAL_PREC==64 && ACCUM_PREC==64)

// same-bit fpreal3 to same-bit accum (no-op).
#define toaccum3( x ) x
// same-bit accum3 to same-bit accum (no-op).
#define tofpreal3( x ) x

#else 

// differing bit fpreal3 to accum3.
static accum3_t
toaccum3(fpreal3 v)
{
    return (accum3_t)(v.x, v.y, v.z);
}

// differing bit accum3 to fpreal3.
static fpreal3
tofpreal3(accum3_t v)
{
    return (fpreal3)(v.x, v.y, v.z);
}

#endif

static accum_t 
reduce(accum_t a, accum_t b)
{
#if REDUCEOP == OP_SUM
    return a + b;
#elif REDUCEOP == OP_MIN
    return fmin(a, b);
#elif REDUCEOP == OP_MAX
    return fmax(a, b);
#endif
    return a;
}

static accum_t 
identity()
{
#if REDUCEOP == OP_SUM
    return 0;
#elif REDUCEOP == OP_MIN
    return FLT_MAX;
#elif REDUCEOP == OP_MAX
    return -FLT_MAX;
#endif
    return 0;
}

static accum_t 
map(accum_t a)
{
#if MAPOP == MAP_IDENT
    return a;
#elif MAPOP == MAP_SQR
    return a * a;
#elif MAPOP == MAP_ABS
    return fabs(a);
#endif
    return a;
}

static accum_t 
mapBinary(accum_t a, accum_t b)
{
#if MAPOP == MAP_MULT
    return a * b;
#elif MAPOP == MAP_ADD
    return a + b;
#endif
    return a;
}

static void
reduceLocal(__local accum_t *ldata, size_t lidx, uint groupsize)
{
    for(size_t offset = groupsize / 2; offset > 0; offset >>= 1)
    {
        if (lidx < offset)
            ldata[lidx] = reduce(ldata[lidx], ldata[lidx + offset]);
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}

// Since there's not built in 3-tuple version of reduce_add.
#ifdef __opencl_c_work_group_collective_functions
static accum3_t
work_group_reduce_add3(accum3_t v)
{
    return (accum3_t)(work_group_reduce_add(v.x),
                      work_group_reduce_add(v.y),
                      work_group_reduce_add(v.z));
}
#endif


#endif
