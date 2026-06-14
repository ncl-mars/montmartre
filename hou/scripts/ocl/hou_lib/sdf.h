/*
 * PROPRIETARY INFORMATION.  This software is proprietary to
 * Side Effects Software Inc., and is not to be reproduced,
 * transmitted, or disclosed in any way without written permission.
 *
 * Produced by:
 *  Side Effects Software Inc
 *  123 Front Street West, Suite 1401
 *  Toronto, Ontario
 *  Canada   M5J 2M2
 *  416-504-9876
 *
 * NAME:    sdf.h
 *
 * COMMENTS: 2D SDF functions used by the pattern generators in COP.
 */

#ifndef __SDF_H__
#define __SDF_H__


// Utility Functions

// Returns the squared distance of the float2 vector.
float length2(float2 _v)
{
    return dot(_v, _v);
}

float ndot(float2 a, float2 b)
{
    return a.x * b.x - a.y * b.y;
}

// Shape declarations

float
sdfLine(float2 pos, float scale, float2 a, float2 b)
{
    a *= scale;
    b *= scale;

    float2 pa = pos - a;
    float2 ba = b - a;
    float h = clamp(dot(pa, ba)/dot(ba, ba), 0.0f, 1.0f);
    return length(pa - ba * h) - 0.001;
}
float
sdfCircle(float2 pos, float r)
{
    return length(pos) - r;
}

float
sdfCutCircle(float2 pos, float scale, float r, float h)
{
    r *= scale;
    h *= scale;

    if(h <= -r) return sdfCircle(pos, r);

    pos.x = fabs(pos.x);

    float w = sqrt(r * r - h * h); // constant for any given shape
    float s = max((h - r) * pos.x * pos.x + w * w * (h + r - 2.0f * pos.y), h * pos.x - w * pos.y);
    return (s < 0.0f) ? length(pos) - r :
           (pos.x < w) ? h - pos.y :
                     length(pos - (float2)(w, h));
}

float
sdfPie(float2 pos, float r, float tb)
{
    if(tb == 1.0f) return sdfCircle(pos, r);

    pos = rotate2D(pos, 90.0f);
    pos.x = fabs(pos.x);

    tb = clamp(tb, 0.0f, 1.0f);
    tb *= M_PI_F;

    // sc is the sin/cos of the arc's aperture
    float2 sc = (float2)(sin(tb), cos(tb));

    float l = length(pos) - r;
    float m = length(pos - sc * clamp(dot(pos, sc), 0.0f, r)); // c=sin/cos of aperture
    return max(l, m * sign(sc.y * pos.x - sc.x * pos.y));
}


float
sdfArc(float2 pos, float scale, float ra, float th, float tb)
{
    ra *= scale;
    th *= scale;

    ra -= th * 0.5;
    pos = rotate2D(pos, 90.0f);
    pos.x = fabs(pos.x);

    tb = clamp(tb, 0.0f, 1.0f);
    tb *= M_PI_F;
    // sc is the sin/cos of the arc's aperture
    float2 sc = (float2)(sin(tb), cos(tb));


    return ((sc.y * pos.x > sc.x * pos.y) ? length(pos - sc * ra) :
                                  fabs(length(pos) - ra)) - th * 0.5;
}

float
sdfRing(float2 pos, float scale, float r, float th, float arc)
{
    r *= scale;
    th *= scale;

    r -= th * 0.5;
    pos = rotate2D(pos, 90.0f);
    pos.x = fabs(pos.x);

    arc = clamp(arc, 0.0f, 1.0f);
    arc *= M_PI_F;
    // sc is the cos/sin of the arc's aperture
    float2 sc = (float2)(cos(arc), sin(arc));

    mat2 xform = (mat2)(sc.x, sc.y, -sc.y, sc.x);
    pos = mat2vecmul(xform, pos);

    return max(fabs(length(pos) - r) - th * 0.5f,
               length((float2)(pos.x, max(0.0f, fabs(r - pos.y) - th * 0.5f))) * sign(pos.x));
}

float
sdfEllipse(float2 pos, float scale, float2 ab)
{
    ab *= scale;

    if(ab.x == ab.y) return length(pos) - ab.x;

    pos = fabs(pos);

    if(pos.x > pos.y) {
        pos = (float2)(pos.y, pos.x);
        ab = (float2)(ab.y, ab.x);
    }
    float l = ab.y * ab.y - ab.x * ab.x;
    float m = ab.x * pos.x / l;
    float n = ab.y * pos.y / l;
    float m2 = m * m;
    float n2 = n * n;
    float c = (m2 + n2 - 1.0f) / 3.0f;
    float c3 = c * c * c;

    float q = c3 + m2 * n2 * 2.0f;
    float d = c3 + m2 * n2;
    float g = m + m * n2;

    float co;

    if(d < 0.0f)
    {
        float h = acos(q / c3) / 3.0f;
        float s = cos(h);
        float t = sin(h) * sqrt(3.0f);
        float rx = sqrt(-c * (s + t + 2.0f) + m2);
        float ry = sqrt(-c * (s - t + 2.0f) + m2);
        co = ry + sign(l) * rx + fabs(g) / (rx * ry);
    }
    else
    {
        float h = 2.0f * m * n * sqrt(d);
        float s = sign(q + h) * pow(fabs(q + h), 1.0f / 3.0f);
        float u = sign(q-h) * pow(fabs(q - h), 1.0f / 3.0f);
        float rx = -s - u - c * 4.0f + 2.0f * m2;
        float ry = (s - u) * sqrt(3.0f);
        float rm = sqrt(rx * rx + ry * ry);
        co = ry / sqrt(rm - rx) + 2.0 * g / rm;
    }
    co = (co-m) / 2.0f;

    float si = sqrt(max(1.0f - co * co , 0.0f));
    float2 r = ab * (float2)(co, si);
    return length(r - pos) * sign(pos.y - r.y);
}

