/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
/****************************************************************************************
Portions of this file are derived from the following 3GPP standard:

    3GPP TS 26.073
    ANSI-C code for the Adaptive Multi-Rate (AMR) speech codec
    Available from http://www.3gpp.org

(C) 2004, 3GPP Organizational Partners (ARIB, ATIS, CCSA, ETSI, TTA, TTC)
Permission to distribute, modify and use this file under the standard license
terms listed above has been obtained from the copyright holder.
****************************************************************************************/
/*
------------------------------------------------------------------------------



 Pathname: ./audio/gsm-amr/c/src/pitch_fr.c
 Functions:


     Date: 02/04/2002

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Added pOverflow as a passed in value to searchFrac and made
              other fixes to the code regarding simple syntax fixes. Removed
              the include of stio.h.

 Description: *lag-- decrements the pointer.  (*lag)-- decrements what is
 pointed to.  The latter is what the coder intended, but the former is
 the coding instruction that was used.

 Description: A common problem -- a comparison != 0 was inadvertantly replaced
 by a comparison == 0.


 Description:  For Norm_Corr() and getRange()
              1. Eliminated unused include files.
              2. Replaced array addressing by pointers
              3. Eliminated math operations that unnecessary checked for
                 saturation, in some cases this by shifting before adding and
                 in other cases by evaluating the operands
              4. Unrolled loops to speed up processing, use decrement loops
              5. Replaced extract_l() call with equivalent code
              6. Modified scaling threshold and group all shifts (avoiding
                 successive shifts)

 Description:  Replaced OSCL mem type functions and eliminated include
               files that now are chosen by OSCL definitions

 Description:  Replaced "int" and/or "char" with OSCL defined types.

 Description: Removed compiler warnings.

 Description:
------------------------------------------------------------------------------
 MODULE DESCRIPTION

      File             : pitch_fr.c
      Purpose          : Find the pitch period with 1/3 or 1/6 subsample
                       : resolution (closed loop).

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/

#include <stdlib.h>

#include "pitch_fr.h"
#include "oper_32b.h"
#include "cnst.h"
#include "enc_lag3.h"
#include "enc_lag6.h"
#include "inter_36.h"
#include "inv_sqrt.h"
#include "convolve.h"

#include "basic_op.h"


/*----------------------------------------------------------------------------
; MACROS
; Define module specific macros here
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; DEFINES
; Include all pre-processor statements here. Include conditional
; compile variables also.
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; LOCAL FUNCTION DEFINITIONS
; Function Prototype declaration
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; LOCAL VARIABLE DEFINITIONS
; Variable declaration - defined here and used outside this module
----------------------------------------------------------------------------*/

