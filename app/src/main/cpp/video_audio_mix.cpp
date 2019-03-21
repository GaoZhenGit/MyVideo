#include "video_audio_mix.h"

using namespace mix;

jint Java_com_codetend_myvideo_FFmpegManager_setAllOption(
        JNIEnv *env, jobject instance, jstring _key, jstring _value) {
    const char *key = env->GetStringUTFChars(_key, 0);
    const char *value = env->GetStringUTFChars(_value, 0);

    if (!strcmp(key, "width")) {
        mix::width = atoi(value);
        LOGI("width set:%d", mix::width);
    } else if (!strcmp(key, "height")) {
        mix::height = atoi(value);
        LOGI("height set:%d", mix::height);
    } else if (!strcmp(key, "output")) {
        mix::output_path = new char[strlen(value)];
        strcpy(mix::output_path, value);
        LOGI("output set:%s", mix::output_path);
    } else if (!strcmp(key, "audio_data_size")) {
        mix::audio_data_size = atoi(value);
        LOGI("audio_data_size set:%d", mix::audio_data_size);
    } else if (!strcmp(key, "fps")) {
        mix::fps = atoi(value);
        LOGI("fps set:%d", mix::fps);
    } else {
        LOGI("nothing set");
    }

    env->ReleaseStringUTFChars(_key, key);
    env->ReleaseStringUTFChars(_value, value);
    return 1;
}

jstring Java_com_codetend_myvideo_FFmpegManager_getAllOption(
        JNIEnv *env, jobject instance, jstring _key) {
    const char *key = env->GetStringUTFChars(_key, 0);
    jstring result = NULL;
    char tmp[20];
    if (!strcmp(key, "width")) {
        sprintf(tmp, "%d", mix::width);
        const char *ret = tmp;
        result = env->NewStringUTF(ret);
    } else if (!strcmp(key, "height")) {
        sprintf(tmp, "%d", mix::height);
        const char *ret = tmp;
        result = env->NewStringUTF(ret);
    } else if (!strcmp(key, "output")) {
        const char *ret = mix::output_path;
        result = env->NewStringUTF(ret);
    } else if (!strcmp(key, "audio_data_size")) {
        sprintf(tmp, "%d", mix::audio_data_size);
        const char *ret = tmp;
        result = env->NewStringUTF(ret);
    } else if (!strcmp(key, "fps")) {
        sprintf(tmp, "%d", mix::fps);
        const char *ret = tmp;
        result = env->NewStringUTF(ret);
    } if (!strcmp(key, "audio_frame_size")) {
        sprintf(tmp, "%d", mix::audio_frame_size);
        const char *ret = tmp;
        result = env->NewStringUTF(ret);
    } else {
        LOGI("nothing get");
    }
    env->ReleaseStringUTFChars(_key, key);
    return result;
}

