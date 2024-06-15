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
        CONTROL = IOBase + 0x08,
        STATUS = IOBase + 0x0C,
        DACRATE = IOBase + 0x10,
        BITRATE = IOBase + 0x14,
    };
}

void init();
void deinit();

void reset();

i64 getAICycles();

bool isEnabled();

void updateStatus();

u32 getSamples();

u32 readIO(const u64 ioaddr);

void writeIO(const u64 ioaddr, const u32 data);

void doSample();

}
