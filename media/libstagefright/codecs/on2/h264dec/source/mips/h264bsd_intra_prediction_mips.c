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
          h264bsdAddResidual

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "../h264bsd_intra_prediction.h"
#include "../h264bsd_util.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/* x- and y-coordinates for each block */
extern const u32 h264bsdBlockX[16];

extern const u32 h264bsdBlockY[16];

extern const u8 h264bsdClip[1280];

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
void h264bsdAddResidual(u8 *data, i16 *residual, u32 blockNum);

/*------------------------------------------------------------------------------

    Function: h264bsdAddResidual

        Functional description:
          Add residual of a block into prediction in macroblock array 'data'.
          The result (residual + prediction) is stored in 'data'.

------------------------------------------------------------------------------*/
void h264bsdAddResidual(u8 *data, i16 *residual, u32 blockNum)
{

/* Variables */

    u32 width;
    const u8 *clp = h264bsdClip + 512;
    register int temp0, temp1, temp2, temp3;
    register int temp4, temp5, temp6, temp7, temp8;

    if (IS_RESIDUAL_EMPTY(residual))
        return;

    temp1 = blockNum;

    __asm__ volatile (
        ".set push                                              \n\t"
        ".set noreorder                                         \n\t"
        "slti   %[temp0],  %[temp1],                16          \n\t"
        "addiu  %[width],  $zero,                   16          \n\t"
        "bne    %[temp0],  $zero,                   1f          \n\t"
        " addiu %[temp3],  $zero,                   4           \n\t"
        "addiu  %[width],  $zero,                   8           \n\t"
        "addiu  %[temp3],  $zero,                   3           \n\t"
        "andi   %[temp1],  %[temp1],                3           \n\t"
        "1:                                                     \n\t"
        "sll    %[temp1],  %[temp1],                2           \n\t"
        "lwx    %[temp0],  %[temp1](%[h264bsdBlockX])           \n\t"
        "lwx    %[temp2],  %[temp1](%[h264bsdBlockY])           \n\t"
        "sllv   %[temp1],  %[temp2],                %[temp3]    \n\t"
        "addu   %[temp0],  %[data],                 %[temp0]    \n\t"
        "addu   %[temp0],  %[temp0],                %[temp1]    \n\t"
        "lh     %[temp1],  0(%[residual])                       \n\t"
        "lh     %[temp2],  2(%[residual])                       \n\t"
        "lh     %[temp3],  4(%[residual])                       \n\t"
        "lh     %[temp4],  6(%[residual])                       \n\t"
        "lbu    %[temp5],  0(%[temp0])                          \n\t"
        "lbu    %[temp6],  1(%[temp0])                          \n\t"
        "lbu    %[temp7],  2(%[temp0])                          \n\t"
        "lbu    %[temp8],  3(%[temp0])                          \n\t"
        "addu   %[temp1],  %[temp1],                %[temp5]    \n\t"
        "addu   %[temp2],  %[temp2],                %[temp6]    \n\t"
        "addu   %[temp3],  %[temp3],                %[temp7]    \n\t"
        "addu   %[temp4],  %[temp4],                %[temp8]    \n\t"
        "lbux   %[temp5],  %[temp1](%[clp])                     \n\t"
        "lbux   %[temp6],  %[temp2](%[clp])                     \n\t"
        "lbux   %[temp7],  %[temp3](%[clp])                     \n\t"
        "lbux   %[temp8],  %[temp4](%[clp])                     \n\t"
        "sb     %[temp5],  0(%[temp0])                          \n\t"
        "sb     %[temp6],  1(%[temp0])                          \n\t"
        "sb     %[temp7],  2(%[temp0])                          \n\t"
        "sb     %[temp8],  3(%[temp0])                          \n\t"
        "addu   %[temp0],  %[temp0],                %[width]    \n\t"
        "lh     %[temp1],  8(%[residual])                       \n\t"
        "lh     %[temp2],  10(%[residual])                      \n\t"
        "lh     %[temp3],  12(%[residual])                      \n\t"
        "lh     %[temp4],  14(%[residual])                      \n\t"
        "lbu    %[temp5],  0(%[temp0])                          \n\t"
        "lbu    %[temp6],  1(%[temp0])                          \n\t"
        "lbu    %[temp7],  2(%[temp0])                          \n\t"
        "lbu    %[temp8],  3(%[temp0])                          \n\t"
        "addu   %[temp1],  %[temp1],                %[temp5]    \n\t"
        "addu   %[temp2],  %[temp2],                %[temp6]    \n\t"
        "addu   %[temp3],  %[temp3],                %[temp7]    \n\t"
        "addu   %[temp4],  %[temp4],                %[temp8]    \n\t"
        "lbux   %[temp5],  %[temp1](%[clp])                     \n\t"
        "lbux   %[temp6],  %[temp2](%[clp])                     \n\t"
        "lbux   %[temp7],  %[temp3](%[clp])                     \n\t"
        "lbux   %[temp8],  %[temp4](%[clp])                     \n\t"
        "sb     %[temp5],  0(%[temp0])                          \n\t"
        "sb     %[temp6],  1(%[temp0])                          \n\t"
        "sb     %[temp7],  2(%[temp0])                          \n\t"
        "sb     %[temp8],  3(%[temp0])                          \n\t"
        "addu   %[temp0],  %[temp0],                %[width]    \n\t"
        "lh     %[temp1],  16(%[residual])                      \n\t"
        "lh     %[temp2],  18(%[residual])                      \n\t"
        "lh     %[temp3],  20(%[residual])                      \n\t"
        "lh     %[temp4],  22(%[residual])                      \n\t"
        "lbu    %[temp5],  0(%[temp0])                          \n\t"
        "lbu    %[temp6],  1(%[temp0])                          \n\t"
        "lbu    %[temp7],  2(%[temp0])                          \n\t"
        "lbu    %[temp8],  3(%[temp0])                          \n\t"
        "addu   %[temp1],  %[temp1],                %[temp5]    \n\t"
        "addu   %[temp2],  %[temp2],                %[temp6]    \n\t"
        "addu   %[temp3],  %[temp3],                %[temp7]    \n\t"
        "addu   %[temp4],  %[temp4],                %[temp8]    \n\t"
        "lbux   %[temp5],  %[temp1](%[clp])                     \n\t"
        "lbux   %[temp6],  %[temp2](%[clp])                     \n\t"
        "lbux   %[temp7],  %[temp3](%[clp])                     \n\t"
        "lbux   %[temp8],  %[temp4](%[clp])                     \n\t"
        "sb     %[temp5],  0(%[temp0])                          \n\t"
        "sb     %[temp6],  1(%[temp0])                          \n\t"
        "sb     %[temp7],  2(%[temp0])                          \n\t"
        "sb     %[temp8],  3(%[temp0])                          \n\t"
        "addu   %[temp0],  %[temp0],                %[width]    \n\t"
        "lh     %[temp1],  24(%[residual])                      \n\t"
        "lh     %[temp2],  26(%[residual])                      \n\t"
        "lh     %[temp3],  28(%[residual])                      \n\t"
        "lh     %[temp4],  30(%[residual])                      \n\t"
        "lbu    %[temp5],  0(%[temp0])                          \n\t"
        "lbu    %[temp6],  1(%[temp0])                          \n\t"
        "lbu    %[temp7],  2(%[temp0])                          \n\t"
        "lbu    %[temp8],  3(%[temp0])                          \n\t"
        "addu   %[temp1],  %[temp1],                %[temp5]    \n\t"
        "addu   %[temp2],  %[temp2],                %[temp6]    \n\t"
        "addu   %[temp3],  %[temp3],                %[temp7]    \n\t"
        "addu   %[temp4],  %[temp4],                %[temp8]    \n\t"
        "lbux   %[temp5],  %[temp1](%[clp])                     \n\t"
        "lbux   %[temp6],  %[temp2](%[clp])                     \n\t"
        "lbux   %[temp7],  %[temp3](%[clp])                     \n\t"
        "lbux   %[temp8],  %[temp4](%[clp])                     \n\t"
        "sb     %[temp5],  0(%[temp0])                          \n\t"
        "sb     %[temp6],  1(%[temp0])                          \n\t"
        "sb     %[temp7],  2(%[temp0])                          \n\t"
        "sb     %[temp8],  3(%[temp0])                          \n\t"
        "addu   %[temp0],  %[temp0],                %[width]    \n\t"
        ".set pop                                               \n\t"

        : [temp0] "=&r" (temp0), [temp1] "+r" (temp1), [temp2] "=&r" (temp2),
          [width] "=&r" (width), [residual] "+r" (residual),
          [temp4] "=&r" (temp4), [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
          [temp7] "=&r" (temp7), [temp8] "=&r" (temp8), [temp3] "=&r" (temp3)
        : [h264bsdBlockX] "r" (h264bsdBlockX), [data] "r" (data),
          [h264bsdBlockY] "r" (h264bsdBlockY), [clp] "r" (clp)
        : "memory"
    );
}

/*lint +e702 */


