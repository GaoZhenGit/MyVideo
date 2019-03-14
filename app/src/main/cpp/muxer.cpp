//
// Created by Administrator on 2019/3/6.
//
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
