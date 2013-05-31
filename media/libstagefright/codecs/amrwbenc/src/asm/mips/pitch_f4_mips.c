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
*      File: pitch_f4_mips.c                                           *
*                                                                      *
*      Description: Find the closed loop pitch period with             *
*               1/4 subsample resolution.                              *
*                                                                      *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "math_op.h"
#include "acelp.h"
#include "cnst.h"

#define UP_SAMP      4
#define L_INTERPOL1  4

/* Local functions */

static void Norm_Corr(
        Word16 exc[],                         /* (i)     : excitation buffer                     */
        Word16 xn[],                          /* (i)     : target vector                         */
        Word16 h[],                           /* (i) Q15 : impulse response of synth/wgt filters */
        Word16 L_subfr,
        Word16 t_min,                         /* (i)     : minimum value of pitch lag.           */
        Word16 t_max,                         /* (i)     : maximum value of pitch lag.           */
        Word16 corr_norm[]                    /* (o) Q15 : normalized correlation                */
        );

static Word16 Interpol_4(                  /* (o)  : interpolated value  */
        Word16 * x,                           /* (i)  : input vector        */
        Word32 frac                           /* (i)  : fraction (-4..+3)   */
        );


Word16 Pitch_fr4_mips(                          /* (o)     : pitch period.                         */
        Word16 exc[],                         /* (i)     : excitation buffer                     */
        Word16 xn[],                          /* (i)     : target vector                         */
        Word16 h[],                           /* (i) Q15 : impulse response of synth/wgt filters */
        Word16 t0_min,                        /* (i)     : minimum value in the searched range.  */
        Word16 t0_max,                        /* (i)     : maximum value in the searched range.  */
        Word16 * pit_frac,                    /* (o)     : chosen fraction (0, 1, 2 or 3).       */
        Word16 i_subfr,                       /* (i)     : indicator for first subframe.         */
        Word16 t0_fr2,                        /* (i)     : minimum value for resolution 1/2      */
        Word16 t0_fr1,                        /* (i)     : minimum value for resolution 1        */
        Word16 L_subfr                        /* (i)     : Length of subframe                    */
        )
{
    Word32 fraction, i;
    Word16 t_min, t_max;
    Word16 max, t0, step, temp;
    Word16 *corr;
    Word16 corr_v[40];                     /* Total length = t0_max-t0_min+1+2*L_inter */

    /* Find interval to compute normalized correlation */

    t_min = t0_min - L_INTERPOL1;
    t_max = t0_max + L_INTERPOL1;
    corr = &corr_v[-t_min];
    /* Compute normalized correlation between target and filtered excitation */
    Norm_Corr(exc, xn, h, L_subfr, t_min, t_max, corr);

    /* Find integer pitch */

    max = corr[t0_min];
    t0 = t0_min;
    for (i = t0_min + 1; i <= t0_max; i++)
    {
        if (corr[i] >= max)
        {
            max = corr[i];
            t0 = i;
        }
    }
    /* If first subframe and t0 >= t0_fr1, do not search fractionnal pitch */
    if ((i_subfr == 0) && (t0 >= t0_fr1))
    {
        *pit_frac = 0;
        return (t0);
    }
    /*------------------------------------------------------------------*
    * Search fractionnal pitch with 1/4 subsample resolution.          *
    * Test the fractions around t0 and choose the one which maximizes  *
    * the interpolated normalized correlation.                         *
    *------------------------------------------------------------------*/

    step = 1;               /* 1/4 subsample resolution */
    fraction = -3;
    if ((t0_fr2 == PIT_MIN)||((i_subfr == 0) && (t0 >= t0_fr2)))
    {
        step = 2;              /* 1/2 subsample resolution */
        fraction = -2;
    }
    if(t0 == t0_min)
    {
        fraction = 0;
    }
    max = Interpol_4(&corr[t0], fraction);

    for (i = fraction + step; i <= 3; i += step)
    {
        temp = Interpol_4(&corr[t0], i);
        if(temp > max)
        {
            max = temp;
            fraction = i;
        }
    }
    /* limit the fraction value in the interval [0,1,2,3] */
    if (fraction < 0)
    {
        fraction += UP_SAMP;
        t0 -= 1;
    }
    *pit_frac = fraction;
    return (t0);
}


