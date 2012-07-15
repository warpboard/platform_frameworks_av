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
import android.drm.test.flapp.tests.FLFunctionalUnitTests;
import android.test.InstrumentationTestRunner;
import android.test.InstrumentationTestSuite;

/**
 * Runner for the Forward Lock functional unit tests.
 */
public class FLUnitRunner extends InstrumentationTestRunner {
    @Override
    public TestSuite getAllTests()  {
        TestSuite suite = new InstrumentationTestSuite(this);
        suite.addTestSuite(FLFunctionalUnitTests.class);
        return suite;
    }

    @Override
    public ClassLoader getLoader() {
        return FLUnitRunner.class.getClassLoader();
    }
}
