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
#include "hw/si.hpp"

namespace hw::sm5 {

constexpr bool ENABLE_DISASSEMBLER = false;

namespace Opcode {
    enum : u8 {
        RC = 0x60,
        SC = 0x61,
        ID = 0x62,
        IE = 0x63,
        EXAX = 0x64,
        ATX = 0x65,
        EXBM = 0x66,
        EXBL = 0x67,
        EX = 0x68,
        PAT = 0x6A,
        TABL = 0x6B,
        TB = 0x6D,
        TC = 0x6E,
        TAM = 0x6F,
        OUTL = 0x71,
        IN = 0x74,
        OUT = 0x75,
        HALT = 0x77,
        INCB = 0x78,
        COMA = 0x79,
        ADD = 0x7A,
        ADC = 0x7B,
        DECB = 0x7C,
        RTN = 0x7D,
        RTNS = 0x7E,
        RTNI = 0x7F,
        TR = 0x80,
        TRS = 0xC0,
        TL = 0xE0,
        CALL = 0xF0,
    };
}

namespace Imm2Opcode {
    enum : u8 {
        RM = 0x10,
        SM = 0x11,
        TM = 0x12,
        TPB = 0x13,
        LDA = 0x14,
        EXC = 0x15,
        EXCI = 0x16,
        EXCD = 0x17,
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
        JoyData = 0,
        JoyBus = 2,
        JoyStatus = 3,
        JoyError = 4,
        CIC = 5,
        BootROMDisable = 6,
        RCP = 7,
        RNG = 9,
        JoyChannelSelect = 10,
        InterruptEnable = 14,
    };
}

namespace JoyError {
    enum : u8 {
        Error = 1 << 3,
    };
}

SM5::SM5() {}

SM5::~SM5() {}

void SM5::reset() {
    std::memset(&regs, 0, sizeof(Registers));

    isOnStandby = false;
}

void SM5::checkInterruptPending() {
    PLOG_INFO << "IME = " << regs.ime << ", IE = " << std::hex << (u16)regs.ie.raw << ", IF = " << (u16)regs.ifl.raw;

    // TODO: clear HALT mode
    if ((regs.ie.raw & regs.ifl.raw) == 0) {
        return;
    }

    if (isOnStandby) {
        PLOG_VERBOSE << "Standby mode exited";

        isOnStandby = false;

        regs.pc.pu = 3;
        regs.pc.pl = 0;
    }

    if (regs.ime) {
        push();

        regs.pc.pu = 2;

        if ((regs.ie.ifa & regs.ifl.ifa) != 0) {
            PLOG_VERBOSE << "Interrupt A triggered";

            regs.pc.pl = 0;
        } else if ((regs.ie.ifb & regs.ifl.ifb) != 0) {
            PLOG_VERBOSE << "Interrupt B triggered";

            regs.pc.pl = 2;
        } else if ((regs.ie.ift & regs.ifl.ift) != 0) {
            PLOG_VERBOSE << "Timer interrupt triggered";

            regs.pc.pl = 4;
        }

        regs.ime = false;
    }
}

void SM5::setInterruptAPending() {
    regs.ifl.ifa = 1;

    checkInterruptPending();
}

void SM5::setRCPPort(const bool isRead, const bool is64B) {
    regs.rcpPort.isRead = isRead;
    regs.rcpPort.is64B = is64B;
}

u8 SM5::fetch() {
    const u8 data = read(regs.pc.raw);

    regs.pc.pl++;

    return data;
}

u8 SM5::readPort(const u8 port) {
    switch (port) {
        case Port::JoyStatus:
            PLOG_WARNING << "Read from Joy Status";

            return 0xA;
        case Port::JoyError:
            PLOG_WARNING << "Read from Joy Error";

            return JoyError::Error;
        case Port::CIC:
            return hw::cic::read();
        case Port::RCP:
            PLOG_VERBOSE << "Read from RCP";

            return regs.rcpPort.raw;
        case Port::RNG:
            PLOG_WARNING << "Read from RNG";

            return 0xFF;
        case Port::JoyChannelSelect:
            PLOG_WARNING << "Read from Joy Channel Select";

            return regs.joyChannel;
        default:
            PLOG_FATAL << "Unrecognized read from port " << (u16)port;

            exit(0);
    }
}

void SM5::writePort(const u8 port, const u8 data) {
    switch (port) {
        case Port::JoyData:
            PLOG_WARNING << "Write to Joy Data (data = " << std::hex << (u16)(data & 0xF) << ")";
            break;
        case Port::JoyBus:
            PLOG_WARNING << "Write to JoyBus (data = " << std::hex << (u16)(data & 0xF) << ")";
            break;
        case Port::JoyError:
            PLOG_WARNING << "Write to Joy Error (data = " << std::hex << (u16)(data & 0xF) << ")";
            break;
        case Port::CIC:
            return hw::cic::write(data & 0xF);
        case Port::BootROMDisable:
            PLOG_VERBOSE << "Write to Boot ROM Disable (data = " << std::hex << (u16)(data & 0xF) << ")";
            break;
        case Port::RNG:
            PLOG_WARNING << "Write to RNG (data = " << std::hex << (u16)(data & 0xF) << ")";
            break;
        case Port::JoyChannelSelect:
            PLOG_WARNING << "Write to Joy Channel Select (data = " << std::hex << (u16)(data & 0xF) << ")";

            regs.joyChannel = data & 0xF;
            break;
        case Port::InterruptEnable:
            PLOG_VERBOSE << "Write to Interrupt Enable (data = " << std::hex << (u16)(data & 0xF) << ")";

            regs.ie.raw = data & 0xF;

            if (regs.ie.ifa == 1) {
                PLOG_VERBOSE << "Port 1 bit 0 interrupt enabled";
            }

            if (regs.ie.ifb == 1) {
                PLOG_VERBOSE << "Port 1 bit 1 interrupt enabled";
            }

            if (regs.ie.ift == 1) {
                PLOG_WARNING << "Timer interrupt enabled";
            }

            checkInterruptPending();
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
    // This skips all 1-byte opcodes
    const u8 op = fetch() & 0xF0;

    // Skip 2-byte opcodes
    if ((op == Opcode::TL) || (op == Opcode::CALL)) {
        regs.pc.pl++;
    }
}

void SM5::ADC(const Instruction instr) {
    const u8 res = regs.xa.a + readRAM(regs.b.raw) + (u8)regs.carry;

    regs.xa.a = res & 0xF;

    if (res > 0xF) {
        regs.carry = true;
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] adc\n", regs.oldPC, instr.raw);
    }
}

void SM5::ADD(const Instruction instr) {
    regs.xa.a += readRAM(regs.b.raw);

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] add\n", regs.oldPC, instr.raw);
    }
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

