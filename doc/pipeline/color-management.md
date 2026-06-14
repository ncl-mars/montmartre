# Color Management — ACES / OCIO Pipeline

Color backbone shared across **Houdini (Karma) → Nuke → DaVinci Resolve**. Built as a reusable studio
template: forked per project and kept identical across episodes and artists.

**Status:** ACES framework adopted — **ACES 2.0** (locked). Companion docs:
[film-emulation.md](film-emulation.md), [camera-tracking.md](camera-tracking.md),
[lookdev-render.md](lookdev-render.md).

---

## 1. Roles of the ACES spaces

ACEScg and ACEScct are not interchangeable options for one slot — they serve different roles:

| Space | Type | Role |
|---|---|---|
| **ACEScg** (AP1) | scene-**linear** | **Render + composite** working space |
| **ACEScct** (log) | log | **Grading** space (Resolve) |
| **ACES2065-1** (AP0) | linear, widest gamut | **Archival / master interchange** (delivery/archive, not working) |

**Rule:** data stays **linear (ACEScg EXR)** from render through comp; **ACEScct (log)** is used only at
the grade; **ACES2065-1** only for archival/master interchange.

**Rationale for a linear render→comp interchange:**
- Compositing operations (merge/over, blur, defocus, light-wrap, grain, filtering) are physically correct
  only in **linear light**; log breaks this.
- EXR is half-float and stores linear HDR without loss. Log encoding saves bits only in integer formats and
  offers no benefit for float EXR — encoding passes to log for interchange is lossy and unnecessary.

---

## 2. Per-stage spaces

| Stage | Working space | Stored / interchanged as |
|---|---|---|
| Karma render | **ACEScg** | scene-linear multichannel EXR (AOVs / cryptomatte) |
| Digital plate input | camera log/gamut → **vendor IDT** → ACEScg | standard ACES IDT for the body used (§4) |
| Nuke comp | **ACEScg** (linear) | **linear EXR** — the shared scene format Houdini↔Nuke |
| Resolve grade | **ACEScct** (log), ACES-managed timeline | — |
| Display / output | **ACES Output Transform** → Rec.709 / P3 per deliverable | — |
| Master / archive | **ACES2065-1** | only when an ACES master is required |

Shared scene/interchange format: **ACEScg scene-linear EXR** (render → comp). Log is grading-only. Render
passes are not stored in log.

---

## 3. Shared OCIO config

A single OCIO config is committed to the project and referenced via `$OCIO` in **Houdini, Nuke, and
Resolve**, guaranteeing identical color across applications, artists, and episodes.

- Official **ACES OCIO config** (v2.x for ACES 2.0 — §5).
- **Version-pinned**: the exact config is committed; no application uses its built-in default.
- **Studio template**: forked per project; the per-project copy is canonical for that show.

---

## 4. Digital plate input transform

Digital cameras have **standard ACES IDTs**, so plate input is straightforward. To bring plates into ACEScg:

- Identify the **camera body + recording color space** actually shot. Here: **Sony A7 II, 8-bit S-Log2 /
  S-Gamut** video — use the matching **Sony S-Log2 / S-Gamut ACES IDT** (see
  [camera-tracking.md](camera-tracking.md)). RAW stills (photogrammetry) carry their own profile.
- Assign the matching **vendor ACES IDT** → ACEScg. If a shot is captured outside a supported log/raw
  profile, characterize and build an input transform for it.
- Plates enter at **Nuke comp**, not Houdini — Houdini renders CG passes only.

---

## 5. ACES 2.0

The project is on **ACES 2.0** (locked). ACES 2.0 changes the **Output Transforms** (unified tone-mapping;
improved highlight, saturation, and gamut handling); the working spaces (ACEScg / ACEScct / ACES2065-1) are
unchanged. It requires the ACES 2.0 Output Transforms and the matching **v2 OCIO config**.

- Keep **Houdini, Nuke, and Resolve on versions that support ACES 2.0** (output transforms + v2 config), so
  the look is identical across all three. Don't mix 1.x and 2.0 output transforms — that produces
  inconsistent looks.

---

## 6. Handoffs & ordering

- **Houdini → Nuke:** linear **ACEScg EXR** (AOVs preserved).
- **Grain / film-emulation:** performed in **Nuke, in linear**, before the Resolve grade, so CG and plate
  share one grain ([film-emulation.md](film-emulation.md)). A small final-grain in Resolve is available if
  grading shifts grain.
- **Nuke → Resolve:** final comps as **ACES EXR** (ACEScg or ACES2065-1) for conform; a log/ProRes proxy
  for offline editorial. Resolve grades in **ACEScct** on the ACES timeline.
- **Resolve → delivery:** ACES Output Transform to the deliverable spec (Rec.709 web master / archive).
- **PATINA:** solo finishing — in-house **edit + cut + grade** (Resolve, ACEScct), no external hand-off.

---

## 7. Decisions to lock

- [x] ACES version: **2.0** (locked)
- [ ] Houdini / Nuke / Resolve on versions supporting ACES 2.0 output transforms + v2 OCIO config
- [ ] Exact OCIO config version, committed + `$OCIO` set on all stations/apps
- [ ] Camera IDT confirmed for the body + recording format actually shot
- [ ] Delivery output transform(s) confirmed (Rec.709 / P3 / resolution)
- [ ] Nuke→Resolve handoff format (ACES EXR finals; proxy codec for offline)