float
sdfQuadraticCircle(float2 pos, float r)
{
    if (r == 0.0f) return sdfCircle(pos, r);

    pos /= r;
    pos = fabs(pos);
    if(pos.y > pos.x ) pos = (float2)(pos.y, pos.x);

    float a = pos.x - pos.y;
    float b = pos.x + pos.y;
    float c = (2.0f * b - 1.0f) / 3.0f;
    float h = a * a + c * c * c;

    float t = 0.0f;
    if( h >= 0.0f )
    {
        h = sqrt(h);
        t = sign(h - a) * pow(fabs(h - a), 1.0f / 3.0f) - pow(h + a, 1.0f / 3.0f);
    }
    else
    {
        float z = sqrt(-c);
        float v = acos(a / (c * z)) / 3.0f;
        t = -z *(cos(v) + sin(v) * 1.732050808f);
    }
    t *= 0.5f;
    float k = 0.75 - t * t;
    float2 w = (float2)(-t + k, t + k) - pos;
    return length(w * r) * sign( a * a * 0.5f + b - 1.5f);
}

float sdfSquircle(float2 pos, float r, float n)
{
    pos = pos / r;
    float2 gs = sign(pos);
    float2 ps = fabs(pos);
    float gm = pow(ps.x, n) + pow(ps.y, n);
    //float gd = pow(gm, 1.0f / n) - r;
    //float2 g = gs * pow(ps, (float2)(n - 1.0f)) * pow(gm, 1.0f / n - 1.0f);
    pos = fabs(pos);
    if (pos.y > pos.x) pos = pos.yx;
    n = 2.0f / n;
    float s = 1.0f;
    float d = 1e20f;
    const int num = 24;
    float2 oq = (float2)(1.0f, 0.0f);
    for (int i = 1; i < num; i++)
    {
        float h = (float)(i) / (float)(num-1);
        float2 q = (float2)(pow(cos(h * M_PI_F / 4.0f), n),
                      pow(sin(h * M_PI_F / 4.0f), n));
        float2  pa = pos - oq;
        float2  ba = q - oq;

        float2  z = pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0f, 1.0f);
        float d2 = dot(z, z);
        if (d2 < d) {
            d = d2;
            s = pa.x * ba.y - pa.y * ba.x;
        }
        oq = q;
    }
    return sqrt(d) * sign(s) * r;
}

float sdfRectangle(float2 pos, float scale, float2 size)
{
    size *= scale * 0.5f;

    float2 d = fabs(pos) - size;
    return length(max(d, 0.0f)) + min(max(d.x, d.y),0.0f);
}

float sdfRoundedRectangle(float2 pos, float scale, float2 size, float4 r)
{
    size *= scale * 0.5f;
    r *= scale;

    r.x = (pos.x > 0.0f) ? r.x : r.z;
    r.y = (pos.x > 0.0f) ? r.y : r.w;
    r.x = (pos.y > 0.0f) ? r.x : r.y;

    float2 d = fabs(pos) - size + (float2)(r.x, r.x);
    return length(max(d, 0.0f)) + min(max(d.x, d.y), 0.0f) - r.x;
}

float sdfDiamond(float2 pos, float scale, float2 b)
{
    b *= scale;

    pos = fabs(pos);
    float h = clamp(ndot(b - (float2)(2.0f, 2.0f) * pos, b)/dot(b, b), -1.0f, 1.0f);
    float d = length(pos - (float2)(0.5f, 0.5f) * b * (float2)(1.0f - h,1.0f + h));
    return d * sign(pos.x * b.y + pos.y * b.x - b.x * b.y);
}

float sdfTrapezoid(float2 pos, float scale, float he, float r1, float r2)
{
    he *= scale * 0.5f;
    r1 *= scale * 0.5f;
    r2 *= scale * 0.5f;

    pos.x = fabs(pos.x);
    float2 k1 = (float2)(r2, he);
    float2 k2 = (float2)(r2 -r1, he * 2.0f);
    float2 ca = (float2)(pos.x - min(pos.x,(pos.y < 0.0f) ? r1 : r2), fabs(pos.y) - he);
    float2 cb = pos - k1 + k2 * clamp(dot(k1 - pos, k2) / length2(k2), 0.0f, 1.0f);
    float s = (cb.x < 0.0f && ca.y < 0.0f) ? -1.0f : 1.0f;
    return sqrt(min(length2(ca), length2(cb))) * s;
}

float sdfParallelogram(float2 pos, float scale, float wi, float he, float sk)
{
    wi *= scale;
    he *= scale;
    sk *= scale;

    pos = (pos.y < 0.0f) ? -pos : pos;
    float2 e = (float2)(sk, he);
    float2  w = pos - e;
    w.x -= clamp(w.x, -wi, wi);
    float2  d = (float2)(dot(w, w), -w.y);
    float s = pos.x * e.y - pos.y * e.x;
    pos = (s < 0.0f) ? -pos : pos;
    float2 v = pos - (float2)(wi, 0.0f);
    v -= e * clamp(dot(v,e) / dot(e,e), -1.0f, 1.0f);
    d = min(d, (float2)(dot(v,v), wi * he - fabs(s)));
    return sqrt(d.x) * sign(-d.y);
}

