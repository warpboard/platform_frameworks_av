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
     2. External compiler flags
     3. Module defines
     4. Local function prototypes
     5. Functions
          h264bsdProcessBlock
          h264bsdProcessChromaDc

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "../h264bsd_transform.h"
#include "../h264bsd_util.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/


/* LevelScale function */
/*-----------------------------------------------------------------------------
    levelScale type changed from i32 to i16 . This change allows using single
    load instruction for loading of two levelScale values.
 -----------------------------------------------------------------------------*/
static const i16 levelScale[6][3] = {
    {10,13,16}, {11,14,18}, {13,16,20}, {14,18,23}, {16,20,25}, {18,23,29}};
/* qp % 6 as a function of qp */
static const u8 qpMod6[52] = {0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,
    0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3};

/* qp / 6 as a function of qp */
static const u8 qpDiv6[52] = {0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,3,3,
    4,4,4,4,4,4,5,5,5,5,5,5,6,6,6,6,6,6,7,7,7,7,7,7,8,8,8,8};

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Function: h264bsdProcessBlock

        Functional description:
            Function performs inverse zig-zag scan, inverse scaling and
            inverse transform for a luma or a chroma residual block

        Inputs:
            data            pointer to data to be processed
            qp              quantization parameter
            skip            skip processing of data[0], set to non-zero value
                            if dc coeff hanled separately
            coeffMap        16 lsb's indicate which coeffs are non-zero,
                            bit 0 (lsb) for coeff 0, bit 1 for coeff 1 etc.

        Outputs:
            data            processed data

        Returns:
            HANTRO_OK       success
            HANTRO_NOK      processed data not in valid range [-512, 511]

