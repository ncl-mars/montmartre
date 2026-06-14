#ifndef COLORS_CL
#define COLORS_CL

#include "./utils.cl"     // frac
#include "./remapping.cl" // saturate

// Color utilities. All colors are float3 RGB.
// Houdini's Cd is LINEAR, so unless noted these expect/return linear color.
// Use srgbToLinear / linearToSrgb to cross the boundary.

#ifndef PI
#define PI 3.1415926535897932f
#endif
#ifndef TAU
#define TAU 6.2831853071795864f
#endif


//------------------------------------------------------------- Luminance
float luminance(float3 c){ return dot(c, (float3)(0.2126f, 0.7152f, 0.0722f)); } // Rec.709, LINEAR
float luma(float3 c){      return dot(c, (float3)(0.299f,  0.587f,  0.114f)); }  // Rec.601, GAMMA/sRGB
float3 grayscale(float3 c){ return (float3)(luminance(c)); }


//------------------------------------------------------------- Transfer (sRGB / gamma)
// sRGB transfer function - exact piecewise
float3 srgbToLinear(float3 c){
    float3 lo = c / 12.92f;
    float3 hi = pow((c + 0.055f) / 1.055f, (float3)(2.4f));
    return mix(lo, hi, step((float3)(0.04045f), c));
}
float3 linearToSrgb(float3 c){
    float3 lo = c * 12.92f;
    float3 hi = 1.055f * pow(max(c, 0.0f), (float3)(1.0f / 2.4f)) - 0.055f;
    return mix(lo, hi, step((float3)(0.0031308f), c));
}
// cheap power-curve gamma (g=2.2 approximates sRGB)
float3 gammaEncode(float3 c, float g){ return pow(max(c, 0.0f), (float3)(1.0f / g)); }
float3 gammaDecode(float3 c, float g){ return pow(max(c, 0.0f), (float3)(g)); }


