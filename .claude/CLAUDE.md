# PATINA / "lichens" — Project Instructions

PATINA is a **speculative, unpaid studio R&D piece** — art-direction + tech research, no client. Use case:
lichen/growth VFX on my own **Sony A7 II** footage (8-bit **S-Log2 / S-Gamut**) of my **Paris** surroundings —
the first test surface is the **zinc/stone rooftop chimneys** outside my flat. RAW is stills only (good for
**photogrammetry** of the chimney surfaces). The lichen element is a Houdini SCA sim (forked from BioGrowth);
Nuke/Blink for comp, Karma for render, Resolve for finish. The plan is to build the spec piece first, then
extract a generic growth-VFX template into `d:\FLAT\_studio` later. The tech is forked unchanged from the UTMB
project (SCA/lichen sim, the OCL/VEX include library, the ACES pipeline, BioGrowth donor architecture).
Generic Houdini VEX & OpenCL rules live in the global `~/.claude/CLAUDE.md`; this file adds PATINA-specific
conventions. When the two conflict, this file wins.

## Ground rules
- **Do not run destructive or filesystem-changing commands** (delete/move/create files, scaffold folders,
  or any system-modifying shell command) **unless the user explicitly asks.** Propose, then wait for an
  explicit yes. The user manages their own files.
- **Propose once.** If declined or unanswered, drop it — do not re-ask.
- **No git commits/pushes** unless explicitly requested.

## Team & work split
Solo project — one person wearing both hats:
- **Lead / dev:** Houdini sim + OpenCL/VEX, Nuke BlinkScript, the lichen system, the pipeline, render
  (Karma MaterialX), comp, finish (Resolve), and these docs.
- **Art direction:** look-intent / mood — palette, lighting feel, the visual target — explored in **Blender
  Cycles** (visual direction, NOT production materials — final shading is authored in Houdini MaterialX) and
  any reference modeling in Blender. Same person; the AD hat sets the direction the Houdini shading matches.

Start at [doc/README.md](../doc/README.md).

## Layout
- `hou/lichens.hiplc` — main Houdini scene. Most code lives in node snippets (see global
  CLAUDE.md re: HoudiniExpressionEditor Temp-file workflow), not on disk.
- `hou/scripts/ocl/`
  - `include/*.cl` — the OpenCL include library (real `#include`d files). See conventions below.
  - `sop/*.cl` — tracking copies only; canonical source is the OCL node snippet (Temp file).
  - `hou_lib/` — clones of Houdini's own OpenCL libs (e.g. `sdf.h`). Do NOT edit; treat as read-only vendor code.
  - `refs/` — reference material.
- `hou/scripts/vex/{include,sop,cvex}` — VEX wrangles & headers.
- `nuke/scripts/blink/{include,nodes}` , `nuke/scripts/py` — Nuke BlinkScript + Python.
- `hou/geo`, `hou/backup`, `doc/` — cache, backups, notes.
- `.vscode/stubs/hou-stubs` — Houdini Python stubs for the IDE.

## OpenCL include library conventions (`hou/scripts/ocl/include/`)
- **Plain functions — no `inline`/`static`.** A kernel is one translation unit (includes
  concatenated once, guarded); the GPU backend inlines leaf functions anyway.
- **Include guards are `NAME_CL`** — no leading `__` (reserved for the compiler in OpenCL,
  e.g. `__kernel`, `__global`). Stems must be unique across the whole library.
- **`__attribute__((overloadable))`** for type-overloaded names (`frac`, `dot2`, `saturate`...).
- **`utils.cl` is the single home for shared math** (`dot2`, `ndot`, `frac`, `maxComp`...).
  Other files `#include "./utils.cl"` instead of redefining.
- **`hou_lib/sdf.h` clash:** it defines `__SDF_H__`, `ndot`, `length2`. `utils.cl`'s `ndot`
  is wrapped in `#ifndef __SDF_H__` so both can coexist. Keep new helpers from colliding with it.
- **Color is LINEAR** (Houdini `Cd`). `color.cl` helpers assume linear in/out; matplotlib
  colormaps (turbo/viridis/...) and tonemappers output display-referred — don't sRGB them twice.

## Conventions
- Prefer reusing existing include helpers over re-deriving math inline.
- When adding a new `include/*.cl`, give it a unique `NAME_CL` guard and `#include "./utils.cl"`
  for shared math rather than copying it.
