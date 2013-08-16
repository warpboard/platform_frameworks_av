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
 Pathname: ./audio/gsm-amr/c/src/pred_lt.c

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Updated template used to PV coding template. First attempt at
          optimizing C code.

 Description: Deleted variables listed in the Local Stores Needed/Modified
          sections.

 Description: Updated file per comments from Phase 2/3 review.

 Description: Synchronized file with UMTS version 3.2.0. Updated coding
              template. Removed unnecessary include files.

 Description: Fixed typecasting issue with TI C compiler. Updated copyright
              year.

 Description:
 (1) Removed instance of static in the const table "inter_6"
 (2) Changed Overflow from a global to a parameter passed via a pointer.
 (3) Made numerous small changes to bring code more in line with PV standards.

 Description:  For pred_ltp()
              1. Eliminated unused include files.
              2. Replaced array addressing by pointers
              3. Eliminated math operations that unnecessary checked for
                 saturation
              4. Unrolled loops to speed up processing, use decrement loops,
                 loaded into memory filter coefficient in linear order for
                 faster execution in main loop.
              5. Eliminated call to round by proper initialization

 Description:  Replaced "int" and/or "char" with defined types.
               Added proper casting (Word32) to some left shifting operations


 Description: Changed round function name to pv_round to avoid conflict with
              round function in C standard library.

 Who:                           Date:
 Description:

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/
#include "pred_lt.h"
#include "cnst.h"

/*----------------------------------------------------------------------------
; MACROS
; Define module specific macros here
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; DEFINES
; Include all pre-processor statements here. Include conditional
; compile variables also.
----------------------------------------------------------------------------*/
#define UP_SAMP_MAX  6
#define L_INTER10    (L_INTERPOL-1)
#define FIR_SIZE     (UP_SAMP_MAX*L_INTER10+1)

/*----------------------------------------------------------------------------
; LOCAL FUNCTION DEFINITIONS
; Function Prototype declaration
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; LOCAL STORE/BUFFER/POINTER DEFINITIONS
; Variable declaration - defined here and used outside this module
----------------------------------------------------------------------------*/

/* 1/6 resolution interpolation filter  (-3 dB at 3600 Hz) */
/* Note: the 1/3 resolution filter is simply a subsampled
 *       version of the 1/6 resolution filter, i.e. it uses
 *       every second coefficient:
 *
 *          inter_3l[k] = inter_6[2*k], 0 <= k <= 3*L_INTER10
 */

const Word16 inter_6_pred_lt[FIR_SIZE] =
{
    29443,
    28346, 25207, 20449, 14701,  8693,  3143,
    -1352, -4402, -5865, -5850, -4673, -2783,
    -672,  1211,  2536,  3130,  2991,  2259,
    1170,     0, -1001, -1652, -1868, -1666,
    -1147,  -464,   218,   756,  1060,  1099,
    904,   550,   135,  -245,  -514,  -634,
    -602,  -451,  -231,     0,   191,   308,
    340,   296,   198,    78,   -36,  -120,
    -163,  -165,  -132,   -79,   -19,    34,
    73,    91,    89,    70,    38,     0
};


