#if (!defined(FILTER) || !defined(FILTER_SIZE) || !defined(FILTER_SUPPORT)) && \
    !defined(FILTER_POINT) && !defined(FILTER_BILINEAR) && \
    !defined(FILTER_BOX) && !defined(FILTER_TRIANGLE) && \
    !defined(FILTER_CUBIC) && !defined(FILTER_MITCHELL) && \
    !defined(FILTER_BSPLINE)
#define FILTER_POINT
#endif

#if defined(FILTER) && defined(FILTER_SIZE) && defined(FILTER_SUPPORT)
// this is so snippet can define it's own filter before including this

#elif defined(FILTER_POINT)
static float4
bufferSampleRectF4(const IMX_Layer* layer, float2 xy, float2 dxy,
                  BorderType border, StorageType storage, int channels)
{
    // Make .5 round toward -infinity so a translate by .5 does not throw away every other pixel
    return bufferIndexF4(layer, convert_int2_sat_rtn(xy + 0.5f), border, storage, channels);
}

#elif defined(FILTER_BILINEAR)
static float4
bufferSampleRectF4(const IMX_Layer* layer, float2 xy, float2 dxy,
                  BorderType border, StorageType storage, int channels)
{
    return bufferSampleF4(layer, xy, border, storage, channels);
}

#elif defined(FILTER_BOX)
static float4
bufferSampleRectF4(const IMX_Layer* layer, float2 xy, float2 dxy,
                  BorderType border, StorageType storage, int channels)
{
    xy = clamp(xy, -1e8f, 1e8f);
    dxy = clamp(dxy, 1.0f, 67.0f);
    float2 r = (dxy + 1.0f) / 2;
    int2 ia; float2 fa;
    _splitCoordinates(r - xy, &ia, &fa);
    ia = -ia;
    int2 ib; float2 fb;
    _splitCoordinates(xy + r, &ib, &fb);
    float4 sum = 0;
    int2 ixy;
    float w = fa.y;
    for (ixy.y = ia.y; ixy.y <= ib.y;)
    {
        ixy.x = ia.x;
        float4 sum1 = fa.x * bufferIndexF4(layer, ixy, border, storage, channels);
        for (ixy.x++; ixy.x < ib.x; ixy.x++)
            sum1 += bufferIndexF4(layer, ixy, border, storage, channels);
        sum1 += fb.x * bufferIndexF4(layer, ixy, border, storage, channels);
        sum += w * sum1;
        w = (++ixy.y) < ib.y ? 1.0f : fb.y;
    }
    return sum / (dxy.x * dxy.y);
}

#elif defined(FILTER_TRIANGLE)
static float4
bufferSampleRectF4(const IMX_Layer* layer, float2 xy, float2 dxy,
                  BorderType border, StorageType storage, int channels)
{
    float2 r = clamp(dxy, 1.0f, 1.0e8f);
    int2 ia = convert_int2_sat_rtp(xy - r);
    int2 ib = convert_int2_sat_rtp(xy + r);
    float4 sum = 0;
    float div = 0;
    int2 ixy;
    int2 inc = (ib - ia) / 61 + 1;
    for (ixy.y = ia.y; ixy.y < ib.y; ixy.y += inc.y)
    {
        float4 sum1 = 0;
        float div1 = 0;
        for (ixy.x = ia.x; ixy.x < ib.x; ixy.x += inc.x)
        {
            float w = 1 - fabs(ixy.x - xy.x) / r.x;
            sum1 += w * bufferIndexF4(layer, ixy, border, storage, channels);
            div1 += w;
        }
        float w = 1 - fabs(ixy.y - xy.y) / r.y;
        sum += w * sum1;
        div += w * div1;
    }
    return sum / div;
}

#elif defined(FILTER_CUBIC)
#define FILTER_SIZE 25
static __constant float Cubic[FILTER_SIZE] = { // mitchell(25, 0, 0.5)
    6, 5.9010415, 5.625, 5.203125, 4.6666665, 4.046875, 3.375, 2.6822917,
    2, 1.359375, 0.7916667, 0.328125, 0, -0.21006945, -0.3472222, -0.421875,
    -0.44444445, -0.4253472, -0.375, -0.30381945, -0.22222222, -0.140625, -0.06944445, -0.019097222,
    0};
#define FILTER Cubic
#define FILTER_SUPPORT 2

#elif defined(FILTER_MITCHELL)
#define FILTER_SIZE 25
static __constant float Mitchell[FILTER_SIZE] = { // mitchell(25, 1/3.0, 1/3.0)
    5.3333335, 5.2540507, 5.0324073, 4.6927085, 4.259259, 3.7563658, 3.2083333, 2.6394675,
    2.074074, 1.5364584, 1.050926, 0.6417824, 0.33333334, 0.116705246, -0.038580246, -0.140625,
    -0.19753087, -0.21739969, -0.20833333, -0.17843364, -0.13580246, -0.088541664, -0.044753086, -0.01253858,
    0}; //-1.7763568e-15};
