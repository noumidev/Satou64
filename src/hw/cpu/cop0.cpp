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

// COP0 registers
namespace Register {
    enum : u32 {
        Count = 9,
        Compare = 11,
        Cause = 13,
    };
};

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

struct Registers {
    u32 count;
    u32 compare;
    Cause cause;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
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
        case Register::Cause:
            regs.cause.raw = data;
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
