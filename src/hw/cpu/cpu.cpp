/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/cpu/cpu.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <string>

#include <plog/Log.h>

#include "sys/memory.hpp"

namespace hw::cpu {

constexpr bool ENABLE_DISASSEMBLER = true;

constexpr u32 ADDR_RESET_VECTOR = 0xBFC00000;
constexpr u32 ADDR_FAST_BOOT = 0xA4000040;

// CPU virtual memory ranges
namespace AddressRangeBase {
    enum : u64 {
        KUSEG = 0,
        KSEG0 = 0x80000000,
        KSEG1 = 0xA0000000,
        KSSEG = 0xC0000000,
        KSEG3 = 0xE0000000,
    };
}

namespace AddressRangeSize {
    enum : u64 {
        KUSEG = 0x80000000,
        KSEG0 = 0x20000000,
        KSEG1 = 0x20000000,
        KSSEG = 0x20000000,
        KSEG3 = 0x20000000,
    };
}

// CPU general-purpose registers
namespace Register {
    enum {
        R0, AT, V0, V1, A0, A1, A2, A3,
        T0, T1, T2, T3, T4, T5, T6, T7,
        S0, S1, S2, S3, S4, S5, S6, S7,
        T8, T9, K0, K1, GP, SP, S8, RA,
        LO, HI,
        NumberOfRegisters,
    };
}

// GPR names
constexpr const char *REG_NAMES[Register::NumberOfRegisters] = {
    "r0", "at", "v0", "v2", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra",
    "lo", "hi",
};

namespace Opcode {
    enum : u32 {
        ORI = 0x0D,
        LUI = 0x0F,
    };
}

enum class ALUOpImm {
    LUI,
    ORI,
};

// CPU instruction
union Instruction {
    u32 raw;

    // Immediate type
    struct {
        u32 immediate : 16;
        u32 rt : 5;
        u32 rs : 5;
        u32 op : 6;
    } iType;

    // Jump type
    struct {
        u32 target : 26;
        u32 op : 6;
    } jType;

    // Register type
    struct {
        u32 funct : 6;
        u32 sa : 5;
        u32 rd : 5;
        u32 rt : 5;
        u32 rs : 5;
        u32 op : 6;
    } rType;
};

// CPU register file
struct RegisterFile {
    // General-purpose registers
    std::array<u64, Register::NumberOfRegisters> regs;

    // Program counters
    u64 pc, npc, cpc;
};

RegisterFile regFile;

void init() {}

void deinit() {}

void run() {}

void reset(const bool isFastBoot) {
    // Clear register file
    std::memset(&regFile, 0, sizeof(RegisterFile));

    // Set program counter
    setPC<false>(ADDR_RESET_VECTOR);
    if (isFastBoot) {
        setPC<false>(ADDR_FAST_BOOT);
    }
}

bool isValidRegisterIndex(const u32 idx) {
    return idx < Register::NumberOfRegisters;
}

u64 get(const u32 idx) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    return regFile.regs[idx];
}

template<bool isCurrentPC>
u64 getPC() {
    if constexpr (isCurrentPC) {
        return regFile.cpc;
    }

    return regFile.pc;
}

template<>
void set(const u32 idx, const u32 data) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    auto &regs = regFile.regs;

    regs[idx] = (i32)data;

    // Hardwired to 0
    regs[Register::R0] = 0;
}

template<>
void set(const u32 idx, const u64 data) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    auto &regs = regFile.regs;

    regs[idx] = data;

    // Hardwired to 0
    regs[Register::R0] = 0;
}

template<bool isBranch>
void setPC(const u64 addr) {
    if constexpr (isBranch) {
        regFile.npc = addr;
    } else {
        regFile.pc = addr;
        regFile.npc = addr + sizeof(Instruction);
    }
}

void advancePC() {
    regFile.pc = regFile.npc;
    regFile.npc += sizeof(Instruction);
}

