/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#include "sys/scheduler.hpp"

#include <cassert>
#include <queue>
#include <vector>

namespace sys::scheduler {

constexpr i64 MAX_RUN_CYCLES = 4096;

// Scheduler event
struct Event {
    u64 id;
    int param;

    i64 timestamp;

    bool operator>(const Event &other) const {
        return timestamp > other.timestamp;
    }
};

// Event queue
std::priority_queue<Event, std::vector<Event>, std::greater<Event>> events;

std::vector<std::function<void(int)>> registeredFuncs;

i64 globalTimestamp = 0;

void init() {}

void deinit() {}

void reset() {}

// Registers an event, returns event ID
u64 registerEvent(const std::function<void(int)> func) {
    static u64 idPool;

    registeredFuncs.push_back(func);

    return idPool++;
}

// Adds a scheduler event
void addEvent(const u64 id, const int param, const i64 cyclesUntilEvent) {
    assert(cyclesUntilEvent > 0);

    events.emplace(Event{id, param, globalTimestamp + cyclesUntilEvent});
}

i64 getRunCycles() {
    return MAX_RUN_CYCLES;
}

void run(const i64 runCycles) {
    const auto newTimestamp = globalTimestamp + runCycles;

    while (events.top().timestamp <= newTimestamp) {
        globalTimestamp = events.top().timestamp;

        const auto id = events.top().id;
        const auto param = events.top().param;

        events.pop();

        registeredFuncs[id](param);
    }

    globalTimestamp = newTimestamp;
}

}
