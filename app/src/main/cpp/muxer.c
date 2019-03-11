//
// Created by Administrator on 2019/3/6.
//
#ifdef __cplusplus
extern "C" {
#endif

#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

#include <android/log.h>
#include "muxer.h"

void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt) {
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    LOGI("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
         av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
         av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
         av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
         pkt->stream_index);
}

int write_frame(AVFormatContext *fmt_ctx,
                const AVRational *time_base,
                AVStream *st,
                AVPacket *pkt) {
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    log_packet(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

//void add_stream(OutputStream *ost, AVFormatContext *oc,
//                AVCodec **codec,
//                enum AVCodecID codec_id) {
//    AVCodecContext *c;
//    int i;
//
//    /* find the encoder */
//    *codec = avcodec_find_encoder(codec_id);
//    if (!(*codec)) {
//        fprintf(stderr, "Could not find encoder for '%s'\n",
//                avcodec_get_name(codec_id));
//        exit(1);
//    }
//
//    ost->st = avformat_new_stream(oc, NULL);
//    if (!ost->st) {
//        fprintf(stderr, "Could not allocate stream\n");
//        exit(1);
//    }
//    ost->st->id = oc->nb_streams - 1;
//    c = avcodec_alloc_context3(*codec);
//    if (!c) {
//        fprintf(stderr, "Could not alloc an encoding context\n");
//        exit(1);
//    }
//    ost->enc = c;
//
//    switch ((*codec)->type) {
//        case AVMEDIA_TYPE_AUDIO:
//            c->sample_fmt = (*codec)->sample_fmts ?
//                            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
//            c->bit_rate = 64000;
//            c->sample_rate = 44100;
//            if ((*codec)->supported_samplerates) {
//                c->sample_rate = (*codec)->supported_samplerates[0];
//                for (i = 0; (*codec)->supported_samplerates[i]; i++) {
//                    if ((*codec)->supported_samplerates[i] == 44100)
//                        c->sample_rate = 44100;
//                }
//            }
//            c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
//            c->channel_layout = AV_CH_LAYOUT_STEREO;
//            if ((*codec)->channel_layouts) {
//                c->channel_layout = (*codec)->channel_layouts[0];
//                for (i = 0; (*codec)->channel_layouts[i]; i++) {
//                    if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
//                        c->channel_layout = AV_CH_LAYOUT_STEREO;
//                }
//            }
//            c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
//            ost->st->time_base = (AVRational) {1, c->sample_rate};
//            break;
//
//        case AVMEDIA_TYPE_VIDEO:
//            c->codec_id = codec_id;
//
//            c->bit_rate = 400000;
//            /* Resolution must be a multiple of two. */
//            c->width = 352;
//            c->height = 288;
//            /* timebase: This is the fundamental unit of time (in seconds) in terms
//             * of which frame timestamps are represented. For fixed-fps content,
//             * timebase should be 1/framerate and timestamp increments should be
//             * identical to 1. */
//            ost->st->time_base = (AVRational) {1, STREAM_FRAME_RATE};
//            c->time_base = ost->st->time_base;
//
//            c->gop_size = 12; /* emit one intra frame every twelve frames at most */
//            c->pix_fmt = AV_PIX_FMT_YUV420P;
//            if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
//                /* just for testing, we also add B-frames */
//                c->max_b_frames = 2;
//            }
//            if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
//                /* Needed to avoid using macroblocks in which some coeffs overflow.
//                 * This does not happen with normal video, it just happens here as
//                 * the motion of the chroma plane does not match the luma plane. */
//                c->mb_decision = 2;
//            }
//            break;
//
//        default:
//            break;
//    }
//
//    /* Some formats want stream headers to be separate. */
//    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
//        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//}

AVStream *add_video_steam(AVFormatContext *avFormatContext, AVCodecContext *codecContext) {
    AVStream *steam = avformat_new_stream(avFormatContext, NULL);
    if (!steam) {
        LOGE("Could not allocate stream");
        return NULL;
    }
    steam->id = avFormatContext->nb_streams - 1;
    steam->time_base = codecContext->time_base;

    codecContext->gop_size = 12; /* emit one intra frame every twelve frames at most */
    if (avFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    int ret = avcodec_parameters_from_context(steam->codecpar, codecContext);
    if (ret < 0) {
        LOGE("Could not copy the stream parameters\n");
        return NULL;
    }
    return steam;
}


#ifdef __cplusplus
}
#endif
