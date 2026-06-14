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
 * NAME:    xnoise.h ( CE Library, OpenCL)
 *
 * COMMENTS:  Simplex noise functions.  Use with the "Include XNoise Data"
 *            option on the GasOpenCL DOP or OpenCL SOP, passing the
 *           "theXNoise" parameter provided to the kernel as the first
 *            parameter to these functions.
 *            NOTE: This is an OpenCL port of OpenSimplex by Kurt Spencer:
 *            https://gist.github.com/KdotJPG/b1270127455a94ac5d19
 */

#ifndef __XNOISE_H
#define __XNOISE_H

// Declarations

// 1D float simplex noise. Equivalent to VEX:
// float xnoise( float )
float xnoise1(global const void *theXNoise, float x);

// 1D vector simplex noise. Equivalent to VEX:
// vector xnoise( float )
float3 xnoisev1(global const void *theXNoise, float x);

// 1D float simplex noise with derivatives. Equivalent to VEX:
// void xnoised( float; float &; float & )
void xnoise1d(global const void *theXNoise, float x,
              float *outn, float *outdndx);

// 1D vector simplex noise with derivatives. Equivalent to VEX:
// void xnoised( float; vector &; vector & )
void xnoisev1d(global const void *theXNoise, float x,
               float3 *outn, float3 *outdndx);

// 2D float simplex noise. Equivalent to VEX:
// float xnoise( float; float )
float xnoise2(global const void *theXNoise, float2 p);

// 2D float simplex noise with derivatives. Equivalent to VEX:
// void xnoised( float; float; float &; float &; float & )
void xnoise2d(global const void *theXNoise, float2 p,
              float *outn, float *outdndx, float *outdndy);

// 2D vector simplex noise. Equivalent to VEX:
// vector xnoise( float; float )
float3 xnoisev2(global const void *theXNoise, float2 p);

// 2D vector simplex noise with derivatives. Equivalent to VEX:
// void xnoised( float; float; vector &; vector &; vector & )
void xnoisev2d(global const void *theXNoise, float2 p,
               float3 *outn, float3 *outdndx, float3 *outdndy);

// 2D curlnoise.  Equivalent to VEX:
// vector curlxnoise2d( float; float )
float2 curlxnoise2(global const void *theXNoise, float2 p);

// 3D float simplex noise.  Equivalent to VEX:
// float xnoise( vector )
float xnoise3(global const void *theXNoise, float3 p);

// 3D float simplex noise with derivatives.  Equivalent to VEX:
// void xnoised( vector; float &; float &; float &; float & )
void xnoise3d(global const void *theXNoise, float3 p,
              float *outn, float *outdndx, float *outdndy, float *outdndz);

// 3D vector simplex noise.  Equivalent to VEX:
// vector xnoise( vector )
float3 xnoisev3(global const void *theXNoise, float3 p);

// 3D vector simplex noise with derivatives.  Equivalent to VEX:
// void xnoised( vector; vector &; vector &; vector &; vector & )
void xnoisev3d(global const void *theXNoise, float3 p, 
               float3 *outn, float3 *outdndx, 
               float3 *outdndy, float3 *outdndz);

// 3D curlnoise.  Equivalent to VEX:
// vector curlxnoise( vector )
float3 curlxnoise3(global const void *theXNoise, float3 p);

// 4D float simplex noise.  Equivalent to VEX:
// float xnoise( vector4 )
float xnoise4(global const void *theXNoise, float4 p);

// 4D float simplex noise with derivatives.  Equivalent to VEX:
// void xnoised( vector4; float &; float &; float &; float &; float & )
void xnoise4d(global const void *theXNoise, float4 p, 
              float *outn, float *outdndx, float *outdndy,
              float *outdndz, float *outdndw);

// 4D vector simplex noise.  Equivalent to VEX:
// vector xnoise( vector4 )
float3 xnoisev4(global const void *theXNoise, float4 p);

// 4D vector simplex noise with derivatives.  Equivalent to VEX:
// void xnoised( vector4; vector &; vector &; vector &; vector &; vector & )
void xnoisev4d(global const void *theXNoise, float4 p, 
               float3 *outn, float3 *outdndx, float3 *outdndy,
               float3 *outdndz, float3 *outdndw);

// 4D curlnoise.  Equivalent to VEX:
// vector curlxnoise( vector4 )
float3 curlxnoise4(global const void *theXNoise, float4 p);

// Implementation

constant int PERMSIZE = 8192;
constant int PERMMASK = 8191;
constant int RTABLE_SIZE = 8200;
constant float FREQ_ADJUST = 1.25f;
// Frequency adjust for old simplex, which we only use for 1-D now.
constant float FREQ_ADJUST_OLD = 0.66666666666f;

inline float pow4(float x)
{
    x *= x;
    return x * x;
}

inline float pow3(float x)
{
    return x * x * x;
}

inline float pow2(float x)
{
    return x * x;
}

inline void
extrapolate1(global const int *thePx, global const float *theXT, 
			 int xsb, float x, float *n, float *dndx)
{
    float attn = pow2(x);
    if (attn < 0.5f)
    {
		float t = pow3(0.5 - attn);
		int index = thePx[xsb & PERMMASK];
		float rx = theXT[index];
		float b = rx * x;
		*n += t * b;
		
		float t26 = -6 * pow2(0.5f - attn);
		*dndx += t26 * x * b + t * rx;
    }
}

inline void
extrapolate1v(global const int *thePx, 
			 global const float *theXT, global const float *theYT, global const float *theZT, 
			 int xsb, float x, float3 *n, float3 *dndx)
{
    float attn = pow2(x);
    if (attn < 0.5f)
    {
		float t = pow3(0.5 - attn);
		int index = thePx[xsb & PERMMASK];
		float3 rx = (float3)(theXT[index], theYT[index], theZT[index]);
		float3 b = rx * x;
		*n += t * b;
		
		float t26 = -6 * pow2(0.5f - attn);
		*dndx += t26 * x * b + t * rx;
    }
}

void xnoise1d(global const void *theXNoise, float x, float *n, float *dndx)
{
    global const int *thePx = (global const int *)theXNoise;
    global const float *theXT = (global const float *)(thePx + 4 * PERMSIZE);
    global const float *theYT = theXT + RTABLE_SIZE;
    global const float *theZT = theYT + RTABLE_SIZE;

    const float STRETCH_CONSTANT_1D = 0.41421356237309515f;
    const float SQUISH_CONSTANT_1D = 0.29289321881345254f;
    const float SQUISH_CONSTANT_1DCOMP = 0.7071067811865475f;
    const float NORM_CONSTANT_1D = 26.82f;

    x *= FREQ_ADJUST_OLD;
    float s = x * STRETCH_CONSTANT_1D;
    int i = floor(x + s);
    float st = i * SQUISH_CONSTANT_1D;
    x -= (i - st);

    *n = *dndx = 0.0f;

    extrapolate1(thePx, theZT, i, x, n, dndx);

    extrapolate1(thePx, theZT, i + 1, x - SQUISH_CONSTANT_1DCOMP, n, dndx);

    *n = *n * NORM_CONSTANT_1D + 0.5f;
    *dndx *= NORM_CONSTANT_1D * FREQ_ADJUST_OLD;
}

float xnoise1(global const void *theXNoise, float x)
{
	float n = 0, dndx = 0;
	xnoise1d(theXNoise, x, &n, &dndx);
	return n;
}

void xnoisev1d(global const void *theXNoise, float x, float3 *n, float3 *dndx)
{
    global const int *thePx = (global const int *)theXNoise;
    global const float *theXT = (global const float *)(thePx + 4 * PERMSIZE);
    global const float *theYT = theXT + RTABLE_SIZE;
    global const float *theZT = theYT + RTABLE_SIZE;

    const float F1 = 0.41421356237309515f;
    const float G1 = 0.29289321881345254f;
    const float G1COMP = 0.7071067811865475f;
    const float NORM1 = 26.82f;

    x *= FREQ_ADJUST_OLD; // *= FREQ_ADJUST
    float s = x * F1;
    int i = floor(x + s);
    float st = i * G1;
    x -= (i - st);

    *n = *dndx = 0.0f;

    extrapolate1v(thePx, theZT, theXT, theYT, i, x, n, dndx);

    extrapolate1v(thePx, theZT, theXT, theYT, i + 1, x - G1COMP, n, dndx);

    *n = *n * NORM1 + 0.5f;
    *dndx *= NORM1 * FREQ_ADJUST_OLD;
}

float3 xnoisev1(global const void *theXNoise, float x)
{
	float3 n = 0, dndx = 0;
	xnoisev1d(theXNoise, x, &n, &dndx);
	return n;
}

inline void
extrapolate2(global const int *thePx, global const int *thePy,
			 global const float *theXT,
			 int xsb, int ysb, float x, float y,
             float *n, float *dndx, float *dndy)
{    
    float attn = pow2(x) + pow2(y);
    if (attn < 2.0f)
    {
        float t = pow4(2.0f - attn);
		int index = (thePx[xsb & PERMMASK] ^ thePy[ysb & PERMMASK]);
		float rx = theXT[index];
		float ry = theXT[index + 1];
		float b = rx * x + ry * y;
		*n += t * b;

		float t38 = -8 * pow3(2.0f - attn);
		*dndx += t38 * x * b + t * rx;
		*dndy += t38 * y * b + t * ry;
    }
}

inline void
extrapolate2v(global const int *thePx, global const int *thePy,
			  global const float *theXT, global const float *theYT, global const float *theZT,
			  int xsb, int ysb, float x, float y,
              float3 *n, float3 *dndx, float3 *dndy)
{    
    float attn = pow2(x) + pow2(y);
    if (attn < 2.0f)
    {
        float t = pow4(2.0f - attn);
		int index = (thePx[xsb & PERMMASK] ^ thePy[ysb & PERMMASK]);
		float3 rx = (float3)(theXT[index], theYT[index], theZT[index]);
		float3 ry = (float3)(theXT[index + 1], theYT[index + 1], theZT[index + 1]);
		float3 b = rx * x + ry * y;
		*n += t * b;

		float t38 = -8 * pow3(2.0f - attn);
		*dndx += t38 * x * b + t * rx;
		*dndy += t38 * y * b + t * ry;
    }
}


void xnoise2d(global const void *theXNoise, float2 p, float *n, float *dndx, float *dndy)
{

    global const int *thePx = (global const int *)theXNoise;
    global const int *thePy = thePx + PERMSIZE;
    global const float *theXT = (global const float *)(thePx + 4 * PERMSIZE);

    const float STRETCH_CONSTANT_2D = -0.211324865405187f;    //(1/Math.sqrt(2+1)-1)/2;
    const float SQUISH_CONSTANT_2D = 0.366025403784439f;      //(Math.sqrt(2+1)-1)/2;
    const float NORM_CONSTANT_2D    = (0.5f / 47.0f) * 9.0f;

    p *= FREQ_ADJUST;
    float x = p.x, y = p.y;

    //Place input coordinates onto grid.
    float stretchOffset = (x + y) * STRETCH_CONSTANT_2D;
    float xs = x + stretchOffset;
    float ys = y + stretchOffset;

    //Floor to get grid coordinates of rhombus (stretched square) super-cell origin.
    int xsb = floor(xs);
    int ysb = floor(ys);

    //Skew out to get actual coordinates of rhombus origin. We'll need these later.
    float squishOffset = (xsb + ysb) * SQUISH_CONSTANT_2D;
    float xb = xsb + squishOffset;
    float yb = ysb + squishOffset;

    //Compute grid coordinates relative to rhombus origin.
    float xins = xs - xsb;
    float yins = ys - ysb;

    //Sum those together to get a value that determines which region we're in.
    float inSum = xins + yins;

    //Positions relative to origin point.
    float dx0 = x - xb;
    float dy0 = y - yb;

    //We'll be defining these inside the next block and using them afterwards.
    float dx_ext, dy_ext;
    int xsv_ext, ysv_ext;

    *n = *dndx = *dndy = 0.0f;
    //Contribution (1,0)
    float dx1 = dx0 - 1 - SQUISH_CONSTANT_2D;
    float dy1 = dy0 - 0 - SQUISH_CONSTANT_2D;
    extrapolate2(thePx, thePy, theXT, xsb + 1, ysb + 0, dx1, dy1, n, dndx, dndy);

    //Contribution (0,1)
    float dx2 = dx0 - 0 - SQUISH_CONSTANT_2D;
    float dy2 = dy0 - 1 - SQUISH_CONSTANT_2D;
    extrapolate2(thePx, thePy, theXT, xsb + 0, ysb + 1, dx2, dy2, n, dndx, dndy);

    if (inSum <= 1)   //We're inside the triangle (2-Simplex) at (0,0)
    {
        float zins = 1 - inSum;
        if (zins > xins || zins > yins)   //(0,0) is one of the closest two triangular vertices
        {
            if (xins > yins)
            {
                xsv_ext = xsb + 1;
                ysv_ext = ysb - 1;
                dx_ext = dx0 - 1;
                dy_ext = dy0 + 1;
            }
            else
            {
                xsv_ext = xsb - 1;
                ysv_ext = ysb + 1;
                dx_ext = dx0 + 1;
                dy_ext = dy0 - 1;
            }
        }
        else     //(1,0) and (0,1) are the closest two vertices.
        {
            xsv_ext = xsb + 1;
            ysv_ext = ysb + 1;
            dx_ext = dx0 - 1 - 2 * SQUISH_CONSTANT_2D;
            dy_ext = dy0 - 1 - 2 * SQUISH_CONSTANT_2D;
        }
    }
    else     //We're inside the triangle (2-Simplex) at (1,1)
    {
        float zins = 2 - inSum;
        if (zins < xins || zins < yins)   //(0,0) is one of the closest two triangular vertices
        {
            if (xins > yins)
            {
                xsv_ext = xsb + 2;
                ysv_ext = ysb + 0;
                dx_ext = dx0 - 2 - 2 * SQUISH_CONSTANT_2D;
                dy_ext = dy0 + 0 - 2 * SQUISH_CONSTANT_2D;
            }
            else
            {
                xsv_ext = xsb + 0;
                ysv_ext = ysb + 2;
                dx_ext = dx0 + 0 - 2 * SQUISH_CONSTANT_2D;
                dy_ext = dy0 - 2 - 2 * SQUISH_CONSTANT_2D;
            }
        }
        else     //(1,0) and (0,1) are the closest two vertices.
        {
            dx_ext = dx0;
            dy_ext = dy0;
            xsv_ext = xsb;
            ysv_ext = ysb;
        }
        xsb += 1;
        ysb += 1;
        dx0 = dx0 - 1 - 2 * SQUISH_CONSTANT_2D;
        dy0 = dy0 - 1 - 2 * SQUISH_CONSTANT_2D;
    }

    //Contribution (0,0) or (1,1)
    extrapolate2(thePx, thePy, theXT, xsb, ysb, dx0, dy0, n, dndx, dndy);

    //Extra Vertex
    extrapolate2(thePx, thePy, theXT, xsv_ext, ysv_ext, dx_ext, dy_ext, n, dndx, dndy);

    *n = *n * NORM_CONSTANT_2D + 0.5f;
    *dndx *= NORM_CONSTANT_2D * FREQ_ADJUST;
    *dndy *= NORM_CONSTANT_2D * FREQ_ADJUST;
}

float xnoise2(global const void *theXNoise, float2 p)
{
	float n = 0, dndx = 0, dndy = 0;
	xnoise2d(theXNoise, p, &n, &dndx, &dndy);
	return n;
}

void xnoisev2d(global const void *theXNoise, float2 p, float3 *n, float3 *dndx, float3 *dndy)
{
    global const int *thePx = (global const int *)theXNoise;
    global const int *thePy = thePx + PERMSIZE;
    global const float *theXT = (global const float *)(thePx + 4 * PERMSIZE);
    global const float *theYT = theXT + RTABLE_SIZE;
    global const float *theZT = theYT + RTABLE_SIZE;

    const float STRETCH_CONSTANT_2D = -0.211324865405187f;    //(1/Math.sqrt(2+1)-1)/2;
    const float SQUISH_CONSTANT_2D = 0.366025403784439f;      //(Math.sqrt(2+1)-1)/2;
    const float NORM_CONSTANT_2D    = (0.5f / 47.0f) * 9.0f;

    p *= FREQ_ADJUST;
    float x = p.x, y = p.y;

    //Place input coordinates onto grid.
    float stretchOffset = (x + y) * STRETCH_CONSTANT_2D;
    float xs = x + stretchOffset;
    float ys = y + stretchOffset;

    //Floor to get grid coordinates of rhombus (stretched square) super-cell origin.
    int xsb = floor(xs);
    int ysb = floor(ys);

    //Skew out to get actual coordinates of rhombus origin. We'll need these later.
    float squishOffset = (xsb + ysb) * SQUISH_CONSTANT_2D;
    float xb = xsb + squishOffset;
    float yb = ysb + squishOffset;

    //Compute grid coordinates relative to rhombus origin.
    float xins = xs - xsb;
    float yins = ys - ysb;

    //Sum those together to get a value that determines which region we're in.
    float inSum = xins + yins;

    //Positions relative to origin point.
    float dx0 = x - xb;
    float dy0 = y - yb;

    //We'll be defining these inside the next block and using them afterwards.
    float dx_ext, dy_ext;
    int xsv_ext, ysv_ext;

    *n = *dndx = *dndy = 0.0f;
    //Contribution (1,0)
    float dx1 = dx0 - 1 - SQUISH_CONSTANT_2D;
    float dy1 = dy0 - 0 - SQUISH_CONSTANT_2D;
    extrapolate2v(thePx, thePy, theXT, theYT, theZT, xsb + 1, ysb + 0, dx1, dy1, n, dndx, dndy);

    //Contribution (0,1)
    float dx2 = dx0 - 0 - SQUISH_CONSTANT_2D;
    float dy2 = dy0 - 1 - SQUISH_CONSTANT_2D;
    extrapolate2v(thePx, thePy, theXT, theYT, theZT, xsb + 0, ysb + 1, dx2, dy2, n, dndx, dndy);

    if (inSum <= 1)   //We're inside the triangle (2-Simplex) at (0,0)
    {
        float zins = 1 - inSum;
        if (zins > xins || zins > yins)   //(0,0) is one of the closest two triangular vertices
        {
            if (xins > yins)
            {
                xsv_ext = xsb + 1;
                ysv_ext = ysb - 1;
                dx_ext = dx0 - 1;
                dy_ext = dy0 + 1;
            }
            else
            {
                xsv_ext = xsb - 1;
                ysv_ext = ysb + 1;
                dx_ext = dx0 + 1;
                dy_ext = dy0 - 1;
            }
        }
        else     //(1,0) and (0,1) are the closest two vertices.
        {
            xsv_ext = xsb + 1;
            ysv_ext = ysb + 1;
            dx_ext = dx0 - 1 - 2 * SQUISH_CONSTANT_2D;
            dy_ext = dy0 - 1 - 2 * SQUISH_CONSTANT_2D;
        }
    }
    else     //We're inside the triangle (2-Simplex) at (1,1)
    {
        float zins = 2 - inSum;
        if (zins < xins || zins < yins)   //(0,0) is one of the closest two triangular vertices
        {
            if (xins > yins)
            {
                xsv_ext = xsb + 2;
                ysv_ext = ysb + 0;
                dx_ext = dx0 - 2 - 2 * SQUISH_CONSTANT_2D;
                dy_ext = dy0 + 0 - 2 * SQUISH_CONSTANT_2D;
            }
            else
            {
                xsv_ext = xsb + 0;
                ysv_ext = ysb + 2;
                dx_ext = dx0 + 0 - 2 * SQUISH_CONSTANT_2D;
                dy_ext = dy0 - 2 - 2 * SQUISH_CONSTANT_2D;
            }
        }
        else     //(1,0) and (0,1) are the closest two vertices.
        {
            dx_ext = dx0;
            dy_ext = dy0;
            xsv_ext = xsb;
            ysv_ext = ysb;
        }
        xsb += 1;
        ysb += 1;
        dx0 = dx0 - 1 - 2 * SQUISH_CONSTANT_2D;
        dy0 = dy0 - 1 - 2 * SQUISH_CONSTANT_2D;
    }

    //Contribution (0,0) or (1,1)
    extrapolate2v(thePx, thePy, theXT, theYT, theZT, xsb, ysb, dx0, dy0, n, dndx, dndy);

    //Extra Vertex
    extrapolate2v(thePx, thePy, theXT, theYT, theZT, xsv_ext, ysv_ext, dx_ext, dy_ext, n, dndx, dndy);

    *n = *n * NORM_CONSTANT_2D + 0.5f;
    *dndx *= NORM_CONSTANT_2D * FREQ_ADJUST;
    *dndy *= NORM_CONSTANT_2D * FREQ_ADJUST;
}

