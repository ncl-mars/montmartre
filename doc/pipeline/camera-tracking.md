# Camera Tracking in Nuke — Workflow & Reference

Project context: **montmartre Episode 1 (summer)** — lichens grown in Houdini (BioGrowth SCA) and composited
onto **digital** plates shot on **montmartre rooftops** (zinc/stone chimneys outside the flat — strong texture
and fine detail, good for the solve). The solved camera feeds the Houdini sim and the Nuke comp.

> **Camera.** Primary: a **Sony A7R V** (full-frame, 61 MP, E-mount) on a **DJI RS gimbal** — hero master
> **12-bit ProRes RAW** (Atomos Ninja V+) + a simultaneous internal **XAVC S-I 4K** S-Log3 safety stream;
> **61 MP RAW stills** for chimney photogrammetry. The gimbal
> gives **controlled, repeatable moves** — deliberate sideways arcs for strong **parallax** (a tight solve
> needs *translation*, not just rotation — the lesson from the early handheld grabs). Legacy body: **Sony A7 II**
> (8-bit S-Log2, 35.8 × 23.9 mm) + Zeiss primes 20/35/55 — older clips (e.g. SH010/C0028) were shot on it.
> Sensor size is a **known spec** read from metadata.

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

Prep is light: develop / conform, an optional denoise on the **tracking branch**, then lens undistort — the
comp returns onto the *original* plate.

```
camera-original
  │   develop / conform      (RAW → Resolve → ACEScg EXR;  XAVC / A7 II → IDT in Nuke)
  ▼
ACEScg plate
  ├─►  [denoise]* ─►  undistort (calib.io grid → STMap) ─►  SOLVE ─►  camera + point cloud
  │
  └─►  ORIGINAL (distorted) plate ─────────►  COMP  ◄─  CG
         (CG rendered in undistorted space, redistorted onto the plate via the kept STMap)

* denoise only when needed — high-ISO or the legacy A7 II (8-bit); the 12-bit ProRes RAW hero usually
  tracks as-is.
```

1. **Develop / conform.** The **ProRes RAW** hero can't be read by Nuke — it's **developed in Resolve →
   ACEScg EXR** first (that EXR becomes the tracking plate). The **XAVC S-I 4K** safety leg conforms via the
   stock **S-Log3 IDT** in Nuke; legacy **A7 II** via its custom S-Log2 IDT. See
   [color-management.md](color-management.md). Confirm fps and that no retime is applied.
2. **Denoise — only if needed, on the tracking copy.** Run it **before** undistort: the undistort warp
   resamples and *correlates* noise (worst at the edges), so clean it while the grain is still uniform
   sensor-space. The 12-bit RAW hero usually tracks as-is — reserve denoise for high-ISO or the 8-bit A7 II.
   Keep the **original** (un-denoised) plate for the comp.
3. **Watch rolling shutter.** CMOS sensors skew fast pans / vibration. Whip pans or rig vibration may need
   a rolling-shutter correction before the solve (or escalation, §6).
4. **Undistort** with `LensDistortion` (feed the calib.io grid). Output an **STMap** for redistorting the
   rendered CG back onto the plate at the end.
5. **Solve** on the **undistorted** plate. The comp later returns onto the **original** (still-distorted)
   plate, with CG redistorted via the kept STMap (§4–5).

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
6. **Orient & scale the scene (CameraTracker coordinate tools).** Select solved tracks in the Viewer,
   right-click:
   - **Set Ground Plane → Selected** — lasso several **cobble tracks** (more = flatter; lays them on Y=0).
   - **Set Origin → Selected** — one ground track becomes world zero.
   - **Set Distance** — select **exactly two** tracks, type a **real measurement** (convention: 1 unit = 1 m).
     No `solve_dist` math needed — RC computes the scale factor.

   A monocular solve is **scale-free** — you *must* inject one real distance, and **on-site tape beats any
   standard**. Pick two **sharp, far-apart, tracked** features (window-frame corners), never soft plaster or
   shutters. **Scale refs (montmartre village ≠ Haussmann — don't trust floor heights):** pavés ~**14 cm**,
   doors ~**2 m**, garde-corps ~**1 m**. Without scale the camera is unitless and the sim won't sit at real
   scale. Re-run **Create Camera / Point Cloud / Scene** after setting coords so outputs bake the new space.
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

### Shooting the grid (calib.io checker)

The grid is a large **calib.io checkerboard** (rigid, flat backing). Shooting it well is the whole game:

**Make-or-break**
- **In-camera lens correction OFF** — Menu → Lens Comp → **Distortion Comp = OFF** (shading / CA off too).
  You're calibrating the *true* optical distortion; if the body pre-corrects it, the STMap fights the plate.
  RAW stills ignore it, but video can bake it — verify.
- **Shoot the grid in the same recording mode as the hero plates** — same sensor crop / readout, not just the
  same lens. The ProRes RAW HDMI readout can differ in coverage from the internal 8K / XAVC; distortion is
  tied to the active sensor area, so a mismatched mode won't line up. Codec is irrelevant (geometry is
  codec-agnostic) — the **crop / format** is what must match.
