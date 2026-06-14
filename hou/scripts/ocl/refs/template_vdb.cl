#include "interpolate.h"
#include "CVDBUtil.h"

// #endif
// #define AT_ix   _bound_gidx
// #define AT_iy   _bound_gidy
// #define AT_iz   _bound_gidz
// #define AT_elemnum      _bound_idx
// #define AT_sdf_data        ((global cnanovdb_griddata * restrict)_bound_sdf)
// #define AT_sdf_bound       1
// #define AT_sdf     cnanovdb_readaccessor_getValueF(&_bound_sdf_acc, &_bound_gcoord)
// #define AT_sdf_active      cnanovdb_readaccessor_isActiveF(&_bound_sdf_acc, &_bound_gcoord)
// #define AT_sdf_get AT_sdf
// #define AT_sdf_pos cnanovdb_indexToWorld(AT_sdf_data, (float3)(_bound_gidx, _bound_gidy, _bound_gidz))
// #define AT_sdf_getAt(_idx_x_, _idx_y_, _idx_z_)    cnanovdb_readaccessor_getIJKValueF(&_bound_sdf_acc, _idx_x_, _idx_y_, _idx_z_)
// #define AT_sdf_valid(_idx_x_, _idx_y_, _idx_z_)    cnanovdb_readaccessor_isIJKActiveF(&_bound_sdf_acc, _idx_x_, _idx_y_, _idx_z_)
// #define AT_sdf_activeAt(_idx_x_, _idx_y_, _idx_z_) cnanovdb_readaccessor_isIJKActiveF(&_bound_sdf_acc, _idx_x_, _idx_y_, _idx_z_)
// #define AT_sdf_sample(_pos_)       cnanovdb_sampleF_trilinear(&_bound_sdf_acc, CAST_VEC3(_pos_))
// #define AT_sdf_worldSample(_pos_)  cnanovdb_sampleF(AT_sdf_data, &_bound_sdf_acc, _pos_)
// #define AT_sdf_worldGradient(_pos_)        cnanovdb_gradient(AT_sdf_data, &_bound_sdf_acc, _pos_)
// #define AT_sdf_set(_val_)  cnanovdb_readaccessor_setValueF(&_bound_sdf_acc, &_bound_gcoord, _val_)
// #define AT_sdf_setAt(_idx_x_, _idx_y_, _idx_z_, _val_)     cnanovdb_readaccessor_setIJKValueF(&_bound_distance_acc, _idx_x_, _idx_y_, _idx_z_, _val_)

// #define AT_col_data     ((global cnanovdb_griddata * restrict)_bound_col)
// #define AT_col_bound    1
// #define AT_col  cnanovdb_Vec3F_to_float3(cnanovdb_readaccessor_getValueF3(&_bound_col_acc, &_bound_gcoord))
// #define AT_col_active   cnanovdb_readaccessor_isActiveF3(&_bound_col_acc, &_bound_gcoord)
// #define AT_col_get      AT_col
// #define AT_col_pos      cnanovdb_indexToWorld(AT_col_data, (float3)(_bound_gidx, _bound_gidy, _bound_gidz))
// #define AT_col_getAt(_idx_x_, _idx_y_, _idx_z_) cnanovdb_Vec3F_to_float3(cnanovdb_readaccessor_getIJKValueF3(&_bound_col_acc, _idx_x_, _idx_y_, _idx_z_))
// #define AT_col_valid(_idx_x_, _idx_y_, _idx_z_) cnanovdb_readaccessor_isIJKActiveF3(&_bound_col_acc, _idx_x_, _idx_y_, _idx_z_)
// #define AT_col_activeAt(_idx_x_, _idx_y_, _idx_z_)      cnanovdb_readaccessor_isIJKActiveF3(&_bound_col_acc, _idx_x_, _idx_y_, _idx_z_)
// #define AT_col_sample(_pos_)    cnanovdb_sampleF3_trilinear(&_bound_col_acc, CAST_VEC3(_pos_))
// #define AT_col_worldSample(_pos_)       cnanovdb_sampleF3(AT_col_data, &_bound_col_acc, _pos_)
// #define AT_col_set(_val_)       cnanovdb_readaccessor_setValueF3(&_bound_col_acc, &_bound_gcoord, cnanovdb_float3_to_Vec3F(_val_))
// #define AT_col_setAt(_idx_x_, _idx_y_, _idx_z_, _val_)  cnanovdb_readaccessor_setIJKValueF3(&_bound_col_acc, _idx_x_, _idx_y_, _idx_z_, cnanovdb_float3_to_Vec3F(_val_))


// #line 1
// // Bind writable position:


// float sdf_voxelsize_x,
// float sdf_voxelsize_y,
// float sdf_voxelsize_z,
// float16 sdf_xformtoworld,
// float16 sdf_xformtovoxel,
// global void * restrict sdf


