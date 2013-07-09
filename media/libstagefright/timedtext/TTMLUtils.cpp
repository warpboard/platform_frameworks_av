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

#define LOG_TAG "TTMLUtils"
#include <utils/Log.h>

#include "TTMLUtils.h"

#include <ctype.h>
#include <math.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/MediaErrors.h>

namespace android {

const char* TTMLUtils::kAttrBegin = "begin";
const char* TTMLUtils::kAttrDuration = "dur";
const char* TTMLUtils::kAttrEnd = "end";
const char* TTMLUtils::kAttrFrameRate = "frameRate";
const char* TTMLUtils::kAttrSubFrameRate = "subFrameRate";
const char* TTMLUtils::kAttrTickRate = "tickRate";

const char* TTMLUtils::kTagBody = "body";
const char* TTMLUtils::kTagBr = "br";
const char* TTMLUtils::kTagDiv = "div";
const char* TTMLUtils::kTagHead = "head";
const char* TTMLUtils::kTagMetadata = "metadata";
const char* TTMLUtils::kTagP = "p";
const char* TTMLUtils::kTagSpan = "span";
const char* TTMLUtils::kTagStyle = "style";
const char* TTMLUtils::kTagStyling = "styling";
const char* TTMLUtils::kTagTT = "tt";

const char* TTMLUtils::kTagSmpteImage = "smpte:image";
const char* TTMLUtils::kTagSmpteData  = "smpte:data";
const char* TTMLUtils::kTagSmpteInformation = "smpte:information";

const char* TTMLUtils::kPCData = "#pcdata";

// static
// Parse timecode attribute
//
// From W3C DFXP Working Draft (http://www.w3.org/TR/ttaf1-dfxp):
//
// 10.3.1 <timeExpression>
//
//    See 6.2.11 ttp:timeBase, 6.2.3 ttp:frameRate, 6.2.9 ttp:subFrameRate,
//    and 6.2.10 ttp:tickRate for further information on explicit specification
//    of time base, frame rate, sub-frame rate, and tick rate.
//
// Syntax Representation -- <timeExpression>
//
//     <timeExpression>
//       : clock-time
//       | offset-time
//     clock-time
//       : hours ":" minutes ":" seconds ( fraction | ":" frames ( "." sub-frames )? )?
//     offset-time
//       : time-count fraction? metric
//     hours
//       : <digit> <digit>
//       | <digit> <digit> <digit>+
//     minutes | seconds
//       : <digit> <digit>
//     frames
//       : <digit> <digit>
//       | <digit> <digit> <digit>+
//     sub-frames
//       : <digit>+
//     fraction
//       : "." <digit>+
//     time-count
//       : <digit>+
//     metric
//       : "h"                 // hours
//       | "m"                 // minutes
//       | "s"                 // seconds
//       | "ms"                // milliseconds
//       | "f"                 // frames
//       | "t"                 // ticks
//
// NOTE: Current implementation doesn't support the metric of ticks for offset-time,
//       and we ignore the sub-frames values for clock-time.
//
// NOTE: Current implementation extends the TTML spec on timeExpression to
//       support negative time offset.
//       Thus <timeExpression> can start with "-" character to indicate it is a
//       negative time offset.
//
status_t TTMLUtils::getTimeFromText(const AString &text, int64_t *timeUs,
        int32_t frameRate, int32_t subFrameRate, int32_t tickRate) {
    static const int32_t kMicroSecPerSec = 1000 * 1000;
    const char *pos = text.c_str();
    int32_t readSize = 0;
    int32_t sign = readSignFromText(&pos, &readSize);
    int32_t hour = readDigitsFromText(&pos, &readSize);
    if (*pos == '\0') {
        return ERROR_MALFORMED;
    }
    if (*pos == ':') {
        // clock-time
        if (readSize < 2) {
            return ERROR_MALFORMED;
        }
        ++pos;
        int32_t min = readDigitsFromText(&pos, &readSize);
        if (*pos != ':' || readSize != 2 || min > 59) {
            return ERROR_MALFORMED;
        }

        ++pos;
        double sec = readDigitsFromText(&pos, &readSize);
        if (sec >= 61.0 || readSize != 2) {
            return ERROR_MALFORMED;
        }
        if (*pos == '.') {
            sec += readFractionFromText(&pos, &readSize);
            if (readSize < 2) {
                return ERROR_MALFORMED;
            }
            if (*pos != '\0') {
                return ERROR_MALFORMED;
            }
        }
        if (*pos == '\0') {
            *timeUs = static_cast<int64_t>(
                    (hour * 60 * 60 + min * 60 + sec) * kMicroSecPerSec * sign);
            return OK;
        } else if (*pos == ':') {
            ++pos;
            int32_t frame = readDigitsFromText(&pos, &readSize);
            int32_t subFrame = 0;
            if (readSize < 2) {
                return ERROR_MALFORMED;
            }
            if (frame >= frameRate || frame < 0) {
                return BAD_VALUE;
            }
            if (*pos == '.') {
                ++pos;
                subFrame = readDigitsFromText(&pos, &readSize);
                if (readSize == 0) {
                    return ERROR_MALFORMED;
                }
            }
            if (*pos) {
                return ERROR_MALFORMED;
            }
            CHECK_GT(frameRate, 0);
            if (subFrameRate > 0) {
                int32_t numOfSubFrames = subFrameRate * frame + subFrame;
                *timeUs = sign * static_cast<int64_t>(
                        (hour * 60 * 60 + min * 60 + sec) * kMicroSecPerSec +
                        numOfSubFrames * kMicroSecPerSec / (frameRate * subFrameRate));
            } else {
                *timeUs = sign * static_cast<int64_t>(
                        (hour * 60 * 60 + min * 60 + sec) * kMicroSecPerSec +
                        frame * kMicroSecPerSec / frameRate);
            }
            return OK;
        } else {
            return ERROR_MALFORMED;
        }
    } else {
        // offset-time
        double time = static_cast<double>(hour);
        if (*pos == '.') {
            time += readFractionFromText(&pos, &readSize);
            if (readSize < 2) {
                return ERROR_MALFORMED;
            }
        }
        switch (*pos) {
            case 'h':
                *timeUs = static_cast<int64_t>(time * 60 * 60 * kMicroSecPerSec * sign);
                break;
            case 'm':
                if (*(pos + 1) == 's') {
                    *timeUs = static_cast<int64_t>(time * 1000 * sign);
                    ++pos;
                } else {
                    *timeUs = static_cast<int64_t>(time * 60 * kMicroSecPerSec * sign);
                }
                break;
            case 's':
                *timeUs = static_cast<int64_t>(time * kMicroSecPerSec * sign);
                break;
            case 'f':
                *timeUs = static_cast<int64_t>(time * kMicroSecPerSec / frameRate * sign);
                break;
            case 't':
                CHECK_GT(tickRate, 0);
                *timeUs = static_cast<int64_t>(time * kMicroSecPerSec / tickRate * sign);
                break;
            default:
                return ERROR_MALFORMED;
        }
        if (*(pos + 1)) {
            return ERROR_MALFORMED;
        }
        return OK;
    }
}

// static
String8 TTMLUtils::convertToPlainText(const char *text, int len) {
    String8 plainText;
    bool shouldAddSpace = false;
    for (int i = 0; i < len; ++i) {
        char curChar = *(text + i);
        if (isspace(curChar)) {
            shouldAddSpace = true;
        } else {
            if (shouldAddSpace) {
                plainText.append(" ");
            }
            plainText.append(text + i, 1);
            shouldAddSpace = false;
        }
    }
    if (shouldAddSpace && plainText.length() > 0) {
        plainText.append(" ");
    }
    return plainText;
}

// static
bool TTMLUtils::isSmpteTag(const char *text) {
    if (!strcasecmp(TTMLUtils::kTagSmpteImage, text) ||
            !strcasecmp(TTMLUtils::kTagSmpteData, text) ||
            !strcasecmp(TTMLUtils::kTagSmpteInformation, text)) {
        return true;
    }
    return false;
}

// static
bool TTMLUtils::isSpace(const char *str) {
    for (uint32_t i = 0; i < strlen(str); ++i) {
        if (!isspace(str[i])) {
            return false;
        }
    }
    return true;
}

// static
int32_t TTMLUtils::readDigitsFromText(const char **pos, int32_t *readSize) {
    int32_t ret = 0;
    *readSize = 0;
    while (**pos) {
        if (**pos >= '0' && **pos <= '9') {
            ret = ret * 10 + **pos - '0';
            ++(*pos);
            ++(*readSize);
        } else {
            break;
        }
    }
    return ret;
}

// static
int32_t TTMLUtils::readSignFromText(const char **pos, int32_t *readSize) {
    *readSize = 0;
    if (**pos == '-') {
        ++(*pos);
        ++(*readSize);
        return -1;
    }
    return 1;
}

// static
double TTMLUtils::readFractionFromText(const char **pos, int32_t *readSize) {
    double ret = 0;
    double factor = 0.1;
    if (**pos != '.') {
        *readSize = 0;
        return -1.0;
    }
    *readSize = 1;
    ++(*pos);
    while (**pos) {
        if (**pos >= '0' && **pos <= '9') {
            ret += (**pos - '0') * factor;
            factor *= 0.1;
            ++(*pos);
            ++(*readSize);
        } else {
            break;
        }
    }
    return ret;
}

}  // namespace android
