# VDB Constraints — Plan

Resumes the pre-physics VDB plan (commit 46fe457, which stored notes inside the .hiplc) on top of the landed physics model. Branch: `feature/vdb-constraints` (rebased on `develop` post-physics merge, 2026-04-18).

## Context

The physics refactor (`feat: physics integrator...`) was built specifically to host this plan. Each VDB constraint becomes an OCL force-contributor node in the chain between `attract` and `integrate`. Ordering is commutative — they all write `normalized_dir × strength` into the shared `v@force_acc` accumulator via `@force_acc.set(@force_acc + force)` (single-writer-per-destination, no atomic needed). `integrate` sums `@force_acc` with `att_*` and tropism on the next pass.

**Force contract** (from biogrowth-plan.md, repeated here):
1. Inner loop skips non-tips (`is_tip < 0.5 continue`).
2. Emit pre-normalized direction scaled by this contributor's own `<name>_strength` parm.
3. Write via `@force_acc.set(@force_acc + dir)`. No count, no averaging. Plain float3 — each writer targets its own point, no atomic.

Violating (1) wastes writes; violating (2) produces magnitude blow-up near strong gradients.

## Contributors

### 1. Avoid + Collision — *soft anticipation + hard resolver on one SDF*

**Fused into `integrate` (2026-04-19).** Originally built as two separate OCL nodes (`avoid` pre-integrate writing to `v@force_acc`, `collision` post-integrate editing P/v), they were merged into the integrator as stages 2 and 4 of its kernel. Reason: they share `collision_sdf`, form a single "wall handling" subsystem, and separating them cost 3× dispatches + 3× full-point filter + an int-atomic encode/decode roundtrip for avoid's force. The sections below describe the behaviour — see [network.md](../network.md#integrate-opencl-kernel--fused-tip-physics) for the fused kernel layout.

Surface behaviour has two phases reading the same `collision_sdf`:

**1a. Avoid (soft, inline stage 2 of integrate, contributes to `a`).**
**Inputs:** `collision_sdf` + `avoid_weight` from the **collision bus** (solver Input 3).
**Kernel:** for each tip, if `sdf < avoid_range` AND the tip is moving into the surface (`dot(velocity, n) < 0`):
```
n       = normalize(grad_sdf)                   // outward
vn      = dot(velocity, n)                      // signed approach speed
r       = velocity - 2·vn·n                     // reflect v across surface
dir     = normalize(r)                          // steer along reflection
falloff = 1 - smoothstep(0, avoid_range, sdf)   // 1 at surface, 0 at range
scale   = avoid_strength × falloff × sample(avoid_weight)
acc_*  += dir × scale
```
**Parms:** `avoid_strength`, `avoid_range`.
**Why reflect velocity instead of pushing along the normal:** a pure normal push drives head-on tips straight back (counterproductive — just stops growth) and over-pushes grazing tips. The velocity-reflection direction bends motion naturally along the wall: head-on → full reversal; oblique → smooth deflection; grazing → nearly no change. Mirrors the geometry of the hard `collision` rebound but applied as a soft force over a wider band.
**Why this exists alongside hard collision:** a force field can't *guarantee* non-penetration — a fast tip on a low `avoid_strength` will still punch through. `avoid` gives smooth anticipatory curving; `collision` is the authoritative last-word resolver for whatever the soft force didn't catch.