/***********************************************************************************
* Function:  Norm_Corr()                                                            *
*                                                                                   *
* Description: Find the normalized correlation between the target vector and the    *
* filtered past excitation.                                                         *
* (correlation between target and filtered excitation divided by the                *
*  square root of energy of target and filtered excitation).                        *
************************************************************************************/
static void Norm_Corr(
        Word16 exc[],                         /* (i)     : excitation buffer                     */
        Word16 xn[],                          /* (i)     : target vector                         */
        Word16 h[],                           /* (i) Q15 : impulse response of synth/wgt filters */
        Word16 L_subfr,
        Word16 t_min,                         /* (i)     : minimum value of pitch lag.           */
        Word16 t_max,                         /* (i)     : maximum value of pitch lag.           */
        Word16 corr_norm[])                   /* (o) Q15 : normalized correlation                */
{
    Word32 i, k, t;
    Word32 corr, exp_corr, norm, exp, scale;
    Word16 exp_norm, excf[L_SUBFR], tmp;
    Word32 L_tmp, L_tmp1, L_tmp2;
    Word16 *ptr1, *ptr2, *ptr_end;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8;
#if !defined(MIPS_DSP_R1_LE)
    Word32 Temp9, Temp10, Temp11, Temp12;
#endif /* !defined(MIPS_DSP_R1_LE) */

    /* compute the filtered excitation for the first delay t_min */
    k = -t_min;
    Convolve_mips(&exc[k], h, excf, 64);

    /* Compute rounded down 1/sqrt(energy of xn[]) */
    ptr1 = xn;
    ptr_end = xn + 64;
#if defined(MIPS_DSP_R2_LE)
    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "mult       $ac0,       $0,         $0          \n\t"
      "1:                                               \n\t"
        "ulw        %[Temp1],   0(%[ptr1])              \n\t"
        "ulw        %[Temp2],   4(%[ptr1])              \n\t"
        "ulw        %[Temp3],   8(%[ptr1])              \n\t"
        "ulw        %[Temp4],   12(%[ptr1])             \n\t"
        "addiu      %[ptr1],    %[ptr1],    16          \n\t"
        "dpa.w.ph   $ac0,       %[Temp1],   %[Temp1]    \n\t"
        "dpa.w.ph   $ac0,       %[Temp2],   %[Temp2]    \n\t"
        "dpa.w.ph   $ac0,       %[Temp3],   %[Temp3]    \n\t"
        "bne        %[ptr1],    %[ptr_end], 1b          \n\t"
        " dpa.w.ph  $ac0,       %[Temp4],   %[Temp4]    \n\t"
        "mflo       %[L_tmp],   $ac0                    \n\t"
        "sll        %[L_tmp],   %[L_tmp],   1           \n\t"
        "addiu      %[L_tmp],   %[L_tmp],   1           \n\t"
        "clz        %[exp],     %[L_tmp]                \n\t"
        "addiu      %[exp],     %[exp],     -33         \n\t"
        "negu       %[exp],     %[exp]                  \n\t"
        "sra        %[scale],   %[exp],     1           \n\t"
        "negu       %[scale],   %[scale]                \n\t"

        ".set pop                                       \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [L_tmp] "=&r" (L_tmp), [scale] "=&r" (scale),
          [exp]   "=&r" (exp),   [ptr1]  "+r"  (ptr1)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "memory"
    );
#elif defined(MIPS_DSP_R1_LE)
    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "mult       $ac0,       $0,         $0          \n\t"
      "1:                                               \n\t"
        "lh         %[Temp1],   0(%[ptr1])              \n\t"
        "lh         %[Temp2],   2(%[ptr1])              \n\t"
        "lh         %[Temp3],   4(%[ptr1])              \n\t"
        "lh         %[Temp4],   6(%[ptr1])              \n\t"
        "madd       $ac0,       %[Temp1],   %[Temp1]    \n\t"
        "madd       $ac0,       %[Temp2],   %[Temp2]    \n\t"
        "madd       $ac0,       %[Temp3],   %[Temp3]    \n\t"
        "madd       $ac0,       %[Temp4],   %[Temp4]    \n\t"
        "lh         %[Temp1],   8(%[ptr1])              \n\t"
        "lh         %[Temp2],   10(%[ptr1])             \n\t"
        "lh         %[Temp3],   12(%[ptr1])             \n\t"
        "lh         %[Temp4],   14(%[ptr1])             \n\t"
        "addiu      %[ptr1],    %[ptr1],    16          \n\t"
        "madd       $ac0,       %[Temp1],   %[Temp1]    \n\t"
        "madd       $ac0,       %[Temp2],   %[Temp2]    \n\t"
        "madd       $ac0,       %[Temp3],   %[Temp3]    \n\t"
        "bne        %[ptr1],    %[ptr_end], 1b          \n\t"
        " madd      $ac0,       %[Temp4],   %[Temp4]    \n\t"
        "mflo       %[L_tmp],   $ac0                    \n\t"
        "sll        %[L_tmp],   %[L_tmp],   1           \n\t"
        "addiu      %[L_tmp],   %[L_tmp],   1           \n\t"
        "clz        %[exp],     %[L_tmp]                \n\t"
        "addiu      %[exp],     %[exp],     -33         \n\t"
        "negu       %[exp],     %[exp]                  \n\t"
        "sra        %[scale],   %[exp],     1           \n\t"
        "negu       %[scale],   %[scale]                \n\t"

        ".set pop                                       \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [L_tmp] "=&r" (L_tmp), [scale] "=&r" (scale),
          [exp]   "=&r" (exp),   [ptr1]  "+r"  (ptr1)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "memory"
    );
