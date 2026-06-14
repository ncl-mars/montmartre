# Allometric length limit (McMahon / WBE) — Plan

Branch: `feature/allometric-length`

## Rule

McMahon's scaling: a branch's maximum length scales with its diameter as `L_max ∝ r^(2/3)`. Thin branches can't grow indefinitely — they self-limit. This closes the Murray-radius → growth-extent feedback loop: today `branch_radius` only affects `self_collide` and sweep thickness, with no effect on how far a branch can grow. With this rule, thick branches grow long, thin ones stop early, and shape emerges from the r→L coupling.

## Schema delta

**Tree nodes — add one attribute:**

- `f@arc_total_since_lateral` (float, init 0). Accumulated arc length since this branch's origin (its most recent lateral spawn, or the seed for the trunk lineage). Per-node, carried forward through tip extensions, reset on lateral spawn.

**Propagation on spawn** (in `topology`):

- **Tip-extension child:** inherits parent's value (`arc_total_since_lateral = parent.arc_total_since_lateral`). Same branch continues along the same lineage axis — the length-since-origin clock does not reset at every node, only at bifurcations.
- **Lateral child:** starts at 0. This is a *new* branch; its origin is here.
- **Parent:** unchanged — becomes interior, value frozen, never read again (consumers filter on `is_tip==1`).

No changes to `set_attractor` except a defensive `f@arc_total_since_lateral = 0;` for compile-block safety.

## Per-kernel changes

### `integrate` — accumulate

Single-line addition in the tip writeback block, right after `@arc_since_spawn.set(...)`:

```cl
@arc_total_since_lateral.set(@arc_total_since_lateral + length(dP));
```

Binding: `#bind point &arc_total_since_lateral float`. Cost: one add per tip per frame. Already tip-only (integrate early-returns on non-tips).

### `allometric_cull` — new dedicated tree-node kernel

The existing `cull` kernel iterates attractors (stall + kill_radius proximity), so it's the wrong work-item domain for a per-tip check. Add a standalone OCL SOP iterating tree nodes:

```cl
#include "$HIP/scripts/ocl/include/biogrowth.cl"

#bind point  &P                       float3
#bind point   is_tree_node            int   read
#bind point   is_tip                  int   read
#bind point   branch_radius           float read
#bind point   arc_total_since_lateral float read
#bind point  &active                  int

#bind parm   allometric_k        float val=4.6
#bind parm   allometric_exponent float val=0.6667

@KERNEL
{
    if (@is_tree_node < 0.5)     return;
    if (@is_tip       < 0.5)     return;

    float L_max = @allometric_k * pow(@branch_radius, @allometric_exponent);
    if (@arc_total_since_lateral > L_max) {
        @active.set(0);
    }
}
```

`&P float3` bound writable only to drive work-item count (per SOP OCL rule); P is never mutated. Cost: one pow + compare per tip per frame. Negligible.

### `topology` — inherit / reset

Tip-extension path — add to the child-init block:

```vex
setpointattrib(0, "arc_total_since_lateral", child, f@arc_total_since_lateral);
```

Lateral-spawn path — add:

```vex
setpointattrib(0, "arc_total_since_lateral", child, 0.0);
```

### `set_seed` — init

```vex
f@arc_total_since_lateral = 0;
```

### `set_attractor` — defensive

```vex
f@arc_total_since_lateral = 0;
```

## Solver parms

- `allometric_enable` (int toggle, default 1) — drives a **Switch SOP** bypassing the `allometric_cull` node when 0. No in-kernel guard; the Switch is the single authority. Useful for A/B comparisons with Murray-only trees.
- `allometric_k` (float, default tune-on-scene). Has units of `length^(1 - exponent)` = `length^(1/3)` when exponent=2/3. Scene-dependent. Start around `k = step_size × 100 / base_radius^(2/3)` as a rough guess, then tune. At `step_size=0.01, base_radius=0.1`, that's `k ≈ 1 / 0.215 ≈ 4.6`. Expect to iterate.
- `allometric_exponent` (float, default 0.6667). McMahon's canonical value for self-similar elastic-stability scaling. Lower values (closer to 0.5) make thin branches die off even faster; higher values (toward 1) approach linear scaling.

## Interaction with Murray

`branch_radius` on a tip is its **seed** value (set at spawn: `base_radius` for trunk, `base_radius × taper_decay` for laterals). Murray only propagates radius on interior nodes — tips keep their seed radius. So the allometric check reads a *constant-per-branch* radius, which is the right signal: "a lateral born thin stays thin, and is allowed to grow only as long as that thinness permits."

