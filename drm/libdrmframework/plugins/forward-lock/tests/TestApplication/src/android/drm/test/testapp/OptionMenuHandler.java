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

import android.content.Intent;
import android.net.Uri;

/**
 * Class Handling the main activity menu option events.
 */
public class OptionMenuHandler {
    /**
     * Handle option menu events that occurs in the main activity.
     * @param mainActivity MainActivity class of this application
     * @param itemId the option itemid that shall be handled.
     * @return true if handled, in all other cases false.
     */
    public static boolean handleOptionEvent(DrmTestAppActivity drmTestAppActivity, int itemId) {
        String path = null;
        boolean result = true;
        if (drmTestAppActivity.getCurrentDir() != null &&
                drmTestAppActivity.getCurrentFile() != null) {
            path = drmTestAppActivity.getCurrentDir() + drmTestAppActivity.getCurrentFile();
        }
        switch (itemId) {
            case Constants.MENU_PLAY:
                if (path == null) {
                    drmTestAppActivity.mNotifyDialog.setMessage("Please select a file to play");
                    drmTestAppActivity.mNotifyDialog.show();
                } else {
                    if (Utils.isDrmFile(drmTestAppActivity.getApplicationContext(),path)) {
                        drmTestAppActivity.playFile(path);
                    } else if (Utils.isAudioVideo(path)) {
                        drmTestAppActivity.playFile(path);
                    } else {
                        drmTestAppActivity.mNotifyDialog.setMessage(
                                "This file type can't be used with the Media Player.");
                        drmTestAppActivity.mNotifyDialog.show();
                    }
                }
                break;
            case Constants.MENU_PLAY_ANDROID_AUDIO:
                if (path != null) {
                    if (!path.startsWith("/")) {
                        path = "/" + path;
                    }
                    String uriString = "file://" + path;
                    Intent i=new Intent(Intent.ACTION_VIEW);
                    i.setDataAndType(Uri.parse(uriString), "audio" + "/*");
                    drmTestAppActivity.startActivity(i);
                }
                result = true;
            case Constants.MENU_PLAY_ANDROID_VIDEO:
                if (path != null) {
                    if (!path.startsWith("/")) {
                        path= "/" + path;
                    }
                    String uriString2 = "file://" + path;
                    Intent i2=new Intent(Intent.ACTION_VIEW);
                    i2.setDataAndType(Uri.parse(uriString2), "video" + "/*");
                    drmTestAppActivity.startActivity(i2);
                }
                break;
            case Constants.MENU_CONVERT_FORWARD_LOCK:
                if (path == null) {
                    drmTestAppActivity.mNotifyDialog.setMessage(
                            "Please select the file that you want to convert.");
                    drmTestAppActivity.mNotifyDialog.show();
                } else {
                    if (!path.endsWith(".dm")) {
                        drmTestAppActivity.mNotifyDialog.setMessage(
                                "The selected file is not a forward lock DRM file.");
                        drmTestAppActivity.mNotifyDialog.show();
                    } else {
                        try {
                            Utils.convertFLfile(drmTestAppActivity.getApplicationContext(),path);
                            drmTestAppActivity.mNotifyDialog.
                            setMessage("Conversion was successful.");
                            drmTestAppActivity.mNotifyDialog.show();
                        } catch (Exception e) {
                            drmTestAppActivity.mNotifyDialog.setMessage(
                                    "Conversion failed with message: " + e.getMessage());
                            drmTestAppActivity.mNotifyDialog.show();
                        }
                    }
                }
                break;
            case Constants.MENU_FILE_INFO:
                if (path == null) {
                    drmTestAppActivity.mNotifyDialog.setMessage(
                            "Please select a file that you want to view info for.");
                    drmTestAppActivity.mNotifyDialog.show();
                } else {
                    if (!Utils.isDrmFile(drmTestAppActivity.getApplicationContext(),path) &&
                            !Utils.isAudioVideo(path)) {
                        drmTestAppActivity.mNotifyDialog.setMessage(
                                "This is not a DRM or audio file type currently " +
                                "registered with the DRM framework.");
                        drmTestAppActivity.mNotifyDialog.show();
                    } else {
                        drmTestAppActivity.showMetaData(path);
                    }
                }
                break;
            default:
                result = false;
                break;
        }
        return result;
    }
}
