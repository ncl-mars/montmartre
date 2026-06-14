# Murray's Rule — Landed 2026-04-20

**Status:** Stage 0–2 landed on `feature/murray-rule`. `murray_enable` defaults to 1; `taper_decay` defaults to 1.0. Stage 1 shipped alongside a stable-id refactor that unblocked a latent cross-frame ptnum corruption — see "Surprise: stale parent_id bug" below.

Replaces the per-spawn static `branch_radius` with a per-frame tips→root propagation: each interior node's radius is derived from the sum of its children's radii raised to a user-tunable exponent. The current geometric-per-depth taper (`taper_decay`) is kept as a seed-time modifier but defaults to `1.0` — effectively bypassed. Radius now emerges from the tree's actual branching structure, not from depth alone.

## Context

Today's `branch_radius` is static per node: assigned at spawn (seed = `base_radius`, mainline inherits, lateral multiplies by `taper_decay`), never updated after. This decouples trunk thickness from how much the trunk actually "feeds" — a sparsely-branched depth-5 parent and a densely-branched depth-5 parent have identical radii. Visually crude; biologically wrong.

Murray's rule / pipe-model propagates radius upward: `r_parent^p = Σ r_child^p`. At `p=2` this is the pipe model / da Vinci's rule (cross-section conservation); at `p=3` the original Murray's law for hydrodynamic optimum. Exposing `p` as a parm lets us A/B visually without code changes. Emergent behaviour: trunks thicken exactly where branches accumulate; sparse lineages stay thin.

