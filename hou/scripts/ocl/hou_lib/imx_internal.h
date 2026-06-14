#ifndef __IMX_INTERNAL_H__
#define __IMX_INTERNAL_H__

#include <matrix.h>

typedef float                   float1;
typedef int                     int1;

/// Converts image coordinates to linear index.
static int
_linearIndex(global const IMX_Stat* restrict stat, int2 xy)
{
#ifdef CHECK_RANGE
    if (xy.x < 0 || xy.x >= stat->resolution.x ||
        xy.y < 0 || xy.y >= stat->resolution.y)
        printf("Error: converting an invalid 2D index at %v2d to linear;"
               "resolution was %v2d.\n", xy, stat->resolution);
#endif
    return xy.x * stat->stride_x + xy.y * stat->stride_y;
}

/// Splits the given coordinates into integer and fractional parts.
static void
_splitCoordinates(float2 xy, int2* ix_p, float2* fx_p)
{
    float2 f;
    *fx_p = fract(xy, &f);
    *ix_p = convert_int2_sat(f);
}

/// Wraps the given coordinates for the specified resolution.
static int2
_wrapCoordinates(int2 xy, int2 res)
{
    int2 p = xy % res;
    return select(p, p + res, p < 0);
}

static int2
_mirrorCoordinates(int2 xy, int2 res)
{
    int2 res2 = res * 2;
    int2 p = _wrapCoordinates(xy, res2);
    return select(p, res2 - p - 1, p >= res);
}

/// Mirrors the given coordinates for the specified resolution. mirrored0 and
/// mirrored1 will have the mirrored versions of x and x+1, respectively.
static void
_mirrorCoordinates2(int2 xy, int2 res, int2* mirrored0, int2* mirrored1)
{
    int2 res2 = res * 2;
    int2 p = _wrapCoordinates(xy, res2);
    *mirrored0 = select(p, res2 - p - 1, p >= res);
    p = _wrapCoordinates(xy + 1, res2);
    *mirrored1 = select(p, res2 - p - 1, p >= res);
}

static bool
_outside(int2 xy, int2 res)
{
    return any(xy < 0) || any(xy >= res);
}

#ifdef CHECK_STORAGE_TYPE_CON
__constant char *
_getStorageName(StorageType storage)
{
    switch (storage)
    {
    case INT8:
        return "INT8";
    case INT16:
        return "INT16";
    case INT32:
        return "INT32";
    case FLOAT16:
        return "FLOAT16";
    case FLOAT32:
        return "FLOAT32";
    }
}
#define _CHECK_STORAGE(what) \
    if (storage != layer->stat->storage) \
        printf("Error: %s(%s), layer is %s\n", what, \
               _getStorageName(storage), _getStorageName(layer->stat->storage));
#else
#define _CHECK_STORAGE(what)
#endif

#ifdef CHECK_CHANNEL_COUNT_CON
#define _CHECK_CHANNEL(what) \
    if (channels != layer->stat->channels) \
        printf("Error: %s(channels=%d), layer is %d channels\n", \
               what, channels, layer->stat->channels);
#else
#define _CHECK_CHANNEL(what)
#endif

#ifdef CHECK_RANGE
#define _CHECK_RANGE(what) \
    if (index < 0 || index >= stat->resolution.y * stat->stride_y) \
        printf("Error: %s index %d out of range\n", what, index);
#else
#define _CHECK_RANGE(what)
#endif

#define CHECK_STAT(what) _CHECK_STORAGE(what) _CHECK_CHANNEL(what) _CHECK_RANGE(what)

#ifdef CHECK_STORAGE_TYPE_WRITE
#define WRITE_ERROR_I() printf("Error: writing integer values to a floating point layer.\n"); return
#define WRITE_ERROR_F() printf("Error: writing floating point values to an integer layer.\n"); return
#else
#define WRITE_ERROR_I() return
#define WRITE_ERROR_F() return
#endif

#ifdef CHECK_STORAGE_TYPE_READ
#define READ_ERROR_I() printf("Error: reading integer values from a floating point layer.\n"); return layer->stat->default_i
#define READ_ERROR_F() printf("Error: reading floating point values from an integer layer.\n"); return layer->stat->default_f
#else
#define READ_ERROR_I() return layer->stat->default_i
#define READ_ERROR_F() return layer->stat->default_f
#endif

