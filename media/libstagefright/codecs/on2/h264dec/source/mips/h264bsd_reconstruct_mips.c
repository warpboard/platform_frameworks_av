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

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "../h264bsd_reconstruct.h"
#include "h264bsd_reconstruct_mips.h"
#include "../h264bsd_macroblock_layer.h"
#include "../h264bsd_image.h"
#include "../h264bsd_util.h"


/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/* Switch off the following Lint messages for this file:
 * Info 701: Shift left of signed quantity (int)
 * Info 702: Shift right of signed quantity (int)
 */
/*lint -e701 -e702 */

/* Luma fractional-sample positions
 *
 *  G a b c H
 *  d e f g
 *  h i j k m
 *  n p q r
 *  M   s   N
 *
 *  G, H, M and N are integer sample positions
 *  a-s are fractional samples that need to be interpolated.
 */
static const u32 lumaFracPos[4][4] = {
  /* G  d  h  n    a  e  i  p    b  f  j   q     c   g   k   r */
    {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15}};

/* clipping table, defined in h264bsd_intra_prediction.c */
extern const u8 h264bsdClip[];

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateChromaHor

        Functional description:
          This function performs chroma interpolation in horizontal direction.
          Overfilling is done only if needed. Reference image (pRef) is
          read at correct position and the predicted part is written to
          macroblock's chrominance (predPartChroma)
        Inputs:
          pRef              pointer to reference frame Cb top-left corner
          x0                integer x-coordinate for prediction
          y0                integer y-coordinate for prediction
          width             width of the reference frame chrominance in pixels
          height            height of the reference frame chrominance in pixels
          xFrac             horizontal fraction for prediction in 1/8 pixels
          chromaPartWidth   width of the predicted part in pixels
          chromaPartHeight  height of the predicted part in pixels
        Outputs:
          predPartChroma    pointer where predicted part is written

