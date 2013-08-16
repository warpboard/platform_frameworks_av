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



 Pathname: ./audio/gsm-amr/c/src/qua_gain.c
 Functions:

     Date: 02/05/2002

------------------------------------------------------------------------------
 REVISION HISTORY

 Description: Updated template used to PV coding template.
 Changed to accept the pOverflow flag for EPOC compatibility.

 Description: Changed include files to lowercase.

 Description:  Replaced OSCL mem type functions and eliminated include
               files that now are chosen by OSCL definitions

 Description:  Replaced "int" and/or "char" with OSCL defined types.

 Description: Added #ifdef __cplusplus around extern'ed table.

 Description:

------------------------------------------------------------------------------
 MODULE DESCRIPTION

    Quantization of pitch and codebook gains.
------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/
#include "qua_gain.h"
#include "typedef.h"
#include "basic_op.h"

#include "mode.h"
#include "cnst.h"
#include "pow2.h"
#include "gc_pred.h"

/*--------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

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

    /*----------------------------------------------------------------------------
    ; EXTERNAL GLOBAL STORE/BUFFER/POINTER REFERENCES
    ; Declare variables used in this module but defined elsewhere
    ----------------------------------------------------------------------------*/
    extern const Word16 table_gain_lowrates[];
    extern const Word16 table_gain_highrates[];

    /*--------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

/*
------------------------------------------------------------------------------
 FUNCTION NAME:
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS


 Inputs:
    mode -- enum Mode -- AMR mode
    Word16 exp_gcode0  -- Word16 -- predicted CB gain (exponent),       Q0
    Word16 frac_gcode0 -- Word16 -- predicted CB gain (fraction),      Q15
    Word16 frac_coeff -- Word16 Array -- energy coeff. (5), fraction part, Q15
    Word16 exp_coeff  -- Word16 Array -- energy coeff. (5), exponent part,  Q0
                                    (frac_coeff and exp_coeff computed in
                                    calc_filt_energies())

    Word16 gp_limit -- Word16 --  pitch gain limit

 Outputs:
    Word16 *gain_pit -- Pointer to Word16 -- Pitch gain,               Q14
    Word16 *gain_cod -- Pointer to Word16 -- Code gain,                Q1
    Word16 *qua_ener_MR122 -- Pointer to Word16 -- quantized energy error,  Q10
                                                (for MR122 MA predictor update)
    Word16 *qua_ener -- Pointer to Word16 -- quantized energy error,        Q10
                                                (for other MA predictor update)
    Flag   *pOverflow -- Pointer to Flag -- overflow indicator

 Returns:
    Word16 -- index of quantization.

 Global Variables Used:


 Local Variables Needed:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

    Quantization of pitch and codebook gains.
------------------------------------------------------------------------------
 REQUIREMENTS

 None

------------------------------------------------------------------------------
 REFERENCES

 qua_gain.c, UMTS GSM AMR speech codec, R99 - Version 3.2.0, March 2, 2001

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

Word16
Qua_gain(                   /* o  : index of quantization.                 */
    enum Mode mode,         /* i  : AMR mode                               */
    Word16 exp_gcode0,      /* i  : predicted CB gain (exponent),      Q0  */
    Word16 frac_gcode0,     /* i  : predicted CB gain (fraction),      Q15 */
    Word16 frac_coeff[],    /* i  : energy coeff. (5), fraction part,  Q15 */
    Word16 exp_coeff[],     /* i  : energy coeff. (5), exponent part,  Q0  */
    /*      (frac_coeff and exp_coeff computed in  */
    /*       calc_filt_energies())                 */
    Word16 gp_limit,        /* i  : pitch gain limit                       */
    Word16 *gain_pit,       /* o  : Pitch gain,                        Q14 */
    Word16 *gain_cod,       /* o  : Code gain,                         Q1  */
    Word16 *qua_ener_MR122, /* o  : quantized energy error,            Q10 */
    /*      (for MR122 MA predictor update)        */
    Word16 *qua_ener,       /* o  : quantized energy error,            Q10 */
    /*      (for other MA predictor update)        */
    Flag   *pOverflow       /* o  : overflow indicator                     */
)
{
    const Word16 *p;
    Word16 i;
    Word16 j;
    Word16 index = 0;
    Word16 gcode0;
    Word16 e_max;
    Word16 temp;
    Word16 exp_code;
    Word16 g_pitch;
    Word16 g2_pitch;
    Word16 g_code;
    Word16 g2_code;
    Word16 g_pit_cod;
    Word16 coeff[5];
    Word16 coeff_lo[5];
    Word16 exp_max[5];
    Word32 L_tmp;
    Word32 L_tmp2;
    Word32 dist_min;
    const Word16 *table_gain;
    Word16 table_len;


    if (mode == MR102 || mode == MR74 || mode == MR67)
    {
        table_len = VQ_SIZE_HIGHRATES;
        table_gain = table_gain_highrates;
    }
    else
    {
        table_len = VQ_SIZE_LOWRATES;
        table_gain = table_gain_lowrates;
    }

    /*-------------------------------------------------------------------*
     *  predicted codebook gain                                          *
     *  ~~~~~~~~~~~~~~~~~~~~~~~                                          *
     *  gc0     = 2^exp_gcode0 + 2^frac_gcode0                           *
     *                                                                   *
     *  gcode0 (Q14) = 2^14*2^frac_gcode0 = gc0 * 2^(14-exp_gcode0)      *
     *-------------------------------------------------------------------*/

    gcode0 = (Word16)(Pow2(14, frac_gcode0, pOverflow));

    /*-------------------------------------------------------------------*
     *  Scaling considerations:                                          *
     *  ~~~~~~~~~~~~~~~~~~~~~~~                                          *
     *-------------------------------------------------------------------*/

    /*
     * The error energy (sum) to be minimized consists of five terms, t[0..4].
     *
     *                      t[0] =    gp^2  * <y1 y1>
     *                      t[1] = -2*gp    * <xn y1>
     *                      t[2] =    gc^2  * <y2 y2>
     *                      t[3] = -2*gc    * <xn y2>
     *                      t[4] =  2*gp*gc * <y1 y2>
     *
     */

    /* determine the scaling exponent for g_code: ec = ec0 - 11 */
#if (MIPS_DSP_R2_LE) || (MIPS_DSP_R1_LE)
    {
        Word32 temp1, temp2, temp3, temp4, temp5;
        __asm__ volatile (
           "li            %[temp1],    11                             \n\t"
           "li            %[temp2],    13                             \n\t"
           "li            %[temp3],    14                             \n\t"
           "lh            %[temp4],    0(%[exp_coeff])                \n\t"
           "lh            %[temp5],    2(%[exp_coeff])                \n\t"
           "subq_s.ph     %[exp_code], %[exp_gcode0],  %[temp1]       \n\t"
           "subq_s.ph     %[temp1],    %[temp4],       %[temp2]       \n\t"
           "subq_s.ph     %[temp2],    %[temp5],       %[temp3]       \n\t"
           "shll_s.ph     %[temp4],    %[exp_code],    1              \n\t"
           "li            %[temp5],    15                             \n\t"
           "sh            %[temp1],    0(%[exp_max])                  \n\t"
           "sh            %[temp2],    2(%[exp_max])                  \n\t"
           "addq_s.ph     %[temp5],    %[temp5],       %[temp4]       \n\t"
           "lh            %[temp1],    4(%[exp_coeff])                \n\t"
           "lh            %[temp2],    6(%[exp_coeff])                \n\t"
           "li            %[temp4],    1                              \n\t"
           "lh            %[temp3],    8(%[exp_coeff])                \n\t"
           "addq_s.ph     %[temp5],    %[temp5],       %[temp1]       \n\t"
           "addq_s.ph     %[temp4],    %[temp4],       %[exp_code]    \n\t"
           "addq_s.ph     %[temp2],    %[temp2],       %[exp_code]    \n\t"
           "addq_s.ph     %[temp4],    %[temp3],       %[temp4]       \n\t"
           "sh            %[temp5],    4(%[exp_max])                  \n\t"
           "sh            %[temp2],    6(%[exp_max])                  \n\t"
           "sh            %[temp4],    8(%[exp_max])                  \n\t"

           :[exp_code] "=&r" (exp_code),
            [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
            [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
            [temp5] "=&r" (temp5)
           :[exp_coeff] "r" (exp_coeff),[exp_max] "r" (exp_max),
            [exp_gcode0] "r" (exp_gcode0)
           :"memory"
        );
    }
#elif (MIPS32_R2_LE)
    exp_code = sub(exp_gcode0, 11, pOverflow);

    /* calculate exp_max[i] = s[i]-1 */
    exp_max[0] = sub(exp_coeff[0], 13, pOverflow);
    exp_max[1] = sub(exp_coeff[1], 14, pOverflow);

    temp = shl(exp_code, 1, pOverflow);
    temp = add(15, temp, pOverflow);
    exp_max[2] = add(exp_coeff[2], temp, pOverflow);

    exp_max[3] = add(exp_coeff[3], exp_code, pOverflow);

    temp = add(1, exp_code, pOverflow);
    exp_max[4] = add(exp_coeff[4], temp, pOverflow);

#endif

    /*-------------------------------------------------------------------*
     *  Find maximum exponent:                                           *
     *  ~~~~~~~~~~~~~~~~~~~~~~                                           *
     *                                                                   *
     *  For the sum operation, all terms must have the same scaling;     *
     *  that scaling should be low enough to prevent overflow. There-    *
     *  fore, the maximum scale is determined and all coefficients are   *
     *  re-scaled:                                                       *
     *                                                                   *
     *    e_max = max(exp_max[i]) + 1;                                   *
     *    e = exp_max[i]-e_max;         e <= 0!                          *
     *    c[i] = c[i]*2^e                                                *
     *-------------------------------------------------------------------*/

    e_max = exp_max[0];
#if (MIPS_DSP_R2_LE) || (MIPS_DSP_R1_LE)
    {
        Word32 temp1, temp2, temp3, temp4, temp5;
        temp1 = exp_max[1];
        temp2 = exp_max[2];
        temp3 = exp_max[3];
        temp4 = exp_max[4];
        temp5 =1;
        if (temp1 > e_max)
        {
            e_max = temp1;
        }
        if (temp2 > e_max)
        {
            e_max = temp2;
        }
        if (temp3 > e_max)
        {
            e_max = temp3;
        }
        if (temp4 > e_max)
        {
            e_max = temp4;
        }
        __asm__ volatile (
            "addq_s.ph     %[e_max],    %[temp5],       %[e_max]       \n\t"
            :[e_max] "+r" (e_max)
            :[temp5] "r" (temp5)
        );
    }
#elif (MIPS32_R2_LE)
    for (i = 1; i < 5; i++)
    {
        if (exp_max[i] > e_max)
        {
            e_max = exp_max[i];
        }
    }
    e_max = add(e_max, 1, pOverflow);      /* To avoid overflow */
#endif

    for (i = 0; i < 5; i++)
    {
        j = sub(e_max, exp_max[i], pOverflow);
        L_tmp = L_deposit_h(frac_coeff[i]);
        L_tmp = L_shr(L_tmp, j, pOverflow);
        L_Extract(L_tmp, &coeff[i], &coeff_lo[i], pOverflow);
    }


    /*-------------------------------------------------------------------*
     *  Codebook search:                                                 *
     *  ~~~~~~~~~~~~~~~~                                                 *
     *                                                                   *
     *  For each pair (g_pitch, g_fac) in the table calculate the        *
     *  terms t[0..4] and sum them up; the result is the mean squared    *
     *  error for the quantized gains from the table. The index for the  *
     *  minimum MSE is stored and finally used to retrieve the quantized *
     *  gains                                                            *
     *-------------------------------------------------------------------*/

    /* start with "infinite" MSE */
    dist_min = MAX_32;

    p = &table_gain[0];

    Word32 temp_const = 0xfffffffe;
    Word32 temp_coeff_00, temp_coeff_11, temp_coeff_22, temp_coeff_33, temp_coeff_44;
    {
        Word32 temp1, temp2, temp3, temp4, temp5;
        __asm__ volatile (
            "lh     %[temp_coeff_00],   0(%[coeff_lo])                  \n\t"
            "lh     %[temp_coeff_11],   2(%[coeff_lo])                  \n\t"
            "lh     %[temp_coeff_22],   4(%[coeff_lo])                  \n\t"
            "lh     %[temp_coeff_33],   6(%[coeff_lo])                  \n\t"
            "lh     %[temp_coeff_44],   8(%[coeff_lo])                  \n\t"
            "sll    %[temp_coeff_00],   %[temp_coeff_00],   1           \n\t"
            "sll    %[temp_coeff_11],   %[temp_coeff_11],   1           \n\t"
            "sll    %[temp_coeff_22],   %[temp_coeff_22],   1           \n\t"
            "sll    %[temp_coeff_33],   %[temp_coeff_33],   1           \n\t"
            "sll    %[temp_coeff_44],   %[temp_coeff_44],   1           \n\t"
            "lh     %[temp1],           0(%[coeff])                     \n\t"
            "lh     %[temp2],           2(%[coeff])                     \n\t"
            "lh     %[temp3],           4(%[coeff])                     \n\t"
            "lh     %[temp4],           6(%[coeff])                     \n\t"
            "lh     %[temp5],           8(%[coeff])                     \n\t"
            "ins    %[temp_coeff_00],   %[temp1],           16,  16     \n\t"
            "ins    %[temp_coeff_11],   %[temp2],           16,  16     \n\t"
            "ins    %[temp_coeff_22],   %[temp3],           16,  16     \n\t"
            "ins    %[temp_coeff_33],   %[temp4],           16,  16     \n\t"
            "ins    %[temp_coeff_44],   %[temp5],           16,  16     \n\t"

            :[temp_coeff_00] "=&r" (temp_coeff_00), [temp_coeff_11] "=&r" (temp_coeff_11),
             [temp_coeff_22] "=&r" (temp_coeff_22), [temp_coeff_33] "=&r" (temp_coeff_33),
             [temp_coeff_44] "=&r" (temp_coeff_44),
             [temp1] "=&r" (temp1), [temp2] "=&r" (temp2), [temp3] "=&r" (temp3),
             [temp4] "=&r" (temp4),[temp5] "=&r" (temp5)
            :[coeff] "r" (coeff),[coeff_lo] "r" (coeff_lo)
            :"memory"
        );
    }

    for (i = 0; i < table_len; i++)
    {

        g_pitch = p[0];
        g_code = p[1];                   /* this is g_fac        */
        p+=4;                             /* skip log2(g_fac)     */

#if (MIPS_DSP_R2_LE)
        if (g_pitch <= gp_limit)
        {
            Word32 temp1, temp2, temp3, temp4, temp5;

            __asm__ volatile (
                "mulq_s.ph       %[g_code],    %[g_code],        %[gcode0]      \n\t"
                "sll             %[temp2],     %[g_pitch],       16             \n\t"
                "mulq_s.ph       %[g2_pitch],  %[g_pitch],       %[g_pitch]     \n\t"
                "mulq_s.ph       %[g2_code],   %[g_code],        %[g_code]      \n\t"
                "mulq_s.ph       %[g_pit_cod], %[g_code],        %[g_pitch]     \n\t"
                //Mpy_32_16
                "sll             %[temp4],     %[g_code],        16             \n\t"
                "sll             %[temp1],     %[g2_pitch],      16             \n\t"
                "sll             %[temp3],     %[g2_code],       16             \n\t"
                "sll             %[temp5],     %[g_pit_cod],     16             \n\t"
                "mulq_s.w        %[temp1],     %[temp_coeff_00], %[temp1]       \n\t"//L_tmp = Mpy_32_16(coeff[0], coeff_lo[0], g2_pitch, pOverflow);
                "mulq_s.w        %[temp2],     %[temp_coeff_11], %[temp2]       \n\t"
                "mulq_s.w        %[temp3],     %[temp_coeff_22], %[temp3]       \n\t"
                "mulq_s.w        %[temp4],     %[temp_coeff_33], %[temp4]       \n\t"
                "mulq_s.w        %[temp5],     %[temp_coeff_44], %[temp5]       \n\t"
                "and             %[temp1],     %[temp1],         %[temp_const]  \n\t"
                "and             %[temp2],     %[temp2],         %[temp_const]  \n\t"
                "and             %[temp3],     %[temp3],         %[temp_const]  \n\t"
                "and             %[temp4],     %[temp4],         %[temp_const]  \n\t"
                "and             %[temp5],     %[temp5],         %[temp_const]  \n\t"
                "addq_s.w        %[temp1],     %[temp2],         %[temp1]       \n\t"// L_tmp = L_add(L_tmp, L_tmp2, pOverflow);
                "addq_s.w        %[temp3],     %[temp3],         %[temp4]       \n\t"
                "addq_s.w        %[temp1],     %[temp5],         %[temp1]       \n\t"
                "addq_s.w        %[temp1],     %[temp3],         %[temp1]       \n\t"

                :[g_code] "+r" (g_code), [g_pitch] "+r" (g_pitch),
                 [L_tmp] "=&r" (L_tmp), [L_tmp2] "=&r" (L_tmp2),
                 [g2_pitch] "=&r" (g2_pitch),[g_pit_cod] "=&r" (g_pit_cod),
                 [g2_code] "=&r" (g2_code),
                 [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                 [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),[temp5] "=&r" (temp5)
                :[gcode0] "r" (gcode0),[temp_coeff_00] "r" (temp_coeff_00),
                 [temp_coeff_11] "r" (temp_coeff_11),[temp_coeff_22] "r" (temp_coeff_22),
                 [temp_coeff_33] "r" (temp_coeff_33),[temp_coeff_44] "r" (temp_coeff_44),
                 [temp_const] "r" (temp_const)
                :"memory", "hi", "lo"
            );

            /* store table index if MSE for this index is lower
               than the minimum MSE seen so far */
            if (temp1 < dist_min)
            {
                dist_min = temp1;
                index = i;
            }
        }
#elif (MIPS_DSP_R1_LE)/*(MIPS_DSP_R1_LE)*/
        if (g_pitch <= gp_limit)
        {
            Word32 temp1, temp2, temp3, temp4, temp5;
            Word32 temp_const_1 = 0xffff0000;
            __asm__ volatile (
                "muleq_s.w.phr   %[g_code],    %[g_code],        %[gcode0]       \n\t"
                "sll             %[temp2],     %[g_pitch],       16              \n\t"
                "muleq_s.w.phl   %[g2_pitch],  %[temp2],         %[temp2]        \n\t"
                "muleq_s.w.phl   %[g2_code],   %[g_code],        %[g_code]       \n\t"
                "muleq_s.w.phl   %[g_pit_cod], %[g_code],        %[temp2]        \n\t"
                //Mpy_32_16
                "and             %[temp4],     %[g_code],        %[temp_const_1] \n\t"
                "and             %[temp1],     %[g2_pitch],      %[temp_const_1] \n\t"
                "and             %[temp3],     %[g2_code],       %[temp_const_1] \n\t"
                "and             %[temp5],     %[g_pit_cod],     %[temp_const_1] \n\t"
                "mult            $ac0,         %[temp_coeff_00], %[temp1]        \n\t"
                "mult            $ac1,         %[temp_coeff_11], %[temp2]        \n\t"
                "mult            $ac2,         %[temp_coeff_22], %[temp3]        \n\t"
                "mult            $ac3,         %[temp_coeff_33], %[temp4]        \n\t"
                "extr.w          %[temp1],     $ac0,             31              \n\t"
                "extr.w          %[temp2],     $ac1,             31              \n\t"
                "extr.w          %[temp3],     $ac2,             31              \n\t"
                "extr.w          %[temp4],     $ac3,             31              \n\t"
                "mult            $ac0,         %[temp_coeff_44], %[temp5]        \n\t"
                "extr.w          %[temp5],     $ac0,             31              \n\t"
                "and             %[temp1],     %[temp1],         %[temp_const]   \n\t"
                "and             %[temp2],     %[temp2],         %[temp_const]   \n\t"
                "and             %[temp3],     %[temp3],         %[temp_const]   \n\t"
                "and             %[temp4],     %[temp4],         %[temp_const]   \n\t"
                "and             %[temp5],     %[temp5],         %[temp_const]   \n\t"
                "addq_s.w        %[temp1],     %[temp2],         %[temp1]        \n\t"// L_tmp = L_add(L_tmp, L_tmp2, pOverflow);
                "addq_s.w        %[temp3],     %[temp3],         %[temp4]        \n\t"
                "addq_s.w        %[temp1],     %[temp5],         %[temp1]        \n\t"
                "addq_s.w        %[temp1],     %[temp3],         %[temp1]        \n\t"

                :[g_code] "+r" (g_code), [L_tmp] "=&r" (L_tmp), [L_tmp2] "=&r" (L_tmp2),
                 [g2_pitch] "=&r" (g2_pitch), [g_pit_cod] "=&r" (g_pit_cod),
                 [g2_code] "=&r" (g2_code), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                 [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),[temp5] "=&r" (temp5)
                :[gcode0] "r" (gcode0),[temp_coeff_00] "r" (temp_coeff_00),
                 [temp_coeff_11] "r" (temp_coeff_11),[temp_coeff_22] "r" (temp_coeff_22),
                 [temp_coeff_33] "r" (temp_coeff_33),[temp_coeff_44] "r" (temp_coeff_44),
                 [temp_const] "r" (temp_const),[temp_const_1] "r" (temp_const_1),
                 [g_pitch] "r" (g_pitch)
                :"memory", "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
                 "$ac3hi", "$ac3lo"
            );

            /* store table index if MSE for this index is lower
               than the minimum MSE seen so far */
            if (temp1 < dist_min)
            {
                dist_min = temp1;
                index = i;
            }
        }
#elif (MIPS32_R2_LE)
        if (g_pitch <= gp_limit)
        {
            Word32 temp1, temp2, temp3, temp4, temp5;
            Word32 temp_const_1 = 0xffff0000;
            __asm__ volatile (
                "mul             %[g_code],        %[g_code],        %[gcode0]        \n\t"
                "sll             %[temp2],         %[g_pitch],       16               \n\t"
                "mul             %[g2_pitch],      %[g_pitch],       %[g_pitch]       \n\t"
                "sll             %[g_code],        %[g_code],        1                \n\t"
                "sra             %[g2_code],       %[g_code],        16               \n\t"
                "mul             %[g_pit_cod],     %[g2_code],       %[g_pitch]       \n\t"
                "mul             %[g2_code],       %[g2_code],       %[g2_code]       \n\t"
                "sll             %[g2_code],       %[g2_code],       1                \n\t"
                //Mpy_32_16
                "and             %[temp4],         %[g_code],        %[temp_const_1]  \n\t"
                "sll             %[g2_pitch],      %[g2_pitch],      1                \n\t"
                "and             %[temp1],         %[g2_pitch],      %[temp_const_1]  \n\t"
                "and             %[temp3],         %[g2_code],       %[temp_const_1]  \n\t"
                "sll             %[g_pit_cod],     %[g_pit_cod],     1                \n\t"
                "and             %[temp5],         %[g_pit_cod],     %[temp_const_1]  \n\t"
                "mult            %[temp_coeff_00], %[temp1]                           \n\t"
                "mfhi            %[temp1]                                             \n\t"
                "mult            %[temp_coeff_11], %[temp2]                           \n\t"
                "mfhi            %[temp2]                                             \n\t"
                "mult            %[temp_coeff_22], %[temp3]                           \n\t"
                "mfhi            %[temp3]                                             \n\t"
                "mult            %[temp_coeff_33], %[temp4]                           \n\t"
                "mfhi            %[temp4]                                             \n\t"
                "mult            %[temp_coeff_44], %[temp5]                           \n\t"
                "mfhi            %[temp5]                                             \n\t"
                "sll             %[temp1],         %[temp1],         1                \n\t"
                "sll             %[temp2],         %[temp2],         1                \n\t"
                "sll             %[temp3],         %[temp3],         1                \n\t"
                "sll             %[temp4],         %[temp4],         1                \n\t"
                "sll             %[temp5],         %[temp5],         1                \n\t"

                :[g_code] "+r" (g_code),
                 [g2_pitch] "=&r" (g2_pitch), [g_pit_cod] "=&r" (g_pit_cod),
                 [g2_code] "=&r" (g2_code), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                 [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),[temp5] "=&r" (temp5)
                :[gcode0] "r" (gcode0),[temp_coeff_00] "r" (temp_coeff_00),
                 [temp_coeff_11] "r" (temp_coeff_11),[temp_coeff_22] "r" (temp_coeff_22),
                 [temp_coeff_33] "r" (temp_coeff_33),[temp_coeff_44] "r" (temp_coeff_44),
                 [temp_const] "r" (temp_const),[temp_const_1] "r" (temp_const_1),
                 [g_pitch] "r" (g_pitch)
                :"memory", "hi", "lo"
            );

            temp1 = L_add(temp2, temp1, pOverflow);
            temp1 = L_add(temp1, temp3, pOverflow);
            temp1 = L_add(temp1, temp4, pOverflow);
            temp1 = L_add(temp1, temp5, pOverflow);

            /* store table index if MSE for this index is lower
               than the minimum MSE seen so far */
            if (temp1 < dist_min)
            {
                dist_min = temp1;
                index = i;
            }
        }
#endif

    }

    /*------------------------------------------------------------------*
     *  read quantized gains and new values for MA predictor memories   *
     *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   *
     *------------------------------------------------------------------*/

    p = &table_gain[shl(index, 2, pOverflow)];
    gain_pit[0] = p[0];
    g_code = p[1];
    qua_ener_MR122[0] = p[2];
    qua_ener[0] = p[3];

    /*------------------------------------------------------------------*
     *  calculate final fixed codebook gain:                            *
     *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                            *
     *                                                                  *
     *   gc = gc0 * g                                                   *
     *------------------------------------------------------------------*/

    L_tmp = L_mult(g_code, gcode0, pOverflow);
    temp  = sub(10, exp_gcode0, pOverflow);
    L_tmp = L_shr(L_tmp, temp, pOverflow);

    *gain_cod = extract_h(L_tmp);

    return index;
}
