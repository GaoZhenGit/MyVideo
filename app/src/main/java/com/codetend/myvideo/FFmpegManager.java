package com.codetend.myvideo;

public class FFmpegManager {
    public static FFmpegManager getInstance() {
        return SingleHolder.INSTANCE;
    }

    private FFmpegManager(){
        System.loadLibrary("native-lib");
    }
    private static class SingleHolder {
        private static volatile FFmpegManager INSTANCE = new FFmpegManager();
    }

    public native int startEncoder(int width, int height);
}