float sdfTriangle(float2 pos, float scale, float2 p0, float2 p1, float2 p2)
{
    if (scale == 0.0f) return sdfCircle(pos, 0.0f);
    p0 *= scale;
    p1 *= scale;
    p2 *= scale;

    float2 e0 = p1 - p0;
    float2 e1 = p2 - p1;
    float2 e2 = p0 - p2;
    float2 v0 = pos - p0;
    float2 v1 = pos - p1;
    float2 v2 = pos - p2;
    float2 pq0 = v0 - e0 * clamp(dot(v0, e0) / dot(e0, e0), 0.0f, 1.0f);
    float2 pq1 = v1 - e1 * clamp(dot(v1, e1) / dot(e1, e1), 0.0f, 1.0f);
    float2 pq2 = v2 - e2 * clamp(dot(v2, e2) / dot(e2, e2), 0.0f, 1.0f);
    float s = sign(e0.x * e2.y - e0.y * e2.x);
    float2 d = min(min((float2)(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),
                       (float2)(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))),
                       (float2)(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x)));
    return -sqrt(d.x)*sign(d.y);
}

float sdfEquilateralTriangle(float2 pos, float r)
{
    const float k = sqrt(3.0f);
    pos.x = fabs(pos.x) - r;
    pos.y = pos.y + r / k;
    if(pos.x + k * pos.y > 0.0f) {
        pos = (float2)(pos.x - k * pos.y, -k * pos.x - pos.y) / 2.0f;
    }
    pos.x -= clamp(pos.x, -2.0f * r, 0.0f);
    return -length(pos)*sign(pos.y);
}

float sdfTriangleIsosceles(float2 pos, float scale, float qw, float qh)
{
    qw *= scale;
    qh *= scale;

    if (qh == 0.0) return sdfLine(pos, scale, (float2)(-qw, 0.0f), (float2)(qw, 0.0f));
    float2 _q = (float2)(qw, qh);
    pos = rotate2D(pos, 180.0f);
    pos.y += _q.y * 0.5;
    pos.x = fabs(pos.x);
    float2 a = pos - _q * clamp(dot(pos, _q) / dot(_q, _q), 0.0f, 1.0f);
    float2 b = pos - _q * (float2)(clamp(pos.x / _q.x, 0.0f, 1.0f), 1.0f);
    float s = -sign(_q.y);
    float2 d = min((float2)(dot(a, a), s * (pos.x * _q.y - pos.y * _q.x)),
                   (float2)(dot(b, b), s * (pos.y - _q.y)));
    return -sqrt(d.x)*sign(d.y);
}

float sdfCapsule(float2 pos, float scale, float h, float r1, float r2)
{
    h *= scale;
    r1 *= scale;
    r2 *= scale;

    if (h == 0.0f) return sdfCircle(pos, min(r1, r2));
    pos.x = fabs(pos.x);
    pos.y += h * 0.5f;
    float b = (r1 - r2) / h;
    float a = sqrt(1.0f - b * b);
    float k = dot(pos, (float2)(-b, a));
    if(k < 0.0f) return length(pos) - r1;
    if(k > a * h) return length(pos - (float2)(0.0f, h)) - r2;
    return dot(pos, (float2)(a, b)) - r1;
}

float sdfDogBone(float2 pos, float scale, float w, float r, float b)
{
    if(fabs(b) < 1e-5) { // prevent division by 0
        pos = rotate2D(pos, 90);
        return sdfCapsule(pos, scale, w, r, r);
    }

    w *= scale;
    r *= scale;

    b = -b;
    w *= 0.5;

    pos = fabs(pos);
    float y  = (w * w - r * r)/(2.0f * r * b);
    float h  = length((float2)(w, y));
    float dE = length(pos-(float2)(w, 0.0f));
    float dC = length(pos-(float2)(0.0f, y)) - h;
    float sb = sign(b);
    return (sb * (w * pos.y + y * (pos.x - w)) > 0.0f ? dE : -sb*dC) - r;
}

float sdfArchway(float2 pos, float scale, float2 wh)
{
    wh *= scale;
    pos.x = fabs(pos.x);
    pos.y = -pos.y;
    float2 q = pos - wh;

    float d1 = length2((float2)(max(q.x, 0.0f), q.y));
    q.x = (pos.y > 0.0f) ? q.x : length(pos) - wh.x;
    float d2 = length2((float2)(q.x, max(q.y, 0.0f)));
    float d = sqrt(min(d1, d2));

    return (max(q.x, q.y) < 0.0f) ? -d : d;
}

