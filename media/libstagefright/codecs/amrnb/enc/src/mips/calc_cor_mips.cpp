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



 Pathname: ./audio/gsm-amr/c/src/calc_cor.c

     Date: 06/12/2000

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Initial Optimization

 Description: Optimize code by calculating two correlation per iteration
              of the outer loop.

 Description: Delete psedocode

 Description: Synchronized file with UMTS version 3.2.0. Updated coding
              template. Removed unnecessary include files.

 Description: Made the following changes per comments from Phase 2/3 review:
              1. Defined one local variable per line.

 Description:
              1. Eliminated unused include file typedef.h.
              2. Replaced array addressing by pointers
              3. Unrolled loops to save extra accesses to memory

 Description:  Replaced "int" and/or "char" with OSCL defined types.

 Description: Using inline functions from fxp_arithmetic.h for mac operations.

 Description: Replacing fxp_arithmetic.h with basic_op.h.

 Description:

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/
#include "calc_cor.h"
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
 FUNCTION NAME: comp_corr
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    scal_sig = array of input samples. (Word16)
    L_frame = length of frame used to compute pitch(Word16)
    lag_max = maximum lag (Word16)
    lag_min = minimum lag (Word16)
    corr = pointer to array of correlations corresponding to the selected
        lags. (Word32)

 Outputs:
    corr = pointer to array of correlations corresponding to the selected
        lags. (Word32)

 Returns:
    none

 Global Variables Used:
    none

 Local Variables Needed:
    none

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

 This function calculates all correlations of scal_sig[] in a given delay
 range.

 The correlation is given by

         cor[t] = <scal_sig[n],scal_sig[n-t]>,  t=lag_min,...,lag_max

 The function outputs all of the correlations

------------------------------------------------------------------------------
 REQUIREMENTS

 none

------------------------------------------------------------------------------
 REFERENCES

 [1] calc_cor.c, UMTS GSM AMR speech codec, R99 - Version 3.2.0, March 2, 2001

------------------------------------------------------------------------------
 PSEUDO-CODE

