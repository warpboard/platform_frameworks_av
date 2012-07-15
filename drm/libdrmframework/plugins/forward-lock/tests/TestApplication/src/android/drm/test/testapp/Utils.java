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
import android.drm.DrmManagerClient;
import android.util.Log;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class Utils {
    /**
     * Reads bytes from a stream input, from current position in stream.
     *
     * @param inStream stream to read from
     * @param readBuffer buffer where data shall be stored.
     * @return Number of bytes read from stream or -1 in case of error or end of stream.
     */
    public static int readBytesFromStream(InputStream inStream, byte[] readBuffer) {
        int byteread = 0;
        int numBytesRead = 0;
        try {
            do {
                byteread = inStream.read();
                if (byteread != -1) {
                    readBuffer[numBytesRead] = (byte)byteread;
                    numBytesRead++;
                }
              } while (byteread != -1 && numBytesRead < readBuffer.length);
        } catch (IOException e) {
            numBytesRead = -1;
            byteread = -1;
            Log.w(Constants.TAG, "Error reading from stream");
        }
        return numBytesRead > 0 ? numBytesRead : byteread;
    }

    /**
     * Converts a OMA DRM V1 Forward lock message file to internal Forward Lock Format.
     *
     * @param contex The context of application converting the file.
     * @param path Path to file to convert.
     * @return filename of the converted file.
     */
    public static String convertFLfile(Context context, String path) {
        ConvertHelper convertHelper = new ConvertHelper(context);
        String outFilename = null;
        if (convertHelper.startConvert(Constants.MIME_OMA_DRM_V1_MESSAGE) < 0) {
            Log.w(Constants.TAG, "Error starting convert session");
        } else {
            FileInputStream inStream = null;
            FileOutputStream outStream = null;

            try {
                byte [] inBuffer = new byte [Constants.FILE_BUFFER_SIZE];
                inStream = new FileInputStream(path);
                outFilename = convertHelper.createFileName(path);
                outStream = new FileOutputStream(outFilename);
                int numBytesRead = -1;
                while((numBytesRead = readBytesFromStream(inStream, inBuffer)) != -1) {
                    byte [] outBuffer = convertHelper.convert(inBuffer, numBytesRead);
                    if (outBuffer != null) {
                        outStream.write(outBuffer);
                    } else {
                        //an error occured while converting data
                        outFilename = null;
                        break;
                    }
                }
            } catch (FileNotFoundException e) {
                Log.w(Constants.TAG, "Error could not open File: "+path);
            } catch (IOException e) {
                Log.w(Constants.TAG, "Error could write to File: "+outFilename);
                e.printStackTrace();
            } finally {
                if (inStream != null) {
                    try {
                        inStream.close();
                    } catch (IOException e) {}
                }
                if (outStream != null) {
                    try {
                        outStream.close();
                    } catch (IOException e) {}
                }
            }
            if (convertHelper.stopConvert(outFilename) < 0) {
                Log.w(Constants.TAG, "Error ending convert session.");
            }
        }
        convertHelper.destroyConvertHelper();
        return outFilename;
    }

    /**
     * Check if a file is drm protected
     * @param context The running context.
     * @param path Path to file to check
     * @return true if file is drm protected in all other cases false.
     */
    public static boolean isDrmFile(Context context, String path) {
        DrmManagerClient drmManagerClient = new DrmManagerClient(context);
        boolean result = false;
        try {
            result = drmManagerClient.canHandle(path, "");
        } catch (IllegalArgumentException e) {
            Log.w(Constants.TAG, "Error path is an illegal argument");
        } catch (IllegalStateException e) {
            Log.w(Constants.TAG, "Could not access drm framework.");
        }
        return result;
    }

    /**
     * Check if a file is of audio or video type.
     * @param path Path to file to check
     * @return true if the file is audio or video and false in all other cases.
     */
    public static boolean isAudioVideo(String path) {
        if (path.length() >= 5) {
            String pathLower = path.toLowerCase();
            return (pathLower.endsWith(".mp3") || pathLower.endsWith(".wav") ||
                    pathLower.endsWith(".mp4") || pathLower.endsWith(".3gp") ||
                    pathLower.endsWith(".m4a") || pathLower.endsWith(".mid") ||
                    pathLower.endsWith(".ogg"));
        }
        return false;
    }

    /**
     * Check if a file is an image.
     * @param path Path to file to check
     * @return true if the file is an image and false in all other cases.
     */
    public static boolean isImageFile(String path) {
        if (path.length() >= 5) {
            String pathLower = path.toLowerCase();
            return (pathLower.endsWith(".jpg") || pathLower.endsWith(".gif") ||
                    pathLower.endsWith(".png") || pathLower.endsWith(".bmp"));
        }
        return false;
    }
}
