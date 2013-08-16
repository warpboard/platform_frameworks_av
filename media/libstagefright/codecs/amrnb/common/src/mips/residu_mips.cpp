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

 Pathname: ./audio/gsm-amr/c/src/residu.c

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Updated template used to PV coding template. First attempt at
          optimizing code.

 Description: Deleted stores listed in the Local Stores Needed/Modified
          section.

 Description: Updated file per comments gathered from Phase 2/3 review.

 Description: Updating to reflect variable name changes made in residu.asm

 Description: Synchronized file with UMTS version 3.2.0. Updated coding
              template. Removed unnecessary include files.

 Description: Made the following changes per comments from Phase 2/3 review:
              1. Modified FOR loops to count down.
              2. Fixed typecasting issue with TI C compiler.

 Description: Made the following changes
              1. Unrolled the convolutional loop.
              2. Performed 4 convolution per pass to avoid recalling the same
                 filter coefficient as many times.
              2. Eliminated math operations that check for saturation.

 Description:  Replaced "int" and/or "char" with OSCL defined types.

 Description: Changed round function name to pv_round to avoid conflict with
              round function in C standard library.

 Who:                           Date:
 Description:

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/
#include "residu.h"
#include "typedef.h"
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
 FUNCTION NAME: Residu
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    coef_ptr = pointer to buffer containing the prediction coefficients
    input_ptr = pointer to buffer containing the speech signal
    input_len = filter order
    residual_ptr = pointer to buffer of residual signal

 Outputs:
    residual_ptr buffer contains the newly calculated the residual signal

 Returns:
    None

 Global Variables Used:
    None

 Local Variables Needed:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

 This function computes the LP residual by filtering the input speech through
 the LP inverse filter A(z).

------------------------------------------------------------------------------
 REQUIREMENTS

 None

------------------------------------------------------------------------------
 REFERENCES

 residu.c, UMTS GSM AMR speech codec, R99 - Version 3.2.0, March 2, 2001

------------------------------------------------------------------------------
 PSEUDO-CODE

 Note: Input argument names were changed to be more descriptive. Shown below
       are the original names. Shown below are the name changes:
           a[]  <-->  coef_ptr[]
           x[]  <-->  input_ptr[]
           y[]  <-->  residual_ptr[]
           lg   <-->  input_len