float sdfCompoundArrow(float2 pos, float scale, float length, int use_points, float2 pt0, float2 pt1,
                      float shaft_thickness, float head_thickness, float head_ratio) {
    if (use_points) {
        // Transform coordinates to aling line between pt0 and pt1
        length = (distance(pt0, pt1));
        float2 center = (pt0 + pt1) * 0.5f;
        pos -= center;
        float2 dir = pt1 - pt0;
        float angle = degrees(atan2(dir.y, dir.x));
        pos = rotate2D(pos, angle);
    }

    float head_length = length * clamp(head_ratio, 0.0f, 1.0f);
    float shaft_length = length - head_length;

    float r = sdfRectangle(pos - (float2)(-head_length * 0.5f, 0.0f), scale, (float2)(shaft_length, shaft_thickness));
    float t = sdfTriangleIsosceles(rotate2D(pos, -90.0f) - (float2)(0.0f, shaft_length * 0.5f), scale, head_thickness, head_length);
    return min(t,r);
}

float sdfCircleWave(float2 pos, float scale, float tb, float ra)
{
    // negative values break the shape, revert to positive values only.
    //ra = max(0.00001f, ra);

    //tb *= scale;
    ra *= scale;

    pos.x = fabs(pos.x);
    tb = M_PI_F * 5.0f / 6.0f * max(tb, 0.0001f);
    float2 co = ra * (float2)(sin(tb), cos(tb));
    pos.x = fabs(fmod(pos.x, co.x * 4.0f) - co.x * 2.0f);

    float2 p1 = pos;
    float2 p2 = (float2)(fabs(pos.x - 2.0f * co.x), -pos.y + 2.0f * co.y);
    float d1 = ((co.y * p1.x > co.x * p1.y) ? length(p1 - co) : fabs(length(p1) - ra));
    float d2 = ((co.y * p2.x > co.x * p2.y) ? length(p2 - co) : fabs(length(p2) - ra));

    return min(d1, d2);
}

float sdfSpiral(float2 pos, float scale, float w, float h, int n)
{
    //h *= scale;
    w *= scale;

    // base point
    pos.x += w;
    float d = length(pos);
    // 8 arcs
    for(int i = 0; i < n; i++)
    {
        pos.x -= w;
        if(pos.x < 0.0f && pos.y > 0.0f) d = min(d, fabs(length(pos) - w));
        pos.y -= w;
        pos = (float2)(-pos.y, pos.x);
        w *= h;
    }
    // tip point
    return min(d, length(pos));
}

float sdfPentagon(float2 pos, float r)
{
    pos.x = fabs(pos.x);
    pos.y = -pos.y;

    const float3 k = (float3)(0.809016994, 0.587785252, 0.726542528);
    pos -= 2.0f * min(dot((float2)(-k.x, k.y), pos), 0.0f) * (float2)(-k.x, k.y);
    pos -= 2.0f * min(dot((float2)( k.x, k.y), pos), 0.0f) * (float2)(k.x, k.y);
    pos -= (float2)(clamp(pos.x, -r * k.z, r * k.z), r);
    return length(pos) * sign(pos.y);
}

float sdfHexagon(float2 pos, float r)
{
    const float3 k = (float3)(-0.866025404, 0.5, 0.577350269);

    pos = fabs(pos);
    pos -= 2.0f * min(dot(k.xy, pos), 0.0f) * k.xy;
    pos -= (float2)(clamp(pos.x, -k.z * r, k.z * r), r);
    return length(pos) * sign(pos.y);
}

float sdfOctogon(float2 pos, float r)
{
    const float3 k = (float3)(-0.9238795325, 0.3826834323, 0.4142135623);

    pos = fabs(pos);
    pos -= 2.0f * min(dot((float2)(k.x, k.y), pos), 0.0f) * (float2)(k.x, k.y);
    pos -= 2.0f * min(dot((float2)(-k.x, k.y), pos), 0.0f) * (float2)(-k.x, k.y);
    pos -= (float2)(clamp(pos.x, -k.z * r, k.z * r), r);
    return length(pos) * sign(pos.y);
}

float sdfHexagram(float2 pos, float r)
{
    const float4 k = (float4)(-0.5, 0.8660254038, 0.5773502692, 1.7320508076);

    pos = fabs(pos);
    pos -= 2.0f * min(dot(k.xy, pos), 0.0f) * k.xy;
    pos -= 2.0f * min(dot(k.yx, pos), 0.0f) * k.yx;
    pos -= (float2)(clamp(pos.x, r * k.z, r * k.w), r);
    return length(pos) * sign(pos.y);
}

float sdfStar(float2 pos, float r, int _n, float _m)
{
    pos.x = fabs(pos.x);

    // next 4 lines can be precomputed for a given shape
    float an = 3.141593 / (float)(_n);
    float en = 3.141593 / _m;  // m is between 2 and n
    float2 acs = (float2)(cos(an), sin(an));
    float2 ecs = (float2)(cos(en), sin(en)); // ecs=vec2(0,1) for regular polygon

    float bn = fmod(atan2(pos.x, pos.y), 2.0f * an) - an;
    pos = length(pos) * (float2)(cos(bn), fabs(sin(bn)));
    pos -= r * acs;
    pos += ecs * clamp(-dot(pos, ecs), 0.0f, r * acs.y / ecs.y);
    return length(pos) * sign(pos.x);
}

float sdfVesicaSegment(float2 pos, float2 a, float2 b, float w)
{
    // shape constants
    float r = 0.5f * length(b - a);
    float d = 0.5f * (r * r - w * w) / w;

    // center, orient and mirror
    float2 v = (b - a) / r;
    float2 c = (b + a) * 0.5f;

    mat2 xform = (mat2)(v.y, v.x, -v.x, v.y);
    float2 q = fabs(mat2vecmul(xform, pos - c));
    q *= 0.5f;

    // feature selection (vertex or body)
    float3 h = (r * q.x < d * (q.y - r)) ? (float3)(0.0f, r, 0.0f) : (float3)(-d, 0.0f, d + w);

    // distance
    return length(q - h.xy) - h.z;
}

