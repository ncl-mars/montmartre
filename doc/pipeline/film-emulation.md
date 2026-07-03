# Film Emulation — Filmic Look on Digital Footage + CG

MONTMARTRE is shot **digital**, but the target look is **filmic**. So the Karma lichen renders *and* the digital
plate both get a film emulation applied — the CG reads as *incrusté* (genuinely part of the image) rather
than pasted on top, and the whole frame reads as film rather than clean digital. Companion to
[camera-tracking.md](camera-tracking.md). BlinkScript implementation follows
[blinkscript-specs.md](../reference/blinkscript-specs.md).

> **One look over the whole frame.** Neither the plate nor the CG carries any film character on its own —
> so the emulation is a deliberate *look* targeting a chosen stock, applied uniformly to the whole
> composite. That keeps grain simple: one structure over everything (§4).

Grain is necessary but not sufficient. The look is sold by ~5 film characteristics applied in the right
order and color space. Blue noise is for dither, not grain.

---

## 1. The film characteristics that sell the look

Ordered by how film-identifying they are. Clean digital (plate and CG) has none of these — all must be added.

| Cue | What it is | Why it matters |
|---|---|---|
| **Halation** | Red/orange glow around highlights — light scatters off the film base and re-exposes the (red-sensitive) layer | One of the strongest "this is film" cues; bright lichen/sky edges need it |
| **MTF / softness** | Film + vintage optics resolve softer than a crisp digital render; resolution falls off toward high frequencies | A razor-sharp digital frame reads as un-filmic |
| **Stock color response** | The target stock's color rendering / tone curve / crosstalk | The whole frame must share one film color personality |
| **Base fog / black level** | Film blacks aren't 0 — there's base density | Pure-black digital shadows betray the look |
| **Grain** | Coarse, clumpy, per-layer, signal-dependent texture | The texture everything else lives inside |
| **Dither** (output) | Final anti-banding on 10-bit/log delivery | Prevents banding in gradients; the blue-noise job |

Plus the geometric/lens cues handled in the tracking doc: **lens distortion** (undistort/redistort) and
the camera solve.

---

## 2. Blue noise ≠ film grain

- **Blue noise** is high-frequency-weighted (energy in fine frequencies, little low-frequency content). It
  visually disappears and is ideal for **dithering** to kill banding.
- **Film grain** is the opposite spectrum: coarse and **clumpy**, with real low/mid-frequency energy — the
  clumping is the grain size. Coarse stocks are chunky; size the grain to the target stock.
- Grain driven by blue noise reads as too-fine, too-even "digital sand" (video, not film).

**Rule:** blue noise → **final dither pass only**. Grain comes from a proper grain model (below). White
noise is also wrong for grain (flat spectrum, no grain size) — grain must be **band-limited**.

---

## 3. Grain — fidelity hierarchy

The grain source is the *signature* of a chosen **target stock**:

1. **Measured grain (best).** A real grain sample of the target stock — a commercial grain library
   (LiveGrain / FilmBox / Dehancer-style). Its real spectrum + magnitude are extracted and applied — it
   *is* the stock, beating any procedural model.
2. **Procedural, done right.** LumaGrain (or Nuke's film-response grain) is battle-tested and a legitimate
   choice. A bespoke model must be:
   - **Per-channel** — 3 emulsion layers, different size/intensity per channel (blue layer is grainiest).
   - **Signal-dependent** — magnitude varies with density (most visible in mids/shadows for negative).
   - **Band-limited** — a real *grain size* (blur/spectrum), never raw per-pixel white noise.

---

## 4. Grain workflow — one structure over the whole frame

### Apply grain in LOG / density space, not scene-linear
Grain is roughly uniform in **density (log)** space, so applying it in log/Cineon yields the correct
"more visible in mids/shadows, less in highlights" behavior for free. Grain applied in scene-linear looks
wrong (over-grainy highlights). Convert → grain → convert back, or run the whole film-ize pass in log.

### CG and plate share ONE emulated grain
Both the plate and the CG start clean, so the route is simple: **composite the CG over the plate, then
apply one grain to the whole result.** No double grain, no matching two grain structures.

- **Optional: denoise the plate first.** If a high-ISO plate carries visible sensor noise, clean it before
  adding grain so you aren't stacking sensor noise *under* the emulated grain (which reads as
  muddy/dual-textured). Well-exposed plates skip this.

---

## 5. The BlinkScript "film-ize" kernel

Blink's role is the integration pass — a single GPU pass applying the film look to the composite,
parameterized per-shot, using the `.blib` library — not reinventing grain. Recommended pipeline order:

```
composite: CG (linear, undistorted) over plate
  → lens redistort (match the undistorted-space render to the plate)
  → match exposure / black level / base fog
  → stock color response (tone curve / crosstalk)
  → MTF softening (match film + optics resolution falloff)
  → halation (highlight extract → blur → tint red/orange → screen back)
  → [to log/density space]
  → grain (measured-spectrum or band-limited per-channel, signal-dependent) over the WHOLE frame
  → [output encode]
  → blue-noise dither
```

Build/Blink notes (per [blinkscript-specs.md](../reference/blinkscript-specs.md)):
- **Grain needs band-limiting** → either ranged access (`eAccessRanged2D` + `setRange`) to blur in-kernel,
  or generate fine noise and blur in a separate node. Pure `process()` hash noise is white — wrong spectrum.
- No scalar→vector broadcast, `f`-suffix literals, `lerp` (Nuke 16+), no overloading across `.blib` — reuse
  `utils.blib` / `noises.blib` / `remapping.blib` helpers.
- Implement a hash/PRNG for noise (no built-in random in Blink).
- Matching a **measured** grain spectrum is most faithful by sampling/FFT-matching the grain sample rather
  than hand-tuning a synthetic spectrum.

### Build vs. buy
- **Dither, halation, MTF, response:** strong candidates to own in the film-ize kernel — cheap, high control.
- **Grain:** prefer **measured grain** of the target stock; if procedural, a **LumaGrain-class** model is a
  legitimate shortcut. Build bespoke grain only for spectral control beyond those.

---

## 6. Checklist

- [ ] Target stock / look chosen; grain source decided (library or stock sample)
- [ ] Halation pass on highlights
- [ ] MTF/softness matched to the film + optics look
- [ ] Stock color response + base fog matched
- [ ] Digital plate denoised first only if noisy (avoid sensor noise under grain)
- [ ] Grain applied in **log** space, ONE structure over the whole composite
- [ ] Lens redistort to marry undistorted render to the plate
- [ ] Blue-noise dither as the final output step
