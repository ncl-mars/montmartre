/*
Noise Library.

This library is a modified version of the noise library found in
Open Shading Language:
github.com/imageworks/OpenShadingLanguage/blob/master/src/include/OSL/oslnoise.h

It contains the subset of noise types needed to implement the MaterialX
standard library. The modifications are mainly conversions from C++ to GLSL.
Produced results should be identical to the OSL noise functions.

Original copyright notice:
------------------------------------------------------------------------
Copyright (c) 2009-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
------------------------------------------------------------------------
*/

#ifndef __MX_NOISE_INTERNAL_H__
#define __MX_NOISE_INTERNAL_H__

static int
mx_wrap_int(int x, int period)
{
    if (period <= 0)
        return x;
    int y = x - x / period * period;
    return y >= 0 ? y : y + period;
}

static float
mx_select(bool b, float t, float f)
{
    return b ? t : f;
}

static float
mx_negate_if(float val, bool b)
{
    return b ? -val : val;
}

static int
mx_floor(float x)
{
    return (int)(floor(x));
}

// return mx_floor as well as the fractional remainder
static float
mx_floorfrac(float x, int* i)
{
    *i = mx_floor(x);
    return x - (float)(*i);
}

static float
mx_bilerp1(float v0, float v1, float v2, float v3, float s, float t)
{
    float s1 = 1.0f - s;
    return (1.0f - t) * (v0*s1 + v1*s) + t * (v2*s1 + v3*s);
}
static float3
mx_bilerp3(float3 v0, float3 v1, float3 v2, float3 v3,
           float s, float t)
{
    float s1 = 1.0f - s;
    return (1.0f - t) * (v0*s1 + v1*s) + t * (v2*s1 + v3*s);
}
static float
mx_trilerp1(float v0, float v1, float v2, float v3,
            float v4, float v5, float v6, float v7,
            float s, float t, float r)
{
    float s1 = 1.0f - s;
    float t1 = 1.0f - t;
    float r1 = 1.0f - r;
    return (r1*(t1*(v0*s1 + v1*s) + t*(v2*s1 + v3*s)) +
            r*(t1*(v4*s1 + v5*s) + t*(v6*s1 + v7*s)));
}
static float3
mx_trilerp3(float3 v0, float3 v1, float3 v2, float3 v3,
            float3 v4, float3 v5, float3 v6, float3 v7,
            float s, float t, float r)
{
    float s1 = 1.0f - s;
    float t1 = 1.0f - t;
    float r1 = 1.0f - r;
    return (r1*(t1*(v0*s1 + v1*s) + t*(v2*s1 + v3*s)) +
            r*(t1*(v4*s1 + v5*s) + t*(v6*s1 + v7*s)));
}

// 2 and 3 dimensional gradient functions - perform a dot product against a
// randomly chosen vector. Note that the gradient vector is not normalized, but
// this only affects the overal "scale" of the result, so we simply account for
// the scale by multiplying in the corresponding "perlin" function.
static float
mx_gradient_float1_2(uint hash, float x, float y)
{
    // 8 possible directions (+-1,+-2) and (+-2,+-1)
    uint h = hash & 7u;
    float u = mx_select(h<4u, x, y);
    float v = 2.0 * mx_select(h<4u, y, x);
    // compute the dot product with (x,y).
    return mx_negate_if(u, (bool)(h&1u)) + mx_negate_if(v, (bool)(h&2u));
}
static float
mx_gradient_float1_3(uint hash, float x, float y, float z)
{
    // use vectors pointing to the edges of the cube
    uint h = hash & 15u;
    float u = mx_select(h<8u, x, y);
    float v = mx_select(h<4u, y, mx_select((h==12u)||(h==14u), x, z));
    return mx_negate_if(u, (bool)(h&1u)) + mx_negate_if(v, (bool)(h&2u));
}
static float3
mx_gradient_float3_2(uint3 hash, float x, float y)
{
    return (float3)(mx_gradient_float1_2(hash.x, x, y),
                    mx_gradient_float1_2(hash.y, x, y),
                    mx_gradient_float1_2(hash.z, x, y));
}
static float3
mx_gradient_float3_3(uint3 hash, float x, float y, float z)
{
    return (float3)(mx_gradient_float1_3(hash.x, x, y, z),
                    mx_gradient_float1_3(hash.y, x, y, z),
                    mx_gradient_float1_3(hash.z, x, y, z));
}
// Scaling factors to normalize the result of gradients above.
// These factors were experimentally calculated to be:
//    2D:   0.6616
//    3D:   0.9820
static float mx_gradient_scale2d_1(float v) { return 0.6616f * v; }
static float mx_gradient_scale3d_1(float v) { return 0.9820f * v; }
static float3 mx_gradient_scale2d_3(float3 v) { return 0.6616f * v; }
static float3 mx_gradient_scale3d_3(float3 v) { return 0.9820f * v; }

