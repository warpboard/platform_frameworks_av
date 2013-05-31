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
*       File: qpisf_2s_mips.c                                          *
*                                                                      *
*       Description: Coding/Decodeing of ISF parameters with predication
*       The ISF vector is quantized using two-stage VQ with split-by-2 *
*       in 1st stage and split-by-5(or 3) in the second stage          *
*                                                                      *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "cnst.h"
#include "acelp.h"

#define N_SURV_MAX 4                       /* 4 survivors max */

Word16 Sub_VQ_mips(                           /* output: return quantization index     */
        Word16 * x,                           /* input : ISF residual vector           */
        Word16 * dico,                        /* input : quantization codebook         */
        Word16 dim,                           /* input : dimention of vector           */
        Word16 dico_size,                     /* input : size of quantization codebook */
        Word32 * distance                     /* output: error of quantization         */
        )
{
    Word16 *p_dico;
    Word32 i, index;
    Word32 dist_min;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5;
    Word32 dist1, dist2, dist3, dist4;
    Word16 *x1, *x_end;
#if !defined(MIPS_DSP_R1_LE)
    Word16 *p1, *p2, *p3;
#else
    Word32 Temp6;
    Word32 coeff1, coeff2, coeff3;
#endif /* !defined(MIPS_DSP_R1_LE) */

    dist_min = MAX_32;
    p_dico = dico;
    index = 0;
    for (i = 0; i < dico_size; i += 4)
    {
        x1 = x;
        x_end = x1 + dim;
#if defined(MIPS_DSP_R1_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $ac0,       $0,         $0          \n\t"
            "mult       $ac1,       $0,         $0          \n\t"
            "mult       $ac2,       $0,         $0          \n\t"
            "subu       %[Temp6],   %[x_end],   %[x1]       \n\t"
            "blez       %[Temp6],   2f                      \n\t"
            " mult      $ac3,       $0,         $0          \n\t"
            "sll        %[coeff1],  %[dim],     1           \n\t"
            "sll        %[coeff2],  %[dim],     2           \n\t"
            "addu       %[coeff3],  %[coeff1],  %[coeff2]   \n\t"
          "1:                                               \n\t"
            "lh         %[Temp1],   0(%[x1])                \n\t"
            "lh         %[Temp2],   0(%[p_dico])            \n\t"
            "lhx        %[Temp3],   %[coeff1](%[p_dico])    \n\t"
            "lhx        %[Temp4],   %[coeff2](%[p_dico])    \n\t"
            "lhx        %[Temp5],   %[coeff3](%[p_dico])    \n\t"
            "addiu      %[Temp6],   %[Temp6],   -2          \n\t"
            "addiu      %[x1],      %[x1],      2           \n\t"
            "subu       %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
            "subu       %[Temp3],   %[Temp1],   %[Temp3]    \n\t"
            "subu       %[Temp4],   %[Temp1],   %[Temp4]    \n\t"
            "subu       %[Temp5],   %[Temp1],   %[Temp5]    \n\t"
            "madd       $ac0,       %[Temp2],   %[Temp2]    \n\t"
            "madd       $ac1,       %[Temp3],   %[Temp3]    \n\t"
            "madd       $ac2,       %[Temp4],   %[Temp4]    \n\t"
            "madd       $ac3,       %[Temp5],   %[Temp5]    \n\t"
            "bgtz       %[Temp6],   1b                      \n\t"
            " addiu     %[p_dico],  %[p_dico],  2           \n\t"
          "2:                                               \n\t"
            "mflo       %[dist1],   $ac0                    \n\t"
            "mflo       %[dist2],   $ac1                    \n\t"
            "mflo       %[dist3],   $ac2                    \n\t"
            "mflo       %[dist4],   $ac3                    \n\t"
            "addu       %[p_dico],  %[p_dico],  %[coeff3]   \n\t"
            "sll        %[dist1],   %[dist1],   1           \n\t"
            "sll        %[dist2],   %[dist2],   1           \n\t"
            "sll        %[dist3],   %[dist3],   1           \n\t"
            "sll        %[dist4],   %[dist4],   1           \n\t"

            ".set pop                                       \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [coeff1] "=&r" (coeff1), [coeff2] "=&r" (coeff2),
              [coeff3] "=&r" (coeff3), [dist1]  "=&r" (dist1),
              [dist2]  "=&r" (dist2),  [dist3]  "=&r" (dist3),
              [dist4]  "=&r" (dist4),  [p_dico] "+r"  (p_dico),
              [x1]     "+r"  (x1)
            : [dim] "r" (dim), [x_end] "r" (x_end)
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi",
              "$ac2lo", "$ac3hi", "$ac3lo", "memory"
        );
#else
        p1 = &p_dico[dim];
        p2 = &p_dico[2*dim];
        p3 = &p_dico[3*dim];

        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $0,         $0                      \n\t"
            "li         %[dist1],   0                       \n\t"
            "li         %[dist2],   0                       \n\t"
            "li         %[dist3],   0                       \n\t"
          "1:                                               \n\t"
            "lh         %[Temp1],   0(%[x1])                \n\t"
            "lh         %[Temp2],   0(%[p_dico])            \n\t"
            "lh         %[Temp3],   0(%[p1])                \n\t"
            "lh         %[Temp4],   0(%[p2])                \n\t"
            "lh         %[Temp5],   0(%[p3])                \n\t"
            "subu       %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
            "mul        %[Temp2],   %[Temp2],   %[Temp2]    \n\t"
            "subu       %[Temp3],   %[Temp1],   %[Temp3]    \n\t"
            "mul        %[Temp3],   %[Temp3],   %[Temp3]    \n\t"
            "subu       %[Temp4],   %[Temp1],   %[Temp4]    \n\t"
            "mul        %[Temp4],   %[Temp4],   %[Temp4]    \n\t"
            "subu       %[Temp5],   %[Temp1],   %[Temp5]    \n\t"
            "madd       %[Temp5],   %[Temp5]                \n\t"
            "addiu      %[p_dico],  %[p_dico],  2           \n\t"
            "addiu      %[x1],      %[x1],      2           \n\t"
            "addiu      %[p1],      %[p1],      2           \n\t"
            "addiu      %[p2],      %[p2],      2           \n\t"
            "addiu      %[p3],      %[p3],      2           \n\t"
            "addu       %[dist1],   %[dist1],   %[Temp2]    \n\t"
            "addu       %[dist2],   %[dist2],   %[Temp3]    \n\t"
            "bne        %[x1],      %[x_end],   1b          \n\t"
            " addu      %[dist3],   %[dist3],   %[Temp4]    \n\t"
            "mflo       %[dist4]                            \n\t"
            "sll        %[dist1],   %[dist1],   1           \n\t"
            "sll        %[dist2],   %[dist2],   1           \n\t"
            "sll        %[dist3],   %[dist3],   1           \n\t"
            "sll        %[dist4],   %[dist4],   1           \n\t"

            ".set pop                                   \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2]  "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4]  "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [dist1]  "=&r" (dist1),
              [dist2] "=&r" (dist2), [dist3]  "=&r" (dist3),
              [dist4] "=&r" (dist4), [p1]     "+r"  (p1),
              [p2]    "+r"  (p2),    [p3]     "+r"  (p3),
              [x1]    "+r"  (x1),    [p_dico] "+r"  (p_dico)
            : [x_end] "r" (x_end)
            : "hi", "lo", "memory"
        );

        p_dico += 3*dim;
