/*
 ** Copyright 2003-2010, VisualOn, Inc.
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

/*****************************************************************************
 *  This file contains operations in double precision.                       *
 *  These operations are not standard double precision operations.           *
 *  They are used where single precision is not enough but the full 32 bits  *
 *  precision is not necessary. For example, the function Div_32() has a     *
 *  24 bits precision which is enough for our purposes.                      *
 *                                                                           *
 *  The double precision numbers use a special representation:               *
 *                                                                           *
 *     L_32 = hi<<16 + lo<<1                                                 *
 *                                                                           *
 *  L_32 is a 32 bit integer.                                                *
 *  hi and lo are 16 bit signed integers.                                    *
 *  As the low part also contains the sign, this allows fast multiplication. *
 *                                                                           *
 *      0x8000 0000 <= L_32 <= 0x7fff fffe.                                  *
 *                                                                           *
 *  We will use DPF (Double Precision Format )in this file to specify        *
 *  this special format.                                                     *
 *****************************************************************************
 *                                                                           *
 * Optimized for MIPS architecture                                           *
 *                                                                           *
******************************************************************************/
#include "typedef.h"
#include "basic_op.h"
#include "oper_32b.h"

/*****************************************************************************
 * Function Mpy_32()                                                         *
 *                                                                           *
 *   Multiply two 32 bit integers (DPF). The result is divided by 2**31      *
 *                                                                           *
 *   L_32 = (hi1*hi2)<<1 + ( (hi1*lo2)>>15 + (lo1*hi2)>>15 )<<1              *
 *                                                                           *
 *   This operation can also be viewed as the multiplication of two Q31      *
 *   number and the result is also in Q31.                                   *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 *  hi1         hi part of first number                                      *
 *  lo1         lo part of first number                                      *
 *  hi2         hi part of second number                                     *
 *  lo2         lo part of second number                                     *
 *                                                                           *
 *****************************************************************************
*/

__inline Word32  Mpy_32 (Word16 hi1, Word16 lo1, Word16 hi2, Word16 lo2)
{
    Word32 lo1_t, lo2_t, lo3_t;
    __asm__ volatile (
        "muleq_s.w.phr      %[lo3_t],       %[hi1],     %[lo2]      \n\t"
        "muleq_s.w.phr      %[lo2_t],       %[hi2],     %[lo1]      \n\t"
        "muleq_s.w.phr      %[lo1_t],       %[hi1],     %[hi2]      \n\t"
        "sra                %[lo3_t],       %[lo3_t],   16          \n\t"
        "sra                %[lo2_t],       %[lo2_t],   16          \n\t"
        "sll                %[lo3_t],       %[lo3_t],   1           \n\t"
        "sll                %[lo2_t],       %[lo2_t],   1           \n\t"
        "addu               %[lo3_t],       %[lo3_t],   %[lo1_t]    \n\t"
        "addu               %[lo3_t],       %[lo2_t],   %[lo3_t]    \n\t"

        : [lo1_t] "=&r" (lo1_t), [lo2_t] "=&r" (lo2_t), [lo3_t] "=&r" (lo3_t)
        : [hi1] "r" (hi1), [hi2] "r" (hi2), [lo1] "r" (lo1), [lo2] "r" (lo2)
    );
    return (lo3_t);
}

/*****************************************************************************
 * Function Mpy_32_16()                                                      *
 *                                                                           *
 *   Multiply a 16 bit integer by a 32 bit (DPF). The result is divided      *
 *   by 2**15                                                                *
 *                                                                           *
 *                                                                           *
 *   L_32 = (hi1*lo2)<<1 + ((lo1*lo2)>>15)<<1                                *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 *  hi          hi part of 32 bit number.                                    *
 *  lo          lo part of 32 bit number.                                    *
 *  n           16 bit number.                                               *
 *                                                                           *
 *****************************************************************************
*/

__inline Word32 Mpy_32_16 (Word16 hi, Word16 lo, Word16 n)
{
    Word32 lo1_t, lo2_t;
    __asm__ volatile (
        "muleq_s.w.phr      %[lo2_t],       %[n],       %[lo]       \n\t"
        "muleq_s.w.phr      %[lo1_t],       %[n],       %[hi]       \n\t"
        "sra                %[lo2_t],       %[lo2_t],   16          \n\t"
        "sll                %[lo2_t],       %[lo2_t],   1           \n\t"
        "addu               %[lo2_t],       %[lo2_t],   %[lo1_t]    \n\t"

        : [lo1_t] "=&r" (lo1_t), [lo2_t] "=&r" (lo2_t)
        : [lo] "r" (lo), [hi] "r" (hi), [n] "r" (n)
        : "hi", "lo"
    );
    return (lo2_t);
}