void Residu (
    Word16 a[], // (i)     : prediction coefficients
    Word16 x[], // (i)     : speech signal
    Word16 y[], // (o)     : residual signal
    Word16 lg   // (i)     : size of filtering
)
{
    Word16 i, j;
    Word32 s;

    for (i = 0; i < lg; i++)
    {
        s = L_mult (x[i], a[0]);
        for (j = 1; j <= M; j++)
        {
            s = L_mac (s, a[j], x[i - j]);
        }
        s = L_shl (s, 3);
        y[i] = pv_round (s);
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

void Residu(
    Word16 coef_ptr[],      /* (i)     : prediction coefficients*/
    Word16 input_ptr[],     /* (i)     : speech signal          */
    Word16 residual_ptr[],  /* (o)     : residual signal        */
    Word16 input_len        /* (i)     : size of filtering      */
)
{
    register Word16 i;
    Word16 *p_input1;
    Word16 *p_coef;
    Word16 *p_residual_ptr = &residual_ptr[input_len-1];
    Word16 *p_input_ptr    = &input_ptr[input_len-1-M];

#if (MIPS_DSP_R2_LE)
    for (i = input_len >> 2; i != 0; i--)
    {
        Word32 temp0, temp1, temp2, temp3, temp4;
        p_coef  = &coef_ptr[M];
        p_input1 = p_input_ptr;
        p_input_ptr -=4;

         __asm__ volatile (
            "mult        $ac0,              $zero,                 $zero        \n\t"
            "mult        $ac1,              $zero,                 $zero        \n\t"
            "mult        $ac2,              $zero,                 $zero        \n\t"
            "mult        $ac3,              $zero,                 $zero        \n\t"
            "ulw         %[temp0],          -2(%[p_coef])                       \n\t"
            "ulw         %[temp1],          0(%[p_input1])                      \n\t"
            "ulw         %[temp2],          -2(%[p_input1])                     \n\t"
            "ulw         %[temp3],          -4(%[p_input1])                     \n\t"
            "ulw         %[temp4],          -6(%[p_input1])                     \n\t"
            "dpax.w.ph   $ac0,              %[temp0],              %[temp1]     \n\t"
            "dpax.w.ph   $ac1,              %[temp0],              %[temp2]     \n\t"
            "dpax.w.ph   $ac2,              %[temp0],              %[temp3]     \n\t"
            "dpax.w.ph   $ac3,              %[temp0],              %[temp4]     \n\t"
            "ulw         %[temp0],          -6(%[p_coef])                       \n\t"
            "ulw         %[temp3],          4(%[p_input1])                      \n\t"
            "ulw         %[temp4],          2(%[p_input1])                      \n\t"
            "dpax.w.ph   $ac2,              %[temp0],              %[temp1]     \n\t"
            "dpax.w.ph   $ac3,              %[temp0],              %[temp2]     \n\t"
            "dpax.w.ph   $ac0,              %[temp0],              %[temp3]     \n\t"
            "dpax.w.ph   $ac1,              %[temp0],              %[temp4]     \n\t"
            "ulw         %[temp0],          -10(%[p_coef])                      \n\t"
            "ulw         %[temp1],          8(%[p_input1])                      \n\t"
            "ulw         %[temp2],          6(%[p_input1])                      \n\t"
            "dpax.w.ph   $ac2,              %[temp0],              %[temp3]     \n\t"
            "dpax.w.ph   $ac3,              %[temp0],              %[temp4]     \n\t"
            "dpax.w.ph   $ac0,              %[temp0],              %[temp1]     \n\t"
            "dpax.w.ph   $ac1,              %[temp0],              %[temp2]     \n\t"
            "ulw         %[temp0],          -14(%[p_coef])                      \n\t"
            "ulw         %[temp3],          12(%[p_input1])                     \n\t"
            "ulw         %[temp4],          10(%[p_input1])                     \n\t"
            "dpax.w.ph   $ac2,              %[temp0],              %[temp1]     \n\t"
            "dpax.w.ph   $ac3,              %[temp0],              %[temp2]     \n\t"
            "dpax.w.ph   $ac0,              %[temp0],              %[temp3]     \n\t"
            "dpax.w.ph   $ac1,              %[temp0],              %[temp4]     \n\t"
            "ulw         %[temp0],          -18(%[p_coef])                      \n\t"
            "ulw         %[temp1],          16(%[p_input1])                     \n\t"
            "ulw         %[temp2],          14(%[p_input1])                     \n\t"
            "dpax.w.ph   $ac2,              %[temp0],              %[temp3]     \n\t"
            "dpax.w.ph   $ac3,              %[temp0],              %[temp4]     \n\t"
            "dpax.w.ph   $ac0,              %[temp0],              %[temp1]     \n\t"
            "dpax.w.ph   $ac1,              %[temp0],              %[temp2]     \n\t"
            "lh          %[temp0],          -20(%[p_coef])                      \n\t"
            "lh          %[temp1],          20(%[p_input1])                     \n\t"
            "lh          %[temp2],          18(%[p_input1])                     \n\t"
            "lh          %[temp3],          16(%[p_input1])                     \n\t"
            "lh          %[temp4],          14(%[p_input1])                     \n\t"
            "madd        $ac0,              %[temp0],              %[temp1]     \n\t"
            "madd        $ac1,              %[temp0],              %[temp2]     \n\t"
            "madd        $ac2,              %[temp0],              %[temp3]     \n\t"
            "madd        $ac3,              %[temp0],              %[temp4]     \n\t"
            "extr_r.w    %[temp0],          $ac0,                  12           \n\t"
            "extr_r.w    %[temp1],          $ac1,                  12           \n\t"
            "extr_r.w    %[temp2],          $ac2,                  12           \n\t"
            "extr_r.w    %[temp3],          $ac3,                  12           \n\t"
            "addiu       %[p_residual_ptr], %[p_residual_ptr],     -8           \n\t"
            "sh          %[temp0],          8(%[p_residual_ptr])                \n\t"
            "sh          %[temp1],          6(%[p_residual_ptr])                \n\t"
            "sh          %[temp2],          4(%[p_residual_ptr])                \n\t"
            "sh          %[temp3],          2(%[p_residual_ptr])                \n\t"

            : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [p_residual_ptr] "+r" (p_residual_ptr)
            : [p_input1] "r" (p_input1), [p_coef] "r" (p_coef)
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
              "$ac3hi", "$ac3lo", "memory"
         );
    }
#elif (MIPS_DSP_R1_LE)/*(MIPS_DSP_R1_LE)*/
    for (i = input_len >> 2; i != 0; i--)
    {
        p_coef  = &coef_ptr[M];
        p_input1 = p_input_ptr;
        p_input_ptr -=4;
        Word32 s1, s2, s3, s4;
        Word32 temp0, temp1, temp2, temp3, temp4;

        __asm__ volatile (
            "lh        %[temp1],          0(%[p_coef])                     \n\t"
            "lh        %[temp0],          -2(%[p_coef])                    \n\t"
            "lh        %[temp2],          2(%[p_input1])                   \n\t"
            "lh        %[temp3],          0(%[p_input1])                   \n\t"
            "lh        %[temp4],          -2(%[p_input1])                  \n\t"
            "lh        %[s1],             -4(%[p_input1])                  \n\t"
            "lh        %[s2],             -6(%[p_input1])                  \n\t"
            "mult      $ac0,              %[temp1],            %[temp3]    \n\t"
            "madd      $ac0,              %[temp0],            %[temp2]    \n\t"
            "mult      $ac1,              %[temp1],            %[temp4]    \n\t"
            "madd      $ac1,              %[temp0],            %[temp3]    \n\t"
            "mult      $ac2,              %[temp1],            %[s1]       \n\t"
            "madd      $ac2,              %[temp0],            %[temp4]    \n\t"
            "mult      $ac3,              %[temp1],            %[s2]       \n\t"
            "madd      $ac3,              %[temp0],            %[s1]       \n\t"
            "lh        %[s3],             4(%[p_input1])                   \n\t"
            "lh        %[temp1],          -4(%[p_coef])                    \n\t"
            "lh        %[temp0],          -6(%[p_coef])                    \n\t"
            "lh        %[s4],             6(%[p_input1])                   \n\t"
            "lh        %[s1],             12(%[p_input1])                  \n\t"
            "madd      $ac0,              %[temp1],            %[s3]       \n\t"
            "madd      $ac0,              %[temp0],            %[s4]       \n\t"
            "madd      $ac1,              %[temp1],            %[temp2]    \n\t"
            "madd      $ac1,              %[temp0],            %[s3]       \n\t"
            "madd      $ac2,              %[temp1],            %[temp3]    \n\t"
            "madd      $ac2,              %[temp0],            %[temp2]    \n\t"
            "madd      $ac3,              %[temp1],            %[temp4]    \n\t"
            "madd      $ac3,              %[temp0],            %[temp3]    \n\t"
            "lh        %[temp3],          8(%[p_input1])                   \n\t"
            "lh        %[temp1],          -8(%[p_coef])                    \n\t"
            "lh        %[temp0],          -10(%[p_coef])                   \n\t"
            "lh        %[temp4],          10(%[p_input1])                  \n\t"
            "lh        %[s2],             14(%[p_input1])                  \n\t"
            "madd      $ac0,              %[temp1],            %[temp3]    \n\t"
            "madd      $ac0,              %[temp0],            %[temp4]    \n\t"
            "madd      $ac1,              %[temp1],            %[s4]       \n\t"
            "madd      $ac1,              %[temp0],            %[temp3]    \n\t"
            "madd      $ac2,              %[temp1],            %[s3]       \n\t"
            "madd      $ac2,              %[temp0],            %[s4]       \n\t"
            "madd      $ac3,              %[temp1],            %[temp2]    \n\t"
            "madd      $ac3,              %[temp0],            %[s3]       \n\t"
            "lh        %[temp1],          -12(%[p_coef])                   \n\t"
            "lh        %[temp0],          -14(%[p_coef])                   \n\t"
            "lh        %[temp2],          16(%[p_input1])                  \n\t"
            "madd      $ac0,              %[temp1],            %[s1]       \n\t"
            "madd      $ac0,              %[temp0],            %[s2]       \n\t"
            "madd      $ac1,              %[temp1],            %[temp4]    \n\t"
            "madd      $ac1,              %[temp0],            %[s1]       \n\t"
            "madd      $ac2,              %[temp1],            %[temp3]    \n\t"
            "madd      $ac2,              %[temp0],            %[temp4]    \n\t"
            "madd      $ac3,              %[temp1],            %[s4]       \n\t"
            "madd      $ac3,              %[temp0],            %[temp3]    \n\t"
            "lh        %[temp1],          -16(%[p_coef])                   \n\t"
            "lh        %[temp0],          -18(%[p_coef])                   \n\t"
            "lh        %[temp3],          18(%[p_input1])                  \n\t"
            "addiu     %[p_residual_ptr], %[p_residual_ptr],   -8          \n\t"
            "madd      $ac0,              %[temp1],            %[temp2]    \n\t"
            "madd      $ac0,              %[temp0],            %[temp3]    \n\t"
            "madd      $ac1,              %[temp1],            %[s2]       \n\t"
            "madd      $ac1,              %[temp0],            %[temp2]    \n\t"
            "madd      $ac2,              %[temp1],            %[s1]       \n\t"
            "madd      $ac2,              %[temp0],            %[s2]       \n\t"
            "madd      $ac3,              %[temp1],            %[temp4]    \n\t"
            "madd      $ac3,              %[temp0],            %[s1]       \n\t"
            "lh        %[temp0],          -20(%[p_coef])                   \n\t"
            "lh        %[temp1],          20(%[p_input1])                  \n\t"
            "madd      $ac3,              %[temp0],            %[s2]       \n\t"
            "madd      $ac2,              %[temp0],            %[temp2]    \n\t"
            "madd      $ac1,              %[temp0],            %[temp3]    \n\t"
            "madd      $ac0,              %[temp0],            %[temp1]    \n\t"
            "extr_r.w  %[s4],             $ac3,                12          \n\t"
            "extr_r.w  %[s3],             $ac2,                12          \n\t"
            "extr_r.w  %[s2],             $ac1,                12          \n\t"
            "extr_r.w  %[s1],             $ac0,                12          \n\t"
            "sh        %[s4],             2(%[p_residual_ptr])             \n\t"
            "sh        %[s3],             4(%[p_residual_ptr])             \n\t"
            "sh        %[s2],             6(%[p_residual_ptr])             \n\t"
            "sh        %[s1],             8(%[p_residual_ptr])             \n\t"

            : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [s1] "=&r" (s1),
              [s2] "=&r" (s2), [s3] "=&r" (s3), [s4] "=&r" (s4),
              [p_residual_ptr] "+r" (p_residual_ptr)
            : [p_input1] "r" (p_input1), [p_coef] "r" (p_coef)
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo", "$ac3hi",
              "$ac3lo", "memory"
         );
    }
#elif (MIPS32_R2_LE)
    for (i = input_len >> 2; i != 0; i--)
    {
        p_coef  = &coef_ptr[M];
        p_input1 = p_input_ptr;
        p_input_ptr -=4;
        Word32 s1, s2, s3, s4;
        Word32 temp0, temp1, temp2, temp3, temp4;
        Word32 temp5, temp6, temp7, temp8, temp9;
        Word32 temp10, temp11, temp12;

        __asm__ volatile (
            "lh        %[temp1],          0(%[p_coef])                     \n\t"
            "lh        %[temp3],          0(%[p_input1])                   \n\t"
            "lh        %[temp4],          -2(%[p_input1])                  \n\t"
            "lh        %[temp5],          -4(%[p_input1])                  \n\t"
            "lh        %[temp6],          -6(%[p_input1])                  \n\t"
            "lh        %[temp0],          -2(%[p_coef])                    \n\t"
            "lh        %[temp2],          2(%[p_input1])                   \n\t"
            "mul       %[s1],             %[temp1],            %[temp3]    \n\t"
            "mul       %[s2],             %[temp1],            %[temp4]    \n\t"
            "mul       %[s3],             %[temp1],            %[temp5]    \n\t"
            "mul       %[s4],             %[temp1],            %[temp6]    \n\t"
            "mul       %[temp9],          %[temp0],            %[temp2]    \n\t"
            "mul       %[temp10],         %[temp0],            %[temp3]    \n\t"
            "mul       %[temp11],         %[temp0],            %[temp4]    \n\t"
            "mul       %[temp12],         %[temp0],            %[temp5]    \n\t"
            "lh        %[temp7],          4(%[p_input1])                   \n\t"
            "lh        %[temp1],          -4(%[p_coef])                    \n\t"
            "lh        %[temp8],          6(%[p_input1])                   \n\t"
            "addu      %[s1],             %[s1],               %[temp9]    \n\t"
            "addu      %[s2],             %[s2],               %[temp10]   \n\t"
            "addu      %[s3],             %[s3],               %[temp11]   \n\t"
            "addu      %[s4],             %[s4],               %[temp12]   \n\t"
            "mul       %[temp9],          %[temp1],            %[temp7]    \n\t"
            "mul       %[temp10],         %[temp1],            %[temp2]    \n\t"
            "mul       %[temp11],         %[temp1],            %[temp3]    \n\t"
            "mul       %[temp12],         %[temp1],            %[temp4]    \n\t"
            "lh        %[temp0],          -6(%[p_coef])                    \n\t"
            "lh        %[temp5],          12(%[p_input1])                  \n\t"
            "addu      %[s1],             %[s1],               %[temp9]    \n\t"
            "mul       %[temp9],          %[temp0],            %[temp8]    \n\t"
            "addu      %[s2],             %[s2],               %[temp10]   \n\t"
            "mul       %[temp10],         %[temp0],            %[temp7]    \n\t"
            "addu      %[s3],             %[s3],               %[temp11]   \n\t"
            "mul       %[temp11],         %[temp0],            %[temp2]    \n\t"
            "addu      %[s4],             %[s4],               %[temp12]   \n\t"
            "mul       %[temp12],         %[temp0],            %[temp3]    \n\t"
            "lh        %[temp3],          8(%[p_input1])                   \n\t"
            "lh        %[temp1],          -8(%[p_coef])                    \n\t"
            "lh        %[temp0],          -10(%[p_coef])                   \n\t"
            "addu      %[s1],             %[s1],               %[temp9]    \n\t"
            "mul       %[temp9],          %[temp1],            %[temp3]    \n\t"
            "addu      %[s2],             %[s2],               %[temp10]   \n\t"
            "mul       %[temp10],         %[temp1],            %[temp8]    \n\t"
            "addu      %[s3],             %[s3],               %[temp11]   \n\t"
            "mul       %[temp11],         %[temp1],            %[temp7]    \n\t"
            "addu      %[s4],             %[s4],               %[temp12]   \n\t"
            "mul       %[temp12],         %[temp1],            %[temp2]    \n\t"
            "lh        %[temp4],          10(%[p_input1])                  \n\t"
            "lh        %[temp6],          14(%[p_input1])                  \n\t"
            "addu      %[s1],             %[s1],               %[temp9]    \n\t"
            "mul       %[temp9],          %[temp0],            %[temp4]    \n\t"
            "addu      %[s2],             %[s2],               %[temp10]   \n\t"
            "mul       %[temp10],         %[temp0],            %[temp3]    \n\t"
            "addu      %[s3],             %[s3],               %[temp11]   \n\t"
            "mul       %[temp11],         %[temp0],            %[temp8]    \n\t"
            "addu      %[s4],             %[s4],               %[temp12]   \n\t"
            "mul       %[temp12],         %[temp0],            %[temp7]    \n\t"
            "lh        %[temp1],          -12(%[p_coef])                   \n\t"
            "lh        %[temp0],          -14(%[p_coef])                   \n\t"
            "lh        %[temp2],          16(%[p_input1])                  \n\t"
            "addu      %[s1],             %[s1],               %[temp9]    \n\t"
            "mul       %[temp9],          %[temp1],            %[temp5]    \n\t"
            "addu      %[s2],             %[s2],               %[temp10]   \n\t"
            "mul       %[temp10],         %[temp1],            %[temp4]    \n\t"
            "addu      %[s3],             %[s3],               %[temp11]   \n\t"
            "mul       %[temp11],         %[temp1],            %[temp3]    \n\t"
            "addu      %[s4],             %[s4],               %[temp12]   \n\t"
            "mul       %[temp12],         %[temp1],            %[temp8]    \n\t"
            "addu      %[s1],             %[s1],               %[temp9]    \n\t"
            "mul       %[temp9],          %[temp0],            %[temp6]    \n\t"
            "addu      %[s2],             %[s2],               %[temp10]   \n\t"
            "mul       %[temp10],         %[temp0],            %[temp5]    \n\t"
            "addu      %[s3],             %[s3],               %[temp11]   \n\t"
            "mul       %[temp11],         %[temp0],            %[temp4]    \n\t"
            "addu      %[s4],             %[s4],               %[temp12]   \n\t"
            "mul       %[temp12],         %[temp0],            %[temp3]    \n\t"
            "lh        %[temp1],          -16(%[p_coef])                   \n\t"
            "lh        %[temp0],          -18(%[p_coef])                   \n\t"
            "lh        %[temp3],          18(%[p_input1])                  \n\t"
            "addu      %[s1],             %[s1],               %[temp9]    \n\t"
            "mul       %[temp9],          %[temp1],            %[temp2]    \n\t"
            "addu      %[s2],             %[s2],               %[temp10]   \n\t"
            "mul       %[temp10],         %[temp1],            %[temp6]    \n\t"
            "addu      %[s3],             %[s3],               %[temp11]   \n\t"
            "mul       %[temp11],         %[temp1],            %[temp5]    \n\t"
            "addu      %[s4],             %[s4],               %[temp12]   \n\t"
            "mul       %[temp12],         %[temp1],            %[temp4]    \n\t"
            "addiu     %[p_residual_ptr], %[p_residual_ptr],   -8          \n\t"
            "addu      %[s1],             %[s1],               %[temp9]    \n\t"
            "mul       %[temp9],          %[temp0],            %[temp3]    \n\t"
            "addu      %[s2],             %[s2],               %[temp10]   \n\t"
            "mul       %[temp10],         %[temp0],            %[temp2]    \n\t"
            "addu      %[s3],             %[s3],               %[temp11]   \n\t"
            "mul       %[temp11],         %[temp0],            %[temp6]    \n\t"
            "addu      %[s4],             %[s4],               %[temp12]   \n\t"
            "mul       %[temp12],         %[temp0],            %[temp5]    \n\t"
            "lh        %[temp0],          -20(%[p_coef])                   \n\t"
            "lh        %[temp1],          20(%[p_input1])                  \n\t"
            "addu      %[s1],             %[s1],               %[temp9]    \n\t"
            "mul       %[temp9],          %[temp0],            %[temp6]    \n\t"
            "addu      %[s2],             %[s2],               %[temp10]   \n\t"
            "mul       %[temp10],         %[temp0],            %[temp2]    \n\t"
            "addu      %[s3],             %[s3],               %[temp11]   \n\t"
            "mul       %[temp11],         %[temp0],            %[temp3]    \n\t"
            "addu      %[s4],             %[s4],               %[temp12]   \n\t"
            "mul       %[temp12],         %[temp0],            %[temp1]    \n\t"
            "addu      %[s4],             %[s4],               %[temp9]    \n\t"
            "addu      %[s3],             %[s3],               %[temp10]   \n\t"
            "addu      %[s2],             %[s2],               %[temp11]   \n\t"
            "addu      %[s1],             %[s1],               %[temp12]   \n\t"
            "addiu     %[s4],             %[s4],               0x800       \n\t"
            "addiu     %[s3],             %[s3],               0x800       \n\t"
            "addiu     %[s2],             %[s2],               0x800       \n\t"
            "addiu     %[s1],             %[s1],               0x800       \n\t"
            "sra       %[s4],             %[s4],               12          \n\t"
            "sra       %[s3],             %[s3],               12          \n\t"
            "sra       %[s2],             %[s2],               12          \n\t"
            "sra       %[s1],             %[s1],               12          \n\t"
            "sh        %[s4],             2(%[p_residual_ptr])             \n\t"
            "sh        %[s3],             4(%[p_residual_ptr])             \n\t"
            "sh        %[s2],             6(%[p_residual_ptr])             \n\t"
            "sh        %[s1],             8(%[p_residual_ptr])             \n\t"

            : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [temp5] "=&r" (temp5),
              [temp6] "=&r" (temp6), [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
              [temp9] "=&r" (temp9), [temp10] "=&r" (temp10), [temp11] "=&r" (temp11),
              [temp12] "=&r" (temp12),
              [s1] "=&r" (s1), [s2] "=&r" (s2), [s3] "=&r" (s3), [s4] "=&r" (s4),
              [p_residual_ptr] "+r" (p_residual_ptr)
            : [p_input1] "r" (p_input1), [p_coef] "r" (p_coef)
            : "hi", "lo", "memory"
         );
    }
#endif
    return;
}