#endif /* defined(MIPS_DSP_R1_LE) */

        if(dist1 < dist_min)
        {
            dist_min = dist1;
            index = i;
        }

        if(dist2 < dist_min)
        {
            dist_min = dist2;
            index = i + 1;
        }

        if(dist3 < dist_min)
        {
            dist_min = dist3;
            index = i + 2;
        }

        if(dist4 < dist_min)
        {
            dist_min = dist4;
            index = i + 3;
        }
    }

    *distance = dist_min;

    /* Reading the selected vector */
    p_dico = &dico[index * dim];
    memcpy (x, p_dico, dim * sizeof(Word16));

    return index;
}

void VQ_stage1_mips(
        Word16 * x,                           /* input : ISF residual vector           */
        Word16 * dico,                        /* input : quantization codebook         */
        Word16 dim,                           /* input : dimention of vector           */
        Word16 dico_size,                     /* input : size of quantization codebook */
        Word16 * index,                       /* output: indices of survivors          */
        Word16 surv                           /* input : number of survivor            */
        )
{
    Word16 *p_dico;
    Word32 i, k, l;
    Word32 dist_min[N_SURV_MAX], dist;

    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8;
    Word32 val = 9;
    Word16 *x1;

    dist_min[0] = MAX_32;
    dist_min[1] = MAX_32;
    dist_min[2] = MAX_32;
    dist_min[3] = MAX_32;
    index[0] = 0;
    index[1] = 1;
    index[2] = 2;
    index[3] = 3;

    p_dico = dico;

    for (i = 0; i < dico_size; i++)
    {
        x1 = x;
#if defined(MIPS_DSP_R2_LE)
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "ulw            %[Temp1],   0(%[x1])                \n\t"
            "ulw            %[Temp2],   0(%[p_dico])            \n\t"
            "ulw            %[Temp3],   4(%[x1])                \n\t"
            "ulw            %[Temp4],   4(%[p_dico])            \n\t"
            "ulw            %[Temp5],   8(%[x1])                \n\t"
            "ulw            %[Temp6],   8(%[p_dico])            \n\t"
            "lh             %[Temp7],   12(%[x1])               \n\t"
            "lh             %[Temp8],   12(%[p_dico])           \n\t"
            "subq.ph        %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
            "subq.ph        %[Temp2],   %[Temp3],   %[Temp4]    \n\t"
            "subq.ph        %[Temp3],   %[Temp5],   %[Temp6]    \n\t"
            "subu           %[Temp4],   %[Temp7],   %[Temp8]    \n\t"
            "mult           $ac0,       $0,         $0          \n\t"
            "dpa.w.ph       $ac0,       %[Temp1],   %[Temp1]    \n\t"
            "dpa.w.ph       $ac0,       %[Temp2],   %[Temp2]    \n\t"
            "dpa.w.ph       $ac0,       %[Temp3],   %[Temp3]    \n\t"
            "madd           $ac0,       %[Temp4],   %[Temp4]    \n\t"
            "bne            %[dim],     %[val],     1f          \n\t"
            " addiu         %[p_dico],  %[p_dico],  14          \n\t"
            "ulw            %[Temp1],   14(%[x1])               \n\t"
            "ulw            %[Temp2],   0(%[p_dico])            \n\t"
            "addiu          %[p_dico],  %[p_dico],  4           \n\t"
            "subq.ph        %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph       $ac0,       %[Temp1],   %[Temp1]    \n\t"
          "1:                                                   \n\t"
            "mflo           %[dist],    $ac0                    \n\t"
            "sll            %[dist],    %[dist],    1           \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2]  "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4]  "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6]  "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8]  "=&r" (Temp8),
              [dist]  "=&r" (dist),  [p_dico] "+r"  (p_dico)
            : [x1]  "r" (x1), [val] "r" (val),
              [dim] "r" (dim)
            : "hi", "lo", "memory"
        );
