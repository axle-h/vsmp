#include "DitherService.h"
#include <algorithm>
#include <stdexcept>

extern "C" {
    #include <libavformat/avformat.h>
}

DitherService::DitherService(VideoFormat sourceFormat, Options* options, int screenWidth, int screenHeight)
    : screenWidth(screenWidth), screenHeight(screenHeight) {

    auto visibleHeight = options->height;
    auto visibleWidth = options->width;
    auto maxRatio = std::min((double) screenWidth / sourceFormat.width, (double) screenHeight / sourceFormat.height);
    auto bestOverflowRatio = std::max((double) visibleWidth / sourceFormat.width, (double) visibleHeight / sourceFormat.height);
    auto minRatio = std::min((double) visibleWidth / sourceFormat.width, (double) visibleHeight / sourceFormat.height);

    auto ratio = bestOverflowRatio < maxRatio && bestOverflowRatio >  minRatio ? bestOverflowRatio : minRatio;

    scaledHeight = (int) (sourceFormat.height * ratio);
    scaledWidth = (int) (sourceFormat.width * ratio);

    offsetY = options->offsetY + (visibleHeight - scaledHeight) / 2;
    offsetX = options->offsetX + (visibleWidth - scaledWidth) / 2;

    swsContext = sws_getContext(
            sourceFormat.width,
            sourceFormat.height,
            sourceFormat.pixelFormat,
            scaledWidth,
            scaledHeight,
            AV_PIX_FMT_GRAY16,
            SWS_LANCZOS | SWS_ACCURATE_RND,
            nullptr, nullptr, nullptr);

    scaledFrame = av_frame_alloc();
    scaledFrame->format = AV_PIX_FMT_GRAY16;
    scaledFrame->width = scaledWidth;
    scaledFrame->height = scaledHeight;
    if (av_frame_get_buffer(scaledFrame, 0) < 0) {
        throw std::exception();
    }

    const auto pixels = screenHeight * screenWidth;
    data.resize(pixels);
    const auto resultSize = pixels % 8 == 0 ? (pixels / 8) : (pixels / 8 + 1);
    result.resize(resultSize);
}

DitherService::~DitherService() {
    sws_freeContext(swsContext);
    av_frame_free(&scaledFrame);
}

bool DitherService::tryDitherNonEmpty(AVFrame *frame) {
    if (sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, scaledFrame->data, scaledFrame->linesize) < 0) {
        throw std::runtime_error("Cannot scale frame");
    }

    // Determine if the image is empty (all black)
    auto scaledData = (uint16_t*) scaledFrame->data[0];
    auto allBlack = true;
    auto scaledPixels = scaledWidth * scaledHeight;
    for (auto p = scaledData; p < scaledData + scaledPixels; p++) {
        if (*p > 0) {
            allBlack = false;
            break;
        }
    }

    if (allBlack) {
        return false;
    }

    // Copy grayscale image into a floating point representation ready for dithering
    const auto lineSize = scaledFrame->linesize[0] / 2;
    for (auto y = 0; y < screenHeight; y++) {
        auto scaledY = y - offsetY;
        auto isScaledY = scaledY >= 0 && scaledY < scaledHeight;
        for (auto x = 0; x < screenWidth; x++) {
            auto scaledX = x - offsetX;
            auto i = y * screenWidth + x;
            data.at(i) = isScaledY && scaledX >= 0 && scaledX < scaledWidth
                ? ((double) scaledData[scaledY * lineSize + scaledX]) / std::numeric_limits<uint16_t>::max()
                : 0;
        }
    }

    // dither -> 1-bit via Floyd Steinberg https://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering
    auto threshold = 0.5;
    for (auto y = 0; y < screenHeight; y++) {
        for (auto x = 0; x < screenWidth; x++) {
            auto i = y * screenWidth + x;
            auto oldPixel = data.at(i);
            auto newPixel = data.at(i) = oldPixel > threshold ? 1.0 : 0;
            auto error = oldPixel - newPixel;

            if (x < screenWidth - 1) {
                // pixel[x + 1][y    ] := pixel[x + 1][y    ] + quant_error * 7 / 16
                data.at(y * screenWidth + x + 1) += error * 7 / 16;
            }

            if (y < screenHeight - 1) {
                auto nextY = (y + 1) * screenWidth;
                if (x > 0) {
                    // pixel[x - 1][y + 1] := pixel[x - 1][y + 1] + quant_error * 3 / 16
                    data.at(nextY + x - 1) += error * 3 / 16;
                }

                // pixel[x    ][y + 1] := pixel[x    ][y + 1] + quant_error * 5 / 16
                data.at(nextY + x) += error * 5 / 16;

                if (x < screenWidth - 1) {
                    // pixel[x + 1][y + 1] := pixel[x + 1][y + 1] + quant_error * 1 / 16
                    data.at(nextY + x + 1) += error / 16;
                }
            }
        }
    }

    for (auto i = 0; i < data.size(); i++) {
        uint8_t bit = 7 - i % 8;
        if (data.at(i) > threshold) {
            // set the bit
            result.at(i / 8) |= 0x01u << bit;
        } else {
            // clear the bit
            result.at(i / 8) &= ~(0x01u << bit);
        }
    }

    return true;
}