static void
_setIndexLinI1a(IMX_Layer* layer, int index, int v, StorageType storage)
{
    switch (storage)
    {
    case INT8:
        ((global char*) layer->data)[index] = (char) v;
        return;
    case INT16:
        ((global short*) layer->data)[index] = (short) v;
        return;
    case INT32:
        ((global int*) layer->data)[index] = v;
        return;
    case FLOAT16:
        vstore_half_rte((float)v, index, (global half*)layer->data);
        break;
    case FLOAT32:
        ((global float*)layer->data)[index] = v;
        break;
    default:
        WRITE_ERROR_I();
    }
}

static void
_setIndexLinF1a(IMX_Layer* layer, int index, float v, StorageType storage)
{
    switch (storage)
    {
    case FLOAT16:
        vstore_half_rte(v, index, (global half*) layer->data);
        break;
    case FLOAT32:
        ((global float*) layer->data)[index] = v;
        break;
    default:
        _setIndexLinI1a(layer, index, convert_int_sat_rtn(v+0.5f), storage);
    }
}

static void
_setIndexLinF2a(IMX_Layer* layer, int index, float2 v, StorageType storage)
{
    switch (storage)
    {
    case FLOAT16:
        vstore_half2_rte(v, index, (global half*) layer->data);
        break;
    case FLOAT32:
        vstore2(v, index, (global float*) layer->data);
        break;
    default:
        WRITE_ERROR_F();
    }
}

static void
_setIndexLinF3a(IMX_Layer* layer, int index, float3 v, StorageType storage)
{
    switch (storage)
    {
    case FLOAT16:
        vstore_half3_rte(v.xyz, index, (global half*) layer->data);
        break;
    case FLOAT32:
        vstore3(v.xyz, index, (global float*) layer->data);
        break;
    default:
        WRITE_ERROR_F();
    }
}

static void
_setIndexLinF4(IMX_Layer* layer, int index, float4 v, StorageType storage,
               int channels)
{
    CHECK_STAT("setIndexF4");
    switch (channels)
    {
    case 1:
        _setIndexLinF1a(layer, index, v.x, storage);
        break;
    case 2:
        _setIndexLinF2a(layer, index, v.xy, storage);
        break;
    case 3:
        _setIndexLinF3a(layer, index, v.xyz, storage);
        break;
    default:
        switch (storage)
        {
        case FLOAT16:
            vstore_half4_rte(v, index, (global half*) layer->data);
            break;
        case FLOAT32:
            vstore4(v, index, (global float*) layer->data);
            break;
        default:
            WRITE_ERROR_F();
        }
        break;
    }
}

static void
_setIndexLinI1(IMX_Layer* layer, int index, int v, StorageType storage,
               int channels)
{
    CHECK_STAT("setIndexI1");
    if (channels == 1)
        _setIndexLinI1a(layer, index, v, storage);
    else
        _setIndexLinF4(layer, index, v, storage, channels);
}

static void
_setIndexLinF1(IMX_Layer* layer, int index, float v, StorageType storage,
               int channels)
{
    CHECK_STAT("setIndexF1");
    if (channels == 1)
        _setIndexLinF1a(layer, index, v, storage);
    else
        _setIndexLinF4(layer, index, v, storage, channels);
}

static void
_setIndexLinF2(IMX_Layer* layer, int index, float2 v, StorageType storage,
               int channels)
{
    CHECK_STAT("setIndexF2");
    if (channels == 2)
        _setIndexLinF2a(layer, index, v, storage);
    else
        _setIndexLinF4(layer, index, (float4)(v,0.0f,1.0f), storage, channels);
}

static void
_setIndexLinF3(IMX_Layer* layer, int index, float3 v, StorageType storage,
               int channels)
{
    CHECK_STAT("setIndexF3");
    if (channels == 3)
        _setIndexLinF3a(layer, index, v, storage);
    else
        _setIndexLinF4(layer, index, (float4)(v,1.0f), storage, channels);
}

////////////////////////////////////////////////////////////////////////////////

static int
_bufferIndexLinI1(const IMX_Layer* layer, int index, StorageType storage,
                  int channels)
{
    CHECK_STAT("bufferIndexI1");

    switch (storage)
    {
    case INT8:
        return ((global char*) layer->data)[index * channels];
    case INT16:
        return ((global short*) layer->data)[index * channels];
    case INT32:
        return ((global int*) layer->data)[index * channels];
    case CONSTANT_INT8:
        return *((global char*) layer->data);
    case CONSTANT_INT16:
        return *((global short*) layer->data);
    case CONSTANT_INT32:
        return *((global int*) layer->data);
    default:
        READ_ERROR_I().x;
    }
}

