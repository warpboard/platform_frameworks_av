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



 Pathname: ./audio/gsm-amr/c/src/cor_h_x.c

     Date: 09/07/2000

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Created a separate file for cor_h_x function.

 Description: Synchronized file with UMTS versin 3.2.0. Updated coding
              template.

 Description: Made the following changes per comments from Phase 2/3 review:
              1. Modified FOR loop in the code to count down.
              2. Fixed typecasting issue with TI C compiler.

 Description: Added call to round() and L_shl() functions in the last FOR
              loop to make code bit-exact. Updated copyright year.

 Description: Modified to pass pOverflow in via a pointer, rather than
 invoking it as a global variable.

 Description: Made the following changes
              1. Unrolled the correlation loop and add mechanism control
                 to compute odd or even number of computations.
              2. Use pointer to avoid continuos addresses calculation
              2. Eliminated math operations that check for saturation.

 Description: Changed round function name to pv_round to avoid conflict with
              round function in C standard library.

 Description:

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/
#include "typedef.h"
#include "cnst.h"
#include "cor_h_x.h"
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
; LOCAL STORE/BUFFER/POINTER DEFINITIONS
; Variable declaration - defined here and used outside this module
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; EXTERNAL FUNCTION REFERENCES
; Declare functions defined elsewhere and referenced in this module
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; EXTERNAL GLOBAL STORE/BUFFER/POINTER REFERENCES
; Declare variables used in this module but defined elsewhere
----------------------------------------------------------------------------*/

/*
------------------------------------------------------------------------------
 FUNCTION NAME: cor_h_x
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    h = vector containing the impulse response of the weighted synthesis
        filter; vector contents are of type Word16; vector length is
        2 * L_SUBFR
    x = target signal vector; vector contents are of type Word16; vector
        length is L_SUBFR
    dn = vector containing the correlation between the target and the
         impulse response; vector contents are of type Word16; vector
         length is L_CODE
    sf = scaling factor of type Word16 ; 2 when mode is MR122, 1 for all
         other modes

 Outputs:
    dn contents are the newly calculated correlation values

    pOverflow = pointer of type Flag * to overflow indicator.

 Returns:
    None

 Global Variables Used:
    None

 Local Variables Needed:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

 This function computes the correlation between the target signal (x) and the
 impulse response (h).

 The correlation is given by: d[n] = sum_{i=n}^{L-1} x[i] h[i-n],
 where: n=0,...,L-1

 d[n] is normalized such that the sum of 5 maxima of d[n] corresponding to
 each position track does not saturate.

------------------------------------------------------------------------------
 REQUIREMENTS

 None

------------------------------------------------------------------------------
 REFERENCES

 cor_h.c, UMTS GSM AMR speech codec, R99 - Version 3.2.0, March 2, 2001

------------------------------------------------------------------------------
 PSEUDO-CODE

void cor_h_x (
    Word16 h[],    // (i): impulse response of weighted synthesis filter
    Word16 x[],    // (i): target
    Word16 dn[],   // (o): correlation between target and h[]
    Word16 sf      // (i): scaling factor: 2 for 12.2, 1 for others
)
{
    cor_h_x2(h, x, dn, sf, NB_TRACK, STEP);
}


void cor_h_x2 (
    Word16 h[],    // (i): impulse response of weighted synthesis filter
    Word16 x[],    // (i): target
    Word16 dn[],   // (o): correlation between target and h[]
    Word16 sf,     // (i): scaling factor: 2 for 12.2, 1 for others
    Word16 nb_track,// (i): the number of ACB tracks
    Word16 step    // (i): step size from one pulse position to the next
                           in one track
)
{
    Word16 i, j, k;
    Word32 s, y32[L_CODE], max, tot;

    // first keep the result on 32 bits and find absolute maximum

    tot = 5;

    for (k = 0; k < nb_track; k++)
    {
        max = 0;
        for (i = k; i < L_CODE; i += step)
        {
            s = 0;
            for (j = i; j < L_CODE; j++)
                s = L_mac (s, x[j], h[j - i]);

            y32[i] = s;

            s = L_abs (s);
            if (L_sub (s, max) > (Word32) 0L)
                max = s;
        }
        tot = L_add (tot, L_shr (max, 1));
    }

    j = sub (norm_l (tot), sf);

    for (i = 0; i < L_CODE; i++)
    {
        dn[i] = pv_round (L_shl (y32[i], j));
    }
}

------------------------------------------------------------------------------
 RESOURCES USED [optional]

 When the code is written for a specific target processor the
 the resources used should be documented below.

 HEAP MEMORY USED: x bytes

 STACK MEMORY USED: x bytes

 CLOCK CYCLES: (cycle count equation for this function) + (variable
                used to represent cycle count for each subroutine
                called)
     where: (cycle count variable) = cycle count for [subroutine
                                     name]

------------------------------------------------------------------------------
 CAUTION [optional]
 [State any special notes, constraints or cautions for users of this function]

------------------------------------------------------------------------------
*/

