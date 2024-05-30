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

#include "hw/cpu/cop0.hpp"
#include "hw/cpu/fpu.hpp"

#include "sys/memory.hpp"

namespace hw::cpu {

constexpr bool ENABLE_DISASSEMBLER = false;

constexpr u32 ADDR_RESET_VECTOR = 0xBFC00000;

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

namespace Coprocessor {
    enum {
        SystemControl,
        FPU,
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
        SPECIAL = 0x00,
        REGIMM = 0x01,
        J = 0x02,
        JAL = 0x03,
        BEQ = 0x04,
        BNE = 0x05,
        BLEZ = 0x06,
        BGTZ = 0x07,
        ADDI = 0x08,
        ADDIU = 0x09,
        SLTI = 0x0A,
        SLTIU = 0x0B,
        ANDI = 0x0C,
        ORI = 0x0D,
        XORI = 0x0E,
        LUI = 0x0F,
        COP0 = 0x10,
        COP1 = 0x11,
        BEQL = 0x14,
        BNEL = 0x15,
        BLEZL = 0x16,
        DADDI = 0x18,
        DADDIU = 0x19,
        LB = 0x20,
        LH = 0x21,
        LW = 0x23,
        LBU = 0x24,
        LHU = 0x25,
        LWU = 0x27,
        SB = 0x28,
        SH = 0x29,
        SW = 0x2B,
        CACHE = 0x2F,
        LWC1 = 0x31,
        LD = 0x37,
        SD = 0x3F,
    };
}

namespace RegimmOpcode {
    enum : u32 {
        BGEZ = 0x01,
        BGEZL = 0x03,
        BGEZAL = 0x11,
    };
}

namespace SpecialOpcode {
    enum : u32 {
        SLL = 0x00,
        SRL = 0x02,
        SRA = 0x03,
        SLLV = 0x04,
        SRLV = 0x06,
        SRAV = 0x07,
        JR = 0x08,
        JALR = 0x09,
        MFHI = 0x10,
        MTHI = 0x11,
        MFLO = 0x12,
        MTLO = 0x13,
        DSLLV = 0x14,
        MULT = 0x18,
        MULTU = 0x19,
        DIV = 0x1A,
        DIVU = 0x1B,
        ADD = 0x20,
        ADDU = 0x21,
        SUBU = 0x23,
        AND = 0x24,
        OR = 0x25,
        XOR = 0x26,
        NOR = 0x27,
        SLT = 0x2A,
        SLTU = 0x2B,
        DSLL = 0x38,
        DSLL32 = 0x3C,
        DSRA32 = 0x3F,
    };
};

namespace CoprocessorOpcode {
    enum : u32 {
        MF = 0x00,
        CF = 0x02,
        MT = 0x04,
        CT = 0x06,
        BC = 0x08,
        CO = 0x10,
        SINGLE = 0x10,
        DOUBLE = 0x11,
        WORD = 0x14,
    };
}

namespace CoprocessorBranchOpcode {
    enum : u32 {
        BCTL = 3,
    };
}

enum class ALUOpImm {
    ADDI,
    ADDIU,
    ANDI,
    DADDI,
    DADDIU,
    LUI,
    ORI,
    SLTI,
    SLTIU,
    XORI,
};

enum class ALUOpReg {
    ADD,
    ADDU,
    AND,
    DIV,
    DIVU,
    DSLL,
    DSLLV,
    DSLL32,
    DSRA32,
    MFHI,
    MFLO,
    MTHI,
    MTLO,
    MULT,
    MULTU,
    NOR,
    OR,
    SLL,
    SLLV,
    SLT,
    SLTU,
    SRA,
    SRAV,
    SRL,
    SRLV,
    SUBU,
    XOR,
};

enum class BranchOp {
    BC1TL,
    BEQ,
    BEQL,
    BLEZ,
    BLEZL,
    BGEZ,
    BGEZL,
    BGEZAL,
    BGTZ,
    BNE,
    BNEL,
};

enum class JumpOp {
    J,
    JAL,
    JALR,
    JR,
};

enum class LoadStoreOp {
    LB,
    LBU,
    LD,
    LH,
    LHU,
    LW,
    LWC1,
    LWU,
    SB,
    SD,
    SH,
    SW,
};

// CPU register file
struct RegisterFile {
    // General-purpose registers
    std::array<u64, Register::NumberOfRegisters> regs;

    // Program counters
    u64 pc, npc, cpc;
};

RegisterFile regFile;

bool inDelaySlot[2];

void init() {
    cop0::init();
}

void deinit() {
    cop0::deinit();
}

void reset() {
    cop0::reset();

    // Clear register file
    std::memset(&regFile, 0, sizeof(RegisterFile));

    // Set program counter
    setPC<false>(ADDR_RESET_VECTOR);

    inDelaySlot[0] = inDelaySlot[1] = false;
}

void raiseException(const u32 exceptionCode) {
    PLOG_VERBOSE << "Exception raised (exception code = " << std::hex << exceptionCode << ")";

    cop0::setExceptionCode(exceptionCode);

    u64 vectorBase = 0xFFFFFFFF80000180;
    if (cop0::getBootExceptionVectors()) {
        PLOG_FATAL << "Unimplemented boot exception vectors";

        exit(0);
    }

    if (!cop0::getExceptionLevel()) {
        if (inDelaySlot[0]) {
            cop0::setExceptionPC(regFile.cpc - 4);
            cop0::setBranchDelay();
        } else {
            cop0::setExceptionPC(regFile.cpc);
        }
    }

    cop0::setExceptionLevel();

    setPC<false>(vectorBase);
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

        inDelaySlot[0] = inDelaySlot[1] = false;
    }
}

