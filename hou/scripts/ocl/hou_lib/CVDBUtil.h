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
 * NAME:    CVBUtil.h ( CE Library, OpenCL)
 *
 * COMMENTS: Utility functions for working with VBD's in OpenCL.
 *
 */


#ifndef __CVDBUTILS_H__
#define __CVDBUTILS_H__


#include "CVDBDefines.h"

// This is because in an import path people must explicitly
// remember to import the defines...
#ifndef __CVDBDefines_H__
#error "CVDBDefines.h must be #imported"
#endif

#include "CNanoVDB.h"
#include "CSampleFromVoxels.h"
#include "matrix.h"

#define CAST_VEC3(POS) \
    ((cnanovdb_Vec3F *) &(POS))

static float3
cnanovdb_Vec3F_to_float3(cnanovdb_Vec3F vec3)
{
    return (float3)(vec3.mVec[0], vec3.mVec[1], vec3.mVec[2]);
}

static int3
cnanovdb_coord_to_int3(cnanovdb_coord coord)
{
    return (int3)(coord.mVec[0], coord.mVec[1], coord.mVec[2]);
}

static cnanovdb_Vec3F
cnanovdb_float3_to_Vec3F(float3 vec3)
{
    cnanovdb_Vec3F result = { .mVec = { vec3.x, vec3.y, vec3.z } };
    return result;
}

static float3
cnanovdb_worldToIndex(const global cnanovdb_griddata *RESTRICT grid, float3 p)
{
    cnanovdb_griddata_worldToIndex(CAST_VEC3(p), grid, CAST_VEC3(p));
    return p;
}

static float3
cnanovdb_indexToWorld(const global cnanovdb_griddata *RESTRICT grid, float3 p)
{
    cnanovdb_griddata_indexToWorld(CAST_VEC3(p), grid, CAST_VEC3(p));
    return p;
}

static float3
cnanovdb_worldToIndexDir(const global cnanovdb_griddata *RESTRICT grid, float3 p)
{
    cnanovdb_griddata_worldToIndexDir(CAST_VEC3(p), grid, CAST_VEC3(p));
    return p;
}

static float3
cnanovdb_indexToWorldDir(const global cnanovdb_griddata *RESTRICT grid, float3 p)
{
    cnanovdb_griddata_indexToWorldDir(CAST_VEC3(p), grid, CAST_VEC3(p));
    return p;
}

static float3
cnanovdb_applyIJT(const global cnanovdb_griddata *RESTRICT grid, float3 p)
{
    cnanovdb_griddata_applyIJT(CAST_VEC3(p), grid, CAST_VEC3(p));
    return p;
}

static void
cnanovdb_getreadaccessor(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc)
{
    global const cnanovdb_rootdataF *root = cnanovdb_treedata_rootF(cnanovdb_griddata_tree(grid));
    cnanovdb_readaccessor_init(acc, root);
}

static float
cnanovdb_sampleF(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    cnanovdb_griddata_worldToIndex(CAST_VEC3(p), grid, CAST_VEC3(p));
    return cnanovdb_sampleF_trilinear(acc, CAST_VEC3(p));
}

static float
cnanovdb_activeSampleF(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc, float3 p, bool *RESTRICT isactive)
{
    cnanovdb_griddata_worldToIndex(CAST_VEC3(p), grid, CAST_VEC3(p));
    cnanovdb_coord repcoord;
    cnanovdb_coord_round(&repcoord, CAST_VEC3(p));
    if (cnanovdb_readaccessor_isActiveF(acc, &repcoord))
    {
        *isactive = true;
        return cnanovdb_sampleF_trilinear(acc, CAST_VEC3(p));
    }
    *isactive = false;
    return 0;
}

static float
cnanovdb_sampleF_stencil(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_stencil1F *RESTRICT stencil, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    cnanovdb_griddata_worldToIndex(CAST_VEC3(p), grid, CAST_VEC3(p));
    return cnanovdb_sampleF_trilinear_stencil(stencil, acc, CAST_VEC3(p));
}

static float
cnanovdb_sampleFidx(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    return cnanovdb_sampleF_trilinear(acc, CAST_VEC3(p));
}

static float
cnanovdb_sampleFidx_stencil(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_stencil1F *RESTRICT stencil, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    return cnanovdb_sampleF_trilinear_stencil(stencil, acc, CAST_VEC3(p));
}

static float3
cnanovdb_sampleF3(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    cnanovdb_griddata_worldToIndex(CAST_VEC3(p), grid, CAST_VEC3(p));
    cnanovdb_Vec3F result = cnanovdb_sampleF3_trilinear(acc, CAST_VEC3(p));
    return (float3)(result.mVec[0], result.mVec[1], result.mVec[2]);
}