void cor_h_x(
    Word16 h[],       /* (i): impulse response of weighted synthesis filter */
    Word16 x[],       /* (i): target                                        */
    Word16 dn[],      /* (o): correlation between target and h[]            */
    Word16 sf,        /* (i): scaling factor: 2 for 12.2, 1 for others      */
    Flag   *pOverflow /* (o): pointer to overflow flag                      */
)
{
    register Word16 i;
    register Word16 j;
    register Word16 k;

    Word32 s;
    Word32 y32[L_CODE];
    Word32 max;
    Word32 tot;

    Word16 *p_x;
    Word16 *p_ptr;
    Word32 *p_y32;


    tot = 5;
#if (MIPS_DSP_R2_LE || MIPS_DSP_R1_LE)
    for (k = 0; k < NB_TRACK; k++)              /* NB_TRACK = 5 */
    {
        max = 0;
        for (i = k; i < L_CODE; i += STEP)      /* L_CODE = 40; STEP = 5 */
        {
            s = 0;
            p_x = &x[i];
            p_ptr = h;
            Word32 temp1,temp2,temp3;

            __asm__ volatile (
                "mult            $ac1,      $zero,            $zero        \n\t"
                "li              %[j],      39                             \n\t"
                "subu            %[j],      %[j],             %[i]         \n\t"
                "sra             %[j],      %[j],             1            \n\t"
                "blezl           %[j],      44f                            \n\t"
             "55:                                                          \n\t"
                "ulw             %[temp1],  0(%[p_x])                      \n\t"
                "ulw             %[temp2],  0(%[p_ptr])                    \n\t"
                "addiu           %[j],      %[j],             -1           \n\t"
                "addiu           %[p_x],    %[p_x],           4            \n\t"
                "addiu           %[p_ptr],  %[p_ptr],         4            \n\t"
                "dpaq_s.w.ph     $ac1,      %[temp1],         %[temp2]     \n\t"
                "bnez            %[j],      55b                            \n\t"
             "44:                                                          \n\t"
                "lh             %[temp1],   0(%[p_x])                      \n\t"
                "lh             %[temp2],   0(%[p_ptr])                    \n\t"
                "addiu          %[p_x],     %[p_x],           2            \n\t"
                "addiu          %[p_ptr],   %[p_ptr],         2            \n\t"
                "maq_s.w.phr    $ac1,       %[temp1],         %[temp2]     \n\t"
                "li             %[temp3],   40                             \n\t"
                "subu           %[temp3],   %[temp3],         %[i]         \n\t"
                "and            %[temp3],   %[temp3],         1            \n\t"
                "bnez           %[temp3],   3f                             \n\t"
                "lh             %[temp1],   0(%[p_x])                      \n\t"
                "lh             %[temp2],   0(%[p_ptr])                    \n\t"
                "addiu          %[p_x],     %[p_x],           2            \n\t"
                "addiu          %[p_ptr],   %[p_ptr],         2            \n\t"
                "maq_s.w.phr    $ac1,       %[temp1],         %[temp2]     \n\t"
             "3:                                                           \n\t"
                "mflo           %[s],       $ac1                           \n\t"
                "absq_s.w       %[temp3],   %[s]                           \n\t"


                : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2), [j] "=&r" (j),
                  [p_x] "+r" (p_x), [p_ptr] "+r" (p_ptr),[s] "=&r" (s),
                  [temp3] "=&r" (temp3)
                : [i] "r" (i)
                : "memory", "$ac1hi", "$ac1lo"
            );

            y32[i] = s;

            if (temp3 > max)
            {
                max = temp3;
            }
        }

        tot += (max >> 1);
    }
