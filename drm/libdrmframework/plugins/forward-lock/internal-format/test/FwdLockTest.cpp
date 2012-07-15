/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include <gtest/gtest.h>

extern "C" {
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
}

#include "FwdLockConv.h"
#include "FwdLockFile.h"
#include "FwdLockGlue.h"

#define INVALID_OFFSET ((off64_t)-1)

#define MAX_NUM_SESSIONS 32
#define MAX_NUM_OPEN_FILES 128

#define KEY_SIZE 16
#define SHA1_HASH_SIZE 20

#define CONTENT_TYPE_LENGTH_POS 7
#define CONTENT_TYPE_POS 8
#define CONTENT_TYPE_LENGTH 9
#define KEY_POS (CONTENT_TYPE_POS + CONTENT_TYPE_LENGTH)

#define READ_BUFFER_SIZE 1023

#define NUM_CRC_TABLE_ENTRIES 256

#define TEST_DATA_DIR "/data/drm/fwdlock-test"

static const char *invalidEncodings[] = {
    "x", "bx", "bix", "binx", "binax", "binarx", "binaryx", "bax", "basx", "basex", "base6x",
    "base64x", "7x", "7bx", "7bix", "7bitx"
};

static const size_t numInvalidEncodings = sizeof invalidEncodings / sizeof *invalidEncodings;

template<typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &os,
        FwdLockConv_Status_t status) {
    return os << static_cast<int>(status);
}

class FwdLockTest : public testing::Test {
protected:
    FwdLockTest() { InitCrcTable(); }
    bool CheckConvertedFile(const char *pFilename,
                            const char *pExpectedContentType,
                            unsigned int expectedChecksum,
                            off64_t expectedFileSize);
    virtual void SetUp() { ASSERT_EQ(0, chdir(TEST_DATA_DIR)); }
private:
    unsigned int mCrcTable[NUM_CRC_TABLE_ENTRIES];
    unsigned char mReadBuffer[READ_BUFFER_SIZE];
    void InitCrcTable();
    unsigned int crc32(unsigned int crc, void *pBuffer, ssize_t numBytes);
};

/**
 * Initializes a lookup table to speed up the CRC-32 calculation.
 */
void FwdLockTest::InitCrcTable() {
    for (unsigned int i = 0; i < NUM_CRC_TABLE_ENTRIES; ++i) {
        unsigned int crc = i << 24;
        for (int j = 0; j < 8; ++j) {
            crc = (crc << 1) ^ (((crc & 0x80000000) != 0) ? 0x04C11DB7 : 0);
        }
        mCrcTable[i] = crc;
    }
}

/**
 * Calculates a CRC-32 checksum incrementally using the same polynomial as the cksum command.
 */
unsigned int FwdLockTest::crc32(unsigned int crc, void *pBuffer, ssize_t numBytes) {
    unsigned char *ptr = static_cast<unsigned char *>(pBuffer);
    for (ssize_t i = 0; i < numBytes; ++i) {
        crc = (crc << 8) ^ mCrcTable[(crc >> 24) ^ *ptr++];
    }
    return crc;
}

bool FwdLockTest::CheckConvertedFile(const char *pFilename,
                                     const char *pExpectedContentType,
                                     unsigned int expectedChecksum,
                                     off64_t expectedFileSize) {
    bool result = false;
    int fileDesc = FwdLockFile_open(pFilename);
    if (fileDesc >= 0) {
        if (FwdLockFile_CheckIntegrity(fileDesc) &&
                strcmp(FwdLockFile_GetContentType(fileDesc), pExpectedContentType) == 0 &&
                FwdLockFile_lseek(fileDesc, 0, SEEK_END) == expectedFileSize &&
                FwdLockFile_lseek(fileDesc, 0, SEEK_SET) == 0) {
            ssize_t numBytesRead;
            unsigned int checksum = 0;
            while ((numBytesRead = FwdLockFile_read(fileDesc, mReadBuffer, READ_BUFFER_SIZE)) > 0) {
                checksum = crc32(checksum, mReadBuffer, numBytesRead);
            }
            if (numBytesRead == 0) {
                off64_t fileSize = FwdLockFile_lseek(fileDesc, 0, SEEK_CUR);
                if (fileSize == expectedFileSize) {
                    while (fileSize > 0) {
                        unsigned char ch = (unsigned char)fileSize;
                        checksum = crc32(checksum, &ch, sizeof ch);
                        fileSize >>= 8;
                    }
                    checksum ^= 0xFFFFFFFF;
                    if (checksum == expectedChecksum) {
                        result = true;
                    }
                }
            }
        }
        (void)FwdLockFile_close(fileDesc);
    }
    remove(pFilename);
    return result;
}

