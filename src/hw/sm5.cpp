/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/sm5.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/cic.hpp"

namespace hw::sm5 {

constexpr bool ENABLE_DISASSEMBLER = true;

namespace Opcode {
    enum : u8 {
        RC = 0x60,
        SC = 0x61,
        EXAX = 0x64,
        EXBL = 0x67,
        EX = 0x68,
        TC = 0x6E,
        OUT = 0x75,
        RTN = 0x7D,
        TR = 0x80,
        TRS = 0xC0,
        TL = 0xE0,
    };
}

namespace Imm2Opcode {
    enum : u8 {
        TPB = 0x13,
        EXC = 0x15,
        EXCI = 0x16,
    };
}

namespace Imm4Opcode {
    enum : u8 {
        ADX = 0,
        LAX = 1,
        LBLX = 2,
        LBMX = 3,
    };
}

namespace Port {
    enum : u8 {
        CIC = 5,
        InterruptEnable = 14,
    };
}

SM5::SM5() {}

SM5::~SM5() {}

void SM5::reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

u8 SM5::fetch() {
    const u8 data = read(regs.pc.raw);

    skip();

    return data;
}

u8 SM5::readPort(const u8 port) {
    switch (port) {
        case Port::CIC:
            return hw::cic::read();
        default:
            PLOG_FATAL << "Unrecognized read from port " << (u16)port;

            exit(0);
    }
}

void SM5::writePort(const u8 port, const u8 data) {
    switch (port) {
        case Port::CIC:
            return hw::cic::write(data & 0xF);
        case Port::InterruptEnable:
            PLOG_VERBOSE << "Write to Interrupt Enable (data = " << std::hex << (u16)(data & 0xF) << ")";

            regs.ie.raw = data & 0xF;

            if (regs.ie.ifa == 1) {
                PLOG_WARNING << "Port 1 bit 0 interrupt enabled";
            }

            if (regs.ie.ifb == 1) {
                PLOG_WARNING << "Port 1 bit 1 interrupt enabled";
            }

            if (regs.ie.ift == 1) {
                PLOG_WARNING << "Timer interrupt enabled";
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized write to port " << (u16)port << " (data = " << std::hex << (u16)data << ")";

            exit(0);
    }
}

void SM5::push() {
    if (regs.sp >= STACK_DEPTH) {
        PLOG_FATAL << "Return address stack overflow";

        exit(0);
    }

    regs.sr[regs.sp].raw = regs.pc.raw;

    regs.sp++;
}

void SM5::pop() {
    if (regs.sp == 0) {
        PLOG_FATAL << "Return address stack underflow";

        exit(0);
    }

    regs.sp--;

    regs.pc.raw = regs.sr[regs.sp].data;
}

void SM5::skip() {
    regs.pc.pl++;
}

void SM5::ADX(const Instruction instr) {
    const u8 imm = instr.imm4Type.immediate;

    const u8 res = regs.xa.a + imm;
    regs.xa.a = res & 0xF;

    // Carry?
    if (res > 0xF) {
        skip();
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] adx #%01X\n", regs.oldPC, instr.raw, imm);
    }
}

void SM5::EX(const Instruction instr) {
    const u8 temp = regs.b.raw;

    regs.b.raw = regs.sb.raw;
    regs.sb.raw = temp;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] ex\n", regs.oldPC, instr.raw);
    }
}

void SM5::EXAX(const Instruction instr) {
    const u8 temp = regs.xa.a;

    regs.xa.a = regs.xa.x;
    regs.xa.x = temp;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] exax\n", regs.oldPC, instr.raw);
    }
}

void SM5::EXBL(const Instruction instr) {
    const u8 temp = regs.xa.a;

    regs.xa.a = regs.b.l;
    regs.b.l = temp;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] exbl\n", regs.oldPC, instr.raw);
    }
}

void SM5::EXC(const Instruction instr) {
    const u8 imm = instr.imm2Type.immediate;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] exc #%x\n", regs.oldPC, instr.raw, imm);
    }

    const u8 temp = regs.xa.a;
    const u8 paddr = regs.b.raw;

    regs.xa.raw = readRAM(paddr);
    writeRAM(paddr, temp);

    regs.b.m ^= imm;
}

void SM5::EXCI(const Instruction instr) {
    const u8 imm = instr.imm2Type.immediate;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] exci #%x\n", regs.oldPC, instr.raw, imm);
    }

    const u8 temp = regs.xa.a;
    const u8 paddr = regs.b.raw;

    regs.xa.a = readRAM(paddr);
    writeRAM(paddr, temp);

    regs.b.l++;
    regs.b.m ^= imm;

    if (regs.b.l == 0) {
        skip();
    }
}

