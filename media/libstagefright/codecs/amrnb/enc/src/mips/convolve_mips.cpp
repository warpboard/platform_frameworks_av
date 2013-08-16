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



 Pathname: ./audio/gsm-amr/c/src/convolve.c

     Date: 06/19/2000

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Optimize for speed. Update to code template.

 Description: Added author name and date, fixed tabs, and added missing
          sections. Updated Input/Output section.

 Description: Optimized code by calculating two convolution sums per iteration
          of the outer loop, thereby, decreasing outer loop count by 2.
          Updated input/output definitions to be the same as the assembly
          file (convolve.asm). Left Pseudo-code section blank.

 Description: Deleted semi-colon in the Pointers modified section.

 Description: Synchronized file with UMTS version 3.2.0. Updated coding
              template. Removed unnecessary include files.

 Description: Made the following changes per comments from Phase 2/3 review:
              1. Fixed typecasting issue with TI C compiler.
              2. Modified FOR loop to count down, wherever applicable.

 Description: Made the following changes
              1. Unrolled the correlation loop.
              2. Performed 2 correlation per pass per sample to avoid recalling
                 the same data twice.
              3. Eliminated math operations that check for saturation.

 Description:
              1. Modified loop counter, extra unrolling did speed up code

 Description:  Replaced "int" and/or "char" with OSCL defined types.

 Description: Using inlines from fxp_arithmetic.h .

 Description: Replacing fxp_arithmetic.h with basic_op.h.

 Description:

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/
#include "typedef.h"
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
; LOCAL STORE/BUFFER/POINTER DEFINITIONS
; Variable declaration - defined here and used outside this module
----------------------------------------------------------------------------*/


