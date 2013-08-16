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

 Pathname: ./audio/gsm-amr/c/src/q_plsf_3.c
 Funtions: Vq_subvec4
           Test_Vq_subvec4
           Vq_subvec3
           Test_Vq_subvec3
           Q_plsf_3

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Updated template used to PV coding template. First attempt at
          optimizing C code.

 Description: Updated modules per Phase 2/3 review comments. Updated
          Vq_subvec3 pseudo-code to reflect the new restructured code.

 Description: Added setting of Overflow flag in inlined code.

 Description: Synchronized file with UMTS version 3.2.0. Updated coding
              template. Removed unnecessary include files.

 Description: Replaced basic_op.h with the header file of the math functions
              used in the file.

 Description: Made the following changes per comments from Phase 2/3 review:
              1. Fixed typecasting issue with TI C compiler.
              2. Optimized IF stament in Vq_subvec3() function.
              3. Updated copyright year.

 Description: Removed redundancy in the Vq_subvec4 function.

 Description: Updated to accept new parameter, Flag *pOverflow.

 Description: Per review comments, added pOverflow flag description
 to the input/outputs section.

 Description: Corrected missed Overflow global variables -- changed to
 proper pOverflow.

 Description: Optimized all functions to further reduce clock cycle usage.
              Updated copyright year.

 Description: Added left shift by 1 in line 1050 of Q_plsf_3().

 Description:  Replaced OSCL mem type functions and eliminated include
               files that now are chosen by OSCL definitions

 Description:  Replaced "int" and/or "char" with OSCL defined types.

 Description: Added #ifdef __cplusplus around extern'ed table.

 Who:                           Date:
 Description:

------------------------------------------------------------------------------
 MODULE DESCRIPTION

 This file contains the functions that perform the quantization of LSF
 parameters with first order MA prediction and split by 3 vector
 quantization (split-VQ).

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/

#include <string.h>

#include "q_plsf.h"
#include "typedef.h"
#include "lsp_lsf.h"
#include "reorder.h"
#include "lsfwt.h"
#include "q_plsf_3_mips.h"

