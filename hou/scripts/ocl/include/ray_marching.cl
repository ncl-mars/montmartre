#ifndef RAYMARCHING_CL
#define RAYMARCHING_CL


#ifndef _MAP_
    float defaultMap(float3 pos)
    {
        return length(pos)-0.5f;
    }
    #define _MAP_ defaultMap
#endif



// https://iquilezles.org/articles/normalsSDF
float3 calcNormal(float3 pos )
{
    float2 e = (float2)(1.0,-1.0)*(float2)(0.5773*0.0005);
    return normalize( e.xyy*_MAP_( pos + e.xyy )+ 
					  e.yyx*_MAP_( pos + e.yyx )+ 
					  e.yxy*_MAP_( pos + e.yxy )+ 
					  e.xxx*_MAP_( pos + e.xxx ));
}


#endif