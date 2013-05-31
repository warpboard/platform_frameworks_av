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
*       File: syn_filt_mips.c                                          *
*                                                                      *
*       Description: Do the synthesis filtering 1/A(z)                 *
*                                                                      *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "math_op.h"
#include "cnst.h"

void Syn_filt_mips(
    Word16 a[],                           /* (i) Q12 : a[m+1] prediction coefficients           */
    Word16 x[],                           /* (i)     : input signal                             */
    Word16 y[],                           /* (o)     : output signal                            */
    Word16 lg,                            /* (i)     : size of filtering                        */
    Word16 mem[],                         /* (i/o)   : memory associated with this filtering.   */
    Word16 update                         /* (i)     : 0=no update, 1=update of memory.         */
    )
{
    Word32 i, a0;
    Word16 y_buf[L_SUBFR16k + M16k];
    Word32 L_tmp;
    Word16 *yy, *p1, *p2, *x1;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8, Temp9;
#if !defined(MIPS_DSP_R2_LE)
    Word32 Temp10, Temp11, Temp12, Temp13, Temp14, Temp15, Temp16, Temp17;
#endif /* !defined(MIPS_DSP_R2_LE) */

    yy = &y_buf[0];
    memcpy (yy, mem, sizeof(Word16) * 16);
    yy += 16;
    a0 = (a[0] >> 1);

    for (i = 0; i < lg; i++)
    {
        p1 = &a[1];
        p2 = &yy[i-16];
        x1 = &x[i];

#if defined(MIPS_DSP_R2_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "lh         %[Temp1],   0(%[x1])                \n\t"
            "ulw        %[Temp2],   0(%[p1])                \n\t"
            "ulw        %[Temp3],   28(%[p2])               \n\t"
            "ulw        %[Temp4],   4(%[p1])                \n\t"
            "ulw        %[Temp5],   24(%[p2])               \n\t"
            "ulw        %[Temp6],   8(%[p1])                \n\t"
            "ulw        %[Temp7],   20(%[p2])               \n\t"
            "ulw        %[Temp8],   12(%[p1])               \n\t"
            "ulw        %[Temp9],   16(%[p2])               \n\t"
            "mult       $ac0,       %[Temp1],   %[a0]       \n\t"
            "dpsx.w.ph  $ac0,       %[Temp2],   %[Temp3]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp4],   %[Temp5]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp6],   %[Temp7]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp8],   %[Temp9]    \n\t"
            "ulw        %[Temp1],   16(%[p1])               \n\t"
            "ulw        %[Temp2],   12(%[p2])               \n\t"
            "ulw        %[Temp3],   20(%[p1])               \n\t"
            "ulw        %[Temp4],   8(%[p2])                \n\t"
            "ulw        %[Temp5],   24(%[p1])               \n\t"
            "ulw        %[Temp6],   4(%[p2])                \n\t"
            "ulw        %[Temp7],   28(%[p1])               \n\t"
            "ulw        %[Temp8],   0(%[p2])                \n\t"
            "dpsx.w.ph  $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "extr_r.w   %[L_tmp],   $ac0,       12          \n\t"
            "shll_s.w   %[L_tmp],   %[L_tmp],   16          \n\t"
            "sra        %[L_tmp],   %[L_tmp],   16          \n\t"

            ".set pop                                       \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [Temp9] "=&r" (Temp9), [L_tmp] "=r"  (L_tmp)
            : [p1] "r" (p1), [p2] "r" (p2),
              [a0] "r" (a0), [x1] "r" (x1)
            : "hi", "lo", "memory"
        );

        y[i] = yy[i] = L_tmp;
#elif defined(MIPS_DSP_R1_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "lh         %[Temp1],   0(%[x1])                \n\t"
            "lh         %[Temp2],   0(%[p1])                \n\t"
            "lh         %[Temp3],   30(%[p2])               \n\t"
            "lh         %[Temp4],   2(%[p1])                \n\t"
            "lh         %[Temp5],   28(%[p2])               \n\t"
            "lh         %[Temp6],   4(%[p1])                \n\t"
            "lh         %[Temp7],   26(%[p2])               \n\t"
            "lh         %[Temp8],   6(%[p1])                \n\t"
            "lh         %[Temp9],   24(%[p2])               \n\t"
            "lh         %[Temp10],  8(%[p1])                \n\t"
            "lh         %[Temp11],  22(%[p2])               \n\t"
            "lh         %[Temp12],  10(%[p1])               \n\t"
            "lh         %[Temp13],  20(%[p2])               \n\t"
            "lh         %[Temp14],  12(%[p1])               \n\t"
            "lh         %[Temp15],  18(%[p2])               \n\t"
            "lh         %[Temp16],  14(%[p1])               \n\t"
            "lh         %[Temp17],  16(%[p2])               \n\t"
            "mult       $ac0,       %[Temp1],   %[a0]       \n\t"
            "msub       $ac0,       %[Temp2],   %[Temp3]    \n\t"
            "msub       $ac0,       %[Temp4],   %[Temp5]    \n\t"
            "msub       $ac0,       %[Temp6],   %[Temp7]    \n\t"
            "msub       $ac0,       %[Temp8],   %[Temp9]    \n\t"
            "msub       $ac0,       %[Temp10],  %[Temp11]   \n\t"
            "msub       $ac0,       %[Temp12],  %[Temp13]   \n\t"
            "msub       $ac0,       %[Temp14],  %[Temp15]   \n\t"
            "msub       $ac0,       %[Temp16],  %[Temp17]   \n\t"
            "lh         %[Temp1],   16(%[p1])               \n\t"
            "lh         %[Temp2],   14(%[p2])               \n\t"
            "lh         %[Temp3],   18(%[p1])               \n\t"
            "lh         %[Temp4],   12(%[p2])               \n\t"
            "lh         %[Temp5],   20(%[p1])               \n\t"
            "lh         %[Temp6],   10(%[p2])               \n\t"
            "lh         %[Temp7],   22(%[p1])               \n\t"
            "lh         %[Temp8],   8(%[p2])                \n\t"
            "lh         %[Temp9],   24(%[p1])               \n\t"
            "lh         %[Temp10],  6(%[p2])                \n\t"
            "lh         %[Temp11],  26(%[p1])               \n\t"
            "lh         %[Temp12],  4(%[p2])                \n\t"
            "lh         %[Temp13],  28(%[p1])               \n\t"
            "lh         %[Temp14],  2(%[p2])                \n\t"
            "lh         %[Temp15],  30(%[p1])               \n\t"
            "lh         %[Temp16],  0(%[p2])                \n\t"
            "msub       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "msub       $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "msub       $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "msub       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "msub       $ac0,       %[Temp9],   %[Temp10]   \n\t"
            "msub       $ac0,       %[Temp11],  %[Temp12]   \n\t"
            "msub       $ac0,       %[Temp13],  %[Temp14]   \n\t"
            "msub       $ac0,       %[Temp15],  %[Temp16]   \n\t"
            "extr_r.w   %[L_tmp],   $ac0,       12          \n\t"
            "shll_s.w   %[L_tmp],   %[L_tmp],   16          \n\t"
            "sra        %[L_tmp],   %[L_tmp],   16          \n\t"

            ".set pop                                       \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
              [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
              [Temp13] "=&r" (Temp13), [Temp14] "=&r" (Temp14),
              [Temp15] "=&r" (Temp15), [Temp16] "=&r" (Temp16),
              [Temp17] "=&r" (Temp17), [L_tmp]  "=&r" (L_tmp)
            : [p1] "r" (p1), [p2] "r" (p2),
              [a0] "r" (a0), [x1] "r" (x1)
            : "hi", "lo", "memory"
        );

        y[i] = yy[i] = L_tmp;
#else
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "lh         %[Temp1],   0(%[x1])                \n\t"
            "lh         %[Temp2],   0(%[p1])                \n\t"
            "lh         %[Temp3],   30(%[p2])               \n\t"
            "lh         %[Temp4],   2(%[p1])                \n\t"
            "lh         %[Temp5],   28(%[p2])               \n\t"
            "lh         %[Temp6],   4(%[p1])                \n\t"
            "lh         %[Temp7],   26(%[p2])               \n\t"
            "lh         %[Temp8],   6(%[p1])                \n\t"
            "lh         %[Temp9],   24(%[p2])               \n\t"
            "lh         %[Temp10],  8(%[p1])                \n\t"
            "lh         %[Temp11],  22(%[p2])               \n\t"
            "lh         %[Temp12],  10(%[p1])               \n\t"
            "lh         %[Temp13],  20(%[p2])               \n\t"
            "lh         %[Temp14],  12(%[p1])               \n\t"
            "lh         %[Temp15],  18(%[p2])               \n\t"
            "lh         %[Temp16],  14(%[p1])               \n\t"
            "lh         %[Temp17],  16(%[p2])               \n\t"
            "mult       %[Temp1],   %[a0]                   \n\t"
            "msub       %[Temp2],   %[Temp3]                \n\t"
            "msub       %[Temp4],   %[Temp5]                \n\t"
            "msub       %[Temp6],   %[Temp7]                \n\t"
            "msub       %[Temp8],   %[Temp9]                \n\t"
            "msub       %[Temp10],  %[Temp11]               \n\t"
            "msub       %[Temp12],  %[Temp13]               \n\t"
            "msub       %[Temp14],  %[Temp15]               \n\t"
            "msub       %[Temp16],  %[Temp17]               \n\t"
            "lh         %[Temp1],   16(%[p1])               \n\t"
            "lh         %[Temp2],   14(%[p2])               \n\t"
            "lh         %[Temp3],   18(%[p1])               \n\t"
            "lh         %[Temp4],   12(%[p2])               \n\t"
            "lh         %[Temp5],   20(%[p1])               \n\t"
            "lh         %[Temp6],   10(%[p2])               \n\t"
            "lh         %[Temp7],   22(%[p1])               \n\t"
            "lh         %[Temp8],   8(%[p2])                \n\t"
            "lh         %[Temp9],   24(%[p1])               \n\t"
            "lh         %[Temp10],  6(%[p2])                \n\t"
            "lh         %[Temp11],  26(%[p1])               \n\t"
            "lh         %[Temp12],  4(%[p2])                \n\t"
            "lh         %[Temp13],  28(%[p1])               \n\t"
            "lh         %[Temp14],  2(%[p2])                \n\t"
            "lh         %[Temp15],  30(%[p1])               \n\t"
            "lh         %[Temp16],  0(%[p2])                \n\t"
            "msub       %[Temp1],   %[Temp2]                \n\t"
            "msub       %[Temp3],   %[Temp4]                \n\t"
            "msub       %[Temp5],   %[Temp6]                \n\t"
            "msub       %[Temp7],   %[Temp8]                \n\t"
            "msub       %[Temp9],   %[Temp10]               \n\t"
            "msub       %[Temp11],  %[Temp12]               \n\t"
            "msub       %[Temp13],  %[Temp14]               \n\t"
            "msub       %[Temp15],  %[Temp16]               \n\t"
            "mflo       %[L_tmp]                            \n\t"

            ".set pop                                       \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
              [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
              [Temp13] "=&r" (Temp13), [Temp14] "=&r" (Temp14),
              [Temp15] "=&r" (Temp15), [Temp16] "=&r" (Temp16),
              [Temp17] "=&r" (Temp17), [L_tmp]  "=&r" (L_tmp)
            : [p1] "r" (p1), [p2] "r" (p2),
              [a0] "r" (a0), [x1] "r" (x1)
            : "hi", "lo", "memory"
        );

        L_tmp = L_shl2(L_tmp, 4);
        y[i] = yy[i] = extract_h(L_add(L_tmp, 0x8000));
#endif /* defined(MIPS_DSP_R2_LE) */
    }

    if (update)
        memcpy(mem, &yy[lg-16], sizeof(Word16) * 16);

    return;
}


