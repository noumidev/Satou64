/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::si {

// SI I/O registers
namespace IORegister {
    enum : u64 {
        IOBase = 0x4800000,
        DRAMADDR = IOBase + 0x00,
        ADRD64B = IOBase + 0x04,
        ADWR64B = IOBase + 0x10,
        STATUS = IOBase + 0x18,
    };
}

void init();
void deinit();

void reset();

void startDMAFromPIF();
void startDMAToPIF();

void doDMAFromPIF();
void doDMAToPIF();

u32 readIO(const u64 ioaddr);

void writeIO(const u64 ioaddr, const u32 data);

}
