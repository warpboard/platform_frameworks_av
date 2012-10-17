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
/*

 Pathname: mix_radix_fft.c
 Funtions: mix_radix_fft

------------------------------------------------------------------------------
 REVISION HISTORY

 Description:  Eliminated pointer dependency ( pData_1) on Buffer address.
               Modified for-loop to countdown loops.

 Description:  No shift information going in/out from fft_rx4_long.

 Description:
            (1) Increased precision on the radix 2 fft coeff. (from Q10 to Q12)
            (2) Increased precision on the input (from 0.5 to 1.0).
            (3) Eliminated hardly used condition (exp = 0).
            (4) Change interface to fft_rx4_long, so now the same function is
                used for forward and inverse calculations.

 Description:  per code review comments, eliminated unnecessary headers

 Who:                       Date:
 Description:

------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    Data         = Input vector, with quantized spectral with a pre-rotation
                   by exp(j(2pi/N)(k+1/8))
                   type Int32 *

    peak_value   = Input, carries the maximum value in input vector "Data"
                   Output, maximum value computed in the first FFT, used
                   to set precision on next stages
                   type Int32 *


 Local Stores/Buffers/Pointers Needed:
    None

 Global Stores/Buffers/Pointers Needed:
    None

 Outputs:
    exponent = shift factor to reflect signal scaling

 Pointers and Buffers Modified:
    Results are return in "Data"

 Local Stores Modified:
    None

 Global Stores Modified:
    None
------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

    mix_radix_fft() mixes radix-2 and radix-4 FFT. This is needed to be able
    to use power of 4 length when the input length sequence is a power of 2.
------------------------------------------------------------------------------
 REQUIREMENTS

    mix_radix_fft() should support only the FFT for the long window case of
    the inverse modified cosine transform (IMDCT)
------------------------------------------------------------------------------
 REFERENCES

  ------------------------------------------------------------------------------
 PSEUDO-CODE


   MODIFY( x[] )
   RETURN( exponent )

------------------------------------------------------------------------------
 RESOURCES USED
   When the code is written for a specific target processor the
     the resources used should be documented below.

 STACK USAGE: [stack count for this module] + [variable to represent
          stack usage for each subroutine called]

     where: [stack usage variable] = stack usage for [subroutine
         name] (see [filename].ext)

 DATA MEMORY USED: x words

 PROGRAM MEMORY USED: x words

 CLOCK CYCLES: [cycle count equation for this module] + [variable
           used to represent cycle count for each subroutine
           called]

     where: [cycle count variable] = cycle count for [subroutine
        name] (see [filename].ext)

------------------------------------------------------------------------------
*/
/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/

#include "fft_rx4.h"
#include "mix_radix_fft.h"
#include "pv_normalize.h"

#include "fxp_mul32.h"

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
; EXTERNAL FUNCTION REFERENCES
; Declare functions defined elsewhere and referenced in this module
----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

    void digit_reversal_swapping(Int32 *y, Int32 *x);

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
; EXTERNAL VARIABLES REFERENCES
; Declare variables used in this module but defined elsewhere
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; EXTERNAL GLOBAL STORE/BUFFER/POINTER REFERENCES
; Declare variables used in this module but defined elsewhere
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
; FUNCTION CODE
----------------------------------------------------------------------------*/

#ifdef MDSP_REV1

