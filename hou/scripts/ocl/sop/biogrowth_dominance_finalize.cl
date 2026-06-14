#include "$HIP/scripts/ocl/include/biogrowth.cl"

#bind point  is_tree_node   int   read
#bind point  &dominance_acc int
#bind point  &dominance     float

@KERNEL
{
    __global int* acc = (__global int*)@dominance_acc.data;
    int idx = get_global_id(0);
    @dominance.set((float)acc[idx] / BG_DOMINANCE_SCALE);
    acc[idx] = 0;
}
