# Apical Dominance

Tip-emitted suppression signal that dampens lateral sprouting on young wood near active leaders. Auxin-like model — signal flows child → parent (basipetally) one hop per frame, attenuating with `decay`. Lose a tip and its suppression drains over N frames → dormant buds below reiterate for free.

## Attributes

- `i@dominance_acc` — int fixed-point accumulator, written by all children into parent each frame; reset every frame by finalize.
- `f@dominance` — float per-node signal; set by finalize from this frame's `dominance_acc`. Consumed by topology.

Both seeded to 0 on seed and on every new child in topology. Uses the int-atomic cross-point pattern ([acc-int-atomic-revival.md](acc-int-atomic-revival.md)) — first real revival beyond `att_*`. Scale defined in `biogrowth.cl`: `BG_DOMINANCE_SCALE = 1000.0f`.

## Kernels

**`dominance_step`** — fused emit + propagate. Runs over tree_nodes; skips seed (`parent_id < 0`). Tips atomic_add `dominance_strength` into parent's acc; interior nodes atomic_add `own dominance × decay` into parent's acc. One pass, one atomic_add per node.

**`dominance_finalize`** — runs over tree_nodes. `@dominance = dominance_acc / BG_DOMINANCE_SCALE; dominance_acc = 0`. Mirrors `murray_finalize` pattern.

## Consumer

In topology's interior-branch threshold:

```
threshold = lateral_base_threshold
          × (1 + split_depth × vigor_penalty   × (vigor_filter_enable > 0))
          × (1 + dominance_k  × f@dominance    × (dominance_enable    > 0))
```

Each modulator multiplies in; each enable flag collapses its factor to `(1 + 0) = 1` when off. Zero-cost-when-off without a Switch-SOP.

## Toggle

Switch-SOP `dominance_enable` bypasses the `dominance_step → dominance_finalize` pair. When bypassed, `f@dominance` stays 0 → the topology factor collapses to 1 regardless of the in-kernel enable flag. Two-level safety.

## Solver order

Runs between `murray_finalize` and `cull` — i.e. before topology, so topology reads this-frame's `f@dominance`. No frame lag on the consumer.

```
… → integrate → murray_propagate → murray_finalize
             → dominance_step    → dominance_finalize
             → cull → topology → blast_unactive
```

## Parms (solver spare)

- `dominance_enable` — int, default 1. Switch-gate.
- `dominance_strength` — float, default 1.0. Per-frame emission from each tip.
- `dominance_decay` — float, default 0.7. Per-hop attenuation as signal walks toward root.
- `dominance_k` — float, default 1.0. Consumer multiplier in topology threshold.

## Tuning notes

- Propagation is **hop-wise**, not metric: `dominance_decay^N` at depth N from tip. With decay = 0.7, hop 4 ≈ 0.24.
- Only appears in steady state once the tree has depth ≥ 2 from any active tip. Shallow trees only show dominance on the direct parent of tips.
- `dominance_k` needs to be non-trivial (1.0–10.0 depending on `dominance` magnitude) before the topology threshold visibly shifts. Start high, back off.
- Biological read: strong leaders = long straight trunks with suppressed close-in laterals. Weak/lost leaders = drained dominance, dormant buds fire below — emergent reiteration.

## Composes with

- **Damage** (future): tip emits `strength × (1 - damage)` — damaged tips suppress less, reiteration of the subtree.
- **Vigor**: both multiply the threshold; vigor does depth heuristic, dominance does tip-distance real signal. Likely reduces need for `vigor_penalty > 0` once dominance is tuned.
