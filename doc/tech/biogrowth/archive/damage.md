# Damage (Orthogonal Event Bus)

Per-node `f@damage ∈ [0, 1]` written by **event sources** and consumed by **existing rules** via one-line modulations. Not a growth rule on its own — an input bus several rules can layer on. Authored breakage (snap a leader at frame 80), collision-impulse damage, and shaded subtrees can all funnel through the same attribute.

## Attribute

- `f@damage` — float in `[0, 1]`, clamped. `0` = healthy, `1` = fully damaged. Seeded `0` on seed and on every topology spawn path.

## One-off scripted breakage

For "snap *this specific* tree_node at *this specific* frame", don't add a dedicated solver node — paste a small VEX wrangle into the solver on the tree-only stream (between split and consumers). Tree_node ptnums are stable post-split-streams so direct ptnum references work as authoring keys:

```vex
// Snap node 42 at frame 80
if (@Frame == 80 && @ptnum == 42) {
    @damage = clamp(@damage + 1.0, 0.0, 1.0);
}

// Multiple events in one wrangle
if (@Frame == 80 && @ptnum == 42) @damage = clamp(@damage + 1.0, 0, 1);
if (@Frame == 95 && @ptnum == 17) @damage = clamp(@damage + 0.5, 0, 1);
```

Authoring concern, not an architectural one. The damage bus exposes the attribute; how individual events get authored is per-scene art direction. For systematic / field-driven damage (pyro, designer-painted zones), use the `damage_field` kernel below.

## Event sources (writers)

Sources accumulate into `@damage`; clamp at the end of each writer. Multiple sources compose additively (clamped).

**1. Scripted paint (Stage 1 — MVP).** A new attribute wrangle inside the solver, on the **tree-only stream** (post-split, before any consumer). Runs each frame; an authored predicate picks tree_nodes and writes `@damage`. Tree_node ptnums are stable post-split-streams, so direct ptnum references work as authoring keys.

```vex
// Break a specific node at a specific frame
if (@Frame == chf("break_frame") && @ptnum == chi("break_ptnum")) {
    @damage = 1.0;
}

// Region paint
if (@P.y > chf("damage_height_threshold") && @Frame >= chi("damage_start_frame")) {
    @damage = max(@damage, chf("damage_paint_strength"));
}

// Group-driven (group authored interactively or upstream)
if (inpointgroup(0, chs("damage_group"), @ptnum)) {
    @damage = chf("damage_paint_strength");
}
```

Spare parms on the wrangle expose the controls (`break_frame`, `break_ptnum`, `damage_paint_strength`, etc.). One wrangle, multiple co-existing predicates if needed. Toggleable via Switch-SOP wrapper if you want to disable scripted paint entirely.

**2. Collision impulse (Stage 4 — later).** Inside `integrate`'s collision stage 4, when a tip is snapped out of a surface, the snap distance `push` is a proxy for impulse. If `push > damage_impulse_threshold`, `@damage += damage_impulse_gain × push`. Cheap; no extra kernel.

**3. Slow healing (Stage 5 — optional).** Per-frame `@damage *= (1 - heal_rate × dt)`. Minor damage fades over time; catastrophic damage persists (because the consumers may freeze growth, preventing recovery feedback). Can be a wrangle or folded into integrate.

## Consumers (readers)

Each consumer is a one-line modification inside an existing rule. They compose freely — applying multiple consumers gives realistic dieback.

**1. Dominance emission** (apical-dominance rule 6). In `dominance_step`, multiply tip emission by `(1 - @damage)`:
```cl
float v = (@is_tip > 0.5) ? @dominance_strength * (1.0f - @damage)
                          : @dominance * @dominance_decay;
```
**Effect:** damaged tips emit less suppression → dormant buds below wake up → emergent reiteration.

**2. Murray radius contribution** (murray rule). In `murray_propagate`, scale the `r^p` push-to-parent by `(1 - @damage)`:
```cl
float v = pow(@branch_radius, @murray_exponent) * (1.0f - @damage) * mult;
```
**Effect:** dying subtrees stop inflating their ancestors' radii — trunks thin out where descendants are dead.

**3. Branch autonomy intake** (branch-autonomy rule). In `integrate`'s light-credit accumulation, subtract damage drain from intake:
```cl
c += (intake - @damage * @damage_intake_drain) * @TimeInc;
```
**Effect:** damaged branches die faster under marginal light — accelerates self-pruning where damage and shade compound.

