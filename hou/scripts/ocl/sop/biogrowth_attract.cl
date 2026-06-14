#include "$HIP/scripts/ocl/include/biogrowth.cl"

#bind point  &P            float3
#bind point   is_attractor int    read
#bind point   active       int    read
#bind point  &att_ix       int
#bind point  &att_iy       int
#bind point  &att_iz       int
#bind point  &att_count    int

#bind detail cell_count int[] read
#bind detail cell_start int[] read
#bind detail sorted_idx int[] read
#bind detail grid_min   float3 read
#bind detail grid_res   float3 read
#bind detail cell_size  float  read

#bind parm perception_radius float val=0.2
#bind parm attract_strength  float val=1.0

@KERNEL
{
    // RUN ON ATTRACTORS
    if (@is_attractor < 0.5) return;
    if (@active       < 0.5) return;

    float3 apos   = @P;
    int3   gres   = convert_int3(@grid_res);
    int3   center = bg_cell_coord(apos, @grid_min, @cell_size);

    __global const int*   counts = (const __global int*)  @cell_count.data;
    __global const int*   starts = (const __global int*)  @cell_start.data;
    __global const int*   sorted = (const __global int*)  @sorted_idx.data;
    __global const float* all_P  = (const __global float*)@P.data;

    __global int* adx = (__global int*)@att_ix.data;
    __global int* ady = (__global int*)@att_iy.data;
    __global int* adz = (__global int*)@att_iz.data;
    __global int* ac  = (__global int*)@att_count.data;

    int   best    = -1;
    float best_d2 = @perception_radius * @perception_radius;

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
            int node_idx = sorted[start + i];
            float3 npos  = vload3(node_idx, all_P);
            float3 delta = apos - npos;
            float  d2    = dot(delta, delta);
            if (d2 < best_d2) { best_d2 = d2; best = node_idx; }
        }
    }

    if (best < 0) return;

    float3 npos_best = vload3(best, all_P);
    float3 dir       = bg_safe_normalize(apos - npos_best) * @attract_strength;
    bg_atomic_add_dir(adx, ady, adz, best, dir);
    atomic_inc(&ac[best]);
}
