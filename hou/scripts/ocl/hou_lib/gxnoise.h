#ifndef __SIMPLEX_NOISE_H
#define __SIMPLEX_NOISE_H

#include "random.h"

/******************************************************************************
 * Declaration of constants.
 ******************************************************************************/

// Sorting permutation for each comparison bit string. The string consist of
// results of c2<c3, c1<c3, c0<c3, c1<c2, c0<c2, c0<c1.
constant int _SIMPLEX_TRAVERSAL_ORDER[64][4] =
{
    {0, 1, 2, 3},   // ABCD -> 000000 = 00
    {1, 0, 2, 3},   // BACD -> 000001 = 01
    {0, 0, 0, 0},
    {1, 2, 0, 3},   // BCAD -> 000011 = 03
    {0, 2, 1, 3},   // ACBD -> 000100 = 04
    {0, 0, 0, 0},
    {2, 0, 1, 3},   // CABD -> 000110 = 06
    {2, 1, 0, 3},   // CBAD -> 000111 = 07
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {1, 2, 3, 0},   // BCDA -> 001011 = 11
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {2, 1, 3, 0},   // CBDA -> 001111 = 15
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 2, 3, 1},   // ACDB -> 010100 = 20
    {0, 0, 0, 0},
    {2, 0, 3, 1},   // CADB -> 010110 = 22
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {2, 3, 0, 1},   // CDAB -> 011110 = 30
    {2, 3, 1, 0},   // CDBA -> 011111 = 31
    {0, 1, 3, 2},   // ABDC -> 100000 = 32
    {1, 0, 3, 2},   // BADC -> 100001 = 33
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {1, 3, 0, 2},   // BDAC -> 101001 = 41
    {0, 0, 0, 0},
    {1, 3, 2, 0},   // BDCA -> 101011 = 43
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 3, 1, 2},   // ADBC -> 110000 = 48
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 3, 2, 1},   // ADCB -> 110100 = 52
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {3, 0, 1, 2},   // DABC -> 111000 = 56
    {3, 1, 0, 2},   // DBAC -> 111001 = 57
    {0, 0, 0, 0},
    {3, 1, 2, 0},   // DBCA -> 111011 = 59
    {3, 0, 2, 1},   // DACB -> 111100 = 60
    {0, 0, 0, 0},
    {3, 2, 0, 1},   // DCAB -> 111110 = 62
    {3, 2, 1, 0}    // DCBA -> 111111 = 63
};

/// Arrays of literals for offsets in each dimensionality.
constant float _SIMPLEX_OFFSETS2[] =
{
    1.0f, 0.0f,
    0.0f, 1.0f
};
constant float _SIMPLEX_OFFSETS3[] =
{
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};
constant float _SIMPLEX_OFFSETS4[] =
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

/// Skew and unskew factors.
constant float _SIMPLEX_F[] =
{
    0.366025403784439,
    0.333333333333333,
    0.309016994374947
};
constant float _SIMPLEX_G[] =
{
    0.211324865405187,
    0.166666666666667,
    0.138196601125011
};

/******************************************************************************
 * Gradient vector code.
 ******************************************************************************/

constant float _SIMPLEX_GRADIENTS_2[] =
{
    1.0f, 0.0f,
    -1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, -1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f,
    1.0f, -1.0f,
    -1.0f, -1.0f
};
#define _SIMPLEX_NG_2 8

constant float _SIMPLEX_GRADIENTS_3[] =
{
    1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f,
    1.0f, 0.0f, 1.0f,
    -1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, -1.0f,
    -1.0f, 0.0f, -1.0f,
    0.0f, 1.0f, 1.0f,
    0.0f, -1.0f, 1.0f,
    0.0f, 1.0f, -1.0f,
    0.0f, -1.0f, -1.0f
};
#define _SIMPLEX_NG_3 12

