#ifndef IQ_NOISES_CL
#define IQ_NOISES_CL

#include "./utils.cl" // frac, mod...
#include "./hashs.cl" // frac, mod...


float voronoiseIq(float3 p, float u, float v ){

	float k = 1.0+63.0*pow(1.0-v,6.0);

    float3 i = floor(p);
    float3 f = fract(p, &f);
    
	float2 a = 0.0f;

    for( int z=-2; z<=2; z++ )
    for( int y=-2; y<=2; y++ )
    for( int x=-2; x<=2; x++ )
    {
        float3  g = (float3)( x, y, z );

		float4  o = hashWithoutSine43(i + g) * (float4)(u,u,u,1.0);
		float3  d = g - f + o.xyz;

		float w = pow( 1.0f-smoothstep(0.0f,1.414f,length(d)), k );
		a += (float2)(o.w*w,w);
    }
	
    return a.x/a.y;
}

float3 simplexhash33(float3 p3){
    // return hashWithoutSine33(x);

    // p3 = frac(p3 * (float3)(.1031,.11369,.13787));
    // p3 += dot(p3, p3.yxz+19.19f);
    // return -1.0f + 2.0f * frac((float3)((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));

    float j = 4096.0f*sin(dot(p3,(float3)(17.0, 59.4, 15.0)));
	float3 r;
	r.z = frac(512.0f*j);
	j *= .125f;
	r.x = frac(512.0f*j);
	j *= .125f;
	r.y = frac(512.0f*j);
	return r-0.5f;

    // uint3 seed = as_uint3(convert_int3(p3));
    // return convert_float3(pcg3d(seed))/(float3)(0xffffffffU);
}

float simplexIq(float3 p){

    const float F3 =  0.3333333;
    const float G3 =  0.1666667;

    /* calculate s and x */
    float3 s = floor(p + dot(p, (float3)(F3)));
    float3 x = p - s + dot(s, (float3)(G3));
    
    /* calculate i1 and i2 */
    float3 e = step(0.0f, x - x.yzx);
    float3 i1 = e*(1.0f - e.zxy);
    float3 i2 = 1.0f - e.zxy*(1.0f - e);
    
    /* x1, x2, x3 */
    float3 x1 = x - i1 + G3;
    float3 x2 = x - i2 + 2.0f*G3;
    float3 x3 = x - 1.0f + 3.0f*G3;
    
    /* 2. find four surflets and store them in d */
    float4 w, d;
    
    /* calculate surflet weights */
    w.x = dot(x, x);
    w.y = dot(x1, x1);
    w.z = dot(x2, x2);
    w.w = dot(x3, x3);
    
    /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */
    w = max(0.6f - w, 0.0f);
    
    /* calculate surflet components */
    d.x = dot(simplexhash33(s), x);
    d.y = dot(simplexhash33(s + i1), x1);
    d.z = dot(simplexhash33(s + i2), x2);
    d.w = dot(simplexhash33(s + 1.0f), x3);
    
    /* multiply d by w^4 */
    w *= w;
    w *= w;
    d *= w;
    
    /* 3. return the sum of the four surflets */
    return dot(d, 52.0f);
}

float3 niqhash33( int3 p ){

    return convert_float3(pcg3d(as_uint3(p)))/(float3)(0xffffffffU)*2.0f-1.0f;

	int3 n = (int3)( p.x*127 + p.y*311 + p.z*74,
                     p.x*269 + p.y*183 + p.z*246,
                     p.x*113 + p.y*271 + p.z*124);

	// 1D hash by Hugo Elias
	n = (n << 13) ^ n;
    n = n * (n * n * 15731 + 789221) + 1376312589;
    return -1.0f+2.0f*convert_float3( n & (int3)(0x0fffffff))/(float)(0x0fffffff);
}
float niqhash13( int3 p ){

    return (float)(pcg3d(as_uint3(p)).x)/(float)(0xffffffffU)*2.0f-1.0f;
}

