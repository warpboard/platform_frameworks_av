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
*  File: residu_mips.c                                                 *
*                                                                      *
*  Description: Compute the LPC residual by filtering                  *
*             the input speech through A(z)                            *
*                                                                      *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"

void Residu_mips(
        Word16 a[],                           /* (i) Q12 : prediction coefficients                     */
        Word16 x[],                           /* (i)     : speech (values x[-m..-1] are needed         */
        Word16 y[],                           /* (o) x2  : residual signal                             */
        Word16 lg                             /* (i)     : size of filtering                           */
        )
{
#if defined(MIPS_DSP_R2_LE)
    Word16 i, *p2, a16, *y_t = &y[0];
    Word32 s, tmp = 0x8000;
    Word32 ta0, ta1, ta2, ta3, ta4, ta5, ta6, ta7, tb8;
    Word32 tb0, tb1, tb2, tb3, tb4, tb5, tb6, tb7;
#else
    Word16 i, *p2, *y_t = &y[0];
    Word32 s, tmp = 0x8000, ta0, ta1, ta2, ta3, ta4, ta5, ta6, ta7, tb8;
    Word32 tb0, tb1, tb2, tb3, tb4, tb5, tb6, tb7;
    Word16 a16;
#endif /* MIPS_DSP_R2_LE */

#if defined(MIPS_DSP_R2_LE)
    __asm__ volatile (
        "ulw        %[ta0],     0(%[a])                 \n\t"
        "ulw        %[ta1],     4(%[a])                 \n\t"
        "ulw        %[ta2],     8(%[a])                 \n\t"
        "ulw        %[ta3],     12(%[a])                \n\t"
        "ulw        %[ta4],     16(%[a])                \n\t"
        "ulw        %[ta5],     20(%[a])                \n\t"
        "ulw        %[ta6],     24(%[a])                \n\t"
        "ulw        %[ta7],     28(%[a])                \n\t"
        : [ta0] "=&r" (ta0), [ta1] "=&r" (ta1), [ta2] "=&r" (ta2),
          [ta3] "=&r" (ta3), [ta4] "=&r" (ta4), [ta5] "=&r" (ta5),
          [ta6] "=&r" (ta6), [ta7] "=r" (ta7)
        : [a] "r" (a)
        : "memory"
    );

    p2 = &x[0];
    a16 = a[16];

    __asm__ volatile (
        "ulw            %[tb0],     -2(%[p2])               \n\t"
        "ulw            %[tb1],     -6(%[p2])               \n\t"
        "ulw            %[tb2],     -10(%[p2])              \n\t"
        "ulw            %[tb3],     -14(%[p2])              \n\t"
        "ulw            %[tb4],     -18(%[p2])              \n\t"
        "ulw            %[tb5],     -22(%[p2])              \n\t"
        "ulw            %[tb6],     -26(%[p2])              \n\t"
        "ulw            %[tb7],     -30(%[p2])              \n\t"
        "addu           %[i],       $zero,      $zero       \n\t"
        "vAa_lp%=:                                          \n\t"
        "beq            %[i],       %[lg],      bBb_lp%=    \n\t"
        "lh             %[tb8],     -32(%[p2])              \n\t"
        "mult           $ac0,       $zero,      $zero       \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb0],     %[ta0]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb1],     %[ta1]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb2],     %[ta2]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb3],     %[ta3]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb4],     %[ta4]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb5],     %[ta5]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb6],     %[ta6]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb7],     %[ta7]      \n\t"
        "dpaq_sa.l.w    $ac0,       %[tb8],     %[a16]      \n\t"
        "addu           %[tb7],     %[tb6],     $zero       \n\t"
        "addu           %[tb6],     %[tb5],     $zero       \n\t"
        "addu           %[tb5],     %[tb4],     $zero       \n\t"
        "addu           %[tb4],     %[tb3],     $zero       \n\t"
        "mflo           %[s],       $ac0                    \n\t"
        "addiu          %[p2],      %[p2],      4           \n\t"
        "addu           %[tb3],     %[tb2],     $zero       \n\t"
        "addu           %[tb2],     %[tb1],     $zero       \n\t"
        "addu           %[tb1],     %[tb0],     $zero       \n\t"
        "ulw            %[tb0],     -2(%[p2])               \n\t"
        "shll_s.w       %[s],       %[s],       4           \n\t"
        "addiu          %[i],       %[i],       2           \n\t"
        "addq_s.w       %[s],       %[s],       %[tmp]      \n\t"
        "sra            %[s],       %[s],       16          \n\t"
        "sh             %[s],       0(%[y_t])               \n\t"
        "addiu          %[y_t],     %[y_t], 4               \n\t"
        "j              vAa_lp%=                            \n\t"
        "bBb_lp%=:                                          \n\t"
        : [tb0] "=&r" (tb0), [s] "=&r" (s), [tb1] "=&r" (tb1),
          [tb3] "=&r" (tb3), [tb4] "=&r" (tb4), [tb5] "=&r" (tb5),
          [tb8] "=&r" (tb8), [tb6] "=&r" (tb6), [tb7] "=&r" (tb7),
          [p2] "+r" (p2), [y_t] "+r" (y_t), [i] "=&r" (i), [tb2] "=&r" (tb2)
        : [ta0] "r" (ta0), [ta1] "r" (ta1), [ta2] "r" (ta2),
          [ta3] "r" (ta3), [ta4] "r" (ta4), [ta5] "r" (ta5), [lg] "r" (lg),
          [ta6] "r" (ta6), [ta7] "r" (ta7), [a16] "r" (a16), [tmp] "r" (tmp)
        : "hi", "lo", "memory"
    );

    p2 = &x[1];
    y_t = &y[1];

    __asm__ volatile (
        "ulw            %[tb0],     -2(%[p2])               \n\t"
        "ulw            %[tb1],     -6(%[p2])               \n\t"
        "ulw            %[tb2],     -10(%[p2])              \n\t"
        "ulw            %[tb3],     -14(%[p2])              \n\t"
        "ulw            %[tb4],     -18(%[p2])              \n\t"
        "ulw            %[tb5],     -22(%[p2])              \n\t"
        "ulw            %[tb6],     -26(%[p2])              \n\t"
        "ulw            %[tb7],     -30(%[p2])              \n\t"
        "addu           %[i],       $zero,      $zero       \n\t"
        "ddd_lp%=:                                          \n\t"
        "beq            %[i],       %[lg],      eee_lp%=    \n\t"
        "lh             %[tb8],     -32(%[p2])              \n\t"
        "mult           $ac0,       $zero,      $zero       \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb0],     %[ta0]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb1],     %[ta1]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb2],     %[ta2]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb3],     %[ta3]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb4],     %[ta4]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb5],     %[ta5]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb6],     %[ta6]      \n\t"
        "dpaqx_s.w.ph   $ac0,       %[tb7],     %[ta7]      \n\t"
        "dpaq_sa.l.w    $ac0,       %[tb8],     %[a16]      \n\t"
        "addu           %[tb7],     %[tb6],     $zero       \n\t"
        "addu           %[tb6],     %[tb5],     $zero       \n\t"
        "addu           %[tb5],     %[tb4],     $zero       \n\t"
        "addu           %[tb4],     %[tb3],     $zero       \n\t"
        "mflo           %[s],       $ac0                    \n\t"
        "addiu          %[p2],      %[p2],      4           \n\t"
        "addu           %[tb3],     %[tb2],     $zero       \n\t"
        "addu           %[tb2],     %[tb1],     $zero       \n\t"
        "addu           %[tb1],     %[tb0],     $zero       \n\t"
        "ulw            %[tb0],     -2(%[p2])               \n\t"
        "shll_s.w       %[s],       %[s],       4           \n\t"
        "addiu          %[i],       %[i],       2           \n\t"
        "addq_s.w       %[s],       %[s],       %[tmp]      \n\t"
        "sra            %[s],       %[s],       16          \n\t"
        "sh             %[s],       0(%[y_t])               \n\t"
        "addiu          %[y_t],     %[y_t],     4           \n\t"
        "j              ddd_lp%=                            \n\t"
        "eee_lp%=:                                          \n\t"
        : [tb0] "=&r" (tb0), [s] "=&r" (s), [tb1] "=&r" (tb1),
          [tb3] "=&r" (tb3), [tb4] "=&r" (tb4), [tb5] "=&r" (tb5),
          [tb8] "=&r" (tb8), [tb6] "=&r" (tb6), [tb7] "=&r" (tb7),
          [p2] "+r" (p2), [y_t] "+r" (y_t), [i] "=&r" (i), [tb2] "=&r" (tb2)
        : [ta0] "r" (ta0), [ta1] "r" (ta1), [ta2] "r" (ta2),
          [ta3] "r" (ta3), [ta4] "r" (ta4), [ta5] "r" (ta5), [lg] "r" (lg),
          [ta6] "r" (ta6), [ta7] "r" (ta7), [a16] "r" (a16), [tmp] "r" (tmp)
        : "hi", "lo", "memory"
    );
#else /* MIPS_DSP_R2_LE */
    __asm__ __volatile__ (
        "ulw        %[ta0],     0(%[a])                 \n\t"
        "ulw        %[ta1],     4(%[a])                 \n\t"
        "ulw        %[ta2],     8(%[a])                 \n\t"
        "ulw        %[ta3],     12(%[a])                \n\t"
        "packrl.ph  %[ta0],     %[ta0],     %[ta0]      \n\t"
        "packrl.ph  %[ta1],     %[ta1],     %[ta1]      \n\t"
        "packrl.ph  %[ta2],     %[ta2],     %[ta2]      \n\t"
        "packrl.ph  %[ta3],     %[ta3],     %[ta3]      \n\t"
        "ulw        %[ta4],     16(%[a])                \n\t"
        "ulw        %[ta5],     20(%[a])                \n\t"
        "ulw        %[ta6],     24(%[a])                \n\t"
        "ulw        %[ta7],     28(%[a])                \n\t"
        "packrl.ph  %[ta4],     %[ta4],     %[ta4]      \n\t"
        "packrl.ph  %[ta5],     %[ta5],     %[ta5]      \n\t"
        "packrl.ph  %[ta6],     %[ta6],     %[ta6]      \n\t"
        "packrl.ph  %[ta7],     %[ta7],     %[ta7]      \n\t"
        : [ta0] "=&r" (ta0), [ta1] "=&r" (ta1), [ta2] "=&r" (ta2),
          [ta3] "=&r" (ta3), [ta4] "=&r" (ta4), [ta5] "=&r" (ta5),
          [ta6] "=&r" (ta6), [ta7] "=r" (ta7)
        : [a] "r" (a)
        : "memory"
    );

    a16 = a[16];
    p2 = &x[0];

    __asm__ __volatile__ (
        "ulw            %[tb0],     -2(%[p2])               \n\t"
        "ulw            %[tb1],     -6(%[p2])               \n\t"
        "ulw            %[tb2],     -10(%[p2])              \n\t"
        "ulw            %[tb3],     -14(%[p2])              \n\t"
        "ulw            %[tb4],     -18(%[p2])              \n\t"
        "ulw            %[tb5],     -22(%[p2])              \n\t"
        "ulw            %[tb6],     -26(%[p2])              \n\t"
        "ulw            %[tb7],     -30(%[p2])              \n\t"
        "addu           %[i],       $zero,      $zero       \n\t"
        "vAa_lp%=:                                          \n\t"
        "beq            %[i],       %[lg],      bBb_lp%=    \n\t"
        "lh             %[tb8],     -32(%[p2])              \n\t"
        "mult           $ac0,       $zero,      $zero       \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb0],     %[ta0]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb1],     %[ta1]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb2],     %[ta2]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb3],     %[ta3]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb4],     %[ta4]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb5],     %[ta5]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb6],     %[ta6]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb7],     %[ta7]      \n\t"
        "dpaq_sa.l.w    $ac0,       %[tb8],     %[a16]      \n\t"
        "addu           %[tb7],     %[tb6],     $zero       \n\t"
        "addu           %[tb6],     %[tb5],     $zero       \n\t"
        "addu           %[tb5],     %[tb4],     $zero       \n\t"
        "addu           %[tb4],     %[tb3],     $zero       \n\t"
        "mflo           %[s],       $ac0                    \n\t"
        "addiu          %[p2],      %[p2],      4           \n\t"
        "addu           %[tb3],     %[tb2],     $zero       \n\t"
        "addu           %[tb2],     %[tb1],     $zero       \n\t"
        "addu           %[tb1],     %[tb0],     $zero       \n\t"
        "ulw            %[tb0],     -2(%[p2])               \n\t"
        "shll_s.w       %[s],       %[s],       4           \n\t"
        "addiu          %[i],       %[i],       2           \n\t"
        "addq_s.w       %[s],       %[s],       %[tmp]      \n\t"
        "sra            %[s],       %[s],       16          \n\t"
        "sh             %[s],       0(%[y_t])               \n\t"
        "addiu          %[y_t],     %[y_t],     4           \n\t"
        "j              vAa_lp%=                            \n\t"
        "bBb_lp%=:                                          \n\t"
        : [tb0] "=&r" (tb0), [s] "=&r" (s), [tb1] "=&r" (tb1),
          [tb2] "=&r" (tb2), [tb3] "=&r" (tb3), [tb4] "=&r" (tb4),
          [tb5] "=&r" (tb5), [tb8] "=&r" (tb8), [tb6] "=&r" (tb6),
          [tb7] "=&r" (tb7), [p2] "+r" (p2), [y_t] "+r" (y_t),
          [i] "=&r" (i)
        : [ta0] "r" (ta0), [ta1] "r" (ta1), [ta2] "r" (ta2),
          [ta3] "r" (ta3), [ta4] "r" (ta4), [ta5] "r" (ta5),
          [lg] "r" (lg), [ta6] "r" (ta6), [ta7] "r" (ta7),
          [a16] "r" (a16), [tmp] "r" (tmp)
        : "hi", "lo", "memory"
    );

    p2 = &x[1];
    y_t = &y[1];

    __asm__ __volatile__ (
        "ulw            %[tb0],     -2(%[p2])               \n\t"
        "ulw            %[tb1],     -6(%[p2])               \n\t"
        "ulw            %[tb2],     -10(%[p2])              \n\t"
        "ulw            %[tb3],     -14(%[p2])              \n\t"
        "ulw            %[tb4],     -18(%[p2])              \n\t"
        "ulw            %[tb5],     -22(%[p2])              \n\t"
        "ulw            %[tb6],     -26(%[p2])              \n\t"
        "ulw            %[tb7],     -30(%[p2])              \n\t"
        "addu           %[i],       $zero,      $zero       \n\t"
        "ddd_lp%=:                                          \n\t"
        "beq            %[i],       %[lg],      eee_lp%=    \n\t"
        "lh             %[tb8],     -32(%[p2])              \n\t"
        "mult           $ac0,       $zero,      $zero       \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb0],     %[ta0]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb1],     %[ta1]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb2],     %[ta2]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb3],     %[ta3]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb4],     %[ta4]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb5],     %[ta5]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb6],     %[ta6]      \n\t"
        "dpaq_s.w.ph    $ac0,       %[tb7],     %[ta7]      \n\t"
        "dpaq_sa.l.w    $ac0,       %[tb8],     %[a16]      \n\t"
        "addu           %[tb7],     %[tb6],     $zero       \n\t"
        "addu           %[tb6],     %[tb5],     $zero       \n\t"
        "addu           %[tb5],     %[tb4],     $zero       \n\t"
        "addu           %[tb4],     %[tb3],     $zero       \n\t"
        "mflo           %[s],       $ac0                    \n\t"
        "addiu          %[p2],      %[p2],      4           \n\t"
        "addu           %[tb3],     %[tb2],     $zero       \n\t"
        "addu           %[tb2],     %[tb1],     $zero       \n\t"
        "addu           %[tb1],     %[tb0],     $zero       \n\t"
        "ulw            %[tb0],     -2(%[p2])               \n\t"
        "shll_s.w       %[s],       %[s],       4           \n\t"
        "addiu          %[i],       %[i],       2           \n\t"
        "addq_s.w       %[s],       %[s],       %[tmp]      \n\t"
        "sra            %[s],       %[s],       16          \n\t"
        "sh             %[s],       0(%[y_t])               \n\t"
        "addiu          %[y_t],     %[y_t],     4           \n\t"
        "j              ddd_lp%=                            \n\t"
        "eee_lp%=:                                          \n\t"
        : [tb0] "=&r" (tb0), [s] "=&r" (s), [tb1] "=&r" (tb1),
          [tb2] "=&r" (tb2), [tb3] "=&r" (tb3), [tb4] "=&r" (tb4),
          [tb5] "=&r" (tb5), [tb8] "=&r" (tb8), [tb6] "=&r" (tb6),
          [tb7] "=&r" (tb7), [p2] "+r" (p2), [y_t] "+r" (y_t),
          [i] "=&r" (i)
        : [ta0] "r" (ta0), [ta1] "r" (ta1), [ta2] "r" (ta2),
          [ta3] "r" (ta3), [ta4] "r" (ta4), [ta5] "r" (ta5),
          [lg] "r" (lg), [ta6] "r" (ta6), [ta7] "r" (ta7),
          [a16] "r" (a16), [tmp] "r" (tmp)
        : "hi", "lo", "memory"
    );
#endif /* MIPS_DSP_R2_LE */

    return;
}