void comp_corr (
    Word16 scal_sig[],  // i   : scaled signal.
    Word16 L_frame,     // i   : length of frame to compute pitch
    Word16 lag_max,     // i   : maximum lag
    Word16 lag_min,     // i   : minimum lag
    Word32 corr[])      // o   : correlation of selected lag
{
    Word16 i, j;
    Word16 *p, *p1;
    Word32 t0;

    for (i = lag_max; i >= lag_min; i--)
    {
       p = scal_sig;
       p1 = &scal_sig[-i];
       t0 = 0;

       for (j = 0; j < L_frame; j++, p++, p1++)
       {
          t0 = L_mac (t0, *p, *p1);
       }
       corr[-i] = t0;
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

void comp_corr(
    Word16 scal_sig[],  /* i   : scaled signal.                     */
    Word16 L_frame,     /* i   : length of frame to compute pitch   */
    Word16 lag_max,     /* i   : maximum lag                        */
    Word16 lag_min,     /* i   : minimum lag                        */
    Word32 corr[])      /* o   : correlation of selected lag        */
{

    /*---------------------------------------------------
    ; lag_max and lag_min are typically negative numbers
    -----------------------------------------------------*/

    /* PIT_MIN_MR122 18        Minimum pitch lag (MR122 mode)           */
    /* PIT_MIN       20        Minimum pitch lag (all other modes)      */
    /* PIT_MAX       143       Maximum pitch lag                        */

    Word16 i;
    Word16 j;
    Word16 *p_scal_sig;

#if (MIPS_DSP_R2_LE) ||  (MIPS_DSP_R1_LE)
    Word16 *p;
    Word16 *p1;
    Word16 *p2;
#elif (MIPS32_R2_LE)
    Word32 t1;
    Word32 t2;
    Word32 t3;
    Word32 t4;
    Word32 temp_loop = (L_frame >> 1);
#endif
    corr = corr - lag_max ;
    p_scal_sig = &scal_sig[-lag_max];

    if(scal_sig[0] != 0) {
        p_scal_sig = &scal_sig[-lag_max];
    }

#if (MIPS_DSP_R2_LE)
    for (i = ((lag_max - lag_min) >> 2) + 1; i > 0; i--)
    {
        int a, b, c, d, ab;
        int a1,b1,a1b,ab1,a1b1;
        int temp1, temp2;
        p  = &scal_sig[0];
        p1 = p_scal_sig;
        p_scal_sig +=4;
        p2 = p + 8*(L_frame >> 3);

        __asm__ volatile (
            ".set push                                              \n\t"
            ".set noreorder                                         \n\t"
            "ulw           %[a],       0(%[p1])                     \n\t"
            "ulw           %[b],       4(%[p1])                     \n\t"
            "packrl.ph     %[ab],      %[b],               %[a]     \n\t"
            "mult          $ac0,       $0,                 $0       \n\t"
            "mult          $ac1,       $0,                 $0       \n\t"
            "mult          $ac2,       $0,                 $0       \n\t"
            "mult          $ac3,       $0,                 $0       \n\t"
        "1:                                                         \n\t"
            "ulw           %[temp1],   0(%[p])                      \n\t"
            "lw            %[a1],      8(%[p1])                     \n\t"
            "lw            %[b1],      12(%[p1])                    \n\t"
            "ulw           %[temp2],   4(%[p])                      \n\t"
            "packrl.ph     %[a1b],     %[a1],              %[b]     \n\t"
            "packrl.ph     %[a1b1],    %[b1],              %[a1]    \n\t"
            "dpaq_s.w.ph   $ac0,       %[temp1],           %[a]     \n\t"
            "dpaq_s.w.ph   $ac0,       %[temp2],           %[b]     \n\t"
            "dpaq_s.w.ph   $ac1,       %[temp1],           %[ab]    \n\t"
            "dpaq_s.w.ph   $ac1,       %[temp2],           %[a1b]   \n\t"
            "dpaq_s.w.ph   $ac2,       %[temp1],           %[b]     \n\t"
            "dpaq_s.w.ph   $ac2,       %[temp2],           %[a1]    \n\t"
            "dpaq_s.w.ph   $ac3,       %[temp1],           %[a1b]   \n\t"
            "dpaq_s.w.ph   $ac3,       %[temp2],           %[a1b1]  \n\t"
            "ulw           %[temp1],   8(%[p])                      \n\t"
            "lw            %[a],       16(%[p1])                    \n\t"
            "lw            %[b],       20(%[p1])                    \n\t"
            "ulw           %[temp2],   12(%[p])                     \n\t"
            "packrl.ph     %[a1b],     %[a],               %[b1]    \n\t"
            "packrl.ph     %[ab],      %[b],               %[a]     \n\t"
            "dpaq_s.w.ph   $ac0,       %[temp1],           %[a1]    \n\t"
            "dpaq_s.w.ph   $ac0,       %[temp2],           %[b1]    \n\t"
            "dpaq_s.w.ph   $ac1,       %[temp1],           %[a1b1]  \n\t"
            "dpaq_s.w.ph   $ac1,       %[temp2],           %[a1b]   \n\t"
            "dpaq_s.w.ph   $ac2,       %[temp1],           %[b1]    \n\t"
            "dpaq_s.w.ph   $ac2,       %[temp2],           %[a]     \n\t"
            "dpaq_s.w.ph   $ac3,       %[temp1],           %[a1b]   \n\t"
            "addiu         %[p],       %[p],               16       \n\t"
            "addiu         %[p1],      %[p1],              16       \n\t"
            "bne           %[p],       %[p2],              1b       \n\t"
            " dpaq_s.w.ph  $ac3,       %[temp2],           %[ab]    \n\t"
            "mflo          %[temp1],   $ac0                         \n\t"
            "mflo          %[temp2],   $ac1                         \n\t"
            "mflo          %[a1],      $ac2                         \n\t"
            "mflo          %[b1],      $ac3                         \n\t"
            "addiu         %[corr],    %[corr],            16       \n\t"
            "sw            %[temp1],   -16(%[corr])                 \n\t"
            "sw            %[temp2],   -12(%[corr])                 \n\t"
            "sw            %[a1],      -8(%[corr])                  \n\t"
            "sw            %[b1],      -4(%[corr])                  \n\t"
            ".set pop                                               \n\t"

            : [temp1]"=&r"(temp1), [temp2]"=&r"(temp2),
              [a]"=&r"(a), [b]"=&r"(b), [ab]"=&r"(ab), [a1]"=&r"(a1),
              [b1]"=&r"(b1), [a1b]"=&r"(a1b), [a1b1]"=&r"(a1b1),
              [p1]"+r"(p1), [p]"+r"(p), [corr]"+r"(corr)
            : [p2]"r"(p2)
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo", "$ac3hi",
              "$ac3lo", "memory"
        );

    }
#elif (MIPS_DSP_R1_LE)/*(MIPS_DSP_R1_LE)*/
    for (i = ((lag_max - lag_min) >> 2) + 1; i > 0; i--)
    {
        int a, b, c, d, ab;
        int a1,b1,a1b,ab1,a1b1;
        int temp1, temp2;
        p  = &scal_sig[0];
        p1 = p_scal_sig;
        p_scal_sig +=4;
        p2 = p + 8*(L_frame >> 3);

        __asm__ volatile (
            ".set push                                               \n\t"
            ".set noreorder                                          \n\t"
            "ulw            %[a],       0(%[p1])                     \n\t"
            "ulw            %[b],       4(%[p1])                     \n\t"
            "packrl.ph      %[ab],      %[b],              %[a]      \n\t"
            "mult           $ac0,       $0,                $0        \n\t"
            "mult           $ac1,       $0,                $0        \n\t"
            "mult           $ac2,       $0,                $0        \n\t"
            "mult           $ac3,       $0,                $0        \n\t"
         "1:                                                         \n\t"
            "ulw            %[temp1],   0(%[p])                      \n\t"
            "lw             %[a1],      8(%[p1])                     \n\t"
            "lw             %[b1],      12(%[p1])                    \n\t"
            "ulw            %[temp2],   4(%[p])                      \n\t"
            "packrl.ph      %[a1b],     %[a1],             %[b]      \n\t"
            "packrl.ph      %[a1b1],    %[b1],             %[a1]     \n\t"
            "maq_s.w.phl    $ac0,       %[temp1],          %[a]      \n\t"
            "maq_s.w.phr    $ac0,       %[temp1],          %[a]      \n\t"
            "maq_s.w.phl    $ac0,       %[temp2],          %[b]      \n\t"
            "maq_s.w.phr    $ac0,       %[temp2],          %[b]      \n\t"
            "maq_s.w.phl    $ac1,       %[temp1],          %[ab]     \n\t"
            "maq_s.w.phr    $ac1,       %[temp1],          %[ab]     \n\t"
            "maq_s.w.phl    $ac1,       %[temp2],          %[a1b]    \n\t"
            "maq_s.w.phr    $ac1,       %[temp2],          %[a1b]    \n\t"
            "maq_s.w.phl    $ac2,       %[temp1],          %[b]      \n\t"
            "maq_s.w.phr    $ac2,       %[temp1],          %[b]      \n\t"
            "maq_s.w.phl    $ac2,       %[temp2],          %[a1]     \n\t"
            "maq_s.w.phr    $ac2,       %[temp2],          %[a1]     \n\t"
            "maq_s.w.phl    $ac3,       %[temp1],          %[a1b]    \n\t"
            "maq_s.w.phr    $ac3,       %[temp1],          %[a1b]    \n\t"
            "maq_s.w.phl    $ac3,       %[temp2],          %[a1b1]   \n\t"
            "maq_s.w.phr    $ac3,       %[temp2],          %[a1b1]   \n\t"
            "ulw            %[temp1],   8(%[p])                      \n\t"
            "lw             %[a],       16(%[p1])                    \n\t"
            "lw             %[b],       20(%[p1])                    \n\t"
            "ulw            %[temp2],   12(%[p])                     \n\t"
            "packrl.ph      %[a1b],     %[a],              %[b1]     \n\t"
            "packrl.ph      %[ab],      %[b],              %[a]      \n\t"
            "maq_s.w.phl    $ac0,       %[temp1],          %[a1]     \n\t"
            "maq_s.w.phr    $ac0,       %[temp1],          %[a1]     \n\t"
            "maq_s.w.phl    $ac0,       %[temp2],          %[b1]     \n\t"
            "maq_s.w.phr    $ac0,       %[temp2],          %[b1]     \n\t"
            "maq_s.w.phl    $ac1,       %[temp1],          %[a1b1]   \n\t"
            "maq_s.w.phr    $ac1,       %[temp1],          %[a1b1]   \n\t"
            "maq_s.w.phl    $ac1,       %[temp2],          %[a1b]    \n\t"
            "maq_s.w.phr    $ac1,       %[temp2],          %[a1b]    \n\t"
            "maq_s.w.phl    $ac2,       %[temp1],          %[b1]     \n\t"
            "maq_s.w.phr    $ac2,       %[temp1],          %[b1]     \n\t"
            "maq_s.w.phl    $ac2,       %[temp2],          %[a]      \n\t"
            "maq_s.w.phr    $ac2,       %[temp2],          %[a]      \n\t"
            "addiu          %[p],       %[p],              16        \n\t"
            "maq_s.w.phl    $ac3,       %[temp1],          %[a1b]    \n\t"
            "maq_s.w.phr    $ac3,       %[temp1],          %[a1b]    \n\t"
            "maq_s.w.phl    $ac3,       %[temp2],          %[ab]     \n\t"
            "maq_s.w.phr    $ac3,       %[temp2],          %[ab]     \n\t"
            "bne            %[p],       %[p2],             1b        \n\t"
            " addiu         %[p1],      %[p1],             16        \n\t"
            "addiu          %[corr],    %[corr],           16        \n\t"
            "mflo           %[temp1],   $ac0                         \n\t"
            "mflo           %[temp2],   $ac1                         \n\t"
            "mflo           %[a1],      $ac2                         \n\t"
            "mflo           %[b1],      $ac3                         \n\t"
            "sw             %[temp1],   -16(%[corr])                 \n\t"
            "sw             %[temp2],   -12(%[corr])                 \n\t"
            "sw             %[a1],      -8(%[corr])                  \n\t"
            "sw             %[b1],      -4(%[corr])                  \n\t"
            ".set pop                                                \n\t"

            : [temp1]"=&r"(temp1), [temp2]"=&r"(temp2), [a]"=&r"(a), [b]"=&r"(b),
              [ab]"=&r"(ab), [a1]"=&r"(a1), [b1]"=&r"(b1), [a1b]"=&r"(a1b),
              [a1b1]"=&r"(a1b1), [p1]"+r"(p1), [p]"+r"(p), [corr]"+r"(corr)
            : [p2]"r"(p2)
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo", "$ac3hi",
              "$ac3lo", "memory"
        );

    }
#elif (MIPS32_R2_LE)
    for (i = ((lag_max - lag_min) >> 2) + 1; i > 0; i--)
    {
        Word16 *p ;
        Word16 *p1;
        Word32 p1_temp_0;
        Word32 p1_temp_1;
        Word32 p1_temp_2;
        Word32 p1_temp_3;
        Word32 p_temp_0 ;
        Word32 p_temp_1 ;
        Word32 temp1, temp2, temp3, temp4;
        __asm__ volatile (
            ".set push                                                  \n\t"
            ".set noreorder                                             \n\t"
            "move       %[p],          %[scal_sig]                      \n\t"
            "move       %[p1],         %[p_scal_sig]                    \n\t"
            "addiu      %[p_scal_sig], %[p_scal_sig],  8                \n\t"
            "move       %[t1],         $zero                            \n\t"
            "move       %[t2],         $zero                            \n\t"
            "move       %[t3],         $zero                            \n\t"
            "move       %[t4],         $zero                            \n\t"
            "move       %[j],          %[temp_loop]                     \n\t"
            "lh         %[p_temp_0],   0(%[p])                          \n\t"
            "lh         %[p_temp_1],   2(%[p])                          \n\t"
            "lh         %[p1_temp_0],  0(%[p1])                         \n\t"
            "lh         %[p1_temp_1],  2(%[p1])                         \n\t"
            "lh         %[p1_temp_2],  4(%[p1])                         \n\t"
            "lh         %[p1_temp_3],  6(%[p1])                         \n\t"
        "11:                                                            \n\t"
            "mul        %[temp1],      %[p_temp_0],    %[p1_temp_0]     \n\t"
            "mul        %[temp2],      %[p_temp_0],    %[p1_temp_1]     \n\t"
            "mul        %[temp3],      %[p_temp_0],    %[p1_temp_2]     \n\t"
            "mul        %[temp4],      %[p_temp_0],    %[p1_temp_3]     \n\t"
            "lh         %[p1_temp_0],  8(%[p1])                         \n\t"
            "lh         %[p_temp_0],   4(%[p])                          \n\t"
            "addiu      %[j],          %[j],           -2               \n\t"
            "addu       %[t1],         %[t1],          %[temp1]         \n\t"
            "addu       %[t2],         %[t2],          %[temp2]         \n\t"
            "addu       %[t3],         %[t3],          %[temp3]         \n\t"
            "addu       %[t4],         %[t4],          %[temp4]         \n\t"
            "mul        %[temp1],      %[p_temp_1],    %[p1_temp_1]     \n\t"
            "mul        %[temp2],      %[p_temp_1],    %[p1_temp_2]     \n\t"
            "mul        %[temp3],      %[p_temp_1],    %[p1_temp_3]     \n\t"
            "mul        %[temp4],      %[p_temp_1],    %[p1_temp_0]     \n\t"
            "lh         %[p_temp_1],   6(%[p])                          \n\t"
            "lh         %[p1_temp_1],  10(%[p1])                        \n\t"
            "addiu      %[p],          %[p],           8                \n\t"
            "addu       %[t1],         %[t1],          %[temp1]         \n\t"
            "mul        %[temp1],      %[p_temp_0],    %[p1_temp_2]     \n\t"
            "addu       %[t2],         %[t2],          %[temp2]         \n\t"
            "mul        %[temp2],      %[p_temp_0],    %[p1_temp_3]     \n\t"
            "addu       %[t3],         %[t3],          %[temp3]         \n\t"
            "addu       %[t4],         %[t4],          %[temp4]         \n\t"
            "lh         %[p1_temp_2],  12(%[p1])                        \n\t"
            "addiu      %[p1],         %[p1],          8                \n\t"
            "addu       %[t1],         %[t1],          %[temp1]         \n\t"
            "mul        %[temp1],      %[p_temp_1],    %[p1_temp_3]     \n\t"
            "addu       %[t2],         %[t2],          %[temp2]         \n\t"
            "lh         %[p1_temp_3],  6(%[p1])                         \n\t"
            "mul        %[temp2],      %[p_temp_1],    %[p1_temp_0]     \n\t"
            "mul        %[temp3],      %[p_temp_1],    %[p1_temp_1]     \n\t"
            "mul        %[temp4],      %[p_temp_1],    %[p1_temp_2]     \n\t"
            "lh         %[p_temp_1],   2(%[p])                          \n\t"
            "addu       %[t1],         %[t1],          %[temp1]         \n\t"
            "addu       %[t2],         %[t2],          %[temp2]         \n\t"
            "addu       %[t3],         %[t3],          %[temp3]         \n\t"
            "addu       %[t4],         %[t4],          %[temp4]         \n\t"
            "mul        %[temp3],      %[p_temp_0],    %[p1_temp_0]     \n\t"
            "mul        %[temp4],      %[p_temp_0],    %[p1_temp_1]     \n\t"
            "lh         %[p_temp_0],   0(%[p])                          \n\t"
            "addu       %[t3],         %[t3],          %[temp3]         \n\t"
            "bgtz       %[j],          11b                              \n\t"
            " addu      %[t4],         %[t4],          %[temp4]         \n\t"
            "sll        %[t1],         %[t1],          1                \n\t"
            "sll        %[t2],         %[t2],          1                \n\t"
            "sll        %[t3],         %[t3],          1                \n\t"
            "sll        %[t4],         %[t4],          1                \n\t"
            "sw         %[t1],         0(%[corr])                       \n\t"
            "sw         %[t2],         4(%[corr])                       \n\t"
            "sw         %[t3],         8(%[corr])                       \n\t"
            "sw         %[t4],         12(%[corr])                      \n\t"
            "addiu      %[corr],       %[corr],        16               \n\t"
            ".set pop                                                   \n\t"
            : [p1_temp_0] "=&r" (p1_temp_0), [p1_temp_1] "=&r" (p1_temp_1),
              [p1_temp_2] "=&r" (p1_temp_2), [p1_temp_3] "=&r" (p1_temp_3),
              [p_temp_0] "=&r" (p_temp_0), [p_temp_1] "=&r" (p_temp_1),
              [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3), [t4]"=&r"(t4),
              [p]"=&r"(p), [p1]"=&r"(p1), [j]"=&r"(j), [corr]"+r"(corr),
              [p_scal_sig]"+r"(p_scal_sig),
              [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4)
            : [temp_loop] "r" (temp_loop), [scal_sig] "r" (scal_sig)
            : "hi", "lo", "memory"
        );
    }
#endif
    return;
}
