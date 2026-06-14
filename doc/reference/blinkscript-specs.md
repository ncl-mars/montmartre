# BlinkScript Language Specifications

Comprehensive reference for the Blink kernel language used in Nuke (The Foundry).
Combines official documentation with gotchas discovered during development.

Sources: [Foundry Blink Reference (Nuke 16.0)](https://learn.foundry.com/nuke/developers/16.0/BlinkUserGuide/BlinkKernelAPIReference/BlinkLanguage.html), Nuke 17.0v1 installation scan, hands-on testing in Nuke Indie.

---

## Data Types

### Scalars
| Type | Notes |
|------|-------|
| `float` | **Must use `f` suffix** (`1.0f`, never `1.0`). No `double` type exists. |
| `int` | Standard integer |
| `bool` | `true` / `false` |
| `char` | Single character only, no string ops |

### Vectors
| Type | Access |
|------|--------|
| `float2`, `float3`, `float4` | `.x`, `.y`, `.z`, `.w` or `[0]`..`[3]` |
| `int2`, `int3`, `int4` | Same access patterns |

Construction:
```cpp
float4 a(1.0f, 2.0f, 3.0f, 4.0f);   // C++ style
float3 b{1.0f, 2.0f, 3.0f};          // Initializer list
float4 c = float4(1.0f);              // All components = 1.0f
```

**Cannot convert between different-length vectors** (e.g. `float3` to `float4` is an error).

### Matrices
| Type | Notes |
|------|-------|
| `float3x3` | 3x3, row-major, access via `m[row][col]` |
| `float4x4` | 4x4, row-major, access via `m[row][col]` |

Methods: `setIdentity()`, `setTranslate()`, `setScale()`, `setRotate()`, `setRotateX/Y/Z()`, `translate()`, `scale()`, `rotate()`, `rotateX/Y/Z()`, `transpose()`, `invert()`.

### Rectangles
| Type | Fields | Methods |
|------|--------|---------|
| `recti` | `.x1`, `.y1`, `.x2`, `.y2` | `width()`, `height()`, `grow()`, `inside()` |
| `rectf` | `.x1`, `.y1`, `.x2`, `.y2` | `width()`, `height()`, `grow()`, `inside()` |

---

## Kernel Structure

```cpp
kernel MyKernel : ImageComputationKernel<ePixelWise> {
    Image<eRead, eAccessPoint, eEdgeClamped> src;
    Image<eWrite> dst;

    param:
        float myParam;

    local:
        float myLocal;

    void define() {
        defineParam(myParam, "My Param", 1.0f);
    }

    void init() {
        myLocal = myParam * 2.0f;
    }

    void process(int2 pos) {
        dst() = src() * myLocal;
    }
};
```

### Lifecycle

| Function | When | Threading | Notes |
|----------|------|-----------|-------|
| `define()` | Once at kernel creation | Single | Only `defineParam()` calls allowed. Cannot call user functions. |
| `init()` | Once before processing | Single | Initialize `local:` vars, call `setRange()`/`setAxis()`. Can call user functions (but not access Images). |
| `process()` | Per pixel/component | Parallel | All threads. `local:` vars are **read-only** here. |

### process() Signatures
- `void process()` — no position info
- `void process(int2 pos)` — `pos.x`, `pos.y` pixel coordinates
- `void process(int3 pos)` — `eComponentWise` only; `pos.z` = component index (0=R, 1=G, 2=B, 3=A)

### Granularity Modes
- **`ePixelWise`** — processes entire pixels (all RGBA at once). Best when channels are interdependent.
- **`eComponentWise`** — processes one channel at a time across all pixels, then the next. Use when channels are independent.

### Constraints
- Exactly ONE output image per kernel
- Multiple input images allowed
- Only RGBA channels — use Shuffle nodes for other channels
- No control over pixel processing order

---

## Variable Scopes

### `param:` block
Exposed as Nuke knobs. Read-only during `process()`.

| Type | Knob Generated |
|------|---------------|
| `float` | Float slider |
| `float2` / `float[2]` | XY handle |
| `float3` / `float[3]` | XYZ handle |
| `float4` / `float[4]` | Color picker (RGBA) |
| `int` | Int knob |
| `bool` | Checkbox |
| `float3x3` | Array knob (3x3 grid) |
| `float4x4` | Array knob (4x4 grid) |

Knob naming follows: `KernelName_DisplayName`.

`eParamProxyScale` — optional property for `defineParam()` that scales parameters with proxy resolution.

### `local:` block
Private variables, not exposed. Set in `init()`, become **read-only** during `process()`. Thread-safe by design.

### Variables in `process()`
Stack-local, per-pixel, per-thread. Fully writable.

---

## Image Access

### Read/Write Spec
| Spec | Use |
|------|-----|
| `eRead` | Input images (read-only) |
| `eWrite` | Output image (write-only) |
| `eReadWrite` | Both — used heavily in ParticleBlinkScript gizmos, may not work in regular BlinkScript node |

### Access Patterns
| Pattern | Syntax | Setup |
|---------|--------|-------|
| `eAccessPoint` | `src()` | Default, current pixel only |
| `eAccessRanged1D` | `src(offset)` | Requires `setRange()` + `setAxis(eX/eY)` in `init()` |
| `eAccessRanged2D` | `src(xOff, yOff)` | Requires `setRange()` in `init()` |
| `eAccessRandom` | `src(x, y)` | Absolute coords, required for `bilinear()` |

### Edge Methods
| Method | Behavior |
|--------|----------|
| `eEdgeNone` | Undefined outside bounds (fastest, no bounds checking) |
| `eEdgeClamped` | Repeats edge pixels |
| `eEdgeConstant` | Returns zero outside bounds |
| `eEdgeNull` | Found in syntax highlighter keywords — undocumented, distinct from `eEdgeNone` |

### Bilinear Sampling
```cpp
bilinear(img, float x, float y)            // full pixel, returns SampleType
bilinear(img, float x, float y, int comp)  // single component
```
Requires `eAccessRandom`.

### Image Properties
Available in `init()` and `process()`:
- `image.bounds` — `recti` with `.x1`, `.y1`, `.x2`, `.y2`, `.width()`, `.height()`
- `image.kComps` — int, number of components (e.g. 4 for RGBA)
- `image.kMin`, `image.kMax` — float, min/max component values
- `image.kWhitePoint` — float
- `image.kClamps` — bool
- `ValueType(image)` — component data type
- `SampleType(image)` — complete pixel type

---

## Functions

### User-Defined Functions
- Defined inside or outside the kernel body
- `inline` keyword supported (defined outside kernel, before it)
- **No function overloading** — each function must have a unique name
- **No recursion** — functions cannot call themselves or form call cycles
- **No default parameter values**
- **No lambdas, no function pointers**

```cpp
inline float3 mix(float3 A, float3 B, float amount) {
    return A * (1.0f - amount) + B * amount;
}

kernel MyKernel : ImageComputationKernel<ePixelWise> {
    // can call mix() here
};
```

### Arrays
- Fixed-size only: `float data[10];`
- Can be initialized: `int arr[4] = {4, 3, 2, 1};`
- **Arrays pass to functions by reference** — modifications persist in calling scope
- Variable-length arrays NOT supported

---

## Preprocessor

### Supported
- `#define` / `#undef` — simple macro replacement
- `#ifdef` / `#else` / `#endif` — conditional compilation

### Not Supported
- `#include` (use `.blib` library files instead)
- `#pragma`
- `#if` with expressions (only `#ifdef`)
- Complex macro functions

---

## Control Flow

### Supported
- `if` / `else if` / `else`
- Ternary: `condition ? valueA : valueB` (works with vector results too)
- `for` loops
- `while` loops
- `break`, `continue`, `return`

### Not Supported
- `switch`
- `do-while`
- `goto` / labels

---

## Type Casting

- **No C-style casts** `(float)x`
- **No C++ casts** (`static_cast<>`, etc.)
- Use **conversion constructors**: `float(myInt)`, `int(myFloat)`
- Integer division needs explicit conversion: `y / float(n - 1)`
- Assignment between same-length vectors of different scalar types is allowed

---

## Built-in Math Functions

### Trigonometric
`sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2` — scalar only (except `cos` works on vectors)

### Exponential / Power
`exp`, `log`, `log2`, `log10`, `pow`, `sqrt`, `rsqrt` (1/sqrt)

### Rounding
`floor`, `ceil`, `round` (nearest, ties to even)

### Absolute / Sign
`fabs` (float), `abs` (int/int1-4), `sign` (returns -1.0f, 0.0f, or 1.0f)

### Remainder
`fmod(x, y)`, `modf(x, *b)` (integer + fractional parts)

### Clamping / Interpolation
`min`, `max`, `clamp(value, min, max)`, `lerp(a, b, t)` (Nuke 16+)

### Reciprocal
`rcp(x)` — returns 1/x

### Vector / Geometry
`dot(x, y)`, `cross(x, y)` (float3 only), `normalize(x)`

### Statistical
`median(float data[], int size)`

### Conditional Vector
`any(condition)`, `all(condition)`

### Atomic (int only, thread-safe)
`atomicAdd(int_type* ptr, int val)`, `atomicInc(int_type* ptr)`

### Debugging (Nuke 16+)
`println(int x, int y, args...)` — prints to terminal at specified pixel. **Warning:** unconditional use prints for every pixel.

### Built-in Constants
- `PI` — used in Foundry's WipeKernel and CurtainKernel examples

### Version Macros (Preprocessor)
Available for conditional compilation across Nuke versions:
- `kNukeVersionMajorNum`
- `kNukeVersionMinorNum`
- `kNukeVersionReleaseNum`

```cpp
#ifdef kNukeVersionMajorNum
// version-specific code
#endif
```

### Missing Functions — Workarounds

| Function | Workaround |
|----------|------------|
| `step(edge, x)` | `(x >= edge) ? 1.0f : 0.0f` |
| `smoothstep(e0, e1, x)` | `t = clamp((x-e0)/(e1-e0), 0, 1); t*t*(3-2*t)` |
| `frac(x)` / `fract(x)` | `x - floor(x)` |
| `mod(x, y)` | `x - y * floor(x / y)` |
| `exp2(x)` | `pow(2.0f, x)` |

---

## Operators

### Arithmetic
`+`, `-`, `*`, `/`, `%` (int only), unary `-`

### Comparison
`==`, `!=`, `<`, `>`, `<=`, `>=`

### Logical
`&&`, `||`, `!`

### Bitwise (int and bool only, NOT float/vector)
`~`, `&`, `|`, `^`, `<<`, `>>`

### Assignment
`=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`

---

## Library Files (.blib)

Introduced in Nuke 16.0. Share common functions across kernels.

- Added via Library Files tab in BlinkScript node
- Paths can be relative to Blink library folder preference, project directory, or absolute
- Support TCL notation for environment variables
- **Baked BlinkScript:** Library files cached and serialized — allows copying between machines
- **Protected BlinkScript:** Cached and encrypted, editor removed

### Function overloading causes ambiguity
Overloaded functions (same name, different parameter types) across .blib files cause:
> `call to member function 'X' is ambiguous`

Use distinct names with type suffixes:
```cpp
// WRONG — ambiguous across .blib boundaries
float frac(float x);
float2 frac(float2 x);

// CORRECT
float frac(float x);
float2 frac2(float2 x);
float3 frac3(float3 x);
```

Same applies to `dot` helpers (`dot22`, `dot23`), `fabs` wrappers (`fabs2`, `fabs3`), `max` wrappers (`max2`, `max3`), etc.

**Note:** `maxComp(float2)` and `maxComp(float3)` overloads have not caused issues so far, but may need renaming if they do.

### Duplicate names cause collisions
Two .blib files cannot define a function with the same name. Move shared helpers to a single shared library (e.g. `utils.blib`).

### Cross-library calls work
Functions defined in one .blib can be called from another .blib or from .blink kernels, as long as the library is loaded in Nuke's Library Files panel.

### Nuke Non-Commercial limitation
Nuke Non-Commercial cannot add Library Files (the Add button does nothing). Nuke Indie works fine.

---

## Vector Operation Gotchas

### No scalar-to-vector broadcasting
Operations like `float4 + scalar` or `scalar - float3` fail:
> `no matching conversion for C-style cast from 'const float' to 'ExpandedType'`

Must expand explicitly:
```cpp
// WRONG
float4 result = myVec4 - 0.5f;

// CORRECT
float4 result = myVec4 - float4(0.5f, 0.5f, 0.5f, 0.5f);
```

**Exception:** scalar multiply DOES work:
```cpp
float3 result = myVec3 * 6.0f;  // OK
```

### No vector fabs/abs
`fabs()` only works on scalar `float`. Expand manually:
```cpp
// WRONG
float3 v = fabs(myVec3);

// CORRECT
float3 v = float3(fabs(myVec3.x), fabs(myVec3.y), fabs(myVec3.z));
```

### No vector max/min
`max()` and `min()` only work with matching scalar types:
```cpp
// WRONG
float2 v = max(myVec2, 0.0f);

// CORRECT
float2 v = float2(max(myVec2.x, 0.0f), max(myVec2.y, 0.0f));
```

---

## What's NOT in BlinkScript (vs C++)

- No `struct`, `class`, `union`, `enum`, `typedef`, `using`
- No templates
- No pointers, no dynamic allocation, no `new`/`delete`
- No STL whatsoever
- No function overloading or operator overloading
- No default parameter values
- No recursion
- No lambdas, no function pointers
- No `switch`, `do-while`, `goto`
- No `static`, `volatile`, `thread_local` (but see `static const` exception below)
- No exception handling (`try`/`catch`/`throw`)
- No `sizeof`, `typeid`
- No C-style or C++ casts
- No `#include`, `#pragma`
- No `double` type
- No strings (only char literals and `println()` string literals)

Only supported type qualifier: `const`. Exception: `static const` works in `local:` block for compile-time constants (used in Foundry's LensFlare example: `static const int kMaxDots = 10;`).

---

## Reserved Names

| Name | Issue | Workaround |
|------|-------|------------|
| `gain` | Reserved keyword | Rename to `iqGain` or similar |

---

## Common Gotchas

1. **Always use `f` suffix** on float literals — `1.0f` not `1.0`, avoids double-precision promotion
2. **`length()` works as Euclidean magnitude** on Nuke 17 (tested in SDF shapes). The 15.1/16.0 docs incorrectly claim it returns element count — this may have been a docs error or was fixed in 17.
3. **No pixel order guarantees** — cannot depend on previously-computed pixels in `process()`
4. **RGBA only** — use Shuffle nodes for other channels
5. **Uninitialized output** may contain garbage with `eAccessPoint`
6. **No bounds checking** — out-of-bounds access crashes Nuke (enable Safety Rails during dev)
7. **No built-in random** — must implement your own hash function
8. **`local:` vars become read-only** during `process()`
9. **GPU timeouts** possible with complex kernels on lower-end GPUs
10. **`eEdgeConstant` returns zero** — can cause black fringing in blur-like operations
11. **Unconnected input images cause errors** — must have something connected
12. **SIMD vectorization** not guaranteed on CPU, may increase compile time

---

## ParticleBlinkScript

Operates on particles instead of images. Same Blink language but with a completely different data model. **CPU only** — no GPU acceleration.

### Kernel Class

Uses the same `ImageComputationKernel<ePixelWise>` base class. There is no `ParticleComputationKernel`. The ParticleBlinkScript node maps particle data into 1D images (width=1, height=numParticles).

```cpp
kernel MyParticleKernel : ImageComputationKernel<ePixelWise>
{
    Image<eReadWrite> p_velocity;
    Image<eRead> p_mass;
    Image<eRead> p_conditions;

    param:
        float _drag;
        float _dt;

    void define() {
        defineParam(_drag, "pa_drag", 0.5f);
        defineParam(_dt, "_dt", 1.0f);
    }

    void process() {
        if (p_conditions()) {
            p_velocity() = p_velocity() * pow(1.0f - _drag, _dt);
        }
    }
};
```

### Particle Attribute Access

Attributes are declared as `Image` objects with a **`p_` prefix**. The node binds each `p_`-prefixed Image to the matching particle attribute (prefix stripped).

```cpp
Image<eReadWrite> p_velocity;   // binds to "velocity"
Image<eRead> p_mass;            // binds to "mass" (read-only)
Image<eReadWrite> p_color;      // binds to "color"
```

Read/write in `process()`:
```cpp
float3 vel = p_velocity();       // read current particle
p_velocity() = vel * 0.99f;     // write back
```

### Standard Particle Attributes

| Declaration | Type | Description |
|---|---|---|
| `p_position` | float3 | Current position |
| `p_initialPosition` | float3 | Position at birth |
| `p_lastPosition` | float3 | Position at previous time step |
| `p_velocity` | float3 | Velocity vector |
| `p_color` | float4 | RGBA color |
| `p_size` | float3 | Particle dimensions (x,y,z) |
| `p_orientation` | float4 | Orientation quaternion |
| `p_rotationAxis` | float3 | Axis of rotation |
| `p_rotationAngle` | float | Angle of rotation |
| `p_rotationVelocity` | float | Angular velocity |
| `p_mass` | float | Mass |
| `p_life` | float | Lifespan duration |
| `p_startTime` | float | Time the particle was created |

### Pseudo-Attribute: `p_conditions`

Declare as `Image<eRead>`. Returns true/false based on the node's Conditions and Region tab settings (probability, age range, spatial region):
```cpp
if (p_conditions()) {
    // only process particles matching conditions
}
```

### Magic Parameters (System-Provided)

Declared as `param:` with underscore prefix. Automatically filled by the node:

| Parameter | Type | Description |
|---|---|---|
| `_frame` | float | Current frame |
| `_systemTime` | float | Particle system's current time |
| `_dt` | float | Time delta for this step |
| `_startTime` | float | Start of current time step |
| `_endTime` | float | End of current time step |

```cpp
param:
    float _dt;
    float _endTime;

void define() {
    defineParam(_dt, "_dt", 1.0f);
    defineParam(_endTime, "_endTime", 1.0f);
}
```

### process() Signatures

- `void process()` — most common, no particle index needed
- `void process(int2 pos)` — `pos.y` = particle index (`pos.x` always 0)

### Parameter Knobs — Major Difference from BlinkScript

**Knobs are NOT auto-generated.** You must manually create User Knobs on the node, matched by name.

- Convention: use `pa_` prefix to avoid clashes with built-in knobs
- The **"Create Knobs"** button on the node will auto-generate missing User Knobs
- If you rename a parameter, you must rename the User Knob too

### Random Access to Other Particles

Declare with `eAccessRandom` to access any particle by index:
```cpp
Image<eRead, eAccessRandom> p_position;
Image<eReadWrite, eAccessRandom> p_life;
```

Access by particle index (y-coordinate):
```cpp
float3 otherPos = p_position(0, j);          // read particle j's position (vector)
float otherLife = p_life(0, j, 0);           // read particle j's life (scalar, component 0)
p_life(0, j, 0) = 0.0f;                     // kill particle j
```

Get total particle count in `init()`:
```cpp
void init() {
    _numParticles = p_position.bounds.height();
}
```

**Critical threading caveat:** No guarantee of processing order across threads. Writing to other particles causes race conditions. The only safe pattern is to serialize by running only on the last particle:
```cpp
void process(int2 pos) {
    if (pos.y == _numParticles - 1) {
        // iterate all particles here — single-threaded but safe
        for (int i = 0; i < _numParticles; ++i) { ... }
    }
}
```

### Image Inputs (Sampling Textures)

Declare with **`image_` prefix** and `eAccessRandom`:
```cpp
Image<eRead, eAccessRandom, eEdgeClamped> image_texture;
```

This creates an input named "texture" on the node. Sample with `bilinear()`:
```cpp
void process() {
    float3 p = p_position();
    float2 uv = /* compute from position */;
    float2 st = uv * float2(image_texture.bounds.width(), image_texture.bounds.height());
    float4 c = bilinear(image_texture, st.x, st.y);
    p_color() = c;
}
```

### Creating / Deleting Particles

- **Cannot create/spawn particles** — use ParticleEmitter or ParticleSpawn nodes upstream
- **Can kill particles** by setting life to 0: `p_life(0, i, 0) = 0.0f;`

### Newborn Particle Time Handling

Particles born mid-timestep have a shorter effective `_dt`. Correct handling:
```cpp
float dt = max(0.0f, min(_dt, float(_endTime - p_startTime())));
```

### ParticleBlinkScript vs BlinkScript Node

| Aspect | BlinkScript | ParticleBlinkScript |
|---|---|---|
| Node type | ImageOp (2D) | ParticleOp (3D) |
| Input/Output | Image streams | Particle streams |
| Iteration space | Pixels (W x H) | Particles (1 x N) |
| GPU support | Yes (OpenCL) | **No (CPU only)** |
| Parameter knobs | Auto-generated | Manual User Knobs (name-matched) |
| Conditions/Region tabs | No | Yes |
| Magic time parameters | No | `_frame`, `_systemTime`, `_dt`, `_startTime`, `_endTime` |

### ParticleBlinkScript Gotchas

1. **Kernel class is still `ImageComputationKernel`** — no separate particle kernel class exists
2. **CPU only** — no GPU acceleration despite Blink supporting it for image kernels
3. **User Knobs must match parameter names exactly** — use "Create Knobs" button to help
4. **`p_life = 0` kills particles** — not obvious from docs, but it's the standard pattern
5. **No spatial acceleration** — no built-in neighbor lookup, all-pairs search is O(n^2)
6. **Random access writes are thread-unsafe** — must serialize (only execute on last particle index)
7. **`p_conditions` is `Image<eRead>`** even though it's boolean-like
8. **Image inputs need `image_` prefix AND `eAccessRandom`** — without random access, image iteration is meaningless
9. **No `noise()` or `random()` built-in** — implement your own or sample a noise image
10. **`_dt` varies for newborn particles** — always clamp with `_endTime - p_startTime()`

---

## Editor Tips

- **Ctrl+Enter** — recompile kernel without leaving the editor
- **Hidden profiler** — set env var `BLINK_EDITOR_PROFILER`, then Ctrl+Shift+O to toggle cProfile profiling of the editor widget
- **Safety Rails** — enable during development to catch out-of-bounds memory access with safe fallback values and terminal logging

---

## Shipped Example Kernels

Located in `C:\Program Files\Nuke17.0v1\Documentation\html\content\example_scripts\`:

| File | Kernel | Technique |
|------|--------|-----------|
| `blinkscript_lensflare.nk` | LensFlareKernel | No inputs, generates from scratch. `inline` helpers, `static const`, local arrays, PRNG |
| `blinkscript_ballomatic.nk` | Ballomatic | `eAccessRandom` + `bilinear()`, `dst.kComps` for looping components |
| `blinkscript_swirlomatic.nk` | Swirlomatic | `eAccessRandom`, trig transforms, `bilinear()` |
| `blinkscript_boxblur1d.nk` | BoxBlur1D | `eAccessRanged1D`, `setRange()`, `setAxis(eX)` |
| `blinkscript_boxblur2d.nk` | BoxBlur2D | `eAccessRanged2D`, `setRange(-xR, -yR, xR, yR)` |
| `blinkscript_median.nk` | Median5x5Kernel | `eAccessRanged2D`, built-in `median(array, length)` |
| `blinkscript_convolution.nk` | ConvolutionKernel | Two inputs (src + filter), `filter.bounds`, `SampleType()`, `ValueType()` |
| `blinkscript_wipe.nk` | WipeKernel | Two inputs, built-in `PI` constant |
| `blinkscript_curtain.nk` | CurtainKernel | `eAccessRanged1D` + `eEdgeConstant`, `process(int3 pos)` componentwise |
| `blinkscript_twopassresize.nk` | ResizeKernel | Two-pass technique (same kernel horizontal then vertical), `bilinear()`, `bool` param |
| `blinkscript_knobs.nk` | KnobsKernel | Demonstrates ALL param types: float, float2/3/4, float[5], float3x3, float4x4, int, int[4], bool |

### Shipped Particle Gizmos

Located in `C:\Program Files\Nuke17.0v1\plugins\`:

| Gizmo | Key Technique |
|-------|---------------|
| `ParticleFuse.gizmo` | `eAccessRandom`, inter-particle interaction, serialized processing |
| `ParticleFlock.gizmo` | Complex flocking with 6 rules, `dot()`, `length()`, `normalize()` |
| `ParticleShockWave.gizmo` | `float4x4` matrix ops: `setIdentity()`, `scale()`, `rotateX/Y/Z()`, `translate()`, `invert()` |
| `ParticleColorByAge.gizmo` | Age-based color with spline interpolation, local arrays |
| `ParticleKill.gizmo` | Simplest kernel — reads condition, kills particle |
| `ParticleGrid.gizmo` | Grid distribution |
| `ParticleDistributeSphere.gizmo` | Sphere distribution |
| `ParticleCylinderFlow.gizmo` | Cylinder flow patterns |
| `ParticleHelixFlow.gizmo` | Helical flow |
| `ParticleConstrainToSphere.gizmo` | Sphere constraint |
| `ParticleAttractToSphere.gizmo` | Sphere attraction |
| `ParticleDirection.gizmo` | Direction control |
| `ParticleProjectDisplace.gizmo` | Project displacement |
| `ParticleProjectImage.gizmo` | Image projection onto particles |

---

## Additional Kernel Types (C++ SDK Only)

Found in `C:\Program Files\Nuke17.0v1\include\Blink\BlinkConstants.h`:

Beyond the standard `ImageComputationKernel` (Iterate), the Blink C++ SDK defines:
- **`kBlinkKernelTypeRolling`** — rolling kernel type
- **`kBlinkKernelTypeReduce`** — reduction kernel type

These are not exposed in BlinkScript but exist in the NDK for C++ plugin development.
