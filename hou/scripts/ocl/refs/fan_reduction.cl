/*
    Use distance to fan falloff
    Raymarch volume toward fan for shadows
*/
    
#bind vdb &src_surf float
#bind vdb &off_surf float

// -- includes
    #include "matrix.h"
    #include "interpolate.h" 
    #include "CVDBUtil.h"
    #include "gxnoise.h"

    #include "$HIP/scripts/ocl/include/matrix_extra.cl"
    #include "$HIP/scripts/ocl/include/utils.cl"
    #include "$HIP/scripts/ocl/include/remapping.cl"
    #include "$HIP/scripts/ocl/include/smooth_min.cl"
    #include "$HIP/scripts/ocl/include/noises_iq.cl"

// -- structs
    typedef struct{
        cnanovdb_readaccessor acc;
        const global cnanovdb_griddata * grid;
    } cvdb;

    typedef struct{
        float off;
    } pmap;

// -- 
float2 mapShadows(cvdb trunc_vdb, pmap par, float3 pos){

    return cnanovdb_sampleF(trunc_vdb.grid, &trunc_vdb.acc, pos) + par.off; // off param !
}

float calcSoftshadow(cvdb trunc_vdb, pmap par, float3 ro, float3 rd, float mint, float maxt){

    float w = 0.5;
    float res = 1.0;
    float ph = 1e20;
    float t = mint;

    for( int i=0; i<64; i++ )
    {
        float h = mapShadows(trunc_vdb, par, ro + rd*t ).x;

        if( h<0.00 ){return 0;}

        float y = h*h/(2.0f*ph);
        float d = sqrt(h*h-y*y);
        res = min( res, d/(w*max(0.0f,t-y)) );

        ph = h;
        t += h; // t += max(h, 0.05f);
    } 
    res = clamp( res, 0.0f, 1.0f );
    return res*res*(3.0f-2.0f*res);
}

@KERNEL{

    if (!@src_surf.active) return;
    if(@Time < 0.01) return;

    //--
    cvdb trunc_vdb;
    cnanovdb_getreadaccessor(@src_surf.data, &trunc_vdb.acc);
    trunc_vdb.grid = (const global cnanovdb_griddata *) @src_surf.data;
    
    cnanovdb_coord gcoord = { .mVec = { @ix, @iy, @iz } };

    float dist = @src_surf;
    float3 pos = cnanovdb_indexToWorld(trunc_vdb.grid, (float3)(@ix, @iy, @iz)+(float3)0.5);
    float3 nor = @src_surf.worldGradient(pos);
    float osurf = @off_surf;
    float faceup = @mask;

    pmap par = {.off = 0.0f};

    // ------------------------------------------- fan
    float3 dir_wind = (float3)(0.0,0.0,1.0);
    mat3 rot; rotation3D(rot, (@fan_rot/180.0f)*PI);
    dir_wind = mat3vecmul(rot, dir_wind);    


    //-- shadows from fan
    float sha = calcSoftshadow(trunc_vdb, par, pos, -dir_wind, 0.25, 20.0);
    sha = clamp(sha, 0.0f,1.0f);
    sha *= 1.0;

    //-- dist from fan
    float fdist = length(pos - @fan_trs);
    fdist = smin_cube(fdist, @fan_max_dist, 0.5);

    fdist /= @fan_ref_dist;
    fdist = saturate(1.0 /(fdist*fdist)); //inverse square law

    //-- fan cone
    float theta = acos(dot(dir_wind, normalize(pos-@fan_trs)));//rads
    theta = fabs((theta/PI)*180.0); // to unsigned deg
    
    float blend_cone = 1.0-clamp(@fan_blend_cone, 0.0f, 1.0f);
    float fang = saturate(mapTo01(blend_cone*@fan_cone_ang, @fan_cone_ang, theta));

    fang = gain(fang, @fan_blend_exp);  // apply falloff shape
    fang = 1.0-fang;


    //----------------------------------------------- Apply
    float off = @fan_amp;
    
    off *= faceup;
    off *= sha;
    off *= fdist;
    off *= fang;
    osurf += off;

    @off_surf.set(osurf);
}

