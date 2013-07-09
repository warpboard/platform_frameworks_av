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

#ifndef TTML_PARSER_H_
#define TTML_PARSER_H_

#include <expat.h>  // XML_Parser

#include <media/stagefright/foundation/ABase.h>
#include <utils/Errors.h>  // status_t
#include <utils/KeyedVector.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Vector.h>

namespace android {

class AString;
class TTMLNode;

class TTMLParser : public RefBase {
public:
    TTMLParser();

    status_t feedMoreTTML(const char *data, int64_t size, int64_t startOffsetUs);
    status_t getNextSubtitleInfo(
            int64_t *startTimeUs, int64_t *endTimeUs,
            AString *subtitle, AString *ttmlFragment);
    void flush();
    void reset();

protected:
    virtual ~TTMLParser();

private:
    static const int32_t kDefaultFrameRate = 30;

    XML_Parser mParser;
    int64_t mStartOffsetUs;
    size_t mCurrentSubtitleIndex;
    int32_t mFrameRate;
    int32_t mSubFrameRate;
    int32_t mTickRate;
    bool mParsingHeadTag;
    // Parser does not keep the header node in the stack. Keep it as a string
    // instead.
    String8 mHeadTagString;
    // Stores subtitle nodes with their start time as keys.
    KeyedVector<int64_t, sp<TTMLNode> > mSubtitleNodes;
    // Used during parsing a ttml document.
    Vector<sp<TTMLNode> > mNodeStack;

    sp<TTMLNode> getNodeFromStack();

    // Handlers for expat parser.
    static void startElementHandler(
           void *data, const char *element, const char **attrs);
    static void endElementHandler(void *data, const char *element);
    static void charDataHandler(void *data, const char *text, int len);

    DISALLOW_EVIL_CONSTRUCTORS(TTMLParser);
};

}  // namespace android

#endif  // TTML_PARSER_H_