/*
------------------------------------------------------------------------------
 FUNCTION NAME: Pred_lt_3or6
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    exc = buffer containing the excitation (Word16)
    T0 = integer pitch lag (Word16)
    frac = fraction of lag (Word16)
    L_subfr = number of samples per subframe (Word16)
    flag3 = flag to indicate the upsampling rate; if set, upsampling
            rate is 3, otherwise, upsampling rate is 6 (Word16)

    pOverflow = pointer to overflow (Flag)

 Returns:
    None

 Outputs:
    exc buffer contains the newly formed adaptive codebook excitation
    pOverflow -> 1 if the add operation resulted in overflow

 Global Variables Used:
    inter_6_pred_lt = (1/6) resolution interpolation filter table (Word16)

 Local Variables Needed:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

 This function computes the result of long term prediction with fractional
 interpolation of resolution 1/3 or 1/6. (Interpolated past excitation).

 The past excitation signal at the given delay is interpolated at
 the given fraction to build the adaptive codebook excitation.
 On return exc[0..L_subfr-1] contains the interpolated signal
 (adaptive codebook excitation).

------------------------------------------------------------------------------
 REQUIREMENTS

 None

------------------------------------------------------------------------------
 REFERENCES

 pred_lt.c, UMTS GSM AMR speech codec, R99 - Version 3.2.0, March 2, 2001

------------------------------------------------------------------------------
 PSEUDO-CODE

void Pred_lt_3or6 (
    Word16 exc[],     // in/out: excitation buffer
    Word16 T0,        // input : integer pitch lag
    Word16 frac,      // input : fraction of lag
    Word16 L_subfr,   // input : subframe size
    Word16 flag3      // input : if set, upsampling rate = 3 (6 otherwise)
)
{
    Word16 i, j, k;
    Word16 *pX0, *pX1, *pX2;
    const Word16 *pC1, *pC2;
    Word32 s;

    pX0 = &exc[-T0];

    frac = negate (frac);
    if (flag3 != 0)
    {
      frac = shl (frac, 1);   // inter_3l[k] = inter_6[2*k] -> k' = 2*k
    }

    if (frac < 0)
    {
        frac = add (frac, UP_SAMP_MAX);
        pX0--;
    }

    for (j = 0; j < L_subfr; j++)
    {
        pX1 = pX0++;
        pX2 = pX0;
        pC1 = &inter_6[frac];
        pC2 = &inter_6[sub (UP_SAMP_MAX, frac)];

        s = 0;
        for (i = 0, k = 0; i < L_INTER10; i++, k += UP_SAMP_MAX)
        {
            s = L_mac (s, pX1[-i], pC1[k]);
            s = L_mac (s, pX2[i], pC2[k]);
        }

        exc[j] = pv_round (s);
    }

    return;
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

void Pred_lt_3or6(
    Word16 exc[],     /* in/out: excitation buffer                          */
    Word16 T0,        /* input : integer pitch lag                          */
    Word16 frac,      /* input : fraction of lag                            */
    Word16 L_subfr,   /* input : subframe size                              */
    Word16 flag3,     /* input : if set, upsampling rate = 3 (6 otherwise)  */
    Flag  *pOverflow  /* output: if set, overflow occurred in this function */
)
{
    register Word16 j;

    Word16 *pX0;
    Word16 *pX2;
    Word16 *pX3;
    Word16 *p_exc;
    const Word16 *pC1_ref;
    const Word16 *pC2_ref;
#if (MIPS_DSP_R2_LE) || (MIPS_DSP_R1_LE)
    Word32 temp_18_16, temp_14_12, temp_10_8, temp_6_4, temp_2_0;
    Word32 temp_1_3, temp_5_7, temp_9_11, temp_13_15, temp_17_19;
#elif (MIPS32_R2_LE)
    register Word16 i;
    register Word16 k;
    Word16 *pC1;
    Word16 Coeff_1[(L_INTER10<<1)];
    Word32 s1;
    Word32 s2;
#endif

    OSCL_UNUSED_ARG(pOverflow);

    pX0 = &(exc[-T0]);

    /* frac goes between -3 and 3 */

    frac = -frac;

    if (flag3 != 0)
    {
        frac <<= 1;   /* inter_3l[k] = inter_6[2*k] -> k' = 2*k */
    }

    if (frac < 0)
    {
        frac += UP_SAMP_MAX;
        pX0--;
    }

    pC1_ref = &inter_6_pred_lt[frac];
    pC2_ref = &inter_6_pred_lt[UP_SAMP_MAX-frac];

#if (MIPS_DSP_R2_LE) || (MIPS_DSP_R1_LE)
    {
        Word32 temp1, temp2, temp3, temp4;
        __asm__ volatile (
            "lh             %[temp_18_16],   108(%[pC1_ref])              \n\t"
            "lh             %[temp1],        96(%[pC1_ref])               \n\t"
            "lh             %[temp_14_12],   84(%[pC1_ref])               \n\t"
            "lh             %[temp2],        72(%[pC1_ref])               \n\t"
            "lh             %[temp_10_8],    60(%[pC1_ref])               \n\t"
            "lh             %[temp3],        48(%[pC1_ref])               \n\t"
            "lh             %[temp_6_4],     36(%[pC1_ref])               \n\t"
            "lh             %[temp4],        24(%[pC1_ref])               \n\t"
            "ins            %[temp_18_16],   %[temp1],       16,  16      \n\t"
            "ins            %[temp_14_12],   %[temp2],       16,  16      \n\t"
            "ins            %[temp_10_8],    %[temp3],       16,  16      \n\t"
            "ins            %[temp_6_4],     %[temp4],       16,  16      \n\t"
            "lh             %[temp_2_0],     12(%[pC1_ref])               \n\t"
            "lh             %[temp1],        0(%[pC1_ref])                \n\t"
            "lh             %[temp_1_3],     0(%[pC2_ref])                \n\t"
            "lh             %[temp2],        12(%[pC2_ref])               \n\t"
            "lh             %[temp_5_7],     24(%[pC2_ref])               \n\t"
            "lh             %[temp3],        36(%[pC2_ref])               \n\t"
            "ins            %[temp_2_0],     %[temp1],       16,  16      \n\t"
            "ins            %[temp_1_3],     %[temp2],       16,  16      \n\t"
            "ins            %[temp_5_7],     %[temp3],       16,  16      \n\t"
            "lh             %[temp_9_11],    48(%[pC2_ref])               \n\t"
            "lh             %[temp1],        60(%[pC2_ref])               \n\t"
            "lh             %[temp_13_15],   72(%[pC2_ref])               \n\t"
            "lh             %[temp2],        84(%[pC2_ref])               \n\t"
            "lh             %[temp_17_19],   96(%[pC2_ref])               \n\t"
            "lh             %[temp3],        108(%[pC2_ref])              \n\t"
            "ins            %[temp_9_11],    %[temp1],       16,  16      \n\t"
            "ins            %[temp_13_15],   %[temp2],       16,  16      \n\t"
            "ins            %[temp_17_19],   %[temp3],       16,  16      \n\t"

            : [temp_18_16] "=&r" (temp_18_16), [temp_14_12] "=&r" (temp_14_12),
              [temp_10_8] "=&r" (temp_10_8), [temp_6_4] "=&r" (temp_6_4),
              [temp_2_0] "=&r" (temp_2_0), [temp_1_3] "=&r" (temp_1_3),
              [temp_5_7] "=&r" (temp_5_7), [temp_9_11] "=&r" (temp_9_11),
              [temp_13_15] "=&r" (temp_13_15), [temp_17_19] "=&r" (temp_17_19),
              [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4)
            : [pC1_ref] "r" (pC1_ref), [pC2_ref] "r" (pC2_ref)
            :  "memory"
        );
    }
#elif (MIPS32_R2_LE)
    pC1 = Coeff_1;
    k = 0;

    for (i = L_INTER10 >> 1; i > 0; i--)
    {
        *(pC1++) = pC1_ref[k];
        *(pC1++) = pC2_ref[k];
        k += UP_SAMP_MAX;
        *(pC1++) = pC1_ref[k];
        *(pC1++) = pC2_ref[k];
        k += UP_SAMP_MAX;

    }
#endif

    p_exc = exc;


#if (MIPS_DSP_R2_LE)
    for (j = (L_subfr >> 1); j != 0 ; j-=2)
    {
        Word32 temp3,temp1,temp2, temp4;
        __asm__ volatile (
            "mult        $ac3,       $zero,       $zero            \n\t"
            "mult        $ac2,       $zero,       $zero            \n\t"
            "mult        $ac1,       $zero,       $zero            \n\t"
            "mult        $ac0,       $zero,       $zero            \n\t"
            "ulw         %[temp1],   -18(%[pX0])                   \n\t"
            "ulw         %[temp2],   -14(%[pX0])                   \n\t"
            "ulw         %[temp3],   -10(%[pX0])                   \n\t"
            "ulw         %[temp4],   -6(%[pX0])                    \n\t"
            "dpa.w.ph    $ac0,       %[temp1],    %[temp_18_16]    \n\t"
            "dpa.w.ph    $ac0,       %[temp2],    %[temp_14_12]    \n\t"
            "dpa.w.ph    $ac0,       %[temp3],    %[temp_10_8]     \n\t"
            "dpa.w.ph    $ac0,       %[temp4],    %[temp_6_4]      \n\t"
            "dpa.w.ph    $ac3,       %[temp2],    %[temp_18_16]    \n\t"
            "dpa.w.ph    $ac3,       %[temp3],    %[temp_14_12]    \n\t"
            "dpa.w.ph    $ac3,       %[temp4],    %[temp_10_8]     \n\t"
            "ulw         %[temp1],   -2(%[pX0])                    \n\t"
            "ulw         %[temp2],   2(%[pX0])                     \n\t"
            "ulw         %[temp3],   6(%[pX0])                     \n\t"
            "ulw         %[temp4],   10(%[pX0])                    \n\t"
            "dpa.w.ph    $ac0,       %[temp1],    %[temp_2_0]      \n\t"
            "dpa.w.ph    $ac0,       %[temp2],    %[temp_1_3]      \n\t"
            "dpa.w.ph    $ac0,       %[temp3],    %[temp_5_7]      \n\t"
            "dpa.w.ph    $ac0,       %[temp4],    %[temp_9_11]     \n\t"
            "dpa.w.ph    $ac3,       %[temp1],    %[temp_6_4]      \n\t"
            "dpa.w.ph    $ac3,       %[temp2],    %[temp_2_0]      \n\t"
            "dpa.w.ph    $ac3,       %[temp3],    %[temp_1_3]      \n\t"
            "dpa.w.ph    $ac3,       %[temp4],    %[temp_5_7]      \n\t"
            "ulw         %[temp2],   14(%[pX0])                    \n\t"
            "ulw         %[temp3],   18(%[pX0])                    \n\t"
            "ulw         %[temp4],   22(%[pX0])                    \n\t"
            "ulw         %[temp1],   -16(%[pX0])                   \n\t"
            "dpa.w.ph    $ac0,       %[temp2],    %[temp_13_15]    \n\t"
            "dpa.w.ph    $ac0,       %[temp3],    %[temp_17_19]    \n\t"
            "dpa.w.ph    $ac3,       %[temp2],    %[temp_9_11]     \n\t"
            "dpa.w.ph    $ac3,       %[temp3],    %[temp_13_15]    \n\t"
            "dpa.w.ph    $ac3,       %[temp4],    %[temp_17_19]    \n\t"
//ac0 an3
            "ulw         %[temp2],   -12(%[pX0])                   \n\t"
            "ulw         %[temp3],   -8(%[pX0])                    \n\t"
            "ulw         %[temp4],   -4(%[pX0])                    \n\t"
            "dpa.w.ph    $ac1,       %[temp1],    %[temp_18_16]    \n\t"
            "dpa.w.ph    $ac1,       %[temp2],    %[temp_14_12]    \n\t"
            "dpa.w.ph    $ac1,       %[temp3],    %[temp_10_8]     \n\t"
            "dpa.w.ph    $ac1,       %[temp4],    %[temp_6_4]      \n\t"
            "dpa.w.ph    $ac2,       %[temp2],    %[temp_18_16]    \n\t"
            "dpa.w.ph    $ac2,       %[temp3],    %[temp_14_12]    \n\t"
            "dpa.w.ph    $ac2,       %[temp4],    %[temp_10_8]     \n\t"
            "ulw         %[temp1],   0(%[pX0])                     \n\t"
            "ulw         %[temp2],   4(%[pX0])                     \n\t"
            "ulw         %[temp3],   8(%[pX0])                     \n\t"
            "ulw         %[temp4],   12(%[pX0])                    \n\t"
            "dpa.w.ph    $ac1,       %[temp1],    %[temp_2_0]      \n\t"
            "dpa.w.ph    $ac1,       %[temp2],    %[temp_1_3]      \n\t"
            "dpa.w.ph    $ac1,       %[temp3],    %[temp_5_7]      \n\t"
            "dpa.w.ph    $ac1,       %[temp4],    %[temp_9_11]     \n\t"
            "dpa.w.ph    $ac2,       %[temp1],    %[temp_6_4]      \n\t"
            "dpa.w.ph    $ac2,       %[temp2],    %[temp_2_0]      \n\t"
            "dpa.w.ph    $ac2,       %[temp3],    %[temp_1_3]      \n\t"
            "dpa.w.ph    $ac2,       %[temp4],    %[temp_5_7]      \n\t"
            "ulw         %[temp1],   16(%[pX0])                    \n\t"
            "ulw         %[temp2],   20(%[pX0])                    \n\t"
            "ulw         %[temp3],   24(%[pX0])                    \n\t"
            "dpa.w.ph    $ac1,       %[temp1],    %[temp_13_15]    \n\t"
            "dpa.w.ph    $ac1,       %[temp2],    %[temp_17_19]    \n\t"
            "dpa.w.ph    $ac2,       %[temp1],    %[temp_9_11]     \n\t"
            "dpa.w.ph    $ac2,       %[temp2],    %[temp_13_15]    \n\t"
            "dpa.w.ph    $ac2,       %[temp3],    %[temp_17_19]    \n\t"
            "mflo        %[temp1],   $ac0                          \n\t"
            "mflo        %[temp3],   $ac3                          \n\t"
            "mflo        %[temp2],   $ac1                          \n\t"
            "mflo        %[temp4],   $ac2                          \n\t"
            "addiu       %[pX0],     %[pX0],      8                \n\t"
            "addiu       %[p_exc],   %[p_exc],    8                \n\t"
            "shra_r.w   %[temp1],    %[temp1],    15               \n\t"
            "shra_r.w   %[temp3],    %[temp3],    15               \n\t"
            "shra_r.w   %[temp2],    %[temp2],    15               \n\t"
            "shra_r.w   %[temp4],    %[temp4],    15               \n\t"
            "usw        %[temp1],   -8(%[p_exc])                   \n\t"
            "usw        %[temp2],   -6(%[p_exc])                   \n\t"
            "usw        %[temp3],   -4(%[p_exc])                   \n\t"
            "usw        %[temp4],   -2(%[p_exc])                   \n\t"

            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),[pX0] "+r" (pX0),
              [p_exc] "+r" (p_exc)
            : [temp_18_16] "r" (temp_18_16), [temp_14_12] "r" (temp_14_12),
              [temp_10_8] "r" (temp_10_8), [temp_6_4] "r" (temp_6_4),
              [temp_2_0] "r" (temp_2_0), [temp_1_3] "r" (temp_1_3),
              [temp_5_7] "r" (temp_5_7), [temp_9_11] "r" (temp_9_11),
              [temp_13_15] "r" (temp_13_15), [temp_17_19] "r" (temp_17_19)
           :  "memory", "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
              "$ac3hi", "$ac3lo"
        );
    }
