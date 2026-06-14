# TODO

> Per-rule documentation lives in [network.md](network.md) under "Biological rules". Per-feature plan docs are archived in `archive/` for historical reference. This file is a chronological landing log + pending work.

## Landed (chronological)

- **VDB constraints** (feature/vdb-constraints, 2026-04-19/20): avoid+collision fused into integrate; surface / density / photo as plugin contributors on the constraints bus. [archive/vdb-constraints.md](archive/vdb-constraints.md).
- **Vigor** (feature/vigor, 2026-04-20): depth-cycled grid filter + pressure-gated lateral branching. [archive/vigor.md](archive/vigor.md).
- **Self-collide** (feature/self-collide, 2026-04-20): per-tip 27-cell repulsion with bidirectional `parent_id` ancestor walk. [archive/self-collide.md](archive/self-collide.md).
- **Sweep meshing** (feature/sweep-meshing, 2026-04-20): per-frame tube meshing outside solver. [archive/sweep-meshing.md](archive/sweep-meshing.md).
- **Murray's rule** (feature/murray-rule, 2026-04-20): tips→root `r_parent = (Σ r_child^p)^(1/p)` propagation. Originally required stable-id machinery (later removed by split-streams). [archive/murray-rule.md](archive/murray-rule.md).
- **Lateral start offset** (feature/lateral-offset, 2026-04-21): `offset_lateral_start` wrangle hides Y-junction seams; polypath + resample added to mesh chain.
- **`force_acc` simplification + rename** (feature/acc-float3, 2026-04-22): plain float3 accumulator replaces int-atomic `acc_*` pattern (single-writer per point, no race). `att_*` kept on int-atomic since attract genuinely writes cross-point. Naming convention: `_acc` suffix = persistent per-point accumulator. Revival recipe for cross-point contributors: [acc-int-atomic-revival.md](archive/acc-int-atomic-revival.md).
- **Allometric length limit** (feature/allometric-length, 2026-04-22): McMahon `L_max = k × r^exp`. Originally a standalone cull kernel; later refactored to in-kernel velocity-zero inside integrate. [archive/allometric.md](archive/allometric.md).
- **Phyllotaxis + per-node cap + Leonardo** (feature/phyllotaxis, 2026-04-23/24): golden-angle azimuth + force-balance pitch + `max_laterals_per_node` cap, four-mode toggle matrix.
- **Reaction wood** (feature/reaction-wood, 2026-04-24): gravity-biased Murray contribution. [archive/reaction-wood.md](archive/reaction-wood.md).
- **Apical dominance** (feature/apical-dominance, 2026-04-24): auxin-like child→parent suppression signal; first `att_*` int-atomic revival. [archive/apical-dominance.md](archive/apical-dominance.md).
- **Branch autonomy** (feature/branch-autonomy, 2026-04-24): light-debt growth scaling fused into integrate; scalar `irradiance` VDB on constraints bus. [archive/branch-autonomy.md](archive/branch-autonomy.md).
- **Split-streams refactor** (refactor/split-streams, 2026-04-25): tree_nodes and attractors run on separate streams inside the solver. Entire stable-id chain deleted; `parent_id` permanently valid as raw ptnum. ~1s/frame at 800k tree_nodes.
- **Damage event bus** (feature/damage, 2026-04-26/27): `f@damage` orthogonal bus; `damage_field` (VDB-driven event source) + `damage_propagate` (parent → child max-with-decay) kernels; one-line consumer modulators in dominance, Murray, and integrate (tip growth-factor + light-credit drain). [archive/damage.md](archive/damage.md).
- **Avoid extraction** (refactor/avoid-extraction, 2026-04-29): `avoid` lifted out of integrate into its own soft contributor on a dedicated `avoid_sdf` VDB; collision stays fused. [archive/avoid.md](archive/avoid.md).
- **Velocity-modifier refactor** (refactor/velocity-modifiers, 2026-04-29): integrate split into force→v + chain of velocity modifiers (allometric, branch_autonomy, damage_growth) + apply (v→P + collision + writeback). Plug-in slot for future modifiers between integrate and apply.
- **Senescence + arc_total_since_seed** (feature/senescence, 2026-04-29): per-lineage maturity rule — first new modifier in the slot. `f@arc_total_since_seed` accumulates cumulative path from root in apply.

## Later: arc_total_since_seed attribute

Cumulative path length from root to each node — complements `arc_total_since_lateral` (which resets per segment) and `arc_since_spawn` (which resets per step). Init 0 on seed, accumulate per-frame in integrate, inherit on both tip extension and lateral spawn (no reset). Useful for age-like proxies, apical-dominance decay, depth/length-based thickness curves, or length-scaled variants of the per-node lateral cap.

## Later: second pass on biological realism

Four remaining candidate rules in the Murray spirit — phyllotaxis (golden-angle laterals), Leonardo's branching-angle rule, reaction/compression wood, branch autonomy (self-pruning by light debt). Plus damage accumulator as an orthogonal event bus. Ranked and sketched in [biological-rules.md](biological-rules.md). Next recommended: phyllotaxis (cheapest visible gain — angle-pick in topology's lateral spawn) or dominance suppression (needs the int-atomic cross-point revival — see [acc-int-atomic-revival.md](archive/acc-int-atomic-revival.md)).

## Later: global simulation scale factor

Murray misbehaves at very small radii — `r^p` with p≈3 underflows / loses precision when child radii are sub-mm, so parent radii collapse or stay pinned at `r_init`. Same hazard for allometric `L_max = k·r^exp` and any radius-driven law. Fix is numerical, not semantic: keep the working values near 1 instead of building a unit system.

Approach: scale input geometry at the solver entrance to a canonical working range (trunk ≈ 1 unit), unscale at the sweep/output stage. Zero kernel changes — radius-law math runs on well-conditioned floats regardless of whether the source is a forest tree or a perfume-bottle vine. If geometry-scaling proves insufficient (e.g. VDB voxel sizes need matching adjustment), promote to an explicit `sim_scale` detail parm consumed by radius-law kernels.

## Later: branch seams / Y-junction holes

`offset_lateral_start` (landed on `feature/lateral-offset`) hides the seams by tucking the lateral's first vertex inside the parent tube. Watertight meshing at junctions still requires a `sweep → VDB from Polygons → Convert VDB (high voxel res)` post-pass. Defer until direct-sweep cosmetics bite (glancing angles, alpha compositing, displacement).

---

## Background: Houdini Project Snapshot Tool

Shelf tool that dumps the full node network to disk so Claude can read the project without needing the .hip open. Needed eventually — would let future sessions start cold without me having to ask for every Temp file.

### Output
- One file per snippet node — VEX wrangles, OCL kernels, Python SOPs. Named by node path (e.g. `obj_geo1_solver_grid_count_kernelcode.cl`), written to `$HIP/scripts/dump/` or `.claude/dump/`.
- `_network.md` — full graph: node name, type, connections, key non-snippet parms, ordered top-to-bottom.

### Python
- `hou.node().parm("snippet").eval()` — reads any code snippet
- `hou.node().outputs()` / `.inputs()` — connection graph
- `hou.node().parmTemplateGroup()` — enumerate parms
- Walk recursively from a root node

### Usage
- Run before a Claude session. Optionally commit the dump, or add `.claude/dump/` back to .gitignore and regenerate on demand.
- Re-run when the network changes.
