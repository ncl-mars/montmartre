#include "$HIP/scripts/ocl/include/biogrowth.cl"

// ---------------------------------------------------------------------------
// photo — spatial tropism via direct vector-field sample.
//
// For each tip, samples a float3 VDB at P and steers along it:
//   force_acc_* += sample(light_dir, P) * photo_strength
//
// Uses: phototropism (global or per-region light direction), custom biased
// tropism fields, wind-aligned growth, any "this way is preferred here"
// spatial hint. Equivalent to scalar tropism with location-dependent
// direction — the field encodes the whole choice.
//
// No in-sim normalization: the sampled vector is used raw. Magnitude is
// the author's responsibility — normalize the VDB upstream (outside sim)
// if you want uniform strength across space, or leave it unnormalized to
// let the field itself encode where the pull is strong vs weak. A
// near-zero sample is a natural no-op, so sparsely-painted fields work
// without special-casing.
//
// Inputs:
//   0 — main geometry stream.
//   1 — constraints bus (solver Input 2, imported via import_constraints).
//       Reads `light_dir` (float3 VDB).
//
// Force contract: most contributors emit `normalized_dir × strength × weight`.
// Photo is the documented opt-out — raw sample × strength, magnitude comes
// from the field. Same opt-in-magnitude philosophy as density's
// gradient_weight=1 mode, but structural (no blend parm).
// Optional binding: contributor is inert when `light_dir` is not wired.
// ---------------------------------------------------------------------------

#bind point  &P                 float3
#bind point   is_tree_node      int    read
#bind point   is_tip            int    read
#bind point  &force_acc               float3

#bind vdb    light_dir          float3 input=1 read optional
#bind parm photo_strength       float  val=1.0

@KERNEL
{
    if (@is_tip       < 0.5) return;
    if (!@light_dir.bound)    return;

    float3 v = @light_dir.worldSample(@P);
    float3 f = v * @photo_strength;

    @force_acc.set(@force_acc + f);
}
