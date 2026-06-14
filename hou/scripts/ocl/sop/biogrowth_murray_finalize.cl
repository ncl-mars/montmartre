#include "$HIP/scripts/ocl/include/biogrowth.cl"

// ---------------------------------------------------------------------------
// murray_finalize — interior-node radius update + accumulator reset.
// Decodes murray_acc (sum of child r^p in fixed-point), takes the 1/p root,
// writes branch_radius. Clears the accumulator for next frame.
//
// Tips keep their spawn-time radius (never written to by propagate).
// When disabled, just clears the accumulator — branch_radius stays at
// whatever topology / set_seed assigned.
// ---------------------------------------------------------------------------

#bind point  &branch_radius float
#bind point  &murray_acc    int
#bind point   is_tree_node  int read
#bind point   is_tip        int read

#bind parm murray_exponent  float val=2.0

@KERNEL
{
    if (@is_tip       > 0.5) return;   // tips keep their spawn-time radius

    float acc = (float)@murray_acc / BG_MURRAY_SCALE;
    if (acc > 0.0f) {
        float r = pow(acc, 1.0f / @murray_exponent);
        @branch_radius.set(r);
    }
    @murray_acc.set(0);
}