float3 xnoisev2(global const void *theXNoise, float2 p)
{
	float3 n = 0, dndx = 0, dndy = 0;
	xnoisev2d(theXNoise, p, &n, &dndx, &dndy);
	return n;
}

// 2D curlnoise.  Equivalent to VEX:
// vector curlxnoise2d( float; float )
float2 curlxnoise2(global const void *theXNoise, float2 p)
{
	float n = 0, dndx = 0, dndy = 0;
	xnoise2d(theXNoise, p, &n, &dndx, &dndy);
	return (float2) (dndy, -dndx);
}

inline void
extrapolate3(global const int *thePx, global const int *thePy, global const int *thePz,
			 global const float *theXT,
             int xsb, int ysb, int zsb,
             float x, float y, float z,
             float *n, float *dndx, float *dndy, float *dndz)
{
    float attn = pow2(x) + pow2(y) + pow2(z);
    if (attn < 2.0f)
    {
        float t = pow4(2.0f - attn);
		int index = (thePx[xsb & PERMMASK] ^ thePy[ysb & PERMMASK] ^ thePz[zsb & PERMMASK]);
        float rx = theXT[index];
        float ry = theXT[index + 1];
        float rz = theXT[index + 2];
        float b = rx * x + ry * y + rz * z;
        *n += t * b;

        float t38 = -8 * pow3(2.0f - attn);
        *dndx += t38 * x * b + t * rx;
        *dndy += t38 * y * b + t * ry;
        *dndz += t38 * z * b + t * rz;
    }
}

inline void
extrapolate3v(global const int *thePx, global const int *thePy, global const int *thePz,
			 global const float *theXT, global const float *theYT, global const float *theZT,
             int xsb, int ysb, int zsb,
             float x, float y, float z,
             float3 *n, float3 *dndx, float3 *dndy, float3 *dndz)
{
    float attn = pow2(x) + pow2(y) + pow2(z);
    if (attn < 2.0f)
    {
        float t = pow4(2.0f - attn);
		int index = (thePx[xsb & PERMMASK] ^ thePy[ysb & PERMMASK] ^ thePz[zsb & PERMMASK]);
		float3 rx = (float3)(theXT[index], theYT[index], theZT[index]);
		float3 ry = (float3)(theXT[index+1], theYT[index+1], theZT[index+1]);
		float3 rz = (float3)(theXT[index+2], theYT[index+2], theZT[index+2]);
        float3 b = rx * x + ry * y + rz * z;
        *n += t * b;

        float t38 = -8 * pow3(2.0f - attn);
        *dndx += t38 * x * b + t * rx;
        *dndy += t38 * y * b + t * ry;
        *dndz += t38 * z * b + t * rz;
    }
}

void xnoise3d(global const void *theXNoise, float3 p,
			  float *n, float *dndx, float *dndy, float *dndz)
{
    const float STRETCH_CONSTANT_3D = (-1.0f / 6.0f); // (1 / sqrt(3 + 1) - 1) / 3
    const float SQUISH_CONSTANT_3D  = (1.0f / 3.0f);  // (sqrt(3 + 1) - 1) / 3
    const float NORM_CONSTANT_3D    = (0.5f / 103.0f) * 20.0f;

    global const int *thePx = (global const int *)theXNoise;
    global const int *thePy = thePx + PERMSIZE;
    global const int *thePz = thePy + PERMSIZE;
    global const float *theXT = (global const float *)(thePx + 4 * PERMSIZE);

    p *= FREQ_ADJUST;
    float x = p.x, y = p.y, z = p.z;

    //Place input coordinates on simplectic honeycomb.
    float stretchOffset = (x + y + z) * STRETCH_CONSTANT_3D;
    float xs = x + stretchOffset;
    float ys = y + stretchOffset;
    float zs = z + stretchOffset;

    //Floor to get simplectic honeycomb coordinates of rhombohedron (stretched cube) super-cell origin.
    int xsb = floor(xs);
    int ysb = floor(ys);
    int zsb = floor(zs);

    //Skew out to get actual coordinates of rhombohedron origin. We'll need these later.
    float squishOffset = (xsb + ysb + zsb) * SQUISH_CONSTANT_3D;
    float xb = xsb + squishOffset;
    float yb = ysb + squishOffset;
    float zb = zsb + squishOffset;

    //Compute simplectic honeycomb coordinates relative to rhombohedral origin.
    float xins = xs - xsb;
    float yins = ys - ysb;
    float zins = zs - zsb;

    //Sum those together to get a value that determines which region we're in.
    float inSum = xins + yins + zins;

    //Positions relative to origin point.
    float dx0 = x - xb;
    float dy0 = y - yb;
    float dz0 = z - zb;

    //We'll be defining these inside the next block and using them afterwards.
    float dx_ext0, dy_ext0, dz_ext0;
    float dx_ext1, dy_ext1, dz_ext1;
    int xsv_ext0, ysv_ext0, zsv_ext0;
    int xsv_ext1, ysv_ext1, zsv_ext1;

    *n = *dndx = *dndy = *dndz = 0.0f;

    if (inSum <= 1)   //We're inside the tetrahedron (3-Simplex) at (0,0,0)
    {

        //Determine which two of (0,0,1), (0,1,0), (1,0,0) are closest.
        uchar aPoint = 0x01;
        float aScore = xins;
        uchar bPoint = 0x02;
        float bScore = yins;
        if (aScore >= bScore && zins > bScore)
        {
            bScore = zins;
            bPoint = 0x04;
        }
        else if (aScore < bScore && zins > aScore)
        {
            aScore = zins;
            aPoint = 0x04;
        }

        //Now we determine the two lattice points not part of the tetrahedron that may contribute.
        //This depends on the closest two tetrahedral vertices, including (0,0,0)
        float wins = 1 - inSum;
        if (wins > aScore || wins > bScore)   //(0,0,0) is one of the closest two tetrahedral vertices.
        {
            uchar c = (bScore > aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.

            if ((c & 0x01) == 0)
            {
                xsv_ext0 = xsb - 1;
                xsv_ext1 = xsb;
                dx_ext0 = dx0 + 1;
                dx_ext1 = dx0;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb + 1;
                dx_ext0 = dx_ext1 = dx0 - 1;
            }

            if ((c & 0x02) == 0)
            {
                ysv_ext0 = ysv_ext1 = ysb;
                dy_ext0 = dy_ext1 = dy0;
                if ((c & 0x01) == 0)
                {
                    ysv_ext1 -= 1;
                    dy_ext1 += 1;
                }
                else
                {
                    ysv_ext0 -= 1;
                    dy_ext0 += 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb + 1;
                dy_ext0 = dy_ext1 = dy0 - 1;
            }

            if ((c & 0x04) == 0)
            {
                zsv_ext0 = zsb;
                zsv_ext1 = zsb - 1;
                dz_ext0 = dz0;
                dz_ext1 = dz0 + 1;
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb + 1;
                dz_ext0 = dz_ext1 = dz0 - 1;
            }
        }
        else     //(0,0,0) is not one of the closest two tetrahedral vertices.
        {
            uchar c = (uchar)(aPoint | bPoint); //Our two extra vertices are determined by the closest two.

            if ((c & 0x01) == 0)
            {
                xsv_ext0 = xsb;
                xsv_ext1 = xsb - 1;
                dx_ext0 = dx0 - 2 * SQUISH_CONSTANT_3D;
                dx_ext1 = dx0 + 1 - SQUISH_CONSTANT_3D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb + 1;
                dx_ext0 = dx0 - 1 - 2 * SQUISH_CONSTANT_3D;
                dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_3D;
            }

            if ((c & 0x02) == 0)
            {
                ysv_ext0 = ysb;
                ysv_ext1 = ysb - 1;
                dy_ext0 = dy0 - 2 * SQUISH_CONSTANT_3D;
                dy_ext1 = dy0 + 1 - SQUISH_CONSTANT_3D;
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb + 1;
                dy_ext0 = dy0 - 1 - 2 * SQUISH_CONSTANT_3D;
                dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_3D;
            }

            if ((c & 0x04) == 0)
            {
                zsv_ext0 = zsb;
                zsv_ext1 = zsb - 1;
                dz_ext0 = dz0 - 2 * SQUISH_CONSTANT_3D;
                dz_ext1 = dz0 + 1 - SQUISH_CONSTANT_3D;
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb + 1;
                dz_ext0 = dz0 - 1 - 2 * SQUISH_CONSTANT_3D;
                dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_3D;
            }
        }

        //Contribution (0,0,0)
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 0, ysb + 0, zsb + 0, dx0, dy0, dz0, n, dndx, dndy, dndz);

        //Contribution (1,0,0)
        float dx1 = dx0 - 1 - SQUISH_CONSTANT_3D;
        float dy1 = dy0 - 0 - SQUISH_CONSTANT_3D;
        float dz1 = dz0 - 0 - SQUISH_CONSTANT_3D;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 1, ysb + 0, zsb + 0, dx1, dy1, dz1, n, dndx, dndy, dndz);

        //Contribution (0,1,0)
        float dx2 = dx0 - 0 - SQUISH_CONSTANT_3D;
        float dy2 = dy0 - 1 - SQUISH_CONSTANT_3D;
        float dz2 = dz1;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 0, ysb + 1, zsb + 0, dx2, dy2, dz2, n, dndx, dndy, dndz);

        //Contribution (0,0,1)
        float dx3 = dx2;
        float dy3 = dy1;
        float dz3 = dz0 - 1 - SQUISH_CONSTANT_3D;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 0, ysb + 0, zsb + 1, dx3, dy3, dz3, n, dndx, dndy, dndz);
    }
    else if (inSum >= 2)     //We're inside the tetrahedron (3-Simplex) at (1,1,1)
    {

        //Determine which two tetrahedral vertices are the closest, out of (1,1,0), (1,0,1), (0,1,1) but not (1,1,1).
        uchar aPoint = 0x06;
        float aScore = xins;
        uchar bPoint = 0x05;
        float bScore = yins;
        if (aScore <= bScore && zins < bScore)
        {
            bScore = zins;
            bPoint = 0x03;
        }
        else if (aScore > bScore && zins < aScore)
        {
            aScore = zins;
            aPoint = 0x03;
        }

        //Now we determine the two lattice points not part of the tetrahedron that may contribute.
        //This depends on the closest two tetrahedral vertices, including (1,1,1)
        float wins = 3 - inSum;
        if (wins < aScore || wins < bScore)   //(1,1,1) is one of the closest two tetrahedral vertices.
        {
            uchar c = (bScore < aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.

            if ((c & 0x01) != 0)
            {
                xsv_ext0 = xsb + 2;
                xsv_ext1 = xsb + 1;
                dx_ext0 = dx0 - 2 - 3 * SQUISH_CONSTANT_3D;
                dx_ext1 = dx0 - 1 - 3 * SQUISH_CONSTANT_3D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb;
                dx_ext0 = dx_ext1 = dx0 - 3 * SQUISH_CONSTANT_3D;
            }

            if ((c & 0x02) != 0)
            {
                ysv_ext0 = ysv_ext1 = ysb + 1;
                dy_ext0 = dy_ext1 = dy0 - 1 - 3 * SQUISH_CONSTANT_3D;
                if ((c & 0x01) != 0)
                {
                    ysv_ext1 += 1;
                    dy_ext1 -= 1;
                }
                else
                {
                    ysv_ext0 += 1;
                    dy_ext0 -= 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb;
                dy_ext0 = dy_ext1 = dy0 - 3 * SQUISH_CONSTANT_3D;
            }

            if ((c & 0x04) != 0)
            {
                zsv_ext0 = zsb + 1;
                zsv_ext1 = zsb + 2;
                dz_ext0 = dz0 - 1 - 3 * SQUISH_CONSTANT_3D;
                dz_ext1 = dz0 - 2 - 3 * SQUISH_CONSTANT_3D;
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb;
                dz_ext0 = dz_ext1 = dz0 - 3 * SQUISH_CONSTANT_3D;
            }
        }
        else     //(1,1,1) is not one of the closest two tetrahedral vertices.
        {
            uchar c = (uchar)(aPoint & bPoint); //Our two extra vertices are determined by the closest two.

            if ((c & 0x01) != 0)
            {
                xsv_ext0 = xsb + 1;
                xsv_ext1 = xsb + 2;
                dx_ext0 = dx0 - 1 - SQUISH_CONSTANT_3D;
                dx_ext1 = dx0 - 2 - 2 * SQUISH_CONSTANT_3D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb;
                dx_ext0 = dx0 - SQUISH_CONSTANT_3D;
                dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
            }

            if ((c & 0x02) != 0)
            {
                ysv_ext0 = ysb + 1;
                ysv_ext1 = ysb + 2;
                dy_ext0 = dy0 - 1 - SQUISH_CONSTANT_3D;
                dy_ext1 = dy0 - 2 - 2 * SQUISH_CONSTANT_3D;
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb;
                dy_ext0 = dy0 - SQUISH_CONSTANT_3D;
                dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
            }

            if ((c & 0x04) != 0)
            {
                zsv_ext0 = zsb + 1;
                zsv_ext1 = zsb + 2;
                dz_ext0 = dz0 - 1 - SQUISH_CONSTANT_3D;
                dz_ext1 = dz0 - 2 - 2 * SQUISH_CONSTANT_3D;
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb;
                dz_ext0 = dz0 - SQUISH_CONSTANT_3D;
                dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
            }
        }

        //Contribution (1,1,0)
        float dx3 = dx0 - 1 - 2 * SQUISH_CONSTANT_3D;
        float dy3 = dy0 - 1 - 2 * SQUISH_CONSTANT_3D;
        float dz3 = dz0 - 0 - 2 * SQUISH_CONSTANT_3D;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 1, ysb + 1, zsb + 0, dx3, dy3, dz3, n, dndx, dndy, dndz);

        //Contribution (1,0,1)
        float dx2 = dx3;
        float dy2 = dy0 - 0 - 2 * SQUISH_CONSTANT_3D;
        float dz2 = dz0 - 1 - 2 * SQUISH_CONSTANT_3D;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 1, ysb + 0, zsb + 1, dx2, dy2, dz2, n, dndx, dndy, dndz);

        //Contribution (0,1,1)
        float dx1 = dx0 - 0 - 2 * SQUISH_CONSTANT_3D;
        float dy1 = dy3;
        float dz1 = dz2;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 0, ysb + 1, zsb + 1, dx1, dy1, dz1, n, dndx, dndy, dndz);

        //Contribution (1,1,1)
        dx0 = dx0 - 1 - 3 * SQUISH_CONSTANT_3D;
        dy0 = dy0 - 1 - 3 * SQUISH_CONSTANT_3D;
        dz0 = dz0 - 1 - 3 * SQUISH_CONSTANT_3D;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 1, ysb + 1, zsb + 1, dx0, dy0, dz0, n, dndx, dndy, dndz);
    }
    else     //We're inside the octahedron (Rectified 3-Simplex) in between.
    {
        float aScore;
        uchar aPoint;
        bool aIsFurtherSide;
        float bScore;
        uchar bPoint;
        bool bIsFurtherSide;

        //Decide between point (0,0,1) and (1,1,0) as closest
        float p1 = xins + yins;
        if (p1 > 1)
        {
            aScore = p1 - 1;
            aPoint = 0x03;
            aIsFurtherSide = true;
        }
        else
        {
            aScore = 1 - p1;
            aPoint = 0x04;
            aIsFurtherSide = false;
        }

        //Decide between point (0,1,0) and (1,0,1) as closest
        float p2 = xins + zins;
        if (p2 > 1)
        {
            bScore = p2 - 1;
            bPoint = 0x05;
            bIsFurtherSide = true;
        }
        else
        {
            bScore = 1 - p2;
            bPoint = 0x02;
            bIsFurtherSide = false;
        }

        //The closest out of the two (1,0,0) and (0,1,1) will replace the furthest out of the two decided above, if closer.
        float p3 = yins + zins;
        if (p3 > 1)
        {
            float score = p3 - 1;
            if (aScore <= bScore && aScore < score)
            {
                aScore = score;
                aPoint = 0x06;
                aIsFurtherSide = true;
            }
            else if (aScore > bScore && bScore < score)
            {
                bScore = score;
                bPoint = 0x06;
                bIsFurtherSide = true;
            }
        }
        else
        {
            float score = 1 - p3;
            if (aScore <= bScore && aScore < score)
            {
                aScore = score;
                aPoint = 0x01;
                aIsFurtherSide = false;
            }
            else if (aScore > bScore && bScore < score)
            {
                bScore = score;
                bPoint = 0x01;
                bIsFurtherSide = false;
            }
        }

        //Where each of the two closest points are determines how the extra two vertices are calculated.
        if (aIsFurtherSide == bIsFurtherSide)
        {
            if (aIsFurtherSide)   //Both closest points on (1,1,1) side
            {

                //One of the two extra points is (1,1,1)
                dx_ext0 = dx0 - 1 - 3 * SQUISH_CONSTANT_3D;
                dy_ext0 = dy0 - 1 - 3 * SQUISH_CONSTANT_3D;
                dz_ext0 = dz0 - 1 - 3 * SQUISH_CONSTANT_3D;
                xsv_ext0 = xsb + 1;
                ysv_ext0 = ysb + 1;
                zsv_ext0 = zsb + 1;

                //Other extra point is based on the shared axis.
                uchar c = (uchar)(aPoint & bPoint);
                if ((c & 0x01) != 0)
                {
                    dx_ext1 = dx0 - 2 - 2 * SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb + 2;
                    ysv_ext1 = ysb;
                    zsv_ext1 = zsb;
                }
                else if ((c & 0x02) != 0)
                {
                    dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 - 2 - 2 * SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb;
                    ysv_ext1 = ysb + 2;
                    zsv_ext1 = zsb;
                }
                else
                {
                    dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 - 2 - 2 * SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb;
                    ysv_ext1 = ysb;
                    zsv_ext1 = zsb + 2;
                }
            }
            else    //Both closest points on (0,0,0) side
            {

                //One of the two extra points is (0,0,0)
                dx_ext0 = dx0;
                dy_ext0 = dy0;
                dz_ext0 = dz0;
                xsv_ext0 = xsb;
                ysv_ext0 = ysb;
                zsv_ext0 = zsb;

                //Other extra point is based on the omitted axis.
                uchar c = (uchar)(aPoint | bPoint);
                if ((c & 0x01) == 0)
                {
                    dx_ext1 = dx0 + 1 - SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb - 1;
                    ysv_ext1 = ysb + 1;
                    zsv_ext1 = zsb + 1;
                }
                else if ((c & 0x02) == 0)
                {
                    dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 + 1 - SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb + 1;
                    ysv_ext1 = ysb - 1;
                    zsv_ext1 = zsb + 1;
                }
                else
                {
                    dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 + 1 - SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb + 1;
                    ysv_ext1 = ysb + 1;
                    zsv_ext1 = zsb - 1;
                }
            }
        }
        else     //One point on (0,0,0) side, one point on (1,1,1) side
        {
            uchar c1, c2;
            if (aIsFurtherSide)
            {
                c1 = aPoint;
                c2 = bPoint;
            }
            else
            {
                c1 = bPoint;
                c2 = aPoint;
            }

            //One contribution is a permutation of (1,1,-1)
            if ((c1 & 0x01) == 0)
            {
                dx_ext0 = dx0 + 1 - SQUISH_CONSTANT_3D;
                dy_ext0 = dy0 - 1 - SQUISH_CONSTANT_3D;
                dz_ext0 = dz0 - 1 - SQUISH_CONSTANT_3D;
                xsv_ext0 = xsb - 1;
                ysv_ext0 = ysb + 1;
                zsv_ext0 = zsb + 1;
            }
            else if ((c1 & 0x02) == 0)
            {
                dx_ext0 = dx0 - 1 - SQUISH_CONSTANT_3D;
                dy_ext0 = dy0 + 1 - SQUISH_CONSTANT_3D;
                dz_ext0 = dz0 - 1 - SQUISH_CONSTANT_3D;
                xsv_ext0 = xsb + 1;
                ysv_ext0 = ysb - 1;
                zsv_ext0 = zsb + 1;
            }
            else
            {
                dx_ext0 = dx0 - 1 - SQUISH_CONSTANT_3D;
                dy_ext0 = dy0 - 1 - SQUISH_CONSTANT_3D;
                dz_ext0 = dz0 + 1 - SQUISH_CONSTANT_3D;
                xsv_ext0 = xsb + 1;
                ysv_ext0 = ysb + 1;
                zsv_ext0 = zsb - 1;
            }

            //One contribution is a permutation of (0,0,2)
            dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
            dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
            dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
            xsv_ext1 = xsb;
            ysv_ext1 = ysb;
            zsv_ext1 = zsb;
            if ((c2 & 0x01) != 0)
            {
                dx_ext1 -= 2;
                xsv_ext1 += 2;
            }
            else if ((c2 & 0x02) != 0)
            {
                dy_ext1 -= 2;
                ysv_ext1 += 2;
            }
            else
            {
                dz_ext1 -= 2;
                zsv_ext1 += 2;
            }
        }

        //Contribution (1,0,0)
        float dx1 = dx0 - 1 - SQUISH_CONSTANT_3D;
        float dy1 = dy0 - 0 - SQUISH_CONSTANT_3D;
        float dz1 = dz0 - 0 - SQUISH_CONSTANT_3D;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 1, ysb + 0, zsb + 0, dx1, dy1, dz1, n, dndx, dndy, dndz);

        //Contribution (0,1,0)
        float dx2 = dx0 - 0 - SQUISH_CONSTANT_3D;
        float dy2 = dy0 - 1 - SQUISH_CONSTANT_3D;
        float dz2 = dz1;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 0, ysb + 1, zsb + 0, dx2, dy2, dz2, n, dndx, dndy, dndz);

        //Contribution (0,0,1)
        float dx3 = dx2;
        float dy3 = dy1;
        float dz3 = dz0 - 1 - SQUISH_CONSTANT_3D;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 0, ysb + 0, zsb + 1, dx3, dy3, dz3, n, dndx, dndy, dndz);

        //Contribution (1,1,0)
        float dx4 = dx0 - 1 - 2 * SQUISH_CONSTANT_3D;
        float dy4 = dy0 - 1 - 2 * SQUISH_CONSTANT_3D;
        float dz4 = dz0 - 0 - 2 * SQUISH_CONSTANT_3D;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 1, ysb + 1, zsb + 0, dx4, dy4, dz4, n, dndx, dndy, dndz);

        //Contribution (1,0,1)
        float dx5 = dx4;
        float dy5 = dy0 - 0 - 2 * SQUISH_CONSTANT_3D;
        float dz5 = dz0 - 1 - 2 * SQUISH_CONSTANT_3D;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 1, ysb + 0, zsb + 1, dx5, dy5, dz5, n, dndx, dndy, dndz);

        //Contribution (0,1,1)
        float dx6 = dx0 - 0 - 2 * SQUISH_CONSTANT_3D;
        float dy6 = dy4;
        float dz6 = dz5;
        extrapolate3(thePx, thePy, thePz, theXT, xsb + 0, ysb + 1, zsb + 1, dx6, dy6, dz6, n, dndx, dndy, dndz);
    }

    //First extra vertex
    extrapolate3(thePx, thePy, thePz, theXT, xsv_ext0, ysv_ext0, zsv_ext0, dx_ext0, dy_ext0, dz_ext0, n, dndx, dndy, dndz);

    //Second extra vertex
    extrapolate3(thePx, thePy, thePz, theXT, xsv_ext1, ysv_ext1, zsv_ext1, dx_ext1, dy_ext1, dz_ext1, n, dndx, dndy, dndz);

	*n = *n * NORM_CONSTANT_3D + 0.5;
    *dndx *= NORM_CONSTANT_3D * FREQ_ADJUST;
    *dndy *= NORM_CONSTANT_3D * FREQ_ADJUST;
    *dndz *= NORM_CONSTANT_3D * FREQ_ADJUST;
}