void Norm_Corr(Word16 exc[],
                      Word16 xn[],
                      Word16 h[],
                      Word16 L_subfr,
                      Word16 t_min,
                      Word16 t_max,
                      Word16 corr_norm[],
                      Flag *pOverflow)
{
    Word16 i;
    Word16 j;
    Word16 k;
    Word16 corr_h;
    Word16 corr_l;
    Word16 norm_h;
    Word16 norm_l;
    Word32 s;
    Word32 s2;
    Word16 excf[40];
    Word16 scaling;
    Word16 h_fac;
    Word16 *s_excf;
    Word16 scaled_excf[40];
    Word16 *p_s_excf,p_s_excf1;
    Word16 *p_excf;
    Word16 temp;
    Word16 *p_x,*p_x1;
    Word16 *p_h;

    k = -t_min;

    /* compute the filtered excitation for the first delay t_min */

    Convolve(&exc[k], h, excf, L_subfr);//L_subfr=40

    /* scale "excf[]" to avoid overflow */
    s = 0;
    p_s_excf = scaled_excf;
    p_excf   = excf;
    Word32 s1=0;

#if (MIPS_DSP_R2_LE)
    {
         Word32 temp1, temp2, temp3, temp4;

         __asm__ volatile (
            ".set push                                                 \n\t"
            ".set noreorder                                            \n\t"
            "li            %[j],        20                             \n\t"
            "mult          $ac1,        $zero,             $zero       \n\t"
         "1:                                                           \n\t"
            "ulw           %[temp1],    0(%[p_excf])                   \n\t"
            "ulw           %[temp2],    4(%[p_excf])                   \n\t"
            "ulw           %[temp3],    8(%[p_excf])                   \n\t"
            "ulw           %[temp4],    12(%[p_excf])                  \n\t"
            "addiu         %[p_excf],   %[p_excf],         16          \n\t"
            "dpa.w.ph      $ac1,        %[temp1],          %[temp1]    \n\t"
            "dpa.w.ph      $ac1,        %[temp2],          %[temp2]    \n\t"
            "dpa.w.ph      $ac1,        %[temp3],          %[temp3]    \n\t"
            "dpa.w.ph      $ac1,        %[temp4],          %[temp4]    \n\t"
            "addiu         %[j],        %[j],              -4          \n\t"
            "shra.ph       %[temp1],    %[temp1],          2           \n\t"
            "shra.ph       %[temp2],    %[temp2],          2           \n\t"
            "shra.ph       %[temp3],    %[temp3],          2           \n\t"
            "shra.ph       %[temp4],    %[temp4],          2           \n\t"
            "usw           %[temp1],    0(%[p_s_excf])                 \n\t"
            "usw           %[temp2],    4(%[p_s_excf])                 \n\t"
            "usw           %[temp3],    8(%[p_s_excf])                 \n\t"
            "usw           %[temp4],    12(%[p_s_excf])                \n\t"
            "bnez          %[j],        1b                             \n\t"
            " addiu        %[p_s_excf], %[p_s_excf],       16          \n\t"
            "mflo          %[s],        $ac1                           \n\t"
            ".set pop                                                  \n\t"
            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [s] "=&r" (s),[j] "=&r" (j),[p_excf] "+r" (p_excf),
              [p_s_excf] "+r" (p_s_excf)
            :
            :"memory",  "$ac1hi", "$ac1lo"
        );
    }
#elif (MIPS_DSP_R1_LE)/*(MIPS_DSP_R1_LE)*/
    {
         Word32 temp1, temp2, temp3, temp4;

         __asm__ volatile (
            ".set push                                                  \n\t"
            ".set noreorder                                             \n\t"
            "li             %[j],        20                             \n\t"
            "mult           $ac1,        $zero,             $zero       \n\t"
         "1:                                                            \n\t"
            "ulw            %[temp1],    0(%[p_excf])                   \n\t"
            "ulw            %[temp2],    4(%[p_excf])                   \n\t"
            "ulw            %[temp3],    8(%[p_excf])                   \n\t"
            "ulw            %[temp4],    12(%[p_excf])                  \n\t"
            "addiu          %[p_excf],   %[p_excf],         16          \n\t"
            "maq_s.w.phr    $ac1,        %[temp1],          %[temp1]    \n\t"
            "maq_s.w.phr    $ac1,        %[temp2],          %[temp2]    \n\t"
            "maq_s.w.phr    $ac1,        %[temp3],          %[temp3]    \n\t"
            "maq_s.w.phr    $ac1,        %[temp4],          %[temp4]    \n\t"
            "maq_s.w.phl    $ac1,        %[temp1],          %[temp1]    \n\t"
            "maq_s.w.phl    $ac1,        %[temp2],          %[temp2]    \n\t"
            "maq_s.w.phl    $ac1,        %[temp3],          %[temp3]    \n\t"
            "maq_s.w.phl    $ac1,        %[temp4],          %[temp4]    \n\t"
            "addiu          %[j],        %[j],              -4          \n\t"
            "shra.ph        %[temp1],    %[temp1],          2           \n\t"
            "shra.ph        %[temp2],    %[temp2],          2           \n\t"
            "shra.ph        %[temp3],    %[temp3],          2           \n\t"
            "shra.ph        %[temp4],    %[temp4],          2           \n\t"
            "usw            %[temp1],    0(%[p_s_excf])                 \n\t"
            "usw            %[temp2],    4(%[p_s_excf])                 \n\t"
            "usw            %[temp3],    8(%[p_s_excf])                 \n\t"
            "usw            %[temp4],    12(%[p_s_excf])                \n\t"
            "bnez           %[j],        1b                             \n\t"
            " addiu         %[p_s_excf], %[p_s_excf],       16          \n\t"
            "extr.w         %[s],        $ac1,              1           \n\t"
            ".set pop                                                   \n\t"
            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [s] "=&r" (s), [j] "=&r" (j), [p_excf] "+r" (p_excf),
              [p_s_excf] "+r" (p_s_excf)
            :
            :"memory",  "$ac1hi", "$ac1lo"
        );
    }
#elif (MIPS32_R2_LE)
    {
         Word32 temp1, temp2, temp3, temp4;
         Word32 temp5, temp6, temp7, temp8;

         __asm__ volatile (
            ".set push                                                 \n\t"
            ".set noreorder                                            \n\t"
            "li            %[j],       20                              \n\t"
            "mult          $zero,      $zero                           \n\t"
         "1:                                                           \n\t"
            "lh            %[temp1],   0(%[p_excf])                    \n\t"
            "lh            %[temp2],   2(%[p_excf])                    \n\t"
            "lh            %[temp3],   4(%[p_excf])                    \n\t"
            "lh            %[temp4],   6(%[p_excf])                    \n\t"
            "lh            %[temp5],   8(%[p_excf])                    \n\t"
            "lh            %[temp6],   10(%[p_excf])                   \n\t"
            "lh            %[temp7],   12(%[p_excf])                   \n\t"
            "lh            %[temp8],   14(%[p_excf])                   \n\t"
            "addiu         %[p_excf],  %[p_excf],         16           \n\t"
            "madd          %[temp1],   %[temp1]                        \n\t"
            "madd          %[temp2],   %[temp2]                        \n\t"
            "madd          %[temp3],   %[temp3]                        \n\t"
            "madd          %[temp4],   %[temp4]                        \n\t"
            "madd          %[temp5],   %[temp5]                        \n\t"
            "madd          %[temp6],   %[temp6]                        \n\t"
            "madd          %[temp7],   %[temp7]                        \n\t"
            "madd          %[temp8],   %[temp8]                        \n\t"
            "addiu         %[j],       %[j],              -4           \n\t"
            "sra           %[temp1],   %[temp1],          2            \n\t"
            "sra           %[temp2],   %[temp2],          2            \n\t"
            "sra           %[temp3],   %[temp3],          2            \n\t"
            "sra           %[temp4],   %[temp4],          2            \n\t"
            "sra           %[temp5],   %[temp5],          2            \n\t"
            "sra           %[temp6],   %[temp6],          2            \n\t"
            "sra           %[temp7],   %[temp7],          2            \n\t"
            "sra           %[temp8],   %[temp8],          2            \n\t"
            "sh            %[temp1],   0(%[p_s_excf])                  \n\t"
            "sh            %[temp2],   2(%[p_s_excf])                  \n\t"
            "sh            %[temp3],   4(%[p_s_excf])                  \n\t"
            "sh            %[temp4],   6(%[p_s_excf])                  \n\t"
            "sh            %[temp5],   8(%[p_s_excf])                  \n\t"
            "sh            %[temp6],   10(%[p_s_excf])                 \n\t"
            "sh            %[temp7],   12(%[p_s_excf])                 \n\t"
            "sh            %[temp8],   14(%[p_s_excf])                 \n\t"
            "bnez          %[j],       1b                              \n\t"
            " addiu        %[p_s_excf],%[p_s_excf],       16           \n\t"
            "mflo          %[s]                                        \n\t"
            ".set pop                                                  \n\t"
            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
              [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
              [s] "=&r" (s),[j] "=&r" (j),[p_excf] "+r" (p_excf),
              [p_s_excf] "+r" (p_s_excf)
            :
            :"memory",  "hi", "lo"
        );
    }
#endif

    if (s <= (67108864L >> 1))
    {
        s_excf = excf;
        h_fac = 12;
        scaling = 0;
    }
    else
    {
        /* "excf[]" is divided by 2 */
        s_excf = scaled_excf;
        h_fac = 14;
        scaling = 2;
    }

    /* loop for every possible period */
    Word16 t_L_subfr = L_subfr - 1;
    for (i = t_min; i <= t_max; i++)
    {
        /* Compute 1/sqrt(energy of excf[]) */

        int temp1,temp2,temp3,temp4;
#if (MIPS_DSP_R2_LE)
        __asm__ volatile (
            ".set push                                             \n\t"
            ".set noreorder                                        \n\t"
            "move           %[p_x],       %[xn]                    \n\t"
            "move           %[p_s_excf],  %[s_excf]                \n\t"
            "mult           $ac0,         $zero,          $zero    \n\t"
            "mult           $ac1,         $zero,          $zero    \n\t"
            "li             %[j],         10                       \n\t"
          "1:                                                      \n\t" //while (j--)
            "ulw            %[temp1],     0(%[p_x])                \n\t"
            "ulw            %[temp2],     4(%[p_x])                \n\t"
            "ulw            %[temp3],     0(%[p_s_excf])           \n\t"
            "ulw            %[temp4],     4(%[p_s_excf])           \n\t"
            "addiu          %[p_x],       %[p_x],         8        \n\t"
            "addiu          %[p_s_excf],  %[p_s_excf],    8        \n\t"
            "maq_s.w.phr    $ac0,         %[temp1],       %[temp3] \n\t"
            "maq_s.w.phr    $ac0,         %[temp2],       %[temp4] \n\t"
            "maq_s.w.phl    $ac0,         %[temp1],       %[temp3] \n\t"
            "maq_s.w.phl    $ac0,         %[temp2],       %[temp4] \n\t"
            "addiu          %[j],         %[j],           -1       \n\t"
            "maq_s.w.phr    $ac1,         %[temp3],       %[temp3] \n\t"
            "maq_s.w.phl    $ac1,         %[temp3],       %[temp3] \n\t"
            "maq_s.w.phr    $ac1,         %[temp4],       %[temp4] \n\t"
            "bnez           %[j],         1b                       \n\t"
            " maq_s.w.phl   $ac1,         %[temp4],       %[temp4] \n\t"
            "extr.w         %[s],         $ac0,           1        \n\t"
            "extr.w         %[s2],        $ac1,           1        \n\t"
            ".set pop                                              \n\t"

            :[p_x] "=&r" (p_x), [p_s_excf] "=&r" (p_s_excf),
             [s] "+r" (s), [s2] "=&r" (s2), [temp1] "=&r" (temp1),
             [temp2] "=&r" (temp2), [temp3] "=&r" (temp3),
             [temp4] "=&r" (temp4), [j] "=&r" (j)
            :[xn] "r" (xn), [s_excf] "r" (s_excf)
            :"memory", "hi", "lo", "$ac1hi", "$ac1lo"
        );
#elif (MIPS_DSP_R1_LE)/*(MIPS_DSP_R1_LE)*/
        __asm__ volatile (
            ".set push                                             \n\t"
            ".set noreorder                                        \n\t"
            "move           %[p_x],       %[xn]                    \n\t"
            "move           %[p_s_excf],  %[s_excf]                \n\t"
            "mult           $ac0,         $zero,          $zero    \n\t"
            "mult           $ac1,         $zero,          $zero    \n\t"
            "li             %[j],         10                       \n\t"
          "1:                                                      \n\t" //while (j--)
            "ulw            %[temp1],     0(%[p_x])                \n\t"
            "ulw            %[temp2],     4(%[p_x])                \n\t"
            "ulw            %[temp3],     0(%[p_s_excf])           \n\t"
            "ulw            %[temp4],     4(%[p_s_excf])           \n\t"
            "addiu          %[p_x],       %[p_x],         8        \n\t"
            "addiu          %[p_s_excf],  %[p_s_excf],    8        \n\t"
            "maq_s.w.phr    $ac0,         %[temp1],       %[temp3] \n\t"
            "maq_s.w.phr    $ac0,         %[temp2],       %[temp4] \n\t"
            "maq_s.w.phl    $ac0,         %[temp1],       %[temp3] \n\t"
            "maq_s.w.phl    $ac0,         %[temp2],       %[temp4] \n\t"
            "addiu          %[j],         %[j],           -1       \n\t"
            "maq_s.w.phr    $ac1,         %[temp3],       %[temp3] \n\t"
            "maq_s.w.phl    $ac1,         %[temp3],       %[temp3] \n\t"
            "maq_s.w.phr    $ac1,         %[temp4],       %[temp4] \n\t"
            "bnez           %[j],         1b                       \n\t"
            " maq_s.w.phl   $ac1,         %[temp4],       %[temp4] \n\t"
            "extr.w         %[s],         $ac0,           1        \n\t"
            "extr.w         %[s2],        $ac1,           1        \n\t"
            ".set pop                                              \n\t"

            :[p_x] "=&r" (p_x), [p_s_excf] "=&r" (p_s_excf),
             [s] "+r" (s), [s2] "=&r" (s2), [temp1] "=&r" (temp1),
             [temp2] "=&r" (temp2), [temp3] "=&r" (temp3),
             [temp4] "=&r" (temp4), [j] "=&r" (j)
            :[xn] "r" (xn), [s_excf] "r" (s_excf)
            :"memory", "hi", "lo", "$ac1hi", "$ac1lo"
        );
#elif (MIPS32_R2_LE)
        int temp5,temp6,temp7,temp8;
        __asm__ volatile (
            ".set push                                             \n\t"
            ".set noreorder                                        \n\t"
            "move           %[p_x],       %[xn]                    \n\t"
            "move           %[p_s_excf],  %[s_excf]                \n\t"
            "move           %[s],         $zero                    \n\t"
            "move           %[s2],        $zero                    \n\t"
            "li             %[j],         10                       \n\t"
            //j= L_subfr >> 1;
          "1:                                                      \n\t" //while (j--)
            "lh             %[temp1],     0(%[p_x])                \n\t"
            "lh             %[temp2],     2(%[p_x])                \n\t"
            "lh             %[temp3],     0(%[p_s_excf])           \n\t"
            "lh             %[temp4],     2(%[p_s_excf])           \n\t"
            "lh             %[temp5],     4(%[p_x])                \n\t"
            "lh             %[temp6],     6(%[p_x])                \n\t"
            "lh             %[temp7],     4(%[p_s_excf])           \n\t"
            "lh             %[temp8],     6(%[p_s_excf])           \n\t"
            "addiu          %[p_x],       %[p_x],         8        \n\t"
            "addiu          %[p_s_excf],  %[p_s_excf],    8        \n\t"
            "mul            %[temp1],     %[temp1],       %[temp3] \n\t"
            "mul            %[temp3],     %[temp3],       %[temp3] \n\t"
            "mul            %[temp2],     %[temp2],       %[temp4] \n\t"
            "mul            %[temp4],     %[temp4],       %[temp4] \n\t"
            "mul            %[temp5],     %[temp5],       %[temp7] \n\t"
            "mul            %[temp7],     %[temp7],       %[temp7] \n\t"
            "mul            %[temp6],     %[temp6],       %[temp8] \n\t"
            "mul            %[temp8],     %[temp8],       %[temp8] \n\t"
            "addiu          %[j],         %[j],           -1       \n\t"
            "addu           %[s],         %[s],           %[temp1] \n\t"
            "addu           %[s2],        %[s2],          %[temp3] \n\t"
            "addu           %[s],         %[s],           %[temp2] \n\t"
            "addu           %[s2],        %[s2],          %[temp4] \n\t"
            "addu           %[s],         %[s],           %[temp5] \n\t"
            "addu           %[s2],        %[s2],          %[temp7] \n\t"
            "addu           %[s],         %[s],           %[temp6] \n\t"
            "bnez           %[j],         1b                       \n\t"
            " addu          %[s2],        %[s2],          %[temp8] \n\t"
            ".set pop                                              \n\t"

            :[p_x] "=&r" (p_x), [p_s_excf] "=&r" (p_s_excf),
             [s] "+r" (s), [s2] "=&r" (s2), [j] "=&r" (j),
             [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
             [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
             [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
             [temp7] "=&r" (temp7), [temp8] "=&r" (temp8)
            :[xn] "r" (xn), [s_excf] "r" (s_excf)
            :"memory", "hi", "lo"
        );
#endif

        s2     = s2 << 1;
        s2     = Inv_sqrt(s2, pOverflow);
        norm_h = (Word16)(s2 >> 16);
        norm_l = (Word16)((s2 >> 1) - (norm_h << 15));
        corr_h = (Word16)(s >> 15);
        corr_l = (Word16)((s) - (corr_h << 15));

        /* Normalize correlation = correlation * (1/sqrt(energy)) */

        s = Mpy_32(corr_h, corr_l, norm_h, norm_l, pOverflow);

        corr_norm[i] = (Word16) s ;

        /* modify the filtered excitation excf[] for the next iteration */


        if (i != t_max)
        {
            Word32 temp1, temp2, temp3, temp4, temp5, temp6;
            Word32 temp7, temp8,temp9;;

            __asm__ volatile (
                "addiu          %[k],        %[k],             -1          \n\t"
                "sll            %[temp1],    %[t_L_subfr],     1           \n\t"
                "sll            %[temp2],    %[k],             1           \n\t"
                "addu           %[temp2],    %[temp2],         %[exc]      \n\t"
                "addu           %[p_s_excf], %[s_excf],        %[temp1]    \n\t"
                "addu           %[p_h],      %[h],             %[temp1]    \n\t"
                "addiu          %[p_excf],   %[p_s_excf],      -2          \n\t"
                "lh             %[temp],     0(%[temp2])                   \n\t"
                "lh             %[temp1],    0(%[p_h])                     \n\t"
                "lh             %[temp2],    -2(%[p_h])                    \n\t"
                "lh             %[temp3],    -4(%[p_h])                    \n\t"
                "lh             %[temp4],    -6(%[p_h])                    \n\t"
                "lh             %[temp5],    -8(%[p_h])                    \n\t"
                "lh             %[temp6],    -10(%[p_h])                   \n\t"
                "mul            %[temp1],    %[temp1],         %[temp]     \n\t"
                "mul            %[temp2],    %[temp2],         %[temp]     \n\t"
                "mul            %[temp3],    %[temp3],         %[temp]     \n\t"
                "mul            %[temp4],    %[temp4],         %[temp]     \n\t"
                "mul            %[temp5],    %[temp5],         %[temp]     \n\t"
                "mul            %[temp6],    %[temp6],         %[temp]     \n\t"
                "lh             %[temp7],    0(%[p_excf])                  \n\t"
                "lh             %[temp8],    -2(%[p_excf])                 \n\t"
                "lh             %[temp9],    -4(%[p_excf])                 \n\t"
                "srav           %[temp1],    %[temp1],         %[h_fac]    \n\t"
                "srav           %[temp2],    %[temp2],         %[h_fac]    \n\t"
                "srav           %[temp3],    %[temp3],         %[h_fac]    \n\t"
                "srav           %[temp4],    %[temp4],         %[h_fac]    \n\t"
                "srav           %[temp5],    %[temp5],         %[h_fac]    \n\t"
                "srav           %[temp6],    %[temp6],         %[h_fac]    \n\t"
                "addu           %[temp1],    %[temp7],         %[temp1]    \n\t"
                "addu           %[temp2],    %[temp8],         %[temp2]    \n\t"
                "addu           %[temp3],    %[temp9],         %[temp3]    \n\t"
                "lh             %[temp7],    -6(%[p_excf])                 \n\t"
                "lh             %[temp8],    -8(%[p_excf])                 \n\t"
                "lh             %[temp9],    -10(%[p_excf])                \n\t"
                "addu           %[temp4],    %[temp7],         %[temp4]    \n\t"
                "addu           %[temp5],    %[temp8],         %[temp5]    \n\t"
                "addu           %[temp6],    %[temp9],         %[temp6]    \n\t"
                "sh             %[temp1],    0(%[p_s_excf])                \n\t"
                "sh             %[temp2],    -2(%[p_s_excf])               \n\t"
                "sh             %[temp3],    -4(%[p_s_excf])               \n\t"
                "sh             %[temp4],    -6(%[p_s_excf])               \n\t"
                "sh             %[temp5],    -8(%[p_s_excf])               \n\t"
                "sh             %[temp6],    -10(%[p_s_excf])              \n\t"
                "lh             %[temp1],    -12(%[p_h])                   \n\t"
                "lh             %[temp2],    -14(%[p_h])                   \n\t"
                "lh             %[temp3],    -16(%[p_h])                   \n\t"
                "lh             %[temp4],    -18(%[p_h])                   \n\t"
                "lh             %[temp5],    -20(%[p_h])                   \n\t"
                "lh             %[temp6],    -22(%[p_h])                   \n\t"
                "mul            %[temp1],    %[temp1],           %[temp]   \n\t"
                "mul            %[temp2],    %[temp2],           %[temp]   \n\t"
                "mul            %[temp3],    %[temp3],           %[temp]   \n\t"
                "mul            %[temp4],    %[temp4],           %[temp]   \n\t"
                "mul            %[temp5],    %[temp5],           %[temp]   \n\t"
                "mul            %[temp6],    %[temp6],           %[temp]   \n\t"
                "lh             %[temp7],    -12(%[p_excf])                \n\t"
                "lh             %[temp8],    -14(%[p_excf])                \n\t"
                "lh             %[temp9],    -16(%[p_excf])                \n\t"
                "srav           %[temp1],    %[temp1],           %[h_fac]  \n\t"
                "srav           %[temp2],    %[temp2],           %[h_fac]  \n\t"
                "srav           %[temp3],    %[temp3],           %[h_fac]  \n\t"
                "srav           %[temp4],    %[temp4],           %[h_fac]  \n\t"
                "srav           %[temp5],    %[temp5],           %[h_fac]  \n\t"
                "srav           %[temp6],    %[temp6],           %[h_fac]  \n\t"
                "addu           %[temp1],    %[temp7],           %[temp1]  \n\t"
                "addu           %[temp2],    %[temp8],           %[temp2]  \n\t"
                "addu           %[temp3],    %[temp9],           %[temp3]  \n\t"
                "lh             %[temp7],    -18(%[p_excf])                \n\t"
                "lh             %[temp8],    -20(%[p_excf])                \n\t"
                "lh             %[temp9],    -22(%[p_excf])                \n\t"
                "addu           %[temp4],    %[temp7],           %[temp4]  \n\t"
                "addu           %[temp5],    %[temp8],           %[temp5]  \n\t"
                "addu           %[temp6],    %[temp9],           %[temp6]  \n\t"
                "sh             %[temp1],    -12(%[p_s_excf])              \n\t"
                "sh             %[temp2],    -14(%[p_s_excf])              \n\t"
                "sh             %[temp3],    -16(%[p_s_excf])              \n\t"
                "sh             %[temp4],    -18(%[p_s_excf])              \n\t"
                "sh             %[temp5],    -20(%[p_s_excf])              \n\t"
                "sh             %[temp6],    -22(%[p_s_excf])              \n\t"
                "lh             %[temp1],    -24(%[p_h])                   \n\t"
                "lh             %[temp2],    -26(%[p_h])                   \n\t"
                "lh             %[temp3],    -28(%[p_h])                   \n\t"
                "lh             %[temp4],    -30(%[p_h])                   \n\t"
                "lh             %[temp5],    -32(%[p_h])                   \n\t"
                "lh             %[temp6],    -34(%[p_h])                   \n\t"
                "mul            %[temp1],    %[temp1],          %[temp]    \n\t"
                "mul            %[temp2],    %[temp2],          %[temp]    \n\t"
                "mul            %[temp3],    %[temp3],          %[temp]    \n\t"
                "mul            %[temp4],    %[temp4],          %[temp]    \n\t"
                "mul            %[temp5],    %[temp5],          %[temp]    \n\t"
                "mul            %[temp6],    %[temp6],          %[temp]    \n\t"
                "lh             %[temp7],    -24(%[p_excf])                \n\t"
                "lh             %[temp8],    -26(%[p_excf])                \n\t"
                "lh             %[temp9],    -28(%[p_excf])                \n\t"
                "srav           %[temp1],    %[temp1],          %[h_fac]   \n\t"
                "srav           %[temp2],    %[temp2],          %[h_fac]   \n\t"
                "srav           %[temp3],    %[temp3],          %[h_fac]   \n\t"
                "srav           %[temp4],    %[temp4],          %[h_fac]   \n\t"
                "srav           %[temp5],    %[temp5],          %[h_fac]   \n\t"
                "srav           %[temp6],    %[temp6],          %[h_fac]   \n\t"
                "addu           %[temp1],    %[temp7],          %[temp1]   \n\t"
                "addu           %[temp2],    %[temp8],          %[temp2]   \n\t"
                "addu           %[temp3],    %[temp9],          %[temp3]   \n\t"
                "lh             %[temp7],    -30(%[p_excf])                \n\t"
                "lh             %[temp8],    -32(%[p_excf])                \n\t"
                "lh             %[temp9],    -34(%[p_excf])                \n\t"
                "addu           %[temp4],    %[temp7],          %[temp4]   \n\t"
                "addu           %[temp5],    %[temp8],          %[temp5]   \n\t"
                "addu           %[temp6],    %[temp9],          %[temp6]   \n\t"
                "sh             %[temp1],    -24(%[p_s_excf])              \n\t"
                "sh             %[temp2],    -26(%[p_s_excf])              \n\t"
                "sh             %[temp3],    -28(%[p_s_excf])              \n\t"
                "sh             %[temp4],    -30(%[p_s_excf])              \n\t"
                "sh             %[temp5],    -32(%[p_s_excf])              \n\t"
                "sh             %[temp6],    -34(%[p_s_excf])              \n\t"
                "lh             %[temp1],    -36(%[p_h])                   \n\t"
                "lh             %[temp2],    -38(%[p_h])                   \n\t"
                "lh             %[temp3],    -40(%[p_h])                   \n\t"
                "lh             %[temp4],    -42(%[p_h])                   \n\t"
                "lh             %[temp5],    -44(%[p_h])                   \n\t"
                "lh             %[temp6],    -46(%[p_h])                   \n\t"
                "mul            %[temp1],    %[temp1],          %[temp]    \n\t"
                "mul            %[temp2],    %[temp2],          %[temp]    \n\t"
                "mul            %[temp3],    %[temp3],          %[temp]    \n\t"
                "mul            %[temp4],    %[temp4],          %[temp]    \n\t"
                "mul            %[temp5],    %[temp5],          %[temp]    \n\t"
                "mul            %[temp6],    %[temp6],          %[temp]    \n\t"
                "lh             %[temp7],    -36(%[p_excf])                \n\t"
                "lh             %[temp8],    -38(%[p_excf])                \n\t"
                "lh             %[temp9],    -40(%[p_excf])                \n\t"
                "srav           %[temp1],    %[temp1],          %[h_fac]   \n\t"
                "srav           %[temp2],    %[temp2],          %[h_fac]   \n\t"
                "srav           %[temp3],    %[temp3],          %[h_fac]   \n\t"
                "srav           %[temp4],    %[temp4],          %[h_fac]   \n\t"
                "srav           %[temp5],    %[temp5],          %[h_fac]   \n\t"
                "srav           %[temp6],    %[temp6],          %[h_fac]   \n\t"
                "addu           %[temp1],    %[temp7],          %[temp1]   \n\t"
                "addu           %[temp2],    %[temp8],          %[temp2]   \n\t"
                "addu           %[temp3],    %[temp9],          %[temp3]   \n\t"
                "lh             %[temp7],    -42(%[p_excf])                \n\t"
                "lh             %[temp8],    -44(%[p_excf])                \n\t"
                "lh             %[temp9],    -46(%[p_excf])                \n\t"
                "addu           %[temp4],    %[temp7],          %[temp4]   \n\t"
                "addu           %[temp5],    %[temp8],          %[temp5]   \n\t"
                "addu           %[temp6],    %[temp9],          %[temp6]   \n\t"
                "sh             %[temp1],    -36(%[p_s_excf])              \n\t"
                "sh             %[temp2],    -38(%[p_s_excf])              \n\t"
                "sh             %[temp3],    -40(%[p_s_excf])              \n\t"
                "sh             %[temp4],    -42(%[p_s_excf])              \n\t"
                "sh             %[temp5],    -44(%[p_s_excf])              \n\t"
                "sh             %[temp6],    -46(%[p_s_excf])              \n\t"
                "lh             %[temp1],    -48(%[p_h])                   \n\t"
                "lh             %[temp2],    -50(%[p_h])                   \n\t"
                "lh             %[temp3],    -52(%[p_h])                   \n\t"
                "lh             %[temp4],    -54(%[p_h])                   \n\t"
                "lh             %[temp5],    -56(%[p_h])                   \n\t"
                "lh             %[temp6],    -58(%[p_h])                   \n\t"
                "mul            %[temp1],    %[temp1],          %[temp]    \n\t"
                "mul            %[temp2],    %[temp2],          %[temp]    \n\t"
                "mul            %[temp3],    %[temp3],          %[temp]    \n\t"
                "mul            %[temp4],    %[temp4],          %[temp]    \n\t"
                "mul            %[temp5],    %[temp5],          %[temp]    \n\t"
                "mul            %[temp6],    %[temp6],          %[temp]    \n\t"
                "lh             %[temp7],    -48(%[p_excf])                \n\t"
                "lh             %[temp8],    -50(%[p_excf])                \n\t"
                "lh             %[temp9],    -52(%[p_excf])                \n\t"
                "srav           %[temp1],    %[temp1],          %[h_fac]   \n\t"
                "srav           %[temp2],    %[temp2],          %[h_fac]   \n\t"
                "srav           %[temp3],    %[temp3],          %[h_fac]   \n\t"
                "srav           %[temp4],    %[temp4],          %[h_fac]   \n\t"
                "srav           %[temp5],    %[temp5],          %[h_fac]   \n\t"
                "srav           %[temp6],    %[temp6],          %[h_fac]   \n\t"
                "addu           %[temp1],    %[temp7],          %[temp1]   \n\t"
                "addu           %[temp2],    %[temp8],          %[temp2]   \n\t"
                "addu           %[temp3],    %[temp9],          %[temp3]   \n\t"
                "lh             %[temp7],    -54(%[p_excf])                \n\t"
                "lh             %[temp8],    -56(%[p_excf])                \n\t"
                "lh             %[temp9],    -58(%[p_excf])                \n\t"
                "addu           %[temp4],    %[temp7],          %[temp4]   \n\t"
                "addu           %[temp5],    %[temp8],          %[temp5]   \n\t"
                "addu           %[temp6],    %[temp9],          %[temp6]   \n\t"
                "sh             %[temp1],    -48(%[p_s_excf])              \n\t"
                "sh             %[temp2],    -50(%[p_s_excf])              \n\t"
                "sh             %[temp3],    -52(%[p_s_excf])              \n\t"
                "sh             %[temp4],    -54(%[p_s_excf])              \n\t"
                "sh             %[temp5],    -56(%[p_s_excf])              \n\t"
                "sh             %[temp6],    -58(%[p_s_excf])              \n\t"
                "lh             %[temp1],    -60(%[p_h])                   \n\t"
                "lh             %[temp2],    -62(%[p_h])                   \n\t"
                "lh             %[temp3],    -64(%[p_h])                   \n\t"
                "lh             %[temp4],    -66(%[p_h])                   \n\t"
                "lh             %[temp5],    -68(%[p_h])                   \n\t"
                "lh             %[temp6],    -70(%[p_h])                   \n\t"
                "mul            %[temp1],    %[temp1],          %[temp]    \n\t"
                "mul            %[temp2],    %[temp2],          %[temp]    \n\t"
                "mul            %[temp3],    %[temp3],          %[temp]    \n\t"
                "mul            %[temp4],    %[temp4],          %[temp]    \n\t"
                "mul            %[temp5],    %[temp5],          %[temp]    \n\t"
                "mul            %[temp6],    %[temp6],          %[temp]    \n\t"
                "lh             %[temp7],    -60(%[p_excf])                \n\t"
                "lh             %[temp8],    -62(%[p_excf])                \n\t"
                "lh             %[temp9],    -64(%[p_excf])                \n\t"
                "srav           %[temp1],    %[temp1],          %[h_fac]   \n\t"
                "srav           %[temp2],    %[temp2],          %[h_fac]   \n\t"
                "srav           %[temp3],    %[temp3],          %[h_fac]   \n\t"
                "srav           %[temp4],    %[temp4],          %[h_fac]   \n\t"
                "srav           %[temp5],    %[temp5],          %[h_fac]   \n\t"
                "srav           %[temp6],    %[temp6],          %[h_fac]   \n\t"
                "addu           %[temp1],    %[temp7],          %[temp1]   \n\t"
                "addu           %[temp2],    %[temp8],          %[temp2]   \n\t"
                "addu           %[temp3],    %[temp9],          %[temp3]   \n\t"
                "lh             %[temp7],    -66(%[p_excf])                \n\t"
                "lh             %[temp8],    -68(%[p_excf])                \n\t"
                "lh             %[temp9],    -70(%[p_excf])                \n\t"
                "addu           %[temp4],    %[temp7],          %[temp4]   \n\t"
                "addu           %[temp5],    %[temp8],          %[temp5]   \n\t"
                "addu           %[temp6],    %[temp9],          %[temp6]   \n\t"
                "sh             %[temp1],    -60(%[p_s_excf])              \n\t"
                "sh             %[temp2],    -62(%[p_s_excf])              \n\t"
                "sh             %[temp3],    -64(%[p_s_excf])              \n\t"
                "lh             %[temp1],    -72(%[p_h])                   \n\t"
                "lh             %[temp2],    -74(%[p_h])                   \n\t"
                "lh             %[temp3],    -76(%[p_h])                   \n\t"
                "mul            %[temp1],    %[temp1],          %[temp]    \n\t"
                "mul            %[temp2],    %[temp2],          %[temp]    \n\t"
                "mul            %[temp3],    %[temp3],          %[temp]    \n\t"
                "sh             %[temp4],    -66(%[p_s_excf])              \n\t"
                "sh             %[temp5],    -68(%[p_s_excf])              \n\t"
                "sh             %[temp6],    -70(%[p_s_excf])              \n\t"
                "lh             %[temp7],    -72(%[p_excf])                \n\t"
                "lh             %[temp8],    -74(%[p_excf])                \n\t"
                "lh             %[temp9],    -76(%[p_excf])                \n\t"
                "srav           %[temp1],    %[temp1],          %[h_fac]   \n\t"
                "srav           %[temp2],    %[temp2],          %[h_fac]   \n\t"
                "srav           %[temp3],    %[temp3],          %[h_fac]   \n\t"
                "addu           %[temp1],    %[temp7],          %[temp1]   \n\t"
                "addu           %[temp2],    %[temp8],          %[temp2]   \n\t"
                "addu           %[temp3],    %[temp9],          %[temp3]   \n\t"
                "sh             %[temp1],    -72(%[p_s_excf])              \n\t"
                "sh             %[temp2],    -74(%[p_s_excf])              \n\t"
                "sh             %[temp3],    -76(%[p_s_excf])              \n\t"
                "srav           %[temp5],    %[temp],           %[scaling] \n\t"
                "sh             %[temp5],    -78(%[p_s_excf])              \n\t"

                :[temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                 [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
                 [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
                 [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
                 [temp9] "=&r" (temp9), [k] "+r" (k), [temp] "=&r" (temp),
                 [p_h] "=&r" (p_h), [p_excf] "=&r" (p_excf),
                 [p_s_excf] "=&r" (p_s_excf)
                :[h_fac] "r" (h_fac),[t_L_subfr] "r" (t_L_subfr),
                 [s_excf] "r" (s_excf),
                 [h] "r" (h),[scaling] "r" (scaling), [exc] "r" (exc)
                :"memory", "hi", "lo"
            );
        }
    }
    return;
}
