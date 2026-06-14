# Performance Audit

Post-`branch-autonomy` checkpoint. All core biological rules have landed (phyllotaxis, per-node cap, Leonardo, Murray, reaction wood, apical dominance, branch autonomy, allometric). Goal: baseline the solver, identify bottlenecks, land targeted optimizations before stacking more rules.

## Test scenes

Three fixed scenes to make runs comparable:

1. **Small** — ~5k attractors, seed → grow to ~5k tree_nodes (~3–5s runtime at 24fps). Fast iteration.
2. **Medium** — ~100k attractors → ~50k tree_nodes. Representative of everyday authoring.
3. **Large** — ~1M+ attractors → stop at ~500k tree_nodes. Stress test; where non-linear blowups surface.

Each scene saved as `hou/perf/perf_small.hip`, `perf_medium.hip`, `perf_large.hip` (or screenshots + parm dumps if lighter). Fixed seed, fixed random streams, fixed frame count — runs reproducible across optimization passes.

Hardware: record GPU / CPU / RAM once at the top of results.

## Measurement protocol

For each scene, record:

- **Total frame time** (Houdini Playbar timer, avg of last 10 frames at the measurement checkpoint).
- **Per-node kernel ms** (Houdini Performance Monitor → Solver DOP → expand OpenCL nodes).
- **Point counts** per frame (tree_nodes, attractors, total points).
- **Memory** (VRAM if visible, main RAM via Task Manager snapshot).

Checkpoints: frame 50, 100, 200, 500 — catches both startup and steady-state.

One measurement = all features **enabled** with default parms. Then do the A/B pass below to attribute cost.

## Baseline runs

**Run 0: everything off.**
Switch-SOPs set to bypass: `vigor_enable`, `surface_enable`, `density_enable`, `photo_enable`, `self_collide_enable`, `murray_enable`, `dominance_enable`. In-kernel: `allometric_enable=0`, `reaction_wood_enable=0`, `branch_autonomy_enable=0`, `phyllo_enable=0`, `leonardo_enable=0`, `vigor_filter_enable=0`. Pure Euler integration + attract + topology.

Establishes the floor cost of the SCA core.

**Run N: one feature at a time.** Enable a single feature, rerun, measure delta. Features to test solo:

- `vigor_filter_enable` — grid filter
- `surface_enable` / `density_enable` / `photo_enable` — VDB soft contributors (each with its bus wired)
- `self_collide_enable` — 27-cell repulsion
- `murray_enable` — radius propagation
- `dominance_enable` — apical dominance
- `allometric_enable` — growth-factor scaling (in-kernel)
- `reaction_wood_enable` — Murray bias (in-kernel)
- `branch_autonomy_enable` + irradiance VDB wired — light credit scaling (in-kernel)
- `phyllo_enable`, `leonardo_enable` — lateral orientation (VEX-side)

**Run ALL: everything on.** Compare sum-of-deltas to actual all-on cost. Differences expose synergies (rare) or redundant overhead (common).

## Suspected hot spots

Priority order to investigate when A/B exposes a bottleneck:

1. **`attract`** — 27-cell neighbor search per attractor. Scales with `attractor_count × average_cell_occupancy`. Large scenes → millions of cell lookups / frame.
2. **`self_collide`** — 27-cell per tip × ancestor walk per candidate. Grows with `tip_count × neighbor_density × ancestor_hops`. Can dominate when tips crowd.
3. **`resolve_sid_scatter` / `resolve_parent_id`** — linear in *total* points (tree + attractors). Grows with the whole stream, not just the tree.
4. **`topology`** — VEX wrangle with many `setpointattrib` calls + `addpoint` / `addvertex`. VEX wrangles are slower per-point than OCL; if topology fires on many points per frame it becomes the bottleneck.
5. **`integrate`** — fused but cost scales with bound VDB count (each `worldSample` is not free). More constraints buses = more per-tip cost.
6. **`grid_count` / `grid_scatter` / `prefix_sum`** — linear scans + atomics. Usually cheap but scale linearly with total points.
7. **`blast_unactive`** — point deletion in Houdini is not free; cost spikes on cull-heavy frames.
8. **VDB sampling inside integrate** — `collision_sdf.worldSample` + `worldGradient` + `avoid_weight` + `response_type` + `irradiance` can stack. Each extra VDB bound = extra per-tip cost.

