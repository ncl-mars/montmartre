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
 * NAME:    feather.h ( CE Library, OpenCL)
 *
 * COMMENTS:
 */

#include "interpolate.h"

#ifndef __FEATHER_H__
#define __FEATHER_H__

#define FEATHER_ATTRIBCLASS_DETAIL 0
#define FEATHER_ATTRIBCLASS_PRIMITIVE 1
#define FEATHER_ATTRIBCLASS_POINT 2
#define FEATHER_ATTRIBCLASS_VERTEX 3

#define FEATHERPARM_ARGS(NAME) \
    , \
    float NAME##_parm, \
    int NAME##_options, \
    float NAME##_inrange1, \
    float NAME##_inrange2, \
    float NAME##_outrange1, \
    float NAME##_outrange2, \
    int NAME##_rampsize,  \
    constant float * restrict NAME##_ramp

#define FEATHERATTRIB_ARGS(NAME) \
    , \
    int NAME##_array_length, \
    int NAME##_array_tuplesize, \
    global float * restrict NAME##_array

#define FEATHERTEX_ARGS(NAME) \
    , \
    int NAME##_tex_stride_x, \
    int NAME##_tex_stride_y, \
    int NAME##_tex_stride_z, \
    int NAME##_tex_stride_offset, \
    int NAME##_tex_res_x, \
    int NAME##_tex_res_y, \
    int NAME##_tex_res_z, \
    global float * restrict NAME##_tex

#define MAKE_FEATHERATTRIB(NAME)\
struct featherattrib_t NAME##_parms = make_featherattrib(\
    NAME##_parm, \
    NAME##_options, \
    NAME##_inrange1, \
    NAME##_inrange2, \
    NAME##_outrange1, \
    NAME##_outrange2, \
    NAME##_rampsize, \
    NAME##_ramp); \

#define SET_FEATHERATTRIB_ARRAY(NAME, NPRIMS)\
set_featherattrib_array( \
    &NAME##_parms, \
    NAME##_array_length, \
    NAME##_array_tuplesize, \
    NAME##_array, \
    NAME##_array_length == NPRIMS ? 1 : 2);

#define SET_FEATHERATTRIB_TEX(NAME)\
set_featherattrib_tex( \
    &NAME##_parms, \
    NAME##_tex_stride_x, \
    NAME##_tex_stride_y, \
    NAME##_tex_stride_z, \
    NAME##_tex_stride_offset, \
    NAME##_tex_res_x, \
    NAME##_tex_res_y, \
    NAME##_tex_res_z, \
    NAME##_tex);

struct featherattrib_t
{
    float parmvalue;

    int options;
    float2 inrange;
    float2 outrange;

    int rampsize;
    constant float *restrict ramp;

    int length;
    int tuplesize;
    global float *restrict array;
    int attribclass;

    int3 tex_stride;
    int tex_stride_offset;
    int3 tex_res;
    global float *restrict tex;
};

struct featherattrib_t
make_featherattrib(
    float parmvalue,
    int options,
    float inrange1,
    float inrange2,
    float outrange1,
    float outrange2,
    int rampsize,
    constant float *restrict ramp)
{
    struct featherattrib_t attribparms;

    attribparms.parmvalue = parmvalue;

    attribparms.options = options;

    attribparms.inrange.x = inrange1;
    attribparms.inrange.y = inrange2;
    attribparms.outrange.x = outrange1;
    attribparms.outrange.y = outrange2;

    attribparms.rampsize = rampsize;
    attribparms.ramp = ramp;

    attribparms.length = 0;
    attribparms.tuplesize = 0;
    attribparms.array = 0;
    attribparms.attribclass = -1;

    attribparms.tex = 0;

    return attribparms;
};

void
set_featherattrib_array(
    struct featherattrib_t *attribparms,
    int length,
    int tuplesize,
    global float * restrict array,
    int attribclass)
{
    attribparms->length = length;
    attribparms->tuplesize = tuplesize;
    attribparms->array = array;
    attribparms->attribclass = attribclass;
};

void
set_featherattrib_tex(
    struct featherattrib_t *attribparms,
    int stride_x,
    int stride_y,
    int stride_z,
    int stride_offset,
    int res_x,
    int res_y,
    int res_z,
    global float * restrict tex)
{
    attribparms->tex_stride.x = stride_x;
    attribparms->tex_stride.y = stride_y;
    attribparms->tex_stride.z = stride_z;
    attribparms->tex_stride_offset = stride_offset;
    attribparms->tex_res.x = res_x;
    attribparms->tex_res.y = res_y;
    attribparms->tex_res.z = res_z;
    attribparms->tex = tex;
};

float
eval_featherattrib(
    struct featherattrib_t *attribparms, int primindex, int ptindex, float2 uv, int tupleindex)
{
    float overval = 1.0;
    int overridden = 0;

    if (attribparms->tex > 0)
    {
        uv.x *= (attribparms->tex_res.x-1) + 0.5;
        uv.y *= (attribparms->tex_res.y-1) + 0.5;

        float value = bilinear_interp_vol(
            uv,
            attribparms->tex,
            attribparms->tex_stride_offset,
            attribparms->tex_stride.x, attribparms->tex_stride.y,
            attribparms->tex_res.x, attribparms->tex_res.y);

        overval = value;
        overridden = 1;
    }
    else if (attribparms->length > 0)
    {
        int index;
        if (attribparms->attribclass == FEATHER_ATTRIBCLASS_PRIMITIVE)
            index = primindex;
        else
            index = ptindex;

        int array_index = attribparms->tuplesize * index
            + min(tupleindex, attribparms->tuplesize-1);

        float attribval = attribparms->array[array_index];

        overval = attribval;
        overridden = 1;
    }

    if (overridden)
    {
        if (attribparms->options & 1)
        {
            float alpha = fitTo01(overval, attribparms->inrange.x, attribparms->inrange.y);
            overval = fit01(alpha, attribparms->outrange.x, attribparms->outrange.y);
        }

        return overval * attribparms->parmvalue;
    }
    else
    {
        return attribparms->parmvalue;
    }
}

float2
eval_featherattrib2(
    struct featherattrib_t *attribparms, int primindex, int ptindex, float2 uv, float u)
{
    float2 val;

    val.x = eval_featherattrib(attribparms, primindex, ptindex, uv, 0);

    if (attribparms->tuplesize > 1)
        val.y = eval_featherattrib(attribparms, primindex, ptindex, uv, 1);
    else
        val.y = val.x;

    if (attribparms->options & 2 && attribparms->rampsize > 0)
    {
        int index = (int)(u * ((float)attribparms->rampsize-0.5));
        if (index >= attribparms->rampsize)
            index = attribparms->rampsize - 1;

        val *= attribparms->ramp[index];
    }

    return val;
}

#endif
