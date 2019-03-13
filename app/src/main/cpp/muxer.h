

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "log_util.h"

typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;

void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);
void add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id);
AVStream *add_video_steam(AVFormatContext *oc, AVCodecContext *c);