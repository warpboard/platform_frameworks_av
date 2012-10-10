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

#ifndef H264SWDEC_UTIL_MIPS_H
#define H264SWDEC_UTIL_MIPS_H

static __inline u32 ABS(int a){
    int ret;
    __asm__ volatile (
        "absq_s.w %[ret],   %[a]    \n\t"

        : [ret] "=r" (ret)
        : [a] "r" (a)
    );
    return ret;
}

#endif /* #ifdef H264SWDEC_UTIL_MIPS_H */

