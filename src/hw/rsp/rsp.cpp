/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/rsp/rsp.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/dp.hpp"
#include "hw/sp.hpp"
#include "hw/cpu/cpu.hpp"

#include "sys/memory.hpp"

namespace hw::rsp {

using cpu::Instruction;

constexpr bool ENABLE_DISASSEMBLER = false;

constexpr u64 NUM_LANES = 8;

constexpr u64 ACCUMULATOR_SHIFT = 16;

// RSP general-purpose registers
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
        IO = 0,
        VectorUnit = 2,
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
        ANDI = 0x0C,
        ORI = 0x0D,
        LUI = 0x0F,
        COP0 = 0x10,
        COP2 = 0x12,
        LB = 0x20,
        LH = 0x21,
        LW = 0x23,
        LBU = 0x24,
        LHU = 0x25,
        SB = 0x28,
        SH = 0x29,
        SW = 0x2B,
        LWC2 = 0x32,
        SWC2 = 0x3A,
    };
}

namespace SpecialOpcode {
    enum : u32 {
        SLL = 0x00,
        SRL = 0x02,
        SRA = 0x03,
        SLLV = 0x04,
        JR = 0x08,
        BREAK = 0x0D,
        ADD = 0x20,
        SUB = 0x22,
        AND = 0x24,
        OR = 0x25,
        NOR = 0x27,
    };
}

namespace RegimmOpcode {
    enum : u32 {
        BLTZ = 0x00,
        BGEZ = 0x01,
    };
}

namespace CoprocessorOpcode {
    enum : u32 {
        MF = 0x00,
        MT = 0x04,
        COMPUTE = 0x10,
    };
}

namespace VUComputeOpcode {
    enum : u32 {
        VMULF = 0x00,
        VMACF = 0x08,
        VXOR = 0x2C,
    };
}

namespace VULoadOpcode {
    enum : u32 {
        LDV = 0x03,
        LQV = 0x04,
    };
}

namespace VUStoreOpcode {
    enum : u32 {
        SSV = 0x01,
        SDV = 0x03,
        SQV = 0x04,
    };
}

enum class ALUOpImm {
    ADDI,
    ANDI,
    LUI,
    ORI,
};

enum class ALUOpReg {
    ADD,
    AND,
    NOR,
    OR,
    SLL,
    SLLV,
    SRA,
    SRL,
    SUB,
};

enum class BranchOp {
    BEQ,
    BGEZ,
    BGTZ,
    BLEZ,
    BLTZ,
    BNE,
};

enum class JumpOp {
    J,
    JAL,
    JR,
};

enum class LoadStoreOp {
    LB,
    LBU,
    LH,
    LHU,
    LW,
    SB,
    SH,
    SW,
};

u16 clampSigned(const i64 data) {
    if (data < -0x8000) {
        return 0x8000; ;
    }

    if (data > 0x7FFF) {
        return 0x7FFF;
    }

    return (u16)data;
}

u32 getAccumulatorIndex(const u32 idx) {
    return 2 - idx;
}

u32 getLaneIndex(const u32 idx) {
    return 7 - idx;
}

struct Accumulator {
    u64 lanes[NUM_LANES];

    u64 getLane(const u32 idx) {
        return lanes[getLaneIndex(idx)];
    }

    i64 getSignedLane(const u32 idx) {
        return ((i64)lanes[getLaneIndex(idx)] << ACCUMULATOR_SHIFT) >> ACCUMULATOR_SHIFT;
    }

    u16 getShort(const u32 idx, const u32 element) {
        return (u16)(lanes[getLaneIndex(idx)] >> (16 * getAccumulatorIndex(element)));
    }

    void setLane(const u32 idx, const u64 data) {
        lanes[getLaneIndex(idx)] = data & 0xFFFFFFFFFFFF;
    }

    void setSignedLane(const u32 idx, const i64 data) {
        lanes[getLaneIndex(idx)] = data;
    }

    void setShort(const u32 idx, const u32 element, const u16 data) {
        const u64 shift = 16 * getAccumulatorIndex(element);
        const u64 mask = 0xFFFF << shift;

        const u64 laneIndex = getLaneIndex(idx);

        lanes[laneIndex] = (lanes[laneIndex] & ~mask) | ((u64)data << shift);
    }
};

