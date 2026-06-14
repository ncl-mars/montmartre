#bind layer sdf? val=0
#bind layer nor? val=0
#bind layer clip? val=1

#bind layer &outline_indices?

#bind layer &ids int
#bind layer &mask? float val = 0.0


#bind parm dot_thresh float val=0.0
#bind parm steps int val=4
#bind parm single_side int val=0


#include "matrix.h"

#include "$HIP/scripts/ocl/include/remapping.cl"
#include "$HIP/scripts/ocl/include/utils.cl"


mat2 rotate2d(float _angle){
    return (mat2)(cos(_angle),-sin(_angle),
                sin(_angle),cos(_angle));
}


#define TAU M_PI * 2.0

@KERNEL
{
    float4 col = {0,0,0,1};

    // if( (sign(@sdf.x) < 0 && @single_side > 0) || @clip.x < 0.5) return;
    if( (sign(@sdf.x) < 0.0 && @single_side > 0) ) return;
    
    
    float maxres = max(@nor.xres, @nor.yres);
    float vsize = 1.0f/maxres;

    float2 nor = @nor.xy;

    float2 P = @P;
    P += vsize * 0.5f;

    float2 off = (float2)(0,vsize * 2.0);
    mat2 rot = rotate2d(TAU / (float)@steps);

    for(int i = 0; i < @steps; i++){

        float2 norn = @nor.imageSample(P+off).xy;
        off = mat2vecmul(rot, off);

        // is edge
        if(dot(nor, norn) < @dot_thresh){

            int idx = @ix + @iy * @sdf.xres;
            __global int* indices = (__global int*)@ids.data;
            int slot = atomic_inc(&indices[0]) + 1;
            indices[slot] = idx;

            @mask.set(1.0);

            break;
        }
    }
}

