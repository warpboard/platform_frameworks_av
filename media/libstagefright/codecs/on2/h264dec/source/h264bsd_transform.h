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
    2. Module defines
    3. Data types
    4. Function prototypes

------------------------------------------------------------------------------*/

#ifndef H264SWDEC_TRANSFORM_H
#define H264SWDEC_TRANSFORM_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

#if MIPS_DSP_R2_LE
/*-----------------------------------------------------------------------------
    data type changed from i32 to i16 . This allows compiler to generate more
    efficient code for MIPS platform. All necessary changes are done in the
    h264bsd_transform.c file.
 -----------------------------------------------------------------------------*/

u32 h264bsdProcessBlock(i16 *data, u32 qp, u32 skip, u32 coeffMap);
void h264bsdProcessLumaDc(i16 *data, u32 qp);
void h264bsdProcessChromaDc(i16 *data, u32 qp);
#else
u32 h264bsdProcessBlock(i32 *data, u32 qp, u32 skip, u32 coeffMap);
void h264bsdProcessLumaDc(i32 *data, u32 qp);
void h264bsdProcessChromaDc(i32 *data, u32 qp);
#endif /* #if MIPS_DSP_R2_LE */

#endif /* #ifdef H264SWDEC_TRANSFORM_H */