void SM5::ATX(const Instruction instr) {
    regs.xa.x = regs.xa.a;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] atx\n", regs.oldPC, instr.raw);
    }
}

void SM5::CALL(const Instruction instr) {
    const u16 pc = ((u16)(instr.raw & 0xF) << 8) | (u16)fetch();

    push();

    regs.pc.raw = pc;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] call %03X\n", regs.oldPC, instr.raw, regs.pc.raw);
    }
}

void SM5::COMA(const Instruction instr) {
    regs.xa.a ^= 0xF;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] coma\n", regs.oldPC, instr.raw);
    }
}

void SM5::DECB(const Instruction instr) {
    regs.b.l--;

    if (regs.b.l == 15) {
        skip();
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] decb\n", regs.oldPC, instr.raw);
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

void SM5::EXBM(const Instruction instr) {
    const u8 temp = regs.xa.a;

    regs.xa.a = regs.b.m;
    regs.b.m = temp;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] exbm\n", regs.oldPC, instr.raw);
    }
}

void SM5::EXC(const Instruction instr) {
    const u8 imm = instr.imm2Type.immediate;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] exc #%x\n", regs.oldPC, instr.raw, imm);
    }

    const u8 temp = regs.xa.a;
    const u8 paddr = regs.b.raw;

    regs.xa.a = readRAM(paddr);
    writeRAM(paddr, temp);

    regs.b.m ^= imm;
}