#define INTERPOLANT 1
// return value noise (in x) and its derivatives (in yzw)
//https://www.shadertoy.com/view/4dffRH
float4 gradnoisedIq( float3 x ){

    
    int3 i = convert_int3(floor(x)); // grid, int hash !
    float3 f = fract(x, &f);
    
    #if INTERPOLANT==1
    // quintic interpolant
    float3 u = f*f*f*(f*(f*6.0f-15.0f)+10.0f);
    float3 du = 30.0f*f*f*(f*(f-2.0f)+1.0f);
    #else
    // cubic interpolant
    float3 u = f*f*(3.0f-2.0f*f);
    float3 du = 6.0f*f*(1.0f-f);
    #endif    

    float3 ga = niqhash33( i+(int3)(0,0,0) );
    float3 gb = niqhash33( i+(int3)(1,0,0) );
    float3 gc = niqhash33( i+(int3)(0,1,0) );
    float3 gd = niqhash33( i+(int3)(1,1,0) );
    float3 ge = niqhash33( i+(int3)(0,0,1) );
	float3 gf = niqhash33( i+(int3)(1,0,1) );
    float3 gg = niqhash33( i+(int3)(0,1,1) );
    float3 gh = niqhash33( i+(int3)(1,1,1) );

    
    // projections
    float va = dot( ga, f-(float3)(0.0,0.0,0.0) );
    float vb = dot( gb, f-(float3)(1.0,0.0,0.0) );
    float vc = dot( gc, f-(float3)(0.0,1.0,0.0) );
    float vd = dot( gd, f-(float3)(1.0,1.0,0.0) );
    float ve = dot( ge, f-(float3)(0.0,0.0,1.0) );
    float vf = dot( gf, f-(float3)(1.0,0.0,1.0) );
    float vg = dot( gg, f-(float3)(0.0,1.0,1.0) );
    float vh = dot( gh, f-(float3)(1.0,1.0,1.0) );
	
    // interpolations
    float k0 = va-vb-vc+vd;
    float3  g0 = ga-gb-gc+gd;
    float k1 = va-vc-ve+vg;
    float3  g1 = ga-gc-ge+gg;
    float k2 = va-vb-ve+vf;
    float3  g2 = ga-gb-ge+gf;
    float k3 = -va+vb+vc-vd+ve-vf-vg+vh;
    float3  g3 = -ga+gb+gc-gd+ge-gf-gg+gh;
    float k4 = vb-va;
    float3  g4 = gb-ga;
    float k5 = vc-va;
    float3  g5 = gc-ga;
    float k6 = ve-va;
    float3  g6 = ge-ga;
    
    return (float4)(
        va + k4*u.x + k5*u.y + k6*u.z + k0*u.x*u.y + k1*u.y*u.z + k2*u.z*u.x + k3*u.x*u.y*u.z,    // value
        ga + g4*u.x + g5*u.y + g6*u.z + g0*u.x*u.y + g1*u.y*u.z + g2*u.z*u.x + g3*u.x*u.y*u.z +   // derivatives
        du * ( (float3)(k4,k5,k6) + (float3)(k0,k1,k2)*u.yzx + (float3)(k2,k0,k1)*u.zxy + k3*u.yzx*u.zxy ));
}

float gradnoiseIq( float3 p )
{
    int3 i = convert_int3(floor(p)); // grid, int hash !
    float3 f = frac( p );

    #if INTERPOLANT==1
    // quintic interpolant
    float3 u = f*f*f*(f*(f*6.0f-15.0f)+10.0f);
    #else
    // cubic interpolant
    float3 u = f*f*(3.0f-2.0f*f);
    #endif    

    return mix( mix( mix( dot( niqhash33( i + (int3)(0,0,0) ), f - (float3)(0,0,0) ), 
                          dot( niqhash33( i + (int3)(1,0,0) ), f - (float3)(1,0,0) ), u.x),
                     mix( dot( niqhash33( i + (int3)(0,1,0) ), f - (float3)(0,1,0) ), 
                          dot( niqhash33( i + (int3)(1,1,0) ), f - (float3)(1,1,0) ), u.x), u.y),
                mix( mix( dot( niqhash33( i + (int3)(0,0,1) ), f - (float3)(0,0,1) ), 
                          dot( niqhash33( i + (int3)(1,0,1) ), f - (float3)(1,0,1) ), u.x),
                     mix( dot( niqhash33( i + (int3)(0,1,1) ), f - (float3)(0,1,1) ), 
                          dot( niqhash33( i + (int3)(1,1,1) ), f - (float3)(1,1,1) ), u.x), u.y), u.z );
}


// return value noise (in x) and its derivatives (in yzw)
float4 valnoisedIq( float3 x ){

    int3 i = convert_int3(floor(x));
    float3 w = fract(x, &w);
    
    #if INTERPOLANT==1
    // quintic interpolant
    float3 u = w*w*w*(w*(w*6.0f-15.0f)+10.0f);
    float3 du = 30.0f*w*w*(w*(w-2.0f)+1.0f);
    #else
    // cubic interpolant
    float3 u = w*w*(3.0f-2.0f*w);
    float3 du = 6.0f*w*(1.0f-w);
    #endif     
 
    float a = niqhash13(i+(int3)(0,0,0));
    float b = niqhash13(i+(int3)(1,0,0));
    float c = niqhash13(i+(int3)(0,1,0));
    float d = niqhash13(i+(int3)(1,1,0));
    float e = niqhash13(i+(int3)(0,0,1));
	float f = niqhash13(i+(int3)(1,0,1));
    float g = niqhash13(i+(int3)(0,1,1));
    float h = niqhash13(i+(int3)(1,1,1));
	

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return (float4)(    k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z, 
        du * (float3)(  k1 + k4*u.y + k6*u.z + k7*u.y*u.z, 
                        k2 + k5*u.z + k4*u.x + k7*u.z*u.x, 
                        k3 + k6*u.x + k5*u.y + k7*u.x*u.y ) );
}

#endif