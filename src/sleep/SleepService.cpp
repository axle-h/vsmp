#include <chrono>
#include "SleepService.h"
#include <unistd.h>
#include <iostream>
#include <date/date.h>

using namespace std::chrono;
using namespace date;

template <class A, class B>
void sleepUntil(A now, B to) {
    auto time = make_time(to - now);
    auto seconds = (time.hours().count() * 60 + time.minutes().count()) * 60 + time.seconds().count();
    sleep(seconds);
}

SleepService::SleepService(Options* options) : schedule(&options->schedule), delaySeconds(options->displaySeconds) {
    reset();
}

void SleepService::reset() {
    ref = steady_clock::now();
}

void SleepService::sleepAndReset() {
    auto end = steady_clock::now();
    auto duration = duration_cast<seconds>(end - ref).count();
    auto delay = delaySeconds - duration;

    if (delay > 0) {
        sleep(delay);
    }
    reset();
}

void SleepService::sleepUntilHoursOfOperation() const {
    if (!schedule->enabled) {
        return;
    }

    const auto now = floor<seconds>(system_clock::now());
    const auto midnight = floor<days>(now);
    const auto from = midnight + hours(schedule->hourFrom);
    const auto to = from + hours(schedule->hoursFor);

    if (now < from) {
        std::cout << "hours of operation are " << from << " - " << to << ", current time is " << now << ", sleeping until " << from << std::endl;
        sleepUntil(now, from);
    } else if (now >= to) {
        const auto tomorrow = from + days(1);
        std::cout << "hours of operation are " << from << " - " << to << ", current time is " << now << ", sleeping until " << tomorrow << std::endl;
        sleepUntil(now, tomorrow);
    }
}