**1b. Collision (hard, inline stage 4 of integrate, edits `P_new` / `v` / `arc`).**
**Inputs:** `collision_sdf` + `response_type` from the collision bus.
**Kernel:** if `sdf < surface_band`:
```
n    = normalize(grad_sdf)
push = surface_band - sdf                       // >= 0
P               += n × push                     // snap out
arc_since_spawn -= push                         // teleport isn't traveled distance
type = round(sample(response_type))             // 0=none, 1=stop, 2=slide, 3=rebound
if type != 0 and dot(v, n) < 0:
  if type == 1:  v  = 0
  if type == 2:  v  = (v - dot(v,n)*n) × (1 - friction)
  if type == 3:  v  = (v - 2*dot(v,n)*n) × restitution
```
**Parms:** `surface_band`, `friction`, `restitution`.
**Why post-integrate:** snapping `P` mid-contributor-chain would corrupt downstream force samples (they'd see the post-snap position against the pre-snap velocity). Running after integrate makes it authoritative — it's the last word on where the tip ends the frame.

**Upstream authoring (unified-SDF pattern).** Multiple logical colliders combine *outside* the solver into VDBs on the collision bus. No DOP wrapping, unbounded collider count:
- `collision_sdf` = VDB Combine (min / union) of every collider SDF.
- `avoid_weight` = scalar VDB for soft-force modulation. Uniform: constant 1. Per-region: paint, voronoi-key, noise.
- `response_type` = scalar VDB encoding `{0,1,2,3}` (none/stop/slide/rebound) keyed by collider region. Authored via voronoi + attribute transfer or a switch per SDF primitive before the Combine.

**Why first:** single conceptual feature (avoid+collision), easy to verify (tips curve around a sphere, don't penetrate it, slide along a plane).

### 2. Surface-project — *attract to isosurface*
**Input:** SDF VDB (reuse or separate — parm-bound).
**Kernel:** pull tips toward the `target_iso` level (default 0). Sign flips across the surface:
```
delta  = sdf_value - target_iso
dir    = normalize(-grad_sdf) × sign(delta)   // toward the surface
weight = smoothstep(surface_range, 0, |delta|)
acc_* += dir × surface_strength × weight
```
**Parms:** `surface_strength`, `surface_range`, `target_iso` (float, default 0).
**Use:** grow on/near a rock face. Can be combined with collision (same SDF, surface_range ≪ collision_range) to make "grow near, don't penetrate."

### 3. Density follow — *up-gradient steering*
**Input:** scalar density VDB (`density`, float).
**Kernel:**
```
grad   = grad_density(P)
dir    = bg_safe_normalize(grad)
weight = saturate((sample_density(P) - density_threshold) / density_range)
acc_* += dir × density_strength × weight
```
**Parms:** `density_strength`, `density_threshold`, `density_range`.
**Use:** nutrients, humidity, soil depth.

### 4. Phototropism / spatial tropism — *direct vector sample*
**Input:** vector VDB (`light_dir`, float3). Authored up-gradient by an upstream volume network (or computed from light source geometry).
**Kernel:**
```
v      = sample_vector(light_dir, P)
dir    = bg_safe_normalize(v)
acc_* += dir × photo_strength
```
**Parms:** `photo_strength`.
**Use:** global light direction, custom biased tropism fields. Equivalent to scalar tropism with per-location direction.

## Cross-cutting

### VDB binding in SOP OCL
`#bind vdb <name> float input=N read` (scalar) or `float3` (vector). **Not `#bind volume`** — that's for native Houdini volumes. Sampling: `@name.worldSample(P)` and `@name.worldGradient(P)` (native, cnanovdb-backed). No central-diff fallback needed.

**Every VDB binding is `optional`.** Add the flag to the bind line and guard usage with `@<name>.bound` inside the kernel. When the VDB isn't wired (upstream bus empty of that named primitive), the contributor early-returns (plain contributors) or skips its stage (fused integrate: avoid+collision disable together when `collision_sdf` unbound). This means every constraint — avoid, collision, surface, density, photo — is individually optional at authoring time. A scene with no colliders, no surface, no density field still cooks: each bound-check is a one-cycle early-out. Default values for companion VDBs if only the primary is wired: `avoid_weight → 1.0`, `response_type → 2 (slide)`.

### Node placement
Inside solver:
```
... → attract → [surface] → [density] → [photo] → integrate → cull → ...
```
Avoid + collision are fused inside `integrate` (stages 2 and 4 of its kernel) and are no longer separate nodes. General soft contributors (`surface` / `density` / `photo`) remain pluggable nodes between `attract` and `integrate`, writing into `acc_*` commutatively.

### Input routing
Three solver inputs:
Solver inputs are 0-based (matches Houdini):
- **Input 0** — seed (one-shot, frame-1).
- **Input 1** — streaming attractors (sim feedback).
- **Input 2** — constraints bus (soft force VDBs). Imported inside the solver via `import_constraints` Object Merge (`opinputpath("../", 2)`).
- **Input 3** — collision bus (`collision_sdf` + `response_type` + `avoid_weight`). Imported via `import_collision` Object Merge (`opinputpath("../", 3)`).

VDB buses never enter sim feedback — they're read-only secondary inputs, matching DOP FLIP/MPM conventions. The fused `integrate` node binds the collision bus (Input 3) directly because it hosts both the avoid (stage 2) and collision (stage 4) inline logic.

### Performance
- One VDB sample per tip per contributor (maybe +6 for central-diff gradient) — cheap vs. attract's 27-cell search.
- Gradient cost is the main concern; reuse gradient across co-located contributors (collision + surface share SDF grad) if profiling warrants.

## Staging

### Stage 0 — plumbing ✅ landed 2026-04-19
- `biogrowth_vdb_probe.cl` no-op contributor proved the chain end-to-end.
- Confirmed `#bind vdb` (not `#bind volume`); `worldSample` / `worldGradient` work.

### Stage 1 — avoid + collision ✅ landed 2026-04-19 (fused into integrate)
- Initially shipped as two separate OCL nodes; same-day refactor fused both into the `integrate` kernel as stages 2 (avoid, soft) and 4 (collision, hard). See [network.md](../network.md#integrate-opencl-kernel--fused-tip-physics).
- Parms on the integrate node: `avoid_strength`, `avoid_range`, `surface_band`, `friction`, `restitution`.
- `integrate` node binds the collision bus (solver Input 3) on its Input 1 for `collision_sdf`, `avoid_weight`, `response_type`.
- Verify: tips curve around sphere; never end frame with `sdf < 0`; slide along a plane.

### Stage 2 — surface-project
- Build `surface` OCL node reusing the SDF input.
- Verify: tips stick to a plane's surface.
- Combined test: sphere collision + plane surface — tips grow along the plane, route around sphere.

### Stage 3 — density follow
- Build `density` OCL node. Gaussian blob scalar field test.
- Verify: tips concentrate in high-density regions.

### Stage 4 — photo / spatial tropism
- Build `photo` OCL node. Uniform direction field test (should match scalar tropism).
- Then: radial field from a point light (tips curve toward the light).

### Stage 5 — cleanup
- Sync `hou/scripts/ocl/sop/*.cl` tracking copies.
- Update `doc/network.md` with the new contributor chain.
- Update `project_biogrowth.md` memory.

## Phase 3 (out of scope for this plan)

Mentioned for continuity, not built here:
- **Taper radius per node** — per-node float attribute driven by depth/age, consumed by the downstream Sweep SOP to give each polyline a thickness profile.
- **Sweep meshing** — outside the solver, turns polylines into tubes. Material/render setup downstream.
- **Self-collision via nearest-branch repulsion** — new OCL contributor `self_collide`, lives inside the solver alongside surface/density/photo. For each tip, 27-cell neighbor search over tree nodes (reuses the existing grid); skip self + near-ancestors via a short `parent_id` hop walk (4–5 hops covers "immediate lineage," beyond that geometric distance handles exclusion naturally); accumulate soft repulsion force when a non-ancestor node is within `branch_radius`. Writes to `acc_*` like any other soft contributor. Replaces the earlier proposal of a Sweep→voxelize→feedback-VDB loop: per-frame updates instead of N-frame stale, no meshing in the hot loop, fits the plugin chain rather than bolting on a separate subsystem. Branch radius is the same per-node attribute driving downstream Sweep, so the data is authored once. Edge case (thin branch crossing between sampled nodes on an older polyline) is handled by upgrading point-distance to closest-point-on-segment if it shows up visually.

## Future extensions

- **Per-contributor falloff ramp.** Current falloff across contributors is a fixed `1 - smoothstep(0, range, |d|)` (C¹-continuous, no edge at range boundary). If a scene proves smoothstep wrong, add a Houdini `Ramp` parm `<name>_falloff_ramp` per contributor, bake it into a 1D float array as a detail attrib (32–64 samples is plenty), and lerp inside the kernel. Deferred until there's evidence the default is actually limiting — a ramp is more work than a discrete dropdown but is the only approach that doesn't sprawl into `<name>_falloff_type` + `<name>_falloff_k` parm pairs per contributor.

## Open questions

- **Gradient support in your Houdini OCL runtime** — does it expose a built-in VDB gradient sample, or do we implement central differences? Affects Stage 0 helper.
- **VDB input count** — are we OK adding Input 3, 4, 5 to the solver, or consolidate via Object Merge from a dedicated "vdb_inputs" null outside the solver?
- **Strength normalization across contributors** — today each contributor picks its own scale. Are we fine with that, or should the plan include a per-contributor weight visualization tool for tuning (e.g., color tips by which force dominated this frame)?