static float
_bufferIndexLinF1(const IMX_Layer* layer, int index, StorageType storage,
                  int channels)
{
    CHECK_STAT("bufferIndexF1");

    switch (storage)
    {
    case FLOAT16:
        return vload_half(index*channels, (global half*) layer->data);
    case FLOAT32:
        return ((global float*) layer->data)[index*channels];
    case CONSTANT_FLOAT16:
        return vload_half(0, (global half*) layer->data);
    case CONSTANT_FLOAT32:
        return ((global float*) layer->data)[0];
    default:
        return _bufferIndexLinI1(layer, index, storage, channels);
    };
}

static float2
_bufferIndexLinF2(const IMX_Layer* layer, int index, StorageType storage,
                  int channels)
{
    CHECK_STAT("bufferIndexF2");

    switch (channels)
    {
    case 1:
        return _bufferIndexLinF1(layer, index, storage, channels);
    default:
        switch (storage)
        {
        case FLOAT16:
            return vload_half2(0, (global half*) layer->data + index * channels);
        case FLOAT32:
            return vload2(0, (global float*) layer->data + index * channels);
        case CONSTANT_FLOAT16:
            return vload_half2(0, (global half*) layer->data);
        case CONSTANT_FLOAT32:
            return vload2(0, (global float*) layer->data);
        default:
            READ_ERROR_F().xy;
        }
    }
}

static float3
_bufferIndexLinF3(const IMX_Layer* layer, int index, StorageType storage,
                  int channels)
{
    CHECK_STAT("bufferIndexF3");

    switch (channels)
    {
    case 1:
        return _bufferIndexLinF1(layer, index, storage, channels);
    case 2:
        return (float3)(_bufferIndexLinF2(layer, index, storage, channels), 0.0f);
    default:
        switch (storage)
        {
        case FLOAT16:
            return vload_half3(0, (global half*) layer->data + index * channels);
        case FLOAT32:
            return vload3(0, (global float*) layer->data + index * channels);
        case CONSTANT_FLOAT16:
            return vload_half3(0, (global half*) layer->data);
        case CONSTANT_FLOAT32:
            return vload3(0, (global float*) layer->data);
        default:
            READ_ERROR_F().xyz;
        }
    }
}

static float4
_bufferIndexLinF4(const IMX_Layer* layer, int index, StorageType storage,
                  int channels)
{
    CHECK_STAT("bufferIndexF4");

    switch (channels)
    {
    case 1:
        return _bufferIndexLinF1(layer, index, storage, channels);
    case 2:
        return (float4)(_bufferIndexLinF2(layer, index, storage, channels), 0.0f, 1.0f);
    case 3:
        return (float4)(_bufferIndexLinF3(layer, index, storage, channels), 1.0f);
    default:
        switch (storage)
        {
        case FLOAT16:
            return vload_half4(index, (global half*) layer->data);
        case FLOAT32:
            return vload4(index, (global float*) layer->data);
        case CONSTANT_FLOAT16:
            return vload_half4(0, (global half*) layer->data);
        case CONSTANT_FLOAT32:
            return vload4(0, (global float*) layer->data);
        default:
            READ_ERROR_F();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

#define _IMPLEMENT_I(DIM)                                            \
                                                                     \
static int ## DIM                                                    \
bufferIndexI ## DIM (const IMX_Layer* layer, int2 xy,                \
                     BorderType border, StorageType storage,         \
                     int channels)                                   \
{                                                                    \
    int2 res = layer->stat->resolution;                              \
    if (xy.x < 0 || xy.x >= res.x || xy.y < 0 || xy.y >= res.y)      \
    {                                                                \
        switch (border)                                              \
        {                                                            \
        case (IMX_CONSTANT):                                         \
            return 0.0f;                                             \
        case (IMX_CLAMP):                                            \
            xy = clamp(xy, (int2)(0), res - 1);                      \
            break;                                                   \
        case (IMX_MIRROR):                                           \
            xy = _mirrorCoordinates(xy, res);                        \
            break;                                                   \
        case (IMX_WRAP): default:                                    \
            xy = _wrapCoordinates(xy, res);                          \
            break;                                                   \
        }                                                            \
    }                                                                \
    return _bufferIndexLinI ## DIM (                                 \
        layer, _linearIndex(layer->stat, xy), storage, channels);    \
}                                                                    \
                                                                     \
static int ## DIM                                                    \
bufferSampleI ## DIM (const IMX_Layer* layer, float2 xy,             \
                      BorderType border, StorageType storage,        \
                      int channels)                                  \
{                                                                    \
    int2 c = convert_int2_sat_rtn(xy+0.5f);                          \
    return bufferIndexI ## DIM (layer, c, border, storage, channels);\
}                                                                    \
                                                                     \
