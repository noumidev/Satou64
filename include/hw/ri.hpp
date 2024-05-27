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
        REFRESH = IOBase + 0x10,
    };
}

// RI RDRAM registers
namespace RDRAMRegister {
    enum : u32 {
        IOBase = 0x3F00000,
        DeviceID = IOBase + 0x04,
        Delay = IOBase + 0x08,
        Mode = IOBase + 0x0C,
        RefRow = IOBase + 0x14,
        IOBaseBroadcast = 0x3F80000,
    };
}

void init();
void deinit();

void reset();

u64 getRDRAMAddress(const u64 ioaddr);

u32 readIO(const u64 ioaddr);
u32 readRDRAM(const u64 ioaddr);

void writeIO(const u64 ioaddr, const u32 data);

void writeRDRAM(const u64 ioaddr, const u32 data);
void writeRDRAMBroadcast(const u64 ioaddr, const u32 data);

}
