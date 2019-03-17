
#ifndef MYVIDEO_MUXER_H
#define MYVIDEO_MUXER_H

#include "log_util.h"

extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
}

void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);

int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);

AVStream *add_video_steam(AVFormatContext *oc, AVCodecContext *c);

AVStream *add_audio_steam(AVFormatContext *oc, AVCodecContext *c);

#endif //MYVIDEO_MUXER_H