/// Bitwise circular rotation left by k bits (for 32 bit unsigned integers)
static uint
mx_rotl32(uint x, int k)
{
    return (x<<k) | (x>>(32-k));
}

static void
mx_bjmix(uint* a, uint* b, uint* c)
{
    *a -= *c; *a ^= mx_rotl32(*c, 4); *c += *b;
    *b -= *a; *b ^= mx_rotl32(*a, 6); *a += *c;
    *c -= *b; *c ^= mx_rotl32(*b, 8); *b += *a;
    *a -= *c; *a ^= mx_rotl32(*c,16); *c += *b;
    *b -= *a; *b ^= mx_rotl32(*a,19); *a += *c;
    *c -= *b; *c ^= mx_rotl32(*b, 4); *b += *a;
}

// Mix up and combine the bits of a, b, and c (doesn't change them, but
// returns a hash of those three original values).
static uint
mx_bjfinal(uint a, uint b, uint c)
{
    c ^= b; c -= mx_rotl32(b,14);
    a ^= c; a -= mx_rotl32(c,11);
    b ^= a; b -= mx_rotl32(a,25);
    c ^= b; c -= mx_rotl32(b,16);
    a ^= c; a -= mx_rotl32(c,4);
    b ^= a; b -= mx_rotl32(a,14);
    c ^= b; c -= mx_rotl32(b,24);
    return c;
}

// Convert a 32 bit integer into a floating point number in [0,1]
static float
mx_bits_to_01(uint bits)
{
    return (float)(bits) / (float)((uint)(0xffffffff));
}

static float
mx_fade(float t)
{
   return t * t * t * (t * (t * 6.0f- 15.0f) + 10.0f);
}

static uint
mx_hash_int1p(int x, int px)
{
    x = mx_wrap_int(x, px);
    
    uint len = 1u;
    uint seed = (uint)(0xdeadbeef) + (len << 2u) + 13u;
    return mx_bjfinal(seed+(uint)(x), seed, seed);
}

static uint
mx_hash_int2p(int x, int y, int px, int py)
{
    x = mx_wrap_int(x, px);
    y = mx_wrap_int(y, py);

    uint len = 2u;
    uint a, b, c;
    a = b = c = (uint)(0xdeadbeef) + (len << 2u) + 13u;
    a += (uint)(x);
    b += (uint)(y);
    return mx_bjfinal(a, b, c);
}

static uint
mx_hash_int3p(int x, int y, int z, int px, int py, int pz)
{
    x = mx_wrap_int(x, px);
    y = mx_wrap_int(y, py);
    z = mx_wrap_int(z, pz);
    
    uint len = 3u;
    uint a, b, c;
    a = b = c = (uint)(0xdeadbeef) + (len << 2u) + 13u;
    a += (uint)(x);
    b += (uint)(y);
    c += (uint)(z);
    return mx_bjfinal(a, b, c);
}