#else
    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "mult       $0,         $0                      \n\t"
      "1:                                               \n\t"
        "lh         %[Temp1],   0(%[ptr1])              \n\t"
        "lh         %[Temp2],   2(%[ptr1])              \n\t"
        "lh         %[Temp3],   4(%[ptr1])              \n\t"
        "lh         %[Temp4],   6(%[ptr1])              \n\t"
        "madd       %[Temp1],   %[Temp1]                \n\t"
        "madd       %[Temp2],   %[Temp2]                \n\t"
        "madd       %[Temp3],   %[Temp3]                \n\t"
        "madd       %[Temp4],   %[Temp4]                \n\t"
        "lh         %[Temp1],   8(%[ptr1])              \n\t"
        "lh         %[Temp2],   10(%[ptr1])             \n\t"
        "lh         %[Temp3],   12(%[ptr1])             \n\t"
        "lh         %[Temp4],   14(%[ptr1])             \n\t"
        "addiu      %[ptr1],    %[ptr1],    16          \n\t"
        "madd       %[Temp1],   %[Temp1]                \n\t"
        "madd       %[Temp2],   %[Temp2]                \n\t"
        "madd       %[Temp3],   %[Temp3]                \n\t"
        "bne        %[ptr1],    %[ptr_end], 1b          \n\t"
        " madd      %[Temp4],   %[Temp4]                \n\t"
        "mflo       %[L_tmp]                            \n\t"
        "sll        %[L_tmp],   %[L_tmp],   1           \n\t"
        "addiu      %[L_tmp],   %[L_tmp],   1           \n\t"
        "clz        %[exp],     %[L_tmp]                \n\t"
        "addiu      %[exp],     %[exp],     -33         \n\t"
        "negu       %[exp],     %[exp]                  \n\t"
        "sra        %[scale],   %[exp],     1           \n\t"
        "negu       %[scale],   %[scale]                \n\t"

        ".set pop                                       \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
          [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
          [L_tmp] "=&r" (L_tmp), [scale] "=&r" (scale),
          [exp]   "=&r" (exp),   [ptr1]  "+r"  (ptr1)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "memory"
    );