float sdfVesica(float2 pos, float scale, float r, float w)
{
    r *= scale;
    w *= scale;

    float2 a = (float2)(0.0f, r);
    float2 b = (float2)(0.0f, -r);

    if (w == 0.0f) return sdfLine(pos, scale, a, b);

    w = min(w, r);

    return sdfVesicaSegment(pos, a, b, w);
}

float sdfMoon(float2 pos, float scale, float ra, float rb, float d)
{
    ra *= scale;
    rb *= scale;

    float end = ra + rb;
    d = fit01(d, -end, end);

    pos.y = fabs(pos.y);

    float a = (ra * ra - rb * rb + d * d) / (2.0 * d);
    float b = sqrt(max(ra * ra - a * a, 0.0f));
    if(d * (pos.x * b - pos.y * a) > d * d * max(b - pos.y, 0.0f))
          return length(pos - (float2)(a, b));
    return max(length(pos) - ra, -(length(pos - (float2)(d, 0.0f)) - rb));
}

float sdfEgg(float2 pos, float scale, float ra, float rb)
{
    ra *= scale;
    rb *= scale;

    rb = min(rb, ra);

    pos.x = fabs(pos.x);

    float k = sqrt(3.0f);
    float r = ra - rb;

    return ((pos.y < 0.0f) ? length((float2)(pos.x, pos.y)) - r :
            (k * (pos.x + r) < pos.y) ? length((float2)(pos.x, pos.y - k * r)) :
                              length((float2)(pos.x + r, pos.y)) - 2.0f * r) - rb;
}

float sdfHorseshoe(float2 pos, float scale, float r, float tb, float2 w)
{
    r *= scale;
    w *= scale;

    pos.x = fabs(pos.x);
    float l = length(pos);

    tb = clamp(tb, 0.0f, 1.0f);
    tb *= 3.141593f;
    // sc is the cos/sin of the arc's aperture
    float2 sc = (float2)(cos(tb), sin(tb));

    mat2 xform = (mat2)(-sc.x, sc.y, sc.y, sc.x);

    pos = mat2vecmul(xform, pos);
    pos = (float2)((pos.y > 0.0f || pos.x > 0.0f) ? pos.x : l * sign(-sc.x),
             (pos.x > 0.0f) ? pos.y : l);
    pos = (float2)(pos.x, fabs(pos.y - r)) - w;
    return length(max(pos, 0.0f)) + min(0.0f, max(pos.x, pos.y));
}

float sdfCross(float2 pos, float scale, float2 b, float r)
{
    b.x *= scale;
    r *= scale;

    b.y = fit01(b.y, b.x, 0.0f);
    pos = fabs(pos);
    pos = (pos.y > pos.x) ? (float2)(pos.y, pos.x) : (float2)(pos.x, pos.y);
    float2 q = pos - b;
    float k = max(q.y, q.x);
    float2  w = (k > 0.0f) ? q : (float2)(b.y - pos.x, -k);
    return sign(k) * length(max(w, 0.0f)) + r;
}

float sdfRoundedX(float2 pos, float scale, float w, float r)
{
    r *= scale;
    w *= scale;

    pos = fabs(pos);
    return length(pos - min(pos.x + pos.y, w) * 0.5f) - r;
}

float sdfMarkerT(float2 pos, float size)
{
    size *= 2.0f;
    float x = -pos.y;
    float y = pos.x;

    float r1 = max(fabs(x - size / 3.0f + size / 3.0f), fabs(x - size / 3.0f - size / 3.0f));
    float r2 = max(fabs(y - size / 3.0f), fabs(y + size / 3.0f));
    float r3 = max(fabs(x), fabs(y));
    float r = max(min(r1, r2), r3);
    r -= size / 2.0f;
    return r;
}

float sdfMarkerCheck(float2 pos, float size)
{
    size *= 2.0f;
    pos.x *= -1.0f;
    pos = rotate2D(pos, -90.0f);

    float x = M_SQRT2_F / 2.0f * (pos.x - pos.y);
    float y = M_SQRT2_F / 2.0f * (pos.x + pos.y);

    float r1 = max(fabs(x - 2.0f * size / 3.0f), fabs(x - 1.0f *size / 3.0f));
    float r2 = max(fabs(y - 2.0f * size / 3.0f), fabs(y - size / 3.0f));
    float r3 = max(fabs(x), max(fabs(x - size / 3.0f), fabs(y)));
    float r = max(min(r1, r2), r3);
    r -= size / 2.0f;
    return r;
}

float sdfMarkerCross(float2 pos, float size)
{
    size *= 2.0f;
    float x = M_SQRT2_F / 2.0f * (pos.x - pos.y);
    float y = M_SQRT2_F / 2.0f * (pos.x + pos.y);

    float r1 = max(fabs(x - size/3.0f), fabs(x + size/3.0f));
    float r2 = max(fabs(y - size/3.0f), fabs(y + size/3.0f));
    float r3 = max(fabs(x), fabs(y));
    float r = max(min(r1, r2), r3);
    r -= size / 2.0f;
    return r;
}

