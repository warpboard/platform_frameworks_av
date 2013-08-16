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
 Pathname: ./audio/gsm-amr/c/src/syn_filt.c

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Making changes based on comments from the review meeting.

 Description: Added typedef to Input/Output Definition section.

 Description: Synchronized file with UMTS version 3.2.0. Updated coding
              template.

 Description: Fixed typecasting issue with the TI C compiler.

 Description: Modified FOR loops to count down.

 Description: Modified FOR loop to count up again so that the correct values
              are stored in the tmp buffer. Updated copyright year.

 Description:
        - Modified for loop and introduced pointers to avoid adding
          offsets
        - Eliminated check for saturation given that the max values of input
          data and coefficients will not saturate the multiply and
          accumulation
        - eliminated memcpy to update history buffer in every pass. This is
          done now just updating the pointers.

 Description:
              1. Eliminated unused include files.
              2. Unrolled loops to process twice as many samples as before,
                 this saves on memory accesses to the vector coeff. a[] and
                 elements in the history buffer of this recursive filter

 Description:
              1. Added overflow check inside both loops. (this is needed just
                 to satisfy bit exactness on the decoder, a faster
                 implementation will add an extra shift, do the same,
                 but will not be bit exact, and it may have better audio
                 quality because will avoid clipping)
              2. Added include file for constant definitions

 Description:  Replaced OSCL mem type functions and eliminated include
               files that now are chosen by OSCL definitions

 Description:  Replaced "int" and/or "char" with OSCL defined types.

 Description: Changed round function name to pv_round to avoid conflict with
              round function in C standard library.

 Description: Using fxp_arithmetic.h that includes inline assembly functions
              for ARM and linux-arm.

 Description: Replacing fxp_arithmetic.h with basic_op.h.

 Who:                           Date:
 Description:

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/

#include <string.h>

#include    "syn_filt.h"
#include    "cnst.h"
#include    "basic_op.h"

#include    "basic_op.h"

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
 FUNCTION NAME: Syn_filt
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    a = buffer containing the prediction coefficients (Word16)  max 2^12
    x = input signal buffer (Word16)                            max 2^15
    y = output signal buffer (Word16)
    lg = size of filtering (Word16)
    mem = memory buffer associated with this filtering (Word16)
    update = flag to indicate memory update; 0=no update, 1=update memory
             (Word16)

 Outputs:
    mem buffer is changed to be the last M data points of the output signal
      if update was set to 1
    y buffer contains the newly calculated filter output

 Returns:
    None

 Global Variables Used:
    None

 Local Variables Needed:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

 Perform synthesis filtering through 1/A(z)

------------------------------------------------------------------------------
 REQUIREMENTS

 None

------------------------------------------------------------------------------
 REFERENCES

 syn_filt.c, UMTS GSM AMR speech codec, R99 - Version 3.2.0, March 2, 2001

------------------------------------------------------------------------------
 PSEUDO-CODE

