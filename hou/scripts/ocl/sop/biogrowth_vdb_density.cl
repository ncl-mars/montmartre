#include "$HIP/scripts/ocl/include/biogrowth.cl"

// ---------------------------------------------------------------------------
// density — up-gradient steering toward regions of higher scalar density.
//
// For each tip, if local density exceeds `density_threshold` the contributor
// adds a force along the density gradient (ascent). Direction blends between
// unit-normalized (scene-independent tuning) and raw-magnitude (physically
// meaningful, continuously varying with gradient steepness):
//
//   weight   = saturate((sample - density_threshold) / density_range)
//   dir_unit = normalize(+grad_density)
//   dir_raw  = grad_density / density_reference_mag
//   dir      = mix(dir_unit, dir_raw, density_gradient_weight)
//   force_acc_*   += dir * density_strength * weight
//
// gradient_weight = 0 (default)
//   → pure unit direction; magnitude is strength × weight everywhere.
//     Predictable, scene-independent. Force is uniform within the band,
//     zero outside. Matches other contributors' convention.
//
// gradient_weight = 1
//   → raw gradient magnitude participates. A density plateau (flat grad)
//     gives no force even inside the band; a sharp transition gives strong
//     force. Field shape itself encodes where steering matters most.
//
// reference_mag
//   → scales raw gradient so raw-magnitude at `reference_mag` matches unit
//     magnitude. Default 1.0; tune per scene when raising gradient_weight.
//
// Sign: gradient points from low → high density, so `+grad` is up-hill. No
// sign flip compared to surface (which used `-grad × sign(delta)` to converge
// on an iso-level).
//
// Inputs:
//   0 — main geometry stream.
//   1 — constraints bus (solver Input 2, imported via import_constraints).
//       Reads `density`.
//
// Uses: nutrient fields, humidity, soil depth, canopy light-pooling — any
// scalar field where "grow toward more" is the desired behaviour. Combine
// with photo for directional light + density for resource concentration.
//
// Force contract: contributors emit `dir × strength × weight` before
// atomic-add. Density is the one contributor where `dir` may carry
// magnitude info (via gradient_weight > 0) — a documented opt-in deviation
// from the strict unit-dir contract. See biogrowth.cl.
// Optional binding: contributor is inert when `density` is not wired.
// ---------------------------------------------------------------------------

#bind point  &P                      float3
#bind point   is_tree_node           int    read
#bind point   is_tip                 int    read
#bind point  &force_acc                    float3

#bind vdb    density                 float  input=1 read optional

#bind parm density_strength          float  val=1.0
#bind parm density_threshold         float  val=0.0
#bind parm density_range             float  val=1.0
#bind parm density_gradient_weight   float  val=0.0
#bind parm density_reference_mag     float  val=1.0

@KERNEL
{
    if (@is_tip       < 0.5) return;
    if (!@density.bound)      return;

    float sample = @density.worldSample(@P);
    float weight = clamp((sample - @density_threshold) / max(@density_range, 1e-6f),
                         0.0f, 1.0f);
    if (weight <= 0.0f) return;     // below threshold — skip the expensive gradient

    float3 grad     = @density.worldGradient(@P);
    float3 dir_unit = bg_safe_normalize(grad);
    float3 dir_raw  = grad / max(@density_reference_mag, 1e-6f);
    float3 dir      = mix(dir_unit, dir_raw, clamp(@density_gradient_weight, 0.0f, 1.0f));

    float3 f = dir * (@density_strength * weight);

    @force_acc.set(@force_acc + f);
}
