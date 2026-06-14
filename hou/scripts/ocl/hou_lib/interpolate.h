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
 * NAME:    interpolate.h ( CE Library, OpenCL)
 *
 * COMMENTS:
 */

#ifndef __INTERPOLATE_H__
#define __INTERPOLATE_H__

// #pragma OPENCL EXTENSION cl_amd_printf : enable

/*#ifdef cl_amd_printf
#define CHECKNAN(f) if (isnan(f)) { printf("%s is NAN, x = %g, y = %g, z = %g\n",#f, x, y, z);}
#define CHECKNAN2D(f) if (isnan(f)) { printf("%s is NAN, x = %g, y = %g\n",#f, x, y);}
#else*/
#define CHECKNAN(f)
#define CHECKNAN2D(f)
// #endif

#define CLAMPOFFSET 1e-4f

// While mix is limited to 0..1, it is tempting to just use a clamp
// on t; but you can get cancellation at t == 1 that stops the result
// from being b.   Selecting on t == 0 will also stop any NANs from
// leaking across, which matches the theory that a mix with 0 should
// able to be short-circuited with an if (t == 0) a, so the fact the
// second component is nan should not affect the result in this case.
#define _SAFE_MIX(a, b, t) \
    select( select( mix(a, b, t), a, t <= 0 ), b, t >= 1 )

static float safe_mix1(float a, float b, float t) { return _SAFE_MIX(a, b, t); }
static float2 safe_mix2(float2 a, float2 b, float2 t) { return _SAFE_MIX(a, b, t); }
static float3 safe_mix3(float3 a, float3 b, float3 t) { return _SAFE_MIX(a, b, t); }
static float4 safe_mix4(float4 a, float4 b, float4 t) { return _SAFE_MIX(a, b, t); }

#undef _SAFE_MIX

static float
lerp(float v1, float v2, float t)
{
    return v1 + (v2 - v1)*t;
}

static float
fit01(float t, float v1, float v2)
{
    return mix(v1, v2, clamp(t, 0.0f, 1.0f));
}

static float
fitTo01(float val, float omin, float omax)
{
    float d = omax - omin;
    if (fabs(d) < 1e-8f)
        return 0.5f;
    if (omin < omax)
    {
        if (val < omin) return 0;
        if (val > omax) return 1;
    }
    else
    {
        if (val < omax) return 1;
        if (val > omin) return 0;
    }
    return (val - omin) / d;
}

// in is an array size long of values to be interpolated within the
// [0, 1] interval.
// pos==0 => in[0]
// pos==1 => in[size-1]
static float
lerpConstant( constant float * in, int size, float pos )
{
    int m = size - 1;
    float flr;
    float t = fract(clamp(pos, 0.0f, 1.0f) * m, &flr);
    int flooridx = convert_int(flr);
    int ceilidx = min(flooridx+1, m);
    return mix(in[flooridx], in[ceilidx], t);
}

static float3
lerpConstant3( constant float * in, int size, float pos )
{
    int m = size - 1;
    float flr;
    float t = fract(clamp(pos, 0.0f, 1.0f) * m, &flr);
    int flooridx = convert_int(flr);
    int ceilidx = min(flooridx+1, m);
    float3 v1 = vload3(flooridx, in);
    float3 v2 = vload3(ceilidx, in);
    return mix(v1, v2, t);
}

static float
centerFromFace(__global const float *a, size_t idx, uint axisstride)
{
    return 0.5f * (a[idx] + a[idx + axisstride]);
}

static float
faceFromCenter(__global const float *a, size_t idx, uint axisstride)
{
    return 0.5f * (a[idx - axisstride] + a[idx]);
}

static float
cornerFromCenter(__global const float *a, size_t idx,
                 uint ystride, uint zstride)
{
    return 0.125f * (a[idx]
                   + a[idx - 1]
                   + a[idx - ystride]
                   + a[idx - zstride]
                   + a[idx - 1 - ystride]
                   + a[idx - 1 - zstride]
                   + a[idx - ystride - zstride]
                   + a[idx - 1 - ystride - zstride]);
}
static float
cornerFromCenter2d(__global const float *a, size_t idx,
                   uint xstride, uint ystride)
{
    return 0.25f * (a[idx] + a[idx - xstride] + a[idx - ystride]
                           + a[idx - xstride - ystride]);
}

// Calc central difference derivative of cell-centered grid
// at a center cell at idx.
static float
dudxAligned(__global const float *u, const uint idx,
                  const uint xstride, const float inv2dx)
{
    return inv2dx * (u[idx + xstride] - u[idx - xstride]);
}

