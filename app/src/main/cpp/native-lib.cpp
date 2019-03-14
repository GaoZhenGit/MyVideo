#include <jni.h>
#include <string>
#include <android/log.h>
#include <pthread.h>
#include "data_queue.h"
#include "log_util.h"


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <unistd.h>
#include "muxer.h"

#define TIME_BASE_FIELD 15
long getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

AVCodec *codec;
AVCodecContext *codecContext = NULL;
AVFormatContext *formatContext = NULL;
AVOutputFormat *fmt = NULL;
AVStream *video_stream = NULL;
AVFrame *frame;
AVPacket *pkt;
FILE *ofd = NULL;
int pts_i;
int64_t base_time;
int queue_running = 0;
AVDictionary *opt = NULL;
data_queue *video_data_queue = NULL;
pthread_t comsume_thread;

void *consume(void *p);

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_startEncoder(
        JNIEnv *env, jobject instance, jint width, jint height, jstring file_name) {
    pts_i = 0;
    base_time = 0;
    const char *output_file_name = env->GetStringUTFChars(file_name, 0);

    //为读写文件生成formatContext
    avformat_alloc_output_context2(&formatContext, NULL, NULL, output_file_name);
    if (!formatContext) {
        LOGE("Could not deduce output format from file extension %s: using MPEG.\n", avcodec_get_name(codecContext->codec_id));
        avformat_alloc_output_context2(&formatContext, NULL, "mpeg", output_file_name);
    }
    if (!formatContext) {
        LOGE("Could not deduce output format from file extension MPEG fail \n");
        return -1;
    }
    av_dict_set_int(&opt, "video_track_timescale", TIME_BASE_FIELD, 0);
    fmt = formatContext->oformat;
    //如果文件后缀的codec找到了，再开始初始化
    codec = avcodec_find_encoder(fmt->video_codec);
    if (!codec) {
        LOGE("encoder:%d, %s not found", fmt->video_codec, avcodec_get_name(fmt->video_codec));
        return -1;
    } else {
        LOGI("encoder:%d, %s used!", fmt->video_codec, avcodec_get_name(fmt->video_codec));
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
    codecContext->bit_rate = 400000;
    /* resolution must be a multiple of two */
    codecContext->width = width;
    codecContext->height = height;
    /* frames per second */
    codecContext->time_base = (AVRational) {1, TIME_BASE_FIELD};
    codecContext->framerate = (AVRational) {TIME_BASE_FIELD, 1};

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
    av_opt_set(codecContext->priv_data, "tune", "zerolatency", 0);
    LOGI("codec id: %d, width: %d, height :%d", codec->id, codecContext->width,
         codecContext->height);

    /* open it */
    int ret = avcodec_open2(codecContext, codec, &opt);
    if (ret < 0) {
        LOGE("Could not open codec:%d,%s", ret, av_err2str(ret));
        return -1;
    }
    LOGI("encoder ready!");
    // 打开读写文件
//    ofd = fopen(output_file_name, "wb");
//    if (!ofd) {
//        LOGE("file open fail");
//        return -1;
//    } else {
//        LOGI("file open success");
//    }
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        video_stream = add_video_steam(formatContext, codecContext);
        LOGI("add steam success:%s", avcodec_get_name(fmt->video_codec));
    }
    av_dump_format(formatContext, 0, output_file_name, 1);
    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&formatContext->pb, output_file_name, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open '%s': %s\n", output_file_name, av_err2str(ret));
            avcodec_free_context(&codecContext);
            codecContext = NULL;
            return -1;
        }
        LOGI("avio_open success");
    }
    ret = avformat_write_header(formatContext, &opt);
    LOGI("write header success");
    if (ret < 0) {
        LOGE("Error occurred when opening output file:%d, %s\n", ret,av_err2str(ret));
        return -1;
    }

    env->ReleaseStringUTFChars(file_name, output_file_name);

    video_data_queue = create_data_queue();

    pthread_create(&comsume_thread, NULL, consume, 0);
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
            LOGI("pkt write:%d", pkt->pts);