float lerpConstant( constant float * in, int size, float pos);

kernel void kernelName( 
    global const int * restrict tilestartsxyz,
    float sdf_voxelsize_x,
    float sdf_voxelsize_y,
    float sdf_voxelsize_z,
    global void * restrict sdf
)
{
    cnanovdb_readaccessor sdf_acc;
    cnanovdb_getreadaccessor(sdf, &sdf_acc);
    int tileidx = get_global_id(0);
    int tileoffset = tileidx & 511;
    tileidx = tileidx >> 9;
    int3 tilebasexyz = vload3(tileidx, tilestartsxyz);
    int gidx = tilebasexyz.x + (tileoffset & 7);
    int gidy = tilebasexyz.y + ((tileoffset >> 3) & 7);
    int gidz = tilebasexyz.z + ((tileoffset >> 6) & 7);
    int idx = (tileidx << 9) + tileoffset;
    cnanovdb_coord gcoord = { .mVec = { gidx, gidy, gidz } };
    if (!cnanovdb_readaccessor_isActiveF(&sdf_acc, &gcoord))
        return;


    float d = cnanovdb_readaccessor_getValueF(&sdf_acc, &gcoord);
    d =1.5;
    cnanovdb_readaccessor_setValueF(&sdf_acc, &gcoord, d);


    // cnanovdb_node0F *node = sdf_acc->mNode[0];
    // uint32_t n = cnanovdb_node0F_CoordToOffset(gcoord);
    // node->mVoxels[idx] = 1.0;
    // sdf[idx] = 1.0f;

}




//
    // float map(cvdb vdb, pmap par, float3 pos){

    //     float d = 0;
    //     d = cnanovdb_sampleF(vdb.grid, &vdb.acc, pos) - 0.05; // off param !

    //     pos *= par.no_scale;


    //     float no = 1.0;

        
    //     // no = wNoise(pos).x-0.5;
    //     // no = voronoi(pos, 0.5).x-0.5;
    //     // no = voronoiDistance(pos)-0.0125;
    //     // no = VEXgxnoise_3_1(pos) * 2 - 1;
    //     // no = VEXcurlgxnoise_3(pos).x  * 2 - 1;
    //     no = fbm(pos + par.no_off, 7)-par.no_dist;


    //     // no = warpFunc(pos*0.25f)-1.5f;
    //     // no = warpFunc(pos*0.25f)*2.0-1.0;


    //     // d = sdfOpSubtractSmooth(d, -no, 0.0f);
    //     // d = sdfOpIntersectSmooth(-no, -d+0.2, 0.0f);
    //     // d = sdfOpIntersectSmooth(d, no, 0.0f);
    //     d = sdfOpUnionSmooth(d, no , 0.0);

    //     return d;
    //     // return min(d,no);
    // }


    // cvdb sdf_vdb;

    // // cnanovdb_readaccessor sdf_acc;
    // cnanovdb_getreadaccessor(sdf, &sdf_vdb.acc);
    // cnanovdb_readaccessor col_acc;
    // cnanovdb_getreadaccessor(col, &col_acc);
    // int tileidx = get_global_id(0);
    // int tileoffset = tileidx & 511;
    // tileidx = tileidx >> 9;
    // int3 tilebasexyz = vload3(tileidx, tilestartsxyz);
    // int gidx = tilebasexyz.x + (tileoffset & 7);
    // int gidy = tilebasexyz.y + ((tileoffset >> 3) & 7);
    // int gidz = tilebasexyz.z + ((tileoffset >> 6) & 7);
    // int idx = (tileidx << 9) + tileoffset;
    // cnanovdb_coord gcoord = { .mVec = { gidx, gidy, gidz } };


    // if (!cnanovdb_readaccessor_isActiveF(&sdf_vdb.acc, &gcoord)) return;

    // float3 gid = (float3)(gidx, gidy, gidz);

    // sdf_vdb.grid = (const global cnanovdb_griddata *) sdf;
    // // const global cnanovdb_griddata *sdf_grid = (const global cnanovdb_griddata *) sdf;


    // float3 pos = cnanovdb_indexToWorld(sdf_vdb.grid, gid);

    // float d = 1e6;
    // d = map(sdf_vdb.grid, &sdf_vdb.acc, pos);


    // float3 cd = (float3)(0.51, 0.5,0.0);
    // cd = calcNormal(sdf_vdb.grid, &sdf_vdb.acc, pos)*-1;
    // // cd = calcNormal(sdf_grid, &sdf_vdb.acc, pos)*-1;

    // // sd -= 0.15;
    // // cd = pos;

    // cnanovdb_readaccessor_setValueF(&sdf_vdb.acc, &gcoord, d);
    // cnanovdb_readaccessor_setValueF3(&col_acc, &gcoord, cnanovdb_float3_to_Vec3F(cd));