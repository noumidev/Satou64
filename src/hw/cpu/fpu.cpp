/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/cpu/fpu.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/cpu/cop0.hpp"
#include "hw/cpu/cpu.hpp"

namespace hw::cpu::fpu {

constexpr bool ENABLE_DISASSEMBLER = true;

constexpr u64 FPR_NUM = 32;
constexpr u64 FPR_MASK = FPR_NUM - 2;

namespace Format {
    enum {
        Single,
        Double,
        Word,
        Long,
        NumberOfFormats,
    };
}

constexpr const char *FORMAT_NAMES[Format::NumberOfFormats] = {
    "Single", "Double", "Word", "Long",
};

constexpr char FORMAT_CHARS[Format::NumberOfFormats] = {
    's', 'd', 'w', 'l',
};

namespace CompareCondition {
    enum : u32 {
        F = 0x0,
        UN = 0x1,
        EQ = 0x2,
        UEQ = 0x3,
        OLT = 0x4,
        ULT = 0x5,
        OLE = 0x6,
        ULE = 0x7,
        SF = 0x8,
        NGLE = 0x9,
        SEQ = 0xA,
        NGL = 0xB,
        LT = 0xC,
        NGE = 0xD,
        LE = 0xE,
        NGT = 0xF,
        NumberOfCompareConditions,
    };
}

constexpr const char *CONDITION_NAMES[CompareCondition::NumberOfCompareConditions] = {
    "f", "un", "eq", "ueq", "olt", "ult", "ole", "ule", "sf", "ngle", "seq", "ngl", "lt", "nge", "le", "ngt",
};

namespace CompareConditionBit {
    enum : u32 {
        LessThan = 1 << 0,
        Equal = 1 << 1,
        Unordered = 1 << 2,
        Signaling = 1 << 3,
    };
}

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

namespace Opcode {
    enum : u32 {
        ADD = 0x00,
        DIV = 0x03,
        TRUNCW = 0x0D,
        CVTS = 0x20,
        CVTD = 0x21,
        CCOND = 0x30,
    };
}

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
    u64 fprs[FPR_NUM];

    Control control;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

bool getCondition() {
    return regs.control.condition != 0;
}

f32 makeSingle(const u32 data) {
    f32 single;
    std::memcpy(&single, &data, sizeof(f32));

    return single;
}

f64 makeDouble(const u64 data) {
    f64 double_;
    std::memcpy(&double_, &data, sizeof(f64));

    return double_;
}

u32 makeWord(const f32 data) {
    u32 word;
    std::memcpy(&word, &data, sizeof(u32));

    return word;
}

u64 makeLong(const f64 data) {
    u64 long_;
    std::memcpy(&long_, &data, sizeof(u64));

    return long_;
}

template<>
u32 get(const u32 idx) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    if (cop0::isLargeFPURegisterFile()) {
        return (u32)regs.fprs[idx];
    } else {
        if ((idx & 1) != 0) {
            return (u32)(regs.fprs[idx & FPR_MASK] >> 32);
        } else {
            return (u32)regs.fprs[idx];
        }
    }
}

template<>
u64 get(const u32 idx) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }
    
    if (cop0::isLargeFPURegisterFile()) {
        return regs.fprs[idx];
    }

    return regs.fprs[idx & FPR_MASK];
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

    if (cop0::isLargeFPURegisterFile()) {
        regs.fprs[idx] &= 0xFFFFFFFF00000000ULL;
        regs.fprs[idx] |= (u64)data;
    } else {
        if ((idx & 1) != 0) {
            regs.fprs[idx & FPR_MASK] &= 0xFFFFFFFFULL;
            regs.fprs[idx & FPR_MASK] |= (u64)data << 32;
        } else {
            regs.fprs[idx] &= 0xFFFFFFFF00000000ULL;
            regs.fprs[idx] |= (u64)data;
        }
    }
}

