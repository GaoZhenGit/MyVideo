package com.codetend.myvideo;


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
        startVideoEncode(width, height, fileName);
    }

    private native int startVideoEncode(int width, int height, String fileName);

    public void enqueueFrame(byte[] data) {
        videoOnFrame(data);
    }

    private native int videoOnFrame(byte[] data);

    public native int endVideoEncode();

    public native int startAudioEncode(int dataLen, String fileName);

    public native int audioOnFrame(byte[] data);

    public native int endAudioEncode();

    public native int setAllOption(String key, String value);

    public native String getAllOption(String key);

    public native int startAllEncode();

    public native int allOnFrame(byte[] data, boolean isVideo);

    public native int endAllEncode();
}
