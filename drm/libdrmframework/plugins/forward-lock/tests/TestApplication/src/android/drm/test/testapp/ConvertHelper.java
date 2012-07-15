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

import android.content.Context;
import android.drm.DrmConvertedStatus;
import android.drm.DrmManagerClient;
import android.util.Log;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

/**
 * Converts Oma drm v1 message file to internal format.
 */
public class ConvertHelper {

private DrmManagerClient mDrmManagerClient;

private int mConvertSessionID = -1;

    public ConvertHelper(Context context) {
        mDrmManagerClient = new DrmManagerClient(context);
    }

   /**
    * Start of converting a file.
    *
    * @param context The context of the application running the convert
    *            session.
    * @param mimeType Mimetype of content that shall be converted.
    * @return a convert session identifier or -1 incase an error occurs.
    */
    public int startConvert(String mimeType) {
        if (mimeType != null && !mimeType.equals("")) {
            try {
                if (mDrmManagerClient != null) {
                    try {
                        mConvertSessionID = mDrmManagerClient.openConvertSession(mimeType);
                    } catch (IllegalArgumentException e) {
                        Log.w(Constants.TAG, "Conversion of Mimetype: " + mimeType
                                + " is not supported.", e);
                    } catch (IllegalStateException e) {
                        Log.w(Constants.TAG, "Could not access Open DrmFramework.", e);
                    }
                }
            } catch (IllegalArgumentException e) {
                Log.w(Constants.TAG,
                        "DrmManagerClient instance could not be created, context is Illegal.");
            } catch (IllegalStateException e) {
                Log.w(Constants.TAG, "DrmManagerClient didn't initialize properly.");
            }
        }
        return mConvertSessionID;
    }

    /**
     * Convert a buffer of data to internal format.
     *
     * @param context The context of the application running the convert
     *            session.
     * @param convertSessionId The convert session identifier.
     * @param buffer Buffer filled with data to convert.
     * @param size The number of bytes that shall be converted.
     * @return A Buffer filled with converted data, if execution is ok, in all
     *         other case null.
     */
    public byte[] convert(byte[] buffer, int size) {
        byte[] result = null;
        if (mConvertSessionID >= 0 && buffer != null) {
            DrmConvertedStatus convertedStatus = null;
            try {
                Log.w(Constants.TAG,"buffer size to convert is = "+size);
                    if (size != buffer.length) {
                        byte[] buf = new byte[size];
                        System.arraycopy(buffer, 0, buf, 0, size);
                        convertedStatus = mDrmManagerClient.convertData(mConvertSessionID, buf);
                    } else {
                        convertedStatus = mDrmManagerClient.
                        convertData(mConvertSessionID, buffer);
                    }
                    if (convertedStatus != null) {
                        if (convertedStatus.statusCode == DrmConvertedStatus.STATUS_OK) {
                            if (convertedStatus.convertedData != null) {
                                result = convertedStatus.convertedData;
                            }
                        }
                    }
            } catch (IllegalArgumentException e) {
                Log.w(Constants.TAG, "Buffer with data to convert is illegal. Convertsession: "
                        + mConvertSessionID, e);
            } catch (IllegalStateException e) {
                Log.w(Constants.TAG, "Could not convert data. Convertsession: "
                        + mConvertSessionID, e);
            }
        }
        return result;
    }

    /**
     * Ends a conversion session of a file.
     *
     * @param context The context of the application running the convert session.
     * @param fileName The filename of the converted file.
     * @return 0 on success -1 on failure.
     */
    public int stopConvert(String fileName) {
        DrmConvertedStatus convertedStatus = null;
        int result = -1;
        if (mConvertSessionID >= 0) {
            try {
                convertedStatus = mDrmManagerClient.closeConvertSession(mConvertSessionID);
                if (convertedStatus != null) {
                    if (convertedStatus.statusCode == DrmConvertedStatus.STATUS_OK) {
                        if (convertedStatus.convertedData != null) {
                            // write signature.
                            RandomAccessFile rndAccessFile = null;
                            try {
                                rndAccessFile = new RandomAccessFile(fileName, "rw");
                                rndAccessFile.seek(convertedStatus.offset);
                                rndAccessFile.write(convertedStatus.convertedData);
                                result = 0;
                            } catch (FileNotFoundException e) {
                                Log.w(Constants.TAG, "File: " + fileName
                                        + " could not be found.",e);
                            } catch (IOException e) {
                                Log.w(Constants.TAG, "Could not access File: " +
                                        fileName+ " .", e);
                            } catch (IllegalArgumentException e) {
                                Log.w(Constants.TAG, "Could not open file in mode: rw", e);
                            } catch (SecurityException e) {
                                Log.w(Constants.TAG, "Access to File: " + fileName
                                        + " was denied denied by SecurityManager.", e);
                            } finally {
                                if (rndAccessFile != null) {
                                    try {
                                        rndAccessFile.close();
                                        rndAccessFile = null;
                                    } catch (IOException e) {
                                        Log.w(Constants.TAG, "Failed to close File:"
                                                + fileName+ ".", e);
                                    }
                                }
                            }
                        }
                    }
                }
            } catch (IllegalStateException e) {
                Log.w(Constants.TAG, "Could not close convertsession. Convertsession: "
                        + mConvertSessionID, e);
            }
        }
        return result;
    }

    /**
     * Create a filename for a Forward Lock file of internal format
     * based on a filename of a Oma DRM v1 message file.
     *
     * @param filename Filename of a oma drm message file.
     * @return Filename for converted file.
     */
    public String createFileName(String filename) {
        if (filename != null) {
            int extensionIndex;
            extensionIndex = filename.lastIndexOf(".");
            if (extensionIndex != -1) {
                filename = filename.substring(0, extensionIndex);
            }
            filename = filename.concat(Constants.FILE_EXT_FWDL_INTERNAL);
        }
        return filename;
    }

    /**
     * Destroys the convert helper.
     * Releases DrmManagerClient
     * Closes the convert session(if not already closed)
     */
    public void destroyConvertHelper() {
        if (mConvertSessionID >= 0) {
            if (mDrmManagerClient != null) {
                mDrmManagerClient.closeConvertSession(mConvertSessionID);
            }
        }
    }
}
