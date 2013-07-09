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


#define LOG_TAG "TTMLParser"
#include <utils/Log.h>

#include "TTMLParser.h"

#include <expat.h>  // XML_Parser
#include <limits.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/MediaErrors.h>
#include <utils/String8.h>

#include "TTMLNode.h"
#include "TTMLUtils.h"

namespace android {

static const int64_t kInvalidTimeUs = INT_MIN;

TTMLParser::TTMLParser()
    : mParser(::XML_ParserCreate(NULL)),
      mStartOffsetUs(0),
      mCurrentSubtitleIndex(0),
      mFrameRate(kDefaultFrameRate),
      mSubFrameRate(-1),
      mTickRate(-1),
      mParsingHeadTag(false) {
    ::XML_SetElementHandler(mParser, startElementHandler, endElementHandler);
    ::XML_SetCharacterDataHandler(mParser, charDataHandler);
    ::XML_SetUserData(mParser, this);
}

TTMLParser::~TTMLParser() {
    ::XML_ParserFree(mParser);
}

status_t TTMLParser::feedMoreTTML(
        const char *data, int64_t size, int64_t startOffsetUs) {
    if (mNodeStack.isEmpty()) {
        reset();
    }

    status_t err = OK;
    CHECK_GE(startOffsetUs, 0);
    mStartOffsetUs = startOffsetUs;
    if (::XML_Parse(mParser, data, size, 0) != XML_STATUS_OK) {
        ALOGE("parse error: %s",
             ::XML_ErrorString(::XML_GetErrorCode(mParser)));
        err = ERROR_MALFORMED;
    }
    return err;
}

status_t TTMLParser::getNextSubtitleInfo(
        int64_t *startTimeUs, int64_t *endTimeUs,
        AString *subtitle, AString *ttmlFragment) {
    if (mSubtitleNodes.isEmpty() ||
        mCurrentSubtitleIndex >= mSubtitleNodes.size()) {
        return NOT_ENOUGH_DATA;
    }
    sp<TTMLNode> node = mSubtitleNodes.valueAt(mCurrentSubtitleIndex);
    *startTimeUs = node->startTimeUs() + mStartOffsetUs;
    *endTimeUs = node->endTimeUs() + mStartOffsetUs;

    subtitle->setTo(node->text().string());
    ttmlFragment->setTo(node->ttmlFragmentFromRoot().string());

    ++mCurrentSubtitleIndex;
    return OK;
}

void TTMLParser::flush() {
    reset();
}

void TTMLParser::reset() {
    mStartOffsetUs = 0;
    mCurrentSubtitleIndex = 0;
    mParsingHeadTag = false;
    mHeadTagString = "";
    mSubtitleNodes.clear();
    mNodeStack.clear();

    ::XML_ParserFree(mParser);
    mParser = ::XML_ParserCreate(NULL);
    ::XML_SetElementHandler(mParser, startElementHandler, endElementHandler);
    ::XML_SetCharacterDataHandler(mParser, charDataHandler);
    ::XML_SetUserData(mParser, this);
}

// static
void TTMLParser::startElementHandler(
        void *data, const char *element, const char **attrs) {
    TTMLParser *ttmlParser = static_cast<TTMLParser *>(data);

    // ref: http://www.w3.org/TR/ttaf1-dfxp/#content-element-vocabulary
    // element can be tt, head, body, div, p, span, br.
    // For other smpte specific tags, please refer:
    // https://www.smpte.org/sites/default/files/st2052-1-2010.pdf
    if (!strcasecmp("head", element) || ttmlParser->mParsingHeadTag) {
        ttmlParser->mParsingHeadTag = true;
        ttmlParser->mHeadTagString.appendFormat("<%s", element);
        if (attrs != NULL) {
            for (int i = 0; attrs[i]; i += 2) {
                ttmlParser->mHeadTagString.appendFormat(
                        " %s=\"%s\"", attrs[i] , attrs[i+1]);
            }
        }
        ttmlParser->mHeadTagString += ">";
    } else if (!strcasecmp(TTMLUtils::kTagTT, element) ||
        !strcasecmp(TTMLUtils::kTagP, element) ||
        !strcasecmp(TTMLUtils::kTagBody, element) ||
        !strcasecmp(TTMLUtils::kTagDiv, element) ||
        !strcasecmp(TTMLUtils::kTagSpan, element) ||
        !strcasecmp(TTMLUtils::kTagBr, element) ||
        !strcasecmp(TTMLUtils::kTagMetadata, element) ||
        TTMLUtils::isSmpteTag(element)) {
        int64_t startTimeUs = 0;
        int64_t endTimeUs = kInvalidTimeUs;
        int64_t durationUs = 0;
        String8 attributes;
        if (attrs != NULL) {
            for (int i = 0; attrs[i]; i += 2) {
                status_t err = OK;

                const char* attribute = index(attrs[i], ':');
                if (attribute == NULL) {
                    attribute = attrs[i];
                } else {
                    attribute++;
                }

                // NOTO: we only extract time related attributes, so that applications can
                // get TTML texts on time.
                if (!strcasecmp(attribute, TTMLUtils::kAttrFrameRate)) {
                    ttmlParser->mFrameRate = atoi(attrs[i+1]);
                } else if (!strcasecmp(attribute, TTMLUtils::kAttrSubFrameRate)) {
                    ttmlParser->mSubFrameRate = atoi(attrs[i+1]);
                } else if (!strcasecmp(attribute, TTMLUtils::kAttrTickRate)) {
                    ttmlParser->mTickRate = atoi(attrs[i+1]);
                } else if (!strcasecmp(attribute, TTMLUtils::kAttrBegin)) {
                    err = TTMLUtils::getTimeFromText(attrs[i+1], &startTimeUs,
                            ttmlParser->mFrameRate, ttmlParser->mSubFrameRate,
                            ttmlParser->mTickRate);
                } else if (!strcasecmp(attribute, TTMLUtils::kAttrEnd)) {
                    err = TTMLUtils::getTimeFromText(attrs[i+1], &endTimeUs,
                            ttmlParser->mFrameRate, ttmlParser->mSubFrameRate,
                            ttmlParser->mTickRate);
                } else if (!strcasecmp(attribute, TTMLUtils::kAttrDuration)) {
                    err = TTMLUtils::getTimeFromText(attrs[i+1], &durationUs,
                            ttmlParser->mFrameRate, ttmlParser->mSubFrameRate,
                            ttmlParser->mTickRate);
                } else {
                    if (attributes.length() > 0)  attributes += " ";
                    attributes = attributes + attrs[i] +
                                 "=\"" + attrs[i+1] + "\"";
                }
                if (err != OK) {
                    ALOGE("Parsing error on a timeExpression: %s=%s",
                         attrs[i], attrs[i+1]);
                }
            }
            if (durationUs > 0) {
                if (endTimeUs != kInvalidTimeUs) {
                    ALOGW("'dur' and 'end' attributes are defined "
                         "at the same time. 'end' value is ignored.");
                }
                endTimeUs = startTimeUs + durationUs;
            }
            if (endTimeUs != kInvalidTimeUs && startTimeUs > endTimeUs) {
                ALOGE("begin time can't be greater than end time.");
            }
        }

        sp<TTMLNode> parentNode = NULL;
        parentNode = ttmlParser->getNodeFromStack();
        if (parentNode != NULL) {
            startTimeUs += parentNode->startTimeUs();
            if (endTimeUs != kInvalidTimeUs) {
                endTimeUs += parentNode->startTimeUs();
            }

            // If the end time remains unspecified, then the end point is
            // interpreted as the end point of the external time interval.
            if ((endTimeUs == kInvalidTimeUs) ||
                (endTimeUs > parentNode->endTimeUs() &&
                 parentNode->endTimeUs() != kInvalidTimeUs)) {
                endTimeUs = parentNode->endTimeUs();
            }
        }

        // Create new TTMLNode and push it to the stack.
        sp<TTMLNode> newNode = new TTMLNode(element, attributes,
                                            startTimeUs, endTimeUs,
                                            parentNode);
        ttmlParser->mNodeStack.push(newNode);
    }
}

sp<TTMLNode> TTMLParser::getNodeFromStack() {
    if (mNodeStack.isEmpty()) {
        return NULL;
    }
    return mNodeStack.editTop();
}

// static
void TTMLParser::endElementHandler(void *data, const char *element) {
    TTMLParser *ttmlParser = static_cast<TTMLParser *>(data);
    sp<TTMLNode> node = ttmlParser->getNodeFromStack();
    CHECK(node != NULL);

    if (ttmlParser->mParsingHeadTag) {
        ttmlParser->mHeadTagString.appendFormat("</%s>", element);
        if (!strcasecmp("head", element)) {
            ttmlParser->mParsingHeadTag = false;
            // the top node of the stack is the ancestor node of 'head'.
            node->setHeadTag(ttmlParser->mHeadTagString);
        }
    } else if (!strcasecmp(TTMLUtils::kTagBr, element) ||
            !strcasecmp(TTMLUtils::kTagMetadata, element) ||
            TTMLUtils::isSmpteTag(element)) {
        ttmlParser->mNodeStack.pop();
        sp<TTMLNode> parentNode = ttmlParser->getNodeFromStack();
        CHECK(parentNode != NULL);
        parentNode->addChildNode(node);
    } else if (!strcasecmp(TTMLUtils::kTagSpan, element)) {
        // TODO: consider how to treat span to support attributes of span tag
        // such as begin/end/style attributes.
        ttmlParser->mNodeStack.pop();
        sp<TTMLNode> parentNode = ttmlParser->getNodeFromStack();
        CHECK(parentNode != NULL);
        parentNode->addChildNode(node);
    } else if (!strcasecmp(TTMLUtils::kTagP, element)) {
        ttmlParser->mSubtitleNodes.add(node->startTimeUs(), node);
        ttmlParser->mNodeStack.pop();
        ALOGV("adding new subtitle node: %lld: %s", node->startTimeUs(),
             node->text().string());
    } else if (!strcasecmp(node->element().string(), element)) {
        ttmlParser->mNodeStack.pop();
    }
}

// static
void TTMLParser::charDataHandler(void *data, const char *text, int len) {
    TTMLParser *ttmlParser = static_cast<TTMLParser *>(data);
    sp<TTMLNode> node = ttmlParser->getNodeFromStack();
    if (node == NULL) {
        ALOGE("it is a malformed ttml : %s", text);
        ::XML_StopParser(ttmlParser->mParser, false);
    }

    String8 plainText = TTMLUtils::convertToPlainText(text, len);
    if (plainText.length() > 0) {
        if (ttmlParser->mParsingHeadTag) {
            ttmlParser->mHeadTagString.append(plainText.string());
        } else if (!TTMLUtils::isSpace(plainText.string())) {
            node->addChildNode(plainText);
        }
    }
}

}  // namespace android