------------------------------------------------------------------------------*/
u32 h264bsdProcessBlock(i16 *data, u32 qp, u32 skip, u32 coeffMap)
{

/* Variables */

    i16 tmp0, tmp1, tmp2, tmp3;
    i32 d1, d2, d3, d4, d5, d6, d7, d8;
    u16 qpDiv;

/* Code */

    qpDiv = qpDiv6[qp];
    tmp1 = levelScale[qpMod6[qp]][0] << qpDiv;
    tmp2 = levelScale[qpMod6[qp]][1] << qpDiv;
    tmp3 = levelScale[qpMod6[qp]][2] << qpDiv;

    if (!skip)
        data[0] = (data[0] * tmp1);

    /* at least one of the rows 1, 2 or 3 contain non-zero coeffs, mask takes
     * the scanning order into account */
    if (coeffMap & 0xFF9C)
    {
        /* do the zig-zag scan and inverse quantization */

        __asm__ volatile (
            "lh     %[d1],  2(%[data])                           \n\t"
            "lh     %[d2],  28(%[data])                          \n\t"
            "lh     %[d3],  30(%[data])                          \n\t"
            "lh     %[d4],  4(%[data])                           \n\t"
            "lh     %[d5],  10(%[data])                          \n\t"
            "lh     %[d6],  8(%[data])                           \n\t"
            "lh     %[d7],  16(%[data])                          \n\t"
            "lh     %[d8],  6(%[data])                           \n\t"
            "mul    %[d1],  %[d1],        %[tmp2]                \n\t"
            "mul    %[d2],  %[d2],        %[tmp2]                \n\t"
            "mul    %[d3],  %[d3],        %[tmp3]                \n\t"
            "mul    %[d4],  %[d4],        %[tmp2]                \n\t"
            "mul    %[d5],  %[d5],        %[tmp1]                \n\t"
            "mul    %[d6],  %[d6],        %[tmp3]                \n\t"
            "mul    %[tmp0],%[d7],        %[tmp2]                \n\t"
            "mul    %[d8],  %[d8],        %[tmp1]                \n\t"
            "sh     %[d1],  2(%[data])                           \n\t"
            "sh     %[d2],  28(%[data])                          \n\t"
            "sh     %[d3],  30(%[data])                          \n\t"
            "sh     %[d4],  8(%[data])                           \n\t"
            "sh     %[d5],  4(%[data])                           \n\t"
            "sh     %[d6],  10(%[data])                          \n\t"
            "sh     %[d8],  16(%[data])                          \n\t"
            "lh     %[d1],  12(%[data])                          \n\t"
            "lh     %[d2],  14(%[data])                          \n\t"
            "lh     %[d3],  24(%[data])                          \n\t"
            "lh     %[d4],  18(%[data])                          \n\t"
            "lh     %[d5],  20(%[data])                          \n\t"
            "lh     %[d6],  22(%[data])                          \n\t"
            "lh     %[d7],  26(%[data])                          \n\t"
            "mul    %[d1],  %[d1],        %[tmp2]                \n\t"
            "mul    %[d2],  %[d2],        %[tmp2]                \n\t"
            "mul    %[d3],  %[d3],        %[tmp3]                \n\t"
            "mul    %[d4],  %[d4],        %[tmp2]                \n\t"
            "mul    %[d5],  %[d5],        %[tmp3]                \n\t"
            "mul    %[d6],  %[d6],        %[tmp1]                \n\t"
            "mul    %[d7],  %[d7],        %[tmp2]                \n\t"
            "sh     %[d1],  6(%[data])                           \n\t"
            "sh     %[d2],  12(%[data])                          \n\t"
            "sh     %[d3],  14(%[data])                          \n\t"
            "sh     %[d4],  24(%[data])                          \n\t"
            "sh     %[tmp0],18(%[data])                          \n\t"
            "sh     %[d5],  26(%[data])                          \n\t"
            "sh     %[d6],  20(%[data])                          \n\t"
            "sh     %[d7],  22(%[data])                          \n\t"

            : [d1] "=&r" (d1), [d2] "=&r" (d2), [d3] "=&r" (d3),
              [d4] "=&r" (d4), [d5] "=&r" (d5), [d6] "=&r" (d6),
              [d7] "=&r" (d7), [d8] "=&r" (d8), [tmp0] "=&r" (tmp0)
            : [tmp1] "r" (tmp1), [tmp2] "r" (tmp2), [tmp3] "r" (tmp3),
              [data] "r" (data)
            : "hi", "lo", "memory"
        );

        /* horizontal transform */

        __asm__ volatile (
            "lh           %[d3],    2(%[data])              \n\t"
            "lh           %[d4],    6(%[data])              \n\t"
            "lh           %[d1],    0(%[data])              \n\t"
            "lh           %[d2],    4(%[data])              \n\t"
            "sra          %[d5],    %[d3],      1           \n\t"
            "sra          %[d6],    %[d4],      1           \n\t"
            "addu         %[tmp0],  %[d1],      %[d2]       \n\t"
            "subu         %[tmp1],  %[d1],      %[d2]       \n\t"
            "subu         %[tmp2],  %[d5],      %[d4]       \n\t"
            "addu         %[tmp3],  %[d3],      %[d6]       \n\t"
            "addu         %[d3],    %[tmp0],    %[tmp3]     \n\t"
            "addu         %[d4],    %[tmp1],    %[tmp2]     \n\t"
            "subu         %[d1],    %[tmp1],    %[tmp2]     \n\t"
            "subu         %[d2],    %[tmp0],    %[tmp3]     \n\t"
            "sh           %[d3],    0(%[data])              \n\t"
            "sh           %[d4],    2(%[data])              \n\t"
            "sh           %[d1],    4(%[data])              \n\t"
            "sh           %[d2],    6(%[data])              \n\t"
            "lh           %[d3],    10(%[data])             \n\t"
            "lh           %[d4],    14(%[data])             \n\t"
            "lh           %[d1],    8(%[data])              \n\t"
            "lh           %[d2],    12(%[data])             \n\t"
            "sra          %[d5],    %[d3],      1           \n\t"
            "sra          %[d6],    %[d4],      1           \n\t"
            "addu         %[tmp0],  %[d1],      %[d2]       \n\t"
            "subu         %[tmp1],  %[d1],      %[d2]       \n\t"
            "subu         %[tmp2],  %[d5],      %[d4]       \n\t"
            "addu         %[tmp3],  %[d3],      %[d6]       \n\t"
            "addu         %[d3],    %[tmp0],    %[tmp3]     \n\t"
            "addu         %[d4],    %[tmp1],    %[tmp2]     \n\t"
            "subu         %[d1],    %[tmp1],    %[tmp2]     \n\t"
            "subu         %[d2],    %[tmp0],    %[tmp3]     \n\t"
            "sh           %[d3],    8(%[data])              \n\t"
            "sh           %[d4],    10(%[data])             \n\t"
            "sh           %[d1],    12(%[data])             \n\t"
            "sh           %[d2],    14(%[data])             \n\t"
            "lh           %[d3],    18(%[data])             \n\t"
            "lh           %[d4],    22(%[data])             \n\t"
            "lh           %[d1],    16(%[data])             \n\t"
            "lh           %[d2],    20(%[data])             \n\t"
            "sra          %[d5],    %[d3],      1           \n\t"
            "sra          %[d6],    %[d4],      1           \n\t"
            "addu         %[tmp0],  %[d1],      %[d2]       \n\t"
            "subu         %[tmp1],  %[d1],      %[d2]       \n\t"
            "subu         %[tmp2],  %[d5],      %[d4]       \n\t"
            "addu         %[tmp3],  %[d3],      %[d6]       \n\t"
            "addu         %[d3],    %[tmp0],    %[tmp3]     \n\t"
            "addu         %[d4],    %[tmp1],    %[tmp2]     \n\t"
            "subu         %[d1],    %[tmp1],    %[tmp2]     \n\t"
            "subu         %[d2],    %[tmp0],    %[tmp3]     \n\t"
            "sh           %[d3],    16(%[data])             \n\t"
            "sh           %[d4],    18(%[data])             \n\t"
            "sh           %[d1],    20(%[data])             \n\t"
            "sh           %[d2],    22(%[data])             \n\t"
            "lh           %[d3],    26(%[data])             \n\t"
            "lh           %[d4],    30(%[data])             \n\t"
            "lh           %[d1],    24(%[data])             \n\t"
            "lh           %[d2],    28(%[data])             \n\t"
            "sra          %[d5],    %[d3],      1           \n\t"
            "sra          %[d6],    %[d4],      1           \n\t"
            "addu         %[tmp0],  %[d1],      %[d2]       \n\t"
            "subu         %[tmp1],  %[d1],      %[d2]       \n\t"
            "subu         %[tmp2],  %[d5],      %[d4]       \n\t"
            "addu         %[tmp3],  %[d3],      %[d6]       \n\t"
            "addu         %[d3],    %[tmp0],    %[tmp3]     \n\t"
            "addu         %[d4],    %[tmp1],    %[tmp2]     \n\t"
            "subu         %[d1],    %[tmp1],    %[tmp2]     \n\t"
            "subu         %[d2],    %[tmp0],    %[tmp3]     \n\t"
            "sh           %[d3],    24(%[data])             \n\t"
            "sh           %[d4],    26(%[data])             \n\t"
            "sh           %[d1],    28(%[data])             \n\t"
            "sh           %[d2],    30(%[data])             \n\t"

            : [d1] "=&r" (d1), [d2] "=&r" (d2), [d3] "=&r" (d3),
              [d4] "=&r" (d4), [d5] "=&r" (d5), [d6] "=&r" (d6),
              [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2),
              [tmp3] "=&r" (tmp3), [tmp0] "=&r" (tmp0)
            : [data] "r" (data)
            : "memory"
         );

        /* then vertical transform */
         __asm__ volatile (
            "lw         %[d3],    8(%[data])                      \n\t"
            "lw         %[d4],    24(%[data])                     \n\t"
            "lw         %[d1],    0(%[data])                      \n\t"
            "lw         %[d2],    16(%[data])                     \n\t"
            "shra.ph    %[d5],    %[d3],       1                  \n\t"
            "shra.ph    %[d6],    %[d4],       1                  \n\t"
            "addq.ph    %[tmp0],  %[d1],       %[d2]              \n\t"
            "addq.ph    %[tmp3],  %[d3],       %[d6]              \n\t"
            "subq.ph    %[tmp1],  %[d1],       %[d2]              \n\t"
            "subq.ph    %[tmp2],  %[d5],       %[d4]              \n\t"
            "addq.ph    %[d3],    %[tmp0],     %[tmp3]            \n\t"
            "addq.ph    %[d4],    %[tmp1],     %[tmp2]            \n\t"
            "subq.ph    %[d1],    %[tmp1],     %[tmp2]            \n\t"
            "subq.ph    %[d2],    %[tmp0],     %[tmp3]            \n\t"
            "shra_r.ph  %[d3],    %[d3],       6                  \n\t"
            "shra_r.ph  %[d4],    %[d4],       6                  \n\t"
            "shra_r.ph  %[d1],    %[d1],       6                  \n\t"
            "shra_r.ph  %[d2],    %[d2],       6                  \n\t"
            "sw         %[d3],    0(%[data])                      \n\t"
            "sw         %[d4],    8(%[data])                      \n\t"
            "sw         %[d1],    16(%[data])                     \n\t"
            "sw         %[d2],    24(%[data])                     \n\t"
            "lw         %[d3],    12(%[data])                     \n\t"
            "lw         %[d4],    28(%[data])                     \n\t"
            "lw         %[d1],    4(%[data])                      \n\t"
            "lw         %[d2],    20(%[data])                     \n\t"
            "shra.ph    %[d5],    %[d3],       1                  \n\t"
            "shra.ph    %[d6],    %[d4],       1                  \n\t"
            "addq.ph    %[tmp0],  %[d1],       %[d2]              \n\t"
            "addq.ph    %[tmp3],  %[d3],       %[d6]              \n\t"
            "subq.ph    %[tmp1],  %[d1],       %[d2]              \n\t"
            "subq.ph    %[tmp2],  %[d5],       %[d4]              \n\t"
            "addq.ph    %[d3],    %[tmp0],     %[tmp3]            \n\t"
            "addq.ph    %[d4],    %[tmp1],     %[tmp2]            \n\t"
            "subq.ph    %[d1],    %[tmp1],     %[tmp2]            \n\t"
            "subq.ph    %[d2],    %[tmp0],     %[tmp3]            \n\t"
            "shra_r.ph  %[d3],    %[d3],       6                  \n\t"
            "shra_r.ph  %[d4],    %[d4],       6                  \n\t"
            "shra_r.ph  %[d1],    %[d1],       6                  \n\t"
            "shra_r.ph  %[d2],    %[d2],       6                  \n\t"
            "sw         %[d3],    4(%[data])                      \n\t"
            "sw         %[d4],    12(%[data])                     \n\t"
            "sw         %[d1],    20(%[data])                     \n\t"
            "sw         %[d2],    28(%[data])                     \n\t"

            : [d1] "=&r" (d1), [d2] "=&r" (d2), [d3] "=&r" (d3),
              [d4] "=&r" (d4), [d5] "=&r" (d5), [d6] "=&r" (d6),
              [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2),
              [tmp3] "=&r" (tmp3), [tmp0] "=&r" (tmp0)
            : [data] "r" (data)
            : "memory"
         );
    }
    else /* rows 1, 2 and 3 are zero */
    {
        /* only dc-coeff is non-zero, i.e. coeffs at original positions
         * 1, 5 and 6 are zero */
        if ((coeffMap & 0x62) == 0)
        {
            __asm__ volatile (
                "lh         %[d1],  0(%[data])                   \n\t"
                "replv.ph   %[d1],  %[d1]                        \n\t"
                "shra_r.ph  %[d1],  %[d1],      6                \n\t"
                "sw         %[d1],  0(%[data])                   \n\t"
                "sw         %[d1],  4(%[data])                   \n\t"
                "sw         %[d1],  8(%[data])                   \n\t"
                "sw         %[d1],  12(%[data])                  \n\t"
                "sw         %[d1],  16(%[data])                  \n\t"
                "sw         %[d1],  20(%[data])                  \n\t"
                "sw         %[d1],  24(%[data])                  \n\t"
                "sw         %[d1],  28(%[data])                  \n\t"

                : [d1] "=&r" (d1)
                : [data] "r" (data)
                : "memory"
             );
        }
        else /* at least one of the coeffs 1, 5 or 6 is non-zero */
        {
            __asm__ volatile (
                "lh         %[d1],   2(%[data])                 \n\t"
                "lh         %[d2],   10(%[data])                \n\t"
                "lh         %[d3],   12(%[data])                \n\t"
                "mul        %[d1],   %[d1],        %[tmp2]      \n\t"
                "mul        %[d2],   %[d2],        %[tmp1]      \n\t"
                "mul        %[d3],   %[d3],        %[tmp2]      \n\t"
                "lh         %[d4],   0(%[data])                 \n\t"
                "sra        %[d5],   %[d1],        1            \n\t"
                "sra        %[d6],   %[d3],        1            \n\t"
                "addu       %[tmp0], %[d4],        %[d2]        \n\t"
                "subu       %[tmp1], %[d4],        %[d2]        \n\t"
                "subu       %[tmp2], %[d5],        %[d3]        \n\t"
                "addu       %[tmp3], %[d1],        %[d6]        \n\t"
                "addu       %[d1],   %[tmp0],      %[tmp3]      \n\t"
                "addu       %[d2],   %[tmp1],      %[tmp2]      \n\t"
                "subu       %[d3],   %[tmp1],      %[tmp2]      \n\t"
                "subu       %[d4],   %[tmp0],      %[tmp3]      \n\t"
                "sll        %[d1],   %[d1],        16           \n\t"
                "sll        %[d3],   %[d3],        16           \n\t"
                "packrl.ph  %[d1],   %[d2],        %[d1]        \n\t"
                "packrl.ph  %[d3],   %[d4],        %[d3]        \n\t"
                "shra_r.ph  %[d1],   %[d1],        6            \n\t"
                "shra_r.ph  %[d3],   %[d3],        6            \n\t"
                "sw         %[d1],   0(%[data])                 \n\t"
                "sw         %[d3],   4(%[data])                 \n\t"
                "sw         %[d1],   8(%[data])                 \n\t"
                "sw         %[d3],   12(%[data])                \n\t"
                "sw         %[d1],   16(%[data])                \n\t"
                "sw         %[d3],   20(%[data])                \n\t"
                "sw         %[d1],   24(%[data])                \n\t"
                "sw         %[d3],   28(%[data])                \n\t"

                : [d1] "=&r" (d1), [d2] "=&r" (d2), [d3] "=&r" (d3),
                  [d4] "=&r" (d4), [d5] "=&r" (d5), [d6] "=&r" (d6),
                  [tmp1] "+r" (tmp1), [tmp2] "+r" (tmp2),
                  [tmp3] "=&r" (tmp3), [tmp0] "=&r" (tmp0)
                : [data] "r" (data)
                : "hi", "lo", "memory"
             );
        }
    }

    return(HANTRO_OK);

}


