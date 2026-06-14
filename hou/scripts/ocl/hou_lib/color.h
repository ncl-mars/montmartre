#ifndef __COLOR_H
#define __COLOR_H


/******************************************************************************
 * HDK-consistent colour transform functions
 ******************************************************************************/

#define COLOR_MIN_SAT (0.5f / 255.0f)

static float3
VEXrgbtohsv(float3 rgb)
{
    float3 hsv;

    float mx, mn;
    mx = max(rgb.x, max(rgb.y, rgb.z));
    mn = min(rgb.x, min(rgb.y, rgb.z));
    float s1 = mx - mn;
    hsv.y = (mx > 0) ? (mx - mn) / mx : 0.0f;
    hsv.z = mx;

    if (hsv.y < COLOR_MIN_SAT)
    {
        hsv.x = 0.0f;
        return hsv;
    }

    float3 rgbs = (mx - rgb) / s1;

    float  hue = 0;

         if (mx == rgb.x) hue = (mn == rgb.y) ? 5.0f + rgbs.z : 1.0f - rgbs.y;
    else if (mx == rgb.y) hue = (mn == rgb.z) ? 1.0f + rgbs.x : 3.0f - rgbs.z;
    else                  hue = (mn == rgb.x) ? 3.0f + rgbs.y : 5.0f - rgbs.x;

    hue *= 60.0f;
    if (hue >= 360.0f) hue -= 360.0f;

    hsv.x = hue / 360.0f;
    return hsv;
}

static float3
VEXhsvtorgb(float3 hsv)
{
    int         i;
    float       f, p, q, t;
    float3      rgb;

    if (hsv.y <= COLOR_MIN_SAT)
    {
        rgb = hsv.z;
        return rgb;
    }

    // Find hextant
    float       h = hsv.x * 6.0f;
    i = floor(h);
    f = h - i;
    i = i % 6; if (i < 0) i += 6;

    hsv.y = min(hsv.y, 1.0f);

    float v = hsv.z;
    float s = hsv.y;

    p = v * (1.0f - s);
    q = v * (1.0f - s * f);
    t = v * (1.0f - (s * (1.0f - f)));

    switch (i)
    {
        case 0: rgb = (float3)(v, t, p); break;
        case 1: rgb = (float3)(q, v, p); break;
        case 2: rgb = (float3)(p, v, t); break;
        case 3: rgb = (float3)(p, q, v); break;
        case 4: rgb = (float3)(t, p, v); break;
        case 5: rgb = (float3)(v, p, q); break;
    }

    return rgb;
}

// Return NTSC or BT 601 luma
// Matches vex functon of same name.
static float
VEXluminance(float3 rgb)
{
    return 0.299f * rgb.x + 0.587f * rgb.y + 0.114f * rgb.z;
}

// Returns SGI luma
static float
VEXluminanceSGI(float3 rgb)
{
    return 0.3086f * rgb.x + 0.6094f * rgb.y + 0.0820f * rgb.z;
}

// Returns BT 709 luma
static float
VEXluminance709(float3 rgb)
{
    return 0.2126f * rgb.x + 0.7152f * rgb.y + 0.0722f * rgb.z;
}

#endif

