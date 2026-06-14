#ifndef __IMX_H__
#define __IMX_H__

/******************************************************************************
 * VERBOSITY OPTIONS
 ******************************************************************************/
 
// By defining certain symbols before including this header, different error
// reporting options can be enabled. These options are listed in the table
// below.
//      SYMBOL                  ||          EFFECT
//  CHECK_RANGE                 ||  Report out-of-range indexing of buffers
//  CHECK_STORAGE_TYPE_READ     ||  Reports attempts to read floating point
//                              ||  values from integer layers or integer values
//                              ||  from floating point layers.
//  CHECK_STORAGE_TYPE_WRITE    ||  Reports attempts to write floating point
//                              ||  values to integer layers or integer values
//                              ||  to floating point layers.
//  CHECK_STORAGE_TYPE_CON      ||  Report if IMX_Layer storage type is not equal
//                              ||  to the (compile-time constant) storage argument
//  CHECK_CHANNEL_COUNT_CON     ||  Report if IMX_Layer channels is not equal
//                              ||  to the (compile-time constant) channels argument
// Note that the checks are done only when the respective symbol is defined, so
// these should only be used for validation and debugging purposes.

/******************************************************************************
 * STRUCTURES
 ******************************************************************************/

// Scale-translate transform.
typedef float4 STXform;

// Border type for a layer.
typedef enum
{
    IMX_CONSTANT,
    IMX_CLAMP,
    IMX_MIRROR,
    IMX_WRAP
} BorderType;

typedef enum
{
    IMX_TYPEINFO_NONE,
    IMX_TYPEINFO_COLOR,
    IMX_TYPEINFO_POSITION,
    IMX_TYPEINFO_VECTOR,
    IMX_TYPEINFO_NORMAL,
    IMX_TYPEINFO_OFFSETNORMAL,
    IMX_TYPEINFO_TEXTURE_COORD,
    IMX_TYPEINFO_ID,
    IMX_TYPEINFO_MASK,
    IMX_TYPEINFO_SDF,
    IMX_TYPEINFO_HEIGHT
} TypeInfoType;

// Type of data stored in a layer.
typedef enum
{
    INT8,
    INT16,
    INT32,
    FLOAT16,
    FLOAT32,
    // dummy values used to indicate IMX_Buffer::isConstant():
    CONSTANT_INT8,
    CONSTANT_INT16,
    CONSTANT_INT32,
    CONSTANT_FLOAT16,
    CONSTANT_FLOAT32
} StorageType;

// A structure containing metadata for a layer.
typedef struct
{
    float16                     image_to_world;
    float16                     world_to_image;

    STXform                     buffer_to_image;
    STXform                     image_to_buffer;
    STXform                     buffer_to_pixel;

    float4                      default_f;
    int4                        default_i;

    int2                        resolution;
    int                         channels;
    int                         stride_x, stride_y;

    BorderType                  border;
    // Typeinfo is a hint and should only affect computation in the rarest
    // of situations.
    TypeInfoType                typeinfo;
    StorageType                 storage;
} IMX_Stat;

// A structure encapsulating a layer.
typedef struct
{
    global void* restrict       data;
    global IMX_Stat* restrict   stat;
} IMX_Layer;

/// multiply v by scale+translate xform
static float2 applySTXform(STXform, float2 v);
static float2 applySTXformInverse(STXform, float2 v);

/// Space transfomrations;
static float2 bufferToImage(global const IMX_Stat* restrict, float2 xy);
static float2 imageToBuffer(global const IMX_Stat* restrict, float2 xy);
static float2 bufferToPixel(global const IMX_Stat* restrict, float2 xy);
static float2 pixelToBuffer(global const IMX_Stat* restrict, float2 xy);
static float2 bufferToTexture(global const IMX_Stat* restrict, float2 xy);
static float2 textureToBuffer(global const IMX_Stat* restrict, float2 xy);

static float3 imageToWorld(global const IMX_Stat* restridct, float2 xy);
static float3 image3ToWorld(global const IMX_Stat* restridct, float3 xy);
static float2 worldToImage(global const IMX_Stat* restridct, float3 xyz);
static float3 worldToImage3(global const IMX_Stat* restridct, float3 xyz);

// The remaining functions are for implementing @ substitutions:
//
//  int @ix, @iy, @ixy                  // output buffer coordinate
//  int @xres                           // output buffer width
//  int @yres                           // output buffer height
//  int2 @res                           // output buffer (width,height)
//  int2 @tilesize                      // tile dimensions passed to CE_Snippet::execute()
// 
// Cur location: .image .pixel .texture suffixes specify space, image default
// @P supports .world as well.
//  float2 @P                           // image coordinate of output pixel
//  float2 @dPdx                        // derivative of @P per @ix
//  float2 @dPdy                        // derivative of @P per @iy
//  float2 @dPdxy                       // (@dPdx.x,@dPdy.y) rectangle for area
//                                      // sampling
//
// 'name' is replaced with the name of a layer binding:
//  void* @name.data                    // raw buffer data
//  IMX_Stat* @name.stat
//  int @name.xres                      // @name's buffer width
//  int @name.yres                      // @name's buffer height
//  float2 @name.res                    // @name's (width,height)
//  BorderType @name.border             // border type, compile-time constant
//  StorageType @name.storage           // data type, compile-time constant
//  int @name.channels                  // # of channels, compile-time constant
//  int @name.tuplesize                 // # of channels, compile-time constant
//
// Space transforms:
//  float2 @name.imageToBuffer(float2) 
//  float2 @name.bufferToImage(float2) 
//  float2 @name.pixelToBuffer(float2) 
//  float2 @name.bufferToPixel(float2) 
//  float2 @name.textureToBuffer(float2) 
//  float2 @name.bufferToTexture(float2) 
//  float3 @name.imageToWorld(float2)
//  float3 @name.image3ToWorld(float3)
//  float2 @name.worldToImage(float3)
//  float3 @name.worldToImage3(float3)
//
//  bool @name.bound                    // same as #ifdef HAS_name
//
//  T @name.bufferIndex(int2)           // value of buffer pixel, does tiling/borders
//  T @name.bufferSample(float2)        // bilinear interpolated (nearest for int)
//  T @name.imageNearest(float2)        // bufferIndex(rint(imageToBuffer(xy))
//  T @name.imageSample(float2)         // bufferSample(imageToBuffer(xy))
//  T @name.textureNearest(float2)      // bufferIndex(rint(textureToBuffer(xy))
//  T @name.textureSample(float2)       // bufferSample(textureToBuffer(xy))
//  T @name                             // @name.imageSample(@P)
//  void @name.set(T v)                 // same as @name.setIndex((int2)(@ix,@iy), v)
//  void @name.setIndex(int2, T v)      // store value of buffer pixel, no test for out of range!
//
// Where T is not int:
//  T @name.dCdx(float2)                // derivative of @name.imageSample() per @ix
//  T @name.dCdx                        // @name.dCdx(@P)
//  T @name.dCdy(float2)                // derivative of @name.imageSample() per @iy
//  T @name.dCdy                        // @name.dCdy(@P)

#include "imx_internal.h"
#endif

