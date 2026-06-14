# Reviving the int-atomic accumulator pattern

`v@force_acc` is a plain float3 because every contributor writes to its own point (single-writer-per-destination). The moment a contributor needs to write **into another point** (cross-point: pairwise forces, signal propagation, parent/child messaging), the plain-float3 pattern breaks — concurrent writes race and results are non-deterministic. At that point, the cross-point rule gets its own dedicated accumulator using the int-atomic fixed-point pattern.

`att_*` (int3-atomic, direction + count) and `murray_acc` (int-atomic, scalar) are the reference implementations already in the codebase.

## Naming

- `<name>_acc` suffix → persistent per-point **accumulator** attribute. Written by one or more contributors, consumed/reset per frame. Examples: `v@force_acc`, `i@murray_acc`, `i@dominance_acc`, `f@damage_acc`.
- Acceleration is never an attribute — it lives only as a local variable (`float3 a`) inside `integrate`.
- Exception: `att_*` predates the convention. Left as-is; the `att_` prefix is its own namespace.

## When to revive

Any contributor where **writer ptnum ≠ destination ptnum**:
- **Dominance suppression** — tip writes into its parent's accumulator (auxin flowing basipetally; see [biological-rules.md](../biological-rules.md) §6).
- **Pairwise self-collide** — each tip pushes every in-range other tip back (today's `self_collide` only writes to `me`; a symmetric version would write to both ends of every pair).
- **Tips → root propagation** for any rule that aggregates child state on the parent (Murray already does this via its own `i@murray_acc`, which *is* a fixed-point int atomic — use it as a second reference).
- **Per-cell / per-voxel aggregation** (e.g. splatting tip density into a grid detail array).

Single-writer contributors (density, photo, surface, the current one-sided self_collide) stay on plain `v@force_acc` — no atomic needed.

## How to revive — scalar accumulator (example: dominance suppression)

Most cross-point signals are **scalar** (strength, credit, debt). One int + one atomic is enough. Use the three-int vector pattern (like `att_*`) only when the signal is a direction.

### 1. Attribute schema

Add **one** int point attribute, zero-initialized in `set_seed` and `topology` (both spawn paths):

```vex
// set_seed.vfl
i@dominance_acc = 0;
```

```vex
// topology.vfl — on every child spawn
setpointattrib(0, "dominance_acc", child, 0);
```

Strip from output via `del_grid_attr` (or whichever attribute-delete node carries the solver exit list).

Fixed-point scale: add a constant to [biogrowth.cl](../hou/scripts/ocl/include/biogrowth.cl):

```cl
#define BG_DOMINANCE_SCALE 10000.0f   // int32 max → ~214k units headroom
```

At `1e4`, a float signal in `[0, 21.4e4]` fits in int32. Signals are bounded by user strength parms so saturation isn't a concern in practice.

### 2. OCL kernel — writer (cross-point)

```cl
#bind point   P                  float3 read        // read-only; we write to OTHER points
#bind point   is_tree_node       int    read
#bind point   is_tip             int    read
#bind point   parent_id          int    read
#bind point  &dominance_acc     int

#bind parm    dominance_strength float  val=1.0
#bind parm    dominance_decay    float  val=0.7

@KERNEL
{
    if (@is_tree_node < 0.5) return;
    if (@is_tip       < 0.5) return;

    int me     = get_global_id(0);
    int parent = @parent_id;
    if (parent < 0) return;

    // Each frame, tip emits `dominance_strength` into its parent.
    // Then a second pass (next frame) will propagate parent → grandparent × decay,
    // and so on up the lineage. One hop per frame = transitive over time,
    // same pattern as Murray.
    float signal = @dominance_strength;
    int   q      = (int)(signal * BG_DOMINANCE_SCALE);

    __global int* adst = (__global int*)@dominance_acc.data;
    atomic_add(&adst[parent], q);
}
```

Raw-pointer cast pattern `(__global int*)@attrib.data` is how you write to a **different** point number — `@attrib.set(v)` only writes to the current thread's own point. Same trick works for any attribute from any input (see the memory entry *OCL: cross-input arbitrary access*).

### 3. OCL kernel — propagate (interior nodes, one hop per frame)

Separate kernel runs after the writer. Each interior node bumps its own parent by `self_acc × decay`, extending the signal one hop further up per frame:

```cl
#bind point   is_tree_node       int    read
#bind point   is_tip             int    read
#bind point   parent_id          int    read
#bind point  &dominance_acc     int

#bind parm    dominance_decay    float  val=0.7

@KERNEL
{
    if (@is_tree_node < 0.5) return;
    if (@is_tip       >= 0.5) return;   // interior only
    int parent = @parent_id;
    if (parent < 0) return;

    int self_q = @dominance_acc;
    if (self_q == 0) return;

    int decayed = (int)((float)self_q * @dominance_decay);
    if (decayed == 0) return;

    __global int* adst = (__global int*)@dominance_acc.data;
    atomic_add(&adst[parent], decayed);
}
```

### 4. OCL kernel — consumer (reads, acts, resets)

The consumer is usually the rule that the signal modulates — for dominance, that's `topology`'s lateral-spawn gate. Decode fixed-point → float, fold into the threshold, then reset for next frame.

```cl
// inside topology or a dedicated decode kernel
float dominance = (float)@dominance_acc / BG_DOMINANCE_SCALE;

// Modulate lateral-spawn threshold (higher dominance → harder to spawn laterals)
float threshold = @lateral_base_threshold
                * (1.0f + @vigor_penalty    * split_depth)
                * (1.0f + @dominance_weight * dominance);

// ... use threshold ...

// Reset for next frame (critical — otherwise ints accumulate across frames)
@dominance_acc.set(0);
```

## Three-int vector pattern (reference: `att_*`)

When the signal is a **direction**, not a scalar, use three ints + `bg_atomic_add_dir` / `bg_decode_dir`:

```cl
#bind point &xdir_acc_ix   int
#bind point &xdir_acc_iy   int
#bind point &xdir_acc_iz   int

// writer
__global int* adx = (__global int*)@xdir_acc_ix.data;
__global int* ady = (__global int*)@xdir_acc_iy.data;
__global int* adz = (__global int*)@xdir_acc_iz.data;
bg_atomic_add_dir(adx, ady, adz, target_pt, dir * strength);

// consumer
float3 signal = bg_decode_dir(@xdir_acc_ix, @xdir_acc_iy, @xdir_acc_iz);
@xdir_acc_ix.set(0); @xdir_acc_iy.set(0); @xdir_acc_iz.set(0);
```

Uses the existing `BG_DIR_SCALE = 1e4` from [biogrowth.cl](../hou/scripts/ocl/include/biogrowth.cl).

## Reset discipline

Every cross-point accumulator must be zeroed **every frame** by exactly one consumer. Two failure modes:

- **No reset** → ints grow unboundedly across frames; saturation bug.
- **Double reset** (writer also resets, or two consumers both reset) → signal lost before use.

Pick one kernel (usually the one reading the signal) to own the reset, and document it at the binding site.

## Why not just leave everything as `v@force_acc`?

Plain float3 `.set(@attr + x)` writes are not atomic. If two work-items simultaneously read-modify-write the same point, one write is lost — standard race. OpenCL 1.2 has no `atomic_add` on floats; emulating via `atomic_cmpxchg` on the int-reinterpreted float is possible but slower and spikier than the fixed-point int-atomic pattern. Int-atomic is the pragmatic choice for cross-point writes in this codebase.

## Migration checklist

Scalar accumulator (dominance-like):
- [ ] Add `i@<name>_acc` to set_seed + both topology spawn paths (zero-init).
- [ ] Add scale constant `BG_<NAME>_SCALE` to biogrowth.cl.
- [ ] Writer kernel binds `&<name>_acc int`, writes via `atomic_add` through raw pointer cast.
- [ ] (Optional) Propagate kernel for transitive hop-per-frame signals.
- [ ] Consumer kernel reads `@<name>_acc`, decodes by dividing by scale, resets to 0.
- [ ] Strip `<name>_acc` in `del_grid_attr` so it doesn't leak into the mesh output.
- [ ] Verify under frame-by-frame bake: consumer output stable across frames (no drift from missing reset) and scales linearly with writer strength (no saturation).

Vector accumulator (self-collide-pairwise-like): same, but three int attribs (`_ix/iy/iz`) and `bg_atomic_add_dir` / `bg_decode_dir` helpers; reuse `BG_DIR_SCALE`.
