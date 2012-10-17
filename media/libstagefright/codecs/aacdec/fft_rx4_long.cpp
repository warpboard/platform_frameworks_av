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

 Pathname: ./src/fft_rx4_long.c
 Funtions: fft_rx4_long

------------------------------------------------------------------------------
 REVISION HISTORY

 Description:
            (1) Eliminated search for max in the main loop.
            (2) Reduced precision on w_256rx4 from Q15 to Q10

 Description:
            (1) Created function fft_rx4_long_no_max to overcome LTP problem.

 Description:
            (1) Modified shift so the accumulation growths faster than the
                downshift, so now the input can be as high as 1.0 and saturation
                will not occurre. The accumulation times the Q10 format will
                never exceed 31 bits. This increases precision
            (2) Eliminated unneeded data moves, used before for max search.
            (3) Eliminated function fft_rx4_long_no_max.

 Description:
            (1) Added comment to explain max search elimination and
                Q format during multiplications

 Who:                       Date:
 Description:

------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS

 Inputs:
    Data       =  Input complex vector, arranged in the following order:
                  real, imag, real, imag...
                  This is a complex vector whose elements (real and Imag) are
                  Int32.
                  type Int32 *

    peak_value =  Input,  peak value of the input vector
                  Output,  peak value of the resulting vector
                  type Int32 *

 Local Stores/Buffers/Pointers Needed:
    None

 Global Stores/Buffers/Pointers Needed:
    None

 Outputs:
    None

 Pointers and Buffers Modified:
    calculation are done in-place and returned in Data

 Local Stores Modified:
    None

 Global Stores Modified:
    None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION

    Fast Fourier Transform, radix 4 with Decimation in Frequency and block
    floating point arithmetic.
    The radix-4 FFT  simply divides the FFT into four smaller FFTs. Each of
    the smaller FFTs is then further divided into smaller ones and so on.
    It consists of log 4 N stages and each stage consists of N/4 dragonflies.

    An FFT is nothing but a bundle of multiplications and summations which
    may overflow during calculations.


    This routine uses a scheme to test and scale the result output from
    each FFT stage in order to fix the accumulation overflow.

    The Input Data should be in Q13 format to get the highest precision.
    At the end of each dragonfly calculation, a test for possible bit growth
    is made, if bit growth is possible the Data is scale down back to Q13.

------------------------------------------------------------------------------
 REQUIREMENTS

    This function should provide a fixed point FFT for an input array
    of size 256.

------------------------------------------------------------------------------
 REFERENCES

    [1] Advance Digital Signal Processing, J. Proakis, C. Rader, F. Ling,
        C. Nikias, Macmillan Pub. Co.

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

#include "pv_audio_type_defs.h"
#include "fft_rx4.h"

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

