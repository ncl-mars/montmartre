## SOP Network — BioGrowth

**Status:** documents the **post-physics-refactor** target state (see [biogrowth-plan.md](./biogrowth-plan.md)). Live code still reflects the pre-refactor direct-step model until Stage 1 ships. Re-verify node names/prefixes against AppData\Temp after each checkpoint.

Last updated: 2026-04-29. Post `feature/senescence`: new `arc_total_since_seed` attribute (cumulative path length from root, accumulated in apply, inherited at every spawn, never reset) + `biogrowth_senescence.cl` velocity modifier (`v *= 1 - pow(arc/max_arc, curve)`) — first new modifier in the slot established by `refactor/velocity-modifiers` (which split `integrate` into force→v + modifier chain + apply, with collision moved into apply). Earlier same-day: `refactor/avoid-extraction` `avoid` lifted out of `integrate`'s stage 2 into its own `biogrowth_vdb_avoid.cl` OCL contributor, reading a dedicated `avoid_sdf` VDB on the constraints bus (decoupled from `collision_sdf` which stays on the collision bus for hard snap-out only). Switch-gated `avoid_enable`. Collision stays fused inside integrate (it's a position resolver, not a force, and tightly coupled to the Euler step). Earlier (2026-04-27): post-damage event-bus landing (`feature/damage`): `f@damage` schema seeded + inherited at every spawn; `damage_field` (VDB-driven event source) + `damage_propagate` (parent → child max-with-decay) kernels on the tree-only stream; one-line consumer modulators in `dominance_step`, `murray_propagate`, and `integrate` (tip growth-factor + light-credit drain). Synthetic per-rule documentation consolidated into the new "Biological rules" section below; per-feature plan docs archived in `doc/archive/`. Earlier landings: split-streams refactor 2026-04-25 (entire stable-id machinery deleted, `parent_id` permanently valid raw ptnum, ~1s/frame at 800k tree_nodes); apical-dominance + branch-autonomy 2026-04-23/24; phyllotaxis + Leonardo + reaction-wood + per-node lateral cap 2026-04-23/24; allometric + Switch-toggle pattern 2026-04-22.

### Two-era model
- **Growth era (this network).** SCA solver grows the tree. Only **tips** integrate under physics; interior nodes freeze on losing tip status. Polyline coherence preserved by construction.
- **Mechanical era (separate downstream deformer, not built yet).** Mature-branch sag, wind, collision run on finished polylines. See Future Extensions in the plan doc.

