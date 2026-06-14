#ifndef VOL_UTILS_CL
#define VOL_UTILS_CL

#include "interpolate.h"

#ifndef VOL_DATA_T

    typedef struct{
        __global const float *data;
        int3 str;
        int str_off;
        int3 res;
        float vsize;
    }voldata_t;

    #define VOL_DATA_T voldata_t
#endif
#ifndef VOL_COORDS_T
    typedef struct{
        int idx;
        int3 gid;
        float3 pos;
    }volcoords_t;

    #define VOL_COORDS_T volcoords_t
#endif


int3 getGridId(){
    return (int3)(
        get_global_id(0),
        get_global_id(1),
        get_global_id(2)
    );
}

float3 __attribute__((overloadable)) gidToLocalPos(float3 gid, float3 res){
    return ((gid+0.5f)*2.0f - res) / max(max(res.x, res.y),res.z);
}
float3 __attribute__((overloadable)) gidToLocalPos(int3 gid, int3 res){
    return gidToLocalPos(convert_float3(gid),convert_float3(res));
}
float3 __attribute__((overloadable)) gidToLocalPos(int3 gid, float3 res){
    return gidToLocalPos(convert_float3(gid),res);
}

int __attribute__((overloadable)) gidToLinearId(int3 gid, int3 res){
    
    return gid.x + gid.y*res.x + gid.z*res.x*res.y;
}
int __attribute__((overloadable)) gidToLinearId(int offset, int stride_x, int stride_y, int stride_z, int3 gid){
    return offset   + stride_x * gid.x
                    + stride_y * gid.y
                    + stride_z * gid.z;
}
int __attribute__((overloadable)) gidToLinearId(VOL_DATA_T vol, int3 gid){
    return vol.str_off  + vol.str.x * gid.x
                        + vol.str.y * gid.y
                        + vol.str.z * gid.z;
}

float3 __attribute__((overloadable)) gidToUvw(float3 gid, float3 res){
    return gid/max(res-(float3)(1.0),1.0f);
}
float3 __attribute__((overloadable)) gidToUvw(int3 gid, int3 res){
    return gidToUvw(convert_float3(gid), convert_float3(res));
}

VOL_COORDS_T getVolCoords(VOL_DATA_T vol){
    VOL_COORDS_T crd;
    crd.gid = getGridId();
    crd.idx = gidToLinearId(vol, crd.gid);
    crd.pos = gidToLocalPos(crd.gid, vol.res);
    return crd;
}

float3 __attribute__((overloadable)) getVolSize(VOL_DATA_T vol){
    return convert_float3(vol.res) * (float3)vol.vsize;
}

float __attribute__((overloadable)) sampleVolBilinear(VOL_DATA_T vol, float2 gpos){

    return bilinear_interp_vol(
        gpos, 
        vol.data,
        vol.str_off,
        vol.str.x, 
        vol.str.y,
        vol.res.x,
        vol.res.y
    );
}
float __attribute__((overloadable)) sampleVolBilinear(VOL_DATA_T vol, int2 gid){
    return sampleVolBilinear(vol, convert_float2(gid)+(float2)(0.5f));
}

float __attribute__((overloadable)) sampleVolTrilinear(VOL_DATA_T vol, float3 gpos){
    
    return trilinear_interp_vol(
        gpos,
        vol.data,
        vol.str_off,
        vol.str.x, 
        vol.str.y,
        vol.str.z,
        vol.res.x,
        vol.res.y,
        vol.res.z
    );
}
float __attribute__((overloadable)) sampleVolTrilinear(VOL_DATA_T vol, int3 gid){
    return sampleVolTrilinear(vol, convert_float3(gid)+(float3)(0.5f));
}

float __attribute__((overloadable)) blurVol(int iter, float radius, VOL_COORDS_T crd, VOL_DATA_T vol){

    float res = vol.data[crd.idx];
    float2 uv = crd.pos.xy * 0.5f + 0.5f;
    float size = radius/(vol.vsize*vol.res.x);

    if (iter > 0){
        iter *= 2;
        for(int j = 0; j<iter; j++){
        for(int i = 0; i<iter; i++){

            float2 o = (float2)(i,j)/(float2)(0.5*(iter-1)) - 1.0f;
            o *= size;

            int2 gid = convert_int2((uv + o)*(float2)vol.res.x);
            gid = clamp(gid, (int2)(0), vol.res.xy-(int2)1);

            float d = vol.data[gidToLinearId(vol, (int3)(gid.x,gid.y,0))];
            res += d;
        }}
        res /= iter*iter +1;
    }
    return res;
}

float __attribute__((overloadable)) blurVol2(int iter, float radius, int2 gid, int2 res){

    // float2 uv = crd.pos.xy * 0.5f + 0.5f;
    // float res = vol.data[crd.idx];
    // float size = radius/(vol.vsize*vol.res.x);

    return 0.0;
    // if (iter > 0){
    //     iter *= 2;
    //     for(int j = 0; j<iter; j++){
    //     for(int i = 0; i<iter; i++){

    //         float2 o = (float2)(i,j)/(float2)(0.5*(iter-1)) - 1.0f;
    //         o *= size;

    //         int2 gid = convert_int2((uv + o)*(float2)vol.res.x);
    //         gid = clamp(gid, (int2)(0), vol.res.xy-(int2)1);

    //         float d = vol.data[gidToLinearId(vol, (int3)(gid.x,gid.y,0))];
    //         res += d;
    //     }}
    //     res /= iter*iter +1;
    // }
    // return res;
}


// ------ DEPRECIATED !! TO  BE REMOVED
    // float3 __attribute__((overloadable)) toLocalPos(int3 gid, int3 res){
    //     return gidToLocalPos(convert_float3(gid),convert_float3(res));
    // }
    // float3 __attribute__((overloadable)) toLocalPos(float3 gid, float3 res){
    //     return gidToLocalPos(gid,res);
    // }

    // int __attribute__((overloadable)) toLinearIndex(int3 gid, int3 res){
        
    //     return gid.x + gid.y*res.x + gid.z*res.x*res.y;
    // }

    // int __attribute__((overloadable)) toLinearIndex(int offset, int stride_x, int stride_y, int stride_z, int3 gid){
    //     return offset   + stride_x * gid.x
    //                     + stride_y * gid.y
    //                     + stride_z * gid.z;
    // }

    // int __attribute__((overloadable)) toLinearIndex(VOL_DATA_T vol, int3 gid){
    //     return vol.str_off  + vol.str.x * gid.x
    //                         + vol.str.y * gid.y
    //                         + vol.str.z * gid.z;
    // }

    // int3 getVoxCoord(){
    //     return (int3)(
    //         get_global_id(0),
    //         get_global_id(1),
    //         get_global_id(2)
    //     );
    // }
// -------


#endif