float sdfMarkerClobber(float2 pos, float size)
{
    size *= 2.0f;
    const float t1 = -M_PI_F/2.0f;
    float2 c1 = 0.25f * (float2)(cos(t1), sin(t1));

    const float t2 = t1 + 2.0f * M_PI_F / 3.0f;
    float2 c2 = 0.25f * (float2)(cos(t2), sin(t2));

    const float t3 = t2 + 2.0f * M_PI_F / 3.0f;
    float2 c3 = 0.25f * (float2)(cos(t3), sin(t3));

    float r1 = length(pos - c1 * size) - size / 3.5f;
    float r2 = length(pos - c2 * size) - size / 3.5f;
    float r3 = length(pos - c3 * size) - size / 3.5f;
    return min(min(r1, r2), r3);
}

float sdfMarkerAsterisk(float2 pos, float size)
{
    size *= 2.0f;

    float x = M_SQRT2_F / 2.0f * (pos.x - pos.y);
    float y = M_SQRT2_F / 2.0f * (pos.x + pos.y);

    float r1 = max(fabs(x)- size / 2.0f, fabs(y) - size / 10.f);
    float r2 = max(fabs(y)- size / 2.0f, fabs(x) - size / 10.f);
    float r3 = max(fabs(pos.x) - size / 2.0f, fabs(pos.y) - size / 10.0f);
    float r4 = max(fabs(pos.y) - size / 2.0f, fabs(pos.x) - size / 10.0f);
    return min(min(r1, r2), min(r3, r4));
}


float sdfMarkerChevron(float2 pos, float size)
{
    size *= 2.0f;
    pos = rotate2D(pos, 180.0f);
    float x = 1.0f / M_SQRT2_F * (pos.x - pos.y);
    float y = 1.0f / M_SQRT2_F * (pos.x + pos.y);
    float r1 = max(fabs(x), fabs(y)) - size / 3.0f;
    float r2 = max(fabs(x - size / 3.0f), fabs(y - size / 3.0f)) - size / 3.0f;
    return max(r1, -r2);
}

float sdfMarkerRing(float2 pos, float size)
{
    size *= 2.0f;
    float r1 = length(pos) - size / 2.0f;
    float r2 = length(pos) - size / 4.0f;
    return max(r1, -r2);
}

float sdfMarkerInfinity(float2 pos, float size)
{
    size *= 2.0f;
    float2 c1 = (float2)(0.25f, 0.0f);
    float2 c2 = (float2)(-0.25f, 0.0f);
    float r1 = length(pos - c1 * size) - size/3.0f;
    float r2 = length(pos - c1 * size) - size/7.0f;
    float r3 = length(pos - c2 * size) - size/3.0f;
    float r4 = length(pos - c2 * size) - size/7.0f;
    return min(max(r1, -r2), max(r3, -r4));
}

float sdfMarkerTag(float2 pos, float size)
{
    size *= 2.0f;
    pos = rotate2D(pos, -135.0f);
    float x = -pos.x;
    float y = pos.y;
    float r1 = max(fabs(x) - size / 2.0f, fabs(y) - size/6.0f);
    float r2 = fabs(x - size / 1.5f) + fabs(y) - size;
    return max(r1, 0.75f * r2);
}

// WIP Shapes
// A collection of shapes that are either implemented but deemed unecessary, or not yet in working order.

float sdfOrientedSquere(float2 _pos, float scale, float2 _a, float2 _b, float _th)
{
    float l = length(_b - _a);
    float2 d = (_b - _a) / l;
    float2 q = (_pos - (_a + _b) * (float2)(0.5f,0.5f));

    mat2 xform = (mat2)(d.x, -d.y, d.y, d.x);
    q = mat2vecmul(xform, q);
    q = fabs(q) - (float2)(l, _th) * (float2)(0.5f, 0.5f);

    return length(max(q, 0.0f)) + min(max(q.x, q.y), 0.0f);
}

float sdfHeart(float2 _pos){

    float scale = 1.0f;
    _pos.y += 0.5f;
    _pos.x = fabs(_pos.x);
    if(_pos.y + _pos.x > 1.0f)
        return sqrt(length2(_pos - (float2)(0.25f, 0.75f))) - sqrt(2.0f)/(4.0f) * scale;

    return sqrt(min(length2(_pos - (float2)(0.0f,1.0f)),
                    length2(_pos - (float2)(0.5,0.5) * max(_pos.x + _pos.y, (float2)(0.0f, 0.0f))))) * sign(_pos.x-_pos.y);
}

float sdfBlobbyCross(float2 _pos, float _he)
{
    _pos = fabs(_pos) * 2.0f;
    _pos = (float2)(fabs(_pos.x - _pos.y), 1.0f - _pos.x - _pos.y) / sqrt(2.0f);

    float p = (_he - _pos.y - 0.25f / _he) / (6.0f * _he);
    float q = _pos.x / (_he * _he * 16.0f);
    float h = q * q - p * p * p;

    float x = 0.0f;
    if( h > 0.0f)
    {
        float r = sqrt(h);
        x = pow(q + r, 1.0f / 3.0f) - pow(fabs(q - r), 1.0f / 3.0f) * sign(r - q);
    }
    else
    {
        float r = sqrt(p);
        x = 2.0f * r * cos(acos(q / (p * r)) / 3.0f);
    }
    x = min(x, sqrt(2.0f) / 2.0f);

    float2 z = (float2)(x, _he * (1.0f - 2.0f * x * x)) - _pos;
    return length(z * 0.6f) * sign(z.y);
}