void SM5::LAX(const Instruction instr) {
    const u8 imm = instr.imm4Type.immediate;

    regs.xa.a = imm;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] lax #%01X\n", regs.oldPC, instr.raw, imm);
    }
}

void SM5::LBLX(const Instruction instr) {
    const u8 imm = instr.imm4Type.immediate;

    regs.b.l = imm;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] lblx #%01X\n", regs.oldPC, instr.raw, imm);
    }
}

void SM5::LBMX(const Instruction instr) {
    const u8 imm = instr.imm4Type.immediate;

    regs.b.m = imm;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] lbmx #%01X\n", regs.oldPC, instr.raw, imm);
    }
}

void SM5::OUT(const Instruction instr) {
    const u8 port = regs.b.l;
    const u8 data = regs.xa.raw;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] out\n", regs.oldPC, instr.raw);
    }

    writePort(port, data);
}

void SM5::RC(const Instruction instr) {
    regs.carry = false;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] rc\n", regs.oldPC, instr.raw);
    }
}

void SM5::RTN(const Instruction instr) {
    pop();

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] rtn\n", regs.oldPC, instr.raw);
    }
}

void SM5::SC(const Instruction instr) {
    regs.carry = true;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] sc\n", regs.oldPC, instr.raw);
    }
}

void SM5::TC(const Instruction instr) {
    if (regs.carry) {
        skip();
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] tc\n", regs.oldPC, instr.raw);
    }
}

void SM5::TL(const Instruction instr) {
    const u16 pc = ((u16)(instr.raw & 0xF) << 8) | (u16)fetch();

    regs.pc.raw = pc;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] tl %03X\n", regs.oldPC, instr.raw, regs.pc.raw);
    }
}

void SM5::TPB(const Instruction instr) {
    const u8 imm = instr.imm2Type.immediate;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] tpb #%x\n", regs.oldPC, instr.raw, imm);
    }

    const u8 port = regs.b.l;
    const u8 data = readPort(port);

    if ((data & (1 << imm)) != 0) {
        skip();
    }
}

void SM5::TR(const Instruction instr) {
    const u8 offset = instr.raw & 0x3F;

    regs.pc.pl = offset;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] tr %03X\n", regs.oldPC, instr.raw, regs.pc.raw);
    }
}

void SM5::TRS(const Instruction instr) {
    const u8 offset = instr.raw & 0x1F;

    push();

    regs.pc.pu = 1;
    regs.pc.pl = offset << 1;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] trs %03X\n", regs.oldPC, instr.raw, regs.pc.raw);
    }
}

void SM5::doInstruction() {
    regs.oldPC = regs.pc.raw;

    Instruction instr;
    instr.raw = fetch();

    // 2-bit immediate instructions
    switch (instr.imm2Type.op) {
        case Imm2Opcode::TPB:
            return TPB(instr);
        case Imm2Opcode::EXC:
            return EXC(instr);
        case Imm2Opcode::EXCI:
            return EXCI(instr);
        default:
            break;
    }

    // 4-bit immediate instructions
    switch (instr.imm4Type.op) {
        case Imm4Opcode::ADX:
            return ADX(instr);
        case Imm4Opcode::LAX:
            return LAX(instr);
        case Imm4Opcode::LBLX:
            return LBLX(instr);
        case Imm4Opcode::LBMX:
            return LBMX(instr);
        default:
            break;
    }

    const u8 op = instr.raw;
    if ((op & 0xC0) == Opcode::TR) {
        return TR(instr);
    }

    if ((op & 0xE0) == Opcode::TRS) {
        return TRS(instr);
    }

    if ((op & 0xF0) == Opcode::TL) {
        return TL(instr);
    }

    switch (op) {
        case Opcode::RC:
            return RC(instr);
        case Opcode::SC:
            return SC(instr);
        case Opcode::EXAX:
            return EXAX(instr);
        case Opcode::EXBL:
            return EXBL(instr);
        case Opcode::EX:
            return EX(instr);
        case Opcode::TC:
            return TC(instr);
        case Opcode::OUT:
            return OUT(instr);
        case Opcode::RTN:
            return RTN(instr);
        default:
            PLOG_FATAL << "Unrecognized instruction " << std::hex << (u16)op << " (PC = " << regs.oldPC << ")";

            exit(0);
    }
}

void SM5::run(const i64 cycles) {
    for (i64 i = 0; i < cycles; i++) {
        doInstruction();
    }
}

}
