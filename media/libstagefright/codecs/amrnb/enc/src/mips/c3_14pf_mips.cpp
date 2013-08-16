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



 Pathname: ./audio/gsm-amr/c/src/c3_14pf.c
 Functions:

     Date: 05/26/2000

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Modified to pass overflow flag through to basic math function.
 The flag is passed back to the calling function by pointer reference.

 Description: Optimized file to reduce clock cycle usage. Updated copyright
              year. Removed unneccesary include files and added only the
              include files for the math functions used. Removed unused
              #defines.

 Description: Changed round function name to pv_round to avoid conflict with
              round function in C standard library.

 Description:  Replaced "int" and/or "char" with OSCL defined types.

 Description:

------------------------------------------------------------------------------
 MODULE DESCRIPTION

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/
#include "c3_14pf.h"
#include "typedef.h"
#include "inv_sqrt.h"
#include "cnst.h"
#include "cor_h.h"
#include "set_sign.h"
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

#define NB_PULSE  3

/*----------------------------------------------------------------------------
; LOCAL FUNCTION DEFINITIONS
; Function Prototype declaration
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; LOCAL VARIABLE DEFINITIONS
; Variable declaration - defined here and used outside this module
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; EXTERNAL GLOBAL STORE/BUFFER/POINTER REFERENCES
; Declare variables used in this module but defined elsewhere
----------------------------------------------------------------------------*/

