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

union CONTROL {
    u32 raw;
    struct {
        u32 dmaEnable : 1;
        u32 : 31;
    };
};

union DACRATE {
    u32 raw;
    struct {
        u32 dacRate : 14;
        u32 : 18;
    };
};

union BITRATE {
    u32 raw;
    struct {
        u32 bitRate : 14;
        u32 : 18;
    };
};

struct Registers {
    DRAMADDR dramaddr;
    LENGTH length;
    CONTROL control;
    DACRATE dacrate;
    BITRATE bitrate;
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
        case IORegister::CONTROL:
            PLOG_INFO << "CONTROL write (data = " << std::hex << data << ")";

            regs.control.raw = data;

            if (regs.control.dmaEnable) {
                PLOG_WARNING << "DMA enabled";
            } else {
                PLOG_VERBOSE << "DMA disabled";
            }
            break;
        case IORegister::STATUS:
            PLOG_INFO << "STATUS write (data = " << std::hex << data << ")";
            PLOG_WARNING << "Interrupt flag cleared";
            break;
        case IORegister::DACRATE:
            PLOG_INFO << "DACRATE write (data = " << std::hex << data << ")";

            regs.dacrate.raw = data;
            break;
        case IORegister::BITRATE:
            PLOG_INFO << "BITRATE write (data = " << std::hex << data << ")";

            regs.bitrate.raw = data;
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
