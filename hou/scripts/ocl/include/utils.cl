#ifndef UTILS_CL
#define UTILS_CL


#define mod(x, y) (x - y * floor(x / y)) // glsl style mod

float  __attribute__((overloadable)) frac(float x){  return x - floor(x); }
float2 __attribute__((overloadable)) frac(float2 x){ return x - floor(x); }
float3 __attribute__((overloadable)) frac(float3 x){ return x - floor(x); }
float4 __attribute__((overloadable)) frac(float4 x){ return x - floor(x); }


float __attribute__((overloadable)) maxComp(float2 v){ return max(v.x, v.y); }
float __attribute__((overloadable)) maxComp(float3 v){ return max(max(v.x, v.y), v.z); }

float __attribute__((overloadable)) minComp(float2 v){ return min(v.x, v.y); }
float __attribute__((overloadable)) minComp(float3 v){ return min(min(v.x, v.y), v.z); }


float __attribute__((overloadable)) dot2(float2 v ) { return dot(v,v); }
float __attribute__((overloadable)) dot2(float3 v ) { return dot(v,v); }

#ifndef __SDF_H__ // houdini's hou_lib/sdf.h already defines ndot
float __attribute__((overloadable)) ndot(float2 a, float2 b ) { return a.x*b.x - a.y*b.y; }
#endif


inline float3 __attribute__((overloadable))  limit(float3 v, float max_len) {
    float sp    = length(v);
    float scale = fmin(1.0f, max_len / (sp + 1e-12f));
    return v * scale;
}

#endif