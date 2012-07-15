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

package android.drm.test.flapp.runners;

import junit.framework.TestSuite;
import android.drm.test.flapp.tests.FLUseCaseBasedTests;
import android.test.suitebuilder.TestMethod;
import android.test.InstrumentationTestRunner;
import android.test.InstrumentationTestSuite;

/**
 * Runner for the Forward Lock Vendor Reception Control tests.
 */
public class FLVRCRunner extends InstrumentationTestRunner {
    @Override
    public TestSuite getAllTests()  {
        TestSuite suite = new InstrumentationTestSuite(this);
        TestMethod[] testMethods = new TestMethod[] {
                new TestMethod("testAI_FL_MIME_type_audio_mp3", FLUseCaseBasedTests.class),
                new TestMethod("testAI_FL_MIME_type_audio_3gp", FLUseCaseBasedTests.class),
                new TestMethod("testAI_FL_MIME_type_audio_wav", FLUseCaseBasedTests.class),
                new TestMethod("testAI_FL_MIME_type_audio_m4a", FLUseCaseBasedTests.class),
                new TestMethod("testAI_FL_MIME_type_audio_mp4", FLUseCaseBasedTests.class),
                new TestMethod("testAI_FL_MIME_type_video_3gp", FLUseCaseBasedTests.class),
                new TestMethod("testAI_FL_MIME_type_video_mp4", FLUseCaseBasedTests.class)
        };
        try {
            for (int i = 0; i < testMethods.length; ++i) {
                suite.addTest(testMethods[i].createTest());
            }
        } catch (Exception e) {
            throw new RuntimeException("Could not create the test suite. Message: "
                    + e.getMessage());
        }
        return suite;
    }

    @Override
    public ClassLoader getLoader() {
        return FLVRCRunner.class.getClassLoader();
    }
}