int w_512rx2_modified[254] =
{
    0x7ffe0000, 0x01920000, 0x7ff60000, 0x03240000, 0x7fea0000, 0x04b60000, 0x7fd90000,
    0x06480000, 0x7fc20000, 0x07d90000, 0x7fa70000, 0x096b0000, 0x7f870000, 0x0afb0000,
    0x7f620000, 0x0c8c0000, 0x7f380000, 0x0e1c0000, 0x7f0a0000, 0x0fab0000, 0x7ed60000,
    0x113a0000, 0x7e9d0000, 0x12c80000, 0x7e600000, 0x14550000, 0x7e1e0000, 0x15e20000,
    0x7dd60000, 0x176e0000, 0x7d8a0000, 0x18f90000, 0x7d3a0000, 0x1a830000, 0x7ce40000,
    0x1c0c0000, 0x7c890000, 0x1d930000, 0x7c2a0000, 0x1f1a0000, 0x7bc60000, 0x209f0000,
    0x7b5d0000, 0x22240000, 0x7aef0000, 0x23a70000, 0x7a7d0000, 0x25280000, 0x7a060000,
    0x26a80000, 0x798a0000, 0x28270000, 0x790a0000, 0x29a40000, 0x78850000, 0x2b1f0000,
    0x77fb0000, 0x2c990000, 0x776c0000, 0x2e110000, 0x76d90000, 0x2f870000, 0x76420000,
    0x30fc0000, 0x75a60000, 0x326e0000, 0x75050000, 0x33df0000, 0x74600000, 0x354e0000,
    0x73b60000, 0x36ba0000, 0x73080000, 0x38250000, 0x72550000, 0x398d0000, 0x719e0000,
    0x3af30000, 0x70e30000, 0x3c570000, 0x70230000, 0x3db80000, 0x6f5f0000, 0x3f170000,
    0x6e970000, 0x40740000, 0x6dca0000, 0x41ce0000, 0x6cf90000, 0x43260000, 0x6c240000,
    0x447b0000, 0x6b4b0000, 0x45cd0000, 0x6a6e0000, 0x471d0000, 0x698c0000, 0x486a0000,
    0x68a70000, 0x49b40000, 0x67bd0000, 0x4afb0000, 0x66d00000, 0x4c400000, 0x65de0000,
    0x4d810000, 0x64e90000, 0x4ec00000, 0x63ef0000, 0x4ffb0000, 0x62f20000, 0x51340000,
    0x61f10000, 0x52690000, 0x60ec0000, 0x539b0000, 0x5fe40000, 0x54ca0000, 0x5ed70000,
    0x55f60000, 0x5dc80000, 0x571e0000, 0x5cb40000, 0x58430000, 0x5b9d0000, 0x59640000,
    0x5a820000, 0x5a820000, 0x59640000, 0x5b9d0000, 0x58430000, 0x5cb40000, 0x571e0000,
    0x5dc80000, 0x55f60000, 0x5ed70000, 0x54ca0000, 0x5fe40000, 0x539b0000, 0x60ec0000,
    0x52690000, 0x61f10000, 0x51340000, 0x62f20000, 0x4ffb0000, 0x63ef0000, 0x4ec00000,
    0x64e90000, 0x4d810000, 0x65de0000, 0x4c400000, 0x66d00000, 0x4afb0000, 0x67bd0000,
    0x49b40000, 0x68a70000, 0x486a0000, 0x698c0000, 0x471d0000, 0x6a6e0000, 0x45cd0000,
    0x6b4b0000, 0x447b0000, 0x6c240000, 0x43260000, 0x6cf90000, 0x41ce0000, 0x6dca0000,
    0x40740000, 0x6e970000, 0x3f170000, 0x6f5f0000, 0x3db80000, 0x70230000, 0x3c570000,
    0x70e30000, 0x3af30000, 0x719e0000, 0x398d0000, 0x72550000, 0x38250000, 0x73080000,
    0x36ba0000, 0x73b60000, 0x354e0000, 0x74600000, 0x33df0000, 0x75050000, 0x326e0000,
    0x75a60000, 0x30fc0000, 0x76420000, 0x2f870000, 0x76d90000, 0x2e110000, 0x776c0000,
    0x2c990000, 0x77fb0000, 0x2b1f0000, 0x78850000, 0x29a40000, 0x790a0000, 0x28270000,
    0x798a0000, 0x26a80000, 0x7a060000, 0x25280000, 0x7a7d0000, 0x23a70000, 0x7aef0000,
    0x22240000, 0x7b5d0000, 0x209f0000, 0x7bc60000, 0x1f1a0000, 0x7c2a0000, 0x1d930000,
    0x7c890000, 0x1c0c0000, 0x7ce40000, 0x1a830000, 0x7d3a0000, 0x18f90000, 0x7d8a0000,
    0x176e0000, 0x7dd60000, 0x15e20000, 0x7e1e0000, 0x14550000, 0x7e600000, 0x12c80000,
    0x7e9d0000, 0x113a0000, 0x7ed60000, 0x0fab0000, 0x7f0a0000, 0x0e1c0000, 0x7f380000,
    0x0c8c0000, 0x7f620000, 0x0afb0000, 0x7f870000, 0x096b0000, 0x7fa70000, 0x07d90000,
    0x7fc20000, 0x06480000, 0x7fd90000, 0x04b60000, 0x7fea0000, 0x03240000, 0x7ff60000,
    0x01920000, 0x7ffe0000
};

