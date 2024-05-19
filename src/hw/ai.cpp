/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/ai.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::ai {

union DRAMADDR {
    u32 raw;
    struct {
        u32 addr : 24;
        u32 : 8;
    };
};

union LENGTH {
    u32 raw;
    struct {
        u32 length : 18;
        u32 : 14;
    };
};

struct Registers {
    DRAMADDR dramaddr;
    LENGTH length;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

void writeIO(const u64 ioaddr, const u32 data) {
    switch (ioaddr) {
        case IORegister::DRAMADDR:
            PLOG_INFO << "DRAMADDR write (data = " << std::hex << data << ")";

            regs.dramaddr.raw = data;
            break;
        case IORegister::LENGTH:
            PLOG_INFO << "LENGTH write (data = " << std::hex << data << ")";

            regs.length.raw = data;
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
