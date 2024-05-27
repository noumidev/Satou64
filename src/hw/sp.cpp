/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/sp.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::sp {

constexpr u32 SIG_NUM = 8;

union STATUS {
    u32 raw;
    struct {
        u32 halted : 1;
        u32 broke : 1;
        u32 dmaBusy : 1;
        u32 dmaFull : 1;
        u32 ioBusy : 1;
        u32 singleStep : 1;
        u32 interruptOnBreak : 1;
        u32 sig : 8;
        u32 : 17;
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

    regs.status.halted = 1;
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        case IORegister::STATUS:
            PLOG_INFO << "STATUS read";

            return regs.status.raw;
        case IORegister::DMABUSY:
            PLOG_INFO << "DMABUSY read";

            return regs.status.dmaBusy;
        case IORegister::PC:
            PLOG_WARNING << "PC read";

            return 0;
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

void writeIO(const u64 ioaddr, const u32 data) {
    switch (ioaddr) {
        case IORegister::STATUS:
            PLOG_INFO << "STATUS write (data = " << std::hex << data << ")";

            switch (data & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_FATAL << "Unimplemented RSP code execution";

                    exit(0);
                case 2:
                    PLOG_VERBOSE << "RSP halted";

                    regs.status.halted = 1;
                    break;
            }

            if ((data & (1 << 2)) != 0) {
                PLOG_VERBOSE << "BREAK flag cleared";

                regs.status.broke = 0;
            }

            // TODO: implement RSP interrupt flag
            switch ((data >> 3) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_WARNING << "Interrupt flag cleared";
                    break;
                case 2:
                    PLOG_FATAL << "Unimplemented RSP interrupt";

                    exit(0);
            }

            switch ((data >> 5) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "Single step disabled";

                    regs.status.singleStep = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "Single step enabled";

                    regs.status.singleStep = 1;
                    break;
            }

            switch ((data >> 7) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "Interrupt on BREAK disabled";

                    regs.status.interruptOnBreak = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "Interrupt on BREAK enabled";

                    regs.status.interruptOnBreak = 1;
                    break;
            }

            // Signal
            for (u32 signal = 0; signal < SIG_NUM; signal++) {
                switch ((data >> (9 + 2 * signal)) & 3) {
                    // No change
                    case 0:
                    case 3:
                        break;
                    case 1:
                        PLOG_VERBOSE << "Signal " << signal << " disabled";

                        regs.status.sig &= ~(1 << signal);
                        break;
                    case 2:
                        PLOG_VERBOSE << "Signal " << signal << " enabled";

                        regs.status.sig |= 1 << signal;
                        break;
                }
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
