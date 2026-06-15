# Nuke Shortcuts — Cheat Sheet

Default keymap. On macOS, **Cmd** replaces **Ctrl**. Custom keymaps are configured under *Preferences →
Panels → Node Graph / Viewer*; entries marked _(verify)_ are version/preference dependent.

## Node graph — create & connect

| Action | Shortcut |
|---|---|
| Create node (search menu) | `Tab` → type name |
| Connect | Drag a node's bottom **output arrow** onto a target — or drag a node's **input arrowhead** onto a source |
| Insert / splice into an existing pipe | Drag a node **onto the connection** until the pipe highlights yellow, drop |
| Disconnect an input ("cut the cable") | Grab the **input arrowhead**, drag to empty canvas, release (no scissors tool exists) |
| Delete node + auto-rejoin | `Delete` / `Backspace` — Nuke bridges input→output if it's 1-in / 1-out |
| Extract (pull selected nodes out of the tree, keep them) | `Ctrl + Shift + X` |
| Branch a new node off an output | Select node, then `Tab`-create — branches a new leg |
| Copy / Cut / Paste | `Ctrl + C` / `Ctrl + X` / `Ctrl + V` (paste connects to selected node) |
| Clone / Declone | `Ctrl + K` / `Alt + Shift + K` |
| Disable / enable (pass-through) | `D` |
| Group selected nodes | `Ctrl + G` |
| Add a Dot (reroute / tidy wires) | `Tab` → "Dot", or `Ctrl`-drag along a pipe; then drag the dot to route |
| Open a node's properties | Double-click the node (or `Return`) |

## Viewer — connect & navigate

| Action | Shortcut |
|---|---|
| **View any node in the Viewer** | Select it, press **`1`** (creates a Viewer if none exists) |
| Connect to Viewer inputs 2…10 | `2`–`9`, `0` — tap the same number to **flip between inputs** (A/B compare) |
| Step Viewer input up / down the tree | `↑` / `↓` |
| Zoom to fit image | `F` |
| Zoom in / out | Scroll wheel (or `+` / `-`) |
| Pan | Middle-mouse drag (or `Alt` + drag) |
| Maximize / restore pane under cursor | `Space` |
| Isolate channel (toggle back with same key) | `R` / `G` / `B` / `A` |
| Toggle proxy mode | `Ctrl + P` |
| Wipe / compare A-B | Set two Viewer inputs, choose A/B wipe in the Viewer toolbar |

## Playback / timeline

| Action | Shortcut |
|---|---|
| Play forward / backward / stop | `L` / `J` / `K` |
| Faster playback (step up through speeds) | Tap `L` (forward) or `J` (reverse) **repeatedly** — each press speeds up |
| Next / previous frame | `→` / `←` |
| Jump to a specific frame | Click the **current-frame field** (bottom of the Viewer) and type the number |
| First / last frame of range | Transport buttons `⏮` / `⏭` on the Viewer timeline (no default key in stock Nuke) |
| Previous / next keyframe (animated knob) | `Alt + ←` / `Alt + →` _(verify)_ |
| Set playback **in / out** range | Drag the green range handles on the timeline, or type into the in/out fields by the transport |
| Loop / bounce / play-once mode | Toggle via the playback-mode button in the Viewer transport |
| Global project frame range (1–N) | `S` → Project Settings → `frame range` |

## General / workflow

| Action | Shortcut |
|---|---|
| Save / Save as | `Ctrl + S` / `Ctrl + Shift + S` |
| Undo / Redo | `Ctrl + Z` / `Ctrl + Y` |
| Select all / none | `Ctrl + A` / `Esc` |
| Fullscreen the current pane | `Space` (toggles maximize) |
| Render / Flipbook selected Write | `F5` (render) / `Alt + F` flipbook _(verify)_ |
| Node search across graph | `Tab` (nodes); `/` to start a node search in some builds _(verify)_ |

## Most-used

- View a node: select it, press `1`.
- Splice a node into a pipe: drag the node onto the connection until it highlights yellow.