/*------------------------------------------------------------------------------

    Function: h264bsdProcessChromaDc

        Functional description:
            Function performs inverse transform and inverse scaling for a
            chroma DC coefficients block

        Inputs:
            data            pointer to data to be processed
            qp              quantization parameter

        Outputs:
            data            processed data

        Returns:
            none

------------------------------------------------------------------------------*/
void h264bsdProcessChromaDc(i16 *data, u32 qp)
{

/* Variables */

    i32 tmp0, tmp1, tmp2, tmp3;
    i32 tmp00, tmp11, tmp22, tmp33;
    u32 qpDiv;
    i32 levScale;
    i32 d1, d2, d3, d4, d5, d6, d7, d8;

/* Code */

    qpDiv = qpDiv6[qp];
    levScale = levelScale[ qpMod6[qp] ][0];

    if (qp >= 6)
    {
        levScale <<= (qpDiv-1);
        __asm__ volatile (
            "lh          %[d1],    0(%[data])              \n\t"
            "lh          %[d2],    2(%[data])              \n\t"
            "lh          %[d3],    4(%[data])              \n\t"
            "lh          %[d4],    6(%[data])              \n\t"
            "lh          %[d5],    8(%[data])              \n\t"
            "lh          %[d6],    10(%[data])             \n\t"
            "lh          %[d7],    12(%[data])             \n\t"
            "lh          %[d8],    14(%[data])             \n\t"
            "addu        %[tmp0],  %[d1],     %[d3]        \n\t"
            "subu        %[tmp1],  %[d1],     %[d3]        \n\t"
            "subu        %[tmp2],  %[d2],     %[d4]        \n\t"
            "addu        %[tmp3],  %[d2],     %[d4]        \n\t"
            "addu        %[tmp00], %[d5],     %[d7]        \n\t"
            "subu        %[tmp11], %[d5],     %[d7]        \n\t"
            "subu        %[tmp22], %[d6],     %[d8]        \n\t"
            "addu        %[tmp33], %[d6],     %[d8]        \n\t"
            "addu        %[d1],    %[tmp0],   %[tmp3]      \n\t"
            "subu        %[d2],    %[tmp0],   %[tmp3]      \n\t"
            "addu        %[d3],    %[tmp1],   %[tmp2]      \n\t"
            "subu        %[d4],    %[tmp1],   %[tmp2]      \n\t"
            "addu        %[d5],    %[tmp00],  %[tmp33]     \n\t"
            "subu        %[d6],    %[tmp00],  %[tmp33]     \n\t"
            "addu        %[d7],    %[tmp11],  %[tmp22]     \n\t"
            "subu        %[d8],    %[tmp11],  %[tmp22]     \n\t"
            "mul         %[d1],    %[d1],     %[levScale]  \n\t"
            "mul         %[d2],    %[d2],     %[levScale]  \n\t"
            "mul         %[d3],    %[d3],     %[levScale]  \n\t"
            "mul         %[d4],    %[d4],     %[levScale]  \n\t"
            "mul         %[d5],    %[d5],     %[levScale]  \n\t"
            "mul         %[d6],    %[d6],     %[levScale]  \n\t"
            "mul         %[d7],    %[d7],     %[levScale]  \n\t"
            "mul         %[d8],    %[d8],     %[levScale]  \n\t"
            "sll         %[d1],    %[d1],     16           \n\t"
            "sll         %[d3],    %[d3],     16           \n\t"
            "sll         %[d5],    %[d5],     16           \n\t"
            "sll         %[d7],    %[d7],     16           \n\t"
            "packrl.ph   %[d1],    %[d2],     %[d1]        \n\t"
            "packrl.ph   %[d3],    %[d4],     %[d3]        \n\t"
            "packrl.ph   %[d5],    %[d6],     %[d5]        \n\t"
            "packrl.ph   %[d7],    %[d8],     %[d7]        \n\t"
            "sw          %[d1],    0(%[data])              \n\t"
            "sw          %[d3],    4(%[data])              \n\t"
            "sw          %[d5],    8(%[data])              \n\t"
            "sw          %[d7],    12(%[data])             \n\t"

            : [d1] "=&r" (d1), [d2] "=&r" (d2), [d3] "=&r" (d3),
              [d4] "=&r" (d4), [d5] "=&r" (d5), [d6] "=&r" (d6),
              [d7] "=&r" (d7), [d8] "=&r" (d8),
              [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2),
              [tmp3] "=&r" (tmp3), [tmp0] "=&r" (tmp0),
              [tmp11] "=&r" (tmp11), [tmp22] "=&r" (tmp22),
              [tmp33] "=&r" (tmp33), [tmp00] "=&r" (tmp00)
            : [data] "r" (data), [levScale] "r" (levScale)
            : "hi", "lo", "memory"
        );
    }
    else
    {
        __asm__ volatile (
            "lh          %[d1],    0(%[data])              \n\t"
            "lh          %[d2],    2(%[data])              \n\t"
            "lh          %[d3],    4(%[data])              \n\t"
            "lh          %[d4],    6(%[data])              \n\t"
            "lh          %[d5],    8(%[data])              \n\t"
            "lh          %[d6],    10(%[data])             \n\t"
            "lh          %[d7],    12(%[data])             \n\t"
            "lh          %[d8],    14(%[data])             \n\t"
            "addu        %[tmp0],  %[d1],     %[d3]        \n\t"
            "subu        %[tmp1],  %[d1],     %[d3]        \n\t"
            "subu        %[tmp2],  %[d2],     %[d4]        \n\t"
            "addu        %[tmp3],  %[d2],     %[d4]        \n\t"
            "addu        %[tmp00], %[d5],     %[d7]        \n\t"
            "subu        %[tmp11], %[d5],     %[d7]        \n\t"
            "subu        %[tmp22], %[d6],     %[d8]        \n\t"
            "addu        %[tmp33], %[d6],     %[d8]        \n\t"
            "addu        %[d1],    %[tmp0],   %[tmp3]      \n\t"
            "subu        %[d2],    %[tmp0],   %[tmp3]      \n\t"
            "addu        %[d3],    %[tmp1],   %[tmp2]      \n\t"
            "subu        %[d4],    %[tmp1],   %[tmp2]      \n\t"
            "addu        %[d5],    %[tmp00],  %[tmp33]     \n\t"
            "subu        %[d6],    %[tmp00],  %[tmp33]     \n\t"
            "addu        %[d7],    %[tmp11],  %[tmp22]     \n\t"
            "subu        %[d8],    %[tmp11],  %[tmp22]     \n\t"
            "mul         %[d1],    %[d1],     %[levScale]  \n\t"
            "mul         %[d2],    %[d2],     %[levScale]  \n\t"
            "mul         %[d3],    %[d3],     %[levScale]  \n\t"
            "mul         %[d4],    %[d4],     %[levScale]  \n\t"
            "mul         %[d5],    %[d5],     %[levScale]  \n\t"
            "mul         %[d6],    %[d6],     %[levScale]  \n\t"
            "mul         %[d7],    %[d7],     %[levScale]  \n\t"
            "mul         %[d8],    %[d8],     %[levScale]  \n\t"
            "sll         %[d1],    %[d1],     16           \n\t"
            "sll         %[d3],    %[d3],     16           \n\t"
            "sll         %[d5],    %[d5],     16           \n\t"
            "sll         %[d7],    %[d7],     16           \n\t"
            "packrl.ph   %[d1],    %[d2],     %[d1]        \n\t"
            "packrl.ph   %[d3],    %[d4],     %[d3]        \n\t"
            "packrl.ph   %[d5],    %[d6],     %[d5]        \n\t"
            "packrl.ph   %[d7],    %[d8],     %[d7]        \n\t"
            "shra.ph     %[d1],    %[d1],     1            \n\t"
            "shra.ph     %[d3],    %[d3],     1            \n\t"
            "shra.ph     %[d5],    %[d5],     1            \n\t"
            "shra.ph     %[d7],    %[d7],     1            \n\t"
            "sw          %[d1],    0(%[data])              \n\t"
            "sw          %[d3],    4(%[data])              \n\t"
            "sw          %[d5],    8(%[data])              \n\t"
            "sw          %[d7],    12(%[data])             \n\t"

            : [d1] "=&r" (d1), [d2] "=&r" (d2), [d3] "=&r" (d3),
              [d4] "=&r" (d4), [d5] "=&r" (d5), [d6] "=&r" (d6),
              [d7] "=&r" (d7), [d8] "=&r" (d8), [tmp1] "=&r" (tmp1),
              [tmp2] "=&r" (tmp2), [tmp3] "=&r" (tmp3),
              [tmp0] "=&r" (tmp0), [tmp11] "=&r" (tmp11),
              [tmp22] "=&r" (tmp22), [tmp33] "=&r" (tmp33),
              [tmp00] "=&r" (tmp00)
            : [data] "r" (data), [levScale] "r" (levScale)
            : "hi", "lo", "memory"
        );
    }
}



