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

#define LOG_TAG "TTMLUtils_test"
#include <utils/Log.h>

#include <gtest/gtest.h>

#include <TTMLUtils.h>

#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/MediaErrors.h>

namespace android {
namespace test {

class TTMLUtilsTest : public testing::Test {
protected:
    static const int32_t kDummyFrameRate = 24;
    static const int32_t kDummySubFrameRate = 10;
    static const int32_t kDummyTickRate = 30;
    void SetUp() { }

    int32_t readDigitsFromText(const char **pos, int32_t *readSize) {
        return TTMLUtils::readDigitsFromText(pos, readSize);
    }

    double readFractionFromText(const char **pos, int32_t *readSize) {
        return TTMLUtils::readFractionFromText(pos, readSize);
    }

    status_t getTimeFromText(const AString &text, int64_t *timeUs,
            int32_t frameRate, int32_t subFrameRate, int32_t tickRate) {
        return TTMLUtils::getTimeFromText(text, timeUs, frameRate, subFrameRate, tickRate);
    }

    int64_t getTimeUs(int32_t hour, int32_t min, double sec) {
        return static_cast<int64_t>(
                (hour * 60 * 60 + min * 60 + sec) * 1000 * 1000);
    }
};

TEST_F(TTMLUtilsTest, ReadDigitsFromText_NULL) {
    const char *text = "";
    const char *pos = text;

    int32_t readSize;
    int32_t value = readDigitsFromText(&pos, &readSize);
    EXPECT_EQ(0, value);
    EXPECT_EQ(0, readSize);
    EXPECT_EQ('\0', *pos);
}

TEST_F(TTMLUtilsTest, ReadDigitsFromText_Integer) {
    const char *text = "123";
    const char *pos = text;

    int32_t readSize;
    int32_t value = readDigitsFromText(&pos, &readSize);
    EXPECT_EQ(123, value);
    EXPECT_EQ(3, readSize);
    EXPECT_EQ('\0', *pos);
}

TEST_F(TTMLUtilsTest, ReadDigitsFromText_IntegerWithNoneDigit) {
    const char *text = "123a";
    const char *pos = text;

    int32_t readSize;
    int32_t value = readDigitsFromText(&pos, &readSize);
    EXPECT_EQ(123, value);
    EXPECT_EQ(3, readSize);
    EXPECT_EQ('a', *pos);
}

TEST_F(TTMLUtilsTest, ReadDigitsFromText_Float) {
    const char *text = "123.456";
    const char *pos = text;

    int32_t readSize;
    int32_t value = readDigitsFromText(&pos, &readSize);
    EXPECT_EQ(123, value);
    EXPECT_EQ(3, readSize);
    EXPECT_EQ('.', *pos);
}

TEST_F(TTMLUtilsTest, ReadDigitsFromText_NoneDigit) {
    const char *text = "abc";
    const char *pos = text;

    int32_t readSize;
    int32_t value = readDigitsFromText(&pos, &readSize);
    EXPECT_EQ(0, value);
    EXPECT_EQ(0, readSize);
    EXPECT_EQ('a', *pos);
}

TEST_F(TTMLUtilsTest, ReadDigitsFromText_Complex) {
    const char *text = "1.2.4.abc";
    const char *pos = text;

    int32_t readSize;
    int32_t value = readDigitsFromText(&pos, &readSize);
    EXPECT_EQ(1, value);
    EXPECT_EQ(1, readSize);
    EXPECT_EQ('.', *pos);

    ++pos;
    value = readDigitsFromText(&pos, &readSize);
    EXPECT_EQ(2, value);
    EXPECT_EQ(1, readSize);
    EXPECT_EQ('.', *pos);

    ++pos;
    value = readDigitsFromText(&pos, &readSize);
    EXPECT_EQ(4, value);
    EXPECT_EQ(1, readSize);
    EXPECT_EQ('.', *pos);

    ++pos;
    value = readDigitsFromText(&pos, &readSize);
    EXPECT_EQ(0, value);
    EXPECT_EQ(0, readSize);
    EXPECT_EQ('a', *pos);
}

TEST_F(TTMLUtilsTest, ReadFractionFromText_NULL) {
    const char *text = "";
    const char *pos = text;

    int32_t readSize;
    double value = readFractionFromText(&pos, &readSize);
    EXPECT_DOUBLE_EQ(-1.0, value);
    EXPECT_EQ(0, readSize);
    EXPECT_EQ('\0', *pos);
}

TEST_F(TTMLUtilsTest, ReadFractionFromText_Integer) {
    const char *text = "123";
    const char *pos = text;

    int32_t readSize;
    double value = readFractionFromText(&pos, &readSize);
    EXPECT_DOUBLE_EQ(-1.0, value);
    EXPECT_EQ(0, readSize);
    EXPECT_EQ('1', *pos);
}

TEST_F(TTMLUtilsTest, ReadFractionFromText_Fraction) {
    const char *text = ".124";
    const char *pos = text;

    int32_t readSize;
    double value = readFractionFromText(&pos, &readSize);
    EXPECT_DOUBLE_EQ(0.124, value);
    EXPECT_EQ(4, readSize);
    EXPECT_EQ('\0', *pos);
}

TEST_F(TTMLUtilsTest, ReadFractionFromText_FractionWithNoneDigit) {
    const char *text = ".124a";
    const char *pos = text;

    int32_t readSize;
    double value = readFractionFromText(&pos, &readSize);
    EXPECT_DOUBLE_EQ(0.124, value);
    EXPECT_EQ(4, readSize);
    EXPECT_EQ('a', *pos);
}

TEST_F(TTMLUtilsTest, ReadFractionFromText_NoneDigit) {
    const char *text = "abc";
    const char *pos = text;

    int32_t readSize;
    double value = readFractionFromText(&pos, &readSize);
    EXPECT_DOUBLE_EQ(-1.0, value);
    EXPECT_EQ(0, readSize);
    EXPECT_EQ('a', *pos);
}

TEST_F(TTMLUtilsTest, ReadFractionFromText_Complex) {
    const char *text = "1.2.4.abc";
    const char *pos = text;

    int32_t readSize;
    double value = readFractionFromText(&pos, &readSize);
    EXPECT_DOUBLE_EQ(-1.0, value);
    EXPECT_EQ(0, readSize);
    EXPECT_EQ('1', *pos);

    ++pos;
    value = readFractionFromText(&pos, &readSize);
    EXPECT_DOUBLE_EQ(0.2, value);
    EXPECT_EQ(2, readSize);
    EXPECT_EQ('.', *pos);

    value = readFractionFromText(&pos, &readSize);
    EXPECT_DOUBLE_EQ(0.4, value);
    EXPECT_EQ(2, readSize);
    EXPECT_EQ('.', *pos);

    value = readFractionFromText(&pos, &readSize);
    EXPECT_DOUBLE_EQ(0.0, value);
    EXPECT_EQ(1, readSize);
    EXPECT_EQ('a', *pos);
}

TEST_F(TTMLUtilsTest, GetTimeFromText_NULL) {
    AString text = "";
    int64_t timeUs = 0;
    status_t ret;

    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);
}

TEST_F(TTMLUtilsTest, GetTimeFromText_ClockTime) {
    AString text;
    int64_t timeUs = 0;
    status_t ret;

    text.setTo("01:20:30");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(1, 20, 30), timeUs);

