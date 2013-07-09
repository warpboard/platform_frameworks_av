/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef TTML_UTILS_H_
#define TTML_UTILS_H_

#include <media/stagefright/foundation/ABase.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/String8.h>

namespace android {

namespace test {
class TTMLUtilsTest;
}

class AString;

class TTMLUtils {
public:
    static const char* kAttrBegin;
    static const char* kAttrDuration;
    static const char* kAttrEnd;
    static const char* kAttrFrameRate;
    static const char* kAttrSubFrameRate;
    static const char* kAttrTickRate;

    static const char* kTagBody;
    static const char* kTagBr;
    static const char* kTagDiv;
    static const char* kTagHead;
    static const char* kTagMetadata;
    static const char* kTagP;
    static const char* kTagSpan;
    static const char* kTagStyle;
    static const char* kTagStyling;
    static const char* kTagTT;

    static const char* kTagSmpteImage;
    static const char* kTagSmpteData;
    static const char* kTagSmpteInformation;

    static const char* kPCData;

    static status_t getTimeFromText(const AString &text, int64_t *timeUs,
            int32_t frameRate, int32_t subFrameRate, int32_t tickRate);
    static String8 convertToPlainText(const char *text, int len);
    static bool isSmpteTag(const char *text);
    static bool isSpace(const char *str);

private:
    friend class test::TTMLUtilsTest;
    static int32_t readDigitsFromText(const char **pos, int32_t *readSize);
    static int32_t readSignFromText(const char **pos, int32_t *readSize);
    static double readFractionFromText(const char **pos, int32_t *readSize);
    DISALLOW_EVIL_CONSTRUCTORS(TTMLUtils);
};

}  // namespace android

#endif  // TTML_UTILS_H_
