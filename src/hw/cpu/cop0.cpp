/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/cpu/cop0.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/cpu/cpu.hpp"

namespace hw::cpu::cop0 {

constexpr bool ENABLE_DISASSEMBLER = true;

constexpr u32 CONFIG_DEFAULT = 0x6E460;

// COP0 registers
namespace Register {
    enum : u32 {
        Index = 0,
        EntryLo0 = 2,
        EntryLo1 = 3,
        PageMask = 5,
        Count = 9,
        EntryHi = 10,
        Compare = 11,
        Status = 12,
        Cause = 13,
        EPC = 14,
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

namespace Opcode {
    enum : u32 {
        TLBWI = 0x02,
        ERET = 0x18,
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
    u64 epc;
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

bool isCoprocessorUsable(const u32 coprocessor) {
    // COP0 is always usable in Kernel mode
    if ((coprocessor == 0) && (regs.status.mode == CPUMode::Kernel)) {
        return true;
    }

    return (regs.status.ce & (1 << coprocessor)) != 0;
}

bool isLargeFPURegisterFile() {
    return regs.status.fr != 0;
}

template<>
u32 get(const u32 idx) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    switch (idx) {
        case Register::Count:
            return regs.count;
        case Register::EntryHi:
            PLOG_WARNING << "EntryHi read";

            return 0;
        case Register::Status:
            return regs.status.raw;
        case Register::Cause:
            return regs.cause.raw;
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
        case Register::Index:
            PLOG_WARNING << "Index write (data = " << std::hex << data << ")";
            break;
        case Register::EntryLo0:
            PLOG_WARNING << "EntryLo0 write (data = " << std::hex << data << ")";
            break;
        case Register::EntryLo1:
            PLOG_WARNING << "EntryLo1 write (data = " << std::hex << data << ")";
            break;
        case Register::PageMask:
            PLOG_WARNING << "PageMask write (data = " << std::hex << data << ")";
            break;
        case Register::Count:
            regs.count = data;
            break;
        case Register::EntryHi:
            PLOG_WARNING << "EntryHi write (data = " << std::hex << data << ")";
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
        case Register::EPC:
            regs.epc = (i32)data;
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

void doInstruction(const Instruction instr) {
    const u32 funct = instr.rType.funct;
    if constexpr (ENABLE_DISASSEMBLER) {
        const u32 pc = getPC<true>();
        switch (funct) {
            case Opcode::TLBWI:
                break;
            case Opcode::ERET:
                std::printf("[%08X:%08X] eret\n", pc, instr.raw);
                break;
            default:
                PLOG_FATAL << "Unrecognized System Control opcode " << std::hex << funct << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

                exit(0);
        }
    }

    switch (funct) {
        case Opcode::TLBWI:
            PLOG_WARNING << "TLBWI instruction";
            break;
        case Opcode::ERET:
            if (regs.status.errorLevel != 0) {
                PLOG_FATAL << "Unimplented return from Error";

                exit(0);
            } else {
                regs.status.exceptionLevel = 0;

                setPC<false>(regs.epc);
            }

            // TODO: clear Load Linked bit
            break;
        default:
            PLOG_FATAL << "Unrecognized System Control opcode " << std::hex << funct << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

            exit(0);
    }
}

}
