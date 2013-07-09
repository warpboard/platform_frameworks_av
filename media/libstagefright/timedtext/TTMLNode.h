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

#ifndef TTML_NODE_H_
#define TTML_NODE_H_

#include <media/stagefright/foundation/ABase.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Vector.h>

namespace android {

class TTMLNode : public RefBase {
public:
    TTMLNode();
    TTMLNode(const char *name, String8 attributes,
             int64_t startTimeUs, int64_t endTimeUs,
             sp<TTMLNode> parent);

    int64_t startTimeUs();
    int64_t endTimeUs();
    String8 element();
    String8 text();
    String8 ttmlFragmentFromRoot();

    void setHeadTag(String8 headTag);
    void addChildNode(sp<TTMLNode> node);
    void addChildNode(String8 plainText);

private:
    String8 ttmlBeginTagsFromRoot();
    String8 ttmlEndTagsToRoot();

    String8 ttmlBeginTag();
    String8 ttmlEndTag();
    String8 ttmlChildren();
    String8 ttmlFragment();

    String8 mElement;
    String8 mAttributes;
    int64_t mStartTimeUs;
    int64_t mEndTimeUs;
    sp<TTMLNode> mParent;
    String8 mPlainText;
    // Meaningful only with <tt> tag.
    String8 mHeadTag;
    // <span>, <br>, #pcdata nodes are stored as children.
    Vector<sp<TTMLNode> > mChildren;

    DISALLOW_EVIL_CONSTRUCTORS(TTMLNode);
};

}  // namespace android

#endif  // TTML_NODE_H_
