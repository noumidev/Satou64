/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/si.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "sys/memory.hpp"

namespace hw::si {

union STATUS {
    u32 raw;
    struct {
        u32 dmaBusy : 1;
        u32 ioBusy : 1;
        u32 readPending : 1;
        u32 dmaError : 1;
        u32 pifChannelState : 4;
        u32 dmaState : 4;
        u32 : 20;
    };
};

struct Registers {
    STATUS status;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        case IORegister::STATUS:
            PLOG_INFO << "STATUS read";

            return regs.status.raw;
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