------------------------------------------------------------------------------*/
void h264bsdInterpolateChromaHor(
  u8 *pRef,
  u8 *predPartChroma,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 xFrac,
  u32 chromaPartWidth,
  u32 chromaPartHeight)
{

/* Variables */

    u32 x, y, val;
    u8 *ptrA, *cbr;
    u32 comp;
    u8 block[9*8*2];

/* Code */

    ASSERT(predPartChroma);
    ASSERT(chromaPartWidth);
    ASSERT(chromaPartHeight);
    ASSERT(xFrac < 8);
    ASSERT(pRef);

    if ((x0 < 0) || ((u32)x0+chromaPartWidth+1 > width) ||
        (y0 < 0) || ((u32)y0+chromaPartHeight > height))
    {
        h264bsdFillBlock(pRef, block, x0, y0, width, height,
            chromaPartWidth + 1, chromaPartHeight, chromaPartWidth + 1);
        pRef += width * height;
        h264bsdFillBlock(pRef, block + (chromaPartWidth+1)*chromaPartHeight,
            x0, y0, width, height, chromaPartWidth + 1,
            chromaPartHeight, chromaPartWidth + 1);

        pRef = block;
        x0 = 0;
        y0 = 0;
        width = chromaPartWidth+1;
        height = chromaPartHeight;
    }

    val = 8 - xFrac;
    u32 xFrac_1, val_1;

    __asm__ volatile (
         "replv.ph           %[val_1],       %[val]                       \n\t"
         "replv.ph           %[xFrac_1],     %[xFrac]                     \n\t"

         : [val_1] "=&r" (val_1), [xFrac_1] "=&r" (xFrac_1)
         : [val] "r" (val), [xFrac] "r" (xFrac)
    );

    for (comp = 0; comp <= 1; comp++)
    {

        ptrA = pRef + (comp * height + (u32)y0) * width + x0;
        cbr = predPartChroma + comp * 8 * 8;

        /* 2x2 pels per iteration
         * bilinear horizontal interpolation */
        for (y = (chromaPartHeight >> 1); y; y--)
        {
            for (x = (chromaPartWidth >> 1); x; x-=2)
            {
                u32 tmp1, tmp2, tmp3, tmp4;
                u32 tmp5, tmp6, tmp7, tmp8;

                __asm__ volatile (
                    "addu           %[tmp4],   %[ptrA],      %[width]   \n\t"
                    "ulw            %[tmp1],   0(%[ptrA])               \n\t"
                    "ulw            %[tmp3],   1(%[ptrA])               \n\t"
                    "ulw            %[tmp2],   0(%[tmp4])               \n\t"
                    "ulw            %[tmp4],   1(%[tmp4])               \n\t"
                    "preceu.ph.qbr  %[tmp5],   %[tmp1]                  \n\t"
                    "preceu.ph.qbr  %[tmp6],   %[tmp3]                  \n\t"
                    "preceu.ph.qbr  %[tmp7],   %[tmp2]                  \n\t"
                    "preceu.ph.qbr  %[tmp8],   %[tmp4]                  \n\t"
                    "mul.ph         %[tmp5],   %[tmp5],      %[val_1]   \n\t"
                    "mul.ph         %[tmp6],   %[tmp6],      %[xFrac_1] \n\t"
                    "mul.ph         %[tmp7],   %[tmp7],      %[val_1]   \n\t"
                    "mul.ph         %[tmp8],   %[tmp8],      %[xFrac_1] \n\t"
                    "addq.ph        %[tmp5],   %[tmp5],      %[tmp6]    \n\t"
                    "addq.ph        %[tmp7],   %[tmp7],      %[tmp8]    \n\t"
                    "shra_r.ph      %[tmp5],   %[tmp5],      3          \n\t"
                    "shra_r.ph      %[tmp7],   %[tmp7],      3          \n\t"
                    "preceu.ph.qbl  %[tmp1],   %[tmp1]                  \n\t"
                    "preceu.ph.qbl  %[tmp3],   %[tmp3]                  \n\t"
                    "preceu.ph.qbl  %[tmp2],   %[tmp2]                  \n\t"
                    "preceu.ph.qbl  %[tmp4],   %[tmp4]                  \n\t"
                    "mul.ph         %[tmp1],   %[tmp1],      %[val_1]   \n\t"
                    "mul.ph         %[tmp3],   %[tmp3],      %[xFrac_1] \n\t"
                    "mul.ph         %[tmp2],   %[tmp2],      %[val_1]   \n\t"
                    "mul.ph         %[tmp4],   %[tmp4],      %[xFrac_1] \n\t"
                    "addq.ph        %[tmp1],   %[tmp1],      %[tmp3]    \n\t"
                    "addq.ph        %[tmp2],   %[tmp2],      %[tmp4]    \n\t"
                    "shra_r.ph      %[tmp1],   %[tmp1],      3          \n\t"
                    "shra_r.ph      %[tmp2],   %[tmp2],      3          \n\t"
                    "addiu          %[ptrA],   %[ptrA],      4          \n\t"
                    "precr.qb.ph    %[tmp5],   %[tmp1],      %[tmp5]    \n\t"
                    "precr.qb.ph    %[tmp7],   %[tmp2],      %[tmp7]    \n\t"
                    "sw             %[tmp5],   0(%[cbr])                \n\t"
                    "sw             %[tmp7],   8(%[cbr])                \n\t"
                    "addiu          %[cbr],    %[cbr],       4          \n\t"

                    : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                      [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                      [tmp7] "=&r" (tmp7), [tmp8] "=&r" (tmp8),
                      [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2),
                      [ptrA] "+r" (ptrA), [cbr] "+r" (cbr)
                    : [width] "r" (width), [val_1] "r" (val_1), [xFrac_1] "r" (xFrac_1)
                    : "hi", "lo", "memory"
                );
            }
            cbr += 2*8 - chromaPartWidth;
            ptrA += 2*width - chromaPartWidth;
        }
    }
}

/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateChromaVer

        Functional description:
          This function performs chroma interpolation in vertical direction.
          Overfilling is done only if needed. Reference image (pRef) is
          read at correct position and the predicted part is written to
          macroblock's chrominance (predPartChroma)

------------------------------------------------------------------------------*/
void h264bsdInterpolateChromaVer(
  u8 *pRef,
  u8 *predPartChroma,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 yFrac,
  u32 chromaPartWidth,
  u32 chromaPartHeight)
{

/* Variables */

    u32 x, y, val;
    u8 *ptrA, *cbr;
    u32 comp;
    u8 block[9*8*2];

/* Code */

    ASSERT(predPartChroma);
    ASSERT(chromaPartWidth);
    ASSERT(chromaPartHeight);
    ASSERT(yFrac < 8);
    ASSERT(pRef);

    if ((x0 < 0) || ((u32)x0+chromaPartWidth > width) ||
        (y0 < 0) || ((u32)y0+chromaPartHeight+1 > height))
    {
        h264bsdFillBlock(pRef, block, x0, y0, width, height, chromaPartWidth,
            chromaPartHeight + 1, chromaPartWidth);
        pRef += width * height;
        h264bsdFillBlock(pRef, block + chromaPartWidth*(chromaPartHeight+1),
            x0, y0, width, height, chromaPartWidth,
            chromaPartHeight + 1, chromaPartWidth);

        pRef = block;
        x0 = 0;
        y0 = 0;
        width = chromaPartWidth;
        height = chromaPartHeight+1;
    }

    val = 8 - yFrac;
    u32 xFrac_1, val_1;

    __asm__ volatile (
        "replv.ph           %[val_1],       %[val]                       \n\t"
        "replv.ph           %[xFrac_1],     %[yFrac]                     \n\t"

        : [val_1] "=&r" (val_1), [xFrac_1] "=&r" (xFrac_1)
        : [val] "r" (val), [yFrac] "r" (yFrac)
    );

    for (comp = 0; comp <= 1; comp++)
    {

        ptrA = pRef + (comp * height + (u32)y0) * width + x0;
        cbr = predPartChroma + comp * 8 * 8;

        /* 2x2 pels per iteration
         * bilinear vertical interpolation */
        for (y = (chromaPartHeight >> 1); y; y--)
        {
            for (x = (chromaPartWidth >> 1); x; x--)
            {
                u32 tmp1, tmp2, tmp3, tmp4;
                u32 tmp5, tmp6, tmp7, tmp8;

                __asm__ volatile (
                    "addu            %[tmp4],   %[ptrA],     %[width]    \n\t"
                    "addu            %[tmp3],   %[tmp4],     %[width]    \n\t"
                    "ulh             %[tmp1],   0(%[ptrA])               \n\t"
                    "ulh             %[tmp2],   0(%[tmp4])               \n\t"
                    "ulh             %[tmp3],   0(%[tmp3])               \n\t"
                    "preceu.ph.qbr   %[tmp5],   %[tmp1]                  \n\t"
                    "preceu.ph.qbr   %[tmp7],   %[tmp2]                  \n\t"
                    "preceu.ph.qbr   %[tmp8],   %[tmp3]                  \n\t"
                    "mul.ph          %[tmp5],   %[tmp5],     %[val_1]    \n\t"
                    "mul.ph          %[tmp6],   %[tmp7],     %[xFrac_1]  \n\t"
                    "mul.ph          %[tmp8],   %[tmp8],     %[xFrac_1]  \n\t"
                    "mul.ph          %[tmp4],   %[tmp7],     %[val_1]    \n\t"
                    "addq.ph         %[tmp5],   %[tmp6],     %[tmp5]     \n\t"
                    "addq.ph         %[tmp8],   %[tmp4],     %[tmp8]     \n\t"
                    "shra_r.ph       %[tmp5],   %[tmp5],     3           \n\t"
                    "shra_r.ph       %[tmp8],   %[tmp8],     3           \n\t"
                    "addiu           %[cbr],    %[cbr],      2           \n\t"
                    "addiu           %[ptrA],   %[ptrA],     2           \n\t"
                    "precr.qb.ph     %[tmp5],   %[tmp5],     %[tmp5]     \n\t"
                    "precr.qb.ph     %[tmp8],   %[tmp8],     %[tmp8]     \n\t"
                    "sh              %[tmp5],   -2(%[cbr])               \n\t"
                    "sh              %[tmp8],   6(%[cbr])                \n\t"

                    : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                      [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                      [tmp7] "=&r" (tmp7), [tmp8] "=&r" (tmp8),
                      [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2),
                      [ptrA] "+r" (ptrA), [cbr] "+r" (cbr)
                    : [width] "r" (width), [val_1] "r" (val_1),
                      [xFrac_1] "r" (xFrac_1)
                    : "hi", "lo", "memory"
                );
            }
            cbr += 2*8 - chromaPartWidth;
            ptrA += 2*width - chromaPartWidth;
        }
    }

}

/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateChromaHorVer

        Functional description:
          This function performs chroma interpolation in horizontal and
          vertical direction. Overfilling is done only if needed. Reference
          image (ref) is read at correct position and the predicted part
          is written to macroblock's chrominance (predPartChroma)

------------------------------------------------------------------------------*/
void h264bsdInterpolateChromaHorVer(
  u8 *ref,
  u8 *predPartChroma,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 xFrac,
  u32 yFrac,
  u32 chromaPartWidth,
  u32 chromaPartHeight)
{
    u8 block[9*9*2];
    u32 y, tmp1, tmp2, tmp3, tmp4;
    u32 comp;
    u8 *ptrA, *cbr;

/* Code */

    ASSERT(predPartChroma);
    ASSERT(chromaPartWidth);
    ASSERT(chromaPartHeight);
    ASSERT(xFrac < 8);
    ASSERT(yFrac < 8);
    ASSERT(ref);

    if ((x0 < 0) || ((u32)x0+chromaPartWidth+1 > width) ||
        (y0 < 0) || ((u32)y0+chromaPartHeight+1 > height))
    {
        h264bsdFillBlock(ref, block, x0, y0, width, height,
            chromaPartWidth + 1, chromaPartHeight + 1, chromaPartWidth + 1);
        ref += width * height;
        h264bsdFillBlock(ref, block + (chromaPartWidth+1)*(chromaPartHeight+1),
            x0, y0, width, height, chromaPartWidth + 1,
            chromaPartHeight + 1, chromaPartWidth + 1);

        ref = block;
        x0 = 0;
        y0 = 0;
        width = chromaPartWidth+1;
        height = chromaPartHeight+1;
    }

    i8 *temp_ptrA;
    i32 t_0 = (8 - yFrac) * (8-xFrac);
    i32 t_1 = (8 - xFrac) * yFrac;
    i32 t_2 = (8 - yFrac) * xFrac;
    i32 t_3 = yFrac * xFrac;

    i32 t_0_2 = t_2 << 8;
    i32 t_1_3 = t_3 << 8;

    t_0_2 =  t_0_2 | t_0;
    t_1_3 =  t_1_3 | t_1;

    __asm__ volatile (
        "replv.ph   %[t_0_2], %[t_0_2]                           \n\t"
        "replv.ph   %[t_1_3], %[t_1_3]                           \n\t"

         : [t_0_2] "+r" (t_0_2),
           [t_1_3] "+r" (t_1_3)
         :
    );

    int tmp_cbr  = 2*8 - chromaPartWidth + 4;
    int tmp_ptrA = 2*width - chromaPartWidth + 4;

    if(chromaPartWidth>4){
        for (comp = 0; comp <= 1; comp++)
        {

            ptrA = ref + (comp * height + (u32)y0) * width + x0;
            cbr = predPartChroma + comp * 8 * 8;

            /* 2x2 pels per iteration
             * bilinear vertical and horizontal interpolation */
            for (y = (chromaPartHeight >> 1); y; y--)
            {
                i32 tmp00, tmp01, tmp10, tmp11, tmp21, tmp31;
                {
                    u8 *pp1, *pp2, *pp3, *pp4;
                    pp1 = ptrA + tmp_ptrA;
                    pp4 = cbr + tmp_cbr;

                    /* prefetch data for the next loop iteration */

                    __asm__ volatile (
                        "addu       %[pp2], %[pp1],    %[width]           \n\t"
                        "addu       %[pp3], %[pp2],    %[width]           \n\t"
                        "pref       0,      0(%[pp1])                     \n\t"       /* prefetch for load for the next loop iteration */
                        "pref       0,      0(%[pp2])                     \n\t"
                        "pref       0,      0(%[pp3])                     \n\t"
                        "pref       1,      0(%[pp4])                     \n\t"       /* prefetch for store for the next loop iteration */

                        : [pp3] "=&r" (pp3), [pp2] "=r" (pp2)
                        : [width] "r" (width), [pp1] "r" (pp1), [pp4] "r" (pp4)
                        : "memory"
                    );
                }

                __asm__ volatile (
                    "add         %[temp_ptrA], %[ptrA],         %[width]  \n\t"
                    "ulw         %[tmp00],     0(%[ptrA])                 \n\t"
                    "ulw         %[tmp10],     1(%[ptrA])                 \n\t"
                    "ulw         %[tmp01],     0(%[temp_ptrA])            \n\t"
                    "ulw         %[tmp11],     1(%[temp_ptrA])            \n\t"
                    "add         %[temp_ptrA], %[temp_ptrA],    %[width]  \n\t"
                    "mult        $ac0,         $zero,           $zero     \n\t"
                    "mult        $ac1,         $zero,           $zero     \n\t"
                    "mult        $ac2,         $zero,           $zero     \n\t"
                    "mult        $ac3,         $zero,           $zero     \n\t"
                    "ulw         %[tmp21],     0(%[temp_ptrA])            \n\t"
                    "ulw         %[tmp31],     1(%[temp_ptrA])            \n\t"
                    "dpau.h.qbr  $ac0,         %[t_0_2],        %[tmp00]  \n\t"
                    "dpau.h.qbr  $ac1,         %[t_0_2],        %[tmp10]  \n\t"
                    "dpau.h.qbr  $ac2,         %[t_0_2],        %[tmp01]  \n\t"
                    "dpau.h.qbr  $ac3,         %[t_0_2],        %[tmp11]  \n\t"
                    "dpau.h.qbr  $ac0,         %[t_1_3],        %[tmp01]  \n\t"
                    "dpau.h.qbr  $ac1,         %[t_1_3],        %[tmp11]  \n\t"
                    "dpau.h.qbr  $ac2,         %[t_1_3],        %[tmp21]  \n\t"
                    "dpau.h.qbr  $ac3,         %[t_1_3],        %[tmp31]  \n\t"
                    "extr_r.w    %[tmp1],      $ac0,            6         \n\t"
                    "extr_r.w    %[tmp2],      $ac1,            6         \n\t"
                    "extr_r.w    %[tmp3],      $ac2,            6         \n\t"
                    "extr_r.w    %[tmp4],      $ac3,            6         \n\t"
                    "addiu       %[ptrA],      %[ptrA],         4         \n\t"
                    "addu        %[temp_ptrA], %[ptrA],         %[width]  \n\t"
                    "mult        $ac0,         $zero,           $zero     \n\t"
                    "mult        $ac1,         $zero,           $zero     \n\t"
                    "mult        $ac2,         $zero,           $zero     \n\t"
                    "mult        $ac3,         $zero,           $zero     \n\t"
                    "dpau.h.qbl  $ac0,         %[t_0_2],        %[tmp00]  \n\t"
                    "dpau.h.qbl  $ac1,         %[t_0_2],        %[tmp10]  \n\t"
                    "dpau.h.qbl  $ac2,         %[t_0_2],        %[tmp01]  \n\t"
                    "dpau.h.qbl  $ac3,         %[t_0_2],        %[tmp11]  \n\t"
                    "dpau.h.qbl  $ac0,         %[t_1_3],        %[tmp01]  \n\t"
                    "dpau.h.qbl  $ac1,         %[t_1_3],        %[tmp11]  \n\t"
                    "dpau.h.qbl  $ac2,         %[t_1_3],        %[tmp21]  \n\t"
                    "dpau.h.qbl  $ac3,         %[t_1_3],        %[tmp31]  \n\t"
                    "extr_r.w    %[tmp00],     $ac0,            6         \n\t"
                    "extr_r.w    %[tmp11],     $ac1,            6         \n\t"
                    "extr_r.w    %[tmp21],     $ac2,            6         \n\t"
                    "extr_r.w    %[tmp31],     $ac3,            6         \n\t"
                    "sb          %[tmp1],      0(%[cbr])                  \n\t"
                    "sb          %[tmp2],      1(%[cbr])                  \n\t"
                    "sb          %[tmp3],      8(%[cbr])                  \n\t"
                    "sb          %[tmp4],      9(%[cbr])                  \n\t"
                    "sb          %[tmp00],     2(%[cbr])                  \n\t"
                    "sb          %[tmp11],     3(%[cbr])                  \n\t"
                    "sb          %[tmp21],     10(%[cbr])                 \n\t"
                    "sb          %[tmp31],     11(%[cbr])                 \n\t"
                    "addiu       %[cbr],       %[cbr],          4         \n\t"
                    "ulw         %[tmp00],     0(%[ptrA])                 \n\t"
                    "ulw         %[tmp10],     1(%[ptrA])                 \n\t"
                    "ulw         %[tmp01],     0(%[temp_ptrA])            \n\t"
                    "ulw         %[tmp11],     1(%[temp_ptrA])            \n\t"
                    "add         %[temp_ptrA], %[temp_ptrA],    %[width]  \n\t"
                    "mult        $ac0,         $zero,           $zero     \n\t"
                    "mult        $ac1,         $zero,           $zero     \n\t"
                    "mult        $ac2,         $zero,           $zero     \n\t"
                    "mult        $ac3,         $zero,           $zero     \n\t"
                    "ulw         %[tmp21],     0(%[temp_ptrA])            \n\t"
                    "ulw         %[tmp31],     1(%[temp_ptrA])            \n\t"
                    "dpau.h.qbr  $ac0,         %[t_0_2],        %[tmp00]  \n\t"
                    "dpau.h.qbr  $ac1,         %[t_0_2],        %[tmp10]  \n\t"
                    "dpau.h.qbr  $ac2,         %[t_0_2],        %[tmp01]  \n\t"
                    "dpau.h.qbr  $ac3,         %[t_0_2],        %[tmp11]  \n\t"
                    "dpau.h.qbr  $ac0,         %[t_1_3],        %[tmp01]  \n\t"
                    "dpau.h.qbr  $ac1,         %[t_1_3],        %[tmp11]  \n\t"
                    "dpau.h.qbr  $ac2,         %[t_1_3],        %[tmp21]  \n\t"
                    "dpau.h.qbr  $ac3,         %[t_1_3],        %[tmp31]  \n\t"
                    "extr_r.w    %[tmp1],      $ac0,            6         \n\t"
                    "extr_r.w    %[tmp2],      $ac1,            6         \n\t"
                    "extr_r.w    %[tmp3],      $ac2,            6         \n\t"
                    "extr_r.w    %[tmp4],      $ac3,            6         \n\t"
                    "addu        %[ptrA],      %[tmp_ptrA]                \n\t"
                    "mult        $ac0,         $zero,           $zero     \n\t"
                    "mult        $ac1,         $zero,           $zero     \n\t"
                    "mult        $ac2,         $zero,           $zero     \n\t"
                    "mult        $ac3,         $zero,           $zero     \n\t"
                    "dpau.h.qbl  $ac0,         %[t_0_2],        %[tmp00]  \n\t"
                    "dpau.h.qbl  $ac1,         %[t_0_2],        %[tmp10]  \n\t"
                    "dpau.h.qbl  $ac2,         %[t_0_2],        %[tmp01]  \n\t"
                    "dpau.h.qbl  $ac3,         %[t_0_2],        %[tmp11]  \n\t"
                    "dpau.h.qbl  $ac0,         %[t_1_3],        %[tmp01]  \n\t"
                    "dpau.h.qbl  $ac1,         %[t_1_3],        %[tmp11]  \n\t"
                    "dpau.h.qbl  $ac2,         %[t_1_3],        %[tmp21]  \n\t"
                    "dpau.h.qbl  $ac3,         %[t_1_3],        %[tmp31]  \n\t"
                    "extr_r.w    %[tmp00],     $ac0,            6         \n\t"
                    "extr_r.w    %[tmp11],     $ac1,            6         \n\t"
                    "extr_r.w    %[tmp21],     $ac2,            6         \n\t"
                    "extr_r.w    %[tmp31],     $ac3,            6         \n\t"
                    "sb          %[tmp1],      0(%[cbr])                  \n\t"
                    "sb          %[tmp2],      1(%[cbr])                  \n\t"
                    "sb          %[tmp3],      8(%[cbr])                  \n\t"
                    "sb          %[tmp4],      9(%[cbr])                  \n\t"
                    "sb          %[tmp00],     2(%[cbr])                  \n\t"
                    "sb          %[tmp11],     3(%[cbr])                  \n\t"
                    "sb          %[tmp21],     10(%[cbr])                 \n\t"
                    "sb          %[tmp31],     11(%[cbr])                 \n\t"
                    "addu        %[cbr],       %[tmp_cbr]                 \n\t"

                    : [tmp00] "=&r" (tmp00), [tmp01] "=&r" (tmp01),
                      [tmp10] "=&r" (tmp10), [tmp11] "=&r" (tmp11),
                      [tmp21] "=&r" (tmp21), [tmp31] "=&r" (tmp31),
                      [tmp1] "=&r" (tmp1), [tmp3] "=&r" (tmp3),
                      [tmp2] "=&r" (tmp2), [tmp4] "=&r" (tmp4),
                      [ptrA] "+r" (ptrA), [cbr] "+r" (cbr),
                      [temp_ptrA] "=&r" (temp_ptrA)
                    : [t_0_2] "r" (t_0_2), [t_1_3] "r" (t_1_3),
                      [tmp_cbr] "r" (tmp_cbr), [tmp_ptrA] "r" (tmp_ptrA),
                      [width] "r" (width)
                    : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
                      "$ac3hi", "$ac3lo", "memory"
                );
            }
        }
    }else{

        for (comp = 0; comp <= 1; comp++)
        {
            ptrA = ref + (comp * height + (u32)y0) * width + x0;
            cbr = predPartChroma + comp * 8 * 8;

            /* 2x2 pels per iteration
             * bilinear vertical and horizontal interpolation */

            for (y = (chromaPartHeight >> 1); y; y--)
            {

                i32 tmp00, tmp01, tmp10, tmp11, tmp21, tmp31;
                {
                    u8 *pp1, *pp2, *pp3, *pp4;
                    pp1 = ptrA + tmp_ptrA;
                    pp4 = cbr + tmp_cbr;

                    /* prefetch data for the next loop iteration */

                    __asm__ volatile (
                        "addu       %[pp2], %[pp1],    %[width]           \n\t"
                        "addu       %[pp3], %[pp2],    %[width]           \n\t"
                        "pref       0,      0(%[pp1])                     \n\t"       /* prefetch for load for the next loop iteration */
                        "pref       0,      0(%[pp2])                     \n\t"
                        "pref       0,      0(%[pp3])                     \n\t"
                        "pref       1,      0(%[pp4])                     \n\t"       /* prefetch for store for the next loop iteration */

                        : [pp2] "=&r" (pp2), [pp3] "=r" (pp3)
                        : [width] "r" (width), [pp4] "r" (pp4), [pp1] "r" (pp1)
                        : "memory"
                    );
                }

                __asm__ volatile (
                    "addu        %[temp_ptrA], %[ptrA],         %[width]  \n\t"
                    "ulw         %[tmp00],     0(%[ptrA])                 \n\t"
                    "ulw         %[tmp10],     1(%[ptrA])                 \n\t"
                    "ulw         %[tmp01],     0(%[temp_ptrA])            \n\t"
                    "ulw         %[tmp11],     1(%[temp_ptrA])            \n\t"
                    "addu        %[temp_ptrA], %[temp_ptrA],    %[width]  \n\t"
                    "mult        $ac0,         $zero,           $zero     \n\t"
                    "mult        $ac1,         $zero,           $zero     \n\t"
                    "mult        $ac2,         $zero,           $zero     \n\t"
                    "mult        $ac3,         $zero,           $zero     \n\t"
                    "ulw         %[tmp21],     0(%[temp_ptrA])            \n\t"
                    "ulw         %[tmp31],     1(%[temp_ptrA])            \n\t"
                    "dpau.h.qbr  $ac0,         %[t_0_2],        %[tmp00]  \n\t"
                    "dpau.h.qbr  $ac1,         %[t_0_2],        %[tmp10]  \n\t"
                    "dpau.h.qbr  $ac2,         %[t_0_2],        %[tmp01]  \n\t"
                    "dpau.h.qbr  $ac3,         %[t_0_2],        %[tmp11]  \n\t"
                    "dpau.h.qbr  $ac0,         %[t_1_3],        %[tmp01]  \n\t"
                    "dpau.h.qbr  $ac1,         %[t_1_3],        %[tmp11]  \n\t"
                    "dpau.h.qbr  $ac2,         %[t_1_3],        %[tmp21]  \n\t"
                    "dpau.h.qbr  $ac3,         %[t_1_3],        %[tmp31]  \n\t"
                    "extr_r.w    %[tmp1],      $ac0,            6         \n\t"
                    "extr_r.w    %[tmp2],      $ac1,            6         \n\t"
                    "extr_r.w    %[tmp3],      $ac2,            6         \n\t"
                    "extr_r.w    %[tmp4],      $ac3,            6         \n\t"
                    "addu        %[ptrA],      %[tmp_ptrA]                \n\t"
                    "mult        $ac0,         $zero,           $zero     \n\t"
                    "mult        $ac1,         $zero,           $zero     \n\t"
                    "mult        $ac2,         $zero,           $zero     \n\t"
                    "mult        $ac3,         $zero,           $zero     \n\t"
                    "dpau.h.qbl  $ac0,         %[t_0_2],        %[tmp00]  \n\t"
                    "dpau.h.qbl  $ac1,         %[t_0_2],        %[tmp10]  \n\t"
                    "dpau.h.qbl  $ac2,         %[t_0_2],        %[tmp01]  \n\t"
                    "dpau.h.qbl  $ac3,         %[t_0_2],        %[tmp11]  \n\t"
                    "dpau.h.qbl  $ac0,         %[t_1_3],        %[tmp01]  \n\t"
                    "dpau.h.qbl  $ac1,         %[t_1_3],        %[tmp11]  \n\t"
                    "dpau.h.qbl  $ac2,         %[t_1_3],        %[tmp21]  \n\t"
                    "dpau.h.qbl  $ac3,         %[t_1_3],        %[tmp31]  \n\t"
                    "extr_r.w    %[tmp00],     $ac0,            6         \n\t"
                    "extr_r.w    %[tmp11],     $ac1,            6         \n\t"
                    "extr_r.w    %[tmp21],     $ac2,            6         \n\t"
                    "extr_r.w    %[tmp31],     $ac3,            6         \n\t"
                    "sb          %[tmp1],      0(%[cbr])                  \n\t"
                    "sb          %[tmp2],      1(%[cbr])                  \n\t"
                    "sb          %[tmp3],      8(%[cbr])                  \n\t"
                    "sb          %[tmp4],      9(%[cbr])                  \n\t"
                    "sb          %[tmp00],     2(%[cbr])                  \n\t"
                    "sb          %[tmp11],     3(%[cbr])                  \n\t"
                    "sb          %[tmp21],     10(%[cbr])                 \n\t"
                    "sb          %[tmp31],     11(%[cbr])                 \n\t"
                    "addu        %[cbr],       %[tmp_cbr]                 \n\t"

                    : [tmp00] "=&r" (tmp00), [tmp01] "=&r" (tmp01),
                      [tmp10] "=&r" (tmp10), [tmp11] "=&r" (tmp11),
                      [tmp21] "=&r" (tmp21), [tmp31] "=&r" (tmp31),
                      [tmp1] "=&r" (tmp1), [tmp3] "=&r" (tmp3),
                      [tmp2] "=&r" (tmp2), [tmp4] "=&r" (tmp4),
                      [ptrA] "+r" (ptrA), [cbr] "+r" (cbr),
                      [temp_ptrA] "=&r" (temp_ptrA)
                    : [t_0_2] "r" (t_0_2), [t_1_3] "r" (t_1_3),
                      [tmp_cbr] "r" (tmp_cbr), [tmp_ptrA] "r" (tmp_ptrA),
                      [width] "r" (width)
                    : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
                      "$ac3hi", "$ac3lo", "memory"
                );
            }
        }
    }
}


/*------------------------------------------------------------------------------

    Function: PredictChroma

        Functional description:
          Top level chroma prediction function that calls the appropriate
          interpolation function. The output is written to macroblock array.

------------------------------------------------------------------------------*/

static void PredictChroma(
  u8 *mbPartChroma,
  u32 xAL,
  u32 yAL,
  u32 partWidth,
  u32 partHeight,
  mv_t *mv,
  image_t *refPic)
{

/* Variables */

    u32 xFrac, yFrac, width, height, chromaPartWidth, chromaPartHeight;
    i32 xInt, yInt;
    u8 *ref;

/* Code */

    ASSERT(mv);
    ASSERT(refPic);
    ASSERT(refPic->data);
    ASSERT(refPic->width);
    ASSERT(refPic->height);

    width  = 8 * refPic->width;
    height = 8 * refPic->height;

    xInt = (xAL >> 1) + (mv->hor >> 3);
    yInt = (yAL >> 1) + (mv->ver >> 3);
    xFrac = mv->hor & 0x7;
    yFrac = mv->ver & 0x7;

    chromaPartWidth  = partWidth >> 1;
    chromaPartHeight = partHeight >> 1;
    ref = refPic->data + 256 * refPic->width * refPic->height;

    if (xFrac && yFrac)
    {
        h264bsdInterpolateChromaHorVer(ref, mbPartChroma, xInt, yInt, width,
                height, xFrac, yFrac, chromaPartWidth, chromaPartHeight);
    }
    else if (xFrac)
    {
        h264bsdInterpolateChromaHor(ref, mbPartChroma, xInt, yInt, width,
                height, xFrac, chromaPartWidth, chromaPartHeight);
    }
    else if (yFrac)
    {
        h264bsdInterpolateChromaVer(ref, mbPartChroma, xInt, yInt, width,
                height, yFrac, chromaPartWidth, chromaPartHeight);
    }
    else
    {
        h264bsdFillBlock(ref, mbPartChroma, xInt, yInt, width, height,
            chromaPartWidth, chromaPartHeight, 8);
        ref += width * height;
        h264bsdFillBlock(ref, mbPartChroma + 8*8, xInt, yInt, width, height,
            chromaPartWidth, chromaPartHeight, 8);
    }

}


/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateVerHalf

        Functional description:
          Function to perform vertical interpolation of pixel position 'h'
          for a block. Overfilling is done only if needed. Reference
          image (ref) is read at correct position and the predicted part
          is written to macroblock array (mb)

------------------------------------------------------------------------------*/
void h264bsdInterpolateVerHalf(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight)
{
    u32 p1[21*21/4+1];
    u32 i, j;
    u8 *ptrC, *ptrV;

    /* Code */

    ASSERT(ref);
    ASSERT(mb);

    if ((x0 < 0) || ((u32)x0+partWidth > width) ||
        (y0 < 0) || ((u32)y0+partHeight+5 > height))
    {
        h264bsdFillBlock(ref, (u8*)p1, x0, y0, width, height,
                partWidth, partHeight+5, partWidth);

        x0 = 0;
        y0 = 0;
        ref = (u8*)p1;
        width = partWidth;
    }

    ref += (u32)y0 * width + (u32)x0;

    ptrC = ref + width;
    ptrV = ptrC + 5*width;

    i32 tmp_const_5, tmp_const_20;

    __asm__ volatile (
        "repl.ph         %[tmp_const_5],          5                \n\t"
        "repl.ph         %[tmp_const_20],         20               \n\t"

        : [tmp_const_5] "=&r" (tmp_const_5),
          [tmp_const_20] "=&r" (tmp_const_20)
        :
    );

    /* 4 pixels per iteration, interpolate using 5 vertical samples */

    for (i = (partHeight >> 2); i; i--)
    {
        for (j = partWidth; j; j-=4)
        {
            u8 * temp_ptrC;
            i32 tmp6, tmp5, tmp4, tmp3,tmp2, tmp1;
            i32 tmp_R_4, tmp_R_1, tmp_R_5, tmp_R_6, tmp_R_3, tmp_R_2;
            i32 tmp_L_4, tmp_L_1, tmp_L_5, tmp_L_6, tmp_L_3, tmp_L_2;

            __asm__ volatile (
                "sll              %[tmp2],      %[width],       1                \n\t"
                "addu             %[tmp3],      %[ptrC],        %[tmp2]          \n\t"
                "addu             %[tmp4],      %[tmp3],        %[width]         \n\t"
                "addu             %[tmp5],      %[tmp4],        %[width]         \n\t"
                "addu             %[tmp6],      %[tmp5],        %[width]         \n\t"
                "addu             %[tmp1],      %[tmp6],        %[width]         \n\t"
                "addu             %[tmp2],      %[tmp1],        %[width]         \n\t"
                "ulw              %[tmp3],      0(%[tmp3])                       \n\t"
                "ulw              %[tmp4],      0(%[tmp4])                       \n\t"
                "ulw              %[tmp5],      0(%[tmp5])                       \n\t"
                "ulw              %[tmp6],      0(%[tmp6])                       \n\t"
                "ulw              %[tmp1],      0(%[tmp1])                       \n\t"
                "ulw              %[tmp2],      0(%[tmp2])                       \n\t"
                "preceu.ph.qbr    %[tmp_R_4],   %[tmp4]                          \n\t"
                "preceu.ph.qbr    %[tmp_R_1],   %[tmp1]                          \n\t"
                "preceu.ph.qbr    %[tmp_R_5],   %[tmp5]                          \n\t"
                "preceu.ph.qbr    %[tmp_R_6],   %[tmp6]                          \n\t"
                "preceu.ph.qbr    %[tmp_R_2],   %[tmp2]                          \n\t"
                "preceu.ph.qbr    %[tmp_R_3],   %[tmp3]                          \n\t"
                "preceu.ph.qbl    %[tmp_L_4],   %[tmp4]                          \n\t"
                "preceu.ph.qbl    %[tmp_L_1],   %[tmp1]                          \n\t"
                "preceu.ph.qbl    %[tmp_L_5],   %[tmp5]                          \n\t"
                "preceu.ph.qbl    %[tmp_L_6],   %[tmp6]                          \n\t"
                "preceu.ph.qbl    %[tmp_L_2],   %[tmp2]                          \n\t"
                "preceu.ph.qbl    %[tmp_L_3],   %[tmp3]                          \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_4],     %[tmp_R_1]       \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_5],     %[tmp_R_6]       \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_4],     %[tmp_L_1]       \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_5],     %[tmp_L_6]       \n\t"
                "mul.ph           %[tmp6],      %[tmp6],        %[tmp_const_5]   \n\t"
                "mul.ph           %[tmp5],      %[tmp5],        %[tmp_const_20]  \n\t"
                "mul.ph           %[tmp4],      %[tmp4],        %[tmp_const_5]   \n\t"
                "mul.ph           %[tmp3],      %[tmp3],        %[tmp_const_20]  \n\t"
                "subq.ph          %[tmp6],      %[tmp5],        %[tmp6]          \n\t"
                "subq.ph          %[tmp4],      %[tmp3],        %[tmp4]          \n\t"
                "addq.ph          %[tmp2],      %[tmp_R_2],     %[tmp_R_3]       \n\t"
                "addq.ph          %[tmp1],      %[tmp_L_2],     %[tmp_L_3]       \n\t"
                "addq.ph          %[tmp2],      %[tmp2],        %[tmp6]          \n\t"
                "addq.ph          %[tmp1],      %[tmp1],        %[tmp4]          \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],        5                \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],        5                \n\t"
                "addu             %[temp_ptrC], %[ptrC],        %[width]         \n\t"
                "shll_s.ph        %[tmp2],      %[tmp2],        7                \n\t"
                "shll_s.ph        %[tmp1],      %[tmp1],        7                \n\t"
                "ulw              %[tmp3],      0(%[temp_ptrC])                  \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_3],     %[tmp_R_6]       \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_4],     %[tmp_R_5]       \n\t"
                "precrqu_s.qb.ph  %[tmp2],      %[tmp1],        %[tmp2]          \n\t"
                "preceu.ph.qbr    %[tmp_R_2],   %[tmp3]                          \n\t"
                "preceu.ph.qbl    %[tmp_L_2],   %[tmp3]                          \n\t"
                "sw               %[tmp2],      48(%[mb])                        \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_3],     %[tmp_L_6]       \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_4],     %[tmp_L_5]       \n\t"
                "mul.ph           %[tmp6],      %[tmp6],        %[tmp_const_5]   \n\t"
                "mul.ph           %[tmp5],      %[tmp5],        %[tmp_const_20]  \n\t"
                "mul.ph           %[tmp4],      %[tmp4],        %[tmp_const_5]   \n\t"
                "mul.ph           %[tmp3],      %[tmp3],        %[tmp_const_20]  \n\t"
                "subq.ph          %[tmp6],      %[tmp5],        %[tmp6]          \n\t"
                "subq.ph          %[tmp4],      %[tmp3],        %[tmp4]          \n\t"
                "addq.ph          %[tmp2],      %[tmp_R_1],     %[tmp_R_2]       \n\t"
                "addq.ph          %[tmp1],      %[tmp_L_1],     %[tmp_L_2]       \n\t"
                "addq.ph          %[tmp2],      %[tmp2],        %[tmp6]          \n\t"
                "addq.ph          %[tmp1],      %[tmp1],        %[tmp4]          \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],        5                \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],        5                \n\t"
                "ulw              %[tmp3],      0(%[ptrC])                       \n\t"
                "shll_s.ph        %[tmp2],      %[tmp2],        7                \n\t"
                "shll_s.ph        %[tmp1],      %[tmp1],        7                \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_2],     %[tmp_R_5]       \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_4],     %[tmp_R_3]       \n\t"
                "precrqu_s.qb.ph  %[tmp2],      %[tmp1],        %[tmp2]          \n\t"
                "preceu.ph.qbr    %[tmp_R_1],   %[tmp3]                          \n\t"
                "preceu.ph.qbl    %[tmp_L_1],   %[tmp3]                          \n\t"
                "sw               %[tmp2],      32(%[mb])                        \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_2],     %[tmp_L_5]       \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_4],     %[tmp_L_3]       \n\t"
                "mul.ph           %[tmp6],      %[tmp6],        %[tmp_const_5]   \n\t"
                "mul.ph           %[tmp5],      %[tmp5],        %[tmp_const_20]  \n\t"
                "mul.ph           %[tmp4],      %[tmp4],        %[tmp_const_5]   \n\t"
                "mul.ph           %[tmp3],      %[tmp3],        %[tmp_const_20]  \n\t"
                "subq.ph          %[tmp6],      %[tmp5],        %[tmp6]          \n\t"
                "subq.ph          %[tmp4],      %[tmp3],        %[tmp4]          \n\t"
                "addq.ph          %[tmp2],      %[tmp_R_6],     %[tmp_R_1]       \n\t"
                "addq.ph          %[tmp1],      %[tmp_L_6],     %[tmp_L_1]       \n\t"
                "addq.ph          %[tmp2],      %[tmp2],        %[tmp6]          \n\t"
                "addq.ph          %[tmp1],      %[tmp1],        %[tmp4]          \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],        5                \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],        5                \n\t"
                "subu             %[temp_ptrC], %[ptrC],        %[width]         \n\t"
                "shll_s.ph        %[tmp2],      %[tmp2],        7                \n\t"
                "shll_s.ph        %[tmp1],      %[tmp1],        7                \n\t"
                "ulw              %[tmp3],      0(%[temp_ptrC])                  \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_1],     %[tmp_R_4]       \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_2],     %[tmp_R_3]       \n\t"
                "precrqu_s.qb.ph  %[tmp2],      %[tmp1],        %[tmp2]          \n\t"
                "preceu.ph.qbr    %[tmp_R_6],   %[tmp3]                          \n\t"
                "preceu.ph.qbl    %[tmp_L_6],   %[tmp3]                          \n\t"
                "sw               %[tmp2],      16(%[mb])                        \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_1],     %[tmp_L_4]       \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_2],     %[tmp_L_3]       \n\t"
                "mul.ph           %[tmp6],      %[tmp6],        %[tmp_const_5]   \n\t"
                "mul.ph           %[tmp5],      %[tmp5],        %[tmp_const_20]  \n\t"
                "mul.ph           %[tmp4],      %[tmp4],        %[tmp_const_5]   \n\t"
                "mul.ph           %[tmp3],      %[tmp3],        %[tmp_const_20]  \n\t"
                "subq.ph          %[tmp6],      %[tmp5],        %[tmp6]          \n\t"
                "subq.ph          %[tmp4],      %[tmp3],        %[tmp4]          \n\t"
                "addq.ph          %[tmp2],      %[tmp_R_5],     %[tmp_R_6]       \n\t"
                "addq.ph          %[tmp1],      %[tmp_L_5],     %[tmp_L_6]       \n\t"
                "addq.ph          %[tmp2],      %[tmp2],        %[tmp6]          \n\t"
                "addq.ph          %[tmp1],      %[tmp1],        %[tmp4]          \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],        5                \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],        5                \n\t"
                "addiu            %[ptrC],      %[ptrC],        4                \n\t"
                "shll_s.ph        %[tmp2],      %[tmp2],        7                \n\t"
                "shll_s.ph        %[tmp1],      %[tmp1],        7                \n\t"
                "precrqu_s.qb.ph  %[tmp2],      %[tmp1],        %[tmp2]          \n\t"
                "sw               %[tmp2],      0(%[mb])                         \n\t"
                "addiu            %[mb],        %[mb],          4                \n\t"

                : [tmp_R_4] "=&r" (tmp_R_4), [tmp_R_1] "=&r" (tmp_R_1),
                  [tmp_R_5] "=&r" (tmp_R_5), [tmp_R_6] "=&r" (tmp_R_6),
                  [tmp_R_3] "=&r" (tmp_R_3), [tmp_R_2] "=&r" (tmp_R_2),
                  [tmp_L_4] "=&r" (tmp_L_4), [tmp_L_1] "=&r" (tmp_L_1),
                  [tmp_L_5] "=&r" (tmp_L_5), [tmp_L_6] "=&r" (tmp_L_6),
                  [tmp_L_3] "=&r" (tmp_L_3), [tmp_L_2] "=&r" (tmp_L_2),
                  [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                  [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                  [tmp2] "=&r" (tmp2), [tmp1] "=&r" (tmp1),
                  [ptrC] "+r" (ptrC), [mb] "+r" (mb),
                  [temp_ptrC] "=&r" (temp_ptrC)
                : [tmp_const_5] "r" (tmp_const_5), [width] "r" (width),
                  [tmp_const_20] "r" (tmp_const_20)
                : "hi", "lo", "memory"
            );
        }
        ptrC += 4*width - partWidth;
        mb += 4*16 - partWidth;
     }
}

/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateVerQuarter_verOffset_1

        Functional description: Function h264bsdInterpolateVerQuarter_verOffset_1
        are replacing the original function h264bsdInterpolateVerQuarter. Input
        parametar verOffset=1

------------------------------------------------------------------------------*/
void h264bsdInterpolateVerQuarter_verOffset_1(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight)    /* 0 for pixel d, 1 for pixel n */
{
    u32 p1[21*21/4+1];
    u32 i, j;
    u8 *ptrC;

    /* Code */

    ASSERT(ref);
    ASSERT(mb);

    if ((x0 < 0) || ((u32)x0+partWidth > width) ||
        (y0 < 0) || ((u32)y0+partHeight+5 > height))
    {
        h264bsdFillBlock(ref, (u8*)p1, x0, y0, width, height,
                partWidth, partHeight+5, partWidth);

        x0 = 0;
        y0 = 0;
        ref = (u8*)p1;
        width = partWidth;
    }

    ref += (u32)y0 * width + (u32)x0;

    ptrC = ref + width;

    /* 4 pixels per iteration
     * interpolate using 5 vertical samples and average between
     * interpolated value and integer sample value */
    i32 tmp_const_5, tmp_const_20;

    __asm__ volatile (
        "repl.ph        %[tmp_const_5],            5               \n\t"
        "repl.ph        %[tmp_const_20],           20              \n\t"

        : [tmp_const_5] "=&r" (tmp_const_5),
          [tmp_const_20] "=&r" (tmp_const_20)
        :
    );

    for (i = (partHeight >> 2); i; i--)
    {
        /* h1 = (16 + A + 16(G+M) + 4(G+M) - 4(C+R) - (C+R) + T) >> 5 */
        for (j = partWidth; j; j-=4)
        {
            u8 * temp_ptrC;
            i32 tmp6, tmp5, tmp4, tmp3,tmp2, tmp1;
            i32 tmp_R_4, tmp_R_1, tmp_R_5, tmp_R_6, tmp_R_3, tmp_R_2;
            i32 tmp_L_4, tmp_L_1, tmp_L_5, tmp_L_6, tmp_L_3, tmp_L_2;

            __asm__ volatile (
                "sll              %[tmp4],       %[width],      1               \n\t"
                "addu             %[temp_ptrC],  %[ptrC],       %[tmp4]         \n\t"
                "ulw              %[tmp3],       0(%[temp_ptrC])                \n\t"
                "addu             %[tmp4],       %[tmp4],       %[width]        \n\t"
                "addu             %[temp_ptrC],  %[ptrC],       %[tmp4]         \n\t"
                "ulw              %[tmp4],       0(%[temp_ptrC])                \n\t"
                "sll              %[tmp1],       %[width],      2               \n\t"
                "addu             %[temp_ptrC],  %[ptrC],       %[tmp1]         \n\t"
                "ulw              %[tmp5],       0(%[temp_ptrC])                \n\t"
                "addu             %[tmp1],       %[tmp1],       %[width]        \n\t"
                "addu             %[temp_ptrC],  %[ptrC],       %[tmp1]         \n\t"
                "ulw              %[tmp6],       0(%[temp_ptrC])                \n\t"
                "addu             %[tmp1],       %[tmp1],       %[width]        \n\t"
                "addu             %[temp_ptrC],  %[ptrC],       %[tmp1]         \n\t"
                "ulw              %[tmp1],       0(%[temp_ptrC])                \n\t"
                "sll              %[tmp2],       %[width],      3               \n\t"
                "subu             %[tmp2],       %[tmp2],       %[width]        \n\t"
                "addu             %[temp_ptrC],  %[ptrC],       %[tmp2]         \n\t"
                "ulw              %[tmp2],       0(%[temp_ptrC])                \n\t"
                "preceu.ph.qbr    %[tmp_R_4],    %[tmp4]                        \n\t"
                "preceu.ph.qbr    %[tmp_R_1],    %[tmp1]                        \n\t"
                "preceu.ph.qbr    %[tmp_R_5],    %[tmp5]                        \n\t"
                "preceu.ph.qbr    %[tmp_R_6],    %[tmp6]                        \n\t"
                "preceu.ph.qbr    %[tmp_R_2],    %[tmp2]                        \n\t"
                "preceu.ph.qbr    %[tmp_R_3],    %[tmp3]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_4],    %[tmp4]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_1],    %[tmp1]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_5],    %[tmp5]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_6],    %[tmp6]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_2],    %[tmp2]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_3],    %[tmp3]                        \n\t"
                "addq.ph          %[tmp6],       %[tmp_R_4],    %[tmp_R_1]      \n\t"
                "addq.ph          %[tmp5],       %[tmp_R_5],    %[tmp_R_6]      \n\t"
                "addq.ph          %[tmp4],       %[tmp_L_4],    %[tmp_L_1]      \n\t"
                "addq.ph          %[tmp3],       %[tmp_L_5],    %[tmp_L_6]      \n\t"
                "mul.ph           %[tmp6],       %[tmp6],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],       %[tmp5],       %[tmp_const_20] \n\t"
                "mul.ph           %[tmp4],       %[tmp4],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],       %[tmp3],       %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],       %[tmp5],       %[tmp6]         \n\t"
                "subq.ph          %[tmp4],       %[tmp3],       %[tmp4]         \n\t"
                "addq.ph          %[tmp2],       %[tmp_R_2],    %[tmp_R_3]      \n\t"
                "addq.ph          %[tmp1],       %[tmp_L_2],    %[tmp_L_3]      \n\t"
                "addq.ph          %[tmp2],       %[tmp2],       %[tmp6]         \n\t"
                "addq.ph          %[tmp1],       %[tmp1],       %[tmp4]         \n\t"
                "shra_r.ph        %[tmp2],       %[tmp2],       5               \n\t"
                "shra_r.ph        %[tmp1],       %[tmp1],       5               \n\t"
                "addu             %[temp_ptrC],  %[ptrC],       %[width]        \n\t"
                "shll_s.ph        %[tmp2],       %[tmp2],       7               \n\t"
                "shll_s.ph        %[tmp1],       %[tmp1],       7               \n\t"
                "addu             %[temp_ptrC],  %[ptrC],       %[width]        \n\t"
                "precrqu_s.qb.ph  %[tmp2],       %[tmp2],       %[tmp2]         \n\t"
                "precrqu_s.qb.ph  %[tmp1],       %[tmp1],       %[tmp1]         \n\t"
                "ulw              %[tmp3],       0(%[temp_ptrC])                \n\t"
                "preceu.ph.qbr    %[tmp2],       %[tmp2]                        \n\t"
                "preceu.ph.qbr    %[tmp1],       %[tmp1]                        \n\t"
                "addq.ph          %[tmp2],       %[tmp2],       %[tmp_R_6]      \n\t"
                "addq.ph          %[tmp1],       %[tmp1],       %[tmp_L_6]      \n\t"
                "shra_r.ph        %[tmp2],       %[tmp2],       1               \n\t"
                "shra_r.ph        %[tmp1],       %[tmp1],       1               \n\t"
                "preceu.ph.qbr    %[tmp_R_2],    %[tmp3]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_2],    %[tmp3]                        \n\t"
                "precr.qb.ph      %[tmp2],       %[tmp1],       %[tmp2]         \n\t"
                "addq.ph          %[tmp6],       %[tmp_R_3],    %[tmp_R_6]      \n\t"
                "addq.ph          %[tmp5],       %[tmp_R_4],    %[tmp_R_5]      \n\t"
                "sw               %[tmp2],       48(%[mb])                      \n\t"
                "addq.ph          %[tmp4],       %[tmp_L_3],    %[tmp_L_6]      \n\t"
                "addq.ph          %[tmp3],       %[tmp_L_4],    %[tmp_L_5]      \n\t"
                "mul.ph           %[tmp6],       %[tmp6],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],       %[tmp5],       %[tmp_const_20] \n\t"
                "mul.ph           %[tmp4],       %[tmp4],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],       %[tmp3],       %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],       %[tmp5],       %[tmp6]         \n\t"
                "subq.ph          %[tmp4],       %[tmp3],       %[tmp4]         \n\t"
                "addq.ph          %[tmp2],       %[tmp_R_1],    %[tmp_R_2]      \n\t"
                "addq.ph          %[tmp1],       %[tmp_L_1],    %[tmp_L_2]      \n\t"
                "addq.ph          %[tmp2],       %[tmp2],       %[tmp6]         \n\t"
                "addq.ph          %[tmp1],       %[tmp1],       %[tmp4]         \n\t"
                "shra_r.ph        %[tmp2],       %[tmp2],       5               \n\t"
                "shra_r.ph        %[tmp1],       %[tmp1],       5               \n\t"
                "ulw              %[tmp3],       0(%[ptrC])                     \n\t"
                "shll_s.ph        %[tmp2],       %[tmp2],       7               \n\t"
                "shll_s.ph        %[tmp1],       %[tmp1],       7               \n\t"
                "addq.ph          %[tmp6],       %[tmp_R_2],    %[tmp_R_5]      \n\t"
                "precrqu_s.qb.ph  %[tmp2],       %[tmp2],       %[tmp2]         \n\t"
                "precrqu_s.qb.ph  %[tmp1],       %[tmp1],       %[tmp1]         \n\t"
                "preceu.ph.qbR    %[tmp_R_1],    %[tmp3]                        \n\t"
                "preceu.ph.qbr    %[tmp2],       %[tmp2]                        \n\t"
                "preceu.ph.qbr    %[tmp1],       %[tmp1]                        \n\t"
                "addq.ph          %[tmp2],       %[tmp2],       %[tmp_R_5]      \n\t"
                "addq.ph          %[tmp1],       %[tmp1],       %[tmp_L_5]      \n\t"
                "shra_r.ph        %[tmp2],       %[tmp2],       1               \n\t"
                "shra_r.ph        %[tmp1],       %[tmp1],       1               \n\t"
                "preceu.ph.qbL    %[tmp_L_1],    %[tmp3]                        \n\t"
                "addq.ph          %[tmp5],       %[tmp_R_4],    %[tmp_R_3]      \n\t"
                "precr.qb.ph      %[tmp2],       %[tmp1],       %[tmp2]         \n\t"
                "addq.ph          %[tmp4],       %[tmp_L_2],    %[tmp_L_5]      \n\t"
                "sw               %[tmp2],       32(%[mb])                      \n\t"
                "addq.ph          %[tmp3],       %[tmp_L_4],    %[tmp_L_3]      \n\t"
                "mul.ph           %[tmp6],       %[tmp6],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],       %[tmp5],       %[tmp_const_20] \n\t"
                "mul.ph           %[tmp4],       %[tmp4],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],       %[tmp3],       %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],       %[tmp5],       %[tmp6]         \n\t"
                "subq.ph          %[tmp4],       %[tmp3],       %[tmp4]         \n\t"
                "addq.ph          %[tmp2],       %[tmp_R_6],    %[tmp_R_1]      \n\t"
                "addq.ph          %[tmp1],       %[tmp_L_6],    %[tmp_L_1]      \n\t"
                "addq.ph          %[tmp2],       %[tmp2],       %[tmp6]         \n\t"
                "addq.ph          %[tmp1],       %[tmp1],       %[tmp4]         \n\t"
                "shra_r.ph        %[tmp2],       %[tmp2],       5               \n\t"
                "shra_r.ph        %[tmp1],       %[tmp1],       5               \n\t"
                "sub              %[temp_ptrC],  %[ptrC],       %[width]        \n\t"
                "shll_s.ph        %[tmp2],       %[tmp2],       7               \n\t"
                "shll_s.ph        %[tmp1],       %[tmp1],       7               \n\t"
                "ulw              %[tmp3],       0(%[temp_ptrC])                \n\t"
                "precrqu_s.qb.ph  %[tmp2],       %[tmp2],       %[tmp2]         \n\t"
                "precrqu_s.qb.ph  %[tmp1],       %[tmp1],       %[tmp1]         \n\t"
                "addq.ph          %[tmp6],       %[tmp_R_1],    %[tmp_R_4]      \n\t"
                "preceu.ph.qbr    %[tmp2],       %[tmp2]                        \n\t"
                "preceu.ph.qbr    %[tmp1],       %[tmp1]                        \n\t"
                "addq.ph          %[tmp2],       %[tmp2],       %[tmp_R_4]      \n\t"
                "addq.ph          %[tmp1],       %[tmp1],       %[tmp_L_4]      \n\t"
                "shra_r.ph        %[tmp2],       %[tmp2],       1               \n\t"
                "shra_r.ph        %[tmp1],       %[tmp1],       1               \n\t"
                "preceu.ph.qbr    %[tmp_R_6],    %[tmp3]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_6],    %[tmp3]                        \n\t"
                "precr.qb.ph      %[tmp2],       %[tmp1],       %[tmp2]         \n\t"
                "addq.ph          %[tmp5],       %[tmp_R_2],    %[tmp_R_3]      \n\t"
                "addq.ph          %[tmp4],       %[tmp_L_1],    %[tmp_L_4]      \n\t"
                "sw               %[tmp2],       16(%[mb])                      \n\t"
                "addq.ph          %[tmp3],       %[tmp_L_2],    %[tmp_L_3]      \n\t"
                "mul.ph           %[tmp6],       %[tmp6],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],       %[tmp5],       %[tmp_const_20] \n\t"
                "mul.ph           %[tmp4],       %[tmp4],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],       %[tmp3],       %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],       %[tmp5],       %[tmp6]         \n\t"
                "subq.ph          %[tmp4],       %[tmp3],       %[tmp4]         \n\t"
                "addq.ph          %[tmp2],       %[tmp_R_5],    %[tmp_R_6]      \n\t"
                "addq.ph          %[tmp1],       %[tmp_L_5],    %[tmp_L_6]      \n\t"
                "addq.ph          %[tmp2],       %[tmp2],       %[tmp6]         \n\t"
                "addq.ph          %[tmp1],       %[tmp1],       %[tmp4]         \n\t"
                "shra_r.ph        %[tmp2],       %[tmp2],       5               \n\t"
                "shra_r.ph        %[tmp1],       %[tmp1],       5               \n\t"
                "addiu            %[ptrC],       %[ptrC],       4               \n\t"
                "shll_s.ph        %[tmp2],       %[tmp2],       7               \n\t"
                "shll_s.ph        %[tmp1],       %[tmp1],       7               \n\t"
                "addiu            %[mb],         %[mb],         4               \n\t"
                "precrqu_s.qb.ph  %[tmp2],       %[tmp2],       %[tmp2]         \n\t"
                "precrqu_s.qb.ph  %[tmp1],       %[tmp1],       %[tmp1]         \n\t"
                "preceu.ph.qbr    %[tmp2],       %[tmp2]                        \n\t"
                "preceu.ph.qbr    %[tmp1],       %[tmp1]                        \n\t"
                "addq.ph          %[tmp2],       %[tmp2],       %[tmp_R_3]      \n\t"
                "addq.ph          %[tmp1],       %[tmp1],       %[tmp_L_3]      \n\t"
                "shra_r.ph        %[tmp2],       %[tmp2],       1               \n\t"
                "shra_r.ph        %[tmp1],       %[tmp1],       1               \n\t"
                "precr.qb.ph      %[tmp6],       %[tmp1],       %[tmp2]         \n\t"
                "sw               %[tmp6],       -4(%[mb])                      \n\t"

                : [tmp_R_4] "=&r" (tmp_R_4), [tmp_R_1] "=&r" (tmp_R_1),
                  [tmp_R_5] "=&r" (tmp_R_5), [tmp_R_6] "=&r" (tmp_R_6),
                  [tmp_R_3] "=&r" (tmp_R_3), [tmp_R_2] "=&r" (tmp_R_2),
                  [tmp_L_4] "=&r" (tmp_L_4), [tmp_L_1] "=&r" (tmp_L_1),
                  [tmp_L_5] "=&r" (tmp_L_5), [tmp_L_6] "=&r" (tmp_L_6),
                  [tmp_L_3] "=&r" (tmp_L_3), [tmp_L_2] "=&r" (tmp_L_2),
                  [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                  [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                  [tmp2] "=&r" (tmp2), [tmp1] "=&r" (tmp1),
                  [ptrC] "+r" (ptrC), [mb] "+r" (mb),
                  [temp_ptrC] "=&r" (temp_ptrC)
                : [tmp_const_5] "r" (tmp_const_5), [tmp_const_20] "r" (tmp_const_20),
                  [width] "r" (width)
                : "hi", "lo", "memory"
            );
        }
        ptrC += 4*width - partWidth;
        mb += 4*16 - partWidth;
    }
}

/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateVerQuarter_verOffset_0

        Functional description: Function h264bsdInterpolateVerQuarter_verOffset_0
        are replacing the original function h264bsdInterpolateVerQuarter.
        Input parametar verOffset=0

------------------------------------------------------------------------------*/

void h264bsdInterpolateVerQuarter_verOffset_0(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight)    /* 0 for pixel d, 1 for pixel n */
{
    u32 p1[21*21/4+1];
    u32 i, j;
    u8 *ptrC;

    /* Code */

    ASSERT(ref);
    ASSERT(mb);

    if ((x0 < 0) || ((u32)x0+partWidth > width) ||
        (y0 < 0) || ((u32)y0+partHeight+5 > height))
    {
        h264bsdFillBlock(ref, (u8*)p1, x0, y0, width, height,
                partWidth, partHeight+5, partWidth);

        x0 = 0;
        y0 = 0;
        ref = (u8*)p1;
        width = partWidth;
    }

    ref += (u32)y0 * width + (u32)x0;

    ptrC = ref + width;

    /* 4 pixels per iteration
     * interpolate using 5 vertical samples and average between
     * interpolated value and integer sample value */
    i32 tmp_const_5, tmp_const_20;

    __asm__ volatile (
        "repl.ph        %[tmp_const_5],          5              \n\t"
        "repl.ph        %[tmp_const_20],         20             \n\t"

        : [tmp_const_5] "=&r" (tmp_const_5),
          [tmp_const_20] "=&r" (tmp_const_20)
        :
    );

    for (i = (partHeight >> 2); i; i--)
    {
        /* h1 = (16 + A + 16(G+M) + 4(G+M) - 4(C+R) - (C+R) + T) >> 5 */
        for (j = partWidth; j; j-=4)
        {
            u8 * temp_ptrC;
            i32 tmp6, tmp5, tmp4, tmp3,tmp2, tmp1;
            i32 tmp_R_4, tmp_R_1, tmp_R_5, tmp_R_6, tmp_R_3, tmp_R_2;
            i32 tmp_L_4, tmp_L_1, tmp_L_5, tmp_L_6, tmp_L_3, tmp_L_2;

            __asm__ volatile (
                "sll              %[tmp4],      %[width],      1               \n\t"
                "addu             %[temp_ptrC], %[ptrC],       %[tmp4]         \n\t"
                "ulw              %[tmp3],      0(%[temp_ptrC])                \n\t"
                "addu             %[tmp4],      %[tmp4],       %[width]        \n\t"
                "addu             %[temp_ptrC], %[ptrC],       %[tmp4]         \n\t"
                "ulw              %[tmp4],      0(%[temp_ptrC])                \n\t"
                "sll              %[tmp1],      %[width],      2               \n\t"
                "addu             %[temp_ptrC], %[ptrC],       %[tmp1]         \n\t"
                "ulw              %[tmp5],      0(%[temp_ptrC])                \n\t"
                "addu             %[tmp1],      %[tmp1],       %[width]        \n\t"
                "addu             %[temp_ptrC], %[ptrC],       %[tmp1]         \n\t"
                "ulw              %[tmp6],      0(%[temp_ptrC])                \n\t"
                "addu             %[tmp1],      %[tmp1],       %[width]        \n\t"
                "addu             %[temp_ptrC], %[ptrC],       %[tmp1]         \n\t"
                "ulw              %[tmp1],      0(%[temp_ptrC])                \n\t"
                "sll              %[tmp2],      %[width],      3               \n\t"
                "subu             %[tmp2],      %[tmp2],       %[width]        \n\t"
                "addu             %[temp_ptrC], %[ptrC],       %[tmp2]         \n\t"
                "ulw              %[tmp2],      0(%[temp_ptrC])                \n\t"
                "preceu.ph.qbr    %[tmp_R_4],   %[tmp4]                        \n\t"
                "preceu.ph.qbr    %[tmp_R_1],   %[tmp1]                        \n\t"
                "preceu.ph.qbr    %[tmp_R_5],   %[tmp5]                        \n\t"
                "preceu.ph.qbr    %[tmp_R_6],   %[tmp6]                        \n\t"
                "preceu.ph.qbr    %[tmp_R_2],   %[tmp2]                        \n\t"
                "preceu.ph.qbr    %[tmp_R_3],   %[tmp3]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_4],   %[tmp4]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_1],   %[tmp1]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_5],   %[tmp5]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_6],   %[tmp6]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_2],   %[tmp2]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_3],   %[tmp3]                        \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_4],    %[tmp_R_1]      \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_5],    %[tmp_R_6]      \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_4],    %[tmp_L_1]      \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_5],    %[tmp_L_6]      \n\t"
                "mul.ph           %[tmp6],      %[tmp6],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],      %[tmp5],       %[tmp_const_20] \n\t"
                "mul.ph           %[tmp4],      %[tmp4],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],      %[tmp3],       %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],      %[tmp5],       %[tmp6]         \n\t"
                "subq.ph          %[tmp4],      %[tmp3],       %[tmp4]         \n\t"
                "addq.ph          %[tmp2],      %[tmp_R_2],    %[tmp_R_3]      \n\t"
                "addq.ph          %[tmp1],      %[tmp_L_2],    %[tmp_L_3]      \n\t"
                "addq.ph          %[tmp2],      %[tmp2],       %[tmp6]         \n\t"
                "addq.ph          %[tmp1],      %[tmp1],       %[tmp4]         \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],       5               \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],       5               \n\t"
                "addu             %[temp_ptrC], %[ptrC],        %[width]       \n\t"
                "shll_s.ph        %[tmp2],      %[tmp2],       7               \n\t"
                "shll_s.ph        %[tmp1],      %[tmp1],       7               \n\t"
                "ulw              %[tmp3],      0(%[temp_ptrC])                \n\t"
                "precrqu_s.qb.ph  %[tmp2],      %[tmp2],       %[tmp2]         \n\t"
                "precrqu_s.qb.ph  %[tmp1],      %[tmp1],       %[tmp1]         \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_3],    %[tmp_R_6]      \n\t"
                "preceu.ph.qbr    %[tmp2],      %[tmp2]                        \n\t"
                "preceu.ph.qbr    %[tmp1],      %[tmp1]                        \n\t"
                "addq.ph          %[tmp2],      %[tmp2],       %[tmp_R_5]      \n\t"
                "addq.ph          %[tmp1],      %[tmp1],       %[tmp_L_5]      \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],       1               \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],       1               \n\t"
                "preceu.ph.qbr    %[tmp_R_2],   %[tmp3]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_2],   %[tmp3]                        \n\t"
                "precr.qb.ph      %[tmp2],      %[tmp1],       %[tmp2]         \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_4],    %[tmp_R_5]      \n\t"
                "sw               %[tmp2],      48(%[mb])                      \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_3],    %[tmp_L_6]      \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_4],    %[tmp_L_5]      \n\t"
                "mul.ph           %[tmp6],      %[tmp6],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],      %[tmp5],       %[tmp_const_20] \n\t"
                "mul.ph           %[tmp4],      %[tmp4],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],      %[tmp3],       %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],      %[tmp5],       %[tmp6]         \n\t"
                "subq.ph          %[tmp4],      %[tmp3],       %[tmp4]         \n\t"
                "addq.ph          %[tmp2],      %[tmp_R_1],    %[tmp_R_2]      \n\t"
                "addq.ph          %[tmp1],      %[tmp_L_1],    %[tmp_L_2]      \n\t"
                "addq.ph          %[tmp2],      %[tmp2],       %[tmp6]         \n\t"
                "addq.ph          %[tmp1],      %[tmp1],       %[tmp4]         \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],       5               \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],       5               \n\t"
                "ulw              %[tmp3],      0(%[ptrC])                     \n\t"
                "shll_s.ph        %[tmp2],      %[tmp2],       7               \n\t"
                "shll_s.ph        %[tmp1],      %[tmp1],       7               \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_2],    %[tmp_R_5]      \n\t"
                "precrqu_s.qb.ph  %[tmp2],      %[tmp2],       %[tmp2]         \n\t"
                "precrqu_s.qb.ph  %[tmp1],      %[tmp1],       %[tmp1]         \n\t"
                "preceu.ph.qbr    %[tmp_R_1],   %[tmp3]                        \n\t"
                "preceu.ph.qbr    %[tmp2],      %[tmp2]                        \n\t"
                "preceu.ph.qbr    %[tmp1],      %[tmp1]                        \n\t"
                "addq.ph          %[tmp2],      %[tmp2],       %[tmp_R_4]      \n\t"
                "addq.ph          %[tmp1],      %[tmp1],       %[tmp_L_4]      \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],       1               \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],       1               \n\t"
                "preceu.ph.qbl    %[tmp_L_1],   %[tmp3]                        \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_4],    %[tmp_R_3]      \n\t"
                "precr.qb.ph      %[tmp2],      %[tmp1],       %[tmp2]         \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_2],    %[tmp_L_5]      \n\t"
                "sw               %[tmp2],      32(%[mb])                      \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_4],    %[tmp_L_3]      \n\t"
                "mul.ph           %[tmp6],      %[tmp6],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],      %[tmp5],       %[tmp_const_20] \n\t"
                "mul.ph           %[tmp4],      %[tmp4],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],      %[tmp3],       %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],      %[tmp5],       %[tmp6]         \n\t"
                "subq.ph          %[tmp4],      %[tmp3],       %[tmp4]         \n\t"
                "addq.ph          %[tmp2],      %[tmp_R_6],    %[tmp_R_1]      \n\t"
                "addq.ph          %[tmp1],      %[tmp_L_6],    %[tmp_L_1]      \n\t"
                "addq.ph          %[tmp2],      %[tmp2],       %[tmp6]         \n\t"
                "addq.ph          %[tmp1],      %[tmp1],       %[tmp4]         \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],       5               \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],       5               \n\t"
                "subu             %[temp_ptrC], %[ptrC],       %[width]        \n\t"
                "shll_s.ph        %[tmp2],      %[tmp2],       7               \n\t"
                "shll_s.ph        %[tmp1],      %[tmp1],       7               \n\t"
                "ulw              %[tmp3],      0(%[temp_ptrC])                \n\t"
                "precrqu_s.qb.ph  %[tmp2],      %[tmp2],       %[tmp2]         \n\t"
                "precrqu_s.qb.ph  %[tmp1],      %[tmp1],       %[tmp1]         \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_1],    %[tmp_R_4]      \n\t"
                "preceu.ph.qbr    %[tmp2],      %[tmp2]                        \n\t"
                "preceu.ph.qbr    %[tmp1],      %[tmp1]                        \n\t"
                "addq.ph          %[tmp2],      %[tmp2],       %[tmp_R_3]      \n\t"
                "addq.ph          %[tmp1],      %[tmp1],       %[tmp_L_3]      \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],       1               \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],       1               \n\t"
                "preceu.ph.qbr    %[tmp_R_6],   %[tmp3]                        \n\t"
                "preceu.ph.qbl    %[tmp_L_6],   %[tmp3]                        \n\t"
                "precr.qb.ph      %[tmp2],      %[tmp1],       %[tmp2]         \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_2],    %[tmp_R_3]      \n\t"
                "sw               %[tmp2],      16(%[mb])                      \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_1],    %[tmp_L_4]      \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_2],    %[tmp_L_3]      \n\t"
                "mul.ph           %[tmp6],      %[tmp6],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],      %[tmp5],       %[tmp_const_20] \n\t"
                "mul.ph           %[tmp4],      %[tmp4],       %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],      %[tmp3],       %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],      %[tmp5],       %[tmp6]         \n\t"
                "subq.ph          %[tmp4],      %[tmp3],       %[tmp4]         \n\t"
                "addq.ph          %[tmp2],      %[tmp_R_5],    %[tmp_R_6]      \n\t"
                "addq.ph          %[tmp1],      %[tmp_L_5],    %[tmp_L_6]      \n\t"
                "addq.ph          %[tmp2],      %[tmp2],       %[tmp6]         \n\t"
                "addq.ph          %[tmp1],      %[tmp1],       %[tmp4]         \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],       5               \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],       5               \n\t"
                "addiu            %[ptrC],      %[ptrC],       4               \n\t"
                "shll_s.ph        %[tmp2],      %[tmp2],       7               \n\t"
                "shll_s.ph        %[tmp1],      %[tmp1],       7               \n\t"
                "addiu            %[mb],        %[mb],         4               \n\t"
                "precrqu_s.qb.ph  %[tmp2],      %[tmp2],       %[tmp2]         \n\t"
                "precrqu_s.qb.ph  %[tmp1],      %[tmp1],       %[tmp1]         \n\t"
                "preceu.ph.qbr    %[tmp2],      %[tmp2]                        \n\t"
                "preceu.ph.qbr    %[tmp1],      %[tmp1]                        \n\t"
                "addq.ph          %[tmp2],      %[tmp2],       %[tmp_R_2]      \n\t"
                "addq.ph          %[tmp1],      %[tmp1],       %[tmp_L_2]      \n\t"
                "shra_r.ph        %[tmp2],      %[tmp2],       1               \n\t"
                "shra_r.ph        %[tmp1],      %[tmp1],       1               \n\t"
                "precr.qb.ph      %[tmp6],      %[tmp1],       %[tmp2]         \n\t"
                "sw               %[tmp6],      -4(%[mb])                      \n\t"

                : [tmp_R_4] "=&r" (tmp_R_4), [tmp_R_1] "=&r" (tmp_R_1),
                  [tmp_R_5] "=&r" (tmp_R_5), [tmp_R_6] "=&r" (tmp_R_6),
                  [tmp_R_3] "=&r" (tmp_R_3), [tmp_R_2] "=&r" (tmp_R_2),
                  [tmp_L_4] "=&r" (tmp_L_4), [tmp_L_1] "=&r" (tmp_L_1),
                  [tmp_L_5] "=&r" (tmp_L_5), [tmp_L_6] "=&r" (tmp_L_6),
                  [tmp_L_3] "=&r" (tmp_L_3), [tmp_L_2] "=&r" (tmp_L_2),
                  [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                  [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                  [tmp2] "=&r" (tmp2), [tmp1] "=&r" (tmp1),
                  [ptrC] "+r" (ptrC), [mb] "+r" (mb),
                  [temp_ptrC] "=&r" (temp_ptrC)
                : [tmp_const_5] "r" (tmp_const_5), [width] "r" (width),
                  [tmp_const_20] "r" (tmp_const_20)
                : "hi", "lo", "memory"
            );
        }
        ptrC += 4*width - partWidth;
        mb += 4*16 - partWidth;
     }
}


/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateHorHalf

        Functional description:
          Function to perform horizontal interpolation of pixel position 'b'
          for a block. Overfilling is done only if needed. Reference
          image (ref) is read at correct position and the predicted part
          is written to macroblock array (mb)

------------------------------------------------------------------------------*/
void h264bsdInterpolateHorHalf(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight)
{
    u32 p1[21*21/4+1];
    u8 *ptrJ;
    u32 x, y;

    /* Code */

    ASSERT(ref);
    ASSERT(mb);
    ASSERT((partWidth&0x3) == 0);
    ASSERT((partHeight&0x3) == 0);

    if ((x0 < 0) || ((u32)x0+partWidth+5 > width) ||
        (y0 < 0) || ((u32)y0+partHeight > height))
    {
        h264bsdFillBlock(ref, (u8*)p1, x0, y0, width, height,
                partWidth+5, partHeight, partWidth+5);

        x0 = 0;
        y0 = 0;
        ref = (u8*)p1;
        width = partWidth + 5;
    }

    ref += (u32)y0 * width + (u32)x0;

    ptrJ = ref + 5;

        i32 tmp_const_5, tmp_const_20;

    __asm__ volatile (
        "repl.ph         %[tmp_const_5],           5              \n\t"
        "repl.ph         %[tmp_const_20],          20             \n\t"

        : [tmp_const_5] "=&r" (tmp_const_5),
          [tmp_const_20] "=r" (tmp_const_20)
        :
    );

    for (y = partHeight; y; y--)
    {
        i32 tmp6, tmp5;
        i32 tmp_5_4, tmp_3_2, tmp_4_3, tmp_2_1;

        __asm__ volatile (
            "ulw               %[tmp6],        -5(%[ptrJ])               \n\t"
            "ulw               %[tmp5],        -4(%[ptrJ])               \n\t"
            "preceu.ph.qbr     %[tmp_5_4],     %[tmp6]                   \n\t"
            "preceu.ph.qbl     %[tmp_3_2],     %[tmp6]                   \n\t"
            "preceu.ph.qbr     %[tmp_4_3],     %[tmp5]                   \n\t"
            "preceu.ph.qbl     %[tmp_2_1],     %[tmp5]                   \n\t"

            : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
              [tmp_5_4] "=&r" (tmp_5_4), [tmp_3_2] "=&r" (tmp_3_2),
              [tmp_4_3] "=&r" (tmp_4_3), [tmp_2_1] "=&r" (tmp_2_1)
            : [ptrJ] "r" (ptrJ)
            : "memory"
         );

        /* Horizontal interpolation, calculate 4 pels per iteration */
        for (x = (partWidth >> 2); x; x--)
        {
            i32 tmp4, tmp3;
            i32 tmp_1_0, tmp_1_2, tmp_0_1, tmp_2_3;

            __asm__ volatile (
                "ulw              %[tmp4],     -1(%[ptrJ])                  \n\t"
                "ulw              %[tmp3],     0(%[ptrJ])                   \n\t"
                "preceu.ph.qbr    %[tmp_1_0],  %[tmp4]                      \n\t"
                "preceu.ph.qbl    %[tmp_1_2],  %[tmp4]                      \n\t"
                "preceu.ph.qbr    %[tmp_0_1],  %[tmp3]                      \n\t"
                "preceu.ph.qbl    %[tmp_2_3],  %[tmp3]                      \n\t"
                "addq.ph          %[tmp6],     %[tmp_4_3],  %[tmp_1_0]      \n\t"
                "addq.ph          %[tmp5],     %[tmp_2_1],  %[tmp_1_2]      \n\t"
                "addq.ph          %[tmp4],     %[tmp_3_2],  %[tmp_2_1]      \n\t"
                "addq.ph          %[tmp3],     %[tmp_1_0],  %[tmp_0_1]      \n\t"
                "mul.ph           %[tmp6],     %[tmp6],     %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],     %[tmp5],     %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp4],     %[tmp4],     %[tmp_const_20] \n\t"
                "mul.ph           %[tmp3],     %[tmp3],     %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],     %[tmp4],     %[tmp6]         \n\t"
                "subq.ph          %[tmp5],     %[tmp3],     %[tmp5]         \n\t"
                "addq.ph          %[tmp4],     %[tmp_5_4],  %[tmp_0_1]      \n\t"
                "addq.ph          %[tmp3],     %[tmp_3_2],  %[tmp_2_3]      \n\t"
                "addq.ph          %[tmp4],     %[tmp4],     %[tmp6]         \n\t"
                "addq.ph          %[tmp3],     %[tmp3],     %[tmp5]         \n\t"
                "shra_r.ph        %[tmp4],     %[tmp4],     5               \n\t"
                "shra_r.ph        %[tmp3],     %[tmp3],     5               \n\t"
                "addi             %[ptrJ],     %[ptrJ],     4               \n\t"
                "shll_s.ph        %[tmp4],     %[tmp4],     7               \n\t"
                "shll_s.ph        %[tmp3],     %[tmp3],     7               \n\t"
                "move             %[tmp_5_4],  %[tmp_1_0]                   \n\t"
                "precrqu_s.qb.ph  %[tmp4],     %[tmp4],     %[tmp4]         \n\t"
                "precrqu_s.qb.ph  %[tmp3],     %[tmp3],     %[tmp3]         \n\t"
                "move             %[tmp_4_3],  %[tmp_0_1]                   \n\t"
                "preceu.ph.qbr    %[tmp4],     %[tmp4]                      \n\t"
                "preceu.ph.qbr    %[tmp3],     %[tmp3]                      \n\t"
                "move             %[tmp_3_2],  %[tmp_1_2]                   \n\t"
                "precr.qb.ph      %[tmp6],     %[tmp3],     %[tmp4]         \n\t"
                "move             %[tmp_2_1],  %[tmp_2_3]                   \n\t"
                "sw               %[tmp6],     0(%[mb])                     \n\t"
                "addiu            %[mb],       %[mb],       4               \n\t"

                : [tmp_1_0] "=&r" (tmp_1_0), [tmp_1_2] "=&r" (tmp_1_2),
                  [tmp_0_1] "=&r" (tmp_0_1), [tmp_2_3] "=&r" (tmp_2_3),
                  [tmp_5_4] "+r" (tmp_5_4), [tmp_3_2] "+r" (tmp_3_2),
                  [tmp_4_3] "+r" (tmp_4_3), [tmp_2_1] "+r" (tmp_2_1),
                  [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                  [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                  [mb] "+r" (mb), [ptrJ] "+r" (ptrJ)
                : [tmp_const_5] "r" (tmp_const_5), [tmp_const_20] "r" (tmp_const_20)
                : "hi", "lo", "memory"
            );
        }
        ptrJ += width - partWidth;
        mb += 16 - partWidth;
    }

}


/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateHorQuarter

        Functional description:
          Function to perform horizontal interpolation of pixel position 'a'
          or 'c' for a block. Overfilling is done only if needed. Reference
          image (ref) is read at correct position and the predicted part
          is written to macroblock array (mb)

------------------------------------------------------------------------------*/
void h264bsdInterpolateHorQuarter(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight,
  u32 horOffset) /* 0 for pixel a, 1 for pixel c */
{
    u32 p1[21*21/4+1];
    u8 *ptrJ;
    u32 x, y;

    /* Code */

    ASSERT(ref);
    ASSERT(mb);

    if ((x0 < 0) || ((u32)x0+partWidth+5 > width) ||
        (y0 < 0) || ((u32)y0+partHeight > height))
    {
        h264bsdFillBlock(ref, (u8*)p1, x0, y0, width, height,
                partWidth+5, partHeight, partWidth+5);

        x0 = 0;
        y0 = 0;
        ref = (u8*)p1;
        width = partWidth + 5;
    }

    ref += (u32)y0 * width + (u32)x0;

    ptrJ = ref + 5;

    i32 tmp_const_5, tmp_const_20;

    __asm__ volatile (
        "repl.ph        %[tmp_const_5],          5                   \n\t"
        "repl.ph        %[tmp_const_20],         20                  \n\t"

        : [tmp_const_5] "=&r" (tmp_const_5),
          [tmp_const_20] "=&r" (tmp_const_20)
        :
    );

    if (!horOffset){

        for (y = partHeight; y; y--)
        {
            i32 tmp_5_4, tmp_3_2, tmp_4_3, tmp_2_1;

            __asm__ volatile (
                "ulw               $t0,           -5(%[ptrJ])       \n\t"
                "ulw               $t1,           -4(%[ptrJ])       \n\t"
                "preceu.ph.qbr     %[tmp_5_4],    $t0               \n\t"
                "preceu.ph.qbl     %[tmp_3_2],    $t0               \n\t"
                "preceu.ph.qbr     %[tmp_4_3],    $t1               \n\t"
                "preceu.ph.qbl     %[tmp_2_1],    $t1               \n\t"

                : [tmp_5_4] "=&r" (tmp_5_4), [tmp_3_2] "=&r" (tmp_3_2),
                  [tmp_4_3] "=&r" (tmp_4_3), [tmp_2_1] "=&r" (tmp_2_1)
                : [ptrJ] "r" (ptrJ)
                : "t0", "t1", "memory"
             );

            /* calculate 4 pels per iteration */
            for (x = (partWidth >> 2); x; x--)
            {

                i32 tmp_1_0, tmp_1_2, tmp_0_1, tmp_2_3;

                __asm__ volatile (
                    "ulw              $t3,         -1(%[ptrJ])                   \n\t"
                    "ulw              $t2,         0(%[ptrJ])                    \n\t"
                    "preceu.ph.qbr    %[tmp_1_0],  $t3                           \n\t"
                    "preceu.ph.qbl    %[tmp_1_2],  $t3                           \n\t"
                    "preceu.ph.qbr    %[tmp_0_1],  $t2                           \n\t"
                    "preceu.ph.qbl    %[tmp_2_3],  $t2                           \n\t"
                    "addq.ph          $t5,         %[tmp_4_3],  %[tmp_1_0]       \n\t"
                    "addq.ph          $t4,         %[tmp_2_1],  %[tmp_1_2]       \n\t"
                    "addq.ph          $t3,         %[tmp_3_2],  %[tmp_2_1]       \n\t"
                    "addq.ph          $t2,         %[tmp_1_0],  %[tmp_0_1]       \n\t"
                    "addq.ph          $t1,         %[tmp_5_4],  %[tmp_0_1]       \n\t"
                    "addq.ph          $t0,         %[tmp_3_2],  %[tmp_2_3]       \n\t"
                    "mul.ph           $t5,         $t5,         %[tmp_const_5]   \n\t"
                    "mul.ph           $t4,         $t4,         %[tmp_const_5]   \n\t"
                    "mul.ph           $t3,         $t3,         %[tmp_const_20]  \n\t"
                    "mul.ph           $t2,         $t2,         %[tmp_const_20]  \n\t"
                    "subq.ph          $t5,         $t3,         $t5              \n\t"
                    "subq.ph          $t4,         $t2,         $t4              \n\t"
                    "addq.ph          $t1,         $t1,         $t5              \n\t"
                    "addq.ph          $t0,         $t0,         $t4              \n\t"
                    "shra_r.ph        $t1,         $t1,         5                \n\t"
                    "shra_r.ph        $t0,         $t0,         5                \n\t"
                    "shll_s.ph        $t1,         $t1,         7                \n\t"
                    "shll_s.ph        $t0,         $t0,         7                \n\t"
                    "precrqu_s.qb.ph  $t1,         $t1,         $t1              \n\t"
                    "precrqu_s.qb.ph  $t0,         $t0,         $t0              \n\t"
                    "preceu.ph.qbr    $t1,         $t1                           \n\t"
                    "preceu.ph.qbr    $t0,         $t0                           \n\t"
                    "addq.ph          $t1,         $t1,         %[tmp_3_2]       \n\t"
                    "addq.ph          $t0,         $t0,         %[tmp_1_0]       \n\t"
                    "addiu            %[ptrJ],     %[ptrJ],     4                \n\t"
                    "shra_r.ph        $t1,         $t1,         1                \n\t"
                    "shra_r.ph        $t0,         $t0,         1                \n\t"
                    "move             %[tmp_5_4],  %[tmp_1_0]                    \n\t"
                    "move             %[tmp_4_3],  %[tmp_0_1]                    \n\t"
                    "precr.qb.ph      $t5,         $t0,         $t1              \n\t"
                    "move             %[tmp_2_1],  %[tmp_2_3]                    \n\t"
                    "sw               $t5,         0(%[mb])                      \n\t"
                    "move             %[tmp_3_2],  %[tmp_1_2]                    \n\t"
                    "addiu            %[mb],       %[mb],       4                \n\t"

                    : [tmp_1_0] "=&r" (tmp_1_0), [tmp_1_2] "=&r" (tmp_1_2),
                      [tmp_0_1] "=&r" (tmp_0_1), [tmp_2_3] "=&r" (tmp_2_3),
                      [mb] "+r" (mb), [ptrJ] "+r" (ptrJ),
                      [tmp_5_4] "+r" (tmp_5_4), [tmp_3_2] "+r" (tmp_3_2),
                      [tmp_4_3] "+r" (tmp_4_3), [tmp_2_1] "+r" (tmp_2_1)
                    : [tmp_const_5] "r" (tmp_const_5), [tmp_const_20] "r" (tmp_const_20)
                    : "hi", "lo", "t0", "t1", "t2", "t3" , "t4", "t5", "memory"
                );
            }
            ptrJ += width - partWidth;
            mb += 16 - partWidth;
        }
    }
    else{
        for (y = partHeight; y; y--)
        {
            i32 tmp_5_4, tmp_3_2, tmp_4_3, tmp_2_1;

            __asm__ volatile (
                "ulw              $t0,            -5(%[ptrJ])           \n\t"
                "ulw              $t1,            -4(%[ptrJ])           \n\t"
                "preceu.ph.qbr    %[tmp_5_4],     $t0                   \n\t"
                "preceu.ph.qbl    %[tmp_3_2],     $t0                   \n\t"
                "preceu.ph.qbr    %[tmp_4_3],     $t1                   \n\t"
                "preceu.ph.qbl    %[tmp_2_1],     $t1                   \n\t"

                : [tmp_5_4] "=&r" (tmp_5_4), [tmp_3_2] "=&r" (tmp_3_2),
                  [tmp_4_3] "=&r" (tmp_4_3), [tmp_2_1] "=&r" (tmp_2_1)
                : [ptrJ] "r" (ptrJ)
                : "t0", "t1", "memory"
             );

            /* calculate 4 pels per iteration */
            for (x = (partWidth >> 2); x; x--)
            {
                i32 tmp_1_0, tmp_1_2, tmp_0_1, tmp_2_3;

                __asm__ volatile (
                    "ulw              $t3,         -1(%[ptrJ])                  \n\t"
                    "ulw              $t2,         0(%[ptrJ])                   \n\t"
                    "preceu.ph.qbr    %[tmp_1_0],  $t3                          \n\t"
                    "preceu.ph.qbl    %[tmp_1_2],  $t3                          \n\t"
                    "preceu.ph.qbr    %[tmp_0_1],  $t2                          \n\t"
                    "preceu.ph.qbl    %[tmp_2_3],  $t2                          \n\t"
                    "addq.ph          $t5,         %[tmp_4_3],  %[tmp_1_0]      \n\t"
                    "addq.ph          $t4,         %[tmp_2_1],  %[tmp_1_2]      \n\t"
                    "addq.ph          $t3,         %[tmp_3_2],  %[tmp_2_1]      \n\t"
                    "addq.ph          $t2,         %[tmp_1_0],  %[tmp_0_1]      \n\t"
                    "addq.ph          $t1,         %[tmp_5_4],  %[tmp_0_1]      \n\t"
                    "addq.ph          $t0,         %[tmp_3_2],  %[tmp_2_3]      \n\t"
                    "mul.ph           $t5,         $t5,         %[tmp_const_5]  \n\t"
                    "mul.ph           $t4,         $t4,         %[tmp_const_5]  \n\t"
                    "mul.ph           $t3,         $t3,         %[tmp_const_20] \n\t"
                    "mul.ph           $t2,         $t2,         %[tmp_const_20] \n\t"
                    "subq.ph          $t5,         $t3,         $t5             \n\t"
                    "subq.ph          $t4,         $t2,         $t4             \n\t"
                    "addq.ph          $t1,         $t1,         $t5             \n\t"
                    "addq.ph          $t0,         $t0,         $t4             \n\t"
                    "shra_r.ph        $t1,         $t1,         5               \n\t"
                    "shra_r.ph        $t0,         $t0,         5               \n\t"
                    "shll_s.ph        $t1,         $t1,         7               \n\t"
                    "shll_s.ph        $t0,         $t0,         7               \n\t"
                    "precrqu_s.qb.ph  $t1,         $t1,         $t1             \n\t"
                    "precrqu_s.qb.ph  $t0,         $t0,         $t0             \n\t"
                    "preceu.ph.qbr    $t1,         $t1                          \n\t"
                    "preceu.ph.qbr    $t0,         $t0                          \n\t"
                    "addq.ph          $t1,         $t1,         %[tmp_2_1]      \n\t"
                    "addq.ph          $t0,         $t0,         %[tmp_0_1]      \n\t"
                    "addiu            %[ptrJ],     %[ptrJ],     4               \n\t"
                    "shra_r.ph        $t1,         $t1,         1               \n\t"
                    "shra_r.ph        $t0,         $t0,         1               \n\t"
                    "move             %[tmp_5_4],  %[tmp_1_0]                   \n\t"
                    "move             %[tmp_4_3],  %[tmp_0_1]                   \n\t"
                    "precr.qb.ph      $t5,         $t0,         $t1             \n\t"
                    "move             %[tmp_2_1],  %[tmp_2_3]                   \n\t"
                    "sw               $t5,         0(%[mb])                     \n\t"
                    "move             %[tmp_3_2],  %[tmp_1_2]                   \n\t"
                    "addiu            %[mb],       %[mb],       4               \n\t"

                    : [tmp_1_0] "=&r" (tmp_1_0), [tmp_1_2] "=&r" (tmp_1_2),
                      [tmp_0_1] "=&r" (tmp_0_1), [tmp_2_3] "=&r" (tmp_2_3),
                      [mb] "+r" (mb), [ptrJ] "+r" (ptrJ),
                      [tmp_5_4] "+r" (tmp_5_4), [tmp_3_2] "+r" (tmp_3_2),
                      [tmp_4_3] "+r" (tmp_4_3), [tmp_2_1] "+r" (tmp_2_1)
                    : [tmp_const_5] "r" (tmp_const_5), [tmp_const_20] "r" (tmp_const_20)
                    : "hi", "lo", "t0", "t1", "t2", "t3", "t4", "t5", "memory"
                );
            }
            ptrJ += width - partWidth;
            mb += 16 - partWidth;
        }
    }
}


/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateHorVerQuarter

        Functional description:
          Function to perform horizontal and vertical interpolation of pixel
          position 'e', 'g', 'p' or 'r' for a block. Overfilling is done only
          if needed. Reference image (ref) is read at correct position and
          the predicted part is written to macroblock array (mb)

------------------------------------------------------------------------------*/
void h264bsdInterpolateHorVerQuarter(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight,
  u32 horVerOffset) /* 0 for pixel e, 1 for pixel g,
                       2 for pixel p, 3 for pixel r */
{
    u32 p1[21*21/4+1];
    u8 *ptrC, *ptrJ, *ptrV;
    u32 x, y;

    /* Code */

    ASSERT(ref);
    ASSERT(mb);

    if ((x0 < 0) || ((u32)x0+partWidth+5 > width) ||
        (y0 < 0) || ((u32)y0+partHeight+5 > height))
    {
        h264bsdFillBlock(ref, (u8*)p1, x0, y0, width, height,
                partWidth+5, partHeight+5, partWidth+5);

        x0 = 0;
        y0 = 0;
        ref = (u8*)p1;
        width = partWidth+5;
    }

    /* Ref points to G + (-2, -2) */
    ref += (u32)y0 * width + (u32)x0;

    /* ptrJ points to either J or Q, depending on vertical offset */
    ptrJ = ref + (((horVerOffset & 0x2) >> 1) + 2) * width + 5;

    /* ptrC points to either C or D, depending on horizontal offset */
    ptrC = ref + width + 2 + (horVerOffset & 0x1);

    i32 tmp_const_5, tmp_const_20;

    __asm__ volatile (
        "repl.ph         %[tmp_const_5],             5               \n\t"
        "repl.ph         %[tmp_const_20],            20              \n\t"

        : [tmp_const_5] "=&r" (tmp_const_5),
          [tmp_const_20] "=&r" (tmp_const_20)
        :
    );

    for (y = partHeight; y; y--)
    {
        i32 tmp6, tmp5;
        i32 tmp_5_4, tmp_3_2, tmp_4_3, tmp_2_1;

        __asm__ volatile (
            "ulw              %[tmp6],        -5(%[ptrJ])          \n\t"
            "ulw              %[tmp5],        -4(%[ptrJ])          \n\t"
            "preceu.ph.qbr    %[tmp_5_4],     %[tmp6]              \n\t"
            "preceu.ph.qbl    %[tmp_3_2],     %[tmp6]              \n\t"
            "preceu.ph.qbr    %[tmp_4_3],     %[tmp5]              \n\t"
            "preceu.ph.qbl    %[tmp_2_1],     %[tmp5]              \n\t"

            : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
              [tmp_5_4] "=&r" (tmp_5_4), [tmp_3_2] "=&r" (tmp_3_2),
              [tmp_4_3] "=&r" (tmp_4_3), [tmp_2_1] "=&r" (tmp_2_1)
            : [ptrJ] "r" (ptrJ)
            : "memory"
         );

        /* Horizontal interpolation, calculate 4 pels per iteration */
        for (x = (partWidth >> 2); x; x--)
        {
            i32 tmp4, tmp3;
            i32 tmp_1_0, tmp_1_2, tmp_0_1, tmp_2_3;

            __asm__ volatile (
                "ulw              %[tmp4],    -1(%[ptrJ])                   \n\t"
                "ulw              %[tmp3],     0(%[ptrJ])                   \n\t"
                "preceu.ph.qbr    %[tmp_1_0],  %[tmp4]                      \n\t"
                "preceu.ph.qbl    %[tmp_1_2],  %[tmp4]                      \n\t"
                "preceu.ph.qbr    %[tmp_0_1],  %[tmp3]                      \n\t"
                "preceu.ph.qbl    %[tmp_2_3],  %[tmp3]                      \n\t"
                "addq.ph          %[tmp6],     %[tmp_4_3],  %[tmp_1_0]      \n\t"
                "addq.ph          %[tmp5],     %[tmp_2_1],  %[tmp_1_2]      \n\t"
                "addq.ph          %[tmp4],     %[tmp_3_2],  %[tmp_2_1]      \n\t"
                "addq.ph          %[tmp3],     %[tmp_1_0],  %[tmp_0_1]      \n\t"
                "mul.ph           %[tmp6],     %[tmp6],     %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],     %[tmp5],     %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp4],     %[tmp4],     %[tmp_const_20] \n\t"
                "mul.ph           %[tmp3],     %[tmp3],     %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],     %[tmp4],     %[tmp6]         \n\t"
                "subq.ph          %[tmp5],     %[tmp3],     %[tmp5]         \n\t"
                "addq.ph          %[tmp4],     %[tmp_5_4],  %[tmp_0_1]      \n\t"
                "addq.ph          %[tmp3],     %[tmp_3_2],  %[tmp_2_3]      \n\t"
                "addq.ph          %[tmp4],     %[tmp4],     %[tmp6]         \n\t"
                "addq.ph          %[tmp3],     %[tmp3],     %[tmp5]         \n\t"
                "shra_r.ph        %[tmp4],     %[tmp4],     5               \n\t"
                "shra_r.ph        %[tmp3],     %[tmp3],     5               \n\t"
                "addi             %[ptrJ],     %[ptrJ],     4               \n\t"
                "shll_s.ph        %[tmp4],     %[tmp4],     7               \n\t"
                "shll_s.ph        %[tmp3],     %[tmp3],     7               \n\t"
                "move             %[tmp_5_4],  %[tmp_1_0]                   \n\t"
                "precrqu_s.qb.ph  %[tmp4],     %[tmp4],     %[tmp4]         \n\t"
                "precrqu_s.qb.ph  %[tmp3],     %[tmp3],     %[tmp3]         \n\t"
                "move             %[tmp_4_3],  %[tmp_0_1]                   \n\t"
                "preceu.ph.qbr    %[tmp4],     %[tmp4]                      \n\t"
                "preceu.ph.qbr    %[tmp3],     %[tmp3]                      \n\t"
                "move             %[tmp_3_2],  %[tmp_1_2]                   \n\t"
                "precr.qb.ph      %[tmp6],     %[tmp3],     %[tmp4]         \n\t"
                "move             %[tmp_2_1],  %[tmp_2_3]                   \n\t"
                "sw               %[tmp6],     0(%[mb])                     \n\t"
                "addiu            %[mb],       %[mb],       4               \n\t"

                : [tmp_1_0] "=&r" (tmp_1_0), [tmp_1_2] "=&r" (tmp_1_2),
                  [tmp_0_1] "=&r" (tmp_0_1), [tmp_2_3] "=&r" (tmp_2_3),
                  [tmp_5_4] "+r" (tmp_5_4), [tmp_3_2] "+r" (tmp_3_2),
                  [tmp_4_3] "+r" (tmp_4_3), [tmp_2_1] "+r" (tmp_2_1),
                  [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                  [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                  [mb] "+r" (mb), [ptrJ] "+r" (ptrJ)
                : [tmp_const_5] "r" (tmp_const_5), [tmp_const_20] "r" (tmp_const_20)
                : "hi", "lo", "memory"
            );
        }
        ptrJ += width - partWidth;
        mb += 16 - partWidth;
    }

    mb -= 16*partHeight;
    ptrV = ptrC + 5*width;

    for (y = (partHeight >> 2); y; y--)
    {
        {
            u8 *pp1, *pp2, *pp3, *pp4;
            pp1 = ptrV + 4*width;
            u8 *tmp_mb = mb + 4*16;

            __asm__ volatile (
                "addu       %[pp2], %[pp1],    %[width]           \n\t"
                "addu       %[pp3], %[pp2],    %[width]           \n\t"
                "addu       %[pp4], %[pp3],    %[width]           \n\t"
                "pref       0,      0(%[pp1])                     \n\t"     /* prefetch for load for the whole next loop that follows */
                "pref       0,      0(%[pp2])                     \n\t"
                "pref       0,      0(%[pp3])                     \n\t"
                "pref       0,      0(%[pp4])                     \n\t"
                "pref       1,      0(%[tmp_mb])                  \n\t"     /* prefetch for store for the whole next loop that follows */

                : [pp2] "=&r" (pp2), [pp3] "=r" (pp3), [pp4] "=r" (pp4)
                : [width] "r" (width), [tmp_mb] "r" (tmp_mb), [pp1] "r" (pp1)
                : "memory"
            );
        }

        /* Vertical interpolation and averaging, 4 pels per iteration */
        for (x = partWidth; x; x -= 4)
        {
            u8 *temp_ptrV;
            i32 tmp6, tmp5, tmp4, tmp3;
            i32 tmp_R_4, tmp_R_1, tmp_R_5, tmp_R_6, tmp_R_3, tmp_R_2;
            i32 tmp_L_4, tmp_L_1, tmp_L_5, tmp_L_6, tmp_L_3, tmp_L_2;
            i32 tmp_R_mb, tmp_L_mb;

            __asm__ volatile (
                ".set push                                                    \n\t"
                ".set noreorder                                               \n\t"
                "sll              %[tmp3],      %[width],     1               \n\t"
                "subu             %[tmp4],      %[ptrV],      %[tmp3]         \n\t"
                "subu             %[tmp5],      %[ptrV],      %[width]        \n\t"
                "addu             %[temp_ptrV], %[ptrV],      %[width]        \n\t"
                "ulw              %[tmp_R_6],   0(%[tmp4])                    \n\t"
                "ulw              %[tmp5],      0(%[tmp5])                    \n\t"
                "lw               %[tmp6],      48(%[mb])                     \n\t"
                "addu             %[tmp3],      %[ptrV],      %[tmp3]         \n\t"
                "preceu.ph.qbr    %[tmp_R_4],   %[tmp_R_6]                    \n\t"
                "preceu.ph.qbl    %[tmp_L_4],   %[tmp_R_6]                    \n\t"
                "ulw              %[tmp_L_6],   0(%[temp_ptrV])               \n\t"
                "preceu.ph.qbr    %[tmp_R_5],   %[tmp5]                       \n\t"
                "preceu.ph.qbl    %[tmp_L_5],   %[tmp5]                       \n\t"
                "ulw              %[tmp3],      0(%[tmp3])                    \n\t"
                "ulw              %[tmp5],      0(%[ptrV])                    \n\t"
                "preceu.ph.qbr    %[tmp_R_1],   %[tmp_L_6]                    \n\t"
                "preceu.ph.qbl    %[tmp_L_1],   %[tmp_L_6]                    \n\t"
                "subu             %[temp_ptrV], %[tmp4],      %[width]        \n\t"
                "preceu.ph.qbr    %[tmp_R_2],   %[tmp3]                       \n\t"
                "preceu.ph.qbl    %[tmp_L_2],   %[tmp3]                       \n\t"
                "preceu.ph.qbr    %[tmp_R_6],   %[tmp5]                       \n\t"
                "preceu.ph.qbl    %[tmp_L_6],   %[tmp5]                       \n\t"
                "ulw              %[tmp4],      0(%[temp_ptrV])               \n\t"
                "preceu.ph.qbr    %[tmp_R_3],   %[tmp4]                       \n\t"
                "preceu.ph.qbl    %[tmp_L_3],   %[tmp4]                       \n\t"
                "preceu.ph.qbr    %[tmp_R_mb],  %[tmp6]                       \n\t"
                "preceu.ph.qbl    %[tmp_L_mb],  %[tmp6]                       \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_4],   %[tmp_R_1]      \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_5],   %[tmp_R_6]      \n\t"
                "mul.ph           %[tmp6],      %[tmp6],      %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],      %[tmp5],      %[tmp_const_20] \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_4],   %[tmp_L_1]      \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_5],   %[tmp_L_6]      \n\t"
                "subq.ph          %[tmp6],      %[tmp5],      %[tmp6]         \n\t"
                "mul.ph           %[tmp4],      %[tmp4],      %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],      %[tmp3],      %[tmp_const_20] \n\t"
                "addq.ph          %[tmp5],      %[tmp_L_2],   %[tmp_L_3]      \n\t"
                "subq.ph          %[tmp4],      %[tmp3],      %[tmp4]         \n\t"
                "addq.ph          %[tmp3],      %[tmp_R_2],   %[tmp_R_3]      \n\t"
                "addq.ph          %[tmp5],      %[tmp5],      %[tmp4]         \n\t"
                "addq.ph          %[tmp3],      %[tmp3],      %[tmp6]         \n\t"
                "shra_r.ph        %[tmp5],      %[tmp5],      5               \n\t"
                "shra_r.ph        %[tmp3],      %[tmp3],      5               \n\t"
                "subu             %[temp_ptrV], %[temp_ptrV], %[width]        \n\t"
                "shll_s.ph        %[tmp5],      %[tmp5],      7               \n\t"
                "shll_s.ph        %[tmp3],      %[tmp3],      7               \n\t"
                "lw               %[tmp4],      32(%[mb])                     \n\t"
                "precrqu_s.qb.ph  %[tmp5],      %[tmp5],      %[tmp5]         \n\t"
                "precrqu_s.qb.ph  %[tmp3],      %[tmp3],      %[tmp3]         \n\t"
                "ulw              %[tmp6],      0(%[temp_ptrV])               \n\t"
                "preceu.ph.qbr    %[tmp5],      %[tmp5]                       \n\t"
                "preceu.ph.qbr    %[tmp3],      %[tmp3]                       \n\t"
                "addq.ph          %[tmp5],      %[tmp5],      %[tmp_L_mb]     \n\t"
                "addq.ph          %[tmp3],      %[tmp3],      %[tmp_R_mb]     \n\t"
                "shra_r.ph        %[tmp5],      %[tmp5],      1               \n\t"
                "shra_r.ph        %[tmp3],      %[tmp3],      1               \n\t"
                "preceu.ph.qbr    %[tmp_R_mb],  %[tmp4]                       \n\t"
                "preceu.ph.qbl    %[tmp_L_mb],  %[tmp4]                       \n\t"
                "precr.qb.ph      %[tmp5],      %[tmp5],      %[tmp3]         \n\t"
                "preceu.ph.qbr    %[tmp_R_2],   %[tmp6]                       \n\t"
                "preceu.ph.qbl    %[tmp_L_2],   %[tmp6]                       \n\t"
                "sw               %[tmp5],      48(%[mb])                     \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_3],   %[tmp_R_6]      \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_4],   %[tmp_R_5]      \n\t"
                "mul.ph           %[tmp6],      %[tmp6],      %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],      %[tmp5],      %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],      %[tmp5],      %[tmp6]         \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_3],   %[tmp_L_6]      \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_4],   %[tmp_L_5]      \n\t"
                "mul.ph           %[tmp4],      %[tmp4],      %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],      %[tmp3],      %[tmp_const_20] \n\t"
                "addq.ph          %[tmp5],      %[tmp_L_1],   %[tmp_L_2]      \n\t"
                "subq.ph          %[tmp4],      %[tmp3],      %[tmp4]         \n\t"
                "addq.ph          %[tmp3],      %[tmp_R_1],   %[tmp_R_2]      \n\t"
                "addq.ph          %[tmp5],      %[tmp5],      %[tmp4]         \n\t"
                "addq.ph          %[tmp3],      %[tmp3],      %[tmp6]         \n\t"
                "subu             %[temp_ptrV], %[temp_ptrV], %[width]        \n\t"
                "shra_r.ph        %[tmp5],      %[tmp5],      5               \n\t"
                "shra_r.ph        %[tmp3],      %[tmp3],      5               \n\t"
                "ulw              %[tmp4],      0(%[temp_ptrV])               \n\t"
                "shll_s.ph        %[tmp5],      %[tmp5],      7               \n\t"
                "shll_s.ph        %[tmp3],      %[tmp3],      7               \n\t"
                "lw               %[tmp6],      16(%[mb])                     \n\t"
                "precrqu_s.qb.ph  %[tmp5],      %[tmp5],      %[tmp5]         \n\t"
                "precrqu_s.qb.ph  %[tmp3],      %[tmp3],      %[tmp3]         \n\t"
                "preceu.ph.qbr    %[tmp_R_1],   %[tmp4]                       \n\t"
                "preceu.ph.qbr    %[tmp5],      %[tmp5]                       \n\t"
                "preceu.ph.qbr    %[tmp3],      %[tmp3]                       \n\t"
                "addq.ph          %[tmp5],      %[tmp5],      %[tmp_L_mb]     \n\t"
                "addq.ph          %[tmp3],      %[tmp3],      %[tmp_R_mb]     \n\t"
                "shra_r.ph        %[tmp5],      %[tmp5],      1               \n\t"
                "shra_r.ph        %[tmp3],      %[tmp3],      1               \n\t"
                "preceu.ph.qbl    %[tmp_L_mb],  %[tmp6]                       \n\t"
                "preceu.ph.qbr    %[tmp_R_mb],  %[tmp6]                       \n\t"
                "precr.qb.ph      %[tmp5],      %[tmp5],      %[tmp3]         \n\t"
                "preceu.ph.qbl    %[tmp_L_1],   %[tmp4]                       \n\t"
                "sw               %[tmp5],      32(%[mb])                     \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_2],   %[tmp_R_5]      \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_4],   %[tmp_R_3]      \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_2],   %[tmp_L_5]      \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_4],   %[tmp_L_3]      \n\t"
                "mul.ph           %[tmp6],      %[tmp6],      %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],      %[tmp5],      %[tmp_const_20] \n\t"
                "mul.ph           %[tmp4],      %[tmp4],      %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],      %[tmp3],      %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],      %[tmp5],      %[tmp6]         \n\t"
                "subq.ph          %[tmp4],      %[tmp3],      %[tmp4]         \n\t"
                "addq.ph          %[tmp3],      %[tmp_R_6],   %[tmp_R_1]      \n\t"
                "addq.ph          %[tmp5],      %[tmp_L_6],   %[tmp_L_1]      \n\t"
                "addq.ph          %[tmp3],      %[tmp3],      %[tmp6]         \n\t"
                "addq.ph          %[tmp5],      %[tmp5],      %[tmp4]         \n\t"
                "subu             %[temp_ptrV], %[temp_ptrV], %[width]        \n\t"
                "shra_r.ph        %[tmp3],      %[tmp3],      5               \n\t"
                "shra_r.ph        %[tmp5],      %[tmp5],      5               \n\t"
                "ulw              %[tmp6],      0(%[temp_ptrV])               \n\t"
                "shll_s.ph        %[tmp3],      %[tmp3],      7               \n\t"
                "shll_s.ph        %[tmp5],      %[tmp5],      7               \n\t"
                "lw               %[tmp4],      0(%[mb])                      \n\t"
                "precrqu_s.qb.ph  %[tmp3],      %[tmp3],      %[tmp3]         \n\t"
                "precrqu_s.qb.ph  %[tmp5],      %[tmp5],      %[tmp5]         \n\t"
                "preceu.ph.qbr    %[tmp_R_6],   %[tmp6]                       \n\t"
                "preceu.ph.qbl    %[tmp_L_6],   %[tmp6]                       \n\t"
                "preceu.ph.qbr    %[tmp3],      %[tmp3]                       \n\t"
                "preceu.ph.qbr    %[tmp5],      %[tmp5]                       \n\t"
                "addq.ph          %[tmp3],      %[tmp3],      %[tmp_R_mb]     \n\t"
                "addq.ph          %[tmp5],      %[tmp5],      %[tmp_L_mb]     \n\t"
                "shra_r.ph        %[tmp3],      %[tmp3],      1               \n\t"
                "shra_r.ph        %[tmp5],      %[tmp5],      1               \n\t"
                "preceu.ph.qbr    %[tmp_R_mb],  %[tmp4]                       \n\t"
                "preceu.ph.qbl    %[tmp_L_mb],  %[tmp4]                       \n\t"
                "precr.qb.ph      %[tmp5],      %[tmp5],      %[tmp3]         \n\t"
                "addq.ph          %[tmp6],      %[tmp_R_1],   %[tmp_R_4]      \n\t"
                "sw               %[tmp5],      16(%[mb])                     \n\t"
                "addq.ph          %[tmp5],      %[tmp_R_3],   %[tmp_R_2]      \n\t"
                "addq.ph          %[tmp4],      %[tmp_L_1],   %[tmp_L_4]      \n\t"
                "addq.ph          %[tmp3],      %[tmp_L_3],   %[tmp_L_2]      \n\t"
                "mul.ph           %[tmp6],      %[tmp6],      %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp5],      %[tmp5],      %[tmp_const_20] \n\t"
                "mul.ph           %[tmp4],      %[tmp4],      %[tmp_const_5]  \n\t"
                "mul.ph           %[tmp3],      %[tmp3],      %[tmp_const_20] \n\t"
                "subq.ph          %[tmp6],      %[tmp5],      %[tmp6]         \n\t"
                "subq.ph          %[tmp4],      %[tmp3],      %[tmp4]         \n\t"
                "addq.ph          %[tmp3],      %[tmp_R_5],   %[tmp_R_6]      \n\t"
                "addq.ph          %[tmp5],      %[tmp_L_5],   %[tmp_L_6]      \n\t"
                "addq.ph          %[tmp3],      %[tmp3],      %[tmp6]         \n\t"
                "addq.ph          %[tmp5],      %[tmp5],      %[tmp4]         \n\t"
                "shra_r.ph        %[tmp3],      %[tmp3],      5               \n\t"
                "shra_r.ph        %[tmp5],      %[tmp5],      5               \n\t"
                "shll_s.ph        %[tmp3],      %[tmp3],      7               \n\t"
                "shll_s.ph        %[tmp5],      %[tmp5],      7               \n\t"
                "precrqu_s.qb.ph  %[tmp3],      %[tmp3],      %[tmp3]         \n\t"
                "precrqu_s.qb.ph  %[tmp5],      %[tmp5],      %[tmp5]         \n\t"
                "preceu.ph.qbr    %[tmp3],      %[tmp3]                       \n\t"
                "preceu.ph.qbr    %[tmp5],      %[tmp5]                       \n\t"
                "addq.ph          %[tmp3],      %[tmp3],      %[tmp_R_mb]     \n\t"
                "addq.ph          %[tmp5],      %[tmp5],      %[tmp_L_mb]     \n\t"
                "shra_r.ph        %[tmp3],      %[tmp3],      1               \n\t"
                "shra_r.ph        %[tmp5],      %[tmp5],      1               \n\t"
                "precr.qb.ph      %[tmp6],      %[tmp5],      %[tmp3]         \n\t"
                "sw               %[tmp6],      0(%[mb])                      \n\t"
                "addiu           %[ptrV],      %[ptrV],       4               \n\t"
                "addiu           %[mb],        %[mb],         4               \n\t"
                ".set pop                                                     \n\t"

                : [tmp_R_4] "=&r" (tmp_R_4), [tmp_R_1] "=&r" (tmp_R_1),
                  [tmp_R_5] "=&r" (tmp_R_5), [tmp_R_6] "=&r" (tmp_R_6),
                  [tmp_R_3] "=&r" (tmp_R_3), [tmp_R_2] "=&r" (tmp_R_2),
                  [tmp_L_4] "=&r" (tmp_L_4), [tmp_L_1] "=&r" (tmp_L_1),
                  [tmp_L_5] "=&r" (tmp_L_5), [tmp_L_6] "=&r" (tmp_L_6),
                  [tmp_L_3] "=&r" (tmp_L_3), [tmp_L_2] "=&r" (tmp_L_2),
                  [tmp5] "=&r" (tmp5), [tmp6] "=&r" (tmp6), [mb] "+r" (mb),
                  [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3), [ptrV] "+r" (ptrV),
                  [tmp_R_mb] "=&r" (tmp_R_mb), [tmp_L_mb] "=&r" (tmp_L_mb),
                  [temp_ptrV] "=&r" (temp_ptrV)
                : [tmp_const_5] "r" (tmp_const_5), [tmp_const_20] "r" (tmp_const_20),
                  [width] "r" (width)
                : "hi", "lo", "memory"
            );
        }
        ptrV += 4*width - partWidth;
        mb += 4*16 - partWidth;
    }
}


/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateMidHalf

        Functional description:
          Function to perform horizontal and vertical interpolation of pixel
          position 'j' for a block. Overfilling is done only if needed.
          Reference image (ref) is read at correct position and the predicted
          part is written to macroblock array (mb)

------------------------------------------------------------------------------*/
void h264bsdInterpolateMidHalf(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight)
{
    u32 p1[21*21/4+1];
    u32 x, y;
    i32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    i16 *ptrC,*b1;
    u8  *ptrJ;
    i16 table[21*16];
    const u8 *clp = h264bsdClip + 512;

    /* Code */

    ASSERT(ref);
    ASSERT(mb);

    if ((x0 < 0) || ((u32)x0+partWidth+5 > width) ||
        (y0 < 0) || ((u32)y0+partHeight+5 > height))
    {
        h264bsdFillBlock(ref, (u8*)p1, x0, y0, width, height,
                partWidth+5, partHeight+5, partWidth+5);

        x0 = 0;
        y0 = 0;
        ref = (u8*)p1;
        width = partWidth+5;
    }

    ref += (u32)y0 * width + (u32)x0;

    b1 = table;
    ptrJ = ref + 5;

    i32 tmp_const_5, tmp_const_20;

    __asm__ volatile (
        "repl.ph         %[tmp_const_5],         5              \n\t"
        "repl.ph         %[tmp_const_20],        20             \n\t"

        : [tmp_const_5] "=&r" (tmp_const_5),
          [tmp_const_20] "=r" (tmp_const_20)
        :
    );

    /* First step: calculate intermediate values for
     * horizontal interpolation */
    for (y = partHeight + 5; y; y--)
    {
        i32 tmp_5_4, tmp_3_2, tmp_4_3, tmp_2_1;

        __asm__ volatile (
            "ulw               %[tmp6],        -5(%[ptrJ])            \n\t"
            "ulw               %[tmp5],        -4(%[ptrJ])            \n\t"
            "preceu.ph.qbr     %[tmp_5_4],     %[tmp6]                \n\t"
            "preceu.ph.qbl     %[tmp_3_2],     %[tmp6]                \n\t"
            "preceu.ph.qbr     %[tmp_4_3],     %[tmp5]                \n\t"
            "preceu.ph.qbl     %[tmp_2_1],     %[tmp5]                \n\t"

            : [tmp_4_3] "=&r" (tmp_4_3), [tmp_2_1] "=&r" (tmp_2_1),
              [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
              [tmp_5_4] "=&r" (tmp_5_4), [tmp_3_2] "=&r" (tmp_3_2)
            : [ptrJ] "r" (ptrJ)
            : "memory"
        );

        for (x = (partWidth >> 2); x; x--)
        {
            i32 tmp_1_0, tmp_1_2, tmp_0_1, tmp_2_3;

            __asm__ volatile (
                "ulw            %[tmp4],     -1(%[ptrJ])                  \n\t"
                "ulw            %[tmp3],     0(%[ptrJ])                   \n\t"
                "preceu.ph.qbr  %[tmp_1_0],  %[tmp4]                      \n\t"
                "preceu.ph.qbl  %[tmp_1_2],  %[tmp4]                      \n\t"
                "preceu.ph.qbr  %[tmp_0_1],  %[tmp3]                      \n\t"
                "preceu.ph.qbl  %[tmp_2_3],  %[tmp3]                      \n\t"
                "addq.ph        %[tmp6],     %[tmp_4_3],  %[tmp_1_0]      \n\t"
                "addq.ph        %[tmp5],     %[tmp_2_1],  %[tmp_1_2]      \n\t"
                "addq.ph        %[tmp4],     %[tmp_3_2],  %[tmp_2_1]      \n\t"
                "addq.ph        %[tmp3],     %[tmp_1_0],  %[tmp_0_1]      \n\t"
                "addq.ph        %[tmp2],     %[tmp_5_4],  %[tmp_0_1]      \n\t"
                "addq.ph        %[tmp1],     %[tmp_3_2],  %[tmp_2_3]      \n\t"
                "mul.ph         %[tmp6],     %[tmp6],     %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp5],     %[tmp5],     %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp4],     %[tmp4],     %[tmp_const_20] \n\t"
                "mul.ph         %[tmp3],     %[tmp3],     %[tmp_const_20] \n\t"
                "subq.ph        %[tmp6],     %[tmp4],     %[tmp6]         \n\t"
                "subq.ph        %[tmp5],     %[tmp3],     %[tmp5]         \n\t"
                "addq.ph        %[tmp2],     %[tmp2],     %[tmp6]         \n\t"
                "addq.ph        %[tmp1],     %[tmp1],     %[tmp5]         \n\t"
                "sw             %[tmp2],     0(%[b1])                     \n\t"
                "sw             %[tmp1],     4(%[b1])                     \n\t"
                "addiu          %[ptrJ],     %[ptrJ],     4               \n\t"
                "addiu          %[b1],       %[b1],       8               \n\t"
                "move           %[tmp_5_4],  %[tmp_1_0]                   \n\t"
                "move           %[tmp_4_3],  %[tmp_0_1]                   \n\t"
                "move           %[tmp_2_1],  %[tmp_2_3]                   \n\t"
                "move           %[tmp_3_2],  %[tmp_1_2]                   \n\t"

                : [tmp_1_0] "=&r" (tmp_1_0), [tmp_1_2] "=&r" (tmp_1_2),
                  [tmp_0_1] "=&r" (tmp_0_1), [tmp_2_3] "=&r" (tmp_2_3),
                  [tmp_5_4] "+r" (tmp_5_4), [tmp_3_2] "+r" (tmp_3_2),
                  [tmp_4_3] "+r" (tmp_4_3), [tmp_2_1] "+r" (tmp_2_1),
                  [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                  [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                  [tmp2] "=&r" (tmp2), [tmp1] "=&r" (tmp1),
                  [ptrJ] "+r" (ptrJ), [b1] "+r" (b1)
                : [tmp_const_5] "r" (tmp_const_5), [tmp_const_20] "r" (tmp_const_20)
                : "hi", "lo", "memory"
            );
        }
        ptrJ += width - partWidth;
    }

    /* Second step: calculate vertical interpolation */
    ptrC = table + partWidth;
    int temp_const_1 = 1;
    int cons_5 = 5;
    int cons_20 = 20;

    for (y = (partHeight >> 2); y; y--)
    {
        /* 4 pels per iteration */
        for (x = partWidth; x; x--)
        {
            int tmp8, tmp9, tmp10, tmp11,tmp12,tmp13;

            __asm__ volatile (
                "sll       %[tmp7],    %[partWidth],  1                \n\t"
                "subu      %[tmp9],    %[ptrC],       %[tmp7]          \n\t"
                "lh        %[tmp9],    0(%[tmp9])                      \n\t"
                "lh        %[tmp10],   0(%[ptrC])                      \n\t"
                "addu      %[tmp11],   %[ptrC],       %[tmp7]          \n\t"
                "lh        %[tmp8],    0(%[tmp11])                     \n\t"
                "addu      %[tmp11],   %[tmp11],      %[tmp7]          \n\t"
                "lh        %[tmp3],    0(%[tmp11])                     \n\t"
                "addu      %[tmp11],   %[tmp11],      %[tmp7]          \n\t"
                "lh        %[tmp4],    0(%[tmp11])                     \n\t"
                "addu      %[tmp11],   %[tmp11],      %[tmp7]          \n\t"
                "lh        %[tmp5],    0(%[tmp11])                     \n\t"
                "addu      %[tmp11],   %[tmp11],      %[tmp7]          \n\t"
                "lh        %[tmp6],    0(%[tmp11])                     \n\t"
                "addu      %[tmp11],   %[tmp11],      %[tmp7]          \n\t"
                "lh        %[tmp1],    0(%[tmp11])                     \n\t"
                "addu      %[tmp11],   %[tmp11],      %[tmp7]          \n\t"
                "lh        %[tmp2],    0(%[tmp11])                     \n\t"
                "addu      %[tmp7],    %[tmp2],       %[tmp3]          \n\t"
                "addu      %[tmp11],   %[tmp1],       %[tmp8]          \n\t"
                "addu      %[tmp12],   %[tmp6],       %[tmp10]         \n\t"
                "addu      %[tmp13],   %[tmp5],       %[tmp9]          \n\t"
                "mult      $ac0,       %[tmp7],       %[temp_const_1]  \n\t"
                "mult      $ac1,       %[tmp11],      %[temp_const_1]  \n\t"
                "mult      $ac2,       %[tmp12],      %[temp_const_1]  \n\t"
                "mult      $ac3,       %[tmp13],      %[temp_const_1]  \n\t"
                "addu      %[tmp7],    %[tmp5],       %[tmp6]          \n\t"
                "addu      %[tmp11],   %[tmp4],       %[tmp5]          \n\t"
                "addu      %[tmp12],   %[tmp4],       %[tmp3]          \n\t"
                "addu      %[tmp13],   %[tmp3],       %[tmp8]          \n\t"
                "madd      $ac0,       %[tmp7],       %[cons_20]       \n\t"
                "madd      $ac1,       %[tmp11],      %[cons_20]       \n\t"
                "madd      $ac2,       %[tmp12],      %[cons_20]       \n\t"
                "madd      $ac3,       %[tmp13],      %[cons_20]       \n\t"
                "addu      %[tmp7],    %[tmp4],       %[tmp1]          \n\t"
                "addu      %[tmp11],   %[tmp3],       %[tmp6]          \n\t"
                "addu      %[tmp12],   %[tmp8],       %[tmp5]          \n\t"
                "addu      %[tmp13],   %[tmp10],      %[tmp4]          \n\t"
                "msub      $ac0,       %[tmp7],       %[cons_5]        \n\t"
                "msub      $ac1,       %[tmp11],      %[cons_5]        \n\t"
                "msub      $ac2,       %[tmp12],      %[cons_5]        \n\t"
                "msub      $ac3,       %[tmp13],      %[cons_5]        \n\t"
                "extr_r.w  %[tmp6],    $ac0,          10               \n\t"
                "extr_r.w  %[tmp5],    $ac1,          10               \n\t"
                "extr_r.w  %[tmp4],    $ac2,          10               \n\t"
                "extr_r.w  %[tmp3],    $ac3,          10               \n\t"
                "lbux      %[tmp6],    %[tmp6](%[clp])                 \n\t"
                "lbux      %[tmp5],    %[tmp5](%[clp])                 \n\t"
                "lbux      %[tmp4],    %[tmp4](%[clp])                 \n\t"
                "lbux      %[tmp3],    %[tmp3](%[clp])                 \n\t"
                "sb        %[tmp6],    48(%[mb])                       \n\t"
                "sb        %[tmp5],    32(%[mb])                       \n\t"
                "sb        %[tmp4],    16(%[mb])                       \n\t"
                "sb        %[tmp3],    0(%[mb])                        \n\t"
                "addiu     %[ptrC],    %[ptrC],       2                \n\t"
                "addiu     %[mb],      %[mb],         1                \n\t"

                : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                  [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                  [tmp2] "=&r" (tmp2), [tmp1] "=&r" (tmp1),
                  [tmp8] "=&r" (tmp8), [tmp9] "=&r" (tmp9),
                  [ptrC] "+r" (ptrC), [mb] "+r" (mb), [tmp7] "=&r" (tmp7),
                  [tmp10] "=&r" (tmp10), [tmp11] "=&r" (tmp11),
                  [tmp12] "=&r" (tmp12), [tmp13] "=&r" (tmp13)
                : [cons_20] "r" (cons_20), [cons_5] "r" (cons_5),
                  [clp] "r" (clp), [temp_const_1] "r" (temp_const_1),
                  [partWidth] "r" (partWidth)
                : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
                  "$ac3hi", "$ac3lo", "memory"
            );
        }
        mb += 4*16 - partWidth;
        ptrC += 3*partWidth;
    }
}


/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateMidVerQuarter

        Functional description:
          Function to perform horizontal and vertical interpolation of pixel
          position 'f' or 'q' for a block. Overfilling is done only if needed.
          Reference image (ref) is read at correct position and the predicted
          part is written to macroblock array (mb)

------------------------------------------------------------------------------*/
void h264bsdInterpolateMidVerQuarter(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight,
  u32 verOffset)    /* 0 for pixel f, 1 for pixel q */
{
    u32 p1[21*21/4+1];
    u32 x, y;
    i32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    i16 *ptrC, *b1;
    u8  *ptrJ;
    i16 table[21*16];
    const u8 *clp = h264bsdClip + 512;

    /* Code */

    ASSERT(ref);
    ASSERT(mb);

    if ((x0 < 0) || ((u32)x0+partWidth+5 > width) ||
        (y0 < 0) || ((u32)y0+partHeight+5 > height))
    {
        h264bsdFillBlock(ref, (u8*)p1, x0, y0, width, height,
                partWidth+5, partHeight+5, partWidth+5);

        x0 = 0;
        y0 = 0;
        ref = (u8*)p1;
        width = partWidth+5;
    }

    ref += (u32)y0 * width + (u32)x0;

    b1 = table;
    ptrJ = ref + 5;

    i32 tmp_const_5, tmp_const_20;

    __asm__ volatile (
        "repl.ph        %[tmp_const_5],         5              \n\t"
        "repl.ph        %[tmp_const_20],        20             \n\t"

        : [tmp_const_5] "=&r" (tmp_const_5),
          [tmp_const_20] "=&r" (tmp_const_20)
        :
    );

    /* First step: calculate intermediate values for
     * horizontal interpolation */
    for (y = partHeight + 5; y; y--)
    {
        i32 tmp_5_4, tmp_3_2, tmp_4_3, tmp_2_1;
        {
            u8 *pp1;
            __asm__ volatile (
                "addu       %[pp1], %[ptrJ],    %[width]           \n\t"
                "pref       0,      0(%[pp1])                      \n\t"       /* prefetch for load for the whole loop that follows */
                "pref       0,      32(%[pp1])                     \n\t"       /* to make sure all the data is prefetched */
                "pref       1,      32(%[mb])                      \n\t"       /* prefetch for store for the whole loop that follows */

                : [pp1] "=&r" (pp1)
                : [ptrJ] "r" (ptrJ), [width] "r" (width), [mb] "r" (mb)
                : "memory"
            );
        }

        __asm__ volatile (
            "ulw              %[tmp6],       -5(%[ptrJ])              \n\t"
            "ulw              %[tmp5],       -4(%[ptrJ])              \n\t"
            "preceu.ph.qbr    %[tmp_5_4],    %[tmp6]                  \n\t"
            "preceu.ph.qbl    %[tmp_3_2],    %[tmp6]                  \n\t"
            "preceu.ph.qbr    %[tmp_4_3],    %[tmp5]                  \n\t"
            "preceu.ph.qbl    %[tmp_2_1],    %[tmp5]                  \n\t"

            : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
              [tmp_5_4] "=&r" (tmp_5_4), [tmp_3_2] "=&r" (tmp_3_2),
              [tmp_4_3] "=&r" (tmp_4_3), [tmp_2_1] "=&r" (tmp_2_1)
            : [ptrJ] "r" (ptrJ)
            : "memory"
         );

        for (x = (partWidth >> 2); x; x--)
        {
            i32 tmp_1_0, tmp_1_2, tmp_0_1, tmp_2_3;

            __asm__ volatile (
                "ulw            %[tmp4],     -1(%[ptrJ])                   \n\t"
                "ulw            %[tmp3],     0(%[ptrJ])                    \n\t"
                "preceu.ph.qbr  %[tmp_1_0],  %[tmp4]                       \n\t"
                "preceu.ph.qbl  %[tmp_1_2],  %[tmp4]                       \n\t"
                "preceu.ph.qbr  %[tmp_0_1],  %[tmp3]                       \n\t"
                "preceu.ph.qbl  %[tmp_2_3],  %[tmp3]                       \n\t"
                "addq.ph        %[tmp6],     %[tmp_4_3],   %[tmp_1_0]      \n\t"
                "addq.ph        %[tmp5],     %[tmp_2_1],   %[tmp_1_2]      \n\t"
                "addq.ph        %[tmp4],     %[tmp_3_2],   %[tmp_2_1]      \n\t"
                "addq.ph        %[tmp3],     %[tmp_1_0],   %[tmp_0_1]      \n\t"
                "addq.ph        %[tmp2],     %[tmp_5_4],   %[tmp_0_1]      \n\t"
                "addq.ph        %[tmp1],     %[tmp_3_2],   %[tmp_2_3]      \n\t"
                "mul.ph         %[tmp6],     %[tmp6],      %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp5],     %[tmp5],      %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp4],     %[tmp4],      %[tmp_const_20] \n\t"
                "mul.ph         %[tmp3],     %[tmp3],      %[tmp_const_20] \n\t"
                "subq.ph        %[tmp6],     %[tmp4],      %[tmp6]         \n\t"
                "subq.ph        %[tmp5],     %[tmp3],      %[tmp5]         \n\t"
                "addq.ph        %[tmp2],     %[tmp2],      %[tmp6]         \n\t"
                "addq.ph        %[tmp1],     %[tmp1],      %[tmp5]         \n\t"
                "sw             %[tmp2],     0(%[b1])                      \n\t"
                "sw             %[tmp1],     4(%[b1])                      \n\t"
                "addiu          %[ptrJ],     %[ptrJ],      4               \n\t"
                "addiu          %[b1],       %[b1],        8               \n\t"
                "move           %[tmp_5_4],  %[tmp_1_0]                    \n\t"
                "move           %[tmp_4_3],  %[tmp_0_1]                    \n\t"
                "move           %[tmp_2_1],  %[tmp_2_3]                    \n\t"
                "move           %[tmp_3_2],  %[tmp_1_2]                    \n\t"

                : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                  [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                  [tmp2] "=&r" (tmp2), [tmp1] "=&r" (tmp1),
                  [tmp_1_0] "=&r" (tmp_1_0), [tmp_1_2] "=&r" (tmp_1_2),
                  [tmp_0_1] "=&r" (tmp_0_1), [tmp_2_3] "=&r" (tmp_2_3),
                  [tmp_5_4] "+r" (tmp_5_4), [tmp_3_2] "+r" (tmp_3_2),
                  [tmp_4_3] "+r" (tmp_4_3), [tmp_2_1] "+r" (tmp_2_1),
                  [ptrJ] "+r" (ptrJ), [b1] "+r" (b1)
                : [tmp_const_5] "r" (tmp_const_5), [tmp_const_20] "r" (tmp_const_20)
                : "hi", "lo", "memory"
            );
        }
        ptrJ += width - partWidth;
    }

    /* Second step: calculate vertical interpolation and average */
    ptrC = table + partWidth;
    /* Pointer to integer sample position, either M or R */
    int temp_const_1 = 1;
    int cons_5 = 5;
    int cons_20 = 20;

    if (!verOffset){
        for (y = (partHeight >> 2); y; y--)
        {
            for (x = partWidth; x; x--)
            {
                int tmp8, tmp9, tmp10, tmp11, tmp12, tmp13;

                __asm__ volatile (
                    "sll         %[tmp7],    %[partWidth],  1                \n\t"
                    "subu        %[tmp9],    %[ptrC],       %[tmp7]          \n\t"
                    "lh          %[tmp10],   0(%[ptrC])                      \n\t"
                    "addu        %[tmp8],    %[ptrC],       %[tmp7]          \n\t"
                    "addu        %[tmp3],    %[tmp8],       %[tmp7]          \n\t"
                    "addu        %[tmp4],    %[tmp3],       %[tmp7]          \n\t"
                    "addu        %[tmp5],    %[tmp4],       %[tmp7]          \n\t"
                    "addu        %[tmp6],    %[tmp5],       %[tmp7]          \n\t"
                    "addu        %[tmp1],    %[tmp6],       %[tmp7]          \n\t"
                    "addu        %[tmp2],    %[tmp1],       %[tmp7]          \n\t"
                    "lh          %[tmp9],    0(%[tmp9])                      \n\t"
                    "lh          %[tmp8],    0(%[tmp8])                      \n\t"
                    "lh          %[tmp3],    0(%[tmp3])                      \n\t"
                    "lh          %[tmp2],    0(%[tmp2])                      \n\t"
                    "lh          %[tmp1],    0(%[tmp1])                      \n\t"
                    "lh          %[tmp4],    0(%[tmp4])                      \n\t"
                    "lh          %[tmp6],    0(%[tmp6])                      \n\t"
                    "lh          %[tmp5],    0(%[tmp5])                      \n\t"
                    "addu        %[tmp7],    %[tmp2],       %[tmp3]          \n\t"
                    "addu        %[tmp11],   %[tmp1],       %[tmp8]          \n\t"
                    "addu        %[tmp12],   %[tmp6],       %[tmp10]         \n\t"
                    "addu        %[tmp13],   %[tmp5],       %[tmp9]          \n\t"
                    "mult        $ac0,       %[tmp7],       %[temp_const_1]  \n\t"
                    "mult        $ac1,       %[tmp11],      %[temp_const_1]  \n\t"
                    "mult        $ac2,       %[tmp12],      %[temp_const_1]  \n\t"
                    "mult        $ac3,       %[tmp13],      %[temp_const_1]  \n\t"
                    "addu        %[tmp7],    %[tmp5],       %[tmp6]          \n\t"
                    "addu        %[tmp11],   %[tmp4],       %[tmp5]          \n\t"
                    "addu        %[tmp12],   %[tmp4],       %[tmp3]          \n\t"
                    "addu        %[tmp13],   %[tmp3],       %[tmp8]          \n\t"
                    "madd        $ac0,       %[tmp7],       %[cons_20]       \n\t"
                    "madd        $ac1,       %[tmp11],      %[cons_20]       \n\t"
                    "madd        $ac2,       %[tmp12],      %[cons_20]       \n\t"
                    "madd        $ac3,       %[tmp13],      %[cons_20]       \n\t"
                    "addu        %[tmp7],    %[tmp4],       %[tmp1]          \n\t"
                    "addu        %[tmp11],   %[tmp3],       %[tmp6]          \n\t"
                    "addu        %[tmp12],   %[tmp8],       %[tmp5]          \n\t"
                    "addu        %[tmp13],   %[tmp10],      %[tmp4]          \n\t"
                    "msub        $ac0,       %[tmp7],       %[cons_5]        \n\t"
                    "msub        $ac1,       %[tmp11],      %[cons_5]        \n\t"
                    "msub        $ac2,       %[tmp12],      %[cons_5]        \n\t"
                    "msub        $ac3,       %[tmp13],      %[cons_5]        \n\t"
                    "shra_r.w    %[tmp7],    %[tmp5],       5                \n\t"
                    "shra_r.w    %[tmp11],   %[tmp4],       5                \n\t"
                    "shra_r.w    %[tmp12],   %[tmp3],       5                \n\t"
                    "shra_r.w    %[tmp13],   %[tmp8],       5                \n\t"
                    "extr_r.w    %[tmp6],    $ac0,          10               \n\t"
                    "extr_r.w    %[tmp5],    $ac1,          10               \n\t"
                    "extr_r.w    %[tmp4],    $ac2,          10               \n\t"
                    "extr_r.w    %[tmp3],    $ac3,          10               \n\t"
                    "lbux        %[tmp7],    %[tmp7](%[clp])                 \n\t"
                    "lbux        %[tmp11],   %[tmp11](%[clp])                \n\t"
                    "lbux        %[tmp12],   %[tmp12](%[clp])                \n\t"
                    "lbux        %[tmp13],   %[tmp13](%[clp])                \n\t"
                    "lbux        %[tmp6],    %[tmp6](%[clp])                 \n\t"
                    "lbux        %[tmp5],    %[tmp5](%[clp])                 \n\t"
                    "lbux        %[tmp4],    %[tmp4](%[clp])                 \n\t"
                    "lbux        %[tmp3],    %[tmp3](%[clp])                 \n\t"
                    "addqh_r.w   %[tmp6],    %[tmp6],       %[tmp7]          \n\t"
                    "addqh_r.w   %[tmp5],    %[tmp5],       %[tmp11]         \n\t"
                    "addqh_r.w   %[tmp4],    %[tmp4],       %[tmp12]         \n\t"
                    "addqh_r.w   %[tmp3],    %[tmp3],       %[tmp13]         \n\t"
                    "sb          %[tmp6],    48(%[mb])                       \n\t"
                    "sb          %[tmp5],    32(%[mb])                       \n\t"
                    "sb          %[tmp4],    16(%[mb])                       \n\t"
                    "sb          %[tmp3],    0(%[mb])                        \n\t"
                    "addiu       %[ptrC],    %[ptrC],       2                \n\t"
                    "addiu       %[mb],      %[mb],         1                \n\t"

                    : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                      [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                      [tmp2] "=&r" (tmp2), [tmp1] "=&r" (tmp1),
                      [ptrC] "+r" (ptrC),
                      [mb] "+r" (mb), [tmp7] "=&r" (tmp7),
                      [tmp8] "=&r" (tmp8), [tmp9] "=&r" (tmp9),
                      [tmp10] "=&r" (tmp10), [tmp11] "=&r" (tmp11),
                      [tmp12] "=&r" (tmp12), [tmp13] "=&r" (tmp13)
                    : [cons_20] "r" (cons_20), [cons_5] "r" (cons_5),
                      [clp] "r" (clp), [temp_const_1] "r" (temp_const_1),
                      [partWidth] "r" (partWidth)
                    : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
                      "$ac3hi", "$ac3lo" , "memory"
                );
            }
            mb += 4*16 - partWidth;
            ptrC += 3*partWidth;
        }
    }else{
        for (y = (partHeight >> 2); y; y--)
        {
            for (x = partWidth; x; x--)
            {
                int tmp8, tmp9, tmp10, tmp11, tmp12, tmp13;

                __asm__ volatile (
                    "sll         %[tmp7],    %[partWidth],   1                \n\t"
                    "subu        %[tmp9],    %[ptrC],        %[tmp7]          \n\t"
                    "lh          %[tmp10],   0(%[ptrC])                       \n\t"
                    "addu        %[tmp8],    %[ptrC],        %[tmp7]          \n\t"
                    "addu        %[tmp3],    %[tmp8],        %[tmp7]          \n\t"
                    "addu        %[tmp4],    %[tmp3],        %[tmp7]          \n\t"
                    "addu        %[tmp5],    %[tmp4],        %[tmp7]          \n\t"
                    "addu        %[tmp6],    %[tmp5],        %[tmp7]          \n\t"
                    "addu        %[tmp1],    %[tmp6],        %[tmp7]          \n\t"
                    "addu        %[tmp2],    %[tmp1],        %[tmp7]          \n\t"
                    "lh          %[tmp9],    0(%[tmp9])                       \n\t"
                    "lh          %[tmp8],    0(%[tmp8])                       \n\t"
                    "lh          %[tmp3],    0(%[tmp3])                       \n\t"
                    "lh          %[tmp2],    0(%[tmp2])                       \n\t"
                    "lh          %[tmp1],    0(%[tmp1])                       \n\t"
                    "lh          %[tmp4],    0(%[tmp4])                       \n\t"
                    "lh          %[tmp6],    0(%[tmp6])                       \n\t"
                    "lh          %[tmp5],    0(%[tmp5])                       \n\t"
                    "addu        %[tmp7],    %[tmp2],        %[tmp3]          \n\t"
                    "addu        %[tmp11],   %[tmp1],        %[tmp8]          \n\t"
                    "addu        %[tmp12],   %[tmp6],        %[tmp10]         \n\t"
                    "addu        %[tmp13],   %[tmp5],        %[tmp9]          \n\t"
                    "mult        $ac0,       %[tmp7],        %[temp_const_1]  \n\t"
                    "mult        $ac1,       %[tmp11],       %[temp_const_1]  \n\t"
                    "mult        $ac2,       %[tmp12],       %[temp_const_1]  \n\t"
                    "mult        $ac3,       %[tmp13],       %[temp_const_1]  \n\t"
                    "addu        %[tmp7],    %[tmp5],        %[tmp6]          \n\t"
                    "addu        %[tmp11],   %[tmp4],        %[tmp5]          \n\t"
                    "addu        %[tmp12],   %[tmp4],        %[tmp3]          \n\t"
                    "addu        %[tmp13],   %[tmp3],        %[tmp8]          \n\t"
                    "madd        $ac0,       %[tmp7],        %[cons_20]       \n\t"
                    "madd        $ac1,       %[tmp11],       %[cons_20]       \n\t"
                    "madd        $ac2,       %[tmp12],       %[cons_20]       \n\t"
                    "madd        $ac3,       %[tmp13],       %[cons_20]       \n\t"
                    "addu        %[tmp7],    %[tmp4],        %[tmp1]          \n\t"
                    "addu        %[tmp11],   %[tmp3],        %[tmp6]          \n\t"
                    "addu        %[tmp12],   %[tmp8],        %[tmp5]          \n\t"
                    "addu        %[tmp13],   %[tmp10],       %[tmp4]          \n\t"
                    "msub        $ac0,       %[tmp7],        %[cons_5]        \n\t"
                    "msub        $ac1,       %[tmp11],       %[cons_5]        \n\t"
                    "msub        $ac2,       %[tmp12],       %[cons_5]        \n\t"
                    "msub        $ac3,       %[tmp13],       %[cons_5]        \n\t"
                    "shra_r.w    %[tmp7],    %[tmp6],        5                \n\t"
                    "shra_r.w    %[tmp11],   %[tmp5],        5                \n\t"
                    "shra_r.w    %[tmp12],   %[tmp4],        5                \n\t"
                    "shra_r.w    %[tmp13],   %[tmp3],        5                \n\t"
                    "extr_r.w    %[tmp6],    $ac0,           10               \n\t"
                    "extr_r.w    %[tmp5],    $ac1,           10               \n\t"
                    "extr_r.w    %[tmp4],    $ac2,           10               \n\t"
                    "extr_r.w    %[tmp3],    $ac3,           10               \n\t"
                    "lbux        %[tmp7],    %[tmp7](%[clp])                  \n\t"
                    "lbux        %[tmp11],   %[tmp11](%[clp])                 \n\t"
                    "lbux        %[tmp12],   %[tmp12](%[clp])                 \n\t"
                    "lbux        %[tmp13],   %[tmp13](%[clp])                 \n\t"
                    "lbux        %[tmp6],    %[tmp6](%[clp])                  \n\t"
                    "lbux        %[tmp5],    %[tmp5](%[clp])                  \n\t"
                    "lbux        %[tmp4],    %[tmp4](%[clp])                  \n\t"
                    "lbux        %[tmp3],    %[tmp3](%[clp])                  \n\t"
                    "addqh_r.w   %[tmp6],    %[tmp6],        %[tmp7]          \n\t"
                    "addqh_r.w   %[tmp5],    %[tmp5],        %[tmp11]         \n\t"
                    "addqh_r.w   %[tmp4],    %[tmp4],        %[tmp12]         \n\t"
                    "addqh_r.w   %[tmp3],    %[tmp3],        %[tmp13]         \n\t"
                    "sb          %[tmp6],    48(%[mb])                        \n\t"
                    "sb          %[tmp5],    32(%[mb])                        \n\t"
                    "sb          %[tmp4],    16(%[mb])                        \n\t"
                    "sb          %[tmp3],    0(%[mb])                         \n\t"
                    "addiu       %[ptrC],    %[ptrC],        2                \n\t"
                    "addiu       %[mb],      %[mb],          1                \n\t"

                    : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                      [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                      [tmp2] "=&r" (tmp2), [tmp1] "=&r" (tmp1),
                      [ptrC] "+r" (ptrC),
                      [mb] "+r" (mb),[tmp7] "=&r" (tmp7),
                      [tmp8] "=&r" (tmp8), [tmp9] "=&r" (tmp9),
                      [tmp10] "=&r" (tmp10), [tmp11] "=&r" (tmp11),
                      [tmp12] "=&r" (tmp12), [tmp13] "=&r" (tmp13)
                    : [cons_20] "r" (cons_20), [cons_5] "r" (cons_5),
                      [clp] "r" (clp), [temp_const_1] "r" (temp_const_1),
                      [partWidth] "r" (partWidth)
                    : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
                      "$ac3hi","$ac3lo", "memory"
                );
            }
            mb += 4*16 - partWidth;
            ptrC += 3*partWidth;
        }
    }
}

/*------------------------------------------------------------------------------

    Function: h264bsdInterpolateMidHorQuarter

        Functional description:
          Function to perform horizontal and vertical interpolation of pixel
          position 'i' or 'k' for a block. Overfilling is done only if needed.
          Reference image (ref) is read at correct position and the predicted
          part is written to macroblock array (mb)

------------------------------------------------------------------------------*/
void h264bsdInterpolateMidHorQuarter(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight,
  u32 horOffset)    /* 0 for pixel i, 1 for pixel k */
{
    u32 p1[21*21/4+1];
    u32 x, y;
    i32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    i16 *ptrJ;
    u8  *ptrC;
    i16 table[21*16];
    i16 *h1;
    i16 tableWidth = (i16)partWidth+5;
    const u8 *clp = h264bsdClip + 512;


    /* Code */

    ASSERT(ref);
    ASSERT(mb);

    if ((x0 < 0) || ((u32)x0+partWidth+5 > width) ||
        (y0 < 0) || ((u32)y0+partHeight+5 > height))
    {
        h264bsdFillBlock(ref, (u8*)p1, x0, y0, width, height,
                partWidth+5, partHeight+5, partWidth+5);

        x0 = 0;
        y0 = 0;
        ref = (u8*)p1;
        width = partWidth+5;
    }

    ref += (u32)y0 * width + (u32)x0;

    h1 = table + tableWidth;
    ptrC = ref + width;

    i32 tmp_const_5, tmp_const_20;

    __asm__ volatile (
        "repl.ph        %[tmp_const_5],            5           \n\t"
        "repl.ph        %[tmp_const_20],           20          \n\t"

        : [tmp_const_5] "=&r" (tmp_const_5),
          [tmp_const_20] "=&r" (tmp_const_20)
        :
    );

    /* First step: calculate intermediate values for
     * vertical interpolation */
    int cons_20 =20;
    int cons_5 =5;
    int temp_const_1 = 1;

    for (y = (partHeight >> 2); y; y--)
    {
        int tmp8, tmp9, tmp10, tmp11,tmp12,tmp13,tmp14;

        __asm__ volatile (
            "subu       %[tmp9],     %[ptrC],        %[width]         \n\t"
            "lbu        %[tmp10],    0(%[ptrC])                       \n\t"
            "addu       %[tmp8],     %[ptrC],        %[width]         \n\t"
            "lbu        %[tmp9],     0(%[tmp9])                       \n\t"
            "addu       %[tmp3],     %[tmp8],        %[width]         \n\t"
            "addu       %[tmp4],     %[tmp3],        %[width]         \n\t"
            "lbu        %[tmp8],     0(%[tmp8])                       \n\t"
            "lbu        %[tmp3],     0(%[tmp3])                       \n\t"
            "addu       %[tmp5],     %[tmp4],        %[width]         \n\t"
            "lbu        %[tmp4],     0(%[tmp4])                       \n\t"
            "addu       %[tmp6],     %[tmp5],        %[width]         \n\t"
            "addu       %[tmp1],     %[tmp6],        %[width]         \n\t"
            "addu       %[tmp2],     %[tmp1],        %[width]         \n\t"
            "lbu        %[tmp5],     0(%[tmp5])                       \n\t"
            "lbu        %[tmp1],     0(%[tmp1])                       \n\t"
            "lbu        %[tmp2],     0(%[tmp2])                       \n\t"
            "lbu        %[tmp6],     0(%[tmp6])                       \n\t"
            "addu       %[tmp13],    %[tmp5],        %[tmp9]          \n\t"
            "addu       %[tmp7],     %[tmp2],        %[tmp3]          \n\t"
            "addu       %[tmp11],    %[tmp1],        %[tmp8]          \n\t"
            "addu       %[tmp12],    %[tmp6],        %[tmp10]         \n\t"
            "mult       $ac0,        %[tmp7],        %[temp_const_1]  \n\t"
            "mult       $ac1,        %[tmp11],       %[temp_const_1]  \n\t"
            "mult       $ac2,        %[tmp12],       %[temp_const_1]  \n\t"
            "mult       $ac3,        %[tmp13],       %[temp_const_1]  \n\t"
            "addu       %[tmp7],     %[tmp5],        %[tmp6]          \n\t"
            "addu       %[tmp11],    %[tmp4],        %[tmp5]          \n\t"
            "addu       %[tmp12],    %[tmp4],        %[tmp3]          \n\t"
            "addu       %[tmp13],    %[tmp3],        %[tmp8]          \n\t"
            "madd       $ac0,        %[tmp7],        %[cons_20]       \n\t"
            "madd       $ac1,        %[tmp11],       %[cons_20]       \n\t"
            "madd       $ac2,        %[tmp12],       %[cons_20]       \n\t"
            "madd       $ac3,        %[tmp13],       %[cons_20]       \n\t"
            "addu       %[tmp7],     %[tmp4],        %[tmp1]          \n\t"
            "addu       %[tmp11],    %[tmp3],        %[tmp6]          \n\t"
            "addu       %[tmp12],    %[tmp8],        %[tmp5]          \n\t"
            "addu       %[tmp13],    %[tmp10],       %[tmp4]          \n\t"
            "msub       $ac0,        %[tmp7],        %[cons_5]        \n\t"
            "msub       $ac1,        %[tmp11],       %[cons_5]        \n\t"
            "msub       $ac2,        %[tmp12],       %[cons_5]        \n\t"
            "msub       $ac3,        %[tmp13],       %[cons_5]        \n\t"
            "mflo       %[tmp6],     $ac0                             \n\t"
            "mflo       %[tmp5],     $ac1                             \n\t"
            "mflo       %[tmp4],     $ac2                             \n\t"
            "mflo       %[tmp3],     $ac3                             \n\t"
            "sll        %[tmp7],     %[tableWidth],  2                \n\t"
            "sll        %[tmp11],    %[tableWidth],  1                \n\t"
            "sll        %[tmp12],    %[tableWidth],  1                \n\t"
            "addu       %[tmp7],     %[h1],          %[tmp7]          \n\t"
            "addu       %[tmp11],    %[h1],          %[tmp11]         \n\t"
            "subu       %[tmp12],    %[h1],          %[tmp12]         \n\t"
            "sh         %[tmp4],     0(%[h1])                         \n\t"
            "sh         %[tmp6],     0(%[tmp7])                       \n\t"
            "sh         %[tmp5],     0(%[tmp11])                      \n\t"
            "sh         %[tmp3],     0(%[tmp12])                      \n\t"
            "addiu      %[ptrC],     %[ptrC],        1                \n\t"
            "addiu      %[h1],       %[h1],          2                \n\t"

            : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
              [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
              [tmp2] "=&r" (tmp2), [tmp1] "=&r" (tmp1),
              [ptrC] "+r" (ptrC), [h1] "+r" (h1),
              [tmp7] "=&r" (tmp7),
              [tmp8] "=&r" (tmp8), [tmp9] "=&r" (tmp9),
              [tmp10] "=&r" (tmp10), [tmp11] "=&r" (tmp11),
              [tmp12] "=&r" (tmp12), [tmp13] "=&r" (tmp13), [tmp14] "=&r" (tmp14)
            : [cons_20] "r" (cons_20), [cons_5] "r" (cons_5),
              [width] "r" (width), [tableWidth] "r" (tableWidth),
              [temp_const_1] "r" (temp_const_1)
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
              "$ac3hi", "$ac3lo", "memory"
        );

        for (x = (u32)tableWidth-1; x; x-=4)
        {

            i32 temp_ptrC;
            i32 tmp_R_4, tmp_R_1, tmp_R_5, tmp_R_6, tmp_R_3, tmp_R_2;
            i32 tmp_L_4, tmp_L_1, tmp_L_5, tmp_L_6, tmp_L_3, tmp_L_2;

            __asm__ volatile (
                "sll            %[tmp4],      %[width],        1               \n\t"
                "addu           %[tmp3],      %[ptrC],         %[tmp4]         \n\t"
                "addu           %[tmp4],      %[tmp3],         %[width]        \n\t"
                "addu           %[tmp5],      %[tmp4],         %[width]        \n\t"
                "ulw            %[tmp3],      0(%[tmp3])                       \n\t"
                "ulw            %[tmp4],      0(%[tmp4])                       \n\t"
                "addu           %[tmp6],      %[tmp5],         %[width]        \n\t"
                "addu           %[tmp1],      %[tmp6],         %[width]        \n\t"
                "addu           %[tmp2],      %[tmp1],         %[width]        \n\t"
                "ulw            %[tmp5],      0(%[tmp5])                       \n\t"
                "ulw            %[tmp6],      0(%[tmp6])                       \n\t"
                "ulw            %[tmp1],      0(%[tmp1])                       \n\t"
                "ulw            %[tmp2],      0(%[tmp2])                       \n\t"
                "preceu.ph.qbr  %[tmp_R_4],   %[tmp4]                          \n\t"
                "preceu.ph.qbr  %[tmp_R_1],   %[tmp1]                          \n\t"
                "preceu.ph.qbr  %[tmp_R_5],   %[tmp5]                          \n\t"
                "preceu.ph.qbr  %[tmp_R_6],   %[tmp6]                          \n\t"
                "preceu.ph.qbr  %[tmp_R_2],   %[tmp2]                          \n\t"
                "preceu.ph.qbr  %[tmp_R_3],   %[tmp3]                          \n\t"
                "preceu.ph.qbl  %[tmp_L_4],   %[tmp4]                          \n\t"
                "preceu.ph.qbl  %[tmp_L_1],   %[tmp1]                          \n\t"
                "preceu.ph.qbl  %[tmp_L_5],   %[tmp5]                          \n\t"
                "preceu.ph.qbl  %[tmp_L_6],   %[tmp6]                          \n\t"
                "preceu.ph.qbl  %[tmp_L_2],   %[tmp2]                          \n\t"
                "preceu.ph.qbl  %[tmp_L_3],   %[tmp3]                          \n\t"
                "addq.ph        %[tmp6],      %[tmp_R_4],      %[tmp_R_1]      \n\t"
                "addq.ph        %[tmp5],      %[tmp_R_5],      %[tmp_R_6]      \n\t"
                "addq.ph        %[tmp4],      %[tmp_L_4],      %[tmp_L_1]      \n\t"
                "addq.ph        %[tmp3],      %[tmp_L_5],      %[tmp_L_6]      \n\t"
                "mul.ph         %[tmp6],      %[tmp6],         %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp5],      %[tmp5],         %[tmp_const_20] \n\t"
                "mul.ph         %[tmp4],      %[tmp4],         %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp3],      %[tmp3],         %[tmp_const_20] \n\t"
                "subq.ph        %[tmp6],      %[tmp5],         %[tmp6]         \n\t"
                "subq.ph        %[tmp4],      %[tmp3],         %[tmp4]         \n\t"
                "addq.ph        %[tmp2],      %[tmp_R_2],      %[tmp_R_3]      \n\t"
                "addq.ph        %[tmp1],      %[tmp_L_2],      %[tmp_L_3]      \n\t"
                "addq.ph        %[tmp2],      %[tmp2],         %[tmp6]         \n\t"
                "addq.ph        %[tmp1],      %[tmp1],         %[tmp4]         \n\t"
                "sll            %[temp_ptrC], %[tableWidth],   2               \n\t"
                "addu           %[temp_ptrC], %[h1],           %[temp_ptrC]    \n\t"
                "usw            %[tmp2],      0(%[temp_ptrC])                  \n\t"
                "usw            %[tmp1],      4(%[temp_ptrC])                  \n\t"
                "addu           %[temp_ptrC], %[ptrC],         %[width]        \n\t"
                "ulw            %[tmp3],      0(%[temp_ptrC])                  \n\t"
                "addq.ph        %[tmp6],      %[tmp_R_3],      %[tmp_R_6]      \n\t"
                "addq.ph        %[tmp5],      %[tmp_R_4],      %[tmp_R_5]      \n\t"
                "addq.ph        %[tmp4],      %[tmp_L_3],      %[tmp_L_6]      \n\t"
                "preceu.ph.qbr  %[tmp_R_2],   %[tmp3]                          \n\t"
                "preceu.ph.qbl  %[tmp_L_2],   %[tmp3]                          \n\t"
                "addq.ph        %[tmp3],      %[tmp_L_4],      %[tmp_L_5]      \n\t"
                "mul.ph         %[tmp6],      %[tmp6],         %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp5],      %[tmp5],         %[tmp_const_20] \n\t"
                "mul.ph         %[tmp4],      %[tmp4],         %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp3],      %[tmp3],         %[tmp_const_20] \n\t"
                "subq.ph        %[tmp6],      %[tmp5],         %[tmp6]         \n\t"
                "subq.ph        %[tmp4],      %[tmp3],         %[tmp4]         \n\t"
                "addq.ph        %[tmp2],      %[tmp_R_1],      %[tmp_R_2]      \n\t"
                "addq.ph        %[tmp1],      %[tmp_L_1],      %[tmp_L_2]      \n\t"
                "sll            %[temp_ptrC], %[tableWidth],   1               \n\t"
                "addu           %[temp_ptrC], %[h1],           %[temp_ptrC]    \n\t"
                "addq.ph        %[tmp2],      %[tmp2],         %[tmp6]         \n\t"
                "addq.ph        %[tmp1],      %[tmp1],         %[tmp4]         \n\t"
                "ulw            %[tmp3],      0(%[ptrC])                       \n\t"
                "usw            %[tmp2],      0(%[temp_ptrC])                  \n\t"
                "usw            %[tmp1],      4(%[temp_ptrC])                  \n\t"
                "addq.ph        %[tmp6],      %[tmp_R_2],      %[tmp_R_5]      \n\t"
                "addq.ph        %[tmp5],      %[tmp_R_4],      %[tmp_R_3]      \n\t"
                "addq.ph        %[tmp4],      %[tmp_L_2],      %[tmp_L_5]      \n\t"
                "preceu.ph.qbr  %[tmp_R_1],   %[tmp3]                          \n\t"
                "preceu.ph.qbl  %[tmp_L_1],   %[tmp3]                          \n\t"
                "addq.ph        %[tmp3],      %[tmp_L_4],      %[tmp_L_3]      \n\t"
                "mul.ph         %[tmp6],      %[tmp6],         %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp5],      %[tmp5],         %[tmp_const_20] \n\t"
                "mul.ph         %[tmp4],      %[tmp4],         %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp3],      %[tmp3],         %[tmp_const_20] \n\t"
                "subq.ph        %[tmp6],      %[tmp5],         %[tmp6]         \n\t"
                "subq.ph        %[tmp4],      %[tmp3],         %[tmp4]         \n\t"
                "addq.ph        %[tmp2],      %[tmp_R_6],      %[tmp_R_1]      \n\t"
                "addq.ph        %[tmp1],      %[tmp_L_6],      %[tmp_L_1]      \n\t"
                "addq.ph        %[tmp2],      %[tmp2],         %[tmp6]         \n\t"
                "addq.ph        %[tmp1],      %[tmp1],         %[tmp4]         \n\t"
                "subu           %[temp_ptrC], %[ptrC],         %[width]        \n\t"
                "usw            %[tmp2],      0(%[h1])                         \n\t"
                "usw            %[tmp1],      4(%[h1])                         \n\t"
                "ulw            %[tmp3],      0(%[temp_ptrC])                  \n\t"
                "addq.ph        %[tmp6],      %[tmp_R_1],      %[tmp_R_4]      \n\t"
                "addq.ph        %[tmp5],      %[tmp_R_2],      %[tmp_R_3]      \n\t"
                "addq.ph        %[tmp4],      %[tmp_L_1],      %[tmp_L_4]      \n\t"
                "preceu.ph.qbr  %[tmp_R_6],   %[tmp3]                          \n\t"
                "preceu.ph.qbl  %[tmp_L_6],   %[tmp3]                          \n\t"
                "addq.ph        %[tmp3],      %[tmp_L_2],      %[tmp_L_3]      \n\t"
                "mul.ph         %[tmp6],      %[tmp6],         %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp5],      %[tmp5],         %[tmp_const_20] \n\t"
                "mul.ph         %[tmp4],      %[tmp4],         %[tmp_const_5]  \n\t"
                "mul.ph         %[tmp3],      %[tmp3],         %[tmp_const_20] \n\t"
                "subq.ph        %[tmp6],      %[tmp5],         %[tmp6]         \n\t"
                "subq.ph        %[tmp4],      %[tmp3],         %[tmp4]         \n\t"
                "addq.ph        %[tmp2],      %[tmp_R_5],      %[tmp_R_6]      \n\t"
                "addq.ph        %[tmp1],      %[tmp_L_5],      %[tmp_L_6]      \n\t"
                "sll            %[temp_ptrC], %[tableWidth],   1               \n\t"
                "subu           %[temp_ptrC], %[h1],           %[temp_ptrC]    \n\t"
                "addq.ph        %[tmp2],      %[tmp2],         %[tmp6]         \n\t"
                "addq.ph        %[tmp1],      %[tmp1],         %[tmp4]         \n\t"
                "usw            %[tmp2],      0(%[temp_ptrC])                  \n\t"
                "usw            %[tmp1],      4(%[temp_ptrC])                  \n\t"
                "addiu          %[ptrC],      %[ptrC],         4               \n\t"
                "addiu          %[h1],        %[h1],           8               \n\t"

                : [tmp6] "=&r" (tmp6), [tmp5] "=&r" (tmp5),
                  [tmp4] "=&r" (tmp4), [tmp3] "=&r" (tmp3),
                  [tmp2] "=&r" (tmp2), [tmp1] "=&r" (tmp1),
                  [tmp_R_4] "=&r" (tmp_R_4), [tmp_R_1] "=&r" (tmp_R_1),
                  [tmp_R_5] "=&r" (tmp_R_5), [tmp_R_6] "=&r" (tmp_R_6),
                  [tmp_R_3] "=&r" (tmp_R_3), [tmp_R_2] "=&r" (tmp_R_2),
                  [tmp_L_4] "=&r" (tmp_L_4), [tmp_L_1] "=&r" (tmp_L_1),
                  [tmp_L_5] "=&r" (tmp_L_5), [tmp_L_6] "=&r" (tmp_L_6),
                  [tmp_L_3] "=&r" (tmp_L_3), [tmp_L_2] "=&r" (tmp_L_2),
                  [ptrC] "+r" (ptrC), [h1] "+r" (h1), [temp_ptrC] "=&r" (temp_ptrC)
                : [width] "r" (width), [tableWidth] "r" (tableWidth),
                  [tmp_const_5] "r" (tmp_const_5), [tmp_const_20] "r" (tmp_const_20)
                : "hi", "lo", "memory"
            );
        }
        ptrC += 4*width - partWidth - 5;
        h1 += 3*tableWidth;
    }

    /* Second step: calculate horizontal interpolation and average */
    ptrJ = table + 5;

    if (!horOffset){

        for (y = partHeight; y; y--)
        {
            tmp6 = *(ptrJ - 5);
            tmp5 = *(ptrJ - 4);
            tmp4 = *(ptrJ - 3);
            tmp3 = *(ptrJ - 2);
            tmp2 = *(ptrJ - 1);

            for (x = (partWidth>>2); x; x--)
            {
                int tmp8, tmp9, tmp10, tmp11, tmp12, tmp13;

                __asm__ volatile (
                    "lh         %[tmp1],    0(%[ptrJ])                      \n\t"
                    "lh         %[tmp8],    2(%[ptrJ])                      \n\t"
                    "lh         %[tmp9],    4(%[ptrJ])                      \n\t"
                    "lh         %[tmp10],   6(%[ptrJ])                      \n\t"
                    "addu       %[tmp7],    %[tmp6],       %[tmp1]          \n\t"
                    "addu       %[tmp11],   %[tmp5],       %[tmp8]          \n\t"
                    "addu       %[tmp12],   %[tmp4],       %[tmp9]          \n\t"
                    "addu       %[tmp13],   %[tmp3],       %[tmp10]         \n\t"
                    "mult       $ac0,       %[tmp7],       %[temp_const_1]  \n\t"
                    "mult       $ac1,       %[tmp11],      %[temp_const_1]  \n\t"
                    "mult       $ac2,       %[tmp12],      %[temp_const_1]  \n\t"
                    "mult       $ac3,       %[tmp13],      %[temp_const_1]  \n\t"
                    "addu       %[tmp7],    %[tmp3],       %[tmp4]          \n\t"
                    "addu       %[tmp11],   %[tmp2],       %[tmp3]          \n\t"
                    "addu       %[tmp12],   %[tmp1],       %[tmp2]          \n\t"
                    "addu       %[tmp13],   %[tmp8],       %[tmp1]          \n\t"
                    "madd       $ac0,       %[tmp7],       %[cons_20]       \n\t"
                    "madd       $ac1,       %[tmp11],      %[cons_20]       \n\t"
                    "madd       $ac2,       %[tmp12],      %[cons_20]       \n\t"
                    "madd       $ac3,       %[tmp13],      %[cons_20]       \n\t"
                    "addu       %[tmp7],    %[tmp2],       %[tmp5]          \n\t"
                    "addu       %[tmp11],   %[tmp1],       %[tmp4]          \n\t"
                    "addu       %[tmp12],   %[tmp8],       %[tmp3]          \n\t"
                    "addu       %[tmp13],   %[tmp9],       %[tmp2]          \n\t"
                    "msub       $ac0,       %[tmp7],       %[cons_5]        \n\t"
                    "msub       $ac1,       %[tmp11],      %[cons_5]        \n\t"
                    "msub       $ac2,       %[tmp12],      %[cons_5]        \n\t"
                    "msub       $ac3,       %[tmp13],      %[cons_5]        \n\t"
                    "shra_r.w   %[tmp7],    %[tmp4],       5                \n\t"
                    "shra_r.w   %[tmp11],   %[tmp3],       5                \n\t"
                    "shra_r.w   %[tmp12],   %[tmp2],       5                \n\t"
                    "shra_r.w   %[tmp13],   %[tmp1],       5                \n\t"
                    "extr_r.w   %[tmp6],    $ac0,          10               \n\t"
                    "extr_r.w   %[tmp5],    $ac1,          10               \n\t"
                    "extr_r.w   %[tmp4],    $ac2,          10               \n\t"
                    "extr_r.w   %[tmp3],    $ac3,          10               \n\t"
                    "lbux       %[tmp7],    %[tmp7](%[clp])                 \n\t"
                    "lbux       %[tmp11],   %[tmp11](%[clp])                \n\t"
                    "lbux       %[tmp12],   %[tmp12](%[clp])                \n\t"
                    "lbux       %[tmp13],   %[tmp13](%[clp])                \n\t"
                    "lbux       %[tmp6],    %[tmp6](%[clp])                 \n\t"
                    "lbux       %[tmp5],    %[tmp5](%[clp])                 \n\t"
                    "lbux       %[tmp4],    %[tmp4](%[clp])                 \n\t"
                    "lbux       %[tmp3],    %[tmp3](%[clp])                 \n\t"
                    "addqh_r.w  %[tmp6],    %[tmp6],       %[tmp7]          \n\t"
                    "addqh_r.w  %[tmp5],    %[tmp5],       %[tmp11]         \n\t"
                    "addqh_r.w  %[tmp4],    %[tmp4],       %[tmp12]         \n\t"
                    "addqh_r.w  %[tmp3],    %[tmp3],       %[tmp13]         \n\t"
                    "sb         %[tmp6],    0(%[mb])                        \n\t"
                    "sb         %[tmp5],    1(%[mb])                        \n\t"
                    "sb         %[tmp4],    2(%[mb])                        \n\t"
                    "sb         %[tmp3],    3(%[mb])                        \n\t"
                    "addiu      %[ptrJ],    %[ptrJ],       8                \n\t"
                    "addiu      %[mb],      %[mb],         4                \n\t"
                    "move       %[tmp3],    %[tmp9]                         \n\t"
                    "move       %[tmp5],    %[tmp1]                         \n\t"
                    "move       %[tmp7],    %[tmp10]                        \n\t"
                    "move       %[tmp4],    %[tmp8]                         \n\t"
                    "move       %[tmp6],    %[tmp2]                         \n\t"
                    "move       %[tmp2],    %[tmp7]                         \n\t"

                    : [tmp6] "+r" (tmp6), [tmp5] "+r" (tmp5),
                      [tmp4] "+r" (tmp4), [tmp3] "+r" (tmp3),
                      [tmp2] "+r" (tmp2), [tmp1] "=&r" (tmp1),
                      [ptrJ] "+r" (ptrJ),
                      [mb] "+r" (mb), [tmp7] "=&r" (tmp7),
                      [tmp8] "=&r" (tmp8), [tmp9] "=&r" (tmp9),
                      [tmp10] "=&r" (tmp10), [tmp11] "=&r" (tmp11),
                      [tmp12] "=&r" (tmp12), [tmp13] "=&r" (tmp13)
                    : [cons_20] "r" (cons_20), [cons_5] "r" (cons_5),
                      [clp] "r" (clp), [temp_const_1] "r" (temp_const_1)
                    : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
                      "$ac3hi","$ac3lo", "memory"
                );
            }
            ptrJ += 5;
            mb += 16 - partWidth;
        }
    }else{
        for (y = partHeight; y; y--)
        {

            tmp6 = *(ptrJ - 5);
            tmp5 = *(ptrJ - 4);
            tmp4 = *(ptrJ - 3);
            tmp3 = *(ptrJ - 2);
            tmp2 = *(ptrJ - 1);

            for (x = (partWidth>>2); x; x--)
            {

                int tmp8, tmp9, tmp10, tmp11, tmp12, tmp13;

                __asm__ volatile (
                    "lh           %[tmp1],     0(%[ptrJ])                      \n\t"
                    "lh           %[tmp8],     2(%[ptrJ])                      \n\t"
                    "lh           %[tmp9],     4(%[ptrJ])                      \n\t"
                    "lh           %[tmp10],    6(%[ptrJ])                      \n\t"
                    "addu         %[tmp7],     %[tmp6],       %[tmp1]          \n\t"
                    "addu         %[tmp11],    %[tmp5],       %[tmp8]          \n\t"
                    "addu         %[tmp12],    %[tmp4],       %[tmp9]          \n\t"
                    "addu         %[tmp13],    %[tmp3],       %[tmp10]         \n\t"
                    "mult         $ac0,        %[tmp7],       %[temp_const_1]  \n\t"
                    "mult         $ac1,        %[tmp11],      %[temp_const_1]  \n\t"
                    "mult         $ac2,        %[tmp12],      %[temp_const_1]  \n\t"
                    "mult         $ac3,        %[tmp13],      %[temp_const_1]  \n\t"
                    "addu         %[tmp7],     %[tmp3],       %[tmp4]          \n\t"
                    "addu         %[tmp11],    %[tmp2],       %[tmp3]          \n\t"
                    "addu         %[tmp12],    %[tmp1],       %[tmp2]          \n\t"
                    "addu         %[tmp13],    %[tmp8],       %[tmp1]          \n\t"
                    "madd         $ac0,        %[tmp7],       %[cons_20]       \n\t"
                    "madd         $ac1,        %[tmp11],      %[cons_20]       \n\t"
                    "madd         $ac2,        %[tmp12],      %[cons_20]       \n\t"
                    "madd         $ac3,        %[tmp13],      %[cons_20]       \n\t"
                    "addu         %[tmp7],     %[tmp2],       %[tmp5]          \n\t"
                    "addu         %[tmp11],    %[tmp1],       %[tmp4]          \n\t"
                    "addu         %[tmp12],    %[tmp8],       %[tmp3]          \n\t"
                    "addu         %[tmp13],    %[tmp9],       %[tmp2]          \n\t"
                    "msub         $ac0,        %[tmp7],       %[cons_5]        \n\t"
                    "msub         $ac1,        %[tmp11],      %[cons_5]        \n\t"
                    "msub         $ac2,        %[tmp12],      %[cons_5]        \n\t"
                    "msub         $ac3,        %[tmp13],      %[cons_5]        \n\t"
                    "shra_r.w     %[tmp7],     %[tmp3],       5                \n\t"
                    "shra_r.w     %[tmp11],    %[tmp2],       5                \n\t"
                    "shra_r.w     %[tmp12],    %[tmp1],       5                \n\t"
                    "shra_r.w     %[tmp13],    %[tmp8],       5                \n\t"
                    "extr_r.w     %[tmp6],     $ac0,          10               \n\t"
                    "extr_r.w     %[tmp5],     $ac1,          10               \n\t"
                    "extr_r.w     %[tmp4],     $ac2,          10               \n\t"
                    "extr_r.w     %[tmp3],     $ac3,          10               \n\t"
                    "lbux         %[tmp7],     %[tmp7](%[clp])                 \n\t"
                    "lbux         %[tmp11],    %[tmp11](%[clp])                \n\t"
                    "lbux         %[tmp12],    %[tmp12](%[clp])                \n\t"
                    "lbux         %[tmp13],    %[tmp13](%[clp])                \n\t"
                    "lbux         %[tmp6],     %[tmp6](%[clp])                 \n\t"
                    "lbux         %[tmp5],     %[tmp5](%[clp])                 \n\t"
                    "lbux         %[tmp4],     %[tmp4](%[clp])                 \n\t"
                    "lbux         %[tmp3],     %[tmp3](%[clp])                 \n\t"
                    "addqh_r.w    %[tmp6],     %[tmp6],       %[tmp7]          \n\t"
                    "addqh_r.w    %[tmp5],     %[tmp5],       %[tmp11]         \n\t"
                    "addqh_r.w    %[tmp4],     %[tmp4],       %[tmp12]         \n\t"
                    "addqh_r.w    %[tmp3],     %[tmp3],       %[tmp13]         \n\t"
                    "sb           %[tmp6],     0(%[mb])                        \n\t"
                    "sb           %[tmp5],     1(%[mb])                        \n\t"
                    "sb           %[tmp4],     2(%[mb])                        \n\t"
                    "sb           %[tmp3],     3(%[mb])                        \n\t"
                    "addiu        %[ptrJ],     %[ptrJ],       8                \n\t"
                    "addiu        %[mb],       %[mb],         4                \n\t"
                    "move         %[tmp3],     %[tmp9]                         \n\t"
                    "move         %[tmp5],     %[tmp1]                         \n\t"
                    "move         %[tmp7],     %[tmp10]                        \n\t"
                    "move         %[tmp4],     %[tmp8]                         \n\t"
                    "move         %[tmp6],     %[tmp2]                         \n\t"
                    "move         %[tmp2],     %[tmp7]                         \n\t"

                    : [tmp6] "+r" (tmp6), [tmp5] "+r" (tmp5),
                      [tmp4] "+r" (tmp4), [tmp3] "+r" (tmp3),
                      [tmp2] "+r" (tmp2), [tmp1] "=&r" (tmp1),
                      [ptrJ] "+r" (ptrJ),
                      [mb] "+r" (mb), [tmp7] "=&r" (tmp7),
                      [tmp8] "=&r" (tmp8), [tmp9] "=&r" (tmp9),
                      [tmp10] "=&r" (tmp10), [tmp11] "=&r" (tmp11),
                      [tmp12] "=&r" (tmp12), [tmp13] "=&r" (tmp13)
                    : [cons_20] "r" (cons_20), [cons_5] "r" (cons_5),
                      [clp] "r" (clp), [temp_const_1] "r" (temp_const_1)
                    : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
                      "$ac3hi","$ac3lo", "memory"
                );
            }
            ptrJ += 5;
            mb += 16 - partWidth;
        }
    }

}


/*------------------------------------------------------------------------------

    Function: h264bsdPredictSamples

        Functional description:
          This function reconstructs a prediction for a macroblock partition.
          The prediction is either copied or interpolated using the reference
          frame and the motion vector. Both luminance and chrominance parts are
          predicted. The prediction is stored in given macroblock array (data).
        Inputs:
          data          pointer to macroblock array (384 bytes) for output
          mv            pointer to motion vector used for prediction
          refPic        pointer to reference picture structure
          xA            x-coordinate for current macroblock
          yA            y-coordinate for current macroblock
          partX         x-offset for partition in macroblock
          partY         y-offset for partition in macroblock
          partWidth     width of partition
          partHeight    height of partition
        Outputs:
          data          macroblock array (16x16+8x8+8x8) where predicted
                        partition is stored at correct position

------------------------------------------------------------------------------*/

void h264bsdPredictSamples(
  u8 *data,
  mv_t *mv,
  image_t *refPic,
  u32 xA,
  u32 yA,
  u32 partX,
  u32 partY,
  u32 partWidth,
  u32 partHeight)

{

/* Variables */

    u32 xFrac, yFrac, width, height;
    i32 xInt, yInt;
    u8 *lumaPartData;

/* Code */

    ASSERT(data);
    ASSERT(mv);
    ASSERT(partWidth);
    ASSERT(partHeight);
    ASSERT(refPic);
    ASSERT(refPic->data);
    ASSERT(refPic->width);
    ASSERT(refPic->height);

    /* luma */
    lumaPartData = data + 16*partY + partX;

    xFrac = mv->hor & 0x3;
    yFrac = mv->ver & 0x3;

    width = 16 * refPic->width;
    height = 16 * refPic->height;

    xInt = (i32)xA + (i32)partX + (mv->hor >> 2);
    yInt = (i32)yA + (i32)partY + (mv->ver >> 2);

    ASSERT(lumaFracPos[xFrac][yFrac] < 16);

    switch (lumaFracPos[xFrac][yFrac])
    {
        case 0: /* G */
            h264bsdFillBlock(refPic->data, lumaPartData,
                    xInt,yInt,width,height,partWidth,partHeight,16);
            break;
        case 1: /* d */
            h264bsdInterpolateVerQuarter_verOffset_0(refPic->data, lumaPartData,
                    xInt, yInt-2, width, height, partWidth, partHeight);
            break;
        case 2: /* h */
            h264bsdInterpolateVerHalf(refPic->data, lumaPartData,
                    xInt, yInt-2, width, height, partWidth, partHeight);
            break;
        case 3: /* n */
            h264bsdInterpolateVerQuarter_verOffset_1(refPic->data, lumaPartData,
                    xInt, yInt-2, width, height, partWidth, partHeight);
            break;
        case 4: /* a */
            h264bsdInterpolateHorQuarter(refPic->data, lumaPartData,
                    xInt-2, yInt, width, height, partWidth, partHeight, 0);
            break;
        case 5: /* e */
            h264bsdInterpolateHorVerQuarter(refPic->data, lumaPartData,
                    xInt-2, yInt-2, width, height, partWidth, partHeight, 0);
            break;
        case 6: /* i */
            h264bsdInterpolateMidHorQuarter(refPic->data, lumaPartData,
                    xInt-2, yInt-2, width, height, partWidth, partHeight, 0);
            break;
        case 7: /* p */
            h264bsdInterpolateHorVerQuarter(refPic->data, lumaPartData,
                    xInt-2, yInt-2, width, height, partWidth, partHeight, 2);
            break;
        case 8: /* b */
            h264bsdInterpolateHorHalf(refPic->data, lumaPartData,
                    xInt-2, yInt, width, height, partWidth, partHeight);
            break;
        case 9: /* f */
            h264bsdInterpolateMidVerQuarter(refPic->data, lumaPartData,
                    xInt-2, yInt-2, width, height, partWidth, partHeight, 0);
            break;
        case 10: /* j */
            h264bsdInterpolateMidHalf(refPic->data, lumaPartData,
                    xInt-2, yInt-2, width, height, partWidth, partHeight);
            break;
        case 11: /* q */
            h264bsdInterpolateMidVerQuarter(refPic->data, lumaPartData,
                    xInt-2, yInt-2, width, height, partWidth, partHeight, 1);
            break;
        case 12: /* c */
            h264bsdInterpolateHorQuarter(refPic->data, lumaPartData,
                    xInt-2, yInt, width, height, partWidth, partHeight, 1);
            break;
        case 13: /* g */
            h264bsdInterpolateHorVerQuarter(refPic->data, lumaPartData,
                    xInt-2, yInt-2, width, height, partWidth, partHeight, 1);
            break;
        case 14: /* k */
            h264bsdInterpolateMidHorQuarter(refPic->data, lumaPartData,
                    xInt-2, yInt-2, width, height, partWidth, partHeight, 1);
            break;
        default: /* case 15, r */
            h264bsdInterpolateHorVerQuarter(refPic->data, lumaPartData,
                    xInt-2, yInt-2, width, height, partWidth, partHeight, 3);
            break;
    }

    /* chroma */
    PredictChroma(
      data + 16*16 + (partY>>1)*8 + (partX>>1),
      xA + partX,
      yA + partY,
      partWidth,
      partHeight,
      mv,
      refPic);

}

/*------------------------------------------------------------------------------

    Function: h264bsdFillRow7

        Functional description:
          This function gets a row of reference pels when horizontal coordinate
          is partly negative or partly greater than reference picture width
          (overfilling some pels on left and/or right edge).
        Inputs:
          ref       pointer to reference samples
          left      amount of pixels to overfill on left-edge
          center    amount of pixels to copy
          right     amount of pixels to overfill on right-edge
        Outputs:
          fill      pointer where samples are stored

------------------------------------------------------------------------------*/
__attribute__((always_inline)) void h264bsdFillRow7(
  u8 *ref,
  u8 *fill,
  i32 left,
  i32 center,
  i32 right)
{
    u8 tmp;

    ASSERT(ref);
    ASSERT(fill);

    tmp = *ref;
    /* store 4 bytes at once */
    i32 tmp32;
    __asm__ volatile (
        "replv.qb   %[tmp32],   %[tmp]      \n\t"

        : [tmp32] "=&r" (tmp32)
        : [tmp] "r" (tmp)
    );

    int x1 = left >> 2;
    int x2 = left & 3;
    int i;
    for (i = 0; i < x1; i++)
    {
        __asm__ volatile (
            "usw            %[tmp32],      0(%[fill])                \n\t"
            "addiu          %[fill],       %[fill],          4       \n\t"

            : [fill] "+r" (fill)
            : [tmp32] "r" (tmp32)
            : "memory"
        );
    }
    for (i = 0; i < x2; i++)
        *fill++ = tmp;

    x1 = center >> 2;
    x2 = center & 3;
    for (i = 0; i < x1; i++)
    {
        __asm__ volatile (
            "ulw         %[tmp32],      0(%[ref])                   \n\t"
            "addiu       %[ref],        %[ref],           4         \n\t"
            "usw         %[tmp32],      0(%[fill])                  \n\t"
            "addiu       %[fill],       %[fill],          4         \n\t"

            : [ref] "+r" (ref), [fill] "+r" (fill), [tmp32] "=&r" (tmp32)
            :
            : "memory"
        );
    }

    for (i = 0; i < x2; i++)
        *fill++ = *ref++;

    tmp = ref[-1];
    __asm__ volatile (
        "replv.qb      %[tmp32],    %[tmp]         \n\t"

        : [tmp32] "=&r" (tmp32)
        : [tmp] "r" (tmp)
    );
    x1 = right >> 2;
    x2 = right & 3;
    for (i = 0; i < x1; i++)
    {
        __asm__ volatile (
            "usw        %[tmp32],       0(%[fill])               \n\t"
            "addiu      %[fill],        %[fill],         4       \n\t"

            : [fill] "+r" (fill)
            : [tmp32] "r" (tmp32)
            : "memory"
        );
    }
    for (i = 0; i < x2; i++)
        *fill++ = tmp;
}
/*------------------------------------------------------------------------------

    Function: h264bsdFillBlock

        Functional description:
          This function gets a block of reference pels. It determines whether
          overfilling is needed or not and repeatedly calls an appropriate
          function (by using a function pointer) that fills one row the block.
        Inputs:
          ref               pointer to reference frame
          x0                x-coordinate for block
          y0                y-coordinate for block
          width             width of reference frame
          height            height of reference frame
          blockWidth        width of block
          blockHeight       height of block
          fillScanLength    length of a line in output array (pixels)
        Outputs:
          fill              pointer to array where output block is written

------------------------------------------------------------------------------*/
void h264bsdFillBlock(
  u8 *ref,
  u8 *fill,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 blockWidth,
  u32 blockHeight,
  u32 fillScanLength)

{

/* Variables */

    i32 xstop, ystop;
    i32 left, x, right;
    i32 top, y, bottom;
    i32 center;
    u8 fleg;


/* Code */

    ASSERT(ref);
    ASSERT(fill);
    ASSERT(width);
    ASSERT(height);
    ASSERT(fill);
    ASSERT(blockWidth);
    ASSERT(blockHeight);

    xstop = x0 + (i32)blockWidth;
    ystop = y0 + (i32)blockHeight;

    /* Choose correct function whether overfilling on left-edge or right-edge
     * is needed or not */
    if (x0 >= 0 && xstop <= (i32)width){
        fleg = 1;
      }
    else{
        fleg = 0;
      }

    if (ystop < 0)
        y0 = -(i32)blockHeight;

    if (xstop < 0)
        x0 = -(i32)blockWidth;

    if (y0 > (i32)height)
        y0 = (i32)height;

    if (x0 > (i32)width)
        x0 = (i32)width;

    xstop = x0 + (i32)blockWidth;
    ystop = y0 + (i32)blockHeight;

    if (x0 > 0)
        ref += x0;

    if (y0 > 0)
        ref += y0 * (i32)width;

    left = x0 < 0 ? -x0 : 0;
    right = xstop > (i32)width ? xstop - (i32)width : 0;
    x = (i32)blockWidth - left - right;

    top = y0 < 0 ? -y0 : 0;
    bottom = ystop > (i32)height ? ystop - (i32)height : 0;
    y = (i32)blockHeight - top - bottom;

    if(fleg )
    {
        int x1 = x >> 2;
        int x2 = x & 0x3;
        int i, j;

        for ( ; top; top-- )
        {
            int t1;

            for (i = 0; i < x1; i++) {
                __asm__ volatile (
                    "ulw        %[t1],       0(%[ref])               \n\t"
                    "addiu      %[ref],      %[ref],         4       \n\t"
                    "usw        %[t1],       0(%[fill])              \n\t"
                    "addiu      %[fill],     %[fill],        4       \n\t"

                    : [fill] "+r" (fill), [ref] "+r" (ref), [t1] "=&r" (t1)
                    :
                    : "memory"
                );
            }
            for (i = 0; i < x2; i++) {
                *fill++ = *ref++;
            }
            ref -= x;
            fill += fillScanLength-x;
        }

        for ( ; y; y-- )
        {
            int t1;
            {
                __asm__ volatile (
                   "pref       0,           32(%[ref])              \n\t"    /* prefetch for load for the whole two loops that follows */
                   "pref       1,           32(%[fill])             \n\t"    /* prefetch for store for the whole loop that follows */

                   :
                   : [fill] "r" (fill), [ref] "r" (ref)
                   : "memory"
                );
            }
            for (i = 0; i < x1; i++) {
                __asm__ volatile (
                    "ulw        %[t1],       0(%[ref])               \n\t"
                    "addiu      %[ref],      %[ref],         4       \n\t"
                    "usw        %[t1],       0(%[fill])              \n\t"
                    "addiu      %[fill],     %[fill],        4       \n\t"

                    : [fill] "+r" (fill), [ref] "+r" (ref), [t1] "=&r" (t1)
                    :
                    : "memory"
                );
            }
            for (i = 0; i < x2; i++) {
                *fill++ = *ref++;
            }
            ref += width-x;
            fill += fillScanLength-x;
        }

        ref -= width;

        for ( ; bottom; bottom-- )
        {
            int t1;

            for (i = 0; i < x1; i++) {
                __asm__ volatile (
                    "ulw        %[t1],       0(%[ref])               \n\t"
                    "addiu      %[ref],      %[ref],         4       \n\t"
                    "usw        %[t1],       0(%[fill])              \n\t"
                    "addiu      %[fill],     %[fill],        4       \n\t"

                    : [fill] "+r" (fill), [ref] "+r" (ref), [t1] "=&r" (t1)
                    :
                    : "memory"
                );
            }
            for (i = 0; i < x2; i++) {
                *fill++ = *ref++;
            }
            ref -= x;
            fill += fillScanLength-x;
        }

    }
    else
    {
        for ( ; top; top-- )
        {
            h264bsdFillRow7(ref, fill, left, x, right);
            fill += fillScanLength;
        }

        for ( ; y; y-- )
        {
            h264bsdFillRow7(ref, fill, left, x, right);
            ref += width;
            fill += fillScanLength;
        }

        ref -= width;

        for ( ; bottom; bottom-- )
        {
            h264bsdFillRow7(ref, fill, left, x, right);
            fill += fillScanLength;
        }
    }
}


