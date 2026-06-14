# Existing Houdini Tree/Growth Tools — Landscape

Survey of what exists as of early 2026, compared to our SOP Solver + OpenCL original-SCA approach.

---

## Commercial / Paid

### Natsura (Early Access, ~$200+)
https://www.natsura.com/
- **Authors:** George Hulm, Feike Postmes (ex-Square Enix)
- **Engine:** APEX graph-based evaluation — node grammar system defines growth rules, `Simulate` node evaluates deferred
- **Features:** botanical simulation (response to environment), parametric modeling, Effectors (gravity, light, collisions), Mappings (attribute-driven ramps over space/time), Cluster Decorator for game-ready foliage (millions of polys), UE5 Nanite export
- **Used on:** SideFX Project Elderwood tech demo
- **Algorithm:** not SCA — proprietary rule-based grammar + simulation on APEX
- **Roots:** unclear
- **GPU:** runs on APEX (multi-threaded, not GPU compute)
- **vs our approach:** the most polished production tool in this space. Different philosophy — grammar/rule-based rather than SCA. More art-direction oriented. No OpenCL, no explicit spatial competition. Likely the best tool for game-ready foliage, but a black box. Ours is lower-level, gives full algorithmic control, and targets offline rendering quality.

### Simple Tree Tools 2.7 ($45 Indie / $90 FX)
https://nwestfelt.gumroad.com/l/SimpleTreeTools
- **Author:** Nils Westfelt (nirfse)
- **Features:** fully procedural tree modeling, 4 types of tropisms, real-time wind + growth animation, pruning by density (kills branches in crowded growth direction), auto-branch mode, game dev workflows
- **Nodes:** Trunk Maker (multi-curve input, seed points), Branch Maker (chain levels), Leaf Scatter
- **Algorithm:** parametric branching (not SCA) — procedural rules per branch generation level
- **Roots:** not mentioned
- **GPU:** no (VEX/SOPs)
- **vs our approach:** fast for art-directed trees, tropisms are nice. But it's a parametric generator, not a growth simulation. No spatial competition, no emergent branching. Good for quick results, not for the organic self-organizing structure SCA produces.

### Houdini VEX Tree Generator ($15)
https://dyarkov.gumroad.com/l/vuDwh
- **Author:** D. Yarkov
- **Features:** POPs + VEX based, skeleton control for shape
- **Algorithm:** particle-based growth, not SCA
- **Minimal info available**

---

## Free / Open Source

