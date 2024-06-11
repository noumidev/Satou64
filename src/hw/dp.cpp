/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/dp.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/rdp/rdp.hpp"

namespace hw::dp {

union START {
    u32 raw;
    struct {
        u32 addr : 24;
        u32 : 8;
    };
};

union END {
    u32 raw;
    struct {
        u32 addr : 24;
        u32 : 8;
    };
};

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
    START start;
    END end;
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
        case IORegister::END:
            PLOG_INFO << "END read";

            return regs.end.raw;
        case IORegister::CURRENT:
            PLOG_INFO << "CURRENT read";

            // Our RDP DMA is instant, CURRENT = END!
            return regs.end.raw;
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
        case IORegister::START:
            PLOG_INFO << "START write (data = " << std::hex << data << ")";

            regs.start.raw = data;
            break;
        case IORegister::END:
            PLOG_INFO << "END write (data = " << std::hex << data << ")";

            regs.end.raw = data;

            regs.start.raw = rdp::processCommandList(regs.start.addr, regs.end.addr);
            break;
        case IORegister::STATUS:
            PLOG_INFO << "STATUS write (data = " << std::hex << data << ")";

            switch (data & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "XBUS disabled";

                    regs.status.xbus = 0;
                    break;
                case 2:
                    PLOG_WARNING << "XBUS enabled";

                    regs.status.xbus = 1;
                    break;
            }

            switch ((data >> 2) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "FREEZE cleared";

                    regs.status.freeze = 0;
                    break;
                case 2:
                    PLOG_WARNING << "FREEZE set";

                    regs.status.freeze = 1;
                    break;
            }

            switch ((data >> 4) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "FLUSH cleared";

                    regs.status.flush = 0;
                    break;
                case 2:
                    PLOG_WARNING << "FLUSH set";

                    regs.status.flush = 1;
                    break;
            }

            if ((data & (1 << 6)) != 0) {
                PLOG_VERBOSE << "TMEM busy cleared";

                regs.status.tmemBusy = 0;
            }

            if ((data & (1 << 7)) != 0) {
                PLOG_VERBOSE << "Pipe busy cleared";

                regs.status.pipeBusy = 0;
            }

            if ((data & (1 << 8)) != 0) {
                PLOG_VERBOSE << "Buffer busy cleared";

                regs.status.bufBusy = 0;
            }

            if ((data & (1 << 9)) != 0) {
                PLOG_WARNING << "CLOCK cleared";
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
