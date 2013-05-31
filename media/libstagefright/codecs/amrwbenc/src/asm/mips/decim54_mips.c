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
*      File: decim54_mips.c                                            *
*                                                                      *
*      Description:Decimation of 16kHz signal to 12.8kHz               *
*                                                                      *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "acelp.h"
#include "cnst.h"

#define FAC5   5
#define DOWN_FAC  26215
#define NB_COEF_DOWN  15

static void Down_samp_mips(
        Word16 * sig,                         /* input:  signal to downsampling  */
        Word16 * sig_d,                       /* output: downsampled signal      */
        Word16 L_frame_d                      /* input:  length of output        */
        );

static Word16 fir_down1[4][30] =
{
    {-5, 24, -50, 54, 0, -128, 294, -408, 344, 0, -647, 1505, -2379, 3034, 13107, 3034, -2379, 1505, -647, 0, 344, -408,
    294, -128, 0, 54, -50, 24, -5, 0},

    {-6, 19, -26, 0, 77, -188, 270, -233, 0, 434, -964, 1366, -1293, 0, 12254, 6575, -2746, 1030, 0, -507, 601, -441,
    198, 0, -95, 99, -58, 18, 0, -1},

    {-3, 9, 0, -41, 111, -170, 153, 0, -295, 649, -888, 770, 0, -1997, 9894, 9894, -1997, 0, 770, -888, 649, -295, 0,
    153, -170, 111, -41, 0, 9, -3},

    {-1, 0, 18, -58, 99, -95, 0, 198, -441, 601, -507, 0, 1030, -2746, 6575, 12254, 0, -1293, 1366, -964, 434, 0,
    -233, 270, -188, 77, 0, -26, 19, -6}
};

void Decim_12k8_mips(
        Word16 sig16k[],                      /* input:  signal to downsampling  */
        Word16 lg,                            /* input:  length of input         */
        Word16 sig12k8[],                     /* output: decimated signal        */
        Word16 mem[]                          /* in/out: memory (2*NB_COEF_DOWN) */
        )
{
    Word16 lg_down;
    Word16 signal[L_FRAME16k + (2 * NB_COEF_DOWN)];

    memcpy(signal, mem, sizeof(Word16) * 2 * NB_COEF_DOWN);
    memcpy(signal + (2 * NB_COEF_DOWN), sig16k, sizeof(Word16) * lg);
    lg_down = (lg * DOWN_FAC)>>15;
    Down_samp_mips(signal + NB_COEF_DOWN, sig12k8, lg_down);
    memcpy (mem, signal + lg, sizeof(Word16) * 2 * NB_COEF_DOWN);

    return;
}