u64 translateAddress(const u64 vaddr) {
    if ((vaddr < AddressRangeBase::KSEG0) || (vaddr >= AddressRangeBase::KSSEG)) {
        PLOG_FATAL << "Unimplemented access to TLB mapped region (address = " << std::hex << vaddr << ")";

        exit(0);
    }

    return vaddr & (AddressRangeSize::KSEG0 - 1);
}

template<>
u8 read(const u64 vaddr) {
    return sys::memory::read<u8>(translateAddress(vaddr));
}

template<>
u16 read(const u64 vaddr) {
    if (!isAlignedAddress<u16>(vaddr)) {
        PLOG_FATAL << "Unaligned read16 address " << std::hex << vaddr;

        exit(0);
    }

    return sys::memory::read<u16>(translateAddress(vaddr));
}

template<>
u32 read(const u64 vaddr) {
    if (!isAlignedAddress<u32>(vaddr)) {
        PLOG_FATAL << "Unaligned read32 address " << std::hex << vaddr;

        exit(0);
    }

    return sys::memory::read<u32>(translateAddress(vaddr));
}

template<>
u64 read(const u64 vaddr) {
    if (!isAlignedAddress<u64>(vaddr)) {
        PLOG_FATAL << "Unaligned read64 address " << std::hex << vaddr;

        exit(0);
    }

    return sys::memory::read<u64>(translateAddress(vaddr));
}

u32 fetch() {
    const u32 data = read<u32>(getPC<true>());

    advancePC();

    return data;
}

template<>
void write(const u64 vaddr, const u8 data) {
    return sys::memory::write(translateAddress(vaddr), data);
}

template<>
void write(const u64 vaddr, const u16 data) {
    if (!isAlignedAddress<u16>(vaddr)) {
        PLOG_FATAL << "Unaligned write16 address " << std::hex << vaddr;

        exit(0);
    }

    return sys::memory::write(translateAddress(vaddr), data);
}

template<>
void write(const u64 vaddr, const u32 data) {
    if (!isAlignedAddress<u32>(vaddr)) {
        PLOG_FATAL << "Unaligned write32 address " << std::hex << vaddr;

        exit(0);
    }

    return sys::memory::write(translateAddress(vaddr), data);
}

template<>
void write(const u64 vaddr, const u64 data) {
    if (!isAlignedAddress<u64>(vaddr)) {
        PLOG_FATAL << "Unaligned write64 address " << std::hex << vaddr;

        exit(0);
    }

    return sys::memory::write(translateAddress(vaddr), data);
}

template<ALUOpImm op>
void doALUImmediate(const Instruction instr) {
    const u32 rs = instr.iType.rs;
    const u32 rt = instr.iType.rt;

    const u32 imm = instr.iType.immediate;

    switch (op) {
        case ALUOpImm::ORI:
            set(rt, get(rs) | (u64)imm);
            break;
        case ALUOpImm::LUI:
            set(rt, imm << 16);
            break;
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *rsName = REG_NAMES[rs];
        const char *rtName = REG_NAMES[rt];

        const u32 pc = getPC<true>();

        switch (op) {
            case ALUOpImm::LUI:
                std::printf("[%08X:%08X] lui %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, imm, rtName, get(rt));
                break;
            case ALUOpImm::ORI:
                std::printf("[%08X:%08X] ori %s, %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, rsName, imm, rtName, get(rt));
                break;
        }
    }
}


void doInstruction() {
    Instruction instr;
    instr.raw = fetch();

    const u32 op = instr.iType.op;
    switch (op) {
        case Opcode::ORI:
            doALUImmediate<ALUOpImm::ORI>(instr);
            break;
        case Opcode::LUI:
            doALUImmediate<ALUOpImm::LUI>(instr);
            break;
        default:
            PLOG_FATAL << "Unrecognized opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

            exit(0);
    }
}

void run(const i64 cycles) {
    for (i64 i = 0; i < cycles; i++) {
        // Set current PC
        regFile.cpc = getPC<false>();

        doInstruction();
    }
}

}
