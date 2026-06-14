# Branch Autonomy (Light-Debt Growth Scaling)

Shade-stressed tips lose growth vigor smoothly — their velocity is scaled by a credit-to-threshold ratio, mirroring allometric's growth-factor pattern. Each tree_node accumulates irradiance intake with slow decay into `f@light_credit`; tips below the threshold have their growth multiplied toward zero. No deletion, no polyline damage — shaded branches quietly stall and get overtaken by their better-lit siblings.

## Rule

Per-frame, all tree_nodes:
```
light_credit += irradiance_sample(P) × intake_gain × dt
light_credit *= (1 - decay × dt)
```

Per-frame, tips only:
```
ratio = clamp(light_credit / mortality, 0, 1)
gf    = pow(ratio, curve)           # 0 at zero credit, 1 at/above mortality
v    *= gf                          # scales Euler-integrated velocity
```

Stacks multiplicatively with allometric's own `gf`. A tip that's both shade-stressed *and* hitting its McMahon length limit dims on both axes.

## Attribute

- `f@light_credit` — per-node float. Seeded to `light_credit_grace` so newborns have a buffer before shade dims their growth. Updated for all tree_nodes every frame inside `integrate` (before the tip/interior split).

Seed in setup wrangle + every topology spawn path:
```vex
f@light_credit = chf("light_credit_grace");
// topology spawn: setpointattrib(0, "light_credit", child, chf("light_credit_grace"));
```

## Implementation

Fused into `integrate` (no standalone kernel). Two blocks:

1. **Pre-split credit update** — applies to all tree_nodes, gated on `@irradiance.bound`. Accumulate + decay into `@light_credit`. Interior nodes also update (data is meaningful for future consumers) but don't consume it yet.
2. **Tip-branch growth scaling** — after the Euler step, scale `v *= gf` where `gf` is derived from the credit ratio. Sits alongside allometric's scaling; the two multiply.

Why fused not standalone:
- A separate pre-integrate kernel that zeroes `velocity` + `force_acc` still leaks tropism force (tropism is inline in integrate, not an attribute).
- Matches the pattern already used for allometric, avoid, collision, vigor accumulation — integrate is the natural home for tip-growth-stoppage logic.

## VDB

Scalar `irradiance` VDB on the constraints bus (Input 1 via `import_constraints`). Distinct from the float3 `photo` VDB consumed by the phototropism contributor:
- `photo` (float3): direction of incoming light — used by phototropism for tip steering.
- `irradiance` (float): scalar light magnitude — used by branch autonomy for intake.

Typically baked from the same raytracing pass (one channel per property). Kept as separate fields because diffuse light has high irradiance with no clear direction — a single float3 can't represent that.

If `irradiance` is unwired, the whole branch-autonomy path is a no-op (credit update skipped, `gf` never computed). Nodes hold at `light_credit_grace` forever.

## Toggle

In-kernel `branch_autonomy_enable` parm inside integrate. When off, credit still accumulates (data stays live for debugging / visualization) but the growth-factor multiplier is skipped. Set `irradiance` unwired for full zero-cost bypass.

## Parms (solver spare)

- `branch_autonomy_enable` — int, default 0. Ship off until tuned.
- `light_credit_grace` — float, default 10.0. Initial credit on seed + each new child. Sets newborn immunity duration.
- `light_credit_decay` — float, default 0.05. Per-second credit decay rate.
- `light_credit_mortality` — float, default 1.0. Credit level at which growth factor reaches 1 (above = full growth, below = dims toward 0).
- `light_credit_curve` — float, default 2.0. Sharpness of the credit → growth-factor curve (higher = longer grace before shade bites, steeper drop).
- `light_credit_intake_gain` — float, default 1.0. Multiplier on raw irradiance sample; tune per-scene to match field magnitude.

## Solver order

Unchanged — integrate sits in its usual position. Branch autonomy is purely internal to integrate now.

## Interaction notes

- **Dominance** (landed): dominated interior nodes spawn fewer laterals → fewer leaves → less irradiance → dimmer growth → self-reinforcing pruning of suppressed subtrees.
- **Allometric** (landed): both scale `v`; multiplicative. A tip short on both light and length runway dims twice.
- **Damage accumulator** (future): `light_credit -= damage × dt` would accelerate shade mortality on damaged branches.

## Not yet

- **Visual self-pruning.** Current design freezes but doesn't remove. If you want shaded branches to visibly disappear, add a downstream cosmetic cull keyed on `light_credit` (shader-side visibility fade, or a render-time blast by threshold). Don't route it back into the sim — it would flicker polylines.
- **Interior consumer.** `light_credit` is tracked on interiors but not consumed yet. Future rules (branch shedding, dieback) can read it.
