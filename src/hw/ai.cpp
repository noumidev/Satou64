/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/ai.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/mi.hpp"

#include "sys/memory.hpp"
#include "sys/scheduler.hpp"

namespace hw::ai {

union DRAMADDR {
    u32 raw;
    struct {
        u32 : 3;
        u32 addr : 21;
        u32 : 8;
    };
};

union LENGTH {
    u32 raw;
    struct {
        u32 : 3;
        u32 length : 15;
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

union STATUS {
    u32 raw;
    struct {
        u32 full : 1;
        u32 count : 14;
        u32 : 1;
        u32 bitClock : 1;
        u32 : 2;
        u32 wordClock : 1;
        u32 : 10;
        u32 busy : 1;
        u32 : 1;
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
    DRAMADDR dramaddr[2];
    LENGTH length[2];
    CONTROL control;
    STATUS status;
    DACRATE dacrate;
    BITRATE bitrate;

    u32 currentSamples;
};

Registers regs;

u32 activeDMAs;

u64 idDoSample;

void init() {
    idDoSample = sys::scheduler::registerEvent([](int) { doSample(); });
}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));

    activeDMAs = 0;
}

// Taken from https://github.com/Dillonb/n64/blob/ad5924d02b6c218b3769c1c2cf4748f177c9eacd/src/interface/ai.c#L30
i64 getAICycles() {
    return std::max(1LL, sys::scheduler::CPU_FREQUENCY / 4 / (regs.dacrate.dacRate + 1)) * 1.037;
}

bool isEnabled() {
    return regs.control.dmaEnable != 0;
}

void updateStatus() {
    regs.status.busy = activeDMAs > 0;
    regs.status.full = activeDMAs > 1;
}

u32 getSamples() {
    return regs.currentSamples;
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        case IORegister::LENGTH:
            PLOG_WARNING << "LENGTH read";

            return regs.length[0].raw;
        case IORegister::STATUS:
            PLOG_WARNING << "STATUS read";

            return regs.status.raw | (regs.status.full << 31) | (regs.control.dmaEnable << 25);
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

void writeIO(const u64 ioaddr, const u32 data) {
    switch (ioaddr) {
        case IORegister::DRAMADDR:
            PLOG_INFO << "DRAMADDR write (data = " << std::hex << data << ")";

            if (activeDMAs < 2) {
                regs.dramaddr[activeDMAs].raw = data;
            }
            break;
        case IORegister::LENGTH:
            PLOG_INFO << "LENGTH write (data = " << std::hex << data << ")";

            if ((activeDMAs < 2) && (data != 0)) {
                regs.length[activeDMAs].raw = data;

                activeDMAs++;

                if ((activeDMAs == 1) && regs.control.dmaEnable) {
                    // "Start" DMA, send interrupt to notify CPU
                    mi::requestInterrupt(mi::InterruptSource::AI);

                    sys::scheduler::addEvent(idDoSample, 0, getAICycles());
                }
            }

            updateStatus();
            break;
        case IORegister::CONTROL:
            PLOG_INFO << "CONTROL write (data = " << std::hex << data << ")";

            regs.control.raw = data;

            if (regs.control.dmaEnable) {
                PLOG_INFO << "DMA enabled";
            } else {
                PLOG_INFO << "DMA disabled";

                regs.currentSamples = 0;
            }
            break;
        case IORegister::STATUS:
            PLOG_INFO << "STATUS write (data = " << std::hex << data << ")";
            PLOG_INFO << "Interrupt flag cleared";

            mi::clearInterrupt(mi::InterruptSource::AI);
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

void doSample() {
    DRAMADDR &dramaddr = regs.dramaddr[0];
    LENGTH &length = regs.length[0];

    // Advance AI DMA
    const u64 paddr = ((u64)dramaddr.addr) << 3;

    regs.currentSamples = sys::memory::read<u32>(paddr);

    dramaddr.addr++;
    length.length--;

    if (length.length == 0) {
        if (activeDMAs > 1) {
            dramaddr = regs.dramaddr[1];
            length = regs.length[1];

            // "Start" DMA, send interrupt to notify CPU
            mi::requestInterrupt(mi::InterruptSource::AI);
        }

        activeDMAs--;

        updateStatus();
    }

    if (length.length != 0) {
        sys::scheduler::addEvent(idDoSample, 0, getAICycles());
    }
}

}
