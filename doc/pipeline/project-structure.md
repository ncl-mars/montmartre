# Project Folder Structure — Studio Template (canonical)

The generic, reusable layout every project forks from. Two layers cohabit in one repo: the **studio
template** (generic tooling) and the **production content** (entity-based). A concrete filled-in example
is [patina-structure.md](patina-structure.md).

## Studio template → project (template-fork)

The studio pipeline is a generic, reusable **seed**; a project is a customization on top.

```
studio-template/      the canonical seed (its own repo/folder)
   libs · config · reference · technos · conventions · _template · .gitignore · CLAUDE rules
        │
        │  fork (copy) at project start
        ▼
<project>/            one project repo — BOTH layers cohabit
   ├─ [tooling]     a snapshot of the template (libraries, config, studio technos)
   └─ [production]  project-only (elements, shots, finishing, project docs)
```

- **One repo** to clone/pull — simple for non-developers.
- The template is a **separate seed**; each project is a **fork** of it. Inside the project the two
  layers cohabit, distinguished by the `[tooling]` / `[production]` blocks below — not folder walls.
- **Reconciliation (manual, by design):** generic improvements flow back to the template seed;
  project-only changes stay in the project. Otherwise the seed and the project copy drift.
- **Physical separation** (git submodule / shared install) is a later, larger-scale concern — it would
  break the one-click pull, so it is deferred.

## Structure

```
<project>/   ── production project (forked from the studio template) ──
│
│   ┌─ inherited from the studio template (tooling — app/discipline based) ─┐
├── hou/scripts/{ocl, vex}        Houdini OpenCL / VEX libraries
├── nuke/scripts/{blink, py}      Nuke BlinkScript + Python
├── config/ocio/                  ACES / OCIO config
├── doc/tech/<techno>/            studio technos (e.g. biogrowth)
│   └──────────────────────────────────────────────────────────────────────┘
│
│   ┌─ production content (ENTITY based) ─────────────────────────────────────┐
├── elements/                     reusable elements / assets
│   └── <element>/                an element (may fork a studio techno)
│       ├── hou/                  Houdini system — <element>_v###.hip + cache
│       ├── blender/              3D modeling + lookdev
│       └── ref/  tex/            element reference / textures
│
├── shots/                        (episode →) (sequence →) shot
│   └── <EP>/<SH>/                e.g. EP01/SH010
│       ├── plate/  ref/          plate, HDRIs, photogrammetry, lens grid
│       ├── track/  cam/          track → solved camera (.abc/.usd) + STMap
│       ├── hou/                  shot assembly: element + cam → render
│       ├── comp/                 Nuke comp
│       └── render/               render EXRs
│
├── edit/                         editorial + cut (Resolve)
│   ├── project/                  Resolve .drp project backups
│   ├── exchange/                 AAF / XML / EDL (Resolve ↔ Nuke conform)
│   ├── audio/                    music / sound for the cut
│   └── cache/                    Resolve cache / optimized media   [heavy]
├── grade/                        grade (Resolve) — in-house, OR hand-off to external colorist
│   ├── project/                  Resolve .drp grade backups (if in-house)
│   ├── powergrades/              PowerGrades / .drx stills / looks
│   ├── luts/                     LUTs / CDLs
│   ├── handoff/                  package to / from an external colorist
│   └── cache/                    Resolve cache   [heavy]
├── deliver/                      masters / deliverables
│   ├── <target>/                 per delivery target (e.g. web / showreel / broadcast)
│   └── archive/                  ACES2065-1 master
│   └──────────────────────────────────────────────────────────────────────┘
│
├── doc/                          project documentation
└── .claude/   .gitignore
```

## Two layers

- **Studio template (tooling — app/discipline based):** `hou/scripts`, `nuke/scripts`, `config/ocio`,
  `doc/tech/<techno>`. App-grouping is the correct convention here — it is pure tooling, no production
  data. App-at-top suits a tools layer; it does not scale to production content (next point).
- **Production (entity based):** `elements/`, `shots/`, `edit/`, `grade/`, `deliver/`. Organised by
  **entity** (element / shot), with the apps *inside* each entity — the production convention. A single
  shot's work lives together (its Houdini, comp, cache), rather than scattered across top-level app folders.

`elements/<element>` is a reusable **element system** (sim + modeling + lookdev). `shots/<EP>/<SH>` is the
**per-shot application** of an element with the shot's tracked camera. These are distinct.

## Finishing chain

```
shots/<EP>/<SH>/render/   (comp EXRs, ACES)
        +  edit/   (cut + AAF/XML/EDL exchange)
                 │
                 ▼
        grade/   (Resolve — in-house, OR hand off to an external colorist)
                 │
                 ▼
        deliver/   (masters per target)
```

- `edit/exchange/` holds the shareable **AAF / XML / EDL** timelines (Resolve ↔ Nuke) — small, versioned,
  the conform source of truth.
- `grade/` is the Resolve working area when grading in-house; for projects that outsource the grade, it
  is the hand-off area and the graded masters return into `deliver/`.

## Conventions

- **Elements:** `elements/<name>/{hou, blender, ref, tex}` — entity-first; apps live inside the element.
- **Shots:** `EP##/SH###`, numbered in tens (`SH010, SH020…`) for inserts; `EP02/…` scales a series.
  Optional `SEQ###` level if shots cluster into sequences. Copy `shots/_template/` to start a shot.
- **DCC saves:** incremental (`<name>_v001.hip, v002…`).
- **Colour:** `config/ocio/` is the single config root (see a project's color-management doc).
- **Studio technos** live in `doc/tech/<techno>/`; an element may **fork** a techno into `elements/`.

## Version control boundary

Heavy/regenerable data is excluded from git, primarily **by file extension globally** (`.exr .mov .bgeo
.vdb .abc .usd .tif .png …`) plus output/cache folders. Versioned: docs, scripts/libraries, `config/`,
`edit/exchange` (AAF/XML/EDL), shot scripts, and solved cameras.

If a heavy folder must live on fast storage / NAS, symlink it (e.g. `shots/.../render`) — the logical
layout is unchanged.