jint Java_com_codetend_myvideo_FFmpegManager_startAllEncode(JNIEnv *env, jobject instance) {
    int ret;
    //1.为读写文件生成formatContext
    avformat_alloc_output_context2(&format_context, NULL, NULL, output_path);
    if (!format_context) {
        LOGE("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&format_context, NULL, "mpeg", output_path);
    }
    if (!format_context) {
        LOGE("Could not deduce output format from file extension MPEG fail \n");
        return -1;
    }
    //2.强行变更视频、音频编码格式
    fmt = format_context->oformat;
    //3.查看是否支持相应的音视频编码格式
    audio_codec = avcodec_find_encoder(fmt->audio_codec);
    video_codec = avcodec_find_encoder(fmt->video_codec);
    if (!audio_codec || !video_codec) {
        LOGE("Codec not found\n");
        return -2;
    } else {
        LOGI("codec %s,%s found", avcodec_get_name(audio_codec->id),
             avcodec_get_name(video_codec->id));
    }
    //4.生成codec_context
    audio_codec_context = avcodec_alloc_context3(audio_codec);
    video_codec_context = avcodec_alloc_context3(video_codec);
    if (!audio_codec_context || !video_codec_context) {
        LOGE("Could not allocate mix context\n");
        return -3;
    }
    //5.设置音视频参数
    ret = set_audio_code_context_param();
    if (ret < 0) {
        LOGE("set_audio_code_context_param fail");
        return -4;
    }
    ret = set_video_code_context_param();
    if (ret < 0) {
        LOGE("set_video_code_context_param fail");
        return -5;
    }
    //6.添加音视频流
    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        audio_stream = add_audio_steam(format_context, audio_codec_context);
        LOGI("add steam success:%s, index:%d", avcodec_get_name(fmt->audio_codec), audio_stream->index);
    }
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        video_stream = add_video_steam(format_context, video_codec_context);
        LOGI("add steam success:%s, index:%d", avcodec_get_name(fmt->video_codec), video_stream->index);
    }

    //7.初始化音视频pkt
    audio_pkt = av_packet_alloc();
    if (!audio_pkt) {
        LOGE("could not allocate the audio packet\n");
        return -6;
    }
    video_pkt = av_packet_alloc();
    if (!video_pkt) {
        LOGE("could not allocate the video packet\n");
        return -7;
    }

    //8.写入流信息
    av_dump_format(format_context, audio_stream->id, output_path, 1);
    av_dump_format(format_context, video_stream->id, output_path, 1);
    //9.音频转换器初始化
    ret = prepare_audio_swr();
    if (ret < 0) {
        return -8;
    }
    audio_frame_size = audio_codec_context->frame_size * audio_codec_context->channels * 2;
    //10.打开文件流
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&format_context->pb, output_path, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open '%s': %s\n", output_path, av_err2str(ret));

            avcodec_free_context(&video_codec_context);
            video_codec_context = NULL;

            avcodec_free_context(&audio_codec_context);
            audio_codec_context = NULL;
            LOGE("avio_open fail");
            return -9;
        }
        LOGI("avio_open success");
    }
    //11.写入头部信息
    ret = avformat_write_header(format_context, &opt);
    LOGI("write header success");
    if (ret < 0) {
        LOGE("Error occurred when opening output file:%d, %s\n", ret, av_err2str(ret));
        return -10;
    }

    //12.初始化音视频数据消费队列
    audio_data_queue = create_data_queue();
    video_data_queue = create_data_queue();
    video_data_queue->set_copy_fun(splitYuv);

    //13.启动音视频消费线程
    audio_thread = new pthread_t();
    video_thread = new pthread_t();
    pthread_create(audio_thread, NULL, mix::audio_consume, 0);
    pthread_create(video_thread, NULL, mix::video_consume, 0);


    LOGI("mix start encode success");
    return 1;
}

jint Java_com_codetend_myvideo_FFmpegManager_allOnFrame(
        JNIEnv *env, jobject instance, jbyteArray data_, jboolean isVideo) {
    jbyte *dataI = env->GetByteArrayElements(data_, NULL);
    uint8_t *data = (uint8_t *) dataI;

    if (isVideo) {
        LOGI("=====================on video frame start=====================");
        int data_size = video_codec_context->width * video_codec_context->height * 3 / 2;
        video_data_queue->add(data, data_size);
        LOGI("=====================video frame finish %d =====================", video_pts_i);
    } else {
        LOGI("=====================on audio frame start=====================");
        int count = audio_data_size / audio_frame_size;
        for (int i = 0; i < count; ++i) {
            audio_data_queue->add(&(data[i * audio_frame_size]), audio_frame_size);
        }
        LOGI("=====================audio frame finish =====================");
    }

    env->ReleaseByteArrayElements(data_, dataI, 0);
    return 1;
}