#elif (MIPS32_R2_LE)
    for (k = 0; k < NB_TRACK; k++)              /* NB_TRACK = 5 */
    {
        max = 0;
        for (i = k; i < L_CODE; i += STEP)      /* L_CODE = 40; STEP = 5 */
        {
            p_x = &x[i];
            p_ptr = h;
            Word32 temp1,temp2,temp3,temp4;

            __asm__ volatile (
                "mult            $zero,     $zero                          \n\t"
                "li              %[j],      39                             \n\t"
                "subu            %[j],      %[j],             %[i]         \n\t"
                "sra             %[j],      %[j],             1            \n\t"
                "blezl           %[j],      44f                            \n\t"
             "55:                                                          \n\t"
                "lh             %[temp1],   0(%[p_x])                      \n\t"
                "lh             %[temp2],   0(%[p_ptr])                    \n\t"
                "lh             %[temp3],   2(%[p_x])                      \n\t"
                "lh             %[temp4],   2(%[p_ptr])                    \n\t"
                "addiu          %[p_x],     %[p_x],           4            \n\t"
                "addiu          %[p_ptr],   %[p_ptr],         4            \n\t"
                "addiu          %[j],       %[j],             -1           \n\t"
                "madd           %[temp1],   %[temp2]                       \n\t"
                "madd           %[temp3],   %[temp4]                       \n\t"
                "bnez           %[j],       55b                            \n\t"
             "44:                                                          \n\t"
                "lh             %[temp1],   0(%[p_x])                      \n\t"
                "lh             %[temp2],   0(%[p_ptr])                    \n\t"
                "addiu          %[p_x],     %[p_x],           2            \n\t"
                "addiu          %[p_ptr],   %[p_ptr],         2            \n\t"
                "madd           %[temp1],   %[temp2]                       \n\t"
                "li             %[temp3],   40                             \n\t"
                "subu           %[temp3],   %[temp3],         %[i]         \n\t"
                "and            %[temp3],   %[temp3],         1            \n\t"
                "bnez           %[temp3],   3f                             \n\t"
                "lh             %[temp1],   0(%[p_x])                      \n\t"
                "lh             %[temp2],   0(%[p_ptr])                    \n\t"
                "addiu          %[p_x],     %[p_x],           2            \n\t"
                "addiu          %[p_ptr],   %[p_ptr],         2            \n\t"
                "madd           %[temp1],   %[temp2]                       \n\t"
             "3:                                                           \n\t"
                "mflo           %[temp1]                                   \n\t"
                "sll            %[temp1],   %[temp1],         1            \n\t"
                "sra            %[temp3],   %[temp1],         31           \n\t"//|temp1|
                "xor            %[temp2],   %[temp1],         %[temp3]     \n\t"
                "subu           %[temp3],   %[temp2],         %[temp3]     \n\t"

                : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2), [j] "=&r" (j),
                  [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
                  [p_x] "+r" (p_x), [p_ptr] "+r" (p_ptr)
                : [i] "r" (i)
                : "memory", "hi", "lo"
            );

            y32[i] = temp1;

            if (temp3 > max)
            {
                max = temp3;
            }
        }

        tot += (max >> 1);
    }