void SM5::EXCD(const Instruction instr) {
    const u8 imm = instr.imm2Type.immediate;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] excd #%x\n", regs.oldPC, instr.raw, imm);
    }

    const u8 temp = regs.xa.a;
    const u8 paddr = regs.b.raw;

    regs.xa.a = readRAM(paddr);
    writeRAM(paddr, temp);

    regs.b.l--;
    regs.b.m ^= imm;

    if (regs.b.l == 15) {
        skip();
    }
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

void SM5::HALT(const Instruction instr) {
    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] id\n", regs.oldPC, instr.raw);
    }

    PLOG_VERBOSE << "Standby mode entered";

    isOnStandby = true;

    if (regs.rcpPort.is64B == 0) {
        PLOG_FATAL << "Unimplemented 4-byte transfer";

        exit(0);
    }

    if (regs.rcpPort.isRead != 0) {
        si::doDMAFromPIF();
    } else {
        si::doDMAToPIF();
    }
}

void SM5::ID(const Instruction instr) {
    regs.ime = false;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] id\n", regs.oldPC, instr.raw);
    }
}

void SM5::IE(const Instruction instr) {
    regs.ime = true;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] ie\n", regs.oldPC, instr.raw);
    }

    checkInterruptPending();
}

void SM5::IN(const Instruction instr) {
    const u8 port = regs.b.l;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] in\n", regs.oldPC, instr.raw);
    }

    const u8 data = readPort(port);

    regs.xa.a = data & 0xF;
}

void SM5::INCB(const Instruction instr) {
    regs.b.l++;

    if (regs.b.l == 0) {
        skip();
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] incb\n", regs.oldPC, instr.raw);
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

void SM5::LDA(const Instruction instr) {
    const u8 imm = instr.imm2Type.immediate;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] lda #%x\n", regs.oldPC, instr.raw, imm);
    }

    regs.xa.a = readRAM(regs.b.raw);
    regs.b.m ^= imm;
}

void SM5::OUT(const Instruction instr) {
    const u8 port = regs.b.l;
    const u8 data = regs.xa.raw;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] out\n", regs.oldPC, instr.raw);
    }

    writePort(port, data);
}

void SM5::OUTL(const Instruction instr) {
    const u8 data = regs.xa.raw;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] outl\n", regs.oldPC, instr.raw);
    }

    writePort(0, data);
}

void SM5::PAT(const Instruction instr) {
    push();

    regs.pc.pu = 4;
    regs.pc.pl = regs.xa.raw & 0x3F;

    regs.xa.raw = read(regs.pc.raw);

    pop();

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] pat\n", regs.oldPC, instr.raw);
    }
}

void SM5::RC(const Instruction instr) {
    regs.carry = false;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] rc\n", regs.oldPC, instr.raw);
    }
}

void SM5::RM(const Instruction instr) {
    const u8 imm = instr.imm2Type.immediate;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] rm #%x\n", regs.oldPC, instr.raw, imm);
    }

    const u8 data = readRAM(regs.b.raw) & ~(1 << imm);
    writeRAM(regs.b.raw, data);
}

void SM5::RTN(const Instruction instr) {
    pop();

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] rtn\n", regs.oldPC, instr.raw);
    }
}

void SM5::RTNI(const Instruction instr) {
    pop();

    regs.ime = true;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] rtni\n", regs.oldPC, instr.raw);
    }

    checkInterruptPending();
}

void SM5::RTNS(const Instruction instr) {
    pop();
    skip();

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] rtns\n", regs.oldPC, instr.raw);
    }
}