#elif defined(MIPS_DSP_R1_LE)
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "ulw            %[Temp1],   0(%[x1])                \n\t"
            "ulw            %[Temp2],   0(%[p_dico])            \n\t"
            "ulw            %[Temp3],   4(%[x1])                \n\t"
            "ulw            %[Temp4],   4(%[p_dico])            \n\t"
            "ulw            %[Temp5],   8(%[x1])                \n\t"
            "ulw            %[Temp6],   8(%[p_dico])            \n\t"
            "lh             %[Temp7],   12(%[x1])               \n\t"
            "lh             %[Temp8],   12(%[p_dico])           \n\t"
            "subq.ph        %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
            "subq.ph        %[Temp3],   %[Temp3],   %[Temp4]    \n\t"
            "subq.ph        %[Temp5],   %[Temp5],   %[Temp6]    \n\t"
            "subu           %[Temp7],   %[Temp7],   %[Temp8]    \n\t"
            "mult           $ac0,       $0,         $0          \n\t"
            "maq_s.w.phr    $ac0,       %[Temp1],   %[Temp1]    \n\t"
            "maq_s.w.phl    $ac0,       %[Temp1],   %[Temp1]    \n\t"
            "maq_s.w.phr    $ac0,       %[Temp3],   %[Temp3]    \n\t"
            "maq_s.w.phl    $ac0,       %[Temp3],   %[Temp3]    \n\t"
            "maq_s.w.phr    $ac0,       %[Temp5],   %[Temp5]    \n\t"
            "maq_s.w.phl    $ac0,       %[Temp5],   %[Temp5]    \n\t"
            "bne            %[dim],     %[val],     1f          \n\t"
            " maq_s.w.phr   $ac0,       %[Temp7],   %[Temp7]    \n\t"
            "lh             %[Temp1],   14(%[x1])               \n\t"
            "lh             %[Temp2],   14(%[p_dico])           \n\t"
            "lh             %[Temp3],   16(%[x1])               \n\t"
            "lh             %[Temp4],   16(%[p_dico])           \n\t"
            "subu           %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
            "subu           %[Temp3],   %[Temp3],   %[Temp4]    \n\t"
            "maq_s.w.phr    $ac0,       %[Temp1],   %[Temp1]    \n\t"
            "maq_s.w.phr    $ac0,       %[Temp3],   %[Temp3]    \n\t"
          "1:                                                   \n\t"
            "mflo           %[dist],    $ac0                    \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [dist]  "=r"  (dist)
            : [x1]  "r" (x),   [p_dico] "r" (p_dico),
              [val] "r" (val), [dim]    "r" (dim)
            : "hi", "lo", "memory"
        );

        p_dico += dim;
