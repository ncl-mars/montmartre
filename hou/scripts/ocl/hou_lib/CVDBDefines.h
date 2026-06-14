#ifndef __CVDBDefines_H__
#define __CVDBDefines_H__

// Define common platform specific work arounds and other
// feature flags.
#if (defined(__H_AMD__) || defined(__H_APPLE__)) && defined(__H_GPU__)
// The AMD OpenCL compiler has an optimizer bug with
// bit shifts and unsigned longs used in the VDB includes,
// so define this to workaround it.
// Most Apple GPUs are AMD so we set for Apple GPUs as well.
#define AVOID_64BIT_SHIFT
#endif

#endif

