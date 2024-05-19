/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::ai {

// AI I/O registers
namespace IORegister {
    enum : u64 {
        IOBase = 0x4500000,
        DRAMADDR = IOBase + 0x00,
        LENGTH = IOBase + 0x04,
    };
}

void init();
void deinit();

void reset();

u32 readIO(const u64 ioaddr);

void writeIO(const u64 ioaddr, const u32 data);

}