float sdfPolygon(float2 _pos, float2 *_v, int _vcount)
{

    float d = dot(_pos - _v[0], _pos - _v[0]);
    float s = 1.0f;

    for(int i = 0, j = _vcount - 1; i < _vcount; j = i, i++)
    {
        float2 e = _v[j] - _v[i];
        float2 w = _pos - _v[i];
        float2 b = w - e * clamp(dot(w, e) / dot(e, e), 0.0f, 1.0f);
        d = min(d, dot(b, b));
        int3 c = (int3)(_pos.y >= _v[i].y, _pos.y < _v[j].y, e.x * w.y > e.y * w.x);
        //if(all(c) || all(not(c))) s *= -1.0f;
    }
    return s * sqrt(d);
}

// Test if point p crosses line (a, b), returns sign of result
float testCross(float2 a, float2 b, float2 p) {
    return sign((b.y - a.y) * (p.x - a.x) - (b.x - a.x) * (p.y - a.y));
}

// Determine which side we're on (using barycentric parameterization)
float signBezier(float2 A, float2 B, float2 C, float2 pos)
{
    float2 a = C - A;
    float2 b = B - A;
    float2 c = pos - A;

    float2 bary = (float2)(c.x * b.y - b.x * c.y, a.x * c.y - c.x * a.y) / (a.x * b.y - b.x * a.y);
    float2 d = (float2)(bary.y * 0.5f, 0.0f) + 1.0f - bary.x - bary.y;
    return mix(sign(d.x * d.x - d.y), mix(-1.0f, 1.0f,
        step(testCross(A, B, pos) * testCross(B, C, pos), 0.0f)),
        step((d.x - d.y), 0.0f)) * testCross(A, C, B);
}

// Solve cubic equation for roots
float3 solveCubic(float a, float b, float c)
{
    float p = b - a * a / 3.0f;
    float p3 = p * p * p;
    float q = a * (2.0f * a * a - 9.0f * b) / 27.0f + c;
    float d = q * q + 4.0f * p3 / 27.0f;
    float offset = -a / 3.0f;
    if(d >= 0.0f) {
        float z = sqrt(d);
        float2 x = ((float2)(z, -z) - q) / 2.0f;
        float2 uv = sign(x) * pow(fabs(x), (float2)(1.0f / 3.0f));
        return (float3)(offset + uv.x + uv.y);
    }
    float v = acos(-sqrt(-27.0f / p3) * q / 2.0f) / 3.0f;
    float m = cos(v), n = sin(v) * 1.732050808f;
    return (float3)(m + m, -n - m, n - m) * sqrt(-p / 3.0f) + offset;
}

// Find the signed distance from a point to a bezier curve
float sdfBezier(float2 pos, float scale, float2 A, float2 B, float2 C)
{
    A *= scale;
    B *= scale;
    C *= scale;

    B = mix(B + (float2)(1e-4), B, fabs(sign(B * 2.0f - A - C)));
    float2 a = B - A;
    float2 b = A - B * 2.0f + C;
    float2 c = a * 2.0f;
    float2 d = A - pos;
    float3 k = (float3)(3.0f * dot(a, b), 2.0f * dot(a,a) + dot(d,b), dot(d,a)) / dot(b,b);
    float3 t = clamp(solveCubic(k.x, k.y, k.z), 0.0f, 1.0f);

    float2 _pos = A + (c + b * t.x) * t.x;
    float dist = length(_pos - pos);
    _pos = A + (c + b * t.y) * t.y;
    dist = min(dist, length(_pos - pos));
    _pos = A + (c + b * t.z) * t.z;
    dist = min(dist, length(_pos - pos));
    return dist * signBezier(A, B, C, pos);
}

float sdfWaveSpiral(float2 pos, float scale, float w, float k)
{
    w *= scale;
    k *= scale;
    // body
    const float kTau = 6.283185307;
    float r = length(pos);
    float a = atan2(pos.y, pos.x);
    float n = floor( 0.5f / w + (log2(r / w) * k - a) / kTau);
    float ra = w * exp2((a + kTau * (min(n + 0.0f, 0.0f) - 0.5f)) / k);
    float rb = w * exp2((a + kTau * (min(n + 1.0f, 0.0f) - 0.5f)) / k);
    float d = min(fabs(r - ra), fabs(r - rb));

    // tip
    return min(d, length(pos + (float2)(w, 0.0f)) );
}

float sdfCompoundLine(float2 pos, float scale, float length, int use_points, float2 pt0, float2 pt1,
                      float start_thickness, float end_thickness, int do_bulge, float bulge) {
    if (use_points) {
        // Transform coordinates to aling line between pt0 and pt1
        length = (distance(pt0, pt1));
        float2 center = (pt0 + pt1) * 0.5f;
        pos -= center;
        float2 dir = pt1 - pt0;
        float angle = degrees(atan2(dir.y, dir.x));
        pos = rotate2D(pos, angle);
    }

    if (do_bulge) return sdfDogBone(pos, scale, length, start_thickness, bulge);
    pos = rotate2D(pos, 90);
    return sdfCapsule(pos, scale, length, end_thickness, start_thickness);
}