struct VectorRegister {
    u16 lanes[NUM_LANES];

    u16 getLane(const u32 idx) {
        return lanes[getLaneIndex(idx)];
    }

    i16 getSignedLane(const u32 idx) {
        return (i16)lanes[getLaneIndex(idx)];
    }

    void setLane(const u32 idx, const u16 data) {
        lanes[getLaneIndex(idx)] = data;
    }
};

struct Registers {
    u32 regs[Register::NumberOfRegisters];

    Accumulator acc;

    VectorRegister vuRegs[32];

    union {
        u32 raw;
        struct {
            u32 addr : 12;
            u32 : 20;
        };
    } pc, npc, cpc;

    u8 getByte(const u32 idx, const u32 element) {
        return vuRegs[idx].lanes[element >> 1] >> (8 * ((element ^ 1) & 1));
    }

    u16 getLane(const u32 idx, const u32 element) {
        return vuRegs[idx].getLane(element);
    }

    void setByte(const u32 idx, const u32 element, const u8 data) {
        const u16 laneData = vuRegs[idx].lanes[element >> 1];

        const u32 shift = 8 * ((element ^ 1) & 1);
        const u16 mask = 0xFF << shift;

        vuRegs[idx].lanes[element >> 1] = (laneData & ~mask) | ((u16)data << shift);
    }

    void setLane(const u32 idx, const u32 element, const u16 data) {
        vuRegs[idx].setLane(element, data);
    }

    VectorRegister broadcast(const u32 idx, const u32 broadcastMod) {
        constexpr u64 BROADCAST_MASKS[16] = {
            0x76543210, 0x76543210, 0x66442200, 0x77553311,
            0x44440000, 0x55551111, 0x66662222, 0x77773333,
            0x00000000, 0x11111111, 0x22222222, 0x33333333,
            0x44444444, 0x55555555, 0x66666666, 0x77777777,
        };

        const VectorRegister &reg = vuRegs[idx];

        const u64 mask = BROADCAST_MASKS[broadcastMod];

        VectorRegister broadcastReg;
        for (u64 i = 0; i < 8; i++) {
            broadcastReg.lanes[i] = reg.lanes[(mask >> (4 * i)) & 7];
        }

        return broadcastReg;
    }
};

u8 *dmem;
u32 *imem;

Registers regFile;

bool isHalted;

void init() {
    dmem = sys::memory::getPointer(sys::memory::MemoryBase::RSP_DMEM);
    imem = (u32 *)sys::memory::getPointer(sys::memory::MemoryBase::RSP_IMEM);
}

void deinit() {}

void reset() {
    std::memset(&regFile, 0, sizeof(Registers));

    isHalted = true;
}

bool isValidRegisterIndex(const u32 idx) {
    return idx < Register::NumberOfRegisters;
}

u32 get(const u32 idx) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    return regFile.regs[idx];
}

u32 getPC() {
    return regFile.pc.addr;
}

u32 getCurrentPC() {
    return regFile.cpc.addr;
}

void branch(const u32 target, const bool condition, const u32 linkReg) {
    // Save return address
    set(linkReg, regFile.npc.addr);

    if (condition) {
        setBranchPC(target);
    }
}

void set(const u32 idx, const u32 data) {
    if (!isValidRegisterIndex(idx)) {
        PLOG_FATAL << "Register index out of bounds";

        exit(0);
    }

    auto &regs = regFile.regs;

    regs[idx] = data;

    // Hardwired to 0
    regs[Register::R0] = 0;
}

void setPC(const u32 addr) {
    regFile.pc.addr = addr;
    regFile.npc.addr = addr + 4;
}

void setBranchPC(const u32 addr) {
    regFile.npc.addr = addr;
}

void advancePC() {
    regFile.pc.addr = regFile.npc.addr;
    regFile.npc.addr += sizeof(Instruction);
}

template<typename T>
T read(const u32 addr) requires std::is_unsigned_v<T> {
    T data = 0;
    for (u32 i = 0; i < sizeof(T); i++) {
        data |= ((T)dmem[(addr + i) & 0xFFF]) << (8 * i);
    }

    return byteswap(data);
}

u32 fetch() {
    const u32 data = byteswap(imem[getCurrentPC() >> 2]);

    advancePC();

    return data;
}