static uint
mx_hash_int4p(int x, int y, int z, int xx, int px, int py, int pz, int pxx)
{
    x = mx_wrap_int(x, px);
    y = mx_wrap_int(y, py);
    z = mx_wrap_int(z, pz);
    xx = mx_wrap_int(xx, pxx);
    
    uint len = 4u;
    uint a, b, c;
    a = b = c = (uint)(0xdeadbeef) + (len << 2u) + 13u;
    a += (uint)(x);
    b += (uint)(y);
    c += (uint)(z);
    mx_bjmix(&a, &b, &c);
    a += (uint)(xx);
    return mx_bjfinal(a, b, c);
}

static uint
mx_hash_int4pb(int x, int y, int z, int xx, int yy,
               int px, int py, int pz, int pxx)
{
    x = mx_wrap_int(x, px);
    y = mx_wrap_int(y, py);
    z = mx_wrap_int(z, pz);
    xx = mx_wrap_int(xx, pxx);

    uint len = 5u;
    uint a, b, c;
    a = b = c = (uint)(0xdeadbeef) + (len << 2u) + 13u;
    a += (uint)(x);
    b += (uint)(y);
    c += (uint)(z);
    mx_bjmix(&a, &b, &c);
    a += (uint)(xx);
    b += (uint)(yy);
    return mx_bjfinal(a, b, c);
}

static uint3
mx_hash_float3_2p(int x, int y, int px, int py)
{
    uint h = mx_hash_int2p(x, y, px, py);
    // we only need the low-order bits to be random, so split out
    // the 32 bit result into 3 parts for each channel
    uint3 result;
    result.x = (h      ) & 0xFFu;
    result.y = (h >> 8 ) & 0xFFu;
    result.z = (h >> 16) & 0xFFu;
    return result;
}

static uint3
mx_hash_float3_3p(int x, int y, int z, int px, int py, int pz)
{
    x = mx_wrap_int(x, px);
    y = mx_wrap_int(y, py);
    z = mx_wrap_int(z, pz);

    uint h = mx_hash_int3p(x, y, z, px, py, pz);
    // we only need the low-order bits to be random, so split out
    // the 32 bit result into 3 parts for each channel
    uint3 result;
    result.x = (h      ) & 0xFFu;
    result.y = (h >> 8 ) & 0xFFu;
    result.z = (h >> 16) & 0xFFu;
    return result;
}

static float
mx_perlin_noise_float_2(float2 p, int2 per)
{
    int X, Y;
    float fx = mx_floorfrac(p.x, &X);
    float fy = mx_floorfrac(p.y, &Y);
    float u = mx_fade(fx);
    float v = mx_fade(fy);
    float result = mx_bilerp1(
        mx_gradient_float1_2(mx_hash_int2p(X  , Y  , per.x, per.y), fx    , fy    ),
        mx_gradient_float1_2(mx_hash_int2p(X+1, Y  , per.x, per.y), fx-1.0f, fy    ),
        mx_gradient_float1_2(mx_hash_int2p(X  , Y+1, per.x, per.y), fx    , fy-1.0f),
        mx_gradient_float1_2(mx_hash_int2p(X+1, Y+1, per.x, per.y), fx-1.0f, fy-1.0f),
        u, v);
    return mx_gradient_scale2d_1(result);
}

