#bind layer src? val=0

#bind layer !&dist
#bind layer !&id
#bind layer !&uv

#bind parm block_ctr float2
#bind parm repeats_ctr float2
#bind parm thick_ctr float

#bind parm block_sde float2
#bind parm repeats_sde float2
#bind parm thick_sde float

#bind parm seed int

#include "$HIP/scripts/ocl/include/sd_shapes_2d.cl"
#include "$HIP/scripts/ocl/include/sdf_operations.cl"

#include "$HIP/scripts/ocl/include/hashs.cl"
#include "$HIP/scripts/ocl/include/utils.cl"
#include "matrix.h"

typedef struct{

    float dist;
    float2 uv;
    float2 id;

}repdata_t;


#define SEED 1048485

float3 shape_ctr(float2 pos, float2 id, float2 s, float thick, int seed){

    float d = 1e9;

    float idy = mod(id.x, 2.0f);
    s.y += idy * 0.15;

    d = sdRoundedRectangle(pos, 1.0, s-(float2)(thick), 0.0f);

    return (float3)(d,pos);
}

float3 shape_sde(float2 pos, float2 id, float2 s, float thick, int seed){

    float d = 1e9;

    float hsh = (float)( pcg2d(as_uint2(id + 10.0f)).x ) / (float)( 0xffffffffU );
    hsh = hsh * 2.0 - 1.0;

    pos = rotate2D(pos,hsh*0.1);
    hsh = lowbias32( as_uint( (int)(id.x*10+seed) ) ) / (float)(0xffffffffu);


    float o = mod(id.x, 2.0f) * 0.125*0.5;
    o += hsh * -0.05;

    pos.y += hsh*0.05;
    pos.y += o;


    d = sdRoundedRectangle(pos, 1.0, s-(float2)(thick), 0.000f);



    return (float3)(d,pos);
}

repdata_t repBlock( float2 p, float2 s, float2 rep, float thick, int seed, int isctr ){

    repdata_t data;
    data.dist = 1e20f;

    float2 id = round(p/s);
    float2 off = sign(p-s*id);

    for( int j=0; j<2; j++ )
    for( int i=0; i<2; i++ )
    {
        float2 rid = id + (float2)((float)i,(float)j)*off;
        rid = clamp(rid,-rep,rep); // limited repetition
        float2 r = p - s*rid;

        float3 res = isctr > 0.5 ? shape_ctr(r,rid, s, thick, seed) : shape_sde(r,rid, s, thick, seed);

        if(res.x < data.dist)
        {
            data.dist = res.x;
            data.uv = res.yz;
            data.id = rid;
        }
    }

    return data;
}


@KERNEL{

    float2 pos = @P;

    float2 blocks = @block_ctr;
    float2 repeats = @repeats_ctr;
    float thick = @thick_ctr;

    float d = 1e15;
    float2 uv = 0.0f;
    float2 id = 0.0;

    pos.y -= 0.0335f;

    pos.x += 0.07f;

    float ctr = fabs(pos.y) - 0.002;

    
    repdata_t rep = repBlock(pos, blocks, repeats, thick, @seed, 1);
    
    d = rep.dist;
    uv = rep.uv;
    id = rep.id;

    //---------------------------------------------------------
    blocks = @block_sde;
    repeats = @repeats_sde;
    thick = @thick_sde;

    rep = repBlock(pos, blocks, repeats, thick, @seed, 0);

    if(d > (thick+0.00025)){
        d = rep.dist;
        uv = rep.uv;
        id = rep.id;
    }


    @dist.set(d);
    @uv.set((float4)(uv,.0f,.0f));
    @id.set((float4)(id,.0f,.0f));
}








    // if(d*100.0 > rep.dist){
    //     d = rep.dist;
    //     uv = rep.uv;
    //     id = rep.id;
    // }


// pos.x += 1.05;
    // repdata_t rep = repBlock(pos, blocks, repeats_ctr, thick_ctr, @seed);
    // d = -rep.dist;
    // d = sin(d*1000.0)*0.5+0.5;