void branch(const u64 target, const bool condition, const u32 linkReg, const bool isLikely) {
    if (inDelaySlot[0]) {
        PLOG_FATAL << "Branch instruction in delay slot";

        exit(0);
    }

    // Save return address
    set(linkReg, regFile.npc);

    // We're in a delay slot now
    inDelaySlot[1] = true;

    if (condition) {
        setPC<true>(target);
    } else if (isLikely) {
        // Skip delay slot
        setPC<false>(regFile.npc);

        inDelaySlot[1] = false;
    }
}

void advanceDelaySlot() {
    inDelaySlot[0] = inDelaySlot[1];
    inDelaySlot[1] = false;
}

void advancePC() {
    regFile.pc = regFile.npc;
    regFile.npc += sizeof(Instruction);
}

u64 translateAddress(const u64 vaddr) {
    const u32 vaddrMasked = vaddr;
    if ((vaddrMasked < AddressRangeBase::KSEG0) || (vaddrMasked >= AddressRangeBase::KSSEG)) {
        PLOG_FATAL << "Unimplemented access to TLB mapped region (address = " << std::hex << vaddrMasked << ")";

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

    const u64 rsData = get(rs);

    switch (op) {
        case ALUOpImm::ADDI:
            // TODO: overflow check

            set(rt, (u32)(rsData + (u64)(i16)imm));
            break;
        case ALUOpImm::ADDIU:
            set(rt, (u32)(rsData + (u64)(i16)imm));
            break;
        case ALUOpImm::ANDI:
            set(rt, rsData & (u64)imm);
            break;
        case ALUOpImm::DADDI:
            // TODO: overflow check

            set(rt, rsData + (u64)(i16)imm);
            break;
        case ALUOpImm::DADDIU:
            set(rt, rsData + (u64)(i16)imm);
            break;
        case ALUOpImm::LUI:
            set(rt, imm << 16);
            break;
        case ALUOpImm::ORI:
            set(rt, rsData | (u64)imm);
            break;
        case ALUOpImm::SLTI:
            set(rt, (u64)((i64)rsData < (i64)(i16)imm));
            break;
        case ALUOpImm::SLTIU:
            set(rt, (u64)(rsData < (u64)(i16)imm));
            break;
        case ALUOpImm::XORI:
            set(rt, rsData ^ (u64)imm);
            break;
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *rsName = REG_NAMES[rs];
        const char *rtName = REG_NAMES[rt];

        const u32 pc = getPC<true>();

        const u64 rtData = get(rt);

        switch (op) {
            case ALUOpImm::ADDI:
                std::printf("[%08X:%08X] addiu %s, %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
            case ALUOpImm::ADDIU:
                std::printf("[%08X:%08X] addiu %s, %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
            case ALUOpImm::ANDI:
                std::printf("[%08X:%08X] andi %s, %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
            case ALUOpImm::DADDI:
                std::printf("[%08X:%08X] daddi %s, %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
            case ALUOpImm::DADDIU:
                std::printf("[%08X:%08X] daddiu %s, %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
            case ALUOpImm::LUI:
                std::printf("[%08X:%08X] lui %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, imm, rtName, rtData);
                break;
            case ALUOpImm::ORI:
                std::printf("[%08X:%08X] ori %s, %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
            case ALUOpImm::SLTI:
                std::printf("[%08X:%08X] slti %s, %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
            case ALUOpImm::SLTIU:
                std::printf("[%08X:%08X] sltiu %s, %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
            case ALUOpImm::XORI:
                std::printf("[%08X:%08X] xori %s, %s, %04X; %s = %016llX\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
        }
    }
}

template<ALUOpReg op>
void doALURegister(const Instruction instr) {
    const u32 rd = instr.rType.rd;
    const u32 rs = instr.rType.rs;
    const u32 rt = instr.rType.rt;

    const u32 sa = instr.rType.sa;

    const u64 rsData = get(rs);
    const u64 rtData = get(rt);

    switch (op) {
        case ALUOpReg::ADD:
            // TODO: overflow check
            set(rd, (u32)(rsData + rtData));
            break;
        case ALUOpReg::ADDU:
            set(rd, (u32)(rsData + rtData));
            break;
        case ALUOpReg::AND:
            set(rd, rsData & rtData);
            break;
        case ALUOpReg::DIV:
            {
                const i32 n = (i32)rsData;
                const i32 d = (i32)rtData;

                if (d == 0) {
                    PLOG_ERROR << "DIVU by zero";

                    if (n < 0) {
                        set(Register::LO, (u32)1);
                    } else {
                        set(Register::LO, (u32)-1);
                    }

                    set(Register::HI, (u32)n);
                } else if ((n == (1 << 31)) && (d == -1)) {
                    set(Register::LO, 1U << 31);
                    set(Register::HI, (u32)0);
                } else {
                    set(Register::LO, (u32)(n / d));
                    set(Register::HI, (u32)(n % d));
                }
            }
            break;
        case ALUOpReg::DIVU:
            {
                const u32 n = (u32)rsData;
                const u32 d = (u32)rtData;

                if (d == 0) {
                    PLOG_ERROR << "DIVU by zero";

                    set(Register::LO, (u32)-1);
                    set(Register::HI, n);
                } else {
                    set(Register::LO, n / d);
                    set(Register::HI, n % d);
                }
            }
            break;
        case ALUOpReg::DSLL:
            set(rd, rtData << sa);
            break;
        case ALUOpReg::DSLLV:
            set(rd, rtData << (rsData & 0x3F));
            break;
        case ALUOpReg::DSLL32:
            set(rd, rtData << (sa + 32));
            break;
        case ALUOpReg::DSRA32:
            set(rd, (u64)((i64)rtData >> (sa + 32)));
            break;
        case ALUOpReg::MFHI:
            set(rd, get(Register::HI));
            break;
        case ALUOpReg::MFLO:
            set(rd, get(Register::LO));
            break;
        case ALUOpReg::MTHI:
            set(Register::HI, rtData);
            break;
        case ALUOpReg::MTLO:
            set(Register::LO, rsData);
            break;
        case ALUOpReg::MULT:
            {
                const u64 res = (u64)(i32)rsData * (u64)(i32)rtData;

                set(Register::LO, (u32)res);
                set(Register::HI, (u32)(res >> 32));
            }
            break;
        case ALUOpReg::MULTU:
            {
                const u64 res = (u64)(u32)rsData * (u64)(u32)rtData;

                set(Register::LO, (u32)res);
                set(Register::HI, (u32)(res >> 32));
            }
            break;
        case ALUOpReg::NOR:
            set(rd, ~(rsData | rtData));
            break;
        case ALUOpReg::OR:
            set(rd, rsData | rtData);
            break;
        case ALUOpReg::SLL:
            set(rd, (u32)(rtData << sa));
            break;
        case ALUOpReg::SLLV:
            set(rd, (u32)(rtData << (rsData & 0x1F)));
            break;
        case ALUOpReg::SLT:
            set(rd, (u64)((i64)rsData < (i64)rtData));
            break;
        case ALUOpReg::SLTU:
            set(rd, (u64)(rsData < rtData));
            break;
        case ALUOpReg::SRA:
            set(rd, (u32)((i64)rtData >> sa));
            break;
        case ALUOpReg::SRAV:
            set(rd, (u32)((i64)rtData >> (rsData & 0x1F)));
            break;
        case ALUOpReg::SRL:
            set(rd, (u32)rtData >> sa);
            break;
        case ALUOpReg::SRLV:
            set(rd, (u32)rtData >> (rsData & 0x1F));
            break;
        case ALUOpReg::SUBU:
            set(rd, (u32)(rsData - rtData));
            break;
        case ALUOpReg::XOR:
            set(rd, rsData ^ rtData);
            break;
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *rdName = REG_NAMES[rd];
        const char *rsName = REG_NAMES[rs];
        const char *rtName = REG_NAMES[rt];

        const u64 rdData = get(rd);

        const u32 pc = getPC<true>();

        switch (op) {
            case ALUOpReg::ADD:
                std::printf("[%08X:%08X] add %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::ADDU:
                std::printf("[%08X:%08X] addu %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::AND:
                std::printf("[%08X:%08X] and %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::DIV:
                std::printf("[%08X:%08X] div %s, %s; LO = %016llX, HI = %016llX\n", pc, instr.raw, rsName, rtName, get(Register::LO), get(Register::HI));
                break;
            case ALUOpReg::DIVU:
                std::printf("[%08X:%08X] divu %s, %s; LO = %016llX, HI = %016llX\n", pc, instr.raw, rsName, rtName, get(Register::LO), get(Register::HI));
                break;
            case ALUOpReg::DSLL:
                std::printf("[%08X:%08X] dsll %s, %s, %u; %s = %016llX\n", pc, instr.raw, rdName, rtName, sa, rdName, rdData);
                break;
            case ALUOpReg::DSLLV:
                std::printf("[%08X:%08X] dsllv %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::DSLL32:
                std::printf("[%08X:%08X] dsll32 %s, %s, %u; %s = %016llX\n", pc, instr.raw, rdName, rtName, sa, rdName, rdData);
                break;
            case ALUOpReg::DSRA32:
                std::printf("[%08X:%08X] dsra32 %s, %s, %u; %s = %016llX\n", pc, instr.raw, rdName, rtName, sa, rdName, rdData);
                break;
            case ALUOpReg::MFHI:
                std::printf("[%08X:%08X] mfhi %s; %s = %016llX\n", pc, instr.raw, rdName, rdName, rdData);
                break;
            case ALUOpReg::MFLO:
                std::printf("[%08X:%08X] mflo %s; %s = %016llX\n", pc, instr.raw, rdName, rdName, rdData);
                break;
            case ALUOpReg::MTHI:
                std::printf("[%08X:%08X] mthi %s; HI = %016llX\n", pc, instr.raw, rsName, rsData);
                break;
            case ALUOpReg::MTLO:
                std::printf("[%08X:%08X] mtlo %s; LO = %016llX\n", pc, instr.raw, rsName, rsData);
                break;
            case ALUOpReg::MULT:
                std::printf("[%08X:%08X] mult %s, %s; LO = %016llX, HI = %016llX\n", pc, instr.raw, rsName, rtName, get(Register::LO), get(Register::HI));
                break;
            case ALUOpReg::MULTU:
                std::printf("[%08X:%08X] multu %s, %s; LO = %016llX, HI = %016llX\n", pc, instr.raw, rsName, rtName, get(Register::LO), get(Register::HI));
                break;
            case ALUOpReg::NOR:
                std::printf("[%08X:%08X] nor %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::OR:
                std::printf("[%08X:%08X] or %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::SLL:
                if (rd == Register::R0) {
                    std::printf("[%08X:%08X] nop\n", pc, instr.raw);
                } else {
                    std::printf("[%08X:%08X] sll %s, %s, %u; %s = %016llX\n", pc, instr.raw, rdName, rtName, sa, rdName, rdData);
                }
                break;
            case ALUOpReg::SLLV:
                std::printf("[%08X:%08X] sllv %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::SLT:
                std::printf("[%08X:%08X] slt %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::SLTU:
                std::printf("[%08X:%08X] sltu %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::SRA:
                std::printf("[%08X:%08X] sra %s, %s, %u; %s = %016llX\n", pc, instr.raw, rdName, rtName, sa, rdName, rdData);
                break;
            case ALUOpReg::SRAV:
                std::printf("[%08X:%08X] srav %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::SRL:
                std::printf("[%08X:%08X] srl %s, %s, %u; %s = %016llX\n", pc, instr.raw, rdName, rtName, sa, rdName, rdData);
                break;
            case ALUOpReg::SRLV:
                std::printf("[%08X:%08X] srlv %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::SUBU:
                std::printf("[%08X:%08X] subu %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::XOR:
                std::printf("[%08X:%08X] xor %s, %s, %s; %s = %016llX\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
        }
    }
}

template<BranchOp op>
void doBranch(const Instruction instr) {
    const u32 rs = instr.iType.rs;
    const u32 rt = instr.iType.rt;

    const u32 imm = instr.iType.immediate;
    const u64 offset = (i16)imm;

    const u64 target = getPC<false>() + (offset << 2);

    const u64 rsData = get(rs);
    const u64 rtData = get(rt);

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *rsName = REG_NAMES[rs];
        const char *rtName = REG_NAMES[rt];

        const u32 pc = getPC<true>();

        switch (op) {
            case BranchOp::BC1TL:
                std::printf("[%08X:%08X] bc1tl %08llX\n", pc, instr.raw, target);
                break;
            case BranchOp::BEQ:
                std::printf("[%08X:%08X] beq %s, %s, %08llX; %s = %016llX, %s = %016llX\n", pc, instr.raw, rsName, rtName, target, rsName, rsData, rtName, rtData);
                break;
            case BranchOp::BEQL:
                std::printf("[%08X:%08X] beql %s, %s, %08llX; %s = %016llX, %s = %016llX\n", pc, instr.raw, rsName, rtName, target, rsName, rsData, rtName, rtData);
                break;
            case BranchOp::BLEZ:
                std::printf("[%08X:%08X] blez %s, %08llX; %s = %016llX\n", pc, instr.raw, rsName, target, rsName, rsData);
                break;
            case BranchOp::BLEZL:
                std::printf("[%08X:%08X] blezl %s, %08llX; %s = %016llX\n", pc, instr.raw, rsName, target, rsName, rsData);
                break;
            case BranchOp::BGEZ:
                std::printf("[%08X:%08X] bgez %s, %08llX; %s = %016llX\n", pc, instr.raw, rsName, target, rsName, rsData);
                break;
            case BranchOp::BGEZL:
                std::printf("[%08X:%08X] bgezl %s, %08llX; %s = %016llX\n", pc, instr.raw, rsName, target, rsName, rsData);
                break;
            case BranchOp::BGEZAL:
                std::printf("[%08X:%08X] bgezal %s, %08llX; %s = %016llX, ra = %016llX\n", pc, instr.raw, rsName, target, rsName, rsData, getPC<false>());
                break;
            case BranchOp::BGTZ:
                std::printf("[%08X:%08X] bgtz %s, %08llX; %s = %016llX\n", pc, instr.raw, rsName, target, rsName, rsData);
                break;
            case BranchOp::BNE:
                std::printf("[%08X:%08X] bne %s, %s, %08llX; %s = %016llX, %s = %016llX\n", pc, instr.raw, rsName, rtName, target, rsName, rsData, rtName, rtData);
                break;
            case BranchOp::BNEL:
                std::printf("[%08X:%08X] bnel %s, %s, %08llX; %s = %016llX, %s = %016llX\n", pc, instr.raw, rsName, rtName, target, rsName, rsData, rtName, rtData);
                break;
        }
    }

    switch (op) {
        case BranchOp::BC1TL:
            branch(target, fpu::getCondition(), Register::R0, true);
            break;
        case BranchOp::BEQ:
            branch(target, rsData == rtData, Register::R0, false);
            break;
        case BranchOp::BEQL:
            branch(target, rsData == rtData, Register::R0, true);
            break;
        case BranchOp::BLEZ:
            branch(target, (i64)rsData <= 0, Register::R0, false);
            break;
        case BranchOp::BLEZL:
            branch(target, (i64)rsData <= 0, Register::R0, true);
            break;
        case BranchOp::BGEZ:
            branch(target, (i64)rsData >= 0, Register::R0, false);
            break;
        case BranchOp::BGEZL:
            branch(target, (i64)rsData >= 0, Register::R0, true);
            break;
        case BranchOp::BGEZAL:
            branch(target, (i64)rsData >= 0, Register::RA, false);
            break;
        case BranchOp::BGTZ:
            branch(target, (i64)rsData > 0, Register::R0, false);
            break;
        case BranchOp::BNE:
            branch(target, rsData != rtData, Register::R0, false);
            break;
        case BranchOp::BNEL:
            branch(target, rsData != rtData, Register::R0, true);
            break;
    }
}

template<int coprocessor>
void doCoprocessor(const Instruction instr) {
    if constexpr (coprocessor > Coprocessor::FPU) {
        PLOG_FATAL << "Unrecognized coprocessor " << coprocessor;
        
        exit(0);
    }

    if (!cop0::isCoprocessorUsable(coprocessor)) {
        PLOG_WARNING << "Unimplemented Coprocessor Unusable exception";
    }

    const u32 rd = instr.rType.rd;
    const u32 rt = instr.rType.rt;

    const u64 rtData = get(rt);

    const u32 op = instr.rType.rs;
    if constexpr (ENABLE_DISASSEMBLER) {
        const char *rtName = REG_NAMES[rt];

        const u32 pc = getPC<true>();

        switch (op) {
            case CoprocessorOpcode::MF:
                std::printf("[%08X:%08X] mfc%d %s, %u; %u = %08X\n", pc, instr.raw, coprocessor, rtName, rd, rd, (u32)rtData);
                break;
            case CoprocessorOpcode::CF:
                std::printf("[%08X:%08X] cfc%d %s, %u; %u = %08X\n", pc, instr.raw, coprocessor, rtName, rd, rd, (u32)rtData);
                break;
            case CoprocessorOpcode::MT:
                std::printf("[%08X:%08X] mtc%d %s, %u; %u = %08X\n", pc, instr.raw, coprocessor, rtName, rd, rd, (u32)rtData);
                break;
            case CoprocessorOpcode::CT:
                std::printf("[%08X:%08X] ctc%d %s, %u; %u = %08X\n", pc, instr.raw, coprocessor, rtName, rd, rd, (u32)rtData);
                break;
            case CoprocessorOpcode::BC:
            case CoprocessorOpcode::CO:
                break;
        default:
            PLOG_FATAL << "Unrecognized coprocessor opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

            exit(0);
        }
    }

    switch (op) {
        case CoprocessorOpcode::MF:
            switch (coprocessor) {
                case Coprocessor::SystemControl:
                    return set(rt, cop0::get<u32>(rd));
                case Coprocessor::FPU:
                    return set(rt, fpu::get<u32>(rd));
            }
            break;
        case CoprocessorOpcode::CF:
            switch (coprocessor) {
                case Coprocessor::SystemControl:
                    PLOG_FATAL << "Invalid coprocessor for CFC";

                    exit(0);
                case Coprocessor::FPU:
                    return set(rt, fpu::getControl(rd));
            }
            break;
        case CoprocessorOpcode::MT:
            switch (coprocessor) {
                case Coprocessor::SystemControl:
                    return cop0::set(rd, (u32)rtData);
                case Coprocessor::FPU:
                    return fpu::set(rd, (u32)rtData);
            }
            break;
        case CoprocessorOpcode::CT:
            switch (coprocessor) {
                case Coprocessor::SystemControl:
                    PLOG_FATAL << "Invalid coprocessor for CTC";

                    exit(0);
                case Coprocessor::FPU:
                    return fpu::setControl(rd, rtData);
            }
            break;
        case CoprocessorOpcode::BC:
            {
                const u32 op = instr.rType.rt;
                switch (op) {
                    case CoprocessorBranchOpcode::BCTL:
                        switch (coprocessor) {
                            case Coprocessor::SystemControl:
                                PLOG_FATAL << "Invalid coprocessor for BCTL";

                                exit(0);
                            case Coprocessor::FPU:
                                return doBranch<BranchOp::BC1TL>(instr);
                        }
                        break;
                    default:
                        PLOG_FATAL << "Unrecognized coprocessor branch opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

                        exit(0);
                }
            }
            break;
        case CoprocessorOpcode::CO:
            switch (coprocessor) {
                case Coprocessor::SystemControl:
                    return cop0::doInstruction(instr);
                case Coprocessor::FPU:
                    // This is CoprocessorOpcode::SINGLE
                    return fpu::doSingle(instr);
            }
            break;
        case CoprocessorOpcode::DOUBLE:
            switch (coprocessor) {
                case Coprocessor::SystemControl:
                    PLOG_FATAL << "Invalid coprocessor for DOUBLE";

                    exit(0);
                case Coprocessor::FPU:
                    return fpu::doDouble(instr);
            }
            break;
        case CoprocessorOpcode::WORD:
            switch (coprocessor) {
                case Coprocessor::SystemControl:
                    PLOG_FATAL << "Invalid coprocessor for WORD";

                    exit(0);
                case Coprocessor::FPU:
                    return fpu::doWord(instr);
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized coprocessor opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

            exit(0);
    }
}

template<JumpOp op>
void doJump(const Instruction instr) {
    const u32 rd = instr.rType.rd;
    const u32 rs = instr.rType.rs;

    u64 target = (getPC<false>() & 0xFFFFFFFFF0000000) | (instr.jType.target << 2);
    if constexpr ((op == JumpOp::JALR) || (op == JumpOp::JR)) {
        target = get(rs);
    }

    if (ENABLE_DISASSEMBLER) {
        const char *rdName = REG_NAMES[rd];
        const char *rsName = REG_NAMES[rs];

        const u32 pc = getPC<true>();

        switch (op) {
            case JumpOp::J:
                std::printf("[%08X:%08X] j %08X\n", pc, instr.raw, (u32)target);
                break;
            case JumpOp::JAL:
                std::printf("[%08X:%08X] jal %08X; ra = %08llX\n", pc, instr.raw, (u32)target, getPC<false>());
                break;
            case JumpOp::JALR:
                std::printf("[%08X:%08X] jalr %s, %s; PC = %08llX, %s = %08llX\n", pc, instr.raw, rdName, rsName, target, rdName, getPC<false>());
                break;
            case JumpOp::JR:
                std::printf("[%08X:%08X] jr %s; PC = %08llX\n", pc, instr.raw, rsName, target);
                break;
        }
    }

    switch (op) {
        case JumpOp::J:
        case JumpOp::JR:
            branch(target, true, Register::R0, false);
            break;
        case JumpOp::JAL:
            branch(target, true, Register::RA, false);
            break;
        case JumpOp::JALR:
            branch(target, true, rd, false);
            break;
    }
}

template<LoadStoreOp op>
void doLoadStore(const Instruction instr) {
    const u32 base = instr.iType.rs;
    const u32 rt = instr.iType.rt;

    const u32 imm = instr.iType.immediate;
    const u64 offset = (i16)imm;

    const u64 vaddr = get(base) + offset;

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *baseName = REG_NAMES[base];
        const char *rtName = REG_NAMES[rt];

        const u32 pc = getPC<true>();

        const u64 data = get(rt);

        switch (op) {
            case LoadStoreOp::LB:
                std::printf("[%08X:%08X] lb %s, %04X(%s); %s = [%08llX]\n", pc, instr.raw, rtName, imm, baseName, rtName, vaddr);
                break;
            case LoadStoreOp::LBU:
                std::printf("[%08X:%08X] lbu %s, %04X(%s); %s = [%08llX]\n", pc, instr.raw, rtName, imm, baseName, rtName, vaddr);
                break;
            case LoadStoreOp::LD:
                std::printf("[%08X:%08X] ld %s, %04X(%s); %s = [%08llX]\n", pc, instr.raw, rtName, imm, baseName, rtName, vaddr);
                break;
            case LoadStoreOp::LH:
                std::printf("[%08X:%08X] lh %s, %04X(%s); %s = [%08llX]\n", pc, instr.raw, rtName, imm, baseName, rtName, vaddr);
                break;
            case LoadStoreOp::LHU:
                std::printf("[%08X:%08X] lhu %s, %04X(%s); %s = [%08llX]\n", pc, instr.raw, rtName, imm, baseName, rtName, vaddr);
                break;
            case LoadStoreOp::LW:
                std::printf("[%08X:%08X] lw %s, %04X(%s); %s = [%08llX]\n", pc, instr.raw, rtName, imm, baseName, rtName, vaddr);
                break;
            case LoadStoreOp::LWC1:
                std::printf("[%08X:%08X] lwc1 %u, %04X(%s); %u = [%08llX]\n", pc, instr.raw, rt, imm, baseName, rt, vaddr);
                break;
            case LoadStoreOp::LWU:
                std::printf("[%08X:%08X] lwu %s, %04X(%s); %s = [%08llX]\n", pc, instr.raw, rtName, imm, baseName, rtName, vaddr);
                break;
            case LoadStoreOp::SB:
                std::printf("[%08X:%08X] sb %s, %04X(%s); [%08llX] = %02X\n", pc, instr.raw, rtName, imm, baseName, vaddr, (u16)data & 0xFF);
                break;
            case LoadStoreOp::SD:
                std::printf("[%08X:%08X] sd %s, %04X(%s); [%08llX] = %016llX\n", pc, instr.raw, rtName, imm, baseName, vaddr, data);
                break;
            case LoadStoreOp::SH:
                std::printf("[%08X:%08X] sw %s, %04X(%s); [%08llX] = %04X\n", pc, instr.raw, rtName, imm, baseName, vaddr, (u16)data);
                break;
            case LoadStoreOp::SW:
                std::printf("[%08X:%08X] sw %s, %04X(%s); [%08llX] = %08X\n", pc, instr.raw, rtName, imm, baseName, vaddr, (u32)data);
                break;
        }
    }

    switch (op) {
        case LoadStoreOp::LB:
            set(rt, (u64)(i8)read<u8>(vaddr));
            break;
        case LoadStoreOp::LBU:
            set(rt, (u64)read<u8>(vaddr));
            break;
        case LoadStoreOp::LD:
            if (!isAlignedAddress<u64>(vaddr)) {
                PLOG_FATAL << "Unaligned LD address " << std::hex << vaddr;

                exit(0);
            }

            set(rt, read<u64>(vaddr));
            break;
        case LoadStoreOp::LH:
            if (!isAlignedAddress<u16>(vaddr)) {
                PLOG_FATAL << "Unaligned LH address " << std::hex << vaddr;

                exit(0);
            }

            set(rt, (u64)(i16)read<u16>(vaddr));
            break;
        case LoadStoreOp::LHU:
            if (!isAlignedAddress<u16>(vaddr)) {
                PLOG_FATAL << "Unaligned LHU address " << std::hex << vaddr;

                exit(0);
            }

            set(rt, (u64)read<u16>(vaddr));
            break;
        case LoadStoreOp::LW:
            if (!isAlignedAddress<u32>(vaddr)) {
                PLOG_FATAL << "Unaligned LW address " << std::hex << vaddr;

                exit(0);
            }

            set(rt, read<u32>(vaddr));
            break;
        case LoadStoreOp::LWC1:
            if (!cop0::isCoprocessorUsable(Coprocessor::FPU)) {
                PLOG_WARNING << "Unimplemented Coprocessor Unusable exception";
            }

            if (!isAlignedAddress<u32>(vaddr)) {
                PLOG_FATAL << "Unaligned LWC1 address " << std::hex << vaddr;

                exit(0);
            }

            fpu::set(rt, read<u32>(vaddr));
            break;
        case LoadStoreOp::LWU:
            if (!isAlignedAddress<u32>(vaddr)) {
                PLOG_FATAL << "Unaligned LWU address " << std::hex << vaddr;

                exit(0);
            }

            set(rt, (u64)read<u32>(vaddr));
            break;
        case LoadStoreOp::SB:
            write(vaddr, (u8)get(rt));
            break;
        case LoadStoreOp::SD:
            if (!isAlignedAddress<u64>(vaddr)) {
                PLOG_FATAL << "Unaligned SD address " << std::hex << vaddr;

                exit(0);
            }

            write(vaddr, get(rt));
            break;
        case LoadStoreOp::SH:
            if (!isAlignedAddress<u16>(vaddr)) {
                PLOG_FATAL << "Unaligned SH address " << std::hex << vaddr;

                exit(0);
            }

            write(vaddr, (u16)get(rt));
            break;
        case LoadStoreOp::SW:
            if (!isAlignedAddress<u32>(vaddr)) {
                PLOG_FATAL << "Unaligned SW address " << std::hex << vaddr;

                exit(0);
            }

            write(vaddr, (u32)get(rt));
            break;
    }
}

void doInstruction() {
    Instruction instr;
    instr.raw = fetch();

    const u32 op = instr.iType.op;
    switch (op) {
        case Opcode::SPECIAL: {
                const u32 funct = instr.rType.funct;
                switch (funct) {
                    case SpecialOpcode::SLL:
                        doALURegister<ALUOpReg::SLL>(instr);
                        break;
                    case SpecialOpcode::SRL:
                        doALURegister<ALUOpReg::SRL>(instr);
                        break;
                    case SpecialOpcode::SRA:
                        doALURegister<ALUOpReg::SRA>(instr);
                        break;
                    case SpecialOpcode::SLLV:
                        doALURegister<ALUOpReg::SLLV>(instr);
                        break;
                    case SpecialOpcode::SRLV:
                        doALURegister<ALUOpReg::SRLV>(instr);
                        break;
                    case SpecialOpcode::SRAV:
                        doALURegister<ALUOpReg::SRAV>(instr);
                        break;
                    case SpecialOpcode::JR:
                        doJump<JumpOp::JR>(instr);
                        break;
                    case SpecialOpcode::JALR:
                        doJump<JumpOp::JALR>(instr);
                        break;
                    case SpecialOpcode::MFHI:
                        doALURegister<ALUOpReg::MFHI>(instr);
                        break;
                    case SpecialOpcode::MTHI:
                        doALURegister<ALUOpReg::MTHI>(instr);
                        break;
                    case SpecialOpcode::MFLO:
                        doALURegister<ALUOpReg::MFLO>(instr);
                        break;
                    case SpecialOpcode::MTLO:
                        doALURegister<ALUOpReg::MTLO>(instr);
                        break;
                    case SpecialOpcode::DSLLV:
                        doALURegister<ALUOpReg::DSLLV>(instr);
                        break;
                    case SpecialOpcode::MULT:
                        doALURegister<ALUOpReg::MULT>(instr);
                        break;
                    case SpecialOpcode::MULTU:
                        doALURegister<ALUOpReg::MULTU>(instr);
                        break;
                    case SpecialOpcode::DIV:
                        doALURegister<ALUOpReg::DIV>(instr);
                        break;
                    case SpecialOpcode::DIVU:
                        doALURegister<ALUOpReg::DIVU>(instr);
                        break;
                    case SpecialOpcode::ADD:
                        doALURegister<ALUOpReg::ADD>(instr);
                        break;
                    case SpecialOpcode::ADDU:
                        doALURegister<ALUOpReg::ADDU>(instr);
                        break;
                    case SpecialOpcode::SUBU:
                        doALURegister<ALUOpReg::SUBU>(instr);
                        break;
                    case SpecialOpcode::AND:
                        doALURegister<ALUOpReg::AND>(instr);
                        break;
                    case SpecialOpcode::OR:
                        doALURegister<ALUOpReg::OR>(instr);
                        break;
                    case SpecialOpcode::XOR:
                        doALURegister<ALUOpReg::XOR>(instr);
                        break;
                    case SpecialOpcode::NOR:
                        doALURegister<ALUOpReg::NOR>(instr);
                        break;
                    case SpecialOpcode::SLT:
                        doALURegister<ALUOpReg::SLT>(instr);
                        break;
                    case SpecialOpcode::SLTU:
                        doALURegister<ALUOpReg::SLTU>(instr);
                        break;
                    case SpecialOpcode::DSLL:
                        doALURegister<ALUOpReg::DSLL>(instr);
                        break;
                    case SpecialOpcode::DSLL32:
                        doALURegister<ALUOpReg::DSLL32>(instr);
                        break;
                    case SpecialOpcode::DSRA32:
                        doALURegister<ALUOpReg::DSRA32>(instr);
                        break;
                    default:
                        PLOG_FATAL << "Unrecognized function " << std::hex << funct << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

                        exit(0);
                }
            }
            break;
        case Opcode::REGIMM: {
                const u32 op = instr.iType.rt;
                switch (op) {
                    case RegimmOpcode::BGEZ:
                        doBranch<BranchOp::BGEZ>(instr);
                        break;
                    case RegimmOpcode::BGEZL:
                        doBranch<BranchOp::BGEZL>(instr);
                        break;
                    case RegimmOpcode::BGEZAL:
                        doBranch<BranchOp::BGEZAL>(instr);
                        break;
                    default:
                        PLOG_FATAL << "Unrecognized REGIMM opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getPC<true>() << ")";

                        exit(0);
                }
            }
            break;
        case Opcode::J:
            doJump<JumpOp::J>(instr);
            break;
        case Opcode::JAL:
            doJump<JumpOp::JAL>(instr);
            break;
        case Opcode::BEQ:
            doBranch<BranchOp::BEQ>(instr);
            break;
        case Opcode::BNE:
            doBranch<BranchOp::BNE>(instr);
            break;
        case Opcode::BLEZ:
            doBranch<BranchOp::BLEZ>(instr);
            break;
        case Opcode::BGTZ:
            doBranch<BranchOp::BGTZ>(instr);
            break;
        case Opcode::ADDI:
            doALUImmediate<ALUOpImm::ADDI>(instr);
            break;
        case Opcode::ADDIU:
            doALUImmediate<ALUOpImm::ADDIU>(instr);
            break;
        case Opcode::SLTI:
            doALUImmediate<ALUOpImm::SLTI>(instr);
            break;
        case Opcode::SLTIU:
            doALUImmediate<ALUOpImm::SLTIU>(instr);
            break;
        case Opcode::ANDI:
            doALUImmediate<ALUOpImm::ANDI>(instr);
            break;
        case Opcode::ORI:
            doALUImmediate<ALUOpImm::ORI>(instr);
            break;
        case Opcode::XORI:
            doALUImmediate<ALUOpImm::XORI>(instr);
            break;
        case Opcode::LUI:
            doALUImmediate<ALUOpImm::LUI>(instr);
            break;
        case Opcode::COP0:
            doCoprocessor<0>(instr);
            break;
        case Opcode::COP1:
            doCoprocessor<1>(instr);
            break;
        case Opcode::BEQL:
            doBranch<BranchOp::BEQL>(instr);
            break;
        case Opcode::BNEL:
            doBranch<BranchOp::BNEL>(instr);
            break;
        case Opcode::BLEZL:
            doBranch<BranchOp::BLEZL>(instr);
            break;
        case Opcode::DADDI:
            doALUImmediate<ALUOpImm::DADDI>(instr);
            break;
        case Opcode::DADDIU:
            doALUImmediate<ALUOpImm::DADDIU>(instr);
            break;
        case Opcode::LB:
            doLoadStore<LoadStoreOp::LB>(instr);
            break;
        case Opcode::LH:
            doLoadStore<LoadStoreOp::LH>(instr);
            break;
        case Opcode::LW:
            doLoadStore<LoadStoreOp::LW>(instr);
            break;
        case Opcode::LBU:
            doLoadStore<LoadStoreOp::LBU>(instr);
            break;
        case Opcode::LHU:
            doLoadStore<LoadStoreOp::LHU>(instr);
            break;
        case Opcode::LWU:
            doLoadStore<LoadStoreOp::LWU>(instr);
            break;
        case Opcode::SB:
            doLoadStore<LoadStoreOp::SB>(instr);
            break;
        case Opcode::SH:
            doLoadStore<LoadStoreOp::SH>(instr);
            break;
        case Opcode::SW:
            doLoadStore<LoadStoreOp::SW>(instr);
            break;
        case Opcode::CACHE:
            PLOG_WARNING << "CACHE instruction";
            break;
        case Opcode::LWC1:
            doLoadStore<LoadStoreOp::LWC1>(instr);
            break;
        case Opcode::LD:
            doLoadStore<LoadStoreOp::LD>(instr);
            break;
        case Opcode::SD:
            doLoadStore<LoadStoreOp::SD>(instr);
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

        advanceDelaySlot();
        doInstruction();
    }
}

}
