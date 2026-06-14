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
 * NAME:    matrix.h ( CE Library, OpenCL)
 *
 * COMMENTS:
 */

#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "typedefines.h"
#include "util.h"

#define PRINTI(v)                                                              \
    printf("%s:\n", #v);                                                       \
    printf("%d\n", v)

#define PRINTU(v)                                                              \
    printf("%s:\n", #v);                                                       \
    printf("%u\n", v)

#define PRINTLI(v)                                                             \
    printf("%s:\n", #v);                                                       \
    printf("%lld\n", v)

#define PRINTLU(v)                                                             \
    printf("%s:\n", #v);                                                       \
    printf("%llu\n", v)

#define PRINTF(v)                                                              \
    printf("%s:\n", #v);                                                       \
    printf("%g\n", v)

#define PRINTVEC3(v)                                                           \
    printf("%s:\n", #v);                                                       \
    printf("%g %g %g\n", v.x, v.y, v.z)

#define PRINTVEC3I(v)                                                          \
    printf("%s:\n", #v);                                                       \
    printf("%d %d %d\n", v.x, v.y, v.z)

#define PRINTMAT3(m)                                                           \
    printf("%s:\n", #m);                                                       \
    printf("%g %g %g\n", m[0].s0, m[0].s1, m[0].s2);                           \
    printf("%g %g %g\n", m[1].s0, m[1].s1, m[1].s2);                           \
    printf("%g %g %g\n", m[2].s0, m[2].s1, m[2].s2)

#define PRINTMAT4(m)                                                           \
    printf("%s:\n", #m);                                                       \
    printf("%g %g %g %g\n", m.s0, m.s1, m.s2, m.s3);                           \
    printf("%g %g %g %g\n", m.s4, m.s5, m.s6, m.s7);                           \
    printf("%g %g %g %g\n", m.s8, m.s9, m.sa, m.sb);                           \
    printf("%g %g %g %g\n", m.sc, m.sd, m.se, m.sf)

// A 3x3 matrix in row-major order (to match UT_Matrix3)
// NOTE: fpreal3 is 4 floats, so this is size 12
typedef fpreal3 mat3[3];  

// A 3x2 matrix in row-major order
typedef fpreal2 mat32[3];

// A 2x2 matrix in row-major order, stored in a single fpreal4
typedef fpreal4 mat2;

// A 4x4 matrix in row-major order, stored in a single fpreal16
typedef fpreal16 mat4;

// Return the sum of entries of the vector v.
static fpreal
vec3sum(const fpreal3 v)
{
    return v.x + v.y + v.z;
}

// Return the product of entries of the vector v.
static fpreal
vec3prod(const fpreal3 v)
{
    return v.x * v.y * v.z;
}

// Create a 2x2 matrix with columns of the specified vectors.
static mat2
mat2fromcols(const fpreal2 c0, const fpreal2 c1)
{
    return (mat2)(c0.s0, c1.s0,
                  c0.s1, c1.s1);
}

// Transpose a 2x2 matrix.
static mat2
transpose2(const mat2 a)
{
    return (mat2)(a.even, a.odd);
}

// Multiply A * B for 2x2 matrices.
static mat2
mat2mul(const mat2 a, const mat2 b)
{
    return (mat2)(dot(a.lo, b.even), dot(a.lo, b.odd),
                  dot(a.hi, b.even), dot(a.hi, b.odd));
}

// Multiply b * A where b is a 2-vector and A is a 2x2 matrix.
// This multiplication order matches VEX.
static fpreal2
mat2vecmul(const mat2 a, const fpreal2 b)
{
    mat2 aT = transpose2(a);
    return (fpreal2)(dot(aT.lo, b), dot(aT.hi, b));
}

// Return the square of the L2-norm of a 2x2 matrix.
static fpreal
squaredNorm2(const mat2 a)
{
    return dot(a.lo, a.lo) + dot(a.hi, a.hi);
}

// Add 3x3 matrix A to matrix B and store the result in matrix C.
static void
mat3add(const mat3 a, const mat3 b, mat3 c)
{
    c[0] = a[0] + b[0];
    c[1] = a[1] + b[1];
    c[2] = a[2] + b[2];
}

// Subtract 3x3 matrix A from matrix B and store the result in C.
static void
mat3sub(const mat3 a, const mat3 b, mat3 c)
{
    c[0] = a[0] - b[0];
    c[1] = a[1] - b[1];
    c[2] = a[2] - b[2];
}

// Set 3x3 matrix A to zero.
static void
mat3zero(mat3 a)
{
    a[0] = (fpreal3)(0.0f);
    a[1] = (fpreal3)(0.0f);
    a[2] = (fpreal3)(0.0f);
}

// Set 3x3 matrix A to the identity.
static void
mat3identity(mat3 a)
{
    a[0] = (fpreal3)(1.0f, 0.0f, 0.0f);
    a[1] = (fpreal3)(0.0f, 1.0f, 0.0f);
    a[2] = (fpreal3)(0.0f, 0.0f, 1.0f);
}

// Copy 3x3 matrix A to matrix B.
static void
mat3copy(const mat3 a, mat3 b)
{
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
}

// Load a 3x3 matrix from memory at the specified index.
// The matrix should be row-major as stored in geometry
// attributes.
static void
mat3load(size_t idx, const global float *a, mat3 m)
{
    idx *= 3;
    m[0] = vload3f(idx, a);
    m[1] = vload3f(idx + 1, a);
    m[2] = vload3f(idx + 2, a);
}

// Store a 3x3 matrix to memory at the specified index.
// The matrix should be row-major as stored in geometry
// attributes.
void 
mat3store(mat3 in, int idx, global fpreal *data)
{
    idx *= 3;
    vstore3(in[0], idx, data);
    vstore3(in[1], idx + 1, data);
    vstore3(in[2], idx + 2, data);
}

// Create a 3x3 matrix with columns of the specified vectors.
static void
mat3fromcols(const fpreal3 c0, const fpreal3 c1, const fpreal3 c2, mat3 m)
{
    m[0] = (fpreal3)(c0.s0, c1.s0, c2.s0);
    m[1] = (fpreal3)(c0.s1, c1.s1, c2.s1);
    m[2] = (fpreal3)(c0.s2, c1.s2, c2.s2);
}

// Transpose a 3x3 matrix.
static void
transpose3(const mat3 a, mat3 b)
{
    mat3fromcols(a[0], a[1], a[2], b);
}

// Multiply A * B and store in C for 3x3 matrices.
static void
mat3mul(const mat3 a, const mat3 b, mat3 c)
{
    mat3 bT;
    transpose3(b, bT);
    c[0] = (fpreal3)(dot(a[0],bT[0]), dot(a[0],bT[1]), dot(a[0],bT[2]));
    c[1] = (fpreal3)(dot(a[1],bT[0]), dot(a[1],bT[1]), dot(a[1],bT[2]));
    c[2] = (fpreal3)(dot(a[2],bT[0]), dot(a[2],bT[1]), dot(a[2],bT[2]));
}

// Multiply b * A where b is a 3-vector and A is a 3x3 matrix.
// This multiplication order matches VEX.
static fpreal3
mat3vecmul(const mat3 a, const fpreal3 b)
{
    mat3 aT;
    transpose3(a, aT);
    return (fpreal3)(dot(aT[0], b), dot(aT[1], b), dot(aT[2], b));
}

// Multiply b * A^T where b is a 3-vector and A is a 3x3 matrix.
// This multiplication order matches VEX.
static fpreal3
mat3Tvecmul(const mat3 a, const fpreal3 b)
{
    return (fpreal3)(dot(a[0], b), dot(a[1], b), dot(a[2], b));
}

// Multiply b * A where b is a 3-vector and A is a 3x3 matrix,
// but discard the third component.
// This multiplication order matches VEX.
static fpreal2
mat3vec2mul(const mat3 a, const fpreal3 b)
{
    mat3 aT;
    transpose3(a, aT);
    return (fpreal2)(dot(aT[0], b), dot(aT[1], b));
}

// Multiply b * A^T where b is a 3-vector and A is a 3x3 matrix,
// but discard the third component.
// This multiplication order matches VEX.
static fpreal2
mat3Tvec2mul(const mat3 a, const fpreal3 b)
{
    return (fpreal2)(dot(a[0], b), dot(a[1], b));
}

// Store the 3x3 matrix that is the outer product of the input
// a and b vectors in C.
static void
outerprod3(const fpreal3 a, const fpreal3 b, mat3 c)
{
    c[0] = a.x * b;
    c[1] = a.y * b;
    c[2] = a.z * b;
}

// Compute C = s * A + t * B, where s, t are scalars and A, B are 3x3 matrices.
static void
mat3lcombine(const fpreal s, const mat3 a, const fpreal t, const mat3 b, mat3 c)
{
    c[0] = s * a[0] + t * b[0];
    c[1] = s * a[1] + t * b[1];
    c[2] = s * a[2] + t * b[2];
}

// Return the square of the L2-norm of a 3x3 matrix.
static fpreal
squaredNorm3(const mat3 a)
{
    return dot(a[0], a[0]) + dot(a[1], a[1]) + dot(a[2], a[2]);
}

// Return the determinant of the supplied 3x3 matrix.
static fpreal
det3(const mat3 a)
{
    fpreal d = a[0].s0 * (a[1].s1 * a[2].s2 - a[1].s2 * a[2].s1);
    d -= a[0].s1 * (a[1].s0 * a[2].s2 - a[1].s2 * a[2].s0);
    d += a[0].s2 * (a[1].s0 * a[2].s1 - a[1].s1 * a[2].s0);
    return d;
}

// Return diagonal vector of the supplied 3x3 matrix.
static fpreal3
diag3(const mat3 a)
{
    return (fpreal3)(a[0].s0, a[1].s1, a[2].s2);
}

// Set a to the 3x3 diagonal matrix defined by the entries of the vector diag.
static void
mat3diag(const fpreal3 diag, mat3 a)
{
    mat3zero(a);
    a[0].x = diag.x;
    a[1].y = diag.y;
    a[2].z = diag.z;
}

// Return the trace of the supplied 3x3 matrix.
static fpreal
trace3(const mat3 m)
{
    return vec3sum(diag3(m));
}

// Set 4x4 matrix A to the identity.
static void
mat4identity(mat4 *a)
{
    (*a).lo.lo = (fpreal4)(1.0f, 0.0f, 0.0f, 0.0f);
    (*a).lo.hi = (fpreal4)(0.0f, 1.0f, 0.0f, 0.0f);
    (*a).hi.lo = (fpreal4)(0.0f, 0.0f, 1.0f, 0.0f);
    (*a).hi.hi = (fpreal4)(0.0f, 0.0f, 0.0f, 1.0f);
}

// Multiply b * A where b is a 2-vector and A is a 4x4 matrix.
// This multiplication order matches VEX.
static fpreal2
mat4vec2mul(const mat4 a, const fpreal2 b)
{
    return b.x * a.lo.lo.xy +
           b.y * a.lo.hi.xy +
                 a.hi.hi.xy;
}

// Multiply b * A where b is a 3-vector and A is a 4x3 matrix,
// assuming a fourth component of the vector to be 0, i.e.
// the typical transformation of a 3d vector by a matrix.
// This multiplication order matches VEX.
static fpreal3
mat43vec3mul(const mat4 a, const fpreal3 b)
{
    fpreal4 result = b.x * a.lo.lo +
                    b.y * a.lo.hi +
                    b.z * a.hi.lo;
    return (fpreal3)(result.x, result.y, result.z);
}

// Multiply b * A where b is a 3-vector and A is a 4x4 matrix,
// assuming a fourth component of the vector to be 1, i.e.
// the typical transformation of a 3d point by a matrix.
// This multiplication order matches VEX.static fpreal3
static fpreal3
mat4vec3mul(const mat4 a, const fpreal3 b)
{
    fpreal4 result = b.x * a.lo.lo +
                    b.y * a.lo.hi +
                    b.z * a.hi.lo +
                    a.hi.hi;
    return (fpreal3)(result.x, result.y, result.z);
}

// Multiply b * A where b is a 3-vector and A is a 4x4 matrix.
// This multiplication order matches VEX.
static fpreal4
mat4vecmul(const mat4 a, const fpreal4 b)
{
    fpreal4 result = b.x * a.lo.lo +
                    b.y * a.lo.hi +
                    b.z * a.hi.lo +
                    b.w * a.hi.hi;
    return result;
}

// UT_Matrix4::solveColumn
// cp:          pivot_col and pivot_row
// c1, c2, c3:  other columns and rows
static void
mat4solvecol(fpreal (*matx)[4][4], int cp)
{
    fpreal      pivot_value_inverse;
    int         c1, c2, c3;

    switch (cp)
    {
        case 0: c1 = 1; c2 = 2; c3 = 3; break;
        case 1: c1 = 0; c2 = 2; c3 = 3; break;
        case 2: c1 = 0; c2 = 1; c3 = 3; break;
        case 3: c1 = 0; c2 = 1; c3 = 2; break;
    }

    // Here we will find the inverse of the pivot, set the pivot to 1,
    // and multiply the row which contains the pivot by the pivot
    // inverse. This might seem a little weird. The algorithm does it
    // this way so that it can gradually replace the input matrix
    // with its inverse.
    pivot_value_inverse = 1.0F/(*matx)[cp][cp];

    (*matx)[cp][cp] = pivot_value_inverse;
    (*matx)[cp][c1] *= pivot_value_inverse;
    (*matx)[cp][c2] *= pivot_value_inverse;
    (*matx)[cp][c3] *= pivot_value_inverse;

    // Now we subtract multiples of the pivot row from the other
    // rows in the matrix. This would be more familiar if the pivot
    // itself hadn't been set to 1 before the pivot row was multiplied
    // by the inverse (see above).

    (*matx)[c1][c1] -= (*matx)[cp][c1]*(*matx)[c1][cp];
    (*matx)[c1][c2] -= (*matx)[cp][c2]*(*matx)[c1][cp];
    (*matx)[c1][c3] -= (*matx)[cp][c3]*(*matx)[c1][cp];

    (*matx)[c2][c1] -= (*matx)[cp][c1]*(*matx)[c2][cp];
    (*matx)[c2][c2] -= (*matx)[cp][c2]*(*matx)[c2][cp];
    (*matx)[c2][c3] -= (*matx)[cp][c3]*(*matx)[c2][cp];

    (*matx)[c3][c1] -= (*matx)[cp][c1]*(*matx)[c3][cp];
    (*matx)[c3][c2] -= (*matx)[cp][c2]*(*matx)[c3][cp];
    (*matx)[c3][c3] -= (*matx)[cp][c3]*(*matx)[c3][cp];

    (*matx)[c1][cp] *= -pivot_value_inverse;
    (*matx)[c2][cp] *= -pivot_value_inverse;
    (*matx)[c3][cp] *= -pivot_value_inverse;
}

// UT_Matrix4::invert
// Linear equation solution by Gauss-Jordan elimination.
static int
mat4invert(fpreal16 *m)
{
    fpreal tol = 0.0;
    fpreal (*matx)[4][4] = (fpreal (*)[4][4]) m;
    
    int indexcol[4], indexrow[4];
    int pivot_row, pivot_col;

    // Check for the very common case of trivial column 3.
    const bool is_trivial_col3 =
        ((*matx)[0][3] == 0) &&
        ((*matx)[1][3] == 0) &&
        ((*matx)[2][3] == 0) &&
        ((*matx)[3][3] == 1);

    // We will need to keep track of which columns we have already found
    // pivots in. For this we use what is essentially a set of flags in the
    // array "pivoted_columns_flags". We initialize them to 0 here.
    bool pivoted_columns_flags[4] = {false, false, false, false};

    // In order to invert an n*n matrix, we will have to find n pivots, and
    // for each pivot reduce each row. We will keep track of which pivot and
    // set of reductions we are working on with "reduction"

    for (int reduction = 0; reduction < 4; reduction++)
    {
        fpreal pivot_value = 0;

        // This is the outer loop of the search for a pivot element
        // This loop finds the pivot element by choosing the element of the
        // matrix which has the largest absolute value of all elements of the
        // array not in columns/rows that contain a previously used pivot.
        for (int row = 0; row < 4; row++)
        {
            // if there hasn't already been a pivot in this row
            if ( !pivoted_columns_flags[row] )
            {
                // if there hasn't already been a pivot in column 0
                if ( !pivoted_columns_flags[0] )
                {
                    const fpreal abs_element_value = fabs((*matx)[row][0]);

                    if ( abs_element_value > pivot_value )
                    {
                        pivot_value = abs_element_value;
                        pivot_row = row;
                        pivot_col = 0;
                    }
                }

                // if there hasn't already been a pivot in column 1
                if ( !pivoted_columns_flags[1] )
                {
                    const fpreal abs_element_value = fabs((*matx)[row][1]);

                    if ( abs_element_value > pivot_value )
                    {
                        pivot_value = abs_element_value;
                        pivot_row = row;
                        pivot_col = 1;
                    }
                }

                // if there hasn't already been a pivot in column 2
                if ( !pivoted_columns_flags[2] )
                {
                    const fpreal abs_element_value = fabs((*matx)[row][2]);

                    if ( abs_element_value > pivot_value )
                    {
                        pivot_value = abs_element_value;
                        pivot_row = row;
                        pivot_col = 2;
                    }
                }

                // if there hasn't already been a pivot in column 3
                if ( !pivoted_columns_flags[3] )
                {
                    const fpreal abs_element_value = fabs((*matx)[row][3]);

                    if ( abs_element_value > pivot_value )
                    {
                        pivot_value = abs_element_value;
                        pivot_row = row;
                        pivot_col = 3;
                    }
                }
            }
        }

        // Odd check here if the matrix is filled with nan's or infinities
        // Verify that the pivot we found is not 0
        if (pivot_value <= tol)
        {
            return 1;
        }

        // Now we have found the pivot element for the current reduction.
        // This element of the matrix is the largest element of all elements
        // not in rows and columns previously used for other pivots.

        // Record that we have found a pivot in the current column
        pivoted_columns_flags[pivot_col] = true;
        indexrow[reduction] = pivot_row;
        indexcol[reduction] = pivot_col;

        // The pivot must be on the diagonal before we can use it. If the
        // pivot we found wasn't already on the diagonal, we swap rows to
        // put it there now.
        if ( pivot_row != pivot_col )
        {
            swapf(&(*matx)[pivot_row][0], &(*matx)[pivot_col][0]);
            swapf(&(*matx)[pivot_row][1], &(*matx)[pivot_col][1]);
            swapf(&(*matx)[pivot_row][2], &(*matx)[pivot_col][2]);
            swapf(&(*matx)[pivot_row][3], &(*matx)[pivot_col][3]);
        }


        // Note that from here on, the pivot is on the diagonal, and thus
        // has the same row index as column index. In particular, while
        // we may have swapped the row the pivot is in, we have not changed
        // the column. Thus, the pivot location will be referred to using
        // pivot_col only.

        switch (pivot_col)
        {
            case 0: mat4solvecol(matx, 0); break;
            case 1: mat4solvecol(matx, 1); break;
            case 2: mat4solvecol(matx, 2); break;
            case 3: mat4solvecol(matx, 3); break;
        }
    }


    // Finally, we "unscramble" the matrix, which was scrambled by
    // row swapping. This will produce the actual inverse of the matrix.
    for (int reduction = 3; reduction >= 0; reduction--)
    {
        int irow = indexrow[reduction];
        int icol = indexcol[reduction];
        if ( irow != icol )
        {
            swapf(&(*matx)[0][irow], &(*matx)[0][icol]);
            swapf(&(*matx)[1][irow], &(*matx)[1][icol]);
            swapf(&(*matx)[2][irow], &(*matx)[2][icol]);
            swapf(&(*matx)[3][irow], &(*matx)[3][icol]);
        }
    }

    if (is_trivial_col3)
    {
        // Force column 3 to be exact if input column 3 was exactly trivial.
        (*matx)[0][3] = 0;
        (*matx)[1][3] = 0;
        (*matx)[2][3] = 0;
        (*matx)[3][3] = 1;
    }

    return 0;
}


static fpreal 
mat2det(const mat2 m)
{
    return m.x * m.w - m.y * m.z;
}

static fpreal 
mat2inv(const mat2 m, mat2 *minvout)
{
    fpreal det = mat2det(m);
    if (det == 0)
        return 0;

    *minvout = (mat2)(
        m.w, -m.y,
        -m.z, m.x
    ) / det;
    return det;
}

static fpreal 
mat3inv(const mat3 m, mat3 minvout)
{
    fpreal det = det3(m);
    if (det == 0)
        return 0;
    
    // Inverse[r][c] = Det(of m excluding r,c) / Det(m)
    minvout[0] = (fpreal3)(
        mat2det((mat2)(m[1].yz, m[2].yz)),
        mat2det((mat2)(m[1].xz, m[2].xz)),
        mat2det((mat2)(m[1].xy, m[2].xy))
    ) / det;
    minvout[1] = (fpreal3)(
        mat2det((mat2)(m[0].yz, m[2].yz)),
        mat2det((mat2)(m[0].xz, m[2].xz)),
        mat2det((mat2)(m[0].xy, m[2].xy))
    ) / det;
    minvout[2] = (fpreal3)(
        mat2det((mat2)(m[0].yz, m[1].yz)),
        mat2det((mat2)(m[0].xz, m[1].xz)),
        mat2det((mat2)(m[0].xy, m[1].xy))
    ) / det;

    return det;
}

static void
mat3scale(mat3 mout, const mat3 m, fpreal scale)
{
    mout[0] = m[0] * scale;
    mout[1] = m[1] * scale;
    mout[2] = m[2] * scale;
}

static void
mat3lincomb2(mat3 mout, const mat3 m1, fpreal scale1, const mat3 m2, fpreal scale2)
{
    mout[0] = m1[0] * scale1 + m2[0] * scale2;
    mout[1] = m1[1] * scale1 + m2[1] * scale2;
    mout[2] = m1[2] * scale1 + m2[2] * scale2;
}

// Rotates the incoming positions with the given angle in degrees.
// It's more efficient to make a rotation matrix if you're going to rotate multiple matrices by the same angle
static fpreal2
rotate2D(fpreal2 pos, fpreal angle)
{
    angle = -angle * M_PI_F/180.0f;
    fpreal ca;
    fpreal sa = sincos(angle, &ca);
    mat2 rot = (mat2)(ca, sa, -sa, ca);
    return mat2vecmul(rot, pos);
}

#ifndef NO_DOUBLE_SUPPORT
// Double versions of subset of the above functions
// (mainly for shapematching ATM).

typedef double3 mat3d[3];

static void
mat3fromcolsd(const double3 c0, const double3 c1, const double3 c2, mat3d m)
{
    m[0] = (double3)(c0.s0, c1.s0, c2.s0);
    m[1] = (double3)(c0.s1, c1.s1, c2.s1);
    m[2] = (double3)(c0.s2, c1.s2, c2.s2);
}

static void
transpose3d(const mat3d a, mat3d b)
{
    mat3fromcolsd(a[0], a[1], a[2], b);
}

#endif

#endif
