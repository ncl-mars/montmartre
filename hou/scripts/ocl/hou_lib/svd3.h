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
 * NAME:    svd3.h (CE Library, OpenCL)
 *
 * COMMENTS:
 *    Quick 3 x 3 singular value decomposition from the paper:
 *    Computing the Singular Value Decomposition of 3 x 3 matrices with
 *    minimal branching and elementary floating point operations
 */

#ifndef SVD3_OPENCL_H
#define SVD3_OPENCL_H

#include <typedefines.h>
#include <platform.h>
#include <matrix.h>

#define COS_PI_OVER_EIGHT 0.9238795325112867f
#define SIN_PI_OVER_EIGHT 0.3826834323650897f
#define GAMMA 5.82842712474619f
#define EPSILON 1.0e-12f

// Givens rotation types to eliminate lower diagonal elements
#define ROT_X_21 21 // zero out element (i=2, j=1)
#define ROT_Y_20 20 // zero out element (i=2, j=0)
#define ROT_Z_10 10 // zero out element (i=1, j=0)

// Returns the square of the L2-norm of a vec3
static float
svd3_length2(const float3 v)
{
    return dot(v, v);
}

// Swap x and y if c is true.
static void
svd3_cond_swap(const bool c, float *x, float *y)
{
    const float tmp = *x;
    *x = (c ? *y : *x);
    *y = (c ? tmp : *y);
}

// Negate x and swap x and y if c is true.
static void
svd3_cond_neg_swap(const bool c, float3 *x, float3 *y)
{
    const float3 tmp = -(*x);
    *x = (c ? *y : *x);
    *y = (c ? tmp : *y);
}

// Compute the cosine and sine given the two non-zero elements of an
// axis-aligned quaternion.
static void
svd3_cos_sin_from_quat(const float ch, const float sh, float *c, float *s)
{
    float ch2 = ch * ch;
    float sh2 = sh * sh;
    float w = 1.0f / (ch2 + sh2);

    *c = (ch2 - sh2) * w;
    *s = (2.0f * sh * ch) * w;
}

// Compute the quaternion representation of an approximated Givens rotation
// that reduces the magnitude of the off-diagonal element doff in the symmetric
// matrix ATA. For example if the element to reduce is ATA_10 (i=1, j=0) then
// the two diagonal elements are d0 = ATA_jj = ATA_00 and d1 = ATA_ii = ATA_11.
static quat
svd3_jacobi_approx_givens_quaternion(
        const int mode,
        const float d0,
        const float d1,
        const float doff)
{
    float ch = 2.0f * (d0 - d1);
    float sh = doff;
    bool b = (GAMMA * sh * sh) < (ch * ch);

    float w = rsqrt(ch * ch + sh * sh);
    ch = (b ? w * ch : COS_PI_OVER_EIGHT);
    sh = (b ? w * sh : SIN_PI_OVER_EIGHT);

    return (float4)(mode == ROT_X_21 ? sh : 0.0f,
                    mode == ROT_Y_20 ? -sh : 0.0f,
                    mode == ROT_Z_10 ? sh : 0.0f, ch);
}

// Single iteration of Jacobi conjugation with Givens rotations to diagonalize
// the 3x3 symmetric matrix ATA. The Givens rotations are accumulated in
// quaternion qV.
static void
svd3_jacobi_conjugation(const int mode, mat3 ATA, quat *qV)
{
    float c, s;
    quat q_rot;
    mat3 Q;

    if (mode == ROT_Z_10)
    {
        // q_rot = (0, 0, sh, ch)
        q_rot = svd3_jacobi_approx_givens_quaternion(mode, ATA[0].x, ATA[1].y, ATA[1].x);
        svd3_cos_sin_from_quat(q_rot.w, q_rot.z, &c, &s);

        Q[0] = (float3)(c,      -s, 0.0f);
        Q[1] = (float3)(s,       c, 0.0f);
        Q[2] = (float3)(0.0f, 0.0f, 1.0f);
    }
    else if (mode == ROT_Y_20)
    {
        // q_rot = (0, -sh, 0, ch)
        q_rot = svd3_jacobi_approx_givens_quaternion(mode, ATA[0].x, ATA[2].z, ATA[2].x);
        svd3_cos_sin_from_quat(q_rot.w, -q_rot.y, &c, &s);

        Q[0] = (float3)(c,    0.0f,   -s);
        Q[1] = (float3)(0.0f, 1.0f, 0.0f);
        Q[2] = (float3)(s,    0.0f,    c);
    }
    else // mode == ROT_X_21
    {
        // q_rot = (sh, 0, 0, ch)
        q_rot = svd3_jacobi_approx_givens_quaternion(mode, ATA[1].y, ATA[2].z, ATA[2].y);
        svd3_cos_sin_from_quat(q_rot.w, q_rot.x, &c, &s);

        Q[0] = (float3)(1.0f, 0.0f, 0.0f);
        Q[1] = (float3)(0.0f,    c,   -s);
        Q[2] = (float3)(0.0f,    s,    c);
    }

    // Q^T * ATA * Q
    mat3 QT, QTATA;
    transpose3(Q, QT);
    mat3mul(QT, ATA, QTATA);
    mat3mul(QTATA, Q, ATA);

    // cumulative rotation update
    *qV = qmultiply(*qV, q_rot);
}

