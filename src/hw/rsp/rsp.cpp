/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/rsp/rsp.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/sp.hpp"
#include "hw/cpu/cpu.hpp"

#include "sys/memory.hpp"

namespace hw::rsp {

using cpu::Instruction;

constexpr bool ENABLE_DISASSEMBLER = true;

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
        ADDI = 0x08,
        ORI = 0x0D,
        LUI = 0x0F,
        LW = 0x23,
        SW = 0x2B,
    };
}

namespace SpecialOpcode {
    enum : u32 {
        BREAK = 0x0D,
        ADD = 0x20,
    };
}

enum class ALUOpImm {
    ADDI,
    LUI,
    ORI,
};

enum class ALUOpReg {
    ADD,
};

enum class LoadStoreOp {
    LW,
    SW,
};

struct Registers {
    u32 regs[Register::NumberOfRegisters];

    union {
        u32 raw;
        struct {
            u32 addr : 12;
            u32 : 20;
        };
    } pc, npc, cpc;
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
        }
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
            case LoadStoreOp::LW:
                std::printf("[%03X:%08X] lw %s, %04X(%s); %s = [%03X]\n", pc, instr.raw, rtName, imm, baseName, rtName, addr);
                break;
            case LoadStoreOp::SW:
                std::printf("[%03X:%08X] sw %s, %04X(%s); [%03X] = %08X\n", pc, instr.raw, rtName, imm, baseName, addr, data);
                break;
        }
    }

    switch (op) {
        case LoadStoreOp::LW:
            set(rt, read<u32>(addr));
            break;
        case LoadStoreOp::SW:
            write(addr, get(rt));
            break;
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
                    case SpecialOpcode::BREAK:
                        if constexpr (ENABLE_DISASSEMBLER) {
                            std::printf("[%03X:%08X] break\n", getCurrentPC(), instr.raw);
                        }

                        sp::BREAK();
                        break;
                    case SpecialOpcode::ADD:
                        doALURegister<ALUOpReg::ADD>(instr);
                        break;
                    default:
                        PLOG_FATAL << "Unrecognized function " << std::hex << funct << " (instruction = " << instr.raw << ", PC = " << getCurrentPC() << ")";

                        exit(0);
                }
            }   
            break;
        case Opcode::ADDI:
            doALUImmediate<ALUOpImm::ADDI>(instr);
            break;
        case Opcode::ORI:
            doALUImmediate<ALUOpImm::ORI>(instr);
            break;
        case Opcode::LUI:
            doALUImmediate<ALUOpImm::LUI>(instr);
            break;
        case Opcode::LW:
            doLoadStore<LoadStoreOp::LW>(instr);
            break;
        case Opcode::SW:
            doLoadStore<LoadStoreOp::SW>(instr);
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
