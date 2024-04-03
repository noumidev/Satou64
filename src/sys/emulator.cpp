/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "sys/emulator.hpp"

#include <plog/Log.h>

#include "hw/pi.hpp"
#include "hw/vi.hpp"
#include "hw/cpu/cpu.hpp"

#include "sys/memory.hpp"

namespace sys::emulator {

constexpr bool IS_FAST_BOOT = true;

void init(const char *romPath) {
    PLOG_INFO << "ROM path = " << romPath;

    sys::memory::init(romPath);

    hw::cpu::init();
    hw::pi::init();
    hw::vi::init();
}

void deinit() {
    sys::memory::deinit();

    hw::cpu::deinit();
    hw::pi::deinit();
    hw::vi::deinit();
}

void run() {
    while (true) {
        hw::cpu::run(1);
    }
}

void reset() {
    sys::memory::reset(IS_FAST_BOOT);

    hw::cpu::reset(IS_FAST_BOOT);
    hw::pi::reset();
    hw::vi::reset();
}

}
