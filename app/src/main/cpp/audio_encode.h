//
// Created by Administrator on 2019/3/13.
//

#ifndef MYVIDEO_AUDIO_ENCODE_H
#define MYVIDEO_AUDIO_ENCODE_H

#include <jni.h>
#include <pthread.h>
#include "log_util.h"
#include "data_queue.h"
#include "muxer.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"

AVCodec *audio_codec;
AVCodecContext *audio_codec_context = NULL;
AVFrame *audio_frame = NULL;
AVPacket *audio_pkt = NULL;
int frame_len = 0;
FILE *audio_ofd = NULL;
data_queue *audio_data_queue = NULL;
int audio_queue_running;
uint32_t audio_pts_i;

AVFormatContext* audio_format_context = NULL;
AVOutputFormat* audio_fmt = NULL;
AVStream* audio_stream = NULL;
int frame_data_size;

SwrContext *swr;
static pthread_t consume_thread;

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_startAudioEncode(JNIEnv *, jobject, jint, jstring);

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_audioOnFrame(JNIEnv *env, jobject instance,
                                                     jbyteArray jdata);

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_endAudioEncode(JNIEnv *env, jobject instance);
}
void *audio_consume(void *);
#endif //MYVIDEO_AUDIO_ENCODE_H
