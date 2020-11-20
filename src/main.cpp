#include <iostream>
#include <memory>
#include <algorithm>

#define E_PAPER 1

#include "dither/DitherService.h"
#include "frame/FrameService.h"
#include "config/Config.h"
#include "sleep/SleepService.h"

#if E_PAPER
    #include "epaper/EPaperDisplay.h"
#else
    const int EPD_WIDTH = 800;
    const int EPD_HEIGHT = 480;
#endif

// TODO validate state & options
int main(int argc, char *argv[]) {
    std::unique_ptr<Config> config(new Config);

    #if E_PAPER
        std::unique_ptr<EPaperDisplay> display(new EPaperDisplay);
        display->init();

        std::vector<std::string> arguments(argv + 1, argv + argc);
        if (std::find(arguments.begin(), arguments.end(), "--test") != arguments.end()) {
            display->writeTestPattern(config->options);
            return 0;
        }
    #endif

    auto state = config->getState();
    if (!state) {
        state = config->setNextState();
    }

    std::unique_ptr<SleepService> sleep(new SleepService(&config->options));

    while (state) {
        std::unique_ptr<FrameService> frameService(new FrameService(state->file));
        std::unique_ptr<DitherService> ditherService(new DitherService(
                frameService->getFormat(), &config->options, EPD_WIDTH, EPD_HEIGHT));
        AVFrame *frame = nullptr;

        std::cout << "Writing file " << state->file << " @" << state->pts + 1 << std::endl;

        #if E_PAPER
            auto firstFrame = true;
        #endif
        auto skippedFrames = 0;
        while (frameService->trySeek(state->pts + 1, &frame)) {
            // Skip all black frames.
            if (ditherService->tryDitherNonEmpty(frame)
                && ++skippedFrames == config->options.frameSkip) {

                #if E_PAPER
                    sleep->sleepUntilHoursOfOperation();
                    if (firstFrame) {
                        firstFrame = false;
                        sleep->reset();
                    } else {
                        sleep->sleepAndReset();
                    }
                    std::cout << "Displaying frame " << frame->pts << std::endl;
                    display->write(ditherService->result);
                #else
                    auto file = fopen("/home/alex/src/vsmp/raw", "wb");
                    fwrite(ditherService->result.data(), 1, ditherService->result.size(), file);
                    fclose(file);
                #endif

                skippedFrames = 0;
            }

            config->setPts(*state, frame->pts);
        }

        state = config->setNextState();
    }

    std::cerr << "No movie files found in " << config->options.path << std::endl;

    return 0;
}