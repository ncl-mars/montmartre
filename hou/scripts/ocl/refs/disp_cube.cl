#bind vdb &surface float
// #bind vdb no float3

#include "matrix.h" 
#include "CVDBUtil.h"


typedef struct{
    cnanovdb_readaccessor acc;
    const global cnanovdb_griddata * grid;
} cvdb;


@KERNEL
{
    cvdb surf_vdb; cnanovdb_getreadaccessor(@surface.data, &surf_vdb.acc);
    surf_vdb.grid = (const global cnanovdb_griddata *) @surface.data;

    float3 pos = cnanovdb_indexToWorld(surf_vdb.grid, (float3)(@ix, @iy, @iz)+(float3)0.5);
    float3 nor = normalize(@surface.worldGradient(pos));

    float off = 0;
    float dist = @surface;
    
    float chair = @chair;



    // float3 no = @no;
    float fac = smoothstep(0.0f, 0.25f, pos.y);
    // off += fac * no.z * 0.0f;

    fac = 0.0;
    fac += smoothstep(0.12f,0.0f,chair);


    dist -= fac;

    @surface.set(dist);


}
