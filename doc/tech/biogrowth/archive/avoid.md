# Avoid: Separate from Integrate

Refactor: extract `avoid` from `integrate`'s stage 2 and run it as its own OCL contributor among `surface` / `density` / `photo` / `self_collide`. Reverses the 2026-04-19 fusion, which was justified by costs that no longer apply.

## Why now

The original fusion (network.md, 2026-04-19) had two perf reasons:
1. Avoid wrote into `acc_*` via int-fixed-point atomics; integrate decoded them — wasted encode/decode work.
2. Three nodes (avoid / integrate / collision) paid 3× kernel dispatches and 3× per-point filter pass.

Both have been mitigated:
1. **Force_acc rename (2026-04-22)** replaced the int-atomic `acc_*` pattern with a plain `v@force_acc` float3, single-writer per point. No encode/decode. Avoid as a separate contributor would now write to `force_acc` like every other soft contributor.
2. **Split-streams refactor (2026-04-25)** removed the `is_tree_node` filter cost from tree-only kernels — they run on the tree-only stream. Adding one more dispatch is one more dispatch, no extra filter pass.

What remains is just **one kernel dispatch** (~10–50µs) per frame to gain a uniform contributor contract.

## What stays inside integrate

**Collision (stage 4)** stays. It modifies position and velocity post-integration — not a force, a position correction. Doesn't fit the force-contributor pattern. Tightly coupled to the Euler step (modifies `P_new` and `v` in registers). Fusion still earns its keep here.

## Conceptual change: separate VDB

Avoid is decoupled from collision entirely. New dedicated VDB:

- **`avoid_sdf`** (float, constraints bus / Input 2): the soft-repulsion field consumed by this contributor. Authored independently — typically low-res, wide-band, encoding "where the tree should be steered away from".
- **`avoid_weight`** (float, constraints bus, optional): per-region scalar weighting, same pattern as before.
- **`collision_sdf`** stays on its own (collision bus / Input 3) for integrate's hard snap-out only. The two no longer share a VDB.

Why split: avoid is steering, collision is precision. Different resolutions, different authoring intents. Sharing one VDB was an artifact of the fusion; un-fused, they don't need to share. Authoring may use the same source mesh and bake both at different settings, or use entirely different fields (e.g. designer "no-grow" sketches feeding `avoid_sdf` only).

## Kernel

`hou/scripts/ocl/sop/biogrowth_vdb_avoid.cl`:

```cl
#include "$HIP/scripts/ocl/include/biogrowth.cl"

#bind point  is_tree_node  int    read
#bind point  is_tip        int    read
#bind point  P             float3 read
#bind point  velocity      float3 read
#bind point  &force_acc    float3

#bind vdb    avoid_sdf     float input=1 read
#bind vdb    avoid_weight  float input=1 read optional

#bind parm   avoid_strength float val=1.0
#bind parm   avoid_range    float val=0.1

@KERNEL
{
    if (@is_tree_node < 0.5) return;
    if (@is_tip       < 0.5) return;

    float sdf = @avoid_sdf.worldSample(@P);
    if (sdf >= @avoid_range) return;                 // field-content gates

    float3 n  = bg_safe_normalize(@avoid_sdf.worldGradient(@P));
    float  vn = dot(@velocity, n);
    if (vn >= 0.0f) return;                          // not moving into the field

    float3 r       = @velocity - 2.0f * vn * n;
    float3 dir     = bg_safe_normalize(r);
    float  falloff = 1.0f - smoothstep(0.0f, @avoid_range, sdf);
    float  wsample = @avoid_weight.bound ? @avoid_weight.worldSample(@P) : 1.0f;

    @force_acc.set(@force_acc + dir * (@avoid_strength * falloff * wsample));
}
```

`avoid_sdf` is bound non-optional: the authoring contract requires it whenever this contributor is enabled (Switch-SOP `avoid_enable` is the bypass for "no avoid this scene"). `avoid_weight` stays optional with default 1.0.

## Authoring

Two separate VDBs upstream:
- `avoid_sdf` baked into the constraints bus (alongside `surface_sdf` / `density` / `photo` / `irradiance`). Typically a low-res / wide-band SDF derived from collision geometry — or a different "no-grow" field if you want the steering to differ from physical collision.
- `collision_sdf` baked into the collision bus (high-res, narrow-band, sharp).

Migration: scenes currently authoring only `collision_sdf` need to also bake an `avoid_sdf` (can start as a copy of `collision_sdf` for backward-compat, then diverge if needed).

## Position in pipeline

Tree-only stream, between split and integrate, sharing the constraints bus with the other VDB contributors:

```
SPLIT
  → surface → density → photo → avoid → self_collide
  → integrate (avoid block deleted from stage 2)
  → murray → dominance → topology
```

Order among soft contributors is commutative — they all write to `force_acc` additively. Place wherever feels natural; alongside `surface` / `density` / `photo` (collision-bus consumer next to constraints-bus consumers) is logical.

## Toggle

Switch-SOP `avoid_enable` wraps the new node — same pattern as `surface_enable` / `density_enable` / `photo_enable` / `self_collide_enable`. Removes the in-kernel `avoid_enable` parm from integrate.

## Parms

Already on the solver: `avoid_strength`, `avoid_range`. Move them onto the new node (or keep them on the solver as spare parms — same effect via channel reference).

## Stages

**Stage 1 — kernel + node.** Add `biogrowth_vdb_avoid` OCL SOP between the existing soft contributors and `integrate`. Wire constraints bus to Input 1. Test: bypass the new node, verify nothing changes (since integrate still has the avoid block). Enable it, verify avoid behavior is preserved (force_acc now carries what was inline before).

**Stage 2 — remove from integrate.** Delete stage 2 (avoid block) from `biogrowth_integrate.cl`. Remove `avoid_enable` in-kernel parm. Verify identical sim output to Stage 1.

**Stage 3 — Switch-SOP wrap.** Add `avoid_enable` Switch wrapping the new node. Default 1.

**Stage 4 — docs + memory.** Update `doc/network.md` (move avoid out of integrate's section, add new soft-contributor entry), this file marked landed, project memory.

## Cost / benefit summary

- **Cost**: one kernel dispatch (~10–50µs/frame). Negligible.
- **Benefit**: uniform contributor contract; Switch-SOP toggle; smaller integrate; composable with future avoid variants (e.g. multiple SDFs).

Worth doing as part of the same general "post-split-streams cleanup" pass.
