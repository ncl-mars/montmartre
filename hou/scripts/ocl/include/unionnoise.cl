
/*  -- NOTES
    --------------------------------------------------------------- random.h
    float3 jitter = VEXrand_4_3(p1, p2, p3, time);
    // Map the random distribution to the hat.
    jitter.x = toHat(jitter.x);
    jitter.y = toHat(jitter.y);
    jitter.z = toHat(jitter.z);

    --------------------------------------------------------------- gx simplex noise
    floatO VEXgxnoise_I_O(floatI x)
    -> for O in {1, 2, 3, 4}, and I in {2, 3, 4} (float1 is float).

    --------------------------------------------------------------- Curl noise
    float2 VEXcurlgxnoise2d_I(floatI x)
    -> for I in {2, 3, 4}.

    float3 VEXcurlgxnoise_I(floatI x) 
    -> for I in {3, 4}.
*/
#include "interpolate.h" 
#include "CVDBUtil.h"
#include "gxnoise.h"
#include "matrix.h"

#include "$HOME/libs/ocl/include/noises.cl"
#include "$HOME/libs/ocl/include/sdf_operations.cl" // includes separately from sdf.h(since it's somewhat long to compile)


typedef struct{
    cnanovdb_readaccessor acc;
    const global cnanovdb_griddata * grid;
    // cnanovdb_coord coord;

} cvdb;

typedef struct{
    float no_scale;
    float3 no_off;
    float no_dist;
} pmap;


// cubic polynomial
float2 smin( float a, float b, float k ) {
    float h = 1.0f - min( fabs(a-b)/(6.0f*k), 1.0f);
    float w = h*h*h;
    float m = w*0.5;
    float s = w*k; 
    return (a<b) ? (float2)(a-s,m) : (float2)(b-s,1.0-m);
}

float fbm(float3 x, int octaves){
    mat3 M3  = {{0.00, 0.80, 0.60}, {-0.80,0.36,-0.48}, {-0.60,-0.48,0.64}};
    mat3 M3i = {{0.00,-0.80,-0.60}, { 0.80,0.36,-0.48}, { 0.60,-0.48,0.64}};

    mat3 m; mat3identity(m);

    float f = 1.95;  // could be 2.0
    float s = 0.49;  // could be 0.5
    float b = 0.5;  // decreasing scale

    float a = 0.0;  // noise
    float3 d = 0.0; // derivative

    for( int i=0; i < octaves; i++ )
    { 
        float n;
        // n = sNoise(x);
        // n = VEXcurlgxnoise_3(x).x+0.6;
        // n = VEXgxnoise_3_1(x)+0.0;

        // float3 cu = VEXcurlgxnoise_3(x);
        // n = (cu.x + cu.y + cu.z)/3.0 ;
        // n = cu.x * cu.y * cu.z * 3.0 ;
        // n = cu.x;


        // n = voronoiDistance(x) + 0.35;
        n = voronoi(x,0.1).x;


        a += b*n; // accu values
        // d += mul(m, b*n.yzw); // accumulate derivatives
        
        b *= s;
        x = mat3vecmul(M3, (float3)f*x);
        mat3scale(m,m,f);
        mat3mul(M3i,m, m);
    }
    return a;
}

float3 fbm4_3( float3 p){

    return (float3)(
        fbm(p,4),
        fbm(p+(float3)(7.8,15.2,-2.9),4),
        fbm(p+(float3)( 16.8,-12.3,85.1),4)
    );
}

float3 fbm6_3(float3 p){

    return (float3)(
        fbm(p+(float3)(16.8,32.0,14.5),6), 
        fbm(p+(float3)(11.5,26.35,-43.2),6),
        fbm(p+(float3)(22.4,33.9,29.1),6)
    ); 
}

float warpFunc( float3 q ){
    // q += 0.03*sin( float3(0.27,0.23)*iTime + length(q)*float3(4.1,4.3));
        float3 o = fbm4_3((float3)0.9*q )*2.0f-(float3)1.0;
    // o += 0.04*sin( float3(0.12,0.14)*iTime + length(o));

    float3 n = fbm6_3((float3)3.0*o )*2.0f-(float3)1.0;
    float f = 0.5f + 0.5f * fbm((float3)1.8*q + (float3)6.0*n, 4);

    // return f;

    return mix( f, f*f*f*3.5f, f*fabs(n.x) );
}

