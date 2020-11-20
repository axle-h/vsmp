#include "Config.h"

#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

std::string getHomePath() {
    const char *homedir;
    if ((homedir = getenv("HOME")) == nullptr) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    std::stringstream ss;
    ss << homedir << "/" << ".vsmp";
    return ss.str();
}

using json = nlohmann::json;

void tryCreateDirectories(const std::string& path) {
    if(access(path.c_str(), F_OK) < 0) {
        if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
            std::stringstream ss;
            ss << "Cannot create config directory " << path;
            throw std::runtime_error(ss.str());
        }
    }
}

Config::Config() :unSyncedUpdates(0) {
    auto configDir = getHomePath();
    tryCreateDirectories(configDir);

    std::stringstream stateStream;
    stateStream << configDir << "/" << "state.json";
    statePath = stateStream.str();

    std::stringstream optionsStream;
    optionsStream << configDir << "/" << "options.json";
    auto optionsPath = optionsStream.str();

    // Get options.
    if (access(optionsPath.c_str(), F_OK) == 0) {
        std::ifstream file(optionsStream.str());
        json j;
        file >> j;
        file.close();

        options = {
            .path = j.at("path"),
            .width = j.at("width"),
            .height = j.at("height"),
            .offsetX = j.at("offsetX"),
            .offsetY = j.at("offsetY"),
            .frameSkip = j.at("frameSkip"),
            .displaySeconds = j.at("displaySeconds"),
            .schedule = {
                .enabled = j.at("schedule").at("enabled"),
                .hourFrom = j.at("schedule").at("hourFrom"),
                .hoursFor = j.at("schedule").at("hoursFor"),
            }
        };
        // TODO validation
    } else {
        std::stringstream moviesStream;
        moviesStream << configDir << "/" << "movies";

        options = {
            .path = moviesStream.str(),
            .width = 800,
            .height = 480,
            .offsetX = 0,
            .offsetY = 0,
            .frameSkip = 1,
            .displaySeconds = 120,
            .schedule = { .enabled = false, .hourFrom = 8, .hoursFor = 14 },
        };
        std::ofstream file(optionsPath, std::ios_base::trunc);
        json j = {
            { "path", options.path },
            { "width", options.width },
            { "height", options.height },
            { "offsetX", options.offsetX },
            { "offsetY", options.offsetY },
            { "frameSkip", options.frameSkip },
            { "displaySeconds", options.displaySeconds },
            { "schedule", {
                { "enabled", options.schedule.enabled },
                { "hourFrom", options.schedule.hourFrom },
                { "hoursFor", options.schedule.hoursFor },
            }}
        };
        file << j << std::endl;
        file.close();
    }

    tryCreateDirectories(options.path);
}

void Config::setState(const State& state) {
    std::ofstream file(statePath, std::ios_base::trunc);

    json j = {
        {"file", state.file},
        {"pts", state.pts},
    };

    file << j << std::endl;
    file.close();
}

std::unique_ptr<State> Config::getState() {
    if (access(statePath.c_str(), F_OK) == 0) {
        std::ifstream file(statePath);
        json j;
        file >> j;
        file.close();

        // TODO validation
        std::unique_ptr<State> state(new State { .file = j.at("file"), .pts = j.at("pts") });
        return state;
    } else {
        return nullptr;
    }
}

std::vector<std::string> getMoviePaths(const std::string& path) {
    static std::vector<std::string> extensions = { ".mp4", ".mkv", ".avi" };
    std::vector<std::string> result;

    auto dir = opendir(path.c_str());
    if (!dir) {
        std::stringstream ss;
        ss << "Cannot open movie path " << path;
        throw std::runtime_error(ss.str());
    }

    auto ent = readdir(dir);
    while (ent) {
        auto s = std::string(ent->d_name);
        if (s.length() > 4 && std::find(extensions.begin(), extensions.end(), s.substr(s.length() - 4)) != extensions.end()) {
            result.emplace_back(s);
        }
        ent = readdir(dir);
    }

    closedir(dir);
    return result;
}

std::unique_ptr<State> Config::setNextState() {
    auto state = getState();
    auto files = getMoviePaths(options.path);

    std::stringstream pathStream;
    pathStream << options.path << "/";

    if (state) {
        auto file = std::find(files.begin(), files.end(), state->file);
        if (file != files.end() && ++file != files.end()) {
            pathStream << *file;
            state->file = pathStream.str();
            state->pts = -1;
            setState(*state);
            return state;
        }
    }

    if (files.empty()) {
        return nullptr;
    }

    pathStream << *files.begin();
    std::unique_ptr<State> state0(new State { .file = pathStream.str(), .pts = -1 });
    setState(*state0);
    return state0;
}

void Config::setPts(State& state, int64_t pts) {
    state.pts = pts;

    // Every 10th frame, write the state to disc
    if (++unSyncedUpdates % 10 == 0) {
        unSyncedUpdates = 0;
        setState(state);
    }
}
