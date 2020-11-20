#pragma once

#include <chrono>
#include "../config/Config.h"

class SleepService {
    int delaySeconds;
    Schedule *schedule;
    std::chrono::steady_clock::time_point ref;

public:
    explicit SleepService(Options* options);
    void reset();
    void sleepAndReset();
    void sleepUntilHoursOfOperation() const;
};