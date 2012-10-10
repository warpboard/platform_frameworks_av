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
          h264bsdFilterPicture
          FilterVerLumaEdge
          FilterHorLumaEdge
          FilterHorLuma
          FilterVerChromaEdge
          FilterHorChromaEdge
          FilterHorChroma
          InnerBoundaryStrengthFour
          GetBoundaryStrengths
          IsSliceBoundaryOnLeft
          IsSliceBoundaryOnTop
          GetMbFilteringFlags
          GetLumaEdgeThresholds
          GetChromaEdgeThresholds
          FilterLuma
          FilterChroma

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "../h264bsd_util.h"
#include "../h264bsd_macroblock_layer.h"
#include "../h264bsd_deblocking.h"
#include "../h264bsd_dpb.h"


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

/* array of alpha values, from the standard */
static const u8 alphas[52] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,5,6,7,8,9,10,
    12,13,15,17,20,22,25,28,32,36,40,45,50,56,63,71,80,90,101,113,127,144,162,
    182,203,226,255,255};

/* array of beta values, from the standard */
static const u8 betas[52] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,3,3,3,3,4,4,
    4,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18};



/* array of tc0 values, from the standard, each triplet corresponds to a
 * column in the table. Indexing goes as tc0[indexA][bS-1] */
static const u8 tc0[52][3] = {
    {0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},
    {0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},
    {0,0,0},{0,0,1},{0,0,1},{0,0,1},{0,0,1},{0,1,1},{0,1,1},{1,1,1},
    {1,1,1},{1,1,1},{1,1,1},{1,1,2},{1,1,2},{1,1,2},{1,1,2},{1,2,3},
    {1,2,3},{2,2,3},{2,2,4},{2,3,4},{2,3,4},{3,3,5},{3,4,6},{3,4,6},
    {4,5,7},{4,5,8},{4,6,9},{5,7,10},{6,8,11},{6,8,13},{7,10,14},{8,11,16},
    {9,12,18},{10,13,20},{11,15,23},{13,17,25}
};


/* mapping of raster scan block index to 4x4 block index */
static const u32 mb4x4Index[16] =
    {0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};

typedef struct {
    const u8 *tc0;
    u32 alpha;
    u32 beta;
} edgeThreshold_t;

typedef struct {
    u32 top;
    u32 left;
} bS_t;

enum { TOP = 0, LEFT = 1, INNER = 2 };

#define FILTER_LEFT_EDGE    0x04
#define FILTER_TOP_EDGE     0x02
#define FILTER_INNER_EDGE   0x01

/* array of tc_incr values. It used in Filter_Horizontal functions which are optimized for MIPS platform */
static const int tc_incr[205]=
    {0x00000,0x00001,0x10000,0x10001,
     0x00001,0x00000,0x00000,0x00000,
     0x10000,0x00000,0x00000,0x00000,
     0x10001,0x00000,0x00000,0x00000,
     0x00001,0x00002,0x10001,0x10002,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x10000,0x10001,0x20000,0x20001,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x10001,0x10002,0x20001,0x20002,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00001,0x00000,0x00000,0x00000,
     0x00002,0x00000,0x00000,0x00000,
     0x10001,0x00000,0x00000,0x00000,
     0x10002,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x10000,0x00000,0x00000,0x00000,
     0x10001,0x00000,0x00000,0x00000,
     0x20000,0x00000,0x00000,0x00000,
     0x20001,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x00000,0x00000,0x00000,0x00000,
     0x10001,0x00000,0x00000,0x00000,
     0x10002,0x00000,0x00000,0x00000,
     0x20001,0x00000,0x00000,0x00000,
     0x20002
    };

/* clipping table defined in intra_prediction.c */
extern const u8 h264bsdClip[];

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

static void InnerBoundaryStrengthFour(mbStorage_t *mb1, i16 ind1, i16 ind2, i16 ind3, i16 ind4, i16 ind5, i16 ind6, i16 ind7, i16 ind8, u8 *temp);


static u32 EdgeBoundaryStrengthLeft(mbStorage_t *mb1, mbStorage_t *mb2);
static u32 EdgeBoundaryStrengthTop(mbStorage_t *mb1, mbStorage_t *mb2);

static u32 IsSliceBoundaryOnLeft(mbStorage_t *mb);

static u32 IsSliceBoundaryOnTop(mbStorage_t *mb);

static u32 GetMbFilteringFlags(mbStorage_t *mb);


static u32 GetBoundaryStrengths(mbStorage_t *mb,  u32 flags, u8 (*bS_new)[16]);

static void FilterLuma(u8 *data, u8 (*bS_new)[16], edgeThreshold_t *thresholds,
        u32 imageWidth);
static void FilterChroma(u8 *cb, u8 *cr, u8 (*bS_new)[16], edgeThreshold_t *thresholds,
        u32 imageWidth);

static void FilterVerLumaEdge( u8 *data, u32 bS, edgeThreshold_t *thresholds,
        u32 imageWidth);
static void FilterHorLumaEdge( u8 *data, u32 bS, edgeThreshold_t *thresholds,
        i32 imageWidth);
static void FilterHorLuma( u8 *data, u32 bS, edgeThreshold_t *thresholds,
        i32 imageWidth);

static void FilterVerChromaEdge( u8 *data, u32 bS, edgeThreshold_t *thresholds,
        u32 imageWidth);

static void FilterHorChromaEdge( u8 *data, u32 bS, edgeThreshold_t *thresholds,
        i32 imageWidth);
static void FilterHorChroma( u8 *data, u32 bS, edgeThreshold_t *thresholds,
        i32 imageWidth);

static void GetLumaEdgeThresholds(
  edgeThreshold_t *thresholds,
  mbStorage_t *mb,
  u32 filteringFlags);

static void GetChromaEdgeThresholds(
  edgeThreshold_t *thresholds,
  mbStorage_t *mb,
  u32 filteringFlags,
  i32 chromaQpIndexOffset);

/*------------------------------------------------------------------------------

    Function: IsSliceBoundaryOnLeft

        Functional description:
            Function to determine if there is a slice boundary on the left side
            of a macroblock.

------------------------------------------------------------------------------*/
inline u32 IsSliceBoundaryOnLeft(mbStorage_t *mb)
{

/* Variables */

/* Code */

    ASSERT(mb && mb->mbA);

    if (mb->sliceId != mb->mbA->sliceId)
        return(HANTRO_TRUE);
    else
        return(HANTRO_FALSE);

}

/*------------------------------------------------------------------------------

    Function: IsSliceBoundaryOnTop

        Functional description:
            Function to determine if there is a slice boundary above the
            current macroblock.

------------------------------------------------------------------------------*/
inline u32 IsSliceBoundaryOnTop(mbStorage_t *mb)
{

/* Variables */

/* Code */

    ASSERT(mb && mb->mbB);

    if (mb->sliceId != mb->mbB->sliceId)
        return(HANTRO_TRUE);
    else
        return(HANTRO_FALSE);

}

/*------------------------------------------------------------------------------

    Function: GetMbFilteringFlags

        Functional description:
          Function to determine which edges of a macroblock has to be
          filtered. Output is a bit-wise OR of FILTER_LEFT_EDGE,
          FILTER_TOP_EDGE and FILTER_INNER_EDGE, depending on which edges
          shall be filtered.

------------------------------------------------------------------------------*/
u32 GetMbFilteringFlags(mbStorage_t *mb)
{

/* Variables */

    u32 flags = 0;

/* Code */

    ASSERT(mb);

    /* nothing will be filtered if disableDeblockingFilterIdc == 1 */
    if (mb->disableDeblockingFilterIdc != 1)
    {
        flags |= FILTER_INNER_EDGE;

        /* filterLeftMbEdgeFlag, left mb is MB_A */
        if (mb->mbA &&
            ((mb->disableDeblockingFilterIdc != 2) ||
             !IsSliceBoundaryOnLeft(mb)))
            flags |= FILTER_LEFT_EDGE;

        /* filterTopMbEdgeFlag */
        if (mb->mbB &&
            ((mb->disableDeblockingFilterIdc != 2) ||
             !IsSliceBoundaryOnTop(mb)))
            flags |= FILTER_TOP_EDGE;
    }

    return(flags);

}


/*------------------------------------------------------------------------------

    Function: InnerBoundaryStrengthFour

        Functional description:
            Function to calculate boundary strength value bs for four
            edges of a macroblock.

------------------------------------------------------------------------------*/
void InnerBoundaryStrengthFour(mbStorage_t *mb1, i16 ind1, i16 ind2, i16 ind3,
         i16 ind4,i16 ind5, i16 ind6, i16 ind7, i16 ind8, u8 *temp)
{
    i32 tmp1, tmp2, tmp3, tmp4;
    i32 mv1, mv2, mv3, mv4;

    tmp1 = mb1->totalCoeff[ind1];
    tmp2 = mb1->totalCoeff[ind2];
    tmp3 = mb1->totalCoeff[ind3];
    tmp4 = mb1->totalCoeff[ind4];
    mv1  = mb1->mv[ind1].hor;
    mv2  = mb1->mv[ind2].hor;
    mv3  = mb1->mv[ind1].ver;
    mv4  = mb1->mv[ind2].ver;

    if (tmp1 || tmp2)
    {
        *temp = 2;
    }
    else if ( (ABS(mv1 - mv2) >= 4) || (ABS(mv3 - mv4) >= 4) ||
              (mb1->refAddr[ind1 >> 2] != mb1->refAddr[ind2 >> 2]) )
    {
        *temp = 1;
    }else
        *temp = 0;

    tmp1 = mb1->totalCoeff[ind5];
    tmp2 = mb1->totalCoeff[ind6];
    mv1  = mb1->mv[ind3].hor;
    mv2  = mb1->mv[ind4].hor;
    mv3  = mb1->mv[ind3].ver;
    mv4  = mb1->mv[ind4].ver;
    temp ++;
    if (tmp3 || tmp4)
    {
        *temp = 2;
    }
    else if ( (ABS(mv1 - mv2) >= 4) || (ABS(mv3 - mv4) >= 4) ||
              (mb1->refAddr[ind3 >> 2] != mb1->refAddr[ind4 >> 2]) )
    {
        *temp = 1;
    }else
        *temp = 0;

    tmp3 = mb1->totalCoeff[ind7];
    tmp4 = mb1->totalCoeff[ind8];
    mv1  = mb1->mv[ind5].hor;
    mv2  = mb1->mv[ind6].hor;
    mv3  = mb1->mv[ind5].ver;
    mv4  = mb1->mv[ind6].ver;
    temp ++;
    if (tmp1 || tmp2)
    {
        *temp = 2;
    }
    else if ( (ABS(mv1 - mv2) >= 4) || (ABS(mv3 - mv4) >= 4) ||
              (mb1->refAddr[ind5 >> 2] != mb1->refAddr[ind6 >> 2]) )
    {
        *temp = 1;
    }else
        *temp = 0;

    mv1 = mb1->mv[ind7].hor;
    mv2 = mb1->mv[ind8].hor;
    mv3 = mb1->mv[ind7].ver;
    mv4 = mb1->mv[ind8].ver;
    temp ++;
    if (tmp3 || tmp4)
    {
        *temp = 2;
    }
    else if ( (ABS(mv1 - mv2) >= 4) || (ABS(mv3 - mv4) >= 4) ||
              (mb1->refAddr[ind7 >> 2] != mb1->refAddr[ind8 >> 2]) )
    {
        *temp = 1;
    }else
        *temp = 0;

}

/*------------------------------------------------------------------------------

    Function: EdgeBoundaryStrengthTop

        Functional description:
            Function to calculate boundary strength value bs for four
            top-most edge of a macroblock.

------------------------------------------------------------------------------*/
u32 EdgeBoundaryStrengthTop(mbStorage_t *mb1, mbStorage_t *mb2)
{
    u32 topBs = 0;
    u32 tmp1, tmp2, tmp3, tmp4;

    u32 tmp5, tmp6, tmp7, out2;
    u32 four = 0x00040004;
    u32 one = 0x00010001;

    tmp1 = mb1->totalCoeff[0];
    tmp2 = mb2->totalCoeff[10];
    tmp3 = mb1->totalCoeff[1];
    tmp4 = mb2->totalCoeff[11];
    if (tmp1 || tmp2)
    {
        topBs = 2;
    }
    else {
        tmp6 = *(u32*) &(mb1->mv[0]);
        tmp7 = *(u32*) &(mb2->mv[10]);

        __asm__ volatile (
            "subq_s.ph     %[tmp5],      %[tmp6],    %[tmp7]       \n\t"
            "absq_s.ph     %[tmp5],      %[tmp5]                   \n\t"
            "cmp.le.ph     %[four],      %[tmp5]                   \n\t"
            "pick.ph       %[out2],      %[one],     $zero         \n\t"

            : [tmp5] "=&r" (tmp5), [out2] "=r" (out2)
            : [tmp6] "r" (tmp6), [tmp7] "r" (tmp7), [four] "r" (four),
              [one] "r" (one)
        );
        if(out2 || (mb1->refAddr[0] != mb2->refAddr[2]))
        {
            topBs = 1;
        }
    }
    tmp1 = mb1->totalCoeff[4];
    tmp2 = mb2->totalCoeff[14];
    if (tmp3 || tmp4)
    {
        topBs += 2 << 8;
    }
    else {
        tmp6 = *(u32*) &(mb1->mv[1]);
        tmp7 = *(u32*) &(mb2->mv[11]);

        __asm__ volatile (
            "subq_s.ph     %[tmp5],      %[tmp6],    %[tmp7]       \n\t"
            "absq_s.ph     %[tmp5],      %[tmp5]                   \n\t"
            "cmp.le.ph     %[four],      %[tmp5]                   \n\t"
            "pick.ph       %[out2],      %[one],     $zero         \n\t"

            : [tmp5] "=&r" (tmp5), [out2] "=r" (out2)
            : [tmp6] "r" (tmp6), [tmp7] "r" (tmp7),
              [four] "r" (four), [one] "r" (one)
        );
        if(out2 || (mb1->refAddr[0] != mb2->refAddr[2]))
        {
            topBs += 1 << 8;
        }
    }
    tmp3 = mb1->totalCoeff[5];
    tmp4 = mb2->totalCoeff[15];
    if (tmp1 || tmp2)
    {
        topBs += 2 << 16;
    }
    else {
        tmp6 = *(u32*) &(mb1->mv[4]);
        tmp7 = *(u32*) &(mb2->mv[14]);

        __asm__ volatile (
            "subq_s.ph     %[tmp5],      %[tmp6],    %[tmp7]       \n\t"
            "absq_s.ph     %[tmp5],      %[tmp5]                   \n\t"
            "cmp.le.ph     %[four],      %[tmp5]                   \n\t"
            "pick.ph       %[out2],      %[one],     $zero         \n\t"

            : [tmp5] "=&r" (tmp5), [out2] "=r" (out2)
            : [tmp6] "r" (tmp6), [tmp7] "r" (tmp7),
              [four] "r" (four), [one] "r" (one)
        );
        if(out2 || (mb1->refAddr[1] != mb2->refAddr[3]))
        {
            topBs += 1 << 16;
        }
    }
    if (tmp3 || tmp4)
    {
        topBs += 2 << 24;
    }
    else {
        tmp6 = *(u32*) &(mb1->mv[5]);
        tmp7 = *(u32*) &(mb2->mv[15]);

        __asm__ volatile (
            "subq_s.ph     %[tmp5],      %[tmp6],    %[tmp7]       \n\t"
            "absq_s.ph     %[tmp5],      %[tmp5]                   \n\t"
            "cmp.le.ph     %[four],      %[tmp5]                   \n\t"
            "pick.ph       %[out2],      %[one],     $zero         \n\t"

            : [tmp5] "=&r" (tmp5), [out2] "=r" (out2)
            : [tmp6] "r" (tmp6), [tmp7] "r" (tmp7),
              [four] "r" (four), [one] "r" (one)
        );
        if(out2 || (mb1->refAddr[1] != mb2->refAddr[3]))
        {
            topBs += 1 << 24;
        }
    }

    return topBs;
}

/*------------------------------------------------------------------------------

    Function: EdgeBoundaryStrengthLeft

        Functional description:
            Function to calculate boundary strength value bs for four
            left-edge of a macroblock.

------------------------------------------------------------------------------*/
u32 EdgeBoundaryStrengthLeft(mbStorage_t *mb1, mbStorage_t *mb2)
{
    u32 leftBs = 0;
    u32 tmp1, tmp2, tmp3, tmp4;

    u32 tmp5, tmp6, tmp7, out2;
    u32 four = 0x00040004;
    u32 one = 0x00010001;

    tmp1 = mb1->totalCoeff[0];
    tmp2 = mb2->totalCoeff[5];
    tmp3 = mb1->totalCoeff[2];
    tmp4 = mb2->totalCoeff[7];

    if (tmp1 || tmp2)
    {
        leftBs = 2;
    }
    else
    {
        tmp6 = *(u32*) &(mb1->mv[0]);
        tmp7 = *(u32*) &(mb2->mv[5]);

        __asm__ volatile (
            "subq_s.ph     %[tmp5],      %[tmp6],    %[tmp7]       \n\t"
            "absq_s.ph     %[tmp5],      %[tmp5]                   \n\t"
            "cmp.le.ph     %[four],      %[tmp5]                   \n\t"
            "pick.ph       %[out2],      %[one],     $zero         \n\t"

            : [tmp5] "=&r" (tmp5), [out2] "=r" (out2)
            : [tmp6] "r" (tmp6), [tmp7] "r" (tmp7),
              [four] "r" (four), [one] "r" (one)
        );

        if(out2 || (mb1->refAddr[0] != mb2->refAddr[1]))
        {
            leftBs = 1;
        }
    }

    tmp1 = mb1->totalCoeff[8];
    tmp2 = mb2->totalCoeff[13];
    if (tmp3 || tmp4)
    {
        leftBs += 2 << 8;
    }
    else {
        tmp6 = *(u32*) &(mb1->mv[2]);
        tmp7 = *(u32*) &(mb2->mv[7]);

        __asm__ volatile (
            "subq_s.ph     %[tmp5],      %[tmp6],    %[tmp7]       \n\t"
            "absq_s.ph     %[tmp5],      %[tmp5]                   \n\t"
            "cmp.le.ph     %[four],      %[tmp5]                   \n\t"
            "pick.ph       %[out2],      %[one],     $zero         \n\t"

            : [tmp5] "=&r" (tmp5), [out2] "=r" (out2)
            : [tmp6] "r" (tmp6), [tmp7] "r" (tmp7),
              [four] "r" (four), [one] "r" (one)
        );
        if(out2 || (mb1->refAddr[0] != mb2->refAddr[1]))
        {
            leftBs += 1 << 8;
        }
    }

    tmp3 = mb1->totalCoeff[10];
    tmp4 = mb2->totalCoeff[15];
    if (tmp1 || tmp2)
    {
        leftBs += 2 << 16;
    }
    else {
        tmp6 = *(u32*) &(mb1->mv[8]);
        tmp7 = *(u32*) &(mb2->mv[13]);

        __asm__ volatile (
            "subq_s.ph     %[tmp5],      %[tmp6],    %[tmp7]       \n\t"
            "absq_s.ph     %[tmp5],      %[tmp5]                   \n\t"
            "cmp.le.ph     %[four],      %[tmp5]                   \n\t"
            "pick.ph       %[out2],      %[one],     $zero         \n\t"

            : [tmp5] "=&r" (tmp5), [out2] "=r" (out2)
            : [tmp6] "r" (tmp6), [tmp7] "r" (tmp7),
              [four] "r" (four), [one] "r" (one)
        );
        if(out2 || (mb1->refAddr[2] != mb2->refAddr[3]))
        {
            leftBs += 1 << 16;
        }
    }

    if (tmp3 || tmp4)
    {
        leftBs += 2 << 24;
    }
    else {
        tmp6 = *(u32*) &(mb1->mv[10]);
        tmp7 = *(u32*) &(mb2->mv[15]);

        __asm__ volatile (
            "subq_s.ph     %[tmp5],      %[tmp6],    %[tmp7]       \n\t"
            "absq_s.ph     %[tmp5],      %[tmp5]                   \n\t"
            "cmp.le.ph     %[four],      %[tmp5]                   \n\t"
            "pick.ph       %[out2],      %[one],     $zero         \n\t"

            : [tmp5] "=&r" (tmp5), [out2] "=r" (out2)
            : [tmp6] "r" (tmp6), [tmp7] "r" (tmp7),
              [four] "r" (four), [one] "r" (one)
        );
        if(out2 || (mb1->refAddr[2] != mb2->refAddr[3]))
        {
            leftBs += 1 << 24;
        }
    }

    return leftBs;
}

