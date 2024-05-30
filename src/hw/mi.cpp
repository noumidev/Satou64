/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/mi.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/cpu/cop0.hpp"

namespace hw::mi {

constexpr u32 VERSION = 0x2020102;

constexpr u32 INTERRUPT_MASK = 0x3F;

constexpr const char *INTERRUPT_NAMES[InterruptSource::NumberOfInterruptSources] = {
    "SP", "SI", "AI", "VI", "PI", "DP",
};

struct MODE {
    u32 repeatCount;

    bool repeatMode;
    bool ebusMode;
    bool upperMode;
};

union INTERRUPT {
    u32 raw;
    struct {
        u32 spPending : 1;
        u32 siPending : 1;
        u32 aiPending : 1;
        u32 viPending : 1;
        u32 piPending : 1;
        u32 dpPending : 1;
        u32 : 26;
    };
};

union MASK {
    u32 raw;
    struct {
        u32 spEnable : 1;
        u32 siEnable : 1;
        u32 aiEnable : 1;
        u32 viEnable : 1;
        u32 piEnable : 1;
        u32 dpEnable : 1;
        u32 : 26;
    };
};

struct Registers {
    MODE mode;
    INTERRUPT interrupt;
    MASK mask;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        case IORegister::VERSION:
            PLOG_INFO << "VERSION read";

            return VERSION;
        case IORegister::INTERRUPT:
            PLOG_INFO << "INTERRUPT read";

            return regs.interrupt.raw;
        case IORegister::MASK:
            PLOG_INFO << "MASK read";

            return regs.mask.raw;
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

void writeIO(const u64 ioaddr, const u32 data) {
    switch (ioaddr) {
        case IORegister::MODE:
            PLOG_INFO << "MODE write (data = " << std::hex << data << ")";

            regs.mode.repeatCount = (data & 0x3F) + 1;

            // Repeat mode
            switch ((data >> 7) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    regs.mode.repeatMode = false;
                    break;
                case 2:
                    regs.mode.repeatMode = true;

                    PLOG_WARNING << "Repeat mode enabled (repeat count = " << regs.mode.repeatCount << ")";
                    break;
            }

            // EBus mode
            switch ((data >> 9) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    regs.mode.ebusMode = false;
                    break;
                case 2:
                    regs.mode.ebusMode = true;

                    PLOG_WARNING << "EBus mode enabled";
                    break;
            }

            // Upper mode
            switch ((data >> 12) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    regs.mode.upperMode = false;
                    break;
                case 2:
                    regs.mode.upperMode = true;

                    PLOG_WARNING << "Upper mode enabled";
                    break;
            }
            break;
        case IORegister::MASK:
            PLOG_INFO << "MASK write (data = " << std::hex << data << ")";

            switch (data & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "SP interrupt disabled";

                    regs.mask.spEnable = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "SP interrupt enabled";

                    regs.mask.spEnable = 1;
                    break;
            }

            switch ((data >> 2) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "SI interrupt disabled";

                    regs.mask.siEnable = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "SI interrupt enabled";

                    regs.mask.siEnable = 1;
                    break;
            }

            switch ((data >> 4) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "AI interrupt disabled";

                    regs.mask.aiEnable = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "AI interrupt enabled";

                    regs.mask.aiEnable = 1;
                    break;
            }

            switch ((data >> 6) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "VI interrupt disabled";

                    regs.mask.viEnable = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "VI interrupt enabled";

                    regs.mask.viEnable = 1;
                    break;
            }

            switch ((data >> 8) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "PI interrupt disabled";

                    regs.mask.piEnable = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "PI interrupt enabled";

                    regs.mask.piEnable = 1;
                    break;
            }

            switch ((data >> 10) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "DP interrupt disabled";

                    regs.mask.dpEnable = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "DP interrupt enabled";

                    regs.mask.dpEnable = 1;
                    break;
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

void requestInterrupt(const u32 source) {
    PLOG_VERBOSE << INTERRUPT_NAMES[source] << " interrupt requested";

    regs.interrupt.raw |= 1 << source;

    setInterruptPending();
}

void clearInterrupt(const u32 source) {
    // PLOG_VERBOSE << INTERRUPT_NAMES[source] << " interrupt cleared";

    regs.interrupt.raw &= ~(1 << source);

    setInterruptPending();
}

void setInterruptPending() {
    if ((regs.interrupt.raw & regs.mask.raw & INTERRUPT_MASK) != 0) {
        cpu::cop0::setInterruptPending(cpu::cop0::InterruptNumber::External);
    } else {
        cpu::cop0::clearInterruptPending(cpu::cop0::InterruptNumber::External);
    }
}

}
