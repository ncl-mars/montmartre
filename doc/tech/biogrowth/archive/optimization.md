# Optimization — Retired & Archived Mechanisms

This doc collects optimization logic that was once part of BioGrowth but has been removed. Preserved so the mechanisms can be re-introduced if future pipeline changes make them worthwhile again.

Each section stands alone: status, what it was, why it was removed, when to reconsider, recipe to rebuild.

---

# 1. grid_dirty — skip uniform-grid rebuild on idle frames

**Status:** removed during the physics refactor (feature/physics, Stage 1).
**Last present in code:** commit `bf1d2b3` (`fix(cull): guard stall state against empty 27-cell searches`) on branch `feature/vdb-constraints`.

---

## What it was

A 1-element detail int array (`i[]@grid_dirty`) used as a boolean gate to skip the uniform-grid rebuild on frames where nothing structural had changed. Under the pre-physics direct-step model, tree nodes only moved during "walking" (the `is_arrived=0 → is_arrived=1` transition inside `grow`), and new nodes only appeared on spawn events. Many frames had neither happening for some tips, so gating the rebuild saved work.

Under the physics model, tips move every frame under the integrator, so the gate is always-true and the optimization is moot.

---

## How it worked

**State:** single-element detail array `grid_dirty[0] ∈ {0, 1}`, persistent across frames via the solver feedback.

**Setters (wrote `1`):**
- `set_grid_dirty` (outside the solver, frame 1 init on the seed stream) — ensured the first frame rebuilt.
- `grow` on arrival (inside the solver) — when a walking tip reached its target, `atomic_xchg(&grid_dirty[0], 1)`.
- `topology` on spawn (inside the solver) — new polyline point created → `setdetailattrib(0, "grid_dirty", array(1), "set")`.

**Clearer (wrote `0`):**
- `grid_dirty` wrangle inside the solver, positioned after `grid_scatter`. Single line: `i[]@grid_dirty[0] = 0;`. Ran every frame, marking the grid as clean immediately after rebuild.

**Guards (early-returned on `grid_dirty[0] < 1`):**
- `grid_count` (OCL) — skipped the per-point `atomic_inc(cell_count[cell])` pass.
- `prefix_sum` (VEX detail wrangle) — skipped the serial exclusive scan.
- `grid_scatter` (OCL) — skipped the per-point cursor-write pass.

**Non-guards:**
- `grid_bounds` always ran — it reinited the grid arrays to zero each frame, and the bbox could shift as walking tips moved.
- `attract`, `grow`, `cull` had no guard — on non-dirty frames the grid arrays were zero (reinited but not rebuilt), and those kernels naturally found nothing in their neighbor searches. The walking-only path through `grow` (the `is_arrived=0` branch) kept tips moving without needing the grid.

**Cross-frame persistence:** `del_grid_attr` stripped the transient grid arrays (`cell_count`, `cell_start`, `cell_offset`, `sorted_idx`, `cell_size`, `total_cells`, `grid_min`, `grid_res`) but **not** `grid_dirty`, so its value survived feedback into the next frame.

---

## Why it was removed

1. **Semantically dead under physics.** Integrate writes `grid_dirty = 1` every frame (tips always move), so the gate's early-return never triggers. A guard that never fires is misleading code.

2. **High maintenance surface for zero behaviour.** Six code sites had to coordinate: `set_grid_dirty` outside solver, the clear wrangle inside, three OCL guards, one `atomic_xchg` in integrate, plus the `del_grid_attr` carve-out. All of that to keep a flag that's always set.

3. **Perf win was already negligible at our scale.** At ~225k points, the rebuild cost is three cheap passes: `grid_count` (parallel atomic_inc, sub-ms), `grid_scatter` (parallel cursor-write, sub-ms), and `prefix_sum` (serial VEX over total_cells — a few hundred thousand ops max in our bbox/cell_size regime, still sub-ms). The dominant pipeline kernel is `attract`, which runs regardless. User confirmed "I don't see any improvement performance-wise anyway" when evaluating removal.

4. **Re-adding is cheap.** The mechanism is small and well-understood; this doc + git history covers the full rebuild recipe.

---

## When to consider re-adding

Bring it back if any of these become true:

- **Grid rebuild shows up as a measurable bottleneck** in frame timing. Today it's dominated by attract's 27-cell neighbor search; if that changes (attract gets a spatial-hash rewrite, or node count grows 10×), `prefix_sum`'s serial scan could start mattering.
- **A mode exists where tips legitimately don't move for many frames.** E.g., a "pause growth" state, a heavy post-growth relaxation pass that runs inside the solver, or a simulation with very high damping that frequently stalls the whole tree.
- **`total_cells` grows drastically.** If the attractor bbox expands or `perception_radius` shrinks, `total_cells = grid_res.x * grid_res.y * grid_res.z` can blow up — then `prefix_sum` (single-threaded VEX) becomes costly linear in that count. (Before re-adding the gate, prefer either an OCL prefix scan or a looser `cell_size`.)

---

## Recipe to reintroduce

If you need it back:

