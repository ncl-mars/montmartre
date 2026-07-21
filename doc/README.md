# montmartre — Documentation

Documentation for **montmartre** — a speculative, unpaid studio R&D program, **place-anchored in montmartre
(Paris)**: all footage is shot next door (Sony A7R V — 12-bit ProRes RAW via Atomos Ninja V+, on a DJI RS gimbal; A7 II/S-Log2
legacy), so plates are cheap and re-shootable — an ideal pipeline-dev proving ground. Bounded to **4 episodes =
4 seasons** of the hill; the reusable tech + pipeline promotes to `_studio` as it proves out.
**Episode 1 (summer)** is lichen/growth VFX on the **zinc/stone rooftop chimneys** outside my flat, rendered in
Karma (a Houdini sim derived from an imported BioGrowth base). The `pipeline/` and `reference/` docs apply
program-wide; `lichen/` (Episode 1) and `tech/` are episode/tech-specific.

## [lichen/](lichen/) — the lichen VFX element · Episode 1 (summer) (in progress)
The dedicated **fork** of BioGrowth into a multi-species lichen simulation system — the hero element of
**Episode 1 (summer)**. ~1-month effort, 3–5 species, inter-species interaction, massive coverage.
- [todo.md](lichen/todo.md) — build plan + staged TODO

## [tech/](tech/) — in-house technologies
Technologies developed for the project.

### [tech/biogrowth/](tech/biogrowth/) — Space Colonization growth solver (donor tech for the lichen element)
The GPU/OpenCL SCA solver and its biological rules; the base the lichen element was forked from.
- [biogrowth-plan.md](tech/biogrowth/biogrowth-plan.md) — physics/architecture plan
- [network.md](tech/biogrowth/network.md) — SOP network, kernels, attribute schema
- [biological-rules.md](tech/biogrowth/biological-rules.md) — Murray, vigor, dominance, etc.
- [todo.md](tech/biogrowth/todo.md) — landing log + pending work
- [archive/](tech/biogrowth/archive/) — per-feature historical plan docs

## [pipeline/](pipeline/) — production workflow
Project-specific workflow, shoot → delivery. Evolves per shot.
- [project-structure.md](pipeline/project-structure.md) — canonical studio template layout + template-fork model
- [montmartre-structure.md](pipeline/montmartre-structure.md) — the montmartre project instance (filled-in graph)
- [camera-tracking.md](pipeline/camera-tracking.md) — matchmove of the digital plates + chimney photogrammetry
- [scan-texture-cleanup.md](pipeline/scan-texture-cleanup.md) — cleaning photogrammetry scan textures (Copernicus + ONNX: LaMa inpaint, delight, super-res)
- [onnx-tiling.md](pipeline/onnx-tiling.md) — tiling fixed-input ONNX models (512 LaMa) across large textures: overlap, feather, valid-crop
- [film-emulation.md](pipeline/film-emulation.md) — filmic look on digital footage + CG (grain, halation, MTF…)
- [lookdev-render.md](pipeline/lookdev-render.md) — shading in Houdini MaterialX → Karma XPU (Blender/Cycles = mood reference)
- [color-management.md](pipeline/color-management.md) — ACES/OCIO across Houdini→Nuke→Resolve (shared config)
- [delivery-spec.md](pipeline/delivery-spec.md) — delivery/mastering for montmartre: web/showreel; Resolve log grading
- [production-tracking.md](pipeline/production-tracking.md) — Kitsu evaluation for FLAT (production tracking; deferred)

## [reference/](reference/) — tool & language facts
Static, reusable reference for the tools. Canonical.
- [nuke-shortcuts.md](reference/nuke-shortcuts.md) — Nuke keyboard reference
- [nuke-dev.md](reference/nuke-dev.md) — Nuke development notes
- [blinkscript-specs.md](reference/blinkscript-specs.md) — BlinkScript language spec + gotchas
- [resources.md](reference/resources.md) — capture/technique reference (HDRI capture, …)
- [realityscan.md](reference/realityscan.md) — RealityScan (formerly RealityCapture) photogrammetry: quality settings, Alembic export, gotchas
- [ml-copernicus.md](reference/ml-copernicus.md) — ML in Copernicus (H22): the ONNX/neural node surface + how it fits the practice
- [onnx-inference.md](reference/onnx-inference.md) — ONNX Inference COP reference paper: node anatomy, constraints, task/model catalog + licences
