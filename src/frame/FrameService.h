#pragma once

#include <string>
#include "VideoFormat.h"

extern "C" {
    #include <libavformat/avformat.h>
}

class FrameService {
    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *dec_ctx;
    AVPacket *pkt;
    AVFrame *frame;
    int video_stream_idx;

    bool tryGetNextPacket();
    bool tryGetNextFrame();
public:
    explicit FrameService(std::string path);
    ~FrameService();
    bool tryGetNext(AVFrame **result);

    /**
     * Seeks to the specified timestamp.
     * @param pts Presentation timestamp to seek to
     * @return true if successful, false otherwise
     */
    bool trySeek(int64_t pts, AVFrame **result);

    VideoFormat getFormat();
};