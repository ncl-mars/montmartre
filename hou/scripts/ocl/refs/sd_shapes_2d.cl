// #include "C:\Program Files\Side Effects Software\Houdini 20.5.522\houdini\ocl\include\interpolate.h" 
#include "interpolate.h"

float lerpConstant( constant float * in, int size, float pos);

kernel void kernelName( 
    int density_stride_x,
    int density_stride_y,
    int density_stride_z,
    int density_stride_offset,
    int density_res_x,
    int density_res_y,
    int density_res_z,
    float density_voxelsize_x,
    float density_voxelsize_y,
    float density_voxelsize_z,
    global float * restrict density
)
{
    int gidx = get_global_id(0);
    int gidy = get_global_id(1);
    int gidz = get_global_id(2);
    int idx = density_stride_offset + density_stride_x * gidx
                               + density_stride_y * gidy
                               + density_stride_z * gidz;

    

    density[idx] = idx*0.00001;

}