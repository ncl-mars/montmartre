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
 * NAME:    CVBD_Colliders.h ( CE Library, OpenCL)
 *
 * COMMENTS:
 */

#ifndef __VDB_COLLIDERS_H__
#define __VDB_COLLIDERS_H__

#include "matrix.h"
#include "quaternion.h"
#include "interpolate.h"
#include "CVDBUtil.h"

// collider type
#define COL_NONE -1
#define COL_STATIC 0
#define COL_TRANSFORMED 1
#define COL_DEFORMED 2

//
// srt utility functions
//

typedef struct
{
    float3 scale;
    quat rot;
    float3 trans;
} srt;

static void
srtomat3(const srt* xform, mat3 m)
{
    mat3 s;
    mat3identity(s);
    s[0].x = xform->scale.x;
    s[1].y = xform->scale.y;
    s[2].z = xform->scale.z;

    mat3 r;
    qtomat3(xform->rot, r);

    mat3mul(s, r, m);
}

static void
srtomat3inv(const srt* xform, mat3 m_inv)
{
    mat3 s_inv;
    mat3identity(s_inv);
    s_inv[0].x /= xform->scale.x;
    s_inv[1].y /= xform->scale.y;
    s_inv[2].z /= xform->scale.z;

    mat3 r_inv;
    quat q_inv = qinvert(xform->rot);
    qtomat3(q_inv, r_inv);

    mat3mul(r_inv, s_inv, m_inv);
}

static void
srtinterpolate(
        const srt* xform0,
        const srt* xform1,
        srt* xform,
        const float bias)
{
    xform->trans = mix(xform0->trans, xform1->trans, bias);
    xform->rot = slerp(xform0->rot, xform1->rot, bias);
    xform->scale = mix(xform0->scale, xform1->scale, bias);
}

static void
srtload(int idx,
        global float* restrict trans,
        global float* restrict rot,
        global float* restrict scale,
        srt* xform)
{
    xform->trans = vload3(idx, trans);
    xform->rot = vload4(idx, rot);
    xform->scale = vload3(idx, scale);
}

typedef struct
{
    int type;
    float friction;
    float sticky;
    bool kill;
    float frame_dt;
    float bias0;
    float bias1;
    srt xform0;
    srt xform1;
    global void* restrict sdf0;
    global void* restrict vel0;
    global void* restrict sdf1;
    global void* restrict vel1;
    cnanovdb_readaccessor sdf0_acc;
    cnanovdb_readaccessor vel0_acc;
    cnanovdb_readaccessor sdf1_acc;
    cnanovdb_readaccessor vel1_acc;
} colliderdata;


static void
initColliderData(
        colliderdata *col_grids, 
        global void* restrict sdf0,
        int type,
        float friction,
        float sticky,
        bool kill)
{
    col_grids->sdf0 = sdf0;
    cnanovdb_getreadaccessor(col_grids->sdf0, &col_grids->sdf0_acc);
    col_grids->type = type;
    col_grids->friction = friction;
    col_grids->sticky = sticky;
    col_grids->kill = kill;
}

// Functions to find the interpolated SDF distance.  If
// that distance is <= rad, c_vel and c_grad will be set
// to the interpolated collision velocity and gradient.
static float
findSdfCollisionStatic(
        const float3 g_wpos,
        const float rad,
        const float offset,
        float3 *c_vel,
        float3 *c_grad,
        const float dt,
        colliderdata* col_grids)
{
    global cnanovdb_griddata* col_sdf0_griddata
            = (global cnanovdb_griddata*)col_grids->sdf0;

    // sample sdf
    float c_sdf = cnanovdb_sampleF(
            col_sdf0_griddata, &col_grids->sdf0_acc, g_wpos) - offset;

    if (c_sdf <= rad)
    {
        // sample gradient
        if (c_grad)
            *c_grad = cnanovdb_gradient(
                    col_sdf0_griddata, &col_grids->sdf0_acc, g_wpos);

        // set collider vel
        if (c_vel)
            *c_vel = (float3)(0.0f);
    }
    return c_sdf;
}

static void
setColliderTransformed(
        colliderdata *col_grids,
        float time, float timeinc,
        float frame_start, float frame_end,
        global float *trans0,
        global float *rot0,
        global float *scale0,
        global float *trans1,
        global float *rot1,
        global float *scale1,
        int collider_idx)
{
    // compute bias for collider interpolation
    col_grids->bias0 = fitTo01(
            time - timeinc, frame_start, frame_end);
    // compute bias for collider interpolation (g_wpos_next)
    col_grids->bias1 = fitTo01(
            time, frame_start, frame_end);
    // load collider srt transforms
    srtload(collider_idx, trans0, rot0, scale0, &col_grids->xform0);
    srtload(collider_idx, trans1, rot1, scale1, &col_grids->xform1);
}

