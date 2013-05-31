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
*      File: p_med_ol_mips.c                                           *
*                                                                      *
*      Description: Compute the open loop pitch lag                    *
*               output: open loop pitch lag                            *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "acelp.h"
#include "oper_32b.h"
#include "math_op.h"
#include "p_med_ol.tab"

Word16 Pitch_med_ol_mips(
        Word16      wsp[],        /*   i: signal used to compute the open loop pitch*/
                                     /*      wsp[-pit_max] to wsp[-1] should be known */
        Coder_State *st,          /* i/o: codec global structure */
        Word16      L_frame       /*   i: length of frame to compute pitch */
        )
{
    Word16 Tm;
    Word16 hi, lo;
    Word16 *ww, *we, *hp_wsp;
    Word16 exp_R0, exp_R1, exp_R2;
    Word32 i, max, R0, R1, R2;
    Word16 *p1, *p2;
    Word16 L_min = 17;                   /* minimum pitch lag: PIT_MIN / OPL_DECIM */
    Word16 L_max = 115;                  /* maximum pitch lag: PIT_MAX / OPL_DECIM */
    Word16 L_0 = st->old_T0_med;         /* old open-loop pitch */
    Word16 *gain = &(st->ol_gain);       /* normalize correlation of hp_wsp for the lag */
    Word16 *hp_wsp_mem = st->hp_wsp_mem; /* memory of the hypass filter for hp_wsp[] (lg = 9)*/
    Word16 *old_hp_wsp = st->old_hp_wsp; /* hypass wsp[] */
    Word16 wght_flg = st->ol_wght_flg;   /* is weighting function used */
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6;
    Word16 *p1_end;
#if !defined(MIPS_DSP_R2_LE)
    Word32 Temp7, Temp8;
#endif /* !defined(MIPS_DSP_R2_LE) */
#if !defined(MIPS_DSP_R1_LE)
    Word32 Temp9, Temp10, Temp11, Temp12;
#endif /* !defined(MIPS_DSP_R1_LE) */

    ww = &corrweight[198];
    we = &corrweight[98 + L_max - L_0];

    max = MIN_32;
    Tm = 0;
    for (i = L_max; i > L_min; i--)
    {
        p1 = wsp;
        p2 = &wsp[-i];
        p1_end = p1 + L_frame;
#if defined(MIPS_DSP_R2_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $ac0,       $0,         $0          \n\t"
          "1:                                               \n\t"
            "ulw        %[Temp1],   0(%[p1])                \n\t"
            "ulw        %[Temp2],   0(%[p2])                \n\t"
            "ulw        %[Temp3],   4(%[p1])                \n\t"
            "ulw        %[Temp4],   4(%[p2])                \n\t"
            "addiu      %[p1],      %[p1],      8           \n\t"
            "dpa.w.ph   $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph   $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "bne        %[p1],      %[p1_end],  1b          \n\t"
            " addiu     %[p2],      %[p2],      8           \n\t"
            "shilo      $ac0,       -1                      \n\t"
            "mflo       %[R0],      $ac0                    \n\t"

            ".set pop                                       \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [R0]    "=r"  (R0),    [p1]    "+r"  (p1),
              [p2]    "+r"  (p2)
            : [p1_end] "r" (p1_end)
            : "hi", "lo", "memory"
        );
#elif defined(MIPS_DSP_R1_LE)
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $ac0,       $0,         $0          \n\t"
          "1:                                               \n\t"
            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp2],   2(%[p1])                \n\t"
            "lh         %[Temp3],   4(%[p1])                \n\t"
            "lh         %[Temp4],   6(%[p1])                \n\t"
            "lh         %[Temp5],   0(%[p2])                \n\t"
            "lh         %[Temp6],   2(%[p2])                \n\t"
            "lh         %[Temp7],   4(%[p2])                \n\t"
            "lh         %[Temp8],   6(%[p2])                \n\t"
            "addiu      %[p1],      %[p1],      8           \n\t"
            "madd       $ac0,       %[Temp1],   %[Temp5]    \n\t"
            "madd       $ac0,       %[Temp2],   %[Temp6]    \n\t"
            "madd       $ac0,       %[Temp3],   %[Temp7]    \n\t"
            "madd       $ac0,       %[Temp4],   %[Temp8]    \n\t"
            "bne        %[p1],      %[p1_end],  1b          \n\t"
            " addiu     %[p2],      %[p2],      8           \n\t"
            "shilo      $ac0,       -1                      \n\t"
            "mflo       %[R0],      $ac0                    \n\t"

            ".set pop                                       \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [R0]    "=&r" (R0),    [p1]    "+r"  (p1),
              [p2]    "+r"  (p2)
            : [p1_end] "r" (p1_end), [ww] "r" (ww)
            : "hi", "lo", "memory"
        );
