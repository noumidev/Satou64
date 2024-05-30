/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include <functional>

#include "../common/types.hpp"

namespace sys::scheduler {

constexpr i64 CPU_FREQUENCY = 93750000;

void init();
void deinit();

void reset();

u64 registerEvent(const std::function<void(int)> func);

void addEvent(const u64 id, const int param, const i64 cycles);

i64 getRunCycles();

void run(const i64 cycles);

}
