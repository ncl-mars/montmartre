# ONNX inference in Copernicus — capabilities & model catalog

*montmartre R&D — reference paper. Compiled 2026-07-20. Houdini 22.0.*

The complete picture of the **generic ONNX Inference COP**: what it is, how the node actually works, what
constrains it, and a task-by-task catalog of models you can realistically run — each flagged with a **licence**,
because montmartre tech promotes to `_studio` and non-commercial / AGPL weights are landmines there.

This is the deep reference behind the overview in [ml-copernicus.md](ml-copernicus.md); the first concrete
application is [scan-texture-cleanup.md](../pipeline/scan-texture-cleanup.md).

---

## 1. What "ONNX inference" means here

- **ONNX** (Open Neural Network Exchange) is an open, framework-neutral file format for a trained model — a
  frozen graph of tensor operations plus weights, in a single `.onnx` (weights >2 GB spill to a sidecar file).
- Houdini runs it through **ONNX Runtime**, which dispatches the graph to an **execution provider** (the actual
  compute backend). You bring a `.onnx`; Houdini feeds it image tensors and reads tensors back.
- Two host nodes:
  - **ONNX Inference COP** — image in, image out. The subject of this paper.
  - **ONNX Inference SOP** — geometry/point-attribute in/out (e.g. `Neural Terrain Generate`). Same engine,
    different data.
- The **wrapped** neural COPs (`Neural Layer to Depth` / MoGe-2, `Neural Layer to Mask` / SAM2, `NCA`,
  `ML Computer Vision Inference`) are pre-configured ONNX pipelines. The generic node is the escape hatch for
  everything they don't cover.

---

## 2. How the ONNX Inference COP works

### Load & auto-configure
1. Set **Model File** to the `.onnx`.
2. Click **Setup Shapes from Model** — this reads the graph and populates input/output **counts, names, shapes,
   and data types** automatically. Always do this first; hand-editing shapes is for edge cases.

### Inputs & outputs
- **Order-dependent, not name-matched.** Model inputs/outputs are bound in the order the ONNX file declares
  them — the *names* are informational. If a model wants `(image, mask)`, wire them in that order.
- **Single tensor:** wire a `File`/upstream COP straight in after setting the channel type (e.g. RGB).
- **Multiple tensors:** raise **Number of Inputs**, split with a **Channel Split COP**, and feed each (e.g. an
  RGB image tensor + a single-channel mask tensor for an inpainter).
- **Data types** supported: Float32, Float16, Int32, Int8, UInt8, Int64, Float64, Bool. (H22 added the non-float
  types and scalar/empty tensors.)

### Layout controls (the fiddly bit)
These reconcile Houdini's image convention with whatever the model was trained on:
- **Input/Output Tensor Shape** — 3×3 tables describing dimension layout.
- **Tensor Order** — `YX` (row-major/horizontal) vs `XY` (transposed). Wrong choice = output rotated/scrambled.
- **Collate Channels Separately** — interleaved `RGBRGB…` vs planar `RRR…GGG…BBB`. Models differ; this toggles it.
- **Flip Input/Output Vertically** — image-origin (top-left vs bottom-left) mismatch.
- **Resample Size** — scales the input to the model's expected resolution, aspect-preserving.
- **Brightness Multiplier** — `1`, `255`, or `1/255`. Bridges `[0,1]` float models and `0–255` integer models.

### Execution
- **Execution Provider** — *Automatic* (prefers CUDA → DirectML/CoreML by platform → CPU), or force **CPU**,
  **CUDA** (needs NVIDIA CUDA + cuDNN installed), **DirectML** (Windows, zero install), **CoreML** (macOS).
- **Device ID** — pick the GPU on multi-GPU boxes.
- **Max Batch Size** — processes the first dynamic dimension in chunks to cap peak VRAM.
- **>2 GB models** load via an external data file (H22); **content-dependent dynamic output sizing** is
  supported in H22 (output resolution can depend on the input).

---

## 3. Constraints & gotchas

- **Fixed vs dynamic input shape.** Many exported models bake a fixed resolution (e.g. big-LaMa is **512×512**).
  If the model has *dynamic axes* you can feed native resolution; if not, you either **Resample** to fit (loses
  detail) or **tile** the image into model-sized patches. Tiling needs **overlap + feather** to hide seams —
  full method in [onnx-tiling.md](../pipeline/onnx-tiling.md).
- **VRAM & latency.** Transformer/diffusion models are heavy; a 4K tile-loop can be minutes. Use *Max Batch
  Size* and DirectML's memory management; consider Float16 weights.