void Syn_filt_32_mips(
        Word16 a[],                           /* (i) Q12 : a[m+1] prediction coefficients */
        Word16 m,                             /* (i)     : order of LP filter             */
        Word16 exc[],                         /* (i) Qnew: excitation (exc[i] >> Qnew)    */
        Word16 Qnew,                          /* (i)     : exc scaling = 0(min) to 8(max) */
        Word16 sig_hi[],                      /* (o) /16 : synthesis high                 */
        Word16 sig_lo[],                      /* (o) /16 : synthesis low                  */
        Word16 lg                             /* (i)     : size of filtering              */
        )
{
    Word32 i,a0;
    Word32 L_tmp, L_tmp1;
    Word16 *p1, *p2, *p3;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8, Temp9;
#if !defined(MIPS_DSP_R1_LE)
    Word32 Temp10, Temp11, Temp12;
#endif /* !defined(MIPS_DSP_R1_LE) */

    a0 = a[0] >> (4 + Qnew);

    for (i = 0; i < lg; i++)
    {
        p1 = a;
        p2 = &sig_lo[i - 1] - 15;
        p3 = &sig_hi[i - 1] - 15;
#if defined(MIPS_DSP_R2_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "ulw        %[Temp1],   0(%[p1])                \n\t"
            "ulw        %[Temp2],   28(%[p2])               \n\t"
            "ulw        %[Temp3],   28(%[p3])               \n\t"
            "ulw        %[Temp4],   4(%[p1])                \n\t"
            "ulw        %[Temp5],   24(%[p2])               \n\t"
            "ulw        %[Temp6],   24(%[p3])               \n\t"
            "ulw        %[Temp7],   8(%[p1])                \n\t"
            "ulw        %[Temp8],   20(%[p2])               \n\t"
            "ulw        %[Temp9],   20(%[p3])               \n\t"
            "mult       $ac0,       $0,         $0          \n\t"
            "dpsx.w.ph  $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp4],   %[Temp5]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "mult       $ac1,       $0,         $0          \n\t"
            "dpsx.w.ph  $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "dpsx.w.ph  $ac1,       %[Temp4],   %[Temp6]    \n\t"
            "dpsx.w.ph  $ac1,       %[Temp7],   %[Temp9]    \n\t"
            "ulw        %[Temp1],   12(%[p1])               \n\t"
            "ulw        %[Temp2],   16(%[p2])               \n\t"
            "ulw        %[Temp3],   16(%[p3])               \n\t"
            "ulw        %[Temp4],   16(%[p1])               \n\t"
            "ulw        %[Temp5],   12(%[p2])               \n\t"
            "ulw        %[Temp6],   12(%[p3])               \n\t"
            "ulw        %[Temp7],   20(%[p1])               \n\t"
            "ulw        %[Temp8],   8(%[p2])                \n\t"
            "ulw        %[Temp9],   8(%[p3])                \n\t"
            "dpsx.w.ph  $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp4],   %[Temp5]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "dpsx.w.ph  $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "dpsx.w.ph  $ac1,       %[Temp4],   %[Temp6]    \n\t"
            "dpsx.w.ph  $ac1,       %[Temp7],   %[Temp9]    \n\t"
            "ulw        %[Temp1],   24(%[p1])               \n\t"
            "ulw        %[Temp2],   4(%[p2])                \n\t"
            "ulw        %[Temp3],   4(%[p3])                \n\t"
            "ulw        %[Temp4],   28(%[p1])               \n\t"
            "ulw        %[Temp5],   0(%[p2])                \n\t"
            "ulw        %[Temp6],   0(%[p3])                \n\t"
            "addiu      %[p1],      %[p1],      32          \n\t"
            "addiu      %[p2],      %[p2],      -32         \n\t"
            "addiu      %[p3],      %[p3],      -32         \n\t"
            "dpsx.w.ph  $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpsx.w.ph  $ac0,       %[Temp4],   %[Temp5]    \n\t"
            "dpsx.w.ph  $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "dpsx.w.ph  $ac1,       %[Temp4],   %[Temp6]    \n\t"
            "mflo       %[L_tmp],   $ac0                    \n\t"
            "mflo       %[L_tmp1],  $ac1                    \n\t"

            ".set pop                                       \n\t"

            : [p1] "+r" (p1), [p2] "+r" (p2), [p3] "+r" (p3),
              [Temp1]  "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9), [L_tmp] "=r"  (L_tmp),
              [L_tmp1] "=r"  (L_tmp1)
            :
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "memory"
        );
#elif defined(MIPS_DSP_R1_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp2],   30(%[p2])               \n\t"
            "lh         %[Temp3],   30(%[p3])               \n\t"
            "lh         %[Temp4],   2(%[p1])                \n\t"
            "lh         %[Temp5],   28(%[p2])               \n\t"
            "lh         %[Temp6],   28(%[p3])               \n\t"
            "lh         %[Temp7],   4(%[p1])                \n\t"
            "lh         %[Temp8],   26(%[p2])               \n\t"
            "lh         %[Temp9],   26(%[p3])               \n\t"
            "mult       $ac0,       $0,         $0          \n\t"
            "msub       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "msub       $ac0,       %[Temp4],   %[Temp5]    \n\t"
            "msub       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "mult       $ac1,       $0,         $0          \n\t"
            "msub       $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "msub       $ac1,       %[Temp4],   %[Temp6]    \n\t"
            "msub       $ac1,       %[Temp7],   %[Temp9]    \n\t"
            "lh         %[Temp1],   6(%[p1])                \n\t"
            "lh         %[Temp2],   24(%[p2])               \n\t"
            "lh         %[Temp3],   24(%[p3])               \n\t"
            "lh         %[Temp4],   8(%[p1])                \n\t"
            "lh         %[Temp5],   22(%[p2])               \n\t"
            "lh         %[Temp6],   22(%[p3])               \n\t"
            "lh         %[Temp7],   10(%[p1])               \n\t"
            "lh         %[Temp8],   20(%[p2])               \n\t"
            "lh         %[Temp9],   20(%[p3])               \n\t"
            "msub       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "msub       $ac0,       %[Temp4],   %[Temp5]    \n\t"
            "msub       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "msub       $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "msub       $ac1,       %[Temp4],   %[Temp6]    \n\t"
            "msub       $ac1,       %[Temp7],   %[Temp9]    \n\t"
            "lh         %[Temp1],   12(%[p1])               \n\t"
            "lh         %[Temp2],   18(%[p2])               \n\t"
            "lh         %[Temp3],   18(%[p3])               \n\t"
            "lh         %[Temp4],   14(%[p1])               \n\t"
            "lh         %[Temp5],   16(%[p2])               \n\t"
            "lh         %[Temp6],   16(%[p3])               \n\t"
            "lh         %[Temp7],   16(%[p1])               \n\t"
            "lh         %[Temp8],   14(%[p2])               \n\t"
            "lh         %[Temp9],   14(%[p3])               \n\t"
            "msub       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "msub       $ac0,       %[Temp4],   %[Temp5]    \n\t"
            "msub       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "msub       $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "msub       $ac1,       %[Temp4],   %[Temp6]    \n\t"
            "msub       $ac1,       %[Temp7],   %[Temp9]    \n\t"
            "lh         %[Temp1],   18(%[p1])               \n\t"
            "lh         %[Temp2],   12(%[p2])               \n\t"
            "lh         %[Temp3],   12(%[p3])               \n\t"
            "lh         %[Temp4],   20(%[p1])               \n\t"
            "lh         %[Temp5],   10(%[p2])               \n\t"
            "lh         %[Temp6],   10(%[p3])               \n\t"
            "lh         %[Temp7],   22(%[p1])               \n\t"
            "lh         %[Temp8],   8(%[p2])                \n\t"
            "lh         %[Temp9],   8(%[p3])                \n\t"
            "msub       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "msub       $ac0,       %[Temp4],   %[Temp5]    \n\t"
            "msub       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "msub       $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "msub       $ac1,       %[Temp4],   %[Temp6]    \n\t"
            "msub       $ac1,       %[Temp7],   %[Temp9]    \n\t"
            "lh         %[Temp1],   24(%[p1])               \n\t"
            "lh         %[Temp2],   6(%[p2])                \n\t"
            "lh         %[Temp3],   6(%[p3])                \n\t"
            "lh         %[Temp4],   26(%[p1])               \n\t"
            "lh         %[Temp5],   4(%[p2])                \n\t"
            "lh         %[Temp6],   4(%[p3])                \n\t"
            "lh         %[Temp7],   28(%[p1])               \n\t"
            "lh         %[Temp8],   2(%[p2])                \n\t"
            "lh         %[Temp9],   2(%[p3])                \n\t"
            "msub       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "msub       $ac0,       %[Temp4],   %[Temp5]    \n\t"
            "msub       $ac0,       %[Temp7],   %[Temp8]    \n\t"
            "msub       $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "msub       $ac1,       %[Temp4],   %[Temp6]    \n\t"
            "msub       $ac1,       %[Temp7],   %[Temp9]    \n\t"
            "lh         %[Temp1],   30(%[p1])               \n\t"
            "lh         %[Temp2],   0(%[p2])                \n\t"
            "lh         %[Temp3],   0(%[p3])                \n\t"
            "addiu      %[p1],      %[p1],      32          \n\t"
            "msub       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "msub       $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "addiu      %[p2],      %[p2],      -32         \n\t"
            "addiu      %[p3],      %[p3],      -32         \n\t"
            "mflo       %[L_tmp],   $ac0                    \n\t"
            "mflo       %[L_tmp1],  $ac1                    \n\t"

            ".set pop                                       \n\t"

            : [p1] "+r" (p1), [p2] "+r" (p2), [p3] "+r" (p3),
              [Temp1]  "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9), [L_tmp] "=r"  (L_tmp),
              [L_tmp1] "=r"  (L_tmp1)
            :
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "memory"
        );
