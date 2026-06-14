#ifndef __SDF_H__
#ifndef SDF_OP_CL
#define SDF_OP_CL

float sdfOpUnion(float _d1, float _d2){
    return min(_d1, _d2);
}
float sdfOpUnionSmooth(float _d1, float _d2, float _k){
    if (_k == 0) return sdfOpUnion(_d1, _d2);

    float h = max(_k - fabs(_d1 - _d2), 0.0f);
    return min(_d1, _d2) - h * h * 0.25f / _k;
}
float sdfOpUnionRound(float _d1, float _d2, float _k){
    float2 u = max((float2)(_k - _d1, _k - _d2), (float2)(0.0f, 0.0f));
    return max(_k, min(_d1, _d2)) - length(u);
}
float sdfOpUnionChamfer(float _d1, float _d2, float _k){
    return min(min(_d1, _d2), (_d1 - _k + _d2) * sqrt(0.5f));
}
// OP INTERSECT
float sdfOpIntersect(float _d1, float _d2){
    return max(_d1, _d2);
}
float sdfOpIntersectSmooth(float _d1, float _d2, float _k){
    return -sdfOpUnionSmooth(-_d1, -_d2, _k);
}
float sdfOpIntersectRound(float _d1, float _d2, float _k){
    float2 u = max((float2)(_k + _d1, _k + _d2), (float2)(0.0f, 0.0f));
    return min(-_k, max(_d1, _d2)) + length(u);
}
float sdfOpIntersectChamfer(float _d1, float _d2, float _k){
    return max(max(_d1, _d2), (_d1 + _k + _d2) * sqrt(0.5f));
}
// OP SUBTRACT
float sdfOpSubtract(float _d1, float _d2){
    return sdfOpIntersect(_d1, -_d2);
}
float sdfOpSubtractSmooth(float _d1, float _d2, float _k){
    return sdfOpIntersectSmooth(_d1, -_d2, _k);
}
float sdfOpSubtractRound(float _d1, float _d2, float _k){
    return sdfOpIntersectRound(_d1, -_d2, _k);
}
float sdfOpSubtractChamfer(float _d1, float _d2, float _k){
    return sdfOpIntersectChamfer(_d1, -_d2, _k);
}
// OP DIFFERENCE
float sdfOpDifference(float _d1, float _d2){
    return max(min(_d1, _d2), -max(_d1, _d2));
}
float sdfOpDifferenceSmooth(float _d1, float _d2, float _k){
    if (_k == 0) return sdfOpDifference(_d1, _d2);

    float h = max(_k - fabs(_d1 - _d2), 0.0f);
    float l = h * h * 0.25f / _k;
    return max(min(_d1, _d2) - l, -max(_d1, _d2) - l);
}
float sdfOpDifferenceRound(float _d1, float _d2, float _k){
    return min(sdfOpIntersectRound(-_d1, _d2, _k), sdfOpIntersectRound(_d1, -_d2, _k));
}
float sdfOpDifferenceChamfer(float _d1, float _d2, float _k){
    return min(sdfOpIntersectChamfer(-_d1, _d2, _k), sdfOpIntersectChamfer(_d1, -_d2, _k));
}

#endif
#endif