//            fwrite(pkt->data, 1, pkt->size, outfile);
            if (frame) {
                ret = write_frame(formatContext, &codecContext->time_base, video_stream, pkt);
                if (ret < 0) {
                    LOGE("write frame fail:%d,%s", ret, av_err2str(ret));
                }
            }
        }
        av_packet_unref(pkt);
    }
}

void *consume(void *) {
    queue_running = 1;
    int left = 1;
    while (queue_running || left) {
        data_node *node = video_data_queue->pop();
        left = node != NULL;
        if (node) {
            LOGI("consume start");
            long start = getCurrentTime();
            int ret;
            frame = av_frame_alloc();
            if (!frame) {
                LOGE("Could not allocate video frame\n");
                continue;
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
                LOGE("frame writeable fail:%d", av_err2str(ret));
            }

            int y_size = codecContext->width * codecContext->height;
            frame->data[0] = node->data; //PCM Data
            frame->data[1] = node->data + y_size * 5 / 4; // V
            frame->data[2] = node->data + y_size; // U
            frame->pts = pts_i;
            pts_i++;
            encode(codecContext, frame, pkt, ofd);
            video_data_queue->free(node);
            long end = getCurrentTime() - start;
            LOGI("consume end:%d, pts:%d, spend:%d", pts_i, frame->pts, start);
        }
    }
    /* flush the encoder */
    encode(codecContext, NULL, pkt, ofd);
    //写入文件尾部
    av_write_trailer(formatContext);
    LOGI("write trailer success");
    if (!(fmt->flags & AVFMT_NOFILE)) {
        /* Close the output file. */
        avio_closep(&formatContext->pb);
        LOGI("avio_closep success");
    }

//    uint8_t endcode[] = {0, 0, 1, 0xb7};
    /* add sequence end code to have a real MPEG file */
//    fwrite(endcode, 1, sizeof(endcode), ofd);
//    fclose(ofd);

    avcodec_free_context(&codecContext);
    codecContext = NULL;
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avformat_free_context(formatContext);
    formatContext = NULL;
    fmt = NULL;
    av_dict_free(&opt);
    opt = NULL;
    free_data_queue(video_data_queue);
    LOGI("end encode!");
    return NULL;
}
//分离yuv中的uv分量，将u分量和v分量放在按照顺序连续排列，而非交叉
static void splitYuv(uint8_t *nv21, uint8_t *nv12) {
    int framesize = codecContext->width * codecContext->height;
    memcpy(nv12, nv21, framesize);
    for (int i = 0; i < framesize / 4; i++) {
        nv12[i + framesize] = nv21[2 * i + framesize];
        nv12[i + framesize + framesize / 4] = nv21[2 * i + framesize + 1];
    }
}

JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_onFrame(JNIEnv *env, jobject instance, jbyteArray data_) {
    if (!codecContext) {
        return -1;
    }
    jbyte *dataI = env->GetByteArrayElements(data_, NULL);
    uint8_t *data = (uint8_t *) dataI;
    LOGI("=====================on frame start=====================");
    int data_size = codecContext->width * codecContext->height * 3 / 2;
    video_data_queue->set_copy_fun(splitYuv);
    video_data_queue->add(data, data_size);
    LOGI("=====================frame finish %d =====================", pts_i);
    env->ReleaseByteArrayElements(data_, dataI, 0);
    return 1;
}


JNIEXPORT jint JNICALL
Java_com_codetend_myvideo_FFmpegManager_endEncode(JNIEnv *env, jobject instance) {
    //stop the thread
    queue_running = 0;
//    /* flush the encoder */
//    encode(codecContext, NULL, pkt, ofd);
//
//    uint8_t endcode[] = {0, 0, 1, 0xb7};
//    /* add sequence end code to have a real MPEG file */
//    fwrite(endcode, 1, sizeof(endcode), ofd);
//    fclose(ofd);
//
//    avcodec_free_context(&codecContext);
//    av_frame_free(&frame);
//    av_packet_free(&pkt);
//    LOGI("end encode!");
    void *tret;
    pthread_join(comsume_thread, &tret);
    pthreadfr
    return 1;
}

}