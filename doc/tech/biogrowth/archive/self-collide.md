# Self-Collision — Plan

Adds a per-tip soft-repulsion contributor (`self_collide`) that pushes growing tips away from older branches of the same tree. Replaces the earlier Sweep→voxelize→feedback-VDB proposal: that approach was heavy (per-frame meshing), stale (last-frame geometry in current-frame SDF), and architecturally awkward (outside the plugin chain). The new design fits the soft-contributor slot alongside surface/density/photo — one 27-cell grid search, one atomic-add into `acc_*`, per-frame fresh, no meshing in the hot loop.

Branch: `feature/self-collide` (cut from `feature/vigor` 2026-04-20).

## Context

The solver has no self-interaction today. Tips can grow through their own parent trunk or cross older laterals — visually wrong for mature trees. Two past proposals:

1. **Sweep→VDB feedback** (original Phase 3 design, rejected). Every frame, Sweep the polylines into tubes, voxelize, feed the SDF back into `collision_sdf` upstream. Cons: N-frame stale under sim feedback lag, N× meshing cost per frame, couples collision to meshing cadence.

2. **Nearest-branch repulsion** (this plan, user-proposed). Each tip does a 27-cell search over tree nodes, repels from any non-ancestor within combined radius. Pros: per-frame fresh, reuses the existing grid (zero extra overhead in grid construction), fits the plugin chain (commutative with other `acc_*` contributors), no meshing in the loop.

The shift from VDB-based to node-based self-collision also opens the door to thickness-aware behaviour: if each node carries a `branch_radius`, the pair metric becomes `d < self.r + other.r` — trunk-trunk contacts are strong, thin-lateral contacts are weak, emergent free-form taper-aware collision.

## Design decisions (user-confirmed)

- **Per-node `branch_radius`, depth-driven.** `base_radius * pow(taper_decay, split_depth)`. Mainline arc-spawn inherits parent's radius unchanged (straight shafts don't thin); lateral spawn applies `taper_decay` (true bifurcations thin the daughter). Downstream Sweep reads the same attribute — authored once, consumed twice. Folds the earlier "Phase 3b taper" work into Phase 3a because radius is now load-bearing for collision.
- **Pair metric: `d < (self.branch_radius + other.branch_radius) × self_collide_range`.** Sphere-sphere with a global range multiplier. Default `self_collide_range = 1.0` = physical contact only. `>1` = personal-space buffer that scales with thickness (trunks get proportionally more buffer than twigs); `<1` = allow overlap. Kept as a multiplier (not additive) so the knob is scale-invariant across the tree's radius range. The falloff's zero point moves with `r_sum`, so the buffer zone inherits the smoothstep for free.
- **Ancestor exclusion via short-hop `parent_id` walk, bidirectional.** Walk up from self's `parent_id` chain for N hops (N=4 default); if any ancestor equals the candidate, skip. Then walk from candidate's chain for N hops; if any equals self, skip. Covers immediate lineage (parent, grandparent, great-grandparent, great-great-grandparent) — beyond that, geometric distance handles exclusion naturally. Cost: 2N memory loads per candidate pair in the worst case; cheap at scale.
- **Written as a tip-only soft contributor.** Same pattern as surface/density/photo: filter on `is_tree_node && is_tip`, atomic-add `normalized_dir × strength × weight` into `acc_*`. Commutative with other soft contributors. Position in pipeline: after the VDB contributors, before integrate. Order doesn't matter mathematically (all atomic-adds) but keeping it last makes the graph readable.
- **Falloff: smoothstep.** `weight = 1 - smoothstep(0, r, d)` where `r = self.branch_radius + other.branch_radius`. Matches the falloff shape used by surface/density/avoid — C¹-continuous, zero at r, one at d=0.

## Attribute schema delta