constant float _SIMPLEX_GRADIENTS_4[] =
{
    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, -1.0f,
    0.0f, 1.0f, -1.0f, 1.0f,
    0.0f, 1.0f, -1.0f, -1.0f,
    0.0f, -1.0f, 1.0f, 1.0f,
    0.0f, -1.0f, 1.0f, -1.0f,
    0.0f, -1.0f, -1.0f, 1.0f,
    0.0f, -1.0f, -1.0f, -1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    -1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, -1.0f,
    -1.0f, 0.0f, 1.0f, -1.0f,
    1.0f, 0.0f, -1.0f, 1.0f,
    -1.0f, 0.0f, -1.0f, 1.0f,
    1.0f, 0.0f, -1.0f, -1.0f,
    -1.0f, 0.0f, -1.0f, -1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 0.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, -1.0f,
    1.0f, -1.0f, 0.0f, -1.0f,
    -1.0f, 1.0f, 0.0f, -1.0f,
    -1.0f, -1.0f, 0.0f, -1.0f,
    1.0f, 1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, -1.0f, 0.0f,
    -1.0f, -1.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, -1.0f, 0.0f
};
#define _SIMPLEX_NG_4 32

/// Return gradient vector at each cell, given the seed.
float2 _getGradient_2(float2 x, uint seed)
{
    uint idx = as_uint(x.y) + SYSwang_inthash(as_uint(x.x));
    idx = SYSwang_inthash(seed + SYSwang_inthash(idx));
    return vload2(idx % _SIMPLEX_NG_2, _SIMPLEX_GRADIENTS_2);
}
float3 _getGradient_3(float3 x, uint seed)
{
    uint idx = as_uint(x.y) + SYSwang_inthash(as_uint(x.x));
    idx = as_uint(x.z) + SYSwang_inthash(idx);
    idx = SYSwang_inthash(seed + SYSwang_inthash(idx));
    return vload3(idx % _SIMPLEX_NG_3, _SIMPLEX_GRADIENTS_3);
}
float4 _getGradient_4(float4 x, uint seed)
{
    uint idx = as_uint(x.y) + SYSwang_inthash(as_uint(x.x));
    idx = as_uint(x.z) + SYSwang_inthash(idx);
    idx = as_uint(x.w) + SYSwang_inthash(idx);
    idx = SYSwang_inthash(seed + SYSwang_inthash(idx));
    return vload4(idx % _SIMPLEX_NG_4, _SIMPLEX_GRADIENTS_4);
}

/******************************************************************************
 * Function declaration templates.
 ******************************************************************************/

/// __SEED_X are the gradient seeds for noise of output dimension X.
#define __SEED_1(IN_DIM) IN_DIM
#define __SEED_2(IN_DIM) (uint2)(5, 10) + 10 * IN_DIM
#define __SEED_3(IN_DIM) (uint3)(10, 22, 34) + 100 * IN_DIM
#define __SEED_4(IN_DIM) (uint4)(20, 40, 60, 80) + 1000 * IN_DIM

/// __SUM_X(v) is the sum of all components of the X-dimensional vector v.
#define __SUM_1(vector) (vector)
#define __SUM_2(vector) (vector.x + vector.y)
#define __SUM_3(vector) (vector.x + vector.y + vector.z)
#define __SUM_4(vector) (vector.x + vector.y + vector.z + vector.w)

/// Vector (and seed) type for each dimensionality.
#define __OP_1 float
#define __OP_2 float2
#define __OP_3 float3
#define __OP_4 float4
#define __SEED_OP_1 uint
#define __SEED_OP_2 uint2
#define __SEED_OP_3 uint3
#define __SEED_OP_4 uint4

/// Amplification factors.
#define __AMP_2 35.0f
#define __AMP_3 38.4f
#define __AMP_4 31.3f

/// Position adjustment factors.
#define __FREQ_ADJ 0.65f

/// Gives the pairwise comparison score for the passed vector.
#define __PAIRWISE_COMP_SCORE_2(vector) \
    (vector.x < vector.y ? 1 : 0)