- **Fill the frame — push the checker into all four corners.** Distortion lives at the edges; no board in the
  corners = nothing for `LensDistortion` to fit there. The big board lets you fill frame *and* keep working
  distance.

**Quality**
- **Flat + frontoparallel** — board perpendicular to the lens axis, sensor plane parallel to it. Tilt =
  perspective that pollutes the fit (tripod, level, centre it).
- **Per prime, prioritise the 20mm** (most distortion). Ideally at a focus distance near the real shots
  (focus breathing shifts distortion).
- **Tack-sharp corners, no glare** — f/5.6–8, ISO 100, flat even light, no specular hotspot (blown squares
  kill corner detection); tripod, no motion blur.
- **A few reframes** nudging the board through centre + each corner beat a single frame.
- **Per body** — A7 II legacy shots need their own grids (different sensor / format).

`LensDistortion` → **Grid Detect / Analysis** on the checker → fits the model → STMap. The known square size
isn't needed for Nuke's line-based fit (only for a *metric* calibration in OpenCV / 3DE).

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

**Aligning the RC scan to the solved camera.** Two independent solves start in different space + scale:
- **Scale-match first** — the RC mesh (real-scaled from one on-site measurement) is the ruler; then fix
  **axis / handedness** (RC up-axis ≠ Nuke Y-up), then rotate/translate to seat the point cloud on the mesh.
  Scale + orient the **parent** of the camera + cloud together (or use the CameraTracker coordinate tools, §3).
- **Validate by projection** — project the plate through the solved camera onto the mesh: the texture must
  **stick** across the *whole* move (parallax reveals depth errors a single frame hides). Sliding = scale,
  lens, or alignment error. Align in **undistorted** space with matching focal/aperture.
- **Low parallax** (mostly dolly + roll, little lateral move) is the hard case — don't grind Nuke; feed the
  **mesh as survey geometry to constrain the solve** (SynthEyes/3DE, §6). A small lateral arc on the next
  shoot beats a lot of dolly+roll.
- **Blender matchmove:** use the **undistorted** plate as background (the solved camera is pinhole) — redistort
  the CG in the Nuke comp (§4).

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

## 7. Photogrammetry image prep (RealityCapture)

**Two input sources** feed RC, sometimes together on the same chimney:
- **Extracted video frames** — the default solo workflow (now): frames pulled from the **developed ProRes RAW**
  hero footage (or the **XAVC S-I 4K** S-Log3 safety stream), exported from Resolve. Lower res than stills,
  watch motion blur / rolling shutter — pick sharp frames.
- **Dedicated on-site still shoot** — 61 MP RAW stills (planned, when not shooting alone). Higher quality.
- **Both combined** — RC happily mixes resolutions/sources, but they **must share the same color/exposure**
  so alignment and texturing stay consistent → run *every* source through the **same** transform below.

All inputs must be **8-bit sRGB, display-referred, full-range, no clipping** — RC matches features by
texture, so blown highlights / crushed shadows = lost geometry. Exported from Resolve (Deliver):

- **Format:** JPEG 8-bit, Quality **Best**; Force sizing + debayer to highest quality.
- **Color:** bake the `ACEScg → sRGB piecewise` transform (the ACES rolloff protects highlights — see
  [color-management.md](color-management.md) §6). Apply it as a node on **all** clips, then leave Deliver
  **Output Color Space / Gamma = "Same as Timeline"** (identity passthrough — do *not* also set Output to
  sRGB, that double-converts). Tag the file **Color Space = Rec.709, Gamma = sRGB**.
- **Data Levels = Full** (NOT Auto/Video — video levels crush to 16–235 and ruin the images). *Only matters
  for integer formats; the scene-linear EXR plate stays Auto/Full, no-op for float.*
- **Verify:** render one frame, compare to the Color viewer — must match. If highlights clip, pull exposure.

The reconstructed surface (point cloud / mesh) lands in solved-camera space to drive the sim's `surface`
VDB constraint (§ intro, §5).

## Checklist

- [ ] Camera/lens metadata verified (focal length, sensor size) — per body actually shot
- [ ] Lens distortion grid shot per prime (calib.io checker; in-camera distortion comp OFF; same recording mode as plates)
- [ ] At least one real-world measurement for scale
- [ ] Plate conformed → undistorted before solving
- [ ] Rolling shutter checked on fast pans / vibration
- [ ] Moving elements masked
- [ ] Solve error < 1px, bad tracks rejected, point cloud stable
- [ ] Ground plane + origin + scale set
- [ ] Camera exported (.abc) and lens matched in Houdini
- [ ] CG rendered undistorted; redistort + look (film-emulation) in comp
