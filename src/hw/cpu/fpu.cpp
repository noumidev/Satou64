/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/cpu/fpu.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/cpu/cpu.hpp"

namespace hw::cpu::fpu {

namespace RoundingMode {
    enum : u32 {
        Nearest,
        TowardZero,
        TowardPositiveInfnity,
        TowardNegativeInfinity,
        NumberOfRoundingModes,
    };
}

constexpr const char *MODE_NAMES[RoundingMode::NumberOfRoundingModes] = {
    "Nearest",
    "Toward 0",
    "Toward +Inf",
    "Toward -Inf",
};

namespace ControlRegister {
    enum : u32 {
        Control = 31,
    };
}

union Control {
    u32 raw;
    struct {
        u32 roundingMode : 2;
        u32 flag : 5;
        u32 enable : 5;
        u32 cause : 6;
        u32 : 5;
        u32 condition : 1;
        u32 flushEnable : 1;
        u32 : 7;
    };
};

struct Registers {
    Control control;
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

u32 getControl(const u32 idx) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    switch (idx) {
        case ControlRegister::Control:
            return regs.control.raw;
        default:
            PLOG_FATAL << "Unrecognized Control register " << idx;

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

void setControl(const u32 idx, const u32 data) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    switch (idx) {
        case ControlRegister::Control:
            regs.control.raw = data;

            PLOG_VERBOSE << "FPU rounding mode = " << MODE_NAMES[regs.control.roundingMode];
            break;
        default:
            PLOG_FATAL << "Unrecognized Control register " << idx << " (data = " << std::hex << data << ")";

            exit(0);
    }
}

void doSingle(const Instruction instr) {
    const u32 funct = instr.rType.funct;
    switch (funct) {
        default:
            PLOG_FATAL << "Unrecognized FPU opcode " << std::hex << funct << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

            exit(0);
    }
}

}
