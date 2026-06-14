# Physics System Refactor — BioGrowth SCA

## Context

The current Space Colonization implementation moves tips by direct position stepping: `attract` accumulates a normalized direction per tree node, `grow` writes `new_P = P + dir * step_size`, and `topology` spawns a child at parent's P. New children walk toward `target_P` via a `is_arrived`-gated lerp inside `grow`. This model has no memory — each step is independent of the previous step's heading — and every new force contributor (future VDB constraints: obstacle, surface, density, light, spatial tropism) has to directly mutate `new_P` in sequence, making composition order-dependent.

We are replacing the direct-step model with a **velocity/acceleration integrator**. Each **tip** tree node carries `v@velocity`. `attract` atomic-adds into its own accumulator `att_*`. All other force producers (tropism v1; future VDB obstacle, surface, density, light, wander, …) atomic-add into a shared `acc_*` accumulator. An `integrate` kernel sums both and runs the semi-implicit Euler step on tips only. Children are spawned when a tip has traveled `step_size` of arc length (not when it "arrives" at a pre-chosen target). The walking stage and `is_arrived` disappear — motion and growth are one thing.

**Two-era model — this plan covers era one only.**

1. **Growth era (this plan).** The SCA solver grows the tree. Only **tips** integrate under physics; interior nodes are frozen once they lose tip status. Polyline coherence is preserved by construction (parent and child segments cannot drift apart). Growth-era bending — curving around obstacles, phototropism, gravitropism — is a tip-steering phenomenon, and the curved trunk is laid down curved. This matches real biology: wood is much stiffer than the growing meristem, so during growth the tree body really is frozen at the timescale of tip extension.
2. **Mechanical era (out of scope; see Future Extensions).** A downstream deformer runs on finished polylines to produce mature-branch sag, wind animation, and collision response. Kept separate from the growth solver to avoid feedback instabilities (force pulls trunk → trunk moves → attractor cells reassign → different growth) and to allow re-deforming one grown tree under many conditions.

The rest of this plan is about the **growth-era** solver. Global forces (tropism, future VDB fields) act **only on tips**. The tree body inherits its geometry from the path each tip traced.

**Why this matters:**
- **Steering behaviours** (Reynolds 1999) — seek, arrive, pursue, wander, flocking — all compose as force contributions on the tip integrator. Enables inertia-driven organic curves, persistent "staring" at a target, smooth S-bends.
- **Modularity for VDB constraints** — each future constraint node atomic-adds a tip force; ordering is commutative. No more `new_P` hand-off between serialized nodes.
- **Animated attractors / moving targets** — velocity model naturally does `pursue` without extra logic.
- **Coherent polylines by construction** — no mid-sim stretching or crossings, clean input for downstream meshing and the mechanical deformer.

Branch: `feature/physics` (already checked out, branched from `feature/vdb-constraints`).

## Design Decisions (user-confirmed)

