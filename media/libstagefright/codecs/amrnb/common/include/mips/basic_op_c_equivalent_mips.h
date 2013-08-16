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

 Pathname: ./include/basic_op_c_equivalent.h

------------------------------------------------------------------------------
 REVISION HISTORY

 Who:                       Date:
 Description:

------------------------------------------------------------------------------
 INCLUDE DESCRIPTION

 This file includes all the C-Equivalent basicop.c functions.

------------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------
; CONTINUE ONLY IF NOT ALREADY DEFINED
----------------------------------------------------------------------------*/
#ifndef BASIC_OP_C_EQUIVALENT_H_MIPS
#define BASIC_OP_C_EQUIVALENT_H_MIPS

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

    /*----------------------------------------------------------------------------
    ; MACROS
    ; Define module specific macros here
    ----------------------------------------------------------------------------*/

    /*----------------------------------------------------------------------------
    ; DEFINES
    ; Include all pre-processor statements here.
    ----------------------------------------------------------------------------*/

    /*----------------------------------------------------------------------------
    ; EXTERNAL VARIABLES REFERENCES
    ; Declare variables used in this module but defined elsewhere
    ----------------------------------------------------------------------------*/

    /*----------------------------------------------------------------------------
    ; SIMPLE TYPEDEF'S
    ----------------------------------------------------------------------------*/

    /*----------------------------------------------------------------------------
    ; ENUMERATED TYPEDEF'S
    ----------------------------------------------------------------------------*/

    /*----------------------------------------------------------------------------
    ; STRUCTURES TYPEDEF'S
    ----------------------------------------------------------------------------*/

    /*----------------------------------------------------------------------------
    ; GLOBAL FUNCTION DEFINITIONS
    ; Function Prototype declaration
    ----------------------------------------------------------------------------*/


    /*
    ------------------------------------------------------------------------------
     FUNCTION NAME: L_add
    ------------------------------------------------------------------------------
     INPUT AND OUTPUT DEFINITIONS

     Inputs:
        L_var1 = 32 bit long signed integer (Word32) whose value falls
                 in the range : 0x8000 0000 <= L_var1 <= 0x7fff ffff.

        L_var2 = 32 bit long signed integer (Word32) whose value falls
                 in the range : 0x8000 0000 <= L_var1 <= 0x7fff ffff.

        pOverflow = pointer to overflow (Flag)

     Outputs:
        pOverflow -> 1 if the 32 bit add operation resulted in overflow

     Returns:
        L_sum = 32-bit sum of L_var1 and L_var2 (Word32)
    */
