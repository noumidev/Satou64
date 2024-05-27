/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/cpu/cop0.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/cpu/cpu.hpp"

namespace hw::cpu::cop0 {

constexpr u32 CONFIG_DEFAULT = 0x6E460;

// COP0 registers
namespace Register {
    enum : u32 {
        Count = 9,
        Compare = 11,
        Status = 12,
        Cause = 13,
        Config = 16,
        TagLo = 28,
        TagHi = 29,
    };
};

// Register write masks
namespace WriteMask {
    enum : u32 {
        Config = 0xF00800F,
    };
}

namespace CPUMode {
    enum : u32 {
        Kernel = 0,
        Supervisor = 1,
        User = 2,
    };
}

// Cause register
union Cause {
    u32 raw;
    struct {
        u32 : 2;
        u32 exceptionCode : 5;
        u32 : 1;
        u32 interruptPending : 8;
        u32 : 12;
        u32 coprocessorError : 2;
        u32 : 1;
        u32 branchDelay : 1;
    };
};

// Config register
union Config {
    u32 raw;
    struct {
        u32 coherencyAlgorithm : 3;
        u32 cu : 1;
        u32 : 11;
        u32 bigEndian : 1;
        u32 : 8;
        u32 dataPattern : 4;
        u32 frequencyRatio : 3;
        u32 : 1;
    };
};

// Status register
union Status {
    u32 raw;
    struct {
        u32 interruptEnable : 1;
        u32 exceptionLevel : 1;
        u32 errorLevel : 1;
        u32 mode : 2;
        u32 ux : 1;
        u32 sx : 1;
        u32 kx : 1;
        u32 interruptMask : 8;
        u32 de : 1;
        u32 ce : 1;
        u32 condition : 1;
        u32 : 1;
        u32 softReset : 1;
        u32 tlbShutdown : 1;
        u32 bootExceptionVectors : 1;
        u32 : 0;
        u32 instructionTraceEnable : 1;
        u32 reverseEndian : 1;
        u32 fr : 1;
        u32 lowPower : 1;
        u32 coprocessorUsable : 4;
    };
};

struct Registers {
    u32 count;
    u32 compare;
    Status status;
    Cause cause;
    Config config;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));

    regs.status.mode = CPUMode::Kernel;
    regs.status.bootExceptionVectors = 1;

    regs.config.raw = CONFIG_DEFAULT;
}

template<>
u32 get(const u32 idx) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    switch (idx) {
        default:
            PLOG_FATAL << "Unrecognized get32 register " << idx;

            exit(0);
    }
}

template<>
u64 get(const u32 idx) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    switch (idx) {
        default:
            PLOG_FATAL << "Unrecognized get64 register " << idx;

            exit(0);
    }
}

template<>
void set(const u32 idx, const u32 data) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    switch (idx) {
        case Register::Count:
            regs.count = data;
            break;
        case Register::Compare:
            regs.compare = data;
            break;
        case Register::Status:
            regs.status.raw = data;
            break;
        case Register::Cause:
            PLOG_WARNING << "Cause write (data = " << std::hex << data << ")";
            break;
        case Register::Config:
            regs.config.raw = (data & WriteMask::Config) | (regs.config.raw & ~WriteMask::Config);
            break;
        case Register::TagLo:
            PLOG_WARNING << "TagLo write (data = " << std::hex << data << ")";
            break;
        case Register::TagHi:
            PLOG_WARNING << "TagHi write (data = " << std::hex << data << ")";
            break;
        default:
            PLOG_FATAL << "Unrecognized set32 register " << idx << " (data = " << std::hex << data << ")";

            exit(0);
    }
}

template<>
void set(const u32 idx, const u64 data) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    switch (idx) {
        default:
            PLOG_FATAL << "Unrecognized set64 register " << idx << " (data = " << std::hex << data << ")";

            exit(0);
    }
}

}
