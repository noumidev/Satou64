/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::pi {

// PI I/O registers
namespace IORegister {
    enum : u64 {
        IOBase = 0x4600000,
        DRAMADDR = IOBase + 0x00,
        CARTADDR = IOBase + 0x04,
        WRLEN = IOBase + 0x0C,
        STATUS = IOBase + 0x10,
    };
}

void init();
void deinit();

void reset();

void doDMAToRAM();

u32 readIO(const u64 ioaddr);

void writeIO(const u64 ioaddr, const u32 data);

}
