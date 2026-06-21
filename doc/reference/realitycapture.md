# RealityCapture — Photogrammetry Reference

Reconstructing Paris rooftop/street surfaces (colonized chimney/wall geometry) from images, to drive the
lichen sim's `surface` VDB constraint in solved-camera space. Inputs: **extracted S-Log3 video frames**
(solo default) + **dedicated 61 MP RAW stills** (planned) — RC mixes both, as long as color is consistent.

## Image input
Export from Resolve as **8-bit sRGB, full-range, no clipping**, every source through the *same* transform —
see [camera-tracking.md](../pipeline/camera-tracking.md) §7.

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
