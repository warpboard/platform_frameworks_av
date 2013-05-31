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
       File: convolve_mips.c                                            *
                                                                        *
    Description:Perform the convolution between two vectors x[] and h[] *
                and write the result in the vector y[]                  *
                                                                        *
************************************************************************
*                                                                       *
*   Optimized for MIPS architecture                                     *
*                                                                       *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"

void Convolve_mips (
        Word16 x[],        /* (i)     : input vector                           */
        Word16 h[],        /* (i)     : impulse response                       */
        Word16 y[],        /* (o)     : output vector                          */
        Word16 L           /* (i)     : vector size                            */
        )
{
    Word32 n;
    Word16 *tmpH, *tmpX;
    Word16 *X_end;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6;
#if !defined(MIPS_DSP_R2_LE)
    Word32 Temp7, Temp8;
#endif /* !defined(MIPS_DSP_R2_LE) */
#if !defined(MIPS_DSP_R1_LE)
    Word32 s;
#else
    Word16 *y1 = y;
    Word32 cond;
#endif /* !defined(MIPS_DSP_R1_LE) */

#if defined(MIPS_DSP_R2_LE)
    for (n = 0; n < 64; n += 4)
    {
        tmpH = h + n - 4;
        tmpX = x;
        X_end = x + n;

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "ulw        %[Temp1],   0(%[tmpX])                  \n\t"
            "ulw        %[Temp2],   8(%[tmpH])                  \n\t"
            "ulw        %[Temp3],   4(%[tmpX])                  \n\t"
            "ulw        %[Temp4],   12(%[tmpH])                 \n\t"
            "subu       %[cond],    %[X_end],       %[tmpX]     \n\t"
            "mult       $ac1,       $0,             $0          \n\t"
            "dpax.w.ph  $ac1,       %[Temp1],       %[Temp2]    \n\t"
            "mult       $ac3,       $0,             $0          \n\t"
            "dpax.w.ph  $ac3,       %[Temp1],       %[Temp4]    \n\t"
            "dpax.w.ph  $ac3,       %[Temp2],       %[Temp3]    \n\t"
            "packrl.ph  %[Temp5],   %[Temp4],       %[Temp2]    \n\t"
            "seh        %[Temp2],   %[Temp2]                    \n\t"
            "seh        %[Temp3],   %[Temp3]                    \n\t"
            "mult       $ac2,       $0,             $0          \n\t"
            "dpax.w.ph  $ac2,       %[Temp1],       %[Temp5]    \n\t"
            "madd       $ac2,       %[Temp2],       %[Temp3]    \n\t"
            "seh        %[Temp1],   %[Temp1]                    \n\t"
            "mult       $ac0,       $0,             $0          \n\t"
            "madd       $ac0,       %[Temp1],       %[Temp2]    \n\t"
            "blez       %[cond],    2f                          \n\t"
            " addiu     %[y1],      %[y1],          8           \n\t"
          "1:                                                   \n\t"
            "ulw        %[Temp1],   0(%[tmpH])                  \n\t"
            "ulw        %[Temp2],   4(%[tmpH])                  \n\t"
            "ulw        %[Temp3],   2(%[tmpX])                  \n\t"
            "ulw        %[Temp4],   6(%[tmpX])                  \n\t"
            "ulw        %[Temp5],   10(%[tmpX])                 \n\t"
            "ulw        %[Temp6],   14(%[tmpX])                 \n\t"
            "addiu      %[tmpX],    %[tmpX],        8           \n\t"
            "addiu      %[cond],    %[cond],        -8          \n\t"
            "dpax.w.ph  $ac0,       %[Temp1],       %[Temp4]    \n\t"
            "dpax.w.ph  $ac0,       %[Temp2],       %[Temp3]    \n\t"
            "dpax.w.ph  $ac2,       %[Temp1],       %[Temp5]    \n\t"
            "dpax.w.ph  $ac2,       %[Temp2],       %[Temp4]    \n\t"
            "packrl.ph  %[Temp3],   %[Temp4],       %[Temp3]    \n\t"
            "packrl.ph  %[Temp4],   %[Temp5],       %[Temp4]    \n\t"
            "packrl.ph  %[Temp5],   %[Temp6],       %[Temp5]    \n\t"
            "dpax.w.ph  $ac1,       %[Temp1],       %[Temp4]    \n\t"
            "dpax.w.ph  $ac1,       %[Temp2],       %[Temp3]    \n\t"
            "dpax.w.ph  $ac3,       %[Temp1],       %[Temp5]    \n\t"
            "dpax.w.ph  $ac3,       %[Temp2],       %[Temp4]    \n\t"
            "bgtz       %[cond],    1b                          \n\t"
            " addiu     %[tmpH],    %[tmpH],        -8          \n\t"
          "2:                                                   \n\t"
            "extr_rs.w  %[Temp1],   $ac0,           15          \n\t"
            "extr_rs.w  %[Temp2],   $ac1,           15          \n\t"
            "extr_rs.w  %[Temp3],   $ac2,           15          \n\t"
            "extr_rs.w  %[Temp4],   $ac3,           15          \n\t"
            "sh         %[Temp1],   -8(%[y1])                   \n\t"
            "sh         %[Temp2],   -6(%[y1])                   \n\t"
            "sh         %[Temp3],   -4(%[y1])                   \n\t"
            "sh         %[Temp4],   -2(%[y1])                   \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [cond]  "=&r" (cond),  [tmpX]  "+r"  (tmpX),
              [tmpH]  "+r"  (tmpH),  [y1]    "+r"  (y1)
            : [X_end] "r" (X_end)
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi",
              "$ac3lo", "memory"
        );
    }