static float
findSdfCollisionTransformed(
        const float3 g_wpos,
        const float rad,
        const float offset,
        float3 *c_vel,
        float3 *c_grad,
        const float dt,
        colliderdata* col_grids)
{
    // interpolate translate, rotation, and scale for current timestep
    srt xform;
    srtinterpolate(
            &col_grids->xform0, &col_grids->xform1, &xform, col_grids->bias0);

    // build current transform (scale*rot) and inverse transform
    // (rot^(-1)*scale^(-1)) from interpolated values
    mat3 map, map_inv;
    srtomat3(&xform, map);
    srtomat3inv(&xform, map_inv);

    // update nanovdb map with interpolated transforms
    global cnanovdb_griddata* col_sdf0_griddata
            = (global cnanovdb_griddata*)col_grids->sdf0;
    cnanovdb_setMap(col_sdf0_griddata, map, map_inv, xform.trans);

    // sample sdf
    float c_sdf = cnanovdb_sampleF(
            col_sdf0_griddata, &col_grids->sdf0_acc, g_wpos) - offset;

    if (c_sdf <= rad)
    {
        // sample gradient
        if (c_grad)
            *c_grad = cnanovdb_gradient(
                    col_sdf0_griddata, &col_grids->sdf0_acc, g_wpos);
        if (c_vel)
        {
            // interpolate translate, rotation, and scale for next timestep
            srt xform_next;
            srtinterpolate(
                    &col_grids->xform0, &col_grids->xform1, &xform_next,
                    col_grids->bias1);

            // build next transform (scale*rot) from interpolated values
            mat3 map_next;
            srtomat3(&xform_next, map_next);

            // compute next world pos
            float3 g_wpos_next = g_wpos;
            g_wpos_next -= xform.trans;
            g_wpos_next = mat3vecmul(map_inv, g_wpos_next);
            g_wpos_next = mat3vecmul(map_next, g_wpos_next);
            g_wpos_next += xform_next.trans;

            // compute collider vel
            *c_vel = (g_wpos_next - g_wpos) / dt;
        }
    }
    return c_sdf;
}

static void
setColliderDeformed(colliderdata *col_grids,
                     float time, float timeinc,
                     float frame_start, float frame_end,
                     global void* restrict vel0,
                     global void* restrict sdf1,
                     global void* restrict vel1)
{
    // compute bias for collider interpolation
    col_grids->bias0 = fitTo01(
            time - timeinc, frame_start, frame_end);
    col_grids->frame_dt = frame_end - frame_start;
    col_grids->vel0 = vel0;
    col_grids->sdf1 = sdf1;
    col_grids->vel1 = vel1;
    cnanovdb_getreadaccessor(col_grids->vel0, &col_grids->vel0_acc);
    cnanovdb_getreadaccessor(col_grids->sdf1, &col_grids->sdf1_acc);
    cnanovdb_getreadaccessor(col_grids->vel1, &col_grids->vel1_acc);
}

static float
findSdfCollisionDeformed(
        const float3 g_wpos,
        const float rad,
        const float offset,
        float3 *c_vel_out,
        float3 *c_grad,
        const float dt,
        colliderdata* col_grids)
{
    global cnanovdb_griddata* col_sdf0_griddata
            = (global cnanovdb_griddata*)col_grids->sdf0;
    global cnanovdb_griddata* col_vel0_griddata
            = (global cnanovdb_griddata*)col_grids->vel0;
    global cnanovdb_griddata* col_sdf1_griddata
            = (global cnanovdb_griddata*)col_grids->sdf1;
    global cnanovdb_griddata* col_vel1_griddata
            = (global cnanovdb_griddata*)col_grids->vel1;

    // sample collider vel
    float3 c_vel0 = cnanovdb_sampleF3(
            col_vel0_griddata, &col_grids->vel0_acc, g_wpos);
    float3 c_vel1 = cnanovdb_sampleF3(
            col_vel1_griddata, &col_grids->vel1_acc, g_wpos);
    float3 c_vel = mix(c_vel0, c_vel1, col_grids->bias0);

    // compute sample pos
    float3 g_wpos0 = g_wpos - col_grids->bias0 * col_grids->frame_dt * c_vel;
    float3 g_wpos1 = g_wpos
                     + (1.0f - col_grids->bias0) * col_grids->frame_dt * c_vel;

    // sample sdf
    float c_sdf0 = cnanovdb_sampleF(
            col_sdf0_griddata, &col_grids->sdf0_acc, g_wpos0) - offset;
    float c_sdf1 = cnanovdb_sampleF(
            col_sdf1_griddata, &col_grids->sdf1_acc, g_wpos1) - offset;

    float c_sdf = mix(c_sdf0, c_sdf1, col_grids->bias0);

    if (c_sdf <= rad)
    {
        // sample gradient
        if (c_grad)
        {
            float3 c_grad0 = cnanovdb_gradient(
                    col_sdf0_griddata, &col_grids->sdf0_acc, g_wpos0);
            float3 c_grad1 = cnanovdb_gradient(
                    col_sdf1_griddata, &col_grids->sdf1_acc, g_wpos1);
            *c_grad = mix(c_grad0, c_grad1, col_grids->bias0);
        }
        if (c_vel_out)
            *c_vel_out = c_vel;
    }
    return c_sdf;
}

static float
findSdfCollision(
        const float3 g_wpos,
        const float rad,
        float3 *c_vel,
        float3 *c_grad,
        const float dt,
        colliderdata* col_grids)
{
    if (col_grids->type == COL_STATIC)
        return findSdfCollisionStatic(g_wpos, rad, 0, c_vel, c_grad, dt, col_grids);
    if (col_grids->type == COL_TRANSFORMED)
        return findSdfCollisionTransformed(g_wpos, rad, 0, c_vel, c_grad, dt, col_grids);
    if (col_grids->type == COL_DEFORMED)
        return findSdfCollisionDeformed(g_wpos, rad, 0, c_vel, c_grad, dt, col_grids);
    return 1e37;  // no hit
}           

#endif
