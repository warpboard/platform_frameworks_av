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
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;
import static android.drm.test.flutilities.DrmFLUtilityFunctions.*;

/**
 * Receives download complete intents directed explicitly from the download provider to this class
 * and passes them on as system wide broadcasts meant to be received by a
 * {@link DonwloadCompleteDynamicReceiver} instance.
 */
public class DownloadCompleteStaticReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        Uri receivedUri = intent.getData();
        String[] projection = new String[] {
                "_data"
        };
        Cursor cursor = context.getContentResolver().query(receivedUri, projection, null,
                                                           null, null);
        if (null == cursor || cursor.getCount() == 0 || !cursor.moveToFirst()) {
            Log.w(DRM_TEST_TAG, "Error in the download provider, it says " +
                  "that the download is complete but the recei" +
                  "ved Uri can not be found " +
                  "in the download database.");
            return;
        }
        int data_column_index = 0;
        try {
            data_column_index = cursor.getColumnIndexOrThrow("_data");
            String filename = cursor.getString(data_column_index);
            Intent broadcastIntent = new Intent(DRM_ACTION_DOWNLOAD_COMPLETE);
            broadcastIntent.putExtra("downloadedFile", filename);
            context.sendBroadcast(broadcastIntent);
        } catch (Exception e) {
            Log.w(DRM_TEST_TAG, "Could not get the column index "
                    + "of the file name for the downloaded file.");
        } finally {
            cursor.close();
        }
    }
}