**4. Growth-factor scaling** (companion to allometric / branch autonomy in `integrate`'s tip branch). Stack `(1 - @damage)` into `gf` so heavily damaged tips stop extending:
```cl
v *= gf * (1.0f - @damage);
```
**Effect:** damaged tip stops growing, even if it has light credit and length runway. Direct freeze, no flicker.

**No tree_node deletion** — the split-streams invariant forbids it. Damage produces freeze, not vanish; render-side fade is the visual-pruning path if desired.

## Toggle

`damage_enable` solver spare parm (int, default 0). Each consumer reads it via `* (damage_enable > 0 ? (1.0 - @damage) : 1.0)` collapse-to-1 pattern (same idiom used by vigor / dominance multipliers in the topology threshold). Off → all consumers no-op even if damage values are set.

Healing toggled by `damage_heal_enable` separately.

## Parms (solver spare)

- `damage_enable` — int, default 0. Master toggle; all consumers collapse to identity when off.
- `damage_heal_enable` — int, default 0.
- `damage_heal_rate` — float, default 0.01. Per-second decay rate.
- `damage_impulse_threshold` — float, default 0.05. Min snap distance for collision damage (Stage 4).
- `damage_impulse_gain` — float, default 1.0. Multiplier on collision impulse.
- `damage_intake_drain` — float, default 1.0. Strength of damage's hit on light intake (Stage 3 consumer).
- `damage_paint_strength` — float, default 1.0. Used by scripted paint wrangles.

## Stages

**Stage 0 — schema.** `f@damage = 0` in setup wrangle + every topology spawn (both tip-extension and lateral-spawn addpoint blocks). Add `damage_enable` parm. No behavior change yet; the attribute exists, nothing reads it.

**Stage 1 — scripted paint event source (MVP).** A new outside-solver wrangle (or solver spare wrangle) groups nodes by an authored predicate and writes `@damage = chf("damage_paint_strength")`. Test with a manually painted group; verify `@damage` shows up in the spreadsheet. No consumers yet — purely visual via Cd debug.

**Stage 2 — dominance consumer (first visible behavior).** Modify `dominance_step` to scale tip emission by `(1 - damage * (damage_enable > 0))`. Land. Test: paint damage on a strong leader → its dominance drops → laterals below sprout (reiteration). This is the marquee biological demo.

**Stage 3 — Murray + branch-autonomy consumers.** Add the `(1 - damage)` factor to `murray_propagate`'s contribution and the intake drain to `integrate`'s light_credit accumulator. Land. Test: damaged subtrees thin out (Murray) and die faster under marginal light (autonomy).

**Stage 4 — collision impulse event source.** In `integrate`'s collision stage, after the snap, accumulate `@damage += damage_impulse_gain × push * (push > damage_impulse_threshold)`. Compose with Stage 2 + 3: a tip that drives repeatedly into a wall accumulates damage, eventually loses dominance, gets thinned by Murray, and freezes under light debt. Emergent dieback from contact stress.

**Stage 5 — optional healing.** Per-frame `@damage *= (1 - heal_rate × dt)` either as a tiny wrangle or folded into integrate. Behind `damage_heal_enable`.

**Stage 6 — docs + memory.** `doc/network.md` schema row, `doc/todo.md` landed entry, project memory section, MEMORY.md index.

## Composition order

Land Stages 1+2 first to validate the bus end-to-end with one consumer. Stages 3 and 4 are independent — order by what scene you want to demo first. Stage 5 is optional and easy to add later.

**Why not start with the growth-factor consumer (#4)?** It's simpler in code (just a multiplier in `integrate`) but boring visually — damaged tips just stop. The dominance consumer (#1) gives reiteration, the most distinctive biological signal of damage propagation, on day one. Lead with the impressive demo.

## Interaction notes

- **Apical dominance + damage**: the marquee composition. Lose a leader → its dominance drains → buds below sprout. Without damage, you'd have to set the leader's `is_tip = 0` manually; with damage, scripted breakage just works.
- **Branch autonomy + damage**: damage accelerates shade death; healthy shaded branches limp along, damaged shaded branches die fast. Compound failure.
- **Murray + damage**: trunks thin where the canopy died. Visible structural retreat.
- **Future consumers**: any future rule reading per-node weights should consider `(1 - damage)` as a candidate modulator. The bus is open-ended.

## Anti-patterns

- **Don't make damage cause `active = 0` on tree_nodes.** Split-streams invariant. Use freeze (`v *= (1 - damage)`).
- **Don't normalize damage globally** (e.g. divide by max). Each event source is local; global normalization couples unrelated subtrees.
- **Don't bake damage into other attributes** (e.g. permanently zero `branch_radius`). Damage is a *modulator*; consumers respect it via `(1 - damage)`. Restoring health (via heal) should restore behavior — only possible if the underlying state isn't destroyed.