// Calc central difference derivative of face-sampled grid
// at a center cell at idx.  This works for all the off-axes directions
// other than the one the face-sampled grid represents.  E.g. you can
// take the dy-derivative of the x-velocity field.
static float
dudxAlignedFace(__global const float *u, const uint idx,
                      const uint ustride, const uint xstride, float inv4dx)
{
    return inv4dx * ((u[idx + xstride] + u[idx + ustride + xstride]) -
                     (u[idx - xstride] + u[idx + ustride - xstride]));
}

// Calc central difference derivative of face-sampled grid
// at a center cell at idx. This only works for the derivative along
// the axis the face-centered grid represents, e.g. you can only take
// the dx-derivative of the x-velocity field.
static float
dudxFaceAtCenter(__global const float *u, const uint idx,
                      const uint xstride, float invdx)
{
    return invdx * (u[idx + xstride] - u[idx]);
}

static float
dudxCenterAtFace(__global const float *u, const uint idx,
                      const uint xstride, float invdx)
{
    return invdx * (u[idx] - u[idx - xstride]);
}

// Calc central difference derivative of center-sampled grid at a corner cell
// at idx, by first averaging along relevant faces (invdx should incorporate
// the averaging factor). axis_stride is stride along the differentiation axis,
// and the other strides are along the remaining axes.
static float
dudxCenterAtCorner(__global const float *u, const uint idx,
                   const uint axis_stride, const uint off_stride1,
                   const uint off_stride2, float inv4dx)
{
    return inv4dx * ((u[idx] + u[idx - off_stride1]
            + u[idx - off_stride2] + u[idx - off_stride1 - off_stride2])
                 -  (u[idx - axis_stride] + u[idx - off_stride1 - axis_stride]
            + u[idx - off_stride2 - axis_stride]
            + u[idx - off_stride1 - off_stride2 - axis_stride]));
}

static float
dudxCenterAtCorner2d(__global const float *u, const uint idx,
                     const uint axis_stride, const uint off_stride,
                     float inv2dx)
{
    return inv2dx * ((u[idx] + u[idx - off_stride])
                 -  (u[idx - axis_stride] + u[idx - off_stride - axis_stride]));
}

static void
bilinear_interp(float x, float y, __global const float *p,
                        size_t idx,
                        __global float *phin,
                        __global float *minphi,
                        __global float *maxphi,
                        uint offset, uint xstride, uint ystride)

{
    // clamp to boundaries
    x = clamp(x, -1.0f, get_global_size(0) - CLAMPOFFSET);
    y = clamp(y, -1.0f, get_global_size(1) - CLAMPOFFSET);

    const int gi = (int)floor(x);
    const int gj = (int)floor(y);

    // get fractional part
    const float sx = x - gi;
    const float sy = y - gj;

    size_t srcidx = offset + gi * xstride + gj * ystride;

    const float i00 = p[srcidx];
    const float i10 = p[srcidx + xstride];
    const float i01 = p[srcidx + ystride];
    const float i11 = p[srcidx + xstride + ystride];

    CHECKNAN2D(i00)
    CHECKNAN2D(i10)
    CHECKNAN2D(i01)
    CHECKNAN2D(i11)
    const float val = (i00 * (1-sx) + i10 * (sx)) * (1-sy) +
                      (i01 * (1-sx) + i11 * (sx)) * (  sy);
    phin[idx] = val;
    if (minphi)
        minphi[idx] = fmin(fmin(fmin(i00, i01), i10), i11);
    if (maxphi)
        maxphi[idx] = fmax(fmax(fmax(i00, i01), i10), i11);
}

static float
bilinear_interp_val(float x, float y, __global const float *p,
                        uint offset, uint xstride, uint ystride,
                        uint offx,  uint offy)
{
    // clamp to boundaries
    x = clamp(x, -1.0f, get_global_size(0) - CLAMPOFFSET - offx);
    y = clamp(y, -1.0f, get_global_size(1) - CLAMPOFFSET - offy);

    const int gi = (int)floor(x);
    const int gj = (int)floor(y);

    // get fractional part
    const float sx = x - gi;
    const float sy = y - gj;

    size_t srcidx = offset + gi * xstride + gj * ystride;

    const float i00 = p[srcidx];
    const float i10 = p[srcidx + xstride];
    const float i01 = p[srcidx + ystride];
    const float i11 = p[srcidx + xstride + ystride];

    return (i00 * (1-sx) + i10 * (sx)) * (1-sy) +
           (i01 * (1-sx) + i11 * (sx)) * (  sy);
}