#elif (MIPS_DSP_R1_LE)/*(MIPS_DSP_R1_LE)*/
    for (j = (L_subfr >> 1); j != 0 ; j-=2)
    {
        Word32 temp3,temp1,temp2, temp4;
        __asm__ volatile (
            "mult           $ac3,       $zero,       $zero            \n\t"
            "mult           $ac2,       $zero,       $zero            \n\t"
            "mult           $ac1,       $zero,       $zero            \n\t"
            "mult           $ac0,       $zero,       $zero            \n\t"
            "ulw            %[temp1],   -18(%[pX0])                   \n\t"
            "ulw            %[temp2],   -14(%[pX0])                   \n\t"
            "ulw            %[temp3],   -10(%[pX0])                   \n\t"
            "ulw            %[temp4],   -6(%[pX0])                    \n\t"
            "maq_s.w.phr    $ac0,       %[temp1],    %[temp_18_16]    \n\t"
            "maq_s.w.phr    $ac0,       %[temp2],    %[temp_14_12]    \n\t"
            "maq_s.w.phr    $ac0,       %[temp3],    %[temp_10_8]     \n\t"
            "maq_s.w.phr    $ac0,       %[temp4],    %[temp_6_4]      \n\t"
            "maq_s.w.phl    $ac0,       %[temp1],    %[temp_18_16]    \n\t"
            "maq_s.w.phl    $ac0,       %[temp2],    %[temp_14_12]    \n\t"
            "maq_s.w.phl    $ac0,       %[temp3],    %[temp_10_8]     \n\t"
            "maq_s.w.phl    $ac0,       %[temp4],    %[temp_6_4]      \n\t"
            "maq_s.w.phr    $ac3,       %[temp2],    %[temp_18_16]    \n\t"
            "maq_s.w.phr    $ac3,       %[temp3],    %[temp_14_12]    \n\t"
            "maq_s.w.phr    $ac3,       %[temp4],    %[temp_10_8]     \n\t"
            "maq_s.w.phl    $ac3,       %[temp2],    %[temp_18_16]    \n\t"
            "maq_s.w.phl    $ac3,       %[temp3],    %[temp_14_12]    \n\t"
            "maq_s.w.phl    $ac3,       %[temp4],    %[temp_10_8]     \n\t"
            "ulw            %[temp1],   -2(%[pX0])                    \n\t"
            "ulw            %[temp2],   2(%[pX0])                     \n\t"
            "ulw            %[temp3],   6(%[pX0])                     \n\t"
            "ulw            %[temp4],   10(%[pX0])                    \n\t"
            "maq_s.w.phr    $ac0,       %[temp1],    %[temp_2_0]      \n\t"
            "maq_s.w.phr    $ac0,       %[temp2],    %[temp_1_3]      \n\t"
            "maq_s.w.phr    $ac0,       %[temp3],    %[temp_5_7]      \n\t"
            "maq_s.w.phr    $ac0,       %[temp4],    %[temp_9_11]     \n\t"
            "maq_s.w.phl    $ac0,       %[temp1],    %[temp_2_0]      \n\t"
            "maq_s.w.phl    $ac0,       %[temp2],    %[temp_1_3]      \n\t"
            "maq_s.w.phl    $ac0,       %[temp3],    %[temp_5_7]      \n\t"
            "maq_s.w.phl    $ac0,       %[temp4],    %[temp_9_11]     \n\t"
            "maq_s.w.phr    $ac3,       %[temp1],    %[temp_6_4]      \n\t"
            "maq_s.w.phr    $ac3,       %[temp2],    %[temp_2_0]      \n\t"
            "maq_s.w.phr    $ac3,       %[temp3],    %[temp_1_3]      \n\t"
            "maq_s.w.phr    $ac3,       %[temp4],    %[temp_5_7]      \n\t"
            "maq_s.w.phl    $ac3,       %[temp1],    %[temp_6_4]      \n\t"
            "maq_s.w.phl    $ac3,       %[temp2],    %[temp_2_0]      \n\t"
            "maq_s.w.phl    $ac3,       %[temp3],    %[temp_1_3]      \n\t"
            "maq_s.w.phl    $ac3,       %[temp4],    %[temp_5_7]      \n\t"
            "ulw            %[temp2],   14(%[pX0])                    \n\t"
            "ulw            %[temp3],   18(%[pX0])                    \n\t"
            "ulw            %[temp4],   22(%[pX0])                    \n\t"
            "ulw            %[temp1],   -16(%[pX0])                   \n\t"
            "maq_s.w.phr    $ac0,       %[temp2],    %[temp_13_15]    \n\t"
            "maq_s.w.phr    $ac0,       %[temp3],    %[temp_17_19]    \n\t"
            "maq_s.w.phl    $ac0,       %[temp2],    %[temp_13_15]    \n\t"
            "maq_s.w.phl    $ac0,       %[temp3],    %[temp_17_19]    \n\t"
            "maq_s.w.phr    $ac3,       %[temp2],    %[temp_9_11]     \n\t"
            "maq_s.w.phr    $ac3,       %[temp3],    %[temp_13_15]    \n\t"
            "maq_s.w.phr    $ac3,       %[temp4],    %[temp_17_19]    \n\t"
            "maq_s.w.phl    $ac3,       %[temp2],    %[temp_9_11]     \n\t"
            "maq_s.w.phl    $ac3,       %[temp3],    %[temp_13_15]    \n\t"
            "maq_s.w.phl    $ac3,       %[temp4],    %[temp_17_19]    \n\t"
            "ulw            %[temp2],   -12(%[pX0])                   \n\t"
            "ulw            %[temp3],   -8(%[pX0])                    \n\t"
            "ulw            %[temp4],   -4(%[pX0])                    \n\t"
            "maq_s.w.phr    $ac1,       %[temp1],    %[temp_18_16]    \n\t"
            "maq_s.w.phr    $ac1,       %[temp2],    %[temp_14_12]    \n\t"
            "maq_s.w.phr    $ac1,       %[temp3],    %[temp_10_8]     \n\t"
            "maq_s.w.phr    $ac1,       %[temp4],    %[temp_6_4]      \n\t"
            "maq_s.w.phl    $ac1,       %[temp1],    %[temp_18_16]    \n\t"
            "maq_s.w.phl    $ac1,       %[temp2],    %[temp_14_12]    \n\t"
            "maq_s.w.phl    $ac1,       %[temp3],    %[temp_10_8]     \n\t"
            "maq_s.w.phl    $ac1,       %[temp4],    %[temp_6_4]      \n\t"
            "maq_s.w.phr    $ac2,       %[temp2],    %[temp_18_16]    \n\t"
            "maq_s.w.phr    $ac2,       %[temp3],    %[temp_14_12]    \n\t"
            "maq_s.w.phr    $ac2,       %[temp4],    %[temp_10_8]     \n\t"
            "maq_s.w.phl    $ac2,       %[temp2],    %[temp_18_16]    \n\t"
            "maq_s.w.phl    $ac2,       %[temp3],    %[temp_14_12]    \n\t"
            "maq_s.w.phl    $ac2,       %[temp4],    %[temp_10_8]     \n\t"
            "ulw            %[temp1],   0(%[pX0])                     \n\t"
            "ulw            %[temp2],   4(%[pX0])                     \n\t"
            "ulw            %[temp3],   8(%[pX0])                     \n\t"
            "ulw            %[temp4],   12(%[pX0])                    \n\t"
            "maq_s.w.phr    $ac1,       %[temp1],    %[temp_2_0]      \n\t"
            "maq_s.w.phr    $ac1,       %[temp2],    %[temp_1_3]      \n\t"
            "maq_s.w.phr    $ac1,       %[temp3],    %[temp_5_7]      \n\t"
            "maq_s.w.phr    $ac1,       %[temp4],    %[temp_9_11]     \n\t"
            "maq_s.w.phl    $ac1,       %[temp1],    %[temp_2_0]      \n\t"
            "maq_s.w.phl    $ac1,       %[temp2],    %[temp_1_3]      \n\t"
            "maq_s.w.phl    $ac1,       %[temp3],    %[temp_5_7]      \n\t"
            "maq_s.w.phl    $ac1,       %[temp4],    %[temp_9_11]     \n\t"
            "maq_s.w.phr    $ac2,       %[temp1],    %[temp_6_4]      \n\t"
            "maq_s.w.phr    $ac2,       %[temp2],    %[temp_2_0]      \n\t"
            "maq_s.w.phr    $ac2,       %[temp3],    %[temp_1_3]      \n\t"
            "maq_s.w.phr    $ac2,       %[temp4],    %[temp_5_7]      \n\t"
            "maq_s.w.phl    $ac2,       %[temp1],    %[temp_6_4]      \n\t"
            "maq_s.w.phl    $ac2,       %[temp2],    %[temp_2_0]      \n\t"
            "maq_s.w.phl    $ac2,       %[temp3],    %[temp_1_3]      \n\t"
            "maq_s.w.phl    $ac2,       %[temp4],    %[temp_5_7]      \n\t"
            "ulw            %[temp1],   16(%[pX0])                    \n\t"
            "ulw            %[temp2],   20(%[pX0])                    \n\t"
            "ulw            %[temp3],   24(%[pX0])                    \n\t"
            "maq_s.w.phr    $ac1,       %[temp1],    %[temp_13_15]    \n\t"
            "maq_s.w.phr    $ac1,       %[temp2],    %[temp_17_19]    \n\t"
            "maq_s.w.phl    $ac1,       %[temp1],    %[temp_13_15]    \n\t"
            "maq_s.w.phl    $ac1,       %[temp2],    %[temp_17_19]    \n\t"
            "maq_s.w.phr    $ac2,       %[temp1],    %[temp_9_11]     \n\t"
            "maq_s.w.phr    $ac2,       %[temp2],    %[temp_13_15]    \n\t"
            "maq_s.w.phr    $ac2,       %[temp3],    %[temp_17_19]    \n\t"
            "maq_s.w.phl    $ac2,       %[temp1],    %[temp_9_11]     \n\t"
            "maq_s.w.phl    $ac2,       %[temp2],    %[temp_13_15]    \n\t"
            "maq_s.w.phl    $ac2,       %[temp3],    %[temp_17_19]    \n\t"
            "extr.w         %[temp1],   $ac0,        1                \n\t"
            "extr.w         %[temp3],   $ac3,        1                \n\t"
            "extr.w         %[temp2],   $ac1,        1                \n\t"
            "extr.w         %[temp4],   $ac2,        1                \n\t"
            "addiu          %[pX0],     %[pX0],      8                \n\t"
            "addiu          %[p_exc],   %[p_exc],    8                \n\t"
            "shra_r.w       %[temp1],   %[temp1],    15               \n\t"
            "shra_r.w       %[temp3],   %[temp3],    15               \n\t"
            "shra_r.w       %[temp2],   %[temp2],    15               \n\t"
            "shra_r.w       %[temp4],   %[temp4],    15               \n\t"
            "usw            %[temp1],   -8(%[p_exc])                  \n\t"
            "usw            %[temp2],   -6(%[p_exc])                  \n\t"
            "usw            %[temp3],   -4(%[p_exc])                  \n\t"
            "usw            %[temp4],   -2(%[p_exc])                  \n\t"

            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),[pX0] "+r" (pX0),
              [p_exc] "+r" (p_exc)
            : [temp_18_16] "r" (temp_18_16), [temp_14_12] "r" (temp_14_12),
              [temp_10_8] "r" (temp_10_8), [temp_6_4] "r" (temp_6_4),
              [temp_2_0] "r" (temp_2_0), [temp_1_3] "r" (temp_1_3),
              [temp_5_7] "r" (temp_5_7), [temp_9_11] "r" (temp_9_11),
              [temp_13_15] "r" (temp_13_15), [temp_17_19] "r" (temp_17_19)
           :  "memory", "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
              "$ac3hi", "$ac3lo"
        );
    }