float xnoise3(global const void *theXNoise, float3 p)
{
	float n = 0, dndx = 0, dndy = 0, dndz = 0;
	xnoise3d(theXNoise, p, &n, &dndx, &dndy, &dndz);
	return n;
}

void xnoisev3d(global const void *theXNoise, float3 p,
			    float3 *n, float3 *dndx, float3 *dndy, float3 *dndz)
{
    const float STRETCH_CONSTANT_3D = (-1.0f / 6.0f); // (1 / sqrt(3 + 1) - 1) / 3
    const float SQUISH_CONSTANT_3D  = (1.0f / 3.0f);  // (sqrt(3 + 1) - 1) / 3
    const float NORM_CONSTANT_3D    = (0.5f / 103.0f) * 20.0f;

    global const int *thePx = (global const int *)theXNoise;
    global const int *thePy = thePx + PERMSIZE;
    global const int *thePz = thePy + PERMSIZE;
    global const float *theXT = (global const float *)(thePx + 4 * PERMSIZE);
    global const float *theYT = theXT + RTABLE_SIZE;
    global const float *theZT = theYT + RTABLE_SIZE;

    p *= FREQ_ADJUST;
    float x = p.x, y = p.y, z = p.z;

    //Place input coordinates on simplectic honeycomb.
    float stretchOffset = (x + y + z) * STRETCH_CONSTANT_3D;
    float xs = x + stretchOffset;
    float ys = y + stretchOffset;
    float zs = z + stretchOffset;

    //Floor to get simplectic honeycomb coordinates of rhombohedron (stretched cube) super-cell origin.
    int xsb = floor(xs);
    int ysb = floor(ys);
    int zsb = floor(zs);

    //Skew out to get actual coordinates of rhombohedron origin. We'll need these later.
    float squishOffset = (xsb + ysb + zsb) * SQUISH_CONSTANT_3D;
    float xb = xsb + squishOffset;
    float yb = ysb + squishOffset;
    float zb = zsb + squishOffset;

    //Compute simplectic honeycomb coordinates relative to rhombohedral origin.
    float xins = xs - xsb;
    float yins = ys - ysb;
    float zins = zs - zsb;

    //Sum those together to get a value that determines which region we're in.
    float inSum = xins + yins + zins;

    //Positions relative to origin point.
    float dx0 = x - xb;
    float dy0 = y - yb;
    float dz0 = z - zb;

    //We'll be defining these inside the next block and using them afterwards.
    float dx_ext0, dy_ext0, dz_ext0;
    float dx_ext1, dy_ext1, dz_ext1;
    int xsv_ext0, ysv_ext0, zsv_ext0;
    int xsv_ext1, ysv_ext1, zsv_ext1;

    *n = *dndx = *dndy = *dndz = 0.0f;

    if (inSum <= 1)   //We're inside the tetrahedron (3-Simplex) at (0,0,0)
    {

        //Determine which two of (0,0,1), (0,1,0), (1,0,0) are closest.
        uchar aPoint = 0x01;
        float aScore = xins;
        uchar bPoint = 0x02;
        float bScore = yins;
        if (aScore >= bScore && zins > bScore)
        {
            bScore = zins;
            bPoint = 0x04;
        }
        else if (aScore < bScore && zins > aScore)
        {
            aScore = zins;
            aPoint = 0x04;
        }

        //Now we determine the two lattice points not part of the tetrahedron that may contribute.
        //This depends on the closest two tetrahedral vertices, including (0,0,0)
        float wins = 1 - inSum;
        if (wins > aScore || wins > bScore)   //(0,0,0) is one of the closest two tetrahedral vertices.
        {
            uchar c = (bScore > aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.

            if ((c & 0x01) == 0)
            {
                xsv_ext0 = xsb - 1;
                xsv_ext1 = xsb;
                dx_ext0 = dx0 + 1;
                dx_ext1 = dx0;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb + 1;
                dx_ext0 = dx_ext1 = dx0 - 1;
            }

            if ((c & 0x02) == 0)
            {
                ysv_ext0 = ysv_ext1 = ysb;
                dy_ext0 = dy_ext1 = dy0;
                if ((c & 0x01) == 0)
                {
                    ysv_ext1 -= 1;
                    dy_ext1 += 1;
                }
                else
                {
                    ysv_ext0 -= 1;
                    dy_ext0 += 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb + 1;
                dy_ext0 = dy_ext1 = dy0 - 1;
            }

            if ((c & 0x04) == 0)
            {
                zsv_ext0 = zsb;
                zsv_ext1 = zsb - 1;
                dz_ext0 = dz0;
                dz_ext1 = dz0 + 1;
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb + 1;
                dz_ext0 = dz_ext1 = dz0 - 1;
            }
        }
        else     //(0,0,0) is not one of the closest two tetrahedral vertices.
        {
            uchar c = (uchar)(aPoint | bPoint); //Our two extra vertices are determined by the closest two.

            if ((c & 0x01) == 0)
            {
                xsv_ext0 = xsb;
                xsv_ext1 = xsb - 1;
                dx_ext0 = dx0 - 2 * SQUISH_CONSTANT_3D;
                dx_ext1 = dx0 + 1 - SQUISH_CONSTANT_3D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb + 1;
                dx_ext0 = dx0 - 1 - 2 * SQUISH_CONSTANT_3D;
                dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_3D;
            }

            if ((c & 0x02) == 0)
            {
                ysv_ext0 = ysb;
                ysv_ext1 = ysb - 1;
                dy_ext0 = dy0 - 2 * SQUISH_CONSTANT_3D;
                dy_ext1 = dy0 + 1 - SQUISH_CONSTANT_3D;
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb + 1;
                dy_ext0 = dy0 - 1 - 2 * SQUISH_CONSTANT_3D;
                dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_3D;
            }

            if ((c & 0x04) == 0)
            {
                zsv_ext0 = zsb;
                zsv_ext1 = zsb - 1;
                dz_ext0 = dz0 - 2 * SQUISH_CONSTANT_3D;
                dz_ext1 = dz0 + 1 - SQUISH_CONSTANT_3D;
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb + 1;
                dz_ext0 = dz0 - 1 - 2 * SQUISH_CONSTANT_3D;
                dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_3D;
            }
        }

        //Contribution (0,0,0)
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 0, ysb + 0, zsb + 0, dx0, dy0, dz0, n, dndx, dndy, dndz);

        //Contribution (1,0,0)
        float dx1 = dx0 - 1 - SQUISH_CONSTANT_3D;
        float dy1 = dy0 - 0 - SQUISH_CONSTANT_3D;
        float dz1 = dz0 - 0 - SQUISH_CONSTANT_3D;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 0, dx1, dy1, dz1, n, dndx, dndy, dndz);

        //Contribution (0,1,0)
        float dx2 = dx0 - 0 - SQUISH_CONSTANT_3D;
        float dy2 = dy0 - 1 - SQUISH_CONSTANT_3D;
        float dz2 = dz1;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 0, dx2, dy2, dz2, n, dndx, dndy, dndz);

        //Contribution (0,0,1)
        float dx3 = dx2;
        float dy3 = dy1;
        float dz3 = dz0 - 1 - SQUISH_CONSTANT_3D;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 0, ysb + 0, zsb + 1, dx3, dy3, dz3, n, dndx, dndy, dndz);
    }
    else if (inSum >= 2)     //We're inside the tetrahedron (3-Simplex) at (1,1,1)
    {

        //Determine which two tetrahedral vertices are the closest, out of (1,1,0), (1,0,1), (0,1,1) but not (1,1,1).
        uchar aPoint = 0x06;
        float aScore = xins;
        uchar bPoint = 0x05;
        float bScore = yins;
        if (aScore <= bScore && zins < bScore)
        {
            bScore = zins;
            bPoint = 0x03;
        }
        else if (aScore > bScore && zins < aScore)
        {
            aScore = zins;
            aPoint = 0x03;
        }

        //Now we determine the two lattice points not part of the tetrahedron that may contribute.
        //This depends on the closest two tetrahedral vertices, including (1,1,1)
        float wins = 3 - inSum;
        if (wins < aScore || wins < bScore)   //(1,1,1) is one of the closest two tetrahedral vertices.
        {
            uchar c = (bScore < aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.

            if ((c & 0x01) != 0)
            {
                xsv_ext0 = xsb + 2;
                xsv_ext1 = xsb + 1;
                dx_ext0 = dx0 - 2 - 3 * SQUISH_CONSTANT_3D;
                dx_ext1 = dx0 - 1 - 3 * SQUISH_CONSTANT_3D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb;
                dx_ext0 = dx_ext1 = dx0 - 3 * SQUISH_CONSTANT_3D;
            }

            if ((c & 0x02) != 0)
            {
                ysv_ext0 = ysv_ext1 = ysb + 1;
                dy_ext0 = dy_ext1 = dy0 - 1 - 3 * SQUISH_CONSTANT_3D;
                if ((c & 0x01) != 0)
                {
                    ysv_ext1 += 1;
                    dy_ext1 -= 1;
                }
                else
                {
                    ysv_ext0 += 1;
                    dy_ext0 -= 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb;
                dy_ext0 = dy_ext1 = dy0 - 3 * SQUISH_CONSTANT_3D;
            }

            if ((c & 0x04) != 0)
            {
                zsv_ext0 = zsb + 1;
                zsv_ext1 = zsb + 2;
                dz_ext0 = dz0 - 1 - 3 * SQUISH_CONSTANT_3D;
                dz_ext1 = dz0 - 2 - 3 * SQUISH_CONSTANT_3D;
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb;
                dz_ext0 = dz_ext1 = dz0 - 3 * SQUISH_CONSTANT_3D;
            }
        }
        else     //(1,1,1) is not one of the closest two tetrahedral vertices.
        {
            uchar c = (uchar)(aPoint & bPoint); //Our two extra vertices are determined by the closest two.

            if ((c & 0x01) != 0)
            {
                xsv_ext0 = xsb + 1;
                xsv_ext1 = xsb + 2;
                dx_ext0 = dx0 - 1 - SQUISH_CONSTANT_3D;
                dx_ext1 = dx0 - 2 - 2 * SQUISH_CONSTANT_3D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb;
                dx_ext0 = dx0 - SQUISH_CONSTANT_3D;
                dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
            }

            if ((c & 0x02) != 0)
            {
                ysv_ext0 = ysb + 1;
                ysv_ext1 = ysb + 2;
                dy_ext0 = dy0 - 1 - SQUISH_CONSTANT_3D;
                dy_ext1 = dy0 - 2 - 2 * SQUISH_CONSTANT_3D;
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb;
                dy_ext0 = dy0 - SQUISH_CONSTANT_3D;
                dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
            }

            if ((c & 0x04) != 0)
            {
                zsv_ext0 = zsb + 1;
                zsv_ext1 = zsb + 2;
                dz_ext0 = dz0 - 1 - SQUISH_CONSTANT_3D;
                dz_ext1 = dz0 - 2 - 2 * SQUISH_CONSTANT_3D;
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb;
                dz_ext0 = dz0 - SQUISH_CONSTANT_3D;
                dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
            }
        }

        //Contribution (1,1,0)
        float dx3 = dx0 - 1 - 2 * SQUISH_CONSTANT_3D;
        float dy3 = dy0 - 1 - 2 * SQUISH_CONSTANT_3D;
        float dz3 = dz0 - 0 - 2 * SQUISH_CONSTANT_3D;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 1, ysb + 1, zsb + 0, dx3, dy3, dz3, n, dndx, dndy, dndz);

        //Contribution (1,0,1)
        float dx2 = dx3;
        float dy2 = dy0 - 0 - 2 * SQUISH_CONSTANT_3D;
        float dz2 = dz0 - 1 - 2 * SQUISH_CONSTANT_3D;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 1, dx2, dy2, dz2, n, dndx, dndy, dndz);

        //Contribution (0,1,1)
        float dx1 = dx0 - 0 - 2 * SQUISH_CONSTANT_3D;
        float dy1 = dy3;
        float dz1 = dz2;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 1, dx1, dy1, dz1, n, dndx, dndy, dndz);

        //Contribution (1,1,1)
        dx0 = dx0 - 1 - 3 * SQUISH_CONSTANT_3D;
        dy0 = dy0 - 1 - 3 * SQUISH_CONSTANT_3D;
        dz0 = dz0 - 1 - 3 * SQUISH_CONSTANT_3D;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 1, ysb + 1, zsb + 1, dx0, dy0, dz0, n, dndx, dndy, dndz);
    }
    else     //We're inside the octahedron (Rectified 3-Simplex) in between.
    {
        float aScore;
        uchar aPoint;
        bool aIsFurtherSide;
        float bScore;
        uchar bPoint;
        bool bIsFurtherSide;

        //Decide between point (0,0,1) and (1,1,0) as closest
        float p1 = xins + yins;
        if (p1 > 1)
        {
            aScore = p1 - 1;
            aPoint = 0x03;
            aIsFurtherSide = true;
        }
        else
        {
            aScore = 1 - p1;
            aPoint = 0x04;
            aIsFurtherSide = false;
        }

        //Decide between point (0,1,0) and (1,0,1) as closest
        float p2 = xins + zins;
        if (p2 > 1)
        {
            bScore = p2 - 1;
            bPoint = 0x05;
            bIsFurtherSide = true;
        }
        else
        {
            bScore = 1 - p2;
            bPoint = 0x02;
            bIsFurtherSide = false;
        }

        //The closest out of the two (1,0,0) and (0,1,1) will replace the furthest out of the two decided above, if closer.
        float p3 = yins + zins;
        if (p3 > 1)
        {
            float score = p3 - 1;
            if (aScore <= bScore && aScore < score)
            {
                aScore = score;
                aPoint = 0x06;
                aIsFurtherSide = true;
            }
            else if (aScore > bScore && bScore < score)
            {
                bScore = score;
                bPoint = 0x06;
                bIsFurtherSide = true;
            }
        }
        else
        {
            float score = 1 - p3;
            if (aScore <= bScore && aScore < score)
            {
                aScore = score;
                aPoint = 0x01;
                aIsFurtherSide = false;
            }
            else if (aScore > bScore && bScore < score)
            {
                bScore = score;
                bPoint = 0x01;
                bIsFurtherSide = false;
            }
        }

        //Where each of the two closest points are determines how the extra two vertices are calculated.
        if (aIsFurtherSide == bIsFurtherSide)
        {
            if (aIsFurtherSide)   //Both closest points on (1,1,1) side
            {

                //One of the two extra points is (1,1,1)
                dx_ext0 = dx0 - 1 - 3 * SQUISH_CONSTANT_3D;
                dy_ext0 = dy0 - 1 - 3 * SQUISH_CONSTANT_3D;
                dz_ext0 = dz0 - 1 - 3 * SQUISH_CONSTANT_3D;
                xsv_ext0 = xsb + 1;
                ysv_ext0 = ysb + 1;
                zsv_ext0 = zsb + 1;

                //Other extra point is based on the shared axis.
                uchar c = (uchar)(aPoint & bPoint);
                if ((c & 0x01) != 0)
                {
                    dx_ext1 = dx0 - 2 - 2 * SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb + 2;
                    ysv_ext1 = ysb;
                    zsv_ext1 = zsb;
                }
                else if ((c & 0x02) != 0)
                {
                    dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 - 2 - 2 * SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb;
                    ysv_ext1 = ysb + 2;
                    zsv_ext1 = zsb;
                }
                else
                {
                    dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 - 2 - 2 * SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb;
                    ysv_ext1 = ysb;
                    zsv_ext1 = zsb + 2;
                }
            }
            else    //Both closest points on (0,0,0) side
            {

                //One of the two extra points is (0,0,0)
                dx_ext0 = dx0;
                dy_ext0 = dy0;
                dz_ext0 = dz0;
                xsv_ext0 = xsb;
                ysv_ext0 = ysb;
                zsv_ext0 = zsb;

                //Other extra point is based on the omitted axis.
                uchar c = (uchar)(aPoint | bPoint);
                if ((c & 0x01) == 0)
                {
                    dx_ext1 = dx0 + 1 - SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb - 1;
                    ysv_ext1 = ysb + 1;
                    zsv_ext1 = zsb + 1;
                }
                else if ((c & 0x02) == 0)
                {
                    dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 + 1 - SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb + 1;
                    ysv_ext1 = ysb - 1;
                    zsv_ext1 = zsb + 1;
                }
                else
                {
                    dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_3D;
                    dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_3D;
                    dz_ext1 = dz0 + 1 - SQUISH_CONSTANT_3D;
                    xsv_ext1 = xsb + 1;
                    ysv_ext1 = ysb + 1;
                    zsv_ext1 = zsb - 1;
                }
            }
        }
        else     //One point on (0,0,0) side, one point on (1,1,1) side
        {
            uchar c1, c2;
            if (aIsFurtherSide)
            {
                c1 = aPoint;
                c2 = bPoint;
            }
            else
            {
                c1 = bPoint;
                c2 = aPoint;
            }

            //One contribution is a permutation of (1,1,-1)
            if ((c1 & 0x01) == 0)
            {
                dx_ext0 = dx0 + 1 - SQUISH_CONSTANT_3D;
                dy_ext0 = dy0 - 1 - SQUISH_CONSTANT_3D;
                dz_ext0 = dz0 - 1 - SQUISH_CONSTANT_3D;
                xsv_ext0 = xsb - 1;
                ysv_ext0 = ysb + 1;
                zsv_ext0 = zsb + 1;
            }
            else if ((c1 & 0x02) == 0)
            {
                dx_ext0 = dx0 - 1 - SQUISH_CONSTANT_3D;
                dy_ext0 = dy0 + 1 - SQUISH_CONSTANT_3D;
                dz_ext0 = dz0 - 1 - SQUISH_CONSTANT_3D;
                xsv_ext0 = xsb + 1;
                ysv_ext0 = ysb - 1;
                zsv_ext0 = zsb + 1;
            }
            else
            {
                dx_ext0 = dx0 - 1 - SQUISH_CONSTANT_3D;
                dy_ext0 = dy0 - 1 - SQUISH_CONSTANT_3D;
                dz_ext0 = dz0 + 1 - SQUISH_CONSTANT_3D;
                xsv_ext0 = xsb + 1;
                ysv_ext0 = ysb + 1;
                zsv_ext0 = zsb - 1;
            }

            //One contribution is a permutation of (0,0,2)
            dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
            dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
            dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
            xsv_ext1 = xsb;
            ysv_ext1 = ysb;
            zsv_ext1 = zsb;
            if ((c2 & 0x01) != 0)
            {
                dx_ext1 -= 2;
                xsv_ext1 += 2;
            }
            else if ((c2 & 0x02) != 0)
            {
                dy_ext1 -= 2;
                ysv_ext1 += 2;
            }
            else
            {
                dz_ext1 -= 2;
                zsv_ext1 += 2;
            }
        }

        //Contribution (1,0,0)
        float dx1 = dx0 - 1 - SQUISH_CONSTANT_3D;
        float dy1 = dy0 - 0 - SQUISH_CONSTANT_3D;
        float dz1 = dz0 - 0 - SQUISH_CONSTANT_3D;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 0, dx1, dy1, dz1, n, dndx, dndy, dndz);

        //Contribution (0,1,0)
        float dx2 = dx0 - 0 - SQUISH_CONSTANT_3D;
        float dy2 = dy0 - 1 - SQUISH_CONSTANT_3D;
        float dz2 = dz1;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 0, dx2, dy2, dz2, n, dndx, dndy, dndz);

        //Contribution (0,0,1)
        float dx3 = dx2;
        float dy3 = dy1;
        float dz3 = dz0 - 1 - SQUISH_CONSTANT_3D;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 0, ysb + 0, zsb + 1, dx3, dy3, dz3, n, dndx, dndy, dndz);

        //Contribution (1,1,0)
        float dx4 = dx0 - 1 - 2 * SQUISH_CONSTANT_3D;
        float dy4 = dy0 - 1 - 2 * SQUISH_CONSTANT_3D;
        float dz4 = dz0 - 0 - 2 * SQUISH_CONSTANT_3D;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 1, ysb + 1, zsb + 0, dx4, dy4, dz4, n, dndx, dndy, dndz);

        //Contribution (1,0,1)
        float dx5 = dx4;
        float dy5 = dy0 - 0 - 2 * SQUISH_CONSTANT_3D;
        float dz5 = dz0 - 1 - 2 * SQUISH_CONSTANT_3D;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 1, dx5, dy5, dz5, n, dndx, dndy, dndz);

        //Contribution (0,1,1)
        float dx6 = dx0 - 0 - 2 * SQUISH_CONSTANT_3D;
        float dy6 = dy4;
        float dz6 = dz5;
        extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 1, dx6, dy6, dz6, n, dndx, dndy, dndz);
    }

    //First extra vertex
    extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsv_ext0, ysv_ext0, zsv_ext0, dx_ext0, dy_ext0, dz_ext0, n, dndx, dndy, dndz);

    //Second extra vertex
    extrapolate3v(thePx, thePy, thePz, theXT, theYT, theZT, xsv_ext1, ysv_ext1, zsv_ext1, dx_ext1, dy_ext1, dz_ext1, n, dndx, dndy, dndz);

	*n = *n * NORM_CONSTANT_3D + 0.5f;
    *dndx *= NORM_CONSTANT_3D * FREQ_ADJUST;
    *dndy *= NORM_CONSTANT_3D * FREQ_ADJUST;
    *dndz *= NORM_CONSTANT_3D * FREQ_ADJUST;
}

