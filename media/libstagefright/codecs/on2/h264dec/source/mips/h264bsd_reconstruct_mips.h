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

#ifndef H264SWDEC_RECONSTRUCT_MIPS_H
#define H264SWDEC_RECONSTRUCT_MIPS_H


/*------------------------------------------------------------------------------
    Functions (h264bsdInterpolateVerQuarter_verOffset_1 and
    h264bsdInterpolateVerQuarter_verOffset_0 are replacing the original function
    h264bsdInterpolateVerQuarter
------------------------------------------------------------------------------*/
void h264bsdInterpolateVerQuarter_verOffset_1(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight);

void h264bsdInterpolateVerQuarter_verOffset_0(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight);

#endif /* #ifdef H264SWDEC_RECONSTRUCT_MIPS_H */

