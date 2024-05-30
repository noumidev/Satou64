/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::sm5 {

constexpr u64 STACK_DEPTH = 4;

struct Registers {
    // Accumulator
    union {
        u8 raw;
        struct {
            u8 a : 4;
            u8 x : 4;
        };
    } xa;

    bool carry;

    // RAM address registers
    union {
        u8 raw;
        struct {
            u8 l : 4;
            u8 m : 4;
        };
    } b, sb;

    // Program counter
    union {
        u16 raw;
        struct {
            u16 pl : 6;
            u16 pu : 6;
            u16 : 4;
        };
    } pc;

    u16 oldPC;

    // Stack and stack pointer
    union {
        u16 raw;
        struct {
            u16 data : 12;
            u16 : 4;
        };
    } sr[STACK_DEPTH];
    u64 sp;

    // Interrupt flags A/B/T
    union {
        u8 raw;
        struct {
            u8 ifa : 1;
            u8 ifb : 1;
            u8 ift : 1;
            u8 : 5;
        };
    } ie;

    // Interrupt master enable
    bool ime;

    // Clock divider
    u16 div;
};

union Instruction {
    u8 raw;

    // 4-bit immediate type
    struct {
        u8 immediate : 4;
        u8 op : 4;
    } imm4Type;

    // 2-bit immediate type
    struct {
        u8 immediate : 2;
        u8 op : 6;
    } imm2Type;
};

struct SM5 {
private:
    Registers regs;

    // Reads immediate data, increments PC
    u8 fetch();

    u8 readPort(const u8 port);

    void writePort(const u8 port, const u8 data);

    void push();
    void pop();

    void skip();

    void ADC(const Instruction instr);
    void ADD(const Instruction instr);
    void ADX(const Instruction instr);
    void ATX(const Instruction instr);
    void CALL(const Instruction instr);
    void COMA(const Instruction instr);
    void DECB(const Instruction instr);
    void EX(const Instruction instr);
    void EXAX(const Instruction instr);
    void EXBL(const Instruction instr);
    void EXBM(const Instruction instr);
    void EXC(const Instruction instr);
    void EXCD(const Instruction instr);
    void EXCI(const Instruction instr);
    void ID(const Instruction instr);
    void IE(const Instruction instr);
    void INCB(const Instruction instr);
    void LAX(const Instruction instr);
    void LBLX(const Instruction instr);
    void LBMX(const Instruction instr);
    void LDA(const Instruction instr);
    void OUT(const Instruction instr);
    void PAT(const Instruction instr);
    void RC(const Instruction instr);
    void RM(const Instruction instr);
    void RTN(const Instruction instr);
    void RTNS(const Instruction instr);
    void SC(const Instruction instr);
    void SM(const Instruction instr);
    void TAM(const Instruction instr);
    void TB(const Instruction instr);
    void TC(const Instruction instr);
    void TL(const Instruction instr);
    void TM(const Instruction instr);
    void TPB(const Instruction instr);
    void TR(const Instruction instr);
    void TRS(const Instruction instr);

    void doInstruction();

public:
    SM5();
    ~SM5();

    void reset();

    u8 (*read)(const u16 paddr);
    u8 (*readRAM)(const u8 paddr);

    void (*write)(const u16 paddr, const u8 data);
    void (*writeRAM)(const u8 paddr, const u8 data);

    void run(const i64 cycles);
};

}