/*
------------------------------------------------------------------------------
 FUNCTION NAME: Vq_subvec4
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    lsf_r1 = pointer to the first LSF residual vector (Q15) (Word16)
    dico = pointer to the quantization codebook (Q15) (const Word16)
    wf1 = pointer to the first LSF weighting factor (Q13) (Word16)
    dico_size = size of quantization codebook (Q0) (Word16)

 Outputs:
    buffer pointed to by lsf_r1 contains the selected vector
    pOverflow -- pointer to Flag -- Flag set when overflow occurs

 Returns:
    index = quantization index (Q0) (Word16)

 Global Variables Used:
    None

 Local Variables Needed:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

 This function performs the quantization of a 4-dimensional subvector.

------------------------------------------------------------------------------
 REQUIREMENTS

 None

------------------------------------------------------------------------------
 REFERENCES

 q_plsf_3.c, UMTS GSM AMR speech codec, R99 - Version 3.2.0, March 2, 2001

------------------------------------------------------------------------------
 PSEUDO-CODE

static Word16
Vq_subvec4(             // o: quantization index,            Q0
    Word16 * lsf_r1,    // i: 1st LSF residual vector,       Q15
    Word16 * dico,      // i: quantization codebook,         Q15
    Word16 * wf1,       // i: 1st LSF weighting factors,     Q13
    Word16 dico_size)   // i: size of quantization codebook, Q0
{
    Word16 i, index = 0;
    Word16 *p_dico, temp;
    Word32 dist_min, dist;

    dist_min = MAX_32;
    p_dico = dico;

    for (i = 0; i < dico_size; i++)
    {
        temp = sub (lsf_r1[0], *p_dico++);
        temp = mult (wf1[0], temp);
        dist = L_mult (temp, temp);

        temp = sub (lsf_r1[1], *p_dico++);
        temp = mult (wf1[1], temp);
        dist = L_mac (dist, temp, temp);

        temp = sub (lsf_r1[2], *p_dico++);
        temp = mult (wf1[2], temp);
        dist = L_mac (dist, temp, temp);

        temp = sub (lsf_r1[3], *p_dico++);
        temp = mult (wf1[3], temp);
        dist = L_mac (dist, temp, temp);


        if (L_sub (dist, dist_min) < (Word32) 0)
        {
            dist_min = dist;
            index = i;
        }
    }

    // Reading the selected vector

    p_dico = &dico[shl (index, 2)];
    lsf_r1[0] = *p_dico++;
    lsf_r1[1] = *p_dico++;
    lsf_r1[2] = *p_dico++;
    lsf_r1[3] = *p_dico;

    return index;

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

Word16 Vq_subvec4( /* o: quantization index,            Q0  */
    Word16 * lsf_r1,      /* i: 1st LSF residual vector,       Q15 */
    const Word16 * dico,  /* i: quantization codebook,         Q15 */
    Word16 * wf1,         /* i: 1st LSF weighting factors,     Q13 */
    Word16 dico_size,     /* i: size of quantization codebook, Q0  */
    Flag  *pOverflow      /* o : Flag set when overflow occurs     */
)
{
    register Word16 i;
    const Word16 *p_dico;
    Word16 index = 0;
    Word32 dist_min = MAX_32;
    p_dico = dico;

    OSCL_UNUSED_ARG(pOverflow);

#if (MIPS_DSP_R2_LE) || (MIPS_DSP_R1_LE)
    Word32 lsf_r1_01;
    Word32 lsf_r1_23;
    Word32 wf1_01;
    Word32 wf1_23;
    Word32 temp9, temp10, temp11, temp12;

    __asm__ volatile (
        "ulw    %[lsf_r1_01],   0(%[lsf_r1])                \n\t"
        "ulw    %[lsf_r1_23],   4(%[lsf_r1])                \n\t"
        "ulw    %[wf1_01],      0(%[wf1])                   \n\t"
        "ulw    %[wf1_23],      4(%[wf1])                   \n\t"
        "ulw    %[temp9],       0(%[p_dico])                \n\t"
        "ulw    %[temp10],      4(%[p_dico])                \n\t"
        "ulw    %[temp11],      8(%[p_dico])                \n\t"
        "ulw    %[temp12],      12(%[p_dico])               \n\t"

        :[lsf_r1_01] "=&r" (lsf_r1_01), [lsf_r1_23] "=&r" (lsf_r1_23),
         [wf1_01] "=&r" (wf1_01),[wf1_23] "=&r" (wf1_23),
         [temp9] "=&r" (temp9), [temp10] "=&r" (temp10),
         [temp11] "=&r" (temp11), [temp12] "=&r" (temp12)
        :[lsf_r1] "r" (lsf_r1), [wf1] "r" (wf1), [p_dico] "r" (p_dico)
        :"memory"
    );
#elif (MIPS32_R2_LE)
    Word16 temp;
    Word32 dist;

    Word16 lsf_r1_0;
    Word16 lsf_r1_1;
    Word16 lsf_r1_2;
    Word16 lsf_r1_3;

    Word16 wf1_0;
    Word16 wf1_1;
    Word16 wf1_2;
    Word16 wf1_3;

    lsf_r1_0 = lsf_r1[0];
    lsf_r1_1 = lsf_r1[1];
    lsf_r1_2 = lsf_r1[2];
    lsf_r1_3 = lsf_r1[3];
    wf1_0 = wf1[0];
    wf1_1 = wf1[1];
    wf1_2 = wf1[2];
    wf1_3 = wf1[3];
#endif


    for (i = 0; i < dico_size; i+=2)
    {
#if (MIPS_DSP_R2_LE)
        Word32  temp1, temp2, temp3, temp4,temp5, temp6;
        Word32  temp7, temp8;
        __asm__ volatile (
            "subq.ph        %[temp1],   %[lsf_r1_01],    %[temp9]    \n\t"
            "subq.ph        %[temp2],   %[lsf_r1_23],    %[temp10]   \n\t"
            "subq.ph        %[temp3],   %[lsf_r1_01],    %[temp11]   \n\t"
            "subq.ph        %[temp4],   %[lsf_r1_23],    %[temp12]   \n\t"
            "mulq_s.ph      %[temp1],   %[wf1_01],       %[temp1]    \n\t"
            "mulq_s.ph      %[temp2],   %[wf1_23],       %[temp2]    \n\t"
            "mulq_s.ph      %[temp3],   %[wf1_01],       %[temp3]    \n\t"
            "mulq_s.ph      %[temp4],   %[wf1_23],       %[temp4]    \n\t"
            "ulw            %[temp9],   16(%[p_dico])                \n\t"
            "ulw            %[temp10],  20(%[p_dico])                \n\t"
            "ulw            %[temp11],  24(%[p_dico])                \n\t"
            "muleq_s.w.phr  %[temp5],   %[temp1],        %[temp1]    \n\t"
            "muleq_s.w.phl  %[temp6],   %[temp1],        %[temp1]    \n\t"
            "muleq_s.w.phr  %[temp7],   %[temp2],        %[temp2]    \n\t"
            "muleq_s.w.phl  %[temp8],   %[temp2],        %[temp2]    \n\t"
            "muleq_s.w.phr  %[temp1],   %[temp3],        %[temp3]    \n\t"
            "muleq_s.w.phl  %[temp2],   %[temp3],        %[temp3]    \n\t"
            "muleq_s.w.phr  %[temp3],   %[temp4],        %[temp4]    \n\t"
            "muleq_s.w.phl  %[temp4],   %[temp4],        %[temp4]    \n\t"
            "ulw            %[temp12],  28(%[p_dico])                \n\t"
            "addu           %[temp5],   %[temp5],        %[temp6]    \n\t"
            "addu           %[temp7],   %[temp7],        %[temp8]    \n\t"
            "addu           %[temp5],   %[temp7],        %[temp5]    \n\t"
            "sra            %[temp5],   %[temp5],        1           \n\t"
            "addu           %[temp1],   %[temp1],        %[temp2]    \n\t"
            "addu           %[temp3],   %[temp3],        %[temp4]    \n\t"
            "addu           %[temp1],   %[temp3],        %[temp1]    \n\t"
            "sra            %[temp1],   %[temp1],        1           \n\t"
            "addiu          %[p_dico],  %[p_dico],       16          \n\t"

            : [p_dico] "+r" (p_dico), [temp6] "=&r" (temp6), [temp1] "=&r" (temp1),
              [temp2] "=&r" (temp2), [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [temp5] "=&r" (temp5), [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
              [temp9] "+r" (temp9), [temp10] "+r" (temp10), [temp11] "+r" (temp11),
              [temp12] "+r" (temp12)
            : [lsf_r1_01] "r" (lsf_r1_01), [lsf_r1_23] "r" (lsf_r1_23),
              [wf1_01] "r" (wf1_01), [wf1_23] "r" (wf1_23)
            :"memory", "hi", "lo"
        );
#elif (MIPS_DSP_R1_LE)/*(MIPS_DSP_R1_LE)*/
        Word32  temp1, temp2, temp3, temp4,temp5, temp6;
        Word32  temp7, temp8;
        __asm__ volatile (
            "subq.ph        %[temp1],   %[lsf_r1_01],    %[temp9]    \n\t"
            "subq.ph        %[temp2],   %[lsf_r1_23],    %[temp10]   \n\t"
            "subq.ph        %[temp3],   %[lsf_r1_01],    %[temp11]   \n\t"
            "subq.ph        %[temp4],   %[lsf_r1_23],    %[temp12]   \n\t"
            "muleq_s.w.phr  %[temp5],   %[wf1_01],       %[temp1]    \n\t"
            "muleq_s.w.phl  %[temp6],   %[wf1_01],       %[temp1]    \n\t"
            "muleq_s.w.phr  %[temp7],   %[wf1_23],       %[temp2]    \n\t"
            "muleq_s.w.phl  %[temp8],   %[wf1_23],       %[temp2]    \n\t"
            "muleq_s.w.phr  %[temp1],   %[wf1_01],       %[temp3]    \n\t"
            "muleq_s.w.phl  %[temp2],   %[wf1_01],       %[temp3]    \n\t"
            "muleq_s.w.phr  %[temp3],   %[wf1_23],       %[temp4]    \n\t"
            "muleq_s.w.phl  %[temp4],   %[wf1_23],       %[temp4]    \n\t"
            "muleq_s.w.phl  %[temp5],   %[temp5],        %[temp5]    \n\t"
            "muleq_s.w.phl  %[temp6],   %[temp6],        %[temp6]    \n\t"
            "muleq_s.w.phl  %[temp7],   %[temp7],        %[temp7]    \n\t"
            "muleq_s.w.phl  %[temp8],   %[temp8],        %[temp8]    \n\t"
            "muleq_s.w.phl  %[temp1],   %[temp1],        %[temp1]    \n\t"
            "muleq_s.w.phl  %[temp2],   %[temp2],        %[temp2]    \n\t"
            "muleq_s.w.phl  %[temp3],   %[temp3],        %[temp3]    \n\t"
            "muleq_s.w.phl  %[temp4],   %[temp4],        %[temp4]    \n\t"
            "ulw            %[temp9],   16(%[p_dico])                \n\t"
            "ulw            %[temp10],  20(%[p_dico])                \n\t"
            "ulw            %[temp11],  24(%[p_dico])                \n\t"
            "ulw            %[temp12],  28(%[p_dico])                \n\t"
            "addu           %[temp5],   %[temp5],        %[temp6]    \n\t"
            "addu           %[temp7],   %[temp7],        %[temp8]    \n\t"
            "addu           %[temp5],   %[temp7],        %[temp5]    \n\t"
            "sra            %[temp5],   %[temp5],        1           \n\t"
            "addu           %[temp1],   %[temp1],        %[temp2]    \n\t"
            "addu           %[temp3],   %[temp3],        %[temp4]    \n\t"
            "addu           %[temp1],   %[temp3],        %[temp1]    \n\t"
            "sra            %[temp1],   %[temp1],        1           \n\t"
            "addiu          %[p_dico],  %[p_dico],       16          \n\t"

           : [p_dico] "+r" (p_dico), [temp6] "=&r" (temp6), [temp1] "=&r" (temp1),
             [temp2] "=&r" (temp2), [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
             [temp5] "=&r" (temp5), [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
             [temp9] "+r" (temp9), [temp10] "+r" (temp10), [temp11] "+r" (temp11),
             [temp12] "+r" (temp12)
           : [lsf_r1_01] "r" (lsf_r1_01), [lsf_r1_23] "r" (lsf_r1_23),
             [wf1_01] "r" (wf1_01), [wf1_23] "r" (wf1_23)
           : "memory", "hi", "lo"
        );
#elif (MIPS32_R2_LE)
        Word32 temp1,temp2,temp3,temp4,temp5,temp6,temp7,dist1;

        __asm__ volatile (
            "lh         %[temp],   0(%[p_dico])                 \n\t"
            "lh         %[temp1],  8(%[p_dico])                 \n\t"
            "lh         %[temp2],  2(%[p_dico])                 \n\t"
            "lh         %[temp3],  10(%[p_dico])                \n\t"
            "lh         %[temp4],  4(%[p_dico])                 \n\t"
            "lh         %[temp5],  12(%[p_dico])                \n\t"
            "lh         %[temp6],  6(%[p_dico])                 \n\t"
            "lh         %[temp7],  14(%[p_dico])                \n\t"
            "subu       %[temp],   %[lsf_r1_0],      %[temp]    \n\t"
            "subu       %[temp1],  %[lsf_r1_0],      %[temp1]   \n\t"
            "subu       %[temp2],  %[lsf_r1_1],      %[temp2]   \n\t"
            "subu       %[temp3],  %[lsf_r1_1],      %[temp3]   \n\t"
            "subu       %[temp4],  %[lsf_r1_2],      %[temp4]   \n\t"
            "subu       %[temp5],  %[lsf_r1_2],      %[temp5]   \n\t"
            "subu       %[temp6],  %[lsf_r1_3],      %[temp6]   \n\t"
            "subu       %[temp7],  %[lsf_r1_3],      %[temp7]   \n\t"
            "mul        %[temp],   %[wf1_0],         %[temp]    \n\t"
            "mul        %[temp1],  %[wf1_0],         %[temp1]   \n\t"
            "mul        %[temp2],  %[wf1_1],         %[temp2]   \n\t"
            "mul        %[temp3],  %[wf1_1],         %[temp3]   \n\t"
            "mul        %[temp4],  %[wf1_2],         %[temp4]   \n\t"
            "mul        %[temp5],  %[wf1_2],         %[temp5]   \n\t"
            "mul        %[temp6],  %[wf1_3],         %[temp6]   \n\t"
            "mul        %[temp7],  %[wf1_3],         %[temp7]   \n\t"
            "sra        %[temp],   %[temp],          15         \n\t"
            "sra        %[temp1],  %[temp1],         15         \n\t"
            "sra        %[temp2],  %[temp2],         15         \n\t"
            "sra        %[temp3],  %[temp3],         15         \n\t"
            "sra        %[temp4],  %[temp4],         15         \n\t"
            "sra        %[temp5],  %[temp5],         15         \n\t"
            "sra        %[temp6],  %[temp6],         15         \n\t"
            "sra        %[temp7],  %[temp7],         15         \n\t"
            "mul        %[temp],   %[temp],          %[temp]    \n\t"
            "mul        %[temp1],  %[temp1],         %[temp1]   \n\t"
            "mul        %[temp2],  %[temp2],         %[temp2]   \n\t"
            "mul        %[temp3],  %[temp3],         %[temp3]   \n\t"
            "mul        %[temp4],  %[temp4],         %[temp4]   \n\t"
            "mul        %[temp5],  %[temp5],         %[temp5]   \n\t"
            "mul        %[temp6],  %[temp6],         %[temp6]   \n\t"
            "mul        %[temp7],  %[temp7],         %[temp7]   \n\t"
            "addu       %[temp2],  %[temp2],         %[temp]    \n\t"
            "addu       %[temp1],  %[temp1],         %[temp3]   \n\t"
            "addu       %[temp2],  %[temp2],         %[temp4]   \n\t"
            "addu       %[temp1],  %[temp1],         %[temp5]   \n\t"
            "addu       %[temp5],  %[temp2],         %[temp6]   \n\t"
            "addu       %[temp1],  %[temp1],         %[temp7]   \n\t"
            "addiu      %[p_dico],  %[p_dico],       16          \n\t"

            : [p_dico] "+r" (p_dico), [temp1] "=&r" (temp1),  [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [temp5] "=&r" (temp5),
              [temp6] "=&r" (temp6), [temp7] "=&r" (temp7), [temp] "=&r" (temp)
            : [lsf_r1_0] "r" (lsf_r1_0), [lsf_r1_1] "r" (lsf_r1_1),
              [lsf_r1_2] "r" (lsf_r1_2), [lsf_r1_3] "r" (lsf_r1_3),
              [wf1_0] "r" (wf1_0), [wf1_1] "r" (wf1_1), [wf1_2] "r" (wf1_2),
              [wf1_3] "r" (wf1_3)
            : "memory", "hi", "lo"
        );
#endif
        if (temp5 < dist_min)
        {
            dist_min = temp5;
            index = i;
        }

        if (temp1 < dist_min)
        {
            dist_min = temp1;
            index = i+1;
        }
    }

    /* Reading the selected vector */

    p_dico = dico + (index << 2);
    lsf_r1[0] = p_dico[0];
    lsf_r1[1] = p_dico[1];
    lsf_r1[2] = p_dico[2];
    lsf_r1[3] = p_dico[3];
    lsf_r1 +=3;

    return(index);

}

/****************************************************************************/


/*
------------------------------------------------------------------------------
 FUNCTION NAME: Test_Vq_subvec4
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    lsf_r1 = pointer to the first LSF residual vector (Q15) (Word16)
    dico = pointer to the quantization codebook (Q15) (const Word16)
    wf1 = pointer to the first LSF weighting factor (Q13) (Word16)
    dico_size = size of quantization codebook (Q0) (Word16)

 Outputs:
    buffer pointed to by lsf_r1 contains the selected vector
    pOverflow -- pointer to Flag -- Flag set when overflow occurs

 Returns:
    index = quantization index (Q0) (Word16)

 Global Variables Used:
    None

 Local Variables Needed:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

 This function calls the static function Vq_subvec4. It is used for testing
 purposes only

------------------------------------------------------------------------------
 REQUIREMENTS

 None

------------------------------------------------------------------------------
 REFERENCES

 None

------------------------------------------------------------------------------
 PSEUDO-CODE


 CALL Vq_subvec4(lsf_r1 = lsf_r1
                 dico = dico
                 wf1 = wf1
                 dico_size = dico_size)
   MODIFYING(nothing)
   RETURNING(index = tst_index4)

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

/****************************************************************************/

Word16 Vq_subvec3( /* o: quantization index,            Q0  */
    Word16 * lsf_r1,      /* i: 1st LSF residual vector,       Q15 */
    const Word16 * dico,  /* i: quantization codebook,         Q15 */
    Word16 * wf1,         /* i: 1st LSF weighting factors,     Q13 */
    Word16 dico_size,     /* i: size of quantization codebook, Q0  */
    Flag use_half,        /* i: use every second entry in codebook */
    Flag  *pOverflow)     /* o : Flag set when overflow occurs     */
{
    register Word16 i;
    Word16 temp;


    const Word16 *p_dico;

    Word16 p_dico_index = 0;
    Word16 index = 0;

    Word32 dist_min;
    Word32 dist, dist1;

    Word16 lsf_r1_0;
    Word16 lsf_r1_1;
    Word16 lsf_r1_2;
    Word16 wf1_0;
    Word16 wf1_1;
    Word16 wf1_2;

#if (MIPS_DSP_R2_LE) || (MIPS_DSP_R1_LE)
    Word32 lsf_r1_01;
    Word32 lsf_r1_12;
    Word32 lsf_r1_20;
    Word32 wf1_01;
    Word32 wf1_12;
    Word32 wf1_20;
#endif
    dist_min = MAX_32;
    p_dico = dico;

    if (use_half != 0)
    {
        p_dico_index = 3;
        lsf_r1_0 = lsf_r1[0];
        lsf_r1_1 = lsf_r1[1];
        lsf_r1_2 = lsf_r1[2];

        wf1_0 = wf1[0];
        wf1_1 = wf1[1];
        wf1_2 = wf1[2];

        for (i = 0; i < dico_size; i++)
        {
            temp = lsf_r1_0 - (*p_dico++);
            temp = (Word16)((((Word32) wf1_0) * temp) >> 15);
            dist = ((Word32) temp) * temp;

            temp = lsf_r1_1 - (*p_dico++);
            temp = (Word16)((((Word32) wf1_1) * temp) >> 15);
            dist += ((Word32) temp) * temp;

            temp = lsf_r1_2 - (*p_dico++);
            temp = (Word16)((((Word32) wf1_2) * temp) >> 15);
            dist += ((Word32) temp) * temp;

            if (dist < dist_min)
            {
                dist_min = dist;
                index = i;
            }

            p_dico = p_dico + p_dico_index;
        }

        p_dico = dico + (3 * index);
        p_dico += (3 * index);

    }
    else
    {

#if (MIPS_DSP_R2_LE) || (MIPS_DSP_R1_LE)
        Word32 temp7, temp8, temp9;
        __asm__ volatile (
            "lh    %[lsf_r1_20],   4(%[lsf_r1])                \n\t"
            "ulw   %[lsf_r1_01],   0(%[lsf_r1])                \n\t"
            "lh    %[wf1_20],      4(%[wf1])                   \n\t"
            "ulw   %[wf1_01],      0(%[wf1])                   \n\t"
            "ulw   %[lsf_r1_12],   2(%[lsf_r1])                \n\t"
            "ulw   %[wf1_12],      2(%[wf1])                   \n\t"
            "ins   %[lsf_r1_20],   %[lsf_r1_01],   16, 16      \n\t"
            "ins   %[wf1_20],      %[wf1_01],      16, 16      \n\t"
            "ulw   %[temp7],       0(%[p_dico])                \n\t"
            "ulw   %[temp8],       4(%[p_dico])                \n\t"
            "ulw   %[temp9],       8(%[p_dico])                \n\t"

            :[lsf_r1_01] "=&r" (lsf_r1_01), [lsf_r1_12] "=&r" (lsf_r1_12),
             [lsf_r1_20] "=&r" (lsf_r1_20), [wf1_01] "=&r" (wf1_01),
             [wf1_12] "=&r" (wf1_12), [wf1_20] "=&r" (wf1_20),
             [temp7] "=&r" (temp7), [temp8] "=&r" (temp8), [temp9] "=&r" (temp9)
            :[lsf_r1] "r" (lsf_r1), [wf1] "r" (wf1), [p_dico] "r" (p_dico)
            :"memory"
        );
#endif

#if (MIPS_DSP_R2_LE)
        for (i = 0; i < dico_size; i+=2)
        {

           Word32  temp1, temp2, temp3, temp4, temp5, temp6;

            __asm__ volatile (
                "subq_s.ph       %[temp1],   %[lsf_r1_01],    %[temp7]      \n\t"
                "subq_s.ph       %[temp2],   %[lsf_r1_20],    %[temp8]      \n\t"
                "subq_s.ph       %[temp3],   %[lsf_r1_12],    %[temp9]      \n\t"
                "mulq_s.ph       %[temp1],   %[wf1_01],       %[temp1]      \n\t"
                "mulq_s.ph       %[temp2],   %[wf1_20],       %[temp2]      \n\t"
                "mulq_s.ph       %[temp3],   %[wf1_12],       %[temp3]      \n\t"
                "ulw             %[temp7],   12(%[p_dico])                  \n\t"
                "ulw             %[temp8],   16(%[p_dico])                  \n\t"
                "muleq_s.w.phr   %[temp4],   %[temp1],        %[temp1]      \n\t"
                "muleq_s.w.phl   %[temp5],   %[temp1],        %[temp1]      \n\t"
                "muleq_s.w.phr   %[temp6],   %[temp2],        %[temp2]      \n\t"
                "muleq_s.w.phl   %[temp1],   %[temp2],        %[temp2]      \n\t"
                "muleq_s.w.phr   %[temp2],   %[temp3],        %[temp3]      \n\t"
                "muleq_s.w.phl   %[temp3],   %[temp3],        %[temp3]      \n\t"
                "ulw             %[temp9],   20(%[p_dico])                  \n\t"
                "addiu           %[p_dico],  %[p_dico],       12            \n\t"
                "addu            %[temp4],   %[temp5],        %[temp4]      \n\t"
                "addu            %[temp4],   %[temp6],        %[temp4]      \n\t"
                "sra             %[temp4],   %[temp4],        1             \n\t"
                "addu            %[temp1],   %[temp2],        %[temp1]      \n\t"
                "addu            %[temp1],   %[temp3],        %[temp1]      \n\t"
                "sra             %[temp1],   %[temp1],        1             \n\t"

                : [p_dico] "+r" (p_dico),
                  [temp6] "=&r" (temp6), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                  [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [temp5] "=&r" (temp5),
                  [temp7] "+r" (temp7), [temp8] "+r" (temp8), [temp9] "+r" (temp9)
                : [lsf_r1_01] "r" (lsf_r1_01), [lsf_r1_12] "r" (lsf_r1_12),
                  [lsf_r1_20] "r" (lsf_r1_20), [wf1_01] "r" (wf1_01),
                  [wf1_12] "r" (wf1_12), [wf1_20] "r" (wf1_20)
                : "memory", "hi", "lo"
            );

            if (temp4 < dist_min)
            {
                dist_min = temp4;
                index = i;
            }

            if (temp1 < dist_min)
            {
                dist_min = temp1;
                index = i+1;
            }
        }
#elif (MIPS_DSP_R1_LE)/*(MIPS_DSP_R1_LE)*/
        for (i = 0; i < dico_size; i+=2)
        {

            Word32  temp1, temp2, temp3, temp4, temp5, temp6;

            __asm__ volatile (
                "subq_s.ph       %[temp1],    %[lsf_r1_01],    %[temp7]    \n\t"
                "subq_s.ph       %[temp2],    %[lsf_r1_20],    %[temp8]    \n\t"
                "subq_s.ph       %[temp3],    %[lsf_r1_12],    %[temp9]    \n\t"
                "muleq_s.w.phr   %[temp7],    %[wf1_01],       %[temp1]    \n\t"
                "muleq_s.w.phl   %[temp8],    %[wf1_01],       %[temp1]    \n\t"
                "muleq_s.w.phr   %[temp9],    %[wf1_20],       %[temp2]    \n\t"
                "muleq_s.w.phl   %[temp1],    %[wf1_20],       %[temp2]    \n\t"
                "muleq_s.w.phr   %[temp2],    %[wf1_12],       %[temp3]    \n\t"
                "muleq_s.w.phl   %[temp3],    %[wf1_12],       %[temp3]    \n\t"
                "muleq_s.w.phl   %[temp4],    %[temp7],        %[temp7]    \n\t"
                "muleq_s.w.phl   %[temp5],    %[temp8],        %[temp8]    \n\t"
                "muleq_s.w.phl   %[temp6],    %[temp9],        %[temp9]    \n\t"
                "muleq_s.w.phl   %[temp1],    %[temp1],        %[temp1]    \n\t"
                "muleq_s.w.phl   %[temp2],    %[temp2],        %[temp2]    \n\t"
                "muleq_s.w.phl   %[temp3],    %[temp3],        %[temp3]    \n\t"
                "ulw             %[temp7],    12(%[p_dico])                \n\t"
                "ulw             %[temp8],    16(%[p_dico])                \n\t"
                "ulw             %[temp9],    20(%[p_dico])                \n\t"
                "addiu           %[p_dico],   %[p_dico],       12          \n\t"
                "addu            %[temp4],    %[temp5],        %[temp4]    \n\t"
                "addu            %[temp4],    %[temp6],        %[temp4]    \n\t"
                "sra             %[temp4],    %[temp4],        1           \n\t"
                "addu            %[temp1],    %[temp2],        %[temp1]    \n\t"
                "addu            %[temp1],    %[temp3],        %[temp1]    \n\t"
                "sra             %[temp1],    %[temp1],        1           \n\t"

                : [p_dico] "+r" (p_dico), [temp6] "=&r" (temp6), [temp1] "=&r" (temp1),
                  [temp2] "=&r" (temp2), [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
                  [temp5] "=&r" (temp5), [temp7] "+r" (temp7), [temp8] "+r" (temp8),
                  [temp9] "+r" (temp9)
                : [lsf_r1_01] "r" (lsf_r1_01), [lsf_r1_12] "r" (lsf_r1_12),
                  [lsf_r1_20] "r" (lsf_r1_20), [wf1_01] "r" (wf1_01),
                  [wf1_12] "r" (wf1_12), [wf1_20] "r" (wf1_20)
                : "memory", "hi", "lo"
            );

            if (temp4 < dist_min)
            {
                dist_min = temp4;
                index = i;
            }

            if (temp1 < dist_min)
            {
                dist_min = temp1;
                index = i+1;
            }
        }
#elif (MIPS32_R2_LE)
        lsf_r1_0 = lsf_r1[0];
        lsf_r1_1 = lsf_r1[1];
        lsf_r1_2 = lsf_r1[2];

        wf1_0 = wf1[0];
        wf1_1 = wf1[1];
        wf1_2 = wf1[2];
        for (i = 0; i < dico_size; i+=2)
        {
            Word32 temp1,temp2,temp3,temp4,temp5,dist2;

            __asm__ volatile (
                "lh         %[temp],   0(%[p_dico])                 \n\t"
                "lh         %[temp1],  2(%[p_dico])                 \n\t"
                "lh         %[temp2],  4(%[p_dico])                 \n\t"
                "lh         %[temp3],  6(%[p_dico])                 \n\t"
                "lh         %[temp4],  8(%[p_dico])                 \n\t"
                "lh         %[temp5],  10(%[p_dico])                \n\t"
                "subu       %[temp],   %[lsf_r1_0],      %[temp]    \n\t"
                "subu       %[temp1],  %[lsf_r1_1],      %[temp1]   \n\t"
                "subu       %[temp2],  %[lsf_r1_2],      %[temp2]   \n\t"
                "subu       %[temp3],  %[lsf_r1_0],      %[temp3]   \n\t"
                "subu       %[temp4],  %[lsf_r1_1],      %[temp4]   \n\t"
                "subu       %[temp5],  %[lsf_r1_2],      %[temp5]   \n\t"
                "mul        %[temp],   %[wf1_0],         %[temp]    \n\t"
                "mul        %[temp1],  %[wf1_1],         %[temp1]   \n\t"
                "mul        %[temp2],  %[wf1_2],         %[temp2]   \n\t"
                "mul        %[temp3],  %[wf1_0],         %[temp3]   \n\t"
                "mul        %[temp4],  %[wf1_1],         %[temp4]   \n\t"
                "mul        %[temp5],  %[wf1_2],         %[temp5]   \n\t"
                "sra        %[temp],   %[temp],          15         \n\t"
                "sra        %[temp1],  %[temp1],         15         \n\t"
                "sra        %[temp2],  %[temp2],         15         \n\t"
                "sra        %[temp3],  %[temp3],         15         \n\t"
                "sra        %[temp4],  %[temp4],         15         \n\t"
                "sra        %[temp5],  %[temp5],         15         \n\t"
                "mul        %[temp],   %[temp],          %[temp]    \n\t"
                "mul        %[temp1],  %[temp1],         %[temp1]   \n\t"
                "mul        %[temp2],  %[temp2],         %[temp2]   \n\t"
                "mul        %[temp3],  %[temp3],         %[temp3]   \n\t"
                "mul        %[temp4],  %[temp4],         %[temp4]   \n\t"
                "mul        %[temp5],  %[temp5],         %[temp5]   \n\t"
                "addu       %[temp1],  %[temp1],         %[temp]    \n\t"
                "addu       %[temp1],  %[temp2],         %[temp1]   \n\t"
                "addu       %[temp3],  %[temp4],         %[temp3]   \n\t"
                "addu       %[temp3],  %[temp5],         %[temp3]   \n\t"

                : [p_dico] "+r" (p_dico), [temp1] "=&r" (temp1),
                  [temp2] "=&r" (temp2), [temp3] "=&r" (temp3),
                  [temp4] "=&r" (temp4), [temp5] "=&r" (temp5), [temp] "=&r" (temp)
                : [lsf_r1_0] "r" (lsf_r1_0), [lsf_r1_1] "r" (lsf_r1_1),
                  [lsf_r1_2] "r" (lsf_r1_2), [wf1_0] "r" (wf1_0),
                  [wf1_1] "r" (wf1_1), [wf1_2] "r" (wf1_2)
                : "memory", "hi", "lo"
            );

            if (temp1 < dist_min)
            {
                dist_min = temp1;
                index = i;
            }
            if (temp3 < dist_min)
            {
                dist_min = temp3;
                index = i+1;
            }

            p_dico = p_dico + 6;
        }
#endif

        p_dico = dico + (3 * index);
    }

    /* Reading the selected vector */
    lsf_r1[0] = p_dico[0];
    lsf_r1[1] = p_dico[1];
    lsf_r1[2] = p_dico[2];
    lsf_r1 +=2;
    p_dico +=2;

    return(index);
}