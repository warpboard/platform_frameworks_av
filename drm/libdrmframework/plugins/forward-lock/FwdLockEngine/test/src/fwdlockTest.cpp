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

#define FWDLOCK_TESTFILE_DM    "/data/data/flenginetest/musictest.dm"
#define FWDLOCK_TESTFILE_FL    "/data/data/flenginetest/musictest.fl"
#define FWDLOCK_DM_MIMETYPE    "application/vnd.oma.drm.message"
#define FWDLOCK_FL_MIMETYPE    "application/x-android-drm-fl"
#define FWDLOCK_NONDRM         "NonDrm.mp3"
#define FWDLOCK_DM_EXTENSION   ".dm"
#define FWDLOCK_FL_EXTENSION   ".fl"
#define FWDLOCK_CONVERT_SIZE   4096
#define FWDLOCK_TEMP_UNIQUEID  99999

// Test class for test cases that access the FL agent directly
class OmaFLAgentTest : public ::testing::Test {

protected:
    virtual void SetUp() {
        mForwardLockEngine = static_cast<IDrmEngine *> (new FwdLockEngine());
        mForwardLockEngine->initialize(FWDLOCK_TEMP_UNIQUEID);
    }

    virtual void TearDown() {
        mForwardLockEngine->terminate(FWDLOCK_TEMP_UNIQUEID);
        delete mForwardLockEngine;
        mForwardLockEngine = NULL;
    }

    IDrmEngine *mForwardLockEngine;
};

// Test class for test cases that access the FL agent through DRM framework
class DrmManagerClientFLTest : public ::testing::Test {

protected:
    virtual void SetUp() {
        mDrmManagerClient = new DrmManagerClient();
    }

    virtual void TearDown() {
        delete mDrmManagerClient;
        mDrmManagerClient = NULL;
    }

    DrmManagerClient* mDrmManagerClient;
};

/**
 * Helper function that converts the .dm file to the internal forward lock format.
 */