**Tree nodes — add:**
- `i@parent_id` (int, seed=-1). Written by topology on spawn: `child.parent_id = @ptnum` (parent is the source point). Mainline and lateral spawn paths both write it.
- `f@branch_radius` (float, seed=`base_radius`). Inherited on spawn:
  - Mainline arc-spawn: `child.branch_radius = parent.branch_radius`.
  - Lateral spawn: `child.branch_radius = parent.branch_radius * taper_decay`.

**No other schema changes.** Ancestor walk reads `parent_id` from neighbouring points via existing cross-input access pattern (same pattern attract uses to read `all_P[]`).

## Kernel — `biogrowth_self_collide.cl`

```cl
#include "$HIP/scripts/ocl/include/biogrowth.cl"

#bind point  &P                    float3
#bind point   is_tree_node         int    read
#bind point   is_tip               int    read
#bind point   parent_id            int    read
#bind point   branch_radius        float  read
#bind point  &force_acc           float3

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
    if (@is_tree_node < 0.5) return;
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

            // Distance check FIRST (cheap: ~6 FLOPs). Most candidates in the
            // 27-cell window fail this and skip the expensive ancestor walk.
            float3 P_other = vload3(other, all_P);
            float3 delta   = P_me - P_other;
            float  d2      = dot(delta, delta);
            float  r_sum   = (r_me + all_r[other]) * @self_collide_range;
            if (d2 >= r_sum * r_sum) continue;

            // Bidirectional ancestor walk (only for candidates in range).
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
```

Note: force is accumulated as a sum across all neighbours (not unit-normalized) because multiple nearby branches should push harder than one. This is a documented deviation from the strict unit-dir force contract — same opt-out density/photo already take.

## Grid eligibility under vigor

Interior tree nodes are filtered out of the grid on their non-eligible frames (vigor Stage 1). This means self_collide only "sees" interior nodes on their duty-cycle frames. Consequences:

- **Trunk-level collision is coarse-grained.** A tip passing near the trunk will only feel it on frames where the trunk nodes are grid-eligible (every 1-4 frames at default vigor parms). For most cases this is fine — tips don't cross trunks in one frame.
- **If visually problematic**, add a `self_collide_override_vigor` toggle that disables the interior filter for collision. Would require grid_count / grid_scatter to emit two grids (filtered + unfiltered) — extra memory. Defer until evidence this actually matters.

## Pipeline

```
… attract → surface → density → photo → self_collide → integrate → cull → vigor_accumulate → topology …
```

All four soft contributors before integrate are commutative (atomic-add to `acc_*`). Ordering is cosmetic — current convention is spatial tropism first, then thickness-aware collision last before integrate.

## Staging

### Stage 0 — schema (no behaviour change)
- Add `parent_id` (int, -1) and `branch_radius` (float, `chf("base_radius")`) to `set_seed`.
- Update `set_attractor` defensively: zero-init both.
- Update topology mainline spawn: `child.parent_id = @ptnum`, `child.branch_radius = f@branch_radius`.
- Update topology lateral spawn: `child.parent_id = @ptnum`, `child.branch_radius = f@branch_radius * chf("taper_decay")`.
- Promote solver parms: `base_radius` (float, default 0.05), `taper_decay` (float, default 0.8).
- Verify: sim output identical to vigor baseline (new attribs exist but are inert).
- **Commit as** `refactor(self_collide): schema — parent_id, branch_radius (no-op)`.

### Stage 1 — naive self_collide (no ancestor exclusion)
- Write `biogrowth_self_collide.cl` kernel as above but with `ancestor_hops = 0` hardcoded.
- Wire it between `photo` and `integrate` in the solver.
- Promote `self_collide_strength` on the solver.
- Verify: tips visibly repel from every nearby node, including their own parent segment. Tips should "flinch" away from their own trunk on spawn. This is the expected artifact — confirms the grid search + force write.
- **Commit as** `feat(self_collide): naive nearest-node repulsion (pre-ancestor-walk)`.