Single geometry stream inside the solver — tree nodes and attractors stay merged throughout. Grid arrays (`cell_count`, `cell_start`, `cell_offset`, `sorted_idx`) live as detail int[] on the geometry. Each OCL kernel filters by the `is_tree_node` / `is_attractor` / `is_tip` int attributes (groups aren't bindable in OCL).

---

## Outside the solver (setup)

```
 [Add: root_pt]          [Box: attractor_bounds]    [VDB Combine: union]   [VDB Combine: union]
       |                         |                          |                       |
 [AW: set_seed]          [PtsFromVol: scatter]    [constraints bus]       [collision bus]
       |                         |                  soft force VDBs      collision_sdf +
       |                  [AW: set_attractor]      (future: surface,     response_type +
       |                         |                  density, photo)      avoid_weight
       |                  [switchif1]  <-- $SF==1
       |                         |
       v                         v                         v                       v
   (input 0)                (input 1)                  (input 2)               (input 3)
       \______________________\_|_/______________________/_______________________/
                                 |
                    [AW: set_stable_counter]   detail: next_stable_id = 1 if missing
                                 |
                         [SOP Solver: biogrowth]
                                 |
                          [split_attractors]
                            |           |
                   [attractors_rest]  [tree_nodes]
                                          |
                                     [Resample] (optional)
                                          |
                                     [AttribBlur: blur_radius]  (branch_radius, connectivity)
                                          |
                                     [AW: set_pscale]  (@pscale = @branch_radius)
                                          |
                                     [Sweep]  (Round Tube, cols=6)
                                          |
                                       mesh_out
```

Indexing: 0-based throughout this doc (matches Houdini's native convention — `opinputpath("../", N)` and the solver's spare-input indices).

**Note on `switchif1` (frame-1 attractor gate).** Temporary bootstrap. The pipeline already supports streaming attractors via `merge_new_attractors` — each frame, Input 1 of the solver is merged in, so in principle new attractors can appear on any frame. The switchif currently clamps this to `$SF == 1` for a one-shot scatter. Relaxing the gate (or replacing Input 1 with an animated source) is how time-additive attractor animation lands. When that happens: if the upstream animated source can re-emit the same point, a dedup step belongs upstream of merge (the solver does not dedup).

---

## Inside the solver (per-frame)

```
   [Prev_Frame]                  [Object Merge: attractors]   ← opinputpath("../", 1)
        \                                  /
         \                                /
          [Merge: merge_new_attractors]                   (frame-1 gate or streaming)
                       │
          [AW: grid_bounds]
                       │
          [OpenCL: grid_count]                            (filters is_tree_node)
                       │
          [AW: prefix_sum]
                       │
          [OpenCL: grid_scatter]                          (filters is_tree_node)
                       │
          [OpenCL: attract]                               (runs over attractors, writes tree att_*)
                       │
          ┌──── [split_tree_nodes] (SPLIT on is_tree_node) ────┐
          │                                                    │
          ▼                                                    ▼
  ───────────────────────────                          ──────────────────
  [ tree-only stream ]                                  [ attractor stream ]
  ───────────────────────────                          ──────────────────
          │                                                    │
   damage system (each Switch-gated)                  
          │   [OpenCL: damage_field]   ← OM import_constraints, Input 2
          │   [Switch: damage_field_enable]                    │
          │   [OpenCL: damage_propagate]                       │
          │   [Switch: damage_propagate_enable]                │
          │   [Switch: damage_enable]   (master bypass)        │
          │                                                    │
   soft contributors (each Switch-gated)                       │ 
          │   [OM: import_constraints] ← Input 2               │ 
          │   [OpenCL: surface]      → surface_enable          │               
          │   [OpenCL: density]      → density_enable          │
          │   [OpenCL: photo]        → photo_enable            │
          │   [OpenCL: avoid]        → avoid_enable            │
          │   [OpenCL: self_collide] → self_collide_enable     │
          │                                                    │
          [OpenCL: integrate]                                  │
          │   force sum (att + force_acc + tropism)             
          │   → Euler + max_speed clamp → write @velocity      │
          │   (interior branch: vigor accumulation)             
          │                                                    │
   velocity modifier chain (each Switch-gated)                 │
          │   [OpenCL: allometric]      → allometric_enable    │
          │   [OpenCL: senescence]      → senescence_enable    │
          │   [OpenCL: branch_autonomy] → branch_autonomy_enable
          │   [OpenCL: damage_growth]   → gated by damage_enable
          │           ← future modifiers slot here              
          │                                                    │
          [OM: import_collision_and_irradiance] ← Input 3      │
          │   collision_sdf + avoid_weight + response_type     │
          │   (irradiance is on the constraints bus, read       
          │    by branch_autonomy directly)                    │
          │                                                    │
          [OpenCL: apply]                              [OpenCL: cull]  ← Input 1: tree-stream P
          │   P_new = P + v·dt; collision snap + response;     (vload3 on @tree_P.data)
          │   writeback @P, @velocity, @arc_*                  │
          │                                                    │
          [OpenCL: murray_propagate]                           │
          [OpenCL: murray_finalize]                            │
          [Switch: murray_enable]                              │
          │                                      [Group Expression: unactive_group]
          [OpenCL: dominance_step]                     group "del" = @active==0
          [OpenCL: dominance_finalize]                         │           && @is_attractor==1
          [Switch: dominance_enable]                           │
          │                                                    │
          [AW: topology]                              [Blast: blast_unactive]
          │   reads this-frame branch_radius +                 │
          │   dominance + damage; appends children             │
          │   to end of tree-only ptnums                       │
          │                                                    │
          └──────── [merge_attractors]  ───────────────────────┘
                       │
          [Attribute Delete: del_grid_attr]
                       │
                      OUT
```

Order note: `merge_new_attractors → grid_{bounds,count,scatter} → attract → split_tree_nodes → [tree: damage_* → soft contributors → integrate → velocity-modifier chain → apply → murray_* → dominance_* → topology] ∥ [attractors: cull → blast_unactive] → merge_attractors → del_grid_attr`. The split happens after `attract` so the grid kernels can stay on the merged stream; cross-stream access only kicks in for `cull` (reads tree-stream `@P` via Input 1). **Integrate is now small** — it does pure force → velocity (Euler + clamp) plus interior vigor accumulation. **Velocity modifiers** (`allometric` → `senescence` → `branch_autonomy` → `damage_growth`) live in their own kernels in a chain between integrate and apply, each Switch-gated. **Apply** consumes velocity → P + collision + writeback; collision is the only thing left fused inside apply because it's a position resolver (post-step P_new snap + conditional v rewrite, register-resident across the snap). Avoid is its own soft contributor (`avoid_sdf` on the constraints bus, distinct from `collision_sdf`). Murray + dominance run *before* topology so it reads this-frame `branch_radius` and `dominance`. Damage event sources (`damage_field`, `damage_propagate`) sit at the top of the tree-only stream so propagated damage is visible to all downstream consumers the same frame.

**The split-streams invariant.** Tree_node ptnums never change, ever. Topology may only **append** new children to the end of the tree-only stream. No deletes, no reorders, no upstream sorts on tree_nodes. `parent_id` set at spawn time as `@ptnum` stays valid for the lifetime of the tree. Branch autonomy + allometric both freeze velocity rather than killing — preserving this invariant. If you ever need to *delete* a tree_node, either (a) revive stable-id machinery for tree-only, or (b) cascade-delete entire subtrees in a way that preserves remaining ptnums. Don't introduce surprise deletion; the blast group is guarded `active == 0 && is_attractor == 1` precisely to enforce this.

**Feature enable pattern.** Optional subsystems (`vigor`, `self_collide`, `surface`, `density`, `photo`, `allometric_cull`, `murray`) are toggled by **Switch SOPs** bypassing their respective OCL nodes, driven by solver spare parms `<feature>_enable`. When a feature is off, the Switch routes geometry around the node — zero dispatch cost, and the graph self-documents which subsystems are active. In-kernel `_enable` guards are therefore absent from these nodes; the Switch is the single authority. Each bypassed contributor must leave the graph in a valid state (e.g. `force_acc` zero-init is fine because `integrate` consumes & resets it every frame; interior `lateral_pressure` stays 0 if vigor branch never writes to it — which is the desired "disabled" behavior).

**`parent_id` is now a permanent raw ptnum.** Pre-refactor, `blast_unactive` deleted attractors from the merged stream, shifting tree_node ptnums every frame. The fix at the time was a stable-id indirection (`assign_sids` / `resize_sid_lookup` / `resolve_sid_scatter` / `resolve_parent_id`) rebuilding `parent_id` from a never-reused `parent_stable_id` each frame. The split-streams refactor (2026-04-25) made all of that unnecessary: tree_nodes live on their own stream throughout the solver, topology only appends, and the re-merge puts trees first so their ptnums survive into the next frame's solver input unchanged. `parent_id = @ptnum` at spawn stays valid across all subsequent frames — no resolve chain, no `stable_id` / `parent_stable_id` / `sid_to_ptnum` attributes. See [optimization.md](./archive/optimization.md) for the archived stable-id mechanism.

**Four solver inputs (0-based, matches Houdini).**
- **Input 0** — seed (one-shot, frame-1).
- **Input 1** — streaming attractors (sim feedback path via `merge_new_attractors`).
- **Input 2** — constraints bus (soft force VDBs: `avoid_sdf`, `surface_sdf`, `density`, `photo`, plus optional `damage_field`). Read-only.
- **Input 3** — collision bus. `collision_sdf` + `response_type` + `avoid_weight` (hard SDF resolver) **plus `irradiance` (branch autonomy)**. Imported into `integrate` via `import_collision_and_irradiance` Object Merge. Read-only.

Two separate buses because the semantics differ: constraints compose freely as forces (each soft contributor writes to `force_acc`); collision is authoritative and applied inline during integrate (stage 4 only — avoid was extracted to its own contributor on the constraints bus 2026-04-29). Keeping them on distinct inputs avoids accidentally routing a collision SDF into a soft-force node or vice versa.

**The buses are never combined inside the solver.** Each kernel that needs a VDB wires its own Input N to the appropriate bus's dedicated Object Merge (`import_constraints` for the soft-force bus, `import_collision` for the hard SDF bus). A kernel needing both would take two inputs — Input 1 from one OM, Input 2 from the other — not a single merged bus. Today only `integrate` consumes a VDB bus (collision, via `import_collision`); future `surface` / `density` / `photo` contributors will consume the constraints bus via `import_constraints`.

### merge_new_attractors
Combines prev-frame geometry (tree nodes + surviving attractors from the previous frame's output) with the Input 1 attractor set. Only merges on frame 1 via Switch-If (`$SF == 1`) outside the solver. Tree-first ordering is preserved across the merge so tree_node ptnums stay at the front of the stream.

### grid_bounds (Attribute Wrangle, detail)
`getbbox_min(0) / getbbox_max(0)` over all points, expand by `pad = cell_size` (`cell_size = perception_radius`). Derives `v@grid_min`, `v@grid_res`, `f@cell_size`, `i@total_cells`. Reinits `cell_count[]`, `cell_start[]`, `cell_offset[]`, `sorted_idx[]` to zero-filled detail int[] every frame.

### grid_count (OpenCL @KERNEL)
Runs over every point. Skips `is_tree_node < 0.5`. `atomic_inc(cell_count[cell_id])` per qualifying tree node.

**Vigor grid filter.** When `vigor_filter_enable=1`, interior tree nodes are additionally gated by `bg_grid_eligible(split_depth, tick, vigor_depth_cap, vigor_phase_stride)`: depth-N interior nodes enter the grid every `2^min(N, vigor_depth_cap)` frames, with per-depth phase stagger `split_depth × vigor_phase_stride`. Tips always participate. The SAME predicate with the SAME parms must run in `grid_scatter` or cell offsets corrupt.

### prefix_sum (Attribute Wrangle, detail)
Serial exclusive prefix sum over `cell_count[]` into `cell_start[]`. Resets `cell_offset[]` to zero.

### grid_scatter (OpenCL @KERNEL)
Runs over every point. Skips non-tree-nodes. Applies the same vigor grid filter as `grid_count` (see above) with matching parms. Claims `sorted_idx[cell_start + cell_offset++] = ptnum` atomically. After this pass, `sorted_idx[]` holds tree-node indices grouped by cell.

Grid is rebuilt unconditionally every frame. The previous `grid_dirty` gate was retired with the physics refactor — see [optimization.md](./archive/optimization.md) for the archived mechanism and re-introduction recipe.

### attract (OpenCL @KERNEL) — winner-take-all over any tree node
Runs over every point. Skips non-attractors / inactive attractors. For each active attractor:
- 27-cell neighbor search in `sorted_idx[]`. No `is_tip` filter — attract picks the overall-closest tree-node (tip or interior) within `perception_radius`.
- If no tree-node in range → skip frame.
- If winner found: direction = `normalize(attractor_pos - node_pos) × attract_strength`, atomic-add into the winner's `att_ix/iy/iz`, `atomic_inc(att_count[winner])`.

Attract owns the `att_*` accumulator exclusively. Writes to interior nodes are how classical SCA branching works: the interior node's `att_count` triggers `topology`'s interior-branching path (see below), which spawns a new side-tip at the interior position.

### VDB contributor pattern

**Soft contributors (`surface` / `density` / `photo` / `avoid` / `self_collide`).** Separate OCL nodes between the damage block and `integrate`. Inputs: Input 0 = main geometry (work-item count), Input 1 = constraints bus (solver Input 2 via `import_constraints` Object Merge). Binding: `#bind vdb <name> float input=1 read` (scalar) or `float3` (vector). Sampling: `@<name>.worldSample(P)` / `@<name>.worldGradient(P)` (cnanovdb-backed). Force contract: skip non-tips → compute force vector → write to `force_acc` (single-writer per point post-`force_acc` rename). Order commutative.

**`biogrowth_vdb_avoid` (OpenCL @KERNEL) — soft SDF anticipation.** Tips-only filter. Samples `avoid_sdf` at `@P`; if `sdf < avoid_range` and the tip is moving into the field (`dot(v, n) < 0`), computes the velocity-reflection direction `normalize(v - 2·vn·n)`, scales by `avoid_strength × (1 - smoothstep(0, avoid_range, sdf)) × sample(avoid_weight)`, writes to `force_acc`. `avoid_sdf` is its own VDB on the constraints bus — distinct from `collision_sdf` on the collision bus. Authoring intent: avoid is steering (low-res, wide-band field), collision is precision (high-res, narrow-band field). Switch-SOP `avoid_enable`. Parms: `avoid_strength`, `avoid_range`.

**self_collide (OpenCL @KERNEL) — tip-only self-repulsion.** Tips-only filter. 27-cell search over the same grid as attract. For each candidate: distance check first (`d2 < (r_me + r_other)²`), then bidirectional `parent_id` walk up to `ancestor_hops` levels — skip if either chain reaches the other. Force = Σ `normalize(delta) × (1 - smoothstep(0, r_sum, d))`, scaled by `self_collide_strength`, atomic-added into `force_acc`. Unit-dir contract opt-out (sum of weighted vectors; multiple near neighbours push harder than one). Per-node radius: `f@branch_radius` — on interior nodes, written by `murray_finalize` each frame as `(Σ r_child^p)^(1/p)`; on tips, keeps its spawn-time value (seed=`base_radius`; mainline inherits unchanged; lateral multiplies by `taper_decay`). `parent_id` is permanently valid post-split-streams — no per-frame resolve needed.

**Wall handling split as of 2026-04-29.** `avoid` and `collision` no longer share a kernel. **Avoid** lives as its own soft contributor (`biogrowth_vdb_avoid`) on the constraints bus, reading a dedicated `avoid_sdf` VDB — it's a force, fits the contributor contract, writes to `force_acc`. **Collision** stays inline inside `integrate`'s stage 4 because it's a position resolver (modifies `P_new` post-step, snaps out of `collision_sdf`, conditionally rewrites `velocity` based on `response_type`), tightly coupled to the Euler step. The two subsystems now have separate authoring contracts: avoid for steering (low-res, wide-band SDF), collision for precision (high-res, narrow-band SDF).

### integrate (OpenCL @KERNEL) — fused tip physics + interior vigor
Runs over every tree node. Two branches inside a single kernel dispatch:
- **Tip branch** (`is_tip > 0.5`): full physics pipeline. **Interior nodes never move.**
- **Interior branch** (`is_tip < 0.5`): vigor accumulation only — decode `att_*` raw sum, add `length(signal) × dt` into `lateral_pressure`, store `signal` into `last_att_force`, reset `att_*`.

Why fused: `att_*` reset has to happen per-frame on both tips and interiors (otherwise it compounds). Keeping the interior-accumulator work in integrate eliminates a separate `vigor_accumulate` dispatch and one extra full-point filter pass. Both branches reset their accumulators at the end, so the same node is clean for next frame regardless of role.

Inputs: Input 0 = main geometry, Input 1 = collision bus (solver Input 3 via `import_collision` Object Merge).

Five stages per tip, all register-local on `v` / `P_new` / `a`:

**1. External force contributors.**
- `att_force = bg_decode_dir(att_*) / att_count` (averaged) when `att_count > 0`.
- `shared_force = @force_acc` (plain float3 sum from `self_collide` / `surface` / `density` / `photo`).
- `tropism_force = bg_safe_normalize(tropism) × tropism_strength`.
- `a = att_force + shared_force + tropism_force`.

**2. Avoid contribution (read from `force_acc`).**
The standalone `biogrowth_vdb_avoid` contributor (between the soft contributors and integrate) has already sampled `avoid_sdf`, computed the velocity-reflection direction, and written its force into `force_acc`. Integrate's stage 1 picks it up alongside other contributors — no separate handling required here.

**3. Semi-implicit Euler.**
```
v' = (v + a·dt) * max(0, 1 - damping·dt)
|v'| clamped to max_speed
P_new = P + v'·dt
arc  += |v'·dt|
```

**4. Inline `collision` (hard post-step resolve).**
Sample `sdf_new = collision_sdf.worldSample(P_new)`. If `sdf_new < surface_band`:
- `n = normalize(grad_sdf_new)`, `push = surface_band - sdf_new`.
- Snap: `P_new += n × push`; `arc -= push` (teleport isn't traveled distance).
- Dispatch `response_type = round(sample(response_type, P_new))`:
  - `0` none — position snap only.
  - `1` stop — `v = 0`.
  - `2` slide — `v = (v - vn·n) × (1 - friction)`.
  - `3` rebound — `v = (v - 2·vn·n) × restitution`.
- Velocity response is gated on `vn < 0` (only modify if tip was moving into the surface).

**5. Writeback + reset.** Commit `P_new`, `v`, `arc`, `arc_total_since_lateral += dP_len` to point attributes. Zero `att_*`, `att_count`, `force_acc` for the next frame. Interior branch handles its own reset symmetrically.

**Parms on the integrate node:** `damping`, `max_speed`, `tropism` (float3), `tropism_strength`, `surface_band`, `friction`, `restitution`. (`avoid_strength` / `avoid_range` moved to the standalone `biogrowth_vdb_avoid` node.)

**Why collision stays fused.** Collision is a *position resolver* (snaps `P_new` out of the surface, conditionally rewrites `velocity` based on `response_type`), not a force. It must run *after* the Euler step (needs the post-step `P_new` to test penetration), needs `v` / `P_new` register-resident across snap + response, and modifies attributes that no other contributor touches. Avoid, by contrast, is a force fitting the contributor contract — extracting it cost only one extra kernel dispatch (the `acc_*` int-atomic encode/decode round-trip that originally justified fusion was removed by the `force_acc` rename). See `doc/avoid.md` for the rationale.

### Allometric length limit (inside integrate)
No longer a standalone cull kernel. The McMahon check (`L_max = allometric_k × branch_radius^allometric_exponent`) now runs inside `integrate`'s tip branch: when `arc_total_since_lateral > L_max`, the tip's velocity is zeroed for the frame (stops growth; the tip itself stays alive and may still receive damage / dominance / Murray contributions). Toggled by `allometric_enable` (in-kernel). Parms unchanged: `allometric_k`, `allometric_exponent`.

Previous archive: standalone `allometric_cull` with `active=0` — switched to velocity-zero because killing tips orphaned their future children and produced frame-to-frame flicker; stopping velocity is gentler and composes with dominance/Murray continuing to work on the now-frozen tip.

### topology (Attribute Wrangle, point) — two spawn paths
Outer filter: `is_tree_node == 1 AND active == 1`. The `active` check skips dead nodes so any kernel that kills a tip mid-frame (currently `allometric_cull`; future: damage, disease) can't produce orphan children. Two independent spawn paths:

**Tip extension** — fires on `is_tip == 1 AND arc_since_spawn >= step_size`:
- Add child at `v@P` (parent's current position — tail). Polyline parent→child.
- Child: `v=parent.velocity × child_velocity_inherit`, `arc_since_spawn=0`, `att_*/acc_*/att_count=0`, `is_tip=1`, `is_tree_node=1`, `active=1`, `birth_time=@Time`.
- Parent: `is_tip=0`, `arc_since_spawn -= step_size`, `velocity={0,0,0}` (frozen interior).
- Early return — one spawn per point per frame.

**Interior branching (SCA-gated lateral spawn)** — fires on `is_tip == 0 AND att_count > 0 AND lateral_count < max_laterals_per_node AND lateral_pressure >= threshold`, where
```
threshold = lateral_base_threshold
          × (1 + split_depth × vigor_penalty  × (vigor_filter_enable > 0))
          × (1 + dominance_k  × f@dominance   × (dominance_enable    > 0))
```
— vigor and dominance modulators stack multiplicatively; each enable flag collapses its factor to 1 when disabled (zero-cost-when-off without a Switch-SOP):
- Orient via phyllotaxis + Leonardo pitch (independently toggled): `phyllo_enable` rotates azimuth around parent-tangent by `phyllo_angle × phyllo_index` (golden-angle spiral); `leonardo_enable` sets pitch from radius ratio `cos(θ) = (r_lateral / r_parent)²` (force-balance) instead of user-dialed `lateral_pitch`. Both off → fallback `dir = normalize(last_att_force)`. `init_v = dir × lateral_init_speed`.
- Add child at `v@P` (same position as parent). Polyline parent→child.
- Child: `velocity = init_v`, `split_depth = parent.split_depth + 1` (lateral increments depth — bifurcations only), `lateral_pressure = 0`, `last_att_force = {0,0,0}`, rest matches tip-extension child.
- Parent: `lateral_pressure = 0`, `last_att_force = {0,0,0}`. (Parent `att_*` is reset every frame by `vigor_accumulate`, not here.) Parent stays `is_tip=0`, `velocity={0,0,0}`.

**Swap vs classical SCA.** The former "immediate branch on `att_count > 0`" path was removed in vigor Stage 3. The threshold formula approximates apical dominance: at `vigor_penalty=0` all depths share one threshold (set it low to approximate classical behaviour with ~1-frame delay); at `vigor_penalty > 0` deep lineages take proportionally longer to sprout, producing a frontier-dominated tree without explicit hormone simulation.

Arc-length on the tip path preserves overshoot and is drift-free under variable `dt`. No `did_grow` / `new_P` / `target_P` writes — those attributes are gone.

Both spawn paths write `child.parent_id = @ptnum`. Post-split-streams, this is permanent — tree_node ptnums never change, so the raw ptnum reference stays valid for the lifetime of the tree.

### murray_propagate (OpenCL @KERNEL)
Per-frame tips→root radius propagation, single-hop. Runs over every tree node with a parent. Each node pushes `r^p × BG_MURRAY_SCALE` (int fixed-point, atomic-adds race-safe across siblings) into its parent's `i@murray_acc`. Transitive tip→root propagation happens over successive frames — one hop per frame is in sync with how fast topology adds children. `BG_MURRAY_SCALE = 1e4` in biogrowth.cl (int32 headroom: typical direct-child count ≤ 4, `r^p ≤ 1`, well within range).

### murray_finalize (OpenCL @KERNEL)
Runs over every tree node. Skips tips (they keep their spawn-time `branch_radius` — a tip has no children, so its accumulator is 0). For interior nodes: `branch_radius = (murray_acc / BG_MURRAY_SCALE)^(1/p)`, then zero `murray_acc` for next frame. Gated on `murray_enable` — when disabled, just clears the accumulator so `branch_radius` stays at spawn-time values; re-enabling picks up live without a sim restart.

Ordering with self_collide: `self_collide` on frame N uses `branch_radius` as written by murray_finalize at end of frame N-1 — one-frame lag, imperceptible at current step sizes.

**Reaction wood (gravity-biased Murray).** In-kernel toggle `reaction_wood_enable` inside `murray_propagate`. When on, the `r^p` contribution pushed into parent is multiplied by `1 + reaction_k × max(0, -dot(branch_axis, up))` where `branch_axis = normalize(child_P - parent_P)`. Only downward-pointing axes get a bonus — hanging/drooping branches thicken ancestors more than vertical ones. See [reaction-wood.md](archive/reaction-wood.md).

### dominance_step (OpenCL @KERNEL) — fused emit + propagate
Runs over every tree node with a parent. Tips atomic_add `dominance_strength × BG_DOMINANCE_SCALE` into parent's `i@dominance_acc`; interior nodes atomic_add `own dominance × dominance_decay × SCALE` into parent's acc. One pass, one atomic per node. Signal walks child → parent one hop per frame; transitive over time. See [apical-dominance.md](archive/apical-dominance.md).

### dominance_finalize (OpenCL @KERNEL)
Runs over every tree node. `f@dominance = dominance_acc / BG_DOMINANCE_SCALE; dominance_acc = 0`. Mirrors `murray_finalize` structure. Together with `dominance_step`, Switch-gated by `dominance_enable`.

### cull (OpenCL @KERNEL) — attractor proximity kill
Runs on the attractor-only stream (post-split). 27-cell neighbor search over the tree-node grid; any tree_node within `kill_radius` → `active=0`, return.

**Cross-input access.** Cull's primary input is the attractor stream (drives iteration and exposes attractor `@P`); Input 1 is the tree-only stream, bound as:
```cl
#bind point tree_P name=P float3 read input=1
```
Tree positions are read via `vload3(node_idx, (const __global float*) @tree_P.data)` where `node_idx` comes from `sorted_idx[]` (tree-stream ptnums, since the grid was built on the tree side).

The grid detail arrays (`cell_count`, `cell_start`, `sorted_idx`, etc.) propagate through the Blast SOP that performs the split, so they're available on the attractor stream without an explicit cross-input bind. (If the grid is ever moved to be authored only on the tree-only stream, those binds need `input=1` too.)

**No stall kill.** The previous `stall_frames` mechanism was retired 2026-04-19 — its original purpose (preventing static-tip over-spawn in the direct-step model) is now handled at the source by `arc_since_spawn` in topology. Archived with revival recipe in [optimization.md](./archive/optimization.md). Re-add if Phase 2 VDB obstacle constraints cause trapped-attractor accumulation.

**No dead-tip cull.** We tried it (Stage 2, Apr 2026) and dropped it: newborn tips flickered in/out before accumulating attract signal, and orphan tips are cheap (attract writes to them only when picked). If perf ever degrades from stranded tips, revisit with either a newborn grace period (`@Time - birth_time >= grace`) or a geometric bounds test (tip `P` outside scatter bbox → `active=0`).

### del_unactive1 + blast_unactive (attractor-only stream)
Group Expression creates `del` group from `@active==0 && @is_attractor==1` — the `is_attractor` guard is load-bearing: it prevents accidental deletion of any tree_node that might ever get `active=0`, preserving the split-streams ptnum invariant. Blast deletes. Runs every frame on the attractor stream only.

### del_grid_attr (Attribute Delete, post-merge)
Strips transient grid attribs before feedback: `cell_count`, `cell_start`, `cell_offset`, `sorted_idx`, `cell_size`, `total_cells`, `grid_min`, `grid_res`. Persistent attribs (`murray_acc`, `dominance_acc`, all point attribs) are not stripped.

---

## Biological rules

Synthetic reference for the rules layered onto the SCA core. Each rule is gated by a Switch-SOP `<rule>_enable` (where applicable) or in-kernel parm. Per-feature plan docs are archived in `doc/archive/` for historical reference.

### Vigor — depth-cycled grid filter + lateral pressure

Approximates apical dominance through two coupled effects: (1) interior nodes participate in the spatial grid only every `2^min(split_depth, depth_cap)` frames, with phase stagger by depth, so deep lineages effectively "see" attractors less often; (2) lateral pressure threshold scales with depth so deeper nodes need more accumulated pull to spawn.

- **Grid filter** (`grid_count` / `grid_scatter`): `bg_grid_eligible(split_depth, tick, vigor_depth_cap, vigor_phase_stride)` gates interior nodes. Tips always participate. Same predicate must run in both kernels or cell offsets corrupt.
- **Pressure accumulator** (in `integrate` interior branch): each frame, `lateral_pressure += length(att_signal) × dt`. Consumed in topology's interior-branch threshold.
- **Threshold modulator** (in topology): `threshold = lateral_base_threshold × (1 + split_depth × vigor_penalty × (vigor_filter_enable > 0)) × ...`. Multiplicative stack with dominance and damage consumers.

Parms: `vigor_filter_enable`, `vigor_depth_cap`, `vigor_phase_stride`, `vigor_penalty`, `lateral_base_threshold`.

### Self-collide — tip repulsion with ancestor exclusion

27-cell tip-only repulsion preventing fine roots from clustering. Ancestor walk (up to `ancestor_hops` levels each direction) excludes recent kin so a tip doesn't repel its own immediate parent / grandparent.

- Force per neighbour: `normalize(delta) × (1 - smoothstep(0, r_sum, d))` where `r_sum = (r_me + r_other) × self_collide_range`.
- Atomic-add into `force_acc` (single-writer per point post-split-streams; plain float3, no int fixed-point).
- Per-node radius `f@branch_radius` (Murray-driven on interior, spawn-time on tips).

Parms: `self_collide_strength`, `self_collide_range`, `ancestor_hops`. Switch-SOP: `self_collide_enable`.

### Allometric length limit (McMahon)

Tip growth slows / stops when arc-length-since-last-lateral exceeds a thickness-scaled maximum. `L_max = allometric_k × branch_radius^allometric_exponent`. Closes the Murray-radius → growth-extent feedback loop.

Inside `integrate`'s tip branch (in-kernel toggle, not Switch-SOP):
```
ratio = clamp(arc_total_since_lateral / L_max, 0, 1)
gf    = 1 - pow(ratio, allometric_curve)    # 1→0 as ratio→1
v *= gf
```

Refactored from a standalone cull kernel to in-kernel velocity-zeroing because killing tips orphaned future children and produced flicker. Now stacks multiplicatively with branch-autonomy and damage growth-factors.

Parms: `allometric_enable`, `allometric_k`, `allometric_exponent`, `allometric_curve`. Schema: `f@arc_total_since_lateral` (accumulated in integrate, reset on lateral spawn).

### Senescence — per-lineage maturity

Per-lineage growth arrest. Mirror of allometric's shape but uses cumulative path from seed instead of since-last-lateral, so it bounds total lineage length rather than per-segment length. Tips that have grown a long way down a long lineage slow and freeze. Composes commutatively with allometric (per-segment) and the other velocity modifiers — both fire in the modifier chain between `integrate` and `apply`.

```
ratio = clamp(arc_total_since_seed / senescence_max_arc, 0, 1)
gf    = 1 - pow(ratio, senescence_curve)
v *= gf
```

Tip-only filter; runs in its own OCL kernel (`biogrowth_senescence.cl`) so it can be Switch-gated independently. Parms: `senescence_enable`, `senescence_max_arc`, `senescence_curve`. Schema: `f@arc_total_since_seed` accumulated in `apply`, inherited at every spawn.

### Branch autonomy — light-debt growth scaling

Per-node `f@light_credit` accumulates from a scalar `irradiance` VDB on the constraints bus, decays slowly. Tip velocity scales by `pow(light_credit / mortality, curve)` — shaded tips dim smoothly. No deletion (preserves the split-streams invariant); damage produces *freeze*, not vanish. Render-side fade is the path for visual self-pruning.

Fully fused inside `integrate`:
- **Pre-split (all tree_nodes)**: `light_credit += (intake - drain) × dt` where `intake = irradiance × intake_gain` and `drain = damage × intake_drain × (damage_enable > 0)`. Multiplicative decay `*= (1 - decay × dt)`.
- **Tip branch**: `v *= pow(clamp(light_credit / mortality, 0, 1), curve)` after allometric.

`irradiance` is a **scalar** VDB carried on the collision bus (Input 3) alongside `collision_sdf` / `avoid_weight` / `response_type`, imported via `import_collision_and_irradiance` Object Merge. Distinct from the float3 `photo` directional field on the constraints bus used by phototropism. Diffuse-shade scenes need both: `irradiance` carries magnitude (for credit accumulation), `photo` carries direction (for tip steering).

Parms: `branch_autonomy_enable` (in-kernel), `light_credit_grace`, `light_credit_decay`, `light_credit_mortality`, `light_credit_curve`, `light_credit_intake_gain`. Schema: `f@light_credit` seeded to grace value, inherited at every spawn.

### Murray's rule — radius propagation

Per-frame tips→root `r_parent^p = Σ r_child^p` propagation via `BG_MURRAY_SCALE` int fixed-point accumulator. Replaces geometric per-depth taper with biology-backed thickness from descendant count. Single-hop per frame; transitive over time. `p=2` (da Vinci pipe model) or `p=3` (true Murray hydrodynamic) — `p=3` recommended to keep seed radius proportional in large trees.

Two-kernel pattern (atomic-write + finalize-read can't safely fuse):
- **murray_propagate**: each child atomic_adds `r^p × scale × mult × damage_factor` into parent's `murray_acc`. `mult` carries reaction-wood bias; `damage_factor` carries the damage modulator.
- **murray_finalize**: interior `branch_radius = (acc / scale)^(1/p)`, then reset acc.

Parms: `murray_enable`, `murray_exponent`, `damage_enable`. Switch-SOP gates the propagate+finalize pair.

### Reaction wood — gravity-biased Murray

In-kernel toggle inside `murray_propagate`. Multiplies the `r^p` push-to-parent by `1 + reaction_k × max(0, -dot(branch_axis, up))`. Drooping branches (downward axis) thicken ancestors more than vertical ones. One-line variation on Murray, no separate kernel.

Parms: `reaction_wood_enable`, `reaction_k`, `up`.

### Apical dominance — auxin-like suppression

Tips emit a suppression signal that flows child → parent (basipetally) one hop per frame, attenuating with `dominance_decay`. Interior nodes propagate their own dominance further. Damaged tips emit less; lose a leader and dominance drains over a few frames → dormant lateral buds upstream wake up (emergent reiteration).

Two-kernel pattern (same shape as Murray):
- **dominance_step** (fused emit + propagate): tips push `strength × (1 - damage × damage_enable)` into parent.acc; interiors push `own dominance × decay` into parent.acc.
- **dominance_finalize**: `f@dominance = acc / scale; acc = 0`.

Consumer: topology threshold modulates by `(1 + dominance_k × dominance × (dominance_enable > 0))`. Multiplicative stack with vigor and damage.

Parms: `dominance_enable`, `dominance_strength`, `dominance_decay`, `dominance_k`. Schema: `i@dominance_acc`, `f@dominance`.

### Damage — orthogonal event bus

Per-node `f@damage ∈ [0,1]` written by event sources, consumed by other rules via `(1 - damage × damage_enable)` modulators. Decoupled from any single rule — composes with everything. Children inherit parent damage at spawn (continuation, not a propagation hop).

**Sources:**
- **`damage_field` kernel** — VDB-driven (pyro heat, painted danger zones, animated burn fields). Reads scalar `damage_field` VDB on constraints bus, accumulates `(sample - threshold) × gain × dt` per frame. Switch-SOP `damage_field_enable`.
- **`damage_propagate` kernel** — parent → child `max(self, parent × decay)` propagation. State inheritance, not event accumulation. One hop per frame; transitive over time. Switch-SOP `damage_propagate_enable`.
- **Scripted breakage** — drop a small wrangle on the tree-only stream for one-off events: `if (@Frame == N && @ptnum == X) @damage = clamp(@damage + 1.0, 0, 1);`. Tree_node ptnums are stable post-split-streams, so direct ptnum references work.
- **Collision impulse** (future): inside `integrate`'s collision stage, accumulate `damage += impulse_gain × push` when push exceeds threshold.

**Consumers** (all gated by master `damage_enable`):
- **Dominance emission** — damaged tips emit less suppression → reiteration upstream of damage.
- **Murray contribution** — damaged subtrees stop pumping radius into ancestors → trunks visibly thin.
- **Tip growth-factor** (in `integrate`) — damaged tips slow / freeze. Stacks with allometric + branch autonomy.
- **Branch autonomy intake drain** — damaged branches die faster under marginal light.

Parms: `damage_enable`, `damage_field_gain`, `damage_field_threshold`, `damage_propagate_decay`, `damage_intake_drain`. Schema: `f@damage` seeded 0, inherited at every spawn.

### Phyllotaxis + Leonardo — lateral spawn orientation

In-VEX inside topology's interior-branch spawn. Two independent toggles, four combinations:
- **Phyllotaxis** (azimuth): rotates the lateral's azimuth around the parent tangent by `phyllo_angle × phyllo_index` per spawn. Golden-angle (137.5°) produces spiral arrangements.
- **Leonardo** (pitch): force-balance angle from radius ratio, `cos(θ) = (r_lateral / r_parent)²`. Thin laterals peel off near 90°, fat ones stay shallow.
- **Both off**: fallback `dir = normalize(last_att_force)` (attractor-driven).
- **Per-node lateral cap**: `i@lateral_count` gated by `max_laterals_per_node` prevents node crowding when `kill_radius` is small.

Parms: `phyllo_enable`, `phyllo_angle`, `leonardo_enable`, `lateral_pitch`, `lateral_init_speed`, `max_laterals_per_node`. Toggles use multiplicative-by-flag pattern (no Switch-SOP).

### Sweep meshing (post-solver)

Per-frame tube meshing outside the solver: `tree_nodes → polypath → resample → blur_radius → set_pscale → sweep`. Reads `f@branch_radius` (Murray-driven) and produces a Round-Tube mesh with col=6. `i@is_lateral_start` is render-time metadata for the deferred lateral-offset wrangle that hides Y-junction seams by tucking lateral first-vertex inside the parent tube.

Watertight Y-junction meshing is deferred — would require a `sweep → VDB from Polygons → Convert VDB` post-pass.

---

## Attribute schema

**Tree nodes:**
- Identity: `P`, `is_tree_node=1`, `is_attractor=0`, `active=1`, `is_tip` (1=tip, 0=interior), `birth_time`.
- Physics state (tips; zeroed on interior): `v@velocity`, `f@arc_since_spawn`.
- Arc-length state:
  - `f@arc_total_since_lateral` — arc since most recent lateral spawn (or seed for trunk lineage). Resets on lateral spawn, inherited on tip extension. Consumed by `allometric`.
  - `f@arc_total_since_seed` — cumulative arc from root, never resets. Inherited on every spawn. Consumed by `senescence`.
- Branch autonomy: `f@light_credit` — light-debt accumulator; seeded to `light_credit_grace`, inherited at every spawn. Updated each frame inside integrate from the `irradiance` VDB.
- Damage: `f@damage` — event-bus scalar in `[0, 1]`; seeded 0, inherited at every spawn. Written by `damage_field` and `damage_propagate`; read by dominance, Murray, integrate (light-credit drain + tip growth-factor) consumers.
- Attract accumulator (reset every frame by integrate on both tip and interior branches): `i@att_ix/iy/iz`, `i@att_count`.
- Shared contributors accumulator (reset every frame by integrate): `v@force_acc` (plain float3; writers all target their own point, no atomic needed).
- Vigor state: `i@split_depth` (bifurcation count; seed=0, increments on lateral spawn only), `f@lateral_pressure` (interior pull accumulator, reset on lateral spawn), `v@last_att_force` (direction of last accumulated pull, used as fallback orientation when both `phyllo_enable` and `leonardo_enable` are off).
- Phyllotaxis / cap state: `i@lateral_count` (per-node counter of laterals spawned from this node; gated by `max_laterals_per_node`), `i@phyllo_index` (azimuth rotation counter; inherits + increments on tip-extension, resets on lateral spawn).
- Apical dominance: `i@dominance_acc` (int fixed-point accumulator; children atomic_add their decayed dominance into parent's slot each frame; reset by finalize), `f@dominance` (post-finalize float, consumed by topology threshold).
- Self-collide state: `i@parent_id` (raw ptnum of parent; set at spawn; permanent post-split-streams since tree_node ptnums never change; seed=-1), `f@branch_radius` (per-node radius; seed=`base_radius`, mainline inherits unchanged, lateral multiplies by `taper_decay` — then overwritten every frame on interior nodes by Murray's finalize).
- Murray state: `i@murray_acc` (fixed-point accumulator for `Σ r_child^p × BG_MURRAY_SCALE`; reset each frame by finalize on interior nodes, always 0 on tips).
- Sweep-meshing marker: `i@is_lateral_start` (set to 1 on lateral children in topology; 0 otherwise). Read by the post-solver lateral-offset wrangle in the mesh chain.

**Attractors:** `P`, `is_tree_node=0`, `is_attractor=1`, `active=1`, `closest_node` (debug/viz).

**Detail (persistent through sim):** none. (Pre-split-streams had `next_stable_id` + `sid_to_ptnum`; both gone.)

**Detail (transient, stripped by del_grid_attr):** `cell_count[]`, `cell_start[]`, `cell_offset[]`, `sorted_idx[]`, `cell_size`, `total_cells`, `grid_min`, `grid_res` — stripped at end of frame; rebuilt next frame.

---

## Force contract

Every contributor (attract + shared) must:
1. Pre-normalize its direction to unit length.
2. Scale by its own `<name>_strength` parm.
3. Skip non-tips in its inner loop.
4. Write to the appropriate accumulator:
   - `attract` → `att_*` + `att_count` (averaged in integrate).
   - All others → shared `v@force_acc` (raw vector sum; each contributor's strength is its own knob; single-writer-per-point so plain `@force_acc.set(@force_acc + force)`).

Fixed-point accumulation (`BG_DIR_SCALE = 1e4` + `bg_atomic_add_dir` / `bg_decode_dir` in `biogrowth.cl`) applies only to `att_*`, where attractors genuinely race on picking the same tip. int32 max ≈ 214k unit-vectors per axis, safe for hotspot tips. Reintroduce the same pattern locally on any future cross-point contributor (e.g. pairwise self_collide, dominance suppression) — see [acc-int-atomic-revival.md](archive/acc-int-atomic-revival.md).

**Naming convention.** `_acc` suffix on an attribute = **accumulator** (persistent per-point, written by one or more contributors, consumed/reset per frame). Examples: `v@force_acc`, `i@murray_acc`, future `i@dominance_acc`, future `f@damage_acc`. Acceleration is never a persistent attribute — it lives as a local variable (`float3 a`) inside `integrate`. Exception: `att_*` predates the convention and is left as-is — the `att_` prefix is its own namespace.

See the plan's "Force contract enforcement" guardrail for failure modes.

---

## Solver spare parms

| Parm | Type | Default | Notes |
|---|---|---|---|
| `step_size` | float | 0.2 | topology rhythm (arc-length threshold) |
| `perception_radius` | float | (project) | attract search + grid cell size |
| `kill_radius` | float | 0.05 | attractor proximity-kill radius |
| `attract_strength` | float | 1.0 | scales attract's force on its winner |
| `damping` | float | 2.0 | `(1 - damping*dt)` — must be < 1/dt |
| `max_speed` | float | `2 × step_size / dt` | velocity clamp; must be ≥ step_size/dt |
| `tropism` | float3 | `{0,-1,0}` | built-in tropism direction |
| `tropism_strength` | float | 0.0 | off by default; turn up for biased growth |
| `child_velocity_inherit` | float | 0.5 | `child.v = parent.v × this` on spawn |
| `vigor_filter_enable` | int | 1 | toggle depth-cycled grid filter in grid_count/grid_scatter |
| `vigor_depth_cap` | int | 2 | max period exponent; depth ≥ cap → every `2^cap` frames |
| `vigor_phase_stride` | int | 1 | phase stagger; frame offset = `split_depth × this` |
| `lateral_base_threshold` | float | 0.5 | `lateral_pressure` required at depth 0 to sprout a lateral (was `vigor_base_threshold`) |
| `vigor_penalty` | float | 1.0 | threshold growth per depth: `base × (1 + d × penalty)` |
| `max_laterals_per_node` | int | 4 | per-node cap on `lateral_count`; saturated nodes stop spawning regardless of pressure |
| `phyllo_enable` | int | 0 | golden-angle azimuth rotation on lateral spawn direction |
| `phyllo_angle` | float | 137.5 | azimuth rotation per successive lateral on same axis |
| `leonardo_enable` | int | 0 | Leonardo pitch: `cos(θ) = (r_lat / r_parent)²` overrides `lateral_pitch` |
| `lateral_pitch` | float | 0.0 | pitch lerp when Leonardo off: 0 = perpendicular, 1 = parallel to parent |
| `lateral_init_speed` | float | 0.01 | initial speed magnitude of lateral child |
| `base_radius` | float | 0.05 | seed `branch_radius`; must be ≤ `perception_radius` (ideally ≤ `0.5 × perception_radius`) |
| `taper_decay` | float | 1.0 | per-bifurcation radius multiplier on lateral spawn (was 0.8 pre-Murray; set <1 to start laterals thinner and let Murray amplify) |
| `self_collide_strength` | float | 1.0 | scales the self-collide force |
| `self_collide_range` | float | 1.0 | multiplier on `r_me + r_other` — personal-space buffer (>1 = buffer, <1 = overlap allowed) |
| `ancestor_hops` | int | 4 | bidirectional `parent_id` walk depth for ancestor exclusion |
| `murray_enable` | int | 1 | Switch-gate for `murray_propagate` + `murray_finalize` |
| `murray_exponent` | float | 2.0 | Murray's p: 2=da Vinci / pipe model (cross-section conservation), 3=Murray (hydrodynamic optimum) |
| `allometric_enable` | int | 1 | Switch-gate for the `allometric` velocity-modifier kernel |
| `allometric_k` | float | ~2.0 | scene-scaled; L_max = k × r^exp. Rough start: `step_size × 100 / base_radius^exp` |
| `allometric_exponent` | float | 0.6667 | McMahon canonical; lower = more aggressive thin-branch stop |
| `allometric_curve` | float | 2.0 | sharpness of growth-factor falloff as ratio approaches 1 |
| `senescence_enable` | int | 0 | Switch-gate for the `senescence` velocity-modifier kernel |
| `senescence_max_arc` | float | 10.0 | scene-scaled; cumulative path-from-seed at which growth fully arrests |
| `senescence_curve` | float | 2.0 | sharpness of growth-factor falloff |
| `reaction_wood_enable` | int | 0 | gravity-biased Murray contribution in `murray_propagate` |
| `reaction_k` | float | 0.3 | strength of reaction-wood bias; `mult = 1 + k × max(0, -dot(axis, up))` |
| `dominance_enable` | int | 1 | Switch-gate for `dominance_step` + `dominance_finalize` |
| `dominance_strength` | float | 1.0 | per-frame tip emission of dominance signal |
| `dominance_decay` | float | 0.7 | per-hop attenuation as signal walks toward root |
| `dominance_k` | float | 1.0 | topology consumer multiplier; scales threshold bump from dominance |
| `vigor_enable` / `self_collide_enable` / `surface_enable` / `density_enable` / `photo_enable` | int | per-feature | Switch-gates for optional subsystems (bypass node when 0) |
