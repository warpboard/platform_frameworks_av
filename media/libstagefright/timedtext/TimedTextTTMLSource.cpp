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

#define __STDINT_LIMITS
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#define LOG_TAG "TimedTextTTMLSource"
#include <utils/Log.h>

#include "TimedTextTTMLSource.h"

#include <binder/Parcel.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>

#include "TextDescriptions.h"
#include "TTMLParser.h"

namespace android {

const int64_t kInvalidTimeUs = INT64_MIN;

TimedTextTTMLSource::TimedTextTTMLSource(const sp<MediaSource>& mediaSource)
    : mSource(mediaSource),
      mParser(new TTMLParser()) {
}

TimedTextTTMLSource::~TimedTextTTMLSource() {
}

status_t TimedTextTTMLSource::start() {
    mParser->reset();
    return mSource->start();
}

status_t TimedTextTTMLSource::read(
        int64_t *startTimeUs, int64_t* endTimeUs, Parcel *parcel,
        const MediaSource::ReadOptions *options) {
    status_t err = UNKNOWN_ERROR;
    bool doSeek = false;
    int64_t seekTimeUs = kInvalidTimeUs;
    MediaSource::ReadOptions::SeekMode seekMode;
    if (options != NULL && options->getSeekTo(&seekTimeUs, &seekMode)) {
        mParser->flush();
        err = readTTMLChunk(options);
        if (err != OK) {
            return err;
        }
        doSeek = true;
    }

    int64_t startTimeUs_ = kInvalidTimeUs;
    int64_t endTimeUs_ = kInvalidTimeUs;
    AString subtitle;
    AString ttmlFragment;
    do {
        err = getNextSubtitleInfo(&startTimeUs_, &endTimeUs_,
                                  &subtitle, &ttmlFragment);
        if (err != OK) {
            return err;
        }
    } while (doSeek && startTimeUs_ < seekTimeUs);

    *startTimeUs = startTimeUs_;
    *endTimeUs = endTimeUs_;
    extractAndAppendLocalDescriptions(startTimeUs_, subtitle,
                                      ttmlFragment, parcel);
    return OK;
}

status_t TimedTextTTMLSource::extractAndAppendLocalDescriptions(
        int64_t timeUs, const AString &text,
        const AString &ttmlFragment, Parcel *parcel) {
    parcel->writeInt32(TextDescriptions::KEY_LOCAL_SETTING);
    parcel->writeInt32(TextDescriptions::KEY_START_TIME);
    parcel->writeInt32(timeUs/1000);

    parcel->writeInt32(TextDescriptions::KEY_STRUCT_TEXT);
    parcel->writeInt32(text.size());
    parcel->writeInt32(text.size());
    if (text.size() > 0) {
        parcel->write(text.c_str(), text.size());
    }

    parcel->writeInt32(TextDescriptions::KEY_RAW_TEXT);
    parcel->writeInt32(ttmlFragment.size());
    if (ttmlFragment.size() > 0) {
        parcel->write(ttmlFragment.c_str(), ttmlFragment.size());
    }
    return OK;
}

status_t TimedTextTTMLSource::getNextSubtitleInfo(
        int64_t *startTimeUs, int64_t *endTimeUs,
        AString *subtitle, AString *ttmlFragment) {
    status_t err = OK;
    while (NOT_ENOUGH_DATA ==
            (err = mParser->getNextSubtitleInfo(
                    startTimeUs, endTimeUs, subtitle, ttmlFragment))) {
        err = readTTMLChunk(NULL);
        if (err != OK) {
            return err;
        }
    }
    return err;
}

status_t TimedTextTTMLSource::readTTMLChunk(MediaSource::ReadOptions const *options) {
    MediaBuffer *textBuffer = NULL;
    status_t err = mSource->read(&textBuffer, options);
    if (err != OK) {
        return err;
    }
    // mOptions should be applied to mSource only once during seeking operation.
    CHECK(textBuffer != NULL);
    int64_t offsetTimeUs = -1;
    CHECK(textBuffer->meta_data()->findInt64(kKeyTime, &offsetTimeUs));
    ALOGV("read size:%d time:%lld", textBuffer->size(), offsetTimeUs);
    err = mParser->feedMoreTTML(
            static_cast<const char *>(textBuffer->data()), textBuffer->size(),
            offsetTimeUs);
    textBuffer->release();
    return err;
}

sp<MetaData> TimedTextTTMLSource::getFormat() {
    return mSource->getFormat();
}

}  // namespace android