1. **Schema.** Add `i[]@grid_dirty` (length 1) as a detail attribute on the seed stream. Must be persistent — exclude it from `del_grid_attr`.
2. **Init.** New `set_grid_dirty` Attribute Wrangle (detail) outside the solver, on the seed branch: `i[]@grid_dirty = array(1);`.
3. **Clear step.** New Attribute Wrangle (detail) inside the solver, after `grid_scatter` and before `attract`: `i[]@grid_dirty[0] = 0;`.
4. **Setters.** Every kernel that mutates grid-relevant state (integrate moving tips, topology spawning nodes) must flip the flag:
   ```cl
   // In OCL: bind `&grid_dirty int[]`, then
   __global int* gd = (__global int*)@grid_dirty.data;
   atomic_xchg(&gd[0], 1);
   ```
   ```vex
   // In VEX detail/point wrangles on spawn:
   int gd[] = array(1);
   setdetailattrib(0, "grid_dirty", gd, "set");
   ```
5. **Guards.** Add `if (@grid_dirty[0] < 1) return;` / VEX equivalent early-return to `grid_count`, `prefix_sum`, `grid_scatter`.
6. **Preserve on feedback.** Leave `grid_dirty` out of the `del_grid_attr` stripped list.

**Gotcha:** `grid_bounds` must still reinit the transient grid arrays to zero every frame, even when the gate is off. The `cell_count[]` etc. arrays live only within a single frame's rebuild; if they survive, `atomic_inc` on the next rebuild will double-count.

**Gotcha:** any new contributor that moves tree nodes must also flip the flag, or the gate will stick off and the grid will go stale. This coupling between "who can move things" and "who knows to mark the grid dirty" is part of why the gate was a maintenance cost.

---

## Related context

- Physics refactor plan: [biogrowth-plan.md](../biogrowth-plan.md)
- Live network documentation: [network.md](../network.md)
- The original motivation traced back to observed frame times where non-dirty frames (no growth events) were measurably cheaper than dirty frames; under the pre-physics model, entire frames could pass with no tips arriving or spawning, especially late in a sim as most tips were culled.

---

# 2. Attractor stall-kill — cull orphans that tips can't reach

**Status:** removed 2026-04-19 on `feature/vdb-constraints`.
**Last present:** immediately before this commit (same branch).

Preserved in case Phase 2 VDB obstacle constraints cause meaningful "trapped attractor" accumulation (tips deflected around obstacles, never reaching attractors behind them).

---

## What it was

A secondary kill path in `cull` (in addition to `kill_radius`): increment `stall_count` each frame the attractor's closest tree-node distance failed to decrease; when `stall_count >= stall_frames`, kill the attractor.

Three per-attractor attributes and one solver parm:
- `f@closest_dist` — latest min distance to any tree-node in the 27-cell search.
- `f@prev_closest_dist` — value from the previous frame.
- `i@stall_count` — consecutive frames without progress.
- `ch("stall_frames")` (int, default 10) — threshold.

Initialized in `set_attractor`: `closest_dist = prev_closest_dist = 1e38; stall_count = 0`.

## Why it was added (and why it became obsolete)

**Added** with the direct-step model to prevent static/quasi-static tips from endlessly adding children frame after frame — `stall_frames` shut down the growth front in regions tips had stopped advancing.

**Obsolete** because the physics refactor added `arc_since_spawn` to `topology`: children spawn only when a tip has traveled `step_size` of arc length. Static or near-static tips no longer spawn, which removes the original rationale.

## Why it was removed

- No longer load-bearing — `arc_since_spawn` handles the "no spawn while static" concern at the source.
- The secondary role (killing orphan attractors tips can't reach) is cheap to leave alive: orphans just sit in the grid; `attract` early-returns for them.
- Saves three attribs across every attractor (potentially millions in the 5–10M scatter target).
- Saves the bookkeeping cost per cull pass (distance track, prev-comparison, counter, threshold test).
- Simplifies cull to a single concern: `kill_radius` proximity.

## When to consider re-adding

- **VDB obstacle constraints trap attractors.** If Phase 2 contributors cause tips to deflect around obstacles and never reach attractors behind them, orphans accumulate in the grid and bloat `total_cells` / `grid_res`. Stall would progressively cull them and shrink the grid.
- **Sparse attractor regime.** 5M+ attractors scattered in a volume where the tree only fills a corner — orphan fraction high, grid bbox wasteful, stall is the natural cleanup.
- **Alternative before re-adding:** periodic GC (every N frames, kill attractors with `att_count == 0` for > M consecutive frames) — cheaper per-frame than continuous stall tracking.

## Recipe to reintroduce

1. **Attribs.** In `set_attractor` add `f@closest_dist = 1e38; f@prev_closest_dist = 1e38; i@stall_count = 0;`.
2. **Cull bindings.** Add `&closest_dist float`, `&prev_closest_dist float`, `&stall_count int` to `biogrowth_cull.cl`; add `#bind parm stall_frames int val=10`.
3. **Cull logic.** Inside the 27-cell search, track `best_d2 = min(best_d2, d2)` alongside the existing `kill_radius` early-return. After the search:
   ```cl
   if (best_d2 < FLT_MAX) {
       float cdist = sqrt(best_d2);
       @closest_dist.set(cdist);
       if (cdist >= @prev_closest_dist) @stall_count.set(@stall_count + 1);
       else                              @stall_count.set(0);
       if (@stall_count >= @stall_frames) @active.set(0);
       @prev_closest_dist.set(cdist);
   }
   ```
4. **Empty-neighborhood guard.** The `if (best_d2 < FLT_MAX)` test is important — without it, attractors with no tree-nodes in their 27 cells would tick `stall_count` against a stale `prev_closest_dist` of `1e38`, never progressing. Guard was the fix for a bug landed in `bf1d2b3`.
5. **Solver parm.** Promote `stall_frames` to a solver spare parm.
