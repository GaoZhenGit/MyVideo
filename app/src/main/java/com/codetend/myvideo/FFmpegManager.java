package com.codetend.myvideo;


import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class FFmpegManager {

    public static FFmpegManager getInstance() {
        return SingleHolder.INSTANCE;
    }

    private FFmpegManager() {
        System.loadLibrary("native-lib");
    }

    private static class SingleHolder {
        private static volatile FFmpegManager INSTANCE = new FFmpegManager();
    }

    public void start(final int width, int height, String fileName) {
        startEncoder(width, height, fileName);
    }

    private native int startEncoder(int width, int height, String fileName);

    public void enqueueFrame(byte[] data) {
        onFrame(data);
    }

    private native int onFrame(byte[] data);

    public native int endEncode();
}
