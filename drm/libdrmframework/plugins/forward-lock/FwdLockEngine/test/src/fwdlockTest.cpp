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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <DrmConvertedStatus.h>
#include <DrmInfoStatus.h>
#include <DrmConstraints.h>
#include <DrmRights.h>
#include <DrmManagerClient.h>
#include <FwdLockEngine.h>
#include <drm_framework_common.h>
#include <DrmSupportInfo.h>
#include <IDrmEngine.h>
#include <errno.h>
#include <utils/String8.h>
#include <gtest/gtest.h>

namespace android {

#define TESTFILE1_DM       "/data/data/flenginetest/musictest.dm"
#define TESTFILE1_OUTFL    "/data/data/flenginetest/musictest.fl"
#define DRM_DM_MIMETYPE    "application/vnd.oma.drm.message"
#define DRM_FL_MIMETYPE    "application/x-android-drm-fl"
#define DRM_DM_EXTENSION   ".dm"
#define DRM_FL_EXTENSION   ".fl"
#define DRM_CONVERT_SIZE   1024

// Test class for test cases that access the FL agent directly
class OmaFLAgentTest : public ::testing::Test {

protected:
    virtual void SetUp() {
        flengine = static_cast<IDrmEngine *> (new FwdLockEngine());
        flengine->initialize(0);
    }

    virtual void TearDown() {
        flengine->terminate(0);
        delete flengine;
        flengine = NULL;
    }

    IDrmEngine *flengine;
};

// Test class for test cases that access the FL agent through DRM framework
class DrmManagerClientFLTest : public ::testing::Test {

protected:
    virtual void SetUp() {
        drmManagerClient = new DrmManagerClient();
    }

    virtual void TearDown() {
        delete drmManagerClient;
        drmManagerClient = NULL;
    }