//------------------------------------------------------------- HSV / HSL
// RGB <-> HSV - Sam Hocevar / IQ branchless
float3 rgb2hsv(float3 c){
    float4 K = (float4)(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
    float4 p = mix((float4)(c.bg, K.wz), (float4)(c.gb, K.xy), step(c.b, c.g));
    float4 q = mix((float4)(p.xyw, c.r), (float4)(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10f;
    return (float3)(fabs(q.z + (q.w - q.y) / (6.0f * d + e)), d / (q.x + e), q.x);
}
// HSV -> RGB - IQ, with smoothstep for softer ramps
float3 hsv2rgb(float3 c){
    float3 rgb = clamp(fabs(fmod(c.x * 6.0f + (float3)(0.0f, 4.0f, 2.0f), (float3)(6.0f)) - 3.0f) - 1.0f, 0.0f, 1.0f);
    rgb = rgb * rgb * (3.0f - 2.0f * rgb);
    return c.z * mix((float3)(1.0f), rgb, c.y);
}

float3 rgb2hsl(float3 c){
    float mx = max(c.r, max(c.g, c.b));
    float mn = min(c.r, min(c.g, c.b));
    float l = (mx + mn) * 0.5f;
    float h = 0.0f, s = 0.0f;
    float d = mx - mn;
    if (d > 1.0e-6f){
        s = (l > 0.5f) ? d / (2.0f - mx - mn) : d / (mx + mn);
        if      (mx == c.r) h = (c.g - c.b) / d + (c.g < c.b ? 6.0f : 0.0f);
        else if (mx == c.g) h = (c.b - c.r) / d + 2.0f;
        else                h = (c.r - c.g) / d + 4.0f;
        h /= 6.0f;
    }
    return (float3)(h, s, l);
}
float hue2rgb(float p, float q, float t){
    t = frac(t);
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}
float3 hsl2rgb(float3 c){
    if (c.y < 1.0e-6f) return (float3)(c.z);
    float q = (c.z < 0.5f) ? c.z * (1.0f + c.y) : c.z + c.y - c.z * c.y;
    float p = 2.0f * c.z - q;
    return (float3)(hue2rgb(p, q, c.x + 1.0f / 3.0f),
                    hue2rgb(p, q, c.x),
                    hue2rgb(p, q, c.x - 1.0f / 3.0f));
}


//------------------------------------------------------------- Grading
float3 brightness(float3 c, float b){ return c * b; }
float3 exposure(float3 c, float ev){ return c * exp2(ev); } // in stops

float3 contrast(float3 c, float k){ return (c - 0.5f) * k + 0.5f; }                  // pivot 0.5
float3 contrastPivot(float3 c, float k, float pivot){ return (c - pivot) * k + pivot; }

float3 saturation(float3 c, float s){ return mix((float3)(luminance(c)), c, s); }    // 0 gray, 1 unchanged

// pushes dull colors more than vivid ones
float3 vibrance(float3 c, float v){
    float mx  = max(c.r, max(c.g, c.b));
    float mn  = min(c.r, min(c.g, c.b));
    float sat = mx - mn;
    return mix((float3)(luminance(c)), c, 1.0f + v * (1.0f - sat));
}

// angle in turns (0..1 == 0..360 deg)
float3 hueShift(float3 c, float angle){
    float3 hsv = rgb2hsv(c);
    hsv.x = frac(hsv.x + angle);
    return hsv2rgb(hsv);
}

// photoshop-style levels; gamma>1 brightens mids
float3 levels(float3 c, float inB, float inW, float gamma, float outB, float outW){
    c = saturate((c - inB) / (inW - inB));
    c = pow(c, (float3)(1.0f / gamma));
    return c * (outW - outB) + outB;
}
float3 posterize(float3 c, float steps){ return floor(c * steps) / steps; }


//------------------------------------------------------------- Palettes (IQ cosine)
// https://iquilezles.org/articles/palettes/
//   rgb(t) = a + b * cos( 2pi * (c*t + d) )
float3 palette(float t, float3 a, float3 b, float3 c, float3 d){
    return a + b * cos(TAU * (c * t + d));
}
float3 paletteRainbow(float t){
    return palette(t, (float3)(0.5f), (float3)(0.5f), (float3)(1.0f), (float3)(0.0f, 0.33f, 0.67f));
}
float3 paletteWarm(float t){
    return palette(t, (float3)(0.5f), (float3)(0.5f), (float3)(1.0f), (float3)(0.0f, 0.10f, 0.20f));
}
float3 paletteSpectrum(float t){
    return palette(t, (float3)(0.5f), (float3)(0.5f), (float3)(2.0f, 1.0f, 0.0f), (float3)(0.5f, 0.20f, 0.25f));
}


//------------------------------------------------------------- Heatmaps / colormaps (t in 0..1)
// Classic "jet" (blue->cyan->green->yellow->red). Cheap, not perceptual.
float3 heatmapJet(float t){
    t = clamp(t, 0.0f, 1.0f);
    return clamp((float3)(1.5f - fabs(4.0f * t - 3.0f),
                          1.5f - fabs(4.0f * t - 2.0f),
                          1.5f - fabs(4.0f * t - 1.0f)), 0.0f, 1.0f);
}
// Google Turbo - perceptual rainbow, polynomial fit by Anton Mikhailov
float3 turbo(float t){
    t = clamp(t, 0.0f, 1.0f);
    float4 kR = (float4)(0.13572138f,  4.61539260f, -42.66032258f, 132.13108234f);
    float4 kG = (float4)(0.09140261f,  2.19418839f,   4.84296658f, -14.18503333f);
    float4 kB = (float4)(0.10667330f, 12.64194608f, -60.58204836f, 110.36276771f);
    float2 kR2 = (float2)(-152.94239396f, 59.28637943f);
    float2 kG2 = (float2)(   4.27729857f,  2.82956604f);
    float2 kB2 = (float2)( -89.90310912f, 27.34824973f);
    float4 v4 = (float4)(1.0f, t, t * t, t * t * t);
    float2 v2 = v4.zw * v4.z; // (t^4, t^5)
    return (float3)(dot(v4, kR) + dot(v2, kR2),
                    dot(v4, kG) + dot(v2, kG2),
                    dot(v4, kB) + dot(v2, kB2));
}
// matplotlib colormaps - degree-6 fits by @mattz, perceptually uniform
// https://www.shadertoy.com/view/WlfXRN
float3 viridis(float t){
    t = clamp(t, 0.0f, 1.0f);
    const float3 c0 = (float3)( 0.2777273272234177f,   0.005407344544966578f, 0.3340998053353061f);
    const float3 c1 = (float3)( 0.1050930431085774f,   1.404613529898575f,    1.384590162594685f);
    const float3 c2 = (float3)(-0.3308618287255563f,   0.214847559468213f,    0.09509516302823659f);
    const float3 c3 = (float3)(-4.634230498983486f,   -5.799100973351585f,  -19.33244095627987f);
    const float3 c4 = (float3)( 6.228269936347081f,   14.17993336680509f,    56.69055260068105f);
    const float3 c5 = (float3)( 4.776384997670288f,  -13.74514537774601f,   -65.35303263337234f);
    const float3 c6 = (float3)(-5.435455855934631f,    4.645852612178535f,    26.3124352495832f);
    return c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
}
float3 inferno(float t){
    t = clamp(t, 0.0f, 1.0f);
    const float3 c0 = (float3)( 0.0002189403691192265f, 0.001651004631001012f, -0.01948089843709184f);
    const float3 c1 = (float3)( 0.1065134194856116f,    0.5639564367884091f,    3.932712388889277f);
    const float3 c2 = (float3)(11.60249308247187f,     -3.972853965665698f,   -15.9423941062914f);
    const float3 c3 = (float3)(-41.70399613139459f,    17.43639888205313f,     44.35414519872813f);
    const float3 c4 = (float3)(77.162935699427f,      -33.40235894210092f,    -81.80730925738993f);
    const float3 c5 = (float3)(-71.31942824499214f,    32.62606426397723f,     73.20951985803202f);
    const float3 c6 = (float3)(25.13112622477341f,    -12.24266895238567f,    -23.07032500287172f);
    return c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
}
float3 magma(float t){
    t = clamp(t, 0.0f, 1.0f);
    const float3 c0 = (float3)(-0.002136485053939582f, -0.000749655052795221f, -0.005386127855323933f);
    const float3 c1 = (float3)( 0.2516605407371642f,    0.6775232436837668f,    2.494026599312351f);
    const float3 c2 = (float3)( 8.353717279216625f,    -3.577719514958484f,     0.3144679030132573f);
    const float3 c3 = (float3)(-27.66873308576866f,    14.26473078096533f,    -13.64921318813922f);
    const float3 c4 = (float3)(52.17613981234068f,    -27.94360607168351f,     12.94416944238394f);
    const float3 c5 = (float3)(-50.76852536473588f,    29.04658282127291f,      4.23415299384598f);
    const float3 c6 = (float3)(18.65570506591883f,    -11.48977351997711f,     -5.601961508734096f);
    return c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
}
float3 plasma(float t){
    t = clamp(t, 0.0f, 1.0f);
    const float3 c0 = (float3)( 0.05873234392399702f, 0.02333670892565664f,  0.5433401826748754f);
    const float3 c1 = (float3)( 2.176514634195958f,   0.2383834171260182f,   0.7539604599784036f);
    const float3 c2 = (float3)(-2.689460476458034f,  -7.455851135738909f,    3.110799939717086f);
    const float3 c3 = (float3)( 6.130348345893603f,  42.3461881477227f,    -28.51885465332158f);
    const float3 c4 = (float3)(-11.10743619062271f, -82.66631109428045f,    60.13984767418263f);
    const float3 c5 = (float3)(10.02306557647065f,   71.41361770095349f,   -54.07218655560067f);
    const float3 c6 = (float3)(-3.658713842777788f, -22.93153465461149f,    18.19190778539828f);
    return c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
}


//------------------------------------------------------------- Tonemapping (HDR linear -> LDR)
float3 tonemapReinhard(float3 c){ return c / (1.0f + c); }
// extended: preserves highlights up to `white`
float3 tonemapReinhardExt(float3 c, float white){
    return (c * (1.0f + c / (white * white))) / (1.0f + c);
}
// ACES filmic - Krzysztof Narkowicz fast approximation
float3 tonemapACES(float3 x){
    float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}
// ACES filmic - Stephen Hill fit (more accurate, with RRT/ODT matrices)
float3 acesInput(float3 v){
    return (float3)(dot((float3)(0.59719f, 0.35458f, 0.04823f), v),
                    dot((float3)(0.07600f, 0.90834f, 0.01566f), v),
                    dot((float3)(0.02840f, 0.13383f, 0.83777f), v));
}
float3 acesOutput(float3 v){
    return (float3)(dot((float3)( 1.60475f, -0.53108f, -0.07367f), v),
                    dot((float3)(-0.10208f,  1.10813f, -0.00605f), v),
                    dot((float3)(-0.00327f, -0.07276f,  1.07602f), v));
}
float3 tonemapACESFitted(float3 c){
    c = acesInput(c);
    float3 a = c * (c + 0.0245786f) - 0.000090537f;
    float3 b = c * (0.983729f * c + 0.4329510f) + 0.238081f;
    c = a / b;
    return saturate(acesOutput(c));
}
// Uncharted 2 filmic - John Hable
float3 uncharted2Partial(float3 x){
    float A = 0.15f, B = 0.50f, C = 0.10f, D = 0.20f, E = 0.02f, F = 0.30f;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
float3 tonemapUncharted2(float3 c, float exposure){
    const float W = 11.2f;
    float3 curr = uncharted2Partial(c * exposure);
    float3 whiteScale = 1.0f / uncharted2Partial((float3)(W));
    return curr * whiteScale;
}
// Unreal 3 - Romain Guy. Bakes an sRGB-ish curve, output is display-ready.
float3 tonemapUnreal(float3 x){ return x / (x + 0.155f) * 1.019f; }


#endif