#endif /* MDSP_REV1 */


Int mix_radix_fft(
    Int32   *Data,
    Int32   *peak_value
)

{

#ifndef MDSP_REV1

    const Int32     *p_w;
    Int32   *pData_1;
    Int32   *pData_2;

    Int32   *pData_3;
    Int32   *pData_4;

    Int32   exp_jw;
    Int32   max1;
    Int32   max2;
    Int32   temp1;
    Int32   temp2;
    Int32   temp3;
    Int32   temp4;
    Int32   diff1;
    Int32   diff2;
    Int     i;
    Int   exp;

    max1 = *peak_value;
    p_w  = w_512rx2;

    pData_1 = Data;
    pData_3 = Data + HALF_FFT_RX4_LENGTH_FOR_LONG;

#else /* MDSP_REV1 */

    Int32 *p_w, *pData_1, exp_jw, exp_jw1, end;
    Int32 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7, temp8;
    Int32 exp, exp_4, max1, max2;

    max1 = *peak_value;
    p_w  = w_512rx2_modified;
    pData_1 = Data;

#endif /* MDSP_REV1 */

#ifndef MDSP_REV1

    /*
     * normalization to 0.9999 (0x7FFF) guarantees proper operation
     */

    exp = 8 - pv_normalize(max1);   /* use 24 bits for mix radix fft */

    if (exp < 4)
    {
        exp = 4;
    }


    temp1      = (*pData_3);
    pData_4    = pData_3 + FFT_RX4_LENGTH_FOR_LONG;
    temp2      = (*pData_4++);



    diff1      = (temp1  - temp2) >> exp;
    *pData_3++ = (temp1  + temp2) >> exp;

    temp3      = (*pData_3);
    temp4      = (*pData_4);

    *pData_4-- = -diff1;
    *pData_3++ = (temp3  + temp4) >> exp;
    *pData_4   = (temp3  - temp4) >> exp;

    temp1      = (*pData_1);
    pData_2    = pData_1 + FFT_RX4_LENGTH_FOR_LONG;
    temp2      = (*pData_2++);
    temp4      = (*pData_2);

    *pData_1++ = (temp1  + temp2) >> exp;

    temp3      = (*pData_1);
    diff1      = (temp1  - temp2) >> exp ;

    *pData_1++ = (temp3  + temp4) >> exp;
    *pData_2-- = (temp3  - temp4) >> exp;
    *pData_2   =  diff1;

    temp1      = (*pData_3);
    pData_4    = pData_3 + FFT_RX4_LENGTH_FOR_LONG;
    temp2      = (*pData_4++);


    for (i = ONE_FOURTH_FFT_RX4_LENGTH_FOR_LONG - 1; i != 0; i--)
    {
        /*
         * radix 2 Butterfly
         */

        diff1      = (temp1  - temp2) >> (exp - 4);
        *pData_3++ = (temp1  + temp2) >> exp;

        temp3      = (*pData_3);
        temp4      = (*pData_4);

        exp_jw     = *p_w++;


        diff2      = (temp3  - temp4) >> (exp - 4);
        *pData_3++ = (temp3  + temp4) >> exp;

        *pData_4-- = -cmplx_mul32_by_16(diff1,  diff2, exp_jw) >> 3;
        *pData_4   =  cmplx_mul32_by_16(diff2, -diff1, exp_jw) >> 3;


        temp1      = (*pData_1);
        pData_2    = pData_1 + FFT_RX4_LENGTH_FOR_LONG;
        temp2      = (*pData_2++);
        temp4      = (*pData_2);

        *pData_1++ = (temp1  + temp2) >> exp;

        temp3      = (*pData_1);
        diff1      = (temp1  - temp2) >> (exp - 4);

        diff2      = (temp3  - temp4) >> (exp - 4);
        *pData_1++ = (temp3  + temp4) >> exp;

        *pData_2-- =  cmplx_mul32_by_16(diff2, -diff1, exp_jw) >> 3;
        *pData_2   =  cmplx_mul32_by_16(diff1,  diff2, exp_jw) >> 3;

        temp1      = (*pData_3);
        pData_4    = pData_3 + FFT_RX4_LENGTH_FOR_LONG;
        temp2      = (*pData_4++);

    }/* for i  */


    fft_rx4_long(
        Data,
        &max1);


    fft_rx4_long(
        &Data[FFT_RX4_LENGTH_FOR_LONG],
        &max2);

    digit_reversal_swapping(Data, &Data[FFT_RX4_LENGTH_FOR_LONG]);

    *peak_value = max1 | max2;

    return(exp);

#else /* MDSP_REV1 */

    __asm__ volatile(
        ".set    push                                   \n\t"
        ".set    noreorder                              \n\t"

        "clz     %[temp0],   %[max1]                    \n\t"
        "subu    %[exp],     $zero,         %[temp0]    \n\t"
        "addiu   %[exp],     %[exp],        9           \n\t"
        "addiu   %[temp3],   $zero,         4           \n\t"
        "lw      %[temp1],   1024(%[pData_1])           \n\t"
        "lw      %[temp2],   3072(%[pData_1])           \n\t"
        "slt     %[temp0],   %[exp],        %[temp3]    \n\t"
        "movn    %[exp],     %[temp3],      %[temp0]    \n\t"
        "subu    %[temp0],   %[temp1],      %[temp2]    \n\t"
        "addu    %[temp5],   %[temp1],      %[temp2]    \n\t"
        "sra     %[temp0],   %[temp0],      %[exp]      \n\t"
        "sra     %[temp5],   %[temp5],      %[exp]      \n\t"
        "addiu   %[exp_4],   %[exp],        -4          \n\t"
        "lw      %[temp3],   1028(%[pData_1])           \n\t"
        "lw      %[temp4],   3076(%[pData_1])           \n\t"
        "subu    %[temp0],   $zero,         %[temp0]    \n\t"
        "sw      %[temp5],   1024(%[pData_1])           \n\t"
        "sw      %[temp0],   3076(%[pData_1])           \n\t"
        "addu    %[temp0],   %[temp3],      %[temp4]    \n\t"
        "subu    %[temp5],   %[temp3],      %[temp4]    \n\t"
        "sra     %[temp0],   %[temp0],      %[exp]      \n\t"
        "sra     %[temp5],   %[temp5],      %[exp]      \n\t"
        "lw      %[temp1],   0(%[pData_1])              \n\t"
        "lw      %[temp2],   2048(%[pData_1])           \n\t"
        "lw      %[temp4],   2052(%[pData_1])           \n\t"
        "sw      %[temp0],   1028(%[pData_1])           \n\t"
        "sw      %[temp5],   3072(%[pData_1])           \n\t"
        "addu    %[temp0],   %[temp1],      %[temp2]    \n\t"
        "subu    %[temp5],   %[temp1],      %[temp2]    \n\t"
        "sra     %[temp0],   %[temp0],      %[exp]      \n\t"
        "sra     %[temp6],   %[temp5],      %[exp]      \n\t"
        "lw      %[temp3],   4(%[pData_1])              \n\t"
        "sw      %[temp0],   0(%[pData_1])              \n\t"
        "addu    %[temp0],   %[temp3],      %[temp4]    \n\t"
        "subu    %[temp5],   %[temp3],      %[temp4]    \n\t"
        "sra     %[temp0],   %[temp0],      %[exp]      \n\t"
        "sra     %[temp5],   %[temp5],      %[exp]      \n\t"
        "sw      %[temp0],   4(%[pData_1])              \n\t"
        "addiu   %[pData_1], %[pData_1],    8           \n\t"
        "lw      %[temp1],   1024(%[pData_1])           \n\t"
        "lw      %[temp2],   3072(%[pData_1])           \n\t"
        "sw      %[temp5],   2044(%[pData_1])           \n\t"
        "sw      %[temp6],   2040(%[pData_1])           \n\t"
        "addiu   %[end],     %[pData_1],    1016        \n\t"
    "1:                                                 \n\t"
        "subu    %[temp0],   %[temp1],      %[temp2]    \n\t"
        "addu    %[temp5],   %[temp1],      %[temp2]    \n\t"
        "srav    %[temp0],   %[temp0],      %[exp_4]    \n\t"
        "srav    %[temp5],   %[temp5],      %[exp]      \n\t"
        "lw      %[temp3],   1028(%[pData_1])           \n\t"
        "lw      %[temp4],   3076(%[pData_1])           \n\t"
        "lw      %[exp_jw1], 0(%[p_w])                  \n\t"
        "lw      %[exp_jw],  4(%[p_w])                  \n\t"
        "addiu   %[p_w],     %[p_w],        8           \n\t"
        "sw      %[temp5],   1024(%[pData_1])           \n\t"
        "subu    %[temp1],   %[temp3],      %[temp4]    \n\t"
        "addu    %[temp5],   %[temp3],      %[temp4]    \n\t"
        "srav    %[temp1],   %[temp1],      %[exp_4]    \n\t"
        "srav    %[temp5],   %[temp5],      %[exp]      \n\t"
        "sw      %[temp5],   1028(%[pData_1])           \n\t"
        "mult    $ac0,       %[temp0],      %[exp_jw1]  \n\t"
        "madd    $ac0,       %[temp1],      %[exp_jw]   \n\t"
        "mult    $ac1,       %[temp1],      %[exp_jw1]  \n\t"
        "msub    $ac1,       %[temp0],      %[exp_jw]   \n\t"
        "lw      %[temp7],   2048(%[pData_1])           \n\t"
        "lw      %[temp6],   0(%[pData_1])              \n\t"
        "mfhi    %[temp2],   $ac0                       \n\t"
        "lw      %[temp4],   2052(%[pData_1])           \n\t"
        "mfhi    %[temp1],   $ac1                       \n\t"
        "lw      %[temp3],   4(%[pData_1])              \n\t"
        "addu    %[temp8],   %[temp6],      %[temp7]    \n\t"
        "srav    %[temp8],   %[temp8],      %[exp]      \n\t"
        "sw      %[temp8],   0(%[pData_1])              \n\t"
        "subu    %[temp0],   %[temp3],      %[temp4]    \n\t"
        "subu    %[temp2],   $zero,         %[temp2]    \n\t"
        "sra     %[temp2],   %[temp2],      3           \n\t"
        "sra     %[temp1],   %[temp1],      3           \n\t"
        "sw      %[temp2],   3076(%[pData_1])           \n\t"
        "sw      %[temp1],   3072(%[pData_1])           \n\t"
        "subu    %[temp5],   %[temp6],      %[temp7]    \n\t"
        "srav    %[temp6],   %[temp0],      %[exp_4]    \n\t"
        "srav    %[temp5],   %[temp5],      %[exp_4]    \n\t"
        "mult    $ac0,       %[temp6],      %[exp_jw1]  \n\t"
        "msub    $ac0,       %[temp5],      %[exp_jw]   \n\t"
        "mult    $ac1,       %[temp5],      %[exp_jw1]  \n\t"
        "madd    $ac1,       %[temp6],      %[exp_jw]   \n\t"
        "addu    %[temp7],   %[temp3],      %[temp4]    \n\t"
        "srav    %[temp7],   %[temp7],      %[exp]      \n\t"
        "mfhi    %[temp0],   $ac0                       \n\t"
        "sw      %[temp7],   4(%[pData_1])              \n\t"
        "mfhi    %[temp6],   $ac1                       \n\t"
        "lw      %[temp1],   1032(%[pData_1])           \n\t"
        "lw      %[temp2],   3080(%[pData_1])           \n\t"
        "addiu   %[pData_1], %[pData_1],    8           \n\t"
        "sra     %[temp0],   %[temp0],      3           \n\t"
        "sra     %[temp6],   %[temp6],      3           \n\t"
        "sw      %[temp0],   2044(%[pData_1])           \n\t"
        "bne     %[pData_1], %[end],        1b          \n\t"
        " sw     %[temp6],   2040(%[pData_1])           \n\t"

        ".set    pop                                    \n\t"
        : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
          [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [temp5] "=&r" (temp5),
          [temp6] "=&r" (temp6), [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
          [p_w] "+r" (p_w), [exp_jw] "=&r" (exp_jw), [exp_jw1] "=&r" (exp_jw1),
          [pData_1] "+r" (pData_1), [end] "=&r" (end),
          [exp_4] "=&r" (exp_4), [exp] "=&r" (exp)
        : [max1] "r" (max1)
        : "memory", "hi", "lo", "$ac1hi", "$ac1lo"
    );


    fft_rx4_long(
        Data,
        &max1);


    fft_rx4_long(
        &Data[FFT_RX4_LENGTH_FOR_LONG],
        &max2);

    digit_reversal_swapping(Data, &Data[FFT_RX4_LENGTH_FOR_LONG]);

    *peak_value = max1 | max2;

    return(exp);

#endif /* MDSP_REV1 */

}