static void                                                          \
_setIndexI ## DIM(IMX_Layer *layer, int2 xy, int ## DIM val, StorageType storage, int channels)   \
{                                                                    \
    int2 res = layer->stat->resolution;                              \
    if (xy.x < 0 || xy.x >= res.x || xy.y < 0 || xy.y >= res.y)      \
        return;                                                      \
    _setIndexLinI ## DIM(layer, _linearIndex(layer->stat, xy), val, storage, channels); \
}                                                                    \
/**/


_IMPLEMENT_I(1)
#undef _IMPLEMENT_I

#define _IMPLEMENT_F(DIM)                                            \
                                                                     \
static float ## DIM                                                  \
_bufferSampleF ## DIM ## _CN(const IMX_Layer* layer, float2 xy,      \
                             StorageType storage, int channels)      \
{                                                                    \
    int2 ix;                                                         \
    float2 fx;                                                       \
    _splitCoordinates(xy, &ix, &fx);                                 \
    global const IMX_Stat* restrict stat = layer->stat;              \
    int2 res = stat->resolution;                                     \
    float ## DIM k = 0.0f;                                           \
    float ## DIM v00 = _outside(ix, res) ? k : _bufferIndexLinF ## DIM ( \
        layer, _linearIndex(stat, ix), storage, channels);           \
    ix.x++;                                                          \
    float ## DIM v10 = _outside(ix, res) ? k : _bufferIndexLinF ## DIM ( \
        layer, _linearIndex(stat, ix), storage, channels);           \
    ix.y++;                                                          \
    float ## DIM v11 = _outside(ix, res) ? k : _bufferIndexLinF ## DIM ( \
        layer, _linearIndex(stat, ix), storage, channels);           \
    ix.x--;                                                          \
    float ## DIM v01 = _outside(ix, res) ? k : _bufferIndexLinF ## DIM ( \
        layer, _linearIndex(stat, ix), storage, channels);           \
    return mix(mix(v00, v10, fx.x), mix(v01, v11, fx.x), fx.y);      \
}                                                                    \
                                                                     \
static float ## DIM                                                  \
_bufferSampleF ## DIM ## _CL(const IMX_Layer* layer, float2 xy,      \
                             StorageType storage, int channels)      \
{                                                                    \
    int2 ix;                                                         \
    float2 fx;                                                       \
    _splitCoordinates(xy, &ix, &fx);                                 \
    global const IMX_Stat* restrict stat = layer->stat;              \
    int2 res1 = stat->resolution - 1;                                \
    int2 ix0 = clamp(ix, (int2)(0), res1);                           \
    int2 ix1 = clamp(ix+1, (int2)(0), res1);                         \
    float ## DIM v00 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, ix0), storage, channels);          \
    float ## DIM v10 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, (int2)(ix1.x, ix0.y)), storage, channels); \
    float ## DIM v01 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, (int2)(ix0.x, ix1.y)), storage, channels); \
    float ## DIM v11 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, ix1), storage, channels);          \
    return mix(mix(v00, v10, fx.x), mix(v01, v11, fx.x), fx.y);      \
}                                                                    \
                                                                     \
static float ## DIM                                                  \
_bufferSampleF ## DIM ## _MR(const IMX_Layer* layer, float2 xy,      \
                             StorageType storage, int channels)      \
{                                                                    \
    int2 ix;                                                         \
    float2 fx;                                                       \
    _splitCoordinates(xy, &ix, &fx);                                 \
    global const IMX_Stat* restrict stat = layer->stat;              \
    int2 ix0, ix1;                                                   \
    _mirrorCoordinates2(ix, stat->resolution, &ix0, &ix1);           \
    float ## DIM v00 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, ix0), storage, channels);          \
    float ## DIM v10 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, (int2)(ix1.x, ix0.y)), storage, channels); \
    float ## DIM v01 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, (int2)(ix0.x, ix1.y)), storage, channels); \
    float ## DIM v11 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, ix1), storage, channels);          \
    return mix(mix(v00, v10, fx.x), mix(v01, v11, fx.x), fx.y);      \
}                                                                    \
                                                                     \
