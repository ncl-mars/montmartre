# RealityCapture — Photogrammetry Reference

Reconstructing montmartre rooftop/street surfaces (colonized chimney/wall geometry) from images, to drive the
lichen sim's `surface` VDB constraint in solved-camera space. Inputs: **frames developed from the ProRes RAW
hero footage** (solo default; the XAVC S-I 4K S-Log3 safety stream is the fallback) + **dedicated 61 MP RAW
stills** (planned, premium) — RC mixes both, as long as color is consistent.

## Capture (for parallax)
- Photogrammetry needs **translation / parallax** — *move around* the subject. A **pano** (fixed nodal point)
  gives **no 3D**; gimbal panos are HDRI/reference only. The gimbal just stabilizes; *you* provide the parallax.
- Solo: **intervalometer** (A7R V built-in, ~1–2 s) while walking = controlled overlap, sharp, less redundant
  than burst — or **rafale**. Either way: **fast shutter ≥ 1/500**, **manual exposure** (fixed/consistent),
  base ISO 100, **RAW**. Overlap 70–80%, many angles + height.
- **Picture Profile / Creative Look do NOT affect RAW** (metadata + JPEG preview/histogram only) — develop RAW
  to sRGB in post. The **ProRes RAW** hero video is likewise developed (no baked curve); **S-Log3 only bakes
  into the XAVC S-I safety video**, so *that* leg needs the develop/transform.

## Image input
Export as **8-bit sRGB, full-range, no clipping**, every source through the *same* transform — see
[camera-tracking.md](../pipeline/camera-tracking.md) §7 (Resolve route) or develop in Lightroom (below).

## Image develop (Lightroom)
**Target, not identical sliders:** every frame → **neutral, no clipping, texture visible everywhere**. RC's
feature matching is **brightness/contrast-robust** (normalized gradients + NCC) so it *tolerates* exposure
variation between frames — but a **clipped highlight / crushed shadow = no data = hole** in the mesh. Clipping
hurts precision far more than a tone difference.
- **Prefer ONE very flat develop** (Highlights pulled hard, Shadows lifted, Contrast 0, Clarity 0) synced to
  all → best photometric consistency (best for multi-view stereo). RAW has the latitude to hold sun + shade in
  one flat develop.
- **Partition by lighting bucket only where the flat develop still clips/crushes** — recover those, converge
  every bucket to the same neutral output. Don't partition unnecessarily (adds photometric inconsistency).
- **Lightroom recipe:** Adobe Neutral profile; **CA + vignetting ON**; **geometric distortion OFF** (RC solves
  it); Sharpening / Clarity / Texture / Dehaze **0**; minimal NR (don't smooth the stone/plaster grain — it's
  the subject); **no crop / transform / upright**. Develop one → **Sync** (Classic) or **Copy/Paste edit
  settings** (cloud) to all. **Cull soft frames** (reject flag).
- **Export:** full size (no resize), **sRGB**, JPEG q90 or 16-bit TIFF, **keep EXIF** (RC reads focal length).
- Lightroom **Classic** beats cloud for a big batch (local, export control, no forced upload).

## Reconstruction quality (high detail)
Quality is dominated by **input + alignment**, then settings, in this order:
- **Normal model → Image downscale = `1`** (full res) — the biggest single detail lever (`2` = half).
- Maximal depth-map pixels = `0` (unlimited).
- **Detail decimation factor = `1.0`** (no decimation).
- **Smoothing ~`1.0`** (down from 1.5) keeps micro-relief — don't drop to 0 on low-texture or noise stays.
- **Maximal vertex count per part** — raise from 5M if the mesh caps and RAM allows.
- Mesh filtration **Filter strength** — lower = more detail retained.
- **Low-texture factors** (the white plaster / smooth zinc reconstruct worst): `Low-texture noise factor` (2.0)
  suppresses noise on flat metal — keep/raise for **zinc**, lower for textured **stone** to preserve grain.
- GPU acceleration on.
- Real ceiling = **input**: 61 MP stills ≫ 8-bit video frames; high **overlap** (70–80%), many angles, even
  light; set a tight **reconstruction region** so the detail budget goes to the subject, not the background.

## Reconstruction strategy — region, parts, simplify
- **Reconstruction Region (the box) = mesh only what you need.** Don't mesh the whole scene. Shrink / move /
  rotate the region box around the target surface → reconstruct inside only. Far smaller mesh, detail
  concentrated, fast. Alignment is unaffected (no re-align). Multiple surfaces → move the box, reconstruct each.
- **Reading the model list (tri counts tell you what each is):** a huge multi-part **"large"** model (e.g.
  *563 M tris, 258 parts*) is the **high-detail** mesh — split at the `Maximal vertex count per part` (5 M)
  limit; ~**1 M-tri singletons** are **preview** meshes; a model that shows "only the cams" is **empty**.
  Select the right model and **make it active** to view it (heavy meshes are LOD'd — give the viewport a beat).
- **Simplify before texture / export / Houdini.** A full-detail mesh (hundreds of M tris) is unusable
  downstream — Reconstruction → **Simplify Tool** to ~5–20 M first. A sim *surface constraint* doesn't need
  full density. Reconstructing a **region** is cleaner than simplifying the monster.

## LiDAR scans section — ignore
Image-only project. The classification filters (Ground / Vegetation / Building / Wire…) apply only to
*imported* LiDAR / laser-scan clouds. None of it is used for photogrammetry.

## Texturing
Run **Texture** (after the mesh) to unwrap UVs + bake the photo texture. Confirm the model actually shows the
texture in the 3D view before exporting.

## Export — gotchas
- **License-gated:** must be **signed into Epic** inside RC (RC 1.4+ from the Epic Launcher; older PPI builds
  need credits). Everything up to export is free; the write is what's gated.
- In this setup **only Alembic (`.abc`) exports cleanly** — OBJ/USD failed (RC's USD texture export is
  historically flaky).
- Alembic carries **mesh + UVs but NOT the material/texture link** — the texture writes as a **separate image**
  alongside the `.abc`. Reconnect it to base-color in the Houdini MaterialX shader; it maps via the imported UVs.
- Select the **textured model** in the scene list before export; use a simple local path (avoid OneDrive /
  long / accented paths).

## Coordinate & scale, interchange
- Scale the scan to real-world from **one on-site measurement** so the mesh is the **ruler** for the camera
  solve; then align the camera to it — see [camera-tracking.md](../pipeline/camera-tracking.md) §3, §5.
- Point-cloud interchange: prefer **`.abc`** or **`.ply`** over `.xyz` (`.xyz` has no header/standard, Nuke
  can't read it directly, ASCII is slow on big clouds).
