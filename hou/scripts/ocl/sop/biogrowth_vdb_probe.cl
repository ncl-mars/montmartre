#include "$HIP/scripts/ocl/include/biogrowth.cl"

// ---------------------------------------------------------------------------
// VDB probe — Stage 0 plumbing check.
// Proves the contributor chain end-to-end before any real constraint lands:
//  - Input 0: main geometry stream (tree nodes + attractors).
//  - Input 1: VDB bus (imported from solver Input 3 via Object Merge).
//  - Reads one scalar VDB, writes zero into acc_*.
// If this cooks without error and the sim runs unchanged, wiring is good.
//
// Force contract (see biogrowth.cl): contributors emit
//   normalized_dir × <name>_strength × optional_weight
// and atomic-add into acc_*. Probe emits (0,0,0) — no behavior change.
// ---------------------------------------------------------------------------

#bind point  &P          float3
#bind point   is_tree_node int    read
#bind point   is_tip     int    read
#bind point  &force_acc float3

#bind vdb    probe_sdf   float  input=1 read

#bind parm probe_strength float val=0.0

@KERNEL
{
    // RUN ON TREE TIPS
    if (@is_tree_node < 0.5) return;
    if (@is_tip       < 0.5) return;

    // Sample the VDB just to exercise the binding — value discarded.
    float  s    = @probe_sdf.worldSample(@P);
    float3 grad = @probe_sdf.worldGradient(@P);

    // No-op write: emit zero force. Keeps the force-contract call site
    // honest so it can be copy-pasted into a real contributor by swapping
    // the dir expression.
    float3 dir = (float3)(0.0f, 0.0f, 0.0f);
    dir *= @probe_strength * s * length(grad);  // uses samples, optimizer-proof

    @force_acc.set(@force_acc + dir);
}