#endif /* defined(MIPS_DSP_R2_LE) */

    /* loop for every possible period */
    for (t = t_min; t <= t_max; t++)
    {
        /* Compute correlation between xn[] and excf[] */
        ptr1 = xn;
        ptr2 = excf;
        ptr_end = xn + 64;
#if defined(MIPS_DSP_R2_LE)
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult       $ac0,           $0,         $0          \n\t"
            "mult       $ac1,           $0,         $0          \n\t"
          "1:                                                   \n\t"
            "ulw        %[Temp1],       0(%[ptr1])              \n\t"
            "ulw        %[Temp2],       0(%[ptr2])              \n\t"
            "ulw        %[Temp3],       4(%[ptr1])              \n\t"
            "ulw        %[Temp4],       4(%[ptr2])              \n\t"
            "ulw        %[Temp5],       8(%[ptr1])              \n\t"
            "ulw        %[Temp6],       8(%[ptr2])              \n\t"
            "ulw        %[Temp7],       12(%[ptr1])             \n\t"
            "ulw        %[Temp8],       12(%[ptr2])             \n\t"
            "addiu      %[ptr1],        %[ptr1],    16          \n\t"
            "addiu      %[ptr2],        %[ptr2],    16          \n\t"
            "dpa.w.ph   $ac0,           %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac0,           %[Temp3],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac0,           %[Temp5],   %[Temp6]    \n\t"
            "dpa.w.ph   $ac0,           %[Temp7],   %[Temp8]    \n\t"
            "dpa.w.ph   $ac1,           %[Temp2],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac1,           %[Temp4],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac1,           %[Temp6],   %[Temp6]    \n\t"
            "bne        %[ptr1],        %[ptr_end], 1b          \n\t"
            " dpa.w.ph  $ac1,           %[Temp8],   %[Temp8]    \n\t"
            "mflo       %[L_tmp],       $ac0                    \n\t"
            "mflo       %[L_tmp1],      $ac1                    \n\t"
            "sll        %[L_tmp],       %[L_tmp],   1           \n\t"
            "sll        %[L_tmp1],      %[L_tmp1],  1           \n\t"
            "addiu      %[L_tmp],       %[L_tmp],   1           \n\t"
            "addiu      %[L_tmp1],      %[L_tmp1],  1           \n\t"
            "sra        %[Temp1],       %[L_tmp],   31          \n\t"
            "sra        %[Temp2],       %[L_tmp1],  31          \n\t"
            "xor        %[Temp1],       %[Temp1],   %[L_tmp]    \n\t"
            "xor        %[Temp2],       %[Temp2],   %[L_tmp1]   \n\t"
            "clz        %[Temp1],       %[Temp1]                \n\t"
            "clz        %[Temp2],       %[Temp2]                \n\t"
            "addiu      %[Temp1],       %[Temp1],   -1          \n\t"
            "addiu      %[Temp2],       %[Temp2],   -1          \n\t"
            "sllv       %[L_tmp],       %[L_tmp],   %[Temp1]    \n\t"
            "sllv       %[L_tmp1],      %[L_tmp1],  %[Temp2]    \n\t"
            "addiu      %[Temp1],       %[Temp1],   -30         \n\t"
            "addiu      %[Temp2],       %[Temp2],   -30         \n\t"
            "negu       %[exp_corr],    %[Temp1]                \n\t"
            "negu       %[exp_norm],    %[Temp2]                \n\t"

            ".set pop                                           \n\t"

            : [Temp1]    "=&r" (Temp1),    [Temp2]    "=&r" (Temp2),
              [Temp3]    "=&r" (Temp3),    [Temp4]    "=&r" (Temp4),
              [Temp5]    "=&r" (Temp5),    [Temp6]    "=&r" (Temp6),
              [Temp7]    "=&r" (Temp7),    [Temp8]    "=&r" (Temp8),
              [L_tmp]    "=&r" (L_tmp),    [L_tmp1]   "=&r" (L_tmp1),
              [exp_corr] "=r"  (exp_corr), [exp_norm] "=r"  (exp_norm),
              [ptr1]     "+r"  (ptr1),     [ptr2]     "+r"  (ptr2)
            : [ptr_end] "r" (ptr_end)
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "memory"
        );
