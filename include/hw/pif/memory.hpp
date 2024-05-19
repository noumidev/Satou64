/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::pif::memory {

namespace MemoryBase {
    enum : u16 {
        ROM = 0,
    };
}

namespace MemorySize {
    enum : u16 {
        ROM = 0x400,
    };
}

void init(const char *pifPath);
void deinit();

void reset();

u8 read(const u16 paddr);

void write(const u16 paddr, const u8 data);

}
