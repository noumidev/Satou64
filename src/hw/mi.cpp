/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/mi.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::mi {

struct MODE {
    u32 repeatCount;

    bool repeatMode;
    bool ebusMode;
    bool upperMode;
};

struct Registers {
    MODE mode;
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
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