// Iteratively compute the quaternion representation of the matrix V such that
// ATA = V^T * S^2 * V by diagonalising the symmetric 3x3 matrix ATA using
// Jacobi conjugation with Givens rotations.
static quat
svd3_jacobi_eigen_analysis(mat3 ATA)
{
    quat qV = (float4)(0.0f, 0.0f, 0.0f, 1.0f);

    for (int i = 0; i < 4; i++)
    {
        svd3_jacobi_conjugation(ROT_Z_10, ATA, &qV);
        svd3_jacobi_conjugation(ROT_X_21, ATA, &qV);
        svd3_jacobi_conjugation(ROT_Y_20, ATA, &qV);
    }

    return qV;
}

// Reorder matrices B and V such that singular values S in B (= U*S) are in
// decreasing order.
static void
svd3_singular_values_sort(mat3 B, mat3 V)
{
    mat3 BT, VT;

    // to column-major order
    transpose3(B, BT);
    transpose3(V, VT);

    float s0 = svd3_length2(BT[0]);
    float s1 = svd3_length2(BT[1]);
    float s2 = svd3_length2(BT[2]);

    bool c = (s0 < s1);
    svd3_cond_neg_swap(c, &BT[0], &BT[1]);
    svd3_cond_neg_swap(c, &VT[0], &VT[1]);
    svd3_cond_swap(c, &s0, &s1);

    c = (s0 < s2);
    svd3_cond_neg_swap(c, &BT[0], &BT[2]);
    svd3_cond_neg_swap(c, &VT[0], &VT[2]);
    svd3_cond_swap(c, &s0, &s2);

    c = (s1 < s2);
    svd3_cond_neg_swap(c, &BT[1], &BT[2]);
    svd3_cond_neg_swap(c, &VT[1], &VT[2]);

    // to row-major order
    transpose3(BT, B);
    transpose3(VT, V);
}

// Compute a Givens quaternion from a pivot element on the diagonal and a
// off-diagonal element to zero out. For example to eliminate element b_10
// (i=1, j=0 in matrix B) we use: d = b_jj = b_00 and a = b_ij = b_10.
static quat
svd3_qr_givens_quaternion(const int mode, const float d, const float a)
{
    const float p = sqrt(d * d + a * a);
    float sh = (p > EPSILON) ? a : 0.0f;
    float ch = fabs(d) + fmax(p, EPSILON);

    const bool b = (d < 0.0f);
    svd3_cond_swap(b, &sh, &ch);

    const float w = rsqrt(ch * ch + sh * sh);
    ch *= w;
    sh *= w;

    return (float4)(mode == ROT_X_21 ?  sh : 0.0f,
                    mode == ROT_Y_20 ? -sh : 0.0f,
                    mode == ROT_Z_10 ?  sh : 0.0f, ch);
}

// Compute the QR decomposition of a 3x3 matrix B such that B = Q * R using
// Givens rotation where Q is orthogonal and R is upper triangular (diagonal
// in our case).
static void
svd3_qr_decomposition(mat3 B, mat3 Q, mat3 R)
{
    float c, s;
    mat3 QT;

    // eliminate element b_10 from matrix B with q_rot = (0, 0, sh, ch)
    quat q_rot_z = svd3_qr_givens_quaternion(ROT_Z_10, B[0].x, B[1].x);
    svd3_cos_sin_from_quat(q_rot_z.w, q_rot_z.z, &c, &s);
    
    QT[0] = (float3)(   c,    s, 0.0f);
    QT[1] = (float3)(  -s,    c, 0.0f);
    QT[2] = (float3)(0.0f, 0.0f, 1.0f);
    mat3mul(QT, B, R);

    // eliminate element b_20 from matrix B with q_rot = (0, -sh, 0, ch)
    quat q_rot_y = svd3_qr_givens_quaternion(ROT_Y_20, R[0].x, R[2].x);
    svd3_cos_sin_from_quat(q_rot_y.w, -q_rot_y.y, &c, &s);
    
    QT[0] = (float3)(   c, 0.0f,    s);
    QT[1] = (float3)(0.0f, 1.0f, 0.0f);
    QT[2] = (float3)(  -s, 0.0f,    c);
    mat3mul(QT, R, B);

    // eliminate element b_21 from matrix B with q_rot = (sh, 0, 0, ch)
    quat q_rot_x = svd3_qr_givens_quaternion(ROT_X_21, B[1].y, B[2].y);
    svd3_cos_sin_from_quat(q_rot_x.w, q_rot_x.x, &c, &s);
    
    QT[0] = (float3)(1.0f, 0.0f, 0.0f);
    QT[1] = (float3)(0.0f,    c,    s);
    QT[2] = (float3)(0.0f,   -s,    c);
    mat3mul(QT, B, R);

    // get orthogonal rotation matrix Q from quaternions
    qtomat3T(qmultiply(q_rot_z, qmultiply(q_rot_y, q_rot_x)), Q);
}

// Compute the singular value decomposition of a 3x3 matrix A into U, S, V such
// that A = U * S * V^T. The singular values in diagonal matrix S are ordered in
// decreasing order while U and V are both orthogonal (rotation) matrices.
static void
svd3(const mat3 A, mat3 U, mat3 S, mat3 V)
{
    // compute ATA = A^T * A
    mat3 ATA, AT;
    transpose3(A, AT);
    mat3mul(AT, A, ATA);

    // compute V from ATA = V * S^2 * V^T
    quat qV = svd3_jacobi_eigen_analysis(ATA);
    qtomat3T(qV, V);

    // reorder B and V such that singular values are in decreasing order
    mat3 B;
    mat3mul(A, V, B);
    svd3_singular_values_sort(B, V);

    // compute U and S from A * V = B
    svd3_qr_decomposition(B, U, S);
}

#endif