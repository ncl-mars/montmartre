# Reaction / Compression Wood

Gravity-biased Murray. Horizontal / hanging branches thicken more than vertical ones — real trees fill the underside with reaction wood to resist bending.

## Rule

In `murray_propagate`, the `r_child^p` contribution pushed into the parent is scaled:

```
mult = 1 + reaction_k × max(0, -dot(branch_axis, up))
```

where `branch_axis = normalize(child_P - parent_P)`. Positive contribution only when the axis has a downward component — drooping branches thicken ancestors more than upright ones.

## Attributes / kernels

None new. One-line tweak inside `murray_propagate`, reads parent's `P` via `@P.data` flat buffer + `vload3`.

## Parms (solver spare)

- `reaction_k` — float, default `0.3`. Strength of gravity bias. `0` = pure Murray.
- `up` — float3, default `(0, 1, 0)`. World up.

## Toggle

Switch-SOP `reaction_wood_enable` pattern (bypasses `murray_propagate`'s reaction variant) — or simpler: `reaction_k = 0` reduces to pure Murray, so no Switch is strictly necessary.
