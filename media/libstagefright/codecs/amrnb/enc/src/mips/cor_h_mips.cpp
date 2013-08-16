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



 Pathname: ./audio/gsm-amr/c/src/cor_h.c

     Date: 06/12/2000

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Updated template used to PV coding template. First attempt at
          optimizing C code.

 Description: Used MAX_16 and MIN_16 when checking the result of Inv_sqrt.
          Synced up to the new template.

 Description: Added setting of Overflow flag in inlined code.

 Description: Took out cor_h_x function and put it in its own file. Sync'ed
          up with the single_func_template.c template. Delete version
          ID variable.

 Description: Synchronized file with UTMS version 3.2.0. Updated coding
              template. Removed unnecessary include files.

 Description: Fixed portion of the code that builds the rr[] matrix. There
              was an error in the original inlining of code that caused
              the code to be not bit-exact with UMTS version 3.2.0.

 Description: Added calls to L_add() and mult() in the code to handle overflow
              scenario. Moved cor_h.h after cnst.h in the Include section.
              Doing this allows the unit test to build using the cnst.h in the
              /test/include directory. Fixed initialization of the accumulator
              in the first calculation of the sum of squares.

 Description: Made the following changes per comments from Phase 2/3 review:
              1. Used #define value instead of hard-coded numbers in the code.
              2. Fixed typecasting issue with TI C compiler.
              3. Removed typecasting of 0x00008000L in the call to L_add.

 Description: Changed pOverflow from a global variable into a function
 parameter.

 Description:
            1. Added pointer to avoid adding offsets in every pass
            2. Eliminate variables defined as registers
            3. Removed extra check for overflow by doing scaling right
               after overflow is detected.
            4. Eliminated calls to basic operations (like extract) not
               needed because of the nature of the number (all bounded)
            5. Eliminated duplicate loop accessing same data
            6. Simplified matrix addressing by use of pointers

 Description:
              1. Eliminated unused include files.
              2. Access twice the number of points when delaing with matrices
                 and in the process only 3 pointers (instead of 4) are needed
              3. Replaced array addressing (array sign[]) by pointers

 Description: Changed round function name to pv_round to avoid conflict with
              round function in C standard library.

 Description: Using inlines from fxp_arithmetic.h .

 Description: Replacing fxp_arithmetic.h with basic_op.h.

 Description:

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/
#include "cnst.h"
#include "cor_h.h"
#include "basicop_malloc.h"
#include "inv_sqrt.h"
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

