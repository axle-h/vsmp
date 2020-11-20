#pragma once

#include <string>
#include <optional>
#include <memory>

struct Schedule {
    bool enabled;
    int hourFrom;
    int hoursFor;
};

struct Options {
    std::string path;
    int width;
    int height;
    int offsetX;
    int offsetY;
    int frameSkip;
    int displaySeconds;
    Schedule schedule;
};

struct State {
    std::string file;
    int64_t pts;
};

class Config {
    std::string statePath;
    int unSyncedUpdates;

    void setState(const State& state);

public:
    Options options;

    Config();

    std::unique_ptr<State> getState();
    std::unique_ptr<State> setNextState();
    void setPts(State& state, int64_t pts);
};

