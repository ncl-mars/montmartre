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
- **Committed file, not the app bundle**: point every app at the committed `config/ocio/*.ocio` (Nuke:
  *custom OCIO config* field or `$OCIO`), **not** an application's bundled config (e.g. Nuke's
  `fn-nuke_studio-config…`). The bundle is the *same ACES math* but is versioned with the **app install** →
  it drifts across collaborators / machines / app versions. The committed file is the single source of truth
  (verified equivalent: same roles — `scene_linear`, `compositing_log`, `matte_paint`, `texture_paint` — and
  the same ACES 2.0 views).
- **Studio template**: forked per project; the per-project copy is canonical for that show.

**Setting `$OCIO` in Houdini.** Houdini reads the config from the `OCIO` environment variable **at launch** —
there is no UI field to pick a config, and if `OCIO` is unset it falls back to its bundled config (don't — §3).
Scope it **per project via a launcher** so it follows the show, not the machine:

```bat
set JOB=D:\FLAT\SPEC\MONTMARTRE
set OCIO=%JOB%\config\ocio\<config>.ocio
"C:\Program Files\Side Effects Software\Houdini 22.0.xxx\bin\houdinifx.exe"
```

Ties `$OCIO` to `$JOB`, the same var Nuke and Resolve read. *Alternative:* a committed
`$JOB/packages/*.json` with `{ "env": [ { "OCIO": "$JOB/config/ocio/<config>.ocio" } ] }` keeps the setting in
the repo — but `OCIO` must exist before color-management init, so the launcher is the sure route. **Verify**
the active config path in the color-management preference reads the committed `config/ocio/…`, not
`…/houdini/ocio/`.

---

## 4. Digital plate input transform

Plates reach ACEScg by one of two routes depending on how the take was recorded. The **A7R V is captured
dual-stream** (Atomos Ninja V+ on the DJI RS accessory handle, off-axis — out of the gimbal balance):
**ProRes RAW 12-bit is the default master**; **XAVC S-I 4K** (10-bit 4:2:2 S-Log3, All-Intra) records
internally **simultaneously** as the safety leg. Same take, two formats, two color entry points — one shared
ACEScg working space downstream.

| Source (per take) | Role | Color entry → ACEScg | Needs Resolve? |
|---|---|---|---|
| **ProRes RAW** (A7R V, 16-bit RAW over HDMI) | **default hero master** | **Resolve Studio** RAW develop (debayer + ISO/WB/exposure + color transform) → **ACEScg EXR** | yes — RAW develop |
| **XAVC S-I 4K** (A7R V internal, S-Log3/S-Gamut3) | simultaneous safety / instant edit | **stock ACES `Sony S-Log3 S-Gamut3` IDT** (in the OCIO config) | no — IDT direct |
| **A7 II** (legacy, 8-bit S-Log2/S-Gamut) | legacy shots only | **custom-built S-Log2/S-Gamut IDT** (see [camera-tracking.md](camera-tracking.md)) | no — IDT direct |
| RAW stills (photogrammetry) | chimney surface | own profile (own decode) | — |

- **RAW bypasses the S-Log3 IDT.** ProRes RAW is undebayered sensor data, *not* S-Log3 video — its color
  entry is the **RAW develop** in Resolve (decode/debayer Bayer→RGB + WB/exposure + color transform), not the
  OCIO IDT. The decode is the hard software constraint: Nuke/Houdini/Karma **cannot decode ProRes RAW** (on
  Windows it isn't a supported Nuke host at all), so the RAW leg is **always** developed to **ACEScg
  scene-linear EXR** in Resolve before it touches comp. Nuke reads finished ProRes (422/4444) but never the
  RAW variant.
- **The S-Log3 IDT stays the entry point for the XAVC S-I leg** — native to the existing pipeline, no Resolve
  step. This is the escape hatch: if a shot doesn't warrant the RAW finish, or comp must start before the
  Resolve conform exists, the S-I file runs straight through the IDT/OCIO path. It's also the frame-accurate
  backup if the HDMI feed to the Ninja ever drops.
- If a shot is captured outside a supported log/raw profile, characterize and build an input transform for it.
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

## 6. Monitoring vs delivery — and the Resolve OCIO performance trap

**Two different transforms for two different jobs.** What you *view* through ≠ what you *render* through:
- **Monitoring** (interactive viewer, ingest / grade): speed matters; a LUT approximation is acceptable.
  Always **disabled before any scene-linear EXR export**.
- **Delivery** (one-time batch render): apply the managed **ACES Output Transform (ODT)** analytically, at
  full precision — **never** a viewer LUT. Render speed is irrelevant (it is not playback), so exactness wins.

**Resolve 21.0 OCIO slowdown (known, ACES 2.0).** Viewing ACEScg through Resolve's **OCIO** color science /
OCIO OFX with the `ACES 2.0 - SDR 100 nits (Rec.709)` view is pathologically slow in **Resolve 21.0** (was
fine in 20.3). 21.0's bundled OCIO runs the ACES 2.0 analytic DRT **off the GPU** — observed CPU busy / GPU
idle (CPU fallback), tracked upstream as OCIO #2116. *Not* a config or hardware fault: the same OCIO node is
fast with a lightweight (non-ACES-2.0) view or a different config.

**Fix — monitor with Resolve's native ACES Transform, not OCIO.** The ingest project runs **DaVinci YRGB
Color Managed (RCM)** anyway, so use the native engine for the viewer:

| ACES Transform (OFX) | Setting |
|---|---|
| ACES Version | `ACES 2.0` |
| Input Transform | `ACEScg - CSC` |
| Output Transform | **`sRGB Piecewise`** (*not* `sRGB Gamma 2.2`) |
| Gamut Compress | `None` |

- **`sRGB Piecewise`, not `Gamma 2.2`:** the OCIO `sRGB - Display` uses the piecewise sRGB EOTF; pure 2.2
  differs in the shadows. `Gamut Compress = None` matches the OCIO view (that control is the optional *input*
  gamut compress, which the view does not apply).
- Native = Blackmagic's GPU-optimized implementation → fast, and matches the `$OCIO` config to within
  monitoring tolerance. **Monitoring only — disable before exporting the ACEScg EXR.**

**Exact-match QC LUT (optional).** `config/ocio/ACEScct_to_ACES2-SDR-sRGB.cube` — a 65³ `ACEScct → ACES 2.0
SDR Rec.709 (sRGB-piecewise)` LUT baked from the committed config via PyOpenColorIO (validated to 2.6e-4).
Kept as ground truth: A/B native vs a `CST(ACEScg→ACEScct) → LUT` node pair in the shadows to *prove*
native == OCIO. Apply only via that CST→LUT pair — the LUT expects an **ACEScct** signal; never index it in
linear (a linear-input bake starves the shadows). Never used at delivery.

---

## 7. Handoffs & ordering

- **Houdini → Nuke:** linear **ACEScg EXR** (AOVs preserved).
- **Grain / film-emulation:** performed in **Nuke, in linear**, before the Resolve grade, so CG and plate
  share one grain ([film-emulation.md](film-emulation.md)). A small final-grain in Resolve is available if
  grading shifts grain.
- **Nuke → Resolve:** final comps as **ACES EXR** (ACEScg or ACES2065-1) for conform; a log/ProRes proxy
  for offline editorial. Resolve grades in **ACEScct** on the ACES timeline.
- **Resolve → delivery:** ACES Output Transform to the deliverable spec (Rec.709 web master / archive).
- **montmartre:** solo finishing — in-house **edit + cut + grade** (Resolve, ACEScct), no external hand-off.

---

## 8. Decisions to lock

- [x] ACES version: **2.0** (locked)
- [ ] Houdini / Nuke / Resolve on versions supporting ACES 2.0 output transforms + v2 OCIO config
- [ ] Exact OCIO config version, committed + `$OCIO` set on all stations/apps (committed file, not app bundle — §3)
- [x] Resolve monitoring = native **ACES Transform** (`sRGB Piecewise`), not the OCIO OFX (21.0 perf — §6)
- [ ] Camera IDT confirmed for the body + recording format actually shot
- [ ] A7R V dual-capture confirmed on the kit: ProRes RAW (Ninja V+) + simultaneous XAVC S-I internal; RAW
      conform target = **ACEScg EXR** out of Resolve (4444 only as a Nuke-native fallback)
- [ ] Delivery output transform(s) confirmed (Rec.709 / P3 / resolution)
- [ ] Nuke→Resolve handoff format (ACES EXR finals; proxy codec for offline)
