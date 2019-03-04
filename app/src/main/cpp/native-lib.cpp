#include <jni.h>
#include <string>
#include <android/log.h>

#define TAG "ffnative" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>

const AVCodec *codec;
AVCodecContext *codecContext = NULL;
AVFrame *frame;
AVPacket *pkt;
FILE *ofd = NULL;
int pts_i;

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_startEncoder(
        JNIEnv *env, jobject instance, jint width, jint height, jstring file_name) {
    pts_i = 0;

    codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    if (!codec) {
        LOGE("encoder not found");
        return -1;
    }
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        LOGE("codecContext not found");
        return -1;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        LOGE("pkt alloc fail");
        return -1;
    }

    /* put sample parameters */
    codecContext->bit_rate = 100000;
    /* resolution must be a multiple of two */
    codecContext->width = width;
    codecContext->height = height;
    /* frames per second */
    codecContext->time_base = (AVRational) {1, 12};
    codecContext->framerate = (AVRational) {12, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    codecContext->gop_size = 10;
    codecContext->max_b_frames = 1;
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(codecContext->priv_data, "preset", "slow", 0);
    }
    av_opt_set(codecContext->priv_data, "preset", "superfast", 0);
    LOGI("codec id: %d, width: %d, height :%d", codec->id, codecContext->width, codecContext->height);

    /* open it */
    int ret = avcodec_open2(codecContext, codec, NULL);
    if (ret < 0) {
        LOGE("Could not open codec:%d,%s", ret, av_err2str(ret));
        return -1;
    }
    LOGI("encoder ready!");
    const char *output_file_name = env->GetStringUTFChars(file_name, 0);
    ofd = fopen(output_file_name, "wb");
    env->ReleaseStringUTFChars(file_name, output_file_name);
    if (!ofd) {
        LOGE("file open fail");
        return -1;
    } else {
        LOGI("file open success");
    }

    return 1;
}

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *outfile) {
    int ret;
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        LOGE("Error sending a frame for encoding\n");
        return;
    }

    av_init_packet(pkt);
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            if (ret == AVERROR(EAGAIN)) {
                LOGI("pkt receive fail again:%d", ret);
            } else {
                LOGI("pkt receive eof");
            }
            //不需要释放pkt？？
            return;
        } else if (ret < 0) {
            LOGE("Error during encoding\n");
        } else {
            LOGI("pkt write");
            fwrite(pkt->data, 1, pkt->size, outfile);
        }
        av_packet_unref(pkt);
    }
}

long getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_onFrame(JNIEnv *env, jobject instance, jbyteArray data_) {
    long start = getCurrentTime();

    jbyte *dataI = env->GetByteArrayElements(data_, NULL);
    uint8_t *data = (uint8_t *) dataI;
    LOGI("=====================on frame start=====================");
    int ret;
    frame = av_frame_alloc();
    if (!frame) {
        LOGE("Could not allocate video frame\n");
        return -1;
    }
    frame->format = codecContext->pix_fmt;
    frame->width = codecContext->width;
    frame->height = codecContext->height;
    frame->sample_aspect_ratio = (AVRational) {0, 1};
    frame->linesize[0] = codecContext->width;
    frame->linesize[1] = codecContext->width / 2;
    frame->linesize[2] = codecContext->width / 2;
    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0) {
        LOGE("av_frame_get_buffer:%d", ret);
    }
    ret = av_frame_make_writable(frame);
    if (ret < 0) {
        LOGE("frame writeable fail");
    }

    int y_size = codecContext->width * codecContext->height;
    frame->data[0] = data; //PCM Data
    frame->data[1] = data + y_size * 5 / 4; // V
    frame->data[2] = data + y_size; // U
    frame->pts = pts_i;
    pts_i++;

    /* encode the image */
    encode(codecContext, frame, pkt, ofd);
    long spend = getCurrentTime() - start;
    LOGI("encode start:%d,spend:%d", start, spend);
    LOGI("=====================frame finish %d =====================", pts_i);
    env->ReleaseByteArrayElements(data_, dataI, 0);
    return 1;
}


JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_endEncode(JNIEnv *env, jobject instance) {
/* flush the encoder */
    encode(codecContext, NULL, pkt, ofd);

    uint8_t endcode[] = {0, 0, 1, 0xb7};
    /* add sequence end code to have a real MPEG file */
    fwrite(endcode, 1, sizeof(endcode), ofd);
    fclose(ofd);

    avcodec_free_context(&codecContext);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    LOGI("end encode!");
    return 1;
}

}