/*------------------------------------------------------------------------------

    Function: h264bsdFilterPicture

        Functional description:
          Perform deblocking filtering for a picture. Filter does not copy
          the original picture anywhere but filtering is performed directly
          on the original image. Parameters controlling the filtering process
          are computed based on information in macroblock structures of the
          filtered macroblock, macroblock above and macroblock on the left of
          the filtered one.

        Inputs:
          image         pointer to image to be filtered
          mb            pointer to macroblock data structure of the top-left
                        macroblock of the picture

        Outputs:
          image         filtered image stored here

        Returns:
          none

------------------------------------------------------------------------------*/
void h264bsdFilterPicture(
  image_t *image,
  mbStorage_t *mb)
{

/* Variables */

    u32 flags;
    u32 picSizeInMbs, mbRow, mbCol;
    u32 picWidthInMbs;
    u8 *data;
    mbStorage_t *pMb;

/*-----------------------------------------------------------------------------
    bS type changed from array of bs_t (u32) to array of (u8). This change
    allows using single load instruction for loading of four bS values. Also,
    this allows compiler to generate more efficient code for MIPS platform.
 -----------------------------------------------------------------------------*/

    u8 bS[2][16];
    edgeThreshold_t thresholds[3];

/* Code */

    ASSERT(image);
    ASSERT(mb);
    ASSERT(image->data);
    ASSERT(image->width);
    ASSERT(image->height);

    picWidthInMbs = image->width;
    data = image->data;
    picSizeInMbs = picWidthInMbs * image->height;

    pMb = mb;

    for (mbRow = 0, mbCol = 0; mbRow < image->height; pMb++)
    {
        flags = GetMbFilteringFlags(pMb);

        if (flags)
        {
            /* GetBoundaryStrengths function returns non-zero value if any of
             * the bS values for the macroblock being processed was non-zero */
            if (GetBoundaryStrengths(pMb, flags, bS))
            {
                /* luma */
                GetLumaEdgeThresholds(thresholds, pMb, flags);
                data = image->data + mbRow * picWidthInMbs * 256 + mbCol * 16;

                FilterLuma((u8*)data, bS, thresholds, picWidthInMbs*16);

                /* chroma */
                GetChromaEdgeThresholds(thresholds, pMb, flags,
                    pMb->chromaQpIndexOffset);
                data = image->data + picSizeInMbs * 256 +
                    mbRow * picWidthInMbs * 64 + mbCol * 8;

                FilterChroma((u8*)data, data + 64*picSizeInMbs, bS,
                        thresholds, picWidthInMbs*8);

            }
        }

        mbCol++;
        if (mbCol == picWidthInMbs)
        {
            mbCol = 0;
            mbRow++;
        }
    }

}

/*------------------------------------------------------------------------------

    Function: FilterVerLumaEdgeL

        Functional description:
            Filter left vertical 16-pixel luma edge.

------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------

   These three functions (FilterVerLumaEdgeL, FilterVerLumaEdge and FilterVerLumaEdge)
   are replacing the original function FilterVerLumaEdge
------------------------------------------------------------------------------*/

void FilterVerLumaEdgeL(
  u8 *data,
  u32 bS,
  edgeThreshold_t *thresholds,
  u32 imageWidth)
{

/* Variables */

    i32 delta, tc, tmp;
    u32 i;
    u8 p0, q0, p1, q1, p2, q2;
    u32 tmpFlag;
    const u8 *clp = h264bsdClip + 512;

/* Code */

    ASSERT(data);
    ASSERT(bS && bS <= 4);
    ASSERT(thresholds);

    if (bS < 4)
    {
        tc = thresholds->tc0[bS-1];
        tmp = tc;
        for (i = 4; i; i--, data += imageWidth)
        {
            p1 = data[-2]; p0 = data[-1];
            q0 = data[0]; q1 = data[1];
            if ( ((unsigned)ABS(p0-q0) < thresholds->alpha) &&
                 ((unsigned)ABS(p1-p0) < thresholds->beta)  &&
                 ((unsigned)ABS(q1-q0) < thresholds->beta) )
            {
                p2 = data[-3];
                q2 = data[2];

                if ((unsigned)ABS(p2-p0) < thresholds->beta)
                {
                    data[-2] = (u8)(p1 + CLIP3(-tc,tc,
                        (p2 + ((p0 + q0 + 1) >> 1) - (p1 << 1)) >> 1));
                    tmp++;
                }

                if ((unsigned)ABS(q2-q0) < thresholds->beta)
                {
                    data[1] = (u8)(q1 + CLIP3(-tc,tc,
                        (q2 + ((p0 + q0 + 1) >> 1) - (q1 << 1)) >> 1));
                    tmp++;
                }

                delta = CLIP3(-tmp, tmp, ((((q0 - p0) << 2) +
                          (p1 - q1) + 4) >> 3));

                p0 = clp[p0 + delta];
                q0 = clp[q0 - delta];
                tmp = tc;
                data[-1] = p0;
                data[ 0] = q0;
            }
        }
    }
    else
    {
        for (i = 4; i; i--, data += imageWidth)
        {
            p1 = data[-2]; p0 = data[-1];
            q0 = data[0]; q1 = data[1];
            if ( ((unsigned)ABS(p0-q0) < thresholds->alpha) &&
                 ((unsigned)ABS(p1-p0) < thresholds->beta)  &&
                 ((unsigned)ABS(q1-q0) < thresholds->beta) )
            {
                tmpFlag =
                    ((unsigned)ABS(p0-q0) < ((thresholds->alpha >> 2) +2)) ?
                        HANTRO_TRUE : HANTRO_FALSE;

                p2 = data[-3];
                q2 = data[2];

                if (tmpFlag && (unsigned)ABS(p2-p0) < thresholds->beta)
                {
                    tmp = p1 + p0 + q0;
                    data[-1] = (u8)((p2 + 2 * tmp + q1 + 4) >> 3);
                    data[-2] = (u8)((p2 + tmp + 2) >> 2);
                    data[-3] = (u8)((2 * data[-4] + 3 * p2 + tmp + 4) >> 3);
                }
                else
                    data[-1] = (2 * p1 + p0 + q1 + 2) >> 2;

                if (tmpFlag && (unsigned)ABS(q2-q0) < thresholds->beta)
                {
                    tmp = p0 + q0 + q1;
                    data[0] = (u8)((p1 + 2 * tmp + q2 + 4) >> 3);
                    data[1] = (u8)((tmp + q2 + 2) >> 2);
                    data[2] = (u8)((2 * data[3] + 3 * q2 + tmp + 4) >> 3);
                }
                else
                    data[0] = (u8)((2 * q1 + q0 + p1 + 2) >> 2);
            }
        }
    }
}

static i32 tmp_table[11] = { 0x0, 0x1 , 0x2, 0, 0x10000, 0x10001, 0x10002, 0, 0x20000, 0x20001, 0x20002 };

/*------------------------------------------------------------------------------

    Function: FilterVerLumaEdge

    Functional description:
        Filter one inner vertical 4-pixel luma edge.

------------------------------------------------------------------------------*/
void FilterVerLumaEdge(
  u8 *data,
  u32 bS,
  edgeThreshold_t *thresholds,
  u32 imageWidth)
{

/* Variables */
    register i32 delta;
    u32 beta = (thresholds->beta << 16) | thresholds->beta;
    register u8  *data_cmp = data + 4 * imageWidth;
    register u32 abb = (thresholds->alpha << 8) | beta;
    register u32 mask = 0x7;
    register i32 tc = thresholds->tc0[bS-1] | (thresholds->tc0[bS-1] << 16);
    register i32 p0, p1, p2, q0, q1, q2, p0q0, cmp1;
    register i32 p1_new, q1_new;
    register i32 tmp_add;
    register u8  *data1 = data;
    register i32 *tmp_table_ptr = tmp_table;

/* Code */

    __asm__ volatile (
        ".set push                                                      \n\t"
        ".set noreorder                                                 \n\t"
        "ulw             %[p1],       -2(%[data])                       \n\t"
        "srl             %[p2],       %[p1],         8                  \n\t"
        "1:                                                             \n\t"
        "addu            %[data],     %[data],       %[imageWidth]      \n\t"
        "subu_s.qb       %[p0],       %[p2],         %[p1]              \n\t"
        "subu_s.qb       %[q1],       %[p1],         %[p2]              \n\t"
        "ulw             %[p1],       -2(%[data])                       \n\t"
        "or              %[q2],       %[q1],         %[p0]              \n\t"
        "cmpgu.lt.qb     %[q0],       %[q2],         %[abb]             \n\t"
        "beq             %[q0],       %[mask],       2f                 \n\t"
        " srl            %[p2],       %[p1],         8                  \n\t"
        "beq             %[data],     %[data_cmp],   7f                 \n\t"
        " nop                                                           \n\t"
        "b               1b                                             \n\t"
        " nop                                                           \n\t"
        "2:                                                             \n\t"
        "beq             %[data],     %[data_cmp],   6f                 \n\t"
        " subu           %[data1],    %[data],       %[imageWidth]      \n\t"
        "3:                                                             \n\t"
        "subu_s.qb       %[p0],       %[p2],         %[p1]              \n\t"
        "subu_s.qb       %[q1],       %[p1],         %[p2]              \n\t"
        "or              %[q2],       %[q1],         %[p0]              \n\t"
        "cmpgu.lt.qb     %[q0],       %[q2],         %[abb]             \n\t"
        "beq             %[q0],       %[mask],       4f                 \n\t"
        " nop                                                           \n\t"
        "addu            %[data],     %[data],       %[imageWidth]      \n\t"
        "beq             %[data],     %[data_cmp],   6f                 \n\t"
        " nop                                                           \n\t"
        "ulw             %[p1],       -2(%[data])                       \n\t"
        "b               3b                                             \n\t"
        " srl            %[p2],       %[p1],         8                  \n\t"
        "6:                                                             \n\t"
        "move            %[data],     %[data1]                          \n\t"
        "addu            %[data_cmp], %[data1],      %[imageWidth]      \n\t"
        "4:                                                             \n\t"
        "lbu             %[tmp_add],  -3(%[data1])                      \n\t"
        "lbu             %[p2],       -3(%[data])                       \n\t"
        "lbu             %[p1],       -2(%[data])                       \n\t"
        "lbu             %[cmp1],     -2(%[data1])                      \n\t"
        "lbu             %[p0],       -1(%[data])                       \n\t"
        "lbu             %[p1_new],   -1(%[data1])                      \n\t"
        "ins             %[p2],       %[tmp_add],    16,     8          \n\t"
        "ins             %[p1],       %[cmp1],       16,     8          \n\t"
        "ins             %[p0],       %[p1_new],     16,     8          \n\t"
        "lbu             %[q0],       0(%[data])                        \n\t"
        "lbu             %[tmp_add],  0(%[data1])                       \n\t"
        "lbu             %[q1],       1(%[data])                        \n\t"
        "lbu             %[q1_new],   1(%[data1])                       \n\t"
        "lbu             %[q2],       2(%[data])                        \n\t"
        "lbu             %[cmp1],     2(%[data1])                       \n\t"
        "ins             %[q0],       %[tmp_add],    16,     8          \n\t"
        "ins             %[q1],       %[q1_new],     16,     8          \n\t"
        "addqh_r.ph      %[p0q0],     %[p0],         %[q0]              \n\t"
        "shll.ph         %[p1_new],   %[p1],         1                  \n\t"
        "shll.ph         %[q1_new],   %[q1],         1                  \n\t"
        "ins             %[q2],       %[cmp1],       16,     8          \n\t"
        "subu.ph         %[p1_new],   %[p0q0],       %[p1_new]          \n\t"
        "subu.ph         %[q1_new],   %[p0q0],       %[q1_new]          \n\t"
        "subu.ph         %[tmp_add],  %[p2],         %[p0]              \n\t"
        "addqh.ph        %[p1_new],   %[p2],         %[p1_new]          \n\t"
        "addqh.ph        %[q1_new],   %[q2],         %[q1_new]          \n\t"
        "subu.ph         %[cmp1],     %[q2],         %[q0]              \n\t"
        "addq.ph         %[p2],       %[p1_new],     %[tc]              \n\t"
        "addq.ph         %[q2],       %[q1_new],     %[tc]              \n\t"
        "absq_s.ph       %[tmp_add],  %[tmp_add]                        \n\t"
        "subq.ph         %[p1_new],   %[p1_new],     %[tc]              \n\t"
        "subq.ph         %[q1_new],   %[q1_new],     %[tc]              \n\t"
        "cmpgdu.lt.qb    %[tmp_add],  %[tmp_add],    %[beta]            \n\t"
        "absq_s.ph       %[p2],       %[p2]                             \n\t"
        "absq_s.ph       %[q2],       %[q2]                             \n\t"
        "subu.ph         %[delta],    %[p1],         %[q1]              \n\t"
        "absq_s.ph       %[p1_new],   %[p1_new]                         \n\t"
        "absq_s.ph       %[q1_new],   %[q1_new]                         \n\t"
        "subu.ph         %[p0q0],     %[q0],         %[p0]              \n\t"
        "subqh.ph        %[p1_new],   %[p2],         %[p1_new]          \n\t"
        "subqh.ph        %[q1_new],   %[q2],         %[q1_new]          \n\t"
        "absq_s.ph       %[cmp1],     %[cmp1]                           \n\t"
        "addu.ph         %[p1_new],   %[p1_new],     %[p1]              \n\t"
        "addu.ph         %[q1_new],   %[q1_new],     %[q1]              \n\t"
        "shll.ph         %[p0q0],     %[p0q0],       2                  \n\t"
        "pick.qb         %[p1_new],   %[p1_new],     %[p1]              \n\t"
        "cmpgdu.lt.qb    %[cmp1],     %[cmp1],       %[beta]            \n\t"
        "addu.ph         %[delta],    %[p0q0],       %[delta]           \n\t"
        "sb              %[p1_new],   -2(%[data])                       \n\t"
        "addu            %[tmp_add],  %[tmp_add],    %[cmp1]            \n\t"
        "sll             %[tmp_add],  %[tmp_add],    2                  \n\t"
        "lwx             %[tmp_add],  %[tmp_add](%[tmp_table_ptr])      \n\t"
        "shra_r.ph       %[delta],    %[delta],      3                  \n\t"
        "pick.qb         %[q1_new],   %[q1_new],     %[q1]              \n\t"
        "addu            %[tmp_add],  %[tmp_add],    %[tc]              \n\t"
        "addq.ph         %[p2],       %[delta],      %[tmp_add]         \n\t"
        "subq.ph         %[delta],    %[delta],      %[tmp_add]         \n\t"
        "srl             %[p1_new],   16                                \n\t"
        "absq_s.ph       %[p2],       %[p2]                             \n\t"
        "absq_s.ph       %[delta],    %[delta]                          \n\t"
        "sb              %[q1_new],   1(%[data])                        \n\t"
        "sb              %[p1_new],   -2(%[data1])                      \n\t"
        "subqh.ph        %[delta],    %[p2], %[delta]                   \n\t"
        "srl             %[q1_new],   16                                \n\t"
        "addu.ph         %[p0],       %[p0],         %[delta]           \n\t"
        "subu.ph         %[q0],       %[q0],         %[delta]           \n\t"
        "sb              %[q1_new],   1(%[data1])                       \n\t"
        "shll_s.ph       %[p0],       %[p0],         7                  \n\t"
        "shll_s.ph       %[q0],       %[q0],         7                  \n\t"
        "precrqu_s.qb.ph %[p0],       %[p0],         %[p0]              \n\t"
        "precrqu_s.qb.ph %[q0],       %[q0],         %[q0]              \n\t"
        "sb              %[p0],       -1(%[data])                       \n\t"
        "sb              %[q0],       0(%[data])                        \n\t"
        "addu            %[data],     %[data],       %[imageWidth]      \n\t"
        "sra             %[p0],       8                                 \n\t"
        "sra             %[q0],       8                                 \n\t"
        "ulw             %[p1],       -2(%[data])                       \n\t"
        "sb              %[p0],       -1(%[data1])                      \n\t"
        "sb              %[q0],       0(%[data1])                       \n\t"
        "bne             %[data],     %[data_cmp],   1b                 \n\t"
        " srl            %[p2],       %[p1],         8                  \n\t"
        "7:                                                             \n\t"
        ".set pop                                                       \n\t"

        : [p0] "=&r" (p0), [p1] "=&r" (p1), [p2] "=&r" (p2),
          [q0] "=&r" (q0), [q1] "=&r" (q1), [q2] "=&r" (q2),
          [p1_new] "=&r" (p1_new), [q1_new] "=&r" (q1_new), [p0q0] "=&r" (p0q0),
          [delta] "=&r" (delta), [cmp1] "=&r" (cmp1), [tmp_add] "=&r" (tmp_add),
          [data1] "=&r" (data1), [data] "+r" (data), [data_cmp] "+r" (data_cmp)
        : [abb] "r" (abb), [beta] "r" (beta), [imageWidth] "r" (imageWidth),
          [tc] "r" (tc), [tmp_table_ptr] "r" (tmp_table_ptr), [mask] "r" (mask)
        : "memory"
    );
}

