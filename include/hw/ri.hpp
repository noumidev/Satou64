/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::ri {

// RI I/O registers
namespace IORegister {
    enum : u64 {
        IOBase = 0x4700000,
        MODE = IOBase + 0x00,
        CONFIG = IOBase + 0x04,
        CURRENTLOAD = IOBase + 0x08,
        SELECT = IOBase + 0x0C,
    };
}

void init();
void deinit();

void reset();

u32 readIO(const u64 ioaddr);

void writeIO(const u64 ioaddr, const u32 data);

}
