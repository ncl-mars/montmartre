#include "$HIP/scripts/ocl/include/biogrowth.cl"

// ---------------------------------------------------------------------------
// self_collide — per-tip soft repulsion from nearby tree nodes of the same
// tree. Ancestor exclusion via bidirectional parent_id walk: for each close
// candidate, walk up to `ancestor_hops` links from both sides; skip if either
// chain hits the other. Distance check runs FIRST so most 27-cell candidates
// skip the walk entirely.
//
// Force is accumulated as a sum (not unit-normalized): multiple nearby
// branches push harder than one. Documented opt-out from the unit-dir force
// contract — same as density/photo.
// ---------------------------------------------------------------------------

#bind point  &P                    float3
#bind point   is_tree_node         int    read
#bind point   is_tip               int    read
#bind point   parent_id            int    read
#bind point   branch_radius        float  read
#bind point  &force_acc                  float3

#bind detail cell_count            int[]  read
#bind detail cell_start            int[]  read
#bind detail sorted_idx            int[]  read
#bind detail grid_min              float3 read
#bind detail grid_res              float3 read
#bind detail cell_size             float  read

#bind parm self_collide_strength   float  val=1.0
#bind parm self_collide_range      float  val=1.0
#bind parm ancestor_hops           int    val=4

@KERNEL 
{
    if (@is_tip       < 0.5) return;

    int me = get_global_id(0);
    float3 P_me = @P;
    float  r_me = @branch_radius;

    __global const int*   counts = (const __global int*)  @cell_count.data;
    __global const int*   starts = (const __global int*)  @cell_start.data;
    __global const int*   sorted = (const __global int*)  @sorted_idx.data;
    __global const float* all_P  = (const __global float*)@P.data;
    __global const int*   all_pid= (const __global int*)  @parent_id.data;
    __global const float* all_r  = (const __global float*)@branch_radius.data;

    int3 gres   = convert_int3(@grid_res);
    int3 center = bg_cell_coord(P_me, @grid_min, @cell_size);

    float3 force = (float3)(0, 0, 0);

    for (int dz = -1; dz <= 1; dz++)
    for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++) {
        int3 nc = center + (int3)(dx, dy, dz);
        if (!bg_cell_valid(nc, gres)) continue;

        int cell_lin = bg_cell_linear(nc, gres);
        int start    = starts[cell_lin];
        int count    = counts[cell_lin];

        for (int i = 0; i < count; i++) {
            int other = sorted[start + i];
            if (other == me) continue;

            // Distance check FIRST — most candidates in the 27-cell window
            // fail this cheap test and skip the expensive ancestor walk.
            float3 P_other = vload3(other, all_P);
            float3 delta   = P_me - P_other;
            float  d2      = dot(delta, delta);
            float  r_sum   = (r_me + all_r[other]) * @self_collide_range;
            if (d2 >= r_sum * r_sum) continue;

            // Bidirectional ancestor walk (only for in-range candidates).
            bool skip = false;
            int p = all_pid[me];
            for (int h = 0; h < @ancestor_hops && p >= 0 && !skip; h++) {
                if (p == other) skip = true;
                p = all_pid[p];
            }
            if (!skip) {
                int q = all_pid[other];
                for (int h = 0; h < @ancestor_hops && q >= 0 && !skip; h++) {
                    if (q == me) skip = true;
                    q = all_pid[q];
                }
            }
            if (skip) continue;

            float d      = sqrt(d2);
            float weight = 1.0f - smoothstep(0.0f, r_sum, d);
            float3 dir   = bg_safe_normalize(delta);
            force += dir * weight;
        }
    }

    force *= @self_collide_strength;

    @force_acc.set(@force_acc + force);
}
