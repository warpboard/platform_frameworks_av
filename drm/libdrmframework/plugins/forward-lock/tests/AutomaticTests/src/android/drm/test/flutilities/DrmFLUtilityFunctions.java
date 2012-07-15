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

import android.app.Activity;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.drm.DrmConvertedStatus;
import android.drm.DrmManagerClient;
import android.drm.test.flapp.FLTestActivity;
import android.media.MediaPlayer;
import android.util.DisplayMetrics;
import android.util.Log;
import android.webkit.CookieManager;
import android.webkit.URLUtil;
import android.widget.Toast;
import android.net.WebAddress;
import android.provider.Downloads;
import android.content.ContentValues;
import android.content.Context;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.util.Calendar;
import java.util.Vector;

/**
 * Utility class for the forward lock tests. Handles conversions, file operations etc.
 */
public class DrmFLUtilityFunctions {
    public final static String DRM_TEST_TAG = "Drm Test";
    public final static String DRM_ACTION_DOWNLOAD_COMPLETE =
        "android.drm.test.DOWNLOAD_COMPLETE";
    public final static String FL_MIMETYPE_DM = "application/vnd.oma.drm.message";
    public final static String FL_MIMETYPE_FL = "application/x-android-drm-fl";
    public final static String AUDIO_MIMETYPE_MPEG = "audio/mpeg";
    public final static String AUDIO_MIMETYPE_M4A = "audio/mp4a-latm";
    public final static String AUDIO_MIMETYPE_WAV = "audio/wav";
    public final static String VIDEO_MIMETYPE_3GP = "video/3gpp";
    public final static String VIDEO_MIMETYPE_MP4 = "video/mp4";