const Int32 W_256rx4_new[486] =
{
    0x7ff50000, 0x03240000, 0x7fd80000, 0x06480000, 0x7fa60000, 0x096a0000,
    0x7fd80000, 0x06480000, 0x7f610000, 0x0c8c0000, 0x7e9c0000, 0x12c80000,
    0x7fa60000, 0x096a0000, 0x7e9c0000, 0x12c80000, 0x7ce30000, 0x1c0b0000,
    0x7f610000, 0x0c8c0000, 0x7d890000, 0x18f90000, 0x7a7c0000, 0x25280000,
    0x7f090000, 0x0fab0000, 0x7c290000, 0x1f1a0000, 0x776b0000, 0x2e110000,
    0x7e9c0000, 0x12c80000, 0x7a7c0000, 0x25280000, 0x73b50000, 0x36ba0000,
    0x7e1d0000, 0x15e20000, 0x78840000, 0x2b1f0000, 0x6f5e0000, 0x3f170000,
    0x7d890000, 0x18f90000, 0x76410000, 0x30fb0000, 0x6a6d0000, 0x471c0000,
    0x7ce30000, 0x1c0b0000, 0x73b50000, 0x36ba0000, 0x64e80000, 0x4ebf0000,
    0x7c290000, 0x1f1a0000, 0x70e20000, 0x3c560000, 0x5ed70000, 0x55f50000,
    0x7b5c0000, 0x22230000, 0x6dc90000, 0x41ce0000, 0x58420000, 0x5cb30000,
    0x7a7c0000, 0x25280000, 0x6a6d0000, 0x471c0000, 0x51330000, 0x62f10000,
    0x79890000, 0x28260000, 0x66cf0000, 0x4c3f0000, 0x49b40000, 0x68a60000,
    0x78840000, 0x2b1f0000, 0x62f10000, 0x51330000, 0x41ce0000, 0x6dc90000,
    0x776b0000, 0x2e110000, 0x5ed70000, 0x55f50000, 0x398c0000, 0x72540000,
    0x76410000, 0x30fb0000, 0x5a820000, 0x5a820000, 0x30fb0000, 0x76410000,
    0x75040000, 0x33df0000, 0x55f50000, 0x5ed70000, 0x28260000, 0x79890000,
    0x73b50000, 0x36ba0000, 0x51330000, 0x62f10000, 0x1f1a0000, 0x7c290000,
    0x72540000, 0x398c0000, 0x4c3f0000, 0x66cf0000, 0x15e20000, 0x7e1d0000,
    0x70e20000, 0x3c560000, 0x471c0000, 0x6a6d0000, 0x0c8c0000, 0x7f610000,
    0x6f5e0000, 0x3f170000, 0x41ce0000, 0x6dc90000, 0x03240000, 0x7ff50000,
    0x6dc90000, 0x41ce0000, 0x3c560000, 0x70e20000, 0xf9b60000, 0x7fd80000,
    0x6c230000, 0x447a0000, 0x36ba0000, 0x73b50000, 0xf0530000, 0x7f090000,
    0x6a6d0000, 0x471c0000, 0x30fb0000, 0x76410000, 0xe7050000, 0x7d890000,
    0x68a60000, 0x49b40000, 0x2b1f0000, 0x78840000, 0xdddb0000, 0x7b5c0000,
    0x66cf0000, 0x4c3f0000, 0x25280000, 0x7a7c0000, 0xd4df0000, 0x78840000,
    0x64e80000, 0x4ebf0000, 0x1f1a0000, 0x7c290000, 0xcc1f0000, 0x75040000,
    0x62f10000, 0x51330000, 0x18f90000, 0x7d890000, 0xc3a80000, 0x70e20000,
    0x60eb0000, 0x539b0000, 0x12c80000, 0x7e9c0000, 0xbb840000, 0x6c230000,
    0x5ed70000, 0x55f50000, 0x0c8c0000, 0x7f610000, 0xb3bf0000, 0x66cf0000,
    0x5cb30000, 0x58420000, 0x06480000, 0x7fd80000, 0xac630000, 0x60eb0000,
    0x5a820000, 0x5a820000, 0x00000000, 0x7fff0000, 0xa57c0000, 0x5a820000,
    0x58420000, 0x5cb30000, 0xf9b60000, 0x7fd80000, 0x9f130000, 0x539b0000,
    0x55f50000, 0x5ed70000, 0xf3720000, 0x7f610000, 0x992f0000, 0x4c3f0000,
    0x539b0000, 0x60eb0000, 0xed360000, 0x7e9c0000, 0x93db0000, 0x447a0000,
    0x51330000, 0x62f10000, 0xe7050000, 0x7d890000, 0x8f1c0000, 0x3c560000,
    0x4ebf0000, 0x64e80000, 0xe0e40000, 0x7c290000, 0x8afa0000, 0x33df0000,
    0x4c3f0000, 0x66cf0000, 0xdad60000, 0x7a7c0000, 0x877a0000, 0x2b1f0000,
    0x49b40000, 0x68a60000, 0xd4df0000, 0x78840000, 0x84a20000, 0x22230000,
    0x471c0000, 0x6a6d0000, 0xcf030000, 0x76410000, 0x82750000, 0x18f90000,
    0x447a0000, 0x6c230000, 0xc9440000, 0x73b50000, 0x80f50000, 0x0fab0000,
    0x41ce0000, 0x6dc90000, 0xc3a80000, 0x70e20000, 0x80260000, 0x06480000,
    0x3f170000, 0x6f5e0000, 0xbe300000, 0x6dc90000, 0x80090000, 0xfcda0000,
    0x3c560000, 0x70e20000, 0xb8e20000, 0x6a6d0000, 0x809d0000, 0xf3720000,
    0x398c0000, 0x72540000, 0xb3bf0000, 0x66cf0000, 0x81e10000, 0xea1c0000,
    0x36ba0000, 0x73b50000, 0xaecb0000, 0x62f10000, 0x83d50000, 0xe0e40000,
    0x33df0000, 0x75040000, 0xaa090000, 0x5ed70000, 0x86750000, 0xd7d80000,
    0x30fb0000, 0x76410000, 0xa57c0000, 0x5a820000, 0x89bd0000, 0xcf030000,
    0x2e110000, 0x776b0000, 0xa1270000, 0x55f50000, 0x8daa0000, 0xc6720000,
    0x2b1f0000, 0x78840000, 0x9d0d0000, 0x51330000, 0x92350000, 0xbe300000,
    0x28260000, 0x79890000, 0x992f0000, 0x4c3f0000, 0x97580000, 0xb64a0000,
    0x25280000, 0x7a7c0000, 0x95910000, 0x471c0000, 0x9d0d0000, 0xaecb0000,
    0x22230000, 0x7b5c0000, 0x92350000, 0x41ce0000, 0xa34b0000, 0xa7bc0000,
    0x1f1a0000, 0x7c290000, 0x8f1c0000, 0x3c560000, 0xaa090000, 0xa1270000,
    0x1c0b0000, 0x7ce30000, 0x8c490000, 0x36ba0000, 0xb13f0000, 0x9b160000,
    0x18f90000, 0x7d890000, 0x89bd0000, 0x30fb0000, 0xb8e20000, 0x95910000,
    0x15e20000, 0x7e1d0000, 0x877a0000, 0x2b1f0000, 0xc0e70000, 0x90a00000,
    0x12c80000, 0x7e9c0000, 0x85820000, 0x25280000, 0xc9440000, 0x8c490000,
    0x0fab0000, 0x7f090000, 0x83d50000, 0x1f1a0000, 0xd1ed0000, 0x88930000,
    0x0c8c0000, 0x7f610000, 0x82750000, 0x18f90000, 0xdad60000, 0x85820000,
    0x096a0000, 0x7fa60000, 0x81620000, 0x12c80000, 0xe3f30000, 0x831b0000,
    0x06480000, 0x7fd80000, 0x809d0000, 0x0c8c0000, 0xed360000, 0x81620000,
    0x03240000, 0x7ff50000, 0x80260000, 0x06480000, 0xf6940000, 0x80580000,
    0x7f610000, 0x0c8c0000, 0x7d890000, 0x18f90000, 0x7a7c0000, 0x25280000,
    0x7d890000, 0x18f90000, 0x76410000, 0x30fb0000, 0x6a6d0000, 0x471c0000,
    0x7a7c0000, 0x25280000, 0x6a6d0000, 0x471c0000, 0x51330000, 0x62f10000,
    0x76410000, 0x30fb0000, 0x5a820000, 0x5a820000, 0x30fb0000, 0x76410000,
    0x70e20000, 0x3c560000, 0x471c0000, 0x6a6d0000, 0x0c8c0000, 0x7f610000,
    0x6a6d0000, 0x471c0000, 0x30fb0000, 0x76410000, 0xe7050000, 0x7d890000,
    0x62f10000, 0x51330000, 0x18f90000, 0x7d890000, 0xc3a80000, 0x70e20000,
    0x5a820000, 0x5a820000, 0x00000000, 0x7fff0000, 0xa57c0000, 0x5a820000,
    0x51330000, 0x62f10000, 0xe7050000, 0x7d890000, 0x8f1c0000, 0x3c560000,
    0x471c0000, 0x6a6d0000, 0xcf030000, 0x76410000, 0x82750000, 0x18f90000,
    0x3c560000, 0x70e20000, 0xb8e20000, 0x6a6d0000, 0x809d0000, 0xf3720000,
    0x30fb0000, 0x76410000, 0xa57c0000, 0x5a820000, 0x89bd0000, 0xcf030000,
    0x25280000, 0x7a7c0000, 0x95910000, 0x471c0000, 0x9d0d0000, 0xaecb0000,
    0x18f90000, 0x7d890000, 0x89bd0000, 0x30fb0000, 0xb8e20000, 0x95910000,
    0x0c8c0000, 0x7f610000, 0x82750000, 0x18f90000, 0xdad60000, 0x85820000,
    0x76410000, 0x30fb0000, 0x5a820000, 0x5a820000, 0x30fb0000, 0x76410000,
    0x5a820000, 0x5a820000, 0x00000000, 0x7fff0000, 0xa57c0000, 0x5a820000,
    0x30fb0000, 0x76410000, 0xa57c0000, 0x5a820000, 0x89bd0000, 0xcf030000
};

