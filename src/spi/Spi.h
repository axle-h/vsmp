#pragma once

#include <cstdint>
#include <string>

class Spi {
    int fd;

public:
    explicit Spi(const std::string& device);
    ~Spi();
    void transfer(const uint8_t *writeBuffer, const uint8_t *readBuffer, uint32_t length) const;
    void write(const uint8_t *buffer, uint8_t length) const;
    void writeByte(uint8_t value) const;
};