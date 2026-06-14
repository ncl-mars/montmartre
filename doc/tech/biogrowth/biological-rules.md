# Biologically-Grounded Rules — Candidates

Follow-up ideas in the spirit of Murray's rule: simple, well-established biological/physical rules that could slot into the existing BioGrowth architecture. Ranked by fit with the current infrastructure (GPU-parallel OCL kernels, per-frame propagation, atomic-add accumulators, VDB constraint buses).

Session brainstorm: 2026-04-20, post-Murray-landed.

---

## 1. Phyllotaxis (golden-angle lateral spawning)

Real trees often rotate successive laterals around the parent's growth axis by ~137.5° (the golden angle) — produces the spiral arrangements visible on pine cones, sunflower seeds, and branching architecture.

**Implementation sketch.** Per-node counter (`i@lateral_spawn_count`) incrementing on each lateral spawn in `topology`. Lateral child's initial direction rotates around parent's `velocity` axis by `n × 137.5°`, combined with the existing `last_att_force` bias (e.g., blend by a user parm).

**Cost.** Cheap — per-spawn O(1). No new kernel; fits inside topology's lateral-branching block.

**Expected visual.** Instantly recognizable spiral arrangements; trees "feel more alive" without any other change.

**Fit with existing.** Couples cleanly with the existing lateral-spawn machinery. Would interact with `last_att_force` — design decision: does attract pull win, or does phyllotaxis enforce regardless? Probably a blend parm.

---

## 2. Allometric length limit (McMahon / WBE scaling)

