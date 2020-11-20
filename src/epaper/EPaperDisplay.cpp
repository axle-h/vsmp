#include "EPaperDisplay.h"

#include <unistd.h>
#include <iostream>

const int EPD_RST_PIN = 17;
const int EPD_DC_PIN = 25;
const int EPD_BUSY_PIN = 24;

void sleepMs(int ms) {
    usleep(ms * 1000);
}

EPaperDisplay::EPaperDisplay() {
    rst = new Gpio(EPD_RST_PIN, out);
    dc = new Gpio(EPD_DC_PIN, out);
    busy = new Gpio(EPD_BUSY_PIN, in);
    spi = new Spi("/dev/spidev0.0");
    const auto pixels = EPD_HEIGHT * EPD_WIDTH;
    screenBufferLength = pixels % 8 == 0 ? (pixels / 8) : (pixels / 8 + 1);
}

EPaperDisplay::~EPaperDisplay() {
    delete rst;
    delete dc;
    delete busy;
    delete spi;
}

void EPaperDisplay::reset() {
    rst->writeValue(true);
    sleepMs(200);
    rst->writeValue(false);
    sleepMs(2);
    rst->writeValue(true);
    sleepMs(200);
}

void EPaperDisplay::waitUntilIdle() {
    do {
        sendCommand(0x71);
        sleepMs(1);
    } while (!busy->readValue());
    sleepMs(200);
}

void EPaperDisplay::sendCommand(uint8_t value) {
    dc->writeValue(false);
    spi->writeByte(value);
}

void EPaperDisplay::sendData(uint8_t *buffer, int length) {
    dc->writeValue(true);
    for (auto p = buffer; p < buffer + length; p++) {
        spi->writeByte(*p);
    }
}

void EPaperDisplay::sendByte(uint8_t value) {
    dc->writeValue(true);
    spi->writeByte(value);
}

void EPaperDisplay::init() {
    reset();

    sendCommand(0x01); //POWER SETTING
    sendByte(0x07);
    sendByte(0x07); //VGH=20V,VGL=-20V
    sendByte(0x3f); //VDH=15V
    sendByte(0x3f); //VDL=-15V

    sendCommand(0x04); //POWER ON
    sleepMs(100);
    waitUntilIdle();

    sendCommand(0X00); //PANNEL SETTING
    sendByte(0x1F); //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

    sendCommand(0x61); //tres
    sendByte(0x03); //source 800
    sendByte(0x20);
    sendByte(0x01); //gate 480
    sendByte(0xE0);

    sendCommand(0X15);
    sendByte(0x00);

    sendCommand(0X50); //VCOM AND DATA INTERVAL SETTING
    sendByte(0x10);
    sendByte(0x07);

    sendCommand(0X60); //TCON SETTING
    sendByte(0x22);
}

void EPaperDisplay::turnOn() {
    sendCommand(0x12); //DISPLAY REFRESH
    sleepMs(100); //!!! The delay here is necessary, 200uS at least!!!
    waitUntilIdle();
}

void EPaperDisplay::clear() {
    std::vector<uint8_t> buffer(screenBufferLength, 0);

    sendCommand(0x10);
    sendData(buffer.data(), screenBufferLength);

    sendCommand(0x13);
    sendData(buffer.data(), screenBufferLength);

    turnOn();
}

void EPaperDisplay::write(std::vector<uint8_t> frame) {
    // 0 is white, 1 is black on an e-paper display
    for (auto i = 0; i < screenBufferLength; i++) {
        frame.at(i) = ~frame.at(i);
    }
    sendCommand(0x13);
    sendData(frame.data(), screenBufferLength);
    turnOn();
}

void EPaperDisplay::writeTestPattern(const Options& options) {
    if (options.offsetX < 0 || options.width <= 0 || options.offsetX + options.width > EPD_WIDTH) {
        throw std::runtime_error("invalid width");
    }

    if (options.offsetY < 0 || options.height <= 0 || options.offsetY + options.height > EPD_HEIGHT) {
        throw std::runtime_error("invalid height");
    }

    std::vector<uint8_t> buffer(screenBufferLength, 0);
    for (auto y = options.offsetY; y < options.offsetY + options.height; y++) {
        for (auto x = options.offsetX; x < options.offsetX + options.width; x++) {
            auto i = y * EPD_WIDTH + x;
            uint8_t bit = 7 - i % 8;
            buffer.at(i / 8) |= 1UL << bit;
        }
    }

    sendCommand(0x13);
    sendData(buffer.data(), screenBufferLength);
    turnOn();
}




