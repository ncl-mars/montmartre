#include "matrix.h"
#include "./utils.cl"

float2 __attribute__((overloadable)) posToUV(float2 pos){

    pos = rotate2D(pos, 90) + 0.5f;             // x fwd, to tex space [0,1]
    pos.x = 1.0-pos.x;
    return pos;
}

float2 __attribute__((overloadable)) posToUV(float2 pos, float scale){

    pos /= scale;                               // to [-0.5,0.5]
    pos = rotate2D(pos, 90) + 0.5f;             // x fwd, to tex space [0,1]
    pos.x = 1.0-pos.x;
    return pos;
}


float2 __attribute__((overloadable)) posToPix(float2 pos, float2 res){

    pos = posToUV(pos);
    pos = clamp(pos, 0.0f, 1.0f);

    pos *= res;    // to index space
    return pos;
}

float2 __attribute__((overloadable)) posToPix(float2 pos, float scale, float2 res){

    pos = posToUV(pos, scale);
    pos = clamp(pos, 0.0f, 1.0f);

    pos *= res;    // to index space
    return pos;
}




int __attribute__((overloadable)) gidToLinearId(int2 gid, int2 res){

    return gid.x + gid.y*res.x;
}


// improved bilinear interpolated tex2D fetch, NO MIP MAP
float fetchClampBilinear( __global const float *tex, float2 uv, float2 res ){

    int2 ires = convert_int2(res.xy);
    float2  fres = res.xy;

    float2 st = (frac(uv)-0.5f/fres)*fres;
    int2 i = convert_int2( floor( st ) );
    float2  w = frac( st );

    // float a = tex[ clamp(i+(int2)(0,0), (int2)0, ires-1) ];
    // float b = tex[ clamp(i+(int2)(1,0), (int2)0, ires-1) ];
    // float c = tex[ clamp(i+(int2)(0,1), (int2)0, ires-1) ];
    // float d = tex[ clamp(i+(int2)(1,1), (int2)0, ires-1) ];
    
    float a = tex[ gidToLinearId(clamp(i+(int2)(0,0), (int2)0, ires-1), ires) ];
    float b = tex[ gidToLinearId(clamp(i+(int2)(1,0), (int2)0, ires-1), ires) ];
    float c = tex[ gidToLinearId(clamp(i+(int2)(0,1), (int2)0, ires-1), ires) ];
    float d = tex[ gidToLinearId(clamp(i+(int2)(1,1), (int2)0, ires-1), ires) ];
    
    return lerp(lerp(a, b, w.x), lerp(c, d, w.x), w.y);
}