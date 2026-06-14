#ifndef __IMX_FILTER_H__
#define __IMX_FILTER_H__

// Only a single filter can be used in a snippet. To choose filter:
//
// Either define one of the following:
//  FILTER_POINT
//  FILTER_BILINEAR
//  FILTER_BOX (floating-point width box, sometimes called a 'tent')
//  FILTER_TRIANGLE
//  FILTER_CUBIC (bicubic interpolation if scale==1)
//  FILTER_MITCHELL (non-interpolating cubic)
//  FILTER_BSPLINE (smooth non-interpolating cubic)
// Or define these symbols, this example is equivalent to FILTER_TRIANGLE:
//  #define FILTER_SIZE 2
//  static __constant float samples[FILTER_SIZE] = { 1, 0 };
//  #define FILTER samples
//  #define FILTER_SUPPORT 1
//
// The following @-substitutions can then be used to sample areas, for float1-4 bindings
// F @name.bufferSampleRect(float2 center, float2 size) -> area in buffer coordinates
// F @name.bufferSampleRectClip(float2 center, float2 size) -> area in buffer coordinates
// F @name.imageSampleRect(float2 center, float2 size) -> area in image coordinates
// F @name.imageSampleRectClip(float2 center, float2 size) -> area in image coordinates

#include "imx_filter_internal.h"
#endif
