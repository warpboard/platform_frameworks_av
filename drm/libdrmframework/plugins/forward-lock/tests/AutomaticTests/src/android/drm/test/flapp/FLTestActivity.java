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

package android.drm.test.flapp;

import android.app.Activity;
import android.drm.test.flutilities.DownloadCompleteDynamicReceiver;
import android.os.Bundle;
import android.content.IntentFilter;
import static android.drm.test.flutilities.DrmFLUtilityFunctions.*;

/**
 * Small test activity used when running the Forward Lock tests.
 */
public class FLTestActivity extends Activity {
    private DownloadCompleteDynamicReceiver mDownloadComplete =
        new DownloadCompleteDynamicReceiver();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onStart() {
        super.onStart();
        registerDownloadReceiver();
    }

    @Override
    public void onStop() {
        unregisterReceiver(mDownloadComplete);
        super.onStop();
    }

    /**
     * Gets the {@link DownloadCompleteDynamicReceiver} instance associated with
     * this {@link FLTestActivity} instance.
     *
     * @return a {@link DownloadCompleteDynamicReceiver} instance.
     */
    public DownloadCompleteDynamicReceiver getDownloadCompleteReceiver() {
        return mDownloadComplete;
    }

    private void registerDownloadReceiver() {
        IntentFilter filter = new IntentFilter(DRM_ACTION_DOWNLOAD_COMPLETE);
        registerReceiver(mDownloadComplete, filter);
    }
}