#define __PAIRWISE_COMP_SCORE_3(vector) \
    (vector.y < vector.z ? 4 : 0) + \
    (vector.x < vector.z ? 2 : 0) + \
    __PAIRWISE_COMP_SCORE_2(vector)
#define __PAIRWISE_COMP_SCORE_4(vector) \
    (vector.z < vector.w ? 32 : 0) + \
    (vector.y < vector.w ? 16 : 0) + \
    (vector.x < vector.w ? 8 : 0) + \
    __PAIRWISE_COMP_SCORE_3(vector)

/// __STACKED_GRAD_DOTS_X(Y) is the stacked vector of dot products of disp and
/// the pseudorandom gradient vector _getGradient_Y(int_x, seeds).
#define __SINGLE_GRAD_DOT(IN_DIM, SEED) \
    dot(disp, _getGradient_ ## IN_DIM (int_x, SEED))
#define __STACKED_GRAD_DOTS_1(IN_DIM) \
    __SINGLE_GRAD_DOT(IN_DIM, seed)
#define __STACKED_GRAD_DOTS_2(IN_DIM) \
    (float2)(__SINGLE_GRAD_DOT(IN_DIM, seed.x), \
             __SINGLE_GRAD_DOT(IN_DIM, seed.y))
#define __STACKED_GRAD_DOTS_3(IN_DIM) \
    (float3)(__SINGLE_GRAD_DOT(IN_DIM, seed.x), \
             __SINGLE_GRAD_DOT(IN_DIM, seed.y), \
             __SINGLE_GRAD_DOT(IN_DIM, seed.z))
#define __STACKED_GRAD_DOTS_4(IN_DIM) \
    (float4)(__SINGLE_GRAD_DOT(IN_DIM, seed.x), \
             __SINGLE_GRAD_DOT(IN_DIM, seed.y), \
             __SINGLE_GRAD_DOT(IN_DIM, seed.z), \
             __SINGLE_GRAD_DOT(IN_DIM, seed.w))

/// Declares the function of the form
///     floatY VEXgxnoise_X_Y(floatX x)
/// where X, Y are IN_DIM, OUT_DIM, respectively.
#define __DECLARE_NOISE_FUNC(IN_DIM, OUT_DIM) \
__OP_ ## OUT_DIM VEXgxnoise_ ## IN_DIM ## _ ## OUT_DIM (__OP_ ## IN_DIM x) \
{ \
    __SEED_OP_ ## OUT_DIM seed = __SEED_ ## OUT_DIM (IN_DIM);

/// Declares the helper function of the form
///     floatX __gxnoise_deriv_H_X(floatX x, uint seed)
#define __DECLARE_NOISE_DERIV_FUNC(IN_OUT_DIM) \
__OP_ ## IN_OUT_DIM __gxnoise_deriv_H_ ## IN_OUT_DIM (__OP_ ## IN_OUT_DIM x, \
    uint seed) \
{ 

/// Start of the noise function logic. The coordinates are first skewed, and the
/// corresponding cell located. This cell is then unskewed, and from it disp is
/// calculated (offset between the input and the cell origin). Finally, c is
/// computed--this is the traversal order index in the array of permutations.
#define __NOISE_FUNC_START(IN_DIM) \
    x *= __FREQ_ADJ; \
    float t = __SUM_ ## IN_DIM (x) * _SIMPLEX_F[IN_DIM - 2]; \
    __OP_ ## IN_DIM int_x = floor(x + t); \
    t = __SUM_ ## IN_DIM (int_x) * _SIMPLEX_G[IN_DIM - 2]; \
    __OP_ ## IN_DIM disp = x - (int_x - t); \
    int c = __PAIRWISE_COMP_SCORE_ ## IN_DIM (disp);

/// Adds the kernel contribution to val.
#define __NOISE_UPDATE(IN_DIM, OUT_DIM) \
    t = 0.5f - dot(disp, disp); \
    if (t > 0) \
    { \
        t = t * t; \
        val += t * t * __STACKED_GRAD_DOTS_ ## OUT_DIM (IN_DIM); \
    }

/// Adds the kernel derivative contribution to dest for the current grad and
/// disp vectors (and the up-to-date t value).
#define __RAW_DERIV_UPDATE(dest) \
        dest += (t * t * t) * (t * grad - 8.0f * dot(disp, grad) * disp);

/// Adds the kernel derivative contribution to val.
#define __DERIV_UPDATE(IN_OUT_DIM) \
    t = 0.5f - dot(disp, disp); \
    if (t > 0) \
    { \
        __OP_ ## IN_OUT_DIM grad = _getGradient_ ## IN_OUT_DIM(int_x, seed); \
        __RAW_DERIV_UPDATE(val) \
    }

/// Adds the kernel derivative contributions to deriv1, deriv2, and deriv3.
#define __DERIV_UPDATE3(IN_DIM) \
    t = 0.5f - dot(disp, disp); \
    if (t > 0) \
    { \
        __OP_ ## IN_DIM grad = _getGradient_ ## IN_DIM(int_x, seed.x); \
        __RAW_DERIV_UPDATE(deriv1); \
        grad = _getGradient_ ## IN_DIM(int_x, seed.y); \
        __RAW_DERIV_UPDATE(deriv2); \
        grad = _getGradient_ ## IN_DIM(int_x, seed.z); \
        __RAW_DERIV_UPDATE(deriv3); \
    }

/// Evaluates to the appropriate offset vector.
#define __SET_SIMPLEX_OFFSET(IN_DIM) \
    __OP_ ## IN_DIM offset = vload ## IN_DIM (_SIMPLEX_TRAVERSAL_ORDER[c][i], \
                                              _SIMPLEX_OFFSETS ## IN_DIM);


/// Updates vertex coordinate and the offset for the current iteration.
#define __CELL_DISP_UPDATE(IN_DIM) \
        __SET_SIMPLEX_OFFSET(IN_DIM) \
        int_x += offset; \
        disp -= offset - _SIMPLEX_G[IN_DIM - 2];

/// Computes the noise value in val.
#define __NOISE_VAL_COMPUTE(IN_DIM, OUT_DIM) \
    __OP_ ## OUT_DIM val = 0; \
    __NOISE_UPDATE(IN_DIM, OUT_DIM) \
    for (int i = 0; i < IN_DIM; i++) \
    { \
        __CELL_DISP_UPDATE(IN_DIM) \
        __NOISE_UPDATE(IN_DIM, OUT_DIM) \
    }

/// Computes gradient of the noise value in val.
#define __NOISE_DERIV_COMPUTE(IN_OUT_DIM) \
    __OP_ ## IN_OUT_DIM val = 0; \
    __DERIV_UPDATE(IN_OUT_DIM) \
    for (int i = 0; i < IN_OUT_DIM; i++) \
    { \
        __CELL_DISP_UPDATE(IN_OUT_DIM) \
        __DERIV_UPDATE(IN_OUT_DIM) \
    }

/// Computes gradients of the first three noise components in deriv1, deriv2,
/// deriv3 vectors.
#define __NOISE_DERIV_COMPUTE3(IN_DIM) \
    __OP_ ## IN_DIM deriv1 = 0; \
    __OP_ ## IN_DIM deriv2 = 0; \
    __OP_ ## IN_DIM deriv3 = 0; \
    __DERIV_UPDATE3(IN_DIM) \
    for (int i = 0; i < IN_DIM; i++) \
    { \
        __CELL_DISP_UPDATE(IN_DIM) \
        __DERIV_UPDATE3(IN_DIM) \
    }

/// Returns val and closes the function block (for the noise itself).
#define __NOISE_FUNC_END(IN_DIM) \
    return val * __AMP_ ## IN_DIM + 0.5f; \
}

/// Returns val and closes the function block (for the noise derivative).
#define __NOISE_DERIV_FUNC_END(IN_DIM) \
    return val * (__AMP_ ## IN_DIM * __FREQ_ADJ); \
}

/// Creates the code for the VEXgxnoise_X_Y() function.
#define __CREATE_NOISE_FUNC(IN_DIM, OUT_DIM) \
__DECLARE_NOISE_FUNC(IN_DIM, OUT_DIM) \
__NOISE_FUNC_START(IN_DIM) \
__NOISE_VAL_COMPUTE(IN_DIM, OUT_DIM) \
__NOISE_FUNC_END(IN_DIM)

/// Creates the code for the __gxnoise_deriv_H_X() helper function.
#define __CREATE_NOISE_DERIV_FUNC(IN_OUT_DIM) \
__DECLARE_NOISE_DERIV_FUNC(IN_OUT_DIM) \
__NOISE_FUNC_START(IN_OUT_DIM) \
__NOISE_DERIV_COMPUTE(IN_OUT_DIM) \
__NOISE_DERIV_FUNC_END(IN_OUT_DIM)

/// Creates all output noise flavors for the given input dimensionality.
#define CREATE_NOISE_FOR_ALL_OUTPUTS(IN_DIM) \
__CREATE_NOISE_FUNC(IN_DIM, 1) \
__CREATE_NOISE_FUNC(IN_DIM, 2) \
__CREATE_NOISE_FUNC(IN_DIM, 3) \
__CREATE_NOISE_FUNC(IN_DIM, 4) \

/// Creates the code for the sx_curl_noise_X_2() function.
#define CREATE_CURL_NOISE_FUNC2(IN_DIM) \
__CREATE_NOISE_DERIV_FUNC(IN_DIM) \
float2 VEXcurlgxnoise2d_ ## IN_DIM(__OP_ ## IN_DIM x) \
{ \
    __OP_ ## IN_DIM deriv \
        = __gxnoise_deriv_H_ ## IN_DIM (x, __SEED_1(IN_DIM)); \
    return (float2)(deriv.y, -deriv.x); \
}

/// Creates the code for the sx_curl_noise_X_3() function.
#define CREATE_CURL_NOISE_FUNC3(IN_DIM) \
float3 VEXcurlgxnoise_ ## IN_DIM(__OP_ ## IN_DIM x) \
{ \
    uint3 seed = __SEED_3(IN_DIM); \
    __NOISE_FUNC_START(IN_DIM) \
    __NOISE_DERIV_COMPUTE3(IN_DIM) \
    return (float3)(deriv3.y - deriv2.z, \
                    deriv1.z - deriv3.x, \
                    deriv2.x - deriv1.y) \
        * (__AMP_ ## IN_DIM * __FREQ_ADJ); \
}

/******************************************************************************
 * Function definitions.
 ******************************************************************************/

/// Actually creates the 12 noise functions
///     floatO VEXgxnoise_I_O(floatI x)
/// for O in {1, 2, 3, 4}, and I in {2, 3, 4} (float1 is float).
CREATE_NOISE_FOR_ALL_OUTPUTS(2)
CREATE_NOISE_FOR_ALL_OUTPUTS(3)
CREATE_NOISE_FOR_ALL_OUTPUTS(4)

/// Actually creates the functions
///     float2 VEXcurlgxnoise2d_I(floatI x)
/// for I in {2, 3, 4}.
CREATE_CURL_NOISE_FUNC2(2)
CREATE_CURL_NOISE_FUNC2(3)
CREATE_CURL_NOISE_FUNC2(4)

/// Actually creates the functions
///     float3 VEXcurlgxnoise_I(floatI x)
/// for I in {3, 4}.
CREATE_CURL_NOISE_FUNC3(3)
CREATE_CURL_NOISE_FUNC3(4)

#endif

