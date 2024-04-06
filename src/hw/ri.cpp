/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/ri.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::ri {

struct Registers {
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        case IORegister::MODE:
            PLOG_INFO << "MODE read";

            return 0;
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

void writeIO(const u64 ioaddr, const u32 data) {
    switch (ioaddr) {
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
