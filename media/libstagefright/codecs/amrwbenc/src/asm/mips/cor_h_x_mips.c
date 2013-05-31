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
*       File: cor_h_x_mips.c                                           *
*                                                                      *
*      Description:Compute correlation between target "x[]" and "h[]"  *
*                  Designed for codebook search (24 pulses, 4 tracks,  *
*                  4 pulses per track, 16 positions in each track) to  *
*                  avoid saturation.                                   *
*                                                                      *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "math_op.h"

#define L_SUBFR   64
#define STEP      4

void cor_h_x_mips(
        Word16 h[],                           /* (i) Q12 : impulse response of weighted synthesis filter */
        Word16 x[],                           /* (i) Q0  : target vector                                 */
        Word16 dn[]                           /* (o) <12bit : correlation between target and h[]         */
        )
{
    Word32 i, j;
    Word32 y32[L_SUBFR], L_tot;
    Word16 *p1, *p2;
    Word32 *p3;
    Word32 L_max, L_max1, L_max2, L_max3;
    Word16 *p1_end;
    Word32 L_tmp1, L_tmp2, L_tmp3, L_tmp4;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6;
#if !defined(MIPS_DSP_R2_LE)
    Word32 Temp7, Temp8;
#endif /* !defined(MIPS_DSP_R2_LE) */
#if !defined(MIPS_DSP_R1_LE)
    Word32 Temp9, Temp10, Temp11, Temp12,
            Temp13, Temp14, Temp15, Temp16, Temp17;
#else
    Word32 *y, cond;
#endif /* !defined(MIPS_DSP_R1_LE) */

    L_tot  = 1;
    L_max  = 0;
    L_max1 = 0;
    L_max2 = 0;
    L_max3 = 0;
    for (i = 0; i < L_SUBFR; i += STEP)
    {
        p1 = &x[i];
        p1_end = p1 + (((L_SUBFR-i-4)>>2)<<2);
        p2 = &h[0];
#if defined(MIPS_DSP_R2_LE)
        y = &y32[i];

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "subu       %[cond],        %[p1_end],  %[p1]       \n\t"
            "mult       $ac0,           $0,         $0          \n\t"
            "mult       $ac1,           $0,         $0          \n\t"
            "mult       $ac2,           $0,         $0          \n\t"
            "blez       %[cond],        2f                      \n\t"
            " mult      $ac3,           $0,         $0          \n\t"
          "1:                                                   \n\t"
            "ulw        %[Temp1],       0(%[p2])                \n\t"
            "ulw        %[Temp2],       4(%[p2])                \n\t"
            "ulw        %[Temp3],       0(%[p1])                \n\t"
            "ulw        %[Temp4],       4(%[p1])                \n\t"
            "ulw        %[Temp5],       8(%[p1])                \n\t"
            "ulw        %[Temp6],       12(%[p1])               \n\t"
            "addiu      %[p2],          %[p2],      8           \n\t"
            "addiu      %[cond],        %[cond],    -8          \n\t"
            "dpa.w.ph   $ac0,           %[Temp1],   %[Temp3]    \n\t"
            "dpa.w.ph   $ac0,           %[Temp2],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac2,           %[Temp1],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac2,           %[Temp2],   %[Temp5]    \n\t"
            "packrl.ph  %[Temp3],       %[Temp4],   %[Temp3]    \n\t"
            "packrl.ph  %[Temp4],       %[Temp5],   %[Temp4]    \n\t"
            "packrl.ph  %[Temp5],       %[Temp6],   %[Temp5]    \n\t"
            "dpa.w.ph   $ac1,           %[Temp1],   %[Temp3]    \n\t"
            "dpa.w.ph   $ac1,           %[Temp2],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac3,           %[Temp1],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac3,           %[Temp2],   %[Temp5]    \n\t"
            "bgtz       %[cond],        1b                      \n\t"
            " addiu     %[p1],          %[p1],      8           \n\t"
          "2:                                                   \n\t"
            "ulw        %[Temp1],       0(%[p1])                \n\t"
            "ulw        %[Temp2],       0(%[p2])                \n\t"
            "ulw        %[Temp3],       4(%[p1])                \n\t"
            "ulw        %[Temp4],       4(%[p2])                \n\t"
            "dpa.w.ph   $ac0,           %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac2,           %[Temp2],   %[Temp3]    \n\t"
            "dpa.w.ph   $ac0,           %[Temp3],   %[Temp4]    \n\t"
            "packrl.ph  %[Temp5],       %[Temp3],   %[Temp1]    \n\t"
            "sra        %[Temp3],       %[Temp3],   16          \n\t"
            "seh        %[Temp6],       %[Temp2]                \n\t"
            "seh        %[Temp4],       %[Temp4]                \n\t"
            "dpa.w.ph   $ac1,           %[Temp2],   %[Temp5]    \n\t"
            "madd       $ac1,           %[Temp3],   %[Temp4]    \n\t"
            "madd       $ac3,           %[Temp3],   %[Temp6]    \n\t"
            "shilo      $ac0,           -1                      \n\t"
            "shilo      $ac1,           -1                      \n\t"
            "shilo      $ac2,           -1                      \n\t"
            "shilo      $ac3,           -1                      \n\t"
            "mflo       %[L_tmp1],      $ac0                    \n\t"
            "mflo       %[L_tmp2],      $ac1                    \n\t"
            "mflo       %[L_tmp3],      $ac2                    \n\t"
            "mflo       %[L_tmp4],      $ac3                    \n\t"
            "addiu      %[L_tmp1],      %[L_tmp1],  1           \n\t"
            "addiu      %[L_tmp2],      %[L_tmp2],  1           \n\t"
            "addiu      %[L_tmp3],      %[L_tmp3],  1           \n\t"
            "addiu      %[L_tmp4],      %[L_tmp4],  1           \n\t"
            "sw         %[L_tmp1],      0(%[y])                 \n\t"
            "sw         %[L_tmp2],      4(%[y])                 \n\t"
            "sw         %[L_tmp3],      8(%[y])                 \n\t"
            "sw         %[L_tmp4],      12(%[y])                \n\t"
            "absq_s.w   %[L_tmp1],      %[L_tmp1]               \n\t"
            "absq_s.w   %[L_tmp2],      %[L_tmp2]               \n\t"
            "absq_s.w   %[L_tmp3],      %[L_tmp3]               \n\t"
            "absq_s.w   %[L_tmp4],      %[L_tmp4]               \n\t"
            "subu       %[L_max],       %[L_tmp1],  %[L_max]    \n\t"
            "subu       %[L_max1],      %[L_tmp2],  %[L_max1]   \n\t"
            "subu       %[L_max2],      %[L_tmp3],  %[L_max2]   \n\t"
            "subu       %[L_max3],      %[L_tmp4],  %[L_max3]   \n\t"
            "sra        %[Temp1],       %[L_max],   31          \n\t"
            "sra        %[Temp2],       %[L_max1],  31          \n\t"
            "sra        %[Temp3],       %[L_max2],  31          \n\t"
            "sra        %[Temp4],       %[L_max3],  31          \n\t"
            "and        %[Temp1],       %[Temp1],   %[L_max]    \n\t"
            "and        %[Temp2],       %[Temp2],   %[L_max1]   \n\t"
            "and        %[Temp3],       %[Temp3],   %[L_max2]   \n\t"
            "and        %[Temp4],       %[Temp4],   %[L_max3]   \n\t"
            "subu       %[L_max],       %[L_tmp1],  %[Temp1]    \n\t"
            "subu       %[L_max1],      %[L_tmp2],  %[Temp2]    \n\t"
            "subu       %[L_max2],      %[L_tmp3],  %[Temp3]    \n\t"
            "subu       %[L_max3],      %[L_tmp4],  %[Temp4]    \n\t"

            ".set pop                                           \n\t"

            : [L_tmp1] "=&r" (L_tmp1), [L_tmp2] "=&r" (L_tmp2),
              [L_tmp3] "=&r" (L_tmp3), [L_tmp4] "=&r" (L_tmp4),
              [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [cond]   "=&r" (cond),   [L_max]  "+r"  (L_max),
              [L_max1] "+r"  (L_max1), [L_max2] "+r"  (L_max2),
              [L_max3] "+r"  (L_max3), [p1]     "+r"  (p1),
              [p2]     "+r"  (p2)
            : [p1_end] "r" (p1_end), [y] "r" (y)
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi",
              "$ac2lo", "$ac3hi", "$ac3lo", "memory"
        );
#elif defined(MIPS_DSP_R1_LE)
        y = &y32[i];

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "subu       %[cond],        %[p1_end],  %[p1]       \n\t"
            "mult       $ac0,           $0,         $0          \n\t"
            "mult       $ac1,           $0,         $0          \n\t"
            "mult       $ac2,           $0,         $0          \n\t"
            "blez       %[cond],        2f                      \n\t"
            " mult      $ac3,           $0,         $0          \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],       0(%[p2])                \n\t"
            "lh         %[Temp2],       2(%[p2])                \n\t"
            "lh         %[Temp3],       4(%[p2])                \n\t"
            "lh         %[Temp4],       6(%[p2])                \n\t"
            "lh         %[Temp5],       0(%[p1])                \n\t"
            "lh         %[Temp6],       2(%[p1])                \n\t"
            "lh         %[Temp7],       4(%[p1])                \n\t"
            "lh         %[Temp8],       6(%[p1])                \n\t"
            "addiu      %[cond],        %[cond],    -8          \n\t"
            "madd       $ac0,           %[Temp1],   %[Temp5]    \n\t"
            "madd       $ac0,           %[Temp2],   %[Temp6]    \n\t"
            "madd       $ac0,           %[Temp3],   %[Temp7]    \n\t"
            "madd       $ac0,           %[Temp4],   %[Temp8]    \n\t"
            "lh         %[Temp5],       8(%[p1])                \n\t"
            "madd       $ac1,           %[Temp1],   %[Temp6]    \n\t"
            "madd       $ac1,           %[Temp2],   %[Temp7]    \n\t"
            "madd       $ac1,           %[Temp3],   %[Temp8]    \n\t"
            "madd       $ac1,           %[Temp4],   %[Temp5]    \n\t"
            "lh         %[Temp6],       10(%[p1])               \n\t"
            "madd       $ac2,           %[Temp1],   %[Temp7]    \n\t"
            "madd       $ac2,           %[Temp2],   %[Temp8]    \n\t"
            "madd       $ac2,           %[Temp3],   %[Temp5]    \n\t"
            "madd       $ac2,           %[Temp4],   %[Temp6]    \n\t"
            "lh         %[Temp7],       12(%[p1])               \n\t"
            "madd       $ac3,           %[Temp1],   %[Temp8]    \n\t"
            "madd       $ac3,           %[Temp2],   %[Temp5]    \n\t"
            "madd       $ac3,           %[Temp3],   %[Temp6]    \n\t"
            "madd       $ac3,           %[Temp4],   %[Temp7]    \n\t"
            "addiu      %[p1],          %[p1],      8           \n\t"
            "bgtz       %[cond],        1b                      \n\t"
            " addiu     %[p2],          %[p2],      8           \n\t"
          "2:                                                   \n\t"
            "lh         %[Temp1],       0(%[p1])                \n\t"
            "lh         %[Temp2],       0(%[p2])                \n\t"
            "lh         %[Temp3],       2(%[p1])                \n\t"
            "lh         %[Temp4],       2(%[p2])                \n\t"
            "lh         %[Temp5],       4(%[p1])                \n\t"
            "lh         %[Temp6],       4(%[p2])                \n\t"
            "lh         %[Temp7],       6(%[p1])                \n\t"
            "lh         %[Temp8],       6(%[p2])                \n\t"
            "madd       $ac0,           %[Temp1],   %[Temp2]    \n\t"
            "madd       $ac0,           %[Temp3],   %[Temp4]    \n\t"
            "madd       $ac0,           %[Temp5],   %[Temp6]    \n\t"
            "madd       $ac0,           %[Temp7],   %[Temp8]    \n\t"
            "madd       $ac1,           %[Temp3],   %[Temp2]    \n\t"
            "madd       $ac1,           %[Temp5],   %[Temp4]    \n\t"
            "madd       $ac1,           %[Temp7],   %[Temp6]    \n\t"
            "madd       $ac2,           %[Temp5],   %[Temp2]    \n\t"
            "madd       $ac2,           %[Temp7],   %[Temp4]    \n\t"
            "madd       $ac3,           %[Temp7],   %[Temp2]    \n\t"
            "shilo      $ac0,           -1                      \n\t"
            "shilo      $ac1,           -1                      \n\t"
            "shilo      $ac2,           -1                      \n\t"
            "shilo      $ac3,           -1                      \n\t"
            "mflo       %[L_tmp1],      $ac0                    \n\t"
            "mflo       %[L_tmp2],      $ac1                    \n\t"
            "mflo       %[L_tmp3],      $ac2                    \n\t"
            "mflo       %[L_tmp4],      $ac3                    \n\t"
            "addiu      %[L_tmp1],      %[L_tmp1],  1           \n\t"
            "addiu      %[L_tmp2],      %[L_tmp2],  1           \n\t"
            "addiu      %[L_tmp3],      %[L_tmp3],  1           \n\t"
            "addiu      %[L_tmp4],      %[L_tmp4],  1           \n\t"
            "sw         %[L_tmp1],      0(%[y])                 \n\t"
            "sw         %[L_tmp2],      4(%[y])                 \n\t"
            "sw         %[L_tmp3],      8(%[y])                 \n\t"
            "sw         %[L_tmp4],      12(%[y])                \n\t"
            "absq_s.w   %[L_tmp1],      %[L_tmp1]               \n\t"
            "absq_s.w   %[L_tmp2],      %[L_tmp2]               \n\t"
            "absq_s.w   %[L_tmp3],      %[L_tmp3]               \n\t"
            "absq_s.w   %[L_tmp4],      %[L_tmp4]               \n\t"
            "subu       %[L_max],       %[L_tmp1],  %[L_max]    \n\t"
            "subu       %[L_max1],      %[L_tmp2],  %[L_max1]   \n\t"
            "subu       %[L_max2],      %[L_tmp3],  %[L_max2]   \n\t"
            "subu       %[L_max3],      %[L_tmp4],  %[L_max3]   \n\t"
            "sra        %[Temp1],       %[L_max],   31          \n\t"
            "sra        %[Temp2],       %[L_max1],  31          \n\t"
            "sra        %[Temp3],       %[L_max2],  31          \n\t"
            "sra        %[Temp4],       %[L_max3],  31          \n\t"
            "and        %[Temp1],       %[Temp1],   %[L_max]    \n\t"
            "and        %[Temp2],       %[Temp2],   %[L_max1]   \n\t"
            "and        %[Temp3],       %[Temp3],   %[L_max2]   \n\t"
            "and        %[Temp4],       %[Temp4],   %[L_max3]   \n\t"
            "subu       %[L_max],       %[L_tmp1],  %[Temp1]    \n\t"
            "subu       %[L_max1],      %[L_tmp2],  %[Temp2]    \n\t"
            "subu       %[L_max2],      %[L_tmp3],  %[Temp3]    \n\t"
            "subu       %[L_max3],      %[L_tmp4],  %[Temp4]    \n\t"

            ".set pop                                           \n\t"

            : [L_tmp1] "=&r" (L_tmp1), [L_tmp2] "=&r" (L_tmp2),
              [L_tmp3] "=&r" (L_tmp3), [L_tmp4] "=&r" (L_tmp4),
              [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [cond]   "=&r" (cond),   [L_max]  "+r"  (L_max),
              [L_max1] "+r"  (L_max1), [L_max2] "+r"  (L_max2),
              [L_max3] "+r"  (L_max3), [p1]     "+r"  (p1),
              [p2]     "+r"  (p2)
            : [p1_end] "r" (p1_end), [y] "r" (y)
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi",
              "$ac2lo", "$ac3hi", "$ac3lo", "memory"
        );
#else
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult       $0,             $0                      \n\t"
            "li         %[L_tmp2],      0                       \n\t"
            "li         %[L_tmp3],      0                       \n\t"
            "beq        %[p1],          %[p1_end],  1f          \n\t"
            " li        %[L_tmp4],      0                       \n\t"
          "2:                                                   \n\t"
            "lh         %[Temp1],       0(%[p2])                \n\t"
            "lh         %[Temp2],       2(%[p2])                \n\t"
            "lh         %[Temp3],       4(%[p2])                \n\t"
            "lh         %[Temp4],       6(%[p2])                \n\t"
            "lh         %[Temp5],       0(%[p1])                \n\t"
            "lh         %[Temp6],       2(%[p1])                \n\t"
            "lh         %[Temp7],       4(%[p1])                \n\t"
            "lh         %[Temp8],       6(%[p1])                \n\t"
            "lh         %[Temp9],       8(%[p1])                \n\t"
            "lh         %[Temp10],      10(%[p1])               \n\t"
            "lh         %[Temp11],      12(%[p1])               \n\t"
            "addiu      %[p1],          %[p1],      8           \n\t"
            "addiu      %[p2],          %[p2],      8           \n\t"
            "madd       %[Temp1],       %[Temp5]                \n\t"
            "madd       %[Temp2],       %[Temp6]                \n\t"
            "madd       %[Temp3],       %[Temp7]                \n\t"
            "madd       %[Temp4],       %[Temp8]                \n\t"
            "mul        %[Temp5],       %[Temp1],   %[Temp6]    \n\t"
            "mul        %[Temp12],      %[Temp2],   %[Temp7]    \n\t"
            "mul        %[Temp13],      %[Temp3],   %[Temp8]    \n\t"
            "mul        %[Temp14],      %[Temp4],   %[Temp9]    \n\t"
            "mul        %[Temp6],       %[Temp1],   %[Temp7]    \n\t"
            "mul        %[Temp15],      %[Temp2],   %[Temp8]    \n\t"
            "mul        %[Temp16],      %[Temp3],   %[Temp9]    \n\t"
            "mul        %[Temp17],      %[Temp4],   %[Temp10]   \n\t"
            "mul        %[Temp7],       %[Temp1],   %[Temp8]    \n\t"
            "mul        %[Temp8],       %[Temp2],   %[Temp9]    \n\t"
            "mul        %[Temp9],       %[Temp3],   %[Temp10]   \n\t"
            "mul        %[Temp10],      %[Temp4],   %[Temp11]   \n\t"
            "addu       %[L_tmp2],      %[L_tmp2],  %[Temp5]    \n\t"
            "addu       %[L_tmp2],      %[L_tmp2],  %[Temp12]   \n\t"
            "addu       %[L_tmp2],      %[L_tmp2],  %[Temp13]   \n\t"
            "addu       %[L_tmp2],      %[L_tmp2],  %[Temp14]   \n\t"
            "addu       %[L_tmp3],      %[L_tmp3],  %[Temp6]    \n\t"
            "addu       %[L_tmp3],      %[L_tmp3],  %[Temp15]   \n\t"
            "addu       %[L_tmp3],      %[L_tmp3],  %[Temp16]   \n\t"
            "addu       %[L_tmp3],      %[L_tmp3],  %[Temp17]   \n\t"
            "addu       %[L_tmp4],      %[L_tmp4],  %[Temp7]    \n\t"
            "addu       %[L_tmp4],      %[L_tmp4],  %[Temp8]    \n\t"
            "addu       %[L_tmp4],      %[L_tmp4],  %[Temp9]    \n\t"
            "bne        %[p1],          %[p1_end],  2b          \n\t"
            " addu      %[L_tmp4],      %[L_tmp4],  %[Temp10]   \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],       0(%[p1])                \n\t"
            "lh         %[Temp2],       0(%[p2])                \n\t"
            "lh         %[Temp3],       2(%[p1])                \n\t"
            "lh         %[Temp4],       2(%[p2])                \n\t"
            "lh         %[Temp5],       4(%[p1])                \n\t"
            "lh         %[Temp6],       4(%[p2])                \n\t"
            "lh         %[Temp7],       6(%[p1])                \n\t"
            "lh         %[Temp8],       6(%[p2])                \n\t"
            "madd       %[Temp1],       %[Temp2]                \n\t"
            "madd       %[Temp3],       %[Temp4]                \n\t"
            "madd       %[Temp5],       %[Temp6]                \n\t"
            "madd       %[Temp7],       %[Temp8]                \n\t"
            "mul        %[Temp9],       %[Temp3],   %[Temp2]    \n\t"
            "mul        %[Temp10],      %[Temp5],   %[Temp4]    \n\t"
            "mul        %[Temp11],      %[Temp7],   %[Temp6]    \n\t"
            "mul        %[Temp12],      %[Temp5],   %[Temp2]    \n\t"
            "mflo       %[L_tmp1]                               \n\t"
            "mul        %[Temp13],      %[Temp7],   %[Temp4]    \n\t"
            "mul        %[Temp14],      %[Temp7],   %[Temp2]    \n\t"
            "addu       %[L_tmp2],      %[L_tmp2],  %[Temp9]    \n\t"
            "addu       %[L_tmp2],      %[L_tmp2],  %[Temp10]   \n\t"
            "addu       %[L_tmp2],      %[L_tmp2],  %[Temp11]   \n\t"
            "sll        %[L_tmp2],      %[L_tmp2],  1           \n\t"
            "addiu      %[L_tmp2],      %[L_tmp2],  1           \n\t"
            "sll        %[L_tmp1],      %[L_tmp1],  1           \n\t"
            "addiu      %[L_tmp1],      %[L_tmp1],  1           \n\t"
            "addu       %[L_tmp3],      %[L_tmp3],  %[Temp12]   \n\t"
            "addu       %[L_tmp3],      %[L_tmp3],  %[Temp13]   \n\t"
            "sll        %[L_tmp3],      %[L_tmp3],  1           \n\t"
            "addiu      %[L_tmp3],      %[L_tmp3],  1           \n\t"
            "addu       %[L_tmp4],      %[L_tmp4],  %[Temp14]   \n\t"
            "sll        %[L_tmp4],      %[L_tmp4],  1           \n\t"
            "addiu      %[L_tmp4],      %[L_tmp4],  1           \n\t"

            ".set pop                                           \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
              [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
              [Temp13] "=&r" (Temp13), [Temp14] "=&r" (Temp14),
              [Temp15] "=&r" (Temp15), [Temp16] "=&r" (Temp16),
              [Temp17] "=&r" (Temp17), [L_tmp1] "=&r" (L_tmp1),
              [L_tmp2] "=&r" (L_tmp2), [L_tmp3] "=&r" (L_tmp3),
              [L_tmp4] "=&r" (L_tmp4), [p1]     "+r"  (p1),
              [p2]     "+r"  (p2)
            : [p1_end] "r" (p1_end)
            : "hi", "lo", "memory"
        );

        y32[i  ] = L_tmp1;
        y32[i+1] = L_tmp2;
        y32[i+2] = L_tmp3;
        y32[i+3] = L_tmp4;

        L_tmp1 = (L_tmp1 > 0)? L_tmp1:-L_tmp1;
        L_tmp2 = (L_tmp2 > 0)? L_tmp2:-L_tmp2;
        L_tmp3 = (L_tmp3 > 0)? L_tmp3:-L_tmp3;
        L_tmp4 = (L_tmp4 > 0)? L_tmp4:-L_tmp4;

        if(L_tmp1 > L_max)
        {
            L_max = L_tmp1;
        }

        if(L_tmp2 > L_max1)
        {
            L_max1 = L_tmp2;
        }

        if(L_tmp3 > L_max2)
        {
            L_max2 = L_tmp3;
        }

        if(L_tmp4 > L_max3)
        {
            L_max3 = L_tmp4;
        }
#endif /* defined(MIPS_DSP_R2_LE) */
    }
    L_max = ((L_max + L_max1 + L_max2 + L_max3) >> 2);
    L_tot = vo_L_add(L_tot, L_max);
    L_tot = vo_L_add(L_tot, (L_max >> 1));

    j = norm_l(L_tot) - 4;
    p1 = dn;
    p3 = y32;
    for (i = 0; i < L_SUBFR; i+=4)
    {
        *p1++ = vo_round(L_shl(*p3++, j));
        *p1++ = vo_round(L_shl(*p3++, j));
        *p1++ = vo_round(L_shl(*p3++, j));
        *p1++ = vo_round(L_shl(*p3++, j));
    }
    return;
}



