/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/dp.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::dp {

union STATUS {
    u32 raw;
    struct {
        u32 xbus : 1;
        u32 freeze : 1;
        u32 flush : 1;
        u32 start : 1;
        u32 tmemBusy : 1;
        u32 pipeBusy : 1;
        u32 bufBusy : 1;
        u32 cbufBusy : 1;
        u32 dmaBusy : 1;
        u32 endPending : 1;
        u32 startPending : 1;
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
