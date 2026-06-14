// #include "C:\Program Files\Side Effects Software\Houdini 20.5.522\houdini\ocl\include\interpolate.h" 
// #include "$HFS/houdini/ocl/include/interpolate.h"
#include "interpolate.h"
float lerpConstant( constant float * in, int size, float pos);

kernel void kernelName( 
    int height_stride_x,
    int height_stride_y,
    int height_stride_z,
    int height_stride_offset,
    int height_res_x,
    int height_res_y,
    int height_res_z,
    float height_voxelsize_x,
    float height_voxelsize_y,
    float height_voxelsize_z,
    global float * restrict height
)
{
    int gidx = get_global_id(0);
    int gidy = get_global_id(1);
    int gidz = get_global_id(2);
    int idx = height_stride_offset + height_stride_x * gidx
                               + height_stride_y * gidy
                               + height_stride_z * gidz;

}