jint Java_com_codetend_myvideo_FFmpegManager_endAllEncode(JNIEnv *env, jobject instance) {
    //同步等待停止双线程
    audio_queue_running = 0;
    video_queue_running = 0;
    void *tret;
    pthread_join(*audio_thread, &tret);
    pthread_join(*video_thread, &tret);

    //写入文件尾部
    av_write_trailer(format_context);
    LOGI("write trailer success");
    if (!(fmt->flags & AVFMT_NOFILE)) {
        /* Close the output file. */
        avio_closep(&format_context->pb);
        LOGI("avio_closep success");
    }

    avcodec_free_context(&audio_codec_context);
    audio_codec_context = NULL;

    avcodec_free_context(&video_codec_context);
    video_codec_context = NULL;

    av_frame_free(&audio_frame);
    av_frame_free(&video_frame);
    av_packet_free(&audio_pkt);
    av_packet_free(&video_pkt);
    avformat_free_context(format_context);
    format_context = NULL;
    fmt = NULL;
    av_dict_free(&opt);
    opt = NULL;

    free_data_queue(audio_data_queue);
    free_data_queue(video_data_queue);

    delete audio_thread;
    audio_thread = NULL;
    delete video_thread;
    video_thread = NULL;

    LOGI("end encode!");
    return 1;
}

int mix::set_audio_code_context_param() {
    audio_codec_context->bit_rate = 96000;
    audio_codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
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
    audio_pts_i = 0;
    return 1;
}

int mix::set_video_code_context_param() {
    /* put sample parameters */
    video_codec_context->bit_rate = 1000000;
    /* resolution must be a multiple of two */
    video_codec_context->width = width;
    video_codec_context->height = height;
    /* frames per second */
    video_codec_context->time_base = (AVRational) {1, fps};
    video_codec_context->framerate = (AVRational) {fps, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    video_codec_context->gop_size = 10;
    video_codec_context->max_b_frames = 1;
    video_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;

    if (video_codec->id == AV_CODEC_ID_H264) {
        av_opt_set(video_codec_context->priv_data, "preset", "slow", 0);
    }
    av_opt_set(video_codec_context->priv_data, "preset", "superfast", 0);
    av_opt_set(video_codec_context->priv_data, "tune", "zerolatency", 0);
    LOGI("codec id: %d, width: %d, height :%d", video_codec->id, video_codec_context->width,
         video_codec_context->height);

    /* open it */
    int ret = avcodec_open2(video_codec_context, video_codec, &opt);
    if (ret < 0) {
        LOGE("Could not open codec:%d,%s", ret, av_err2str(ret));
        return -1;
    }
    video_pts_i = 0;
    return 1;
}

int mix::prepare_audio_swr() {
    //fmt s16 转 fltp, 因为aac只支持fltp
    swr = swr_alloc();
    int ret = 0;
    ret += av_opt_set_int(swr, "in_channel_layout", audio_codec_context->channel_layout, NULL);
    ret += av_opt_set_int(swr, "out_channel_layout", audio_codec_context->channel_layout, NULL);
    ret += av_opt_set_int(swr, "in_sample_rate", audio_codec_context->sample_rate, NULL);
    ret += av_opt_set_int(swr, "out_sample_rate", audio_codec_context->sample_rate, NULL);
    ret += av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_S16, NULL);
    ret += av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, NULL);
    if (ret == 0) {
        LOGI("swr set success");
        swr_init(swr);
        return 1;
    } else {
        LOGE("swr set fail");
        return -1;
    }
}

void *mix::audio_consume(void *) {
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

            //多声道
            uint8_t *outs[2];
            outs[0] = new uint8_t[audio_codec_context->frame_size]{0};
            outs[1] = new uint8_t[audio_codec_context->frame_size]{0};
            // 因为ffmpeg 的frame_size 默认为1024,所以一次采样次数至少为1024次，由于一次采集两个声道，每个声道占两个字节，
            // 采集1024次就是1024*2*2=4096个字节，所以麦克风应该一次送来4096个字节
            // 这里的单位是sample，所以使用frame_size，为1024
            ret = swr_convert(swr, (uint8_t **) &outs, audio_codec_context->frame_size,
                              (const uint8_t **) &node->data, audio_codec_context->frame_size);
            if (ret < 0) {
                LOGE("Error while converting\n");
                continue;
            } else {
                LOGI("swr_convert count:%d", ret);
            }
            audio_frame->data[0] = outs[0];
            audio_frame->data[1] = outs[1];
            audio_frame->pts = av_rescale_q(audio_pts_i,
                                            (AVRational) {1, audio_codec_context->sample_rate},
                                            audio_codec_context->time_base);
//            audio_frame->pts = audio_pts_i * (audio_codec_context->frame_size * 1000 / audio_codec_context->sample_rate);

            audio_encode(audio_codec_context, audio_frame, audio_pkt, NULL);
            audio_data_queue->free(node);

            audio_frame->data[0] = NULL;
            audio_frame->data[1] = NULL;
            delete[] outs[0];
            delete[] outs[1];

            audio_pts_i += audio_frame->nb_samples;
            LOGI("audio_consume end:%d, pts:%d", audio_pts_i, audio_frame->pts);
        }
    }
    audio_encode(audio_codec_context, NULL, audio_pkt, NULL);
    return NULL;
}

