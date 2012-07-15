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
import android.media.MediaPlayer.OnBufferingUpdateListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.webkit.URLUtil;
import android.widget.SeekBar;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;

/**
 * The test application media player
 */
public class DrmMediaPlayer  implements OnErrorListener, OnBufferingUpdateListener,
        OnCompletionListener, MediaPlayer.OnPreparedListener, SurfaceHolder.Callback {
    private SeekBar mSeekMedia;
    private SurfaceView mPreview;
    private SurfaceHolder mHolder;
    private String mCurrent;
    private MediaPlayer mMediaPlayer;
    private ProgressBarHandler mProgressBarHandler;
    private boolean mIsPrepared = false;

    public DrmMediaPlayer(SeekBar seekMedia, SurfaceView preview, String path) {
        mCurrent = path;
        mSeekMedia = seekMedia;
        mPreview = preview;
        mHolder = mPreview.getHolder();
        mHolder.addCallback(this);
        mHolder.setKeepScreenOn(true);
        mHolder.setSizeFromLayout();
    }

    /**
     * Start rendering of a content.
     */
    public void play() {
        try {
            if (mIsPrepared && mMediaPlayer != null) {
                mMediaPlayer.start();
                return;
            }
            mMediaPlayer = new MediaPlayer();
            mMediaPlayer.setOnErrorListener(this);
            mMediaPlayer.setOnBufferingUpdateListener(this);
            mMediaPlayer.setOnCompletionListener(this);
            mMediaPlayer.setOnPreparedListener(this);
            mMediaPlayer.setAudioStreamType(2);
            mProgressBarHandler = new ProgressBarHandler(mMediaPlayer,mSeekMedia);
            mSeekMedia.setIndeterminate(false);
            mSeekMedia.setOnSeekBarChangeListener(mProgressBarHandler);

            Runnable r = new Runnable() {
                public void run() {
                    try {
                        setDataSource(mCurrent);
                        mMediaPlayer.setDisplay(mHolder);
                        mPreview.bringToFront();
                        mMediaPlayer.prepare();
                        mMediaPlayer.setScreenOnWhilePlaying(true);
                        mHolder.setKeepScreenOn(true);
                        Log.v(Constants.TAG, "Duration:  ===>" + mMediaPlayer.getDuration());
                        Log.v(Constants.TAG, "MP STARTED");
                    } catch (IOException e) {
                        Log.e(Constants.TAG, e.getMessage(), e);
                    }
                    mProgressBarHandler.start();
                }
            };
            new Thread(r).start();
        } catch (Exception e) {
            Log.e(Constants.TAG, "error: " + e.getMessage(), e);
            if (mMediaPlayer != null) {
                mMediaPlayer.stop();
                mMediaPlayer.release();
            }
        }
    }

    /**
     * Pause rendering of a content.
     */
    public void pause() {
        if (mMediaPlayer != null) {
            if (mMediaPlayer.isPlaying()) {
                try {
                    mMediaPlayer.pause();
                } catch (Exception e) {
                    Log.e(Constants.TAG, "error: " + e.getMessage(), e);
                }
            }
        }
    }

    /**
     * Stop rendering of a content.
     */
    public void stop() {
        if (mIsPrepared) {
            if (mMediaPlayer != null) {
                mSeekMedia.setProgress(0);
                try {
                    mMediaPlayer.stop();
                } catch (Exception e) {
                    Log.e(Constants.TAG, "error: " + e.getMessage(), e);
                } finally {
                    mIsPrepared = false;
                    mMediaPlayer.release();
                    mMediaPlayer = null;
                }
            }
        }
    }

    /**
     * Resets and rewind to beginning.
     */
    public void reset() {
        if (mIsPrepared) {
            if (mMediaPlayer != null) {
                try {
                    mMediaPlayer.seekTo(0);
                } catch (Exception e) {
                    Log.e(Constants.TAG, "error: " + e.getMessage(), e);
                }
            }
        }
    }

    private void setDataSource(String path) {
        if (URLUtil.isNetworkUrl(path)) {
            path = downloadContent(path);
        }
        if ((path != null) && (mMediaPlayer != null)) {
            try {
                mMediaPlayer.setDataSource(path);
            } catch (IOException e) {
                Log.e(Constants.TAG, "Error reading data while setting data source.");
            } catch (IllegalArgumentException e2) {
                Log.e(Constants.TAG, "Illegal path.");
            } catch (IllegalStateException e3) {
                Log.e(Constants.TAG, "setDataSource called in the wrong state.");
            }
        } else {
            Log.e(Constants.TAG, "setDataSource called with null as in parameter.");
        }
    }

    private String downloadContent(String path) {
        String result = null;
        FileOutputStream out = null;
        InputStream in = null;
        try {
            URL url = new URL(path);
            URLConnection cn = url.openConnection();
            cn.connect();
            in = cn.getInputStream();
            if (in != null) {
                File tempFile = File.createTempFile("mediaplayertmp", "dat");
                String tempPath = tempFile.getAbsolutePath();
                out = new FileOutputStream(tempFile);
                byte buf[] = new byte[128];
                do {
                    int numread = in.read(buf);
                    if (numread <= 0)
                        break;
                    out.write(buf, 0, numread);
                } while (true);
                result = tempPath;
            }
        } catch (IOException ex) {
            Log.e(Constants.TAG, "Error reading data while setting data source.");
        } finally {
            if (out != null) {
                try {
                    out.close();
                } catch (IOException e) {}
            }
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {}
            }
        }
        return result;
    }

    public boolean onError(MediaPlayer mediaPlayer, int what, int extra) {
        Log.e(Constants.TAG, "onError--->   what:" + what + "    extra:" + extra);
        if (mediaPlayer != null) {
            if (mIsPrepared) {
                try {
                    mediaPlayer.stop();
                } catch (Exception e) {
                    Log.e(Constants.TAG, "error: " + e.getMessage(), e);
                }
            }
            mediaPlayer.release();
        }
        return true;
    }

    public void onPrepared(MediaPlayer mediaplayer) {
        mIsPrepared = true;
        try {
            mMediaPlayer.start();
        } catch (Exception e) {
            Log.e(Constants.TAG, "error: " + e.getMessage(), e);
        }
        mSeekMedia.setMax(mMediaPlayer.getDuration());
    }

    public void surfaceCreated(SurfaceHolder surfaceholder) {
        play();
    }

    public void onBufferingUpdate(MediaPlayer arg0, int percent) {}

    public void onCompletion(MediaPlayer arg0) {}

    public void surfaceChanged(SurfaceHolder surfaceholder, int i, int j, int k) {}

    public void surfaceDestroyed(SurfaceHolder surfaceholder) {}
}