static float
mx_perlin_noise_float_3(float3 p, int3 per)
{
    int X, Y, Z;
    float fx = mx_floorfrac(p.x, &X);
    float fy = mx_floorfrac(p.y, &Y);
    float fz = mx_floorfrac(p.z, &Z);
    float u = mx_fade(fx);
    float v = mx_fade(fy);
    float w = mx_fade(fz);
    float result = mx_trilerp1(
        mx_gradient_float1_3(mx_hash_int3p(X  , Y  , Z  , per.x, per.y, per.z),
                             fx    , fy    , fz    ),
        mx_gradient_float1_3(mx_hash_int3p(X+1, Y  , Z  , per.x, per.y, per.z),
                             fx-1.0f, fy    , fz    ),
        mx_gradient_float1_3(mx_hash_int3p(X  , Y+1, Z  , per.x, per.y, per.z),
                             fx    , fy-1.0f, fz    ),
        mx_gradient_float1_3(mx_hash_int3p(X+1, Y+1, Z  , per.x, per.y, per.z),
                             fx-1.0f, fy-1.0f, fz    ),
        mx_gradient_float1_3(mx_hash_int3p(X  , Y  , Z+1, per.x, per.y, per.z),
                             fx    , fy    , fz-1.0f),
        mx_gradient_float1_3(mx_hash_int3p(X+1, Y  , Z+1, per.x, per.y, per.z),
                             fx-1.0f, fy    , fz-1.0f),
        mx_gradient_float1_3(mx_hash_int3p(X  , Y+1, Z+1, per.x, per.y, per.z),
                             fx    , fy-1.0f, fz-1.0f),
        mx_gradient_float1_3(mx_hash_int3p(X+1, Y+1, Z+1, per.x, per.y, per.z),
                             fx-1.0f, fy-1.0f, fz-1.0f),
        u, v, w);
    return mx_gradient_scale3d_1(result);
}

static float3
mx_perlin_noise_float3_2(float2 p, int2 per)
{
    int X, Y;
    float fx = mx_floorfrac(p.x, &X);
    float fy = mx_floorfrac(p.y, &Y);
    float u = mx_fade(fx);
    float v = mx_fade(fy);
    float3 result = mx_bilerp3(
        mx_gradient_float3_2(mx_hash_float3_2p(X  , Y  , per.x, per.y), fx    , fy    ),
        mx_gradient_float3_2(mx_hash_float3_2p(X+1, Y  , per.x, per.y), fx-1.0f, fy    ),
        mx_gradient_float3_2(mx_hash_float3_2p(X  , Y+1, per.x, per.y), fx    , fy-1.0f),
        mx_gradient_float3_2(mx_hash_float3_2p(X+1, Y+1, per.x, per.y), fx-1.0f, fy-1.0f),
        u, v);
    return mx_gradient_scale2d_3(result);
}

static float3
mx_perlin_noise_float3_3(float3 p, int3 per)
{
    int X, Y, Z;
    float fx = mx_floorfrac(p.x, &X);
    float fy = mx_floorfrac(p.y, &Y);
    float fz = mx_floorfrac(p.z, &Z);
    float u = mx_fade(fx);
    float v = mx_fade(fy);
    float w = mx_fade(fz);
    float3 result = mx_trilerp3(
        mx_gradient_float3_3(mx_hash_float3_3p(X  , Y  , Z  , per.x, per.y, per.z),
                         fx    , fy    , fz    ),
        mx_gradient_float3_3(mx_hash_float3_3p(X+1, Y  , Z  , per.x, per.y, per.z),
                         fx-1.0f, fy    , fz    ),
        mx_gradient_float3_3(mx_hash_float3_3p(X  , Y+1, Z  , per.x, per.y, per.z),
                         fx    , fy-1.0f, fz    ),
        mx_gradient_float3_3(mx_hash_float3_3p(X+1, Y+1, Z  , per.x, per.y, per.z),
                         fx-1.0f, fy-1.0f, fz    ),
        mx_gradient_float3_3(mx_hash_float3_3p(X  , Y  , Z+1, per.x, per.y, per.z),
                         fx    , fy    , fz-1.0f),
        mx_gradient_float3_3(mx_hash_float3_3p(X+1, Y  , Z+1, per.x, per.y, per.z),
                         fx-1.0f, fy    , fz-1.0f),
        mx_gradient_float3_3(mx_hash_float3_3p(X  , Y+1, Z+1, per.x, per.y, per.z),
                         fx    , fy-1.0f, fz-1.0f),
        mx_gradient_float3_3(mx_hash_float3_3p(X+1, Y+1, Z+1, per.x, per.y, per.z),
                         fx-1.0f, fy-1.0f, fz-1.0f),
        u, v, w);
    return mx_gradient_scale3d_3(result);
}

