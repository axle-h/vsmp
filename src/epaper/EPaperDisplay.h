#pragma once

#include <vector>
#include "../gpio/Gpio.h"
#include "../spi/Spi.h"
#include "../config/Config.h"

const int EPD_WIDTH = 800;
const int EPD_HEIGHT = 480;

class EPaperDisplay {
    Gpio *rst, *dc, *busy;
    Spi *spi;
    int screenBufferLength;

    void reset();
    void waitUntilIdle();
    void sendCommand(uint8_t value);
    void sendByte(uint8_t value);
    void sendData(uint8_t *buffer, int length);

public:
    EPaperDisplay();
    ~EPaperDisplay();

    void init();
    void turnOn();
    void clear();
    void write(std::vector<uint8_t> frame);
    void writeTestPattern(const Options& options);
};