/*
------------------------------------------------------------------------------
 FUNCTION NAME: Convolve
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    x = pointer to input vector of L elements of type Word16
    h = pointer to the filter's impulse response vector of L elements
        of type Word16
    y = pointer to the output vector of L elements of type Word16 used for
        storing the convolution of x and h;
    L = Length of the convolution; type definition is Word16

 Outputs:
    y buffer contains the new convolution output

 Returns:
    None

 Global Variables Used:
    None

 Local Variables Needed:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

 Perform the convolution between two vectors x[] and h[] and write the result
 in the vector y[]. All vectors are of length L and only the first L samples
 of the convolution are computed.

 The convolution is given by:

    y[n] = sum_{i=0}^{n} x[i] h[n-i],        n=0,...,L-1

------------------------------------------------------------------------------
 REQUIREMENTS

 None

------------------------------------------------------------------------------
 REFERENCES

 convolve.c, UMTS GSM AMR speech codec, R99 - Version 3.2.0, March 2, 2001

------------------------------------------------------------------------------
 PSEUDO-CODE

void Convolve (
    Word16 x[],        // (i)     : input vector
    Word16 h[],        // (i)     : impulse response
    Word16 y[],        // (o)     : output vector
    Word16 L           // (i)     : vector size
)
{
    Word16 i, n;
    Word32 s;

    for (n = 0; n < L; n++)
    {
        s = 0;                  move32 ();
        for (i = 0; i <= n; i++)
        {
            s = L_mac (s, x[i], h[n - i]);
        }
        s = L_shl (s, 3);
        y[n] = extract_h (s);   move16 ();
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

void Convolve(
    Word16 x[],        /* (i)     : input vector                           */
    Word16 h[],        /* (i)     : impulse response                       */
    Word16 y[],        /* (o)     : output vector                          */
    Word16 L           /* (i)     : vector size                            */
)
{
    register Word16 i, n;
    Word16 *tmpH, *tmpX, *X_end;
    Word32 temp1, temp2, temp3, temp4;
    Word32 temp5, temp6, temp7, temp8;
#if (MIPS_DSP_R2_LE) ||  (MIPS_DSP_R1_LE)
    Word32 cond;
#elif (MIPS32_R2_LE)
    register Word16 i1, i2;
    Word32 temp9;
    Word32 s1, s2, s1t0, s1t1, s1t2, s1t3;
#endif

#if (MIPS_DSP_R2_LE)
    for (n = 1; n < L; n += 4)
    {
        tmpH = h + n - 4;
        tmpX = x;
        X_end = x + n - 1;

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"
            "ulw        %[temp1],   0(%[tmpX])                  \n\t"
            "ulw        %[temp2],   6(%[tmpH])                  \n\t"
            "ulw        %[temp3],   4(%[tmpX])                  \n\t"
            "ulw        %[temp4],   10(%[tmpH])                 \n\t"
            "subu       %[cond],    %[X_end],       %[tmpX]     \n\t"
            "mult       $ac1,       $0,             $0          \n\t"
            "dpax.w.ph  $ac1,       %[temp1],       %[temp2]    \n\t"
            "mult       $ac3,       $0,             $0          \n\t"
            "dpax.w.ph  $ac3,       %[temp1],       %[temp4]    \n\t"
            "dpax.w.ph  $ac3,       %[temp2],       %[temp3]    \n\t"
            "packrl.ph  %[temp5],   %[temp4],       %[temp2]    \n\t"
            "seh        %[temp2],   %[temp2]                    \n\t"
            "seh        %[temp3],   %[temp3]                    \n\t"
            "mult       $ac2,       $0,             $0          \n\t"
            "dpax.w.ph  $ac2,       %[temp1],       %[temp5]    \n\t"
            "madd       $ac2,       %[temp2],       %[temp3]    \n\t"
            "seh        %[temp1],   %[temp1]                    \n\t"
            "mult       $ac0,       $0,             $0          \n\t"
            "madd       $ac0,       %[temp1],       %[temp2]    \n\t"
            "blez       %[cond],    2f                          \n\t"
            " addiu     %[y],       %[y],           8           \n\t"
          "1:                                                   \n\t"
            "ulw        %[temp1],   -2(%[tmpH])                 \n\t"
            "ulw        %[temp2],   2(%[tmpH])                  \n\t"
            "ulw        %[temp3],   2(%[tmpX])                  \n\t"
            "ulw        %[temp4],   6(%[tmpX])                  \n\t"
            "ulw        %[temp5],   10(%[tmpX])                 \n\t"
            "ulw        %[temp6],   14(%[tmpX])                 \n\t"
            "addiu      %[tmpX],    %[tmpX],        8           \n\t"
            "addiu      %[cond],    %[cond],        -8          \n\t"
            "dpax.w.ph  $ac0,       %[temp1],       %[temp4]    \n\t"
            "dpax.w.ph  $ac0,       %[temp2],       %[temp3]    \n\t"
            "dpax.w.ph  $ac2,       %[temp1],       %[temp5]    \n\t"
            "dpax.w.ph  $ac2,       %[temp2],       %[temp4]    \n\t"
            "packrl.ph  %[temp3],   %[temp4],       %[temp3]    \n\t"
            "packrl.ph  %[temp4],   %[temp5],       %[temp4]    \n\t"
            "packrl.ph  %[temp5],   %[temp6],       %[temp5]    \n\t"
            "dpax.w.ph  $ac1,       %[temp1],       %[temp4]    \n\t"
            "dpax.w.ph  $ac1,       %[temp2],       %[temp3]    \n\t"
            "dpax.w.ph  $ac3,       %[temp1],       %[temp5]    \n\t"
            "dpax.w.ph  $ac3,       %[temp2],       %[temp4]    \n\t"
            "bgtz       %[cond],    1b                          \n\t"
            " addiu     %[tmpH],    %[tmpH],        -8          \n\t"
          "2:                                                   \n\t"
            "extr.w     %[temp1],   $ac0,           12          \n\t"
            "extr.w     %[temp2],   $ac1,           12          \n\t"
            "extr.w     %[temp3],   $ac2,           12          \n\t"
            "extr.w     %[temp4],   $ac3,           12          \n\t"
            "sh         %[temp1],   -8(%[y])                    \n\t"
            "sh         %[temp2],   -6(%[y])                    \n\t"
            "sh         %[temp3],   -4(%[y])                    \n\t"
            "sh         %[temp4],   -2(%[y])                    \n\t"
            ".set pop                                           \n\t"

            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
              [tmpX] "+r" (tmpX), [tmpH] "+r" (tmpH),
              [cond] "=&r" (cond), [y] "+r" (y)
            : [X_end] "r" (X_end)
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
              "$ac3hi", "$ac3lo", "memory"
        );
    }