#endif

    j = norm_l(tot) - sf;

    p_ptr = dn;
    p_y32 = y32;;

    if(j>0){
#if (MIPS_DSP_R2_LE || MIPS_DSP_R1_LE)
        for (i = 10; i != 0; i-=5)
        {
            Word32 temp1, temp2, temp3 ,temp4;

            __asm__ volatile (
               "lw             %[temp1],   0(%[p_y32])               \n\t"
               "lw             %[temp2],   4(%[p_y32])               \n\t"
               "lw             %[temp3],   8(%[p_y32])               \n\t"
               "lw             %[temp4],   12(%[p_y32])              \n\t"
               "sllv           %[temp1],   %[temp1],         %[j]    \n\t"
               "sllv           %[temp2],   %[temp2],         %[j]    \n\t"
               "sllv           %[temp3],   %[temp3],         %[j]    \n\t"
               "sllv           %[temp4],   %[temp4],         %[j]    \n\t"
               "shra_r.w       %[temp1],   %[temp1],         16      \n\t"
               "shra_r.w       %[temp2],   %[temp2],         16      \n\t"
               "shra_r.w       %[temp3],   %[temp3],         16      \n\t"
               "shra_r.w       %[temp4],   %[temp4],         16      \n\t"
               "sh             %[temp1],   0(%[p_ptr])               \n\t"
               "sh             %[temp2],   2(%[p_ptr])               \n\t"
               "sh             %[temp3],   4(%[p_ptr])               \n\t"
               "sh             %[temp4],   6(%[p_ptr])               \n\t"
               "lw             %[temp1],   16(%[p_y32])              \n\t"
               "lw             %[temp2],   20(%[p_y32])              \n\t"
               "lw             %[temp3],   24(%[p_y32])              \n\t"
               "lw             %[temp4],   28(%[p_y32])              \n\t"
               "sllv           %[temp1],   %[temp1],         %[j]    \n\t"
               "sllv           %[temp2],   %[temp2],         %[j]    \n\t"
               "sllv           %[temp3],   %[temp3],         %[j]    \n\t"
               "sllv           %[temp4],   %[temp4],         %[j]    \n\t"
               "shra_r.w       %[temp1],   %[temp1],         16      \n\t"
               "shra_r.w       %[temp2],   %[temp2],         16      \n\t"
               "shra_r.w       %[temp3],   %[temp3],         16      \n\t"
               "shra_r.w       %[temp4],   %[temp4],         16      \n\t"
               "sh             %[temp1],   8(%[p_ptr])               \n\t"
               "sh             %[temp2],   10(%[p_ptr])              \n\t"
               "sh             %[temp3],   12(%[p_ptr])              \n\t"
               "sh             %[temp4],   14(%[p_ptr])              \n\t"
               "lw             %[temp1],   32(%[p_y32])              \n\t"
               "lw             %[temp2],   36(%[p_y32])              \n\t"
               "lw             %[temp3],   40(%[p_y32])              \n\t"
               "lw             %[temp4],   44(%[p_y32])              \n\t"
               "sllv           %[temp1],   %[temp1],         %[j]    \n\t"
               "sllv           %[temp2],   %[temp2],         %[j]    \n\t"
               "sllv           %[temp3],   %[temp3],         %[j]    \n\t"
               "sllv           %[temp4],   %[temp4],         %[j]    \n\t"
               "shra_r.w       %[temp1],   %[temp1],         16      \n\t"
               "shra_r.w       %[temp2],   %[temp2],         16      \n\t"
               "shra_r.w       %[temp3],   %[temp3],         16      \n\t"
               "shra_r.w       %[temp4],   %[temp4],         16      \n\t"
               "sh             %[temp1],   16(%[p_ptr])              \n\t"
               "sh             %[temp2],   18(%[p_ptr])              \n\t"
               "sh             %[temp3],   20(%[p_ptr])              \n\t"
               "sh             %[temp4],   22(%[p_ptr])              \n\t"
               "lw             %[temp1],   48(%[p_y32])              \n\t"
               "lw             %[temp2],   52(%[p_y32])              \n\t"
               "lw             %[temp3],   56(%[p_y32])              \n\t"
               "lw             %[temp4],   60(%[p_y32])              \n\t"
               "sllv           %[temp1],   %[temp1],         %[j]    \n\t"
               "sllv           %[temp2],   %[temp2],         %[j]    \n\t"
               "sllv           %[temp3],   %[temp3],         %[j]    \n\t"
               "sllv           %[temp4],   %[temp4],         %[j]    \n\t"
               "shra_r.w       %[temp1],   %[temp1],         16      \n\t"
               "shra_r.w       %[temp2],   %[temp2],         16      \n\t"
               "shra_r.w       %[temp3],   %[temp3],         16      \n\t"
               "shra_r.w       %[temp4],   %[temp4],         16      \n\t"
               "sh             %[temp1],   24(%[p_ptr])              \n\t"
               "sh             %[temp2],   26(%[p_ptr])              \n\t"
               "sh             %[temp3],   28(%[p_ptr])              \n\t"
               "sh             %[temp4],   30(%[p_ptr])              \n\t"
               "lw             %[temp1],   64(%[p_y32])              \n\t"
               "lw             %[temp2],   68(%[p_y32])              \n\t"
               "lw             %[temp3],   72(%[p_y32])              \n\t"
               "lw             %[temp4],   76(%[p_y32])              \n\t"
               "sllv           %[temp1],   %[temp1],         %[j]    \n\t"
               "sllv           %[temp2],   %[temp2],         %[j]    \n\t"
               "sllv           %[temp3],   %[temp3],         %[j]    \n\t"
               "sllv           %[temp4],   %[temp4],         %[j]    \n\t"
               "shra_r.w       %[temp1],   %[temp1],         16      \n\t"
               "shra_r.w       %[temp2],   %[temp2],         16      \n\t"
               "shra_r.w       %[temp3],   %[temp3],         16      \n\t"
               "shra_r.w       %[temp4],   %[temp4],         16      \n\t"
               "sh             %[temp1],   32(%[p_ptr])              \n\t"
               "sh             %[temp2],   34(%[p_ptr])              \n\t"
               "sh             %[temp3],   36(%[p_ptr])              \n\t"
               "sh             %[temp4],   38(%[p_ptr])              \n\t"

               : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                 [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
                 [p_ptr] "+r" (p_ptr), [p_y32] "+r" (p_y32)
               : [j] "r" (j)
               : "memory"
            );

            p_y32+=20;
            p_ptr+=20;
        }
#elif (MIPS32_R2_LE)
        Word32 temp_c = 0x00008000;
        for (i = 10; i != 0; i-=5)
        {
            Word32 temp1, temp2, temp3 ,temp4;
            Word32 temp5, temp6, temp7 ,temp8;

            __asm__ volatile (
               "lw             %[temp1],   0(%[p_y32])                    \n\t"
               "lw             %[temp2],   4(%[p_y32])                    \n\t"
               "lw             %[temp3],   8(%[p_y32])                    \n\t"
               "lw             %[temp4],   12(%[p_y32])                   \n\t"
               "lw             %[temp5],   16(%[p_y32])                   \n\t"
               "lw             %[temp6],   20(%[p_y32])                   \n\t"
               "lw             %[temp7],   24(%[p_y32])                   \n\t"
               "lw             %[temp8],   28(%[p_y32])                   \n\t"
               "sllv           %[temp1],   %[temp1],         %[j]         \n\t"
               "sllv           %[temp2],   %[temp2],         %[j]         \n\t"
               "sllv           %[temp3],   %[temp3],         %[j]         \n\t"
               "sllv           %[temp4],   %[temp4],         %[j]         \n\t"
               "sllv           %[temp5],   %[temp5],         %[j]         \n\t"
               "sllv           %[temp6],   %[temp6],         %[j]         \n\t"
               "sllv           %[temp7],   %[temp7],         %[j]         \n\t"
               "sllv           %[temp8],   %[temp8],         %[j]         \n\t"
               "addu           %[temp1],   %[temp1],         %[temp_c]    \n\t"
               "addu           %[temp2],   %[temp2],         %[temp_c]    \n\t"
               "addu           %[temp3],   %[temp3],         %[temp_c]    \n\t"
               "addu           %[temp4],   %[temp4],         %[temp_c]    \n\t"
               "addu           %[temp5],   %[temp5],         %[temp_c]    \n\t"
               "addu           %[temp6],   %[temp6],         %[temp_c]    \n\t"
               "addu           %[temp7],   %[temp7],         %[temp_c]    \n\t"
               "addu           %[temp8],   %[temp8],         %[temp_c]    \n\t"
               "sra            %[temp1],   %[temp1],         16           \n\t"
               "sra            %[temp2],   %[temp2],         16           \n\t"
               "sra            %[temp3],   %[temp3],         16           \n\t"
               "sra            %[temp4],   %[temp4],         16           \n\t"
               "sra            %[temp5],   %[temp5],         16           \n\t"
               "sra            %[temp6],   %[temp6],         16           \n\t"
               "sra            %[temp7],   %[temp7],         16           \n\t"
               "sra            %[temp8],   %[temp8],         16           \n\t"
               "sh             %[temp1],   0(%[p_ptr])                    \n\t"
               "sh             %[temp2],   2(%[p_ptr])                    \n\t"
               "sh             %[temp3],   4(%[p_ptr])                    \n\t"
               "sh             %[temp4],   6(%[p_ptr])                    \n\t"
               "sh             %[temp5],   8(%[p_ptr])                    \n\t"
               "sh             %[temp6],   10(%[p_ptr])                   \n\t"
               "sh             %[temp7],   12(%[p_ptr])                   \n\t"
               "sh             %[temp8],   14(%[p_ptr])                   \n\t"
               "lw             %[temp1],   32(%[p_y32])                   \n\t"
               "lw             %[temp2],   36(%[p_y32])                   \n\t"
               "lw             %[temp3],   40(%[p_y32])                   \n\t"
               "lw             %[temp4],   44(%[p_y32])                   \n\t"
               "lw             %[temp5],   48(%[p_y32])                   \n\t"
               "lw             %[temp6],   52(%[p_y32])                   \n\t"
               "lw             %[temp7],   56(%[p_y32])                   \n\t"
               "lw             %[temp8],   60(%[p_y32])                   \n\t"
               "sllv           %[temp1],   %[temp1],         %[j]         \n\t"
               "sllv           %[temp2],   %[temp2],         %[j]         \n\t"
               "sllv           %[temp3],   %[temp3],         %[j]         \n\t"
               "sllv           %[temp4],   %[temp4],         %[j]         \n\t"
               "sllv           %[temp5],   %[temp5],         %[j]         \n\t"
               "sllv           %[temp6],   %[temp6],         %[j]         \n\t"
               "sllv           %[temp7],   %[temp7],         %[j]         \n\t"
               "sllv           %[temp8],   %[temp8],         %[j]         \n\t"
               "addu           %[temp1],   %[temp1],         %[temp_c]    \n\t"
               "addu           %[temp2],   %[temp2],         %[temp_c]    \n\t"
               "addu           %[temp3],   %[temp3],         %[temp_c]    \n\t"
               "addu           %[temp4],   %[temp4],         %[temp_c]    \n\t"
               "addu           %[temp5],   %[temp5],         %[temp_c]    \n\t"
               "addu           %[temp6],   %[temp6],         %[temp_c]    \n\t"
               "addu           %[temp7],   %[temp7],         %[temp_c]    \n\t"
               "addu           %[temp8],   %[temp8],         %[temp_c]    \n\t"
               "sra            %[temp1],   %[temp1],         16           \n\t"
               "sra            %[temp2],   %[temp2],         16           \n\t"
               "sra            %[temp3],   %[temp3],         16           \n\t"
               "sra            %[temp4],   %[temp4],         16           \n\t"
               "sra            %[temp5],   %[temp5],         16           \n\t"
               "sra            %[temp6],   %[temp6],         16           \n\t"
               "sra            %[temp7],   %[temp7],         16           \n\t"
               "sra            %[temp8],   %[temp8],         16           \n\t"
               "sh             %[temp1],   16(%[p_ptr])                   \n\t"
               "sh             %[temp2],   18(%[p_ptr])                   \n\t"
               "sh             %[temp3],   20(%[p_ptr])                   \n\t"
               "sh             %[temp4],   22(%[p_ptr])                   \n\t"
               "sh             %[temp5],   24(%[p_ptr])                   \n\t"
               "sh             %[temp6],   26(%[p_ptr])                   \n\t"
               "sh             %[temp7],   28(%[p_ptr])                   \n\t"
               "sh             %[temp8],   30(%[p_ptr])                   \n\t"
               "lw             %[temp1],   64(%[p_y32])                   \n\t"
               "lw             %[temp2],   68(%[p_y32])                   \n\t"
               "lw             %[temp3],   72(%[p_y32])                   \n\t"
               "lw             %[temp4],   76(%[p_y32])                   \n\t"
               "sllv           %[temp1],   %[temp1],         %[j]         \n\t"
               "sllv           %[temp2],   %[temp2],         %[j]         \n\t"
               "sllv           %[temp3],   %[temp3],         %[j]         \n\t"
               "sllv           %[temp4],   %[temp4],         %[j]         \n\t"
               "addu           %[temp1],   %[temp1],         %[temp_c]    \n\t"
               "addu           %[temp2],   %[temp2],         %[temp_c]    \n\t"
               "addu           %[temp3],   %[temp3],         %[temp_c]    \n\t"
               "addu           %[temp4],   %[temp4],         %[temp_c]    \n\t"
               "sra            %[temp1],   %[temp1],         16           \n\t"
               "sra            %[temp2],   %[temp2],         16           \n\t"
               "sra            %[temp3],   %[temp3],         16           \n\t"
               "sra            %[temp4],   %[temp4],         16           \n\t"
               "sh             %[temp1],   32(%[p_ptr])                   \n\t"
               "sh             %[temp2],   34(%[p_ptr])                   \n\t"
               "sh             %[temp3],   36(%[p_ptr])                   \n\t"
               "sh             %[temp4],   38(%[p_ptr])                   \n\t"

               : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                 [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
                 [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
                 [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
                 [p_ptr] "+r" (p_ptr), [p_y32] "+r" (p_y32)
               : [j] "r" (j), [temp_c] "r" (temp_c)
               : "memory"
            );

            p_y32+=20;
            p_ptr+=20;
        }
#endif
    }
    else{
        for (i = L_CODE >> 1; i != 0; i--)
        {
            s = L_shl(*(p_y32++), j, pOverflow);
            *(p_ptr++) = (s + 0x00008000) >> 16;
            s = L_shl(*(p_y32++), j, pOverflow);
            *(p_ptr++) = (s + 0x00008000) >> 16;
        }
    }
    return;


}
