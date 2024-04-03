/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "sys/emulator.hpp"

#include <plog/Log.h>

#include "sys/memory.hpp"

namespace sys::emulator {

constexpr bool IS_FAST_BOOT = true;

void init(const char *romPath) {
    PLOG_INFO << "ROM path = " << romPath;

    sys::memory::init(romPath);
}

void deinit() {
    sys::memory::deinit();
}

void run() {}

void reset() {
    sys::memory::reset(IS_FAST_BOOT);
}

}