static float ## DIM                                                  \
_bufferSampleF ## DIM ## _WR(const IMX_Layer* layer, float2 xy,      \
                             StorageType storage, int channels)      \
{                                                                    \
    int2 ix;                                                         \
    float2 fx;                                                       \
    _splitCoordinates(xy, &ix, &fx);                                 \
    global const IMX_Stat* restrict stat = layer->stat;              \
    int2 ix0 = _wrapCoordinates(ix, stat->resolution);               \
    int2 ix1 = _wrapCoordinates(ix + 1, stat->resolution);           \
    float ## DIM v00 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, ix0), storage, channels);          \
    float ## DIM v10 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, (int2)(ix1.x, ix0.y)), storage, channels); \
    float ## DIM v01 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, (int2)(ix0.x, ix1.y)), storage, channels); \
    float ## DIM v11 = _bufferIndexLinF ## DIM (                     \
        layer, _linearIndex(stat, ix1), storage, channels);          \
    return mix(mix(v00, v10, fx.x), mix(v01, v11, fx.x), fx.y);      \
}                                                                    \
                                                                     \
static float ## DIM                                                  \
bufferSampleF ## DIM (const IMX_Layer* layer, float2 xy,             \
                     BorderType border, StorageType storage,         \
                     int channels)                                   \
{                                                                    \
    switch (border)                                                  \
    {                                                                \
    case (IMX_CONSTANT):                                             \
        return _bufferSampleF ## DIM ## _CN(                         \
            layer, xy, storage, channels);                           \
    case (IMX_CLAMP):                                                \
        return _bufferSampleF ## DIM ## _CL(                         \
            layer, xy, storage, channels);                           \
    case (IMX_MIRROR):                                               \
        return _bufferSampleF ## DIM ## _MR(                         \
            layer, xy, storage, channels);                           \
    case (IMX_WRAP): default:                                        \
        return _bufferSampleF ## DIM ## _WR(                         \
            layer, xy, storage, channels);                           \
    }                                                                \
}                                                                    \
                                                                     \
static float ## DIM                                                  \
bufferIndexF ## DIM (const IMX_Layer* layer, int2 xy,                \
                     BorderType border, StorageType storage,         \
                     int channels)                                   \
{                                                                    \
    int2 res = layer->stat->resolution;                              \
    if (xy.x < 0 || xy.x >= res.x || xy.y < 0 || xy.y >= res.y)      \
    {                                                                \
        switch (border)                                              \
        {                                                            \
        case (IMX_CONSTANT):                                         \
            return 0.0f;                                             \
        case (IMX_CLAMP):                                            \
            xy = clamp(xy, (int2)(0), res - 1);                      \
            break;                                                   \
        case (IMX_MIRROR):                                           \
            xy = _mirrorCoordinates(xy, res);                        \
            break;                                                   \
        case (IMX_WRAP): default:                                    \
            xy = _wrapCoordinates(xy, res);                          \
            break;                                                   \
        }                                                            \
    }                                                                \
    return _bufferIndexLinF ## DIM (                                 \
        layer, _linearIndex(layer->stat, xy), storage, channels);    \
}                                                                    \
                                                                     \
static void                                                          \
_setIndexF ## DIM(IMX_Layer *layer, int2 xy, float ## DIM val, StorageType storage, int channels)   \
{                                                                    \
    int2 res = layer->stat->resolution;                              \
    if (xy.x < 0 || xy.x >= res.x || xy.y < 0 || xy.y >= res.y)      \
        return;                                                      \
    _setIndexLinF ## DIM(layer, _linearIndex(layer->stat, xy), val, storage, channels); \
}                                                                    \
/**/

_IMPLEMENT_F(1)
_IMPLEMENT_F(2)
_IMPLEMENT_F(3)
_IMPLEMENT_F(4)
#undef _IMPLEMENT_F

static float2
applySTXform(STXform xform, float2 v)
{
    return v * xform.lo + xform.hi;
}

static float2
applySTXformInverse(STXform xform, float2 v)
{
    return (v - xform.hi) / xform.lo;
}

