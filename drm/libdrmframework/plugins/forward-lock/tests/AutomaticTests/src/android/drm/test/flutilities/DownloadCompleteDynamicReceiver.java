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

package android.drm.test.flutilities;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import java.util.concurrent.CountDownLatch;

/**
 * Receives download complete intents from a {@link DonwloadCompleteStaticReceiver} instance.
 */
public class DownloadCompleteDynamicReceiver extends BroadcastReceiver {
    private String downloadedFilePath = "";
    private CountDownLatch downloadLatch = new CountDownLatch(1);

    @Override
    public void onReceive(Context context, Intent intent) {
        downloadedFilePath = intent.getStringExtra("downloadedFile");
        if (downloadedFilePath != null) {
            if ((downloadedFilePath.startsWith("/mnt")) &&
               (downloadedFilePath.length() > "/mnt".length())) {
                downloadedFilePath = downloadedFilePath.substring(4, downloadedFilePath.length());
            }
        }
        downloadLatch.countDown();
    }

    public String getDownloadedFileName() {
        return downloadedFilePath;
    }

    public CountDownLatch getLatch() {
        return downloadLatch;
    }

    public void resetLatch() {
       downloadedFilePath = "";
       downloadLatch = new CountDownLatch(1);
    }
}