static void Down_samp_mips(
        Word16 * sig,                         /* input:  signal to downsampling  */
        Word16 * sig_d,                       /* output: downsampled signal      */
        Word16 L_frame_d                      /* input:  length of output        */
        )
{
    Word32 i, j, frac, pos;
    Word16 *x, *y;
    Word32 L_sum;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6;
#if !defined(MIPS_DSP_R2_LE)
    Word32 Temp7, Temp8, Temp9, Temp10, Temp11, Temp12;
#endif /* !defined(MIPS_DSP_R2_LE) */

    pos = 0;
    for (j = 0; j < L_frame_d; j++)
    {
        i = (pos >> 2);
        frac = pos & 3;
        x = sig + i - NB_COEF_DOWN + 1;
        y = (Word16 *)(fir_down1 + frac);
#if defined(MIPS_DSP_R2_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $ac0,       $0,         $0          \n\t"
            "ulw        %[Temp1],   0(%[x])                 \n\t"
            "ulw        %[Temp2],   0(%[y])                 \n\t"
            "ulw        %[Temp3],   4(%[x])                 \n\t"
            "ulw        %[Temp4],   4(%[y])                 \n\t"
            "ulw        %[Temp5],   8(%[x])                 \n\t"
            "ulw        %[Temp6],   8(%[y])                 \n\t"
            "dpa.w.ph   $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "ulw        %[Temp1],   12(%[x])                \n\t"
            "ulw        %[Temp2],   12(%[y])                \n\t"
            "ulw        %[Temp3],   16(%[x])                \n\t"
            "ulw        %[Temp4],   16(%[y])                \n\t"
            "ulw        %[Temp5],   20(%[x])                \n\t"
            "ulw        %[Temp6],   20(%[y])                \n\t"
            "dpa.w.ph   $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "ulw        %[Temp1],   24(%[x])                \n\t"
            "ulw        %[Temp2],   24(%[y])                \n\t"
            "ulw        %[Temp3],   28(%[x])                \n\t"
            "ulw        %[Temp4],   28(%[y])                \n\t"
            "ulw        %[Temp5],   32(%[x])                \n\t"
            "ulw        %[Temp6],   32(%[y])                \n\t"
            "dpa.w.ph   $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "ulw        %[Temp1],   36(%[x])                \n\t"
            "ulw        %[Temp2],   36(%[y])                \n\t"
            "ulw        %[Temp3],   40(%[x])                \n\t"
            "ulw        %[Temp4],   40(%[y])                \n\t"
            "ulw        %[Temp5],   44(%[x])                \n\t"
            "ulw        %[Temp6],   44(%[y])                \n\t"
            "dpa.w.ph   $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "ulw        %[Temp1],   48(%[x])                \n\t"
            "ulw        %[Temp2],   48(%[y])                \n\t"
            "ulw        %[Temp3],   52(%[x])                \n\t"
            "ulw        %[Temp4],   52(%[y])                \n\t"
            "ulw        %[Temp5],   56(%[x])                \n\t"
            "ulw        %[Temp6],   56(%[y])                \n\t"
            "dpa.w.ph   $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "extr_r.w   %[L_sum],   $ac0,       14          \n\t"
            "shll_s.w   %[L_sum],   %[L_sum],   16          \n\t"
            "sra        %[L_sum],   %[L_sum],   16          \n\t"

            ".set pop                                       \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [L_sum] "=&r" (L_sum)
            : [x] "r" (x), [y] "r" (y)
            : "hi", "lo", "memory"
        );
        sig_d[j] = L_sum;
#elif defined(MIPS_DSP_R1_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $ac0,       $0,         $0          \n\t"
            "lh         %[Temp1],   0(%[x])                 \n\t"
            "lh         %[Temp2],   0(%[y])                 \n\t"
            "lh         %[Temp3],   2(%[x])                 \n\t"
            "lh         %[Temp4],   2(%[y])                 \n\t"
            "lh         %[Temp5],   4(%[x])                 \n\t"
            "lh         %[Temp6],   4(%[y])                 \n\t"
            "lh         %[Temp7],   6(%[x])                 \n\t"
            "lh         %[Temp8],   6(%[y])                 \n\t"
            "lh         %[Temp9],   8(%[x])                 \n\t"
            "lh         %[Temp10],  8(%[y])                 \n\t"
            "lh         %[Temp11],  10(%[x])                \n\t"
            "lh         %[Temp12],  10(%[y])                \n\t"
            "madd       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "madd       $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "madd       $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "madd       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "madd       $ac0,       %[Temp9],   %[Temp10]   \n\t"
            "madd       $ac0,       %[Temp11],  %[Temp12]   \n\t"
            "lh         %[Temp1],   12(%[x])                \n\t"
            "lh         %[Temp2],   12(%[y])                \n\t"
            "lh         %[Temp3],   14(%[x])                \n\t"
            "lh         %[Temp4],   14(%[y])                \n\t"
            "lh         %[Temp5],   16(%[x])                \n\t"
            "lh         %[Temp6],   16(%[y])                \n\t"
            "lh         %[Temp7],   18(%[x])                \n\t"
            "lh         %[Temp8],   18(%[y])                \n\t"
            "lh         %[Temp9],   20(%[x])                \n\t"
            "lh         %[Temp10],  20(%[y])                \n\t"
            "lh         %[Temp11],  22(%[x])                \n\t"
            "lh         %[Temp12],  22(%[y])                \n\t"
            "madd       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "madd       $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "madd       $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "madd       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "madd       $ac0,       %[Temp9],   %[Temp10]   \n\t"
            "madd       $ac0,       %[Temp11],  %[Temp12]   \n\t"
            "lh         %[Temp1],   24(%[x])                \n\t"
            "lh         %[Temp2],   24(%[y])                \n\t"
            "lh         %[Temp3],   26(%[x])                \n\t"
            "lh         %[Temp4],   26(%[y])                \n\t"
            "lh         %[Temp5],   28(%[x])                \n\t"
            "lh         %[Temp6],   28(%[y])                \n\t"
            "lh         %[Temp7],   30(%[x])                \n\t"
            "lh         %[Temp8],   30(%[y])                \n\t"
            "lh         %[Temp9],   32(%[x])                \n\t"
            "lh         %[Temp10],  32(%[y])                \n\t"
            "lh         %[Temp11],  34(%[x])                \n\t"
            "lh         %[Temp12],  34(%[y])                \n\t"
            "madd       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "madd       $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "madd       $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "madd       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "madd       $ac0,       %[Temp9],   %[Temp10]   \n\t"
            "madd       $ac0,       %[Temp11],  %[Temp12]   \n\t"
            "lh         %[Temp1],   36(%[x])                \n\t"
            "lh         %[Temp2],   36(%[y])                \n\t"
            "lh         %[Temp3],   38(%[x])                \n\t"
            "lh         %[Temp4],   38(%[y])                \n\t"
            "lh         %[Temp5],   40(%[x])                \n\t"
            "lh         %[Temp6],   40(%[y])                \n\t"
            "lh         %[Temp7],   42(%[x])                \n\t"
            "lh         %[Temp8],   42(%[y])                \n\t"
            "lh         %[Temp9],   44(%[x])                \n\t"
            "lh         %[Temp10],  44(%[y])                \n\t"
            "lh         %[Temp11],  46(%[x])                \n\t"
            "lh         %[Temp12],  46(%[y])                \n\t"
            "madd       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "madd       $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "madd       $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "madd       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "madd       $ac0,       %[Temp9],   %[Temp10]   \n\t"
            "madd       $ac0,       %[Temp11],  %[Temp12]   \n\t"
            "lh         %[Temp1],   48(%[x])                \n\t"
            "lh         %[Temp2],   48(%[y])                \n\t"
            "lh         %[Temp3],   50(%[x])                \n\t"
            "lh         %[Temp4],   50(%[y])                \n\t"
            "lh         %[Temp5],   52(%[x])                \n\t"
            "lh         %[Temp6],   52(%[y])                \n\t"
            "lh         %[Temp7],   54(%[x])                \n\t"
            "lh         %[Temp8],   54(%[y])                \n\t"
            "lh         %[Temp9],   56(%[x])                \n\t"
            "lh         %[Temp10],  56(%[y])                \n\t"
            "lh         %[Temp11],  58(%[x])                \n\t"
            "lh         %[Temp12],  58(%[y])                \n\t"
            "madd       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "madd       $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "madd       $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "madd       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "madd       $ac0,       %[Temp9],   %[Temp10]   \n\t"
            "madd       $ac0,       %[Temp11],  %[Temp12]   \n\t"
            "mflo       %[L_sum],   $ac0                    \n\t"
            "shra_r.w   %[L_sum],   %[L_sum],   14          \n\t"
            "shll_s.w   %[L_sum],   %[L_sum],   16          \n\t"
            "sra        %[L_sum],   %[L_sum],   16          \n\t"

            ".set pop                                       \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
              [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
              [L_sum]  "=&r" (L_sum)
            : [x] "r" (x), [y] "r" (y)
            : "hi", "lo", "memory"
        );
        sig_d[j] = L_sum;
#else
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $0,         $0                      \n\t"
            "lh         %[Temp1],   0(%[x])                 \n\t"
            "lh         %[Temp2],   0(%[y])                 \n\t"
            "lh         %[Temp3],   2(%[x])                 \n\t"
            "lh         %[Temp4],   2(%[y])                 \n\t"
            "lh         %[Temp5],   4(%[x])                 \n\t"
            "lh         %[Temp6],   4(%[y])                 \n\t"
            "lh         %[Temp7],   6(%[x])                 \n\t"
            "lh         %[Temp8],   6(%[y])                 \n\t"
            "lh         %[Temp9],   8(%[x])                 \n\t"
            "lh         %[Temp10],  8(%[y])                 \n\t"
            "lh         %[Temp11],  10(%[x])                \n\t"
            "lh         %[Temp12],  10(%[y])                \n\t"
            "madd       %[Temp1],   %[Temp2]                \n\t"
            "madd       %[Temp3],   %[Temp4]                \n\t"
            "madd       %[Temp5],   %[Temp6]                \n\t"
            "madd       %[Temp7],   %[Temp8]                \n\t"
            "madd       %[Temp9],   %[Temp10]               \n\t"
            "madd       %[Temp11],  %[Temp12]               \n\t"
            "lh         %[Temp1],   12(%[x])                \n\t"
            "lh         %[Temp2],   12(%[y])                \n\t"
            "lh         %[Temp3],   14(%[x])                \n\t"
            "lh         %[Temp4],   14(%[y])                \n\t"
            "lh         %[Temp5],   16(%[x])                \n\t"
            "lh         %[Temp6],   16(%[y])                \n\t"
            "lh         %[Temp7],   18(%[x])                \n\t"
            "lh         %[Temp8],   18(%[y])                \n\t"
            "lh         %[Temp9],   20(%[x])                \n\t"
            "lh         %[Temp10],  20(%[y])                \n\t"
            "lh         %[Temp11],  22(%[x])                \n\t"
            "lh         %[Temp12],  22(%[y])                \n\t"
            "madd       %[Temp1],   %[Temp2]                \n\t"
            "madd       %[Temp3],   %[Temp4]                \n\t"
            "madd       %[Temp5],   %[Temp6]                \n\t"
            "madd       %[Temp7],   %[Temp8]                \n\t"
            "madd       %[Temp9],   %[Temp10]               \n\t"
            "madd       %[Temp11],  %[Temp12]               \n\t"
            "lh         %[Temp1],   24(%[x])                \n\t"
            "lh         %[Temp2],   24(%[y])                \n\t"
            "lh         %[Temp3],   26(%[x])                \n\t"
            "lh         %[Temp4],   26(%[y])                \n\t"
            "lh         %[Temp5],   28(%[x])                \n\t"
            "lh         %[Temp6],   28(%[y])                \n\t"
            "lh         %[Temp7],   30(%[x])                \n\t"
            "lh         %[Temp8],   30(%[y])                \n\t"
            "lh         %[Temp9],   32(%[x])                \n\t"
            "lh         %[Temp10],  32(%[y])                \n\t"
            "lh         %[Temp11],  34(%[x])                \n\t"
            "lh         %[Temp12],  34(%[y])                \n\t"
            "madd       %[Temp1],   %[Temp2]                \n\t"
            "madd       %[Temp3],   %[Temp4]                \n\t"
            "madd       %[Temp5],   %[Temp6]                \n\t"
            "madd       %[Temp7],   %[Temp8]                \n\t"
            "madd       %[Temp9],   %[Temp10]               \n\t"
            "madd       %[Temp11],  %[Temp12]               \n\t"
            "lh         %[Temp1],   36(%[x])                \n\t"
            "lh         %[Temp2],   36(%[y])                \n\t"
            "lh         %[Temp3],   38(%[x])                \n\t"
            "lh         %[Temp4],   38(%[y])                \n\t"
            "lh         %[Temp5],   40(%[x])                \n\t"
            "lh         %[Temp6],   40(%[y])                \n\t"
            "lh         %[Temp7],   42(%[x])                \n\t"
            "lh         %[Temp8],   42(%[y])                \n\t"
            "lh         %[Temp9],   44(%[x])                \n\t"
            "lh         %[Temp10],  44(%[y])                \n\t"
            "lh         %[Temp11],  46(%[x])                \n\t"
            "lh         %[Temp12],  46(%[y])                \n\t"
            "madd       %[Temp1],   %[Temp2]                \n\t"
            "madd       %[Temp3],   %[Temp4]                \n\t"
            "madd       %[Temp5],   %[Temp6]                \n\t"
            "madd       %[Temp7],   %[Temp8]                \n\t"
            "madd       %[Temp9],   %[Temp10]               \n\t"
            "madd       %[Temp11],  %[Temp12]               \n\t"
            "lh         %[Temp1],   48(%[x])                \n\t"
            "lh         %[Temp2],   48(%[y])                \n\t"
            "lh         %[Temp3],   50(%[x])                \n\t"
            "lh         %[Temp4],   50(%[y])                \n\t"
            "lh         %[Temp5],   52(%[x])                \n\t"
            "lh         %[Temp6],   52(%[y])                \n\t"
            "lh         %[Temp7],   54(%[x])                \n\t"
            "lh         %[Temp8],   54(%[y])                \n\t"
            "lh         %[Temp9],   56(%[x])                \n\t"
            "lh         %[Temp10],  56(%[y])                \n\t"
            "lh         %[Temp11],  58(%[x])                \n\t"
            "lh         %[Temp12],  58(%[y])                \n\t"
            "madd       %[Temp1],   %[Temp2]                \n\t"
            "madd       %[Temp3],   %[Temp4]                \n\t"
            "madd       %[Temp5],   %[Temp6]                \n\t"
            "madd       %[Temp7],   %[Temp8]                \n\t"
            "madd       %[Temp9],   %[Temp10]               \n\t"
            "madd       %[Temp11],  %[Temp12]               \n\t"
            "mflo       %[L_sum]                            \n\t"

            ".set pop                                       \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
              [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
              [L_sum]  "=&r" (L_sum)
            : [x] "r" (x), [y] "r" (y)
            : "hi", "lo", "memory"
        );
        L_sum = L_shl2(L_sum, 2);
        sig_d[j] = extract_h(L_add(L_sum, 0x8000));
#endif /* defined(MIPS_DSP_R2_LE) */
        pos += FAC5;
    }
    return;
}


