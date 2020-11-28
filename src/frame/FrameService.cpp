#include "FrameService.h"

#include <sstream>
#include <iostream>

FrameService::FrameService(const std::string path) {
    // Open input file, and allocate format context
    if (avformat_open_input(&fmt_ctx, path.data(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Could not open source file");
    }

    // Retrieve stream information
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        throw std::runtime_error("Could not find stream information");
    }

    AVCodec *codec = nullptr;
    video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (video_stream_idx < 0) {
        throw std::runtime_error("Could not find a video stream");
    }

    // Allocate a codec context for the decoder
    dec_ctx = avcodec_alloc_context3(codec);
    if (!dec_ctx) {
        std::stringstream ss;
        ss << "Failed to allocate the " << av_get_media_type_string(AVMEDIA_TYPE_VIDEO) << " codec context";
        throw std::runtime_error(ss.str());
    }

    // Copy codec parameters from input stream to output codec context
    if (avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[video_stream_idx]->codecpar) < 0) {
        std::stringstream ss;
        ss << "Failed to copy " << av_get_media_type_string(AVMEDIA_TYPE_VIDEO) << " codec parameters to decoder context";
        throw std::runtime_error(ss.str());
    }

    // Init the decoder
    if (avcodec_open2(dec_ctx, codec, nullptr) < 0) {
        std::stringstream ss;
        ss << "Failed to open " << av_get_media_type_string(AVMEDIA_TYPE_VIDEO) << " codec";
        throw std::runtime_error(ss.str());
    }

    // Allocate a packet and frame
    frame = av_frame_alloc();
    if (!frame) {
        throw std::runtime_error("Could not allocate frame");
    }

    pkt = new AVPacket();
    av_init_packet(pkt);
    pkt->data = nullptr;
    pkt->size = 0;

    if (pkt->pos == -1 && !tryGetNextPacket()) {
        throw std::runtime_error("Could not read first packet");
    }
}

FrameService::~FrameService() {
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avformat_close_input(&fmt_ctx);
    avcodec_close(dec_ctx);
}

VideoFormat FrameService::getFormat() {
    VideoFormat format = { .width = dec_ctx->width, .height = dec_ctx->height, .pixelFormat = dec_ctx->pix_fmt };
    return format;
}

bool FrameService::tryGetNextPacket() {
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        // check if the packet belongs to a stream we are interested in, otherwise skip it
        if (pkt->stream_index != video_stream_idx) {
            av_packet_unref(pkt);
            continue;
        }

        if (avcodec_send_packet(dec_ctx, pkt) < 0) {
            std::cerr << "Error decoding packet" << std::endl;
            av_packet_unref(pkt);
            continue;
        }

        return true;
    }
    return false;
}

bool FrameService::tryGetNextFrame() {
    while (true) {
        auto ret = avcodec_receive_frame(dec_ctx, frame);
        switch (ret) {
            case AVERROR_EOF:
                // End of the packet, no frame.
                return false;
            case AVERROR(EAGAIN):
                // Try again...
                return false;
            default:
                // Skip the rest of this packet on error
                return ret >= 0;
        }
    }
}

bool FrameService::tryGetNext(AVFrame **result) {
    while (!tryGetNextFrame()) {
        av_packet_unref(pkt);
        if (!tryGetNextPacket()) {
            return false;
        }
    }

    if (result) {
        *result = frame;
    }

    return true;
}

bool FrameService::trySeek(int64_t pts, AVFrame **result) {
    if (frame->pkt_pos < 0 && !tryGetNext(result)) {
        throw std::runtime_error("Cannot seek to first frame");
    }

    if (frame->pts >= pts) {
        return true;
    }

    const auto delta = pts - frame->pts;
    if (delta > 1000) {
        std::cout << "Seeking forward by " << delta << " frames, this might take a while" << std::endl;
        if (av_seek_frame(fmt_ctx, video_stream_idx, pts * 0.95, 0) < 0) {
            throw std::runtime_error("Seek failed");
        }
    }

    while (frame->pts < pts) {
        if (!tryGetNext(result)) {
            return false;
        }
    }

    return true;
}