/*
------------------------------------------------------------------------------
 FUNCTION NAME: search_3i40
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    dn[]         Array of type Word16 -- correlation between target and h[]
    dn2[]        Array of type Word16 -- maximum of corr. in each track.
    rr[][L_CODE] Double Array of type Word16 -- autocorrelation matrix

 Outputs:
    codvec[]     Array of type Word16 -- algebraic codebook vector
    pOverflow    Pointer to Flag      -- set when overflow occurs

 Returns:
    None

 Global Variables Used:
    None

 Local Variables Needed:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

 PURPOSE: Search the best codevector; determine positions of the 3 pulses
          in the 40-sample frame.
------------------------------------------------------------------------------
 REQUIREMENTS

 None

------------------------------------------------------------------------------
 REFERENCES

 [1] c3_14pf.c, UMTS GSM AMR speech codec, R99 - Version 3.2.0, March 2, 2001

------------------------------------------------------------------------------
 PSEUDO-CODE

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

int pos[36] = { 0,1,2,2,0,1,1,2,0,
                0,1,4,4,0,1,1,4,0,
                0,3,2,2,0,3,3,2,0,
                0,3,4,4,0,3,3,4,0};

void search_3i40(
    Word16 dn[],         /* i : correlation between target and h[] */
    Word16 dn2[],        /* i : maximum of corr. in each track.    */
    Word16 rr[][L_CODE], /* i : matrix of autocorrelation          */
    Word16 codvec[],     /* o : algebraic codebook vector          */
    Flag   * pOverflow   /* o : Flag set when overflow occurs      */
)
{
    Word16 i0;
    Word16 i1;
    Word16 i2;

    Word16 *rri1, *rri0;

    Word16 ix = 0; /* initialization only needed to keep gcc silent */
    Word16 ps = 0; /* initialization only needed to keep gcc silent */

    Word16 i;
    Word16 track1;
    Word16 track2;
    Word32 *ipos;

    Word32 psk;
    Word32 ps0;
    Word32 ps1;
    Word32 sq;
    Word32 sq1;
    Word32 alpk;
    Word32 alp;
    Word32 alp_16;

    Word16 *p_codvec = &codvec[0];

    Word32 s;
    Word32 alp0;
    Word32 alp1;

    psk = -1;
    alpk = 1;

    for (i = 0; i < NB_PULSE; i++)
    {
        *(p_codvec++) = i;
    }

    ipos = pos;
#if (MIPS_DSP_R2_LE)
    for (i = 0; i < 12; i++)
    {
        /*----------------------------------------------------------------*
         * i0 loop: try 8 positions.                                      *
         *----------------------------------------------------------------*/

        /* account for ptr. init. (rr[io]) */
        for (i0 = ipos[0]; i0 < L_CODE; i0 += STEP)
        {
            if (dn2[i0] >= 0)
            {
                Word32 i1_end;
                Word16 *rr_i2;
                Word32 temp1,temp3,temp4,temp6,temp5;
                __asm__ volatile (
                    ".set push                                                     \n\t"
                    ".set noreorder                                                \n\t"
                    "lw            %[ix],      4(%[ipos])                          \n\t"
                    "sll           %[temp1],   %[i0],                1             \n\t"
                    "li            %[temp4],   40                                  \n\t"
                    "sll           %[i1],      %[ix],                1             \n\t"
                    "mul           %[temp5],   %[i1],                %[temp4]      \n\t"
                    "mul           %[temp3],   %[temp4],             %[temp1]      \n\t"
                    "addiu         %[i1_end],  %[i1],                70            \n\t"
                    "li            %[sq],      -1                                  \n\t"
                    "li            %[alp],     1                                   \n\t"
                    "li            %[ps],      0                                   \n\t"
                    "lhx           %[ps0],     %[temp1](%[dn])                     \n\t"
                    "addu          %[temp5],   %[temp5],             %[i1]         \n\t"
                    "addu          %[rri1],    %[temp5],             %[rr]         \n\t"
                    "addu          %[rri0],    %[temp3],             %[rr]         \n\t"
                    "lhx           %[alp0],    %[temp1](%[rri0])                   \n\t"
                    "lhx           %[temp6],   %[i1](%[dn])                        \n\t"
                    "lh            %[temp5],   0(%[rri1])                          \n\t"
                    "lhx           %[temp1],   %[i1](%[rri0])                      \n\t"
                    "addiu         %[rri1],    %[rri1],              410           \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "mulq_s.ph     %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "shra_r.w      %[alp_16],  %[alp1],              2             \n\t"
                    //for (i1 = ipos[1]; i1 < L_CODE; i1 += STEP)
                "22:                                                               \n\t"
                    "addiu         %[i1],      %[i1],                10            \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "lhx           %[temp6],   %[i1](%[dn])                        \n\t"
                    "lh            %[temp5],   0(%[rri1])                          \n\t"
                    "lhx           %[temp1],   %[i1](%[rri0])                      \n\t"
                    "addiu         %[rri1],    %[rri1],              410           \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   11f                                 \n\t"
                    " addu         %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "addiu         %[ix],      %[i1],                -10           \n\t"
                    "srl           %[ix],      %[ix],                1             \n\t"
                "11:                                                               \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "mulq_s.ph     %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "bne           %[i1],      %[i1_end],            22b           \n\t"
                    " shra_r.w     %[alp_16],  %[alp1],              2             \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   33f                                 \n\t"
                    " li           %[temp4],   40                                  \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "srl           %[ix],      %[i1],                1             \n\t"
                "33:                                                               \n\t"
                    /*----------------------------------------------------------------*
                    * i2 loop: 8 positions.                                          *
                    *----------------------------------------------------------------*/
                    "move          %[i1],      %[ix]                               \n\t"
                    "lw            %[ix],      8(%[ipos])                          \n\t"
                    "move          %[ps0],     %[ps]                               \n\t"
                    "li            %[sq],      -1                                  \n\t"
                    "sll           %[i2],      %[ix],                1             \n\t"
                    "mul           %[temp3],   %[temp4],             %[i2]         \n\t"
                    "sll           %[temp1],   %[i1],                1             \n\t"
                    "mul           %[temp5],   %[temp4],             %[temp1]      \n\t"
                    "sll           %[alp0],    %[alp],               2             \n\t"
                    "li            %[alp],     1                                   \n\t"
                    "li            %[ps],      0                                   \n\t"
                    "addiu         %[i1_end],  %[i2],                70            \n\t"
                    "addu          %[rr_i2],   %[temp3],             %[rr]         \n\t"
                    "addu          %[rr_i2],   %[rr_i2],             %[i2]         \n\t"
                    "addu          %[rri1],    %[temp5],             %[rr]         \n\t"
                    //second for
                    "lhx           %[temp6],   %[i2](%[dn])                        \n\t"
                    "lh            %[temp5],   0(%[rr_i2])                         \n\t"
                    "lhx           %[temp1],   %[i2](%[rri1])                      \n\t"
                    "lhx           %[temp3],   %[i2](%[rri0])                      \n\t"
                    "addiu         %[rr_i2],   %[rr_i2],             410           \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "mulq_s.ph     %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "addu          %[temp1],   %[temp3],             %[temp1]      \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "shra_r.w      %[alp_16],  %[alp1],              4             \n\t"
                "2:                                                                \n\t"
                    "addiu         %[i2],      %[i2],                10            \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "lh            %[temp5],   0(%[rr_i2])                         \n\t"
                    "lhx           %[temp1],   %[i2](%[rri1])                      \n\t"
                    "lhx           %[alp1],    %[i2](%[rri0])                      \n\t"
                    "addiu         %[rr_i2],   %[rr_i2],             410           \n\t"
                    "lhx           %[temp6],   %[i2](%[dn])                        \n\t"
                    "addu          %[temp1],   %[alp1],              %[temp1]      \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   1f                                  \n\t"
                    " addu         %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "addiu         %[ix],      %[i2],                -10           \n\t"
                    "srl           %[ix],      %[ix],                1             \n\t"
                "1:                                                                \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "mulq_s.ph     %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "bne           %[i2],      %[i1_end],            2b            \n\t"
                    " shra_r.w     %[alp_16],  %[alp1],              4             \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   3f                                  \n\t"
                    "nop                                                           \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "srl           %[ix],      %[i2],                1             \n\t"
                "3:                                                                \n\t"
                    "move          %[i2],      %[ix]                               \n\t"
                    ".set pop                                                      \n\t"

                    :[temp1] "=&r" (temp1), [ps1] "=&r" (ps1),
                     [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),[i1_end] "=&r" (i1_end),
                     [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),[ps0] "=&r" (ps0) ,
                     [alp_16] "=&r" (alp_16), [sq1] "=&r" (sq1), [alp0] "=&r" (alp0),
                     [alp] "=&r" (alp), [sq] "=&r" (sq),[ix] "=&r" (ix), [ps] "=&r" (ps),
                     [rri1] "=&r" (rri1),[alp1] "=&r" (alp1),[rri0] "=&r" (rri0),
                     [i1] "=&r" (i1),[i2] "=&r" (i2),[rr_i2] "=&r" (rr_i2)
                    :[dn] "r" (dn), [rr] "r" (rr),[i0] "r" (i0),[ipos] "r" (ipos)
                    : "memory", "hi", "lo"
                );

                if (alpk * sq > psk * alp)
                {
                    psk = sq;
                    alpk = alp;

                    codvec[0] = i0;
                    codvec[1] = i1;
                    codvec[2] = i2;
                }
            }
        }
        ipos += 3;
    }
    return;
#elif (MIPS_DSP_R1_LE)/*(MIPS_DSP_R1_LE)*/
    for (i = 0; i < 12; i++)
    {
        /*----------------------------------------------------------------*
         * i0 loop: try 8 positions.                                      *
         *----------------------------------------------------------------*/

        /* account for ptr. init. (rr[io]) */
        for (i0 = ipos[0]; i0 < L_CODE; i0 += STEP)
        {
            if (dn2[i0] >= 0)
            {
                Word32 i1_end;
                Word16 *rr_i2;
                Word32 temp1,temp3,temp4,temp6,temp5;
                __asm__ volatile (
                    "lw            %[ix],      4(%[ipos])                          \n\t"//ix = ipos[1];
                    "sll           %[temp1],   %[i0],                1             \n\t"
                    "li            %[temp4],   40                                  \n\t"
                    "sll           %[i1],      %[ix],                1             \n\t"//i1 = ix*2
                    "mul           %[temp5],   %[i1],                %[temp4]      \n\t"
                    "mul           %[temp3],   %[temp4],             %[temp1]      \n\t"
                    "addiu         %[i1_end],  %[i1],                70            \n\t"//Word32 i1_end; = ix*2 + STEP*2*7;
                    "li            %[sq],      -1                                  \n\t"
                    "li            %[alp],     1                                   \n\t"
                    "li            %[ps],      0                                   \n\t"
                    "lhx           %[ps0],     %[temp1](%[dn])                     \n\t"// ps0 = dn[i0];
                    "addu          %[temp5],   %[temp5],             %[i1]         \n\t"
                    "addu          %[rri1],    %[temp5],             %[rr]         \n\t"//Word16 *rri1; = &rr[ix][ix];
                    "addu          %[rri0],    %[temp3],             %[rr]         \n\t"//rri0 = rr[i0];
                    "lhx           %[alp0],    %[temp1](%[rri0])                   \n\t"//alp0 = (Word32) rr[i0][i0];
                    "lhx           %[temp6],   %[i1](%[dn])                        \n\t"
                    "lh            %[temp5],   0(%[rri1])                          \n\t"
                    "lhx           %[temp1],   %[i1](%[rri0])                      \n\t"
                    "addiu         %[rri1],    %[rri1],              410           \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "muleq_s.w.phr %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "shra_r.w      %[alp_16],  %[alp1],              2             \n\t"
                    "sra           %[sq1],     %[sq1],               16            \n\t"
                    //for (i1 = ipos[1]; i1 < L_CODE; i1 += STEP)
                  "22:                                                             \n\t"
                    "addiu         %[i1],      %[i1],                10            \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "lhx           %[temp6],   %[i1](%[dn])                        \n\t"
                    "lh            %[temp5],   0(%[rri1])                          \n\t"
                    "lhx           %[temp1],   %[i1](%[rri0])                      \n\t"
                    "addiu         %[rri1],    %[rri1],              410           \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   11f                                 \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "addiu         %[ix],      %[i1],                -10           \n\t"
                    "srl           %[ix],      %[ix],                1             \n\t"
                  "11:                                                             \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "muleq_s.w.phr %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "shra_r.w      %[alp_16],  %[alp1],              2             \n\t"
                    "sra           %[sq1],     %[sq1],               16            \n\t"
                    "bne           %[i1],      %[i1_end],            22b           \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   33f                                 \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "srl           %[ix],      %[i1],                1             \n\t"
                  "33:                                                             \n\t"
                  /*----------------------------------------------------------------*
                   * i2 loop: 8 positions.                                          *
                   *----------------------------------------------------------------*/
                    "move          %[i1],      %[ix]                               \n\t"
                    "li            %[temp4],   40                                  \n\t"
                    "lw            %[ix],      8(%[ipos])                          \n\t"//ix = ipos[1];
                    "move          %[ps0],     %[ps]                               \n\t"
                    "li            %[sq],      -1                                  \n\t"
                    "sll           %[i2],      %[ix],                1             \n\t"//i2 = ix*2
                    "mul           %[temp3],   %[temp4],             %[i2]         \n\t"
                    "sll           %[temp1],   %[i1],                1             \n\t"
                    "mul           %[temp5],   %[temp4],             %[temp1]      \n\t"
                    "sll           %[alp0],    %[alp],               2             \n\t"
                    "li            %[alp],     1                                   \n\t"
                    "li            %[ps],      0                                   \n\t"
                    "addiu         %[i1_end],  %[i2],                70            \n\t"//Word32 i1_end; = ix*2 + STEP*2*7;
                    "addu          %[rr_i2],   %[temp3],             %[rr]         \n\t"
                    "addu          %[rr_i2],   %[rr_i2],             %[i2]         \n\t"//rri2 = &rr[i2][i2];
                    "addu          %[rri1],    %[temp5],             %[rr]         \n\t"//rri0 = rr[i1];
                    //second for
                    "lhx           %[temp6],   %[i2](%[dn])                        \n\t"
                    "lh            %[temp5],   0(%[rr_i2])                         \n\t"
                    "lhx           %[temp1],   %[i2](%[rri1])                      \n\t"
                    "lhx           %[temp3],   %[i2](%[rri0])                      \n\t"
                    "addiu         %[rr_i2],   %[rr_i2],             410           \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "muleq_s.w.phr %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "addu          %[temp1],   %[temp3],             %[temp1]      \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "shra_r.w      %[alp_16],  %[alp1],              4             \n\t"
                    "sra           %[sq1],     %[sq1],               16            \n\t"
                  "2:                                                              \n\t"
                    "addiu         %[i2],      %[i2],                10            \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "lh            %[temp5],   0(%[rr_i2])                         \n\t"
                    "lhx           %[temp1],   %[i2](%[rri1])                      \n\t"
                    "lhx           %[alp1],    %[i2](%[rri0])                      \n\t"
                    "addiu         %[rr_i2],   %[rr_i2],             410           \n\t"
                    "lhx           %[temp6],   %[i2](%[dn])                        \n\t"
                    "addu          %[temp1],   %[alp1],              %[temp1]      \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "addu          %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   1f                                  \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "addiu         %[ix],      %[i2],                -10           \n\t"
                    "srl           %[ix],      %[ix],                1             \n\t"
                 "1:                                                               \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "muleq_s.w.phr %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "shra_r.w      %[alp_16],  %[alp1],              4             \n\t"
                    "sra           %[sq1],     %[sq1],               16            \n\t"
                    "bne           %[i2],      %[i1_end],            2b            \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   3f                                  \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "srl           %[ix],      %[i2],                1             \n\t"
                 "3:                                                               \n\t"
                    "move          %[i2],      %[ix]                               \n\t"

                    :[temp1] "=&r" (temp1), [ps1] "=&r" (ps1), [temp3] "=&r" (temp3),
                     [temp4] "=&r" (temp4),[i1_end] "=&r" (i1_end),
                     [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),[ps0] "=&r" (ps0) ,
                     [alp_16] "=&r" (alp_16), [sq1] "=&r" (sq1), [alp0] "=&r" (alp0),
                     [alp] "=&r" (alp), [sq] "=&r" (sq),[ix] "=&r" (ix), [ps] "=&r" (ps),
                     [rri1] "=&r" (rri1),[alp1] "=&r" (alp1),[rri0] "=&r" (rri0),
                     [i1] "=&r" (i1),[i2] "=&r" (i2),[rr_i2] "=&r" (rr_i2)
                    :[dn] "r" (dn), [rr] "r" (rr),[i0] "r" (i0),[ipos] "r" (ipos)
                    : "memory", "hi", "lo"
                );

                if (alpk * sq > psk * alp)
                {
                    psk = sq;
                    alpk = alp;

                    codvec[0] = i0;
                    codvec[1] = i1;
                    codvec[2] = i2;
                }
            }
        }
        ipos += 3;
    }
    return;
#elif (MIPS32_R2_LE)
    for (i = 0; i < 12; i++)
    {
        /*----------------------------------------------------------------*
         * i0 loop: try 8 positions.                                      *
         *----------------------------------------------------------------*/

        /* account for ptr. init. (rr[io]) */
        for (i0 = ipos[0]; i0 < L_CODE; i0 += STEP)
        {
            if (dn2[i0] >= 0)
            {
                Word32 i1_end;
                Word16 *rr_i2;
                Word32 temp1,temp3,temp4,temp6,temp5;
                __asm__ volatile (
                    "lw            %[ix],      4(%[ipos])                          \n\t"//ix = ipos[1];
                    "sll           %[temp1],   %[i0],                1             \n\t"
                    "li            %[temp4],   40                                  \n\t"
                    "sll           %[i1],      %[ix],                1             \n\t"//i1 = ix*2
                    "mul           %[temp5],   %[i1],                %[temp4]      \n\t"
                    "mul           %[temp3],   %[temp4],             %[temp1]      \n\t"
                    "addiu         %[i1_end],  %[i1],                70            \n\t"//Word32 i1_end; = ix*2 + STEP*2*7;
                    "addu          %[ps0],     %[temp1],             %[dn]         \n\t"
                    "li            %[sq],      -1                                  \n\t"
                    "li            %[alp],     1                                   \n\t"
                    "li            %[ps],      0                                   \n\t"
                    "lh            %[ps0],     0(%[ps0])                           \n\t"// ps0 = dn[i0];
                    "addu          %[temp5],   %[temp5],             %[i1]         \n\t"
                    "addu          %[rri1],    %[temp5],             %[rr]         \n\t"//Word16 *rri1; = &rr[ix][ix];
                    "addu          %[rri0],    %[temp3],             %[rr]         \n\t"//rri0 = rr[i0];
                    "addu          %[temp6],   %[i1],                %[dn]         \n\t"
                    "addu          %[alp0],    %[temp1],             %[rri0]       \n\t"
                    "addu          %[temp1],   %[i1],                %[rri0]       \n\t"
                    "lh            %[temp6],   0(%[temp6])                         \n\t"
                    "lh            %[alp0],    0(%[alp0])                          \n\t"//alp0 = (Word32) rr[i0][i0];
                    "lh            %[temp5],   0(%[rri1])                          \n\t"
                    "lh            %[temp1],   0(%[temp1])                         \n\t"
                    "addiu         %[rri1],    %[rri1],              410           \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "mul           %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "addiu         %[alp_16],  %[alp1],              2             \n\t"
                    "sra           %[alp_16],  %[alp_16],            2             \n\t"
                    "sra           %[sq1],     %[sq1],               15            \n\t"
                    //for (i1 = ipos[1]; i1 < L_CODE; i1 += STEP)
                  "22:                                                             \n\t"
                    "addiu         %[i1],      %[i1],                10            \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "addu          %[temp6],   %[i1],                %[dn]         \n\t"
                    "addu          %[temp1],   %[i1],                %[rri0]       \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "lh            %[temp6],   0(%[temp6])                         \n\t"
                    "lh            %[temp5],   0(%[rri1])                          \n\t"
                    "lh            %[temp1],   0(%[temp1])                         \n\t"
                    "addiu         %[rri1],    %[rri1],              410           \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   11f                                 \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "addiu         %[ix],      %[i1],                -10           \n\t"
                    "srl           %[ix],      %[ix],                1             \n\t"
                  "11:                                                             \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "mul           %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "addiu         %[alp_16],  %[alp1],              2             \n\t"
                    "sra           %[alp_16],  %[alp_16],            2             \n\t"
                    "sra           %[sq1],     %[sq1],               15            \n\t"
                    "bne           %[i1],      %[i1_end],            22b           \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   33f                                 \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "srl           %[ix],      %[i1],                1             \n\t"
                  "33:                                                             \n\t"
                  /*----------------------------------------------------------------*
                   * i2 loop: 8 positions.                                          *
                   *----------------------------------------------------------------*/
                    "move          %[i1],      %[ix]                               \n\t"
                    "li            %[temp4],   40                                  \n\t"
                    "lw            %[ix],      8(%[ipos])                          \n\t"//ix = ipos[1];
                    "move          %[ps0],     %[ps]                               \n\t"
                    "li            %[sq],      -1                                  \n\t"
                    "sll           %[i2],      %[ix],                1             \n\t"//i2 = ix*2
                    "mul           %[temp3],   %[temp4],             %[i2]         \n\t"
                    "sll           %[temp1],   %[i1],                1             \n\t"
                    "mul           %[temp5],   %[temp4],             %[temp1]      \n\t"
                    "sll           %[alp0],    %[alp],               2             \n\t"
                    "li            %[alp],     1                                   \n\t"
                    "li            %[ps],      0                                   \n\t"
                    "addiu         %[i1_end],  %[i2],                70            \n\t"//Word32 i1_end; = ix*2 + STEP*2*7;
                    "addu          %[rr_i2],   %[temp3],             %[rr]         \n\t"
                    "addu          %[temp6],   %[i2],                %[dn]         \n\t"
                    "addu          %[rr_i2],   %[rr_i2],             %[i2]         \n\t"//rri2 = &rr[i2][i2];
                    "addu          %[rri1],    %[temp5],             %[rr]         \n\t"//rri0 = rr[i1];
                    "addu          %[temp1],   %[i2],                %[rri1]       \n\t"
                    "addu          %[temp3],   %[i2],                %[rri0]       \n\t"
                    //second for
                    "lh            %[temp6],   0(%[temp6])                         \n\t"
                    "lh            %[temp5],   0(%[rr_i2])                         \n\t"
                    "lh            %[temp1],   0(%[temp1])                         \n\t"
                    "lh            %[temp3],   0(%[temp3])                         \n\t"
                    "addiu         %[rr_i2],   %[rr_i2],             410           \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "mul           %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "addu          %[temp1],   %[temp3],             %[temp1]      \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "addiu         %[alp_16],  %[alp1],              8             \n\t"
                    "sra           %[alp_16],  %[alp_16],            4             \n\t"
                    "sra           %[sq1],     %[sq1],               15            \n\t"
                  "2:                                                              \n\t"
                    "addiu         %[i2],      %[i2],                10            \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "addu          %[temp1],   %[i2],                %[rri1]       \n\t"
                    "addu          %[alp1],    %[i2],                %[rri0]       \n\t"
                    "addu          %[temp6],   %[i2],                %[dn]         \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "lh            %[temp5],   0(%[rr_i2])                         \n\t"
                    "lh            %[temp1],   0(%[temp1])                         \n\t"
                    "lh            %[alp1],    0(%[alp1])                          \n\t"
                    "addiu         %[rr_i2],   %[rr_i2],             410           \n\t"
                    "lh            %[temp6],   0(%[temp6])                         \n\t"
                    "addu          %[temp1],   %[alp1],              %[temp1]      \n\t"
                    "sll           %[temp1],   %[temp1],             1             \n\t"
                    "addu          %[alp1],    %[temp5],             %[alp0]       \n\t"
                    "addu          %[alp1],    %[alp1],              %[temp1]      \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   1f                                  \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "addiu         %[ix],      %[i2],                -10           \n\t"
                    "srl           %[ix],      %[ix],                1             \n\t"
                 "1:                                                               \n\t"
                    "addu          %[ps1],     %[ps0],               %[temp6]      \n\t"
                    "mul           %[sq1],     %[ps1],               %[ps1]        \n\t"
                    "addiu         %[alp_16],  %[alp1],              8             \n\t"
                    "sra           %[alp_16],  %[alp_16],            4             \n\t"
                    "sra           %[sq1],     %[sq1],               15            \n\t"
                    "bne           %[i2],      %[i1_end],            2b            \n\t"
                    "mul           %[temp3],   %[alp],               %[sq1]        \n\t"
                    "mul           %[temp4],   %[sq],                %[alp_16]     \n\t"
                    "subu          %[temp3],   %[temp4],             %[temp3]      \n\t"
                    "bgez          %[temp3],   3f                                  \n\t"
                    "move          %[sq],      %[sq1]                              \n\t"
                    "move          %[ps],      %[ps1]                              \n\t"
                    "move          %[alp],     %[alp_16]                           \n\t"
                    "srl           %[ix],      %[i2],                1             \n\t"
                 "3:                                                               \n\t"
                    "move          %[i2],      %[ix]                               \n\t"

                    :[temp1] "=&r" (temp1), [ps1] "=&r" (ps1),
                     [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),[i1_end] "=&r" (i1_end),
                     [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),[ps0] "=&r" (ps0) ,
                     [alp_16] "=&r" (alp_16), [sq1] "=&r" (sq1), [alp0] "=&r" (alp0),
                     [alp] "=&r" (alp), [sq] "=&r" (sq),[ix] "=&r" (ix), [ps] "=&r" (ps),
                     [rri1] "=&r" (rri1),[alp1] "=&r" (alp1),[rri0] "=&r" (rri0),
                     [i1] "=&r" (i1),[i2] "=&r" (i2),[rr_i2] "=&r" (rr_i2)
                    :[dn] "r" (dn), [rr] "r" (rr),[i0] "r" (i0),[ipos] "r" (ipos)
                    : "memory", "hi", "lo"
                );

                if (alpk * sq > psk * alp)
                {
                    psk = sq;
                    alpk = alp;

                    codvec[0] = i0;
                    codvec[1] = i1;
                    codvec[2] = i2;
                }
            }
        }
        ipos += 3;
    }
    return;
#endif
}

/****************************************************************************/