float3 xnoisev3(global const void *theXNoise, float3 p)
{
	float3 n = 0, dndx = 0, dndy = 0, dndz = 0;
	xnoisev3d(theXNoise, p, &n, &dndx, &dndy, &dndz);
	return n;
}

float3 curlxnoise3(global const void *theXNoise, float3 p)
{
	float3 n = 0, dndx = 0, dndy = 0, dndz = 0;
	xnoisev3d(theXNoise, p, &n, &dndx, &dndy, &dndz);
	return (float3)(dndy.z - dndz.y,
                    dndz.x - dndx.z,
                    dndx.y - dndy.x);
}

inline void
extrapolate4(global const int *thePx, global const int *thePy, global const int *thePz, global const int *thePw,
			 global const float *theXT,
             int xsb, int ysb, int zsb, int wsb,
             float x, float y, float z, float w,
             float *n, float *dndx, float *dndy, float *dndz, float *dndw)
{
    float attn = pow2(x) + pow2(y) + pow2(z) + pow2(w);
    if (attn < 2.0f)
    {
        float t = pow4(2.0f - attn);
		int index = (thePx[xsb & PERMMASK] ^ thePy[ysb & PERMMASK] ^ thePz[zsb & PERMMASK] ^ thePw[wsb & PERMMASK]);
        float rx = theXT[index];
        float ry = theXT[index + 1];
        float rz = theXT[index + 2];
        float rw = theXT[index + 3];
        float b = rx * x + ry * y + rz * z + rw * w;
        *n += t * b;

        float t38 = -8 * pow3(2.0f - attn);
        *dndx += t38 * x * b + t * rx;
        *dndy += t38 * y * b + t * ry;
        *dndz += t38 * z * b + t * rz;
        *dndw += t38 * w * b + t * rw;
    }
}

inline void
extrapolate4v(global const int *thePx, global const int *thePy, global const int *thePz, global const int *thePw,
			 global const float *theXT, global const float *theYT, global const float *theZT,
             int xsb, int ysb, int zsb, int wsb,
             float x, float y, float z, float w,
             float3 *n, float3 *dndx, float3 *dndy, float3 *dndz, float3 *dndw)
{
    float attn = pow2(x) + pow2(y) + pow2(z) + pow2(w);
    if (attn < 2.0f)
    {
        float t = pow4(2.0f - attn);
		int index = (thePx[xsb & PERMMASK] ^ thePy[ysb & PERMMASK] ^ thePz[zsb & PERMMASK] ^ thePw[wsb & PERMMASK]);
		float3 rx = (float3)(theXT[index], theYT[index], theZT[index]);
		float3 ry = (float3)(theXT[index+1], theYT[index+1], theZT[index+1]);
		float3 rz = (float3)(theXT[index+2], theYT[index+2], theZT[index+2]);
		float3 rw = (float3)(theXT[index+3], theYT[index+3], theZT[index+3]);
        float3 b = rx * x + ry * y + rz * z + rw * w;
        *n += t * b;

        float t38 = -8 * pow3(2.0f - attn);
        *dndx += t38 * x * b + t * rx;
        *dndy += t38 * y * b + t * ry;
        *dndz += t38 * z * b + t * rz;
        *dndw += t38 * w * b + t * rw;
    }
}