#elif (MIPS_DSP_R1_LE)/*(MIPS_DSP_R1_LE)*/
    for (n = 1; n < L; n += 4)
    {
        tmpH = h + n - 4;
        tmpX = x;
        X_end = x + n - 1;

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"
            "lh         %[temp1],   0(%[tmpX])                  \n\t"
            "lh         %[temp2],   2(%[tmpX])                  \n\t"
            "lh         %[temp3],   4(%[tmpX])                  \n\t"
            "lh         %[temp4],   6(%[tmpX])                  \n\t"
            "lh         %[temp5],   6(%[tmpH])                  \n\t"
            "lh         %[temp6],   8(%[tmpH])                  \n\t"
            "lh         %[temp7],   10(%[tmpH])                 \n\t"
            "lh         %[temp8],   12(%[tmpH])                 \n\t"
            "subu       %[cond],    %[X_end],       %[tmpX]     \n\t"
            "mult       $ac0,       %[temp1],       %[temp5]    \n\t"
            "mult       $ac1,       %[temp1],       %[temp6]    \n\t"
            "madd       $ac1,       %[temp2],       %[temp5]    \n\t"
            "mult       $ac2,       %[temp1],       %[temp7]    \n\t"
            "madd       $ac2,       %[temp2],       %[temp6]    \n\t"
            "madd       $ac2,       %[temp3],       %[temp5]    \n\t"
            "mult       $ac3,       %[temp1],       %[temp8]    \n\t"
            "madd       $ac3,       %[temp2],       %[temp7]    \n\t"
            "madd       $ac3,       %[temp3],       %[temp6]    \n\t"
            "madd       $ac3,       %[temp4],       %[temp5]    \n\t"
            "blez       %[cond],    2f                          \n\t"
            " addiu     %[y],       %[y],           8           \n\t"
          "1:                                                   \n\t"
            "lh         %[temp1],   4(%[tmpH])                  \n\t"
            "lh         %[temp2],   2(%[tmpH])                  \n\t"
            "lh         %[temp3],   0(%[tmpH])                  \n\t"
            "lh         %[temp4],   -2(%[tmpH])                 \n\t"
            "lh         %[temp5],   2(%[tmpX])                  \n\t"
            "lh         %[temp6],   4(%[tmpX])                  \n\t"
            "lh         %[temp7],   6(%[tmpX])                  \n\t"
            "lh         %[temp8],   8(%[tmpX])                  \n\t"
            "addiu      %[cond],    %[cond],        -8          \n\t"
            "madd       $ac0,       %[temp1],       %[temp5]    \n\t"
            "madd       $ac0,       %[temp2],       %[temp6]    \n\t"
            "madd       $ac0,       %[temp3],       %[temp7]    \n\t"
            "madd       $ac0,       %[temp4],       %[temp8]    \n\t"
            "lh         %[temp5],   10(%[tmpX])                 \n\t"
            "madd       $ac1,       %[temp1],       %[temp6]    \n\t"
            "madd       $ac1,       %[temp2],       %[temp7]    \n\t"
            "madd       $ac1,       %[temp3],       %[temp8]    \n\t"
            "madd       $ac1,       %[temp4],       %[temp5]    \n\t"
            "lh         %[temp6],   12(%[tmpX])                 \n\t"
            "madd       $ac2,       %[temp1],       %[temp7]    \n\t"
            "madd       $ac2,       %[temp2],       %[temp8]    \n\t"
            "madd       $ac2,       %[temp3],       %[temp5]    \n\t"
            "madd       $ac2,       %[temp4],       %[temp6]    \n\t"
            "lh         %[temp7],   14(%[tmpX])                 \n\t"
            "addiu      %[tmpX],    %[tmpX],        8           \n\t"
            "madd       $ac3,       %[temp1],       %[temp8]    \n\t"
            "madd       $ac3,       %[temp2],       %[temp5]    \n\t"
            "madd       $ac3,       %[temp3],       %[temp6]    \n\t"
            "madd       $ac3,       %[temp4],       %[temp7]    \n\t"
            "bgtz       %[cond],    1b                          \n\t"
            " addiu     %[tmpH],    %[tmpH],        -8          \n\t"
          "2:                                                   \n\t"
            "extr.w     %[temp1],   $ac0,           12          \n\t"
            "extr.w     %[temp2],   $ac1,           12          \n\t"
            "extr.w     %[temp3],   $ac2,           12          \n\t"
            "extr.w     %[temp4],   $ac3,           12          \n\t"
            "sh         %[temp1],   -8(%[y])                    \n\t"
            "sh         %[temp2],   -6(%[y])                    \n\t"
            "sh         %[temp3],   -4(%[y])                    \n\t"
            "sh         %[temp4],   -2(%[y])                    \n\t"
            ".set pop                                           \n\t"

            : [temp1]"=&r"(temp1), [temp2]"=&r"(temp2), [temp3]"=&r"(temp3),
              [temp4]"=&r"(temp4), [temp5]"=&r"(temp5), [temp6]"=&r"(temp6),
              [temp7]"=&r"(temp7), [temp8]"=&r"(temp8), [tmpX]"+r"(tmpX),
              [tmpH]"+r"(tmpH), [cond]"=&r"(cond), [y]"+r"(y)
            : [X_end]"r"(X_end)
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo", "$ac3hi",
              "$ac3lo", "memory"
        );
    }
