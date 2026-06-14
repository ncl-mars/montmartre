#include "$HIP/scripts/ocl/include/biogrowth.cl"

#bind point &P            float3
#bind point  is_attractor int    read
#bind point &active       int

#bind point tree_P name=P float3 read input=1    // tree-stream P as a separate bind


#bind detail cell_count int[] read
#bind detail cell_start int[] read
#bind detail sorted_idx int[] read
#bind detail grid_min   float3 read
#bind detail grid_res   float3 read
#bind detail cell_size  float  read

#bind parm kill_radius float val=0.05

@KERNEL
{
    // if (@is_attractor < 0.5) return;
    if (@active       < 0.5) return;

    float3 apos   = @P;
    float  kr2    = @kill_radius * @kill_radius;
    int3   gres   = convert_int3(@grid_res);
    int3   center = bg_cell_coord(apos, @grid_min, @cell_size);

    __global const int*   counts = (const __global int*)  @cell_count.data;
    __global const int*   starts = (const __global int*)  @cell_start.data;
    __global const int*   sorted = (const __global int*)  @sorted_idx.data;


    __global const float* all_P  = (const __global float*)@tree_P.data;
    // __global const float* all_P  = (const __global float*)@P.data;

    for (int dz = -1; dz <= 1; dz++)
    for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++)
    {
        int3 nc = center + (int3)(dx, dy, dz);
        if (!bg_cell_valid(nc, gres)) continue;

        int cell_lin = bg_cell_linear(nc, gres);
        int start    = starts[cell_lin];
        int count    = counts[cell_lin];

        for (int i = 0; i < count; i++)
        {
            int    node_idx = sorted[start + i];
            float3 delta    = apos - vload3(node_idx, all_P);
            if (dot(delta, delta) < kr2) { @active.set(0); return; }
        }
    }
}

