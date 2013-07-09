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

#define LOG_TAG "TimedTextTTMLSource_test"
#include <utils/Log.h>

#include <gtest/gtest.h>

#include <binder/Parcel.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <utils/misc.h>

#include <TimedTextSource.h>
#include <TimedTextTTMLSource.h>
#include <TTMLUtils.h>

#include "SampleTTMLChunk.h"

namespace android {
namespace test {

class SSMediaSourceStub : public MediaSource {
public:
    SSMediaSourceStub(const char **chunks, int32_t size):
        mChunkIndex(0),
        mTTMLChunks(chunks),
        mTotalNumOfChunk(size) {
    }
    virtual ~SSMediaSourceStub() {}

    virtual status_t start(MetaData *params = NULL) {
        mChunkIndex = 0;
        return OK;
    }

    virtual status_t stop() {
        return OK;
    }

    virtual sp<MetaData> getFormat() {
        return NULL;
    }

    virtual status_t read(MediaBuffer **buffer,
            const ReadOptions *options = NULL) {
        if (mChunkIndex == mTotalNumOfChunk) {
            return NOT_ENOUGH_DATA;
        }
        CHECK_LT(mChunkIndex, mTotalNumOfChunk);
        const char *chunkData = mTTMLChunks[mChunkIndex];
        MediaBuffer* data = new MediaBuffer(strlen(chunkData));
        memcpy(data->data(), chunkData, strlen(chunkData));
        // Each TTML chunk has a time offset : mChunkIndex * 1 min.
        data->meta_data()->setInt64(kKeyTime, mChunkIndex * 60 * 1000000);
        *buffer = data;
        mChunkIndex++;
        return OK;
    }

private:
    int32_t mChunkIndex;
    const char **mTTMLChunks;
    int32_t mTotalNumOfChunk;
};

class TimedTextTTMLSourceTest : public testing::Test {
protected:
    void SetUp() { }

    int getSubtitleFromParcel(const char** subtitle, const Parcel& parcel) {
        int32_t len;
        parcel.setDataPosition(16);
        parcel.readInt32(&len);
        parcel.setDataPosition(24);
        *subtitle = static_cast<const char*>(parcel.readInplace(len));
        return len;
    }

    int getFragmentFromParcel(const char** fragment, const Parcel& parcel) {
        int32_t len;
        parcel.setDataPosition(16);
        parcel.readInt32(&len);
        parcel.setDataPosition((31 + len) / 4 * 4);
        parcel.readInt32(&len);
        *fragment = static_cast<const char*>(parcel.readInplace(len));
        return len;
    }

    void CheckStartTimeMs(const Parcel& parcel, int32_t timeMs) {
        int32_t intval;
        parcel.setDataPosition(8);
        parcel.readInt32(&intval);
        EXPECT_EQ(timeMs, intval);
    }

    void CheckSubtitleStartsWith(const Parcel& parcel, const char* content) {
        const char* subtitle;
        int len = getSubtitleFromParcel(&subtitle, parcel);
        int32_t content_len = strlen(content);
        CHECK_GE(len, content_len);
        EXPECT_TRUE(strncmp(subtitle, content, content_len) == 0);
    }

    void CheckFragmentStartsWith(const Parcel& parcel, const char* content) {
        const char* fragment;
        int len = getFragmentFromParcel(&fragment, parcel);
        int32_t content_len = strlen(content);
        CHECK_GE(len, content_len);
        EXPECT_TRUE(strncmp(fragment, content, content_len) == 0);
    }

    void CheckFragmentEndsWith(const Parcel& parcel, const char* content) {
        const char* fragment;
        int len = getFragmentFromParcel(&fragment, parcel);
        int32_t content_len = strlen(content);
        CHECK_GE(len, content_len);
        EXPECT_TRUE(strncasecmp(fragment + len - content_len, content, content_len) == 0);
    }

    void CheckFragmentContains(const Parcel& parcel, Vector<const char*> tags) {
        const char* fragment;
        int len = getFragmentFromParcel(&fragment, parcel);
        const char *t = fragment;
        for (size_t i = 0; i < tags.size(); i++) {
            t = strstr(t, tags[i]);
            EXPECT_TRUE(t != NULL);
        }
    }

