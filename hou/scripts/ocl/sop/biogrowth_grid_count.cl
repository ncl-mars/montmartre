#include "$HIP/scripts/ocl/include/biogrowth.cl"

#bind point &P float3
#bind point is_tree_node int read

#bind detail &cell_count int[]
#bind detail grid_min    float3 read
#bind detail grid_res    float3 read
#bind detail cell_size   float  read

#bind point  is_tip         int read
#bind point  split_depth    int read
#bind parm   vigor_filter_enable  int val=1
#bind parm vigor_depth_cap    int val=2
#bind parm vigor_phase_stride int val=1

#bind point  lateral_count         int read
#bind parm   max_laterals_per_node int val=4

@KERNEL
{
    //-- RUN OVER NODE_TREE
    if (@is_tree_node < 0.5) return;

    int tick = (int)(@Time / @TimeInc + 0.5f);

    if (@is_tip < 0.5) {
        if (@lateral_count >= @max_laterals_per_node) return;
        if (@vigor_filter_enable > 0.5) {
            if (!bg_grid_eligible(@split_depth, tick, @vigor_depth_cap, @vigor_phase_stride)) return;
        }
    }




    int3 gres  = convert_int3(@grid_res);
    int3 coord = bg_cell_coord(@P, @grid_min, @cell_size);

    if (!bg_cell_valid(coord, gres)) return;

    int cell_lin = bg_cell_linear(coord, gres);
    __global int* counts = (__global int*)@cell_count.data;
    atomic_inc(&counts[cell_lin]);
}