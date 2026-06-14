# Resources & Techniques

Reusable capture/technique reference for the studio (project-agnostic). Grows over time.

- [HDRI capture](#hdri-capture-image-based-lighting) — on-set lighting capture for IBL in Karma

---

## HDRI capture (image-based lighting)

Capturing the on-set lighting environment as a 32-bit float equirectangular (latlong) HDRI, so CG can be
lit to match the plate in Karma. Core kit: chrome + grey balls and a fisheye + nodal head.

### What is captured
- A **scene-referred, full-dynamic-range** 360×180 map of the light at the **CG's location** — from deep
  shadow up to (ideally) the sun.
- On-set **reference**: grey ball (diffuse light direction/color), chrome ball (reflections/highlight
  positions), and a ColorChecker (color/exposure calibration).

### Two methods

**A. Chrome-ball (fast, on-set standard, lower fidelity)**
1. Place a **mirror/chrome ball** where the CG will sit. Shoot from a few metres back with a **longer
   lens** (minimizes camera reflection + edge distortion).
2. **Exposure-bracket** the ball (lock aperture + ISO, vary **shutter only**) across the full range —
   deep shadow to highlight.
3. Shoot from **2–3 angles** to fill the blind spot behind the ball and reduce pole distortion.
4. Unwrap reflection → latlong (PTGui, or a Nuke spherical transform). Merge brackets → HDR.
- Pros: seconds to shoot, survives fast-changing light. Cons: lower res/quality, blind spot, distortion.

**B. Bracketed panorama (hero quality)**
1. Camera on a **panoramic head** at the **no-parallax (nodal) point**, with a **fisheye** (8–15mm) or wide.
2. Rotations covering full 360×180 (e.g. 3–6 around + zenith + nadir).
3. **Exposure-bracket every position** (wide range — sun→shadow).
4. **Stitch + HDR-merge** (PTGui / Hugin) → **32-bit float EXR equirectangular**, scene-linear (no tonecurve baked).
- Pros: highest res/DR. Cons: slow — risky when outdoor light is moving.

**C. 360 camera (Ricoh Theta etc.)** — fast latlong for scout/reference; bracket if supported. Limited
DR/res, not ideal for hero IBL.

### Process essentials
- **RAW, manual, fixed WB.** Bracket by **shutter only** so DoF/noise don't shift between exposures.
- **Bracketing is mandatory** — a single exposure is not HDR. Span shadow → sun.
- **The sun outruns any bracket.** Either a separate fast/**ND-filtered** sun frame, or clip the sun in-map
  and **re-add a calibrated sun** as a distant light (sun extraction — preferred for hero).
- **Absolute-exposure calibration** — a **grey card / known reflectance** and/or a meter reading, so the
  HDRI's light levels are physically meaningful, not arbitrary.
- **Color calibration** — ColorChecker in a reference frame; HDRI color/WB matched to the plate.
- **Nadir patch** — the tripod/rig shows at the bottom; a handheld nadir frame (rig moved) patches it.
- **Capture per setup, close in time to the plate.** Light is the variable being frozen; re-grab if it shifts.

### Outdoor / urban rooftop specifics
- **Light changes fast** (sun + moving cloud). The **chrome-ball or 360** are preferred for speed in moving
  light; the slow pano is reserved for stable light. **Timestamp** every grab and re-shoot per setup.
- **Wide DR** (bright sky vs shadowed zinc/stone) → wide brackets + ND for the sun are essential.
- Sky blow-out and a strong sky gradient affect ambient on the rooftop surfaces — capture the sky range
  honestly. Reflective **zinc** roofing throws specular highlights; the chrome ball and bracketing must hold
  those without clipping.

### Using it in Karma (verify against the Houdini version)
- Equirectangular EXR → **Dome Light** (latlong) for sky/ambient IBL.
- **Extract the sun** as a **Distant Light** with direction + intensity from the HDRI (sharper,
  controllable shadows than the clipped sun pixel).
- **Orient the dome** to the plate — align via chrome-ball reflections / known features / the tracked camera.
- **Match exposure** — dome/sun intensity set so a CG grey ball + ColorChecker match the on-set references.

### Gear & software
- **Buy (cheap, standing kit):** chrome ball + grey ball, ColorChecker, grey card, ND filters, nodal/pano head.
- **Use:** A7 + fisheye (or wide) for panos; 360 cam for speed/scout.
- **Software:** PTGui (stitch + HDR merge, industry standard) or Hugin (free); Lightroom for RAW; Nuke for
  cleanup / sun extraction / latlong ops; PTGui or Nuke for chrome-ball unwrap.

#### Motorized / robotic pano heads
Automate the bracketed sphere (rotate → fire brackets → advance), for repeatable, hands-off, drift-free panos.

- **Nodal Ninja Mecha (C1 / E2)** — programmable motorized rotator with bracketing; mid-priced; the sensible
  studio-kit choice.
- **Seitz Roundshot VR Drive** — high-end VFX/HDRI tool, very precise, auto-brackets; €€€€, only at volume.
- **GigaPan EPIC Pro** — classic robotic mount, gigapixel-oriented, older/prosumer.
- **Syrp Genie** — motion-control crossover (also timelapse moves), flexible but not purpose-built.

**Caveat — they solve precision, not moving light.** A full bracketed motorized sphere takes **minutes**;
in fast-changing outdoor light (drifting cloud/sun, e.g. a Paris rooftop) the environment drifts *during* capture,
so a robotic head can be **worse** than a quick chrome-ball grab. Motorized heads suit **stable light**
(studio/interior/steady overcast); the **chrome ball** remains the fast outdoor grab.

**Buy vs rent:** for occasional/niche use, **renting a high-end head for the occasion** (e.g. Seitz
Roundshot VR Drive) is preferable to buying. The **Nodal Ninja Mecha** is the buy option if a head becomes
standing kit. High-end motorized HDRI heads are a **niche rental** — availability must be verified with pro
photo/cinema rental houses (or shipped in).

Sony A7 II compatibility note: the Mecha mounts/triggers an A7 II via a **Sony Multi-Terminal shutter
cable** (the A7 II uses the Multi/Micro-USB port, not USB-C); confirm the exact cable on Nodal Ninja's
compatibility list. The A7 II's in-camera bracketing range is modest — plan multiple bracket sets for full
sun-to-shadow DR.

### On-set checklist
- [ ] HDRI captured at each CG location, per lighting setup
- [ ] Exposure-bracketed (shadow → sun), shutter-only, RAW
- [ ] Separate ND sun grab (or plan sun extraction)
- [ ] Grey ball + chrome ball + ColorChecker reference frames
- [ ] Grey card / meter reading for absolute-exposure calibration
- [ ] Nadir patch frame (rig moved)
- [ ] Timestamped; re-grabbed if light shifted between setups
- [ ] Merged to 32-bit float equirectangular EXR, scene-linear
