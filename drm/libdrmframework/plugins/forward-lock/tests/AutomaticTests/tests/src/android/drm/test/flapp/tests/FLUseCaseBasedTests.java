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

import android.drm.DrmManagerClient;
import android.test.SingleLaunchActivityTestCase;
import static android.test.MoreAsserts.*;
import java.util.Vector;
import android.drm.test.flapp.FLTestActivity;
import static android.drm.test.flutilities.DrmFLUtilityFunctions.*;
import static android.drm.test.content.FLContentUrls.*;

/**
 * Use case based Forward Lock tests. The normal use case involves
 * downloading and playing audio or video content.
 */
public class FLUseCaseBasedTests extends SingleLaunchActivityTestCase<FLTestActivity> {
    private DrmManagerClient mManager;
    private FLTestActivity mTestActivity;
    private Vector<String> mFilesToRemove = new Vector<String>();

    public FLUseCaseBasedTests() {
        super(FLTestActivity.class.getPackage().getName(), FLTestActivity.class);
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
        if (mFilesToRemove != null) {
            for (int i = 0; i < mFilesToRemove.size(); ++i) {
                deleteFile(mFilesToRemove.get(i));
            }
            mFilesToRemove.clear();
        }
        super.tearDown();
    }

    public void testAI_FL_MIME_type_audio_mp3() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(FL_MP3,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 3000));
    }

    public void testAI_FL_MIME_type_audio_3gp() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(FL_3GP_AUDIO,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 300));
    }

    public void testAI_FL_MIME_type_audio_wav() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(FL_WAV,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 800));
    }

    public void testAI_FL_MIME_type_audio_m4a() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(FL_M4A,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 800));
    }

    public void testAI_FL_MIME_type_audio_mp4() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(FL_MP4_AUDIO,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 800));
    }

    public void testAI_FL_MIME_type_video_3gp() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(FL_3GP_VIDEO,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 3000));
    }

    public void testAI_FL_MIME_type_video_mp4() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(FL_MP4_VIDEO,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 3000));
    }

    public void testAI_FL_FL_Support() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(FL_Support,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 3000));
    }

    public void testFL_DOFF_FL_in_DCF() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(FL_in_DCF,
                FL_MIMETYPE_DM, mTestActivity);
        // Check that file did not download
        assertEquals("The file did download successfully", "", filePath);
    }

    public void testFL_RF_FL_Audio() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(MP3_AUDIO_BINARY,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 3000));
    }

    public void testFL_RF_FL_Video() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(GP_VIDEO_BINARY,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 3000));
    }

    public void testGE_FDMF_One_bodypart() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(ONE_BODY,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 3000));
    }

    public void testGE_FDMF_No_bodypart() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(NO_BODY,
                FL_MIMETYPE_DM, mTestActivity);
        // Check that file did not download
        assertEquals("The file did download successfully", "", filePath);
    }

    public void testGE_FDMF_End_Boundary_Delimiter() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(NO_END_DELIMITER,
                FL_MIMETYPE_DM, mTestActivity);
        // Check that file did not download
        assertEquals("The file did download successfully", "", filePath);
    }

    public void testGE_FDMF_Start_Boundary_Delimiter() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(NO_START_DELIMITER,
                FL_MIMETYPE_DM, mTestActivity);
        // Check that file did not download
        assertEquals("The file did download successfully", "", filePath);
    }

    public void testGE_Binary_encoded_media() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(BINARY_MEDIA,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 1900));
    }

    public void testGE_7bit_encoded_media() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(SEVEN_BIT_MEDIA,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 1500));
    }

    public void testGE_8bit_encoded_media() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(EIGHT_BIT_MEDIA,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 1900));
    }

    public void testGE_base64_encoded_media() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(BASE64_MEDIA,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 1900));
    }

    public void testMISC_FNL_File_Name_Length() throws Exception {
        // Download forward lock file
        String filePath = downloadContent(FILE_NAME_LENGHT,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The file did not download successfully", filePath, "");
        mFilesToRemove.add(filePath);
        // Play file
        assertTrue(playAndVerifyPlaying(filePath, 2500));
    }

    public void testPerf_BI_AU_FL_001() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_AUDIO_001,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_AUDIO_001_unprot,
                AUDIO_MIMETYPE_MPEG , mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 1900);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_AU_FL_002() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_AUDIO_002,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_AUDIO_002_unprot, AUDIO_MIMETYPE_MPEG,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 1900);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_AU_FL_003() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_AUDIO_003,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_AUDIO_003_unprot, AUDIO_MIMETYPE_MPEG,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 1900);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_AU_FL_004() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_AUDIO_004,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_AUDIO_004_unprot, AUDIO_MIMETYPE_MPEG,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 1900);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_AU_FL_005() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_AUDIO_005,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_AUDIO_005_unprot, AUDIO_MIMETYPE_WAV,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 3000);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_AU_FL_007() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_AUDIO_007,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_AUDIO_007_unprot, AUDIO_MIMETYPE_M4A ,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 3000);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_AU_FL_008() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_AUDIO_008,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_AUDIO_008_unprot, AUDIO_MIMETYPE_M4A,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 3000);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_AU_FL_009() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_AUDIO_009,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_AUDIO_009_unprot, AUDIO_MIMETYPE_M4A,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 3000);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_VI_FL_001() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_VIDEO_001,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_VIDEO_001_unprot, VIDEO_MIMETYPE_3GP,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 2000);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_VI_FL_002() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_VIDEO_002,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_VIDEO_002_unprot, VIDEO_MIMETYPE_MP4,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 2000);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_VI_FL_003() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_VIDEO_003,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_VIDEO_003_unprot, VIDEO_MIMETYPE_3GP,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 2000);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_VI_FL_004() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_VIDEO_004,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_VIDEO_004_unprot, VIDEO_MIMETYPE_MP4,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 3000);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_VI_FL_005() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_VIDEO_005,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_VIDEO_005_unprot, VIDEO_MIMETYPE_3GP,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 3000);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }

    public void testPerf_BI_VI_FL_006() throws Exception {
        // Download protected file
        String protectedFile = downloadContent(PERF_BIT_VIDEO_006,
                FL_MIMETYPE_DM, mTestActivity);
        assertNotEqual("The protected file did not download successfully", protectedFile, "");
        mFilesToRemove.add(protectedFile);
        // Download unprotected file
        String unprotectedFile = downloadContent(PERF_BIT_VIDEO_006_unprot, VIDEO_MIMETYPE_3GP,
                mTestActivity);
        assertNotEqual("The unprotected file did not download successfully", unprotectedFile, "");
        mFilesToRemove.add(unprotectedFile);
        // Play files
        long diff = diffStartPlayProtAndUnprotFile(protectedFile,
                unprotectedFile, 3000);
        if (diff > 1) {
            fail("Time difference is greater than 1 second between protected and "
                    + "unprotected files");
        }
    }
}