Downstream consumers (`self_collide` for collision radius, `sweep-meshing` for tube scale) both read `branch_radius` unchanged — the rule swaps *what writes the attribute*, not who reads it. `self_collide` sees propagated radii with a one-frame lag (Murray runs after topology, collision runs before next frame's topology) — imperceptible at current step sizes.

## Design decisions (user-confirmed)

- **Exponent as runtime parm.** `murray_exponent` (float, default 2.0 — da Vinci / pipe model). Set to 3.0 for true Murray's law. Non-integer values are fine; the kernel uses `pow` either way.
- **Keep `taper_decay` as a bypass-able seed-time modifier.** Default changes to `1.0` (no taper at lateral spawn). Setting it to `<1` still gives laterals a smaller starting radius at spawn, which propagates through Murray's rule the next frame. Convenient knob for "thinner new twigs" without removing the mechanism.
- **Inline, every frame, post-topology.** Murray runs as a two-kernel pass (`murray_propagate` + `murray_finalize`) after `topology` in the solver. `self_collide` on frame N uses the radius propagated on frame N-1 — effectively current to within one step_size.
- **Per-parent reduction via atomic-add on an int fixed-point accumulator.** Same pattern as `acc_*`: each node writes its `r^p` scaled by `BG_MURRAY_SCALE` into its parent's `i@murray_acc`. Finalize decodes, takes the `1/p` root, writes `branch_radius`.
- **Single-hop per pass is enough.** Each frame runs one propagation step (child → direct parent only). Transitive propagation (tip → root) happens over successive frames as the tree grows — which is the same timescale at which children are added anyway. No multi-pass inner loop needed; the sim's temporal evolution is the outer loop.
- **Tips are untouched by finalize.** A tip by definition has no children, so its `murray_acc` stays `0`. Finalize filters on `is_tip==0` and overwrites only interior radii. Tips keep their spawn-time `base_radius` (possibly modified by `taper_decay`).

## Attribute schema delta

**Tree nodes — add:**
- `i@murray_acc` (int, seed=0). Fixed-point accumulator for `Σ r_child^p × BG_MURRAY_SCALE`. Reset to 0 by `murray_finalize` each frame on interior nodes; remains 0 on tips (never written to). Initialized to 0 in `set_seed` and both topology spawn paths. Zero-init defensively in `set_attractor`.

**Include — add:**
- `BG_MURRAY_SCALE` constant in `biogrowth.cl`. Proposed value: `1e4` (same as `BG_DIR_SCALE`). Typical direct-child count ≤ 4, `r^p ≤ 1.0`; sum × scale well within int32 range. Document under "force contract" comment block.

## Kernels

### `biogrowth_murray_propagate.cl`
Runs over every tree node. Each node pushes its `r^p` to its parent's accumulator.

```cl
#bind point   P             float3 read
#bind point   is_tree_node  int    read
#bind point   parent_id     int    read
#bind point   branch_radius float  read
#bind point  &murray_acc    int
#bind parm    murray_exponent float val=2.0

@KERNEL
{
    if (@is_tree_node < 0.5) return;
    if (@parent_id    <  0 ) return;   // seed has no parent

    __global int* acc = (__global int*)@murray_acc.data;

    float v = pow(@branch_radius, @murray_exponent);
    int   q = (int)(v * BG_MURRAY_SCALE + 0.5f);
    atomic_add(&acc[@parent_id], q);
}
```

Note: `&P` is bound read-only via the normal binding (no writable attribute), so the kernel needs a writable detail or different first-writable. Actually the writable attribute for SOP OCL must exist — bind `&murray_acc` as the writable, it determines iteration over points. Confirm in implementation.

### `biogrowth_murray_finalize.cl`
Runs over every tree node. Interior nodes decode, take the `1/p` root, write `branch_radius`, and reset the accumulator. Tips pass through untouched.

```cl
#bind point   P              float3 read
#bind point   is_tree_node   int    read
#bind point   is_tip         int    read
#bind point  &branch_radius  float
#bind point  &murray_acc     int
#bind parm    murray_exponent float val=2.0
#bind parm    murray_enable   int   val=1

@KERNEL
{
    if (@is_tree_node < 0.5) return;
    if (@is_tip       > 0.5) return;      // tips keep their spawn-time radius
    if (@murray_enable< 0.5) { @murray_acc.set(0); return; }

    float acc = (float)@murray_acc / BG_MURRAY_SCALE;
    if (acc > 0.0f) {
        float r = pow(acc, 1.0f / @murray_exponent);
        @branch_radius.set(r);
    }
    @murray_acc.set(0);
}
```

When disabled, the kernel just resets the accumulator and skips the update so `branch_radius` stays at spawn-time values. Re-enabling picks up live without a sim restart.

## Pipeline position

```
… attract → [soft contributors] → self_collide → integrate → cull → vigor_accumulate → topology
  → murray_propagate → murray_finalize
  → del_unactive → del_grid_attr
```

After `topology` so newly-spawned children participate the same frame. Before `del_unactive` / `del_grid_attr` so dead nodes' contributions don't leak into the propagation (they'll be stripped next — but the accumulator was cleared by finalize anyway). `self_collide` on frame N+1 reads the `branch_radius` that murray wrote at the end of frame N — one-frame lag.

## Parm changes

**New on solver:**
- `murray_enable` (int, 1)
- `murray_exponent` (float, 2.0)

**Default change:**
- `taper_decay` default from `0.8` → `1.0`. Existing `self_collide` scenes that were relying on per-depth taper will now see trunk thickening driven by Murray instead — *intentional*, this is the whole point of the feature. If an artist wants the old depth-based taper, set `taper_decay < 1` and optionally disable Murray (`murray_enable=0`).

## Staging

### Stage 0 — schema (no behaviour change)
- Add `i@murray_acc = 0` to `set_seed`, `set_attractor`, and both topology spawn paths.
- Add `BG_MURRAY_SCALE` constant to `biogrowth.cl`.
- Promote `murray_enable=0` (disabled by default this stage) and `murray_exponent=2.0` solver parms.
- **Commit as** `refactor(murray): schema — murray_acc + BG_MURRAY_SCALE (no-op)`.

### Stage 1 — propagate + finalize kernels
- Write `biogrowth_murray_propagate.cl` and `biogrowth_murray_finalize.cl`. Wire as two OCL SOP nodes after `topology`.
- Tracking copies under `hou/scripts/ocl/sop/`.
- Leave `murray_enable=0` by default; verify kernels compile and run inert.
- Flip `murray_enable=1` at test time, verify trunk visibly thickens proportional to descendant count. Sweep exponent 1.5 → 2 → 3 and confirm the visual difference (higher exponent → more emphasis on the thickest branches' contribution).
- Verify `self_collide` behaves correctly: thick trunks should push harder than thin laterals of the same depth.
- **Commit as** `feat(murray): per-frame tips→root radius propagation`.

### Stage 2 — defaults + cleanup + docs
- Flip `murray_enable` default to `1` on the solver.
- Flip `taper_decay` default to `1.0`.
- Update `doc/network.md`: add both kernels to the pipeline diagram, note attribute schema delta.
- Update `doc/self-collide.md`: note that `branch_radius` is no longer static — reads what Murray writes.
- Update `doc/todo.md`: move Murray's rule from "Next" to "Landed".
- Update `project_biogrowth.md` memory.
- **Commit as** `docs(murray): defaults + network + memory sync`.

## Guardrails / tuning

- **Exponent out of `[1, 4]`** is nonsensical. `p<1` gives mid-values that don't match any physical interpretation; `p>4` makes differences between siblings dominate the trunk radius and the tree looks lumpy. No hard clamp; rely on the default `2.0` being sensible.
- **Exponent=1** is just a straight sum, which means the root's radius equals the total count of tips × `base_radius`. Almost certainly not what anyone wants, but leaving the parm open-ended is fine.
- **Fixed-point precision.** `BG_MURRAY_SCALE = 1e4` gives ~0.0001 resolution on `r^p`. For `r ~ 0.05` and `p = 2`, that's `r^p ~ 0.0025`, which scaled is 25 — plenty of headroom. Watch for saturation only if users crank `base_radius` to >1.
- **One-frame lag.** `self_collide` on frame N uses `branch_radius` from end-of-frame N-1. At growth velocities where tips move less than one `branch_radius` per frame, this is imperceptible. If it ever matters, reorder `murray_*` to run before `self_collide` (requires `topology` to write provisional radii first or carry two generations — deferred).
- **Interaction with `taper_decay < 1`.** Keeping taper on at spawn means each lateral starts at a smaller radius → contributes less to its parent's Murray sum → parent's propagated radius is smaller than the "taper=1" case. Compositionally valid; just be aware that dropping taper amplifies Murray's trunk-thickening effect.

## Future extensions (out of scope)

- **Multi-pass inner loop** if one-frame lag ever becomes visually noticeable. Run propagation N times per frame where N = max current split_depth. Needs a pre-pass to find max depth. Cheap; deferred because single-pass-per-growth-step is already in sync with how fast the tree adds nodes.
- **Murray with flow asymmetry.** Biological Murray assumes one inlet / N outlets. If we ever drive radius by simulated fluid flow (a virtual "xylem pressure" field), the asymmetry matters. Way out of scope; noted for completeness.
- **Per-lineage exponent blending.** Young lineages might use `p=3` for vigorous tapering and old lineages `p=2` for quieter trunks. Would need a per-node blend; trivial to add to the propagate kernel. Defer until a scene asks for it.

---

## Surprise: stale parent_id bug (2026-04-20)

While bringing up Stage 1, Murray produced wildly unstable `branch_radius` values — trunk thickness flickered frame-to-frame, some interior nodes went to 0, others ballooned. Diagnostic: tag each node whose `parent_id` referenced a non-matching parent_stable_id after resolve → `bad_parent` fractions above zero at arbitrary frames.

**Root cause.** `parent_id` stored a ptnum. `blast_unactive` at end-of-frame deletes inactive attractors; that shifts ptnums for everything past the deleted points, including tree-node children whose ptnums interleave with attractor ptnums in the point list. Every frame, every surviving node's stored `parent_id` could silently point at a *different* point (usually another tree node, occasionally an attractor). Murray's propagate summed `r^p` into random parents → chaos.

This bug was latent in `self_collide` too — its bidirectional ancestor walk uses `parent_id`. It was invisible there because (a) `self_collide_strength = 0.025` is 40× weaker than `attract_strength`, and (b) most ancestor-walk hits are against real ancestors even by accident (close ptnums tend to be local-in-tree). Murray exposed the bug because `r^p` summation is globally unforgiving.

**Original fix (2026-04-20).** Added a stable-id indirection layer: monotonic never-reused `i@stable_id`, `i@parent_stable_id` stored at spawn, `parent_id` rebuilt each frame from a scatter/gather detail array. Four nodes at solver entry (`assign_sids` / `resize_sid_lookup` / `resolve_sid_scatter` / `resolve_parent_id`). Worked correctly but became the dominant cost as scenes grew.

**Replaced 2026-04-25 by the split-streams refactor.** Tree_nodes and attractors run on separate streams inside the solver; topology only appends children to the tree-only stream; tree-first re-merge preserves tree_node ptnums forever. `parent_id = @ptnum` set at spawn is now permanently valid — no resolve chain needed. Stable-id attributes deleted. See `doc/performance.md` and `doc/network.md` for the new pipeline.

**Verification before refactor.** AttribPromote Max over `bad_parent` → 0 across all frames. Trunk thickness stable and proportional to descendant count, as expected from Murray. After the refactor: same correctness, no resolve cost.

## Future cleanup

- `closest_node` on attractors is also a stored ptnum (debug/viz only). If any future consumer reads it across frames, same bug — migrate it to stable-id then. Currently write-only intra-frame, so safe.
