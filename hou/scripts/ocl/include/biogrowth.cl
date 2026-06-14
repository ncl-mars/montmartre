#ifndef __BIOGROWTH_CL__
#define __BIOGROWTH_CL__

// ---------------------------------------------------------------------------
// Safe normalize
// could be a ternary op such as (len > 1e-6f) ? v / len : 0
// ---------------------------------------------------------------------------
inline float3 bg_safe_normalize(float3 v) {
    return v * rsqrt(dot(v, v) + 1e-12f);
}

// ---------------------------------------------------------------------------
// Uniform grid helpers
// cell_size = perception_radius
// Neighbor search is always +-1 in each dim (27 cells), covers any radius <= cell_size
// ---------------------------------------------------------------------------
inline int3 bg_cell_coord(float3 pos, float3 grid_min, float cell_size) {
    return convert_int3(floor((pos - grid_min) / cell_size));
}

inline int bg_cell_linear(int3 coord, int3 grid_res) {
    return coord.x
         + coord.y * grid_res.x
         + coord.z * grid_res.x * grid_res.y;
}

inline int bg_cell_valid(int3 coord, int3 grid_res) {
    return coord.x >= 0 && coord.x < grid_res.x
        && coord.y >= 0 && coord.y < grid_res.y
        && coord.z >= 0 && coord.z < grid_res.z;
}

// ---------------------------------------------------------------------------
// Fixed-point scale for atomic float3 accumulation.(OpenCL 1.2 — int atomics only)
// int32 saturates at ~2.14e9; at scale 1e4 each axis can hold ~214k unit-vectors
// before overflow — safe for hotspot nodes picked by many contributors.
// Force contract: every contributor must emit `normalized_dir * strength_parm`
// before calling bg_atomic_add_dir so the accumulator stays O(n_contributors).
// ---------------------------------------------------------------------------
#define BG_DIR_SCALE 10000.0f

// ---------------------------------------------------------------------------
// Fixed-point scale for Murray's rule radius propagation.
// Each interior node accumulates Sum(r_child^p) * BG_MURRAY_SCALE.
// Direct-child count is typically <= 4 and r^p <= 1 for base_radius <= 1,
// so 1e4 keeps well within int32 range with plenty of precision.
// Reserved separately from BG_DIR_SCALE so the two semantics stay decoupled.
// ---------------------------------------------------------------------------
#define BG_MURRAY_SCALE 10000.0f


inline void bg_atomic_add_dir(global int* gx, global int* gy, global int* gz,
                               int idx, float3 dir) {
    atomic_add(&gx[idx], (int)(dir.x * BG_DIR_SCALE));
    atomic_add(&gy[idx], (int)(dir.y * BG_DIR_SCALE));
    atomic_add(&gz[idx], (int)(dir.z * BG_DIR_SCALE));
}

inline float3 bg_decode_dir(int ix, int iy, int iz) {
    return (float3)((float)ix, (float)iy, (float)iz) / BG_DIR_SCALE;
}

// ---------------------------------------------------------------------------
// Vigor — grid-participation filter.
//
// Interior tree nodes re-enter the uniform grid (and hence attract's search
// space) on a duty cycle keyed by their bifurcation depth. Depth 0 every
// frame, depth 1 every 2, depth 2 every 4, … capped at 2^depth_cap.
//
// `phase = depth * phase_stride` staggers which depth-N nodes appear on
// which frame, so populations don't pulse (all-in / all-out per period).
//
// Tips are exempt — the growth frontier is maximally vigorous by definition
// and always participates. Callers should test `is_tip` separately before
// invoking this helper.
//
// Both `grid_count` and `grid_scatter` MUST apply the same predicate with
// the same parameters — the scatter relies on offsets laid out by count;
// disagreement corrupts the grid.
// ---------------------------------------------------------------------------
inline bool bg_grid_eligible(int split_depth, int frame,
                             int depth_cap, int phase_stride) {
    int d      = min(split_depth, depth_cap);
    int period = 1 << d;
    int phase  = split_depth * phase_stride;
    return ((frame + phase) % period) == 0;
}


#define BG_DOMINANCE_SCALE 1000.0f


#endif // __BIOGROWTH_CL__