### Stage 2 — ancestor walk
- Change `ancestor_hops` default to 4.
- Add the bidirectional parent_id walk loops (code in the kernel block above).
- Verify: the flinch artifact disappears. Growth looks natural near trunk; tips still repel from distant older branches of the same tree. Crank `self_collide_strength` to confirm the force is actually doing work.
- **Commit as** `feat(self_collide): bidirectional parent_id ancestor walk`.

### Stage 3 — cleanup + docs
- Sync `hou/scripts/ocl/sop/biogrowth_self_collide.cl` tracking copy.
- Update `doc/network.md` with the kernel and new attribs.
- Update `project_biogrowth.md` memory.
- **Commit as** `docs(self_collide): network + memory sync`.

## Numerical / tuning notes

- **Effective range must fit the grid window: `base_radius × self_collide_range ≤ perception_radius`.** Otherwise the 27-cell search misses candidates within the buffer. Recommended: `base_radius × self_collide_range ≤ 0.5 × perception_radius` so taper doesn't push any candidate outside the window. Enforced via a Python pre-sim warning log (not a hard clamp — artists may want to push it and see what happens).
- **`ancestor_hops = 4`** is a compromise. Too low: immediate lineage leaks (tip repels from parent). Too high: distant ancestors (which should be repelled) get excluded. Test scenes with long straight trunks — if a tip at depth 10 on the same trunk doesn't repel from depth 2, bump the hop count or add a distance fallback (skip only if chain-distance AND geometric-distance are both small).
- **Force magnitude is unbounded in dense tangles.** A node in the middle of a dense cluster could sum dozens of repulsion vectors, producing huge `acc_*`. Integrate's `max_speed` clamp is the backstop, but it's worth keeping `self_collide_strength` modest by default.
- **Taper artifact under geometric decay (v1).** `base_radius * pow(taper_decay, split_depth)` means two laterals from a depth-5 parent get the same radius regardless of how many children the parent actually has. A sparsely-branched parent looks indistinguishable from a densely-branched one. Murray's rule (Future Extensions) is the proper fix; v1 accepts the artifact because it's cosmetic and only visible in scenes with wildly uneven branching.
- **Edge case: thin branch crossing between sampled nodes.** Point-distance can miss a thin branch if the nearest grid node is farther than one segment length away. Vigor's grid filter makes this worse (interior nodes are sparse in the grid). Upgrade to closest-point-on-segment if it shows up visually — iterate over polyline prims in the neighbour cells, not just points. Deferred until evidence.

## Future extensions (out of scope)

- **Murray's rule radius propagation (planned follow-up).** Replace the geometric-per-depth taper with `r_parent² = sum(r_child²)` enforced via a post-topology pass that walks tips→root. More biologically faithful — trunks grow thick only where they actually feed many children, not just because they sit at depth 0. Geometric taper is a stand-in; Murray's is the target end state. Deferred purely because (a) it requires a per-frame tree walk (cheap at current scale but new code), (b) we want to see geometric taper break under real scenes before committing to the upgrade. Tracked in `doc/todo.md`.
- **Closest-point-on-segment distance.** Replace point-point distance with point-on-polyline-segment distance for thin-branch crossings. Would need neighbour prim iteration in the kernel.
- **Vigor-aware collision.** When a tip is repelled into stasis, let the pressure accumulate and spawn a lateral that escapes the tangle. Couples self_collide to vigor's lateral_pressure — nice emergent behaviour, but load of tuning. Revisit after baseline self_collide lands and feels right.

## Open questions

- **Should self-collide apply to interior nodes too?** Today only tips integrate, so only tips move under `acc_*`. Interior nodes getting a repulsion force would be wasted computation (never consumed). But tips approaching interior nodes need the interior's radius for the pair metric — that's read-only, so interior nodes don't need to be in the kernel's writable path. Current plan: tips-only early-return is correct.
- **Integration with vigor's grid filter: override?** Deferred (see "Grid eligibility under vigor" above). Default behaviour is to respect the filter; add override only if visual problems show up.
