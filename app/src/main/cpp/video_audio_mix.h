//
// Created by Administrator on 2019/3/16.
//

#ifndef MYVIDEO_VIDEO_AUDIO_MIX_H
#define MYVIDEO_VIDEO_AUDIO_MIX_H

#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include "log_util.h"
#include "data_queue.h"
#include "muxer.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_setAllOption(
        JNIEnv *env, jobject instance, jstring key, jstring value);

JNIEXPORT jstring JNICALL
Java_com_codetend_myvideo_FFmpegManager_getAllOption(
        JNIEnv *env, jobject instance, jstring key);

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_startAllEncode(
        JNIEnv *env, jobject instance);
JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_allOnFrame(
        JNIEnv *env, jobject instance, jbyteArray data_, jboolean isVideo);

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_endAllEncode(JNIEnv *env, jobject instance);

}

namespace mix {
    AVCodec *audio_codec = NULL;
    AVCodec *video_codec = NULL;

    AVCodecContext *audio_codec_context = NULL;
    AVCodecContext *video_codec_context = NULL;

    AVFrame *audio_frame = NULL;
    AVFrame *video_frame = NULL;

    AVPacket *audio_pkt = NULL;
    AVPacket *video_pkt = NULL;

    data_queue *audio_data_queue = NULL;
    data_queue *video_data_queue = NULL;

    bool audio_queue_running = 1;
    bool video_queue_running = 1;

    AVStream *audio_stream = NULL;
    AVStream *video_stream = NULL;

    AVFormatContext *format_context = NULL;
    AVOutputFormat *fmt = NULL;

    uint32_t audio_pts_i = 0;
    uint32_t video_pts_i = 0;

    int width = 0;
    int height = 0;
    // 每次调用音频的数据大小
    int audio_data_size = 0;
    // 内部能处理的每帧音频数据大小
    int audio_frame_size = 0;

    int fps = 30;

    AVDictionary *opt = NULL;

    char *output_path = NULL;
    //音频转换器
    SwrContext *swr;

    pthread_t *audio_thread;
    pthread_t *video_thread;

    int set_audio_code_context_param();
    int set_video_code_context_param();
    int prepare_audio_swr();
    void *audio_consume(void *);
    void *video_consume(void *);
    void audio_encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output);
    void video_encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile);
    void splitYuv(uint8_t *nv21, uint8_t *nv12);
}
#endif //MYVIDEO_VIDEO_AUDIO_MIX_H