//-----------------------------------------------------------------
float2 map(cvdb vdb, pmap par, float3 pos){


    float d = 0;

    float sd_src = cnanovdb_sampleF(vdb.grid, &vdb.acc, pos)-0.015; // off param !

    float f = 0.075  ; //base = 0.015
    d = sd_src - f;

    float no = 1.0;
    float3 q = pos * par.no_scale;
    // float3 q = pos * par.no_scale * (float3)0.5;

    no = fbm(q + par.no_off, 9) - par.no_dist;

    d -= (no<0.0 )*4.5f;
    d = sdfOpUnionSmooth(d, no , 0.0);

    no -= f;

    float b = 0.055;

    d += (no>-b)*f;
    d = sdfOpUnionSmooth(d, sd_src, 0.0001);


    return (float2)(d, no);
}

float3 calcNormal(cvdb vdb, pmap par, float3 pos, float eps){

    float2 e = (float2)(1.0,-1.0)*(float2)(0.5773*eps);
    return normalize( e.xyy*map( vdb, par, pos + e.xyy ).x+ 
                                          e.yyx*map( vdb, par, pos + e.yyx ).x+ 
                                          e.yxy*map( vdb, par, pos + e.yxy ).x+ 
                                          e.xxx*map( vdb, par, pos + e.xxx ).x);
} 

//-----------------------------------------------------------------
kernel void unionNoise( 

    global const int * restrict tilestartsxyz,
    global void * restrict sdf,
    // global void * restrict col,
    global void * restrict mask,

    float no_scale,
    float3 no_off,
    float no_dist
)
{
    pmap params = {
        .no_scale = no_scale, 
        .no_off = no_off,
        .no_dist = no_dist
    };

    // store accessors in struct
    cvdb sdf_vdb;
    cnanovdb_getreadaccessor(sdf, &sdf_vdb.acc);
    sdf_vdb.grid = (const global cnanovdb_griddata *) sdf;

    cnanovdb_readaccessor mask_acc;
    cnanovdb_getreadaccessor(mask, &mask_acc);
    
    // cnanovdb_readaccessor col_acc;
    // cnanovdb_getreadaccessor(col, &col_acc);

    int tileidx = get_global_id(0);
    int tileoffset = tileidx & 511;
    tileidx = tileidx >> 9;
    int3 tilebasexyz = vload3(tileidx, tilestartsxyz);
    int gidx = tilebasexyz.x + (tileoffset & 7);
    int gidy = tilebasexyz.y + ((tileoffset >> 3) & 7);
    int gidz = tilebasexyz.z + ((tileoffset >> 6) & 7);
    int idx = (tileidx << 9) + tileoffset;
    cnanovdb_coord gcoord = { .mVec = { gidx, gidy, gidz } };

    if (!cnanovdb_readaccessor_isActiveF(&sdf_vdb.acc, &gcoord)) return;

    float3 pos = cnanovdb_indexToWorld(sdf_vdb.grid, (float3)(gidx, gidy, gidz)+(float3)0.5);

    // float sd_src = cnanovdb_sampleF(sdf_vdb.grid, &sdf_vdb.acc, pos)-0.005; // off param !
    float2 res = 1e6;
    res = map(sdf_vdb, params, pos);

    // sd -= 0.15;
    // float3 cd = (float3)(0.51, 0.5,0.0);
    // cd = calcNormal(sdf_vdb, params, pos, 0.001 )*-1;

    cnanovdb_readaccessor_setValueF(&sdf_vdb.acc, &gcoord, res.x);
    cnanovdb_readaccessor_setValueF(&mask_acc, &gcoord, res.y);
    // cnanovdb_readaccessor_setValueF3(&col_acc, &gcoord, cnanovdb_float3_to_Vec3F(cd));
} 

