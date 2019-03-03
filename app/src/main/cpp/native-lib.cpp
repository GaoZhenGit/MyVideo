#include <jni.h>
#include <string>
#include <android/log.h>

#define TAG "FFNative" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型

extern "C" {
#include <libavcodec/avcodec.h>

const AVCodec *codec;
AVCodecContext *codecContext = NULL;

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_startEncoder(
        JNIEnv *env, jobject instance, jint width, jint height) {
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        LOGI("encoder not found");
        return -1;
    }
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        LOGI("codecContext not found");
        return -1;
    }
    return 1;
}

}