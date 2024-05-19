/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/pif.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::pif {

// IPL2 and IPL3 seeds
constexpr u32 PIF_SEEDS = 0x3F3F;

namespace PIFRAM {
    enum : u64 {
        Seeds = 0x1FC007E4,
        Status = 0x1FC007FC,
    };
}

namespace Command {
    enum : u32 {
        SendJoyBus = 1 << 0,
        ChallengeResponse = 1 << 1,
        TerminateBoot = 1 << 3,
        LockBootROM = 1 << 4,
        AcquireChecksum = 1 << 5,
        RunChecksum = 1 << 6,
    };
}

void init() {}

void deinit() {}

void reset() {}

u32 read(const u64 paddr) {
    switch (paddr) {
        case PIFRAM::Seeds:
            PLOG_INFO << "Seeds read";

            return PIF_SEEDS;
        case PIFRAM::Status:
            PLOG_INFO << "Status read";

            return 0;
        default:
            PLOG_FATAL << "Unrecognized read (address = " << std::hex << paddr << ")";

            exit(0);
    }
}

void write(const u64 paddr, const u32 data) {
    switch (paddr) {
        case PIFRAM::Status:
            PLOG_INFO << "Status write (data = " << std::hex << data << ")";

            doCommand(data);
            break;
        default:
            PLOG_FATAL << "Unrecognized write (address = " << std::hex << paddr << ", data = " << data << ")";

            exit(0);
    }
}

void doCommand(const u32 command) {
    // Go through the command byte to determine which command(s) to run
    if ((command & Command::SendJoyBus) != 0) {
        PLOG_FATAL << "Unimplemented Send JoyBus command";

        exit(0);
    }

    if ((command & Command::ChallengeResponse) != 0) {
        PLOG_FATAL << "Unimplemented Challenge/Response command";

        exit(0);
    }

    if ((command & Command::TerminateBoot) != 0) {
        PLOG_VERBOSE << "Terminate Boot";
    }

    if ((command & Command::LockBootROM) != 0) {
        PLOG_WARNING << "Lock Boot ROM";
    }

    if ((command & Command::AcquireChecksum) != 0) {
        PLOG_FATAL << "Unimplemented Acquire Checksum command";

        exit(0);
    }

    if ((command & Command::RunChecksum) != 0) {
        PLOG_FATAL << "Unimplemented Run Checksum command";

        exit(0);
    }
}

}