void mix::audio_encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output) {
    int ret = 0;
    LOGI("start audio encode");
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        LOGE("Error sending the frame to the encoder:%d,%s", ret, av_err2str(ret));
        return;
    }
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            LOGE("Error encoding audio frame\n");
            return;
        }
        ret = write_frame(format_context, &audio_codec_context->time_base, audio_stream, pkt);
        if (ret < 0) {
            LOGE("write frame fail:%d,%s", ret, av_err2str(ret));
        }
        av_packet_unref(pkt);
    }
}

void *mix::video_consume(void *) {
    video_queue_running = 1;
    int left = 1;
    while (video_queue_running || left) {
        data_node *node = video_data_queue->pop();
        left = node != NULL;
        if (node) {
            LOGI("video consume start");
            int ret;
            video_frame = av_frame_alloc();
            if (!video_frame) {
                LOGE("Could not allocate video frame\n");
                continue;
            }

            video_frame->format = video_codec_context->pix_fmt;
            video_frame->width = video_codec_context->width;
            video_frame->height = video_codec_context->height;
            video_frame->sample_aspect_ratio = (AVRational) {0, 1};
            video_frame->linesize[0] = video_codec_context->width;
            video_frame->linesize[1] = video_codec_context->width / 2;
            video_frame->linesize[2] = video_codec_context->width / 2;
            ret = av_frame_get_buffer(video_frame, 32);
            if (ret < 0) {
                LOGE("av_frame_get_buffer:%d,%s", ret, av_err2str(ret));
            }
            ret = av_frame_make_writable(video_frame);
            if (ret < 0) {
                LOGE("frame writeable fail:%d,%s", ret, av_err2str(ret));
            }

            int y_size = video_codec_context->width * video_codec_context->height;
            video_frame->data[0] = node->data; //PCM Data
            video_frame->data[1] = node->data + y_size * 5 / 4; // V
            video_frame->data[2] = node->data + y_size; // U
            video_frame->pts = video_pts_i;
            video_pts_i++;
            video_encode(video_codec_context, video_frame, video_pkt, NULL);
            video_data_queue->free(node);
            LOGI("consume end:%d, pts:%d", video_pts_i, video_frame->pts);
        }
    }
    /* flush the encoder */
    video_encode(video_codec_context, NULL, video_pkt, NULL);
    LOGI("end encode!");
    return NULL;
}

void mix::video_encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile) {
    int ret;
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        LOGE("Error sending a video frame for encoding\n");
        return;
    }

    av_init_packet(pkt);
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            //不需要释放pkt？？
            return;
        } else if (ret < 0) {
            LOGE("Error during encoding\n");
        } else {
            LOGI("pkt write:%d", pkt->pts);
            if (frame) {
                ret = write_frame(format_context, &video_codec_context->time_base,
                                  video_stream, pkt);
                if (ret < 0) {
                    LOGE("write frame fail:%d,%s", ret, av_err2str(ret));
                }
            }
        }
        av_packet_unref(pkt);
    }
}

//分离yuv中的uv分量，将u分量和v分量放在按照顺序连续排列，而非交叉
void mix::splitYuv(uint8_t *nv21, uint8_t *nv12) {
    int framesize = video_codec_context->width * video_codec_context->height;
    memcpy(nv12, nv21, framesize);
    for (int i = 0; i < framesize / 4; i++) {
        nv12[i + framesize] = nv21[2 * i + framesize];
        nv12[i + framesize + framesize / 4] = nv21[2 * i + framesize + 1];
    }
}