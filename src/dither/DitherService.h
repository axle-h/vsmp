#pragma once

#include <vector>
#include "../frame/VideoFormat.h"
#include "../config/Config.h"

extern "C" {
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

class DitherService {
    SwsContext *swsContext;
    AVFrame *scaledFrame;
    int screenWidth;
    int screenHeight;
    int offsetX;
    int offsetY;
    int scaledWidth;
    int scaledHeight;

    std::vector<double> data;

public:
    /**
     * The bitmap data stored in row major order.
     */
    std::vector<uint8_t> result;

    DitherService(VideoFormat sourceFormat, Options* options, int screenWidth, int screenHeight);
    ~DitherService();

    /**
     * Convert to a 1bpp bitmap.
     * @param frame The frame to dither.
     * @return true if the frame is non-black, false otherwise
     */
    bool tryDitherNonEmpty(AVFrame *frame);
};
