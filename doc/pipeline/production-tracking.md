# Production Tracking — Kitsu (evaluation & adoption notes)

**Status: DEFERRED — adopt later, not now.** This page captures the case for bringing a production tracker
into FLAT, what to use, and how to roll it out when the time comes. No action is committed yet.

Scope note: **MONTMARTRE is a solo spec/R&D program** — it does **not** need a tracker on its own. This is a
**studio-level (FLAT) decision**, because **FLAT is a 3-person, multi-discipline studio** (Houdini/comp/pipeline
+ editorial/Resolve + Blender/Cycles, with grade sometimes external). At 3 people with real handoffs, a tracker
starts to pay back.

---

## 1. What Kitsu is

[Kitsu](https://www.cg-wire.com/kitsu) (by **CGWire**) is an open-source **production tracker** — the
open alternative to ShotGrid / ftrack.

- **Tracks:** shots, assets, sequences, **tasks + statuses**, breakdowns (which assets are in which shot),
  **review playlists with frame-accurate notes**, time logs, scheduling, a Kanban board.
- **Deploy:** self-host via Docker (the API backend is **"Zou"**), or CGWire's paid **SaaS** (hosted).
- **API:** clean Python client (**Gazu**) — so it can be scripted/integrated with our DCC pipeline.
- **Artist-friendly web UI** — usable by the non-developer on the team without command-line.

---

## 2. The crucial distinction — tracking vs pipeline plumbing

These are two different layers. Don't conflate them (we use "pipeline" loosely for both):

| Layer | What it does | Tool |
|---|---|---|
| **Coordination / tracking** | who's on which shot/task, statuses, review notes, breakdowns | **Kitsu** |
| **Pipeline plumbing** | publish / version / load assets across DCCs (Houdini↔Nuke↔Blender↔Resolve) | **Ayon** (ex-OpenPype, by Ynput) — *uses Kitsu as its tracker backend* |

- "I want to know who's doing what and what's approved" → **Kitsu**.
- "I want one canonical, versioned publish of a cache/render that other steps load without guesswork" →
  **Ayon + Kitsu**.

Kitsu alone does **not** manage file versions. That's Ayon's job. Start with the tracking layer; the plumbing
layer is a much larger adoption (see §5).

---

## 3. Pertinence for FLAT

At **3 people across Houdini → Nuke → Resolve/Blender**, the things a tracker solves are real:

- **Handoffs** between disciplines (sim → comp → edit → grade) need visible status.
- **Review cycles** with versioned, frame-accurate notes (esp. with an editor + external colorist).
- **"Who's doing what"** stops fitting in one head once 3 people share shots/assets.

Verdict: **pertinent — worth piloting.** (For a strictly solo effort it would be overkill; the trigger is the
team size, not the tech.)

---

## 4. Recommendation (phased)

1. **Phase 1 — Kitsu only (tracking).** Immediate value, low overhead. Begin **SaaS-first** (trade a modest
   per-seat fee for zero ops) to prove value before investing in self-hosting. The only developer (lead) would
   otherwise own all the infra — minimise that early.
2. **Phase 2 — Ayon + Kitsu (plumbing)** — *only once version-management is a felt pain*. Adds publish/load/
   versioning across DCCs. Bigger learning curve and maintenance; the lead is sole maintainer. Don't take it on
   until tracking is bedded in.

**Do not adopt both at once.**

---

## 5. When to actually pull the trigger

Adopt on a concrete trigger, not speculatively:

- [ ] A shared project has **two+ people with handoffs** (already true for FLAT in general).
- [ ] Enough **shots/assets that status no longer fits in your head / a TODO**.
- [ ] You want **client/collaborator review** with versioned feedback.
- [ ] (Phase 2) Repeated **"which version of X does Y use?"** confusion → time for Ayon.

---

## 6. Rollout plan (when we do it)

1. **Pilot on a calm project** (MONTMARTRE or the next small job) — never first onboard during a deadline crunch.
2. **Model the real workflow**, not an idealised one: shots, the Houdini→Nuke→Resolve task chain, review
   playlists. See the starter shape in §7.
3. **Use it for 2–3 weeks.** If the team actually keeps it updated, standardise; if not, stop — a stale tracker
   is worse than none.
4. Only then **evaluate Ayon** for the publish/version layer.

---

## 7. Starter shape — FLAT pipeline mapped to Kitsu

A first cut, to be refined during the pilot. Mirrors our [project-structure.md](project-structure.md) layout.

```
Project (e.g. MONTMARTRE)
├── Assets
│   └── lichen            (element / sim system)   tasks: sim, lookdev (Houdini MaterialX), modeling (Blender)
├── Sequences / Shots
│   └── EP01 / SH010      tasks (the discipline chain):
│       ├── plate         ingest / prep
│       ├── track         matchmove (Nuke) → cam
│       ├── lighting/render  Houdini + Karma
│       ├── comp          Nuke (film-emulation)
│       ├── edit          Resolve cut
│       └── grade         Resolve / external colorist
└── Task statuses: todo → wip → review → retake → done
```

- Task types map 1:1 to the shot subfolders we already use (`plate/ track/ hou/ comp/ render/` + edit/grade).
- **Review playlists** per sequence for the editor + colorist handoffs.
- Keep it lean — only model tasks the team will actually update.

---

## 8. Cost, infra & ownership

- **SaaS (CGWire):** per-seat monthly; no maintenance. Recommended to start.
- **Self-host (Zou + Docker):** free software, but the **lead owns hosting, backups, updates** — real ongoing
  ops on one person. Move here only after SaaS proves the value and self-hosting is worth the time.
- **Non-developer collaborator:** the web UI is fine for them; keep the setup simple so it stays that way.

---

## 9. Risks / caveats

- **Discipline > features.** A tracker nobody updates is worse than nothing. Track only what gets maintained.
- **Admin overhead lands on the lead** (sole developer). Budget that time honestly.
- **Don't over-model.** Resist tracking every micro-task; it rots and breeds resentment.
- Conflicts with FLAT's stated lightweight philosophy ([project-structure.md](project-structure.md) defers
  heavy infra) — so adopt deliberately, at the tracking layer first, not as a big-bang pipeline overhaul.

---

## 10. Open decisions / next steps (when revived)

- [ ] SaaS vs self-host for the Phase-1 pilot (recommend SaaS first)
- [ ] Which project hosts the pilot (MONTMARTRE vs next small job)
- [ ] Final task-type / status list for the FLAT workflow (refine §7)
- [ ] Who is the de-facto coordinator/admin (likely the lead)
- [ ] Defer Ayon decision until tracking is bedded in

---

## References

- Kitsu — https://www.cg-wire.com/kitsu
- Kitsu docs — https://kitsu.cg-wire.com
- Zou (API backend) — https://zou.cg-wire.com
- Gazu (Python API client) — https://gazu.cg-wire.com
- Ayon (ex-OpenPype, pipeline + Kitsu backend) — https://ynput.io