#define FILTER Mitchell
#define FILTER_SUPPORT 2

#elif defined(FILTER_BSPLINE)
#define FILTER_SIZE 25
static __constant float BSpline[FILTER_SIZE] = { // mitchell(25, 1, 0)
    4, 3.9600694, 3.8472223, 3.671875, 3.4444444, 3.1753473, 2.875, 2.5538194,
    2.2222223, 1.890625, 1.5694444, 1.2690972, 1, 0.7702546, 0.5787037, 0.421875,
    0.2962963, 0.19849537, 0.125, 0.07233796, 0.037037037, 0.015625, 0.0046296297, 0.0005787037,
    0};
#define FILTER BSpline
#define FILTER_SUPPORT 2

#endif

#if defined(FILTER)

static float
sampleLookup(constant float * in, float pos)
{
    float flr;
    float t = fract(pos, &flr);
    int flooridx = convert_int(flr);
    int ceilidx = flooridx+1;
    ceilidx = min(ceilidx, FILTER_SIZE-1);
    return mix(in[flooridx], in[ceilidx], t);
}

static float4
bufferSampleRectF4(const IMX_Layer* layer, float2 xy, float2 dxy,
                  BorderType border, StorageType storage, int channels)
{
    float2 r = min(max(dxy, 1.0f) * FILTER_SUPPORT, 1.0e8f);
    int2 ia = convert_int2_sat_rtp(xy - r);
    int2 ib = convert_int2_sat_rtp(xy + r);
    float2 k = (FILTER_SIZE - 1) / r;
    float4 sum = 0;
    float div = 0;
    int2 ixy;
    int2 inc = (ib - ia) / 61 + 1;
    for (ixy.y = ia.y; ixy.y < ib.y; ixy.y += inc.y)
    {
        float4 sum1 = 0;
        float div1 = 0;
        for (ixy.x = ia.x; ixy.x < ib.x; ixy.x += inc.x)
        {
            float w = sampleLookup(FILTER, fabs(ixy.x - xy.x) * k.x);
            sum1 += w * bufferIndexF4(layer, ixy, border, storage, channels);
            div1 += w;
        }
        float w = sampleLookup(FILTER, fabs(ixy.y - xy.y) * k.y);
        sum += w * sum1;
        div += w * div1;
    }
    return sum / div;
}
#endif

// Intersects the incoming sampling window with the layer's footprint and
// returns the filtered value in that rectangle. (Thus, this should never read
// outside pixels.)
static float4
bufferSampleRectClipF4(const IMX_Layer* layer, float2 xy, float2 dxy,
                       StorageType storage, int channels)
{
#if defined(FILTER_POINT)
    return bufferSampleRectF4(layer, xy, dxy, IMX_CONSTANT, storage, channels);
#else
    float2 wh = convert_float2(layer->stat->resolution);
    dxy *= 0.5f;
    float2 xy0 = max(xy - dxy, -0.5f);
    float2 xy1 = min(xy + dxy, wh - 0.5f);
    if (any(xy0 >= xy1))
        return 0.0f;
    return bufferSampleRectF4(layer, (xy1 + xy0) * 0.5f, xy1 - xy0, IMX_CLAMP,
                              storage, channels);
#endif
}

static float4
constImageSampleRectClip(float2 xy, float2 dxy, float4 defval)
{
    // Clip to image space; if there is any intersection, return defval;
    // otherwise, return 0 since we're fully outside the image.
    float2 xy0 = max(xy - dxy, -1.0f);
    float2 xy1 = min(xy + dxy, 1.0f);
    return any(xy0 >= xy1) ? 0.0f : defval;
}

// convert derivatives (or a parallelogram) to nearest ortho rectangle
static float2
wh_from_dP(float2 dPdx, float2 dPdy)
{
    return hypot(dPdx, dPdy);
}

/* Python to generate the filters
def mitchell(n, B, C):
    print("[%d] = {"%n, end='');
    for i in range(0,n):
        if i: print(",", end='')
        if not(i%8): print("\n   ", end='')
        x = (2.0*i)/(n-1)
        if (x < 1):
            v = ((12-9*B-6*C)*x-18+12*B+6*C)*x*x+6-2*B;
        else:
            v = (((-B-6*C)*x+6*B+30*C)*x-12*B-48*C)*x+8*B+24*C;
        print(" %.6g"%v, end='')
    print("};")

mitchell(25, 1/3.0, 1/3.0)
*/