/*------------------------------------------------------------------------------

    Function: FilterVerLumaEdge

    Functional description:
        Filter left inner vertical 16-pixel luma edge.

------------------------------------------------------------------------------*/
void FilterVerLumaEdges(
  u8 *data,
  u32 bS,
  edgeThreshold_t *thresholds,
  u32 imageWidth)
{

/* Variables */
    register i32 delta;
    u32 beta = (thresholds->beta << 16) | thresholds->beta;
    register u8 *data_cmp = data + 16*imageWidth;
    register u32 abb = (thresholds->alpha << 8) | beta;
    register u32 mask = 0x7;
    register i32 tc = thresholds->tc0[bS-1] | (thresholds->tc0[bS-1] << 16);
    register i32 p0, p1, p2, q0, q1, q2, p0q0, cmp1;
    register i32 p1_new, q1_new;
    register i32 tmp_add;
    register u8  *data1 = data;
    register i32 *tmp_table_ptr = tmp_table;

/* Code */

    __asm__ volatile (
        ".set push                                                      \n\t"
        ".set noreorder                                                 \n\t"
        "ulw             %[p1],       -2(%[data])                       \n\t"
        "srl             %[p2],       %[p1],         8                  \n\t"
        "1:                                                             \n\t"
        "addu            %[data],     %[data],       %[imageWidth]      \n\t"
        "subu_s.qb       %[p0],       %[p2],         %[p1]              \n\t"
        "subu_s.qb       %[q1],       %[p1],         %[p2]              \n\t"
        "ulw             %[p1],       -2(%[data])                       \n\t"
        "or              %[q2],       %[q1],         %[p0]              \n\t"
        "cmpgu.lt.qb     %[q0],       %[q2],         %[abb]             \n\t"
        "beq             %[q0],       %[mask],       2f                 \n\t"
        " srl            %[p2],       %[p1],         8                  \n\t"
        "beq             %[data],     %[data_cmp],   7f                 \n\t"
        " nop                                                           \n\t"
        "b               1b                                             \n\t"
        " nop                                                           \n\t"
        "2:                                                             \n\t"
        "beq             %[data],     %[data_cmp],   6f                 \n\t"
        " subu           %[data1],    %[data],       %[imageWidth]      \n\t"
        "3:                                                             \n\t"
        "subu_s.qb       %[p0],       %[p2],         %[p1]              \n\t"
        "subu_s.qb       %[q1],       %[p1],         %[p2]              \n\t"
        "or              %[q2],       %[q1],         %[p0]              \n\t"
        "cmpgu.lt.qb     %[q0],       %[q2],         %[abb]             \n\t"
        "beq             %[q0],       %[mask],       4f                 \n\t"
        " nop                                                           \n\t"
        "addu            %[data],     %[data],       %[imageWidth]      \n\t"
        "beq             %[data],     %[data_cmp],   6f                 \n\t"
        " nop                                                           \n\t"
        "ulw             %[p1],       -2(%[data])                       \n\t"
        "b               3b                                             \n\t"
        " srl            %[p2],       %[p1],         8                  \n\t"
        "6:                                                             \n\t"
        "move            %[data],     %[data1]                          \n\t"
        "addu            %[data_cmp], %[data1],      %[imageWidth]      \n\t"
        "4:                                                             \n\t"
        "lbu             %[tmp_add],  -3(%[data1])                      \n\t"
        "lbu             %[p2],       -3(%[data])                       \n\t"
        "lbu             %[p1],       -2(%[data])                       \n\t"
        "lbu             %[cmp1],     -2(%[data1])                      \n\t"
        "lbu             %[p0],       -1(%[data])                       \n\t"
        "lbu             %[p1_new],   -1(%[data1])                      \n\t"
        "ins             %[p2],       %[tmp_add],    16,     8          \n\t"
        "ins             %[p1],       %[cmp1],       16,     8          \n\t"
        "ins             %[p0],       %[p1_new],     16,     8          \n\t"
        "lbu             %[q0],       0(%[data])                        \n\t"
        "lbu             %[tmp_add],  0(%[data1])                       \n\t"
        "lbu             %[q1],       1(%[data])                        \n\t"
        "lbu             %[q1_new],   1(%[data1])                       \n\t"
        "lbu             %[q2],       2(%[data])                        \n\t"
        "lbu             %[cmp1],     2(%[data1])                       \n\t"
        "ins             %[q0],       %[tmp_add],    16,     8          \n\t"
        "ins             %[q1],       %[q1_new],     16,     8          \n\t"
        "addqh_r.ph      %[p0q0],     %[p0],         %[q0]              \n\t"
        "shll.ph         %[p1_new],   %[p1],         1                  \n\t"
        "shll.ph         %[q1_new],   %[q1],         1                  \n\t"
        "ins             %[q2],       %[cmp1],       16,     8          \n\t"
        "subu.ph         %[p1_new],   %[p0q0],       %[p1_new]          \n\t"
        "subu.ph         %[q1_new],   %[p0q0],       %[q1_new]          \n\t"
        "subu.ph         %[tmp_add],  %[p2],         %[p0]              \n\t"
        "addqh.ph        %[p1_new],   %[p2],         %[p1_new]          \n\t"
        "addqh.ph        %[q1_new],   %[q2],         %[q1_new]          \n\t"
        "subu.ph         %[cmp1],     %[q2],         %[q0]              \n\t"
        "addq.ph         %[p2],       %[p1_new],     %[tc]              \n\t"
        "addq.ph         %[q2],       %[q1_new],     %[tc]              \n\t"
        "absq_s.ph       %[tmp_add],  %[tmp_add]                        \n\t"
        "subq.ph         %[p1_new],   %[p1_new],     %[tc]              \n\t"
        "subq.ph         %[q1_new],   %[q1_new],     %[tc]              \n\t"
        "cmpgdu.lt.qb    %[tmp_add],  %[tmp_add],    %[beta]            \n\t"
        "absq_s.ph       %[p2],       %[p2]                             \n\t"
        "absq_s.ph       %[q2],       %[q2]                             \n\t"
        "subu.ph         %[delta],    %[p1],         %[q1]              \n\t"
        "absq_s.ph       %[p1_new],   %[p1_new]                         \n\t"
        "absq_s.ph       %[q1_new],   %[q1_new]                         \n\t"
        "subu.ph         %[p0q0],     %[q0],         %[p0]              \n\t"
        "subqh.ph        %[p1_new],   %[p2],         %[p1_new]          \n\t"
        "subqh.ph        %[q1_new],   %[q2],         %[q1_new]          \n\t"
        "absq_s.ph       %[cmp1],     %[cmp1]                           \n\t"
        "addu.ph         %[p1_new],   %[p1_new],     %[p1]              \n\t"
        "addu.ph         %[q1_new],   %[q1_new],     %[q1]              \n\t"
        "shll.ph         %[p0q0],     %[p0q0],       2                  \n\t"
        "pick.qb         %[p1_new],   %[p1_new],     %[p1]              \n\t"
        "cmpgdu.lt.qb    %[cmp1],     %[cmp1],       %[beta]            \n\t"
        "addu.ph         %[delta],    %[p0q0],       %[delta]           \n\t"
        "sb              %[p1_new],   -2(%[data])                       \n\t"
        "addu            %[tmp_add],  %[tmp_add],    %[cmp1]            \n\t"
        "sll             %[tmp_add],  %[tmp_add],    2                  \n\t"
        "lwx             %[tmp_add],  %[tmp_add](%[tmp_table_ptr])      \n\t"
        "shra_r.ph       %[delta],    %[delta],      3                  \n\t"
        "pick.qb         %[q1_new],   %[q1_new],     %[q1]              \n\t"
        "addu            %[tmp_add],  %[tmp_add],    %[tc]              \n\t"
        "addq.ph         %[p2],       %[delta],      %[tmp_add]         \n\t"
        "subq.ph         %[delta],    %[delta],      %[tmp_add]         \n\t"
        "srl             %[p1_new],   16                                \n\t"
        "absq_s.ph       %[p2],       %[p2]                             \n\t"
        "absq_s.ph       %[delta],    %[delta]                          \n\t"
        "sb              %[q1_new],   1(%[data])                        \n\t"
        "sb              %[p1_new],   -2(%[data1])                      \n\t"
        "subqh.ph        %[delta],    %[p2], %[delta]                   \n\t"
        "srl             %[q1_new],   16                                \n\t"
        "addu.ph         %[p0],       %[p0],         %[delta]           \n\t"
        "subu.ph         %[q0],       %[q0],         %[delta]           \n\t"
        "sb              %[q1_new],   1(%[data1])                       \n\t"
        "shll_s.ph       %[p0],       %[p0],         7                  \n\t"
        "shll_s.ph       %[q0],       %[q0],         7                  \n\t"
        "precrqu_s.qb.ph %[p0],       %[p0],         %[p0]              \n\t"
        "precrqu_s.qb.ph %[q0],       %[q0],         %[q0]              \n\t"
        "sb              %[p0],       -1(%[data])                       \n\t"
        "sb              %[q0],       0(%[data])                        \n\t"
        "addu            %[data],     %[data],       %[imageWidth]      \n\t"
        "sra             %[p0],       8                                 \n\t"
        "sra             %[q0],       8                                 \n\t"
        "ulw             %[p1],       -2(%[data])                       \n\t"
        "sb              %[p0],       -1(%[data1])                      \n\t"
        "sb              %[q0],       0(%[data1])                       \n\t"
        "bne             %[data],     %[data_cmp],   1b                 \n\t"
        " srl            %[p2],       %[p1],         8                  \n\t"
        "7:                                                             \n\t"
        ".set pop                                                       \n\t"

        : [p0] "=&r" (p0), [p1] "=&r" (p1), [p2] "=&r" (p2),
          [q0] "=&r" (q0), [q1] "=&r" (q1), [q2] "=&r" (q2),
          [p1_new] "=&r" (p1_new), [q1_new] "=&r" (q1_new), [p0q0] "=&r" (p0q0),
          [delta] "=&r" (delta), [cmp1] "=&r" (cmp1), [tmp_add] "=&r" (tmp_add),
          [data1] "=&r" (data1), [data] "+r" (data), [data_cmp] "+r" (data_cmp)
        : [abb] "r" (abb), [beta] "r" (beta), [imageWidth] "r" (imageWidth),
          [tc] "r" (tc), [tmp_table_ptr] "r" (tmp_table_ptr), [mask] "r" (mask)
        : "memory"
    );
}