#if (MIPS_DSP_R2_LE)

    static inline Word32 L_add(register Word32 L_var1, register Word32 L_var2, Flag *pOverflow)
    {
        Word32 L_sum;

        OSCL_UNUSED_ARG(pOverflow);

        __asm__ volatile (
            "addq_s.w %[L_sum], %[L_var1], %[L_var2] \n\t"
            : [L_sum] "=r" (L_sum)
            : [L_var1] "r" (L_var1), [L_var2] "r" (L_var2)
        );
        return (L_sum);
    }

    /*
    ------------------------------------------------------------------------------
     FUNCTION NAME: L_sub
    ------------------------------------------------------------------------------
     INPUT AND OUTPUT DEFINITIONS

     Inputs:
        L_var1 = 32 bit long signed integer (Word32) whose value falls
                 in the range : 0x8000 0000 <= L_var1 <= 0x7fff ffff.

        L_var2 = 32 bit long signed integer (Word32) whose value falls
                 in the range : 0x8000 0000 <= L_var1 <= 0x7fff ffff.

        pOverflow = pointer to overflow (Flag)

     Outputs:
        pOverflow -> 1 if the 32 bit add operation resulted in overflow

     Returns:
        L_diff = 32-bit difference of L_var1 and L_var2 (Word32)
    */
    static inline Word32 L_sub(register Word32 L_var1, register Word32 L_var2,
                               register Flag *pOverflow)
    {
        Word32 L_diff;

        OSCL_UNUSED_ARG(pOverflow);

        __asm__ volatile (
            "subq_s.w %[L_diff], %[L_var1], %[L_var2] \n\t"
            : [L_diff] "=r" (L_diff)
            : [L_var1] "r" (L_var1), [L_var2] "r" (L_var2)
        );
        return (L_diff);
    }


    /*
    ------------------------------------------------------------------------------
     FUNCTION NAME: L_mac
    ------------------------------------------------------------------------------
     INPUT AND OUTPUT DEFINITIONS

     Inputs:
        L_var3 = 32 bit long signed integer (Word32) whose value falls
                 in the range : 0x8000 0000 <= L_var3 <= 0x7fff ffff.
        var1 = 16 bit short signed integer (Word16) whose value falls in
               the range : 0xffff 8000 <= var1 <= 0x0000 7fff.
        var2 = 16 bit short signed integer (Word16) whose value falls in
               the range : 0xffff 8000 <= var2 <= 0x0000 7fff.

        pOverflow = pointer to overflow (Flag)

     Outputs:
        pOverflow -> 1 if the 32 bit add operation resulted in overflow

     Returns:
        result = 32-bit result of L_var3 + (var1 * var2)(Word32)
    */
    __inline Word32 L_mac(Word32 L_var3, Word16 var1, Word16 var2, Flag *pOverflow)
    {
        Word32 result;

        OSCL_UNUSED_ARG(pOverflow);

        __asm__ volatile (
            "muleq_s.w.phr  %[result], %[var1],    %[var2]   \n\t"
            "addq_s.w       %[result], %[L_var3],  %[result]    \n\t"
            : [result] "=&r" (result)
            : [var1] "r" (var1), [var2] "r" (var2), [L_var3] "r" (L_var3)
            : "hi", "lo"
        );
        return (result);
    }

    /*
    ------------------------------------------------------------------------------
     FUNCTION NAME: L_mult
    ------------------------------------------------------------------------------
     INPUT AND OUTPUT DEFINITIONS

     Inputs:
        L_var1 = 16 bit short signed integer (Word16) whose value falls in
               the range : 0xffff 8000 <= var1 <= 0x0000 7fff.

        L_var2 = 16 bit short signed integer (Word16) whose value falls in
               the range : 0xffff 8000 <= var1 <= 0x0000 7fff.

        pOverflow = pointer to overflow (Flag)

     Outputs:
        pOverflow -> 1 if the 32 bit add operation resulted in overflow

     Returns:
        L_product = 32-bit product of L_var1 and L_var2 (Word32)
    */
    static inline Word32 L_mult(Word16 var1, Word16 var2, Flag *pOverflow)
    {
        register Word32 L_product;

        OSCL_UNUSED_ARG(pOverflow);

        __asm__ volatile (
            "muleq_s.w.phr %[L_product], %[var1], %[var2]   \n\t"
            : [L_product] "=r" (L_product)
            : [var1] "r" (var1), [var2] "r" (var2)
            : "hi", "lo"
        );

        return (L_product);
    }


    /*
    ------------------------------------------------------------------------------
     FUNCTION NAME: L_msu
    ------------------------------------------------------------------------------
     INPUT AND OUTPUT DEFINITIONS

     Inputs:
        L_var3 = 32 bit long signed integer (Word32) whose value falls
                 in the range : 0x8000 0000 <= L_var3 <= 0x7fff ffff.

        var1 = 16 bit short signed integer (Word16) whose value falls in
               the range : 0xffff 8000 <= var1 <= 0x0000 7fff.
        var2 = 16 bit short signed integer (Word16) whose value falls in
               the range : 0xffff 8000 <= var2 <= 0x0000 7fff.

        pOverflow = pointer to overflow (Flag)

     Outputs:
        pOverflow -> 1 if the 32 bit operation resulted in overflow

     Returns:
        result = 32-bit result of L_var3 - (var1 * var2)
    */

    static inline Word32 L_msu(Word32 L_var3, Word16 var1, Word16 var2, Flag *pOverflow)
    {
        Word32 result;

        OSCL_UNUSED_ARG(pOverflow);

        __asm__ volatile (
            "muleq_s.w.phr %[result], %[var1],   %[var2]      \n\t"
            "subq_s.w      %[result], %[L_var3], %[result]    \n\t"
            : [result] "=&r" (result)
            : [var1] "r" (var1), [var2] "r" (var2), [L_var3] "r" (L_var3)
            : "hi", "lo"
        );
        return (result);
    }

    /*
    ------------------------------------------------------------------------------
     FUNCTION NAME: Mpy_32
    ------------------------------------------------------------------------------
     INPUT AND OUTPUT DEFINITIONS

     Inputs:
        L_var1_hi = most significant word of first input (Word16).
        L_var1_lo = least significant word of first input (Word16).
        L_var2_hi = most significant word of second input (Word16).
        L_var2_lo = least significant word of second input (Word16).

        pOverflow = pointer to overflow (Flag)

     Outputs:
        pOverflow -> 1 if the 32 bit multiply operation resulted in overflow

     Returns:
        L_product = 32-bit product of L_var1 and L_var2 (Word32)
    */
    __inline Word32 Mpy_32(Word16 L_var1_hi,
                           Word16 L_var1_lo,
                           Word16 L_var2_hi,
                           Word16 L_var2_lo,
                           Flag   *pOverflow)
    {
        Word32 L_sum;
        Word32 temp0, temp1, temp2, temp3;

        OSCL_UNUSED_ARG(pOverflow);

        __asm__ volatile (
            "sll        %[temp0],  %[L_var1_hi],   16             \n\t"
            "sll        %[temp1],  %[L_var2_hi],   16             \n\t"
            "sll        %[temp2],  %[L_var1_lo],   1              \n\t"
            "sll        %[temp3],  %[L_var2_lo],   1              \n\t"
            "mult       $ac0,      %[temp0],       %[temp1]       \n\t"
            "madd       $ac0,      %[temp0],       %[temp3]       \n\t"
            "mtlo       $0,        $ac0                           \n\t"
            "madd       $ac0,      %[temp1],       %[temp2]       \n\t"
            "mfhi       %[L_sum],  $ac0                           \n\t"
            "shll_s.w   %[L_sum],  %[L_sum],       1              \n\t"
            : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1),
              [temp2] "=&r" (temp2), [temp3] "=&r" (temp3),
              [L_sum] "=&r" (L_sum)
            : [L_var2_lo] "r" (L_var2_lo), [L_var1_lo] "r" (L_var1_lo),
              [L_var1_hi] "r" (L_var1_hi), [L_var2_hi] "r" (L_var2_hi)
            : "hi", "lo"
        );
        return (L_sum);
    }

    /*
    ------------------------------------------------------------------------------
     FUNCTION NAME: Mpy_32_16
    ------------------------------------------------------------------------------
     INPUT AND OUTPUT DEFINITIONS

     Inputs:
        L_var1_hi = most significant 16 bits of 32-bit input (Word16).
        L_var1_lo = least significant 16 bits of 32-bit input (Word16).
        var2  = 16-bit signed integer (Word16).

        pOverflow = pointer to overflow (Flag)

     Outputs:
        pOverflow -> 1 if the 32 bit product operation resulted in overflow

     Returns:
        product = 32-bit product of the 32-bit L_var1 and 16-bit var1 (Word32)
    */

    __inline Word32 Mpy_32_16(Word16 L_var1_hi,
                              Word16 L_var1_lo,
                              Word16 var2,
                              Flag *pOverflow)
    {

        Word32 L_sum;
        Word32 temp0, temp1, temp2;

        OSCL_UNUSED_ARG(pOverflow);

        __asm__ volatile (
            "addiu      %[temp2],  $zero,          -2             \n\t"
            "sll        %[temp0],  %[var2],        16             \n\t"
            "sll        %[temp1],  %[L_var1_lo],   1              \n\t"
            "ins        %[temp1],  %[L_var1_hi],   16, 16         \n\t"
            "mulq_s.w   %[L_sum],  %[temp1],       %[temp0]       \n\t"
            "and        %[L_sum],  %[L_sum],       %[temp2]       \n\t"
            : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1),
              [L_sum] "=&r" (L_sum), [temp2] "=&r" (temp2)
            : [var2] "r" (var2), [L_var1_lo] "r" (L_var1_lo),
              [L_var1_hi] "r" (L_var1_hi)
            : "hi", "lo"
        );
        return (L_sum);
    }

    /*
    ------------------------------------------------------------------------------
     FUNCTION NAME: mult
    ------------------------------------------------------------------------------
     INPUT AND OUTPUT DEFINITIONS

     Inputs:
        var1 = 16 bit short signed integer (Word16) whose value falls in
               the range : 0xffff 8000 <= var1 <= 0x0000 7fff.

        var2 = 16 bit short signed integer (Word16) whose value falls in
               the range : 0xffff 8000 <= var2 <= 0x0000 7fff.

        pOverflow = pointer to overflow (Flag)

     Outputs:
        pOverflow -> 1 if the add operation resulted in overflow

     Returns:
        product = 16-bit limited product of var1 and var2 (Word16)
    */
    static inline Word16 mult(Word16 var1, Word16 var2, Flag *pOverflow)
    {
        register Word32 product;

        OSCL_UNUSED_ARG(pOverflow);

        __asm__ volatile (
            "muleq_s.w.phr  %[product], %[var1],    %[var2]    \n\t"
            "sra            %[product], %[product], 16         \n\t"
            : [product] "=r" (product)
            : [var1] "r" (var1), [var2] "r" (var2)
            : "hi", "lo"
        );

        /* Return the product as a 16 bit value by type casting Word32 to Word16 */

        return ((Word16) product);
    }


    /*----------------------------------------------------------------------------
    ; END
    ----------------------------------------------------------------------------*/
#endif /* #ifdef FUNCTIONS_basic */

/*----------------------------------------------------------------------------
    ; END
    ----------------------------------------------------------------------------*/

#endif /* BASIC_OP_C_EQUIVALENT_H_MIPS */

