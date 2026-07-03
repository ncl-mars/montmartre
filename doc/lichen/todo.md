# Japanese Lichen System — Build Plan / TODO

The dedicated Japanese lichen simulation system — the hero element of **Episode 1 (summer)**. Lives in
`hou/lichens.hiplc`. The BioGrowth code (`d:/SPEC/BioGrowth/`) is **imported as donor material and
transformed into the lichen system** — a clean break: no upstream relationship, no merge-back. BioGrowth
is not maintained alongside; the lichen system is canonical.

- **Timeline:** ~1 month — started 2026-06-06, target ~**2026-07-06**.
- **Goal:** a complete Japanese lichen simulation system.
- Biology + per-species rules reference: [../tech/biogrowth/biological-rules.md](../tech/biogrowth/biological-rules.md);
  upstream solver design: [../tech/biogrowth/network.md](../tech/biogrowth/network.md), [biogrowth-plan.md](../tech/biogrowth/biogrowth-plan.md).

---

## Requirements

- **3–5 lichen species** in one sim — Japanese, **fruticose** (branching) morphology.
  Hero: **Usnea / saruogase** (pendulous beard). Secondary: **Cladonia**-type cushions. (Crustose /
  foliose / moss are a different primitive — out of scope, see below.)
- **Per-species variation:** color, growth rate, branching behaviour, tropism, length cap.
- **Inter-species interaction (KEY):** competition for substrate, colony boundaries, overgrowth/territory.
- **Massive coverage:** lichen blankets large surface areas — scale/performance is a first-class concern.

---

## Architecture decision (2026-06-06)

**Single solver, species-tagged nodes, one shared spatial grid.** (Not N separate sims — interaction is a
primary goal, and the shared grid makes cross-species queries cheap.)

- Add `i@species` to every node.
- Lift global solver spare-parms → **per-species lookup** (species-indexed detail arrays) for values that
  differ per species (speed, branching density, tropism, length cap, color).
- Interaction = **species-aware variants** of existing kernels (`attract`, `self_collide`, `cull`) —
  reuse the grid, filter/branch on `species`.
- Fully dedicated to lichen → **tree-only cruft can be removed** (strong apical dominance defaults,
  mechanical-era sag, hero-trunk Murray); bake in lichen assumptions.

---

## First steps

- [ ] **Import the BioGrowth code/solver into `lichens.hiplc`** and begin transforming it for lichen.
- [ ] **Scale: handled in-project, non-blocking.** Thin filaments hit Murray/allometric underflow at sub-mm
      radii. Default fix: **scale the scene up for the sim, scale back down after** (global wrapper, no
      kernel changes). Promote to a real `sim_scale` parm only if that proves insufficient. Not a gate.

---

## Staged plan (each stage ships a watchable result)

### Stage 0 — Multi-species scaffold (no interaction yet)
- [ ] `i@species` attribute, seeded per holdfast.
- [ ] Per-species param lookup (speed / branching / tropism / length cap).
- [ ] Per-species **color** → `Cd` via tuned `color.cl` palettes.
- [ ] Two species growing independently, distinct look. Confirms the scaffold.

### Stage 1 — Cross-species exclusion (boundaries)
- [ ] Species-aware `self_collide`: stronger repulsion against other species → visible colony boundaries.

### Stage 2 — Competition / territory
- [ ] Species-aware `attract` / `cull`: first species to claim a region's attractors locks others out
      (succession, overgrowth, territory).

### Stage 3 — Massive coverage (scale & perf)
- [ ] Coverage strategy: dense attractor field over the reconstructed surface; tiling / region splitting if needed.
- [ ] Performance pass: BioGrowth ran ~1s/frame at 800k nodes — "massive" likely pushes past that.
      Profile; consider LOD, instancing of mature tufts, or sim-then-scatter for background coverage.

### Stage 4 — Behaviour & look polish
- [ ] Per-species morphology dial-in against reference (Usnea pendulous vs Cladonia cushion).
- [ ] Surface fuzz (fibrils/soredia) as render-time detail (displacement/instanced), not solver branching.
- [ ] Sway → downstream mechanical deformer (not the growth solver; currently unbuilt) if needed.

---

## Open / needs from production

- [ ] Director reference board → lock the 3–5 species + their look
- [ ] Substrate(s) the lichen colonizes in-plate (rock? bark?) → seeding + surface reconstruction
- [ ] Growth-rate ratios between species (which out-competes which)
- [ ] Color reference per species → tuned palettes
- [ ] Coverage scale target (how much surface, how dense) → drives the perf budget

---

## Out of scope

- Crustose / foliose / moss looks (different primitive — reaction-diffusion / instanced fuzz, not SCA).
- Mechanical sway/wind animation (downstream deformer, post-growth — see two-era model in the plan doc).
- A reusable/general tool **yet** — MONTMARTRE builds the lichen system bespoke for Episode 1 first; the
  generic growth-VFX tooling promotes to `d:\FLAT\_studio` once it proves out — **later**, not now.
