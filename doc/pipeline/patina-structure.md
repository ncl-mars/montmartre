# PATINA — Project Structure (instance)

The concrete **instance** of the studio template. For the generic pattern, the two-layer model, and the
template-fork mechanism, see [project-structure.md](project-structure.md). This document shows PATINA's
actual content and its customizations.

## Structure

```
PATINA/   ── speculative growth-VFX R&D — Paris rooftop plates, CG/VFX elements ──
│
│   ┌─ inherited from the studio template (tooling) ─────────────────────────┐
├── hou/scripts/{ocl, vex}        Houdini OpenCL / VEX libraries
├── nuke/scripts/{blink, py}      Nuke BlinkScript + Python
├── config/ocio/                  ACES / OCIO config (ACES 2.0, v2 config)
├── doc/tech/biogrowth/           BioGrowth studio techno (lichen forked from it)
│   └──────────────────────────────────────────────────────────────────────┘
│
│   ┌─ PATINA production content ──────────────────────────────────────────────┐
├── elements/
│   └── lichen/                   hero element — multi-species fruticose lichen (fork of BioGrowth)
│       ├── hou/                  multi-species SCA sim — lichen_v###.hip + cache
│       ├── blender/              mood / look-intent in Cycles (ref for Houdini MaterialX) + ref modeling
│       └── ref/  tex/            species reference (Usnea / Cladonia), textures
│
├── shots/                        EP## / SH### — Paris rooftop plates (A7R V S-Log3; A7 II S-Log2 legacy)
│   └── EP01/SH010/
│       ├── plate/  ref/          camera-original footage, HDRIs, photogrammetry (chimney stills), lens grid
│       ├── track/  cam/          Nuke track → solved camera (.abc/.usd) + STMap
│       ├── hou/                  lichen-on-chimney assembly + Karma XPU render
│       ├── comp/                 Nuke comp (film-emulation: grain / halation / MTF)
│       └── render/               Karma EXRs (ACES)
│
├── edit/                         editorial + cut (Resolve)
│   ├── project/                  Resolve .drp project backups (cut)
│   ├── exchange/                 AAF / XML / EDL (Resolve ↔ Nuke)
│   ├── audio/                    music / sound for the cut
│   └── cache/                    Resolve cache / optimized media   [heavy]
├── grade/                        color grade — in-house (Resolve, ACEScct)
│   ├── project/                  Resolve .drp grade backups
│   ├── powergrades/              PowerGrades / .drx stills / looks
│   └── luts/                     LUTs / CDLs
├── deliver/
│   ├── web/                      web / showreel master (Rec.709 SDR)
│   └── archive/                  ACES2065-1 master
│   └──────────────────────────────────────────────────────────────────────┘
│
├── doc/                          project documentation (lichen/, pipeline/, reference/, tech/)
└── .claude/   .gitignore
```

## What PATINA fills in / customizes

- **Element:** the lichen element — a fork of the BioGrowth studio techno → `elements/lichen/`.
  3–5 species, inter-species interaction, massive coverage. See [../lichen/todo.md](../lichen/todo.md).
- **Plates:** digital capture, **Paris rooftops** — urban zinc/stone surfaces, hero subject the chimneys
  outside the flat (strong texture/detail for the solve and for photogrammetry). Primary camera: **Sony A7R V**
  (10-bit S-Log3 / S-Gamut3, up to 8K, MP4) on a **DJI RS gimbal**; **A7 II** (8-bit S-Log2) legacy. 61 MP RAW
  stills for chimney photogrammetry. See [camera-tracking.md](camera-tracking.md), [film-emulation.md](film-emulation.md).
- **Render:** Karma XPU; shading authored in **Houdini MaterialX** (Blender/Cycles = mood / look-intent
  reference on the AD side, not production materials). See [lookdev-render.md](lookdev-render.md).
- **Finishing:** in-house **edit + cut + grade** (Resolve, ACEScct) — solo, no external hand-off.
- **Delivery:** web / showreel master + archive master. See [delivery-spec.md](delivery-spec.md).
- **Colour:** ACES via a shared OCIO config. See [color-management.md](color-management.md).
- **Later:** extract the generic growth-VFX tooling back into a studio template (`d:\FLAT\_studio`).