static float
mx_cell_noise_float_1(float p, int per)
{
    int ix = mx_floor(p);
    return mx_bits_to_01(mx_hash_int1p(ix, per));
}

static float
mx_cell_noise_float_2(float2 p, int2 per)
{
    int ix = mx_floor(p.x);
    int iy = mx_floor(p.y);
    return mx_bits_to_01(mx_hash_int2p(ix, iy, per.x, per.y));
}

static float
mx_cell_noise_float_3(float3 p, int3 per)
{
    int ix = mx_floor(p.x);
    int iy = mx_floor(p.y);
    int iz = mx_floor(p.z);
    return mx_bits_to_01(mx_hash_int3p(ix, iy, iz, per.x, per.y, per.z));
}

static float
mx_cell_noise_float_4(float4 p, int4 per)
{
    int ix = mx_floor(p.x);
    int iy = mx_floor(p.y);
    int iz = mx_floor(p.z);
    int iw = mx_floor(p.w);
    return mx_bits_to_01(mx_hash_int4p(ix, iy, iz, iw,
                                     per.x, per.y, per.z, per.w));
}

static float3
mx_cell_noise_float3_1(float p, int per)
{
    int ix = mx_floor(p);
    return (float3)(
            mx_bits_to_01(mx_hash_int2p(ix, 0, per, 0)),
            mx_bits_to_01(mx_hash_int2p(ix, 1, per, 0)),
            mx_bits_to_01(mx_hash_int2p(ix, 2, per, 0))
    );
}

static float3
mx_cell_noise_float3_2(float2 p, int2 per)
{
    int ix = mx_floor(p.x);
    int iy = mx_floor(p.y);
    return (float3)(
            mx_bits_to_01(mx_hash_int3p(ix, iy, 0, per.x, per.y, 0)),
            mx_bits_to_01(mx_hash_int3p(ix, iy, 1, per.x, per.y, 0)),
            mx_bits_to_01(mx_hash_int3p(ix, iy, 2, per.x, per.y, 0))
    );
}

static float3
mx_cell_noise_float3_3(float3 p, int3 per)
{
    int ix = mx_floor(p.x);
    int iy = mx_floor(p.y);
    int iz = mx_floor(p.z);
    return (float3)(
            mx_bits_to_01(mx_hash_int4p(ix, iy, iz, 0, per.x, per.y, per.z, 0)),
            mx_bits_to_01(mx_hash_int4p(ix, iy, iz, 1, per.x, per.y, per.z, 0)),
            mx_bits_to_01(mx_hash_int4p(ix, iy, iz, 2, per.x, per.y, per.z, 0))
    );
}

static float3
mx_cell_noise_float3_4(float4 p, int4 per)
{
    int ix = mx_floor(p.x);
    int iy = mx_floor(p.y);
    int iz = mx_floor(p.z);
    int iw = mx_floor(p.w);
    return (float3)(
            mx_bits_to_01(mx_hash_int4pb(ix, iy, iz, iw, 0,
                                         per.x, per.y, per.z, per.w)),
            mx_bits_to_01(mx_hash_int4pb(ix, iy, iz, iw, 1,
                                         per.x, per.y, per.z, per.w)),
            mx_bits_to_01(mx_hash_int4pb(ix, iy, iz, iw, 2,
                                         per.x, per.y, per.z, per.w))
    );
}

static float
mx_fractal_noise_float(float3 p, int octaves, float lacunarity, float diminish,
                       int3 per)
{
    // If period is positive along some axis, lacunarity has to be an integer to
    // preserve tileability.
    
    float result = 0.0f;
    float amplitude = 1.0;
    for (int i = 0;  i < octaves; ++i)
    {
        result += amplitude * mx_perlin_noise_float_3(p, per);
        amplitude *= diminish;
        p *= lacunarity;
    }
    return result;
}

