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
        BSDDOM1LAT = IOBase + 0x14,
        BSDDOM1PWD = IOBase + 0x18,
        BSDDOM1PGS = IOBase + 0x1C,
        BSDDOM1RLS = IOBase + 0x20,
        BSDDOM2LAT = IOBase + 0x24,
        BSDDOM2PWD = IOBase + 0x28,
        BSDDOM2PGS = IOBase + 0x2C,
        BSDDOM2RLS = IOBase + 0x30,
    };
}

void init();
void deinit();

void reset();

void doDMAToRAM();

u32 readIO(const u64 ioaddr);

void writeIO(const u64 ioaddr, const u32 data);

}