static void convertFile(DrmManagerClient* mDrmManagerClient,
                        const char* drmFile,
                        const char* textFile) {
    int outFileDesc = -1;
    int inFileDesc = open(drmFile, O_RDONLY);
    ASSERT_TRUE(-1 < inFileDesc);
    outFileDesc = open(textFile, O_CREAT | O_TRUNC | O_WRONLY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    ASSERT_TRUE(-1 < outFileDesc);
    ssize_t size = FWDLOCK_CONVERT_SIZE;
    char buff[FWDLOCK_CONVERT_SIZE];
    int convertID = mDrmManagerClient->openConvertSession(String8(FWDLOCK_DM_MIMETYPE));
    ASSERT_TRUE(0 <= convertID);
    DrmBuffer* drmBuff = new DrmBuffer();
    drmBuff->data = buff;
    while (0 <= convertID && 0 < size) {
        size = read(inFileDesc, buff, size);
        // Check that we could read the file
        ASSERT_TRUE((size > 0) && (size <= FWDLOCK_CONVERT_SIZE));
        drmBuff->length = size;
        DrmConvertedStatus* convertedData = NULL;
        convertedData = mDrmManagerClient->convertData(convertID, drmBuff);
        // Check that converted data went fine.
        ASSERT_TRUE(NULL != convertedData);
        ASSERT_TRUE(DrmConvertedStatus::STATUS_OK == convertedData->statusCode);
        // if converted data is returned, write the the output file
        if (NULL != convertedData->convertedData) {
            if (NULL != convertedData->convertedData->data &&
                    convertedData->convertedData->length > 0) {
                write(outFileDesc, convertedData->convertedData->data,
                        convertedData->convertedData->length);
            }
            delete convertedData->convertedData->data;
            delete convertedData->convertedData;
        }
        delete convertedData;
        if (size < FWDLOCK_CONVERT_SIZE) {
            break;
        }
    }
    delete drmBuff;
    // Close the conversion session and write the signature
    DrmConvertedStatus* closeData = mDrmManagerClient->closeConvertSession(convertID);
    // Check that we got the signature back.
    ASSERT_TRUE(NULL != closeData && NULL != closeData->convertedData &&
            NULL != closeData->convertedData->data && closeData->convertedData->length > 0);

    lseek(outFileDesc, closeData->offset, SEEK_SET);
    write(outFileDesc, closeData->convertedData->data,
            closeData->convertedData->length);
    delete closeData->convertedData->data;
    delete closeData->convertedData;
    delete closeData;

    (void)close(inFileDesc);
    (void)close(outFileDesc);
}

/**
 * TC - testOriginalMimeType: tests functionality about getOriginalMimeType through framework
 */
TEST_F(DrmManagerClientFLTest, testOriginalMimeType) {
    convertFile(mDrmManagerClient, FWDLOCK_TESTFILE_DM, FWDLOCK_TESTFILE_FL);
    // Check that correct original mimetype is received.
    ASSERT_EQ(String8("audio/mpeg"),
            mDrmManagerClient->getOriginalMimeType(String8(FWDLOCK_TESTFILE_FL)));
}

/**
 * TC - testEngineSupport: tests engine support info
 */
TEST_F(OmaFLAgentTest, testEngineSupport) {
    int mimeCount = 0;
    int suffixCount = 0;
    bool found;
    DrmSupportInfo *supportInfo = mForwardLockEngine->getSupportInfo(FWDLOCK_TEMP_UNIQUEID);
    // Check that supportInfo != NULL
    ASSERT_TRUE(supportInfo != NULL);
    mimeCount = supportInfo->getMimeTypeCount();
    suffixCount = supportInfo->getFileSuffixCount();
    // Check that we have correct number of mimetypes
    ASSERT_EQ(2, mimeCount);
    // Check that we have correct number of file extensions
    ASSERT_EQ(2, suffixCount);

    String8 mimeStrList[] = {
        String8(FWDLOCK_FL_MIMETYPE),
        String8(FWDLOCK_DM_MIMETYPE)
    };
    // Verify that both strings are found in the received list of mimetypes.
    for (int i = 0; i < 2; i++) {
        found = false;
        DrmSupportInfo::MimeTypeIterator mimeIter = supportInfo->getMimeTypeIterator();
        while (mimeIter.hasNext()) {
            String8 curStr = mimeIter.next();
            if (curStr == mimeStrList [i]) {
                found = true;
                break;
            }
        }
        // Check that mimeType is available in the list
        ASSERT_TRUE(found);
    }

    String8 suffixStrList[] = {
        String8(FWDLOCK_DM_EXTENSION),
        String8(FWDLOCK_FL_EXTENSION)
    };
    // Verify that both strings are found in the received list of file suffix.
    for (int i = 0; i < 2; i++) {
        found = false;
        DrmSupportInfo::FileSuffixIterator suffixIter = supportInfo->getFileSuffixIterator();
        while (suffixIter.hasNext()) {
            String8 curStr = suffixIter.next();
            if (curStr == suffixStrList [i]) {
                found = true;
                break;
            }
        }
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
            mForwardLockEngine->getDrmObjectType(FWDLOCK_TEMP_UNIQUEID, String8(""), String8("")));
    // Check that correct code is received for a dm file.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            mForwardLockEngine->getDrmObjectType(FWDLOCK_TEMP_UNIQUEID,
                    String8(FWDLOCK_TESTFILE_DM), String8("")));
    // Check that correct error code is received for a dm file with incorrect mimetype.
    ASSERT_EQ((int)DrmObjectType::UNKNOWN,
            mForwardLockEngine->getDrmObjectType(FWDLOCK_TEMP_UNIQUEID,
                    String8(FWDLOCK_TESTFILE_DM), String8("wrong/mime")));
    // Check that correct code is received for a dm file with correct mimetype.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            mForwardLockEngine->getDrmObjectType(FWDLOCK_TEMP_UNIQUEID,
                    String8(FWDLOCK_TESTFILE_DM), String8(FWDLOCK_DM_MIMETYPE)));
    // Check that correct code is received for a dm file without filename with correct mimetype.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            mForwardLockEngine->getDrmObjectType(FWDLOCK_TEMP_UNIQUEID, String8(""),
                    String8(FWDLOCK_DM_MIMETYPE)));
    // Check that correct code is received for a fl file without mimetype.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            mForwardLockEngine->getDrmObjectType(FWDLOCK_TEMP_UNIQUEID,
                    String8(FWDLOCK_TESTFILE_FL), String8("")));
    // Check that correct error code is received for a fl filewith wrong mimetype.
    ASSERT_EQ((int)DrmObjectType::UNKNOWN,
            mForwardLockEngine->getDrmObjectType(FWDLOCK_TEMP_UNIQUEID,
                    String8(FWDLOCK_TESTFILE_FL), String8("wrong/mime")));
    // Check that correct code is received for a fl file with correct mimetype.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            mForwardLockEngine->getDrmObjectType(FWDLOCK_TEMP_UNIQUEID,
                    String8(FWDLOCK_TESTFILE_FL), String8(FWDLOCK_FL_MIMETYPE)));
    // Check that correct code is received for a fl file without filename with correct mimetype.
    ASSERT_EQ((int)DrmObjectType::CONTENT,
            mForwardLockEngine->getDrmObjectType(FWDLOCK_TEMP_UNIQUEID, String8(""),
                    String8(FWDLOCK_FL_MIMETYPE)));
}

/**
 * TC - testCanHandle: tests whether canHandle works
 */
TEST_F(OmaFLAgentTest, testCanHandle) {
    // Check that canHandle with empty String returns FALSE
    ASSERT_FALSE(mForwardLockEngine->canHandle(FWDLOCK_TEMP_UNIQUEID, String8("")));
    // Check that canHandle with incorrect String returns FALSE
    ASSERT_FALSE(mForwardLockEngine->canHandle(FWDLOCK_TEMP_UNIQUEID, String8("abcalkd")));
    // Check that canHandle with incorrect extension returns FALSE
    ASSERT_FALSE(mForwardLockEngine->canHandle(FWDLOCK_TEMP_UNIQUEID, String8("temp/temp.mp3")));
    // Check that canHandle with empty extension returns FALSE
    ASSERT_FALSE(mForwardLockEngine->canHandle(FWDLOCK_TEMP_UNIQUEID, String8("temp/temp.")));
    // Check that canHandle with dm file returns TRUE
    ASSERT_TRUE(mForwardLockEngine->canHandle(FWDLOCK_TEMP_UNIQUEID, String8(FWDLOCK_TESTFILE_DM)));
    // Check that canHandle with dm file returns TRUE
    ASSERT_TRUE(mForwardLockEngine->canHandle(FWDLOCK_TEMP_UNIQUEID, String8(FWDLOCK_TESTFILE_FL)));
}

