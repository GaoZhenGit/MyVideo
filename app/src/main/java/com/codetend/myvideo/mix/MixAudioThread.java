package com.codetend.myvideo.mix;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

import com.codetend.myvideo.FFmpegManager;

public class MixAudioThread extends Thread {
    //指定采样率 （MediaRecoder 的采样率通常是8000Hz AAC的通常是44100Hz。 设置采样率为44100，目前为常用的采样率，官方文档表示这个值可以兼容所有的设置）
    private static final int mSampleRateInHz = 44100;
    //指定捕获音频的声道数目。在AudioFormat类中指定用于此的常量
    private static final int mChannelConfig = AudioFormat.CHANNEL_IN_STEREO;
    //指定音频量化位数 ,在AudioFormaat类中指定了以下各种可能的常量。通常我们选择ENCODING_PCM_16BIT和ENCODING_PCM_8BIT PCM代表的是脉冲编码调制，它实际上是原始音频样本。
    //因此可以设置每个样本的分辨率为16位或者8位，16位将占用更多的空间和处理能力,表示的音频也更加接近真实。
    private static final int mAudioFormat = AudioFormat.ENCODING_PCM_16BIT;
    private AudioRecord mAudioRecord = null;
    private boolean isReading = true;
    // native能处理的最小的buffer size，实际需要整数倍，不然音频会速度不对
    private int mSuggestBufferSize;
    // 实际录音的buffer size
    private int mRealBufferSize;
    // 麦克风最小的buffer size
    private int mMinBufferSize;

    public void init() {
        mMinBufferSize = AudioRecord.getMinBufferSize(mSampleRateInHz, mChannelConfig, mAudioFormat);//计算最小缓冲区
        mSuggestBufferSize = Integer.parseInt(FFmpegManager.getInstance().getAllOption("audio_frame_size"));
        if (mSuggestBufferSize > mMinBufferSize) {
            mRealBufferSize = mSuggestBufferSize;
        } else {
            mRealBufferSize = mSuggestBufferSize;
//            while (mRealBufferSize < mMinBufferSize) {
//                mRealBufferSize *= 2;
//            }
        }
        FFmpegManager.getInstance().setAllOption("audio_data_size", String.valueOf(mRealBufferSize));
        mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, mSampleRateInHz, mChannelConfig,
                mAudioFormat, mRealBufferSize);//创建AudioRecorder对象
    }

    @Override
    public void run() {
        init();
        mAudioRecord.startRecording();
        byte[] buffer = new byte[mRealBufferSize];
        while (isReading) {
            mAudioRecord.read(buffer, 0, mRealBufferSize);
            FFmpegManager.getInstance().allOnFrame(buffer, false);
        }
    }

    public void finish() {
        isReading = false;
        if (mAudioRecord != null) {
            mAudioRecord.stop();
            mAudioRecord.release();
        }
    }
}
