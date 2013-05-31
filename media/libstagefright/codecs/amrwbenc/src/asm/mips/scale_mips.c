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
*       File: scale_mips.c                                             *
*                                                                      *
*       Description: Scale signal to get maximum of dynamic            *
*                                                                      *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"

void Scale_sig_mips(
        Word16 x[],                           /* (i/o) : signal to scale               */
        Word16 lg,                            /* (i)   : size of x[]                   */
        Word16 exp                            /* (i)   : exponent: x = round(x << exp) */
          )
{
#ifdef MIPS_DSP_R1_LE
    Word32 i;
    Word32 shift, res, dev;
    Word32 L_tmp1, L_tmp2, L_tmp3, L_tmp4;
    Word32 a1, a2, a3, a4;
    Word16 *px;
#else /* MIPS_DSP_R1_LE */
    Word32 i, one = 1, one_t, exp_t;
    Word32 shift, res, dev;
    Word32 L_tmp, L_tmp2, L_tmp3, L_tmp4;
    Word32 a1, a2, a3, a4;
    Word16 *px;
#endif /* MIPS_DSP_R1_LE */

    shift = exp + 16;
    res = lg & 0x00000003;
    dev = lg >> 2;

    if(exp > 0)
    {
#ifdef MIPS_DSP_R1_LE
        for (i = lg - 1 ; i >= lg - res; i--)
        {
            px = &x[i];
            __asm__ volatile (
                ".set push                                              \n\t"
                ".set noreorder                                         \n\t"
                "lh                 %[a1],      0(%[px])                \n\t"
                "shllv_s.w          %[L_tmp1],  %[a1],      %[shift]    \n\t"
                "precrq_rs.ph.w     %[a1],      $zero,      %[L_tmp1]   \n\t"
                "sh                 %[a1],      0(%[px])                \n\t"
                ".set pop                                               \n\t"
                : [L_tmp1] "=&r" (L_tmp1), [a1] "=&r" (a1)
                : [shift] "r" (shift), [px] "r" (px)
                : "memory"
            );
        }

        for (i = lg - res -1 ; i >= 0; i-=4)
        {
            px = &x[i];
            __asm__ volatile (
                ".set push                                              \n\t"
                ".set noreorder                                         \n\t"
                "lh                 %[a1],      -6(%[px])               \n\t"
                "lh                 %[a2],      -4(%[px])               \n\t"
                "lh                 %[a3],      -2(%[px])               \n\t"
                "lh                 %[a4],      0(%[px])                \n\t"
                "shllv_s.w          %[L_tmp1],  %[a1],      %[shift]    \n\t"
                "shllv_s.w          %[L_tmp2],  %[a2],      %[shift]    \n\t"
                "shllv_s.w          %[L_tmp3],  %[a3],      %[shift]    \n\t"
                "shllv_s.w          %[L_tmp4],  %[a4],      %[shift]    \n\t"
                "precrq_rs.ph.w     %[a1],      %[L_tmp2],  %[L_tmp1]   \n\t"
                "precrq_rs.ph.w     %[a2],      %[L_tmp4],  %[L_tmp3]   \n\t"
                "usw                %[a1],      -6(%[px])               \n\t"
                "usw                %[a2],      -2(%[px])               \n\t"
                ".set pop                                               \n\t"
                : [L_tmp1] "=&r" (L_tmp1), [L_tmp2] "=&r" (L_tmp2), [a1] "=&r" (a1),
                  [L_tmp3] "=&r" (L_tmp3), [L_tmp4] "=&r" (L_tmp4), [a2] "=&r" (a2),
                  [a3] "=&r" (a3), [a4] "=&r" (a4)
                : [shift] "r" (shift), [px] "r" (px)
                : "memory"
            );
        }
#else /* MIPS_DSP_R1_LE */
        for (i = lg - 1 ; i >= lg - res; i--)
        {
            L_tmp = L_shl2(x[i], 16 + exp);
            x[i] = extract_h(L_add(L_tmp, 0x8000));
        }

        for (i = lg - res -1 ; i >= 0; i-=4)
        {
            L_tmp = L_shl2(x[i-3], 16 + exp);
            L_tmp2 = L_shl2(x[i-2], 16 + exp);
            L_tmp3 = L_shl2(x[i-1], 16 + exp);
            L_tmp4 = L_shl2(x[i], 16 + exp);
            x[i-3] = extract_h(L_add(L_tmp, 0x8000));
            x[i-2] = extract_h(L_add(L_tmp2, 0x8000));
            x[i-1] = extract_h(L_add(L_tmp3, 0x8000));
            x[i] = extract_h(L_add(L_tmp4, 0x8000));
        }
#endif /* MIPS_DSP_R1_LE */
    }
    else
    {
        exp = -exp;
#ifdef MIPS_DSP_R1_LE
        for (i = lg - 1 ; i >= lg - res; i--)
        {
            px = &x[i];
            __asm__ volatile (
                ".set push                                              \n\t"
                ".set noreorder                                         \n\t"
                "lh                 %[a1],      0(%[px])                \n\t"
                "shrav_r.w          %[a1],      %[a1],      %[exp]      \n\t"
                "sh                 %[a1],      0(%[px])                \n\t"
                ".set pop                                               \n\t"
                : [a1] "=&r" (a1)
                : [exp] "r" (exp), [px] "r" (px)
                : "memory"
            );
        }

        for (i = lg - res -1 ; i >= 0; i-=4)
        {
            px = &x[i];
            __asm__ volatile (
                ".set push                                              \n\t"
                ".set noreorder                                         \n\t"
                "lh                 %[a1],    -6(%[px])                 \n\t"
                "lh                 %[a2],    -4(%[px])                 \n\t"
                "lh                 %[a3],    -2(%[px])                 \n\t"
                "lh                 %[a4],    0(%[px])                  \n\t"
                "shrav_r.w          %[a1],    %[a1],        %[exp]      \n\t"
                "shrav_r.w          %[a2],    %[a2],        %[exp]      \n\t"
                "shrav_r.w          %[a3],    %[a3],        %[exp]      \n\t"
                "shrav_r.w          %[a4],    %[a4],        %[exp]      \n\t"
                "sh                 %[a1],    -6(%[px])                 \n\t"
                "sh                 %[a2],    -4(%[px])                 \n\t"
                "sh                 %[a3],    -2(%[px])                 \n\t"
                "sh                 %[a4],    0(%[px])                  \n\t"
                ".set pop                                               \n\t"
                : [a1] "=&r" (a1), [a2] "=&r" (a2), [a3] "=&r" (a3), [a4] "=&r" (a4)
                : [exp] "r" (exp), [px] "r" (px)
                : "memory"
            );
        }
#else /* MIPS_DSP_R1_LE */
        exp_t = exp - 1;
        for (i = lg - 1 ; i >= lg - res; i--)
        {
            px = &x[i];
            __asm__ volatile (
                ".set push                                  \n\t"
                ".set noreorder                             \n\t"
                "lh     %[a1],      0(%[px])                \n\t"
                "sllv   %[one_t],   %[one],     %[exp_t]    \n\t"
                "addu   %[a1],      %[a1],      %[one_t]    \n\t"
                "srav   %[a1],      %[a1],      %[exp]      \n\t"
                "sh     %[a1],      0(%[px])                \n\t"
                ".set pop                                   \n\t"
                : [a1] "=&r" (a1), [one_t] "=&r" (one_t)
                : [exp] "r" (exp), [px] "r" (px), [one] "r" (one),
                  [exp_t] "r" (exp_t)
                : "memory"
            );
        }

        for (i = lg - res -1 ; i >= 0; i-=4)
        {
            px = &x[i];
            __asm__ volatile (
                ".set push                                  \n\t"
                ".set noreorder                             \n\t"
                "lh     %[a1],      -6(%[px])               \n\t"
                "lh     %[a2],      -4(%[px])               \n\t"
                "lh     %[a3],      -2(%[px])               \n\t"
                "lh     %[a4],      0(%[px])                \n\t"
                "sllv   %[one_t],   %[one],     %[exp_t]    \n\t"
                "addu   %[a1],      %[a1],      %[one_t]    \n\t"
                "addu   %[a2],      %[a2],      %[one_t]    \n\t"
                "addu   %[a3],      %[a3],      %[one_t]    \n\t"
                "addu   %[a4],      %[a4],      %[one_t]    \n\t"
                "srav   %[a1],      %[a1],      %[exp]      \n\t"
                "srav   %[a2],      %[a2],      %[exp]      \n\t"
                "srav   %[a3],      %[a3],      %[exp]      \n\t"
                "srav   %[a4],      %[a4],      %[exp]      \n\t"
                "sh     %[a1],      -6(%[px])               \n\t"
                "sh     %[a2],      -4(%[px])               \n\t"
                "sh     %[a3],      -2(%[px])               \n\t"
                "sh     %[a4],      0(%[px])                \n\t"
                ".set pop                                   \n\t"
                : [a1] "=&r" (a1), [a2] "=&r" (a2), [a3] "=&r" (a3),
                  [a4] "=&r" (a4), [one_t] "=&r" (one_t)
                : [exp] "r" (exp), [px] "r" (px), [one] "r" (one),
                  [exp_t] "r" (exp_t)
                : "memory"
            );
        }
#endif /* MIPS_DSP_R1_LE */
    }
    return;
}



