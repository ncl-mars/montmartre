## Nuke 17 + VSCode BlinkScript Workflow — Full Setup Guide

### Prerequisites

- **Windows 11**
- **VSCode** installed
- **Nuke 17** installed (Non-Commercial, Indie, or Commercial) — e.g. `C:\Program Files\Nuke17.0v1\`

---

### Step 1 — Install VSCode Extensions

1. Install **NukeTools** extension by virgilsisoe (`Ctrl+Shift+X` → search "NukeTools")
2. NukeTools automatically installs **NukeServerSocket** into `C:\Users\<user>\.nuke\NukeTools\nukeserversocket\`

### Step 2 — Configure NukeTools executable path

In VSCode: `Ctrl+,` → search "NukeTools" → set **Executable Path**:
```
C:\Program Files\Nuke17.0v1\Nuke17.0.exe
```

### Step 3 — Keyboard shortcut

Add to VSCode keybindings (`Ctrl+Shift+P` → "Preferences: Open Keyboard Shortcuts (JSON)"):

```json
{
    "key": "ctrl+shift+r",
    "command": "nuke-tools.runCodeInsideNuke",
    "when": "resourceExtname == .blink"
}
```

This lets you send the current `.blink` file to Nuke with `Ctrl+Shift+R` instead of going through the command palette each time.

### Step 4 — Fix NukeServerSocket for PySide6 (Nuke 17)

Nuke 17 ships PySide6/Qt6. NukeServerSocket was written for PySide2/Qt5 and needs 4 patches.

---

#### Fix 4a — Socket signal name (critical: without this, no data transfers)

**File:** `C:\Users\<user>\.nuke\NukeTools\nukeserversocket\nukeserversocket\server.py`

**Line 68** — change `.error` to `.errorOccurred`:

```python
# BEFORE:
self._socket.error.connect(lambda err: LOGGER.error('Socket error: %s', err))

# AFTER:
self._socket.errorOccurred.connect(lambda err: LOGGER.error('Socket error: %s', err))
```

**Why:** PySide6 renamed the `QTcpSocket.error` signal to `errorOccurred`. In PySide6, `.error` is a method, not a signal — connecting to it silently breaks the socket and no data is ever received.

---

#### Fix 4b — Timeout disable logic (connection stays open)

**File:** `C:\Users\<user>\.nuke\NukeTools\nukeserversocket\nukeserversocket\main.py`

**Two changes:**

**1) `_on_data_received` method (~line 120)** — add a guard so timer doesn't start when timeout is 0:

```python
# BEFORE:
@Slot(str)
def _on_data_received(self):
    self._timer.start(self._model.get_server_timeout())

# AFTER:
@Slot(str)
def _on_data_received(self):
    timeout = self._model.get_server_timeout()
    if timeout and timeout > 0:
        self._timer.start(timeout)
```

**2) `_on_connect` method (~line 135)** — same guard on initial connect:

```python
# BEFORE:
LOGGER.info('Listening on %s...', port)
self._timer.start(self._model.get_server_timeout())
self._view.port_input.setEnabled(False)

# AFTER:
LOGGER.info('Listening on %s...', port)

timeout = self._model.get_server_timeout()
if timeout and timeout > 0:
    self._timer.start(timeout)
self._view.port_input.setEnabled(False)
```

**Why:** Without this, setting timeout to 0 fires the timer immediately, disconnecting the server instantly.

---

#### Fix 4c — Set timeout to 0

**File:** `C:\Users\<user>\.nuke\nukeserversocket.json`

```json
{
    "port": 54321,
    "server_timeout": 0,
    "mirror_script_editor": false,
    "clear_output": true,
    "format_output": "[%d NukeTools] %F%n%t"
}
```

---

#### Fix 4d — Windows backslash path for .blink files

**File:** `C:\Users\<user>\.nuke\NukeTools\nukeserversocket\nukeserversocket\controllers\nuke.py`

**`_blink_wrapper` method (~line 90)** — replace backslashes with forward slashes in the file path:

```python
# BEFORE:
knobs['kernelSourceFile'].setValue('{file}')
```
with format argument:
```python
file=data.file,
```

```python
# AFTER:
knobs['kernelSourceFile'].setValue('{file}')
```
with format argument:
```python
file=data.file.replace('\\', '/'),
```

Full corrected method for reference:

```python
def _blink_wrapper(self, data: ReceivedData) -> str:
    return dedent("""
        node = nuke.toNode("{filename}") or nuke.createNode('BlinkScript', 'name {filename}')
        knobs = node.knobs()
        knobs['kernelSourceFile'].setValue('{file}')
        knobs['kernelSource'].setText({text})
        knobs['recompile'].execute()
        """).format(
        filename=os.path.splitext(os.path.basename(data.file))[0],
        file=data.file.replace('\\', '/'),
        text=json.dumps(data.text)
    ).strip()
