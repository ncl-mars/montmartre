

#bind point &P float3
#bind point &uv_layout float2
#bind point &Cd float3

#bind point &dist_layout float
#bind point &is_clip int

#bind point &endpoint int

typedef struct{
    __global const float *data;
    int3 str;
    int str_off;
    int3 res;
    float vsize;
    float iso;

}voldata_t;

#define VOL_DATA_T voldata_t

typedef struct{
    VOL_DATA_T vol0;
    VOL_DATA_T vol1;
    int mode; // 0 = all, 1 = layout, 2 = disp
}data_t;


#include "matrix.h"
#include "$HIP/scripts/ocl/include/utils.cl"
#include "$HIP/scripts/ocl/include/lacoste.cl"
#include "$HIP/scripts/ocl/include/volumes.cl"
#include "$HIP/scripts/ocl/include/remapping.cl"
#include "$HIP/scripts/ocl/include/noises_ashima.cl"


float map(data_t data, float2 pos){

    float2 fres = (float2)(data.vol0.res.x, data.vol0.res.y);
    float2 pix = posToPix(pos, fres);

    float d0 = sampleVolBilinear(data.vol0, pix) + data.vol0.iso;
    float d1 = sampleVolBilinear(data.vol1, pix) + data.vol1.iso;

    if (data.mode == 0) return min(d0,d1);
    else if (data.mode == 1) return d0;
    else return d1;
}

float2 calcNormal(data_t data, float2 pos){

    const float eps = 0.001; // or some other value
    const float2 h = (float2)(eps,0);

    float2 nor = normalize((float2)(
        map(data, pos+h.xy) - map(data, pos-h.xy),
        map(data, pos+h.yx) - map(data, pos-h.yx))
    );

    return nor;
}

@KERNEL
{
    float3 pos = @P;
    float2 q = (float2)(pos.x, pos.z);      // img, 2d coords
    q /= @map_scale;

    VOL_DATA_T sdf0_v = {
        @sdf.data,
        (int3)(@sdf.stride_x, @sdf.stride_y, @sdf.stride_z),
        @sdf.stride_offset,
        (int3)(@sdf.xres,@sdf.yres, @sdf.zres),
        @iso_layout};

    VOL_DATA_T sdf1_v = {
        @sdf_disp.data,
        (int3)(@sdf_disp.stride_x, @sdf_disp.stride_y, @sdf_disp.stride_z),
        @sdf_disp.stride_offset,
        (int3)(@sdf_disp.xres,@sdf_disp.yres, @sdf_disp.zres),
        @iso_disp
    };

    data_t data = {sdf0_v, sdf1_v, 0};

    float d = map(data, q) * @map_scale;
    float3 col = 0.0f;
    
    float3 off = 0.0f;
    float2 nor2 = calcNormal(data, q);

    float3 nor = (float3)(nor2.x, 0.0, nor2.y);
    float3 tan = cross(nor, (float3)(0.0,1.0,0.0));



    float2 pix = posToPix(q, (float2)(@uvx.xres, @uvx.yres));
    data.mode = 1;

    nor2 = calcNormal(data, q);

    float fac = dot(nor2, (float2)(-1,0));
    fac = smoothstep(0.95f, 1.0f, fac);

    // fac = saturate(mapTo01(0.95f,1.0f,fac-0.01));
    // fac = min(fac, -fac + 1) * 2.0;

    
    float3 c = fac;
    d -= fac * 0.0004;

    
    float clip = (d < @disp_dist);
    
    float d_str = @sdf_nor.sample2d(pix) * @map_scale;
    d_str = (1.0-smoothstep(0.00075f, 0.00085f, d_str)) * 2.0 - 1.0;

    
    d_str = sign(d_str);
    if(d_str < 0) d_str *= 5.0;

    // d_str = 1.0;    
    // float3 c = d_str;
    // @Cd.set(d_str);                 
                        

    // off.y = @y_depth_endpoint * (@endpoint > 0.5f);
    off.y = @y_depth_endpoint * clip * 0.5;

    // fac = 1.0;
    
    off += nor * -0.5f * clip * @nor_str_endpoint * d_str * mix(-fabs(d), -0.001f, fac);

    // off += nor * -0.5f * clip * @nor_str_endpoint * d_str * -0.001f;
    // off += nor * -0.5f * clip * -fabs(d) * @nor_str_endpoint * d_str;


    // off += nor * -0.5f * clip * @nor_str_endpoint * d_str * -0.001f;
    // off += nor * -0.5f * clip * -fabs(d) * @nor_str_endpoint * d_str;
    pos += off;

    float2 qn = q * @no_freq;
    float2 no = (float2)( perlinAsh(qn), perlinAsh(qn+(float2)(10.5,-513.4)) );
    no *= @no_amp * smoothstep(0.0f, @no_dist, d);
    pos += (float3)(no.x, 0.0, no.y);

    q = (float2)(pos.x, pos.z);      // img, 2d coords
    q /= @map_scale;

    pix = posToPix(q, (float2)(@uvx.xres, @uvx.yres));
    float2 uv = (float2)(@uvx.sample2d(pix), @uvy.sample2d(pix));
    @uv_layout.set(uv);

    // data.mode = 0;
    // d = map(data, q) * 1 + 0.00125;
    d = map(data, q);

    col = 1.0f;

    @P.set(pos);
    @dist_layout.set(d);

    return;
}






// ____________________________________________________________________
    // col = (float3)(nor.x, 0.0, nor.y);    
    // float d = sampleVolBilinear(sdf0_v, pix);
    // float3 col = 0.0f;
    // float3 off = 0.0f;
    // const float range = 0.001;

    // float d_dist = @sdf_disp.sample2d(q) + 0.001;
    // d_dist *= @map_scale * 2.0f;

    // @dist_layout.set(d_dist);


    // if(d_dist > 0.001) return;

    // @is_clip.set(1);

    // // compute normals
    // const float eps = 0.001; // or some other value
    // const float2 h = (float2)(eps,0);

    // float2 d_nor = normalize((float2)(
    //     @sdf_disp.sample2d(q+h.xy) - @sdf_disp.sample2d(q-h.xy),
    //     @sdf_disp.sample2d(q+h.yx) - @sdf_disp.sample2d(q-h.yx))
    // );



    // float2 offxy = d_nor * fabs(d_dist);

    // uv += offxy;
    // @uv_layout.set(uv);

    // d_nor = -d_nor.yx;                          

    // // col = (float3)(d_nor.x, 0.0f, d_nor.y);
    // offxy = d_nor * fabs(d_dist);

    // off = (float3)(offxy.x, 0.0, offxy.y);
    // pos += off;
           
    // @Cd.set(col);
    // @P.set(pos);