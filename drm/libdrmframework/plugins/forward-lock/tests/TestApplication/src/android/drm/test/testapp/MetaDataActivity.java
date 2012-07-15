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

import android.drm.test.testapp.R;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.widget.TextView;

/**
 * Activity to display metadata related to a file.
 */
public class MetaDataActivity extends Activity {
    private TextView mTextArtist;
    private TextView mTextAlbum;
    private TextView mTextTitle;
    private TextView mTextAuthor;
    private TextView mTextTrackNo;
    private TextView mTextYear;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.metadata);
        mTextArtist = (TextView)findViewById(R.id.artist);
        mTextAlbum = (TextView)findViewById(R.id.album);
        mTextTitle = (TextView)findViewById(R.id.title);
        mTextAuthor = (TextView)findViewById(R.id.author);
        mTextTrackNo = (TextView)findViewById(R.id.trackno);
        mTextYear = (TextView)findViewById(R.id.year);
        String path = "/mnt" + getIntent().getData().getPath();
        Uri songUri = android.provider.MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        Cursor c = managedQuery(songUri,null,
                MediaStore.MediaColumns.DATA + " = ?", new String[]{path}, null);
        try {
            MetaData metaData = new MetaData(c);
            mTextArtist.setText(metaData.getArtist());
            mTextAlbum.setText(metaData.getAlbum());
            mTextTitle.setText(metaData.getTitle());
            mTextAuthor.setText(metaData.getAuthor());
            mTextTrackNo.setText(metaData.getTrackno());
            mTextYear.setText(metaData.getTrackno());
        } catch (IllegalArgumentException e) {
            AlertDialog.Builder dlgNotify = new AlertDialog.Builder(this);
            dlgNotify.setTitle(Constants.TAG);
            dlgNotify.setNeutralButton("Close", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dlg, int sumthin) {
                    finish();
                }
            });
            dlgNotify.setMessage("An error occured while retrieving metadata.");
            dlgNotify.show();
        } finally {
            if (c != null) {
                c.close();
            }
        }
    }
}
