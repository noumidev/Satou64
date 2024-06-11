/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include <type_traits>

#include "common/types.hpp"

namespace hw::rsp {

union VUInstruction {
    u32 raw;
    struct {
        u32 offset : 7;
        u32 element : 4;
        u32 opcode : 5;
        u32 vt : 5;
        u32 base : 5;
        u32 : 6;
    } loadType;
};

void init();
void deinit();

void reset();

// Returns true if register index is valid
bool isValidRegisterIndex(const u32 idx);

u32 get(const u32 idx);

u32 getPC();
u32 getCurrentPC();

void branch(const u32 target, const bool condition, const u32 linkReg);

void set(const u32 idx, const u32 data);

void setPC(const u32 addr);
void setBranchPC(const u32 addr);

void advancePC();

u32 fetch();

void LQV(const VUInstruction instr);

void doInstruction();

void run(const i64 cycles);

}