#else
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "lh             %[Temp1],   0(%[x1])                \n\t"
            "lh             %[Temp2],   0(%[p_dico])            \n\t"
            "lh             %[Temp3],   2(%[x1])                \n\t"
            "lh             %[Temp4],   2(%[p_dico])            \n\t"
            "lh             %[Temp5],   4(%[x1])                \n\t"
            "lh             %[Temp6],   4(%[p_dico])            \n\t"
            "lh             %[Temp7],   6(%[x1])                \n\t"
            "lh             %[Temp8],   6(%[p_dico])            \n\t"
            "subu           %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
            "subu           %[Temp2],   %[Temp3],   %[Temp4]    \n\t"
            "subu           %[Temp3],   %[Temp5],   %[Temp6]    \n\t"
            "subu           %[Temp4],   %[Temp7],   %[Temp8]    \n\t"
            "mult           %[Temp1],   %[Temp1]                \n\t"
            "madd           %[Temp2],   %[Temp2]                \n\t"
            "madd           %[Temp3],   %[Temp3]                \n\t"
            "madd           %[Temp4],   %[Temp4]                \n\t"
            "lh             %[Temp1],   8(%[x1])                \n\t"
            "lh             %[Temp2],   8(%[p_dico])            \n\t"
            "lh             %[Temp3],   10(%[x1])               \n\t"
            "lh             %[Temp4],   10(%[p_dico])           \n\t"
            "lh             %[Temp5],   12(%[x1])               \n\t"
            "lh             %[Temp6],   12(%[p_dico])           \n\t"
            "subu           %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
            "subu           %[Temp2],   %[Temp3],   %[Temp4]    \n\t"
            "subu           %[Temp3],   %[Temp5],   %[Temp6]    \n\t"
            "madd           %[Temp1],   %[Temp1]                \n\t"
            "madd           %[Temp2],   %[Temp2]                \n\t"
            "bne            %[dim],     %[val],     1f          \n\t"
            " madd          %[Temp3],   %[Temp3]                \n\t"
            "lh             %[Temp1],   14(%[x1])               \n\t"
            "lh             %[Temp2],   14(%[p_dico])           \n\t"
            "lh             %[Temp3],   16(%[x1])               \n\t"
            "lh             %[Temp4],   16(%[p_dico])           \n\t"
            "subu           %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
            "subu           %[Temp2],   %[Temp3],   %[Temp4]    \n\t"
            "madd           %[Temp1],   %[Temp1]                \n\t"
            "madd           %[Temp2],   %[Temp2]                \n\t"
          "1:                                                   \n\t"
            "mflo           %[dist]                             \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [dist]  "=r"  (dist)
            : [x1]  "r" (x1),  [p_dico] "r" (p_dico),
              [val] "r" (val), [dim]    "r" (dim)
            : "hi", "lo", "memory"
        );

        dist <<= 1;
        p_dico += dim;
#endif /* defined(MIPS_DSP_R2_LE) */

        for (k = 0; k < surv; k++)
        {
            if(dist < dist_min[k])
            {
                for (l = surv - 1; l > k; l--)
                {
                    dist_min[l] = dist_min[l - 1];
                    index[l] = index[l - 1];
                }
                dist_min[k] = dist;
                index[k] = i;
                break;
            }
        }
    }
    return;
}