static void
trilinear_interp(float x, float y, float z, __global const float *p,
                    size_t idx,
                    __global float *phin,
                    __global float *minphi,
                    __global float *maxphi,
                    uint offset, uint ystride, uint zstride)
{
    x = clamp(x, -1.0f, get_global_size(0) - CLAMPOFFSET);
    y = clamp(y, -1.0f, get_global_size(1) - CLAMPOFFSET);
    z = clamp(z, -1.0f, get_global_size(2) - CLAMPOFFSET);

    const int gi = (int)floor(x);
    const int gj = (int)floor(y);
    const int gk = (int)floor(z);

    const float sx = x - gi;
    const float sy = y - gj;
    const float sz = z - gk;

    size_t srcidx = offset + gi + gj * ystride + gk * zstride;

    const float i000 = p[srcidx];
    const float i100 = p[srcidx + 1];
    const float i010 = p[srcidx + ystride];
    const float i110 = p[srcidx + 1 + ystride];
    const float i001 = p[srcidx + zstride];
    const float i101 = p[srcidx + 1 + zstride];
    const float i011 = p[srcidx + ystride + zstride];
    const float i111 = p[srcidx + 1 + ystride + zstride];

    CHECKNAN(i000)
    CHECKNAN(i100)
    CHECKNAN(i010)
    CHECKNAN(i110)
    CHECKNAN(i001)
    CHECKNAN(i101)
    CHECKNAN(i011)
    CHECKNAN(i111)
    const float val = ((i000 * (1 - sx) + i100 * (sx)) * (1 - sy) +
                      (i010 * (1 - sx) + i110 * (sx)) * (  sy)) * (1 - sz) +
                      ((i001 * (1 - sx) + i101 * (sx)) * (1 - sy) +
                      (i011 * (1 - sx) + i111 * (sx)) * (  sy)) * (  sz);

    phin[idx] = val;
    if (minphi)
        minphi[idx] = fmin(fmin(fmin(fmin(fmin(fmin(fmin(i000, i001), i010),
                                            i011), i100), i101), i110), i111);
    if (maxphi)
        maxphi[idx] = fmax(fmax(fmax(fmax(fmax(fmax(fmax(i000, i001), i010),
                                            i011), i100), i101), i110), i111);
}

static float
trilinear_interp_val(float x, float y, float z, __global const float *p,
                    uint offset, uint ystride, uint zstride,
		    uint offx,	uint offy, uint offz )
{
    x = clamp(x, -1.0f, get_global_size(0) - CLAMPOFFSET - offx);
    y = clamp(y, -1.0f, get_global_size(1) - CLAMPOFFSET - offy);
    z = clamp(z, -1.0f, get_global_size(2) - CLAMPOFFSET - offz);

    const int gi = (int)floor(x);
    const int gj = (int)floor(y);
    const int gk = (int)floor(z);

    const float sx = x - gi;
    const float sy = y - gj;
    const float sz = z - gk;

    size_t srcidx = offset + gi + gj * ystride + gk * zstride;

    const float i000 = p[srcidx];
    const float i100 = p[srcidx + 1];
    const float i010 = p[srcidx + ystride];
    const float i110 = p[srcidx + 1 + ystride];
    const float i001 = p[srcidx + zstride];
    const float i101 = p[srcidx + 1 + zstride];
    const float i011 = p[srcidx + ystride + zstride];
    const float i111 = p[srcidx + 1 + ystride + zstride];

    return  ((i000 * (1 - sx) + i100 * (sx)) * (1 - sy) +
            (i010 * (1 - sx) + i110 * (sx)) * (  sy)) * (1 - sz) +
            ((i001 * (1 - sx) + i101 * (sx)) * (1 - sy) +
            (i011 * (1 - sx) + i111 * (sx)) * (  sy)) * (  sz);
}

static float
bilinear_interp_vol(float2 pos, __global const float *p,
                    uint offset, uint xstride, uint ystride,
		    uint resx, uint resy)
{
    float x = clamp(pos.x-0.5f, (float)0, (float)(resx-1));
    float y = clamp(pos.y-0.5f, (float)0, (float)(resy-1));

    const int gi = (int)floor(x);
    const int gj = (int)floor(y);

    // In case our clamp is exactly res#-1
    const int dx = select((int)xstride, (int)0, gi == resx-1);
    const int dy = select((int)ystride, (int)0, gj == resy-1);

    const float sx = x - gi;
    const float sy = y - gj;

    size_t srcidx = offset + gi * xstride + gj * ystride;

    const float i000 = p[srcidx];
    const float i100 = p[srcidx + dx];
    const float i010 = p[srcidx + dy];
    const float i110 = p[srcidx + dx + dy];

    return mix( mix(i000, i100, sx),
                mix(i010, i110, sx), sy);
}

