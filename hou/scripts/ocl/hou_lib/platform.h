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
 * NAME:    platform.h ( CE Library, OpenCL)
 *
 * COMMENTS: OpenCL portability / platform macros.
 */

// Some newer drivers that support 3.0 (Intel) will define
// these flags but leave them blank if -cl-std=CL3.0 is not
// specified.  This is incorrect, as they should be undefined
// in that case.  So we un-define them on everything < 3.0,
// to be re-defined for NVIDIA 2.0 devices below.
#if (__OPENCL_C_VERSION__ < 300)
#undef __opencl_c_work_group_collective_functions
#undef __opencl_c_device_enqueue
#endif

// Currently the only GPU platform we support that uses
// these are NVIDIA, so we (re-)define the OpenCL 3.0
// macros so we have only one define to test in the code.
#if (__OPENCL_C_VERSION__ >= 200 && defined(__H_NVIDIA__))
#define __opencl_c_work_group_collective_functions 1
#define __opencl_c_device_enqueue 1
#endif