#elif defined(MIPS_DSP_R1_LE)
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult       $ac0,           $0,         $0          \n\t"
            "mult       $ac1,           $0,         $0          \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],       0(%[ptr1])              \n\t"
            "lh         %[Temp2],       0(%[ptr2])              \n\t"
            "lh         %[Temp3],       2(%[ptr1])              \n\t"
            "lh         %[Temp4],       2(%[ptr2])              \n\t"
            "lh         %[Temp5],       4(%[ptr1])              \n\t"
            "lh         %[Temp6],       4(%[ptr2])              \n\t"
            "lh         %[Temp7],       6(%[ptr1])              \n\t"
            "lh         %[Temp8],       6(%[ptr2])              \n\t"
            "madd       $ac0,           %[Temp1],   %[Temp2]    \n\t"
            "madd       $ac0,           %[Temp3],   %[Temp4]    \n\t"
            "madd       $ac0,           %[Temp5],   %[Temp6]    \n\t"
            "madd       $ac0,           %[Temp7],   %[Temp8]    \n\t"
            "madd       $ac1,           %[Temp2],   %[Temp2]    \n\t"
            "madd       $ac1,           %[Temp4],   %[Temp4]    \n\t"
            "madd       $ac1,           %[Temp6],   %[Temp6]    \n\t"
            "madd       $ac1,           %[Temp8],   %[Temp8]    \n\t"
            "lh         %[Temp1],       8(%[ptr1])              \n\t"
            "lh         %[Temp2],       8(%[ptr2])              \n\t"
            "lh         %[Temp3],       10(%[ptr1])             \n\t"
            "lh         %[Temp4],       10(%[ptr2])             \n\t"
            "lh         %[Temp5],       12(%[ptr1])             \n\t"
            "lh         %[Temp6],       12(%[ptr2])             \n\t"
            "lh         %[Temp7],       14(%[ptr1])             \n\t"
            "lh         %[Temp8],       14(%[ptr2])             \n\t"
            "addiu      %[ptr1],        %[ptr1],    16          \n\t"
            "addiu      %[ptr2],        %[ptr2],    16          \n\t"
            "madd       $ac0,           %[Temp1],   %[Temp2]    \n\t"
            "madd       $ac0,           %[Temp3],   %[Temp4]    \n\t"
            "madd       $ac0,           %[Temp5],   %[Temp6]    \n\t"
            "madd       $ac0,           %[Temp7],   %[Temp8]    \n\t"
            "madd       $ac1,           %[Temp2],   %[Temp2]    \n\t"
            "madd       $ac1,           %[Temp4],   %[Temp4]    \n\t"
            "madd       $ac1,           %[Temp6],   %[Temp6]    \n\t"
            "bne        %[ptr1],        %[ptr_end], 1b          \n\t"
            " madd      $ac1,           %[Temp8],   %[Temp8]    \n\t"
            "mflo       %[L_tmp],       $ac0                    \n\t"
            "mflo       %[L_tmp1],      $ac1                    \n\t"
            "sll        %[L_tmp],       %[L_tmp],   1           \n\t"
            "sll        %[L_tmp1],      %[L_tmp1],  1           \n\t"
            "addiu      %[L_tmp],       %[L_tmp],   1           \n\t"
            "addiu      %[L_tmp1],      %[L_tmp1],  1           \n\t"
            "sra        %[Temp1],       %[L_tmp],   31          \n\t"
            "sra        %[Temp2],       %[L_tmp1],  31          \n\t"
            "xor        %[Temp1],       %[Temp1],   %[L_tmp]    \n\t"
            "xor        %[Temp2],       %[Temp2],   %[L_tmp1]   \n\t"
            "clz        %[Temp1],       %[Temp1]                \n\t"
            "clz        %[Temp2],       %[Temp2]                \n\t"
            "addiu      %[Temp1],       %[Temp1],   -1          \n\t"
            "addiu      %[Temp2],       %[Temp2],   -1          \n\t"
            "sllv       %[L_tmp],       %[L_tmp],   %[Temp1]    \n\t"
            "sllv       %[L_tmp1],      %[L_tmp1],  %[Temp2]    \n\t"
            "addiu      %[Temp1],       %[Temp1],   -30         \n\t"
            "addiu      %[Temp2],       %[Temp2],   -30         \n\t"
            "negu       %[exp_corr],    %[Temp1]                \n\t"
            "negu       %[exp_norm],    %[Temp2]                \n\t"

            ".set pop                                           \n\t"

            : [Temp1]    "=&r" (Temp1),    [Temp2]    "=&r" (Temp2),
              [Temp3]    "=&r" (Temp3),    [Temp4]    "=&r" (Temp4),
              [Temp5]    "=&r" (Temp5),    [Temp6]    "=&r" (Temp6),
              [Temp7]    "=&r" (Temp7),    [Temp8]    "=&r" (Temp8),
              [exp_corr] "=r"  (exp_corr), [exp_norm] "=r"  (exp_norm),
              [L_tmp]    "=r"  (L_tmp),    [L_tmp1]   "=r"  (L_tmp1),
              [ptr1]     "+r"  (ptr1),     [ptr2]     "+r"  (ptr2)
            : [ptr_end] "r" (ptr_end)
            : "hi", "lo", "memory"
        );