static float3
mx_fractal_noise_float3(float3 p, int octaves, float lacunarity, float diminish,
                        int3 per)
{
    // If period is positive along some axis, lacunarity has to be an integer to
    // preserve tileability.

    float3 result = (float3)(0.0f);
    float amplitude = 1.0;
    for (int i = 0;  i < octaves; ++i)
    {
        result += amplitude * mx_perlin_noise_float3_3(p, per);
        amplitude *= diminish;
        p *= lacunarity;
    }
    return result;
}

static float2
mx_fractal_noise_float2(float3 p, int octaves, float lacunarity, float diminish,
                        int3 per)
{
    // If period is positive along some axis, lacunarity has to be an integer to
    // preserve tileability.
    // Offsetting positions for the second component results in the exact same
    // value for both if offset is an integral vector multiple of period.

    return (float2)(mx_fractal_noise_float(p, octaves, lacunarity,
                                           diminish, per),
                    mx_fractal_noise_float(p+(float3)(19, 193, 17), octaves, lacunarity,
                                           diminish, per));
}

static float4
mx_fractal_noise_float4(float3 p, int octaves, float lacunarity, float diminish,
                        int3 per)
{
    // If period is positive along some axis, lacunarity has to be an integer to
    // preserve tileability.

    float3 c = mx_fractal_noise_float3(p, octaves, lacunarity, diminish, per);
    float f = mx_fractal_noise_float(p+(float3)(19, 193, 17), octaves, lacunarity,
                                     diminish, per);
    return (float4)(c, f);
}

static float
mx_worley_distance2(float2 p, int x, int y, int xoff, int yoff, float jitter,
                    int metric, int2 per)
{
    float3  tmp = mx_cell_noise_float3_2((float2)(x+xoff, y+yoff), per);
    float2  off = (float2)(tmp.x, tmp.y);

    off -= 0.5f;
    off *= jitter;
    off += 0.5f;

    float2 cellpos = (float2)((float)(x), (float)(y)) + off;
    float2 diff = cellpos - p;
    if (metric == 2)
        return fabs(diff.x) + fabs(diff.y);       // Manhattan distance
    if (metric == 3)
        return max(fabs(diff.x), fabs(diff.y));   // Chebyshev distance
    // Either Euclidian or Distance^2
    return dot(diff, diff);
}

static float
mx_worley_distance3(float3 p, int x, int y, int z, int xoff, int yoff, int zoff,
                    float jitter, int metric, int3 per)
{
    float3  off = mx_cell_noise_float3_3((float3)(x+xoff, y+yoff, z+zoff), per);

    off -= 0.5f;
    off *= jitter;
    off += 0.5f;

    float3 cellpos = (float3)((float)(x), (float)(y), (float)(z)) + off;
    float3 diff = cellpos - p;
    if (metric == 2)
        return fabs(diff.x) + fabs(diff.y) + fabs(diff.z); // Manhattan distance
    if (metric == 3)
        return max(max(fabs(diff.x), fabs(diff.y)), fabs(diff.z)); // Chebyshev distance
    // Either Euclidian or Distance^2
    return dot(diff, diff);
}

static float
mx_worley_noise_float_2(float2 p, float jitter, int metric, int2 per)
{
    int X, Y;
    float2 localpos = (float2)(mx_floorfrac(p.x, &X), mx_floorfrac(p.y, &Y));
    float sqdist = 1e6f;        // Some big number for jitter > 1 (not all GPUs may be IEEE)
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float dist = mx_worley_distance2(localpos, x, y, X, Y, jitter, metric, per);
            sqdist = min(sqdist, dist);
        }
    }
    if (metric == 0)
        sqdist = sqrt(sqdist);
    return sqdist;
}