/**
 * TC - testOriginalMimeType: tests functionality about getOriginalMimeType
 */
TEST_F(OmaFLAgentTest, testOriginalMimeType) {
    // Check that correct original mimetype is received for empty filename.
    ASSERT_EQ(String8(""), mForwardLockEngine->getOriginalMimeType(FWDLOCK_TEMP_UNIQUEID,
            String8("")));
    // Check that correct original mimetype is received for incorrect filename.
    ASSERT_EQ(String8(""), mForwardLockEngine->getOriginalMimeType(FWDLOCK_TEMP_UNIQUEID,
            String8("wrongfilename")));
}

/**
 * TC - testValidateAction: tests getValidateAction with different actions
 */
TEST_F(OmaFLAgentTest, testValidateAction) {
    ActionDescription description(FWDLOCK_TEMP_UNIQUEID, 0);
    // Validate that validateAction returns TRUE for DEFAULT action on fl file.
    ASSERT_TRUE(mForwardLockEngine->validateAction(FWDLOCK_TEMP_UNIQUEID,
            String8(FWDLOCK_TESTFILE_FL), Action::DEFAULT, description));
    // Validate that validateAction returns FALSE for TRANSFER action on fl file.
    ASSERT_FALSE(mForwardLockEngine->validateAction(FWDLOCK_TEMP_UNIQUEID,
            String8(FWDLOCK_TESTFILE_FL), Action::TRANSFER, description));
    // Validate that validateAction returns FALSE for TRANSFER action on none drm file.
    ASSERT_FALSE(mForwardLockEngine->validateAction(FWDLOCK_TEMP_UNIQUEID, String8(FWDLOCK_NONDRM),
            Action::TRANSFER, description));
    // Validate that validateAction returns FALSE for PLAY action on none drm file.
    ASSERT_FALSE(mForwardLockEngine->validateAction(FWDLOCK_TEMP_UNIQUEID, String8(FWDLOCK_NONDRM),
            Action::PLAY, description));
}

/**
 * TC - testCheckRightsStatus: tests checkRightsStatus with different actions
 */
TEST_F(OmaFLAgentTest, testCheckRightsStatus) {
    // Validate that checkRightsStatus returns RIGHTS_VALID for DEFAULT action on fl file.
    ASSERT_EQ((int)RightsStatus::RIGHTS_VALID,
            mForwardLockEngine->checkRightsStatus(FWDLOCK_TEMP_UNIQUEID,
                    String8(FWDLOCK_TESTFILE_FL), Action::DEFAULT));
    // Validate that checkRightsStatus returns RIGHTS_INVALID for TRANSFER action on fl file.
    ASSERT_EQ((int)RightsStatus::RIGHTS_INVALID,
            mForwardLockEngine->checkRightsStatus(FWDLOCK_TEMP_UNIQUEID,
                    String8(FWDLOCK_TESTFILE_FL), Action::TRANSFER));
    // Validate that checkRightsStatus returns RIGHTS_INVALID for TRANSFER action on none fl file.
    ASSERT_EQ((int)RightsStatus::RIGHTS_INVALID,
            mForwardLockEngine->checkRightsStatus(FWDLOCK_TEMP_UNIQUEID, String8(FWDLOCK_NONDRM),
                    Action::TRANSFER));
    // Validate that checkRightsStatus returns RIGHTS_INVALID for PLAY action on none fl file.
    ASSERT_EQ((int)RightsStatus::RIGHTS_INVALID,
            mForwardLockEngine->checkRightsStatus(FWDLOCK_TEMP_UNIQUEID, String8(FWDLOCK_NONDRM),
                    Action::TRANSFER));
}

/**
 * TC - testGetConstraints: tests getConstraints. Its not used for Forward Lock Engine.
 */
TEST_F(OmaFLAgentTest, testGetConstraints) {
    DrmConstraints* drmConstraints = NULL;
    drmConstraints = mForwardLockEngine->getConstraints(FWDLOCK_TEMP_UNIQUEID, NULL, 0);
    // Validate that getConstraints returns NULL for failure.
    ASSERT_TRUE(NULL == drmConstraints);
    String8 pathFL(FWDLOCK_TESTFILE_FL);
    drmConstraints = mForwardLockEngine->getConstraints(FWDLOCK_TEMP_UNIQUEID, &pathFL,
            Action::PLAY);
    // Validate that getConstraints returns a object for valid drm file.
    ASSERT_TRUE(NULL != drmConstraints);
    delete drmConstraints;
}

};
