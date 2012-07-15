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

import android.media.MediaPlayer;
import android.util.Log;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

/**
 * Class responsible for keeping the progress bar of a rendering session up to data.
 * update happens every 50ms while rendering and in case user updates the current position.
 */
public class ProgressBarHandler extends Thread implements OnSeekBarChangeListener{
private MediaPlayer mMediaPlayer;
private SeekBar mSeekBar;
private boolean mUserSeek = false;

    public ProgressBarHandler(MediaPlayer mediaPlayer, SeekBar seekBar) {
        mMediaPlayer= mediaPlayer;
        mSeekBar = seekBar;
    }

    public void run() {
        while (true) {
            try {
                if (!mUserSeek) {
                    mSeekBar.setProgress(mMediaPlayer.getCurrentPosition());
                }
                try {
                    Thread.sleep(50);
                } catch(InterruptedException e) {
                    Log.w(Constants.TAG, "Progress thread was interrupted unexpectedly.");
                }
            } catch (NullPointerException e) {
                break;
            } catch (IllegalStateException e) {
                break;
            }
        }
    }

    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
    }

    public void onStartTrackingTouch(SeekBar seekBar) {
        mUserSeek = true;
    }

    public void onStopTrackingTouch(SeekBar seekBar) {
        try {
            mMediaPlayer.seekTo(seekBar.getProgress());
        } catch (IllegalStateException e) {}
        mUserSeek = false;
    }
}
