# Vigor & Lateral Branching — Plan

Adds the horticultural concept of **vigor** to the SCA: tree nodes track their bifurcation depth, and younger (low-depth) lineages are both **more visible to attractors** (grid filter) and **more reactive when picked** (lower lateral-branch threshold). Produces an emergent heading region — the growth frontier dominates, interior segments become quiescent — without scripting it globally.

In horticulture, *vigor* is "growth potential of a meristem"; it declines with tissue age / distance from the apex. Young wood is vigorous, old wood is exhausted. This plan encodes that directly: `split_depth` is the raw bookkeeping, "vigor" is the emergent behaviour the thresholds produce.

Sits on top of the landed physics model. Complementary to `vdb-constraints.md` (soft-force contributors). Branch: TBD (likely `feature/vigor`, branched from `develop` once VDB constraints land).

## Context

Today only tips attract (`attract` skips `is_tip<1` candidates). This cleanly implements classical SCA: the growth front consumes attractors as it advances, interior segments are frozen in place. But it also means:
- **No lateral branching.** Once a node stops being a tip (after spawning its child), it's dead to attract forever. The tree can only grow from its current leaf set.
- **Uniform frontier reactivity.** Every tip behaves identically regardless of how deep in the tree it sits. A tip three bifurcations from the seed reacts the same as a tip ten bifurcations in.

This plan adds two coupled mechanisms, both gated on `split_depth`:

1. **Grid-participation filter** — interior nodes can re-enter attract's search space, throttled by their bifurcation depth. Deep nodes appear in the grid only on periodic frames; shallow nodes every frame. This both enables lateral branching and prevents old trunk from flooding the grid.

2. **Vigor-gated lateral spawn** — an interior node that accumulates enough `acc_*` pull sprouts a lateral tip. The threshold for sprouting scales with bifurcation depth — young nodes sprout readily, deep nodes are dormant.

Together: the frontier is *present* in the grid and *eager* to react; the interior is *sparse* in the grid and *resistant* to react. Both gates push in the same direction, producing a strong heading region that self-organizes from local rules.

## Core concept: split_depth

A per-node int attribute tracking **true bifurcations only**, not mainline continuation.