static float2
bufferToImage(global const IMX_Stat* restrict stat, float2 v)
{
    return applySTXform(stat->buffer_to_image, v);
}

static float2
imageToBuffer(global const IMX_Stat* restrict stat, float2 v)
{
    return applySTXform(stat->image_to_buffer, v);
}

static float2
bufferToPixel(global const IMX_Stat* restrict stat, float2 v)
{
    return applySTXform(stat->buffer_to_pixel, v);
}

static float2
pixelToBuffer(global const IMX_Stat* restrict stat, float2 v)
{
    return applySTXformInverse(stat->buffer_to_pixel, v);
}

static float2
bufferToTexture(global const IMX_Stat* restrict stat, float2 v)
{
    return (v + 0.5f) / (float2)(stat->resolution.x, stat->resolution.y);
}

static float2
textureToBuffer(global const IMX_Stat* restrict stat, float2 v)
{
    return v * ((float2)(stat->resolution.x, stat->resolution.y)) - 0.5f;
}

static float3
imageToWorld(global const IMX_Stat* restrict stat, float2 xy)
{
    float4      xyzw = 0;
    xyzw.xy = xy;
    xyzw.w = 1;
    xyzw = mat4vecmul(stat->image_to_world, xyzw);

    xyzw.xyz /= xyzw.w;
    return xyzw.xyz;
}

static float3
image3ToWorld(global const IMX_Stat* restrict stat, float3 xyz)
{
    float4      xyzw = 0;
    xyzw.xyz = xyz;
    xyzw.w = 1;
    xyzw = mat4vecmul(stat->image_to_world, xyzw);

    xyzw.xyz /= xyzw.w;
    return xyzw.xyz;
}

static float2
worldToImage(global const IMX_Stat* restrict stat, float3 xyz)
{
    float4      xyzw = 0;
    xyzw.xyz = xyz;
    xyzw.w = 1;
    xyzw = mat4vecmul(stat->world_to_image, xyzw);

    xyzw.xy /= xyzw.w;
    return xyzw.xy;
}

static float3
worldToImage3(global const IMX_Stat* restrict stat, float3 xyz)
{
    float4      xyzw = 0;
    xyzw.xyz = xyz;
    xyzw.w = 1;
    xyzw = mat4vecmul(stat->world_to_image, xyzw);

    xyzw.xyz /= xyzw.w;
    return xyzw.xyz;
}

static float4
dCdxF4(const IMX_Layer* layer, float2 ixy,
       BorderType border, StorageType storage, int channels,
       const IMX_Layer* dst)
{
    float2 xy = imageToBuffer(layer->stat, ixy);
    float d = (dst->stat->buffer_to_image.x * layer->stat->image_to_buffer.x) / 2;
    float4 a = bufferSampleF4(layer, (float2)(xy.x + d, xy.y), border, storage, channels);
    float4 b = bufferSampleF4(layer, (float2)(xy.x - d, xy.y), border, storage, channels);
    return a - b;
}

static float4
dCdxF4aligned(const IMX_Layer* layer, int2 xy,
              BorderType border, StorageType storage, int channels)
{
    float4 a = bufferIndexF4(layer, (int2)(xy.x+1, xy.y), border, storage, channels);
    float4 b = bufferIndexF4(layer, (int2)(xy.x-1, xy.y), border, storage, channels);
    return (a-b) * 0.5f;
}

static float4
dCdyF4(const IMX_Layer* layer, float2 ixy,
       BorderType border, StorageType storage, int channels,
       const IMX_Layer* dst)
{
    float2 xy = imageToBuffer(layer->stat, ixy);
    float d = (dst->stat->buffer_to_image.y * layer->stat->image_to_buffer.y) / 2;
    float4 a = bufferSampleF4(layer, (float2)(xy.x, xy.y + d), border, storage, channels);
    float4 b = bufferSampleF4(layer, (float2)(xy.x, xy.y - d), border, storage, channels);
    return a - b;
}

static float4
dCdyF4aligned(const IMX_Layer* layer, int2 xy,
              BorderType border, StorageType storage, int channels)
{
    float4 a = bufferIndexF4(layer, (int2)(xy.x, xy.y+1), border, storage, channels);
    float4 b = bufferIndexF4(layer, (int2)(xy.x, xy.y-1), border, storage, channels);
    return (a-b) * 0.5f;
}

#endif

