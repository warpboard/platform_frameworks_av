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

#ifndef TIMED_TEXT_TTML_SOURCE_H_
#define TIMED_TEXT_TTML_SOURCE_H_

#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>

#include "TimedTextSource.h"

namespace android {

class AString;
class Parcel;
class TTMLParser;

class TimedTextTTMLSource : public TimedTextSource {
public:
    TimedTextTTMLSource(const sp<MediaSource>& mediaSource);
    virtual status_t start();
    virtual status_t stop() { return mSource->stop(); }
    virtual status_t read(
            int64_t *startTimeUs,
            int64_t *endTimeUs,
            Parcel *parcel,
            const MediaSource::ReadOptions *options = NULL);
    virtual sp<MetaData> getFormat();

protected:
    virtual ~TimedTextTTMLSource();

private:
    sp<MediaSource> mSource;
    sp<TTMLParser> mParser;

    status_t extractAndAppendLocalDescriptions(
            int64_t timeUs, const AString &text,
            const AString &ttmlFragment, Parcel *parcel);
    status_t getNextSubtitleInfo(int64_t *startTimeUs, int64_t *endTimeUs,
            AString *subtitle, AString *ttmlFragment);
    status_t readTTMLChunk(MediaSource::ReadOptions const *options);

    DISALLOW_EVIL_CONSTRUCTORS(TimedTextTTMLSource);
};

}  // namespace android

#endif  // TIMED_TEXT_TTML_SOURCE_H_
