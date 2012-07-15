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
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.ListView;
import android.widget.AdapterView.OnItemClickListener;

import java.io.File;

public class DrmTestAppActivity extends Activity implements OnItemClickListener {
    AlertDialog.Builder mNotifyDialog = null;
    ListView mListViewFile;
    String[] mCurDirList;
    private final String ROOT_DIR = "/";
    private String mCurDir = ROOT_DIR;
    private String mCurFile = null;
    FileManagerAdapter mFileList;
    private int mLastMarked = -1;
    Thread mTimerThread;
    ButtonActionHandler mButtonActionHandler;

    // Handler constants
    public static final int HANDLER_MESSAGE_TIMER = 2;

    Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (mCurDir != null) {
                if ((new File(mCurDir)).canRead()) {
                    String[] newFileList = (new File(mCurDir)).list();
                    mCurDirList = newFileList;
                    mFileList.updateTheFileList(newFileList);
                } else {
                    // It seems like memory card has been removed, goto root.
                    listDir("/");
                }
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mNotifyDialog = new AlertDialog.Builder(this);
        mNotifyDialog.setTitle("DRM Test Application");
        mNotifyDialog.setNeutralButton("Close", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dlg, int sumthin) {
            }
        });
        setContentView(R.layout.main);
        mListViewFile = (ListView)findViewById(R.id.listviewFile);
        mListViewFile.setOnItemClickListener(this);
        mListViewFile.setSelection(0);
        listDir(ROOT_DIR);
        setUpButtons();
    }

    @Override
    public void onResume() {
        super.onResume();
        mTimerThread = new Thread(new Runnable() {
            // Used to check if the file manager needs to be updated.
            public void run() {
                try {
                    while (true) {
                        // This thread repeats until interrupted in the OnPause() of the Activity.
                        Thread.sleep(5000);
                        handler.sendMessage(handler.obtainMessage());
                    }
                } catch(InterruptedException ex) {
                    // Thrown when executing timerThread.interrupt()
                    // in the OnPause() of the Activity.
                }
            }
        });
        mTimerThread.start();
    }

    @Override
    public void onPause() {
        mTimerThread.interrupt();
        super.onPause();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        if (mCurDir.equals(ROOT_DIR) || mCurDir.equals("")) {
             // If we are in the root folder, pressing the back button
             // will minimize the activity.
            finish();
        } else {
            stepUpOneLevel();
        }
    }

    public void playFile(String path) {
        Uri uriSend = Uri.parse("file://" + path);
        Intent i = new Intent(Intent.ACTION_DEFAULT, uriSend, this,
                android.drm.test.testapp.DrmMediaPlayerActivity.class);
        startActivity(i);
    }

    public void showMetaData(String path) {
        Uri uriSend = Uri.parse("file://" + path);
        Intent i = new Intent(Intent.ACTION_DEFAULT, uriSend, this,
                android.drm.test.testapp.MetaDataActivity.class);
        startActivity(i);
    }

    public void stepUpOneLevel() {
        if (!mCurDir.equals(ROOT_DIR)) {
            String temp = mCurDir.substring(0, mCurDir.length() - 1);
            mCurDir = temp.substring(0, temp.lastIndexOf("/") + 1);
            listDir(mCurDir);
        }
    }

    public String getCurrentFile() {
        return mCurFile;
    }

    public String getCurrentDir() {
        return mCurDir;
    }

    /* Creates the menu items */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        menu.add(0, Constants.MENU_PLAY, 0, "Play");
        menu.add(0, Constants.MENU_PLAY_ANDROID_AUDIO, 0, "Play in Android audio app");
        menu.add(0, Constants.MENU_PLAY_ANDROID_VIDEO, 0, "Play in Android video app");
        menu.add(0, Constants.MENU_CONVERT_FORWARD_LOCK, 0, "Convert forward lock file");
        menu.add(0, Constants.MENU_FILE_INFO, 0, "View metadata");
        return true;
    }

    /* Handles item selections */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        super.onOptionsItemSelected(item);
        return OptionMenuHandler.handleOptionEvent(this, item.getItemId());
    }

    public void onItemClick(AdapterView<?> parent, View v, int position, long id) {
        mCurFile = mCurDirList[position];
        String path = mCurDir + mCurFile;
        if ((new File(path)).isDirectory()) {
            mFileList.unMark();
            listDir(path + "/");
            mCurFile = null;
        } else {
            mFileList.unMark();
            if (position != mLastMarked) {
                mFileList.setMarked(position);
                mLastMarked = position;
            } else {
                mCurFile = null;
                mLastMarked = -1;
            }
        }
    }

    private void listDir(String dir) {
        mCurFile = null;
        String[] directoryList = (new File(dir)).list();
        if (directoryList != null) {
            mCurDir = dir;
            mCurDirList = directoryList;
        } else {
            return;
        }
        mFileList = new FileManagerAdapter(this, mCurDirList, mCurDir);
        // Bind the array adapter to the listview.
        mListViewFile.setAdapter(mFileList);
        mListViewFile.setSelection(0);
    }

    private void setUpButtons() {
        mButtonActionHandler = new ButtonActionHandler(mNotifyDialog, this);
        Button button = (Button)findViewById(R.id.buttonConvertFL);
        button.setOnClickListener(mButtonActionHandler);
        button = (Button)findViewById(R.id.buttonFileInfo);
        button.setOnClickListener(mButtonActionHandler);
        button = (Button)findViewById(R.id.buttonPlay);
        button.setOnClickListener(mButtonActionHandler);
        button = (Button)findViewById(R.id.buttonUpDir);
        button.setOnClickListener(mButtonActionHandler);
    }

}