- **Only tips integrate.** The integrate kernel early-returns on non-tips. Interior nodes are frozen. This makes the whole plan robust by construction: no drift → no broken polylines. Force contributors are expected to skip non-tips on their side too (cheap early-return in the inner loop) — writing to non-tip accumulators is harmless but wasteful.
- **Child velocity inheritance is configurable.** Expose `child_velocity_inherit` (float parm, 0..1, default 0.5) on the solver. `child.v = parent.v * child_velocity_inherit`. Leaves room later for an envelope/falloff generator driving this per-node by depth, local density, or age.
- **Attract has its own accumulator; all other contributors share a second one.** `attract` writes to `att_ix/iy/iz` and increments `att_count` via int-fixed-point atomics (attractors genuinely race on picking the same tip). Integrate averages attract's sum by `att_count` so a tip chosen by 50 attractors doesn't get 50× the force of a tip chosen by 1. **All other contributors** (tropism v1; future VDB chain) write `normalized_dir × strength_parm` into a shared `v@force_acc` (plain float3) via `@acc.set(@acc + force)` — no count, no averaging, just raw vector sum. No atomic is needed because every contributor writes to its own point (single-writer-per-destination). Each contributor's strength parm is its own knob; composition is linear addition. Reintroduce the int-atomic pattern locally on any future cross-point contributor (pairwise self-collide, dominance suppression).
- **`attract` is winner-take-all over *any* tree node (classical SCA).** Each attractor picks the overall closest tree-node (tip or interior) within `perception_radius`. Writing to interior nodes is how classical SCA produces side-branches: an interior trunk-node that happens to be the closest thing to a fresh attractor accumulates signal and spawns a new tip on the next topology pass. Restricting attract to tips-only eliminates this mechanism → single-trunk growth only. **This was a design revision (Stage 1b):** the earlier plan filtered candidates by `is_tip<1` and produced no branching; we reverted the filter.
- **Interior nodes accumulate attract signal but don't move.** Integrate still early-returns on `is_tip<1` — only tips integrate. Interior accumulators are consumed by `topology`'s interior-branching path (see Per-Kernel Changes), which spawns a new tip-child at the interior node's position when `att_count > 0`, then resets the parent's accumulators. This preserves polyline coherence (interior geometry never moves) while restoring branching.
- **Tip detection via `i@is_tip`** maintained in `topology`: seed has `is_tip=1`; on spawn, parent → `is_tip=0`, child → `is_tip=1`. O(1), no graph walk.
- **Semi-implicit Euler** with damping and velocity clamp: `v = (v + a*dt) * max(0, 1 - damping*dt); |v| = min(|v|, max_speed); P += v*dt`. See Integrator Survey section for why this over Verlet / RK4 / explicit Euler.
- **Arc-length spawning** replaces `did_grow`: each node accumulates `arc_since_spawn += length(v*dt)`; topology spawns when `>= step_size` and subtracts `step_size` (preserves overshoot, drift-free under variable `dt`).
- **`is_arrived`, `target_P`, `did_grow`, `new_P` all disappear.** Walking and arrived collapse into "integrate under physics."
- **Force contract:** contributors emit `normalized_direction × strength_parm` before writing. Scope (tips-only vs. any tree node) is per-contributor: **attract writes to any tree node** (drives branching); tropism is inline in integrate (tips-only by construction); future VDB contributors choose their own scope. Accumulators on interior nodes that don't trigger branching must still be reset per-frame (today this is safe because only attract writes to interior, and topology resets `att_*` when it branches; a future contributor writing to `v@force_acc` on interiors would need a reset path — see Stage 2 gotcha).
- **Fixed-point scale for `att_*`:** drop from `BG_DIR_SCALE=1e6` to `1e4`. At 1e6, int32 saturates at ~2140 unit-vectors — hotspot nodes near the seed can exceed this. 1e4 gives ~214k headroom with plenty of precision for force directions. `v@force_acc` is plain float3 — no scaling.

## Integrator Survey

Spun out to [physics.md](./archive/physics.md) — full candidate comparison, decision rationale, and tuning limits. Summary: **semi-implicit Euler** (one force eval per frame, symplectic, bounded energy, composes cleanly with damping + velocity clamp). Verlet / RK4 rejected on force-eval cost; explicit Euler rejected on stability.

---

## Attribute Schema Delta

**Tree nodes — add:**
- `v@velocity` (float3, init 0)
- `f@arc_since_spawn` (float, init 0)
- `i@is_tip` (int, seed init 1)
- `i@att_ix, i@att_iy, i@att_iz, i@att_count` — attract-only accumulator (winner-sum + count for averaging)
- `v@force_acc` — shared float3 accumulator for all other contributors (raw vector sum, single-writer-per-destination, no atomic)

**Tree nodes — rename:**
- `grow_dir_ix/iy/iz` → `att_ix/iy/iz` (the existing attract accumulator is now attract-only, hence the `att_` prefix)
- `grow_count` → `att_count`

**Tree nodes — remove:**
- `is_arrived`, `target_P`, `did_grow`, `new_P`