/*
------------------------------------------------------------------------------
 FUNCTION NAME: cor_h
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    h = vector containing the impulse response of the weighted synthesis
        filter; vector contents are of type Word16; vector length is
        2 * L_SUBFR
    sign = vector containing the sign information for the correlation
           values; vector contents are of type Word16; vector length is
           L_CODE
    rr = autocorrelation matrix; matrix contents are of type Word16;
         matrix dimension is L_CODE by L_CODE

 Outputs:
    rr contents are the newly calculated autocorrelation values

 Returns:
    None

 Global Variables Used:
    None

 Local Variables Needed:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

 This function computes correlations of the impulse response (h) needed for
 the codebook search, and includes the sign information into the correlations.

 The correlations are given by:
    rr[i][j] = sum_{n=i}^{L-1} h[n-i] h[n-j];   i>=j; i,j=0,...,L-1

 The sign information is included by:
    rr[i][j] = rr[i][j]*sign[i]*sign[j]

------------------------------------------------------------------------------
 REQUIREMENTS

 None

------------------------------------------------------------------------------
 REFERENCES

 cor_h.c, UMTS GSM AMR speech codec, R99 - Version 3.2.0, March 2, 2001

------------------------------------------------------------------------------
 PSEUDO-CODE

void cor_h (
    Word16 h[],         // (i) : impulse response of weighted synthesis
                                 filter
    Word16 sign[],      // (i) : sign of d[n]
    Word16 rr[][L_CODE] // (o) : matrix of autocorrelation
)
{
    Word16 i, j, k, dec, h2[L_CODE];
    Word32 s;

    // Scaling for maximum precision

    s = 2;
    for (i = 0; i < L_CODE; i++)
        s = L_mac (s, h[i], h[i]);

    j = sub (extract_h (s), 32767);
    if (j == 0)
    {
        for (i = 0; i < L_CODE; i++)
        {
            h2[i] = shr (h[i], 1);
        }
    }
    else
    {
        s = L_shr (s, 1);
        k = extract_h (L_shl (Inv_sqrt (s), 7));
        k = mult (k, 32440);                     // k = 0.99*k

        for (i = 0; i < L_CODE; i++)
        {
            h2[i] = pv_round (L_shl (L_mult (h[i], k), 9));
        }
    }

    // build matrix rr[]
    s = 0;
    i = L_CODE - 1;
    for (k = 0; k < L_CODE; k++, i--)
    {
        s = L_mac (s, h2[k], h2[k]);
        rr[i][i] = pv_round (s);
    }

    for (dec = 1; dec < L_CODE; dec++)
    {
        s = 0;
        j = L_CODE - 1;
        i = sub (j, dec);
        for (k = 0; k < (L_CODE - dec); k++, i--, j--)
        {
            s = L_mac (s, h2[k], h2[k + dec]);
            rr[j][i] = mult (pv_round (s), mult (sign[i], sign[j]));
            rr[i][j] = rr[j][i];
        }
    }
}

---------------------------------------------------------------------------
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

void cor_h(
    Word16 h[],          /* (i) : impulse response of weighted synthesis
                                  filter                                  */
    Word16 sign[],       /* (i) : sign of d[n]                            */
    Word16 rr[][L_CODE], /* (o) : matrix of autocorrelation               */
    Flag  *pOverflow
)
{
    register Word16 i;
    register Word16 dec;

    Word16 h2[L_CODE];
    Word32 s;
    Word32 s2;
    Word16 tmp1;
    Word16 tmp2;
    Word16 tmp11;
    Word16 tmp22;

    Word16 *p_h;
    Word16 *p_h2;
    Word16 *rr1;
    Word16 *rr2;
    Word16 *rr3;
    Word16 *p_rr_ref1;
    Word16 *p_sign1;
    Word16 *p_sign2;

    /* Scaling for maximum precision */

    /* Initialize accumulator to 1 since left shift happens    */
    /* after the accumulation of the sum of squares (original  */
    /* code initialized s to 2)                                */
    p_h = h;

#if (MIPS_DSP_R2_LE)
    {
        Word32 temp1, temp2, temp3, temp4;
        __asm__ volatile (
            ".set push                                                  \n\t"
            ".set noreorder                                             \n\t"
            "li             %[temp1],   1                               \n\t"
            "li             %[i],       20                              \n\t"
            "mthi           $zero,      $ac1                            \n\t"
            "mtlo           %[temp1],   $ac1                            \n\t"
         "1:                                                            \n\t"
            "ulw            %[temp1],   0(%[p_h])                       \n\t"
            "ulw            %[temp2],   4(%[p_h])                       \n\t"
            "ulw            %[temp3],   8(%[p_h])                       \n\t"
            "ulw            %[temp4],   12(%[p_h])                      \n\t"
            "addiu          %[p_h],     %[p_h],         16              \n\t"
            "dpa.w.ph       $ac1,       %[temp1],       %[temp1]        \n\t"
            "dpa.w.ph       $ac1,       %[temp2],       %[temp2]        \n\t"
            "dpa.w.ph       $ac1,       %[temp3],       %[temp3]        \n\t"
            "addiu          %[i],       %[i],           -4              \n\t"
            "bnez           %[i],       1b                              \n\t"
            " dpa.w.ph      $ac1,       %[temp4],       %[temp4]        \n\t"
            "mflo           %[s],       $ac1                            \n\t"
            ".set pop                                                   \n\t"
            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [s] "=&r" (s),[i] "=&r" (i),[p_h] "+r" (p_h)
            :
            :"memory",  "$ac1hi", "$ac1lo"
        );
    }
#elif (MIPS_DSP_R1_LE) || (MIPS32_R2_LE)
    {
        Word32 temp1, temp2, temp3, temp4;
        __asm__ volatile (
            ".set push                                                  \n\t"
            ".set noreorder                                             \n\t"
            "li             %[temp1],   1                               \n\t"
            "li             %[i],       20                              \n\t"
            "mthi           $zero                                       \n\t"
            "mtlo           %[temp1]                                    \n\t"
         "1:                                                            \n\t"
            "lh             %[temp1],   0(%[p_h])                       \n\t"
            "lh             %[temp2],   2(%[p_h])                       \n\t"
            "lh             %[temp3],   4(%[p_h])                       \n\t"
            "lh             %[temp4],   6(%[p_h])                       \n\t"
            "madd           %[temp1],   %[temp1]                        \n\t"
            "madd           %[temp2],   %[temp2]                        \n\t"
            "madd           %[temp3],   %[temp3]                        \n\t"
            "madd           %[temp4],   %[temp4]                        \n\t"
            "lh             %[temp1],   8(%[p_h])                       \n\t"
            "lh             %[temp2],   10(%[p_h])                      \n\t"
            "lh             %[temp3],   12(%[p_h])                      \n\t"
            "lh             %[temp4],   14(%[p_h])                      \n\t"
            "madd           %[temp1],   %[temp1]                        \n\t"
            "madd           %[temp2],   %[temp2]                        \n\t"
            "madd           %[temp3],   %[temp3]                        \n\t"
            "madd           %[temp4],   %[temp4]                        \n\t"
            "addiu          %[i],       %[i],        -4                 \n\t"
            "bnez           %[i],       1b                              \n\t"
            " addiu         %[p_h],     %[p_h],      16                 \n\t"
            "mflo           %[s]                                        \n\t"
            ".set pop                                                   \n\t"

            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [s] "=&r" (s),[i] "=&r" (i),[p_h] "+r" (p_h)
            :
            :"memory",  "hi", "lo"
        );
    }
#endif

    s <<= 1;

    if (s & MIN_32)
    {
        p_h2 = h2;
        p_h  = h;

        for (i = (L_CODE >> 1); i != 0; i--)
        {
            *(p_h2++) =  *(p_h++)  >> 1;
            *(p_h2++) =  *(p_h++)  >> 1;
        }
    }
    else
    {

        s >>= 1;

        s = Inv_sqrt(s, pOverflow);

        if (s < (Word32) 0x00ffffffL)
        {
            /* k = 0.99*k */
            dec = (Word16)(((s >> 9) * 32440) >> 15);
        }
        else
        {
            dec = 32440;  /* 0.99 */
        }

        p_h  = h;
        p_h2 = h2;

#if (MIPS_DSP_R2_LE) || (MIPS_DSP_R1_LE)
        {
             Word32 temp1, temp2, temp3, temp4;
             Word32 temp5, temp6, temp7, temp8;
            __asm__ volatile (
                ".set push                                                  \n\t"
                ".set noreorder                                             \n\t"
                "li             %[i],       20                              \n\t"
                "lh             %[temp1],   0(%[p_h])                       \n\t"
                "lh             %[temp2],   2(%[p_h])                       \n\t"
                "lh             %[temp3],   4(%[p_h])                       \n\t"
                "lh             %[temp4],   6(%[p_h])                       \n\t"
             "1:                                                            \n\t"
                "mult           $ac0,       %[temp1],    %[dec]             \n\t"
                "mult           $ac1,       %[temp2],    %[dec]             \n\t"
                "mult           $ac2,       %[temp3],    %[dec]             \n\t"
                "mult           $ac3,       %[temp4],    %[dec]             \n\t"
                "addiu          %[i],       %[i],        -2                 \n\t"
                "extr_r.w       %[temp5],   $ac0,        6                  \n\t"
                "extr_r.w       %[temp6],   $ac1,        6                  \n\t"
                "extr_r.w       %[temp7],   $ac2,        6                  \n\t"
                "extr_r.w       %[temp8],   $ac3,        6                  \n\t"
                "lh             %[temp1],   8(%[p_h])                       \n\t"
                "lh             %[temp2],   10(%[p_h])                      \n\t"
                "lh             %[temp3],   12(%[p_h])                      \n\t"
                "lh             %[temp4],   14(%[p_h])                      \n\t"
                "addiu          %[p_h],     %[p_h],      8                  \n\t"
                "sh             %[temp5],   0(%[p_h2])                      \n\t"
                "sh             %[temp6],   2(%[p_h2])                      \n\t"
                "sh             %[temp7],   4(%[p_h2])                      \n\t"
                "sh             %[temp8],   6(%[p_h2])                      \n\t"
                "bnez           %[i],       1b                              \n\t"
                " addiu         %[p_h2],    %[p_h2],     8                  \n\t"
                ".set pop                                                   \n\t"

                : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                  [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
                  [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
                  [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
                  [p_h2] "+r" (p_h2),[p_h] "+r" (p_h),
                  [i] "=&r" (i)
                : [dec] "r" (dec)
                : "memory", "$ac3hi", "$ac3lo", "$ac1hi", "$ac1lo",
                  "$ac2hi", "$ac2lo", "hi", "lo"
            );
        }
#elif (MIPS32_R2_LE)
        {
             Word32 temp1, temp2, temp3, temp4;
             Word32 temp5, temp6, temp7, temp8;
            __asm__ volatile (
                ".set push                                                  \n\t"
                ".set noreorder                                             \n\t"
                "li             %[i],       20                              \n\t"
                "lh             %[temp1],   0(%[p_h])                       \n\t"
                "lh             %[temp2],   2(%[p_h])                       \n\t"
                "lh             %[temp3],   4(%[p_h])                       \n\t"
                "lh             %[temp4],   6(%[p_h])                       \n\t"
             "1:                                                            \n\t"
                "mul            %[temp5],   %[temp1],    %[dec]             \n\t"
                "mul            %[temp6],   %[temp2],    %[dec]             \n\t"
                "mul            %[temp7],   %[temp3],    %[dec]             \n\t"
                "mul            %[temp8],   %[temp4],    %[dec]             \n\t"
                "addiu          %[i],       %[i],        -2                 \n\t"
                "lh             %[temp1],   8(%[p_h])                       \n\t"
                "lh             %[temp2],   10(%[p_h])                      \n\t"
                "lh             %[temp3],   12(%[p_h])                      \n\t"
                "lh             %[temp4],   14(%[p_h])                      \n\t"
                "addiu          %[temp5],   %[temp5],    0x20               \n\t"
                "addiu          %[temp6],   %[temp6],    0x20               \n\t"
                "addiu          %[temp7],   %[temp7],    0x20               \n\t"
                "addiu          %[temp8],   %[temp8],    0x20               \n\t"
                "sra            %[temp5],   %[temp5],    6                  \n\t"
                "sra            %[temp6],   %[temp6],    6                  \n\t"
                "sra            %[temp7],   %[temp7],    6                  \n\t"
                "sra            %[temp8],   %[temp8],    6                  \n\t"
                "addiu          %[p_h],     %[p_h],      8                  \n\t"
                "sh             %[temp5],   0(%[p_h2])                      \n\t"
                "sh             %[temp6],   2(%[p_h2])                      \n\t"
                "sh             %[temp7],   4(%[p_h2])                      \n\t"
                "sh             %[temp8],   6(%[p_h2])                      \n\t"
                "bnez           %[i],       1b                              \n\t"
                " addiu         %[p_h2],    %[p_h2],     8                  \n\t"
                ".set pop                                                   \n\t"

                : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                  [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
                  [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
                  [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
                  [p_h2] "+r" (p_h2),[p_h] "+r" (p_h),
                  [i] "=&r" (i)
                : [dec] "r" (dec)
                : "memory", "hi", "lo"
            );
        }
#endif
    }
    /* build matrix rr[] */
    s = 0;

    p_h2 = h2;

    rr1 = &rr[L_CODE-1][L_CODE-1];

    for (i = 20; i != 0 ; i-=2)
    {
        tmp1   = *(p_h2++);
        s = amrnb_fxp_mac_16_by_16bb((Word32) tmp1, (Word32) tmp1, s);
        *rr1 = (Word16)((s + 0x00004000L) >> 15);
        rr1 -= (L_CODE + 1);
        tmp1   = *(p_h2++);
        s = amrnb_fxp_mac_16_by_16bb((Word32) tmp1, (Word32) tmp1, s);
        *rr1 = (Word16)((s + 0x00004000L) >> 15);
        rr1 -= (L_CODE + 1);

        tmp1   = *(p_h2++);
        s = amrnb_fxp_mac_16_by_16bb((Word32) tmp1, (Word32) tmp1, s);
        *rr1 = (Word16)((s + 0x00004000L) >> 15);
        rr1 -= (L_CODE + 1);
        tmp1   = *(p_h2++);
        s = amrnb_fxp_mac_16_by_16bb((Word32) tmp1, (Word32) tmp1, s);
        *rr1 = (Word16)((s + 0x00004000L) >> 15);
        rr1 -= (L_CODE + 1);
    }


    p_rr_ref1 = rr[L_CODE-1];

#if (MIPS_DSP_R2_LE) || (MIPS_DSP_R1_LE)
    for (dec = 1; dec < L_CODE; dec += 2)
    {
        Word32 temp1, temp2, temp3, temp4, temp5, temp6;
        Word32 temp7, temp8, temp9, temp10,temp_b;

        __asm__ volatile (
            ".set push                                                  \n\t"
            ".set noreorder                                             \n\t"
            "li             %[temp2],   39                              \n\t"
            "li             %[temp10],  40                              \n\t"
            "sll            %[temp1],   %[dec],         1               \n\t"
            "subu           %[i],       %[temp2],       %[dec]          \n\t"//(39-dec)
            "move           %[p_h2],    %[h2]                           \n\t"
            "sll            %[temp3],   %[i],           1               \n\t"//(39-dec)<<1
            "mul            %[temp9],   %[temp3],       %[temp10]       \n\t"
            "addu           %[p_h],     %[h2],          %[temp1]        \n\t"
            "addu           %[rr1],     %[p_rr_ref1],   %[temp3]        \n\t"
            "addiu          %[p_sign1], %[sign],        78              \n\t"
            "mult           $ac3,       $zero,          $zero           \n\t"
            "mult           $ac1,       $zero,          $zero           \n\t"
            "subu           %[p_sign2], %[p_sign1],     %[temp1]        \n\t"
            "addu           %[temp7],   %[rr],          %[temp9]        \n\t"
            "beqz           %[i],       2f                              \n\t"
            " addiu         %[rr2],     %[temp7],       78              \n\t"
            //for (i = (39 - dec); i != 0 ; i-=2)
         "1:                                                            \n\t"
            "lh             %[temp4],   0(%[p_sign1])                   \n\t"
            "lh             %[temp6],   0(%[p_sign2])                   \n\t"
            "lh             %[temp5],   -2(%[p_sign2])                  \n\t"
            "lh             %[temp1],   0(%[p_h2])                      \n\t"
            "lh             %[temp3],   0(%[p_h])                       \n\t"
            "lh             %[temp2],   2(%[p_h])                       \n\t"
            "mul            %[temp9],   %[temp4],       %[temp6]        \n\t"
            "mul            %[temp10],  %[temp4],       %[temp5]        \n\t"
            "madd           $ac3,       %[temp1],       %[temp3]        \n\t"
            "madd           $ac1,       %[temp1],       %[temp2]        \n\t"
            "lh             %[temp1],   2(%[p_h2])                      \n\t"
            "lh             %[temp3],   4(%[p_h])                       \n\t"
            "extr_r.w       %[temp7],   $ac3,           15              \n\t"
            "extr_r.w       %[temp8],   $ac1,           15              \n\t"
            "lh             %[temp4],   -2(%[p_sign1])                  \n\t"
            "lh             %[temp6],   -4(%[p_sign2])                  \n\t"
            "madd           $ac3,       %[temp1],       %[temp2]        \n\t"
            "madd           $ac1,       %[temp1],       %[temp3]        \n\t"
            "sra            %[temp9],   %[temp9],       15              \n\t"
            "sra            %[temp10],  %[temp10],      15              \n\t"
            "mul            %[temp7],   %[temp7],       %[temp9]        \n\t"
            "mul            %[temp8],   %[temp8],       %[temp10]       \n\t"
            "mul            %[temp9],   %[temp4],       %[temp5]        \n\t"
            "mul            %[temp10],  %[temp4],       %[temp6]        \n\t"
            "extr_r.w       %[temp1],   $ac3,           15              \n\t"
            "extr_r.w       %[temp2],   $ac1,           15              \n\t"
            "addiu          %[p_h],     %[p_h],         4               \n\t"
            "sra            %[temp7],   %[temp7],       15              \n\t"
            "sra            %[temp8],   %[temp8],       15              \n\t"
            "sra            %[temp9],   %[temp9],       15              \n\t"
            "sra            %[temp10],  %[temp10],      15              \n\t"
            "sh             %[temp7],   0(%[rr2])                       \n\t"
            "mul            %[temp1],   %[temp1],       %[temp9]        \n\t"
            "mul            %[temp2],   %[temp2],       %[temp10]       \n\t"
            "sh             %[temp7],   0(%[rr1])                       \n\t"
            "sh             %[temp8],   -2(%[rr1])                      \n\t"
            "sh             %[temp8],   -80(%[rr2])                     \n\t"
            "addiu          %[p_sign1], %[p_sign1],     -4              \n\t"
            "addiu          %[p_sign2], %[p_sign2],     -4              \n\t"
            "addiu          %[rr1],     %[rr1],         -164            \n\t"
            "addiu          %[rr2],     %[rr2],         -164            \n\t"
            "sra            %[temp1],   %[temp1],       15              \n\t"
            "sra            %[temp2],   %[temp2],       15              \n\t"
            "addiu          %[i],       %[i],           -2              \n\t"
            "sh             %[temp1],   82(%[rr2])                      \n\t"
            "sh             %[temp1],   82(%[rr1])                      \n\t"
            "sh             %[temp2],   80(%[rr1])                      \n\t"
            "sh             %[temp2],   2(%[rr2])                       \n\t"
            "bnez           %[i],       1b                              \n\t"
            " addiu         %[p_h2],    %[p_h2],        4               \n\t"
         "2:                                                            \n\t"
            "lh             %[temp4],   0(%[p_sign1])                   \n\t"
            "lh             %[temp6],   0(%[p_sign2])                   \n\t"
            "lh             %[temp1],   0(%[p_h2])                      \n\t"
            "lh             %[temp3],   0(%[p_h])                       \n\t"
            "mul            %[temp4],   %[temp4],       %[temp6]        \n\t"
            "madd           $ac3,       %[temp1],       %[temp3]        \n\t"
            "extr_r.w       %[temp1],   $ac3,           15              \n\t"
            "sra            %[temp4],   %[temp4],       15              \n\t"
            "mul            %[temp4],   %[temp4],       %[temp1]        \n\t"
            "addiu          %[rr1],     %[rr1],         -82             \n\t"
            "addiu          %[rr2],     %[rr2],         -82             \n\t"
            "sra            %[temp4],   %[temp4],       15              \n\t"
            "sh             %[temp4],   82(%[rr1])                      \n\t"
            "sh             %[temp4],   82(%[rr2])                      \n\t"
            ".set pop                                                   \n\t"

            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
              [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
              [temp9] "=&r" (temp9), [temp10] "=&r" (temp10),
              [p_h2] "=&r" (p_h2), [p_h] "=&r" (p_h),
              [i] "=&r" (i),
              [p_sign1] "=&r" (p_sign1), [p_sign2] "=&r" (p_sign2),
              [rr1] "=&r" (rr1), [rr2] "=&r" (rr2)
            : [dec] "r" (dec), [h2] "r" (h2), [sign] "r" (sign),
              [p_rr_ref1] "r" (p_rr_ref1), [rr] "r" (rr)
            : "hi", "lo", "memory", "$ac3hi", "$ac3lo", "$ac1hi", "$ac1lo"

        );
    }
#elif (MIPS32_R2_LE)
    for (dec = 1; dec < L_CODE; dec += 2)
    {
        Word32 temp1, temp2, temp3, temp4, temp5, temp6;
        Word32 temp7, temp8, temp9, temp10;
        Word32 temp_ac1, temp_ac3;

        __asm__ volatile (
            ".set push                                                   \n\t"
            ".set noreorder                                              \n\t"
            "li             %[temp2],    39                              \n\t"
            "li             %[temp10],   40                              \n\t"
            "sll            %[temp1],    %[dec],         1               \n\t"
            "subu           %[i],        %[temp2],       %[dec]          \n\t"//(39-dec)
            "move           %[p_h2],     %[h2]                           \n\t"
            "sll            %[temp3],    %[i],           1               \n\t"//(39-dec)<<1
            "mul            %[temp9],    %[temp3],       %[temp10]       \n\t"
            "addu           %[p_h],      %[h2],          %[temp1]        \n\t"
            "addu           %[rr1],      %[p_rr_ref1],   %[temp3]        \n\t"
            "addiu          %[p_sign1],  %[sign],        78              \n\t"
            "move           %[temp_ac1], $zero                           \n\t"
            "move           %[temp_ac3], $zero                           \n\t"
            "subu           %[p_sign2],  %[p_sign1],     %[temp1]        \n\t"
            "addu           %[temp7],    %[rr],          %[temp9]        \n\t"
            "beqz           %[i],        2f                              \n\t"
            " addiu         %[rr2],      %[temp7],       78              \n\t"
            //for (i = (39 - dec); i != 0 ;  i-=2)
         "1:                                                             \n\t"
            "lh             %[temp4],    0(%[p_sign1])                   \n\t"
            "lh             %[temp6],    0(%[p_sign2])                   \n\t"
            "lh             %[temp5],    -2(%[p_sign2])                  \n\t"
            "lh             %[temp1],    0(%[p_h2])                      \n\t"
            "lh             %[temp3],    0(%[p_h])                       \n\t"
            "lh             %[temp2],    2(%[p_h])                       \n\t"
            "mul            %[temp9],    %[temp4],       %[temp6]        \n\t"
            "mul            %[temp10],   %[temp4],       %[temp5]        \n\t"
            "mul            %[temp6],    %[temp1],       %[temp3]        \n\t"
            "mul            %[temp4],    %[temp1],       %[temp2]        \n\t"
            "lh             %[temp1],    2(%[p_h2])                      \n\t"
            "lh             %[temp3],    4(%[p_h])                       \n\t"
            "sra            %[temp9],    %[temp9],       15              \n\t"
            "sra            %[temp10],   %[temp10],      15              \n\t"
            "addu           %[temp_ac3], %[temp_ac3],    %[temp6]        \n\t"
            "addu           %[temp_ac1], %[temp_ac1],    %[temp4]        \n\t"
            "addiu          %[temp7],    %[temp_ac3],    0x4000          \n\t"
            "addiu          %[temp8],    %[temp_ac1],    0x4000          \n\t"
            "sra            %[temp7],    %[temp7],       15              \n\t"
            "sra            %[temp8],    %[temp8],       15              \n\t"
            "lh             %[temp4],    -2(%[p_sign1])                  \n\t"
            "lh             %[temp6],    -4(%[p_sign2])                  \n\t"
            "mul            %[temp2],    %[temp1],       %[temp2]        \n\t"
            "mul            %[temp1],    %[temp1],       %[temp3]        \n\t"
            "mul            %[temp7],    %[temp7],       %[temp9]        \n\t"
            "mul            %[temp8],    %[temp8],       %[temp10]       \n\t"
            "mul            %[temp9],    %[temp4],       %[temp5]        \n\t"
            "mul            %[temp10],   %[temp4],       %[temp6]        \n\t"
            "addu           %[temp_ac3], %[temp_ac3],    %[temp2]        \n\t"
            "addu           %[temp_ac1], %[temp_ac1],    %[temp1]        \n\t"
            "addiu          %[temp1],    %[temp_ac3],    0x4000          \n\t"
            "addiu          %[temp2],    %[temp_ac1],    0x4000          \n\t"
            "sra            %[temp1],    %[temp1],       15              \n\t"
            "sra            %[temp2],    %[temp2],       15              \n\t"
            "addiu          %[p_h],      %[p_h],         4               \n\t"
            "sra            %[temp7],    %[temp7],       15              \n\t"
            "sra            %[temp8],    %[temp8],       15              \n\t"
            "sra            %[temp9],    %[temp9],       15              \n\t"
            "sra            %[temp10],   %[temp10],      15              \n\t"
            "sh             %[temp7],    0(%[rr2])                       \n\t"
            "mul            %[temp1],    %[temp1],       %[temp9]        \n\t"
            "mul            %[temp2],    %[temp2],       %[temp10]       \n\t"
            "sh             %[temp7],    0(%[rr1])                       \n\t"
            "sh             %[temp8],    -2(%[rr1])                      \n\t"
            "sh             %[temp8],    -80(%[rr2])                     \n\t"
            "addiu          %[p_h2],     %[p_h2],        4               \n\t"
            "addiu          %[p_sign1],  %[p_sign1],     -4              \n\t"
            "addiu          %[p_sign2],  %[p_sign2],     -4              \n\t"
            "addiu          %[rr1],      %[rr1],         -164            \n\t"
            "addiu          %[rr2],      %[rr2],         -164            \n\t"
            "sra            %[temp1],    %[temp1],       15              \n\t"
            "sra            %[temp2],    %[temp2],       15              \n\t"
            "addiu          %[i],        %[i],           -2              \n\t"
            "sh             %[temp1],    82(%[rr2])                      \n\t"
            "sh             %[temp1],    82(%[rr1])                      \n\t"
            "sh             %[temp2],    80(%[rr1])                      \n\t"
            "bnez           %[i],        1b                              \n\t"
            " sh            %[temp2],    2(%[rr2])                       \n\t"
         "2:                                                             \n\t"
            "lh             %[temp4],    0(%[p_sign1])                   \n\t"
            "lh             %[temp6],    0(%[p_sign2])                   \n\t"
            "lh             %[temp1],    0(%[p_h2])                      \n\t"
            "lh             %[temp3],    0(%[p_h])                       \n\t"
            "mul            %[temp4],    %[temp4],       %[temp6]        \n\t"
            "mul            %[temp1],    %[temp1],       %[temp3]        \n\t"
            "addiu          %[rr1],      %[rr1],         -82             \n\t"
            "addiu          %[rr2],      %[rr2],         -82             \n\t"
            "sra            %[temp4],    %[temp4],       15              \n\t"
            "addu           %[temp_ac3], %[temp_ac3],    %[temp1]        \n\t"
            "addiu          %[temp1],    %[temp_ac3],    0x4000          \n\t"
            "sra            %[temp1],    %[temp1],       15              \n\t"
            "mul            %[temp4],    %[temp4],       %[temp1]        \n\t"
            "sra            %[temp4],    %[temp4],       15              \n\t"
            "sh             %[temp4],    82(%[rr1])                      \n\t"
            "sh             %[temp4],    82(%[rr2])                      \n\t"
            ".set pop                                                    \n\t"

            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
              [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
              [temp9] "=&r" (temp9), [temp10] "=&r" (temp10),
              [p_h2] "=&r" (p_h2), [p_h] "=&r" (p_h),
              [i] "=&r" (i),
              [temp_ac1] "=&r" (temp_ac1), [temp_ac3] "=&r" (temp_ac3),
              [p_sign1] "=&r" (p_sign1), [p_sign2] "=&r" (p_sign2),
              [rr1] "=&r" (rr1), [rr2] "=&r" (rr2)
            : [dec] "r" (dec), [h2] "r" (h2), [sign] "r" (sign),
              [p_rr_ref1] "r" (p_rr_ref1), [rr] "r" (rr)
            : "hi", "lo", "memory"
        );
    }
#endif
    return;
}