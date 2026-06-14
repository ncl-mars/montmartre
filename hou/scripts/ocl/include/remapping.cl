#ifndef REMAPPING_CL
#define REMAPPING_CL

#ifndef PI
#define PI 3.1415926535897932f
#endif


float3 spherize(float3 pos, float radius, float blend){
    
    // --- Step 1: Compute vector from origin ---
    float len = length(pos);

    // Prevent division by zero
    if (len < 1e-6f) return pos;

    // --- Step 2: Compute spherical projection ---
    float3 dir = normalize(pos);
    float3 sphere_pos = dir * radius;

    return mix(pos, sphere_pos, clamp(blend,0.0f,1.0f));

}
/*
    float sphericalDisplace(float2 wpos, float radius){
        
        float d2 = dot2(wpos);
        float r2 = radius * radius;
        return (sqrt(r2 - d2) - radius) * (d2 <= r2);
    }
*/

//------------------------------------------------------------- Range Mapping
float __attribute__((overloadable)) mapTo01(float imin, float imax, float x){
    return (x - imin) / (imax - imin);
}
float __attribute__((overloadable)) mapRange(float imin, float imax, float x){
    return mapTo01(imin, imax, x);
}
float __attribute__((overloadable)) mapRange(float imin, float imax, float omin, float omax, float x){
    return mapTo01(imin, imax, x) * (omax-omin) + omin;
}

float __attribute__((overloadable)) saturate(float x){
    return clamp(x, 0.0f, 1.0f);
}
float2 __attribute__((overloadable)) saturate(float2 x){
    return clamp(x, 0.0f, 1.0f);
}
float3 __attribute__((overloadable)) saturate(float3 x){
    return clamp(x, 0.0f, 1.0f);
}

float __attribute__((overloadable)) saturatectr(float x){
    return clamp(x, -1.0f, 1.0f);
}
float2 __attribute__((overloadable)) saturatectr(float2 x){
    return clamp(x, -1.0f, 1.0f);
}
float3 __attribute__((overloadable)) saturatectr(float3 x){
    return clamp(x, -1.0f, 1.0f);
}
float __attribute__((overloadable)) rampToTri(float x){
    return min(x, -x+1.0f)*2.0;
}

//------------------------------------------------------------- Identities
float __attribute__((overloadable)) almostIdentity( float x, float m, float n ){

    if( x>m ) return x;
    float a = 2.0*n - m;
    float b = 2.0*m - 3.0*n;
    float t = x/m;
    return (a*t + b)*t*t + n;
}
float __attribute__((overloadable)) almostIdentity( float x, float n ){

    return sqrt(x*x+n*n);
}
float integralSmoothstep( float x, float T ){

    if( x>T ) return x - T/2.0;
    return x*x*x*(1.0-x*0.5/T)/T/T;
}

//------------------------------------------------------------- Impulses
float expImpulse( float x, float k ){

    float h = k*x;
    return h*exp(1.0-h);
}
float quaImpulse( float k, float x ){

    return 2.0*sqrt(k)*x/(1.0+k*x*x);
}
float polyImpulse( float k, float n, float x ){

    return (n/(n-1.0))*
           pow((n-1.0)*k,1.0/n)*
           x/(1.0+k*pow(x,n));
}
float expSustainedImpulse( float x, float f, float k ){

    float s = max(x-f,0.0f);
    return min( x*x/(f*f), 1.0f+(2.0f/f)*s*exp(-k*s));
}
float sinc( float x, float k ){

    float a = PI*(k*x-1.0);
    return sin(a)/a;
}
float trunc_fallof( float x, float m ){

    x /= m;
    return (x-2.0)*x+1.0;
}

//------------------------------------------------------------- Impulses
float almostUnitIdentity( float x ){

    return x*x*(2.0-x);
}
float gain( float x, float k ) {

    float a = 0.5*pow(2.0f*((x<0.5f)?x:1.0f-x), k);
    return (x<0.5)?a:1.0-a;
}
float parabola( float x, float k ){

    return pow( 4.0f*x*(1.0f-x), k );
}
float pcurve( float x, float a, float b ){

    float k = pow(a+b,a+b)/(pow(a,a)*pow(b,b));
    return k*pow(x,a)*pow(1.0f-x,b);
}
float tone( float x, float k ){

    return (k+1.0)/(1.0+k*x);
}

//------------------------------------------------------------- Pulses, Bumps and Steps
float cubicPulse( float c, float w, float x ){

    x = fabs(x - c);
    if( x>w ) return 0.0;
    x /= w;
    return 1.0 - x*x*(3.0-2.0*x);
}
float rationalBump( float x, float k ){

    return 1.0/(1.0+k*x*x);
}
float expStep( float x, float n ){

    return exp2( -exp2(n)*pow(x,n) );
}


#endif



// float2 slerp(float2 start, float2 end, float percent){
//     // Dot product - the cosine of the angle between 2 vectors.
//     float dt = dot(start, end);     
//     // Clamp it to be in the range of Acos()
//     // This may be unnecessary, but floating point
//     // precision can be a fickle mistress.
//     dt = clamp(dt, -1.0f, 1.0f);
//     // Acos(dot) returns the angle between start and end,
//     // And multiplying that by percent returns the angle between
//     // start and the final result.
//     float theta = acos(dt)*percent;
//     float2 RelativeVec = normalize(end - start*dt); // Orthonormal basis
//     // The final result.
//     return ((start*cos(theta)) + (RelativeVec*sin(theta)));
// }
