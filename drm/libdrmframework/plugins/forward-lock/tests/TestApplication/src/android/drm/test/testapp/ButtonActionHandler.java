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

package android.drm.test.testapp;

import android.app.AlertDialog;
import android.view.View;
import android.view.View.OnClickListener;

/**
 * Class handling button press events in the main activity.
 */
public class ButtonActionHandler implements OnClickListener {
private AlertDialog.Builder mAlerterDialog;
private DrmTestAppActivity mDrmTestAppActivity;

    public ButtonActionHandler(AlertDialog.Builder alerterDialog,
                               DrmTestAppActivity drmTestAppActivity) {
        mDrmTestAppActivity = drmTestAppActivity;
        mAlerterDialog = alerterDialog;
    }

    public void onClick(View v) {
        String path = null;
        switch (v.getId()) {
            case R.id.buttonFileInfo:
                if (null == mDrmTestAppActivity.getCurrentFile()) {
                    mAlerterDialog.setMessage("Please select a file that you want to view "
                            + "the info for.");
                    mAlerterDialog.show();
                    return;
                }
                path = mDrmTestAppActivity.getCurrentDir() + mDrmTestAppActivity.getCurrentFile();
                if (!Utils.isDrmFile(mDrmTestAppActivity.getApplicationContext(),path) &&
                        !Utils.isAudioVideo(path)) {
                    mAlerterDialog.setMessage("This is not a DRM or audio file type currently " +
                        "registered with the DRM framework.");
                    mAlerterDialog.show();
                } else {
                    mDrmTestAppActivity.showMetaData(path);
                }
                break;
            case R.id.buttonPlay:
                if (null == mDrmTestAppActivity.getCurrentFile()) {
                    mAlerterDialog.setMessage("Please select a file to play");
                    mAlerterDialog.show();
                    return;
                }
                path = mDrmTestAppActivity.getCurrentDir() + mDrmTestAppActivity.getCurrentFile();
                if (Utils.isDrmFile(mDrmTestAppActivity.getApplicationContext(),path)) {
                    mDrmTestAppActivity.playFile(path);
                    return;
                }
                if (Utils.isAudioVideo(path)) {
                    mDrmTestAppActivity.playFile(path);
                } else {
                    mAlerterDialog.setMessage("This file type can't be used with "
                            + "the Media Player.");
                    mAlerterDialog.show();
                }
                break;
            case R.id.buttonConvertFL:
                path = mDrmTestAppActivity.getCurrentDir() + mDrmTestAppActivity.getCurrentFile();
                if (null == mDrmTestAppActivity.getCurrentFile()) {
                    mAlerterDialog.setMessage("Please select the file that you want to convert.");
                    mAlerterDialog.show();
                    return;
                }
                if (!path.endsWith(".dm")) {
                    mAlerterDialog.setMessage("The selected file is not a forward "
                            + "lock DRM file.");
                    mAlerterDialog.show();
                    return;
                }
                try {
                    Utils.convertFLfile(mDrmTestAppActivity.getApplicationContext(),path);
                    mAlerterDialog.setMessage("Conversion was successful.");
                    mAlerterDialog.show();
                } catch (Exception e) {
                    mAlerterDialog.setMessage("Conversion failed.");
                    mAlerterDialog.show();
                }
                break;
            case R.id.buttonUpDir:
                mDrmTestAppActivity.onBackPressed();
                break;
        }
    }
}