    text.setTo("01:59:60.5");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(1, 59, 60.5), timeUs);

    text.setTo("01:20:30.45");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(1, 20, 30.45), timeUs);

    text.setTo("111:00:00");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(111, 0, 0), timeUs);

    text.setTo("01:20:30:15");
    ret = getTimeFromText(text, &timeUs, 30, -1, -1);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(1, 20, 30 + 15.0 / 30.0), timeUs);

    text.setTo("01:20:30:15.5");
    ret = getTimeFromText(text, &timeUs, 30, 10, -1);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(1, 20, 30 + 15.5 / 30.0), timeUs);

    text.setTo("-01:20:30");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(1, 20, 30) * -1, timeUs);
}

TEST_F(TTMLUtilsTest, GetTimeFromText_IntegerOffsetTime) {
    AString text;
    int64_t timeUs = 0;
    status_t ret;

    text.setTo("123h");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(123, 0, 0), timeUs);

    text.setTo("123m");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(0, 123, 0), timeUs);

    text.setTo("123s");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(0, 0, 123), timeUs);

    text.setTo("123ms");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(123000, timeUs);

    text.setTo("15f");
    ret = getTimeFromText(text, &timeUs, 30, -1, -1);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(0, 0, 0.5), timeUs);

    text.setTo("123t");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, 1000 * 1000);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(123, timeUs);

    text.setTo("-123h");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(123, 0, 0) * -1, timeUs);
}

TEST_F(TTMLUtilsTest, GetTimeFromText_FloatOffsetTime) {
    AString text;
    int64_t timeUs = 0;
    status_t ret;

    text.setTo("1.5h");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(1, 30, 0), timeUs);

    text.setTo("1.5m");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(0, 1, 30), timeUs);

    text.setTo("1.5s");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(0, 0, 1.5), timeUs);

    text.setTo("1.5ms");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(1500, timeUs);

    text.setTo("1.5f");
    ret = getTimeFromText(text, &timeUs, 30, -1, -1);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(0, 0, 1.5 / 30.0), timeUs);

    text.setTo("1.5t");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, 100 * 1000);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(15, timeUs);

    text.setTo("-1.5h");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(OK, ret);
    EXPECT_EQ(getTimeUs(1, 30, 0) * -1, timeUs);
}

TEST_F(TTMLUtilsTest, GetTimeFromText_ClockTimeMalformed) {
    AString text;
    int64_t timeUs = 0;
    status_t ret;

    text.setTo("1:20:30");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("01:20:30:12.");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("01:60:30");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("01:59:61");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("01:20:30s");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("01:20:30.45:67");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("01:20:30:12:78");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("1.5:20:30:40");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("01:20:30.123:10");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);
}

TEST_F(TTMLUtilsTest, GetTimeFromText_OffsetTimeMalformed) {
    AString text;
    int64_t timeUs = 0;
    status_t ret;

    text.setTo("1hr");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("1.5");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("1.5us");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);

    text.setTo("1.h");
    ret = getTimeFromText(text, &timeUs, kDummyFrameRate, kDummySubFrameRate, kDummyTickRate);
    EXPECT_EQ(ERROR_MALFORMED, ret);
}

}  // namespace test
}  // namespace android
