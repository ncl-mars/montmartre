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
 * NAME:    util.h ( CE Library, OpenCL)
 *
 * COMMENTS:
 */

#ifndef __UTIL_H__
#define __UTIL_H__

static void swapf(fpreal *a, fpreal *b)
{
    fpreal t = *a;
    *a = *b;
    *b = t;
}

#endif