- **Operator / opset support.** ONNX Runtime must support every op in the graph on the chosen provider. Exotic
  ops (custom attention, deformable convs) may fail or silently fall back to CPU. Prefer models exported at a
  mainstream **opset (≥17)**.
- **Colour management.** Models expect **display-referred sRGB in `[0,1]`** (or `0–255`). Convert *out* of
  ACEScg before inference and back after — see [color-management.md](../pipeline/color-management.md). Feeding
  scene-linear in is the #1 wrong-result bug.
- **Temporal coherence.** Per-frame image models (depth, matte, inpaint) **flicker** across a sequence unless
  stabilised or conditioned. Fine for stills and texture bakes; careful on animated plates.
- **⚠ Licensing (studio-critical).** Model *weights* often carry a different licence than the *code*. Three
  buckets: **permissive** (MIT / Apache-2.0 / BSD — safe to promote to `_studio`), **non-commercial**
  (CC-BY-NC — research only, e.g. BRIA RMBG, some Depth-Anything-V2 large weights), and **AGPL** (Ultralytics
  YOLO — copyleft, contaminating for a studio pipeline). **Verify the weights licence before anything reaches
  `_studio`.**

---

## 4. What's possible — task catalog

Each task is an image→image (or image→map) operation you can run through the generic COP. Licences are the
*typical* state of the common weights — **always re-verify**.

