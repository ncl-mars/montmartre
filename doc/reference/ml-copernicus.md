# Machine learning in Copernicus (Houdini 22.0)

*montmartre R&D — technical note. Compiled 2026-07-20. Scope: EP01 (summer) — lichen / photogrammetry.*

What actually shipped in the H22 image context, and the handful of nodes that land on a
lichen-growth, place-anchored practice.

---

## The shape of it

Copernicus doesn't "do AI" as one button. It exposes a **generic ONNX runner** plus a small set of
**purpose-built neural nodes**, and a **training side that lives in TOPs (PDG)**. Inference in COPs/SOPs,
training in PDG.

Everything runs pre-trained `.onnx` models on the GPU. You either drop in a model someone else trained,
use one of the wrapped nodes below, or train your own in the TOP graph and infer it back in the image
network.

---

## What's in the box (H22)

Inference surface grouped by context. Node names as they appear in the tab menu.

| Node | Context | What it does |
|---|---|---|
| **ONNX Inference** | COP | Runs *any* ONNX model on a layer. H22: >2 GB models via external data, 8 tensor dtypes, scalar tensors, per-GPU Device ID, real CPU multithreading, content-dependent output sizing. |
| **Neural Layer to Depth** | COP | MoGe-2 model — *metric-scale* depth, normal & position maps from a single image. |
| **Neural Layer to Mask** | COP | Meta SAM2 segmentation — coordinate prompts → clean masks. |
| **NCA Core / NCA Decode** | COP | Neural Cellular Automata — grow procedural organic patterns from a seed; trainable in-COP. |
| **ML Computer Vision Inference** | COP | Apply a trained detection / segmentation model to detect & mask objects. |
| **ML Train Neural Cellular Automata** | TOP | Train an NCA on your own reference imagery. |
| **ML Train / Preprocess Computer Vision** | TOP | Build a dataset and train object detection / segmentation. |
| **ML Train GSplats** | TOP | Train a 3D Gaussian-Splatting reconstruction. |
| **ONNX Inference · Neural Terrain Generate** | SOP | Geometry-side inference & learned erosion — adjacent, not image-context. |

---

## How it fits the practice

Ranked by how much it earns its place in *this* program. The honest read: Copernicus ML touches the
**image / plate / scan** side of montmartre, **not** the lichen sim. The lichen is an *element* — a 3D SCA
simulation forked from the biogrowth techno, going geo → MaterialX → Karma. None of these neural nodes belong
inside that element; they earn their keep on plates, scans, and comp.

### 1. Clean photogrammetry scan textures — **HIGH · first to deal with**
`ONNX Inference COP (generic)` · `Fill Holes / Poisson COP`

The live problem: baked scan textures (`assets/scan/<loc>/tex`) with holes, dirt, baked lighting, and soft
resolution. The generic ONNX runner does the heavy jobs — **LaMa** inpaint for large holes, **intrinsic
decomposition / delight** to recover clean albedo for MaterialX, **super-res** to recover detail — with
deterministic `Fill Holes`/`Poisson` for the small stuff. Full workflow, models, and gotchas in
[scan-texture-cleanup.md](../pipeline/scan-texture-cleanup.md).

### 2. One-click roto for plates & scan masks — **HIGH**
`Neural Layer to Mask (SAM2) COP`

Coordinate-prompt segmentation: click the zinc flashing, a stone course, the sky — get a clean mask. Feeds comp
mattes in Nuke, and provides the **dirt/occlusion mask** the scan-cleanup step needs — far more precise than
keying luminance for the black-pixel scan-hole problem.

### 3. Depth, normal & position from a single photo — **HIGH**
`Neural Layer to Depth (MoGe-2) COP`

**Metric-scale** is the operative word — ties straight into the camera-tracking scale / alignment work. Pull
depth mattes for z-based comp, proxy geo for projection, or a normal map from a façade photographed but not
scanned. Useful as a cheap sanity check on photogrammetry scale, and a fast relight reference for the AD hat
in Blender. Single-frame — treat sequences with care (see caveats).

### 4. Measure the hill over four seasons — **SITUATIONAL**
`ML Train + CV Inference COP/TOP`

Train a detector/segmenter on lichen *in the plates* (not the sim), run it across a re-shoot series, and you can
**quantify real-world coverage change** between episodes — turning "one year of the hill" into measured growth
data, and auto-generating masks at scale. Real setup cost (dataset labelling); worth it only if seasonal
comparison becomes a deliverable.

### 5. Splat the rooftop as a reference env — **SITUATIONAL**
`ML Train GSplats TOP`

Reconstruct the chimney rooftop from the shoot as Gaussian splats — a fast photoreal backdrop / previz
environment complementing the photogrammetry mesh. Fits the re-shootable proving-ground ethos. Reference and
layout only, not production geometry.