#else
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "li         %[L_tmp1],      0                       \n\t"
            "mult       $0,             $0                      \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],       0(%[ptr1])              \n\t"
            "lh         %[Temp2],       0(%[ptr2])              \n\t"
            "lh         %[Temp3],       2(%[ptr1])              \n\t"
            "lh         %[Temp4],       2(%[ptr2])              \n\t"
            "lh         %[Temp5],       4(%[ptr1])              \n\t"
            "lh         %[Temp6],       4(%[ptr2])              \n\t"
            "lh         %[Temp7],       6(%[ptr1])              \n\t"
            "lh         %[Temp8],       6(%[ptr2])              \n\t"
            "mul        %[Temp9],       %[Temp2],   %[Temp2]    \n\t"
            "mul        %[Temp10],      %[Temp4],   %[Temp4]    \n\t"
            "mul        %[Temp11],      %[Temp6],   %[Temp6]    \n\t"
            "mul        %[Temp12],      %[Temp8],   %[Temp8]    \n\t"
            "madd       %[Temp1],       %[Temp2]                \n\t"
            "madd       %[Temp3],       %[Temp4]                \n\t"
            "madd       %[Temp5],       %[Temp6]                \n\t"
            "madd       %[Temp7],       %[Temp8]                \n\t"
            "addu       %[L_tmp1],      %[L_tmp1],  %[Temp9]    \n\t"
            "addu       %[L_tmp1],      %[L_tmp1],  %[Temp10]   \n\t"
            "addu       %[L_tmp1],      %[L_tmp1],  %[Temp11]   \n\t"
            "addu       %[L_tmp1],      %[L_tmp1],  %[Temp12]   \n\t"
            "lh         %[Temp1],       8(%[ptr1])              \n\t"
            "lh         %[Temp2],       8(%[ptr2])              \n\t"
            "lh         %[Temp3],       10(%[ptr1])             \n\t"
            "lh         %[Temp4],       10(%[ptr2])             \n\t"
            "lh         %[Temp5],       12(%[ptr1])             \n\t"
            "lh         %[Temp6],       12(%[ptr2])             \n\t"
            "lh         %[Temp7],       14(%[ptr1])             \n\t"
            "lh         %[Temp8],       14(%[ptr2])             \n\t"
            "addiu      %[ptr1],        %[ptr1],    16          \n\t"
            "addiu      %[ptr2],        %[ptr2],    16          \n\t"
            "mul        %[Temp9],       %[Temp2],   %[Temp2]    \n\t"
            "mul        %[Temp10],      %[Temp4],   %[Temp4]    \n\t"
            "mul        %[Temp11],      %[Temp6],   %[Temp6]    \n\t"
            "mul        %[Temp12],      %[Temp8],   %[Temp8]    \n\t"
            "madd       %[Temp1],       %[Temp2]                \n\t"
            "madd       %[Temp3],       %[Temp4]                \n\t"
            "madd       %[Temp5],       %[Temp6]                \n\t"
            "madd       %[Temp7],       %[Temp8]                \n\t"
            "addu       %[L_tmp1],      %[L_tmp1],  %[Temp9]    \n\t"
            "addu       %[L_tmp1],      %[L_tmp1],  %[Temp10]   \n\t"
            "addu       %[L_tmp1],      %[L_tmp1],  %[Temp11]   \n\t"
            "bne        %[ptr1],        %[ptr_end], 1b          \n\t"
            " addu      %[L_tmp1],      %[L_tmp1],  %[Temp12]   \n\t"
            "mflo       %[L_tmp]                                \n\t"
            "sll        %[L_tmp],       %[L_tmp],   1           \n\t"
            "sll        %[L_tmp1],      %[L_tmp1],  1           \n\t"
            "addiu      %[L_tmp],       %[L_tmp],   1           \n\t"
            "addiu      %[L_tmp1],      %[L_tmp1],  1           \n\t"
            "sra        %[Temp1],       %[L_tmp],   31          \n\t"
            "sra        %[Temp2],       %[L_tmp1],  31          \n\t"
            "xor        %[Temp1],       %[Temp1],   %[L_tmp]    \n\t"
            "xor        %[Temp2],       %[Temp2],   %[L_tmp1]   \n\t"
            "clz        %[Temp1],       %[Temp1]                \n\t"
            "clz        %[Temp2],       %[Temp2]                \n\t"
            "addiu      %[Temp1],       %[Temp1],   -1          \n\t"
            "addiu      %[Temp2],       %[Temp2],   -1          \n\t"
            "sllv       %[L_tmp],       %[L_tmp],   %[Temp1]    \n\t"
            "sllv       %[L_tmp1],      %[L_tmp1],  %[Temp2]    \n\t"
            "addiu      %[Temp1],       %[Temp1],   -30         \n\t"
            "addiu      %[Temp2],       %[Temp2],   -30         \n\t"
            "negu       %[exp_corr],    %[Temp1]                \n\t"
            "negu       %[exp_norm],    %[Temp2]                \n\t"

            ".set pop                                           \n\t"

            : [Temp1]    "=&r" (Temp1),    [Temp2]    "=&r" (Temp2),
              [Temp3]    "=&r" (Temp3),    [Temp4]    "=&r" (Temp4),
              [Temp5]    "=&r" (Temp5),    [Temp6]    "=&r" (Temp6),
              [Temp7]    "=&r" (Temp7),    [Temp8]    "=&r" (Temp8),
              [Temp9]    "=&r" (Temp9),    [Temp10]   "=&r" (Temp10),
              [Temp11]   "=&r" (Temp11),   [Temp12]   "=&r" (Temp12),
              [L_tmp]    "=&r" (L_tmp),    [L_tmp1]   "=&r" (L_tmp1),
              [exp_corr] "=&r" (exp_corr), [exp_norm] "=&r" (exp_norm),
              [ptr1]     "+r"  (ptr1),     [ptr2]     "+r"  (ptr2)
            : [ptr_end] "r" (ptr_end)
            : "hi", "lo", "memory"
        );