**Topology propagation on spawn:**
- child: `P` = parent.P, `v` = parent.v × `child_velocity_inherit`, `arc_since_spawn=0`, `att_*=0`, `acc={0,0,0}`, `is_tip=1`, `is_tree_node=1`, `active=1`, `birth_time=@Time`
- parent: `is_tip=0`, `arc_since_spawn -= step_size`, `velocity={0,0,0}` (no longer a tip → won't integrate → freeze explicitly to avoid stale v leaking into any debug viz)

## Pipeline

```
grid_bounds → grid_count → prefix_sum → grid_scatter
  → attract → integrate → cull → topology → del_unactive → del_grid_attr
```

`grow` renamed to `integrate`. Order `integrate → cull → topology`: integrate must finish writing P before cull reads it for attractor-distance checks. Topology runs last so its interior-branching path reads the accumulated `att_*` on interior nodes before anything else stomps them — integrate only resets `att_*` for tips (early-returns on interior), so interior accumulators survive into topology.

## Per-Kernel Changes

### `attract` (force contributor — winner-take-all, any tree node)
- Bindings: current `&P, is_attractor, active, &closest_node, &att_ix/iy/iz, &att_count` + grid detail arrays. **No** `is_tip` filter.
- Filter: `is_attractor < 0.5 return; active < 0.5 return;`.
- Loop: 27-cell search picks the overall-closest tree-node within `perception_radius`:
  ```
  int   best    = -1;
  float best_d2 = perception_radius * perception_radius;
  for each neighbor cell (27-cell loop):
    for each node_idx in cell:
      float3 npos  = vload3(node_idx, all_P);
      float  d2    = dot(apos - npos, apos - npos);
      if (d2 < best_d2) { best_d2 = d2; best = node_idx; }
  if (best < 0) return;                            // no tree-node in perception_radius
  float3 dir = bg_safe_normalize(apos - npos_best) * attract_strength;  // seek: tip → attractor
  bg_atomic_add_dir(att_ix, att_iy, att_iz, best, dir);
  atomic_inc(&att_count[best]);
  ```
- Parms: single `attract_strength` (float, default 1.0).
- **Sign note:** `apos - npos_best` = (attractor_pos - tree_node_pos), a direction vector pointing *from* the tree node *toward* the attractor. This is the correct seek direction for pulling the node in.
- `closest_node` write is kept for debug/viz; can be dropped if unused.

### `integrate` (new — replaces `grow`)
- Bindings: `&P, &velocity, &att_ix/iy/iz, &att_count, &force_acc (float3), &arc_since_spawn, is_tree_node int read, is_tip int read`.
- Parms: `damping` (float, default 2.0), `max_speed` (float, default `2 * step_size / nominal_dt`), `tropism` (float3, default {0,-1,0}), `tropism_strength` (float, default 0.0 — off by default, since default behaviour should be SCA-driven shape, not biased), `step_size` (kept as topology rhythm parm).
- Body:
  ```
  if (@is_tree_node < 0.5) return;
  if (@is_tip       < 0.5) return;    // interior nodes never move

  // Attract: averaged over the N attractors that picked this tip
  float3 att_force = (float3)(0, 0, 0);
  if (@att_count > 0) {
      att_force = bg_decode_dir(@att_ix, @att_iy, @att_iz) / (float)@att_count;
  }

  // Shared contributors (future VDB chain): raw vector sum, each already scaled
  float3 shared_force = @force_acc;

  // Built-in tropism — inline because it's cheap and needs no grid access
  float3 tropism_force = bg_safe_normalize(@tropism) * @tropism_strength;

  float3 a = att_force + shared_force + tropism_force;

  // Semi-implicit Euler
  float3 v = (@velocity + a * @TimeInc) * max(0.0f, 1.0f - @damping * @TimeInc);
  float sp = length(v);
  if (sp > @max_speed) v *= @max_speed / sp;

  float3 dP = v * @TimeInc;
  @P.set(@P + dP);
  @velocity.set(v);
  @arc_since_spawn.set(@arc_since_spawn + length(dP));

  // Reset accumulators for next frame
  @att_ix.set(0); @att_iy.set(0); @att_iz.set(0); @att_count.set(0);
  @force_acc.set((float3)(0.0f, 0.0f, 0.0f));
  ```
- Invariant: single writer per node; no cross-point reads.
- Composition math: `a = (att_sum / att_count) + Σ_i (contributor_i_dir × contributor_i_strength) + (trop_dir × trop_strength)`. Each contributor owns its strength; attract's coverage-averaging is isolated to the `att_*` slot.

### `topology` (two spawn paths)
- Outer filter: `if (i@is_tree_node < 1) return;`
- **Tip extension path:** `i@is_tip == 1 && f@arc_since_spawn >= chf("step_size")` → spawn child at `v@P` (parent's current position — tail of the new segment), polyline parent→child.
  - Child: `is_tip=1, is_tree_node=1, active=1, birth_time=@Time, velocity = parent.velocity * child_velocity_inherit, arc_since_spawn=0, att_*=0, att_count=0, acc_*=0`.
  - Parent: `i@is_tip=0; v@velocity={0,0,0}; f@arc_since_spawn -= chf("step_size");` (parent freezes — interior node).
  - `return;` — one spawn per point per frame.
- **Interior branching path:** `i@is_tip == 0 && i@att_count > 0` → interior node has been targeted by ≥1 attractor. Spawn new tip at parent's P.
  - Decode direction: `vector dir = normalize(set((float)i@att_ix, (float)i@att_iy, (float)i@att_iz));` (scale factor cancels under normalize — no need to touch `BG_DIR_SCALE` in VEX).
  - Child: same attribs as tip-extension child, but `velocity = dir * 1e-3` (tiny nudge — enough to make the child strictly closer to the attractor next frame than the parent; integrator accelerates it from there).
  - Parent: reset `i@att_ix=0; i@att_iy=0; i@att_iz=0; i@att_count=0; v@force_acc={0,0,0};` — parent remains interior. Reset prevents re-spawn pile-up next frame.
- No `did_grow` / `new_P` writes anywhere.
- **Why the nudge is velocity, not position:** segment starts at length 0 (parent and child at same P), which matches the tip-extension path. The integrator grows the segment over the child's lifetime.

### `cull` (attractor proximity kill)
- Single concern: attractor within `kill_radius` of any tree node → `active=0`.
- **Stall kill removed 2026-04-19.** Made obsolete by `arc_since_spawn` in topology (static tips don't over-spawn at the source anymore). Archived in [optimization.md](./archive/optimization.md) with revival recipe; re-add if Phase 2 VDB constraints cause trapped-attractor accumulation.
- **Dead-tip cull tried and dropped (Stage 2, Apr 2026).** Flickered newborn tips; orphan tips are cheap (attract only writes to picked nodes); interior branching resurrects dead regions when new attractors arrive. If stranded-tip perf matters later: newborn grace period (`@Time - birth_time >= grace`) or geometric bounds test against scatter bbox.

### `grid_count`, `grid_scatter`
- Remove `is_arrived<1` filter (attribute gone). Replace with `is_tree_node<0.5` only. Attract searches over all tree nodes (not just arrived ones) because every tree node exists in "final" state now — no walking.

### `set_seed` (outside solver)
- Remove: `is_arrived=1`, `target_P=P`, `new_P=0`, `did_grow=0`, `grow_count=0`, `grow_dir_ix/iy/iz=0`.
- Add: `v@velocity={0,0,0}`, `f@arc_since_spawn=0`, `i@is_tip=1`, `i@att_ix=0; i@att_iy=0; i@att_iz=0; i@att_count=0;`, `v@force_acc={0,0,0};`.
- Keep: `is_tree_node=1, is_attractor=0, active=1`.

### `set_attractor` (outside solver)
- Unchanged.

### `del_grid_attr`
- Detail attribs stripped: unchanged list (`cell_count, cell_start, cell_offset, sorted_idx, cell_size, total_cells, grid_min, grid_res`).

### `biogrowth.cl` include
- `BG_DIR_SCALE` drop from `1000000.0f` to `10000.0f`. Add block comment documenting the **force contract** (contributors emit `normalized_dir × strength_parm`).

## Numerical Stability & Tuning Guardrails

- **Damping × dt < 1.** Clamp via `max(0, 1 - damping*dt)`. At 24fps (`dt≈0.0417`), safe damping < 24. Document on solver parm.
- **max_speed × dt ≥ step_size.** Otherwise topology never spawns. Recommend default `max_speed = 2 * step_size / expected_dt`.
- **max_speed × dt ≪ many node-lengths.** Otherwise one frame traverses far and polylines zig-zag visually.
- **BG_DIR_SCALE=1e4** — gives ~214k unit-vectors of headroom at int32 max; safe for hotspot nodes.
- **First-frame bootstrap.** Seed has `v=0` and `is_tip=1`. Attract writes `att_*` → integrate produces v. Works as long as attractors exist within `perception_radius` of the seed on frame 1.
- **Force contract enforcement.** Every contributor must pre-normalize and scale by its own strength parm, and skip non-tips. Violations → magnitude blow-up near strong gradients (e.g. a raw `1/d²` obstacle field written into `v@force_acc` would dominate other contributors in the shared sum).
- **Two-era boundary.** Nothing in the growth-era solver should try to move interior nodes. That work belongs to the downstream mechanical deformer. If you find yourself wanting to, add it to the deformer, not here.

## Staged Sequence

### Stage 0 — Schema migration, no behaviour change
- Update `set_seed` snippet: add `velocity`, `arc_since_spawn`, `is_tip=1` init; rename `grow_dir_*/grow_count` → `att_*/att_count` in the init list; add `acc={0,0,0}` (new, inert in Stage 0).
- Update `topology` snippet: set `is_tip=1` on child, `is_tip=0` on parent (no other behavior change).
- Rename `grow_dir_ix/iy/iz` → `att_ix/iy/iz` and `grow_count` → `att_count` in `attract` and `grow` kernels (find/replace in snippets).
- Drop `BG_DIR_SCALE` to `1e4` in `biogrowth.cl`. Re-render a frame — visual identical (scale is internal to accumulator, decoded before normalize).

**Exit criteria:** sim output identical to `feature/vdb-constraints` reference. Commit as `refactor: physics schema migration (no-op)`.

### Stage 1 — Core physics swap
- Replace `grow` snippet with `integrate` kernel (new body per spec above, with early-return on `is_tip<1`). Rename the node.
- Rewrite `attract` to write attract-only accumulator `att_* += normalized_dir × attract_strength; att_count += 1`, skipping non-tips during the 27-cell search.
- Rewrite `topology`: arc-length spawning, child velocity inheritance, `is_tip` maintenance, parent `velocity={0,0,0}` freeze. Remove all `did_grow` / `new_P` / `target_P` writes.
- Remove the `grid_dirty` mechanism entirely: delete the `set_grid_dirty` node outside the solver, the inner `grid_dirty` clear wrangle, and the gate early-return in `grid_count` / `prefix_sum` / `grid_scatter`. Grid rebuilds every frame. See [optimization.md](./archive/optimization.md) for the archived recipe if it ever needs to come back.
- Remove `is_arrived` filter in `grid_count` / `grid_scatter` (attribute gone).
- Promote new solver parms: `damping`, `max_speed`, `attract_strength`, `child_velocity_inherit`, `tropism_strength`. Remove/retire the old `tropism_strength` if it had a different meaning.

**Exit criteria:** full sim runs end-to-end with physics. Tune `damping`/`max_speed`/`step_size` until tree looks plausible. Compare frame 175 shape to reference — shape will differ (that's the point) but behaviour should be "tree-like": tips advance toward attractors, spawn laterals, cull works.

**Commit as** `feat: physics integrator replaces direct-step motion`.

### Stage 2 — Polish & cleanup
- Dead-tip cull: tried and dropped (flickered newborn tips, orphans are cheap). See the `cull` section above for the revisit recipe.
- Delete dead attribs from `del_grid_attr` if any remain; verify `is_arrived` doesn't leak via feedback.
- Update `biogrowth.cl` comment block with force-contract documentation (contributors emit `normalized_dir × strength_parm`, skip non-tips, write to shared `acc_*` — attract is the only exception and owns its own `att_*` accumulator).
- **Spin out the Integrator Survey section of this plan into `doc/physics.md`** so it lives alongside `network.md` and `biogrowth-plan.md` as a reference doc. Link from `doc/network.md`.
- Update tracking copies under `d:/SPEC/BioGrowth/hou/scripts/ocl/sop/*.cl` to match live snippets.
- Update `doc/network.md` and `doc/biogrowth-plan.md` to document the physics pipeline (new attribs, new kernels, force-scope policy, arc-length spawning). Update `project_biogrowth.md` memory.

**Commit as** `refactor: physics cleanup, docs`.

## Critical Files

Live code (AppData\Temp\*, current session prefixes — will change on reboot):
- `55_set_seed_snippet.vfl` — add physics attribs
- `41_topology_snippet.vfl` — arc-length spawn, is_tip, child v inheritance
- `46_attract_kernelcode.cl` — force contributor
- `47_grow_kernelcode.cl` → rename to `integrate` — full rewrite
- `40_cull_kernelcode.cl` — attractor proximity kill only (stall + dead-tip both removed)
- `37_grid_count_kernelcode.cl`, `39_grid_scatter_kernelcode.cl`, `38_prefix_sum_snippet.vfl` — drop is_arrived filter and the `grid_dirty` early-return
- `48_grid_dirty_snippet.vfl` — **delete the node** (see [optimization.md](./archive/optimization.md))
- `61_set_grid_dirty_snippet.vfl` (outside solver) — **delete the node**

Real files:
- `d:/SPEC/BioGrowth/hou/scripts/ocl/include/biogrowth.cl` — drop `BG_DIR_SCALE` to 1e4, add force-contract doc comment
- `d:/SPEC/BioGrowth/hou/scripts/ocl/sop/*.cl` — tracking copies, sync after live code stabilizes
- `doc/network.md`, `doc/biogrowth-plan.md`, `doc/todo.md` — post-refactor updates

Solver spare parms to add/promote:
- `damping` (float, default 2.0)
- `max_speed` (float, default `2 * step_size / nominal_dt`)
- `attract_strength` (float, default 1.0)
- `tropism_strength` (float, default 0.0 — off by default; turn on for biased growth)
- `child_velocity_inherit` (float, default 0.5)

Kept as-is: `step_size`, `perception_radius`, `kill_radius`, `tropism` (float3 direction).

## Verification

End-to-end on the sim timeline:
1. **Stage 0 check** — cook frame 175 before and after; diff P/attributes. Must be byte-identical.
2. **Stage 1 sanity** — seed → advances under attract force; laterals spawn at arc distances ≈ `step_size`; no NaN/inf in `P` or `velocity`; `|v|` stays bounded at `max_speed`.
3. **Polyline coherence** — cook to frame 175, inspect segments: no two linked points should ever be farther apart than `~step_size + ε`, and no segment should cross itself. This is the safety net for "only tips integrate." If any segment stretches, `is_tip` maintenance is broken somewhere and interior nodes are drifting. Hard failure.
4. **Physics parameter sweep** — vary `damping` 0.5 / 2 / 8: low damping should show overshoot/oscillation, high damping should look nearly-quasistatic. Confirms integrator responds.
5. **Child inheritance sweep** — `child_velocity_inherit = 0 / 0.5 / 1.0`: visible branching character shift (crisp angles → smooth continuation).
6. **Interior branching** — place an attractor whose closest tree node is an interior (non-tip) segment. On the next topology pass, the interior node should spawn a new tip-child that begins growing toward the attractor. Confirms the interior-branching path triggers and that the parent interior does NOT drift (its `P` remains exactly what it was at the moment `is_tip` flipped to 0).
7. **Attractor-less region** — tip entering a volume with no attractors: under tropism, drifts at steady-state `tropism_strength / damping`; with `tropism_strength=0`, damps to stop. Does not cull (no dead-tip cull in current build — see the `cull` section for the reintroduction recipe).
8. **Tropism biases tips only** — set `tropism_strength=2`, default direction `{0,-1,0}`: all tips bend downward as they grow; existing interior segments stay exactly where they were spawned (visible as a dataset-diff against a `tropism_strength=0` cook — interior point positions should match bit-for-bit).
9. **Perf** — compare ms/frame at frame 175 vs `feature/vdb-constraints` baseline (~300ms). Grid is now rebuilt every frame (was gated by `grid_dirty`); expect a small regression, should stay sub-ms at our scale. Attract kernel cost is essentially unchanged (same 27-cell search, same single-winner atomic-add pattern; only change is the skip-non-tips filter inside the loop).

## Future Extensions (out of scope for this plan)

- **Envelope / falloff generator** driving `child_velocity_inherit` per node by age, depth, or local density.
- **Wander force** (persistent random walk for organic S-curves).
- **Flocking** between nearby tips (branches gently avoid each other — superseded in practice by the `self_collide` contributor, see `vdb-constraints.md` Phase 3).
- **Arrive / pursue** steering for animated targets (ties into animated-attractors direction).
- **VDB spatial tropism / obstacle / surface / density / light** — resume the original `feature/vdb-constraints` plan on top of this physics model; each becomes a force-contributor OCL node in a chain between `attract` and `integrate`. (Avoid + collision landed 2026-04-19 as inlined stages of `integrate`; Stages 2–4 surface-project / density / photo pending.)
- **Vigor + lateral branching** — introduces `i@split_depth` (bifurcations only), a depth-cycled grid-participation filter, and a `lateral_pressure` accumulator that lets interior nodes sprout laterals when crossed a depth-scaled threshold. Produces an emergent heading region (frontier dominates, interior quiescent) from horticultural *vigor*. Scheduled after VDB constraints complete. See [vigor.md](./archive/vigor.md).

### Mechanical Deformation — Downstream Pass (not part of the SCA solver)

Global body deformation (mature-branch sag under gravity, wind animation, collision with moving objects) is a distinct physical regime from growth and belongs in a separate solver *after* polylines are finalised, not inside the growth loop.

**Why separation matters:**
- **Biologically accurate.** Real wood is much stiffer than the growing tip. During growth, the tree body is effectively frozen on the timescale of tip extension. Growth-era bending (curving around obstacles, phototropism) is a tip-steering phenomenon; the curved trunk is laid down curved, not retroactively deformed. The tip-only integrator already captures this faithfully.
- **Mechanical-era behaviour is what actually sells render realism:** willow droop, oak-limb arc under weight, wind displacement, snow load. This is *post-growth static/dynamic equilibrium on a finished structure*, not growth dynamics.
- **Coupling global forces back into the SCA growth loop risks feedback instabilities** (force pulls trunk → trunk moves → attractor cells reassign → different growth → different forces) and couples two concerns that want independent tuning.

**Proposed architecture:**

```
SCA solver (tip-only physics)  →  polylines + per-node attributes
                                            ↓
                                Branch-thickness + age pass
                                            ↓
                                  Mechanical deformer
                                  (Cosserat rod / PBD beam)
                                    - gravity sag (per-segment)
                                    - wind displacement
                                    - contact with moving kin objects
                                    - stiffness scales with age/diameter
                                            ↓
                                      deformed polylines
                                            ↓
                                         meshing
```

**Design sketch** (not implemented here, just to pin intent):
- Input: the SCA output polylines with `birth_time` + derived `branch_diameter` per node.
- Attributes needed for deformer: `parent_idx` per node (add at cheap cost during SCA or derive from polyline primitives post-hoc), `stiffness` per segment (function of diameter, age), `mass` per segment (function of diameter² × length).
- Solver: either (a) Discrete Elastic Rods (Bergou et al.) for highest fidelity including torsion, or (b) PBD with distance + bending constraints for simpler tuning and better GPU mapping. Branching junctions handled as multi-child constraints at each interior node.
- Root is pinned (seed position held fixed); everything downstream hangs from it.
- Runs per-frame for animation (wind) or once-baked for static renders (gravity sag equilibrium).

**Why this also unlocks more workflow value:**
- **Re-deform under many conditions from one grown tree** — morning-calm vs. evening-wind, summer-leafed vs. bare winter — without re-growing.
- **Per-shot animation** decoupled from the multi-minute growth sim.
- **Meshing thickness data is shared** with the deformer's stiffness model — one attribute system, two consumers.

**When to pick this up:** after the SCA physics refactor ships and meshing begins. It replaces any temptation to sneak mechanical realism into the growth solver and sets up clean foundations for wind/dynamic animation on finished trees.
