# Sweep Meshing — Plan

Turns the solver's polyline output into tubes by sweeping a circular cross-section scaled by each node's `branch_radius` (authored by the self-collide feature). Lives **outside the solver** — the growth sim is unchanged.

Branch: `feature/sweep-meshing` (cut from `develop` 2026-04-20 after `feature/self-collide` merged).

## Context

Until now the viewport showed raw polylines — useful for debugging growth, useless for rendering. `branch_radius` has been authored per-node since Phase 3a and was already shared downstream by design ("authored once, consumed twice"). This phase is the second consumer.

Mesh is rebuilt **every frame** (not gated to final frame). Reasons: live preview during sim iteration, meshing cost is small relative to the solver at current scale, and a per-frame mesh lets the user see bifurcation tapering and self-collide behaviour cosmetically without waiting.

## Design decisions (user-confirmed)

- **Round cross-section, 6 sides.** Cheap; later displacement will hide the low poly count. Column count exposed as a parm for hero/background trade-offs.
- **Attribute Blur on `branch_radius`.** Raw attribute has hard steps at bifurcations (lateral child drops by `taper_decay` instantly). Blur along polylines smooths the visible join without destroying the overall taper profile. Few iterations (2–3) is enough.
- **Lateral marker via `i@is_lateral_start` on the child.** Set in topology's interior-branching spawn block; zero for mainline children and seed. Currently unused by the mesh chain (see "Future extensions: lateral start offset"), but kept as render-time metadata — useful later for varying bark / colour / displacement on laterals vs. mainline.
- **Sweep reads `pscale`.** The Sweep SOP's radius input is `float@pscale` (or `vector@scale`) — it does NOT read an arbitrary attribute name. A tiny point wrangle just before the Sweep aliases `@pscale = @branch_radius`, keeping `branch_radius` intact upstream for self-collide.
- **Per-frame mesh rebuild.** No Switch-If gate. Keep the chain fast by (a) bypassable Resample, (b) bounded Attribute Blur iterations, (c) low poly column count by default.

## Schema delta

**Tree nodes — add:**
- `i@is_lateral_start` (int, seed=0). Written on spawn:
  - Mainline arc-spawn: `child.is_lateral_start = 0`.
  - Lateral spawn: `child.is_lateral_start = 1`.

Defensively zero-init in `set_attractor` to keep compile-block merges happy.

## Mesh chain (outside solver, downstream of `tree_nodes`)

```
tree_nodes
  → Resample (maxlen, optional)                        // smooth polylines before blur
  → blur_radius (AttribBlur on @branch_radius, 2–3 iters, connectivity, pin off)
  → set_pscale (point wrangle: f@pscale = f@branch_radius)
  → Sweep (round, cols=6, reads @pscale automatically)
  → mesh_out
```

### set_pscale (Attribute Wrangle, point)

One line:
```vex
f@pscale = f@branch_radius;
```

Sweep uses `pscale` by convention; aliasing here keeps `branch_radius` unchanged upstream (self-collide keeps reading it in sim).

### Sweep SOP

- Surface Shape: Round Tube
- Columns: `sweep_columns` parm (default 6)
- Radius: driven by `@pscale` (written by the `set_pscale` wrangle above). Sweep does not accept arbitrary attribute names — it looks for `pscale` (float) or `scale` (vector).
- Open polylines → closed tubes (open-ended). Endcaps optional — default off (tips are thin, caps would poke out).

## Parms (promote on a null/OUT node or dedicated controller)

| Parm | Default | Notes |
|---|---|---|
| `resample_enable` | 0 | off by default — solver polylines are already fine-grained |
| `resample_maxlen` | 0.5 × step_size | if resample on |
| `blur_iters` | 3 | Attribute Blur iterations on `branch_radius` |
| `sweep_columns` | 6 | Sweep cross-section resolution |

## Staging

### Stage 0 — schema
- Add `i@is_lateral_start` to `set_seed` (= 0) and `set_attractor` (= 0).
- Update topology:
  - Tip extension block: `setpointattrib(0, "is_lateral_start", child, 0);`
  - Interior branching block: `setpointattrib(0, "is_lateral_start", child, 1);`
- **Commit as** `refactor(sweep): schema — is_lateral_start (no-op)`.

### Stage 1 — mesh chain
- Build the chain outside the solver downstream of `tree_nodes`.
- Add parms above on a dedicated controller null.
- Verify: mesh appears, tapers with depth, lateral forks look non-degenerate.
- **Commit as** `feat(sweep): per-frame tube meshing from branch_radius`.

### Stage 2 — polish + docs
- Sync tracking copies if any OCL involved (none expected).
- Update `doc/network.md` with the downstream mesh chain (outside-solver section).
- Update `project_biogrowth.md` memory: Phase 3b landed.
- Remove "Phase 3b" from `doc/todo.md`.
- **Commit as** `docs(sweep): network + memory sync`.

## Tuning / caveats

- **Blur iterations × polyline length** — too many iterations across a very long trunk will wash out the taper entirely. 2–3 iters is a starting point; if trunks look uniformly thick, drop to 1.
- **Self-intersection at Y-junctions** — the lateral polyline starts exactly at the parent's position, so swept tubes intersect rather than blend. Cosmetic for direct renders; becomes holes if we later VDB-from-polys → Convert VDB. Fix deferred (see "Future extensions: lateral start offset").
- **Performance at 600k+ points** — Sweep SOP is generally fast but scales with columns × polyprims × average polyprim length. Watch frame time; drop columns first if it regresses.

## Future extensions (out of scope)

- **Lateral start offset.** Push the first vertex of each lateral polyline along its initial direction by a fraction of its `branch_radius`, so the lateral's tube starts *inside* the parent tube instead of exactly at the parent's position. Gives cleaner Y-junctions that convert to VDB with fewer internal holes. Tried in Stage 1 — a naive wrangle using `pointprims(0, @ptnum)[0]` was ambiguous at points that belong to multiple polylines (parent→self *and* self→next after the lateral extends). Proper fix: disambiguate by picking the polyline where this point is vertex 1 (the "arriving" edge), or walk `parent_id` directly. `i@is_lateral_start` is already authored and waiting to drive it. Revisit when meshing artefacts matter.
- **Junction welding** via VDB-from-polys → Convert VDB at high voxel res. Would close all internal holes including the Y-intersections above. Defer until meshing artefacts show up in renders.
- **Cross-section profile curves** for hero shots (bark ridges, elliptical cross-sections).
- **Murray's rule radius propagation** (already tracked in `doc/todo.md` from Phase 3a). Would make the trunk/lateral radius relationship biologically faithful before the sweep consumes it.
