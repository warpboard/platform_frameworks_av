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
*       File: hp6k_mips.c                                              *
*                                                                      *
*   Description:15th order band pass 6kHz to 7kHz FIR filter           *
*       frequency: 4kHz   5kHz  5.5kHz  6kHz  6.5kHz  7kHz 7.5kHz 8kHz *
*   dB loss:  -60dB  -45dB  -13dB   -3dB   0dB    -3dB -13dB  -45dB    *
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

#define L_FIR 31

/* filter coefficients (gain=4.0) */

extern Word16 fir_6k_7k[L_FIR];

void Filt_6k_7k_mips(
        Word16 signal[],                      /* input:  signal                  */
        Word16 lg,                            /* input:  length of input         */
        Word16 mem[]                          /* in/out: memory (size=30)        */
           )
{
    Word16 x[L_SUBFR16k + (L_FIR - 1)];
    Word32 i, L_tmp;
    Word32 L_tmp2, L_tmp3, L_tmp4;
    Word32 mem_t;
    Word32 x0, x1, x2, x3;
    Word32 xt0, xt1, xt2, xt3;
    Word32 x27, x28, x29, x30;

    Copy(mem, x, L_FIR - 1);
    for (i = lg - 1; i >= 0; i--)
    {
        x[i + L_FIR - 1] = signal[i] >> 2;                         /* gain of filter = 4 */
    }

    for (i = 0; i < lg; i+=4)
    {
        __asm__ volatile (
            "sll        %[x3],      %[i],       1           \n\t"
            "addu       %[mem_t],   %[x],       %[x3]       \n\t"
            "lh         %[L_tmp],   0(%[fir_6k_7k])         \n\t"
            "lh         %[L_tmp2],  2(%[fir_6k_7k])         \n\t"
            "lh         %[L_tmp3],  4(%[fir_6k_7k])         \n\t"
            "lh         %[L_tmp4],  6(%[fir_6k_7k])         \n\t"
            "lh         %[x0],      0(%[mem_t])             \n\t"
            "lh         %[x1],      2(%[mem_t])             \n\t"
            "lh         %[x2],      4(%[mem_t])             \n\t"
            "lh         %[x3],      6(%[mem_t])             \n\t"
            "lh         %[x30],     60(%[mem_t])            \n\t"
            "lh         %[x29],     58(%[mem_t])            \n\t"
            "lh         %[x28],     56(%[mem_t])            \n\t"
            "lh         %[x27],     54(%[mem_t])            \n\t"

            "addu       %[xt0],     %[x30],     %[x0]       \n\t"
            "addu       %[xt1],     %[x29],     %[x1]       \n\t"
            "addu       %[xt2],     %[x28],     %[x2]       \n\t"
            "addu       %[xt3],     %[x27],     %[x3]       \n\t"
            "mult       $ac0,       %[xt0],     %[L_tmp]    \n\t"
            "madd       $ac0,       %[xt1],     %[L_tmp2]   \n\t"
            "madd       $ac0,       %[xt2],     %[L_tmp3]   \n\t"
            "madd       $ac0,       %[xt3],     %[L_tmp4]   \n\t"

            "lh         %[x0],      8(%[mem_t])             \n\t"
            "lh         %[x27],     62(%[mem_t])            \n\t"
            "addu       %[xt2],     %[x30],     %[x2]       \n\t"
            "addu       %[xt0],     %[x29],     %[x3]       \n\t"
            "addu       %[xt3],     %[x28],     %[x0]       \n\t"
            "addu       %[xt1],     %[x27],     %[x1]       \n\t"
            "mult       $ac1,       %[xt2],     %[L_tmp2]   \n\t"
            "madd       $ac1,       %[xt0],     %[L_tmp3]   \n\t"
            "madd       $ac1,       %[xt3],     %[L_tmp4]   \n\t"
            "madd       $ac1,       %[xt1],     %[L_tmp]    \n\t"

            "lh         %[x1],      10(%[mem_t])            \n\t"
            "lh         %[x28],     64(%[mem_t])            \n\t"
            "addu       %[xt2],     %[x27],     %[x3]       \n\t"
            "addu       %[xt0],     %[x30],     %[x0]       \n\t"
            "addu       %[xt3],     %[x29],     %[x1]       \n\t"
            "addu       %[xt1],     %[x28],     %[x2]       \n\t"
            "mult       $ac2,       %[xt2],     %[L_tmp2]   \n\t"
            "madd       $ac2,       %[xt0],     %[L_tmp3]   \n\t"
            "madd       $ac2,       %[xt3],     %[L_tmp4]   \n\t"
            "madd       $ac2,       %[xt1],     %[L_tmp]    \n\t"

            "lh         %[x2],      12(%[mem_t])            \n\t"
            "lh         %[x29],     66(%[mem_t])            \n\t"
            "addu       %[xt2],     %[x28],     %[x0]       \n\t"
            "addu       %[xt0],     %[x27],     %[x1]       \n\t"
            "addu       %[xt3],     %[x30],     %[x2]       \n\t"
            "addu       %[xt1],     %[x29],     %[x3]       \n\t"
            "mult       $ac3,       %[xt2],     %[L_tmp2]   \n\t"
            "madd       $ac3,       %[xt0],     %[L_tmp3]   \n\t"
            "madd       $ac3,       %[xt3],     %[L_tmp4]   \n\t"
            "madd       $ac3,       %[xt1],     %[L_tmp]    \n\t"

            "lh         %[x30],     52(%[mem_t])            \n\t"
            "lh         %[x29],     50(%[mem_t])            \n\t"
            "lh         %[x28],     48(%[mem_t])            \n\t"
            "lh         %[L_tmp],   8(%[fir_6k_7k])         \n\t"
            "lh         %[L_tmp2],  10(%[fir_6k_7k])        \n\t"
            "lh         %[L_tmp3],  12(%[fir_6k_7k])        \n\t"

            "addu       %[xt0],     %[x30],     %[x0]       \n\t"
            "addu       %[xt1],     %[x29],     %[x1]       \n\t"
            "addu       %[xt2],     %[x28],     %[x2]       \n\t"
            "madd       $ac0,       %[xt0],     %[L_tmp]    \n\t"
            "madd       $ac0,       %[xt1],     %[L_tmp2]   \n\t"
            "madd       $ac0,       %[xt2],     %[L_tmp3]   \n\t"

            "lh         %[x3],      14(%[mem_t])            \n\t"
            "lh         %[x27],     54(%[mem_t])            \n\t"
            "addu       %[xt0],     %[x30],     %[x2]       \n\t"
            "addu       %[xt1],     %[x29],     %[x3]       \n\t"
            "addu       %[xt2],     %[x27],     %[x1]       \n\t"
            "madd       $ac1,       %[xt0],     %[L_tmp2]   \n\t"
            "madd       $ac1,       %[xt1],     %[L_tmp3]   \n\t"
            "madd       $ac1,       %[xt2],     %[L_tmp]    \n\t"

            "lh         %[x0],      16(%[mem_t])            \n\t"
            "lh         %[x28],     56(%[mem_t])            \n\t"
            "addu       %[xt0],     %[x27],     %[x3]       \n\t"
            "addu       %[xt1],     %[x30],     %[x0]       \n\t"
            "addu       %[xt2],     %[x28],     %[x2]       \n\t"
            "madd       $ac2,       %[xt0],     %[L_tmp2]   \n\t"
            "madd       $ac2,       %[xt1],     %[L_tmp3]   \n\t"
            "madd       $ac2,       %[xt2],     %[L_tmp]    \n\t"

            "lh         %[x1],      18(%[mem_t])            \n\t"
            "lh         %[x29],     58(%[mem_t])            \n\t"
            "addu       %[xt0],     %[x28],     %[x0]       \n\t"
            "addu       %[xt1],     %[x27],     %[x1]       \n\t"
            "addu       %[xt2],     %[x29],     %[x3]       \n\t"
            "madd       $ac3,       %[xt0],     %[L_tmp2]   \n\t"
            "madd       $ac3,       %[xt1],     %[L_tmp3]   \n\t"
            "madd       $ac3,       %[xt2],     %[L_tmp]    \n\t"

            "lh         %[x2],      20(%[mem_t])            \n\t"
            "lh         %[x3],      22(%[mem_t])            \n\t"
            "lh         %[x30],     44(%[mem_t])            \n\t"
            "lh         %[x29],     42(%[mem_t])            \n\t"
            "lh         %[x28],     40(%[mem_t])            \n\t"
            "lh         %[x27],     38(%[mem_t])            \n\t"
            "lh         %[L_tmp],   16(%[fir_6k_7k])        \n\t"
            "lh         %[L_tmp2],  18(%[fir_6k_7k])        \n\t"
            "lh         %[L_tmp3],  20(%[fir_6k_7k])        \n\t"
            "lh         %[L_tmp4],  22(%[fir_6k_7k])        \n\t"

            "addu       %[xt0],     %[x30],     %[x0]       \n\t"
            "addu       %[xt1],     %[x29],     %[x1]       \n\t"
            "addu       %[xt2],     %[x28],     %[x2]       \n\t"
            "addu       %[xt3],     %[x27],     %[x3]       \n\t"
            "madd       $ac0,       %[xt0],     %[L_tmp]    \n\t"
            "madd       $ac0,       %[xt1],     %[L_tmp2]   \n\t"
            "madd       $ac0,       %[xt2],     %[L_tmp3]   \n\t"
            "madd       $ac0,       %[xt3],     %[L_tmp4]   \n\t"

            "lh         %[x0],      24(%[mem_t])            \n\t"
            "lh         %[x27],     46(%[mem_t])            \n\t"
            "addu       %[xt2],     %[x30],     %[x2]       \n\t"
            "addu       %[xt0],     %[x29],     %[x3]       \n\t"
            "addu       %[xt3],     %[x28],     %[x0]       \n\t"
            "addu       %[xt1],     %[x27],     %[x1]       \n\t"
            "madd       $ac1,       %[xt2],     %[L_tmp2]   \n\t"
            "madd       $ac1,       %[xt0],     %[L_tmp3]   \n\t"
            "madd       $ac1,       %[xt3],     %[L_tmp4]   \n\t"
            "madd       $ac1,       %[xt1],     %[L_tmp]    \n\t"

            "lh         %[x1],      26(%[mem_t])            \n\t"
            "lh         %[x28],     48(%[mem_t])            \n\t"
            "addu       %[xt2],     %[x27],     %[x3]       \n\t"
            "addu       %[xt0],     %[x30],     %[x0]       \n\t"
            "addu       %[xt3],     %[x29],     %[x1]       \n\t"
            "addu       %[xt1],     %[x28],     %[x2]       \n\t"
            "madd       $ac2,       %[xt2],     %[L_tmp2]   \n\t"
            "madd       $ac2,       %[xt0],     %[L_tmp3]   \n\t"
            "madd       $ac2,       %[xt3],     %[L_tmp4]   \n\t"
            "madd       $ac2,       %[xt1],     %[L_tmp]    \n\t"

            "lh         %[x2],      28(%[mem_t])            \n\t"
            "lh         %[x29],     50(%[mem_t])            \n\t"
            "addu       %[xt2],     %[x28],     %[x0]       \n\t"
            "addu       %[xt0],     %[x27],     %[x1]       \n\t"
            "addu       %[xt3],     %[x30],     %[x2]       \n\t"
            "addu       %[xt1],     %[x29],     %[x3]       \n\t"
            "madd       $ac3,       %[xt2],     %[L_tmp2]   \n\t"
            "madd       $ac3,       %[xt0],     %[L_tmp3]   \n\t"
            "madd       $ac3,       %[xt3],     %[L_tmp4]   \n\t"
            "madd       $ac3,       %[xt1],     %[L_tmp]    \n\t"

            "lh         %[x3],      30(%[mem_t])            \n\t"
            "lh         %[x30],     36(%[mem_t])            \n\t"
            "lh         %[x29],     34(%[mem_t])            \n\t"
            "lh         %[x28],     32(%[mem_t])            \n\t"
            "lh         %[L_tmp],   24(%[fir_6k_7k])        \n\t"
            "lh         %[L_tmp2],  26(%[fir_6k_7k])        \n\t"
            "lh         %[L_tmp3],  28(%[fir_6k_7k])        \n\t"
            "lh         %[L_tmp4],  30(%[fir_6k_7k])        \n\t"

            "addu       %[xt0],     %[x30],     %[x0]       \n\t"
            "addu       %[xt1],     %[x29],     %[x1]       \n\t"
            "addu       %[xt2],     %[x28],     %[x2]       \n\t"
            "madd       $ac0,       %[xt0],     %[L_tmp]    \n\t"
            "madd       $ac0,       %[xt1],     %[L_tmp2]   \n\t"
            "madd       $ac0,       %[xt2],     %[L_tmp3]   \n\t"
            "madd       $ac0,       %[x3],      %[L_tmp4]   \n\t"

            "lh         %[x27],     38(%[mem_t])            \n\t"
            "lh         %[x0],      32(%[mem_t])            \n\t"
            "addu       %[xt0],     %[x30],     %[x2]       \n\t"
            "addu       %[xt1],     %[x29],     %[x3]       \n\t"
            "addu       %[xt2],     %[x27],     %[x1]       \n\t"
            "madd       $ac1,       %[xt0],     %[L_tmp2]   \n\t"
            "madd       $ac1,       %[xt1],     %[L_tmp3]   \n\t"
            "madd       $ac1,       %[xt2],     %[L_tmp]    \n\t"
            "madd       $ac1,       %[x0],      %[L_tmp4]   \n\t"

            "lh         %[x28],     40(%[mem_t])            \n\t"
            "addu       %[xt0],     %[x27],     %[x3]       \n\t"
            "addu       %[xt1],     %[x30],     %[x0]       \n\t"
            "madd       $ac2,       %[xt0],     %[L_tmp2]   \n\t"
            "madd       $ac2,       %[xt1],     %[L_tmp3]   \n\t"
            "addu       %[xt2],     %[x28],     %[x2]       \n\t"
            "madd       $ac2,       %[xt2],     %[L_tmp]    \n\t"
            "madd       $ac2,       %[x29],     %[L_tmp4]   \n\t"

            "lh         %[x1],      42(%[mem_t])            \n\t"
            "addu       %[xt0],     %[x28],     %[x0]       \n\t"
            "addu       %[xt1],     %[x27],     %[x29]      \n\t"
            "madd       $ac3,       %[xt0],     %[L_tmp2]   \n\t"
            "madd       $ac3,       %[xt1],     %[L_tmp3]   \n\t"
            "addu       %[xt2],     %[x1],      %[x3]       \n\t"
            "madd       $ac3,       %[xt2],     %[L_tmp]    \n\t"
            "madd       $ac3,       %[x30],     %[L_tmp4]   \n\t"

            "extr_r.w   %[L_tmp],   $ac0,       15          \n\t"
            "extr_r.w   %[L_tmp2],  $ac1,       15          \n\t"
            "extr_r.w   %[L_tmp3],  $ac2,       15          \n\t"
            "extr_r.w   %[L_tmp4],  $ac3,       15          \n\t"

            : [mem_t] "=&r" (mem_t), [x0] "=&r" (x0),
              [x30] "=&r"(x30), [L_tmp] "=&r" (L_tmp), [x2] "=&r" (x2),
              [x1] "=&r" (x1), [x3] "=&r" (x3), [x29] "=&r" (x29),
              [x28] "=&r" (x28), [x27] "=&r" (x27), [xt2] "=&r" (xt2),
              [xt1] "=&r" (xt1), [xt3] "=&r" (xt3), [xt0] "=&r" (xt0),
              [L_tmp2] "=r&" (L_tmp2), [L_tmp3] "=&r" (L_tmp3), [L_tmp4] "=&r" (L_tmp4)
            : [i] "r" (i), [x] "r" (x), [fir_6k_7k] "r" (fir_6k_7k)
            : "hi", "lo", "memory", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
              "$ac3hi", "$ac3lo"
        );

        signal[i] = L_tmp;
        signal[i+1] = L_tmp2;
        signal[i+2] = L_tmp3;
        signal[i+3] = L_tmp4;
    }

    Copy(x + lg, mem, L_FIR - 1);

}