    DrmManagerClient* drmManagerClient;
};

/**
 * Helper function that converts the .dm file to the internal forward lock format.
 */
static void convertFile(DrmManagerClient* drmManagerClient,
                        const char* drmFile,
                        const char* textFile) {
    int outFileDesc = -1;
    int inFileDesc = open(drmFile, O_RDONLY);
    // Check that file is opened
    EXPECT_TRUE(-1 < inFileDesc);
    if (-1 < inFileDesc) {
        outFileDesc = open(textFile, O_CREAT | O_TRUNC | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        // Check that file is opened
        EXPECT_TRUE(-1 < outFileDesc);
        if (-1 < outFileDesc) {
            ssize_t size = DRM_CONVERT_SIZE;
            char buff[DRM_CONVERT_SIZE];
            int convertID = drmManagerClient->openConvertSession(String8(DRM_DM_MIMETYPE));
            EXPECT_TRUE(0 <= convertID);
            while (0 <= convertID && 0 < size) {
                size = read(inFileDesc, buff, size);
                // Check that we could read the file
                EXPECT_TRUE((size > 0) && (size <= DRM_CONVERT_SIZE));
                if ((size > 0) && (size <= DRM_CONVERT_SIZE)) {
                    DrmBuffer* drmBuff = new DrmBuffer();
                    drmBuff->data = buff;
                    drmBuff->length = size;
                    DrmConvertedStatus* pConvertedData = NULL;
                    pConvertedData = drmManagerClient->convertData(convertID, drmBuff);
                    // Check that converted data went fine.
                    EXPECT_TRUE(NULL != pConvertedData);
                    EXPECT_TRUE(DrmConvertedStatus::STATUS_OK == pConvertedData->statusCode);
                    if (NULL != pConvertedData &&
                            (DrmConvertedStatus::STATUS_OK == pConvertedData->statusCode)) {
                        // if converted data is returned, write the the output file
                        if (NULL != pConvertedData->convertedData &&
                                NULL != pConvertedData->convertedData->data &&
                            pConvertedData->convertedData->length > 0) {
                            write(outFileDesc, pConvertedData->convertedData->data,
                                    pConvertedData->convertedData->length);
                        }
                    }
                    delete drmBuff;
                    if (NULL != pConvertedData) {
                        if (NULL != pConvertedData->convertedData) {
                            delete pConvertedData->convertedData->data;
                            delete pConvertedData->convertedData;
                        }
                        delete pConvertedData;
                    }
                }
                if (size < DRM_CONVERT_SIZE) {
                    break;
                }
            }
            // Close the conversion session and write the signature
            DrmConvertedStatus* pCloseData = drmManagerClient->closeConvertSession(convertID);
            // Check that we got the signature back.
            EXPECT_TRUE(NULL != pCloseData && NULL != pCloseData->convertedData &&
                    NULL != pCloseData->convertedData->data &&
                    pCloseData->convertedData->length > 0);
            if (NULL != pCloseData && NULL != pCloseData->convertedData &&
                    NULL != pCloseData->convertedData->data &&
                    pCloseData->convertedData->length > 0) {
                lseek(outFileDesc, pCloseData->offset, SEEK_SET);
                write(outFileDesc, pCloseData->convertedData->data,
                        pCloseData->convertedData->length);
            }
            if (NULL != pCloseData) {
                if (NULL != pCloseData->convertedData) {
                    delete pCloseData->convertedData->data;
                    delete pCloseData->convertedData;
                }
                delete pCloseData;
            }
        }
    }
    if (0 <= inFileDesc) {
        (void)close(inFileDesc);
    }
    if (0 <= outFileDesc) {
        (void)close(outFileDesc);
    }
}

/**
 * TC - testOriginalMimeType: tests functionality about getOriginalMimeType through framework
 */
TEST_F(DrmManagerClientFLTest, testOriginalMimeType) {
    convertFile(drmManagerClient, TESTFILE1_DM, TESTFILE1_OUTFL);
    // Check that correct original mimetype is received.
    ASSERT_EQ(String8("audio/mpeg"),
            drmManagerClient->getOriginalMimeType(String8(TESTFILE1_OUTFL)));
}

/**
 * TC - testEngineSupport: tests engine support info
 */
TEST_F(OmaFLAgentTest, testEngineSupport) {
    int mimeCount = 0;
    int suffixCount = 0;
    bool found;
    DrmSupportInfo *supportInfo = flengine->getSupportInfo(0);
    // Check that supportInfo != NULL
    ASSERT_TRUE(supportInfo != NULL);
    mimeCount = supportInfo->getMimeTypeCount();
    suffixCount = supportInfo->getFileSuffixCount();
    // Check that we have correct amount of mimetypes
    ASSERT_EQ(2, mimeCount);
    // Check that we have correct amount of file extensions
    ASSERT_EQ(2, suffixCount);

    String8 mimeStrList[] = {
        String8(DRM_FL_MIMETYPE),
        String8(DRM_DM_MIMETYPE)
    };
    // Verify that both strings are found in the received list of mimetypes.
    for (int i = 0; i < 2; i++) {
        found = false;
        DrmSupportInfo::MimeTypeIterator mimeIter = supportInfo->getMimeTypeIterator();
        do {
            String8 curStr = mimeIter.next();
            if (curStr == mimeStrList [i]) {
                found = true;
                break;
            }
            if (!mimeIter.hasNext()) {
                found = false;
            }
        } while (!found);
        // Check that mimeType is available in the list
        ASSERT_TRUE(found);
    }

    String8 suffixStrList[] = {
        String8(DRM_DM_EXTENSION),
        String8(DRM_FL_EXTENSION)
    };
    // Verify that both strings are found in the received list of file suffix.
    for (int i = 0; i < 2; i++) {
        found = false;
        DrmSupportInfo::FileSuffixIterator suffixIter = supportInfo->getFileSuffixIterator();
        do {
            String8 curStr = suffixIter.next();
            if (curStr == suffixStrList [i]) {
                found = true;
                break;
            }
            if (!suffixIter.hasNext()) {
                found = false;
            }
        } while (!found);
        // Check that file suffix is available in the list
        ASSERT_TRUE(found);
    }
}

/**
 * TC - testGetDrmObjectType: tests getDrmObjectType API
 */
TEST_F(OmaFLAgentTest, testGetDrmObjectType) {
    // Check that correct error code is received for empty strings
    ASSERT_EQ((int)DrmObjectType::UNKNOWN,
            flengine->getDrmObjectType(0, String8(""), String8("")));
    // Check that correct code is received for a dm file.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            flengine->getDrmObjectType(0, String8(TESTFILE1_DM), String8("")));
    // Check that correct error code is received for a dm file with incorrect mimetype.
    ASSERT_EQ((int)DrmObjectType::UNKNOWN,
            flengine->getDrmObjectType(0, String8(TESTFILE1_DM), String8("wrong/mime")));
    // Check that correct code is received for a dm file with correct mimetype.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            flengine->getDrmObjectType(0, String8(TESTFILE1_DM), String8(DRM_DM_MIMETYPE)));
    // Check that correct code is received for a dm file without filename with correct mimetype.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            flengine->getDrmObjectType(0, String8(""), String8(DRM_DM_MIMETYPE)));
    // Check that correct code is received for a fl file without mimetype.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            flengine->getDrmObjectType(0, String8(TESTFILE1_OUTFL), String8("")));
    // Check that correct error code is received for a fl filewith wrong mimetype.
    ASSERT_EQ((int)DrmObjectType::UNKNOWN,
            flengine->getDrmObjectType(0, String8(TESTFILE1_OUTFL), String8("wrong/mime")));
    // Check that correct code is received for a fl file with correct mimetype.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            flengine->getDrmObjectType(0, String8(TESTFILE1_OUTFL), String8(DRM_FL_MIMETYPE)));
    // Check that correct code is received for a fl file without filename with correct mimetype.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            flengine->getDrmObjectType(0, String8(""), String8(DRM_FL_MIMETYPE)));
}

/**
 * TC - testCanHandle: tests whether canHandle works
 */
TEST_F(OmaFLAgentTest, testCanHandle) {
    // Check that canHandle with empty String returns FALSE
    ASSERT_FALSE(flengine->canHandle(0, String8("")));
    // Check that canHandle with incorrect String returns FALSE
    ASSERT_FALSE(flengine->canHandle(0, String8("abcalkd")));
    // Check that canHandle with incorrect extension returns FALSE
    ASSERT_FALSE(flengine->canHandle(0, String8("temp/temp.mp3")));
    // Check that canHandle with empty extension returns FALSE
    ASSERT_FALSE(flengine->canHandle(0, String8("temp/temp.")));
    // Check that canHandle with dm file returns TRUE
    ASSERT_TRUE(flengine->canHandle(0, String8(TESTFILE1_DM)));
    // Check that canHandle with dm file returns TRUE
    ASSERT_TRUE(flengine->canHandle(0, String8(TESTFILE1_OUTFL)));
}

/**
 * TC - testOriginalMimeType: tests functionality about getOriginalMimeType
 */
TEST_F(OmaFLAgentTest, testOriginalMimeType) {
    // Check that correct original mimetype is received for empty filename.
    ASSERT_EQ(String8(""), flengine->getOriginalMimeType(0, String8("")));
    // Check that correct original mimetype is received for incorrect filename.
    ASSERT_EQ(String8(""), flengine->getOriginalMimeType(0, String8("wrongfilename")));
}

/**
 * TC - testValidateAction: tests getValidateAction with different actions
 */
TEST_F(OmaFLAgentTest, testValidateAction) {
    ActionDescription description(0, 0);
    // Validate that validateAction returns TRUE for DEFAULT action on fl file.
    ASSERT_TRUE(flengine->validateAction(0, String8(TESTFILE1_OUTFL),
            Action::DEFAULT, description));
    // Validate that validateAction returns FALSE for TRANSFER action on fl file.
    ASSERT_FALSE(flengine->validateAction(0, String8(TESTFILE1_OUTFL),
            Action::TRANSFER, description));
    // Validate that validateAction returns FALSE for TRANSFER action on none drm file.
    ASSERT_FALSE(flengine->validateAction(0, String8("NonDrm.mp3"),
            Action::TRANSFER, description));
    // Validate that validateAction returns FALSE for PLAY action on none drm file.
    ASSERT_FALSE(flengine->validateAction(0, String8("NonDrm.mp3"),
            Action::PLAY, description));
}

/**
 * TC - testCheckRightsStatus: tests checkRightsStatus with different actions
 */
TEST_F(OmaFLAgentTest, testCheckRightsStatus) {
    // Validate that checkRightsStatus returns RIGHTS_VALID for DEFAULT action on fl file.
    ASSERT_EQ((int)RightsStatus::RIGHTS_VALID,
            flengine->checkRightsStatus(0, String8(TESTFILE1_OUTFL), Action::DEFAULT));
    // Validate that checkRightsStatus returns RIGHTS_INVALID for TRANSFER action on fl file.
    ASSERT_EQ((int)RightsStatus::RIGHTS_INVALID,
            flengine->checkRightsStatus(0, String8(TESTFILE1_OUTFL), Action::TRANSFER));
    // Validate that checkRightsStatus returns RIGHTS_INVALID for TRANSFER action on none fl file.
    ASSERT_EQ((int)RightsStatus::RIGHTS_INVALID,
            flengine->checkRightsStatus(0, String8("NonDrm.mp3"), Action::TRANSFER));
    // Validate that checkRightsStatus returns RIGHTS_INVALID for PLAY action on none fl file.
    ASSERT_EQ((int)RightsStatus::RIGHTS_INVALID,
            flengine->checkRightsStatus(0, String8("NonDrm.mp3"), Action::TRANSFER));
}

/**
 * TC - testGetConstraints: tests getConstraints. Its not used for Forward Lock Engine.
 */
TEST_F(OmaFLAgentTest, testGetConstraints) {
    DrmConstraints* drmConstraints = NULL;
    drmConstraints = flengine->getConstraints(0, NULL, 0);
    // Validate that getConstraints returns NULL for failure.
    ASSERT_TRUE(NULL == drmConstraints);
    String8 pathFL(TESTFILE1_OUTFL);
    drmConstraints = flengine->getConstraints(0, &pathFL, Action::PLAY);
    // Validate that getConstraints returns a object for valid drm file.
    ASSERT_TRUE(NULL != drmConstraints);
    delete drmConstraints;
}

};
