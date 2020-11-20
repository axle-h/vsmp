#include <sstream>
#include <unistd.h>
#include "Gpio.h"

using namespace std;

template <class T>
void safeWrite(const string& path, T value) {
    ofstream file (path, ios::out | ios::binary);
    if (!file.is_open()) {
        stringstream ss;
        ss << "Cannot write to " << path;
        throw runtime_error(ss.str());
    }
    file << value;
    file.close();
}

string getGpioPath(int pin, const string& value) {
    stringstream ss;
    ss << "/sys/class/gpio/gpio" << pin << "/" << value;
    return ss.str();
}

Gpio::Gpio(int pin, PinDirection direction) :pin(pin), direction(direction) {
    safeWrite("/sys/class/gpio/export", pin);

    // We need to allow time for the udev rules to fire.
    sleep(1);

    auto directionPath = getGpioPath(pin, "direction");
    safeWrite(directionPath, direction == in ? "in" : "out");

    path = getGpioPath(pin, "value");
}

Gpio::~Gpio() {
    safeWrite("/sys/class/gpio/unexport", pin);
}

bool Gpio::readValue() const {
    ifstream file;
    file.open(path, ios::in | ios::binary);
    if (!file.is_open()) {
        stringstream ss;
        ss << "Cannot read from " << path;
        throw runtime_error(ss.str());
    }

    uint8_t i;
    file >> i;
    file.close();

    return (i & 0x01u) == 0x01u;
}

void Gpio::writeValue(bool value) const {
    safeWrite(path, value ? '1' : '0');
}
