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
          h264bsdWriteMacroblock
          h264bsdWriteOutputBlocks

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "../h264bsd_image.h"
#include "../h264bsd_util.h"
#include "../h264bsd_neighbour.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/* x- and y-coordinates for each block, defined in h264bsd_intra_prediction.c */
extern const u32 h264bsdBlockX[];
extern const u32 h264bsdBlockY[];

/* clipping table, defined in h264bsd_intra_prediction.c */
extern const u8 h264bsdClip[];

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/



/*------------------------------------------------------------------------------

    Function: h264bsdWriteMacroblock

        Functional description:
            Write one macroblock into the image. Both luma and chroma
            components will be written at the same time.

        Inputs:
            data    pointer to macroblock data to be written, 256 values for
                    luma followed by 64 values for both chroma components

        Outputs:
            image   pointer to the image where the macroblock will be written

        Returns:
            none

------------------------------------------------------------------------------*/
void h264bsdWriteMacroblock(image_t *image, u8 *data)
{

/* Variables */

    u32 i;
    u32 width;
    u32 *lum, *cb, *cr;
    u32 *ptr;
    u32 tmp1, tmp2, tmp3, tmp4, width_4;

/* Code */

    ASSERT(image);
    ASSERT(data);
    ASSERT(!((u32)data&0x3));

    width = image->width;

    /*lint -save -e826 lum, cb and cr used to copy 4 bytes at the time, disable
     * "area too small" info message */
    lum = (u32*)image->luma;
    cb = (u32*)image->cb;
    cr = (u32*)image->cr;
    ASSERT(!((u32)lum&0x3));
    ASSERT(!((u32)cb&0x3));
    ASSERT(!((u32)cr&0x3));

    ptr = (u32*)data;
    width_4 = width*16;

    __asm__ volatile (
        "lw       %[tmp1],      0(%[ptr])                 \n\t"
        "lw       %[tmp2],      4(%[ptr])                 \n\t"
        "lw       %[tmp3],      8(%[ptr])                 \n\t"
        "lw       %[tmp4],      12(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],    %[width_4]     \n\t"
        "lw       %[tmp1],      16(%[ptr])                \n\t"
        "lw       %[tmp2],      20(%[ptr])                \n\t"
        "lw       %[tmp3],      24(%[ptr])                \n\t"
        "lw       %[tmp4],      28(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu      %[lum],      %[lum],    %[width_4]     \n\t"
        "lw       %[tmp1],      32(%[ptr])                \n\t"
        "lw       %[tmp2],      36(%[ptr])                \n\t"
        "lw       %[tmp3],      40(%[ptr])                \n\t"
        "lw       %[tmp4],      44(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu      %[lum],      %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      48(%[ptr])                \n\t"
        "lw       %[tmp2],      52(%[ptr])                \n\t"
        "lw       %[tmp3],      56(%[ptr])                \n\t"
        "lw       %[tmp4],      60(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu      %[lum],      %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      64(%[ptr])                \n\t"
        "lw       %[tmp2],      68(%[ptr])                \n\t"
        "lw       %[tmp3],      72(%[ptr])                \n\t"
        "lw       %[tmp4],      76(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      80(%[ptr])                \n\t"
        "lw       %[tmp2],      84(%[ptr])                \n\t"
        "lw       %[tmp3],      88(%[ptr])                \n\t"
        "lw       %[tmp4],      92(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      96(%[ptr])                \n\t"
        "lw       %[tmp2],      100(%[ptr])               \n\t"
        "lw       %[tmp3],      104(%[ptr])               \n\t"
        "lw       %[tmp4],      108(%[ptr])               \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      112(%[ptr])               \n\t"
        "lw       %[tmp2],      116(%[ptr])               \n\t"
        "lw       %[tmp3],      120(%[ptr])               \n\t"
        "lw       %[tmp4],      124(%[ptr])               \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      128(%[ptr])               \n\t"
        "lw       %[tmp2],      132(%[ptr])               \n\t"
        "lw       %[tmp3],      136(%[ptr])               \n\t"
        "lw       %[tmp4],      140(%[ptr])               \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      144(%[ptr])               \n\t"
        "lw       %[tmp2],      148(%[ptr])               \n\t"
        "lw       %[tmp3],      152(%[ptr])               \n\t"
        "lw       %[tmp4],      156(%[ptr])               \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      160(%[ptr])               \n\t"
        "lw       %[tmp2],      164(%[ptr])               \n\t"
        "lw       %[tmp3],      168(%[ptr])               \n\t"
        "lw       %[tmp4],      172(%[ptr])               \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      176(%[ptr])               \n\t"
        "lw       %[tmp2],      180(%[ptr])               \n\t"
        "lw       %[tmp3],      184(%[ptr])               \n\t"
        "lw       %[tmp4],      188(%[ptr])               \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      192(%[ptr])               \n\t"
        "lw       %[tmp2],      196(%[ptr])               \n\t"
        "lw       %[tmp3],      200(%[ptr])               \n\t"
        "lw       %[tmp4],      204(%[ptr])               \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      208(%[ptr])               \n\t"
        "lw       %[tmp2],      212(%[ptr])               \n\t"
        "lw       %[tmp3],      216(%[ptr])               \n\t"
        "lw       %[tmp4],      220(%[ptr])               \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      224(%[ptr])               \n\t"
        "lw       %[tmp2],      228(%[ptr])               \n\t"
        "lw       %[tmp3],      232(%[ptr])               \n\t"
        "lw       %[tmp4],      236(%[ptr])               \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],     %[width_4]    \n\t"
        "lw       %[tmp1],      240(%[ptr])               \n\t"
        "lw       %[tmp2],      244(%[ptr])               \n\t"
        "lw       %[tmp3],      248(%[ptr])               \n\t"
        "lw       %[tmp4],      252(%[ptr])               \n\t"
        "sw       %[tmp1],      0(%[lum])                 \n\t"
        "sw       %[tmp2],      4(%[lum])                 \n\t"
        "sw       %[tmp3],      8(%[lum])                 \n\t"
        "sw       %[tmp4],      12(%[lum])                \n\t"
        "addu     %[lum],       %[lum],      %[width_4]   \n\t"
        "addiu    %[ptr],       %[ptr],      256          \n\t"
        "srl      %[width_4],   %[width_4],  1            \n\t"
        /* second loop */
        "lw       %[tmp1],      0(%[ptr])                 \n\t"
        "lw       %[tmp2],      4(%[ptr])                 \n\t"
        "lw       %[tmp3],      8(%[ptr])                 \n\t"
        "lw       %[tmp4],      12(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[cb])                  \n\t"
        "sw       %[tmp2],      4(%[cb])                  \n\t"
        "addu     %[cb],        %[cb],     %[width_4]     \n\t"
        "sw       %[tmp3],      0(%[cb])                  \n\t"
        "sw       %[tmp4],      4(%[cb])                  \n\t"
        "addu     %[cb],        %[cb],     %[width_4]     \n\t"
        "lw       %[tmp1],      16(%[ptr])                \n\t"
        "lw       %[tmp2],      20(%[ptr])                \n\t"
        "lw       %[tmp3],      24(%[ptr])                \n\t"
        "lw       %[tmp4],      28(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[cb])                  \n\t"
        "sw       %[tmp2],      4(%[cb])                  \n\t"
        "addu     %[cb],        %[cb],     %[width_4]     \n\t"
        "sw       %[tmp3],      0(%[cb])                  \n\t"
        "sw       %[tmp4],      4(%[cb])                  \n\t"
        "addu     %[cb],        %[cb],     %[width_4]     \n\t"
        "lw       %[tmp1],      32(%[ptr])                \n\t"
        "lw       %[tmp2],      36(%[ptr])                \n\t"
        "lw       %[tmp3],      40(%[ptr])                \n\t"
        "lw       %[tmp4],      44(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[cb])                  \n\t"
        "sw       %[tmp2],      4(%[cb])                  \n\t"
        "addu     %[cb],        %[cb],     %[width_4]     \n\t"
        "sw       %[tmp3],      0(%[cb])                  \n\t"
        "sw       %[tmp4],      4(%[cb])                  \n\t"
        "addu     %[cb],        %[cb],     %[width_4]     \n\t"
        "lw       %[tmp1],      48(%[ptr])                \n\t"
        "lw       %[tmp2],      52(%[ptr])                \n\t"
        "lw       %[tmp3],      56(%[ptr])                \n\t"
        "lw       %[tmp4],      60(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[cb])                  \n\t"
        "sw       %[tmp2],      4(%[cb])                  \n\t"
        "addu     %[cb],        %[cb],     %[width_4]     \n\t"
        "sw       %[tmp3],      0(%[cb])                  \n\t"
        "sw       %[tmp4],      4(%[cb])                  \n\t"
        "addu     %[cb],        %[cb],     %[width_4]     \n\t"
        "addiu    %[ptr],       %[ptr],    64             \n\t"
        /* third loop */
        "lw       %[tmp1],      0(%[ptr])                 \n\t"
        "lw       %[tmp2],      4(%[ptr])                 \n\t"
        "lw       %[tmp3],      8(%[ptr])                 \n\t"
        "lw       %[tmp4],      12(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[cr])                  \n\t"
        "sw       %[tmp2],      4(%[cr])                  \n\t"
        "addu     %[cr],        %[cr],     %[width_4]     \n\t"
        "sw       %[tmp3],      0(%[cr])                  \n\t"
        "sw       %[tmp4],      4(%[cr])                  \n\t"
        "addu     %[cr],        %[cr],     %[width_4]     \n\t"
        "lw       %[tmp1],      16(%[ptr])                \n\t"
        "lw       %[tmp2],      20(%[ptr])                \n\t"
        "lw       %[tmp3],      24(%[ptr])                \n\t"
        "lw       %[tmp4],      28(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[cr])                  \n\t"
        "sw       %[tmp2],      4(%[cr])                  \n\t"
        "addu     %[cr],        %[cr],     %[width_4]     \n\t"
        "sw       %[tmp3],      0(%[cr])                  \n\t"
        "sw       %[tmp4],      4(%[cr])                  \n\t"
        "addu     %[cr],        %[cr],     %[width_4]     \n\t"
        "lw       %[tmp1],      32(%[ptr])                \n\t"
        "lw       %[tmp2],      36(%[ptr])                \n\t"
        "lw       %[tmp3],      40(%[ptr])                \n\t"
        "lw       %[tmp4],      44(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[cr])                  \n\t"
        "sw       %[tmp2],      4(%[cr])                  \n\t"
        "addu     %[cr],        %[cr],     %[width_4]     \n\t"
        "sw       %[tmp3],      0(%[cr])                  \n\t"
        "sw       %[tmp4],      4(%[cr])                  \n\t"
        "addu     %[cr],        %[cr],     %[width_4]     \n\t"
        "lw       %[tmp1],      48(%[ptr])                \n\t"
        "lw       %[tmp2],      52(%[ptr])                \n\t"
        "lw       %[tmp3],      56(%[ptr])                \n\t"
        "lw       %[tmp4],      60(%[ptr])                \n\t"
        "sw       %[tmp1],      0(%[cr])                  \n\t"
        "sw       %[tmp2],      4(%[cr])                  \n\t"
        "addu     %[cr],        %[cr],     %[width_4]     \n\t"
        "sw       %[tmp3],      0(%[cr])                  \n\t"
        "sw       %[tmp4],      4(%[cr])                  \n\t"
        "addu     %[cr],        %[cr],     %[width_4]     \n\t"
        "addiu    %[ptr],       %[ptr],    64             \n\t"

        : [tmp1] "=&r" (tmp1), [ptr] "+r" (ptr),
          [tmp2] "=&r" (tmp2), [lum] "+r" (lum),
          [tmp3] "=&r" (tmp3), [tmp4] "=&r" (tmp4),
          [width_4] "+r" (width_4),[cr] "+r" (cr),
          [cb] "+r" (cb)
        :
        : "memory"
    );
}

