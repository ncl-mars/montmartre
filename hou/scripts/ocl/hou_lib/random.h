#ifndef __RANDOM_H
#define __RANDOM_H

/******************************************************************************
 * HDK-consistent floor, integer hash, and fast random number generation.
 ******************************************************************************/

/// Returns the largest representable integer no greater than the given input.
static float
SYSfloorIL(float val)
{
    uint tmp = as_uint(val);
    uint shift = (tmp >> 23) & 0xff;

    if(shift < 0x7f)
    {
        return (tmp > 0x80000000) ? -1.0F : 0.0F;
    }
    else if(shift < 0x96)
    {
        uint mask = 0xffffffff << (0x96 - shift);
        if(tmp & 0x80000000)
        {
            if((tmp & ~mask) & 0x7fffff)
            {
                tmp &= mask;
                return as_float(tmp) - 1;
            }
            else
            {
                return val;
            }
        }
        else
        {
            return as_float(tmp & mask);
        }
    }
    else
    {
        return val;
    }
}

/// Consistent integer hash with the HDK.
static uint
SYSwang_inthash(uint key)
{
    key += ~(key << 16);
    key ^=  (key >>  5);
    key +=  (key <<  3);
    key ^=  (key >> 13);
    key += ~(key <<  9);
    key ^=  (key >> 17);
    return key;
}

/// Generates a uniform random number in the 0-1 range from the given seed and
/// updates the seed.
static float
SYSfastRandom(uint* seed)
{
    uint temp;
    *seed = (*seed) * 1664525 + 1013904223;
    temp = 0x3f800000 | (0x007fffff & (*seed));
    return as_float(temp) - 1.0f;
}

/******************************************************************************
 * Hashing macros for varying number of inputs.
 ******************************************************************************/

/// Conversion from a single float to an integer, for the clamped and unclamped
/// cases.
#define C_HASH1(x) ((int) SYSfloorIL(x))
#define U_HASH1(x) (as_uint(x))

/// Hash functions for 2-4 integers, used by clamped float hashing.
#define HASH2I(x, y) ((x^0xffffdead) * (y^0xffffc0de))
#define HASH3I(x, y, z) ((x^0xffff3ce3) * (y^0xffff7ba5) * (z^0xffffd169))
#define HASH4I(x, y, z, w) ((x^0xffff3ce3) * (y^0xffff7ba5) * (z^0xffffd169) \
                                           * (w^0xffff0397))
/// These macros declare hashing functions for clamped floating point numbers.
#define C_HASH2(x, y) HASH2I(C_HASH1(x), C_HASH1(y))
#define C_HASH3(x, y, z) HASH3I(C_HASH1(x), C_HASH1(y), C_HASH1(z))
#define C_HASH4(x, y, z, w) HASH4I(C_HASH1(x), C_HASH1(y), C_HASH1(z), \
                                   C_HASH1(w))

/// This macro hashes 2 integers, used by generic float hashing.
#define U_HASH2_RAW(x, y) y + SYSwang_inthash(x)
/// These macros declare hashing functions for generic floating point numbers.
#define U_HASH2(x, y) U_HASH2_RAW(U_HASH1(x), U_HASH1(y))
#define U_HASH3(x, y, z) U_HASH2_RAW(U_HASH2(x, y), U_HASH1(z))
#define U_HASH4(x, y, z, w) U_HASH2_RAW(U_HASH3(x, y, z), U_HASH1(w))

/******************************************************************************
 * Helper macros to hash inputs and generate outputs.
 ******************************************************************************/

/// Macros to hash the given number of floating point variables and store the
/// result in a new integer called hash.
#define C_1_HASH uint hash = SYSwang_inthash(C_HASH1(x));
#define C_2_HASH uint hash = SYSwang_inthash(C_HASH2(x, y));
#define C_3_HASH uint hash = SYSwang_inthash(C_HASH3(x, y, z));
#define C_4_HASH uint hash = SYSwang_inthash(C_HASH4(x, y, z, w));
#define U_1_HASH int hash = SYSwang_inthash(U_HASH1(x));
#define U_2_HASH int hash = SYSwang_inthash(U_HASH2(x, y));
#define U_3_HASH int hash = SYSwang_inthash(U_HASH3(x, y, z));
#define U_4_HASH int hash = SYSwang_inthash(U_HASH4(x, y, z, w));
/// Macros to return a vector of K random numbers; hash integer variable must be
/// declared and initialized.
#define RET_1_RAND return SYSfastRandom((uint*) &hash);
#define RET_2_RAND return (float2)(SYSfastRandom((uint*) &hash), \
                                   SYSfastRandom((uint*) &hash));
