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
    u8 b, sb;

    // Program counter
    union {
        u16 raw;
        struct {
            u16 pl : 6;
            u16 pu : 6;
            u16 : 4;
        };
    } pc;

    // Stack and stack pointer
    union {
        u16 raw;
        struct {
            u16 data : 12;
            u16 : 4;
        };
    } sr[STACK_DEPTH];
    u64 sp;

    // Interrupt flags A/B/T, master enable
    bool ifa, ifb, ift;
    bool ime;

    // Clock divider
    u16 div;
};

struct SM5 {
private:
    Registers regs;

    // Reads immediate data, increments PC
    u8 fetchImmediate();

    void doInstruction();

public:
    SM5();
    ~SM5();

    void reset();

    u8 (*read)(const u16 paddr);

    void (*write)(const u16 paddr, const u8 data);

    void run(const i64 cycles);
};

}
