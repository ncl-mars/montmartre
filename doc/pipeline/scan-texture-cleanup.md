# Scan texture cleanup (Copernicus + ONNX)

*montmartre R&D — pipeline note. Compiled 2026-07-20.*

Cleaning the **baked textures** of a photogrammetry reconstruction (`assets/scan/<location>/tex`) so they're
usable for lookdev: fill missing/dirty regions, strip baked lighting to a clean albedo, and recover resolution.
Runs in **Copernicus** with the generic **ONNX Inference COP** (plus deterministic fill nodes for the small
stuff). See [realityscan.md](../reference/realityscan.md) for how the scan/textures are produced,
[ml-copernicus.md](../reference/ml-copernicus.md) for the ML node surface, and
[onnx-inference.md](../reference/onnx-inference.md) for the ONNX COP mechanics + wider model catalog.

> **Where this sits:** RealityScan project + source stills stay in the capturing shot's `prep/photogram/`; only
> the clean output mesh + tex go to `assets/scan/<location>/`. This cleanup operates on that output texture set,
> in **UV/texture space** (stable across camera moves), and writes back into the asset.

---

## The three jobs, in order

Do them in this sequence — each assumes the previous is done.

### 0. Get a mask
Everything downstream needs a "where's the problem" mask.
- **Coverage gaps are free** — the scan already knows where there was no data (zero-coverage UV islands).
  Use that as the hole mask directly; do **not** key luminance (a genuinely dark surface reads black too).
- **Dirt / occlusion** (leaves, wires, a passing pigeon baked into the texture) — paint it, or prompt
  **SAM2** (`Neural Layer to Mask COP`) with a couple of clicks to segment the offending region.

### 1. Fill holes / remove dirt — **LaMa inpainting**
The workhorse. [LaMa](https://github.com/advimman/lama) ("large-mask", Fast-Fourier-Convolution based) is the
standard 2D inpainter that most photogrammetry / 3D pipelines bolt on. Clean ONNX port ready for Copernicus:
[**Carve/LaMa-ONNX**](https://huggingface.co/Carve/LaMa-ONNX).

Wiring facts that shape the setup:
- **Two inputs** — the RGB texture *and* the mask (white = fill). Use `Channel Split` / multi-input on the
  ONNX Inference COP. ONNX I/O is **order-dependent** (not matched by name) — match the model's input order,
  then hit *Setup Shapes from Model*.
- **Fixed 512×512, opset 17.** For a 4K scan texture, **tile** into 512 patches with **overlap + feather** to
  hide seams (tiling preserves detail; a straight resample down/up loses it). Full method —
  [onnx-tiling.md](onnx-tiling.md).
- LaMa deliberately produces **smooth, plausible** fills rather than crisp invented detail — which is what you
  want on a surface texture that has to stay consistent across views.

> **Don't ML the small stuff.** Tiny holes and thin gaps go to the deterministic
> [`Fill Holes` / `Poisson`](https://www.sidefx.com/docs/houdini/nodes/cop/fill.html) COPs — cheaper, exact,
> temporally stable. LaMa is only for regions big enough to need *invented* structure.

### 2. Strip baked lighting → clean albedo — **intrinsic decomposition / delighting**
Photogrammetry textures bake in the day's lighting, which fights Karma relighting. **Intrinsic image
decomposition** separates reflectance (albedo) from shading, recovering a clean base colour for the MaterialX
shader. Active research area for [photogrammetry albedo recovery](https://arxiv.org/abs/2204.04142).

- **Least turnkey of the three.** No single blessed ONNX file the way LaMa has one — expect to convert an
  intrinsic-decomposition / delighting model to ONNX yourself, and quality varies with the surface. Treat as a
  spike, not a guaranteed win.
- Diffusion-based approaches (Marigold-style intrinsics) are the current frontier if the simpler nets aren't
  clean enough.

### 3. Recover resolution — **super-resolution**
If a texture is soft or you want 2–4× more pixels for hero close-ups: Real-ESRGAN / SwinIR both have mature
ONNX exports. Single-input RGB → RGB — the easiest of the three to wire.

---

## The two things that will bite you

- **Colour space — don't feed scene-linear.** These models expect display-referred **sRGB in `[0,1]`**.
  Convert *out* of ACEScg before inference and back after (the *Brightness Multiplier* 1/255 handles 0–255
  models). Feeding scene-linear straight in is the #1 "why does it look wrong" bug. See
  [color-management.md](color-management.md).
- **Execution provider (Windows).** **DirectML** runs with zero install on any GPU. The faster **CUDA** path
  needs NVIDIA's CUDA toolkit + cuDNN present. Leave on *Automatic* unless chasing throughput; set *Device ID*
  on multi-GPU.

---

## Full sequence

```
scan texture (assets/scan/<loc>/tex)
   │
   ├─ coverage/UV mask  (+ SAM2 for dirt)
   │
   ├─ small gaps  → Fill Holes / Poisson        (deterministic, no ML)
   ├─ large gaps  → LaMa inpaint  (ONNX, 512 tiles + feather)
   ├─ (optional)  → delight → albedo             (intrinsic decomposition, DIY ONNX)
   └─ (optional)  → super-res  (Real-ESRGAN/SwinIR ONNX)
   │
   └─ write back → assets/scan/<loc>/tex  (cleaned set for MaterialX lookdev)
```

---

## Sources

- [LaMa — resolution-robust large-mask inpainting (WACV 2022)](https://github.com/advimman/lama)
- [Carve/LaMa-ONNX — ONNX port (512×512, opset 17)](https://huggingface.co/Carve/LaMa-ONNX)
- [Intrinsic image decomposition for photogrammetry albedo recovery (arXiv 2204.04142)](https://arxiv.org/abs/2204.04142)
- [ONNX Inference COP](https://www.sidefx.com/docs/houdini/nodes/cop/onnx.html) · [How to use ONNX Inference](https://www.sidefx.com/docs/houdini/copernicus/onnx_inference.html)
- [Fill COP (deterministic hole fill)](https://www.sidefx.com/docs/houdini/nodes/cop/fill.html)
