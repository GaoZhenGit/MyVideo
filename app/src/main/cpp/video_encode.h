//
// Created by Administrator on 2019/3/14.
//

#ifndef MYVIDEO_VIDEO_ENCODE_H
#define MYVIDEO_VIDEO_ENCODE_H

#include <jni.h>
#include <pthread.h>
#include "muxer.h"
#include "data_queue.h"
#include "log_util.h"

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"

AVCodec *video_codec;
AVCodecContext *video_codec_context = NULL;
AVFormatContext *formatContext = NULL;
AVOutputFormat *fmt = NULL;
AVStream *video_stream = NULL;
AVFrame *video_frame;
AVPacket *video_pkt;
FILE *video_ofd = NULL;
int video_pts_i;
int queue_running = 0;
AVDictionary *opt = NULL;
data_queue *video_data_queue = NULL;
pthread_t consume_thread;

#define TIME_BASE_FIELD 15

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_startVideoEncode(
        JNIEnv *env, jobject instance, jint width, jint height, jstring file_name);

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_videoOnFrame(JNIEnv *env, jobject instance, jbyteArray data_);

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_endVideoEncode(JNIEnv *env, jobject instance);

}

void *video_consume(void *p);

//分离yuv中的uv分量，将u分量和v分量放在按照顺序连续排列，而非交叉
static void splitYuv(uint8_t *nv21, uint8_t *nv12);

#endif //MYVIDEO_VIDEO_ENCODE_H
