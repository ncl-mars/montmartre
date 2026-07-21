# Tiling fixed-input ONNX models in Copernicus

*montmartre R&D — pipeline note. Compiled 2026-07-20.*

Running a **fixed-input** ONNX model (e.g. big-LaMa at 512×512) across a large image (a 4K scan texture) at
**native resolution** by cutting it into tiles, inferring each, and reassembling seamlessly. **Downsampling the
whole image to the model size is deliberately out of scope here** — this doc is only about keeping full
resolution.

Parent references: [onnx-inference.md](../reference/onnx-inference.md) (the COP mechanics) and
[scan-texture-cleanup.md](scan-texture-cleanup.md) (the workflow that needs this).

---

## 1. Why naïve tiling seams

Chop the image into an abutting grid, infer each tile independently, and you get **visible seams** on every
boundary. Three causes:

1. **Edge / padding artifacts.** At a tile border the model has no data past the edge and pads (usually zeros).
   Models with a **large receptive field** — LaMa's Fourier convolutions are effectively *global* — degrade in a
   band near every edge.
2. **Per-tile global statistics.** Any model that normalises over its whole input computes different stats per
   tile → tone/brightness steps at the join.
3. **Independent predictions.** Nothing forces tile A's right edge to agree with tile B's left edge.

The fix is three things stacked: **overlap**, **feather**, and a **valid-crop** border.

---

## 2. Overlap

Step the tiles by **less** than the tile size so neighbours share a margin. A 512 tile advancing 384 px leaves a
**128 px overlap**; every pixel in a seam zone is then covered by 2 tiles (4 at corners) — the material you blend
across.

Size the overlap to the model's receptive field: **wide-receptive-field models (LaMa) want a generous overlap**
(128); small-receptive-field models (super-res) need only 16–32.

---

## 3. Feather (weighted blend)

In the overlap band, don't hard-cut — **weighted-average** the overlapping tiles with a falloff:

```
out(p) = Σ_t  w_t(p) · tile_t(p)   /   Σ_t w_t(p)
```

`w_t(p)` is the **feather weight**: 1 at the tile centre, ramping to 0 at the tile edge (a 1D window multiplied
in x and y).

**Window choice:**
- **Linear (triangular)** — fine and cheap.
- **Raised-cosine / Hann** — `w(d) = 0.5·(1 − cos(π·d/margin))` over the overlap margin. Smoother (no derivative
  kink), and at overlap = ½·tile it forms a **partition of unity** (weights sum to 1) → no normalisation drift.
  Use this.

```
tile A weight  ▔▔▔▔▔╲______
tile B weight  ______╱▔▔▔▔▔   →  smooth crossfade across the overlap, no seam
```

---

## 4. Valid-crop (context border)

Feather alone still *blends in* the poor edge pixels. Better: feed a full 512 tile but **keep only the central
region** (e.g. 384×384); the outer ~64 px on each side is fed purely as **context** the model sees and you
discard. Abut the valid centres with a thin feather. For large-receptive-field models like LaMa this is the
single biggest quality win — usually combine it *with* a small feather.

---

## 5. Building it in Copernicus

No auto-tile exists on the ONNX node — assemble it from primitives. Per-tile op chain, accumulated across the grid:

1. **Extract** the tile: crop/inset the source to a 512 window at `(tx, ty)`.
2. **Infer**: ONNX Inference COP on that tile.
3. **Weight**: multiply the tile output by the **feather mask** (precompute *once* — same mask every tile) and,
   if valid-cropping, by the valid-region mask.
4. **Place & accumulate**: transform the weighted tile back to `(tx, ty)` and **add** it into two accumulator
   layers — the weighted colour `Σ w·tile` and the weight `Σ w`.
5. **Iterate** over the tile grid, then **normalise**: `out = Σ(w·tile) / Σ(w)` (guard divide-by-zero).

The iteration is driven by a **COP loop / feedback block** stepping `(tx, ty)` across the grid.
> ⚠ *Confirm the exact loop-block node name in the H22 tab menu before building — this doc specifies the
> algorithm, not the node label.*

**Precompute the feather mask once** — it's identical for every interior tile; only edge/corner tiles of the whole
image differ (they have no neighbour on one side, so their weight there should stay 1, not fall off, or you'll
darken the image border).

**Skip empty tiles.** For inpainting, a tile whose mask has no holes needs no inference — pass the source through.
Big saving on sparse damage.

---

## 6. Defaults (LaMa-class, native-res)

| Param | Value |
|---|---|
| Tile | 512×512 (model-fixed) |
| Overlap | 128 px |
| Valid crop | discard outer ~48–64 px |
| Feather | raised-cosine over the overlap band |
| Image-border tiles | clamp weight to 1 on the outer side (no falloff into the void) |

---

## 7. Per-task notes

- **Super-resolution — tiles freely.** Small receptive field, tiles blend cleanly; 16–32 px overlap is plenty.
  ⚠ the output tile is **N× larger** — place it at `N·(tx, ty)` and size accumulators to the output resolution.
- **Inpainting — watch hole size.** The model must *see a whole hole* to invent coherent structure. A hole
  larger than `tile − overlap` is never fully visible to any tile and the fill breaks down. Tiling is for
  **small, scattered** damage; whole-image context for a big hole is a different (downsampling) approach, out of
  scope here.
- **Depth / normal — don't tile.** Each tile predicts *relative* values on its own scale, so adjacent tiles
  disagree on absolute level → step discontinuities feather can't hide. These models are usually dynamic-input
  anyway; run them whole.

---

## 8. Gotchas

- **Colour space consistency.** Do the feather blend in the *same* space you infer in (display-referred sRGB,
  per [color-management.md](color-management.md)); convert back to ACEScg only after normalising.
- **Premultiplication.** If the tile output carries alpha, blend premultiplied, or the feather will fringe.
- **Check seams at grazing exposure.** Boost contrast / add a temporary ramp over the result to reveal any
  residual boundary before you commit — seams hide in midtones.

---

## Sources

- [ONNX Inference COP](https://www.sidefx.com/docs/houdini/nodes/cop/onnx.html)
- [LaMa — resolution-robust large-mask inpainting](https://github.com/advimman/lama) · [Carve/LaMa-ONNX (512×512)](https://huggingface.co/Carve/LaMa-ONNX)
