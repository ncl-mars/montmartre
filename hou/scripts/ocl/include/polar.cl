#ifndef POLAR_CL
#define POLAR_CL

//-- 2D
float2 __attribute__((overloadable)) cartToPol(float2 p) {

    return (float2)
    (
        sqrt(p.x*p.x + p.y*p.y),    // r
        atan2(p.y, p.x)             // θ
    );
}

float2 __attribute__((overloadable)) polToCart(float2 p) {

    return (float2)
    (
        p.x * cos(p.y),       
        p.x * sin(p.y)
    );
}

//-- 3D
float3 __attribute__((overloadable)) cartToPol(float3 p) {

    float r = sqrt(p.x*p.x + p.y*p.y + p.z*p.z);

    return (float3)
    (
        r,
        atan2(p.y, p.x),        // θ
        acos(p.z / r)           // φ
    );
}

float3 __attribute__((overloadable)) polToCart(float3 p) {

    return (float3)
    (
        p.x * sin(p.z) * cos(p.y), // x
        p.x * sin(p.z) * sin(p.y), // y
        p.x * cos(p.z)
    );
}


#endif


// float3 __attribute__((overloadable)) cartToPolar(float3 p){

//     float r = sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
//     float theta = atan2(p.z, p.x);
//     float phi = (r > 0.0f) ? acos(clamp(p.y / r, -1.0f, 1.0f)) : 0.0f;

//     return (float3)(r,theta,phi);
// }

// float3 __attribute__((overloadable)) polarToCart(float3 p){

//     float r = sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
//     float theta = atan2(p.z, p.x);
//     float phi = (r > 0.0f) ? acos(clamp(p.y / r, -1.0f, 1.0f)) : 0.0f;

//     return (float3)(r,theta,phi);
// }