### — Not a fit: NCA for the lichen — **OFF-PROJECT**
`NCA Core / Decode COP` · `ML Train NCA TOP`

Tempting near a growth project, but **NCA is a 2D self-organising pattern generator, not a substitute for the
SCA lichen element** — the lichen is a simulation, not a neural texture. NCA is interesting in its own right
(see the explainer below); keep it in the *explore drawer* as a texture/motion toy or R&D curiosity, not part
of the lichen pipeline.

### — Tangential: learned weathering / erosion
`Neural Terrain Generate SOP`

Built for terrain, so off-topic as shipped — but the concept (a trained erosion operator) is conceptually
next-door to weathering stone. Park it; revisit only if weathering becomes its own R&D thread.

---

## NCA — what it actually is

[**Growing Neural Cellular Automata**](https://distill.pub/2020/growing-ca/) (Mordvintsev et al., Distill 2020).

Start from classic cellular automata: a grid of cells, each updating itself every step based only on its
**neighbours**, by a fixed rule (Conway's Life is the toy version). NCA throws out the hand-written rule and
**learns it as a tiny neural network** via gradient descent.

- Each cell holds a small **state vector** — RGBA *plus* hidden channels it uses as scratch memory.
- Every step: each cell **perceives** its neighbourhood (Sobel filters), runs that through the little NN, and
  nudges its own state. Stochastically, over thousands of steps.
- Trained so that from a **single seed cell**, the grid **grows into a target image**.

Why it feels biological:
- It's a differentiable model of **morphogenesis** — how one stem cell becomes an organism.
- **It regenerates.** Cut a hole in the grown pattern and it *heals back* — the rule is purely local and
  self-stabilising; nobody coded the repair, it emerges.
- It can be trained to **persist** (stop overgrowing) and be **isotropic** (rotation-free).

**In Copernicus (H22):** `NCA Core COP` runs the learned update and evolves cell states step by step;
`NCA Decode COP` turns hidden state into visible pixels; and a **training recipe** lets you train your own on a
target image, entirely in the COP graph — GPU, iterative, watch-it-grow.

**Where it sits for montmartre:** a self-organising 2D pattern/texture generator — an *explore-drawer* toy or
R&D curiosity. Evocative of organic growth, which is why it's tempting near a lichen project, but it is **not**
the SCA lichen element (see "Not a fit" above).

---

## Before you wire anything up

- **Execution provider (Windows):** DirectML works out of the box on any GPU with no install. The CUDA path is
  faster but needs NVIDIA's CUDA toolkit + cuDNN present. Leave on *Automatic* unless chasing throughput; set
  *Device ID* on multi-GPU.
- **Colour management — don't feed scene-linear:** most models expect display-referred sRGB in `[0,1]`. Convert
  out of ACEScg before inference and back after (*Brightness Multiplier* 1/255 handles the 0–255 side). Feeding
  raw scene-linear in is the classic "why does the mask look wrong" bug.
- **Temporal coherence:** SAM2, MoGe-2 and image inpainters are per-frame — they flicker across a sequence unless
  stabilised or conditioned. Fine for stills and texture bakes; care on animated plates.
- **Inference ≠ magic fill:** for black-pixel scan holes specifically, deterministic `Fill Holes` / `Poisson`
  is still first choice. Reach for ML only when a hole is large enough to need *invented* structure, not smooth
  interpolation.

---

## Related docs

- [onnx-inference.md](onnx-inference.md) — deep reference on the generic ONNX Inference COP: node anatomy, constraints, full model catalog + licences
- [scan-texture-cleanup.md](../pipeline/scan-texture-cleanup.md) — the ONNX scan-cleaning workflow (fit #1)
- [realityscan.md](realityscan.md) — how the scans/textures are produced
- [camera-tracking.md](../pipeline/camera-tracking.md) — matchmove + photogrammetry scale (fit #3)

## Sources

- [What's new: Machine Learning (H22)](https://www.sidefx.com/docs/houdini/news/22/ml.html)
- [What's new: Copernicus (H22)](https://www.sidefx.com/docs/houdini/news/22/copernicus.html)
- [ONNX Inference COP](https://www.sidefx.com/docs/houdini/nodes/cop/onnx.html)
- [How to use ONNX Inference](https://www.sidefx.com/docs/houdini/copernicus/onnx_inference.html)
- [Copernicus](https://www.sidefx.com/docs/houdini/copernicus/index.html)
- [Fill COP (deterministic hole fill)](https://www.sidefx.com/docs/houdini/nodes/cop/fill.html)
- [Growing Neural Cellular Automata — Distill 2020](https://distill.pub/2020/growing-ca/)