#else
        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"

            "mult       $0,         $0                      \n\t"
          "1:                                               \n\t"
            "lh         %[Temp1],   0(%[p1])                \n\t"
            "lh         %[Temp2],   2(%[p1])                \n\t"
            "lh         %[Temp3],   4(%[p1])                \n\t"
            "lh         %[Temp4],   6(%[p1])                \n\t"
            "lh         %[Temp5],   0(%[p2])                \n\t"
            "lh         %[Temp6],   2(%[p2])                \n\t"
            "lh         %[Temp7],   4(%[p2])                \n\t"
            "lh         %[Temp8],   6(%[p2])                \n\t"
            "addiu      %[p1],      %[p1],      8           \n\t"
            "addiu      %[p2],      %[p2],      8           \n\t"
            "madd       %[Temp1],   %[Temp5]                \n\t"
            "madd       %[Temp2],   %[Temp6]                \n\t"
            "madd       %[Temp3],   %[Temp7]                \n\t"
            "bne        %[p1],      %[p1_end],  1b          \n\t"
            " madd      %[Temp4],   %[Temp8]                \n\t"
            "mflo       %[R0]                               \n\t"
            "sll        %[R0],      %[R0],      1           \n\t"

            ".set pop                                       \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [R0]    "=&r" (R0),    [p1]    "+r"  (p1),
              [p2]    "+r"  (p2)
            : [p1_end] "r" (p1_end)
            : "hi", "lo", "memory"
        );
#endif /* defined(MIPS_DSP_R2_LE) */
        /* Weighting of the correlation function.   */
        hi = R0>>16;
        lo = (R0 & 0xffff)>>1;

        R0 = Mpy_32_16(hi, lo, *ww);
        ww--;

        if ((L_0 > 0) && (wght_flg > 0))
        {
            /* Weight the neighbourhood of the old lag. */
            hi = R0>>16;
            lo = (R0 & 0xffff)>>1;
            R0 = Mpy_32_16(hi, lo, *we);
            we--;
        }
        if(R0 >= max)
        {
            max = R0;
            Tm = i;
        }
    }

    /* Hypass the wsp[] vector */
    hp_wsp = old_hp_wsp + L_max;
    Hp_wsp(wsp, hp_wsp, L_frame, hp_wsp_mem);

    /* Compute normalize correlation at delay Tm */
    p1 = hp_wsp;
    p2 = hp_wsp - Tm;
    p1_end = p1 + L_frame;
