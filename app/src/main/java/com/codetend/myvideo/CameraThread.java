package com.codetend.myvideo;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.Looper;
import android.util.Log;
import android.view.SurfaceView;

public class CameraThread extends Thread implements Camera.PreviewCallback {

    private Camera mCamera;
    private SurfaceView mSurfaceView;
    private Camera.Size mSize;
    private Looper mLooper;

    public void setSurfaceView(SurfaceView surfaceView) {
        this.mSurfaceView = surfaceView;
    }

    @Override
    public void run() {
        Looper.prepare();
        mLooper = Looper.myLooper();
        init();
        FFmpegManager.getInstance().startEncoder(mSize.width, mSize.height);
        mCamera.startPreview();
        Looper.loop();
    }

    private void init() {
        try {
            mCamera = Camera.open(0);
            Camera.Parameters parameters = mCamera.getParameters();
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
            parameters.setPreviewFormat(ImageFormat.NV21);
            parameters.setAutoWhiteBalanceLock(true);
            mSize = parameters.getSupportedPreviewSizes().get(0);
            parameters.setPreviewSize(mSize.width, mSize.height);
            mCamera.setParameters(parameters);
            mCamera.setDisplayOrientation(90);
            mCamera.setPreviewCallbackWithBuffer(this);
            byte[] data = new byte[mSize.width * mSize.height * ImageFormat.getBitsPerPixel(ImageFormat.NV21) / 8];
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
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        Log.i("CameraThread", "onPreviewFrame");
        camera.addCallbackBuffer(data);
    }

    private void swapNV21ToNV12(byte[] nv21, byte[] nv12, int width, int height) {
        if (nv21 == null || nv12 == null) return;
        int framesize = width * height;
        int i = 0, j = 0;
        System.arraycopy(nv21, 0, nv12, 0, framesize);
        for (j = 0; j < framesize / 2; j += 2) {
            nv12[framesize + j + 1] = nv21[j + framesize];
        }

        for (j = 0; j < framesize / 2; j += 2) {
            nv12[framesize + j] = nv21[j + framesize + 1];
        }
    }
}
