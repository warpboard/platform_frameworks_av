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

import android.database.Cursor;
import android.provider.MediaStore;
import android.util.Log;

/**
 * Container class for metadata.
 *
 */
public class MetaData {
    private String mTitle = "";
    private String mArtist = "";
    private String mAlbum = "";
    private String mYear = "";
    private String mTrackno = "";
    private String mAuthor = "";

    public MetaData(Cursor c) throws IllegalArgumentException{
        if (populateMetadata(c) > 0) {
            throw new IllegalArgumentException ();
        }
    }

    public String getTitle() {
        return mTitle;
    }

    public String getArtist() {
        return mArtist;
    }

    public String getAlbum() {
        return mAlbum;
    }

    public String getYear() {
        return mYear;
    }

    public String getTrackno() {
        return mTrackno;
    }

    public String getAuthor() {
        return mAuthor;
    }

    /**
     * Method to retrieve metadata of a file from the media store.
     * @param Cursor to retrieve the metadata for.
     * @return DrmMetaData when on success, all other cases null.
     */
    private int populateMetadata(Cursor c) {
        int result = -1;
        if (c.getCount() > 0) {
            try {
                c.moveToNext();
                int dataColumnIndex = c.getColumnIndexOrThrow(
                        MediaStore.Audio.AudioColumns.ARTIST);
                mArtist = c.getString(dataColumnIndex);
                dataColumnIndex = c.getColumnIndexOrThrow(MediaStore.Audio.AudioColumns.ALBUM);
                mAlbum = c.getString(dataColumnIndex);
                dataColumnIndex = c.getColumnIndexOrThrow(MediaStore.Audio.AudioColumns.TITLE);
                mTitle = c.getString(dataColumnIndex);
                dataColumnIndex = c.getColumnIndexOrThrow(
                        MediaStore.Audio.AudioColumns.COMPOSER);
                mAuthor = c.getString(dataColumnIndex);
                dataColumnIndex = c.getColumnIndexOrThrow(MediaStore.Audio.AudioColumns.TRACK);
                mTrackno = c.getString(dataColumnIndex);
                dataColumnIndex = c.getColumnIndexOrThrow(MediaStore.Audio.AudioColumns.YEAR);
                mYear = c.getString(dataColumnIndex);
                result = 0;
            } catch (IllegalArgumentException e) {
                Log.w(Constants.TAG, "Invalid column index.");
            } finally {
                c.close();
            }
        }
        return result;
    }
}