static int CreateForwardLockFile(const char *pFilename) {
    static const char fileContent[] = "---\r\n\r\n\r\n-----";
    int fileDesc =
            open(pFilename, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fileDesc >= 0 &&
            (write(fileDesc, fileContent, sizeof fileContent - 1) != sizeof fileContent - 1 ||
                    lseek64(fileDesc, 0, SEEK_SET) != 0)) {
        (void)close(fileDesc);
        fileDesc = -1;
    }
    return fileDesc;
}

static int SafeClose(int fileDesc) {
    return (fileDesc >= 0) ? close(fileDesc) : -1;
}

static bool FileExists(const char *pFilename) {
    return access(pFilename, F_OK) == 0;
}

TEST_F(FwdLockTest, ConverterWhiteBoxNormalCase01) {
    (void)SafeClose(CreateForwardLockFile(".dm"));
    EXPECT_EQ(FwdLockConv_Status_OK, FwdLockConv_ConvertFile(".dm", ".fl", NULL));
    (void)remove(".dm");
    EXPECT_TRUE(CheckConvertedFile(".fl", "text/plain", 0xFFFFFFFF, 0));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNormalCase02) {
    int inputFileDesc = CreateForwardLockFile(".dm");
    int outputFileDesc = creat(".fl", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    EXPECT_EQ(FwdLockConv_Status_OK,
              FwdLockConv_ConvertOpenFile(inputFileDesc, read, outputFileDesc, write, lseek64,
                      NULL));
    (void)SafeClose(inputFileDesc);
    (void)SafeClose(outputFileDesc);
    (void)remove(".dm");
    EXPECT_TRUE(CheckConvertedFile(".fl", "text/plain", 0xFFFFFFFF, 0));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNullCheck01) {
    FwdLockConv_Output_t output;
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument, FwdLockConv_OpenSession(NULL, &output));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNullCheck02) {
    int sessionId;
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument, FwdLockConv_OpenSession(&sessionId, NULL));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNullCheck03) {
    FwdLockConv_Output_t output;
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument, FwdLockConv_ConvertData(0, NULL, 0, &output));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNullCheck04) {
    char buffer[1];
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument,
              FwdLockConv_ConvertData(0, buffer, sizeof buffer, NULL));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNullCheck05) {
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument, FwdLockConv_CloseSession(0, NULL));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNullCheck06) {
    off64_t errorPos;
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument,
              FwdLockConv_ConvertOpenFile(0, NULL, 0, write, lseek64, &errorPos));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNullCheck07) {
    off64_t errorPos;
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument,
              FwdLockConv_ConvertOpenFile(0, read, 0, NULL, lseek64, &errorPos));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNullCheck08) {
    off64_t errorPos;
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument,
              FwdLockConv_ConvertOpenFile(0, read, 0, write, NULL, &errorPos));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNullCheck09) {
    off64_t errorPos;
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument, FwdLockConv_ConvertFile(NULL, "", &errorPos));
}

TEST_F(FwdLockTest, ConverterWhiteBoxNullCheck10) {
    off64_t errorPos;
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument, FwdLockConv_ConvertFile("", NULL, &errorPos));
}

TEST_F(FwdLockTest, ConverterWhiteBoxInvalidSession01) {
    char buffer[1];
    FwdLockConv_Output_t output;
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument,
              FwdLockConv_ConvertData(-1, buffer, sizeof buffer, &output));
}