Later subtree thickening (Murray pushes a node's radius up as its descendants grow) affects only interior nodes — it doesn't grant retroactive growth budget to the tip that already exhausted its allometric limit and got culled. This is biologically correct: the tip stopped when it stopped; subsequent radius inflation is structural, not developmental.

## Pipeline

One new node (`allometric_cull`) inserted between `integrate` and `topology`:

```
… integrate (accumulate) → allometric_cull (consume) → vigor_accumulate → topology (inherit/reset) → cull → …
```

Order matters: `integrate` must accumulate before `allometric_cull` reads, and `topology` must propagate on spawn before next frame's `integrate` accumulates. Placing `allometric_cull` right after `integrate` also means a tip that crosses its limit is deactivated before `topology` could spawn a child from it this frame.

## Staged sequence

### Stage 0 — schema migration

- Add `f@arc_total_since_lateral = 0;` to `set_seed` and `set_attractor`.
- Add inherit/reset to both topology spawn paths.
- Bind `&arc_total_since_lateral float` in `integrate`, add the accumulate line.
- Do **not** consume yet — `cull` unchanged. `allometric_enable=0`.

**Exit criteria.** Attribute exists on every tree node, grows monotonically on tips, is inherited on tip-extension, resets on lateral spawn, never read by anything. Sim output identical to `develop`. Commit: `refactor: allometric schema (no-op)`.

### Stage 1 — cull consumer

- Add the cull check gated on `allometric_enable`.
- Promote `allometric_enable`, `allometric_k`, `allometric_exponent` to solver spare parms.
- Set `allometric_enable=1`, pick `allometric_k` by inspection.

**Exit criteria.** Thin branches visibly terminate at length ∝ `r^(2/3)`; trunks persist. Frontier tapers naturally instead of producing indefinitely long thin threads. Tune `allometric_k` until sparse lineages look right.

**Commit:** `feat: allometric length limit (McMahon L ∝ r^(2/3))`.

### Stage 2 — polish & docs

- Update [network.md](../network.md) schema list and cull node description.
- Update [todo.md](../todo.md) — move to Landed.
- Update [biogrowth-plan.md](../biogrowth-plan.md) if the schema delta is relevant there.
- Cross-check [biological-rules.md](../biological-rules.md) rule 2 — add "Landed YYYY-MM-DD" banner.
- Update `project_biogrowth.md` memory.

**Commit:** `refactor: allometric docs + defaults`.

## Tuning guardrails

- **`allometric_k` is scene-scaled.** If you change `step_size` or `base_radius`, `k` needs retuning. Document on the solver parm tooltip.
- **Don't set exponent > 1.** That would make thick branches grow proportionally longer than thin ones without bound, defeating the purpose (classical McMahon gives < 1 for self-limiting).
- **Disable+enable A/B.** With `allometric_enable=0`, the sim falls back to Murray-only (current `develop` behavior) — useful for before/after comparisons during tuning.

## Critical files

Live code (AppData\Temp\*, current session prefixes — will change on reboot):
- `56_set_seed_snippet.vfl` — add init
- `SetAttractor` snippet — defensive init
- `41_topology_snippet.vfl` — inherit (tip path) + reset (lateral path)
- `52_integrate_kernelcode.cl` — binding + accumulate line
- `1048_allometric_cull_kernelcode.cl` — new dedicated kernel

Real files:
- `hou/scripts/vex/sop/biogrowth_set_seed.vfl`, `biogrowth_set_attractor.vfl`, `biogrowth_topology.vfl`
- `hou/scripts/ocl/sop/biogrowth_integrate.cl`, `biogrowth_allometric_cull.cl` (new tracking copy)
- `doc/network.md`, `doc/biological-rules.md`, `doc/todo.md`, `memory/project_biogrowth.md`

## Verification

1. **Stage 0 diff.** Cook frame 175 before/after; attributes and positions must be byte-identical.
2. **Stage 1 visual.** At `allometric_k` tuned from inspection, thin laterals visibly stop extending while trunks continue. Compare tree silhouette against `develop` — trunk shape unchanged, interior frontier less hairy.
3. **Parm sweep.** `allometric_k × {0.5, 1, 2}` should show proportional length change on thin branches without affecting trunk.
4. **Exponent sweep.** `allometric_exponent × {0.5, 0.667, 0.8}` — lower values = more aggressive thin-branch death; higher = more uniform length.
5. **Enable toggle A/B.** `allometric_enable=0` reproduces develop exactly; `=1` shows the expected taming.
