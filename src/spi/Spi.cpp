#include "Spi.h"

#include <stdexcept>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <vector>

const uint8_t BITS_PER_WORD = 8;
const uint32_t SPEED = 10000000;
const uint8_t MODE = SPI_MODE_0;

Spi::Spi(const std::string& device) {
    if ((fd = open(device.c_str(), O_RDWR)) < 0) {
        std::stringstream ss;
        ss << "Cannot open spi device " << device;
        throw std::runtime_error(ss.str());
    }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS_PER_WORD) < 0) {
        throw std::runtime_error("Cannot set SPI bits per word");
    }

    if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) < 0) {
        throw std::runtime_error("Cannot set SPI mode");
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &SPEED) < 0) {
        throw std::runtime_error("Cannot set SPI speed");
    }
}

Spi::~Spi() {
    close(fd);
}

void Spi::transfer(const uint8_t *writeBuffer, const uint8_t *readBuffer, uint32_t length) const {
    spi_ioc_transfer message {
        .tx_buf = (unsigned long) writeBuffer,
        .rx_buf = (unsigned long) readBuffer,
        .len = length,
        .speed_hz = SPEED,
        .bits_per_word = BITS_PER_WORD,
    };
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &message) < 0) {
        throw std::runtime_error("Failure during SPI transfer");
    }
}

void Spi::write(const uint8_t *buffer, uint8_t length) const {
    transfer(buffer, nullptr, length);
}

void Spi::writeByte(uint8_t value) const {
    write(&value, 1);
}