- Seed: `split_depth = 0`.
- Arc-length mainline spawn (tip's rhythmic step): child inherits `split_depth` from parent unchanged. This is continuation of the same lineage, not a branch event.
- Lateral spawn (new rule, this plan): the sprouting interior node keeps its own `split_depth`; the new lateral tip gets `split_depth = parent.split_depth + 1`.

**Why track bifurcations, not arc-spawns:** a tip that grows straight for 100 arc-spawns is still one lineage. Incrementing depth every arc-spawn would age nodes by distance traveled, not by topological branching — which decorrelates from the vigor intuition. Depth should answer "how many times has this line split off from its ancestor?"

## Mechanism 1 — Grid-participation filter

### Eligibility rule

Both `grid_count` and `grid_scatter` guard their per-point work by the same predicate:

```cl
inline bool bg_grid_eligible(int split_depth, int frame) {
    int period = 1 << min(split_depth, VIGOR_DEPTH_CAP);  // 1, 2, 4, 8, 16, ... capped
    int phase  = split_depth * VIGOR_PHASE_STRIDE;        // stagger across frames
    return ((frame + phase) % period) == 0;
}
```

- Depth 0 → every frame.
- Depth 1 → every 2 frames.
- Depth 2 → every 4 frames.
- ...
- Depth ≥ cap → every `2^cap` frames (effectively dormant).

`VIGOR_PHASE_STRIDE` staggers *which* depth-N nodes appear on which frame, so you don't get population pulses (e.g. "all depth-3 nodes pop in frame 8, then vanish for 7 frames").

Tips remain a special case: `is_tip ? true : bg_grid_eligible(split_depth, frame)` — a tip always participates regardless of its depth, because the growth frontier is by definition maximally vigorous.

### Why in grid_count / grid_scatter and not in attract

Putting the filter in attract would mean the 27-cell loop still walks non-participating nodes (costs the main loop budget). Putting it in grid construction means those nodes simply aren't in the grid — attract's hot loop is unchanged and gets smaller candidate sets for free.

Both kernels must apply the *exact same* predicate: `grid_scatter` uses the offsets laid out by `grid_count` + `prefix_sum`, so disagreement corrupts the grid.

### Tuning parms

- `vigor_depth_cap` (int, default 4) — max period exponent. Depth ≥ cap means the node participates every `2^cap` frames.
- `vigor_phase_stride` (int, default 3) — phase stagger coefficient.
- `vigor_filter_enable` (toggle, default on) — disable the filter for A/B comparison with classical tips-only SCA.

## Mechanism 2 — Vigor-gated lateral spawn

### Trigger

An interior node (`is_tip == 0`) accumulates `acc_*` force from attract each frame it's grid-eligible. When the magnitude of its accumulated pull crosses a depth-scaled threshold, it sprouts a lateral tip.

**Accumulation state.** Introduce `f@lateral_pressure` on every tree node, incremented by a dedicated `vigor_accumulate` OCL kernel placed between `integrate` and `topology`:
```
signal = bg_decode_dir(att_ix, att_iy, att_iz)   // RAW attract sum, no averaging
lateral_pressure += length(signal) * dt
```
The accumulation must run in its own kernel (not inside integrate) because integrate's `is_tip<0.5` early-return is load-bearing — topology's classical SCA interior-branching path reads interior `att_*` after integrate, so integrate cannot touch non-tip state. The new kernel only *reads* `att_*` and does not reset it; topology's existing path still consumes those accumulators as before. Stage 3 will swap topology's trigger for a `lateral_pressure` threshold and take over the reset.

Signal source is `att_*` (attract's accumulator), not `acc_*`, because soft contributors (surface/density/photo) early-return on `is_tip<0.5` and so write zero to interior `acc_*`. Attract is the only writer to interior accumulators today.

No averaging by `att_count`: a node picked by more attractors legitimately *should* heat up faster — that's the "hotspot deserves to branch" intent.

**Threshold — vigor curve.**
```
branch_threshold(d) = lateral_base_threshold * (1 + d * vigor_penalty)
```
Low depth → low threshold → sprouts readily (high vigor). High depth → high threshold → dormant (low vigor).

**Spawn condition (in topology, new rule):**
```
if (!is_tip && lateral_pressure >= branch_threshold(split_depth)):
    spawn lateral child:
        P                = parent.P
        v                = normalize(last_att_force) * |parent.v| * lateral_velocity_inherit
        is_tip           = 1
        split_depth      = parent.split_depth + 1
        lateral_pressure = 0
    parent.lateral_pressure = 0
```

The parent remains non-tip (it didn't become a tip again — it just sprouted one). The new lateral tip inherits velocity *direction* from the last accumulated pull, not from the parent's own drift velocity — otherwise the lateral would shoot in the parent's mainline direction, defeating the point.

### Why acc_* magnitude as the signal

The integrator already reads `acc_*` every frame. No extra sample. And the magnitude has physical meaning: "attractors were pulling hard on this segment this frame." Accumulating over time (rather than a single-frame threshold) filters out attractor noise — a segment needs *sustained* pressure to branch.

### Coupling with the grid filter

A deep node only gets grid-eligible every N frames, so its `acc_*` is zero on non-eligible frames (attract didn't see it). That means `lateral_pressure` accrues on a `1/N`-duty-cycle for deep nodes — additional slowdown beyond the raw threshold scaling. The two mechanisms multiply rather than add, which is intentional: we want the vigor falloff to be steep.

### Tuning parms

- `lateral_base_threshold` (float) — minimum accumulated pull for a depth-0 node to sprout.
- `vigor_penalty` (float) — threshold growth per depth step.
- `lateral_velocity_inherit` (float, 0..1, default 0.5) — how much of the parent's speed the lateral gets.

## Attribute schema delta

**Tree nodes — add:**
- `i@split_depth` (int, seed init 0).
- `f@lateral_pressure` (float, init 0). Interior-only semantically; stays 0 on tips.
- `v@last_att_force` (float3, init 0) — pass-through from integrate to topology so the lateral inherits a sensible initial direction. See Open Questions.

**Tree nodes — no removals.**

**Propagation on spawn:**
- Mainline (tip arc-length spawn, existing rule): child inherits `split_depth` from parent unchanged, `lateral_pressure = 0`, `last_att_force = 0`.
- Lateral (new rule): new tip gets `split_depth = parent.split_depth + 1`, `lateral_pressure = 0`, `last_att_force = 0`; parent keeps its own `split_depth`, resets `lateral_pressure = 0`.

## Per-kernel changes

### `grid_count`
- Guard: `if (@is_tip < 0.5 && !bg_grid_eligible(@split_depth, @Frame)) return;`
- Tips always participate; interior nodes only on their eligible frames.

### `grid_scatter`
- Same guard as `grid_count` (must agree with the counts laid out by prefix_sum).

### `integrate`
- **Unchanged.** Its `is_tip<0.5` early-return is load-bearing for classical SCA interior-branching (topology consumes interior `att_*` after integrate). Moved the vigor bookkeeping to its own kernel (below).

### `vigor_accumulate` (new kernel, between integrate and topology)
- File: `hou/scripts/ocl/sop/biogrowth_vigor_accumulate.cl`.
- Runs over all points; filters on `is_tree_node && !is_tip && att_count>0`.
- Reads raw attract sum from `att_*` (no averaging, no reset), writes `lateral_pressure += |signal|*dt` and `last_att_force = signal`.
- Pipeline: `… integrate → vigor_accumulate → topology → …`.

### `topology`
- Existing arc-length tip-spawn rule unchanged.
- New lateral spawn branch (interior-node wrangle over tree nodes):
  ```vex
  if (i@is_tree_node >= 1 && i@is_tip < 1) {
      float threshold = chf("lateral_base_threshold") * (1.0 + i@split_depth * chf("vigor_penalty"));
      if (f@lateral_pressure >= threshold) {
          int child = addpoint(0, v@P);
          setpointattrib(0, "is_tree_node",     child, 1);
          setpointattrib(0, "is_tip",           child, 1);
          setpointattrib(0, "active",           child, 1);
          setpointattrib(0, "split_depth",      child, i@split_depth + 1);
          setpointattrib(0, "lateral_pressure", child, 0.0);
          setpointattrib(0, "arc_since_spawn",  child, 0.0);

          vector dir = normalize(v@last_att_force);
          float  spd = length(v@velocity) * chf("lateral_velocity_inherit");
          setpointattrib(0, "v",                child, dir * spd);
          setpointattrib(0, "birth_time",       child, @Time);
          addprim(0, "polyline", @ptnum, child);

          f@lateral_pressure = 0;
      }
  }
  ```

### `biogrowth.cl` include
- Add `bg_grid_eligible` inline function. `VIGOR_DEPTH_CAP` and `VIGOR_PHASE_STRIDE` as `#define`s (tunable rebuild-time); promote to parms only if we find we need runtime tuning.

### `set_seed` (outside solver)
- Add `i@split_depth = 0; f@lateral_pressure = 0; v@last_att_force = {0,0,0};`

### `set_attractor` (outside solver)
- Unchanged (attractors don't need vigor attribs, but the compile block may complain — add zero-inits defensively if so).

## Staging

### Stage 0 — schema + depth tracking, no behaviour change
- Add `split_depth` (int, seed=0), `lateral_pressure` (float, 0), `last_att_force` (v@, 0) to `set_seed` and defensively to `set_attractor`.
- Update mainline topology spawn: child inherits `split_depth` from parent.
- Verify: sim output identical to pre-plan baseline (new attribs exist but are inert).
- **Commit as** `refactor: vigor schema (split_depth, lateral_pressure; no-op)`.

### Stage 1 — grid filter
- Implement `bg_grid_eligible` in `biogrowth.cl`.
- Guard `grid_count` and `grid_scatter` on the predicate.
- Promote `vigor_depth_cap`, `vigor_phase_stride`, `vigor_filter_enable` on the solver.
- Verify: with filter disabled, sim identical to Stage 0. With filter enabled, no lateral branching yet (topology doesn't sprout laterals) but grid density should visibly drop as the tree deepens — confirm via a detail attrib counting grid participants per frame.
- **Commit as** `feat(vigor): depth-cycled grid participation filter`.

### Stage 2 — vigor accumulation
- Add `vigor_accumulate` OCL kernel between `integrate` and `topology`. Reads interior `att_*`, writes `lateral_pressure` and `last_att_force`. Does not reset `att_*`.
- No integrate change. No topology change. Classical SCA interior-branching still fires as before.
- Verify: plotting `lateral_pressure` as point color should show hot interior segments near live attractors, cold ones far from attractors. Deep nodes heat up slower thanks to the Stage 1 grid duty-cycle.
- **Commit as** `feat(vigor): lateral pressure accumulation on interior nodes`.

### Stage 3 — lateral branching
- Topology sprouts laterals when `lateral_pressure >= branch_threshold(split_depth)`.
- Tune `lateral_base_threshold`, `vigor_penalty`, `lateral_velocity_inherit`.
- Verify: trees develop laterals off interior segments; deep lineages stay quiescent; frontier dominates.
- **Commit as** `feat(vigor): lateral branching on depth-scaled pressure threshold`.

### Stage 4 — cleanup
- Sync tracking copies in `hou/scripts/ocl/sop/*.cl`.
- Update `doc/network.md` with the new attribs, kernels, and vigor mechanisms.
- Update `project_biogrowth.md` memory.
- **Commit as** `docs: vigor pipeline, tracking copy sync`.

## Open questions

- **`last_att_force` vs alternatives.** Integrate resets `acc_*` at end of kernel; topology runs after integrate. To orient a lateral, topology needs the direction pull came from. Options: (a) persist `v@last_att_force` written by integrate, consumed by topology — current plan; (b) defer reset of `acc_*` until after topology runs; (c) move lateral-spawn decision into integrate (but integrate is not supposed to do topology). (a) is cleanest, costs 3 floats per tree node.

- **Mainline spawn and split_depth.** Current plan: mainline (tip arc-length spawn) does *not* increment `split_depth` — same lineage continuing. But the chain grows indefinitely. Do we want a slow "travel ages you" drift (bump depth every K mainline spawns)? Probably no — keeps semantics clean — but worth a test scene with long straight runs.

- **Interaction with cull.** Dead-tip cull today checks `|v| < v_min_tip` and `acc_count == 0`. A dormant interior node (high depth, not grid-eligible most frames) will have `acc_count == 0` on non-eligible frames but it's not a tip, so it's out of cull's scope. Confirm cull's tip filter stays strict.

- **Phase_stride distribution.** `phase = d * stride` is simple but low-depth nodes cluster at frame 0 of their period. An LCG-like hash of `(split_depth, node_id)` would spread them better. Probably overkill for now; revisit if visible pulsing appears.

- **Attractor competition tip vs interior.** Today attract's winner-take-all picks the closest tip. If interior nodes compete for attractors too, an interior node could win, making the tip wait. Is that desirable (closer wins — realistic) or do we bias toward tips? Probably bias-toward-tips: `d2 *= tip_bias_factor` when candidate is a tip (default 0.5, halves tips' effective distance). New parm `tip_bias_factor`.

- **Compile block compatibility.** The filter in `grid_count`/`grid_scatter` reads `@split_depth` and `@is_tip`. Both are per-point ints — should compile cleanly. If compile block issues re-emerge, cast types explicitly before the block.

## Future extensions (out of scope for this plan)

- **Vigor as a continuous age model** — smooth decay + recovery, not just depth-indexed threshold. Could couple to `birth_time` for real elapsed-time aging.
- **Auxin / hormonal analog** — treat `lateral_pressure` as a diffusible quantity traveling along the tree graph (basipetal/acropetal transport). Gives rise to correlated branching waves and more realistic apical dominance dynamics.
- **Dormancy-retraction** — a lateral that sprouts but fails to attract for N frames retracts back into parent (pruning the false start).
- **Per-branch vigor envelope** driven by external vector fields (light, density) — couples vigor to the VDB-constraint pipeline so environmental factors modulate branching rate, not just branching direction.
