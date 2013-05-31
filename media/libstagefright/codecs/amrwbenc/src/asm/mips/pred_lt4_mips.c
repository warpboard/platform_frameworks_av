/*
 ** Copyright 2003-2010, VisualOn, Inc.
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

/***********************************************************************
*      File: pred_lt4_mips.c                                           *
*                                                                      *
*      Description: Compute the result of long term prediction with    *
*      fractional interpolation of resolution 1/4                      *
*      on return exc[0..L_subr-1] contains the interpolated signal     *
*      (adaptive codebook excitation)                                  *
*                                                                      *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"

#define UP_SAMP      4
#define L_INTERPOL2  16

extern Word16 inter4_2[4][32];

void Pred_lt4_mips(
        Word16 exc[],                         /* in/out: excitation buffer */
        Word16 T0,                            /* input : integer pitch lag */
        Word16 frac,                          /* input : fraction of lag   */
        Word16 L_subfr                        /* input : subframe size     */
        )
{
    Word16 j, k, *x;
    Word16 *ptr;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8;
#if defined(MIPS_DSP_R2_LE)
    Word16 *e, *ptr_end;
    Word32 Temp9, Temp10;
#elif defined(MIPS_DSP_R1_LE)
    Word16 *e, *ptr_end;
    Word32 Temp9, Temp10, Temp11, Temp12, Temp13, Temp14, Temp15, Temp16;
#else
    Word32 L_sum;
#endif /* defined(MIPS_DSP_R2_LE) */

    x = exc - T0;
    frac = -frac;
    if (frac < 0)
    {
        frac += UP_SAMP;
        x--;
    }
    x -= 15;
    k = 3 - frac;

    ptr = &(inter4_2[k][0]);
#if defined(MIPS_DSP_R2_LE)
    e = exc;
    for (j = 0; j < (L_subfr - 1) >> 2; j++)
    {
        ptr_end = ptr + 32;

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult           $ac0,       $0,         $0          \n\t"
            "mult           $ac1,       $0,         $0          \n\t"
            "mult           $ac2,       $0,         $0          \n\t"
            "mult           $ac3,       $0,         $0          \n\t"
          "1:                                                   \n\t"
            "ulw            %[Temp1],   0(%[ptr])               \n\t"
            "ulw            %[Temp2],   4(%[ptr])               \n\t"
            "ulw            %[Temp3],   8(%[ptr])               \n\t"
            "ulw            %[Temp4],   12(%[ptr])              \n\t"
            "ulw            %[Temp5],   0(%[x])                 \n\t"
            "ulw            %[Temp6],   4(%[x])                 \n\t"
            "ulw            %[Temp7],   8(%[x])                 \n\t"
            "ulw            %[Temp8],   12(%[x])                \n\t"
            "ulw            %[Temp9],   16(%[x])                \n\t"
            "lh             %[Temp10],  20(%[x])                \n\t"
            "addiu          %[ptr],     %[ptr],     16          \n\t"
            "dpa.w.ph       $ac0,       %[Temp1],   %[Temp5]    \n\t"
            "dpa.w.ph       $ac0,       %[Temp2],   %[Temp6]    \n\t"
            "dpa.w.ph       $ac0,       %[Temp3],   %[Temp7]    \n\t"
            "dpa.w.ph       $ac0,       %[Temp4],   %[Temp8]    \n\t"
            "dpa.w.ph       $ac2,       %[Temp1],   %[Temp6]    \n\t"
            "dpa.w.ph       $ac2,       %[Temp2],   %[Temp7]    \n\t"
            "dpa.w.ph       $ac2,       %[Temp3],   %[Temp8]    \n\t"
            "dpa.w.ph       $ac2,       %[Temp4],   %[Temp9]    \n\t"
            "packrl.ph      %[Temp5],   %[Temp6],   %[Temp5]    \n\t"
            "packrl.ph      %[Temp6],   %[Temp7],   %[Temp6]    \n\t"
            "packrl.ph      %[Temp7],   %[Temp8],   %[Temp7]    \n\t"
            "packrl.ph      %[Temp8],   %[Temp9],   %[Temp8]    \n\t"
            "packrl.ph      %[Temp9],   %[Temp10],  %[Temp9]    \n\t"
            "dpa.w.ph       $ac1,       %[Temp1],   %[Temp5]    \n\t"
            "dpa.w.ph       $ac1,       %[Temp2],   %[Temp6]    \n\t"
            "dpa.w.ph       $ac1,       %[Temp3],   %[Temp7]    \n\t"
            "dpa.w.ph       $ac1,       %[Temp4],   %[Temp8]    \n\t"
            "dpa.w.ph       $ac3,       %[Temp1],   %[Temp6]    \n\t"
            "dpa.w.ph       $ac3,       %[Temp2],   %[Temp7]    \n\t"
            "dpa.w.ph       $ac3,       %[Temp3],   %[Temp8]    \n\t"
            "dpa.w.ph       $ac3,       %[Temp4],   %[Temp9]    \n\t"
            "bne            %[ptr],     %[ptr_end], 1b          \n\t"
            " addiu         %[x],       %[x],       16          \n\t"
            "extr_r.w       %[Temp1],   $ac0,       14          \n\t"
            "extr_r.w       %[Temp2],   $ac1,       14          \n\t"
            "extr_r.w       %[Temp3],   $ac2,       14          \n\t"
            "extr_r.w       %[Temp4],   $ac3,       14          \n\t"
            "addiu          %[x],       %[x],       -56         \n\t"
            "addiu          %[ptr],     %[ptr],     -64         \n\t"
            "shll_s.w       %[Temp1],   %[Temp1],   16          \n\t"
            "shll_s.w       %[Temp2],   %[Temp2],   16          \n\t"
            "shll_s.w       %[Temp3],   %[Temp3],   16          \n\t"
            "shll_s.w       %[Temp4],   %[Temp4],   16          \n\t"
            "precrq.ph.w    %[Temp2],   %[Temp2],   %[Temp1]    \n\t"
            "precrq.ph.w    %[Temp4],   %[Temp4],   %[Temp3]    \n\t"
            "usw            %[Temp2],   0(%[e])                 \n\t"
            "usw            %[Temp4],   4(%[e])                 \n\t"
            "addiu          %[e],       %[e],       8           \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2]  "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4]  "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6]  "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8]  "=&r" (Temp8),
              [Temp9] "=&r" (Temp9), [Temp10] "=&r" (Temp10),
              [x]     "+r"  (x),     [ptr]    "+r"  (ptr),
              [e]     "+r"  (e)
            : [ptr_end] "r" (ptr_end)
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo",
              "memory"
        );
    }

    ptr_end = ptr + 32;

    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "mult       $ac0,       $0,         $0          \n\t"
      "2:                                               \n\t"
        "ulw        %[Temp1],   0(%[ptr])               \n\t"
        "ulw        %[Temp2],   0(%[x])                 \n\t"
        "ulw        %[Temp3],   4(%[ptr])               \n\t"
        "ulw        %[Temp4],   4(%[x])                 \n\t"
        "ulw        %[Temp5],   8(%[ptr])               \n\t"
        "ulw        %[Temp6],   8(%[x])                 \n\t"
        "ulw        %[Temp7],   12(%[ptr])              \n\t"
        "ulw        %[Temp8],   12(%[x])                \n\t"
        "addiu      %[ptr],     %[ptr],     16          \n\t"
        "dpa.w.ph   $ac0,       %[Temp1],   %[Temp2]    \n\t"
        "dpa.w.ph   $ac0,       %[Temp3],   %[Temp4]    \n\t"
        "dpa.w.ph   $ac0,       %[Temp5],   %[Temp6]    \n\t"
        "dpa.w.ph   $ac0,       %[Temp7],   %[Temp8]    \n\t"
        "bne        %[ptr],     %[ptr_end], 2b          \n\t"
        " addiu     %[x],       %[x],       16          \n\t"
        "extr_r.w   %[Temp1],   $ac0,       14          \n\t"
        "shll_s.w   %[Temp1],   %[Temp1],   16          \n\t"
        "sra        %[Temp1],   %[Temp1],   16          \n\t"

        ".set pop                                       \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
          [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
          [ptr]   "+r"  (ptr),   [x]     "+r"  (x)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "memory"
    );

    exc[L_subfr-1] = Temp1;