#elif (MIPS32_R2_LE)
    for (j = (L_subfr >> 1); j != 0 ; j-=1)
    {
        pX0++;
        pX2 = pX0;
        pX3 = pX0++;
        pC1 = Coeff_1;
        s1  = 0x00004000L;
        s2  = 0x00004000L;

        {
            Word32 temp1, temp2, temp3, temp4, temp5;
            Word32 temp6, temp7, temp8, temp9, temp10;
            __asm__ volatile (
                "lh            %[temp1],   0(%[pX3])                     \n\t"
                "lh            %[temp2],   -2(%[pX3])                    \n\t"
                "lh            %[temp3],   0(%[pX2])                     \n\t"
                "lh            %[temp4],   2(%[pX2])                     \n\t"
                "lh            %[temp5],   0(%[pC1])                     \n\t"
                "lh            %[temp6],   2(%[pC1])                     \n\t"
                "mul           %[temp7],   %[temp1],         %[temp5]    \n\t"
                "mul           %[temp8],   %[temp2],         %[temp5]    \n\t"
                "mul           %[temp9],   %[temp3],         %[temp6]    \n\t"
                "mul           %[temp10],  %[temp4],         %[temp6]    \n\t"
                "lh            %[temp1],   -4(%[pX3])                    \n\t"
                "lh            %[temp3],   4(%[pX2])                     \n\t"
                "lh            %[temp5],   4(%[pC1])                     \n\t"
                "addu          %[s2],      %[s2],            %[temp7]    \n\t"
                "addu          %[s1],      %[s1],            %[temp8]    \n\t"
                "lh            %[temp6],   6(%[pC1])                     \n\t"
                "addu          %[s1],      %[s1],            %[temp9]    \n\t"
                "addu          %[s2],      %[s2],            %[temp10]   \n\t"
                "mul           %[temp7],   %[temp2],         %[temp5]    \n\t"
                "mul           %[temp8],   %[temp1],         %[temp5]    \n\t"
                "mul           %[temp9],   %[temp4],         %[temp6]    \n\t"
                "mul           %[temp10],  %[temp3],         %[temp6]    \n\t"
                "lh            %[temp2],   -6(%[pX3])                    \n\t"
                "lh            %[temp4],   6(%[pX2])                     \n\t"
                "lh            %[temp5],   8(%[pC1])                     \n\t"
                "addu          %[s2],      %[s2],            %[temp7]    \n\t"
                "addu          %[s1],      %[s1],            %[temp8]    \n\t"
                "lh            %[temp6],   10(%[pC1])                    \n\t"
                "addu          %[s1],      %[s1],            %[temp9]    \n\t"
                "addu          %[s2],      %[s2],            %[temp10]   \n\t"
                "mul           %[temp7],   %[temp1],         %[temp5]    \n\t"
                "mul           %[temp8],   %[temp2],         %[temp5]    \n\t"
                "mul           %[temp9],   %[temp3],         %[temp6]    \n\t"
                "mul           %[temp10],  %[temp4],         %[temp6]    \n\t"
                "lh            %[temp1],   -8(%[pX3])                    \n\t"
                "lh            %[temp3],   8(%[pX2])                     \n\t"
                "lh            %[temp5],   12(%[pC1])                    \n\t"
                "addu          %[s2],      %[s2],            %[temp7]    \n\t"
                "addu          %[s1],      %[s1],            %[temp8]    \n\t"
                "lh            %[temp6],   14(%[pC1])                    \n\t"
                "addu          %[s1],      %[s1],            %[temp9]    \n\t"
                "addu          %[s2],      %[s2],            %[temp10]   \n\t"
                "mul           %[temp7],   %[temp2],         %[temp5]    \n\t"
                "mul           %[temp8],   %[temp1],         %[temp5]    \n\t"
                "mul           %[temp9],   %[temp4],         %[temp6]    \n\t"
                "mul           %[temp10],  %[temp3],         %[temp6]    \n\t"
                "lh            %[temp2],   -10(%[pX3])                   \n\t"
                "lh            %[temp4],   10(%[pX2])                    \n\t"
                "lh            %[temp5],   16(%[pC1])                    \n\t"
                "addu          %[s2],      %[s2],            %[temp7]    \n\t"
                "addu          %[s1],      %[s1],            %[temp8]    \n\t"
                "lh            %[temp6],   18(%[pC1])                    \n\t"
                "addu          %[s1],      %[s1],            %[temp9]    \n\t"
                "addu          %[s2],      %[s2],            %[temp10]   \n\t"
                "mul           %[temp7],   %[temp1],         %[temp5]    \n\t"
                "mul           %[temp8],   %[temp2],         %[temp5]    \n\t"
                "mul           %[temp9],   %[temp3],         %[temp6]    \n\t"
                "mul           %[temp10],  %[temp4],         %[temp6]    \n\t"
                "lh            %[temp1],   -12(%[pX3])                   \n\t"
                "lh            %[temp3],   12(%[pX2])                    \n\t"
                "lh            %[temp5],   20(%[pC1])                    \n\t"
                "addu          %[s2],      %[s2],            %[temp7]    \n\t"
                "addu          %[s1],      %[s1],            %[temp8]    \n\t"
                "lh            %[temp6],   22(%[pC1])                    \n\t"
                "addu          %[s1],      %[s1],            %[temp9]    \n\t"
                "addu          %[s2],      %[s2],            %[temp10]   \n\t"
                "mul           %[temp7],   %[temp2],         %[temp5]    \n\t"
                "mul           %[temp8],   %[temp1],         %[temp5]    \n\t"
                "mul           %[temp9],   %[temp4],         %[temp6]    \n\t"
                "mul           %[temp10],  %[temp3],         %[temp6]    \n\t"
                "lh            %[temp2],   -14(%[pX3])                   \n\t"
                "lh            %[temp4],   14(%[pX2])                    \n\t"
                "lh            %[temp5],   24(%[pC1])                    \n\t"
                "addu          %[s2],      %[s2],            %[temp7]    \n\t"
                "addu          %[s1],      %[s1],            %[temp8]    \n\t"
                "lh            %[temp6],   26(%[pC1])                    \n\t"
                "addu          %[s1],      %[s1],            %[temp9]    \n\t"
                "addu          %[s2],      %[s2],            %[temp10]   \n\t"
                "mul           %[temp7],   %[temp1],         %[temp5]    \n\t"
                "mul           %[temp8],   %[temp2],         %[temp5]    \n\t"
                "mul           %[temp9],   %[temp3],         %[temp6]    \n\t"
                "mul           %[temp10],  %[temp4],         %[temp6]    \n\t"
                "lh            %[temp1],   -16(%[pX3])                   \n\t"
                "lh            %[temp3],   16(%[pX2])                    \n\t"
                "lh            %[temp5],   28(%[pC1])                    \n\t"
                "addu          %[s2],      %[s2],            %[temp7]    \n\t"
                "addu          %[s1],      %[s1],            %[temp8]    \n\t"
                "lh            %[temp6],   30(%[pC1])                    \n\t"
                "addu          %[s1],      %[s1],            %[temp9]    \n\t"
                "addu          %[s2],      %[s2],            %[temp10]   \n\t"
                "mul           %[temp7],   %[temp2],         %[temp5]    \n\t"
                "mul           %[temp8],   %[temp1],         %[temp5]    \n\t"
                "mul           %[temp9],   %[temp4],         %[temp6]    \n\t"
                "mul           %[temp10],  %[temp3],         %[temp6]    \n\t"
                "lh            %[temp2],   -18(%[pX3])                   \n\t"
                "lh            %[temp4],   18(%[pX2])                    \n\t"
                "lh            %[temp5],   32(%[pC1])                    \n\t"
                "addu          %[s2],      %[s2],            %[temp7]    \n\t"
                "addu          %[s1],      %[s1],            %[temp8]    \n\t"
                "lh            %[temp6],   34(%[pC1])                    \n\t"
                "addu          %[s1],      %[s1],            %[temp9]    \n\t"
                "addu          %[s2],      %[s2],            %[temp10]   \n\t"
                "mul           %[temp7],   %[temp1],         %[temp5]    \n\t"
                "mul           %[temp8],   %[temp2],         %[temp5]    \n\t"
                "mul           %[temp9],   %[temp3],         %[temp6]    \n\t"
                "mul           %[temp10],  %[temp4],         %[temp6]    \n\t"
                "lh            %[temp1],   -20(%[pX3])                   \n\t"
                "lh            %[temp3],   20(%[pX2])                    \n\t"
                "lh            %[temp5],   36(%[pC1])                    \n\t"
                "addu          %[s2],      %[s2],            %[temp7]    \n\t"
                "addu          %[s1],      %[s1],            %[temp8]    \n\t"
                "lh            %[temp6],   38(%[pC1])                    \n\t"
                "addu          %[s1],      %[s1],            %[temp9]    \n\t"
                "addu          %[s2],      %[s2],            %[temp10]   \n\t"
                "mul           %[temp7],   %[temp2],         %[temp5]    \n\t"
                "mul           %[temp8],   %[temp1],         %[temp5]    \n\t"
                "mul           %[temp9],   %[temp4],         %[temp6]    \n\t"
                "mul           %[temp10],  %[temp3],         %[temp6]    \n\t"
                "addu          %[s2],      %[s2],            %[temp7]    \n\t"
                "addu          %[s1],      %[s1],            %[temp8]    \n\t"
                "addu          %[s1],      %[s1],            %[temp9]    \n\t"
                "addu          %[s2],      %[s2],            %[temp10]   \n\t"

                :[temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                 [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
                 [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
                 [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
                 [temp9] "=&r" (temp9), [temp10] "=&r" (temp10),
                 [s1] "+r" (s1), [s2] "+r" (s2)
                :[pX3] "r" (pX3), [pX2] "r" (pX2),
                 [pC1] "r" (pC1)
                :"memory", "hi", "lo"
            );

        }

        *(p_exc++) = (Word16)(s1 >> 15);
        *(p_exc++) = (Word16)(s2 >> 15);
    }
#endif

    return;
}

