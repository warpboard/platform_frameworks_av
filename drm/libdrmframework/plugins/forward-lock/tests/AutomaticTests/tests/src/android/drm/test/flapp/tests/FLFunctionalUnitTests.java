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

package android.drm.test.flapp.tests;

import android.content.ContentValues;
import android.drm.DrmInfo;
import android.drm.DrmInfoRequest;
import android.drm.DrmManagerClient;
import android.drm.DrmRights;
import android.drm.DrmStore;
import android.test.SingleLaunchActivityTestCase;
import android.drm.test.flapp.FLTestActivity;
import static android.drm.test.flutilities.DrmFLUtilityFunctions.*;
import android.drm.test.flapp.R;
import java.util.HashMap;

/**
 * Tests that use the public API of the {@link DrmManagerClient} class to test the
 * corresponding functions in the forward lock engine.
 */
public class FLFunctionalUnitTests extends SingleLaunchActivityTestCase<FLTestActivity>  {
    private DrmManagerClient mManager;
    private FLTestActivity mTestActivity;
    // mappings between resource ids and the filenames they are supposed to have.
    private final HashMap<Integer, String> mResourceFileNameMappings =
            new HashMap<Integer, String>();
    private static final String mWhereToStoreTestFiles = "/sdcard/DRMflTests/";

    public FLFunctionalUnitTests() {
        super(FLTestActivity.class.getPackage().getName(), FLTestActivity.class);
        mResourceFileNameMappings.put(Integer.valueOf(R.raw.audio_mp3), "audio_mp3.fl");
        mResourceFileNameMappings.put(Integer.valueOf(R.raw.sample_rights), "sample_rights.dr");
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTestActivity = getActivity();
        mManager = new DrmManagerClient(mTestActivity);
        // Check that the FL plugin was loaded
        if (!isPluginLoaded("OMA V1 Forward Lock", mManager)) {
            throw new Exception("The forward lock plugin was not loaded.");
        }
    }

    @Override
    protected void tearDown() throws Exception {
        deleteFolder(mWhereToStoreTestFiles);
        super.tearDown();
    }

    public void testConversion() throws Exception {
        putOnPhoneAndConvert(R.raw.audio_mp3,
                mResourceFileNameMappings.get(R.raw.audio_mp3),
                mWhereToStoreTestFiles,
                mTestActivity, mManager);
    }

    public void testAcquireDrmInfo() throws Exception {
        /*acquireDrmInfo is used for rights specific operations
        that are not relevant for forward lock files but is used hereto see that no crash occurs.*/
        DrmInfoRequest testRequest = new DrmInfoRequest(1, FL_MIMETYPE_DM);
        DrmInfoRequest testRequest2 = new DrmInfoRequest(2, FL_MIMETYPE_FL);
        assertEquals(1, (mManager.acquireDrmInfo(testRequest)).getInfoType());
        assertEquals(2, (mManager.acquireDrmInfo(testRequest2)).getInfoType());
    }

    public void testCanHandle() throws Exception {
        String pathToFLfile = putOnPhoneAndConvert(R.raw.audio_mp3,
                mResourceFileNameMappings.get(R.raw.audio_mp3), mWhereToStoreTestFiles,
                mTestActivity, mManager);
        assertTrue(mManager.canHandle(pathToFLfile, ""));
    }

    public void testCheckRightsStatus() throws Exception {
        String pathToFLfile = putOnPhoneAndConvert(R.raw.audio_mp3,
                mResourceFileNameMappings.get(R.raw.audio_mp3), mWhereToStoreTestFiles,
                mTestActivity, mManager);
        // Forward lock has constraint-free rights.
        assertEquals(DrmStore.RightsStatus.RIGHTS_VALID,
                mManager.checkRightsStatus(pathToFLfile));
    }

    public void testGetConstraints() throws Exception  {
        String pathToFLfile = putOnPhoneAndConvert(R.raw.audio_mp3,
                mResourceFileNameMappings.get(R.raw.audio_mp3), mWhereToStoreTestFiles,
                mTestActivity, mManager);
        // Try to get constraints from permission play
        ContentValues localContent = mManager.getConstraints(pathToFLfile, 1);
        assertEquals("Empty constraint structure should been returned for the file since " +
                "it is forward lock", 0, localContent.size());
    }

    public void testGetDrmObjectType() throws Exception {
        String pathToFLfile = putOnPhoneAndConvert(R.raw.audio_mp3,
                mResourceFileNameMappings.get(R.raw.audio_mp3), mWhereToStoreTestFiles,
                mTestActivity, mManager);
        assertEquals(DrmStore.DrmObjectType.CONTENT, mManager.getDrmObjectType(pathToFLfile,
                FL_MIMETYPE_FL));
    }

    public void testGetOriginalMimeType() throws Exception {
        String pathToFLfile = putOnPhoneAndConvert(R.raw.audio_mp3,
                mResourceFileNameMappings.get(R.raw.audio_mp3), mWhereToStoreTestFiles,
                mTestActivity, mManager);
        assertEquals(AUDIO_MIMETYPE_MPEG, mManager.getOriginalMimeType(pathToFLfile));
    }

    public void testProcessDrmInfo() throws Exception {
        /*processDrmInfo is not used for forward lock but we try to use it anyways so that we
         * assure that no error condition occurs.*/
        byte[] testBytes = new byte[] {7, 6, 5, 4, 3, 2, 1, 0};
        String mime = FL_MIMETYPE_FL;
        String mime2 = FL_MIMETYPE_DM;
        DrmInfo testInfo = new DrmInfo(1, testBytes, mime);
        DrmInfo testInfo2 = new DrmInfo(1, testBytes, mime2);
        assertEquals(DrmManagerClient.ERROR_NONE, mManager.processDrmInfo(testInfo));
        assertEquals(DrmManagerClient.ERROR_NONE, mManager.processDrmInfo(testInfo2));
    }

    public void testRemoveRights() throws Exception {
        // FL file does not have constraints but try anyways
        String pathToFLfile = putOnPhoneAndConvert(R.raw.audio_mp3,
                mResourceFileNameMappings.get(R.raw.audio_mp3), mWhereToStoreTestFiles,
                mTestActivity, mManager);
        // The line below should do nothing because there are no rights handling for FL.
        mManager.removeRights(pathToFLfile);
    }

    public void testSaveRights() throws Exception {
        // FL file does not have constraints but try to save rights anyways
        String contentRightsPath = putOnPhone(R.raw.sample_rights,
                mResourceFileNameMappings.get(R.raw.sample_rights), mWhereToStoreTestFiles,
                mTestActivity);
        DrmRights testRights = new DrmRights(contentRightsPath, FL_MIMETYPE_FL);
        DrmRights testRights2 = new DrmRights(contentRightsPath, FL_MIMETYPE_DM);
        // The lines below should do nothing because there are no rights handling for FL.
        mManager.saveRights(testRights, "", "dummy");
        mManager.saveRights(testRights2, "", "dummy");
    }
}