#endif /* defined(MIPS_DSP_R2_LE) */
        corr = extract_h(L_tmp);
        Isqrt_n(&L_tmp1, &exp_norm);
        norm = extract_h(L_tmp1);
        /* Normalize correlation = correlation * (1/sqrt(energy)) */

        L_tmp = vo_L_mult(corr, norm);

        L_tmp2 = exp_corr + exp_norm + scale;
        if(L_tmp2 < 0)
        {
            L_tmp2 = -L_tmp2;
            L_tmp = L_tmp >> L_tmp2;
        }
        else
        {
            L_tmp = L_tmp << L_tmp2;
        }

        corr_norm[t] = vo_round(L_tmp);
        /* modify the filtered excitation excf[] for the next iteration */

        if(t != t_max)
        {
            k = -(t + 1);
            tmp = exc[k];
            for (i = 63; i > 0; i--)
            {
                excf[i] = add1(vo_mult(tmp, h[i]), excf[i - 1]);
            }
            excf[0] = vo_mult(tmp, h[0]);
        }
    }
    return;
}

/************************************************************************************
* Function: Interpol_4()                                                             *
*                                                                                    *
* Description: For interpolating the normalized correlation with 1/4 resolution.     *
**************************************************************************************/

/* 1/4 resolution interpolation filter (-3 dB at 0.791*fs/2) in Q14 */
static Word16 inter4_1[4][8] =
{
    {-12, 420, -1732, 5429, 13418, -1242, 73, 32},
    {-26, 455, -2142, 9910, 9910,  -2142, 455, -26},
    {32,  73, -1242, 13418, 5429, -1732, 420, -12},
    {206, -766, 1376, 14746, 1376, -766, 206, 0}
};