    /**
     * Puts a project resource file at a specified location
     * on the target device/emulator.
     *
     * @param id Android project resource ID.
     * @param filename Filename that the resource will have on the target device/emulator.
     * @param foldername Path of a folder where the file should be put.
     * @param context The activity context in in which this function is executed.
     * @return The full path of the file on the device/emulator.
     *
     * @throws IOException
     * @throws IllegalArgumentException
     */
    public final static String putOnPhone(int id, String filename, String foldername,
            Activity context) throws IOException, IllegalArgumentException {
        if ((filename == null) || (foldername == null) || (context == null)) {
            throw new IllegalArgumentException();
        }
        DisplayMetrics metrics = new DisplayMetrics();
        context.getWindowManager().getDefaultDisplay().getMetrics(metrics);
        Configuration defaultConfig = new Configuration();
        defaultConfig.setToDefaults();
        AssetManager testAssetManager = context.getApplication().getAssets();

        File testFileDirectory = new File(foldername);
        if (!testFileDirectory.exists()) {
            boolean testBool = testFileDirectory.mkdirs();
            if (!testBool) {
                throw new IOException("Could not create the test file directory!" +
                        " You probably haven't got a sdcard in your phone or you haven't " +
                        "set the WRITE_EXTERNAL_STORAGE permission" +
                        " in your manifest file.");
            }
        }
        File file = new File(foldername + filename);
        if (file.exists()) {
            // if an old converted file exists on the phone, just overwrite it
            if (!file.delete()) {
                throw new IOException("There is already a converted file with the " +
                        "same name on the sdcard and this file could not be deleted.");
            }
        }
        if (!file.createNewFile()) {
            throw new IOException("Could not create the file in the destination folder.");
        }

        FileOutputStream outputFile = new FileOutputStream(foldername + filename);
        Resources testResources = new Resources(testAssetManager, metrics, defaultConfig);
        InputStream resInputStream = testResources.openRawResource(id);
        try {
            byte[] readBuffer = new byte[4096];
            int bytes = 0;
            while ((bytes = resInputStream.read(readBuffer)) != -1) {
                outputFile.write(readBuffer, 0, bytes);
            }
        } finally {
            resInputStream.close();
            outputFile.close();
        }

        return (foldername + filename);
    }

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
        }
        return numBytesRead > 0 ? numBytesRead : byteread;
    }

   /**
     * Converts a forward lock file of the type application/vnd.oma.drm.message
     * to an internal encrypted format.
     *
     * @param pathTodmFile Path to the forward lock file which should be converted.
     *
     * @param manager {@link DrmManagerClient} instance which should perform the conversion.
     *
     * @param foldername Path of a folder where the file should be put.
     *
     * @param context The activity context in which this function is executed.
     *
     * @return The full path of the file on the device/emulator.
     *
     * @throws IOException
     *
     * @throws IllegalArgumentException
     */
    public final static String convertFLfile(String pathTodmFile, DrmManagerClient manager)
    throws IOException, IllegalArgumentException {
        // Converts .dm file to internal forward lock format
        if (pathTodmFile == null || manager == null) {
            throw new IllegalArgumentException();
        }
        int returnedId = -1;
        returnedId = manager.openConvertSession(FL_MIMETYPE_DM);
        if (returnedId < 0) {
            throw new IOException("The FL engine failed to open the convert session.");
        }
        RandomAccessFile dmFile = null;
        dmFile = new RandomAccessFile(pathTodmFile, "r");

        if (dmFile.length() == 0) {
            dmFile.close();
            deleteFile(pathTodmFile);
            throw new IOException("The FL file given as input had the size of zero bytes.");
        }
        long lastBufferSize = dmFile.length() % 4096;
        long numberOfIterations = (dmFile.length() / 4096) + 1;
        byte[] byteBuffer = new byte[4096];
        String newFilePath = pathTodmFile.substring(0, pathTodmFile.length() - 4);
        newFilePath += ".fl";
        RandomAccessFile flFile = new RandomAccessFile(newFilePath, "rw");
        flFile.getChannel().truncate(1);
        long newPositionWriteBuffer = 0;

        for (int i = 0; i < numberOfIterations; ++i) {
            dmFile.seek(4096 * i);
            if (i == (numberOfIterations - 1)) {
                if (lastBufferSize == 0) {
                    break;
                }
                byteBuffer = new byte[(int)lastBufferSize];
            }
            if (dmFile.read(byteBuffer) == -1) {
                dmFile.close();
                flFile.close();
                deleteFile(pathTodmFile);
                throw new IOException("Error in FL conversion, reached the end of " +
                        "the file unexpectedly.");
            }
            DrmConvertedStatus convStatus = manager.convertData(returnedId, byteBuffer);
            if (convStatus.statusCode != DrmConvertedStatus.STATUS_OK) {
                dmFile.close();
                flFile.close();
                deleteFile(pathTodmFile);
                throw new IOException("Error in FL conversion, conversion returned not " +
                        "OK for one of the buffers.");
            }
            flFile.seek(newPositionWriteBuffer);
            if (convStatus.convertedData != null) {
                flFile.write(convStatus.convertedData);
                newPositionWriteBuffer += convStatus.convertedData.length;
            }
        }
        DrmConvertedStatus convStatus2 = null;
        convStatus2 = manager.closeConvertSession(returnedId);

        if (convStatus2 != null) {
            if (convStatus2.statusCode != DrmConvertedStatus.STATUS_OK) {
                dmFile.close();
                flFile.close();
                deleteFile(pathTodmFile);
                throw new IOException("Error in FL conversion, " +
                        "conversion returned not OK when closing the convert session.");
            }
            if (convStatus2.statusCode == DrmConvertedStatus.STATUS_OK) {
                if (convStatus2.convertedData != null) {
                    // write signature.
                    flFile.seek(convStatus2.offset);
                    flFile.write(convStatus2.convertedData);
                }
            }
        }
        dmFile.close();
        flFile.close();
        deleteFile(pathTodmFile);

        return newFilePath;
    }

    /**
     * Puts a forward lock resource file at a specified location
     * on the target device/emulator and converts it to the internal encrypted format.
     *
     * @param id Android resource ID.
     * @param filename Filename that the forward lock resource should have on
     *        the device/emulator.
     * @param foldername Path of a folder where the file should be put.
     * @param context The activity context in which this function is executed.
     * @param manager {@link DrmManagerClient} instance which should perform the conversion.
     * @return The full path of the file on the device/emulator.
     *
     * @throws IOException
     * @throws IllegalArgumentException
     */
    public final static String putOnPhoneAndConvert(int id, String filename, String foldername,
                                                    Activity context,
                                                    DrmManagerClient manager)
    throws IOException, IllegalArgumentException {
        if ((filename == null) || (foldername == null) || (context ==null) || (manager == null)) {
            throw new IllegalArgumentException();
        }
        String resultingFilePath = putOnPhone(id, filename, foldername, context);
        return convertFLfile(resultingFilePath, manager);
    }

    /**
     * Deletes a file from the device/emulator.
     *
     * @param pathToFile Full path to the file which should be deleted.
     *
     * @throws IllegalArgumentException
     * @throws SecurityException
     */
    public final static void deleteFile(String pathToFile) throws
    IllegalArgumentException, SecurityException {
        if (pathToFile == null) {
            throw new IllegalArgumentException();
        }
        File file = new File(pathToFile);
        if (file.exists()) {
            if (!file.delete()) {
                Log.w(DRM_TEST_TAG, "Could not delete the file " + pathToFile);
            }
        }
    }

    /**
     * Deletes a folder from the device/emulator.
     *
     * @param pathToFolder Full path to the folder which should be deleted.
     *
     * @throws IllegalArgumentException
     * @throws SecurityException
     */
    public final static void deleteFolder(String pathToFolder) throws
    IllegalArgumentException, SecurityException {
        if (pathToFolder == null) {
            throw new IllegalArgumentException();
        }
        File folder = new File(pathToFolder);
        if (folder.exists()) {
            File files[] = folder.listFiles();
            if (files != null) {
                for (int i = 0; i < files.length; ++i) {
                    if (files[i].isDirectory()) {
                        deleteFolder(files[i].getAbsolutePath());
                    } else if (!files[i].delete()) {
                        Log.w(DRM_TEST_TAG, "Could not delete the file "
                                + files[i].getAbsolutePath());
                    }
                }
            }
            if (!folder.delete()) {
                Log.w(DRM_TEST_TAG, "Could not delete the folder: " + pathToFolder);
            }
        }
    }

    /**
     * Checks if a certain DRM plugin is loaded in the DRM framework.
     *
     * @param manager {@link DrmManagerClient} instance to look for the plugin with.
     * @return true if the plugin is found and false if it isn't.
     *
     * @throws IllegalArgumentException
     */
    public final static boolean isPluginLoaded(String pluginName,
                                               DrmManagerClient manager)
    throws IllegalArgumentException {
        if ((pluginName == null) || (manager == null)) {
            throw new IllegalArgumentException();
        }
        String[] testStringArray = manager.getAvailableDrmEngines();
        boolean found = false;
        for (int i = 0; i < testStringArray.length; ++i) {
            if (testStringArray[i].equals(pluginName))
            {
                found = true;
                break;
            }
        }
        return found;
    }

    /**
     * Gets the bytes of a project resource file and returns them in
     * an array.
     *
     * @param id The project ID of the resource file.
     * @param context The activity context in which this function is executed.
     * @return The bytes of the resource file.
     *
     * @throws IOException
     * @throws IllegalArgumentException
     */
    public final static byte[] getBytesFromResource(int id,
                                                    Activity context)
    throws IOException, IllegalArgumentException {
        if (context == null) {
            throw new IllegalArgumentException();
        }
        DisplayMetrics metrics = new DisplayMetrics();
        context.getWindowManager().getDefaultDisplay().getMetrics(metrics);
        Configuration defaultConfig = new Configuration();
        defaultConfig.setToDefaults();
        AssetManager testAssetManager = context.getApplication().getAssets();
        Resources testResources = new Resources(testAssetManager, metrics, defaultConfig);
        InputStream resInputStream = testResources.openRawResource(id);
        Vector<Byte> byteVector = new Vector<Byte>();
        Integer readInt = Integer.valueOf(0);
        try {
            while (true) {
                readInt = resInputStream.read();
                if (readInt.intValue() == -1) {
                    break;
                }
                byteVector.add(Byte.valueOf(readInt.byteValue()));
            }
        } finally {
            resInputStream.close();
        }
        byte[] returnBytes = new byte[byteVector.size()];
        for (int i = 0; i < byteVector.size(); ++i) {
            returnBytes[i] = byteVector.get(i);
        }
        return returnBytes;
    }

    /**
     * Creates a file from a byte array and puts it on the device/emulator.
     *
     * @param folderName The path to the folder on the emulator/device
     * in which the file should be put.
     *
     * @param filename The filename of the file that will be created.
     * @param testData The byte array that will be written to file.
     * @return The full path of the file created on the phone.
     *
     * @throws IOException
     * @throws IllegalArgumentException
     */
    public static String createFileFromByteArrayAndPutOnPhone(String folderName,
                                                              String filename,
                                                              byte[] testData)
    throws IOException, IllegalArgumentException {
        if ((filename == null) || (testData == null)) {
            throw new IllegalArgumentException();
        }
        File testFileDirectory = new File(folderName);
        if (!testFileDirectory.exists()) {
            boolean testBool = testFileDirectory.mkdirs();
            if (!testBool) {
                throw new IOException("Could not create the test file directory!" +
                        " You probably haven't got a sdcard in your phone or you haven't " +
                        "set the WRITE_EXTERNAL_STORAGE permission" +
                        " in your manifest file.");
            }
        }
        File file = new File(folderName + filename);
        if (file.exists()) {
            // if an old converted file exists on the phone, just overwrite it
            if (!file.delete()) {
                throw new IOException("There is already a converted file with the " +
                        "same name on the sdcard and this file could not be deleted.");
            }
        }
        if (!file.createNewFile()) {
            throw new IOException("Could not create the file in the destination folder.");
        }
        FileOutputStream outputFile = new FileOutputStream(folderName + filename);
        try {
            outputFile.write(testData);
        } finally {
            outputFile.close();
        }
        return (folderName + filename);
    }

    /**
     * Downloads a file from a web server to the sdcard.
     *
     * @param urlToFile The URL of the file to download.
     * @param mimetype The mimetype of the file to download.
     * @param context The test activity context used for downloading.
     *
     * @return The full path of the file downloaded given that the download was successful.
     *         For unsuccessful downloads the function returns "" (empty string).
     *
     * @throws InterruptedException
     *
     * @throws IllegalArgumentException
     */
    public static String downloadContent(String urlToFile, String mimetype,
                                         FLTestActivity context)
    throws InterruptedException, IllegalArgumentException {
        if ((urlToFile == null) || (mimetype == null) || (context == null)) {
            throw new IllegalArgumentException();
        }
        DownloadCompleteDynamicReceiver listener = context.getDownloadCompleteReceiver();
        listener.resetLatch();
        WebAddress adressToFile;
        try {
            adressToFile = new WebAddress(urlToFile);
        } catch (Exception e) {
            Log.w(DRM_TEST_TAG, "Exception trying to parse url to " +
                    "the test file (please verify the link):");
            return "";
        }
        String filenameGuess = URLUtil.guessFileName(urlToFile,
                null, mimetype);
        ContentValues downloadValues = new ContentValues();
        downloadValues.put(Downloads.Impl.COLUMN_URI, adressToFile.toString());
        downloadValues.put(Downloads.Impl.COLUMN_NOTIFICATION_PACKAGE,
                context.getPackageName());
        downloadValues.put(Downloads.Impl.COLUMN_NOTIFICATION_CLASS,
                DownloadCompleteStaticReceiver.class.getCanonicalName());
        downloadValues.put(Downloads.Impl.COLUMN_VISIBILITY,
                Downloads.Impl.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
        downloadValues.put(Downloads.Impl.COLUMN_MIME_TYPE, mimetype);
        downloadValues.put(Downloads.Impl.COLUMN_FILE_NAME_HINT, filenameGuess);
        downloadValues.put(Downloads.Impl.COLUMN_DESTINATION, Downloads.Impl.DESTINATION_EXTERNAL);
        downloadValues.put(Downloads.Impl.COLUMN_DESCRIPTION, adressToFile.getHost());
        context.getContentResolver().insert(Downloads.Impl.CONTENT_URI, downloadValues);

        listener.getLatch().await();
        String filename = listener.getDownloadedFileName();
        if (filename == null) {
            Log.w("DrmFLTest", "downloadContent failed because the filename" +
                    "returned from the downloadProvider after downloading was null");
            return "";
        }
        File file = new File(filename);
        if (file.exists()) {
            return filename;
        } else {
            return "";
        }
    }

    /**
     * Plays an audio or video file.
     *
     * @param filePath The full path to the file.
     * @param timeToPlay The duration of the playing.
     * @return true if the file could be played and false otherwise.
     *
     * @throws InterruptedException
     * @throws IllegalStateException
     * @throws IOException
     * @throws IllegalArgumentException
     */
    public static boolean playAndVerifyPlaying(String filePath,
                                               long timeToPlay)
    throws IllegalArgumentException, IllegalStateException, IOException, InterruptedException {
        MediaPlayer mp = new MediaPlayer();
        boolean isPlaying = false;
        try {
            mp.setDataSource(filePath);
            mp.prepare();
            mp.start();
            Thread.sleep(timeToPlay);
            if (mp.isPlaying()) {
                isPlaying = true;
            } else {
                isPlaying = false;
            }
            mp.stop();
        } finally {
            mp.release();
        }
        return isPlaying;
    }

    /**
     * First a forward lock protected audio or video file is played. During the playing,
     * time is measured. Then the corresponding file in the unprotected format will be played
     * and the difference in setup time of the media player is calculated. This gives a
     * measure on how much DRM degrades the media playing performance.
     *
     * @param fileProt The full path to the FL file.
     * @param fileUnProt The full path to the unprotected file.
     * @param timeToPlay The duration of the playing.
     * @return true if the file could be played and false otherwise.
     *
     * @throws InterruptedException
     * @throws IllegalStateException
     * @throws IOException
     * @throws IllegalArgumentException
     */
    public static long diffStartPlayProtAndUnprotFile(String fileProt, String fileUnProt,
                                                      long timeToPlay)
    throws IllegalArgumentException, IllegalStateException, IOException, InterruptedException {
        MediaPlayer mp = new MediaPlayer();
        long timeDiff = 0, time1 = 0, time2 = 0;
        try {
            Calendar time = Calendar.getInstance();
            mp.setDataSource(fileProt);
            mp.prepare();
            time1 =  time.getTimeInMillis();
            mp.start();
            Thread.sleep(timeToPlay);
            timeDiff =  time.getTimeInMillis();
            time1 = timeDiff - time1;
            mp.stop();
            mp.reset();
            mp.setDataSource(fileUnProt);
            mp.prepare();
            time2 =  time.getTimeInMillis();
            mp.start();
            Thread.sleep(timeToPlay);
            timeDiff =  time.getTimeInMillis();
            time2 = timeDiff - time2;
            mp.stop();
        }
        finally {
            mp.release();
        }
        timeDiff = time1 - time2;
        return timeDiff;
    }
}