TEST_F(FwdLockTest, ConverterWhiteBoxInvalidSession02) {
    FwdLockConv_Output_t output;
    EXPECT_EQ(FwdLockConv_Status_InvalidArgument, FwdLockConv_CloseSession(-1, &output));
}

TEST_F(FwdLockTest, ConverterWhiteBoxFileNotFound) {
    EXPECT_EQ(FwdLockConv_Status_FileNotFound, FwdLockConv_ConvertFile("", ".fl", NULL));
}

TEST_F(FwdLockTest, ConverterWhiteBoxFileCreationFailed) {
    (void)SafeClose(CreateForwardLockFile(".dm"));
    EXPECT_EQ(FwdLockConv_Status_FileCreationFailed, FwdLockConv_ConvertFile(".dm", "", NULL));
    (void)remove(".dm");
}

static ssize_t FailingRead(int fileDesc, void *pBuffer, size_t numBytes) {
    return -1;
}

TEST_F(FwdLockTest, ConverterWhiteBoxFileReadError) {
    int inputFileDesc = CreateForwardLockFile(".dm");
    int outputFileDesc = creat(".fl", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    EXPECT_EQ(FwdLockConv_Status_FileReadError,
              FwdLockConv_ConvertOpenFile(inputFileDesc, FailingRead, outputFileDesc, write,
                                          lseek64, NULL));
    (void)SafeClose(inputFileDesc);
    (void)SafeClose(outputFileDesc);
    (void)remove(".dm");
    (void)remove(".fl");
}

static ssize_t FailingWrite(int fileDesc, const void *pBuffer, size_t numBytes) {
    return -1;
}

TEST_F(FwdLockTest, ConverterWhiteBoxFileWriteError) {
    int inputFileDesc = CreateForwardLockFile(".dm");
    int outputFileDesc = creat(".fl", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    EXPECT_EQ(FwdLockConv_Status_FileWriteError,
              FwdLockConv_ConvertOpenFile(inputFileDesc, read, outputFileDesc, FailingWrite,
                                          lseek64, NULL));
    (void)SafeClose(inputFileDesc);
    (void)SafeClose(outputFileDesc);
    (void)remove(".dm");
    (void)remove(".fl");
}

static off64_t FailingLSeek(int fileDesc, off64_t offset, int whence) {
    return INVALID_OFFSET;
}

TEST_F(FwdLockTest, ConverterWhiteBoxFileSeekError) {
    int inputFileDesc = CreateForwardLockFile(".dm");
    int outputFileDesc = creat(".fl", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    EXPECT_EQ(FwdLockConv_Status_FileSeekError,
              FwdLockConv_ConvertOpenFile(inputFileDesc, read, outputFileDesc, write, FailingLSeek,
                                          NULL));
    (void)SafeClose(inputFileDesc);
    (void)SafeClose(outputFileDesc);
    (void)remove(".dm");
    (void)remove(".fl");
}

TEST_F(FwdLockTest, ConverterWhiteBoxEmptyFile) {
    off64_t errorPos;
    (void)SafeClose(creat(".dm", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    EXPECT_EQ(FwdLockConv_Status_SyntaxError, FwdLockConv_ConvertFile(".dm", ".fl", &errorPos));
    (void)remove(".dm");
    EXPECT_EQ(0, errorPos);
    EXPECT_FALSE(FileExists(".fl"));
}

TEST_F(FwdLockTest, ConverterWhiteBoxTooManySessions) {
    int sessionIds[MAX_NUM_SESSIONS];
    FwdLockConv_Output_t outputs[MAX_NUM_SESSIONS];
    for (int i = 0; i < MAX_NUM_SESSIONS; ++i) {
        sessionIds[i] = -1;
        EXPECT_EQ(FwdLockConv_Status_OK, FwdLockConv_OpenSession(&sessionIds[i], &outputs[i]));
        EXPECT_GE(sessionIds[i], 0);
    }
    int sessionId = 0;
    FwdLockConv_Output_t output;
    FwdLockConv_Status_t status = FwdLockConv_OpenSession(&sessionId, &output);
    EXPECT_EQ(FwdLockConv_Status_TooManySessions, status);
    EXPECT_LT(sessionId, 0);
    if (status == FwdLockConv_Status_OK) {
        EXPECT_EQ(FwdLockConv_Status_SyntaxError, FwdLockConv_CloseSession(sessionId, &output));
    }
    for (int i = 0; i < MAX_NUM_SESSIONS; ++i) {
        if (sessionIds[i] >= 0) {
            EXPECT_EQ(FwdLockConv_Status_SyntaxError,
                      FwdLockConv_CloseSession(sessionIds[i], &outputs[i]));
        }
    }
    sessionId = -1;
    status = FwdLockConv_OpenSession(&sessionId, &output);
    EXPECT_EQ(FwdLockConv_Status_OK, status);
    EXPECT_GE(sessionId, 0);
    if (status == FwdLockConv_Status_OK) {
        EXPECT_EQ(FwdLockConv_Status_SyntaxError, FwdLockConv_CloseSession(sessionId, &output));
    }
}

static void TryEncoding(const char *pEncoding) {
    int sessionId;
    FwdLockConv_Output_t output;
    FwdLockConv_Status_t status = FwdLockConv_OpenSession(&sessionId, &output);
    EXPECT_EQ(FwdLockConv_Status_OK, status);
    if (status == FwdLockConv_Status_OK) {
        static const char buffer[] = "---\r\nContent-Transfer-Encoding: ";
        status = FwdLockConv_ConvertData(sessionId, buffer, sizeof buffer - 1, &output);
        EXPECT_EQ(FwdLockConv_Status_OK, status);
        if (status == FwdLockConv_Status_OK) {
            EXPECT_EQ(FwdLockConv_Status_UnsupportedContentTransferEncoding,
                      FwdLockConv_ConvertData(sessionId, pEncoding, strlen(pEncoding), &output));
        }
        EXPECT_EQ(FwdLockConv_Status_SyntaxError, FwdLockConv_CloseSession(sessionId, &output));
    }
}

TEST_F(FwdLockTest, ConverterWhiteBoxInvalidEncoding) {
    for (size_t i = 0; i < numInvalidEncodings; ++i) {
        TryEncoding(invalidEncodings[i]);
    }
}

TEST_F(FwdLockTest, ConverterBlackBoxBinary) {
    ASSERT_EQ(FwdLockConv_Status_OK, FwdLockConv_ConvertFile("binary.dm", "binary.fl", NULL));
    EXPECT_TRUE(CheckConvertedFile("binary.fl", "image/png", 146838918, 101));
}

TEST_F(FwdLockTest, ConverterBlackBoxBase64) {
    ASSERT_EQ(FwdLockConv_Status_OK, FwdLockConv_ConvertFile("base64.dm", "base64.fl", NULL));
    EXPECT_TRUE(CheckConvertedFile("base64.fl", "image/png", 146838918, 101));
}

TEST_F(FwdLockTest, ConverterBlackBox7bit) {
    ASSERT_EQ(FwdLockConv_Status_OK, FwdLockConv_ConvertFile("7bit.dm", "7bit.fl", NULL));
    EXPECT_TRUE(CheckConvertedFile("7bit.fl", "image/png", 146838918, 101));
}

TEST_F(FwdLockTest, ConverterBlackBox8bit) {
    ASSERT_EQ(FwdLockConv_Status_OK, FwdLockConv_ConvertFile("8bit.dm", "8bit.fl", NULL));
    EXPECT_TRUE(CheckConvertedFile("8bit.fl", "image/png", 146838918, 101));
}

TEST_F(FwdLockTest, ConverterBlackBoxCombinedDelivery) {
    ASSERT_EQ(FwdLockConv_Status_UnsupportedFileFormat,
              FwdLockConv_ConvertFile("combined-delivery.dm", "combined-delivery.fl", NULL));
    EXPECT_FALSE(FileExists("combined-delivery.fl"));
}

TEST_F(FwdLockTest, ConverterBlackBoxSeparateDelivery) {
    ASSERT_EQ(FwdLockConv_Status_UnsupportedFileFormat,
              FwdLockConv_ConvertFile("separate-delivery.dm", "separate-delivery.fl", NULL));
    EXPECT_FALSE(FileExists("separate-delivery.fl"));
}

TEST_F(FwdLockTest, ConverterBlackBoxPreamble) {
    ASSERT_EQ(FwdLockConv_Status_OK, FwdLockConv_ConvertFile("preamble.dm", "preamble.fl", NULL));
    EXPECT_TRUE(CheckConvertedFile("preamble.fl", "image/png", 146838918, 101));
}

TEST_F(FwdLockTest, ConverterBlackBoxWhitespaceAfterBoundary) {
    ASSERT_EQ(FwdLockConv_Status_OK,
              FwdLockConv_ConvertFile("whitespace-after-boundary.dm",
                                      "whitespace-after-boundary.fl", NULL));
    EXPECT_TRUE(CheckConvertedFile("whitespace-after-boundary.fl", "image/png", 146838918, 101));
}

TEST_F(FwdLockTest, ConverterBlackBoxBoundaryTooLong) {
    off64_t errorPos;
    ASSERT_EQ(FwdLockConv_Status_SyntaxError,
              FwdLockConv_ConvertFile("boundary-too-long.dm", "boundary-too-long.fl", &errorPos));
    EXPECT_EQ(73, errorPos);
    EXPECT_FALSE(FileExists("boundary-too-long.fl"));
}

TEST_F(FwdLockTest, ConverterBlackBoxDelimiterTruncated) {
    off64_t errorPos;
    ASSERT_EQ(FwdLockConv_Status_SyntaxError,
              FwdLockConv_ConvertFile("delimiter-truncated.dm", "delimiter-truncated.fl",
                                      &errorPos));
    EXPECT_EQ(213, errorPos);
    EXPECT_FALSE(FileExists("delimiter-truncated.fl"));
}

TEST_F(FwdLockTest, ConverterBlackBoxfalseMatchJustBeforeDelimiter) {
    ASSERT_EQ(FwdLockConv_Status_OK,
              FwdLockConv_ConvertFile("false-match-just-before-delimiter.dm",
                                      "false-match-just-before-delimiter.fl", NULL));
    EXPECT_TRUE(CheckConvertedFile("false-match-just-before-delimiter.fl", "text/plain",
                                   3568853871U, 2));
}

TEST_F(FwdLockTest, ConverterBlackBoxInterspersedWhitespace) {
    ASSERT_EQ(FwdLockConv_Status_OK,
              FwdLockConv_ConvertFile("interspersed-whitespace.dm",
                                      "interspersed-whitespace.fl", NULL));
    EXPECT_TRUE(CheckConvertedFile("interspersed-whitespace.fl", "text/plain", 1585990281, 5));
}

TEST_F(FwdLockTest, ConverterBlackBoxNonWhitespaceAfterPadding) {
    off64_t errorPos;
    ASSERT_EQ(FwdLockConv_Status_SyntaxError,
              FwdLockConv_ConvertFile("non-whitespace-after-padding.dm",
                                      "non-whitespace-after-padding.fl", &errorPos));
    EXPECT_EQ(78, errorPos);
    EXPECT_FALSE(FileExists("non-whitespace-after-padding.fl"));
}

TEST_F(FwdLockTest, DecoderWhiteBoxFileNotFound) {
    errno = 0;
    EXPECT_EQ(-1, FwdLockFile_open(""));
    EXPECT_EQ(ENOENT, errno);
}

TEST_F(FwdLockTest, DecoderWhiteBoxNullFilename) {
    errno = 0;
    EXPECT_EQ(-1, FwdLockFile_open(NULL));
    EXPECT_EQ(EFAULT, errno);
}

TEST_F(FwdLockTest, DecoderWhiteBoxNullReadBuffer) {
    (void)SafeClose(CreateForwardLockFile(".dm"));
    EXPECT_EQ(FwdLockConv_Status_OK, FwdLockConv_ConvertFile(".dm", ".fl", NULL));
    int fileDesc = FwdLockFile_open(".fl");
    EXPECT_GE(fileDesc, 0);
    if (fileDesc >= 0) {
        EXPECT_EQ(0, FwdLockFile_read(fileDesc, NULL, 1));
        (void)FwdLockFile_close(fileDesc);
    }
    (void)remove(".dm");
    (void)remove(".fl");
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidFileDescriptor01) {
    errno = 0;
    EXPECT_EQ(-1, FwdLockFile_attach(-1));
    EXPECT_EQ(EBADF, errno);
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidFileDescriptor02) {
    errno = 0;
    EXPECT_EQ(-1, FwdLockFile_read(-1, NULL, 0));
    EXPECT_EQ(EBADF, errno);
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidFileDescriptor03) {
    errno = 0;
    EXPECT_EQ(-1, FwdLockFile_lseek(-1, 0, SEEK_SET));
    EXPECT_EQ(EBADF, errno);
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidFileDescriptor04) {
    errno = 0;
    EXPECT_EQ(-1, FwdLockFile_detach(-1));
    EXPECT_EQ(EBADF, errno);
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidFileDescriptor05) {
    errno = 0;
    EXPECT_EQ(-1, FwdLockFile_close(-1));
    EXPECT_EQ(EBADF, errno);
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidFileDescriptor06) {
    errno = 0;
    EXPECT_FALSE(FwdLockFile_CheckDataIntegrity(-1));
    EXPECT_EQ(EBADF, errno);
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidFileDescriptor07) {
    errno = 0;
    EXPECT_FALSE(FwdLockFile_CheckHeaderIntegrity(-1));
    EXPECT_EQ(EBADF, errno);
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidFileDescriptor08) {
    errno = 0;
    EXPECT_FALSE(FwdLockFile_CheckIntegrity(-1));
    EXPECT_EQ(EBADF, errno);
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidFileDescriptor09) {
    errno = 0;
    EXPECT_EQ(NULL, FwdLockFile_GetContentType(-1));
    EXPECT_EQ(EBADF, errno);
}

static void SetInvalidFilePos(off64_t offset, int whence) {
    (void)SafeClose(CreateForwardLockFile(".dm"));
    EXPECT_EQ(FwdLockConv_Status_OK, FwdLockConv_ConvertFile(".dm", ".fl", NULL));
    int fileDesc = FwdLockFile_open(".fl");
    EXPECT_GE(fileDesc, 0);
    if (fileDesc >= 0) {
        EXPECT_EQ(INVALID_OFFSET, FwdLockFile_lseek(fileDesc, offset, whence));
        (void)FwdLockFile_close(fileDesc);
    }
    (void)remove(".dm");
    (void)remove(".fl");
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidOffset01) {
    SetInvalidFilePos(-1, SEEK_SET); // Would set the offset inside the header part of the file.
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidOffset02) {
    SetInvalidFilePos(-1, SEEK_CUR); // Would set the offset inside the header part of the file.
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidOffset03) {
    SetInvalidFilePos(-1, SEEK_END); // Would set the offset inside the header part of the file.
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidOffset04) {
    SetInvalidFilePos(-100, SEEK_SET); // Would set the offset before the beginning of the file.
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidOffset05) {
    SetInvalidFilePos(-100, SEEK_CUR); // Would set the offset before the beginning of the file.
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidOffset06) {
    SetInvalidFilePos(-100, SEEK_END); // Would set the offset before the beginning of the file.
}

TEST_F(FwdLockTest, DecoderWhiteBoxInvalidOrigin) {
    SetInvalidFilePos(0, -1);
}

TEST_F(FwdLockTest, DecoderWhiteBoxTooManyOpenFiles) {
    errno = 0;
    (void)SafeClose(CreateForwardLockFile(".dm"));
    EXPECT_EQ(FwdLockConv_Status_OK, FwdLockConv_ConvertFile(".dm", ".fl", NULL));
    int fileDescs[MAX_NUM_OPEN_FILES];
    for (int i = 0; i < MAX_NUM_OPEN_FILES; ++i) {
        fileDescs[i] = FwdLockFile_open(".fl");
        EXPECT_GE(fileDescs[i], 0);
    }
    int fileDesc = FwdLockFile_open(".fl");
    EXPECT_LT(fileDesc, 0);
    if (fileDesc >= 0) {
        (void)FwdLockFile_close(fileDesc);
    }
    for (int i = 0; i < MAX_NUM_OPEN_FILES; ++i) {
        if (fileDescs[i] >= 0) {
            (void)FwdLockFile_close(fileDescs[i]);
        }
    }
    fileDesc = FwdLockFile_open(".fl");
    EXPECT_GE(fileDesc, 0);
    if (fileDesc >= 0) {
        (void)FwdLockFile_close(fileDesc);
    }
    (void)remove(".dm");
    (void)remove(".fl");
    EXPECT_EQ(ENFILE, errno);
}

static void ManipulateIntegrity(off64_t filePos,
                                unsigned char byte,
                                bool expectedDataResult,
                                bool expectedHeaderResult,
                                bool shouldFailOnOpen = false) {
    ASSERT_EQ(FwdLockConv_Status_OK, FwdLockConv_ConvertFile("binary.dm", "binary.fl", NULL));
    int fileDesc = open("binary.fl", O_WRONLY);
    EXPECT_GE(fileDesc, 0);
    if (fileDesc >= 0) {
        EXPECT_EQ(filePos, lseek64(fileDesc, filePos, SEEK_SET));
        EXPECT_EQ(static_cast<ssize_t>(sizeof byte), write(fileDesc, &byte, sizeof byte));
        ASSERT_EQ(0, close(fileDesc));
        fileDesc = FwdLockFile_open("binary.fl");
        if (shouldFailOnOpen) {
            EXPECT_LT(fileDesc, 0);
        } else {
            EXPECT_GE(fileDesc, 0);
        }
        if (fileDesc >= 0) {
            EXPECT_EQ(expectedDataResult, FwdLockFile_CheckDataIntegrity(fileDesc));
            EXPECT_EQ(expectedHeaderResult, FwdLockFile_CheckHeaderIntegrity(fileDesc));
            ASSERT_EQ(0, FwdLockFile_close(fileDesc));
        }
    }
    (void)remove("binary.fl");
}

TEST_F(FwdLockTest, DecoderBlackBoxCheckIntegrity01) {
    ManipulateIntegrity(CONTENT_TYPE_LENGTH_POS, UCHAR_MAX, false, false, true);
}

TEST_F(FwdLockTest, DecoderBlackBoxCheckIntegrity02) {
    ManipulateIntegrity(CONTENT_TYPE_LENGTH_POS, 0, false, false, true);
}

TEST_F(FwdLockTest, DecoderBlackBoxCheckIntegrity03) {
    ManipulateIntegrity(CONTENT_TYPE_POS, 0, true, false);
}

TEST_F(FwdLockTest, DecoderBlackBoxCheckIntegrity04) {
    ManipulateIntegrity(KEY_POS + KEY_SIZE - 1, 0, false, false, true);
}

TEST_F(FwdLockTest, DecoderBlackBoxCheckIntegrity05) {
    const off64_t dataSignaturePos = KEY_POS + FwdLockGlue_GetEncryptedKeyLength(KEY_SIZE);
    ManipulateIntegrity(dataSignaturePos, 0, false, false);
}

TEST_F(FwdLockTest, DecoderBlackBoxCheckIntegrity06) {
    const off64_t headerSignaturePos = KEY_POS + FwdLockGlue_GetEncryptedKeyLength(KEY_SIZE) +
            SHA1_HASH_SIZE;
    ManipulateIntegrity(headerSignaturePos, 0, true, false);
}

TEST_F(FwdLockTest, DecoderBlackBoxCheckIntegrity07) {
    const off64_t dataPos = KEY_POS + FwdLockGlue_GetEncryptedKeyLength(KEY_SIZE) +
            (2 * SHA1_HASH_SIZE);
    ManipulateIntegrity(dataPos, 0, false, true);
}