#elif defined(MIPS_DSP_R1_LE)
    for (n = 0; n < 64; n += 4)
    {
        tmpH = h + n - 4;
        tmpX = x;
        X_end = x + n;

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "lh         %[Temp1],   0(%[tmpX])                  \n\t"
            "lh         %[Temp2],   2(%[tmpX])                  \n\t"
            "lh         %[Temp3],   4(%[tmpX])                  \n\t"
            "lh         %[Temp4],   6(%[tmpX])                  \n\t"
            "lh         %[Temp5],   8(%[tmpH])                  \n\t"
            "lh         %[Temp6],   10(%[tmpH])                 \n\t"
            "lh         %[Temp7],   12(%[tmpH])                 \n\t"
            "lh         %[Temp8],   14(%[tmpH])                 \n\t"
            "subu       %[cond],    %[X_end],       %[tmpX]     \n\t"
            "mult       $ac0,       $0,             $0          \n\t"
            "mult       $ac0,       %[Temp1],       %[Temp5]    \n\t"
            "mult       $ac1,       $0,             $0          \n\t"
            "mult       $ac1,       %[Temp1],       %[Temp6]    \n\t"
            "madd       $ac1,       %[Temp2],       %[Temp5]    \n\t"
            "mult       $ac2,       $0,             $0          \n\t"
            "mult       $ac2,       %[Temp1],       %[Temp7]    \n\t"
            "madd       $ac2,       %[Temp2],       %[Temp6]    \n\t"
            "madd       $ac2,       %[Temp3],       %[Temp5]    \n\t"
            "mult       $ac3,       $0,             $0          \n\t"
            "mult       $ac3,       %[Temp1],       %[Temp8]    \n\t"
            "madd       $ac3,       %[Temp2],       %[Temp7]    \n\t"
            "madd       $ac3,       %[Temp3],       %[Temp6]    \n\t"
            "madd       $ac3,       %[Temp4],       %[Temp5]    \n\t"
            "blez       %[cond],    2f                          \n\t"
            " addiu     %[y1],      %[y1],          8           \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],   6(%[tmpH])                  \n\t"
            "lh         %[Temp2],   4(%[tmpH])                  \n\t"
            "lh         %[Temp3],   2(%[tmpH])                  \n\t"
            "lh         %[Temp4],   0(%[tmpH])                  \n\t"
            "lh         %[Temp5],   2(%[tmpX])                  \n\t"
            "lh         %[Temp6],   4(%[tmpX])                  \n\t"
            "lh         %[Temp7],   6(%[tmpX])                  \n\t"
            "lh         %[Temp8],   8(%[tmpX])                  \n\t"
            "addiu      %[cond],    %[cond],        -8          \n\t"
            "madd       $ac0,       %[Temp1],       %[Temp5]    \n\t"
            "madd       $ac0,       %[Temp2],       %[Temp6]    \n\t"
            "madd       $ac0,       %[Temp3],       %[Temp7]    \n\t"
            "madd       $ac0,       %[Temp4],       %[Temp8]    \n\t"
            "lh         %[Temp5],   10(%[tmpX])                 \n\t"
            "madd       $ac1,       %[Temp1],       %[Temp6]    \n\t"
            "madd       $ac1,       %[Temp2],       %[Temp7]    \n\t"
            "madd       $ac1,       %[Temp3],       %[Temp8]    \n\t"
            "madd       $ac1,       %[Temp4],       %[Temp5]    \n\t"
            "lh         %[Temp6],   12(%[tmpX])                 \n\t"
            "madd       $ac2,       %[Temp1],       %[Temp7]    \n\t"
            "madd       $ac2,       %[Temp2],       %[Temp8]    \n\t"
            "madd       $ac2,       %[Temp3],       %[Temp5]    \n\t"
            "madd       $ac2,       %[Temp4],       %[Temp6]    \n\t"
            "lh         %[Temp7],   14(%[tmpX])                 \n\t"
            "addiu      %[tmpX],    %[tmpX],        8           \n\t"
            "madd       $ac3,       %[Temp1],       %[Temp8]    \n\t"
            "madd       $ac3,       %[Temp2],       %[Temp5]    \n\t"
            "madd       $ac3,       %[Temp3],       %[Temp6]    \n\t"
            "madd       $ac3,       %[Temp4],       %[Temp7]    \n\t"
            "bgtz       %[cond],    1b                          \n\t"
            " addiu     %[tmpH],    %[tmpH],        -8          \n\t"
          "2:                                                   \n\t"
            "extr_rs.w  %[Temp1],   $ac0,           15          \n\t"
            "extr_rs.w  %[Temp2],   $ac1,           15          \n\t"
            "extr_rs.w  %[Temp3],   $ac2,           15          \n\t"
            "extr_rs.w  %[Temp4],   $ac3,           15          \n\t"
            "sh         %[Temp1],   -8(%[y1])                   \n\t"
            "sh         %[Temp2],   -6(%[y1])                   \n\t"
            "sh         %[Temp3],   -4(%[y1])                   \n\t"
            "sh         %[Temp4],   -2(%[y1])                   \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [cond]  "=&r" (cond),  [tmpX]  "+r"  (tmpX),
              [tmpH]  "+r"  (tmpH),  [y1]    "+r"  (y1)
            : [X_end] "r" (X_end)
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi",
              "$ac3lo", "memory"
        );
    }
