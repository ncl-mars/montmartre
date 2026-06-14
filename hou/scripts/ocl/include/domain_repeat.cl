#ifndef DOMAIN_REPEAT_CL
#define DOMAIN_REPEAT_CL

float2 __attribute__((overloadable)) opRepLim( float2 p, float s, float2 lima, float2 limb )
{
    return p-s*clamp(round(p/s),lima,limb);
}


#ifndef SHAPE
    float defaultShape(float2 pos, float2 id)
    {
        return length(pos)-0.5f;
    }
    #define SHAPE defaultShape
#endif

float __attribute__((overloadable)) opRepLim( float2 p, float2 s, float2 rep )
{
    float2 id = round(p/s);
    float2 off = sign(p-s*id);

    float d = 1e20f;
    for( int j=0; j<2; j++ )
    for( int i=0; i<2; i++ )
    {
        float2 rid = id + (float2)((float)i,(float)j)*off;
        rid = clamp(rid,-rep,rep); // limited repetition
        float2 r = p - s*rid;
        d = min( d, SHAPE(r,rid) );
    }

    return d;

}

float opRepLimMirror( float2 p, float2 s, float2 rep )
{
    float2 id = round(p/s);
    id = clamp(id,-rep,rep); // limited repetition
    float2 r = p - s*id;

    r = (float2)( (((int)(id.x)&1)==0) ? r.x : -r.x,
                  (((int)(id.y)&1)==0) ? r.y : -r.y );

    return SHAPE( r, id );
    // return (float4)(r, id);
}


// https://www.shadertoy.com/view/XtSczV
float opRepRot( float2 p, int n )
{
    float sp = 6.283185f/(float)(n);
    float an = atan2(p.y,p.x);
    float id = floor(an/sp);

    float a1 = sp*(id+0.0f);
    float a2 = sp*(id+1.0f);
    float ca1 = cos(a1), sa1 = sin(a1);
    float ca2 = cos(a2), sa2 = sin(a2);
    float2 r1 = (float2)(ca1*p.x - sa1*p.y, sa1*p.x + ca1*p.y);
    float2 r2 = (float2)(ca2*p.x - sa2*p.y, sa2*p.x + ca2*p.y);

    return min( SHAPE(r1,id+0.0f), SHAPE(r2,id+1.0f) );
}


// rectangular repetition - ONLY WORKS FOR SYMMETRIC SHAPES
float2 opRepRect( float2 p, float2 size, float s )
{
    p = fabs(p/s) - (size*0.5f - 0.5f);
    p = (p.x<p.y) ? p.yx : p.xy;
    p.y -= min(0.0f, round(p.y));
    return p*s;
}

#endif