void SM5::SC(const Instruction instr) {
    regs.carry = true;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] sc\n", regs.oldPC, instr.raw);
    }
}

void SM5::SM(const Instruction instr) {
    const u8 imm = instr.imm2Type.immediate;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] sm #%x\n", regs.oldPC, instr.raw, imm);
    }

    const u8 data = readRAM(regs.b.raw) | (1 << imm);
    writeRAM(regs.b.raw, data);
}

void SM5::TABL(const Instruction instr) {
    if (regs.xa.a == regs.b.l) {
        skip();
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] tabl\n", regs.oldPC, instr.raw);
    }
}

void SM5::TAM(const Instruction instr) {
    if (regs.xa.a == readRAM(regs.b.raw)) {
        skip();
    }

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] tam\n", regs.oldPC, instr.raw);
    }
}

void SM5::TB(const Instruction instr) {
    if (regs.ifl.ifb != 0) {
        skip();
    }

    regs.ifl.ifb = 0;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] tb\n", regs.oldPC, instr.raw);
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

void SM5::TM(const Instruction instr) {
    const u8 imm = instr.imm2Type.immediate;

    if constexpr (ENABLE_DISASSEMBLER) {
        std::printf("[%03X:%02X] tm #%x\n", regs.oldPC, instr.raw, imm);
    }

    if (regs.oldPC == 0xF7) {
        return;
    }

    if ((readRAM(regs.b.raw) & (1 << imm)) != 0) {
        skip();
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
        case Imm2Opcode::RM:
            return RM(instr);
        case Imm2Opcode::SM:
            return SM(instr);
        case Imm2Opcode::TM:
            return TM(instr);
        case Imm2Opcode::TPB:
            return TPB(instr);
        case Imm2Opcode::LDA:
            return LDA(instr);
        case Imm2Opcode::EXC:
            return EXC(instr);
        case Imm2Opcode::EXCI:
            return EXCI(instr);
        case Imm2Opcode::EXCD:
            return EXCD(instr);
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

    if ((op & 0xF0) == Opcode::CALL) {
        return CALL(instr);
    }

    switch (op) {
        case Opcode::RC:
            return RC(instr);
        case Opcode::SC:
            return SC(instr);
        case Opcode::ID:
            return ID(instr);
        case Opcode::IE:
            return IE(instr);
        case Opcode::EXAX:
            return EXAX(instr);
        case Opcode::ATX:
            return ATX(instr);
        case Opcode::EXBM:
            return EXBM(instr);
        case Opcode::EXBL:
            return EXBL(instr);
        case Opcode::EX:
            return EX(instr);
        case Opcode::PAT:
            return PAT(instr);
        case Opcode::TABL:
            return TABL(instr);
        case Opcode::TB:
            return TB(instr);
        case Opcode::TC:
            return TC(instr);
        case Opcode::TAM:
            return TAM(instr);
        case Opcode::OUTL:
            return OUTL(instr);
        case Opcode::IN:
            return IN(instr);
        case Opcode::OUT:
            return OUT(instr);
        case Opcode::HALT:
            return HALT(instr);
        case Opcode::INCB:
            return INCB(instr);
        case Opcode::COMA:
            return COMA(instr);
        case Opcode::ADD:
            return ADD(instr);
        case Opcode::ADC:
            return ADC(instr);
        case Opcode::DECB:
            return DECB(instr);
        case Opcode::RTN:
            return RTN(instr);
        case Opcode::RTNS:
            return RTNS(instr);
        case Opcode::RTNI:
            return RTNI(instr);
        default:
            PLOG_FATAL << "Unrecognized instruction " << std::hex << (u16)op << " (PC = " << regs.oldPC << ")";

            exit(0);
    }
}

void SM5::run(const i64 cycles) {
    for (i64 i = 0; i < cycles; i++) {
        if (isOnStandby) {
            return;
        }

        doInstruction();
    }
}

}
