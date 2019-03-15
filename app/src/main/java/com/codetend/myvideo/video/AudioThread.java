package com.codetend.myvideo.video;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.codetend.myvideo.FFmpegManager;

public class AudioThread extends Thread {
    //指定采样率 （MediaRecoder 的采样率通常是8000Hz AAC的通常是44100Hz。 设置采样率为44100，目前为常用的采样率，官方文档表示这个值可以兼容所有的设置）
    private static final int mSampleRateInHz = 44100;
    //指定捕获音频的声道数目。在AudioFormat类中指定用于此的常量
    private static final int mChannelConfig = AudioFormat.CHANNEL_IN_STEREO;
    //指定音频量化位数 ,在AudioFormaat类中指定了以下各种可能的常量。通常我们选择ENCODING_PCM_16BIT和ENCODING_PCM_8BIT PCM代表的是脉冲编码调制，它实际上是原始音频样本。
    //因此可以设置每个样本的分辨率为16位或者8位，16位将占用更多的空间和处理能力,表示的音频也更加接近真实。
    private static final int mAudioFormat = AudioFormat.ENCODING_PCM_16BIT;
    //指定缓冲区大小。调用AudioRecord类的getMinBufferSize方法可以获得。
    private int mBufferSizeInBytes = AudioRecord.getMinBufferSize(mSampleRateInHz, mChannelConfig, mAudioFormat);//计算最小缓冲区
    //创建AudioRecord。AudioRecord类实际上不会保存捕获的音频，因此需要手动创建文件并保存下载。
    private AudioRecord mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, mSampleRateInHz, mChannelConfig,
            mAudioFormat, mBufferSizeInBytes);//创建AudioRecorder对象
    private boolean isReading = true;

    @Override
    public void run() {
        mAudioRecord.startRecording();
        int ret = FFmpegManager.getInstance().startAudioEncode(mBufferSizeInBytes, "/sdcard/demo.mp4");
        if (ret < 0) {
            Log.e("AudioThread", "audio init fail");
            return;
        }
        byte[] buffer = new byte[mBufferSizeInBytes];
        while (isReading) {
            mAudioRecord.read(buffer, 0, mBufferSizeInBytes);
            FFmpegManager.getInstance().audioOnFrame(buffer);
        }
    }

    public void finish() {
        isReading = false;
        if (mAudioRecord != null) {
            mAudioRecord.stop();
            mAudioRecord.release();
            Handler handler = new Handler(Looper.getMainLooper());
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    FFmpegManager.getInstance().endAudioEncode();
                }
            },2000);

        }
    }
}