```

**Why:** Windows paths with `\U`, `\n`, etc. cause Python `SyntaxError: unicode escape` when injected into the code string sent to Nuke.

---

### Step 5 — Workflow

1. **Launch Nuke normally** (Start menu / shortcut — **not** from VSCode, which crashes due to Electron/Qt6 conflict: `QEventDispatcherWin32::wakeUp: Failed to post a message`)
2. In Nuke: open **NukeServerSocket** panel → click **Connect**
3. In Nuke: create a **BlinkScript** node (`Tab` → "BlinkScript"), connect an input image
4. In VSCode: open a `.blink` file
5. Press `Ctrl+Shift+R` — sends the kernel to Nuke, creates/updates the BlinkScript node matching the filename, and recompiles

---

### Step 6 — Fix NukeTools Syntax Highlighting (path bug)

NukeTools v0.17.0 has a bug where the `package.json` references grammar files at `./language/` but they actually live in `./resources/language/`. This breaks BlinkScript (and `.nk`) syntax highlighting.

**File:** `C:\Users\<user>\.vscode\extensions\virgilsisoe.nuke-tools-0.17.0\package.json`

Fix these 3 paths in `contributes.grammars` and `contributes.languages`:

```
"./language/blinkscript.tmLanguage.json"  →  "./resources/language/blinkscript.tmLanguage.json"
"./language/nuke.tmLanguage.json"         →  "./resources/language/nuke.tmLanguage.json"
"./language/language-configuration.json"  →  "./resources/language/language-configuration.json"
```

**Reload VSCode** after editing (`Ctrl+Shift+P` → "Reload Window").

**Note:** This patch will be overwritten if the NukeTools extension updates. Re-apply after any update.

---

### Optional — BlinkScript C++ Linting

NukeTools provides syntax highlighting for `.blink` files natively. For additional IntelliSense/autocomplete, you can set up the **C/C++ extension** (Microsoft) with a stub header:

1. Install C/C++ extension
2. Create `blinkscript.h` in your project root — declaring all BlinkScript built-in types (`Image<>`, `float2/3/4`, `int2/3/4`, `recti`, `float3x3`, `float4x4`, kernel base classes, math functions, operator overloads)
3. Create `.vscode/c_cpp_properties.json` with `forcedInclude` pointing to the stub header
4. **Trade-off:** This requires associating `*.blink → cpp`, which overrides NukeTools' native highlighting

---

### Notes — Maintenance

- **NukeServerSocket updates:** If NukeTools updates the NukeServerSocket plugin (via extension update or reinstall), all 4 patches will be overwritten and must be re-applied. Check after any NukeTools extension update.
- **Nuke version upgrades:** The `.nuke` folder is shared across all Nuke versions on the same machine. Patches survive Nuke upgrades as long as NukeServerSocket itself isn't reinstalled.
- **PySide6 import:** NukeServerSocket source files already have `from PySide6` imports in this version. If you're on a version that still has `from PySide2`, you'll also need to update all PySide2 imports to PySide6 across the plugin.

### Notes — File Formats & License Tiers

| License | Project format | Can open in Commercial? | Can open Commercial files? |
|---|---|---|---|
| **Non-Commercial** | `.nknc` | No (encrypted) | Yes |
| **Indie** | `.nkind` / `.hroxind` / `.gzind` | No (encrypted) | Yes |
| **Commercial** | `.nk` / `.hrox` | — | — |

- All encrypted formats (`.nknc`, `.nkind`) are **one-way**: Indie/NC can read Commercial files, but Commercial **cannot** read Indie/NC files. There is no converter.
- **What carries over across all licenses:** `.blink` kernels, Python scripts, gizmo source code, exported media (EXR, PNG, etc.), EDLs, XMLs, geometry via WriteGeo. These are all plain text or standard formats.
- **What does NOT carry over:** Nuke project files (`.nk` / `.nkind` / `.nknc`), saved node graphs, comp scripts. If you move from Indie to Commercial, you rebuild your comps from scratch.

### Notes — Migrating from Non-Commercial to Indie

1. **Uninstall** Nuke Non-Commercial
2. **Install** Nuke Indie (same major version recommended, e.g. 17.x)
3. **Update** the executable path in VSCode NukeTools settings to point to the new install
4. **Verify patches:** Open the 4 patched files and confirm the changes are still in place. If NukeServerSocket was reinstalled during the process, re-apply all patches.
5. **Your `.nuke` folder is untouched** — `menu.py`, `nukeserversocket.json`, and all plugin files persist across installs
6. **Your `.blink` files and VSCode config are untouched** — they live in your project directory, not in the Nuke install
7. **Non-Commercial `.nknc` projects will NOT open in Indie** — same encryption restriction applies between tiers. Start fresh or re-export what you need before uninstalling