## Landed optimizations

### Split-streams refactor — entire stable-id chain deleted (2026-04-25)

After landing the `resize_sid_lookup` preallocation, the new top of the profile was the rest of the sid chain (`resolve_sid_scatter`, `assign_sids`, `resolve_parent_id`) plus `cull`. We were planning targeted optimizations (skip attractors in scatter, migrate `assign_sids` to OCL) when the better answer surfaced: separate the streams entirely so the sid chain becomes unnecessary.

**Insight.** The whole stable_id machinery existed to keep `parent_id` valid across `blast_unactive` deleting attractors and shifting tree_node ptnums. If tree_nodes never share a stream with attractors during topology / never get deleted, their ptnums never shift, and `parent_id` stored as a raw ptnum at spawn stays valid forever.

**Refactor.** Inside the solver, after `attract`, split into tree-only stream + attractor-only stream. All tree-only kernels (surface, density, photo, self_collide, integrate, murray, dominance, topology) run on the tree stream. Cull + blast_unactive run on the attractor stream. Re-merge tree-first at the end. Topology appends children at the end of the tree-only ptnum range, preserved through the merge.

**Deleted entirely:**
- Nodes: `assign_sids`, `resize_sid_lookup`, `resolve_sid_scatter`, `resolve_parent_id`.
- Setup: `set_stable_counter` `next_stable_id` init + `sid_to_ptnum` preallocation.
- Attributes: `i@stable_id`, `i@parent_stable_id`, `i[]@sid_to_ptnum` detail, `i@next_stable_id` detail counter.

**Modified:**
- `cull` runs on attractor-only stream; reads tree positions via cross-input bind `#bind point tree_P name=P float3 read input=1` and `vload3(node_idx, @tree_P.data)`.
- `blast_unactive` group guarded `active==0 && is_attractor==1` — invariant enforcement: no tree_node ever gets deleted.
- Topology comment block locks the no-delete-no-reorder invariant.

**Impact.** 800k tree_nodes simulating in ~1 second per frame at frame 240 (was multi-second pre-refactor). The entire sid chain timing budget — previously the dominant fraction of frame time — is now zero. Net architectural simplification: smaller schema, shorter pipeline, fewer kernels, no per-frame ptnum-rebuild cost.

**Supersedes the planned `assign_sids` → OCL migration** documented in earlier "Known hotspots". That plan is now obsolete (the kernel doesn't exist).

### `resize_sid_lookup` — deleted (preallocation outside solver)

**Symptom:** node dominated the profiler at >250 tree_nodes, scaling weirdly with total point count (tree + attractors). Arithmetic was trivial (one `len()` + conditional `resize()`) — cost was in VEX-wrangle-touches-detail-array machinery.

**Investigations that didn't help:**
- Amortized doubling of the resize target (`max(n, cur*2)` instead of exact `n`) — no improvement. Confirms the cost wasn't the reallocation itself.
- Making `sid_to_ptnum` persistent across frames (removing from `del_grid_attr` strip list) — also didn't help.

**Fix that worked:** pre-allocate `sid_to_ptnum` once outside the solver to a fixed size, delete the resize wrangle entirely. One-time upstream wrangle (piggybacked on `set_stable_counter`):
```vex
i@next_stable_id = 1;
resize(i[]@sid_to_ptnum, chi("max_stable_ids"));   // e.g. 2,000,000
```
Solver graph no longer contains `biogrowth_resize_sid_lookup`. Zero per-frame cost for the resize path. Trade: ~8 MB of always-allocated int array for 2M entries — negligible (a 2K RGBA float image is 4× larger). Overflow guardrail: `resolve_sid_scatter` writes silently fail on out-of-bounds; if a sim exceeds `max_stable_ids`, bump and restart.

**Impact:** removed as top-1 hotspot; new top-3 are `resolve_sid_scatter`, `assign_sids`, `cull` — see below.

## Known hotspots (observed)

### `resize_sid_lookup` detail wrangle

Dominant cost once scene grows past ~250 tree_nodes. The arithmetic is trivial (one `len()` check + conditional `resize()`) — cost is in VEX-wrangle-touches-detail-array machinery. Amortized doubling of the resize target didn't help (confirmed: cost isn't the allocation). Making the array persistent across frames (removing from `del_grid_attr` strip list) also didn't help.