template<typename T>
void write(const u32 addr, T data) requires std::is_unsigned_v<T> {
    data = byteswap(data);

    for (u32 i = 0; i < sizeof(T); i++) {
        dmem[(addr + i) & 0xFFF] = (u8)(data >> (8 * i));
    }
}

template<ALUOpImm op>
void doALUImmediate(const Instruction instr) {
    const u32 rs = instr.iType.rs;
    const u32 rt = instr.iType.rt;

    const u32 imm = instr.iType.immediate;

    const u32 rsData = get(rs);

    switch (op) {
        case ALUOpImm::ADDI:
            set(rt, rsData + (u32)(i16)imm);
            break;
        case ALUOpImm::ANDI:
            set(rt, rsData & imm);
            break;
        case ALUOpImm::LUI:
            set(rt, imm << 16);
            break;
        case ALUOpImm::ORI:
            set(rt, rsData | imm);
            break;
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *rsName = REG_NAMES[rs];
        const char *rtName = REG_NAMES[rt];

        const u32 pc = getCurrentPC();

        const u32 rtData = get(rt);

        switch (op) {
            case ALUOpImm::ADDI:
                std::printf("[%03X:%08X] addi %s, %s, %04X; %s = %08X\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
            case ALUOpImm::ANDI:
                std::printf("[%03X:%08X] andi %s, %s, %04X; %s = %08X\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
                break;
            case ALUOpImm::LUI:
                std::printf("[%03X:%08X] lui %s, %04X; %s = %08X\n", pc, instr.raw, rtName, imm, rtName, rtData);
                break;
            case ALUOpImm::ORI:
                std::printf("[%03X:%08X] ori %s, %s, %04X; %s = %08X\n", pc, instr.raw, rtName, rsName, imm, rtName, rtData);
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

    const u32 rsData = get(rs);
    const u32 rtData = get(rt);

    switch (op) {
        case ALUOpReg::ADD:
            set(rd, rsData + rtData);
            break;
        case ALUOpReg::AND:
            set(rd, rsData & rtData);
            break;
        case ALUOpReg::NOR:
            set(rd, ~(rsData | rtData));
            break;
        case ALUOpReg::OR:
            set(rd, rsData | rtData);
            break;
        case ALUOpReg::SLL:
            set(rd, rtData << sa);
            break;
        case ALUOpReg::SLLV:
            set(rd, rtData << (rsData & 0x1F));
            break;
        case ALUOpReg::SRA:
            set(rd, (i32)rtData >> sa);
            break;
        case ALUOpReg::SRL:
            set(rd, rtData >> sa);
            break;
        case ALUOpReg::SUB:
            set(rd, rsData - rtData);
            break;
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *rdName = REG_NAMES[rd];
        const char *rsName = REG_NAMES[rs];
        const char *rtName = REG_NAMES[rt];

        const u32 rdData = get(rd);

        const u32 pc = getCurrentPC();

        switch (op) {
            case ALUOpReg::ADD:
                std::printf("[%03X:%08X] add %s, %s, %s; %s = %08X\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::AND:
                std::printf("[%03X:%08X] and %s, %s, %s; %s = %08X\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::NOR:
                std::printf("[%03X:%08X] nor %s, %s, %s; %s = %08X\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::OR:
                std::printf("[%03X:%08X] or %s, %s, %s; %s = %08X\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::SLL:
                if (rd == Register::R0) {
                    std::printf("[%03X:%08X] nop\n", pc, instr.raw);
                } else {
                    std::printf("[%03X:%08X] sll %s, %s, %u; %s = %08X\n", pc, instr.raw, rdName, rtName, sa, rdName, rdData);
                }
                break;
            case ALUOpReg::SLLV:
                std::printf("[%03X:%08X] sllv %s, %s, %s; %s = %08X\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
            case ALUOpReg::SRA:
                std::printf("[%03X:%08X] sra %s, %s, %u; %s = %08X\n", pc, instr.raw, rdName, rtName, sa, rdName, rdData);
                break;
            case ALUOpReg::SRL:
                std::printf("[%03X:%08X] srl %s, %s, %u; %s = %08X\n", pc, instr.raw, rdName, rtName, sa, rdName, rdData);
                break;
            case ALUOpReg::SUB:
                std::printf("[%03X:%08X] sub %s, %s, %s; %s = %08X\n", pc, instr.raw, rdName, rsName, rtName, rdName, rdData);
                break;
        }
    }
}

template<BranchOp op>
void doBranch(const Instruction instr) {
    const u32 rs = instr.iType.rs;
    const u32 rt = instr.iType.rt;

    const u32 imm = instr.iType.immediate;
    const u32 offset = (i16)imm;

    const u32 target = (getPC() + (offset << 2)) & 0xFFC;

    const u32 rsData = get(rs);
    const u32 rtData = get(rt);

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *rsName = REG_NAMES[rs];
        const char *rtName = REG_NAMES[rt];

        const u32 pc = getCurrentPC();

        switch (op) {
            case BranchOp::BEQ:
                std::printf("[%03X:%08X] beq %s, %s, %03X; %s = %08X, %s = %08X\n", pc, instr.raw, rsName, rtName, target, rsName, rsData, rtName, rtData);
                break;
            case BranchOp::BGEZ:
                std::printf("[%03X:%08X] bgez %s, %03X; %s = %08X\n", pc, instr.raw, rsName, target, rsName, rsData);
                break;
            case BranchOp::BGTZ:
                std::printf("[%03X:%08X] bgtz %s, %03X; %s = %08X\n", pc, instr.raw, rsName, target, rsName, rsData);
                break;
            case BranchOp::BLEZ:
                std::printf("[%03X:%08X] blez %s, %03X; %s = %08X\n", pc, instr.raw, rsName, target, rsName, rsData);
                break;
            case BranchOp::BLTZ:
                std::printf("[%03X:%08X] bltz %s, %03X; %s = %08X\n", pc, instr.raw, rsName, target, rsName, rsData);
                break;
            case BranchOp::BNE:
                std::printf("[%03X:%08X] bne %s, %s, %03X; %s = %08X, %s = %08X\n", pc, instr.raw, rsName, rtName, target, rsName, rsData, rtName, rtData);
                break;
        }
    }

    switch (op) {
        case BranchOp::BEQ:
            branch(target, rsData == rtData, Register::R0);
            break;
        case BranchOp::BGEZ:
            branch(target, (i32)rsData >= 0, Register::R0);
            break;
        case BranchOp::BGTZ:
            branch(target, (i32)rsData > 0, Register::R0);
            break;
        case BranchOp::BLEZ:
            branch(target, (i32)rsData <= 0, Register::R0);
            break;
        case BranchOp::BLTZ:
            branch(target, (i32)rsData < 0, Register::R0);
            break;
        case BranchOp::BNE:
            branch(target, rsData != rtData, Register::R0);
            break;
    }
}

template<int coprocessor>
void doCoprocessor(const Instruction instr) {
    const u32 rd = instr.rType.rd;
    const u32 rt = instr.rType.rt;

    const u32 rtData = get(rt);

    const u32 op = instr.rType.rs;
    if constexpr (ENABLE_DISASSEMBLER) {
        const char *rtName = REG_NAMES[rt];

        const u32 pc = getCurrentPC();

        switch (op) {
            case CoprocessorOpcode::MF:
                std::printf("[%03X:%08X] mfc%d %s, %u\n", pc, instr.raw, coprocessor, rtName, rd);
                break;
            case CoprocessorOpcode::MT:
                std::printf("[%03X:%08X] mtc%d %s, %u; %u = %08X\n", pc, instr.raw, coprocessor, rtName, rd, rd, rtData);
                break;
            default:
                PLOG_FATAL << "Unrecognized coprocessor opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getCurrentPC() << ")";

                exit(0);
        }
    }

    switch (op) {
        case CoprocessorOpcode::MF:
            switch (coprocessor) {
                case Coprocessor::IO:
                    if (rd < 8) {
                        return set(rt, sp::readIO(sp::IORegister::IOBase + 4 * rd));
                    } else if (rd < 16) {
                        return set(rt, dp::readIO(dp::IORegister::IOBase + 4 * (rd - 8)));
                    } else {
                        PLOG_FATAL << "Unrecognized COP0 register " << rd;

                        exit(0);
                    }
                    break;
                default:
                    PLOG_FATAL << "Invalid coprocessor for MFC";

                    exit(0);
            }
            break;
        case CoprocessorOpcode::MT:
            switch (coprocessor) {
                case Coprocessor::IO:
                    if (rd < 8) {
                        return sp::writeIO(sp::IORegister::IOBase + 4 * rd, rtData);
                    } else if (rd < 16) {
                        return dp::writeIO(dp::IORegister::IOBase + 4 * (rd - 8), rtData);
                    } else {
                        PLOG_FATAL << "Unrecognized COP0 register " << rd;

                        exit(0);
                    }
                    break;
                case Coprocessor::VectorUnit:
                    {
                        const VUInstruction vuInstr{.raw = instr.raw};

                        regFile.setLane(rd, vuInstr.loadType.element >> 1, (u16)rtData);
                    }
                    break;
                default:
                    PLOG_FATAL << "Invalid coprocessor for MTC";

                    exit(0);
            }
            break;
        default:
            if (op >= CoprocessorOpcode::COMPUTE) {
                if (coprocessor != Coprocessor::VectorUnit) {
                    PLOG_FATAL << "Invalid coprocessor for COMPUTE";

                    exit(0);
                }

                const VUInstruction vuInstr{.raw = instr.raw};

                const u32 op = vuInstr.computeType.opcode;
                switch (op) {
                    case VUComputeOpcode::VMULF:
                        VMULF(vuInstr);
                        break;
                    case VUComputeOpcode::VMACF:
                        VMACF(vuInstr);
                        break;
                    case VUComputeOpcode::VXOR:
                        VXOR(vuInstr);
                        break;
                    default:
                        PLOG_FATAL << "Unrecognized COMPUTE opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getCurrentPC() << ")";

                        exit(0);
                }

                return;
            }

            PLOG_FATAL << "Unrecognized coprocessor opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getCurrentPC() << ")";

            exit(0);
    }
}

template<JumpOp op>
void doJump(const Instruction instr) {
    const u32 rd = instr.rType.rd;
    const u32 rs = instr.rType.rs;

    u32 target = (instr.jType.target << 2) & 0xFFC;
    if constexpr (op == JumpOp::JR) {
        target = get(rs) & 0xFFC;
    }

    if (ENABLE_DISASSEMBLER) {
        const char *rdName = REG_NAMES[rd];
        const char *rsName = REG_NAMES[rs];

        const u32 pc = getCurrentPC();

        switch (op) {
            case JumpOp::J:
                std::printf("[%03X:%08X] j %03X\n", pc, instr.raw, target);
                break;
            case JumpOp::JAL:
                std::printf("[%03X:%08X] jal %08X; ra = %08X\n", pc, instr.raw, target, getPC());
                break;
            case JumpOp::JR:
                std::printf("[%03X:%08X] jr %s; PC = %03X\n", pc, instr.raw, rsName, target);
                break;
        }
    }

    switch (op) {
        case JumpOp::J:
        case JumpOp::JR:
            branch(target, true, Register::R0);
            break;
        case JumpOp::JAL:
            branch(target, true, Register::RA);
            break;
    }
}

template<LoadStoreOp op>
void doLoadStore(const Instruction instr) {
    const u32 base = instr.iType.rs;
    const u32 rt = instr.iType.rt;

    const u32 imm = instr.iType.immediate;
    const u32 offset = (i16)imm;

    const u32 addr = get(base) + offset;

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *baseName = REG_NAMES[base];
        const char *rtName = REG_NAMES[rt];

        const u32 pc = getCurrentPC();

        const u32 data = get(rt);

        switch (op) {
            case LoadStoreOp::LB:
                std::printf("[%03X:%08X] lb %s, %04X(%s); %s = [%03X]\n", pc, instr.raw, rtName, imm, baseName, rtName, addr);
                break;
            case LoadStoreOp::LBU:
                std::printf("[%03X:%08X] lbu %s, %04X(%s); %s = [%03X]\n", pc, instr.raw, rtName, imm, baseName, rtName, addr);
                break;
            case LoadStoreOp::LH:
                std::printf("[%03X:%08X] lh %s, %04X(%s); %s = [%03X]\n", pc, instr.raw, rtName, imm, baseName, rtName, addr);
                break;
            case LoadStoreOp::LHU:
                std::printf("[%03X:%08X] lhu %s, %04X(%s); %s = [%03X]\n", pc, instr.raw, rtName, imm, baseName, rtName, addr);
                break;
            case LoadStoreOp::LW:
                std::printf("[%03X:%08X] lw %s, %04X(%s); %s = [%03X]\n", pc, instr.raw, rtName, imm, baseName, rtName, addr);
                break;
            case LoadStoreOp::SB:
                std::printf("[%03X:%08X] sb %s, %04X(%s); [%03X] = %02X\n", pc, instr.raw, rtName, imm, baseName, addr, (u8)data);
                break;
            case LoadStoreOp::SH:
                std::printf("[%03X:%08X] sh %s, %04X(%s); [%03X] = %04X\n", pc, instr.raw, rtName, imm, baseName, addr, (u16)data);
                break;
            case LoadStoreOp::SW:
                std::printf("[%03X:%08X] sw %s, %04X(%s); [%03X] = %08X\n", pc, instr.raw, rtName, imm, baseName, addr, data);
                break;
        }
    }

    switch (op) {
        case LoadStoreOp::LB:
            set(rt, (u32)(i8)read<u8>(addr));
            break;
        case LoadStoreOp::LBU:
            set(rt, (u32)read<u8>(addr));
            break;
        case LoadStoreOp::LH:
            set(rt, (u32)(i16)read<u16>(addr));
            break;
        case LoadStoreOp::LHU:
            set(rt, (u32)read<u16>(addr));
            break;
        case LoadStoreOp::LW:
            set(rt, read<u32>(addr));
            break;
        case LoadStoreOp::SB:
            write(addr, (u8)get(rt));
            break;
        case LoadStoreOp::SH:
            write(addr, (u16)get(rt));
            break;
        case LoadStoreOp::SW:
            write(addr, get(rt));
            break;
    }
}

void LDV(const VUInstruction instr) {
    const u32 base = instr.loadType.base;
    const u32 vt = instr.loadType.vt;

    u32 element = instr.loadType.element;

    const u32 offset = (u32)(((i32)instr.loadType.offset << 25) >> 22);

    u32 addr = (get(base) + offset) & 0xFFF;

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *baseName = REG_NAMES[base];

        const u32 pc = getCurrentPC();

        std::printf("[%03X:%08X] ldv v%u[%u], %03X(%s); v%u[%u] = [%03X]\n", pc, instr.raw, vt, element, instr.loadType.offset << 4, baseName, vt, element, addr);
    }

    u32 lastElement = element + 7;
    if (lastElement > 15) {
        lastElement = 15;
    }

    for (; element <= lastElement; element++, addr++) {
        regFile.setByte(vt, element, read<u8>(addr & 0xFFF));
    }
}

void LQV(const VUInstruction instr) {
    const u32 base = instr.loadType.base;
    const u32 vt = instr.loadType.vt;

    const u32 element = instr.loadType.element;

    const u32 offset = (u32)(((i32)instr.loadType.offset << 25) >> 21);

    const u32 addr = (get(base) + offset) & 0xFFF;

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *baseName = REG_NAMES[base];

        const u32 pc = getCurrentPC();

        std::printf("[%03X:%08X] lqv v%u[%u], %03X(%s); v%u[%u] = [%03X]\n", pc, instr.raw, vt, element, instr.loadType.offset << 4, baseName, vt, element, addr);
    }

    u32 i = 0;
    while ((addr + i) <= ((addr & 0xFF0) + 15)) {
        regFile.setByte(vt, (element + i) & 15, read<u8>(addr + i));

        i += 1;
    }
}

void SDV(const VUInstruction instr) {
    const u32 base = instr.loadType.base;
    const u32 vt = instr.loadType.vt;

    const u32 element = instr.loadType.element;

    const u32 offset = (u32)(((i32)instr.loadType.offset << 25) >> 22);

    const u32 addr = (get(base) + offset) & 0xFFF;

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *baseName = REG_NAMES[base];

        const u32 pc = getCurrentPC();

        std::printf("[%03X:%08X] sdv v%u[%u], %03X(%s); v%u[%u] = [%03X]\n", pc, instr.raw, vt, element, instr.loadType.offset << 4, baseName, vt, element, addr);
    }

    for (u32 i = 0; i < 8; i++) {
        write((addr + i) & 0xFFF, regFile.getByte(vt, (element + i) & 15));
    }
}

void SQV(const VUInstruction instr) {
    const u32 base = instr.loadType.base;
    const u32 vt = instr.loadType.vt;

    const u32 element = instr.loadType.element;

    const u32 offset = (u32)(((i32)instr.loadType.offset << 25) >> 21);

    const u32 addr = (get(base) + offset) & 0xFFF;

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *baseName = REG_NAMES[base];

        const u32 pc = getCurrentPC();

        std::printf("[%03X:%08X] sqv v%u[%u], %03X(%s); v%u[%u] = [%03X]\n", pc, instr.raw, vt, element, instr.loadType.offset << 4, baseName, vt, element, addr);
    }

    u32 i = 0;
    while ((addr + i) <= ((addr & 0xFF0) + 15)) {
        write(addr + i, regFile.getByte(vt, (element + i) & 15));

        i += 1;
    }
}

