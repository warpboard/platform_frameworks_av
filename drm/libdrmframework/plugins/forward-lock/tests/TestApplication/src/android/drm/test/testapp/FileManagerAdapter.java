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
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.io.File;

/**
 * File manager Adapter class
 * Listing the files on file system in specific directory.
 *
 */
public class FileManagerAdapter extends ArrayAdapter<String> {
    private final DrmTestAppActivity mDrmTestAppActivity;
    private String mFileList[] = null;
    private String mDir = null;
    private int mMarkPosition = -1;

    FileManagerAdapter(DrmTestAppActivity drmTestAppActivity, String[] fileList, String dir) {
        super(drmTestAppActivity, R.layout.fileman, fileList);
        this.mDrmTestAppActivity = drmTestAppActivity;
        this.mFileList = fileList;
        this.mDir = dir;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        LayoutInflater inflater = mDrmTestAppActivity.getLayoutInflater();
        View row = inflater.inflate(R.layout.fileman, null);

        if (mFileList == null) {
            // Error, return empty row to avoid crash.
            return row;
        }
        if (mFileList.length == 0) {
            // Error, return empty row to avoid crash.
            return row;
        }
        if (position >= mFileList.length) {
            if (position >= (2 * mFileList.length)) {
                position = mFileList.length - 1;
            } else {
                int diff = position - (mFileList.length - 1);
                position = mFileList.length - diff;
            }
        }
        String path = mDir + mFileList[position];
        TextView label = (TextView) row.findViewById(R.id.filemanText);
        label.setText(mFileList[position]);
        if (position == mMarkPosition) {
            label.setBackgroundColor(Color.BLUE);
        }
        ImageView icon = (ImageView) row.findViewById(R.id.filemanImage);
        if ((new File(path)).isDirectory()) {
            icon.setImageResource(R.drawable.folder);
        } else if (Utils.isDrmFile(mDrmTestAppActivity, path)) {
            if (path.endsWith(".dm")) {
                icon.setImageResource(R.drawable.file);
            } else if (path.endsWith(".fl")) {
                icon.setImageResource(R.drawable.audiovideo);
            }
        } else if (Utils.isAudioVideo(path)) {
            icon.setImageResource(R.drawable.audiovideo);
        } else if (Utils.isImageFile(path)) {
            icon.setImageResource(R.drawable.image);
        } else  {
            icon.setImageResource(R.drawable.file);
        }
        return row;
    }

    public void setText(int position, String strangen) {
        mFileList[position] = strangen;
        this.notifyDataSetChanged();
    }

    public void setMarked(int position) {
        mMarkPosition = position;
        this.notifyDataSetChanged();
    }

    public void unMark() {
        mMarkPosition = -1;
        this.notifyDataSetChanged();
    }

    public void updateTheFileList(String[] newFileList) {
        mFileList = newFileList;
        this.notifyDataSetChanged();
    }
}
