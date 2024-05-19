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

}