static float
trilinear_interp_vol(float3 pos, __global const float *p,
                    uint offset, uint xstride, uint ystride, uint zstride,
		    uint resx, uint resy, uint resz)
{
    float x = clamp(pos.x-0.5f, (float)0, (float)(resx-1));
    float y = clamp(pos.y-0.5f, (float)0, (float)(resy-1));
    float z = clamp(pos.z-0.5f, (float)0, (float)(resz-1));

    const int gi = (int)floor(x);
    const int gj = (int)floor(y);
    const int gk = (int)floor(z);

    // In case our clamp is exactly res#-1
    const int dx = select((int)xstride, (int)0, gi == resx-1);
    const int dy = select((int)ystride, (int)0, gj == resy-1);
    const int dz = select((int)zstride, (int)0, gk == resz-1);

    const float sx = x - gi;
    const float sy = y - gj;
    const float sz = z - gk;

    size_t srcidx = offset + gi * xstride + gj * ystride + gk * zstride;

    const float i000 = p[srcidx];
    const float i100 = p[srcidx + dx];
    const float i010 = p[srcidx + dy];
    const float i110 = p[srcidx + dx + dy];
    const float i001 = p[srcidx + dz];
    const float i101 = p[srcidx + dx + dz];
    const float i011 = p[srcidx + dy + dz];
    const float i111 = p[srcidx + dx + dy + dz];

    return mix( mix( mix(i000, i100, sx),
                     mix(i010, i110, sx), sy),
                mix( mix(i001, i101, sx),
                     mix(i011, i111, sx), sy),
                sz);
}

static float4
linesegInterpolationWeights(float u)
{
    return (float4)(1 - u, u, 0, 0);
}

// From GEOtriInterpolationWeights, but
static float4
triInterpolationWeights(float u, float v)
{
    // Triangle - use barycentric coordinates

    // This is a hack to make sure we are given proper
    // barycentric u, v coordinates.  That is, we require
    // u+v <= 1, and if that's not the case we hack it so
    // u = 1-u, v = 1-v, thus ensuring u+v <= 1.  (This
    // assumes that u, v are each between 0 and 1)
    // This is used for when evaluateInteriorPoint is
    // called from POP_GenVar for birthing from a surface.
    //
    // Note we actually flip on the u+v = 1 axis instead
    // of what is described above so slightly outside points
    // do not teleport to opposite locations.
    float uv = 1 - u - v;

    // Assume valid uv's.
#if 0
    if (uv < 0)
    {
        u += uv;
        v += uv;
        uv = -uv;
    }
#endif

    return (float4) (uv, u, v, 0);
}

// From GEOquadInterpolationWeights
static float4
quadInterpolationWeights(float u, float v)
{
    float u1 = 1 - u;
    float v1 = 1 - v;
    return (float4)(u1 * v1, u1 * v, u * v, u * v1);
}

// From GEO_PrimTetrahedron::remapTetCoords
static float4
tetInterpolationWeights(float u, float v, float w)
{
    float uvw = 1 - u - v - w;

    // Assume valid uv's.
#if 0
    if (uvw < 0)
    {
        // Mirror in u + v == 1 plane, reducing from 6 tetrahedra to 3,
        // i.e. a right triangular prism, whose right-angle edge is
        // along the w axis.
        if (u + v > 1)
        {
            float bary = 1 - u - v;
            u += bary;
            v += bary;
            uvw -= 2 * bary;
        }
        // Mirror the far tetrahedron (shares no face with the tet to keep)
        // into the middle tetrahedron (shares one face with the tet to keep)
        if (u + w > 1)
        {
            // Weight of point at (1,0,1), since 1+1-1 = 1
            float weight = (u + w - 1);
            // Subtract component of (1,0,1), which only changes u and w
            u -= weight;//*1
            //v -= weight*0;
            w -= weight;//*1
            // Add component of (0,1,0), which only changes v
            //u += weight*0;
            v += weight;//*1
            //w += weight*0;
            // Update uvw
            uvw += weight;
        }
        // Mirror the remaining outside tetrahedron into the tet to keep
        if (uvw < 0)
        {
            // Weight of point at (0,1,1), since -(1-0-1-1) = 1
            float weight = -uvw;
            // Subtract component of (0,1,1), which only changes v and w
            //u -= weight*0
            v -= weight;//*1
            w -= weight;//*1
            // Add component of (0,0,0), which requires no change
            // Update uvw
            uvw = -uvw; //equivalent to uvw += 2*weight;
        }
    }
#endif

    return (float4)(uvw, u, v, w);
}