#else
    for (n = 0; n < 64;)
    {
        tmpH = h + n;
        tmpX = x;
        X_end = x + n;

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "lh         %[Temp1],   0(%[tmpX])                  \n\t"
            "lh         %[Temp2],   0(%[tmpH])                  \n\t"
            "beq        %[tmpX],    %[X_end],       2f          \n\t"
            " mult      %[Temp1],   %[Temp2]                    \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],   2(%[tmpX])                  \n\t"
            "lh         %[Temp2],   -2(%[tmpH])                 \n\t"
            "lh         %[Temp3],   4(%[tmpX])                  \n\t"
            "lh         %[Temp4],   -4(%[tmpH])                 \n\t"
            "lh         %[Temp5],   6(%[tmpX])                  \n\t"
            "lh         %[Temp6],   -6(%[tmpH])                 \n\t"
            "lh         %[Temp7],   8(%[tmpX])                  \n\t"
            "lh         %[Temp8],   -8(%[tmpH])                 \n\t"
            "addiu      %[tmpX],    %[tmpX],        8           \n\t"
            "addiu      %[tmpH],    %[tmpH],        -8          \n\t"
            "madd       %[Temp1],   %[Temp2]                    \n\t"
            "madd       %[Temp3],   %[Temp4]                    \n\t"
            "madd       %[Temp5],   %[Temp6]                    \n\t"
            "bne        %[tmpX],    %[X_end],       1b          \n\t"
            " madd      %[Temp7],   %[Temp8]                    \n\t"
          "2:                                                   \n\t"
            "mflo       %[s]                                    \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [tmpX]  "+r"  (tmpX),  [tmpH]  "+r"  (tmpH),
              [s]     "=r"  (s)
            : [X_end] "r" (X_end)
            : "hi", "lo", "memory"
        );
        y[n] = ((s<<1) + 0x8000)>>16;

        tmpH = h + n + 1;
        tmpX = x;

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "lh         %[Temp1],   0(%[tmpX])                  \n\t"
            "lh         %[Temp2],   0(%[tmpH])                  \n\t"
            "lh         %[Temp3],   2(%[tmpX])                  \n\t"
            "lh         %[Temp4],   -2(%[tmpH])                 \n\t"
            "mult       %[Temp1],   %[Temp2]                    \n\t"
            "beq        %[tmpX],    %[X_end],       2f          \n\t"
            " madd      %[Temp3],   %[Temp4]                    \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],   4(%[tmpX])                  \n\t"
            "lh         %[Temp2],   -4(%[tmpH])                 \n\t"
            "lh         %[Temp3],   6(%[tmpX])                  \n\t"
            "lh         %[Temp4],   -6(%[tmpH])                 \n\t"
            "lh         %[Temp5],   8(%[tmpX])                  \n\t"
            "lh         %[Temp6],   -8(%[tmpH])                 \n\t"
            "lh         %[Temp7],   10(%[tmpX])                 \n\t"
            "lh         %[Temp8],   -10(%[tmpH])                \n\t"
            "addiu      %[tmpX],    %[tmpX],        8           \n\t"
            "addiu      %[tmpH],    %[tmpH],        -8          \n\t"
            "madd       %[Temp1],   %[Temp2]                    \n\t"
            "madd       %[Temp3],   %[Temp4]                    \n\t"
            "madd       %[Temp5],   %[Temp6]                    \n\t"
            "bne        %[tmpX],    %[X_end],       1b          \n\t"
            " madd      %[Temp7],   %[Temp8]                    \n\t"
          "2:                                                   \n\t"
            "mflo       %[s]                                    \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [tmpX]  "+r"  (tmpX),  [tmpH]  "+r"  (tmpH),
              [s]     "=r"  (s)
            : [X_end] "r" (X_end)
            : "hi", "lo", "memory"
        );
        y[n+1] = ((s<<1) + 0x8000)>>16;

        tmpH = h + n + 2;
        tmpX = x;

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "lh         %[Temp1],   0(%[tmpX])                  \n\t"
            "lh         %[Temp2],   0(%[tmpH])                  \n\t"
            "lh         %[Temp3],   2(%[tmpX])                  \n\t"
            "lh         %[Temp4],   -2(%[tmpH])                 \n\t"
            "lh         %[Temp5],   4(%[tmpX])                  \n\t"
            "lh         %[Temp6],   -4(%[tmpH])                 \n\t"
            "mult       %[Temp1],   %[Temp2]                    \n\t"
            "madd       %[Temp3],   %[Temp4]                    \n\t"
            "beq        %[tmpX],    %[X_end],       2f          \n\t"
            " madd      %[Temp5],   %[Temp6]                    \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],   6(%[tmpX])                  \n\t"
            "lh         %[Temp2],   -6(%[tmpH])                 \n\t"
            "lh         %[Temp3],   8(%[tmpX])                  \n\t"
            "lh         %[Temp4],   -8(%[tmpH])                 \n\t"
            "lh         %[Temp5],   10(%[tmpX])                 \n\t"
            "lh         %[Temp6],   -10(%[tmpH])                \n\t"
            "lh         %[Temp7],   12(%[tmpX])                 \n\t"
            "lh         %[Temp8],   -12(%[tmpH])                \n\t"
            "addiu      %[tmpX],    %[tmpX],        8           \n\t"
            "addiu      %[tmpH],    %[tmpH],        -8          \n\t"
            "madd       %[Temp1],   %[Temp2]                    \n\t"
            "madd       %[Temp3],   %[Temp4]                    \n\t"
            "madd       %[Temp5],   %[Temp6]                    \n\t"
            "bne        %[tmpX],    %[X_end],       1b          \n\t"
            " madd      %[Temp7],   %[Temp8]                    \n\t"
          "2:                                                   \n\t"
            "mflo       %[s]                                    \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [tmpX]  "+r"  (tmpX),  [tmpH]  "+r"  (tmpH),
              [s]     "=r"  (s)
            : [X_end] "r" (X_end)
            : "hi", "lo", "memory"
        );
        y[n+2] = ((s<<1) + 0x8000)>>16;

        tmpH = h + n + 3;
        tmpX = x;

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "lh         %[Temp1],   0(%[tmpX])                  \n\t"
            "lh         %[Temp2],   0(%[tmpH])                  \n\t"
            "lh         %[Temp3],   2(%[tmpX])                  \n\t"
            "lh         %[Temp4],   -2(%[tmpH])                 \n\t"
            "lh         %[Temp5],   4(%[tmpX])                  \n\t"
            "lh         %[Temp6],   -4(%[tmpH])                 \n\t"
            "lh         %[Temp7],   6(%[tmpX])                  \n\t"
            "lh         %[Temp8],   -6(%[tmpH])                 \n\t"
            "mult       %[Temp1],   %[Temp2]                    \n\t"
            "madd       %[Temp3],   %[Temp4]                    \n\t"
            "madd       %[Temp5],   %[Temp6]                    \n\t"
            "beq        %[tmpX],    %[X_end],       2f          \n\t"
            " madd      %[Temp7],   %[Temp8]                    \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],   8(%[tmpX])                  \n\t"
            "lh         %[Temp2],   -8(%[tmpH])                 \n\t"
            "lh         %[Temp3],   10(%[tmpX])                 \n\t"
            "lh         %[Temp4],   -10(%[tmpH])                \n\t"
            "lh         %[Temp5],   12(%[tmpX])                 \n\t"
            "lh         %[Temp6],   -12(%[tmpH])                \n\t"
            "lh         %[Temp7],   14(%[tmpX])                 \n\t"
            "lh         %[Temp8],   -14(%[tmpH])                \n\t"
            "addiu      %[tmpX],    %[tmpX],        8           \n\t"
            "addiu      %[tmpH],    %[tmpH],        -8          \n\t"
            "madd       %[Temp1],   %[Temp2]                    \n\t"
            "madd       %[Temp3],   %[Temp4]                    \n\t"
            "madd       %[Temp5],   %[Temp6]                    \n\t"
            "bne        %[tmpX],    %[X_end],       1b          \n\t"
            " madd      %[Temp7],   %[Temp8]                    \n\t"
          "2:                                                   \n\t"
            "mflo       %[s]                                    \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [tmpX]  "+r"  (tmpX),  [tmpH]  "+r"  (tmpH),
              [s]     "=r"  (s)
            : [X_end] "r" (X_end)
            : "hi", "lo", "memory"
        );
        y[n+3] = ((s<<1) + 0x8000)>>16;
        n+=4;
    }
#endif /* defined(MIPS_DSP_R2_LE) */
    return;
}



