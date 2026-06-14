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
 * NAME:    quaternion.h ( CE Library, OpenCL)
 *
 * COMMENTS:
 */

#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#include "typedefines.h"
#include "matrix.h"

#define FLOAT_MIN 1.17549e-38f

#define PRINTQUAT(v)                                                           \
    printf("%s:\n", #v);                                                       \
    printf("%g %g %g %g\n", v.x, v.y, v.z, v.w)

// These functions are used by $SHS/ocl/compute/vector.cl.
static void
quatMultiply(fpreal4 *dst,
	     const fpreal4 *a,
	     const fpreal4 *b)
{
    fpreal3 a_pt = (*a).xyz;
    fpreal3 b_pt = (*b).xyz;
    dst->w = (*a).w * (*b).w - dot(a_pt, b_pt);
    dst->xyz = (*a).w * b_pt + (*b).w * a_pt + cross(a_pt, b_pt);
}

static void
quatRotate(fpreal3 *dst,
	   const fpreal4 *quat,
	   const fpreal3 *pt)
{
    fpreal4 tmp;
    fpreal4 pt_quat = (fpreal4)(pt->x, pt->y, pt->z, (fpreal)0);
    quatMultiply(&tmp, quat, &pt_quat);

    fpreal4 mult_tmp = (fpreal4)(-quat->xyz, quat->w);

    fpreal4 ret;
    quatMultiply(&ret, &tmp, &mult_tmp);
    *dst = ret.xyz;
}

// These functions are used by $SHS/ocl/sim/pbd_constraints.cl.
typedef fpreal4 quat;

// Return the conjugate of the input quaternion.
static quat
qconjugate(quat q)
{
    return (quat)(-q.xyz, q.w);
}

// Return the inverse of the input quaternion.
static quat
qinvert(quat q)
{
    float n = dot(q, q);
    if (n > FLOAT_MIN)
    {
        q = qconjugate(q);
        q /= n;
    }
    return q;
}

// Multiply the two input quaternions and return the result.
static quat
qmultiply(quat q0, quat q1)
{
    fpreal3   v1 = q0.xyz;
    fpreal3   v2 = q1.xyz;
    fpreal    s1 = q0.w, s2 = q1.w;

    fpreal w = s1 * s2 - dot(v1, v2);
    fpreal3 v3 = s1 * v2 + s2 * v1 + cross(v1, v2);
    return (quat)(v3, w);
}

// Rotate the input vector by the rotation stored in quaternion q.
static fpreal3
qrotate(quat q, fpreal3 v)
{
    return qmultiply(qmultiply(q, (quat)(v, 0)), qconjugate(q)).xyz;
}

// Rotate the input vector by the inverse of the rotation stored
// in quaternion q.
static fpreal3
qrotateinv(quat q, fpreal3 v)
{
    return qmultiply(qmultiply(qconjugate(q), (quat)(v, 0)), q).xyz;
}

// Return either q0 + q1 or q0 - q1, depending on which is closer
// in 4d space.
static quat
qcloser(quat q0, quat q1)
{
    quat q0plus = q0 + q1;
    q0 -= q1;
    return (dot(q0, q0) > dot(q0plus, q0plus) ? q0plus : q0);
}

// Return the 3x3 rotation matrix corresponding to the quaterion.
// This rotation matrix matches the qconvert VEX function.
static void
qtomat3(quat q, mat3 r)
{
    const fpreal tx  = 2.0f * q.x;
    const fpreal ty  = 2.0f * q.y;
    const fpreal tz  = 2.0f * q.z;
    const fpreal twx = tx * q.w;
    const fpreal twy = ty * q.w;
    const fpreal twz = tz * q.w;
    const fpreal txx = tx * q.x;
    const fpreal txy = ty * q.x;
    const fpreal txz = tz * q.x;
    const fpreal tyy = ty * q.y;
    const fpreal tyz = tz * q.y;
    const fpreal tzz = tz * q.z;

    r[0] = (fpreal3)(1.0f - (tyy + tzz), txy + twz, txz - twy);
    r[1] = (fpreal3)(txy - twz, 1.0f - (txx + tzz), tyz + twx);
    r[2] = (fpreal3)(txz + twy, tyz - twx, 1.0f - (txx + tyy));
}

// Return the tranpose of the 3x3 rotation matrix corresponding to the quaterion.
static void
qtomat3T(quat q, mat3 r)
{
    const fpreal tx  = 2.0f * q.x;
    const fpreal ty  = 2.0f * q.y;
    const fpreal tz  = 2.0f * q.z;
    const fpreal twx = tx * q.w;
    const fpreal twy = ty * q.w;
    const fpreal twz = tz * q.w;
    const fpreal txx = tx * q.x;
    const fpreal txy = ty * q.x;
    const fpreal txz = tz * q.x;
    const fpreal tyy = ty * q.y;
    const fpreal tyz = tz * q.y;
    const fpreal tzz = tz * q.z;

    r[0] = (fpreal3)(1.0f - (tyy + tzz), txy - twz, txz + twy);
    r[1] = (fpreal3)(txy + twz, 1.0f - (txx + tzz), tyz - twx);
    r[2] = (fpreal3)(txz - twy, tyz + twx, 1.0f - (txx + tyy));
}

// Return the quaterion representing the specified axis/angle rotation.
static quat
qfromaxisangle(fpreal a, fpreal3 axis)
{
    const fpreal ha = 0.5f * a;
    return (quat)(sin(ha) * axis, cos(ha));
}

// Extract the rotation from the provided matrix using the algorithm from
// "A robust method to extract the rotational part of deformations."
// The input q is hopefully the rotation from a previous timestep or constraint
// solve for faster convergence.
static quat
extractRotation(const mat3 A, quat q, int maxiter)
{
    mat3 Acol, Rcol;
    transpose3(A, Acol);
    for(int i = 0; i < maxiter; i++)
    {
        qtomat3(q, Rcol);
        fpreal3 omega = cross(Rcol[0], Acol[0]) + cross(Rcol[1], Acol[1]) + cross(Rcol[2], Acol[2]);
        omega /= fabs(dot(Rcol[0], Acol[0]) + dot(Rcol[1], Acol[1]) + dot(Rcol[2], Acol[2])) + 1e-5f;
        fpreal w = length(omega);
        if (w < 1e-5f)
            break;
        q = qmultiply(qfromaxisangle(w, omega / w), q);
        q = normalize(q);
    }
    return q;
}

// UT_Quaternion::interpolate
static quat slerp(const quat from, const quat to, fpreal t)
{
    fpreal               omega, cosom, sinom, sclp, sclq;
    quat                 result;
    quat                 adjustedtarget = to;
    fpreal               u = 1.0 - t;

    // This ensures we interpolate along the shortest possible arc.
    cosom = dot(from, adjustedtarget);
    if (cosom < 0.0)
    {
        adjustedtarget = -adjustedtarget;
        cosom = -cosom;
    }

    // UT_ASSERT_P(cosom >= 0.0);

    if( cosom > -1.0 )
    {
        if( cosom < 1.0 - 1e-5 )
        {
            // normal case
            omega = acos(cosom);
            sinom = sin(omega);
            sclp = sin( u*omega ) / sinom;
            sclq = sin( t*omega ) / sinom;
        }
        else
        {
            // case where quats are too close together.
            sclp=u;
            sclq=t;
        }

        result.x = sclp*from.x + sclq*adjustedtarget.x;
        result.y = sclp*from.y + sclq*adjustedtarget.y;
        result.z = sclp*from.z + sclq*adjustedtarget.z;
        result.w = sclp*from.w + sclq*adjustedtarget.w;
    }
    else
    {
        // case where quats are almost completely opposite.
        result.x = -from.y;
        result.y = +from.x;
        result.z = -from.w;
        result.w = +from.z;

        sclp = sin( 0.5*u*M_PI );
        sclq = sin( 0.5*t*M_PI );

        result.x = sclp*from.x + sclq*result.x;
        result.y = sclp*from.y + sclq*result.y;
        result.z = sclp*from.z + sclq*result.z;
    }

    return result;
}

quat
evaluateCubicHalfCoeffsQ(float2 coeffs, int2 indices, global quat *values)
{
    if (indices.x >= 0 && indices.y >= 0)
    {
        return slerp(
            values[indices.x],
            values[indices.y],
            (coeffs.y)/(coeffs.x+coeffs.y));
    }
    else if (indices.x >= 0)
    {
        return values[indices.x];
    }
    else if (indices.y >= 0)
    {
        return values[indices.y];
    }
    else
    {
        quat res = {0,0,0,0};
        return res;
    }
}

quat evaluateCubicCoeffsQ(float4 coeffs, int4 indices, global quat *values)
{
    quat half1, half2;
    bool has_half1, has_half2;

    half1 = evaluateCubicHalfCoeffsQ(coeffs.lo, indices.lo, values);
    half2 = evaluateCubicHalfCoeffsQ(coeffs.hi, indices.hi, values);

    float2 half_coeffs = 0;

    if (indices.x >= 0)
        half_coeffs.x += coeffs.x;
    if (indices.y >= 0)
        half_coeffs.x += coeffs.y;

    if (indices.z >= 0)
        half_coeffs.y += coeffs.z;
    if (indices.w >= 0)
        half_coeffs.y += coeffs.w;

    float t = half_coeffs.y / (half_coeffs.x + half_coeffs.y);

    return slerp(half1, half2, t);
}

#ifndef NO_DOUBLE_SUPPORT
// Double versions of subset of the above functions
// (mainly for shapematching ATM).

typedef double4 quatd;

static quatd
qconjugated(quatd q)
{
    return (quatd)(-q.xyz, q.w);
}

static quatd
qmultiplyd(quatd q0, quatd q1)
{
    double3   v1 = q0.xyz;
    double3   v2 = q1.xyz;
    double    s1 = q0.w, s2 = q1.w;

    double w = s1 * s2 - dot(v1, v2);
    double3 v3 = s1 * v2 + s2 * v1 + cross(v1, v2);
    return (quatd)(v3, w);
}

static double3
qrotated(quatd q, double3 v)
{
    return qmultiplyd(qmultiplyd(q, (quatd)(v, 0)), qconjugated(q)).xyz;
}

static double3
qrotateinvd(quatd q, double3 v)
{
    return qmultiplyd(qmultiplyd(qconjugated(q), (quatd)(v, 0)), q).xyz;
}

static quatd
qfromaxisangled(double a, double3 axis)
{
    const double ha = 0.5 * a;
    return (quatd)(sin(ha) * axis, cos(ha));
}

static void
qtomat3d(quatd q, mat3d r)
{
    const double tx  = 2.0f * q.x;
    const double ty  = 2.0f * q.y;
    const double tz  = 2.0f * q.z;
    const double twx = tx * q.w;
    const double twy = ty * q.w;
    const double twz = tz * q.w;
    const double txx = tx * q.x;
    const double txy = ty * q.x;
    const double txz = tz * q.x;
    const double tyy = ty * q.y;
    const double tyz = tz * q.y;
    const double tzz = tz * q.z;

    r[0] = (double3)(1.0f - (tyy + tzz), txy + twz, txz - twy);
    r[1] = (double3)(txy - twz, 1.0f - (txx + tzz), tyz + twx);
    r[2] = (double3)(txz + twy, tyz - twx, 1.0f - (txx + tyy));
}

static quatd
extractRotationd(const mat3d A, quatd q, int maxiter)
{
    mat3d Acol, Rcol;
    transpose3d(A, Acol);
    for(int i = 0; i < maxiter; i++)
    {
        qtomat3d(q, Rcol);
        double3 omega = cross(Rcol[0], Acol[0]) + cross(Rcol[1], Acol[1]) + cross(Rcol[2], Acol[2]);
        omega /= fabs(dot(Rcol[0], Acol[0]) + dot(Rcol[1], Acol[1]) + dot(Rcol[2], Acol[2])) + 1e-10f;
        double w = length(omega);
        if (w < 1e-10f)
            break;
        q = qmultiplyd(qfromaxisangled(w, omega / w), q);
        q = normalize(q);
    }
    return q;
}
#endif

#endif
