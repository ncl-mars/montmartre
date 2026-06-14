// dave hoskin, copyright at https://www.shadertoy.com/view/4djSRW
#ifndef HASH_CL
#define HASH_CL

#include "./utils.cl" // frac, mod...

//   #define hash(x)  ( float( hashi(x) ) / float( 0xffffffffU ) )

//FNeyret
//bias: 0.17353355999581582 ( very probably the best of its kind )
uint lowbias32(uint x){

    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}
// bias: 0.020888578919738908 = minimal theoretic limit
uint triple32(uint x){

    x ^= x >> 17;
    x *= 0xed5ad4bbU;
    x ^= x >> 11;
    x *= 0xac4c1b51U;
    x ^= x >> 15;
    x *= 0x31848babU;
    x ^= x >> 14;
    return x;
}

float3 hashWithoutSine33(float3 p3) {
	p3 = frac(p3 * (float3)(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33f);
    return frac((p3.xxy + p3.yxx)*p3.zyx);
}

float4 hashWithoutSine43(float3 p){

	float4 p4 = frac( (float4)(p.xyzx)*(float4)(.1031,.1030,.0973,.1099));
    
    p4 += dot(p4, p4.wzxy+33.33f);

    return frac((p4.xxyz+p4.yzzw)*p4.zywx);
}


// https://www.pcg-random.org/
uint pcg(uint v){

	uint state = v * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

uint2 pcg2d(uint2 v){

    v = v * 1664525u + 1013904223u;

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u; // optionnal?

    v = v ^ (v>>16u);

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v>>16u);

    return v;
}
// http://www.jcgt.org/published/0009/03/02/
uint3 pcg3d(uint3 v) {

    v = v * 1664525u + 1013904223u;

    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

    v = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u; // optionnal?

    v ^= v >> 16u;

    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

    return v;
}
// http://www.jcgt.org/published/0009/03/02/
uint3 pcg3d16(uint3 v){

    v = v * 12829u + 47989u;

    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

	v >>= 16u;

    return v;
}
// http://www.jcgt.org/published/0009/03/02/
uint4 pcg4d(uint4 v){

    v = v * 1664525u + 1013904223u;
    
    v.x += v.y*v.w;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    v.w += v.y*v.z;

    v = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u; // optionnal?
    
    v ^= v >> 16u;
    
    v.x += v.y*v.w;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    v.w += v.y*v.z;
    
    return v;
}

#endif


//
    // float hash11(float p){

    //     float _p = p;
    //     _p = fract(_p * .1031, &_p);
    //     _p *= _p + 33.33;
    //     _p *= _p + _p;
    //     return fract(_p, &_p);
    // }
    // //  1 out, 2 in...
    // float hash12(float2 p){
    // 	float3 p3  = frac(float3(p.xyx) * .1031);
    //     p3 += dot(p3, p3.yzx + 33.33);
    //     return frac((p3.x + p3.y) * p3.z);
    // }
    // //  1 out, 3 in...
    // float hash13(float3 p3){

    // 	p3  = frac(p3 * .1031);
    //     p3 += dot(p3, p3.zyx + 31.32);
    //     return frac((p3.x + p3.y) * p3.z);

    // // 1 out 4 in...
    // float hash14(float4 p4){

    // 	p4 = frac(p4  * float4(.1031, .1030, .0973, .1099));
    //     p4 += dot(p4, p4.wzxy+33.33);
    //     return frac((p4.x + p4.y) * (p4.z + p4.w));
    // }
    // //  2 out, 1 in...
    // float2 hash21(float p){

    // 	float3 p3 = frac(float3(p) * float3(.1031, .1030, .0973));
    // 	p3 += dot(p3, p3.yzx + 33.33);
    //     return frac((p3.xx+p3.yz)*p3.zy);

    // }
    // ///  2 out, 2 in...
    // float2 hash22(float2 p){

    // 	float3 p3 = frac(float3(p.xyx) * float3(.1031, .1030, .0973));
    //     p3 += dot(p3, p3.yzx+33.33);
    //     return frac((p3.xx+p3.yz)*p3.zy);

    // }
    // ///  2 out, 3 in...
    // float2 hash23(float3 p3){

    // 	p3 = frac(p3 * float3(.1031, .1030, .0973));
    //     p3 += dot(p3, p3.yzx+33.33);
    //     return frac((p3.xx+p3.yz)*p3.zy);
    // }
    // //  3 out, 1 in...
    float3 hash31(float p){

       float3 p3 = frac((float3)(p) * (float3)(.1031, .1030, .0973));
       p3 += dot(p3, p3.yzx+33.33f);
       return frac((p3.xxy+p3.yzz)*p3.zyx); 
    }

    // ///  3 out, 2 in...
    // float3 hash32(float2 p){

    // 	float3 p3 = frac(float3(p.xyx) * float3(.1031, .1030, .0973));
    //     p3 += dot(p3, p3.yxz+33.33);
    //     return frac((p3.xxy+p3.yzz)*p3.zyx);
    // }
    // ///  3 out, 3 in...
    // float3 hash33(float3 p3){

    // 	p3 = frac(p3 * float3(.1031, .1030, .0973));
    //     p3 += dot(p3, p3.yxz+33.33);
    //     return frac((p3.xxy + p3.yxx)*p3.zyx);

    // }
    // // 4 out, 1 in...
    // float4 hash41(float p){

    // 	float4 p4 = frac(float4(p) * float4(.1031, .1030, .0973, .1099));
    //     p4 += dot(p4, p4.wzxy+33.33);
    //     return frac((p4.xxyz+p4.yzzw)*p4.zywx);
        
    // }
    // // 4 out, 2 in...
    // float4 hash42(float2 p){

    // 	float4 p4 = frac(float4(p.xyxy) * float4(.1031, .1030, .0973, .1099));
    //     p4 += dot(p4, p4.wzxy+33.33);
    //     return frac((p4.xxyz+p4.yzzw)*p4.zywx);

    // }
    // // 4 out, 3 in...
    // float4 hash43(float3 p){

    // 	float4 p4 = frac(float4(p.xyzx)  * float4(.1031, .1030, .0973, .1099));
    //     p4 += dot(p4, p4.wzxy+33.33);
    //     return frac((p4.xxyz+p4.yzzw)*p4.zywx);
    // }
    // // 4 out, 4 in...
    // float4 hash44(float4 p4){

    // 	p4 = frac(p4  * float4(.1031, .1030, .0973, .1099));
    //     p4 += dot(p4, p4.wzxy+33.33);
    //     return frac((p4.xxyz+p4.yzzw)*p4.zywx);
    // }

