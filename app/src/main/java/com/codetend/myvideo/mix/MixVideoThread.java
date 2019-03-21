package com.codetend.myvideo.mix;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.SurfaceView;

import com.codetend.myvideo.FFmpegManager;

import java.util.List;

public class MixVideoThread extends Thread implements Camera.PreviewCallback {
    private Camera mCamera;
    private SurfaceView mSurfaceView;
    private Camera.Size mSize;
    private int mDataSize;
    private Looper mLooper;

    private int callbackCount = 0;
    private int lastCount = 0;
    private Handler mMainHandler = new Handler(Looper.getMainLooper());
    private Runnable mRateCountLogger = new Runnable() {
        @Override
        public void run() {
            Log.i("CameraThread", String.format("callback rate:%d,current:%d",(callbackCount - lastCount), callbackCount));
            lastCount = callbackCount;
            mMainHandler.postDelayed(this, 1000);
        }
    };
    private int fps;

    public void setSurfaceView(SurfaceView surfaceView) {
        this.mSurfaceView = surfaceView;
    }

    @Override
    public void run() {
        Looper.prepare();
        mLooper = Looper.myLooper();
        init();
        mCamera.startPreview();
        mMainHandler.post(mRateCountLogger);
        Looper.loop();
    }

    public static List<Camera.Size> getSupportPreviewSize() {
        Camera camera = Camera.open(0);
        Camera.Parameters parameters = camera.getParameters();
        List<Camera.Size> sizes = parameters.getSupportedPreviewSizes();
        camera.release();
        return sizes;
    }

    public void setPreviewSize(Camera.Size size) {
        mSize = size;
        FFmpegManager.getInstance().setAllOption("width", String.valueOf(mSize.width));
        FFmpegManager.getInstance().setAllOption("height", String.valueOf(mSize.height));
    }

    public void setFps(int fps) {
        this.fps = fps;
        FFmpegManager.getInstance().setAllOption("fps", String.valueOf(fps));
    }

    private void init() {
        try {
            mCamera = Camera.open(1);
            Camera.Parameters parameters = mCamera.getParameters();
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
            parameters.setPreviewFormat(ImageFormat.NV21);
            parameters.setPreviewFpsRange(fps * 1000, fps * 1000);
            parameters.setAutoWhiteBalanceLock(true);
            if (mSize == null) {
                mSize = parameters.getSupportedPreviewSizes().get(9);
            }
            parameters.setPreviewSize(mSize.width, mSize.height);
            mCamera.setParameters(parameters);
            mCamera.setDisplayOrientation(90);
            mCamera.setPreviewCallbackWithBuffer(this);
            mDataSize = mSize.width * mSize.height * ImageFormat.getBitsPerPixel(ImageFormat.NV21) / 8;
            byte[] data = new byte[mDataSize];
            mCamera.addCallbackBuffer(data);
            mCamera.setPreviewDisplay(mSurfaceView.getHolder());
        } catch (Throwable t) {
            t.printStackTrace();
        }
    }

    public void finish() {
        if (mCamera != null) {
            mCamera.stopPreview();
            mCamera.release();
        }
        if (mLooper != null) {
            mLooper.quit();
        }
        mMainHandler.removeCallbacks(mRateCountLogger);
    }
    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        Log.i("CameraThread", "onPreviewFrame");
        callbackCount++;
        camera.addCallbackBuffer(data);
        FFmpegManager.getInstance().allOnFrame(data, true);
    }
}
