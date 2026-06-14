# Camera Tracking in Nuke — Workflow & Reference

Project context: a short spec piece of lichens grown in Houdini (BioGrowth SCA) and composited onto
**digital** plates shot around **Paris rooftops** (zinc/stone chimneys outside the flat — strong texture
and fine detail, good for the solve). The solved camera feeds the Houdini sim and the Nuke comp.

> **Camera.** A **Sony A7 II** (full-frame, **35.8 × 23.9 mm** sensor, E-mount) with three **Zeiss primes
> — 20 / 35 / 55 mm**. Video is **8-bit S-Log2 / S-Gamut** (no internal RAW video); **RAW is stills only**,
> used for chimney photogrammetry. A bit old but a perfectly capable setup, and the primes are what matter.
> The sensor size is a **known spec** read from metadata.

> **What the track needs.** Footage carries **embedded metadata** (focal length, sensor, shutter, ISO,
> frame rate) and a known sensor, so the camera intrinsics are read, not reconstructed. The work is:
> lens-distortion grids, **3D reconstruction of the colonized chimney surfaces** (point cloud / modeled /
> photogrammetry from RAW stills), the solve itself, real-world scale, and export to Houdini.

> **Not just a camera track.** The lichens grow on the chimney/rooftop surfaces in the plate, so the sim
> needs the colonized **surface geometry reconstructed in 3D** (from the tracker point cloud, modeled to
> match, or **photogrammetry from RAW set stills**) — living in the solved camera space — to drive its
> `surface` VDB constraint. Set reference is planned around this: many RAW stills of the hero chimneys for
> photogrammetry.

> **Tool assessment.** Nuke's CameraTracker (NukeX toolset, in Nuke Indie) is good, not best. The
> high-end standard is **3DEqualizer**; **SynthEyes** is the value workhorse for supervised solves. For
> these textured rooftop/architectural plates Nuke is more than adequate — hard urban edges and chimney
> detail give strong features + parallax. Escalation to SynthEyes/3DE is reserved for shots that fight the
> solve (§6).

---

## 1. Standard plates & data for a matchmove

One set per shot. Names follow common industry convention.

| Plate / data | What it is | Purpose |
|---|---|---|
| **Source plate** | The camera-original footage (log or raw), full res | Master reference; the final comp returns onto this |
| **Tracking plate** | Conformed / transcoded version (linear or working space) | The plate the solve runs on — clean digital features |
| **Undistort plate** | Lens distortion removed (via STMap / LensDistortion) | Solve and place CG in **undistorted space**, redistort at output |
| **Lens grid** | Checkerboard/dot grid shot **per lens** on set | Computes the distortion model — the single most valuable on-set asset |
| **Reference stills / HDRI** | Set photos, witness-cam, 360° HDRI | Lighting, scale, set reconstruction, eyeline checks |
| **Camera & lens metadata** | Focal length, body/sensor size, T-stop, shutter, ISO, fps | **Embedded in digital files** — verify, don't reconstruct. Note the lens (20 / 35 / 55) per shot |
| **Survey / measurements** | Any known real-world distance or height | Scales + orients the solve to real units (critical for a sim that grows on the geo) |

Minimum for a usable solve: tracking plate, focal length (from metadata), and at least one real-world
measurement for scale. Everything else improves quality and integration.

---

## 2. Plate prep pipeline (before solving)

Prep is light: conform to the working space, then lens undistort.

```
camera-original → conform / linearize → lens undistort → SOLVE
                                              │
                              (keep distortion model / STMap
                               for the final comp)
```

1. **Conform / linearize.** Bring the camera-original (log or raw) into the working color space via the
   correct IDT (see [color-management.md](color-management.md)). Confirm fps and that no retime is applied.
2. **Watch rolling shutter.** CMOS sensors skew fast pans / vibration. Whip pans or rig vibration may need
   a rolling-shutter correction before the solve (or escalation, §6).
3. **Undistort** with `LensDistortion` (feed the lens grid). Output an **STMap** for redistorting the
   rendered CG back to match the plate at the end.
4. Solve on the **undistorted** plate.

