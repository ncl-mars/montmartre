# Physics — Integrator Choice

Reference doc for the semi-implicit Euler integrator in the BioGrowth growth-era solver. Spun out of the Stage 2 refactor; originally section of `biogrowth-plan.md`. Read with `network.md` (pipeline) and `biogrowth-plan.md` (design context).

**Scope:** the **growth era** only — tip advection under atomic-add force contributors. Mechanical-era deformation on mature polylines is a separate problem (future Cosserat/PBD pass on the grown tree; not in this solver).

## Problem statement

Advance each **tip** tree-node's `(P, v)` under accumulated acceleration `a` (summed from `attract`, `tropism`, and future VDB contributors — all atomic-added into per-node accumulators before this kernel runs). Axes that matter:

- **Stability** — no blow-up at `dt ≈ 0.04s` (24fps).
- **Cost per step** — force evaluations = attract passes = our pipeline's dominant cost. One force eval per frame is the budget.
- **Energy behaviour** — symplectic integrators stay bounded; non-symplectic drift over 200+ frame sims.
- **Damping + velocity clamping** — must compose cleanly. We want `(1 - damping·dt)` velocity bleed and `|v| ≤ max_speed` without scheme-specific acrobatics.

## Candidates considered

### 1. Forward (Explicit) Euler
```
v' = v + a·dt
P' = P + v·dt       // uses OLD velocity
```
- Cost: 1 force eval.
- Stability: poor. Unstable for any oscillatory force. Energy grows frame-over-frame. *"Explicit Euler gains energy over time"* — Gaffer.
- **Rejected.**

### 2. Semi-implicit (Symplectic) Euler  ← **CHOSEN**
```
v' = v + a·dt
P' = P + v'·dt      // uses NEW velocity
```
- Cost: 1 force eval.
- Stability: dramatically better than explicit Euler despite identical O(1) local accuracy. Symplectic — energy stays bounded on average, no secular drift.
- Damping + clamp: trivially composable around the velocity update.
- Gaffer's recommendation; Box2D, Unity rigidbody, Bullet all use variants. Handles stiff springs *better* than Verlet in practice.

### 3. Velocity Verlet
```
P' = P + v·dt + 0.5·a·dt²
compute a' at P'
v' = v + 0.5·(a + a')·dt
```
- Cost: **2 force evals per step** (old P and new P). In our pipeline, a "force eval" means running `attract` + future VDB chain again against the freshly-moved tips — 2× the dominant kernel cost. Non-starter.
- Stability: symplectic, O(2) local. Better long-horizon energy conservation than semi-implicit Euler.
- Goes unstable with stiff forces unless velocity is clamped (which we already do).
- **Rejected on cost.**

### 4. RK4
4 weighted force evaluations per step.
- Cost: **4 force evals per step.**
- High O(4) local accuracy, but **not symplectic** — energy drifts on long oscillators (*"maintains frequency but loses energy"* — Gaffer).
- **Rejected on cost + energy.**

### 5. Leapfrog
Algebraically equivalent to semi-implicit Euler in our configuration (no separately-stored half-step velocity).
- **No improvement over chosen.**

## Decision

Semi-implicit Euler, because:

1. **One force eval per frame** matches the pipeline cost model. Attract (the dominant kernel) runs once; future VDB contributors run once each. Verlet would double all of that.
2. **Symplectic + bounded energy** means no long-horizon tuning for drift. Damping bleeds any residual drift further.
3. **Composes cleanly with damping and `max_speed`** — both go around the velocity update, nothing tricky.
4. **Battle-tested in game physics** at comparable `dt`. Our regime (bounded forces, cosmetic growth, ~200-frame sims) is squarely where this scheme shines.

## Implementation

Runs in [integrate_kernelcode.cl](../hou/scripts/ocl/sop/biogrowth_integrate.cl) (tracking copy); live snippet is whatever prefix Houdini's Temp dir assigns.

```cl
float dt   = @TimeInc;
float damp = max(0.0f, 1.0f - @damping * dt);
float3 v   = (@velocity + a * dt) * damp;
v          = limit(v, @max_speed);           // utils.cl
float3 dP  = v * dt;
@P.set(@P + dP);
@velocity.set(v);
@arc_since_spawn.set(@arc_since_spawn + length(dP));
```

`a` is the sum of three force sources — see `biogrowth-plan.md` "Per-Kernel Changes → integrate" and `network.md` "integrate" for the full breakdown.

## Tuning limits

- **`damping · dt < 1`** — otherwise `(1 - damping·dt) < 0` and velocity flips sign each frame. At 24fps → `damping < 24`. Default 2.0.
- **`max_speed · dt ≥ step_size`** — otherwise topology never spawns (arc_since_spawn never reaches threshold). Default `max_speed = 2 · step_size / expected_dt`.
- **`max_speed · dt ≪ many node-lengths`** — if one frame traverses far, polylines zig-zag visually. Keep `max_speed · dt` in O(step_size).
- **No stiff forces** — attract / tropism strengths are bounded by user parms; no `1/d² → ∞` behaviour once `kill_radius` clamps the falloff. Semi-implicit Euler is stable in this regime without further precaution.

## If we later need higher accuracy

- **Substeps first.** Run integrate twice per frame with `dt/2` — doubles integrate cost but **not** force-eval cost (same `a` reused). Drop-in upgrade, no architecture change.
- **Velocity Verlet.** Accept the 2× attract cost only if substepped semi-implicit Euler still visibly jitters. Would require evaluating forces against the half-step P, which means a second attract pass per frame — the whole point of avoiding it.
- **Damping ramp.** For smoother settling near attractors, let `damping` increase with local attractor density (more pulls → more bleed). Cheap, doesn't change the scheme.

## Sources

- [Integration Basics — Gaffer on Games](https://gafferongames.com/post/integration_basics/)
- [Semi-implicit Euler — Wikipedia](https://en.wikipedia.org/wiki/Semi-implicit_Euler_method)
- [Verlet integration — Wikipedia](https://en.wikipedia.org/wiki/Verlet_integration)
- [Stiff Springs, Velocity Verlet vs Semi-Implicit Euler — GameDev.net](https://comic.gamedev.net/forums/topic/586988-stiff-springs-velocity-verlet-vs-semi-implicit-euler/)