void xnoise4d(global const void *theXNoise, float4 p,
			  float *n, float *dndx, float *dndy, float *dndz, float *dndw)
{
    const float STRETCH_CONSTANT_4D = -0.138196601125011f;    //(1/Math.sqrt(4+1)-1)/4;
    const float SQUISH_CONSTANT_4D  = 0.309016994374947f;      //(Math.sqrt(4+1)-1)/4;
    const float NORM_CONSTANT_4D    = (1.0f / 60.0f) * 5.0f;

    global const int *thePx = (global const int *)theXNoise;
    global const int *thePy = thePx + PERMSIZE;
    global const int *thePz = thePy + PERMSIZE;
    global const int *thePw = thePz + PERMSIZE;
    global const float *theXT = (global const float *)(thePw + PERMSIZE);

    p *= FREQ_ADJUST;
    float x = p.x, y = p.y, z = p.z, w = p.w;
    //Place input coordinates on simplectic honeycomb.
    float stretchOffset = (x + y + z + w) * STRETCH_CONSTANT_4D;
    float xs = x + stretchOffset;
    float ys = y + stretchOffset;
    float zs = z + stretchOffset;
    float ws = w + stretchOffset;

    //Floor to get simplectic honeycomb coordinates of rhombo-hypercube super-cell origin.
    int xsb = floor(xs);
    int ysb = floor(ys);
    int zsb = floor(zs);
    int wsb = floor(ws);

    //Skew out to get actual coordinates of stretched rhombo-hypercube origin. We'll need these later.
    float squishOffset = (xsb + ysb + zsb + wsb) * SQUISH_CONSTANT_4D;
    float xb = xsb + squishOffset;
    float yb = ysb + squishOffset;
    float zb = zsb + squishOffset;
    float wb = wsb + squishOffset;

    //Compute simplectic honeycomb coordinates relative to rhombo-hypercube origin.
    float xins = xs - xsb;
    float yins = ys - ysb;
    float zins = zs - zsb;
    float wins = ws - wsb;

    //Sum those together to get a value that determines which region we're in.
    float inSum = xins + yins + zins + wins;

    //Positions relative to origin point.
    float dx0 = x - xb;
    float dy0 = y - yb;
    float dz0 = z - zb;
    float dw0 = w - wb;

    //We'll be defining these inside the next block and using them afterwards.
    float dx_ext0, dy_ext0, dz_ext0, dw_ext0;
    float dx_ext1, dy_ext1, dz_ext1, dw_ext1;
    float dx_ext2, dy_ext2, dz_ext2, dw_ext2;
    int xsv_ext0, ysv_ext0, zsv_ext0, wsv_ext0;
    int xsv_ext1, ysv_ext1, zsv_ext1, wsv_ext1;
    int xsv_ext2, ysv_ext2, zsv_ext2, wsv_ext2;

    *n = *dndx = *dndy = *dndz = *dndw = 0.0f;

    if (inSum <= 1)   //We're inside the pentachoron (4-Simplex) at (0,0,0,0)
    {

        //Determine which two of (0,0,0,1), (0,0,1,0), (0,1,0,0), (1,0,0,0) are closest.
        uchar aPoint = 0x01;
        float aScore = xins;
        uchar bPoint = 0x02;
        float bScore = yins;
        if (aScore >= bScore && zins > bScore)
        {
            bScore = zins;
            bPoint = 0x04;
        }
        else if (aScore < bScore && zins > aScore)
        {
            aScore = zins;
            aPoint = 0x04;
        }
        if (aScore >= bScore && wins > bScore)
        {
            bScore = wins;
            bPoint = 0x08;
        }
        else if (aScore < bScore && wins > aScore)
        {
            aScore = wins;
            aPoint = 0x08;
        }

        //Now we determine the three lattice points not part of the pentachoron that may contribute.
        //This depends on the closest two pentachoron vertices, including (0,0,0,0)
        float uins = 1 - inSum;
        if (uins > aScore || uins > bScore)   //(0,0,0,0) is one of the closest two pentachoron vertices.
        {
            uchar c = (bScore > aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.
            if ((c & 0x01) == 0)
            {
                xsv_ext0 = xsb - 1;
                xsv_ext1 = xsv_ext2 = xsb;
                dx_ext0 = dx0 + 1;
                dx_ext1 = dx_ext2 = dx0;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb + 1;
                dx_ext0 = dx_ext1 = dx_ext2 = dx0 - 1;
            }

            if ((c & 0x02) == 0)
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
                dy_ext0 = dy_ext1 = dy_ext2 = dy0;
                if ((c & 0x01) == 0x01)
                {
                    ysv_ext0 -= 1;
                    dy_ext0 += 1;
                }
                else
                {
                    ysv_ext1 -= 1;
                    dy_ext1 += 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
                dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 1;
            }

            if ((c & 0x04) == 0)
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
                dz_ext0 = dz_ext1 = dz_ext2 = dz0;
                if ((c & 0x03) != 0)
                {
                    if ((c & 0x03) == 0x03)
                    {
                        zsv_ext0 -= 1;
                        dz_ext0 += 1;
                    }
                    else
                    {
                        zsv_ext1 -= 1;
                        dz_ext1 += 1;
                    }
                }
                else
                {
                    zsv_ext2 -= 1;
                    dz_ext2 += 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
                dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 1;
            }

            if ((c & 0x08) == 0)
            {
                wsv_ext0 = wsv_ext1 = wsb;
                wsv_ext2 = wsb - 1;
                dw_ext0 = dw_ext1 = dw0;
                dw_ext2 = dw0 + 1;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb + 1;
                dw_ext0 = dw_ext1 = dw_ext2 = dw0 - 1;
            }
        }
        else     //(0,0,0,0) is not one of the closest two pentachoron vertices.
        {
            uchar c = (uchar)(aPoint | bPoint); //Our three extra vertices are determined by the closest two.

            if ((c & 0x01) == 0)
            {
                xsv_ext0 = xsv_ext2 = xsb;
                xsv_ext1 = xsb - 1;
                dx_ext0 = dx0 - 2 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx0 + 1 - SQUISH_CONSTANT_4D;
                dx_ext2 = dx0 - SQUISH_CONSTANT_4D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb + 1;
                dx_ext0 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx_ext2 = dx0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c & 0x02) == 0)
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
                dy_ext0 = dy0 - 2 * SQUISH_CONSTANT_4D;
                dy_ext1 = dy_ext2 = dy0 - SQUISH_CONSTANT_4D;
                if ((c & 0x01) == 0x01)
                {
                    ysv_ext1 -= 1;
                    dy_ext1 += 1;
                }
                else
                {
                    ysv_ext2 -= 1;
                    dy_ext2 += 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
                dy_ext0 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dy_ext1 = dy_ext2 = dy0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c & 0x04) == 0)
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
                dz_ext0 = dz0 - 2 * SQUISH_CONSTANT_4D;
                dz_ext1 = dz_ext2 = dz0 - SQUISH_CONSTANT_4D;
                if ((c & 0x03) == 0x03)
                {
                    zsv_ext1 -= 1;
                    dz_ext1 += 1;
                }
                else
                {
                    zsv_ext2 -= 1;
                    dz_ext2 += 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
                dz_ext0 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dz_ext1 = dz_ext2 = dz0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c & 0x08) == 0)
            {
                wsv_ext0 = wsv_ext1 = wsb;
                wsv_ext2 = wsb - 1;
                dw_ext0 = dw0 - 2 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw0 - SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 + 1 - SQUISH_CONSTANT_4D;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb + 1;
                dw_ext0 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw_ext2 = dw0 - 1 - SQUISH_CONSTANT_4D;
            }
        }

        //Contribution (0,0,0,0)
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 0, zsb + 0, wsb + 0, dx0, dy0, dz0, dw0, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,0,0)
        float dx1 = dx0 - 1 - SQUISH_CONSTANT_4D;
        float dy1 = dy0 - 0 - SQUISH_CONSTANT_4D;
        float dz1 = dz0 - 0 - SQUISH_CONSTANT_4D;
        float dw1 = dw0 - 0 - SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 0, zsb + 0, wsb + 0, dx1, dy1, dz1, dw1, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,0,0)
        float dx2 = dx0 - 0 - SQUISH_CONSTANT_4D;
        float dy2 = dy0 - 1 - SQUISH_CONSTANT_4D;
        float dz2 = dz1;
        float dw2 = dw1;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 1, zsb + 0, wsb + 0, dx2, dy2, dz2, dw2, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,1,0)
        float dx3 = dx2;
        float dy3 = dy1;
        float dz3 = dz0 - 1 - SQUISH_CONSTANT_4D;
        float dw3 = dw1;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 0, zsb + 1, wsb + 0, dx3, dy3, dz3, dw3, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,0,1)
        float dx4 = dx2;
        float dy4 = dy1;
        float dz4 = dz1;
        float dw4 = dw0 - 1 - SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 0, zsb + 0, wsb + 1, dx4, dy4, dz4, dw4, n, dndx, dndy, dndz, dndw);
    }
    else if (inSum >= 3)     //We're inside the pentachoron (4-Simplex) at (1,1,1,1)
    {
        //Determine which two of (1,1,1,0), (1,1,0,1), (1,0,1,1), (0,1,1,1) are closest.
        uchar aPoint = 0x0E;
        float aScore = xins;
        uchar bPoint = 0x0D;
        float bScore = yins;
        if (aScore <= bScore && zins < bScore)
        {
            bScore = zins;
            bPoint = 0x0B;
        }
        else if (aScore > bScore && zins < aScore)
        {
            aScore = zins;
            aPoint = 0x0B;
        }
        if (aScore <= bScore && wins < bScore)
        {
            bScore = wins;
            bPoint = 0x07;
        }
        else if (aScore > bScore && wins < aScore)
        {
            aScore = wins;
            aPoint = 0x07;
        }

        //Now we determine the three lattice points not part of the pentachoron that may contribute.
        //This depends on the closest two pentachoron vertices, including (0,0,0,0)
        float uins = 4 - inSum;
        if (uins < aScore || uins < bScore)   //(1,1,1,1) is one of the closest two pentachoron vertices.
        {
            uchar c = (bScore < aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.

            if ((c & 0x01) != 0)
            {
                xsv_ext0 = xsb + 2;
                xsv_ext1 = xsv_ext2 = xsb + 1;
                dx_ext0 = dx0 - 2 - 4 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx_ext2 = dx0 - 1 - 4 * SQUISH_CONSTANT_4D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb;
                dx_ext0 = dx_ext1 = dx_ext2 = dx0 - 4 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x02) != 0)
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
                dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 1 - 4 * SQUISH_CONSTANT_4D;
                if ((c & 0x01) != 0)
                {
                    ysv_ext1 += 1;
                    dy_ext1 -= 1;
                }
                else
                {
                    ysv_ext0 += 1;
                    dy_ext0 -= 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
                dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 4 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x04) != 0)
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
                dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 1 - 4 * SQUISH_CONSTANT_4D;
                if ((c & 0x03) != 0x03)
                {
                    if ((c & 0x03) == 0)
                    {
                        zsv_ext0 += 1;
                        dz_ext0 -= 1;
                    }
                    else
                    {
                        zsv_ext1 += 1;
                        dz_ext1 -= 1;
                    }
                }
                else
                {
                    zsv_ext2 += 1;
                    dz_ext2 -= 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
                dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 4 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x08) != 0)
            {
                wsv_ext0 = wsv_ext1 = wsb + 1;
                wsv_ext2 = wsb + 2;
                dw_ext0 = dw_ext1 = dw0 - 1 - 4 * SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 - 2 - 4 * SQUISH_CONSTANT_4D;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb;
                dw_ext0 = dw_ext1 = dw_ext2 = dw0 - 4 * SQUISH_CONSTANT_4D;
            }
        }
        else     //(1,1,1,1) is not one of the closest two pentachoron vertices.
        {
            uchar c = (uchar)(aPoint & bPoint); //Our three extra vertices are determined by the closest two.

            if ((c & 0x01) != 0)
            {
                xsv_ext0 = xsv_ext2 = xsb + 1;
                xsv_ext1 = xsb + 2;
                dx_ext0 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx0 - 2 - 3 * SQUISH_CONSTANT_4D;
                dx_ext2 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb;
                dx_ext0 = dx0 - 2 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx_ext2 = dx0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x02) != 0)
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
                dy_ext0 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dy_ext1 = dy_ext2 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
                if ((c & 0x01) != 0)
                {
                    ysv_ext2 += 1;
                    dy_ext2 -= 1;
                }
                else
                {
                    ysv_ext1 += 1;
                    dy_ext1 -= 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
                dy_ext0 = dy0 - 2 * SQUISH_CONSTANT_4D;
                dy_ext1 = dy_ext2 = dy0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x04) != 0)
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
                dz_ext0 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dz_ext1 = dz_ext2 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
                if ((c & 0x03) != 0)
                {
                    zsv_ext2 += 1;
                    dz_ext2 -= 1;
                }
                else
                {
                    zsv_ext1 += 1;
                    dz_ext1 -= 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
                dz_ext0 = dz0 - 2 * SQUISH_CONSTANT_4D;
                dz_ext1 = dz_ext2 = dz0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x08) != 0)
            {
                wsv_ext0 = wsv_ext1 = wsb + 1;
                wsv_ext2 = wsb + 2;
                dw_ext0 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 - 2 - 3 * SQUISH_CONSTANT_4D;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb;
                dw_ext0 = dw0 - 2 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw_ext2 = dw0 - 3 * SQUISH_CONSTANT_4D;
            }
        }

        //Contribution (1,1,1,0)
        float dx4 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dy4 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dz4 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dw4 = dw0 - 3 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 1, zsb + 1, wsb + 0, dx4, dy4, dz4, dw4, n, dndx, dndy, dndz, dndw);

        //Contribution (1,1,0,1)
        float dx3 = dx4;
        float dy3 = dy4;
        float dz3 = dz0 - 3 * SQUISH_CONSTANT_4D;
        float dw3 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 1, zsb + 0, wsb + 1, dx3, dy3, dz3, dw3, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,1,1)
        float dx2 = dx4;
        float dy2 = dy0 - 3 * SQUISH_CONSTANT_4D;
        float dz2 = dz4;
        float dw2 = dw3;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 0, zsb + 1, wsb + 1, dx2, dy2, dz2, dw2, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,1,1)
        float dx1 = dx0 - 3 * SQUISH_CONSTANT_4D;
        float dz1 = dz4;
        float dy1 = dy4;
        float dw1 = dw3;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 1, zsb + 1, wsb + 1, dx1, dy1, dz1, dw1, n, dndx, dndy, dndz, dndw);

        //Contribution (1,1,1,1)
        dx0 = dx0 - 1 - 4 * SQUISH_CONSTANT_4D;
        dy0 = dy0 - 1 - 4 * SQUISH_CONSTANT_4D;
        dz0 = dz0 - 1 - 4 * SQUISH_CONSTANT_4D;
        dw0 = dw0 - 1 - 4 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 1, zsb + 1, wsb + 1, dx0, dy0, dz0, dw0, n, dndx, dndy, dndz, dndw);
    }
    else if (inSum <= 2)     //We're inside the first dispentachoron (Rectified 4-Simplex)
    {
        float aScore;
        uchar aPoint;
        bool aIsBiggerSide = true;
        float bScore;
        uchar bPoint;
        bool bIsBiggerSide = true;

        //Decide between (1,1,0,0) and (0,0,1,1)
        if (xins + yins > zins + wins)
        {
            aScore = xins + yins;
            aPoint = 0x03;
        }
        else
        {
            aScore = zins + wins;
            aPoint = 0x0C;
        }

        //Decide between (1,0,1,0) and (0,1,0,1)
        if (xins + zins > yins + wins)
        {
            bScore = xins + zins;
            bPoint = 0x05;
        }
        else
        {
            bScore = yins + wins;
            bPoint = 0x0A;
        }

        //Closer between (1,0,0,1) and (0,1,1,0) will replace the further of a and b, if closer.
        if (xins + wins > yins + zins)
        {
            float score = xins + wins;
            if (aScore >= bScore && score > bScore)
            {
                bScore = score;
                bPoint = 0x09;
            }
            else if (aScore < bScore && score > aScore)
            {
                aScore = score;
                aPoint = 0x09;
            }
        }
        else
        {
            float score = yins + zins;
            if (aScore >= bScore && score > bScore)
            {
                bScore = score;
                bPoint = 0x06;
            }
            else if (aScore < bScore && score > aScore)
            {
                aScore = score;
                aPoint = 0x06;
            }
        }

        //Decide if (1,0,0,0) is closer.
        float p1 = 2 - inSum + xins;
        if (aScore >= bScore && p1 > bScore)
        {
            bScore = p1;
            bPoint = 0x01;
            bIsBiggerSide = false;
        }
        else if (aScore < bScore && p1 > aScore)
        {
            aScore = p1;
            aPoint = 0x01;
            aIsBiggerSide = false;
        }

        //Decide if (0,1,0,0) is closer.
        float p2 = 2 - inSum + yins;
        if (aScore >= bScore && p2 > bScore)
        {
            bScore = p2;
            bPoint = 0x02;
            bIsBiggerSide = false;
        }
        else if (aScore < bScore && p2 > aScore)
        {
            aScore = p2;
            aPoint = 0x02;
            aIsBiggerSide = false;
        }

        //Decide if (0,0,1,0) is closer.
        float p3 = 2 - inSum + zins;
        if (aScore >= bScore && p3 > bScore)
        {
            bScore = p3;
            bPoint = 0x04;
            bIsBiggerSide = false;
        }
        else if (aScore < bScore && p3 > aScore)
        {
            aScore = p3;
            aPoint = 0x04;
            aIsBiggerSide = false;
        }

        //Decide if (0,0,0,1) is closer.
        float p4 = 2 - inSum + wins;
        if (aScore >= bScore && p4 > bScore)
        {
            bScore = p4;
            bPoint = 0x08;
            bIsBiggerSide = false;
        }
        else if (aScore < bScore && p4 > aScore)
        {
            aScore = p4;
            aPoint = 0x08;
            aIsBiggerSide = false;
        }

        //Where each of the two closest points are determines how the extra three vertices are calculated.
        if (aIsBiggerSide == bIsBiggerSide)
        {
            if (aIsBiggerSide)   //Both closest points on the bigger side
            {
                uchar c1 = (uchar)(aPoint | bPoint);
                uchar c2 = (uchar)(aPoint & bPoint);
                if ((c1 & 0x01) == 0)
                {
                    xsv_ext0 = xsb;
                    xsv_ext1 = xsb - 1;
                    dx_ext0 = dx0 - 3 * SQUISH_CONSTANT_4D;
                    dx_ext1 = dx0 + 1 - 2 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    xsv_ext0 = xsv_ext1 = xsb + 1;
                    dx_ext0 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    dx_ext1 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
                }

                if ((c1 & 0x02) == 0)
                {
                    ysv_ext0 = ysb;
                    ysv_ext1 = ysb - 1;
                    dy_ext0 = dy0 - 3 * SQUISH_CONSTANT_4D;
                    dy_ext1 = dy0 + 1 - 2 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    ysv_ext0 = ysv_ext1 = ysb + 1;
                    dy_ext0 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    dy_ext1 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
                }

                if ((c1 & 0x04) == 0)
                {
                    zsv_ext0 = zsb;
                    zsv_ext1 = zsb - 1;
                    dz_ext0 = dz0 - 3 * SQUISH_CONSTANT_4D;
                    dz_ext1 = dz0 + 1 - 2 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    zsv_ext0 = zsv_ext1 = zsb + 1;
                    dz_ext0 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    dz_ext1 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
                }

                if ((c1 & 0x08) == 0)
                {
                    wsv_ext0 = wsb;
                    wsv_ext1 = wsb - 1;
                    dw_ext0 = dw0 - 3 * SQUISH_CONSTANT_4D;
                    dw_ext1 = dw0 + 1 - 2 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    wsv_ext0 = wsv_ext1 = wsb + 1;
                    dw_ext0 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    dw_ext1 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
                }

                //One combination is a permutation of (0,0,0,2) based on c2
                xsv_ext2 = xsb;
                ysv_ext2 = ysb;
                zsv_ext2 = zsb;
                wsv_ext2 = wsb;
                dx_ext2 = dx0 - 2 * SQUISH_CONSTANT_4D;
                dy_ext2 = dy0 - 2 * SQUISH_CONSTANT_4D;
                dz_ext2 = dz0 - 2 * SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 - 2 * SQUISH_CONSTANT_4D;
                if ((c2 & 0x01) != 0)
                {
                    xsv_ext2 += 2;
                    dx_ext2 -= 2;
                }
                else if ((c2 & 0x02) != 0)
                {
                    ysv_ext2 += 2;
                    dy_ext2 -= 2;
                }
                else if ((c2 & 0x04) != 0)
                {
                    zsv_ext2 += 2;
                    dz_ext2 -= 2;
                }
                else
                {
                    wsv_ext2 += 2;
                    dw_ext2 -= 2;
                }

            }
            else     //Both closest points on the smaller side
            {
                //One of the two extra points is (0,0,0,0)
                xsv_ext2 = xsb;
                ysv_ext2 = ysb;
                zsv_ext2 = zsb;
                wsv_ext2 = wsb;
                dx_ext2 = dx0;
                dy_ext2 = dy0;
                dz_ext2 = dz0;
                dw_ext2 = dw0;

                //Other two points are based on the omitted axes.
                uchar c = (uchar)(aPoint | bPoint);

                if ((c & 0x01) == 0)
                {
                    xsv_ext0 = xsb - 1;
                    xsv_ext1 = xsb;
                    dx_ext0 = dx0 + 1 - SQUISH_CONSTANT_4D;
                    dx_ext1 = dx0 - SQUISH_CONSTANT_4D;
                }
                else
                {
                    xsv_ext0 = xsv_ext1 = xsb + 1;
                    dx_ext0 = dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_4D;
                }

                if ((c & 0x02) == 0)
                {
                    ysv_ext0 = ysv_ext1 = ysb;
                    dy_ext0 = dy_ext1 = dy0 - SQUISH_CONSTANT_4D;
                    if ((c & 0x01) == 0x01)
                    {
                        ysv_ext0 -= 1;
                        dy_ext0 += 1;
                    }
                    else
                    {
                        ysv_ext1 -= 1;
                        dy_ext1 += 1;
                    }
                }
                else
                {
                    ysv_ext0 = ysv_ext1 = ysb + 1;
                    dy_ext0 = dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_4D;
                }

                if ((c & 0x04) == 0)
                {
                    zsv_ext0 = zsv_ext1 = zsb;
                    dz_ext0 = dz_ext1 = dz0 - SQUISH_CONSTANT_4D;
                    if ((c & 0x03) == 0x03)
                    {
                        zsv_ext0 -= 1;
                        dz_ext0 += 1;
                    }
                    else
                    {
                        zsv_ext1 -= 1;
                        dz_ext1 += 1;
                    }
                }
                else
                {
                    zsv_ext0 = zsv_ext1 = zsb + 1;
                    dz_ext0 = dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_4D;
                }

                if ((c & 0x08) == 0)
                {
                    wsv_ext0 = wsb;
                    wsv_ext1 = wsb - 1;
                    dw_ext0 = dw0 - SQUISH_CONSTANT_4D;
                    dw_ext1 = dw0 + 1 - SQUISH_CONSTANT_4D;
                }
                else
                {
                    wsv_ext0 = wsv_ext1 = wsb + 1;
                    dw_ext0 = dw_ext1 = dw0 - 1 - SQUISH_CONSTANT_4D;
                }

            }
        }
        else     //One point on each "side"
        {
            uchar c1, c2;
            if (aIsBiggerSide)
            {
                c1 = aPoint;
                c2 = bPoint;
            }
            else
            {
                c1 = bPoint;
                c2 = aPoint;
            }

            //Two contributions are the bigger-sided point with each 0 replaced with -1.
            if ((c1 & 0x01) == 0)
            {
                xsv_ext0 = xsb - 1;
                xsv_ext1 = xsb;
                dx_ext0 = dx0 + 1 - SQUISH_CONSTANT_4D;
                dx_ext1 = dx0 - SQUISH_CONSTANT_4D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb + 1;
                dx_ext0 = dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x02) == 0)
            {
                ysv_ext0 = ysv_ext1 = ysb;
                dy_ext0 = dy_ext1 = dy0 - SQUISH_CONSTANT_4D;
                if ((c1 & 0x01) == 0x01)
                {
                    ysv_ext0 -= 1;
                    dy_ext0 += 1;
                }
                else
                {
                    ysv_ext1 -= 1;
                    dy_ext1 += 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb + 1;
                dy_ext0 = dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x04) == 0)
            {
                zsv_ext0 = zsv_ext1 = zsb;
                dz_ext0 = dz_ext1 = dz0 - SQUISH_CONSTANT_4D;
                if ((c1 & 0x03) == 0x03)
                {
                    zsv_ext0 -= 1;
                    dz_ext0 += 1;
                }
                else
                {
                    zsv_ext1 -= 1;
                    dz_ext1 += 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb + 1;
                dz_ext0 = dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x08) == 0)
            {
                wsv_ext0 = wsb;
                wsv_ext1 = wsb - 1;
                dw_ext0 = dw0 - SQUISH_CONSTANT_4D;
                dw_ext1 = dw0 + 1 - SQUISH_CONSTANT_4D;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsb + 1;
                dw_ext0 = dw_ext1 = dw0 - 1 - SQUISH_CONSTANT_4D;
            }

            //One contribution is a permutation of (0,0,0,2) based on the smaller-sided point
            xsv_ext2 = xsb;
            ysv_ext2 = ysb;
            zsv_ext2 = zsb;
            wsv_ext2 = wsb;
            dx_ext2 = dx0 - 2 * SQUISH_CONSTANT_4D;
            dy_ext2 = dy0 - 2 * SQUISH_CONSTANT_4D;
            dz_ext2 = dz0 - 2 * SQUISH_CONSTANT_4D;
            dw_ext2 = dw0 - 2 * SQUISH_CONSTANT_4D;
            if ((c2 & 0x01) != 0)
            {
                xsv_ext2 += 2;
                dx_ext2 -= 2;
            }
            else if ((c2 & 0x02) != 0)
            {
                ysv_ext2 += 2;
                dy_ext2 -= 2;
            }
            else if ((c2 & 0x04) != 0)
            {
                zsv_ext2 += 2;
                dz_ext2 -= 2;
            }
            else
            {
                wsv_ext2 += 2;
                dw_ext2 -= 2;
            }
        }

        //Contribution (1,0,0,0)
        float dx1 = dx0 - 1 - SQUISH_CONSTANT_4D;
        float dy1 = dy0 - 0 - SQUISH_CONSTANT_4D;
        float dz1 = dz0 - 0 - SQUISH_CONSTANT_4D;
        float dw1 = dw0 - 0 - SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 0, zsb + 0, wsb + 0, dx1, dy1, dz1, dw1, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,0,0)
        float dx2 = dx0 - 0 - SQUISH_CONSTANT_4D;
        float dy2 = dy0 - 1 - SQUISH_CONSTANT_4D;
        float dz2 = dz1;
        float dw2 = dw1;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 1, zsb + 0, wsb + 0, dx2, dy2, dz2, dw2, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,1,0)
        float dx3 = dx2;
        float dy3 = dy1;
        float dz3 = dz0 - 1 - SQUISH_CONSTANT_4D;
        float dw3 = dw1;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 0, zsb + 1, wsb + 0, dx3, dy3, dz3, dw3, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,0,1)
        float dx4 = dx2;
        float dy4 = dy1;
        float dz4 = dz1;
        float dw4 = dw0 - 1 - SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 0, zsb + 0, wsb + 1, dx4, dy4, dz4, dw4, n, dndx, dndy, dndz, dndw);

        //Contribution (1,1,0,0)
        float dx5 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy5 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz5 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw5 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 1, zsb + 0, wsb + 0, dx5, dy5, dz5, dw5, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,1,0)
        float dx6 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy6 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz6 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw6 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 0, zsb + 1, wsb + 0, dx6, dy6, dz6, dw6, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,0,1)
        float dx7 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy7 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz7 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw7 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 0, zsb + 0, wsb + 1, dx7, dy7, dz7, dw7, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,1,0)
        float dx8 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy8 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz8 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw8 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 1, zsb + 1, wsb + 0, dx8, dy8, dz8, dw8, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,0,1)
        float dx9 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy9 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz9 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw9 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 1, zsb + 0, wsb + 1, dx9, dy9, dz9, dw9, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,1,1)
        float dx10 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy10 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz10 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw10 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 0, zsb + 1, wsb + 1, dx10, dy10, dz10, dw10, n, dndx, dndy, dndz, dndw);
    }
    else     //We're inside the second dispentachoron (Rectified 4-Simplex)
    {
        float aScore;
        uchar aPoint;
        bool aIsBiggerSide = true;
        float bScore;
        uchar bPoint;
        bool bIsBiggerSide = true;

        //Decide between (0,0,1,1) and (1,1,0,0)
        if (xins + yins < zins + wins)
        {
            aScore = xins + yins;
            aPoint = 0x0C;
        }
        else
        {
            aScore = zins + wins;
            aPoint = 0x03;
        }

        //Decide between (0,1,0,1) and (1,0,1,0)
        if (xins + zins < yins + wins)
        {
            bScore = xins + zins;
            bPoint = 0x0A;
        }
        else
        {
            bScore = yins + wins;
            bPoint = 0x05;
        }

        //Closer between (0,1,1,0) and (1,0,0,1) will replace the further of a and b, if closer.
        if (xins + wins < yins + zins)
        {
            float score = xins + wins;
            if (aScore <= bScore && score < bScore)
            {
                bScore = score;
                bPoint = 0x06;
            }
            else if (aScore > bScore && score < aScore)
            {
                aScore = score;
                aPoint = 0x06;
            }
        }
        else
        {
            float score = yins + zins;
            if (aScore <= bScore && score < bScore)
            {
                bScore = score;
                bPoint = 0x09;
            }
            else if (aScore > bScore && score < aScore)
            {
                aScore = score;
                aPoint = 0x09;
            }
        }

        //Decide if (0,1,1,1) is closer.
        float p1 = 3 - inSum + xins;
        if (aScore <= bScore && p1 < bScore)
        {
            bScore = p1;
            bPoint = 0x0E;
            bIsBiggerSide = false;
        }
        else if (aScore > bScore && p1 < aScore)
        {
            aScore = p1;
            aPoint = 0x0E;
            aIsBiggerSide = false;
        }

        //Decide if (1,0,1,1) is closer.
        float p2 = 3 - inSum + yins;
        if (aScore <= bScore && p2 < bScore)
        {
            bScore = p2;
            bPoint = 0x0D;
            bIsBiggerSide = false;
        }
        else if (aScore > bScore && p2 < aScore)
        {
            aScore = p2;
            aPoint = 0x0D;
            aIsBiggerSide = false;
        }

        //Decide if (1,1,0,1) is closer.
        float p3 = 3 - inSum + zins;
        if (aScore <= bScore && p3 < bScore)
        {
            bScore = p3;
            bPoint = 0x0B;
            bIsBiggerSide = false;
        }
        else if (aScore > bScore && p3 < aScore)
        {
            aScore = p3;
            aPoint = 0x0B;
            aIsBiggerSide = false;
        }

        //Decide if (1,1,1,0) is closer.
        float p4 = 3 - inSum + wins;
        if (aScore <= bScore && p4 < bScore)
        {
            bScore = p4;
            bPoint = 0x07;
            bIsBiggerSide = false;
        }
        else if (aScore > bScore && p4 < aScore)
        {
            aScore = p4;
            aPoint = 0x07;
            aIsBiggerSide = false;
        }

        //Where each of the two closest points are determines how the extra three vertices are calculated.
        if (aIsBiggerSide == bIsBiggerSide)
        {
            if (aIsBiggerSide)   //Both closest points on the bigger side
            {
                uchar c1 = (uchar)(aPoint & bPoint);
                uchar c2 = (uchar)(aPoint | bPoint);

                //Two contributions are permutations of (0,0,0,1) and (0,0,0,2) based on c1
                xsv_ext0 = xsv_ext1 = xsb;
                ysv_ext0 = ysv_ext1 = ysb;
                zsv_ext0 = zsv_ext1 = zsb;
                wsv_ext0 = wsv_ext1 = wsb;
                dx_ext0 = dx0 - SQUISH_CONSTANT_4D;
                dy_ext0 = dy0 - SQUISH_CONSTANT_4D;
                dz_ext0 = dz0 - SQUISH_CONSTANT_4D;
                dw_ext0 = dw0 - SQUISH_CONSTANT_4D;
                dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_4D;
                dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_4D;
                dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw0 - 2 * SQUISH_CONSTANT_4D;
                if ((c1 & 0x01) != 0)
                {
                    xsv_ext0 += 1;
                    dx_ext0 -= 1;
                    xsv_ext1 += 2;
                    dx_ext1 -= 2;
                }
                else if ((c1 & 0x02) != 0)
                {
                    ysv_ext0 += 1;
                    dy_ext0 -= 1;
                    ysv_ext1 += 2;
                    dy_ext1 -= 2;
                }
                else if ((c1 & 0x04) != 0)
                {
                    zsv_ext0 += 1;
                    dz_ext0 -= 1;
                    zsv_ext1 += 2;
                    dz_ext1 -= 2;
                }
                else
                {
                    wsv_ext0 += 1;
                    dw_ext0 -= 1;
                    wsv_ext1 += 2;
                    dw_ext1 -= 2;
                }

                //One contribution is a permutation of (1,1,1,-1) based on c2
                xsv_ext2 = xsb + 1;
                ysv_ext2 = ysb + 1;
                zsv_ext2 = zsb + 1;
                wsv_ext2 = wsb + 1;
                dx_ext2 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dy_ext2 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dz_ext2 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
                if ((c2 & 0x01) == 0)
                {
                    xsv_ext2 -= 2;
                    dx_ext2 += 2;
                }
                else if ((c2 & 0x02) == 0)
                {
                    ysv_ext2 -= 2;
                    dy_ext2 += 2;
                }
                else if ((c2 & 0x04) == 0)
                {
                    zsv_ext2 -= 2;
                    dz_ext2 += 2;
                }
                else
                {
                    wsv_ext2 -= 2;
                    dw_ext2 += 2;
                }
            }
            else     //Both closest points on the smaller side
            {
                //One of the two extra points is (1,1,1,1)
                xsv_ext2 = xsb + 1;
                ysv_ext2 = ysb + 1;
                zsv_ext2 = zsb + 1;
                wsv_ext2 = wsb + 1;
                dx_ext2 = dx0 - 1 - 4 * SQUISH_CONSTANT_4D;
                dy_ext2 = dy0 - 1 - 4 * SQUISH_CONSTANT_4D;
                dz_ext2 = dz0 - 1 - 4 * SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 - 1 - 4 * SQUISH_CONSTANT_4D;

                //Other two points are based on the shared axes.
                uchar c = (uchar)(aPoint & bPoint);

                if ((c & 0x01) != 0)
                {
                    xsv_ext0 = xsb + 2;
                    xsv_ext1 = xsb + 1;
                    dx_ext0 = dx0 - 2 - 3 * SQUISH_CONSTANT_4D;
                    dx_ext1 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    xsv_ext0 = xsv_ext1 = xsb;
                    dx_ext0 = dx_ext1 = dx0 - 3 * SQUISH_CONSTANT_4D;
                }

                if ((c & 0x02) != 0)
                {
                    ysv_ext0 = ysv_ext1 = ysb + 1;
                    dy_ext0 = dy_ext1 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    if ((c & 0x01) == 0)
                    {
                        ysv_ext0 += 1;
                        dy_ext0 -= 1;
                    }
                    else
                    {
                        ysv_ext1 += 1;
                        dy_ext1 -= 1;
                    }
                }
                else
                {
                    ysv_ext0 = ysv_ext1 = ysb;
                    dy_ext0 = dy_ext1 = dy0 - 3 * SQUISH_CONSTANT_4D;
                }

                if ((c & 0x04) != 0)
                {
                    zsv_ext0 = zsv_ext1 = zsb + 1;
                    dz_ext0 = dz_ext1 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    if ((c & 0x03) == 0)
                    {
                        zsv_ext0 += 1;
                        dz_ext0 -= 1;
                    }
                    else
                    {
                        zsv_ext1 += 1;
                        dz_ext1 -= 1;
                    }
                }
                else
                {
                    zsv_ext0 = zsv_ext1 = zsb;
                    dz_ext0 = dz_ext1 = dz0 - 3 * SQUISH_CONSTANT_4D;
                }

                if ((c & 0x08) != 0)
                {
                    wsv_ext0 = wsb + 1;
                    wsv_ext1 = wsb + 2;
                    dw_ext0 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    dw_ext1 = dw0 - 2 - 3 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    wsv_ext0 = wsv_ext1 = wsb;
                    dw_ext0 = dw_ext1 = dw0 - 3 * SQUISH_CONSTANT_4D;
                }
            }
        }
        else     //One point on each "side"
        {
            uchar c1, c2;
            if (aIsBiggerSide)
            {
                c1 = aPoint;
                c2 = bPoint;
            }
            else
            {
                c1 = bPoint;
                c2 = aPoint;
            }

            //Two contributions are the bigger-sided point with each 1 replaced with 2.
            if ((c1 & 0x01) != 0)
            {
                xsv_ext0 = xsb + 2;
                xsv_ext1 = xsb + 1;
                dx_ext0 = dx0 - 2 - 3 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb;
                dx_ext0 = dx_ext1 = dx0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x02) != 0)
            {
                ysv_ext0 = ysv_ext1 = ysb + 1;
                dy_ext0 = dy_ext1 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
                if ((c1 & 0x01) == 0)
                {
                    ysv_ext0 += 1;
                    dy_ext0 -= 1;
                }
                else
                {
                    ysv_ext1 += 1;
                    dy_ext1 -= 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb;
                dy_ext0 = dy_ext1 = dy0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x04) != 0)
            {
                zsv_ext0 = zsv_ext1 = zsb + 1;
                dz_ext0 = dz_ext1 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
                if ((c1 & 0x03) == 0)
                {
                    zsv_ext0 += 1;
                    dz_ext0 -= 1;
                }
                else
                {
                    zsv_ext1 += 1;
                    dz_ext1 -= 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb;
                dz_ext0 = dz_ext1 = dz0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x08) != 0)
            {
                wsv_ext0 = wsb + 1;
                wsv_ext1 = wsb + 2;
                dw_ext0 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw0 - 2 - 3 * SQUISH_CONSTANT_4D;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsb;
                dw_ext0 = dw_ext1 = dw0 - 3 * SQUISH_CONSTANT_4D;
            }

            //One contribution is a permutation of (1,1,1,-1) based on the smaller-sided point
            xsv_ext2 = xsb + 1;
            ysv_ext2 = ysb + 1;
            zsv_ext2 = zsb + 1;
            wsv_ext2 = wsb + 1;
            dx_ext2 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
            dy_ext2 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
            dz_ext2 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
            dw_ext2 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
            if ((c2 & 0x01) == 0)
            {
                xsv_ext2 -= 2;
                dx_ext2 += 2;
            }
            else if ((c2 & 0x02) == 0)
            {
                ysv_ext2 -= 2;
                dy_ext2 += 2;
            }
            else if ((c2 & 0x04) == 0)
            {
                zsv_ext2 -= 2;
                dz_ext2 += 2;
            }
            else
            {
                wsv_ext2 -= 2;
                dw_ext2 += 2;
            }
        }

        //Contribution (1,1,1,0)
        float dx4 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dy4 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dz4 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dw4 = dw0 - 3 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 1, zsb + 1, wsb + 0, dx4, dy4, dz4, dw4, n, dndx, dndy, dndz, dndw);

        //Contribution (1,1,0,1)
        float dx3 = dx4;
        float dy3 = dy4;
        float dz3 = dz0 - 3 * SQUISH_CONSTANT_4D;
        float dw3 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 1, zsb + 0, wsb + 1, dx3, dy3, dz3, dw3, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,1,1)
        float dx2 = dx4;
        float dy2 = dy0 - 3 * SQUISH_CONSTANT_4D;
        float dz2 = dz4;
        float dw2 = dw3;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 0, zsb + 1, wsb + 1, dx2, dy2, dz2, dw2, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,1,1)
        float dx1 = dx0 - 3 * SQUISH_CONSTANT_4D;
        float dz1 = dz4;
        float dy1 = dy4;
        float dw1 = dw3;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 1, zsb + 1, wsb + 1, dx1, dy1, dz1, dw1, n, dndx, dndy, dndz, dndw);

        //Contribution (1,1,0,0)
        float dx5 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy5 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz5 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw5 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 1, zsb + 0, wsb + 0, dx5, dy5, dz5, dw5, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,1,0)
        float dx6 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy6 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz6 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw6 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 0, zsb + 1, wsb + 0, dx6, dy6, dz6, dw6, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,0,1)
        float dx7 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy7 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz7 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw7 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 1, ysb + 0, zsb + 0, wsb + 1, dx7, dy7, dz7, dw7, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,1,0)
        float dx8 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy8 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz8 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw8 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 1, zsb + 1, wsb + 0, dx8, dy8, dz8, dw8, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,0,1)
        float dx9 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy9 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz9 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw9 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 1, zsb + 0, wsb + 1, dx9, dy9, dz9, dw9, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,1,1)
        float dx10 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy10 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz10 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw10 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4(thePx, thePy, thePz, thePw, theXT, xsb + 0, ysb + 0, zsb + 1, wsb + 1, dx10, dy10, dz10, dw10, n, dndx, dndy, dndz, dndw);
    }

    //First extra vertex
    extrapolate4(thePx, thePy, thePz, thePw, theXT, xsv_ext0, ysv_ext0, zsv_ext0, wsv_ext0, dx_ext0, dy_ext0, dz_ext0, dw_ext0, n, dndx, dndy, dndz, dndw);

    //Second extra vertex
    extrapolate4(thePx, thePy, thePz, thePw, theXT, xsv_ext1, ysv_ext1, zsv_ext1, wsv_ext1, dx_ext1, dy_ext1, dz_ext1, dw_ext1, n, dndx, dndy, dndz, dndw);

    //Third extra vertex
    extrapolate4(thePx, thePy, thePz, thePw, theXT, xsv_ext2, ysv_ext2, zsv_ext2, wsv_ext2, dx_ext2, dy_ext2, dz_ext2, dw_ext2, n, dndx, dndy, dndz, dndw);

    *n = *n * NORM_CONSTANT_4D + 0.5f;
    *dndx *= NORM_CONSTANT_4D * FREQ_ADJUST;
    *dndy *= NORM_CONSTANT_4D * FREQ_ADJUST;
    *dndz *= NORM_CONSTANT_4D * FREQ_ADJUST;
    *dndw *= NORM_CONSTANT_4D * FREQ_ADJUST;
}

