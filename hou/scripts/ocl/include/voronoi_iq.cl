/*
    // ----------------------------------------------- import exemple
        typedef struct{
            float smooth;
            float time;
        }voroparm_t;

        #define VORO_PARM_T voroparm_t

        void voroUpdate(float3 *offset, float3 *color, float3 gid, VORO_PARM_T data){

            uint3 seed3 = as_uint3(convert_int3(gid));
            float3 hash3 = convert_float3(pcg3d(seed3)) / (float3)(0xffffffffU);
            
            float3 o;
            o = 0.5f + 0.5f*sin( data.time + PI*hash3 );


            int id = (int)dot(gid,(float3)(7.0,113.0, 1451.0));
            uint seed1 = as_uint(id);
            float hash1 = lowbias32(seed1) / (float)(0xffffffffu);

            float3 col = 0.0; 
            col = 0.5f + 0.5f * sin( hash1 * 2.5f + 3.5f + (float3)(2.0,1.0,0.0));
            col = col*col;

            (*offset) = o;
            (*color) = col;
        }
        #define VORO_UPDATE voroUpdate
        #include "$HOME/libs/ocl/include/voronoi_iq.cl"
    //
*/
#ifndef IQ_VORONOI_CL
#define IQ_VORONOI_CL

#include "./utils.cl" // frac, mod...
#include "./hashs.cl" // frac, mod...

#ifndef VORO_PARM_T
    typedef struct{
        float smooth;
        float time;
    }voroparm_t;

    #define VORO_PARM_T voroparm_t
#endif

typedef struct{
    float sdist;
    float cdist;
    float3 ccol;
    int3 cid;
    float3 cpos;
}vorocell_t;

typedef struct{
    float sdist;
    float edist;
    float cdist;
    int3 cid;
    float3 cpos;
}voroedge_t;

// #define HASH_I lowbias32
float3 __attribute__((overloadable)) vorohash33(int3 x){
    return convert_float3(pcg3d(as_uint3(x)))/(float3)(0xffffffffU);
}
float3 __attribute__((overloadable)) vorohash33(float3 x){
    return hashWithoutSine33(x);
}
float __attribute__((overloadable)) vorohash11(float x){
    return lowbias32( as_uint( (int)(x) ) ) / (float)(0xffffffffu);
}

#ifndef VORO_UPDATE
    void voroUpdate_default(float3 *offset, float3 *color, float3 gid, VORO_PARM_T parm){
        float3 hash3 = vorohash33(convert_int3(gid));
        (*offset) = hash3;
        (*color) = normalize(hash3);
    }

    #define VORO_UPDATE voroUpdate_default
#endif

#ifndef VORO_SMIN_T
    // Cubic Polynomial Smooth-minimum
    float2 vorosmin( float a, float b, float k )
    {
        k *= 6.0;
        float h = max( k-fabs(a-b), 0.0f )/k;
        float m = h*h*h*0.5;
        float s = m*k*(1.0/3.0); 
        return (a<b) ? (float2)(a-s,m) : (float2)(b-s,1.0-m);
    }

    // DD  cubic polynomial
    // float vorosmin( float a, float b, float k ){
    //     k *= 16.0/3.0;
    //     float h = max( k-fabs(a-b), 0.0f)/k;
    //     return min(a,b) - h*h*h*(4.0-h)*k*(1.0/16.0);
    // }
    #define VORO_SMIN_T vorosmin
#endif

//return float4 instead?
vorocell_t __attribute__((overloadable)) vorocellIq( float3 x, VORO_PARM_T parm){

    vorocell_t res = {.sdist = 8, .cdist =8};

    float3 n = floor( x );
    float3 f = fract( x, &f);

    for( int k=-2; k<=2; k++ )
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        float3 g = (float3)((float)i, (float)j, (float)k);

        float3 off = 0.0f; 
        float3 col = 0.0f;

        VORO_UPDATE(&off, &col, n+g, parm);

        float3 pos = g - f + off;
		float d = length(pos);    // distance to cell

        if( d<res.cdist){
            res.cdist = d;
            res.cid = convert_int3(n+g);
            res.cpos = pos;
        }

        // float2 sm = VORO_SMIN_T( res.sdist, d, parm.smooth);
        // res.sdist = sm.x;
        // res.ccol = mix( res.ccol , col, sm.y);

		float h = smoothstep( -1.0f, 1.0f, (res.sdist-d)/parm.smooth ); // do the smooth min for colors and sdistances
	    res.sdist = mix( res.sdist,   d, h ) - h*(1.0-h)*parm.smooth/(1.0+3.0*parm.smooth); // sdistance
		res.ccol  = mix( res.ccol , col, h ) - (float3)(h*(1.0-h)*parm.smooth/(1.0+3.0*parm.smooth)); // color
    }
	
	return res;
}

float4 __attribute__((overloadable)) vorocellIq( float4 p4, float smooth){
    VORO_PARM_T parm = { 
        .smooth = smooth,
        .time = p4.z
    };
    vorocell_t vcell = vorocellIq(p4.xyz, parm);
    return (float4)(vcell.sdist, vcell.ccol);
}

voroedge_t __attribute__((overloadable)) voroedgeIq(float3 x, VORO_PARM_T parm){

    voroedge_t res = {.sdist = 8, .edist = 8, .cdist = 8};

    float3 n = floor(x);
    float3 f = fract(x, &f);
    float3 mg; 
    
    //---------------------------------- first pass: regular voronoi
        for( int k=-2; k<=2; k++ )
        for( int j=-2; j<=2; j++ )
        for( int i=-2; i<=2; i++ ){

            float3 g = (float3)(i,j,k);

            float3 off = 0.0f;
            float3 col = 0.0f;
            VORO_UPDATE(&off, &col, n+g, parm);

            float3 pos = g + off - f;
            float d = dot(pos,pos);

            if( d<res.cdist ){

                res.cdist = d;
                res.cpos = pos;
                res.cid = convert_int3(n+g);
                mg = g;
            }
        }

    //---------------------------------- second pass: distance to borders
        for( int k=-2; k<=2; k++ )
        for( int j=-2; j<=2; j++ )
        for( int i=-2; i<=2; i++ ){

            float3 g = mg + (float3)(i,j,k);

            float3 off = 0.0f;
            float3 col = 0.0f;
            VORO_UPDATE(&off, &col, n+g, parm);

            float3 r = g + off - f;

            if( dot(res.cpos-r,res.cpos-r)>0.00001 ){
                
                float d = dot(0.5f*(res.cpos+r), normalize(r-res.cpos));                
                res.edist = min( res.edist, d);

                // res.sdist = VORO_SMIN_T( res.sdist, d, parm.smooth).x;

                float h = smoothstep( -1.0f, 1.0f, (res.sdist-d)/parm.smooth ); // do the smooth min for colors and sdistances
                res.sdist = mix( res.sdist,   d, h ) - h*(1.0-h)*parm.smooth/(1.0+3.0*parm.smooth); // sdistance
            }
        }

        return res;
}

float4 __attribute__((overloadable)) voroedgeIq( float4 p4, float smooth){
    VORO_PARM_T parm = { 
        .smooth = smooth,
        .time = p4.z
    };
    voroedge_t vedge = voroedgeIq(p4.xyz, parm);
    return (float4)(vedge.sdist, vedge.cpos);
}

#endif
