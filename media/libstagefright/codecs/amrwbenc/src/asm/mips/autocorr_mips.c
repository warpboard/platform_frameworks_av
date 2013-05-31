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
*       File: autocorr_mips.c                                          *
*                                                                      *
*       Description:Compute autocorrelations of signal with windowing  *
*                                                                      *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "acelp.h"

extern const Word16 vo_window[384];

void Autocorr_mips(
        Word16 x[],                           /* (i)    : Input signal                      */
        Word16 m,                             /* (i)    : LPC order                         */
        Word16 r_h[],                         /* (o) Q15: Autocorrelations  (msb)           */
        Word16 r_l[]                          /* (o)    : Autocorrelations  (lsb)           */
        )
{
    Word32 i, norm, shift;
    Word16 y[L_WINDOW];
    Word32 L_sum, L_sum1, F_LEN;
    Word16 *p1,*p2,*p3;
    const Word16 *p4;
    Word16 *ptr_end;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6,
            Temp7, Temp8, Temp9, Temp10, Temp11, Temp12;
#if !defined(MIPS_DSP_R1_LE)
    Word32 Temp13;
#endif /* !defined(MIPS_DSP_R1_LE) */

    p1 = x;
    p4 = vo_window;
    p3 = y;
    ptr_end = p1 + L_WINDOW;
#if defined(MIPS_DSP_R2_LE)
    __asm__ volatile (
        ".set push                                          \n\t"
        ".set noreorder                                     \n\t"

        "mult           $ac1,       $0,         $0          \n\t"
      "1:                                                   \n\t"
        "ulw            %[Temp1],   0(%[p1])                \n\t"
        "ulw            %[Temp2],   0(%[p4])                \n\t"
        "ulw            %[Temp3],   4(%[p1])                \n\t"
        "ulw            %[Temp4],   4(%[p4])                \n\t"
        "ulw            %[Temp5],   8(%[p1])                \n\t"
        "ulw            %[Temp6],   8(%[p4])                \n\t"
        "ulw            %[Temp7],   12(%[p1])               \n\t"
        "ulw            %[Temp8],   12(%[p4])               \n\t"
        "addiu          %[p1],      %[p1],      16          \n\t"
        "addiu          %[p4],      %[p4],      16          \n\t"
        "muleq_s.w.phr  %[Temp9],   %[Temp1],   %[Temp2]    \n\t"
        "muleq_s.w.phr  %[Temp10],  %[Temp3],   %[Temp4]    \n\t"
        "muleq_s.w.phr  %[Temp11],  %[Temp5],   %[Temp6]    \n\t"
        "muleq_s.w.phr  %[Temp12],  %[Temp7],   %[Temp8]    \n\t"
        "muleq_s.w.phl  %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
        "muleq_s.w.phl  %[Temp2],   %[Temp3],   %[Temp4]    \n\t"
        "muleq_s.w.phl  %[Temp3],   %[Temp5],   %[Temp6]    \n\t"
        "muleq_s.w.phl  %[Temp4],   %[Temp7],   %[Temp8]    \n\t"
        "precrq_rs.ph.w %[Temp1],   %[Temp1],   %[Temp9]    \n\t"
        "precrq_rs.ph.w %[Temp2],   %[Temp2],   %[Temp10]   \n\t"
        "precrq_rs.ph.w %[Temp3],   %[Temp3],   %[Temp11]   \n\t"
        "precrq_rs.ph.w %[Temp4],   %[Temp4],   %[Temp12]   \n\t"
        "dpa.w.ph       $ac1,       %[Temp1],   %[Temp1]    \n\t"
        "dpa.w.ph       $ac1,       %[Temp2],   %[Temp2]    \n\t"
        "dpa.w.ph       $ac1,       %[Temp3],   %[Temp3]    \n\t"
        "dpa.w.ph       $ac1,       %[Temp4],   %[Temp4]    \n\t"
        "usw            %[Temp1],   0(%[p3])                \n\t"
        "usw            %[Temp2],   4(%[p3])                \n\t"
        "usw            %[Temp3],   8(%[p3])                \n\t"
        "usw            %[Temp4],   12(%[p3])               \n\t"
        "bne            %[p1],      %[ptr_end], 1b          \n\t"
        " addiu         %[p3],      %[p3],      16          \n\t"
        "extr.w         %[L_sum],   $ac1,       7           \n\t"
        "lui            %[Temp1],   0x10                    \n\t"
        "addu           %[L_sum],   %[L_sum],   %[Temp1]    \n\t"
        "sra            %[norm],    %[L_sum],   31          \n\t"
        "xor            %[norm],    %[norm],    %[L_sum]    \n\t"
        "clz            %[norm],    %[norm]                 \n\t"
        "addiu          %[norm],    %[norm],    -1          \n\t"

        ".set pop                                           \n\t"

        : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
          [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
          [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
          [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
          [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
          [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
          [L_sum]  "=&r" (L_sum),  [norm]   "=&r" (norm),
          [p1]     "+r"  (p1),     [p3]     "+r"  (p3),
          [p4]     "+r"  (p4)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "$ac1hi", "$ac1lo",
          "memory"
    );
#elif defined(MIPS_DSP_R1_LE)
    __asm__ volatile (
        ".set push                                          \n\t"
        ".set noreorder                                     \n\t"

        "mult           $ac1,       $0,         $0          \n\t"
      "1:                                                   \n\t"
        "ulw            %[Temp1],   0(%[p1])                \n\t"
        "ulw            %[Temp2],   0(%[p4])                \n\t"
        "ulw            %[Temp3],   4(%[p1])                \n\t"
        "ulw            %[Temp4],   4(%[p4])                \n\t"
        "ulw            %[Temp5],   8(%[p1])                \n\t"
        "ulw            %[Temp6],   8(%[p4])                \n\t"
        "ulw            %[Temp7],   12(%[p1])               \n\t"
        "ulw            %[Temp8],   12(%[p4])               \n\t"
        "addiu          %[p1],      %[p1],      16          \n\t"
        "addiu          %[p4],      %[p4],      16          \n\t"
        "muleq_s.w.phr  %[Temp9],   %[Temp1],   %[Temp2]    \n\t"
        "muleq_s.w.phr  %[Temp10],  %[Temp3],   %[Temp4]    \n\t"
        "muleq_s.w.phr  %[Temp11],  %[Temp5],   %[Temp6]    \n\t"
        "muleq_s.w.phr  %[Temp12],  %[Temp7],   %[Temp8]    \n\t"
        "muleq_s.w.phl  %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
        "muleq_s.w.phl  %[Temp4],   %[Temp3],   %[Temp4]    \n\t"
        "muleq_s.w.phl  %[Temp6],   %[Temp5],   %[Temp6]    \n\t"
        "muleq_s.w.phl  %[Temp8],   %[Temp7],   %[Temp8]    \n\t"
        "shra_r.w       %[Temp1],   %[Temp9],   16          \n\t"
        "shra_r.w       %[Temp3],   %[Temp10],  16          \n\t"
        "shra_r.w       %[Temp5],   %[Temp11],  16          \n\t"
        "shra_r.w       %[Temp7],   %[Temp12],  16          \n\t"
        "shra_r.w       %[Temp2],   %[Temp2],   16          \n\t"
        "shra_r.w       %[Temp4],   %[Temp4],   16          \n\t"
        "shra_r.w       %[Temp6],   %[Temp6],   16          \n\t"
        "shra_r.w       %[Temp8],   %[Temp8],   16          \n\t"
        "madd           $ac1,       %[Temp1],   %[Temp1]    \n\t"
        "madd           $ac1,       %[Temp2],   %[Temp2]    \n\t"
        "madd           $ac1,       %[Temp3],   %[Temp3]    \n\t"
        "madd           $ac1,       %[Temp4],   %[Temp4]    \n\t"
        "madd           $ac1,       %[Temp5],   %[Temp5]    \n\t"
        "madd           $ac1,       %[Temp6],   %[Temp6]    \n\t"
        "madd           $ac1,       %[Temp7],   %[Temp7]    \n\t"
        "madd           $ac1,       %[Temp8],   %[Temp8]    \n\t"
        "sh             %[Temp1],   0(%[p3])                \n\t"
        "sh             %[Temp2],   2(%[p3])                \n\t"
        "sh             %[Temp3],   4(%[p3])                \n\t"
        "sh             %[Temp4],   6(%[p3])                \n\t"
        "sh             %[Temp5],   8(%[p3])                \n\t"
        "sh             %[Temp6],   10(%[p3])               \n\t"
        "sh             %[Temp7],   12(%[p3])               \n\t"
        "sh             %[Temp8],   14(%[p3])               \n\t"
        "bne            %[p1],      %[ptr_end], 1b          \n\t"
        " addiu         %[p3],      %[p3],      16          \n\t"
        "extr.w         %[L_sum],   $ac1,       7           \n\t"
        "lui            %[Temp1],   0x10                    \n\t"
        "addu           %[L_sum],   %[L_sum],   %[Temp1]    \n\t"
        "sra            %[norm],    %[L_sum],   31          \n\t"
        "xor            %[norm],    %[norm],    %[L_sum]    \n\t"
        "clz            %[norm],    %[norm]                 \n\t"
        "addiu          %[norm],    %[norm],    -1          \n\t"

        ".set pop                                           \n\t"

        : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
          [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
          [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
          [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
          [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
          [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
          [L_sum]  "=&r" (L_sum),  [norm]   "=&r" (norm),
          [p1]     "+r"  (p1),     [p3]     "+r"  (p3),
          [p4]     "+r"  (p4)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "$ac1hi", "$ac1lo",
          "memory"
    );
#else
    __asm__ volatile (
        ".set push                                          \n\t"
        ".set noreorder                                     \n\t"

        "mult           $0,         $0                      \n\t"
      "1:                                                   \n\t"
        "lh             %[Temp1],   0(%[p1])                \n\t"
        "lh             %[Temp2],   0(%[p4])                \n\t"
        "lh             %[Temp3],   2(%[p1])                \n\t"
        "lh             %[Temp4],   2(%[p4])                \n\t"
        "lh             %[Temp5],   4(%[p1])                \n\t"
        "lh             %[Temp6],   4(%[p4])                \n\t"
        "lh             %[Temp7],   6(%[p1])                \n\t"
        "lh             %[Temp8],   6(%[p4])                \n\t"
        "mul            %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
        "mul            %[Temp2],   %[Temp3],   %[Temp4]    \n\t"
        "mul            %[Temp3],   %[Temp5],   %[Temp6]    \n\t"
        "mul            %[Temp4],   %[Temp7],   %[Temp8]    \n\t"
        "lh             %[Temp5],   8(%[p1])                \n\t"
        "lh             %[Temp6],   8(%[p4])                \n\t"
        "lh             %[Temp7],   10(%[p1])               \n\t"
        "lh             %[Temp8],   10(%[p4])               \n\t"
        "lh             %[Temp9],   12(%[p1])               \n\t"
        "lh             %[Temp10],  12(%[p4])               \n\t"
        "lh             %[Temp11],  14(%[p1])               \n\t"
        "lh             %[Temp12],  14(%[p4])               \n\t"
        "addiu          %[p1],      %[p1],      16          \n\t"
        "addiu          %[p4],      %[p4],      16          \n\t"
        "mul            %[Temp5],   %[Temp5],   %[Temp6]    \n\t"
        "mul            %[Temp6],   %[Temp7],   %[Temp8]    \n\t"
        "mul            %[Temp7],   %[Temp9],   %[Temp10]   \n\t"
        "mul            %[Temp8],   %[Temp11],  %[Temp12]   \n\t"
        "addiu          %[Temp1],   %[Temp1],   0x4000      \n\t"
        "addiu          %[Temp2],   %[Temp2],   0x4000      \n\t"
        "addiu          %[Temp3],   %[Temp3],   0x4000      \n\t"
        "addiu          %[Temp4],   %[Temp4],   0x4000      \n\t"
        "sra            %[Temp1],   %[Temp1],   15          \n\t"
        "sra            %[Temp2],   %[Temp2],   15          \n\t"
        "sra            %[Temp3],   %[Temp3],   15          \n\t"
        "sra            %[Temp4],   %[Temp4],   15          \n\t"
        "addiu          %[Temp5],   %[Temp5],   0x4000      \n\t"
        "addiu          %[Temp6],   %[Temp6],   0x4000      \n\t"
        "addiu          %[Temp7],   %[Temp7],   0x4000      \n\t"
        "addiu          %[Temp8],   %[Temp8],   0x4000      \n\t"
        "sra            %[Temp5],   %[Temp5],   15          \n\t"
        "sra            %[Temp6],   %[Temp6],   15          \n\t"
        "sra            %[Temp7],   %[Temp7],   15          \n\t"
        "sra            %[Temp8],   %[Temp8],   15          \n\t"
        "madd           %[Temp1],   %[Temp1]                \n\t"
        "madd           %[Temp2],   %[Temp2]                \n\t"
        "madd           %[Temp3],   %[Temp3]                \n\t"
        "madd           %[Temp4],   %[Temp4]                \n\t"
        "madd           %[Temp5],   %[Temp5]                \n\t"
        "madd           %[Temp6],   %[Temp6]                \n\t"
        "madd           %[Temp7],   %[Temp7]                \n\t"
        "madd           %[Temp8],   %[Temp8]                \n\t"
        "sh             %[Temp1],   0(%[p3])                \n\t"
        "sh             %[Temp2],   2(%[p3])                \n\t"
        "sh             %[Temp3],   4(%[p3])                \n\t"
        "sh             %[Temp4],   6(%[p3])                \n\t"
        "sh             %[Temp5],   8(%[p3])                \n\t"
        "sh             %[Temp6],   10(%[p3])               \n\t"
        "sh             %[Temp7],   12(%[p3])               \n\t"
        "sh             %[Temp8],   14(%[p3])               \n\t"
        "bne            %[p1],      %[ptr_end], 1b          \n\t"
        " addiu         %[p3],      %[p3],      16          \n\t"
        "mfhi           %[Temp1]                            \n\t"
        "mflo           %[L_sum]                            \n\t"
        "sll            %[Temp1],   %[Temp1],   25          \n\t"
        "srl            %[L_sum],   %[L_sum],   7           \n\t"
        "or             %[L_sum],   %[L_sum],   %[Temp1]    \n\t"

        ".set pop                                           \n\t"

        : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
          [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
          [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
          [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
          [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
          [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
          [L_sum]  "=&r" (L_sum),  [p1]     "+r"  (p1),
          [p3]     "+r"  (p3),     [p4]     "+r"  (p4)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "memory"
    );

    L_sum += 0x100000;
    norm = norm_l(L_sum);
#endif /* defined(MIPS_DSP_R2_LE) */

    shift = 4 - (norm >> 1);
    p1 = y;
    ptr_end = p1 + L_WINDOW;

#if defined(MIPS_DSP_R2_LE)
    if (shift > 0)
    {
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult       $ac0,           $0,         $0          \n\t"
          "1:                                                   \n\t"
            "ulw        %[Temp1],       0(%[p1])                \n\t"
            "ulw        %[Temp2],       4(%[p1])                \n\t"
            "ulw        %[Temp3],       8(%[p1])                \n\t"
            "ulw        %[Temp4],       12(%[p1])               \n\t"
            "shrav_r.ph %[Temp1],       %[Temp1],   %[shift]    \n\t"
            "shrav_r.ph %[Temp2],       %[Temp2],   %[shift]    \n\t"
            "shrav_r.ph %[Temp3],       %[Temp3],   %[shift]    \n\t"
            "shrav_r.ph %[Temp4],       %[Temp4],   %[shift]    \n\t"
            "usw        %[Temp1],       0(%[p1])                \n\t"
            "usw        %[Temp2],       4(%[p1])                \n\t"
            "usw        %[Temp3],       8(%[p1])                \n\t"
            "usw        %[Temp4],       12(%[p1])               \n\t"
            "addiu      %[p1],          %[p1],      16          \n\t"
            "dpa.w.ph   $ac0,           %[Temp1],   %[Temp1]    \n\t"
            "dpa.w.ph   $ac0,           %[Temp2],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac0,           %[Temp3],   %[Temp3]    \n\t"
            "bne        %[p1],          %[ptr_end], 1b          \n\t"
            " dpa.w.ph  $ac0,           %[Temp4],   %[Temp4]    \n\t"
            "shilo      $ac0,           -1                      \n\t"
            "mflo       %[L_sum],       $ac0                    \n\t"
            "addiu      %[L_sum],       %[L_sum],   1           \n\t"
            "sra        %[norm],        %[L_sum],   31          \n\t"
            "xor        %[norm],        %[norm],    %[L_sum]    \n\t"
            "clz        %[norm],        %[norm]                 \n\t"
            "addiu      %[norm],        %[norm],    -1          \n\t"
            "sllv       %[L_sum],       %[L_sum],   %[norm]     \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [L_sum] "=&r" (L_sum), [norm]  "=&r" (norm),
              [p1]    "+r"  (p1)
            : [shift] "r" (shift), [ptr_end] "r" (ptr_end)
            : "hi", "lo", "memory"
        );
    } else {
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult       $ac0,           $0,         $0          \n\t"
          "1:                                                   \n\t"
            "ulw        %[Temp1],       0(%[p1])                \n\t"
            "ulw        %[Temp2],       4(%[p1])                \n\t"
            "ulw        %[Temp3],       8(%[p1])                \n\t"
            "ulw        %[Temp4],       12(%[p1])               \n\t"
            "addiu      %[p1],          %[p1],      16          \n\t"
            "dpa.w.ph   $ac0,           %[Temp1],   %[Temp1]    \n\t"
            "dpa.w.ph   $ac0,           %[Temp2],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac0,           %[Temp3],   %[Temp3]    \n\t"
            "bne        %[p1],          %[ptr_end], 1b          \n\t"
            " dpa.w.ph  $ac0,           %[Temp4],   %[Temp4]    \n\t"
            "shilo      $ac0,           -1                      \n\t"
            "mflo       %[L_sum],       $ac0                    \n\t"
            "addiu      %[L_sum],       %[L_sum],   1           \n\t"
            "sra        %[norm],        %[L_sum],   31          \n\t"
            "xor        %[norm],        %[norm],    %[L_sum]    \n\t"
            "clz        %[norm],        %[norm]                 \n\t"
            "addiu      %[norm],        %[norm],    -1          \n\t"
            "sllv       %[L_sum],       %[L_sum],   %[norm]     \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [L_sum] "=&r" (L_sum), [norm]  "=&r" (norm),
              [p1]    "+r"  (p1)
            : [shift] "r" (shift), [ptr_end] "r" (ptr_end)
            : "hi", "lo", "memory"
        );
    }
#elif defined(MIPS_DSP_R1_LE)
    if(shift > 0)
    {
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult       $ac0,           $0,         $0          \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],       0(%[p1])                \n\t"
            "lh         %[Temp2],       2(%[p1])                \n\t"
            "lh         %[Temp3],       4(%[p1])                \n\t"
            "lh         %[Temp4],       6(%[p1])                \n\t"
            "lh         %[Temp5],       8(%[p1])                \n\t"
            "lh         %[Temp6],       10(%[p1])               \n\t"
            "lh         %[Temp7],       12(%[p1])               \n\t"
            "lh         %[Temp8],       14(%[p1])               \n\t"
            "shrav_r.w  %[Temp1],       %[Temp1],   %[shift]    \n\t"
            "shrav_r.w  %[Temp2],       %[Temp2],   %[shift]    \n\t"
            "shrav_r.w  %[Temp3],       %[Temp3],   %[shift]    \n\t"
            "shrav_r.w  %[Temp4],       %[Temp4],   %[shift]    \n\t"
            "shrav_r.w  %[Temp5],       %[Temp5],   %[shift]    \n\t"
            "shrav_r.w  %[Temp6],       %[Temp6],   %[shift]    \n\t"
            "shrav_r.w  %[Temp7],       %[Temp7],   %[shift]    \n\t"
            "shrav_r.w  %[Temp8],       %[Temp8],   %[shift]    \n\t"
            "madd       $ac0,           %[Temp1],   %[Temp1]    \n\t"
            "madd       $ac0,           %[Temp2],   %[Temp2]    \n\t"
            "madd       $ac0,           %[Temp3],   %[Temp3]    \n\t"
            "madd       $ac0,           %[Temp4],   %[Temp4]    \n\t"
            "madd       $ac0,           %[Temp5],   %[Temp5]    \n\t"
            "sh         %[Temp1],       0(%[p1])                \n\t"
            "sh         %[Temp2],       2(%[p1])                \n\t"
            "sh         %[Temp3],       4(%[p1])                \n\t"
            "sh         %[Temp4],       6(%[p1])                \n\t"
            "sh         %[Temp5],       8(%[p1])                \n\t"
            "sh         %[Temp6],       10(%[p1])               \n\t"
            "sh         %[Temp7],       12(%[p1])               \n\t"
            "sh         %[Temp8],       14(%[p1])               \n\t"
            "addiu      %[p1],          %[p1],      16          \n\t"
            "madd       $ac0,           %[Temp6],   %[Temp6]    \n\t"
            "madd       $ac0,           %[Temp7],   %[Temp7]    \n\t"
            "bne        %[p1],          %[ptr_end], 1b          \n\t"
            " madd      $ac0,           %[Temp8],   %[Temp8]    \n\t"
            "shilo      $ac0,           -1                      \n\t"
            "mflo       %[L_sum],       $ac0                    \n\t"
            "addiu      %[L_sum],       %[L_sum],   1           \n\t"
            "sra        %[norm],        %[L_sum],   31          \n\t"
            "xor        %[norm],        %[norm],    %[L_sum]    \n\t"
            "clz        %[norm],        %[norm]                 \n\t"
            "addiu      %[norm],        %[norm],    -1          \n\t"
            "sllv       %[L_sum],       %[L_sum],   %[norm]     \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [L_sum] "=&r" (L_sum), [norm]  "=&r" (norm),
              [p1]    "+r"  (p1)
            : [ptr_end] "r" (ptr_end), [shift] "r" (shift)
            : "hi", "lo", "memory"
        );
    } else {
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult       $ac0,           $0,         $0          \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],       0(%[p1])                \n\t"
            "lh         %[Temp2],       2(%[p1])                \n\t"
            "lh         %[Temp3],       4(%[p1])                \n\t"
            "lh         %[Temp4],       6(%[p1])                \n\t"
            "lh         %[Temp5],       8(%[p1])                \n\t"
            "lh         %[Temp6],       10(%[p1])               \n\t"
            "lh         %[Temp7],       12(%[p1])               \n\t"
            "lh         %[Temp8],       14(%[p1])               \n\t"
            "madd       $ac0,           %[Temp1],   %[Temp1]    \n\t"
            "madd       $ac0,           %[Temp2],   %[Temp2]    \n\t"
            "madd       $ac0,           %[Temp3],   %[Temp3]    \n\t"
            "madd       $ac0,           %[Temp4],   %[Temp4]    \n\t"
            "madd       $ac0,           %[Temp5],   %[Temp5]    \n\t"
            "sh         %[Temp1],       0(%[p1])                \n\t"
            "sh         %[Temp2],       2(%[p1])                \n\t"
            "sh         %[Temp3],       4(%[p1])                \n\t"
            "sh         %[Temp4],       6(%[p1])                \n\t"
            "sh         %[Temp5],       8(%[p1])                \n\t"
            "sh         %[Temp6],       10(%[p1])               \n\t"
            "sh         %[Temp7],       12(%[p1])               \n\t"
            "sh         %[Temp8],       14(%[p1])               \n\t"
            "addiu      %[p1],          %[p1],      16          \n\t"
            "madd       $ac0,           %[Temp6],   %[Temp6]    \n\t"
            "madd       $ac0,           %[Temp7],   %[Temp7]    \n\t"
            "bne        %[p1],          %[ptr_end], 1b          \n\t"
            " madd      $ac0,           %[Temp8],   %[Temp8]    \n\t"
            "shilo      $ac0,           -1                      \n\t"
            "mflo       %[L_sum],       $ac0                    \n\t"
            "addiu      %[L_sum],       %[L_sum],   1           \n\t"
            "sra        %[norm],        %[L_sum],   31          \n\t"
            "xor        %[norm],        %[norm],    %[L_sum]    \n\t"
            "clz        %[norm],        %[norm]                 \n\t"
            "addiu      %[norm],        %[norm],    -1          \n\t"
            "sllv       %[L_sum],       %[L_sum],   %[norm]     \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [L_sum] "=&r" (L_sum), [norm]  "=&r" (norm),
              [p1]    "+r"  (p1)
            : [ptr_end] "r" (ptr_end), [shift] "r" (shift)
            : "hi", "lo", "memory"
        );
    }
#else
    if(shift > 0)
    {
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "li         %[Temp5],       1                       \n\t"
            "addiu      %[Temp6],       %[shift],   -1          \n\t"
            "sll        %[Temp5],       %[Temp5],   %[Temp6]    \n\t"
            "mult       $0,             $0                      \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],       0(%[p1])                \n\t"
            "lh         %[Temp2],       2(%[p1])                \n\t"
            "lh         %[Temp3],       4(%[p1])                \n\t"
            "lh         %[Temp4],       6(%[p1])                \n\t"
            "addu       %[Temp1],       %[Temp1],   %[Temp5]    \n\t"
            "addu       %[Temp2],       %[Temp2],   %[Temp5]    \n\t"
            "addu       %[Temp3],       %[Temp3],   %[Temp5]    \n\t"
            "addu       %[Temp4],       %[Temp4],   %[Temp5]    \n\t"
            "sra        %[Temp1],       %[Temp1],   %[shift]    \n\t"
            "sra        %[Temp2],       %[Temp2],   %[shift]    \n\t"
            "sra        %[Temp3],       %[Temp3],   %[shift]    \n\t"
            "sra        %[Temp4],       %[Temp4],   %[shift]    \n\t"
            "madd       %[Temp1],       %[Temp1]                \n\t"
            "madd       %[Temp2],       %[Temp2]                \n\t"
            "madd       %[Temp3],       %[Temp3]                \n\t"
            "sh         %[Temp1],       0(%[p1])                \n\t"
            "sh         %[Temp2],       2(%[p1])                \n\t"
            "sh         %[Temp3],       4(%[p1])                \n\t"
            "sh         %[Temp4],       6(%[p1])                \n\t"
            "addiu      %[p1],          %[p1],      8           \n\t"
            "bne        %[p1],          %[ptr_end], 1b          \n\t"
            " madd      %[Temp4],       %[Temp4]                \n\t"
            "mflo       %[L_sum]                                \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [p1]    "+r"  (p1),    [L_sum] "=r"  (L_sum)
            : [shift] "r" (shift), [ptr_end] "r" (ptr_end)
            : "hi", "lo", "memory"
        );
    } else {
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult       $0,         $0                          \n\t"
          "1:                                                   \n\t"
            "lh         %[Temp1],   0(%[p1])                    \n\t"
            "lh         %[Temp2],   2(%[p1])                    \n\t"
            "lh         %[Temp3],   4(%[p1])                    \n\t"
            "lh         %[Temp4],   6(%[p1])                    \n\t"
            "madd       %[Temp1],   %[Temp1]                    \n\t"
            "madd       %[Temp2],   %[Temp2]                    \n\t"
            "madd       %[Temp3],   %[Temp3]                    \n\t"
            "madd       %[Temp4],   %[Temp4]                    \n\t"
            "lh         %[Temp1],   8(%[p1])                    \n\t"
            "lh         %[Temp2],   10(%[p1])                   \n\t"
            "lh         %[Temp3],   12(%[p1])                   \n\t"
            "lh         %[Temp4],   14(%[p1])                   \n\t"
            "addiu      %[p1],      %[p1],          16          \n\t"
            "madd       %[Temp1],   %[Temp1]                    \n\t"
            "madd       %[Temp2],   %[Temp2]                    \n\t"
            "madd       %[Temp3],   %[Temp3]                    \n\t"
            "bne        %[p1],      %[ptr_end],     1b          \n\t"
            " madd      %[Temp4],   %[Temp4]                    \n\t"
            "mflo       %[L_sum]                                \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [p1]    "+r"  (p1),    [L_sum] "=r"  (L_sum)
            : [ptr_end] "r" (ptr_end)
            : "hi", "lo", "memory"
        );
    }

    L_sum <<= 1;
    L_sum++;
    norm = norm_l(L_sum);
    L_sum = (L_sum << norm);
#endif

    r_h[0] = L_sum >> 16;
    r_l[0] = (L_sum & 0xffff)>>1;

    for (i = 1; i <= 8; i++)
    {
        F_LEN = (Word32)(L_WINDOW - 2*i);
        p1 = y;
        p2 = y + (2*i)-1;
        ptr_end = p1 + F_LEN - (F_LEN & 2);
#if defined(MIPS_DSP_R2_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $ac0,       $0,         $0          \n\t"
            "mult       $ac1,       $0,         $0          \n\t"
            "andi       %[Temp6],   %[F_LEN],   2           \n\t"
          "1:                                               \n\t"
            "ulw        %[Temp1],   0(%[p1])                \n\t"
            "ulw        %[Temp2],   0(%[p2])                \n\t"
            "ulw        %[Temp3],   4(%[p1])                \n\t"
            "ulw        %[Temp4],   4(%[p2])                \n\t"
            "ulw        %[Temp5],   8(%[p2])                \n\t"
            "addiu      %[p1],      %[p1],      8           \n\t"
            "dpa.w.ph   $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "packrl.ph  %[Temp2],   %[Temp4],   %[Temp2]    \n\t"
            "packrl.ph  %[Temp4],   %[Temp5],   %[Temp4]    \n\t"
            "dpa.w.ph   $ac1,       %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac1,       %[Temp3],   %[Temp4]    \n\t"
            "bne        %[p1],      %[ptr_end], 1b          \n\t"
            " addiu     %[p2],      %[p2],      8           \n\t"
            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp2],   0(%[p2])                \n\t"
            "andi       %[Temp3],   %[F_LEN],   2           \n\t"
            "beqz       %[Temp3],   2f                      \n\t"
            " madd      $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "ulw        %[Temp1],   2(%[p2])                \n\t"
            "ulw        %[Temp3],   2(%[p1])                \n\t"
            "ulw        %[Temp2],   0(%[p1])                \n\t"
            "addiu      %[p1],      %[p1],      4           \n\t"
            "addiu      %[p2],      %[p2],      4           \n\t"
            "dpa.w.ph   $ac0,       %[Temp1],   %[Temp3]    \n\t"
            "dpa.w.ph   $ac1,       %[Temp1],   %[Temp2]    \n\t"
          "2:                                               \n\t"
            "mflo       %[L_sum1],  $ac0                    \n\t"
            "mflo       %[L_sum],   $ac1                    \n\t"
            "sllv       %[L_sum1],  %[L_sum1],  %[norm]     \n\t"
            "sllv       %[L_sum],   %[L_sum],   %[norm]     \n\t"

            ".set pop                                       \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2]  "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4]  "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6]  "=&r" (Temp6),
              [L_sum] "=&r" (L_sum), [L_sum1] "=&r" (L_sum1),
              [p1]    "+r"  (p1),    [p2]     "+r"  (p2)
            : [ptr_end] "r" (ptr_end), [F_LEN] "r" (F_LEN),
              [norm]    "r" (norm)
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "memory"
        );
#elif defined(MIPS_DSP_R1_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $ac0,       $0,         $0          \n\t"
            "mult       $ac1,       $0,         $0          \n\t"
          "1:                                               \n\t"
            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp2],   2(%[p1])                \n\t"
            "lh         %[Temp3],   4(%[p1])                \n\t"
            "lh         %[Temp4],   6(%[p1])                \n\t"
            "lh         %[Temp5],   0(%[p2])                \n\t"
            "lh         %[Temp6],   2(%[p2])                \n\t"
            "lh         %[Temp7],   4(%[p2])                \n\t"
            "lh         %[Temp8],   6(%[p2])                \n\t"
            "lh         %[Temp9],   8(%[p2])                \n\t"
            "addiu      %[p1],      %[p1],      8           \n\t"
            "madd       $ac0,       %[Temp1],   %[Temp5]    \n\t"
            "madd       $ac0,       %[Temp2],   %[Temp6]    \n\t"
            "madd       $ac0,       %[Temp3],   %[Temp7]    \n\t"
            "madd       $ac0,       %[Temp4],   %[Temp8]    \n\t"
            "madd       $ac1,       %[Temp1],   %[Temp6]    \n\t"
            "madd       $ac1,       %[Temp2],   %[Temp7]    \n\t"
            "madd       $ac1,       %[Temp3],   %[Temp8]    \n\t"
            "madd       $ac1,       %[Temp4],   %[Temp9]    \n\t"
            "bne        %[p1],      %[ptr_end], 1b          \n\t"
            " addiu     %[p2],      %[p2],      8           \n\t"
            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp2],   0(%[p2])                \n\t"
            "andi       %[Temp3],   %[F_LEN],   2           \n\t"
            "beqz       %[Temp3],   2f                      \n\t"
            " madd      $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp4],   2(%[p2])                \n\t"
            "lh         %[Temp2],   2(%[p1])                \n\t"
            "lh         %[Temp5],   4(%[p2])                \n\t"
            "lh         %[Temp3],   4(%[p1])                \n\t"
            "madd       $ac1,       %[Temp1],   %[Temp4]    \n\t"
            "madd       $ac1,       %[Temp2],   %[Temp5]    \n\t"
            "madd       $ac0,       %[Temp2],   %[Temp4]    \n\t"
            "madd       $ac0,       %[Temp3],   %[Temp5]    \n\t"
          "2:                                               \n\t"
            "mflo       %[L_sum1],  $ac0                    \n\t"
            "mflo       %[L_sum],   $ac1                    \n\t"
            "sllv       %[L_sum1],  %[L_sum1],  %[norm]     \n\t"
            "sllv       %[L_sum],   %[L_sum],   %[norm]     \n\t"

            ".set pop                                       \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2] "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4] "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6] "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8] "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [L_sum] "=&r" (L_sum),
              [L_sum1] "=&r" (L_sum1), [p1]    "+r"  (p1),
              [p2]     "+r"  (p2)
            : [ptr_end] "r" (ptr_end), [F_LEN] "r" (F_LEN),
              [norm]    "r" (norm)
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "memory"
        );
#else
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $0,         $0                      \n\t"
            "li         %[L_sum],   0                       \n\t"
          "1:                                               \n\t"
            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp2],   0(%[p2])                \n\t"
            "lh         %[Temp3],   2(%[p1])                \n\t"
            "lh         %[Temp4],   2(%[p2])                \n\t"
            "lh         %[Temp5],   4(%[p1])                \n\t"
            "lh         %[Temp6],   4(%[p2])                \n\t"
            "lh         %[Temp7],   6(%[p1])                \n\t"
            "lh         %[Temp8],   6(%[p2])                \n\t"
            "lh         %[Temp9],   8(%[p2])                \n\t"
            "mul        %[Temp10],  %[Temp1],   %[Temp4]    \n\t"
            "mul        %[Temp11],  %[Temp3],   %[Temp6]    \n\t"
            "mul        %[Temp12],  %[Temp5],   %[Temp8]    \n\t"
            "mul        %[Temp13],  %[Temp7],   %[Temp9]    \n\t"
            "madd       %[Temp1],   %[Temp2]                \n\t"
            "madd       %[Temp3],   %[Temp4]                \n\t"
            "madd       %[Temp5],   %[Temp6]                \n\t"
            "madd       %[Temp7],   %[Temp8]                \n\t"
            "addiu      %[p1],      %[p1],      8           \n\t"
            "addiu      %[p2],      %[p2],      8           \n\t"
            "addu       %[L_sum],   %[L_sum],   %[Temp10]   \n\t"
            "addu       %[L_sum],   %[L_sum],   %[Temp11]   \n\t"
            "addu       %[L_sum],   %[L_sum],   %[Temp12]   \n\t"
            "bne        %[p1],      %[ptr_end], 1b          \n\t"
            " addu      %[L_sum],   %[L_sum],   %[Temp13]   \n\t"
            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp2],   0(%[p2])                \n\t"
            "andi       %[Temp3],   %[F_LEN],   2           \n\t"
            "beqz       %[Temp3],   2f                      \n\t"
            " madd      %[Temp1],   %[Temp2]                \n\t"
            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp4],   2(%[p2])                \n\t"
            "lh         %[Temp2],   2(%[p1])                \n\t"
            "lh         %[Temp5],   4(%[p2])                \n\t"
            "lh         %[Temp3],   4(%[p1])                \n\t"
            "mul        %[Temp6],   %[Temp1],   %[Temp4]    \n\t"
            "mul        %[Temp7],   %[Temp2],   %[Temp5]    \n\t"
            "madd       %[Temp2],   %[Temp4]                \n\t"
            "madd       %[Temp3],   %[Temp5]                \n\t"
            "addu       %[L_sum],   %[L_sum],   %[Temp6]    \n\t"
            "addu       %[L_sum],   %[L_sum],   %[Temp7]    \n\t"
          "2:                                               \n\t"
            "mflo       %[L_sum1]                           \n\t"

            ".set pop                                       \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
              [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
              [Temp13] "=&r" (Temp13), [L_sum]  "=&r" (L_sum),
              [L_sum1] "=&r" (L_sum1), [p1]     "+r"  (p1),
              [p2]     "+r"  (p2)
            : [ptr_end] "r" (ptr_end), [F_LEN] "r" (F_LEN)
            : "hi", "lo", "memory"
        );

        L_sum1 = L_sum1<< norm;
        L_sum  = L_sum << norm;
#endif

        r_h[(2*i)-1] = L_sum1 >> 15;
        r_l[(2*i)-1] = L_sum1 & 0x00007fff;
        r_h[(2*i)] = L_sum >> 15;
        r_l[(2*i)] = L_sum & 0x00007fff;
    }
    return;
}



