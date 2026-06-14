#ifndef __mtlx_NOISE_H__
#define __mtlx_NOISE_H__

#include "mtlx_noise_internal.h"

/// mtlx_noise2d_X: equivalent to MaterialX noise2d node, produces X channels.
/// Perlin noise from 2D space.
static float
mtlx_noise2d_1(float amplitude, float pivot, float2 texcoord, int2 period)
{
    float value = mx_perlin_noise_float_2(texcoord, period);
    return value * amplitude + pivot;
}
static float2
mtlx_noise2d_2(float2 amplitude, float2 pivot, float2 texcoord, int2 period)
{
    float3 value = mx_perlin_noise_float3_2(texcoord, period);
    return value.xy * amplitude + pivot;
}
static float3
mtlx_noise2d_3(float3 amplitude, float3 pivot, float2 texcoord, int2 period)
{
    float3 value = mx_perlin_noise_float3_2(texcoord, period);
    return value * amplitude + pivot;
}
static float4
mtlx_noise2d_4(float4 amplitude, float4 pivot, float2 texcoord, int2 period)
{
    float3 xyz = mx_perlin_noise_float3_2(texcoord, period);
    float w = mx_perlin_noise_float_2(texcoord + (float2)(19.0, 73.0), period);
    return (float4)(xyz, w) * amplitude + pivot;
}

/// mtlx_noise3d_X: equivalent to MaterialX noise3d node, produces X channels.
/// Perlin noise from 3D space.
static float
mtlx_noise3d_1(float amplitude, float pivot, float3 position, int3 period)
{
    float value = mx_perlin_noise_float_3(position, period);
    return value * amplitude + pivot;
}
static float2
mtlx_noise3d_2(float2 amplitude, float2 pivot, float3 position, int3 period)
{
    float3 value = mx_perlin_noise_float3_3(position, period);
    return value.xy * amplitude + pivot;
}
static float3
mtlx_noise3d_3(float3 amplitude, float3 pivot, float3 position, int3 period)
{
    float3 value = mx_perlin_noise_float3_3(position, period);
    return value * amplitude + pivot;
}
static float4
mtlx_noise3d_4(float4 amplitude, float4 pivot, float3 position, int3 period)
{
    float3 xyz = mx_perlin_noise_float3_3(position, period);
    float w = mx_perlin_noise_float_3(position + (float3)(19.0, 73.0, 29.0),
                                      period);
    return (float4)(xyz, w) * amplitude + pivot;
}

/// mtlx_fractal3d_X: equivalent to MaterialX fractal3d node, produces X
/// channels. Combination of several octaves of Perlin noise from 3D space.
static float
mtlx_fractal3d_1(float amplitude, int octaves, float lacunarity, float diminish,
                 float3 position, int3 period)
{
    float value = mx_fractal_noise_float(position, octaves, lacunarity,
                                         diminish, period);
    return value * amplitude;
}
static float2
mtlx_fractal3d_2(float2 amplitude, int octaves, float lacunarity,
                 float diminish, float3 position, int3 period)
{
    float2 value = mx_fractal_noise_float2(position, octaves, lacunarity,
                                           diminish, period);
    return value * amplitude;
}
static float3
mtlx_fractal3d_3(float3 amplitude, int octaves, float lacunarity,
                 float diminish, float3 position, int3 period)
{
    float3 value = mx_fractal_noise_float3(position, octaves, lacunarity,
                                           diminish, period);
    return value * amplitude;
}
static float4
mtlx_fractal3d_4(float4 amplitude, int octaves, float lacunarity,
                 float diminish, float3 position, int3 period)
{
    float4 value = mx_fractal_noise_float4(position, octaves, lacunarity,
                                           diminish, period);
    return value * amplitude;
}

/// mtlx_cellnoise2d_1: equivalent to MaterialX cellnoise2d node, produces 1
/// channel from 2D position.
static float
mtlx_cellnoise2d_1(float2 texcoord, int2 period)
{
    return mx_cell_noise_float_2(texcoord, period);
}

/// mtlx_cellnoise3d_1: equivalent to MaterialX cellnoise3d node, produces 1
/// channel from 3D position.
static float
mtlx_cellnoise3d_1(float3 position, int3 period)
{
    return mx_cell_noise_float_3(position, period);
}

/// mtlx_worleynoise2d_X: equivalent to MaterialX worleynoise2d node, produces X
/// channels. Worley noise from 2D position.
static float
mtlx_worleynoise2d_1(float2 texcoord, float jitter, int2 period)
{
    return mx_worley_noise_float_2(texcoord, jitter, 0, period);
}
static float2
mtlx_worleynoise2d_2(float2 texcoord, float jitter, int2 period)
{
    return mx_worley_noise_float2_2(texcoord, jitter, 0, period);
}
static float3
mtlx_worleynoise2d_3(float2 texcoord, float jitter, int2 period)
{
    return mx_worley_noise_float3_2(texcoord, jitter, 0, period);
}

/// mtlx_worleynoise3d_X: equivalent to MaterialX worleynoise3d node, produces X
/// channels. Worley noise from 3D position.
static float
mtlx_worleynoise3d_1(float3 position, float jitter, int3 period)
{
    return mx_worley_noise_float_3(position, jitter, 0, period);
}
static float2
mtlx_worleynoise3d_2(float3 position, float jitter, int3 period)
{
    return mx_worley_noise_float2_3(position, jitter, 0, period);
}
static float3
mtlx_worleynoise3d_3(float3 position, float jitter, int3 period)
{
    return mx_worley_noise_float3_3(position, jitter, 0, period);
}

#endif

