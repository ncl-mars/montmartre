# PATINA / "lichens" — Documentation

Documentation for **PATINA** — a speculative, unpaid studio R&D piece: lichen/growth VFX composited onto
my own digital footage (Sony A7 II, S-Log2/S-Gamut) of **Paris rooftop surfaces** — first test surface the
**zinc/stone chimneys** outside my flat — rendered in Karma. **Lichen is the current hero element** (a Houdini
sim derived from an imported BioGrowth base); other growth elements will follow and get their own areas. The
`pipeline/` and `reference/` docs apply project-wide; `lichen/` and `tech/` are element/tech-specific.

## [lichen/](lichen/) — the lichen VFX element (in progress)
The dedicated **fork** of BioGrowth into a multi-species lichen simulation system — the hero element of the
spec piece. ~1-month effort, 3–5 species, inter-species interaction, massive coverage.
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
- [patina-structure.md](pipeline/patina-structure.md) — the PATINA project instance (filled-in graph)
- [camera-tracking.md](pipeline/camera-tracking.md) — matchmove of the digital plates + chimney photogrammetry
- [film-emulation.md](pipeline/film-emulation.md) — filmic look on digital footage + CG (grain, halation, MTF…)
- [lookdev-render.md](pipeline/lookdev-render.md) — shading in Houdini MaterialX → Karma XPU (Blender/Cycles = mood reference)
- [color-management.md](pipeline/color-management.md) — ACES/OCIO across Houdini→Nuke→Resolve (shared config)
- [delivery-spec.md](pipeline/delivery-spec.md) — delivery/mastering for the spec piece: web/showreel; Resolve log grading
- [production-tracking.md](pipeline/production-tracking.md) — Kitsu evaluation for FLAT (production tracking; deferred)

## [reference/](reference/) — tool & language facts
Static, reusable reference for the tools. Canonical.
- [nuke-shortcuts.md](reference/nuke-shortcuts.md) — Nuke keyboard reference
- [nuke-dev.md](reference/nuke-dev.md) — Nuke development notes
- [blinkscript-specs.md](reference/blinkscript-specs.md) — BlinkScript language spec + gotchas
- [resources.md](reference/resources.md) — capture/technique reference (HDRI capture, …)