/*------------------------------------------------------------------------------

    Function: FilterHorLumaEdge

        Functional description:
            Filter one horizontal 4-pixel luma edge

------------------------------------------------------------------------------*/
void FilterHorLumaEdge(
  u8 *data,
  u32 bS,
  edgeThreshold_t *thresholds,
  i32 imageWidth)
{

/* Variables */

    i32 tc;
    u32 alpha = thresholds->alpha, beta = thresholds->beta;
    u32 mask1, mask2, mask3;
    u32 p0, q0, p1, q1, p2, q2;
    u32 temp1,temp2,temp3,temp4;
    u32 temp5,temp6,temp7,temp8;
    u32 temp10,temp11;

/* Code */

    ASSERT(data);
    ASSERT(bS < 4);
    ASSERT(thresholds);

    tc = thresholds->tc0[bS-1];

    __asm__ volatile (
        "replv.qb        %[alpha],  %[alpha]                       \n\t"
        "replv.qb        %[beta],   %[beta]                        \n\t"
        "replv.ph        %[tc],     %[tc]                          \n\t"

        : [alpha] "+r" (alpha), [beta] "+r" (beta),
          [tc] "+r" (tc)
        :
    );

    __asm__ volatile (
        "subu            %[temp1],  %[data],      %[imageWidth]     \n\t"
        "subu            %[temp2],  %[temp1],     %[imageWidth]     \n\t"
        "subu            %[temp3],  %[temp2],     %[imageWidth]     \n\t"
        "addu            %[temp4],  %[data],      %[imageWidth]     \n\t"
        "addu            %[temp5],  %[temp4],     %[imageWidth]     \n\t"
        "lw              %[q0],     0(%[data])                      \n\t"
        "lw              %[p0],     0(%[temp1])                     \n\t"
        "lw              %[p1],     0(%[temp2])                     \n\t"
        "lw              %[p2],     0(%[temp3])                     \n\t"
        "lw              %[q1],     0(%[temp4])                     \n\t"
        "lw              %[q2],     0(%[temp5])                     \n\t"
        "subu_s.qb       %[temp1],  %[p0],        %[q0]             \n\t"
        "subu_s.qb       %[temp7],  %[q0],        %[p0]             \n\t"
        "subu_s.qb       %[temp2],  %[p1],        %[p0]             \n\t"
        "subu_s.qb       %[temp8],  %[p0],        %[p1]             \n\t"
        "subu_s.qb       %[temp3],  %[q1],        %[q0]             \n\t"
        "subu_s.qb       %[temp11], %[q0],        %[q1]             \n\t"
        "subu_s.qb       %[temp4],  %[p2],        %[p0]             \n\t"
        "subu_s.qb       %[temp10], %[p0],        %[p2]             \n\t"
        "subu_s.qb       %[temp5],  %[q2],        %[q0]             \n\t"
        "subu_s.qb       %[temp6],  %[q0],        %[q2]             \n\t"
        "or              %[temp1],  %[temp1],     %[temp7]          \n\t"
        "or              %[temp2],  %[temp2],     %[temp8]          \n\t"
        "or              %[temp3],  %[temp3],     %[temp11]         \n\t"
        "or              %[temp4],  %[temp4],     %[temp10]         \n\t"
        "or              %[temp5],  %[temp5],     %[temp6]          \n\t"
        "cmpgu.lt.qb     %[mask1],  %[temp1],     %[alpha]          \n\t"
        "cmpgu.lt.qb     %[temp2],  %[temp2],     %[beta]           \n\t"
        "cmpgu.lt.qb     %[temp3],  %[temp3],     %[beta]           \n\t"
        "cmpgu.lt.qb     %[temp4],  %[temp4],     %[beta]           \n\t"
        "cmpgu.lt.qb     %[temp5],  %[temp5],     %[beta]           \n\t"
        "and             %[mask1],  %[mask1],     %[temp2]          \n\t"
        "and             %[mask1],  %[mask1],     %[temp3]          \n\t"
        "and             %[mask2],  %[mask1],     %[temp4]          \n\t"
        "and             %[mask3],  %[mask1],     %[temp5]          \n\t"
        "preceu.ph.qbr   %[temp1],  %[p0]                           \n\t"
        "preceu.ph.qbr   %[temp4],  %[q0]                           \n\t"
        "preceu.ph.qbr   %[temp2],  %[p1]                           \n\t"
        "preceu.ph.qbr   %[temp3],  %[p2]                           \n\t"
        "addqh_r.ph      %[temp7],  %[temp1],     %[temp4]          \n\t"
        "shll.ph         %[temp5],  %[temp2],     1                 \n\t"
        "addq.ph         %[temp6],  %[temp7],     %[temp3]          \n\t"
        "preceu.ph.qbr   %[temp1],  %[q1]                           \n\t"
        "subqh.ph        %[temp5],  %[temp6],     %[temp5]          \n\t"
        "preceu.ph.qbr   %[temp4],  %[q2]                           \n\t"
        "subq.ph         %[temp6],  %[temp5],     %[tc]             \n\t"
        "addq.ph         %[temp5],  %[temp5],     %[tc]             \n\t"
        "absq_s.ph       %[temp6],  %[temp6]                        \n\t"
        "absq_s.ph       %[temp5],  %[temp5]                        \n\t"
        "shll.ph         %[temp8],  %[temp1],     1                 \n\t"
        "preceu.ph.qbl   %[temp11], %[q0]                           \n\t"
        "subqh.ph        %[temp5],  %[temp5],     %[temp6]          \n\t"
        "addq.ph         %[temp6],  %[temp7],     %[temp4]          \n\t"
        "addq.ph         %[temp5],  %[temp5],     %[temp2]          \n\t"
        "subqh.ph        %[temp8],  %[temp6],     %[temp8]          \n\t"
        "preceu.ph.qbl   %[temp3],  %[p2]                           \n\t"
        "subq.ph         %[temp6],  %[temp8],     %[tc]             \n\t"
        "addq.ph         %[temp8],  %[temp8],     %[tc]             \n\t"
        "absq_s.ph       %[temp6],  %[temp6]                        \n\t"
        "absq_s.ph       %[temp8],  %[temp8]                        \n\t"
        "preceu.ph.qbl   %[temp4],  %[p0]                           \n\t"
        "preceu.ph.qbl   %[temp2],  %[p1]                           \n\t"
        "subqh.ph        %[temp8],  %[temp8],     %[temp6]          \n\t"
        "addqh_r.ph      %[temp7],  %[temp4],     %[temp11]         \n\t"
        "addq.ph         %[temp6],  %[temp8],     %[temp1]          \n\t"
        "shll.ph         %[temp8],  %[temp2],     1                 \n\t"
        "addq.ph         %[temp3],  %[temp7],     %[temp3]          \n\t"
        "preceu.ph.qbl   %[temp1],  %[q1]                           \n\t"
        "subqh.ph        %[temp8],  %[temp3],     %[temp8]          \n\t"
        "preceu.ph.qbl   %[temp4],  %[q2]                           \n\t"
        "subq.ph         %[temp3],  %[temp8],     %[tc]             \n\t"
        "addq.ph         %[temp8],  %[temp8],     %[tc]             \n\t"
        "absq_s.ph       %[temp3],  %[temp3]                        \n\t"
        "absq_s.ph       %[temp8],  %[temp8]                        \n\t"
        "subqh.ph        %[temp8],  %[temp8],     %[temp3]          \n\t"
        "addq.ph         %[temp3],  %[temp7],     %[temp4]          \n\t"
        "addq.ph         %[temp10], %[temp8],     %[temp2]          \n\t"
        "shll.ph         %[temp7],  %[temp1],     1                 \n\t"
        "precr.qb.ph     %[temp5],  %[temp10],    %[temp5]          \n\t"
        "subqh.ph        %[temp7],  %[temp3],     %[temp7]          \n\t"
        "subq.ph         %[temp2],  %[temp7],     %[tc]             \n\t"
        "addq.ph         %[temp7],  %[temp7],     %[tc]             \n\t"
        "absq_s.ph       %[temp2],  %[temp2]                        \n\t"
        "absq_s.ph       %[temp7],  %[temp7]                        \n\t"
        "sll             %[temp3],  %[mask2],     4                 \n\t"
        "or              %[temp3],  %[temp3],     %[mask3]          \n\t"
        "subqh.ph        %[temp7],  %[temp7],     %[temp2]          \n\t"
        "andi            %[temp3],  %[temp3],     0x33              \n\t"
        "addq.ph         %[temp11], %[temp7],     %[temp1]          \n\t"
        "sll             %[temp3],  %[temp3],     2                 \n\t"
        "precr.qb.ph     %[temp11], %[temp11],    %[temp6]          \n\t"
        "preceu.ph.qbr   %[temp1],  %[p0]                           \n\t"
        "preceu.ph.qbr   %[temp4],  %[q0]                           \n\t"
        "preceu.ph.qbr   %[temp2],  %[p1]                           \n\t"
        "preceu.ph.qbr   %[temp10], %[q1]                           \n\t"
        "subq.ph         %[temp7],  %[temp4],     %[temp1]          \n\t"
        "subq.ph         %[temp8],  %[temp2],     %[temp10]         \n\t"
        "lwx             %[temp6],  %[temp3](%[tc_incr])            \n\t"
        "li              %[temp3],  0x40004                         \n\t"
        "shll.ph         %[temp7],  %[temp7],     2                 \n\t"
        "addq.ph         %[temp8],  %[temp8],     %[temp3]          \n\t"
        "addq.ph         %[temp7],  %[temp7],     %[temp8]          \n\t"
        "sll             %[temp2],  %[mask2],     4                 \n\t"
        "shra.ph         %[temp7],  %[temp7],     3                 \n\t"
        "addq.ph         %[temp6],  %[temp6],     %[tc]             \n\t"
        "or              %[temp2],  %[temp2],     %[mask3]          \n\t"
        "subq.ph         %[temp8],  %[temp7],     %[temp6]          \n\t"
        "addq.ph         %[temp7],  %[temp7],     %[temp6]          \n\t"
        "absq_s.ph       %[temp8],  %[temp8]                        \n\t"
        "absq_s.ph       %[temp7],  %[temp7]                        \n\t"
        "andi            %[temp2],  %[temp2],     0xcc              \n\t"
        "sll             %[temp2],  %[temp2],     2                 \n\t"
        "subqh.ph        %[temp7],  %[temp7],     %[temp8]          \n\t"
        "lwx             %[temp10], %[temp2](%[tc_incr])            \n\t"
        "subq.ph         %[temp3],  %[temp4],     %[temp7]          \n\t"
        "addq.ph         %[temp6],  %[temp7],     %[temp1]          \n\t"
        "shll_s.ph       %[temp3],  %[temp3],     7                 \n\t"
        "shll_s.ph       %[temp6],  %[temp6],     7                 \n\t"
        "preceu.ph.qbl   %[temp1],  %[p0]                           \n\t"
        "preceu.ph.qbl   %[temp4],  %[q0]                           \n\t"
        "preceu.ph.qbl   %[temp2],  %[p1]                           \n\t"
        "preceu.ph.qbl   %[temp8],  %[q1]                           \n\t"
        "subq.ph         %[temp7],  %[temp4],     %[temp1]          \n\t"
        "subq.ph         %[temp8],  %[temp2],     %[temp8]          \n\t"
        "li              %[temp2],  0x40004                         \n\t"
        "shll.ph         %[temp7],  %[temp7],     2                 \n\t"
        "addq.ph         %[temp8],  %[temp8],     %[temp2]          \n\t"
        "addq.ph         %[temp7],  %[temp7],     %[temp8]          \n\t"
        "sll             %[mask3],  %[mask3],     24                \n\t"
        "addq.ph         %[temp10], %[temp10],    %[tc]             \n\t"
        "shra.ph         %[temp7],  %[temp7],     3                 \n\t"
        "sll             %[mask2],  %[mask2],     24                \n\t"
        "subq.ph         %[temp8],  %[temp7],     %[temp10]         \n\t"
        "addq.ph         %[temp7],  %[temp7],     %[temp10]         \n\t"
        "absq_s.ph       %[temp8],  %[temp8]                        \n\t"
        "absq_s.ph       %[temp7],  %[temp7]                        \n\t"
        "sll             %[mask1],  %[mask1],     24                \n\t"
        "subqh.ph        %[temp7],  %[temp7],     %[temp8]          \n\t"
        "wrdsp           %[mask1]                                   \n\t"
        "subq.ph         %[temp2],  %[temp4],     %[temp7]          \n\t"
        "addq.ph         %[temp10], %[temp7],     %[temp1]          \n\t"
        "shll_s.ph       %[temp2],  %[temp2],     7                 \n\t"
        "shll_s.ph       %[temp10], %[temp10],    7                 \n\t"
        "precrqu_s.qb.ph %[temp3],  %[temp2],     %[temp3]          \n\t"
        "precrqu_s.qb.ph %[temp6],  %[temp10],    %[temp6]          \n\t"
        "pick.qb         %[temp3],  %[temp3],     %[q0]             \n\t"
        "pick.qb         %[temp6],  %[temp6],     %[p0]             \n\t"
        "wrdsp           %[mask2]                                   \n\t"
        "sw              %[temp3],  0(%[data])                      \n\t"
        "subu            %[temp1],  %[data],      %[imageWidth]     \n\t"
        "subu            %[temp10], %[temp1],     %[imageWidth]     \n\t"
        "addu            %[temp2],  %[data],      %[imageWidth]     \n\t"
        "pick.qb         %[temp5],  %[temp5],     %[p1]             \n\t"
        "wrdsp           %[mask3]                                   \n\t"
        "sw              %[temp6],  0(%[temp1])                     \n\t"
        "sw              %[temp5],  0(%[temp10])                    \n\t"
        "pick.qb         %[temp11], %[temp11],    %[q1]             \n\t"
        "sw              %[temp11], 0(%[temp2])                     \n\t"

        : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2), [temp3] "=&r" (temp3),
          [temp4] "=&r" (temp4), [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
          [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
          [temp10] "=&r" (temp10), [temp11] "=&r" (temp11),
          [mask1] "=&r" (mask1), [mask2] "=&r" (mask2), [mask3] "=&r" (mask3),
          [q0] "=&r" (q0), [p0] "=&r" (p0), [p1] "=&r" (p1), [p2] "=&r" (p2),
          [q1] "=&r" (q1), [q2] "=&r" (q2)
        : [alpha] "r" (alpha), [beta] "r" (beta), [tc] "r" (tc),
          [tc_incr] "r" (tc_incr), [data] "r" (data), [imageWidth] "r" (imageWidth)
        : "memory"
    );
}

/*------------------------------------------------------------------------------

    Function: FilterHorLuma

        Functional description:
            Filter all four successive horizontal 4-pixel luma edges. This can
            be done when bS is equal to all four edges.

------------------------------------------------------------------------------*/
void FilterHorLuma(
  u8 *data,
  u32 bS,
  edgeThreshold_t *thresholds,
  i32 imageWidth)
{

    /* Variables */
    i32 tc, tmp;
    u32 i;
    u8 p0, q0, p1, q1, p2, q2;
    u32 tmpFlag;

    u32 alpha = thresholds->alpha, beta = thresholds->beta;

    /* Code */

    ASSERT(data);
    ASSERT(bS <= 4);
    ASSERT(thresholds);

    if (bS < 4)
    {
        tc = thresholds->tc0[bS-1];

        __asm__ volatile (
            "replv.qb           %[alpha],       %[alpha]                \n\t"
            "replv.qb           %[beta],        %[beta]                 \n\t"
            "replv.ph           %[tc],          %[tc]                   \n\t"

            : [alpha] "+r" (alpha), [beta] "+r" (beta),
              [tc] "+r" (tc)
            :
        );

        for (i = 16; i; i-=4, data+=4)
        {
            u32 mask1, mask2, mask3;
            u32 temp1,temp2,temp3,temp4;
            u32 temp5,temp6,temp7,temp8;
            u32 temp10,temp11;

            __asm__ volatile (
                "subu            %[temp1],    %[data],     %[imageWidth]     \n\t"
                "subu            %[temp2],    %[temp1],    %[imageWidth]     \n\t"
                "subu            %[temp3],    %[temp2],    %[imageWidth]     \n\t"
                "addu            %[temp4],    %[data],     %[imageWidth]     \n\t"
                "addu            %[temp5],    %[temp4],    %[imageWidth]     \n\t"
                "lw              %[q0],       0(%[data])                     \n\t"
                "lw              %[p0],       0(%[temp1])                    \n\t"
                "lw              %[p1],       0(%[temp2])                    \n\t"
                "lw              %[p2],       0(%[temp3])                    \n\t"
                "lw              %[q1],       0(%[temp4])                    \n\t"
                "lw              %[q2],       0(%[temp5])                    \n\t"
                "subu_s.qb       %[temp1],    %[p0],       %[q0]             \n\t"
                "subu_s.qb       %[temp7],    %[q0],       %[p0]             \n\t"
                "subu_s.qb       %[temp2],    %[p1],       %[p0]             \n\t"
                "subu_s.qb       %[temp8],    %[p0],       %[p1]             \n\t"
                "subu_s.qb       %[temp3],    %[q1],       %[q0]             \n\t"
                "subu_s.qb       %[temp11],   %[q0],       %[q1]             \n\t"
                "subu_s.qb       %[temp4],    %[p2],       %[p0]             \n\t"
                "subu_s.qb       %[temp10],   %[p0],       %[p2]             \n\t"
                "subu_s.qb       %[temp5],    %[q2],       %[q0]             \n\t"
                "subu_s.qb       %[temp6],    %[q0],       %[q2]             \n\t"
                "or              %[temp1],    %[temp1],    %[temp7]          \n\t"
                "or              %[temp2],    %[temp2],    %[temp8]          \n\t"
                "or              %[temp3],    %[temp3],    %[temp11]         \n\t"
                "or              %[temp4],    %[temp4],    %[temp10]         \n\t"
                "or              %[temp5],    %[temp5],    %[temp6]          \n\t"
                "cmpgu.lt.qb     %[mask1],    %[temp1],    %[alpha]          \n\t"
                "cmpgu.lt.qb     %[temp2],    %[temp2],    %[beta]           \n\t"
                "cmpgu.lt.qb     %[temp3],    %[temp3],    %[beta]           \n\t"
                "cmpgu.lt.qb     %[temp4],    %[temp4],    %[beta]           \n\t"
                "cmpgu.lt.qb     %[temp5],    %[temp5],    %[beta]           \n\t"
                "and             %[mask1],    %[mask1],    %[temp2]          \n\t"
                "and             %[mask1],    %[mask1],    %[temp3]          \n\t"
                "and             %[mask2],    %[mask1],    %[temp4]          \n\t"
                "and             %[mask3],    %[mask1],    %[temp5]          \n\t"
                "preceu.ph.qbr   %[temp1],    %[p0]                          \n\t"
                "preceu.ph.qbr   %[temp4],    %[q0]                          \n\t"
                "preceu.ph.qbr   %[temp2],    %[p1]                          \n\t"
                "preceu.ph.qbr   %[temp3],    %[p2]                          \n\t"
                "addqh_r.ph      %[temp7],    %[temp1],    %[temp4]          \n\t"
                "shll.ph         %[temp5],    %[temp2],    1                 \n\t"
                "addq.ph         %[temp6],    %[temp7],    %[temp3]          \n\t"
                "preceu.ph.qbr   %[temp1],    %[q1]                          \n\t"
                "subqh.ph        %[temp5],    %[temp6],    %[temp5]          \n\t"
                "preceu.ph.qbr   %[temp4],    %[q2]                          \n\t"
                "subq.ph         %[temp6],    %[temp5],    %[tc]             \n\t"
                "addq.ph         %[temp5],    %[temp5],    %[tc]             \n\t"
                "absq_s.ph       %[temp6],    %[temp6]                       \n\t"
                "absq_s.ph       %[temp5],    %[temp5]                       \n\t"
                "shll.ph         %[temp8],    %[temp1],    1                 \n\t"
                "preceu.ph.qbl   %[temp11],   %[q0]                          \n\t"
                "subqh.ph        %[temp5],    %[temp5],    %[temp6]          \n\t"
                "addq.ph         %[temp6],    %[temp7],    %[temp4]          \n\t"
                "addq.ph         %[temp5],    %[temp5],    %[temp2]          \n\t"
                "subqh.ph        %[temp8],    %[temp6],    %[temp8]          \n\t"
                "preceu.ph.qbl   %[temp3],    %[p2]                          \n\t"
                "subq.ph         %[temp6],    %[temp8],    %[tc]             \n\t"
                "addq.ph         %[temp8],    %[temp8],    %[tc]             \n\t"
                "absq_s.ph       %[temp6],    %[temp6]                       \n\t"
                "absq_s.ph       %[temp8],    %[temp8]                       \n\t"
                "preceu.ph.qbl   %[temp4],    %[p0]                          \n\t"
                "preceu.ph.qbl   %[temp2],    %[p1]                          \n\t"
                "subqh.ph        %[temp8],    %[temp8],    %[temp6]          \n\t"
                "addqh_r.ph      %[temp7],    %[temp4],    %[temp11]         \n\t"
                "addq.ph         %[temp6],    %[temp8],    %[temp1]          \n\t"
                "shll.ph         %[temp8],    %[temp2],    1                 \n\t"
                "addq.ph         %[temp3],    %[temp7],    %[temp3]          \n\t"
                "preceu.ph.qbl   %[temp1],    %[q1]                          \n\t"
                "subqh.ph        %[temp8],    %[temp3],    %[temp8]          \n\t"
                "preceu.ph.qbl   %[temp4],    %[q2]                          \n\t"
                "subq.ph         %[temp3],    %[temp8],    %[tc]             \n\t"
                "addq.ph         %[temp8],    %[temp8],    %[tc]             \n\t"
                "absq_s.ph       %[temp3],    %[temp3]                       \n\t"
                "absq_s.ph       %[temp8],    %[temp8]                       \n\t"
                "subqh.ph        %[temp8],    %[temp8],    %[temp3]          \n\t"
                "addq.ph         %[temp3],    %[temp7],    %[temp4]          \n\t"
                "addq.ph         %[temp10],   %[temp8],    %[temp2]          \n\t"
                "shll.ph         %[temp7],    %[temp1],    1                 \n\t"
                "precr.qb.ph     %[temp5],    %[temp10],   %[temp5]          \n\t"
                "subqh.ph        %[temp7],    %[temp3],    %[temp7]          \n\t"
                "subq.ph         %[temp2],    %[temp7],    %[tc]             \n\t"
                "addq.ph         %[temp7],    %[temp7],    %[tc]             \n\t"
                "absq_s.ph       %[temp2],    %[temp2]                       \n\t"
                "absq_s.ph       %[temp7],    %[temp7]                       \n\t"
                "sll             %[temp3],    %[mask2],    4                 \n\t"
                "or              %[temp3],    %[temp3],    %[mask3]          \n\t"
                "subqh.ph        %[temp7],    %[temp7],    %[temp2]          \n\t"
                "andi            %[temp3],    %[temp3],    0x33              \n\t"
                "addq.ph         %[temp11],   %[temp7],    %[temp1]          \n\t"
                "sll             %[temp3],    %[temp3],    2                 \n\t"
                "precr.qb.ph     %[temp11],   %[temp11],   %[temp6]          \n\t"
                "preceu.ph.qbr   %[temp1],    %[p0]                          \n\t"
                "preceu.ph.qbr   %[temp4],    %[q0]                          \n\t"
                "preceu.ph.qbr   %[temp2],    %[p1]                          \n\t"
                "preceu.ph.qbr   %[temp10],   %[q1]                          \n\t"
                "subq.ph         %[temp7],    %[temp4],    %[temp1]          \n\t"
                "subq.ph         %[temp8],    %[temp2],    %[temp10]         \n\t"
                "lwx             %[temp6],    %[temp3](%[tc_incr])           \n\t"
                "li              %[temp3],    0x40004                        \n\t"
                "shll.ph         %[temp7],    %[temp7],    2                 \n\t"
                "addq.ph         %[temp8],    %[temp8],    %[temp3]          \n\t"
                "addq.ph         %[temp7],    %[temp7],    %[temp8]          \n\t"
                "sll             %[temp2],    %[mask2],    4                 \n\t"
                "shra.ph         %[temp7],    %[temp7],    3                 \n\t"
                "addq.ph         %[temp6],    %[temp6],    %[tc]             \n\t"
                "or              %[temp2],    %[temp2],     %[mask3]         \n\t"
                "subq.ph         %[temp8],    %[temp7],    %[temp6]          \n\t"
                "addq.ph         %[temp7],    %[temp7],    %[temp6]          \n\t"
                "absq_s.ph       %[temp8],    %[temp8]                       \n\t"
                "absq_s.ph       %[temp7],    %[temp7]                       \n\t"
                "andi            %[temp2],    %[temp2],    0xcc              \n\t"
                "sll             %[temp2],    %[temp2],    2                 \n\t"
                "subqh.ph        %[temp7],    %[temp7],    %[temp8]          \n\t"
                "lwx             %[temp10],   %[temp2](%[tc_incr])           \n\t"
                "subq.ph         %[temp3],    %[temp4],    %[temp7]          \n\t"
                "addq.ph         %[temp6],    %[temp7],    %[temp1]          \n\t"
                "shll_s.ph       %[temp3],    %[temp3],    7                 \n\t"
                "shll_s.ph       %[temp6],    %[temp6],    7                 \n\t"
                "preceu.ph.qbl   %[temp1],    %[p0]                          \n\t"
                "preceu.ph.qbl   %[temp4],    %[q0]                          \n\t"
                "preceu.ph.qbl   %[temp2],    %[p1]                          \n\t"
                "preceu.ph.qbl   %[temp8],    %[q1]                          \n\t"
                "subq.ph         %[temp7],    %[temp4],    %[temp1]          \n\t"
                "subq.ph         %[temp8],    %[temp2],    %[temp8]          \n\t"
                "li              %[temp2],    0x40004                        \n\t"
                "shll.ph         %[temp7],    %[temp7],    2                 \n\t"
                "addq.ph         %[temp8],    %[temp8],    %[temp2]          \n\t"
                "addq.ph         %[temp7],    %[temp7],    %[temp8]          \n\t"
                "sll             %[mask3],    %[mask3],    24                \n\t"
                "addq.ph         %[temp10],   %[temp10],   %[tc]             \n\t"
                "shra.ph         %[temp7],    %[temp7],    3                 \n\t"
                "sll             %[mask2],    %[mask2],    24                \n\t"
                "subq.ph         %[temp8],    %[temp7],    %[temp10]         \n\t"
                "addq.ph         %[temp7],    %[temp7],    %[temp10]         \n\t"
                "absq_s.ph       %[temp8],    %[temp8]                       \n\t"
                "absq_s.ph       %[temp7],    %[temp7]                       \n\t"
                "sll             %[mask1],    %[mask1],    24                \n\t"
                "subqh.ph        %[temp7],    %[temp7],    %[temp8]          \n\t"
                "wrdsp           %[mask1]                                    \n\t"
                "subq.ph         %[temp2],    %[temp4],    %[temp7]          \n\t"
                "addq.ph         %[temp10],   %[temp7],    %[temp1]          \n\t"
                "shll_s.ph       %[temp2],    %[temp2],    7                 \n\t"
                "shll_s.ph       %[temp10],   %[temp10],   7                 \n\t"
                "precrqu_s.qb.ph %[temp3],    %[temp2],    %[temp3]          \n\t"
                "precrqu_s.qb.ph %[temp6],    %[temp10],   %[temp6]          \n\t"
                "pick.qb         %[temp3],    %[temp3],    %[q0]             \n\t"
                "pick.qb         %[temp6],    %[temp6],    %[p0]             \n\t"
                "wrdsp           %[mask2]                                    \n\t"
                "sw              %[temp3],    0(%[data])                     \n\t"
                "subu            %[temp1],    %[data],     %[imageWidth]     \n\t"
                "subu            %[temp10],   %[temp1],    %[imageWidth]     \n\t"
                "add             %[temp2],    %[data],     %[imageWidth]     \n\t"
                "pick.qb         %[temp5],    %[temp5],    %[p1]             \n\t"
                "wrdsp           %[mask3]                                    \n\t"
                "sw              %[temp6],    0(%[temp1])                    \n\t"
                "sw              %[temp5],    0(%[temp10])                   \n\t"
                "pick.qb         %[temp11],   %[temp11],   %[q1]             \n\t"
                "sw              %[temp11],   0(%[temp2])                    \n\t"

                : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2), [temp3] "=&r" (temp3),
                  [temp4] "=&r" (temp4), [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
                  [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
                  [temp10] "=&r" (temp10), [temp11] "=&r" (temp11),
                  [mask1] "=&r" (mask1), [mask2] "=&r" (mask2), [mask3] "=&r" (mask3),
                  [q0] "=&r" (q0), [p0] "=&r" (p0), [p1] "=&r" (p1), [p2] "=&r" (p2),
                  [q1] "=&r" (q1), [q2] "=&r" (q2)
                : [alpha] "r" (alpha), [beta] "r" (beta), [tc] "r" (tc),
                  [tc_incr] "r" (tc_incr), [data] "r" (data), [imageWidth] "r" (imageWidth)
                : "memory"
            );
        }
    }
    else
    {
        for (i = 16; i; i--, data++)
        {
            p1 = data[-imageWidth*2]; p0 = data[-imageWidth];
            q0 = data[0]; q1 = data[imageWidth];
            if ( ((unsigned)ABS(p0-q0) < thresholds->alpha) &&
                 ((unsigned)ABS(p1-p0) < thresholds->beta)  &&
                 ((unsigned)ABS(q1-q0) < thresholds->beta) )
            {
                tmpFlag = ((unsigned)ABS(p0-q0) < ((thresholds->alpha >> 2) +2))
                            ? HANTRO_TRUE : HANTRO_FALSE;

                p2 = data[-imageWidth*3];
                q2 = data[imageWidth*2];

                if (tmpFlag && (unsigned)ABS(p2-p0) < thresholds->beta)
                {
                    tmp = p1 + p0 + q0;
                    data[-imageWidth] = (u8)((p2 + 2 * tmp + q1 + 4) >> 3);
                    data[-imageWidth*2] = (u8)((p2 + tmp + 2) >> 2);
                    data[-imageWidth*3] = (u8)((2 * data[-imageWidth*4] +
                                           3 * p2 + tmp + 4) >> 3);
                }
                else
                    data[-imageWidth] = (u8)((2 * p1 + p0 + q1 + 2) >> 2);

                if (tmpFlag && (unsigned)ABS(q2-q0) < thresholds->beta)
                {
                    tmp = p0 + q0 + q1;
                    data[ 0] = (u8)((p1 + 2 * tmp + q2 + 4) >> 3);
                    data[imageWidth] = (u8)((tmp + q2 + 2) >> 2);
                    data[imageWidth*2] = (u8)((2 * data[imageWidth*3] +
                                          3 * q2 + tmp + 4) >> 3);
                }
                else
                    data[0] = (2 * q1 + q0 + p1 + 2) >> 2;
            }
        }
    }
}

/*------------------------------------------------------------------------------

    Function: FilterVerChromaEdge

        Functional description:
            Filter one inner vertical 2-pixel chroma edge

------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------

   These three functions (FilterVerChromaEdge, FilterVerChromaEdges_Left and FilterVerChromaEdges)
   are replacing the original function FilterVerChromaEdge
------------------------------------------------------------------------------*/
void FilterVerChromaEdge(
  u8 *data,
  u32 bS,
  edgeThreshold_t *thresholds,
  u32 width)
{

/* Variables */

    i32 tc;
    const u8 *clp = h264bsdClip + 512;
    u8 *data1 = data + width;
    u8 alpha, beta;
    register i32 mask = 7;
    register int temp0, temp1, temp2, temp3;
    register int temp4, temp5, temp6;

/* Code */

    ASSERT(data);
    ASSERT(bS <= 4);
    ASSERT(thresholds);
    tc = thresholds->tc0[bS-1] + 1;
    alpha = thresholds->alpha;
    beta = thresholds->beta;
    register i32 abb = (beta << 16) | (alpha << 8) | beta;

    if (bS < 4)
    {
        __asm__ volatile (
            "ulw            %[temp0],   -2(%[data])                 \n\t"
            "ulw            %[temp3],   -2(%[data1])                \n\t"
            "srl            %[temp1],   %[temp0],       8           \n\t"
            "srl            %[temp4],   %[temp3],       8           \n\t"
            "subu_s.qb      %[temp2],   %[temp0],       %[temp1]    \n\t"
            "subu_s.qb      %[temp1],   %[temp1],       %[temp0]    \n\t"
            "subu_s.qb      %[temp5],   %[temp3],       %[temp4]    \n\t"
            "subu_s.qb      %[temp4],   %[temp4],       %[temp3]    \n\t"
            "or             %[temp2],   %[temp2],       %[temp1]    \n\t"
            "cmpgu.lt.qb    %[temp2],   %[temp2],       %[abb]      \n\t"
            "or             %[temp5],   %[temp5],       %[temp4]    \n\t"
            "cmpgu.lt.qb    %[temp5],   %[temp5],       %[abb]      \n\t"
            "bne            %[temp2],   %[mask],        2f          \n\t"
            " lbu           %[temp3],   -1(%[data])                 \n\t"
            "lbu            %[temp4],   0(%[data])                  \n\t"
            "lbu            %[temp2],   -2(%[data])                 \n\t"
            "lbu            %[temp1],   1(%[data])                  \n\t"
            "subu           %[temp0],   %[temp4],       %[temp3]    \n\t"
            "subu           %[temp1],   %[temp2],       %[temp1]    \n\t"
            "sll            %[temp0],   %[temp0],       2           \n\t"
            "addiu          %[temp1],   %[temp1],       4           \n\t"
            "addu           %[temp0],   %[temp0],       %[temp1]    \n\t"
            "sra            %[temp0],   %[temp0],       3           \n\t"
            "addq_s.w       %[temp1],   %[temp0],       %[tc]       \n\t"
            "subu           %[temp2],   %[temp0],       %[tc]       \n\t"
            "absq_s.w       %[temp1],   %[temp1]                    \n\t"
            "absq_s.w       %[temp2],   %[temp2]                    \n\t"
            "subq_s.w       %[temp0],   %[temp1],       %[temp2]    \n\t"
            "shra_r.w       %[temp0],   %[temp0],       1           \n\t"
            "addq_s.w       %[temp3],   %[temp3],       %[temp0]    \n\t"
            "subq_s.w       %[temp4],   %[temp4],       %[temp0]    \n\t"
            "lbux           %[temp0],   %[temp3](%[clp])            \n\t"
            "lbux           %[temp1],   %[temp4](%[clp])            \n\t"
            "sb             %[temp0],   -1(%[data])                 \n\t"
            "sb             %[temp1],   0(%[data])                  \n\t"
            "2:                                                     \n\t"
            "bne            %[temp5],   %[mask],        3f          \n\t"
            " lbu           %[temp3],   -1(%[data1])                \n\t"
            "lbu            %[temp4],   0(%[data1])                 \n\t"
            "lbu            %[temp2],   -2(%[data1])                \n\t"
            "lbu            %[temp1],   1(%[data1])                 \n\t"
            "subu           %[temp0],   %[temp4],       %[temp3]    \n\t"
            "subu           %[temp1],   %[temp2],       %[temp1]    \n\t"
            "sll            %[temp0],   %[temp0],       2           \n\t"
            "addiu          %[temp1],   %[temp1],       4           \n\t"
            "addu           %[temp0],   %[temp0],       %[temp1]    \n\t"
            "sra            %[temp0],   %[temp0],       3           \n\t"
            "addq_s.w       %[temp1],   %[temp0],       %[tc]       \n\t"
            "subu           %[temp2],   %[temp0],       %[tc]       \n\t"
            "absq_s.w       %[temp1],   %[temp1]                    \n\t"
            "absq_s.w       %[temp2],   %[temp2]                    \n\t"
            "subq_s.w       %[temp0],   %[temp1],       %[temp2]    \n\t"
            "shra_r.w       %[temp0],   %[temp0],       1           \n\t"
            "addq_s.w       %[temp3],   %[temp3],       %[temp0]    \n\t"
            "subq_s.w       %[temp4],   %[temp4],       %[temp0]    \n\t"
            "lbux           %[temp0],   %[temp3](%[clp])            \n\t"
            "lbux           %[temp1],   %[temp4](%[clp])            \n\t"
            "sb             %[temp0],   -1(%[data1])                \n\t"
            "sb             %[temp1],   0(%[data1])                 \n\t"
            "3:                                                     \n\t"

            : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [temp5] "=&r" (temp5)
            : [clp] "r" (clp), [tc] "r"( tc), [abb] "r" (abb), [mask] "r" (mask),
              [data] "r" (data), [data1] "r" (data1)
            : "memory"
        );

    }
    else
    {
        __asm__ volatile (
            "ulw            %[temp0],   -2(%[data])                 \n\t"
            "ulw            %[temp3],   -2(%[data1])                \n\t"
            "srl            %[temp1],   %[temp0],    8              \n\t"
            "srl            %[temp4],   %[temp3],    8              \n\t"
            "subu_s.qb      %[temp2],   %[temp0],    %[temp1]       \n\t"
            "subu_s.qb      %[temp1],   %[temp1],    %[temp0]       \n\t"
            "subu_s.qb      %[temp5],   %[temp3],    %[temp4]       \n\t"
            "subu_s.qb      %[temp4],   %[temp4],    %[temp3]       \n\t"
            "or             %[temp2],   %[temp2],    %[temp1]       \n\t"
            "cmpgu.lt.qb    %[temp2],   %[temp2],    %[abb]         \n\t"
            "or             %[temp5],   %[temp5],    %[temp4]       \n\t"
            "cmpgu.lt.qb    %[temp5],   %[temp5],    %[abb]         \n\t"
            "bne            %[temp2],   %[mask],     2f             \n\t"
            " lbu           %[temp3],   -1(%[data])                 \n\t"
            "lbu            %[temp4],   0(%[data])                  \n\t"
            "lbu            %[temp2],   -2(%[data])                 \n\t"
            "lbu            %[temp1],   1(%[data])                  \n\t"
            "sll            %[temp0],   %[temp2],    1              \n\t"
            "addu           %[temp6],   %[temp3],    %[temp1]       \n\t"
            "addiu          %[temp6],   %[temp6],    2              \n\t"
            "addu           %[temp6],   %[temp6],    %[temp0]       \n\t"
            "sll            %[temp0],   %[temp1],    1              \n\t"
            "addu           %[temp3],   %[temp4],    %[temp2]       \n\t"
            "addiu          %[temp3],   %[temp3],    2              \n\t"
            "addu           %[temp3],   %[temp3],    %[temp0]       \n\t"
            "sra            %[temp6],   %[temp6],    2              \n\t"
            "sra            %[temp3],   %[temp3],    2              \n\t"
            "sb             %[temp6],   -1(%[data])                 \n\t"
            "sb             %[temp3],   0(%[data])                  \n\t"
            "2:                                                     \n\t"
            "bne            %[temp5],   %[mask],     3f             \n\t"
            " lbu           %[temp3],   -1(%[data1])                \n\t"
            "lbu            %[temp4],   (%[data1])                  \n\t"
            "lbu            %[temp2],   -2(%[data1])                \n\t"
            "lbu            %[temp1],   1(%[data1])                 \n\t"
            "sll            %[temp0],   %[temp2],    1              \n\t"
            "addu           %[temp6],   %[temp3],    %[temp1]       \n\t"
            "addiu          %[temp6],   %[temp6],    2              \n\t"
            "addu           %[temp6],   %[temp6],    %[temp0]       \n\t"
            "sll            %[temp0],   %[temp1],    1              \n\t"
            "addu           %[temp3],   %[temp4],    %[temp2]       \n\t"
            "addiu          %[temp3],   %[temp3],    2              \n\t"
            "addu           %[temp3],   %[temp3],    %[temp0]       \n\t"
            "sra            %[temp6],   %[temp6],    2              \n\t"
            "sra            %[temp3],   %[temp3],    2              \n\t"
            "sb             %[temp6],   -1(%[data1])                \n\t"
            "sb             %[temp3],   0(%[data1])                 \n\t"
            "3:                                                     \n\t"

            : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [temp5] "=&r" (temp5),
              [temp6] "=&r" (temp6)
            : [abb] "r" (abb), [data] "r" (data), [data1] "r" (data1),
              [mask] "r" (mask)
            : "memory"
        );
    }
}

/*------------------------------------------------------------------------------

    Function: FilterVerChromaEdges_Left

        Functional description:
            Filter left vertical 4-pixel chroma edge

------------------------------------------------------------------------------*/

void FilterVerChromaEdges_Left(
  u8 *data,
  u8 *data1,
  u32 bS,
  edgeThreshold_t *thresholds,
  u32 width)
{

/* Variables */

    i32 tc;
    const u8 *clp = h264bsdClip + 512;
    u8 i;

    u8 alpha, beta;
    register i32 mask = 7;
    register int temp0, temp1, temp2, temp3;
    register int temp4, temp5, temp6;
    register u8 *data_cmp = data + 8*width;

/* Code */

    ASSERT(data);
    ASSERT(bS <= 4);
    ASSERT(thresholds);
    tc = thresholds->tc0[bS-1] + 1;
    alpha = thresholds->alpha;
    beta = thresholds->beta;
    register i32 abb = (beta << 16) | (alpha << 8) | beta;

    if (bS < 4)
    {
        __asm__ volatile (
            ".set push                                              \n\t"
            ".set noreorder                                         \n\t"
            "3:                                                     \n\t"
            "ulw            %[temp0],   -2(%[data])                 \n\t"
            "ulw            %[temp3],   -2(%[data1])                \n\t"
            "srl            %[temp1],   %[temp0],       8           \n\t"
            "srl            %[temp4],   %[temp3],       8           \n\t"
            "subu_s.qb      %[temp2],   %[temp0],       %[temp1]    \n\t"
            "subu_s.qb      %[temp1],   %[temp1],       %[temp0]    \n\t"
            "subu_s.qb      %[temp5],   %[temp3],       %[temp4]    \n\t"
            "subu_s.qb      %[temp3],   %[temp4],       %[temp3]    \n\t"
            "or             %[temp2],   %[temp2],       %[temp1]    \n\t"
            "cmpgu.lt.qb    %[temp2],   %[temp2],       %[abb]      \n\t"
            "or             %[temp5],   %[temp5],       %[temp3]    \n\t"
            "bne            %[temp2],   %[mask],        2f          \n\t"
            " cmpgu.lt.qb   %[temp5],   %[temp5],       %[abb]      \n\t"
            "lbu            %[temp3],   -1(%[data])                 \n\t"
            "lbu            %[temp4],   0(%[data])                  \n\t"
            "lbu            %[temp2],   -2(%[data])                 \n\t"
            "lbu            %[temp1],   1(%[data])                  \n\t"
            "subu           %[temp0],   %[temp4],       %[temp3]    \n\t"
            "subu           %[temp1],   %[temp2],       %[temp1]    \n\t"
            "sll            %[temp0],   %[temp0],       2           \n\t"
            "addiu          %[temp1],   %[temp1],       4           \n\t"
            "addu           %[temp0],   %[temp0],       %[temp1]    \n\t"
            "sra            %[temp0],   %[temp0],       3           \n\t"
            "addq_s.w       %[temp1],   %[temp0],       %[tc]       \n\t"
            "subu           %[temp2],   %[temp0],       %[tc]       \n\t"
            "absq_s.w       %[temp1],   %[temp1]                    \n\t"
            "absq_s.w       %[temp2],   %[temp2]                    \n\t"
            "subq_s.w       %[temp0],   %[temp1],       %[temp2]    \n\t"
            "shra_r.w       %[temp0],   %[temp0],       1           \n\t"
            "addq_s.w       %[temp3],   %[temp3],       %[temp0]    \n\t"
            "subq_s.w       %[temp4],   %[temp4],       %[temp0]    \n\t"
            "lbux           %[temp0],   %[temp3](%[clp])            \n\t"
            "lbux           %[temp1],   %[temp4](%[clp])            \n\t"
            "sb             %[temp0],   -1(%[data])                 \n\t"
            "sb             %[temp1],   0(%[data])                  \n\t"
            "2:                                                     \n\t"
            "bne            %[temp5],   %[mask],        5f          \n\t"
            " addu          %[data],    %[data],        %[width]    \n\t"
            "lbu            %[temp3],   -1(%[data1])                \n\t"
            "lbu            %[temp4],   0(%[data1])                 \n\t"
            "lbu            %[temp2],   -2(%[data1])                \n\t"
            "lbu            %[temp1],   1(%[data1])                 \n\t"
            "subu           %[temp0],   %[temp4],       %[temp3]    \n\t"
            "subu           %[temp1],   %[temp2],       %[temp1]    \n\t"
            "sll            %[temp0],   %[temp0],       2           \n\t"
            "addiu          %[temp1],   %[temp1],       4           \n\t"
            "addu           %[temp0],   %[temp0],       %[temp1]    \n\t"
            "sra            %[temp0],   %[temp0],       3           \n\t"
            "addq_s.w       %[temp1],   %[temp0],       %[tc]       \n\t"
            "subu           %[temp2],   %[temp0],       %[tc]       \n\t"
            "absq_s.w       %[temp1],   %[temp1]                    \n\t"
            "absq_s.w       %[temp2],   %[temp2]                    \n\t"
            "subq_s.w       %[temp0],   %[temp1],       %[temp2]    \n\t"
            "shra_r.w       %[temp0],   %[temp0],       1           \n\t"
            "addq_s.w       %[temp3],   %[temp3],       %[temp0]    \n\t"
            "subq_s.w       %[temp4],   %[temp4],       %[temp0]    \n\t"
            "lbux           %[temp0],   %[temp3](%[clp])            \n\t"
            "lbux           %[temp1],   %[temp4](%[clp])            \n\t"
            "sb             %[temp0],   -1(%[data1])                \n\t"
            "sb             %[temp1],   0(%[data1])                 \n\t"
            "5:                                                     \n\t"
            " bne           %[data],    %[data_cmp],    3b          \n\t"
            "addu           %[data1],   %[data1],       %[width]    \n\t"
            ".set pop                                               \n\t"

            : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [temp5] "=&r" (temp5),
              [data] "+r" (data), [data1] "+r" (data1)
            : [clp] "r" (clp), [tc] "r" (tc), [abb] "r" (abb),
              [width]"r"(width), [data_cmp] "r" (data_cmp), [mask] "r" (mask)
            : "memory"
        );
    }
    else
    {
        for (i = 8; i; i--)
        {
            __asm__ volatile (
                "ulw            %[temp0],  -2(%[data])                  \n\t"
                "ulw            %[temp3],  -2(%[data1])                 \n\t"
                "srl            %[temp1],  %[temp0],     8              \n\t"
                "srl            %[temp4],  %[temp3],     8              \n\t"
                "subu_s.qb      %[temp2],  %[temp0],     %[temp1]       \n\t"
                "subu_s.qb      %[temp1],  %[temp1],     %[temp0]       \n\t"
                "subu_s.qb      %[temp5],  %[temp3],     %[temp4]       \n\t"
                "subu_s.qb      %[temp4],  %[temp4],     %[temp3]       \n\t"
                "or             %[temp2],  %[temp2],     %[temp1]       \n\t"
                "cmpgu.lt.qb    %[temp2],  %[temp2],     %[abb]         \n\t"
                "or             %[temp5],  %[temp5],     %[temp4]       \n\t"
                "cmpgu.lt.qb    %[temp5],  %[temp5],     %[abb]         \n\t"
                "bne            %[temp2],  %[mask],      2f             \n\t"
                " lbu           %[temp3],  -1(%[data])                  \n\t"
                "lbu            %[temp4],  0(%[data])                   \n\t"
                "lbu            %[temp2],  -2(%[data])                  \n\t"
                "lbu            %[temp1],  1(%[data])                   \n\t"
                "sll            %[temp0],  %[temp2],     1              \n\t"
                "addu           %[temp6],  %[temp3],     %[temp1]       \n\t"
                "addiu          %[temp6],  %[temp6],     2              \n\t"
                "addu           %[temp6],  %[temp6],     %[temp0]       \n\t"
                "sll            %[temp0],  %[temp1],     1              \n\t"
                "addu           %[temp3],  %[temp4],     %[temp2]       \n\t"
                "addiu          %[temp3],  %[temp3],     2              \n\t"
                "addu           %[temp3],  %[temp3],     %[temp0]       \n\t"
                "sra            %[temp6],  %[temp6],     2              \n\t"
                "sra            %[temp3],  %[temp3],     2              \n\t"
                "sb             %[temp6],  -1(%[data])                  \n\t"
                "sb             %[temp3],  0(%[data])                   \n\t"
                "2:                                                     \n\t"
                " addu          %[data],   %[data],      %[width]       \n\t"
                "bne            %[temp5],  %[mask],      3f             \n\t"
                " lbu           %[temp3],  -1(%[data1])                 \n\t"
                "lbu            %[temp4],  (%[data1])                   \n\t"
                "lbu            %[temp2],  -2(%[data1])                 \n\t"
                "lbu            %[temp1],  1(%[data1])                  \n\t"
                "sll            %[temp0],  %[temp2],     1              \n\t"
                "addu           %[temp6],  %[temp3],     %[temp1]       \n\t"
                "addiu          %[temp6],  %[temp6],     2              \n\t"
                "addu           %[temp6],  %[temp6],     %[temp0]       \n\t"
                "sll            %[temp0],  %[temp1],     1              \n\t"
                "addu           %[temp3],  %[temp4],     %[temp2]       \n\t"
                "addiu          %[temp3],  %[temp3],     2              \n\t"
                "addu           %[temp3],  %[temp3],     %[temp0]       \n\t"
                "sra            %[temp6],  %[temp6],     2              \n\t"
                "sra            %[temp3],  %[temp3],     2              \n\t"
                "sb             %[temp6],  -1(%[data1])                 \n\t"
                "sb             %[temp3],  0(%[data1])                  \n\t"
                "3:                                                     \n\t"
                " addu          %[data1],  %[data1],     %[width]       \n\t"

                : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                  [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [temp5] "=&r" (temp5),
                  [data] "+r" (data), [data1] "+r" (data1), [temp6] "=&r" (temp6)
                : [abb] "r" (abb), [width] "r" (width), [mask] "r" (mask)
                : "memory"
           );

        }
    }
}

/*------------------------------------------------------------------------------

    Function: FilterVerChromaEdges

        Functional description:
            Filter one inner vertical 4-pixel chroma edge

------------------------------------------------------------------------------*/
void FilterVerChromaEdges(
  u8 *data,
  u8 *data1,
  u32 bS,
  edgeThreshold_t *thresholds,
  u32 width)
{

/* Variables */

    i32 tc;
    const u8 *clp = h264bsdClip + 512;
    u8 alpha, beta;
    register i32 mask = 7;
    register int temp0, temp1, temp2, temp3;
    register int temp4, temp5;
    register u8  *data_cmp = data + 8*width;

/* Code */

    ASSERT(data);
    ASSERT(bS <= 4);
    ASSERT(thresholds);

    tc = thresholds->tc0[bS-1] + 1;
    alpha = thresholds->alpha;
    beta = thresholds->beta;
    register i32 abb = (beta << 16) | (alpha << 8) | beta;

    __asm__ volatile (
        ".set push                                              \n\t"
        ".set noreorder                                         \n\t"
        "3:                                                     \n\t"
        "ulw            %[temp0],   -2(%[data])                 \n\t"
        "ulw            %[temp3],   -2(%[data1])                \n\t"
        "srl            %[temp1],   %[temp0],       8           \n\t"
        "srl            %[temp4],   %[temp3],       8           \n\t"
        "subu_s.qb      %[temp2],   %[temp0],       %[temp1]    \n\t"
        "subu_s.qb      %[temp1],   %[temp1],       %[temp0]    \n\t"
        "subu_s.qb      %[temp5],   %[temp3],       %[temp4]    \n\t"
        "subu_s.qb      %[temp3],   %[temp4],       %[temp3]    \n\t"
        "or             %[temp2],   %[temp2],       %[temp1]    \n\t"
        "cmpgu.lt.qb    %[temp2],   %[temp2],       %[abb]      \n\t"
        "or             %[temp5],   %[temp5],       %[temp3]    \n\t"
        "bne            %[temp2],   %[mask],        2f          \n\t"
        "cmpgu.lt.qb    %[temp5],   %[temp5],       %[abb]      \n\t"
        "lbu            %[temp3],   -1(%[data])                 \n\t"
        "lbu            %[temp4],   0(%[data])                  \n\t"
        "lbu            %[temp2],   -2(%[data])                 \n\t"
        "lbu            %[temp1],   1(%[data])                  \n\t"
        "subu           %[temp0],   %[temp4],       %[temp3]    \n\t"
        "subu           %[temp1],   %[temp2],       %[temp1]    \n\t"
        "sll            %[temp0],   %[temp0],       2           \n\t"
        "addiu          %[temp1],   %[temp1],       4           \n\t"
        "addu           %[temp0],   %[temp0],       %[temp1]    \n\t"
        "sra            %[temp0],   %[temp0],       3           \n\t"
        "addq_s.w       %[temp1],   %[temp0],       %[tc]       \n\t"
        "subu           %[temp2],   %[temp0],       %[tc]       \n\t"
        "absq_s.w       %[temp1],   %[temp1]                    \n\t"
        "absq_s.w       %[temp2],   %[temp2]                    \n\t"
        "subq_s.w       %[temp0],   %[temp1],       %[temp2]    \n\t"
        "shra_r.w       %[temp0],   %[temp0],       1           \n\t"
        "addq_s.w       %[temp3],   %[temp3],       %[temp0]    \n\t"
        "subq_s.w       %[temp4],   %[temp4],       %[temp0]    \n\t"
        "lbux           %[temp0],   %[temp3](%[clp])            \n\t"
        "lbux           %[temp1],   %[temp4](%[clp])            \n\t"
        "sb             %[temp0],   -1(%[data])                 \n\t"
        "sb             %[temp1],   0(%[data])                  \n\t"
        "2:                                                     \n\t"
        "bne            %[temp5],   %[mask],        5f          \n\t"
        " addu          %[data],    %[data],        %[width]    \n\t"
        "lbu            %[temp3],   -1(%[data1])                \n\t"
        "lbu            %[temp4],   (%[data1])                  \n\t"
        "lbu            %[temp2],   -2(%[data1])                \n\t"
        "lbu            %[temp1],   1(%[data1])                 \n\t"
        "subu           %[temp0],   %[temp4],       %[temp3]    \n\t"
        "subu           %[temp1],   %[temp2],       %[temp1]    \n\t"
        "sll            %[temp0],   %[temp0],       2           \n\t"
        "addiu          %[temp1],   %[temp1],       4           \n\t"
        "addu           %[temp0],   %[temp0],       %[temp1]    \n\t"
        "sra            %[temp0],   %[temp0],       3           \n\t"
        "addq_s.w       %[temp1],   %[temp0],       %[tc]       \n\t"
        "subu           %[temp2],   %[temp0],       %[tc]       \n\t"
        "absq_s.w       %[temp1],   %[temp1]                    \n\t"
        "absq_s.w       %[temp2],   %[temp2]                    \n\t"
        "subq_s.w       %[temp0],   %[temp1],       %[temp2]    \n\t"
        "shra_r.w       %[temp0],   %[temp0],       1           \n\t"
        "addq_s.w       %[temp3],   %[temp3],       %[temp0]    \n\t"
        "subq_s.w       %[temp4],   %[temp4],       %[temp0]    \n\t"
        "lbux           %[temp0],   %[temp3](%[clp])            \n\t"
        "lbux           %[temp1],   %[temp4](%[clp])            \n\t"
        "sb             %[temp0],   -1(%[data1])                \n\t"
        "sb             %[temp1],   0(%[data1])                 \n\t"
        "5:                                                     \n\t"
        "bne            %[data],    %[data_cmp],    3b          \n\t"
        " addu          %[data1],   %[data1],       %[width]    \n\t"
        ".set pop                                               \n\t"

        : [temp0] "=&r" (temp0), [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
          [temp3] "=&r" (temp3), [temp4] "=&r" (temp4), [temp5] "=&r" (temp5),
          [data] "+r" (data), [data1] "+r" (data1)
        : [clp] "r" (clp), [tc] "r" (tc), [abb] "r" (abb),
          [width] "r" (width), [data_cmp] "r" (data_cmp), [mask] "r" (mask)
        : "memory"
    );
}


/*------------------------------------------------------------------------------

    Function: FilterHorChromaEdge

        Functional description:
            Filter one horizontal 2-pixel chroma edge

------------------------------------------------------------------------------*/
void FilterHorChromaEdge(
  u8 *data,
  u32 bS,
  edgeThreshold_t *thresholds,
  i32 width)
{

/* Variables */

    i32  tc;
    u32 alpha = thresholds->alpha, beta = thresholds->beta;
    u32 mask1;
    u32 p0, q0;
    u32 temp1,temp2,temp3,temp4;
    u32 temp5,temp6,temp7;

/* Code */

    ASSERT(data);
    ASSERT(bS < 4);
    ASSERT(thresholds);

    tc = thresholds->tc0[bS-1] + 1;

    __asm__ volatile (
        "replv.qb           %[alpha],       %[alpha]                    \n\t"
        "replv.qb           %[beta],        %[beta]                     \n\t"
        "replv.ph           %[tc],          %[tc]                       \n\t"

        : [alpha] "+r" (alpha), [beta] "+r" (beta),
          [tc] "+r" (tc)
        :
     );

    __asm__ volatile (
        "subu               %[temp1],       %[data],        %[width]    \n\t"
        "subu               %[temp2],       %[temp1],       %[width]    \n\t"
        "addu               %[temp3],       %[data],        %[width]    \n\t"
        "lh                 %[q0],          0(%[data])                  \n\t"
        "lh                 %[p0],          0(%[temp1])                 \n\t"
        "lh                 %[temp2],       0(%[temp2])                 \n\t"
        "lh                 %[temp3],       0(%[temp3])                 \n\t"
        "preceu.ph.qbr      %[temp4],       %[p0]                       \n\t"
        "preceu.ph.qbr      %[temp5],       %[q0]                       \n\t"
        "preceu.ph.qbr      %[temp6],       %[temp2]                    \n\t"
        "preceu.ph.qbr      %[temp7],       %[temp3]                    \n\t"
        "subu.ph            %[temp1],       %[temp4],       %[temp5]    \n\t"
        "subu.ph            %[temp2],       %[temp6],       %[temp4]    \n\t"
        "subu.ph            %[temp3],       %[temp7],       %[temp5]    \n\t"
        "absq_s.ph          %[temp1],       %[temp1]                    \n\t"
        "absq_s.ph          %[temp2],       %[temp2]                    \n\t"
        "absq_s.ph          %[temp3],       %[temp3]                    \n\t"
        "precr.qb.ph        %[temp1],       %[temp1],       %[temp1]    \n\t"
        "precr.qb.ph        %[temp2],       %[temp2],       %[temp2]    \n\t"
        "precr.qb.ph        %[temp3],       %[temp3],       %[temp3]    \n\t"
        "cmpgu.lt.qb        %[mask1],       %[temp1],       %[alpha]    \n\t"
        "cmpgu.lt.qb        %[temp2],       %[temp2],       %[beta]     \n\t"
        "cmpgu.lt.qb        %[temp3],       %[temp3],       %[beta]     \n\t"
        "and                %[mask1],       %[mask1],       %[temp2]    \n\t"
        "and                %[mask1],       %[mask1],       %[temp3]    \n\t"
        "subq.ph            %[temp1],       %[temp5],       %[temp4]    \n\t"
        "subq.ph            %[temp7],       %[temp6],       %[temp7]    \n\t"
        "li                 %[temp2],       0x40004                     \n\t"
        "shll.ph            %[temp1],       %[temp1],       2           \n\t"
        "addq.ph            %[temp7],       %[temp7],       %[temp2]    \n\t"
        "sll                %[mask1],       %[mask1],       24          \n\t"
        "addq.ph            %[temp1],       %[temp1],       %[temp7]    \n\t"
        "shra.ph            %[temp1],       %[temp1],       3           \n\t"
        "subq.ph            %[temp7],       %[temp1],       %[tc]       \n\t"
        "addq.ph            %[temp1],       %[temp1],       %[tc]       \n\t"
        "absq_s.ph          %[temp7],       %[temp7]                    \n\t"
        "absq_s.ph          %[temp1],       %[temp1]                    \n\t"
        "subqh.ph           %[temp1],       %[temp1],       %[temp7]    \n\t"
        "wrdsp              %[mask1]                                    \n\t"
        "subq.ph            %[temp2],       %[temp5],       %[temp1]    \n\t"
        "addq.ph            %[temp4],       %[temp1],       %[temp4]    \n\t"
        "precr.qb.ph        %[temp3],       %[temp4],       %[temp2]    \n\t"
        "sra                %[temp5],       %[temp3],       16          \n\t"
        "pick.qb            %[temp3],       %[temp3],       %[q0]       \n\t"
        "pick.qb            %[temp5],       %[temp5],       %[p0]       \n\t"
        "sh                 %[temp3],       0(%[data])                  \n\t"
        "subu               %[temp1],       %[data],        %[width]    \n\t"
        "sh                 %[temp5],       0(%[temp1])                 \n\t"

        : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2), [temp3] "=&r" (temp3),
          [temp4] "=&r" (temp4), [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
          [temp7] "=&r" (temp7), [mask1] "=&r" (mask1), [q0] "=&r" (q0),
          [p0] "=&r" (p0)
        : [alpha] "r" (alpha), [beta] "r" (beta), [tc] "r" (tc),
          [data] "r" (data), [width] "r" (width)
        : "memory"
    );
}


/*------------------------------------------------------------------------------

    Function: FilterHorChroma

        Functional description:
            Filter all four successive horizontal 2-pixel chroma edges. This
            can be done if bS is equal for all four edges.

------------------------------------------------------------------------------*/
void FilterHorChroma(
  u8 *data,
  u32 bS,
  edgeThreshold_t *thresholds,
  i32 width)
{

/* Variables */

    i32 tc;
    u32 i;
    u32 alpha = thresholds->alpha, beta = thresholds->beta;

/* Code */

    ASSERT(data);
    ASSERT(bS <= 4);
    ASSERT(thresholds);
    {
        u8 *pp = data;
        u8 *pp1;

        __asm__ volatile (
            "pref   7,      0(%[pp])                \n\t"       /* prefetch for store retained for the whole loop that follows */
            "pref   7,      32(%[pp])               \n\t"       /* to make sure all the data is prefetched */
            "subu   %[pp1], %[pp],      %[width]    \n\t"
            "pref   7,      0(%[pp1])               \n\t"       /* prefetch for store retained for the whole loop that follows */
            "pref   7,      32(%[pp1])              \n\t"       /* to make sure all the data is prefetched */
            "subu   %[pp1], %[pp1],     %[width]    \n\t"
            "pref   6,      0(%[pp1])               \n\t"       /* prefetch for load retained for the whole loop that follows */
            "pref   6,      32(%[pp1])              \n\t"       /* to make sure all the data is prefetched */
            "addu   %[pp1], %[pp],      %[width]    \n\t"
            "pref   6,      0(%[pp1])               \n\t"       /* prefetch for load retained for the whole loop that follows */
            "pref   6,      32(%[pp1])              \n\t"       /* to make sure all the data is prefetched */

            : [pp1] "=&r" (pp1)
            : [pp] "r" (pp), [width] "r" (width)
            : "memory"
        );
    }

    __asm__ volatile (
        "replv.qb       %[alpha],     %[alpha]         \n\t"
        "replv.qb       %[beta],      %[beta]          \n\t"

        : [alpha] "+r" (alpha), [beta] "+r" (beta)
        :
    );

    if (bS < 4)
    {
        tc = thresholds->tc0[bS-1]+1;

        __asm__ volatile (
            "replv.ph           %[tc],      %[tc]                      \n\t"

            :[tc] "+r" (tc)
            :
        );
        for (i = 8; i; i=i-4, data=data+4)
        {
            u32 mask1;
            u32 p0, q0, p1, q1;
            u32 temp1,temp2,temp3,temp4;
            u32 temp5,temp6,temp7,temp8;
            u32 temp10,temp11;

            __asm__ volatile (
                "subu            %[temp1],   %[data],     %[width]      \n\t"
                "subu            %[temp2],   %[temp1],    %[width]      \n\t"
                "addu            %[temp3],   %[data],     %[width]      \n\t"
                "lw              %[q0],      0(%[data])                 \n\t"
                "lw              %[p0],      0(%[temp1])                \n\t"
                "lw              %[p1],      0(%[temp2])                \n\t"
                "lw              %[q1],      0(%[temp3])                \n\t"
                "subu_s.qb       %[temp1],   %[p0],       %[q0]         \n\t"
                "subu_s.qb       %[temp7],   %[q0],       %[p0]         \n\t"
                "subu_s.qb       %[temp2],   %[p1],       %[p0]         \n\t"
                "subu_s.qb       %[temp8],   %[p0],       %[p1]         \n\t"
                "subu_s.qb       %[temp3],   %[q1],       %[q0]         \n\t"
                "subu_s.qb       %[temp11],  %[q0],       %[q1]         \n\t"
                "or              %[temp1],   %[temp1],    %[temp7]      \n\t"
                "or              %[temp2],   %[temp2],    %[temp8]      \n\t"
                "or              %[temp3],   %[temp3],    %[temp11]     \n\t"
                "cmpgu.lt.qb     %[mask1],   %[temp1],    %[alpha]      \n\t"
                "cmpgu.lt.qb     %[temp2],   %[temp2],    %[beta]       \n\t"
                "cmpgu.lt.qb     %[temp3],   %[temp3],    %[beta]       \n\t"
                "and             %[mask1],   %[mask1],    %[temp2]      \n\t"
                "and             %[mask1],   %[mask1],    %[temp3]      \n\t"
                "preceu.ph.qbr   %[temp1],   %[p0]                      \n\t"
                "preceu.ph.qbr   %[temp4],   %[q0]                      \n\t"
                "preceu.ph.qbr   %[temp2],   %[p1]                      \n\t"
                "preceu.ph.qbr   %[temp3],   %[q1]                      \n\t"
                "subq.ph         %[temp5],   %[temp4],    %[temp1]      \n\t"
                "subq.ph         %[temp6],   %[temp2],    %[temp3]      \n\t"
                "shll.ph         %[temp5],   %[temp5],    2             \n\t"
                "preceu.ph.qbl   %[temp10],  %[p0]                      \n\t"
                "addq.ph         %[temp5],   %[temp5],    %[temp6]      \n\t"
                "preceu.ph.qbl   %[temp11],  %[q0]                      \n\t"
                "shra_r.ph       %[temp5],   %[temp5],    3             \n\t"
                "preceu.ph.qbl   %[temp2],   %[p1]                      \n\t"
                "preceu.ph.qbl   %[temp3],   %[q1]                      \n\t"
                "subq.ph         %[temp6],   %[tc],       %[temp5]      \n\t"
                "addq.ph         %[temp5],   %[tc],       %[temp5]      \n\t"
                "absq_s.ph       %[temp6],   %[temp6]                   \n\t"
                "absq_s.ph       %[temp5],   %[temp5]                   \n\t"
                "subq.ph         %[temp7],   %[temp11],   %[temp10]     \n\t"
                "subq.ph         %[temp8],   %[temp2],    %[temp3]      \n\t"
                "subqh.ph        %[temp5],   %[temp5],    %[temp6]      \n\t"
                "shll.ph         %[temp7],   %[temp7],    2             \n\t"
                "addq.ph         %[temp6],   %[temp5],    %[temp1]      \n\t"
                "addq.ph         %[temp7],   %[temp7],    %[temp8]      \n\t"
                "subq.ph         %[temp5],   %[temp4],    %[temp5]      \n\t"
                "shra_r.ph       %[temp7],   %[temp7],    3             \n\t"
                "shll_s.ph       %[temp6],   %[temp6],    7             \n\t"
                "shll_s.ph       %[temp5],   %[temp5],    7             \n\t"
                "subq.ph         %[temp1],   %[tc],       %[temp7]      \n\t"
                "addq.ph         %[temp4],   %[tc],       %[temp7]      \n\t"
                "absq_s.ph       %[temp1],   %[temp1]                   \n\t"
                "absq_s.ph       %[temp4],   %[temp4]                   \n\t"
                "sll             %[mask1],   %[mask1],    24            \n\t"
                "wrdsp           %[mask1]                               \n\t"
                "subqh.ph        %[temp4],   %[temp4],    %[temp1]      \n\t"
                "addq.ph         %[temp8],   %[temp4],    %[temp10]     \n\t"
                "subq.ph         %[temp7],   %[temp11],   %[temp4]      \n\t"
                "shll_s.ph       %[temp8],   %[temp8],    7             \n\t"
                "shll_s.ph       %[temp7],   %[temp7],    7             \n\t"
                "sub             %[temp1],   %[data],     %[width]      \n\t"
                "precrqu_s.qb.ph %[temp8],   %[temp8],    %[temp6]      \n\t"
                "precrqu_s.qb.ph %[temp7],   %[temp7],    %[temp5]      \n\t"
                "pick.qb         %[temp7],   %[temp7],    %[q0]         \n\t"
                "pick.qb         %[temp8],   %[temp8],    %[p0]         \n\t"
                "sw              %[temp7],   0(%[data])                 \n\t"
                "sw              %[temp8],   0(%[temp1])                \n\t"

                : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                  [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
                  [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
                  [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
                  [temp10] "=&r" (temp10), [temp11] "=&r" (temp11),
                  [mask1] "=&r" (mask1), [q0] "=&r" (q0), [p0] "=&r" (p0),
                  [p1] "=&r" (p1), [q1] "=&r" (q1)
                : [alpha] "r" (alpha), [beta] "r" (beta), [tc] "r" (tc),
                  [data] "r" (data),[width] "r" (width)
                : "memory"
            );
        }
    }
    else
    {
        for (i = 8; i; i=i-4, data=data+4)
        {
            u32 mask1;
            u32 p0, q0, p1, q1;
            u32 temp1,temp2,temp3,temp4;
            u32 temp5,temp6,temp7,temp8;
            u32 temp10,temp11;

            __asm__ volatile (
                "lw             %[q0],     0(%[data])                \n\t"
                "subu           %[temp1],  %[data],     %[width]     \n\t"
                "lw             %[p0],     0(%[temp1])               \n\t"
                "subu           %[temp1],  %[temp1],    %[width]     \n\t"
                "lw             %[p1],     0(%[temp1])               \n\t"
                "addu           %[temp1],  %[data],     %[width]     \n\t"
                "lw             %[q1],     0(%[temp1])               \n\t"
                "subu_s.qb      %[temp1],  %[p0],       %[q0]        \n\t"
                "subu_s.qb      %[temp7],  %[q0],       %[p0]        \n\t"
                "subu_s.qb      %[temp2],  %[p1],       %[p0]        \n\t"
                "subu_s.qb      %[temp8],  %[p0],       %[p1]        \n\t"
                "subu_s.qb      %[temp3],  %[q1],       %[q0]        \n\t"
                "subu_s.qb      %[temp11], %[q0],       %[q1]        \n\t"
                "or             %[temp1],  %[temp1],    %[temp7]     \n\t"
                "or             %[temp2],  %[temp2],    %[temp8]     \n\t"
                "or             %[temp3],  %[temp3],    %[temp11]    \n\t"
                "cmpgu.lt.qb    %[mask1],  %[temp1],    %[alpha]     \n\t"
                "cmpgu.lt.qb    %[temp2],  %[temp2],    %[beta]      \n\t"
                "cmpgu.lt.qb    %[temp3],  %[temp3],    %[beta]      \n\t"
                "and            %[mask1],  %[mask1],    %[temp2]     \n\t"
                "and            %[mask1],  %[mask1],    %[temp3]     \n\t"
                "preceu.ph.qbr  %[temp1],  %[p0]                     \n\t"
                "preceu.ph.qbr  %[temp4],  %[q0]                     \n\t"
                "preceu.ph.qbr  %[temp2],  %[p1]                     \n\t"
                "preceu.ph.qbr  %[temp3],  %[q1]                     \n\t"
                "shll.ph        %[temp5],  %[temp2],    1            \n\t"
                "shll.ph        %[temp6],  %[temp3],    1            \n\t"
                "addq.ph        %[temp7],  %[temp1],    %[temp3]     \n\t"
                "addq.ph        %[temp8],  %[temp4],    %[temp2]     \n\t"
                "addq.ph        %[temp5],  %[temp5],    %[temp7]     \n\t"
                "addq.ph        %[temp6],  %[temp6],    %[temp8]     \n\t"
                "shra_r.ph      %[temp10], %[temp5],    2            \n\t"
                "shra_r.ph      %[temp11], %[temp6],    2            \n\t"
                "preceu.ph.qbl  %[temp1],  %[p0]                     \n\t"
                "preceu.ph.qbl  %[temp4],  %[q0]                     \n\t"
                "preceu.ph.qbl  %[temp2],  %[p1]                     \n\t"
                "preceu.ph.qbl  %[temp3],  %[q1]                     \n\t"
                "shll.ph        %[temp5],  %[temp2],    1            \n\t"
                "shll.ph        %[temp6],  %[temp3],    1            \n\t"
                "addq.ph        %[temp7],  %[temp1],    %[temp3]     \n\t"
                "addq.ph        %[temp8],  %[temp4],    %[temp2]     \n\t"
                "addq.ph        %[temp5],  %[temp5],    %[temp7]     \n\t"
                "addq.ph        %[temp6],  %[temp6],    %[temp8]     \n\t"
                "shra_r.ph      %[temp5],  %[temp5],    2            \n\t"
                "shra_r.ph      %[temp6],  %[temp6],    2            \n\t"
                "sll            %[mask1],  %[mask1],    24           \n\t"
                "wrdsp          %[mask1]                             \n\t"
                "precr.qb.ph    %[temp5],  %[temp5],    %[temp10]    \n\t"
                "precr.qb.ph    %[temp6],  %[temp6],    %[temp11]    \n\t"
                "pick.qb        %[temp5],  %[temp5],    %[p0]        \n\t"
                "pick.qb        %[temp6],  %[temp6],    %[q0]        \n\t"
                "subu           %[temp1],  %[data],     %[width]     \n\t"
                "sw             %[temp6],  0(%[data])                \n\t"
                "sw             %[temp5],  0(%[temp1])               \n\t"

                : [temp1] "=&r" (temp1), [temp2] "=&r" (temp2),
                  [temp3] "=&r" (temp3), [temp4] "=&r" (temp4),
                  [temp5] "=&r" (temp5), [temp6] "=&r" (temp6),
                  [temp7] "=&r" (temp7), [temp8] "=&r" (temp8),
                  [temp10] "=&r" (temp10), [temp11] "=&r" (temp11),
                  [mask1] "=&r" (mask1), [q0] "=&r" (q0),
                  [p0] "=&r" (p0), [p1] "=&r" (p1), [q1] "=&r" (q1)
                : [alpha] "r" (alpha), [beta] "r" (beta),
                  [data] "r" (data), [width] "r" (width)
                : "memory"
              );
        }
    }
}


/*------------------------------------------------------------------------------

    Function: GetBoundaryStrengths

        Functional description:
            Function to calculate boundary strengths for all edges of a
            macroblock. Function returns HANTRO_TRUE if any of the bS values for
            the macroblock had non-zero value, HANTRO_FALSE otherwise.

------------------------------------------------------------------------------*/
static u32 GetBoundaryStrengths(mbStorage_t *mb,  u32 flags, u8 (*bS_new)[16])
{

     /*this flag is set HANTRO_TRUE as soon as any boundary strength value is
      * non-zero*/
    u32 nonZeroBs = HANTRO_FALSE;
    u32 *pTmp;
    u32 isIntraMb;
    u32 tmp1, tmp2, tmp3, tmp4;
/* Code */

    ASSERT(mb);
    ASSERT(bS);
    ASSERT(flags);

    isIntraMb = IS_INTRA_MB(*mb);

    /* top edges */
    pTmp = (u32*)&bS_new[1][0];
    if (flags & FILTER_TOP_EDGE)
    {
        if (isIntraMb || IS_INTRA_MB(*mb->mbB))
        {
            *pTmp = 0x04040404;
            nonZeroBs = HANTRO_TRUE;
        }
        else
        {
            *pTmp = EdgeBoundaryStrengthTop(mb, mb->mbB);
            if (*pTmp)
                nonZeroBs = HANTRO_TRUE;
        }
    }
    else
    {
        *pTmp = 0;
    }

    /* left edges */
    pTmp = (u32*)&bS_new[0][0];
    if (flags & FILTER_LEFT_EDGE)
    {
        if (isIntraMb || IS_INTRA_MB(*mb->mbA))
        {
            *pTmp = 0x04040404;
            nonZeroBs = HANTRO_TRUE;
        }
        else
        {
            *pTmp = EdgeBoundaryStrengthLeft(mb, mb->mbA);
            if (!nonZeroBs && *pTmp)
                nonZeroBs = HANTRO_TRUE;
        }
    }
    else
    {
        *pTmp = 0;
    }
    /* inner edges */
    if (IS_INTRA_MB(*mb))
    {
        pTmp = (u32*)&bS_new[0][0];
        pTmp[1] = pTmp[2] = pTmp[3] = pTmp[5] = pTmp[6] = pTmp[7] = 0x03030303;
        nonZeroBs = HANTRO_TRUE;
    }
    else
    {
        pTmp = (u32*)mb->totalCoeff;
        u32 pom;
        /* 16x16 inter mb -> ref addresses or motion vectors cannot differ,
         * only check if either of the blocks contain coefficients */
        if (h264bsdNumMbPart(mb->mbType) == 1)
        {
            tmp1 = *pTmp++; tmp2 = *pTmp++; tmp3 = *pTmp++; tmp4 = *pTmp++;
            pom = tmp1 | tmp2;
            bS_new[0][4]= (tmp1) ? 2 : 0; /* [1] || [0] */
            bS_new[0][5]= (tmp2) ? 2 : 0; /* [3] || [2] */
            bS_new[0][8]= (tmp3 & 0x0000ffff) || (tmp1 & 0xffff0000) ? 2 : 0; /* [4] || [1] */
            bS_new[0][12]= (tmp3) ? 2 : 0; /* [5] || [4] */
            bS_new[1][4]= (pom & 0x0000ffff)? 2 : 0; /* [2] || [0] */
            bS_new[1][5]= (pom & 0xffff0000) ? 2 : 0; /* [3] || [1] */
            pom = tmp3 | tmp4;
            bS_new[0][9]= (tmp4 & 0x0000ffff) || (tmp2 & 0xffff0000) ? 2 : 0; /* [6] || [3] */
            bS_new[0][13]= (tmp4) ? 2 : 0; /* [7] || [6] */
            bS_new[1][6]= (pom & 0x0000ffff) ? 2 : 0; /* [6] || [4] */
            bS_new[1][7]= (pom & 0xffff0000) ? 2 : 0; /* [7] || [5] */
            tmp1 = *pTmp++; pom = tmp1 | tmp2;
            bS_new[0][6]= (tmp1) ? 2 : 0; /* [9] || [8] */
            bS_new[1][8]= (pom & 0x0000ffff) ? 2 : 0; /* [8] || [2] */
            bS_new[1][9]= (pom & 0xffff0000) ? 2 : 0; /* [9] || [3] */
            tmp2 = *pTmp++; tmp3 = *pTmp++; pom = tmp3 | tmp4;
            bS_new[0][10]= (tmp3 & 0x0000ffff) || (tmp1 & 0xffff0000) ? 2 : 0; /* [12] || [9] */
            bS_new[0][14]= (tmp3) ? 2 : 0; /* [13] || [12] */
            bS_new[1][10]= (pom & 0x0000ffff) ? 2 : 0; /* [12] || [6] */
            bS_new[1][11]= (pom & 0xffff0000) ? 2 : 0; /* [13] || [7] */
            pom = tmp1 | tmp2;
            bS_new[0][7]= (tmp2) ? 2 : 0; /* [11] || [10] */
            bS_new[1][12]= (pom & 0x0000ffff) ? 2 : 0; /* [10] || [8] */
            bS_new[1][13]= (pom & 0xffff0000) ? 2 : 0; /* [11] || [9] */
            tmp4 = *pTmp; pom = tmp3 | tmp4;
            bS_new[0][11]= (tmp4 & 0x0000ffff) || (tmp2 & 0xffff0000) ? 2 : 0; /* [14] || [11] */
            bS_new[0][15]= (tmp4) ? 2 : 0; /* [15] || [14] */
            bS_new[1][14]= (pom & 0x0000ffff) ? 2 : 0; /* [14] || [12] */
            bS_new[1][15]= (pom & 0xffff0000) ? 2 : 0; /* [15] || [13] */
        }
        /* 16x8 inter mb -> ref addresses and motion vectors can be different
         * only for the middle horizontal edge, for the other top edges it is
         * enough to check whether the blocks contain coefficients or not. The
         * same applies to all internal left edges. */
        else if (mb->mbType == P_L0_L0_16x8)
        {
            tmp1 = *pTmp++; tmp2 = *pTmp++; tmp3 = *pTmp++; tmp4 = *pTmp++;
            pom = tmp1 | tmp2;
            bS_new[0][4]= (tmp1) ? 2 : 0; /* [1] || [0] */
            bS_new[0][5]= (tmp2) ? 2 : 0; /* [3] || [2] */
            bS_new[0][8]= (tmp3 & 0x0000ffff) || (tmp1 & 0xffff0000) ? 2 : 0; /* [4] || [1] */
            bS_new[0][12]= (tmp3) ? 2 : 0; /* [5] || [4] */
            bS_new[1][4]= (pom & 0x0000ffff) ? 2 : 0; /* [2] || [0] */
            bS_new[1][5]= (pom & 0xffff0000) ? 2 : 0; /* [3] || [1] */
            pom = tmp3 | tmp4;
            bS_new[0][9]= (tmp4 & 0x0000ffff) || (tmp2 & 0xffff0000) ? 2 : 0; /* [6] || [3] */
            bS_new[0][13]= (tmp4) ? 2 : 0; /* [7] || [6] */
            bS_new[1][6]= (pom & 0x0000ffff) ? 2 : 0; /* [6] || [4] */
            bS_new[1][7]= (pom & 0xffff0000) ? 2 : 0; /* [7] || [5] */
            tmp1 = *pTmp++; tmp2 = *pTmp++; tmp3 = *pTmp++;
            bS_new[0][6]= (tmp1) ? 2 : 0; /* [9] || [8] */
            bS_new[0][10]= (tmp3 & 0x0000ffff) || (tmp1 & 0xffff0000) ? 2 : 0; /* [12] || [9] */
            bS_new[0][14]= (tmp3) ? 2 : 0; /* [13] || [12] */
            pom = tmp1 | tmp2;
            bS_new[0][7]= (tmp2) ? 2 : 0; /* [11] || [10] */
            bS_new[1][12]= (pom & 0x0000ffff) ? 2 : 0; /* [10] || [8] */
            bS_new[1][13]= (pom & 0xffff0000) ? 2 : 0; /* [11] || [9] */
            tmp4 = *pTmp;
            pom = tmp3 | tmp4;
            bS_new[0][11]= (tmp4 & 0x0000ffff) || (tmp2 & 0xffff0000) ? 2 : 0; /* [14] || [11] */
            bS_new[0][15]= (tmp4) ? 2 : 0; /* [15] || [14] */
            bS_new[1][14]= (pom & 0x0000ffff) ? 2 : 0; /* [14] || [12] */
            bS_new[1][15]= (pom & 0xffff0000) ? 2 : 0; /* [15] || [13] */

            InnerBoundaryStrengthFour(mb, 8, 2, 9, 3, 12, 6, 13, 7, &bS_new[1][8]);
        }
        /* 8x16 inter mb -> ref addresses and motion vectors can be different
         * only for the middle vertical edge, for the other left edges it is
         * enough to check whether the blocks contain coefficients or not. The
         * same applies to all internal top edges. */
        else if (mb->mbType == P_L0_L0_8x16)
        {
            tmp1 = *pTmp++; tmp2 = *pTmp++; tmp3 = *pTmp++; tmp4 = *pTmp++;
            pom = tmp1 | tmp2;
            bS_new[0][4]= (tmp1) ? 2 : 0; /* [1] || [0] */
            bS_new[0][5]= (tmp2) ? 2 : 0; /* [3] || [2] */
            bS_new[0][12]= (tmp3) ? 2 : 0; /* [5] || [4] */
            bS_new[1][4]= (pom & 0x0000ffff) ? 2 : 0; /* [2] || [0] */
            bS_new[1][5]= (pom & 0xffff0000) ? 2 : 0; /* [3] || [1] */
            pom = tmp3 | tmp4;
            bS_new[0][13]= (tmp4) ? 2 : 0; /* [7] || [6] */
            bS_new[1][6]= (pom & 0x0000ffff) ? 2 : 0; /* [6] || [4] */
            bS_new[1][7]= (pom & 0xffff0000) ? 2 : 0; /* [7] || [5] */
            tmp1 = *pTmp++; pom = tmp1 | tmp2;
            bS_new[0][6]= (tmp1) ? 2 : 0; // [9]  || [8]
            bS_new[1][8]= (pom & 0x0000ffff) ? 2 : 0; // [8]  || [2]
            bS_new[1][9]= (pom & 0xffff0000) ? 2 : 0; /* [9] || [8] */
            tmp2 = *pTmp++; tmp3 = *pTmp++; pom = tmp3 | tmp4;
            bS_new[0][14]= (tmp3) ? 2 : 0; /* [13] || [12] */
            bS_new[1][10]= (pom & 0x0000ffff) ? 2 : 0; /* [12] || [6] */
            bS_new[1][11]= (pom & 0xffff0000) ? 2 : 0; /* [13] || [7] */
            pom = tmp1 | tmp2;
            bS_new[0][7]= (tmp2) ? 2 : 0; /* [11] || [10] */
            bS_new[1][12]= (pom & 0x0000ffff) ? 2 : 0; /* [10] || [8] */
            bS_new[1][13]= (pom & 0xffff0000) ? 2 : 0; /* [11] || [9] */
            tmp4 = *pTmp; pom = tmp3 | tmp4;
            bS_new[0][15]= (tmp4) ? 2 : 0; /* [15] || [14] */
            bS_new[1][14]= (pom & 0x0000ffff) ? 2 : 0; /* [14] || [12] */
            bS_new[1][15]= (pom & 0xffff0000) ? 2 : 0; /* [15] || [13] */

            InnerBoundaryStrengthFour(mb, 4, 1, 6, 3, 12, 9, 14, 11, &bS_new[0][8]);
        }
        else
        {
            InnerBoundaryStrengthFour(mb, 2, 0, 3, 1, 6, 4, 7, 5, &bS_new[1][4]);
            InnerBoundaryStrengthFour(mb, 8, 2, 9, 3, 12, 6, 13, 7, &bS_new[1][8]);
            InnerBoundaryStrengthFour(mb, 10, 8, 11, 9, 14, 12, 15, 13, &bS_new[1][12]);

            InnerBoundaryStrengthFour(mb, 1, 0, 3, 2, 9, 8, 10, 11, &bS_new[0][4]);
            InnerBoundaryStrengthFour(mb, 4, 1, 6, 3, 12, 9, 14, 11, &bS_new[0][8]);
            InnerBoundaryStrengthFour(mb, 5, 4, 7, 6, 13, 12, 15, 14, &bS_new[0][12]);
        }

        pTmp = (u32*)&bS_new[0][0];
        if (!nonZeroBs && (pTmp[1] || pTmp[2] || pTmp[3] ||
                           pTmp[5] || pTmp[6] || pTmp[7]) )
            nonZeroBs = HANTRO_TRUE;
    }
    return(nonZeroBs);
}

/*------------------------------------------------------------------------------

    Function: GetLumaEdgeThresholds

        Functional description:
            Compute alpha, beta and tc0 thresholds for inner, left and top
            luma edges of a macroblock.

------------------------------------------------------------------------------*/
void GetLumaEdgeThresholds(
  edgeThreshold_t *thresholds,
  mbStorage_t *mb,
  u32 filteringFlags)
{

/* Variables */

    u32 indexA, indexB;
    u32 qpAv, qp, qpTmp;

/* Code */

    ASSERT(thresholds);
    ASSERT(mb);

    qp = mb->qpY;

    indexA = (u32)CLIP3(0, 51, (i32)qp + mb->filterOffsetA);
    indexB = (u32)CLIP3(0, 51, (i32)qp + mb->filterOffsetB);

    thresholds[INNER].alpha = alphas[indexA];
    thresholds[INNER].beta = betas[indexB];
    thresholds[INNER].tc0 = tc0[indexA];

    if (filteringFlags & FILTER_TOP_EDGE)
    {
        qpTmp = mb->mbB->qpY;
        if (qpTmp != qp)
        {
            qpAv = (qp + qpTmp + 1) >> 1;

            indexA = (u32)CLIP3(0, 51, (i32)qpAv + mb->filterOffsetA);
            indexB = (u32)CLIP3(0, 51, (i32)qpAv + mb->filterOffsetB);

            thresholds[TOP].alpha = alphas[indexA];
            thresholds[TOP].beta = betas[indexB];
            thresholds[TOP].tc0 = tc0[indexA];
        }
        else
        {
            thresholds[TOP].alpha = thresholds[INNER].alpha;
            thresholds[TOP].beta = thresholds[INNER].beta;
            thresholds[TOP].tc0 = thresholds[INNER].tc0;
        }
    }
    if (filteringFlags & FILTER_LEFT_EDGE)
    {
        qpTmp = mb->mbA->qpY;
        if (qpTmp != qp)
        {
            qpAv = (qp + qpTmp + 1) >> 1;

            indexA = (u32)CLIP3(0, 51, (i32)qpAv + mb->filterOffsetA);
            indexB = (u32)CLIP3(0, 51, (i32)qpAv + mb->filterOffsetB);

            thresholds[LEFT].alpha = alphas[indexA];
            thresholds[LEFT].beta = betas[indexB];
            thresholds[LEFT].tc0 = tc0[indexA];
        }
        else
        {
            thresholds[LEFT].alpha = thresholds[INNER].alpha;
            thresholds[LEFT].beta = thresholds[INNER].beta;
            thresholds[LEFT].tc0 = thresholds[INNER].tc0;
        }
    }

}

/*------------------------------------------------------------------------------

    Function: GetChromaEdgeThresholds

        Functional description:
            Compute alpha, beta and tc0 thresholds for inner, left and top
            chroma edges of a macroblock.

------------------------------------------------------------------------------*/
void GetChromaEdgeThresholds(
  edgeThreshold_t *thresholds,
  mbStorage_t *mb,
  u32 filteringFlags,
  i32 chromaQpIndexOffset)
{

/* Variables */

    u32 indexA, indexB;
    u32 qpAv, qp, qpTmp;

/* Code */

    ASSERT(thresholds);
    ASSERT(mb);

    qp = mb->qpY;
    qp = h264bsdQpC[CLIP3(0, 51, (i32)qp + chromaQpIndexOffset)];

    indexA = (u32)CLIP3(0, 51, (i32)qp + mb->filterOffsetA);
    indexB = (u32)CLIP3(0, 51, (i32)qp + mb->filterOffsetB);

    thresholds[INNER].alpha = alphas[indexA];
    thresholds[INNER].beta = betas[indexB];
    thresholds[INNER].tc0 = tc0[indexA];

    if (filteringFlags & FILTER_TOP_EDGE)
    {
        qpTmp = mb->mbB->qpY;
        if (qpTmp != mb->qpY)
        {
            qpTmp = h264bsdQpC[CLIP3(0, 51, (i32)qpTmp + chromaQpIndexOffset)];
            qpAv = (qp + qpTmp + 1) >> 1;

            indexA = (u32)CLIP3(0, 51, (i32)qpAv + mb->filterOffsetA);
            indexB = (u32)CLIP3(0, 51, (i32)qpAv + mb->filterOffsetB);

            thresholds[TOP].alpha = alphas[indexA];
            thresholds[TOP].beta = betas[indexB];
            thresholds[TOP].tc0 = tc0[indexA];
        }
        else
        {
            thresholds[TOP].alpha = thresholds[INNER].alpha;
            thresholds[TOP].beta = thresholds[INNER].beta;
            thresholds[TOP].tc0 = thresholds[INNER].tc0;
        }
    }
    if (filteringFlags & FILTER_LEFT_EDGE)
    {
        qpTmp = mb->mbA->qpY;
        if (qpTmp != mb->qpY)
        {
            qpTmp = h264bsdQpC[CLIP3(0, 51, (i32)qpTmp + chromaQpIndexOffset)];
            qpAv = (qp + qpTmp + 1) >> 1;

            indexA = (u32)CLIP3(0, 51, (i32)qpAv + mb->filterOffsetA);
            indexB = (u32)CLIP3(0, 51, (i32)qpAv + mb->filterOffsetB);

            thresholds[LEFT].alpha = alphas[indexA];
            thresholds[LEFT].beta = betas[indexB];
            thresholds[LEFT].tc0 = tc0[indexA];
        }
        else
        {
            thresholds[LEFT].alpha = thresholds[INNER].alpha;
            thresholds[LEFT].beta = thresholds[INNER].beta;
            thresholds[LEFT].tc0 = thresholds[INNER].tc0;
        }
    }

}

/*------------------------------------------------------------------------------

    Function: FilterLuma

        Functional description:
            Function to filter all luma edges of a macroblock

------------------------------------------------------------------------------*/
void FilterLuma(
  u8 *data,
  u8 (*bS_new)[16],
  edgeThreshold_t *thresholds,
  u32 width)
{
    u32 vblock;
    u8 *ptr;
    u32 offset;
    edgeThreshold_t *threshold;
    u8 *tmp_l = &bS_new[0][0];
    u8 *tmp = &bS_new[1][0];

    ASSERT(data);
    ASSERT(bS);
    ASSERT(thresholds);

    ptr = data;

    {
        u8 *data1 = data;

        /* prefetching all needed data for  FilterVerLumaEdges and FilterVerLumaEdgeL
         * functions that follows */
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            "addu    %[data1],  %[data1],        %[width]       \n\t"
            "pref    1,         0(%[data1])                     \n\t"
            ".set pop                                           \n\t"

            : [data1] "+r" (data1)
            : [width] "r" (width)
            : "memory"
        );
    }

    threshold = thresholds + LEFT;

    if(tmp_l[0] == tmp_l[1] && tmp_l[1] == tmp_l[2] && tmp_l[2] == tmp_l[3] && tmp_l[0] < 4) {
        if (tmp_l[0]) {
            FilterVerLumaEdges(ptr, tmp_l[0], threshold, width);
        }
    }
    else {
        if (tmp_l[0])
            FilterVerLumaEdgeL(ptr, tmp_l[0], threshold, width);
        if (tmp_l[1])
            FilterVerLumaEdgeL(ptr+width*4, tmp_l[1], threshold, width);
        if (tmp_l[2])
            FilterVerLumaEdgeL(ptr+width*8, tmp_l[2], threshold, width);
        if (tmp_l[3])
            FilterVerLumaEdgeL(ptr+width*12, tmp_l[3], threshold, width);
    }

    ptr += 4;
    tmp_l+=4;

    threshold = thresholds + INNER;
    for (vblock = 3; vblock--;)
    {
        if(tmp_l[0] == tmp_l[1] && tmp_l[1] == tmp_l[2] && tmp_l[2] == tmp_l[3]) {
            if (tmp_l[0]) {
                FilterVerLumaEdges(ptr, tmp_l[0], threshold, width);
            }
        }
        else {
            if (tmp_l[0])
                FilterVerLumaEdge(ptr, tmp_l[0], threshold, width);
            if (tmp_l[1])
                FilterVerLumaEdge(ptr+width*4, tmp_l[1], threshold, width);
            if (tmp_l[2])
                FilterVerLumaEdge(ptr+width*8, tmp_l[2], threshold, width);
            if (tmp_l[3])
                FilterVerLumaEdge(ptr+width*12, tmp_l[3], threshold, width);
        }

        ptr += 4;
        tmp_l+=4;
    }

    offset = TOP;
    ptr = data;

    for (vblock = 4; vblock--;)
    {
        /* if bS is equal for all horizontal edges of the row -> perform
         * filtering with FilterHorLuma, otherwise use FilterHorLumaEdge for
         * each edge separately. offset variable indicates top macroblock edge
         * on the first loop round, inner edge for the other rounds */
        if (tmp[0] == tmp[1] && tmp[1] == tmp[2] &&
            tmp[2] == tmp[3])
        {
            if(tmp[0])
            {
                {
                    u8 *pp = ptr + 4*width;
                    u8 *pp1;
                    /* prefetch for store retained for the FilterHorLuma function that follows */
                    __asm__ volatile (
                        "pref       7,      32(%[pp])                      \n\t"
                        "subu       %[pp1], %[pp],      %[width]           \n\t"
                        "pref       7,      32(%[pp1])                     \n\t"
                        "subu       %[pp1], %[pp1],     %[width]           \n\t"
                        "pref       7,      32(%[pp1])                     \n\t"
                        "subu       %[pp1], %[pp1],     %[width]           \n\t"
                        "pref       7,      32(%[pp1])                     \n\t"
                        "subu       %[pp1], %[pp1],     %[width]           \n\t"
                        "addu       %[pp1], %[pp],      %[width]           \n\t"
                        "pref       7,      32(%[pp1])                     \n\t"
                        "addu       %[pp1], %[pp1],     %[width]           \n\t"
                        "pref       7,      32(%[pp1])                     \n\t"

                        : [pp1] "=&r" (pp1)
                        : [pp] "r" (pp), [width] "r" (width)
                        : "memory"
                    );
                }

                FilterHorLuma(ptr, tmp[0], thresholds + offset, (i32)width);
            }
        }
        else
        {
            {
                u8 *pp = ptr + width*4;
                u8 *pp1;
                /* prefetching all needed data for  FilterHorLumaEdge function */
                __asm__ volatile (
                    "pref   7,      0(%[pp])                \n\t"       /* prefetch for store retained for the FilterHorLumaEdges functions that follows */
                    "pref   7,      32(%[pp])               \n\t"       /* to make sure all the data is prefetched */
                    "subu   %[pp1], %[pp],      %[width]    \n\t"
                    "pref   7,      0(%[pp1])               \n\t"
                    "pref   7,      32(%[pp1])              \n\t"
                    "subu   %[pp1], %[pp1],     %[width]    \n\t"
                    "pref   7,      0(%[pp1])               \n\t"
                    "pref   7,      32(%[pp1])              \n\t"
                    "addu   %[pp1], %[pp],      %[width]    \n\t"
                    "pref   7,      0(%[pp1])               \n\t"
                    "pref   7,      32(%[pp1])              \n\t"
                    "addu   %[pp1], %[pp],      %[width]    \n\t"
                    "pref   7,      0(%[pp1])               \n\t"
                    "pref   7,      32(%[pp1])              \n\t"

                    : [pp1] "=&r" (pp1)
                    : [pp] "r" (pp), [width] "r" (width)
                    : "memory"
                );
            }
            if(tmp[0])
                FilterHorLumaEdge(ptr, tmp[0], thresholds+offset,
                    (i32)width);
            if(tmp[1])
                FilterHorLumaEdge(ptr+4, tmp[1], thresholds+offset,
                    (i32)width);
            if(tmp[2])
                FilterHorLumaEdge(ptr+8, tmp[2], thresholds+offset,
                    (i32)width);
            if(tmp[3])
                FilterHorLumaEdge(ptr+12, tmp[3], thresholds+offset,
                    (i32)width);
        }
        /* four pixel rows ahead, i.e. next row of 4x4-blocks */
        ptr += width*4;
        tmp += 4;
        offset = INNER;
    }
}


/*------------------------------------------------------------------------------

    Function: FilterChroma

        Functional description:
            Function to filter all chroma edges of a macroblock

------------------------------------------------------------------------------*/
void FilterChroma(
  u8 *dataCb,
  u8 *dataCr,
  u8 (*bS_new)[16],
  edgeThreshold_t *thresholds,
  u32 width)
{

/* Variables */

    u32 vblock;
    u32 offset;
    u8  *tmp = &bS_new[1][0];
    u8  *tmp_l = &bS_new[0][0];

/* Code */

    ASSERT(dataCb);
    ASSERT(dataCr);
    ASSERT(bS_new);
    ASSERT(thresholds);

    {
        u8  *data1 = dataCb;
        u8  *data2 = dataCr;

        /* prefetch for store for FilterVerChromaEdge, FilterVerChromaEdges_Left
         * and FilterHorChromaEdge functions that follows */

        __asm__ volatile (
            ".set push                                      \n\t"
            ".set noreorder                                 \n\t"
            /* prefetch data for FilterVerChroma... functions */
            "pref    1,         0(%[data1])                 \n\t"
            "addu    %[data1],  %[data1],        %[width]   \n\t"
            "pref    1,         0(%[data1])                 \n\t"
            "addu    %[data1],  %[data1],        %[width]   \n\t"
            "pref    1,         0(%[data1])                 \n\t"
            "addu    %[data1],  %[data1],        %[width]   \n\t"
            "pref    1,         0(%[data1])                 \n\t"
            "addu    %[data1],  %[data1],        %[width]   \n\t"
            "pref    1,         0(%[data1])                 \n\t"
            "addu    %[data1],  %[data1],        %[width]   \n\t"
            "pref    1,         0(%[data1])                 \n\t"
            "addu    %[data1],  %[data1],        %[width]   \n\t"
            "pref    1,         0(%[data1])                 \n\t"
            "addu    %[data1],  %[data1],        %[width]   \n\t"
            "pref    1,         0(%[data1])                 \n\t"
            "pref    1,         0(%[data2])                 \n\t"
            "addu    %[data2],  %[data2],        %[width]   \n\t"
            "pref    1,         0(%[data2])                 \n\t"
            "addu    %[data2],  %[data2],        %[width]   \n\t"
            "pref    1,         0(%[data2])                 \n\t"
            "addu    %[data2],  %[data2],        %[width]   \n\t"
            "pref    1,         0(%[data2])                 \n\t"
            "addu    %[data2],  %[data2],        %[width]   \n\t"
            "pref    1,         0(%[data2])                 \n\t"
            "addu    %[data2],  %[data2],        %[width]   \n\t"
            "pref    1,         0(%[data2])                 \n\t"
            "addu    %[data2],  %[data2],        %[width]   \n\t"
            "pref    1,         0(%[data2])                 \n\t"
            "addu    %[data2],  %[data2],        %[width]   \n\t"
            "pref    1,         0(%[data2])                 \n\t"
            /* prefetch data for FilterHorChroma... functions */
            "subu     %[data1],  %[dataCb],      %[width]   \n\t"
            "pref     1,         0(%[data1])                \n\t"
            "subu     %[data1],  %[data1],       %[width]   \n\t"
            "pref     1,         0(%[data1])                \n\t"
            "subu     %[data2],  %[dataCr],      %[width]   \n\t"
            "pref     1,         0(%[data2])                \n\t"
            "subu     %[data2],  %[data2],       %[width]   \n\t"
            "pref     1,         0(%[data2])                \n\t"
            ".set pop                                       \n\t"

            : [data1] "+r" (data1), [data2] "+r" (data2)
            : [width] "r" (width), [dataCb] "r" (dataCb),
              [dataCr] "r" (dataCr)
            : "memory"
        );
    }

    /* loop block rows, perform filtering for all vertical edges of the block
     * row first, then filter each horizontal edge of the row */
    /* only perform filtering if bS is non-zero, first two of the four
     * FilterVerChromaEdge calls handle the left edge of the macroblock,
     * others filter the inner edge. Note that as chroma uses bS values
     * determined for luma edges, each bS is used only for 2 pixels of
     * a 4-pixel edge */

    if (tmp_l[0] == tmp_l[1] && tmp_l[0] == tmp_l[2] &&
        tmp_l[0] == tmp_l[3])
    {
        if (tmp_l[0]) {
            FilterVerChromaEdges_Left(dataCb, dataCr, tmp_l[0], thresholds + LEFT, width);
        }
    } else {
        if (tmp_l[0])
        {
            FilterVerChromaEdge(dataCb, tmp_l[0], thresholds + LEFT, width);
            FilterVerChromaEdge(dataCr, tmp_l[0], thresholds + LEFT, width);
        }
        if (tmp_l[1])
        {
            FilterVerChromaEdge(dataCb+2*width, tmp_l[1], thresholds + LEFT,
                width);
            FilterVerChromaEdge(dataCr+2*width, tmp_l[1], thresholds + LEFT,
                width);
        }
    if (tmp_l[2])
        {
            FilterVerChromaEdge(dataCb+4*width, tmp_l[2], thresholds + LEFT,
                width);
            FilterVerChromaEdge(dataCr+4*width, tmp_l[2], thresholds + LEFT,
                width);
        }
    if (tmp_l[3])
        {
            FilterVerChromaEdge(dataCb+6*width, tmp_l[3], thresholds + LEFT,
                width);
            FilterVerChromaEdge(dataCr+6*width, tmp_l[3], thresholds + LEFT,
                width);
        }
    }
    if (tmp_l[8] == tmp_l[9] && tmp_l[8] == tmp_l[10] &&
        tmp_l[8] == tmp_l[11])
    {
        if (tmp_l[8]) {
            FilterVerChromaEdges(dataCb + 4, dataCr + 4, tmp_l[8], thresholds + INNER, width);
    }
    } else {
     if (tmp_l[8])
        {
            FilterVerChromaEdge(dataCb+4, tmp_l[8], thresholds + INNER,
                width);
            FilterVerChromaEdge(dataCr+4, tmp_l[8], thresholds + INNER,
                width);
        }
     if (tmp_l[9])
        {
            FilterVerChromaEdge(dataCb+2*width+4, tmp_l[9],
                thresholds + INNER, width);
            FilterVerChromaEdge(dataCr+2*width+4, tmp_l[9],
                thresholds + INNER, width);
        }
         if (tmp_l[10])
        {
            FilterVerChromaEdge(dataCb+4*width+4, tmp_l[10],
                thresholds + INNER, width);
            FilterVerChromaEdge(dataCr+4*width+4, tmp_l[10],
                thresholds + INNER, width);
        }
         if (tmp_l[11])
        {
            FilterVerChromaEdge(dataCb+6*width+4, tmp_l[11],
                thresholds + INNER, width);
            FilterVerChromaEdge(dataCr+6*width+4, tmp_l[11],
                thresholds + INNER, width);
        }
    }

    /* if bS is equal for all horizontal edges of the row -> perform
     * filtering with FilterHorChroma, otherwise use FilterHorChromaEdge
     * for each edge separately. offset variable indicates top macroblock
     * edge on the first loop round, inner edge for the second */
    offset = TOP;
    for (vblock = 0; vblock < 2; vblock++)
    {
        if (tmp[0] == tmp[1] && tmp[1] == tmp[2] &&
            tmp[2] == tmp[3])
        {
            if(tmp[0])
            {
                FilterHorChroma(dataCb, tmp[0], thresholds+offset,
                    (i32)width);
                FilterHorChroma(dataCr, tmp[0], thresholds+offset,
                    (i32)width);
            }
        }
        else
        {
            if (tmp[0])
            {
                FilterHorChromaEdge(dataCb, tmp[0], thresholds+offset,
                    (i32)width);
                FilterHorChromaEdge(dataCr, tmp[0], thresholds+offset,
                    (i32)width);
            }
            if (tmp[1])
            {
                FilterHorChromaEdge(dataCb+2, tmp[1], thresholds+offset,
                    (i32)width);
                FilterHorChromaEdge(dataCr+2, tmp[1], thresholds+offset,
                    (i32)width);
            }
            if (tmp[2])
            {
                FilterHorChromaEdge(dataCb+4, tmp[2], thresholds+offset,
                    (i32)width);
                FilterHorChromaEdge(dataCr+4, tmp[2], thresholds+offset,
                    (i32)width);
            }
            if (tmp[3])
            {
                FilterHorChromaEdge(dataCb+6, tmp[3], thresholds+offset,
                    (i32)width);
                FilterHorChromaEdge(dataCr+6, tmp[3], thresholds+offset,
                    (i32)width);
            }
        }

        tmp += 8;
        dataCb += width*4;
        dataCr += width*4;
        offset = INNER;
    }
}

/*lint +e701 +e702 */