#endif /* MDSP_REV1 */


void fft_rx4_long(
    Int32      Data[],
    Int32      *peak_value)

{
    Int     n1;
    Int     n2;
    Int     j;
    Int     k;
    Int     i;

    Int32   t1;
    Int32   t2;
    Int32   r1;
    Int32   r2;
    Int32   r3;
    Int32   r4;
    Int32   s1;
    Int32   s2;
    Int32   s3;
    Int32   *pData1;
    Int32   *pData2;
    Int32   *pData3;
    Int32   *pData4;
    Int32   temp1;
    Int32   temp2;
    Int32   temp3;
    Int32   temp4;
    Int32   max;

#ifndef MDSP_REV1

    Int32   exp_jw1;
    Int32   exp_jw2;
    Int32   exp_jw3;

    const Int32  *pw = W_256rx4;

#else /* MDSP_REV1 */

    Int32  rTmp0, rTmp1;
    Int32  iTmp0, iTmp1;
    Int32  z, z1;

    const Int32  *p_w = W_256rx4_new;

#endif /* MDSP_REV1 */

    n2 = FFT_RX4_LONG;

    for (k = FFT_RX4_LONG; k > 4; k >>= 2)
    {

        n1 = n2;
        n2 >>= 2;

        for (i = 0; i < FFT_RX4_LONG; i += n1)
        {
            pData1 = &Data[ i<<1];
            pData2 = pData1 + n1;

            temp1   = *pData1;
            temp2   = *pData2;

            r1      = temp1 + temp2;
            r2      = temp1 - temp2;

            pData3 = pData1 + (n1 >> 1);
            pData4 = pData3 + n1;
            temp3   = *pData3++;
            temp4   = *pData4++;

            t1      = temp3 + temp4;

            *(pData1++) = (r1 + t1);
            t2      = temp3 - temp4;
            *(pData2++) = (r1 - t1);

            temp1   = *pData1;
            temp2   = *pData2;

            s1      = temp1 + temp2;
            temp3   = *pData3;
            s2      = temp1 - temp2;
            temp4   = *pData4;
            *pData3--  = (s2 - t2);
            *pData4--  = (s2 + t2);

            t1      = temp3 + temp4;

            *pData1    = (s1 + t1);
            *pData2    = (s1 - t1);

            r1      = temp3 - temp4;

            *pData4    = (r2 - r1);
            *pData3    = (r2 + r1);

        }  /* i */

#ifndef MDSP_REV1

        for (j = 1; j < n2; j++)
        {

            exp_jw1 = (*pw++);
            exp_jw2 = (*pw++);
            exp_jw3 = (*pw++);


            for (i = j; i < FFT_RX4_LONG; i += n1)
            {
                pData1 = &Data[ i<<1];
                pData2 = pData1 + n1;

                temp1   = *pData1;
                temp2   = *pData2++;

                r1      = temp1 + temp2;
                r2      = temp1 - temp2;

                pData3 = pData1 + (n1 >> 1);
                pData4 = pData3 + n1;
                temp3   = *pData3++;
                temp4   = *pData4++;

                r3      = temp3 + temp4;
                r4      = temp3 - temp4;

                *(pData1++) = (r1 + r3);
                r1          = (r1 - r3) << 1;

                temp2   = *pData2;
                temp1   = *pData1;

                s1      = temp1 + temp2;
                s2      = temp1 - temp2;
                s3      = (s2 + r4) << 1;
                s2      = (s2 - r4) << 1;

                temp3   = *pData3;
                temp4   = *pData4;

                t1      = temp3 + temp4;
                t2      = temp3 - temp4;

                *pData1  = (s1 + t1);
                s1       = (s1 - t1) << 1;

                *pData2--  = cmplx_mul32_by_16(s1, -r1, exp_jw2);
                r3      = (r2 - t2) << 1;
                *pData2    = cmplx_mul32_by_16(r1,  s1, exp_jw2);

                r2      = (r2 + t2) << 1;

                *pData3--  = cmplx_mul32_by_16(s2, -r2, exp_jw1);
                *pData3    = cmplx_mul32_by_16(r2,  s2, exp_jw1);

                *pData4--  = cmplx_mul32_by_16(s3, -r3, exp_jw3);
                *pData4    = cmplx_mul32_by_16(r3,  s3, exp_jw3);

            }  /* i */

        }  /*  j */

    } /* k */

#else /* MDSP_REV1 */

        for (j = 1; j < n2; j++)
        {

            for (i = j; i < FFT_RX4_LONG; i += n1)
            {
                pData1 = &Data[ i<<1];
                pData2 = pData1 + n1;

                temp1   = *pData1;
                temp2   = *pData2++;

                r1      = temp1 + temp2;
                r2      = temp1 - temp2;

                pData3 = pData1 + (n1 >> 1);
                pData4 = pData3 + n1;
                temp3   = *pData3++;
                temp4   = *pData4++;

                r3      = temp3 + temp4;
                r4      = temp3 - temp4;

                *(pData1++) = (r1 + r3);
                r1          = (r1 - r3) << 1;

                temp2   = *pData2;
                temp1   = *pData1;

                s1      = temp1 + temp2;
                s2      = temp1 - temp2;
                s3      = (s2 + r4) << 1;
                s2      = (s2 - r4) << 1;

                temp3   = *pData3;
                temp4   = *pData4;

                t1      = temp3 + temp4;
                t2      = temp3 - temp4;

                *pData1  = (s1 + t1);
                s1       = (s1 - t1) << 1;

                __asm__ volatile(

                    "lw         %[rTmp0],     8(%[p_w])                 \n\t"
                    "lw         %[iTmp0],     12(%[p_w])                \n\t"
                    "subu       %[r3],        %[r2],        %[t2]       \n\t"
                    "addu       %[r2],        %[r2],        %[t2]       \n\t"
                    "lw         %[rTmp1],     0(%[p_w])                 \n\t"
                    "lw         %[iTmp1],     4(%[p_w])                 \n\t"
                    "sll        %[r3],        %[r3],        1           \n\t"
                    "sll        %[r2],        %[r2],        1           \n\t"
                    "mult       $ac0,         %[rTmp0],     %[s1]       \n\t"
                    "mult       $ac1,         %[rTmp0],     %[r1]       \n\t"
                    "mult       $ac2,         %[rTmp1],     %[s2]       \n\t"
                    "mult       $ac3,         %[rTmp1],     %[r2]       \n\t"
                    "msub       $ac0,         %[r1],        %[iTmp0]    \n\t"
                    "madd       $ac1,         %[iTmp0],     %[s1]       \n\t"
                    "msub       $ac2,         %[r2],        %[iTmp1]    \n\t"
                    "madd       $ac3,         %[iTmp1],     %[s2]       \n\t"
                    "mfhi       %[z],         $ac0                      \n\t"
                    "mfhi       %[z1],        $ac1                      \n\t"
                    "mfhi       %[iTmp1],     $ac2                      \n\t"
                    "lw         %[rTmp0],     16(%[p_w])                \n\t"
                    "mfhi       %[rTmp1],     $ac3                      \n\t"
                    "lw         %[iTmp0],     20(%[p_w])                \n\t"
                    "mult       $ac0,         %[rTmp0],     %[s3]       \n\t"
                    "sw         %[z],         0(%[pData2])              \n\t"
                    "sw         %[z1],        -4(%[pData2])             \n\t"
                    "mult       $ac1,         %[rTmp0],     %[r3]       \n\t"
                    "sw         %[iTmp1],     0(%[pData3])              \n\t"
                    "sw         %[rTmp1],     -4(%[pData3])             \n\t"
                    "msub       $ac0,         %[r3],        %[iTmp0]    \n\t"
                    "madd       $ac1,         %[iTmp0],     %[s3]       \n\t"
                    "addiu      %[pData2],    %[pData2],    -4          \n\t"
                    "addiu      %[pData3],    %[pData3],    -4          \n\t"
                    "mfhi       %[z],         $ac0                      \n\t"
                    "mfhi       %[iTmp1],     $ac1                      \n\t"
                    "sw         %[z],         0(%[pData4])              \n\t"
                    "sw         %[iTmp1],     -4(%[pData4])             \n\t"
                    "addiu      %[pData4],    %[pData4],    -4          \n\t"

                    : [rTmp0] "=&r" (rTmp0), [rTmp1] "=&r" (rTmp1), [z] "=&r" (z),
                      [iTmp0] "=&r" (iTmp0), [iTmp1] "=&r" (iTmp1), [z1] "=&r" (z1),
                      [pData2] "+r" (pData2), [pData3] "+r" (pData3), [r2] "+r" (r2),
                      [pData4] "+r" (pData4), [r3] "=&r" (r3)
                    : [r1] "r" (r1), [s1] "r" (s1), [s2] "r" (s2), [s3] "r" (s3),
                      [t2] "r" (t2), [p_w] "r" (p_w)
                    : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo",
                      "memory"
                );

            }  /* i */

            p_w += 6;

        }  /*  j */

    } /* k */