float xnoise4(global const void *theXNoise, float4 p)
{
	float n = 0, dndx = 0, dndy = 0, dndz = 0, dndw = 0;
	xnoise4d(theXNoise, p, &n, &dndx, &dndy, &dndz, &dndw);
	return n;
}

void xnoisev4d(global const void *theXNoise, float4 p,
			  float3 *n, float3 *dndx, float3 *dndy, float3 *dndz, float3 *dndw)
{
    global const int *thePx = (global const int *)theXNoise;
    global const int *thePy = thePx + PERMSIZE;
    global const int *thePz = thePy + PERMSIZE;
    global const int *thePw = thePz + PERMSIZE;
    global const float *theXT = (global const float *)(thePw + PERMSIZE);
    global const float *theYT = theXT + RTABLE_SIZE;
    global const float *theZT = theYT + RTABLE_SIZE;

    const float STRETCH_CONSTANT_4D = -0.138196601125011f;    //(1/Math.sqrt(4+1)-1)/4;
    const float SQUISH_CONSTANT_4D  = 0.309016994374947f;      //(Math.sqrt(4+1)-1)/4;
    const float NORM_CONSTANT_4D    = (1.0f / 60.0f) * 5.0f;

    p *= FREQ_ADJUST;
    float x = p.x, y = p.y, z = p.z, w = p.w;
    //Place input coordinates on simplectic honeycomb.
    float stretchOffset = (x + y + z + w) * STRETCH_CONSTANT_4D;
    float xs = x + stretchOffset;
    float ys = y + stretchOffset;
    float zs = z + stretchOffset;
    float ws = w + stretchOffset;

    //Floor to get simplectic honeycomb coordinates of rhombo-hypercube super-cell origin.
    int xsb = floor(xs);
    int ysb = floor(ys);
    int zsb = floor(zs);
    int wsb = floor(ws);

    //Skew out to get actual coordinates of stretched rhombo-hypercube origin. We'll need these later.
    float squishOffset = (xsb + ysb + zsb + wsb) * SQUISH_CONSTANT_4D;
    float xb = xsb + squishOffset;
    float yb = ysb + squishOffset;
    float zb = zsb + squishOffset;
    float wb = wsb + squishOffset;

    //Compute simplectic honeycomb coordinates relative to rhombo-hypercube origin.
    float xins = xs - xsb;
    float yins = ys - ysb;
    float zins = zs - zsb;
    float wins = ws - wsb;

    //Sum those together to get a value that determines which region we're in.
    float inSum = xins + yins + zins + wins;

    //Positions relative to origin point.
    float dx0 = x - xb;
    float dy0 = y - yb;
    float dz0 = z - zb;
    float dw0 = w - wb;

    //We'll be defining these inside the next block and using them afterwards.
    float dx_ext0, dy_ext0, dz_ext0, dw_ext0;
    float dx_ext1, dy_ext1, dz_ext1, dw_ext1;
    float dx_ext2, dy_ext2, dz_ext2, dw_ext2;
    int xsv_ext0, ysv_ext0, zsv_ext0, wsv_ext0;
    int xsv_ext1, ysv_ext1, zsv_ext1, wsv_ext1;
    int xsv_ext2, ysv_ext2, zsv_ext2, wsv_ext2;

    *n = *dndx = *dndy = *dndz = *dndw = 0.0f;

    if (inSum <= 1)   //We're inside the pentachoron (4-Simplex) at (0,0,0,0)
    {

        //Determine which two of (0,0,0,1), (0,0,1,0), (0,1,0,0), (1,0,0,0) are closest.
        uchar aPoint = 0x01;
        float aScore = xins;
        uchar bPoint = 0x02;
        float bScore = yins;
        if (aScore >= bScore && zins > bScore)
        {
            bScore = zins;
            bPoint = 0x04;
        }
        else if (aScore < bScore && zins > aScore)
        {
            aScore = zins;
            aPoint = 0x04;
        }
        if (aScore >= bScore && wins > bScore)
        {
            bScore = wins;
            bPoint = 0x08;
        }
        else if (aScore < bScore && wins > aScore)
        {
            aScore = wins;
            aPoint = 0x08;
        }

        //Now we determine the three lattice points not part of the pentachoron that may contribute.
        //This depends on the closest two pentachoron vertices, including (0,0,0,0)
        float uins = 1 - inSum;
        if (uins > aScore || uins > bScore)   //(0,0,0,0) is one of the closest two pentachoron vertices.
        {
            uchar c = (bScore > aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.
            if ((c & 0x01) == 0)
            {
                xsv_ext0 = xsb - 1;
                xsv_ext1 = xsv_ext2 = xsb;
                dx_ext0 = dx0 + 1;
                dx_ext1 = dx_ext2 = dx0;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb + 1;
                dx_ext0 = dx_ext1 = dx_ext2 = dx0 - 1;
            }

            if ((c & 0x02) == 0)
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
                dy_ext0 = dy_ext1 = dy_ext2 = dy0;
                if ((c & 0x01) == 0x01)
                {
                    ysv_ext0 -= 1;
                    dy_ext0 += 1;
                }
                else
                {
                    ysv_ext1 -= 1;
                    dy_ext1 += 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
                dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 1;
            }

            if ((c & 0x04) == 0)
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
                dz_ext0 = dz_ext1 = dz_ext2 = dz0;
                if ((c & 0x03) != 0)
                {
                    if ((c & 0x03) == 0x03)
                    {
                        zsv_ext0 -= 1;
                        dz_ext0 += 1;
                    }
                    else
                    {
                        zsv_ext1 -= 1;
                        dz_ext1 += 1;
                    }
                }
                else
                {
                    zsv_ext2 -= 1;
                    dz_ext2 += 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
                dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 1;
            }

            if ((c & 0x08) == 0)
            {
                wsv_ext0 = wsv_ext1 = wsb;
                wsv_ext2 = wsb - 1;
                dw_ext0 = dw_ext1 = dw0;
                dw_ext2 = dw0 + 1;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb + 1;
                dw_ext0 = dw_ext1 = dw_ext2 = dw0 - 1;
            }
        }
        else     //(0,0,0,0) is not one of the closest two pentachoron vertices.
        {
            uchar c = (uchar)(aPoint | bPoint); //Our three extra vertices are determined by the closest two.

            if ((c & 0x01) == 0)
            {
                xsv_ext0 = xsv_ext2 = xsb;
                xsv_ext1 = xsb - 1;
                dx_ext0 = dx0 - 2 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx0 + 1 - SQUISH_CONSTANT_4D;
                dx_ext2 = dx0 - SQUISH_CONSTANT_4D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb + 1;
                dx_ext0 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx_ext2 = dx0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c & 0x02) == 0)
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
                dy_ext0 = dy0 - 2 * SQUISH_CONSTANT_4D;
                dy_ext1 = dy_ext2 = dy0 - SQUISH_CONSTANT_4D;
                if ((c & 0x01) == 0x01)
                {
                    ysv_ext1 -= 1;
                    dy_ext1 += 1;
                }
                else
                {
                    ysv_ext2 -= 1;
                    dy_ext2 += 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
                dy_ext0 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dy_ext1 = dy_ext2 = dy0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c & 0x04) == 0)
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
                dz_ext0 = dz0 - 2 * SQUISH_CONSTANT_4D;
                dz_ext1 = dz_ext2 = dz0 - SQUISH_CONSTANT_4D;
                if ((c & 0x03) == 0x03)
                {
                    zsv_ext1 -= 1;
                    dz_ext1 += 1;
                }
                else
                {
                    zsv_ext2 -= 1;
                    dz_ext2 += 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
                dz_ext0 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dz_ext1 = dz_ext2 = dz0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c & 0x08) == 0)
            {
                wsv_ext0 = wsv_ext1 = wsb;
                wsv_ext2 = wsb - 1;
                dw_ext0 = dw0 - 2 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw0 - SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 + 1 - SQUISH_CONSTANT_4D;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb + 1;
                dw_ext0 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw_ext2 = dw0 - 1 - SQUISH_CONSTANT_4D;
            }
        }

        //Contribution (0,0,0,0)
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 0, zsb + 0, wsb + 0, dx0, dy0, dz0, dw0, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,0,0)
        float dx1 = dx0 - 1 - SQUISH_CONSTANT_4D;
        float dy1 = dy0 - 0 - SQUISH_CONSTANT_4D;
        float dz1 = dz0 - 0 - SQUISH_CONSTANT_4D;
        float dw1 = dw0 - 0 - SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 0, wsb + 0, dx1, dy1, dz1, dw1, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,0,0)
        float dx2 = dx0 - 0 - SQUISH_CONSTANT_4D;
        float dy2 = dy0 - 1 - SQUISH_CONSTANT_4D;
        float dz2 = dz1;
        float dw2 = dw1;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 0, wsb + 0, dx2, dy2, dz2, dw2, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,1,0)
        float dx3 = dx2;
        float dy3 = dy1;
        float dz3 = dz0 - 1 - SQUISH_CONSTANT_4D;
        float dw3 = dw1;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 0, zsb + 1, wsb + 0, dx3, dy3, dz3, dw3, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,0,1)
        float dx4 = dx2;
        float dy4 = dy1;
        float dz4 = dz1;
        float dw4 = dw0 - 1 - SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 0, zsb + 0, wsb + 1, dx4, dy4, dz4, dw4, n, dndx, dndy, dndz, dndw);
    }
    else if (inSum >= 3)     //We're inside the pentachoron (4-Simplex) at (1,1,1,1)
    {
        //Determine which two of (1,1,1,0), (1,1,0,1), (1,0,1,1), (0,1,1,1) are closest.
        uchar aPoint = 0x0E;
        float aScore = xins;
        uchar bPoint = 0x0D;
        float bScore = yins;
        if (aScore <= bScore && zins < bScore)
        {
            bScore = zins;
            bPoint = 0x0B;
        }
        else if (aScore > bScore && zins < aScore)
        {
            aScore = zins;
            aPoint = 0x0B;
        }
        if (aScore <= bScore && wins < bScore)
        {
            bScore = wins;
            bPoint = 0x07;
        }
        else if (aScore > bScore && wins < aScore)
        {
            aScore = wins;
            aPoint = 0x07;
        }

        //Now we determine the three lattice points not part of the pentachoron that may contribute.
        //This depends on the closest two pentachoron vertices, including (0,0,0,0)
        float uins = 4 - inSum;
        if (uins < aScore || uins < bScore)   //(1,1,1,1) is one of the closest two pentachoron vertices.
        {
            uchar c = (bScore < aScore ? bPoint : aPoint); //Our other closest vertex is the closest out of a and b.

            if ((c & 0x01) != 0)
            {
                xsv_ext0 = xsb + 2;
                xsv_ext1 = xsv_ext2 = xsb + 1;
                dx_ext0 = dx0 - 2 - 4 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx_ext2 = dx0 - 1 - 4 * SQUISH_CONSTANT_4D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb;
                dx_ext0 = dx_ext1 = dx_ext2 = dx0 - 4 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x02) != 0)
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
                dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 1 - 4 * SQUISH_CONSTANT_4D;
                if ((c & 0x01) != 0)
                {
                    ysv_ext1 += 1;
                    dy_ext1 -= 1;
                }
                else
                {
                    ysv_ext0 += 1;
                    dy_ext0 -= 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
                dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 4 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x04) != 0)
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
                dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 1 - 4 * SQUISH_CONSTANT_4D;
                if ((c & 0x03) != 0x03)
                {
                    if ((c & 0x03) == 0)
                    {
                        zsv_ext0 += 1;
                        dz_ext0 -= 1;
                    }
                    else
                    {
                        zsv_ext1 += 1;
                        dz_ext1 -= 1;
                    }
                }
                else
                {
                    zsv_ext2 += 1;
                    dz_ext2 -= 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
                dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 4 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x08) != 0)
            {
                wsv_ext0 = wsv_ext1 = wsb + 1;
                wsv_ext2 = wsb + 2;
                dw_ext0 = dw_ext1 = dw0 - 1 - 4 * SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 - 2 - 4 * SQUISH_CONSTANT_4D;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb;
                dw_ext0 = dw_ext1 = dw_ext2 = dw0 - 4 * SQUISH_CONSTANT_4D;
            }
        }
        else     //(1,1,1,1) is not one of the closest two pentachoron vertices.
        {
            uchar c = (uchar)(aPoint & bPoint); //Our three extra vertices are determined by the closest two.

            if ((c & 0x01) != 0)
            {
                xsv_ext0 = xsv_ext2 = xsb + 1;
                xsv_ext1 = xsb + 2;
                dx_ext0 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx0 - 2 - 3 * SQUISH_CONSTANT_4D;
                dx_ext2 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb;
                dx_ext0 = dx0 - 2 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx_ext2 = dx0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x02) != 0)
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
                dy_ext0 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dy_ext1 = dy_ext2 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
                if ((c & 0x01) != 0)
                {
                    ysv_ext2 += 1;
                    dy_ext2 -= 1;
                }
                else
                {
                    ysv_ext1 += 1;
                    dy_ext1 -= 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
                dy_ext0 = dy0 - 2 * SQUISH_CONSTANT_4D;
                dy_ext1 = dy_ext2 = dy0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x04) != 0)
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
                dz_ext0 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dz_ext1 = dz_ext2 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
                if ((c & 0x03) != 0)
                {
                    zsv_ext2 += 1;
                    dz_ext2 -= 1;
                }
                else
                {
                    zsv_ext1 += 1;
                    dz_ext1 -= 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
                dz_ext0 = dz0 - 2 * SQUISH_CONSTANT_4D;
                dz_ext1 = dz_ext2 = dz0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c & 0x08) != 0)
            {
                wsv_ext0 = wsv_ext1 = wsb + 1;
                wsv_ext2 = wsb + 2;
                dw_ext0 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 - 2 - 3 * SQUISH_CONSTANT_4D;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb;
                dw_ext0 = dw0 - 2 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw_ext2 = dw0 - 3 * SQUISH_CONSTANT_4D;
            }
        }

        //Contribution (1,1,1,0)
        float dx4 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dy4 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dz4 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dw4 = dw0 - 3 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 1, zsb + 1, wsb + 0, dx4, dy4, dz4, dw4, n, dndx, dndy, dndz, dndw);

        //Contribution (1,1,0,1)
        float dx3 = dx4;
        float dy3 = dy4;
        float dz3 = dz0 - 3 * SQUISH_CONSTANT_4D;
        float dw3 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 1, zsb + 0, wsb + 1, dx3, dy3, dz3, dw3, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,1,1)
        float dx2 = dx4;
        float dy2 = dy0 - 3 * SQUISH_CONSTANT_4D;
        float dz2 = dz4;
        float dw2 = dw3;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 1, wsb + 1, dx2, dy2, dz2, dw2, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,1,1)
        float dx1 = dx0 - 3 * SQUISH_CONSTANT_4D;
        float dz1 = dz4;
        float dy1 = dy4;
        float dw1 = dw3;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 1, wsb + 1, dx1, dy1, dz1, dw1, n, dndx, dndy, dndz, dndw);

        //Contribution (1,1,1,1)
        dx0 = dx0 - 1 - 4 * SQUISH_CONSTANT_4D;
        dy0 = dy0 - 1 - 4 * SQUISH_CONSTANT_4D;
        dz0 = dz0 - 1 - 4 * SQUISH_CONSTANT_4D;
        dw0 = dw0 - 1 - 4 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 1, zsb + 1, wsb + 1, dx0, dy0, dz0, dw0, n, dndx, dndy, dndz, dndw);
    }
    else if (inSum <= 2)     //We're inside the first dispentachoron (Rectified 4-Simplex)
    {
        float aScore;
        uchar aPoint;
        bool aIsBiggerSide = true;
        float bScore;
        uchar bPoint;
        bool bIsBiggerSide = true;

        //Decide between (1,1,0,0) and (0,0,1,1)
        if (xins + yins > zins + wins)
        {
            aScore = xins + yins;
            aPoint = 0x03;
        }
        else
        {
            aScore = zins + wins;
            aPoint = 0x0C;
        }

        //Decide between (1,0,1,0) and (0,1,0,1)
        if (xins + zins > yins + wins)
        {
            bScore = xins + zins;
            bPoint = 0x05;
        }
        else
        {
            bScore = yins + wins;
            bPoint = 0x0A;
        }

        //Closer between (1,0,0,1) and (0,1,1,0) will replace the further of a and b, if closer.
        if (xins + wins > yins + zins)
        {
            float score = xins + wins;
            if (aScore >= bScore && score > bScore)
            {
                bScore = score;
                bPoint = 0x09;
            }
            else if (aScore < bScore && score > aScore)
            {
                aScore = score;
                aPoint = 0x09;
            }
        }
        else
        {
            float score = yins + zins;
            if (aScore >= bScore && score > bScore)
            {
                bScore = score;
                bPoint = 0x06;
            }
            else if (aScore < bScore && score > aScore)
            {
                aScore = score;
                aPoint = 0x06;
            }
        }

        //Decide if (1,0,0,0) is closer.
        float p1 = 2 - inSum + xins;
        if (aScore >= bScore && p1 > bScore)
        {
            bScore = p1;
            bPoint = 0x01;
            bIsBiggerSide = false;
        }
        else if (aScore < bScore && p1 > aScore)
        {
            aScore = p1;
            aPoint = 0x01;
            aIsBiggerSide = false;
        }

        //Decide if (0,1,0,0) is closer.
        float p2 = 2 - inSum + yins;
        if (aScore >= bScore && p2 > bScore)
        {
            bScore = p2;
            bPoint = 0x02;
            bIsBiggerSide = false;
        }
        else if (aScore < bScore && p2 > aScore)
        {
            aScore = p2;
            aPoint = 0x02;
            aIsBiggerSide = false;
        }

        //Decide if (0,0,1,0) is closer.
        float p3 = 2 - inSum + zins;
        if (aScore >= bScore && p3 > bScore)
        {
            bScore = p3;
            bPoint = 0x04;
            bIsBiggerSide = false;
        }
        else if (aScore < bScore && p3 > aScore)
        {
            aScore = p3;
            aPoint = 0x04;
            aIsBiggerSide = false;
        }

        //Decide if (0,0,0,1) is closer.
        float p4 = 2 - inSum + wins;
        if (aScore >= bScore && p4 > bScore)
        {
            bScore = p4;
            bPoint = 0x08;
            bIsBiggerSide = false;
        }
        else if (aScore < bScore && p4 > aScore)
        {
            aScore = p4;
            aPoint = 0x08;
            aIsBiggerSide = false;
        }

        //Where each of the two closest points are determines how the extra three vertices are calculated.
        if (aIsBiggerSide == bIsBiggerSide)
        {
            if (aIsBiggerSide)   //Both closest points on the bigger side
            {
                uchar c1 = (uchar)(aPoint | bPoint);
                uchar c2 = (uchar)(aPoint & bPoint);
                if ((c1 & 0x01) == 0)
                {
                    xsv_ext0 = xsb;
                    xsv_ext1 = xsb - 1;
                    dx_ext0 = dx0 - 3 * SQUISH_CONSTANT_4D;
                    dx_ext1 = dx0 + 1 - 2 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    xsv_ext0 = xsv_ext1 = xsb + 1;
                    dx_ext0 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    dx_ext1 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
                }

                if ((c1 & 0x02) == 0)
                {
                    ysv_ext0 = ysb;
                    ysv_ext1 = ysb - 1;
                    dy_ext0 = dy0 - 3 * SQUISH_CONSTANT_4D;
                    dy_ext1 = dy0 + 1 - 2 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    ysv_ext0 = ysv_ext1 = ysb + 1;
                    dy_ext0 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    dy_ext1 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
                }

                if ((c1 & 0x04) == 0)
                {
                    zsv_ext0 = zsb;
                    zsv_ext1 = zsb - 1;
                    dz_ext0 = dz0 - 3 * SQUISH_CONSTANT_4D;
                    dz_ext1 = dz0 + 1 - 2 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    zsv_ext0 = zsv_ext1 = zsb + 1;
                    dz_ext0 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    dz_ext1 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
                }

                if ((c1 & 0x08) == 0)
                {
                    wsv_ext0 = wsb;
                    wsv_ext1 = wsb - 1;
                    dw_ext0 = dw0 - 3 * SQUISH_CONSTANT_4D;
                    dw_ext1 = dw0 + 1 - 2 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    wsv_ext0 = wsv_ext1 = wsb + 1;
                    dw_ext0 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    dw_ext1 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
                }

                //One combination is a permutation of (0,0,0,2) based on c2
                xsv_ext2 = xsb;
                ysv_ext2 = ysb;
                zsv_ext2 = zsb;
                wsv_ext2 = wsb;
                dx_ext2 = dx0 - 2 * SQUISH_CONSTANT_4D;
                dy_ext2 = dy0 - 2 * SQUISH_CONSTANT_4D;
                dz_ext2 = dz0 - 2 * SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 - 2 * SQUISH_CONSTANT_4D;
                if ((c2 & 0x01) != 0)
                {
                    xsv_ext2 += 2;
                    dx_ext2 -= 2;
                }
                else if ((c2 & 0x02) != 0)
                {
                    ysv_ext2 += 2;
                    dy_ext2 -= 2;
                }
                else if ((c2 & 0x04) != 0)
                {
                    zsv_ext2 += 2;
                    dz_ext2 -= 2;
                }
                else
                {
                    wsv_ext2 += 2;
                    dw_ext2 -= 2;
                }

            }
            else     //Both closest points on the smaller side
            {
                //One of the two extra points is (0,0,0,0)
                xsv_ext2 = xsb;
                ysv_ext2 = ysb;
                zsv_ext2 = zsb;
                wsv_ext2 = wsb;
                dx_ext2 = dx0;
                dy_ext2 = dy0;
                dz_ext2 = dz0;
                dw_ext2 = dw0;

                //Other two points are based on the omitted axes.
                uchar c = (uchar)(aPoint | bPoint);

                if ((c & 0x01) == 0)
                {
                    xsv_ext0 = xsb - 1;
                    xsv_ext1 = xsb;
                    dx_ext0 = dx0 + 1 - SQUISH_CONSTANT_4D;
                    dx_ext1 = dx0 - SQUISH_CONSTANT_4D;
                }
                else
                {
                    xsv_ext0 = xsv_ext1 = xsb + 1;
                    dx_ext0 = dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_4D;
                }

                if ((c & 0x02) == 0)
                {
                    ysv_ext0 = ysv_ext1 = ysb;
                    dy_ext0 = dy_ext1 = dy0 - SQUISH_CONSTANT_4D;
                    if ((c & 0x01) == 0x01)
                    {
                        ysv_ext0 -= 1;
                        dy_ext0 += 1;
                    }
                    else
                    {
                        ysv_ext1 -= 1;
                        dy_ext1 += 1;
                    }
                }
                else
                {
                    ysv_ext0 = ysv_ext1 = ysb + 1;
                    dy_ext0 = dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_4D;
                }

                if ((c & 0x04) == 0)
                {
                    zsv_ext0 = zsv_ext1 = zsb;
                    dz_ext0 = dz_ext1 = dz0 - SQUISH_CONSTANT_4D;
                    if ((c & 0x03) == 0x03)
                    {
                        zsv_ext0 -= 1;
                        dz_ext0 += 1;
                    }
                    else
                    {
                        zsv_ext1 -= 1;
                        dz_ext1 += 1;
                    }
                }
                else
                {
                    zsv_ext0 = zsv_ext1 = zsb + 1;
                    dz_ext0 = dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_4D;
                }

                if ((c & 0x08) == 0)
                {
                    wsv_ext0 = wsb;
                    wsv_ext1 = wsb - 1;
                    dw_ext0 = dw0 - SQUISH_CONSTANT_4D;
                    dw_ext1 = dw0 + 1 - SQUISH_CONSTANT_4D;
                }
                else
                {
                    wsv_ext0 = wsv_ext1 = wsb + 1;
                    dw_ext0 = dw_ext1 = dw0 - 1 - SQUISH_CONSTANT_4D;
                }

            }
        }
        else     //One point on each "side"
        {
            uchar c1, c2;
            if (aIsBiggerSide)
            {
                c1 = aPoint;
                c2 = bPoint;
            }
            else
            {
                c1 = bPoint;
                c2 = aPoint;
            }

            //Two contributions are the bigger-sided point with each 0 replaced with -1.
            if ((c1 & 0x01) == 0)
            {
                xsv_ext0 = xsb - 1;
                xsv_ext1 = xsb;
                dx_ext0 = dx0 + 1 - SQUISH_CONSTANT_4D;
                dx_ext1 = dx0 - SQUISH_CONSTANT_4D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb + 1;
                dx_ext0 = dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x02) == 0)
            {
                ysv_ext0 = ysv_ext1 = ysb;
                dy_ext0 = dy_ext1 = dy0 - SQUISH_CONSTANT_4D;
                if ((c1 & 0x01) == 0x01)
                {
                    ysv_ext0 -= 1;
                    dy_ext0 += 1;
                }
                else
                {
                    ysv_ext1 -= 1;
                    dy_ext1 += 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb + 1;
                dy_ext0 = dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x04) == 0)
            {
                zsv_ext0 = zsv_ext1 = zsb;
                dz_ext0 = dz_ext1 = dz0 - SQUISH_CONSTANT_4D;
                if ((c1 & 0x03) == 0x03)
                {
                    zsv_ext0 -= 1;
                    dz_ext0 += 1;
                }
                else
                {
                    zsv_ext1 -= 1;
                    dz_ext1 += 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb + 1;
                dz_ext0 = dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x08) == 0)
            {
                wsv_ext0 = wsb;
                wsv_ext1 = wsb - 1;
                dw_ext0 = dw0 - SQUISH_CONSTANT_4D;
                dw_ext1 = dw0 + 1 - SQUISH_CONSTANT_4D;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsb + 1;
                dw_ext0 = dw_ext1 = dw0 - 1 - SQUISH_CONSTANT_4D;
            }

            //One contribution is a permutation of (0,0,0,2) based on the smaller-sided point
            xsv_ext2 = xsb;
            ysv_ext2 = ysb;
            zsv_ext2 = zsb;
            wsv_ext2 = wsb;
            dx_ext2 = dx0 - 2 * SQUISH_CONSTANT_4D;
            dy_ext2 = dy0 - 2 * SQUISH_CONSTANT_4D;
            dz_ext2 = dz0 - 2 * SQUISH_CONSTANT_4D;
            dw_ext2 = dw0 - 2 * SQUISH_CONSTANT_4D;
            if ((c2 & 0x01) != 0)
            {
                xsv_ext2 += 2;
                dx_ext2 -= 2;
            }
            else if ((c2 & 0x02) != 0)
            {
                ysv_ext2 += 2;
                dy_ext2 -= 2;
            }
            else if ((c2 & 0x04) != 0)
            {
                zsv_ext2 += 2;
                dz_ext2 -= 2;
            }
            else
            {
                wsv_ext2 += 2;
                dw_ext2 -= 2;
            }
        }

        //Contribution (1,0,0,0)
        float dx1 = dx0 - 1 - SQUISH_CONSTANT_4D;
        float dy1 = dy0 - 0 - SQUISH_CONSTANT_4D;
        float dz1 = dz0 - 0 - SQUISH_CONSTANT_4D;
        float dw1 = dw0 - 0 - SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 0, wsb + 0, dx1, dy1, dz1, dw1, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,0,0)
        float dx2 = dx0 - 0 - SQUISH_CONSTANT_4D;
        float dy2 = dy0 - 1 - SQUISH_CONSTANT_4D;
        float dz2 = dz1;
        float dw2 = dw1;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 0, wsb + 0, dx2, dy2, dz2, dw2, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,1,0)
        float dx3 = dx2;
        float dy3 = dy1;
        float dz3 = dz0 - 1 - SQUISH_CONSTANT_4D;
        float dw3 = dw1;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 0, zsb + 1, wsb + 0, dx3, dy3, dz3, dw3, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,0,1)
        float dx4 = dx2;
        float dy4 = dy1;
        float dz4 = dz1;
        float dw4 = dw0 - 1 - SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 0, zsb + 0, wsb + 1, dx4, dy4, dz4, dw4, n, dndx, dndy, dndz, dndw);

        //Contribution (1,1,0,0)
        float dx5 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy5 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz5 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw5 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 1, zsb + 0, wsb + 0, dx5, dy5, dz5, dw5, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,1,0)
        float dx6 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy6 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz6 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw6 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 1, wsb + 0, dx6, dy6, dz6, dw6, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,0,1)
        float dx7 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy7 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz7 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw7 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 0, wsb + 1, dx7, dy7, dz7, dw7, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,1,0)
        float dx8 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy8 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz8 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw8 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 1, wsb + 0, dx8, dy8, dz8, dw8, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,0,1)
        float dx9 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy9 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz9 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw9 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 0, wsb + 1, dx9, dy9, dz9, dw9, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,1,1)
        float dx10 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy10 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz10 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw10 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 0, zsb + 1, wsb + 1, dx10, dy10, dz10, dw10, n, dndx, dndy, dndz, dndw);
    }
    else     //We're inside the second dispentachoron (Rectified 4-Simplex)
    {
        float aScore;
        uchar aPoint;
        bool aIsBiggerSide = true;
        float bScore;
        uchar bPoint;
        bool bIsBiggerSide = true;

        //Decide between (0,0,1,1) and (1,1,0,0)
        if (xins + yins < zins + wins)
        {
            aScore = xins + yins;
            aPoint = 0x0C;
        }
        else
        {
            aScore = zins + wins;
            aPoint = 0x03;
        }

        //Decide between (0,1,0,1) and (1,0,1,0)
        if (xins + zins < yins + wins)
        {
            bScore = xins + zins;
            bPoint = 0x0A;
        }
        else
        {
            bScore = yins + wins;
            bPoint = 0x05;
        }

        //Closer between (0,1,1,0) and (1,0,0,1) will replace the further of a and b, if closer.
        if (xins + wins < yins + zins)
        {
            float score = xins + wins;
            if (aScore <= bScore && score < bScore)
            {
                bScore = score;
                bPoint = 0x06;
            }
            else if (aScore > bScore && score < aScore)
            {
                aScore = score;
                aPoint = 0x06;
            }
        }
        else
        {
            float score = yins + zins;
            if (aScore <= bScore && score < bScore)
            {
                bScore = score;
                bPoint = 0x09;
            }
            else if (aScore > bScore && score < aScore)
            {
                aScore = score;
                aPoint = 0x09;
            }
        }

        //Decide if (0,1,1,1) is closer.
        float p1 = 3 - inSum + xins;
        if (aScore <= bScore && p1 < bScore)
        {
            bScore = p1;
            bPoint = 0x0E;
            bIsBiggerSide = false;
        }
        else if (aScore > bScore && p1 < aScore)
        {
            aScore = p1;
            aPoint = 0x0E;
            aIsBiggerSide = false;
        }

        //Decide if (1,0,1,1) is closer.
        float p2 = 3 - inSum + yins;
        if (aScore <= bScore && p2 < bScore)
        {
            bScore = p2;
            bPoint = 0x0D;
            bIsBiggerSide = false;
        }
        else if (aScore > bScore && p2 < aScore)
        {
            aScore = p2;
            aPoint = 0x0D;
            aIsBiggerSide = false;
        }

        //Decide if (1,1,0,1) is closer.
        float p3 = 3 - inSum + zins;
        if (aScore <= bScore && p3 < bScore)
        {
            bScore = p3;
            bPoint = 0x0B;
            bIsBiggerSide = false;
        }
        else if (aScore > bScore && p3 < aScore)
        {
            aScore = p3;
            aPoint = 0x0B;
            aIsBiggerSide = false;
        }

        //Decide if (1,1,1,0) is closer.
        float p4 = 3 - inSum + wins;
        if (aScore <= bScore && p4 < bScore)
        {
            bScore = p4;
            bPoint = 0x07;
            bIsBiggerSide = false;
        }
        else if (aScore > bScore && p4 < aScore)
        {
            aScore = p4;
            aPoint = 0x07;
            aIsBiggerSide = false;
        }

        //Where each of the two closest points are determines how the extra three vertices are calculated.
        if (aIsBiggerSide == bIsBiggerSide)
        {
            if (aIsBiggerSide)   //Both closest points on the bigger side
            {
                uchar c1 = (uchar)(aPoint & bPoint);
                uchar c2 = (uchar)(aPoint | bPoint);

                //Two contributions are permutations of (0,0,0,1) and (0,0,0,2) based on c1
                xsv_ext0 = xsv_ext1 = xsb;
                ysv_ext0 = ysv_ext1 = ysb;
                zsv_ext0 = zsv_ext1 = zsb;
                wsv_ext0 = wsv_ext1 = wsb;
                dx_ext0 = dx0 - SQUISH_CONSTANT_4D;
                dy_ext0 = dy0 - SQUISH_CONSTANT_4D;
                dz_ext0 = dz0 - SQUISH_CONSTANT_4D;
                dw_ext0 = dw0 - SQUISH_CONSTANT_4D;
                dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_4D;
                dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_4D;
                dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw0 - 2 * SQUISH_CONSTANT_4D;
                if ((c1 & 0x01) != 0)
                {
                    xsv_ext0 += 1;
                    dx_ext0 -= 1;
                    xsv_ext1 += 2;
                    dx_ext1 -= 2;
                }
                else if ((c1 & 0x02) != 0)
                {
                    ysv_ext0 += 1;
                    dy_ext0 -= 1;
                    ysv_ext1 += 2;
                    dy_ext1 -= 2;
                }
                else if ((c1 & 0x04) != 0)
                {
                    zsv_ext0 += 1;
                    dz_ext0 -= 1;
                    zsv_ext1 += 2;
                    dz_ext1 -= 2;
                }
                else
                {
                    wsv_ext0 += 1;
                    dw_ext0 -= 1;
                    wsv_ext1 += 2;
                    dw_ext1 -= 2;
                }

                //One contribution is a permutation of (1,1,1,-1) based on c2
                xsv_ext2 = xsb + 1;
                ysv_ext2 = ysb + 1;
                zsv_ext2 = zsb + 1;
                wsv_ext2 = wsb + 1;
                dx_ext2 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dy_ext2 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dz_ext2 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
                if ((c2 & 0x01) == 0)
                {
                    xsv_ext2 -= 2;
                    dx_ext2 += 2;
                }
                else if ((c2 & 0x02) == 0)
                {
                    ysv_ext2 -= 2;
                    dy_ext2 += 2;
                }
                else if ((c2 & 0x04) == 0)
                {
                    zsv_ext2 -= 2;
                    dz_ext2 += 2;
                }
                else
                {
                    wsv_ext2 -= 2;
                    dw_ext2 += 2;
                }
            }
            else     //Both closest points on the smaller side
            {
                //One of the two extra points is (1,1,1,1)
                xsv_ext2 = xsb + 1;
                ysv_ext2 = ysb + 1;
                zsv_ext2 = zsb + 1;
                wsv_ext2 = wsb + 1;
                dx_ext2 = dx0 - 1 - 4 * SQUISH_CONSTANT_4D;
                dy_ext2 = dy0 - 1 - 4 * SQUISH_CONSTANT_4D;
                dz_ext2 = dz0 - 1 - 4 * SQUISH_CONSTANT_4D;
                dw_ext2 = dw0 - 1 - 4 * SQUISH_CONSTANT_4D;

                //Other two points are based on the shared axes.
                uchar c = (uchar)(aPoint & bPoint);

                if ((c & 0x01) != 0)
                {
                    xsv_ext0 = xsb + 2;
                    xsv_ext1 = xsb + 1;
                    dx_ext0 = dx0 - 2 - 3 * SQUISH_CONSTANT_4D;
                    dx_ext1 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    xsv_ext0 = xsv_ext1 = xsb;
                    dx_ext0 = dx_ext1 = dx0 - 3 * SQUISH_CONSTANT_4D;
                }

                if ((c & 0x02) != 0)
                {
                    ysv_ext0 = ysv_ext1 = ysb + 1;
                    dy_ext0 = dy_ext1 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    if ((c & 0x01) == 0)
                    {
                        ysv_ext0 += 1;
                        dy_ext0 -= 1;
                    }
                    else
                    {
                        ysv_ext1 += 1;
                        dy_ext1 -= 1;
                    }
                }
                else
                {
                    ysv_ext0 = ysv_ext1 = ysb;
                    dy_ext0 = dy_ext1 = dy0 - 3 * SQUISH_CONSTANT_4D;
                }

                if ((c & 0x04) != 0)
                {
                    zsv_ext0 = zsv_ext1 = zsb + 1;
                    dz_ext0 = dz_ext1 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    if ((c & 0x03) == 0)
                    {
                        zsv_ext0 += 1;
                        dz_ext0 -= 1;
                    }
                    else
                    {
                        zsv_ext1 += 1;
                        dz_ext1 -= 1;
                    }
                }
                else
                {
                    zsv_ext0 = zsv_ext1 = zsb;
                    dz_ext0 = dz_ext1 = dz0 - 3 * SQUISH_CONSTANT_4D;
                }

                if ((c & 0x08) != 0)
                {
                    wsv_ext0 = wsb + 1;
                    wsv_ext1 = wsb + 2;
                    dw_ext0 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
                    dw_ext1 = dw0 - 2 - 3 * SQUISH_CONSTANT_4D;
                }
                else
                {
                    wsv_ext0 = wsv_ext1 = wsb;
                    dw_ext0 = dw_ext1 = dw0 - 3 * SQUISH_CONSTANT_4D;
                }
            }
        }
        else     //One point on each "side"
        {
            uchar c1, c2;
            if (aIsBiggerSide)
            {
                c1 = aPoint;
                c2 = bPoint;
            }
            else
            {
                c1 = bPoint;
                c2 = aPoint;
            }

            //Two contributions are the bigger-sided point with each 1 replaced with 2.
            if ((c1 & 0x01) != 0)
            {
                xsv_ext0 = xsb + 2;
                xsv_ext1 = xsb + 1;
                dx_ext0 = dx0 - 2 - 3 * SQUISH_CONSTANT_4D;
                dx_ext1 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
            }
            else
            {
                xsv_ext0 = xsv_ext1 = xsb;
                dx_ext0 = dx_ext1 = dx0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x02) != 0)
            {
                ysv_ext0 = ysv_ext1 = ysb + 1;
                dy_ext0 = dy_ext1 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
                if ((c1 & 0x01) == 0)
                {
                    ysv_ext0 += 1;
                    dy_ext0 -= 1;
                }
                else
                {
                    ysv_ext1 += 1;
                    dy_ext1 -= 1;
                }
            }
            else
            {
                ysv_ext0 = ysv_ext1 = ysb;
                dy_ext0 = dy_ext1 = dy0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x04) != 0)
            {
                zsv_ext0 = zsv_ext1 = zsb + 1;
                dz_ext0 = dz_ext1 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
                if ((c1 & 0x03) == 0)
                {
                    zsv_ext0 += 1;
                    dz_ext0 -= 1;
                }
                else
                {
                    zsv_ext1 += 1;
                    dz_ext1 -= 1;
                }
            }
            else
            {
                zsv_ext0 = zsv_ext1 = zsb;
                dz_ext0 = dz_ext1 = dz0 - 3 * SQUISH_CONSTANT_4D;
            }

            if ((c1 & 0x08) != 0)
            {
                wsv_ext0 = wsb + 1;
                wsv_ext1 = wsb + 2;
                dw_ext0 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
                dw_ext1 = dw0 - 2 - 3 * SQUISH_CONSTANT_4D;
            }
            else
            {
                wsv_ext0 = wsv_ext1 = wsb;
                dw_ext0 = dw_ext1 = dw0 - 3 * SQUISH_CONSTANT_4D;
            }

            //One contribution is a permutation of (1,1,1,-1) based on the smaller-sided point
            xsv_ext2 = xsb + 1;
            ysv_ext2 = ysb + 1;
            zsv_ext2 = zsb + 1;
            wsv_ext2 = wsb + 1;
            dx_ext2 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
            dy_ext2 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
            dz_ext2 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
            dw_ext2 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
            if ((c2 & 0x01) == 0)
            {
                xsv_ext2 -= 2;
                dx_ext2 += 2;
            }
            else if ((c2 & 0x02) == 0)
            {
                ysv_ext2 -= 2;
                dy_ext2 += 2;
            }
            else if ((c2 & 0x04) == 0)
            {
                zsv_ext2 -= 2;
                dz_ext2 += 2;
            }
            else
            {
                wsv_ext2 -= 2;
                dw_ext2 += 2;
            }
        }

        //Contribution (1,1,1,0)
        float dx4 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dy4 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dz4 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
        float dw4 = dw0 - 3 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 1, zsb + 1, wsb + 0, dx4, dy4, dz4, dw4, n, dndx, dndy, dndz, dndw);

        //Contribution (1,1,0,1)
        float dx3 = dx4;
        float dy3 = dy4;
        float dz3 = dz0 - 3 * SQUISH_CONSTANT_4D;
        float dw3 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 1, zsb + 0, wsb + 1, dx3, dy3, dz3, dw3, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,1,1)
        float dx2 = dx4;
        float dy2 = dy0 - 3 * SQUISH_CONSTANT_4D;
        float dz2 = dz4;
        float dw2 = dw3;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 1, wsb + 1, dx2, dy2, dz2, dw2, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,1,1)
        float dx1 = dx0 - 3 * SQUISH_CONSTANT_4D;
        float dz1 = dz4;
        float dy1 = dy4;
        float dw1 = dw3;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 1, wsb + 1, dx1, dy1, dz1, dw1, n, dndx, dndy, dndz, dndw);

        //Contribution (1,1,0,0)
        float dx5 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy5 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz5 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw5 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 1, zsb + 0, wsb + 0, dx5, dy5, dz5, dw5, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,1,0)
        float dx6 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy6 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz6 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw6 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 1, wsb + 0, dx6, dy6, dz6, dw6, n, dndx, dndy, dndz, dndw);

        //Contribution (1,0,0,1)
        float dx7 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dy7 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz7 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw7 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 1, ysb + 0, zsb + 0, wsb + 1, dx7, dy7, dz7, dw7, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,1,0)
        float dx8 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy8 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz8 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw8 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 1, wsb + 0, dx8, dy8, dz8, dw8, n, dndx, dndy, dndz, dndw);

        //Contribution (0,1,0,1)
        float dx9 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy9 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dz9 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dw9 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 1, zsb + 0, wsb + 1, dx9, dy9, dz9, dw9, n, dndx, dndy, dndz, dndw);

        //Contribution (0,0,1,1)
        float dx10 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dy10 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
        float dz10 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
        float dw10 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
        extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsb + 0, ysb + 0, zsb + 1, wsb + 1, dx10, dy10, dz10, dw10, n, dndx, dndy, dndz, dndw);
    }

    //First extra vertex
    extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsv_ext0, ysv_ext0, zsv_ext0, wsv_ext0, dx_ext0, dy_ext0, dz_ext0, dw_ext0, n, dndx, dndy, dndz, dndw);

    //Second extra vertex
    extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsv_ext1, ysv_ext1, zsv_ext1, wsv_ext1, dx_ext1, dy_ext1, dz_ext1, dw_ext1, n, dndx, dndy, dndz, dndw);

    //Third extra vertex
    extrapolate4v(thePx, thePy, thePz, thePw, theXT, theYT, theZT, xsv_ext2, ysv_ext2, zsv_ext2, wsv_ext2, dx_ext2, dy_ext2, dz_ext2, dw_ext2, n, dndx, dndy, dndz, dndw);

    *n = *n * NORM_CONSTANT_4D + 0.5f;
    *dndx *= NORM_CONSTANT_4D * FREQ_ADJUST;
    *dndy *= NORM_CONSTANT_4D * FREQ_ADJUST;
    *dndz *= NORM_CONSTANT_4D * FREQ_ADJUST;
    *dndw *= NORM_CONSTANT_4D * FREQ_ADJUST;
}

float3 xnoisev4(global const void *theXNoise, float4 p)
{
	float3 n = 0, dndx = 0, dndy = 0, dndz = 0, dndw = 0;
	xnoisev4d(theXNoise, p, &n, &dndx, &dndy, &dndz, &dndw);
	return n;
}

// 4D curlnoise.  Equivalent to VEX:
// vector curlxnoise( vector4 )
float3 curlxnoise4(global const void *theXNoise, float4 p)
{
	float3 n = 0, dndx = 0, dndy = 0, dndz = 0, dndw = 0;
	xnoisev4d(theXNoise, p, &n, &dndx, &dndy, &dndz, &dndw);
	return (float3)(dndy.z - dndz.y,
                    dndz.x - dndx.z,
                    dndx.y - dndy.x);
}

#endif
