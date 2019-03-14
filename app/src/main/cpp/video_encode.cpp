#include "video_encode.h"

jint Java_com_codetend_myvideo_FFmpegManager_startEncoder(
        JNIEnv *env, jobject instance, jint width, jint height, jstring file_name) {
    video_pts_i = 0;
    const char *output_file_name = env->GetStringUTFChars(file_name, 0);

    //为读写文件生成formatContext
    avformat_alloc_output_context2(&formatContext, NULL, NULL, output_file_name);
    if (!formatContext) {
        LOGE("Could not deduce output format from file extension %s: using MPEG.\n",
             avcodec_get_name(video_codec_context->codec_id));
        avformat_alloc_output_context2(&formatContext, NULL, "mpeg", output_file_name);
    }
    if (!formatContext) {
        LOGE("Could not deduce output format from file extension MPEG fail \n");
        return -1;
    }
    av_dict_set_int(&opt, "video_track_timescale", TIME_BASE_FIELD, 0);
    fmt = formatContext->oformat;
    //如果文件后缀的codec找到了，再开始初始化
    video_codec = avcodec_find_encoder(fmt->video_codec);
    if (!video_codec) {
        LOGE("encoder:%d, %s not found", fmt->video_codec, avcodec_get_name(fmt->video_codec));
        return -1;
    } else {
        LOGI("encoder:%d, %s used!", fmt->video_codec, avcodec_get_name(fmt->video_codec));
    }
    video_codec_context = avcodec_alloc_context3(video_codec);
    if (!video_codec_context) {
        LOGE("codecContext not found");
        return -1;
    }

    video_pkt = av_packet_alloc();
    if (!video_pkt) {
        LOGE("pkt alloc fail");
        return -1;
    }

    /* put sample parameters */
    video_codec_context->bit_rate = 400000;
    /* resolution must be a multiple of two */
    video_codec_context->width = width;
    video_codec_context->height = height;
    /* frames per second */
    video_codec_context->time_base = (AVRational) {1, TIME_BASE_FIELD};
    video_codec_context->framerate = (AVRational) {TIME_BASE_FIELD, 1};

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
        video_stream = add_video_steam(formatContext, video_codec_context);
        LOGI("add steam success:%s", avcodec_get_name(fmt->video_codec));
    }
    av_dump_format(formatContext, 0, output_file_name, 1);
    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&formatContext->pb, output_file_name, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open '%s': %s\n", output_file_name, av_err2str(ret));
            avcodec_free_context(&video_codec_context);
            video_codec_context = NULL;
            return -1;
        }
        LOGI("avio_open success");
    }
    ret = avformat_write_header(formatContext, &opt);
    LOGI("write header success");
    if (ret < 0) {
        LOGE("Error occurred when opening output file:%d, %s\n", ret, av_err2str(ret));
        return -1;
    }

    env->ReleaseStringUTFChars(file_name, output_file_name);

    video_data_queue = create_data_queue();

    pthread_create(&consume_thread, NULL, consume, 0);
    return 1;
}

jint Java_com_codetend_myvideo_FFmpegManager_onFrame(JNIEnv *env, jobject instance, jbyteArray data_) {
    if (!video_codec_context) {
        return -1;
    }
    jbyte *dataI = env->GetByteArrayElements(data_, NULL);
    uint8_t *data = (uint8_t *) dataI;
    LOGI("=====================on frame start=====================");
    int data_size = video_codec_context->width * video_codec_context->height * 3 / 2;
    video_data_queue->set_copy_fun(splitYuv);
    video_data_queue->add(data, data_size);
    LOGI("=====================frame finish %d =====================", video_pts_i);
    env->ReleaseByteArrayElements(data_, dataI, 0);
    return 1;
}

jint Java_com_codetend_myvideo_FFmpegManager_endEncode(JNIEnv *env, jobject instance) {
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
    pthread_join(consume_thread, &tret);
    return 1;
}

static void video_encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
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
                ret = write_frame(formatContext, &video_codec_context->time_base, video_stream, pkt);
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
            video_encode(video_codec_context, video_frame, video_pkt, video_ofd);
            video_data_queue->free(node);
            LOGI("consume end:%d, pts:%d", video_pts_i, video_frame->pts);
        }
    }
    /* flush the encoder */
    video_encode(video_codec_context, NULL, video_pkt, video_ofd);
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

    avcodec_free_context(&video_codec_context);
    video_codec_context = NULL;
    av_frame_free(&video_frame);
    av_packet_free(&video_pkt);
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
    int framesize = video_codec_context->width * video_codec_context->height;
    memcpy(nv12, nv21, framesize);
    for (int i = 0; i < framesize / 4; i++) {
        nv12[i + framesize] = nv21[2 * i + framesize];
        nv12[i + framesize + framesize / 4] = nv21[2 * i + framesize + 1];
    }
}