void SSV(const VUInstruction instr) {
    const u32 base = instr.loadType.base;
    const u32 vt = instr.loadType.vt;

    const u32 element = instr.loadType.element;

    const u32 offset = (u32)(((i32)instr.loadType.offset << 25) >> 24);

    const u32 addr = (get(base) + offset) & 0xFFF;

    if constexpr (ENABLE_DISASSEMBLER) {
        const char *baseName = REG_NAMES[base];

        const u32 pc = getCurrentPC();

        std::printf("[%03X:%08X] ssv v%u[%u], %03X(%s); v%u[%u] = [%03X]\n", pc, instr.raw, vt, element, instr.loadType.offset << 4, baseName, vt, element, addr);
    }

    write(addr, regFile.getLane(vt, element >> 1));
}

void VMACF(const VUInstruction instr) {
    const u32 vd = instr.computeType.vd;
    const u32 vs = instr.computeType.vs;
    const u32 vt = instr.computeType.vt;

    const u32 broadcastMod = instr.computeType.broadcastMod;

    Accumulator &acc = regFile.acc;

    VectorRegister &vsData = regFile.vuRegs[vs];
    VectorRegister vtData = regFile.broadcast(vt, broadcastMod);

    for (u64 i = 0; i < NUM_LANES; i++) {
        const i32 product = (i32)vsData.getSignedLane(i) * (i32)vtData.getSignedLane(i) * 2;

        acc.setSignedLane(i, acc.getSignedLane(i) + (i64)product);

        regFile.setLane(vd, i, clampSigned(acc.getSignedLane(i) >> 16));
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        const u32 pc = getCurrentPC();

        std::printf("[%03X:%08X] vmacf v%u, v%u, v%u[%u]\n", pc, instr.raw, vd, vs, vt, broadcastMod);
    }
}

