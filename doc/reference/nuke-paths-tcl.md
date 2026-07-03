# Nuke Paths & TCL — Cheat Sheet

File knobs evaluate **TCL**, not shell. No bare `$VAR` — wrap in `[getenv VAR]`. Forward slashes always (Win too).

## Houdini → Nuke

| Houdini | Nuke |
|---|---|
| `$HIP` | `[file dirname [value root.name]]` |
| `$HIPNAME` | `[file rootname [file tail [value root.name]]]` |
| `$JOB` | `[getenv JOB]` — or **Project Directory** + relative path |
| `$HOME` | `[getenv HOME]` (Win: `[getenv USERPROFILE]`) |
| `$F` / `$F4` | `%d` / `%04d` |
| env var | `[getenv NAME]` |
| python | `[python {…}]` |

## Project Directory (= portable `$JOB`/`$HIP`)
`Root` knob **project_directory**. Set once → all paths can be relative.
- Track the script's own folder: `[python {nuke.script_directory()}]`
- Then read/write paths become `./plates/…`, `../prep/…`.

## Image sequences
- One folder per sequence; version subfolder (`…/v001/`).
- Pad: `SH020_plate_v001.####.exr` → knob writes `%04d`.
- Write node **creates missing dirs** on render.

## Gotchas
- `[value root.name]` empty until the script is **saved** → `$HIP` blank on a fresh script.
- Backslashes break TCL — never paste Windows `\` paths.
- TCL evals at knob-read; wrap dynamic bits, don't hardcode frames.

## montmartre launch (env before Nuke starts; OCIO can't be set from init.py)
```bat
set "JOB=D:/FLAT/SPEC/MONTMARTRE"
set "OCIO=D:/FLAT/SPEC/MONTMARTRE/config/ocio/studio-config-v3.0.0_aces-v2.0_ocio-v2.4.ocio"
```
Then `[getenv JOB]/shots/EP01/SH020/plate/v001/SH020_plate_v001.####.exr`.
