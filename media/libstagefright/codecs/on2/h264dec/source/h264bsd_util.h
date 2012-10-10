/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*------------------------------------------------------------------------------

    Table of contents

    1. Include headers
    2. Module defines
    3. Data types
    4. Function prototypes

------------------------------------------------------------------------------*/

#ifndef H264SWDEC_UTIL_H
#define H264SWDEC_UTIL_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#ifdef _ASSERT_USED
#include <assert.h>
#endif

#include "H264SwDecApi.h"

#if defined(_RANGE_CHECK) || defined(_DEBUG_PRINT) || defined(_ERROR_PRINT)
#include <stdio.h>
#endif

#include "basetype.h"
#if MIPS_DSP_R2_LE
#include "./mips/h264bsd_util_mips.h"
#endif /* #if MIPS_DSP_R2_LE */
#include "h264bsd_stream.h"
#include "h264bsd_image.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

#define HANTRO_OK   0
#define HANTRO_NOK  1

#define HANTRO_TRUE     (1)
#define HANTRO_FALSE    (0)

#ifndef NULL
#define NULL 0
#endif

#define MEMORY_ALLOCATION_ERROR 0xFFFF
#define PARAM_SET_ERROR 0xFFF0

/* value to be returned by GetBits if stream buffer is empty */
#define END_OF_STREAM 0xFFFFFFFFU

#if MIPS_DSP_R2_LE
/*-----------------------------------------------------------------------------
    EMPTY_RESIDUAL_INDICATOR constant changed because of changing type of values
    that are compared with EMPTY_RESIDUAL_INDICATOR (buffer level from residual_t
    struct type changed from int to short).
------------------------------------------------------------------------------*/

#define EMPTY_RESIDUAL_INDICATOR 0x7fff
#else
#define EMPTY_RESIDUAL_INDICATOR 0xFFFFFF
#endif /* #if MIPS_DSP_R2_LE */

/* macro to mark a residual block empty, i.e. contain zero coefficients */
#define MARK_RESIDUAL_EMPTY(residual) ((residual)[0] = EMPTY_RESIDUAL_INDICATOR)
/* macro to check if residual block is empty */
#define IS_RESIDUAL_EMPTY(residual) ((residual)[0] == EMPTY_RESIDUAL_INDICATOR)

/* macro for assertion, used only if compiler flag _ASSERT_USED is defined */
#ifdef _ASSERT_USED
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr)
#endif

/* macro for range checking an value, used only if compiler flag _RANGE_CHECK
 * is defined */
#ifdef _RANGE_CHECK
#define RANGE_CHECK(value, minBound, maxBound) \
{ \
    if ((value) < (minBound) || (value) > (maxBound)) \
        fprintf(stderr, "Warning: Value exceeds given limit(s)!\n"); \
}
#else
#define RANGE_CHECK(value, minBound, maxBound)
#endif

/* macro for range checking an array, used only if compiler flag _RANGE_CHECK
 * is defined */
#ifdef _RANGE_CHECK
#define RANGE_CHECK_ARRAY(array, minBound, maxBound, length) \
{ \
    i32 i; \
    for (i = 0; i < (length); i++) \
        if ((array)[i] < (minBound) || (array)[i] > (maxBound)) \
            fprintf(stderr,"Warning: Value [%d] exceeds given limit(s)!\n",i); \
}
#else
#define RANGE_CHECK_ARRAY(array, minBound, maxBound, length)
#endif

/* macro for debug printing, used only if compiler flag _DEBUG_PRINT is
 * defined */
#ifdef _DEBUG_PRINT
#define DEBUG(args) printf args
#else
#define DEBUG(args)
#endif

/* macro for error printing, used only if compiler flag _ERROR_PRINT is
 * defined */
#ifdef _ERROR_PRINT
#define EPRINT(msg) fprintf(stderr,"ERROR: %s\n",msg)
#else
#define EPRINT(msg)
#endif

#if MIPS_DSP_R2_LE
static __inline u32 MIN(int x, int y)
{
   u32 r;
   r = y ^ ((x ^ y) & -(x < y));
   return (r);
}

static __inline u32 MAX(int x, int y)
{
   u32 r;
   r = x ^ ((x ^ y) & -(x < y));
   return (r);
}

static __inline int CLIP3(int a, int b, int x)
{
   register int x1, x2;
   x1 = ABS(x-a);
   x2 = ABS(x-b);
   x = x1 + (a+b);
   x -= x2;
   x >>= 1;
   return (x);
}

static __inline int CLIP1(int x)
{
   register int x1, x2;
   x1 = ABS(x);
   x2 = ABS(x - 255);
   x = x1 + 255;
   x -= x2;
   x >>= 1;
   return (x);
}
#else
/* macro to get smaller of two values */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/* macro to get greater of two values */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/* macro to get absolute value */
#define ABS(a) (((a) < 0) ? -(a) : (a))

/* macro to clip a value z, so that x <= z =< y */
#define CLIP3(x,y,z) (((z) < (x)) ? (x) : (((z) > (y)) ? (y) : (z)))

/* macro to clip a value z, so that 0 <= z =< 255 */
#define CLIP1(z) (((z) < 0) ? 0 : (((z) > 255) ? 255 : (z)))
#endif /* #if MIPS_DSP_R2_LE */

/* macro to allocate memory */
#define ALLOCATE(ptr, count, type) \
{ \
    (ptr) = H264SwDecMalloc((count) * sizeof(type)); \
}

/* macro to free allocated memory */
#define FREE(ptr) \
{ \
    H264SwDecFree((ptr)); (ptr) = NULL; \
}

#define ALIGN(ptr, bytePos) \
        (ptr + ( ((bytePos - (int)ptr) & (bytePos - 1)) / sizeof(*ptr) ))

extern const u32 h264bsdQpC[52];

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
#ifndef H264DEC_NEON
u32 h264bsdCountLeadingZeros(u32 value, u32 length);
#else
u32 h264bsdCountLeadingZeros(u32 value);
#endif
u32 h264bsdRbspTrailingBits(strmData_t *strmData);

u32 h264bsdMoreRbspData(strmData_t *strmData);

u32 h264bsdNextMbAddress(u32 *pSliceGroupMap, u32 picSizeInMbs, u32 currMbAddr);

void h264bsdSetCurrImageMbPointers(image_t *image, u32 mbNum);

#endif /* #ifdef H264SWDEC_UTIL_H */