void VMULF(const VUInstruction instr) {
    const u32 vd = instr.computeType.vd;
    const u32 vs = instr.computeType.vs;
    const u32 vt = instr.computeType.vt;

    const u32 broadcastMod = instr.computeType.broadcastMod;

    Accumulator &acc = regFile.acc;

    VectorRegister &vsData = regFile.vuRegs[vs];
    VectorRegister vtData = regFile.broadcast(vt, broadcastMod);

    for (u64 i = 0; i < NUM_LANES; i++) {
        const i32 product = (i32)vsData.getSignedLane(i) * (i32)vtData.getSignedLane(i) * 2 + 0x8000;

        acc.setSignedLane(i, product);

        regFile.setLane(vd, i, clampSigned(acc.getSignedLane(i) >> 16));
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        const u32 pc = getCurrentPC();

        std::printf("[%03X:%08X] vmulf v%u, v%u, v%u[%u]\n", pc, instr.raw, vd, vs, vt, broadcastMod);
    }
}

void VXOR(const VUInstruction instr) {
    const u32 vd = instr.computeType.vd;
    const u32 vs = instr.computeType.vs;
    const u32 vt = instr.computeType.vt;

    const u32 broadcastMod = instr.computeType.broadcastMod;

    Accumulator &acc = regFile.acc;

    VectorRegister &vsData = regFile.vuRegs[vs];
    VectorRegister vtData = regFile.broadcast(vt, broadcastMod);

    for (u64 i = 0; i < NUM_LANES; i++) {
        acc.setShort(i, 0, vsData.getLane(i) ^ vtData.getLane(i));

        regFile.setLane(vd, i, acc.getShort(i, 0));
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        const u32 pc = getCurrentPC();

        std::printf("[%03X:%08X] vxor v%u, v%u, v%u[%u]\n", pc, instr.raw, vd, vs, vt, broadcastMod);
    }
}

