package com.codetend.myvideo.mix;

import android.hardware.Camera;
import android.view.SurfaceView;

import com.codetend.myvideo.FFmpegManager;

public class MixHelper {
    private MixAudioThread mixAudioThread;
    private MixVideoThread mixVideoThread;
    private SurfaceView mSurfaceView;
    private Camera.Size mSize;

    public void setSurfaceView(SurfaceView surfaceView) {
        this.mSurfaceView = surfaceView;
    }

    public void setPreviewSize(Camera.Size size) {
        mSize = size;
    }

    public void start() {
        if (mixVideoThread == null || mixAudioThread == null) {
            init();
        } else {
            finish();
            init();
        }
        FFmpegManager.getInstance().setAllOption("output", "sdcard/mix.avi");
        if(FFmpegManager.getInstance().startAllEncode() > 0) {
            mixVideoThread.start();
            mixAudioThread.start();
        }
    }

    public void finish() {
        if (mixVideoThread != null && mixAudioThread != null) {
            mixVideoThread.finish();
            mixAudioThread.finish();

            mixAudioThread = null;
            mixVideoThread = null;

            FFmpegManager.getInstance().endAllEncode();
        }
    }

    private void init() {
        mixAudioThread = new MixAudioThread();
        mixVideoThread = new MixVideoThread();
        mixVideoThread.setFps(30);
        mixVideoThread.setPreviewSize(mSize);
        mixVideoThread.setSurfaceView(mSurfaceView);
    }
}
