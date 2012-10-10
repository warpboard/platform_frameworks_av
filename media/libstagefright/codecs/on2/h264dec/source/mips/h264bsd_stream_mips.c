/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*------------------------------------------------------------------------------

    Table of contents

     1. Include headers
     2. External compiler flags
     3. Module defines
     4. Local function prototypes
     5. Functions
          h264bsdShowBits32

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "../h264bsd_util.h"
#include "../h264bsd_stream.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Function: h264bsdShowBits32

        Functional description:
            Read 32 bits from the stream buffer. Buffer is left as it is, i.e.
            no bits are removed. First bit read from the stream is the MSB of
            the return value. If there is not enough bits in the buffer ->
            bits beyong the end of the stream are set to '0' in the return
            value.

        Input:
            pStrmData   pointer to stream data structure

        Output:
            none

        Returns:
            bits read from stream

------------------------------------------------------------------------------*/

u32 h264bsdShowBits32(strmData_t *pStrmData)
{
    u32 temp0, temp1, temp2, temp3, *pStrm, bits;

    __asm__ volatile (
        ".set push                                          \n\t"
        ".set noreorder                                     \n\t"
        "lw         %[temp1],    0(%[pStrmData])            \n\t"
        "lw         %[temp2],    16(%[pStrmData])           \n\t"
        "sll        %[temp1],    %[temp1],       3          \n\t"
        "subu       %[bits],     %[temp1],       %[temp2]   \n\t"
        "lw         %[pStrm],    4(%[pStrmData])            \n\t"
        "slti       %[temp2],    %[bits],        32         \n\t"
        "beq        %[temp2],    $zero,          5f         \n\t"
        " lw        %[temp1],    8(%[pStrmData])            \n\t"
        "bne        %[bits],     $zero,          6f         \n\t"
        " addiu     %[temp2],    %[temp1],       24         \n\t"
        "b          7f                                      \n\t"
        " xor       %[temp0],    %[temp0],       %[temp0]   \n\t"
        "6:                                                 \n\t"
        "lbu        %[temp0],    0(%[pStrm])                \n\t"
        "addiu      %[bits],     %[bits],        -8         \n\t"
        "addu       %[bits],     %[bits],        %[temp1]   \n\t"
        "sllv       %[temp0],    %[temp0],       %[temp2]   \n\t"
        "addiu      %[temp2],    %[temp2],       -8         \n\t"
        "addiu      %[temp3],    %[bits],        -24        \n\t"
        "beq        %[temp3],    $zero,          2f         \n\t"
        " addiu     %[temp1],    %[bits],        -16        \n\t"
        "beq        %[temp1],    $zero,          3f         \n\t"
        " addiu     %[temp3],    %[bits],        -8         \n\t"
        "beq        %[temp3],    $zero,          4f         \n\t"
        " lbu       %[temp1],    1(%[pStrm])                \n\t"
        "b          7f                                      \n\t"
        " nop                                               \n\t"
        "2:                                                 \n\t"
        "ulw        %[temp1],    1(%[pStrm])                \n\t"
        "addiu      %[temp2],    %[temp2],       -16        \n\t"
        "addiu      %[pStrm],    %[pStrm],       4          \n\t"
        "rotr       %[temp1],    %[temp1],       16         \n\t"
        "wsbh       %[temp1],    %[temp1]                   \n\t"
        "srl        %[temp1],    %[temp1],       8          \n\t"
        "sllv       %[temp1],    %[temp1],       %[temp2]   \n\t"
        "b          7f                                      \n\t"
        " or        %[temp0],    %[temp0],       %[temp1]   \n\t"
        "3:                                                 \n\t"
        "lhu        %[temp1],    1(%[pStrm])                \n\t"
        "addiu      %[temp2],    %[temp2],       -8         \n\t"
        "addiu      %[pStrm],    %[pStrm],       3          \n\t"
        "wsbh       %[temp1],    %[temp1]                   \n\t"
        "sllv       %[temp1],    %[temp1],       %[temp2]   \n\t"
        "b          7f                                      \n\t"
        " or        %[temp0],    %[temp0],       %[temp1]   \n\t"
        "4:                                                 \n\t"
        "addiu      %[pStrm],    %[pStrm],       2          \n\t"
        "sllv       %[temp1],    %[temp1],       %[temp2]   \n\t"
        "b          7f                                      \n\t"
        " or        %[temp0],    %[temp0],       %[temp1]   \n\t"
        "5:                                                 \n\t"
        "ulw        %[temp0],    0(%[pStrm])                \n\t"
        "rotr       %[temp0],    %[temp0],       16         \n\t"
        "beq        %[temp1],    $zero,          7f         \n\t"
        " wsbh      %[temp0],    %[temp0]                   \n\t"
        "1:                                                 \n\t"
        "lbu        %[temp3],    4(%[pStrm])                \n\t"
        "addiu      %[temp2],    $zero,          8          \n\t"
        "subu       %[temp2],    %[temp2],       %[temp1]   \n\t"
        "sllv       %[temp0],    %[temp0],       %[temp1]   \n\t"
        "srlv       %[temp3],    %[temp3],       %[temp2]   \n\t"
        "or         %[temp0],    %[temp0],       %[temp3]   \n\t"
        "7:                                                 \n\t"
        ".set pop                                           \n\t"

        : [temp0]"=&r"(temp0), [temp1]"=&r"(temp1),
          [temp2]"=&r"(temp2), [temp3]"=&r"(temp3),
          [pStrm]"=&r"(pStrm), [bits]"=&r"(bits)
        : [pStrmData]"r"(pStrmData)
        : "memory"
    );

    return temp0;
}


