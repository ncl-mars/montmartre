/*
* can't concat to vector in kernel decleration! bit padding?
*/

////////////////////////////////////////////////////////////////////////// IMPORTS
#include "interpolate.h" 
#include "matrix.h"
#include "sdf.h"
#include "quaternion.h"

#include "$HOME/libs/ocl/include/sd_shapes_3d.cl"
#include "$HOME/libs/ocl/include/matrix_extra.cl"

// #include "typedefines.h"

////////////////////////////////////////////////////////////////////////// METHODS
float lerpConstant( constant float * in, int size, float pos);

int toLinearIndex(int offset, int stride_x, int stride_y, int stride_z, int3 gid){
    return offset   + stride_x * gid.x
                    + stride_y * gid.y
                    + stride_z * gid.z;
}

int3 getVoxCoord(){
    return (int3)(
        get_global_id(0),
        get_global_id(1),
        get_global_id(2)
    );
}

float3 toLocalPos(float3 gid, float3 res){
    return ((gid + (float3)0.5) * (float3)2.0 - res) / res;
}


////////////////////////////////////////////////////////////////////////// KERNELS
//-- Display 3D SDF Shapes library
kernel void sdShapes3D( 

    // strides
    int str_x,
    int str_y,
    int str_z,
    int str_off,
    
    // grid resolution
    int res_x,
    int res_y,
    int res_z,

    // voxelsize
    float dx,
    float dy,
    float dz,

    float16 xformtoworld,               // voxel transform
    global float * restrict density     // volume data

    )
{
    int3 gid = getVoxCoord();
    int idx = toLinearIndex(str_off, str_x, str_y, str_z, gid);
    
    float3 res = (float3)(res_x,res_y,res_z);
    float3 pos = toLocalPos(convert_float3(gid), res);

    mat4 m = (mat4)xformtoworld;


    float d = 1e6; 
    float3 q = pos;
    q.x -= 0.5;
    q.y -= 0.5;
 
    // matrix rotations
    mat3 mrot; 
    rotationAxisAngle(mrot, (float3)(1.0f,0.0f,0.0f), 0.9f);
    // rotateAxisAngle(mrot, (float3)(.0f,1.0f,.0f), 0.2f);

    q = mat3vecmul(mrot, q);
    d = sdBox(q, (float3)(0.15,0.5,0.15 )); 

 
    q = pos;
    q.x += 0.5;
    q.y += 0.5;
 
    // quaternions rotations 
    quat qrot = qfromaxisangle(0.5, (float3)(1.0f,.0f,.0f));
    q = qrotate(qrot, q);
    d = min(d,sdTorus(q, (float2)(0.25,0.25)));


    d = fabs(d);

    float f = 1.0-smoothstep(0.0f,0.05f,d);
    density[idx] = f;
}