template<>
void set(const u32 idx, const u64 data) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    if (cop0::isLargeFPURegisterFile()) {
        regs.fprs[idx] = data;
    } else {
        regs.fprs[idx & FPR_MASK] = data;
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

template<int format>
void ADD(const Instruction instr) {
    if ((format != Format::Single) && (format != Format::Double)) {
        PLOG_FATAL << "Invalid format for ADD";

        exit(0);
    }

    const u32 fd = instr.fType.fd;
    const u32 fs = instr.fType.fs;
    const u32 ft = instr.fType.ft;

    switch (format) {
        case Format::Single:
            set(fd, makeWord(makeSingle(get<u32>(fs)) + makeSingle(get<u32>(ft))));
            break;
        default:
            PLOG_FATAL << "Unimplemented format " << FORMAT_NAMES[format];

            exit(0);
    }
    
    if constexpr (ENABLE_DISASSEMBLER) {
        const u32 pc = getPC<true>();

        const f32 single = makeSingle(get<u32>(fd));

        std::printf("[%08X:%08X] add.%c %u, %u, %u; %u = %f\n", pc, instr.raw, FORMAT_CHARS[format], fd, fs, ft, fd, single);
    }
}

template<int format>
void CCOND(const Instruction instr) {
    if ((format != Format::Single) && (format != Format::Double)) {
        PLOG_FATAL << "Invalid format for C.COND";

        exit(0);
    }

    const u32 fs = instr.fType.fs;
    const u32 ft = instr.fType.ft;

    f64 fsData, ftData;
    switch (format) {
        case Format::Single:
            fsData = (f64)makeSingle(get<u32>(fs));
            ftData = (f64)makeSingle(get<u32>(ft));
            break;
        case Format::Double:
            fsData = makeDouble(get<u64>(fs));
            ftData = makeDouble(get<u64>(ft));
            break;
    }

    const u32 condition = instr.fType.funct & 0xF;
    u32 flags = 0;

    if (std::isnan(fsData) || std::isnan(ftData)) {
        if ((condition & CompareConditionBit::Signaling) != 0) {
            PLOG_WARNING << "Unhandled Invalid Operation exception";
        }

        flags |= CompareConditionBit::Unordered;
    } else {
        if (fsData < ftData) {
            flags |= CompareConditionBit::LessThan;
        }

        if (fsData == ftData) {
            flags |= CompareConditionBit::Equal;
        }
    }

    regs.control.condition = (condition & flags) != 0;

    if constexpr (ENABLE_DISASSEMBLER) {
        const u32 pc = getPC<true>();

        std::printf("[%08X:%08X] c.%s.%c %u, %u; %u = %lf, %u = %lf, COND = %u\n", pc, instr.raw, CONDITION_NAMES[condition], FORMAT_CHARS[format], fs, ft, fs, fsData, ft, ftData, regs.control.condition);
    }
}

template<int format>
void CVTD(const Instruction instr) {
    if (format == Format::Double) {
        PLOG_FATAL << "Invalid format for CVT.D";

        exit(0);
    }

    const u32 fd = instr.fType.fd;
    const u32 fs = instr.fType.fs;

    u64 data;
    switch (format) {
        case Format::Word:
            data = makeLong((f64)get<u32>(fs));
            break;
        default:
            PLOG_FATAL << "Unimplemented format " << FORMAT_NAMES[format];

            exit(0);
    }

    set<u64>(fd, data);
    
    if constexpr (ENABLE_DISASSEMBLER) {
        const u32 pc = getPC<true>();

        const f64 double_ = makeDouble(data);

        std::printf("[%08X:%08X] cvt.d.%c %u, %u; %u = %lf\n", pc, instr.raw, FORMAT_CHARS[format], fd, fs, fd, double_);
    }
}

template<int format>
void CVTS(const Instruction instr) {
    if (format == Format::Single) {
        PLOG_FATAL << "Invalid format for CVT.S";

        exit(0);
    }

    const u32 fd = instr.fType.fd;
    const u32 fs = instr.fType.fs;

    u32 data;
    switch (format) {
        case Format::Double:
            data = makeWord((f32)makeDouble(get<u64>(fs)));
            break;
        case Format::Word:
            data = makeWord((f32)get<u32>(fs));
            break;
        default:
            PLOG_FATAL << "Unimplemented format " << FORMAT_NAMES[format];

            exit(0);
    }

    set<u32>(fd, data);
    
    if constexpr (ENABLE_DISASSEMBLER) {
        const u32 pc = getPC<true>();

        const f32 single = makeSingle(data);

        std::printf("[%08X:%08X] cvt.s.%c %u, %u; %u = %f\n", pc, instr.raw, FORMAT_CHARS[format], fd, fs, fd, single);
    }
}

template<int format>
void DIV(const Instruction instr) {
    if ((format != Format::Single) && (format != Format::Double)) {
        PLOG_FATAL << "Invalid format for DIV";

        exit(0);
    }

    const u32 fd = instr.fType.fd;
    const u32 fs = instr.fType.fs;
    const u32 ft = instr.fType.ft;

    switch (format) {
        case Format::Single:
            set(fd, makeWord(makeSingle(get<u32>(fs)) / makeSingle(get<u32>(ft))));
            break;
        default:
            PLOG_FATAL << "Unimplemented format " << FORMAT_NAMES[format];

            exit(0);
    }
    
    if constexpr (ENABLE_DISASSEMBLER) {
        const u32 pc = getPC<true>();

        const f32 single = makeSingle(get<u32>(fd));

        std::printf("[%08X:%08X] div.%c %u, %u, %u; %u = %f\n", pc, instr.raw, FORMAT_CHARS[format], fd, fs, ft, fd, single);
    }
}

template<int format>
void TRUNCW(const Instruction instr) {
    if ((format != Format::Single) && (format != Format::Double)) {
        PLOG_FATAL << "Invalid format for TRUNC.W";

        exit(0);
    }

    const u32 fd = instr.fType.fd;
    const u32 fs = instr.fType.fs;

    u32 data;
    switch (format) {
        case Format::Single:
            data = (u32)std::trunc(makeSingle(get<u32>(fs)));
            break;
        case Format::Double:
            data = (u32)std::trunc(makeDouble(get<u64>(fs)));
            break;
        default:
            PLOG_FATAL << "Unimplemented format " << FORMAT_NAMES[format];

            exit(0);
    }

    set<u32>(fd, data);
    
    if constexpr (ENABLE_DISASSEMBLER) {
        const u32 pc = getPC<true>();

        std::printf("[%08X:%08X] trunc.w.%c %u, %u; %u = %08X\n", pc, instr.raw, FORMAT_CHARS[format], fd, fs, fd, data);
    }
}

void doSingle(const Instruction instr) {
    const u32 funct = instr.rType.funct;
    switch (funct) {
        case Opcode::ADD:
            return ADD<Format::Single>(instr);
        case Opcode::DIV:
            return DIV<Format::Single>(instr);
        case Opcode::TRUNCW:
            return TRUNCW<Format::Single>(instr);
        default:
            if (funct >= Opcode::CCOND) {
                return CCOND<Format::Single>(instr);
            }

            PLOG_FATAL << "Unrecognized SINGLE opcode " << std::hex << funct << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

            exit(0);
    }
}

void doDouble(const Instruction instr) {
    const u32 funct = instr.rType.funct;
    switch (funct) {
        case Opcode::CVTS:
            return CVTS<Format::Double>(instr);
        default:
            PLOG_FATAL << "Unrecognized DOUBLE opcode " << std::hex << funct << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

            exit(0);
    }
}

void doWord(const Instruction instr) {
    const u32 funct = instr.rType.funct;
    switch (funct) {
        case Opcode::CVTS:
            return CVTS<Format::Word>(instr);
        case Opcode::CVTD:
            return CVTD<Format::Word>(instr);
        default:
            PLOG_FATAL << "Unrecognized WORD opcode " << std::hex << funct << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

            exit(0);
    }
}

}
