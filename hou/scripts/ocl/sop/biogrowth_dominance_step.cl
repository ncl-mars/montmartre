#include "$HIP/scripts/ocl/include/biogrowth.cl"

#bind point  is_tree_node   int   read
#bind point  is_tip         int   read
#bind point  parent_id      int   read
#bind point  dominance      float read
#bind point  &dominance_acc int

#bind parm   dominance_strength float val=1.0
#bind parm   dominance_decay    float val=0.7

@KERNEL
{
    if (@parent_id    <  0 ) return;

    float v = (@is_tip > 0.5) ? @dominance_strength
                              : @dominance * @dominance_decay;
    if (v <= 0.0f) return;

    __global int* acc = (__global int*)@dominance_acc.data;
    int q = (int)(v * BG_DOMINANCE_SCALE + 0.5f);
    atomic_add(&acc[@parent_id], q);
}