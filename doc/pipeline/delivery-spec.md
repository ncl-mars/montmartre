# Delivery & Mastering Spec — Spec Piece (web / showreel)

Output specification for **PATINA**, a self-published spec/R&D piece. No client, no installation — the
deliverables are a **web / showreel master** (portfolio, Vimeo/YouTube, festival/portfolio submissions)
plus an **archival master**. Mastered from the in-house **DaVinci Resolve** grade, downstream of
[color-management.md](color-management.md) (ACES/OCIO); grading happens in **ACEScct (log)**.

> Since there is no external installer or client, the targets below are **self-chosen**. Pick the showreel
> spec once and keep it stable so every shot conforms to the same canvas, fps, and color.

---

## 1. Targets

| Target | Purpose | Notes |
|---|---|---|
| **Web / showreel master** | Portfolio, Vimeo/YouTube, festival/portfolio submissions | Primary deliverable |
| **Archival master** | Future-proof source to re-derive any output | ACES2065-1 EXR sequence or ProRes 4444 |
| **Social cutdowns** (deferred) | 9:16 / 1:1 / 4:5 reframes | Derive from the same Resolve master if ever needed |

---

## 2. Master format

- **Resolution:** **UHD 3840×2160** (or 1920×1080 if footage/render budget favours it) — pick one and hold it.
- **Aspect:** **16:9** for web; a wider crop (e.g. 2.39:1 letterbox) is a creative choice, decide once.
- **Codec:** **ProRes 4444 / 422 HQ** (QuickTime) as the high-quality master; an EXR sequence for the
  archival master. Web platforms re-encode, so deliver a clean high-bitrate master and let them transcode.
- **Color space:** **Rec.709 / gamma 2.2** for the web master (via the ACES Rec.709 Output Transform).
- **Frame rate:** match capture (confirm **24 / 25**); no retime.
- **Audio:** stereo if scored; otherwise silent/ambient. Decide per cut.

---

## 3. DaVinci Resolve workflow

> Solo finishing: in-house **edit + cut + grade**, all in Resolve. No external hand-off.

### Project color management
- **Color science: ACEScct** (ACES with ACEScct as the working/grading space). ACES version: **2.0**
  (locked) — see [color-management.md](color-management.md).
- **Input:** Nuke comp finals delivered as **ACES EXR** (ACES2065-1 or ACEScg). Set the ACES **Input
  Transform** accordingly (no-op / ACES2065-1 for AP0 EXRs; ACEScg if delivered as AP1). Linear EXR carries
  the full scene-referred range into the grade.
- **Output (ODT):** **Rec.709 / gamma 2.2** for the web master; ACES2065-1 for the archival master.

### Grading in ACEScct (log)
- Grade operations run in the **ACEScct log working space**, where mid-gray sits ~0.41 and tonal controls
  behave predictably. The **viewer shows the image through the ODT** — grading is judged on the display look
  while the math stays in log.
- Recommended node order (per shot): **Offset** (global exposure-like balance, clean in log) → **Contrast +
  pivot** → **Color Wheels** (lift/gamma/gain by tonal range) → **secondaries / qualifiers / windows** →
  shot-matching. Keep the creative look on a master grade.
- **Scopes:** judge balance and legality on waveform / parade / vectorscope **through the ODT** (the
  delivered look), not the raw log. Avoid crushed blacks / clipped highlights.

### Master
- Grade **one master** in ACEScct (the creative look) through the **Rec.709 ODT**.
- Grading one master this way keeps any later outputs (social cutdowns, alternate ODTs) cheap to derive from
  the same source.

### Reference & QC
- Grade on a **calibrated Rec.709 reference monitor**.
- **QC on real playback paths** — check the web master on the actual platforms (Vimeo/YouTube re-encode and
  can shift gamma/saturation) and on a couple of typical viewing devices before calling it final.

### Deliver page
- Master resolution; master fps; ProRes for the web master / EXR for archive; Rec.709 tag; loop-ready if it
  loops. Consistent naming/versioning (`patina_showreel_version`).

---

## 4. Open items to confirm

- [ ] Showreel resolution + aspect locked (UHD vs HD; 16:9 vs wider)
- [ ] Capture/master frame rate (24 / 25) confirmed
- [ ] Web codec/bitrate for upload masters; archive format (ACES2065-1 EXR vs ProRes 4444)
- [ ] Audio: scored vs silent/ambient
- [ ] Whether social cutdowns are in scope for the first release

---

## 5. Checklist — showreel master

- [ ] Master resolution + correct fps
- [ ] ProRes master (web) + EXR/ProRes 4444 archive
- [ ] Graded in ACEScct through the Rec.709 ODT, QC'd on real playback paths
- [ ] Color space + range tagged
- [ ] Loop-ready in/out (if it loops)

---

## Deferred

- **Social-network deliverables** — out of scope for now. When revived, they derive from the same Resolve
  master (Rec.709 SDR ODT + reframes to 9:16 / 1:1 / 4:5).