#elif defined(MIPS_DSP_R1_LE)
    e = exc;
    for (j = 0; j < (L_subfr - 1) >> 2; j++)
    {
        ptr_end = ptr + 32;

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult           $ac0,       $0,         $0          \n\t"
            "mult           $ac1,       $0,         $0          \n\t"
            "mult           $ac2,       $0,         $0          \n\t"
            "mult           $ac3,       $0,         $0          \n\t"
          "1:                                                   \n\t"
            "lh             %[Temp1],   0(%[ptr])               \n\t"
            "lh             %[Temp2],   2(%[ptr])               \n\t"
            "lh             %[Temp3],   4(%[ptr])               \n\t"
            "lh             %[Temp4],   6(%[ptr])               \n\t"
            "lh             %[Temp5],   8(%[ptr])               \n\t"
            "lh             %[Temp6],   10(%[ptr])              \n\t"
            "lh             %[Temp7],   12(%[ptr])              \n\t"
            "lh             %[Temp8],   14(%[ptr])              \n\t"
            "lh             %[Temp9],   0(%[x])                 \n\t"
            "lh             %[Temp10],  2(%[x])                 \n\t"
            "lh             %[Temp11],  4(%[x])                 \n\t"
            "lh             %[Temp12],  6(%[x])                 \n\t"
            "lh             %[Temp13],  8(%[x])                 \n\t"
            "lh             %[Temp14],  10(%[x])                \n\t"
            "lh             %[Temp15],  12(%[x])                \n\t"
            "lh             %[Temp16],  14(%[x])                \n\t"
            "addiu          %[ptr],     %[ptr],     16          \n\t"
            "madd           $ac0,       %[Temp1],   %[Temp9]    \n\t"
            "madd           $ac0,       %[Temp2],   %[Temp10]   \n\t"
            "madd           $ac0,       %[Temp3],   %[Temp11]   \n\t"
            "madd           $ac0,       %[Temp4],   %[Temp12]   \n\t"
            "madd           $ac0,       %[Temp5],   %[Temp13]   \n\t"
            "madd           $ac0,       %[Temp6],   %[Temp14]   \n\t"
            "madd           $ac0,       %[Temp7],   %[Temp15]   \n\t"
            "madd           $ac0,       %[Temp8],   %[Temp16]   \n\t"
            "lh             %[Temp9],   16(%[x])                \n\t"
            "madd           $ac1,       %[Temp1],   %[Temp10]   \n\t"
            "madd           $ac1,       %[Temp2],   %[Temp11]   \n\t"
            "madd           $ac1,       %[Temp3],   %[Temp12]   \n\t"
            "madd           $ac1,       %[Temp4],   %[Temp13]   \n\t"
            "madd           $ac1,       %[Temp5],   %[Temp14]   \n\t"
            "madd           $ac1,       %[Temp6],   %[Temp15]   \n\t"
            "madd           $ac1,       %[Temp7],   %[Temp16]   \n\t"
            "madd           $ac1,       %[Temp8],   %[Temp9]    \n\t"
            "lh             %[Temp10],  18(%[x])                \n\t"
            "madd           $ac2,       %[Temp1],   %[Temp11]   \n\t"
            "madd           $ac2,       %[Temp2],   %[Temp12]   \n\t"
            "madd           $ac2,       %[Temp3],   %[Temp13]   \n\t"
            "madd           $ac2,       %[Temp4],   %[Temp14]   \n\t"
            "madd           $ac2,       %[Temp5],   %[Temp15]   \n\t"
            "madd           $ac2,       %[Temp6],   %[Temp16]   \n\t"
            "madd           $ac2,       %[Temp7],   %[Temp9]    \n\t"
            "madd           $ac2,       %[Temp8],   %[Temp10]   \n\t"
            "lh             %[Temp11],  20(%[x])                \n\t"
            "madd           $ac3,       %[Temp1],   %[Temp12]   \n\t"
            "madd           $ac3,       %[Temp2],   %[Temp13]   \n\t"
            "madd           $ac3,       %[Temp3],   %[Temp14]   \n\t"
            "madd           $ac3,       %[Temp4],   %[Temp15]   \n\t"
            "madd           $ac3,       %[Temp5],   %[Temp16]   \n\t"
            "madd           $ac3,       %[Temp6],   %[Temp9]    \n\t"
            "madd           $ac3,       %[Temp7],   %[Temp10]   \n\t"
            "madd           $ac3,       %[Temp8],   %[Temp11]   \n\t"
            "bne            %[ptr],     %[ptr_end], 1b          \n\t"
            " addiu         %[x],       %[x],       16          \n\t"
            "extr_r.w       %[Temp1],   $ac0,       14          \n\t"
            "extr_r.w       %[Temp2],   $ac1,       14          \n\t"
            "extr_r.w       %[Temp3],   $ac2,       14          \n\t"
            "extr_r.w       %[Temp4],   $ac3,       14          \n\t"
            "addiu          %[x],       %[x],       -56         \n\t"
            "addiu          %[ptr],     %[ptr],     -64         \n\t"
            "shll_s.w       %[Temp1],   %[Temp1],   16          \n\t"
            "shll_s.w       %[Temp2],   %[Temp2],   16          \n\t"
            "shll_s.w       %[Temp3],   %[Temp3],   16          \n\t"
            "shll_s.w       %[Temp4],   %[Temp4],   16          \n\t"
            "precrq.ph.w    %[Temp2],   %[Temp2],   %[Temp1]    \n\t"
            "precrq.ph.w    %[Temp4],   %[Temp4],   %[Temp3]    \n\t"
            "usw            %[Temp2],   0(%[e])                 \n\t"
            "usw            %[Temp4],   4(%[e])                 \n\t"
            "addiu          %[e],       %[e],       8           \n\t"

            ".set pop                                           \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
              [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
              [Temp13] "=&r" (Temp13), [Temp14] "=&r" (Temp14),
              [Temp15] "=&r" (Temp15), [Temp16] "=&r" (Temp16),
              [x]      "+r"  (x),      [ptr]    "+r"  (ptr),
              [e]      "+r"  (e)
            : [ptr_end] "r" (ptr_end)
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo",
              "memory"
        );
    }

    ptr_end = ptr + 32;

    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "mult       $ac0,       $0,         $0          \n\t"
      "2:                                               \n\t"
        "lh         %[Temp1],   0(%[ptr])               \n\t"
        "lh         %[Temp2],   0(%[x])                 \n\t"
        "lh         %[Temp3],   2(%[ptr])               \n\t"
        "lh         %[Temp4],   2(%[x])                 \n\t"
        "lh         %[Temp5],   4(%[ptr])               \n\t"
        "lh         %[Temp6],   4(%[x])                 \n\t"
        "lh         %[Temp7],   6(%[ptr])               \n\t"
        "lh         %[Temp8],   6(%[x])                 \n\t"
        "lh         %[Temp9],   8(%[ptr])               \n\t"
        "lh         %[Temp10],  8(%[x])                 \n\t"
        "lh         %[Temp11],  10(%[ptr])              \n\t"
        "lh         %[Temp12],  10(%[x])                \n\t"
        "lh         %[Temp13],  12(%[ptr])              \n\t"
        "lh         %[Temp14],  12(%[x])                \n\t"
        "lh         %[Temp15],  14(%[ptr])              \n\t"
        "lh         %[Temp16],  14(%[x])                \n\t"
        "madd       $ac0,       %[Temp1],   %[Temp2]    \n\t"
        "madd       $ac0,       %[Temp3],   %[Temp4]    \n\t"
        "madd       $ac0,       %[Temp5],   %[Temp6]    \n\t"
        "madd       $ac0,       %[Temp7],   %[Temp8]    \n\t"
        "madd       $ac0,       %[Temp9],   %[Temp10]   \n\t"
        "madd       $ac0,       %[Temp11],  %[Temp12]   \n\t"
        "madd       $ac0,       %[Temp13],  %[Temp14]   \n\t"
        "madd       $ac0,       %[Temp15],  %[Temp16]   \n\t"
        "addiu      %[ptr],     %[ptr],     16          \n\t"
        "bne        %[ptr],     %[ptr_end], 2b          \n\t"
        " addiu     %[x],       %[x],       16          \n\t"
        "extr_r.w   %[Temp1],   $ac0,       14          \n\t"
        "shll_s.w   %[Temp1],   %[Temp1],   16          \n\t"
        "sra        %[Temp1],   %[Temp1],   16          \n\t"

        ".set pop                                       \n\t"

        : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
          [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
          [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
          [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
          [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
          [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
          [Temp13] "=&r" (Temp13), [Temp14] "=&r" (Temp14),
          [Temp15] "=&r" (Temp15), [Temp16] "=&r" (Temp16),
          [ptr]    "+r"  (ptr),    [x]      "+r"  (x)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "memory"
    );

    exc[L_subfr-1] = Temp1;
#else
    for (j = 0; j < L_subfr; j++)
    {
        __asm__ volatile (
            "lh             %[Temp1],   0(%[x])                 \n\t"
            "lh             %[Temp2],   0(%[ptr])               \n\t"
            "lh             %[Temp3],   2(%[x])                 \n\t"
            "lh             %[Temp4],   2(%[ptr])               \n\t"
            "lh             %[Temp5],   4(%[x])                 \n\t"
            "lh             %[Temp6],   4(%[ptr])               \n\t"
            "lh             %[Temp7],   6(%[x])                 \n\t"
            "lh             %[Temp8],   6(%[ptr])               \n\t"
            "mult           %[Temp1],   %[Temp2]                \n\t"
            "madd           %[Temp3],   %[Temp4]                \n\t"
            "madd           %[Temp5],   %[Temp6]                \n\t"
            "madd           %[Temp7],   %[Temp8]                \n\t"
            "lh             %[Temp1],   8(%[x])                 \n\t"
            "lh             %[Temp2],   8(%[ptr])               \n\t"
            "lh             %[Temp3],   10(%[x])                \n\t"
            "lh             %[Temp4],   10(%[ptr])              \n\t"
            "lh             %[Temp5],   12(%[x])                \n\t"
            "lh             %[Temp6],   12(%[ptr])              \n\t"
            "lh             %[Temp7],   14(%[x])                \n\t"
            "lh             %[Temp8],   14(%[ptr])              \n\t"
            "madd           %[Temp1],   %[Temp2]                \n\t"
            "madd           %[Temp3],   %[Temp4]                \n\t"
            "madd           %[Temp5],   %[Temp6]                \n\t"
            "madd           %[Temp7],   %[Temp8]                \n\t"
            "lh             %[Temp1],   16(%[x])                \n\t"
            "lh             %[Temp2],   16(%[ptr])              \n\t"
            "lh             %[Temp3],   18(%[x])                \n\t"
            "lh             %[Temp4],   18(%[ptr])              \n\t"
            "lh             %[Temp5],   20(%[x])                \n\t"
            "lh             %[Temp6],   20(%[ptr])              \n\t"
            "lh             %[Temp7],   22(%[x])                \n\t"
            "lh             %[Temp8],   22(%[ptr])              \n\t"
            "madd           %[Temp1],   %[Temp2]                \n\t"
            "madd           %[Temp3],   %[Temp4]                \n\t"
            "madd           %[Temp5],   %[Temp6]                \n\t"
            "madd           %[Temp7],   %[Temp8]                \n\t"
            "lh             %[Temp1],   24(%[x])                \n\t"
            "lh             %[Temp2],   24(%[ptr])              \n\t"
            "lh             %[Temp3],   26(%[x])                \n\t"
            "lh             %[Temp4],   26(%[ptr])              \n\t"
            "lh             %[Temp5],   28(%[x])                \n\t"
            "lh             %[Temp6],   28(%[ptr])              \n\t"
            "lh             %[Temp7],   30(%[x])                \n\t"
            "lh             %[Temp8],   30(%[ptr])              \n\t"
            "madd           %[Temp1],   %[Temp2]                \n\t"
            "madd           %[Temp3],   %[Temp4]                \n\t"
            "madd           %[Temp5],   %[Temp6]                \n\t"
            "madd           %[Temp7],   %[Temp8]                \n\t"
            "lh             %[Temp1],   32(%[x])                \n\t"
            "lh             %[Temp2],   32(%[ptr])              \n\t"
            "lh             %[Temp3],   34(%[x])                \n\t"
            "lh             %[Temp4],   34(%[ptr])              \n\t"
            "lh             %[Temp5],   36(%[x])                \n\t"
            "lh             %[Temp6],   36(%[ptr])              \n\t"
            "lh             %[Temp7],   38(%[x])                \n\t"
            "lh             %[Temp8],   38(%[ptr])              \n\t"
            "madd           %[Temp1],   %[Temp2]                \n\t"
            "madd           %[Temp3],   %[Temp4]                \n\t"
            "madd           %[Temp5],   %[Temp6]                \n\t"
            "madd           %[Temp7],   %[Temp8]                \n\t"
            "lh             %[Temp1],   40(%[x])                \n\t"
            "lh             %[Temp2],   40(%[ptr])              \n\t"
            "lh             %[Temp3],   42(%[x])                \n\t"
            "lh             %[Temp4],   42(%[ptr])              \n\t"
            "lh             %[Temp5],   44(%[x])                \n\t"
            "lh             %[Temp6],   44(%[ptr])              \n\t"
            "lh             %[Temp7],   46(%[x])                \n\t"
            "lh             %[Temp8],   46(%[ptr])              \n\t"
            "madd           %[Temp1],   %[Temp2]                \n\t"
            "madd           %[Temp3],   %[Temp4]                \n\t"
            "madd           %[Temp5],   %[Temp6]                \n\t"
            "madd           %[Temp7],   %[Temp8]                \n\t"
            "lh             %[Temp1],   48(%[x])                \n\t"
            "lh             %[Temp2],   48(%[ptr])              \n\t"
            "lh             %[Temp3],   50(%[x])                \n\t"
            "lh             %[Temp4],   50(%[ptr])              \n\t"
            "lh             %[Temp5],   52(%[x])                \n\t"
            "lh             %[Temp6],   52(%[ptr])              \n\t"
            "lh             %[Temp7],   54(%[x])                \n\t"
            "lh             %[Temp8],   54(%[ptr])              \n\t"
            "madd           %[Temp1],   %[Temp2]                \n\t"
            "madd           %[Temp3],   %[Temp4]                \n\t"
            "madd           %[Temp5],   %[Temp6]                \n\t"
            "madd           %[Temp7],   %[Temp8]                \n\t"
            "lh             %[Temp1],   56(%[x])                \n\t"
            "lh             %[Temp2],   56(%[ptr])              \n\t"
            "lh             %[Temp3],   58(%[x])                \n\t"
            "lh             %[Temp4],   58(%[ptr])              \n\t"
            "lh             %[Temp5],   60(%[x])                \n\t"
            "lh             %[Temp6],   60(%[ptr])              \n\t"
            "lh             %[Temp7],   62(%[x])                \n\t"
            "lh             %[Temp8],   62(%[ptr])              \n\t"
            "madd           %[Temp1],   %[Temp2]                \n\t"
            "madd           %[Temp3],   %[Temp4]                \n\t"
            "madd           %[Temp5],   %[Temp6]                \n\t"
            "madd           %[Temp7],   %[Temp8]                \n\t"
            "mflo           %[L_sum]                            \n\t"

            : [Temp1]"=&r"(Temp1), [Temp2]"=&r"(Temp2),
              [Temp3]"=&r"(Temp3), [Temp4]"=&r"(Temp4),
              [Temp5]"=&r"(Temp5), [Temp6]"=&r"(Temp6),
              [Temp7]"=&r"(Temp7), [Temp8]"=&r"(Temp8),
              [L_sum]"=r"(L_sum)
            : [ptr]"r"(ptr), [x]"r"(x)
            : "hi", "lo", "memory"
        );

        L_sum = L_shl2(L_sum, 2);
        exc[j] = extract_h(L_add(L_sum, 0x8000));
        x++;
    }
#endif /* defined(MIPS_DSP_R2_LE) */

    return;
}



