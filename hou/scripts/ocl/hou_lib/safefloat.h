// This header file contains the OpenCL port of safe integer-float conversion
// from UT_SafeFloat, where you should look for explanations of what's
// happening.

#ifndef __SAFE_FLOAT_H__
#define __SAFE_FLOAT_H__

__constant const int MANTISSA_BITS = 23;
__constant const int EXPONENT_BITS = 8;
__constant const int ZERO_EXP = (1 << (EXPONENT_BITS - 1)) - 1;
__constant const int INFINITE_EXP = (1 << EXPONENT_BITS) - 1;
__constant const int START_EXP = ZERO_EXP + (MANTISSA_BITS + 1);
__constant const int EXP_LENGTH = INFINITE_EXP - START_EXP;
__constant const int LENGTH = EXP_LENGTH * (1 << MANTISSA_BITS);
__constant const int START_ENCODED = (START_EXP << MANTISSA_BITS);
__constant const int ONE_ENCODED = (ZERO_EXP << MANTISSA_BITS);

static float
packIntToSafeFloat(int input)
{
    // Absolute value of -2^31 is -2^31 < 0 in C++, so handle that explicitly
    // too. (abs(-2^31) = -2^31 in OpenCL too, but abs(-2^31) < 0 evaluates to
    // false...)
    if (abs(input) <= (1 << (MANTISSA_BITS + 1)) || input == -2147483648)
        return (float) input;

    uint output_uint = 0;
    if (input < 0)
    {
        output_uint |= (1 << (MANTISSA_BITS + EXPONENT_BITS));
        input = -input;
    }
    input -= (1 << (MANTISSA_BITS + 1));

    if (input < LENGTH)
    {
        output_uint |= (START_ENCODED + input);
        return as_float(output_uint);
    }

    input -= LENGTH;
    input += (1 << MANTISSA_BITS);

    if (input >= ONE_ENCODED)
        input = ONE_ENCODED - 1;

    output_uint |= input;
    return as_float(output_uint);
}

static int
unpackIntFromSafeFloat(float input)
{
    if (input == 0)
        return 0;

    uint output_uint = as_uint(input);

    bool negative = (output_uint >> (MANTISSA_BITS + EXPONENT_BITS));
    output_uint &= ~(1 << (MANTISSA_BITS + EXPONENT_BITS));

    if (output_uint >= ONE_ENCODED && output_uint <= START_ENCODED)
        return (int) input;

    if (output_uint > START_ENCODED)
    {
        output_uint -= START_ENCODED;
        if (output_uint >= LENGTH)
            output_uint = LENGTH - 1;
        output_uint += (1 << (MANTISSA_BITS + 1));
        int output = as_int(output_uint);
        return negative ? -output : output;
    }

    if (output_uint < (1 << MANTISSA_BITS))
        return 0;
    if (output_uint >= ONE_ENCODED)
        output_uint = ONE_ENCODED - 1;

    output_uint += LENGTH + ((1 << (MANTISSA_BITS + 1)) - (1 << MANTISSA_BITS));
    int output = as_int(output_uint);
    if (negative)
        output = -output;

    return output;
}

#endif

