#pragma once

#include <fstream>

enum PinDirection { in, out };

class Gpio {
    int pin;
    PinDirection direction;
    std::string path;

public:
    Gpio(int pin, PinDirection direction);
    ~Gpio();

    bool readValue() const;
    void writeValue(bool value) const;
};