#include <unistd.h>
#include "audio_encode.h"

static int check_sample_fmt(const AVCodec *codec) {
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        LOGI("support:%s", av_get_sample_fmt_name(*p));
        p++;
    }
    return 0;
}

jint Java_com_codetend_myvideo_FFmpegManager_startAudioEncode(JNIEnv *env, jobject instance,
                                                              jint data_len, jstring _file_name) {
    const char *file_name = env->GetStringUTFChars(_file_name, 0);

    //为读写文件生成formatContext
    avformat_alloc_output_context2(&audio_format_context, NULL, NULL, file_name);
    if (!audio_format_context) {
        LOGE("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&audio_format_context, NULL, "mpeg", file_name);
    }
    if (!audio_format_context) {
        LOGE("Could not deduce output format from file extension MPEG fail \n");
        return -1;
    }
    audio_fmt = audio_format_context->oformat;

    audio_codec = avcodec_find_encoder(audio_fmt->audio_codec);
    if (!audio_codec) {
        LOGE("Codec not found\n");
        return -1;
    } else {
        LOGI("codec %s found", avcodec_get_name(audio_codec->id));
    }
    audio_codec_context = avcodec_alloc_context3(audio_codec);
    if (!audio_codec_context) {
        LOGE("Could not allocate audio codec context\n");
        return -2;
    }
    /* put sample parameters */
    audio_codec_context->bit_rate = 96000;

    /* check that the encoder supports s16 pcm input */
    audio_codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
//    if (!check_sample_fmt(codec, c->sample_fmt)) {
//        fprintf(stderr, "Encoder does not support sample format %s",
//                av_get_sample_fmt_name(c->sample_fmt));
//        exit(1);
//    }
    audio_codec_context->sample_rate = 44100;
    audio_codec_context->channel_layout = AV_CH_LAYOUT_STEREO;
    audio_codec_context->channels = av_get_channel_layout_nb_channels(
            audio_codec_context->channel_layout);

    /* open it */
    int ret = avcodec_open2(audio_codec_context, audio_codec, NULL);
    if (ret < 0) {
        LOGE("Could not open codec:%d,%s", ret, av_err2str(ret));
        return -3;
    }

    /* packet for holding encoded output */
    audio_pkt = av_packet_alloc();
    if (!audio_pkt) {
        LOGE("could not allocate the packet\n");
        return -4;
    }

    frame_len = data_len;
//    audio_ofd = fopen(file_name, "wb");
//    if (!audio_ofd) {
//        LOGE("file open fail!");
//        return -7;
//    }
    env->ReleaseStringUTFChars(_file_name, file_name);
    audio_data_queue = create_data_queue();
    audio_pts_i = 0;

    //fmt s16 转 fltp, 因为aac只支持fltp
    swr = swr_alloc();
    av_opt_set_int(swr, "in_channel_layout", audio_codec_context->channel_layout, NULL);
    av_opt_set_int(swr, "out_channel_layout", audio_codec_context->channel_layout, NULL);
    av_opt_set_int(swr, "in_sample_rate", audio_codec_context->sample_rate, NULL);
    av_opt_set_int(swr, "out_sample_rate", audio_codec_context->sample_rate, NULL);
    av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_S16, NULL);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, NULL);
    swr_init(swr);

    if (audio_fmt->audio_codec != AV_CODEC_ID_NONE) {
        audio_stream = add_audio_steam(audio_format_context, audio_codec_context);
        LOGI("add steam success:%s", avcodec_get_name(audio_fmt->audio_codec));
    }
    av_dump_format(audio_format_context, 0, file_name, 1);
    /* open the output file, if needed */
    if (!(audio_fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&audio_format_context->pb, file_name, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open '%s': %s\n", file_name, av_err2str(ret));
            avcodec_free_context(&audio_codec_context);
            audio_codec_context = NULL;
            return -1;
        }
        LOGI("avio_open success");
    }
    ret = avformat_write_header(audio_format_context, NULL);
    LOGI("write header success");
    if (ret < 0) {
        LOGE("Error occurred when opening output file:%d, %s\n", ret, av_err2str(ret));
        return -1;
    }

    pthread_create(&consume_thread, NULL, audio_consume, 0);
    LOGI("audio encode start success");
    return 1;
}

static void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output) {
    int ret = 0;
    LOGI("start audio encode");
/* send the frame for encoding */
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        LOGE("Error sending the frame to the encoder:%d,%s", ret, av_err2str(ret));
        return;
    }
    LOGI("audio encode success");

