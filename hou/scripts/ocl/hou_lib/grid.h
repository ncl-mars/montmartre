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
 * NAME:    grid.h (CE Library, OpenCL)
 *
 * COMMENTS:
 *    Simple grid data structure to bin points
 */

#ifndef POINTGRID_OPENCL_H
#define POINTGRID_OPENCL_H

#define USE_LONG
// for exint.
#include <typedefines.h>

#define SYS_EXINT_MAX 9223372036854775807

typedef struct
{
    int3                 res;
    fpreal3              bbmin;
    float                voxsize;
} grid_t;

// Returns -1 if the provided point and resolution
// would exceed the exint linear idx.
static exint
gridToLinearIdx(int3 p, int3 res)
{
    exint maxres = SYS_EXINT_MAX;
    maxres /= res.x;
    if (maxres < (res.y * res.z))
        return -1;
    maxres /= res.y;
    if (maxres < res.z)
        return -1;

    return ((exint)p.z * res.y + (exint)p.y) * res.x + (exint)p.x;
}

static int3
linearIdxToGrid(exint idx, int3 res)
{
    int x, y, z;
    x = idx % res.x;
    idx -= x;
    idx /= res.x;
    y = idx % res.y;
    idx -= y;
    idx /= res.y;
    z = idx;
    return (int3)(x, y, z);
}

static int3
toGridIntSpace(grid_t *grid, fpreal3 p)
{
    p -= grid->bbmin;
    p /= grid->voxsize;
    return (int3)(floor(p.x), floor(p.y), floor(p.z));
}

static exint
gridIntToLinear(grid_t *grid, int3 vox)
{
    return gridToLinearIdx(vox, grid->res);
}

#endif