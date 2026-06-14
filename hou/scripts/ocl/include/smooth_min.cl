
#ifndef SMOOTH_MIN_CL
#define SMOOTH_MIN_CL

// https://iquilezles.org/articles/smin/

// -------------------------------------------- Direct Diffrence Family
// exponential
float smin_exp( float a, float b, float k ){
    k *= 1.0;
    float r = exp2(-a/k) + exp2(-b/k);
    return -k*log2(r);
}
// root
float smin_root( float a, float b, float k ){
    k *= 2.0;
    float x = b-a;
    return 0.5*( a+b-sqrt(x*x+k*k) );
}
// sigmoid
float smin_sigm( float a, float b, float k ){
    k *= log(2.0);
    float x = b-a;
    return a + x/(1.0-exp2(x/k));
}
// quadratic polynomial
float smin_quad( float a, float b, float k ){
    k *= 4.0;
    float h = max( k-fabs(a-b), 0.0f )/k;
    return min(a,b) - h*h*k*(1.0/4.0);
}
// cubic polynomial
float smin_cube( float a, float b, float k ){
    k *= 6.0;
    float h = max( k-fabs(a-b), 0.0f)/k;
    return min(a,b) - h*h*h*k*(1.0/6.0);
}
// quartic polynomial
float smin_quart( float a, float b, float k ){
    k *= 16.0/3.0;
    float h = max( k-fabs(a-b), 0.0f)/k;
    return min(a,b) - h*h*h*(4.0-h)*k*(1.0/16.0);
}
// circular
float smin_circ( float a, float b, float k ){
    k *= 1.0/(1.0-sqrt(0.5));
    float h = max( k-fabs(a-b), 0.0f)/k;
    return min(a,b) - k*0.5*(1.0+h-sqrt(1.0-h*(h-2.0)));
}
// circular geometrical
float smin_circ_geo( float a, float b, float k ){
    k *= 1.0/(1.0-sqrt(0.5));
    return max(k,min(a,b)) -
           length(max(k-(float2)(a,b),0.0f));
}

// -------------------------------------------- Clamped Differences Family
    // // quadratic polynomial
    // float smin( float a, float b, float k )
    // {
    //     k *= 4.0;
    //     float h = max( k-abs(a-b), 0.0 )/k;
    //     return min(a,b) - h*h*k*(1.0/4.0);
    // }
    // cubic polynomial
    // float smin( float a, float b, float k )
    // {
    //     k *= 6.0;
    //     float h = max( k-fabs(a-b), 0.0f )/k;
    //     return min(a,b) - h*h*h*k*(1.0/6.0);
    // }
    // // quartic polynomial
    // float smin( float a, float b, float k )
    // {
    //     k *= 16.0/3.0;
    //     float h = max( k-abs(a-b), 0.0 )/k;
    //     return min(a,b) - h*h*h*(4.0-h)*k*(1.0/16.0);
    // }


#endif