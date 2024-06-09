/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/si.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/mi.hpp"
#include "hw/pif/pif.hpp"

#include "sys/memory.hpp"

namespace hw::si {

union DRAMADDR {
    u32 raw;
    struct {
        u32 addr : 24;
        u32 : 8;
    };
};

union ADRD64B {
    u32 raw;
    struct {
        u32 : 2;
        u32 addr : 30;
    };
};

union ADWR64B {
    u32 raw;
    struct {
        u32 : 2;
        u32 addr : 30;
    };
};

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
    DRAMADDR dramaddr;
    ADRD64B adrd64b;
    ADWR64B adwr64b;
    STATUS status;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

void startDMAFromPIF() {
    if (regs.status.dmaBusy != 0) {
        PLOG_ERROR << "SI DMA is still active";

        return;
    }

    const u64 dramaddr = regs.dramaddr.addr;
    const u64 pifaddr = ((u64)regs.adrd64b.addr) << 2;

    PLOG_VERBOSE << "DMA from PIF requested (DRAM address = " << std::hex << dramaddr << ", PIF RAM address = " << pifaddr << ")";

    regs.status.dmaBusy = 1;

    pif::setInterruptAPending();
    pif::setRCPPort(true, true);
}

void startDMAToPIF() {
    if (regs.status.dmaBusy != 0) {
        PLOG_ERROR << "SI DMA is still active";

        return;
    }

    const u64 dramaddr = regs.dramaddr.addr;
    const u64 pifaddr = ((u64)regs.adrd64b.addr) << 2;

    PLOG_VERBOSE << "DMA to PIF requested (DRAM address = " << std::hex << dramaddr << ", PIF RAM address = " << pifaddr << ")";

    regs.status.dmaBusy = 1;

    pif::setInterruptAPending();
    pif::setRCPPort(false, true);
}

void doDMAFromPIF() {
    if (regs.status.dmaBusy == 0) {
        return;
    }

    const u64 dramaddr = regs.dramaddr.addr;
    const u64 pifaddr = ((u64)regs.adrd64b.addr) << 2;

    PLOG_VERBOSE << "DMA from PIF (DRAM address = " << std::hex << dramaddr << ", PIF RAM address = " << pifaddr << ")";

    for (u64 i = 0; i < 64; i += 4) {
        sys::memory::write(dramaddr + i, byteswap(pif::read<u32>(pifaddr + i)));
    }

    regs.status.dmaBusy = 0;
    regs.dramaddr.addr += 64;

    pif::setInterruptAPending();

    mi::requestInterrupt(mi::InterruptSource::SI);
}

void doDMAToPIF() {
    if (regs.status.dmaBusy == 0) {
        return;
    }

    const u64 dramaddr = regs.dramaddr.addr;
    const u64 pifaddr = ((u64)regs.adwr64b.addr) << 2;

    PLOG_VERBOSE << "DMA to PIF (DRAM address = " << std::hex << dramaddr << ", PIF RAM address = " << pifaddr << ")";

    for (u64 i = 0; i < 64; i += 4) {
        pif::write(pifaddr + i, byteswap(sys::memory::read<u32>(dramaddr + i)));
    }

    regs.status.dmaBusy = 0;
    regs.dramaddr.addr += 64;

    pif::setInterruptAPending();

    mi::requestInterrupt(mi::InterruptSource::SI);
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
        case IORegister::ADRD64B:
            PLOG_INFO << "ADRD64B write (data = " << std::hex << data << ")";

            regs.adrd64b.raw = data;

            startDMAFromPIF();
            break;
        case IORegister::ADWR64B:
            PLOG_INFO << "ADWR64B write (data = " << std::hex << data << ")";

            regs.adwr64b.raw = data;

            startDMAToPIF();
            break;
        case IORegister::STATUS:
            PLOG_INFO << "STATUS write (data = " << std::hex << data << ")";

            PLOG_INFO << "Interrupt flag cleared";

            mi::clearInterrupt(mi::InterruptSource::SI);
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
