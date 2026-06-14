#include "$HIP/scripts/ocl/include/biogrowth.cl"

// ---------------------------------------------------------------------------
// surface — soft force pulling tips toward an SDF isosurface.
//
// For each tip within surface_range of the target iso-level:
//   delta  = sdf(P) - target_iso
//   dir    = normalize(-grad) * sign(delta)      // toward the surface
//   weight = 1 - smoothstep(0, surface_range, |delta|)
//   force_acc_* += dir * surface_strength * weight
//
// Sign logic: grad points in the +sdf direction (outward from surface).
//   outside (delta > 0): dir = -grad → inward toward surface ✓
//   inside  (delta < 0): dir = +grad → outward toward surface ✓
//
// Inputs:
//   0 — main geometry stream (tree nodes + attractors).
//   1 — constraints bus (solver Input 2, imported via import_constraints).
//       Reads `surface_sdf`.
//
// Co-existence with collision (Stage 1): if the same SDF drives both,
// route it into both buses upstream (one VDB, two renames). Keep
// surface_range ≲ avoid_range so the pull never fights the wall
// resolver — surface attracts from outside the avoid band, avoid pushes
// back inside it, collision is the last word on penetration.
//
// Force contract: contributors emit `normalized_dir × strength × weight`
// before atomic-add. See biogrowth.cl for the rationale.
// ---------------------------------------------------------------------------

#bind point  &P              float3
#bind point   is_tree_node   int    read
#bind point   is_tip         int    read
#bind point  &force_acc     float3

#bind vdb    surface_sdf     float  input=1 read optional

#bind parm surface_strength  float  val=1.0
#bind parm surface_range     float  val=0.5
#bind parm target_iso        float  val=0.0

@KERNEL
{
    if (@is_tip       < 0.5) return;
    if (!@surface_sdf.bound) return;          // no VDB wired → contributor inert

    float sdf   = @surface_sdf.worldSample(@P);
    float delta = sdf - @target_iso;
    float absD  = fabs(delta);
    if (absD >= @surface_range) return;

    float3 grad   = @surface_sdf.worldGradient(@P);
    float  s      = (delta > 0.0f) ? 1.0f : ((delta < 0.0f) ? -1.0f : 0.0f);
    float3 dir    = bg_safe_normalize(-grad) * s;
    float  weight = 1.0f - smoothstep(0.0f, @surface_range, absD);
    float3 f      = dir * (@surface_strength * weight);

    @force_acc.set(@force_acc + f);
}
