/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::sp {

// SP I/O registers
namespace IORegister {
    enum : u64 {
        IOBase = 0x4040000,
        SPADDR = IOBase + 0x00,
        RAMADDR = IOBase + 0x04,
        RDLEN = IOBase + 0x08,
        WRLEN = IOBase + 0x0C,
        STATUS = IOBase + 0x10,
        DMAFULL = IOBase + 0x14,
        DMABUSY = IOBase + 0x18,
        SEMAPHORE = IOBase + 0x1C,
        PC = IOBase + 0x40000,
    };
}

void init();
void deinit();

void reset();

void BREAK();

bool isHalted();

void doDMAToRAM();
void doDMAToRSP();

u32 readIO(const u64 ioaddr);

void writeIO(const u64 ioaddr, const u32 data);

}
