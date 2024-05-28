/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::mi {

// MI I/O registers
namespace IORegister {
    enum : u64 {
        IOBase = 0x4300000,
        MODE = IOBase + 0x00,
        VERSION = IOBase + 0x04,
        MASK = IOBase + 0x0C,
    };
}

void init();
void deinit();

void reset();

u32 readIO(const u64 ioaddr);

void writeIO(const u64 ioaddr, const u32 data);

}
