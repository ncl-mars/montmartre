#ifndef _FRACTALS_E_
#define _FRACTALS_E_



#include "matrix.h"
#include "./noises.cl"


#ifndef __NOISE_FUNC__
#define __NOISE_FUNC__ sNoise
#endif 


// https://www.shadertoy.com/view/4ttSWf
// typedef struct{
//     float scale;
//     int octave;

// } pfract;



// returns 3D fbm
float sFbm(float3 x, int octaves)
{
    mat3 M3  = {{0.00, 0.80, 0.60}, {-0.80,0.36,-0.48}, {-0.60,-0.48,0.64}};
    mat3 M3i = {{0.00,-0.80,-0.60}, { 0.80,0.36,-0.48}, { 0.60,-0.48,0.64}};

    mat3 m; mat3identity(m);

    float f = 1.75;  // could be 2.0
    float s = 0.6;  // could be 0.5
    float b = 0.8;  // decreasing scale

    float a = 0.0;  // noise
    float3 d = 0.0; // derivative


    for( int i=0; i < octaves; i++ )
    {
        float n = _NOISE_FUNC_(x);
        // float n = sNoise(x);
        a += b*n; // accu values
        // d += mul(m, b*n.yzw); // accumulate derivatives
        
        b *= s;
        x = mat3vecmul(M3, (float3)f*x);

        mat3scale(m,m,f);
        mat3mul(M3i,m, m);
    }
    return a;
}

#endif
 