A well-exposed plate tracks as-is; a light denoise on the *tracking* copy is only worth it on an unusually
noisy high-ISO shot (keep the original for the comp).

---

## 3. Nuke CameraTracker workflow

1. Input the prepped tracking plate. Set the frame range to the shot.
2. **CameraTracker → Settings:** enter **focal length** + **sensor size** **from the file metadata**
   (verify against the camera/lens used — A7 II is full-frame 35.8 × 23.9 mm). Lock known values; solve the
   rest. Set lens type (prime — the 20/35/55 are primes, not zooms).
3. **Mask moving elements** — roto people, traffic, birds, foliage, flags into the node's **mask input** so
   they don't pollute the solve.
4. **Track Features**, then **Solve**.
5. **Evaluate the solve:**
   - **Solve error** in pixels: `< 1.0` usable, `< 0.5` good, `< 0.3` excellent.
   - Inspect per-track error; **reject high-error and short-lived tracks**, then **re-solve**.
   - Scrub the **3D point cloud** — points should sit stably on the surfaces, not swim or balloon.
6. **Orient & scale the scene:** define the **ground plane**, set the **origin**, and set **scale** from
   a known measurement. Without this the camera is unitless and the growth sim won't sit at real scale.
7. **Create** Camera, PointCloud, and Scene from the solve.

---

## 4. Lens distortion

- Shoot a **distortion grid per lens** on set (ideally per focus distance) — the highest-ROI on-set asset.
  Shoot one grid per prime (20 / 35 / 55), per the body in use.
- `LensDistortion` analyses the grid → distortion model → **STMap**.
- Track + place CG in **undistorted** space; **redistort** the final CG render with the inverse STMap so
  it marries the original (still-distorted) plate.
- Without a grid, `LensDistortion` / CameraTracker can *estimate* distortion from footage, but a shot grid
  is more reliable. Wide primes (the 20mm) carry the most distortion, so prioritise their grids.

---

## 5. Export to Houdini (for the sim)

- Bake the solved camera to **Alembic (`.abc`)** (preferred) or **FBX**; `.chan` carries transforms only.
- Bring the **point cloud** across as the alignment reference for placing/growing geo on the surfaces.
- **Match the lens in Houdini:** focal length + aperture must match Nuke's. Mind Nuke's aperture/FOV
  conventions (horizontal aperture, separate vertical) vs Houdini's. The aperture is the **sensor width**
  of the body used — get it from metadata, not assumed.
- Render CG in **undistorted** space to match the undistort plate; **redistort** in the Nuke comp to land
  on the original plate (film-emulation / look is applied there too — see [film-emulation.md](film-emulation.md)).
- **Scale note:** thin-filament radii hit Murray/allometric underflow at sub-mm scale. The sim scene is
  scaled up for the solve and back down after (no kernel changes). See `doc/lichen/todo.md`.

---

## 6. Escalation to SynthEyes / 3DE

Escalate when a shot has:
- **Low parallax** — long-lens / telephoto shots with little camera translation.
- **Rolling-shutter skew** that a correction pass doesn't fully fix.
- **Mostly sky / featureless surfaces** with few stable features.
- A **hero shot** requiring a bulletproof solve.

(Zoom / lens breathing isn't a concern here — the 20/35/55 are fixed primes — but it would also escalate.)

Workflow: solve in **SynthEyes** (supervised tracking + lens tools) or **3DEqualizer** (industry standard),
then export the camera to Nuke/Houdini. The comp + sim pipeline downstream is identical — only the solve
moves out of Nuke.

---

## Checklist

- [ ] Camera/lens metadata verified (focal length, sensor size) — per body actually shot
- [ ] Lens distortion grid shot per prime (20 / 35 / 55)
- [ ] At least one real-world measurement for scale
- [ ] Plate conformed → undistorted before solving
- [ ] Rolling shutter checked on fast pans / vibration
- [ ] Moving elements masked
- [ ] Solve error < 1px, bad tracks rejected, point cloud stable
- [ ] Ground plane + origin + scale set
- [ ] Camera exported (.abc) and lens matched in Houdini
- [ ] CG rendered undistorted; redistort + look (film-emulation) in comp
