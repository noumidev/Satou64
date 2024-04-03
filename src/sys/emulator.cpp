/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "sys/emulator.hpp"

#include <plog/Log.h>

namespace sys::emulator {

void init(const char *romPath) {
    PLOG_INFO << "ROM path = " << romPath;
}

void deinit() {}

void run() {}

void reset() {}

}
