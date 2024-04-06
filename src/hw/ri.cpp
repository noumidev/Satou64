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

union MODE {
    u32 raw;
    struct {
        u32 operationMode : 2;
        u32 stopTransmit : 1;
        u32 stopReceive : 1;
        u32 : 28;
    };
};

union CONFIG {
    u32 raw;
    struct {
        u32 currentControl : 6;
        u32 autoCurrentControl : 1;
        u32 : 25;
    };
};

union SELECT {
    u32 raw;
    struct {
        u32 rsel : 4;
        u32 tsel : 4;
        u32 : 24;
    };
};

struct Registers {
    MODE mode;
    CONFIG config;
    SELECT select;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        case IORegister::SELECT:
            PLOG_INFO << "SELECT read";

            return regs.select.raw;
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

void writeIO(const u64 ioaddr, const u32 data) {
    switch (ioaddr) {
        case IORegister::MODE:
            PLOG_INFO << "MODE write (data = " << std::hex << data << ")";

            regs.mode.raw = data;
            break;
        case IORegister::CONFIG:
            PLOG_INFO << "CONFIG write (data = " << std::hex << data << ")";

            regs.config.raw = data;
            break;
        case IORegister::CURRENTLOAD:
            PLOG_INFO << "CURRENTLOAD write (data = " << std::hex << data << ")";
            break;
        case IORegister::SELECT:
            PLOG_INFO << "SELECT write (data = " << std::hex << data << ")";

            regs.select.raw = data;
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