float sdfCompoundCircle(float2 pos, float scale, float radius, int do_arc, float arc,
                        int do_thickness, float thickness, int roundends, int quadratic)
{
    if (quadratic) return sdfQuadraticCircle(pos, radius * scale);
    if (do_arc == 0 && do_thickness == 0) return sdfCircle(pos, radius * scale);
    if (do_arc == 1 && do_thickness == 0) return sdfPie(pos, radius * scale, arc);

    if (do_arc == 0)
    {
        roundends = 1;
        arc = 1;
    }

    thickness = fit01(thickness, 0.0f, radius);
    if (roundends) return sdfArc(pos, scale, radius, thickness, arc);
        return sdfRing(pos, scale, radius, thickness, arc);
}


// SDF Operations

// The followings are used for modifing a single signed distance field. 
float sdfOpIsoOffset(float _d, float _r)
{
    return _d - _r;
}	

float sdfOpOnion(float _d, float _r)
{
    return fabs(_d) - _r;
}

// The followings are used to combine two singed distance fields in various ways.
// OP UNION
float sdfOpUnion(float _d1, float _d2)
{
    return min(_d1, _d2);
}

float sdfOpUnionSmooth(float _d1, float _d2, float _k)
{
    if (_k == 0) return sdfOpUnion(_d1, _d2);

    float h = max(_k - fabs(_d1 - _d2), 0.0f);
    return min(_d1, _d2) - h * h * 0.25f / _k;
}

float sdfOpUnionRound(float _d1, float _d2, float _k)
{
    float2 u = max((float2)(_k - _d1, _k - _d2), (float2)(0.0f, 0.0f));
    return max(_k, min(_d1, _d2)) - length(u);
}

float sdfOpUnionChamfer(float _d1, float _d2, float _k)
{
    return min(min(_d1, _d2), (_d1 - _k + _d2) * sqrt(0.5f));
}

// OP INTERSECT
float sdfOpIntersect(float _d1, float _d2)
{
    return max(_d1, _d2);
}

float sdfOpIntersectSmooth(float _d1, float _d2, float _k)
{
    return -sdfOpUnionSmooth(-_d1, -_d2, _k);
}

float sdfOpIntersectRound(float _d1, float _d2, float _k)
{
    float2 u = max((float2)(_k + _d1, _k + _d2), (float2)(0.0f, 0.0f));
    return min(-_k, max(_d1, _d2)) + length(u);
}

float sdfOpIntersectChamfer(float _d1, float _d2, float _k)
{
    return max(max(_d1, _d2), (_d1 + _k + _d2) * sqrt(0.5f));
}

// OP SUBTRACT
float sdfOpSubtract(float _d1, float _d2)
{
    return sdfOpIntersect(_d1, -_d2);
}

float sdfOpSubtractSmooth(float _d1, float _d2, float _k)
{
    return sdfOpIntersectSmooth(_d1, -_d2, _k);
}

float sdfOpSubtractRound(float _d1, float _d2, float _k)
{
    return sdfOpIntersectRound(_d1, -_d2, _k);
}

float sdfOpSubtractChamfer(float _d1, float _d2, float _k)
{
    return sdfOpIntersectChamfer(_d1, -_d2, _k);
}

// OP DIFFERENCE
float sdfOpDifference(float _d1, float _d2)
{
    return max(min(_d1, _d2), -max(_d1, _d2));
}

float sdfOpDifferenceSmooth(float _d1, float _d2, float _k)
{
    if (_k == 0) return sdfOpDifference(_d1, _d2);

    float h = max(_k - fabs(_d1 - _d2), 0.0f);
    float l = h * h * 0.25f / _k;
    return max(min(_d1, _d2) - l, -max(_d1, _d2) - l);
}

float sdfOpDifferenceRound(float _d1, float _d2, float _k)
{
    return min(sdfOpIntersectRound(-_d1, _d2, _k), sdfOpIntersectRound(_d1, -_d2, _k));
}

float sdfOpDifferenceChamfer(float _d1, float _d2, float _k)
{
    return min(sdfOpIntersectChamfer(-_d1, _d2, _k), sdfOpIntersectChamfer(_d1, -_d2, _k));
}

float sdfFishScale(float2 pos, float thickness) {
    thickness = fit01(thickness, 0.0f, 1.0f);
    float arc = 0.5f + (thickness * 0.1f);
    float r1 = sdfRing(rotate2D(pos, -90.0f), 1.0f, 1.0f, thickness, arc);
    float r2 = sdfRing(rotate2D(pos + (float2)(-1.0f, 1.0f), -90.0f), 1.0f, 1.0f, thickness, arc);
    float r3 = sdfRing(rotate2D(pos + (float2)(1.0f, 1.0f), -90.0f), 1.0f, 1.0f, thickness, arc);
    float r4 = sdfRing(rotate2D(pos + (float2)(-1.0f, -1.0f), -90.0f), 1.0f, 1.0f, thickness, arc);
    float r5 = sdfRing(rotate2D(pos + (float2)(1.0f, -1.0f), -90.0f), 1.0f, 1.0f, thickness, arc);

    return -sdfOpUnion(sdfOpUnion(sdfOpUnion(r2, r3), sdfOpUnion(r4, r5)), r1);
}

float sdfOctagonDot(float2 pos, float thickness) {
    float d = sdfStar(rotate2D(pos, 22.5f), 1.1f, 8, 2);
    d = fabs(d);
    d = sdfOpIsoOffset(d, thickness);
    return -d;
}

#endif
