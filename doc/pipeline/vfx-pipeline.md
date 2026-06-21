# VFX Pipeline — Plate → Track → Comp → Grade (pristine, end-to-end)

The clean working process for a PATINA shot, ACES throughout. **Not theory — the actual steps**, built and
verified on **EP01/SH010** (terracotta chimney pots, shot on the legacy **A7 II** — 8-bit S-Log2, 1080p, 25 fps).
**Primary camera going forward: Sony A7R V** (10-bit S-Log3/S-Gamut3, on a DJI RS gimbal), captured
**dual-stream** via an **Atomos Ninja V+** (on the RS accessory handle, off the gimbal balance): **ProRes RAW
12-bit is the default hero master**, with **XAVC S-I 4K** (S-Log3, All-Intra) recorded internally at the same
time as the safety leg. Two formats per take → two color entry points → one ACEScg pipeline (see
[color-management.md](color-management.md) §4). Each stage gets filled in as it's run.

---

## 1. Plate

The goal is identical regardless of source: land an **`ACEScg`, 16-bit half** EXR sequence at
`shots/<EP>/<SH>/plate/<SH>_plate_v###.####.exr` that everything downstream reads as `ACEScg` (IDT/decode
already baked). Only the *front* of the step differs by recording format.

**SH010 (legacy A7 II, S-Log2):**
- **Read** camera-original `footage/C0028.MP4` → colorspace **`S-Log2 S-Gamut`** (custom IDT built into the
  OCIO config — see [color-management.md](color-management.md)).
- Trim to the shot → **Write** the plate EXR (`ACEScg`, 16-bit half) → `shots/EP01/SH010/plate/SH010_plate_v001.####.exr` (300 frames).
- Read that EXR back (colorspace `ACEScg`, **not** S-Log2 — IDT already baked) for everything downstream.

**A7R V (going forward, dual-capture):**
- **Default — ProRes RAW leg → RAW develop (Resolve Studio).** The mosaic must be **decoded (debayered,
  Bayer→RGB) before anything can read it** — and that decode is the hard software constraint here: Nuke/Houdini
  can't decode ProRes RAW, so Resolve does it. The full stage is a **RAW develop / reconstruction**: decode +
  ISO/WB/exposure + color transform, on an ACES-managed timeline → **render the plate as `ACEScg` scene-linear
  EXR** to the path above. *This EXR is the plate* — comp/track read it as `ACEScg`,
  same as any other. (A `ProRes 4444` 12-bit conform is the only Nuke-native fallback if EXR isn't wanted.)
- **Safety — XAVC S-I leg:** read the internal `.mxf/.mp4` in Nuke directly → colorspace
  **`Sony S-Log3 S-Gamut3`** (stock ACES IDT) → Write the plate EXR (`ACEScg`). No Resolve step. Use this when
  a shot doesn't need the RAW finish, when comp must start before the RAW conform exists, or as the backup if
  the HDMI feed dropped.
- Either way, downstream reads `ACEScg` EXR — the rest of the pipeline (§2 on) is source-agnostic.

## 2. Camera track — Nuke CameraTracker

**Node recipe**
```
Read (plate EXR, ACEScg) → CameraTracker → Camera (Classic 3D, Link output ✔) → Scene → WriteGeo (.abc)
```
- Export → `shots/EP01/SH010/cam/SH010_cam_v001.abc` (git-tracked as small interchange).
- **Gotchas:** WriteGeo needs a **Scene** wrapper (it writes the scene's cameras, not a bare Camera); use
  **Classic 3D** (the new-3D camera won't plug into WriteGeo). A `Link output` camera exports fine — WriteGeo
  bakes the animation into the `.abc` at write time.

**Settings that matter (CameraTracker ▸ Settings)**
- **Focal Length: `Known` = 35** (Zeiss FE 35/2.8) — lock it; with weak parallax, leaving focal to solve kills it.
- **Film Back: `35.8 × 23.9`** (Mirrorless A7 preset). **Camera Motion: `Free Camera`.**
- **Number of Features** — ceiling, not a quota. **Detection Threshold** — *higher* = only strong features.
  **Refine Feature Locations ✔** — sub-pixel accuracy. **Minimum Length** — longer = stabler (drops flickery
  short tracks). **Feature Separation / Track Threshold** — distribution / per-track quality.

**The refinement loop**
1. **Track** (detect + follow) — *then* **Solve** (compute camera). Two separate steps.
2. **Prune high-error tracks:** Viewer markers colour green→red by error — **box-select the red ones, Delete**;
   or sort/threshold in the **AutoTracks** tab. Keep distribution spread (don't strip a whole region).
3. **Update Solve** (re-fits on the culled set without recomputing). Repeat to plateau.
4. **Clear+Track** after *settings* changes; **Update Solve** after *pruning*.

**Reading the solve**
- Per-track colour coding + the **Error** field (average) / per-frame error.
- Benchmarks: **< 1.0 px usable · < 0.5 good · < 0.3 excellent** — *and* the scrub-stick test (does CG stay glued).

**SH010 notes:** weak parallax (small handheld step) — but S-Log2 held shadow detail, so the dark pot interior
gave **valid tracked points**. **Masking the static shadow made it worse** (removed real features) → mask only
*movers / reflections / non-rigid* things, never detail-bearing static shadow.

### Point cloud → surface reconstruction (the most useful scene output)
The point cloud is the hub between the track and the actual CG — **export it, don't skip it.**

- **Export it:** CameraTracker → Export **`Point Cloud`** (or `Scene`) → WriteGeo → `.abc`, alongside the camera.
- **It's a 3D scaffold in solved-camera space** — anything modeled/**snapped to the cloud is locked to the
  plate through the camera automatically** = instant matchmove. A hand-placed object that only fits *one
  frame* **will drift**: a 2D fit doesn't pin depth — place geo *on the cloud*, not by eye.
- **QC the camera with it** (the definitive camera-vs-geo test): import the cloud next to the camera in
  Blender/Houdini and scrub —
  - points **lock to the plate** → camera transferred fine, solve is good → any drift is *your geo's depth*;
  - points **drift too** → the *camera transfer* is broken (fps / axis / scale / un-baked animation), not your geo.
- **Reconstruct the chimney surface** off the cloud: model-to-cloud, and/or **register the high-res RAW-stills
  photogrammetry to the cloud** to marry hero-res geometry to the live-action camera.
- That reconstructed surface **is the lichen substrate** — it drives the sim's `surface` VDB constraint
  (see [camera-tracking.md](camera-tracking.md)). **Chain: track → point cloud → reconstruct chimney geo → lichen surface.**

### Matchmove QC gotchas (hit on SH010)
- **`Unknown Lens` ⇒ pinhole camera matching the *undistorted* plate.** QC'd over the *original distorted*
  plate, expect **edge drift** — use an undistorted plate as backdrop (or redistort CG at output).
- **fps must match end-to-end** (footage 25 / project / `.abc` export / Blender all equal). The `.abc` camera
  is *time-based*; the plate EXR is *frame-based* (no fps). Mismatch = progressive drift.
- **Verify the camera actually animates** in the DCC (scrub 1–300) — a static import = WriteGeo didn't bake the move.
- **Confirm the film-back imported** (focal + sensor) — Alembic often drops it, leaving the DCC default → wrong FOV → "fits then drift."

## 3. Comp — Nuke
_TBD — fill in when run (CG-over-plate, film-emulation pass; see [film-emulation.md](film-emulation.md))._

## 4. Grade — DaVinci Resolve (ACEScct)
_TBD — fill in when run (see [color-management.md](color-management.md), [delivery-spec.md](delivery-spec.md))._
