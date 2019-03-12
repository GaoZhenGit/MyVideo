package com.codetend.myvideo.video;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.Looper;
import android.util.Log;
import android.view.SurfaceView;

import com.codetend.myvideo.FFmpegManager;

import java.util.List;

public class CameraThread extends Thread implements Camera.PreviewCallback {

    private Camera mCamera;
    private SurfaceView mSurfaceView;
    private Camera.Size mSize;
    private int mDataSize;
    private Looper mLooper;

    public void setSurfaceView(SurfaceView surfaceView) {
        this.mSurfaceView = surfaceView;
    }

    @Override
    public void run() {
        Looper.prepare();
        mLooper = Looper.myLooper();
        init();
        FFmpegManager.getInstance().start(mSize.width, mSize.height, "/sdcard/demo.avi");
        mCamera.startPreview();
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
    }

    private void init() {
        try {
            mCamera = Camera.open(0);
            Camera.Parameters parameters = mCamera.getParameters();
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
            parameters.setPreviewFormat(ImageFormat.NV21);
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
        FFmpegManager.getInstance().endEncode();
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        Log.i("CameraThread", "onPreviewFrame");
        camera.addCallbackBuffer(data);
        byte[] nv12 = new byte[data.length];
        splitYuv(data, nv12, mSize.width, mSize.height, mDataSize);
        FFmpegManager.getInstance().enqueueFrame(nv12);
    }

    /**
     * 分离yuv中的uv分量，将u分量和v分量放在按照顺序连续排列，而非交叉
     * @param nv21
     * @param nv12
     * @param width
     * @param height
     * @param dataSize
     */
    private void splitYuv(byte[] nv21, byte[] nv12, int width, int height, int dataSize) {
        int framesize = width * height;
        System.arraycopy(nv21, 0, nv12, 0, framesize);
        for (int i = 0; i < framesize / 4; i++) {
            nv12[i + framesize] = nv21[2 * i + framesize];
            nv12[i + framesize + framesize / 4] = nv21[2 * i + framesize + 1];
        }
    }
}