void
computeSubdCurveCoeffsAndIndices(
    float u,
    int n,
    bool closed,
    float4 *coeffs_ptr,
    int4 *indices_ptr)
{
    const float16 theSubDFirstBasis = {
        1.0, -1.0,  0.0,  1.0/6.0,
        0.0,  1.0,  0.0, -1.0/3.0,
        0.0,  0.0,  0.0,  1.0/6.0,
        0.0,  0.0,  0.0,  0.0
    };

    const float16 theOpenBasis = {
        1.0/6.0, -0.5,  0.5, -1.0/6.0,
        2.0/3.0,  0.0, -1.0,  0.5,
        1.0/6.0,  0.5,  0.5, -0.5,
        0.0,      0.0,  0.0,  1.0/6.0
    };

    float4 coeffs = 0.0;
    int4 indices = 0.0;

    // UT_ASSERT_P(n >= 1);

    // Special cases for n <= 2
    if (n == 1)
    {
        indices.x = -1;
        indices.y = 0;
        indices.z = -1;
        indices.w = -1;

        coeffs.x = 0;
        coeffs.y = 1;
        coeffs.z = 0;
        coeffs.w = 0;

        *coeffs_ptr = coeffs;
        *indices_ptr = indices;
    }
    else if (n == 2)
    {
        if (closed)
        {
            u *= 2;
            if (u > 1)
                u = 2-u;
        }

        indices.x = -1;
        indices.y = 0;
        indices.z = 1;
        indices.w = -1;

        coeffs.x = 0.0;
        coeffs.y = 1.0 - u;
        coeffs.z = u;
        coeffs.w = 0.0;
    }

    const float16 *basis = &theOpenBasis;
    const float16 *midbasis = &theOpenBasis;

    int nedges = n - !closed;
    u *= nedges;
    int i = floor(u);
    i = clamp(i, 0, nedges-1);
    float t = u - i;

    float16 temp;
    if (i == 0)
    {
        if (closed)
        {
            indices.x = n-1;
            indices.y = 0;
            indices.z = 1;
            indices.w = 2;
        }
        else
        {
            basis = &theSubDFirstBasis;

            indices.x = 0;
            indices.y = 1;
            indices.z = 2;
            indices.w = -1;
        }
    }
    else if (i >= n-2)
    {
        if (closed)
        {
            if (i == n-2)
            {
                indices.x = n-3;
                indices.y = n-2;
                indices.z = n-1;
                indices.w = 0;
            }
            else
            {
                // UT_ASSERT_P(i == n-1);
                indices.x = n-2;
                indices.y = n-1;
                indices.z = 0;
                indices.w = 1;
            }
        }
        else
        {
            basis = &theSubDFirstBasis;

            indices.x = -1;
            indices.y = n-3;
            indices.z = n-2;
            indices.w = n-1;

            temp.lo.lo = (*basis).hi.hi;
            temp.lo.hi = (*basis).hi.lo;
            temp.hi.lo = (*basis).lo.hi;
            temp.hi.hi = (*basis).lo.lo;
            basis = &temp;
            t = 1-t;
        }
    }
    else
    {
        indices.x = i-1;
        indices.y = i;
        indices.z = i+1;
        indices.w = i+2;
    }

    float t2 = t*t;
    float4 tpow = {1.0f, t, t2, t2*t};

    (*coeffs_ptr).x = dot(tpow, (*basis).lo.lo);
    (*coeffs_ptr).y = dot(tpow, (*basis).lo.hi);
    (*coeffs_ptr).z = dot(tpow, (*basis).hi.lo);
    (*coeffs_ptr).w = dot(tpow, (*basis).hi.hi);

    *indices_ptr = indices;
}

float3 evaluateCubicCoeffs_vload3(float4 coeffs, int4 indices, global float *values)
{
    float3 res = 0;

    if (indices.x >= 0)
        res += coeffs.x * vload3(indices.x, values);
    if (indices.y >= 0)
        res += coeffs.y * vload3(indices.y, values);
    if (indices.z >= 0)
        res += coeffs.z * vload3(indices.z, values);
    if (indices.w >= 0)
        res += coeffs.w * vload3(indices.w, values);

    return res;
}

float3 evaluateCubicCoeffs_3(float4 coeffs, int4 indices, float3 *values)
{
    float3 res = 0;

    if (indices.x >= 0)
        res += coeffs.x * values[indices.x];
    if (indices.y >= 0)
        res += coeffs.y * values[indices.y];
    if (indices.z >= 0)
        res += coeffs.z * values[indices.z];
    if (indices.w >= 0)
        res += coeffs.w * values[indices.w];

    return res;
}
#endif