### GGro (Beta, free non-commercial)
https://giogargiulo.gumroad.com/l/ggro
- **Author:** Gio Gargiulo
- **Features:** organic growth toolkit — multiple solvers in one HDA:
  - **Space Colonization Solver** (the one relevant to us)
  - Reaction Diffusion (mesh-based, unlike Houdini's built-in 2D version)
  - Coral Growth
  - Growth from guides / velocity fields
  - Linear growth on polylines
  - Adaptive remeshing with ID conservation
  - Self-collision, colliders
- **Houdini:** 21+ (most features work on older)
- **Algorithm:** includes SCA among other solvers
- **GPU:** no — mesh-based VEX
- **vs our approach:** the closest in scope. Has SCA but implemented in VEX on meshes, not OpenCL on points. Mesh-based workflow means it handles surface growth (coral, reaction-diffusion) well, but point-based SCA with GPU spatial hashing should be faster for large tree structures. Worth looking at for inspiration on the multi-solver architecture.

### TreeGen (open source, academic)
https://github.com/demiaster/treegen
- **Author:** demiaster (Bournemouth University MSc project)
- **Features:** HDA implementing Runions 2007 SCA directly. Control growth via root shape, foliage envelope, space-filling parameters. Parameters mapped to visual characteristics (silhouette, branching structure).
- **Includes:** .hda, VEX snippets, final report, example scenes
- **Algorithm:** original SCA (Runions 2007) — the same algorithm we're implementing
- **GPU:** no (VEX)
- **Roots:** supports root shaping
- **vs our approach:** closest to our algorithm. Same SCA, same Houdini context, but pure VEX — no GPU acceleration. Good reference for parameter mapping and art-direction controls. Our OpenCL version should be significantly faster for high attractor/node counts.

### Associated thesis: *Art-directing procedural vegetation in Houdini using a space colonization algorithm*
https://nccastaff.bournemouth.ac.uk/jmacey/MastersProject/MSc17/01/report.pdf
- Feriani, Bournemouth University — detailed write-up of the TreeGen implementation, art-direction strategies

### Bailey3D Tree Root Generator (open source)
https://bailey3d.com/2021/10/30/space_colonisation/
- **Features:** generates tree roots from input spawn positions, SCA-based
- **Algorithm:** space colonization for branch/root generation
- **Focus:** roots specifically
- **vs our approach:** root-specific SCA implementation, useful reference for underground volume setup and root-specific parameter tuning.

### JZTREES (open source)
https://github.com/joshuazt/JZTREES
- **Author:** Joshua Tilbrook
- **Features:** procedural tree generation + FX (wind, collision, fracturing). KineFX for wind/growth animation, Wire + Bullet sim for interactive trees. RBD fracturing support.
- **Requires:** Houdini 18.5+
- **Algorithm:** parametric (not SCA)
- **GPU:** no
- **vs our approach:** more of an FX/animation toolkit than a growth simulator. Interesting for the post-sim pipeline (wind, destruction) but the generation itself is parametric, not competition-based.

---

## Official SideFX

### Labs Tree Tools (free, ships with SideFX Labs)
https://www.sidefx.com/docs/houdini/nodes/sop/labs--quick_basic_tree.html
- **Features:** procedural branching over generations, branch count/resolution/bending/pruning, leaf generation, overall silhouette control. KineFX rig output for wind/growth animation.
- **Algorithm:** parametric branching per generation (not SCA)
- **GPU:** no
- **vs our approach:** quick and easy, good for background trees. Not a growth simulation — no spatial competition, no emergent structure. Parametric feel.

### Houdini L-System SOP (built-in)
- **Features:** Lindenmayer system string rewriting, turtle graphics
- **Limitations (for our use case):** regenerates entire structure each evaluation (no incremental growth), hard to animate without popping, no spatial awareness, string rules get unwieldy for organic structures
- **vs our approach:** fundamentally different paradigm. Good for mathematical/fractal patterns, poor for realistic tree growth simulation.

---

## Tutorials (not tools, but relevant implementations)

### Entagma — VEX Space Colonization
https://entagma.com/1028-2/
- Manuel implements Runions' SCA in VEX inside a SOP Solver
- Structure: Associate wrangle → Expand Tree wrangle → Remove Attractors wrangle
- Uses Houdini's `nearpoints()` / `pcfind()` for closest-point queries
- **Pure VEX, no GPU** — but the 3-wrangle decomposition (associate, expand, cull) mirrors our 3-kernel OCL pipeline
- Good pedagogical reference for understanding the SCA data flow in Houdini

### Bournemouth MSc — Art-directing procedural vegetation
https://nccastaff.bournemouth.ac.uk/jmacey/MastersProject/MSc17/01/report.pdf
- Full thesis on TreeGen implementation + art direction strategies
- Discusses parameter-to-visual mapping, envelope sculpting, pruning

---

## Summary: what exists vs what we're building

| Tool | Algorithm | GPU | Animated Growth | Roots | Original SCA |
|------|-----------|-----|-----------------|-------|-------------|
| **Natsura** | Grammar/rules (APEX) | APEX (CPU multi-thread) | yes | unclear | no |
| **Simple Tree Tools** | Parametric branching | no | yes | no | no |
| **GGro** | SCA + others (mesh-based) | no | yes | unclear | partial |
| **TreeGen** | SCA (Runions 2007) | no | yes (solver) | yes | yes |
| **Bailey3D** | SCA | no | unclear | yes (focus) | yes |
| **JZTREES** | Parametric + FX | no | yes (KineFX) | no | no |
| **Labs Tree** | Parametric generations | no | yes (KineFX) | no | no |
| **L-System SOP** | String rewriting | no | no (regenerates) | no | no |
| **Entagma tut** | SCA (VEX) | no | yes (solver) | no | yes |
| **Ours** | **SCA (Runions 2007)** | **OpenCL** | **yes (solver)** | **yes** | **yes** |

**Gap we're filling:** no existing Houdini tool implements the original SCA on GPU. The VEX implementations (TreeGen, Entagma, GGro) are CPU multi-threaded, which is fast — but OpenCL on GPU gives access to thousands of cores vs dozens of CPU threads, which matters when the inner loop is "each attractor queries a spatial grid of tree nodes" across 50K+ points growing every frame. Our OpenCL pipeline with spatial grid hashing targets that scale, with faithful SCA, animated growth in a SOP Solver, supporting both branches and roots.

More importantly: **no existing tool treats the growth simulation as the final render.** They all generate a static tree, optionally faking growth with carve/animation. Our solver produces actual frame-by-frame space colonization — the simulation IS the output.
