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
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.ImageButton;
import android.widget.SeekBar;

/**
 * Drm media player activity class.
 */
public class DrmMediaPlayerActivity extends Activity {
    private SurfaceView mPreview;
    private ImageButton mPlay;
    private ImageButton mPause;
    private ImageButton mReset;
    private ImageButton mStop;
    private SeekBar mSeekMedia;
    private DrmMediaPlayer mDrmMediaPlayer;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.mpmain);
        mPreview = (SurfaceView) findViewById(R.id.surface);
        mPlay = (ImageButton) findViewById(R.id.play);
        mPause = (ImageButton) findViewById(R.id.pause);
        mReset = (ImageButton) findViewById(R.id.reset);
        mStop = (ImageButton) findViewById(R.id.stop);
        mSeekMedia = (SeekBar)  findViewById(R.id.seekMedia);

        mPlay.setImageResource(R.drawable.play);
        mPlay.setMaxHeight(15);
        mPlay.setMaxWidth(15);
        mStop.setImageResource(R.drawable.stop);
        mPause.setImageResource(R.drawable.pause);
        mReset.setImageResource(R.drawable.reset);
        mSeekMedia.setHorizontalScrollBarEnabled(true);
        mSeekMedia.setMax(10);

        getWindow().setFormat(PixelFormat.TRANSPARENT);

        mPreview.setEnabled(true);
        mPreview.setVisibility(SurfaceView.VISIBLE);
        mPreview.bringToFront();
        mPreview.dispatchWindowVisibilityChanged(SurfaceView.VISIBLE);

        Intent fIntent = getIntent();
        final String path = fIntent.getData().getPath();
        mDrmMediaPlayer = new DrmMediaPlayer(mSeekMedia, mPreview, path);

        mPlay.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                mDrmMediaPlayer.play();
            }
        });
        mPause.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                if (mDrmMediaPlayer != null) {
                    mDrmMediaPlayer.pause();
                }
            }
        });
        mReset.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                if (mDrmMediaPlayer != null) {
                    mDrmMediaPlayer.reset();
                }
            }
        });
        mStop.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                if (mDrmMediaPlayer != null) {
                    mDrmMediaPlayer.stop();
                }
            }
        });
    }

}