static float3
cnanovdb_activeSampleF3(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc, float3 p, bool *RESTRICT isactive)
{
    cnanovdb_griddata_worldToIndex(CAST_VEC3(p), grid, CAST_VEC3(p));
    cnanovdb_coord repcoord;
    cnanovdb_coord_round(&repcoord, CAST_VEC3(p));
    if (cnanovdb_readaccessor_isActiveF3(acc, &repcoord))
    {
        *isactive = true;
        cnanovdb_Vec3F result = cnanovdb_sampleF3_trilinear(acc, CAST_VEC3(p));
        return (float3)(result.mVec[0], result.mVec[1], result.mVec[2]);
    }
    *isactive = false;
    return (float3)(0.0f, 0.0f, 0.0f);
}

static float3
cnanovdb_sampleF3_stencil(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_stencil1F3 *RESTRICT stencil, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    cnanovdb_griddata_worldToIndex(CAST_VEC3(p), grid, CAST_VEC3(p));
    cnanovdb_Vec3F result = cnanovdb_sampleF3_trilinear_stencil(stencil, acc, CAST_VEC3(p));
    return (float3)(result.mVec[0], result.mVec[1], result.mVec[2]);
}

static float3
cnanovdb_sampleF3idx(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    cnanovdb_Vec3F result = cnanovdb_sampleF3_trilinear(acc, CAST_VEC3(p));
    return (float3)(result.mVec[0], result.mVec[1], result.mVec[2]);
}

static float3
cnanovdb_sampleF3idx_stencil(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_stencil1F3 *RESTRICT stencil, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    cnanovdb_Vec3F result = cnanovdb_sampleF3_trilinear_stencil(stencil, acc, CAST_VEC3(p));
    return (float3)(result.mVec[0], result.mVec[1], result.mVec[2]);
}

static float3
cnanovdb_gradient(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    float3 grad;

    cnanovdb_griddata_worldToIndex(CAST_VEC3(p), grid, CAST_VEC3(p));
    cnanovdb_sampleF_gradient(CAST_VEC3(grad), acc, CAST_VEC3(p));
    cnanovdb_griddata_applyIJT(CAST_VEC3(grad), grid, CAST_VEC3(grad));
    return grad;
}

static float3
cnanovdb_gradient0(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    float3 grad;

    cnanovdb_griddata_worldToIndex(CAST_VEC3(p), grid, CAST_VEC3(p));
    cnanovdb_sampleF_gradient0(CAST_VEC3(grad), acc, CAST_VEC3(p));
    cnanovdb_griddata_applyIJT(CAST_VEC3(grad), grid, CAST_VEC3(grad));
    return grad;
}

static float3
cnanovdb_gradient0_stencil(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_stencil1F *RESTRICT stencil, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    float3 grad;

    cnanovdb_griddata_worldToIndex(CAST_VEC3(p), grid, CAST_VEC3(p));
    cnanovdb_sampleF_gradient0_stencil(CAST_VEC3(grad), stencil, acc, CAST_VEC3(p));
    cnanovdb_griddata_applyIJT(CAST_VEC3(grad), grid, CAST_VEC3(grad));
    return grad;
}

static float3
cnanovdb_gradientidx(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    float3 grad;

    cnanovdb_sampleF_gradient(CAST_VEC3(grad), acc, CAST_VEC3(p));
    return grad;
}

static float3
cnanovdb_gradientidx0(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    float3 grad;

    cnanovdb_sampleF_gradient0(CAST_VEC3(grad), acc, CAST_VEC3(p));
    return grad;
}

static float3
cnanovdb_gradientidx0_stencil(const global cnanovdb_griddata *RESTRICT grid, cnanovdb_stencil1F *RESTRICT stencil, cnanovdb_readaccessor *RESTRICT acc, float3 p)
{
    float3 grad;

    cnanovdb_sampleF_gradient0_stencil(CAST_VEC3(grad), stencil, acc, CAST_VEC3(p));
    return grad;
}

static void
cnanovdb_setMap(
        global cnanovdb_griddata* griddata,
        const mat3 map,
        const mat3 mapinv,
        const float3 trans)
{
    griddata->mMap.mVecF[0] = trans.x;
    griddata->mMap.mVecF[1] = trans.y;
    griddata->mMap.mVecF[2] = trans.z;
    for (int i = 0; i < 3; ++i)
    {
        // We transpose the matrix here to match the nanovdb implementation
        // take a look at the cnanovdb_map_apply() in CNanoVDB.h. The
        // multiplication is performed from the right like it is the convention
        // for column-major matrices. Also take a look at Map::set() in
        // NanoVDB.h which explicitly transpose the matrix when setting the map.
        griddata->mMap.mMatF[i + 0] = map[i].x;
        griddata->mMap.mMatF[i + 3] = map[i].y;
        griddata->mMap.mMatF[i + 6] = map[i].z;
        griddata->mMap.mInvMatF[i + 0] = mapinv[i].x;
        griddata->mMap.mInvMatF[i + 3] = mapinv[i].y;
        griddata->mMap.mInvMatF[i + 6] = mapinv[i].z;
    }
}

#endif
