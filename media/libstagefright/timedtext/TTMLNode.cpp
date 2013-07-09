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

#define LOG_TAG "TTMLNode"
#include <utils/Log.h>

#include "TTMLNode.h"
#include "TTMLUtils.h"

#include <media/stagefright/foundation/ADebug.h>
#include <utils/Errors.h>
#include <utils/String8.h>

namespace android {

TTMLNode::TTMLNode() {}

TTMLNode::TTMLNode(const char *name, String8 attributes,
                   int64_t startTimeUs, int64_t endTimeUs,
                   sp<TTMLNode> parent) :
    mElement(name), mAttributes(attributes),
    mStartTimeUs(startTimeUs), mEndTimeUs(endTimeUs),
    mParent(parent), mPlainText("") {
}

int64_t TTMLNode::startTimeUs() {
    return mStartTimeUs;
}

int64_t TTMLNode::endTimeUs() {
    return mEndTimeUs;
}

String8 TTMLNode::element() {
    return mElement;
}

String8 TTMLNode::text() {
    if (!strcasecmp(TTMLUtils::kPCData, mElement.string())) {
        return mPlainText;
    }
    if (!strcasecmp(TTMLUtils::kTagBr, mElement.string())) {
        return String8("\n");
    }
    if (!strcasecmp(TTMLUtils::kTagMetadata, mElement.string())) {
        return String8();
    }
    String8 plainText;
    char lastPlainTextChar = 0;
    for (size_t i = 0; i < mChildren.size(); ++i) {
        String8 appendText = mChildren.itemAt(i)->text();
        if (*appendText.string() == ' ' && lastPlainTextChar == ' ') {
            plainText.append(appendText.string() + 1, appendText.length() - 1);
        } else {
            plainText.append(appendText);
        }
        lastPlainTextChar = *(appendText.string() + appendText.length() - 1);
    }
    return plainText;
}

String8 TTMLNode::ttmlFragmentFromRoot() {
    return ttmlBeginTagsFromRoot() + ttmlChildren() + ttmlEndTagsToRoot();
}

void TTMLNode::setHeadTag(String8 headTag) {
    mHeadTag = headTag;
}

void TTMLNode::addChildNode(sp<TTMLNode> node) {
    CHECK(!strcasecmp(TTMLUtils::kTagSpan, node->element().string()) ||
          !strcasecmp(TTMLUtils::kTagBr, node->element().string()) ||
          !strcasecmp(TTMLUtils::kPCData, node->element().string()) ||
          !strcasecmp(TTMLUtils::kTagMetadata, node->element().string()) ||
          TTMLUtils::isSmpteTag(node->element().string()));
    mChildren.add(node);
}

void TTMLNode::addChildNode(String8 plainText) {
    if (plainText.length() > 0) {
        sp<TTMLNode> node = new TTMLNode();
        node->mElement = TTMLUtils::kPCData;
        node->mParent = this;
        node->mPlainText = plainText;

        addChildNode(node);
    }
}

String8 TTMLNode::ttmlBeginTagsFromRoot() {
    if (mParent != NULL) {
        return mParent->ttmlBeginTagsFromRoot() + ttmlBeginTag();
    }
    return ttmlBeginTag();
}

String8 TTMLNode::ttmlEndTagsToRoot() {
    if (mParent != NULL) {
        return ttmlEndTag() + mParent->ttmlEndTagsToRoot();
    }
    return ttmlEndTag();
}

String8 TTMLNode::ttmlBeginTag() {
    String8 ttmlTags;
    if (mAttributes.length() > 0) {
        ttmlTags = String8::format("<%s %s>", mElement.string(),
                                   mAttributes.string());
    } else {
        ttmlTags = String8::format("<%s>", mElement.string());
    }
    if (!strcasecmp(TTMLUtils::kTagTT, mElement.string()) && mHeadTag.length() > 0) {
        ttmlTags.append(mHeadTag);
    }
    return ttmlTags;
}

String8 TTMLNode::ttmlEndTag() {
    return String8::format("</%s>", mElement.string());
}

String8 TTMLNode::ttmlChildren() {
    String8 nodeString;
    for (size_t i = 0; i < mChildren.size(); ++i) {
        nodeString.append(mChildren.itemAt(i)->ttmlFragment());
    }
    return nodeString;
}

String8 TTMLNode::ttmlFragment() {
    if (!strcasecmp(TTMLUtils::kTagBr, mElement.string())) {
        return String8("<br/>");
    }
    if (!strcasecmp(TTMLUtils::kPCData, mElement.string())) {
        return mPlainText;
    }
    return ttmlBeginTag() + ttmlChildren() + ttmlEndTag();
}

}  // namespace android