/*------------------------------------------------------------------------------

    Function: h264bsdWriteOutputBlocks

        Functional description:
            Write one macroblock into the image. Prediction for the macroblock
            and the residual are given separately and will be combined while
            writing the data to the image

        Inputs:
            data        pointer to macroblock prediction data, 256 values for
                        luma followed by 64 values for both chroma components
            mbNum       number of the macroblock
            residual    pointer to residual data, 16 16-element arrays for luma
                        followed by 4 16-element arrays for both chroma
                        components

        Outputs:
            image       pointer to the image where the data will be written

        Returns:
            none

------------------------------------------------------------------------------*/
void h264bsdWriteOutputBlocks(image_t *image, u32 mbNum, u8 *data,
        i16 residual[][16])
{

/* Variables */

    u32 picWidth, picSize;
    u8 *lum, *cb, *cr;
    u8 *imageBlock;
    u8 *tmp;
    u32 row, col;
    u32 block;
    u32 x, y;
    i16 *pRes;
    i32 tmp1, tmp2, tmp3, tmp4, tmp11, tmp2a, tmp4a;

/* Code */

    ASSERT(image);
    ASSERT(data);
    ASSERT(mbNum < image->width * image->height);
    ASSERT(!((u32)data&0x3));

    /* Image size in macroblocks */
    picWidth = image->width;
    picSize = picWidth * image->height;
    row = mbNum / picWidth;
    col = mbNum % picWidth;

    /* Output macroblock position in output picture */
    lum = (image->data + row * picWidth * 256 + col * 16);
    cb = (image->data + picSize * 256 + row * picWidth * 64 + col * 8);
    cr = (cb + picSize * 64);

    picWidth *= 16;

    for (block = 0; block < 16; block++)
    {
        x = h264bsdBlockX[block];
        y = h264bsdBlockY[block];

        pRes = residual[block];

        ASSERT(pRes);

        tmp = data + y*16 + x;
        imageBlock = lum + y*picWidth + x;

        ASSERT(!((u32)tmp&0x3));
        ASSERT(!((u32)imageBlock&0x3));

        if (IS_RESIDUAL_EMPTY(pRes))
        {
            /*lint -e826 */
            i32 *in32 = (i32*)tmp;
            i32 *out32 = (i32*)imageBlock;
            i32 picWidth4 = picWidth >> 2;

            /* Residual is zero => copy prediction block to output */
            tmp1 = in32[0];
            tmp2 = in32[4];
            *out32 = tmp1;
            out32 += picWidth4;
            tmp3 = in32[8];
            tmp4 = in32[12];
            *out32 = tmp2;
            out32 += picWidth4;
            *out32 = tmp3;
            out32 += picWidth4;
            *out32 = tmp4;
        }
        else
        {

            RANGE_CHECK_ARRAY(pRes, -512, 511, 16);

            /* Calculate image = prediction + residual
             * Process four pixels in a loop */
            __asm__ volatile (
                "lw              %[tmp1],      0(%[tmp])                    \n\t"
                "lw              %[tmp2],      0(%[pRes])                   \n\t"
                "lw              %[tmp4],      4(%[pRes])                   \n\t"
                "preceu.ph.qbr   %[tmp11],     %[tmp1]                      \n\t"
                "preceu.ph.qbl   %[tmp12],     %[tmp1]                      \n\t"
                "lw              %[tmp2a],     8(%[pRes])                   \n\t"
                "addu.ph         %[tmp2],      %[tmp2],        %[tmp11]     \n\t"
                "addu.ph         %[tmp4],      %[tmp4],        %[tmp12]     \n\t"
                "lw              %[tmp1],      16(%[tmp])                   \n\t"
                "lw              %[tmp4a],     12(%[pRes])                  \n\t"
                "shll_s.ph       %[tmp2],      %[tmp2],        7            \n\t"
                "shll_s.ph       %[tmp4],      %[tmp4],        7            \n\t"
                "preceu.ph.qbr   %[tmp11],     %[tmp1]                      \n\t"
                "preceu.ph.qbl   %[tmp12],     %[tmp1]                      \n\t"
                "precrqu_s.qb.ph %[tmp2],      %[tmp4],        %[tmp2]      \n\t"
                "lw              %[tmp4],      20(%[pRes])                  \n\t"
                "addu.ph         %[tmp2a],     %[tmp2a],       %[tmp11]     \n\t"
                "addu.ph         %[tmp4a],     %[tmp4a],       %[tmp12]     \n\t"
                "sw              %[tmp2],      0(%[imageBlock])             \n\t"
                "lw              %[tmp1],      32(%[tmp])                   \n\t"
                "lw              %[tmp2],      16(%[pRes])                  \n\t"
                "shll_s.ph       %[tmp2a],     %[tmp2a],       7            \n\t"
                "shll_s.ph       %[tmp4a],     %[tmp4a],       7            \n\t"
                "addu            %[imageBlock],%[imageBlock],  %[picWidth]  \n\t"
                "preceu.ph.qbr   %[tmp11],     %[tmp1]                      \n\t"
                "preceu.ph.qbl   %[tmp12],     %[tmp1]                      \n\t"
                "precrqu_s.qb.ph %[tmp2a],     %[tmp4a],       %[tmp2a]     \n\t"
                "lw              %[tmp1],      48(%[tmp])                   \n\t"
                "addu.ph         %[tmp2],      %[tmp2],        %[tmp11]     \n\t"
                "addu.ph         %[tmp4],      %[tmp4],        %[tmp12]     \n\t"
                "sw              %[tmp2a],     0(%[imageBlock])             \n\t"
                "lw              %[tmp2a],     24(%[pRes])                  \n\t"
                "shll_s.ph       %[tmp2],      %[tmp2],        7            \n\t"
                "shll_s.ph       %[tmp4],      %[tmp4],        7            \n\t"
                "lw              %[tmp4a],     28(%[pRes])                  \n\t"
                "preceu.ph.qbr   %[tmp11],     %[tmp1]                      \n\t"
                "preceu.ph.qbl   %[tmp12],     %[tmp1]                      \n\t"
                "precrqu_s.qb.ph %[tmp2],      %[tmp4],        %[tmp2]      \n\t"
                "addu.ph         %[tmp2a],     %[tmp2a],       %[tmp11]     \n\t"
                "addu.ph         %[tmp4a],     %[tmp4a],       %[tmp12]     \n\t"
                "addu            %[imageBlock],%[imageBlock],  %[picWidth]  \n\t"
                "shll_s.ph       %[tmp2a],     %[tmp2a],       7            \n\t"
                "shll_s.ph       %[tmp4a],     %[tmp4a],       7            \n\t"
                "sw              %[tmp2],      0(%[imageBlock])             \n\t"
                "addu            %[imageBlock],%[imageBlock],  %[picWidth]  \n\t"
                "precrqu_s.qb.ph %[tmp2a],     %[tmp4a],       %[tmp2a]     \n\t"
                "sw              %[tmp2a],     0(%[imageBlock])             \n\t"

                : [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2), [tmp4] "=&r" (tmp4),
                  [tmp2a] "=&r" (tmp2a), [tmp4a] "=&r" (tmp4a),
                  [tmp11] "=&r" (tmp11), [tmp12] "=&r" (tmp3),
                  [imageBlock] "+r" (imageBlock)
                : [picWidth] "r" (picWidth), [pRes] "r" (pRes), [tmp] "r" (tmp)
                : "memory"
            );
        }
    }

    picWidth = picWidth >> 1;
    for (block = 16; block <= 23; block++)
    {
        pRes = residual[block];

        x = h264bsdBlockX[block & 0x3];
        y = h264bsdBlockY[block & 0x3];

        pRes = residual[block];

        ASSERT(pRes);

        tmp = data + 256;
        imageBlock = cb;

        if (block >= 20)
        {
            imageBlock = cr;
            tmp += 64;
        }

        tmp += y*8 + x;
        imageBlock += y*picWidth + x;

        ASSERT(!((u32)tmp&0x3));
        ASSERT(!((u32)imageBlock&0x3));

        if (IS_RESIDUAL_EMPTY(pRes))
        {
            /*lint -e826 */
            i32 *in32 = (i32*)tmp;
            i32 *out32 = (i32*)imageBlock;
            i32 picWidth4 = picWidth >> 2;

            /* Residual is zero => copy prediction block to output */
            tmp1 = in32[0];
            tmp2 = in32[2];
            *out32 = tmp1;
            out32 += picWidth4;
            tmp3 = in32[4];
            tmp4 = in32[6];
            *out32 = tmp2;
            out32 += picWidth4;
            *out32 = tmp3;
            out32 += picWidth4;
            *out32 = tmp4;
        }
        else
        {

            RANGE_CHECK_ARRAY(pRes, -512, 511, 16);

            __asm__ volatile (
                "lw              %[tmp1],       0(%[tmp])                    \n\t"
                "lw              %[tmp2],       0(%[pRes])                   \n\t"
                "lw              %[tmp4],       4(%[pRes])                   \n\t"
                "preceu.ph.qbr   %[tmp11],      %[tmp1]                      \n\t"
                "preceu.ph.qbl   %[tmp12],      %[tmp1]                      \n\t"
                "lw              %[tmp2a],      8(%[pRes])                   \n\t"
                "addu.ph         %[tmp2],       %[tmp2],        %[tmp11]     \n\t"
                "addu.ph         %[tmp4],       %[tmp4],        %[tmp12]     \n\t"
                "lw              %[tmp1],       8(%[tmp])                    \n\t"
                "lw              %[tmp4a],      12(%[pRes])                  \n\t"
                "shll_s.ph       %[tmp2],       %[tmp2],        7            \n\t"
                "shll_s.ph       %[tmp4],       %[tmp4],        7            \n\t"
                "preceu.ph.qbr   %[tmp11],      %[tmp1]                      \n\t"
                "preceu.ph.qbl   %[tmp12],      %[tmp1]                      \n\t"
                "precrqu_s.qb.ph %[tmp2],       %[tmp4],        %[tmp2]      \n\t"
                "lw              %[tmp4],       20(%[pRes])                  \n\t"
                "addu.ph         %[tmp2a],      %[tmp2a],       %[tmp11]     \n\t"
                "addu.ph         %[tmp4a],      %[tmp4a],       %[tmp12]     \n\t"
                "sw              %[tmp2],       0(%[imageBlock])             \n\t"
                "lw              %[tmp1],       16(%[tmp])                   \n\t"
                "lw              %[tmp2],       16(%[pRes])                  \n\t"
                "shll_s.ph       %[tmp2a],      %[tmp2a],       7            \n\t"
                "shll_s.ph       %[tmp4a],      %[tmp4a],       7            \n\t"
                "addu            %[imageBlock], %[imageBlock],  %[picWidth]  \n\t"
                "preceu.ph.qbr   %[tmp11],      %[tmp1]                      \n\t"
                "preceu.ph.qbl   %[tmp12],      %[tmp1]                      \n\t"
                "precrqu_s.qb.ph %[tmp2a],      %[tmp4a],       %[tmp2a]     \n\t"
                "lw              %[tmp1],       24(%[tmp])                   \n\t"
                "addu.ph         %[tmp2],       %[tmp2],        %[tmp11]     \n\t"
                "addu.ph         %[tmp4],       %[tmp4],        %[tmp12]     \n\t"
                "sw              %[tmp2a],      0(%[imageBlock])             \n\t"
                "lw              %[tmp2a],      24(%[pRes])                  \n\t"
                "shll_s.ph       %[tmp2],       %[tmp2],        7            \n\t"
                "shll_s.ph       %[tmp4],       %[tmp4],        7            \n\t"
                "lw              %[tmp4a],      28(%[pRes])                  \n\t"
                "preceu.ph.qbr   %[tmp11],      %[tmp1]                      \n\t"
                "preceu.ph.qbl   %[tmp12],      %[tmp1]                      \n\t"
                "precrqu_s.qb.ph %[tmp2],       %[tmp4],        %[tmp2]      \n\t"
                "addu.ph         %[tmp2a],      %[tmp2a],       %[tmp11]     \n\t"
                "addu.ph         %[tmp4a],      %[tmp4a],       %[tmp12]     \n\t"
                "addu            %[imageBlock], %[imageBlock],  %[picWidth]  \n\t"
                "shll_s.ph       %[tmp2a],      %[tmp2a],       7            \n\t"
                "shll_s.ph       %[tmp4a],      %[tmp4a],       7            \n\t"
                "sw              %[tmp2],       0(%[imageBlock])             \n\t"
                "addu            %[imageBlock], %[imageBlock],  %[picWidth]  \n\t"
                "precrqu_s.qb.ph %[tmp2a],      %[tmp4a],       %[tmp2a]     \n\t"
                "sw              %[tmp2a],      0(%[imageBlock])             \n\t"

                : [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2), [tmp4] "=&r" (tmp4),
                  [tmp2a] "=&r" (tmp2a), [tmp4a] "=&r" (tmp4a),
                  [tmp11] "=&r" (tmp11), [tmp12] "=&r" (tmp3),
                  [imageBlock] "+r" (imageBlock)
                : [picWidth] "r" (picWidth), [pRes] "r" (pRes), [tmp] "r" (tmp)
                : "memory"
            );

        }
    }

}