static float2
mx_worley_noise_float2_2(float2 p, float jitter, int metric, int2 per)
{
    int X, Y;
    float2 localpos = (float2)(mx_floorfrac(p.x, &X), mx_floorfrac(p.y, &Y));
    float2 sqdist = (float2)(1e6f, 1e6f);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float dist = mx_worley_distance2(localpos, x, y, X, Y, jitter, metric, per);
            if (dist < sqdist.x)
            {
                sqdist.y = sqdist.x;
                sqdist.x = dist;
            }
            else if (dist < sqdist.y)
            {
                sqdist.y = dist;
            }
        }
    }
    if (metric == 0)
        sqdist = sqrt(sqdist);
    return sqdist;
}

static float3
mx_worley_noise_float3_2(float2 p, float jitter, int metric, int2 per)
{
    int X, Y;
    float2 localpos = (float2)(mx_floorfrac(p.x, &X), mx_floorfrac(p.y, &Y));
    float3 sqdist = (float3)(1e6f, 1e6f, 1e6f);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float dist = mx_worley_distance2(localpos, x, y, X, Y, jitter, metric, per);
            if (dist < sqdist.x)
            {
                sqdist.z = sqdist.y;
                sqdist.y = sqdist.x;
                sqdist.x = dist;
            }
            else if (dist < sqdist.y)
            {
                sqdist.z = sqdist.y;
                sqdist.y = dist;
            }
            else if (dist < sqdist.z)
            {
                sqdist.z = dist;
            }
        }
    }
    if (metric == 0)
        sqdist = sqrt(sqdist);
    return sqdist;
}

static float
mx_worley_noise_float_3(float3 p, float jitter, int metric, int3 per)
{
    int X, Y, Z;
    float3 localpos = (float3)(mx_floorfrac(p.x, &X), mx_floorfrac(p.y, &Y), mx_floorfrac(p.z, &Z));
    float sqdist = 1e6f;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int z = -1; z <= 1; ++z)
            {
                float dist = mx_worley_distance3(localpos, x, y, z, X, Y, Z, jitter, metric, per);
                sqdist = min(sqdist, dist);
            }
        }
    }
    if (metric == 0)
        sqdist = sqrt(sqdist);
    return sqdist;
}

static float2
mx_worley_noise_float2_3(float3 p, float jitter, int metric, int3 per)
{
    int X, Y, Z;
    float3 localpos = (float3)(mx_floorfrac(p.x, &X), mx_floorfrac(p.y, &Y), mx_floorfrac(p.z, &Z));
    float2 sqdist = (float2)(1e6f, 1e6f);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int z = -1; z <= 1; ++z)
            {
                float dist = mx_worley_distance3(localpos, x, y, z, X, Y, Z, jitter, metric, per);
                if (dist < sqdist.x)
                {
                    sqdist.y = sqdist.x;
                    sqdist.x = dist;
                }
                else if (dist < sqdist.y)
                {
                    sqdist.y = dist;
                }
            }
        }
    }
    if (metric == 0)
        sqdist = sqrt(sqdist);
    return sqdist;
}

static float3
mx_worley_noise_float3_3(float3 p, float jitter, int metric, int3 per)
{
    int X, Y, Z;
    float3 localpos = (float3)(mx_floorfrac(p.x, &X), mx_floorfrac(p.y, &Y), mx_floorfrac(p.z, &Z));
    float3 sqdist = (float3)(1e6f, 1e6f, 1e6f);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int z = -1; z <= 1; ++z)
            {
                float dist = mx_worley_distance3(localpos, x, y, z, X, Y, Z, jitter, metric, per);
                if (dist < sqdist.x)
                {
                    sqdist.z = sqdist.y;
                    sqdist.y = sqdist.x;
                    sqdist.x = dist;
                }
                else if (dist < sqdist.y)
                {
                    sqdist.z = sqdist.y;
                    sqdist.y = dist;
                }
                else if (dist < sqdist.z)
                {
                    sqdist.z = dist;
                }
            }
        }
    }
    if (metric == 0)
        sqdist = sqrt(sqdist);
    return sqdist;
}

#endif