void Syn_filt (
    Word16 a[],     // (i)     : a[M+1] prediction coefficients   (M=10)
    Word16 x[],     // (i)     : input signal
    Word16 y[],     // (o)     : output signal
    Word16 lg,      // (i)     : size of filtering
    Word16 mem[],   // (i/o)   : memory associated with this filtering.
    Word16 update   // (i)     : 0=no update, 1=update of memory.
)
{
    Word16 i, j;
    Word32 s;
    Word16 tmp[80];   // This is usually done by memory allocation (lg+M)
    Word16 *yy;

    // Copy mem[] to yy[]

    yy = tmp;

    for (i = 0; i < M; i++)
    {
        *yy++ = mem[i];
    }

    // Do the filtering.

    for (i = 0; i < lg; i++)
    {
        s = L_mult (x[i], a[0]);
        for (j = 1; j <= M; j++)
        {
            s = L_msu (s, a[j], yy[-j]);
        }
        s = L_shl (s, 3);
        *yy++ = pv_round (s);
    }

    for (i = 0; i < lg; i++)
    {
        y[i] = tmp[i + M];
    }

    // Update of memory if update==1

    if (update != 0)
    {
        for (i = 0; i < M; i++)
        {
            mem[i] = y[lg - M + i];
        }
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

void Syn_filt(
    Word16 a[],     /* (i)   : a[M+1] prediction coefficients   (M=10)  */
    Word16 x[],     /* (i)   : input signal                             */
    Word16 y[],     /* (o)   : output signal                            */
    Word16 lg,      /* (i)   : size of filtering   (40)                 */
    Word16 mem[],   /* (i/o) : memory associated with this filtering.   */
    Word16 update   /* (i)   : 0=no update, 1=update of memory.         */
)
{
    Word16 i;
    Word32 s1;
    Word32 s2;
    Word16 *p_a;
    Word16 *p_x;

    p_x  = x;

#if (MIPS_DSP_R2_LE)
    Word32 *p_y;
    Word32 s3;
    Word32 s4;
    Word32 *p_yy1 = (int*)mem;

    /* Do the filtering. */
    p_y  = (int*)y;

    p_a  = a + 1;
    Word32 a1 = a[0];
#elif (MIPS_DSP_R1_LE) || (MIPS32_R2_LE)
    Word16 *p_y;
    Word16 *p_yy1;
    Word16 tmp[2*M]; /* This is usually done by memory allocation (lg+M) */
    Word16 *yy;
    Word16 temp;
    /* Copy mem[] to yy[] */

    yy = tmp;

    memcpy(yy, mem, M*sizeof(Word16));

    yy = yy + M;

    /* Do the filtering. */

    p_y  = y;
    p_yy1 = &yy[-1];
#endif


#if (MIPS_DSP_R2_LE)
    {
        Word32 s1a, s1b, s1c, s1d, s1e;
        Word32 s2a, s2b, s2c, s2d, s2e;
        Word32 temp0, temp1, temp2, temp3, temp4;
        Word32 temp5, temp6, temp7, temp8;
        __asm__ volatile (
            ".set push                                                 \n\t"
            ".set noreorder                                            \n\t"
            "ulw            %[s1a],     0(%[p_a])                      \n\t"
            "ulw            %[s1b],     4(%[p_a])                      \n\t"
            "ulw            %[s1c],     8(%[p_a])                      \n\t"
            "ulw            %[s1d],     12(%[p_a])                     \n\t"
            "ulw            %[s1e],     16(%[p_a])                     \n\t"
            "packrl.ph      %[s2a],     %[s1b],        %[s1a]          \n\t"
            "packrl.ph      %[s2b],     %[s1c],        %[s1b]          \n\t"
            "packrl.ph      %[s2c],     %[s1d],        %[s1c]          \n\t"
            "packrl.ph      %[s2d],     %[s1e],        %[s1d]          \n\t"
            "packrl.ph      %[s2e],     %[s1a],        %[s1e]          \n\t"
            "li             %[i],       4                              \n\t"
            "ulw            %[temp0],   16(%[p_yy1])                   \n\t"
            "ulw            %[temp1],   12(%[p_yy1])                   \n\t"
            "ulw            %[temp2],   8(%[p_yy1])                    \n\t"
            "ulw            %[temp3],   4(%[p_yy1])                    \n\t"
            "ulw            %[temp4],   0(%[p_yy1])                    \n\t"
         "2:                                                           \n\t"
            "lh             %[s1],      0(%[p_x])                      \n\t"
            "lh             %[s2],      2(%[p_x])                      \n\t"
            "lh             %[s3],      4(%[p_x])                      \n\t"
            "mult           $ac0,       %[s1],       %[a1]             \n\t"
            "dpsx.w.ph      $ac0,       %[temp0],    %[s1a]            \n\t"
            "dpsx.w.ph      $ac0,       %[temp1],    %[s1b]            \n\t"
            "dpsx.w.ph      $ac0,       %[temp2],    %[s1c]            \n\t"
            "dpsx.w.ph      $ac0,       %[temp3],    %[s1d]            \n\t"
            "dpsx.w.ph      $ac0,       %[temp4],    %[s1e]            \n\t"
            "mult           $ac1,       %[s2],       %[a1]             \n\t"
            "dpsx.w.ph      $ac1,       %[temp0],    %[s2a]            \n\t"
            "dpsx.w.ph      $ac1,       %[temp1],    %[s2b]            \n\t"
            "dpsx.w.ph      $ac1,       %[temp2],    %[s2c]            \n\t"
            "extr_r.w       %[s1],      $ac0,        12                \n\t"
            "dpsx.w.ph      $ac1,       %[temp3],    %[s2d]            \n\t"
            "mult           $ac2,       %[s3],       %[a1]             \n\t"
            "dpsx.w.ph      $ac2,       %[temp3],    %[s1e]            \n\t"
            "dpsx.w.ph      $ac2,       %[temp2],    %[s1d]            \n\t"
            "dpsx.w.ph      $ac2,       %[temp1],    %[s1c]            \n\t"
            "dpsx.w.ph      $ac2,       %[temp0],    %[s1b]            \n\t"
            "shll_s.w       %[s1],      %[s1],       16                \n\t"
            "lh             %[s4],      6(%[p_x])                      \n\t"
            "sra            %[s1],      %[s1],       16                \n\t"
            "ins            %[temp4],   %[s1],       0,       16       \n\t"
            "dpsx.w.ph      $ac1,       %[temp4],    %[s2e]            \n\t"
            "mult           $ac3,       %[s4],       %[a1]             \n\t"
            "dpsx.w.ph      $ac3,       %[temp0],    %[s2b]            \n\t"
            "dpsx.w.ph      $ac3,       %[temp1],    %[s2c]            \n\t"
            "dpsx.w.ph      $ac3,       %[temp2],    %[s2d]            \n\t"
            "extr_r.w       %[s2],      $ac1,        12                \n\t"
            "lh             %[s1],      8(%[p_x])                      \n\t"
            "lh             %[s3],      10(%[p_x])                     \n\t"
            "lh             %[s4],      12(%[p_x])                     \n\t"
            "mult           $ac0,       %[s1],       %[a1]             \n\t"
            "dpsx.w.ph      $ac0,       %[temp0],    %[s1c]            \n\t"
            "dpsx.w.ph      $ac0,       %[temp1],    %[s1d]            \n\t"
            "shll_s.w       %[s2],      %[s2],       16                \n\t"
            "dpsx.w.ph      $ac0,       %[temp2],    %[s1e]            \n\t"
            "sra            %[s2],      %[s2],       16                \n\t"
            "ins            %[temp4],   %[s2],       16,      16       \n\t"
            "dpsx.w.ph      $ac2,       %[temp4],    %[s1a]            \n\t"
            "mult           $ac1,       %[s3],       %[a1]             \n\t"
            "dpsx.w.ph      $ac1,       %[temp1],    %[s2d]            \n\t"
            "usw            %[temp4],   0(%[p_y])                      \n\t"
            "extr_r.w       %[s3],      $ac2,        12                \n\t"
            "dpsx.w.ph      $ac3,       %[temp4],    %[s2a]            \n\t"
            "dpsx.w.ph      $ac0,       %[temp4],    %[s1b]            \n\t"
            "dpsx.w.ph      $ac1,       %[temp4],    %[s2b]            \n\t"
            "dpsx.w.ph      $ac1,       %[temp0],    %[s2c]            \n\t"
            "lh             %[s1],      14(%[p_x])                     \n\t"
            "shll_s.w       %[s3],      %[s3],       16                \n\t"
            "mult           $ac2,       %[s4],       %[a1]             \n\t"
            "sra            %[s3],      %[s3],       16                \n\t"
            "ins            %[temp3],   %[s3],       0,       16       \n\t"
            "dpsx.w.ph      $ac3,       %[temp3],    %[s2e]            \n\t"
            "dpsx.w.ph      $ac2,       %[temp4],    %[s1c]            \n\t"
            "dpsx.w.ph      $ac2,       %[temp0],    %[s1d]            \n\t"
            "dpsx.w.ph      $ac2,       %[temp1],    %[s1e]            \n\t"
            "extr_r.w       %[s4],      $ac3,        12                \n\t"
            "mult           $ac3,       %[s1],       %[a1]             \n\t"
            "dpsx.w.ph      $ac3,       %[temp4],    %[s2c]            \n\t"
            "dpsx.w.ph      $ac3,       %[temp0],    %[s2d]            \n\t"
            "lh             %[s2],      16(%[p_x])                     \n\t"
            "shll_s.w       %[s4],      %[s4],       16                \n\t"
            "lh             %[s3],      18(%[p_x])                     \n\t"
            "sra            %[s4],      %[s4],       16                \n\t"
            "ins            %[temp3],   %[s4],       16,      16       \n\t"
            "dpsx.w.ph      $ac0,       %[temp3],    %[s1a]            \n\t"
            "dpsx.w.ph      $ac1,       %[temp3],    %[s2a]            \n\t"
            "dpsx.w.ph      $ac2,       %[temp3],    %[s1b]            \n\t"
            "usw             %[temp3],   4(%[p_y])                     \n\t"
            "extr_r.w       %[s1],      $ac0,        12                \n\t"
            "mult           $ac0,       %[s2],       %[a1]             \n\t"
            "dpsx.w.ph      $ac0,       %[temp3],    %[s1c]            \n\t"
            "dpsx.w.ph      $ac0,       %[temp4],    %[s1d]            \n\t"
            "dpsx.w.ph      $ac0,       %[temp0],    %[s1e]            \n\t"
            "dpsx.w.ph      $ac3,       %[temp3],    %[s2b]            \n\t"
            "shll_s.w       %[s1],      %[s1],       16                \n\t"
            "sra            %[s1],      %[s1],       16                \n\t"
            "ins            %[temp2],   %[s1],       0,       16       \n\t"
            "dpsx.w.ph      $ac1,       %[temp2],    %[s2e]            \n\t"
            "extr_r.w       %[s2],      $ac1,        12                \n\t"
            "mult           $ac1,       %[s3],       %[a1]             \n\t"
            "dpsx.w.ph      $ac1,       %[temp3],    %[s2c]            \n\t"
            "dpsx.w.ph      $ac1,       %[temp4],    %[s2d]            \n\t"
            "shll_s.w       %[s2],      %[s2],       16                \n\t"
            "sra            %[s2],      %[s2],       16                \n\t"
            "ins            %[temp2],   %[s2],       16,      16       \n\t"
            "dpsx.w.ph      $ac2,       %[temp2],    %[s1a]            \n\t"
            "dpsx.w.ph      $ac3,       %[temp2],    %[s2a]            \n\t"
            "dpsx.w.ph      $ac0,       %[temp2],    %[s1b]            \n\t"
            "dpsx.w.ph      $ac1,       %[temp2],    %[s2b]            \n\t"
            "usw            %[temp2],   8(%[p_y])                      \n\t"
            "extr_r.w       %[s3],      $ac2,        12                \n\t"
            "shll_s.w       %[s3],      %[s3],       16                \n\t"
            "sra            %[s3],      %[s3],       16                \n\t"
            "ins            %[temp1],   %[s3],       0,       16       \n\t"
            "dpsx.w.ph      $ac3,       %[temp1],    %[s2e]            \n\t"
            "extr_r.w       %[s4],      $ac3,        12                \n\t"
            "shll_s.w       %[s4],      %[s4],       16                \n\t"
            "sra            %[s4],      %[s4],       16                \n\t"
            "ins            %[temp1],   %[s4],       16,      16       \n\t"
            "dpsx.w.ph      $ac0,       %[temp1],    %[s1a]            \n\t"
            "dpsx.w.ph      $ac1,       %[temp1],    %[s2a]            \n\t"
            "usw            %[temp1],   12(%[p_y])                     \n\t"
            "extr_r.w       %[s1],      $ac0,        12                \n\t"
            "shll_s.w       %[s1],      %[s1],       16                \n\t"
            "sra            %[s1],      %[s1],       16                \n\t"
            "ins            %[temp0],   %[s1],       0,       16       \n\t"
            "dpsx.w.ph      $ac1,       %[temp0],    %[s2e]            \n\t"
            "extr_r.w       %[s2],      $ac1,        12                \n\t"
            "addiu          %[i],       %[i],        -1                \n\t"
            "addiu          %[p_x],     %[p_x],      20                \n\t"
            "shll_s.w       %[s2],      %[s2],       16                \n\t"
            "sra            %[s2],      %[s2],       16                \n\t"
            "ins            %[temp0],   %[s2],       16,      16       \n\t"
            "usw            %[temp0],   16(%[p_y])                     \n\t"
            "bnez           %[i],       2b                             \n\t"
            " addiu         %[p_y],     %[p_y],      20                \n\t"
            ".set pop                                                  \n\t"

            :[temp1] "=&r" (temp1), [temp2] "=&r" (temp2), [i] "=&r" (i),
             [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
             [temp0] "=&r" (temp0), [p_y] "+r" (p_y), [s1] "=&r" (s1),
             [s2] "=&r" (s2),[p_x] "+r" (p_x), [s3] "=&r" (s3), [s4] "=&r" (s4),
             [s1a] "=&r" (s1a), [s2a] "=&r" (s2a), [s2b] "=&r" (s2b), [s2c] "=&r" (s2c),
             [s2d] "=&r" (s2d), [s2e] "=&r" (s2e), [s1d] "=&r" (s1d),
             [s1e] "=&r" (s1e), [s1c] "=&r" (s1c), [s1b] "=&r" (s1b)
            :[a1] "r" (a1), [p_yy1] "r" (p_yy1), [p_a] "r" (p_a)
            :"memory", "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
        );
    }
#elif (MIPS_DSP_R1_LE)
    Word32 tmp_a0 = a[0];
    Word32 tmp_a1 = a[1];
    Word32 tmp_a2 = a[2];
    Word32 tmp_a3 = a[3];
    Word32 tmp_a4 = a[4];
    Word32 tmp_a5 = a[5];
    Word32 tmp_a6 = a[6];
    Word32 tmp_a7 = a[7];
    Word32 tmp_a8 = a[8];
    Word32 tmp_a9 = a[9];
    Word32 tmp_a10 = a[10];

    for (i = 5; i != 0; i--)
    {
        int temp1, temp2, temp3, temp4, temp5,temp6;

        __asm__ volatile (
            "lh             %[temp1],     0(%[p_x])                \n\t"
            "lh             %[temp3],     2(%[p_x])                \n\t"
            "lh             %[temp5],     0(%[p_yy1])              \n\t"
            "mult           $ac0,         %[temp1],    %[tmp_a0]   \n\t"
            "mult           $ac1,         %[temp3],    %[tmp_a0]   \n\t"
            "msub           $ac0,         %[tmp_a1],   %[temp5]    \n\t"
            "lh             %[temp3],     -2(%[p_yy1])             \n\t"
            "lh             %[temp4],     -4(%[p_yy1])             \n\t"
            "msub           $ac1,         %[tmp_a2],   %[temp5]    \n\t"
            "msub           $ac0,         %[tmp_a2],   %[temp3]    \n\t"
            "msub           $ac1,         %[tmp_a3],   %[temp3]    \n\t"
            "msub           $ac0,         %[tmp_a3],   %[temp4]    \n\t"
            "lh             %[temp5],     -6(%[p_yy1])             \n\t"
            "lh             %[temp6],     -8(%[p_yy1])             \n\t"
            "msub           $ac1,         %[tmp_a4],   %[temp4]    \n\t"
            "msub           $ac0,         %[tmp_a4],   %[temp5]    \n\t"
            "msub           $ac1,         %[tmp_a5],   %[temp5]    \n\t"
            "msub           $ac0,         %[tmp_a5],   %[temp6]    \n\t"
            "lh             %[temp3],     -10(%[p_yy1])            \n\t"
            "lh             %[temp4],     -12(%[p_yy1])            \n\t"
            "msub           $ac1,         %[tmp_a6],   %[temp6]    \n\t"
            "msub           $ac0,         %[tmp_a6],   %[temp3]    \n\t"
            "msub           $ac1,         %[tmp_a7],   %[temp3]    \n\t"
            "msub           $ac0,         %[tmp_a7],   %[temp4]    \n\t"
            "lh             %[temp5],     -14(%[p_yy1])            \n\t"
            "lh             %[temp6],     -16(%[p_yy1])            \n\t"
            "msub           $ac1,         %[tmp_a8],   %[temp4]    \n\t"
            "msub           $ac0,         %[tmp_a8],   %[temp5]    \n\t"
            "lh             %[temp3],     -18(%[p_yy1])            \n\t"
            "msub           $ac1,         %[tmp_a9],   %[temp5]    \n\t"
            "msub           $ac0,         %[tmp_a9],   %[temp6]    \n\t"
            "msub           $ac1,         %[tmp_a10],  %[temp6]    \n\t"
            "msub           $ac0,         %[tmp_a10],  %[temp3]    \n\t"
            "extr_r.w       %[temp3],     $ac0,        12          \n\t"
            "addiu          %[p_yy1],     %[yy],       2           \n\t"
            "addiu          %[p_x],       %[p_x],      4           \n\t"
            "addiu          %[yy],        %[yy],       4           \n\t"
            "addiu          %[p_y],       %[p_y],      4           \n\t"
            "shll_s.w       %[temp3],     %[temp3],    16          \n\t"
            "sra            %[temp1],     %[temp3],    16          \n\t"
            "msub           $ac1,         %[tmp_a1],   %[temp1]    \n\t"
            "extr_r.w       %[temp4],     $ac1,        12          \n\t"
            "sh             %[temp1],     -4(%[yy])                \n\t"
            "sh             %[temp1],     -4(%[p_y])               \n\t"
            "shll_s.w       %[temp4],     %[temp4],    16          \n\t"
            "sra            %[temp4],     %[temp4],    16          \n\t"
            "sh             %[temp4],     -2(%[yy])                \n\t"
            "sh             %[temp4],     -2(%[p_y])               \n\t"

            :[p_x] "+r" (p_x), [yy] "+r" (yy), [p_y] "+r" (p_y),
             [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
             [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
             [temp5] "=&r" (temp5), [temp6] "=&r" (temp6), [p_yy1] "+r" (p_yy1)
            :[a] "r" (a) ,[y] "r" (y),
             [tmp_a0] "r" (tmp_a0), [tmp_a1] "r" (tmp_a1),
             [tmp_a2] "r" (tmp_a2), [tmp_a3] "r" (tmp_a3),
             [tmp_a4] "r" (tmp_a4), [tmp_a5] "r" (tmp_a5),
             [tmp_a6] "r" (tmp_a6), [tmp_a7] "r" (tmp_a7),
             [tmp_a8] "r" (tmp_a8), [tmp_a9] "r" (tmp_a9),
             [tmp_a10] "r" (tmp_a10)
            :"memory", "hi", "lo", "$ac1hi", "$ac1lo"
        );
    }

    p_yy1 = &y[M-1];

    for (i = 15; i != 0; i--)
    {
        int temp1, temp2, temp3, temp4, temp5,temp6;

        __asm__ volatile (
            "lh             %[temp1],     0(%[p_x])                \n\t"
            "lh             %[temp2],     2(%[p_x])                \n\t"
            "lh             %[temp5],     0(%[p_yy1])              \n\t"
            "lh             %[temp3],     -2(%[p_yy1])             \n\t"
            "lh             %[temp4],     -4(%[p_yy1])             \n\t"
            "mult           $ac0,         %[temp1],    %[tmp_a0]   \n\t"
            "mult           $ac1,         %[temp2],    %[tmp_a0]   \n\t"
            "msub           $ac0,         %[tmp_a1],   %[temp5]    \n\t"
            "msub           $ac1,         %[tmp_a2],   %[temp5]    \n\t"
            "msub           $ac0,         %[tmp_a2],   %[temp3]    \n\t"
            "msub           $ac1,         %[tmp_a3],   %[temp3]    \n\t"
            "msub           $ac0,         %[tmp_a3],   %[temp4]    \n\t"
            "lh             %[temp5],     -6(%[p_yy1])             \n\t"
            "lh             %[temp6],     -8(%[p_yy1])             \n\t"
            "lh             %[temp1],     -10(%[p_yy1])            \n\t"
            "lh             %[temp2],     -12(%[p_yy1])            \n\t"
            "msub           $ac1,         %[tmp_a4],   %[temp4]    \n\t"
            "msub           $ac0,         %[tmp_a4],   %[temp5]    \n\t"
            "msub           $ac1,         %[tmp_a5],   %[temp5]    \n\t"
            "msub           $ac0,         %[tmp_a5],   %[temp6]    \n\t"
            "msub           $ac1,         %[tmp_a6],   %[temp6]    \n\t"
            "msub           $ac0,         %[tmp_a6],   %[temp1]    \n\t"
            "msub           $ac1,         %[tmp_a7],   %[temp1]    \n\t"
            "msub           $ac0,         %[tmp_a7],   %[temp2]    \n\t"
            "lh             %[temp5],     -14(%[p_yy1])            \n\t"
            "lh             %[temp6],     -16(%[p_yy1])            \n\t"
            "lh             %[temp3],     -18(%[p_yy1])            \n\t"
            "msub           $ac1,         %[tmp_a8],   %[temp2]    \n\t"
            "msub           $ac0,         %[tmp_a8],   %[temp5]    \n\t"
            "msub           $ac1,         %[tmp_a9],   %[temp5]    \n\t"
            "msub           $ac0,         %[tmp_a9],   %[temp6]    \n\t"
            "msub           $ac1,         %[tmp_a10],  %[temp6]    \n\t"
            "msub           $ac0,         %[tmp_a10],  %[temp3]    \n\t"
            "extr_r.w       %[temp3],     $ac0,        12          \n\t"
            "addiu          %[p_yy1],     %[p_y],      2           \n\t"
            "addiu          %[p_x],       %[p_x],      4           \n\t"
            "addiu          %[p_y],       %[p_y],      4           \n\t"
            "shll_s.w       %[temp3],     %[temp3],    16          \n\t"
            "sra            %[temp1],     %[temp3],    16          \n\t"
            "msub           $ac1,         %[tmp_a1],   %[temp1]    \n\t"
            "extr_r.w       %[temp4],     $ac1,        12          \n\t"
            "sh             %[temp1],     -4(%[p_y])               \n\t"
            "shll_s.w       %[temp4],     %[temp4],    16          \n\t"
            "sra            %[temp4],     %[temp4],    16          \n\t"
            "sh             %[temp4],     -2(%[p_y])               \n\t"

            :[p_x] "+r" (p_x), [p_y] "+r" (p_y), [p_yy1] "+r" (p_yy1),
             [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
             [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
             [temp5] "=&r" (temp5), [temp6] "=&r" (temp6)
            :[a] "r" (a) ,[y] "r" (y),
             [tmp_a0] "r" (tmp_a0), [tmp_a1] "r" (tmp_a1),
             [tmp_a2] "r" (tmp_a2), [tmp_a3] "r" (tmp_a3),
             [tmp_a4] "r" (tmp_a4), [tmp_a5] "r" (tmp_a5),
             [tmp_a6] "r" (tmp_a6), [tmp_a7] "r" (tmp_a7),
             [tmp_a8] "r" (tmp_a8), [tmp_a9] "r" (tmp_a9),
             [tmp_a10] "r" (tmp_a10)
            :"memory", "hi", "lo", "$ac1hi", "$ac1lo"
        );
    }
#elif (MIPS32_R2_LE)
    Word32 rnd = 0x800;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8, Temp9, Temp10;

    for (i = M >> 1; i != 0; i--)
    {
        p_a  = a;

        __asm__ volatile (
            "lh         %[Temp1],   2(%[p_x])               \n\t"
            "lh         %[Temp2],   0(%[p_x])               \n\t"
            "lh         %[Temp3],   0(%[p_a])               \n\t"
            "mtlo       %[rnd]                              \n\t"
            "move       %[s1],      %[rnd]                  \n\t"
            "mul        %[Temp10],  %[Temp2],   %[Temp3]    \n\t"
            "madd       %[Temp1],   %[Temp3]                \n\t"
            "lh         %[Temp1],   4(%[p_a])               \n\t"
            "lh         %[Temp2],   2(%[p_a])               \n\t"
            "lh         %[Temp3],   0(%[p_yy1])             \n\t"
            "lh         %[Temp4],   6(%[p_a])               \n\t"
            "lh         %[Temp5],   -2(%[p_yy1])            \n\t"
            "addu       %[s1],      %[s1],      %[Temp10]   \n\t"
            "lh         %[Temp7],   8(%[p_a])               \n\t"
            "lh         %[Temp8],   -4(%[p_yy1])            \n\t"
            "mul        %[Temp2],   %[Temp2],   %[Temp3]    \n\t"
            "mul        %[Temp6],   %[Temp1],   %[Temp5]    \n\t"
            "mul        %[Temp9],   %[Temp4],   %[Temp8]    \n\t"
            "msub       %[Temp1],   %[Temp3]                \n\t"
            "msub       %[Temp4],   %[Temp5]                \n\t"
            "msub       %[Temp7],   %[Temp8]                \n\t"
            "lh         %[Temp1],   10(%[p_a])              \n\t"
            "subu       %[s1],      %[s1],      %[Temp2]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp6]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp9]    \n\t"
            "lh         %[Temp2],   -6(%[p_yy1])            \n\t"
            "lh         %[Temp4],   12(%[p_a])              \n\t"
            "lh         %[Temp5],   -8(%[p_yy1])            \n\t"
            "lh         %[Temp8],   14(%[p_a])              \n\t"
            "lh         %[Temp9],   -10(%[p_yy1])           \n\t"
            "mul        %[Temp3],   %[Temp7],   %[Temp2]    \n\t"
            "mul        %[Temp6],   %[Temp1],   %[Temp5]    \n\t"
            "mul        %[Temp7],   %[Temp4],   %[Temp9]    \n\t"
            "msub       %[Temp1],   %[Temp2]                \n\t"
            "msub       %[Temp4],   %[Temp5]                \n\t"
            "msub       %[Temp8],   %[Temp9]                \n\t"
            "lh         %[Temp1],   16(%[p_a])              \n\t"
            "subu       %[s1],      %[s1],      %[Temp3]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp6]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp7]    \n\t"
            "lh         %[Temp2],   -12(%[p_yy1])           \n\t"
            "lh         %[Temp4],   18(%[p_a])              \n\t"
            "lh         %[Temp5],   -14(%[p_yy1])           \n\t"
            "lh         %[Temp7],   20(%[p_a])              \n\t"
            "lh         %[Temp9],   -16(%[p_yy1])           \n\t"
            "lh         %[Temp10],  -18(%[p_yy1])           \n\t"
            "mul        %[Temp3],   %[Temp8],   %[Temp2]    \n\t"
            "mul        %[Temp6],   %[Temp1],   %[Temp5]    \n\t"
            "mul        %[Temp8],   %[Temp4],   %[Temp9]    \n\t"
            "mul        %[Temp10],  %[Temp10],  %[Temp7]    \n\t"
            "msub       %[Temp1],   %[Temp2]                \n\t"
            "msub       %[Temp4],   %[Temp5]                \n\t"
            "msub       %[Temp7],   %[Temp9]                \n\t"
            "subu       %[s1],      %[s1],      %[Temp3]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp6]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp8]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp10]   \n\t"
            "mflo       %[s2]                               \n\t"

            : [s1] "=&r" (s1), [s2] "=&r" (s2),
              [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2), [Temp3] "=&r" (Temp3),
              [Temp4] "=&r" (Temp4), [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8), [Temp9] "=&r" (Temp9),
              [Temp10] "=&r" (Temp10)
            : [rnd] "r" (rnd), [p_a] "r" (p_a), [p_yy1] "r" (p_yy1), [p_x] "r" (p_x)
            : "hi", "lo", "memory"
        );

        p_a   += 9;
        p_yy1 -= 9;

        p_x += 2;

        /* check for overflow on s1 */
        if ((UWord32)(s1 - 0xf8000000L) < 0x0fffffffL)
        {
            temp = (Word16)(s1 >> 12);
        }
        else if (s1 > 0x07ffffffL)
        {
            temp = MAX_16;
        }
        else
        {
            temp = MIN_16;
        }

        s2 = amrnb_fxp_msu_16_by_16bb((Word32)a[1], (Word32)temp, s2);

        *(yy++)  = temp;
        *(p_y++) = temp;

        p_yy1 = yy;

        /* check for overflow on s2 */
        if ((UWord32)(s2 - 0xf8000000L) < 0x0fffffffL)
        {
            temp = (Word16)(s2 >> 12);
        }
        else if (s2 > 0x07ffffffL)
        {
            temp = MAX_16;
        }
        else
        {
            temp = MIN_16;
        }

        *(yy++)  = temp;
        *(p_y++) = temp;
    }

    p_yy1 = &y[M-1];

    for (i = (lg - M) >> 1; i != 0; i--)
    {
        p_a  = a;

        __asm__ volatile (
            "lh         %[Temp1],   2(%[p_x])               \n\t"
            "lh         %[Temp2],   0(%[p_x])               \n\t"
            "lh         %[Temp3],   0(%[p_a])               \n\t"
            "mtlo       %[rnd]                              \n\t"
            "move       %[s1],      %[rnd]                  \n\t"
            "mul        %[Temp10],  %[Temp2],   %[Temp3]    \n\t"
            "madd       %[Temp1],   %[Temp3]                \n\t"
            "lh         %[Temp1],   4(%[p_a])               \n\t"
            "lh         %[Temp2],   2(%[p_a])               \n\t"
            "lh         %[Temp3],   0(%[p_yy1])             \n\t"
            "lh         %[Temp4],   6(%[p_a])               \n\t"
            "lh         %[Temp5],   -2(%[p_yy1])            \n\t"
            "addu       %[s1],      %[s1],      %[Temp10]   \n\t"
            "lh         %[Temp7],   8(%[p_a])               \n\t"
            "lh         %[Temp8],   -4(%[p_yy1])            \n\t"
            "mul        %[Temp2],   %[Temp2],   %[Temp3]    \n\t"
            "mul        %[Temp6],   %[Temp1],   %[Temp5]    \n\t"
            "mul        %[Temp9],   %[Temp4],   %[Temp8]    \n\t"
            "msub       %[Temp1],   %[Temp3]                \n\t"
            "msub       %[Temp4],   %[Temp5]                \n\t"
            "msub       %[Temp7],   %[Temp8]                \n\t"
            "lh         %[Temp1],   10(%[p_a])              \n\t"
            "subu       %[s1],      %[s1],      %[Temp2]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp6]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp9]    \n\t"
            "lh         %[Temp2],   -6(%[p_yy1])            \n\t"
            "lh         %[Temp4],   12(%[p_a])              \n\t"
            "lh         %[Temp5],   -8(%[p_yy1])            \n\t"
            "lh         %[Temp8],   14(%[p_a])              \n\t"
            "lh         %[Temp9],   -10(%[p_yy1])           \n\t"
            "mul        %[Temp3],   %[Temp7],   %[Temp2]    \n\t"
            "mul        %[Temp6],   %[Temp1],   %[Temp5]    \n\t"
            "mul        %[Temp7],   %[Temp4],   %[Temp9]    \n\t"
            "msub       %[Temp1],   %[Temp2]                \n\t"
            "msub       %[Temp4],   %[Temp5]                \n\t"
            "msub       %[Temp8],   %[Temp9]                \n\t"
            "lh         %[Temp1],   16(%[p_a])              \n\t"
            "subu       %[s1],      %[s1],      %[Temp3]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp6]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp7]    \n\t"
            "lh         %[Temp2],   -12(%[p_yy1])           \n\t"
            "lh         %[Temp4],   18(%[p_a])              \n\t"
            "lh         %[Temp5],   -14(%[p_yy1])           \n\t"
            "lh         %[Temp7],   20(%[p_a])              \n\t"
            "lh         %[Temp9],   -16(%[p_yy1])           \n\t"
            "lh         %[Temp10],  -18(%[p_yy1])           \n\t"
            "mul        %[Temp3],   %[Temp8],   %[Temp2]    \n\t"
            "mul        %[Temp6],   %[Temp1],   %[Temp5]    \n\t"
            "mul        %[Temp8],   %[Temp4],   %[Temp9]    \n\t"
            "mul        %[Temp10],  %[Temp10],  %[Temp7]    \n\t"
            "msub       %[Temp1],   %[Temp2]                \n\t"
            "msub       %[Temp4],   %[Temp5]                \n\t"
            "msub       %[Temp7],   %[Temp9]                \n\t"
            "subu       %[s1],      %[s1],      %[Temp3]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp6]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp8]    \n\t"
            "subu       %[s1],      %[s1],      %[Temp10]   \n\t"
            "mflo       %[s2]                               \n\t"

            : [s1] "=&r" (s1), [s2] "=&r" (s2),
              [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2), [Temp3] "=&r" (Temp3),
              [Temp4] "=&r" (Temp4), [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8), [Temp9] "=&r" (Temp9),
              [Temp10] "=&r" (Temp10)
            : [rnd] "r" (rnd), [p_a] "r" (p_a), [p_yy1] "r" (p_yy1), [p_x] "r" (p_x)
            : "hi", "lo", "memory"
        );

        p_a   += 9;
        p_yy1 -= 9;

        p_x += 2;

        if ((UWord32)(s1 - 0xf8000000L) < 0x0fffffffL)
        {
            temp = (Word16)(s1 >> 12);
        }
        else if (s1 > 0x07ffffffL)
        {
            temp = MAX_16;
        }
        else
        {
            temp = MIN_16;
        }

        s2 = amrnb_fxp_msu_16_by_16bb((Word32)a[1], (Word32)temp, s2);

        *(p_y++) = temp;
        p_yy1 = p_y;

        if ((UWord32)(s2 - 0xf8000000L) < 0x0fffffffL)
        {
            *(p_y++) = (Word16)(s2 >> 12);
        }
        else if (s2 > 0x07ffffffL)
        {
            *(p_y++) = MAX_16;
        }
        else
        {
            *(p_y++) = MIN_16;
        }
    }
#endif


    /* Update of memory if update==1 */
    if (update != 0)
    {
        memcpy(mem, &y[lg-M], M*sizeof(Word16));
    }

    return;
}