static Word16 Interpol_4(                  /* (o)  : interpolated value  */
        Word16 * x,                           /* (i)  : input vector        */
        Word32 frac                           /* (i)  : fraction (-4..+3)   */
        )
{
    Word16 sum;
    Word32  k;
    Word16 *ptr;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8;
#if !defined(MIPS_DSP_R1_LE)
    Word32 L_sum;
#endif /* !defined(MIPS_DSP_R1_LE) */

    if (frac < 0)
    {
        frac += UP_SAMP;
        x--;
    }
    x = x - L_INTERPOL1 + 1;
    k = UP_SAMP - 1 - frac;
    ptr = &(inter4_1[k][0]);

#if defined(MIPS_DSP_R2_LE)
    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "ulw        %[Temp1],   0(%[x])                 \n\t"
        "ulw        %[Temp2],   0(%[ptr])               \n\t"
        "ulw        %[Temp3],   4(%[x])                 \n\t"
        "ulw        %[Temp4],   4(%[ptr])               \n\t"
        "ulw        %[Temp5],   8(%[x])                 \n\t"
        "ulw        %[Temp6],   8(%[ptr])               \n\t"
        "ulw        %[Temp7],   12(%[x])                \n\t"
        "ulw        %[Temp8],   12(%[ptr])              \n\t"
        "mult       $ac0,       $0,         $0          \n\t"
        "dpa.w.ph   $ac0,       %[Temp1],   %[Temp2]    \n\t"
        "dpa.w.ph   $ac0,       %[Temp3],   %[Temp4]    \n\t"
        "dpa.w.ph   $ac0,       %[Temp5],   %[Temp6]    \n\t"
        "dpa.w.ph   $ac0,       %[Temp7],   %[Temp8]    \n\t"
        "extr_r.w   %[sum],     $ac0,       14          \n\t"
        "shll_s.w   %[sum],     %[sum],     16          \n\t"
        "sra        %[sum],     %[sum],     16          \n\t"

        ".set pop                                       \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
          [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
          [sum]   "=&r" (sum)
        : [x] "r" (x), [ptr] "r" (ptr)
        : "hi", "lo", "memory"
    );
#elif defined(MIPS_DSP_R1_LE)
    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "lh         %[Temp1],   0(%[x])                 \n\t"
        "lh         %[Temp2],   0(%[ptr])               \n\t"
        "lh         %[Temp3],   2(%[x])                 \n\t"
        "lh         %[Temp4],   2(%[ptr])               \n\t"
        "lh         %[Temp5],   4(%[x])                 \n\t"
        "lh         %[Temp6],   4(%[ptr])               \n\t"
        "lh         %[Temp7],   6(%[x])                 \n\t"
        "lh         %[Temp8],   6(%[ptr])               \n\t"
        "mult       $ac0,       %[Temp1],   %[Temp2]    \n\t"
        "madd       $ac0,       %[Temp3],   %[Temp4]    \n\t"
        "madd       $ac0,       %[Temp5],   %[Temp6]    \n\t"
        "madd       $ac0,       %[Temp7],   %[Temp8]    \n\t"
        "lh         %[Temp1],   8(%[x])                 \n\t"
        "lh         %[Temp2],   8(%[ptr])               \n\t"
        "lh         %[Temp3],   10(%[x])                \n\t"
        "lh         %[Temp4],   10(%[ptr])              \n\t"
        "lh         %[Temp5],   12(%[x])                \n\t"
        "lh         %[Temp6],   12(%[ptr])              \n\t"
        "lh         %[Temp7],   14(%[x])                \n\t"
        "lh         %[Temp8],   14(%[ptr])              \n\t"
        "madd       $ac0,       %[Temp1],   %[Temp2]    \n\t"
        "madd       $ac0,       %[Temp3],   %[Temp4]    \n\t"
        "madd       $ac0,       %[Temp5],   %[Temp6]    \n\t"
        "madd       $ac0,       %[Temp7],   %[Temp8]    \n\t"
        "extr_r.w   %[sum],     $ac0,       14          \n\t"
        "shll_s.w   %[sum],     %[sum],     16          \n\t"
        "sra        %[sum],     %[sum],     16          \n\t"

        ".set pop                                       \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
          [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
          [sum]   "=&r" (sum)
        : [x] "r" (x), [ptr] "r" (ptr)
        : "hi", "lo", "memory"
    );
#else
    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "lh         %[Temp1],   0(%[x])                 \n\t"
        "lh         %[Temp2],   0(%[ptr])               \n\t"
        "lh         %[Temp3],   2(%[x])                 \n\t"
        "lh         %[Temp4],   2(%[ptr])               \n\t"
        "lh         %[Temp5],   4(%[x])                 \n\t"
        "lh         %[Temp6],   4(%[ptr])               \n\t"
        "lh         %[Temp7],   6(%[x])                 \n\t"
        "lh         %[Temp8],   6(%[ptr])               \n\t"
        "mult       %[Temp1],   %[Temp2]                \n\t"
        "madd       %[Temp3],   %[Temp4]                \n\t"
        "madd       %[Temp5],   %[Temp6]                \n\t"
        "madd       %[Temp7],   %[Temp8]                \n\t"
        "lh         %[Temp1],   8(%[x])                 \n\t"
        "lh         %[Temp2],   8(%[ptr])               \n\t"
        "lh         %[Temp3],   10(%[x])                \n\t"
        "lh         %[Temp4],   10(%[ptr])              \n\t"
        "lh         %[Temp5],   12(%[x])                \n\t"
        "lh         %[Temp6],   12(%[ptr])              \n\t"
        "lh         %[Temp7],   14(%[x])                \n\t"
        "lh         %[Temp8],   14(%[ptr])              \n\t"
        "madd       %[Temp1],   %[Temp2]                \n\t"
        "madd       %[Temp3],   %[Temp4]                \n\t"
        "madd       %[Temp5],   %[Temp6]                \n\t"
        "madd       %[Temp7],   %[Temp8]                \n\t"
        "mflo       %[L_sum]                            \n\t"

        ".set pop                                       \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
          [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
          [L_sum] "=r"  (L_sum)
        : [x] "r" (x), [ptr] "r" (ptr)
        : "hi", "lo", "memory"
    );
    sum = extract_h(L_add(L_shl2(L_sum, 2), 0x8000));
#endif /* defined(MIPS_DSP_R2_LE) */
    return (sum);
}