#if defined(MIPS_DSP_R2_LE)
    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "mult       $ac0,       $0,         $0          \n\t"
        "mult       $ac1,       $0,         $0          \n\t"
        "mult       $ac2,       $0,         $0          \n\t"
      "1:                                               \n\t"
        "ulw        %[Temp1],   0(%[p1])                \n\t"
        "ulw        %[Temp2],   0(%[p2])                \n\t"
        "ulw        %[Temp3],   4(%[p1])                \n\t"
        "ulw        %[Temp4],   4(%[p2])                \n\t"
        "ulw        %[Temp5],   8(%[p1])                \n\t"
        "ulw        %[Temp6],   8(%[p2])                \n\t"
        "addiu      %[p1],      %[p1],      8           \n\t"
        "dpa.w.ph   $ac0,       %[Temp1],   %[Temp2]    \n\t"
        "dpa.w.ph   $ac0,       %[Temp3],   %[Temp4]    \n\t"
        "dpa.w.ph   $ac1,       %[Temp2],   %[Temp2]    \n\t"
        "dpa.w.ph   $ac1,       %[Temp4],   %[Temp4]    \n\t"
        "dpa.w.ph   $ac2,       %[Temp1],   %[Temp1]    \n\t"
        "dpa.w.ph   $ac2,       %[Temp3],   %[Temp3]    \n\t"
        "bne        %[p1],      %[p1_end],  1b          \n\t"
        " addiu     %[p2],      %[p2],      8           \n\t"
        "shilo      $ac0,       -1                      \n\t"
        "shilo      $ac1,       -2                      \n\t"
        "shilo      $ac2,       -2                      \n\t"
        "mflo       %[R0],      $ac0                    \n\t"
        "extr_r.w   %[R1],      $ac1,       1           \n\t"
        "extr_r.w   %[R2],      $ac2,       1           \n\t"
        "sra        %[exp_R0],  %[R0],      31          \n\t"
        "sra        %[exp_R1],  %[R1],      31          \n\t"
        "sra        %[exp_R2],  %[R2],      31          \n\t"
        "xor        %[exp_R0],  %[exp_R0],  %[R0]       \n\t"
        "xor        %[exp_R1],  %[exp_R1],  %[R1]       \n\t"
        "xor        %[exp_R2],  %[exp_R2],  %[R2]       \n\t"
        "clz        %[exp_R0],  %[exp_R0]               \n\t"
        "clz        %[exp_R1],  %[exp_R1]               \n\t"
        "clz        %[exp_R2],  %[exp_R2]               \n\t"
        "addiu      %[exp_R0],  %[exp_R0],  -1          \n\t"
        "addiu      %[exp_R1],  %[exp_R1],  -1          \n\t"
        "addiu      %[exp_R2],  %[exp_R2],  -1          \n\t"
        "sllv       %[R0],      %[R0],      %[exp_R0]   \n\t"
        "sllv       %[R1],      %[R1],      %[exp_R1]   \n\t"
        "sllv       %[R2],      %[R2],      %[exp_R2]   \n\t"

        ".set pop                                       \n\t"

        : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
          [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
          [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
          [exp_R0] "=&r" (exp_R0), [exp_R1] "=&r" (exp_R1),
          [exp_R2] "=&r" (exp_R2), [R0]     "=&r" (R0),
          [R1]     "=&r" (R1),     [R2]     "=&r" (R2),
          [p1]     "+r"  (p1),     [p2]     "+r"  (p2)
        : [p1_end] "r" (p1_end)
        : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi",
          "$ac2lo", "memory"
    );
#elif defined(MIPS_DSP_R1_LE)
    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "mult       $ac0,       $0,         $0          \n\t"
        "mult       $ac1,       $0,         $0          \n\t"
        "mult       $ac2,       $0,         $0          \n\t"
      "1:                                               \n\t"
        "lh         %[Temp1],   0(%[p1])                \n\t"
        "lh         %[Temp2],   2(%[p1])                \n\t"
        "lh         %[Temp3],   4(%[p1])                \n\t"
        "lh         %[Temp4],   6(%[p1])                \n\t"
        "lh         %[Temp5],   0(%[p2])                \n\t"
        "lh         %[Temp6],   2(%[p2])                \n\t"
        "lh         %[Temp7],   4(%[p2])                \n\t"
        "lh         %[Temp8],   6(%[p2])                \n\t"
        "addiu      %[p1],      %[p1],      8           \n\t"
        "madd       $ac0,       %[Temp1],   %[Temp5]    \n\t"
        "madd       $ac0,       %[Temp2],   %[Temp6]    \n\t"
        "madd       $ac0,       %[Temp3],   %[Temp7]    \n\t"
        "madd       $ac0,       %[Temp4],   %[Temp8]    \n\t"
        "madd       $ac1,       %[Temp5],   %[Temp5]    \n\t"
        "madd       $ac1,       %[Temp6],   %[Temp6]    \n\t"
        "madd       $ac1,       %[Temp7],   %[Temp7]    \n\t"
        "madd       $ac1,       %[Temp8],   %[Temp8]    \n\t"
        "madd       $ac2,       %[Temp1],   %[Temp1]    \n\t"
        "madd       $ac2,       %[Temp2],   %[Temp2]    \n\t"
        "madd       $ac2,       %[Temp3],   %[Temp3]    \n\t"
        "madd       $ac2,       %[Temp4],   %[Temp4]    \n\t"
        "bne        %[p1],      %[p1_end],  1b          \n\t"
        " addiu     %[p2],      %[p2],      8           \n\t"
        "shilo      $ac0,       -1                      \n\t"
        "shilo      $ac1,       -1                      \n\t"
        "shilo      $ac2,       -1                      \n\t"
        "mflo       %[R0],      $ac0                    \n\t"
        "extr_rs.w  %[R1],      $ac1,       0           \n\t"
        "extr_rs.w  %[R2],      $ac2,       0           \n\t"
        "sra        %[exp_R0],  %[R0],  31              \n\t"
        "sra        %[exp_R1],  %[R1],  31              \n\t"
        "sra        %[exp_R2],  %[R2],  31              \n\t"
        "xor        %[exp_R0],  %[exp_R0],  %[R0]       \n\t"
        "xor        %[exp_R1],  %[exp_R1],  %[R1]       \n\t"
        "xor        %[exp_R2],  %[exp_R2],  %[R2]       \n\t"
        "clz        %[exp_R0],  %[exp_R0]               \n\t"
        "clz        %[exp_R1],  %[exp_R1]               \n\t"
        "clz        %[exp_R2],  %[exp_R2]               \n\t"
        "addiu      %[exp_R0],  %[exp_R0],  -1          \n\t"
        "addiu      %[exp_R1],  %[exp_R1],  -1          \n\t"
        "addiu      %[exp_R2],  %[exp_R2],  -1          \n\t"
        "sllv       %[R0],      %[R0],      %[exp_R0]   \n\t"
        "sllv       %[R1],      %[R1],      %[exp_R1]   \n\t"
        "sllv       %[R2],      %[R2],      %[exp_R2]   \n\t"

        ".set pop                                       \n\t"

        : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
          [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
          [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
          [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
          [exp_R0] "=&r" (exp_R0), [exp_R1] "=&r" (exp_R1),
          [exp_R2] "=&r" (exp_R2), [R0]     "=&r" (R0),
          [R1]     "=&r" (R1),     [R2]     "=&r" (R2),
          [p1]     "+r"  (p1),     [p2]     "+r"  (p2)
        : [p1_end] "r" (p1_end)
        : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi",
          "$ac2lo", "memory"
    );
#else
    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

        "mult       $0,         $0                      \n\t"
        "li         %[R1],      0                       \n\t"
        "li         %[R2],      0                       \n\t"
      "1:                                               \n\t"
        "lh         %[Temp1],   0(%[p1])                \n\t"
        "lh         %[Temp2],   0(%[p2])                \n\t"
        "lh         %[Temp3],   2(%[p1])                \n\t"
        "lh         %[Temp4],   2(%[p2])                \n\t"
        "mul        %[Temp5],   %[Temp1],   %[Temp1]    \n\t"
        "mul        %[Temp6],   %[Temp2],   %[Temp2]    \n\t"
        "mul        %[Temp7],   %[Temp3],   %[Temp3]    \n\t"
        "mul        %[Temp8],   %[Temp4],   %[Temp4]    \n\t"
        "madd       %[Temp1],   %[Temp2]                \n\t"
        "madd       %[Temp3],   %[Temp4]                \n\t"
        "lh         %[Temp1],   4(%[p1])                \n\t"
        "lh         %[Temp2],   4(%[p2])                \n\t"
        "lh         %[Temp3],   6(%[p1])                \n\t"
        "lh         %[Temp4],   6(%[p2])                \n\t"
        "addiu      %[p1],      %[p1],      8           \n\t"
        "addiu      %[p2],      %[p2],      8           \n\t"
        "mul        %[Temp9],   %[Temp1],   %[Temp1]    \n\t"
        "mul        %[Temp10],  %[Temp2],   %[Temp2]    \n\t"
        "mul        %[Temp11],  %[Temp3],   %[Temp3]    \n\t"
        "mul        %[Temp12],  %[Temp4],   %[Temp4]    \n\t"
        "madd       %[Temp1],   %[Temp2]                \n\t"
        "madd       %[Temp3],   %[Temp4]                \n\t"
        "addu       %[R2],      %[R2],      %[Temp5]    \n\t"
        "addu       %[R1],      %[R1],      %[Temp6]    \n\t"
        "addu       %[R2],      %[R2],      %[Temp7]    \n\t"
        "addu       %[R1],      %[R1],      %[Temp8]    \n\t"
        "addu       %[R2],      %[R2],      %[Temp9]    \n\t"
        "addu       %[R1],      %[R1],      %[Temp10]   \n\t"
        "addu       %[R2],      %[R2],      %[Temp11]   \n\t"
        "bne        %[p1],      %[p1_end],  1b          \n\t"
        " addu      %[R1],      %[R1],      %[Temp12]   \n\t"
        "mflo       %[R0]                               \n\t"
        "sll        %[R1],      %[R1],      1           \n\t"
        "sll        %[R2],      %[R2],      1           \n\t"
        "sll        %[R0],      %[R0],      1           \n\t"
        "addiu      %[R1],      %[R1],      1           \n\t"
        "addiu      %[R2],      %[R2],      1           \n\t"
        "sra        %[exp_R0],  %[R0],  31              \n\t"
        "sra        %[exp_R1],  %[R1],  31              \n\t"
        "sra        %[exp_R2],  %[R2],  31              \n\t"
        "xor        %[exp_R0],  %[exp_R0],  %[R0]       \n\t"
        "xor        %[exp_R1],  %[exp_R1],  %[R1]       \n\t"
        "xor        %[exp_R2],  %[exp_R2],  %[R2]       \n\t"
        "clz        %[exp_R0],  %[exp_R0]               \n\t"
        "clz        %[exp_R1],  %[exp_R1]               \n\t"
        "clz        %[exp_R2],  %[exp_R2]               \n\t"
        "addiu      %[exp_R0],  %[exp_R0],  -1          \n\t"
        "addiu      %[exp_R1],  %[exp_R1],  -1          \n\t"
        "addiu      %[exp_R2],  %[exp_R2],  -1          \n\t"
        "sllv       %[R0],      %[R0],      %[exp_R0]   \n\t"
        "sllv       %[R1],      %[R1],      %[exp_R1]   \n\t"
        "sllv       %[R2],      %[R2],      %[exp_R2]   \n\t"

        ".set pop                                       \n\t"

        : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
          [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
          [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
          [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
          [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
          [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
          [exp_R0] "=&r" (exp_R0), [exp_R1] "=&r" (exp_R1),
          [exp_R2] "=&r" (exp_R2), [R0]     "=&r" (R0),
          [R1]     "=&r" (R1),     [R2]     "=&r" (R2),
          [p1]     "+r"  (p1),     [p2]     "+r"  (p2)
        : [p1_end] "r" (p1_end)
        : "hi", "lo", "memory"
    );
#endif /* defined(MIPS_DSP_R2_LE) */

    R1 = vo_L_mult(vo_round(R1), vo_round(R2));

    i = norm_l(R1);
    R1 = (R1 << i);

    exp_R1 += exp_R2;
    exp_R1 += i;
    exp_R1 = 62 - exp_R1;

    Isqrt_n(&R1, &exp_R1);

    R0 = vo_L_mult(voround(R0), voround(R1));
    exp_R0 = 31 - exp_R0;
    exp_R0 += exp_R1;

    *gain = vo_round(L_shl(R0, exp_R0));

    /* Shitf hp_wsp[] for next frame */

    for (i = 0; i < L_max; i++)
    {
        old_hp_wsp[i] = old_hp_wsp[i + L_frame];
    }

    return (Tm);
}
