/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/pi.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "sys/memory.hpp"

namespace hw::pi {

union DRAMADDR {
    u32 raw;
    struct {
        u32 addr : 24;
        u32 : 8;
    };
};

struct CARTADDR {
    u32 addr;
};

union WRLEN {
    u32 raw;
    struct {
        u32 len : 24;
        u32 : 8;
    };
};

union STATUS {
    u32 raw;
    struct {
        u32 dmaBusy : 1;
        u32 ioBusy : 1;
        u32 error : 1;
        u32 : 29;
    };
};

struct Registers {
    DRAMADDR dramaddr;
    CARTADDR cartaddr;
    WRLEN wrlen;
    STATUS status;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

void doDMAToRAM() {
    const u32 cartaddr = regs.cartaddr.addr;
    const u32 dramaddr = regs.dramaddr.addr;
    const u32 len = regs.wrlen.len + 1;

    PLOG_VERBOSE << "DMA to RAM (cart address = " << std::hex << cartaddr << ", DRAM address = " << dramaddr << ", length = " << len << ")";

    const u8 *src = sys::memory::getPointer(cartaddr);
    u8 *dst = sys::memory::getPointer(dramaddr);

    std::memcpy(dst, src, len);

    regs.status.dmaBusy = 0;
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
        case IORegister::DRAMADDR:
            PLOG_INFO << "DRAMADDR write (data = " << std::hex << data << ")";

            regs.dramaddr.raw = data;
            break;
        case IORegister::CARTADDR:
            PLOG_INFO << "CARTADDR write (data = " << std::hex << data << ")";

            regs.cartaddr.addr = data;
            break;
        case IORegister::WRLEN:
            PLOG_INFO << "WRLEN write (data = " << std::hex << data << ")";

            regs.wrlen.len = data;

            doDMAToRAM();
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