#else
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $0,         $0                      \n\t"
            "li         %[L_tmp],   0                       \n\t"
            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp2],   30(%[p2])               \n\t"
            "lh         %[Temp3],   30(%[p3])               \n\t"
            "lh         %[Temp4],   2(%[p1])                \n\t"
            "lh         %[Temp5],   28(%[p2])               \n\t"
            "lh         %[Temp6],   28(%[p3])               \n\t"
            "lh         %[Temp7],   4(%[p1])                \n\t"
            "lh         %[Temp8],   26(%[p2])               \n\t"
            "lh         %[Temp9],   26(%[p3])               \n\t"
            "lh         %[Temp10],  6(%[p1])                \n\t"
            "lh         %[Temp11],  24(%[p2])               \n\t"
            "lh         %[Temp12],  24(%[p3])               \n\t"
            "mul        %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
            "mul        %[Temp5],   %[Temp4],   %[Temp5]    \n\t"
            "mul        %[Temp8],   %[Temp7],   %[Temp8]    \n\t"
            "mul        %[Temp11],  %[Temp10],  %[Temp11]   \n\t"
            "msub       %[Temp1],   %[Temp3]                \n\t"
            "msub       %[Temp4],   %[Temp6]                \n\t"
            "msub       %[Temp7],   %[Temp9]                \n\t"
            "msub       %[Temp10],  %[Temp12]               \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp2]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp5]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp8]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp11]   \n\t"
            "lh         %[Temp1],   8(%[p1])                \n\t"
            "lh         %[Temp2],   22(%[p2])               \n\t"
            "lh         %[Temp3],   22(%[p3])               \n\t"
            "lh         %[Temp4],   10(%[p1])               \n\t"
            "lh         %[Temp5],   20(%[p2])               \n\t"
            "lh         %[Temp6],   20(%[p3])               \n\t"
            "lh         %[Temp7],   12(%[p1])               \n\t"
            "lh         %[Temp8],   18(%[p2])               \n\t"
            "lh         %[Temp9],   18(%[p3])               \n\t"
            "lh         %[Temp10],  14(%[p1])               \n\t"
            "lh         %[Temp11],  16(%[p2])               \n\t"
            "lh         %[Temp12],  16(%[p3])               \n\t"
            "mul        %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
            "mul        %[Temp5],   %[Temp4],   %[Temp5]    \n\t"
            "mul        %[Temp8],   %[Temp7],   %[Temp8]    \n\t"
            "mul        %[Temp11],  %[Temp10],  %[Temp11]   \n\t"
            "msub       %[Temp1],   %[Temp3]                \n\t"
            "msub       %[Temp4],   %[Temp6]                \n\t"
            "msub       %[Temp7],   %[Temp9]                \n\t"
            "msub       %[Temp10],  %[Temp12]               \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp2]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp5]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp8]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp11]   \n\t"
            "lh         %[Temp1],   16(%[p1])               \n\t"
            "lh         %[Temp2],   14(%[p2])               \n\t"
            "lh         %[Temp3],   14(%[p3])               \n\t"
            "lh         %[Temp4],   18(%[p1])               \n\t"
            "lh         %[Temp5],   12(%[p2])               \n\t"
            "lh         %[Temp6],   12(%[p3])               \n\t"
            "lh         %[Temp7],   20(%[p1])               \n\t"
            "lh         %[Temp8],   10(%[p2])               \n\t"
            "lh         %[Temp9],   10(%[p3])               \n\t"
            "lh         %[Temp10],  22(%[p1])               \n\t"
            "lh         %[Temp11],  8(%[p2])                \n\t"
            "lh         %[Temp12],  8(%[p3])                \n\t"
            "mul        %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
            "mul        %[Temp5],   %[Temp4],   %[Temp5]    \n\t"
            "mul        %[Temp8],   %[Temp7],   %[Temp8]    \n\t"
            "mul        %[Temp11],  %[Temp10],  %[Temp11]   \n\t"
            "msub       %[Temp1],   %[Temp3]                \n\t"
            "msub       %[Temp4],   %[Temp6]                \n\t"
            "msub       %[Temp7],   %[Temp9]                \n\t"
            "msub       %[Temp10],  %[Temp12]               \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp2]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp5]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp8]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp11]   \n\t"
            "lh         %[Temp1],   24(%[p1])               \n\t"
            "lh         %[Temp2],   6(%[p2])                \n\t"
            "lh         %[Temp3],   6(%[p3])                \n\t"
            "lh         %[Temp4],   26(%[p1])               \n\t"
            "lh         %[Temp5],   4(%[p2])                \n\t"
            "lh         %[Temp6],   4(%[p3])                \n\t"
            "lh         %[Temp7],   28(%[p1])               \n\t"
            "lh         %[Temp8],   2(%[p2])                \n\t"
            "lh         %[Temp9],   2(%[p3])                \n\t"
            "lh         %[Temp10],  30(%[p1])               \n\t"
            "lh         %[Temp11],  0(%[p2])                \n\t"
            "lh         %[Temp12],  0(%[p3])                \n\t"
            "mul        %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
            "mul        %[Temp5],   %[Temp4],   %[Temp5]    \n\t"
            "mul        %[Temp8],   %[Temp7],   %[Temp8]    \n\t"
            "mul        %[Temp11],  %[Temp10],  %[Temp11]   \n\t"
            "msub       %[Temp1],   %[Temp3]                \n\t"
            "msub       %[Temp4],   %[Temp6]                \n\t"
            "msub       %[Temp7],   %[Temp9]                \n\t"
            "msub       %[Temp10],  %[Temp12]               \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp2]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp5]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp8]    \n\t"
            "subu       %[L_tmp],   %[L_tmp],   %[Temp11]   \n\t"
            "mflo       %[L_tmp1]                           \n\t"

            ".set pop                                       \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
              [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
              [L_tmp]  "=&r" (L_tmp),  [L_tmp1] "=&r" (L_tmp1)
            : [p1] "r" (p1), [p2] "r" (p2), [p3] "r" (p3)
            : "hi", "lo", "memory"
        );

        p1 += 16;
        p2 -= 16;
        p3 -= 16;
#endif /* defined(MIPS_DSP_R2_LE) */

        L_tmp = L_tmp >> 11;
        L_tmp += vo_L_mult(exc[i], a0);

        L_tmp = L_tmp - (L_tmp1<<1);

        L_tmp = L_tmp >> 3;
        sig_hi[i] = extract_h(L_tmp);

        L_tmp >>= 4;
        sig_lo[i] = (Word16)((L_tmp - (sig_hi[i] << 13)));
    }

    return;
}