Branch max-length scales with diameter: typically `L ∝ r^(2/3)` (McMahon's scaling). Thin branches can't grow indefinitely — they self-limit.

**Implementation sketch.** Add `f@arc_total_since_lateral` (accumulates per-frame). Cull or deactivate tip when exceeding `k × branch_radius^(2/3)`. `k` as a solver parm.

**Cost.** Cheap — one comparison per tip per frame in cull.

**Expected visual.** Natural frontier-taming; sparse lineages taper off instead of growing forever as thin threads.

**Fit with existing.** Couples directly with Murray (its output drives the limit). Closes a genuinely missing feedback loop: currently branch_radius influences self_collide and sweep thickness but has no effect on how far a branch can grow.

**Why this is the biggest architectural win.** Without it, Murray radii are a decorative output. With it, Murray becomes causally in the sim — thick branches grow long, thin ones stop early, shape emerges from the r→L feedback.

---

## 3. Leonardo's branching-angle rule

Force balance at a bifurcation: `cos(θ_trunk)·r_trunk² = Σ cos(θ_child)·r_child²`. Drives biologically-grounded branching angles coupled to Murray radii.

**Implementation sketch.** At lateral spawn time, compute the lateral's initial direction to satisfy the force-balance equation given parent's direction and the child's Murray-seed radius. Replaces the current `normalize(last_att_force) × 1e-3` nudge with a geometrically-motivated direction.

**Cost.** Moderate — trig + vector math at spawn time. Still O(1) per spawn.

**Expected visual.** More correct branching angles, particularly on thick trunks (laterals emerge at shallower angles because force balance demands it). Less arbitrary-looking junctions.

**Fit with existing.** Replaces `last_att_force` orientation, which is heuristic. Best combined with phyllotaxis (one drives the rotation axis; this drives the pitch angle).

---

## 4. Reaction / compression wood (gravity-biased Murray)

Horizontal branches thicken asymmetrically on the underside to resist bending — "compression wood" in gymnosperms, "tension wood" in angiosperms. Visually, horizontal branches are thicker than vertical ones at the same descendant count.

**Implementation sketch.** In `murray_propagate`, multiply the `r^p` contribution by `1 + k × max(0, -dot(branch_axis, up))` where `branch_axis` is the unit vector from parent to child. `k` and `up` as solver parms.

**Cost.** One dot product + scalar multiply inside the existing propagate kernel. Negligible.

**Expected visual.** Subtle but realistic trunk-to-horizontal-branch widening. Vertical trunks unchanged; horizontal laterals thicken.

**Fit with existing.** Small additive tweak on top of Murray — no new kernels, no new attributes, no pipeline change. Easiest win once Murray is confident.

---

## 5. Branch autonomy (self-pruning by light debt)

Each branch accumulates light intake over time (photo VDB sample × dt); if the long-term integrated intake falls below a survival threshold, the branch is marked inactive. Natural shade-mortality without hand-authoring.

**Implementation sketch.** Add `f@light_credit` per tree node, accumulate in a new OCL kernel between `integrate` and `cull`: `light_credit += photo_sample(P) × dt`. Slow decay term: `light_credit *= (1 - decay × dt)`. When `light_credit < mortality_threshold`, set `active = 0`. Blast cleans up.

**Cost.** One VDB sample + scalar update per tree node per frame. Comparable to any other VDB contributor.

**Expected visual.** Shaded interior branches die and are removed over time. Emergent crown structure: canopy survives, understorey self-prunes.

**Fit with existing.** Slots directly onto the photo VDB bus (solver Input 2). Naturally extends the already-planned photo contributor. Biologically the most satisfying death mechanism — replaces cull's "no attractors in range" kill with something more meaningful.

---

## 6. Apical dominance via auxin-like suppression field

The current vigor system approximates apical dominance with a depth penalty (depth-N interiors sprout every `2^N` frames). Real apical dominance is mediated by auxin: tips produce it, it flows basipetally (toward the root), and suppresses lateral-bud outgrowth along the way. Distance-from-tip matters, not just depth.

**Implementation sketch.** Each tip emits a "dominance" signal that decays with distance down its own lineage. Interior nodes accumulate received dominance; high accumulation raises the lateral-sprout threshold. Two structural options:

- **Kernel-only (preferred).** Tip writes `dominance_strength` into its parent's `f@dominance_acc` each frame. A second pass propagates `dominance_acc` one hop further with decay (`parent.dominance_acc += self.dominance_acc × decay`). After N frames, dominance reaches N hops deep from each tip — matches Murray's "one hop per frame, transitive over time" pattern. Topology's lateral threshold becomes `lateral_base_threshold × (1 + d × vigor_penalty) × (1 + k × dominance_acc)`.
- **Explicit graph walk.** Per-frame tips→root traversal; more exact but expensive (multi-pass inner loop). Defer unless the per-frame propagation looks wrong.

**Cost.** One extra kernel on the same tips→root pattern as Murray. Cheap.

**Expected visual.** Long straight trunks under strong terminal shoots (dominance suppresses laterals on young wood); vigorous lateral branching where a tip is distant or damaged (reduced dominance reaching there). Reiteration comes for free: if a leader is lost, the suppression it was applying vanishes, and dormant buds below sprout.

**Fit with existing.** Layers cleanly on top of vigor — vigor throttles by depth (approximation), dominance throttles by tip-distance (real signal). Both multiply into the threshold. Would likely reduce the need for `vigor_penalty > 0`.

**Connection to `acc_*`.** This rule writes from one point (tip) into another (its parent) — cross-point contributor. First real use case justifying the atomic-int pattern beyond `att_*`. Use its own `f@dominance_acc` int-fixed-point accumulator following the `BG_MURRAY_SCALE` / `att_*` template — revival recipe in [acc-int-atomic-revival.md](archive/acc-int-atomic-revival.md).

---

## 7. Damage accumulator (orthogonal event bus)

`f@damage` per node, bumped by events: collision impulse exceeding threshold, user-painted group, scripted break ("leader snaps at frame 80"), or integrated from future contact-stress. Not a growth rule on its own — it's an input bus that several existing rules can consume.

**Implementation sketch.** Per-node float `f@damage ∈ [0, 1]`, clamped. Event sources (each its own small kernel or wrangle):
- Scripted/painted: a wrangle with a group picks nodes and writes `@damage = 1.0`.
- Collision impulse: if the collision stage in `integrate` resolves a penetration > threshold, `@damage += k × penetration` on the affected tip.
- Slow healing (optional): `@damage *= (1 - heal_rate × dt)` each frame — minor damage fades, catastrophic damage persists.

Consumers (each is a one-line tweak in an existing rule):
- **Dominance emission** (rule 6): tip emits `dominance_strength × (1 - damage)` — damaged tips suppress less, dormant buds below wake up → reiteration.
- **Murray radius** (landed): scale `r_child` contribution by `(1 - damage)` before the `r^p` sum → dead/dying subtrees stop inflating their ancestors.
- **Cull** (landed): add `damage > damage_kill_threshold → active = 0`.
- **Branch autonomy** (rule 5): `light_credit -= damage × dt` — damaged branches die faster under marginal light.

**Cost.** Event writes are sparse (per-frame for collision-driven, once-for-authored). Consumer reads are one `@damage` fetch per already-running kernel. Negligible.

**Expected visual.** Authored breakage: snap a trunk at frame N, watch laterals below reiterate over 20–40 frames as dominance drains and Murray reapportions radius. Emergent breakage: a branch that drives repeatedly into a collision SDF accumulates damage and eventually sheds — no hand-authoring.

**Fit with existing.** Orthogonal. Doesn't depend on any single rule; slots value into whichever consumers are present. Ship alone (with just the scripted-paint source + cull consumer) for a minimum viable "break this branch" feature; compose with dominance for reiteration; compose with Murray + autonomy for full dieback dynamics.

**Ordering.** Best shipped **after** dominance (so reiteration has a consumer to wire into) and **after** any one event source you actually want (scripted paint is cheapest). Adds nothing visible without at least one consumer rule live.

---

## Ranking

**Biggest visual return for least effort:** phyllotaxis. Low cost, immediately recognizable.

**Biggest biological / architectural return:** allometric length limit. Closes the Murray-radius → growth-extent feedback loop, which is the single missing coupling between "Murray writes radii" and "radii affect the sim." Without it, Murray is cosmetic; with it, shape emerges from the rule.

**Combined recommendation for a "pass 2 on biological realism":**
1. Phyllotaxis — visual polish.
2. Allometric length limit — structural shape-maker.
3. Apical dominance (auxin-like) — upgrades vigor from depth heuristic to distance-mediated signal; enables reiteration.
4. Leonardo's angle rule — geometric polish at junctions.
5. Reaction wood — asymmetric thickening.
6. Branch autonomy — long-horizon mortality, ties into photo.
7. Damage accumulator — orthogonal event bus; ship after dominance to unlock reiteration from authored or collision-induced breakage.

Each can ship as an independent feature branch following the Murray template (stage 0 schema → stage 1 kernel → stage 2 defaults + docs).

## Out of scope (noted for completeness)

- **Full LIGNUM-style carbon balance** — per-segment photosynthesis/respiration with explicit carbon flow. Way out of scope; would require replacing Murray with a full allocation model.
- **Reiteration as a named rule** — now covered implicitly by dominance (rule 6) + damage (rule 7) composition; no standalone rule needed.
- **Thigmotropism** — contact-stress feedback from collisions. Would require the collision bus to expose contact accumulation.
- **Cosserat-rod mechanical era** — already in the long-term plan as post-growth deformation; not a growth-time rule.