#endif /* MDSP_REV1 */

    max = 0;

    pData1 = Data - 7;


    for (i = ONE_FOURTH_FFT_RX4_LONG; i != 0 ; i--)
    {
        pData1 += 7;
        pData2 = pData1 + 4;


        temp1   = *pData1;
        temp2   = *pData2++;

        r1      = temp1 + temp2;
        r2      = temp1 - temp2;

        pData3 = pData1 + 2;
        pData4 = pData1 + 6;
        temp1   = *pData3++;
        temp2   = *pData4++;

        t1      = temp1 + temp2;
        t2      = temp1 - temp2;

        temp1       = (r1 + t1);
        r1          = (r1 - t1);
        *(pData1++) = temp1;
        max        |= (temp1 >> 31) ^ temp1;



        temp2   = *pData2;
        temp1   = *pData1;

        s1      = temp1 + temp2;
        s2      = temp1 - temp2;


        temp1   = *pData3;
        temp2   = *pData4;

        s3      = (s2 + t2);
        s2      = (s2 - t2);

        t1      = temp1 + temp2;
        t2      = temp1 - temp2;

        temp1      = (s1 + t1);
        *pData1    = temp1;
        temp2      = (s1 - t1);

        max       |= (temp1 >> 31) ^ temp1;
        *pData2--  = temp2;
        max       |= (temp2 >> 31) ^ temp2;

        *pData2    = r1;
        max       |= (r1 >> 31) ^ r1;
        *pData3--  = s2;
        max       |= (s2 >> 31) ^ s2;
        *pData4--  = s3;
        max       |= (s3 >> 31) ^ s3;

        temp1      = (r2 - t2);
        *pData4    = temp1;
        temp2      = (r2 + t2);
        *pData3    = temp2;
        max       |= (temp1 >> 31) ^ temp1;
        max       |= (temp2 >> 31) ^ temp2;

    }  /* i */

    *peak_value = max;

    return ;

}

