/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::vi {

// VI I/O registers
namespace IORegister {
    enum : u64 {
        IOBase = 0x4400000,
        CONTROL = IOBase + 0x00,
        ORIGIN = IOBase + 0x04,
        WIDTH = IOBase + 0x08,
        INTR = IOBase + 0x0C,
        CURRENT = IOBase + 0x10,
        BURST = IOBase + 0x14,
        VSYNC = IOBase + 0x18,
        HSYNC = IOBase + 0x1C,
        LEAP = IOBase + 0x20,
        HSTART = IOBase + 0x24,
        VSTART = IOBase + 0x28,
        VBURST = IOBase + 0x2C,
        XSCALE = IOBase + 0x30,
        YSCALE = IOBase + 0x34,
    };
}

void init();
void deinit();

void reset();

u32 readIO(const u64 ioaddr);

void writeIO(const u64 ioaddr, const u32 data);

}