### Restoration & cleanup
| Task | Models (ONNX-friendly) | Licence | Notes / montmartre use |
|---|---|---|---|
| **Inpainting** (holes, dirt removal) | [LaMa / big-lama](https://github.com/advimman/lama) ([ONNX port](https://huggingface.co/Carve/LaMa-ONNX)), MI-GAN, MAT | Apache-2.0 (verify weights) | Fill missing/dirty scan-texture regions. 512×512 fixed → tile. See scan-cleanup doc. |
| **Super-resolution** | [Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN), [SwinIR](https://github.com/JingyunLiang/SwinIR), BSRGAN, plus community 4× models on [OpenModelDB](https://openmodeldb.info) | BSD-3 / Apache-2.0 (community varies) | Upscale soft scan textures / HDRIs for hero close-ups. |
| **Denoise / restore** | [SCUNet](https://github.com/cszn/SCUNet), NAFNet, Restormer, DnCNN | Apache-2.0 / research | De-noise stills or scan textures. (Render denoise → prefer Karma/OIDN.) |

### Estimation (image → map)
| Task | Models | Licence | Notes / montmartre use |
|---|---|---|---|
| **Monocular depth** | [Depth Anything V2](https://github.com/LiheYoung/Depth-Anything), [MiDaS](https://github.com/isl-org/MiDaS), [Marigold](https://github.com/prs-eth/Marigold) | DA-V2 small/base Apache-2.0 · **large often CC-BY-NC** · MiDaS MIT | Depth mattes, proxy geo, z-comp. (MoGe-2 wrapped node gives *metric* scale.) |
| **Surface normals** | DSINE, StableNormal, Marigold-Normals, Lotus | mostly research (verify) | Normal maps from façades you photographed but didn't scan. |
| **Intrinsic / delight** | [Intrinsic (Careaga)](https://github.com/compphoto/Intrinsic), IID nets, Marigold-Intrinsics, StableDelight | research (verify) | Strip baked light → clean albedo for MaterialX. Least turnkey; often DIY-export. |
| **Optical flow** | [RAFT](https://github.com/princeton-vl/RAFT), GMFlow | BSD-3 / Apache-2.0 | Two-frame retime / temporal work. Heavy. |

### Segmentation & matting
| Task | Models | Licence | Notes / montmartre use |
|---|---|---|---|
| **Prompted segmentation** | SAM / SAM2 (wrapped `Neural Layer to Mask`) | Apache-2.0 | Click-mask zinc/stone/sky/lichen on plates + scan dirt masks. |
| **Salient / background matte** | [BiRefNet](https://github.com/ZhengPeng7/BiRefNet), [rembg](https://github.com/danielgatis/rembg) (U²-Net / ISNet), MODNet | **BiRefNet & rembg MIT** · BRIA RMBG **CC-BY-NC** | High-res subject isolation. Prefer BiRefNet/U²-Net for commercial safety; avoid BRIA weights. |
| **Object detect / semantic seg** | YOLO-seg, Mask2Former, ResNet/DeepLab (via `ML Computer Vision Inference`) | **YOLO = AGPL ⚠** · others Apache/MIT | Detect/segment lichen across a re-shoot series. AGPL YOLO is a studio landmine — pick a permissive backbone. |

### Generative / stylistic
| Task | Models | Licence | Notes / montmartre use |
|---|---|---|---|
| **Neural style / filters** | fast-neural-style ([ONNX Model Zoo](https://github.com/onnx/models)), AdaIN | BSD/MIT | AD-hat look exploration; not production shading. |
| **Colorization** | DeOldify, ONNX Model Zoo colorizer | MIT (verify) | Niche; reference/archival footage. |
| **Diffusion (SD / ControlNet)** | SD-Turbo/SDXL ONNX, ControlNet | mixed (SD = CreativeML OpenRAIL, use-restricted) | *Possible* with >2 GB support, but multi-model (text-enc + UNet + VAE) is awkward in one COP and VRAM-hungry. Usually better run outside Houdini. |

### Pattern generation
| Task | Models | Licence | Notes / montmartre use |
|---|---|---|---|
| **Neural Cellular Automata** | wrapped `NCA Core/Decode` + train-in-COP | — | Self-organising 2D patterns. *Explore-drawer* toy — **not** the SCA lichen element. |

---

## 5. Getting & making models

**Ready-made sources**
- [Hugging Face](https://huggingface.co) — filter by the `onnx` library tag; most nets above have a community port.
- [ONNX Model Zoo](https://github.com/onnx/models) — curated, permissively-licensed classics (style, SR, seg, detect).
- [OpenModelDB](https://openmodeldb.info) — huge catalog of community 4× upscalers with per-model licences.

**Converting PyTorch → ONNX** (when only a `.pth`/`.ckpt` exists)
- `torch.onnx.export(...)` — set **`dynamo=True`** (the current recommended exporter, `torch.export`-based).
- **`model.eval()` first** — Dropout/BatchNorm behave differently in train vs inference; exporting in train mode
  bakes the wrong behaviour.
- **`dynamic_axes`** to keep H/W/batch flexible: `dynamic_axes={'input':{0:'batch',2:'h',3:'w'}}`. Without it the
  input resolution is frozen and you must tile.
- **`opset_version`** ≥ 17 for good ONNX-Runtime coverage.
- **Unsupported ops**: replace with supported equivalents, or register a symbolic via
  `torch.onnx.register_custom_op_symbolic`. Some research nets need light surgery before they'll export cleanly.

See PyTorch's [torch.onnx docs](https://docs.pytorch.org/docs/stable/onnx.html).

---

## 6. montmartre applications (summary)

The canonical fit ranking lives in [ml-copernicus.md](ml-copernicus.md#how-it-fits-the-practice) — this is just
the pointer map from *this* paper's models to where they land:

- **Scan-texture cleanup** (first priority) — inpaint + delight + super-res on `assets/scan/…/tex`. Full
  workflow: [scan-texture-cleanup.md](../pipeline/scan-texture-cleanup.md).
- **Plate masks / roto** — SAM2 (wrapped) for comp mattes and scan dirt masks.
- **Depth / normal from plates** — MoGe-2 (wrapped, metric) into [camera-tracking.md](../pipeline/camera-tracking.md).
- **Seasonal coverage measurement** — permissive detector/segmenter over a re-shoot series.
- **Not the lichen element** — the lichen is a 3D SCA sim; neural texture/pattern gen (NCA, diffusion) stays in
  the explore drawer.

---

## Sources

- SideFX H22: [ONNX Inference COP](https://www.sidefx.com/docs/houdini/nodes/cop/onnx.html) · [How to use ONNX Inference](https://www.sidefx.com/docs/houdini/copernicus/onnx_inference.html) · [What's new: ML](https://www.sidefx.com/docs/houdini/news/22/ml.html)
- Model hubs: [Hugging Face](https://huggingface.co) · [ONNX Model Zoo](https://github.com/onnx/models) · [OpenModelDB](https://openmodeldb.info)
- Inpaint: [LaMa](https://github.com/advimman/lama) · [Carve/LaMa-ONNX](https://huggingface.co/Carve/LaMa-ONNX)
- Super-res / restore: [Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN) · [SwinIR](https://github.com/JingyunLiang/SwinIR) · [BasicSR](https://github.com/XPixelGroup/BasicSR)
- Depth / normal / intrinsic: [Depth Anything](https://github.com/LiheYoung/Depth-Anything) · [MiDaS](https://github.com/isl-org/MiDaS) · [Marigold](https://github.com/prs-eth/Marigold) · [Intrinsic](https://github.com/compphoto/Intrinsic)
- Matting: [BiRefNet](https://github.com/ZhengPeng7/BiRefNet) · [rembg](https://github.com/danielgatis/rembg)
- Export: [torch.onnx](https://docs.pytorch.org/docs/stable/onnx.html)