**Proposed fix: delete the node, pre-allocate `sid_to_ptnum` once outside the solver.**
- One-time upstream wrangle: `resize(i[]@sid_to_ptnum, chi("max_stable_ids"));` with a generous fixed size (e.g. 2M entries = ~8 MB).
- Remove `biogrowth_resize_sid_lookup` from the solver graph entirely.
- `resolve_sid_scatter` + `resolve_parent_id` work unchanged — they write/read into preallocated slots.
- Overflow guardrail: if sim exceeds `max_stable_ids`, scatter writes silently fail on out-of-bounds; bump the number and restart.
- Expose `max_stable_ids` as a solver spare parm (or outside-solver wrangle parm) for per-scene tuning.

Fallback if preallocation is too rigid: migrate the wrangle to an OCL detail-write kernel (`#bind detail &sid_to_ptnum int[]`). Bypasses VEX-wrangle overhead, keeps dynamic sizing.

### Stable-id management — superseded by split-streams refactor

(See "Landed optimizations" above. The whole sid chain is gone — these targeted optimizations are no longer needed.)

## Expected quick wins

Without measuring, candidate optimizations ranked by likely payoff:

1. **Skip `resolve_sid_scatter` / `resolve_parent_id` when nothing changed.** If no new points spawned and no deletions happened, `parent_id` is still valid from last frame. A single "dirty" flag set by topology / blast → short-circuit the resolve chain. Re-introduces the `grid_dirty` pattern for the sid chain.
2. **`ancestor_hops` tuning.** Currently default 4. Measure whether hops 3 or 2 produce visibly different self-collide behavior at typical tip densities. Each hop saved = fewer pointer chases.
3. **VDB binds pruning.** If a scene doesn't use `avoid_weight` or `response_type`, the kernel still checks `.bound` cheaply — but the import_collision OM is still evaluated upstream. Skip wiring the OM entirely when not needed.
4. **Murray / dominance finalize fusion?** They run back-to-back, both iterate all tree_nodes, both read + write a per-node attribute. Could fuse into one kernel (`finalize_all`) that handles both, halving dispatch. Only if measurement says it's worth the code churn.
5. **Topology → OCL migration.** Biggest unknown. Topology is currently VEX for flexibility (it adds points, edits polylines). Migration to OCL would require a rewrite but likely halves topology cost. Measure first; defer unless it's clearly the bottleneck.
6. **Per-feature kernel bypass when unused.** Switch-SOPs already handle most; verify no hidden always-run paths.

## Anti-goals

- Don't chase micro-optimizations before measuring. Cache line layout, loop unrolling, atomic strength reduction — all worth < 5% and only matter if the kernel is already compute-bound.
- Don't fuse kernels aggressively. The vigor + avoid + collision fusions into integrate paid off because they shared state. Fusing unrelated rules pollutes the kernel and makes debugging harder.
- Don't delete the Switch-SOP enables. They remain the single biggest authoring affordance — turning off any feature is a one-click test.

## Output artifacts

- **`doc/performance.md`** — this file, updated with each run's numbers.
- **`doc/performance-results.md`** — raw tables (per-scene, per-feature, before/after each optimization). Grows across commits.
- **Commit style on this branch:** one optimization per commit. Commit message format:
  ```
  perf: <area> — <optimization>

  Medium scene, frame 200:
    before: XX ms/frame
    after:  YY ms/frame  (−NN%)

  Large scene, frame 500:
    before: AA ms/frame
    after:  BB ms/frame  (−MM%)
  ```

## Staging

1. Set up the three perf scenes + record hardware.
2. Run baseline + all-on, one A/B pass. Log numbers.
3. Read Performance Monitor — pick top-3 kernels by total time.
4. Investigate each: measure, hypothesize, implement, re-measure.
5. Land one optimization per commit.
6. Merge back to `develop` when numbers stabilize.