void doInstruction() {
    Instruction instr;
    instr.raw = fetch();

    const u32 op = instr.iType.op;
    switch (op) {
        case Opcode::SPECIAL:
            {
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
                    case SpecialOpcode::JR:
                        doJump<JumpOp::JR>(instr);
                        break;
                    case SpecialOpcode::BREAK:
                        if constexpr (ENABLE_DISASSEMBLER) {
                            std::printf("[%03X:%08X] break\n", getCurrentPC(), instr.raw);
                        }

                        sp::BREAK();
                        break;
                    case SpecialOpcode::ADD:
                        doALURegister<ALUOpReg::ADD>(instr);
                        break;
                    case SpecialOpcode::SUB:
                        doALURegister<ALUOpReg::SUB>(instr);
                        break;
                    case SpecialOpcode::AND:
                        doALURegister<ALUOpReg::AND>(instr);
                        break;
                    case SpecialOpcode::OR:
                        doALURegister<ALUOpReg::OR>(instr);
                        break;
                    case SpecialOpcode::NOR:
                        doALURegister<ALUOpReg::NOR>(instr);
                        break;
                    default:
                        PLOG_FATAL << "Unrecognized function " << std::hex << funct << " (instruction = " << instr.raw << ", PC = " << getCurrentPC() << ")";

                        exit(0);
                }
            }   
            break;
        case Opcode::REGIMM: {
                const u32 op = instr.iType.rt;
                switch (op) {
                    case RegimmOpcode::BLTZ:
                        doBranch<BranchOp::BLTZ>(instr);
                        break;
                    case RegimmOpcode::BGEZ:
                        doBranch<BranchOp::BGEZ>(instr);
                        break;
                    default:
                        PLOG_FATAL << "Unrecognized REGIMM opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getCurrentPC() << ")";

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
        case Opcode::ANDI:
            doALUImmediate<ALUOpImm::ANDI>(instr);
            break;
        case Opcode::ORI:
            doALUImmediate<ALUOpImm::ORI>(instr);
            break;
        case Opcode::LUI:
            doALUImmediate<ALUOpImm::LUI>(instr);
            break;
        case Opcode::COP0:
            doCoprocessor<0>(instr);
            break;
        case Opcode::COP2:
            doCoprocessor<2>(instr);
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
        case Opcode::SB:
            doLoadStore<LoadStoreOp::SB>(instr);
            break;
        case Opcode::SH:
            doLoadStore<LoadStoreOp::SH>(instr);
            break;
        case Opcode::SW:
            doLoadStore<LoadStoreOp::SW>(instr);
            break;
        case Opcode::LWC2:
            {
                const VUInstruction vuInstr{.raw = instr.raw};

                const u32 op = vuInstr.loadType.opcode;
                switch (op) {
                    case VULoadOpcode::LDV:
                        LDV(vuInstr);
                        break;
                    case VULoadOpcode::LQV:
                        LQV(vuInstr);
                        break;
                    default:
                        PLOG_FATAL << "Unrecognized VU load opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getCurrentPC() << ")";

                        exit(0);
                }
            }
            break;
        case Opcode::SWC2:
            {
                const VUInstruction vuInstr{.raw = instr.raw};

                const u32 op = vuInstr.loadType.opcode;
                switch (op) {
                    case VUStoreOpcode::SSV:
                        SSV(vuInstr);
                        break;
                    case VUStoreOpcode::SDV:
                        SDV(vuInstr);
                        break;
                    case VUStoreOpcode::SQV:
                        SQV(vuInstr);
                        break;
                    default:
                        PLOG_FATAL << "Unrecognized VU store opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getCurrentPC() << ")";

                        exit(0);
                }
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized opcode " << std::hex << op << " (instruction = " << instr.raw << ", PC = " << getCurrentPC() << ")";

            exit(0);
    }
}

void run(const i64 cycles) {
    for (i64 i = 0; i < cycles; i++) {
        if (sp::isHalted()) {
            return;
        }

        regFile.cpc.addr = getPC();

        doInstruction();
    }
}

}