#elif (MIPS32_R2_LE)
    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"
        "lh         %[temp1],   0(%[x])                 \n\t"
        "lh         %[temp2],   2(%[x])                 \n\t"
        "lh         %[temp3],   0(%[h])                 \n\t"
        "lh         %[temp4],   2(%[h])                 \n\t"
        "mul        %[s1],      %[temp1],   %[temp3]    \n\t"
        "mult       %[temp1],   %[temp4]                \n\t"
        "madd       %[temp2],   %[temp3]                \n\t"
        "sra        %[s1],      %[s1],      12          \n\t"
        "lh         %[temp5],   4(%[x])                 \n\t"
        "lh         %[temp6],   6(%[x])                 \n\t"
        "mflo       %[s2]                               \n\t"
        "lh         %[temp7],   4(%[h])                 \n\t"
        "lh         %[temp8],   6(%[h])                 \n\t"
        "sra        %[s2],      %[s2],      12          \n\t"
        "sh         %[s1],      0(%[y])                 \n\t"
        "sh         %[s2],      2(%[y])                 \n\t"
        "mul        %[s1],      %[temp1],   %[temp7]    \n\t"
        "mul        %[s1t0],    %[temp2],   %[temp4]    \n\t"
        "mul        %[s1t1],    %[temp5],   %[temp3]    \n\t"
        "mult       %[temp1],   %[temp8]                \n\t"
        "madd       %[temp2],   %[temp7]                \n\t"
        "madd       %[temp5],   %[temp4]                \n\t"
        "madd       %[temp6],   %[temp3]                \n\t"
        "addu       %[s1],      %[s1t0],    %[s1]       \n\t"
        "addu       %[s1],      %[s1t1],    %[s1]       \n\t"
        "mflo       %[s2]                               \n\t"
        "sra        %[s1],      %[s1],      12          \n\t"
        "sh         %[s1],      4(%[y])                 \n\t"
        "sra        %[s2],      %[s2],      12          \n\t"
        "sh         %[s2],      6(%[y])                 \n\t"
        "addiu      %[y],       %[y],       8           \n\t"
        ".set pop                                       \n\t"
        : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2), [y] "+r" (y),
          [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [s1] "=&r" (s1),
          [s2] "=&r" (s2), [s1t0] "=&r" (s1t0), [s1t1] "=&r" (s1t1),
          [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
          [temp7] "=&r" (temp7), [temp8] "=&r" (temp8)
        : [h] "r" (h), [x] "r" (x)
        : "hi", "lo", "memory"
    );

    for (n = 5; n < L; n = n + 2)
    {
        h = h + n;
        i = (n - 1) >> 1;
        i1 = i >> 1;
        i2 = i & 0x0001;

        __asm__ volatile (
            "lh         %[temp1],   0(%[x])                 \n\t"
            "lh         %[s2],      0(%[h])                 \n\t"
            "lh         %[s1],      -2(%[h])                \n\t"
            "mult       %[s2],      %[temp1]                \n\t"
            "mul        %[s1],      %[temp1],   %[s1]       \n\t"
            "addiu      %[x],       %[x],       2           \n\t"
            "addiu      %[h],       %[h],       -2          \n\t"
          "1:                                               \n\t"
            "lh         %[temp1],   0(%[x])                 \n\t"
            "lh         %[temp2],   2(%[x])                 \n\t"
            "lh         %[temp3],   0(%[h])                 \n\t"
            "lh         %[temp4],   -2(%[h])                \n\t"
            "lh         %[s2],      -4(%[h])                \n\t"
            "mul        %[s1t0],    %[temp1],   %[temp4]    \n\t"
            "mul        %[s1t1],    %[temp2],   %[s2]       \n\t"
            "madd       %[temp1],   %[temp3]                \n\t"
            "madd       %[temp2],   %[temp4]                \n\t"
            "lh         %[temp1],   4(%[x])                 \n\t"
            "lh         %[temp2],   6(%[x])                 \n\t"
            "lh         %[temp3],   -6(%[h])                \n\t"
            "lh         %[temp4],   -8(%[h])                \n\t"
            "mul        %[s1t2],    %[temp1],   %[temp3]    \n\t"
            "mul        %[s1t3],    %[temp2],   %[temp4]    \n\t"
            "madd       %[temp1],   %[s2]                   \n\t"
            "madd       %[temp3],   %[temp2]                \n\t"
            "addiu      %[x],       %[x],       8           \n\t"
            "addiu      %[h],       %[h],       -8          \n\t"
            "addiu      %[i1],      %[i1],      -1          \n\t"
            "addu       %[s1],      %[s1t0],    %[s1]       \n\t"
            "addu       %[s1],      %[s1t1],    %[s1]       \n\t"
            "addu       %[s1],      %[s1t2],    %[s1]       \n\t"
            "addu       %[s1],      %[s1t3],    %[s1]       \n\t"
            "bnez       %[i1],      1b                      \n\t"
            "beq        %[i2],      $zero,      2f          \n\t"
            "lh         %[temp1],   0(%[x])                 \n\t"
            "lh         %[temp2],   2(%[x])                 \n\t"
            "lh         %[temp3],   0(%[h])                 \n\t"
            "lh         %[s2],      -2(%[h])                \n\t"
            "lh         %[temp4],   -4(%[h])                \n\t"
            "mul        %[s1t0],    %[temp1],   %[s2]       \n\t"
            "mul        %[s1t1],    %[temp2],   %[temp4]    \n\t"
            "madd       %[temp1],   %[temp3]                \n\t"
            "madd       %[temp2],   %[s2]                   \n\t"
            "addiu      %[x],       %[x],       4           \n\t"
            "addiu      %[h],       %[h],       -4          \n\t"
            "addu       %[s1],      %[s1t0],    %[s1]       \n\t"
            "addu       %[s1],      %[s1t1],    %[s1]       \n\t"
          "2:                                               \n\t"
            "lh         %[temp1],   0(%[x])                 \n\t"
            "madd       %[temp1],   %[temp4]                \n\t"
            "addiu      %[y],       %[y],       4           \n\t"
            "mflo       %[s2]                               \n\t"
            "sra        %[s1],      %[s1],      12          \n\t"
            "sh         %[s1],      -4(%[y])                \n\t"
            "sra        %[s2],      %[s2],      12          \n\t"
            "sh         %[s2],      -2(%[y])                \n\t"
            : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2), [x] "+r" (x),
              [h] "+r" (h), [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
              [s1t2] "=&r" (s1t2), [i1] "+r" (i1), [s1] "=&r" (s1),
              [s2] "=&r" (s2), [s1t0] "=&r" (s1t0), [s1t1] "=&r" (s1t1),
              [s1t3] "=&r" (s1t3), [y] "+r" (y)
            : [i2] "r" (i2)
            : "hi", "lo", "memory"
        );

        x = x - n;
    }
#endif

    return;
}