#define RET_3_RAND return (float3)(SYSfastRandom((uint*) &hash), \
                                   SYSfastRandom((uint*) &hash), \
                                   SYSfastRandom((uint*) &hash));
#define RET_4_RAND return (float4)(SYSfastRandom((uint*) &hash), \
                                   SYSfastRandom((uint*) &hash), \
                                   SYSfastRandom((uint*) &hash), \
                                   SYSfastRandom((uint*) &hash));

/******************************************************************************
 * Generation of final VEX_equivalent random_fhash() functions.
 ******************************************************************************/

static int VEXrandom_fhash_1(float x)
{
    U_1_HASH
    return hash;
}
static int VEXrandom_fhash_2(float x, float y)
{
    U_2_HASH
    return hash;
}
static int VEXrandom_fhash_3(float x, float y, float z)
{
    U_3_HASH
    return hash;
}
static int VEXrandom_fhash_4(float x, float y, float z, float w)
{
    U_4_HASH
    return hash;
}

/******************************************************************************
 * Generation of the final VEX-equivalent random() functions with float inputs.
 ******************************************************************************/

/// Macro that generates the code for random functions. NUM should be 2-4
/// (number of random numbers to return).
#define CREATE_RANDOM(NUM) \
static float ## NUM VEXrandom_1_ ## NUM(float x) \
{ \
    C_1_HASH \
    RET_ ## NUM ## _RAND \
} \
static float ## NUM VEXrandom_2_ ## NUM(float x, float y) \
{ \
    C_2_HASH \
    RET_ ## NUM ## _RAND \
} \
static float ## NUM VEXrandom_3_ ## NUM(float x, float y, float z) \
{ \
    C_3_HASH \
    RET_ ## NUM ## _RAND \
} \
static float ## NUM VEXrandom_4_ ## NUM(float x, float y, float z, float w) \
{ \
    C_4_HASH \
    RET_ ## NUM ## _RAND \
}
/// Macro that generates the code for returning a single floating point random.
#define CREATE_RANDOM_FLOAT \
static float VEXrandom_1_1(float x) \
{ \
    C_1_HASH \
    RET_1_RAND \
} \
static float VEXrandom_2_1(float x, float y) \
{ \
    C_2_HASH \
    RET_1_RAND \
} \
static float VEXrandom_3_1(float x, float y, float z) \
{ \
    C_3_HASH \
    RET_1_RAND \
} \
static float VEXrandom_4_1(float x, float y, float z, float w) \
{ \
    C_4_HASH \
    RET_1_RAND \
}

/// Create the functions.
CREATE_RANDOM_FLOAT
CREATE_RANDOM(2)
CREATE_RANDOM(3)
CREATE_RANDOM(4)

/******************************************************************************
 * Generation of the final VEX-equivalent rand() functions.
 ******************************************************************************/

/// Macro that generates the code for rand functions. NUM should be 2-4 (number
/// of random numbers to return).
#define CREATE_RAND(NUM) \
static float ## NUM VEXrand_1_ ## NUM(float x) \
{ \
    U_1_HASH \
    hash = (int) SYSwang_inthash(hash); \
    RET_ ## NUM ## _RAND \
} \
static float ## NUM VEXrand_2_ ## NUM(float x, float y) \
{ \
    U_2_HASH \
    hash = (int) SYSwang_inthash(hash); \
    RET_ ## NUM ## _RAND \
} \
static float ## NUM VEXrand_3_ ## NUM(float x, float y, float z) \
{ \
    U_3_HASH \
    hash = (int) SYSwang_inthash(hash); \
    RET_ ## NUM ## _RAND \
} \
static float ## NUM VEXrand_4_ ## NUM(float x, float y, float z, float w) \
{ \
    U_4_HASH \
    hash = (int) SYSwang_inthash(hash); \
    RET_ ## NUM ## _RAND \
}
/// Macro that generates the code for returning a single floating point rand.
#define CREATE_RAND_FLOAT \
static float VEXrand_1_1(float x) \
{ \
    U_1_HASH \
    hash = (int) SYSwang_inthash(hash); \
    RET_1_RAND \
} \
static float VEXrand_2_1(float x, float y) \
{ \
    U_2_HASH \
    hash = (int) SYSwang_inthash(hash); \
    RET_1_RAND \
} \
static float VEXrand_3_1(float x, float y, float z) \
{ \
    U_3_HASH \
    hash = (int) SYSwang_inthash(hash); \
    RET_1_RAND \
} \
static float VEXrand_4_1(float x, float y, float z, float w) \
{ \
    U_4_HASH \
    hash = (int) SYSwang_inthash(hash); \
    RET_1_RAND \
}

/// Create the functions.
CREATE_RAND_FLOAT
CREATE_RAND(2)
CREATE_RAND(3)
CREATE_RAND(4)

#endif