    void CheckDataEquals(const Parcel& parcel, const char* subtitle, const char* ttmlFragment) {
        const char* data;
        int len = getSubtitleFromParcel(&data, parcel);
        int32_t subtitleLen = strlen(subtitle);
        EXPECT_EQ(subtitleLen, len);
        EXPECT_TRUE(strncmp(data, subtitle, subtitleLen) == 0);

        len = getFragmentFromParcel(&data, parcel);
        const char* data2 = data;
        while (*ttmlFragment) {
            if (*ttmlFragment == *data) {
                data++;
                ttmlFragment++;
            } else if (isspace(*ttmlFragment)) {
                ttmlFragment++;
            } else {
                FAIL();
            }
        }
    }

    sp<TimedTextSource> mSource;
    int64_t startTimeUs;
    int64_t endTimeUs;
    Parcel parcel;
    status_t err;
};

TEST_F(TimedTextTTMLSourceTest, parcelHasTTMLFragment) {
    sp<MediaSource> stub= new SSMediaSourceStub(
            kTTMLSimpleChunk,
            NELEM(kTTMLSimpleChunk));
    mSource = new TimedTextTTMLSource(stub);
    err = mSource->read(&startTimeUs, &endTimeUs, &parcel);
    EXPECT_EQ(OK, err);
    CheckStartTimeMs(parcel, 610000);
    CheckDataEquals(parcel,
                    "Subtitle 1-1",
                    "<tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
                    " xmlns:tts=\"http://www.w3.org/ns/ttml#styling\">"
                    "<head>"
                    "    <metadata>"
                    "      <ttm:title>Document Metadata Example</ttm:title>"
                    "    </metadata>"
                    "    <styling>"
                    "      <style xml:id=\"normal\" "
                    "tts:fontFamily=\"sansSerif\"></style>"
                    "    </styling>"
                    "  </head>"
                    "<body style=\"normal\" region=\"bottom\">"
                    "<div><p>Subtitle 1-1</p></div></body></tt>");
}

TEST_F(TimedTextTTMLSourceTest, parsingBrAndSpanTag) {
    sp<MediaSource> stub= new SSMediaSourceStub(
            kTTMLChunkWithBrAndSpan,
            NELEM(kTTMLChunkWithBrAndSpan));
    mSource = new TimedTextTTMLSource(stub);
    err = mSource->read(&startTimeUs, &endTimeUs, &parcel);
    EXPECT_EQ(OK, err);
    CheckStartTimeMs(parcel, 610000);
    CheckDataEquals(parcel,
                    "Subtitle 1-1\nSubtitle 1-2",
                    "<tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
                    " xmlns:tts=\"http://www.w3.org/ns/ttml#styling\">"
                    "<head>"
                    "    <metadata>"
                    "      <ttm:title>Document Metadata Example</ttm:title>"
                    "    </metadata>"
                    "    <styling>"
                    "      <style xml:id=\"normal\" "
                    "tts:fontFamily=\"sansSerif\"></style>"
                    "    </styling>"
                    "  </head>"
                    "<body style=\"normal\" region=\"bottom\">"
                    "<div><p>Subtitle 1-1<br/>"
                    "Subtitle <span style=\"italic\">1-2</span>"
                    "</p></div></body></tt>");
}

TEST_F(TimedTextTTMLSourceTest, parsingMultipleSpaces) {
    sp<MediaSource> stub= new SSMediaSourceStub(
            kTTMLChunkWithMultipleSpaces,
            NELEM(kTTMLChunkWithMultipleSpaces));
    mSource = new TimedTextTTMLSource(stub);
    err = mSource->read(&startTimeUs, &endTimeUs, &parcel);
    EXPECT_EQ(OK, err);
    CheckStartTimeMs(parcel, 610000);
    CheckDataEquals(parcel,
                    " Subtitle 1-1 ",
                    "<tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
                    " xmlns:tts=\"http://www.w3.org/ns/ttml#styling\">"
                    "<head>"
                    "    <metadata>"
                    "      <ttm:title>Document Metadata Example</ttm:title>"
                    "    </metadata>"
                    "    <styling>"
                    "      <style xml:id=\"normal\" "
                    "tts:fontFamily=\"sansSerif\"></style>"
                    "    </styling>"
                    "  </head>"
                    "<body style=\"normal\" region=\"bottom\">"
                    "    <div>"
                    "      <p>"
                    "        Subtitle  1-1"
                    "      </p>"
                    "   </div>"
                    " </body></tt>");
}

TEST_F(TimedTextTTMLSourceTest, parsingMetaData) {
    sp<MediaSource> stub= new SSMediaSourceStub(
            kTTMLChunkWithMetaData,
            NELEM(kTTMLChunkWithMetaData));
    mSource = new TimedTextTTMLSource(stub);
    err = mSource->read(&startTimeUs, &endTimeUs, &parcel);
    EXPECT_EQ(OK, err);
    CheckStartTimeMs(parcel, 10000);
    CheckDataEquals(
            parcel,
            "",
            "<tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
            " xmlns:tts=\"http://www.w3.org/ns/ttml#styling\""
            " xmlns:ttm=\"http://www.w3.org/ns/ttml#metadata\""
            " xmlns:smpte=\"http://www.smpte-ra.org/schemas/2052-1/2010/smpte-tt\">"
            "<head>"
            "    <metadata>"
            "      <ttm:title>Document Metadata Example</ttm:title>"
            "    </metadata>"
            "    <styling>"
            "      <style xml:id=\"normal\" tts:fontFamily=\"sansSerif\"></style>"
            "    </styling>"
            "  </head>"
            "<body style=\"normal\">"
            "<div><p>"
            "<metadata>"
            "<smpte:image imagetype=\"PNG\" encoding=\"Base64\">"
            "  iVBO...gg=="
            "</smpte:image>"
            "</metadata>"
            "</p></div></body></tt>");
}

TEST_F(TimedTextTTMLSourceTest, readEmptyChunk) {
    sp<MediaSource> stub= new SSMediaSourceStub(
            kTTMLEmptyChunk, NELEM(kTTMLEmptyChunk));
    mSource = new TimedTextTTMLSource(stub);

    err = mSource->read(&startTimeUs, &endTimeUs, &parcel);
    EXPECT_EQ(err, NOT_ENOUGH_DATA);
}

TEST_F(TimedTextTTMLSourceTest, readIncludingEmptyChunk) {
    sp<MediaSource> stub= new SSMediaSourceStub(
            kTTMLChunksIncludingEmptyChunk,
            NELEM(kTTMLChunksIncludingEmptyChunk));
    mSource = new TimedTextTTMLSource(stub);

    for (int i = 0; i < 3; i++) {
        err = mSource->read(&startTimeUs, &endTimeUs, &parcel);
        EXPECT_EQ(err, OK);

        CheckStartTimeMs(parcel, (i * 2 + 1) * 10000);
        AString subtitle = StringPrintf("Subtitle 1-%d", i+1);
        CheckSubtitleStartsWith(parcel, subtitle.c_str());
        parcel.freeData();
    }
    for (int i = 0; i < 3; i++) {
        err = mSource->read(&startTimeUs, &endTimeUs, &parcel);
        EXPECT_EQ(err, OK);

        CheckStartTimeMs(parcel, 2 * 60000 + (i * 2 + 1) * 10000);
        AString subtitle = StringPrintf("Subtitle 3-%d", i+1);
        CheckSubtitleStartsWith(parcel, "Subtitle");
        parcel.freeData();
    }
    err = mSource->read(&startTimeUs, &endTimeUs, &parcel);
    EXPECT_EQ(err, NOT_ENOUGH_DATA);
}

TEST_F(TimedTextTTMLSourceTest, readAll) {
    sp<MediaSource> stub= new SSMediaSourceStub(
            kTTMLChunks, NELEM(kTTMLChunks));
    mSource = new TimedTextTTMLSource(stub);

    for (int chunkIndex = 0; chunkIndex < 3; chunkIndex++) {
        for (int i = 0; i < 3; i++) {
            err = mSource->read(&startTimeUs, &endTimeUs, &parcel);
            EXPECT_EQ(err, OK);

            // Calculate expected start time:
            // 1. each chunk has a start offset : chunkIndex * 60 sec.
            // 2. <div> tag has begin time : chunkIndex * 60 sec.
            // 3. each subtitle has begin time : (i * 2 + 1) * 10 sec.
            CheckStartTimeMs(parcel,
                             chunkIndex * 60000 * 2 + (i * 2 + 1) * 10000);
            CheckSubtitleStartsWith(parcel, "Subtitle");
            Vector<const char*> tags;
            tags.push(TTMLUtils::kTagTT);
            tags.push(TTMLUtils::kTagHead);
            tags.push(TTMLUtils::kTagStyling);
            tags.push(TTMLUtils::kTagStyle);
            tags.push(TTMLUtils::kTagBody);
            tags.push(TTMLUtils::kTagDiv);
            tags.push(TTMLUtils::kTagP);
            CheckFragmentStartsWith(parcel, "<tt");
            CheckFragmentContains(parcel, tags);
            CheckFragmentEndsWith(parcel, "</tt>");
            parcel.freeData();
        }
    }
    err = mSource->read(&startTimeUs, &endTimeUs, &parcel);
    EXPECT_EQ(err, NOT_ENOUGH_DATA);
}

}  // namespace test
}  // namespace android