/* read all the available output packets (in general there may be any
 * number of them */
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        LOGI("avcodec_receive_packet success");
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            LOGE("Error encoding audio frame\n");
            return;
        }
//        fwrite(pkt->data, 1, pkt->size, output);
        ret = write_frame(audio_format_context, &audio_codec_context->time_base, audio_stream, pkt);
        LOGI("write_frame success");
        if (ret < 0) {
            LOGE("write frame fail:%d,%s", ret, av_err2str(ret));
        }
        av_packet_unref(pkt);
    }
}

jint Java_com_codetend_myvideo_FFmpegManager_audioOnFrame(JNIEnv *env, jobject instance,
                                                          jbyteArray data_) {
    LOGI("=====================on audio frame start=====================");
    jbyte *dataI = env->GetByteArrayElements(data_, NULL);
    uint8_t *data = (uint8_t *) dataI;
    audio_data_queue->add(data, frame_len);
    LOGI("=====================audio frame finish =====================");
    env->ReleaseByteArrayElements(data_, dataI, 0);
    return 1;
}

void *audio_consume(void *) {
    audio_queue_running = 1;
    int left = 1;
    usleep(200 * 1000);
    while (audio_queue_running || left) {
        data_node *node = audio_data_queue->pop();
        left = node != NULL;
        if (node) {
            LOGI("audio consume start");
            int ret;
            audio_frame = av_frame_alloc();
            if (!audio_frame) {
                LOGE("Could not allocate audio_frame\n");
                continue;
            }
            audio_frame->nb_samples = audio_codec_context->frame_size;
            audio_frame->format = audio_codec_context->sample_fmt;
            audio_frame->channel_layout = audio_codec_context->channel_layout;
            ret = av_frame_get_buffer(audio_frame, 0);
            if (ret < 0) {
                LOGE("av_frame_get_buffer:%d,%s", ret, av_err2str(ret));
            }
            ret = av_frame_make_writable(audio_frame);
            if (ret < 0) {
                LOGE("frame writeable fail:%d,%s", ret, av_err2str(ret));
            }

            uint8_t *outs[2];
            outs[0] = new uint8_t[frame_len];
            outs[1] = new uint8_t[frame_len];
            ret = swr_convert(swr, (uint8_t **) &outs, frame_len * 4,
                              (const uint8_t **) &node->data, frame_len / 4);
            if (ret < 0) {
                LOGE("Error while converting\n");
                continue;
            }
            audio_frame->data[0] = outs[0];
            audio_frame->data[1] = outs[1];
//            audio_frame->pts = av_rescale_q(audio_pts_i,
//                                            (AVRational) {1, audio_codec_context->sample_rate},
//                                            audio_codec_context->time_base);
//            audio_frame->pts = audio_pts_i * (audio_codec_context->frame_size * 1000 / audio_codec_context->sample_rate);

            encode(audio_codec_context, audio_frame, audio_pkt, audio_ofd);
            audio_data_queue->free(node);

            audio_frame->data[0] = NULL;
            audio_frame->data[1] = NULL;
            delete[] outs[0];
            delete[] outs[1];

            audio_pts_i += audio_frame->nb_samples;
//            audio_pts_i++;
            LOGI("audio_consume end:%d, pts:%d", audio_pts_i, audio_frame->pts);
        }
    }

    encode(audio_codec_context, NULL, audio_pkt, audio_ofd);
    //写入文件尾部
    av_write_trailer(audio_format_context);
    LOGI("write trailer success");
    if (!(audio_fmt->flags & AVFMT_NOFILE)) {
        /* Close the output file. */
        avio_closep(&audio_format_context->pb);
        LOGI("avio_closep success");
    }

    avcodec_free_context(&audio_codec_context);
    audio_codec_context = NULL;
    av_frame_free(&audio_frame);
    av_packet_free(&audio_pkt);
    avformat_free_context(audio_format_context);
    audio_format_context = NULL;
    audio_fmt = NULL;
//    av_dict_free(&opt);
//    opt = NULL;
    free_data_queue(audio_data_queue);
    LOGI("end encode!");

    return NULL;
}

jint Java_com_codetend_myvideo_FFmpegManager_endAudioEncode(JNIEnv *env, jobject instance) {
    audio_queue_running = 0;
    void *tret;
    pthread_join(consume_thread, &tret);
    return 1;
}