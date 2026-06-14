//
    //  https://github.com/stegu/webgl-noise/blob/master/src/classicnoise3D.glsl

    // Description : Array and textureless GLSL 2D/3D/4D simplex 
    //               noise functions.
    //      Author : Ian McEwan, Ashima Arts.
    //  Maintainer : stegu
    //     Lastmod : 20201014 (stegu)
    //     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
    //               Distributed under the MIT License. See LICENSE file.
    //               https://github.com/ashima/webgl-noise
    //               https://github.com/stegu/webgl-noise
// 
#ifndef NOISES_CL
#define NOISES_CL

#include "./noises_ashima.cl"
#include "./voronoi_iq.cl"
#include "./noises_iq.cl"


#endif



// //------------------------------------------------------------- 
    // // replace these by something better (production ready)
    // float3 Hash33( float3 p ){

    // 	p = (float3)( dot(p,(float3)(127.1,311.7, 74.7)),
    // 			  dot(p,(float3)(269.5,183.3,246.1)),
    // 			  dot(p,(float3)(113.5,271.9,124.6)));

    //     float3 q = (float3)-1.0 + (float3)2.0*frac(sin(p)*(float3)43758.5453123, &q);
    // 	return q;
    // }

    // float2 Hash22( float2 p ){

    // 	p = (float2)( dot(p,(float2)(127.1,311.7)), dot(p,(float2)(269.5,183.3)));
    // 	float2 q = (float2)-1.0 + (float2)2.0*frac(sin(p)*(float2)43758.5453123, &q);
    //     return q;
    // }

    // float 12( float2 p ){

    //     float2 q;
    //     q = (float2)50.0*frac( p*(float2)0.3183099, &q);

    //     return frac( p.x*p.y*(p.x+p.y) );
    // }

    // float3 IHash33( uint3 x ){

    //     const uint k = 1103515245U;  // GLIB C

    //     x = ((x>>8U)^x.yzx)*k;
    //     x = ((x>>8U)^x.yzx)*k;
    //     x = ((x>>8U)^x.yzx)*k;
        
    //     return (float3)x*(1.0/float(0xffffffffU));
    // }


    // //------------------------------------------------------------- noises
    // // https://www.shadertoy.com/view/4ttSWf
    // float Noise(float3 x ){

    //     float3 p = floor(x);
    //     float3 w = frac(x);
        
    //     float3 u = w*w*w*(w*(w*6.0-15.0)+10.0);

    //     float n = p.x + 317.0*p.y + 157.0*p.z;
        
    //     float a = Hash12(n+0.0);
    //     float b = Hash12(n+1.0);
    //     float c = Hash12(n+317.0);
    //     float d = Hash12(n+318.0);
    //     float e = Hash12(n+157.0);
    // 	float f = Hash12(n+158.0);
    //     float g = Hash12(n+474.0);
    //     float h = Hash12(n+475.0);

    //     float k0 =   a;
    //     float k1 =   b - a;
    //     float k2 =   c - a;
    //     float k3 =   e - a;
    //     float k4 =   a - b - c + d;
    //     float k5 =   a - c - e + g;
    //     float k6 =   a - b - e + f;
    //     float k7 = - a + b + c - d + e - f - g + h;

    //     return -1.0+2.0*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z);
    // }

    // #ifndef INTERPOLANT
    //     #define INTERPOLANT 0
    // #endif

    // float GradNoise3D(float3 p ){

    //     float3 i = floor( p );
    //     float3 f = frac( p );

    //     #if INTERPOLANT==1
    //     // quintic interpolant
    //     float3 u = f*f*f*(f*(f*6.0-15.0)+10.0);
    //     #else
    //     // cubic interpolant
    //     float3 u = f*f*(3.0-2.0*f);
    //     #endif    

    //     return mix( mix(  mix(   dot( Hash33( i + float3(0.0,0.0,0.0) ), f - float3(0.0,0.0,0.0) ), 
    //                                 dot( Hash33( i + float3(1.0,0.0,0.0) ), f - float3(1.0,0.0,0.0) ), u.x),
    //                         mix(   dot( Hash33( i + float3(0.0,1.0,0.0) ), f - float3(0.0,1.0,0.0) ), 
    //                                 dot( Hash33( i + float3(1.0,1.0,0.0) ), f - float3(1.0,1.0,0.0) ), u.x), u.y),
    //                 mix( mix(     dot( Hash33( i + float3(0.0,0.0,1.0) ), f - float3(0.0,0.0,1.0) ), 
    //                                 dot( Hash33( i + float3(1.0,0.0,1.0) ), f - float3(1.0,0.0,1.0) ), u.x),
    //                      mix(      dot( Hash33( i + float3(0.0,1.0,1.0) ), f - float3(0.0,1.0,1.0) ), 
    //                                 dot( Hash33( i + float3(1.0,1.0,1.0) ), f - float3(1.0,1.0,1.0) ), u.x), u.y), u.z );
    // }

    // float SimplexNoise(float2 p ){

    //     const float K1 = 0.366025404; // (sqrt(3)-1)/2
    //     const float K2 = 0.211324865; // (3-sqrt(3))/6

    // 	float2  i = floor( p + (p.x+p.y)*K1 );
    //     float2  a = p - i + (i.x+i.y)*K2;
    //     float m = step(a.y,a.x); 
    //     float2  o = float2(m,1.0-m);
    //     float2  b = a - o + K2;
    // 	float2  c = a - 1.0 + 2.0*K2;
    //     float3  h = max( 0.5-float3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
    // 	float3  n = h*h*h*h*float3( dot(a,Hash22(i+0.0)), dot(b,Hash22(i+o)), dot(c,Hash22(i+1.0)));
    //     return dot( n, (float3)70.0 );
    // }


    // //----------------------------------------------------------------------- FBM
    // // float Hash11( float n ) { return frac(sin(n)*753.5453123); }
    // int IHash11(int a){
    // 	a = (a ^ 61) ^ (a >> 16);
    // 	a = a + (a << 3);
    // 	a = a ^ (a >> 4);
    // 	a = a * 0x27d4eb2d;
    // 	a = a ^ (a >> 15);
    // 	return a;
    // }

    // #define Hash11(a) ( float(IHash11(a)) / float(0x7FFFFFFF) ) // Uniform[0,1]

    // //---------------------------------------------------------------
    // // value noise, and its analytical derivatives
    // //---------------------------------------------------------------
    // float4 NoiseD3(float3 x )
    // {
    //     float3 p = floor(x);
    //     float3 w = frac(x);
    // 	float3 u = w*w*(3.0-2.0*w);
    //     float3 du = 6.0*w*(1.0-w);
        
    //     float n = p.x + p.y*157.0 + 113.0*p.z;
        
    //     float a = Hash11(n+  0.0);
    //     float b = Hash11(n+  1.0);
    //     float c = Hash11(n+157.0);
    //     float d = Hash11(n+158.0);
    //     float e = Hash11(n+113.0);
    // 	float f = Hash11(n+114.0);
    //     float g = Hash11(n+270.0);
    //     float h = Hash11(n+271.0);

    //     float k0 =   a;
    //     float k1 =   b - a;
    //     float k2 =   c - a;
    //     float k3 =   e - a;
    //     float k4 =   a - b - c + d;
    //     float k5 =   a - c - e + g;
    //     float k6 =   a - b - e + f;
    //     float k7 = - a + b + c - d + e - f - g + h;

    //     return float4( k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z, 
    //                  du * (float3(k1,k2,k3) + u.yzx*float3(k4,k5,k6) + u.zxy*float3(k6,k4,k5) + k7*u.yzx*u.zxy ));
    // }

    // #ifndef M3
    // #define M3  float3x3(0.00, 0.80, 0.60, -0.80,0.36,-0.48, -0.60,-0.48,0.64)
    // #define M3i float3x3(0.00,-0.80,-0.60,  0.80,0.36,-0.48,  0.60,-0.48,0.64)
    // #endif


    // // returns 3D fbm and its 3 derivatives
    // float4 FbmD(float3 x, int octaves )
    // {
    //     float3x3 m = float3x3(
    //         1.0,0.0,0.0,
    //         0.0,1.0,0.0,
    //         0.0,0.0,1.0
    //     );

    //     float f = 1.98;  // could be 2.0
    //     float s = 0.49;  // could be 0.5
        
    //     float a = 0.0;  // noise
    //     float3 d = 0.0; // derivative

    //     float b = 0.5;  // decreasing scale
                                
    //     for( int i=0; i < octaves; i++ )
    //     {
    //         float4 n = NoiseD3(x);
    //         a += b*n.x;                // accumulate values
    //         d += mul(m, b*n.yzw);      // accumulate derivatives
    //         b *= s;
    //         x = mul(M3, f*x);
    //         m = mul(M3i,f*m);
    //     }
    //     return float4(a, d);
    // }






    // const float3x3 m3  = float3x3( 
    //     0.00,  0.80,  0.60,
    //     -0.80,  0.36, -0.48,
    //     -0.60, -0.48,  0.64
    // );
    // const float3x3 m3i = float3x3( 
    //     0.00, -0.80, -0.60,
    //     0.80,  0.36, -0.48,
    //     0.60, -0.48,  0.64 
    // );



    // // value noise, and its analytical derivatives
    // float4 noised( in float3 x )
    // {
    //     float3 p = floor(x);
    //     float3 w = frac(x);
    //     #if 1
    //     float3 u = w*w*w*(w*(w*6.0-15.0)+10.0);
    //     float3 du = 30.0*w*w*(w*(w-2.0)+1.0);
    //     #else
    //     float3 u = w*w*(3.0-2.0*w);
    //     float3 du = 6.0*w*(1.0-w);
    //     #endif

    //     float n = p.x + 317.0*p.y + 157.0*p.z;
        
    //     float a = hash1(n+0.0);
    //     float b = hash1(n+1.0);
    //     float c = hash1(n+317.0);
    //     float d = hash1(n+318.0);
    //     float e = hash1(n+157.0);
    // 	float f = hash1(n+158.0);
    //     float g = hash1(n+474.0);
    //     float h = hash1(n+475.0);

    //     float k0 =   a;
    //     float k1 =   b - a;
    //     float k2 =   c - a;
    //     float k3 =   e - a;
    //     float k4 =   a - b - c + d;
    //     float k5 =   a - c - e + g;
    //     float k6 =   a - b - e + f;
    //     float k7 = - a + b + c - d + e - f - g + h;

    //     return float4( -1.0+2.0*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z), 
    //                       2.0* du * float3( k1 + k4*u.y + k6*u.z + k7*u.y*u.z,
    //                                       k2 + k5*u.z + k4*u.x + k7*u.z*u.x,
    //                                       k3 + k6*u.x + k5*u.y + k7*u.x*u.y ) );
    // }






    // void CSSimplexfracalRGB()
    // {
    //     float f = 0;

    //     uv *= 5.0;
    //     mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
    //     f  = 0.5000*noise( uv ); uv = m*uv;
    //     f += 0.2500*noise( uv ); uv = m*uv;
    //     f += 0.1250*noise( uv ); uv = m*uv;
    //     f += 0.0625*noise( uv ); uv = m*uv;
    // }